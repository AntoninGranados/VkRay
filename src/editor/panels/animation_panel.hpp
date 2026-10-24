#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "core/animation/keyframe.hpp"
#include "panel.hpp"

struct RowContext;

class AnimationPanel : public Panel {
public:
    std::string getTitle() const override { return "Animation"; }
    DockSlot getDockSlot() const override { return DockSlot::Bottom; }
    void draw() override;

private:
    struct SegmentPopupState {
        std::string label;
        Field* field;
        int fromFrame;
    };

    template <typename T> static std::vector<float> decompose(T v);
    static std::vector<float> decomposeInterpolation(const Keyframe& from, const Keyframe& to, float t);
    static void drawSegmentGraph(const std::string& label, const Keyframe& from, const Keyframe& to);
    static std::optional<int> drawRow(const RowContext& ctx, const char* label, const char* id,
                                      const std::map<int, Keyframe>& keyframes);

    std::optional<SegmentPopupState> pendingSegment;
};
