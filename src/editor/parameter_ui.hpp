#pragma once

#include <filesystem>

#include "core/parameter_handler.hpp"

class ParameterUI {
public:
    static void drawGroup(ParameterHandler& handler, const ParameterPath& root);

private:
    static bool drawParam(ParamBase& param);
    static bool drawNode(ParameterHandler& handler, const ParameterPath& prefix, bool& restartNeeded);
};
