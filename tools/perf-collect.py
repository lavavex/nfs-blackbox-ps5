#!/usr/bin/env python3
"""Collect a whole prospero-win session's frame profile from the console.

The console keeps only the last ~1 MB of a session's log (session-N.log plus
session-N.previous.log), a few seconds at full profiling. This polls both over
FTP every --interval seconds, keeps each REC line once (by its seq number), and
writes the stitched log to OUT.log as it goes. Stop it with Ctrl-C or --minutes.

  perf-collect.py OUT [--host ADDRESS, or PS5_HOST] [--interval 1] [--minutes 30]
  perf-collect.py --report OUT.log      # summary per 5 s of the session
"""
import argparse, collections, ftplib, io, os, re, sys, time

ap = argparse.ArgumentParser()
ap.add_argument("out")
ap.add_argument("--host", default=os.environ.get("PS5_HOST"), required=not os.environ.get("PS5_HOST")); ap.add_argument("--port", type=int, default=2121)
ap.add_argument("--interval", type=float, default=1.0); ap.add_argument("--minutes", type=float, default=30)
ap.add_argument("--report", action="store_true")
args = ap.parse_args()
LOGS = "/data/prospero-win/logs"
SEQ = re.compile(rb"^REC seq=(\d+) ")


def report(path):
    hz = None
    buckets = collections.OrderedDict()
    def b(t): return buckets.setdefault(int(t // 5) * 5, collections.defaultdict(list))
    for line in open(path, errors="ignore"):
        m = re.search(r" t=([\d.]+) ", line)
        if not m: continue
        t = float(m.group(1))
        if "PW_VK_RADV_PRESENT" in line:
            iv = re.search(r"interval=(\d+)", line)
            if iv and hz: b(t)["ms"].append(int(iv.group(1)) / hz * 1000)
        elif "PW_NATIVE_PROFILE version=3" in line:
            f = dict(kv.split("=", 1) for kv in line.split() if "=" in kv)
            hz = hz or int(f.get("tsc_hz", 0)) or None
            if hz:
                busy = (int(f["wall"]) - int(f["syscall"])) / hz * 1000
                b(t)["busy_" + f["tid"]].append(busy)
        elif "PW_VK_RADV_PROFILE" in line and hz:
            th = re.search(r"thread=(\d+)", line).group(1)
            d = re.search(r"vkCmdDrawIndexed=(\d+)/", line)
            w = re.search(r"vkWaitSemaphores=(\d+)/(\d+)", line)
            if d: b(t)["draws_" + th].append(int(d.group(1)))
            if w: b(t)["waitms_" + th].append(int(w.group(2)) / hz * 1000)
        elif "PW_WINE64 memory" in line:
            dm = re.search(r" dmem=(\d+)K", line)
            if dm: b(t)["dmemMB"].append(int(dm.group(1)) / 1024)
    keys = sorted({k for v in buckets.values() for k in v if k != "ms"})
    print("t(s)   frames  fps   p90ms  " + "  ".join(keys))
    for t, v in buckets.items():
        ms = sorted(v["ms"])
        if not ms: continue
        avg = sum(ms) / len(ms)
        row = f"{t:5d}  {len(ms):5d}  {1000/avg:5.0f}  {ms[int(len(ms)*.9)]:5.1f}  "
        row += "  ".join(f"{(sum(v[k])/len(v[k]) if v[k] else 0):{len(k)}.1f}" for k in keys)
        print(row)


if args.report:
    report(args.out); sys.exit(0)

last, out = -1, open(args.out + ".log", "ab")
end = time.time() + args.minutes * 60
current = None
while time.time() < end:
    try:
        f = ftplib.FTP(); f.connect(args.host, args.port, timeout=10); f.login(); f.voidcmd("TYPE I")
        lines = []; f.retrlines("LIST " + LOGS, lines.append)
        logs = [(l.split()[5:8], l.split()[-1]) for l in lines if re.search(r"session-\d+\.log$", l)]
        # newest session by mtime (ls dates, same day): the one written last
        newest = max(logs, key=lambda x: (x[0][1], x[0][2]))[1] if logs else None
        if newest != current:
            print(f"session {newest}", flush=True); current = newest; last = -1
        got = 0
        for name in (newest.replace(".log", ".previous.log"), newest):
            buf = io.BytesIO()
            try: f.retrbinary(f"RETR {LOGS}/{name}", buf.write)
            except ftplib.error_perm: continue
            top = last
            for line in buf.getvalue().splitlines(keepends=True):
                m = SEQ.match(line)
                if not m or int(m.group(1)) <= last: continue
                out.write(line); got += 1; top = max(top, int(m.group(1)))
            last = top
        out.flush(); f.quit()
        print(f"{time.strftime('%H:%M:%S')} +{got} lines", flush=True)
    except Exception as e:
        print("poll failed:", e, flush=True)
    time.sleep(args.interval)
