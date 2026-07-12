#pragma once

#include "app_context.hpp"

class IPanel {
public:
    virtual ~IPanel() = default;
    virtual void draw(AppContext& ctx) = 0;
};
