#pragma once

#include <concepts>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "core/field.hpp"
#include "utils/string_utils.hpp"

namespace ecs {

class Component;

struct ComponentField : Field {
    ComponentField() = default;
    explicit ComponentField(bool isAnimatable) : animatable(isAnimatable) {}

    bool isAnimatable() const { return animatable; }
    void setAnimatable(bool newAnimatable) { animatable = newAnimatable; }

private:
    bool animatable = false;
};

struct ComponentPayload {
    std::string id;
    std::type_index typeId;
    std::function<void*()> construct;
    std::function<void(void*)> destroy;
    std::function<Component*(void*)> asComponent = nullptr;

    bool operator==(const ComponentPayload& other) const {
        return id == other.id && typeId == other.typeId;
    };
};

class ComponentType {
public:
    class Builder;
    static ComponentType::Builder builder(std::string id);

    const std::string& getId() const { return id; }
    const std::string& getLabel() const { return label; }
    const std::string& getDescription() const { return description; }
    const std::string& getIcon() const { return icon; }
    const std::string& getGroup() const { return group; }
    const std::vector<std::string>& getNeeds() const { return needs; }
    const std::vector<std::string>& getConflicts() const { return conflicts; }

    const std::vector<ComponentField>& getFields() const { return fields; }
    const ComponentField& getField(const std::string& id) const { return fields[fieldIndex.at(id)]; }

    const std::vector<ComponentPayload>& getPayloads() const { return payloads; }
    size_t getPayloadIndex(const std::string& id) const { return payloadIndex.at(id); }

    static const std::vector<ComponentType>& all();
    static std::optional<std::reference_wrapper<const ComponentType>> find(const std::string& id);

    bool operator==(const ComponentType&) const = default;

private:
    friend class ComponentType::Builder;

    static std::vector<ComponentType> storage;

    std::string id;
    std::string label;
    std::string description;
    std::string icon;
    std::string group;

    std::vector<std::string> needs;
    std::vector<std::string> conflicts;
    std::vector<ComponentField> fields;
    std::unordered_map<std::string, size_t> fieldIndex;
    std::vector<ComponentPayload> payloads;
    std::unordered_map<std::string, size_t> payloadIndex;
};

class ComponentType::Builder {
public:
    explicit Builder(std::string id);

    Builder& description(std::string description);
    Builder& icon(std::string icon);
    Builder& group(std::string group);

    template <typename T>
    Builder& field(std::string id, T defaultValue = T{}, FieldMetadata metadata = {}, bool animatable = false) {
        if (type.fieldIndex.contains(id))
            throw std::invalid_argument("duplicate field id: " + id);
        ComponentField f(animatable);
        static_cast<Field&>(f) = Field::make<T>(id, snakeCaseToLabel(id), defaultValue, std::move(metadata));
        type.fieldIndex[f.getId()] = type.fields.size();
        type.fields.push_back(std::move(f));
        return *this;
    }

    Builder& field(ComponentField f) {
        if (type.fieldIndex.contains(f.getId()))
            throw std::invalid_argument("duplicate field id: " + f.getId());
        type.fieldIndex[f.getId()] = type.fields.size();
        type.fields.push_back(std::move(f));
        return *this;
    }

    template <typename T>
    Builder& payload(std::string id) {
        if (type.payloadIndex.contains(id))
            throw std::invalid_argument("duplicate payload id: " + id);
        type.payloadIndex[id] = type.payloads.size();
        ComponentPayload p{
            std::move(id),
            std::type_index(typeid(T)),
            []() -> void* { return new T(); },
            [](void* p) { delete static_cast<T*>(p); },
        };
        if constexpr (requires (T& t) { { t.getComponent() } -> std::same_as<Component&>; })
            p.asComponent = [](void* p) -> Component* { return &static_cast<T*>(p)->getComponent(); };
        type.payloads.push_back(std::move(p));
        return *this;
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
    ComponentType buildDetached();

private:
    ComponentType type = {};
};

} // namespace ecs
