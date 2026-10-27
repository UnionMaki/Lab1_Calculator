#include "parser.h"

#include <cctype>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
#include <set>

namespace {

    #define NEG_PRIO 6

   /* bool findPluginB(const PluginRegistry& plugins, const std::string& name) {
        for (const auto& p : plugins) {
            if (p.name == name) return true;
        }
        return false;
    }*/

    enum class RawKind { Number, Identifier, Op, LParen, RParen, Comma };

    struct RawToken {
        RawKind     kind;
        std::string text;
        double      number = 0.0;
    };

    std::vector<RawToken> lex(const std::string& s) {
        std::vector<RawToken> out;
        std::set<char> stdOpSet = { '(', ')', ',', '+', '-', '*', '/' };
        size_t i = 0;
        while (i < s.size()) {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (std::isspace(c)) { ++i; continue; }

            if (std::isdigit(c) || s[i] == '.') {
                size_t j = i;
                while (j < s.size()
                    && (std::isdigit(static_cast<unsigned char>(s[j])) || s[j] == '.')) ++j;
                std::string num = s.substr(i, j - i);
                try {
                    out.push_back({ RawKind::Number, num, std::stod(num) });
                }
                catch (const std::exception&) {
                    throw std::runtime_error("Invalid number literal: " + num);
                }
                i = j;
                continue;
            }
            if (std::isalpha(c) || s[i] == '_') {
                size_t j = i;
                while (j < s.size()
                    && (std::isalnum(static_cast<unsigned char>(s[j])) || s[j] == '_')) ++j;
                out.push_back({ RawKind::Identifier, s.substr(i, j - i), 0.0 });
                i = j;
                continue;
            }
            switch (s[i]) {
            case '(': out.push_back({ RawKind::LParen, "(", 0.0 }); break;
            case ')': out.push_back({ RawKind::RParen, ")", 0.0 }); break;
            case ',': out.push_back({ RawKind::Comma,  ",", 0.0 }); break;
            case '+': case '-': case '*': case '/':
                out.push_back({ RawKind::Op, std::string(1, s[i]), 0.0 });
                break;
            default:
                size_t j = i;
                while (j < s.size() && stdOpSet.find(s[j]) == stdOpSet.end() && !std::isspace(s[j]) && !std::isdigit(s[j]))
                    ++j;
                if (j - i == 1) {
                    out.push_back({RawKind::Op, std::string(1, s[i]), 0.0});
                    i = j;
                    continue;
                }
                throw std::runtime_error(std::string("Unexpected character: ") + s.substr(i, j - i));
            }
            ++i;
        }
        return out;
    }


    bool isBuiltinOp(const std::string& op) {
        return op == "+" || op == "-" || op == "*" || op == "/" || op == "neg";
    }

    int builtinPriority(const std::string& op) {
        if (op == "*" || op == "/") return 3;
        if (op == "+" || op == "-") return 2;
        return 0;
    }


    int pluginOpPriority(const std::string& op) {
        return 5;
    }

    struct OpStackItem {
        enum Kind { Op, Function, LParen } kind;
        std::string text;
        int arity = 0;
    };

    bool isUnaryContext(const std::vector<RawToken>& toks, size_t i) {
        if (i == 0) return true;
        const auto& prev = toks[i - 1];
        return prev.kind == RawKind::LParen
            || prev.kind == RawKind::Comma
            || prev.kind == RawKind::Op;
    }

} 

std::vector<RpnToken> parseToRpn(const std::string& expr, const PluginRegistry& plugins) {
    auto raw = lex(expr);
    std::vector<RpnToken> output;
    std::stack<OpStackItem> ops;
    std::stack<int> argCounts;

    auto popAndEmit = [&]() {
        auto& top = ops.top();
        if (top.kind == OpStackItem::Function) {
            output.push_back({ TokenKind::Function, top.text, 0.0, top.arity });
        }
        else if (top.kind == OpStackItem::Op) {
            int arity = (top.text == "neg") ? 1 : 2;
            output.push_back({ TokenKind::Operator, top.text, 0.0, arity });
        }
        ops.pop();
        };

    for (size_t i = 0; i < raw.size(); ++i) {
        const auto& t = raw[i];
        switch (t.kind) {

        case RawKind::Number:
            output.push_back({ TokenKind::Number, "", t.number, 0 });
            break;

        case RawKind::Identifier: {
            if (i + 1 >= raw.size() || raw[i + 1].kind != RawKind::LParen) {
                throw std::runtime_error("Function name must be followed by '(': " + t.text);
            }
            auto p = findPlugin(plugins, t.text);
            if (!p) throw std::runtime_error("Unknown function: " + t.text);
            ops.push({ OpStackItem::Function, t.text, p->arity });
            break;
        }

        case RawKind::LParen: {
            bool isCall = i > 0 && raw[i - 1].kind == RawKind::Identifier;
            ops.push({ OpStackItem::LParen, "(", 0 });
            argCounts.push(isCall ? 1 : 0);
            break;
        }

        case RawKind::Comma: {
            while (!ops.empty() && ops.top().kind != OpStackItem::LParen) {
                popAndEmit();
            }
            if (ops.empty())
                throw std::runtime_error("',' outside any '('");
            if (argCounts.top() == 0)
                throw std::runtime_error("',' outside a function argument list");
            argCounts.top()++;
            break;
        }


        case RawKind::RParen: {
            if (i == 0
                || raw[i - 1].kind == RawKind::LParen
                || raw[i - 1].kind == RawKind::Comma) {
                throw std::runtime_error("Empty parentheses or empty argument");
            }

            while (!ops.empty() && ops.top().kind != OpStackItem::LParen) {
                popAndEmit();
            }
            if (ops.empty()) throw std::runtime_error("Unbalanced ')'");
            ops.pop();
            int argc = argCounts.top(); argCounts.pop();

            if (!ops.empty() && ops.top().kind == OpStackItem::Function) {
                auto fn = ops.top();
                if (argc != fn.arity) {
                    throw std::runtime_error(
                        "Function " + fn.text + " expects "
                        + std::to_string(fn.arity) + " argument(s), got "
                        + std::to_string(argc));
                }
                popAndEmit();
            }
            break;
        }

        case RawKind::Op: {
            std::string op = t.text;


            bool unary = isUnaryContext(raw, i);
            if (unary) {
                if (op == "+") continue;
                if (op == "-") op = "neg";
                else throw std::runtime_error("Unexpected prefix operator: " + op);
            }

            int prec;
            bool rightAssoc;

            if (isBuiltinOp(op)) {

                (op != "neg") ? prec = builtinPriority(op) : prec = NEG_PRIO;
                
                    
                rightAssoc = false;
            }
            else {

                auto p = findPlugin(plugins, op);
                if (!p) {
                    throw std::runtime_error(
                        "Unknown operator: " + op
                        + " (no such plugin loaded)");
                }
                if (p->arity != 2) {
                    throw std::runtime_error(
                        "Operator " + op + " must have arity 2, got "
                        + std::to_string(p->arity));
                }
                prec = pluginOpPriority(op);
                rightAssoc = true;
            }

            while (!ops.empty() && ops.top().kind == OpStackItem::Op) {
                const auto& top = ops.top();
                int topPrec;
                if (isBuiltinOp(top.text) || top.text == "neg") {
                    topPrec = (top.text == "neg") ? 4 : builtinPriority(top.text);
                }
                else {
                    topPrec = pluginOpPriority(top.text);
                }
                if (topPrec > prec || (topPrec == prec && !rightAssoc)) {
                    popAndEmit();
                }
                else break;
            }
            ops.push({ OpStackItem::Op, op, 0 });
            break;
        }
        }
    }

    while (!ops.empty()) {
        if (ops.top().kind == OpStackItem::LParen)
            throw std::runtime_error("Unbalanced '('");
        if (ops.top().kind == OpStackItem::Function)
            throw std::runtime_error("Function without call: " + ops.top().text);
        popAndEmit();
    }

    return output;
}