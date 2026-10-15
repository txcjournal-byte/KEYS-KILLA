#pragma once
#include "VaultTypes.h"
#include "../DspUtil.h"
#include <juce_audio_basics/juce_audio_basics.h>

// CHROME VAULT v3 runtime of the arpeggiator and the gate (allocation-free after prepare). The engine owns one Arp (it
// turns the played keys into notes before the voices) and one Gate per sound slot (on the layer mix, before the FX chain).
// Both follow the host tempo and position when the host plays, else a free-running beat clock. See docs/VAULT.md.
namespace vault
{
class Arp
{
public:
    struct Event { int sample; int note; float velocity; bool on; };   // generated notes (velocity 0..1)
    static constexpr int maxEvents = 512;

    void reset();                                        // nothing held, nothing sounding
    void setSettings (const VaultArp& s)
    {
        if (set.latch && ! s.latch)   // latch off: let go of the keys that are no longer down
            for (int i = numHeld - 1; i >= 0; --i) if (! physDown[held[i].note]) { held[i] = held[--numHeld]; seqDirty = true; }
        if (s.mode != set.mode || s.octaves != set.octaves) seqDirty = true;
        set = s;
    }
    const VaultArp& settings() const { return set; }

    // the played keys (from the MIDI input, in order). sample = position in the current block.
    void keyDown (int note, float velocity);
    void keyUp (int note);
    // n samples starting at beat `beat0` (bps = beats per sample); `hostPlaying` = the beat clock is the host's
    // (steps snap to its grid). Fills `out` (sorted by sample) with note ons / offs, at most maxEvents. Returns the count.
    int process (int n, double beat0, double bps, bool hostPlaying, Event* out);
    // ends every sounding note now (events into `out`), keeps the held keys; returns the count
    int allNotesOff (Event* out, int sample = 0);
    bool anySounding() const { return numSounding > 0; }
    int heldCount() const { return numHeld; }
    bool isRunning() const { return running; }
    int stepIndex() const { return (int) lastStep; }   // the lane of the last step (the UI's step light), -1 = stopped

private:
    struct Held { int note; float vel; uint32_t order; };
    struct Sounding { int note; double offBeat; };
    void buildSequence();
    int pushEvent (Event* out, int count, int sample, int note, float vel, bool on);

    VaultArp set;
    Held held[128] {};
    int numHeld = 0;
    bool physDown[128] {};
    uint32_t orderCounter = 0;
    int seq[128 * 4] {};
    int seqLen = 0, seqPos = 0;
    bool seqDirty = true;
    Sounding sounding[64] {};
    int numSounding = 0;
    bool running = false;
    double nextStepBeat = 0, gridOrigin = 0, lastBeatEnd = -1;
    int64_t stepCounter = 0, lastStep = -1;
    kk::Rng rng;
};

class Gate
{
public:
    void prepare (double sampleRate) { sr = sampleRate; reset(); }
    void reset() { level = 1.0f; }
    // in place; beat0 / bps like Arp::process; depth / smooth: effective values; the mix glides from mixFrom to mixTo
    // over the block (the gate's on / off switch fades)
    void process (float* L, float* R, int n, const VaultGate& g, double beat0, double bps, float depth, float mixFrom, float mixTo, float smooth);
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

// the arp pattern as MIDI (drag & drop): `notes` (key, velocity 0..1) held for `bars` 4/4 bars at `bpm`, 960 ticks per quarter
juce::MidiFile arpPatternToMidi (const VaultArp& settings, const std::vector<std::pair<int, float>>& notes, double bars, double bpm);
} // namespace vault
