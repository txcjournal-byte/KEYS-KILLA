#pragma once
#include "VaultTypes.h"
#include "../DspUtil.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

// CHROME VAULT v3 runtime of the arpeggiator and the gate (allocation-free after prepare). The engine owns one Arp (it
// turns the played keys into notes before the voices) and one Gate per sound slot (on the layer mix, before the FX chain).
// Both follow the host tempo and position when the host plays, else a free-running beat clock. See docs/VAULT.md.
namespace vault
{
// The one step clock of the arpeggiator and the gate: step k starts at origin + k * len; odd steps start later by
// swing * len / 2 (swing 0..1 = 50 % .. 75 %). Host playing: origin 0 = the song start, so steps, swing and lanes sit on
// the song's grid (and a loop plays the same every time); host stopped: origin = the first key.
struct StepGrid
{
    static double start (int64_t k, double origin, double len, float swing)
    {
        return origin + (double) k * len + ((k & 1) != 0 ? (double) swing * 0.5 * len : 0.0);
    }
    // the step that contains `beat`: start (k) <= beat < start (k + 1)
    static int64_t indexAt (double beat, double origin, double len, float swing)
    {
        const double x = (beat - origin) / len + 1.0e-9;
        const auto pair = (int64_t) std::floor (x * 0.5);
        const double pos = x - (double) pair * 2.0;
        return pair * 2 + (pos < 1.0 + (double) swing * 0.5 ? 0 : 1);
    }
};

class Arp
{
public:
    struct Event { int sample; int note; float velocity; bool on; };   // generated notes (velocity 0..1)
    static constexpr int maxEvents = 1024;   // > 2 x 128: a step's offs + ons always fit
    struct Clock
    {
        double beat0 = 0, bps = 0;           // the beat (quarter notes) at the block's first sample, beats per sample
        bool hostPlaying = false;            // beat0 is the host's song position (steps on its grid) - else a free clock
        double barStart = 0, barBeats = 4;   // a bar line (ppq) and the bar length in beats (host; Every bar retrigger)
        double loopStart = 0, loopEnd = 0;   // the host's loop (ppq) while it loops, else 0 / 0
    };

    void reset();                                        // nothing held, nothing sounding
    void setSettings (const VaultArp& s);
    const VaultArp& settings() const { return set; }

    // the played keys (from the MIDI input, in order) - call between process() calls at the key's sample
    void keyDown (int note, float velocity);
    void keyUp (int note);
    void setPedal (bool down);                           // sustain pedal: released keys stay in the pattern while it is down
    // n samples from `clock` - fills `out` (sorted by sample) with note ons / offs, at most maxEvents; returns the count
    int process (int n, const Clock& clock, Event* out);
    int process (int n, double beat0, double bps, bool hostPlaying, Event* out)
    {
        Clock c; c.beat0 = beat0; c.bps = bps; c.hostPlaying = hostPlaying;
        return process (n, c, out);
    }
    // ends every sounding note now (events into `out`), keeps the held keys; returns the count
    int allNotesOff (Event* out, int sample = 0);
    bool anySounding() const { return numSounding > 0; }
    int heldCount() const { return numHeld; }
    // the pattern's notes (latched ones included) in the order they were played; returns the count
    int heldNotes (int* notes, float* velocities, int max) const;
    uint32_t heldVersion() const { return heldVer; }     // changes whenever the held notes change
    bool isRunning() const { return running; }
    int stepIndex() const { return (int) lastStep; }   // the lane of the last step (the UI's step light), -1 = stopped

    // the notes one pass of the order plays for these held notes (pitch, in played order) - tests / UI
    static int orderOf (const VaultArp& s, const int* notesInPlayedOrder, int n, int* out, int maxOut);

private:
    struct Held { int note; float vel; uint32_t order; };
    void removeHeld (int note);
    void buildSequence();
    void realign (double beat, bool host, bool catchLate);
    int processSpan (int n, const Clock& clock, Event* out, int cap);
    int pushEvent (Event* out, int count, int sample, int note, float vel, bool on);
    int endSounding (Event* out, int count, int sample);

    static constexpr int maxSeq = 2048;
    int eventCap = maxEvents;
    VaultArp set;
    Held held[128] {};
    int numHeld = 0, numPhys = 0;
    bool physDown[128] {};
    bool pedal = false;
    uint32_t orderCounter = 0, heldVer = 0;
    int16_t seq[maxSeq] {};
    float seqVel[maxSeq] {};
    int seqLen = 0, seqPos = 0, chordPass = 0, lastNote = -1;
    bool seqDirty = true, restartPending = false, newChordPending = false;
    bool sounding[128] {};                               // the generated notes that sound now ...
    double offAt[128] {};                                // ... and when they end (beats)
    int numSounding = 0;
    bool joinPending[128] {};                            // Chord: keys that joined just after the step (played at once)
    bool anyJoin = false;
    bool running = false, haveClock = false, lastPlaying = false;
    double origin = 0, gridLen = 0.25, lastBeatEnd = 0, now = 0;
    double freeBarOrigin = 0, lastBarDone = -1.0e18;
    int64_t k = 0, laneBase = 0, lastStep = -1;
    double lastStepBeat = -1.0e18, lastStepOff = 0;
    bool lastStepChord = false;
    int lastStepShift = 0, lastStepLane = 0;
    kk::Rng rng;
    // scratch for buildSequence
    int sortedNotes[128 * 5] {}, base[maxSeq] {};   // held notes + their octave copies (m + 4 m)
    float sortedVel[128 * 5] {};
};

class Gate
{
public:
    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset() { level = 1.0f; }
    // in place; beat0 / bps like Arp::process; depth / smooth: effective values; the mix glides from mixFrom to mixTo
    // over the block (the gate's on / off switch fades)
    // loopStart / loopEnd: the host's loop while it plays one (a wrap inside the block goes back to its start)
    void process (float* L, float* R, int n, const VaultGate& g, double beat0, double bps, float depth, float mixFrom, float mixTo, float smooth,
                  double loopStart = 0.0, double loopEnd = 0.0);
    float currentLevel() const { return level; }
private:
    double sr = 44100;
    float level = 1.0f;
};

// LFO value (-1..1) at phase 0..1 (S&H value supplied)
inline float lfoValue (int shape, float ph, float sh)
{
    switch (shape)
    {
        case VaultLfo::triangle:   return ph < 0.5f ? ph * 4.0f - 1.0f : 3.0f - ph * 4.0f;
        case VaultLfo::square:     return ph < 0.5f ? 1.0f : -1.0f;
        case VaultLfo::saw:        return 1.0f - 2.0f * ph;
        case VaultLfo::sampleHold: return sh;
        default:                   return std::sin (kk::twoPi * ph);
    }
}

// the arp pattern as MIDI (drag & drop): `notes` (key, velocity 0..1, in played order) held for `bars` 4/4 bars at `bpm`,
// 960 ticks per quarter - rendered by the same Arp the engine plays (host grid from bar 1), so the file is what it plays
juce::MidiFile arpPatternToMidi (const VaultArp& settings, const std::vector<std::pair<int, float>>& notes, double bars, double bpm);
} // namespace vault
