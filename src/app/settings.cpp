// the settings file beside the exe
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <atomic>
#include <fstream>
#include <sstream>

#include <toml++/toml.hpp>

#include "app/app.h"
#include "core/log.h"

namespace app {

namespace {

const char* kModes[] = {"fullscreen", "borderless", "windowed"};

std::filesystem::path File() { return ExeDir() / kGame.settings; }

}  // namespace

Settings& Get() {
    static Settings s;
    return s;
}

void Load() {
    Settings& s = Get();
    toml::table t;
    try {
        t = toml::parse_file(File().string());
    } catch (...) {
        return;
    }
    std::string mode = t["display_mode"].value_or(std::string(kModes[uint32_t(s.display_mode)]));
    for (uint32_t i = 0; i < 3; i++)
        if (mode == kModes[i]) s.display_mode = DisplayMode(i);
    s.width = t["width"].value_or(s.width);
    s.height = t["height"].value_or(s.height);
    // el archivo viejo tenia render_percent (100 = nativa): se acepta y se pasa a multiplicador
    if (auto sc = t["render_scale"].value<int64_t>()) s.render_scale = uint32_t(std::clamp<int64_t>(*sc, 1, 16));
    else if (auto pc = t["render_percent"].value<int64_t>())
        s.render_scale = uint32_t(std::clamp<int64_t>(((*pc + 50) / 100), 1, 16));
    s.fps_cap = 60;
    s.vsync = true;
    s.aniso = 0;
    s.renderer = t["renderer"].value_or(s.renderer);
    s.join_ip = t["join_ip"].value_or(s.join_ip);
    if (auto lang_str = t["language"].value<std::string>()) {
        std::string l = *lang_str;
        std::transform(l.begin(), l.end(), l.begin(), [](unsigned char c) { return std::tolower(c); });
        if (l == "spanish" || l == "es" || l == "spa" || l == "espanol") s.language = 5;
        else if (l == "french" || l == "fr" || l == "fre") s.language = 4;
        else if (l == "german" || l == "de" || l == "ger") s.language = 3;
        else if (l == "italian" || l == "it" || l == "ita") s.language = 6;
        else if (l == "japanese" || l == "ja" || l == "jpn") s.language = 2;
        else s.language = 1;
    } else if (auto lang_num = t["language"].value<int64_t>()) {
        s.language = uint32_t(*lang_num);
    }
    s.menu_language = t["menu_language"].value_or(s.language == 5 ? "es" : "en");
    for (const Option& o : kGame.options) {
        if (o.flag) *o.flag = t[o.key].value_or(*o.flag);
        else *o.value = t[o.key].value_or(*o.value);
    }
    if (kGame.load) kGame.load(&t);
}

namespace {

toml::table Table() {
    const Settings& s = Get();
    toml::table t{{"display_mode", kModes[uint32_t(s.display_mode)]},
                  {"width", int64_t(s.width)},
                  {"height", int64_t(s.height)},
                  {"render_scale", int64_t(s.render_scale)},
                  {"renderer", s.renderer},
                  {"join_ip", s.join_ip},
                  {"language", int64_t(s.language)},
                  {"menu_language", s.menu_language}};
    for (const Option& o : kGame.options) {
        if (o.flag) t.insert(o.key, *o.flag);
        else t.insert(o.key, int64_t(*o.value));
    }
    if (kGame.save) kGame.save(&t);
    return t;
}

}  // namespace

void Save() {
    std::ofstream f(File());
    f << Table() << "\n";
}

void LogSettings() {
    std::ostringstream text;
    text << Table();
    std::istringstream lines(text.str());
    LOGR("Settings:");
    for (std::string line; std::getline(lines, line);)
        if (!line.empty()) LOGR("  %s", line.c_str());
}

Mode Desktop() {
    HMONITOR mon = MonitorFromWindow(static_cast<HWND>(Window()), MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    DEVMODEW dm{};
    dm.dmSize = sizeof(dm);
    if (GetMonitorInfoW(mon, &mi) && EnumDisplaySettingsW(mi.szDevice, ENUM_REGISTRY_SETTINGS, &dm))
        return {dm.dmPelsWidth, dm.dmPelsHeight};
    return {uint32_t(GetSystemMetrics(SM_CXSCREEN)), uint32_t(GetSystemMetrics(SM_CYSCREEN))};
}

Mode Output() {
    const Settings& s = Get();
    if (s.width && s.height) return {s.width, s.height};
    return Desktop();
}

bool Widescreen() {
    // asked per frame and per HUD quad: the desktop mode lookup only runs when the resolution setting changes
    static std::atomic<uint64_t> key{~0ull};
    static std::atomic<bool> wide{true};
    const Settings& s = Get();
    uint64_t k = uint64_t(s.width) << 32 | s.height;
    if (key.exchange(k) != k) {
        Mode m = Output();
        wide = m.width * 2 >= m.height * 3;  // 3:2 or wider
    }
    return wide;
}

float RenderScale(uint32_t) {
    // multiplicador de resolucion interna estilo emulador: x1 nativa, x2, x4...
    return float(Get().render_scale);
}

uint32_t Language() {
    return Get().language;
}

}  // namespace app
