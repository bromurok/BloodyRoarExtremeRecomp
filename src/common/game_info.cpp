#include "app/app.h"
#include "config_menu.h"
#include "core/input.h"
#include "core/log.h"

#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <toml++/toml.hpp>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

struct KeyBindings {
    std::vector<int> up = {VK_UP, 'W'};
    std::vector<int> down = {VK_DOWN, 'S'};
    std::vector<int> left = {VK_LEFT, 'A'};
    std::vector<int> right = {VK_RIGHT, 'D'};
    std::vector<int> start = {VK_RETURN, VK_SPACE};
    std::vector<int> back = {VK_ESCAPE, VK_BACK};
    std::vector<int> punch = {'J', 'Z'};        // A
    std::vector<int> kick = {'K', 'X'};         // B
    std::vector<int> beast = {'U', 'C'};        // X
    std::vector<int> y_button = {'I', 'V'};     // Y
    std::vector<int> black_rb = {'E', '2'};     // Black / RB
    std::vector<int> white_lb = {'Q', '1'};     // White / LB
    std::vector<int> lt_trigger = {VK_SHIFT, VK_LSHIFT}; // LT (Sidestep in)
    std::vector<int> rt_trigger = {VK_RSHIFT, VK_CONTROL}; // RT (Sidestep away)
} s_keys_config;

int ParseKeyName(std::string name) {
    std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::toupper(c); });
    if (name.empty()) return 0;
    if (name.size() == 1) {
        char c = name[0];
        if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) return c;
    }
    if (name == "UP") return VK_UP;
    if (name == "DOWN") return VK_DOWN;
    if (name == "LEFT") return VK_LEFT;
    if (name == "RIGHT") return VK_RIGHT;
    if (name == "ENTER" || name == "RETURN") return VK_RETURN;
    if (name == "SPACE") return VK_SPACE;
    if (name == "ESC" || name == "ESCAPE") return VK_ESCAPE;
    if (name == "BACKSPACE" || name == "BACK") return VK_BACK;
    if (name == "TAB") return VK_TAB;
    if (name == "SHIFT") return VK_SHIFT;
    if (name == "LSHIFT") return VK_LSHIFT;
    if (name == "RSHIFT") return VK_RSHIFT;
    if (name == "CTRL" || name == "CONTROL") return VK_CONTROL;
    if (name == "LCTRL") return VK_LCONTROL;
    if (name == "RCTRL") return VK_RCONTROL;
    if (name == "ALT") return VK_MENU;
    return 0;
}

std::string KeyToString(int vk) {
    if ((vk >= 'A' && vk <= 'Z') || (vk >= '0' && vk <= '9')) return std::string(1, char(std::tolower(vk)));
    switch (vk) {
        case VK_UP: return "up";
        case VK_DOWN: return "down";
        case VK_LEFT: return "left";
        case VK_RIGHT: return "right";
        case VK_RETURN: return "enter";
        case VK_SPACE: return "space";
        case VK_ESCAPE: return "esc";
        case VK_BACK: return "backspace";
        case VK_TAB: return "tab";
        case VK_SHIFT: return "shift";
        case VK_LSHIFT: return "lshift";
        case VK_RSHIFT: return "rshift";
        case VK_CONTROL: return "ctrl";
        default: return "";
    }
}

void LoadKeyList(const toml::node_view<const toml::node>& node, std::vector<int>& dest) {
    if (const auto* arr = node.as_array()) {
        std::vector<int> parsed;
        for (const auto& elem : *arr) {
            if (auto s = elem.value<std::string>()) {
                int k = ParseKeyName(*s);
                if (k > 0) parsed.push_back(k);
            }
        }
        if (!parsed.empty()) dest = std::move(parsed);
    } else if (auto s = node.value<std::string>()) {
        int k = ParseKeyName(*s);
        if (k > 0) dest = {k};
    }
}

void OnLoadSettings(const void* table_ptr) {
    if (!table_ptr) return;
    const auto& t = *static_cast<const toml::table*>(table_ptr);
    const auto* kb = t.get_as<toml::table>("keyboard");
    if (!kb) return;

    LoadKeyList((*kb)["up"], s_keys_config.up);
    LoadKeyList((*kb)["down"], s_keys_config.down);
    LoadKeyList((*kb)["left"], s_keys_config.left);
    LoadKeyList((*kb)["right"], s_keys_config.right);
    LoadKeyList((*kb)["start"], s_keys_config.start);
    LoadKeyList((*kb)["back"], s_keys_config.back);
    LoadKeyList((*kb)["punch"], s_keys_config.punch);
    LoadKeyList((*kb)["kick"], s_keys_config.kick);
    LoadKeyList((*kb)["beast"], s_keys_config.beast);
    LoadKeyList((*kb)["y"], s_keys_config.y_button);
    LoadKeyList((*kb)["black_rb"], s_keys_config.black_rb);
    LoadKeyList((*kb)["white_lb"], s_keys_config.white_lb);
    LoadKeyList((*kb)["lt_trigger"], s_keys_config.lt_trigger);
    LoadKeyList((*kb)["rt_trigger"], s_keys_config.rt_trigger);
}

void AppendArray(toml::table& tbl, const char* key, const std::vector<int>& vks) {
    toml::array arr;
    for (int vk : vks) {
        std::string s = KeyToString(vk);
        if (!s.empty()) arr.push_back(s);
    }
    tbl.insert(key, arr);
}

void OnSaveSettings(void* table_ptr) {
    if (!table_ptr) return;
    auto& t = *static_cast<toml::table*>(table_ptr);
    toml::table kb;
    AppendArray(kb, "up", s_keys_config.up);
    AppendArray(kb, "down", s_keys_config.down);
    AppendArray(kb, "left", s_keys_config.left);
    AppendArray(kb, "right", s_keys_config.right);
    AppendArray(kb, "start", s_keys_config.start);
    AppendArray(kb, "back", s_keys_config.back);
    AppendArray(kb, "punch", s_keys_config.punch);
    AppendArray(kb, "kick", s_keys_config.kick);
    AppendArray(kb, "beast", s_keys_config.beast);
    AppendArray(kb, "y", s_keys_config.y_button);
    AppendArray(kb, "black_rb", s_keys_config.black_rb);
    AppendArray(kb, "white_lb", s_keys_config.white_lb);
    AppendArray(kb, "lt_trigger", s_keys_config.lt_trigger);
    AppendArray(kb, "rt_trigger", s_keys_config.rt_trigger);
    t.insert_or_assign("keyboard", kb);
}

std::atomic<bool> s_keys[256] = {};
std::atomic<bool> s_active = false;
// pidiendo el estado crudo de un hueco SDL: el callback no debe meter el teclado
thread_local bool s_raw_poll = false;

bool OnMessage(void* hwnd, uint32_t msg, uintptr_t wp, intptr_t lp, intptr_t* result) {
    switch (msg) {
        case WM_KEYDOWN:
            if (wp == VK_F1 && !(lp & 0x40000000)) {  // F1 abre/cierra el menu (sin repetir)
                cfgmenu::Toggle();
                for (auto& k : s_keys) k.store(false, std::memory_order_relaxed);
                return true;
            }
            if (cfgmenu::Open()) return true;  // con el menu abierto el teclado no pasa al juego
            if (wp < 256) {
                s_keys[wp].store(true, std::memory_order_relaxed);
                if (!s_active.load(std::memory_order_relaxed)) {
                    s_active.store(true, std::memory_order_relaxed);
                    LOGI("game: keyboard controller activated by key %u", (uint32_t)wp);
                }
            }
            break;
        case WM_KEYUP:
            if (wp < 256) s_keys[wp].store(false, std::memory_order_relaxed);
            break;
        case WM_KILLFOCUS:
            for (auto& k : s_keys) k.store(false, std::memory_order_relaxed);
            break;
    }
    return false;
}

void OnPad(int port, input::Pad& pad) {
    if (s_raw_poll) return;  // llega crudo del hueco SDL: devolverlo tal cual, sin teclado

    // menu abierto o ventana sin foco: el teclado global (GetAsyncKeyState) no debe colarse al juego
    if (cfgmenu::Blocked()) {
        pad.buttons = 0;
        std::memset(pad.analog, 0, sizeof(pad.analog));
        pad.lx = pad.ly = pad.rx = pad.ry = 0;
        return;
    }

    // Event-driven keyboard polling: avoids dozens of synchronous GetAsyncKeyState kernel calls per frame
    auto is_down = [](int vk) -> bool {
        return vk >= 0 && vk < 256 && s_keys[vk].load(std::memory_order_relaxed);
    };

    // Control durante los primeros 12 segundos (FMV / Logo):
    // Mantenemos la espera de 12 s, pero tan pronto como se pulse START se desbloquea TODO el input
    // para que el usuario pueda avanzar rapido sin esperar los 12 s completos.
    static auto s_boot_time = std::chrono::steady_clock::now();
    static bool s_skipped_boot = false;
    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - s_boot_time).count();
    const bool booting = !s_skipped_boot && elapsed_ms < 12000;

    // Mando 2 (puerto 1): forzamos conectado para que el juego arranque y quede el versus.
    // Si hay mando fisico manda su estado crudo; si no, estado neutro.
    if (port >= 1 && port <= 3) {
        s_raw_poll = true;
        input::Pad p_raw = input::Poll(port - 1);
        s_raw_poll = false;
        pad = p_raw;
        pad.connected = true;  // forzar para activar versus
        if (booting) {
            pad.buttons = (pad.buttons & 0x0010) ? 0x0010 : 0;
            std::memset(pad.analog, 0, sizeof(pad.analog));
            pad.lx = pad.ly = pad.rx = pad.ry = 0;
        }
        return;
    }
    if (port != 0) return;

    // Puerto 0: teclado SIEMPRE conectado para que el juego inicie sin mando
    // Limpiamos cualquier estado que pudiera venir mezclado y aplicamos solo teclado
    pad.connected = true;
    pad.buttons = 0;
    std::memset(pad.analog, 0, sizeof(pad.analog));
    pad.lx = pad.ly = pad.rx = pad.ry = 0;
    // Siempre activo para evitar que el juego bloquee sin haber pulsado tecla
    s_active.store(true, std::memory_order_relaxed);

    auto any_down = [&](const std::vector<int>& vks) -> bool {
        for (int vk : vks) {
            if (is_down(vk)) return true;
        }
        return false;
    };

    if (booting) {
        bool start = any_down(s_keys_config.start);
        pad.buttons = start ? 0x0010 : 0x0000;
        std::memset(pad.analog, 0, sizeof(pad.analog));
        pad.lx = pad.ly = pad.rx = pad.ry = 0;
        if (start) s_skipped_boot = true;
        return;
    }

    if (!s_active.load(std::memory_order_relaxed) && !any_down(s_keys_config.start)) {
        return;
    }

    // D-Pad / Movement
    if (any_down(s_keys_config.up)) { pad.buttons |= 0x0001; pad.ly = 32767; }
    if (any_down(s_keys_config.down)) { pad.buttons |= 0x0002; pad.ly = -32767; }
    if (any_down(s_keys_config.left)) { pad.buttons |= 0x0004; pad.lx = -32767; }
    if (any_down(s_keys_config.right)) { pad.buttons |= 0x0008; pad.lx = 32767; }

    // Start & Back
    if (any_down(s_keys_config.start)) pad.buttons |= 0x0010;
    if (any_down(s_keys_config.back)) pad.buttons |= 0x0020;

    // Face Buttons: A, B, X, Y
    if (any_down(s_keys_config.punch)) pad.analog[0] = 0xFF;    // A (Punch / Select)
    if (any_down(s_keys_config.kick)) pad.analog[1] = 0xFF;     // B (Kick / Cancel)
    if (any_down(s_keys_config.beast)) pad.analog[2] = 0xFF;    // X (Beast)
    if (any_down(s_keys_config.y_button)) pad.analog[3] = 0xFF; // Y

    // Shoulders: Black (RB) & White (LB)
    if (any_down(s_keys_config.black_rb)) pad.analog[4] = 0xFF; // Black / RB
    if (any_down(s_keys_config.white_lb)) pad.analog[5] = 0xFF; // White / LB

    // Triggers: LT & RT (Sidestep)
    if (any_down(s_keys_config.lt_trigger)) pad.analog[6] = 0xFF; // LT (Sidestep in)
    if (any_down(s_keys_config.rt_trigger)) pad.analog[7] = 0xFF; // RT (Sidestep away)
}

// el mando fisico vive en el hueco 0 de SDL, pero el juego lo pide en el puerto 1.
// La vibracion de cada hueco SDL queda aqui para que un puerto no la pise con la de otro:
// el hueco N recibe la del puerto N (el teclado no vibra, asi que el 0 repite la del mando).
uint16_t s_slot_rumble_left[4] = {};
uint16_t s_slot_rumble_right[4] = {};

void OnRumble(int port, uint16_t& left, uint16_t& right) {
    if (port < 0 || port > 3) return;
    static thread_local bool inner = false;
    if (inner) return;  // input::Rumble() vuelve a llamar aqui: dejar los valores tal cual
    inner = true;
    if (port >= 1 && port <= 3) {
        s_slot_rumble_left[port - 1] = left;
        s_slot_rumble_right[port - 1] = right;
        input::Rumble(port - 1, left, right);  // el hueco SDL que sirve a este puerto
    }
    left = s_slot_rumble_left[port];   // lo que el hueco SDL `port` tenia que llevar
    right = s_slot_rumble_right[port];
    inner = false;
}

void OnUi(double fps) { cfgmenu::Draw(fps); }

}  // namespace

// what the runtime asks of the game; strings must be set, hooks may stay null
const app::GameInfo app::kGame = {
    .title = L"Bloody Roar Extreme",     // window and splash
    .short_title = L"Bloody Roar",       // message boxes
    .name = L"BloodyRoarExtreme",        // window classes, crash dumps, handover event
    .log = "bloody_roar",                // log file prefix
    .settings = "settings.toml",         // settings file beside the exe
    .splash = L"splash",                 // optional splash.png or splash.bmp beside the exe
    .missing = L"Put the game in the assets folder: its disc image (.iso), or default.xbe with its folders beside it.",
    .images = {{"default.xbe", "BloodyRoarExtreme"}}, // each recompiled xbe and its exe, for title launches
    .options = {},                        // game settings kept in the settings file
    .load = OnLoadSettings,               // reads more from the settings file's toml::table
    .save = OnSaveSettings,               // writes more into it
    .init = nullptr,                      // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = nullptr,                    // the game window is up
    .message = OnMessage,                 // window messages first; true takes one
    .ui = OnUi,                           // ImGui drawing each frame
    .pad = OnPad,                         // changes a polled pad
    .rumble = OnRumble,                   // changes a rumble before it is set
};
