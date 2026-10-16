#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "VaultEngine.h"
#include <functional>

// CHROME VAULT by TrapWin - the plugin. A preset-bank instrument: pick a sound, play.
class VaultProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    // parameter IDs (stable - saved in host projects)
    static constexpr const char* idPreset = "presetIndex";   // int 0..maxPresets-1 (automatable)
    static constexpr const char* idMacro[vault::numMacros] { "macro1", "macro2", "macro3", "macro4" };   // the 4 sound macros, 0..1
    static constexpr const char* idVolume = "volume";        // dB, -60..+6 (default 0)
    // v3: the MAIN page knobs (vault::Quick order; 0..1, vault::quickInfo()[i].neutral = as designed) + the quick switches
    static constexpr const char* idQuick[vault::numQuick] { "qCutoff", "qReso", "qAttack", "qRelease", "qEnvAmt", "qGlide", "qVibrato",
                                                            "qTremolo", "qReverb", "qDelay", "qChorus", "qDrive", "qWidth", "qEqTilt", "qPan" };
    static constexpr const char* idFxOn = "fxOn";             // bool, default on: every FX slot (the output stage stays)
    static constexpr const char* idArpOn = "arpOn";           // bool: the arpeggiator (a preset that uses it turns it on)
    static constexpr const char* idGateOn = "gateOn";         // bool: the gate
    static constexpr int maxPresets = 5000;
    enum class Section { fx, arp, gate };                     // "keep while browsing" locks

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

    //==========================================================================
    // v3 editor API (message thread)
    // The current sound: the selected preset + every edit (FX chain, modulation, macro targets, arp / gate, layers).
    // Saved with the host project; a preset switch starts from the bank preset again (keeping the locked sections).
    const VaultPreset& currentSound() const { return live; }
    bool isSoundEdited() const;                            // differs from the bank preset
    void editSound (const std::function<void (VaultPreset&)>& change);   // change the current sound (applied without re-striking)
    void revertSound();                                    // back to the bank preset (locked sections stay)
    // the "edited" badge: ANY change from the preset as it loads - the sound, a MAIN knob off "as designed", a sound macro
    // moved, the FX switched off, the arp / gate switched differently (sections kept while browsing excepted)
    bool hasEdits() const;
    void revertAll();                                      // all of that back: revertSound + MAIN knobs + macros + switches
    // the host's text of a MAIN knob parameter in the plugin's units (see VaultProcessor.cpp) and its inverse
    juce::String quickParamText (int q, float value01) const;
    static juce::String quickOffsetText (int q, float value01);
    static float quickParamFromText (int q, const juce::String& text);
    juce::String macroName (int index) const;              // the current sound's macro names ("" = unused knob)
    float getQuick (int q) const;                          // vault::Quick, 0..1
    void setQuick (int q, float value01);                  // notifies the host
    void resetQuick();                                     // every MAIN knob to neutral
    bool getSwitch (const char* id) const;                 // idFxOn / idArpOn / idGateOn
    void setSwitch (const char* id, bool on);
    void setLock (Section s, bool locked) { locks[(int) s] = locked; }
    bool getLock (Section s) const { return locks[(int) s].load(); }
    // MAIN page EFFECTS: the chain slot a knob drives (-1 = none: the knob uses a default slot) and its LED
    int fxSlotForQuick (int q) const;
    bool isQuickFxOn (int q) const;                        // the slot is on (a default slot counts as on while the knob is up)
    void setQuickFxOn (int q, bool on);                    // toggles the slot; switching on with no slot adds one (default settings)
    // the effective state of the audio engine (approximate, for meters / step lights)
    int arpStep() const { return engine.arpStep(); }
    float lfoOutput (int i) const { return engine.lfoOutput (i); }
    const VaultEngine& audioEngine() const { return engine; }
    // MIDI drag: the arp pattern (held keys, else the C5 major triad) for `bars` bars at the host tempo
    juce::MidiFile arpPatternMidi (double bars = 2.0) const;
    juce::File writeArpMidiFile (double bars = 2.0) const;  // a temporary .mid file for performExternalDragDropOfFiles
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
    // what the editor showed when it was closed (saved with the project): the browser's category (-1 = All Sounds) - a
    // reopened window shows the same list, scrolled to the sound that plays
    struct EditorMemory { int category = -1; };
    EditorMemory editorMemory;
    juce::AudioProcessorValueTreeState apvts;

    // the small bank used when no full bank is embedded: one 1-part preset per EVOLVE factory preset
    static std::vector<VaultPreset> buildFallbackBank();
    static int vaultCategoryOfFactory (int kkCategory);

    //==========================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;   // ends the notes (no stuck arp)
    using AudioProcessor::processBlock;
    using AudioProcessor::processBlockBypassed;

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
    void resetLiveTo (int index, bool keepLocked);      // live = bank preset (+ the locked sections of the previous live sound)
    void publishLive();                                 // compile `live` and hand it to the audio thread

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

    // v3: the current sound (message thread) and its hand-over to the audio thread (triple buffer, no locks / allocation there)
    VaultPreset live;
    int liveFor = -1;                                    // the bank index `live` belongs to
    std::array<VaultCompiledPreset, 3> editBuf;
    std::array<int, 3> editBufFor { -1, -1, -1 };
    std::atomic<int> editMid { 1 };
    int editBack = 0, editFront = 2;                     // writer / reader ends
    int editAppliedFor = -2;                             // audio thread: the preset the last edit belongs to
    std::atomic<bool> locks[3] { { false }, { false }, { false } };
    std::atomic<float>* quickParam[vault::numQuick] {};
    juce::RangedAudioParameter* quickParamObj[vault::numQuick] {};
    juce::RangedAudioParameter* macroParamObj[vault::numMacros] {};
    std::atomic<float>* fxOnParam = nullptr; std::atomic<float>* arpOnParam = nullptr; std::atomic<float>* gateOnParam = nullptr;
    std::atomic<double> hostBpm { 120.0 };
    std::array<std::atomic<float>, 128> keysHeldUi {};   // keys down (arp MIDI export)
    bool bypassedNotesOff = false;                       // audio thread

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VaultProcessor)
};
