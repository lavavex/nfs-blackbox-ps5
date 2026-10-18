#!/usr/bin/env python3
"""Download and verify everything nfsmw-x360.py installs, without touching
the console: the 360 pack (with its LowPerf shaders), HD Content Support, HD
Fonts and the cutscenes for one region, into the same cache (~/nfsmw-mods).
nfsmw-x360.py --yes then finds them there and skips the downloads.
    nfsmw-x360-prefetch.py [--region ntsc|pal]"""
import importlib.util, json, os, sys
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location("x360", os.path.join(here, "nfsmw-x360.py"))
x360 = importlib.util.module_from_spec(spec); spec.loader.exec_module(x360)
region = sys.argv[sys.argv.index("--region") + 1] if "--region" in sys.argv else "ntsc"
m = json.load(open(os.path.join(here, "nfsmw-x360.json")))
items = [(f, os.path.join(x360.CACHE, "x360-stuff-4.2", f["path"])) for f in m["x360"]["files"] + m["x360"]["lowperf"]]
items += [(m["hdcs"], os.path.join(x360.CACHE, "NFS HD Content Support.zip")),
          (m["fonts"], os.path.join(x360.CACHE, "NFSMWHDFonts-1-2.7z")),
          (m["cutscenes"][region], os.path.join(x360.CACHE, f"X360 Cutscenes 1.1 {region.upper()}.rar"))]
total = sum(i["size"] for i, _ in items); done = 0
import time
def fetch_with_retries(item, dest, tries=6):
    # Google Drive refuses requests for a while after many in a row ("Cannot
    # retrieve the public link ... many accesses"): wait and ask again.
    for attempt in range(1, tries + 1):
        try:
            return x360.fetch(item, dest)
        except Exception as error:
            if attempt == tries: raise
            wait = (60, 120, 300, 300, 300)[attempt - 1]
            print(f"  {os.path.basename(dest)}: {type(error).__name__}; retry {attempt}/{tries - 1} in {wait} s", flush=True)
            time.sleep(wait)
for n, (item, dest) in enumerate(items, 1):
    fetch_with_retries(item, dest)
    done += item["size"]
    print(f"[{n}/{len(items)}] {done / total:6.1%} verified {os.path.relpath(dest, x360.CACHE)}", flush=True)
print(f"all {len(items)} files verified in {x360.CACHE} ({total / 1e9:.2f} GB)")
