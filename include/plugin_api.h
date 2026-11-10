// plugin_api.h — общий заголовок для хоста и плагинов
#pragma once
#include <cstddef>

extern "C" {
    struct FunctionInfo {
        const char* name; 
        int         arity;
    };

    __declspec(dllexport) const FunctionInfo* GetFunctionInfo();
    __declspec(dllexport) double Compute(const double* args, std::size_t argc);
}