#!/usr/bin/env python3
"""The scan an export must pass before it can go public (tools/publish.sh and
tools/publish-launcher.sh): in text files, a LAN address, this Mac's home folder or user
name, an e-mail address of the owner, anything shaped like a CD key; any file of the game's
own kinds (.BUN, .BIN, .LZC, ...). Files over 1 MB are listed for a look by eye.

  tools/publish-scan.py FOLDER     (exit 1 when it refuses)
"""
import os, re, sys
root = sys.argv[1]
user = b"rob" + b"erth"  # in two parts, so this script doesn't match itself
game_kinds = re.compile(r"\.(bun|bin|lzc|viv|mad|bnk|abk|big|fng|tpk|lzh|vp6|mpc)$", re.I)
rules = [
    ("a LAN address", re.compile(rb"\b(?:10\.\d{1,3}\.\d{1,3}\.\d{1,3}|192\.168\.\d{1,3}\.\d{1,3})\b")),
    ("this Mac's home folder or user", re.compile(rb"/Users/" + user + rb"\b|\b" + user + rb"@")),
    ("the owner's e-mail", re.compile(rb"@lavavex\.net\b")),
    ("something shaped like a CD key", re.compile(rb"\b[A-Z0-9]{4}(?:-[A-Z0-9]{4}){3,4}\b")),
]
# Placeholders that are nobody's network: the launcher's .env.example (upstream's template).
allowed = {b"192.168.1.100"}
bad, big = [], []
for folder, dirs, files in os.walk(root):
    dirs.sort()
    for name in sorted(files):
        path = os.path.join(folder, name)
        rel = os.path.relpath(path, root)
        data = open(path, "rb").read()
        if game_kinds.search(name):
            bad.append(f"{rel}: a kind of file the game is made of")
        if b"\0" not in data[:8192]:  # text: binaries are looked at by size, below
            for what, rule in rules:
                for m in rule.finditer(data):
                    if m.group(0) in allowed:
                        continue
                    bad.append(f"{rel}: {what} ({m.group(0)[:40].decode(errors='replace')})")
        if len(data) > 1 << 20:
            big.append(f"{len(data):>12,}  {rel}")
print("files over 1 MB (look at each):")
print("\n".join(big) or "  none")
if bad:
    print("\nthe scan refuses this export:")
    print("\n".join(sorted(set(bad))))
    sys.exit(1)
print("\nscan: clean")
