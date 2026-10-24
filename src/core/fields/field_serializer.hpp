#pragma once

#include "nlohmann/json.hpp"

#include "core/fields/field.hpp"
#include "utils/json_dsl.hpp"

nlohmann::ordered_json fieldValueToJson(const FieldValue& f);
FieldValue fieldValueFromJson(const nlohmann::ordered_json& v, FieldType type);
void applyField(const nlohmann::ordered_json& j, Field& f, const ResolveCtx& ctx);
