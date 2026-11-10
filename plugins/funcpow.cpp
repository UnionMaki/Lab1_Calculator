#define PLUGIN_EXPORTS
#include "plugin_api.h"

#include <cmath>
#include <stdexcept>
#include <sstream>

namespace {
    const FunctionInfo kInfo{ "^", 2 };

    bool isIntegerValue(double v) {
        return std::isfinite(v) && std::floor(v) == v;
    }

    std::string toStr(double v) {
        std::ostringstream os;
        os << v;
        return os.str();
    }
}

extern "C" {

    const FunctionInfo* GetFunctionInfo() {
        return &kInfo;
    }

    double Compute(const double* args, std::size_t argc) {
        if (!args || argc != 2) {
            throw std::invalid_argument("pow: expected exactly 2 arguments");
        }
        const double base = args[0];
        const double exp = args[1];

        if (!std::isfinite(base) || !std::isfinite(exp)) {
            throw std::domain_error(
                "pow: non-finite argument (" + toStr(base) + ", " + toStr(exp) + ")");
        }
        if (base == 0.0 && exp < 0.0) {
            throw std::domain_error("pow: 0 raised to a negative power");
        }
        if (base < 0.0 && !isIntegerValue(exp)) {
            throw std::domain_error(
                "pow: negative base with non-integer exponent ("
                + toStr(base) + ", " + toStr(exp) + ")");
        }

        const double result = std::pow(base, exp);
        if (std::isnan(result)) {
            throw std::domain_error(
                "pow: result is NaN for (" + toStr(base) + ", " + toStr(exp) + ")");
        }
        if (std::isinf(result)) {
            throw std::overflow_error(
                "pow: result overflows to infinity for ("
                + toStr(base) + ", " + toStr(exp) + ")");
        }
        return result;
    }

}
