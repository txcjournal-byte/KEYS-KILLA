#pragma once
#include "VaultTypes.h"
#include "VaultLibrary.h"
#include "VaultSampler.h"
#include "VaultFx.h"
#include "VaultMod.h"
#include "models/ModelVoice.h"
#include "../VoiceBuild.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <memory>
#include <mutex>

// CHROME VAULT sound engine (v3): [arpeggiator] -> up to 4 parts (layers) of any kind - synth (kk::SynthEngine), sample
// (vault::SamplerPart playing a library instrument) or model (a vault::models::ModelVoice pool) -> layer levels / pans
// (+ modulation) -> tremolo / volume / pan modulation -> gate -> the preset's FX chain (vault::FxChain: up to 8 slots +
// the default slots the MAIN knobs need, then the output stage: mono lows, output gain, finish + limiter) -> peak control
// -> preset gain trim -> mono below 120 Hz -> master volume -> pan -> safety limiter.
// Every block: 2 LFOs + mod wheel / velocity / aftertouch routings + the 4 sound macros + the MAIN page quick knobs set
// the effective parameters (control rate, 128 samples, smoothed). No APVTS: the engine plays VaultPresets directly;
// usable offline (render()) without any GUI or plugin wrapper. See docs/VAULT.md.

// a modulation / macro target resolved for the engine
struct VaultCompiledTarget
{
    vault::TargetRef ref;
    vault::FxType fxType = vault::FxType::reverb;   // fx targets: the slot type the target was made for
    float amount = 0, min = 0, max = 1, curve = 0;
};

// A preset with its parameter IDs resolved to kk::vk keys - built once (message thread / load time), so switching
// presets on the audio thread is allocation-free.
struct VaultCompiledPart
{
    VaultPartKind kind = VaultPartKind::synth;
    std::vector<std::pair<int, float>> kv;   // (kk::vk key, sanitized real value); keys not listed = default (synth + FX keys)
    float level = 1, pan = 0;
    int transpose = 0;                       // octave * 12 + semi
    // sample parts
    vault::InstrumentHandle* instrument = nullptr;   // nullptr: unknown id / no library -> the part is silent
    int articulation = 0;
    std::array<float, vault::numSampleParams> sp {}; // sanitized sample parameters (defaults filled in)
    // model parts
    int model = -1;                                  // index in vault::models::ModelRegistry, -1 = unknown -> silent
    int numModelParams = 0;
    std::array<float, vault::models::maxModelParams> mp {};
};

// the v3 sections, fixed size (copied on the audio thread without allocating)
struct VaultSoundControl
{
    int numFx = 0;
    std::array<VaultFxSlot, vault::maxFxSlots> fx {};
    float finish = 0;
    struct Lfo
    {
        uint8_t shape = 0; float rate = 2; bool sync = false; uint8_t div = 8; float delay = 0, fade = 0, phase = 0; bool retrig = true; float depth = 1;
        int numDests = 0; std::array<VaultCompiledTarget, vault::maxLfoDests> dests {};
    };
    std::array<Lfo, vault::numLfos> lfo {};
    int numMw = 0, numVel = 0, numAt = 0;
    std::array<VaultCompiledTarget, vault::maxRoutings> mw {}, vel {}, at {};
    std::array<int, vault::numSoundMacros> numMacroTargets {};
    std::array<std::array<VaultCompiledTarget, vault::maxMacroTargets>, vault::numSoundMacros> macro {};
    VaultArp arp;
    VaultGate gate;
};

struct VaultCompiledPreset
{
    int numParts = 0;
    std::array<VaultCompiledPart, vault::maxParts> parts;
    float gain = 1;
    float peakThresh = 0;                            // peak control threshold (linear, before the trim), 0 = off (vault::peakKey)
    float peakVelExp = 0;                            // the threshold follows the last note-on velocity: thr * vel^exp (vault::peakVelKey)
    float macroDefaults[vault::numMacros] { 0.5f, 0.5f, 0.5f, 0.5f };
    VaultSoundControl ctl;
    std::shared_ptr<vault::SampleLibrary> library;   // keeps the instrument handles alive
};

struct VaultTransport { double bpm = 120.0; double ppq = 0.0; bool playing = false; };   // host tempo / position

class VaultEngine
{
public:
    using Transport = VaultTransport;

    VaultEngine();
    ~VaultEngine();

    // resolves sample instrument ids against `library` (nullptr = vault::SampleLibrary::getShared()) and model ids
    // against the model registry; a v1 / v2 style preset is converted (vault::upgradeToV3) first. Message thread / load time.
    static VaultCompiledPreset compile (const VaultPreset& preset, std::shared_ptr<vault::SampleLibrary> library = nullptr);

    // allocates (message thread / before playback). numParts < 4 saves memory for offline renders of small presets.
    // crossfadeSlot = false skips the second (switch crossfade) sound slot: then every setPreset is instant.
    void prepare (double sampleRate, int maxBlockSize, int numPartsToPrepare = vault::maxParts, bool crossfadeSlot = true);
    void releaseResources();
    bool isPrepared() const { return preparedParts > 0; }

    // audio thread (or the single offline thread). Allocation-free: copies the values in; `p` can go away afterwards.
    // instant = false: the new sound starts at once in the second slot (held notes are re-struck with it) and crossfades in
    // over xfadeSeconds (equal power) while the old sound - its voices and its own FX tails - fades out. A switch that
    // arrives during a crossfade waits for it to finish (only the latest one is kept).
    // "Keep while browsing" (setLocks): a locked section (FX chain / arpeggiator / gate) stays as it is.
    // applyLocks = false: the preset already carries the sections it should have (a restored / edited sound)
    void setPreset (const VaultCompiledPreset& p, bool instant = false, bool applyLocks = true);
    // audio thread: an edit of the CURRENT sound (FX / modulation / macro targets / arp / gate / layer levels and values):
    // applied at the next block without re-striking notes. If the layers changed (kind, instrument, model, count) it
    // becomes a crossfaded preset switch.
    void updateSound (const VaultCompiledPreset& p);

    // any thread. value 0..1: the 4 sound macros (the preset's macroDefaults = as designed)
    void setMacro (int index, float value01);
    float getMacro (int index) const;
    // any thread: the MAIN page quick knobs (vault::Quick), 0..1, vault::quickInfo()[i].neutral = as designed
    void setQuick (int index, float value01);
    float getQuick (int index) const;
    void setVolume (float linearGain);        // master volume (linear, 0..2), smoothed
    void setFxOn (bool on);                   // all FX slots (crossfaded); the output stage stays
    void setArpOn (bool on);
    void setGateOn (bool on);
    void setLocks (bool fx, bool arp, bool gate);
    bool isFxOn() const { return fxOn.load(); }
    bool isArpOn() const { return arpOn.load(); }
    bool isGateOn() const { return gateOn.load(); }

    // audio thread: renders `buffer` (1 or 2 channels; it is overwritten) playing `midi`
    void process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Transport& transport = Transport());
    void allNotesOff (bool hard);            // audio thread

    // NON-audio thread (the processor's timer, offline renders): creates the model voice pools that the current presets
    // asked for (a model part is silent until its pool exists). Sample instruments are loaded by vault::SampleLibrary.
    void serviceResources();
    // any thread: true while a part of the current preset waits for its instrument / model voices
    bool isLoading() const { return loading.load (std::memory_order_relaxed); }

    // ---- state for the UI / tests (audio thread values, read anywhere; approximate while playing)
    int arpStep() const { return arpStepUi.load (std::memory_order_relaxed); }       // -1 = not running
    float lfoPhase (int i) const { return juce::isPositiveAndBelow (i, (int) vault::numLfos) ? lfoPhaseUi[i].load() : 0.0f; }
    float lfoOutput (int i) const { return juce::isPositiveAndBelow (i, (int) vault::numLfos) ? lfoValueUi[i].load() : 0.0f; }
    // the current sound's effective FX chain (after locks, default slots, macros, quick knobs, modulation)
    int chainSize() const;
    vault::FxType chainType (int entry) const;
    int chainSlot (int entry) const;          // the preset's slot index, -1 = a default slot
    float chainParam (int entry, int param) const;
    float chainOnGain (int entry) const;
    float effectiveFinish() const;
    // tests: the last notes the voices got (arp output included); sample = engine sample clock
    struct NoteLog { int64_t sample; int note; float velocity; bool on; };
    int readNoteLog (NoteLog* out, int maxCount) const;
    void clearNoteLog() { noteLogCount = 0; }

    // OFFLINE: one note (note on at 0, note off at 70 % of `seconds`, then the release tail until `seconds`), stereo.
    // Uses the preset's macro defaults (or `macros`, 4 values), volume 1, the preset's arp / gate switches. Thread-safe.
    static juce::AudioBuffer<float> render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate);
    static juce::AudioBuffer<float> render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate,
                                            const float* macros);
    // OFFLINE, general: render a MIDI sequence (sample positions) of `numSamples` samples
    static juce::AudioBuffer<float> renderMidi (const VaultPreset& preset, const juce::MidiBuffer& midi, int numSamples, double sampleRate,
                                                const float* macros = nullptr);
    // the same with an explicit library (tests / tools); the instruments are loaded synchronously first
    static juce::AudioBuffer<float> renderMidi (const VaultPreset& preset, const juce::MidiBuffer& midi, int numSamples, double sampleRate,
                                                const float* macros, std::shared_ptr<vault::SampleLibrary> library);

    // loudness of a render for the bank generator: the loudest 400 ms window RMS (both channels), dBFS.
    // The factory sounds are matched to targetLoudnessDb: preset.gain = dB->gain (target - measured at gain 1).
    static float measureLoudnessDb (const juce::AudioBuffer<float>& buffer, double sampleRate);
    static constexpr float targetLoudnessDb = -15.0f;

    static constexpr int chunk = 128;          // internal block (parameters / macros / LFOs update at this rate)
    static constexpr float monoBelowHz = 120.0f;     // everything below is mono: side high-pass corner at 160 Hz (8th order)
    static constexpr float sideHighPassHz = 160.0f;
    static constexpr float dcBlockHz = 5.0f;         // 1-pole DC blocker on the mid signal
    static constexpr float xfadeSeconds = 0.040f;    // preset switch crossfade

private:
    struct Part;
    struct ModelPool;
    static constexpr int maxEntries = vault::FxChain::maxEntries;
    struct Program
    {
        int numParts = 0;
        float values[vault::maxParts][kk::vk::count];
        float level[vault::maxParts] {}, pan[vault::maxParts] {}, panL[vault::maxParts] {}, panR[vault::maxParts] {};
        int transpose[vault::maxParts] {};
        VaultPartKind kind[vault::maxParts] {};
        vault::InstrumentHandle* instrument[vault::maxParts] {};
        int articulation[vault::maxParts] {};
        float sp[vault::maxParts][vault::numSampleParams] {};
        int model[vault::maxParts] {};
        int numModelParams[vault::maxParts] {};
        float mp[vault::maxParts][vault::models::maxModelParams] {};
        float gain = 1;
        float peakThresh = 0, peakVelExp = 0;
        // v3
        VaultSoundControl ctl;
        bool monoLows = false, anyBass = false;
        int numEntries = 0;
        vault::FxChain::Entry entries[maxEntries] {};   // the effective chain with the base parameters
        int firstOfType[vault::numFxTypes] {};          // entry index, -1 = none
        int entryOfSlot[vault::maxFxSlots] {};
        // resolved targets: entry index for fx targets (-1 = dropped)
        int8_t lfoEntry[vault::numLfos][vault::maxLfoDests] {};
        int8_t mwEntry[vault::maxRoutings] {}, velEntry[vault::maxRoutings] {}, atEntry[vault::maxRoutings] {};
        int8_t macroEntry[vault::numSoundMacros][vault::maxMacroTargets] {};
    };
    // per-block effective values (scratch, one per slot)
    struct Effective
    {
        float glob[5] {};                              // pitch (st), cutoff (oct), reso (+), volume (x 1 + v), pan (+)
        float quick[vault::numQuick] {};               // effective quick knobs
        float classic[4] {};
        float layerAbs[vault::maxParts][2] {}; bool layerAbsSet[vault::maxParts][2] {};
        float layerAdd[vault::maxParts][vault::TargetRef::numLayerParams] {};
        float fx[maxEntries][vault::maxFxParams] {};
        float fxNorm[maxEntries][vault::maxFxParams] {};
        bool fxTouched[maxEntries] {};
        float lfoDepth[vault::numLfos] {}, lfoRate[vault::numLfos] {};   // abs (macro) or base, then adds
        float lfoDepthAdd[vault::numLfos] {}, lfoRateAdd[vault::numLfos] {};
        float gate[3] {}, arpGate = 0.8f, finish = 0;
    };
    // One complete sound: a program, its parts (voices + LFO state), its own FX chain (with its own reverb / delay tails)
    // and scratch buffers. Two slots: the current sound and, during a preset switch, the outgoing one fading out.
    struct Slot
    {
        std::unique_ptr<Program> prog;
        std::array<std::unique_ptr<Part>, vault::maxParts> parts;
        std::unique_ptr<vault::FxChain> chain;
        std::unique_ptr<Effective> eff;
        kk::VoiceParams vp[vault::maxParts];
        std::vector<float> busL, busR, busG, partL, partR, lfo1, lfo2;
        bool prepared = false;
        vault::InstrumentHandle* held[vault::maxParts] {};   // instruments acquired by this slot's program
        float peakEnv = 0, peakRaw = 0, peakThrSm = 0; int peakHold = 0;   // peak control state (vault::peakKey)
        static constexpr int peakDelayMax = 256;                              // look-ahead delay line (1 ms, up to 192 kHz)
        std::array<std::array<float, peakDelayMax>, 2> peakDl {}; int peakPos = 0;
        // v3 modulation state
        struct LfoState { double phase = 0; float sh = 0, lastPh = 0, value = 0, prevValue = 0; } lfo[vault::numLfos];
        double trigBeat = 0; double trigTime = 1.0e9;    // the last "first note" (LFO retrigger, delay / fade)
        float gl[vault::maxParts] {}, gr[vault::maxParts] {};   // layer gains applied in the last block (ramps)
        bool gainsValid = false;
        float volMod = 1, panMod = 0, volModPrev = 1, panModPrev = 0;
        float vibPhase = 0, tremPhase = 0;
        vault::Gate gate;
        float gateMix = 0;                               // gate mix glide (0 = off)
    };
    void slotAcquire (Slot& s);
    void slotRelease (Slot& s);
    ModelPool* poolFor (Slot& s, int part) const;
    void loadProgram (Program& dst, const VaultCompiledPreset& p, bool applyLocks);
    void resolveProgram (Program& dst);
    void resetSlot (Slot& s, bool clearFx);
    void beginSwitch();
    void switchInstant();
    void applyEdit();
    void computeEffective (Slot& s, double beat, double bpm, bool advance);
    void updateParams (Slot& s, double beatPos, double bpm, bool advance = true);   // advance = a block plays (free LFOs move)
    void renderLfos (Slot& s, int len, double beat0, double bps, int c0);
    void handleNote (bool on, int note, float vel);
    void handleOther (const juce::MidiMessage& m);
    void noteOnAll (Slot& s, int note, float vel);
    void noteOffAll (Slot& s, int note);
    void logNote (int note, float vel, bool on);
    int numParts (const Slot& s) const { return std::min (s.prog->numParts, preparedParts); }

    double sr = 44100.0, lastBpm = 120.0;
    int preparedParts = 0, maxBlock = 0;
    Slot slots[2];
    int cur = 0;                             // the slot playing the current preset; 1 - cur fades out during a switch
    bool haveXfadeSlot = false;              // false: switches are instant (offline renders prepare only one slot)
    std::unique_ptr<Program> pending, editProg;
    bool hasPending = false, hasEdit = false;

    std::atomic<float> macroTarget[vault::numMacros];
    float macroSm[vault::numMacros] { 0.5f, 0.5f, 0.5f, 0.5f };
    std::atomic<float> quickTarget[vault::numQuick];
    float quickSm[vault::numQuick] {};
    std::atomic<float> volumeTarget { 1.0f };
    float volumeSm = 1.0f;
    std::atomic<bool> fxOn { true }, arpOn { false }, gateOn { false }, lockFx { false }, lockArp { false }, lockGate { false };
    float panSmL = 1.0f, panSmR = 1.0f;

    // preset switch: equal-power crossfade, old slot cos, new slot sin over xfadeSeconds
    bool xfading = false;
    float xfPos = 0.0f, xfStep = 0.0f;

    // MIDI state
    std::array<float, 128> held {};          // velocity of the notes the voices play (arp output when the arp runs)
    std::array<float, 128> keysDown {};      // velocity of the keys physically down
    float lastVelocity = 0.8f;               // the last note-on velocity (peak control threshold, velocity routings)
    float pitchWheel = 0, modWheel = 0, aftertouch = 0;
    bool sustain = false;
    double freeBeat = 0, clockBeat = 0, clockBps = 0;
    bool clockPlaying = false;
    int64_t sampleClock = 0, eventClock = 0;

    // the arpeggiator (MIDI before the voices) and the block's note / MIDI events
    vault::Arp arp;
    bool arpWasOn = false;
    struct Ev { int sample; uint8_t type; int note; float vel; uint8_t b[3]; int len; };   // type 0 note on, 1 note off, 2 other
    std::vector<Ev> evs;
    std::vector<vault::Arp::Event> arpEvs;

    // mono below 120 Hz: 8th-order Butterworth high-pass (4 biquads) on the side signal
    struct Biquad2 { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
                     inline float tick (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; } };
    Biquad2 sideHp[4];
    float dcX = 0, dcY = 0, dcA = 0.9993f;   // DC blocker (mid)
    // safety limiter
    float limEnv = 0, limRel = 0.999f;
    std::atomic<bool> loading { false };
    std::atomic<int> arpStepUi { -1 };
    std::atomic<float> lfoPhaseUi[vault::numLfos], lfoValueUi[vault::numLfos];
    static constexpr int noteLogSize = 512;
    NoteLog noteLog[noteLogSize] {};
    int noteLogCount = 0;
    std::mutex resourceLock;                 // serviceResources / prepare / releaseResources (never the audio thread)
};
