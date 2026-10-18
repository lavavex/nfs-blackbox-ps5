# DXVK

## Our build, in the bare prefix (2026-10-12)

`tools/build-dxvk.sh` (or `tools/build-prospero-win.sh dxvk`): DXVK `v3.1.1` (`b1a1c99a`) plus
`patches/dxvk/` (32-bit reference/use counters; the submit and CS threads woken after the
lock is released), every DLL (d3d8, d3d9, d3d10core, d3d11, dxgi) for 32- and 64-bit, built
with DXVK's own `package-release.sh` and MinGW-w64 GCC. Packed as
`.deps/dxvk-3.1.1-nfsbb.tar.gz` in DXVK's release layout. With `-Wl,--no-insert-timestamp`
and `SOURCE_DATE_EPOCH` (the strip step stamps PE headers otherwise), two builds on the Mac
gave the same archive: sha256 `99428efca7da6a70624d463d01503de5a9ad6ec33da8f3df94cc9627ce78af31`
(GCC 16.2.0, Homebrew mingw-w64 14.0.0). `tools/make-bare-prefix.py` installs it into the
prefix through pw_install's local-archive keys (prospero-win patch 0028), all five DLLs native.
The 32-bit `d3d9.dll` is 8,644,240 bytes (the console's d3d9-only build below was 7,966,734).

# DXVK lean (the console's build, 2026-10-11)

Source on build-pc: `~/src/dxvk`, branch `prospero/v3.1.1-lean`. It is
`v3.1.1` (`b1a1c99a`) plus two commits. The patches are `patches/dxvk/`.

The build that is configured in `build.w32` was started 2026-10-11 16:24.
`build.w32/meson-logs/meson-log.txt` records:

```
meson setup build.w32 --cross-file build-win32.txt \
  -Dbuildtype=release -Denable_d3d10=false -Denable_d3d11=false \
  -Denable_dxgi=false -Dprefix=/usr
ninja -C build.w32
i686-w64-mingw32-strip -s build.w32/src/d3d9/d3d9.dll
```

`build-win32.txt` sets `c` and `cpp` to `i686-w64-mingw32-gcc` /
`i686-w64-mingw32-g++`, host cpu `x86`, system `windows`. Meson was 1.12.1.
The compiler was GCC 16.2.0.

| File | Size | sha256 |
| --- | --- | --- |
| `build.w32/src/d3d9/d3d9.dll` before strip | 20,417,979 | `675eed97109716b25de17520f918e5b9f76ded67b35f41ce5d944748055ca658` |
| that file after `i686-w64-mingw32-strip -s` on 2026-10-11 | 7,966,734 | `f596455aa9b96c80af8036b6ea58d3dab19d603f89db3fb8fd32ba047f736012` |
| console `syswow64/d3d9.dll` (the one in use) | 7,966,734 | `ff68775bf59b322e44d9388d57d744158bb30b99a0d2fed40c692ab66375747e` |
| console `syswow64/d3d9.dll.3.1.1` | 7,856,142 | `265888c31ca78dffa290c39cb7e50bfb02762590e41927906e46fb32f01497fa` |
| console `syswow64/d3d9.dll.prospero-full` | 7,991,310 | `45234c0001db15d157cb03d165e928c4837bed0ab7c690a9e45bbbf6c3235468` |

The stripped tree and the console DLL are the same size and not the same
bytes. The console DLL is the one to ship by hash. A rebuild from these
patches can be checked against `ff68775b…`.

`NFSMW/dxvk.conf` is `inis/dxvk.conf`: `d3d9.cachedDynamicBuffers = True`.
