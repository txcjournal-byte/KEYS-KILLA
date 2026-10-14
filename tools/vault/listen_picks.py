#!/usr/bin/env python3
"""Pick 4 representative presets per category for the owner's listening page.

usage: listen_picks.py report.md prints.tsv out_picks.tsv
#1 = the category's first accepted preset (recipes list the plain instrument first); #2..#4 = farthest-point picks
(gate fingerprint distance) among the first half of the category's accepted presets (plain instruments and
articulations come before treatments in every recipe), so the four sound as different as the category allows.
"""
import sys
import numpy as np

CATS = {"piano": "Piano", "keys": "Keys & Organ", "gtr": "Guitars", "bass": "Bass", "str": "Strings", "brass": "Brass",
        "wind": "Flutes & Woodwinds", "orch": "Orchestral", "bell": "Bells & Mallets", "vox": "Vocals & Choir",
        "pluck": "Plucks", "lead": "Leads", "pad": "Pads", "synth": "Synth", "world": "World", "fx": "FX & Atmos"}
# hand-picked first (the owner's own examples: grand to electric piano, nylon / steel / banjo / electric guitars, analog /
# distorted / bass guitar / orchestral basses, real orchestra, several flutes, different voices, ambient pads); a name the
# bank does not hold is skipped and the farthest-point pick fills the row
PREFER = {
    "Piano": ["Concert Grand", "Upright", "Tine Piano", "Felt Concert Grand"],
    "Keys & Organ": ["Full Pedal Organ", "Flemish Harpsichord", "Funk Keys", "Full Drawbars"],
    "Guitars": ["Classical Nylon", "Steel Acoustic", "Bluegrass Banjo", "Neck Electric", "Jazz Archtop"],
    "Bass": ["Finger Bass", "Ladder Saw Bass", "Growl Bass", "Contrabass Pizzicato", "Reese Bass"],
    "Strings": ["Full Strings", "Solo Violin", "Violin Pizzicato", "Chamber Strings"],
    "Brass": ["Trumpet", "French Horn", "Brass Stab Ensemble", "Full Brass"],
    "Flutes & Woodwinds": ["Concert Flute", "Piccolo", "Alto Recorder Vibrato", "Woodwind Choir", "Soprano Recorder"],
    "Orchestral": ["Orchestral Tutti", "Orchestra Hit", "Timpani Roll", "Woodwind Quartet"],
    "Bells & Mallets": ["Marimba", "Motor Vibes", "Glockenspiel", "Trap Bells"],
    "Vocals & Choir": ["Female Ha", "Chamber Choir", "Male Ah", "Female Whisper"],
    "Plucks": ["Glass Pluck", "Wood Pluck", "Saw Pluck"],
    "Leads": ["Thin Lead", "Whistle Lead", "Distorted Lead", "Theremin"],
    "Pads": ["Warm Pad", "Choir Pad", "Cathedral Pad"],
    "Synth": ["Poly Saw", "Rave Stab", "Square Stab"],
    "World": ["Dan Tranh", "Thumb Piano", "Mbira", "Zither and Flute"],
    "FX & Atmos": ["Riser", "Downlifter"],
}
EDGES = [0, 144, 156, 300, 312, 314]


def dist(a, b):
    t = 0.0
    for g in range(5):
        d = a[EDGES[g]:EDGES[g + 1]] - b[EDGES[g]:EDGES[g + 1]]
        t += float((d * d).mean())
    return (t / 5) ** 0.5


def main():
    rep, prints, out = sys.argv[1:4]
    layers, recipe = {}, {}
    for line in open(rep, encoding="utf-8"):
        if not line.startswith("| cv."):
            continue
        c = [x.strip() for x in line.strip().strip("|").split(" | ")]
        if c[4] != "ok":
            continue
        layers[c[0]] = c[3]
        recipe[c[0]] = c[2]
    rows = {}
    for line in open(prints, encoding="utf-8"):
        c = line.rstrip("\n").split("\t")
        if len(c) < 9 or c[2] != "ok":
            continue
        rows[c[0]] = (c[1], np.array([float(x) for x in c[8:]], dtype=np.float32))
    with open(out, "w", encoding="utf-8") as f:
        f.write("id\tname\tcategory\tlayers\trecipe\n")
        for slug, cat in CATS.items():
            ids = sorted([i for i in rows if i.split(".")[1] == slug], key=lambda i: int(i.split(".")[2]))
            if not ids:
                continue
            pool = ids[: max(4, len(ids) // 2)]
            byname = {rows[i][0]: i for i in ids}
            chosen = [byname[n] for n in PREFER.get(cat, []) if n in byname][:4]
            if not chosen:
                chosen = [pool[0]]
            while len(chosen) < min(4, len(pool)):
                best, bd = None, -1.0
                for i in pool:
                    if i in chosen:
                        continue
                    d = min(dist(rows[i][1], rows[j][1]) for j in chosen)
                    if d > bd:
                        best, bd = i, d
                chosen.append(best)
            for i in chosen:
                f.write(f"{i}\t{rows[i][0]}\t{cat}\t{layers.get(i, '')}\t{recipe.get(i, '')}\n")
                print(cat, "|", i, rows[i][0], "|", layers.get(i, ""))


if __name__ == "__main__":
    main()
