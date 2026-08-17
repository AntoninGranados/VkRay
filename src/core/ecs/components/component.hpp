#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "component_type.hpp"

namespace ecs {

class Component {
public:
    explicit Component(const ComponentType& proto) : type(&proto) {
        for (const auto& f : proto.getFields()) {
            fieldIndex[f.getId()] = fields.size();
            fields.push_back(f);
        }
        for (const auto& p : proto.getPayloads())
            payloads.emplace_back(p.construct(), p.destroy);
    }
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;

    template<typename T>
    T get(const std::string& id) const { return getField(id).get<T>(); }

    template<typename T>
    void set(const std::string& id, const T& v) { getField(id).set(v); }

    ComponentField& getField(const std::string& id) {
        assert(fieldIndex.contains(id));
        return fields[fieldIndex.at(id)];
    }
    const ComponentField& getField(const std::string& id) const {
        assert(fieldIndex.contains(id));
        return fields[fieldIndex.at(id)];
    }

    std::vector<ComponentField>& getFields() { return fields; }
    const std::vector<ComponentField>& getFields() const { return fields; }

    template<typename T>
    T& payload(const std::string& id) {
        assert(type->getPayloads()[type->getPayloadIndex(id)].typeId == std::type_index(typeid(T)));
        return *static_cast<T*>(payloads[type->getPayloadIndex(id)].get());
    }
    template<typename T>
    const T& payload(const std::string& id) const {
        assert(type->getPayloads()[type->getPayloadIndex(id)].typeId == std::type_index(typeid(T)));
        return *static_cast<const T*>(payloads[type->getPayloadIndex(id)].get());
    }

    const ComponentType& getType() const { return *type; }

private:
    const ComponentType* type;
    std::vector<ComponentField> fields;
    std::unordered_map<std::string, size_t> fieldIndex;
    std::vector<std::unique_ptr<void, std::function<void(void*)>>> payloads;
};

} // namespace ecs
