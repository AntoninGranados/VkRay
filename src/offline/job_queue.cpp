#include "job_queue.hpp"

#include <fstream>
#include <random>
#include <stdexcept>
#include <unordered_map>

#include "nlohmann/json.hpp"

#include "utils/json_resolve.hpp"

using json = nlohmann::json;

static constexpr int JOB_VERSION = 1;

static std::unordered_map<std::string, std::filesystem::path> kAovPaths = {
    { "position_w", "renderer/aov/position_w" },
    { "position",   "renderer/aov/position"   },
    { "normal_w",   "renderer/aov/normal_w"   },
    { "normal",     "renderer/aov/normal"     },
    { "albedo",     "renderer/aov/albedo"     },
    { "roughness",  "renderer/aov/roughness"  },
    { "mat_type",   "renderer/aov/mat_type"   },
    { "sky_mask",   "renderer/aov/sky_mask"   },
};

static void parseAovs(const json& arr, std::vector<ParameterOverride>& overrides) {
    for (const auto& name : arr) {
        const std::string s = name.get<std::string>();
        auto it = kAovPaths.find(s);
        if (it != kAovPaths.end())
            overrides.push_back({ it->second, true });
    }
}

JobQueue JobQueue::fromFile(const std::filesystem::path& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error(std::format("Cannot open job queue file [{}]", path.string()));

    json root = json::parse(f, nullptr, true, true);

    const int version = root.value("version", -1);
    if (version != JOB_VERSION)
        throw std::runtime_error(std::format(
            "Job version mismatch in `{}`: expected {}, got {}", path.string(), std::to_string(JOB_VERSION), std::to_string(version)
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
                if      (val.is_boolean())        parameterOverrides.push_back({key, val.get<bool>()});
                else if (val.is_number_integer()) parameterOverrides.push_back({key, val.get<int32_t>()});
                else if (val.is_number_float())   parameterOverrides.push_back({key, val.get<float>()});
                else if (val.is_string())         parameterOverrides.push_back({key, val.get<std::string>()});
                else if (val.is_array() && val.size() >= 2) {
                    bool isInt = val[0].is_number_integer();
                    if      ( isInt && val.size() == 2) parameterOverrides.push_back({key, glm::ivec2(val[0].get<int>(), val[1].get<int>())});
                    else if ( isInt && val.size() == 3) parameterOverrides.push_back({key, glm::ivec3(val[0].get<int>(), val[1].get<int>(), val[2].get<int>())});
                    else if ( isInt && val.size() == 4) parameterOverrides.push_back({key, glm::ivec4(val[0].get<int>(), val[1].get<int>(), val[2].get<int>(), val[3].get<int>())});
                    else if (!isInt && val.size() == 2) parameterOverrides.push_back({key, glm::vec2(val[0].get<float>(), val[1].get<float>())});
                    else if (!isInt && val.size() == 3) parameterOverrides.push_back({key, glm::vec3(val[0].get<float>(), val[1].get<float>(), val[2].get<float>())});
                    else if (!isInt && val.size() == 4) parameterOverrides.push_back({key, glm::vec4(val[0].get<float>(), val[1].get<float>(), val[2].get<float>(), val[3].get<float>())});
                }
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
