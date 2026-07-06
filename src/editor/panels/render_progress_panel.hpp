#pragma once

#include "panel.hpp"

#include "app/app_context.hpp"

class RenderProgressPanel: IPanel {
public:
    void draw(AppContext& ctx) override;
};
