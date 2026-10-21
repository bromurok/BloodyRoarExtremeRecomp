#include "enhancements.h"
#include "app/app.h"
#include <toml++/toml.hpp>
#include <filesystem>
#include <fstream>

namespace enhancements {

namespace {

Config s_cfg;
bool s_loaded = false;

std::filesystem::path File() {
    return app::ExeDir() / "enhancements.toml";
}

}  // namespace

Config& Get() {
    if (!s_loaded) {
        Load();
    }
    return s_cfg;
}

void Load() {
    s_loaded = true;
    try {
        auto tbl = toml::parse_file(File().string());
        s_cfg.anti_aliasing = tbl["anti_aliasing"].value_or(true);
    } catch (...) {
        s_cfg.anti_aliasing = true;
    }
}

void Save() {
    std::ofstream out(File());
    if (!out) return;
    out << "# Configuracion de mejoras graficas (Bloody Roar Extreme)\n";
    out << "anti_aliasing = " << (s_cfg.anti_aliasing ? "true" : "false") << "\n";
}

}  // namespace enhancements
