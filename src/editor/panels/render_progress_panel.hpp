#pragma once

#include "panel.hpp"

#include "utils/progress.hpp"

class RenderProgressPanel: public IPanel {
private:
    void content() override;
    ProgressTimer timer;
};
