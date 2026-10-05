#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include "Params.h"
#include "Synth.h"
#include "Fx.h"
#include "Presets.h"
#include "Loops.h"
#include "Chop.h"
#include "MelodyRack.h"
#include "Library.h"
#include "World.h"
#include "Rolls.h"
#include "DrumBoost.h"
#include "PairLab.h"
#include "Harvest.h"
#include "VstHost.h"
#include "MelodyGen.h"
#include "MixCoach.h"
#include "SoundWorld.h"
#include "FxPro.h"
#include "Alchemy.h"
#include "Living.h"
#include <map>

class KeysKillaProcessor : public juce::AudioProcessor, private juce::AsyncUpdater
{
public:
    explicit KeysKillaProcessor (bool withModules = true);   // false: the light offline thumbnail renderer
    ~KeysKillaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "EVOLVE"; }
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
    bool exportPack (const juce::File& zip, const juce::String& packName = "MY PACK");

    // v0.34 SOUND PACKS: a .kkpack is a zip with pack.json (name, author, info), cover.png and *.kkpreset sounds
    // (later also samples/*.wav). Installed into Documents/KEYS KILLA/Packs/<pack name>/. The factory sounds are the FACTORY pack.
    struct PackSound { juce::File file; juce::String name, pack; int cat = -1; };
    struct PackInfo { juce::String name, author, info; juce::File dir, cover; int sounds = 0; };
    static juce::File packsDir();
    juce::String installPack (const juce::File& kkpack, juce::String* error = nullptr);   // returns the pack name ("" = failed)
    bool removePack (const juce::String& name);
    void rescanPacks();
    const std::vector<PackInfo>& packs() const { return packList; }
    const std::vector<PackSound>& packSounds() const { return packSoundList; }

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
    void clearParent (int slot);
    std::array<std::array<float, 64>, 2> parentWave {};        // PARENT A / B waveforms (rendered by renderNextThumbnail)
    std::array<juce::int64, 2> parentWaveSig { 0, 0 };
    bool parentsReady() const { return labSlotFilled (0) && labSlotFilled (1); }
    // v0.35: BREED LAB with your own sounds - a dropped WAV makes the lab breed audio (PAIR engine); the keys play the kids
    std::atomic<bool> labAudio { false };
    bool labAudioMode() const { return labAudio.load(); }
    std::array<bool, 2> labWav { false, false };
    std::array<juce::String, 2> labWavFile;
    std::array<kk::PairPtr, 2> labAudioParents;
    bool labDropFile (int slot, const juce::File& f);          // your sound into PARENT A / B
    kk::PairPtr labParentAudio (int slot);                     // the parent as audio (bank sounds are rendered)
    juce::String labParentName (int slot) const;
    bool labSlotFilled (int slot) const { return labWav[(size_t) juce::jlimit (0, 1, slot)] || parents[(size_t) juce::jlimit (0, 1, slot)].valid(); }
    void labBreedAudio();                                      // 6 kids from the two parents as audio
    void auditionParent (int slot);                            // click = hear the parent (and play it on the keys)
    void auditionAncestor (int slot);

    // ---------------- v0.36 EVOLVE: one seed, a living tree of sounds ----------------
    // The seed (a bank sound, the current sound or any WAV) sits in the middle; 6 children grow around it. Click one: it
    // becomes the middle and the next generation grows. SAFE <-> WILD decides how far they may wander.
    struct FxGenome { std::array<bool, kk::numRackSlots> on {}; std::array<float, kk::numRackValues> v {}; juce::String name; };
    struct EvoNode
    {
        Genome g;                    // synth sound (bank seeds); g.loop = its melody
        kk::PairPtr audio;           // audio sound (WAV seeds and their family)
        FxGenome fx;                 // v0.38 IDEA: its effect chain (sound + melody + FX evolve together)
        bool picked = false;         // you chose it (the MAP shows your way)
        juce::String file;           // an audio node kept as a WAV (POCKET / WORLD files)
        int parent = -1, gen = 0;
        std::vector<int> kids;
        std::array<float, 64> wave {}; bool waveReady = false;
        juce::String name;
        bool isAudio() const { return audio != nullptr; }
    };
    // v0.38 IDEA MODE: a node is a whole idea - its sound, its melody and its effects; hover = hear the idea in time
    bool evoIdea = true;
    void evoPick (int node);                                     // you chose it: it grows, and EVOLVE learns your taste
    void evoNotMyTaste (int node);                               // fewer like this
    void evoApplyIdea (int node);                                // its effects onto the FX rack (IDEA MODE)
    juce::File evoExportIdea (int node);                         // the whole idea as a loop WAV (bars at the project tempo)
    juce::AudioBuffer<float> renderIdea (int node, double rate);
    // v0.39 LAYERS: what the next children change - ALL, only the SOUND, only the MELODY, only the FX, only the BEAT
    enum { layerAll, layerSound, layerMelody, layerFx };   // layerMelody is not used any more (v0.40: melodies have their own page)
    int evoLayer = layerAll;
    // POCKET: keep ideas from any tree; drop one onto a bubble = a cross between two trees
    std::vector<EvoNode> pocket;
    void evoPocketAdd (int node);
    void evoPocketRemove (int i);
    void evoSeedNode (const EvoNode& n);                        // a pocket idea starts a new tree
    void evoAuditionNode (const EvoNode& n, bool preview = true);
    void evoCross (const EvoNode& a, int target);                // a hybrid of a and the target node: it becomes the middle
    // ALIVE: the middle idea slowly changes by itself while it plays; CATCH = this moment becomes the middle
    bool evoAliveStep (float amount);
    void evoCatch();
    Genome evoLive; bool evoLiveValid = false; int aliveCount = 0;
    // the tree as a file (a WORLD) - and inside the project
    juce::ValueTree evoToTree (bool withAudioFiles);
    void evoFromTree (const juce::ValueTree& et);
    bool evoSaveWorld (const juce::File& f);
    bool evoLoadWorld (const juce::File& f);
    // MY TASTE: what you pick / save / drag is learned (kept in Documents/KEYS KILLA/taste.txt, it grows with you)
    static constexpr int tasteDims = 22;                         // 8 sound + 11 effects + 3 melody
    struct Taste { std::array<float, tasteDims> like {}, dislike {}; float nLike = 0, nDislike = 0; };
    Taste taste; bool tasteLoaded = false;
    float evoTasteAmt = 0.5f;                                    // 0 = ignore it ... 1 = only what you like
    std::array<float, tasteDims> evoDescribe (const EvoNode& n) const;
    float tasteScore (const std::array<float, tasteDims>& d) const;
    void tasteLearn (const EvoNode& n, float weight);           // weight > 0 like, < 0 dislike
    void tasteLoad(); void tasteSave() const;
    int tastePicks() const { return (int) std::round (taste.nLike); }
    std::vector<EvoNode> evo;
    int evoCenter = -1;
    float evoWild = 0.35f;
    std::atomic<int> evoVer { 0 };
    juce::String evoSeedFile;                                  // a WAV seed (kept with the project)
    bool evoActive = false;                                     // the EVOLVE page is open (the keys play its sound)
    void applyGenomePublic (const Genome& g) { applyGenome (g, true); }
    void evoReset();
    void evoSeedPreset (int idx);
    void evoSeedCurrent();
    bool evoSeedFromFile (const juce::File& f);
    void evoSeedRandom();
    void evoSeedGenome (const Genome& g);
    // v0.42 SOUND WORLD: every dot of the map is a sound of its region, two regions connected = a hybrid
    Genome worldSound (int dot);
    Genome worldConnect (int regionA, int regionB, uint32_t seed);
    void worldPlay (const Genome& g, bool preview);              // on the keys (and heard)
    Genome worldCurrent; int worldRegion = -1, worldRegionB = -1, worldDot = -1;                        // v0.41: a synth genome (a MATCH strand) starts the tree
    // v0.43 ALCHEMY: a sound from EXCITER x BODY x MATTER (glass..mud) x SIZE - the bank is only hidden DNA
    Genome alchemy (int exciter, int body, float matter, float size, uint32_t seed);
    void alcUse (const Genome& g, bool preview);                 // on the keys (and heard)
    std::atomic<int> alcExc { 0 }, alcBody { 3 }; std::atomic<float> alcMatter { 0.3f }, alcSize { 0.5f }; uint32_t alcSeed = 1;
    // v0.41 MATCH: drop a WAV - four strands of synth sounds grow toward it in the background (any strand can be planted at any time)
    struct MatchStrand { Genome g; float match = 0; std::array<float, 64> wave {}; int gen = 0; };
    bool evoMatchStart (const juce::File& f);
    bool evoMatchStartBuffer (const juce::AudioBuffer<float>& audio, double rate, const juce::String& name);
    bool evoMatchFromKeys();                                     // v0.42: the sound on the keys right now (a preset, a sample, an EVOLVE sound)
    void evoMatchStop();
    std::vector<MatchStrand> matchStrands() const;               // a copy (the search writes them)
    std::atomic<bool> matchRunning { false };
    std::atomic<float> matchProgress { 0 };
    std::atomic<int> matchVer { 0 };
    juce::String matchTargetName; std::array<float, 64> matchTargetWave {};
    void evoSeedSound (kk::PairPtr s);
    void evoNewMelody (int node);                               // another melody for that sound                          // any sound (an edited sample ...) becomes the seed
    void evoGrow (int node, bool reroll);                       // 6 new children of this node
    void evoFocus (int node);                                   // the node becomes the middle (its kids grow), the keys play it
    void evoAudition (int node, bool preview = true);           // hear it - the keys play it, the middle stays
    void evoMorph (int a, int b, float t);                      // live blend between two bank-family sounds
    kk::PairPtr evoAsSound (int node);
    juce::File evoExportWav (int node);
    juce::File evoExportMidi (int node);
    std::vector<int> evoPath() const;                           // seed ... middle
    juce::String evoName (const EvoNode& parent, int k, uint32_t seed) const;
    kk::PairPtr childAsSound (int i);                          // a bank child rendered as a sound (save / drag)
    juce::File exportChildWav (int i);
    juce::File exportGenomeWav (const Genome& g);                 // any bank / bred sound as a WAV (drag into FL)
    void genomeParentSet (int slot);
    juce::AudioBuffer<float> renderGenomeAudio (const Genome& g, double rate, double seconds);
    void setParentChild (int slot, int childIndex);
    void setParentGenome (int slot, const Genome& g) { if (g.valid()) { parents[(size_t) juce::jlimit (0, 1, slot)] = g; ++labVer; genomeParentSet (slot); } }
    void randomParent (int slot);
    void stepParent (int slot, int dir);
    int  breed();                                       // 6 children from the two parents
    void selectChild (int i);                           // loads it as the current sound
    void setChildGene (int child, int gene, int parentSlot);   // audio mode: the child is spliced from A and B
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

    // ---------------- v0.40 MELODY EVOLVE: melodies have their own page ----------------
    // SURPRISE ME: 8 melodies in your key / scale / bars.  FROM MY MELODY: your melody from FL (LISTEN, or a dropped .mid)
    // -> its key is found -> 8 variations.  Click one = it becomes the parent, 8 children grow.  Drag = MIDI into FL.
    std::vector<kk::mel::Melody> mels;
    std::vector<int> melShown;                 // the 8 on screen
    int melParent = -1, melPlaying = -1;
    kk::mel::Melody melMine; bool melHasMine = false, melFromMine = false;
    int melKey = 0, melScale = kk::mel::scMinor, melBars = 8;
    kk::mel::Style melStyle;
    int melGenre = kk::mel::gTrap;             // v0.41: the style the melodies are made in (-1 = FREE)
    float melBpm = 140.0f;                     // its tempo (written into the .mid; the rhythm follows it)
    int melLayers = 0;                         // 0 = MELODY, 1 = MELODY + CHORDS, 2 = CHORDS
    void melSetGenre (int g);                  // also picks the genre's scale and tempo
    // v0.41 MIX LAB: SHAPE EQ + PUNCH COMP + TIME MACHINE on the output, the COACH reads its meters
    kk::MixLabState mixLab;
    // v0.42 EVOLVE FX PRO modules
    kk::pro::ReelState reel; kk::pro::DialState dial; kk::pro::WarpState warp; kk::pro::BossState boss;
    kk::pro::LiquidState liquid; kk::pro::IntentState intent; kk::pro::ErosionState erosion;   // v0.43 organic modules
    juce::ValueTree proToTree() const;
    void proFromTree (const juce::ValueTree& t);
    int coachGenre = kk::mel::gTrap;
    juce::ValueTree mixToTree() const;
    void mixFromTree (const juce::ValueTree& t);
    // v0.42 EVOLVE FX CHAINS: MIX LAB + FX RACK + STEP FX together, ready-made or your own
    static const juce::StringArray& chainNames();
    static const juce::StringArray& chainHints();
    void chainApply (int i);
    void chainReset();                                           // everything off / flat
    void chainEvolve (float wild);                               // a mutation of what you have now
    juce::ValueTree chainToTree (const juce::String& name) const;
    void chainFromTree (const juce::ValueTree& t);
    static juce::File chainFolder();
    juce::File chainSave (const juce::String& name) const;
    bool chainLoad (const juce::File& f);
    std::atomic<int> melVer { 0 };
    void melGenerate();                        // 8 new (SURPRISE ME or from your melody)
    void melEvolve (int idx, bool reroll);     // it becomes the parent: 8 children
    void melBack();
    void melPlay (int idx);                    // -1 = stop.  The melody plays the sound on the keys, in time
    void melPlayMine();
    juce::File melExport (int idx);            // a .mid for FL (the name says key, scale, tempo)
    juce::File melSave (int idx);              // into Documents/KEYS KILLA/Melodies
    juce::File melExportWav (int idx);         // v0.42: the melody played by the sample on the keys (SAMPLER MELODY), as a WAV
    void melListen (bool on);                  // LISTEN: catch the melody FL plays into this plugin
    bool melListening() const { return melListenOn.load(); }
    int  melHeardNotes() const { return melHeard.load(); }
    float melHeardAudio() const { return audRecPeak.load(); }   // v0.42: audio coming in while LISTEN is on (EVOLVE as an effect)
    juce::String melListenSource;                               // "MIDI" or "AUDIO": what the last LISTEN used
    bool melLoadMidiFile (const juce::File& f);
    bool melLoadAudioFile (const juce::File& f);   // v0.41 AUDIO -> MIDI: a WAV (vocal, sample, melody) becomes your melody
    void playCustomLoop (const std::vector<kk::LoopNote>& notes, double lenBeats);

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
    void resumeLoop() { loopOn = true; ++labVer; ++pairVer; }   // SPACE again: the same melody plays on
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
    Genome genomeFromCurrentPublic() const { return genomeFromCurrent(); }
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
    // what the keys play (playDigga is kept only so old projects load; it now plays the KEYS KILLA sound)
    enum PlayMode { playKeys, playDigga, play808, playSnare, playHat, playPair, playVst, playChop, playKick, playOpenHat, playPerc, playDrumFx };
    static int modeOfDrum (int d) { return d < 3 ? play808 + d : playKick + (d - 3); }
    static int drumOfMode (int pm) { return pm >= play808 && pm <= playHat ? pm - play808 : pm >= playKick && pm <= playDrumFx ? 3 + pm - playKick : -1; }
    // PAIR YOUR OWN: your sounds (WAV / Digga one-shots) -> BREED -> 6 children -> keys + loops
    bool loadPairParent (int slot, const juce::File& f);
    // CHOP / SLICE: DIGGA's sample cut into slices (pads, keys, drag out, to PAIR / bank)
    kk::ChopLab chop;
    std::atomic<int> chopPad { -1 };
    juce::AudioBuffer<float> fxIn, extBuf;
    juce::MidiBuffer vstNoMidi;
    kk::WorldStage worldExt;                                     // SOUND WORLD for PAIR / VST / CHOP / input                               // FX INPUT (mixer insert) copy                             // UI pad -> audio thread
    bool chopLoadFile (const juce::File& f);                    // SAMPLER: load your sample (WAV / AIFF / FLAC / MP3)
    void chopClear();                                           // CLEAR: the sampler is empty
    bool chopMutate (int start, int end, bool kill);            // MUTATE / KILL the selection (end <= start: everything)
    bool chopUndoMutate();
    bool chopCanUndo() const { return ! chopUndo.empty(); }
    juce::AudioBuffer<float> chopRegion (int start, int end, bool loop) const;
    juce::File exportChopRegion (int start, int end, bool loop) const;   // the selection as a WAV (drag into FL)
    bool chopRegionToParent (int start, int end, int slot);     // the selection becomes a parent in BREED LAB
    std::vector<std::shared_ptr<const juce::AudioBuffer<float>>> chopUndo;
    juce::String chopFile;
    kk::MelodyRack rack;                                        // FX RACK (message thread writes, audio reads)
    bool chopToPair (int slice, int slot = -1);
    bool chopToBank (int slice);
    void clearPairParent (int slot);
    void pairBreed (bool newChildren = true);   // false: same children, only the flavour changes
    void selectPairKid (int i, bool audition);                  // the kid plays on the keys / in the loop
    void togglePairLoop (int i);                                // a new melody loop with this kid (host tempo)
    juce::File exportPairKid (int i);
    juce::File exportPairLoop() const;
    std::array<kk::PairPtr, kk::PairLab::maxParents> pairParents;
    std::vector<kk::PairPtr> pairKids;
    std::vector<std::array<int, numGenes>> pairKidGenes;         // v0.37: GENES for your own sounds (A / B per gene)
    int geneOfSelected (int gene) const;                         // the selected child's gene source (synth or audio), -1 = none

    // ---------------- v0.37 THE SOUND ON THE KEYS ----------------
    // One "active sound" for the keys / FL's piano roll: the last thing you picked (a bank child, your sound, the SAMPLER).
    // Pages never take it away - EDIT / FX / MY SOUNDS work on whatever is active.
    void setPlayMode (int mode);
    void useSample (kk::PairPtr s, bool audition);               // your sound / an audio child becomes the active sound
    kk::PairPtr activeSample() const { return pairPlayer.sound(); }
    bool sampleActive() const { return (int) apvts.getRawParameterValue (ID::playMode)->load() == playPair && pairPlayer.sound() != nullptr; }
    bool chopActive() const { return (int) apvts.getRawParameterValue (ID::playMode)->load() == playChop; }
    // SAMPLE EDIT: what EDIT does when the active sound is a sample (your sound, an audio child, the SAMPLER)
    enum SampleEditId { seTune, seFine, seStart, seAttack, seRelease, seReverse, seTone, seLowCut, seDrive, seCrush, seChorus, seSpace, seEcho, seWidth, seGain, numSampleEdit };
    static const char* sampleEditName (int i);
    static float sampleEditDefault (int i);
    std::array<std::atomic<float>, numSampleEdit> sampleEdit;
    void resetSampleEdit();
    void sampleMacrosNeutral();                                  // the 8 big knobs back to neutral (they now colour the sample)
    juce::AudioBuffer<float> renderEditedSample (kk::PairPtr s);  // the sample with SAMPLE EDIT + the knobs, offline (drag / save)
    kk::PairPtr editedSample();                                  // the active sample as edited (for SAVE / DRAG)
    juce::File exportEditedSample();
    bool sampleEdited() const;                                   // v0.42: EDIT changed the sound on the keys
    kk::PairPtr withEdits (kk::PairPtr s);                       // s as you hear it: with the EDIT changes when s is the sound on the keys

    // ---------------- v0.37 FX EVOLVE (SURPRISE FX) + STEP FX ----------------
    FxGenome fxCurrent() const;
    void fxApply (const FxGenome& g);
    FxGenome fxMutate (const FxGenome& g, float wild, uint32_t seed) const;
    FxGenome fxSurprise (uint32_t seed) const;
    // STEP FX: a 16-step grid of effects that play in time on the melody bus (stutter, reverse, tape stop, filter ...)
    enum StepFx { sfStutter, sfReverse, sfTape, sfFilter, sfGate, sfEcho, sfPitchUp, sfCrush, sfRoll, sfPitchDown, sfPan, sfRiser, numStepFx };   // v0.41: + ROLL, OCTAVE DOWN, PAN, RISER
    static const char* stepFxName (int i);
    static constexpr int numSteps = 16;
    std::array<std::array<std::atomic<bool>, numSteps>, numStepFx> stepGrid;
    std::atomic<bool> stepOn { false };
    std::atomic<int> stepNow { -1 };                             // the playing step (UI)
    std::atomic<int> stepRate { 1 };                             // 0 = 1/8, 1 = 1/16, 2 = 1/32
    std::atomic<float> stepMix { 1.0f };
    void stepClear();
    void stepRandom (uint32_t seed, float density);
    void stepPreset (int which);
    juce::String stepToString() const;
    void stepFromString (const juce::String& s);
    // SAMPLER FLIPS (EVOLVE for beats): a pattern of chops on a 1/16 grid, played in time; it grows like EVOLVE
    struct FlipStep { int slice = -1; int len = 1; float vel = 0.9f; bool rev = false; int semi = 0; };
    struct Flip { std::vector<FlipStep> steps; juce::String name; int parent = -1; std::vector<int> kids; int gen = 0; };
    std::vector<Flip> flips;
    int flipCenter = -1, flipPlaying = -1;
    std::atomic<int> flipVer { 0 }, flipStepNow { -1 };
    std::atomic<bool> flipOn { false };
    void flipSeed();                                             // a first flip from the slices
    void flipGrow (int node, bool reroll, float wild);
    void flipPlay (int node);                                    // -1 = stop
    juce::File flipMidiFile (int node) const;
    juce::File flipWavFile (int node);                           // the flip rendered as audio (drag into FL)
    juce::AudioBuffer<float> renderFlip (int node, double bpm);
    std::atomic<bool> chopChoke { true };                        // MONO pads: a new pad stops the one before
    int pairSel = -1, pairFlavor = 0, pairLoopKid = -1;
    std::atomic<int> pairVer { 0 };
    // HARVEST: sounds collected from your songs / samples (bank by character)
    void harvestFile (const juce::File& f);                    // runs in the background
    int  harvestFromChop();                                     // the SAMPLER's sample -> sounds in the bank
    bool harvesting() const { return harvestJobs.load() > 0; }
    void bankToPair (int bankIndex, int slot = -1);             // -1 = first free slot
    void auditionBank (int bankIndex);
    std::vector<kk::HarvestItem> bank;
    // the bank on disk: Documents/KEYS KILLA/Bank/<SHELF>/*.wav (VST captures are saved right away)
    // MY SOUNDS: your own folders (apart from the factory sounds and the HARVEST bank)
    juce::String lastFolder { kk::Library::defaultFolder() }, lastKit { kk::Kits::defaultKit() };
    juce::File saveDrumToKit (int d, const juce::String& kit);   // the boosted drum into a drum kit folder
    juce::File saveToFolder (kk::PairPtr s, const juce::String& folder);
    juce::File saveToSoundKit (kk::PairPtr s, const juce::String& kit, const juce::String& category = {});   // SOUND KITS
    juce::String lastSoundKit { kk::SoundKits::defaultKit() };
    void auditionFile (const juce::File& f);                   // hear a sound file on the keys (PAIR player)
    static juce::File bankFolder();
    void loadSavedBank();
    int  saveBank();                                            // saves the unsaved (harvested) sounds; returns how many
    void addToBank (kk::PairPtr s, const juce::String& origin, bool save, int shelf = -1);
    void removeFromBank (int i);                                // also deletes its WAV in the Bank folder
    void clearShelf (int cat);
    void moveInBank (int i, int cat);
    void sortBank();
    // PAIR FROM VST
    kk::VstHost vst, vstB;                                       // PAIR FROM VST: plugin A (left) and B (right)
    kk::VstHost& host (int side) { return side == 1 ? vstB : vst; }
    std::array<std::vector<kk::VstHost::Sound>, 2> vstSounds;     // their sounds, read without their windows
    std::array<int, 2> vstSel { -1, -1 }, vstTakes { 0, 0 };
    void unloadVstSide (int side);                              // REMOVE the plugin of side A / B
    std::atomic<int> vstKeys { 0 };
    int pairUse = 4;                                            // BREED uses the first N sounds (PAIR FROM VST: A + B = 2)                             // the keys play A (0) or B (1)
    juce::String loadVstSide (int side, const juce::String& id);
    bool pickVstSound (int side, int index, int note = 60);     // the sound -> captured -> SOUND A / B (and heard)
    bool vstSideToBank (int side, int shelf = -1);
    juce::StringArray vstList;                                  // installed instrument plugin files
    juce::String loadVst (const juce::String& id);
    juce::String captureVst (int note, int shelf = -1);         // shelf -1 = AUTO (name, then the ears)
    juce::String captureVstProgram (int program, int note);   // GRAB SOUNDS: one of the plugin's presets into the bank                         // records the plugin into the bank; returns the sound name
    bool bankLoaded = false;
    juce::StringArray harvestedFrom;
    // DRUM BOOST (808 / SNARE-CLAP / HI-HAT): your drum WAV in, boosted WAV out
    bool loadDrum (int d, const juce::File& f);
    void clearDrum (int d);
    const kk::DrumBoost& drum (int d) const { return drums[(size_t) juce::jlimit (0, kk::numDrumSlots - 1, d)]; }
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
    std::array<int, kk::numDrumSlots> patStyle {}, patBars { 2, 2, 2, 2, 2, 2, 2 };
    std::array<float, kk::numDrumSlots> patDensity { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };
    std::atomic<bool> kitPlay { false };                        // DRUM KIT: every drum with a sample plays its pattern together
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
    std::unique_ptr<kk::FxRack> rackFx { std::make_unique<kk::FxRack>() };   // FX RACK on the melody bus
    std::vector<float> rackL, rackR, rackG;
    int rackTail = 0;      // samples the rack keeps running after the last effect went off (echo / reverb tails)
    double gatePhase = 0;
    float gateEnv = 1.0f;
    void processRack (juce::AudioBuffer<float>& buffer, int n, double beatPos, double bps);
    // modules
    juce::MidiBuffer vstMidi;
    int modBlock = 0, lastPlayMode = 0, reportedLatency = 0;
    kk::WorldStage worldStage;
    kk::PairLab pairPlayer;
    std::unique_ptr<kk::FxRack> extFx { std::make_unique<kk::FxRack>() };   // v0.37: the knobs / SAMPLE EDIT on your sounds
    std::vector<float> extL, extR, extG;
    int extTail = 0;
    kk::SvfCoef extLpC, extHpC; kk::SvfState extLp[2], extHp[2]; float extLpHz = -1, extHpHz = -1;
    kk::FxParams sampleFxParams (bool& any) const;
    void processSampleFx (float* L, float* R, int n, double beatPos, double bps);
    // STEP FX state (audio thread)
    std::vector<float> stepBufL, stepBufR, echoL, echoR; int stepW = 0, echoW = 0, echoTail = 0; float stepEnv[numStepFx] {}; kk::SvfState stepLp[2], stepHp[2]; float stepTapePos = 0, stepRevPos = 0;
    int stepLast = -1; double stepRevStart = 0; float stepHold[2] {};
    void processStepFx (juce::AudioBuffer<float>& buffer, int n, double beatPos, double bps);
    kk::MixLabDsp mixDsp;
    kk::pro::ReelDsp reelDsp; kk::pro::DialDsp dialDsp; kk::pro::WarpDsp warpDsp; kk::pro::BossDsp bossDsp;
    kk::pro::LiquidDsp liquidDsp; kk::pro::IntentDsp intentDsp; kk::pro::ErosionDsp erosionDsp;
    juce::AudioBuffer<float> scIn;                               // LIQUID: the sidechain (EVOLVE FX PRO), copied before the buffer is cleared
    // MATCH (background search)
    juce::ThreadPool matchPool { 1 };
    std::atomic<bool> matchStop { false };
    mutable juce::SpinLock matchLock;
    std::vector<MatchStrand> matchStrandsData;
    static juce::AudioBuffer<float> renderWith (KeysKillaProcessor& r, const Genome& g, double rate, double seconds, int note);
    // flip player
    juce::SpinLock flipLock; std::vector<FlipStep> flipSeq; double flipOrigin = 0; int flipLastStep = -1;
    juce::File sessionDir() const;
    // MELODY: a custom loop (the loop player plays these notes instead of a generated loop) + LISTEN capture
    bool customLoop = false; std::vector<kk::LoopNote> customSeq; double customLen = 32.0;
    struct RecEv { double beat; int note; bool on; float vel; };
    juce::SpinLock recLock; std::vector<RecEv> rec; std::atomic<bool> melListenOn { false }; std::atomic<int> melHeard { 0 };
    std::vector<float> audRec; std::atomic<int> audRecN { 0 }; std::atomic<double> audRecBeat { 0.0 }; std::atomic<float> audRecPeak { 0 };   // v0.42 LISTEN AUDIO
    juce::String activeSampleFile;                               // the active sample, kept with the project
    const void* activeSampleSaved = nullptr;
    juce::ThreadPool harvestPool { 1 };
    std::atomic<int> harvestJobs { 0 };
    juce::SpinLock harvestLock;
    std::vector<std::vector<kk::HarvestItem>> harvestDone;   // finished jobs, merged on the message thread
    std::array<juce::String, kk::PairLab::maxParents> pairFiles;
    uint32_t pairSeed = 1;
    std::array<kk::DrumBoost, kk::numDrumSlots> drums;
    std::array<std::atomic<int>, kk::numDrumSlots> drumPad {};
    std::array<int, kk::numDrumSlots> drumSig { -1, -1, -1, -1, -1, -1, -1 }, drumPadOff {}, drumPadNote { -1, -1, -1, -1, -1, -1, -1 };
    std::array<double, kk::numDrumSlots> drumDirtyAt {};
    std::array<int, kk::numDrumSlots> drumSeenSig { -1, -1, -1, -1, -1, -1, -1 };
    std::array<std::atomic<bool>, kk::numDrumSlots> drumBusy {};          // a background boost render is running
    juce::ThreadPool drumPool { 1 };
    int drumSignature (int d) const;
    kk::BoostParams boostParams (int d) const;
    std::array<std::array<float, 2>, 2> clipDc {};
    mutable juce::SpinLock rollLock;
    std::array<std::vector<kk::RollHit>, kk::numDrumSlots> patterns;
    int patMaskWas = 0, pat808Off = -1, pat808Note = 60; bool patHostWas = false; double patOrigin = 0;
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
    std::vector<PackInfo> packList;
    std::vector<PackSound> packSoundList;
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
