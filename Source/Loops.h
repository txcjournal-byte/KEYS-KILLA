#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <utility>
#include <vector>
#include "DspUtil.h"
#include "LoopFragments.h"

// BREED LOOPS: melody loops assembled from a library of hand-written trap phrases (Source/LoopFragments.h,
// written in tools/melody_fragments.py in the style of the loops producers picked).
// 8 bars, one track (low line + riff, like melody-pack MIDI):
//     [OPENER | ANSWER] [OPENER | TURN] [OPENER | ANSWER] [OPENER | ENDING]
// Bar 1 of each pair sits on the root, bar 2 on the bass plan's chord, the ending leads back to the start.
// Only phrases whose strong beats are chord tones of their bar are combined, and the answer starts near where
// the opener ends - so every combination makes musical sense. 16 bars: the second half answers differently.
// 20 openers x 16 answers x 10 turns x 7 endings x 5 bass plans x 2 bass rhythms x 3 registers x 12 keys.
namespace kk
{
enum LoopGene { loopOpener, loopAnswer, loopTurn, loopEnding, loopBass, loopBassRhythm, loopRegister, loopOpenerVar, loopAnswerVar, numLoopGenes };

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
    using namespace loopfrag;
    inline constexpr uint32_t size[numLoopGenes] { (uint32_t) numOpeners, (uint32_t) numAnswers, (uint32_t) numTurns, (uint32_t) numEndings,
                                                   (uint32_t) numBass, 2, 3, 6, 6 };
    inline constexpr int numVariants = 6;

    // phrase variants: rhythm and register change, notes on beats 1 and 3 stay (so the chords still fit)
    inline Frag variant (const Frag& f, int v)
    {
        Frag o {};
        auto add = [&o] (int st, int ln, int s) { if (o.n < 16 && ln > 0) o.v[o.n++] = { (signed char) st, (signed char) ln, (signed char) s }; };
        auto strong = [] (int st) { return st == 0 || st == 8; };
        switch (v % numVariants)
        {
            case 1:   // dotted: straight 8th pairs become long-short
                for (int i = 0; i < f.n; ++i)
                {
                    const auto n = f.v[i];
                    if (i + 1 < f.n && n.ln == 2 && f.v[i + 1].ln == 2 && f.v[i + 1].st == n.st + 2 && n.st % 4 == 0 && ! strong (n.st + 2))
                    { add (n.st, 3, n.s); add (n.st + 3, 1, f.v[i + 1].s); ++i; }
                    else add (n.st, n.ln, n.s);
                }
                break;
            case 2:   // syncopated: off-beat notes land a 16th later
                for (int i = 0; i < f.n; ++i)
                {
                    const auto n = f.v[i];
                    const int next = i + 1 < f.n ? f.v[i + 1].st : 16;
                    if (! strong (n.st) && n.st % 2 == 0 && n.st + 1 < next && n.ln >= 2) add (n.st + 1, n.ln - 1, n.s);
                    else add (n.st, n.ln, n.s);
                }
                break;
            case 3:   // octave leaps on every second off-beat note
            {
                int k = 0;
                for (int i = 0; i < f.n; ++i)
                {
                    const auto n = f.v[i];
                    int s = n.s;
                    if (! strong (n.st) && (k++ % 2) == 1) s = s + 12 <= 27 ? s + 12 : s - 12;
                    add (n.st, n.ln, s);
                }
                break;
            }
            case 4:   // stutter: long notes become 16th rolls
                for (int i = 0; i < f.n; ++i)
                {
                    const auto n = f.v[i];
                    if (n.ln >= 4) { add (n.st, 1, n.s); add (n.st + 1, 1, n.s); add (n.st + 2, n.ln - 2, n.s); }
                    else add (n.st, n.ln, n.s);
                }
                break;
            case 5:   // sparser: off-beat 8ths are held from the note before
                for (int i = 0; i < f.n; ++i)
                {
                    const auto n = f.v[i];
                    if (o.n > 0 && ! strong (n.st) && n.st % 4 == 2) { o.v[o.n - 1].ln = (signed char) (n.st + n.ln - o.v[o.n - 1].st); continue; }
                    add (n.st, n.ln, n.s);
                }
                break;
            default: return f;
        }
        return o.n > 0 ? o : f;
    }
    inline bool fits (const Frag& f, int chordMask)
    {
        for (int i = 0; i < f.n; ++i)
            if ((f.v[i].st == 0 || f.v[i].st == 8) && ((chordMask >> (f.v[i].s % 12)) & 1) == 0) return false;
        return true;
    }
    inline bool answerOk (int opener, int answer, int bassPlan)
    {
        const auto& o = openers[opener]; const auto& a = answers[answer];
        return fits (a, bass[bassPlan].chordMask) && std::abs (a.v[0].s - o.v[o.n - 1].s) <= 7;
    }
    inline bool turnOk (int turn, int bassPlan) { return fits (turns[turn], bass[bassPlan].chordMask); }
}

inline const char* loopGeneName (int g)
{
    static const char* n[] { "START", "ANSWER", "TURN", "ENDING", "BASS", "BASS RHYTHM", "REGISTER" };
    return n[g < 0 || g >= numLoopGenes ? 0 : g];
}

inline uint32_t randomGene (int gene, Rng& r) { return r.next() % loopdata::size[gene]; }

// make sure the answer and the turn fit the chords and the opener (next fitting phrase, deterministic)
inline void fixLoop (LoopGenes& l)
{
    using namespace loopdata;
    for (int i = 0; i < numLoopGenes; ++i) l.g[(size_t) i] %= size[i];
    const int o = (int) l.g[loopOpener];
    int b = (int) l.g[loopBass];
    for (int tries = 0; tries < numBass; ++tries, b = (b + 1) % numBass)
    {
        bool okA = false, okT = false;
        for (int k = 0; k < numAnswers && ! okA; ++k) okA = answerOk (o, ((int) l.g[loopAnswer] + k) % numAnswers, b);
        for (int k = 0; k < numTurns && ! okT; ++k) okT = turnOk (((int) l.g[loopTurn] + k) % numTurns, b);
        if (okA && okT) break;
    }
    l.g[loopBass] = (uint32_t) b;
    for (int k = 0; k < numAnswers; ++k) { const int a = ((int) l.g[loopAnswer] + k) % numAnswers; if (answerOk (o, a, b)) { l.g[loopAnswer] = (uint32_t) a; break; } }
    for (int k = 0; k < numTurns; ++k) { const int t = ((int) l.g[loopTurn] + k) % numTurns; if (turnOk (t, b)) { l.g[loopTurn] = (uint32_t) t; break; } }
}

inline LoopGenes loopFromSeed (uint32_t seed)
{
    LoopGenes l;
    Rng r; r.seed (hash32 (seed ^ 0x51ed270bu));
    for (int i = 0; i < numLoopGenes; ++i) l.g[(size_t) i] = randomGene (i, r);
    if (r.uni() < 0.6f) l.g[loopRegister] = 0;       // mostly the home register
    l.key = (int) (r.next() % 12u);
    l.valid = true;
    fixLoop (l);
    return l;
}

// a new melody that is never identical to the sources: phrases mixed from them, at least two phrases changed
inline LoopGenes mixLoops (const std::vector<LoopGenes>& from, uint32_t seed, float wild)
{
    Rng r; r.seed (seed ^ 0x2545f491u);
    std::vector<const LoopGenes*> src;
    for (auto& f : from) if (f.valid) src.push_back (&f);
    if (src.empty()) return loopFromSeed (seed);
    LoopGenes c = *src[r.next() % src.size()];
    for (int i = 0; i < numLoopGenes; ++i)
    {
        c.g[(size_t) i] = src[r.next() % src.size()]->g[(size_t) i];
        if (r.uni() < 0.15f + wild * 0.5f) c.g[(size_t) i] = randomGene (i, r);
    }
    for (int k = 0; k < 2; ++k)
    {
        const int gene = (int) (r.next() % 4u);   // opener / answer / turn / ending
        c.g[(size_t) gene] = (c.g[(size_t) gene] + 1 + r.next() % (loopdata::size[gene] - 1)) % loopdata::size[gene];
    }
    c.g[loopOpenerVar] = randomGene (loopOpenerVar, r);   // every melody gets its own rhythm / register variants
    c.g[loopAnswerVar] = randomGene (loopAnswerVar, r);
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
    if (from.size() == 2 && r.uni() < std::abs (leanA - 0.5f)) from.erase (from.begin() + (leanA > 0.5f ? 1 : 0));   // lean to one parent
    return mixLoops (from, seed, wild);
}

// NEW MELODY: same key and bass - new phrases
inline LoopGenes rerollLoop (const LoopGenes& l, uint32_t seed)
{
    LoopGenes c = l.valid ? l : loopFromSeed (seed);
    Rng r; r.seed (hash32 (seed ^ 0x7f4a7c15u));
    for (int i : { (int) loopOpener, (int) loopAnswer, (int) loopTurn, (int) loopEnding, (int) loopOpenerVar, (int) loopAnswerVar }) c.g[(size_t) i] = randomGene (i, r);
    fixLoop (c);
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
    int hi = key <= 4 ? 60 + key : 48 + key;
    const int lo = key <= 4 ? 36 + key : 24 + key;
    const int reg = (int) (l.g[loopRegister] % 3);
    if (reg == 1 && hi + 12 <= 72) hi += 12;
    if (reg == 2 && hi - 12 >= 50) hi -= 12;
    const int o = (int) (l.g[loopOpener] % size[loopOpener]);
    const int bp = (int) (l.g[loopBass] % size[loopBass]);
    const int e = (int) (l.g[loopEnding] % size[loopEnding]);
    auto nextFitting = [&] (int start, int count, auto ok) { for (int k = 1; k < count; ++k) { const int x = (start + k) % count; if (ok (x)) return x; } return start; };
    const int a1 = (int) (l.g[loopAnswer] % size[loopAnswer]), t1 = (int) (l.g[loopTurn] % size[loopTurn]);
    const int a2 = nextFitting (a1, numAnswers, [&] (int x) { return answerOk (o, x, bp); });
    const int t2 = nextFitting (t1, numTurns, [&] (int x) { return turnOk (x, bp); });
    const bool held = l.g[loopBassRhythm] % 2 == 0;
    out.reserve (200);
    for (int bar = 0; bar < bars; ++bar)
    {
        const bool second = bar >= 8;
        const int pos = bar % 8;
        // the opener changes its variant every other time (and again in the second 8 bars), the answer has its own
        const int pair = pos / 2 + (second ? 1 : 0);
        const int ov = (int) (l.g[loopOpenerVar] % numVariants), av = (int) (l.g[loopAnswerVar] % numVariants);
        Frag frag = variant (openers[o], pair % 2 == 0 ? ov : (ov + 3) % numVariants);
        int bassSemi = 0;
        if (pos % 2 == 1)
        {
            bassSemi = pos == 7 ? bass[bp].ending : bass[bp].bar2;
            frag = pos == 7 ? endings[e] : pos == 3 ? turns[second ? t2 : t1] : variant (answers[second ? a2 : a1], second ? (av + 2) % numVariants : av);
        }
        const Frag* f = &frag;
        const float b0 = (float) bar * 4.0f;
        if (! riffOnly)
        {
            if (held) out.push_back ({ b0, 3.97f, lo + bassSemi, true });
            else for (auto [st, ln] : { std::pair<int, int> { 0, 6 }, { 6, 4 }, { 10, 2 }, { 12, 4 } })
                out.push_back ({ b0 + (float) st * 0.25f, (float) ln * 0.25f - 0.03f, lo + bassSemi, true });
        }
        if (! bassOnly)
            for (int i = 0; i < f->n; ++i)
                out.push_back ({ b0 + (float) f->v[i].st * 0.25f, (float) f->v[i].ln * 0.25f - 0.02f, hi + f->v[i].s, false });
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
