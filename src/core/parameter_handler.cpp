#include "parameter_handler.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>

IntParam::IntParam(
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
    restart = restart_;
}

std::string IntParam::print() {
    return std::format("| `{}` | {} | Integer | {} | {} ... {} | {} |", path.c_str(), label, defaultValue, minValue, maxValue, restart ? "✓" : "-");
}

FloatParam::FloatParam(
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
    restart = restart_;
}

// TODO: fix the hardcoded 1 decimal precision
std::string FloatParam::print() {
    return std::format("| `{}` | {} | Float | {} | {:.1f} ... {:.1f} | {} |", path.c_str(), label, defaultValue, minValue, maxValue, restart ? "✓" : "-");
}

BoolParam::BoolParam(
    const ParameterPath& path_,
    const std::string&   label_,
    bool                 value_,
    bool                 restart_
) : value(value_), defaultValue(value_) {
    path    = path_;
    label   = label_;
    restart = restart_;
}

std::string BoolParam::print() {
    return std::format("| `{}` | {} | Boolean | {} | - | {} |", path.c_str(), label, defaultValue, restart ? "✓" : "-");
}

EnumParam::EnumParam(
    const ParameterPath&     path_,
    const std::string&       label_,
    int                      value_,
    std::vector<std::string> items_,
    bool                     restart_
) : value(value_), defaultValue(value_), items(std::move(items_)) {
    path    = path_;
    label   = label_;
    restart = restart_;
}

std::string EnumParam::print() {
    std::string list = "";
    for (const auto& item : items) {
        if (list.empty()) list = std::format("`{}`", item);
        else list = std::format("{} • `{}`", list, item);
    }
    return std::format("| `{}` | {} | Enumeration | `{}` | {} | {} |", path.c_str(), label, items[defaultValue], list, restart ? "✓" : "-");
}

IntParam& ParameterHandler::addInt(
    const ParameterPath& path,
    const std::string&   label,
    int                  value,
    int                  minValue,
    int                  maxValue,
    int                  step,
    bool                 restart
) {
    auto param = std::make_unique<IntParam>(path, label, value, minValue, maxValue, step, restart);
    index[path.generic_string()] = param.get();
    params.push_back(std::move(param));
    return static_cast<IntParam&>(*params.back());
}

FloatParam& ParameterHandler::addFloat(
    const ParameterPath& path,
    const std::string&   label,
    float                value,
    float                minValue,
    float                maxValue,
    float                step,
    bool                 restart
) {
    auto param = std::make_unique<FloatParam>(path, label, value, minValue, maxValue, step, restart);
    index[path.generic_string()] = param.get();
    params.push_back(std::move(param));
    return static_cast<FloatParam&>(*params.back());
}

BoolParam& ParameterHandler::addBool(
    const ParameterPath& path,
    const std::string&   label,
    bool                 value,
    bool                 restart
) {
    auto param = std::make_unique<BoolParam>(path, label, value, restart);
    index[path.generic_string()] = param.get();
    params.push_back(std::move(param));
    return static_cast<BoolParam&>(*params.back());
}

void ParameterHandler::serializeParameterPath(std::ofstream& file, const ParameterPath& prefix, int depth) {
    for (const auto& param : params) {
        if (param->path.parent_path() != prefix) continue;
        file << param->print() << std::endl;
    }

    std::vector<std::string> seen;
    for (const auto& param : params) {
        auto rel = param->path.lexically_relative(prefix);
        if (rel.empty()) continue;
        auto it = rel.begin();
        std::string seg = it->string();
        if (seg == "..") continue;
        it++;
        if (it == rel.end()) continue;
        if (std::find(seen.begin(), seen.end(), seg) != seen.end()) continue;
        seen.push_back(seg);

        auto labelIt = nodeLabels.find((prefix / seg).generic_string());
        const std::string& displayLabel = labelIt != nodeLabels.end() ? labelIt->second : seg;
        file << std::endl << std::string(depth+2, '#') << ' ' << displayLabel << std::endl;
        file << "| Path | Label | Type | Default | Constraints | Restart |" << std::endl;
        file << "|------|-------|------|---------|-------------|---------|" << std::endl;

        serializeParameterPath(file, prefix / seg, depth+1);
    }
}

void ParameterHandler::saveDocumentation(std::filesystem::path path) {
    std::ofstream file;
    file.open(path);
    file.clear();

    file << "# Parameters" << std::endl;

    serializeParameterPath(file, "");

    file.close();
}

void ParameterHandler::setLabel(const ParameterPath& path, const std::string& label) {
    nodeLabels[path.generic_string()] = label;
}

void ParameterHandler::bindInt(const ParameterPath& path, int* ptr) {
    auto& param = getParam<IntParam>(path);
    param.onSync = [ptr, &param]() { *ptr = param.get(); };
    param.onSync();
}

void ParameterHandler::bindFloat(const ParameterPath& path, float* ptr) {
    auto& param = getParam<FloatParam>(path);
    param.onSync = [ptr, &param]() { *ptr = param.get(); };
    param.onSync();
}

void ParameterHandler::bindBool(const ParameterPath& path, bool* ptr) {
    auto& param = getParam<BoolParam>(path);
    param.onSync = [ptr, &param]() { *ptr = param.get(); };
    param.onSync();
}

void ParameterHandler::bindEnum(const ParameterPath& path, int* ptr) {
    auto& param = getParam<EnumParam>(path);
    param.onSync = [ptr, &param]() { *ptr = param.get(); };
    param.onSync();
}

void ParameterHandler::bindInt(const ParameterPath& path, std::function<void(int)> callback) {
    auto& param = getParam<IntParam>(path);
    param.onSync = [callback = std::move(callback), &param]() { callback(param.get()); };
    param.onSync();
}

void ParameterHandler::bindFloat(const ParameterPath& path, std::function<void(float)> callback) {
    auto& param = getParam<FloatParam>(path);
    param.onSync = [callback = std::move(callback), &param]() { callback(param.get()); };
    param.onSync();
}

void ParameterHandler::bindBool(const ParameterPath& path, std::function<void(bool)> callback) {
    auto& param = getParam<BoolParam>(path);
    param.onSync = [callback = std::move(callback), &param]() { callback(param.get()); };
    param.onSync();
}

void ParameterHandler::bindEnum(const ParameterPath& path, std::function<void(int)> callback) {
    auto& param = getParam<EnumParam>(path);
    param.onSync = [callback = std::move(callback), &param]() { callback(param.get()); };
    param.onSync();
}

int& ParameterHandler::getInt(const ParameterPath& path) {
    return getParam<IntParam>(path).get();
}

float& ParameterHandler::getFloat(const ParameterPath& path) {
    return getParam<FloatParam>(path).get();
}

bool& ParameterHandler::getBool(const ParameterPath& path) {
    return getParam<BoolParam>(path).get();
}

void ParameterHandler::setInt(const ParameterPath& path, int value) {
    auto& param = getParam<IntParam>(path);
    param.get() = value;
    if (param.onSync) param.onSync();
}

void ParameterHandler::setFloat(const ParameterPath& path, float value) {
    auto& param = getParam<FloatParam>(path);
    param.get() = value;
    if (param.onSync) param.onSync();
}

void ParameterHandler::setBool(const ParameterPath& path, bool value) {
    auto& param = getParam<BoolParam>(path);
    param.get() = value;
    if (param.onSync) param.onSync();
}

// TODO: should probably not crash and only log the issue
void ParameterHandler::setEnumByName(const ParameterPath& path, const std::string& name) {
    auto& param = getParam<EnumParam>(path);
    if (!param.setByName(name))
        throw std::runtime_error("Unknown enum value '" + name + "' for parameter: " + path.generic_string());
    if (param.onSync) param.onSync();
}

void ParameterHandler::resetAll() {
    for (auto& p : params) p->reset();
}
