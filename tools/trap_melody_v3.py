#!/usr/bin/env python3
"""Prototype v3: dark trap loop melodies written like producer melody packs.

One MIDI track, 8-bar loop. A 2-bar cell is played four times; only the tail of bars 4 and 8 changes.
Registers are mixed in ONE line: low held notes (octave 2-3) under short high pings (octave 4-5).
No chord track: the harmony comes from the low notes and the minor 5th / b6 / b2 colour.
Cells are original, written note by note: (start, length, midi) in 16ths, 32 steps = 2 bars."""
import struct, sys, os

PPQ = 96
Q = PPQ // 4

MELODIES = [
    dict(name="KK Loop 01 - F MIN 142BPM", bpm=142,
         cell=[(0, 6, 49), (4, 1, 72), (6, 10, 48), (8, 4, 65), (12, 1, 72), (14, 2, 68),
               (16, 6, 44), (20, 1, 72), (22, 8, 46), (24, 4, 67), (28, 1, 72), (30, 2, 68)],
         tail4=[(16, 6, 44), (20, 1, 72), (22, 8, 46), (24, 2, 70), (26, 2, 68), (28, 1, 72), (30, 2, 67)],
         tail8=[(16, 6, 44), (20, 1, 72), (22, 10, 48), (24, 2, 70), (26, 2, 68), (28, 4, 67)]),
    dict(name="KK Loop 02 - C# MIN 146BPM", bpm=146,
         cell=[(0, 16, 37), (0, 2, 73), (2, 2, 68), (4, 2, 69), (6, 2, 68), (8, 2, 64), (10, 2, 63), (12, 2, 62), (14, 2, 64),
               (16, 16, 33), (16, 2, 73), (18, 2, 68), (20, 2, 69), (22, 2, 71), (24, 4, 73), (28, 2, 76), (30, 2, 75)],
         tail4=[(16, 16, 33), (16, 2, 73), (18, 2, 68), (20, 2, 69), (22, 2, 71), (24, 4, 73), (28, 4, 80)],
         tail8=[(16, 8, 33), (16, 2, 73), (18, 2, 68), (20, 2, 69), (22, 2, 68), (24, 8, 32), (24, 2, 64), (26, 2, 63), (28, 4, 62)]),
    dict(name="KK Loop 03 - A MIN 138BPM", bpm=138,
         cell=[(0, 12, 45), (0, 12, 52), (0, 2, 76), (3, 2, 77), (6, 2, 76), (8, 4, 72), (12, 4, 41), (12, 4, 48), (12, 4, 71),
               (16, 12, 46), (16, 12, 53), (16, 2, 74), (19, 2, 77), (22, 2, 74), (24, 6, 70), (28, 4, 40), (28, 4, 52), (30, 2, 69)],
         tail4=[(16, 12, 46), (16, 12, 53), (16, 2, 74), (19, 2, 77), (22, 2, 74), (24, 4, 76), (28, 4, 40), (28, 4, 52), (28, 4, 72)],
         tail8=[(16, 12, 46), (16, 12, 53), (16, 2, 74), (19, 2, 77), (22, 2, 81), (24, 8, 76), (28, 4, 40), (28, 4, 52)]),
    # set 2 (feedback: no bells for long melodies -> leads, strings, flutes, pads)
    dict(name="KK Loop 04 - G MIN 144BPM", bpm=144,
         cell=[(0, 6, 55), (4, 4, 70), (6, 10, 51), (8, 6, 74), (14, 2, 72),
               (16, 6, 50), (18, 4, 69), (22, 10, 56), (24, 4, 67), (28, 4, 68)],
         tail4=[(16, 6, 50), (18, 4, 69), (22, 10, 56), (24, 2, 70), (26, 2, 69), (28, 4, 67)],
         tail8=[(16, 6, 50), (18, 4, 74), (22, 10, 43), (24, 8, 67)]),
    dict(name="KK Loop 05 - D# MIN 150BPM", bpm=150,
         cell=[(0, 16, 39), (0, 3, 75), (3, 3, 78), (6, 2, 77), (8, 3, 75), (11, 3, 73), (14, 2, 70),
               (16, 8, 35), (16, 3, 75), (19, 3, 78), (22, 2, 80), (24, 8, 34), (24, 4, 82), (28, 2, 80), (30, 2, 78)],
         tail4=[(16, 8, 35), (16, 3, 75), (19, 3, 78), (22, 2, 77), (24, 8, 34), (24, 6, 76), (30, 2, 75)],
         tail8=[(16, 16, 35), (16, 3, 75), (19, 3, 73), (22, 2, 70), (24, 8, 75)]),
]

def loop(m, bars=8):
    ev = []
    for rep in range(bars // 2):
        tail = m["tail8"] if rep % 4 == 3 else m["tail4"] if rep % 2 == 1 else None
        notes = [n for n in m["cell"] if n[0] < 16] + tail if tail else m["cell"]
        for s, l, n in notes:
            ev.append((rep * 32 * Q + s * Q, l * Q - 2, n, 100))
    return ev

def vlq(n):
    out = [n & 0x7F]; n >>= 7
    while n: out.append((n & 0x7F) | 0x80); n >>= 7
    return bytes(reversed(out))

def trk(msgs):
    msgs.sort(key=lambda m: (m[0], (m[1][0] & 0xF0) == 0x90))
    data, last = b"", 0
    for t, m in msgs: data += vlq(t - last) + m; last = t
    data += vlq(0) + b"\xff\x2f\x00"
    return b"MTrk" + struct.pack(">I", len(data)) + data

def write(path, m, bars):
    tempo = trk([(0, b"\xff\x51\x03" + struct.pack(">I", int(60_000_000 / m["bpm"]))[1:])])
    notes = []
    for t, l, n, v in loop(m, bars):
        notes += [(t, bytes([0x90, n, v])), (t + l, bytes([0x80, n, 0]))]
    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 1, 2, PPQ) + tempo + trk(notes))

if __name__ == "__main__":
    out = sys.argv[1]; os.makedirs(out, exist_ok=True)
    pick = sys.argv[2:]
    for m in MELODIES:
        if pick and not any(k in m["name"] for k in pick): continue
        write(os.path.join(out, m["name"] + ".mid"), m, 8)          # the loop (8 bars, like a melody pack)
        write(os.path.join(out, "_render16_" + m["name"] + ".mid"), m, 16)
        print(m["name"])
