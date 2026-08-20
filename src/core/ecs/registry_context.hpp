#pragma once

#include <any>
#include <typeindex>
#include <unordered_map>

namespace ecs {

class RegistryContext {
public:
    template <typename T, typename... Args>
    T& emplace(Args&&... args) {
        auto [it, inserted] = values.emplace(std::type_index(typeid(T)), std::any(std::in_place_type<T>, std::forward<Args>(args)...));
        return std::any_cast<T&>(it->second);
    }

    template <typename T>
    T& get() { return std::any_cast<T&>(values.at(std::type_index(typeid(T)))); }

    template <typename T>
    const T& get() const { return std::any_cast<const T&>(values.at(std::type_index(typeid(T)))); }

private:
    std::unordered_map<std::type_index, std::any> values;
};

} // namespace ecs
