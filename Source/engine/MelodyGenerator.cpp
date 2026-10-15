#include "MelodyGenerator.h"

#include <algorithm>
#include <cmath>

namespace ln
{
namespace
{
// ---- rhythm cells --------------------------------------------------------------------------------------------
enum Cell { Q, E2, S4, ES, SE, DOT, SYN, OFF, R, T3, H, DQ, NumCells };

struct Slot
{
    double start = 0, len = 0;
};

int cellBeats (int c) { return (c == H || c == DQ) ? 2 : 1; }

void addCell (int c, double t, std::vector<Slot>& out)
{
    auto add = [&] (double s, double l) { out.push_back ({ t + s, l }); };
    switch (c)
    {
        case Q:   add (0, 1); break;
        case E2:  add (0, 0.5); add (0.5, 0.5); break;
        case S4:  add (0, 0.25); add (0.25, 0.25); add (0.5, 0.25); add (0.75, 0.25); break;
        case ES:  add (0, 0.5); add (0.5, 0.25); add (0.75, 0.25); break;
        case SE:  add (0, 0.25); add (0.25, 0.25); add (0.5, 0.5); break;
        case DOT: add (0, 0.75); add (0.75, 0.25); break;
        case SYN: add (0, 0.25); add (0.25, 0.5); add (0.75, 0.25); break;
        case OFF: add (0.5, 0.5); break;
        case R:   break;
        case T3:  add (0, 1.0 / 3.0); add (1.0 / 3.0, 1.0 / 3.0); add (2.0 / 3.0, 1.0 / 3.0); break;
        case H:   add (0, 2); break;
        case DQ:  add (0, 1.5); add (1.5, 0.5); break;
        default:  break;
    }
}

enum Motion { Step, Arp, Hook, Pedal, Leap };

struct Character
{
    const char* name;
    int weights[NumCells];  // Q E2 S4 ES SE DOT SYN OFF R T3 H DQ
    Motion motion;
    int lo, hi;             // scale-degree range (0 = tonic around G3..F#4)
    double gate;
    double breathe;         // chance of a rest on the last beat of the 2-bar motif
};

const Character kCharacters[kNumMelodies] = {
    { "STEPWISE",    { 3, 6, 0, 2, 1, 2, 0, 0, 1, 0, 0, 1 }, Step,  1, 9,  0.92, 0.45 },
    { "ARPEGGIO",    { 0, 3, 5, 3, 2, 0, 0, 0, 0, 0, 0, 0 }, Arp,   0, 11, 0.90, 0.20 },
    { "HIGH HOOK",   { 1, 3, 0, 2, 0, 0, 4, 3, 1, 0, 0, 0 }, Hook,  6, 13, 0.85, 0.40 },
    { "LOW MOTIF",   { 2, 4, 0, 1, 2, 3, 0, 0, 1, 0, 0, 1 }, Step, -3, 5,  0.90, 0.40 },
    { "SPARSE",      { 3, 0, 0, 0, 0, 0, 0, 1, 3, 0, 4, 3 }, Step,  0, 9,  0.95, 0.30 },
    { "BUSY RUN",    { 0, 1, 6, 2, 2, 0, 1, 0, 0, 0, 0, 0 }, Step,  0, 10, 0.88, 0.15 },
    { "TRIPLETS",    { 1, 1, 0, 0, 0, 0, 0, 0, 1, 6, 0, 0 }, Arp,   0, 10, 0.90, 0.35 },
    { "DOTTED",      { 1, 1, 0, 0, 0, 5, 0, 0, 0, 0, 0, 3 }, Step,  1, 10, 0.90, 0.30 },
    { "SYNCOPATED",  { 0, 1, 0, 1, 0, 0, 5, 4, 1, 0, 0, 0 }, Hook,  2, 10, 0.85, 0.35 },
    { "CALL+ANSWER", { 0, 3, 3, 3, 1, 0, 1, 0, 0, 0, 0, 0 }, Step,  -1, 12, 0.90, 0.60 },
    { "BOUNCE",      { 0, 5, 1, 0, 0, 0, 0, 2, 0, 0, 0, 0 }, Pedal, 0, 10, 0.50, 0.25 },
    { "ZIGZAG",      { 1, 4, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0 }, Leap, -1, 11, 0.85, 0.30 },
};
constexpr int kCallAnswer = 9;
const int kAnswerWeights[NumCells] = { 2, 1, 0, 0, 0, 1, 0, 1, 3, 0, 2, 1 };

const std::array<int, 4> kMinorProgressions[] = {
    { 0, 5, 2, 6 }, { 0, 3, 5, 4 }, { 0, 6, 5, 6 }, { 0, 5, 3, 4 }, { 0, 2, 5, 6 }, { 0, 4, 5, 3 }, { 0, 3, 6, 2 },
};
const std::array<int, 4> kMajorProgressions[] = {
    { 0, 4, 5, 3 }, { 0, 5, 3, 4 }, { 5, 3, 0, 4 }, { 0, 3, 5, 4 }, { 0, 3, 4, 3 }, { 0, 1, 3, 4 },
};

bool nearlyInt (double x, double step) { const double r = x / step; return std::abs (r - std::round (r)) < 1e-6; }

struct Line
{
    std::vector<Slot> slots;
    std::vector<int> degrees;
};

class Generator
{
public:
    Generator (const SetSettings& s, int row)
        : settings (s),
          rng (mixSeed (mixSeed (s.seed, (uint64_t) row + 1), (uint64_t) s.rolls[(size_t) row] * 7919u + 13u)),
          ch (kCharacters[row]), characterIndex (row)
    {
        if (settings.key.minor)
            prog = kMinorProgressions[rng.range (0, (int) std::size (kMinorProgressions) - 1)];
        else
            prog = kMajorProgressions[rng.range (0, (int) std::size (kMajorProgressions) - 1)];
    }

    Melody run (int row)
    {
        Melody m;
        m.index = row;
        m.character = characterIndex;
        m.characterName = ch.name;
        m.progression = prog;
        const int bars = settings.bars >= 16 ? 16 : 8;
        m.lengthBeats = bars * 4.0;

        // motif A over chords 1-2, A' over chords 3-4 (same rhythm, contour moved + snapped), B = answer + ending
        const Line A = makeMotif (0);
        const Line A1 = vary (A, 2, 1);
        const Line B = makeEnding (A, A1, false);
        std::vector<const Line*> sections { &A, &A1, &A, &B };

        Line A2, B2;
        if (bars == 16)
        {
            A2 = vary (A, 2, 2);
            B2 = makeEnding (A, A2, true);
            sections.push_back (&A);
            sections.push_back (&A1);
            sections.push_back (&A2);
            sections.push_back (&B2);
        }

        for (size_t si = 0; si < sections.size(); ++si)
        {
            const Line& l = *sections[si];
            for (size_t i = 0; i < l.slots.size(); ++i)
            {
                Note n;
                n.start = (double) si * 8.0 + l.slots[i].start;
                n.length = l.slots[i].len;
                n.pitch = std::clamp (degreeToPitch (settings.key, l.degrees[i]), 36, 96);
                n.velocity = velocityFor (l.slots[i].start, (int) i);
                m.notes.push_back (n);
            }
        }

        std::sort (m.notes.begin(), m.notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
        // gate + strictly monophonic + inside the loop
        for (size_t i = 0; i < m.notes.size(); ++i)
        {
            auto& n = m.notes[i];
            const bool last = i + 1 == m.notes.size();
            double len = n.length * (last ? 0.95 : ch.gate);
            const double limit = last ? m.lengthBeats - n.start : m.notes[i + 1].start - n.start;
            len = std::min (len, limit);
            n.length = std::max (len, 1.0 / 32.0);
            // snap to the MIDI tick grid (480 ppq) so files and in-memory notes are identical
            n.start = std::round (n.start * 480.0) / 480.0;
            n.length = std::max (1.0, std::floor (n.length * 480.0)) / 480.0;
        }

        for (int b = 0; b < bars; ++b)
        {
            const int c = prog[(size_t) (b % 4)];
            for (int t : { 0, 2, 4 })
                m.chords.push_back ({ degreeToPitch (settings.key, c + t) - 12, b * 4.0, 4.0, 80 });
        }
        return m;
    }

private:
    int chordAtBar (int bar) const { return prog[(size_t) floorMod (bar, 4)]; }

    int clampDeg (int d, int lo, int hi) const
    {
        while (d > hi) d -= 2;   // reflect back into range
        while (d < lo) d += 2;
        return std::clamp (d, lo, hi);
    }

    int nearestChordTone (int target, int chord, int lo, int hi, int preferDir) const
    {
        for (int dist = 0; dist <= 7; ++dist)
        {
            const int a = target + (preferDir >= 0 ? dist : -dist);
            const int b = target - (preferDir >= 0 ? dist : -dist);
            if (a >= lo && a <= hi && isChordTone (a, chord)) return a;
            if (b >= lo && b <= hi && isChordTone (b, chord)) return b;
        }
        return clampDeg (target, lo, hi);
    }

    std::vector<Slot> makeRhythm (double from, double to, const int* weights, double breathe)
    {
        std::vector<Slot> slots;
        for (int attempt = 0; attempt < 12; ++attempt)
        {
            slots.clear();
            double t = from;
            while (t < to - 1e-9)
            {
                int w[NumCells];
                for (int c = 0; c < NumCells; ++c)
                {
                    w[c] = weights[c];
                    const int bInBar = (int) std::floor (t) % 4;
                    if (cellBeats (c) == 2 && (bInBar > 2 || t + 2 > to + 1e-9)) w[c] = 0;
                    if (t < from + 1e-9 && (c == R || c == OFF)) w[c] = 0;  // the motif starts on the beat
                }
                int c = rng.weighted (w, NumCells);
                if (t >= to - 1.0 - 1e-9 && to - from >= 4.0 && rng.chance (breathe)) c = R;  // breathe before the loop
                addCell (c, t, slots);
                t += cellBeats (c);
            }
            if (slots.size() >= (size_t) ((to - from) * 0.5) || attempt == 11) break;
        }
        if (slots.empty()) slots.push_back ({ from, 1.0 });
        return slots;
    }

    // one pitch decision (scale degree) for the note at time t (beats within the section) on chord `chord`
    int nextDegree (int cur, int noteIdx, double t, int chord, int lo, int hi)
    {
        int cand = cur;
        switch (ch.motion)
        {
            case Step:
            {
                if (rng.chance (0.22)) dir = -dir;
                cand = rng.chance (0.12) ? cur : cur + dir * (rng.chance (0.8) ? 1 : 2);
                if (cand > hi || cand < lo) { dir = -dir; cand = cur + dir; }
                break;
            }
            case Arp:
            {
                cand = cur + dir;
                while (! isChordTone (cand, chord)) cand += dir;
                if (cand > hi || cand < lo || rng.chance (0.15))
                {
                    dir = -dir;
                    cand = cur + dir;
                    while (! isChordTone (cand, chord)) cand += dir;
                }
                return clampDeg (cand, lo, hi);
            }
            case Hook:
            {
                if (hook.empty())
                {
                    const int centre = nearestChordTone ((lo + hi) / 2, chord, lo, hi, 1);
                    hook = { centre, centre + (rng.chance (0.5) ? 1 : -1), centre + (rng.chance (0.5) ? 2 : -2), centre };
                    if (rng.chance (0.5)) std::swap (hook[1], hook[2]);
                }
                cand = hook[(size_t) (noteIdx % (int) hook.size())] + (rng.chance (0.15) ? (rng.chance (0.5) ? 1 : -1) : 0);
                break;
            }
            case Pedal:
            {
                if (noteIdx % 2 == 0)
                {
                    // bounce on the chord root in the low part of the range
                    int d = lo;
                    while (floorMod (d - chord, 7) != 0) ++d;
                    return clampDeg (d, lo, hi);
                }
                if (rng.chance (0.25)) dir = -dir;
                line = (line == 0 ? cur + 3 : line) + dir * rng.range (1, 2);
                if (line > hi || line < lo + 2) { dir = -dir; line = std::clamp (line, lo + 2, hi); }
                cand = line;
                break;
            }
            case Leap:
            {
                const int mid = (lo + hi) / 2;
                const int target = cur <= mid ? cur + rng.range (4, 7) : cur - rng.range (4, 7);
                cand = nearestChordTone (target, chord, lo, hi, cur <= mid ? 1 : -1);
                if (rng.chance (0.25)) cand = cur + (rng.chance (0.5) ? 1 : -1);
                break;
            }
        }

        if (ch.motion != Leap && std::abs (cand - cur) > 5)
            cand = cur + (cand > cur ? 2 : -2);

        const bool strong = nearlyInt (t, 2.0);           // beats 1 and 3
        const bool onBeat = nearlyInt (t, 1.0);
        if (strong || (onBeat && rng.chance (0.5)))
            cand = nearestChordTone (cand, chord, lo, hi, cand >= cur ? 1 : -1);
        return clampDeg (cand, lo, hi);
    }

    Line makeMotif (int barOffset)
    {
        Line l;
        if (characterIndex == kCallAnswer)
        {
            l.slots = makeRhythm (0.0, 4.0, ch.weights, 0.0);
            auto answer = makeRhythm (4.0, 8.0, kAnswerWeights, 0.6);
            l.slots.insert (l.slots.end(), answer.begin(), answer.end());
        }
        else
        {
            l.slots = makeRhythm (0.0, 8.0, ch.weights, ch.breathe);
        }

        int lo = ch.lo, hi = ch.hi;
        int cur = nearestChordTone ((lo + hi) / 2 + rng.range (-2, 2), chordAtBar (barOffset), lo, hi, 1);
        dir = rng.chance (0.5) ? 1 : -1;
        for (size_t i = 0; i < l.slots.size(); ++i)
        {
            const double t = l.slots[i].start;
            if (characterIndex == kCallAnswer) { if (t < 4.0) { lo = 5; hi = 12; } else { lo = -1; hi = 6; } }
            const int chord = chordAtBar (barOffset + (int) std::floor (t / 4.0));
            cur = i == 0 ? cur : nextDegree (cur, (int) i, t, chord, lo, hi);
            if (i == 0) cur = clampDeg (cur, lo, hi);
            l.degrees.push_back (cur);
        }
        lastDegree = cur;
        return l;
    }

    static int shiftFor (int fromChord, int toChord)
    {
        int s = floorMod (toChord - fromChord, 7);
        return s > 3 ? s - 7 : s;
    }

    // same rhythm, contour moved to chords (barOffset, barOffset+1) and snapped; the last notes vary
    Line vary (const Line& a, int barOffset, int variation)
    {
        Line l = a;
        const int shift = shiftFor (chordAtBar (0), chordAtBar (barOffset));
        const int n = (int) l.slots.size();
        const int varyFrom = n - std::min (n - 1, variation == 1 ? 2 : 3);
        int lo = ch.lo, hi = ch.hi;
        for (int i = 0; i < n; ++i)
        {
            const double t = l.slots[(size_t) i].start;
            if (characterIndex == kCallAnswer) { if (t < 4.0) { lo = 5; hi = 12; } else { lo = -1; hi = 6; } }
            const int chord = chordAtBar (barOffset + (int) std::floor (t / 4.0));
            int d = clampDeg (a.degrees[(size_t) i] + shift, lo, hi);
            if (i >= varyFrom && i > 0)
                d = nextDegree (l.degrees[(size_t) i - 1], i, t, chord, lo, hi);
            else if (nearlyInt (t, 2.0))
                d = nearestChordTone (d, chord, lo, hi, shift >= 0 ? 1 : -1);
            l.degrees[(size_t) i] = d;
        }
        return l;
    }

    // B: first bar echoes A' (call), second bar answers and lands on a clear final note that loops back to bar 1
    Line makeEnding (const Line& a, const Line& a1, bool finalResolution)
    {
        Line l;
        for (size_t i = 0; i < a1.slots.size(); ++i)
            if (a1.slots[i].start < 4.0 - 1e-9)
            {
                l.slots.push_back (a1.slots[i]);
                l.degrees.push_back (a1.degrees[i]);
            }

        const bool early = rng.chance (finalResolution ? 0.5 : 0.33);
        const double endAt = early ? 4.0 : 6.0;
        const double endLen = early ? (rng.chance (0.5) ? 2.0 : 3.0) : (rng.chance (0.5) ? 1.0 : 1.5);

        int lo = ch.lo, hi = ch.hi;
        if (characterIndex == kCallAnswer) { lo = -1; hi = 6; }
        int cur = l.degrees.empty() ? a.degrees.front() : l.degrees.back();
        if (endAt > 4.0)
        {
            const int* w = characterIndex == kCallAnswer ? kAnswerWeights : ch.weights;
            auto fill = makeRhythm (4.0, endAt, w, 0.0);
            for (auto& s : fill)
            {
                const int chord = chordAtBar (2 + (int) std::floor (s.start / 4.0));
                cur = nextDegree (cur, (int) l.slots.size(), s.start, chord, lo, hi);
                l.slots.push_back (s);
                l.degrees.push_back (cur);
            }
        }

        const int lastChord = chordAtBar (3);
        int fin;
        if (finalResolution)
        {
            // home: the tonic closest to where we are
            fin = cur;
            for (int dist = 0; dist <= 7; ++dist)
            {
                if (floorMod (cur - dist, 7) == 0 && cur - dist >= lo) { fin = cur - dist; break; }
                if (floorMod (cur + dist, 7) == 0 && cur + dist <= hi) { fin = cur + dist; break; }
            }
        }
        else
        {
            // a chord tone of the last chord close to the first note, so the loop flows back into bar 1
            fin = nearestChordTone (a.degrees.front(), lastChord, lo, hi, 1);
        }
        l.slots.push_back ({ endAt, endLen });
        l.degrees.push_back (clampDeg (fin, std::min (lo, fin), std::max (hi, fin)));
        return l;
    }

    int velocityFor (double t, int idx)
    {
        int v = nearlyInt (t, 4.0) ? 110 : nearlyInt (t, 2.0) ? 102 : nearlyInt (t, 1.0) ? 94 : 84;
        if (ch.motion == Pedal && idx % 2 == 0) v -= 10;
        return std::clamp (v + rng.range (-6, 6), 1, 127);
    }

    SetSettings settings;
    Rng rng;
    const Character& ch;
    int characterIndex;
    std::array<int, 4> prog {};
    int dir = 1, line = 0, lastDegree = 0;
    std::vector<int> hook;
};
} // namespace

const char* characterName (int character) { return kCharacters[floorMod (character, kNumMelodies)].name; }

Melody generateMelody (const SetSettings& s, int row)
{
    row = floorMod (row, kNumMelodies);
    Generator g (s, row);
    return g.run (row);
}

MelodySet generateSet (const SetSettings& s)
{
    MelodySet set;
    set.settings = s;
    for (int i = 0; i < kNumMelodies; ++i)
        set.melodies[(size_t) i] = generateMelody (s, i);
    return set;
}

Key luckyKey (uint32_t seed)
{
    Rng r (mixSeed (seed, 0x4C55434Bull)); // "LUCK"
    Key k;
    k.root = r.range (0, 11);
    k.minor = r.chance (0.7);
    return k;
}
} // namespace ln
