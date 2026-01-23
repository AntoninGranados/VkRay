#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

class ParamBase {
public:
    virtual ~ParamBase() = default;
    virtual bool draw() = 0;

    std::string id;
    std::string label;
    std::string group;
    bool restart = false;
};

class IntParam : public ParamBase {
public:
    IntParam(
        const std::string &id_,
        const std::string &label_,
        int value_,
        int minValue_,
        int maxValue_,
        int step_,
        bool restart_,
        const std::string &group_
    );
    bool draw() override;
    int &get() { return value; }

private:
    int value = 0;
    int minValue = 0;
    int maxValue = 0;
    int step = 1;
};

class FloatParam : public ParamBase {
public:
    FloatParam(
        const std::string &id_,
        const std::string &label_,
        float value_,
        float minValue_,
        float maxValue_,
        float step_,
        bool restart_,
        const std::string &group_
    );
    bool draw() override;
    float &get() { return value; }

private:
    float value = 0.0f;
    float minValue = 0.0f;
    float maxValue = 0.0f;
    float step = 0.1f;
};

class BoolParam : public ParamBase {
public:
    BoolParam(
        const std::string &id_,
        const std::string &label_,
        bool value_,
        bool restart_,
        const std::string &group_
    );
    bool draw() override;
    bool &get() { return value; }

private:
    bool value = false;
};

class EnumParam : public ParamBase {
public:
    EnumParam(
        const std::string &id_,
        const std::string &label_,
        int value_,
        std::vector<std::string> items_,
        bool restart_,
        const std::string &group_
    );
    bool draw() override;
    int &get() { return value; }

private:
    int value = 0;
    std::vector<std::string> items;
    std::vector<const char*> itemsName;
};

class ParameterSystem {
public:
    IntParam &addInt(
        const std::string &id,
        const std::string &label,
        int value,
        int minValue,
        int maxValue,
        int step,
        bool restart,
        const std::string &group
    );
    FloatParam &addFloat(
        const std::string &id,
        const std::string &label,
        float value,
        float minValue,
        float maxValue,
        float step,
        bool restart,
        const std::string &group
    );
    BoolParam &addBool(
        const std::string &id,
        const std::string &label,
        bool value,
        bool restart,
        const std::string &group
    );
    EnumParam &addEnum(
        const std::string &id,
        const std::string &label,
        int value,
        std::vector<std::string> items,
        bool restart,
        const std::string &group
    );

    bool drawGroup(const std::string &group, bool &restartRequested);
    int &getInt(const std::string &id);
    float &getFloat(const std::string &id);
    bool &getBool(const std::string &id);
    void setInt(const std::string &id, int value);
    void setFloat(const std::string &id, float value);
    void setBool(const std::string &id, bool value);
    template <typename EnumT>
    EnumT getEnum(const std::string &id) {
        return static_cast<EnumT>(getParam<EnumParam>(id).get());
    }
    template <typename EnumT>
    void setEnum(const std::string &id, EnumT value) {
        getParam<EnumParam>(id).get() = static_cast<int>(value);
    }

private:
    std::vector<std::unique_ptr<ParamBase> > params;
    std::unordered_map<std::string, ParamBase*> index;

    template <typename T>
    T &getParam(const std::string &id) {
        auto it = index.find(id);
        if (it == index.end())
            throw std::runtime_error("Parameter not found: " + id);
        auto param = dynamic_cast<T*>(it->second);
        if (!param)
            throw std::runtime_error("Parameter type mismatch: " + id);
        return *param;
    }
};
