#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using ParameterPath = std::filesystem::path;

struct ParameterCondition {
    ParameterPath param;
    bool when = true;
};

class ParameterBase {
public:
    virtual ~ParameterBase() = default;
    virtual void reset() = 0;
    virtual std::string print() = 0;

    void setDescription(std::string d) { description = std::move(d); }
    void setCondition(ParameterCondition c) { condition = std::move(c); }

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
        const std::string&   label_,
        int  value_,
        int  minValue_,
        int  maxValue_,
        int  step_,
        bool restart_
    );
    std::string print() override;
    void reset() override { value = defaultValue; if (onSync) onSync(); }
    int& get() { return value; }
    int getMin()  const { return minValue; }
    int getMax()  const { return maxValue; }
    int getStep() const { return step; }

private:
    int value        = 0;
    int defaultValue = 0;
    int minValue     = 0;
    int maxValue     = 0;
    int step         = 1;
};

class FloatParameter : public ParameterBase {
public:
    FloatParameter(
        const ParameterPath& path_,
        const std::string&   label_,
        float value_,
        float minValue_,
        float maxValue_,
        float step_,
        bool  restart_
    );
    std::string print() override;
    void  reset() override { value = defaultValue; if (onSync) onSync(); }
    float& get() { return value; }
    float getMin()  const { return minValue; }
    float getMax()  const { return maxValue; }
    float getStep() const { return step; }

private:
    float value        = 0.0f;
    float defaultValue = 0.0f;
    float minValue     = 0.0f;
    float maxValue     = 0.0f;
    float step         = 0.1f;
};

class BoolParameter : public ParameterBase {
public:
    BoolParameter(
        const ParameterPath& path_,
        const std::string&   label_,
        bool value_,
        bool restart_
    );
    std::string print() override;
    void  reset() override { value = defaultValue; if (onSync) onSync(); }
    bool& get() { return value; }

private:
    bool value        = false;
    bool defaultValue = false;
};

class EnumParameter : public ParameterBase {
public:
    EnumParameter(
        const ParameterPath&      path_,
        const std::string&        label_,
        int                       value_,
        std::vector<std::string>  items_,
        bool                      restart_
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
    int value        = 0;
    int defaultValue = 0;
    std::vector<std::string> items;
};

class ParameterHandler {
public:
    static ParameterHandler fromFile(std::filesystem::path path = "./assets/parameters/parameters.json");
    void saveDocumentation(std::filesystem::path path = "./docs/parameters.md");
    void setNodeLabel (const ParameterPath& path, const std::string& label);
    void resetAll();

    IntParameter& addInt(
        const ParameterPath& path,
        const std::string&   label,
        int  value,
        int  minValue,
        int  maxValue,
        int  step,
        bool restartAccumulation
    );
    FloatParameter& addFloat(
        const ParameterPath& path,
        const std::string&   label,
        float value,
        float minValue,
        float maxValue,
        float step,
        bool  restartAccumulation
    );
    BoolParameter& addBool(
        const ParameterPath& path,
        const std::string&   label,
        bool value,
        bool restartAccumulation
    );
    EnumParameter& addEnum(
        const ParameterPath&     path,
        const std::string&       label,
        int                      value,
        std::vector<std::string> items,
        bool                     restartAccumulation
    );
    template <typename EnumT>
    EnumParameter& addEnum(
        const ParameterPath&     path,
        const std::string&       label,
        EnumT                    value,
        std::vector<std::string> items,
        bool                     restartAccumulation
    ) {
        return addEnum(path, label, std::to_underlying(value), items, restartAccumulation);
    }

    void bindInt  (const ParameterPath& path, int*   ptr);
    void bindFloat(const ParameterPath& path, float* ptr);
    void bindBool (const ParameterPath& path, bool*  ptr);
    void bindEnum (const ParameterPath& path, int*   ptr);
    void bindInt  (const ParameterPath& path, std::function<void(int)>   callback);
    void bindFloat(const ParameterPath& path, std::function<void(float)> callback);
    void bindBool (const ParameterPath& path, std::function<void(bool)>  callback);
    void bindEnum (const ParameterPath& path, std::function<void(int)>   callback);
    
    int&   getInt  (const ParameterPath& path);
    float& getFloat(const ParameterPath& path);
    bool&  getBool (const ParameterPath& path);
    int&   getEnum (const ParameterPath& path);
    template <typename EnumT>
    EnumT  getEnum (const ParameterPath& path) {
        return static_cast<EnumT>(getEnum(path));
    }
    
    void setInt       (const ParameterPath& path, int   value);
    void setFloat     (const ParameterPath& path, float value);
    void setBool      (const ParameterPath& path, bool  value);
    void setEnum      (const ParameterPath& path, int   value);
    void setEnumByName(const ParameterPath& path, const std::string& name);
    template <typename EnumT>
    void setEnum      (const ParameterPath& path, EnumT value) {
        setEnum(path, std::to_underlying(value));
    }

    const std::vector<std::unique_ptr<ParameterBase>>& getParameterList() const { return parameters; }
    const std::unordered_map<std::string, std::string>& getNodeLabels() const { return nodeLabels; }

private:
    std::vector<std::unique_ptr<ParameterBase>> parameters;
    std::unordered_map<std::string, ParameterBase*> index;
    std::unordered_map<std::string, std::string> nodeLabels;

    void serializeParameterPath(std::ofstream& file, const ParameterPath& prefix, int depth = 0);

    template <typename T>
    T& getParameter(const ParameterPath& path) {
        auto it = index.find(path.generic_string());
        if (it == index.end())
            throw std::runtime_error("Parameter not found: " + path.generic_string());
        auto parameter = dynamic_cast<T*>(it->second);
        if (!parameter)
            throw std::runtime_error("Parameter type mismatch: " + path.generic_string());
        return *parameter;
    }
};
