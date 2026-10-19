#include "config_menu.h"
#include "app/app.h"
#include <imgui.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <vector>

namespace cfgmenu {
namespace {

std::atomic<bool> s_open = false;

struct Mode {
    uint32_t w, h;
    bool declared;  // el monitor lo declara: sirve en pantalla completa
};
std::vector<Mode> s_modes;

// escritorio cacheado: app::Desktop() llama a EnumDisplaySettingsW y NO debe correr por frame
app::Mode s_desk{0, 0};
bool s_desk_ok = false;

void RefreshDesk() {
    s_desk = app::Desktop();
    s_desk_ok = true;
}

// modos que el monitor declara + los tipicos, para que siempre exista 4:3 y 16:9 en cualquier equipo
void BuildModes() {
    if (!s_modes.empty()) return;
    auto add = [](uint32_t w, uint32_t h, bool declared) {
        for (Mode& m : s_modes)
            if (m.w == w && m.h == h) {
                m.declared |= declared;
                return;
            }
        s_modes.push_back({w, h, declared});
    };
#ifdef _WIN32
    HWND hwnd = (HWND)app::Window();
    HMONITOR mon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    GetMonitorInfoW(mon, &mi);
    for (DWORD i = 0;; i++) {
        DEVMODEW dm{};
        dm.dmSize = sizeof(dm);
        if (!EnumDisplaySettingsW(mi.szDevice, i, &dm)) break;
        uint32_t w = dm.dmPelsWidth, h = dm.dmPelsHeight;
        if (w < 640 || h < 480 || w > 7680 || h > 4320) continue;
        add(w, h, true);
    }
#endif
    for (Mode m : {Mode{640, 480, false}, {800, 600, false}, {1024, 768, false}, {1280, 720, false},
                   {1280, 960, false}, {1366, 768, false}, {1600, 1200, false}, {1920, 1080, false},
                   {2560, 1440, false}, {3840, 2160, false}})
        add(m.w, m.h, false);
    std::sort(s_modes.begin(), s_modes.end(), [](const Mode& a, const Mode& b) {
        return a.h != b.h ? a.h > b.h : a.w > b.w;
    });
}

const char* AspectTag(uint32_t w, uint32_t h) {
    if (!h) return "";
    if (w * 3 == h * 4) return "  4:3";
    if (w * 9 == h * 16) return "  16:9";
    if (w * 10 == h * 16) return "  16:10";
    return "";
}

void PushStyle() {
    ImGuiStyle& st = ImGui::GetStyle();
    st.WindowRounding = 10.0f;
    st.FrameRounding = 5.0f;
    st.GrabRounding = 4.0f;
    st.WindowBorderSize = 1.0f;
    st.FrameBorderSize = 0.0f;
    st.WindowPadding = ImVec2(18, 14);
    st.ItemSpacing = ImVec2(12, 9);
    st.ScrollbarRounding = 6.0f;
    const ImVec4 accent(0.82f, 0.12f, 0.12f, 1.0f);
    const ImVec4 accent_hi(1.0f, 0.25f, 0.25f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.07f, 0.06f, 0.07f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.55f, 0.08f, 0.08f, 0.9f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.92f, 0.90f, 0.90f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.16f, 0.13f, 0.14f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.24f, 0.16f, 0.17f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.30f, 0.17f, 0.18f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.30f, 0.10f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.13f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, accent_hi);
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.35f, 0.10f, 0.10f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.48f, 0.13f, 0.13f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, accent_hi);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, accent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, accent);
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, accent_hi);
    ImGui::PushStyleColor(ImGuiCol_Separator, ImVec4(0.45f, 0.10f, 0.10f, 0.7f));
    (void)accent;
}

void PopStyle() { ImGui::PopStyleColor(16); }

void Section(const char* name) {
    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.95f, 0.30f, 0.25f, 1.0f), "%s", name);
    ImGui::Separator();
    ImGui::Spacing();
}

// etiqueta arriba del widget: con widgets a ancho completo la etiqueta al lado se corta
void Label(const char* text) { ImGui::TextUnformatted(text); }

bool s_dirty = false;

}  // namespace

void Draw(double fps) {
    (void)fps;
    if (!s_open.load(std::memory_order_relaxed)) return;
    BuildModes();
    if (!s_desk_ok) RefreshDesk();

    auto& st = app::Get();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(620, 0), ImGuiCond_Always);
    PushStyle();

    static const char* kModeNames[3] = {"Pantalla completa", "Sin bordes (recomendado)", "Ventana"};
    static const char* kScaleNames[5] = {"Nativa (x1)", "x2", "x4", "x6", "x8"};
    static const uint32_t kScaleVals[5] = {1, 2, 4, 6, 8};

    if (ImGui::Begin("Bloody Roar Extreme  -  Configuracion", nullptr,
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(ImVec4(0.95f, 0.30f, 0.25f, 1.0f), "BLOODY ROAR EXTREME");
        ImGui::SameLine();
        ImGui::TextDisabled("- Configuracion (F1)");

        Section("PANTALLA");

        int mode = int(st.display_mode);
        Label("Modo de pantalla");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##mode", kModeNames[mode < 3 ? mode : 1])) {
            for (int i = 0; i < 3; i++)
                if (ImGui::Selectable(kModeNames[i], i == mode)) {
                    st.display_mode = app::DisplayMode(i);
                    app::ApplyDisplay();  // se aplica solo al elegir (coalescida)
                }
            ImGui::EndCombo();
        }

        // en pantalla completa solo sirven los modos que declara el monitor:
        // los sinteticos hacen fallar ChangeDisplaySettingsExW en silencio
        const bool full = st.display_mode == app::DisplayMode::Fullscreen;
        char cur[96];
        if (st.width && st.height)
            snprintf(cur, sizeof(cur), "%ux%u%s", st.width, st.height, AspectTag(st.width, st.height));
        else
            snprintf(cur, sizeof(cur), "Automatico (escritorio %ux%u)", s_desk.width, s_desk.height);
        Label("Resolucion");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##res", cur)) {
            char desk[64];
            snprintf(desk, sizeof(desk), "Automatico (escritorio %ux%u)", s_desk.width, s_desk.height);
            if (ImGui::Selectable(desk, st.width == 0 && st.height == 0)) {
                st.width = st.height = 0;
                app::ApplyDisplay();
            }
            for (const Mode& m : s_modes) {
                if (full && !m.declared) continue;
                char label[64];
                snprintf(label, sizeof(label), "%ux%u%s", m.w, m.h, AspectTag(m.w, m.h));
                if (ImGui::Selectable(label, st.width == m.w && st.height == m.h)) {
                    st.width = m.w;
                    st.height = m.h;
                    app::ApplyDisplay();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::TextDisabled("En 'Sin bordes' la imagen se centra en el monitor;");
        ImGui::TextDisabled("las resoluciones 4:3 se ven con barras negras laterales.");

        Section("IMAGEN");

        int si = -1;
        for (int i = 0; i < 5; i++)
            if (st.render_scale == kScaleVals[i]) si = i;
        char scale_cur[40];
        if (si >= 0)
            snprintf(scale_cur, sizeof(scale_cur), "%s", kScaleNames[si]);
        else
            snprintf(scale_cur, sizeof(scale_cur), "%ux (personalizado)", st.render_scale);
        Label("Resolucion interna (calidad)");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::BeginCombo("##scale", scale_cur)) {
            for (int i = 0; i < 5; i++)
                if (ImGui::Selectable(kScaleNames[i], i == si)) {
                    st.render_scale = kScaleVals[i];
                    s_dirty = true;
                }
            ImGui::EndCombo();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        static auto saved_at = std::chrono::steady_clock::time_point{};
        if (ImGui::Button("Guardar", ImVec2(150, 0))) {
            app::Save();
            saved_at = std::chrono::steady_clock::now();
            s_dirty = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cerrar", ImVec2(110, 0))) s_open.store(false, std::memory_order_relaxed);

        if (s_dirty)
            ImGui::TextDisabled("Cambios sin guardar: pulsa Guardar.");
        else if (saved_at != std::chrono::steady_clock::time_point{} &&
                 std::chrono::steady_clock::now() - saved_at < std::chrono::seconds(3))
            ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Guardado en settings.toml");
    }
    ImGui::End();
    PopStyle();
}

void Toggle() {
    s_open.store(!s_open.load(std::memory_order_relaxed), std::memory_order_relaxed);
    if (s_open.load(std::memory_order_relaxed)) RefreshDesk();  // al abrir: refresca el escritorio actual
}
void Close() { s_open.store(false, std::memory_order_relaxed); }
bool Open() { return s_open.load(std::memory_order_relaxed); }

bool Blocked() {
    if (s_open.load(std::memory_order_relaxed)) return true;
#ifdef _WIN32
    HWND me = (HWND)app::Window();
    if (!me) return false;
    HWND fg = GetForegroundWindow();
    // sin foco el teclado global no debe llegar al juego (GetAsyncKeyState es de todo el sistema)
    return fg != me && (!fg || GetAncestor(fg, GA_ROOT) != me);
#else
    return false;
#endif
}

}  // namespace cfgmenu
