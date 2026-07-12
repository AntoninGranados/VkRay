#pragma once

#include <filesystem>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using ParameterPath = std::filesystem::path;

class ParamBase {
public:
    virtual ~ParamBase() = default;
    virtual void reset() = 0;
    virtual std::string print() = 0;

    ParameterPath         path;
    std::string           label;
    bool                  restart = false;
    std::function<void()> onSync;
};

class IntParam : public ParamBase {
public:
    IntParam(
        const ParameterPath& path_,
        const std::string&   label_,
        int                  value_,
        int                  minValue_,
        int                  maxValue_,
        int                  step_,
        bool                 restart_
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

class FloatParam : public ParamBase {
public:
    FloatParam(
        const ParameterPath& path_,
        const std::string&   label_,
        float                value_,
        float                minValue_,
        float                maxValue_,
        float                step_,
        bool                 restart_
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

class BoolParam : public ParamBase {
public:
    BoolParam(
        const ParameterPath& path_,
        const std::string&   label_,
        bool                 value_,
        bool                 restart_
    );
    std::string print() override;
    void  reset() override { value = defaultValue; if (onSync) onSync(); }
    bool& get() { return value; }

private:
    bool value        = false;
    bool defaultValue = false;
};

class EnumParam : public ParamBase {
public:
    EnumParam(
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
    int                      value        = 0;
    int                      defaultValue = 0;
    std::vector<std::string> items;
};

// TODO: add parameter descriptions
// TODO: define the parameters in a separate file so we can "preprocess" (e.g. build the doc) at build time
class ParameterHandler {
public:

    IntParam& addInt(
        const ParameterPath& path,
        const std::string&   label,
        int                  value,
        int                  minValue,
        int                  maxValue,
        int                  step,
        bool                 restart
    );
    FloatParam& addFloat(
        const ParameterPath& path,
        const std::string&   label,
        float                value,
        float                minValue,
        float                maxValue,
        float                step,
        bool                 restart
    );
    BoolParam& addBool(
        const ParameterPath& path,
        const std::string&   label,
        bool                 value,
        bool                 restart
    );
    template <typename EnumT>
    EnumParam& addEnum(
        const ParameterPath&     path,
        const std::string&       label,
        EnumT                    value,
        std::vector<std::string> items,
        bool                     restart
    ) {
        auto param = std::make_unique<EnumParam>(path, label, std::to_underlying(value), std::move(items), restart);
        index[path.generic_string()] = param.get();
        params.push_back(std::move(param));
        return static_cast<EnumParam&>(*params.back());
    }

    void saveDocumentation(std::filesystem::path path = "./docs/parameters.md");
    void setLabel (const ParameterPath& path, const std::string& label);
    void resetAll();

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
    void setInt       (const ParameterPath& path, int   value);
    void setFloat     (const ParameterPath& path, float value);
    void setBool      (const ParameterPath& path, bool  value);
    void setEnumByName(const ParameterPath& path, const std::string& name);

    template <typename EnumT>
    EnumT getEnum(const ParameterPath& path) {
        return static_cast<EnumT>(getParam<EnumParam>(path).get());
    }
    template <typename EnumT>
    void setEnum(const ParameterPath& path, EnumT value) {
        auto& param = getParam<EnumParam>(path);
        param.get() = std::to_underlying(value);
        if (param.onSync) param.onSync();
    }

    const std::vector<std::unique_ptr<ParamBase>>&     getParamList()   const { return params; }
    const std::unordered_map<std::string, std::string>& getNodeLabels() const { return nodeLabels; }

private:
    std::vector<std::unique_ptr<ParamBase>>      params;
    std::unordered_map<std::string, ParamBase*>  index;
    std::unordered_map<std::string, std::string> nodeLabels;

    void serializeParameterPath(std::ofstream& file, const ParameterPath& prefix, int depth = 0);

    template <typename T>
    T& getParam(const ParameterPath& path) {
        auto it = index.find(path.generic_string());
        if (it == index.end())
            throw std::runtime_error("Parameter not found: " + path.generic_string());
        auto param = dynamic_cast<T*>(it->second);
        if (!param)
            throw std::runtime_error("Parameter type mismatch: " + path.generic_string());
        return *param;
    }
};
