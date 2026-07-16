#pragma once

#include "panel.hpp"

class RendererPanel: public IPanel {
private:
    void content() override;
    bool promptImagePath();
    bool promptVideoPath();
};
