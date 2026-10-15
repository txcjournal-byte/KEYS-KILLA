// Lucky Notes engine tests: melody sets, MIDI round trip (juce::MidiFile), offline preview render (clicks / peak).
// Exit code 0 = all passed.
#include <juce_audio_basics/juce_audio_basics.h>

#include "MelodyGenerator.h"
#include "MidiWriter.h"
#include "PreviewEngine.h"

#include <cmath>
#include <cstdio>
#include <set>

using namespace ln;

static int failures = 0, checks = 0;
#define CHECK(cond, ...)                                                   \
    do {                                                                   \
        ++checks;                                                          \
        if (! (cond)) {                                                    \
            ++failures;                                                    \
            if (failures < 60) {                                           \
                std::printf ("FAIL %s:%d: %s -- ", __FILE__, __LINE__, #cond); \
                std::printf (__VA_ARGS__);                                 \
                std::printf ("\n");                                        \
            }                                                              \
        }                                                                  \
    } while (0)

static bool sameNotes (const std::vector<Note>& a, const std::vector<Note>& b)
{
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i)
        if (a[i].pitch != b[i].pitch || std::abs (a[i].start - b[i].start) > 1e-9 || std::abs (a[i].length - b[i].length) > 1e-9
            || a[i].velocity != b[i].velocity)
            return false;
    return true;
}

static void checkMidi (const Melody& m, double bpm, const Key& key, int bars)
{
    const auto bytes = melodyMidi (m, bpm, key);
    juce::MemoryInputStream in (bytes.data(), bytes.size(), false);
    juce::MidiFile mf;
    int format = -1;
    const bool ok = mf.readFrom (in, true, &format);
    CHECK (ok, "MIDI file does not parse (row %d)", m.index);
    if (! ok) return;
    CHECK (format == 1, "format %d", format);
    CHECK (mf.getNumTracks() == 2, "tracks %d", mf.getNumTracks());
    CHECK (mf.getTimeFormat() == kTicksPerQuarter, "ppq %d", (int) mf.getTimeFormat());

    juce::MidiMessageSequence tempos;
    mf.findAllTempoEvents (tempos);
    CHECK (tempos.getNumEvents() == 1, "tempo events %d", tempos.getNumEvents());
    if (tempos.getNumEvents() > 0)
    {
        const double fileBpm = 60.0 / tempos.getEventPointer (0)->message.getTempoSecondsPerQuarterNote();
        CHECK (std::abs (fileBpm - bpm) < 0.01, "tempo %.4f != %.4f", fileBpm, bpm);
    }

    const auto* track = mf.getTrack (1);
    int noteOns = 0;
    juce::String name;
    double lastTick = 0, maxNoteEnd = 0;
    for (int i = 0; i < track->getNumEvents(); ++i)
    {
        const auto& msg = track->getEventPointer (i)->message;
        lastTick = std::max (lastTick, msg.getTimeStamp());
        if (msg.isNoteOn()) { ++noteOns; CHECK (pitchInScale (key, msg.getNoteNumber()), "MIDI note %d out of scale", msg.getNoteNumber()); }
        if (msg.isNoteOff()) maxNoteEnd = std::max (maxNoteEnd, msg.getTimeStamp());
        if (msg.isTrackNameEvent()) name = msg.getTextFromTextMetaEvent();
        if (msg.isEndOfTrackMetaEvent())
            CHECK (std::abs (msg.getTimeStamp() - bars * 4.0 * kTicksPerQuarter) < 0.5, "end of track at %.1f", msg.getTimeStamp());
    }
    CHECK (noteOns == (int) m.notes.size(), "note-ons %d vs %d", noteOns, (int) m.notes.size());
    CHECK (name.startsWith ("Lucky Notes"), "track name '%s'", name.toRawUTF8());
    CHECK (maxNoteEnd <= bars * 4.0 * kTicksPerQuarter + 0.5, "note beyond end %.1f", maxNoteEnd);
    CHECK (std::abs (lastTick - bars * 4.0 * kTicksPerQuarter) < 0.5, "length %.1f ticks", lastTick);

    // chords file
    const auto cb = chordsMidi (m, bpm, key);
    juce::MemoryInputStream cin (cb.data(), cb.size(), false);
    juce::MidiFile cf;
    CHECK (cf.readFrom (cin), "chords MIDI does not parse");
    if (cf.getNumTracks() == 2)
    {
        int ons = 0;
        for (auto* e : *cf.getTrack (1))
            if (e->message.isNoteOn()) { ++ons; CHECK (pitchInScale (key, e->message.getNoteNumber()), "chord note out of scale"); }
        CHECK (ons == bars * 3, "chord notes %d", ons);
    }
}

static void testMelodies()
{
    const double bpm = 150.0;
    std::printf ("-- melody sets\n");
    double sumNotes[kNumMelodies] {};
    int sets = 0;
    for (int bars : { 8, 16 })
        for (int root = 0; root < 12; ++root)
            for (bool minor : { true, false })
                for (uint32_t seed : { 1u, 777u, 0xC0FFEEu })
                {
                    SetSettings s;
                    s.key = { root, minor };
                    s.bars = bars;
                    s.seed = seed + (uint32_t) root * 31u;
                    const MelodySet set = generateSet (s);
                    ++sets;
                    CHECK (set.melodies.size() == 12, "count");
                    std::set<std::string> names;
                    for (int i = 0; i < kNumMelodies; ++i)
                    {
                        const Melody& m = set.melodies[(size_t) i];
                        names.insert (m.characterName);
                        sumNotes[i] += (double) m.notes.size();
                        CHECK (m.index == i, "index");
                        CHECK (std::abs (m.lengthBeats - bars * 4.0) < 1e-9, "length %.3f", m.lengthBeats);
                        CHECK (m.notes.size() >= 6, "row %d only %d notes", i, (int) m.notes.size());
                        CHECK (m.notes.size() <= (size_t) PreviewSequencer::kMaxNotes, "too many notes");
                        int lo = 127, hi = 0;
                        for (size_t n = 0; n < m.notes.size(); ++n)
                        {
                            const Note& x = m.notes[n];
                            lo = std::min (lo, x.pitch); hi = std::max (hi, x.pitch);
                            CHECK (pitchInScale (s.key, x.pitch), "pitch %d not in %s (row %d)", x.pitch, keyShortName (s.key).c_str(), i);
                            CHECK (x.pitch >= 48 && x.pitch <= 91, "pitch %d out of range", x.pitch);
                            CHECK (x.start >= 0 && x.length > 0 && x.start + x.length <= m.lengthBeats + 1e-9,
                                   "note %.3f+%.3f outside %0.1f", x.start, x.length, m.lengthBeats);
                            CHECK (x.velocity >= 1 && x.velocity <= 127, "velocity");
                            if (n + 1 < m.notes.size())
                                CHECK (x.start + x.length <= m.notes[n + 1].start + 1e-9, "overlap row %d at %.3f", i, x.start);
                        }
                        CHECK (hi - lo >= 3, "row %d range too small (%d..%d)", i, lo, hi);
                        CHECK (! m.notes.empty() && m.notes.front().start < 1e-9, "row %d does not start on beat 1", i);
                        CHECK (m.chords.size() == (size_t) bars * 3, "chords");
                        if (root % 4 == 0 && seed == 1u) checkMidi (m, bpm, s.key, bars);
                        for (int j = 0; j < i; ++j)
                            CHECK (! sameNotes (m.notes, set.melodies[(size_t) j].notes), "rows %d and %d are identical", i, j);
                    }
                    CHECK (names.size() == 12, "%d distinct characters", (int) names.size());

                    // deterministic per seed
                    const MelodySet again = generateSet (s);
                    for (int i = 0; i < kNumMelodies; ++i)
                        CHECK (sameNotes (set.melodies[(size_t) i].notes, again.melodies[(size_t) i].notes), "not deterministic row %d", i);

                    // another seed -> other melodies
                    SetSettings s2 = s;
                    s2.seed = s.seed + 1;
                    const MelodySet other = generateSet (s2);
                    int same = 0;
                    for (int i = 0; i < kNumMelodies; ++i)
                        same += sameNotes (set.melodies[(size_t) i].notes, other.melodies[(size_t) i].notes) ? 1 : 0;
                    CHECK (same == 0, "%d rows unchanged by a new seed", same);

                    // re-roll changes exactly that row
                    SetSettings s3 = s;
                    s3.rolls[3] = 1;
                    const MelodySet rolled = generateSet (s3);
                    for (int i = 0; i < kNumMelodies; ++i)
                    {
                        const bool eq = sameNotes (set.melodies[(size_t) i].notes, rolled.melodies[(size_t) i].notes);
                        CHECK (i == 3 ? ! eq : eq, "re-roll row 3 affected row %d", i);
                    }
                }
    std::printf ("   %d sets checked; average notes per row:", sets);
    for (int i = 0; i < kNumMelodies; ++i) std::printf (" %s=%.1f", characterName (i), sumNotes[i] / sets);
    std::printf ("\n");

    CHECK (midiFileName (6, 150.0, { 5, true }, 8) == "LuckyNotes_07_150BPM_Fmin_8bars.mid", "%s", midiFileName (6, 150.0, { 5, true }, 8).c_str());
    CHECK (midiFileName (0, 142.5, { 1, false }, 16, true) == "LuckyNotes_01_142.5BPM_C#maj_16bars_Chords.mid", "%s",
           midiFileName (0, 142.5, { 1, false }, 16, true).c_str());
    CHECK (bpmText (149.98) == "150", "bpm text");

    // non-integer tempo survives the round trip
    SetSettings s;
    s.key = { 2, true };
    const auto m = generateMelody (s, 0);
    checkMidi (m, 97.5, s.key, 8);

    // print one set so a human can eyeball it in the CI log
    s.key = { 5, true };
    s.seed = 7;
    const auto set = generateSet (s);
    for (const auto& mel : set.melodies)
    {
        std::printf ("   %02d %-12s", mel.index + 1, mel.characterName.c_str());
        for (size_t i = 0; i < std::min<size_t> (mel.notes.size(), 14); ++i)
            std::printf (" %s%d@%.2f", noteName (mel.notes[i].pitch), mel.notes[i].pitch / 12 - 1, mel.notes[i].start);
        std::printf (" ... (%d notes)\n", (int) mel.notes.size());
    }
}

// --------------------------------------------------------------------------------------------------------------
struct RenderStats
{
    float peak = 0, maxStep = 0, maxStepNoOnset = 0;
    bool finite = true;
    int noteOns = 0;
};

static RenderStats renderMelody (const Melody& m, Wave w, double sr, int block, double bpm, double seconds,
                                 bool withJump = false, double stopAt = -1.0, double waveChangeAt = -1.0)
{
    PreviewEngine eng;
    eng.synth.setWave (w);
    eng.prepare (sr);
    eng.seq.setPattern (m.notes.data(), (int) m.notes.size(), m.lengthBeats);
    const double bps = bpm / 60.0 / sr;
    const int total = (int) (seconds * sr);
    std::vector<float> out ((size_t) total, 0.0f);
    std::vector<char> onset ((size_t) total, 0);
    NoteEvent ev[256];
    double beat = 0;
    bool jumped = false;
    for (int pos = 0; pos < total; pos += block)
    {
        const int n = std::min (block, total - pos);
        const double t = pos / sr;
        const bool playing = stopAt < 0 || t < stopAt;
        if (waveChangeAt >= 0 && t >= waveChangeAt) eng.synth.setWave (Wave::Triangle);
        if (withJump && ! jumped && beat > 5.9) { beat = 0.37; jumped = true; }   // host loop / relocate mid-note
        const int ne = eng.render (out.data() + pos, n, beat, bps, playing, ev, 256);
        for (int i = 0; i < ne; ++i)
            if (ev[i].on) { for (int k = -2; k <= 2; ++k) { const int p = pos + ev[i].sampleOffset + k; if (p >= 0 && p < total) onset[(size_t) p] = 1; } }
        beat += n * bps;
    }

    RenderStats st;
    float prev = 0;
    for (int i = 0; i < total; ++i)
    {
        const float x = out[(size_t) i];
        if (! std::isfinite (x)) st.finite = false;
        if (x != 0.0f && std::abs (x) < 1.0e-30f) st.finite = false; // denormal
        st.peak = std::max (st.peak, std::abs (x));
        const float step = std::abs (x - prev);
        st.maxStep = std::max (st.maxStep, step);
        if (! onset[(size_t) i]) st.maxStepNoOnset = std::max (st.maxStepNoOnset, step);
        prev = x;
    }
    return st;
}

static int countNoteOns (const Melody& m, double sr, double bpm)
{
    PreviewEngine eng;
    eng.prepare (sr);
    eng.seq.setPattern (m.notes.data(), (int) m.notes.size(), m.lengthBeats);
    const double bps = bpm / 60.0 / sr;
    std::vector<float> buf (512);
    NoteEvent ev[256];
    int ons = 0;
    const int total = (int) std::floor ((m.lengthBeats - 1e-9) / bps); // exactly one loop: [0, length)
    for (int pos = 0; pos < total; pos += 512)
    {
        const int n = std::min (512, total - pos);
        const int ne = eng.render (buf.data(), n, pos * bps, bps, true, ev, 256);
        for (int i = 0; i < ne; ++i) ons += ev[i].on ? 1 : 0;
    }
    return ons;
}

static void testPreview()
{
    std::printf ("-- preview synth\n");
    const float minus3dB = std::pow (10.0f, -3.0f / 20.0f);
    SetSettings s;
    s.key = { 0, false };  // C major: the highest notes of the set
    s.seed = 99;
    const MelodySet set = generateSet (s);
    const char* waveNames[] = { "MORSE", "8-BIT", "TV TEST", "ENVELOPE" };

    for (double sr : { 44100.0, 48000.0, 96000.0 })
        for (int block : { 512, 37 })
            for (Wave w : { Wave::Sine, Wave::Square, Wave::Triangle, Wave::EnvelopeOnly })
            {
                RenderStats worst;
                for (const auto& m : set.melodies)
                {
                    const auto st = renderMelody (m, w, sr, block, 150.0, 7.0);
                    worst.peak = std::max (worst.peak, st.peak);
                    worst.maxStep = std::max (worst.maxStep, st.maxStep);
                    worst.maxStepNoOnset = std::max (worst.maxStepNoOnset, st.maxStepNoOnset);
                    worst.finite = worst.finite && st.finite;
                }
                // envelope: >= 3 ms attack -> per-sample change <= gain / (0.003 * sr)
                const float envLimit = (float) (0.30 / (0.003 * sr)) * 1.05f;
                // waveform: max slope of the highest note (<= C6 + margin) plus envelope
                const float waveLimit = (float) (0.34 * 2.0 * 3.14159265 * 1400.0 / sr) + envLimit;
                std::printf ("   %-8s sr=%-6.0f block=%-3d peak=%.3f (%.2f dBFS) maxStep=%.4f (no onsets %.4f)\n",
                             waveNames[(int) w], sr, block, worst.peak, 20.0 * std::log10 (std::max (worst.peak, 1e-9f)),
                             worst.maxStep, worst.maxStepNoOnset);
                CHECK (worst.finite, "NaN / denormal output");
                CHECK (worst.peak <= minus3dB, "%s peak %.4f above -3 dBFS", waveNames[(int) w], worst.peak);
                CHECK (worst.peak >= 0.05f, "%s too quiet (%.4f)", waveNames[(int) w], worst.peak);
                if (w == Wave::EnvelopeOnly)
                    CHECK (worst.maxStep <= envLimit, "envelope step %.5f > %.5f (click)", worst.maxStep, envLimit);
                else if (w != Wave::Square)  // a square wave has (band-limited) edges by design; its envelope is checked above
                    CHECK (worst.maxStepNoOnset <= waveLimit, "%s step %.4f > %.4f (click)", waveNames[(int) w], worst.maxStepNoOnset, waveLimit);
            }

    // stop mid-note, host relocation mid-note, sound switch mid-note: still click-free
    for (const auto& m : set.melodies)
    {
        const float lim = (float) (0.34 * 2.0 * 3.14159265 * 1400.0 / 48000.0) + (float) (0.30 / (0.003 * 48000.0)) * 1.05f;
        auto a = renderMelody (m, Wave::Sine, 48000.0, 256, 150.0, 4.0, false, 1.37);
        CHECK (a.maxStepNoOnset <= lim, "stop click %.4f", a.maxStepNoOnset);
        auto e = renderMelody (m, Wave::EnvelopeOnly, 48000.0, 256, 150.0, 4.0, false, 1.37);
        CHECK (e.maxStep <= (float) (0.30 / (0.003 * 48000.0)) * 1.05f, "stop envelope click %.5f", e.maxStep);
        auto b = renderMelody (m, Wave::Sine, 48000.0, 256, 150.0, 5.0, true);
        CHECK (b.maxStepNoOnset <= lim, "relocate click %.4f", b.maxStepNoOnset);
        auto c = renderMelody (m, Wave::Sine, 48000.0, 256, 150.0, 4.0, false, -1.0, 1.01);
        CHECK (c.maxStepNoOnset <= lim, "sound switch click %.4f", c.maxStepNoOnset);
        CHECK (countNoteOns (m, 48000.0, 150.0) == (int) m.notes.size(), "row %d: preview plays %d of %d notes",
               m.index, countNoteOns (m, 48000.0, 150.0), (int) m.notes.size());
    }

    // silence after stop
    {
        PreviewEngine eng;
        eng.prepare (48000.0);
        const auto& m = set.melodies[0];
        eng.seq.setPattern (m.notes.data(), (int) m.notes.size(), m.lengthBeats);
        std::vector<float> buf (480);
        eng.render (buf.data(), 480, 0.0, 150.0 / 60.0 / 48000.0, true, nullptr, 0);
        for (int i = 0; i < 20; ++i) eng.render (buf.data(), 480, 0.0, 150.0 / 60.0 / 48000.0, false, nullptr, 0);
        CHECK (eng.synth.isSilent(), "voices still sounding 200 ms after stop");
        float mx = 0; for (float x : buf) mx = std::max (mx, std::abs (x));
        CHECK (mx == 0.0f, "output not silent after stop (%.6f)", mx);
    }
}

int main (int argc, char** argv)
{
    juce::ignoreUnused (argc, argv);
    testMelodies();
    testPreview();
    std::printf ("%d checks, %d failures\n", checks, failures);
    std::printf (failures == 0 ? "ALL TESTS PASSED\n" : "TESTS FAILED\n");
    return failures == 0 ? 0 : 1;
}
