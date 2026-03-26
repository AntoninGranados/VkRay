#pragma once

#include "panel.hpp"

#include "app/app_context.hpp"

class RenderPanel: IPanel {
public:
    void draw(AppContext& ctx) override;
};
