#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "component_type.hpp"

namespace ecs {

class Component {
public:
    explicit Component(const ComponentType& proto) : type(&proto), fields(proto.getFields()) {
        for (const auto& p : proto.getPayloads()) payloads.emplace_back(p.construct(), p.destroy);
    }
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;

    template <typename T> T get(const std::string& id) const { return getField(id).get<T>(); }

    template <typename T> void set(const std::string& id, const T& v) { getField(id).set(v); }

    Field& getField(const std::string& id) {
        if (Field* found = findField(id)) return *found;
        throw std::out_of_range("unknown field id: " + id);
    }
    const Field& getField(const std::string& id) const { return const_cast<Component*>(this)->getField(id); }

    std::vector<Field>& getFields() { return fields; }
    const std::vector<Field>& getFields() const { return fields; }

    void forEachField(const std::function<void(Field&)>& fn) {
        for (Field& f : fields) fn(f);
        const auto& payloadTypes = type->getPayloads();
        for (size_t i = 0; i < payloads.size(); ++i)
            if (payloadTypes[i].asComponent) payloadTypes[i].asComponent(payloads[i].get())->forEachField(fn);
    }

    template <typename T> T& payload(const std::string& id) {
        assert(type->getPayloads()[type->getPayloadIndex(id)].typeId == std::type_index(typeid(T)));
        return *static_cast<T*>(payloads[type->getPayloadIndex(id)].get());
    }
    template <typename T> const T& payload(const std::string& id) const {
        assert(type->getPayloads()[type->getPayloadIndex(id)].typeId == std::type_index(typeid(T)));
        return *static_cast<const T*>(payloads[type->getPayloadIndex(id)].get());
    }

    const ComponentType& getType() const { return *type; }

private:
    Field* findField(const std::string& id) {
        if (const std::optional<size_t> index = type->findFieldIndex(id)) return &fields[*index];

        const auto& payloadTypes = type->getPayloads();
        for (size_t i = 0; i < payloads.size(); ++i) {
            if (!payloadTypes[i].asComponent) continue;
            if (Field* found = payloadTypes[i].asComponent(payloads[i].get())->findField(id)) return found;
        }
        return nullptr;
    }

    const ComponentType* type;
    std::vector<Field> fields;
    std::vector<std::unique_ptr<void, std::function<void(void*)>>> payloads;
};

} // namespace ecs
