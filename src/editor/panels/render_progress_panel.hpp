#pragma once

#include "panel.hpp"

#include "app_context.hpp"
#include "utils/progress.hpp"

class RenderProgressPanel: IPanel {
public:
    void draw(AppContext& ctx) override;

private:
    ProgressTimer timer;
};
