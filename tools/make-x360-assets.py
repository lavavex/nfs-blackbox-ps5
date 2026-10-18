#!/usr/bin/env python3
"""Repack MW's Xbox 360 files (Drive-only upstream) into the zips the manager downloads.

    tools/make-x360-assets.py [--cache DIR] [--out DIR]

Fetches the Xbox 360 Stuff Pack 4.2 files (tools/nfsmw-x360.json) and the 33 Xbox 360
Converted Movies (profiles/nfsmw/game-manifest.json hashes, sources.md Drive ids) into the
cache (default ~/nfsmw-mods, which tools/nfsmw-x360-prefetch.py also fills), each checked by
sha256, then writes:
  OUT/nfsmw-x360-stuff-pack-4.2.zip   the pack, game-relative paths, without the glass shader
  OUT/nfsmw-x360-movies-1.zip, -2.zip the movies (MOVIES/...), split to stay under 2 GB
and profiles/nfsmw/x360-assets.json (each zip's name, size, sha256 and members), which
tools/make-nfsmw-plan.py reads. The zips are stored (no compression), sorted, with fixed
times, so the same files give the same zips anywhere. They are published as GitHub release
assets of lavavex/nfs-blackbox-assets (tools/upload-x360-assets.sh); OUT defaults to
release/assets.
"""
import argparse, hashlib, importlib.util, json, os, re, sys, time, zipfile
sys.dont_write_bytecode = True  # importing nfsmw-x360.py leaves no __pycache__

R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("x360", os.path.join(R, "tools", "nfsmw-x360.py"))
x360 = importlib.util.module_from_spec(spec); spec.loader.exec_module(x360)
ap = argparse.ArgumentParser()
ap.add_argument("--cache", default=os.path.expanduser("~/nfsmw-mods"))
ap.add_argument("--out", default=os.path.join(R, "release", "assets"))
args = ap.parse_args()

STAMP = (2026, 1, 1, 0, 0, 0)
LIMIT = 2_000_000_000  # GitHub release assets must be under 2 GiB; stay well below
GLASS = "FX/COMPILED/glassreflectshader.fx.o"  # a white smear on interior glass under DXVK
RELEASE = "https://github.com/lavavex/nfs-blackbox-assets/releases/download/nfsmw-x360-1/"


def fetch(item, dest, tries=6):
    # Drive refuses requests for a while after many in a row: wait and ask again.
    for attempt in range(1, tries + 1):
        try:
            return x360.fetch(item, dest)
        except (Exception, SystemExit) as error:
            if attempt == tries:
                raise
            wait = (60, 120, 300, 300, 300)[attempt - 1]
            print(f"  {os.path.basename(dest)}: {type(error).__name__}; retry {attempt} in {wait} s", flush=True)
            time.sleep(wait)


pack = json.load(open(os.path.join(R, "tools", "nfsmw-x360.json")))["x360"]["files"]
stuff = []  # (game-relative path, cache file, sha256, size)
for f in pack:
    rel = f["path"][len("Files/"):]
    if rel == GLASS:
        continue
    dest = os.path.join(args.cache, "x360-stuff-4.2", f["path"])
    fetch(f, dest)
    stuff.append((rel, dest, f["sha256"], f["size"]))

manifest = json.load(open(os.path.join(R, "profiles", "nfsmw", "game-manifest.json")))["files"]
drive = dict(re.findall(r"\| `([^`]+\.vp6)` \| `([^`]+)` \|", open(os.path.join(R, "profiles", "nfsmw", "sources.md")).read()))
movies = []
for r in sorted((r for r in manifest if r["mod"] == "360 Converted Movies"), key=lambda r: r["path"]):
    name = r["path"].split("/")[-1]
    dest = os.path.join(args.cache, "x360-movies", name)
    fetch({"drive": drive[name], "size": r["size"], "sha256": r["sha256"]}, dest)
    movies.append((r["path"], dest, r["sha256"], r["size"]))
assert len(movies) == 33, len(movies)


def split(entries):
    parts, part, size = [], [], 0
    for e in entries:
        if part and size + e[3] > LIMIT:
            parts.append(part); part, size = [], 0
        part.append(e); size += e[3]
    return parts + [part]


def write(name, entries):
    path = os.path.join(args.out, name)
    with zipfile.ZipFile(path + ".part", "w", zipfile.ZIP_STORED, allowZip64=True) as z:
        for rel, src, _, _ in sorted(entries):
            info = zipfile.ZipInfo(rel, STAMP)
            info.external_attr = 0o100644 << 16
            with open(src, "rb") as f, z.open(info, "w", force_zip64=os.path.getsize(src) > 0x7FFFFFFF) as w:
                for block in iter(lambda: f.read(1 << 20), b""):
                    w.write(block)
    os.replace(path + ".part", path)
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    size = os.path.getsize(path)
    assert size < LIMIT + (1 << 20), f"{name}: {size} bytes"
    print(f"{h.hexdigest()}  {size:>11}  {name} ({len(entries)} files)", flush=True)
    return {"name": name, "url": RELEASE + name, "sha256": h.hexdigest(), "size": size,
            "members": [{"path": rel, "sha256": s, "size": n} for rel, _, s, n in sorted(entries)]}


os.makedirs(args.out, exist_ok=True)
zips = {"stuff_pack": [write("nfsmw-x360-stuff-pack-4.2.zip", stuff)],
        "movies": [write(f"nfsmw-x360-movies-{i}.zip", part) for i, part in enumerate(split(movies), 1)]}
out = os.path.join(R, "profiles", "nfsmw", "x360-assets.json")
open(out, "w").write(json.dumps({"release": RELEASE, **zips}, indent=1) + "\n")
print(f"wrote {os.path.relpath(out, R)}")
