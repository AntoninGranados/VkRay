#pragma once

#include "panel.hpp"

#include "app_context.hpp"

class CameraPanel: IPanel {
public:
    void draw(AppContext& ctx) override;
};
