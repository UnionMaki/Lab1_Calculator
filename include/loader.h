#pragma once
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <windows.h>

#include "plugin_api.h"

struct ModuleDeleter {
    void operator()(HMODULE h) const noexcept {
        if (h) FreeLibrary(h);
    }
};
using ModuleHandle = std::unique_ptr<std::remove_pointer_t<HMODULE>, ModuleDeleter>;

struct LoadedPlugin {
    std::string  name;
    int          arity;
    double     (*compute)(const double*, std::size_t);
    ModuleHandle handle;
};

using PluginRegistry = std::vector<LoadedPlugin>;

std::vector<std::filesystem::path> findPluginFiles(const std::filesystem::path& pluginsDir);
PluginRegistry loadAllPlugins(const std::filesystem::path& pluginsDir);
const LoadedPlugin* findPlugin(const PluginRegistry& plugins, const std::string& name);