/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * The payload half of a Home-screen tile (native/tile/pw_tile_main.c).
 *
 * An app in its sandbox cannot start another title: the loader leaves
 * sceLncUtilLaunchApp unresolved for it (the first tile jumped to address 0).
 * A payload can, as PS5_Vulkan's ps5vkctl does. So the tile writes what to
 * start to TILE_REQUEST, sends this program to the console's payload loader
 * (loopback port 9021, as prospero-win sends lapy.elf) and closes; this
 * program waits for the tile to be gone, starts the target title with
 * "profile=<id>" and "home=1", and exits.
 *
 * Built with the PS5 payload SDK by tools/build_tile.sh. What each launch
 * call answered is appended to /data/prospero-win/logs/tile-<profile>.log.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define TILE_REQUEST "/data/prospero-win/tile-request"
#define TILE_LOG_DIR "/data/prospero-win/logs"

/* The launch parameter etaHEN's launcher passes, field for field. */
typedef struct lnc_app_param {
    uint32_t size;
    int user_id;
    uint32_t app_opt;
    uint64_t crash_report;
    uint64_t check_flag;
} lnc_app_param_t;

typedef struct login_user_id_list {
    int user_id[4];
} login_user_id_list_t;

extern int sceLncUtilLaunchApp(const char *title_id, const char *argv[], lnc_app_param_t *param);
extern int sceSystemServiceLaunchApp(const char *title_id, const char *argv[], lnc_app_param_t *param);
extern uint32_t sceLncUtilGetAppIdOfRunningBigApp(void);
extern int sceLncUtilGetAppTitleId(uint32_t app_id, char *title_id);
extern int sceUserServiceInitialize(void *param);
extern int sceUserServiceGetForegroundUser(int *user_id);
extern int sceUserServiceGetLoginUserIdList(login_user_id_list_t *list);

static FILE *log_file;

static void log_line(const char *format, ...) __attribute__((format(printf, 1, 2)));
static void log_line(const char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    if (log_file) {
        vfprintf(log_file, format, arguments);
        fputc('\n', log_file);
        fflush(log_file);
    }
    va_end(arguments);
}

static int login_user(void)
{
    login_user_id_list_t users;
    memset(&users, 0, sizeof(users));
    if (sceUserServiceGetLoginUserIdList(&users) != 0) return -1;
    for (int i = 0; i < 4; i++)
        if (users.user_id[i] >= 0) return users.user_id[i];
    return -1;
}

/* The title id of the application in the foreground, or "" when none. */
static void running_title(char *out, size_t size)
{
    char title[32] = {0};
    uint32_t app_id = sceLncUtilGetAppIdOfRunningBigApp();
    out[0] = 0;
    if (app_id != 0xffffffffu && app_id != 0 && sceLncUtilGetAppTitleId(app_id, title) == 0)
        snprintf(out, size, "%s", title);
}

int main(void)
{
    char line[160], profile[96] = "", target[16] = "PPSA99995", tile[16] = "";
    char word[112], path[192], running[32];

    FILE *request = fopen(TILE_REQUEST, "r");
    if (!request) return 1;
    while (fgets(line, sizeof(line), request)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "profile=", 8)) snprintf(profile, sizeof(profile), "%s", line + 8);
        else if (!strncmp(line, "target=", 7)) snprintf(target, sizeof(target), "%s", line + 7);
        else if (!strncmp(line, "tile=", 5)) snprintf(tile, sizeof(tile), "%s", line + 5);
    }
    fclose(request);
    unlink(TILE_REQUEST);
    if (!profile[0]) return 1;
    snprintf(path, sizeof(path), TILE_LOG_DIR "/tile-%s.log", profile);
    log_file = fopen(path, "a");
    snprintf(word, sizeof(word), "profile=%s", profile);
    const char *argv[] = { word, "home=1", NULL };

    /* Let the tile finish closing: one application in front at a time. */
    for (int wait = 0; wait < 40; wait++) {
        running_title(running, sizeof(running));
        if (!tile[0] || strcmp(running, tile)) break;
        usleep(100000);
    }
    log_line("payload: in front now: '%s'; starting %s with %s home=1", running, target, word);

    sceUserServiceInitialize(NULL);
    int foreground = -1;
    if (sceUserServiceGetForegroundUser(&foreground) != 0) foreground = -1;
    const int login = login_user();
    const struct {
        const char *name;
        int (*call)(const char *, const char *[], lnc_app_param_t *);
        int user_id;
    } attempts[] = {
        { "lncUtil/foreground", sceLncUtilLaunchApp, foreground },
        { "lncUtil/login", sceLncUtilLaunchApp, login },
        { "systemService/login", sceSystemServiceLaunchApp, login },
    };
    /* While the tile's sandbox is still being removed, ShadowMount+ cannot
     * mount the target ("launch mount unavailable: Device busy") and the
     * launch is refused (0x80940033): give it a moment, then keep asking. */
    usleep(1500000);
    int started = 0;
    for (int round = 1; round <= 15 && !started; round++) {
        for (size_t i = 0; i < sizeof(attempts) / sizeof(attempts[0]) && !started; i++) {
            if (attempts[i].user_id < 0) continue;
            lnc_app_param_t param;
            memset(&param, 0, sizeof(param));
            param.size = (uint32_t)sizeof(param);
            param.user_id = attempts[i].user_id;
            int rc = attempts[i].call(target, argv, &param);
            log_line("payload: round %d %s user %d -> 0x%08x", round, attempts[i].name,
                     attempts[i].user_id, (unsigned)rc);
            started = rc == 0;
        }
        if (!started) usleep(1000000);
    }
    log_line("payload: %s", started ? "started" : "gave up");
    if (log_file) fclose(log_file);
    return 0;
}
