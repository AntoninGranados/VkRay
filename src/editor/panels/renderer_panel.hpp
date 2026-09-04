#pragma once

#include "FontAwesome/IconsFontAwesome7.h"

#include "panel.hpp"

class RendererPanel: public Panel {
public:
    std::string getTitle() const override { return ICON_FA_CAMERA " Renderer"; }
    void draw() override;
};
