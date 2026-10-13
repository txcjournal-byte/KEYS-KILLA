#pragma once
// CHROME VAULT bank quality gate - shared by the bank generator (tools/vault/BankGen.cpp, target VaultBankGen) and the
// shipped-bank checks in tests/VaultTests.cpp. Header-only; needs VaultEngine (Source/vault) and juce_dsp (FFT).
//
// One analysis of an offline render (VaultEngine::render: note on at 0, note off at 70 %, release tail to the end):
//   loudness   K-weighted (BS.1770 shelf + RLB high-pass) RMS, loudest 400 ms window, both channels  ("LUFS-like", dB)
//   peak, DC, NaN, silence
//   clicks     note start (first sample + sample jump in the first 2 ms vs the next 200 ms),
//              note end (sample jump around the note-off vs the 90 ms before it)
//   stereo     mono-sum loss (stereo power / mono-sum power), side energy under 90 Hz vs the whole signal, side / mid ratio
//   tail       release decay fitted after the note-off, extrapolated to -60 dB below the loudest window
//   colour     spectral centroid, attack time (to 50 % of the envelope peak)
//   print      a small spectral + envelope fingerprint for near-duplicate detection
#include "../../Source/vault/VaultEngine.h"
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <cmath>
#include <vector>

namespace vaultqc
{
// test notes (MIDI, C3 = 60 as in most DAWs): 808 & Bass at C1 / C2, everything else at C3 / C5
inline int noteLow (int category)  { return category == vault::cat808Bass ? 36 : 60; }
inline int noteHigh (int category) { return category == vault::cat808Bass ? 48 : 84; }
constexpr double sampleRate = 48000.0;
constexpr float velocity = 0.8f;
constexpr double baseSeconds = 1.5;

// loudness target per category (K-weighted, loudest 400 ms; bass / 808 louder, pads / atmospheres quieter); tolerance +-1 dB
inline float targetLoudness (int category)
{
    static const float t[vault::numCategories] {
        -12.0f,   // 808 & Bass
        -15.0f,   // Bells & Mallets
        -14.5f,   // Piano & Keys
        -14.5f,   // Plucks
        -13.5f,   // Leads
        -16.0f,   // Pads
        -15.0f,   // Strings
        -14.0f,   // Brass & Horns
        -15.0f,   // Flutes & Winds
        -14.5f,   // Guitars
        -15.5f,   // Vocals & Choir
        -14.0f,   // Synth
        -14.5f,   // Arps & Sequences
        -14.5f,   // Orchestral
        -15.0f,   // World
        -17.0f }; // FX & Atmos
    return t[juce::jlimit (0, (int) vault::numCategories - 1, category)];
}
constexpr float loudnessTolerance = 1.0f;
constexpr float peakCeiling = 0.89f;        // the engine's safety limiter acts above this: a preset must stay under it
inline float tailLimitSeconds (int category)  // release tail to -60 dB (pads and atmospheres may ring longer)
{
    return category == vault::catPads || category == vault::catFxAtmos ? 16.0f : 8.0f;
}

// length of the QC render: 1.5 s, longer for slow attacks so the loudness is measured on the full note
inline double renderSeconds (const VaultPreset& p)
{
    float att = 0;
    for (auto& part : p.parts)
        for (auto& [id, v] : part.values) if (id == "attack") att = std::max (att, v);
    return juce::jlimit (baseSeconds, 4.0, baseSeconds + 1.5 * (double) att);
}

constexpr int fpBands = 20, fpEarly = 6, fpEnv = 10, fpSize = fpBands + fpEarly + fpEnv + 1;
using Print = std::array<float, fpSize>;

struct Analysis
{
    bool finite = true;
    float peak = 0, rms = 0, dc = 0;
    float loud = -120;                          // K-weighted loudest 400 ms, dB
    float firstSample = 0, onsetJump = 0, laterJump = 0, offJump = 0, preOffJump = 0;
    float monoLossDb = 0, lowSideDb = -120, sideToMidDb = -120;
    float tailSeconds = 0;
    float centroidHz = 0, attackMs = 0;
    Print print {};
};

namespace detail
{
struct Bq
{
    double b0, b1, b2, a1, a2, z1 = 0, z2 = 0;
    inline double tick (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};
inline Bq lowPass (double hz, double q, double sr)
{
    const double w = juce::MathConstants<double>::twoPi * hz / sr, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
    return { (1 - cs) / 2 / a0, (1 - cs) / a0, (1 - cs) / 2 / a0, -2 * cs / a0, (1 - al) / a0 };
}
// BS.1770 K-weighting for any sample rate (pre-filter shelf + RLB high-pass), the standard analog prototypes
inline void kWeighting (double sr, Bq& shelf, Bq& hp)
{
    {
        const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / sr), Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
        const double a0 = 1.0 + K / Q + K * K;
        shelf = { (Vh + Vb * K / Q + K * K) / a0, 2.0 * (K * K - Vh) / a0, (Vh - Vb * K / Q + K * K) / a0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
    }
    {
        const double f0 = 38.13547087602444, Q = 0.5003270373238773;
        const double K = std::tan (juce::MathConstants<double>::pi * f0 / sr), a0 = 1.0 + K / Q + K * K;
        hp = { 1.0, -2.0, 1.0, 2.0 * (K * K - 1.0) / a0, (1.0 - K / Q + K * K) / a0 };
    }
}
inline float db (double power) { return (float) (10.0 * std::log10 (std::max (1.0e-14, power))); }
} // namespace detail

inline Analysis analyse (const juce::AudioBuffer<float>& b, double sr, double noteOffSeconds)
{
    using namespace detail;
    Analysis a;
    const int n = b.getNumSamples();
    if (n < 64 || b.getNumChannels() < 2) { a.finite = false; return a; }
    const float* L = b.getReadPointer (0);
    const float* R = b.getReadPointer (1);
    const int off = juce::jlimit (1, n - 1, (int) std::round (noteOffSeconds * sr));

    double sum = 0, sumL = 0, sumR = 0, pMono = 0;
    for (int i = 0; i < n; ++i)
    {
        if (! std::isfinite (L[i]) || ! std::isfinite (R[i])) { a.finite = false; return a; }
        a.peak = std::max (a.peak, std::max (std::abs (L[i]), std::abs (R[i])));
        sum += 0.5 * ((double) L[i] * L[i] + (double) R[i] * R[i]);
        sumL += L[i]; sumR += R[i];
        const double m = 0.5 * ((double) L[i] + R[i]);
        pMono += m * m;
    }
    a.rms = (float) std::sqrt (sum / n);
    a.dc = (float) std::max (std::abs (sumL / n), std::abs (sumR / n));
    a.monoLossDb = db (sum / n) - db (pMono / n);

    auto jump = [&] (int from, int to)
    {
        float j = 0;
        for (int i = std::max (1, from); i < std::min (n, to); ++i)
            j = std::max (j, std::max (std::abs (L[i] - L[i - 1]), std::abs (R[i] - R[i - 1])));
        return j;
    };
    const int ms2 = (int) (0.002 * sr);
    a.firstSample = std::max (std::abs (L[0]), std::abs (R[0]));
    a.onsetJump = jump (1, ms2);
    a.laterJump = jump (ms2, (int) (0.2 * sr));
    a.offJump = jump (off - 64, off + 512);
    a.preOffJump = jump (off - 4096, off - 64);

    // K-weighted loudness: loudest 400 ms window (100 ms hop)
    {
        Bq s1, h1, s2, h2;
        kWeighting (sr, s1, h1); s2 = s1; h2 = h1;
        const int hop = (int) (0.1 * sr), win = 4 * hop;
        std::vector<double> blocks ((size_t) (n / hop + 1), 0.0);
        for (int i = 0; i < n; ++i)
        {
            const double kl = h1.tick (s1.tick (L[i])), kr = h2.tick (s2.tick (R[i]));
            blocks[(size_t) (i / hop)] += kl * kl + kr * kr;
        }
        double best = 0;
        const int nb = (int) blocks.size();
        for (int k = 0; k + 4 <= std::max (4, nb); ++k)
        {
            double e = 0;
            for (int j = k; j < std::min (nb, k + 4); ++j) e += blocks[(size_t) j];
            best = std::max (best, e / win);
        }
        a.loud = -0.691f + db (best);
    }

    // side under 90 Hz (8th-order low-pass) against the whole signal
    {
        Bq s[4] { lowPass (90, 0.5412, sr), lowPass (90, 1.3066, sr), lowPass (90, 0.5412, sr), lowPass (90, 1.3066, sr) };
        double es = 0, eSide = 0, eMid = 0;
        for (int i = 0; i < n; ++i)
        {
            const double side = 0.5 * ((double) L[i] - R[i]), mid = 0.5 * ((double) L[i] + R[i]);
            const double x = s[3].tick (s[2].tick (s[1].tick (s[0].tick (side))));
            es += x * x; eSide += side * side; eMid += mid * mid;
        }
        a.lowSideDb = db (es / n) - db (sum / n);
        a.sideToMidDb = db (eSide) - db (eMid);
    }

    // envelope in 10 ms frames (mono power)
    const int fr = (int) (0.01 * sr), nf = n / fr;
    std::vector<float> envDb ((size_t) nf);
    float envMax = -200;
    for (int f = 0; f < nf; ++f)
    {
        double e = 0;
        for (int i = f * fr; i < (f + 1) * fr; ++i) e += 0.5 * ((double) L[i] * L[i] + (double) R[i] * R[i]);
        envDb[(size_t) f] = db (e / fr);
        envMax = std::max (envMax, envDb[(size_t) f]);
    }
    // attack: first frame within 6 dB of the peak frame
    for (int f = 0; f < nf; ++f) if (envDb[(size_t) f] >= envMax - 6.0f) { a.attackMs = (float) f * 10.0f; break; }

    // release tail: linear fit of the frame levels from 60 ms after the note-off to the end, extrapolated to -60 dB below the peak
    {
        const int f0 = (off + (int) (0.06 * sr)) / fr;
        double sx = 0, sy = 0, sxx = 0, sxy = 0; int cnt = 0;
        for (int f = f0; f < nf; ++f)
        {
            if (envDb[(size_t) f] < envMax - 75.0f) continue;   // already gone
            const double x = (double) (f - f0) * 0.01, y = envDb[(size_t) f];
            sx += x; sy += y; sxx += x * x; sxy += x * y; ++cnt;
        }
        const float floorDb = envMax - 60.0f;
        const float endDb = nf > 0 ? envDb[(size_t) (nf - 1)] : -200.0f;
        const double elapsed = (double) (n - off) / sr;
        if (endDb < floorDb)   // decayed inside the render: where it crossed -60 dB
        {
            a.tailSeconds = 0;
            for (int f = off / fr; f < nf; ++f)
                if (envDb[(size_t) f] < floorDb) { a.tailSeconds = std::max (0.0f, (float) (f * fr - off) / (float) sr); break; }
        }
        else if (cnt < 4) a.tailSeconds = (float) elapsed;
        else
        {
            const double slope = (cnt * sxy - sx * sy) / std::max (1.0e-12, cnt * sxx - sx * sx);   // dB per second (negative)
            a.tailSeconds = slope < -0.5 ? (float) (elapsed + (endDb - floorDb) / -slope) : 99.0f;
        }
    }

    // spectrum: FFT 2048 frames over the held part of the note -> centroid and 20 log bands (50 Hz .. 16 kHz)
    {
        constexpr int order = 11, size = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<float> buf ((size_t) (2 * size)), mag ((size_t) (size / 2), 0.0f), magEarly ((size_t) (size / 2), 0.0f);
        std::vector<float> win ((size_t) size);
        for (int i = 0; i < size; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size);
        auto frame = [&] (int start, std::vector<float>& acc)
        {
            std::fill (buf.begin(), buf.end(), 0.0f);
            for (int i = 0; i < size; ++i)
            {
                const int k = start + i;
                buf[(size_t) i] = k < n ? 0.5f * (L[k] + R[k]) * win[(size_t) i] : 0.0f;
            }
            fft.performFrequencyOnlyForwardTransform (buf.data());
            for (int k = 0; k < size / 2; ++k) acc[(size_t) k] += buf[(size_t) k] * buf[(size_t) k];
        };
        frame (0, magEarly);
        for (int s = size / 2; s + size <= off; s += size) frame (s, mag);
        if (off < size + size / 2) frame (0, mag);
        double cw = 0, cs = 0;
        for (int k = 1; k < size / 2; ++k) { const double f = k * sr / size; cw += f * mag[(size_t) k]; cs += mag[(size_t) k]; }
        a.centroidHz = (float) (cs > 0 ? cw / cs : 0);

        auto bands = [&] (const std::vector<float>& m, int nb, float* out)
        {
            double tot = 0;
            std::vector<double> e ((size_t) nb, 0.0);
            for (int k = 1; k < size / 2; ++k)
            {
                const double f = k * sr / size;
                if (f < 50.0 || f >= 16000.0) continue;
                const int bi = juce::jlimit (0, nb - 1, (int) (std::log (f / 50.0) / std::log (16000.0 / 50.0) * nb));
                e[(size_t) bi] += m[(size_t) k]; tot += m[(size_t) k];
            }
            for (int i = 0; i < nb; ++i) out[i] = juce::jlimit (-60.0f, 0.0f, db (e[(size_t) i]) - db (tot));
        };
        bands (mag, fpBands, a.print.data());
        bands (magEarly, fpEarly, a.print.data() + fpBands);
    }
    // envelope print: 10 segments over the render, dB below the peak (floor -60)
    for (int s = 0; s < fpEnv; ++s)
    {
        const int fa = s * nf / fpEnv, fb = std::max (fa + 1, (s + 1) * nf / fpEnv);
        float m = -200;
        for (int f = fa; f < std::min (nf, fb); ++f) m = std::max (m, envDb[(size_t) f]);
        a.print[(size_t) (fpBands + fpEarly + s)] = juce::jlimit (-60.0f, 0.0f, m - envMax);
    }
    a.print[(size_t) (fpSize - 1)] = juce::jlimit (-40.0f, 10.0f, a.sideToMidDb);
    return a;
}

// distance of two fingerprints (dB-like units): spectrum + attack spectrum + envelope + width
inline float printDistance (const Print& x, const Print& y)
{
    double s = 0;
    for (int i = 0; i < fpBands; ++i) { const double d = x[(size_t) i] - y[(size_t) i]; s += d * d; }
    double e = 0;
    for (int i = fpBands; i < fpBands + fpEarly; ++i) { const double d = x[(size_t) i] - y[(size_t) i]; e += d * d; }
    double v = 0;
    for (int i = fpBands + fpEarly; i < fpBands + fpEarly + fpEnv; ++i) { const double d = x[(size_t) i] - y[(size_t) i]; v += d * d; }
    const double w = 0.5 * (x[(size_t) (fpSize - 1)] - y[(size_t) (fpSize - 1)]);
    return (float) std::sqrt (s / fpBands + 0.5 * e / fpEarly + v / fpEnv + w * w * 0.1);
}

// the gate. Empty string = pass, else the first failing reason (stable short codes, used in the report).
// `predictedGainScale`: the render was made at a different preset gain; levels are scaled by this factor first.
inline juce::String gate (const Analysis& a, int category, float gainScale = 1.0f, bool checkLoudness = true)
{
    const float g = gainScale, gDb = juce::Decibels::gainToDecibels (g);
    if (! a.finite) return "nan";
    if (a.rms * g < 1.0e-4f || a.loud + gDb < -60.0f) return "silent";
    if (a.peak * g >= peakCeiling) return "clip";
    if (a.dc > 0.02f * a.rms + 1.0e-4f / g) return "dc";
    if (a.firstSample * g > 0.01f || a.onsetJump > 1.5f * a.laterJump + 0.02f / g) return "click-start";
    if (a.offJump > 1.5f * a.preOffJump + 0.01f / g) return "click-end";
    if (a.monoLossDb > 3.0f) return "mono-collapse";
    if (a.lowSideDb > -30.0f) return "low-side";
    if (a.tailSeconds > tailLimitSeconds (category)) return "long-tail";
    if (checkLoudness && std::abs (a.loud + gDb - targetLoudness (category)) > loudnessTolerance) return "loudness";
    return {};
}

// real brands / products / companies that must never appear in a preset name or part label (word match, case-insensitive)
inline const juce::StringArray& bannedWords()
{
    static const juce::StringArray b { "roland", "korg", "yamaha", "moog", "minimoog", "juno", "jupiter", "prophet", "oberheim", "nord", "casio", "akai",
                                       "mpc", "rhodes", "wurli", "wurlitzer", "hammond", "leslie", "mellotron", "fender", "gibson", "marshall", "vox",
                                       "steinway", "bosendorfer", "clavinet", "serum", "omnisphere", "nexus", "kontakt", "spitfire", "massive", "sylenth",
                                       "vital", "arturia", "native", "nintendo", "gameboy", "atari", "sega", "kalimba", "stratocaster", "telecaster",
                                       "les paul", "dx7", "tr-808", "tr808", "909", "linn", "emu", "e-mu", "ensoniq", "waldorf", "sequential", "access",
                                       "virus", "novation", "behringer", "elektron", "teenage", "op-1", "ableton", "fl studio", "logic", "cubase",
                                       "splice", "apple", "google", "sony", "playstation", "xbox", "tesla", "gucci", "prada", "rolex", "nike", "adidas",
                                       "trapwin", "evolve", "keys killa" };
    return b;
}
inline bool isCleanName (const juce::String& s)
{
    const auto l = " " + s.toLowerCase().replaceCharacters ("-_.,&", "     ") + " ";
    for (auto& word : bannedWords())
    {
        const auto w = word.replaceCharacters ("-_.,&", "     ");
        if (l.contains (" " + w + " ") || l.contains (" " + w + "s ")) return false;
    }
    return true;
}

// render one test note of a preset at its own gain
inline juce::AudioBuffer<float> renderNote (const VaultPreset& p, int note, double& noteOff)
{
    const double secs = renderSeconds (p);
    noteOff = secs * 0.7;
    return VaultEngine::render (p, note, velocity, secs, sampleRate);
}
// render one test note at the preset's own gain and analyse it
inline Analysis renderAndAnalyse (const VaultPreset& p, int note)
{
    double off = 0;
    const auto b = renderNote (p, note, off);
    return analyse (b, sampleRate, off);
}
} // namespace vaultqc
