#pragma once

#include "panel.hpp"

#include "utils/progress.hpp"

class RenderProgressPanel: public Panel {
private:
    void content() override;
    ProgressTimer timer;
};
