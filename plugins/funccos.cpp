#define PLUGIN_EXPORTS
#include "plugin_api.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <stdexcept>

namespace {
    constexpr double kDegToRad = M_PI / 180.0;
    const FunctionInfo kInfo{ "cos", 1 };
}

extern "C" {

    const FunctionInfo* GetFunctionInfo() {
        return &kInfo;
    }

    double Compute(const double* args, std::size_t argc) {
        if (!args || argc != 1) {
            throw std::invalid_argument("cos: expected exactly 1 argument");
        }
        const double deg = args[0];
        if (!std::isfinite(deg)) {
            throw std::domain_error("cos: argument is not finite");
        }
        const double result = std::cos(deg * kDegToRad);
     /*   if (!std::isfinite(result)) {
            throw std::runtime_error("cos: result is not finite");
        }*/
        return result;
    }

}