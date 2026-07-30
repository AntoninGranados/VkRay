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
        const ecs::Field* field;
    };
    struct MaterialTrack {
        MaterialHandle handle;
        std::string field;
    };
    struct SegmentPopupState {
        std::string label;
        Keyframe from;
        Keyframe to;
        std::variant<EntityTrack, MaterialTrack> track;
    };

    using MaterialField = std::pair<const char*, const char*>;

    static std::vector<float> extractComponents(const KeyframeValue& value);
    static void drawSegmentGraph(const std::string& label, const Keyframe& from, const Keyframe& to);
    static std::optional<std::pair<Keyframe, Keyframe>> drawRow(const RowContext& ctx, const char* label, const char* id, const std::map<int, Keyframe>& keyframes);
    static std::vector<MaterialField> materialAnimFields(MaterialType type);

    std::optional<SegmentPopupState> pendingSegment;

    void content() override;
};
