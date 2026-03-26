#pragma once

#include "panel.hpp"

#include "app/app_context.hpp"

class StatsPanel: IPanel {
public:
    void draw(AppContext& ctx) override;
};
