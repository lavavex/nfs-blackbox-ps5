#!/usr/bin/env python3
"""Compare gameplay frame times between prospero-win sessions from the live log.

  tools/perf-compare.py NAME=SESSION.log [NAME=SESSION.log ...]

A SESSION.log is one session cut out of logs/live.log (the socat listener's
file), from its "PW_WINE64 autostart" line on, e.g.:
  N=$(grep -a -n "PW_WINE64 autostart" logs/live.log | tail -1 | cut -d: -f1)
  tail -n +$N logs/live.log > /tmp/run.log

Needs the profile's [debug] env PW_NATIVE_PROFILE=1 (prospero-win then logs
PW_VK_RADV_PRESENT / PW_VK_RADV_PROFILE per frame). Frames with 300 draws or
fewer (menus, loading) are left out. Draw counts differ between routes, so
compare frame times at equal draw counts (the binned columns), not medians.
"""
import collections, re, sys

def frames(path):
    ms, draws, submits = {}, collections.Counter(), collections.Counter()
    hz = None
    for line in open(path, errors="ignore"):
        if hz is None:
            h = re.search(r"tsc_hz=(\d+)", line)
            if h: hz = int(h.group(1))
        fm = re.search(r" frame=(\d+)", line)
        if not fm: continue
        f = int(fm.group(1))
        if "PW_VK_RADV_PRESENT" in line:
            iv = re.search(r"interval=(\d+)", line)
            if iv: ms[f] = int(iv.group(1))
        elif "PW_VK_RADV_PROFILE" in line:
            d = re.search(r"vkCmdDrawIndexed=(\d+)/", line)
            if d: draws[f] += int(d.group(1))
            s = re.search(r"vkQueueSubmit2=(\d+)/", line)
            if s: submits[f] += int(s.group(1))
    hz = hz or 1596300000
    return [(draws[f], ms[f] / hz * 1000, submits[f]) for f in ms if draws[f] > 300]

for arg in sys.argv[1:]:
    name, path = arg.split("=", 1)
    pts = frames(path)
    if not pts:
        print(name, "no gameplay frames"); continue
    t = sorted(m for _, m, _ in pts); d = sorted(x for x, _, _ in pts)
    bins = collections.defaultdict(list)
    for x, m, _ in pts: bins[x // 400 * 400].append(m)
    print(f"{name:14s} frames {len(pts):6d} median {t[len(t)//2]:.2f}ms ({1000/t[len(t)//2]:.0f} fps) "
          f"p90 {t[int(len(t)*.9)]:.1f}ms p99 {t[int(len(t)*.99)]:.1f}ms draws p50 {d[len(d)//2]} "
          f"submits/f {sum(s for *_, s in pts)/len(pts):.1f} | "
          + " ".join(f"{k}:{sum(v)/len(v):.1f}" for k, v in sorted(bins.items()) if len(v) > 50))
