// CHROME VAULT physical models: unit tests (run by VaultTests, alone with "VaultTests -models").
//
// For every registered model (Source/vault/models): stability (no NaN / Inf / denormals, long held notes with extreme
// parameters, bounded output), pitch accuracy within +-5 cents from C2 to C7 (FL Studio names: MIDI 24..84), level rules
// at C5 (velocity 1.0 peaks below -1 dBFS), clicks (release, bends, the end of a voice, onsets), determinism, and no
// allocation on the audio thread (noteOn / setParams / setPitch / render / noteOff / kill).
#include "../Source/vault/VaultEngine.h"
#include "../Source/vault/models/ModelVoice.h"
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cstdio>

namespace allocHook   // tests/VaultTests.cpp: operator new counts while countAllocs is set on this thread
{
extern thread_local bool countAllocs;
extern std::atomic<int> allocs;
}

namespace
{
using vault::models::ModelInfo;
using vault::models::ModelRegistry;

int modelFails = 0;
void mcheck (bool ok, const juce::String& what)
{
    if (! ok) ++modelFails;
    std::printf ("  [%s] %s\n", ok ? "ok" : "FAIL", what.toRawUTF8());
}

float hzOf (int note) { return 440.0f * std::exp2 (((float) note - 69.0f) / 12.0f); }

std::vector<float> paramsOf (const ModelInfo& mi, std::initializer_list<std::pair<const char*, float>> over)
{
    std::vector<float> p;
    for (auto& in : mi.params)
    {
        float v = in.def;
        for (auto& [k, x] : over) if (juce::String (k) == in.id) v = juce::jlimit (in.lo, in.hi, x);
        p.push_back (v);
    }
    return p;
}

struct Render { std::vector<float> l, r; int activeUntil = -1; };

// drive one voice like the engine does: setPitch every 128 samples, noteOff at `hold` seconds; pitchFn: semitone offset
Render run (const ModelInfo& mi, const std::vector<float>& p, int note, float vel, double secs, double hold, double sr,
            std::function<float (int)> bend = nullptr, int block = 128)
{
    Render o;
    const int total = (int) (secs * sr), holdN = (int) (hold * sr);
    o.l.assign ((size_t) total, 0.0f); o.r.assign ((size_t) total, 0.0f);
    auto v = mi.create();
    v->prepare (sr, 512);
    v->setParams (p.data(), (int) p.size());
    v->noteOn (note, vel, hzOf (note));
    bool off = false;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        if (! off && pos >= holdN) { v->noteOff (0.5f); off = true; }
        if (! v->isActive()) { if (o.activeUntil < 0) o.activeUntil = pos; continue; }
        v->setPitch (hzOf (note) * (bend ? std::exp2 (bend (pos) / 12.0f) : 1.0f));
        v->render (o.l.data() + pos, o.r.data() + pos, n);
    }
    return o;
}

// fundamental frequency near `expect`: Hann window, zero-padded FFT, parabolic interpolation on the log magnitude;
// `ensemble`: power-weighted centre of the +-60 cent band (detuned choirs)
double measureHz (const std::vector<float>& x, double sr, int from, int len, double expect, bool ensemble = false)
{
    const int order = 18, n = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> d ((size_t) n * 2, 0.0f);
    len = std::min ({ len, n, (int) x.size() - from });
    for (int i = 0; i < len; ++i)
        d[(size_t) i] = x[(size_t) (from + i)] * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) len));
    fft.performFrequencyOnlyForwardTransform (d.data());
    const double bin = sr / n;
    const int k0 = std::max (2, (int) (expect * std::exp2 (-60.0 / 1200.0) / bin)), k1 = (int) (expect * std::exp2 (60.0 / 1200.0) / bin) + 1;
    if (ensemble)
    {
        double ps = 0, fs = 0;
        for (int k = k0; k <= k1; ++k) { const double pw = (double) d[(size_t) k] * d[(size_t) k]; ps += pw; fs += pw * k * bin; }
        return ps > 0 ? fs / ps : 0.0;
    }
    int best = k0;
    for (int k = k0; k <= k1; ++k) if (d[(size_t) k] > d[(size_t) best]) best = k;
    const double a = std::log (d[(size_t) best - 1] + 1e-20), b = std::log (d[(size_t) best] + 1e-20), c = std::log (d[(size_t) best + 1] + 1e-20);
    const double den = a - 2 * b + c;
    return (best + (std::abs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0)) * bin;
}

float peakOf (const std::vector<float>& x, int from = 0, int to = -1)
{
    float m = 0.0f;
    to = to < 0 ? (int) x.size() : std::min (to, (int) x.size());
    for (int i = std::max (0, from); i < to; ++i) m = std::max (m, std::abs (x[(size_t) i]));
    return m;
}

float maxDiff (const std::vector<float>& x, int from, int to, int order)
{
    float m = 0.0f;
    from = std::max (order, from); to = std::min (to, (int) x.size());
    for (int i = from; i < to; ++i)
    {
        const float d = order == 1 ? x[(size_t) i] - x[(size_t) i - 1] : x[(size_t) i] - 2.0f * x[(size_t) i - 1] + x[(size_t) i - 2];
        m = std::max (m, std::abs (d));
    }
    return m;
}

bool isVoice (const juce::String& id) { return id.startsWith ("voice."); }
bool isEnsemble (const juce::String& id) { return id == "voice.choir" || id == "voice.pad"; }
} // namespace

int vaultModelTests()
{
    modelFails = 0;
    const double sr = 48000.0;
    auto& reg = ModelRegistry::get();
    std::printf ("physical models (%d registered)\n", reg.size());
    mcheck (reg.size() >= 21, "registry: " + reg.ids().joinIntoString (" "));
    {
        // the click detectors themselves: a 262 Hz sine passes, the same sine with a 0.2 step fails
        std::vector<float> s ((size_t) sr), c;
        for (size_t i = 0; i < s.size(); ++i) s[i] = 0.3f * std::sin (6.2831853f * 261.6f * (float) i / (float) sr);
        c = s;
        for (size_t i = s.size() / 2; i < s.size(); ++i) c[i] += 0.2f;
        const float smooth = maxDiff (s, 100, (int) s.size(), 2), stepped = maxDiff (c, 100, (int) c.size(), 2);
        mcheck (smooth <= 0.01f * 0.3f && stepped > 0.01f * 0.3f && stepped > 1.6f * smooth, "click detector: a sine passes, a step is caught");
    }
    for (int mIdx = 0; mIdx < reg.size(); ++mIdx)
    {
        const auto& mi = reg.at (mIdx);
        if (mi.id == "sine") continue;
        const bool voice = isVoice (mi.id), ens = isEnsemble (mi.id);
        // exact pitch: no vibrato / scatter (voices); the plain articulation
        const auto pExact = paramsOf (mi, { { "m.vibrato", 0.0f }, { "m.detune", 0.0f }, { "m.motion", 0.0f } });
        const auto pDef = paramsOf (mi, {});

        // ---- pitch C2..C7 within +-5 cents (voices: C3..C7 - a voice sings a range like a sampled instrument and stays
        // silent at MIDI 24, 32.7 Hz: VoiceModels.cpp lowestNote, owner report item 0)
        juce::String pitchLine; float worst = 0.0f;
        for (int note = 24; note <= 84; note += 12)
        {
            const auto o = run (mi, pExact, note, 0.9f, 2.2, 2.0, sr);
            if (voice && note < 36)
            {
                mcheck (peakOf (o.l) < 1.0e-4f, mi.id + ": silent below its range (MIDI " + juce::String (note) + ", peak " + juce::String (peakOf (o.l), 5) + ")");
                pitchLine << "- ";
                continue;
            }
            const int from = (int) ((ens ? 0.9 : 0.3) * sr);
            const double f = measureHz (o.l, sr, from, (int) (1.2 * sr), hzOf (note));
            const float c = (float) (1200.0 * std::log2 (f / hzOf (note)));
            worst = std::max (worst, std::abs (c));
            pitchLine << juce::String (c, 1) << " ";
        }
        mcheck (worst <= 5.0f, mi.id + (voice ? ": pitch C3..C7 within 5 cents (" : ": pitch C2..C7 within 5 cents (") + pitchLine.trim() + ")");
        // other sample rates: C5 in tune and level within 3 dB of 48 kHz
        {
            const auto ref = run (mi, pExact, 60, 0.9f, 2.2, 2.0, sr);
            juce::String line; bool ok = true;
            for (double sr2 : { 44100.0, 96000.0 })
            {
                const auto o = run (mi, pExact, 60, 0.9f, 2.2, 2.0, sr2);
                const double f = measureHz (o.l, sr2, (int) ((ens ? 0.9 : 0.3) * sr2), (int) (1.2 * sr2), hzOf (60));
                const float c = (float) (1200.0 * std::log2 (f / hzOf (60)));
                const float dbDiff = juce::Decibels::gainToDecibels (peakOf (o.l) / std::max (1e-9f, peakOf (ref.l)));
                ok = ok && std::abs (c) <= 5.0f && std::abs (dbDiff) <= 3.0f;
                line << juce::String ((int) sr2) << " Hz: " << juce::String (c, 1) << " c / " << juce::String (dbDiff, 1) << " dB  ";
            }
            mcheck (ok, mi.id + ": other sample rates at C5 - " + line.trim());
        }
        if (ens)   // with the singers' natural scatter + vibrato: the section's centre stays within 12 cents
        {
            const auto o = run (mi, pDef, 60, 0.9f, 3.2, 3.0, sr);
            const double f = measureHz (o.l, sr, (int) (1.0 * sr), (int) (2.0 * sr), hzOf (60), true);
            const float c = (float) (1200.0 * std::log2 (f / hzOf (60)));
            mcheck (std::abs (c) <= 12.0f, mi.id + ": ensemble centre at C5 " + juce::String (c, 1) + " cents (scatter SD " + juce::String (pDef[13], 0) + " c)");
        }

        // ---- level at C5, velocity 1: below -1 dBFS (and audible)
        {
            const auto o = run (mi, pDef, 60, 1.0f, 2.0, 1.5, sr);
            const float pk = std::max (peakOf (o.l), peakOf (o.r));
            mcheck (pk < 0.89f && pk > 0.05f, mi.id + ": C5 velocity 1 peak " + juce::String (juce::Decibels::gainToDecibels (pk), 1) + " dBFS");
        }

        // ---- stability: long held notes at C2 / C5 / C7, default and extreme parameters; finite, bounded, no denormals
        {
            bool finite = true, denormal = false; float pk = 0.0f;
            auto scan = [&] (const Render& o)
            {
                for (auto* ch : { &o.l, &o.r })
                    for (float s : *ch)
                    {
                        if (! std::isfinite (s)) finite = false;
                        if (s != 0.0f && std::abs (s) < 1.0e-30f) denormal = true;
                        pk = std::max (pk, std::abs (s));
                    }
            };
            std::vector<std::vector<float>> sets { pDef };
            {
                // extremes: every parameter at its top, then at its bottom (level kept at 1)
                std::vector<float> hi, lo;
                for (auto& in : mi.params) { hi.push_back (in.id == "m.level" ? 1.0f : in.hi); lo.push_back (in.id == "m.level" ? 1.0f : in.lo); }
                sets.push_back (hi); sets.push_back (lo);
            }
            for (auto& ps : sets)
                for (int note : { 24, 60, 96 })
                    scan (run (mi, ps, note, 1.0f, 12.0, 10.0, sr));
            mcheck (finite && ! denormal && pk < 4.0f, mi.id + ": stable over 10 s held notes at C2 / C5 / C8 with default / max / min parameters (peak "
                                                  + juce::String (juce::Decibels::gainToDecibels (pk), 1) + " dBFS)");
        }

        // ---- clicks
        {
            // release: the step at note-off is not larger than the signal's own steps just before it
            const auto o = run (mi, pDef, 60, 0.9f, 2.5, 1.0, sr);
            const int off = (int) (1.0 * sr);
            const float before = maxDiff (o.l, off - (int) (0.02 * sr), off, 1), at = maxDiff (o.l, off, off + (int) (0.005 * sr), 1);
            const bool relOk = at <= 2.0f * before + 1.0e-4f;
            // the end of the voice: it stops only once silent (the last 64 samples are below -70 dBFS)
            bool endOk = true;
            if (o.activeUntil > 64) endOk = peakOf (o.l, o.activeUntil - 64, o.activeUntil) < 3.2e-4f;
            // onset: starts from silence (no step on the first sample)
            const float pk = peakOf (o.l);
            const bool onsetOk = std::abs (o.l[0]) < 0.5f * pk && std::abs (o.l[1]) < 0.7f * pk;
            mcheck (relOk && endOk && onsetOk, mi.id + ": no click at note-off (" + juce::String (at / std::max (1e-9f, before), 2) + "x the steps before), at the voice end"
                                               + (o.activeUntil > 0 ? juce::String (" (") + juce::String (o.activeUntil / sr, 2) + " s)" : juce::String (" (still sounding)"))
                                               + ", or at the onset");
            // bends: +-2 semitones of vibrato-like bending (sine 5 Hz) do not add steps over the plain note
            const auto plain = run (mi, pDef, 60, 0.9f, 1.5, 1.4, sr);
            const auto bent = run (mi, pDef, 60, 0.9f, 1.5, 1.4, sr, [sr] (int pos) { return 2.0f * std::sin (6.2831853f * 5.0f * (float) pos / (float) sr); });
            const int a = (int) (0.2 * sr), b = (int) (1.3 * sr);
            const float d2p = maxDiff (plain.l, a, b, 2), d2b = maxDiff (bent.l, a, b, 2);
            // a click is a step: its 2nd difference is a sizeable fraction of the signal. Smooth time-varying delay reads
            // only move partials (very dark tones can double a tiny 2nd difference without any step)
            const float pkB = peakOf (bent.l, a, b);
            mcheck (d2b <= std::max (1.6f * d2p, 0.01f * pkB) + 1.0e-4f, mi.id + ": bends +-2 semitones without clicks (2nd difference "
                    + juce::String (d2b / std::max (1e-9f, d2p), 2) + "x the plain note, " + juce::String (d2b / std::max (1e-9f, pkB), 4) + " of the peak)");
        }

        // ---- through the engine (as a preset part: voice pool, FX, limiter), 44.1 kHz
        {
            VaultPreset pr; pr.id = "t.m." + mi.id; pr.name = mi.name; pr.category = vault::catSynth;
            VaultPart part; part.kind = VaultPartKind::model; part.source = mi.id; part.label = mi.name;
            pr.parts.push_back (part);
            const auto b = VaultEngine::render (pr, 60, 0.8f, 2.0, 44100.0);
            bool finite = true;
            for (int c = 0; c < b.getNumChannels(); ++c)
                for (int i = 0; i < b.getNumSamples(); ++i) finite = finite && std::isfinite (b.getSample (c, i));
            const float pk = b.getMagnitude (0, b.getNumSamples());
            mcheck (finite && pk > 0.03f && pk < 0.99f, mi.id + ": plays through the engine as a preset part (peak "
                                                        + juce::String (juce::Decibels::gainToDecibels (pk), 1) + " dBFS)");
        }

        // ---- determinism and no allocation on the audio thread
        {
            const auto a = run (mi, pDef, 64, 0.7f, 0.5, 0.3, sr), b = run (mi, pDef, 64, 0.7f, 0.5, 0.3, sr);
            mcheck (a.l == b.l && a.r == b.r, mi.id + ": deterministic (same note + parameters = same samples)");
            auto v = mi.create();
            v->prepare (sr, 512);
            std::vector<float> l (128), r (128);
            allocHook::allocs = 0;
            allocHook::countAllocs = true;
            for (int rep = 0; rep < 3; ++rep)
            {
                v->setParams (pDef.data(), (int) pDef.size());
                v->noteOn (48 + rep * 7, 0.8f, hzOf (48 + rep * 7));
                for (int blk = 0; blk < 60; ++blk) { v->setPitch (hzOf (48 + rep * 7) * (1.0f + 0.001f * (float) blk)); v->render (l.data(), r.data(), 128); }
                v->noteOff (0.5f);
                for (int blk = 0; blk < 20; ++blk) v->render (l.data(), r.data(), 128);
                v->kill();
            }
            allocHook::countAllocs = false;
            mcheck (allocHook::allocs.load() == 0, mi.id + ": no allocation in noteOn / setParams / setPitch / render / noteOff / kill");
        }
    }
    std::printf ("model failures: %d\n", modelFails);
    return modelFails;
}
