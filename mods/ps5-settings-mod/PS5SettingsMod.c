/*
 * PS5SettingsMod.asi - the game's own Options menu holds the PS5 app's settings.
 *
 * The Online Options page (its menu entry and title renamed "PS5 Options") is replaced by:
 *   Performance overlay   Off / Frame rate / Full
 *   Xbox 360 look         Off / On (3.7 GB download)
 *   Repair on next start  Off / On
 * and the button bar's Triangle (Defaults on the other pages) becomes "Restart and apply".
 * The Video page is not modified. Square can't be used: Options.fng (this page's package) maps
 * no PAD_BUTTON4, so Square never reaches the game's code here (OptionsPCDisplay.fng maps it to
 * its advanced message 0x9CDD83CA). Triangle (PAD_BUTTON5) reaches the options screen's handler
 * 0x545750, which opens the reset-to-defaults box (0x545AD4 pushes its text, 0x8AEF5AE8) and
 * resets on its Yes (message 0xD05FC3A3, 0x545BF0). On this page both are ours: the box asks
 * to restart and apply, and Yes does it. Calling through the screen's vtable (0x89CF60) is not
 * how it is reached: wrappers put there were never called. The "Defaults" text (0x00CA369E,
 * shared by the Audio, Gameplay and Player pages) says "Restart and apply" only while this page
 * is built: every page build (0x545C50, its call at 0x545C53) puts it back first.
 *
 * Each change is written to <data>\ingame-<title>.txt (overlay=, x360=, repair=), the
 * file the game's PS5 app (its launcher) reads at its next start and applies: the overlay
 * into the profile, a repair for the 360 look or when asked for, then the game. Restart also
 * writes <runner>\return-<title>, which makes prospero-win start the app's launcher instead
 * of going to Home, and closes the game.
 * The choices shown come from that file, else the launcher's settings.txt (overlay.<title>,
 * game_prefix) and the install record (installs\<prefix>\<game>.json, "x360").
 *
 * speed.exe must be the DRM-free 1.3 build (6,029,312 bytes). Every patched byte is checked
 * first; a mismatch is logged and leaves the game alone. How the exe was read: REPORT.md and
 * fe_options.c beside this file (the work this mod started from, FEngPerformanceOverlay).
 *
 * Settings: PS5SettingsMod.ini beside it, [settings] title, game, data, runner, prefix (each
 * optional). Log: PS5SettingsMod.log beside it, the run before as PS5SettingsMod.previous.log.
 */
#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define BUILD_ONLINE     0x52A290
#define ONLINE_CALL      0x545DEA
#define BUILD_MENU       0x528F30
#define MENU_CALL        0x5452C4
#define CLEAR_ROWS       0x588C90
#define RESOLVE_NUMBERED 0x573CC0
#define INIT_TOGGLE      0x589300
#define ADD_TOGGLE       0x588570
#define SET_LANG_HASH    0x515C00
#define GAME_MALLOC      0x652AD0
#define RES_VTABLE       0x89BB04
#define NAME_TEMPLATE    0x8A1944
#define LANG_TABLE       (*(void **)0x91CF80)
#define LANG_COUNT       (*(unsigned *)0x91CF78)

/* FEHashUpper of the pad messages (start 0xFFFFFFFF, hash = c + 33 * hash). */
#define PAD_LEFT   0x9120409Eu
#define PAD_RIGHT  0xB5971BF1u
#define PAD_BUTTON5 0xC519BFC4u /* Triangle: the bar's Defaults */
#define H_DEFAULTS 0x00CA369Eu  /* "Defaults", English.bin */
/* In the options screen's handler (0x545750) and page build (0x545C50). */
#define DIALOG_TEXT_SITE 0x545AD4 /* push 0x8AEF5AE8, the reset box's text */
#define DIALOG_TEXT_NEXT 0x545AD9
#define DEFAULTS_YES_SITE 0x545BF0 /* mov ecx, [0x91CF90]: the reset, on the box's Yes */
#define DEFAULTS_YES_NEXT 0x545BF6
#define HANDLER_DONE     0x545C2E /* the handler's way out */
#define REBUILD_CALL     0x545C53
#define REBUILD_CLEAR    0x588C90
#define STR_(x) #x
#define STR(x) STR_(x)
#define H_RESET_ASK 0x8AEF5AE8u /* "Are you sure you want to reset to defaults?..." */
/* FEHashUpper of names in no 1.3 language file and not in the exe. */
#define H_OVERLAY  0x65CD36F0u /* BB_PERF_OVERLAY */
#define H_PERF_OFF 0xCE4EC5C9u /* BB_PERF_OFF */
#define H_PERF_FPS 0xCE4EA0D7u /* BB_PERF_FPS */
#define H_PERF_FULL 0x9822D061u /* BB_PERF_FULL */
#define H_X360     0x8D1E87B3u /* BB_X360 */
#define H_X360_ON  0x2AC8ED6Fu /* BB_X360_ON */
#define H_REPAIR   0x423499A5u /* BB_REPAIR */
#define H_OFF      0x51D9FF5Du /* BB_OFF */
#define H_ON       0x027AF83Fu /* BB_ON */
#define H_RESTART_ASK 0xAD8AE445u /* BB_RESTART_ASK */
/* Already in the game's table: their text becomes "PS5 Options". */
#define H_OL_OPTIONS       0x7D347E05u /* the menu entry */
#define H_OL_OPTIONS_TITLE 0x22B1C2E6u /* the page title */

#define FILE_CAP 8192
#define LANG_CAP 65536

typedef struct LangRecord { unsigned hash; const char *text; } LangRecord;
typedef void (__attribute__((thiscall)) *InitToggle)(void *widget, int enabled);
typedef int (__attribute__((thiscall)) *AddToggle)(void *screen, void *widget, int unused);
typedef void *(__attribute__((thiscall)) *ResolveNumbered)(void *screen, const char *templ);
typedef void (__attribute__((cdecl)) *SetLangHash)(void *obj, unsigned hash);
typedef void *(__attribute__((cdecl)) *GameMalloc)(unsigned bytes);
typedef void (__attribute__((thiscall)) *WidgetNotify)(void *widget, unsigned hash);
typedef void (__attribute__((thiscall)) *WidgetDraw)(void *widget);
typedef void (__attribute__((thiscall)) *ClearRows)(void *screen);

static const unsigned char kOnlineCall[5] = { 0xE8, 0xA1, 0x44, 0xFE, 0xFF };
static const unsigned char kMenuCall[5] = { 0xE8, 0x67, 0x3C, 0xFE, 0xFF };
static const unsigned char kRebuildCall[5] = { 0xE8, 0x38, 0x30, 0x04, 0x00 };
static const unsigned char kDialogText[5] = { 0x68, 0xE8, 0x5A, 0xEF, 0x8A };
static const unsigned char kDefaultsYes[6] = { 0x8B, 0x0D, 0x90, 0xCF, 0x91, 0x00 };
static const char kMenuName[] = "PS5 Options";
static const char kOverlayText[] = "Performance overlay";
/* Merged into the game's table in hash order (sorted once at load). */
static LangRecord g_ours[] = {
    { H_OL_OPTIONS, kMenuName },        { H_OL_OPTIONS_TITLE, kMenuName },
    { H_OVERLAY, kOverlayText },        { H_PERF_OFF, "Off" },
    { H_PERF_FPS, "Frame rate" },       { H_PERF_FULL, "Full" },
    { H_X360, "Xbox 360 look" },        { H_X360_ON, "On (3.7 GB download)" },
    { H_REPAIR, "Repair on next start" },

    { H_OFF, "Off" },
    { H_ON, "On" },
    { H_RESTART_ASK, "Restart the game now to apply these settings?^"
                     "Progress since your last save is lost. An Xbox 360 look change or a "
                     "repair is done before the game starts again." },
};
#define OURS_COUNT (sizeof g_ours / sizeof g_ours[0])

/* No Cursor row: the touchpad moves the game's cursor always (owner, 2026-10-18: with it off,
 * Windows' cursor still showed in cutscenes). */
enum { ROW_OVERLAY, ROW_X360, ROW_REPAIR, ROWS };
static const unsigned kRowLabel[ROWS] = { H_OVERLAY, H_X360, H_REPAIR };
static const char kRestartText[] = "Restart and apply";

/* Where things are (PS5SettingsMod.ini can say otherwise). */
static char g_title[32] = "PPSA99403";              /* the game's PS5 app */
static char g_game[32] = "NFSMW";                   /* the plan's game */
static char g_data[MAX_PATH] = "Z:\\data\\nfs-blackbox";
static char g_runner[MAX_PATH] = "Z:\\data\\prospero-win";
static char g_prefix[64] = "nfs-blackbox";          /* the plan's prefix */

static FILE *log_file;
static int g_overlay;   /* 0 off, 1 frame rate, 2 full */
static int g_x360;      /* 0 off, 1 on */
static int g_repair;    /* 0, 1 */
static int g_quitting;  /* the game is closing */
static void *g_vt[ROWS][15];
static LangRecord *g_lang_installed;
static int g_unknown_logged;

static void klog(const char *format, ...)
{
    va_list args;
    if (!log_file)
        return;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    fputc('\n', log_file);
    fflush(log_file);
}

/* ---- the files ---------------------------------------------------------------------------- */

/* The whole file, NUL-terminated, when it is at most FILE_CAP bytes. */
static int read_small(const char *path, char *out)
{
    FILE *file = fopen(path, "rb");
    size_t n;
    int extra;
    if (!file)
        return 0;
    n = fread(out, 1, FILE_CAP, file);
    extra = fgetc(file);
    fclose(file);
    if (extra != EOF)
        return 0;
    out[n] = 0;
    return 1;
}

/* The value of "key=" on its own line, without a trailing CR; "" when there is none. */
static void line_value(const char *text, const char *key, char *out, size_t size)
{
    const size_t len = strlen(key);
    const char *line = text;
    out[0] = 0;
    while (line && *line) {
        const char *end = strchr(line, '\n');
        size_t n = end ? (size_t)(end - line) : strlen(line);
        if (n > len && memcmp(line, key, len) == 0 && line[len] == '=') {
            size_t v = n - len - 1;
            if (v && line[len + 1 + v - 1] == '\r')
                v--;
            if (v >= size)
                v = size - 1;
            memcpy(out, line + len + 1, v);
            out[v] = 0;
            return;
        }
        line = end ? end + 1 : NULL;
    }
}

/* What the next start would do: the in-game file when there is one, else the launcher's
 * settings and the install record. */
static void load_choices(void)
{
    static char text[FILE_CAP + 1];
    char path[MAX_PATH * 2], prefix[64], value[32], key[64];

    snprintf(prefix, sizeof prefix, "%s", g_prefix);
    g_overlay = 0;
    g_x360 = 0;
    g_repair = 0;
    snprintf(path, sizeof path, "%s\\settings.txt", g_data);
    if (read_small(path, text)) {
        line_value(text, "game_prefix", value, sizeof value);
        if (value[0])
            snprintf(prefix, sizeof prefix, "%s", value);
        snprintf(key, sizeof key, "overlay.%s", g_title);
        line_value(text, key, value, sizeof value);
        g_overlay = strcmp(value, "fps") == 0 ? 1 : strcmp(value, "full") == 0 ? 2 : 0;
    }
    snprintf(path, sizeof path, "%s\\installs\\%s\\%s.json", g_data, prefix, g_game);
    if (read_small(path, text))
        g_x360 = strstr(text, "\"x360\"") != NULL;
    snprintf(path, sizeof path, "%s\\ingame-%s.txt", g_data, g_title);
    if (read_small(path, text)) {
        line_value(text, "overlay", value, sizeof value);
        if (value[0])
            g_overlay = strcmp(value, "fps") == 0 ? 1 : strcmp(value, "full") == 0 ? 2 : 0;
        line_value(text, "x360", value, sizeof value);
        if (value[0])
            g_x360 = strcmp(value, "on") == 0;
        line_value(text, "repair", value, sizeof value);
        g_repair = strcmp(value, "1") == 0;
    }
    klog("choices: prefix=%s overlay=%d x360=%d repair=%d", prefix, g_overlay, g_x360, g_repair);
}

/* The in-game file, whole, written beside and moved over (never half-written). setup: the
 * launcher stays on its page instead of starting the game again. */
static int save_choices(int setup)
{
    static const char *const overlay[] = { "off", "fps", "full" };
    char path[MAX_PATH * 2], part[MAX_PATH * 2 + 8];
    FILE *file;
    int ok;

    snprintf(path, sizeof path, "%s\\ingame-%s.txt", g_data, g_title);
    snprintf(part, sizeof part, "%s.part", path);
    file = fopen(part, "wb");
    if (!file) {
        klog("can't write %s", part);
        return 0;
    }
    fprintf(file, "overlay=%s\nx360=%s\nrepair=%d\nsetup=%d\n", overlay[g_overlay],
            g_x360 ? "on" : "off", g_repair, setup);
    ok = fclose(file) == 0 && MoveFileExA(part, path, MOVEFILE_REPLACE_EXISTING);
    klog("saved overlay=%s x360=%d repair=%d setup=%d: %s", overlay[g_overlay],
         g_x360, g_repair, setup, ok ? "ok" : "failed");
    return ok;
}

/* ---- quitting to the launcher ------------------------------------------------------------- */

static BOOL CALLBACK close_window(HWND window, LPARAM unused)
{
    DWORD process = 0;
    (void)unused;
    GetWindowThreadProcessId(window, &process);
    if (process == GetCurrentProcessId() && IsWindowVisible(window)) {
        PostMessageA(window, WM_SYSCOMMAND, SC_CLOSE, 0); /* what Alt+F4 becomes */
        return FALSE;
    }
    return TRUE;
}

static DWORD WINAPI end_late(LPVOID unused)
{
    (void)unused;
    Sleep(10000);
    klog("quit: the game is still running, ending it");
    ExitProcess(0);
}

/* The request for prospero-win (start the app's eboot.bin, not Home), then the game's own
 * way out; a game still running 10 s later is ended. */
static void quit_to_launcher(int setup)
{
    char path[MAX_PATH * 2];
    FILE *file;

    if (g_quitting || !save_choices(setup))
        return;
    snprintf(path, sizeof path, "%s\\return-%s", g_runner, g_title);
    file = fopen(path, "wb");
    if (!file || fclose(file) != 0) {
        klog("can't write %s; not quitting", path);
        return;
    }
    klog("quit to %s: %s written", setup ? "setup" : "restart", path);
    g_quitting = 1;
    EnumWindows(close_window, 0);
    CreateThread(NULL, 0, end_late, NULL, 0, NULL);
}

/* ---- the game's text ---------------------------------------------------------------------- */

static char *lookup_language(unsigned hash)
{
    char *text;
    __asm__ volatile (
        "movl %1, %%edx\n\t"
        "movl $0x56BB80, %%eax\n\t"
        "call *%%eax\n\t"
        : "=a"(text)
        : "r"(hash)
        : "ecx", "edx", "cc", "memory"
    );
    return text;
}

static int language_ready(void)
{
    unsigned i;
    for (i = 0; i < OURS_COUNT; i++)
        if (lookup_language(g_ours[i].hash) != g_ours[i].text)
            return 0;
    return 1;
}

static int renamed(unsigned hash)
{
    return hash == H_OL_OPTIONS || hash == H_OL_OPTIONS_TITLE;
}

/* The game's sorted table and ours, merged into a new one (the game's malloc). The two
 * renamed records take our text; a new hash the game already has keeps the game's. */
static void ensure_language(void)
{
    LangRecord *base, *neu;
    unsigned count, i, j, k;

    if (language_ready())
        return;
    base = (LangRecord *)LANG_TABLE;
    count = base ? LANG_COUNT : 0;
    if (base && base == g_lang_installed) {
        klog("language table was already replaced and a string is missing; rows will not be added");
        return;
    }
    if (count > LANG_CAP) {
        klog("language table is unexpectedly large; strings not added");
        return;
    }
    neu = ((GameMalloc)GAME_MALLOC)((count + OURS_COUNT) * sizeof(LangRecord));
    if (!neu) {
        klog("could not allocate the language table; strings not added");
        return;
    }
    i = j = k = 0;
    while (i < count && j < OURS_COUNT) {
        if (base[i].hash == g_ours[j].hash) {
            neu[k++] = renamed(g_ours[j].hash) ? g_ours[j] : base[i];
            klog(renamed(g_ours[j].hash) ? "renamed hash %08X to PS5 Options"
                                         : "hash %08X is already in the language table; left it",
                 g_ours[j].hash);
            i++;
            j++;
        } else if (base[i].hash < g_ours[j].hash)
            neu[k++] = base[i++];
        else
            neu[k++] = g_ours[j++];
    }
    while (i < count)
        neu[k++] = base[i++];
    while (j < OURS_COUNT)
        neu[k++] = g_ours[j++];
    LANG_TABLE = neu;
    LANG_COUNT = k;
    g_lang_installed = neu;
    klog("strings added to the language table");
}

/* ---- the rows ----------------------------------------------------------------------------- */

static unsigned row_value(int row)
{
    static const unsigned overlay[] = { H_PERF_OFF, H_PERF_FPS, H_PERF_FULL };
    switch (row) {
    case ROW_OVERLAY: return overlay[g_overlay];
    case ROW_X360: return g_x360 ? H_X360_ON : H_OFF;
    default: return g_repair ? H_ON : H_OFF;
    }
}

static void row_draw(int row, void *widget)
{
    void *label = *(void **)((char *)widget + 0x2C);
    void *value = *(void **)((char *)widget + 0x30);
    if (label)
        ((SetLangHash)SET_LANG_HASH)(label, kRowLabel[row]);
    if (value)
        ((SetLangHash)SET_LANG_HASH)(value, row_value(row));
}

/* Left and right change a value row and save it. Other messages change nothing; the first
 * few are logged. */
static void row_act(int row, void *widget, unsigned hash)
{
    void **vt = *(void ***)widget;
    const int step = hash == PAD_RIGHT ? 1 : hash == PAD_LEFT ? -1 : 0;
    int changed = 0;

    if (step && row == ROW_OVERLAY) {
        g_overlay = (g_overlay + 3 + step) % 3;
        changed = 1;
    } else if (step && row == ROW_X360) {
        g_x360 = !g_x360;
        changed = 1;
    } else if (step && row == ROW_REPAIR) {
        g_repair = !g_repair;
        changed = 1;
    } else if (!step && g_unknown_logged < 24) {
        g_unknown_logged++;
        klog("row %d got message %08X", row, hash);
    }
    if (changed && !g_quitting)
        save_choices(0);
    ((unsigned char *)widget)[0x2A] = 1;
    ((WidgetNotify)vt[14])(widget, hash);
    ((WidgetDraw)vt[3])(widget);
}

#define ROW_FUNCTIONS(n)                                                                        \
    static void __attribute__((thiscall)) act_##n(void *widget, int ignored, unsigned hash)   \
    {                                                                                           \
        (void)ignored;                                                                          \
        row_act(n, widget, hash);                                                               \
    }                                                                                           \
    static void __attribute__((thiscall)) draw_##n(void *widget) { row_draw(n, widget); }
ROW_FUNCTIONS(0)
ROW_FUNCTIONS(1)
ROW_FUNCTIONS(2)
static void *const kAct[ROWS] = { (void *)act_0, (void *)act_1, (void *)act_2 };
static void *const kDraw[ROWS] = { (void *)draw_0, (void *)draw_1, (void *)draw_2 };

/* The language record of "Defaults", whatever table is current; its own text kept to put
 * back. on: "Restart and apply". */
static void restart_label(int on)
{
    static const char *defaults_text;
    LangRecord *table = (LangRecord *)LANG_TABLE;
    unsigned lo = 0, hi = table ? LANG_COUNT : 0;
    while (lo < hi) {
        const unsigned mid = (lo + hi) / 2;
        if (table[mid].hash < H_DEFAULTS)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo >= (table ? LANG_COUNT : 0) || table[lo].hash != H_DEFAULTS)
        return;
    if (table[lo].text != kRestartText)
        defaults_text = table[lo].text;
    if (on || defaults_text)
        table[lo].text = on ? kRestartText : defaults_text;
}

static int online_page(void)
{
    char *options = *(char **)0x91CF90;
    char *state = options ? *(char **)(options + 0x10) : NULL;
    return state && *(int *)(state + 0x24) == 9;
}

/* The reset box's text: ours on this page. */
unsigned dialog_text(void)
{
    return online_page() ? H_RESTART_ASK : H_RESET_ASK;
}

/* The box's Yes: on this page, restart and apply instead of resetting (1: handled). */
int defaults_yes(void)
{
    if (!online_page())
        return 0;
    klog("Yes: restart and apply");
    quit_to_launcher(0);
    return 1;
}

/* Every options page build: "Defaults" is itself again unless this page's rows set it. */
void rebuild_before(void)
{
    restart_label(0);
}

__attribute__((naked, used)) static void dialog_text_hook(void)
{
    __asm__ volatile (
        "call _dialog_text\n\t"
        "pushl %eax\n\t"
        "movl $" STR(DIALOG_TEXT_NEXT) ", %eax\n\t"
        "jmp *%eax\n\t"
    );
}

__attribute__((naked, used)) static void defaults_yes_hook(void)
{
    __asm__ volatile (
        "call _defaults_yes\n\t"
        "testl %eax, %eax\n\t"
        "jz 1f\n\t"
        "movl $" STR(HANDLER_DONE) ", %eax\n\t"
        "jmp *%eax\n"
        "1:\n\t"
        "movl 0x91CF90, %ecx\n\t"
        "movl $" STR(DEFAULTS_YES_NEXT) ", %eax\n\t"
        "jmp *%eax\n\t"
    );
}

__attribute__((naked, used)) static void rebuild_hook(void)
{
    __asm__ volatile (
        "pushl %ecx\n\t"
        "call _rebuild_before\n\t"
        "popl %ecx\n\t"
        "movl $" STR(REBUILD_CLEAR) ", %eax\n\t"
        "jmp *%eax\n\t"
    );
}

/* Called after 0x52A290 returns: its stock row cleared, ours added from the index the clear
 * restores (1). Options.fng names rows 1..10. */
void rows_after_build(void *screen)
{
    unsigned *src = (unsigned *)RES_VTABLE;
    int row;


    ensure_language();
    if (lookup_language(H_OVERLAY) != kOverlayText) {
        klog("the rows' text is not in the language table; stock online row left in place");
        return;
    }
    if (src[1] != 0x50F970 || src[3] != 0x51B7A0) {
        klog("resolution vtable is not the 1.3 layout; stock online row left in place");
        return;
    }
    load_choices();
    ((ClearRows)CLEAR_ROWS)(screen);
    for (row = 0; row < ROWS; row++) {
        const int index = *(int *)((char *)screen + 0x104);
        void *widget;
        if (!((ResolveNumbered)RESOLVE_NUMBERED)(screen, (const char *)NAME_TEMPLATE)) {
            klog("OPTION_NAME_%d is not in this package; rows from %d on not added", index, row);
            return;
        }
        widget = ((GameMalloc)GAME_MALLOC)(0x5C);
        if (!widget) {
            klog("widget allocation failed; rows from %d on not added", row);
            return;
        }
        ((InitToggle)INIT_TOGGLE)(widget, 1);
        memcpy(g_vt[row], src, sizeof g_vt[row]);
        g_vt[row][1] = kAct[row];
        g_vt[row][3] = kDraw[row];
        *(void **)widget = g_vt[row];
        ((AddToggle)ADD_TOGGLE)(screen, widget, 1);
        klog("added row %d at index %d", row, index);
    }
    restart_label(1);
}

/* Before the options menu builds its entries: "PS5 Options" on the first draw. */
void menu_before(void)
{
    ensure_language();
    restart_label(0);
}

__attribute__((naked, used)) static void menu_hook(void)
{
    __asm__ volatile (
        "pushl %ecx\n\t"
        "call _menu_before\n\t"
        "popl %ecx\n\t"
        "movl $0x528F30, %eax\n\t"
        "call *%eax\n\t"
        "ret\n\t"
    );
}

__attribute__((naked, used)) static void online_hook(void)
{
    __asm__ volatile (
        "pushl %ecx\n\t"
        "call _menu_before\n\t"
        "popl %ecx\n\t"
        "pushl %ecx\n\t"
        "movl $0x52A290, %eax\n\t"
        "call *%eax\n\t"
        "popl %ecx\n\t"
        "pushl %ecx\n\t"
        "call _rows_after_build\n\t"
        "addl $4, %esp\n\t"
        "ret\n\t"
    );
}

/* ---- loading ------------------------------------------------------------------------------ */

static void patch_call(unsigned site_va, const unsigned char expect[5], unsigned target,
                       void *hook, const char *ok)
{
    unsigned char *site = (unsigned char *)site_va;
    DWORD old;

    if (memcmp(site, expect, 5) != 0) {
        klog("0x%X is %02X %02X %02X %02X %02X; expected %02X %02X %02X %02X %02X; not hooked",
             site_va, site[0], site[1], site[2], site[3], site[4], expect[0], expect[1], expect[2],
             expect[3], expect[4]);
        return;
    }
    if ((unsigned)(site_va + 5 + *(int *)(site + 1)) != target) {
        klog("call at 0x%X does not target 0x%X; not hooked", site_va, target);
        return;
    }
    if (!VirtualProtect(site, 5, PAGE_EXECUTE_READWRITE, &old)) {
        klog("VirtualProtect failed at 0x%X; not hooked", site_va);
        return;
    }
    site[0] = 0xE8;
    *(int *)(site + 1) = (int)hook - (int)(site + 5);
    VirtualProtect(site, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), site, 5);
    klog("%s", ok);
}

/* A jump to hook over len bytes at site_va (the rest made nops), when they are what 1.3 has. */
static void patch_jump(unsigned site_va, const unsigned char *expect, unsigned len, void *hook,
                       const char *ok)
{
    unsigned char *site = (unsigned char *)site_va;
    DWORD old;
    unsigned i;

    if (memcmp(site, expect, len) != 0) {
        klog("0x%X is not the 1.3 code; %s: not hooked", site_va, ok);
        return;
    }
    if (!VirtualProtect(site, len, PAGE_EXECUTE_READWRITE, &old)) {
        klog("VirtualProtect failed at 0x%X; not hooked", site_va);
        return;
    }
    site[0] = 0xE9;
    *(int *)(site + 1) = (int)hook - (int)(site + 5);
    for (i = 5; i < len; i++)
        site[i] = 0x90;
    VirtualProtect(site, len, old, &old);
    FlushInstructionCache(GetCurrentProcess(), site, len);
    klog("%s", ok);
}

static void ini(const char *dir, const char *key, char *value, DWORD size)
{
    char path[MAX_PATH * 2], fallback[MAX_PATH];
    snprintf(path, sizeof path, "%sPS5SettingsMod.ini", dir);
    snprintf(fallback, sizeof fallback, "%s", value);
    GetPrivateProfileStringA("settings", key, fallback, value, size, path);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
    char dir[MAX_PATH], path[MAX_PATH + 32], *slash;
    unsigned i, j;

    (void)reserved;
    if (reason != DLL_PROCESS_ATTACH)
        return TRUE;
    DisableThreadLibraryCalls(instance);
    GetModuleFileNameA(instance, dir, sizeof dir);
    slash = strrchr(dir, '\\');
    if (slash)
        slash[1] = 0;
    /* The last run's log is kept: a restart from this menu starts the game again at once. */
    {
        char previous[MAX_PATH + 48];
        snprintf(path, sizeof path, "%sPS5SettingsMod.log", dir);
        snprintf(previous, sizeof previous, "%sPS5SettingsMod.previous.log", dir);
        MoveFileExA(path, previous, MOVEFILE_REPLACE_EXISTING);
    }
    log_file = fopen(path, "w");
    ini(dir, "title", g_title, sizeof g_title);
    ini(dir, "game", g_game, sizeof g_game);
    ini(dir, "data", g_data, sizeof g_data);
    ini(dir, "runner", g_runner, sizeof g_runner);
    ini(dir, "prefix", g_prefix, sizeof g_prefix);
    klog("PS5SettingsMod: title=%s game=%s data=%s runner=%s prefix=%s", g_title, g_game, g_data,
         g_runner, g_prefix);
    /* Our records in hash order, for the merge. */
    for (i = 1; i < OURS_COUNT; i++)
        for (j = i; j > 0 && g_ours[j - 1].hash > g_ours[j].hash; j--) {
            LangRecord swap = g_ours[j];
            g_ours[j] = g_ours[j - 1];
            g_ours[j - 1] = swap;
        }
    load_choices();
    patch_call(MENU_CALL, kMenuCall, BUILD_MENU, menu_hook, "options menu hooked");
    patch_call(ONLINE_CALL, kOnlineCall, BUILD_ONLINE, online_hook, "online options builder hooked");
    patch_call(REBUILD_CALL, kRebuildCall, REBUILD_CLEAR, rebuild_hook, "page build hooked");
    patch_jump(DIALOG_TEXT_SITE, kDialogText, sizeof kDialogText, dialog_text_hook,
               "reset box text hooked");
    patch_jump(DEFAULTS_YES_SITE, kDefaultsYes, sizeof kDefaultsYes, defaults_yes_hook,
               "reset box Yes hooked");
    return TRUE;
}
