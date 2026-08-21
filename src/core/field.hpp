#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/ecs/entity.hpp"

enum class FieldType {
    Bool,
    Int, IVec2, IVec3, IVec4,
    Float, Vec2, Vec3, Vec4,
    Quat,
    Entity,
    String,
    Enum,
    Path
};

struct PathExtension {
    std::string ext;
    std::string name;
    std::string displayName() const { return name.empty() ? ext : name; }
};

struct FieldPreset {
    std::string label;
    std::variant<float, int, std::string> value;
};

struct NumericMeta {
    float min = -std::numeric_limits<float>::infinity();
    float max =  std::numeric_limits<float>::infinity();
    float step = 0.0f;
    bool color = false;
    std::vector<FieldPreset> presets;
};

struct PathMeta {
    std::vector<PathExtension> extensions;
    bool save = false;
    std::vector<FieldPreset> presets;
};

struct EnumMeta {
    std::vector<std::string> items;
};

struct EntityMeta {
    std::vector<std::string> needs;
    std::vector<std::string> conflicts;
};

using FieldMetadata = std::variant<std::monostate, NumericMeta, PathMeta, EnumMeta, EntityMeta>;

class FieldValue {
public:
    static constexpr size_t maxStringSize = 128;
    static constexpr size_t maxPathSize = 512;

    FieldType getType() const { return type; }

    template<typename T>
    static FieldValue make(const T& v) {
        FieldValue fv;
        if constexpr (std::is_same_v<T, std::filesystem::path>)
            { fv.type = FieldType::Path; fv.size = maxPathSize; }
        else if constexpr (std::is_same_v<T, std::string>)
            { fv.type = FieldType::String; fv.size = maxStringSize; }
        else {
            fv.type = typeOf<T>(); fv.size = sizeof(T);
        }
        fv.value.resize(fv.size, std::byte{0});
        writeBlob(fv.value, v);
        return fv;
    }

    template<typename T> T get() const { return readBlob<T>(value); }
    template<typename T> void set(const T& v) { writeBlob(value, v); }

    friend bool operator==(const FieldValue& a, const FieldValue& b) { return a.type == b.type && a.value == b.value; }
    friend bool operator!=(const FieldValue& a, const FieldValue& b) { return !(a == b); }

    template<typename F>
    void dispatch(F&& f) const {
        switch (type) {
            case FieldType::Bool:  f(get<bool>()); break;
            case FieldType::Int:
            case FieldType::Enum:  f(get<int>()); break;
            case FieldType::IVec2: f(get<glm::ivec2>()); break;
            case FieldType::IVec3: f(get<glm::ivec3>()); break;
            case FieldType::IVec4: f(get<glm::ivec4>()); break;
            case FieldType::Float: f(get<float>()); break;
            case FieldType::Vec2:  f(get<glm::vec2>()); break;
            case FieldType::Vec3:  f(get<glm::vec3>()); break;
            case FieldType::Vec4:  f(get<glm::vec4>()); break;
            case FieldType::Quat:  f(get<glm::quat>()); break;
            default: break;
        }
    }

protected:
    template<typename T> static FieldType typeOf();

    FieldType type = FieldType::Bool;
    size_t size = 0;
    std::vector<std::byte> value;

    template<typename T>
    static T readBlob(const std::vector<std::byte>& buf) {
        if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, std::filesystem::path>)
            return T(reinterpret_cast<const char*>(buf.data()));
        else
            return *reinterpret_cast<const T*>(buf.data());
    }

    template<typename T>
    static void writeBlob(std::vector<std::byte>& buf, const T& v) {
        if constexpr (std::is_same_v<T, std::filesystem::path>) {
            std::fill(buf.begin(), buf.end(), std::byte{0});
            auto s = v.string();
            std::snprintf(reinterpret_cast<char*>(buf.data()), buf.size(), "%s", s.c_str());
        } else if constexpr (std::is_same_v<T, std::string>) {
            std::fill(buf.begin(), buf.end(), std::byte{0});
            std::snprintf(reinterpret_cast<char*>(buf.data()), buf.size(), "%s", v.c_str());
        } else {
            *reinterpret_cast<T*>(buf.data()) = v;
        }
    }
};

class Field : public FieldValue {
public:
    const std::string& getId() const { return id; }
    const std::string& getLabel() const { return label; }
    const FieldMetadata& getMetadata() const { return metadata; }

    template<typename T>
    static Field make(std::string id, std::string label, const T& def, FieldMetadata metadata = {}) {
        Field f;
        f.id = std::move(id);
        f.label = std::move(label);
        f.metadata = std::move(metadata);
        if constexpr (std::is_same_v<T, std::filesystem::path>)
            { f.type = FieldType::Path; f.size = maxPathSize; }
        else if constexpr (std::is_same_v<T, std::string>)
            { f.type = FieldType::String; f.size = maxStringSize; }
        else {
            f.type = typeOf<T>(); f.size = sizeof(T);
            if constexpr (std::is_same_v<T, int>)
                if (std::holds_alternative<EnumMeta>(f.metadata)) f.type = FieldType::Enum;
        }
        f.value.resize(f.size, std::byte{0});
        writeBlob(f.value, def);
        f.defaultValue = f.value;
        return f;
    }

    template<typename T> T getDefault() const { return readBlob<T>(defaultValue); }
    template<typename T> void setDefault(const T& v) { writeBlob(defaultValue, v); }

    void reset() { value = defaultValue; }

    int findPreset() const {
        const auto* presets = getPresets();
        if (!presets) return -1;
        for (int i = 0; i < (int)presets->size(); i++) {
            bool m = std::visit([this](const auto& val) -> bool {
                using V = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<V, std::string>) {
                    if (type == FieldType::Path)   return get<std::filesystem::path>().string() == val;
                    if (type == FieldType::String) return get<std::string>() == val;
                } else if constexpr (std::is_same_v<V, int>) {
                    if (type == FieldType::Int || type == FieldType::Enum) return get<int>() == val;
                } else {
                    if (type == FieldType::Float) return get<float>() == val;
                }
                return false;
            }, (*presets)[i].value);
            if (m) return i;
        }
        return -1;
    }

    void applyPreset(int idx) {
        const auto* presets = getPresets();
        if (!presets) return;
        std::visit([this](const auto& val) {
            using V = std::decay_t<decltype(val)>;
            if constexpr (std::is_same_v<V, std::string>) {
                if (type == FieldType::Path)   set<std::filesystem::path>(std::filesystem::path(val));
                if (type == FieldType::String) set<std::string>(val);
            } else if constexpr (std::is_same_v<V, int>) {
                if (type == FieldType::Int || type == FieldType::Enum) set<int>(val);
            } else {
                if (type == FieldType::Float) set<float>(val);
            }
        }, (*presets)[idx].value);
    }

    const std::vector<FieldPreset>* getPresets() const {
        if (const auto* num = std::get_if<NumericMeta>(&metadata)) return &num->presets;
        if (const auto* path = std::get_if<PathMeta>(&metadata)) return &path->presets;
        return nullptr;
    }

private:
    std::string id;
    std::string label;
    FieldMetadata metadata;
    std::vector<std::byte> defaultValue;
};
