# PC Display options: one new row

Target: DRM-free Need for Speed Most Wanted PC v1.3 `speed.exe`.

| | |
|---|---|
| Path | `profiles/nfsmw/built/speed.exe` |
| Size | 6,029,312 bytes |
| sha256 | `80774c2e5d619b4f120b48d4462896fd504c263399d203a238769cffde1d253c` |
| Image base | `0x400000` |

For `.text`, `.rdata`, and the on-disk part of `.data`, file offset equals RVA. There is no relocation directory. The two runtime patches (frame-limiter NOP at file `0x242ED2`, large-address byte at file `0x11E`) are not in this file. Addresses below are virtual addresses in this file.

The reconstructed C is `fe_options.c` / `fe_options.h` in this folder, and the same files under `~/Projects/mw-re/decomp/speed-1.3/`. It is a reading of this exe. The Ghidra tree in `mw-re/decompiled/exe_c` is the encrypted disc exe and was not used.

Checked with Capstone 5.0.7 against this exe, and by parsing `OptionsPCDisplay.fng` out of decompressed `FrontB.lzc` (`~/Projects/mw-re/mw13/FRONTEND/FrontB.lzc` → `FrontB.bin`, 6,677,024 bytes, 148 inner JDLZ packages).

## 1. How a Display row is built

The PC options screen is allocated at `0x570A96` (`push 0x158`, `call 0x652AD0`) and constructed at `0x54FD60`. Its vtable is `0x89CF60`, four slots. Slot 3 (`+0x0C`) is `0x545C50`.

```
0x54fd8b: mov dword ptr [esi], 0x89cf60
0x54fdc9: mov dword ptr [0x91ca20], eax    ; eax = screen+0x144
0x54fddb: mov byte ptr [esi + 0x154], bl   ; bl is 0
0x54fdeb: add ecx, 9
0x54fdee: mov dword ptr [esi + 0x10c], ecx ; 9, or 10 when byte +0x124 is set
0x54fe4e: call 0x545c50
```

`0x91CA20` points at `screen+0x144`, not at a separate options singleton.

The base menu constructor `0x593920` stores the row-name templates and starts the row index at 1:

```
0x593988: mov dword ptr [esi + 0xb0], 0x8a1944   ; "OPTION_NAME_"
0x593992: mov dword ptr [esi + 0xb4], 0x8a1934   ; "OPTION_DATA_"
0x59399c: mov dword ptr [esi + 0xb8], 0x8a1924   ; "OPTION_IMAGE_"
0x5939a6: mov dword ptr [esi + 0xbc], 0x8a1914   ; "OPTION_BACKING_"
0x5939b0: mov dword ptr [esi + 0xc0], 0x8a1908   ; "LEFT_ARROW_"
0x5939ba: mov dword ptr [esi + 0xc4], 0x8a18f8   ; "RIGHT_ARROW_"
0x5939c4: mov dword ptr [esi + 0xc8], 0x8a18f0   ; "SLIDER_"
0x593a39: mov dword ptr [esi + 0x104], 1
```

`screen+0x10C` is a scroll-marker threshold. `0x593B40` is `cmp edi, [esi+0x10C]` / `ja` to a fallback marker. It is not the number of name slots.

`0x545C50` (thiscall, screen in ecx) always starts with `call 0x588C90`, which walks the circular list at `screen+0x2C`, destroys every widget, and sets `screen+0x104` back to 1. The message handler at `0x545A91` flips `screen+0x154` and calls `[vtable+0x0C]`, so opening the advanced page rebuilds from an empty list.

The category is `[[[0x91CF90]+0x10]+0x24]`. `cmp eax, 9` / `ja 0x545DEF`, then `jmp [eax*4+0x545DFC]`.

| Category | Address | What it calls |
|---|---|---|
| 0 | `0x545D81` | `0x529310` |
| 1 | `0x545D93` | `0x5295F0` (PC display) |
| 2, 5, 6, 7, 8 | `0x545DEF` | nothing, then `0x5A5EA0` |
| 3 | `0x545DA5` | `0x529C10` |
| 4 | `0x545DB7` | `0x529F10` |
| 9 | `0x545DE8` | `0x52A290` |

The call that builds the display page:

```
0x545d93: mov ecx, esi
0x545d95: call 0x5295f0          ; bytes E8 56 38 FE FF
0x545d9a: push 0
0x545d9c: mov ecx, esi
0x545d9e: call 0x5a5ea0
```

`0x5295F0` has one caller. `screen+0x154 == 0` (the constructor value) adds two widgets. Nonzero adds thirteen. The init byte on most of the long-page rows is `(global > 0)`; the row is still added when that byte is 0.

| Index | Size | Vtable | Init byte | First hash written by its Draw |
|---|---|---|---|---|
| short 1 | `0x9C` | `0x89BB40` | 1 | `OPT_VO_LOD` |
| short 2 | `0x5C` | `0x89BB04` | 1 | `OPT_VO_RESOLUTION` |
| long 1 | `0x5C` | `0x89BB04` | 1 | resolution |
| long 2 | `0x9C` | `0x89BC78` | `[0x901890] > 0` | `OPT_DO_AA` |
| long 3 | `0x9C` | `0x89BCB8` | `[0x9018A0] > 0` | `OPT_DO_TEXTURE_FILTERING` |
| long 4 | `0x5C` | `0x89BD34` | 1 | `OPT_DO_VSYNC` |
| long 5 | `0x9C` | `0x89BC38` | `[0x90187C] > 0` | `OPT_DO_WORLD_LOD` |
| long 6 | `0x9C` | `0x89BD70` | `[0x90185C] > 0` | `OPT_DO_ROAD_REFLECT_DETAIL` |
| long 7 | `0x9C` | `0x89BE28` | `[0x9018B8] > 0` | `OPT_DO_SHADOW_DETAIL` |
| long 8 | `0x5C` | `0x89BBFC` | `[0x901880] > 0` | `OPT_DO_CAR_GEOMETRY_DETAIL` |
| long 9 | `0x5C` | `0x89BBC0` | `[0x901840] > 0` | `OPT_DO_CAR_REFLECT_DETAIL` |
| long 10 | `0x9C` | `0x89BB80` | `[0x901844] > 0` | `OPT_DO_CAR_REFLECT_UPDATE_RATE` |
| long 11 | `0x5C` | `0x89BDB0` | `[0x901898] > 0` | `OPT_DO_RAIN_ENABLE` |
| long 12 | `0x5C` | `0x89BCF8` | `[0x901884] > 0` | `OPT_DO_OVER_BIRGHT` |
| long 13 | `0x5C` | `0x89BDEC` | `[0x9018B0] > 0` | `OPT_DO_VISUAL_TREATMENT` |

A toggle is `push 0x5C` / `call 0x652AD0` / `push 1` / `call 0x589300` / `mov [widget], vtable` / `push 1` / `push widget` / `call 0x588570`. A slider is the same shape with `0x9C`, `0x589540`, and `0x588860`. `0x589300` ends `ret 4`. `0x588570` ends `ret 8`. The second argument of `0x588570` is not read.

`0x588570` binds the row, then links it:

- Label: `0x573CC0(screen, "OPTION_NAME_")`. That joins the template with `"%d"` (`0x89D764`), prints `screen+0x104`, FEHashUpper's it (`0x5AF1C0`), and `0x5251E0` requires an FE object of type 2.
- Value: the same with `"OPTION_DATA_"`.
- Arrows: `0x573D10`. Same numbering, but `0x5251A0` requires type 1. If the numbered arrow is missing it calls `0x573140` with the format `"%s0"` (`0x8A0BB8`), which is how `LEFT_ARROW_0` / `RIGHT_ARROW_0` are found.
- The widget's node is `widget+4`, inserted before the head at `screen+0x2C`. `screen+0x104` increments. Draw (vtable `+0x0C`) runs before `0x588570` returns, after the label and value pointers are stored at `widget+0x2C` and `widget+0x30`.

`OptionsPCDisplay.fng` (FrontB inner JDLZ at `0x516C8C`, decompressed 76,316 bytes, 150 ObjD chunks) contains `OPTION_NAME_1`..`_15` and `OPTION_DATA_1`..`_15`, and `RIGHT_ARROW_0` only. It has no `LEFT_ARROW_*`, no per-index right arrows, no backings, no sliders. Stock rows already live with that: a missing arrow lookup returns null. The short page's next index is 3. The long page's next index is 14. Both names exist in this package. There is no `OPTION_NAME_16`.

`Pause_PCDisplayOptions.fng` is the package name pushed for the in-game screen (string at `0x89718C`). It is not one of the 148 packages in FrontB. `Options.fng` in FrontB has names 1..10 only, and it is not the package `0x5295F0` fills. The plugin looks the next name up and skips the row when the lookup returns null.

The header object `0xCE757C8A` (type 2) is set to `H_ADVANCED` (`0x1459065C`) on the short page and `H_BASIC` (`0x9C88B5C8`) on the long page. The page title is `OPT_VIDEO_TITLE` or, when `screen+0x124` is set, `PAUSE_VIDEO_OPTIONS_TITLE`, written by `0x525220` onto object `0x42ADB44C`.

## 2. Where a stock value lives

The resolution widget's vtable is `0x89BB04`, 15 slots. Slot 1 is `0x50F970`. Slot 3 is `0x51B7A0`. Slot 14 is `0x574670`, which is only `ret 4`.

`0x50F970` reads the action hash from the second stack argument (`[esp+0x10]` after `push esi` / `push edi`). `0x9120409E` decrements. `0xB5971BF1` increments. The index wraps across 0..5 and skips a zero dword in the six-entry table at `0x93DBEC`. It stores the index at `[[0x91CA20]+0]+0x6C`, sets `widget+0x2A`, calls slot 14, calls Draw, and `ret 8`. Any other hash leaves the index alone and still redraws.

`0x51B7A0` writes `OPT_VO_RESOLUTION` (`0xC96D57C9`) to `label+0x60` and one of the six `OPT_VO_PC_RES_*` hashes to `value+0x60`.

The block at `screen+0x144` is allocated lazily inside `0x5295F0`: `push 0x84` / `call 0x652AD0` / `call 0x50EFB0`, then `rep movsd` of `0x21` dwords from `0x9017B0` over that block, then `screen+0x148 = [0x982C00]`. A second `0x84`-byte block is allocated at `screen+0x14C` the same way; the stores that follow that constructor were not copied line by line.

Vsync and the other on/off rows keep their bytes in that block too (`Draw` reads `+0x74` and `+0x78` on the inner object for two of them). Where the block is written to the registry or to `user.reg` was not traced. The new row does not use the block.

## 3. Widescreen Fix does not add a row

Current Widescreen Fix (`Resolution.ixx` from the 2026-09-12 tree) replaces the function matching `A1 ?? ?? ?? ?? 56 57 8B 7C 24 ?? 81 FF`, which in this exe is `0x50F970`, and points the support-array loads at its own list. It hooks `GetRacingResolution` and, on a hit pattern, the wide string function.

In this exe that wide hook is `0x57E920` (three call sites, `0x515064`, `0x5151D4`, `0x524F70`). Its first instructions are `mov edx, [esp+0xC]` / `call 0x56BB80`. On a hit it copies the string and returns 1. The hook body in their source calls the original first and writes a `WxH` string only when the original returns 0.

Their narrow hook resolves to `0x56BC10` (first pattern hit `0x512BFB`). The hooked function in their source only calls the original.

Neither hook allocates a widget or calls `0x588570`. Extra resolutions are more values on the row that already exists. The safe shape is: match bytes, call the original, keep the failure case as a no-op.

## 4. Online Options, which is the page the plugin takes over

Category 9 is Online. The menu entry is created inside `0x528F30` (the only caller is `0x5452C4`, bytes `E8 67 3C FE FF`) when bit 3 of `[0x91CF90]+0x12C` is set:

```
0x5290d0: push 0
0x5290d1: push 0x7d347e05          ; OL_OPTIONS
0x5290d6: push 0x5a5e5137
0x5290db: call 0x586fa0           ; stores OL_OPTIONS at icon+0x2C
0x5290e2: mov dword ptr [edi], 0x89b900
```

`0x89B904` is `0x50F100`. On message `0x0C407210` it stores 9 at `[[[0x91CF90]+0x10]+0x24]`. The highlight updater `0x573870` (a slot on the options-menu vtables, including the one beside `MainOptions`) reads `icon+0x2C` and calls `0x515C00`. That is the only use of `OL_OPTIONS` in this exe. `OL_OPTIONS_TITLE` (`0x22B1C2E6`) is also used once.

The package router at `0x5455FA` sends category 9 to `0x545642`, the same path as categories 0, 3 and 4. With the online bit set that path pushes `Options.fng` (`0x89E55C`), including from the pause screen. `Pause_Options.fng` is pushed only when `screen+0x148` is set and the online bit is clear, which is the case where this menu entry is not created.

`Options.fng` is the FrontB inner JDLZ at decompressed offset `0x51203C` (decompressed 92,604 bytes). It contains `OPTION_NAME_1`..`_10`, `OPTION_DATA_1`..`_10`, and `LEFT_ARROW_` / `RIGHT_ARROW_` 1..10. It contains the title object `0x42ADB44C`. It does not contain `OPTION_NAME_11`.

Category 9's builder is `0x52A290`. The jump lands at `0x545DE8` (`mov ecx, esi`) and the call is the next instruction:

```
0x545de8: mov ecx, esi
0x545dea: call 0x52a290          ; bytes E8 A1 44 FE FF
0x545def: push 0
0x545df1: mov ecx, esi
0x545df3: call 0x5a5ea0
```

`0x52A290` sets the title, adds one toggle, sets the title again, and copies online settings:

```
0x52a2b7: push 0x966c856d        ; PAUSE_ONLINE_OPTIONS_TITLE when screen+0x124 is set
0x52a2be: push 0xe463b5f7        ; OPT_ONLINE_TITLE otherwise
0x52a2c3: push 0x42adb44c
0x52a2c9: call 0x525220
0x52a2f6: mov dword ptr [edi], 0x89c1b0
0x52a30d: call 0x588570          ; one row
0x52a315: push 0x22b1c2e6        ; OL_OPTIONS_TITLE, overwrites the title above
0x52a320: call 0x525220
0x52a34f: mov dword ptr [esi + 0x140], edi   ; 0x100-byte block from 0x57FE80
0x52a366: rep movsd              ; 0x40 dwords from [0x91CF90]+0x134
```

That one row's vtable is `0x89C1B0`. Slot 1 is `0x50F710`: left (`0x9120409E`) or right (`0xB5971BF1`) flips the byte at `[0x91CF90]+0x175`. Slot 3 is `0x51B4A0`: label `OL_OP_BADCONWARNING` (`0xD1009B34`), value `COMMON_ON` (`0x417B2604`) or `COMMON_OFF` (`0x70DFE5C2`). That is the whole online page.

The plugin hooks `0x5452C4` and `0x545DEA`, not `0x545D95`. Before the menu builder and before `0x52A290`, it merges the language table and replaces the text pointers of `OL_OPTIONS` and `OL_OPTIONS_TITLE` with `Black Box`. `0x52A290` still runs, so the 0x100-byte block is still filled and the title hash is still `OL_OPTIONS_TITLE`, which now looks up `Black Box`. After it returns, the plugin calls `0x588C90` (the same clear `0x545C50` used) and adds the Performance overlay row at the index the clear restores, which is 1. `OPTION_NAME_1` is in `Options.fng`. If that lookup returns null, the stock row has already been cleared and the log says so.

## 5. A new row, and text that is not a stock string

The Video page's builder `0x5295F0` is not hooked. Its first bytes are an SEH frame, and the old call site `0x545D95` is left alone. The row is added on the online page instead, after `0x588C90` has put `screen+0x104` back to 1.

The plugin copies the 15-slot table from `0x89BB04` only when slot 1 is still `0x50F970` and slot 3 is still `0x51B7A0`, then replaces those two slots. The online row's own table is `0x89C1B0`; the resolution table is the one that was checked, and its slot 0 (`0x528430`) frees the widget with the game's free (`0x652B00`) on the next rebuild. The widget is allocated with the game's malloc (`0x652AD0`, a `jmp`). `Options.fng` has `OPTION_NAME_1` for the index the clear restores.

Text is not a patch of `GetLocalizedString`, because Widescreen Fix already hooks `0x56BC10` and `0x57E920`. Both searches end in `0x56BB80`:

```
0x56bb81: mov ebx, dword ptr [0x91cf80]
0x56bb87: test ebx, ebx
0x56bb89: jne 0x56bb8f
0x56bb8b: xor eax, eax
0x56bb8d: pop ebx
0x56bb8e: ret
0x56bb91: mov esi, dword ptr [0x91cf78]
```

Records are 8 bytes, hash then `char *`, sorted ascending unsigned. The count is the number of records. `0x56BC10` / `0x56BC60` fall through to hash `0x9BB9CCC3` (`ERROR_DEFAULT_STRING`) on a miss. `0x460CA0` is a bounded byte copy, so an ASCII string in that table is what the wide copy consumes.

The plugin allocates a new table with `0x652AD0`, merges six records in hash order, and stores the pointer and count. It does not free the previous table. If the game later replaces the pointer, the next menu or online-page build merges again. A collision on `OL_OPTIONS` or `OL_OPTIONS_TITLE` keeps the hash and stores the plugin's `Black Box` pointer. A collision on one of the four new hashes keeps the game's string, and the row is not added. The four new hashes are FEHashUpper (same as bStringHash on already-uppercase text: start `0xFFFFFFFF`, `hash = char + 33 * hash`) of names that occur nowhere in this exe, in `English.bin`, or in `Labels.bin`:

| Hash | Key | Text the row shows |
|---|---|---|
| `0x65CD36F0` | `BB_PERF_OVERLAY` | Performance overlay |
| `0xCE4EC5C9` | `BB_PERF_OFF` | Off |
| `0xCE4EA0D7` | `BB_PERF_FPS` | Frame rate |
| `0x9822D061` | `BB_PERF_FULL` | Full |

The two replaced records are not new keys. Each has one use in this exe, listed in section 4. The English.bin wording of those keys was not decoded; the hash table there stores an offset, and that offset was not turned back into the source string. The replacement does not depend on that wording.

Draw calls `0x515C00` (cdecl, object, hash), which is the same flag-and-notify sequence as `0x51B7A0`, storing the hash at object `+0x60`. Stock rows on other pages keep their own hashes.

## 6. Left, right, and Cross

The row's Act matches `0x50F970`: thiscall, `ret 8`, hash in the second stack argument. `0x9120409E` moves Off ← Frame rate ← Full ← Off. `0xB5971BF1` moves the other way. Any other hash does not change the value; the row still sets `widget+0x2A`, calls slot 14, and redraws.

Those two hashes are the ones the resolution row treats as left and right. Which XtendedInput binding produces them was not traced. Cross is not one of them in `0x50F970`, so this row does not treat Cross as a value change either. If Cross is an accept at the menu level, it behaves as it does on Resolution.

The value is not stored in the `0x84`-byte options block. It is 0, 1, or 2 in the plugin. On a real change the plugin rewrites `Z:\data\prospero-blackbox\settings-ingame.txt`: the `overlay=` line becomes `off`, `fps`, or `full`, and every other line of that file is copied. The file is created if it is missing. A file over 8192 bytes is left untouched. The plugin reads the same line when it loads.

The launcher, as of this tree, reads `ingame-<title>.txt` (`ingame-PPSA99403.txt` for MW), not `settings-ingame.txt`. This row does not feed the launcher.

## The plugin

`FEngPerformanceOverlay.c`, built with:

```
i686-w64-mingw32-gcc -O2 -shared -static-libgcc -s -std=c99 \
  -Wl,--no-insert-timestamp -o FEngPerformanceOverlay.asi FEngPerformanceOverlay.c
```

Compiler: i686-w64-mingw32-gcc 16.2.0. Output of this build: 18,432 bytes, sha256 `6a984112a1da59f071b1d758adf1f4828d090181c0e1ad4ea945ec29c6e2a108`.

The disassembly of that ASI shows:

- the menu hook is `push ecx` / `call menu_before` / `pop ecx` / `mov $0x528F30, %eax` / `call` / `ret`
- the online hook calls `menu_before`, then `0x52A290`, then `overlay_after_build`
- `overlay_after_build` calls `0x588C90` before it looks up `OPTION_NAME_`
- Act is `ret $0x8`, compares `0x9120409E` and `0xB5971BF1`, and calls vtable `+0x38` then `+0x0C`
- Draw is a plain `ret`, reads `widget+0x2C` and `widget+0x30`, and calls `0x515C00`
- the name lookup is `mov ecx, screen` / `push 0x8A1944` / `call 0x573CC0`
- the widget is `call 0x652AD0` (size `0x5C`), `call 0x589300`, then `call 0x588570` with ecx = screen

If `0x5452C4` is not `E8 67 3C FE FF`, or that call's target is not `0x528F30`, the menu rename is not installed. If `0x545DEA` is not `E8 A1 44 FE FF`, or that call's target is not `0x52A290`, the page is not hooked. A vtable whose slot 1 is not `0x50F970` or whose slot 3 is not `0x51B7A0` leaves the stock online row in place. Each miss is logged with the bytes that were there.

Log file: same path as the ASI, with `.log` instead of `.asi`.

## What was not run

The game was not started. There is no Wine on this Mac, and no screenshot of the row or of the file it writes. Compile output and the disassembly above are the check that was done.

Also not traced: the registry write of the stock options block, the XtendedInput hash for Cross, the body of `0x57FE80`, and the English wording of `OL_OPTIONS`. The Video page is unchanged. On a package that has no `OPTION_NAME_1`, the log says the stock row was cleared and the new row was not added.
