#pragma once

#include "FontAwesome/IconsFontAwesome7.h"

#include "panel.hpp"

#include "utils/progress.hpp"

class RenderProgressPanel: public Panel {
public:
    std::string getTitle() const override { return ICON_FA_STOPWATCH " Loading"; }
    void draw() override;

private:
    ProgressTimer timer;
};
