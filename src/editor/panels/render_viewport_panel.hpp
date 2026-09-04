#pragma once

#include "panel.hpp"

class RenderViewportPanel : public Panel {
public:
    std::string getTitle() const override { return "RenderView"; }
    void draw() override;
};
