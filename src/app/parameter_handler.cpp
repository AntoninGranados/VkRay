#include "app/parameter_handler.hpp"

#include "imgui/imgui.h"

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

bool IntParam::draw() {
    ImGui::PushID(path.generic_string().c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed = ImGui::DragInt("##value", &value, static_cast<float>(step), minValue, maxValue);
    ImGui::PopID();
    if (changed && onSync) onSync();
    return changed;
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

bool FloatParam::draw() {
    ImGui::PushID(path.generic_string().c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed = ImGui::DragFloat("##value", &value, step, minValue, maxValue);
    ImGui::PopID();
    if (changed && onSync) onSync();
    return changed;
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

bool BoolParam::draw() {
    ImGui::PushID(path.generic_string().c_str());
    bool changed = ImGui::Checkbox(label.c_str(), &value);
    ImGui::PopID();
    if (changed && onSync) onSync();
    return changed;
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

bool EnumParam::draw() {
    itemsName.clear();
    itemsName.reserve(items.size());
    for (const std::string& item : items)
        itemsName.push_back(item.c_str());
    ImGui::PushID(path.generic_string().c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed = ImGui::Combo("##value", &value, itemsName.data(), static_cast<int>(itemsName.size()));
    ImGui::PopID();
    if (changed && onSync) onSync();
    return changed;
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

bool ParameterHandler::drawNode(const ParameterPath& prefix, bool& restartRequested) {
    bool changed = false;

    for (const auto& param : params) {
        if (param->path.parent_path() != prefix) continue;
        if (param->draw()) {
            changed = true;
            if (param->restart) restartRequested = true;
        }
    }

    std::vector<std::string> seen;
    for (const auto& param : params) {
        auto rel = param->path.lexically_relative(prefix);
        if (rel.empty()) continue;
        auto it = rel.begin();
        std::string seg = it->string();
        if (seg == "..") continue;
        ++it;
        if (it == rel.end()) continue;
        if (std::find(seen.begin(), seen.end(), seg) != seen.end()) continue;
        seen.push_back(seg);

        auto labelIt = nodeLabels.find((prefix / seg).generic_string());
        const std::string& displayLabel = labelIt != nodeLabels.end() ? labelIt->second : seg;
        if (ImGui::CollapsingHeader(displayLabel.c_str())) {
            changed |= drawNode(prefix / seg, restartRequested);
        }
    }

    return changed;
}

bool ParameterHandler::drawGroup(const ParameterPath& root, bool& restartRequested) {
    return drawNode(root, restartRequested);
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

void ParameterHandler::bind(const ParameterPath& path, std::function<void()> callback) {
    auto it = index.find(path.generic_string());
    if (it == index.end())
        throw std::runtime_error("Parameter not found: " + path.generic_string());
    it->second->onSync = std::move(callback);
    it->second->onSync();
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

void ParameterHandler::resetAll() {
    for (auto& p : params) p->reset();
}
