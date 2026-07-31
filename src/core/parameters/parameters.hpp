#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "core/field.hpp"

using ParameterPath = std::filesystem::path;

struct ParameterCondition {
    ParameterPath param;
    bool when = true;
};

class Parameter : public Field {
public:
    const ParameterPath& getPath() const { return path; }
    const std::optional<std::string>& getDescription() const { return description; }
    bool isRestartingAnimation() const { return restartAccumulation; }
    const std::optional<ParameterCondition>& getCondition() const { return condition; }

    Parameter& setDescription(std::string d) { description = std::move(d); return *this; }
    Parameter& setCondition(ParameterCondition c) { condition = std::move(c); return *this; }

    template<typename T>
    Parameter& bind(T* ptr) requires (!std::is_enum_v<T>) {
        onSync = [ptr, this]() { *ptr = get<T>(); };
        onSync();
        return *this;
    }
    template<typename T>
    Parameter& bind(T* ptr) requires (std::is_enum_v<T>) {
        onSync = [ptr, this]() { *ptr = static_cast<T>(get<int>()); };
        onSync();
        return *this;
    }
    template<typename T>
    Parameter& bind(std::function<void(T)> cb) requires (!std::is_enum_v<T>) {
        onSync = [cb = std::move(cb), this]() { cb(get<T>()); };
        onSync();
        return *this;
    }
    template<typename T>
    Parameter& bind(std::function<void(T)> cb) requires (std::is_enum_v<T>) {
        onSync = [cb = std::move(cb), this]() { cb(static_cast<T>(get<int>())); };
        onSync();
        return *this;
    }

    template<typename T>
    void set(const T& v) { Field::set(v); sync(); }

    void reset() { Field::reset(); sync(); }
    void sync() { if (onSync) onSync(); }

    std::string print() const;

    template<typename T>
    static Parameter make(const ParameterPath& path, const std::string& label, const T& value, FieldMetadata metadata = {}, bool restartAccumulation = false) {
        Parameter p;
        static_cast<Field&>(p) = Field::make<T>(path.generic_string(), label, value, std::move(metadata));
        p.path = path;
        p.restartAccumulation = restartAccumulation;
        return p;
    }

private:
    ParameterPath path;
    std::optional<std::string> description;
    bool restartAccumulation = false;
    std::optional<ParameterCondition> condition;
    std::function<void()> onSync;
};

class ParameterRegistry {
public:
    void setNodeLabel(const ParameterPath& path, const std::string& label) { nodeLabels[path.generic_string()] = label; }
    const std::vector<std::unique_ptr<Parameter>>& getAll() const { return parameters; }
    const std::unordered_map<std::string, std::string>& getNodeLabels() const { return nodeLabels; }
    void resetAll();
    void syncAll();

    template <typename T>
    Parameter& add(const ParameterPath& path, const std::string& label, const T& value, FieldMetadata metadata = {}, bool restartAccumulation = false) {
        auto up = std::make_unique<Parameter>(Parameter::make<T>(path, label, value, std::move(metadata), restartAccumulation));
        index[up->getPath().generic_string()] = up.get();
        parameters.push_back(std::move(up));
        return *parameters.back();
    }

    template <typename T> T get(const ParameterPath& path) requires (!std::is_enum_v<T>) {
        return getParam(path).get<T>();
    }
    template <typename T> T get(const ParameterPath& path) requires (std::is_enum_v<T>) {
        return static_cast<T>(getParam(path).get<int>());
    }

    template <typename T> void set(const ParameterPath& path, const T& value) requires (!std::is_enum_v<T>) {
        getParam(path).set(value);
    }
    template <typename T> void set(const ParameterPath& path, T value) requires (std::is_enum_v<T>) {
        getParam(path).set(std::to_underlying(value));
    }

    template <typename T> void bind(const ParameterPath& path, T* ptr) {
        getParam(path).bind<T>(ptr);
    }
    template <typename T> void bind(const ParameterPath& path, std::function<void(T)> cb) {
        getParam(path).bind<T>(std::move(cb));
    }

    void setEnumByName(const ParameterPath& path, const std::string& name);

private:
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unordered_map<std::string, Parameter*> index;
    std::unordered_map<std::string, std::string> nodeLabels;

    Parameter& getParam(const ParameterPath& path);
};
