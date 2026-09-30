#pragma once
#include <array>
#include <cstdint>
#include <utility>
#include <vector>
#include "DspUtil.h"

// BREED LOOPS: every sound in the lab carries a melody loop, inherited like its sound.
// A combinatorial trap melody generator - no fixed template:
//   scale (5) x key (12) x rhythm of bar 1 (24) x rhythm of bar 2 (24) x chord progression (12 x 2 speeds)
//   x bass line (5) x texture (doubling 4 x follow / pedal 2 x note length 3) x form (6) x a 32-bit melody seed.
// Musical rules keep it trap: minor-family scales, chord tones on strong beats, mostly steps with octave jumps,
// a 2-bar motif that repeats and answers itself (A A B A ...), the loop ends on the root or the 5th.
// One track mixes the low line and the riff, like melody-pack MIDI. Every child gets a new melody seed.
namespace kk
{
enum LoopGene { loopScale, loopRhythmA, loopRhythmB, loopMelody, loopHarmony, loopBass, loopTexture, loopForm, numLoopGenes };

struct LoopGenes
{
    std::array<uint32_t, numLoopGenes> g {};
    int key = 0;              // 0 = C ... 11 = B
    bool valid = false;
    bool operator== (const LoopGenes& o) const { return g == o.g && key == o.key && valid == o.valid; }
};

struct LoopNote { float start, len; int note; bool low; };   // beats

namespace loopdata
{
    // gene sizes (0 = free 32-bit value)
    inline constexpr uint32_t size[numLoopGenes] { 5, 24, 24, 0, 24, 5, 24, 6 };
    inline constexpr int scales[5][7] { { 0, 2, 3, 5, 7, 8, 10 },    // natural minor
                                        { 0, 2, 3, 5, 7, 8, 11 },    // harmonic minor
                                        { 0, 1, 3, 5, 7, 8, 10 },    // phrygian
                                        { 0, 2, 3, 5, 7, 9, 10 },    // dorian
                                        { 0, 1, 4, 5, 7, 8, 10 } };  // phrygian dominant
    inline const char* scaleName (int s) { static const char* n[] { "MIN", "HARM MIN", "PHRYG", "DORIAN", "PHRYG DOM" }; return n[s % 5]; }
    inline constexpr int progs[12][4] { { 0, 0, 0, 0 }, { 0, 5, 0, 5 }, { 0, 5, 2, 6 }, { 0, 5, 3, 4 }, { 0, 3, 5, 4 }, { 0, 1, 0, 1 },
                                        { 0, 6, 5, 6 }, { 0, 0, 5, 6 }, { 0, 3, 0, 5 }, { 0, 4, 5, 3 }, { 5, 6, 0, 0 }, { 0, 2, 5, 6 } };
    constexpr int TPB = 48;   // ticks per bar: 16ths = 3, 8th triplets = 4

    using Cell = std::vector<std::pair<int, int>>;   // (start, length) in ticks
    inline const std::vector<Cell>& cells()
    {
        static const std::vector<Cell> c = []
        {
            std::vector<Cell> v;
            auto s = [&v] (std::initializer_list<std::pair<int, int>> l) { Cell x; for (auto p : l) x.push_back ({ p.first * 3, p.second * 3 }); v.push_back (x); };
            auto t = [&v] (std::initializer_list<std::pair<int, int>> l) { Cell x; for (auto p : l) x.push_back ({ p.first * 4, p.second * 4 }); v.push_back (x); };
            s ({ { 0, 2 }, { 2, 2 }, { 4, 2 }, { 6, 2 }, { 8, 2 }, { 10, 2 }, { 12, 2 }, { 14, 2 } });           // straight 8ths
            s ({ { 0, 3 }, { 3, 3 }, { 6, 2 }, { 8, 3 }, { 11, 3 }, { 14, 2 } });                               // dotted bounce
            s ({ { 0, 6 }, { 6, 10 } });                                                                        // long drone notes
            s ({ { 0, 2 }, { 3, 2 }, { 6, 2 }, { 8, 4 }, { 14, 2 } });                                          // syncopated
            s ({ { 0, 1 }, { 1, 1 }, { 2, 2 }, { 4, 4 }, { 8, 1 }, { 9, 1 }, { 10, 2 }, { 12, 4 } });           // pickup runs
            s ({ { 0, 4 }, { 8, 6 }, { 14, 2 } });                                                              // sparse
            s ({ { 0, 2 }, { 2, 2 }, { 4, 4 }, { 12, 4 } });                                                    // call with a rest
            s ({ { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 }, { 7, 1 }, { 8, 8 } });   // 16th roll + hold
            s ({ { 2, 2 }, { 6, 2 }, { 10, 2 }, { 14, 2 } });                                                   // offbeats
            s ({ { 0, 3 }, { 3, 3 }, { 6, 3 }, { 9, 3 }, { 12, 2 }, { 14, 2 } });                               // 3-3-3-3-2-2
            s ({ { 0, 6 }, { 6, 2 }, { 8, 6 }, { 14, 2 } });                                                    // long-short
            s ({ { 4, 2 }, { 6, 2 }, { 8, 8 } });                                                               // late entry
            t ({ { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 }, { 7, 1 }, { 8, 1 }, { 9, 1 }, { 10, 1 }, { 11, 1 } }); // triplet 8ths
            t ({ { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 1 }, { 5, 1 }, { 6, 6 } });                     // triplet run + hold
            t ({ { 0, 1 }, { 1, 1 }, { 2, 1 }, { 4, 1 }, { 5, 1 }, { 6, 1 }, { 7, 1 }, { 8, 1 }, { 10, 2 } });// triplet gallop
            s ({ { 0, 4 }, { 4, 4 }, { 8, 4 }, { 12, 4 } });                                                    // quarters
            s ({ { 0, 8 }, { 8, 8 } });                                                                         // halves
            s ({ { 0, 16 } });                                                                                  // whole note
            s ({ { 0, 3 }, { 3, 1 }, { 4, 2 }, { 6, 3 }, { 9, 1 }, { 10, 2 }, { 12, 4 } });                    // skip rhythm
            s ({ { 0, 1 }, { 1, 1 }, { 2, 1 }, { 3, 1 }, { 4, 4 }, { 8, 1 }, { 9, 1 }, { 10, 1 }, { 11, 1 }, { 12, 4 } }); // stutter
            s ({ { 0, 2 }, { 2, 2 }, { 4, 2 }, { 7, 5 }, { 12, 4 } });                                          // anticipation
            s ({ { 2, 2 }, { 4, 2 }, { 6, 4 }, { 10, 2 }, { 12, 4 } });                                         // rest start
            s ({ { 0, 2 }, { 3, 2 }, { 6, 2 }, { 10, 2 }, { 12, 2 }, { 14, 2 } });                              // bell pattern
            s ({ { 0, 6 }, { 6, 6 }, { 12, 4 } });                                                              // dotted quarters
            return v;
        }();
        return c;
    }
}

inline const char* loopGeneName (int g)
{
    static const char* n[] { "SCALE", "RHYTHM 1", "RHYTHM 2", "MELODY", "CHORDS", "BASS", "TEXTURE", "FORM" };
    return n[g < 0 || g >= numLoopGenes ? 0 : g];
}

inline uint32_t randomGene (int gene, Rng& r)
{
    if (loopdata::size[gene] == 0) return r.next() | 1u;
    if (gene == loopScale)   // natural minor most, then harmonic minor / phrygian
    {
        const float x = r.uni();
        return x < 0.45f ? 0u : x < 0.67f ? 1u : x < 0.85f ? 2u : x < 0.94f ? 3u : 4u;
    }
    return r.next() % loopdata::size[gene];
}

inline LoopGenes loopFromSeed (uint32_t seed)
{
    LoopGenes l;
    Rng r; r.seed (hash32 (seed ^ 0x51ed270bu));
    for (int i = 0; i < numLoopGenes; ++i) l.g[(size_t) i] = randomGene (i, r);
    l.key = (int) (r.next() % 12u);
    l.valid = true;
    return l;
}

// child loop: rhythm, scale, chords, bass, texture, form each from one parent; WILD mutates them.
// The melody seed is always new, so every child plays a different melody in its family's style.
inline LoopGenes crossLoops (const LoopGenes& a, const LoopGenes& b, uint32_t seed, float leanA, float wild)
{
    if (! a.valid) return b.valid ? crossLoops (b, b, seed, 1.0f, wild) : loopFromSeed (seed);
    if (! b.valid) return crossLoops (a, a, seed, 1.0f, wild);
    Rng r; r.seed (seed ^ 0x2545f491u);
    LoopGenes c; c.valid = true;
    for (int i = 0; i < numLoopGenes; ++i)
    {
        c.g[(size_t) i] = r.uni() < leanA ? a.g[(size_t) i] : b.g[(size_t) i];
        if (i != loopMelody && r.uni() < 0.12f + wild * 0.5f) c.g[(size_t) i] = randomGene (i, r);
    }
    c.g[loopMelody] = hash32 (a.g[loopMelody] * 31u + b.g[loopMelody] + seed) | 1u;
    c.key = r.uni() < leanA ? a.key : b.key;
    return c;
}

// NEW MELODY: same key, scale, chords and bass - new rhythms, form and melody
inline LoopGenes rerollLoop (const LoopGenes& l, uint32_t seed)
{
    LoopGenes c = l.valid ? l : loopFromSeed (seed);
    Rng r; r.seed (hash32 (seed ^ 0x7f4a7c15u));
    for (int i : { (int) loopRhythmA, (int) loopRhythmB, (int) loopMelody, (int) loopForm, (int) loopTexture }) c.g[(size_t) i] = randomGene (i, r);
    return c;
}

// notes of the loop in beats. keyOverride >= 0 replaces the loop's own key.
// bassOnly: bass sounds play the low line; riffOnly: mono leads play just the melody.
inline std::vector<LoopNote> buildLoop (const LoopGenes& l, int keyOverride, int bars, bool bassOnly, bool riffOnly)
{
    using namespace loopdata;
    std::vector<LoopNote> out;
    if (! l.valid) return out;
    bars = bars > 8 ? 16 : 8;
    const int key = keyOverride >= 0 ? keyOverride % 12 : l.key;
    const int* sc = scales[l.g[loopScale] % 5];
    const int* prog = progs[l.g[loopHarmony] % 12];
    const int chordBars = (l.g[loopHarmony] / 12) % 2 == 0 ? 2 : 1;
    const int tex = (int) (l.g[loopTexture] % 24);
    const int doubling = tex / 6, follow = (tex / 3) % 2, gateSel = tex % 3;
    const int form = (int) (l.g[loopForm] % 6);
    const int bassStyle = (int) (l.g[loopBass] % 5);
    const int root = 57 + ((key - 9 + 12) % 12);     // A3 .. G#4
    auto pitch = [&] (int deg, int base)
    {
        const int o = deg >= 0 ? deg / 7 : -((-deg + 6) / 7);
        return base + 12 * o + sc[deg - 7 * o];
    };
    auto chordAt = [&] (int bar) { return prog[(bar / chordBars) % 4]; };
    auto snapChord = [] (int deg, int chord)   // nearest chord tone (triad + 7th)
    {
        int best = deg, bd = 99;
        for (int o = -14; o <= 14; o += 7)
            for (int k : { 0, 2, 4, 6 }) { const int c = chord + k + o; const int d = std::abs (c - deg); if (d < bd || (d == bd && c < best)) { bd = d; best = c; } }
        return best;
    };
    Rng r; r.seed (hash32 (l.g[loopMelody] ^ 0x9e3779b9u));

    // ---- the 2-bar motif: rhythm cells + a guided random walk on scale degrees
    struct MN { int tick, len, deg; };
    auto makeMotif = [&] (int cellA, int cellB, int startBar)
    {
        std::vector<MN> m;
        const auto& cs = cells();
        const int n = (int) cs.size();
        cellA %= n; cellB %= n;
        for (int guard = 0; guard < n && cs[(size_t) cellA].size() + cs[(size_t) cellB].size() < 7; ++guard) cellB = (cellB + 7) % n;   // a motif needs some notes
        for (int b = 0; b < 2; ++b)
            for (auto [st, ln] : cs[(size_t) (b == 0 ? cellA : cellB)]) m.push_back ({ b * TPB + st, ln, 0 });
        const int shape = (int) (r.next() % 4);   // arch, falling, rising, wave
        int cur = (int[]) { 0, 2, 4, 7 } [r.next() % 4];
        int prev = 99, same = 0;
        for (size_t i = 0; i < m.size(); ++i)
        {
            const float pos = (float) i / (float) std::max<size_t> (1, m.size() - 1);
            if (i > 0)
            {
                float up = 0.5f;
                if (shape == 0) up = pos < 0.5f ? 0.7f : 0.3f;
                else if (shape == 1) up = 0.3f;
                else if (shape == 2) up = pos < 0.7f ? 0.72f : 0.25f;
                else up = ((i / 3) % 2 == 0) ? 0.68f : 0.32f;
                const float x = r.uni();
                const int step = x < 0.07f ? 0 : x < 0.45f ? 1 : x < 0.70f ? 2 : x < 0.82f ? 3 : x < 0.89f ? 4 : x < 0.93f ? 5 : 7;
                cur += (r.uni() < up ? 1 : -1) * step;
                if (cur > 11) cur -= 7;
                if (cur < -3) cur += 7;
            }
            const int bar = startBar + m[i].tick / TPB;
            if (m[i].tick % 24 == 0 || (m[i].len >= 12 && r.uni() < 0.6f))   // beats 1 and 3, most long notes: chord tones
            {
                const int want = cur;
                cur = snapChord (cur, chordAt (bar));
                if (cur == prev && i > 0) cur = snapChord (want + (want >= prev ? 2 : -2), chordAt (bar));   // keep moving
            }
            if (cur == prev && ++same >= 2) { cur += r.uni() < 0.5f ? 1 : -1; same = 0; }
            else if (cur != prev) same = 0;
            m[i].deg = prev = cur;
        }
        if (! m.empty()) m.back().deg = snapChord (m.back().deg, chordAt (startBar + 1));
        return m;
    };
    auto vary = [&] (std::vector<MN> m, int op, int startBar)
    {
        const int first = m.empty() ? 0 : m.front().deg;
        switch (op)
        {
            case 0:   // new tail
            {
                int cur = m.empty() ? 0 : m[m.size() * 6 / 10].deg;
                for (size_t i = m.size() * 6 / 10; i < m.size(); ++i)
                {
                    cur += (r.uni() < 0.5f ? 1 : -1) * (1 + (int) (r.next() % 2));
                    cur = std::max (-3, std::min (11, cur));
                    m[i].deg = (m[i].tick % 12 == 0 || i + 1 == m.size()) ? snapChord (cur, chordAt (startBar + m[i].tick / TPB)) : cur;
                }
                break;
            }
            case 1: for (auto& n : m) n.deg = std::min (11, n.deg + 2); break;                                 // a third higher
            case 2: for (auto& n : m) n.deg = std::max (-3, std::min (11, 2 * first - n.deg)); break;          // mirrored
            case 3: { std::vector<MN> k; for (size_t i = 0; i < m.size(); ++i) if (i % 2 == 0 || m[i].tick % 12 == 0) k.push_back (m[i]); else if (! k.empty()) k.back().len += m[i].len; m = k; break; }  // sparser
            case 4: for (auto& n : m) if (n.tick >= TPB) n.deg = std::min (13, n.deg + 7); break;                // bar 2 an octave up
            default: for (auto& n : m) if (n.tick >= TPB) n.deg = std::max (-3, n.deg - 2); break;             // bar 2 falls
        }
        for (auto& n : m) if (n.tick % 24 == 0) n.deg = snapChord (n.deg, chordAt (startBar + n.tick / TPB));
        return m;
    };

    const auto A = makeMotif ((int) l.g[loopRhythmA], (int) l.g[loopRhythmB], 0);
    const int opB = (int) (r.next() % 6), opC = (int) (r.next() % 6);
    static const char formSlots[6][4] { { 'A', 'A', 'A', 'T' }, { 'A', 'B', 'A', 'b' }, { 'A', 'A', 'B', 'T' },
                                        { 'A', 'B', 'A', 'C' }, { 'A', 'T', 'A', 'B' }, { 'A', 'B', 'B', 'T' } };
    const float gate = gateSel == 0 ? 0.95f : gateSel == 1 ? 0.72f : 0.45f;
    std::vector<LoopNote> mel;
    for (int slot = 0; slot < bars / 2; ++slot)
    {
        const int bar0 = slot * 2;
        char f = formSlots[form][slot % 4];
        if (slot >= 4 && f == 'A' && slot % 4 == 2) f = 'C';   // 16 bars: the second half answers differently
        std::vector<MN> m = A;
        if (f == 'B' || f == 'b') m = vary (A, opB, bar0);
        if (f == 'C') m = vary (A, opC, bar0);
        if (f == 'T' || f == 'b') m = vary (m, 0, bar0);
        const int shift = follow ? chordAt (bar0) - chordAt (0) : 0;   // follow the chords or stay over a pedal
        for (size_t i = 0; i < m.size(); ++i)
        {
            int d = m[i].deg + shift;
            if (follow && m[i].tick % 24 == 0) d = snapChord (d, chordAt (bar0 + m[i].tick / TPB));
            if (slot == bars / 2 - 1 && i + 1 == m.size()) d = (d % 7 + 7) % 7 < 4 ? d - (d % 7 + 7) % 7 : d - (d % 7 + 7) % 7 + 4;   // loop ends on root / 5th
            const float st = (float) (bar0 * TPB + m[i].tick) / 12.0f;
            const float len = std::max (0.12f, (float) m[i].len / 12.0f * (m[i].len >= 12 ? 0.97f : gate));
            mel.push_back ({ st, len, pitch (d, root), false });
        }
    }
    // keep the melody around the middle of the keyboard
    if (! mel.empty())
    {
        float mean = 0; for (auto& n : mel) mean += (float) n.note; mean /= (float) mel.size();
        const int sh = 12 * (int) std::lround ((68.0f - mean) / 12.0f);
        for (auto& n : mel) n.note = std::max (40, std::min (96, n.note + sh));
    }
    if (! bassOnly)
    {
        for (auto& n : mel)
        {
            out.push_back (n);
            if (riffOnly) continue;
            if (doubling == 1 && std::fmod (n.start, 1.0f) < 0.01f) out.push_back ({ n.start, n.len, n.note - 12, false });   // octave under the beats
            if (doubling == 2 && n.len >= 1.4f) out.push_back ({ n.start, n.len, n.note - 7, false });                     // fifth under long notes
            if (doubling == 3)                                                                                              // diatonic third below
            {
                int best = n.note - 3;
                for (int dd : { 3, 4 }) { const int c = n.note - dd; for (int k = 0; k < 7; ++k) if ((c - key - sc[k]) % 12 == 0) best = c; }
                out.push_back ({ n.start, n.len, best, false });
            }
        }
    }
    // ---- low line
    if (! riffOnly && (bassStyle != 4 || bassOnly))
    {
        const int lowRoot = root - 24;
        for (int bar = 0; bar < bars; ++bar)
        {
            const int ch = bassStyle == 3 ? 0 : chordAt (bar);
            const int p = pitch (ch, lowRoot) - (pitch (ch, lowRoot) - lowRoot > 7 ? 12 : 0);
            const float b0 = (float) bar * 4.0f;
            switch (bassOnly && bassStyle == 4 ? 0 : bassStyle)
            {
                case 0: case 3: if (bar % chordBars == 0 || bassStyle == 3) out.push_back ({ b0, (bassStyle == 3 ? 4.0f : 4.0f * (float) chordBars) - 0.05f, p, true }); break;
                case 1: out.push_back ({ b0, 2.2f, p, true }); out.push_back ({ b0 + 2.5f, 0.45f, p + 12, true }); out.push_back ({ b0 + 3.0f, 0.9f, p, true }); break;
                case 2: out.push_back ({ b0, 1.4f, p, true }); out.push_back ({ b0 + 1.5f, 0.9f, p, true }); out.push_back ({ b0 + 2.5f, 0.45f, p, true }); out.push_back ({ b0 + 3.0f, 0.9f, p + (bar % 2 ? 7 : 0), true }); break;
                default: break;
            }
        }
    }
    std::sort (out.begin(), out.end(), [] (const LoopNote& a, const LoopNote& b) { return a.start != b.start ? a.start < b.start : a.note < b.note; });
    return out;
}

inline const char* keyName (int k)
{
    static const char* n[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return n[((k % 12) + 12) % 12];
}
} // namespace kk
