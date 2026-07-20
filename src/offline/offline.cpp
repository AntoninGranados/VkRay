#include "offline.hpp"

#include <format>

#include "core/core.hpp"
#include "core/scene/scene_serializer.hpp"

#include "utils/log.hpp"
#include "utils/progress.hpp"

void Offline::run(JobQueue& queue) {
    const int totalJobs = static_cast<int>(queue.entries().size());

    int jobIndex = 0;
    while (Job* job = queue.nextPending()) {
        jobIndex++;
        initParameters(job->parameterOverrides);

        Core::resize(
            Core::getParameters().getInt("renderer/output/width"),
            Core::getParameters().getInt("renderer/output/height")
        );

        LightMode lightMode = LightMode::Day;
        if (!SceneSerializer::load(Core::getScene(), lightMode, job->scene.string(), job->seed)) {
            Log::error("Offline", std::format("Failed to load the scene `{}` for the job {}", job->scene.string(), jobIndex));
            queue.fail();
            continue;
        }
        Log::success("Offline", std::format("[{}/{}] Loaded: `{}`", jobIndex, totalJobs, job->scene.string()));
        Core::setRenderMode(RenderMode::RenderSingle);
        Core::getParameters().setEnum<LightMode>("scene/light_mode", lightMode);

        Core::getEngine().waitIdle();
        Core::restartAccumulation();

        const uint32_t totalSamples = Core::getParameters().getInt("renderer/sampling/render_samples");
        ProgressBar bar(
            "[" + std::to_string(jobIndex) + "/" + std::to_string(totalJobs) + "]",
            totalSamples,
            "spp"
        );
        for (uint32_t i = 0; i < totalSamples; i++) {
            Core::renderFrame();
            queue.setProgress(static_cast<float>(i + 1) / static_cast<float>(totalSamples));
            bar.step();
        }
        bar.close();

        Core::getCoreRenderer().saveCapture(Core::getParameters().getPath("renderer/output/output_image"));

        queue.complete();
    }
}

void Offline::initParameters(const std::vector<ParameterOverride>& overrides) {
    Core::getParameters().resetAll();
    for (const auto& parameterOverride : overrides) {
        std::visit([&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr      (std::is_same_v<T, bool>)        Core::getParameters().setBool(parameterOverride.key, v);
            else if constexpr (std::is_same_v<T, int>)         Core::getParameters().setInt(parameterOverride.key, v);
            else if constexpr (std::is_same_v<T, float>)       Core::getParameters().setFloat(parameterOverride.key, v);
            else if constexpr (std::is_same_v<T, std::string>) Core::getParameters().setEnumByName(parameterOverride.key, v);
            else if constexpr (std::is_same_v<T, std::filesystem::path>) Core::getParameters().setPath(parameterOverride.key, v);
        }, parameterOverride.value);
    }
}