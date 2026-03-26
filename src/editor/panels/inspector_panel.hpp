#pragma once

#include "panel.hpp"

#include "app/app_context.hpp"

class InspectorPanel: IPanel {
public:
    void draw(AppContext& ctx) override;
};
