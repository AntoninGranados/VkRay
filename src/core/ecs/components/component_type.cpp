#include "component_type.hpp"

namespace ecs {

ComponentType::Builder ComponentType::builder(std::string id) {
    return ComponentType::Builder(std::move(id));
}

ComponentType::Builder::Builder(std::string id) {
    type.label = deriveLabel(id);
    type.id = std::move(id);
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

ComponentType& ComponentType::Builder::build() {
    ComponentType::storage.push_back(std::move(type));
    return ComponentType::storage.back();
}

std::string ComponentType::Builder::deriveLabel(const std::string& id) {
    std::string result;
    bool capitalize = true;
    for (const auto& c : id) {
        if (capitalize) {
            result += std::toupper(c);
            capitalize = false;
        } else if (c == '_') {
            result += ' ';
            capitalize = true;
        } else {
            result += c;
        }
    }
    return result;
}

} // namespace ecs
