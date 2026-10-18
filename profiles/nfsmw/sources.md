# Where the Most Wanted mods came from

Measured 2026-10-11. Archive sha256 values below are of the files in this
repo, except the ASI loader zip, which is the copy in build-pc's
`~/.cache/prospero-win/` and is not committed. The game, `speed.exe`, and
the Microsoft `d3dx9` DLLs are not in the repo. Their names, sizes, and
sha256 are in `game-manifest.json` and `prefix.md`.

HD fonts (nfsmods mod 1195) and the Xbox 360 cutscene rar (mod 2518) are in
`tools/nfsmw-x360.json` and are not installed.

## Xbox 360 Stuff Pack 4.2

| | |
| --- | --- |
| page | https://nfsmods.xyz/mod/1200 |
| files | Google Drive folder `19hqllTeyoBFXhT1F2-krIZdGmQxL0EaQ` |

There is no single archive. Each file's Drive id, size, and sha256 is
`tools/nfsmw-x360.json`. The glass shader is in that list
(`Files/FX/COMPILED/glassreflectshader.fx.o`, 22,932 bytes, sha256
`353af4c0aeb1fcba5133473d2079658f81f1d5f04dbee685459b4fb6a4cb6558`). On the
console that file is `FX/COMPILED/glassreflectshader.fx.o.off`.

## HUD Adapter 1.0.0.800

| | |
| --- | --- |
| page | https://nfsmods.xyz/mod/638 |
| source | https://github.com/nlgzrgn/NFSMWHUDAdapter/ |
| archive | `mods/companions/658865-NFSMW HUD Adapter v1.0.0.800.zip` |
| url | https://nfsmods.xyz/uploads/mods/638/658865-NFSMW%20HUD%20Adapter%20v1.0.0.800.zip |
| size | 42,211 |
| sha256 | `d0c034879a797bbf095e1cd679ae5ac3701089cb547ca77292fed6ad7270a69b` |

Members: `Read Me.txt`, `scripts/NFSMWHUDAdapter.asi`. No ini.

## Front-End Shadows

| | |
| --- | --- |
| page | https://nfsmods.xyz/mod/3005 |
| source | https://github.com/AeroWidescreen/NFSMWFEShadows |
| archive | `mods/companions/9cbcd3-NFSMWFEShadows.zip` |
| url | https://nfsmods.xyz/uploads/mods/3005/9cbcd3-NFSMWFEShadows.zip |
| size | 996,309 |
| sha256 | `97c9980d0237250d66d39311930a595f7938cc0b84c7112b82deebfa06e90ac5` |

Members under `NFSMWFEShadows/`: `dinput8.dll` (2,353,664 bytes; not the
`dinput8.dll` on the console), `scripts/NFSMWFEShadows.asi`,
`scripts/NFSMWFEShadows.ini`, plus `readme.txt` at the zip root.

## HD Reflections

| | |
| --- | --- |
| page | https://nfsmods.xyz/mod/3363 |
| source | https://github.com/AeroWidescreen/NFSHDReflections |
| archive | `mods/companions/a486f3-NFS HD Reflections.zip` |
| url | https://nfsmods.xyz/uploads/mods/3363/a486f3-NFS%20HD%20Reflections.zip |
| size | 636,650 |
| sha256 | `daaa6aefd4b0918fb8adaadcd80a4cc4322d8063270a31aeb3f75e07de386d78` |

The zip also covers Carbon, ProStreet, Underground, and Underground 2. The
Most Wanted files are
`NFS HD Reflections/NFSMWHDReflections/scripts/NFSMWHDReflections.asi` and
`NFSMWHDReflections.ini`.

## HD Content Support

Already pinned in `tools/nfsmw-x360.json` (`hdcs`). The zip is also at
`mods/companions/NFS HD Content Support.zip`.

| | |
| --- | --- |
| page | https://nfsmods.xyz/mod/2151 |
| url | https://nfsmods.xyz/uploads/mods/2151/4c35cb-NFS%20HD%20Content%20Support.zip |
| size | 527,322 |
| sha256 | `60c900a8f01b12ab79625194833a83feb64c1caee07b8177c8d356ae587d5197` |

Most Wanted member:
`NFS HD Content Support/NFSMWHDContentSupport/scripts/NFSMWHDContentSupport.asi`.
No Most Wanted ini. Underground and Underground 2 have inis in the same zip.

## Xbox 360 Converted Movies

| | |
| --- | --- |
| page | https://nfsmods.xyz/mod/3006 |
| nfsmods archive | `mods/companions/8b951e-X360 Converted Movies - Download link.zip` |
| url | https://nfsmods.xyz/uploads/mods/3006/8b951e-X360%20Converted%20Movies%20-%20Download%20link.zip |
| size | 627 |
| sha256 | `478ec71a99ef259171614313a184ec19b39fa69c1519e4189a5d379e6db1396a` |

That zip is a link, not the movies. Its `Download (Google Drive).url` points
at https://drive.google.com/drive/folders/168qRM9BBFuiSDfU727oX0a18Blkt7lUU?usp=sharing.
The files that match the console were downloaded from the nested folders
`1x1j5Xwq3RGCNqoeh-vjxxD3HRnqkIH1O` (Mod) and
`1gPZhTidT0Ub82WpZgO5STJWHjxX2xT_R` (MOVIES). There is no one archive sha256.
The vp6 files are not in the repo. All 33 match the console.

| File | Drive id |
| --- | --- |
| `attract_movie_english_ntsc.vp6` | `1U6l-iXOx5fbEhVsV9z_JiUsOcHoha4CE` |
| `blacklist_01_english_ntsc.vp6` | `1PyFUQy5_jdeJtwsIpOMU2hU3dBPMHfF1` |
| `blacklist_02_english_ntsc.vp6` | `1othMljwmQWUWhgwg1P7XxEY66ttuRYUj` |
| `blacklist_03_english_ntsc.vp6` | `1UZgJ2Wsqmife-IhIFj6egpzf5g1nzeFT` |
| `blacklist_04_english_ntsc.vp6` | `1jXgkkok7g7PZF1FoMExVhUAAmAgYBkH_` |
| `blacklist_05_english_ntsc.vp6` | `1RCnG84LqPM9sv3--lB1r-MkYTjcqWobu` |
| `blacklist_06_english_ntsc.vp6` | `1FDrHay-7Hxowh7jasIG14zyPVo5LUchI` |
| `blacklist_07_english_ntsc.vp6` | `1kVVuHuGvWkVDsWc16uKmCdCBZXOBy0bd` |
| `blacklist_08_english_ntsc.vp6` | `1LRtHwLxqIQCf2hTkDQ3kvmbiQnC5HkE8` |
| `blacklist_09_english_ntsc.vp6` | `1kEB3aUtx_WdVUyBI5me5ya7QhnLreO6k` |
| `blacklist_10_english_ntsc.vp6` | `1VumIKpAuVWDd3O9Yx3gQQ9pVnErKXjwa` |
| `blacklist_11_english_ntsc.vp6` | `1ZANL5NxjjsvK-22gsLUkdju07Yhr0vQl` |
| `blacklist_12_english_ntsc.vp6` | `1vVKMpItG6yIbzMrtD-RQO9yghZqhlSP4` |
| `blacklist_13_english_ntsc.vp6` | `10cJza871zy3OPy2ZoLYjLUMklG2jnl6F` |
| `blacklist_14_english_ntsc.vp6` | `17PCcUVsDKr2AtHtjw4ZGBhoejnEVuFKT` |
| `blacklist_15_english_ntsc.vp6` | `1A1a24T-M5RKG_fomvWHWLPwr7X5EAzPh` |
| `bounty_tutorial_english_ntsc.vp6` | `1PEarMpWLw2Qp-GBd0p1rMlDY_GBKOl1E` |
| `drag_tutorial_english_ntsc.vp6` | `1j1FyOywJSyfecnoikrjAToMTxuoShwaF` |
| `eahd_bumper_english_ntsc.vp6` | `1t6NueKTqjUXuTEMgvHJKrDre9aFHilV2` |
| `ealogo_english_ntsc.vp6` | `1DBaj9pqNv2FolIMphHcS3mjZiOuHXW4u` |
| `psa_english_ntsc.vp6` | `1BedECfxAzSrthdT97QuAR4ULPZOHZzWh` |
| `pursuit_tutorial_english_ntsc.vp6` | `1qYJiVtuaO0N7okIWX2RgOcoBYkYLC5Nq` |
| `speedtrap_tutorial_english_ntsc.vp6` | `1A7gYIlR49Rw8NgpgZUP7go8t7GKHl24R` |
| `storyfmv_bla134_english_ntsc.vp6` | `1uvC7eoIjKSnocpP9QBVjiPOVvN2BjBE6` |
| `storyfmv_bus12_english_ntsc.vp6` | `1Rw1xPjjSqGvDNEehYKIUwiXv0HTY2iJA` |
| `storyfmv_cro06_coh06a_english_ntsc.vp6` | `1EQ9j2ltq2MOGImVyCjtdKhdGvwnVBiNb` |
| `storyfmv_her136_english_ntsc.vp6` | `1b3iLRtAohBtEsDY0_JJiUgIRXEapEVnH` |
| `storyfmv_pin11_english_ntsc.vp6` | `1YUbQLX75KT8t9Tq51wO4PNSQnAlYAB2z` |
| `storyfmv_rac01_english_ntsc.vp6` | `12TvAfGMbYJ3ceBgqioyRe7uwwLNVS2N6` |
| `storyfmv_rap30_english_ntsc.vp6` | `1ysAaqmVyIYWBaTkkuvX6SHI_5SXjxmp2` |
| `storyfmv_raz08_english_ntsc.vp6` | `1JFe9AFLHJ87R5DYDP8Qw8IXhafx7827Y` |
| `storyfmv_saf25_english_ntsc.vp6` | `1WhDp_V3pL4oIGbhWpBshH-shVBppLC-g` |
| `tollbooth_tutorial_english_ntsc.vp6` | `1ek_RA_tWRCjPyKDErgBaSFbvjqcMOrvd` |
| `ReadMe.txt` | `1moqOyl2Zq0N702Tnha21E82_TQ3cC10g` |

## XtendedInput 1.22

Pinned in `tools/make-nfsmw-plan.py`. The zip in the repo
keeps the upstream name.

| | |
| --- | --- |
| url | https://github.com/xan1242/NFS-XtendedInput/releases/download/1.22/Release-MW-Pack.zip |
| archive | `mods/Release-MW-Pack.zip` |
| recipe filename | `NFS-XtendedInput-1.22-MW.zip` |
| size | 1,520,654 |
| sha256 | `a78fbf5a2ba672557b23d7633175f391563a264c064762b2b50df88fb1a57fd0` |

Members: `dinput8.dll` (not the console copy), `EventReference.txt`,
`GLOBAL/XtendedInputButtons.tpk`, `README.md`, `scripts/nfs_cursor.cur`,
`scripts/NFS_XtendedInput.asi`, `scripts/NFS_XtendedInput.default.ini`,
`scripts/NFS_XtendedInput.ini`.

## Widescreen Fix

| | |
| --- | --- |
| url | https://github.com/ThirteenAG/WidescreenFixesPack/releases/download/nfsmw/NFSMostWanted.WidescreenFix.zip |
| archive | `mods/NFSMostWanted.WidescreenFix.zip` |
| recipe filename | `NFSMostWanted.WidescreenFix-20260912.zip` |
| size | 8,911,715 |
| sha256 | `7be912197ac19134d5a64dfe61225e4ef85d612c4f64b96045727c3bf6735df4` |

Members: `dinput8.dll`, `scripts/NFSMostWanted.WidescreenFix.asi`,
`scripts/NFSMostWanted.WidescreenFix.ini`,
`scripts/NFSMostWanted.WidescreenFix.tpk`. The `dinput8.dll` in this zip is
the same file as the ASI loader zip (5,418,336 bytes, sha256
`d5a059aa467a7a7127c8f6169f79fa63ff0f55986ee9eb2fd9a281bebf2aa2e6`), and that
is the console `dinput8.dll`.

## ASI loader v9.7.4

| | |
| --- | --- |
| url | https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases/download/v9.7.4/Ultimate-ASI-Loader.zip |
| cached as | `~/.cache/prospero-win/Ultimate-ASI-Loader-v9.7.4.zip` on build-pc |
| size | 3,910,913 |
| sha256 | `952cebfc30d525afc2bdbaca954329d405ded3aa688a83027354dae14dfd5c5f` |

The only file member is `dinput8.dll`, hash above.

## DSOAL r648

| | |
| --- | --- |
| page | https://github.com/kcat/dsoal/releases/tag/r648 |
| asset URL | https://github.com/kcat/dsoal/releases/download/r648/DSOAL.zip |
| archive | `mods/dsoal/DSOAL.zip` |
| size | 1,096,598 |
| sha256 | `66281302a69e23c59eb828674b817098d2087495028adcc4c3ad9339043cd252` |

The GitHub API lists that asset at 1,096,598 bytes. `curl -fL` of the asset
URL returned HTTP 404 on 2026-10-11, so this sha256 is the file already in
the repo, not a fresh download.

Inside it, `DSOAL_r648.zip` is 6,844,446 bytes, sha256
`5b4e47671dea3f9882f3686f9fd5c3a111dcd57c8170dd5bfe0f5a5f18e31966`
(inside `mods/dsoal/DSOAL.zip`). `DSOAL/Documentation/DSOAL-Version.txt`
says commit `2978c478d43f8a2da4c8885e850d5f290a162b2b`, tag r648, 2025-02-09.

Win32 members used by the game:

| Member | Size | sha256 |
| --- | --- | --- |
| `DSOAL/Win32/dsound.dll` | 291,328 | `592909fd14adadbb165579610b5201f864322c1f723130fd728d77fc65ea6bcb` |
| `DSOAL/Win32/dsoal-aldrv.dll` | 1,255,424 | `081cdc62a70536416ab140a86321540979326666bc3d3f8aa5cd51c7dc329af4` |
| `DSOAL/Win32/alsoft.ini` | 28,530 | `f5fdc31cc0b0d45c919336347e6294421684b0fa42cfb4b8942db03421c0ab32` |

The console `dsound.dll` and `dsoal-aldrv.dll` match those two. The console
`alsoft.ini` does not. It is the stock ini plus this block, and that longer
file is `inis/alsoft.ini` (28,756 bytes, sha256
`8095d96846443b30759ba5618d0ce76e8b0ccfd8d2ec7fa971e165fae90d2f6c`):

```
# prospero-win: the PS5 port takes 48 kHz stereo; a deeper buffer than the default 3 x 1024 for the console's coarser thread wake-ups.
[general]
channels = stereo
frequency = 48000
period_size = 1024
periods = 4
hrtf = false
```

DSOAL is in the game folder. `syswow64/dsound.dll` is Wine's builtin.

## Ours

`speed.exe` is described in `speed-exe.md`. No download URL was saved for
the DRM-free base. The exe is not in the repo.

`X360StuffAsyncLuminance.asi` and `NFSMWConsoleKeyboard.asi` are in `asi.md`.
`dxvk.conf` is `inis/dxvk.conf`. The lean `d3d9.dll` is in `dxvk.md`; the
DLL is not in the repo.
