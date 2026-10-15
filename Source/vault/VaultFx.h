#pragma once
#include "VaultTypes.h"
#include "../Fx.h"
#include <memory>

// CHROME VAULT v3 FX chain: independent slot processors around the EVOLVE FX stages (Source/Fx.h, Source/Studio.h),
// a pool of 2 processors per type (a chain holds at most 2 slots of a type), on / off with a 10 ms crossfade, and the
// output stage of the v2 master rack (mono lows, DC block, output gain, the "finish" glue / air / widening + its peak
// limiter). Allocation-free after prepare(). See docs/VAULT.md "v3".
namespace vault
{
struct FxContext
{
    double bpm = 120, beatPos = 0;   // tempo, beat position at the block start
    const float* ghostSend = nullptr;   // the synth voices' ghost send (Ghost slots)
};

// one processor of one type; p = the slot's effective parameters (real values, vault::fxTypeInfo order)
class FxProcessor
{
public:
    virtual ~FxProcessor() = default;
    virtual void prepare (double sampleRate, int maxBlock) = 0;
    virtual void clear() = 0;
    virtual void process (float* L, float* R, int n, const float* p, const FxContext& ctx) = 0;
    // true while the processor still makes sound with an amount of 0 (tails) - the chain keeps calling it
    virtual bool alwaysRun() const { return false; }
    static std::unique_ptr<FxProcessor> create (FxType t);
};

class FxChain
{
public:
    static constexpr int maxEntries = maxFxSlots + 6;   // the preset's slots + the default slots of the MAIN page knobs
    struct Entry
    {
        FxType type = FxType::reverb;
        int slot = -1;                       // the preset's chain position, -1 = a default slot (MAIN knob / classic macro)
        bool on = true;                      // the slot's switch (the global FX switch is applied on top)
        float p[maxFxParams] {};             // effective parameters, set every block by the engine
    };
    struct Output
    {
        float outGain = 1, finish = 0, width = 0.5f;
        bool monoLows = false;
    };

    void prepare (double sampleRate, int maxBlock);
    void clear();                            // every processor's state, the output stage, the crossfades
    // the effective chain (message / audio thread, allocation-free). Processors that were not in use are cleared first.
    void setLayout (const Entry* entries, int count);
    int size() const { return numEntries; }
    Entry& entry (int i) { return entries[(size_t) i]; }
    const Entry& entry (int i) const { return entries[(size_t) i]; }
    // L / R in place; allOn = the global FX switch
    void process (float* L, float* R, int n, const FxContext& ctx, bool allOn, const Output& out);
    float onGain (int i) const { return gains[(size_t) i]; }

private:
    FxProcessor* procFor (int entryIndex) const;
    double sr = 44100;
    int maxBlock = 128;
    std::array<std::unique_ptr<FxProcessor>, (size_t) numFxTypes * maxSameFxType> pool;
    std::array<Entry, maxEntries> entries {};
    std::array<int, maxEntries> inst {};     // pool index per entry
    std::array<float, maxEntries> gains {};  // bypass crossfade 0..1
    std::array<bool, (size_t) numFxTypes * maxSameFxType> used {};
    int numEntries = 0;
    float xfStep = 0.01f;
    std::vector<float> dryL, dryR;
    // output stage (the v2 rack's tail)
    kk::OnePole sideLp[2];
    kk::DcBlock dc[2];
    kk::Biquad subsonic[2];
    kk::MasterStage mastering;
};
} // namespace vault
