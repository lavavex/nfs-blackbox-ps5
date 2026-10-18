#!/usr/bin/env python3
"""Make NFS Most Wanted on the console look like the Xbox 360 version, over FTP.

Installs, in this order (a later file wins):
  1. Xbox 360 Stuff Pack 4.2, easy installation (elaymm4 and others):
     the 360's shaders, textures, lighting, visual treatment, rain drops
     and sound. --lowperf adds its cheaper shadow-filtering shaders.
  2. NFS HD Content Support (Aero_): lets the game show HD fonts and videos.
  3. NFSMW HD Fonts 1.2 (BadassBaboon & Aero_), only with --hd-fonts (the
     owner prefers the original fonts): the body font (GLOBALA.BUN) and
     language textures. Its FrontB.lzc, which holds the title font, is
     left out: the 360 pack has its own FrontB.lzc.
  4. Xbox 360 cutscenes 1.1 (Zyphir & Aven): the 360's 720p movies, in the
     region (NTSC or PAL) of the movies already on the console. English only.
and sets:
  Widescreen Fix: FixHUD 0, Scaling 0, FMVWidescreenMode 0, ImproveShadowLOD 0,
                  AutoScaleShadowsRes 0, ConsoleGamma 2, FEScale 1.00,
                  DisableMotionBlur 1 (the 360 pack does the HUD, FOV, menu
                  and movie scaling itself)
  X360Stuff.ini:  MotionBlur 0, WidescreenMovies 1 (the 360 movies are 16:9)
It removes Xbox Rain Droplets and the old HD Font Support plugin when they
are installed: the 360 pack draws its own rain drops, and HD Content Support
replaces the plugin.

Downloads (about 4 GB) are checked against nfsmw-x360.json and kept in
~/nfsmw-mods. Google Drive files come from its direct download endpoint (gdown,
if installed, is the fallback); archives are unpacked with bsdtar. Without --yes the
plan is printed and nothing is downloaded or sent. Every console file
replaced or removed is kept under ~/nfsmw-settings/<time>-x360/ (the game's
own movies included, about 1.5 GB); --undo DIR restores them and removes
what was new. Close the game first.
    nfsmw-x360.py [--lowperf] [--hd-fonts] [--region ntsc|pal] [--host IP] [--yes]
    nfsmw-x360.py --undo DIR [--host IP]
LOCAL=DIR stands a folder in for the console.
"""
import argparse, ftplib, hashlib, json, os, re, shutil, subprocess, sys, tempfile, time, urllib.request

GAME = "/data/prospero-win/prefixes/nfs-blackbox/drive_c/Games/NFSMW"
HERE = os.path.dirname(os.path.abspath(__file__))
CACHE = os.path.expanduser(os.environ.get("NFSMW_CACHE", "~/nfsmw-mods"))
WSF = "scripts/NFSMostWanted.WidescreenFix.ini"
X360_INI = "scripts/X360Stuff.ini"
SETTINGS = {
    # As the pack's ReadMe (Easy installation, step 2) lists them, plus
    # AutoScaleShadowsRes 0 (its Xbox 360 accurate settings) and motion blur off.
    # Except WriteSettingsToFile 0: under Wine, 1 makes the game render at 640x480,
    # which blows out the 360 lighting (found on the PC test, 2026-10-10).
    WSF: {"FixHUD": "0", "Scaling": "0", "FMVWidescreenMode": "0", "WriteSettingsToFile": "0",
          "ImproveShadowLOD": "0", "AutoScaleShadowsRes": "0", "ConsoleGamma": "2",
          "DisableMotionBlur": "1", "SMAA": "1"},
    X360_INI: {"MotionBlur": "0", "WidescreenMovies": "1"},
}
# Pack files left out: the 360 glass shader draws interior glass (the bus
# station's windows) as a blown-out white smear under DXVK, on PC and PS5 alike;
# without it the game's own glass shader is used (found 2026-10-11).
SKIP = {"FX/COMPILED/glassreflectshader.fx.o"}
REMOVE = ["scripts/NFSMostWanted.XboxRainDroplets.asi", "scripts/NFSMostWanted.XboxRainDroplets.ini",
          "scripts/NFSMWHDFontSupport.asi"]


class Console:
    def __init__(self, host):
        self.local = os.environ.get("LOCAL")
        self.listed = {}
        if not self.local:
            self.ftp = ftplib.FTP()
            self.ftp.connect(host, 2121, timeout=120)
            self.ftp.login()

    def listdir(self, path):
        """Names in a console folder; empty when it does not exist."""
        if path not in self.listed:
            if self.local:
                p = self.local + path
                self.listed[path] = os.listdir(p) if os.path.isdir(p) else []
            else:
                lines = []
                try:
                    self.ftp.retrlines(f"LIST {path}", lines.append)
                except ftplib.error_perm:
                    pass
                names = [l.split(maxsplit=8)[8].split(" -> ")[0] for l in lines if len(l.split(maxsplit=8)) == 9]
                self.listed[path] = [n for n in names if n not in (".", "..")]
        return self.listed[path]

    def resolve(self, rel):
        """rel spelled as the console already spells each part (GLOBAL, not
        Global): the console's file system is case-sensitive, Wine is not."""
        done = []
        for part in rel.split("/"):
            match = [n for n in self.listdir("/".join([GAME] + done)) if n.lower() == part.lower()]
            done.append(match[0] if match else part)
        return "/".join(done)

    def exists(self, rel):
        parent, _, name = rel.rpartition("/")
        return name in self.listdir("/".join([GAME, parent]) if parent else GAME)

    def get(self, rel, local):
        os.makedirs(os.path.dirname(local), exist_ok=True)
        if self.local:
            shutil.copyfile(self.local + f"{GAME}/{rel}", local)
            return
        with open(local, "wb") as f:
            self.ftp.retrbinary(f"RETR {GAME}/{rel}", f.write, 1 << 20)

    def put(self, rel, local):
        path = f"{GAME}/{rel}"
        if self.local:
            os.makedirs(os.path.dirname(self.local + path), exist_ok=True)
            shutil.copyfile(local, self.local + path)
            return
        parts = path.strip("/").split("/")[:-1]
        for i in range(1, len(parts) + 1):
            try:
                self.ftp.mkd("/" + "/".join(parts[:i]))
            except ftplib.error_perm:
                pass  # exists
        with open(local, "rb") as f:
            self.ftp.storbinary(f"STOR {path}", f, 1 << 20)
        if self.ftp.size(path) != os.path.getsize(local):
            sys.exit(f"{rel}: the console's copy is not the size sent")

    def delete(self, rel):
        if self.local:
            os.remove(self.local + f"{GAME}/{rel}")
        else:
            self.ftp.voidcmd(f"DELE {GAME}/{rel}")  # ftpsrv answers 226, which ftp.delete() rejects


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def fetch(item, dest):
    """Download one manifest item to dest, unless a good copy is there."""
    if os.path.isfile(dest) and os.path.getsize(dest) == item["size"] and sha256(dest) == item["sha256"]:
        return dest
    os.makedirs(os.path.dirname(dest), exist_ok=True)
    print(f"  downloading {os.path.basename(dest)} ({item['size'] / 1e6:.1f} MB)", flush=True)
    if "drive" in item:
        # Drive's direct download endpoint (confirm=t skips the large-file
        # warning page). gdown's scraping route gets "Cannot retrieve the
        # public link" after a burst of requests; it stays as a fallback.
        url = f"https://drive.usercontent.google.com/download?id={item['drive']}&export=download&confirm=t"
        try:
            with urllib.request.urlopen(urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"}), timeout=120) as r, \
                    open(dest, "wb") as f:
                shutil.copyfileobj(r, f, 1 << 20)
        except OSError:
            try:
                import gdown
            except ImportError:
                sys.exit("the Drive download failed, and the gdown fallback needs: python3 -m pip install --user gdown")
            gdown.download(id=item["drive"], output=dest, quiet=item["size"] < 50e6)
    else:
        with urllib.request.urlopen(urllib.request.Request(item["url"], headers={"User-Agent": "Mozilla/5.0"}), timeout=120) as r, \
                open(dest, "wb") as f:
            shutil.copyfileobj(r, f, 1 << 20)
    if not os.path.isfile(dest) or sha256(dest) != item["sha256"]:
        sys.exit(f"{dest}: not the file pinned in nfsmw-x360.json (changed upstream?); nothing sent")
    return dest


def unpack(archive, members, into):
    tar = shutil.which("bsdtar") or (shutil.which("tar") if sys.platform == "darwin" else None)
    if not tar:
        sys.exit("unpacking .rar and .7z needs bsdtar (macOS's own tar is bsdtar)")
    os.makedirs(into, exist_ok=True)
    subprocess.run([tar, "-xf", archive, "-C", into, *members], check=True)


def set_ini(path, values):
    text = open(path, encoding="latin-1", newline="").read()
    for key, value in values.items():
        text, n = re.subn(rf"^({key}\s*=\s*)[^\s;#]+", rf"\g<1>{value}", text, flags=re.M)
        if not n:
            sys.exit(f"{key} not in {os.path.basename(path)}; nothing sent")
    open(path, "w", encoding="latin-1", newline="").write(text)


def undo(console, root):
    for rel in filter(None, open(os.path.join(root, "new.txt")).read().split("\n")):
        if console.exists(rel):
            console.delete(rel)
            print("removed", rel)
    files = os.path.join(root, "files")
    for dirpath, _, names in os.walk(files):
        for name in names:
            local = os.path.join(dirpath, name)
            rel = os.path.relpath(local, files).replace(os.sep, "/")
            console.put(rel, local)
            print("restored", rel)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--lowperf", action="store_true", help="the 360 pack's cheaper shadow shaders")
    ap.add_argument("--hd-fonts", action="store_true", help="also install NFSMW HD Fonts (off: the original fonts)")
    ap.add_argument("--no-cutscenes", action="store_true",
                    help="leave the movies (and HD Content Support) out, e.g. for Xbox 360 Converted Movies")
    ap.add_argument("--region", choices=("ntsc", "pal"), help="default: the region of the console's movies")
    ap.add_argument("--host", default=os.environ.get("PS5_HOST"), required=not os.environ.get("PS5_HOST"))
    ap.add_argument("--yes", action="store_true", help="download and install")
    ap.add_argument("--undo", metavar="DIR")
    args = ap.parse_args()
    console = Console(args.host)
    if args.undo:
        return undo(console, os.path.expanduser(args.undo))

    if args.no_cutscenes:
        SETTINGS[X360_INI]["WidescreenMovies"] = "0"  # the PC movies stay: 16:9 letterboxed in 4:3
    manifest = json.load(open(os.path.join(HERE, "nfsmw-x360.json")))
    if not console.exists(console.resolve(WSF)):
        sys.exit(f"no {WSF} under {GAME} on the console; is the game installed there?")

    movies = [n.lower() for n in console.listdir(f"{GAME}/{console.resolve('MOVIES')}")]
    region = args.region or ("pal" if any(n.endswith("_pal.vp6") for n in movies) else
                             "ntsc" if any(n.endswith("_ntsc.vp6") for n in movies) else None)
    if not region:
        sys.exit("no _ntsc or _pal movies in the console's MOVIES folder; pass --region")
    if movies and not any("_english_" in n for n in movies):
        print("note: the console's movies are not English; the 360 cutscenes are, and will play instead")
    cut = manifest["cutscenes"][region]

    # (rel in the game folder, how to get the local file) in install order.
    plan = []
    for item in manifest["x360"]["files"]:
        if item["path"][len("Files/"):] not in SKIP:
            plan.append((item["path"][len("Files/"):], ("x360", item)))
    if args.lowperf:
        for item in manifest["x360"]["lowperf"]:
            rel = "FX/COMPILED/" + item["path"].rsplit("/", 1)[1]
            if rel not in SKIP:
                plan.append((rel, ("x360", item)))
    if not args.no_cutscenes:
        for member, rel in manifest["hdcs"]["members"].items():
            plan.append((rel, ("hdcs", member)))
    if args.hd_fonts:
        for member, rel in manifest["fonts"]["members"].items():
            plan.append((rel, ("fonts", member)))
    for name in ([] if args.no_cutscenes else cut["members"]):
        plan.append(("MOVIES/" + name, ("cut", name)))
    plan.append((WSF, ("ini", WSF)))
    files = {}
    for rel, how in plan:
        files[console.resolve(rel)] = how  # a later source wins
    removals = [console.resolve(r) for r in REMOVE if console.exists(console.resolve(r))]

    by_top = {}
    for rel in files:
        top = rel.split("/")[0]
        new, old = by_top.get(top, (0, 0))
        by_top[top] = (new + (not console.exists(rel)), old + console.exists(rel))
    print(f"Xbox 360 look for NFS Most Wanted ({region.upper()} cutscenes{', LowPerf shaders' if args.lowperf else ''}):")
    for top, (new, old) in sorted(by_top.items()):
        print(f"  {top + '/':<10} {new:>4} new, {old:>4} replaced")
    for rel in removals:
        print(f"  remove     {rel}")
    for rel, values in SETTINGS.items():
        print(f"  {rel}: " + ", ".join(f"{k} = {v}" for k, v in values.items()))
    if not args.yes:
        print(f"nothing downloaded or sent; run again with --yes (about {manifest['download_gb']} GB to download, kept in {CACHE})")
        return

    # Fetch and unpack everything before the console is touched.
    work = tempfile.mkdtemp(prefix="nfsmw-x360-")
    cache = {}
    def local_of(rel, how):
        kind, what = how
        if kind == "x360":
            return fetch(what, os.path.join(CACHE, "x360-stuff-4.2", what["path"]))
        if kind == "ini":
            dest = os.path.join(work, "ini", rel)
            console.get(rel, dest)
            return dest
        if kind not in cache:
            item = manifest[kind] if kind != "cut" else cut
            name = {"hdcs": "NFS HD Content Support.zip", "fonts": "NFSMWHDFonts-1-2.7z",
                    "cut": f"X360 Cutscenes 1.1 {region.upper()}.rar"}[kind]
            archive = fetch(item, os.path.join(CACHE, name))
            # The movies are most of the archive: unpack it whole.
            unpack(archive, [] if kind == "cut" else list(item["members"]), os.path.join(work, kind))
            cache[kind] = os.path.join(work, kind)
            missing = [m for m in item["members"] if not os.path.isfile(os.path.join(cache[kind], m))]
            if missing:
                sys.exit(f"{name} lacks {', '.join(missing)}; nothing sent")
        return os.path.join(cache[kind], what)
    files_local = {}
    for rel, how in files.items():
        files_local[rel] = local_of(rel, how)
    x360_ini = console.resolve(X360_INI)
    edited_x360 = os.path.join(work, "ini", "X360Stuff.ini")
    os.makedirs(os.path.dirname(edited_x360), exist_ok=True)
    shutil.copyfile(files_local[x360_ini], edited_x360)
    files_local[x360_ini] = edited_x360
    set_ini(files_local[console.resolve(WSF)], SETTINGS[WSF])
    set_ini(edited_x360, SETTINGS[X360_INI])

    backup = os.path.expanduser(time.strftime("~/nfsmw-settings/%Y%m%d-%H%M%S-x360"))
    os.makedirs(backup)
    new = [rel for rel in files if not console.exists(rel)]
    open(os.path.join(backup, "new.txt"), "w").write("\n".join(new) + "\n")
    print(f"backing up the console's copies to {backup}")
    for rel in [r for r in files if r not in new] + removals:
        console.get(rel, os.path.join(backup, "files", rel))
    for rel in removals:
        console.delete(rel)
        print("removed", rel)
    total = sum(os.path.getsize(p) for p in files_local.values())
    sent, start = 0, time.time()
    for rel in sorted(files_local):
        console.put(rel, files_local[rel])
        sent += os.path.getsize(files_local[rel])
        print(f"  {sent / total:6.1%}  {rel}", flush=True)
    shutil.rmtree(work, ignore_errors=True)
    print(f"installed {len(files_local)} files ({len(new)} new, {sent / 1e9:.2f} GB in {time.time() - start:.0f} s)")
    print(f"undo: {sys.argv[0]} --undo {backup}")


if __name__ == "__main__":
    main()
