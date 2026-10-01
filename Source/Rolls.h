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
} // namespace kk
