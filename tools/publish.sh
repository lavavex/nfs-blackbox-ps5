#!/usr/bin/env bash
# The public copy of this repo, made fresh from the committed HEAD: a new history of one
# commit, without what stays private, scanned before anything can go up. It never pushes:
# publishing is the owner's call (AGENTS.md rule 3); the last line says how.
#
#   tools/publish.sh OUT
#
# The public README is publish/README.md (this repo's README.md describes the private
# workspace); publish/ itself stays out (publish/assets-README.md is the assets repo's).
# Left out: HANDOFF.md and AGENTS.md (this workspace's notes), .github/ (it builds from the
# private manager branch), and the third-party mod zips in mods/ (the plan downloads each
# from its own page, pinned by SHA-256).
# Kept: tools/, patches/, profiles/ (with the DRM-free speed.exe and the 1.2 -> 1.3 diff:
# the owner's decision of 2026-10-13), art/ (the owner's decision of 2026-10-18), our mods'
# sources, and the submodules at their pins. The launcher's source is tools/publish-launcher.sh.
#
# The scan fails the export on, in text files: a LAN address, this Mac's home folder or user
# name, an e-mail address of the owner, anything shaped like a CD key; and on any file of
# the game's own kinds (.BUN, .BIN, .LZC, ...). It lists every file over 1 MB for a look by
# eye (the DRM-free speed.exe and the 1.2 -> 1.3 diff are expected there).
set -euo pipefail
W=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
out=${1:?usage: tools/publish.sh OUT}
[[ -e $out ]] && { echo "$out exists: give a new folder" >&2; exit 2; }
cd "$W"
[[ -z $(git status --porcelain -- . ':!prospero-win' ':!PS5_Mesa' ':!PS5_Vulkan') ]] ||
    { echo "uncommitted changes: commit first, the export is HEAD" >&2; exit 2; }
head=$(git rev-parse --short HEAD)

# The files, from HEAD.
mkdir -p "$out"
excludes=(HANDOFF.md AGENTS.md .github publish 'mods/*.zip' 'mods/companions' 'mods/dsoal')
pathspec=(.)
for e in "${excludes[@]}"; do pathspec+=(":(exclude,glob)$e" ":(exclude,glob)$e/**"); done
git archive --format=tar HEAD -- "${pathspec[@]}" | tar -x -C "$out"
git show HEAD:publish/README.md > "$out/README.md"
[[ -f $out/LICENSE ]] || { echo "no LICENSE at HEAD" >&2; exit 2; }
# Submodules come out of git archive as empty folders: they become gitlinks again below.
mapfile -t subs < <(git ls-files -s | awk '$1 == "160000" {print $2, $4}')

# The scan.
python3 "$W/tools/publish-scan.py" "$out"

# A new history of one commit, the submodules at the same pins.
cd "$out"
git init -q -b main
git add -A
for sub in "${subs[@]}"; do
    read -r sha path <<<"$sub"
    rmdir "$path" 2>/dev/null || true
    git update-index --add --cacheinfo "160000,$sha,$path"
done
git commit -q -m "NFS Black Box PS5, from nfs-blackbox-ps5 $head"
git log --oneline | cat
echo "$(git ls-files | wc -l | tr -d ' ') files in $out. Nothing was pushed: with the owner's OK,"
echo "  git -C $out remote add origin <the public repo> && git -C $out push -u origin main"
