#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/field.hpp"

namespace ecs {

struct ComponentField : Field {
    ComponentField() = default;
    ComponentField(bool isPrivate, bool isAnimatable)
        : fieldPrivate(isPrivate), animatable(isAnimatable) {}

    bool isPrivate() const { return fieldPrivate; }
    bool isAnimatable() const { return animatable; }

private:
    bool fieldPrivate = false;
    bool animatable = false;
};

class ComponentType {
public:
    class Builder;
    static ComponentType::Builder builder(std::string id);

    const std::string& getId() const { return id; }
    const std::string& getLabel() const { return label; }
    const std::string& getIcon() const { return icon; }
    const std::string& getGroup() const { return group; }
    const std::vector<std::string>& getNeeds() const { return needs; }
    const std::vector<std::string>& getConflicts() const { return conflicts; }

    const std::vector<ComponentField>& getFields() const { return fields; }
    const ComponentField& getField(const std::string& id) const { return fields[fieldIndex.at(id)]; }

    static const std::vector<ComponentType>& all();

private:
    friend class ComponentType::Builder;

    static std::vector<ComponentType> storage;

    std::string id;
    std::string label;
    std::string icon;
    std::string group;

    std::vector<std::string> needs;
    std::vector<std::string> conflicts;
    std::vector<ComponentField> fields;
    std::unordered_map<std::string, size_t> fieldIndex;
};

class ComponentType::Builder {
public:
    explicit Builder(std::string id);

    Builder& icon(std::string icon);
    Builder& group(std::string group);

    template <typename T>
    Builder& field(std::string id, T defaultValue = T{}, FieldMetadata metadata = {}, bool animatable = false) {
        return addField<T>(std::move(id), std::move(defaultValue), std::move(metadata), false, animatable);
    }

    template <typename T>
    Builder& privateField(std::string id, T defaultValue = T{}, FieldMetadata metadata = {}, bool animatable = false) {
        return addField<T>(std::move(id), std::move(defaultValue), std::move(metadata), true, animatable);
    }

    template <typename ...Args>
    Builder& needs(Args... ids) {
        (type.needs.push_back(std::string(ids)), ...);
        return *this;
    }

    template <typename ...Args>
    Builder& conflicts(Args... ids) {
        (type.conflicts.push_back(std::string(ids)), ...);
        return *this;
    }

    ComponentType& build();

private:
    std::string deriveLabel(const std::string& id);

    template <typename T>
    Builder& addField(std::string id, T defaultValue, FieldMetadata metadata, bool isPrivate, bool animatable = false) {
        if (type.fieldIndex.contains(id))
            throw std::invalid_argument("duplicate field id: " + id);
        ComponentField f(isPrivate, animatable);
        static_cast<Field&>(f) = Field::make<T>(id, deriveLabel(id), defaultValue, std::move(metadata));
        type.fieldIndex[f.getId()] = type.fields.size();
        type.fields.push_back(std::move(f));
        return *this;
    }

    ComponentType type = {};
};

} // namespace ecs
