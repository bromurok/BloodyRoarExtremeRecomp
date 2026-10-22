// controllers through SDL: Xbox, PlayStation, Switch and generic USB/DirectInput/XInput pads
#include "core/input.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iterator>
#include <mutex>

#include <SDL3/SDL.h>

#include "app/app.h"
#include "core/log.h"

namespace input {

namespace {

struct ControllerSlot {
    SDL_JoystickID id = 0;
    SDL_Gamepad* gamepad = nullptr;
    SDL_Joystick* joystick = nullptr;
};

std::mutex g_lock;
bool g_sdl = false;
ControllerSlot g_pads[4] = {};

void AddPad(SDL_JoystickID id) {
    for (const auto& slot : g_pads) {
        if (slot.id == id) return;
    }
    ControllerSlot* free_slot = nullptr;
    for (auto& slot : g_pads) {
        if (!slot.gamepad && !slot.joystick) {
            free_slot = &slot;
            break;
        }
    }
    if (!free_slot) return;

    free_slot->id = id;
    if (SDL_IsGamepad(id)) {
        free_slot->gamepad = SDL_OpenGamepad(id);
        const char* name = free_slot->gamepad ? SDL_GetGamepadName(free_slot->gamepad) : SDL_GetError();
        LOGR("input: controller %d (gamepad) %s: %s", int(free_slot - g_pads) + 1,
             free_slot->gamepad ? "plugged in" : "failed to open", name ? name : "");
    } else {
        free_slot->joystick = SDL_OpenJoystick(id);
        const char* name = free_slot->joystick ? SDL_GetJoystickName(free_slot->joystick) : SDL_GetError();
        LOGR("input: controller %d (generic joystick) %s: %s", int(free_slot - g_pads) + 1,
             free_slot->joystick ? "plugged in" : "failed to open", name ? name : "");
    }
}

void RemovePad(SDL_JoystickID id) {
    for (int i = 0; i < 4; i++) {
        if (g_pads[i].id != id) continue;
        if (g_pads[i].gamepad) {
            SDL_CloseGamepad(g_pads[i].gamepad);
            g_pads[i].gamepad = nullptr;
        }
        if (g_pads[i].joystick) {
            SDL_CloseJoystick(g_pads[i].joystick);
            g_pads[i].joystick = nullptr;
        }
        g_pads[i].id = 0;
        LOGR("input: controller %d unplugged", i + 1);
    }
}

// once a millisecond at most, however many ports and threads read
void UpdatePads() {
    static uint64_t last = 0;
    uint64_t now = SDL_GetTicks();
    if (!g_sdl || now == last) return;
    last = now;
    SDL_PumpEvents();
    SDL_Event e;
    while (SDL_PeepEvents(&e, 1, SDL_GETEVENT, SDL_EVENT_FIRST, SDL_EVENT_LAST) == 1) {
        if (e.type == SDL_EVENT_GAMEPAD_ADDED) AddPad(e.gdevice.which);
        else if (e.type == SDL_EVENT_GAMEPAD_REMOVED) RemovePad(e.gdevice.which);
        else if (e.type == SDL_EVENT_JOYSTICK_ADDED) AddPad(e.jdevice.which);
        else if (e.type == SDL_EVENT_JOYSTICK_REMOVED) RemovePad(e.jdevice.which);
    }
}

}  // namespace

void Init() {
    SDL_SetHint(SDL_HINT_JOYSTICK_THREAD, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_RAWINPUT, "1");
    SDL_SetHint(SDL_HINT_XINPUT_ENABLED, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_WGI, "1");

    if (!SDL_Init(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK)) {
        LOGE("input: SDL failed to start, no controllers: %s", SDL_GetError());
        return;
    }
    int v = SDL_GetVersion();
    LOGR("input: SDL %d.%d.%d initialized", SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v));

    if (std::filesystem::exists("gamecontrollerdb.txt")) {
        int n = SDL_AddGamepadMappingsFromFile("gamecontrollerdb.txt");
        LOGR("input: loaded %d mappings from gamecontrollerdb.txt", n);
    }

    std::lock_guard lock(g_lock);
    g_sdl = true;
    int count = 0;
    if (SDL_JoystickID* ids = SDL_GetJoysticks(&count)) {
        for (int i = 0; i < count; i++) AddPad(ids[i]);
        SDL_free(ids);
    }
}

Pad Poll(int port) {
    Pad p;
    {
        std::lock_guard lock(g_lock);
        UpdatePads();
        if (port >= 0 && port < 4) {
            const auto& slot = g_pads[port];
            if (slot.gamepad) {
                p.connected = true;
                constexpr SDL_GamepadButton kButtons[] = {
                    SDL_GAMEPAD_BUTTON_DPAD_UP,   SDL_GAMEPAD_BUTTON_DPAD_DOWN,  SDL_GAMEPAD_BUTTON_DPAD_LEFT,
                    SDL_GAMEPAD_BUTTON_DPAD_RIGHT, SDL_GAMEPAD_BUTTON_START,     SDL_GAMEPAD_BUTTON_BACK,
                    SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK};
                for (int i = 0; i < 8; i++) p.buttons |= uint16_t(SDL_GetGamepadButton(slot.gamepad, kButtons[i]) << i);
                // A B X Y black white
                constexpr SDL_GamepadButton kAnalog[] = {SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST,
                                                         SDL_GAMEPAD_BUTTON_WEST,  SDL_GAMEPAD_BUTTON_NORTH,
                                                         SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER};
                for (int i = 0; i < 6; i++) p.analog[i] = SDL_GetGamepadButton(slot.gamepad, kAnalog[i]) ? 0xFF : 0;
                auto axis = [&](SDL_GamepadAxis a) { return int(SDL_GetGamepadAxis(slot.gamepad, a)); };
                // 0 to 32767 into 0 to 255
                p.analog[6] = uint8_t(std::max(axis(SDL_GAMEPAD_AXIS_LEFT_TRIGGER), 0) >> 7);
                p.analog[7] = uint8_t(std::max(axis(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER), 0) >> 7);
                // SDL's sticks have y down
                // not plain negation: -32768 has no positive
                p.lx = int16_t(axis(SDL_GAMEPAD_AXIS_LEFTX));
                p.ly = int16_t(-1 - axis(SDL_GAMEPAD_AXIS_LEFTY));
                p.rx = int16_t(axis(SDL_GAMEPAD_AXIS_RIGHTX));
                p.ry = int16_t(-1 - axis(SDL_GAMEPAD_AXIS_RIGHTY));
            } else if (slot.joystick) {
                p.connected = true;
                SDL_Joystick* joy = slot.joystick;
                int n_btns = SDL_GetNumJoystickButtons(joy);

                // Common generic layout: 0: A, 1: B, 2: X, 3: Y
                if (n_btns > 0 && SDL_GetJoystickButton(joy, 0)) p.analog[0] = 0xFF; // A
                if (n_btns > 1 && SDL_GetJoystickButton(joy, 1)) p.analog[1] = 0xFF; // B
                if (n_btns > 2 && SDL_GetJoystickButton(joy, 2)) p.analog[2] = 0xFF; // X
                if (n_btns > 3 && SDL_GetJoystickButton(joy, 3)) p.analog[3] = 0xFF; // Y
                // Shoulders: 4: LB/White, 5: RB/Black
                if (n_btns > 4 && SDL_GetJoystickButton(joy, 4)) p.analog[5] = 0xFF; // LB
                if (n_btns > 5 && SDL_GetJoystickButton(joy, 5)) p.analog[4] = 0xFF; // RB
                // Triggers or extra buttons: 6: LT, 7: RT
                if (n_btns > 6 && SDL_GetJoystickButton(joy, 6)) p.analog[6] = 0xFF; // LT
                if (n_btns > 7 && SDL_GetJoystickButton(joy, 7)) p.analog[7] = 0xFF; // RT
                // Back & Start: 8: Back, 9: Start
                if (n_btns > 8 && SDL_GetJoystickButton(joy, 8)) p.buttons |= 0x0020; // Back
                if (n_btns > 9 && SDL_GetJoystickButton(joy, 9)) p.buttons |= 0x0010; // Start
                // Stick clicks
                if (n_btns > 10 && SDL_GetJoystickButton(joy, 10)) p.buttons |= 0x0040; // LStick
                if (n_btns > 11 && SDL_GetJoystickButton(joy, 11)) p.buttons |= 0x0080; // RStick

                // D-Pad from Hat
                if (SDL_GetNumJoystickHats(joy) > 0) {
                    Uint8 hat = SDL_GetJoystickHat(joy, 0);
                    if (hat & SDL_HAT_UP) { p.buttons |= 0x0001; p.ly = 32767; }
                    if (hat & SDL_HAT_DOWN) { p.buttons |= 0x0002; p.ly = -32767; }
                    if (hat & SDL_HAT_LEFT) { p.buttons |= 0x0004; p.lx = -32767; }
                    if (hat & SDL_HAT_RIGHT) { p.buttons |= 0x0008; p.lx = 32767; }
                }

                // Analog Axes
                int n_axes = SDL_GetNumJoystickAxes(joy);
                if (n_axes >= 2) {
                    int16_t jx = SDL_GetJoystickAxis(joy, 0);
                    int16_t jy = SDL_GetJoystickAxis(joy, 1);
                    if (std::abs(jx) > 4000) p.lx = jx;
                    if (std::abs(jy) > 4000) p.ly = int16_t(-1 - jy);
                }
                if (n_axes >= 4) {
                    int16_t rx = SDL_GetJoystickAxis(joy, 2);
                    int16_t ry = SDL_GetJoystickAxis(joy, 3);
                    if (std::abs(rx) > 4000) p.rx = rx;
                    if (std::abs(ry) > 4000) p.ry = int16_t(-1 - ry);
                }
            }
        }
    }
    if (app::kGame.pad) app::kGame.pad(port, p);
    return p;
}

// the motors run until set again
void Rumble(int port, uint16_t left, uint16_t right) {
    if (app::kGame.rumble) app::kGame.rumble(port, left, right);
    std::lock_guard lock(g_lock);
    if (port >= 0 && port < 4) {
        if (g_pads[port].gamepad) SDL_RumbleGamepad(g_pads[port].gamepad, left, right, 0);
        else if (g_pads[port].joystick) SDL_RumbleJoystick(g_pads[port].joystick, left, right, 0);
    }
}

}  // namespace input
