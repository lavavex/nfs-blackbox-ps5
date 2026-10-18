#!/usr/bin/env bash
# The launcher's public copy (the owner's choice, 2026-10-18: a fresh one-commit repo, no
# old history): the committed HEAD of the manager checkout beside this repo, the source of
# the eboot.bin in every game app's release. Its README is publish/launcher-README.md; the
# upstream README is kept as docs/prospero-blackbox-README.md. Scanned like tools/publish.sh
# (tools/publish-scan.py). It never pushes.
#
#   tools/publish-launcher.sh OUT [--manager ../prospero-blackbox]
set -euo pipefail
W=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
out=${1:?usage: tools/publish-launcher.sh OUT [--manager DIR]}
manager=$(dirname "$W")/prospero-blackbox
[[ ${2:-} == --manager ]] && manager=${3:?--manager DIR}
[[ -e $out ]] && { echo "$out exists: give a new folder" >&2; exit 2; }
cd "$manager"
[[ -z $(git status --porcelain) ]] || { echo "$manager has uncommitted changes: commit first" >&2; exit 2; }
head=$(git rev-parse --short HEAD) branch=$(git rev-parse --abbrev-ref HEAD)
[[ -z $(git ls-files -s | awk '$1 == "160000"') ]] || { echo "the launcher has submodules: not handled" >&2; exit 2; }

mkdir -p "$out"
git archive --format=tar HEAD | tar -x -C "$out"
mkdir -p "$out/docs"
mv "$out/README.md" "$out/docs/prospero-blackbox-README.md"
git -C "$W" show HEAD:publish/launcher-README.md > "$out/README.md"
[[ -f $out/LICENSE ]] || { echo "no LICENSE in the launcher" >&2; exit 2; }

python3 "$W/tools/publish-scan.py" "$out"

cd "$out"
git init -q -b main
git add -A
git commit -q -m "NFS Black Box launcher, from prospero-blackbox $branch $head"
git log --oneline | cat
echo "$(git ls-files | wc -l | tr -d ' ') files in $out. Nothing was pushed: with the owner's OK,"
echo "  git -C $out remote add origin <the public repo> && git -C $out push -u origin main"
