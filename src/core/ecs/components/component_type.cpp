#include "component_type.hpp"

#include "utils/string_utils.hpp"

namespace ecs {

ComponentType::Builder ComponentType::builder(std::string id) { return ComponentType::Builder(std::move(id)); }

ComponentType::Builder::Builder(std::string id) {
    type.label = snakeCaseToLabel(id);
    type.id = std::move(id);
}

ComponentType::Builder& ComponentType::Builder::description(std::string description) {
    type.description = std::move(description);
    return *this;
}

ComponentType::Builder& ComponentType::Builder::icon(std::string icon) {
    type.icon = std::move(icon);
    return *this;
}

ComponentType::Builder& ComponentType::Builder::group(std::string group) {
    type.group = std::move(group);
    return *this;
}

std::deque<ComponentType>& ComponentType::storage() {
    static std::deque<ComponentType> types;
    return types;
}

std::vector<const ComponentType*>& ComponentType::registered() {
    static std::vector<const ComponentType*> types;
    return types;
}

const std::vector<const ComponentType*>& ComponentType::all() { return registered(); }

std::optional<std::reference_wrapper<const ComponentType>> ComponentType::find(const std::string& id) {
    for (const ComponentType* type : registered())
        if (type->getId() == id) return *type;
    return std::nullopt;
}

std::vector<const ComponentType*> ComponentType::inGroup(const std::string& group) {
    std::vector<const ComponentType*> types;
    for (const ComponentType* type : registered())
        if (type->getGroup() == group) types.push_back(type);
    return types;
}

const ComponentType& ComponentType::Builder::build() {
    storage().push_back(std::move(type));
    registered().push_back(&storage().back());
    return storage().back();
}

ComponentType ComponentType::Builder::buildDetached() { return std::move(type); }

} // namespace ecs
