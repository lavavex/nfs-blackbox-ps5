#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# Build a game's own app: prospero-win under its own title id, name and icon,
# that starts one profile straight from the Home screen (no launcher) and
# closes to Home when the game ends. It shares /data/prospero-win (profiles,
# prefixes, saves) with prospero-win.
#
# The app is prospero-win's package (tools/package_release.sh's PPSA99995)
# with three things of its own:
#   eboot.bin  tools/build_native.sh with PW_TITLE_ID and PW_AUTOSTART_PROFILE
#              (the title id also names the sandbox path the runtime is read from);
#   lapy.elf   the Lapy helper built for this title id: a helper only elevates
#              the title it was built for (PS5-Lapy-JB-Daemon, TARGET_TITLE);
#   sce_sys/   param.json with the id and name, the icon (icon0.png, 512x512),
#              and optionally the Home-screen background: the Shell shows the BC7
#              textures pic0.dds (tile selected) and pic1.dds (launch), 3840x2160,
#              made by ps5-vulkan-mp's tools/prepare-assets.sh (bc7enc_rdo; build it
#              once with make -C ../ps5-vulkan-mp assets-deps); pic0.png is kept too,
#              and music (snd0.at9: ATRAC9 48 kHz stereo, as the console's own
#              titles carry; no open-source encoder exists, so it is supplied).
#
#   tools/build_game_app.sh --title-id PPSA99996 --name "NFS Most Wanted" \
#       --profile nfs-most-wanted-2005 [--icon art.png] [--background bg.png] [--music snd0.at9] \
#       --package ../release-main/PPSA99995 --out ../release-main/apps
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
lapy_url=https://github.com/mpereiraesaa/PS5-Lapy-JB-Daemon.git
lapy_commit=b48b7d7236eca25c7b9dfd6c040c763ae05dde80   # v0.3.2-experimental, as build_native.sh fetches
title_id= name= profile= icon= background= music= package= out=
while [[ $# -gt 0 ]]; do
    case $1 in
    --title-id) title_id=$2; shift 2 ;;
    --name) name=$2; shift 2 ;;
    --profile) profile=$2; shift 2 ;;
    --icon) icon=$2; shift 2 ;;
    --background) background=$2; shift 2 ;;
    --music) music=$2; shift 2 ;;
    --package) package=$2; shift 2 ;;
    --out) out=$2; shift 2 ;;
    *) echo "build_game_app: unknown argument $1" >&2; exit 2 ;;
    esac
done
[[ $title_id =~ ^[A-Z]{4}[0-9]{5}$ && $title_id != PPSA99995 && -n $name &&
   $profile =~ ^[A-Za-z0-9_.-]+$ && -f $package/eboot.bin && -n $out ]] || {
    echo "usage: $0 --title-id PPSA99996 --name NAME --profile ID [--icon PNG] --package DIR --out DIR" >&2
    exit 2; }
foundation=${PS5_NATIVE_FOUNDATION:-$root/.deps/ps5-native-app-boilerplate}
sdk=$foundation/.deps/native/ps5-payload-sdk
[[ -x $sdk/bin/prospero-clang ]] || { echo "build_game_app: run tools/build_native.sh first" >&2; exit 2; }

echo "==> Lapy helper for $title_id"
lapy=$root/.deps/lapy-src
[[ -d $lapy/.git ]] || git clone -q "$lapy_url" "$lapy"
git -C "$lapy" fetch -q origin "$lapy_commit" 2>/dev/null || true
git -C "$lapy" checkout -q --detach "$lapy_commit"
PS5_PAYLOAD_SDK="$sdk" make -s -C "$lapy" owned-helper TARGET_TITLE="$title_id" \
    LOGGING_CLIENT="$root/native/ps5log"
helper=$lapy/build/owned_root_helper-$title_id
python3 - "$helper/release.json" "$lapy_commit" "$title_id" <<'PY'
import json, sys
json.dump({"repo": "mpereiraesaa/PS5-Lapy-JB-Daemon", "tag": "local-build",
           "commit": sys.argv[2], "target_title": sys.argv[3]}, open(sys.argv[1], "w"), indent=1)
PY

echo "==> eboot for $title_id (autostart $profile)"
PW_TITLE_ID=$title_id PW_AUTOSTART_PROFILE=$profile PW_LAPY_DIR=$helper PW_OUTPUT_SUFFIX=-app \
    "$root/tools/build_native.sh"
dist=$root/dist/$title_id-app

echo "==> app folder"
app=$out/$title_id
mkdir -p "$out"
rsync -a --delete --exclude=dev.conf "$package/" "$app/"
cp "$dist/eboot.bin" "$app/eboot.bin"
cp "$dist/lapy.elf" "$app/lapy.elf"
chmod 755 "$app/eboot.bin"
python3 - "$root/sce_sys/param.json" "$app/sce_sys/param.json" "$title_id" "$name" "$profile" <<'PY'
import json, re, sys
template, out, title_id, name, profile = sys.argv[1:]
param = json.load(open(template))     # keeps attribute3: the high-frame-rate declaration
label = re.sub(r"[^A-Z0-9]", "", profile.upper())[:13].ljust(13, "0") + "APP"
param["titleId"] = title_id
param["conceptId"] = title_id[4:]
param["contentId"] = f"UP9000-{title_id}_00-{label}"
param["localizedParameters"] = {"defaultLanguage": "en-US", "en-US": {"titleName": name}}
json.dump(param, open(out, "w"), indent=2)
open(out, "a").write("\n")
PY
if [[ -n $icon ]]; then
    python3 -c 'import struct,sys; d=open(sys.argv[1],"rb").read(24); assert d[:8]==b"\x89PNG\r\n\x1a\n" and struct.unpack(">II",d[16:24])==(512,512), "icon must be a 512x512 PNG"' "$icon"
    cp "$icon" "$app/sce_sys/icon0.png"
fi
if [[ -n $background ]]; then
    python3 -c 'import struct,sys; d=open(sys.argv[1],"rb").read(26); assert d[:8]==b"\x89PNG\r\n\x1a\n" and struct.unpack(">II",d[16:24])==(3840,2160) and d[25]==2, "background must be a 3840x2160 RGB PNG"' "$background"
    cp "$background" "$app/sce_sys/pic0.png"
    assets=${PS5_ASSETS_TOOL:-$root/../ps5-vulkan-mp/tools/prepare-assets.sh}
    [[ -f $assets ]] || { echo "build_game_app: no $assets (ps5-vulkan-mp)" >&2; exit 2; }
    textures=$(mktemp -d)
    cp "$app/sce_sys/icon0.png" "$textures/"   # the tool validates its folder as a whole sce_sys
    bash "$assets" --background "$background" --output-directory "$textures" ||
        { echo "build_game_app: background conversion failed (make -C ../ps5-vulkan-mp assets-deps?)" >&2; exit 2; }
    cp "$textures/pic0.dds" "$textures/pic1.dds" "$app/sce_sys/"
    rm -rf -- "$textures"
fi
if [[ -n $music ]]; then
    python3 -c 'import sys; d=open(sys.argv[1],"rb").read(24); assert d[:4]==b"RIFF" and d[8:12]==b"WAVE" and d[20:22]==b"\xfe\xff", "music must be an ATRAC9 RIFF/WAVE (snd0.at9)"' "$music"
    cp "$music" "$app/sce_sys/snd0.at9"
fi
echo "build_game_app: $app ($name, starts $profile)"
sha256sum "$app/eboot.bin" "$app/lapy.elf"
