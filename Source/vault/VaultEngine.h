#pragma once
#include "VaultTypes.h"
#include "../VoiceBuild.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <memory>

// CHROME VAULT sound engine: up to 4 kk::SynthEngine parts (layers) -> one shared master FX chain (kk::FxRack, built from
// part 0's FX values) -> 4 macros -> mono below 120 Hz -> preset gain trim -> master volume -> safety limiter.
// No APVTS: the engine plays VaultPresets directly. Usable offline (render()) without any GUI or plugin wrapper.

// A preset with its parameter IDs resolved to kk::vk keys - built once (message thread / load time), so switching
// presets on the audio thread is allocation-free.
struct VaultCompiledPart
{
    std::vector<std::pair<int, float>> kv;   // (kk::vk key, sanitized real value); keys not listed = default
    float level = 1, pan = 0;
    int transpose = 0;                       // octave * 12 + semi
};
struct VaultCompiledPreset
{
    int numParts = 0;
    std::array<VaultCompiledPart, vault::maxParts> parts;
    float gain = 1;
    float macroDefaults[vault::numMacros] { 0.5f, 0.5f, 0.5f, 0.5f };
};

struct VaultTransport { double bpm = 120.0; double ppq = 0.0; bool playing = false; };   // host tempo / position

class VaultEngine
{
public:
    using Transport = VaultTransport;

    VaultEngine();
    ~VaultEngine();

    static VaultCompiledPreset compile (const VaultPreset& preset);

    // allocates (message thread / before playback). numParts < 4 saves memory for offline renders of small presets.
    // crossfadeSlot = false skips the second (switch crossfade) sound slot: then every setPreset is instant.
    void prepare (double sampleRate, int maxBlockSize, int numPartsToPrepare = vault::maxParts, bool crossfadeSlot = true);
    void releaseResources();
    bool isPrepared() const { return preparedParts > 0; }

    // audio thread (or the single offline thread). Allocation-free: copies the values in; `p` can go away afterwards.
    // instant = false: the new sound starts at once in the second slot (held notes are re-struck with it) and crossfades in
    // over xfadeSeconds (equal power) while the old sound - its voices and its own FX tails - fades out. A switch that
    // arrives during a crossfade waits for it to finish (only the latest one is kept).
    void setPreset (const VaultCompiledPreset& p, bool instant = false);

    // any thread. value 0..1; 0.5 = the sound exactly as designed (macroTone, macroSpace, macroDrive, macroMotion)
    void setMacro (int index, float value01);
    float getMacro (int index) const;
    void setVolume (float linearGain);        // master volume (linear, 0..2), smoothed

    // audio thread: renders `buffer` (1 or 2 channels; it is overwritten) playing `midi`
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Transport& transport = Transport());
    void allNotesOff (bool hard);            // audio thread

    // OFFLINE: one note (note on at 0, note off at 70 % of `seconds`, then the release tail until `seconds`), stereo.
    // Uses the preset's macro defaults (or `macros`, 4 values) and volume 1. Thread-safe (own engine per call).
    static juce::AudioBuffer<float> render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate);
    static juce::AudioBuffer<float> render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate,
                                            const float* macros);
    // OFFLINE, general: render a MIDI sequence (sample positions) of `numSamples` samples
    static juce::AudioBuffer<float> renderMidi (const VaultPreset& preset, const juce::MidiBuffer& midi, int numSamples, double sampleRate,
                                                const float* macros = nullptr);

    // loudness of a render for the bank generator: the loudest 400 ms window RMS (both channels), dBFS.
    // The factory sounds are matched to targetLoudnessDb: preset.gain = dB->gain (target - measured at gain 1).
    static float measureLoudnessDb (const juce::AudioBuffer<float>& buffer, double sampleRate);
    static constexpr float targetLoudnessDb = -15.0f;

    static constexpr int chunk = 128;          // internal block (parameters / macros update at this rate)
    static constexpr float monoBelowHz = 120.0f;     // everything below is mono: side high-pass corner at 160 Hz (8th order)
    static constexpr float sideHighPassHz = 160.0f;
    static constexpr float dcBlockHz = 5.0f;         // 1-pole DC blocker on the mid signal
    static constexpr float xfadeSeconds = 0.040f;    // preset switch crossfade

private:
    struct Part;
    struct Program
    {
        int numParts = 0;
        float values[vault::maxParts][kk::vk::count];
        float level[vault::maxParts] {}, panL[vault::maxParts] {}, panR[vault::maxParts] {};
        int transpose[vault::maxParts] {};
        float gain = 1;
    };
    // One complete sound: a program, its parts (voices + LFO state), its own FX rack (with its own reverb / delay tails)
    // and scratch buffers. Two slots: the current sound and, during a preset switch, the outgoing one fading out.
    struct Slot
    {
        std::unique_ptr<Program> prog;
        std::array<std::unique_ptr<Part>, vault::maxParts> parts;
        std::unique_ptr<kk::FxRack> fx;
        kk::VoiceParams vp[vault::maxParts];
        kk::FxParams fp;
        std::vector<float> busL, busR, busG, partL, partR, lfo1, lfo2;
        bool prepared = false;
    };
    void loadProgram (Program& dst, const VaultCompiledPreset& p);
    void resetSlot (Slot& s, bool clearFx);
    void beginSwitch();
    void switchInstant();
    void updateParams (Slot& s, double beatPos, double bpm);
    void renderLfos (Slot& s, int len, double beat0, double bps, int c0);
    void handleMidi (const juce::MidiMessage& m);
    void noteOnAll (Slot& s, int note, float vel);
    void noteOffAll (Slot& s, int note);
    int numParts (const Slot& s) const { return std::min (s.prog->numParts, preparedParts); }

    double sr = 44100.0, lastBpm = 120.0;
    int preparedParts = 0, maxBlock = 0;
    Slot slots[2];
    int cur = 0;                             // the slot playing the current preset; 1 - cur fades out during a switch
    bool haveXfadeSlot = false;              // false: switches are instant (offline renders prepare only one slot)
    std::unique_ptr<Program> pending;
    bool hasPending = false;

    std::atomic<float> macroTarget[vault::numMacros];
    float macroSm[vault::numMacros] { 0.5f, 0.5f, 0.5f, 0.5f };
    std::atomic<float> volumeTarget { 1.0f };
    float volumeSm = 1.0f;

    // preset switch: equal-power crossfade, old slot cos, new slot sin over xfadeSeconds
    bool xfading = false;
    float xfPos = 0.0f, xfStep = 0.0f;

    // MIDI state
    std::array<float, 128> held {};          // velocity of keys held down (0 = up)
    float pitchWheel = 0, modWheel = 0, aftertouch = 0;
    bool sustain = false;
    double freeBeat = 0;

    // mono below 120 Hz: 8th-order Butterworth high-pass (4 biquads) on the side signal
    struct Biquad2 { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
                     inline float tick (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; } };
    Biquad2 sideHp[4];
    float dcX = 0, dcY = 0, dcA = 0.9993f;   // DC blocker (mid)
    // safety limiter
    float limEnv = 0, limRel = 0.999f;
};
