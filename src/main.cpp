#include <cmath>
#include <iomanip>
#include <iostream>
#include <string>

#include "evaluator.h"
#include "loader.h"
#include "parser.h"

void runOne(const std::string& expr, const PluginRegistry& plugins) {
    auto rpn = parseToRpn(expr, plugins);
    double val = evaluate(rpn, plugins);
    if (std::abs(val - std::round(val)) < 1e-10 && std::abs(val) < 1e15) {
        std::cout << static_cast<long long>(std::llround(val)) << "\n";
    }
    else {
        std::cout << std::setprecision(12) << val << "\n";
    }
}

int main(int argc, char** argv) {
    try {
        auto plugins = loadAllPlugins("./plugins");
        std::cout << "Loaded " << plugins.size() << " plugin(s).\n";

        if (argc >= 2) {
            std::string expr;
            for (int i = 1; i < argc; ++i) {
                if (i > 1) expr += ' ';
                expr += argv[i];
            }
            try {
                runOne(expr, plugins);
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
                return 1;
            }
            return 0;
        }

        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            try {
                runOne(line, plugins);
            }
            catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << "\n";
            }
        }
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
}