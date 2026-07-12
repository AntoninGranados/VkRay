#pragma once

#include "panel.hpp"

#include "utils/progress.hpp"

class RenderProgressPanel: IPanel {
public:
    void draw() override;

private:
    ProgressTimer timer;
};
