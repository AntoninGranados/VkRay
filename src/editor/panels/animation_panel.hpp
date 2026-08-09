#pragma once

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "core/animation/keyframe.hpp"
#include "core/ecs/components/component_type.hpp"
#include "core/ecs/entity.hpp"
#include "core/scene/object/material.hpp"
#include "panel.hpp"

struct RowContext;

class AnimationPanel : public IPanel {
    struct EntityTrack {
        ecs::Entity entity;
        const ecs::ComponentType* type;
        std::string fieldId;
    };
    struct MaterialTrack {
        MaterialHandle handle;
        std::string fieldId;
    };
    struct SegmentPopupState {
        std::string label;
        Keyframe from;
        Keyframe to;
        std::variant<EntityTrack, MaterialTrack> track;
    };

    template<typename T> static std::vector<float> decompose(T v);
    static std::vector<float> decomposeInterpolation(const Keyframe& from, const Keyframe& to, float t);
    static void drawSegmentGraph(const std::string& label, const Keyframe& from, const Keyframe& to);
    static std::optional<std::pair<Keyframe, Keyframe>> drawRow(const RowContext& ctx, const char* label, const char* id, const std::map<int, Keyframe>& keyframes);

    std::optional<SegmentPopupState> pendingSegment;

    void content() override;
};
