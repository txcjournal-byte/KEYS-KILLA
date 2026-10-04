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

inline void setLengths (Melody& m, Rng& r)
{
    std::sort (m.notes.begin(), m.notes.end(), [] (const Note& a, const Note& b) { return a.start < b.start; });
    const float total = m.beats();
    for (size_t i = 0; i < m.notes.size(); ++i)
    {
        const float next = i + 1 < m.notes.size() ? m.notes[i + 1].start : total;
        const float gap = next - m.notes[i].start;
        float len = gap * (r.uni() < 0.2f ? 0.5f : 0.92f);
        len = std::clamp (len, 0.1f, 2.0f);
        m.notes[i].len = std::max (0.08f, std::min (len, gap - 0.02f));
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
    const auto prog = progression (seed ^ 0x77u, p.scale, p.bars);
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
                else if (gap >= 0.5f && r.uni() < wild * 0.3f)   // a 1/32 roll into the next note
                    for (int q = 1; q <= 2; ++q) { Note nt = m.notes[i + 1]; nt.start = m.notes[i + 1].start - 0.125f * (float) q; nt.vel *= 0.7f; add.push_back (nt); }
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
            auto fresh = generate (seed * 31u + 7u, p.key, p.scale, std::max (1, p.bars / 2), s2);
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

inline juce::MidiFile toMidiFile (const Melody& m, double bpm)
{
    const int ppq = 960;
    juce::MidiMessageSequence seq;
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)); tempo.setTimeStamp (0); seq.addEvent (tempo);
    for (auto& n : m.notes)
    {
        seq.addEvent (juce::MidiMessage::noteOn (1, n.pitch, (juce::uint8) std::clamp ((int) (n.vel * 127.0f), 1, 127)), std::round (n.start * ppq));
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

inline juce::String melodyName (uint32_t seed, int gen, int k)
{
    static const char* a[] { "Velvet", "Night", "Glass", "Ghost", "Silver", "Lunar", "Smoke", "Crystal", "Dusk", "Neon", "Cold", "Rain", "Ember", "Echo", "Faded", "Golden" };
    static const char* b[] { "Line", "Hook", "Theme", "Riff", "Motif", "Call", "Song", "Spell", "Walk", "Rise", "Fall", "Loop" };
    return juce::String (a[(seed >> 3) % 16]) + " " + b[(seed >> 9) % 12] + " " + juce::String (gen) + "." + juce::String (k + 1);
}
} // namespace kk::mel
