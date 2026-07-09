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
    { "position_w", "pathtracer/aov/position_w" },
    { "position",   "pathtracer/aov/position"   },
    { "normal_w",   "pathtracer/aov/normal_w"   },
    { "normal",     "pathtracer/aov/normal"     },
    { "albedo",     "pathtracer/aov/albedo"     },
    { "roughness",  "pathtracer/aov/roughness"  },
    { "mat_type",   "pathtracer/aov/mat_type"   },
    { "sky_mask",   "pathtracer/aov/sky_mask"   },
};

static void parseAovs(const json& arr, std::vector<ParamOverride>& overrides) {
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
        throw std::runtime_error("Cannot open job queue file: " + path.string());

    json root = json::parse(f, nullptr, true, true);

    const int version = root.value("version", -1);
    if (version != JOB_VERSION)
        throw std::runtime_error("Job version mismatch in '" + path.string() + "': expected " + std::to_string(JOB_VERSION) + ", got " + std::to_string(version));

    JobQueue queue;
    std::mt19937 rng(0);

    for (const auto& j : root.at("jobs")) {
        const uint32_t repeatCount = j.contains("repeat") ? j.at("repeat").value("count", 1u) : 1u;

        struct Checkpoint {
            uint32_t                   spp;
            std::vector<ParamOverride> aovOverrides;
        };

        std::vector<ParamOverride> paramOverrides;
        if (j.contains("parameters")) {
            for (const auto& [key, val] : j.at("parameters").items()) {
                if      (val.is_boolean())        paramOverrides.push_back({key, val.get<bool>()});
                else if (val.is_number_integer()) paramOverrides.push_back({key, val.get<int32_t>()});
                else if (val.is_number_float())   paramOverrides.push_back({key, val.get<float>()});
                else if (val.is_string())         paramOverrides.push_back({key, val.get<std::string>()});
            }
        }

        std::vector<Checkpoint> checkpoints;
        const auto& samplesJson = j.at("samples");
        for (const auto& entry : samplesJson.is_array() ? samplesJson : json::array({samplesJson})) {
            if (entry.is_number()) {
                checkpoints.push_back({ entry.get<uint32_t>(), {} });
            } else {
                std::vector<ParamOverride> cpAovs;
                if (entry.contains("aovs")) parseAovs(entry.at("aovs"), cpAovs);
                checkpoints.push_back({ entry.at("spp").get<uint32_t>(), std::move(cpAovs) });
            }
        }

        std::vector<ParamOverride> jobAovOverrides;
        if (j.contains("aovs")) parseAovs(j.at("aovs"), jobAovOverrides);

        for (uint32_t n = 0; n < repeatCount; ++n) {
            ResolveCtx nCtx{ rng, {{"n", {static_cast<int>(n), static_cast<int>(repeatCount)}}} };
            const std::string scenePath = resolveTemplate(j.at("scene").get<std::string>(), nCtx);
            const uint32_t    nSeed     = rng();

            for (size_t ci = 0; ci < checkpoints.size(); ++ci) {
                ResolveCtx ctx = nCtx;
                ctx.tokens["spp"] = {static_cast<int>(ci), static_cast<int>(checkpoints.size())};

                const std::vector<ParamOverride>& aovOverrides =
                    checkpoints[ci].aovOverrides.empty() ? jobAovOverrides : checkpoints[ci].aovOverrides;

                Job job;
                job.scene              = scenePath;
                job.output             = resolveTemplate(j.at("output").get<std::string>(), ctx);
                job.samples            = checkpoints[ci].spp;
                job.seed               = nSeed;
                job.width              = j.at("width").get<uint32_t>();
                job.height             = j.at("height").get<uint32_t>();
                job.parameterOverrides = paramOverrides;
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
