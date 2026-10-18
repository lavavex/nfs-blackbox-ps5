// NFSBlackBoxMenu.asi - the "Black Box" options menu, drawn over the game.
//
// The game's own app (the launcher) installs and starts the game; this plugin lets the player
// change its setup from inside the game. L3+R3 held for half a second opens or closes the
// menu. Its choices are written to <data>/ingame-<title>.txt (through Wine's Z: drive, the
// console's /), and the launcher applies them the next time the game starts: the performance
// overlay (profile), the Xbox 360 look (a repair with or without it) or a plain repair.
// "Save and restart" and "Quit to setup" also quit the game and ask prospero-win to start the
// app's own eboot.bin (the launcher) instead of going to Home: <runner>/return-<title>.
//
// It draws with Dear ImGui on the game's Direct3D 9 device: speed.exe's import of
// Direct3DCreate9 is redirected, CreateDevice is wrapped, and the device's Present draws the
// menu before the frame is shown; Reset releases and remakes ImGui's device objects. While the
// menu is open, XInputGetState gives the game an idle pad (a 5-byte hot-patch jump in each
// loaded xinput DLL, when its prologue is the hot-patchable one; otherwise left alone).
//
// Settings: NFSBlackBoxMenu.ini beside it ([menu] title, game, data, prefix, runner). Log:
// NFSBlackBoxMenu.log beside it.
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d9.h>
#include <xinput.h>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>

#include "imgui.h"
#include "imgui_impl_dx9.h"
#include "roboto_medium.h" // generated at build time from the pinned ImGui's misc/fonts

namespace
{
// ---- settings and log --------------------------------------------------------------------
std::string g_dir;                   // this plugin's folder, with a trailing backslash
std::string g_title = "PPSA99403";   // the game's app (the launcher's title ID)
std::string g_game = "NFSMW";        // the plan's game (installs/<prefix>/<game>.json)
std::string g_data = "Z:\\data\\prospero-blackbox";
std::string g_prefix = "nfs-blackbox"; // the plan's prefix when settings.txt names none
std::string g_runner = "Z:\\data\\prospero-win"; // prospero-win's data folder
FILE *g_log = nullptr;

void log(const char *format, ...)
{
    if (!g_log)
        return;
    va_list args;
    va_start(args, format);
    vfprintf(g_log, format, args);
    va_end(args);
    fputc('\n', g_log);
    fflush(g_log);
}

std::string ini(const char *key, const std::string &fallback)
{
    char value[512] = {};
    GetPrivateProfileStringA("menu", key, fallback.c_str(), value, sizeof value,
                             (g_dir + "NFSBlackBoxMenu.ini").c_str());
    return value;
}

bool read_text(const std::string &path, std::string &out)
{
    FILE *file = fopen(path.c_str(), "rb");
    if (!file)
        return false;
    char buffer[4096];
    out.clear();
    for (size_t n; (n = fread(buffer, 1, sizeof buffer, file)) > 0;)
        out.append(buffer, n);
    fclose(file);
    return true;
}

// "key=value" lines: the value of key, or "".
std::string line_value(const std::string &text, const std::string &key)
{
    size_t at = 0;
    while (at < text.size())
    {
        size_t end = text.find('\n', at);
        if (end == std::string::npos)
            end = text.size();
        std::string line = text.substr(at, end - at);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        if (line.compare(0, key.size() + 1, key + "=") == 0)
            return line.substr(key.size() + 1);
        at = end + 1;
    }
    return {};
}

// ---- the menu's state ----------------------------------------------------------------------
int g_overlay = 0;      // 0 off, 1 frame rate, 2 full
bool g_x360 = false;    // the Xbox 360 look
bool g_repair = false;  // repair on the next start
bool g_open = false;
bool g_saved = false;   // a choice was written this session
std::string g_status;

// What the next start would do: the in-game file when there is one, else the launcher's
// settings (overlay.<title>) and the install record ("options": ["x360"]).
void load_choices()
{
    std::string ingame, settings, record;
    read_text(g_data + "\\settings.txt", settings);
    std::string prefix = line_value(settings, "game_prefix");
    if (prefix.empty())
        prefix = g_prefix;
    std::string overlay = line_value(settings, "overlay." + g_title);
    read_text(g_data + "\\installs\\" + prefix + "\\" + g_game + ".json", record);
    g_x360 = record.find("\"x360\"") != std::string::npos;
    g_repair = false;
    if (read_text(g_data + "\\ingame-" + g_title + ".txt", ingame))
    {
        if (!line_value(ingame, "overlay").empty())
            overlay = line_value(ingame, "overlay");
        if (!line_value(ingame, "x360").empty())
            g_x360 = line_value(ingame, "x360") == "on";
        g_repair = line_value(ingame, "repair") == "1";
    }
    g_overlay = overlay == "fps" ? 1 : overlay == "full" ? 2 : 0;
    log("choices: prefix=%s overlay=%d x360=%d repair=%d", prefix.c_str(), g_overlay, g_x360, g_repair);
}

// setup: the launcher shows its screen instead of starting the game again.
bool save_choices(bool setup = false)
{
    const std::string path = g_data + "\\ingame-" + g_title + ".txt";
    const std::string part = path + ".part";
    FILE *file = fopen(part.c_str(), "wb");
    if (!file)
    {
        log("can't write %s", part.c_str());
        return false;
    }
    fprintf(file, "overlay=%s\nx360=%s\nrepair=%d\nsetup=%d\n",
            g_overlay == 1 ? "fps" : g_overlay == 2 ? "full" : "off", g_x360 ? "on" : "off",
            g_repair ? 1 : 0, setup ? 1 : 0);
    const bool ok = fclose(file) == 0 && MoveFileExA(part.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING);
    log("saved %s: %d", path.c_str(), ok);
    return ok;
}

// Quit the game, back to the launcher: the request file for prospero-win; the caller then
// closes the game's window. A game that hasn't ended 10 s later is ended.
bool g_quitting = false;

DWORD WINAPI end_late(LPVOID)
{
    Sleep(10000);
    log("quit: the game is still running, ending it");
    ExitProcess(0);
}

bool quit_to_launcher()
{
    const std::string path = g_runner + "\\return-" + g_title;
    FILE *file = fopen(path.c_str(), "wb");
    if (!file || fclose(file) != 0)
    {
        log("can't write %s", path.c_str());
        return false;
    }
    log("quit: %s written", path.c_str());
    g_quitting = true;
    CreateThread(nullptr, 0, end_late, nullptr, 0, nullptr);
    return true;
}

// ---- XInput: the real state for the menu, an idle pad for the game while it is open -------
typedef DWORD(WINAPI *GetStateFn)(DWORD, XINPUT_STATE *);
GetStateFn g_get_state = nullptr; // the original (through its trampoline when hooked)

DWORD WINAPI idle_get_state(DWORD user, XINPUT_STATE *state)
{
    const DWORD result = g_get_state ? g_get_state(user, state) : ERROR_DEVICE_NOT_CONNECTED;
    if (g_open && result == ERROR_SUCCESS && state)
        std::memset(&state->Gamepad, 0, sizeof state->Gamepad); // the game sees nothing pressed
    return result;
}

// mov edi,edi / push ebp / mov ebp,esp: the hot-patchable prologue Wine gives its exports.
bool hook_export(HMODULE module, const char *name)
{
    auto *target = reinterpret_cast<unsigned char *>(GetProcAddress(module, name));
    static const unsigned char prologue[5] = {0x8B, 0xFF, 0x55, 0x8B, 0xEC};
    if (!target || std::memcmp(target, prologue, 5) != 0)
    {
        log("%s: not hooked (prologue %02x %02x %02x %02x %02x)", name, target ? target[0] : 0,
            target ? target[1] : 0, target ? target[2] : 0, target ? target[3] : 0, target ? target[4] : 0);
        return false;
    }
    // The trampoline: the same five bytes, then a jump back past them.
    auto *trampoline = static_cast<unsigned char *>(
        VirtualAlloc(nullptr, 16, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (!trampoline)
        return false;
    std::memcpy(trampoline, target, 5);
    trampoline[5] = 0xE9;
    *reinterpret_cast<int *>(trampoline + 6) = static_cast<int>((target + 5) - (trampoline + 10));
    DWORD old = 0;
    if (!VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old))
        return false;
    if (!g_get_state)
        g_get_state = reinterpret_cast<GetStateFn>(trampoline);
    target[0] = 0xE9;
    *reinterpret_cast<int *>(target + 1) =
        static_cast<int>(reinterpret_cast<unsigned char *>(&idle_get_state) - (target + 5));
    VirtualProtect(target, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), target, 5);
    return true;
}

GetStateFn reader()
{
    static GetStateFn plain = nullptr;
    if (g_get_state)
        return g_get_state;
    if (!plain)
        for (const char *name : {"xinput1_3.dll", "xinput1_4.dll", "xinput9_1_0.dll"})
            if (HMODULE module = LoadLibraryA(name))
                if ((plain = reinterpret_cast<GetStateFn>(GetProcAddress(module, "XInputGetState"))))
                    break;
    return plain;
}

void hook_xinput()
{
    static bool done = false;
    if (done)
        return;
    done = true;
    for (const char *name : {"xinput1_3.dll", "xinput1_4.dll", "xinput9_1_0.dll", "xinput1_2.dll", "xinput1_1.dll"})
        if (HMODULE module = GetModuleHandleA(name))
            log("%s XInputGetState hooked=%d", name, hook_export(module, "XInputGetState"));
}

// ---- the menu --------------------------------------------------------------------------------
bool g_imgui = false;
HWND g_window = nullptr;
WORD g_buttons = 0;     // last frame's buttons
DWORD g_combo_since = 0; // when L3+R3 went down (0: not held)

void feed_pad(ImGuiIO &io)
{
    XINPUT_STATE state{};
    GetStateFn get = reader();
    const bool connected = get && get(0, &state) == ERROR_SUCCESS;
    const WORD buttons = connected ? state.Gamepad.wButtons : 0;
    // L3+R3 held 0.5 s: open or close.
    const WORD combo = XINPUT_GAMEPAD_LEFT_THUMB | XINPUT_GAMEPAD_RIGHT_THUMB;
    if ((buttons & combo) == combo)
    {
        if (!g_combo_since)
            g_combo_since = GetTickCount();
        else if (g_combo_since != 1 && GetTickCount() - g_combo_since >= 500)
        {
            g_open = !g_open;
            g_combo_since = 1; // once per hold
            if (g_open)
            {
                load_choices();
                g_status.clear();
            }
            log("menu %s", g_open ? "opened" : "closed");
        }
    }
    else
        g_combo_since = 0;
    if (!g_open)
    {
        g_buttons = buttons;
        return;
    }
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    const struct
    {
        WORD bit;
        ImGuiKey key;
    } map[] = {{XINPUT_GAMEPAD_DPAD_UP, ImGuiKey_GamepadDpadUp},
               {XINPUT_GAMEPAD_DPAD_DOWN, ImGuiKey_GamepadDpadDown},
               {XINPUT_GAMEPAD_DPAD_LEFT, ImGuiKey_GamepadDpadLeft},
               {XINPUT_GAMEPAD_DPAD_RIGHT, ImGuiKey_GamepadDpadRight},
               {XINPUT_GAMEPAD_A, ImGuiKey_GamepadFaceDown},   // Cross: activate
               {XINPUT_GAMEPAD_B, ImGuiKey_GamepadFaceRight}}; // Circle: back
    for (const auto &entry : map)
        if ((buttons & entry.bit) != (g_buttons & entry.bit))
            io.AddKeyEvent(entry.key, (buttons & entry.bit) != 0);
    g_buttons = buttons;
}

void init_imgui(IDirect3DDevice9 *device)
{
    D3DDEVICE_CREATION_PARAMETERS creation{};
    device->GetCreationParameters(&creation);
    g_window = creation.hFocusWindow;
    D3DVIEWPORT9 viewport{};
    device->GetViewport(&viewport);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr;
    io.LogFilename = nullptr;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.MouseDrawCursor = false;
    // One press, one step: a held D-pad repeats after 0.45 s, then 6 steps a second.
    io.KeyRepeatDelay = 0.45f;
    io.KeyRepeatRate = 0.16f;
    // Sized for the back buffer: about 2.5% of its height per line (54 px at 2160).
    const float scale = viewport.Height / 720.0f;
    ImFontConfig config;
    config.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryCompressedTTF(roboto_medium_compressed_data, roboto_medium_compressed_size,
                                             17.0f * scale, &config);
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowRounding = 10.0f;
    style.FrameRounding = 6.0f;
    style.WindowPadding = ImVec2(18, 16);
    style.ItemSpacing = ImVec2(10, 10);
    style.FramePadding = ImVec2(10, 6);
    style.ScaleAllSizes(scale);
    ImVec4 *c = style.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.07f, 0.07f, 0.08f, 0.94f);
    c[ImGuiCol_TitleBg] = c[ImGuiCol_TitleBgActive] = ImVec4(0.96f, 0.64f, 0.02f, 1.0f);
    c[ImGuiCol_Text] = ImVec4(0.95f, 0.94f, 0.91f, 1.0f);
    c[ImGuiCol_CheckMark] = ImVec4(0.96f, 0.64f, 0.02f, 1.0f);
    c[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = c[ImGuiCol_FrameBgActive] = ImVec4(0.28f, 0.26f, 0.22f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.22f, 0.22f, 0.24f, 1.0f);
    c[ImGuiCol_ButtonHovered] = c[ImGuiCol_ButtonActive] = ImVec4(0.96f, 0.64f, 0.02f, 1.0f);
    c[ImGuiCol_NavHighlight] = ImVec4(0.96f, 0.64f, 0.02f, 1.0f);
    ImGui_ImplDX9_Init(device);
    g_imgui = true;
    log("imgui ready: viewport %lux%lu scale %.2f window %p", viewport.Width, viewport.Height, scale,
        static_cast<void *>(g_window));
}

void draw_menu()
{
    ImGuiIO &io = ImGui::GetIO();
    const ImVec2 size(io.DisplaySize.x * 0.36f, 0.0f);
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always,
                            ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(size, ImGuiCond_Always);
    ImGui::Begin("Black Box: Most Wanted", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings);
    ImGui::TextDisabled("Performance overlay");
    ImGui::RadioButton("Off##overlay", &g_overlay, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Frame rate", &g_overlay, 1);
    ImGui::SameLine();
    ImGui::RadioButton("Full", &g_overlay, 2);
    ImGui::Spacing();
    ImGui::TextDisabled("Xbox 360 look");
    int look = g_x360 ? 1 : 0;
    ImGui::RadioButton("Off##x360", &look, 0);
    ImGui::SameLine();
    ImGui::RadioButton("On (about 3.7 GB to download)", &look, 1);
    g_x360 = look == 1;
    ImGui::Spacing();
    ImGui::Checkbox("Repair the install on the next start", &g_repair);
    ImGui::Spacing();
    ImGui::TextWrapped("Changes apply when the game starts again: restart now, or save them for "
                       "the next start.");
    ImGui::Spacing();
    if (g_quitting)
        ImGui::TextUnformatted("Quitting...");
    else
    {
        if (ImGui::Button("Save and restart"))
        {
            g_saved = save_choices() && quit_to_launcher();
            g_status = g_saved ? "" : "Couldn't save: see NFSBlackBoxMenu.log";
            if (g_saved) // what Alt+F4 becomes: the game's own way out
                PostMessageA(g_window, WM_SYSCOMMAND, SC_CLOSE, 0);
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            g_saved = save_choices();
            g_status = g_saved ? "" : "Couldn't save: see NFSBlackBoxMenu.log";
            if (g_saved)
                g_open = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Quit to setup"))
        {
            g_saved = save_choices(true) && quit_to_launcher();
            g_status = g_saved ? "" : "Couldn't save: see NFSBlackBoxMenu.log";
            if (g_saved) // what Alt+F4 becomes: the game's own way out
                PostMessageA(g_window, WM_SYSCOMMAND, SC_CLOSE, 0);
        }
        ImGui::SameLine();
        if (ImGui::Button("Close") || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight, false))
            g_open = false;
    }
    if (!g_status.empty())
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.3f, 1.0f), "%s", g_status.c_str());
    ImGui::TextDisabled("L3 + R3: open or close this menu");
    ImGui::End();
}

void frame(IDirect3DDevice9 *device)
{
    if (!g_imgui)
        init_imgui(device);
    hook_xinput();
    ImGuiIO &io = ImGui::GetIO();
    feed_pad(io);
    if (!g_open)
        return;
    D3DVIEWPORT9 viewport{};
    device->GetViewport(&viewport);
    io.DisplaySize = ImVec2(static_cast<float>(viewport.Width), static_cast<float>(viewport.Height));
    // The real time between frames (the game runs at ~110 fps, not 60): ImGui's key repeat
    // counts in it. A long gap (the menu was closed) counts as one frame.
    static LARGE_INTEGER frequency{}, last{};
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    if (!frequency.QuadPart)
        QueryPerformanceFrequency(&frequency);
    const float elapsed = last.QuadPart ? static_cast<float>(now.QuadPart - last.QuadPart) /
                                              static_cast<float>(frequency.QuadPart)
                                        : 0.0f;
    last = now;
    io.DeltaTime = elapsed > 0.0f && elapsed < 0.25f ? elapsed : 1.0f / 60.0f;
    ImGui_ImplDX9_NewFrame();
    ImGui::NewFrame();
    draw_menu();
    ImGui::EndFrame();
    ImGui::Render();
    if (SUCCEEDED(device->BeginScene()))
    {
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
        device->EndScene();
    }
}

// ---- Direct3D 9 hooks -----------------------------------------------------------------------
typedef HRESULT(WINAPI *PresentFn)(IDirect3DDevice9 *, const RECT *, const RECT *, HWND, const RGNDATA *);
typedef HRESULT(WINAPI *ResetFn)(IDirect3DDevice9 *, D3DPRESENT_PARAMETERS *);
typedef HRESULT(WINAPI *CreateDeviceFn)(IDirect3D9 *, UINT, D3DDEVTYPE, HWND, DWORD, D3DPRESENT_PARAMETERS *,
                                         IDirect3DDevice9 **);
typedef IDirect3D9 *(WINAPI *CreateFn)(UINT);
PresentFn g_present = nullptr;
ResetFn g_reset = nullptr;
CreateDeviceFn g_create_device = nullptr;
CreateFn g_create = nullptr;

HRESULT WINAPI present(IDirect3DDevice9 *device, const RECT *source, const RECT *dest, HWND window,
                       const RGNDATA *dirty)
{
    frame(device);
    return g_present(device, source, dest, window, dirty);
}

HRESULT WINAPI reset(IDirect3DDevice9 *device, D3DPRESENT_PARAMETERS *parameters)
{
    if (g_imgui)
        ImGui_ImplDX9_InvalidateDeviceObjects();
    const HRESULT result = g_reset(device, parameters);
    if (g_imgui && SUCCEEDED(result))
        ImGui_ImplDX9_CreateDeviceObjects();
    log("Reset: 0x%08lx", static_cast<unsigned long>(result));
    return result;
}

template <typename T> void patch_slot(void **table, int index, T replacement, T &original)
{
    DWORD old = 0;
    if (!VirtualProtect(&table[index], sizeof(void *), PAGE_READWRITE, &old))
        return;
    if (!original)
        original = reinterpret_cast<T>(table[index]);
    table[index] = reinterpret_cast<void *>(replacement);
    VirtualProtect(&table[index], sizeof(void *), old, &old);
}

HRESULT WINAPI create_device(IDirect3D9 *d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
                             D3DPRESENT_PARAMETERS *parameters, IDirect3DDevice9 **device)
{
    const HRESULT result = g_create_device(d3d, adapter, type, window, flags, parameters, device);
    log("CreateDevice: 0x%08lx %ux%u", static_cast<unsigned long>(result),
        parameters ? parameters->BackBufferWidth : 0, parameters ? parameters->BackBufferHeight : 0);
    if (SUCCEEDED(result) && device && *device)
    {
        void **table = *reinterpret_cast<void ***>(*device);
        patch_slot(table, 17, &present, g_present); // Present
        patch_slot(table, 16, &reset, g_reset);     // Reset
    }
    return result;
}

IDirect3D9 *WINAPI direct3d_create9(UINT version)
{
    IDirect3D9 *d3d = g_create(version);
    log("Direct3DCreate9: %p", static_cast<void *>(d3d));
    if (d3d)
        patch_slot(*reinterpret_cast<void ***>(d3d), 16, &create_device, g_create_device); // CreateDevice
    return d3d;
}

// speed.exe's import of d3d9.dll!Direct3DCreate9, pointed at ours.
bool hook_import()
{
    auto *base = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    auto *dos = reinterpret_cast<IMAGE_DOS_HEADER *>(base);
    auto *nt = reinterpret_cast<IMAGE_NT_HEADERS *>(base + dos->e_lfanew);
    const auto &dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    for (auto *imp = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(base + dir.VirtualAddress); imp->Name; ++imp)
    {
        if (_stricmp(reinterpret_cast<char *>(base + imp->Name), "d3d9.dll") != 0)
            continue;
        auto *names = reinterpret_cast<IMAGE_THUNK_DATA *>(base + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
        auto *slots = reinterpret_cast<IMAGE_THUNK_DATA *>(base + imp->FirstThunk);
        for (; names->u1.AddressOfData; ++names, ++slots)
        {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal))
                continue;
            auto *by_name = reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(base + names->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<char *>(by_name->Name), "Direct3DCreate9") != 0)
                continue;
            void **slot = reinterpret_cast<void **>(&slots->u1.Function);
            patch_slot(slot, 0, &direct3d_create9, g_create);
            return true;
        }
    }
    return false;
}
} // namespace

extern "C" BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;
    DisableThreadLibraryCalls(instance);
    char path[MAX_PATH] = {};
    GetModuleFileNameA(instance, path, MAX_PATH);
    g_dir = path;
    g_dir.erase(g_dir.find_last_of("\\/") + 1);
    g_log = fopen((g_dir + "NFSBlackBoxMenu.log").c_str(), "w");
    g_title = ini("title", g_title);
    g_game = ini("game", g_game);
    g_data = ini("data", g_data);
    g_prefix = ini("prefix", g_prefix);
    g_runner = ini("runner", g_runner);
    log("NFSBlackBoxMenu: title=%s game=%s data=%s prefix=%s runner=%s", g_title.c_str(), g_game.c_str(),
        g_data.c_str(), g_prefix.c_str(), g_runner.c_str());
    log("Direct3DCreate9 import hooked=%d", hook_import());
    return TRUE;
}
