# Most Wanted, as it is on the console

Facts the plan generator (`tools/make-nfsmw-plan.py`) reads. This folder does not contain
the game or a CD key. `system-ea.reg` has no `ergc` block. `user-ea.reg` drops the default
value, which was the placeholder `INSERTYOURCDKEYHERE`.

| File | What it is |
| --- | --- |
| `game-manifest.json` | 165 console files under `Games/NFSMW` that are new or different from the full 1.3 tree, with size, sha256, and which mod |
| `sources.md` | Download URL, archive name, archive sha256, and paths inside each archive |
| `inis/` | The console's mod inis, `alsoft.ini`, and `dxvk.conf` |
| `user-ea.reg`, `system-ea.reg` | The live EA Games registry blocks |
| `built/speed.exe` | The DRM-free 1.3 exe the plan installs (private; see `speed-exe.md`) |
| `nfsmw-1.2-to-1.3.zip` | Our 1.2 → 1.3 diff (private) |
| `bare-prefix.yml` | The recipe for the bare prefix (`tools/make-bare-prefix.py`) |
| `speed-exe.md` | Byte patches from the DRM-free 1.3 exe to the console exe |
| `dxvk.md` | Our DXVK 3.1.1 build and the lean branch |
| `asi.md` | Our two ASIs: where each comes from |
| `prefix.md` | The bare prefix and how it is made reproducibly |
