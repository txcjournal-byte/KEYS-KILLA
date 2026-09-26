#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Synth.h"
#include "Fx.h"
#include "Presets.h"

class KeysKillaProcessor : public juce::AudioProcessor
{
public:
    KeysKillaProcessor();
    ~KeysKillaProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "KEYS KILLA"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 8.0; }

    int getNumPrograms() override { return (int) factoryPresets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, currentPreset); }
    void setCurrentProgram (int index) override { loadPreset (index); }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- message-thread API used by the editor ---
    void loadPreset (int index);
    void initPatch();
    void rollDice (int tile);
    bool undoDice();
    juce::String currentName() const { return presetName; }
    int  currentPresetIndex() const { return currentPreset; }
    bool saveUserPreset (const juce::File& f);
    bool loadUserPreset (const juce::File& f);

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    std::atomic<float> meterL { 0 }, meterR { 0 };
    std::atomic<bool>  overload { false };
    std::atomic<float> guiPitch { 0 }, guiMod { 0 };   // from on-screen wheels
    std::array<std::atomic<bool>, 128> playing {};
    int uiTile = -1, uiEra = -1; bool uiExclusive = false;   // browser filter (kept while editor is closed)

private:
    void buildVoiceParams (kk::VoiceParams& vp, kk::FxParams& fp);
    void handleMidi (const juce::MidiMessage& m);
    void expandAndArp (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, double beatPos, double bpm, bool playingHost);
    void syncParamsToState();
    void applyValues (const std::vector<std::pair<const char*, float>>& values);

    std::vector<std::atomic<float>*> paramCache;
    kk::SynthEngine synth;
    kk::FxRack fx;
    kk::VoiceParams vp;
    kk::FxParams fp;

    std::vector<float> bufL, bufR, bufG, lfoBuf;
    juce::MidiBuffer processedMidi;
    double sr = 44100;
    float lfoPhase = 0;
    double freeBeat = 0;
    float midiPitch = 0, midiMod = 0;

    // arp / chord
    bool lastChord = false, lastArp = false;
    std::array<bool, 128> arpHeld {};
    int arpIndex = 0, arpNote = -1;
    int64_t arpLastStep = -1;

    int currentPreset = -1;
    juce::String presetName { "Init" };
    std::vector<juce::ValueTree> diceHistory;
    int diceCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeysKillaProcessor)
};
