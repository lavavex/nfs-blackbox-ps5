#!/usr/bin/env bash
# NFSBlackBoxMenu.asi (mods/NFSBlackBoxMenu): the in-game "Black Box" options menu, with Dear
# ImGui pinned by commit and its Roboto-Medium font embedded -> release/menu/.
# Runs in the build image (tools/build.sh step menu); no link time stamp, so the same
# compiler gives the same bytes.
set -euo pipefail
W=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
[[ $(uname -sm) == "Linux x86_64" ]] || { echo "build-blackbox-menu.sh runs in the build image: use tools/build.sh" >&2; exit 2; }
imgui_commit=c1aaac65e896a45f81f47a4d68f9959e0f86c859 # v1.93.0
src=$W/.deps/imgui work=$W/.deps/blackbox-menu out=$W/release/menu
if [[ $(git -C "$src" rev-parse HEAD 2>/dev/null) != "$imgui_commit" ]]; then
    rm -rf "$src" && mkdir -p "$src"
    git -C "$src" init -q
    git -C "$src" fetch -q --depth 1 https://github.com/ocornut/imgui.git "$imgui_commit"
    git -C "$src" checkout -q FETCH_HEAD
fi
rm -rf "$work" && mkdir -p "$work" "$out"
# The font as ImGui's compressed array (its own tool, built for the host).
c++ -O2 -o "$work/binary_to_compressed_c" "$src/misc/fonts/binary_to_compressed_c.cpp"
"$work/binary_to_compressed_c" "$src/misc/fonts/Roboto-Medium.ttf" roboto_medium > "$work/roboto_medium.h"
flags=(-std=c++17 -O2 -fno-exceptions -fno-rtti -DIMGUI_DISABLE_WIN32_FUNCTIONS -DIMGUI_DISABLE_DEFAULT_SHELL_FUNCTIONS
       -I"$src" -I"$src/backends" -I"$work")
objects=()
for file in "$src/imgui.cpp" "$src/imgui_draw.cpp" "$src/imgui_tables.cpp" "$src/imgui_widgets.cpp" \
            "$src/backends/imgui_impl_dx9.cpp" "$W/mods/NFSBlackBoxMenu/NFSBlackBoxMenu.cpp"; do
    object=$work/$(basename "${file%.cpp}").o
    i686-w64-mingw32-g++ "${flags[@]}" -c "$file" -o "$object"
    objects+=("$object")
done
i686-w64-mingw32-g++ -shared -s -static -static-libgcc -static-libstdc++ -Wl,--no-insert-timestamp \
    -o "$out/NFSBlackBoxMenu.asi" "${objects[@]}"
(cd "$out" && sha256sum NFSBlackBoxMenu.asi && stat -c '%s bytes' NFSBlackBoxMenu.asi)
