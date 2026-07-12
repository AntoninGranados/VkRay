#pragma once

#include "panel.hpp"

#include "app_context.hpp"

class AnimationPanel: IPanel {
public:
    void draw(AppContext& ctx) override;
};
