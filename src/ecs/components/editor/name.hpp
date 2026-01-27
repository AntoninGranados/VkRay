#pragma once

#include <string>
#include <utility>

namespace ecs {

struct Name {
    std::string value;

    void setValue(std::string newValue) {
        value = std::move(newValue);
    }
};

} // namespace ecs
