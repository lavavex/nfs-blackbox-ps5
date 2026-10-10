/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * A game's own Home-screen tile.
 *
 * A tile is a tiny title of its own (its title id, name and icon) whose only
 * job is to start prospero-win (PPSA99995) straight into one game:
 * sceLncUtilLaunchApp with "profile=<id>" and "home=1", so the game closes to
 * Home rather than to the launcher (src/pw_wine_launch.h). prospero-win
 * still requests the /data mount, loads the library and runs Wine itself;
 * the tile needs no elevation.
 *
 * Which game: /app0/tile.conf, "profile=<id>" (and optionally
 * "target=<TITLEID>", default PPSA99995), written by tools/build_tile.sh,
 * so one eboot serves every tile. What each launch call answered goes to
 * /data/prospero-win/logs/tile-<profile>.log when the title can write there.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* The launch parameter etaHEN's launcher passes, field for field
 * (as PS5_Vulkan's ps5vkctl uses it on a console). */
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
extern int sceUserServiceInitialize(void *param);
extern int sceUserServiceGetForegroundUser(int *user_id);
extern int sceUserServiceGetLoginUserIdList(login_user_id_list_t *list);

#define TILE_CONF "/app0/tile.conf"

/* The ways of asking the console to start a title, in the order etaHEN and
 * AirPSX use them (as ps5vkctl tries them). A table of pointers, kept even
 * when the loop is unrolled: it gives the image the .data.rel.ro section
 * ps5-native-tool requires. */
enum tile_user { TILE_FOREGROUND, TILE_LOGIN };
static const struct tile_attempt {
    const char *name;
    int (*call)(const char *, const char *[], lnc_app_param_t *);
    enum tile_user user;
} attempts[] __attribute__((used)) = {
    { "lncUtil/foreground", sceLncUtilLaunchApp, TILE_FOREGROUND },
    { "lncUtil/login", sceLncUtilLaunchApp, TILE_LOGIN },
    { "systemService/login", sceSystemServiceLaunchApp, TILE_LOGIN },
};
#define TILE_LOG_DIR "/data/prospero-win/logs"

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

/* "key=value" lines; returns 0 when profile= was found. */
static int read_conf(char *profile, size_t profile_size, char *target, size_t target_size)
{
    char line[160];
    FILE *conf = fopen(TILE_CONF, "r");
    if (!conf) return -1;
    profile[0] = 0;
    while (fgets(line, sizeof(line), conf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "profile=", 8)) snprintf(profile, profile_size, "%s", line + 8);
        else if (!strncmp(line, "target=", 7)) snprintf(target, target_size, "%s", line + 7);
    }
    fclose(conf);
    return profile[0] ? 0 : -1;
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

int main(void)
{
    char profile[96], target[16] = "PPSA99995", word[112], path[192];

    if (read_conf(profile, sizeof(profile), target, sizeof(target)) != 0) return 1;
    snprintf(path, sizeof(path), TILE_LOG_DIR "/tile-%s.log", profile);
    log_file = fopen(path, "a");
    snprintf(word, sizeof(word), "profile=%s", profile);
    const char *argv[] = { word, "home=1", NULL };

    sceUserServiceInitialize(NULL);
    int foreground = -1;
    if (sceUserServiceGetForegroundUser(&foreground) != 0) foreground = -1;
    int login = login_user();
    log_line("tile: start %s with %s home=1 (foreground user %d, login user %d)",
             target, word, foreground, login);

    for (size_t i = 0; i < sizeof(attempts) / sizeof(attempts[0]); i++) {
        int user_id = attempts[i].user == TILE_FOREGROUND ? foreground : login;
        if (user_id < 0) continue;
        lnc_app_param_t param;
        memset(&param, 0, sizeof(param));
        param.size = (uint32_t)sizeof(param);
        param.user_id = user_id;
        int rc = attempts[i].call(target, argv, &param);
        log_line("tile: %s user %d -> 0x%08x", attempts[i].name, user_id, (unsigned)rc);
        if (rc == 0) break;
    }
    if (log_file) fclose(log_file);
    /* The tile's work is done; ending it returns the console to whatever
     * started (prospero-win, or Home if the launch was refused). */
    return 0;
}
