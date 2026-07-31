#pragma once

#include <cassert>
#include <string>
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

    const ComponentType& getType() const { return *type; }

private:
    const ComponentType* type;
    std::vector<ComponentField> fields;
    std::unordered_map<std::string, size_t> fieldIndex;
};

} // namespace ecs
