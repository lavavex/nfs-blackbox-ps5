#!/usr/bin/env bash
# Publish MW's repacked Xbox 360 zips (tools/make-x360-assets.py -> release/assets/) as the
# GitHub release the plan's URLs name: lavavex/nfs-blackbox-assets, tag nfsmw-x360-1.
# Public: run it only with the owner's go. Each zip is checked against
# profiles/nfsmw/x360-assets.json before it goes up, and the release's assets after.
#
#   tools/upload-x360-assets.sh            # needs gh, logged in with write access
set -euo pipefail
W=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
repo=lavavex/nfs-blackbox-assets tag=nfsmw-x360-1
mapfile -t zips < <(python3 -c '
import json, sys
a = json.load(open(sys.argv[1]))
for z in a["stuff_pack"] + a["movies"]: print(z["name"], z["sha256"], z["size"])' "$W/profiles/nfsmw/x360-assets.json")
files=()
for line in "${zips[@]}"; do
    read -r name sha size <<<"$line"
    f=$W/release/assets/$name
    [[ $(stat -c %s "$f" 2>/dev/null || stat -f %z "$f") == "$size" ]] &&
        [[ $(shasum -a 256 "$f" | cut -d' ' -f1) == "$sha" ]] ||
        { echo "$f isn't the pinned zip: run tools/make-x360-assets.py" >&2; exit 2; }
    files+=("$f")
done
gh repo view "$repo" >/dev/null 2>&1 ||
    { echo "$repo doesn't exist: gh repo create $repo --public (the owner's call)" >&2; exit 2; }
gh release view "$tag" -R "$repo" >/dev/null 2>&1 ||
    gh release create "$tag" -R "$repo" --title "Most Wanted: Xbox 360 graphics, 1" \
        --notes "The Xbox 360 Stuff Pack 4.2 (without the glass shader) and the Xbox 360 Converted Movies for Need for Speed: Most Wanted (2005), repacked from their Google Drive releases for the NFS Black Box PS5 apps (the optional Xbox 360 look). Credit and thanks to their authors."
gh release upload "$tag" -R "$repo" --clobber "${files[@]}"
gh release view "$tag" -R "$repo" --json assets --jq '.assets[] | "\(.size)  \(.name)"'
