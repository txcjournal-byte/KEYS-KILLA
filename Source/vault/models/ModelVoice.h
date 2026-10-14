#pragma once
#include "../VaultTypes.h"
#include <functional>
#include <memory>

// CHROME VAULT physical / modelled instruments ("model" parts).
//
// A model is ONE monophonic voice class (ModelVoice) plus a registry entry (ModelInfo: id, display name, parameter list,
// voice count, factory). The engine owns a pool of `maxVoices` voices per part that plays the model, allocates them
// OFF the audio thread (VaultEngine::serviceResources) the first time a preset needs the model, and does the voice
// allocation, sustain pedal, pitch bend, MOTION vibrato, level / pan / width and the master FX itself.
//
// Rules for a ModelVoice implementation:
//   * prepare() may allocate (called on a non-audio thread, before the voice is ever used). Nothing else may allocate,
//     lock, or do I/O - noteOn / noteOff / setPitch / setParams / render / kill run on the audio thread.
//   * render() ADDS its output into left / right (the buffers already hold other voices). Stereo is allowed.
//   * Level: a velocity 1.0 note at the home note (MIDI 60) should peak around -12..-6 dBFS and never above -1 dBFS.
//     The played fundamental must be the loudest partial (or within 3 dB) unless the model is an inharmonic bell / FX.
//   * isActive() turns false once the voice is silent after noteOff (or kill()); the pool reuses it then.
//   * Deterministic: the same note + params must render the same samples (seed noise from the note / a counter).
//
// Adding a model: write the class (Source/vault/models/MyModel.h/.cpp), then add one line to registerBuiltinModels()
// in Source/vault/models/ModelRegistry.cpp. Presets refer to it by its id (VaultPart::source, kind = model) and set its
// parameters with "m.<name>" keys in VaultPart::values (missing = the declared default).
namespace vault::models
{
constexpr int maxModelParams = 32;

class ModelVoice
{
public:
    virtual ~ModelVoice() = default;

    // non-audio thread, may allocate: sample rate and the largest render() block
    virtual void prepare (double sampleRate, int maxBlockSize) = 0;
    // audio thread: the part's parameter values, in the order of ModelInfo::params (already clamped to lo..hi).
    // Called before every noteOn and whenever the values change (preset switch); `numParams` = ModelInfo::params.size()
    virtual void setParams (const float* values, int numParams) = 0;
    // audio thread: start a note. freqHz already includes the part transpose (bend / vibrato come through setPitch)
    virtual void noteOn (int midiNote, float velocity, float freqHz) = 0;
    // audio thread: key released (after the sustain pedal); the voice decays naturally and then turns inactive
    virtual void noteOff (float releaseVelocity) = 0;
    // audio thread: pitch change while sounding (bend / vibrato), called at most once per 128-sample chunk
    virtual void setPitch (float freqHz) = 0;
    // audio thread: ADD numSamples samples into left / right
    virtual void render (float* left, float* right, int numSamples) = 0;
    // audio thread: silence immediately and reset the state (no allocation)
    virtual void kill() = 0;
    virtual bool isActive() const = 0;
};

struct ModelInfo
{
    juce::String id;                    // stable id used by presets, e.g. "sine", "string.bowed"
    juce::String name;                  // display name
    std::vector<ParamInfo> params;      // ids "m.<name>", ranges, defaults (at most maxModelParams)
    int maxVoices = 16;                 // polyphony of one part
    std::function<std::unique_ptr<ModelVoice>()> create;
};

class ModelRegistry
{
public:
    static ModelRegistry& get();                         // process-wide, filled once with registerBuiltinModels()
    void add (ModelInfo info);                           // message thread, at start-up (before presets are compiled)
    int indexOf (const juce::String& id) const;          // -1 if unknown
    const ModelInfo* find (const juce::String& id) const;
    const ModelInfo& at (int index) const { return models[(size_t) index]; }
    int size() const { return (int) models.size(); }
    juce::StringArray ids() const;

private:
    ModelRegistry();
    std::vector<ModelInfo> models;
};

// the models compiled into CHROME VAULT (ModelRegistry.cpp) - one line per model
void registerBuiltinModels (ModelRegistry&);
} // namespace vault::models
