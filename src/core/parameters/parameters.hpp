#pragma once

#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/type_trait.hpp>

using ParameterPath = std::filesystem::path;

struct PathExtension {
    std::string ext;
    std::string name;
    std::string displayName() const { return name.empty() ? ext : name; }
};

struct ParameterCondition {
    ParameterPath param;
    bool when = true;
};

class ParameterBase {
public:
    virtual ~ParameterBase() = default;
    virtual void reset() = 0;
    virtual std::string print() = 0;

    ParameterBase& setDescription(std::string d) { description = std::move(d); return *this; }
    ParameterBase& setCondition(ParameterCondition c) { condition = std::move(c); return *this; }

    ParameterPath path;
    std::string label;
    std::optional<std::string> description;
    bool restartAccumulation = false;
    std::optional<ParameterCondition> condition;
    std::function<void()> onSync;
};

class IntParameter : public ParameterBase {
public:
    IntParameter(
        const ParameterPath& path_,
        const std::string& label_,
        int value_,
        int minValue_,
        int maxValue_,
        int step_,
        bool restart_
    );
    std::string print() override;
    void reset() override { value = defaultValue; if (onSync) onSync(); }
    int& get() { return value; }
    int getMin() const { return minValue; }
    int getMax() const { return maxValue; }
    int getStep() const { return step; }

private:
    int value = 0;
    int defaultValue = 0;
    int minValue = 0;
    int maxValue = 0;
    int step = 1;
};

class FloatParameter : public ParameterBase {
public:
    FloatParameter(
        const ParameterPath& path_,
        const std::string& label_,
        float value_,
        float minValue_,
        float maxValue_,
        float step_,
        bool restart_
    );
    std::string print() override;
    void reset() override { value = defaultValue; if (onSync) onSync(); }
    float& get() { return value; }
    float getMin() const { return minValue; }
    float getMax() const { return maxValue; }
    float getStep() const { return step; }

private:
    float value = 0.0f;
    float defaultValue = 0.0f;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    float step = 0.1f;
};

class BoolParameter : public ParameterBase {
public:
    BoolParameter(
        const ParameterPath& path_,
        const std::string& label_,
        bool value_,
        bool restart_
    );
    std::string print() override;
    void reset() override { value = defaultValue; if (onSync) onSync(); }
    bool& get() { return value; }

private:
    bool value = false;
    bool defaultValue = false;
};

class EnumParameter : public ParameterBase {
public:
    EnumParameter(
        const ParameterPath& path_,
        const std::string& label_,
        int value_,
        std::vector<std::string> items_,
        bool restart_
    );
    std::string print() override;
    void reset() override { value = defaultValue; if (onSync) onSync(); }
    int& get() { return value; }
    const std::vector<std::string>& getItems() const { return items; }
    bool setByName(const std::string& name) {
        for (size_t i = 0; i < items.size(); i++)
            if (items[i] == name) { value = static_cast<int>(i); return true; }
        return false;
    }

private:
    int value = 0;
    int defaultValue = 0;
    std::vector<std::string> items;
};

class PathParameter : public ParameterBase {
public:
    PathParameter(
        const ParameterPath& path_,
        const std::string& label_,
        std::filesystem::path value_,
        std::vector<PathExtension> extensions_,
        bool restart_
    );
    std::string print() override;
    void reset() override { value = defaultValue; if (onSync) onSync(); }
    std::filesystem::path& get() { return value; }
    const std::vector<PathExtension>& getExtensions() const { return extensions; }

private:
    std::filesystem::path value;
    std::filesystem::path defaultValue;
    std::vector<PathExtension> extensions;
};

template <typename T>
class VecParameter : public ParameterBase {
public:
    VecParameter(
        const ParameterPath& path_,
        const std::string& label_,
        T value_,
        T minValue_,
        T maxValue_,
        float step_,
        bool restart_
    ) : value(value_), defaultValue(value_), minValue(minValue_), maxValue(maxValue_), step(step_) {
        path = path_;
        label = label_;
        restartAccumulation = restart_;
    }
    std::string print() override {
        using V = typename T::value_type;
        std::string val, mn, mx;
        for (int i = 0; i < T::length(); i++) {
            if (i > 0) { val += ", "; mn += ", "; mx += ", "; }
            val += std::format("{}", value[i]);
            mn  += std::format("{}", minValue[i]);
            mx  += std::format("{}", maxValue[i]);
        }
        bool hasMn = minValue != T(std::numeric_limits<V>::lowest());
        bool hasMx = maxValue != T(std::numeric_limits<V>::max());
        std::string constraints = "-";
        if (hasMn && hasMx) constraints = std::format("({}) ... ({})", mn, mx);
        else if (hasMn)     constraints = std::format("({}) ...", mn);
        else if (hasMx)     constraints = std::format("... ({})", mx);
        return std::format("| `{}` | {} | {} | Vec{} | ({}) | {} | {} |",
            path.c_str(), label, description.value_or("-"), T::length(),
            val, constraints, restartAccumulation ? "✓" : "-");
    }
    void reset() override { value = defaultValue; if (onSync) onSync(); }
    T& get() { return value; }
    T getMin() const { return minValue; }
    T getMax() const { return maxValue; }
    float getStep() const { return step; }

private:
    T value;
    T defaultValue;
    T minValue;
    T maxValue;
    float step;
};

class ParameterRegistry {
public:
    void setNodeLabel(const ParameterPath& path, const std::string& label) { nodeLabels[path.generic_string()] = label; }
    const std::vector<std::unique_ptr<ParameterBase>>& getParameterList() const { return parameters; }
    const std::unordered_map<std::string, std::string>& getNodeLabels() const { return nodeLabels; }
    void resetAll();
    void syncAll();

    IntParameter& addInt(
        const ParameterPath& path,
        const std::string& label,
        int value,
        int minValue,
        int maxValue,
        int step,
        bool restartAccumulation
    );
    FloatParameter& addFloat(
        const ParameterPath& path,
        const std::string& label,
        float value,
        float minValue,
        float maxValue,
        float step,
        bool restartAccumulation
    );
    BoolParameter& addBool(
        const ParameterPath& path,
        const std::string&   label,
        bool value,
        bool restartAccumulation
    );
    EnumParameter& addEnum(
        const ParameterPath& path,
        const std::string& label,
        int value,
        std::vector<std::string> items,
        bool restartAccumulation
    );
    template <typename EnumT>
    EnumParameter& addEnum(
        const ParameterPath& path,
        const std::string& label,
        EnumT value,
        std::vector<std::string> items,
        bool restartAccumulation
    ) {
        return addEnum(path, label, std::to_underlying(value), items, restartAccumulation);
    }
    PathParameter& addPath(
        const ParameterPath& path,
        const std::string& label,
        std::filesystem::path value,
        std::vector<PathExtension> extensions,
        bool restartAccumulation
    );
    template <typename T>
    VecParameter<T>& addVec(
        const ParameterPath& path,
        const std::string& label,
        T value,
        T minValue,
        T maxValue,
        float step,
        bool restartAccumulation
    ) {
        auto parameter = std::make_unique<VecParameter<T>>(path, label, value, minValue, maxValue, step, restartAccumulation);
        index[path.generic_string()] = parameter.get();
        parameters.push_back(std::move(parameter));
        return static_cast<VecParameter<T>&>(*parameters.back());
    }

    template <typename T> T get(const ParameterPath& path) requires (!std::is_enum_v<T> && !glm::type<T>::is_vec);
    template <typename T> T get(const ParameterPath& path) requires (std::is_enum_v<T>) {
        return static_cast<T>(getParameter<EnumParameter>(path).get());
    }
    template <typename T> T get(const ParameterPath& path) requires (glm::type<T>::is_vec) {
        return getParameter<VecParameter<T>>(path).get();
    }

    template <typename T> void set(const ParameterPath& path, T value) requires (!std::is_enum_v<T> && !glm::type<T>::is_vec);
    template <typename T> void set(const ParameterPath& path, T value) requires (std::is_enum_v<T>) {
        auto& parameter = getParameter<EnumParameter>(path);
        parameter.get() = std::to_underlying(value);
        if (parameter.onSync) parameter.onSync();
    }
    template <typename T> void set(const ParameterPath& path, T value) requires (glm::type<T>::is_vec) {
        auto& parameter = getParameter<VecParameter<T>>(path);
        parameter.get() = value;
        if (parameter.onSync) parameter.onSync();
    }

    template <typename T> void bind(const ParameterPath& path, T* ptr) requires (!std::is_enum_v<T> && !glm::type<T>::is_vec);
    template <typename T> void bind(const ParameterPath& path, T* ptr) requires (std::is_enum_v<T>) {
        auto& parameter = getParameter<EnumParameter>(path);
        parameter.onSync = [ptr, &parameter]() { *ptr = static_cast<T>(parameter.get()); };
        parameter.onSync();
    }
    template <typename T> void bind(const ParameterPath& path, T* ptr) requires (glm::type<T>::is_vec) {
        auto& parameter = getParameter<VecParameter<T>>(path);
        parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
        parameter.onSync();
    }

    template <typename T> void bind(const ParameterPath& path, std::function<void(T)> callback) requires (!std::is_enum_v<T> && !glm::type<T>::is_vec);
    template <typename T> void bind(const ParameterPath& path, std::function<void(T)> callback) requires (std::is_enum_v<T>) {
        auto& parameter = getParameter<EnumParameter>(path);
        parameter.onSync = [callback = std::move(callback), &parameter]() { callback(static_cast<T>(parameter.get())); };
        parameter.onSync();
    }
    template <typename T> void bind(const ParameterPath& path, std::function<void(T)> callback) requires (glm::type<T>::is_vec) {
        auto& parameter = getParameter<VecParameter<T>>(path);
        parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
        parameter.onSync();
    }

    void setEnumByName(const ParameterPath& path, const std::string& name);

private:
    std::vector<std::unique_ptr<ParameterBase>> parameters;
    std::unordered_map<std::string, ParameterBase*> index;
    std::unordered_map<std::string, std::string> nodeLabels;

    template <typename T>
    T& getParameter(const ParameterPath& path) {
        auto it = index.find(path.generic_string());
        if (it == index.end()) throw std::runtime_error("Parameter not found: " + path.generic_string());
        auto parameter = dynamic_cast<T*>(it->second);
        if (!parameter) throw std::runtime_error("Parameter type mismatch: " + path.generic_string());
        return *parameter;
    }
};
