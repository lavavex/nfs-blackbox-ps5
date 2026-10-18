#!/usr/bin/env python3
"""Make a clean Most Wanted 1.2 folder from the owner's own discs (or the EU ISO).

    extract-mw.py SOURCE OUT

SOURCE is a folder holding the disc files: the NA discs copied into one folder
(src/mw05-na/all: 0compressed.zip … 3compressed.zip, speed.exe) or a mounted EU DVD
(0compressed.zip, 1compressed.zip, speed.exe). Its *compressed.zip archives are
unpacked in order, backslash paths turned into folders, and the disc's speed.exe added:
the same tree Prospero BlackBox's base-games/NFSMW expects (copy it to a USB drive as
NFSMW/, or over FTP). The installer's own extras (Support/, server.dll, …) are not
included, as in the copy the console imported.

At the end, GLOBAL/GAMEPLAY.BIN is checked: 1.2 is 2,103,680 bytes, CRC 91a3ddf1.
Known results: NA 1407 files / 3,028,017,159 bytes; EU 1406 / 2,971,568,507.

To get 1.3 from it without running EA's code: prospero-blackbox's
`tools/rtpatch.py Patch.v1.3.exe OUT OUT-1.3-files` (then copy those over a copy of OUT).
"""
import glob
import os
import sys
import zipfile
import zlib


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.split("\n")[2].strip())
    source, out = sys.argv[1], sys.argv[2]
    archives = sorted(glob.glob(os.path.join(source, "*compressed.zip")))
    if not archives or not os.path.isfile(os.path.join(source, "speed.exe")):
        sys.exit("%s has no *compressed.zip and speed.exe: not a Most Wanted disc folder" % source)
    if os.path.exists(out) and os.listdir(out):
        sys.exit("%s isn't empty" % out)
    for archive in archives:
        print("unpacking", os.path.basename(archive))
        with zipfile.ZipFile(archive) as zf:
            for info in zf.infolist():
                if info.is_dir():
                    continue
                target = os.path.join(out, info.filename.replace("\\", "/"))
                os.makedirs(os.path.dirname(target), exist_ok=True)
                with zf.open(info) as src, open(target, "wb") as dst:
                    while chunk := src.read(1 << 22):
                        dst.write(chunk)
    with open(os.path.join(source, "speed.exe"), "rb") as src, open(os.path.join(out, "speed.exe"), "wb") as dst:
        dst.write(src.read())
    files = [os.path.join(d, f) for d, _, names in os.walk(out) for f in names]
    print("%d files, %d bytes" % (len(files), sum(map(os.path.getsize, files))))
    folder = os.path.join(out, "GLOBAL")
    names = [n for n in os.listdir(folder) if n.lower() == "gameplay.bin"]
    data = open(os.path.join(folder, names[0]), "rb").read() if names else b""
    crc = zlib.crc32(data)
    print("GLOBAL/GAMEPLAY.BIN: %d bytes, CRC %08x" % (len(data), crc))
    if (len(data), crc) != (2103680, 0x91A3DDF1):
        sys.exit("that isn't Most Wanted 1.2")
    print("Most Wanted 1.2")


if __name__ == "__main__":
    main()
