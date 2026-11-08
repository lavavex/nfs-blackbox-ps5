#!/usr/bin/env python3
"""The second cut: fewer, longer shots in the recording's order, 0.4 s dissolves between them
(sound crossfaded too), style A cards fading in and out. raw.mp4 -> hypeA.mp4."""
import os, subprocess, sys

S = os.path.dirname(os.path.abspath(__file__))
os.chdir(S)
X = 0.4  # dissolve
ENC = ["-c:v", "libx264", "-preset", "slow", "-crf", "17", "-pix_fmt", "yuv420p", "-r", "60",
       "-c:a", "aac", "-b:a", "192k", "-ar", "48000", "-ac", "2"]

def run(args):
    subprocess.run(["ffmpeg", "-v", "error", "-y"] + args, check=True)

def card_filter(dur):
    # The card fades in 0.6 s into the shot and out 0.5 s before it ends.
    return (f"format=rgba,fade=t=in:st=0.6:d=0.5:alpha=1,"
            f"fade=t=out:st={dur - 1.0:.2f}:d=0.5:alpha=1")

# (name, start in raw.mp4, length, card or None, speed)
shots = [
    ("flyover", 423.0, 6.0, "a_title.png", 1),
    ("garage", 30.0, 4.0, None, 1),
    ("slider", 44.0, 12.0, "a_slider.png", 1),    # the pursuit: plays, freezes at 49.6 for the A/B, plays on (run 2 = +272.0 s)
    ("crash", 74.0, 6.5, None, 1),
    ("start", 119.5, 7.5, None, 1),
    ("race", 140.0, 5.0, None, 1),
    ("options", 220.3, 6.0, None, 1),              # 3:40 to 3:46: the 360 look off, then the prompt (the menu names itself)
    ("install", 231.0, 4.25, "a_install.png", 4),  # 17 s of the ring at four times the speed
    ("end", 186.0, 5.0, "a_end.png", 1),
]

parts = []
for i, (name, start, dur, card, speed) in enumerate(shots):
    out = f"a{i:02d}_{name}.mp4"
    if name == "slider":
        # The pursuit plays to FREEZE, holds there while a divider sweeps the 360 look (left)
        # across to the original (right, the same frame of run 2, 272.0 s later) and back, then
        # plays on. The sound dips out for the freeze and comes back with the picture.
        FREEZE, HOLD = 49.6, 4.0
        before, after = FREEZE - start, dur - (FREEZE - start) - HOLD
        run(["-ss", str(FREEZE), "-i", "raw.mp4", "-frames:v", "1", "fz_a.png"])
        run(["-ss", str(FREEZE + 272.0), "-i", "raw.mp4", "-frames:v", "1", "fz_b.png"])
        # 1 at the start (all 360), 0 halfway (all original), 1 again at the end; still at both ends.
        x = f"(0.5+0.5*cos(2*PI*min(max((T-0.5)/{HOLD - 1.0},0),1)))"
        run(["-ss", str(start), "-t", str(before), "-i", "raw.mp4",
             "-loop", "1", "-t", str(HOLD), "-r", "60", "-i", "fz_a.png",
             "-loop", "1", "-t", str(HOLD), "-r", "60", "-i", "fz_b.png",
             "-loop", "1", "-t", str(HOLD), "-i", card,
             "-f", "lavfi", "-i", f"color=white:s=4x1080:r=60:d={HOLD}",
             "-f", "lavfi", "-t", str(HOLD), "-i", "anullsrc=r=48000:cl=stereo",
             "-ss", str(FREEZE), "-t", str(after), "-i", "raw.mp4",
             "-filter_complex",
             f"[0:v]setpts=PTS-STARTPTS,format=yuv420p[p1];"
             f"[1:v]format=yuv420p,setpts=PTS-STARTPTS[fa];[2:v]format=yuv420p,setpts=PTS-STARTPTS[fb];"
             f"[fa][fb]blend=all_expr='if(lt(X,W*{x}),A,B)'[m];"
             f"[m][4:v]overlay=x='W*{x.replace('T', 't')}-2':y=0:shortest=1[ml];"
             f"[3:v]format=rgba,fade=t=in:st=0.2:d=0.4:alpha=1,fade=t=out:st={HOLD - 0.5}:d=0.4:alpha=1[c];"
             f"[ml][c]overlay=0:0:format=auto,format=yuv420p[p2];"
             f"[6:v]setpts=PTS-STARTPTS,format=yuv420p[p3];"
             f"[0:a]asetpts=PTS-STARTPTS,afade=t=out:st={before - 0.3}:d=0.3[a1];[5:a]asetpts=PTS-STARTPTS[a2];"
             f"[6:a]asetpts=PTS-STARTPTS,afade=t=in:d=0.3[a3];"
             f"[p1][a1][p2][a2][p3][a3]concat=n=3:v=1:a=1[v][a]",
             "-map", "[v]", "-map", "[a]"] + ENC + [out])
    elif name == "compare":
        # The 360 run at 2:46 and the PC run at 7:57: the same spot of the same race.
        run(["-ss", "166.0", "-t", str(dur), "-i", "raw.mp4", "-ss", "477.0", "-t", str(dur), "-i", "raw.mp4",
             "-loop", "1", "-t", str(dur), "-i", card, "-filter_complex",
             f"[0:v]crop=960:1080:480:0[l];[1:v]crop=960:1080:480:0[r];[l][r]hstack[s];"
             f"[2:v]{card_filter(dur)}[c];[s][c]overlay=0:0:format=auto[v]",
             "-map", "[v]", "-map", "0:a"] + ENC + [out])
    elif speed != 1:
        src = dur * speed
        run(["-ss", str(start), "-t", str(src), "-i", "raw.mp4", "-loop", "1", "-t", str(dur), "-i", card,
             "-f", "lavfi", "-t", str(dur), "-i", "anullsrc=r=48000:cl=stereo", "-filter_complex",
             f"[0:v]setpts=PTS/{speed}[f];[1:v]{card_filter(dur)}[c];[f][c]overlay=0:0:format=auto[v]",
             "-map", "[v]", "-map", "2:a", "-t", str(dur)] + ENC + [out])
    elif card:
        run(["-ss", str(start), "-t", str(dur), "-i", "raw.mp4", "-loop", "1", "-t", str(dur), "-i", card,
             "-filter_complex", f"[1:v]{card_filter(dur)}[c];[0:v][c]overlay=0:0:format=auto[v]",
             "-map", "[v]", "-map", "0:a"] + ENC + [out])
    else:
        run(["-ss", str(start), "-t", str(dur), "-i", "raw.mp4"] + ENC + [out])
    parts.append((out, dur))

# Dissolves: each shot fades into the next over X seconds, the sound crossfaded with it; the
# first fades up from black, the last down to black.
inputs, vf, af = [], [], []
for out, _ in parts:
    inputs += ["-i", out]
v, a, t = "[0:v]", "[0:a]", parts[0][1]
for i in range(1, len(parts)):
    vf.append(f"{v}[{i}:v]xfade=transition=fade:duration={X}:offset={t - X:.3f}[v{i}]")
    af.append(f"{a}[{i}:a]acrossfade=d={X}[a{i}]")
    v, a, t = f"[v{i}]", f"[a{i}]", t + parts[i][1] - X
vf.append(f"{v}fade=t=in:d=0.6,fade=t=out:st={t - 0.9:.3f}:d=0.9[vout]")
af.append(f"{a}afade=t=in:d=0.6,afade=t=out:st={t - 1.2:.3f}:d=1.2,alimiter=limit=0.89[aout]")
run(inputs + ["-filter_complex", ";".join(vf + af), "-map", "[vout]", "-map", "[aout]"] + ENC
    + ["-movflags", "+faststart", "hypeA.mp4"])
print(f"hypeA.mp4: {t:.1f} s")
