#pragma once

#include "panel.hpp"

#include "app_context.hpp"

class RendererPanel: IPanel {
public:
    void draw(AppContext& ctx) override;

private:
    bool promptImagePath(AppContext& ctx);
    bool promptVideoPath(AppContext& ctx);
};
