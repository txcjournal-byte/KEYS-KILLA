#pragma once
// CHROME VAULT v2 recipe quality gate (VaultBankGen -recipe <name>|all). Header-only, included by BankGen.cpp.
// v3: presets are rendered with their whole sound - FX chain, modulation, sound macros at their values, arp / gate when
// `on` (recipe::design, Recipe.h) - so every rule below applies to the FX as designed. The bank is written as CVB v3.
//
// Every preset of the recipe(s) is rendered at 48 kHz, velocity 0.8, at C5 (MIDI 60 = the home note, FL Studio naming),
// note off after 1.5 s and rendered on until the tail is gone, plus 2.5 s at C4 / C6 and C5 at velocity 0.35 / 1.0.
//   gate      NaN, silence, DC, clicks (note start / end / anywhere: BankQC / BankAudit detectors), clipping (vel 1.0 peak
//             under the safety limiter after normalisation), mono-sum loss > 3 dB, side energy under 120 Hz,
//             tail <= 10 s (Pads / FX & Atmos <= 16 s)
//   comfort   at C5: the sounding fundamental within 3 dB of the strongest harmonic (not Bells & Mallets / FX & Atmos),
//             2-5 kHz <= 10 % of the power, > 6 kHz <= 3 %, octave-up parts >= 12 dB (K-weighted) under part 0,
//             velocity 0.35 -> 1.0 opens >= 3 dB
//   loudness  K-weighted loudest 400 ms at C5 set to the category target (BankQC.h) minus bankHeadroomDb (6 dB of
//             headroom so chords stay under the safety limiter) by VaultPreset::gain (quieter only when the velocity-1.0
//             peak would reach 0.84 - short / muted plucks - and by at most 10 dB)
//   chords    the owner's listening phrase (C5 E5 G5 C6 + the C5 triad, velocity ~0.77) at the final gain: its peak before
//             the safety limiter (Result::chordOver, dB over the 0.89 ceiling; the assembly QA allows maxChordOver)
//   distinct  fingerprint = log-mel (24 bands) x 6 time segments at C5 + x 3 at C4 / C6 + 12 cepstral coefficients,
//             normalised to the render's own maximum, + the C5 harmonic profile (H1..H12) + amplitude-modulation depth
//             and stereo width; distance = RMS over these groups (dB). A preset closer than -dist (default 3.5) to an
//             already accepted one (recipe order) is rejected.
#include "Recipe.h"
#include "../BankQC.h"
#include "../BankAudit.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <array>
#include <atomic>
#include <functional>
#include <cstring>
#include <map>
#include <mutex>
#include <set>
#include <thread>

namespace recipegate
{
constexpr double sr = 48000.0;
constexpr double holdSeconds = 1.5;
inline float tailLimit (int category) { return category == vault::catPads || category == vault::catFxAtmos ? 16.0f : 10.0f; }
inline bool inharmonicCategory (int category) { return category == vault::catBellsMallets || category == vault::catFxAtmos; }
// the v2 bank sits this far under the BankQC category targets: one note at velocity 0.8 at -20.5 (Piano) .. -18 (Bass)
// K-weighted, so the C5 triad of the listening phrase stays under the engine's safety limiter (0.89) for nearly every preset
constexpr float bankHeadroomDb = 6.0f;
inline float bankTarget (int category) { return vaultqc::targetLoudness (category) - bankHeadroomDb; }
constexpr float maxChordOver = 2.0f;
constexpr float maxTrimDb = 22.0f;     // the loudness trim may lift a preset by at most 22 dB   // assembly QA: the phrase may touch the limiter by at most 2 dB
inline juce::AudioBuffer<float> renderPhrase (const VaultPreset& p, double seconds = 4.0);

inline juce::AudioBuffer<float> render (const VaultPreset& p, int note, float vel, double offSeconds, double totalSeconds)
{
    juce::MidiBuffer mb;
    mb.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, (int) std::round (vel * 127.0f))), 0);
    mb.addEvent (juce::MidiMessage::noteOff (1, note), (int) std::round (offSeconds * sr));
    return VaultEngine::renderMidi (p, mb, (int) std::round (totalSeconds * sr), sr);
}

// the sounding pitch of a key on part 0
inline float soundingHz (const VaultPreset& p, int key)
{
    const auto& a = p.parts.front();
    const int n = key + 12 * a.octave + a.semi;
    return 440.0f * std::pow (2.0f, (float) (n - 69) / 12.0f);
}

struct Spectrum { std::vector<float> pow; double binHz = 1; };
// power spectrum of the mono sum over [t0, t0 + 2^order / sr)
inline Spectrum spectrum (const juce::AudioBuffer<float>& b, double t0, int order = 15)
{
    const int n = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> d ((size_t) (2 * n), 0.0f);
    const int s0 = (int) (t0 * sr);
    for (int i = 0; i < n; ++i)
    {
        const int s = s0 + i;
        if (s >= b.getNumSamples()) break;
        const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (n - 1));
        d[(size_t) i] = 0.5f * (b.getSample (0, s) + b.getSample (1, s)) * w;
    }
    fft.performFrequencyOnlyForwardTransform (d.data());
    Spectrum sp;
    sp.binHz = sr / n;
    sp.pow.resize ((size_t) (n / 2));
    for (int i = 0; i < n / 2; ++i) sp.pow[(size_t) i] = d[(size_t) i] * d[(size_t) i];
    return sp;
}
inline double bandPower (const Spectrum& s, double lo, double hi)
{
    double acc = 0;
    for (int i = std::max (1, (int) (lo / s.binHz)); i < std::min ((int) s.pow.size(), (int) (hi / s.binHz)); ++i) acc += s.pow[(size_t) i];
    return acc;
}
inline float peakDb (const Spectrum& s, double lo, double hi)
{
    float m = 0;
    for (int i = std::max (1, (int) (lo / s.binHz)); i <= std::min ((int) s.pow.size() - 1, (int) (hi / s.binHz)); ++i) m = std::max (m, s.pow[(size_t) i]);
    return 10.0f * std::log10 (std::max (1.0e-20f, m));
}

// fingerprint: log-mel bands per time segment, dB under the render's own maximum, floored at -60
constexpr int melBands = 24;
inline std::vector<float> melFrames (const juce::AudioBuffer<float>& b, const std::vector<std::pair<double, double>>& segs)
{
    constexpr int order = 11, n = 1 << order;
    juce::dsp::FFT fft (order);
    auto mel = [] (double hz) { return 2595.0 * std::log10 (1.0 + hz / 700.0); };
    const double m0 = mel (40.0), m1 = mel (16000.0);
    std::vector<float> out;
    std::vector<float> d ((size_t) (2 * n));
    for (auto [t0, t1] : segs)
    {
        std::vector<double> band (melBands, 1.0e-12);
        for (int s0 = (int) (t0 * sr); s0 + n <= std::min (b.getNumSamples(), (int) (t1 * sr) + n / 2); s0 += n / 2)
        {
            std::fill (d.begin(), d.end(), 0.0f);
            for (int i = 0; i < n; ++i)
                d[(size_t) i] = 0.5f * (b.getSample (0, s0 + i) + b.getSample (1, s0 + i))
                                * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (n - 1)));
            fft.performFrequencyOnlyForwardTransform (d.data());
            for (int i = 1; i < n / 2; ++i)
            {
                const double hz = i * sr / n;
                if (hz < 40.0 || hz >= 16000.0) continue;
                const int k = juce::jlimit (0, melBands - 1, (int) ((mel (hz) - m0) / (m1 - m0) * melBands));
                band[(size_t) k] += (double) d[(size_t) i] * d[(size_t) i];
            }
        }
        for (auto v : band) out.push_back ((float) (10.0 * std::log10 (v)));
    }
    float mx = -300;
    for (auto v : out) mx = std::max (mx, v);
    for (auto& v : out) v = std::max (-60.0f, v - mx);
    return out;
}

struct Result
{
    VaultPreset preset;
    juce::String fail;            // empty = passed the gate
    float loud = -120, gainDb = 0, quietBy = 0, peak127 = 0;
    float h1Db = 0, share25 = 0, share6k = 0, centroid = 0, tail = 0, velRange = 0, octUpDb = -99, monoLoss = 0, spike = 0;
    float share8k = 0;            // > 8 kHz share of the C5 power (assembly QA)
    float chordOver = -99;        // the listening phrase's peak before the safety limiter, dB over its 0.89 ceiling (assembly QA)
    std::vector<float> print;
    float minDist = 1.0e9f; juce::String nearest;
    std::array<float, 12> harm {};
    juce::String group;           // the recipe that made it (assembly: cross-recipe distinctness)
};

inline void roundLikeCache (Result& r);
inline void analyse (Result& r)
{
    auto& p = r.preset;
    const int cat = p.category;
    p.gain = 1.0f;
    const float lim = tailLimit (cat);
    const auto c5 = render (p, vault::homeNote, 0.8f, holdSeconds, holdSeconds + lim + 1.0);
    const auto a = vaultqc::analyse (c5, sr, holdSeconds);
    if (! a.finite) { r.fail = "nan"; return; }
    r.loud = a.loud;
    if (a.rms < 1.0e-5f || a.loud < -55.0f) { r.fail = "silent"; return; }
    // tail: last 10 ms frame within 60 dB of the loudest, from the note-off
    {
        const int fr = (int) (0.01 * sr), nf = c5.getNumSamples() / fr;
        std::vector<float> e ((size_t) nf);
        float mx = -200;
        for (int f = 0; f < nf; ++f)
        {
            double acc = 0;
            for (int i = f * fr; i < (f + 1) * fr; ++i) acc += 0.5 * ((double) c5.getSample (0, i) * c5.getSample (0, i) + (double) c5.getSample (1, i) * c5.getSample (1, i));
            e[(size_t) f] = vaultaudit::db (acc / fr); mx = std::max (mx, e[(size_t) f]);
        }
        int last = 0;
        for (int f = 0; f < nf; ++f) if (e[(size_t) f] >= mx - 60.0f) last = f;
        r.tail = (float) ((last + 1) * 0.01 - holdSeconds);
        if (last >= nf - 2) r.tail = lim + 1.0f;
    }
    juce::AudioBuffer<float> head (2, (int) (2.5 * sr));
    for (int c = 0; c < 2; ++c) head.copyFrom (c, 0, c5, c, 0, head.getNumSamples());
    float sideTot = -120, sideMid = -120, midTot = -120;
    vaultaudit::lowBand (head, sideTot, sideMid, midTot);
    r.monoLoss = a.monoLossDb;
    r.spike = vaultaudit::worstSpike (head);
    double sl = 0, sr2 = 0;
    for (int i = 0; i < head.getNumSamples(); ++i) { sl += head.getSample (0, i); sr2 += head.getSample (1, i); }
    const float dcAbs = (float) std::max (std::abs (sl / head.getNumSamples()), std::abs (sr2 / head.getNumSamples()));

    // comfort spectrum (0.05 .. 0.73 s)
    const auto sp = spectrum (c5, 0.05);
    {
        const double tot = bandPower (sp, 20, 20000);
        r.share25 = (float) (bandPower (sp, 2000, 5000) / std::max (1e-20, tot));
        r.share6k = (float) (bandPower (sp, 6000, 20000) / std::max (1e-20, tot));
        r.share8k = (float) (bandPower (sp, 8000, 20000) / std::max (1e-20, tot));
        double num = 0;
        for (int i = 1; i < (int) sp.pow.size(); ++i) num += i * sp.binHz * sp.pow[(size_t) i];
        r.centroid = (float) (num / std::max (1e-20, tot));
        const float f0 = soundingHz (p, vault::homeNote);
        auto h1Under = [f0] (const Spectrum& x)
        {
            const float h = peakDb (x, f0 * 0.97, f0 * 1.03);
            float top = h;
            for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) top = std::max (top, peakDb (x, k * f0 * 0.985, k * f0 * 1.015));
            return h - top;
        };
        const float h1 = peakDb (sp, f0 * 0.97, f0 * 1.03);
        float strongest = h1;
        for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
        // the fundamental in the best of three windows (0.05 / 0.4 / 0.75 s): layers a few cents apart beat, and one
        // 0.7 s window can catch the fundamental in a beat's trough (the library tuning pass moved such layers by a
        // few cents and flipped this measure on held brass / piano / tutti layers); a missing fundamental (an
        // octave-up layer on top, a thin wave) is missing in all three
        r.h1Db = std::max ({ h1 - strongest, h1Under (spectrum (c5, 0.4)), h1Under (spectrum (c5, 0.75)) });
        for (int k = 1; k <= 12; ++k)   // harmonic profile (fingerprint): saw vs square, pickup / pluck position, octave courses
            r.harm[(size_t) (k - 1)] = k * f0 < 16000.0f ? std::max (-50.0f, peakDb (sp, k * f0 * (k == 1 ? 0.97 : 0.985), k * f0 * (k == 1 ? 1.03 : 1.015)) - strongest) : -50.0f;
    }
    // velocity
    const auto v1 = render (p, vault::homeNote, 1.0f, holdSeconds, 2.0);
    const auto vl = render (p, vault::homeNote, 0.35f, holdSeconds, 2.0);
    r.peak127 = v1.getMagnitude (0, v1.getNumSamples());
    r.velRange = vaultqc::analyse (v1, sr, holdSeconds).loud - vaultqc::analyse (vl, sr, holdSeconds).loud;
    // octave-up parts vs part 0
    for (size_t k = 1; k < p.parts.size(); ++k)
    {
        const int t0 = 12 * p.parts[0].octave + p.parts[0].semi, tk = 12 * p.parts[k].octave + p.parts[k].semi;
        if (tk < t0 + 12) continue;
        VaultPreset solo = p, main = p;
        solo.parts = { p.parts[k] };
        solo.parts[0].level = p.parts[k].level * juce::Decibels::decibelsToGain (p.parts[k].get ("gain", 0.0f) - p.parts[0].get ("gain", 0.0f));
        main.parts = { p.parts[0] };
        const float ls = vaultqc::analyse (render (solo, vault::homeNote, 0.8f, holdSeconds, 2.0), sr, holdSeconds).loud;
        const float lm = vaultqc::analyse (render (main, vault::homeNote, 0.8f, holdSeconds, 2.0), sr, holdSeconds).loud;
        r.octUpDb = std::max (r.octUpDb, ls - lm);
    }
    // loudness normalisation
    const float target = bankTarget (cat);
    float g = target - r.loud;
    const float peakAt = juce::Decibels::gainToDecibels (std::max (1.0e-6f, r.peak127)) + g;
    if (peakAt > juce::Decibels::gainToDecibels (0.84f)) { r.quietBy = peakAt - juce::Decibels::gainToDecibels (0.84f); g -= r.quietBy; }
    r.gainDb = g;
    r.preset.gain = juce::Decibels::decibelsToGain (g);
    const float lin = r.preset.gain;
    {   // chord headroom at the final gain (18 dB down so the limiter never acts, the peak scaled back)
        auto q = p;
        q.gain = lin * 0.125f;
        const auto ph = renderPhrase (q);
        r.chordOver = juce::Decibels::gainToDecibels (8.0f * ph.getMagnitude (0, ph.getNumSamples()) / 0.89f);
    }

    // fingerprint
    {
        const auto c4 = render (p, vault::homeNote - 12, 0.8f, holdSeconds, 2.0);
        const auto c6 = render (p, vault::homeNote + 12, 0.8f, holdSeconds, 2.0);
        r.print = melFrames (c5, { { 0.0, 0.05 }, { 0.05, 0.15 }, { 0.15, 0.4 }, { 0.4, 0.8 }, { 0.8, 1.4 }, { 1.5, 2.1 } });
        const auto mid = melFrames (c5, { { 0.05, 1.4 } });
        for (int q = 1; q <= 12; ++q)   // cepstrum of the held log-mel spectrum (scaled to dB-like units)
        {
            double c = 0;
            for (int m = 0; m < melBands; ++m) c += mid[(size_t) m] * std::cos (juce::MathConstants<double>::pi * q * (m + 0.5) / melBands);
            r.print.push_back ((float) (c * 2.0 / melBands) * 2.0f);
        }
        for (auto* b : { &c4, &c6 })
            for (auto v : melFrames (*b, { { 0.0, 0.1 }, { 0.1, 0.5 }, { 0.5, 1.4 } })) r.print.push_back (v);
        for (auto h : r.harm) r.print.push_back (h);
        // motion + width: amplitude-modulation depth (beating, chorus, tremolo, wobble) and side / mid of the held note
        {
            const int fr = (int) (0.01 * sr), f0i = 20, f1i = 140;
            std::vector<float> e;
            double side = 0, mid = 0;
            for (int f = f0i; f < f1i; ++f)
            {
                double acc = 0;
                for (int i = f * fr; i < (f + 1) * fr; ++i)
                {
                    const double l = c5.getSample (0, i), rr = c5.getSample (1, i);
                    acc += l * l + rr * rr; side += (l - rr) * (l - rr); mid += (l + rr) * (l + rr);
                }
                e.push_back (vaultaudit::db (acc / fr + 1.0e-12));
            }
            double am = 0;
            for (int f = 7; f + 7 < (int) e.size(); ++f)
            {
                double m = 0;
                for (int g = f - 7; g <= f + 7; ++g) m += e[(size_t) g];
                am += std::pow (e[(size_t) f] - m / 15.0, 2.0);
            }
            am = std::sqrt (am / std::max (1, (int) e.size() - 14));
            r.print.push_back ((float) std::min (10.0, am) * 3.0f);
            r.print.push_back (juce::jlimit (-40.0f, 0.0f, vaultaudit::db (side / std::max (1.0e-12, mid))) * 0.5f);
        }
    }

    // the gate (levels at the final gain)
    if (a.peak * lin >= 0.885f || r.peak127 * lin >= 0.885f) r.fail = "clip";
    else if (dcAbs > 0.002f && dcAbs > 0.02f * a.rms) r.fail = "dc";
    else if (a.firstSample * lin > 0.01f || a.onsetJump > 1.5f * a.laterJump + 0.02f / lin) r.fail = "click-start";
    else if (a.offJump > 1.5f * a.preOffJump + 0.01f / lin) r.fail = "click-end";
    else if (r.spike > 8.0f) r.fail = "click-spike";
    else if (a.monoLossDb > 3.0f) r.fail = "mono-loss";
    else if (sideTot > -30.0f || (midTot > -30.0f && sideMid > -20.0f)) r.fail = "low-side";
    else if (r.tail > lim) r.fail = "tail";
    else if (! inharmonicCategory (cat) && r.h1Db < -3.0f) r.fail = "fundamental";
    else if (r.share25 > 0.10f) r.fail = "harsh-2-5k";
    else if (r.share6k > 0.03f) r.fail = "piercing-highs";
    else if (r.octUpDb > -12.0f) r.fail = "octave-layer";
    else if (r.velRange < 3.0f) r.fail = "velocity";
    else if (r.quietBy > 10.0f) r.fail = "too-peaky";
    else if (auto np = vaultaudit::nameProblem (p.name); np.isNotEmpty()) r.fail = "name: " + np;
    else if (p.name.containsIgnoreCase ("808")) r.fail = "name: 808";
    else
        for (auto& part : p.parts)
            if (part.label.containsIgnoreCase ("808") || ! vaultqc::isCleanName (part.label)) r.fail = "label";
    roundLikeCache (r);   // a fresh analysis gives exactly what the cache gives back: same bank from a cold or a warm cache
}

// fingerprint layout: C5 log-mel 6 x 24 | 12 cepstral | C4 / C6 log-mel 2 x 3 x 24 | 12 harmonics | AM depth, width.
// Distance = RMS over the five groups of each group's RMS dB difference (every group weighs the same, so a different
// harmonic structure or motion counts as much as a different spectral envelope).
inline float distance (const std::vector<float>& a, const std::vector<float>& b)
{
    static const int edges[] { 0, 144, 156, 300, 312, 314 };
    if (a.size() != b.size() || a.size() < 314) return 0.0f;
    double tot = 0;
    for (int g = 0; g < 5; ++g)
    {
        double s = 0;
        for (int i = edges[g]; i < edges[g + 1]; ++i) { const double d = a[(size_t) i] - b[(size_t) i]; s += d * d; }
        tot += s / (edges[g + 1] - edges[g]);
    }
    return (float) std::sqrt (tot / 5.0);
}

inline void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
}

// ---- assembly (VaultBankGen -recipe all): caches, cross-recipe distinctness, the assembly QA, listening renders ----------
constexpr int gateVersion = 4;   // bump when analyse() changes (invalidates the analysis cache)
constexpr int peakControlVersion = 2;
constexpr int macroCheckVersion = 1;   // bump when the assembly macro check / narrowing changes (invalidates macros.tsv)   // bump when the engine's peak control changes (invalidates only presets that use it)

// the recipe that owns a cross-recipe near-duplicate pair keeps it in this order: real instruments first, generic synth last
inline int recipePriority (const juce::String& g)
{
    static const juce::StringArray order { "keys", "strings", "orch", "winds", "perc-voc", "synth" };
    const int i = order.indexOf (g);
    return i < 0 ? order.size() : i;
}

// assembly QA ceilings at C5 (velocity 0.8): spectral centroid per category (Hz) and the > 8 kHz share of the power
inline float centroidCeiling (int c)
{
    static const float t[vault::numCategories] { 1300, 1400, 1300, 700, 1400, 1500, 1500, 1500, 2200, 1500, 1500, 1600, 1400, 1600, 1600, 2200 };
    return t[juce::jlimit (0, (int) vault::numCategories - 1, c)];
}
constexpr float maxShare8k = 0.01f;

inline uint64_t presetHash (const VaultPreset& p)
{
    VaultPreset q = p;
    q.gain = 1.0f;
    const auto mb = vault::serializeBank ({ q }, false);
    uint64_t h = 1469598103934665603ull ^ (uint64_t) gateVersion;
    if (! p.parts.empty() && p.parts.front().get (vault::peakKey, 0.0f) < -0.01f) h ^= (uint64_t) peakControlVersion << 32;
    const auto* d = (const uint8_t*) mb.getData();
    for (size_t i = 0; i < mb.getSize(); ++i) { h ^= d[i]; h *= 1099511628211ull; }
    return h;
}

// analysis cache: one line per analysed preset: hash \t fail \t numbers (scalars, harmonics, fingerprint)
inline juce::String cacheLine (uint64_t h, const Result& r)
{
    juce::String s;
    s << juce::String::toHexString ((juce::int64) h) << "\t" << r.fail << "\t";
    for (float v : { r.loud, r.gainDb, r.quietBy, r.peak127, r.h1Db, r.share25, r.share6k, r.share8k, r.centroid, r.tail, r.velRange, r.octUpDb, r.monoLoss, r.spike, r.chordOver })
        s << juce::String (v, 6) << " ";
    for (auto v : r.harm) s << juce::String (v, 4) << " ";
    s << "\t";
    for (auto v : r.print) s << juce::String (v, 4) << " ";
    return s;
}
inline bool fromCacheLine (const juce::String& line, Result& r)
{
    auto cols = juce::StringArray::fromTokens (line, "\t", "");
    if (cols.size() < 4) return false;
    r.fail = cols[1];
    auto sc = juce::StringArray::fromTokens (cols[2], " ", ""); sc.removeEmptyStrings();
    constexpr int ns = 15;
    if (sc.size() != ns + 12) return false;
    float* f[] { &r.loud, &r.gainDb, &r.quietBy, &r.peak127, &r.h1Db, &r.share25, &r.share6k, &r.share8k, &r.centroid, &r.tail, &r.velRange, &r.octUpDb, &r.monoLoss, &r.spike, &r.chordOver };
    for (int i = 0; i < ns; ++i) *f[i] = sc[i].getFloatValue();
    for (int i = 0; i < 12; ++i) r.harm[(size_t) i] = sc[ns + i].getFloatValue();
    auto pr = juce::StringArray::fromTokens (cols[3], " ", ""); pr.removeEmptyStrings();
    r.print.clear();
    for (auto& v : pr) r.print.push_back (v.getFloatValue());
    r.preset.gain = juce::Decibels::decibelsToGain (r.gainDb);
    return true;
}

// the owner's listening phrase at the home note: C5 E5 G5 C6 (0.4 s each), then the C5 major triad held, 4 s in all
inline juce::AudioBuffer<float> renderPhrase (const VaultPreset& p, double seconds)
{
    juce::MidiBuffer mb;
    auto at = [] (double t) { return (int) std::round (t * sr); };
    const int mel[] { 60, 64, 67, 72 };
    for (int i = 0; i < 4; ++i)
    {
        mb.addEvent (juce::MidiMessage::noteOn (1, mel[i], (juce::uint8) 100), at (0.42 * i));
        mb.addEvent (juce::MidiMessage::noteOff (1, mel[i]), at (0.42 * i + 0.38));
    }
    for (int n : { 60, 64, 67 })
    {
        mb.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 96), at (1.8));
        mb.addEvent (juce::MidiMessage::noteOff (1, n), at (3.3));
    }
    auto b = VaultEngine::renderMidi (p, mb, at (seconds), sr);
    const int fade = at (0.35);
    for (int c = 0; c < b.getNumChannels(); ++c) b.applyGainRamp (c, b.getNumSamples() - fade, fade, 1.0f, 0.0f);
    return b;
}
inline bool writeOgg (const juce::File& f, const juce::AudioBuffer<float>& b, int quality)
{
   #if JUCE_USE_OGGVORBIS
    f.deleteFile();
    juce::OggVorbisAudioFormat ogg;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (ogg.createWriterFor (os.get(), sr, 2, 16, {}, quality)))
        {
            os.release();
            return w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
   #else
    juce::ignoreUnused (f, b, quality);
   #endif
    return false;
}

// listening clip for a sound macro: the knob swept from 0 to 1 over the clip (the other macros at their defaults) while
// C5 E5 G5 C6 G5 E5 ... plays (0.6 s notes every 0.7 s, velocity ~0.77)
inline juce::AudioBuffer<float> renderMacroSweep (const VaultPreset& p, int macroIndex, double seconds = 6.0)
{
    const auto compiled = VaultEngine::compile (p);
    auto eng = std::make_unique<VaultEngine>();
    for (int m = 0; m < vault::numMacros; ++m) eng->setMacro (m, m == macroIndex ? 0.0f : compiled.macroDefaults[m]);
    eng->setArpOn (compiled.ctl.arp.on);
    eng->setGateOn (compiled.ctl.gate.on);
    constexpr int block = 256;
    eng->prepare (sr, block, std::max (1, compiled.numParts), false);
    eng->setPreset (compiled, true);
    for (int k = 0; k < compiled.numParts; ++k)
        if (auto* h = compiled.parts[(size_t) k].instrument; h != nullptr && compiled.library != nullptr)
            compiled.library->loadNow (h);
    eng->serviceResources();
    juce::MidiBuffer mb;
    auto at = [] (double t) { return (int) std::round (t * sr); };
    const int mel[] { 60, 64, 67, 72, 67, 64 };
    const double sweepEnd = seconds - 0.6;
    int k = 0;
    for (double t = 0.0; t + 0.6 < sweepEnd; t += 0.7, ++k)
    {
        mb.addEvent (juce::MidiMessage::noteOn (1, mel[k % 6], (juce::uint8) 98), at (t));
        mb.addEvent (juce::MidiMessage::noteOff (1, mel[k % 6]), at (t + 0.6));
    }
    const int n = at (seconds);
    juce::AudioBuffer<float> out (2, n), buf (2, block);
    out.clear();
    for (int s0 = 0; s0 < n; s0 += block)
    {
        const int len = std::min (block, n - s0);
        eng->setMacro (macroIndex, (float) juce::jlimit (0.0, 1.0, (s0 / sr) / std::max (0.1, sweepEnd - 0.5)));
        juce::MidiBuffer slice;
        for (const auto meta : mb)
            if (meta.samplePosition >= s0 && meta.samplePosition < s0 + len) slice.addEvent (meta.getMessage(), meta.samplePosition - s0);
        buf.setSize (2, len, false, false, true);
        eng->process (buf, slice);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, s0, buf, c, 0, len);
    }
    const int fade = at (0.35);
    for (int c = 0; c < 2; ++c) out.applyGainRamp (c, n - fade, fade, 1.0f, 0.0f);
    return out;
}

inline juce::String statLine (std::vector<float> v, int decimals, float scale = 1.0f)
{
    if (v.empty()) return "-";
    std::sort (v.begin(), v.end());
    auto f = [&] (float x) { return decimals > 0 ? juce::String (x * scale, decimals) : juce::String (juce::roundToInt (x * scale)); };
    return f (v.front()) + " / " + f (v[v.size() / 2]) + " / " + f (v.back());
}

inline void roundLikeCache (Result& r)
{
    const auto fail = r.fail;
    fromCacheLine (cacheLine (0, r), r);
    r.fail = fail;
}

// ---- assembly macro check (VaultBankGen -recipe all -macros): every sound macro of every accepted preset at 0 and at 1 --
// rendered at C5 at the preset's final gain with the whole v3 sound (FX chain on, modulation, arp / gate as designed); an
// extreme fails when it breaks a bank rule that the designed sound (macros at their defaults) keeps: clipping at velocity
// 1.0, 2-5 kHz > 10 %, > 6 kHz > 3 %, > 8 kHz > 1 % (assembly QA), the fundamental > 3 dB under the strongest harmonic (not
// Bells & Mallets / FX & Atmos), NaN / silence, > 4 dB louder than the designed sound, or a tail longer than the gate's
// category limit (10 s, Pads / FX & Atmos 16 s; the same 60 dB rule as analyse(), checked on a full-length render only
// when the short probe still rings at its end). The renders already carry the preset gain (the engine applies it).
struct MacroProbe
{
    juce::AudioBuffer<float> c5;
    float loud = -120, s25 = 0, s6 = 0, s8 = 0, h1 = 0, peak127 = 0, tailEnd = -200, tailMax = -200;
    bool finite = true;
};
inline MacroProbe macroProbe (const VaultPreset& p, const float* mv, double total)
{
    MacroProbe r;
    juce::MidiBuffer mb;
    mb.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, (juce::uint8) 102), 0);
    mb.addEvent (juce::MidiMessage::noteOff (1, vault::homeNote), (int) (holdSeconds * sr));
    r.c5 = VaultEngine::renderMidi (p, mb, (int) (total * sr), sr, mv);
    const auto a = vaultqc::analyse (r.c5, sr, holdSeconds);
    r.finite = a.finite;
    if (! r.finite) return r;
    r.loud = a.loud;
    const auto sp = spectrum (r.c5, 0.05);
    const double tot = std::max (1e-20, bandPower (sp, 20, 20000));
    r.s25 = (float) (bandPower (sp, 2000, 5000) / tot);
    r.s6 = (float) (bandPower (sp, 6000, 20000) / tot);
    r.s8 = (float) (bandPower (sp, 8000, 20000) / tot);
    const float f0 = soundingHz (p, vault::homeNote);
    const float h1 = peakDb (sp, f0 * 0.97, f0 * 1.03);
    float strongest = h1;
    for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
    r.h1 = h1 - strongest;
    const int fr = (int) (0.01 * sr), nf = r.c5.getNumSamples() / fr;
    for (int fi = 0; fi < nf; ++fi)
    {
        double acc = 0;
        for (int i = fi * fr; i < (fi + 1) * fr; ++i) acc += 0.5 * ((double) r.c5.getSample (0, i) * r.c5.getSample (0, i) + (double) r.c5.getSample (1, i) * r.c5.getSample (1, i));
        const float e = vaultaudit::db (acc / fr + 1e-20);
        r.tailMax = std::max (r.tailMax, e);
        if (fi >= nf - 10) r.tailEnd = std::max (r.tailEnd, e);
    }
    juce::MidiBuffer hb;
    hb.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, (juce::uint8) 127), 0);
    hb.addEvent (juce::MidiMessage::noteOff (1, vault::homeNote), (int) (0.7 * sr));
    const auto hard = VaultEngine::renderMidi (p, hb, (int) (0.8 * sr), sr, mv);
    r.peak127 = std::max (hard.getMagnitude (0, hard.getNumSamples()), r.c5.getMagnitude (0, r.c5.getNumSamples()));
    return r;
}
inline juce::String macroJudge (const MacroProbe& x, const MacroProbe& ref, float lin, bool longTail, bool inharmonic)
{
    if (! x.finite) return "nan";
    if (x.loud < -70.0f && ref.loud > -50.0f) return "silent";
    if (x.peak127 * lin >= 0.885f && x.peak127 > ref.peak127 * 1.06f) return "clip";   // lin: 1 for renders at the final gain
    if (x.s25 > 0.10f && x.s25 > ref.s25 + 0.005f) return "harsh-2-5k";
    if (x.s6 > 0.03f && x.s6 > ref.s6 + 0.002f) return "piercing-6k";
    if (x.s8 > maxShare8k && x.s8 > ref.s8 + 0.001f) return "air-8k";
    if (! inharmonic && x.h1 < -3.0f && x.h1 < ref.h1 - 0.5f) return "fundamental";
    if (x.loud > ref.loud + 4.0f) return "louder";
    if (longTail && x.tailEnd > x.tailMax - 55.0f && x.tailEnd > ref.tailEnd + 3.0f) return "tail";   // candidate: see gateTail()
    return {};
}
// the gate's tail length (analyse(): last 10 ms frame within 60 dB of the loudest, from the note-off) at velocity 0.8
inline float gateTail (const VaultPreset& p, const float* mv)
{
    const float lim = tailLimit (p.category);
    juce::MidiBuffer mb;
    mb.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, (juce::uint8) 102), 0);
    mb.addEvent (juce::MidiMessage::noteOff (1, vault::homeNote), (int) (holdSeconds * sr));
    const auto c5 = VaultEngine::renderMidi (p, mb, (int) ((holdSeconds + lim + 1.0) * sr), sr, mv);
    const int fr = (int) (0.01 * sr), nf = c5.getNumSamples() / fr;
    std::vector<float> e ((size_t) nf);
    float mx = -200;
    for (int f = 0; f < nf; ++f)
    {
        double acc = 0;
        for (int i = f * fr; i < (f + 1) * fr; ++i) acc += 0.5 * ((double) c5.getSample (0, i) * c5.getSample (0, i) + (double) c5.getSample (1, i) * c5.getSample (1, i));
        e[(size_t) f] = vaultaudit::db (acc / fr); mx = std::max (mx, e[(size_t) f]);
    }
    int last = 0;
    for (int f = 0; f < nf; ++f) if (e[(size_t) f] >= mx - 60.0f) last = f;
    return last >= nf - 2 ? lim + 1.0f : (float) ((last + 1) * 0.01 - holdSeconds);
}
inline float macroExtremeDistance (const MacroProbe& a, const MacroProbe& b)
{
    const std::vector<std::pair<double, double>> segs { { 0.0, 0.15 }, { 0.15, 0.8 }, { 0.8, 1.4 }, { 1.5, 2.5 } };
    const auto x = melFrames (a.c5, segs), y = melFrames (b.c5, segs);
    double s = 0;
    for (size_t i = 0; i < x.size() && i < y.size(); ++i) s += (x[i] - y[i]) * (x[i] - y[i]);
    return (float) std::sqrt (s / std::max<size_t> (1, x.size()));
}
struct MacroCheck { int checked = 0, narrowed = 0, curveRefit = 0; juce::StringArray fails, fixed; int quiet = 0; std::map<juce::String, int> why; };

// narrow sound macro i to the knob positions [a, b] = [d - d * kl, d + (1 - d) * kh] around its default d (kl / kh = 1 keeps a
// side, 0 takes it away) and re-map the knob to 0..1: every target's min / max become its old values at a and b, the default
// moves so the designed sound is exactly the same (linear targets; a curved target gets the curve that passes through its
// designed value again, found by bisection - returns false when no curve in -1..1 does).
inline bool narrowMacro (VaultPreset& p, int i, float kl, float kh)
{
    const float d = p.macroDefaults[i];
    const float a = d - d * kl, b = d + (1.0f - d) * kh;
    const float nd = b - a > 1.0e-6f ? (d - a) / (b - a) : d;
    bool exact = true;
    auto curveAt = [] (float m, float c)
    {
        m = juce::jlimit (0.0f, 1.0f, m);
        if (c > 0.001f) return std::pow (m, 1.0f + 3.0f * c);
        if (c < -0.001f) return 1.0f - std::pow (1.0f - m, 1.0f - 3.0f * c);
        return m;
    };
    for (auto& t : p.macros[(size_t) i].targets)
    {
        auto g = [&t, &curveAt] (float x) { return t.min + (t.max - t.min) * curveAt (x, t.curve); };
        const float v = g (d), lo = g (a), hi = g (b);
        if (std::abs (t.curve) > 0.001f && std::abs (hi - lo) > 1.0e-9f)
        {
            const float want = (v - lo) / (hi - lo);   // curveAt (nd, c) must give this; it falls as c rises
            float c0 = -1.0f, c1 = 1.0f;
            if (want > curveAt (nd, c0) || want < curveAt (nd, c1)) exact = false;
            for (int it = 0; it < 60; ++it) { const float c = 0.5f * (c0 + c1); (curveAt (nd, c) > want ? c0 : c1) = c; }
            t.curve = 0.5f * (c0 + c1);
        }
        t.min = lo; t.max = hi;
    }
    p.macroDefaults[i] = nd;
    return exact;
}

// the assembly macro check; with narrow = true a failing side is narrowed (rounds of x0.55, the 4th takes the side away)
// until both extremes pass - the sound at the knob's default does not change
inline MacroCheck checkPresetMacros (VaultPreset& p, bool narrow = false)
{
    MacroCheck mc;
    bool longTail = false;
    for (auto& m : p.macros) for (auto& t : m.targets) if (t.target.contains ("reverb") || t.target.contains ("delay") || t.target.contains ("release")) longTail = true;
    const double total = longTail ? 8.0 : 3.5;
    const auto ref = macroProbe (p, p.macroDefaults, total);
    for (int i = 0; i < 4; ++i)
    {
        const auto& mac = p.macros[(size_t) i];
        if (mac.targets.empty()) continue;
        ++mc.checked;
        MacroProbe lo, hi;
        juce::String badLo, badHi, first;
        for (int round = 0; round < 5; ++round)
        {
            float mv[4];
            std::copy (std::begin (p.macroDefaults), std::end (p.macroDefaults), mv);
            mv[i] = 0.0f; lo = macroProbe (p, mv, total);
            mv[i] = 1.0f; hi = macroProbe (p, mv, total);
            auto judge = [&] (const MacroProbe& x, float v)
            {
                auto bad = macroJudge (x, ref, 1.0f, longTail, inharmonicCategory (p.category));
                if (bad == "tail")   // the short probe still rings: measure the whole tail like the gate does
                {
                    mv[i] = v;
                    const float t = gateTail (p, mv);
                    bad = t > tailLimit (p.category) ? juce::String ("tail") : juce::String();
                }
                return bad;
            };
            badLo = judge (lo, 0.0f);
            badHi = judge (hi, 1.0f);
            if (badLo.isEmpty() && badHi.isEmpty()) break;
            if (round == 0) first = (badLo.isNotEmpty() ? "0:" + badLo : juce::String()) + (badHi.isNotEmpty() ? (badLo.isNotEmpty() ? " " : "") + juce::String ("1:") + badHi : juce::String());
            if (! narrow || round == 4) break;
            const float k = round == 3 ? 0.0f : 0.55f;
            if (! narrowMacro (p, i, badLo.isNotEmpty() ? k : 1.0f, badHi.isNotEmpty() ? k : 1.0f)) ++mc.curveRefit;
            ++mc.narrowed;
        }
        if (first.isNotEmpty() && badLo.isEmpty() && badHi.isEmpty())
            mc.fixed.add (p.id + "\t" + p.name + "\t" + juce::String (i + 1) + "\t" + mac.name + "\t" + first + "\tnarrowed");
        if (badLo.isNotEmpty() || badHi.isNotEmpty())
        {
            mc.fails.add (p.id + "\t" + p.name + "\t" + juce::String (i + 1) + "\t" + mac.name + "\t" + (badLo.isNotEmpty() ? "0:" + badLo : juce::String())
                          + (badHi.isNotEmpty() ? (badLo.isNotEmpty() ? " " : "") + juce::String ("1:") + badHi : juce::String())
                          + juce::String::formatted ("\tref 2-5k %.1f%% >6k %.2f%% >8k %.2f%% H1 %.1f peak %.3f | lo %.1f%% %.2f%% %.2f%% %.1f %.3f | hi %.1f%% %.2f%% %.2f%% %.1f %.3f",
                                                     100 * ref.s25, 100 * ref.s6, 100 * ref.s8, ref.h1, ref.peak127, 100 * lo.s25, 100 * lo.s6, 100 * lo.s8, lo.h1, lo.peak127,
                                                     100 * hi.s25, 100 * hi.s6, 100 * hi.s8, hi.h1, hi.peak127));
        }
        for (auto& w : juce::StringArray::fromTokens (first, " ", "")) ++mc.why[w.fromFirstOccurrenceOf (":", false, false)];
        if (lo.finite && hi.finite && macroExtremeDistance (lo, hi) < 1.0f && std::abs (lo.loud - hi.loud) < 1.0f) ++mc.quiet;
    }
    return mc;
}

// VaultBankGen -recipe <name>|all [-out bank.cvb] [-report report.md] [-threads N] [-dist 3.5] [-wdist 3.5] [-qa] [-spread 1.5]
//              [-cache dir] [-rebuild name,name] [-demo <dir> <id or name,...>] [-listen <dir> <id or name|...>] [-prints f.tsv]
//              [-macros [fails.tsv]] [-chords]
//   -dist     distinctness threshold; with "all" it applies across recipes (each recipe already ran its own pass), and
//             -wdist (default: the same as -dist) inside one recipe (0 = trust the recipe's own pass)
//   -qa       the assembly QA on top of the gate: loudness within -spread dB of the category target, > 8 kHz share,
//             spectral-centroid ceilings per category
//   -cache    keep each recipe's output (<dir>/<recipe>.cvb, rebuilt only when named in -rebuild) and every analysis
//             (<dir>/analysis.tsv, keyed by a hash of the preset) so assembly re-runs only measure what changed; with
//             -macros also the macro check (<dir>/macros.tsv + macros.cvb, keyed by the preset incl. its final gain)
//   -macros [fails.tsv]  every sound macro of every accepted preset at 0 and 1 (checkPresetMacros); a failing side is
//             narrowed (the default sound stays the same); the file lists the fixed and the still-failing extremes
//   -chords [-chordsout f.tsv]  the listening-phrase headroom per category
inline int runRecipes (int argc, char** argv)
{
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const juce::String which = argv[2];
    juce::String outPath, reportPath, demoDir, demoIds, printsPath, cacheDir, listenDir, listenIds, chordsPath;
    juce::StringArray rebuild;
    int threads = (int) std::max (1u, std::thread::hardware_concurrency());
    float minDist = 3.5f, withinDist = -1.0f, spread = 1.5f;
    bool qa = false, chords = false, macros = false;
    juce::String macrosPath;
    for (int i = 3; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a == "-out" && i + 1 < argc) outPath = argv[++i];
        else if (a == "-report" && i + 1 < argc) reportPath = argv[++i];
        else if (a == "-threads" && i + 1 < argc) threads = juce::jmax (1, juce::String (argv[++i]).getIntValue());
        else if (a == "-dist" && i + 1 < argc) minDist = (float) juce::String (argv[++i]).getDoubleValue();
        else if (a == "-wdist" && i + 1 < argc) withinDist = (float) juce::String (argv[++i]).getDoubleValue();
        else if (a == "-spread" && i + 1 < argc) spread = (float) juce::String (argv[++i]).getDoubleValue();
        else if (a == "-qa") qa = true;
        else if (a == "-cache" && i + 1 < argc) cacheDir = argv[++i];
        else if (a == "-rebuild" && i + 1 < argc) rebuild = juce::StringArray::fromTokens (argv[++i], ",", "");
        else if (a == "-demo" && i + 2 < argc) { demoDir = argv[++i]; demoIds = argv[++i]; }
        else if (a == "-listen" && i + 2 < argc) { listenDir = argv[++i]; listenIds = argv[++i]; }
        else if (a == "-prints" && i + 1 < argc) printsPath = argv[++i];   // TSV: id, name, result, fingerprint (threshold studies)
        else if (a == "-chords") chords = true;
        else if (a == "-chordsout" && i + 1 < argc) { chords = true; chordsPath = argv[++i]; }
        else if (a == "-macros") { macros = true; if (i + 1 < argc && ! juce::String (argv[i + 1]).startsWith ("-")) macrosPath = argv[++i]; }
    }
    const bool assembly = which == "all";
    if (withinDist < 0.0f) withinDist = minDist;   // one definition of "near-duplicate" for the whole bank
    RecipeContext ctx;
    ctx.library = vault::SampleLibrary::getShared();
    std::printf ("library: %s (%s)\n", ctx.library->libraryPath().getFullPathName().toRawUTF8(), ctx.library->libraryFound() ? "found" : "MISSING");
    const juce::File cache = cacheDir.isNotEmpty() ? cwd.getChildFile (cacheDir) : juce::File();
    if (cache != juce::File()) cache.createDirectory();
    std::vector<Result> all;
    std::map<juce::String, int> builtPerRecipe;
    for (auto& e : recipe::registry())
        if (assembly || which == e.name)
        {
            std::vector<VaultPreset> made;
            const auto cf = cache != juce::File() ? cache.getChildFile (e.name + ".cvb") : juce::File();
            if (cf != juce::File() && cf.existsAsFile() && ! rebuild.contains (e.name) && ! rebuild.contains ("all") && vault::loadBankFile (cf, made))
                std::printf ("recipe %s: %d presets from the cache\n", e.name.toRawUTF8(), (int) made.size());
            else
            {
                const auto t0 = juce::Time::getMillisecondCounterHiRes();
                made = e.build (ctx);
                std::printf ("recipe %s: %d presets built in %.0f s\n", e.name.toRawUTF8(), (int) made.size(), (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
                if (cf != juce::File()) vault::saveBankFile (cf, made);
            }
            builtPerRecipe[e.name] = (int) made.size();
            for (auto& p : made) { Result r; r.preset = p; r.group = e.name; all.push_back (std::move (r)); }
        }
    if (all.empty()) { std::printf ("no recipe '%s' (registered:", which.toRawUTF8()); for (auto& e : recipe::registry()) std::printf (" %s", e.name.toRawUTF8()); std::printf (")\n"); return 1; }
    if (assembly)   // real instruments win a cross-recipe near-duplicate pair (recipe order inside a recipe is kept)
        std::stable_sort (all.begin(), all.end(), [] (const Result& a, const Result& b) { return recipePriority (a.group) < recipePriority (b.group); });
    std::printf ("%d candidate presets, %d threads\n", (int) all.size(), threads);
    {
        std::set<juce::String> ids, names;
        for (auto& r : all)
        {
            juce::String why;
            if (! vault::validatePreset (r.preset, &why)) r.fail = "invalid: " + why;
            else if (! ids.insert (r.preset.id).second) r.fail = "duplicate id";
            else if (! assembly && ! names.insert (r.preset.name.toLowerCase()).second) r.fail = "duplicate name";   // assembly: checked among the accepted
        }
    }
    // analysis (cached by preset hash)
    std::map<uint64_t, juce::String> cached;
    const auto cacheFile = cache != juce::File() ? cache.getChildFile ("analysis.tsv") : juce::File();
    if (cacheFile.existsAsFile())
    {
        juce::StringArray lines;
        cacheFile.readLines (lines);
        for (auto& l : lines)
            if (l.isNotEmpty()) cached[(uint64_t) l.upToFirstOccurrenceOf ("\t", false, false).getHexValue64()] = l;
    }
    std::vector<uint64_t> hashes (all.size());
    std::vector<int> todo;
    int fromCache = 0;
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (all[i].fail.isNotEmpty()) continue;
        hashes[i] = presetHash (all[i].preset);
        auto it = cached.find (hashes[i]);
        if (it != cached.end() && fromCacheLine (it->second, all[i])) ++fromCache;
        else todo.push_back ((int) i);
    }
    std::printf ("analysis: %d from the cache, %d to measure\n", fromCache, (int) todo.size());
    std::mutex cacheLock;
    std::unique_ptr<juce::FileOutputStream> cacheOut;
    if (cacheFile != juce::File()) { cacheOut = std::make_unique<juce::FileOutputStream> (cacheFile); if (! cacheOut->openedOk()) cacheOut.reset(); }
    auto store = [&] (uint64_t h, const Result& r)
    {
        if (cacheOut == nullptr) return;
        const std::lock_guard<std::mutex> lock (cacheLock);
        *cacheOut << cacheLine (h, r) << "\n";
        cacheOut->flush();
    };
    auto parallelFor = [threads] (int count, const std::function<void (int)>& fn)
    {
        std::atomic<int> next { 0 }, done { 0 };
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t)
            pool.emplace_back ([&]
            {
                for (int k; (k = next++) < count;)
                {
                    fn (k);
                    if (const int d = ++done; d % 50 == 0) std::printf ("  %d / %d\n", d, count);
                }
            });
        for (auto& t : pool) t.join();
    };
    parallelFor ((int) todo.size(), [&] (int k)
    {
        auto& r = all[(size_t) todo[(size_t) k]];
        analyse (r);
        store (hashes[(size_t) todo[(size_t) k]], r);
    });
    // peak control: a preset the vel-1.0 peak holds more than -spread dB under the category loudness, or whose listening
    // phrase (the C5 triad) would drive the safety limiter more than maxChordOver dB, gets the preset peak control
    // (vault::peakKey, a 1 ms look-ahead peak compressor before the trim), threshold iterated until both fit
    int peakFixed = 0, peakTried = 0;
    if (qa)
    {
        // how far a measured preset is from fitting (dB; <= 0 fits): loudness short of the target, phrase over the limiter
        auto shortBy = [spread] (const Result& r) { return std::max (r.quietBy - (spread - 0.2f), r.chordOver - (maxChordOver - 0.2f)); };
        std::vector<int> need;
        for (int i = 0; i < (int) all.size(); ++i)
            if (all[(size_t) i].fail.isEmpty() && shortBy (all[(size_t) i]) > 0.0f) need.push_back (i);
        peakTried = (int) need.size();
        std::printf ("peak control: %d presets under the loudness spread or over the chord headroom\n", (int) need.size());
        std::vector<Result> best (all.size());
        std::vector<float> excess (all.size(), 0.0f);
        const auto orig = all;
        for (int iter = 0; iter < 8 && ! need.empty(); ++iter)
        {
            parallelFor ((int) need.size(), [&] (int k)
            {
                const int i = need[(size_t) k];
                const auto& o = orig[(size_t) i];
                const float q = shortBy (iter == 0 ? o : best[(size_t) i]) + spread - 0.2f;
                excess[(size_t) i] = juce::jmin (16.0f, excess[(size_t) i] + juce::jmax (0.5f, (q - 0.7f) * 1.2f));
                Result t;
                t.preset = o.preset; t.group = o.group;
                t.preset.parts.front().set (vault::peakKey, juce::jlimit (-60.0f, -0.1f, juce::Decibels::gainToDecibels (juce::jmax (1.0e-6f, o.peak127)) - excess[(size_t) i]));
                // the threshold follows the velocity like the sound's own level does (dB per dB of velocity, measured 0.35 -> 1.0)
                t.preset.parts.front().set (vault::peakVelKey, juce::jlimit (0.0f, 4.0f, o.velRange / (20.0f * std::log10 (1.0f / 0.35f))));
                const auto h = presetHash (t.preset);
                if (auto it = cached.find (h); it == cached.end() || ! fromCacheLine (it->second, t)) { analyse (t); store (h, t); }
                best[(size_t) i] = std::move (t);
            });
            std::vector<int> again;
            for (int i : need)
                if (best[(size_t) i].fail.isEmpty() && shortBy (best[(size_t) i]) > 0.0f && excess[(size_t) i] < 16.0f) again.push_back (i);
            std::printf ("  pass %d: %d still under\n", iter + 1, (int) again.size());
            need.swap (again);
        }
        for (int i = 0; i < (int) all.size(); ++i)
            if (best[(size_t) i].print.size() > 0 && best[(size_t) i].fail.isEmpty() && best[(size_t) i].quietBy <= spread
                && best[(size_t) i].chordOver <= maxChordOver)
            {
                all[(size_t) i] = best[(size_t) i];
                ++peakFixed;
            }
        std::printf ("peak control: %d of %d presets now fit (loudness within %.1f dB, phrase headroom)\n", peakFixed, peakTried, spread);
        std::map<juce::String, int> why;   // what stopped the rest
        for (int i = 0; i < (int) all.size(); ++i)
            if (best[(size_t) i].print.size() > 0 && ! (best[(size_t) i].fail.isEmpty() && best[(size_t) i].quietBy <= spread
                                                         && best[(size_t) i].chordOver <= maxChordOver))
                ++why[best[(size_t) i].fail.isNotEmpty() ? best[(size_t) i].fail.upToFirstOccurrenceOf (":", false, false)
                      : best[(size_t) i].quietBy > spread ? "still " + juce::String ((int) std::ceil (best[(size_t) i].quietBy)) + " dB under"
                                                          : "phrase " + juce::String ((int) std::ceil (best[(size_t) i].chordOver)) + " dB over"];
        for (auto& [w, n] : why) std::printf ("  unresolved: %-22s %d\n", w.toRawUTF8(), n);
    }
    cacheOut.reset();
    // a trim above +22 dB lifts the noise floor and the release tails of a near-silent render (and validatePreset allows at
    // most x16 = +24 dB): such a preset is too quiet as designed
    for (auto& r : all)
        if (r.fail.isEmpty() && r.gainDb > maxTrimDb) r.fail = "too-quiet";
    // assembly QA (computed from the measurements, so the limits can change without re-measuring)
    if (qa)
        for (auto& r : all)
        {
            if (r.fail.isNotEmpty()) continue;
            const int c = r.preset.category;
            if (r.quietBy > spread) r.fail = "loudness-spread";
            else if (r.share8k > maxShare8k) r.fail = "air-8k";
            else if (r.centroid > centroidCeiling (c)) r.fail = "centroid";
            else if (r.chordOver > maxChordOver) r.fail = "chord-headroom";
        }
    // names for the browser list (a 22-character name fits its column; longer designer names use "&" for "and")
    for (auto& r : all)
        if (r.preset.name.length() > 22 && r.preset.name.contains (" and "))
            r.preset.name = r.preset.name.replace (" and ", " & ");
    // distinctness: in order (assembly: recipe priority, then recipe order); inside a recipe -wdist, across recipes -dist
    std::vector<int> accepted;
    std::set<juce::String> acceptedNames;
    for (int i = 0; i < (int) all.size(); ++i)
    {
        auto& r = all[(size_t) i];
        bool dupWithin = false, dupCross = false;
        for (int j : accepted)
        {
            const auto& o = all[(size_t) j];
            const float d = distance (r.print, o.print);
            if (d < r.minDist) { r.minDist = d; r.nearest = o.preset.name; }
            if (o.group == r.group) dupWithin |= d < withinDist;
            else dupCross |= d < minDist;
        }
        if (r.fail.isEmpty() && dupWithin) r.fail = "near-duplicate";
        if (r.fail.isEmpty() && dupCross) r.fail = "cross-duplicate";
        if (r.fail.isEmpty() && ! acceptedNames.insert (r.preset.name.toLowerCase()).second) r.fail = "duplicate name";
        if (r.fail.isEmpty()) accepted.push_back (i);
    }
    // min pairwise distance among the accepted (all pairs, and pairs from different recipes)
    float minPair = 1.0e9f, minCross = 1.0e9f; juce::String pairA, pairB, crossA, crossB;
    for (size_t x = 0; x < accepted.size(); ++x)
        for (size_t y = x + 1; y < accepted.size(); ++y)
        {
            const auto& a = all[(size_t) accepted[x]]; const auto& b = all[(size_t) accepted[y]];
            const float d = distance (a.print, b.print);
            if (d < minPair) { minPair = d; pairA = a.preset.name; pairB = b.preset.name; }
            if (a.group != b.group && d < minCross) { minCross = d; crossA = a.preset.name; crossB = b.preset.name; }
        }
    // macro extremes of every accepted preset (at its final gain, FX on): -macros [fails.tsv]
    int macrosChecked = 0, macrosQuiet = 0, macrosNarrowed = 0, macrosCurve = 0;
    juce::StringArray macroFails, macrosFixed;
    std::map<juce::String, int> macroWhy;
    if (macros)
    {
        std::vector<MacroCheck> mcs (accepted.size());
        // cache (with -cache): macros.tsv + macros.cvb, one entry per checked preset, keyed by the preset as checked (hash
        // of everything incl. the final gain): the counters / fail lines and the preset with its narrowed macros
        const auto mcTsv = cache != juce::File() ? cache.getChildFile ("macros.tsv") : juce::File();
        const auto mcCvb = cache != juce::File() ? cache.getChildFile ("macros.cvb") : juce::File();
        auto macroKey = [] (const VaultPreset& p)
        {
            uint32_t g = 0;
            std::memcpy (&g, &p.gain, sizeof (g));
            return (presetHash (p) ^ ((uint64_t) g * 0x9E3779B97F4A7C15ull)) + (uint64_t) macroCheckVersion;
        };
        auto pack = [] (const juce::StringArray& a) { return a.joinIntoString (juce::String::charToString (0x1e)).replace ("\t", juce::String::charToString (0x1f)); };
        auto unpack = [] (const juce::String& t)
        {
            juce::StringArray a;
            if (t.isNotEmpty()) a.addTokens (t, juce::String::charToString (0x1e), "");
            for (auto& x : a) x = x.replace (juce::String::charToString (0x1f), "\t");
            return a;
        };
        std::map<uint64_t, std::pair<juce::String, VaultPreset>> mcCache;
        {
            juce::StringArray lines;
            std::vector<VaultPreset> bank;
            if (mcTsv.existsAsFile() && mcCvb.existsAsFile() && vault::loadBankFile (mcCvb, bank))
            {
                mcTsv.readLines (lines);
                lines.removeEmptyStrings();
                if (lines.size() == (int) bank.size())
                    for (int i = 0; i < lines.size(); ++i)
                        mcCache[(uint64_t) lines[i].upToFirstOccurrenceOf ("\t", false, false).getHexValue64()] = { lines[i], bank[(size_t) i] };
            }
        }
        std::vector<uint64_t> keys (accepted.size());
        std::vector<int> mcTodo;
        for (size_t k = 0; k < accepted.size(); ++k)
        {
            auto& pr = all[(size_t) accepted[k]].preset;
            keys[k] = macroKey (pr);
            auto it = mcCache.find (keys[k]);
            if (it != mcCache.end() && it->second.second.id == pr.id)
            {
                auto c = juce::StringArray::fromTokens (it->second.first, "\t", "");
                while (c.size() < 8) c.add ({});
                auto& mc = mcs[k];
                mc.checked = c[1].getIntValue(); mc.narrowed = c[2].getIntValue(); mc.curveRefit = c[3].getIntValue(); mc.quiet = c[4].getIntValue();
                for (auto& w : juce::StringArray::fromTokens (c[5], ",", "")) if (w.contains ("=")) mc.why[w.upToFirstOccurrenceOf ("=", false, false)] = w.fromFirstOccurrenceOf ("=", false, false).getIntValue();
                mc.fails = unpack (c[6]); mc.fixed = unpack (c[7]);
                pr = it->second.second;
            }
            else mcTodo.push_back ((int) k);
        }
        std::printf ("macro check: %d presets (%d from the cache)\n", (int) accepted.size(), (int) (accepted.size() - mcTodo.size()));
        parallelFor ((int) mcTodo.size(), [&] (int j)
        {
            const auto k = (size_t) mcTodo[(size_t) j];
            mcs[k] = checkPresetMacros (all[(size_t) accepted[k]].preset, true);
        });
        if (mcTsv != juce::File())
        {
            for (int k : mcTodo)
            {
                const auto& mc = mcs[(size_t) k];
                juce::StringArray why;
                for (auto& [w, n] : mc.why) why.add (w + "=" + juce::String (n));
                juce::String line;
                line << juce::String::toHexString ((juce::int64) keys[(size_t) k]) << "\t" << mc.checked << "\t" << mc.narrowed << "\t" << mc.curveRefit << "\t" << mc.quiet
                     << "\t" << why.joinIntoString (",") << "\t" << pack (mc.fails) << "\t" << pack (mc.fixed);
                mcCache[keys[(size_t) k]] = { line, all[(size_t) accepted[(size_t) k]].preset };
            }
            juce::StringArray lines;
            std::vector<VaultPreset> bank;
            for (auto& [h, e] : mcCache) { lines.add (e.first); bank.push_back (e.second); }
            vault::saveBankFile (mcCvb, bank);
            mcTsv.replaceWithText (lines.joinIntoString ("\n") + "\n");
        }
        for (auto& mc : mcs)
        {
            macrosChecked += mc.checked; macrosQuiet += mc.quiet; macroFails.addArray (mc.fails); macrosFixed.addArray (mc.fixed);
            macrosNarrowed += mc.narrowed; macrosCurve += mc.curveRefit;
            for (auto& [w, n] : mc.why) macroWhy[w] += n;
        }
        std::printf ("macro check: %d macros at 0 and 1; %d had an extreme outside the bank rules, %d of them fixed by narrowing that side "
                     "(%d narrowings, %d curve re-fits off the designed value), %d left; %d barely audible\n", macrosChecked,
                     macrosFixed.size() + macroFails.size(), macrosFixed.size(), macrosNarrowed, macrosCurve, macroFails.size(), macrosQuiet);
        for (auto& [w, n] : macroWhy) std::printf ("  macro extreme %-14s %d\n", w.toRawUTF8(), n);
        for (int k = 0; k < std::min (40, macroFails.size()); ++k) std::printf ("  %s\n", macroFails[k].toRawUTF8());
        if (macrosPath.isNotEmpty()) cwd.getChildFile (macrosPath).replaceWithText ((macrosFixed.joinIntoString ("\n") + "\n" + macroFails.joinIntoString ("\n")).trim() + "\n");
    }
    std::map<juce::String, int> fails;
    std::map<int, int> perCat;
    for (auto& r : all) { if (r.fail.isNotEmpty()) ++fails[r.fail.upToFirstOccurrenceOf (":", false, false)]; else ++perCat[r.preset.category]; }
    std::printf ("accepted %d of %d; min pairwise distance %.2f (%s / %s); across recipes %.2f (%s / %s)\n", (int) accepted.size(), (int) all.size(),
                 minPair, pairA.toRawUTF8(), pairB.toRawUTF8(), minCross < 1e8f ? minCross : 0.0f, crossA.toRawUTF8(), crossB.toRawUTF8());
    for (auto& [c, n] : perCat) std::printf ("  %-20s %d\n", vault::categoryNames()[c].toRawUTF8(), n);
    for (auto& [f, n] : fails) std::printf ("  rejected %-16s %d\n", f.toRawUTF8(), n);

    if (outPath.isNotEmpty())
    {
        std::vector<VaultPreset> bank;
        for (int i : accepted) bank.push_back (all[(size_t) i].preset);
        std::stable_sort (bank.begin(), bank.end(), [] (const VaultPreset& a, const VaultPreset& b) { return a.category < b.category; });
        if (! vault::saveBankFile (cwd.getChildFile (outPath), bank)) { std::printf ("cannot write %s\n", outPath.toRawUTF8()); return 1; }
        std::printf ("wrote %s (%d presets)\n", outPath.toRawUTF8(), (int) bank.size());
    }
    if (reportPath.isNotEmpty())
    {
        juce::String s;
        s << "# CHROME VAULT bank report: " << which << "\n\n"
          << "Generated by `VaultBankGen -recipe " << which << "` (tools/vault/recipes/RecipeGate.h). Every number is measured on the C5 render "
          << "(MIDI 60, FL Studio naming), velocity 0.8, 48 kHz, unless noted. Nobody listened; this is measurement only.\n\n"
          << "* accepted **" << (int) accepted.size() << "** of " << (int) all.size() << " candidates\n"
          << "* minimum pairwise fingerprint distance **" << juce::String (minPair, 2) << "** (" << pairA << " / " << pairB << ")"
          << (assembly ? ", across recipes **" + juce::String (minCross, 2) + "** (" + crossA + " / " + crossB + "), cross-recipe threshold " + juce::String (minDist, 2) : juce::String())
          << "\n* peak control set on " << peakFixed << " presets (" << peakTried << " were under the loudness spread or over the chord headroom)"
          << (macros ? "\n* sound macros checked at 0 and 1 (C5, final gain, FX on): " + juce::String (macrosChecked) + "; "
                       + juce::String (macrosFixed.size() + macroFails.size()) + " had an extreme outside the bank rules, "
                       + juce::String (macrosFixed.size()) + " fixed by narrowing that side (default sound unchanged), "
                       + juce::String (macroFails.size()) + " left; " + juce::String (macrosQuiet) + " barely audible" : juce::String())
          << "\n* recipes:";
        for (auto& [n, c] : builtPerRecipe) s << " " << n << " " << c << ";";
        s << "\n* rejected:";
        for (auto& [f, n] : fails) s << " " << f << " " << n << ";";
        s << "\n\n## Per category (accepted; min / median / max)\n\n"
          << "| category | presets | 1 / 2 / 3 / 4 parts | loudness dev dB | H1 vs strongest dB | 2-5 kHz % | > 6 kHz % | > 8 kHz % | centroid Hz (ceiling) | vel 0.35-1.0 dB | tail s | phrase peak vs limiter dB |\n"
          << "|---|---|---|---|---|---|---|---|---|---|---|---|\n";
        for (int c = 0; c < vault::numCategories; ++c)
        {
            std::vector<float> dev, h1, s25, s6, s8, cen, vel, tail, chord;
            int parts[5] {};
            for (int i : accepted)
            {
                const auto& r = all[(size_t) i];
                if (r.preset.category != c) continue;
                dev.push_back (-r.quietBy); h1.push_back (r.h1Db); s25.push_back (r.share25); s6.push_back (r.share6k); s8.push_back (r.share8k);
                cen.push_back (r.centroid); vel.push_back (r.velRange); tail.push_back (r.tail); chord.push_back (r.chordOver);
                ++parts[juce::jlimit (1, 4, (int) r.preset.parts.size())];
            }
            s << "| " << vault::categoryNames()[c] << " | " << (int) dev.size() << " | " << parts[1] << " / " << parts[2] << " / " << parts[3] << " / " << parts[4]
              << " | " << statLine (dev, 1) << " | " << statLine (h1, 1) << " | " << statLine (s25, 1, 100) << " | " << statLine (s6, 2, 100) << " | " << statLine (s8, 2, 100)
              << " | " << statLine (cen, 0) << " (" << juce::String ((int) centroidCeiling (c)) << ") | " << statLine (vel, 1) << " | " << statLine (tail, 1) << " | " << statLine (chord, 1) << " |\n";
        }
        s << "\n## Every candidate\n\n"
          << "| id | name | recipe | parts | result | loud | gain dB | dev dB | H1 dB | 2-5k % | >6k % | >8k % | centroid | tail s | vel dB | oct-up dB | phrase dB | nearest (dist) |\n"
          << "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
        for (auto& r : all)
        {
            juce::StringArray parts;
            for (auto& pt : r.preset.parts) parts.add (pt.label + (pt.octave != 0 ? " (" + juce::String (pt.octave > 0 ? "+" : "") + juce::String (pt.octave) + ")" : juce::String()));
            s << "| " << r.preset.id << " | " << r.preset.name << " | " << r.group << " | " << parts.joinIntoString (" + ") << " | " << (r.fail.isEmpty() ? "ok" : r.fail)
              << " | " << juce::String (r.loud, 1) << " | " << juce::String (r.gainDb, 1) << " | " << juce::String (-r.quietBy, 1) << " | " << juce::String (r.h1Db, 1)
              << " | " << juce::String (100 * r.share25, 1) << " | " << juce::String (100 * r.share6k, 2) << " | " << juce::String (100 * r.share8k, 2)
              << " | " << juce::String ((int) r.centroid) << " | " << juce::String (r.tail, 1) << " | " << juce::String (r.velRange, 1)
              << " | " << (r.octUpDb > -98 ? juce::String (r.octUpDb, 1) : juce::String ("-")) << " | " << juce::String (r.chordOver, 1) << " | " << r.nearest << " (" << juce::String (r.minDist < 1e8f ? r.minDist : 0.0f, 2) << ") |\n";
        }
        cwd.getChildFile (reportPath).replaceWithText (s);
        std::printf ("wrote %s\n", reportPath.toRawUTF8());
    }
    if (printsPath.isNotEmpty())
    {
        const auto pf = cwd.getChildFile (printsPath);
        pf.deleteFile();
        juce::FileOutputStream os (pf);
        for (auto& r : all)
        {
            juce::String t;
            t << r.preset.id << "\t" << r.preset.name << "\t" << (r.fail.isEmpty() ? "ok" : r.fail) << "\t" << r.group << "\t" << (int) r.preset.parts.size()
              << "\t" << juce::String (-r.quietBy, 2) << "\t" << juce::String (r.centroid, 0) << "\t" << juce::String (100 * r.share8k, 3);
            for (auto v : r.print) t << "\t" << juce::String (v, 2);
            t << "\n";
            os << t;
        }
        os.flush();
        std::printf ("wrote %s\n", printsPath.toRawUTF8());
    }
    if (chords)   // chord headroom: the listening phrase (melody + C5 triad) of every accepted preset, peak before the safety limiter
    {
        std::vector<float> over (accepted.size());
        for (size_t k = 0; k < accepted.size(); ++k) over[k] = all[(size_t) accepted[k]].chordOver;
        std::map<int, std::vector<float>> perCatOver;
        for (size_t k = 0; k < accepted.size(); ++k) perCatOver[all[(size_t) accepted[k]].preset.category].push_back (over[k]);
        std::printf ("chord headroom (phrase peak before the limiter, dB over its 0.89 ceiling: min / median / max, share over):\n");
        for (auto& [c, v] : perCatOver)
        {
            int n = 0; for (auto x : v) n += x > 0.0f ? 1 : 0;
            std::printf ("  %-20s %s  %d of %d over\n", vault::categoryNames()[c].toRawUTF8(), statLine (v, 1).toRawUTF8(), n, (int) v.size());
        }
        std::vector<size_t> order (accepted.size());
        for (size_t k = 0; k < order.size(); ++k) order[k] = k;
        std::sort (order.begin(), order.end(), [&] (size_t a, size_t b) { return over[a] > over[b]; });
        for (size_t k = 0; k < std::min<size_t> (25, order.size()); ++k)
        {
            const auto& r = all[(size_t) accepted[order[k]]];
            std::printf ("  worst %+.1f dB  %s %s (%s)\n", over[order[k]], r.preset.id.toRawUTF8(), r.preset.name.toRawUTF8(), r.group.toRawUTF8());
        }
        if (chordsPath.isNotEmpty())
        {
            juce::String t;
            for (size_t k = 0; k < accepted.size(); ++k) t << all[(size_t) accepted[k]].preset.id << "\t" << juce::String (over[k], 2) << "\n";
            cwd.getChildFile (chordsPath).replaceWithText (t);
        }
    }
    if (demoDir.isNotEmpty())
    {
        auto dir = cwd.getChildFile (demoDir);
        dir.createDirectory();
        for (auto& id : juce::StringArray::fromTokens (demoIds, ",", ""))
            for (auto& r : all)
                if (r.preset.id == id || r.preset.name == id)
                {
                    auto b = render (r.preset, vault::homeNote, 0.8f, 1.8, 3.0);
                    const int fade = (int) (0.05 * sr);
                    for (int c = 0; c < 2; ++c) b.applyGainRamp (c, b.getNumSamples() - fade, fade, 1.0f, 0.0f);
                    const auto f = dir.getChildFile (r.preset.name.replaceCharacters (" /", "__") + " C5.wav");
                    writeWav (f, b);
                    std::printf ("demo %s\n", f.getFullPathName().toRawUTF8());
                }
    }
    if (listenDir.isNotEmpty())   // the owner's listening phrase (C5 E5 G5 C6 + C5 triad, 4 s) as OGG, ids or names separated by '|'
    {
        auto dir = cwd.getChildFile (listenDir);
        dir.createDirectory();
        for (auto& id : juce::StringArray::fromTokens (listenIds, "|", ""))
            for (auto& r : all)
                if (r.fail.isEmpty() && (r.preset.id == id || r.preset.name == id))
                {
                    const auto b = renderPhrase (r.preset);
                    const auto f = dir.getChildFile (r.preset.id + ".ogg");
                    const bool ok = writeOgg (f, b, 2);
                    std::printf ("listen %s %s peak %.3f %s\n", r.preset.id.toRawUTF8(), r.preset.name.toRawUTF8(), b.getMagnitude (0, b.getNumSamples()), ok ? "ok" : "FAILED");
                }
    }
    return 0;
}
} // namespace recipegate
