#include "parameter_handler.hpp"

#include <algorithm>
#include <fstream>
#include <limits>

#include "nlohmann/json.hpp"

using json = nlohmann::json;

void ParameterHandler::saveDocumentation(std::filesystem::path path) {
    std::ofstream file;
    file.open(path);
    file.clear();

    file << "# Parameters" << std::endl;
    file << std::endl;
    file << "> [!NOTE]  " << std::endl;
    file << "> Parameters are defined in `assets/parameters/parameters.json` as a hierarchical JSON file.  " << std::endl;
    file << "> Leaf entries require a `\"default\"` field, from which the parameter type is inferred.  " << std::endl;
    file << "> Nested objects without `\"default\"` are display groups and may carry an optional `\"label\"`.  " << std::endl;

    serializeParameterPath(file, "");

    file.close();
}

void ParameterHandler::serializeParameterPath(std::ofstream& file, const ParameterPath& prefix, int depth) {
    for (const auto& parameter : parameters) {
        if (parameter->path.parent_path() != prefix) continue;
        file << parameter->print() << std::endl;
    }

    std::vector<std::string> seen;
    for (const auto& parameter : parameters) {
        auto rel = parameter->path.lexically_relative(prefix);
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

static void parseNode(const json& obj, ParameterHandler& handler, const std::string& path) {
    if (obj.contains("default")) {
        std::string label = obj.at("label").get<std::string>();
        bool restart = obj.value("restart_accumulation", false);
        const auto& def = obj.at("default");

        if (def.is_boolean()) {
            handler.addBool(path, label, def.get<bool>(), restart);
        } else if (def.is_number_integer()) {
            handler.addInt(path, label, def.get<int>(),
                obj.value("min", INT_MIN),
                obj.value("max", INT_MAX),
                obj.value("step", 1),
                restart);
        } else if (def.is_number_float()) {
            handler.addFloat(path, label, def.get<float>(),
                obj.value("min", std::numeric_limits<float>::lowest()),
                obj.value("max", std::numeric_limits<float>::max()),
                obj.value("step", 1e-3f),
                restart);
        } else if (def.is_string() && obj.contains("items")) {
            std::string defName = def.get<std::string>();
            std::vector<std::string> items = obj.at("items").get<std::vector<std::string>>();
            int defIdx = 0;
            for (size_t i = 0; i < items.size(); i++)
                if (items[i] == defName) { defIdx = static_cast<int>(i); break; }
            handler.addEnum(path, label, defIdx, std::move(items), restart);
        }
    } else {
        if (obj.contains("label"))
            handler.setNodeLabel(path, obj.at("label").get<std::string>());
        for (const auto& [key, val] : obj.items()) {
            if (key == "label") continue;
            parseNode(val, handler, path + "/" + key);
        }
    }
}

ParameterHandler ParameterHandler::fromFile(std::filesystem::path path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error(std::format("Cannot open parameter file [{}]", path.string()));

    ParameterHandler parameters;
    json root = json::parse(f, nullptr, true, true);

    for (const auto& [key, val] : root.items())
        parseNode(val, parameters, key);

    return parameters;
}

void ParameterHandler::setNodeLabel(const ParameterPath& path, const std::string& label) {
    nodeLabels[path.generic_string()] = label;
}

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
    return std::format("| `{}` | {} | Integer | {} | {} ... {} | {} |", path.c_str(), label, defaultValue, minValue, maxValue, restartAccumulation ? "✓" : "-");
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
    return std::format("| `{}` | {} | Float | {} | {:.1f} ... {:.1f} | {} |", path.c_str(), label, defaultValue, minValue, maxValue, restartAccumulation ? "✓" : "-");
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
    return std::format("| `{}` | {} | Boolean | {} | - | {} |", path.c_str(), label, defaultValue, restartAccumulation ? "✓" : "-");
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
    return std::format("| `{}` | {} | Enumeration | `{}` | {} | {} |", path.c_str(), label, items[defaultValue], list, restartAccumulation ? "✓" : "-");
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

void ParameterHandler::bindInt(const ParameterPath& path, int* ptr) {
    auto& parameter = getParameter<IntParameter>(path);
    parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
    parameter.onSync();
}

void ParameterHandler::bindFloat(const ParameterPath& path, float* ptr) {
    auto& parameter = getParameter<FloatParameter>(path);
    parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
    parameter.onSync();
}

void ParameterHandler::bindBool(const ParameterPath& path, bool* ptr) {
    auto& parameter = getParameter<BoolParameter>(path);
    parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
    parameter.onSync();
}

void ParameterHandler::bindEnum(const ParameterPath& path, int* ptr) {
    auto& parameter = getParameter<EnumParameter>(path);
    parameter.onSync = [ptr, &parameter]() { *ptr = parameter.get(); };
    parameter.onSync();
}

void ParameterHandler::bindInt(const ParameterPath& path, std::function<void(int)> callback) {
    auto& parameter = getParameter<IntParameter>(path);
    parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
    parameter.onSync();
}

void ParameterHandler::bindFloat(const ParameterPath& path, std::function<void(float)> callback) {
    auto& parameter = getParameter<FloatParameter>(path);
    parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
    parameter.onSync();
}

void ParameterHandler::bindBool(const ParameterPath& path, std::function<void(bool)> callback) {
    auto& parameter = getParameter<BoolParameter>(path);
    parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
    parameter.onSync();
}

void ParameterHandler::bindEnum(const ParameterPath& path, std::function<void(int)> callback) {
    auto& parameter = getParameter<EnumParameter>(path);
    parameter.onSync = [callback = std::move(callback), &parameter]() { callback(parameter.get()); };
    parameter.onSync();
}

int& ParameterHandler::getInt(const ParameterPath& path) {
    return getParameter<IntParameter>(path).get();
}

float& ParameterHandler::getFloat(const ParameterPath& path) {
    return getParameter<FloatParameter>(path).get();
}

bool& ParameterHandler::getBool(const ParameterPath& path) {
    return getParameter<BoolParameter>(path).get();
}

int& ParameterHandler::getEnum(const ParameterPath& path) {
    return getParameter<EnumParameter>(path).get();
}

void ParameterHandler::setInt(const ParameterPath& path, int value) {
    auto& parameter = getParameter<IntParameter>(path);
    parameter.get() = value;
    if (parameter.onSync) parameter.onSync();
}

void ParameterHandler::setFloat(const ParameterPath& path, float value) {
    auto& parameter = getParameter<FloatParameter>(path);
    parameter.get() = value;
    if (parameter.onSync) parameter.onSync();
}

void ParameterHandler::setBool(const ParameterPath& path, bool value) {
    auto& parameter = getParameter<BoolParameter>(path);
    parameter.get() = value;
    if (parameter.onSync) parameter.onSync();
}

void ParameterHandler::setEnum(const ParameterPath& path, int value) {
    auto& parameter = getParameter<EnumParameter>(path);
    parameter.get() = value;
    if (parameter.onSync) parameter.onSync();
}

// TODO: should probably not crash and only log the issue
void ParameterHandler::setEnumByName(const ParameterPath& path, const std::string& name) {
    auto& parameter = getParameter<EnumParameter>(path);
    if (!parameter.setByName(name))
        throw std::runtime_error("Unknown enum value '" + name + "' for parameter: " + path.generic_string());
    if (parameter.onSync) parameter.onSync();
}
