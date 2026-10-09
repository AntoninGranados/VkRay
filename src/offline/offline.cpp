#include "offline.hpp"

#include <format>

#include "core/core.hpp"
#include "core/scene/scene_serializer.hpp"

#include "utils/log.hpp"
#include "utils/progress.hpp"

void Offline::run() {
    JobQueue& queue = Core::getJobQueue();
    const int totalJobs = static_cast<int>(queue.entries().size());

    int jobIndex = 0;
    while (Job* job = queue.nextPending()) {
        jobIndex++;
        initParameters(job->parameterOverrides);
        Core::consumeResize();

        if (!SceneSerializer::load(Core::getScene(), job->scene.string(), job->seed)) {
            Log::error("Offline",
                       std::format("Failed to load the scene `{}` for the job {}", job->scene.string(), jobIndex));
            queue.fail();
            continue;
        }
        Log::success("Offline", std::format("[{}/{}] Loaded: `{}`", jobIndex, totalJobs, job->scene.string()));
        Core::setRenderMode(RenderMode::RenderSingle);

        Core::getEngine().waitIdle();

        const uint32_t totalSamples = Core::getParameters().get<int>("renderer/sampling/render_samples");
        Core::setTargetSampleCount(static_cast<int>(totalSamples));
        Core::restartAccumulation();

        ProgressBar bar(std::format("[{}/{}]", jobIndex, totalJobs), totalSamples, "spp");
        while (!Core::isRenderFinished()) {
            Core::renderFrame();
            const uint32_t sampleCount = Core::getSampleCount();
            queue.setProgress(static_cast<float>(sampleCount) / static_cast<float>(totalSamples));
            bar.update(sampleCount);
        }
        bar.close();

        const std::filesystem::path outputPath =
            !Core::getOutputPath().empty()
                ? Core::getOutputPath()
                : Core::getParameters().get<std::filesystem::path>("renderer/output/output_image");
        Core::saveCapture(outputPath);

        queue.complete();
    }
}

void Offline::initParameters(const std::vector<ParameterOverride>& overrides) {
    Core::getParameters().resetAll();
    for (const auto& parameterOverride : overrides) {
        std::visit(
            [&](auto&& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>)
                    Core::getParameters().setEnumByName(parameterOverride.key, v);
                else
                    Core::getParameters().set(parameterOverride.key, v);
            },
            parameterOverride.value);
    }
}