#pragma once

#include <string>

#include "core/fields/field.hpp"

namespace ui {

namespace {

struct FieldItem {
    Field* field = nullptr;
    std::vector<FieldItem> children;
};

}

bool drawField(Field& field, const std::string& widgetId);
bool drawGroupedFields(std::vector<Field&>& fields, const std::string& widgetId);

}
