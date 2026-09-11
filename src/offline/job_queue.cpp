#include "job_queue.hpp"

#include <fstream>
#include <random>
#include <stdexcept>
#include <type_traits>

#include "nlohmann/json.hpp"

#include "core/fields/field_serializer.hpp"
#include "core/render_structures.hpp"
#include "utils/json_dsl.hpp"

using json = nlohmann::json;

static constexpr int kJobVersion = 1;

static void parseAovs(const json& arr, std::vector<ParameterOverride>& overrides) {
    for (const auto& name : arr) {
        const std::string s = name.get<std::string>();
        for (const AOVChannel& channel : kAOVChannels) {
            if (s == channel.name) {
                overrides.push_back({ std::string("renderer/aov/") + channel.name, true });
                break;
            }
        }
    }
}

JobQueue JobQueue::fromFile(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error(std::format("Cannot open job queue file [{}]", path.string()));

    json root = json::parse(f, nullptr, true, true);

    const int version = root.value("version", -1);
    if (version != kJobVersion)
        throw std::runtime_error(std::format(
            "Job version mismatch in `{}`: expected {}, got {}", path.string(), std::to_string(kJobVersion), std::to_string(version)
        ));

    JobQueue queue;
    std::mt19937 rng(0);

    for (const auto& j : root.at("jobs")) {
        const uint32_t repeatCount = j.contains("repeat") ? j.at("repeat").value("count", 1u) : 1u;

        struct Checkpoint {
            uint32_t                   spp;
            std::vector<ParameterOverride> aovOverrides;
        };

        std::vector<ParameterOverride> parameterOverrides;
        if (j.contains("render_size")) {
            const auto& rs = j.at("render_size");
            parameterOverrides.push_back({"renderer/output/render_size", glm::ivec2(rs[0].get<int>(), rs[1].get<int>())});
        }
        if (j.contains("parameters")) {
            for (const auto& [key, val] : j.at("parameters").items()) {
                std::optional<FieldValue> fv = inferFieldValueFromJson(val);
                if (!fv) continue;
                if (fv->getType() == FieldType::String) parameterOverrides.push_back({key, fv->get<std::string>()});
                else fv->dispatch([&](auto v) {
                    if constexpr (std::is_constructible_v<ParameterValue, decltype(v)>)
                        parameterOverrides.push_back({key, v});
                });
            }
        }

        std::vector<Checkpoint> checkpoints;
        const auto& samplesJson = j.at("samples");
        for (const auto& entry : samplesJson.is_array() ? samplesJson : json::array({samplesJson})) {
            if (entry.is_number()) {
                checkpoints.push_back({ entry.get<uint32_t>(), {} });
            } else {
                std::vector<ParameterOverride> cpAovs;
                if (entry.contains("aovs")) parseAovs(entry.at("aovs"), cpAovs);
                checkpoints.push_back({ entry.at("spp").get<uint32_t>(), std::move(cpAovs) });
            }
        }

        std::vector<ParameterOverride> jobAovOverrides;
        if (j.contains("aovs")) parseAovs(j.at("aovs"), jobAovOverrides);

        for (uint32_t n = 0; n < repeatCount; ++n) {
            ResolveCtx nCtx{ rng, {{"n", {static_cast<int>(n), static_cast<int>(repeatCount)}}} };
            const std::string scenePath = resolveTemplate(j.at("scene").get<std::string>(), nCtx);
            const uint32_t    nSeed     = rng();

            for (size_t ci = 0; ci < checkpoints.size(); ++ci) {
                ResolveCtx ctx = nCtx;
                ctx.tokens["spp"] = {static_cast<int>(ci), static_cast<int>(checkpoints.size())};

                const std::vector<ParameterOverride>& aovOverrides =
                    checkpoints[ci].aovOverrides.empty() ? jobAovOverrides : checkpoints[ci].aovOverrides;

                Job job;
                job.scene  = scenePath;
                job.seed   = nSeed;
                job.parameterOverrides = parameterOverrides;
                job.parameterOverrides.push_back({"renderer/output/output_image", std::filesystem::path(resolveTemplate(j.at("output").get<std::string>(), ctx))});
                job.parameterOverrides.push_back({"renderer/sampling/render_samples", static_cast<int>(checkpoints[ci].spp)});
                job.parameterOverrides.insert(job.parameterOverrides.end(), aovOverrides.begin(), aovOverrides.end());

                queue.enqueue(std::move(job));
            }
        }
    }

    return queue;
}

void JobQueue::enqueue(Job job) {
    jobs.push_back(std::move(job));
}

void JobQueue::cancel(size_t index) {
    if (index >= jobs.size()) return;
    if (static_cast<int>(index) == runningIndex) return;
    jobs.erase(jobs.begin() + static_cast<ptrdiff_t>(index));
    if (runningIndex > static_cast<int>(index)) runningIndex--;
}

void JobQueue::clearPending() {
    if (runningIndex >= 0) {
        int shift = 0;
        for (int i = 0; i < runningIndex; ++i)
            if (jobs[i].status == JobStatus::Pending) ++shift;
        runningIndex -= shift;
    }
    std::erase_if(jobs, [](const Job& j) { return j.status == JobStatus::Pending; });
}

bool JobQueue::isEmpty() const {
    return jobs.empty();
}

bool JobQueue::isRunning() const {
    return runningIndex >= 0;
}

const std::vector<Job>& JobQueue::entries() const {
    return jobs;
}

Job* JobQueue::nextPending() {
    for (auto& job : jobs) {
        if (job.status == JobStatus::Pending) {
            job.status   = JobStatus::Running;
            runningIndex = static_cast<int>(&job - jobs.data());
            return &job;
        }
    }
    return nullptr;
}

const Job* JobQueue::running() const {
    if (runningIndex < 0) return nullptr;
    return &jobs[runningIndex];
}

void JobQueue::setProgress(float progress) {
    if (runningIndex < 0) return;
    jobs[runningIndex].progress = progress;
}

void JobQueue::complete() {
    if (runningIndex < 0) return;
    jobs[runningIndex].status   = JobStatus::Done;
    jobs[runningIndex].progress = 1.0f;
    runningIndex = -1;
}

void JobQueue::fail() {
    if (runningIndex < 0) return;
    jobs[runningIndex].status = JobStatus::Failed;
    runningIndex = -1;
}
