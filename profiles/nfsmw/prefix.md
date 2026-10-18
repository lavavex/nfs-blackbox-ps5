# Prefix

## The bare prefix the manager unpacks (2026-10-12)

`tools/make-bare-prefix.py OUT.zip` (or `tools/build-prospero-win.sh prefix`) makes it from
the trees `build-prospero-win.sh` built:
- prospero-win's `pw_install.py` creates the prefix from `bare-prefix.yml` with the built host
  Wine and our DXVK (3.1.1 + patches/dxvk, see `dxvk.md`), given as a local archive;
- it is packed as `pw_prefix.py` pushes a prefix to the console: `.pw-symlinks` tables,
  `wowprospero.dll` as the WoW64 x86 CPU, `wowprospero.dll` and `wow64native.dll` in
  `system32`, the `prospero` user's folders;
- registry key times, the machine/video IDs, device container IDs, device install times,
  the display driver's date and the machine's name (`PROSPERO`/`prospero`, empty domain)
  are fixed, so the same build trees give the same zip on any day and any machine name;
- so are the values wineboot takes from the building machine's CPU (`NUMBER_OF_PROCESSORS`
  and `PROCESSOR_*`, set to the PS5's 8 Zen 2 cores: Family 23 Model 71) and the display
  adapter's LUID (0x400), handed out at run time in an order that varies.

It is made in the build image (`tools/build.sh`), on a machine that runs 32-bit x86 code
(on a Mac, the emulated `nfsbb-x86` VM; HANDOFF section 2). `make-bare-prefix.py` refuses a
prefix whose `syswow64` is missing or that holds crash dumps (Rosetta in a Linux VM made one:
910 files, no 32-bit side), and pins `.update-timestamp`.

2026-10-13, two clean builds: 1,814 files (865 in `syswow64`), 165 folders, the same bytes.
The old Mac-built prefix (host Wine under macOS Rosetta) had 1,816: the extra two were the
Mac-only `winemac.drv` (32/64-bit).

The 2026-10-11 comparison of the hand-made console prefix with a fresh one (build-pc) is in git
history (`profiles/nfsmw/prefix.md` and `prefix-size-diff.json` at `06645b1`).
