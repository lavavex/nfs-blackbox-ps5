# Our ASIs

## NFSMWConsoleKeyboard.asi (base tier)

Built and released by its own repo's CI, lavavex/NFSMW-ConsoleKeyboard (ubuntu-24.04,
gcc-mingw-w64-i686 13.2; its Makefile pins the timestamp and image base, so the same
compiler gives the same bytes). The plan downloads release v1.1: 45,568 bytes, sha256
`b65b1c745c32cfc7fb9b6ad41eeb3487e0649068731a29273d8e6006b5b9d1ec`. Older local builds
and the console's original are in git history (`asi.md` before 2026-10-13).

## X360StuffAsyncLuminance.asi (the Xbox 360 option)

Built from `mods/X360StuffAsyncLuminance/X360StuffAsyncLuminance.c` by `tools/build-x360-bits.sh`
in the build image (MinGW-w64 GCC 13, `-O2 -s -Wl,--no-insert-timestamp`): 43,520 bytes, sha256
`98f7b9338ecf038ec623df128e00ad8e94dbea3d9908b394fc36cd12352b46fa`, the same on every build. The
plan pins it and the manager carries it. The console's hand-built copy (16,384 bytes, `d8a35fc7…`,
another GCC) is in git history; the rebuilt one still needs its fps check on the console
(it removes X360Stuff's per-frame GPU stall: about +13 fps).
