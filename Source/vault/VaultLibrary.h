#pragma once
#include "VaultTypes.h"
#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

// CHROME VAULT sample library: the instrument index (Library/library.json), a background loader that decodes the
// instruments presets need (audio/*.ogg -> 16-bit RAM), an LRU cache with a memory budget, and a lock-free hand-off to
// the audio thread. Format and rules: docs/VAULT.md ("Sample library").
namespace vault
{
//==============================================================================
// index data (parsed from library.json, always in memory, small)
struct SampleZone
{
    juce::String file;          // path relative to Library/audio/
    int root = 60;              // MIDI note of the recorded pitch (after the builder's pitch check)
    float fine = 0.0f;          // cents to ADD so the zone plays in tune (builder: -measured deviation)
    int lo = 0, hi = 127;       // key range (inclusive)
    int vlo = 1, vhi = 127;     // velocity range 1..127 (inclusive)
    int rr = 0;                 // round-robin index inside its key / velocity cell (0..)
    float gainDb = 0.0f;        // extra zone gain (the builder bakes its loudness match into the audio, usually 0)
    int start = 0;              // first frame to play
    int loopStart = 0, loopEnd = 0, xfade = 0;   // frames; loopEnd > loopStart = sustain loop with a runtime crossfade of `xfade` frames before loopEnd
    int frames = 0, channels = 0;                // as stored (informative; the decoder's values are used)
    bool releaseTrigger = false;                 // true: a release sample (played at note-off)
    bool looped() const { return loopEnd > loopStart + 16; }
};

struct SampleArticulationDesc
{
    juce::String name;                 // "sustain", "staccato", "pizzicato", "vibrato", ...
    bool oneShot = false;              // true: plays to the end, note-off ignored (mallets, plucks)
    float release = 0.3f;              // default release time (s): fade after note-off
    float releaseGainDb = 0.0f;        // level of the release samples
    float releaseDecayDb = 6.0f;       // release samples get this many dB quieter per second the note was held (max -30)
    std::vector<SampleZone> zones;     // attack + release zones
};

struct InstrumentDesc
{
    juce::String id, name, family, credit;
    std::vector<SampleArticulationDesc> articulations;
    int articulationIndex (const juce::String& name) const;   // 0 for "" or unknown names
};

//==============================================================================
// decoded audio (RAM). Planar int16 with `pad` zero frames before and after (interpolation taps never leave the buffer).
struct SampleAudio
{
    static constexpr int pad = 16;
    int channels = 0, frames = 0;
    double sampleRate = 44100.0;
    std::vector<int16_t> data;         // channels x (pad + frames + pad)
    const int16_t* channel (int c) const { return data.data() + (size_t) juce::jlimit (0, channels - 1, c) * (size_t) (frames + 2 * pad) + pad; }
    size_t bytes() const { return data.size() * sizeof (int16_t); }
};

// one loaded instrument: everything the audio thread needs, immutable once published
struct SampleInstrumentData
{
    struct Zone
    {
        const SampleAudio* audio = nullptr;
        int root = 60; float fine = 0, gain = 1;
        int vlo = 1, vhi = 127, rr = 0;
        int start = 0, loopStart = 0, loopEnd = 0, xfade = 0;
        bool looped = false;
    };
    struct Articulation
    {
        bool oneShot = false;
        float release = 0.3f, releaseGain = 1.0f, releaseDecayDb = 6.0f;
        std::vector<Zone> zones, releaseZones;
        // per key: [keyStart[k], keyStart[k+1]) into keyZones (indices into zones, sorted by vlo then rr)
        std::array<uint32_t, 129> keyStart {}, relKeyStart {};
        std::vector<uint16_t> keyZones, relKeyZones;
    };
    juce::String id;
    std::vector<std::unique_ptr<SampleAudio>> audio;
    std::vector<Articulation> articulations;
    size_t bytes = 0;

    // audio thread: the zone for key / velocity (1..127) / round-robin counter, or nullptr
    const Zone* pick (int articulation, int key, int velocity, unsigned rrCounter, bool release) const;
};

//==============================================================================
// one library instrument as seen by presets and the audio thread (stable address for the library's lifetime)
class InstrumentHandle
{
public:
    explicit InstrumentHandle (InstrumentDesc d) : desc (std::move (d)) {}
    const InstrumentDesc desc;

    // audio thread: the loaded data or nullptr (silent while loading / evicted / failed). Call acquire() first and
    // release() when the engine stops using it (the data then stays valid at least until release()).
    const SampleInstrumentData* data() const { return loaded.load (std::memory_order_seq_cst); }
    void acquire() { users.fetch_add (1, std::memory_order_seq_cst); lastUse.store (++useClock(), std::memory_order_relaxed); }
    void release() { users.fetch_sub (1, std::memory_order_seq_cst); }
    void request() { wanted.store (true, std::memory_order_relaxed); }        // audio thread OK: ask the loader for it
    bool isLoaded() const { return data() != nullptr; }
    bool hasFailed() const { return failed.load(); }
    bool isLoading() const { return ! isLoaded() && ! hasFailed() && (wanted.load() || pins.load() > 0); }

private:
    friend class SampleLibrary;
    static std::atomic<uint32_t>& useClock() { static std::atomic<uint32_t> c { 0 }; return c; }
    std::atomic<const SampleInstrumentData*> loaded { nullptr };
    std::unique_ptr<SampleInstrumentData> owned;          // the loader thread owns the data
    std::atomic<int> users { 0 }, pins { 0 };
    std::atomic<bool> wanted { false }, failed { false };
    std::atomic<uint32_t> lastUse { 0 };
};

//==============================================================================
class SampleLibrary : private juce::Thread
{
public:
    static constexpr int64_t defaultBudgetBytes = (int64_t) 1536 * 1024 * 1024;   // 1.5 GB of decoded audio

    // process-wide library at the standard location (see searchLocations()), created on first use and shared by every
    // plugin instance; destroyed when the last holder lets go
    static std::shared_ptr<SampleLibrary> getShared();
    // the folders searched, in order: $CHROME_VAULT_LIBRARY, the system folder (Windows %ProgramData%\TrapWin\CHROME VAULT\Library,
    // macOS /Library/Application Support/TrapWin/CHROME VAULT/Library, Linux /usr/share/TrapWin/CHROME VAULT/Library), then the
    // user folder (Windows %APPDATA%\TrapWin\CHROME VAULT\Library, macOS ~/Library/Application Support/TrapWin/CHROME VAULT/Library,
    // Linux ~/.local/share/TrapWin/CHROME VAULT/Library). The first one holding a readable library.json wins.
    static juce::Array<juce::File> searchLocations();

    explicit SampleLibrary (const juce::File& libraryFolder);   // an explicit folder (tests / tools); may not exist
    ~SampleLibrary() override;

    bool libraryFound() const { return found; }
    juce::File libraryPath() const { return folder; }          // the folder used (or the first searched when none was found)
    juce::String errorMessage() const { return error; }
    int numInstruments() const { return (int) handles.size(); }
    InstrumentHandle* instrument (int index) const { return handles[(size_t) index].get(); }
    InstrumentHandle* find (const juce::String& id) const;     // nullptr if unknown (any thread)

    // budget for decoded audio (bytes, default 1.5 GB or $CHROME_VAULT_RAM_MB); unused instruments are evicted
    // least-recently-used first when it is exceeded. Instruments in use / pinned are never evicted.
    void setMemoryBudget (int64_t bytes);
    int64_t memoryBudget() const { return budget.load(); }
    int64_t memoryUsed() const { return used.load(); }

    // any thread: keep these loaded (pin) / allow eviction again (unpin). Pinning also requests the load.
    void pin (InstrumentHandle*);
    void unpin (InstrumentHandle*);
    // non-audio thread: decode now, in this thread (offline renders, tests). True when loaded.
    bool loadNow (InstrumentHandle*);
    // tests: wait until nothing is waiting to load (or the timeout)
    bool waitUntilIdle (int timeoutMs);

private:
    void run() override;
    bool loadInstrument (InstrumentHandle&);          // under loadLock
    void evictIfNeeded();
    bool parseIndex (const juce::File& json);

    juce::File folder;
    bool found = false;
    juce::String error;
    std::vector<std::unique_ptr<InstrumentHandle>> handles;
    std::map<juce::String, InstrumentHandle*> byId;
    std::atomic<int64_t> budget { defaultBudgetBytes }, used { 0 };
    std::mutex loadLock;
    std::atomic<bool> busy { false };
};
} // namespace vault
