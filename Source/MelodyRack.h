#pragma once
#include <array>
#include <atomic>
#include <juce_core/juce_core.h>

// v0.32 FX RACK: KEYS KILLA's own effects on the whole melody bus (synth + PAIR + VST + SAMPLER), drums stay dry.
// Independent of the presets: it stays when you change sounds. The UI writes, the audio thread reads (atomics).
namespace kk
{
enum RackSlot { rkDrive, rkLofi, rkChorus, rkPhaser, rkFlanger, rkHalf, rkGate, rkDelay, rkReverb, rkEq, rkWidth, numRackSlots };
enum RackValue { rvDrive, rvDriveType, rvCrush, rvChorus, rvPhaser, rvFlanger, rvHalf, rvGateRate, rvGateDepth,
                 rvDelayMix, rvDelayFb, rvDelayTime, rvRevMix, rvRevSize, rvRevType, rvEqLow, rvEqHigh, rvWidth, numRackValues };

inline const char* rackSlotName (int s)
{
    static const char* n[] { "DRIVE", "LO-FI", "CHORUS", "PHASER", "FLANGER", "HALF-TIME", "STUTTER", "DELAY", "REVERB", "EQ", "WIDTH" };
    return n[juce::jlimit (0, numRackSlots - 1, s)];
}

struct MelodyRack
{
    std::array<std::atomic<bool>, numRackSlots> on {};
    std::array<std::atomic<float>, numRackValues> v {};
    MelodyRack() { reset(); }
    void reset()
    {
        for (auto& o : on) o = false;
        static const float d[numRackValues] { 0.35f, 1, 0.3f, 0.4f, 0.35f, 0.3f, 0.5f, 1, 0.8f,
                                              0.25f, 0.35f, 2, 0.3f, 0.6f, 0, 0, 0, 0.75f };
        for (int i = 0; i < numRackValues; ++i) v[(size_t) i] = d[i];
    }
    bool anyOn() const { for (auto& o : on) if (o.load()) return true; return false; }
    juce::String toString() const
    {
        juce::StringArray a;
        for (auto& o : on) a.add (o.load() ? "1" : "0");
        for (auto& x : v) a.add (juce::String (x.load(), 4));
        return a.joinIntoString (",");
    }
    void fromString (const juce::String& s)
    {
        const auto a = juce::StringArray::fromTokens (s, ",", "");
        if (a.size() != numRackSlots + numRackValues) return;
        for (int i = 0; i < numRackSlots; ++i) on[(size_t) i] = a[i] == "1";
        for (int i = 0; i < numRackValues; ++i) v[(size_t) i] = a[numRackSlots + i].getFloatValue();
    }
};
} // namespace kk
