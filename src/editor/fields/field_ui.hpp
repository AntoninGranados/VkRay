#pragma once

#include <functional>
#include <string>
#include <vector>

#include "core/fields/field.hpp"

namespace ui {

struct FieldGroup {
    Field* field = nullptr;
    FieldPath id;
    bool showHeader = true;
    std::function<bool()> disabledWhen;
    std::vector<FieldGroup> children;
};

std::vector<FieldGroup> buildFieldGroups(const std::vector<Field*>& fields, const FieldPath& prefix = "");

using DrawFieldLeaf = std::function<bool(Field&, const std::string&)>;
using FieldGroupLabel = std::function<std::string(const FieldPath&)>;

bool drawFieldGroups(std::vector<FieldGroup>& groups, const std::string& widgetId, const DrawFieldLeaf& drawLeaf, const FieldGroupLabel& label = {});

bool drawField(Field& field, const std::string& widgetId);
bool drawGroupedFields(std::vector<Field>& fields, const std::string& widgetId);

}
