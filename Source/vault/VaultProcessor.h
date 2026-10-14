#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VaultEngine.h"

// CHROME VAULT by TrapWin - the plugin. A preset-bank instrument: pick a sound, play.
class VaultProcessor : public juce::AudioProcessor, private juce::Timer
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
    void previewNote (int midiNote = vault::homeNote, float velocity = 0.8f, double seconds = 0.9);   // audition from the UI (any thread)

    // sample library (shared by all instances; see vault::SampleLibrary)
    bool libraryFound() const { return library != nullptr && library->libraryFound(); }
    juce::File libraryPath() const { return library != nullptr ? library->libraryPath() : juce::File(); }
    vault::SampleLibrary* sampleLibrary() const { return library.get(); }
    // true while the current preset waits for sample instruments / model voices (its parts are silent meanwhile)
    bool isLoading() const;
    // true if the current preset needs the library but an instrument is missing (no library installed / unknown id)
    bool isMissingSamples() const;
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
        std::shared_ptr<vault::SampleLibrary> library;
    };
    static std::shared_ptr<const Bank> sharedBank();   // loaded once per process, shared by all instances

    void timerCallback() override;
    void updatePins (int presetIndex);

    std::shared_ptr<const Bank> bankData;
    std::shared_ptr<vault::SampleLibrary> library;
    std::vector<vault::InstrumentHandle*> pinned;   // instruments of the current preset kept in RAM (message thread)
    int pinnedFor = -2;
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
