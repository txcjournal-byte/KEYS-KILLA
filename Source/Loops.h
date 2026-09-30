#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <vector>
#include "DspUtil.h"

// BREED LOOPS: every melody is composed new - no phrase library, no template.
// The rules come from the trap loops producers picked (and melody-pack MIDI):
//   - a small palette of 4-6 pitches from the minor key (root, 5th, b3, b6, b7, octave, sometimes b2 / 9th)
//   - one anchor note the line keeps returning to
//   - a 1-bar rhythm drawn on the 16th grid with trap accents; bar 2 is a close mutation of it
//   - a melodic shape (falling run, anchor alternation, arch, pedal + answer, zig-zag)
//   - notes on beats 1 and 3 are chord tones of the bar's chord
//   - 2-bar motif, repeated A A' A A'' over 8 bars; the last bar walks back to the start
// Every value comes from 32-bit seeds, so there are billions of different melodies, and every BREED rolls new ones.
// One track mixes the low line and the riff, like melody-pack MIDI.
namespace kk
{
enum LoopGene { loopRhythm, loopPalette, loopShape, loopHarmony, loopDensity, loopBassRhythm, loopRegister, loopVariation, numLoopGenes };

struct LoopGenes
{
    std::array<uint32_t, numLoopGenes> g {};
    int key = 0;              // 0 = C ... 11 = B (minor)
    bool valid = false;
    bool operator== (const LoopGenes& o) const { return g == o.g && key == o.key && valid == o.valid; }
};

struct LoopNote { float start, len; int note; bool low; };   // beats

namespace loopdata
{
    // gene sizes (0 = free 32-bit seed)
    inline constexpr uint32_t size[numLoopGenes] { 0, 0, 5, 8, 3, 3, 3, 0 };
    // chord plans: bass of bar 2 (semitones from the root) and of the last bar
    struct Plan { int bar2, ending; };
    inline constexpr Plan plans[8] { { -4, -5 }, { -2, -5 }, { 5, -2 }, { -9, -2 }, { 0, -5 }, { -5, -2 }, { -4, -2 }, { 1, -5 } };
    inline bool inMinor (int s) { static const bool m[12] { true, false, true, true, false, true, false, true, true, false, true, false }; return m[((s % 12) + 12) % 12]; }
    // chord tones (triad of the natural minor, the b2 chord as major) for a bass note s semitones from the root
    inline bool chordTone (int s, int chordRoot)
    {
        const int r = ((chordRoot % 12) + 12) % 12, x = ((s % 12) + 12) % 12;
        const int third = inMinor (r + 4) ? 4 : 3;
        return x == r || x == (r + third) % 12 || x == (r + 7) % 12;
    }
}

inline const char* loopGeneName (int g)
{
    static const char* n[] { "RHYTHM", "NOTES", "SHAPE", "CHORDS", "DENSITY", "BASS", "REGISTER", "VARIATION" };
    return n[g < 0 || g >= numLoopGenes ? 0 : g];
}

inline uint32_t randomGene (int gene, Rng& r) { return loopdata::size[gene] == 0 ? (r.next() | 1u) : r.next() % loopdata::size[gene]; }

inline void fixLoop (LoopGenes& l) { for (int i = 0; i < numLoopGenes; ++i) if (loopdata::size[i] > 0) l.g[(size_t) i] %= loopdata::size[i]; }

inline LoopGenes loopFromSeed (uint32_t seed)
{
    LoopGenes l;
    Rng r; r.seed (hash32 (seed ^ 0x51ed270bu));
    for (int i = 0; i < numLoopGenes; ++i) l.g[(size_t) i] = randomGene (i, r);
    if (r.uni() < 0.6f) l.g[loopRegister] = 0;
    l.key = (int) (r.next() % 12u);
    l.valid = true;
    return l;
}

// a new melody from the sources: each gene from one of them, WILD mutates; always a new tune
inline LoopGenes mixLoops (const std::vector<LoopGenes>& from, uint32_t seed, float wild)
{
    Rng r; r.seed (seed ^ 0x2545f491u);
    std::vector<const LoopGenes*> src;
    for (auto& f : from) if (f.valid) src.push_back (&f);
    if (src.empty()) return loopFromSeed (seed);
    LoopGenes c;
    for (int i = 0; i < numLoopGenes; ++i)
    {
        c.g[(size_t) i] = src[r.next() % src.size()]->g[(size_t) i];
        if (r.uni() < 0.2f + wild * 0.5f) c.g[(size_t) i] = randomGene (i, r);
    }
    const float x = r.uni();
    if (x < 0.7f) c.g[loopRhythm] = randomGene (loopRhythm, r);
    if (x > 0.3f) c.g[loopPalette] = randomGene (loopPalette, r);
    c.g[loopVariation] = randomGene (loopVariation, r);
    c.key = src[r.next() % src.size()]->key;
    c.valid = true;
    fixLoop (c);
    return c;
}

inline LoopGenes crossLoops (const LoopGenes& a, const LoopGenes& b, uint32_t seed, float leanA, float wild)
{
    Rng r; r.seed (seed ^ 0x9e3779b9u);
    std::vector<LoopGenes> from;
    if (a.valid) from.push_back (a);
    if (b.valid) from.push_back (b);
    if (from.size() == 2 && r.uni() < std::abs (leanA - 0.5f)) from.erase (from.begin() + (leanA > 0.5f ? 1 : 0));
    return mixLoops (from, seed, wild);
}

// NEW MELODY: same key, chords and bass - new rhythm, notes, shape
inline LoopGenes rerollLoop (const LoopGenes& l, uint32_t seed)
{
    LoopGenes c = l.valid ? l : loopFromSeed (seed);
    Rng r; r.seed (hash32 (seed ^ 0x7f4a7c15u));
    for (int i : { (int) loopRhythm, (int) loopPalette, (int) loopShape, (int) loopDensity, (int) loopVariation }) c.g[(size_t) i] = randomGene (i, r);
    fixLoop (c);
    return c;
}

namespace loopgen
{
    struct N { int st, ln, s; };   // 16ths within a bar, semitones from the root

    inline std::vector<int> palette (uint32_t seed, int& anchor)
    {
        Rng r; r.seed (hash32 (seed ^ 0xa511e9b3u));
        struct P { int s; float w; };
        static const P pool[] { { 7, 0.9f }, { 3, 0.8f }, { 12, 0.85f }, { 8, 0.6f }, { 10, 0.5f }, { 15, 0.5f }, { 5, 0.35f },
                                { 2, 0.3f }, { 19, 0.3f }, { 14, 0.25f }, { 17, 0.2f }, { 20, 0.2f }, { 1, 0.12f } };
        std::vector<int> pal { 0 };
        const int want = 4 + (int) (r.next() % 3u);   // 4..6 pitches
        for (int guard = 0; (int) pal.size() < want && guard < 200; ++guard)
        {
            const auto& p = pool[r.next() % (sizeof (pool) / sizeof (pool[0]))];
            if (r.uni() < p.w && std::find (pal.begin(), pal.end(), p.s) == pal.end()) pal.push_back (p.s);
        }
        static const int anchors[] { 12, 7, 15, 12, 0 };
        anchor = anchors[r.next() % 5u];
        if (std::find (pal.begin(), pal.end(), anchor) == pal.end()) pal.push_back (anchor);
        std::sort (pal.begin(), pal.end());
        return pal;
    }

    inline std::vector<std::pair<int, int>> rhythm (uint32_t seed, int density)
    {
        Rng r; r.seed (hash32 (seed ^ 0x3c6ef372u));
        static const float w[16] { 1.0f, 0.2f, 0.45f, 0.65f, 0.55f, 0.2f, 0.65f, 0.35f, 0.9f, 0.2f, 0.55f, 0.6f, 0.6f, 0.25f, 0.55f, 0.35f };
        static const int lo[3] { 3, 5, 8 }, hi[3] { 5, 8, 12 };
        const int d = std::clamp (density, 0, 2);
        const int target = lo[d] + (int) (r.next() % (uint32_t) (hi[d] - lo[d] + 1));
        bool on[16] {};
        on[0] = r.uni() < 0.88f;
        int count = on[0] ? 1 : 0;
        for (int guard = 0; count < target && guard < 400; ++guard)
        {
            const int st = (int) (r.next() % 16u);
            if (! on[st] && r.uni() < w[st]) { on[st] = true; ++count; }
        }
        const int style = (int) (r.next() % 3u);   // 0 legato, 1 mixed, 2 staccato
        std::vector<std::pair<int, int>> out;
        for (int st = 0; st < 16; ++st)
        {
            if (! on[st]) continue;
            int next = st + 1; while (next < 16 && ! on[next]) ++next;
            int len = std::min (next - st, style == 2 ? 1 : 4);
            if (style == 1 && r.uni() < 0.4f) len = std::max (1, len / 2);
            out.push_back ({ st, len });
        }
        return out;
    }

    // bar 2 answers bar 1: move, add or drop one or two hits
    inline std::vector<std::pair<int, int>> mutate (std::vector<std::pair<int, int>> rh, Rng& r)
    {
        const int moves = 1 + (int) (r.next() % 2u);
        for (int m = 0; m < moves && ! rh.empty(); ++m)
        {
            const float x = r.uni();
            const size_t i = r.next() % rh.size();
            if (x < 0.4f && rh.size() > 2 && rh[i].first != 0) rh.erase (rh.begin() + (long) i);
            else if (x < 0.75f)
            {
                const int st = (int) (r.next() % 16u);
                bool taken = false; for (auto& h : rh) taken |= h.first == st;
                if (! taken) rh.push_back ({ st, 2 });
            }
            else if (rh[i].first != 0 && rh[i].first < 15) rh[i].first += 1;
        }
        std::sort (rh.begin(), rh.end());
        rh.erase (std::unique (rh.begin(), rh.end(), [] (const auto& a, const auto& b) { return a.first == b.first; }), rh.end());
        for (size_t i = 0; i < rh.size(); ++i)
        {
            const int next = i + 1 < rh.size() ? rh[i + 1].first : 16;
            rh[i].second = std::clamp (rh[i].second, 1, next - rh[i].first);
        }
        return rh;
    }

    inline int nearestChordTone (const std::vector<int>& pal, int s, int chordRoot)
    {
        int best = -1, bd = 99;
        for (int p : pal) if (loopdata::chordTone (p, chordRoot) && std::abs (p - s) < bd) { bd = std::abs (p - s); best = p; }
        if (best >= 0) return best;
        for (int d = 0; d < 12; ++d)   // the palette has no tone of this chord: nearest one in the key
            for (int sg : { -1, 1 }) { const int c = s + sg * d; if (loopdata::chordTone (c, chordRoot) && loopdata::inMinor (c)) return c; }
        return s;
    }

    inline std::vector<N> shapeBar (const std::vector<std::pair<int, int>>& rh, const std::vector<int>& pal, int anchor, int shape,
                                   int startIdx, int chordRoot, Rng& r)
    {
        std::vector<N> out;
        const int P = (int) pal.size();
        int idx = std::clamp (startIdx, 0, P - 1);
        int dir = shape == 2 ? 1 : -1;
        const int anchorIdx = (int) (std::find (pal.begin(), pal.end(), anchor) - pal.begin());
        for (size_t i = 0; i < rh.size(); ++i)
        {
            int s;
            switch (shape)
            {
                case 0:   // falling run that bounces back up
                    s = pal[(size_t) idx]; idx -= 1 + (r.uni() < 0.2f ? 1 : 0); if (idx < 0) idx = P - 1 - (int) (r.next() % 2u); break;
                case 1:   // anchor alternation: anchor, move, anchor, move...
                    if (i % 2 == 0) s = anchor;
                    else { s = pal[(size_t) idx]; idx += dir; if (idx < 0 || idx >= P) { dir = -dir; idx = std::clamp (idx, 0, P - 1); } }
                    break;
                case 2:   // arch: up to the middle, then down
                    s = pal[(size_t) idx]; if (i + 1 == rh.size() / 2) dir = -1; idx = std::clamp (idx + dir, 0, P - 1); break;
                case 3:   // pedal on the anchor, the last notes answer
                    s = i + 2 >= rh.size() ? pal[(size_t) std::clamp (anchorIdx + (i + 1 == rh.size() ? -2 : -1), 0, P - 1)] : anchor; break;
                default:  // zig-zag
                    s = pal[(size_t) idx]; idx = std::clamp (idx + ((i % 2 == 0) ? 2 : -1) * dir, 0, P - 1); if (idx == 0 || idx == P - 1) dir = -dir; break;
            }
            if (rh[i].first == 0 || rh[i].first == 8) s = nearestChordTone (pal, s, chordRoot);   // beats 1 and 3
            out.push_back ({ rh[i].first, rh[i].second, s });
        }
        for (size_t i = 2; i < out.size(); ++i)   // no three equal notes in a row (except the pedal shape)
            if (shape != 3 && out[i].s == out[i - 1].s && out[i].s == out[i - 2].s && out[i].st != 0 && out[i].st != 8)
            {
                const int k = (int) (std::find (pal.begin(), pal.end(), out[i].s) - pal.begin());
                out[i].s = pal[(size_t) std::clamp (k + (k > 0 ? -1 : 1), 0, P - 1)];
            }
        return out;
    }
}

// notes of the loop in beats. keyOverride >= 0 replaces the loop's own key.
// bassOnly: bass sounds play the low line; riffOnly: mono leads play just the melody.
inline std::vector<LoopNote> buildLoop (const LoopGenes& l, int keyOverride, int bars, bool bassOnly, bool riffOnly)
{
    using namespace loopgen;
    std::vector<LoopNote> out;
    if (! l.valid) return out;
    bars = bars > 8 ? 16 : 8;
    const int key = keyOverride >= 0 ? keyOverride % 12 : l.key;
    int hi = key <= 4 ? 60 + key : 48 + key;
    const int lo = key <= 4 ? 36 + key : 24 + key;
    const int reg = (int) (l.g[loopRegister] % 3);
    if (reg == 1 && hi + 12 <= 72) hi += 12;
    if (reg == 2 && hi - 12 >= 50) hi -= 12;
    const auto plan = loopdata::plans[l.g[loopHarmony] % 8];
    const int shape = (int) (l.g[loopShape] % 5);
    int anchor = 12;
    const auto pal = palette (l.g[loopPalette], anchor);
    const int P = (int) pal.size();
    Rng r; r.seed (hash32 (l.g[loopVariation] ^ 0x85ebca6bu));
    const auto r1 = rhythm (l.g[loopRhythm], (int) (l.g[loopDensity] % 3));
    const auto r2 = mutate (r1, r);
    const int anchorIdx = (int) (std::find (pal.begin(), pal.end(), anchor) - pal.begin());
    const int start = r.uni() < 0.6f ? P - 1 - (int) (r.next() % 2u) : anchorIdx;
    const auto barA = shapeBar (r1, pal, anchor, shape, start, 0, r);
    const auto barB = shapeBar (r2, pal, anchor, shape, std::clamp (start + (r.uni() < 0.5f ? 1 : -1), 0, P - 1), plan.bar2, r);
    auto lift = [&] (std::vector<N> b)   // bar 4: the answer reaches up
    {
        if (b.size() >= 2) { b.back().s = pal.back() < 15 ? pal.back() + 12 : pal.back(); b[b.size() - 2].s = pal[(size_t) std::max (0, P - 2)]; }
        return b;
    };
    auto ending = [&] (std::vector<N> b)   // bar 8: walks down to a tone of the last chord
    {
        int k = P - 1;
        for (auto& n : b) { n.s = pal[(size_t) k]; k = std::max (0, k - 1); }
        if (! b.empty()) b.back().s = nearestChordTone (pal, b.back().s, plan.ending);
        return b;
    };
    auto varyA = [&] (std::vector<N> b)   // bars 5 / 9-16: one note of the motif changes
    {
        if (b.size() >= 3)
        {
            auto& n = b[1 + r.next() % (b.size() - 2)];
            if (n.st != 0 && n.st != 8) n.s = pal[(size_t) (r.next() % (uint32_t) P)];
        }
        return b;
    };
    const auto barB2 = lift (barB), barEnd = ending (r2 == r1 ? barB : shapeBar (r2, pal, anchor, 0, P - 1, plan.ending, r)), barA2 = varyA (barA);
    const int bassStyle = (int) (l.g[loopBassRhythm] % 3);
    out.reserve (240);
    for (int bar = 0; bar < bars; ++bar)
    {
        const int pos = bar % 8;
        const std::vector<N>* b = &barA;
        int bassSemi = 0;
        if (pos % 2 == 1) { b = pos == 7 ? &barEnd : pos == 3 ? &barB2 : &barB; bassSemi = pos == 7 ? plan.ending : plan.bar2; }
        else if (pos == 4 || bar >= 8) b = &barA2;
        const float b0 = (float) bar * 4.0f;
        if (! riffOnly)
        {
            if (bassStyle == 0) out.push_back ({ b0, 3.97f, lo + bassSemi, true });
            else if (bassStyle == 2) for (int q = 0; q < 4; ++q) out.push_back ({ b0 + (float) q, 0.45f, lo + bassSemi + (q == 2 ? 12 : 0), true });
            else for (auto [st, ln] : { std::pair<int, int> { 0, 6 }, { 6, 4 }, { 10, 2 }, { 12, 4 } })
                out.push_back ({ b0 + (float) st * 0.25f, (float) ln * 0.25f - 0.03f, lo + bassSemi, true });
        }
        if (! bassOnly)
            for (auto& n : *b) out.push_back ({ b0 + (float) n.st * 0.25f, (float) n.ln * 0.25f - 0.02f, hi + n.s, false });
    }
    // keep the melody in the middle of the keyboard (C4..C5 area, the register gene shifts it a little)
    float mean = 0; int cnt = 0;
    for (auto& n : out) if (! n.low) { mean += (float) n.note; ++cnt; }
    if (cnt > 0)
    {
        const float target = reg == 1 ? 72.0f : reg == 2 ? 62.0f : 67.0f;
        const int sh = 12 * (int) std::lround ((target - mean / (float) cnt) / 12.0f);
        for (auto& n : out) if (! n.low) n.note = std::clamp (n.note + sh, 40, 96);
    }
    std::sort (out.begin(), out.end(), [] (const LoopNote& x, const LoopNote& y) { return x.start != y.start ? x.start < y.start : x.note < y.note; });
    return out;
}

inline const char* keyName (int k)
{
    static const char* n[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return n[((k % 12) + 12) % 12];
}
} // namespace kk
