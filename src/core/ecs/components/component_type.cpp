#include "component_type.hpp"

#include "utils/string_utils.hpp"

namespace ecs {

ComponentType::Builder ComponentType::builder(std::string id) {
    return ComponentType::Builder(std::move(id));
}

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

std::vector<ComponentType> ComponentType::storage;

const std::vector<ComponentType>& ComponentType::all() {
    return storage;
}

std::optional<std::reference_wrapper<const ComponentType>> ComponentType::find(const std::string& id) {
    for (const auto& t : storage)
        if (t.getId() == id) return t;
    return std::nullopt;
}

ComponentType& ComponentType::Builder::build() {
    ComponentType::storage.push_back(std::move(type));
    return ComponentType::storage.back();
}

ComponentType ComponentType::Builder::buildDetached() {
    return std::move(type);
}

} // namespace ecs
