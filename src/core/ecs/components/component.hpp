#pragma once

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "component_type.hpp"

namespace ecs {

class Component {
public:
    explicit Component(const ComponentType& proto) : type(&proto), data(proto.getTotalSize()) {
        for (const auto& f : proto.getFields())
            std::memcpy(data.data() + f.offset, f.defaultValue.data(), f.size);
    }
    Component(Component&&) = default;
    Component& operator=(Component&&) = default;

    template<typename T>
    T get(const std::string& fieldId) const {
        return *reinterpret_cast<const T*>(getRaw(fieldId));
    }

    template<typename T>
    void set(const std::string& fieldId, const T& value) {
        *reinterpret_cast<T*>(getRaw(fieldId)) = value;
    }

    template<typename T>
    T* getPtr(const std::string& fieldId) {
        static_assert(!std::is_same_v<T, std::string>, "use getPtr<char>() for String fields");
        return reinterpret_cast<T*>(getRaw(fieldId));
    }

    template<typename T>
    const T* getPtr(const std::string& fieldId) const {
        static_assert(!std::is_same_v<T, std::string>, "use getPtr<char>() for String fields");
        return reinterpret_cast<const T*>(getRaw(fieldId));
    }

    const ComponentType& getType() const { return *type; }

private:
    std::byte* getRaw(const std::string& fieldId) {
        return data.data() + type->getField(fieldId).offset;
    }

    const std::byte* getRaw(const std::string& fieldId) const {
        return data.data() + type->getField(fieldId).offset;
    }

    const ComponentType* type;
    std::vector<std::byte> data;
};

template<>
inline std::string Component::get<std::string>(const std::string& fieldId) const {
    return std::string(reinterpret_cast<const char*>(getRaw(fieldId)));
}

template<>
inline void Component::set<std::string>(const std::string& fieldId, const std::string& value) {
    std::snprintf(reinterpret_cast<char*>(getRaw(fieldId)), maxStringFieldSize, "%s", value.c_str());
}

} // namespace ecs
