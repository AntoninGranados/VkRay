#include "offline.hpp"

#include <cstdlib>
#include <format>
#include <fstream>
#include <random>
#include <stdexcept>

#include <glm/glm.hpp>

#include "core/core.hpp"
#include "core/export_service.hpp"
#include "core/fields/field_serializer.hpp"
#include "core/render_structures.hpp"
#include "core/scene/scene_serializer.hpp"

#include "utils/json_dsl.hpp"
#include "utils/log.hpp"
#include "utils/progress.hpp"

using json = nlohmann::ordered_json;

std::vector<Job> Offline::loadJobs(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open()) throw std::runtime_error(std::format("Cannot open job file [{}]", path.string()));

    json root = json::parse(f, nullptr, true, true);

    const int version = root.value("version", -1);
    if (version != kJobVersion)
        throw std::runtime_error(
            std::format("Job version mismatch in `{}`: expected {}, got {}", path.string(), kJobVersion, version));

    std::vector<Job> jobs;
    std::mt19937 rng(0);
    const std::filesystem::path jobDirectory = std::filesystem::absolute(path).parent_path();

    for (const auto& j : root.at("jobs")) {
        const uint32_t repeatCount = j.contains("repeat") ? j.at("repeat").value("count", 1u) : 1u;

        struct Checkpoint {
            uint32_t spp;
            std::vector<ParameterOverride> aovOverrides;
        };

        std::vector<ParameterOverride> parameterOverrides;
        if (j.contains("render_size"))
            parameterOverrides.push_back({"renderer/output/render_size", j.at("render_size")});
        if (j.contains("parameters"))
            for (const auto& [key, val] : j.at("parameters").items()) parameterOverrides.push_back({key, val});

        std::vector<Checkpoint> checkpoints;
        const auto& samplesJson = j.at("samples");
        for (const auto& entry : samplesJson.is_array() ? samplesJson : json::array({samplesJson})) {
            if (entry.is_number()) {
                checkpoints.push_back({entry.get<uint32_t>(), {}});
            } else {
                std::vector<ParameterOverride> cpAovs;
                if (entry.contains("aovs")) parseAovs(entry.at("aovs"), cpAovs);
                checkpoints.push_back({entry.at("spp").get<uint32_t>(), std::move(cpAovs)});
            }
        }

        std::vector<ParameterOverride> jobAovOverrides;
        if (j.contains("aovs")) parseAovs(j.at("aovs"), jobAovOverrides);

        for (uint32_t n = 0; n < repeatCount; ++n) {
            ResolveCtx nCtx{rng, {{"n", {static_cast<int>(n), static_cast<int>(repeatCount)}}}};
            const std::filesystem::path scenePath =
                (jobDirectory / resolveTemplate(j.at("scene").get<std::string>(), nCtx)).lexically_normal();
            const uint32_t nSeed = rng();

            for (size_t ci = 0; ci < checkpoints.size(); ++ci) {
                ResolveCtx ctx = nCtx;
                ctx.tokens["spp"] = {static_cast<int>(ci), static_cast<int>(checkpoints.size())};

                const std::vector<ParameterOverride>& aovOverrides =
                    checkpoints[ci].aovOverrides.empty() ? jobAovOverrides : checkpoints[ci].aovOverrides;

                Job job;
                job.scene = scenePath;
                job.seed = nSeed;
                job.parameterOverrides = parameterOverrides;
                job.parameterOverrides.push_back(
                    {"renderer/output/output_image",
                     (jobDirectory / resolveTemplate(j.at("output").get<std::string>(), ctx))
                         .lexically_normal()
                         .string()});
                job.parameterOverrides.push_back(
                    {"renderer/sampling/render_samples", static_cast<int>(checkpoints[ci].spp)});
                job.parameterOverrides.insert(job.parameterOverrides.end(), aovOverrides.begin(), aovOverrides.end());

                jobs.push_back(std::move(job));
            }
        }
    }

    return jobs;
}

int Offline::run(const std::vector<Job>& jobs) {
    const int totalJobs = static_cast<int>(jobs.size());

    int failedJobs = 0;
    int jobIndex = 0;
    for (const Job& job : jobs) {
        jobIndex++;
        initParameters(job.parameterOverrides);
        const glm::ivec2 renderSize = Core::getParameters().get<glm::ivec2>("renderer/output/render_size");
        Core::requestResize(renderSize.x, renderSize.y);
        Core::consumeResize();

        if (!SceneSerializer::load(Core::getScene(), job.scene.string(), job.seed)) {
            Log::error("Offline",
                       std::format("Failed to load the scene `{}` for the job {}", job.scene.string(), jobIndex));
            failedJobs++;
            continue;
        }
        Log::success("Offline", std::format("[{}/{}] Loaded: `{}`", jobIndex, totalJobs, job.scene.string()));
        Core::setRenderMode(RenderMode::RenderSingle);

        Core::getEngine().waitIdle();

        PathtraceRenderer& renderer = Core::getCoreRenderer();
        const uint32_t totalSamples = Core::getParameters().get<int>("renderer/sampling/render_samples");
        renderer.restartAccumulation();

        ProgressBar bar(std::format("[{}/{}]", jobIndex, totalJobs), totalSamples, "spp");
        while (!renderer.isRenderFinished()) {
            Core::renderFrame();
            bar.update(renderer.getSampleCount());
        }
        bar.close();

        const std::filesystem::path outputPath =
            !Core::getOutputPath().empty()
                ? Core::getOutputPath()
                : Core::getParameters().get<std::filesystem::path>("renderer/output/output_image");
        ExportService::save(renderer, outputPath);
    }
    return failedJobs == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}

void Offline::parseAovs(const json& names, std::vector<ParameterOverride>& overrides) {
    for (const auto& name : names) {
        const std::string s = name.get<std::string>();
        for (const char* aovName : kAOVNames) {
            if (s == aovName) {
                overrides.push_back({std::string("renderer/aov/") + aovName, true});
                break;
            }
        }
    }
}

void Offline::initParameters(const std::vector<ParameterOverride>& overrides) {
    Core::getParameters().resetAll();
    std::mt19937 rng(0);
    const ResolveCtx ctx{rng, {}};
    for (const ParameterOverride& parameterOverride : overrides)
        applyField(parameterOverride.value, Core::getParameters().getParam(parameterOverride.key), ctx);
}
