#!/usr/bin/env python3
"""Measure the CHROME VAULT physical models against physics / literature targets and real recordings.

    VaultModelLab -measure <renders>                      # direct voice renders (48 kHz, 24-bit)
    python3 -I tools/vault/models_measure.py <renders> [--sources /home/user/sgossner] [--only prefix]

For every variant at C5 (MIDI 60, FL Studio naming) it prints: peak level, pitch error, fundamental vs strongest partial,
2nd harmonic, 2-5 kHz energy share, spectral centroid, partial decay times (T60 of partials 1..6), inharmonicity B (fit of
f_k = k f0 sqrt((1 + B k^2) / (1 + B))), attack time, and for voices H1-H2 and LPC formants (rendered at C3 so the
harmonics sample the envelope densely). Each line is checked against the targets below (C5 comfort rules + literature);
the sweep renders (C2..C7) are checked for pitch (+-5 cents) and level evenness. Optional: the same measurements on
similar real recordings from the CC0 sources (harp / psaltery plucks, FM electric piano, recorder breath).
Only numpy is needed. Exit code = number of FAILs.
"""
import os
import struct
import sys

import numpy as np

# ---------------------------------------------------------------------------------------------------------------- I/O
def read_wav(path):
    with open(path, "rb") as f:
        data = f.read()
    if data[:4] != b"RIFF" or data[8:12] != b"WAVE":
        raise ValueError("not a wav: " + path)
    pos, fmt, raw = 12, None, None
    while pos + 8 <= len(data):
        cid, size = data[pos:pos + 4], struct.unpack("<I", data[pos + 4:pos + 8])[0]
        body = data[pos + 8:pos + 8 + size]
        if cid == b"fmt ":
            tag, ch, sr = struct.unpack("<HHI", body[:8])
            bits = struct.unpack("<H", body[14:16])[0]
            if tag == 0xFFFE:
                tag = struct.unpack("<H", body[24:26])[0]
            fmt = (tag, ch, sr, bits)
        elif cid == b"data":
            raw = body
        pos += 8 + size + (size & 1)
    tag, ch, sr, bits = fmt
    if tag == 3:
        x = np.frombuffer(raw, dtype="<f4" if bits == 32 else "<f8").astype(np.float64)
    elif bits == 16:
        x = np.frombuffer(raw, dtype="<i2").astype(np.float64) / 32768.0
    elif bits == 24:
        b = np.frombuffer(raw[: len(raw) // 3 * 3], dtype=np.uint8).reshape(-1, 3).astype(np.int32)
        v = b[:, 0] | (b[:, 1] << 8) | (b[:, 2] << 16)
        v = np.where(v >= 1 << 23, v - (1 << 24), v)
        x = v.astype(np.float64) / float(1 << 23)
    elif bits == 32:
        x = np.frombuffer(raw, dtype="<i4").astype(np.float64) / float(1 << 31)
    else:
        raise ValueError("unsupported wav format")
    x = x[: len(x) // ch * ch].reshape(-1, ch)
    return sr, x


def mono(x):
    return x.mean(axis=1)


def db(v):
    return 20.0 * np.log10(np.maximum(v, 1e-12))


def note_hz(n):
    return 440.0 * 2.0 ** ((n - 69) / 12.0)


# ----------------------------------------------------------------------------------------------------------- analysis
def spectrum(x, sr, t0, dur, nfft=1 << 17):
    a, b = int(t0 * sr), int(min(len(x), (t0 + dur) * sr))
    seg = x[a:b]
    if len(seg) < 256:
        return None, None
    w = np.hanning(len(seg))
    S = np.abs(np.fft.rfft(seg * w, n=max(nfft, len(seg))))
    f = np.fft.rfftfreq(max(nfft, len(seg)), 1.0 / sr)
    return f, S


def peak_near(f, S, hz, rel=0.03):
    lo, hi = np.searchsorted(f, hz * (1 - rel)), np.searchsorted(f, hz * (1 + rel)) + 1
    lo, hi = max(1, lo), min(len(S) - 2, hi)
    if hi <= lo:
        return hz, 1e-12
    k = lo + int(np.argmax(S[lo:hi]))
    a, b, c = np.log(S[k - 1] + 1e-20), np.log(S[k] + 1e-20), np.log(S[k + 1] + 1e-20)
    den = a - 2 * b + c
    d = 0.5 * (a - c) / den if abs(den) > 1e-12 else 0.0
    return (k + d) * (f[1] - f[0]), np.exp(b - 0.25 * (a - c) * d)


def pitch_cents(x, sr, f_expect, t0=0.35, dur=1.0, ensemble=False):
    """pitch of the fundamental: interpolated spectral peak; for ensembles (choir, pad: many detuned singers) the
    power-weighted mean frequency of the fundamental's band (+-60 cents), i.e. the pitch centre of the section"""
    f, S = spectrum(x, sr, t0, dur, nfft=1 << 19)
    if f is None:
        return 9999.0
    if ensemble:
        band = (f > f_expect * 2 ** (-60 / 1200)) & (f < f_expect * 2 ** (60 / 1200))
        p = S[band] ** 2
        hz = float((f[band] * p).sum() / max(1e-30, p.sum()))
    else:
        hz, _ = peak_near(f, S, f_expect, rel=0.06)
    return 1200.0 * np.log2(hz / f_expect)


def harmonics(x, sr, f0, t0=0.05, dur=0.5, kmax=20, B=0.0):
    f, S = spectrum(x, sr, t0, dur)
    out = []
    for k in range(1, kmax + 1):
        target = k * f0 * np.sqrt((1 + B * k * k) / (1 + B))
        if target > sr * 0.45:
            break
        fk, ak = peak_near(f, S, target, rel=0.02 if k > 1 else 0.04)
        # level = energy in a band around the partial (+-1.5 %, at least +-4 bins): robust for detuned ensembles,
        # vibrato and beating string pairs, where a single FFT peak under-reads the partial
        bw = max(4 * (f[1] - f[0]), 0.015 * fk)
        band = (f > fk - bw) & (f < fk + bw)
        ak = float(np.sqrt((S[band] ** 2).sum() / 1.5)) if band.any() else ak
        out.append((k, fk, ak))
    return out


def band_share(x, sr, lo, hi, t0=0.05, dur=0.75):
    f, S = spectrum(x, sr, t0, dur, nfft=1 << 15)
    p = S ** 2
    return float(p[(f >= lo) & (f < hi)].sum() / max(1e-30, p[1:].sum())), float((f * p).sum() / max(1e-30, p.sum()))


def fit_B(h):
    """least-squares inharmonicity from partial frequencies (partials within 45 dB of the strongest)"""
    if len(h) < 4:
        return 0.0
    amax = max(a for _, _, a in h)
    f1 = h[0][1]
    ks, ys = [], []
    for k, fk, ak in h:
        if k >= 2 and ak > amax * 10 ** (-45 / 20):
            ks.append(k)
            ys.append((fk / (k * f1)) ** 2)
    if len(ks) < 3:
        return 0.0
    ks, ys = np.array(ks, float), np.array(ys)
    # y = (1 + B k^2) / (1 + B) ~= 1 + B (k^2 - 1)
    A = ks ** 2 - 1
    return float(np.dot(A, ys - 1) / np.dot(A, A))


def partial_t60(x, sr, f0, k, B=0.0, end=None, frame=8192, hop=2048, ref_db=None):
    """T60 of partial k from the slope of its level (dB) between 0.1 s and the end of the held note / 40 dB down"""
    fk = k * f0 * np.sqrt((1 + B * k * k) / (1 + B))
    if fk > sr * 0.45:
        return None
    end = len(x) / sr if end is None else end
    w = np.hanning(frame)
    times, levels = [], []
    nfft = 1 << 15
    f = np.fft.rfftfreq(nfft, 1.0 / sr)
    for s in range(int(0.1 * sr), int(end * sr) - frame, hop):
        S = np.abs(np.fft.rfft(x[s:s + frame] * w, n=nfft))
        _, a = peak_near(f, S, fk, rel=0.015)
        times.append((s + frame / 2) / sr)
        levels.append(db(a))
    if len(levels) < 4:
        return None
    levels = np.array(levels)
    top = levels.max()
    if ref_db is not None and top < ref_db - 60:
        return None   # partial too weak to measure (pickup / pluck-position notch)
    keep = levels > top - 40
    i_end = len(levels) if keep.all() else int(np.argmin(keep))
    t, l = np.array(times[:max(i_end, 4)]), levels[:max(i_end, 4)]
    slope = np.polyfit(t, l, 1)[0]
    return float(-60.0 / slope) if slope < -1e-3 else 99.0


def ref_track(x, sr, f0, hold, frame=8192):
    """max level (dB, same STFT scale as partial_t60) of the fundamental: the reference for 'too weak to measure'"""
    w = np.hanning(frame)
    nfft = 1 << 15
    f = np.fft.rfftfreq(nfft, 1.0 / sr)
    best = -999.0
    for s in range(int(0.1 * sr), int(min(hold, 0.6) * sr), 2048):
        S = np.abs(np.fft.rfft(x[s:s + frame] * w, n=nfft))
        best = max(best, db(peak_near(f, S, f0, rel=0.015)[1]))
    return best


def attack_time(x, sr):
    env = np.abs(x)
    n = int(0.002 * sr)
    env = np.convolve(env, np.ones(n) / n, mode="same")
    pk = env.max()
    if pk <= 0:
        return 0.0
    a = np.argmax(env > 0.1 * pk)
    b = np.argmax(env > 0.9 * pk)
    return max(0.0, (b - a) / sr)


def lpc_formants(x, sr, order=12, fs_target=11025, t0=0.4, dur=0.6):
    step = max(1, int(round(sr / fs_target)))
    fs = sr / step
    seg = x[int(t0 * sr):int((t0 + dur) * sr)]
    # low-pass (windowed sinc) then decimate
    n = 129
    hcut = 0.45 / step
    tt = np.arange(n) - (n - 1) / 2
    h = 2 * hcut * np.sinc(2 * hcut * tt) * np.hamming(n)
    y = np.convolve(seg, h, mode="same")[::step]
    y = np.append(y[0], y[1:] - 0.7 * y[:-1]) * np.hamming(len(y))
    r = np.correlate(y, y, mode="full")[len(y) - 1:len(y) + order]
    a = np.zeros(order + 1)
    a[0] = 1.0
    e = r[0]
    for i in range(1, order + 1):
        acc = r[i] + np.dot(a[1:i], r[i - 1:0:-1])
        kk = -acc / e
        a[1:i + 1] = a[1:i + 1] + kk * a[i - 1::-1][:i]
        e *= 1 - kk * kk
    roots = [z for z in np.roots(a) if np.imag(z) > 0]
    fm = []
    for z in roots:
        fhz = np.angle(z) * fs / (2 * np.pi)
        bw = -np.log(abs(z)) * fs / np.pi
        if 150 < fhz < fs * 0.48 and bw < 500:
            fm.append((fhz, bw))
    return sorted(fm)


def envelope_formants(x, sr, f0, t0=0.3, dur=0.8, fmax=4000.0):
    """formants from the harmonic envelope: levels of the harmonics of a low note (dense sampling of the tract response),
    local maxima refined by a parabola through the neighbouring harmonics (log level)"""
    f, S = spectrum(x, sr, t0, dur)
    ks = range(1, int(fmax / f0))
    # +6 dB / octave: undo the falling slope of the glottal source above its own peak, leaving the tract envelope
    lv = np.array([db(peak_near(f, S, k * f0, rel=0.02)[1]) + 20 * np.log10(k) for k in ks])
    out = []
    for i in range(1, len(lv) - 1):
        if lv[i] >= lv[i - 1] and lv[i] > lv[i + 1]:
            a, b, c = lv[i - 1], lv[i], lv[i + 1]
            den = a - 2 * b + c
            d = 0.5 * (a - c) / den if abs(den) > 1e-9 else 0.0
            out.append(((i + 1 + d) * f0, b))
    return out


def hnr_db(x, sr, f0, t0=0.3, dur=0.7):
    """harmonic-to-noise ratio: energy near the harmonics (+-3 %, at least +-18 Hz) vs the rest (above 100 Hz)"""
    ff, SS = spectrum(x, sr, t0, dur, nfft=1 << 15)
    mask = np.zeros_like(ff, dtype=bool)
    for k in range(1, 40):
        mask |= np.abs(ff - k * f0) < max(0.03 * k * f0, 18.0)   # vibrato sidebands (+-3 x ~5.5 Hz) count as harmonic
    return 10 * np.log10((SS[mask] ** 2).sum() / max(1e-30, (SS[~mask & (ff > 100)] ** 2).sum()))


def clicks(x, sr):
    """largest |2nd difference| in the first 3 ms relative to the louder of the next 100 ms (attack click ratio)"""
    d2 = np.abs(np.diff(x, 2))
    a = d2[: int(0.003 * sr)].max(initial=0)
    b = d2[int(0.003 * sr):int(0.1 * sr)].max(initial=1e-12)
    return a / b


# --------------------------------------------------------------------------------------------------------- targets
# Literature / physics targets at C5 (MIDI 60 = 261.6 Hz). t60_1: T60 of the fundamental (s), t60_hi: T60 of partial 6
# (~1.6 kHz); B: inharmonicity range. Sources: plucked strings - Fletcher & Rossing ch. 9-10, Valimaki et al. 1996
# (guitar loss filters), Erkut et al. 2000 (classical guitar parameters: nylon high partials decay several times faster
# than the fundamental); banjo - Woodhouse, Politzer & Mansour 2021 (louder start, faster decay than guitar); electric
# basses / guitars - solid body: long sustain; tine / reed pianos - Pfeifle 2017 (tine pianos sustain long, reed pianos
# shorter); singing formants - Csound manual formant appendix; voice source - Fant 1995 (H1-H2 rises with Rd).
TARGETS = {
    "guitar.acoustic": dict(t60_1=(2.5, 9), t60_hi=(0.3, 3), B=(0, 4e-4)),
    "guitar.nylon": dict(t60_1=(1.8, 6), t60_hi=(0.1, 1.2), B=(0, 2e-4)),
    "guitar.banjo": dict(t60_1=(0.8, 3), t60_hi=(0.1, 1.5), B=(0, 5e-4)),
    "guitar.twelve": dict(t60_1=(2.5, 9), t60_hi=(0.3, 3), B=(0, 4e-4)),
    "guitar.electric": dict(t60_1=(3, 14), t60_hi=(0.5, 5), B=(0, 4e-4)),
    "guitar.crunch": dict(t60_1=(2, 14), t60_hi=(0.3, 6), B=(0, 1e-3)),
    "guitar.jazz": dict(t60_1=(1.5, 7), t60_hi=(0.1, 1.5), B=(0, 4e-4)),
    "bass.finger": dict(t60_1=(1.5, 8), t60_hi=(0.1, 2), B=(0, 2e-3)),
    "bass.pick": dict(t60_1=(1.5, 8), t60_hi=(0.1, 2.5), B=(0, 2e-3)),
    "bass.slap": dict(t60_1=(1.2, 8), t60_hi=(0.1, 3), B=(0, 2e-3)),
    "bass.fretless": dict(t60_1=(1.2, 7), t60_hi=(0.05, 1.2), B=(0, 2e-3)),
    "ep.tine.mellow": dict(t60_1=(3, 12)),
    "ep.tine.bell": dict(t60_1=(3, 12)),
    "ep.reed": dict(t60_1=(1.5, 6)),
    "ep.clav": dict(t60_1=(1.2, 6), B=(0, 1e-3)),
    "keys.toy": dict(t60_1=(0.8, 4)),
}
VOWEL_F12 = {  # (F1, F2) of the tables used: female = alto, male = tenor
    "voice_female_ah": (800, 1150), "voice_female_oh": (450, 800), "voice_female_oo": (325, 700), "voice_female_ee": (350, 1700),
    "voice_male_ah": (650, 1080), "voice_male_oh": (400, 800), "voice_male_oo": (350, 600), "voice_male_ee": (290, 1870),
}

VARIANT_MODEL = {}


def model_of(variant):
    return VARIANT_MODEL.get(variant, "")


def load_variants(lab_list):
    for line in lab_list.splitlines():
        p = line.split()
        if len(p) == 2:
            VARIANT_MODEL[p[0]] = p[1]


# --------------------------------------------------------------------------------------------------------- report
def measure_file(path, f0=note_hz(60), hold=2.5, model=""):
    sr, x2 = read_wav(path)
    x = mono(x2)
    r = {}
    r["peak"] = db(np.abs(x2).max())
    inharm = model.startswith(("guitar", "bass", "ep.clav"))
    ens = model in ("voice.choir", "voice.pad")
    # ensembles: singers beat against each other (slow, random H1 level): read the partials over 1.5 s after the attack
    ht0, hdur = (0.3, 1.5) if ens else (0.05, 0.5)
    h0 = harmonics(x, sr, f0, t0=ht0, dur=hdur, kmax=20)
    B = fit_B(h0) if inharm else 0.0
    h = harmonics(x, sr, f0, t0=ht0, dur=hdur, kmax=16, B=max(0.0, B)) if inharm else h0
    amps = [a for _, _, a in h]
    r["fund_vs_max"] = db(amps[0]) - db(max(amps)) if amps else 0
    r["h2"] = db(amps[1]) - db(amps[0]) if len(amps) > 1 else -99
    r["h1h2"] = -r["h2"]
    r["share"], r["centroid"] = band_share(x, sr, 2000, 5000)
    # voices: vibrato spreads the fundamental into FM sidebands - use the band's power centroid (the perceived centre)
    r["pitch"] = pitch_cents(x, sr, f0, t0=0.5 if ens else 0.35, dur=2.0 if ens else 1.0, ensemble=model.startswith("voice."))
    r["B"] = B
    ref = db(max(amps)) if amps else None
    r["t60"] = [partial_t60(x, sr, f0, k, max(0.0, B), end=hold, ref_db=None if k == 1 else ref_track(x, sr, f0, hold))
                for k in (1, 2, 3, 4, 6)]
    r["attack"] = attack_time(x, sr)
    r["click"] = clicks(x, sr)
    return r


def fmt_t60(t):
    return " ".join("  -  " if v is None else ("%5.2f" % v if v < 99 else "  >99") for v in t)


def check(name, r, model, fails, warns):
    msgs = []
    fx = "whisper" in name
    if r["peak"] > -1.0:
        msgs.append("FAIL peak > -1 dBFS")
    elif not (-15.0 <= r["peak"] <= -5.0) and "_v100" in name:
        msgs.append("WARN peak outside -15..-5 dBFS")
    if not fx and r["fund_vs_max"] < -3.0:
        msgs.append("FAIL fundamental %.1f dB under strongest partial" % r["fund_vs_max"])
    # ensembles (choir, pad): the pitch centre of 8-24 scattered singers (SD 12-14 cents) wanders a few cents
    tol = 10.0 if model in ("voice.choir", "voice.pad") else 5.0
    if not fx and abs(r["pitch"]) > tol and "_ha" not in name:
        msgs.append("FAIL pitch %+.1f c" % r["pitch"])
    if r["share"] > 0.15:
        msgs.append("FAIL 2-5 kHz share %.0f %%" % (r["share"] * 100))
    elif r["share"] > 0.08:
        msgs.append("WARN 2-5 kHz share %.0f %%" % (r["share"] * 100))
    tg = TARGETS.get(model, {})
    plain = "_mute" not in name and "_harmonic" not in name and "_ha" not in name
    if plain and "_v100" in name:
        t1 = r["t60"][0]
        if "t60_1" in tg and t1 is not None and not (tg["t60_1"][0] <= t1 <= tg["t60_1"][1]):
            msgs.append("WARN T60(1) %.2f s outside %s" % (t1, tg["t60_1"]))
        t6 = r["t60"][4]
        if "t60_hi" in tg and t6 is not None and not (tg["t60_hi"][0] <= t6 <= tg["t60_hi"][1]):
            msgs.append("WARN T60(6) %.2f s outside %s" % (t6, tg["t60_hi"]))
        if "B" in tg and not (tg["B"][0] <= r["B"] <= tg["B"][1]):
            msgs.append("WARN B %.1e outside %s" % (r["B"], tg["B"]))
    for m in msgs:
        (fails if m.startswith("FAIL") else warns).append(name + ": " + m)
    return msgs


def main():
    args = [a for a in sys.argv[1:]]
    if not args:
        print(__doc__)
        return 2
    d = args[0]
    sources = None
    only = ""
    if "--sources" in args:
        sources = args[args.index("--sources") + 1]
    if "--only" in args:
        only = args[args.index("--only") + 1]
    lst = os.path.join(d, "variants.txt")
    if os.path.exists(lst):
        load_variants(open(lst).read())
    fails, warns = [], []
    print("%-34s %6s %6s %7s %6s %6s %6s %8s %-29s %7s %6s" % ("variant @C5", "peak", "cents", "f1-max", "H2", "2-5k%",
                                                            "cent", "B", "T60 partials 1 2 3 4 6 (s)", "attack", "click"))
    for fn in sorted(os.listdir(d)):
        if not fn.endswith(".wav") or fn.startswith(("sweep_", "formant_")) or only not in fn:
            continue
        name = fn[:-4]
        variant = name.rsplit("_v", 1)[0]
        model = model_of(variant)
        r = measure_file(os.path.join(d, fn), model=model)
        msgs = check(name, r, model, fails, warns)
        print("%-34s %6.1f %+6.1f %7.1f %6.1f %6.1f %6.0f %8.1e %-29s %6.0fms %6.2f %s" % (
            name, r["peak"], r["pitch"] if abs(r["pitch"]) < 999 else 0, r["fund_vs_max"], r["h2"], r["share"] * 100,
            r["centroid"], r["B"], fmt_t60(r["t60"]), r["attack"] * 1000, r["click"], "; ".join(msgs)))

    # formants (vowels rendered at C2 = MIDI 36 by VaultModelLab -measure: harmonics every 65 Hz sample the envelope)
    print("\nformants (vowels at C2, harmonic envelope) F1 meas/target   F2 meas/target   H1-H2 at C5")
    for fn in sorted(os.listdir(d)):
        if not fn.startswith("formant_") or only not in fn:
            continue
        v = fn[len("formant_"):-4]
        if v not in VOWEL_F12:
            continue
        sr, x2 = read_wav(os.path.join(d, fn))
        tF1, tF2 = VOWEL_F12[v]
        # harmonic-envelope peaks (primary); LPC as a fallback when F1 / F2 merge into one envelope peak
        fm = envelope_formants(mono(x2), sr, note_hz(36))
        if len(fm) < 2:
            fm = lpc_formants(mono(x2), sr)
        f1 = min(fm, key=lambda p: abs(p[0] - tF1))[0] if fm else 0
        f2 = min(fm, key=lambda p: abs(p[0] - tF2))[0] if fm else 0
        e1, e2 = abs(f1 - tF1) / tF1, abs(f2 - tF2) / tF2
        c5 = os.path.join(d, v + "_v100.wav")
        h12 = measure_file(c5)["h1h2"] if os.path.exists(c5) else 0
        hnr = hnr_db(mono(read_wav(c5)[1]), 48000, note_hz(60)) if os.path.exists(c5) else 0
        flag = "" if e1 < 0.15 and e2 < 0.15 else "WARN formant off > 15 %"
        if flag:
            warns.append(v + ": " + flag)
        print("  %-32s %5.0f / %4d    %5.0f / %4d    %5.1f dB  HNR %4.1f dB %s" % (v, f1, tF1, f2, tF2, h12, hnr, flag))

    # voice quality: harmonic-to-noise ratio at C5 (literature: healthy sung vowels well above 15 dB, breathy phonation
    # roughly 5-15 dB, whisper below 0 dB - noise only). Reference: the alto recorder's breath (see --sources)
    print("\nvoice quality at C5: HNR (harmonic-to-noise ratio)")
    for v, lo, hi in (("voice_female_ah", 15, 60), ("voice_male_ah", 15, 60), ("voice_female_breathy", 5, 15), ("voice_male_breathy", 5, 15),
                      ("voice_female_whisper", -30, 0), ("voice_male_whisper", -30, 0), ("choir_mixed16_ah", 15, 45), ("vocalpad_oo", 10, 30)):
        p = os.path.join(d, v + "_v100.wav")
        if not os.path.exists(p) or (only and only not in v):
            continue
        h = hnr_db(mono(read_wav(p)[1]), 48000, note_hz(60))
        flag = "" if lo <= h <= hi else "WARN HNR outside %d..%d dB" % (lo, hi)
        if flag:
            warns.append(v + ": " + flag)
        print("  %-24s %5.1f dB  (target %d..%d) %s" % (v, h, lo, hi, flag))

    # sweeps: pitch +-5 cents C2..C7 and level evenness
    print("\nsweep C2..C7 (MIDI 24..84): pitch error (cents) / peak (dBFS)")
    models = sorted({fn[6:].rsplit("_", 1)[0] for fn in os.listdir(d) if fn.startswith("sweep_")})
    for m in models:
        if only and only not in m:
            continue
        row, peaks = [], []
        for n in range(24, 85, 12):
            p = os.path.join(d, "sweep_%s_%d.wav" % (m, n))
            if not os.path.exists(p):
                continue
            sr, x2 = read_wav(p)
            x = mono(x2)
            c = pitch_cents(x, sr, note_hz(n), t0=0.3, dur=1.2, ensemble=m.startswith("voice_"))
            pk = db(np.abs(x2).max())
            peaks.append(pk)
            row.append("%+5.1f/%5.1f" % (c, pk))
            if abs(c) > (10.0 if m in ("voice_choir", "voice_pad") else 5.0):
                fails.append("%s @%d: pitch %+.1f c" % (m, n, c))
            if pk > -1.0:
                fails.append("%s @%d: peak %.1f dBFS" % (m, n, pk))
        spread = (max(peaks) - min(peaks)) if peaks else 0
        if spread > 12:
            warns.append("%s: level spread %.1f dB over C2..C7" % (m, spread))
        print("  %-18s %s  spread %.1f dB" % (m, "  ".join(row), spread))

    if sources:
        compare_sources(sources)

    print("\n%d FAIL, %d WARN" % (len(fails), len(warns)))
    for m in fails + warns:
        print("  " + m)
    return len(fails)


def compare_sources(root):
    """the same measurements on similar real recordings (CC0 sources)"""
    refs = [
        # (VCSL file names sit one octave above the sounding pitch: "C3" files sound at 261.6 Hz = our C5)
        ("Folk harp 262 Hz (nylon-like string)", "vcsl2/Chordophones/Composite Chordophones/Folk Harp/EWHarp_Normal_C3_v3_RR1.wav", "guitar"),
        ("Folk harp 523 Hz", "vcsl2/Chordophones/Composite Chordophones/Folk Harp/EWHarp_Normal_C4_v3_RR1.wav", "guitar"),
        ("Psaltery pluck 523 Hz (steel string)", "vcsl2/Chordophones/Zithers/Psaltery, Bowed and Plucked/Pluck/BowedPsaltery_C4_Main_Pluck_rr1.wav", "guitar"),
        ("Concert harp 247 Hz (VSCO B3)", "vsco/Strings/Harp/KSHarp_B3_mf.wav", "guitar"),
        ("FM electric piano 262 Hz (TX81Z)", "vcsl2/Electrophones/TX81Z/Piano 1/Piano 1_C3_vl3.wav", "ep"),
        ("FM electric piano 262 Hz soft", "vcsl2/Electrophones/TX81Z/Piano 1/Piano 1_C3_vl1.wav", "ep"),
        ("FM piano 2 262 Hz (TX81Z)", "vcsl2/Electrophones/TX81Z/FM Piano/FMPiano_C3_vl3.wav", "ep"),
    ]
    for sub, label in (("Baroque Alto Recorder/Sustain", "Alto recorder (breath noise)"),):
        rec = os.path.join(root, "vcsl2/Aerophones/Edge-blown Aerophones", sub)
        if os.path.isdir(rec):
            fs = sorted(f for f in os.listdir(rec) if f.lower().endswith(".wav"))
            pick = [f for f in fs if "C5" in f or "C4" in f] or fs
            if pick:
                refs.append((label + " " + pick[0][-20:], os.path.join(rec, pick[0]), "breath"))
    print("\nreal recordings (same measurements; pitch is searched around the strongest low partial)")
    for label, rel, kind in refs:
        p = rel if os.path.isabs(rel) else os.path.join(root, rel)
        if not os.path.exists(p):
            print("  %-34s (missing)" % label)
            continue
        sr, x2 = read_wav(p)
        x = mono(x2)
        x = x / max(1e-9, np.abs(x).max())
        # find the actual fundamental (file names in the sources can be an octave off)
        f, S = spectrum(x, sr, 0.05, 0.6)
        lowband = (f > 50) & (f < 1500)
        fpk = f[lowband][np.argmax(S[lowband])]
        # step down to the lowest partial that is at least -20 dB of the peak and a subharmonic of it
        for div in (4, 3, 2):
            fc, ac = peak_near(f, S, fpk / div, rel=0.03)
            if db(ac) > db(S[lowband].max()) - 20:
                fpk = fc
                break
        f0, _ = peak_near(f, S, fpk, rel=0.03)
        h = harmonics(x, sr, f0, kmax=16)
        B = fit_B(h) if kind == "guitar" else 0.0
        amps = [a for _, _, a in h]
        share, cen = band_share(x, sr, 2000, 5000)
        t60 = [partial_t60(x, sr, f0, k, max(0.0, B)) for k in (1, 2, 3, 4, 6)]
        extra = ""
        if kind == "breath":
            extra = " HNR %.1f dB" % hnr_db(x, sr, f0)
        print("  %-34s f0 %6.1f Hz  f1-max %5.1f  H2 %5.1f  2-5k %4.1f %%  cent %5.0f  B %.1e  T60 %s%s" % (
            label, f0, db(amps[0]) - db(max(amps)), db(amps[1]) - db(amps[0]), share * 100, cen, B, fmt_t60(t60), extra))


if __name__ == "__main__":
    sys.exit(main())
