#pragma once
#include <array>
#include <cmath>
#include <algorithm>
#include "DspUtil.h"

// v0.16 SOUND WORLD: one click colours the whole KEYS KILLA sound like a classic instrument family,
// plus the TRANCE GATE. Runs on the synth output before the FX rack, loudness-matched.
namespace kk
{
enum World { worldOff, worldRompler, worldFat, worldGlass, worldShine, worldOrganic, worldVelocity, worldDrift, worldMix, numWorlds };

class WorldStage
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& d : cho) d.prepare ((int) (0.04f * sr) + 8);
        gAtk = 1.0f - std::exp (-1.0f / (0.001f * sr)); gRel = 1.0f - std::exp (-1.0f / (0.003f * sr));
        reset();
    }
    void reset()
    {
        for (auto& d : cho) d.clear();
        for (auto& s : st) s = {};
        inEnv = outEnv = 1.0e-4f; gateEnv = 1; chPh = 0; drPh = 0; drA = drB = 0; drT = 0;
    }

    // world 0..8, amount 0..1, gate 0 = off / 1..5 patterns, beat = host beat of sample 0, bps = beats per sample
    void process (float* L, float* R, int n, int world, float amount, int gate, float gateDepth, double beat, double bps)
    {
        if ((world == worldOff || amount <= 0.001f) && gate == 0) { lastWorld = 0; return; }
        if (world != lastWorld) { for (auto& s : st) s = {}; lastWorld = world; }
        const float a = std::clamp (amount, 0.0f, 1.0f);
        const float slow = std::exp (-1.0f / (0.3f * sr));
        for (int i = 0; i < n; ++i)
        {
            float x[2] { L[i], R[i] };
            const float dry[2] { x[0], x[1] };
            if (world != worldOff && a > 0.001f)
            {
                for (int c = 0; c < 2; ++c) x[c] = shape (world, c, x[c]);
                spatial (world, x);
                // loudness match: the world changes the colour, not the level
                const float li = 0.5f * (dry[0] * dry[0] + dry[1] * dry[1]), lo = 0.5f * (x[0] * x[0] + x[1] * x[1]);
                inEnv = li + slow * (inEnv - li); outEnv = lo + slow * (outEnv - lo);
                const float g = std::clamp (std::sqrt ((inEnv + 1.0e-9f) / (outEnv + 1.0e-9f)), 0.25f, 2.0f);
                for (int c = 0; c < 2; ++c) x[c] = dry[c] + (x[c] * g - dry[c]) * a;
            }
            if (gate > 0)
            {
                const double b = beat + bps * i;
                const float target = gateOpen (gate, b) ? 1.0f : 1.0f - std::clamp (gateDepth, 0.0f, 1.0f);
                gateEnv += (target - gateEnv) * (target > gateEnv ? gAtk : gRel);
                x[0] *= gateEnv; x[1] *= gateEnv;
            }
            L[i] = x[0]; R[i] = x[1];
        }
    }

    static const char* name (int w)
    {
        static const char* n[] { "OFF", "ROMPLER 90", "FAT ANALOG", "GLASS SQUASH", "HI-FI SHINE", "ORGANIC", "VELOCITY DEEP", "DRIFT ANALOG", "MIX READY" };
        return n[std::clamp (w, 0, (int) numWorlds - 1)];
    }

private:
    struct Ch
    {
        float hp1 = 0, hp2 = 0, lp1 = 0, lp2 = 0, lp3 = 0, env = 0, envB[3] {}, dc = 0, dcx = 0;
        SvfState peak, noiseBp;
    };
    static float onePole (float& z, float x, float hz, float srate) { const float k = std::exp (-twoPi * hz / srate); z = x + k * (z - x); return z; }
    float follow (float& e, float x, float atkMs, float relMs) const
    {
        const float ax = std::abs (x);
        const float k = std::exp (-1.0f / ((ax > e ? atkMs : relMs) * 0.001f * sr));
        e = ax + k * (e - ax); return e;
    }
    float comp (float& e, float x, float thresh, float ratio, float atkMs, float relMs) const
    {
        const float lev = follow (e, x, atkMs, relMs);
        return lev > thresh ? x * std::pow (lev / thresh, 1.0f / ratio - 1.0f) : x;
    }

    float shape (int world, int c, float x)
    {
        auto& s = st[(size_t) c];
        switch (world)
        {
            case worldRompler:   // 90s PCM rompler: no sub mud, hard mids, squeezed
            {
                x -= onePole (s.hp1, x, 150.0f, sr); x -= onePole (s.hp2, x, 150.0f, sr);
                SvfCoef pc; pc.set (2800.0f, 1.2f, sr); s.peak.tick (pc, x); x += s.peak.bp * 1.1f;
                x = std::round (x * 2048.0f) / 2048.0f;   // 12-bit PCM grain
                return onePole (s.lp1, comp (s.env, x * 1.8f, 0.15f, 6.0f, 2.0f, 60.0f), 9000.0f, sr);
            }
            case worldFat: // fat analog: warm low end, driven, a little darker
            {
                const float low = onePole (s.lp1, x, 160.0f, sr);
                x += low * 0.9f;
                x = std::tanh (x * 3.5f) / 3.5f * 1.3f;
                return onePole (s.lp2, x, 5000.0f, sr);
            }
            case worldGlass: // three-band upward / downward squash, asymmetric grit
            {
                const float lo = onePole (s.lp1, x, 220.0f, sr), rest = x - lo;
                const float mid = onePole (s.lp2, rest, 2500.0f, sr), hi = rest - mid;
                auto ott = [this] (float& e, float v, float boost)
                {
                    const float lev = follow (e, v, 3.0f, 90.0f);
                    const float g = std::clamp (std::pow (0.22f / (lev + 1.0e-4f), 0.55f), 0.3f, 5.0f);
                    return v * g * boost;
                };
                float y = ott (s.envB[0], lo, 0.9f) + ott (s.envB[1], mid, 1.1f) + ott (s.envB[2], hi, 1.6f);
                y = y - 0.2f * y * y + 0.05f * y * y * y;
                const float o = y - s.dcx + 0.995f * s.dc; s.dcx = y; s.dc = o;   // the asymmetry makes DC
                return o;
            }
            case worldShine:  // modern hi-fi: clean, shiny top
            {
                const float hi = x - onePole (s.lp1, x, 7000.0f, sr);
                return x + hi * 1.0f;
            }
            case worldOrganic: // organic / foley: breath noise that follows the note, soft top
            {
                const float e = follow (s.env, x, 5.0f, 200.0f);
                SvfCoef nc; nc.set (5500.0f, 1.0f, sr); s.noiseBp.tick (nc, rng.bi());
                return onePole (s.lp1, x, 7000.0f, sr) + s.noiseBp.bp * e * 0.8f;
            }
            case worldVelocity: // deep multisample feel: soft = felt and dark, hard = open and saturated
            {
                const float e = follow (s.env, x, 4.0f, 150.0f);
                const float bright = std::clamp (e * 3.0f, 0.0f, 1.0f);
                const float lowp = onePole (s.lp1, x, 1500.0f + 9000.0f * bright, sr);
                const float y = lowp + (x - lowp) * (0.25f + 0.75f * bright);
                return y + (std::tanh (y * 2.0f) / 2.0f - y) * bright * 0.6f;
            }
            case worldDrift: // component analog: pre-filter drive (the drift is in spatial())
            {
                const float y = std::tanh (x * 3.0f) / 3.0f * 1.8f;
                return onePole (s.lp1, y, 6500.0f, sr);
            }
            case worldMix: // mix-ready rompler: glued, bright, finished
            {
                float y = comp (s.env, x * 1.6f, 0.15f, 5.0f, 3.0f, 120.0f);
                const float hi = y - onePole (s.lp1, y, 5000.0f, sr);
                return y + hi * 0.7f;
            }
            default: return x;
        }
    }

    // chorus (HI-FI SHINE, ORGANIC shimmer) and per-channel analog drift (DRIFT, FAT ANALOG)
    void spatial (int world, float* x)
    {
        const bool chorus = world == worldShine || world == worldOrganic;
        const bool drift = world == worldDrift || world == worldFat;
        if (! chorus && ! drift) return;
        float rate = 0.6f, base = 6.0f, depth = 3.5f, mix = 0.8f;
        if (world == worldOrganic) { rate = 0.13f; base = 14.0f; depth = 5.0f; mix = 0.4f; }
        if (drift)
        {
            // smoothed sample & hold wander: a few cents of pitch drift, different per side
            drPh += 0.35f / sr;
            if (drPh >= 1) { drPh -= 1; drA = drB; drB = rng.bi(); }
            drT = drA + (drB - drA) * (0.5f - 0.5f * std::cos (pi * drPh));
            rate = 0; base = 3.0f; depth = world == worldDrift ? 3.0f : 1.6f; mix = 1.0f;
        }
        chPh += rate / sr; if (chPh >= 1) chPh -= 1;
        for (int c = 0; c < 2; ++c)
        {
            const float m = drift ? drT * (c == 0 ? 1.0f : -0.8f) : std::sin (twoPi * (chPh + (c == 0 ? 0.0f : 0.5f)));
            cho[(size_t) c].push (x[c]);
            const float wet = cho[(size_t) c].read ((base + depth * m) * 0.001f * sr);
            x[c] = drift ? wet : x[c] * (1.0f - mix * 0.5f) + wet * mix;
        }
    }

    static bool gateOpen (int gate, double beat)
    {
        // 16 steps per bar (1/16) gate patterns
        static constexpr uint16_t pat[6] { 0xffff, 0x5555, 0xffff, 0x0000, 0xb6db, 0xed5a };
        const double b = std::max (0.0, beat);
        if (gate == 1) return std::fmod (b, 0.5) < 0.25;                     // 1/8
        if (gate == 2) return std::fmod (b, 0.25) < 0.125;                   // 1/16
        if (gate == 3) return std::fmod (b, 1.0 / 3.0) < 1.0 / 6.0;          // 1/8 triplets
        const int step = (int) std::floor (b * 4.0) % 16;
        const double frac = b * 4.0 - std::floor (b * 4.0);
        return ((pat[(size_t) std::clamp (gate, 0, 5)] >> step) & 1) && frac < 0.75;
    }

    float sr = 44100;
    float gAtk = 0.5f, gRel = 0.01f;
    std::array<Ch, 2> st;
    std::array<DelayLine, 2> cho;
    float inEnv = 1.0e-4f, outEnv = 1.0e-4f, gateEnv = 1, chPh = 0, drPh = 0, drA = 0, drB = 0, drT = 0;
    int lastWorld = 0;
    Rng rng;
};
} // namespace kk
