#include "parameter_handler.hpp"

#include "imgui/imgui.h"

#include <stdexcept>

IntParam::IntParam(
    const std::string &id_,
    const std::string &label_,
    int value_,
    int minValue_,
    int maxValue_,
    int step_,
    bool restart_,
    const std::string &group_
) : value(value_), minValue(minValue_), maxValue(maxValue_), step(step_) {
    id = id_;
    label = label_;
    group = group_;
    restart = restart_;
}

bool IntParam::draw() {
    ImGui::PushID(id.c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed = ImGui::DragInt("##value", &value, static_cast<float>(step), minValue, maxValue);
    ImGui::PopID();
    return changed;
}

FloatParam::FloatParam(
    const std::string &id_,
    const std::string &label_,
    float value_,
    float minValue_,
    float maxValue_,
    float step_,
    bool restart_,
    const std::string &group_
) : value(value_), minValue(minValue_), maxValue(maxValue_), step(step_) {
    id = id_;
    label = label_;
    group = group_;
    restart = restart_;
}

bool FloatParam::draw() {
    ImGui::PushID(id.c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed = ImGui::DragFloat("##value", &value, step, minValue, maxValue);
    ImGui::PopID();
    return changed;
}

BoolParam::BoolParam(
    const std::string &id_,
    const std::string &label_,
    bool value_,
    bool restart_,
    const std::string &group_
) : value(value_) {
    id = id_;
    label = label_;
    group = group_;
    restart = restart_;
}

bool BoolParam::draw() {
    ImGui::PushID(id.c_str());
    ImGui::Text("%s", label.c_str());
    bool changed = ImGui::Checkbox("##value", &value);
    ImGui::PopID();
    return changed;
}

EnumParam::EnumParam(
    const std::string &id_,
    const std::string &label_,
    int value_,
    std::vector<std::string> items_,
    bool restart_,
    const std::string &group_
) : value(value_), items(std::move(items_)) {
    id = id_;
    label = label_;
    group = group_;
    restart = restart_;
}

bool EnumParam::draw() {
    itemsName.clear();
    itemsName.reserve(items.size());
    for (const std::string &item : items)
        itemsName.push_back(item.c_str());
    ImGui::PushID(id.c_str());
    ImGui::Text("%s", label.c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool changed = ImGui::Combo("##value", &value, itemsName.data(), static_cast<int>(itemsName.size()));
    ImGui::PopID();
    return changed;
}

IntParam &ParameterHandler::addInt(
    const std::string &id,
    const std::string &label,
    int value,
    int minValue,
    int maxValue,
    int step,
    bool restart,
    const std::string &group
) {
    auto param = std::make_unique<IntParam>(id, label, value, minValue, maxValue, step, restart, group);
    params.push_back(std::move(param));
    index[id] = params.back().get();
    return static_cast<IntParam&>(*params.back());
}

FloatParam &ParameterHandler::addFloat(
    const std::string &id,
    const std::string &label,
    float value,
    float minValue,
    float maxValue,
    float step,
    bool restart,
    const std::string &group
) {
    auto param = std::make_unique<FloatParam>(id, label, value, minValue, maxValue, step, restart, group);
    params.push_back(std::move(param));
    index[id] = params.back().get();
    return static_cast<FloatParam&>(*params.back());
}

BoolParam &ParameterHandler::addBool(
    const std::string &id,
    const std::string &label,
    bool value,
    bool restart,
    const std::string &group
) {
    auto param = std::make_unique<BoolParam>(id, label, value, restart, group);
    params.push_back(std::move(param));
    index[id] = params.back().get();
    return static_cast<BoolParam&>(*params.back());
}

EnumParam &ParameterHandler::addEnum(
    const std::string &id,
    const std::string &label,
    int value,
    std::vector<std::string> items,
    bool restart,
    const std::string &group
) {
    auto param = std::make_unique<EnumParam>(id, label, value, std::move(items), restart, group);
    params.push_back(std::move(param));
    index[id] = params.back().get();
    return static_cast<EnumParam&>(*params.back());
}

bool ParameterHandler::drawGroup(const std::string &group, bool &restartRequested) {
    bool changed = false;
    for (const auto &param : params) {
        if (param->group != group) continue;
        if (param->draw()) {
            changed = true;
            if (param->restart)
                restartRequested = true;
        }
    }
    return changed;
}

int &ParameterHandler::getInt(const std::string &id) {
    return getParam<IntParam>(id).get();
}

float &ParameterHandler::getFloat(const std::string &id) {
    return getParam<FloatParam>(id).get();
}

bool &ParameterHandler::getBool(const std::string &id) {
    return getParam<BoolParam>(id).get();
}

void ParameterHandler::setInt(const std::string &id, int value) {
    getParam<IntParam>(id).get() = value;
}

void ParameterHandler::setFloat(const std::string &id, float value) {
    getParam<FloatParam>(id).get() = value;
}

void ParameterHandler::setBool(const std::string &id, bool value) {
    getParam<BoolParam>(id).get() = value;
}
