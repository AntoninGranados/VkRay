#pragma once

#include "panel.hpp"

class DebugPanel : public Panel {
public:
    std::string getTitle() const override { return "Debug View"; }
    void draw() override;
};
