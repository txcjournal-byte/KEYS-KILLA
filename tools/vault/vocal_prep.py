#!/usr/bin/env python3
"""CHROME VAULT vocal source preparation (docs/VAULT_VOCAL_SOURCES.md).

VaultLibBuild reads one note per file with the note in the file name. Most of the free vocal material is not shaped like
that, so this script turns it into files VaultLibBuild can map (tools/vault/library_map_vocals.json):

  * "slice": a sung scale / melody (VocalSet demo clips) is cut into its stable-pitch notes (YIN pitch track, 10 ms hop;
    a note = >= 0.25 s within `tol` semitones of its running median, spread <= `maxSd` cents, not 42-50 cents between two
    semitones). Each note is written as
    <prefix>_<Note>_<rr>.wav (rr counts repeated notes) with a 15 ms fade-in and a 40 ms fade-out.
  * "sf2": the samples of one SoundFont preset (tools/vault/sf2_extract.py), stereo pairs merged.
  * "renote": a folder of <Note>[-n].wav files with unreliable octave numbers / intonation: linked as
    <prefix>_<Note>_<rr>.wav named after the nearest heard semitone (YIN).
  * "link": a single recording gets a note name: a symlink <prefix>_<Note>_1.<ext> -> the source (FLAC stays FLAC,
    VaultLibBuild decodes it).

Usage:  python3 -I tools/vault/vocal_prep.py [--src /home/user/vocal-src] [--out /home/user/vocal-src/prep]
Needs numpy only (stdlib `wave` reads the 16-bit PCM sources). Never writes into the repository.
"""
import argparse, json, os, sys, wave
import numpy as np

NAMES = 'C C# D D# E F F# G G# A A# B'.split()

def note_name(m):
    m = int(round(m))
    return f"{NAMES[m % 12]}{m // 12 - 1}"

def read_wav(path):
    with wave.open(path, 'rb') as w:
        ch, sw, sr, n = w.getnchannels(), w.getsampwidth(), w.getframerate(), w.getnframes()
        raw = w.readframes(n)
    if sw != 2:
        raise ValueError(f"{path}: only 16-bit PCM is supported")
    x = np.frombuffer(raw, dtype='<i2').astype(np.float64) / 32768.0
    return x.reshape(-1, ch), sr

def write_wav(path, x, sr):
    y = np.clip(np.round(x * 32767.0), -32768, 32767).astype('<i2')
    with wave.open(path, 'wb') as w:
        w.setnchannels(y.shape[1]); w.setsampwidth(2); w.setframerate(sr); w.writeframes(y.tobytes())

def yin(x, sr, fmin, fmax, thr=0.15):
    """YIN (de Cheveigne & Kawahara 2002) on one frame -> (f0, aperiodicity)."""
    tmax = int(sr / fmin); tmin = max(2, int(sr / fmax)); w = len(x) - tmax - 2
    if w < tmax:
        return 0.0, 1.0
    d = np.zeros(tmax + 2)
    for t in range(1, tmax + 2):
        e = x[:w] - x[t:t + w]
        d[t] = np.dot(e, e)
    cm = np.ones_like(d)
    run = np.cumsum(d[1:])
    cm[1:] = d[1:] * np.arange(1, tmax + 2) / np.maximum(run, 1e-12)
    idx, t = None, tmin
    while t < tmax:
        if cm[t] < thr:
            while t + 1 < tmax and cm[t + 1] < cm[t]:
                t += 1
            idx = t
            break
        t += 1
    if idx is None:
        idx = tmin + int(np.argmin(cm[tmin:tmax]))
    a, b, c = cm[idx - 1], cm[idx], cm[idx + 1]
    den = a - 2 * b + c
    tt = idx + (0.5 * (a - c) / den if abs(den) > 1e-12 else 0.0)
    return sr / tt, float(cm[idx])

def segments(mono, sr, tol, min_dur=0.25, fmin=70.0, fmax=1400.0):
    hop, win = int(0.01 * sr), int(0.05 * sr)
    frames = []
    for s in range(0, len(mono) - win, hop):
        seg = mono[s:s + win]
        db = 20 * np.log10(np.sqrt(np.mean(seg ** 2)) + 1e-12)
        f, ap = yin(seg, sr, fmin, fmax)
        frames.append((s / sr, 69 + 12 * np.log2(f / 440.0) if f > 0 else 0.0, ap, db))
    loud = max(fr[3] for fr in frames)
    out, cur = [], []

    def flush():
        if len(cur) * 0.01 >= min_dur:
            ms = np.array([c[1] for c in cur])
            out.append({'start': cur[0][0], 'end': cur[-1][0] + 0.05, 'midi': float(np.median(ms)),
                        'sd': float(np.std(ms) * 100.0), 'level': float(np.median([c[2] for c in cur]))})

    for t, m, ap, db in frames:
        ok = ap < 0.3 and db > loud - 30 and m > 0
        if ok and cur and abs(m - np.median([c[1] for c in cur[-10:]])) < tol:
            cur.append((t, m, db))
        else:
            flush()
            cur = [(t, m, db)] if ok else []
    flush()
    return out

def fade(x, sr, fin=0.015, fout=0.04):
    n = len(x)
    a, b = min(n, int(fin * sr)), min(n, int(fout * sr))
    if a > 0:
        x[:a] *= (0.5 - 0.5 * np.cos(np.pi * np.arange(a) / a))[:, None]
    if b > 0:
        x[n - b:] *= (0.5 + 0.5 * np.cos(np.pi * np.arange(b) / b))[:, None]
    return x

# what is prepared: (kind, source relative to --src, output folder, file prefix, options)
JOBS = [
    # VocalSet (CC BY 4.0), singer "f2" demo clips published in the authors' GitHub Pages repository
    # (the vibrato-Ah scale, the inhale and the Sonic Pi choir swell / vocal tone were evaluated and left out:
    #  docs/VAULT_VOCAL_SOURCES.md)
    ('slice', 'ial/demos/samples/f2_scales_straight_e.wav', 'vocalset', 'F2_straight_e', {'tol': 0.6, 'maxSd': 45}),
    ('slice', 'ial/demos/samples/f2_scales_breathy_e.wav',  'vocalset', 'F2_breathy_e',  {'tol': 0.6, 'maxSd': 45}),
    # MTG sms-tools-materials (CC BY 4.0, freesound pack 13038 by xserra)
    ('link',  'sms-tools-materials/sounds/singing-female.wav', 'mtg', 'Female_oo', {'note': 'G#4'}),
    ('link',  'sms-tools-materials/sounds/soprano-E4.wav',     'mtg', 'Soprano_ah', {'note': 'E4'}),
    # vocobox human-voice-dataset (MIT): note files whose octave numbers start at A (A3 = 110 Hz); renamed by the heard pitch
    ('renote', 'human-voice-dataset/data/voices/martin/notes/exports/mono', 'vocobox', 'Male_ah', {'skip': ['instable', 'ko', 'bended']}),
    # Fluid (R3) GM SoundFont (MIT, (c) Frank Wen), GitHub release pianobooster/fluid-soundfont v3.1
    ('sf2',   'fluid/FluidR3_GM.sf2', 'fluid', 'Choir_aahs', {'preset': '0:52'}),
    ('sf2',   'fluid/FluidR3_GM.sf2', 'fluid', 'Voice_oohs', {'preset': '0:53'}),
    ('sf2',   'fluid/FluidR3_GM.sf2', 'fluid', 'Solo_vox',   {'preset': '0:85', 'skip': 'chiff', 'roots': '0-90'}),
]

def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--src', default='/home/user/vocal-src')
    ap.add_argument('--out', default='/home/user/vocal-src/prep')
    args = ap.parse_args()
    manifest = []
    for kind, rel, folder, prefix, opt in JOBS:
        src = os.path.join(args.src, rel)
        odir = os.path.join(args.out, folder)
        os.makedirs(odir, exist_ok=True)
        for f in os.listdir(odir):
            if f.startswith(prefix + '_'):
                os.remove(os.path.join(odir, f))
        if not os.path.exists(src):
            print(f"!! missing {src}")
            continue
        if kind == 'sf2':   # tools/vault/sf2_extract.py does the work
            import subprocess
            cmd = [sys.executable, '-I', os.path.join(os.path.dirname(os.path.abspath(__file__)), 'sf2_extract.py'), src,
                   '--preset', opt['preset'], '--out', odir, '--prefix', prefix]
            if 'skip' in opt: cmd += ['--skip', opt['skip']]
            if 'roots' in opt: cmd += ['--roots', opt['roots']]
            subprocess.run(cmd, check=True)
            manifest.append({'file': f"{folder}/{prefix}_manifest.json", 'source': rel, 'preset': opt['preset']})
            continue
        if kind == 'renote':   # a folder of <Note>[-n].wav files: keep the pitch class, take the octave from the heard pitch
            count = {}
            for f in sorted(os.listdir(src)):
                if not f.endswith('.wav') or any(k in f for k in opt['skip']):
                    continue
                stem = f[:-4]
                parts = stem.split('-')
                pc_name = parts[0].rstrip('0123456789')
                if pc_name not in NAMES or len(parts) > 2 or (len(parts) == 2 and not parts[1].isdigit()):
                    print(f"   skip {f}: name not <Note>[-n]")
                    continue
                x, sr = read_wav(os.path.join(src, f))
                mono = x.mean(axis=1)
                mid = mono[len(mono) // 4: len(mono) // 4 + int(0.12 * sr)]
                f0, ap = yin(mid, sr, 60.0, 600.0)
                if f0 <= 0 or ap > 0.3:
                    print(f"   skip {f}: no clear pitch")
                    continue
                heard = 69 + 12 * np.log2(f0 / 440.0)
                # the singer drifts up to ~60 cents flat in the upper notes: the zone root is the nearest heard semitone
                # (VaultLibBuild stores the rest as fine tuning)
                midi = int(round(heard))
                name = note_name(midi)
                count[name] = count.get(name, 0) + 1
                dst = os.path.join(odir, f"{prefix}_{name}_{count[name]}.wav")
                os.symlink(os.path.join(src, f), dst)
                manifest.append({'file': os.path.relpath(dst, args.out), 'source': f"{rel}/{f}", 'heardMidi': round(heard, 2)})
                print(f"renote {f:14s} heard {heard:6.2f} -> {os.path.basename(dst)}")
            continue
        if kind == 'link':
            ext = os.path.splitext(src)[1]
            dst = os.path.join(odir, f"{prefix}_{opt['note']}_1{ext}")
            os.symlink(os.path.abspath(src), dst)
            manifest.append({'file': os.path.relpath(dst, args.out), 'source': rel, 'note': opt['note']})
            print(f"link  {rel} -> {os.path.relpath(dst, args.out)}")
            continue
        x, sr = read_wav(src)
        segs = segments(x.mean(axis=1), sr, opt['tol'])
        count = {}
        for s in segs:
            if s['sd'] > opt['maxSd']:
                print(f"   skip {rel} {s['start']:.2f}-{s['end']:.2f}: pitch spread {s['sd']:.0f} c")
                continue
            off = abs(s['midi'] - round(s['midi']))
            if off > 0.42:   # sung between two semitones: the root (and so the tuning) would be ambiguous
                print(f"   skip {rel} {s['start']:.2f}-{s['end']:.2f}: {s['midi']:.2f} is {off*100:.0f} c from the nearest semitone")
                continue
            name = note_name(s['midi'])
            count[name] = count.get(name, 0) + 1
            dst = os.path.join(odir, f"{prefix}_{name}_{count[name]}.wav")
            a, b = int(s['start'] * sr), int(s['end'] * sr)
            write_wav(dst, fade(x[a:b].copy(), sr), sr)
            manifest.append({'file': os.path.relpath(dst, args.out), 'source': rel, 'start': round(s['start'], 3),
                             'end': round(s['end'], 3), 'midi': round(s['midi'], 2), 'spreadCents': round(s['sd'], 1)})
            print(f"slice {rel} {s['start']:6.2f}-{s['end']:6.2f}  {name:4s} ({s['midi']:.2f}, sd {s['sd']:.0f} c) -> {os.path.basename(dst)}")
    with open(os.path.join(args.out, 'prep_manifest.json'), 'w') as fh:
        json.dump(manifest, fh, indent=1)
    print(f"{len(manifest)} files -> {args.out}")
    return 0

if __name__ == '__main__':
    sys.exit(main())
