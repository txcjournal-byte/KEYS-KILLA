#pragma once
#include <array>
#include <cstdint>
#include <vector>
#include "DspUtil.h"

// BREED LOOPS: every sound in the lab carries a melody loop, inherited like its sound.
// The recipe comes from the loops producers picked in testing (tools/trap_loop_recipe.py):
//   bar 1: low root held; an 8th riff opens root-5th-b6-5th, then falls towards the dark b2
//   bar 2: the bass drops (b6 / b7 / III); the riff opens the same, climbs, ends on a longer high note
//   bars 1-2 x4: bar 4 ends on a high jump, bar 8 falls b3-2-b2 over the 5th below
// One line mixes both registers, like melody-pack MIDI. Semitones are relative to the key root.
namespace kk
{
enum LoopGene { loopStart, loopFall, loopClimb, loopJump, loopBass, numLoopGenes };

struct LoopGenes
{
    std::array<int, numLoopGenes> g {};
    int key = 0;              // 0 = C ... 11 = B (minor)
    bool valid = false;
    bool operator== (const LoopGenes& o) const { return g == o.g && key == o.key && valid == o.valid; }
};

struct LoopNote { float start, len; int note; bool low; };   // beats

namespace loopdata
{
    inline constexpr int open[4][4]  { { 12, 7, 8, 7 }, { 12, 7, 8, 10 }, { 12, 8, 7, 8 }, { 15, 12, 7, 8 } };
    inline constexpr int fall[5][4]  { { 12, 7, 3, 1 }, { 3, 2, 1, 3 }, { 7, 3, 2, 1 }, { 8, 7, 3, 1 }, { 12, 10, 8, 7 } };
    inline constexpr int climb[4][4][2] { { { 10, 2 }, { 15, 4 }, { 13, 2 }, { 12, 2 } }, { { 10, 2 }, { 12, 4 }, { 15, 2 }, { 14, 2 } },
                                          { { 10, 2 }, { 15, 4 }, { 17, 2 }, { 15, 2 } }, { { 12, 2 }, { 15, 4 }, { 13, 2 }, { 12, 2 } } };
    inline constexpr int jump[4] { 19, 24, 20, 17 };
    inline constexpr int bass[3] { -4, -2, -9 };               // b6, b7, relative major
    inline constexpr int size[numLoopGenes] { 4, 5, 4, 4, 3 };
}

inline const char* loopGeneName (int g)
{
    static const char* n[] { "START", "FALL", "CLIMB", "JUMP", "BASS" };
    return n[g < 0 || g >= numLoopGenes ? 0 : g];
}

inline LoopGenes loopFromSeed (uint32_t seed)
{
    LoopGenes l;
    uint32_t h = hash32 (seed ^ 0x51ed270bu);
    for (int i = 0; i < numLoopGenes; ++i) { h = hash32 (h + (uint32_t) i * 0x9e3779b9u); l.g[(size_t) i] = (int) (h % (uint32_t) loopdata::size[i]); }
    l.key = (int) (hash32 (h ^ 0xabcdu) % 12u);
    l.valid = true;
    return l;
}

// child loop: every loop gene from one parent (leanA = chance of parent A), WILD mutates genes
inline LoopGenes crossLoops (const LoopGenes& a, const LoopGenes& b, uint32_t seed, float leanA, float wild)
{
    if (! a.valid) return b.valid ? b : loopFromSeed (seed);
    if (! b.valid) return a;
    Rng r; r.seed (seed ^ 0x2545f491u);
    LoopGenes c; c.valid = true;
    bool fromA = false, fromB = false;
    for (int i = 0; i < numLoopGenes; ++i)
    {
        const bool useA = r.uni() < leanA;
        c.g[(size_t) i] = useA ? a.g[(size_t) i] : b.g[(size_t) i];
        (useA ? fromA : fromB) = true;
        if (r.uni() < 0.06f + wild * 0.45f) c.g[(size_t) i] = (int) (r.uni() * (float) loopdata::size[i]) % loopdata::size[i];
    }
    if (! fromA || ! fromB) { const int i = (int) (r.uni() * numLoopGenes) % numLoopGenes; c.g[(size_t) i] = fromA ? b.g[(size_t) i] : a.g[(size_t) i]; }
    c.key = r.uni() < leanA ? a.key : b.key;
    return c;
}

// notes of the loop in beats. keyOverride >= 0 replaces the loop's own key.
// bassOnly: bass sounds play the low line; riffOnly: mono leads play just the riff.
inline std::vector<LoopNote> buildLoop (const LoopGenes& l, int keyOverride, int bars, bool bassOnly, bool riffOnly)
{
    using namespace loopdata;
    std::vector<LoopNote> out;
    if (! l.valid) return out;
    const int key = keyOverride >= 0 ? keyOverride % 12 : l.key;
    const int hi = key <= 4 ? 60 + key : 48 + key;
    const int lo = key <= 4 ? 36 + key : 24 + key;
    const int b2 = bass[l.g[loopBass] % 3];
    out.reserve (140);
    for (int half = 0; half < (bars > 8 ? 2 : 1); ++half)
    {
        const int* op = open[l.g[loopStart] % 4];
        const int* fl = fall[(l.g[loopFall] + half * 2) % 5];     // the second 8 bars answer with another fall
        const auto& cl = climb[l.g[loopClimb] % 4];
        const int pk = jump[(l.g[loopJump] + half) % 4];
        auto n = [&] (int bar, int step, int len, int semi, bool low)
        {
            if ((low && riffOnly) || (! low && bassOnly)) return;
            const float s = (float) ((half * 8 + bar) * 16 + step) * 0.25f;
            out.push_back ({ s, (float) len * 0.25f - 0.02f, (low ? lo : hi) + semi, low });
        };
        for (int rep = 0; rep < 4; ++rep)
        {
            const int b = rep * 2;
            n (b, 0, 16, 0, true);
            for (int i = 0; i < 4; ++i) n (b, 2 * i, 2, op[i], false);
            for (int i = 0; i < 4; ++i) n (b, 8 + 2 * i, 2, fl[i], false);
            if (rep == 3)   // bar 8: turnaround
            {
                n (b + 1, 0, 8, b2, true); n (b + 1, 8, 8, -5, true);
                for (int i = 0; i < 4; ++i) n (b + 1, 2 * i, 2, op[i], false);
                n (b + 1, 8, 2, 3, false); n (b + 1, 10, 2, 2, false); n (b + 1, 12, 4, 1, false);
                continue;
            }
            n (b + 1, 0, 16, b2, true);
            for (int i = 0; i < 3; ++i) n (b + 1, 2 * i, 2, op[i], false);
            int t = 6;
            const int climbN = rep == 1 ? 2 : 4;
            for (int i = 0; i < climbN; ++i) { n (b + 1, t, cl[i][1], cl[i][0], false); t += cl[i][1]; }
            if (rep == 1) n (b + 1, t, 16 - t, pk, false);
        }
    }
    return out;
}

inline const char* keyName (int k)
{
    static const char* n[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return n[((k % 12) + 12) % 12];
}
} // namespace kk
