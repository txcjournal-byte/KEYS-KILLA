#pragma once
// CHROME VAULT bank AUDIO AUDIT - an independent second opinion on the shipped bank (VaultBankGen -audit <bank.cvb>).
//
// The generator's gate (BankQC.h) renders two notes per preset. The audit renders every preset again, differently:
//   * one note at the spec pitch (C4 = MIDI 60, 808 & Bass at C2 = MIDI 36), velocity 0.8, note off at 1.05 s, analysed over 1.5 s,
//     then the render keeps running until the release tail is 60 dB down (or 8 s after the note-off)  -> the real tail length
//   * the same note at velocity 1.0 (peak / limiter)
//   * the lowest and highest note of the playable range (MIDI 24 / 96): NaN, silence, limiter (report only)
// and checks: NaN, silence, peak > -0.5 dBFS, safety limiter reached, DC, clicks at note start / note end / anywhere in the tail,
// mono-sum loss > 3 dB, side energy below 120 Hz (FFT, exact band), loudness outliers > 2 dB from the category median,
// tail > 8 s, CPU (thread CPU time per second of audio, > 3x the median of presets with the same layer count),
// near-duplicates (an own time-frequency fingerprint, different from the generator's), names (unique, ASCII English, banned words /
// phrases).
#include "BankQC.h"
#include <algorithm>
#include <ctime>
#include <map>

namespace vaultaudit
{
constexpr double sr = 48000.0;
constexpr double specSeconds = 1.5, noteOffSeconds = 1.05, maxTailSeconds = 8.0;
inline int specNote (int category) { return category == vault::cat808Bass ? 36 : 60; }

// thresholds
constexpr float peakMaxDb = -0.5f;           // hard: never above -0.5 dBFS
constexpr float limiterLevel = 0.885f;       // the engine's safety limiter acts at 0.89: a factory sound must stay under it at velocity 0.8
constexpr float monoLossMaxDb = 3.0f;
constexpr float lowSideMaxDb = -30.0f;       // side power below 120 Hz vs the total power
constexpr float lowSideVsMidMaxDb = -20.0f;  // ... and vs the mid power below 120 Hz (when the lows matter)
constexpr float loudnessOutlierDb = 2.0f;
constexpr float cpuOutlierFactor = 3.0f;
constexpr float dupDistance = 2.0f;          // fingerprint distance (dB RMS over the time-frequency grid)

constexpr int fpBands = 24, fpSegs = 5, fpSize = fpBands * fpSegs + 3;
using Print = std::array<float, fpSize>;

struct Result
{
    bool finite = true, finiteExtremes = true;
    float peak = 0, peak127 = 0, rms = 0, dcAbs = 0, dcRel = 0;
    float loud = -120;                 // K-weighted loudest 400 ms of the 1.5 s spec render
    float loudFull = -120;             // ... of the full note (longer render for slow attacks, as the generator measures it)
    float firstSample = 0, onJump = 0, laterJump = 0, offJump = 0, preOffJump = 0, spike = 0;
    float monoLossDb = 0, lowSideTotDb = -120, lowSideMidDb = -120, lowMidTotDb = -120;
    float tail = 0;                    // seconds from the note-off until 60 dB under the loudest 10 ms frame
    float cpu = 0;                     // thread CPU seconds per second of audio (spec render)
    float extremeLowLoud = -120, extremeHighLoud = -120, extremePeak = 0;
    Print print {};
    juce::StringArray fails, infos;
};

inline double threadCpuSeconds()
{
   #if defined (CLOCK_THREAD_CPUTIME_ID)
    timespec t {};
    clock_gettime (CLOCK_THREAD_CPUTIME_ID, &t);
    return (double) t.tv_sec + 1.0e-9 * (double) t.tv_nsec;
   #else
    return juce::Time::getMillisecondCounterHiRes() * 0.001;
   #endif
}

inline float db (double p) { return (float) (10.0 * std::log10 (std::max (1.0e-14, p))); }

// renders one note; `out` gets the first `keepSeconds`; keeps running (up to note-off + maxTail + 0.5 s) to measure the tail when
// `tail` != nullptr. Returns thread CPU seconds spent processing the first `keepSeconds`.
inline double renderNote (const VaultPreset& p, int note, float vel, double keepSeconds, juce::AudioBuffer<float>& out, float* tail,
                          double offSeconds = noteOffSeconds)
{
    const auto compiled = VaultEngine::compile (p);
    auto eng = std::make_unique<VaultEngine>();
    for (int m = 0; m < vault::numMacros; ++m) eng->setMacro (m, compiled.macroDefaults[m]);
    constexpr int block = 512;
    const int keep = (int) std::round (keepSeconds * sr), off = (int) std::round (offSeconds * sr);
    const int maxTotal = tail != nullptr ? off + (int) ((maxTailSeconds + 0.6) * sr) : keep;
    out.setSize (2, keep);
    out.clear();
    eng->prepare (sr, block, std::max (1, compiled.numParts));
    eng->setPreset (compiled, true);
    const double c0 = threadCpuSeconds();   // processing only (not the allocation in prepare)
    double cpu = 0;
    juce::AudioBuffer<float> buf (2, block);
    const auto v = (juce::uint8) juce::jlimit (1, 127, (int) std::round (vel * 127.0f));
    // tail tracking: 10 ms frames
    const int fr = (int) (0.01 * sr);
    double frameAcc = 0; int frameFill = 0, frameIdx = 0;
    float envMax = -200; int lastLoudFrame = 0, quietFrames = 0;
    std::vector<float> env;
    for (int s0 = 0; s0 < maxTotal; s0 += block)
    {
        const int len = std::min (block, maxTotal - s0);
        juce::MidiBuffer mb;
        if (s0 == 0) mb.addEvent (juce::MidiMessage::noteOn (1, note, v), 0);
        if (off >= s0 && off < s0 + len) mb.addEvent (juce::MidiMessage::noteOff (1, note), off - s0);
        buf.setSize (2, len, false, false, true);
        eng->process (buf, mb);
        if (s0 < keep)
        {
            const int n = std::min (len, keep - s0);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, s0, buf, c, 0, n);
            if (s0 + len >= keep) cpu = threadCpuSeconds() - c0;
        }
        if (tail == nullptr) continue;
        const float* L = buf.getReadPointer (0); const float* R = buf.getReadPointer (1);
        bool done = false;
        for (int i = 0; i < len; ++i)
        {
            frameAcc += 0.5 * ((double) L[i] * L[i] + (double) R[i] * R[i]);
            if (++frameFill == fr)
            {
                const float e = db (frameAcc / fr);
                env.push_back (e);
                envMax = std::max (envMax, e);
                if (e >= envMax - 60.0f) { lastLoudFrame = frameIdx; quietFrames = 0; }
                else ++quietFrames;
                ++frameIdx; frameAcc = 0; frameFill = 0;
                // 1.6 s quiet after the note-off: done (long delay gaps are shorter than that)
                if ((s0 + i) > off && quietFrames > 160 && s0 + len >= keep) { done = true; break; }
            }
        }
        if (done) break;
    }
    if (cpu <= 0) cpu = threadCpuSeconds() - c0;
    if (tail != nullptr)
    {
        // last frame within 60 dB of the loudest frame, measured from the note-off
        int last = 0;
        for (int f = 0; f < (int) env.size(); ++f) if (env[(size_t) f] >= envMax - 60.0f) last = f;
        *tail = std::max (0.0f, (float) ((last + 1) * fr - off) / (float) sr);   // still loud at the end: > maxTailSeconds
    }
    return cpu;
}

// side / mid power below 120 Hz: Welch, 8192-point Hann, half overlap
inline void lowBand (const juce::AudioBuffer<float>& b, float& sideTotDb, float& sideMidDb, float& midTotDb)
{
    constexpr int order = 13, size = 1 << order;
    const int n = b.getNumSamples();
    const float* L = b.getReadPointer (0); const float* R = b.getReadPointer (1);
    juce::dsp::FFT fft (order);
    std::vector<float> bs ((size_t) (2 * size)), bm ((size_t) (2 * size)), win ((size_t) size);
    for (int i = 0; i < size; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size);
    double sLow = 0, mLow = 0, tot = 0;
    for (int s0 = 0; s0 < std::max (1, n - size / 2); s0 += size / 2)
    {
        std::fill (bs.begin(), bs.end(), 0.0f); std::fill (bm.begin(), bm.end(), 0.0f);
        for (int i = 0; i < size; ++i)
        {
            const int k = s0 + i;
            if (k >= n) break;
            bs[(size_t) i] = 0.5f * (L[k] - R[k]) * win[(size_t) i];
            bm[(size_t) i] = 0.5f * (L[k] + R[k]) * win[(size_t) i];
        }
        fft.performFrequencyOnlyForwardTransform (bs.data());
        fft.performFrequencyOnlyForwardTransform (bm.data());
        for (int k = 1; k < size / 2; ++k)
        {
            const double f = k * sr / size, ps = (double) bs[(size_t) k] * bs[(size_t) k], pm = (double) bm[(size_t) k] * bm[(size_t) k];
            tot += ps + pm;
            if (f < 120.0) { sLow += ps; mLow += pm; }
        }
    }
    sideTotDb = db (sLow) - db (tot);
    sideMidDb = db (sLow) - db (mLow);
    midTotDb = db (mLow) - db (tot);
}

// own fingerprint: 24 log bands (40 Hz .. 16 kHz) x 5 time segments (0-50 ms, 50-200, 200-600, 600-1050, 1050-1500 ms), dB re the
// loudest cell (floor -60), + side/mid in 3 bands. Pitch is the same for all presets of a category, so this compares timbre + envelope.
inline Print fingerprint (const juce::AudioBuffer<float>& b)
{
    constexpr int order = 10, size = 1 << order, hop = size / 4;
    const int n = b.getNumSamples();
    const float* L = b.getReadPointer (0); const float* R = b.getReadPointer (1);
    const double segEnd[fpSegs] { 0.05, 0.2, 0.6, 1.05, 1.5 };
    juce::dsp::FFT fft (order);
    std::vector<float> bm ((size_t) (2 * size)), bs ((size_t) (2 * size)), win ((size_t) size);
    for (int i = 0; i < size; ++i) win[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) size);
    std::array<double, fpBands * fpSegs> cell {};
    std::array<double, 3> sideE {}, midE {};
    for (int s0 = -size / 2; s0 + size / 2 < n; s0 += hop)
    {
        const double centre = (s0 + size / 2) / sr;
        int seg = 0;
        while (seg < fpSegs - 1 && centre >= segEnd[seg]) ++seg;
        std::fill (bm.begin(), bm.end(), 0.0f); std::fill (bs.begin(), bs.end(), 0.0f);
        for (int i = 0; i < size; ++i)
        {
            const int k = s0 + i;
            if (k < 0 || k >= n) continue;
            bm[(size_t) i] = 0.5f * (L[k] + R[k]) * win[(size_t) i];
            bs[(size_t) i] = 0.5f * (L[k] - R[k]) * win[(size_t) i];
        }
        fft.performFrequencyOnlyForwardTransform (bm.data());
        fft.performFrequencyOnlyForwardTransform (bs.data());
        for (int k = 1; k < size / 2; ++k)
        {
            const double f = k * sr / size;
            const double pm = (double) bm[(size_t) k] * bm[(size_t) k], ps = (double) bs[(size_t) k] * bs[(size_t) k];
            const int g = f < 300 ? 0 : f < 3000 ? 1 : 2;
            midE[(size_t) g] += pm; sideE[(size_t) g] += ps;
            if (f < 40.0 || f >= 16000.0) continue;
            const int bi = juce::jlimit (0, fpBands - 1, (int) (std::log (f / 40.0) / std::log (16000.0 / 40.0) * fpBands));
            cell[(size_t) (seg * fpBands + bi)] += pm + ps;
        }
    }
    Print p {};
    double mx = 1.0e-14;
    for (auto c : cell) mx = std::max (mx, c);
    for (size_t i = 0; i < cell.size(); ++i) p[i] = juce::jlimit (-60.0f, 0.0f, db (cell[i]) - db (mx));
    for (int g = 0; g < 3; ++g) p[(size_t) (fpBands * fpSegs + g)] = juce::jlimit (-40.0f, 10.0f, db (sideE[(size_t) g]) - db (midE[(size_t) g]));
    return p;
}

inline float distance (const Print& a, const Print& b)
{
    double s = 0;
    for (int i = 0; i < fpBands * fpSegs; ++i) { const double d = a[(size_t) i] - b[(size_t) i]; s += d * d; }
    s /= fpBands * fpSegs;
    double w = 0;
    for (int g = 0; g < 3; ++g) { const double d = 0.5 * (a[(size_t) (fpBands * fpSegs + g)] - b[(size_t) (fpBands * fpSegs + g)]); w += d * d; }
    return (float) std::sqrt (s + 0.1 * w / 3);
}

// the largest sample step anywhere after the attack (from 30 ms) relative to the local level: a click in a release / sequence
// step / voice kill shows up as a step much larger than the signal's own slope around it. Returns step / local slope (max).
inline float worstSpike (const juce::AudioBuffer<float>& b)
{
    const int n = b.getNumSamples();
    const float* L = b.getReadPointer (0); const float* R = b.getReadPointer (1);
    const int fr = 48;   // 1 ms frames
    const int nf = n / fr;
    std::vector<float> d ((size_t) nf, 0.0f);
    for (int f = 0; f < nf; ++f)
        for (int i = std::max (1, f * fr); i < (f + 1) * fr; ++i)
            d[(size_t) f] = std::max (d[(size_t) f], std::max (std::abs (L[i] - L[i - 1]), std::abs (R[i] - R[i - 1])));
    float worst = 0;
    for (int f = 30; f < nf; ++f)
    {
        if (d[(size_t) f] < 0.01f) continue;   // inaudible steps
        float around = 0;
        for (int g = std::max (0, f - 20); g < std::min (nf, f + 21); ++g) if (std::abs (g - f) > 2) around = std::max (around, d[(size_t) g]);
        worst = std::max (worst, d[(size_t) f] / std::max (around, 1.0e-4f));
    }
    return worst;
}

// margins: the generator checks with a little room, the audit with the plain limits
struct Limits { float tail = (float) maxTailSeconds, monoLoss = monoLossMaxDb, lowSideTot = lowSideMaxDb, lowSideVsMid = lowSideVsMidMaxDb, spike = 8.0f; };

// the spec render (velocity 0.8, note off at 1.05 s, 1.5 s analysed + the real tail) and everything measured on it
inline bool measureSpec (const VaultPreset& p, Result& r)
{
    juce::AudioBuffer<float> b;
    r.cpu = (float) (renderNote (p, specNote (p.category), 0.8f, specSeconds, b, &r.tail) / specSeconds);
    const auto a = vaultqc::analyse (b, sr, noteOffSeconds);
    r.finite = a.finite;
    if (! a.finite) return false;
    r.peak = a.peak; r.rms = a.rms; r.loud = a.loud;
    r.firstSample = a.firstSample; r.onJump = a.onsetJump; r.laterJump = a.laterJump; r.offJump = a.offJump; r.preOffJump = a.preOffJump;
    r.monoLossDb = a.monoLossDb;
    {
        double sl = 0, sr2 = 0;
        const int n = b.getNumSamples();
        for (int i = 0; i < n; ++i) { sl += b.getSample (0, i); sr2 += b.getSample (1, i); }
        r.dcAbs = (float) std::max (std::abs (sl / n), std::abs (sr2 / n));
        r.dcRel = r.dcAbs / std::max (1.0e-9f, r.rms);
    }
    lowBand (b, r.lowSideTotDb, r.lowSideMidDb, r.lowMidTotDb);
    r.print = fingerprint (b);
    r.spike = worstSpike (b);
    return true;
}

// the per-preset checks of the spec render (stable short codes: the generator maps them to repairs)
inline void specFails (Result& r, const Limits& lim = {})
{
    if (! r.finite) { r.fails.add ("nan"); return; }
    if (r.rms < 1.0e-4f || r.loud < -50.0f) r.fails.add ("silent");
    if (juce::Decibels::gainToDecibels (std::max (r.peak, r.peak127)) > peakMaxDb) r.fails.add ("peak>-0.5dB");
    if (r.peak >= limiterLevel) r.fails.add ("limiter");
    if (r.dcAbs > 0.002f && r.dcRel > 0.02f) r.fails.add ("dc");
    if (r.firstSample > 0.01f || r.onJump > 1.5f * r.laterJump + 0.02f) r.fails.add ("click-start");
    if (r.offJump > 1.5f * r.preOffJump + 0.01f) r.fails.add ("click-end");
    if (r.spike > lim.spike) r.fails.add ("click-spike");
    if (r.monoLossDb > lim.monoLoss) r.fails.add ("mono-loss");
    if (r.lowSideTotDb > lim.lowSideTot || (r.lowMidTotDb > -30.0f && r.lowSideMidDb > lim.lowSideVsMid)) r.fails.add ("low-side");
    if (r.tail > lim.tail) r.fails.add ("tail>8s");
}

inline Result analysePreset (const VaultPreset& p)
{
    Result r;
    const int note = specNote (p.category);
    if (! measureSpec (p, r)) { r.fails.add ("nan"); return r; }

    // full note loudness (slow attacks need longer than 1.5 s - as the generator measures them)
    {
        const double secs = vaultqc::renderSeconds (p);
        if (secs > specSeconds + 0.01)
        {
            juce::AudioBuffer<float> f;
            renderNote (p, note, 0.8f, secs, f, nullptr, secs * 0.7);
            r.loudFull = vaultqc::analyse (f, sr, secs * 0.7).loud;
        }
        else r.loudFull = r.loud;
    }
    // velocity 1.0
    {
        juce::AudioBuffer<float> f;
        renderNote (p, note, 1.0f, specSeconds, f, nullptr);
        r.peak127 = f.getMagnitude (0, f.getNumSamples());
        if (! std::isfinite (r.peak127)) { r.finite = false; r.fails.add ("nan-vel127"); }
    }
    // the ends of the range (report only, except NaN)
    {
        juce::AudioBuffer<float> f;
        for (int k = 0; k < 2; ++k)
        {
            renderNote (p, k == 0 ? 24 : 96, 0.8f, 0.8, f, nullptr, 0.5);
            const auto e = vaultqc::analyse (f, sr, 0.5);
            if (! e.finite) { r.finiteExtremes = false; r.fails.add ("nan-extreme"); break; }
            (k == 0 ? r.extremeLowLoud : r.extremeHighLoud) = e.loud;
            r.extremePeak = std::max (r.extremePeak, e.peak);
        }
    }
    specFails (r);
    // report-only
    if (r.peak127 >= limiterLevel) r.infos.add ("limiter-vel127");
    if (r.extremePeak >= limiterLevel) r.infos.add ("limiter-extreme");
    if (r.extremeLowLoud < -60.0f || r.extremeHighLoud < -60.0f) r.infos.add ("silent-extreme");
    return r;
}

//==============================================================================
// names
inline const juce::StringArray& offensiveWords()
{
    static const juce::StringArray b { "kill", "killer", "murder", "rape", "nazi", "hitler", "slave", "suicide", "cocaine", "heroin", "crack", "meth",
                                       "whore", "slut", "bitch", "fuck", "shit", "cunt", "nigga", "nigger", "fag", "retard", "gun", "glock", "pistol",
                                       "rifle", "shooter", "bullet", "terror", "jihad", "isis", "kkk", "porn", "sex", "pimp", "thug" };
    return b;
}
// "" = fine, else why
inline juce::String nameProblem (const juce::String& name)
{
    if (name.isEmpty() || name.trim() != name) return "empty/whitespace";
    for (auto c : name) if (c > 126 || c < 32) return "non-ASCII";
    if (! vaultqc::isCleanName (name)) return "brand word / phrase";
    const auto l = " " + name.toLowerCase().replaceCharacters ("-_.,&", "     ") + " ";
    for (auto& w : offensiveWords()) if (l.contains (" " + w + " ") || l.contains (" " + w + "s ")) return "offensive '" + w + "'";
    return {};
}
} // namespace vaultaudit
