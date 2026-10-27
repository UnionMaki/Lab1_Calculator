#pragma once
#include <vector>

#include "loader.h"
#include "parser.h"

double evaluate(const std::vector<RpnToken>& rpn, const PluginRegistry& plugins);