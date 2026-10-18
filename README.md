# NFS Black Box PS5

EA Black Box's Need for Speed games as homebrew apps on a jailbroken PS5, running the PC
versions through [prospero-win](https://github.com/mpereiraesaa/prospero-win). **Most Wanted
(2005)** is the first: app `PPSA99403`.

**You need your own copy of the game.** Nothing here contains it. The app installs the PC
game from a copy you put on the console (or on a USB drive) and adds the mods to it.

## What the app does

- Installs Most Wanted from your copy: 1.2 (the disc) or 1.3, unmodded.
  A 1.2 copy is brought up to 1.3 as it installs. Your copy is never changed.
- Adds the quality-of-life mods: Widescreen Fix, the ASI loader, XtendedInput (DualSense,
  with the touchpad as the cursor), the HUD Adapter, DSOAL, and a console keyboard.
- Optionally, the full **Xbox 360 look**: the Xbox 360 Stuff Pack 4.2, the converted Xbox
  360 movies, HD Content Support, Front-End Shadows and HD Reflections (about 3.7 GB more,
  from [nfs-blackbox-assets](https://github.com/lavavex/nfs-blackbox-assets)).
- Runs at 4K. **PS5 Options** in the game's own Options menu holds the app's settings: a
  performance overlay, the Xbox 360 look, and a repair on the next start. Triangle there
  restarts the game with them.

## On the console

Tested on a PS5 Pro, firmware 13.60, with etaHEN, kstuff and ShadowMount+. Install the
app from the homebrew catalog, open it, and follow the steps on its page:

1. Install the PC game on a computer from your disc (1.2) or a 1.3 copy. Leave it unmodded.
2. Copy its folder to the console as `/data/nfs-blackbox/base-games/NFSMW` (over FTP), or
   put it on a USB drive as `NFSMW`.
3. Open the app, pick the Xbox 360 look or not, and press Install. A copy on USB is copied
   to the console as part of the install. It takes about three minutes with the Xbox 360
   look on a wired connection.

After that the tile starts the game directly.

## Building

Everything builds with one script in a pinned container (Ubuntu 24.04 by digest, apt from a
dated snapshot), from the committed sources, so two builds give the same bytes:

```sh
git clone --recurse-submodules <this repo> nfs-blackbox-ps5
git clone https://github.com/lavavex/nfs-blackbox-launcher prospero-blackbox   # the launcher
cd nfs-blackbox-ps5 && tools/build.sh
```

It needs an x86-64 Linux machine with Docker and about 40 GB free; a clean build takes a few
hours. The app and its zip land in `release/game-apps/`, with `release/SHA256SUMS`.

What is where:

| Path | What |
| --- | --- |
| `tools/` | the build (`build.sh`), the install plan generator, the bundle, the app zip |
| `patches/` | our changes to prospero-win, Mesa, the PS5 Vulkan driver and DXVK |
| `profiles/nfsmw/` | Most Wanted as the plan sees it: hashes, inis, registry, the 1.2 → 1.3 diff |
| `art/` | the apps' Home-screen art (the games' covers are EA's) |
| `mods/` | our own mods: the PS5 Options page (`ps5-settings-mod`), a GPU stall fix for the Xbox 360 look (`X360StuffAsyncLuminance`), and two retired ones kept for reference (an ImGui menu, a D3D9 tracer) |
| submodules | prospero-win, PS5_Mesa, PS5_Vulkan, PS5_PayloadSDK, the native app boilerplate, at pinned commits |

`profiles/nfsmw/built/speed.exe` is the DRM-free 1.3 executable: SecuROM doesn't run under
Wine. It is no use without your own copy of the game. The 1.2 → 1.3 diff is the difference
between clean 1.2 and 1.3 installs (the launcher's `tools/make-upgrade.py`), checked to turn
one into the other file by file.

## Credits

- [prospero-win](https://github.com/mpereiraesaa/prospero-win), PS5_Mesa and PS5_Vulkan by
  mpereiraesaa, and the PS5 Payload SDK: Windows games on the PS5.
- Wine, Mesa, DXVK, LLVM: the stack underneath.
- The mods, each downloaded from its own page and pinned by SHA-256: Widescreen Fix and the
  Ultimate ASI Loader (ThirteenAG), NFS-XtendedInput (xan1242), NFSMW HUD Adapter (nlgzrgn),
  DSOAL (kcat), NFS HD Content Support, Front-End Shadows and HD Reflections (Aero_), the
  [Xbox 360 Stuff Pack](https://nfsmods.xyz/mod/1200) and the
  [Xbox 360 Converted Movies](https://nfsmods.xyz/mod/3006) by their authors on nfsmods.xyz.
- Need for Speed: Most Wanted is EA's. This project is not affiliated with EA or Sony.

## License

GPL-3.0 (`LICENSE`). The patches in `patches/` follow the licences of the projects they
change, and the submodules keep their own.
