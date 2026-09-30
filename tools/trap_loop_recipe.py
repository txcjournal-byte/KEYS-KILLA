#!/usr/bin/env python3
"""The BREED LOOP recipe (from the loops the user picked: 02 and 08), as a generator.

Semitones are relative to the key root; the riff sits one octave above the root (root = 12).
  bar 1: low root held; 8th riff opens root-5th-b6-5th, then falls towards the dark b2
  bar 2: bass drops to b6 (or b7); riff opens the same, climbs and ends on a longer high note
  bars 1-2 x4: bar 4 ends with a high jump, bar 8 falls b3-2-b2 over the bass on the 5th below
One MIDI track, 8 bars. Seeded, so a seed always gives the same loop."""
import random, struct, sys, os

PPQ, Q = 96, 24
NN = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
OPEN   = [[12, 7, 8, 7], [12, 7, 8, 10], [12, 8, 7, 8], [15, 12, 7, 8]]           # bar openers (4 x 8th)
FALL1  = [[12, 7, 3, 1], [3, 2, 1, 3], [7, 3, 2, 1], [8, 7, 3, 1], [12, 10, 8, 7]] # rest of bar 1
CLIMB  = [[(10, 2), (15, 4), (13, 2), (12, 2)], [(10, 2), (12, 4), (15, 2), (14, 2)],
          [(10, 2), (15, 4), (17, 2), (15, 2)], [(12, 2), (15, 4), (13, 2), (12, 2)]]
PEAK   = [19, 24, 20, 17]                                                          # bar 4 high jump
BASS2  = [-4, -4, -2]                                                              # b6 (most), b7

def make(seed):
    r = random.Random(seed)
    key, bpm = r.randrange(12), r.choice([138, 140, 142, 144, 146, 148, 150])
    opn, fall, climb, peak, b2 = r.choice(OPEN), r.choice(FALL1), r.choice(CLIMB), r.choice(PEAK), r.choice(BASS2)
    hi = 60 + key if key <= 4 else 48 + key          # riff root around C4..E4 -> riff E4..G5
    lo = 36 + key if key <= 4 else 24 + key          # bass root around C2..B2
    ev = []
    def n(bar, s, l, semi, low=False):
        ev.append(((bar * 16 + s) * Q, l * Q - 2, (lo if low else hi) + semi))
    for rep in range(4):
        b = rep * 2
        n(b, 0, 16, 0, True)
        for i, s in enumerate(opn + fall): n(b, 2 * i, 2, s)
        if rep == 3:                                  # bar 8: turnaround
            n(b + 1, 0, 8, b2, True); n(b + 1, 8, 8, -5, True)
            for i, s in enumerate(opn): n(b + 1, 2 * i, 2, s)
            for i, s in enumerate([3, 2]): n(b + 1, 8 + 2 * i, 2, s)
            n(b + 1, 12, 4, 1)
            continue
        n(b + 1, 0, 16, b2, True)
        for i, s in enumerate(opn[:3]): n(b + 1, 2 * i, 2, s)
        t = 6
        for s, l in (climb if rep != 1 else climb[:2]):
            n(b + 1, t, l, s); t += l
        if rep == 1: n(b + 1, t, 16 - t, peak)
    name = f"KK Loop R{seed:03d} - {NN[key]} MIN {bpm}BPM"
    return name, bpm, ev

def vlq(v):
    o = [v & 0x7F]; v >>= 7
    while v: o.append((v & 0x7F) | 0x80); v >>= 7
    return bytes(reversed(o))

def trk(msgs):
    msgs.sort(key=lambda m: (m[0], (m[1][0] & 0xF0) == 0x90))
    d, last = b"", 0
    for t, m in msgs: d += vlq(t - last) + m; last = t
    d += vlq(0) + b"\xff\x2f\x00"
    return b"MTrk" + struct.pack(">I", len(d)) + d

def write(path, bpm, ev, reps=1):
    tempo = trk([(0, b"\xff\x51\x03" + struct.pack(">I", int(60_000_000 / bpm))[1:])])
    msgs = []
    for k in range(reps):
        for t, l, p in ev:
            t += k * 8 * 16 * Q
            msgs += [(t, bytes([0x90, p, 100])), (t + l, bytes([0x80, p, 0]))]
    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 1, 2, PPQ) + tempo + trk(msgs))

if __name__ == "__main__":
    out = sys.argv[1]; os.makedirs(out, exist_ok=True)
    for seed in map(int, sys.argv[2:]):
        name, bpm, ev = make(seed)
        write(os.path.join(out, name + ".mid"), bpm, ev)
        write(os.path.join(out, "_render16_" + name + ".mid"), bpm, ev, 2)
        print(name)
