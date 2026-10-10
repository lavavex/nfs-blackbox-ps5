#!/usr/bin/env bash
# SPDX-License-Identifier: LGPL-2.1-or-later
# Build a game's own Home-screen tile (native/tile/pw_tile_main.c): a tiny
# title with its own id, name and icon that starts prospero-win straight into
# one profile, closing to Home when the game ends. Uses the same foundation,
# SDK and signing as tools/build_native.sh (run that first: it prepares them).
#
#   tools/build_tile.sh --title-id PPSA99996 --name "NFS Most Wanted" \
#       --profile nfs-most-wanted-2005 [--icon art.png] [--target PPSA99995] \
#       --out ../release-main/tiles
# writes OUT/<title-id>/ (eboot.bin, sce_sys/, sce_module/libc.prx, tile.conf);
# deploy it like the app folder (/data/homebrew/<title-id>). The icon must be
# 512x512; without --icon a plain one with the name is drawn.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
title_id= name= profile= icon= target=PPSA99995 out=
while [[ $# -gt 0 ]]; do
    case $1 in
    --title-id) title_id=$2; shift 2 ;;
    --name) name=$2; shift 2 ;;
    --profile) profile=$2; shift 2 ;;
    --icon) icon=$2; shift 2 ;;
    --target) target=$2; shift 2 ;;
    --out) out=$2; shift 2 ;;
    *) echo "build_tile: unknown argument $1" >&2; exit 2 ;;
    esac
done
[[ $title_id =~ ^[A-Z]{4}[0-9]{5}$ && -n $name && $profile =~ ^[A-Za-z0-9_.-]+$ && -n $out ]] || {
    echo "usage: $0 --title-id PPSA99996 --name NAME --profile ID [--icon PNG] [--target TITLEID] --out DIR" >&2
    exit 2; }
[[ $title_id != "$target" ]] || { echo "build_tile: the tile needs its own title id, not $target" >&2; exit 2; }

pin=$(sed -n 's/^pin=//p' "$root/tools/build_native.sh")
foundation=${PS5_NATIVE_FOUNDATION:-$root/.deps/ps5-native-app-boilerplate}
sdk=$foundation/.deps/native/ps5-payload-sdk
native=$foundation/tooling/native
tool=$foundation/build/host/ps5-native-tool-$pin
layout=$native/ps5-pie-high.ld
for need in "$sdk/bin/prospero-lld" "$tool" "$layout" "$foundation/runtime/libc.prx"; do
    [[ -e $need ]] || { echo "build_tile: $need is missing: run tools/build_native.sh first" >&2; exit 2; }
done
cc=(env PS5_PAYLOAD_SDK="$sdk" sh "$foundation/tooling/prospero-clang18")

build=$root/build/tile/$title_id
app=$out/$title_id
rm -rf -- "$build" "$app"
mkdir -p "$build" "$app/sce_sys" "$app/sce_module"
"${cc[@]}" -std=c11 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections \
    -c "$root/native/tile/pw_tile_main.c" -o "$build/tile.o"
"${cc[@]}" -std=c++20 -O2 -Wall -Wextra -Werror -fno-exceptions -fno-rtti \
    -ffunction-sections -fdata-sections -c "$native/app_crt.cpp" -o "$build/app_crt.o"
"$sdk/bin/prospero-lld" -T "$layout" --eh-frame-hdr -e _start -o "$build/llvm-pie.elf" \
    "$build/app_crt.o" "$build/tile.o" --as-needed "$sdk"/target/lib/*.so
"$tool" link --in "$build/llvm-pie.elf" --out "$build/eboot.elf" \
    --stub-dir "$sdk/target/lib" --module-sdk 0x02000009 \
    --companion-sdk 0x08050001 --file-name eboot.elf
"$tool" self --sign --in "$build/eboot.elf" --out "$app/eboot.bin" --magic 0x1D3D154F
cp "$foundation/runtime/libc.prx" "$app/sce_module/libc.prx"
chmod 755 "$app/eboot.bin" "$app/sce_module/libc.prx"
printf 'profile=%s\ntarget=%s\n' "$profile" "$target" > "$app/tile.conf"

python3 - "$root/sce_sys/param.json" "$app/sce_sys/param.json" "$title_id" "$name" "$profile" <<'PY'
import json, re, sys
template, out, title_id, name, profile = sys.argv[1:]
param = json.load(open(template))
label = re.sub(r"[^A-Z0-9]", "", profile.upper())[:12].ljust(12, "0") + "TILE"
param["titleId"] = title_id
param["conceptId"] = title_id[4:]
param["contentId"] = f"UP9000-{title_id}_00-{label}"
param["localizedParameters"] = {"defaultLanguage": "en-US", "en-US": {"titleName": name}}
param["attribute3"] = 0   # the tile shows no video; prospero-win declares its own output
json.dump(param, open(out, "w"), indent=2)
open(out, "a").write("\n")
PY

if [[ -n $icon ]]; then
    python3 - "$icon" <<'PY'
import struct, sys
data = open(sys.argv[1], "rb").read(32)
assert data[:8] == b"\x89PNG\r\n\x1a\n", "not a PNG"
w, h = struct.unpack(">II", data[16:24])
assert (w, h) == (512, 512), f"icon is {w}x{h}, needs 512x512"
PY
    cp "$icon" "$app/sce_sys/icon0.png"
else
    python3 - "$app/sce_sys/icon0.png" "$name" <<'PY'
import struct, sys, zlib
# A plain 512x512 RGB icon (dark, with a light band) when no art is given.
out, size = sys.argv[1], 512
rows = bytearray()
for y in range(size):
    rows.append(0)
    color = (230, 230, 230) if 300 <= y < 340 else (24, 28, 40)
    rows += bytes(color) * size
def chunk(kind, body):
    return struct.pack(">I", len(body)) + kind + body + struct.pack(">I", zlib.crc32(kind + body) & 0xffffffff)
png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", size, size, 8, 2, 0, 0, 0)) \
    + chunk(b"IDAT", zlib.compress(bytes(rows), 9)) + chunk(b"IEND", b"")
open(out, "wb").write(png)
PY
fi
echo "build_tile: $app ($name -> $target profile=$profile)"
sha256sum "$app/eboot.bin"
