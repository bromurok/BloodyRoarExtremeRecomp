#include "app/app.h"
#include "config_menu.h"
#include "core/input.h"
#include "core/log.h"

#include <atomic>
#include <chrono>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace {

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

    auto is_down = [](int vk) -> bool {
        return (GetAsyncKeyState(vk) & 0x8000) != 0 || (vk >= 0 && vk < 256 && s_keys[vk].load(std::memory_order_relaxed));
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

    if (booting) {
        bool start = is_down(VK_RETURN) || is_down(VK_SPACE);
        pad.buttons = start ? 0x0010 : 0x0000;
        std::memset(pad.analog, 0, sizeof(pad.analog));
        pad.lx = pad.ly = pad.rx = pad.ry = 0;
        if (start) s_skipped_boot = true;
        return;
    }

    if (!s_active.load(std::memory_order_relaxed) && !is_down(VK_RETURN) && !is_down(VK_SPACE)) {
        return;
    }

    // D-Pad / Movement (Arrows and WASD)
    if (is_down(VK_UP) || is_down('W')) { pad.buttons |= 0x0001; pad.ly = 32767; }
    if (is_down(VK_DOWN) || is_down('S')) { pad.buttons |= 0x0002; pad.ly = -32767; }
    if (is_down(VK_LEFT) || is_down('A')) { pad.buttons |= 0x0004; pad.lx = -32767; }
    if (is_down(VK_RIGHT) || is_down('D')) { pad.buttons |= 0x0008; pad.lx = 32767; }

    // Start (Enter, Space) & Back (Escape, Backspace)
    if (is_down(VK_RETURN) || is_down(VK_SPACE)) pad.buttons |= 0x0010;
    if (is_down(VK_ESCAPE) || is_down(VK_BACK)) pad.buttons |= 0x0020;

    // Face Buttons: A, B, X, Y
    if (is_down('J') || is_down('Z')) pad.analog[0] = 0xFF; // A (Punch / Select)
    if (is_down('K') || is_down('X')) pad.analog[1] = 0xFF; // B (Kick / Cancel)
    if (is_down('U') || is_down('C')) pad.analog[2] = 0xFF; // X (Beast)
    if (is_down('I') || is_down('V')) pad.analog[3] = 0xFF; // Y

    // Shoulders: Black (RB) & White (LB)
    if (is_down('E') || is_down('2')) pad.analog[4] = 0xFF; // Black / RB
    if (is_down('Q') || is_down('1')) pad.analog[5] = 0xFF; // White / LB

    // Triggers: LT & RT
    if (is_down(VK_SHIFT) || is_down(VK_LSHIFT)) pad.analog[6] = 0xFF; // LT
    if (is_down(VK_RSHIFT) || is_down(VK_CONTROL)) pad.analog[7] = 0xFF; // RT
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
    .load = nullptr,                      // reads more from the settings file's toml::table
    .save = nullptr,                      // writes more into it
    .init = nullptr,                      // after the settings load, before the window and boot
    .loaded = nullptr,                    // image in memory, before its entry point runs
    .window = nullptr,                    // the game window is up
    .message = OnMessage,                 // window messages first; true takes one
    .ui = OnUi,                           // ImGui drawing each frame
    .pad = OnPad,                         // changes a polled pad
    .rumble = OnRumble,                   // changes a rumble before it is set
};
