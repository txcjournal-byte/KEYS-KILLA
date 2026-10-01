#pragma once
#include <array>
#include <cmath>
#include <vector>
#include <algorithm>
#include <juce_audio_basics/juce_audio_basics.h>
#include "DspUtil.h"
#include "modules/k808/Engine.h"

// v0.15 DRUMS: the 808 / SNARE / CLAP modules and the hats for ROLLS - all synthesised, no samples.
// 808 = sine with punch (pitch drop), tone (harmonics), real slides (legato notes glide) -> 808 Killa DSP.
namespace kk
{
enum DrumTarget { drum808, drumSnare, drumClap, drumHat, numDrums };

struct DrumParams
{
    float b8Tune = 0, b8Decay = 1.6f, b8Punch = 0.4f, b8Glide = 0.08f, b8Tone = 0.25f, b8Drive = 30, b8Clip = 3, b8Level = 0;
    int b8Sat = 1;
    float snTune = 0, snBody = 0.5f, snSnap = 0.6f, snDecay = 0.4f, snTone = 0.5f, snLevel = 0;
    float clTune = 0, clSpread = 0.5f, clDecay = 0.4f, clTone = 0.5f, clWidth = 0.5f, clLevel = 0;
    float htTune = 0, htDecay = 0.08f, htTone = 0.5f, htLevel = 0;
};

struct DrumEvent { int pos = 0; int target = 0; int note = 60; float vel = 0.8f; bool on = true; };

class Drums
{
public:
    static constexpr int maxEvents = 512;

    void prepare (double sampleRate, int maxBlock)
    {
        sr = (float) sampleRate;
        k808.prepare (sampleRate, std::max (32, maxBlock));
        b8Buf.setSize (2, std::max (32, maxBlock));
        peFast = std::exp (-1.0f / (0.012f * sr));
        held.reserve (128);
        reset();
    }
    void reset()
    {
        bass = {}; for (auto& v : snares) v = {}; for (auto& v : claps) v = {}; for (auto& v : hats) v = {};
        held.clear(); k808.reset(); nEvents = 0;
    }
    int latency808() const { return k808.getLatencySamples(); }

    // events must be added in time order for the current block (pos 0..n-1)
    void add (const DrumEvent& e) { if (nEvents < maxEvents) events[(size_t) nEvents++] = e; }
    void allOff() { held.clear(); if (bass.active) bass.release = true; }
    bool busy() const
    {
        if (bass.active) return true;
        for (auto& v : snares) if (v.active) return true;
        for (auto& v : claps) if (v.active) return true;
        for (auto& v : hats) if (v.active) return true;
        return false;
    }

    // adds the drums to L / R
    void process (float* L, float* R, int n, const DrumParams& p)
    {
        if (b8Buf.getNumSamples() < n) { nEvents = 0; return; }
        b8Buf.clear (0, n); b8Buf.clear (1, n);
        float* bl = b8Buf.getWritePointer (0);
        std::sort (events.begin(), events.begin() + nEvents, [] (const DrumEvent& a, const DrumEvent& b) { return a.pos < b.pos; });
        int ev = 0;
        bool any808 = bass.active;
        for (int i = 0; i < n; ++i)
        {
            while (ev < nEvents && events[(size_t) ev].pos <= i) { handle (events[(size_t) ev], p); any808 |= bass.active; ++ev; }
            if (bass.active) bl[i] = tick808 (p);
            float l = 0, r = 0;
            for (auto& v : snares) if (v.active) tickSnare (v, p, l, r);
            for (auto& v : claps) if (v.active) tickClap (v, p, l, r);
            for (auto& v : hats) if (v.active) tickHat (v, p, l, r);
            L[i] += l; R[i] += r;
        }
        while (ev < nEvents) handle (events[(size_t) ev++], p);
        nEvents = 0;
        if (any808 || b8Tail > 0)
        {
            b8Buf.copyFrom (1, 0, b8Buf, 0, 0, n);
            k808::EngineParams kp;
            kp.drive = p.b8Drive; kp.satMode = p.b8Sat; kp.clipDriveDb = p.b8Clip; kp.duckDepth = 0;
            kp.outputDb = p.b8Level; kp.focusDb = 2.0f + p.b8Drive * 0.03f;
            k808.process (b8Buf, 2, nullptr, kp);
            const float* ol = b8Buf.getReadPointer (0); const float* orr = b8Buf.getReadPointer (1);
            for (int i = 0; i < n; ++i) { L[i] += ol[i]; R[i] += orr[i]; }
            b8Tail = any808 ? (int) (sr * 0.3f) : std::max (0, b8Tail - n);   // let the 808 DSP ring out, then sleep
        }
    }

private:
    struct Bass { bool active = false, release = false; float phase = 0, logF = 0, logTarget = 0, amp = 0, gain = 0, pe = 0, vel = 1; int note = -1, attack = 0; };
    struct Snare { bool active = false; float p1 = 0, p2 = 0, pe = 0, body = 0, noise = 0, snap = 0, vel = 1, semi = 0; SvfState hp[2], lp[2]; };
    struct Clap { bool active = false; int t = 0; float vel = 1, semi = 0, tail = 0; SvfState bp[2]; };
    struct Hat { bool active = false; float env = 0, vel = 1, semi = 0; std::array<float, 6> ph {}; SvfState bp, hp; };

    void handle (const DrumEvent& e, const DrumParams& p)
    {
        switch (e.target)
        {
            case drum808:
                if (e.on) on808 (e.note, e.vel, p); else off808 (e.note);
                break;
            case drumSnare: if (e.on) { auto& v = pick (snares); v = {}; v.active = true; v.vel = e.vel; v.semi = (float) (e.note - 60); v.body = 1; v.noise = 1; v.snap = 1; v.pe = 1; } break;
            case drumClap:  if (e.on) { auto& v = pick (claps); v = {}; v.active = true; v.vel = e.vel; v.semi = (float) (e.note - 60); } break;
            case drumHat:   if (e.on) { auto& v = pick (hats); const auto keep = v.ph; v = {}; v.ph = keep; v.active = true; v.vel = e.vel; v.semi = (float) (e.note - 60); v.env = 1; } break;
            default: break;
        }
    }
    template <typename V, size_t N> V& pick (std::array<V, N>& pool)
    {
        for (auto& v : pool) if (! v.active) return v;
        return pool[(size_t) (rr++ % N)];
    }

    // ---------------- 808 ----------------
    void on808 (int note, float vel, const DrumParams& p)
    {
        const bool slide = bass.active && ! bass.release && ! held.empty() && p.b8Glide > 0.001f;
        held.erase (std::remove (held.begin(), held.end(), note), held.end());
        if (held.size() < 128) held.push_back (note);
        const float target = std::log2 (semisToHz ((float) note + p.b8Tune));
        bass.logTarget = target; bass.note = note;
        if (slide) return;                                   // legato: glide, no retrigger
        if (! bass.active) bass.phase = 0;
        bass.logF = target; bass.active = true; bass.release = false;
        bass.vel = 0.35f + 0.65f * vel; bass.pe = 1; bass.attack = (int) (sr * 0.0015f);
        bass.gain = bass.amp;                                // retrigger from the current level: no click
        bass.amp = 1;
    }
    void off808 (int note)
    {
        held.erase (std::remove (held.begin(), held.end(), note), held.end());
        if (! bass.active || note != bass.note) return;
        if (! held.empty()) { bass.note = held.back(); bass.logTarget = std::log2 (semisToHz ((float) bass.note + curTune)); return; }   // slide back
        bass.release = true;
    }
    float tick808 (const DrumParams& p)
    {
        curTune = p.b8Tune;
        const float glideT = std::max (0.004f, p.b8Glide);
        bass.logF += (bass.logTarget - bass.logF) * (1.0f - std::exp (-1.0f / (glideT * 0.35f * sr)));
        bass.pe *= std::exp (-1.0f / (0.035f * sr));
        const float f = std::exp2 (bass.logF) * (1.0f + p.b8Punch * 2.2f * bass.pe);
        bass.phase += f / sr; if (bass.phase >= 1) bass.phase -= 1;
        const float decay = std::max (0.08f, p.b8Decay);
        bass.amp *= std::exp (std::log (0.001f) / ((bass.release ? 0.045f : decay) * sr));
        float env = bass.amp;
        if (bass.attack > 0) { const float a = 1.0f - (float) bass.attack / (sr * 0.0015f); env = bass.gain + (bass.amp - bass.gain) * a; --bass.attack; }
        const float drv = 1.0f + p.b8Tone * 5.0f;
        float s = std::sin (twoPi * bass.phase);
        s = std::tanh (s * drv) / std::tanh (drv);
        if (bass.amp < 1.0e-4f) { bass.active = false; bass.amp = 0; }
        return s * env * bass.vel * 0.7f;
    }

    // ---------------- snare ----------------
    void tickSnare (Snare& v, const DrumParams& p, float& l, float& r)
    {
        const float t = std::exp2 ((p.snTune + v.semi) / 12.0f);
        v.pe *= peFast;
        v.p1 += 185.0f * t * (1.0f + 0.7f * v.pe) / sr; if (v.p1 >= 1) v.p1 -= 1;
        v.p2 += 330.0f * t * (1.0f + 0.5f * v.pe) / sr; if (v.p2 >= 1) v.p2 -= 1;
        const float dec = 0.05f + p.snDecay * 0.6f;
        v.body *= std::exp (-6.9f / ((0.04f + dec * 0.35f) * sr));
        v.noise *= std::exp (-6.9f / (dec * sr));
        v.snap *= std::exp (-6.9f / (0.006f * sr));
        SvfCoef hc, lc; hc.set (900.0f * t, 1.2f, sr); lc.set (2500.0f + p.snTone * 9000.0f, 1.0f, sr);
        const float body = (std::sin (twoPi * v.p1) + 0.6f * std::sin (twoPi * v.p2)) * v.body * (0.3f + 0.7f * p.snBody);
        float out[2];
        for (int c = 0; c < 2; ++c)
        {
            const float w = noise.bi();
            v.hp[c].tick (hc, w); v.lp[c].tick (lc, v.hp[c].hp);
            out[c] = body * 0.55f + v.lp[c].lp * v.noise * (0.35f + 0.8f * p.snSnap) + w * v.snap * 0.5f * p.snSnap;
        }
        const float g = v.vel * juce::Decibels::decibelsToGain (p.snLevel) * 0.6f;
        l += out[0] * g; r += (out[0] * 0.75f + out[1] * 0.25f) * g;
        if (v.noise < 1.0e-4f && v.body < 1.0e-4f) v.active = false;
    }

    // ---------------- clap ----------------
    void tickClap (Clap& v, const DrumParams& p, float& l, float& r)
    {
        const float t = std::exp2 ((p.clTune + v.semi) / 12.0f);
        const float sp = (0.004f + p.clSpread * 0.012f) * sr;
        SvfCoef bc; bc.set ((800.0f + p.clTone * 1400.0f) * t, 0.7f, sr);
        const float dec = 0.08f + p.clDecay * 0.7f;
        float out[2];
        for (int c = 0; c < 2; ++c)
        {
            const float tc = (float) v.t - (c == 1 ? p.clWidth * 0.0015f * sr : 0.0f);
            float env = 0;
            if (tc >= 0)
            {
                const int burst = (int) (tc / sp);
                if (burst < 3) env = std::exp (-(tc - (float) burst * sp) / (0.0035f * sr));
                else env = std::exp (-(tc - 3.0f * sp) / (dec / 6.9f * sr));
            }
            v.bp[c].tick (bc, noise.bi());
            out[c] = v.bp[c].bp * env * 1.6f;
        }
        ++v.t;
        const float g = v.vel * juce::Decibels::decibelsToGain (p.clLevel) * 0.8f;
        l += out[0] * g; r += out[1] * g;
        if ((float) v.t > 3.0f * sp + dec * sr + 0.01f * sr) v.active = false;
    }

    // ---------------- hats (808-style six square oscillators) ----------------
    void tickHat (Hat& v, const DrumParams& p, float& l, float& r)
    {
        static constexpr float f[6] { 205.3f, 304.4f, 369.6f, 522.7f, 540.0f, 800.0f };
        const float t = std::exp2 ((p.htTune + v.semi) / 12.0f) * 1.4f;
        float s = 0;
        for (size_t k = 0; k < 6; ++k) { v.ph[k] += f[k] * t / sr; if (v.ph[k] >= 1) v.ph[k] -= 1; s += v.ph[k] < 0.5f ? 1.0f : -1.0f; }
        s = s / 6.0f + noise.bi() * 0.35f;
        SvfCoef bc, hc; bc.set (std::min (sr * 0.42f, 10000.0f), 1.0f, sr); hc.set (5000.0f + p.htTone * 4000.0f, 1.2f, sr);
        v.bp.tick (bc, s); v.hp.tick (hc, v.bp.bp);
        v.env *= std::exp (-6.9f / (std::max (0.01f, p.htDecay) * sr));
        const float o = v.hp.hp * v.env * v.vel * juce::Decibels::decibelsToGain (p.htLevel) * 2.4f;
        l += o; r += o;
        if (v.env < 1.0e-4f) v.active = false;
    }

    float sr = 44100, curTune = 0;
    float peFast = 0.9981f;
    Bass bass;
    std::vector<int> held;
    std::array<Snare, 4> snares;
    std::array<Clap, 3> claps;
    std::array<Hat, 6> hats;
    std::array<DrumEvent, maxEvents> events;
    int nEvents = 0, b8Tail = 0;
    unsigned rr = 0;
    Rng noise;
    k808::Engine k808;
    juce::AudioBuffer<float> b8Buf;
};

// ---------------- ROLLS: trap hi-hat roll patterns ----------------
struct RollHit { double beat = 0; float vel = 0.8f; int semi = 0; double len = 0.1; };

// style 0 CLASSIC (Atlanta 1/8 + rolls), 1 TRIPLET, 2 DRILL, 3 CRAZY (1/64 bursts, pitch ramps)
inline std::vector<RollHit> makeRolls (uint32_t seed, int style, int bars, float density)
{
    Rng rng; rng.seed (hash32 (seed * 2654435761u + (uint32_t) style * 97u + 1u));
    std::vector<RollHit> out;
    density = std::clamp (density, 0.0f, 1.0f);
    // roll types: hits per 1/8 note
    static constexpr int rollHits[] { 2, 3, 4, 6, 8 };
    static constexpr float styleW[4][5] { { 3, 3, 3, 1, 0.5f }, { 1, 6, 1, 3, 0.5f }, { 1, 5, 1, 3, 1 }, { 1, 2, 3, 3, 4 } };
    auto pickRoll = [&]
    {
        float sum = 0; for (float w : styleW[style & 3]) sum += w;
        float x = rng.uni() * sum;
        for (int k = 0; k < 5; ++k) { x -= styleW[style & 3][k]; if (x <= 0) return rollHits[k]; }
        return 4;
    };
    const int cells = bars * 8;   // 1/8 notes
    int skipUntil = -1;
    for (int c = 0; c < cells; ++c)
    {
        const double b = c * 0.5;
        const int inBar = c % 8;
        const bool barEnd = inBar >= 6, phraseEnd = (c / 8) % 2 == 1 && barEnd;
        if (c < skipUntil) continue;
        float pRoll = (0.12f + 0.45f * density) * (barEnd ? 1.8f : inBar % 2 == 1 ? 1.0f : 0.55f) * (phraseEnd ? 1.4f : 1.0f);
        if (style == 3) pRoll *= 1.4f;
        if (inBar == 0) pRoll *= 0.2f;   // the 1 stays clean
        // drill: a few gaps in the base grid
        if (style == 2 && inBar % 4 == 3 && rng.uni() < 0.4f) continue;
        if (rng.uni() < pRoll)
        {
            const int per8 = pickRoll();
            const int span = (rng.uni() < 0.35f + 0.3f * density && c + 1 < cells) ? 2 : 1;   // 1/8 or a whole beat
            const int n = per8 * span;
            const double step = 0.5 / per8;
            const int velShape = (int) (rng.uni() * 4.0f);   // 0 up, 1 down, 2 flat, 3 accent first
            const int pitchShape = rng.uni() < (style == 3 ? 0.7f : 0.3f) ? (rng.uni() < 0.7f ? 1 : -1) : 0;
            const int pitchSpan = 2 + (int) (rng.uni() * 6.0f);
            for (int k = 0; k < n; ++k)
            {
                const float x = n > 1 ? (float) k / (float) (n - 1) : 0.0f;
                float v = velShape == 0 ? 0.45f + 0.5f * x : velShape == 1 ? 0.95f - 0.45f * x : velShape == 2 ? 0.7f : (k == 0 ? 0.95f : 0.55f);
                v = std::clamp (v + rng.bi() * 0.04f, 0.2f, 1.0f);
                const int semi = pitchShape == 0 ? 0 : (int) std::lround (pitchShape * pitchSpan * x);
                out.push_back ({ b + k * step, v, semi, step * 0.6 });
            }
            skipUntil = c + span;
            continue;
        }
        // base grid
        if (style == 1)   // 1/8 triplet feel base: two 1/16T + rest pattern stays readable
        {
            out.push_back ({ b, inBar % 2 == 0 ? 0.9f : 0.62f, 0, 0.1 });
        }
        else if (style == 3)
        {
            out.push_back ({ b, inBar % 2 == 0 ? 0.92f : 0.6f, 0, 0.1 });
            if (rng.uni() < 0.5f) out.push_back ({ b + 0.25, 0.45f, 0, 0.08 });
        }
        else
        {
            out.push_back ({ b, inBar % 2 == 0 ? 0.92f : 0.66f, 0, 0.12 });
        }
    }
    return out;
}
} // namespace kk
