#pragma once

#include <string>

enum class DockSlot { None, Left, Right, Bottom, Center };

class Panel {
public:
    virtual ~Panel() = default;
    virtual std::string getTitle() const = 0;
    virtual DockSlot getDockSlot() const { return DockSlot::None; }
    virtual void draw() = 0;
};
