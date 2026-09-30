#!/usr/bin/env python3
"""Melody loops from a library of hand-written phrase fragments (style of the picked loops 02 / 08).

A loop is 8 bars, one track (low line + riff, like melody-pack MIDI):
    [OPENER | ANSWER] [OPENER | TURN] [OPENER | ANSWER] [OPENER | ENDING]
Bar 1 of every pair sits on the root (i), bar 2 on the bass plan's chord (VI, VII, iv, III or pedal),
the ending bar on the 5th / 7th below and leads back to the start.
Fragments are 1 bar: (start, length, semitone) in 16ths, semitones from the key root (12 = root an octave up).
Only fragments whose strong beats are chord tones of their bar are combined; the answer must start near
where the opener ends. Every loop is logged with its fragment ids, so ratings can prune bad fragments.

usage: melody_fragments.py <outdir> <count> [seed]"""
import random, struct, sys, os, csv

S = lambda *notes: [tuple(n) for n in notes]
E8 = lambda *p: [(i * 2, 2, x) for i, x in enumerate(p)]          # straight 8ths

OPENERS = {
    "O01": E8(12, 7, 8, 7, 12, 7, 3, 1),
    "O02": E8(12, 7, 8, 10, 12, 15, 14, 12),
    "O03": S((0, 3, 15), (3, 3, 14), (6, 2, 12), (8, 3, 10), (11, 3, 8), (14, 2, 7)),
    "O04": S((0, 2, 7), (2, 2, 8), (4, 2, 7), (6, 2, 3), (8, 4, 7), (12, 2, 8), (14, 2, 7)),
    "O05": S((0, 1, 12), (1, 1, 10), (2, 1, 8), (3, 1, 7), (4, 4, 12), (8, 2, 15), (10, 2, 12), (12, 4, 7)),
    "O06": S((0, 6, 12), (6, 2, 15), (8, 6, 14), (14, 2, 12)),
    "O07": E8(19, 15, 12, 15, 19, 15, 14, 12),
    "O08": S((0, 2, 12), (2, 2, 12), (4, 2, 15), (6, 2, 12), (8, 2, 19), (10, 2, 17), (12, 4, 15)),
    "O09": S((0, 4, 7), (4, 2, 8), (6, 2, 7), (8, 4, 12), (12, 2, 10), (14, 2, 8)),
    "O10": S((2, 2, 12), (6, 2, 15), (10, 2, 14), (14, 2, 12)),
    "O11": S((0, 3, 12), (3, 3, 7), (6, 2, 8), (8, 3, 12), (11, 3, 7), (14, 2, 3)),
    "O12": E8(15, 12, 8, 7, 15, 12, 8, 10),
    "O13": S((0, 1, 12), (1, 1, 13), (2, 2, 12), (4, 2, 7), (6, 2, 8), (8, 4, 7), (12, 4, 3)),
    "O14": S((0, 6, 19), (6, 2, 17), (8, 4, 15), (12, 2, 14), (14, 2, 12)),
    "O15": S((0, 2, 7), (2, 2, 12), (4, 2, 15), (6, 2, 17), (8, 4, 19), (12, 4, 15)),
    "O16": E8(24, 19, 15, 12, 19, 15, 12, 7),
    "O17": S((0, 2, 12), (3, 2, 12), (6, 2, 15), (8, 2, 14), (11, 2, 14), (14, 2, 12)),
    "O18": S((0, 4, 15), (4, 4, 14), (8, 4, 12), (12, 4, 10)),
    "O19": E8(3, 7, 12, 7, 3, 7, 13, 12),
    "O20": S((0, 3, 19), (3, 3, 15), (6, 3, 12), (9, 3, 15), (12, 2, 14), (14, 2, 15)),
}
ANSWERS = {
    "A01": S((0, 2, 12), (2, 2, 7), (4, 2, 8), (6, 2, 10), (8, 4, 15), (12, 2, 13), (14, 2, 12)),
    "A02": S((0, 2, 12), (2, 2, 7), (4, 2, 8), (6, 2, 10), (8, 8, 19)),
    "A03": S((0, 2, 15), (2, 2, 12), (4, 2, 8), (6, 2, 12), (8, 4, 15), (12, 4, 20)),
    "A04": S((0, 6, 8), (6, 2, 7), (8, 6, 3), (14, 2, 7)),
    "A05": S((0, 2, 12), (2, 2, 8), (4, 2, 3), (6, 2, 8), (8, 2, 12), (10, 2, 15), (12, 4, 17)),
    "A06": S((0, 3, 20), (3, 3, 19), (6, 2, 15), (8, 3, 12), (11, 3, 15), (14, 2, 17)),
    "A07": S((0, 2, 10), (2, 2, 14), (4, 2, 17), (6, 2, 14), (8, 4, 10), (12, 4, 12)),
    "A08": S((0, 4, 15), (4, 2, 17), (6, 2, 15), (8, 4, 14), (12, 4, 10)),
    "A09": S((0, 2, 8), (2, 2, 12), (4, 2, 15), (6, 2, 20), (8, 8, 19)),
    "A10": S((0, 1, 15), (1, 1, 14), (2, 2, 12), (4, 4, 8), (8, 2, 7), (10, 2, 8), (12, 4, 12)),
    "A11": S((0, 2, 17), (2, 2, 15), (4, 2, 14), (6, 2, 10), (8, 4, 14), (12, 4, 17)),
    "A12": S((0, 6, 12), (6, 2, 8), (8, 6, 15), (14, 2, 12)),
    "A13": E8(20, 15, 12, 8, 15, 12, 8, 7),
    "A14": S((0, 2, 22), (2, 2, 19), (4, 2, 17), (6, 2, 14), (8, 4, 17), (12, 4, 15)),
    "A15": S((0, 3, 15), (3, 3, 12), (6, 2, 10), (8, 8, 12)),
    "A16": S((0, 2, 3), (2, 2, 8), (4, 2, 12), (6, 2, 15), (8, 4, 12), (12, 2, 10), (14, 2, 8)),
}
TURNS = {
    "T01": S((0, 2, 12), (2, 2, 15), (4, 2, 19), (6, 2, 20), (8, 8, 24)),
    "T02": S((0, 2, 15), (2, 2, 17), (4, 4, 19), (8, 8, 22)),
    "T03": S((0, 1, 12), (1, 1, 14), (2, 1, 15), (3, 1, 17), (4, 1, 19), (5, 1, 20), (6, 2, 22), (8, 8, 24)),
    "T04": S((0, 4, 20), (4, 4, 19), (8, 8, 15)),
    "T05": S((0, 2, 12), (2, 2, 8), (4, 2, 12), (6, 2, 15), (8, 4, 20), (12, 4, 19)),
    "T06": S((0, 6, 17), (6, 2, 15), (8, 8, 14)),
    "T07": S((0, 2, 12), (2, 2, 7), (4, 2, 8), (6, 2, 10), (8, 4, 12), (12, 4, 24)),
    "T08": S((0, 2, 19), (2, 2, 20), (4, 2, 19), (6, 2, 15), (8, 8, 20)),
    "T09": S((0, 2, 14), (2, 2, 17), (4, 2, 22), (6, 2, 17), (8, 8, 22)),
    "T10": S((0, 2, 15), (2, 2, 19), (4, 2, 22), (6, 2, 19), (8, 4, 22), (12, 4, 26)),
}
ENDINGS = {   # bar 8 (bass on the 5th / b7 below), leads back to bar 1
    "E01": S((0, 2, 12), (2, 2, 7), (4, 2, 8), (6, 2, 7), (8, 2, 3), (10, 2, 2), (12, 4, 1)),
    "E02": S((0, 2, 15), (2, 2, 14), (4, 2, 12), (6, 2, 10), (8, 8, 7)),
    "E03": S((0, 2, 8), (2, 2, 7), (4, 2, 5), (6, 2, 3), (8, 4, 2), (12, 4, 7)),
    "E04": S((0, 4, 14), (4, 4, 12), (8, 4, 11), (12, 4, 7)),
    "E05": S((0, 2, 19), (2, 2, 17), (4, 2, 15), (6, 2, 14), (8, 2, 12), (10, 2, 10), (12, 4, 7)),
    "E06": S((0, 6, 10), (6, 2, 8), (8, 8, 7)),
    "E07": S((0, 2, 12), (2, 2, 7), (4, 2, 8), (6, 2, 10), (8, 2, 14), (10, 2, 12), (12, 4, 10)),
}
# bass plans: bar-2 bass (semitones from the root), ending bass, chord tones of bar 2
BASS = {
    "B1 VI":   (-4, -5, {8, 0, 3, 7}),
    "B2 VII":  (-2, -5, {10, 2, 5, 8}),
    "B3 iv":   (-7, -2, {5, 8, 0, 3}),
    "B4 III":  (-9, -2, {3, 7, 10, 2}),
    "B5 PEDAL": (0, -5, {0, 3, 7, 10, 2}),
}
CHORD_I = {0, 3, 7, 10, 2}
BASS_RHYTHM = {"held": [(0, 16)], "808": [(0, 6), (6, 4), (10, 2), (12, 4)]}

def fits(frag, chord):
    return all(s % 12 in chord for st, ln, s in frag if st in (0, 8))

def build(rng, pools):
    bname = rng.choice(list(BASS))
    b2, bend, chord2 = BASS[bname]
    opener = pools["O"].pop()
    o = OPENERS[opener]
    answers = [k for k, a in ANSWERS.items() if fits(a, chord2) and abs(a[0][2] - o[-1][2]) <= 7]
    turns = [k for k, t in TURNS.items() if fits(t, chord2)]
    if not answers or not turns:
        return None
    answer = rng.choice(answers)
    turn = rng.choice(turns)
    ending = rng.choice(list(ENDINGS))
    brhy = rng.choice(["held", "held", "808"])
    key = rng.choice([0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11])
    bpm = rng.choice([136, 138, 140, 142, 144, 146, 148, 150])
    return dict(opener=opener, answer=answer, turn=turn, ending=ending, bass=bname, brhy=brhy, key=key, bpm=bpm)

def notes(l):
    key = l["key"]
    hi = 60 + key if key <= 4 else 48 + key
    lo = 36 + key if key <= 4 else 24 + key
    b2, bend, _ = BASS[l["bass"]]
    bars = [(OPENERS[l["opener"]], 0), (ANSWERS[l["answer"]], b2), (OPENERS[l["opener"]], 0), (TURNS[l["turn"]], b2),
            (OPENERS[l["opener"]], 0), (ANSWERS[l["answer"]], b2), (OPENERS[l["opener"]], 0), (ENDINGS[l["ending"]], bend)]
    ev = []
    for bar, (frag, bass) in enumerate(bars):
        t0 = bar * 16
        for st, ln in BASS_RHYTHM[l["brhy"]]:
            ev.append((t0 + st, ln, lo + bass))
        for st, ln, s in frag:
            ev.append((t0 + st, ln, hi + s))
    return ev

def vlq(v):
    o = [v & 0x7F]; v >>= 7
    while v: o.append((v & 0x7F) | 0x80); v >>= 7
    return bytes(reversed(o))

def trk(msgs):
    msgs.sort(key=lambda m: (m[0], (m[1][0] & 0xF0) == 0x90))
    d, last = b"", 0
    for t, m in msgs: d += vlq(t - last) + m; last = t
    return b"MTrk" + struct.pack(">I", len(d) + 4) + d + vlq(0) + b"\xff\x2f\x00"

def write(path, l, reps=1):
    Q = 24   # ticks per 16th at 96 ppq
    tempo = trk([(0, b"\xff\x51\x03" + struct.pack(">I", int(60_000_000 / l["bpm"]))[1:])])
    msgs = []
    for k in range(reps):
        for st, ln, n in notes(l):
            t = (k * 128 + st) * Q
            msgs += [(t, bytes([0x90, n, 100])), (t + ln * Q - 2, bytes([0x80, n, 0]))]
    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 1, 2, 96) + tempo + trk(msgs))

if __name__ == "__main__":
    out, count = sys.argv[1], int(sys.argv[2])
    rng = random.Random(int(sys.argv[3]) if len(sys.argv) > 3 else 1)
    os.makedirs(out, exist_ok=True)
    NN = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
    op = list(OPENERS) * 3; rng.shuffle(op)
    pools = {"O": op}
    rows, i = [], 0
    while i < count and pools["O"]:
        l = build(rng, pools)
        if l is None: continue
        i += 1
        name = f"{i:02d} - {NN[l['key']]} MIN {l['bpm']}BPM"
        write(os.path.join(out, name + ".mid"), l)
        write(os.path.join(out, "_render_" + name + ".mid"), l, 2)
        rows.append([f"{i:02d}", NN[l["key"]], l["bpm"], l["opener"], l["answer"], l["turn"], l["ending"], l["bass"], l["brhy"]])
    with open(os.path.join(out, "fragments.csv"), "w", newline="") as f:
        csv.writer(f).writerows([["loop", "key", "bpm", "opener", "answer", "turn", "ending", "bass", "bass rhythm"]] + rows)
    for r in rows: print(*r)
