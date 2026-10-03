#pragma once
#include <array>
#include <cmath>
#include <vector>
#include <cstdint>
#include "DspUtil.h"

// v0.38 BEAT for EVOLVE ideas: a small built-in trap kit (808 kick, snare / clap, hats, open hat) and the beat genes.
// Only for hearing an idea inside the plugin - the beat goes to FL as MIDI (GM notes: 36 kick, 38 snare, 42 hat, 46 open hat).
namespace kk
{
struct BeatGenes { uint32_t kick = 0, hat = 0; float density = 0.5f, rolls = 0.4f; bool valid = false; };
struct BeatHit { float beat; int drum; float vel; };   // drum: 0 kick, 1 snare, 2 hat, 3 open hat
enum { tdKick, tdSnare, tdHat, tdOpenHat };

inline BeatGenes beatFromSeed (uint32_t seed)
{
    Rng r; r.seed (hash32 (seed ^ 0x6a09e667u));
    BeatGenes b; b.kick = r.next() | 1u; b.hat = r.next() | 1u; b.density = 0.3f + 0.5f * r.uni(); b.rolls = 0.2f + 0.6f * r.uni(); b.valid = true;
    return b;
}
inline BeatGenes mutateBeat (const BeatGenes& p, uint32_t seed, float wild)
{
    BeatGenes b = p.valid ? p : beatFromSeed (seed);
    Rng r; r.seed (hash32 (seed ^ 0xbb67ae85u));
    const float x = r.uni();
    if (x < 0.45f + 0.3f * wild) b.hat = r.next() | 1u;     // new hat rolls (the most "alive" part)
    if (x > 0.55f - 0.3f * wild) b.kick = r.next() | 1u;    // a new kick pattern
    b.density = std::clamp (b.density + (r.uni() * 2.0f - 1.0f) * (0.1f + 0.3f * wild), 0.15f, 0.95f);
    b.rolls = std::clamp (b.rolls + (r.uni() * 2.0f - 1.0f) * (0.1f + 0.4f * wild), 0.0f, 1.0f);
    b.valid = true;
    return b;
}
inline BeatGenes crossBeat (const BeatGenes& a, const BeatGenes& b, uint32_t seed)
{
    Rng r; r.seed (hash32 (seed ^ 0x3c6ef372u));
    BeatGenes c; c.valid = true;
    c.kick = r.uni() < 0.5f ? a.kick : b.kick; c.hat = r.uni() < 0.5f ? a.hat : b.hat;
    c.density = 0.5f * (a.density + b.density); c.rolls = 0.5f * (a.rolls + b.rolls);
    return c;
}

// trap at half time: snare / clap on beat 3 of every bar, an 808 kick pattern, hats in 1/8 with 1/16, triplet and 1/32 rolls
inline std::vector<BeatHit> beatHits (const BeatGenes& g, int bars)
{
    std::vector<BeatHit> out;
    Rng rk; rk.seed (hash32 (g.kick ^ 0x510e527fu));
    Rng rh; rh.seed (hash32 (g.hat ^ 0x9b05688cu));
    // one 2-bar kick idea, repeated with small changes
    static const float kickW[16] { 1.0f, 0.0f, 0.1f, 0.35f, 0.0f, 0.1f, 0.5f, 0.15f, 0.0f, 0.45f, 0.25f, 0.35f, 0.05f, 0.2f, 0.4f, 0.25f };
    std::array<std::array<bool, 16>, 2> kick {};
    for (int b = 0; b < 2; ++b)
        for (int s = 0; s < 16; ++s) kick[(size_t) b][(size_t) s] = (b == 0 && s == 0) || (s != 8 && rk.uni() < kickW[s] * (0.4f + 1.1f * g.density));
    for (int bar = 0; bar < bars; ++bar)
    {
        const float b0 = (float) bar * 4.0f;
        const auto& kp = kick[(size_t) (bar % 2)];
        for (int s = 0; s < 16; ++s) if (kp[(size_t) s]) out.push_back ({ b0 + (float) s * 0.25f, tdKick, s == 0 ? 1.0f : 0.85f });
        out.push_back ({ b0 + 2.0f, tdSnare, 1.0f });
        if (bar % 2 == 1 && rk.uni() < g.density * 0.6f) out.push_back ({ b0 + 3.75f, tdSnare, 0.55f });   // a ghost before the turn
        // hats: 1/8 grid, some beats rolled
        for (int e = 0; e < 8; ++e)
        {
            const float t = b0 + (float) e * 0.5f;
            const float roll = rh.uni();
            if (roll < g.rolls * 0.35f && e % 2 == 1)          // 1/32 roll over the 1/8
                for (int k = 0; k < 4; ++k) out.push_back ({ t + (float) k * 0.125f, tdHat, 0.55f + 0.1f * (float) k });
            else if (roll < g.rolls * 0.6f)                     // triplet 1/16
                for (int k = 0; k < 3; ++k) out.push_back ({ t + (float) k * (0.5f / 3.0f), tdHat, 0.6f + 0.1f * (float) k });
            else if (roll < g.rolls * 0.85f)                    // two 1/16
                { out.push_back ({ t, tdHat, 0.8f }); out.push_back ({ t + 0.25f, tdHat, 0.6f }); }
            else out.push_back ({ t, tdHat, e % 2 == 0 ? 0.85f : 0.65f });
        }
        if (rh.uni() < 0.3f + 0.3f * g.density) out.push_back ({ b0 + 3.5f, tdOpenHat, 0.7f });
    }
    std::sort (out.begin(), out.end(), [] (const BeatHit& a, const BeatHit& b) { return a.beat < b.beat; });
    return out;
}

class TrapDrums
{
public:
    void prepare (double rate) { sr = (float) rate; for (auto& v : voices) v.on = false; }
    void hit (int drum, float vel, int offset)
    {
        if (drum == tdHat || drum == tdOpenHat) for (auto& v : voices) if (v.on && v.drum == tdOpenHat) v.choke = true;   // a closed hat stops the open one
        if (drum == tdKick) for (auto& v : voices) if (v.on && v.drum == tdKick) v.choke = true;                          // 808s never stack
        Voice* f = nullptr;
        for (auto& v : voices) if (! v.on) { f = &v; break; }
        if (f == nullptr) { f = &voices[0]; for (auto& v : voices) if (v.t > f->t) f = &v; }
        *f = {}; f->on = true; f->drum = drum; f->vel = vel; f->delay = offset; f->rng.seed (seedCount++ * 2654435761u + 17u);
    }
    void allOff() { for (auto& v : voices) v.choke = true; }
    void render (float* L, float* R, int n, float gain)
    {
        const float dt = 1.0f / sr;
        for (auto& v : voices)
        {
            if (! v.on) continue;
            for (int i = 0; i < n; ++i)
            {
                if (v.delay > 0) { --v.delay; continue; }
                float y = 0, pan = 0;
                const float t = v.t;
                switch (v.drum)
                {
                    case tdKick:
                    {
                        const float f = 46.0f + 110.0f * std::exp (-t / 0.035f);
                        v.ph += f * dt; v.ph -= std::floor (v.ph);
                        y = std::sin (twoPi * v.ph) * std::exp (-t / 0.55f) * 0.95f;
                        y = std::tanh (y * 1.6f) * 0.8f + (t < 0.004f ? 0.3f * (1.0f - t / 0.004f) : 0.0f);
                        if (t > 1.6f) v.on = false;
                        break;
                    }
                    case tdSnare:
                    {
                        const float nz = v.rng.bi();
                        v.lp += (nz - v.lp) * 0.35f;
                        const float bright = nz - v.lp;
                        v.ph += 185.0f * dt; v.ph -= std::floor (v.ph);
                        // clap-ish: three quick bursts, then the body
                        const float bursts = t < 0.03f ? (std::fmod (t, 0.01f) < 0.004f ? 1.0f : 0.35f) : 1.0f;
                        y = (bright * 0.75f * std::exp (-t / 0.16f) + std::sin (twoPi * v.ph) * 0.45f * std::exp (-t / 0.05f)) * bursts * 0.75f;
                        if (t > 0.6f) v.on = false;
                        break;
                    }
                    default:   // hats
                    {
                        const float nz = v.rng.bi();
                        v.lp += (nz - v.lp) * 0.55f;
                        const float hp = nz - v.lp;
                        const float dec = v.drum == tdOpenHat ? 0.24f : 0.035f;
                        y = hp * std::exp (-t / dec) * (v.drum == tdOpenHat ? 0.32f : 0.4f);
                        pan = v.drum == tdOpenHat ? -0.2f : 0.25f;
                        if (t > dec * 7.0f) v.on = false;
                        break;
                    }
                }
                if (v.choke) { v.chokeGain -= dt / 0.012f; if (v.chokeGain <= 0.0f) { v.on = false; } y *= std::max (0.0f, v.chokeGain); }
                y *= v.vel * gain;
                L[i] += y * (1.0f - std::max (0.0f, pan)); R[i] += y * (1.0f + std::min (0.0f, pan));
                v.t += dt;
                if (! v.on) break;
            }
        }
    }
private:
    struct Voice { bool on = false, choke = false; int drum = 0, delay = 0; float t = 0, vel = 1, ph = 0, lp = 0, chokeGain = 1; Rng rng; };
    std::array<Voice, 16> voices {};
    float sr = 44100.0f;
    uint32_t seedCount = 1;
};
} // namespace kk
