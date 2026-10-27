#include "loader.h"

#include <iostream>

namespace fs = std::filesystem;

std::vector<fs::path> findPluginFiles(const fs::path& pluginsDir) {
    std::vector<fs::path> result;
    if (!fs::exists(pluginsDir) || !fs::is_directory(pluginsDir)) {
        return result;
    }
    for (const auto& entry : fs::directory_iterator(pluginsDir)) {
        if (entry.is_regular_file() && entry.path().extension() == L".dll") {
            result.push_back(entry.path());
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

PluginRegistry loadAllPlugins(const fs::path& pluginsDir) {
    PluginRegistry plugins;

    for (const auto& dllPath : findPluginFiles(pluginsDir)) {
        try {
            HMODULE raw = LoadLibraryW(dllPath.wstring().c_str());
            if (!raw) {
                std::wcerr << L"[skip] cannot load " << dllPath
                    << L" (error " << GetLastError() << L")\n";
                continue;
            }
            ModuleHandle handle(raw);

            auto getInfo = reinterpret_cast<const FunctionInfo * (*)()>(
                GetProcAddress(raw, "GetFunctionInfo"));
            auto compute = reinterpret_cast<double (*)(const double*, std::size_t)>(
                GetProcAddress(raw, "Compute"));

            if (!getInfo || !compute) {
                std::wcerr << L"[skip] " << dllPath
                    << L" does not export required symbols\n";
                continue;
            }

            const FunctionInfo* info = getInfo();
            if (!info || !info->name || info->name[0] == '\0'
                || (info->arity != 1 && info->arity != 2)) {
                std::wcerr << L"[skip] " << dllPath
                    << L" returned invalid FunctionInfo\n";
                continue;
            }

            if (findPlugin(plugins, info->name)) {
                std::wcerr << L"[skip] " << dllPath
                    << L" duplicate function name: "
                    << std::string(info->name).c_str() << L"\n";
                continue;
            }

            plugins.push_back({ info->name, info->arity, compute, std::move(handle) });
            std::wcout << L"[ok] loaded " << dllPath.filename().wstring()
                << L" -> " << info->name
                << L" (arity=" << info->arity << L")\n";
        }
        catch (const std::exception& e) {
            std::wcerr << L"[skip] " << dllPath
                << L" exception: " << e.what() << L"\n";
        }
    }
    return plugins;
}

const LoadedPlugin* findPlugin(const PluginRegistry& plugins, const std::string& name) {
    for (const auto& p : plugins) {
        if (p.name == name) return &p;
    }
    return nullptr;
}