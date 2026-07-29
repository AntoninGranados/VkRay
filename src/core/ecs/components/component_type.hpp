#pragma once

#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "fields.hpp"

namespace ecs {

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

    const std::vector<Field>& getFields() const { return fields; }
    const Field& getField(const std::string& id) const { return fields[fieldIndex.at(id)]; }
    size_t getTotalSize() const { return totalSize; }

    static const std::vector<ComponentType>& all();

private:
    friend class ComponentType::Builder;

    static std::vector<ComponentType> storage;

    std::string id;
    std::string label;

    std::string icon = "";
    std::string group = "";

    std::vector<std::string> needs = {};
    std::vector<std::string> conflicts = {};
    std::vector<Field> fields = {};
    std::unordered_map<std::string, size_t> fieldIndex = {};
    size_t totalSize = 0;
};

class ComponentType::Builder {
public:
    explicit Builder(std::string id);

    Builder& icon(std::string icon);
    Builder& group(std::string group);

    template <typename T>
    Builder& field(std::string id, T defaultValue = T{}, FieldMetadata metadata = {}) {
        return addField<T>(std::move(id), std::move(defaultValue), metadata, false);
    }

    template <typename T>
    Builder& privateField(std::string id, T defaultValue = T{}, FieldMetadata metadata = {}) {
        return addField<T>(std::move(id), std::move(defaultValue), metadata, true);
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
    Builder& addField(std::string id, T defaultValue, FieldMetadata metadata, bool isPrivate) {
        const size_t size = std::is_same_v<T, std::string> ? maxStringFieldSize : sizeof(T);
        const size_t index = type.fields.size();
        std::string label = deriveLabel(id);
        if (type.fieldIndex.contains(id))
            throw std::invalid_argument("duplicate field id: " + id);
        std::vector<std::byte> defBytes(size, std::byte{0});
        if constexpr (std::is_same_v<T, std::string>)
            std::snprintf(reinterpret_cast<char*>(defBytes.data()), size, "%s", defaultValue.c_str());
        else
            std::memcpy(defBytes.data(), &defaultValue, size);
        type.fieldIndex.emplace(id, index);
        type.fields.push_back(Field{
            .id = std::move(id),
            .label = std::move(label),
            .type = getFieldType<T>(),
            .metadata = metadata,
            .size = size,
            .offset = currentOffset,
            .isPrivate = isPrivate,
            .defaultValue = std::move(defBytes)
        });
        currentOffset += size;
        return *this;
    }

    ComponentType type = {};
    size_t currentOffset = 0;
};

} // namespace ecs
