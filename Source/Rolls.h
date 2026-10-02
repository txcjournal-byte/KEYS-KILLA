#pragma once
#include <vector>
#include <algorithm>
#include <cmath>
#include "DspUtil.h"

// ROLLS: trap hi-hat roll MIDI generator (drag the pattern into FL onto your own hi-hat sample)
namespace kk
{
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
// ---------------- 808 patterns: trap bass lines in the key, with slides (overlapping notes glide) ----------------
// style 0 ATLANTA (root heavy, bouncy), 1 DRILL (sliding, syncopated), 2 PLUGG (sparse, long), 3 RAGE (busy, octave jumps)
inline std::vector<RollHit> make808 (uint32_t seed, int style, int bars, float density)
{
    Rng rng; rng.seed (hash32 (seed * 40503u + (uint32_t) style * 131u + 7u));
    std::vector<RollHit> out;
    density = std::clamp (density, 0.0f, 1.0f);
    static constexpr int minor[] { 0, 3, 5, 7, 10, 12, -2, -5 };
    const int stepsPerBar = 16;
    // a one-bar rhythm idea, varied per bar (16th grid); trap 808s follow the kick: 1, the "and" of 2, 3 ...
    static constexpr float base[4][16] {
        { 1, 0, 0, .3f, 0, 0, .6f, 0, .5f, 0, .4f, 0, 0, .5f, 0, 0 },
        { 1, 0, 0, .6f, 0, .3f, 0, .6f, 0, 0, .7f, 0, .4f, 0, .5f, 0 },
        { 1, 0, 0, 0, 0, 0, 0, 0, .5f, 0, 0, 0, 0, 0, .3f, 0 },
        { 1, 0, .5f, 0, .6f, 0, .5f, .3f, .7f, 0, .5f, 0, .6f, .4f, .5f, .3f } };
    const auto& pat = base[style & 3];
    int prevSemi = 0;
    for (int bar = 0; bar < bars; ++bar)
        for (int st = 0; st < stepsPerBar; ++st)
        {
            float p = pat[st] * (0.55f + 0.9f * density);
            if (st == 0 && bar % 2 == 0) p = 1.0f;
            if (rng.uni() >= p) continue;
            int semi = 0;
            const float r = rng.uni();
            if (st != 0) semi = r < 0.5f ? 0 : minor[(int) (rng.uni() * (style == 3 ? 8.0f : 6.0f)) % 8];
            if (bar % 4 == 3 && st >= 12) semi = minor[(int) (rng.uni() * 5.0f)];   // turnaround at the phrase end
            // length: to the next hit-ish; drill / plugg ring longer
            int len = 1 + (int) (rng.uni() * (style == 2 ? 6.0f : 3.0f));
            const double beat = (bar * stepsPerBar + st) * 0.25;
            const bool slide = (style == 1 || style == 3) && semi != prevSemi && rng.uni() < 0.45f && ! out.empty();
            if (slide) out.back().len = beat - out.back().beat + 0.12;   // overlap = 808 glide
            out.push_back ({ beat, 0.8f + 0.2f * rng.uni(), semi, len * 0.25 - 0.02 });
            prevSemi = semi;
        }
    // never past the end
    const double end = bars * 4.0;
    for (auto& h : out) h.len = std::min (h.len, end - h.beat - 0.01);
    return out;
}

// ---------------- snare / clap: backbeat + trap rolls and flams ----------------
// style 0 TRAP (3 + fills), 1 TRIPLET ROLLS, 2 DRILL (late snare + ghost notes), 3 BUILD-UP (rolls ramp up, pitch climbs)
inline std::vector<RollHit> makeSnares (uint32_t seed, int style, int bars, float density)
{
    Rng rng; rng.seed (hash32 (seed * 92821u + (uint32_t) style * 17u + 3u));
    std::vector<RollHit> out;
    density = std::clamp (density, 0.0f, 1.0f);
    static constexpr double rollStarts[] { 3.0, 3.5, 3.25, 2.5, 3.75 };
    for (int bar = 0; bar < bars; ++bar)
    {
        const double b0 = bar * 4.0;
        const bool phraseEnd = bar % 2 == 1 || bars == 1;
        // backbeat: the "3" of trap half-time (drill: a 16th late sometimes)
        const double back = b0 + (style == 2 && rng.uni() < 0.5f ? 2.25 : 2.0);
        out.push_back ({ back, 0.95f, 0, 0.2 });
        // ghost snares on 16ths (drill and dense patterns more)
        const float pGhost = (style == 2 ? 0.22f : 0.08f) + 0.16f * density;
        for (int st = 0; st < 16; ++st)
        {
            const double t = b0 + st * 0.25;
            if (std::abs (t - back) < 0.3 || st >= 12) continue;
            if (rng.uni() < pGhost) out.push_back ({ t, 0.25f + 0.2f * rng.uni(), 0, 0.1 });
        }
        // flam before the backbeat
        if (rng.uni() < 0.2f + 0.3f * density) out.push_back ({ back - 0.0625, 0.45f, 0, 0.05 });
        // rolls into the next bar / phrase
        if (phraseEnd || rng.uni() < 0.25f + 0.35f * density)
        {
            static constexpr int rates[4][4] { { 4, 4, 8, 6 }, { 3, 6, 6, 12 }, { 4, 6, 8, 3 }, { 8, 8, 12, 16 } };
            const int per = rates[style & 3][(int) (rng.uni() * 3.999f)];   // hits per beat
            const double start = b0 + (style == 3 ? (rng.uni() < 0.5f ? 2.5 : 3.0) : rollStarts[(int) (rng.uni() * 4.999f)]);
            const double endB = b0 + 4.0;
            const int n = std::max (1, (int) std::ceil ((endB - start) * per - 1.0e-6));   // every hit stays inside the bar
            const int velShape = (int) (rng.uni() * 3.999f);   // 0 crescendo, 1 decrescendo, 2 accent every 3, 3 flat hard
            const int pitchDir = style == 3 ? 1 : rng.uni() < 0.35f ? (rng.uni() < 0.6f ? 1 : -1) : 0;
            const int pitchSpan = style == 3 ? 7 : 2 + (int) (rng.uni() * 5.0f);
            for (int k = 0; k < n; ++k)
            {
                const float x = n > 1 ? (float) k / (float) (n - 1) : 1.0f;
                float v = velShape == 0 ? 0.35f + 0.6f * x : velShape == 1 ? 0.95f - 0.5f * x : velShape == 2 ? (k % 3 == 0 ? 0.95f : 0.5f) : 0.85f;
                v = std::clamp (v + rng.bi() * 0.04f, 0.15f, 1.0f);
                const int semi = (int) std::lround (pitchDir * pitchSpan * x);
                out.push_back ({ start + k / (double) per, v, semi, 0.5 / per });
            }
        }
    }
    std::sort (out.begin(), out.end(), [] (const RollHit& a, const RollHit& c) { return a.beat < c.beat; });
    return out;
}
// ---------------- v0.32: kick, open hat, perc and FX patterns (every drum page can generate + edit) ----------------
// kick: style 0 TRAP (1 + syncopated 16ths), 1 DRILL (sliding, late), 2 BOUNCE (busy, plugg), 3 HALF-TIME (sparse, heavy)
inline std::vector<RollHit> makeKicks (uint32_t seed, int style, int bars, float density)
{
    Rng rng; rng.seed (hash32 (seed * 7919u + (uint32_t) style * 61u + 11u));
    std::vector<RollHit> out;
    density = std::clamp (density, 0.0f, 1.0f);
    static constexpr float base[4][16] {
        { 1, 0, 0, .25f, 0, 0, .55f, 0, 0, 0, .5f, 0, 0, .35f, 0, .2f },
        { 1, 0, 0, .5f, 0, 0, 0, .55f, 0, 0, .45f, 0, 0, 0, .5f, 0 },
        { 1, 0, .4f, 0, 0, .45f, 0, .4f, .5f, 0, .4f, 0, .45f, 0, .35f, .3f },
        { 1, 0, 0, 0, 0, 0, 0, .3f, 0, 0, .35f, 0, 0, 0, 0, 0 } };
    const auto& pat = base[style & 3];
    for (int bar = 0; bar < bars; ++bar)
        for (int st = 0; st < 16; ++st)
        {
            float pr = pat[st] * (0.5f + 0.9f * density);
            if (st == 0) pr = 1.0f;
            if (st == 8 && style != 3) pr *= 0.2f;   // the snare's beat stays clear
            if (rng.uni() >= pr) continue;
            out.push_back ({ (bar * 16 + st) * 0.25, st == 0 ? 1.0f : 0.75f + 0.2f * rng.uni(), 0, 0.2 });
        }
    for (auto& h : out) h.len = std::min (h.len, bars * 4.0 - h.beat - 0.01);   // nothing rings past the end
    return out;
}
// open hat / crash: style 0 OFFBEAT (the "ands"), 1 SPARSE (end of phrases), 2 SYNCOPATED, 3 BUSY
inline std::vector<RollHit> makeOpenHats (uint32_t seed, int style, int bars, float density)
{
    Rng rng; rng.seed (hash32 (seed * 6007u + (uint32_t) style * 43u + 5u));
    std::vector<RollHit> out;
    density = std::clamp (density, 0.0f, 1.0f);
    for (int bar = 0; bar < bars; ++bar)
        for (int st = 0; st < 16; ++st)
        {
            float pr = 0;
            switch (style & 3)
            {
                case 0:  pr = st % 4 == 2 ? 0.35f + 0.6f * density : 0.0f; break;
                case 1:  pr = (bar % 2 == 1 && st == 14) || (bar == 0 && st == 0) ? 0.9f : st % 8 == 6 ? 0.1f * density : 0.0f; break;
                case 2:  pr = (st == 3 || st == 7 || st == 11 || st == 14) ? 0.25f + 0.5f * density : 0.0f; break;
                default: pr = st % 2 == 0 ? 0.15f + 0.5f * density : 0.08f * density; break;
            }
            if (rng.uni() >= pr) continue;
            out.push_back ({ (bar * 16 + st) * 0.25, 0.6f + 0.35f * rng.uni(), 0, 0.5 });
        }
    for (auto& h : out) h.len = std::min (h.len, bars * 4.0 - h.beat - 0.01);   // nothing rings past the end
    return out;
}
// percussion: style 0 RIMS (ghost 16ths), 1 TRIPLET, 2 BOUNCE (3-3-2), 3 SHAKER (steady 16ths, accents) - pitch moves like toms / congas
inline std::vector<RollHit> makePercs (uint32_t seed, int style, int bars, float density)
{
    Rng rng; rng.seed (hash32 (seed * 4513u + (uint32_t) style * 29u + 13u));
    std::vector<RollHit> out;
    density = std::clamp (density, 0.0f, 1.0f);
    static constexpr int tones[] { 0, 0, 3, 5, 7, -2, 12 };
    for (int bar = 0; bar < bars; ++bar)
    {
        const double b0 = bar * 4.0;
        if ((style & 3) == 1)   // triplet 8ths
        {
            for (int k = 0; k < 12; ++k)
                if (rng.uni() < 0.2f + 0.55f * density)
                    out.push_back ({ b0 + k / 3.0, k % 3 == 0 ? 0.85f : 0.55f, tones[(int) (rng.uni() * 6.99f)], 0.15 });
            continue;
        }
        for (int st = 0; st < 16; ++st)
        {
            float pr = 0; float v = 0.6f;
            switch (style & 3)
            {
                case 0:  pr = (st % 4 == 3 ? 0.45f : st % 2 == 1 ? 0.2f : 0.05f) * (0.5f + density); v = 0.5f + 0.35f * rng.uni(); break;
                case 2:  pr = (st == 0 || st == 3 || st == 6 || st == 8 || st == 11 || st == 14) ? 0.55f + 0.4f * density : 0.05f; v = st % 8 == 0 ? 0.9f : 0.65f; break;
                default: pr = 0.6f + 0.4f * density; v = st % 4 == 2 ? 0.85f : st % 2 == 0 ? 0.6f : 0.38f; break;
            }
            if (rng.uni() >= std::min (1.0f, pr)) continue;
            const int semi = (style & 3) == 3 ? 0 : tones[(int) (rng.uni() * 6.99f)];
            out.push_back ({ b0 + st * 0.25, v, semi, 0.12 });
        }
    }
    for (auto& h : out) h.len = std::min (h.len, bars * 4.0 - h.beat - 0.01);   // nothing rings past the end
    return out;
}
// FX: style 0 INTRO (an impact on the 1), 1 PHRASE (every 2 bars), 2 RISER (into the next phrase), 3 STUTTER (fast repeats at the end)
inline std::vector<RollHit> makeFxHits (uint32_t seed, int style, int bars, float density)
{
    Rng rng; rng.seed (hash32 (seed * 3001u + (uint32_t) style * 19u + 17u));
    std::vector<RollHit> out;
    density = std::clamp (density, 0.0f, 1.0f);
    const double end = bars * 4.0;
    switch (style & 3)
    {
        case 0:  out.push_back ({ 0.0, 1.0f, 0, 2.0 }); if (density > 0.5f && bars > 1) out.push_back ({ end - 4.0, 0.8f, 0, 2.0 }); break;
        case 1:  for (int b = 0; b < bars; b += 2) out.push_back ({ b * 4.0, 0.9f, (int) (rng.uni() * 2.0f) * 12 - 0, 1.5 }); break;
        case 2:  out.push_back ({ std::max (0.0, end - 2.0), 0.9f, 0, 1.95 }); break;
        default:
        {
            const int n = 4 + (int) (density * 8.0f);
            for (int k = 0; k < n; ++k) out.push_back ({ end - 1.0 + k / (double) n, 0.5f + 0.5f * (float) k / (float) n, k * 12 / n, 0.5 / n });
            break;
        }
    }
    for (auto& h : out) h.len = std::min (h.len, end - h.beat - 0.01);
    return out;
}
} // namespace kk
