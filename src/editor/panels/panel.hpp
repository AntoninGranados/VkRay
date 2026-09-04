#pragma once

#include <string>

class Panel {
public:
    virtual ~Panel() = default;
    virtual std::string getTitle() const = 0;
    virtual void draw() = 0;
};
