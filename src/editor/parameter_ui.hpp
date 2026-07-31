#pragma once

#include <optional>
#include <vector>

#include "core/parameters/parameters.hpp"

struct ParameterItem {
    Parameter* parameter = nullptr;
    ParameterPath path;
    bool collapsible = false;
    std::optional<ParameterCondition> condition;
    std::vector<ParameterItem> children;
};

class ParameterUI {
public:
    static void drawGroup(const ParameterPath& root);

private:
    static ParameterUI& get();

    static bool drawParameter(Parameter& p);
    static void drawItem(ParameterItem& item, bool& changed, bool& restartNeeded);
    static std::vector<ParameterItem> buildItems(const ParameterPath& prefix);

    ParameterItem root;
};
