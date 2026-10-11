/* SPDX-License-Identifier: LGPL-2.1-or-later */
/*
 * A game's own Home-screen tile.
 *
 * A tile is a tiny title of its own (its title id, name and icon) whose only
 * job is to start prospero-win (PPSA99995) straight into one game, with
 * "profile=<id>" and "home=1" so the game closes to Home rather than to the
 * launcher (src/pw_wine_launch.h). prospero-win still requests the /data
 * mount, loads the library and runs Wine itself.
 *
 * An app cannot start another title itself: the loader leaves
 * sceLncUtilLaunchApp unresolved in a sandboxed title (the first tile jumped
 * to address 0 calling it). So the tile writes the request to TILE_REQUEST,
 * sends its payload, tile-launch.elf (native/tile/pw_tile_launch.c), to the
 * console's payload loader on loopback port 9021, as prospero-win sends
 * lapy.elf, and stays open: the payload starts the game while the tile is
 * still the running application, which the system (and ShadowMount+, which
 * refuses another title while an exited one awaits a same-app restart)
 * treat as an ordinary switch from one app to the next, closing the tile.
 * If nothing has replaced the tile after TILE_WAIT_S, it closes itself.
 *
 * Which game: /app0/tile.conf ("profile=<id>", "target=<TITLEID>",
 * "tile=<own TITLEID>"), written by tools/build_tile.sh, so one eboot serves
 * every tile. Progress is appended to /data/prospero-win/logs/tile-<id>.log.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>

int sceNetConnect(int, const struct sockaddr *, socklen_t);
int sceNetSend(int, const void *, size_t, int);
int sceNetSetsockopt(int, int, int, const void *, socklen_t);
int sceNetShutdown(int, int);
int sceNetSocket(const char *, int, int, int);
int sceNetSocketClose(int);
int sceSystemServiceLoadExec(const char *path, char *const argv[]);

#define TILE_LOADER_PORT 9021
#define TILE_TIMEOUT_US 5000000
#define TILE_WAIT_S 30

/* Where the tile reads, writes and sends. A table of pointers, kept as data:
 * it gives the image the .data.rel.ro section ps5-native-tool requires. */
enum { TILE_CONF, TILE_PAYLOAD, TILE_REQUEST, TILE_LOG_DIR };
static const char *const tile_paths[] __attribute__((used)) = {
    [TILE_CONF] = "/app0/tile.conf",
    [TILE_PAYLOAD] = "/app0/tile-launch.elf",
    [TILE_REQUEST] = "/data/prospero-win/tile-request",
    [TILE_LOG_DIR] = "/data/prospero-win/logs",
};

struct tile_sockaddr_in {
    uint8_t length;
    uint8_t family;
    uint16_t port;
    uint32_t address;
    uint16_t virtual_port;
    uint8_t zero[6];
};

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

/* tile.conf's "key=value" lines; returns 0 when profile= was found. */
static int read_conf(char *profile, size_t profile_size, char *target, size_t target_size,
                     char *tile, size_t tile_size)
{
    char line[160];
    FILE *conf = fopen(tile_paths[TILE_CONF], "r");
    if (!conf) return -1;
    profile[0] = 0;
    while (fgets(line, sizeof(line), conf)) {
        line[strcspn(line, "\r\n")] = 0;
        if (!strncmp(line, "profile=", 8)) snprintf(profile, profile_size, "%s", line + 8);
        else if (!strncmp(line, "target=", 7)) snprintf(target, target_size, "%s", line + 7);
        else if (!strncmp(line, "tile=", 5)) snprintf(tile, tile_size, "%s", line + 5);
    }
    fclose(conf);
    return profile[0] ? 0 : -1;
}

/* Stream the payload to the loader; 0 when all of it was sent. */
static int send_payload(void)
{
    uint8_t buffer[4096];
    uint32_t timeout = TILE_TIMEOUT_US;
    struct tile_sockaddr_in address;
    long total = 0;
    int result = -1;

    int payload = open(tile_paths[TILE_PAYLOAD], O_RDONLY);
    if (payload < 0) { log_line("tile: no %s", tile_paths[TILE_PAYLOAD]); return -1; }
    int socket = sceNetSocket("pw_tile", AF_INET, SOCK_STREAM, 6 /* TCP */);
    if (socket < 0) { log_line("tile: socket -> 0x%08x", (unsigned)socket); close(payload); return -1; }
    for (int option = 0x1105; option <= 0x1106; option++)   /* send and receive timeouts */
        (void)sceNetSetsockopt(socket, 0xffff, option, &timeout, sizeof(timeout));
    memset(&address, 0, sizeof(address));
    address.length = sizeof(address);
    address.family = AF_INET;
    address.port = (uint16_t)((TILE_LOADER_PORT << 8) | (TILE_LOADER_PORT >> 8));
    address.address = UINT32_C(0x0100007f);
    int rc = sceNetConnect(socket, (const struct sockaddr *)&address, sizeof(address));
    if (rc < 0) {
        log_line("tile: no payload loader on port %d (0x%08x)", TILE_LOADER_PORT, (unsigned)rc);
        goto done;
    }
    for (;;) {
        ssize_t count = read(payload, buffer, sizeof(buffer));
        if (count == 0) { result = 0; break; }
        if (count < 0) break;
        for (ssize_t sent = 0; sent < count;) {
            int n = sceNetSend(socket, buffer + sent, (size_t)(count - sent), 0);
            if (n <= 0) { log_line("tile: send -> 0x%08x after %ld bytes", (unsigned)n, total); goto done; }
            sent += n;
            total += n;
        }
    }
    (void)sceNetShutdown(socket, 1);   /* the payload ends here, whichever way the loader reads */
    log_line("tile: sent %s (%ld bytes) to the payload loader", tile_paths[TILE_PAYLOAD], total);
done:
    sceNetSocketClose(socket);
    close(payload);
    return result;
}

int main(void)
{
    char profile[96], target[16] = "PPSA99995", tile[16] = "", path[192];

    if (read_conf(profile, sizeof(profile), target, sizeof(target), tile, sizeof(tile)) != 0)
        return 1;
    snprintf(path, sizeof(path), "%s/tile-%s.log", tile_paths[TILE_LOG_DIR], profile);
    log_file = fopen(path, "a");
    FILE *request = fopen(tile_paths[TILE_REQUEST], "w");
    if (!request) {
        log_line("tile: cannot write %s", tile_paths[TILE_REQUEST]);
        return 1;
    }
    fprintf(request, "profile=%s\ntarget=%s\ntile=%s\n", profile, target, tile);
    fclose(request);
    log_line("tile: %s asks for %s profile=%s home=1", tile, target, profile);
    if (send_payload() != 0) unlink(tile_paths[TILE_REQUEST]);
    else
        for (int second = 0; second < TILE_WAIT_S; second++) sleep(1);   /* the switch ends us */
    log_line("tile: nothing replaced the tile; closing");
    if (log_file) fclose(log_file);
    /* Close the way a title does (returning from main ends in a system call
     * the sandbox refuses: SIGSYS). */
    sceSystemServiceLoadExec("exit", NULL);
    return 0;
}
