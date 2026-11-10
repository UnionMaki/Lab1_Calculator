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

    enum class RawKind { Number, Identifier, Op, LParen, RParen, Comma, Empty };

    struct RawToken {
        RawKind     kind = RawKind::Empty;
        std::string text;
        double      number = 0.0;
    };

    bool isSpaceChar(char ch) {
        return std::isspace(static_cast<unsigned char>(ch)) != 0;
    }

    bool isDigitChar(char ch) {
        return std::isdigit(static_cast<unsigned char>(ch)) != 0;
    }

    bool isAlphaChar(char ch) {
        return std::isalpha(static_cast<unsigned char>(ch)) != 0;
    }

    bool isAlnumChar(char ch) {
        return std::isalnum(static_cast<unsigned char>(ch)) != 0;
    }

    bool isNumberStart(char ch) {
        return isDigitChar(ch) || ch == '.';
    }

    bool isNumberChar(char ch) {
        return isDigitChar(ch) || ch == '.';
    }

    bool isIdentStart(char ch) {
        return isAlphaChar(ch) || ch == '_';
    }

    bool isIdentChar(char ch) {
        return isAlnumChar(ch) || ch == '_';
    }

    bool isStdOp(char ch) {
        static const std::string ops = "(),+-*/";
        return ops.find(ch) != std::string::npos;
    }

    size_t scanNumber(const std::string& s, size_t i) {
        size_t j = i;
        while (j < s.size() && isNumberChar(s[j])) {
            ++j;
        }
        return j;
    }

    RawToken makeNumber(const std::string& s, size_t i, size_t j) {
        std::string num = s.substr(i, j - i);
        try {
            return { RawKind::Number, num, std::stod(num) };
        }
        catch (const std::exception&) {
            throw std::runtime_error("Invalid number literal: " + num);
        }
    }

    size_t scanIdentifier(const std::string& s, size_t i) {
        size_t j = i;
        while (j < s.size() && isIdentChar(s[j])) {
            ++j;
        }
        return j;
    }

    RawToken makeIdentifier(const std::string& s, size_t i, size_t j) {
        return { RawKind::Identifier, s.substr(i, j - i), 0.0 };
    }

    bool tryMakeSimpleToken(char c, RawToken& token) {
        switch (c) {
        case '(':
            token = { RawKind::LParen, "(", 0.0 };
            return true;
        case ')':
            token = { RawKind::RParen, ")", 0.0 };
            return true;
        case ',':
            token = { RawKind::Comma, ",", 0.0 };
            return true;
        case '+':
        case '-':
        case '*':
        case '/':
            token = { RawKind::Op, std::string(1, c), 0.0 };
            return true;
        default:
            return false;
        }
    }

    RawToken makeUnexpectedOrOp(const std::string& s, size_t i, size_t& next) {
        size_t j = i;
        while (j < s.size()
            && !isStdOp(s[j])
            && !isSpaceChar(s[j])
            && !isDigitChar(s[j])) {
            ++j;
        }

        if (j - i == 1) {
            next = j;
            return { RawKind::Op, std::string(1, s[i]), 0.0 };
        }

        throw std::runtime_error(
            std::string("Unexpected character: ") + s.substr(i, j - i)
        );
    }


    std::vector<RawToken> lex(const std::string& s) {
        std::vector<RawToken> out;
        size_t i = 0;

        while (i < s.size()) {
            const char c = s[i];

            if (isSpaceChar(c)) {
                ++i;
                continue;
            }

            if (isNumberStart(c)) {
                const size_t j = scanNumber(s, i);
                out.push_back(makeNumber(s, i, j));
                i = j;
                continue;
            }

            if (isIdentStart(c)) {
                const size_t j = scanIdentifier(s, i);
                out.push_back(makeIdentifier(s, i, j));
                i = j;
                continue;
            }

            RawToken simple{};
            if (tryMakeSimpleToken(c, simple)) {
                out.push_back(simple);
                ++i;
                continue;
            }

            size_t next = i;
            out.push_back(makeUnexpectedOrOp(s, i, next));
            i = next;
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


    int pluginOpPriority() {
        return 5;
    }

    struct OpStackItem {
        enum Kind { Op, Function, LParen, Empty } kind = Kind::Empty;
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

    void popAndEmit(std::stack<OpStackItem>& ops,
        std::vector<RpnToken>& output) {
        const auto& top = ops.top();

        if (top.kind == OpStackItem::Function) {
            output.push_back({ TokenKind::Function, top.text, 0.0, top.arity });
        }
        else if (top.kind == OpStackItem::Op) {
            const int arity = (top.text == "neg") ? 1 : 2;
            output.push_back({ TokenKind::Operator, top.text, 0.0, arity });
        }

        ops.pop();
    }

    int stackOpPriority(const OpStackItem& item) {
        if (item.text == "neg") {
            return 4;
        }
        if (isBuiltinOp(item.text)) {
            return builtinPriority(item.text);
        }
        return pluginOpPriority();
    }

    bool shouldPopOp(const OpStackItem& top, int prec, bool rightAssoc) {
        if (top.kind != OpStackItem::Op) {
            return false;
        }

        const int topPrec = stackOpPriority(top);
        return topPrec > prec || (topPrec == prec && !rightAssoc);
    }

    void handleNumber(const RawToken& t, std::vector<RpnToken>& output) {
        output.push_back({ TokenKind::Number, "", t.number, 0 });
    }

    void handleIdentifier(const std::vector<RawToken>& raw,
        size_t i,
        const RawToken& t,
        const PluginRegistry& plugins,
        std::stack<OpStackItem>& ops) {
        if (i + 1 >= raw.size() || raw[i + 1].kind != RawKind::LParen) {
            throw std::runtime_error("Function name must be followed by '(': " + t.text);
        }

        const auto* p = findPlugin(plugins, t.text);
        if (!p) {
            throw std::runtime_error("Unknown function: " + t.text);
        }

        ops.push({ OpStackItem::Function, t.text, p->arity });
    }

    void handleLParen(const std::vector<RawToken>& raw,
        size_t i,
        std::stack<OpStackItem>& ops,
        std::stack<int>& argCounts) {
        const bool isCall = i > 0 && raw[i - 1].kind == RawKind::Identifier;
        ops.push({ OpStackItem::LParen, "(", 0 });
        argCounts.push(isCall ? 1 : 0);
    }

    void handleComma(std::stack<OpStackItem>& ops,
        std::stack<int>& argCounts,
        std::vector<RpnToken>& output) {
        while (!ops.empty() && ops.top().kind != OpStackItem::LParen) {
            popAndEmit(ops, output);
        }

        if (ops.empty()) {
            throw std::runtime_error("',' outside any '('");
        }
        if (argCounts.top() == 0) {
            throw std::runtime_error("',' outside a function argument list");
        }

        argCounts.top()++;
    }

    void handleRParen(const std::vector<RawToken>& raw,
        size_t i,
        std::stack<OpStackItem>& ops,
        std::stack<int>& argCounts,
        std::vector<RpnToken>& output) {
        if (i == 0
            || raw[i - 1].kind == RawKind::LParen
            || raw[i - 1].kind == RawKind::Comma) {
            throw std::runtime_error("Empty parentheses or empty argument");
        }

        while (!ops.empty() && ops.top().kind != OpStackItem::LParen) {
            popAndEmit(ops, output);
        }

        if (ops.empty()) {
            throw std::runtime_error("Unbalanced ')'");
        }

        ops.pop();

        const int argc = argCounts.top();
        argCounts.pop();

        if (!ops.empty() && ops.top().kind == OpStackItem::Function) {
            const auto& fn = ops.top();

            if (argc != fn.arity) {
                throw std::runtime_error(
                    "Function " + fn.text + " expects "
                    + std::to_string(fn.arity) + " argument(s), got "
                    + std::to_string(argc));
            }

            popAndEmit(ops, output);
        }
    }

    struct OpInfo {
        int prec;
        bool rightAssoc;
    };

    OpInfo resolveOpInfo(const std::string& op,
        const PluginRegistry& plugins) {
        if (isBuiltinOp(op)) {
            const int prec = (op == "neg") ? NEG_PRIO : builtinPriority(op);
            return { prec, false };
        }

        const auto* p = findPlugin(plugins, op);

        if (!p) {
            throw std::runtime_error(
                "Unknown operator: " + op + " (no such plugin loaded)");
        }

        if (p->arity != 2) {
            throw std::runtime_error(
                "Operator " + op + " must have arity 2, got "
                + std::to_string(p->arity));
        }

        return { pluginOpPriority(), true };
    }

    void handleOp(const std::vector<RawToken>& raw,
        size_t i,
        const RawToken& t,
        const PluginRegistry& plugins,
        std::stack<OpStackItem>& ops,
        std::vector<RpnToken>& output) {
        std::string op = t.text;

        if (isUnaryContext(raw, i)) {
            if (op == "+") {
                return;
            }
            if (op == "-") {
                op = "neg";
            }
            else {
                throw std::runtime_error("Unexpected prefix operator: " + op);
            }
        }

        const OpInfo info = resolveOpInfo(op, plugins);

        while (!ops.empty()
            && shouldPopOp(ops.top(), info.prec, info.rightAssoc)) {
            popAndEmit(ops, output);
        }

        ops.push({ OpStackItem::Op, op, 0 });
    }

    void finalizeOps(std::stack<OpStackItem>& ops,
        std::vector<RpnToken>& output) {
        while (!ops.empty()) {
            if (ops.top().kind == OpStackItem::LParen) {
                throw std::runtime_error("Unbalanced '('");
            }

            if (ops.top().kind == OpStackItem::Function) {
                throw std::runtime_error("Function without call: " + ops.top().text);
            }

            popAndEmit(ops, output);
        }
    }

} 


std::vector<RpnToken> parseToRpn(const std::string& expr,
const PluginRegistry& plugins) {
const auto raw = lex(expr);

std::vector<RpnToken> output;
std::stack<OpStackItem> ops;
std::stack<int> argCounts;

for (size_t i = 0; i < raw.size(); ++i) {
    const auto& t = raw[i];

    switch (t.kind) {
    case RawKind::Number:
        handleNumber(t, output);
        break;

    case RawKind::Identifier:
        handleIdentifier(raw, i, t, plugins, ops);
        break;

    case RawKind::LParen:
        handleLParen(raw, i, ops, argCounts);
        break;

    case RawKind::Comma:
        handleComma(ops, argCounts, output);
        break;

    case RawKind::RParen:
        handleRParen(raw, i, ops, argCounts, output);
        break;

    case RawKind::Op:
        handleOp(raw, i, t, plugins, ops, output);
        break;
    }
}

finalizeOps(ops, output);
return output;
}