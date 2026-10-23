#pragma once

#include "FontAwesome/IconsFontAwesome7.h"

#include "panel.hpp"

class RendererPanel : public Panel {
public:
    std::string getTitle() const override { return ICON_FA_CAMERA " Renderer"; }
    DockSlot getDockSlot() const override { return DockSlot::Left; }
    void draw() override;
};
