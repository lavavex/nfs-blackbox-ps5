#!/usr/bin/env bash
# PS5SettingsMod.asi (mods/ps5-settings-mod): the PS5 app's settings in MW's own Options menu
# (the Online page, renamed Black Box) -> release/settings-mod/.
# Runs in the build image (tools/build.sh step settings-mod); no link time stamp, so the same
# compiler gives the same bytes.
set -euo pipefail
W=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
[[ $(uname -sm) == "Linux x86_64" ]] || { echo "build-settings-mod.sh runs in the build image: use tools/build.sh" >&2; exit 2; }
out=$W/release/settings-mod
mkdir -p "$out"
i686-w64-mingw32-gcc -std=gnu99 -O2 -Wall -Wextra -Werror -shared -static-libgcc -s \
    -Wl,--no-insert-timestamp -o "$out/PS5SettingsMod.asi" "$W/mods/ps5-settings-mod/PS5SettingsMod.c"
(cd "$out" && sha256sum PS5SettingsMod.asi && stat -c '%s bytes' PS5SettingsMod.asi)
