#pragma once

#include "FontAwesome/IconsFontAwesome7.h"

#include "panel.hpp"

class ScenePanel : public Panel {
public:
    std::string getTitle() const override { return ICON_FA_CUBES " Scene"; }
    void draw() override;
};
