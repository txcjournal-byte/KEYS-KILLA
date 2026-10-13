#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VaultEngine.h"

// CHROME VAULT by TrapWin - the plugin. A preset-bank instrument: pick a sound, play.
class VaultProcessor : public juce::AudioProcessor
{
public:
    // parameter IDs (stable - saved in host projects)
    static constexpr const char* idPreset = "presetIndex";   // int 0..maxPresets-1 (automatable)
    static constexpr const char* idMacro[vault::numMacros] { "macro1", "macro2", "macro3", "macro4" };   // TONE SPACE DRIVE MOTION, 0..1
    static constexpr const char* idVolume = "volume";        // dB, -60..+6 (default 0)
    static constexpr int maxPresets = 5000;

    enum class BankSource { embedded, file, factoryFallback };

    VaultProcessor();
    ~VaultProcessor() override;

    //==========================================================================
    // for the editor (message thread)
    const std::vector<VaultPreset>& bank() const { return bankData->presets; }
    BankSource bankSource() const { return bankData->source; }
    juce::String bankDescription() const;          // e.g. "5000 presets (built-in bank)"
    void selectPreset (int index);                 // instant; also resets the 4 macros to the preset's defaults
    int currentPreset() const;                     // index into bank(), always valid while the bank is not empty
    const VaultPreset* currentPresetData() const;
    int findPreset (const juce::String& id) const; // -1 if not in the bank
    float getMacro (int index) const;              // 0..1
    void setMacro (int index, float value01);      // notifies the host
    float getVolumeDb() const;
    void setVolumeDb (float dB);
    void previewNote (int midiNote, float velocity = 0.8f, double seconds = 0.9);   // audition from the UI (any thread)
    float getOutputPeak() const { return outPeak.load(); }   // last block peak (for a meter)
    juce::MidiKeyboardState& getKeyboardState() { return keyboardState; }   // on-screen keyboard
    juce::AudioProcessorValueTreeState apvts;

    // the small bank used when no full bank is embedded: one 1-part preset per EVOLVE factory preset
    static std::vector<VaultPreset> buildFallbackBank();
    static int vaultCategoryOfFactory (int kkCategory);

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CHROME VAULT"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 4.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    struct Bank
    {
        std::vector<VaultPreset> presets;
        std::vector<VaultCompiledPreset> compiled;
        BankSource source = BankSource::factoryFallback;
    };
    static std::shared_ptr<const Bank> sharedBank();   // loaded once per process, shared by all instances

    std::shared_ptr<const Bank> bankData;
    VaultEngine engine;
    juce::MidiKeyboardState keyboardState;
    std::atomic<float>* presetParam = nullptr;
    std::atomic<float>* macroParam[vault::numMacros] {};
    std::atomic<float>* volumeParam = nullptr;
    int loadedIndex = -1;
    std::atomic<int> previewReq { -1 }; std::atomic<float> previewVel { 0.8f }; std::atomic<int> previewLen { 0 };
    int previewActive = -1, previewOffIn = -1;
    std::atomic<float> outPeak { 0.0f };
    double sr = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VaultProcessor)
};
