#pragma once
#include <string>
#include <vector>

#include "loader.h"

enum class TokenKind {
    Number,
    Operator,
    Function,
};

struct RpnToken {
    TokenKind   kind;
    std::string text;
    double      number = 0.0;
    int         arity = 0;
};

std::vector<RpnToken> parseToRpn(const std::string& expr, const PluginRegistry& plugins);