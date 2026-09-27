#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Synth.h"
#include "Fx.h"
#include "Presets.h"
#include <map>

class KeysKillaProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    KeysKillaProcessor();
    ~KeysKillaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "KEYS KILLA"; }
    bool acceptsMidi() const override { return true; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 10.0; }

    int getNumPrograms() override { return (int) factoryPresets().size(); }
    int getCurrentProgram() override { return juce::jmax (0, currentPreset); }
    void setCurrentProgram (int index) override
    {
        // hosts may switch programs from the audio thread - never load a preset there
        if (index == currentPreset) return;   // hosts echo the current program back - nothing to do
        if (juce::MessageManager::existsAndIsCurrentThread()) loadPreset (index);
        else { pendingProgram = index; triggerAsyncUpdate(); }
    }
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ---------------- message-thread API used by the editor ----------------
    void loadPreset (int index);
    void initPatch();
    void revert();
    bool isModified() const;
    juce::String currentName() const { return presetName; }
    int  currentPresetIndex() const { return currentPreset; }
    juce::File currentUserFile() const { return userFile; }
    juce::StringArray macroNames() const { return macroLabels; }

    // user presets (JSON)
    static juce::File userPresetDir();
    juce::Array<juce::File> userPresets() const;
    bool saveUserPreset (const juce::File& f);
    bool loadUserPreset (const juce::File& f);
    bool renameUserPreset (const juce::String& newName);
    bool deleteUserPreset();
    int  importPack (const juce::File& zipOrFolder);
    bool exportPack (const juce::File& zip);

    // DICE
    enum DiceLock { lockEngine, lockFilter, lockEnv, lockMod, lockFx, lockExclusive, numLocks };
    static const char* lockName (int i);
    void rollDice (int tile);
    bool undoDice();
    void restoreDice (int historyIndex);
    juce::StringArray diceHistoryNames() const;
    std::array<bool, numLocks> diceLocks {};

    // ERA MORPH corners (0 classic, 1 melodic, 2 raw, 3 aggressive): factory preset index or -1
    void setMorphCorner (int corner, int presetIndex);
    int  morphCorner (int corner) const { return corners[(size_t) corner]; }

    // A/B, undo (parameter snapshots - cheap, no per-change bookkeeping)
    void switchAB();
    void copyAtoB();
    int  currentAB() const { return abSlot; }
    void captureUndo();   // call when the user finished an edit (mouse up)
    bool undo();
    bool redo();
    bool canUndo() const { return ! undoStack.empty(); }
    bool canRedo() const { return ! redoStack.empty(); }

    // FX order + settings
    std::array<int, kk::numFxSlots> getFxOrder() const;
    void setFxOrder (const std::array<int, kk::numFxSlots>& o);
    std::atomic<bool> eco { false };

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    std::atomic<float> meterL { 0 }, meterR { 0 };
    std::atomic<bool>  overload { false };
    std::atomic<float> guiPitch { 0 }, guiMod { 0 };   // from on-screen wheels
    std::array<std::atomic<bool>, 128> playing {};
    int uiTile = -1, uiEra = -1, uiSub = -1; bool uiExclusive = false;   // browser filter (kept while editor is closed)

private:
    void buildVoiceParams (kk::VoiceParams& vp, kk::FxParams& fp);
    void handleMidi (const juce::MidiMessage& m);
    void processMidi (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples, double beatPos, double bpm);
    void applyValues (const std::vector<std::pair<juce::String, float>>& values);
    void syncParamsToState();
    void snapshotForModified();
    void rebuildCornerBank();
    int  lockOf (const juce::String& id) const;
    float value (int i) const;
    int  snapToScale (int note) const;

    // parameter table
    std::vector<juce::RangedAudioParameter*> params;
    std::vector<std::atomic<float>*> raw;
    std::vector<bool> morphable, discrete, keepParam;
    std::map<juce::String, int> idIndex;
    bool loadingPreset = false;
    int indexOf (const juce::String& id) const;
    struct Idx;
    std::unique_ptr<Idx> ix;

    kk::SynthEngine synth;
    std::unique_ptr<kk::FxRack> fxPtr { std::make_unique<kk::FxRack>() };
    kk::VoiceParams vp;
    kk::FxParams fp;

    std::vector<float> bufL, bufR, bufG, lfoBuf, lfo2Buf;
    juce::MidiBuffer processedMidi;
    double sr = 44100;
    float lfoPhase = 0, lfo2Phase = 0, sh1 = 0, sh2 = 0, lastPh1 = 0, lastPh2 = 0;
    kk::Rng shRng;
    double freeBeat = 0;
    float midiPitch = 0, midiMod = 0, midiAT = 0;
    int64_t sampleClock = 0;
    float bypassGain = 1.0f; bool bypassed = false;
    std::atomic<int> pendingProgram { -1 };
    void handleAsyncUpdate() override { const int p = pendingProgram.exchange (-1); if (p >= 0) loadPreset (p); }
    std::atomic<bool> presetJump { false }; int dipPos = 1 << 20;   // short dip when many params jump at once

    // key lock / chord / arp
    bool lastChord = false, lastArp = false, lastKeyLock = false;
    std::array<int, 128> noteMap {};                 // input note -> locked note
    std::array<bool, 128> arpHeld {};
    std::array<int, 128> arpOrder {}; int arpOrderN = 0;
    int arpIndex = 0, arpDir = 1;
    int64_t arpLastStep = -1;
    kk::Rng arpRng;
    struct Pending { int64_t due; int note; float vel; bool on; };
    std::array<Pending, 256> pending {}; int pendingN = 0;
    void addPending (int64_t due, int note, float vel, bool on);

    // morph
    std::array<int, 4> corners { -1, -1, -1, -1 };
    std::array<std::array<std::vector<float>, 4>, 2> cornerBank;
    std::atomic<int> cornerBankIdx { 0 };
    std::atomic<bool> morphActive { false };
    float morphW[4] { 0.25f, 0.25f, 0.25f, 0.25f };

    // fx order
    std::array<std::atomic<int>, kk::numFxSlots> fxOrder;

    // presets
    int currentPreset = -1;
    juce::String presetName { "Init" };
    juce::File userFile;
    juce::StringArray macroLabels;
    std::vector<float> loadedSnapshot;
    struct DiceEntry { juce::String name; juce::ValueTree state; };
    std::vector<DiceEntry> diceHistory;
    int diceCount = 0;
    juce::ValueTree abState[2];
    std::vector<std::vector<float>> undoStack, redoStack;
    std::vector<float> lastSnap;
    std::vector<float> snapshot() const;
    void applySnapshot (const std::vector<float>& v);
    bool presetForcedArp = false;
    int abSlot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KeysKillaProcessor)
};
