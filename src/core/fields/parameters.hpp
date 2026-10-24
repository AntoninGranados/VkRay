#pragma once

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include <glm/glm.hpp>

#include "field.hpp"

class Parameter : public Field {
public:
    const std::optional<std::string>& getDescription() const { return description; }
    bool isRestartingAnimation() const { return restartAccumulation; }

    Parameter& setDescription(std::string d) {
        description = std::move(d);
        return *this;
    }

    std::string print() const;

    template <typename T>
    static Parameter make(const FieldPath& id, const std::string& label, const T& value, FieldMetadata metadata = {},
                          bool restartAccumulation = false) {
        Parameter p;
        static_cast<Field&>(p) = Field::make<T>(id, label, value, std::move(metadata));
        p.restartAccumulation = restartAccumulation;
        return p;
    }

private:
    std::optional<std::string> description;
    bool restartAccumulation = false;
};

class ParameterRegistry {
public:
    void setNodeLabel(const FieldPath& id, const std::string& label) { nodeLabels[id.string()] = label; }
    const std::vector<std::unique_ptr<Parameter>>& getAll() const { return parameters; }
    const std::unordered_map<std::string, std::string>& getNodeLabels() const { return nodeLabels; }
    void resetAll();

    template <typename T>
    Parameter& add(const FieldPath& id, const std::string& label, const T& value, FieldMetadata metadata = {},
                   bool restartAccumulation = false) {
        std::unique_ptr<Parameter> up =
            std::make_unique<Parameter>(Parameter::make<T>(id, label, value, std::move(metadata), restartAccumulation));
        index[up->getId().string()] = up.get();
        parameters.push_back(std::move(up));
        return *parameters.back();
    }

    template <typename T>
    T get(const FieldPath& id)
        requires(!std::is_enum_v<T>)
    {
        return getParam(id).get<T>();
    }
    template <typename T>
    T get(const FieldPath& id)
        requires(std::is_enum_v<T>)
    {
        return static_cast<T>(getParam(id).get<int>());
    }

    Parameter& getParam(const FieldPath& id);

private:
    std::vector<std::unique_ptr<Parameter>> parameters;
    std::unordered_map<std::string, Parameter*> index;
    std::unordered_map<std::string, std::string> nodeLabels;
};
