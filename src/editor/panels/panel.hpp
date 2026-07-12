#pragma once

class IPanel {
public:
    virtual ~IPanel() = default;
    virtual void draw() = 0;
};
