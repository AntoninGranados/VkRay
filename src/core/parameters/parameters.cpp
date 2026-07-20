#include "parameters.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::ordered_json;

void ParameterHandler::resetAll() {
    for (auto& p : parameters) p->reset();
}

IntParameter::IntParameter(
    const ParameterPath& path_,
    const std::string&   label_,
    int                  value_,
    int                  minValue_,
    int                  maxValue_,
    int                  step_,
    bool                 restart_
) : value(value_), defaultValue(value_), minValue(minValue_), maxValue(maxValue_), step(step_) {
    path    = path_;
    label   = label_;
    restartAccumulation = restart_;
}

std::string IntParameter::print() {
    return std::format("| `{}` | {} | {} | Integer | {} | {} ... {} | {} |", path.c_str(), label, description.value_or("-"), defaultValue, minValue, maxValue, restartAccumulation ? "✓" : "-");
}

FloatParameter::FloatParameter(
    const ParameterPath& path_,
    const std::string&   label_,
    float                value_,
    float                minValue_,
    float                maxValue_,
    float                step_,
    bool                 restart_
) : value(value_), defaultValue(value_), minValue(minValue_), maxValue(maxValue_), step(step_) {
    path    = path_;
    label   = label_;
    restartAccumulation = restart_;
}

// TODO: fix the hardcoded 1 decimal precision
std::string FloatParameter::print() {
    return std::format("| `{}` | {} | {} | Float | {} | {:.1f} ... {:.1f} | {} |", path.c_str(), label, description.value_or("-"), defaultValue, minValue, maxValue, restartAccumulation ? "✓" : "-");
}

BoolParameter::BoolParameter(
    const ParameterPath& path_,
    const std::string&   label_,
    bool                 value_,
    bool                 restart_
) : value(value_), defaultValue(value_) {
    path    = path_;
    label   = label_;
    restartAccumulation = restart_;
}

std::string BoolParameter::print() {
    return std::format("| `{}` | {} | {} | Boolean | {} | - | {} |", path.c_str(), label, description.value_or("-"), defaultValue, restartAccumulation ? "✓" : "-");
}

EnumParameter::EnumParameter(
    const ParameterPath&     path_,
    const std::string&       label_,
    int                      value_,
    std::vector<std::string> items_,
    bool                     restart_
) : value(value_), defaultValue(value_), items(std::move(items_)) {
    path    = path_;
    label   = label_;
    restartAccumulation = restart_;
}

std::string EnumParameter::print() {
    std::string list = "";
    for (const auto& item : items) {
        if (list.empty()) list = std::format("`{}`", item);
        else list = std::format("{} • `{}`", list, item);
    }
    return std::format("| `{}` | {} | {} | Enumeration | `{}` | {} | {} |", path.c_str(), label, description.value_or("-"), items[defaultValue], list, restartAccumulation ? "✓" : "-");
}

PathParameter::PathParameter(
    const ParameterPath&       path_,
    const std::string&         label_,
    std::filesystem::path      value_,
    std::vector<PathExtension> extensions_,
    bool                       restart_
) : value(value_), defaultValue(value_), extensions(std::move(extensions_)) {
    path = path_;
    label = label_;
    restartAccumulation = restart_;
}

std::string PathParameter::print() {
    std::string exts;
    for (const auto& e : extensions) {
        if (!exts.empty()) exts += ", ";
        exts += e.displayName() + " (." + e.ext + ")";
    }
    return std::format("| `{}` | {} | {} | Path | `{}` | {} | {} |",
        path.c_str(), label, description.value_or("-"),
        defaultValue.string(), exts.empty() ? "-" : exts,
        restartAccumulation ? "✓" : "-");
}

IntParameter& ParameterHandler::addInt(
    const ParameterPath& path,
    const std::string&   label,
    int                  value,
    int                  minValue,
    int                  maxValue,
    int                  step,
    bool                 restartAccumulation
) {
    auto parameter = std::make_unique<IntParameter>(path, label, value, minValue, maxValue, step, restartAccumulation);
    index[path.generic_string()] = parameter.get();
    parameters.push_back(std::move(parameter));
    return static_cast<IntParameter&>(*parameters.back());
}

FloatParameter& ParameterHandler::addFloat(
    const ParameterPath& path,
    const std::string&   label,
    float                value,
    float                minValue,
    float                maxValue,
    float                step,
    bool                 restartAccumulation
) {
    auto parameter = std::make_unique<FloatParameter>(path, label, value, minValue, maxValue, step, restartAccumulation);
    index[path.generic_string()] = parameter.get();
    parameters.push_back(std::move(parameter));
    return static_cast<FloatParameter&>(*parameters.back());
}

BoolParameter& ParameterHandler::addBool(
    const ParameterPath& path,
    const std::string&   label,
    bool                 value,
    bool                 restartAccumulation
) {
    auto parameter = std::make_unique<BoolParameter>(path, label, value, restartAccumulation);
    index[path.generic_string()] = parameter.get();
    parameters.push_back(std::move(parameter));
    return static_cast<BoolParameter&>(*parameters.back());
}

EnumParameter& ParameterHandler::addEnum(
    const ParameterPath&     path,
    const std::string&       label,
    int                      value,
    std::vector<std::string> items,
    bool                     restartAccumulation
) {
    auto parameter = std::make_unique<EnumParameter>(path, label, value, std::move(items), restartAccumulation);
    index[path.generic_string()] = parameter.get();
    parameters.push_back(std::move(parameter));
    return static_cast<EnumParameter&>(*parameters.back());
}

PathParameter& ParameterHandler::addPath(
    const ParameterPath&       path,
    const std::string&         label,
    std::filesystem::path      value,
    std::vector<PathExtension> extensions,
    bool                       restartAccumulation
) {
    auto parameter = std::make_unique<PathParameter>(path, label, std::move(value), std::move(extensions), restartAccumulation);
    index[path.generic_string()] = parameter.get();
    parameters.push_back(std::move(parameter));
    return static_cast<PathParameter&>(*parameters.back());
}

// TODO: should probably not crash and only log the issue
void ParameterHandler::setEnumByName(const ParameterPath& path, const std::string& name) {
    auto& parameter = getParameter<EnumParameter>(path);
    if (!parameter.setByName(name))
        throw std::runtime_error("Unknown enum value '" + name + "' for parameter: " + path.generic_string());
    if (parameter.onSync) parameter.onSync();
}

// ===================== get =====================

template <>
bool ParameterHandler::get<bool>(const ParameterPath& path) {
    return getParameter<BoolParameter>(path).get();
}

template <>
int ParameterHandler::get<int>(const ParameterPath& path) {
    return getParameter<IntParameter>(path).get();
}

template <>
float ParameterHandler::get<float>(const ParameterPath& path) {
    return getParameter<FloatParameter>(path).get();
}

template <>
std::filesystem::path ParameterHandler::get<std::filesystem::path>(const ParameterPath& path) {
    return getParameter<PathParameter>(path).get();
}

// ===================== set =====================

template <>
void ParameterHandler::set<bool>(const ParameterPath& path, bool value) {
    auto& parameter = getParameter<BoolParameter>(path);
    parameter.get() = value;
    if (parameter.onSync) parameter.onSync();
}

template <>
void ParameterHandler::set<int>(const ParameterPath& path, int value) {
    auto& parameter = getParameter<IntParameter>(path);
    parameter.get() = value;
    if (parameter.onSync) parameter.onSync();
}

template <>
void ParameterHandler::set<float>(const ParameterPath& path, float value) {
    auto& parameter = getParameter<FloatParameter>(path);
    parameter.get() = value;
    if (parameter.onSync) parameter.onSync();
}

template <>
void ParameterHandler::set<std::filesystem::path>(const ParameterPath& path, std::filesystem::path value) {
    auto& parameter = getParameter<PathParameter>(path);
    parameter.get() = std::move(value);
    if (parameter.onSync) parameter.onSync();
}

// ===================== bind (pointer) =====================

template <>
void ParameterHandler::bind<bool>(const ParameterPath& path, bool* ptr) {
    auto& parameter = getParameter<BoolParameter>(path);
    parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
    parameter.onSync();
}

template <>
void ParameterHandler::bind<int>(const ParameterPath& path, int* ptr) {
    auto& parameter = getParameter<IntParameter>(path);
    parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
    parameter.onSync();
}

template <>
void ParameterHandler::bind<float>(const ParameterPath& path, float* ptr) {
    auto& parameter = getParameter<FloatParameter>(path);
    parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
    parameter.onSync();
}

template <>
void ParameterHandler::bind<std::filesystem::path>(const ParameterPath& path, std::filesystem::path* ptr) {
    auto& parameter = getParameter<PathParameter>(path);
    parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
    parameter.onSync();
}

// ===================== bind (callback) =====================

template <>
void ParameterHandler::bind<bool>(const ParameterPath& path, std::function<void(bool)> callback) {
    auto& parameter = getParameter<BoolParameter>(path);
    parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
    parameter.onSync();
}

template <>
void ParameterHandler::bind<int>(const ParameterPath& path, std::function<void(int)> callback) {
    auto& parameter = getParameter<IntParameter>(path);
    parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
    parameter.onSync();
}

template <>
void ParameterHandler::bind<float>(const ParameterPath& path, std::function<void(float)> callback) {
    auto& parameter = getParameter<FloatParameter>(path);
    parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
    parameter.onSync();
}

template <>
void ParameterHandler::bind<std::filesystem::path>(const ParameterPath& path, std::function<void(std::filesystem::path)> callback) {
    auto& parameter = getParameter<PathParameter>(path);
    parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
    parameter.onSync();
}
