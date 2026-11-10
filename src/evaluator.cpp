#include "evaluator.h"

#include <stdexcept>

double evaluate(const std::vector<RpnToken>& rpn, const PluginRegistry& plugins) {
    std::vector<double> stack;

    auto pop = [&]() -> double {
        if (stack.empty()) throw std::runtime_error("Stack underflow");
        double v = stack.back();
        stack.pop_back();
        return v;
        };

    for (const auto& t : rpn) {
        switch (t.kind) {
        case TokenKind::Number:
            stack.push_back(t.number);
            break;

        case TokenKind::Operator: {

            if (t.text == "neg") {
                double a = pop();
                stack.push_back(-a);
                break; 
            }

            if (t.text.size() != 1) {
                throw std::runtime_error("Unknown operator: " + t.text);
            }

            switch (t.text[0]) {
            case '+': {
                double b = pop();
                double a = pop();
                stack.push_back(a + b);
                break;
            }
            case '-': {
                double b = pop();
                double a = pop();
                stack.push_back(a - b);
                break;
            }
            case '*': {
                double b = pop();
                double a = pop();
                stack.push_back(a * b);
                break;
            }
            case '/': {
                double b = pop();
                double a = pop();
                if (b == 0.0) throw std::runtime_error("Division by zero");
                stack.push_back(a / b);
                break;
            }
            default:
                if (const LoadedPlugin* p = findPlugin(plugins, t.text)) {
                    double b = pop();
                    double a = pop();
                    std::vector<double> args = { a, b };
                    stack.push_back(p->compute(args.data(), args.size()));
                    break;
                }

                throw std::runtime_error("Unknown operator: " + t.text);
            }
            break;
        }

        case TokenKind::Function: {
            const LoadedPlugin* p = findPlugin(plugins, t.text);
            if (!p) throw std::runtime_error("Unknown function: " + t.text);
            std::vector<double> args(static_cast<size_t>(t.arity));
            for (int i = t.arity - 1; i >= 0; --i) {
                args[static_cast<size_t>(i)] = pop();
            }
            try {
                stack.push_back(p->compute(args.data(), args.size()));
            }
            catch (const std::exception& e) {
                throw std::runtime_error(t.text + " failed: " + e.what());
            }
            break;
        }
        }
    }

    if (stack.size() != 1) {
        throw std::runtime_error("Invalid expression: stack size = "
            + std::to_string(stack.size()) + " (expected 1)");
    }
    return stack.back();
}