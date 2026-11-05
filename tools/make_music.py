#!/usr/bin/env python3
"""Writes the Floe Slide music as ZMT (Zeal Music Tracker) files in assets/.

All tracks are original. Each pattern is 32 sixteenth steps on up to 3 voices; voice 4 is left
to the sound effects. A track holds at most 3 patterns (3 pattern buffers in the dead tileset blob).
Step tokens: '.' hold, '-' note off, 'C5' / 'F#4' notes, 'k s h' drum hits on the noise voice.
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
from zealart import zmt, voice  # noqa: E402

OUT = os.path.join(ROOT, 'assets')

# "Drift Ice": the puzzle loop. Calm, so it can run for minutes. A minor / C, music-box arpeggios.
HATS = "h . . . h . . . h . . . h . . . h . . . h . . . h . . . h . h ."
FLOE = [
    [   # A: Am | F
        voice("triangle", 75, "A4 C5 E5 C5 A4 C5 E5 C5 A4 C5 E5 C5 A4 C5 E5 C5 F4 A4 C5 A4 F4 A4 C5 A4 F4 A4 C5 A4 F4 A4 C5 A4"),
        voice("square", 25, "E6 . . . . . . . D6 . C6 . B5 . . . A5 . . . . . . . C6 . . . . . - ."),
        voice("noise", 0, HATS),
        None,
    ],
    [   # B: C | G
        voice("triangle", 75, "C5 E5 G5 E5 C5 E5 G5 E5 C5 E5 G5 E5 C5 E5 G5 E5 B4 D5 G5 D5 B4 D5 G5 D5 B4 D5 G5 D5 B4 D5 G5 D5"),
        voice("square", 25, "G5 . . . E5 . . . G5 . A5 . G5 . . . D6 . . . B5 . . . G5 . . . . . - ."),
        voice("noise", 0, HATS),
        None,
    ],
    [   # C: Dm | E, back to the top
        voice("triangle", 75, "D5 F5 A5 F5 D5 F5 A5 F5 D5 F5 A5 F5 D5 F5 A5 F5 E5 G#5 B5 G#5 E5 G#5 B5 G#5 E5 G#5 B5 G#5 E5 G#5 B5 G#5"),
        voice("square", 25, "F6 . . . E6 . D6 . A5 . . . . . . . B5 . . . G#5 . . . E5 . . . . . - ."),
        voice("noise", 0, HATS),
        None,
    ],
]

# Title: slow and wide, C major lullaby over a pulsing bass
TITLE = [
    [
        voice("square", 25, "E5 . . . G5 . . . C6 . . . . . B5 . A5 . . . G5 . . . E5 . . . . . - ."),
        voice("triangle", 100, "C3 . . . C3 . . . G2 . . . G2 . . . A2 . . . A2 . . . F2 . . . G2 . . ."),
        voice("noise", 0, "h . . . . . . . h . . . . . . . h . . . . . . . h . . . . . . ."),
        None,
    ],
    [
        voice("square", 25, "F5 . . . E5 . . . D5 . . . . . C5 . D5 . . . E5 . . . G5 . . . . . - ."),
        voice("triangle", 100, "F2 . . . F2 . . . C3 . . . C3 . . . D3 . . . D3 . . . G2 . . . G2 . . ."),
        voice("noise", 0, "h . . . . . . . h . . . . . . . h . . . . . . . h . . h . . . ."),
        None,
    ],
]

END = [[   # 6 frames a step
    voice("square", 50, "C6 . G5 . E5 . G5 . C6 . E6 . G6 . . . F6 . E6 . D6 . . . C6 . . . . . - ."),
    voice("triangle", 100, "C3 . . . E3 . . . G3 . . . C4 . . . F3 . . . G3 . . . C3 . . . - . . ."),
    voice("noise", 25, "k . . . s . . . k . k . s . . . k . . . s . . . k . s . s s s s"),
    None,
]]

TRACKS = {
    "floe.zmt": zmt("DRIFT ICE", 28, FLOE, [0, 1, 0, 2]),
    "title.zmt": zmt("FLOE SLIDE", 32, TITLE, [0, 1]),
    "end.zmt": zmt("ALL CLEAR", 24, END, [0]),
}

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for name, data in TRACKS.items():
        with open(os.path.join(OUT, name), "wb") as f:
            f.write(data)
        print(f"{name}: {len(data)} bytes")
