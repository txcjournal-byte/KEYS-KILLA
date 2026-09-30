#!/usr/bin/env python3
"""Prototype: rule-based trap melody generator (16 bars) -> MIDI.
Melody = a short 2-bar motif (hand-written trap rhythms) placed on chord tones of a typical minor progression,
repeated and varied A A' B A''. Output: one MIDI file per melody with a Melody track and a Chords track."""
import random, struct, sys, os

PPQ = 480
STEP = PPQ // 4          # 16th note
TRIP = PPQ // 3          # 8th triplet

SCALES = {
    "Minor":          [0, 2, 3, 5, 7, 8, 10],
    "Harmonic Minor": [0, 2, 3, 5, 7, 8, 11],
    "Phrygian":       [0, 1, 3, 5, 7, 8, 10],
}
NOTE_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]

# progressions as scale-degree roots (0 = i), one chord per bar, 4 bars
PROGRESSIONS = {
    "Minor":          [[0, 5, 2, 6], [0, 5, 3, 4], [0, 0, 5, 6], [0, 6, 5, 6], [0, 3, 5, 4]],
    "Harmonic Minor": [[0, 5, 3, 4], [0, 3, 4, 0], [0, 5, 1, 4], [0, 0, 5, 4]],
    "Phrygian":       [[0, 1, 0, 1], [0, 1, 6, 0], [0, 5, 1, 0]],
}

# 2-bar rhythms: list of (start, length) in 16ths (32 steps = 2 bars); 't' entries are triplet runs
RHYTHMS = [
    [(0, 3), (3, 3), (6, 2), (8, 2), (10, 2), (12, 4), (16, 3), (19, 3), (22, 2), (24, 6)],              # dotted bounce
    [(0, 2), (2, 2), (4, 2), (7, 1), (8, 4), (14, 2), (16, 2), (18, 2), (20, 2), (23, 1), (24, 8)],      # bell run + hold
    [(0, 4), (6, 2), (8, 2), (10, 2), (12, 4), (16, 4), (22, 2), (24, 2), (26, 2), (28, 4)],             # call / answer
    [(0, 1), (1, 1), (2, 2), (4, 2), (6, 2), (8, 6), (16, 1), (17, 1), (18, 2), (20, 2), (22, 2), (24, 8)],  # fast pickup
    [(0, 3), (3, 3), (6, 4), (12, 2), (14, 2), (16, 3), (19, 3), (22, 4), (28, 4)],                     # syncopated
    [(0, 6), (6, 2), (8, 4), (12, 4), (16, 6), (22, 2), (24, 8)],                                         # slow dark
    "triplet",
]
TRIPLET_2BAR = [(0, 1), (1, 1), (2, 1), (3, 2), (6, 1), (7, 1), (8, 2), (12, 1), (13, 1), (14, 1), (15, 2), (18, 3), (21, 3)]  # in 8th-triplets (24 per 2 bars)

def degree_to_midi(root, scale, deg, octave_base):
    o, d = divmod(deg, 7)
    return octave_base + root + 12 * o + scale[d]

def make_melody(seed):
    rnd = random.Random(seed)
    scale_name = rnd.choice(["Minor", "Minor", "Harmonic Minor", "Phrygian"])
    scale = SCALES[scale_name]
    root = rnd.randrange(12)
    prog = rnd.choice(PROGRESSIONS[scale_name])
    rhythm = rnd.choice(RHYTHMS)
    base = 60 if root < 7 else 48      # melody around C4..C5
    bpm = rnd.choice([130, 140, 145, 150, 160])

    # events in ticks for the 2-bar motif
    if rhythm == "triplet":
        motif_times = [(s * TRIP, l * TRIP) for s, l in TRIPLET_2BAR]
        rname = "triplet"
    else:
        motif_times = [(s * STEP, l * STEP) for s, l in rhythm]
        rname = f"r{RHYTHMS.index(rhythm)}"

    # pitch contour of the motif: steps within the scale, anchored on chord tones on strong beats
    contour = []
    cur = rnd.choice([0, 2, 4, 7])      # start on a chord tone (scale degrees, 7 = octave)
    for i, (t, l) in enumerate(motif_times):
        strong = (t % PPQ == 0)
        if i == 0:
            contour.append(cur); continue
        move = rnd.choice([-2, -1, -1, 1, 1, 2, 0, 0, 3, -2])
        cur = max(-1, min(7, cur + move))
        if strong and cur % 7 not in (0, 2, 4):          # land strong beats on chord tones
            cur += rnd.choice([-1, 1])
        contour.append(cur)
    contour[-1] = rnd.choice([0, 4, 2])                     # motif ends on a stable tone

    def snap_to_chord(deg, chord):
        """nearest chord tone (scale degrees, any octave)"""
        cands = [chord + k + o for k in (0, 2, 4) for o in (-14, -7, 0, 7, 14)]
        return min(cands, key=lambda c: (abs(c - deg), c))

    def phrase(bar0, variant):
        """4 bars = the 2-bar motif twice over the progression; harmony follows the chords"""
        out = []
        n = len(motif_times)
        prev_deg = contour[0]
        for half in range(2):
            for i, (t, l) in enumerate(motif_times):
                abs_t = bar0 * 4 * PPQ + half * 8 * PPQ + t
                bar = abs_t // (4 * PPQ)
                chord = prog[bar % 4]
                deg = contour[i]
                if variant == "lift": deg += 2 if half == 1 else 0            # B part: answer a third higher
                if variant == "sparse" and i % 3 == 1: continue
                if t % PPQ == 0: deg = snap_to_chord(deg, chord)
                if half == 1 and i >= n - 2 and variant in ("answer", "final"):   # cadence: nearest chord tone / tonic
                    deg = snap_to_chord (prev_deg, chord) if variant == "answer" else min ((0, 7), key=lambda c: abs (c - prev_deg))
                prev_deg = deg
                note = degree_to_midi(root, scale, deg, base)
                vel = 105 if t % PPQ == 0 else rnd.randint(78, 96)
                out.append((abs_t, min(l, 8 * PPQ - t) - 10, note, vel))
        return out

    events = []
    # A A' B A'' (each 4 bars)
    events += phrase(0, "motif")
    events += phrase(4, "answer")
    events += phrase(8, rnd.choice(["lift", "sparse"]))
    events += phrase(12, "final")
    # last note long on the root, nearest to where the line ends
    last_t = 16 * 4 * PPQ - 4 * PPQ
    events = sorted(e for e in events if e[0] < last_t)
    prev = events[-1][2]
    fin = min((base + root + 12 * o for o in range(-2, 3)), key=lambda n: abs(n - prev))
    events.append((last_t, 4 * PPQ - 20, fin, 100))
    # whole melody into a comfortable register (average around G4)
    mean = sum(e[2] for e in events) / len(events)
    shift = 12 * round((67 - mean) / 12)
    events = [(t, l, n + shift, v) for t, l, n, v in events]
    chords = []
    for bar in range(16):
        d = prog[bar % 4]
        tones = [degree_to_midi(root, scale, d + k, 48 if root < 5 else 36) for k in (0, 2, 4)]
        for n in tones:
            chords.append((bar * 4 * PPQ, 4 * PPQ - 20, n, 70))
    desc = f"{NOTE_NAMES[root]} {scale_name}, {bpm} BPM, chords {'-'.join(['i','ii','III','iv','v','VI','VII'][d] for d in prog)}, rhythm {rname}"
    return events, chords, bpm, desc, NOTE_NAMES[root] + " " + scale_name

def vlq(n):
    b = [n & 0x7F]; n >>= 7
    while n: b.append((n & 0x7F) | 0x80); n >>= 7
    return bytes(reversed(b))

def track(events, channel, name, bpm=None):
    msgs = []
    if bpm: msgs.append((0, b"\xff\x51\x03" + struct.pack(">I", int(60_000_000 / bpm))[1:]))
    msgs.append((0, b"\xff\x03" + vlq(len(name)) + name.encode()))
    for t, l, n, v in events:
        msgs.append((t, bytes([0x90 | channel, n, v])))
        msgs.append((t + max(10, l), bytes([0x80 | channel, n, 0])))
    msgs.sort(key=lambda m: (m[0], m[1][0] & 0xF0 == 0x90))
    data = b""; last = 0
    for t, m in msgs:
        data += vlq(t - last) + m; last = t
    data += vlq(0) + b"\xff\x2f\x00"
    return b"MTrk" + struct.pack(">I", len(data)) + data

def write_midi(path, tracks):
    with open(path, "wb") as f:
        f.write(b"MThd" + struct.pack(">IHHH", 6, 1, len(tracks), PPQ))
        for t in tracks: f.write(t)

if __name__ == "__main__":
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    lines = []
    for i in range(10):
        ev, ch, bpm, desc, key = make_melody(1000 + i * 17)
        name = f"KK Trap Melody {i + 1:02d} - {key} {bpm}BPM"
        write_midi(os.path.join(out, name + ".mid"), [track(ev, 0, "Melody", bpm), track(ch, 1, "Chords")])
        lines.append(f"{i + 1:02d}: {desc}")
        print(name, "|", desc)
    open(os.path.join(out, "info.txt"), "w").write("\n".join(lines) + "\n")
