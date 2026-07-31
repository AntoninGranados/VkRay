#include "field.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

template<> FieldType FieldValue::typeOf<bool>()        { return FieldType::Bool; }
template<> FieldType FieldValue::typeOf<int>()         { return FieldType::Int; }
template<> FieldType FieldValue::typeOf<glm::ivec2>()  { return FieldType::IVec2; }
template<> FieldType FieldValue::typeOf<glm::ivec3>()  { return FieldType::IVec3; }
template<> FieldType FieldValue::typeOf<glm::ivec4>()  { return FieldType::IVec4; }
template<> FieldType FieldValue::typeOf<float>()       { return FieldType::Float; }
template<> FieldType FieldValue::typeOf<glm::vec2>()   { return FieldType::Vec2; }
template<> FieldType FieldValue::typeOf<glm::vec3>()   { return FieldType::Vec3; }
template<> FieldType FieldValue::typeOf<glm::vec4>()   { return FieldType::Vec4; }
template<> FieldType FieldValue::typeOf<glm::quat>()   { return FieldType::Quat; }
template<> FieldType FieldValue::typeOf<std::string>() { return FieldType::String; }
