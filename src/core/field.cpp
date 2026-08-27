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
template<> FieldType FieldValue::typeOf<ecs::Entity>() { return FieldType::Entity; }
template<> FieldType FieldValue::typeOf<std::string>() { return FieldType::String; }

Field Field::makeNumeric(FieldType type, std::string id, std::string label, const std::vector<float>& n, NumericMeta metadata) {
    switch (type) {
        case FieldType::Float: return Field::make<float>(std::move(id), std::move(label), n[0], metadata);
        case FieldType::Int:   return Field::make<int>(std::move(id), std::move(label), int(n[0]), metadata);
        case FieldType::Vec2:  return Field::make<glm::vec2>(std::move(id), std::move(label), n.size() == 1 ? glm::vec2(n[0]) : glm::vec2(n[0], n[1]), metadata);
        case FieldType::Vec3:  return Field::make<glm::vec3>(std::move(id), std::move(label), n.size() == 1 ? glm::vec3(n[0]) : glm::vec3(n[0], n[1], n[2]), metadata);
        case FieldType::Vec4:  return Field::make<glm::vec4>(std::move(id), std::move(label), n.size() == 1 ? glm::vec4(n[0]) : glm::vec4(n[0], n[1], n[2], n[3]), metadata);
        case FieldType::IVec2: return Field::make<glm::ivec2>(std::move(id), std::move(label), n.size() == 1 ? glm::ivec2(int(n[0])) : glm::ivec2(int(n[0]), int(n[1])), metadata);
        case FieldType::IVec3: return Field::make<glm::ivec3>(std::move(id), std::move(label), n.size() == 1 ? glm::ivec3(int(n[0])) : glm::ivec3(int(n[0]), int(n[1]), int(n[2])), metadata);
        default:               return Field::make<glm::ivec4>(std::move(id), std::move(label), n.size() == 1 ? glm::ivec4(int(n[0])) : glm::ivec4(int(n[0]), int(n[1]), int(n[2]), int(n[3])), metadata);
    }
}
