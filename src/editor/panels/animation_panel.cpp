#include "animation_panel.hpp"

#include <algorithm>
#include <cstdio>
#include <type_traits>
#include <vector>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/animation/animation_handler.hpp"
#include "core/animation/animation_store.hpp"
#include "core/core.hpp"
#include "core/ecs/components.hpp"
#include "core/scene/scene.hpp"
#include "editor/editor.hpp"
#include "editor/ui_utils.hpp"

std::vector<float> AnimationPanel::extractComponents(const KeyframeValue& value) {
    return std::visit([](const auto& v) -> std::vector<float> {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, float>)       return { v };
        if constexpr (std::is_same_v<T, glm::vec2>)   return { v.x, v.y };
        if constexpr (std::is_same_v<T, glm::vec3>)   return { v.x, v.y, v.z };
        if constexpr (std::is_same_v<T, glm::vec4>)   return { v.x, v.y, v.z, v.w };
        if constexpr (std::is_same_v<T, int>)         return { float(v) };
        if constexpr (std::is_same_v<T, glm::ivec2>)  return { float(v.x), float(v.y) };
        if constexpr (std::is_same_v<T, glm::ivec3>)  return { float(v.x), float(v.y), float(v.z) };
        if constexpr (std::is_same_v<T, glm::ivec4>)  return { float(v.x), float(v.y), float(v.z), float(v.w) };
        if constexpr (std::is_same_v<T, glm::quat>)   return { v.x, v.y, v.z, v.w };
        if constexpr (std::is_same_v<T, bool>)        return { v ? 1.0f : 0.0f };
        return { 0.0f };
    }, value);
}

void AnimationPanel::drawSegmentGraph(const std::string& label, const Keyframe& from, const Keyframe& to) {
    constexpr int kSamples = 64;
    constexpr float kPlotWidth = 320.0f;
    constexpr float kPlotHeight = 60.0f;

    static const char* kComponentLabels[] = { "X", "Y", "Z", "W" };
    const ImVec4 kComponentColors[] = {
        ui::kDraculaRed,
        ui::kDraculaGreen,
        ui::kDraculaCyan,
        ui::kDraculaOrange,
    };

    std::vector<std::vector<float>> componentSamples;
    for (int i = 0; i < kSamples; i++) {
        const float t = float(i) / float(kSamples - 1);
        const std::vector<float> components = extractComponents(Keyframe::interpolate(from, to, t));
        if (componentSamples.empty())
            componentSamples.resize(components.size());
        for (size_t c = 0; c < components.size(); c++)
            componentSamples[c].push_back(components[c]);
    }

    if (componentSamples.empty()) return;

    float minValue = componentSamples[0][0];
    float maxValue = componentSamples[0][0];
    for (const auto& samples : componentSamples)
        for (float v : samples) { minValue = std::min(minValue, v); maxValue = std::max(maxValue, v); }

    if (maxValue - minValue < 1e-5f) { minValue -= 0.5f; maxValue += 0.5f; }
    const float pad = (maxValue - minValue) * 0.1f;
    minValue -= pad;
    maxValue += pad;

    ImGui::TextDisabled("%s", label.c_str());

    for (size_t c = 0; c < componentSamples.size(); c++) {
        const char* plotLabel = componentSamples.size() == 1 ? "##plot" : kComponentLabels[c % 4];
        ImGui::PushStyleColor(ImGuiCol_PlotLines, kComponentColors[c % 4]);
        ImGui::PlotLines(plotLabel, componentSamples[c].data(), kSamples, 0, nullptr, minValue, maxValue, ImVec2(kPlotWidth, kPlotHeight));
        ImGui::PopStyleColor();
    }
}

struct RowContext {
    ImDrawList* dl;
    float labelWidth;
    float rowHeight;
    float rowSpacing;
    float timelineWidth;
    int endFrame;
    int currentFrame;
    int fps;
    ImU32 barBg;
    ImU32 keyframeColor;
    ImU32 labelColor;
    ImU32 tickFrame;
    ImU32 tickSecond;
};

std::optional<std::pair<Keyframe, Keyframe>> AnimationPanel::drawRow(const RowContext& ctx, const char* label, const char* id, const std::map<int, Keyframe>& keyframes) {
    const ImVec2 rowStart = ImGui::GetCursorScreenPos();
    const float textY = rowStart.y + (ctx.rowHeight - ImGui::GetTextLineHeight()) * 0.5f;
    ctx.dl->AddText(ImVec2(rowStart.x, textY), ctx.labelColor, label);

    ImGui::SetCursorScreenPos(ImVec2(rowStart.x + ctx.labelWidth, rowStart.y));
    ImGui::PushID(id);
    ImGui::InvisibleButton("##tl", ImVec2(ctx.timelineWidth, ctx.rowHeight));
    if (ImGui::IsItemActive() && ImGui::IsMouseDown(0)) {
        const float t = std::clamp(
            (ImGui::GetIO().MousePos.x - ImGui::GetItemRectMin().x) / ctx.timelineWidth, 0.0f, 1.0f);
        AnimationHandler& anim = Core::getAnimation();
        anim.reset(std::min(static_cast<int>(std::round(t * float(ctx.endFrame - 1))), ctx.endFrame - 1));
        anim.pause();
        Core::requestAccumulationRestart();
    }
    const bool rightClicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    ImGui::PopID();

    const ImVec2 barMin = ImGui::GetItemRectMin();
    const ImVec2 barMax = ImGui::GetItemRectMax();
    ctx.dl->AddRectFilled(barMin, barMax, ctx.barBg, 2.0f);

    if (ctx.endFrame > 1) {
        const float span = float(ctx.endFrame - 1);
        for (int i = 1; i < ctx.endFrame; i++) {
            const float x = barMin.x + (float(i) / span) * ctx.timelineWidth;
            if (ctx.fps > 0 && i % ctx.fps == 0)
                ctx.dl->AddLine(ImVec2(x, barMin.y), ImVec2(x, barMax.y), ctx.tickSecond, 1.0f);
            else
                ctx.dl->AddLine(ImVec2(x, barMax.y - 3.0f), ImVec2(x, barMax.y), ctx.tickFrame, 1.0f);
        }
        for (const auto& [frame, keyframe] : keyframes) {
            const float x = barMin.x + (float(frame) / span) * ctx.timelineWidth;
            const float cy = barMin.y + ctx.rowHeight * 0.5f;
            ctx.dl->AddRectFilled(ImVec2(x - 4.0f, cy - 4.0f), ImVec2(x + 4.0f, cy + 4.0f), ctx.keyframeColor);
        }
        const float x = barMin.x + (float(ctx.currentFrame) / span) * ctx.timelineWidth;
        ctx.dl->AddLine(ImVec2(x, barMin.y), ImVec2(x, barMax.y), ctx.keyframeColor, 2.0f);
    }

    ImGui::SetCursorScreenPos(ImVec2(rowStart.x, rowStart.y + ctx.rowHeight + ctx.rowSpacing));

    if (ctx.endFrame > 1 && rightClicked && keyframes.size() >= 2) {
        const float span = float(ctx.endFrame - 1);
        const float mouseX = ImGui::GetIO().MousePos.x;
        for (auto it = keyframes.begin(); it != keyframes.end(); ++it) {
            const auto next = std::next(it);
            if (next == keyframes.end()) break;
            const float x0 = barMin.x + (float(it->first) / span) * ctx.timelineWidth;
            const float x1 = barMin.x + (float(next->first) / span) * ctx.timelineWidth;
            if (mouseX >= x0 && mouseX <= x1)
                return std::make_pair(it->second, next->second);
        }
    }
    return std::nullopt;
}

std::vector<AnimationPanel::MaterialField> AnimationPanel::materialAnimFields(MaterialType type) {
    switch (type) {
        case MaterialType::Principled:
            return {{"albedo","Albedo"},{"roughness","Roughness"},{"metalness","Metalness"},{"ior","IoR"},{"transmission","Transmission"}};
        case MaterialType::Emissive:
            return {{"albedo","Albedo"},{"emissionStrength","Emission Strength"}};
        case MaterialType::GgxMetal:
            return {{"albedo","Albedo"},{"roughness","Roughness"}};
        case MaterialType::GgxGlossy:
            return {{"albedo","Albedo"},{"roughness","Roughness"},{"ior","IoR"}};
        case MaterialType::Dielectric:
            return {{"albedo","Albedo"},{"roughness","Roughness"},{"ior","IoR"},{"density","Density"},{"transmission","Transmission"},{"anisotropic","Anisotropic"}};
        case MaterialType::Volume:
            return {{"albedo","Albedo"},{"density","Density"},{"anisotropic","Anisotropic"}};
        default:
            return {{"albedo","Albedo"}};
    }
}

void AnimationPanel::content() {
    Scene& scene = Core::getScene();
    AnimationHandler& animation = Core::getAnimation();

    ui::setNextWindowFixed();
    ImGui::Begin("Animation");

    // Controls
    {
        bool paused = animation.isPaused();
        if (ImGui::Button(paused ? ICON_FA_PLAY : ICON_FA_PAUSE, { 24, 0 }))
            animation.toggle();
        ImGui::SameLine();

        if (scene.isPhysicsBakeInProgress()) {
            const int total = std::max(1, scene.getPhysicsBakeTotalFrames());
            const int current = std::clamp(scene.getPhysicsBakeCurrentFrame(), 0, total);
            const float progress = float(current) / float(total);
            char overlay[16];
            std::snprintf(overlay, sizeof(overlay), "%.0f%%", progress * 100.0f);
            ImGui::ProgressBar(progress, ImVec2(120.0f, 0.0f), "");
            const ImVec2 textSize = ImGui::CalcTextSize(overlay);
            const ImVec2 barMin = ImGui::GetItemRectMin();
            const ImVec2 barMax = ImGui::GetItemRectMax();
            ImGui::GetWindowDrawList()->AddText(
                ImVec2((barMin.x + barMax.x - textSize.x) * 0.5f, (barMin.y + barMax.y - textSize.y) * 0.5f),
                ImGui::GetColorU32(ImGuiCol_Text), overlay);
        } else if (ImGui::Button(ICON_FA_HARD_DRIVE " Bake Physics", { 120, 0 })) {
            scene.bakePhysics();
        }
        ImGui::SameLine();

        // TODO: put this in the constants
        ImGui::PushItemWidth(50);
        int currentFrame = animation.getFrame();
        if (ImGui::DragInt("##CurrentFrame", &currentFrame, 1, 0, animation.getEndFrame() - 1)) {
            animation.reset(currentFrame);
            animation.pause();
            Core::requestAccumulationRestart();
        }
        ImGui::SameLine();
        ImGui::TextDisabled("/");
        ImGui::SameLine();
        int endFrame = animation.getEndFrame();
        if (ImGui::DragInt("##EndFrame", &endFrame, 1, 1))
            animation.setEndFrame(endFrame);
        ImGui::PopItemWidth();
    }

    ImGui::Separator();

    ImGui::BeginChild("##timeline", ImVec2(0, 0));

    // Timeline
    {
        const SceneSelection& sel = Editor::getUi().getSelection();
        const bool hasEntity = sel.entity >= 0 && sel.entity < static_cast<int>(scene.getEntities().size());
        const bool hasMaterial = sel.material >= 0 && sel.material < static_cast<int>(scene.getMaterials().size());

        if (!hasEntity && !hasMaterial) {
            ImGui::TextDisabled("No selection");
        } else {
            AnimationStore& store = scene.getAnimationStore();

            constexpr float labelWidth = 150.0f;
            constexpr float rowHeight = 18.0f;
            constexpr float rowSpacing = 3.0f;
            const float timelineWidth = ImGui::GetContentRegionAvail().x - labelWidth;
            const int endFrame = animation.getEndFrame();
            const int currentFrame = animation.getFrame();
            ImDrawList* dl = ImGui::GetWindowDrawList();

            const int fps = animation.getFps();
            const ImU32 barBg = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
            const ImU32 keyframeColor = ImGui::ColorConvertFloat4ToU32(ui::kKeyframeOnColor);
            const ImVec4 textCol = ImGui::GetStyleColorVec4(ImGuiCol_Text);
            const ImU32 tickFrame = ImGui::ColorConvertFloat4ToU32(ImVec4(textCol.x, textCol.y, textCol.z, 0.12f));
            const ImU32 tickSecond = ImGui::ColorConvertFloat4ToU32(ImVec4(textCol.x, textCol.y, textCol.z, 0.35f));
            const ImU32 labelColor = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyleColorVec4(ImGuiCol_Text));

            const RowContext ctx {
                .dl = dl,
                .labelWidth = labelWidth,
                .rowHeight = rowHeight,
                .rowSpacing = rowSpacing,
                .timelineWidth = timelineWidth,
                .endFrame = endFrame,
                .currentFrame = currentFrame,
                .fps = fps,
                .barBg = barBg,
                .keyframeColor = keyframeColor,
                .labelColor = labelColor,
                .tickFrame = tickFrame,
                .tickSecond = tickSecond,
            };

            if (hasEntity) {
                ecs::Registry& registry = scene.getRegistry();
                const ecs::Entity entity = scene.getEntities()[static_cast<size_t>(sel.entity)];
                bool firstGroup = true;

                for (const ecs::ComponentType& ct : ecs::ComponentType::all()) {
                    if (!registry.has(entity, ct)) continue;

                    bool hasAnim = false;
                    for (const ecs::Field& f : ct.getFields())
                        if (f.metadata.animatable) { hasAnim = true; break; }
                    if (!hasAnim) continue;

                    if (!firstGroup) ImGui::Dummy(ImVec2(0, 4.0f));
                    firstGroup = false;

                    ImGui::TextDisabled("%s", ct.getLabel().c_str());

                    for (const ecs::Field& f : ct.getFields()) {
                        if (!f.metadata.animatable) continue;
                        const Track& track = store.getTrack(entity, ct, f);
                        if (auto seg = drawRow(ctx, f.label.c_str(), f.id.c_str(), track.getKeyframes())) {
                            pendingSegment = { f.label, seg->first, seg->second, EntityTrack{ entity, &ct, &f } };
                            ImGui::OpenPopup("##segment_interp");
                        }
                    }
                }

                if (registry.has(entity, ecs::MaterialRef)) {
                    const int handle = registry.get(entity, ecs::MaterialRef).get<int>("handle");
                    if (handle >= 0 && handle < static_cast<int>(scene.getMaterials().size())) {
                        if (!firstGroup) ImGui::Dummy(ImVec2(0, 4.0f));
                        ImGui::TextDisabled("Material");
                        for (auto& [fieldId, fieldLabel] : materialAnimFields(scene.getMaterials()[handle].type)) {
                            const Track& fieldTrack = store.getTrack(handle, fieldId);
                            if (auto seg = drawRow(ctx, fieldLabel, fieldId, fieldTrack.getKeyframes())) {
                                pendingSegment = { fieldLabel, seg->first, seg->second, MaterialTrack{ handle, fieldId } };
                                ImGui::OpenPopup("##segment_interp");
                            }
                        }
                    }
                }
            }

            if (!hasEntity && hasMaterial) {
                ImGui::TextDisabled("Material");
                for (auto& [fieldId, fieldLabel] : materialAnimFields(scene.getMaterials()[sel.material].type)) {
                    const Track& fieldTrack = store.getTrack(sel.material, fieldId);
                    if (auto seg = drawRow(ctx, fieldLabel, fieldId, fieldTrack.getKeyframes())) {
                        pendingSegment = { fieldLabel, seg->first, seg->second, MaterialTrack{ sel.material, fieldId } };
                        ImGui::OpenPopup("##segment_interp");
                    }
                }
            }

            static const char* kInterpolationNames[] = { "Step", "Linear", "Cubic", "Ease In", "Ease Out", "Ease In-Out" };
            ImGui::SetNextWindowSize(ImVec2(380.0f, 0.0f), ImGuiCond_Always);
            if (pendingSegment && ImGui::BeginPopup("##segment_interp")) {
                int current = static_cast<int>(pendingSegment->from.interpolation);
                if (ImGui::Combo("##interp_mode", &current, kInterpolationNames, IM_ARRAYSIZE(kInterpolationNames))) {
                    const Interpolation interpolation = static_cast<Interpolation>(current);
                    pendingSegment->from.interpolation = interpolation;
                    std::visit([&](const auto& t) {
                        using T = std::decay_t<decltype(t)>;
                        if constexpr (std::is_same_v<T, EntityTrack>)
                            store.getTrack(t.entity, *t.type, *t.field).setInterpolation(pendingSegment->from.frame, interpolation);
                        else
                            store.getTrack(t.handle, t.field).setInterpolation(pendingSegment->from.frame, interpolation);
                    }, pendingSegment->track);
                }
                drawSegmentGraph(pendingSegment->label, pendingSegment->from, pendingSegment->to);
                ImGui::EndPopup();
            }
        }
    }

    ImGui::EndChild();

    ImGui::End();
}
