#include "ui_utils.hpp"

#include <nfd.hpp>

#include "FontAwesome/IconsFontAwesome7.h"
#include "imgui/imgui.h"

#include "core/animation/animation_store.hpp"
#include "core/core.hpp"
#include "core/scene/scene.hpp"

namespace ui {

namespace {

void drawKeyframeIcon(bool has, bool hovered, bool active) {
    ImVec4 color;
    if (has) {
        if (active) color = luma(kKeyframeOnColor, 0.7f);
        else if (hovered) color = luma(kKeyframeOnColor, 1.3f);
        else color = kKeyframeOnColor;
    } else {
        color = ImGui::GetStyleColorVec4(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    }

    ImFont* font = ImGui::GetFont();
    const ImVec2 rectMin = ImGui::GetItemRectMin();
    const ImVec2 rectMax = ImGui::GetItemRectMax();
    constexpr float iconSize = 10.0f;
    const ImVec2 textSize = font->CalcTextSizeA(iconSize, FLT_MAX, 0.0f, ICON_FA_SQUARE);
    const ImVec2 pos = ImVec2(rectMin.x + (rectMax.x - rectMin.x - textSize.x) * 0.5f,
                              rectMin.y + (rectMax.y - rectMin.y - textSize.y) * 0.5f);
    ImGui::GetWindowDrawList()->AddText(font, iconSize, pos,
        ImGui::ColorConvertFloat4ToU32(color), ICON_FA_SQUARE);
}

} // namespace

void drawKeyframeButton(ecs::Entity e, ecs::Component& c, const std::string& fieldId) {
    AnimationStore& store = Core::getScene().getAnimationStore();
    const int frame = Core::getAnimation().getFrame();
    const bool has = store.has(e, c.getType(), fieldId, frame);

    constexpr float iconSize = 10.0f;
    ImGui::PushID((fieldId + "_keyframe").c_str());
    const bool clicked = ImGui::InvisibleButton("##keyframe", ImVec2(iconSize, ImGui::GetTextLineHeight()));
    const bool hovered = ImGui::IsItemHovered();
    const bool active = ImGui::IsItemActive();
    ImGui::PopID();

    if (clicked) {
        if (has) store.remove(e, c.getType(), fieldId, frame);
        else store.capture(e, c, fieldId, frame);
        Core::markRenderDirty();
    }

    drawKeyframeIcon(has, hovered, active);
    ImGui::SameLine();
}

bool beginCenteredModal(const char* name) {
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(mainViewport->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    return ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove);
}

void endCenteredModal() {
    PushCancelStyleColor();
    if (ImGui::Button(ICON_FA_BAN " Cancel", kButtonSize))
        ImGui::CloseCurrentPopup();
    PopCancelStyleColor();
    ImGui::EndPopup();
}

namespace {
std::vector<nfdfilteritem_t> toNfdFilters(const std::vector<FileFilter>& filters) {
    std::vector<nfdfilteritem_t> items;
    for (const auto& f : filters) items.push_back({ f.name.c_str(), f.extensions.c_str() });
    return items;
}
}

std::optional<std::filesystem::path> openFileDialog(const std::vector<FileFilter>& filters, const std::filesystem::path& defaultDir) {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    const std::vector<nfdfilteritem_t> items = toNfdFilters(filters);
    const std::string dir = defaultDir.string();
    if (NFD::OpenDialog(outPath, items.data(), static_cast<nfdfiltersize_t>(items.size()), dir.empty() ? nullptr : dir.c_str()) != NFD_OKAY)
        return std::nullopt;
    return std::filesystem::path(outPath.get());
}

std::optional<std::filesystem::path> saveFileDialog(const std::vector<FileFilter>& filters, const std::filesystem::path& defaultDir, const std::string& defaultName, const std::string& forceExtension) {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    const std::vector<nfdfilteritem_t> items = toNfdFilters(filters);
    const std::string dir = defaultDir.string();
    if (NFD::SaveDialog(outPath, items.data(), static_cast<nfdfiltersize_t>(items.size()), dir.empty() ? nullptr : dir.c_str(), defaultName.empty() ? nullptr : defaultName.c_str()) != NFD_OKAY)
        return std::nullopt;
    std::filesystem::path path(outPath.get());
    if (!forceExtension.empty() && path.extension() != "." + forceExtension)
        path += "." + forceExtension;
    return path;
}

std::optional<std::filesystem::path> pickFolderDialog(const std::filesystem::path& defaultDir) {
    NFD::Guard guard;
    NFD::UniquePath outPath;
    const std::string dir = defaultDir.string();
    if (NFD::PickFolder(outPath, dir.empty() ? nullptr : dir.c_str()) != NFD_OKAY)
        return std::nullopt;
    return std::filesystem::path(outPath.get());
}

}   // namespace ui
