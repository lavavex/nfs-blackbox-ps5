# speed.exe

The repo does not contain the exe. The console file is the DRM-free 1.3 exe
plus two edits. A byte compare of the three copies on build-pc (2026-10-11)
found no other differences.

## Base

The unpatched file is 6,029,312 bytes.

| | |
| --- | --- |
| md5 | `c0516b485065fabdd69579816b5df763` |
| sha256 | `80774c2e5d619b4f120b48d4462896fd504c263399d203a238769cffde1d253c` |
| PE characteristics | `0x010F` at file offset `0x11E` (large-address aware is off) |
| bytes at `0x242ED2` | `E8 29 A0 00 00` |

The same bytes are `~/mw-check/mw13/speed.exe` (mtime 2022-01-22),
`~/prospero-library/prefixes/nfs-blackbox/drive_c/Games/NFSMW/speed.exe`,
and `~/mw-re/speed.exe`. Near the start of the file the bytes spell
`*!ReLOADeD!*`. No download URL was saved on the Mac or the PC. The disc exe
is a different file (7,250,800 bytes, md5 `2788db37bb35ee52716810c47d550172`).

## Edits

1. Frame limiter. At file offset `0x242ED2`, replace `E8 29 A0 00 00` with
   `90 90 90 90 90`. The `E8` is a relative call. The displacement is
   `0x0000A029`. The next instruction is at `0x242ED7`, so the call target is
   file offset `0x24CF00`. Five NOPs drop the call.
   That edit alone is `~/mw-pc-test/speed.exe.limiter-patched`:
   md5 `3f06e9390ffc6e2fa996fa672220067e`,
   sha256 `63c9bce5bf157f4025b56eba5a7aa867773d2ee4dfc7530790672a7aa127fadd`.

2. Large-address aware. The COFF characteristics field is a little-endian
   uint16 at file offset `0x11E` (`e_lfanew` is `0x108`, and the field is 22
   bytes after that). Change the low byte from `0x0F` to `0x2F`, so the field
   goes from `0x010F` to `0x012F`. That sets `IMAGE_FILE_LARGE_ADDRESS_AWARE`
   (`0x0020`).

## Console file

Both edits. Still 6,029,312 bytes. md5 `af3d597280ba02423ab88c57a9dc5924`,
sha256 `fe3e406617afd5457b51263257fca22bd93cfd29f8ec702d8b8f6c83ea93cc48`.
Fetched from
`ftp://<console>:2121/data/prospero-win/prefixes/nfs-blackbox/drive_c/Games/NFSMW/speed.exe`
and checked against the copy in `src/speed.exe` on the Mac and the PC.
`src/` is gitignored.
