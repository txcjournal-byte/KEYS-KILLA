#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "engine/MelodyGenerator.h"
#include "engine/MidiWriter.h"
#include "engine/PreviewEngine.h"

#include <atomic>
#include <memory>
#include <mutex>

// User settings (message thread). Saved in the host project.
struct LuckySettings
{
    int root = 9;            // key root chosen by the user (A)
    bool minor = true;
    bool lucky = false;      // LUCKY: random key on every GENERATE
    int bars = 8;            // 8 | 16
    bool sync = true;        // follow the host tempo
    double manualBpm = 140.0;
    int sound = 0;           // 0 MORSE (sine) | 1 8-BIT (square) | 2 TV TEST (triangle)
    uint32_t seed = 1;
    ln::Key resolvedKey { 9, true };  // the key of the current set (differs from root/minor in LUCKY mode)
    std::array<uint32_t, ln::kNumMelodies> rolls {};
};

class LuckyNotesProcessor : public juce::AudioProcessor
{
public:
    LuckyNotesProcessor();
    ~LuckyNotesProcessor() override = default;

    // --- AudioProcessor ---
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlock (juce::AudioBuffer<double>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Lucky Notes"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return true; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // --- Lucky Notes (message thread) ---
    LuckySettings getSettings() const;
    void setKey (int root, bool minor);     // regenerates with the same seed
    void setLucky (bool lucky);
    void setBars (int bars);                // regenerates with the same seed
    void setSync (bool sync);
    void setManualBpm (double bpm);
    void setSound (int sound);
    void generate();                        // new seed (and new key in LUCKY mode), rolls reset
    void reroll (int row);

    std::shared_ptr<const ln::MelodySet> getSet() const;
    double getEffectiveBpm() const;         // host tempo when synced and known, else the manual tempo
    bool hasHostTempo() const { return hostBpm.load() > 0.0; }
    double getHostBpm() const { return hostBpm.load(); }
    bool isHostPlaying() const { return hostPlaying.load(); }

    void startPreview (int row);
    void stopPreview();
    int getPreviewRow() const { return previewRow.load(); }
    double getPreviewBeat() const { return previewBeat.load(); }   // position inside the loop (UI playhead)

    // file export
    juce::File writeMidiToFolder (int row, const juce::File& folder, bool chords = false) const;
    juce::String midiFileName (int row, bool chords = false) const;

    // bumps whenever the set or settings change (the editor polls it)
    int getStateVersion() const { return stateVersion.load(); }

    double getUiScale() const { return uiScale.load(); }
    void setUiScale (double s) { uiScale.store (s); }

    static constexpr double kMinBpm = 60.0, kMaxBpm = 200.0;

private:
    void rebuild();   // settings -> set (message thread, or setStateInformation's thread)
    void pushPattern(); // current preview row -> audio thread
    template <typename Sample> void render (juce::AudioBuffer<Sample>&, juce::MidiBuffer&);

    mutable std::mutex lock;
    LuckySettings settings;
    std::shared_ptr<const ln::MelodySet> set;

    // audio thread hand-over (try-lock: the audio thread never waits)
    struct Pattern
    {
        std::array<ln::Note, ln::PreviewSequencer::kMaxNotes> notes;
        int count = 0;
        double length = 32.0;
        int version = 0;
    };
    juce::SpinLock patternLock;
    Pattern pending;
    int audioPatternVersion = -1;
    Pattern audioCopy;

    ln::PreviewEngine engine;
    std::vector<float> mono;
    ln::NoteEvent events[256];
    double freeBeat = 0.0;
    bool wasHostPlaying = false;
    int audioRow = -1;

    std::atomic<int> previewRow { -1 };
    std::atomic<int> previewRequest { 0 };     // increments on every start (restart the free-running position)
    int audioRequestSeen = 0;
    std::atomic<double> previewBeat { 0.0 };
    std::atomic<double> hostBpm { 0.0 };
    std::atomic<bool> hostPlaying { false };
    std::atomic<double> manualBpmAtomic { 140.0 };
    std::atomic<bool> syncAtomic { true };
    std::atomic<int> soundAtomic { 0 };
    std::atomic<int> stateVersion { 0 };
    std::atomic<double> uiScale { 1.0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuckyNotesProcessor)
};
