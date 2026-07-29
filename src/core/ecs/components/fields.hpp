#pragma once

#include <cstddef>
#include <limits>
#include <string>
#include <vector>

namespace ecs {

constexpr size_t maxStringFieldSize = 128;

enum FieldType {
    Bool,
    Int, Int2, Int3, Int4,
    Float, Float2, Float3, Float4,
    Quat,
    String
};

struct FieldMetadata {
    float min = -std::numeric_limits<float>::infinity();
    float max =  std::numeric_limits<float>::infinity();
    float step = 1e-5f;
    bool animatable = false;
};

struct Field {
    std::string id;
    std::string label;
    FieldType type;
    FieldMetadata metadata;
    size_t size = 0;
    size_t offset = 0;
    bool isPrivate = false;
    std::vector<std::byte> defaultValue;
};

template<typename T> FieldType getFieldType();

} // namespace ecs
