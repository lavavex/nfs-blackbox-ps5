# The Twitter hype video (2026-10-18)

`build.py` cuts `raw.mp4` (the owner's 8:36 PS5 capture of MW's prologue, run 1 with the Xbox
360 look, run 2 without; on the console under
`/user/av_contents/video/NPXS40087/PPSA99403/275/20261018_151528_00387719.mp4`, not in the repo)
into `hypeA.mp4`: ten shots in the recording's order, 0.4 s dissolves, style A text from
`cards.swift` (`swift cards.swift DIR` writes the PNGs into DIR). The pursuit cutscene freezes at
0:49.6 for an A/B slider against run 2, which plays the same scripted scene 272.0 s later
(matched by frame correlation). Run both in one folder with raw.mp4 beside them; ffmpeg 9 and
macOS Swift, nothing else. The owner's v4 is ~/Desktop/nfsmw-ps5-hype-v4.mp4.
