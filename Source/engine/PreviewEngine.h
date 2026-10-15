// Lucky Notes engine: preview synth ("beeps") + loop sequencer (plain C++, real-time safe: no allocation in render).
#pragma once

#include "Theory.h"

#include <cmath>

namespace ln
{
enum class Wave { Sine = 0, Square = 1, Triangle = 2, EnvelopeOnly = 3 }; // MORSE / 8-BIT / TV TEST / (tests)

// linear ramp towards a target (like juce::SmoothedValue<float, Linear>)
class LinearSmoother
{
public:
    void reset (double sampleRate, double seconds) { rampSamples = std::max (1, (int) (sampleRate * seconds)); countdown = 0; current = target; }
    void setCurrentAndTarget (float v) { current = target = v; countdown = 0; }
    void setTarget (float v)
    {
        if (v == target) return;
        target = v;
        countdown = rampSamples;
        step = (target - current) / (float) rampSamples;
    }
    float next()
    {
        if (countdown <= 0) return current = target;
        --countdown;
        current += step;
        return current;
    }
    float getTarget() const { return target; }
    float getCurrent() const { return current; }
    bool isSmoothing() const { return countdown > 0; }

private:
    float current = 0, target = 0, step = 0;
    int countdown = 0, rampSamples = 1;
};

class PreviewSynth
{
public:
    static constexpr int kVoices = 6;
    static constexpr float kPeakLimit = 0.70f; // -3.1 dBFS hard safety (never reached in normal use)

    void prepare (double sampleRate);
    void setWave (Wave w) { pendingWave = w; }
    Wave getWave() const { return wave; }

    void noteOn (int pitch, int velocity);
    void noteOff (int pitch);
    void allNotesOff();         // smooth release of everything
    void reset();               // hard reset (not real-time use)

    // replaces out[0..n) with the mono preview signal
    void render (float* out, int numSamples);
    bool isSilent() const;

    double getSampleRate() const { return sr; }

private:
    enum Stage { Off, Attack, Decay, Sustain, Release };
    struct Voice
    {
        Stage stage = Off;
        int pitch = -1;
        double phase = 0, inc = 0;
        float level = 0, attackStep = 0, decayStep = 0, releaseStep = 0, amp = 0;
        uint64_t age = 0;
    };

    float renderVoice (Voice& v);
    static double polyBlep (double t, double dt);

    Voice voices[kVoices];
    double sr = 48000.0;
    Wave wave = Wave::Sine, pendingWave = Wave::Sine;
    LinearSmoother master;
    uint64_t ageCounter = 0;
    float attackSec = 0.005f, decaySec = 0.12f, sustain = 0.62f, releaseSec = 0.07f;
};

struct NoteEvent
{
    int sampleOffset;
    int pitch;
    int velocity;
    bool on;
};

// Loops a melody. Times are in beats; the caller supplies the beat position of the block's first sample
// (host ppqPosition when the transport runs, a free-running counter otherwise).
class PreviewSequencer
{
public:
    static constexpr int kMaxNotes = 600;

    void setPattern (const Note* notes, int count, double lengthBeats);
    void clearPattern() { count = 0; }
    bool hasPattern() const { return count > 0 && length > 0; }

    // emits events for [beatStart, beatStart + numSamples * beatsPerSample) into out (max maxEvents), returns count
    int process (double beatStart, double beatsPerSample, int numSamples, NoteEvent* out, int maxEvents);
    // note-off for the sounding note (if any)
    int stop (NoteEvent* out, int maxEvents);
    void resetPosition() { expected = -1.0e9; }

private:
    struct Ev { double beat; int pitch; int vel; bool on; };
    Ev events[kMaxNotes * 2];
    int count = 0;
    double length = 0;
    int activePitch = -1;
    double expected = -1.0e9;
};

// sequencer + synth: what the plugin and the offline tests run
class PreviewEngine
{
public:
    void prepare (double sampleRate) { synth.prepare (sampleRate); seq.resetPosition(); }
    PreviewSynth synth;
    PreviewSequencer seq;

    // renders one block; events (also usable as MIDI out) are written to evOut if non-null
    int render (float* out, int numSamples, double beatStart, double beatsPerSample, bool playing,
                NoteEvent* evOut, int maxEvents);

private:
    bool wasPlaying = false;
    NoteEvent scratch[256];
};
} // namespace ln
