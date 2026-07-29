#include "fields.hpp"

#include <glm/glm.hpp>

namespace ecs {

template<> FieldType getFieldType<bool>() { return FieldType::Bool; };
template<> FieldType getFieldType<int>() { return FieldType::Int; };
template<> FieldType getFieldType<glm::ivec2>() { return FieldType::Int2; };
template<> FieldType getFieldType<glm::ivec3>() { return FieldType::Int3; };
template<> FieldType getFieldType<glm::ivec4>() { return FieldType::Int4; };
template<> FieldType getFieldType<float>() { return FieldType::Float; };
template<> FieldType getFieldType<glm::vec2>() { return FieldType::Float2; };
template<> FieldType getFieldType<glm::vec3>() { return FieldType::Float3; };
template<> FieldType getFieldType<glm::vec4>() { return FieldType::Float4; };
template<> FieldType getFieldType<glm::quat>() { return FieldType::Quat; };
template<> FieldType getFieldType<std::string>() { return FieldType::String; };

} // namespace ecs