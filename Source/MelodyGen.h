#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include "DspUtil.h"

// v0.40 MELODY EVOLVE: melodies that grow. In any key and scale, from nothing (SURPRISE ME) or from your own melody.
// A melody lives in scale degrees, so every change stays in the key. Children = variations: new notes on the same
// rhythm, a new rhythm for the same notes, an answer phrase, busier, simpler, mirrored, shifted ...
namespace kk::mel
{
struct Note { float start = 0, len = 0.25f; int pitch = 60; float vel = 0.85f; };

enum Scale { scMinor, scMajor, scDorian, scPhrygian, scHarmMinor, scLydian, scMixolydian, scPentaMinor, scPentaMajor, scBlues, numScales };
inline const char* scaleName (int s)
{
    static const char* n[] { "MINOR", "MAJOR", "DORIAN", "PHRYGIAN", "HARMONIC MINOR", "LYDIAN", "MIXOLYDIAN", "MINOR PENTATONIC", "MAJOR PENTATONIC", "BLUES" };
    return n[std::clamp (s, 0, (int) numScales - 1)];
}
inline const std::vector<int>& scaleSteps (int s)
{
    static const std::vector<int> t[] { { 0, 2, 3, 5, 7, 8, 10 }, { 0, 2, 4, 5, 7, 9, 11 }, { 0, 2, 3, 5, 7, 9, 10 }, { 0, 1, 3, 5, 7, 8, 10 },
                                        { 0, 2, 3, 5, 7, 8, 11 }, { 0, 2, 4, 6, 7, 9, 11 }, { 0, 2, 4, 5, 7, 9, 10 }, { 0, 3, 5, 7, 10 },
                                        { 0, 2, 4, 7, 9 }, { 0, 3, 5, 6, 7, 10 } };
    return t[std::clamp (s, 0, (int) numScales - 1)];
}
inline const char* keyName (int k) { static const char* n[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }; return n[((k % 12) + 12) % 12]; }
inline bool isMinorish (int s) { return s == scMinor || s == scDorian || s == scPhrygian || s == scHarmMinor || s == scPentaMinor || s == scBlues; }

struct Melody
{
    std::vector<Note> notes;
    int bars = 8, key = 0, scale = scMinor;
    juce::String name, how;                 // how: what made it ("NEW NOTES", "ANSWER" ...)
    int parent = -1, gen = 0;
    std::vector<int> kids;
    // v0.41 GENRES: which style made it, its tempo, its chords (scale-degree roots per bar) and the CHORDS layer
    int genre = -1; float bpm = 140.0f;
    std::vector<int> prog;
    std::vector<Note> chords;
    float beats() const { return (float) bars * 4.0f; }
};

struct Style { float density = 0.5f, wild = 0.35f, range = 0.5f; };

// ---------------- scale degrees <-> pitches ----------------
inline int baseNote (int key, float range) { return 48 + ((key % 12) + 12) % 12 + (int) std::round ((range - 0.5f) * 2.0f) * 12; }   // C4 .. C6 area
inline int degreeToPitch (int d, int key, int scale, float range)
{
    const auto& st = scaleSteps (scale); const int n = (int) st.size();
    const int oct = (int) std::floor ((float) d / (float) n), idx = ((d % n) + n) % n;
    return baseNote (key, range) + 12 + 12 * oct + st[(size_t) idx];
}
inline int pitchToDegree (int pitch, int key, int scale, float range)
{
    int best = 0, bd = 1000;
    for (int d = -21; d <= 28; ++d) { const int dd = std::abs (degreeToPitch (d, key, scale, range) - pitch); if (dd < bd) { bd = dd; best = d; } }
    return best;
}

// chords as scale-degree roots per bar (i, VI, iv, v ... in minor; I, V, vi, IV in major)
inline std::vector<int> progression (uint32_t seed, int scale, int bars)
{
    static const int minorP[][4] { { 0, 5, 3, 4 }, { 0, 3, 5, 6 }, { 0, 6, 5, 6 }, { 0, 4, 5, 3 }, { 0, 0, 5, 4 }, { 0, 5, 6, 4 }, { 0, 2, 3, 4 } };
    static const int majorP[][4] { { 0, 4, 5, 3 }, { 0, 5, 3, 4 }, { 0, 3, 0, 4 }, { 0, 3, 5, 4 }, { 5, 3, 0, 4 }, { 0, 2, 3, 4 } };
    Rng r; r.seed (hash32 (seed ^ 0x1f83d9abu));
    const bool minor = isMinorish (scale);
    const int* p = minor ? minorP[r.next() % 7] : majorP[r.next() % 6];
    std::vector<int> out;
    for (int b = 0; b < bars; ++b) out.push_back (p[b % 4]);
    return out;
}
inline bool isChordTone (int degree, int chordRoot, int scaleSize)
{
    const int x = ((degree - chordRoot) % scaleSize + scaleSize) % scaleSize;
    return scaleSize >= 7 ? (x == 0 || x == 2 || x == 4) : (x == 0 || x == 2 || x == 3);
}
inline int nearestChordTone (int degree, int chordRoot, int scaleSize)
{
    for (int dd = 0; dd < 4; ++dd)
    {
        if (isChordTone (degree - dd, chordRoot, scaleSize)) return degree - dd;
        if (isChordTone (degree + dd, chordRoot, scaleSize)) return degree + dd;
    }
    return degree;
}

// ---------------- the rhythm of one bar (16 sixteenths) ----------------
inline std::vector<int> barRhythm (Rng& r, float density)
{
    static const float w[16] { 1.0f, 0.15f, 0.45f, 0.35f, 0.6f, 0.2f, 0.55f, 0.3f, 0.85f, 0.15f, 0.45f, 0.4f, 0.55f, 0.25f, 0.5f, 0.3f };
    std::vector<int> on;
    for (int s = 0; s < 16; ++s) if (r.uni() < std::min (0.95f, w[s] * (0.35f + 1.1f * density))) on.push_back (s);
    if (on.empty() || on[0] != 0) on.insert (on.begin(), r.uni() < 0.8f ? 0 : 2);
    if (on.size() < 2) on.push_back (8);
    std::sort (on.begin(), on.end()); on.erase (std::unique (on.begin(), on.end()), on.end());
    return on;
}

// notes in scale degrees for a rhythm: a walk that lands on chord tones on the strong beats
inline std::vector<int> walk (Rng& r, int count, int start, const std::vector<int>& steps16, int chordRoot, int scaleSize, float wild)
{
    std::vector<int> d; int cur = start;
    for (int i = 0; i < count; ++i)
    {
        if (i > 0)
        {
            const float x = r.uni();
            int step = x < 0.32f ? 1 : x < 0.62f ? -1 : x < 0.76f ? 2 : x < 0.88f ? -2 : x < 0.94f ? 0 : (r.uni() < 0.5f ? 3 + (int) (wild * 2) : -3 - (int) (wild * 2));
            cur += step;
            if (cur > 9) cur -= 2; if (cur < -4) cur += 2;
        }
        const int s = steps16[(size_t) i] % 16;
        if (s == 0 || s == 8 || (s == 4 && r.uni() < 0.5f)) cur = nearestChordTone (cur, chordRoot, scaleSize);
        d.push_back (cur);
    }
    return d;
}

inline float genreStaccato (int genre);
inline void setLengths (Melody& m, Rng& r)
{
    std::sort (m.notes.begin(), m.notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
    const float total = m.beats();
    const float stac = genreStaccato (m.genre);
    for (size_t i = 0; i < m.notes.size(); ++i)
    {
        size_t j = i + 1;
        while (j < m.notes.size() && m.notes[j].start < m.notes[i].start + 0.02f) ++j;   // stacked notes (dyads) ring together
        const float next = j < m.notes.size() ? m.notes[j].start : total;
        const float gap = std::max (0.06f, next - m.notes[i].start);
        float len = gap * (r.uni() < 0.2f ? 0.5f : 0.92f);
        if (stac > 0 && gap <= 1.01f && r.uni() < stac) len = std::min (gap, 0.25f);      // short bell / pluck notes
        len = std::clamp (len, 0.1f, 2.0f);
        m.notes[i].len = std::max (0.06f, std::min (len, gap - 0.02f));
    }
}

// keep a line inside about an octave and a half around its middle (octave folds stay in the key)
inline void foldRange (Melody& m, int maxSpan = 19)
{
    if (m.notes.size() < 2) return;
    std::vector<int> p; for (auto& n : m.notes) p.push_back (n.pitch);
    std::nth_element (p.begin(), p.begin() + (long) p.size() / 2, p.end());
    const int mid = p[p.size() / 2], half = maxSpan / 2;
    for (auto& n : m.notes) { while (n.pitch > mid + half + 1) n.pitch -= 12; while (n.pitch < mid - half) n.pitch += 12; }
}

// ---------------- SURPRISE ME: a new melody (motif, its answer, a resolution) ----------------
inline Melody generate (uint32_t seed, int key, int scale, int bars, const Style& st)
{
    Melody m; m.key = key; m.scale = scale; m.bars = bars; m.how = "NEW";
    Rng r; r.seed (hash32 (seed * 2654435761u + 77u));
    const int n = (int) scaleSteps (scale).size();
    const auto prog = progression (seed, scale, bars);
    m.prog = prog;
    // two motifs (A, B) of one bar each; a 4-bar phrase is A A' B A'' and a phrase answers the one before
    const auto rA = barRhythm (r, st.density), rB = barRhythm (r, st.density * 0.9f + 0.05f);
    const int startDeg = isChordTone (r.next() % 2 ? 2 : 4, 0, n) ? (int) (r.next() % 2 ? 2 : 4) : 0;
    const auto pA = walk (r, (int) rA.size(), startDeg, rA, prog[0], n, st.wild);
    for (int b = 0; b < bars; ++b)
    {
        const int inPhrase = b % 4, phrase = b / 4;
        const bool useB = inPhrase == 2;
        const auto& rh = useB ? rB : rA;
        std::vector<int> deg;
        if (useB) deg = walk (r, (int) rh.size(), pA.empty() ? 2 : pA.back() + (r.uni() < 0.5f ? 1 : -1), rh, prog[(size_t) b], n, st.wild);
        else
        {
            // A again, moved to the chord of this bar, a little changed (more in later phrases)
            const int shift = nearestChordTone (pA.empty() ? 0 : pA[0] + (prog[(size_t) b] - prog[0]), prog[(size_t) b], n) - (pA.empty() ? 0 : pA[0]);
            deg = pA;
            for (auto& d : deg) d += inPhrase == 0 && phrase % 2 == 0 ? 0 : shift;
            const float change = 0.1f + 0.25f * st.wild + 0.1f * (float) phrase;
            for (size_t i = 1; i < deg.size(); ++i) if (r.uni() < change) deg[i] += r.uni() < 0.5f ? 1 : -1;
        }
        if (inPhrase == 3 && ! deg.empty())   // the phrase ends: home (or the 5th on the first half of a long melody)
            deg.back() = (b == bars - 1 || phrase % 2 == 1) ? (deg.back() > 3 ? 7 : 0) : 4;
        for (size_t i = 0; i < rh.size() && i < deg.size(); ++i)
        {
            Note nt; nt.start = (float) b * 4.0f + (float) rh[i] * 0.25f;
            nt.pitch = degreeToPitch (deg[i], key, scale, st.range);
            nt.vel = rh[i] % 4 == 0 ? 0.92f : 0.72f + 0.15f * r.uni();
            m.notes.push_back (nt);
        }
    }
    setLengths (m, r);
    if (! m.notes.empty()) m.notes.back().len = std::max (m.notes.back().len, std::min (2.0f, m.beats() - m.notes.back().start - 0.05f));
    foldRange (m);
    return m;
}

inline std::vector<int> prog_first_bars (const std::vector<int>& prog, int bars)
{
    std::vector<int> out;
    for (int b = 0; b < bars; ++b) out.push_back (prog.empty() ? 0 : prog[(size_t) (b % (int) prog.size())]);
    return out;
}

// ---------------- v0.41 GENRES: melodies that fit the style and the tempo ----------------
// Rules from real trap melodies (2-bar phrases repeated, 1/8 grid, natural minor, short bell notes + long holds,
// half-step falls, octave jumps, a triplet roll at the end), house (off-beat rootless 7/9 stabs, 16th swing),
// drum & bass (high 1/16 arpeggios over a sustained pad), drill (half-step tension, 3-3-2 rhythm), lo-fi / R&B (swing, 7ths)
enum Genre { gTrap, gDark, gDrill, gPlugg, gRnb, gLofi, gHouse, gDnb, gAfro, numGenres };
inline const char* genreName (int g)
{
    static const char* n[] { "TRAP", "DARK TRAP", "DRILL", "PLUGG", "R&B", "LO-FI", "HOUSE", "DNB", "AFRO" };
    return g >= 0 && g < numGenres ? n[g] : "FREE";
}
struct GenreInfo { int bpm, lo, hi, scale; const char* hint; };
inline GenreInfo genreInfo (int g)
{
    switch (g)
    {
        case gTrap:  return { 140, 130, 160, scMinor,      "2-bar hook, 1/8 grid, bells + holds, triplet roll" };
        case gDark:  return { 138, 128, 155, scMinor,      "slow dark hook, stacked notes, half-step falls" };
        case gDrill: return { 142, 138, 146, scPhrygian,   "half-step tension, 3-3-2 rhythm, triplet runs" };
        case gPlugg: return { 150, 140, 165, scMajor,      "bright bouncy 1-bar loop, high register" };
        case gRnb:   return { 75,  60,  100, scDorian,     "smooth lines, 7th chords, soft swing" };
        case gLofi:  return { 82,  70,  92,  scDorian,     "lazy swung 1/16, jazzy chords, laid-back" };
        case gHouse: return { 126, 120, 130, scDorian,     "off-beat stabs, syncopated riff, 16th swing" };
        case gDnb:   return { 174, 170, 178, scMinor,      "high 1/16 arpeggios over a long pad" };
        case gAfro:  return { 108, 98,  118, scMajor,      "3-3-2 groove, call and answer" };
        default:     return { 140, 60,  180, scMinor,      "" };
    }
}
inline float genreStaccato (int g)
{
    switch (g) { case gTrap: return 0.5f; case gDark: return 0.3f; case gPlugg: return 0.55f; case gDrill: return 0.45f;
                 case gHouse: return 0.85f; case gAfro: return 0.5f; default: return 0.0f; }
}
inline float genreSwing (int g) { return g == gLofi ? 0.07f : g == gRnb ? 0.04f : g == gHouse ? 0.04f : g == gAfro ? 0.03f : 0.0f; }   // even 1/16 late (beats)

inline std::vector<int> genreProgression (Rng& r, int g, int scale, int bars)
{
    const bool minor = isMinorish (scale);
    std::vector<std::array<int, 4>> P;
    switch (g)
    {
        case gTrap:  P = minor ? std::vector<std::array<int, 4>> { { 0, 5, 3, 4 }, { 0, 4, 0, 4 }, { 0, 6, 0, 6 }, { 0, 5, 2, 6 }, { 0, 3, 5, 4 }, { 0, 0, 5, 5 }, { 5, 4, 0, 0 } }
                               : std::vector<std::array<int, 4>> { { 5, 3, 0, 4 }, { 0, 4, 5, 3 }, { 5, 5, 3, 4 } }; break;
        case gDark:  P = minor ? std::vector<std::array<int, 4>> { { 0, 0, 5, 4 }, { 0, 5, 0, 4 }, { 0, 3, 0, 5 }, { 0, 0, 3, 3 }, { 0, 6, 5, 6 } }
                               : std::vector<std::array<int, 4>> { { 5, 5, 3, 4 }, { 5, 2, 3, 4 } }; break;
        case gDrill: P = scale == scPhrygian ? std::vector<std::array<int, 4>> { { 1, 0, 0, 1 }, { 0, 1, 0, 6 }, { 0, 0, 1, 1 }, { 0, 5, 1, 0 } }
                       : scale == scHarmMinor ? std::vector<std::array<int, 4>> { { 0, 5, 4, 5 }, { 0, 4, 5, 4 }, { 0, 3, 4, 4 } }
                       : std::vector<std::array<int, 4>> { { 0, 5, 6, 4 }, { 0, 5, 0, 4 }, { 0, 3, 4, 0 } }; break;
        case gPlugg: P = minor ? std::vector<std::array<int, 4>> { { 0, 5, 2, 6 }, { 5, 6, 0, 0 }, { 0, 3, 6, 2 } }
                               : std::vector<std::array<int, 4>> { { 0, 5, 3, 4 }, { 3, 4, 2, 5 }, { 3, 2, 5, 4 }, { 5, 3, 0, 4 } }; break;
        case gRnb:   P = minor ? std::vector<std::array<int, 4>> { { 0, 3, 6, 2 }, { 3, 4, 0, 5 }, { 0, 5, 3, 4 }, { 0, 3, 0, 3 } }
                               : std::vector<std::array<int, 4>> { { 1, 4, 0, 5 }, { 3, 2, 1, 4 }, { 0, 5, 1, 4 } }; break;
        case gLofi:  P = minor ? std::vector<std::array<int, 4>> { { 3, 6, 2, 5 }, { 0, 3, 6, 2 }, { 0, 5, 3, 4 } }
                               : std::vector<std::array<int, 4>> { { 1, 4, 0, 5 }, { 3, 2, 1, 4 }, { 0, 5, 1, 4 } }; break;
        case gHouse: P = minor ? std::vector<std::array<int, 4>> { { 0, 6, 0, 6 }, { 0, 5, 6, 6 }, { 0, 3, 6, 2 }, { 0, 0, 3, 3 } }
                               : std::vector<std::array<int, 4>> { { 0, 4, 5, 3 }, { 1, 4, 0, 0 }, { 3, 4, 0, 5 } }; break;
        case gDnb:   P = minor ? std::vector<std::array<int, 4>> { { 0, 6, 5, 6 }, { 0, 5, 2, 6 }, { 0, 5, 3, 4 }, { 0, 0, 6, 6 } }
                               : std::vector<std::array<int, 4>> { { 0, 4, 5, 3 }, { 5, 3, 0, 4 } }; break;
        default:     P = minor ? std::vector<std::array<int, 4>> { { 0, 6, 5, 6 }, { 0, 3, 4, 0 }, { 0, 5, 6, 6 } }
                               : std::vector<std::array<int, 4>> { { 0, 3, 4, 3 }, { 5, 3, 0, 4 }, { 0, 4, 5, 3 } }; break;
    }
    const auto& p = P[r.next() % P.size()];
    const int n = (int) scaleSteps (scale).size();
    std::vector<int> out;
    for (int b = 0; b < bars; ++b) out.push_back (std::min (p[(size_t) (b % 4)], n - 1));
    return out;
}

// a note line through a rhythm: chord tones on the strong beats, steps / falls / jumps like real hooks
inline std::vector<int> hookLine (Rng& r, const std::vector<float>& starts, const std::vector<int>& prog, int n, float wild, int g, int startDeg)
{
    std::vector<int> d; int cur = startDeg;
    const bool dark = g == gDark || g == gDrill;
    for (size_t i = 0; i < starts.size(); ++i)
    {
        const int bar = std::clamp ((int) (starts[i] / 4.0f), 0, (int) prog.size() - 1);
        const float inBeat = starts[i] - std::floor (starts[i]);
        if (i > 0)
        {
            const float x = r.uni();
            if (x < 0.26f) cur -= 1;                                  // the half-step / step fall (b6 -> 5, b3 -> 2 ...)
            else if (x < 0.40f) cur += 1;
            else if (x < 0.52f) {}                                    // repeat: hooks hammer one note
            else if (x < 0.74f) cur = nearestChordTone (cur + (r.uni() < 0.5f ? 2 : -2), prog[(size_t) bar], n);
            else if (x < 0.84f) cur += r.uni() < 0.5f ? 4 : -4;       // a fifth / fourth
            else if (x < 0.84f + 0.1f * (0.4f + wild)) cur += cur < 3 ? n : -n;   // octave jump
            else cur += r.uni() < 0.5f ? 2 : -2;
            while (cur > n + 4) cur -= n;
            while (cur < -3) cur += n;
        }
        if (inBeat < 0.01f && ((int) std::round (starts[i]) % 2 == 0 || r.uni() < 0.5f)) cur = nearestChordTone (cur, prog[(size_t) bar], n);
        if (dark && i > 0 && inBeat < 0.01f && r.uni() < 0.12f) cur = (cur / n) * n + 1;   // lean on the 2nd (the b2 in phrygian)
        d.push_back (cur);
    }
    return d;
}

inline void addTripletRoll (Melody& m, float at, int pitch, Rng& r)
{
    // 1/16 triplet roll: 3 notes in a 1/8, falling in velocity (95 -> 65)
    for (int q = 0; q < 3; ++q)
    {
        Note nt; nt.start = at + (float) q / 6.0f; nt.len = 1.0f / 6.0f - 0.02f;
        nt.pitch = pitch + (q == 1 && r.uni() < 0.4f ? 0 : 0);
        nt.vel = (95.0f - 15.0f * (float) q) / 127.0f;
        m.notes.push_back (nt);
    }
}

// TRAP / DARK TRAP / PLUGG / DRILL: a short phrase repeated, the last time it answers
inline void genHook (Melody& m, Rng& r, const Style& st)
{
    const int g = m.genre, n = (int) scaleSteps (m.scale).size();
    const bool halfTime = m.bpm < 100.0f && g != gPlugg;           // 70 BPM trap = 140 BPM trap: the grid gets finer
    const float unit = g == gDrill ? 0.25f : (halfTime ? 0.25f : 0.5f);
    const int phraseBars = g == gPlugg ? 1 : 2;
    const float phraseBeats = 4.0f * (float) phraseBars;
    // rhythm of one phrase
    std::vector<float> starts;
    if (g == gDrill)
    {
        static const int groups[][6] { { 0, 3, 6, 8, 11, 14 }, { 0, 3, 6, 10, 12, 14 }, { 0, 2, 6, 8, 11, 13 }, { 0, 3, 8, 11, 14, 15 } };
        for (int b = 0; b < phraseBars; ++b)
        {
            const auto& gp = groups[r.next() % 4];
            for (int k = 0; k < 6; ++k) if (k == 0 || r.uni() < 0.55f + 0.4f * st.density) starts.push_back ((float) b * 4.0f + (float) gp[k] * 0.25f);
        }
    }
    else
    {
        static const float w8[8] { 1.0f, 0.5f, 0.55f, 0.7f, 0.8f, 0.5f, 0.55f, 0.6f };
        const int slotsPerBar = (int) std::round (4.0f / unit);
        for (int b = 0; b < phraseBars; ++b)
            for (int s = 0; s < slotsPerBar; ++s)
            {
                const float w = unit >= 0.5f ? w8[s % 8] : (s % 2 == 0 ? w8[(s / 2) % 8] : 0.22f);
                const float dens = g == gDark ? st.density * 0.75f : g == gPlugg ? st.density * 1.15f : st.density;
                if ((b == 0 && s == 0) || r.uni() < std::min (0.92f, w * (0.25f + 0.95f * dens))) starts.push_back ((float) b * 4.0f + (float) s * unit);
            }
        if (g == gPlugg)   // bouncy 1/16 pickups into beats
            for (size_t i = 0, sz = starts.size(); i < sz; ++i)
                if (r.uni() < 0.25f && starts[i] >= 0.25f) starts.push_back (starts[i] - 0.25f);
        std::sort (starts.begin(), starts.end()); starts.erase (std::unique (starts.begin(), starts.end()), starts.end());
    }
    if (starts.size() < 3) { starts.push_back (2.0f); starts.push_back (3.0f); std::sort (starts.begin(), starts.end()); starts.erase (std::unique (starts.begin(), starts.end()), starts.end()); }
    const int startDeg = r.uni() < 0.5f ? 0 : (r.uni() < 0.5f ? 4 : n);
    std::vector<int> phraseProg (prog_first_bars (m.prog, phraseBars));
    auto deg = hookLine (r, starts, phraseProg, n, st.wild, g, startDeg);
    if (g == gPlugg) for (auto& d : deg) { const int x = ((d % n) + n) % n; if (! isMinorish (m.scale) && (x == 3 || x == 6)) d += (x == 3 ? -1 : -1); }   // pentatonic feel
    // lengths inside the phrase
    std::vector<float> lens (starts.size());
    const float stac = genreStaccato (g);
    for (size_t i = 0; i < starts.size(); ++i)
    {
        const float next = i + 1 < starts.size() ? starts[i + 1] : phraseBeats;
        const float gap = next - starts[i];
        lens[i] = (r.uni() < stac && gap <= 1.01f) ? std::min (0.25f, gap - 0.02f) : std::min (2.0f, gap * 0.95f);
    }
    lens.back() = std::max (lens.back(), std::min (phraseBeats - starts.back() - 0.05f, 1.5f));
    // dyads: some long notes get a note below (a third / a fourth in the scale)
    const float dyad = g == gDark ? 0.4f : g == gTrap ? 0.15f : g == gDrill ? 0.12f : 0.08f;
    std::vector<bool> stack (starts.size());
    for (size_t i = 0; i < starts.size(); ++i) stack[i] = lens[i] >= 0.9f && r.uni() < dyad;
    const int reps = std::max (1, (int) std::round (m.beats() / phraseBeats));
    for (int rep = 0; rep < reps; ++rep)
    {
        const bool answer = (rep % 4 == 3) || (reps <= 2 && rep == reps - 1);   // the 4th time (or the last of a short loop) changes
        const float off = (float) rep * phraseBeats;
        for (size_t i = 0; i < starts.size(); ++i)
        {
            const bool secondHalf = starts[i] >= phraseBeats * 0.5f;
            int d = deg[i];
            if (answer && secondHalf)
            {
                const int bar = std::clamp ((int) ((off + starts[i]) / 4.0f), 0, m.bars - 1);
                d = nearestChordTone (d - 1 - (int) (r.uni() * 2.0f), m.prog[(size_t) bar], n);
                if (i + 1 == starts.size()) d = r.uni() < 0.6f ? 0 : 4;
            }
            Note nt; nt.start = off + starts[i]; nt.len = lens[i];
            nt.pitch = degreeToPitch (d, m.key, m.scale, st.range);
            const bool strong = starts[i] - std::floor (starts[i]) < 0.01f;
            nt.vel = (strong ? 100.0f : 88.0f) / 127.0f + r.bi() * 0.04f;
            m.notes.push_back (nt);
            if (stack[i]) { Note lo = nt; lo.pitch = degreeToPitch (d - (r.uni() < 0.7f ? 2 : 3), m.key, m.scale, st.range); lo.vel *= 0.85f; m.notes.push_back (lo); }
        }
        // the end of an answer: a triplet roll into the next loop (trap) / a run (drill)
        if (answer && (g == gTrap || g == gDrill || g == gDark) && r.uni() < 0.75f)
        {
            const float at = off + phraseBeats - 0.5f;
            m.notes.erase (std::remove_if (m.notes.begin(), m.notes.end(), [&] (const Note& x) { return x.start >= at - 0.01f && x.start < off + phraseBeats; }), m.notes.end());
            for (auto& x : m.notes) if (x.start < at && x.start + x.len > at) x.len = std::max (0.06f, at - x.start - 0.02f);
            addTripletRoll (m, at, degreeToPitch (r.uni() < 0.5f ? 0 : 4, m.key, m.scale, st.range), r);
        }
    }
}

// R&B / LO-FI: a sung line (motif - answer), soft swing, longer notes, little slides
inline Melody generate (uint32_t seed, int key, int scale, int bars, const Style& st);
inline void genSmooth (Melody& m, Rng& r, const Style& st, uint32_t seed)
{
    Style s2 = st; s2.density = st.density * (m.genre == gLofi ? 0.8f : 0.9f);
    auto base = generate (seed, m.key, m.scale, m.bars, s2);
    m.notes = base.notes;
    m.prog = base.prog.empty() ? m.prog : base.prog;
    for (auto& x : m.notes) x.len = std::min (2.5f, x.len * 1.15f);
    if (m.genre == gRnb)   // grace-note slides into some long notes
    {
        std::vector<Note> add;
        for (auto& x : m.notes)
            if (x.len >= 0.75f && x.start >= 0.25f && r.uni() < 0.3f)
            {
                Note gnote = x; gnote.start = x.start - 0.125f; gnote.len = 0.1f; gnote.vel = x.vel * 0.7f;
                gnote.pitch = degreeToPitch (pitchToDegree (x.pitch, m.key, m.scale, st.range) + 1, m.key, m.scale, st.range);
                add.push_back (gnote);
            }
        m.notes.insert (m.notes.end(), add.begin(), add.end());
    }
}

// HOUSE: a syncopated 16th riff that lives on the off-beats, short gates
inline void genHouse (Melody& m, Rng& r, const Style& st)
{
    const int n = (int) scaleSteps (m.scale).size();
    static const float w[16] { 0.7f, 0.15f, 0.8f, 0.55f, 0.3f, 0.2f, 0.75f, 0.6f, 0.35f, 0.25f, 0.8f, 0.5f, 0.3f, 0.45f, 0.7f, 0.35f };
    std::vector<float> starts;
    for (int b = 0; b < 2; ++b)
        for (int s = 0; s < 16; ++s)
            if (r.uni() < std::min (0.9f, w[s] * (0.35f + 0.9f * st.density))) starts.push_back ((float) b * 4.0f + (float) s * 0.25f);
    if (starts.size() < 4) { starts = { 0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 6.5f }; }
    std::vector<int> pp (prog_first_bars (m.prog, 2));
    auto deg = hookLine (r, starts, pp, n, st.wild * 0.6f, gHouse, r.uni() < 0.5f ? 2 : 4);
    const int reps = std::max (1, m.bars / 2);
    for (int rep = 0; rep < reps; ++rep)
        for (size_t i = 0; i < starts.size(); ++i)
        {
            const int bar = std::clamp ((int) ((float) rep * 2.0f + starts[i] / 4.0f), 0, m.bars - 1);
            int d = deg[i];
            if (rep % 2 == 1 && m.prog[(size_t) bar] != pp[(size_t) std::min ((int) pp.size() - 1, (int) (starts[i] / 4.0f))]) d = nearestChordTone (d, m.prog[(size_t) bar], n);   // the riff follows the chords
            Note nt; nt.start = (float) rep * 8.0f + starts[i]; nt.len = 0.25f + 0.125f * r.uni();
            nt.pitch = degreeToPitch (d, m.key, m.scale, st.range); nt.vel = (92.0f + r.bi() * 8.0f) / 127.0f;
            m.notes.push_back (nt);
        }
}

// DNB: 1/16 arpeggios high up, cascading through the chord
inline void genDnb (Melody& m, Rng& r, const Style& st)
{
    const int n = (int) scaleSteps (m.scale).size();
    const int shape = (int) (r.next() % 3);   // up, up-down, cascade
    const int keepEvery = st.density > 0.66f ? 1 : st.density > 0.33f ? 2 : 3;
    for (int b = 0; b < m.bars; ++b)
    {
        const int root = m.prog[(size_t) b];
        std::vector<int> tones { root, root + 2, root + 4, root + n, root + n + 2, root + n + 4 };
        const int variant = (b % 2 == 1 && r.uni() < 0.5f) ? 1 : 0;
        for (int s = 0; s < 16; ++s)
        {
            if (s % keepEvery != 0 && ! (s == 15 && keepEvery == 1)) continue;
            int idx;
            if (shape == 0) idx = s % 6;
            else if (shape == 1) { const int c = s % 10; idx = c < 6 ? c : 10 - c; }
            else { const int c = s % 8; idx = c < 4 ? c : c - 2; }
            idx = std::clamp (idx + variant, 0, 5);
            Note nt; nt.start = (float) b * 4.0f + (float) s * 0.25f;
            nt.len = 0.25f * (float) keepEvery * 1.6f;   // long sustain, they overlap into a cascade
            nt.pitch = degreeToPitch (tones[(size_t) idx], m.key, m.scale, st.range) + 12;
            nt.vel = (s % 4 == 0 ? 96.0f : 80.0f) / 127.0f + r.bi() * 0.03f;
            m.notes.push_back (nt);
        }
    }
}

// AFRO: 3-3-2 groove, call (bars 1-2) and answer (bars 3-4), pentatonic
inline void genAfro (Melody& m, Rng& r, const Style& st)
{
    const int n = (int) scaleSteps (m.scale).size();
    static const int pat[][6] { { 0, 3, 6, 8, 10, 12 }, { 0, 3, 6, 10, 12, 14 }, { 0, 2, 6, 8, 11, 14 } };
    const auto& p = pat[r.next() % 3];
    std::vector<float> call;
    for (int b = 0; b < 2; ++b) for (int k = 0; k < 6; ++k) if (k == 0 || r.uni() < 0.45f + 0.5f * st.density) call.push_back ((float) b * 4.0f + (float) p[k] * 0.25f);
    std::vector<int> pp (prog_first_bars (m.prog, 2));
    auto dc = hookLine (r, call, pp, n, st.wild * 0.5f, gAfro, 2);
    auto penta = [&] (int d) { const int x = ((d % n) + n) % n; return (n == 7 && (x == 3 || x == 6)) ? d - 1 : d; };
    for (int rep = 0; rep < std::max (1, m.bars / 2); ++rep)
    {
        const bool answer = rep % 2 == 1;
        for (size_t i = 0; i < call.size(); ++i)
        {
            int d = penta (dc[i] + (answer ? (i + 1 == call.size() ? -dc[i] : -1) : 0));
            Note nt; nt.start = (float) rep * 8.0f + call[i]; nt.len = 0.3f + 0.2f * r.uni();
            nt.pitch = degreeToPitch (d, m.key, m.scale, st.range); nt.vel = (90.0f + r.bi() * 10.0f) / 127.0f;
            m.notes.push_back (nt);
        }
    }
}

// voice a chord (scale degrees) close to a centre pitch: compact, smooth from chord to chord
inline std::vector<int> voiceChord (const std::vector<int>& degs, int key, int scale, int centre)
{
    std::vector<int> out;
    for (int d : degs)
    {
        int p = degreeToPitch (d, key, scale, 0.5f);
        while (p > centre + 6) p -= 12;
        while (p < centre - 6) p += 12;
        if (std::find (out.begin(), out.end(), p) == out.end()) out.push_back (p);
    }
    std::sort (out.begin(), out.end());
    return out;
}

// the CHORDS layer: pads (trap), 7ths (R&B / lo-fi), off-beat rootless stabs (house), long pads (DNB), syncopated (afro)
inline void genChords (Melody& m, Rng& r)
{
    m.chords.clear();
    const int g = m.genre, n = (int) scaleSteps (m.scale).size();
    int centre = 55 + ((m.key % 12) + 12) % 12 / 2;
    for (int b = 0; b < m.bars; ++b)
    {
        const int root = m.prog.empty() ? 0 : m.prog[(size_t) b];
        std::vector<int> degs;
        if (g == gHouse) degs = { root + 2, root + 4, root + 6, root + 8 };            // rootless 9th
        else if (g == gRnb || g == gLofi) degs = { root, root + 2, root + 4, root + 6 };
        else if (g == gPlugg) degs = { root, root + 2, root + 4, root + 6 };
        else if (g == gDark || g == gDrill) degs = r.uni() < 0.5f ? std::vector<int> { root, root + 4 } : std::vector<int> { root, root + 2, root + 4 };
        else degs = { root, root + 2, root + 4 };
        if (n < 7) degs = { root, root + 2, root + 3 };
        auto v = voiceChord (degs, m.key, m.scale, centre);
        if (! v.empty()) centre = (centre * 2 + (v.front() + v.back()) / 2) / 3;
        auto add = [&] (float s, float len, float vel)
        {
            for (size_t i = 0; i < v.size(); ++i)
            {
                Note nt; nt.start = (float) b * 4.0f + s + (g == gLofi ? 0.02f * (float) i : 0.0f);
                nt.len = len; nt.pitch = v[i]; nt.vel = vel;
                m.chords.push_back (nt);
            }
        };
        const bool sameAsNext = b + 1 < m.bars && ! m.prog.empty() && m.prog[(size_t) b + 1] == root;
        switch (g)
        {
            case gHouse: for (int k = 0; k < 4; ++k) if (k == 0 || r.uni() < 0.85f) add (0.5f + (float) k, 0.3f, 0.75f); break;   // 240 / 720 / 1200 / 1680 ticks
            case gAfro:  add (0.0f, 0.6f, 0.7f); add (1.5f, 0.4f, 0.62f); add (2.75f, 0.6f, 0.66f); break;
            case gDnb:   { auto keep = v; v = { degreeToPitch (root, m.key, m.scale, 0.5f) - 12 }; add (0.0f, 3.95f, 0.62f); v = keep; add (0.0f, 3.95f, 0.55f); break; }
            default:     if (b > 0 && ! m.prog.empty() && m.prog[(size_t) b - 1] == root && ! m.chords.empty()) { for (auto it = m.chords.rbegin(); it != m.chords.rend() && it->start > (float) (b - 1) * 4.0f - 0.01f; ++it) it->len += 4.0f; }
                         else add (0.0f, sameAsNext ? 3.95f : 3.95f, 0.66f);
                         break;
        }
    }
}

inline void humanize (Melody& m, Rng& r)
{
    const float sw = genreSwing (m.genre);
    for (auto* v : { &m.notes, &m.chords })
        for (auto& x : *v)
        {
            const float q = x.start * 4.0f;
            if (sw > 0 && std::abs (q - std::round (q)) < 0.01f && ((int) std::round (q)) % 2 == 1) x.start += sw;   // swing the even 1/16
            if (x.start > 0.05f) x.start += (r.uni() * 14.0f - 6.0f) / 480.0f;   // -6 .. +8 ticks
            if (m.genre == gLofi && x.start > 0.05f) x.start += 0.01f;            // laid back
            x.vel = std::clamp (x.vel, 0.3f, 1.0f);
        }
}

// SURPRISE ME with a genre
inline Melody generateGenre (uint32_t seed, int genre, int key, int scale, int bars, const Style& st, float bpm)
{
    if (genre < 0 || genre >= numGenres) { auto m = generate (seed, key, scale, bars, st); m.bpm = bpm; return m; }
    Melody m; m.key = key; m.scale = scale; m.bars = bars; m.genre = genre; m.bpm = bpm; m.how = genreName (genre);
    Rng r; r.seed (hash32 (seed * 2654435761u + 1013u));
    m.prog = genreProgression (r, genre, scale, bars);
    switch (genre)
    {
        case gRnb: case gLofi: genSmooth (m, r, st, seed); break;
        case gHouse: genHouse (m, r, st); break;
        case gDnb:   genDnb (m, r, st); break;
        case gAfro:  genAfro (m, r, st); break;
        default:     genHook (m, r, st); break;
    }
    std::sort (m.notes.begin(), m.notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
    m.notes.erase (std::remove_if (m.notes.begin(), m.notes.end(), [&] (const Note& x) { return x.start >= m.beats() - 0.01f; }), m.notes.end());
    for (auto& x : m.notes) x.len = std::max (0.06f, std::min (x.len, m.beats() - x.start - 0.01f));
    if (genre != gDnb) foldRange (m);
    genChords (m, r);
    humanize (m, r);
    return m;
}

// ---------------- children: variations that stay in the key ----------------
inline const char* variationName (int k)
{
    static const char* n[] { "NEW NOTES", "NEW RHYTHM", "VARIATION", "BUSIER", "SIMPLER", "ANSWER", "MIRROR", "SHIFT" };
    return n[((k % 8) + 8) % 8];
}

inline Melody vary (const Melody& p, int kind, uint32_t seed, float wild, const Style& st)
{
    Melody m = p; m.kids.clear(); m.how = variationName (kind);
    Rng r; r.seed (hash32 (seed ^ 0x2545f491u));
    const int n = (int) scaleSteps (p.scale).size();
    const auto prog = p.prog.size() == (size_t) p.bars ? p.prog : progression (seed ^ 0x77u, p.scale, p.bars);
    auto deg = [&] (const Note& x) { return pitchToDegree (x.pitch, p.key, p.scale, st.range); };
    auto pit = [&] (int d) { return degreeToPitch (d, p.key, p.scale, st.range); };
    const float amount = 0.2f + 0.6f * wild;
    switch (((kind % 8) + 8) % 8)
    {
        case 0:   // NEW NOTES: the same rhythm, a new line through it
        {
            std::vector<int> steps; for (auto& x : m.notes) steps.push_back ((int) std::round (x.start * 4.0f));
            const int startD = m.notes.empty() ? 0 : deg (m.notes[0]);
            int cur = startD;
            for (size_t i = 0; i < m.notes.size(); ++i)
            {
                const int bar = std::clamp ((int) (m.notes[i].start / 4.0f), 0, p.bars - 1);
                if (i > 0) { const float x = r.uni(); cur += x < 0.35f ? 1 : x < 0.7f ? -1 : x < 0.85f ? 2 : -2; cur = std::clamp (cur, -4, 9); }
                if (steps[i] % 8 == 0) cur = nearestChordTone (cur, prog[(size_t) bar], n);
                if (r.uni() < 1.0f - amount * 0.5f && i % 4 == 0) cur = nearestChordTone (deg (p.notes[i]), prog[(size_t) bar], n);   // keep some anchors
                m.notes[i].pitch = pit (cur);
            }
            break;
        }
        case 1:   // NEW RHYTHM: the same notes in order, a new groove
        {
            std::vector<int> pitches; for (auto& x : m.notes) pitches.push_back (x.pitch);
            m.notes.clear();
            size_t k = 0;
            if (p.genre >= 0 && ! pitches.empty())   // a new groove of the same style
            {
                auto fresh = generateGenre (seed * 13u + 5u, p.genre, p.key, p.scale, p.bars, st, p.bpm);
                for (auto x : fresh.notes) { x.pitch = pitches[k % pitches.size()]; m.notes.push_back (x); ++k; }
                break;
            }
            for (int b = 0; b < p.bars && ! pitches.empty(); ++b)
            {
                const auto rh = barRhythm (r, std::clamp (st.density + (r.uni() - 0.5f) * 0.3f, 0.1f, 0.95f));
                for (int s : rh) { Note nt; nt.start = (float) b * 4.0f + (float) s * 0.25f; nt.pitch = pitches[k % pitches.size()]; nt.vel = s % 4 == 0 ? 0.92f : 0.75f; m.notes.push_back (nt); ++k; }
            }
            setLengths (m, r);
            break;
        }
        case 2:   // VARIATION: some notes step up / down
            for (size_t i = 1; i < m.notes.size(); ++i) if (r.uni() < amount * 0.6f) m.notes[i].pitch = pit (deg (m.notes[i]) + (r.uni() < 0.5f ? 1 : -1) * (r.uni() < wild ? 2 : 1));
            break;
        case 3:   // BUSIER: passing notes and little rolls
        {
            std::vector<Note> add;
            for (size_t i = 0; i + 1 < m.notes.size(); ++i)
            {
                const float gap = m.notes[i + 1].start - m.notes[i].start;
                if (gap >= 0.5f && r.uni() < 0.35f + amount * 0.4f)
                {
                    const int a = deg (m.notes[i]), b = deg (m.notes[i + 1]);
                    Note nt = m.notes[i]; nt.start = m.notes[i].start + gap * 0.5f; nt.vel *= 0.85f;
                    nt.pitch = pit (a == b ? a + (r.uni() < 0.5f ? 1 : -1) : (a + b) / 2 + (a < b ? 0 : 1));
                    add.push_back (nt);
                }
                else if (gap >= 0.5f && r.uni() < wild * 0.3f + (p.genre == gTrap || p.genre == gDrill ? 0.2f : 0.0f))
                {
                    if (p.genre == gTrap || p.genre == gDrill || p.genre == gDark)   // a 1/16 triplet roll into the next note
                        for (int q = 1; q <= 2; ++q) { Note nt = m.notes[i + 1]; nt.start = m.notes[i + 1].start - (float) q / 6.0f; nt.len = 0.14f; nt.vel *= 0.65f + 0.1f * (float) q; add.push_back (nt); }
                    else   // a 1/32 roll
                        for (int q = 1; q <= 2; ++q) { Note nt = m.notes[i + 1]; nt.start = m.notes[i + 1].start - 0.125f * (float) q; nt.vel *= 0.7f; add.push_back (nt); }
                }
            }
            m.notes.insert (m.notes.end(), add.begin(), add.end());
            setLengths (m, r);
            break;
        }
        case 4:   // SIMPLER: the weak notes go, the strong ones ring longer
        {
            std::vector<Note> keep;
            for (auto& x : m.notes) { const int s = (int) std::round (x.start * 4.0f) % 16; if (s % 4 == 0 || r.uni() > 0.45f + amount * 0.3f) keep.push_back (x); }
            if (keep.size() >= 3) m.notes = keep;
            setLengths (m, r);
            break;
        }
        case 5:   // ANSWER: the first half stays, the second half answers it
        {
            const float half = p.beats() * 0.5f;
            m.notes.erase (std::remove_if (m.notes.begin(), m.notes.end(), [&] (const Note& x) { return x.start >= half; }), m.notes.end());
            Style s2 = st; s2.wild = std::max (st.wild, 0.4f);
            auto fresh = generateGenre (seed * 31u + 7u, p.genre, p.key, p.scale, std::max (1, p.bars / 2), s2, p.bpm);
            for (auto x : fresh.notes) { x.start += half; m.notes.push_back (x); }
            if (! m.notes.empty()) m.notes.back().pitch = pit (deg (m.notes.back()) > 3 ? 7 : 0);
            setLengths (m, r);
            break;
        }
        case 6:   // MIRROR: up becomes down (around the first note), in the scale
        {
            if (m.notes.empty()) break;
            const int axis = deg (m.notes[0]);
            const float from = r.uni() < 0.5f ? 0.0f : p.beats() * 0.5f;
            for (auto& x : m.notes) if (x.start >= from) x.pitch = pit (std::clamp (2 * axis - deg (x), -5, 10));
            break;
        }
        default:  // SHIFT: the groove moves by a 1/8, or a half jumps an octave
        {
            if (r.uni() < 0.5f)
            {
                for (auto& x : m.notes) { x.start += 0.5f; if (x.start >= p.beats()) x.start -= p.beats(); }
                setLengths (m, r);
            }
            else
            {
                const float from = r.uni() < 0.5f ? 0.0f : p.beats() * 0.5f, to = from + p.beats() * 0.5f;
                const int dir = r.uni() < 0.5f ? 12 : -12;
                for (auto& x : m.notes) if (x.start >= from && x.start < to) x.pitch += dir;
            }
            break;
        }
    }
    // keep it playable: inside the keyboard, in the key
    for (auto& x : m.notes) { x.pitch = std::clamp (x.pitch, 36, 96); x.pitch = pit (deg (x)); }
    if (((kind % 8) + 8) % 8 != 7) foldRange (m);   // SHIFT may jump an octave on purpose
    return m;
}

// ---------------- your melody: key detection + clean-up ----------------
// Krumhansl-Schmuckler: which key fits the time each pitch class sounds
inline void detectKey (const std::vector<Note>& notes, int& key, int& scale)
{
    static const float majP[12] { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
    static const float minP[12] { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };
    float h[12] {};
    for (auto& n : notes) h[((n.pitch % 12) + 12) % 12] += std::max (0.1f, n.len);
    float best = -1e9f; key = 0; scale = scMinor;
    auto corr = [&] (const float* prof, int k)
    {
        float mx = 0, mp = 0; for (int i = 0; i < 12; ++i) { mx += h[i]; mp += prof[i]; } mx /= 12; mp /= 12;
        float num = 0, dx = 0, dp = 0;
        for (int i = 0; i < 12; ++i) { const float a = h[(i + k) % 12] - mx, b = prof[i] - mp; num += a * b; dx += a * a; dp += b * b; }
        return num / std::sqrt (dx * dp + 1e-9f);
    };
    for (int k = 0; k < 12; ++k)
    {
        const float cm = corr (majP, k), cn = corr (minP, k);
        if (cm > best) { best = cm; key = k; scale = scMajor; }
        if (cn > best) { best = cn; key = k; scale = scMinor; }
    }
}

// raw notes (beats) -> a clean melody: on the 1/16 grid, starting at bar 1, 4 / 8 / 16 bars
inline Melody fromNotes (std::vector<Note> raw, int wantBars = 0)
{
    Melody m; m.how = "YOURS"; m.name = "Your melody";
    if (raw.empty()) return m;
    std::sort (raw.begin(), raw.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
    const float origin = std::floor (raw.front().start / 4.0f) * 4.0f;
    float end = 0;
    for (auto& n : raw)
    {
        n.start = std::round ((n.start - origin) * 4.0f) / 4.0f;
        n.len = std::max (0.125f, std::round (n.len * 8.0f) / 8.0f);
        end = std::max (end, n.start + n.len);
    }
    const int barsUsed = std::max (1, (int) std::ceil (end / 4.0f - 0.01f));
    m.bars = wantBars > 0 ? wantBars : barsUsed <= 4 ? 4 : barsUsed <= 8 ? 8 : 16;
    for (auto& n : raw) if (n.start < m.beats()) { n.len = std::min (n.len, m.beats() - n.start); m.notes.push_back (n); }
    detectKey (m.notes, m.key, m.scale);
    return m;
}

// layers: 0 = melody, 1 = melody + chords, 2 = chords only
inline std::vector<Note> layerNotes (const Melody& m, int layers)
{
    std::vector<Note> out;
    if (layers != 2) out = m.notes;
    if (layers >= 1) out.insert (out.end(), m.chords.begin(), m.chords.end());
    if (out.empty()) out = m.notes;
    return out;
}
inline juce::MidiFile toMidiFile (const Melody& m, double bpm, int layers = 0)
{
    const int ppq = 960;
    juce::MidiMessageSequence seq;
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)); tempo.setTimeStamp (0); seq.addEvent (tempo);
    for (auto& n : layerNotes (m, layers))
    {
        seq.addEvent (juce::MidiMessage::noteOn (1, n.pitch, (juce::uint8) std::clamp ((int) (n.vel * 127.0f), 1, 127)), std::round (std::max (0.0f, n.start) * ppq));
        seq.addEvent (juce::MidiMessage::noteOff (1, n.pitch), std::round ((n.start + n.len) * ppq));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq); mf.addTrack (seq);
    return mf;
}

// a .mid file (from FL or anywhere): the track with the most notes
inline std::vector<Note> notesFromMidiFile (const juce::MidiFile& src)
{
    juce::MidiFile mf = src;
    const int tpq = mf.getTimeFormat() > 0 ? mf.getTimeFormat() : 960;
    std::vector<Note> best;
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        juce::MidiMessageSequence seq (*mf.getTrack (t));
        seq.updateMatchedPairs();
        std::vector<Note> out;
        for (int i = 0; i < seq.getNumEvents(); ++i)
        {
            auto* e = seq.getEventPointer (i);
            if (! e->message.isNoteOn()) continue;
            Note n; n.pitch = e->message.getNoteNumber(); n.vel = e->message.getFloatVelocity();
            n.start = (float) (e->message.getTimeStamp() / tpq);
            const double off = e->noteOffObject != nullptr ? e->noteOffObject->message.getTimeStamp() : e->message.getTimeStamp() + tpq / 4;
            n.len = (float) std::max (0.05, (off - e->message.getTimeStamp()) / tpq);
            out.push_back (n);
        }
        if (out.size() > best.size()) best = out;
    }
    return best;
}


// v0.41 AUDIO -> MIDI: a sung / played / sampled melody becomes notes (monophonic pitch tracking, YIN at 11 kHz)
inline std::vector<Note> notesFromAudio (const juce::AudioBuffer<float>& in, double rate, double bpm)
{
    std::vector<Note> out;
    if (in.getNumSamples() < 2048 || rate <= 0) return out;
    const int dec = std::max (1, (int) std::round (rate / 11025.0));
    const double sr = rate / dec;
    const int total = std::min (in.getNumSamples(), (int) (rate * 40.0));
    std::vector<float> x; x.reserve ((size_t) (total / dec + 1));
    float lp1 = 0, lp2 = 0; const float a = std::exp (-twoPi * 4000.0f / (float) rate);
    for (int i = 0; i < total; ++i)
    {
        float v = 0; for (int c = 0; c < in.getNumChannels(); ++c) v += in.getSample (c, i);
        v /= (float) std::max (1, in.getNumChannels());
        lp1 = v + a * (lp1 - v); lp2 = lp1 + a * (lp2 - lp1);
        if (i % dec == 0) x.push_back (lp2);
    }
    const int W = 512, hop = 128, tauMin = (int) (sr / 1100.0), tauMax = std::min (W - 2, (int) (sr / 65.0));
    float peak = 0; for (float v : x) peak = std::max (peak, std::abs (v));
    if (peak < 1e-4f) return out;
    std::vector<float> pitch, level;
    std::vector<float> d ((size_t) tauMax + 1);
    for (int s0 = 0; s0 + W + tauMax < (int) x.size(); s0 += hop)
    {
        double e = 0; for (int i = 0; i < W; ++i) e += x[(size_t) (s0 + i)] * x[(size_t) (s0 + i)];
        const float rms = (float) std::sqrt (e / W);
        level.push_back (rms);
        if (rms < peak * 0.03f) { pitch.push_back (-1); continue; }
        for (int tau = 1; tau <= tauMax; ++tau)
        {
            double sum = 0;
            for (int i = 0; i < W; ++i) { const float df = x[(size_t) (s0 + i)] - x[(size_t) (s0 + i + tau)]; sum += df * df; }
            d[(size_t) tau] = (float) sum;
        }
        // cumulative mean normalised difference, first dip under the threshold
        double run = 0; int best = -1;
        std::vector<float> cm ((size_t) tauMax + 1, 1.0f);
        for (int tau = 1; tau <= tauMax; ++tau) { run += d[(size_t) tau]; cm[(size_t) tau] = run > 0 ? (float) (d[(size_t) tau] * tau / run) : 1.0f; }
        for (int tau = std::max (2, tauMin); tau < tauMax; ++tau)
            if (cm[(size_t) tau] < 0.18f) { while (tau + 1 < tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; best = tau; break; }
        if (best < 0) { pitch.push_back (-1); continue; }
        // a strong 2nd harmonic can fool the first dip: if the period twice as long fits clearly better, it is the real note
        if (best * 2 + 1 < tauMax && cm[(size_t) best * 2] < cm[(size_t) best] * 0.5f) best *= 2;
        const float y0 = cm[(size_t) best - 1], y1 = cm[(size_t) best], y2 = cm[(size_t) best + 1];
        const float den = y0 - 2 * y1 + y2;
        const float t = (float) best + (std::abs (den) > 1e-9f ? 0.5f * (y0 - y2) / den : 0.0f);
        pitch.push_back (69.0f + 12.0f * std::log2 ((float) sr / t / 440.0f));
    }
    // median of 5 against octave jumps and glitches
    std::vector<float> pm = pitch;
    for (size_t i = 2; i + 2 < pitch.size(); ++i)
    {
        std::array<float, 5> w { pitch[i - 2], pitch[i - 1], pitch[i], pitch[i + 1], pitch[i + 2] };
        int voiced = 0; for (float v : w) voiced += v > 0;
        if (voiced < 3) { pm[i] = -1; continue; }
        std::sort (w.begin(), w.end());
        pm[i] = w[(size_t) (5 - voiced / 2 - 1)];
    }
    // segment into notes
    const double beatsPerFrame = hop / sr * bpm / 60.0;
    int start = -1; float cur = 0; int n = 0;
    auto close = [&] (int endFrame)
    {
        if (start < 0) return;
        const double len = (endFrame - start) * beatsPerFrame;
        if (len * 60.0 / bpm >= 0.06)
        {
            // a frame is voiced once the window is half into the note: start where the note began
            Note nt; nt.start = (float) (start * beatsPerFrame + (W * 0.5 + hop * 0.5) / sr * bpm / 60.0); nt.len = (float) len; nt.pitch = std::clamp ((int) std::round (cur / (float) n), 24, 108);
            float lv = 0; for (int f = start; f < endFrame; ++f) lv = std::max (lv, level[(size_t) f]);
            nt.vel = std::clamp (0.45f + 0.55f * lv / peak, 0.3f, 1.0f);
            out.push_back (nt);
        }
        start = -1; n = 0; cur = 0;
    };
    for (int f = 0; f < (int) pm.size(); ++f)
    {
        const float p = pm[(size_t) f];
        const bool onset = f > 0 && level[(size_t) f] > level[(size_t) f - 1] * 1.8f && level[(size_t) f] > peak * 0.08f;
        if (p < 0) { close (f); continue; }
        if (start >= 0 && (std::abs (p - cur / (float) n) > 0.75f || onset)) close (f);
        if (start < 0) { start = f; cur = 0; n = 0; }
        cur += p; ++n;
    }
    close ((int) pm.size());
    return out;
}

inline juce::String melodyName (uint32_t seed, int gen, int k)
{
    static const char* a[] { "Velvet", "Night", "Glass", "Ghost", "Silver", "Lunar", "Smoke", "Crystal", "Dusk", "Neon", "Cold", "Rain", "Ember", "Echo", "Faded", "Golden" };
    static const char* b[] { "Line", "Hook", "Theme", "Riff", "Motif", "Call", "Song", "Spell", "Walk", "Rise", "Fall", "Loop" };
    return juce::String (a[(seed >> 3) % 16]) + " " + b[(seed >> 9) % 12] + " " + juce::String (gen) + "." + juce::String (k + 1);
}
} // namespace kk::mel
