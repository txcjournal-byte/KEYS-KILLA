#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Synth.h"
#include "Fx.h"
#include "Presets.h"
#include "Loops.h"
#include "Chop.h"
#include "World.h"
#include "Rolls.h"
#include "DrumBoost.h"
#include "PairLab.h"
#include "Harvest.h"
#include "VstHost.h"
#include <map>

class KeysKillaProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    explicit KeysKillaProcessor (bool withModules = true);   // false: the light offline thumbnail renderer
    ~KeysKillaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override { for (auto& m : modules) if (m) m->releaseResources(); }
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
    void rollDice (int category);
    void breedWith (int presetIndex);
    bool undoDice();
    void restoreDice (int historyIndex);
    juce::StringArray diceHistoryNames() const;
    std::array<bool, numLocks> diceLocks {};

    // ---------------- BREED LAB ----------------
    enum Gene { geneBody, geneAttack, geneTexture, geneSpace, geneMovement, geneCharacter, numGenes };
    static const char* geneName (int g);
    struct Genome
    {
        juce::String name; int cat = -1, era = 0, gen = 0, preset = -1;
        std::vector<float> v;              // normalised parameter values
        kk::LoopGenes loop;                // its melody style (BREED LOOPS)
        bool valid() const { return ! v.empty(); }
    };
    struct Child
    {
        Genome g; std::array<int, numGenes> genes {}; uint32_t seed = 0; int rating = 0;
        std::array<float, 64> wave {}; bool waveReady = false; bool hybrid = false;
    };
    struct Generation { Genome parents[2]; std::vector<Child> kids; };
    struct TreeResult { Genome g; std::array<float, 64> wave {}; bool waveReady = false; int rating = 0; };
    void setParentPreset (int slot, int presetIndex);
    void setParentCurrent (int slot);
    void setParentChild (int slot, int childIndex);
    void setParentGenome (int slot, const Genome& g) { if (g.valid()) { parents[(size_t) juce::jlimit (0, 1, slot)] = g; ++labVer; } }
    void randomParent (int slot);
    void stepParent (int slot, int dir);
    int  breed();                                       // 6 children from the two parents
    void selectChild (int i);                           // loads it as the current sound
    void setChildGene (int child, int gene, int parentSlot);
    void toggleGeneLock (int gene);
    void rateChild (int child, int stars);
    void restoreGeneration (int h);
    void previewChild (int i);                          // select + play a short note
    bool renderNextThumbnail();                         // message thread, one child per call
    void releaseThumbnailRenderer();                    // editor closed: free the offline renderer
    const Genome& parent (int s) const { return parents[(size_t) juce::jlimit (0, 1, s)]; }
    const std::vector<Child>& kids() const { return children; }
    int  selectedChild() const { return selChild; }
    bool geneLocked (int g) const { return geneLock[(size_t) g]; }
    int  geneLockSource (int g) const { return geneLockSrc[(size_t) g]; }
    const std::vector<Generation>& generations() const { return history; }
    int  labVersion() const { return labVer; }
    std::atomic<int> previewNote { -1 };
    std::atomic<int> arpCurStep { -1 };               // playing arp step (UI)
    float breedWild = 0.25f;                          // WILD rail: 0 safe ... 1 crazy
    void resetParams (const juce::StringArray& ids);  // back to the sound as it was loaded

    // ---------------- FAMILY TREE: up to 4 sounds -> BREED -> 6 new sounds or 6 melody loops ----------------
    static constexpr int numAncestors = 4;
    enum { treeSound = 0, treeLoop = 1 };
    void setAncestorPreset (int slot, int presetIndex);
    void setAncestorCurrent (int slot);
    void setAncestorGenome (int slot, const Genome& g);
    void clearAncestor (int slot);
    void randomAncestor (int slot);
    void stepAncestor (int slot, int dir);
    const Genome& ancestor (int slot) const { return ancestors[(size_t) juce::jlimit (0, numAncestors - 1, slot)]; }
    int  treeBreed();                                 // always new: 6 sounds (SOUND) or 6 sounds with new melodies (LOOP)
    void setTreeMode (int m) { treeMode = m == treeLoop ? treeLoop : treeSound; ++labVer; }
    int  getTreeMode() const { return treeMode; }
    const std::vector<TreeResult>& treeKids() const { return treeResults; }
    int  treeSelected() const { return treeSel; }
    void selectTreeResult (int i);                    // load it (LOOP mode: with its melody)
    void playTreeResult (int i);                      // SOUND: short note, LOOP: start / stop its loop
    void rateTreeResult (int i, int stars);
    void newMelody (int i);                           // LOOP: a new melody for this result, same sound
    bool loopIsTree (int i) const { return loopOn.load() && treeSel == i && loopOwner == 1; }
    // BREED LAB (main page) SOUND / LOOP: in LOOP mode a child's play button plays its melody loop
    bool mainLoopMode = false;
    void playChild (int i);
    bool loopIsChild (int i) const { return loopOn.load() && selChild == i && loopOwner == 3; }
    int  loopOwnerId() const { return loopOwner; }
    void touchLab() { ++labVer; }

    // ---------------- BREED LOOPS ----------------
    void toggleLoop();                                // play the current sound's loop (host tempo, bar synced)
    void stopLoop() { loopOn = false; }
    bool loopPlaying() const { return loopOn.load(); }
    void setLoopBars (int bars);                      // 8 or 16
    int  loopBars() const { return loopBarsN; }
    void setLoopKey (int key);                        // -1 = AUTO (the loop's own key, or KEY when Key Lock is on)
    int  loopKey() const { return loopKeyN; }
    int  effectiveLoopKey (const kk::LoopGenes& l) const;
    const kk::LoopGenes& currentLoop() const { return curLoop; }
    std::vector<kk::LoopNote> loopNotes (const Genome& g) const;
    juce::File exportLoopMidi (const Genome& g) const; // temp .mid for drag & drop into the host
    juce::File exportSoundWav (int note = 60);
    juce::AudioBuffer<float> renderSound (int note, int presetIndex);   // offline, 44.1 kHz
    juce::String pairDice (int slot);          // DRAG TO DAW: the current sound as a one-shot WAV
    Genome currentGenome() const { return genomeFromCurrent(); }
    std::atomic<float> loopBeat { -1.0f };            // playhead in beats (UI), -1 = stopped
    std::atomic<double> lastBpm { 140.0 };

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
    std::atomic<bool> panicFlag { false };   // all notes off, handled on the audio thread
    void panic() { panicFlag = true; }

    juce::AudioProcessorValueTreeState apvts;
    juce::MidiKeyboardState keyboardState;
    std::atomic<float> meterL { 0 }, meterR { 0 };
    // ---- v0.17 modules: the other KILLA plugins inside KEYS KILLA, with their own design ----
    // HALF = Voodoo Killa and EFFECTOR = Effector Killa process only the melodies (KEYS KILLA + DIGGA),
    // DIGGA = Digga Killa (sampler). Drums never go through them.
    enum Module { modHalf, modEffector, modDigga, numModules };
    juce::AudioProcessor* module (int m) const { return m >= 0 && m < numModules ? modules[(size_t) m].get() : nullptr; }
    enum PlayMode { playKeys, playDigga, play808, playSnare, playHat, playPair, playVst, playChop };
    // PAIR YOUR OWN: your sounds (WAV / Digga one-shots) -> BREED -> 6 children -> keys + loops
    bool loadPairParent (int slot, const juce::File& f);
    // CHOP / SLICE: DIGGA's sample cut into slices (pads, keys, drag out, to PAIR / bank)
    kk::ChopLab chop;
    std::atomic<int> chopPad { -1 };
    juce::AudioBuffer<float> fxIn;                               // FX INPUT (mixer insert) copy                             // UI pad -> audio thread
    bool chopFromDigga();
    bool chopToPair (int slice, int slot = -1);
    bool chopToBank (int slice);
    void clearPairParent (int slot);
    int  pairFromDigga();                                       // Digga's one-shots fill the empty slots; returns how many
    void pairBreed (bool newChildren = true);   // false: same children, only the flavour changes
    void selectPairKid (int i, bool audition);                  // the kid plays on the keys / in the loop
    void togglePairLoop (int i);                                // a new melody loop with this kid (host tempo)
    juce::File exportPairKid (int i) const;
    juce::File exportPairLoop() const;
    std::array<kk::PairPtr, kk::PairLab::maxParents> pairParents;
    std::vector<kk::PairPtr> pairKids;
    int pairSel = -1, pairFlavor = 0, pairLoopKid = -1;
    std::atomic<int> pairVer { 0 };
    // HARVEST: sounds collected from your songs / samples (bank by character)
    void harvestFile (const juce::File& f);                    // runs in the background
    int  harvestFromDigga();                                    // DIGGA's loops + one-shots; returns how many sources
    bool harvesting() const { return harvestJobs.load() > 0; }
    void bankToPair (int bankIndex, int slot = -1);             // -1 = first free slot
    void auditionBank (int bankIndex);
    std::vector<kk::HarvestItem> bank;
    // the bank on disk: Documents/KEYS KILLA/Bank/<SHELF>/*.wav (VST captures are saved right away)
    static juce::File bankFolder();
    void loadSavedBank();
    int  saveBank();                                            // saves the unsaved (harvested) sounds; returns how many
    void addToBank (kk::PairPtr s, const juce::String& origin, bool save);
    void removeFromBank (int i);                                // also deletes its WAV in the Bank folder
    void clearShelf (int cat);
    void sortBank();
    // PAIR FROM VST
    kk::VstHost vst;
    juce::StringArray vstList;                                  // installed instrument plugin files
    juce::String loadVst (const juce::String& id);
    juce::String captureVst (int note);
    juce::String captureVstProgram (int program, int note);   // GRAB SOUNDS: one of the plugin's presets into the bank                         // records the plugin into the bank; returns the sound name
    bool bankLoaded = false;
    juce::StringArray harvestedFrom;
    // DRUM BOOST (808 / SNARE-CLAP / HI-HAT): your drum WAV in, boosted WAV out
    bool loadDrum (int d, const juce::File& f);
    void clearDrum (int d);
    const kk::DrumBoost& drum (int d) const { return drums[(size_t) juce::jlimit (0, 2, d)]; }
    juce::File exportDrum (int d) const;
    void hitDrum (int d, int note = -1);                        // UI pad (any thread)
    void renderDrum (int d);                                    // message thread
    void moduleHousekeeping();                                  // message thread, ~30 Hz from the editor
    // PATTERNS per drum page: 0 = 808 line, 1 = snare / clap rolls, 2 = hi-hat rolls
    void generatePattern (int d, int style, int bars, float density);
    void setPattern (int d, std::vector<kk::RollHit> pat, int bars);
    std::vector<kk::RollHit> pattern (int d) const;
    juce::File exportPatternMidi (int d) const;
    std::atomic<int> patPlay { -1 };                            // which drum's pattern plays (-1 = none)
    std::atomic<float> patBeat { -1.0f };
    std::atomic<int> patVer { 0 };
    std::array<int, 3> patStyle { 0, 0, 0 }, patBars { 2, 2, 2 };
    std::array<float, 3> patDensity { 0.5f, 0.5f, 0.5f };
    std::atomic<bool>  overload { false };
    std::atomic<float> guiPitch { 0 }, guiMod { 0 };   // from on-screen wheels
    std::array<std::atomic<bool>, 128> playing {};
    int uiTile = -1, uiEra = -1, uiSub = -1, uiCat = -1; bool uiExclusive = false;   // browser filter (kept while editor is closed)

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
    static int snapToScaleWith (int note, int key, int mask);

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
    // modules
    std::array<std::unique_ptr<juce::AudioProcessor>, 3> modules;
    juce::AudioBuffer<float> modBuf;                            // scratch: DIGGA output
    juce::MidiBuffer modMidi, noMidi, keysForModules, vstMidi;
    std::array<float, 2> modFade { 0, 0 };                      // HALF / EFFECTOR on-off crossfades
    juce::AudioBuffer<float> modDry;
    int modBlock = 0, lastPlayMode = 0, reportedLatency = 0;
    void processModules (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& keysMidi, int n);
    void runModule (int m, juce::AudioBuffer<float>& buffer, int n, juce::MidiBuffer& midi);
    kk::WorldStage worldStage;
    kk::PairLab pairPlayer;
    juce::ThreadPool harvestPool { 1 };
    std::atomic<int> harvestJobs { 0 };
    juce::SpinLock harvestLock;
    std::vector<std::vector<kk::HarvestItem>> harvestDone;   // finished jobs, merged on the message thread
    std::array<juce::String, kk::PairLab::maxParents> pairFiles;
    uint32_t pairSeed = 1;
    std::array<kk::DrumBoost, 3> drums;
    std::array<std::atomic<int>, 3> drumPad {};
    std::array<int, 3> drumSig { -1, -1, -1 }, drumPadOff { 0, 0, 0 }, drumPadNote { -1, -1, -1 };
    std::array<double, 3> drumDirtyAt { 0, 0, 0 };
    int drumSignature (int d) const;
    kk::BoostParams boostParams (int d) const;
    std::array<std::array<float, 2>, 2> clipDc {};
    mutable juce::SpinLock rollLock;
    std::array<std::vector<kk::RollHit>, 3> patterns;
    int patRunning = -1, pat808Off = -1, pat808Note = 60; bool patHostWas = false; double patOrigin = 0;
    void renderPatterns (int n, double beatPos, double bps, bool hostPlaying);
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
    void handleAsyncUpdate() override { const int p = pendingProgram.exchange (-1); if (p >= 0) loadPreset (p); moduleHousekeeping(); }
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

    // breed lab
    std::array<Genome, 2> parents;
    std::vector<Child> children;
    int selChild = -1, labVer = 0;
    bool hybridHint = false;
    std::array<bool, numGenes> geneLock {};
    std::array<int, numGenes> geneLockSrc {};
    uint32_t breedCount = 0;
    std::vector<Generation> history;
    std::vector<int> geneOfParam;                       // per parameter, -1 = not inherited
    std::unique_ptr<KeysKillaProcessor> thumbRenderer;  // offline copy for the children's waveforms
    std::array<Genome, numAncestors> ancestors;
    std::vector<TreeResult> treeResults;
    int treeSel = -1, treeMode = treeSound, loopOwner = 0;   // loopOwner 1 = a FAMILY TREE result
    uint32_t treeCount = 0;
    Child makeChildOf (const Genome& pa, const Genome& pb, int k, uint32_t seed, const std::array<int, numGenes>* forced, bool useLocks) const;
    void renderWave (const Genome& g, std::array<float, 64>& wave);
    void setCurrentLoop (const kk::LoopGenes& l);
    // loop player (notes swapped under a spin lock, the audio thread only try-locks)
    kk::LoopGenes curLoop;
    int loopBarsN = 8, loopKeyN = -1;
    std::atomic<bool> loopOn { false }, loopDirty { false };
    juce::SpinLock loopLock;
    std::vector<kk::LoopNote> loopSeq;                  // guarded by loopLock
    double loopLenBeats = 32.0, loopOrigin = 0.0;
    bool loopRunning = false, loopHostWas = false;
    std::array<bool, 128> loopActive {};
    void renderLoop (juce::MidiBuffer& out, int n, double beatPos, double bps, bool hostPlaying);
    void rebuildLoopSeq();
    int previewOffIn = -1, previewActive = -1;
    Genome genomeFromPreset (int idx) const;
    Genome genomeFromCurrent() const;
    Child makeChild (int k, uint32_t seed, const std::array<int, numGenes>* forcedGenes) const;
    void applyGenome (const Genome& g, bool asPreset);
    void saveLab (juce::ValueTree& state) const;
    void loadLab (const juce::ValueTree& state);

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
