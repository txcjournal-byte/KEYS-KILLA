#!/usr/bin/env python3
"""Prototype v2: full trap melody arrangements (16 bars) from hand-written motifs.

Tracks: Lead, Lead Octave (double an octave down), Stabs (chord hits on the trap rhythm), Pad, Bass.
Motifs are original, written note by note in scale degrees; the arrangement repeats and varies them
A A' B A'' like a producer would loop an 8-bar idea."""
import struct, sys, os

PPQ = 480
S16 = PPQ // 4
T8 = PPQ // 3
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
SCALES = {"Phrygian": [0, 1, 3, 5, 7, 8, 10], "Harmonic Minor": [0, 2, 3, 5, 7, 8, 11], "Minor": [0, 2, 3, 5, 7, 8, 10]}

def deg2midi(root, scale, deg, base):
    o, d = divmod(deg, 7)
    return base + root + 12 * o + SCALES[scale][d]

# ---- the three attempts: motif = 2 bars of (start, length, degree) in 16ths (or 8th-triplets when trip=True)
ATTEMPTS = [
    dict(name="01 Dark Phrygian Bells", key=10, scale="Phrygian", bpm=140, trip=False,
         prog=[0, 1, 0, 5],                                   # i  bII  i  bVI  (dark, phrygian b2)
         A=[(0, 2, 7), (2, 2, 4), (4, 2, 5), (6, 2, 4), (8, 3, 8), (11, 3, 7), (14, 2, 4),
            (16, 2, 7), (18, 2, 4), (20, 2, 5), (22, 2, 4), (24, 2, 1), (26, 2, 2), (28, 4, 0)],
         B=[(0, 3, 9), (3, 3, 8), (6, 2, 7), (8, 2, 5), (10, 2, 4), (12, 4, 5),
            (16, 3, 8), (19, 3, 7), (22, 2, 5), (24, 2, 4), (26, 2, 2), (28, 4, 1)],
         stabs=[0, 3, 6, 10, 12]),                            # 16th positions in each bar
    dict(name="02 Triplet Harmonic Minor", key=8, scale="Harmonic Minor", bpm=150, trip=True,
         prog=[0, 5, 3, 4],                                   # i  VI  iv  V (raised 7th = dark tension)
         A=[(0, 1, 7), (1, 1, 4), (2, 1, 2), (3, 1, 7), (4, 1, 4), (5, 1, 2), (6, 2, 8), (8, 1, 7), (9, 1, 4), (10, 2, 5),
            (12, 1, 7), (13, 1, 4), (14, 1, 2), (15, 1, 6), (16, 1, 4), (17, 1, 2), (18, 3, 4), (21, 3, 6)],
         B=[(0, 2, 9), (2, 1, 8), (3, 2, 7), (5, 1, 5), (6, 3, 4), (9, 3, 5),
            (12, 2, 8), (14, 1, 7), (15, 2, 6), (17, 1, 4), (18, 6, 6)],
         stabs=[0, 6, 8, 12]),
    dict(name="03 Orchestral Minor Anthem", key=0, scale="Minor", bpm=130, trip=False,
         prog=[0, 5, 2, 6],                                   # i  VI  III  VII  (epic anthem loop)
         A=[(0, 4, 4), (4, 2, 7), (6, 2, 6), (8, 4, 4), (12, 2, 2), (14, 2, 4),
            (16, 4, 3), (20, 2, 2), (22, 2, 0), (24, 6, 1), (30, 2, 2)],
         B=[(0, 2, 9), (2, 2, 7), (4, 4, 8), (8, 2, 7), (10, 2, 6), (12, 4, 4),
            (16, 2, 7), (18, 2, 6), (20, 4, 4), (24, 8, 6)],
         stabs=[0, 3, 8, 11, 14]),
]

def build(a):
    root, scale = a["key"], a["scale"]
    unit = T8 if a["trip"] else S16
    bar = 4 * PPQ
    lead, dbl, stabs, pad, bass = [], [], [], [], []
    base = 60 if root <= 6 else 48
    def put_motif(motif, bar0, lift=0, final=False):
        for i, (s, l, d) in enumerate(motif):
            t = bar0 * bar + s * unit
            if final and i == len(motif) - 1: d, l = 0 if d < 4 else 7, l + 4
            n = deg2midi(root, scale, d + lift, base)
            vel = 112 if (s * unit) % PPQ == 0 else 92
            lead.append((t, l * unit - 15, n, vel))
            dbl.append((t, l * unit - 15, n - 12, vel - 20))
    # A (1-4)  A (5-8)  B (9-12)  A final (13-16)
    put_motif(a["A"], 0); put_motif(a["A"], 2)
    put_motif(a["A"], 4); put_motif(a["A"], 6)
    put_motif(a["B"], 8); put_motif(a["B"], 10)
    put_motif(a["A"], 12); put_motif(a["A"], 14, final=True)
    for b in range(16):
        ch = a["prog"][b % 4]
        tones = [deg2midi(root, scale, ch + k, 48) for k in (0, 2, 4)]
        for n in tones:
            pad.append((b * bar, bar - 20, n, 62))
            for p in a["stabs"]:
                stabs.append((b * bar + p * S16, 2 * S16 - 20, n + 12, 100 if p == 0 else 84))
        r = deg2midi(root, scale, ch, 36)
        for p, l in ((0, 6), (6, 2), (8, 4), (12, 2), (14, 2)):   # bass groove on the chord root
            bass.append((b * bar + p * S16, l * S16 - 20, r if p != 14 else r + 12, 105))
    return dict(Lead=lead, LeadOctave=dbl, Stabs=stabs, Pad=pad, Bass=bass)

def vlq(n):
    out = [n & 0x7F]; n >>= 7
    while n: out.append((n & 0x7F) | 0x80); n >>= 7
    return bytes(reversed(out))

def track(events, ch, name, bpm=None):
    msgs = []
    if bpm: msgs.append((0, b"\xff\x51\x03" + struct.pack(">I", int(60_000_000 / bpm))[1:]))
    msgs.append((0, b"\xff\x03" + vlq(len(name)) + name.encode()))
    for t, l, n, v in events:
        msgs.append((t, bytes([0x90 | ch, n, v]))); msgs.append((t + max(10, l), bytes([0x80 | ch, n, 0])))
    msgs.sort(key=lambda m: (m[0], m[1][0] & 0xF0 == 0x90))
    data, last = b"", 0
    for t, m in msgs: data += vlq(t - last) + m; last = t
    data += vlq(0) + b"\xff\x2f\x00"
    return b"MTrk" + struct.pack(">I", len(data)) + data

if __name__ == "__main__":
    out = sys.argv[1]; os.makedirs(out, exist_ok=True)
    for a in ATTEMPTS:
        tr = build(a)
        name = f"KK Trap {a['name']} - {NOTE_NAMES[a['key']]} {a['scale']} {a['bpm']}BPM"
        with open(os.path.join(out, name + ".mid"), "wb") as f:
            f.write(b"MThd" + struct.pack(">IHHH", 6, 1, 5, PPQ))
            for i, (k, ev) in enumerate(tr.items()):
                f.write(track(ev, i, k, a["bpm"] if i == 0 else None))
        print(name)
