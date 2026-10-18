#!/usr/bin/env bash
# The parts of MW's Xbox 360 graphics option this repo makes -> release/x360/:
#   d3dx9_26.dll, d3dx9_43.dll   (x86) from Microsoft's DirectX June 2010 redistributable,
#                                pinned by sha256; the same bytes as the console's
#   X360StuffAsyncLuminance.asi  our ASI (mods/X360StuffAsyncLuminance), built without a
#                                link time stamp, so the same compiler gives the same bytes
# Runs in the build image (tools/build.sh step x360); tools/make-bundle.py carries them.
set -euo pipefail
W=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
[[ $(uname -sm) == "Linux x86_64" ]] || { echo "build-x360-bits.sh runs in the build image: use tools/build.sh" >&2; exit 2; }
redist=directx_Jun2010_redist.exe
redist_sha=053f76dcbb28802e23341b6a787e3b0791c0fa5c8d4d011b1044172dbf89c73b
url=https://download.microsoft.com/download/8/4/A/84A35BF1-DAFE-4AE8-82AF-AD2AE20B6B14/$redist
out=$W/release/x360 work=$W/.deps/x360-bits
mkdir -p "$W/downloads" "$out"
if ! printf '%s  %s\n' "$redist_sha" "$W/downloads/$redist" | sha256sum --check --status 2>/dev/null; then
    curl -sfL -o "$W/downloads/$redist.part" "$url"
    printf '%s  %s\n' "$redist_sha" "$W/downloads/$redist.part" | sha256sum --check --status
    mv "$W/downloads/$redist.part" "$W/downloads/$redist"
fi
rm -rf "$work" && mkdir -p "$work"
cabextract -q -d "$work/cabs" -F 'Jun2005_d3dx9_26_x86.cab' -F 'Jun2010_d3dx9_43_x86.cab' "$W/downloads/$redist"
for cab in "$work"/cabs/*.cab; do cabextract -q -d "$work/dll" -F '*.dll' "$cab"; done
cp "$work/dll/d3dx9_26.dll" "$work/dll/d3dx9_43.dll" "$out/"
printf '%s  %s\n' 8ea96fe01c3c86a36fcb3795ae03eb12034003e335ef475571efaeda17c5bc78 "$out/d3dx9_26.dll" \
    0b28546be22c71834501f7d7185ede5d79742457331c7ee09efc14490dd64f5f "$out/d3dx9_43.dll" | sha256sum --check --quiet
i686-w64-mingw32-gcc -std=gnu11 -O2 -Wall -Wextra -shared -static-libgcc -s -Wl,--no-insert-timestamp \
    -o "$out/X360StuffAsyncLuminance.asi" "$W/mods/X360StuffAsyncLuminance/X360StuffAsyncLuminance.c"
(cd "$out" && sha256sum d3dx9_26.dll d3dx9_43.dll X360StuffAsyncLuminance.asi)
