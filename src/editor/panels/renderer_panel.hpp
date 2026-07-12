#pragma once

#include "panel.hpp"

class RendererPanel: IPanel {
public:
    void draw() override;

private:
    bool promptImagePath();
    bool promptVideoPath();
};
