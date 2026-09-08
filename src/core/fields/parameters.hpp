#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "field.hpp"

struct ParameterCondition {
    FieldPath param;
    bool when = true;
};

class Parameter : public Field {
public:
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
    static Parameter make(const FieldPath& id, const std::string& label, const T& value, FieldMetadata metadata = {}, bool restartAccumulation = false) {
        Parameter p;
        static_cast<Field&>(p) = Field::make<T>(id, label, value, std::move(metadata));
        p.restartAccumulation = restartAccumulation;
        return p;
    }

private:
    std::optional<std::string> description;
    bool restartAccumulation = false;
    std::optional<ParameterCondition> condition;
    std::function<void()> onSync;
};

class ParameterRegistry {
public:
    void setNodeLabel(const FieldPath& id, const std::string& label) { nodeLabels[id.string()] = label; }
    const std::vector<std::unique_ptr<Parameter>>& getAll() const { return parameters; }
    const std::unordered_map<std::string, std::string>& getNodeLabels() const { return nodeLabels; }
    void resetAll();
    void syncAll();

    template <typename T>
    Parameter& add(const FieldPath& id, const std::string& label, const T& value, FieldMetadata metadata = {}, bool restartAccumulation = false) {
        std::unique_ptr<Parameter> up = std::make_unique<Parameter>(Parameter::make<T>(id, label, value, std::move(metadata), restartAccumulation));
        index[up->getId().string()] = up.get();
        parameters.push_back(std::move(up));
        return *parameters.back();
    }

    template <typename T> T get(const FieldPath& id) requires (!std::is_enum_v<T>) {
        return getParam(id).get<T>();
    }
    template <typename T> T get(const FieldPath& id) requires (std::is_enum_v<T>) {
        return static_cast<T>(getParam(id).get<int>());
    }

    template <typename T> void set(const FieldPath& id, const T& value) requires (!std::is_enum_v<T>) {
        getParam(id).set(value);
    }
    template <typename T> void set(const FieldPath& id, T value) requires (std::is_enum_v<T>) {
        getParam(id).set(std::to_underlying(value));
    }

    template <typename T> void bind(const FieldPath& id, T* ptr) {
        getParam(id).bind<T>(ptr);
    }
    template <typename T> void bind(const FieldPath& id, std::function<void(T)> cb) {
        getParam(id).bind<T>(std::move(cb));
    }

    void setEnumByName(const FieldPath& id, const std::string& name);

private:
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unordered_map<std::string, Parameter*> index;
    std::unordered_map<std::string, std::string> nodeLabels;

    Parameter& getParam(const FieldPath& id);
};
