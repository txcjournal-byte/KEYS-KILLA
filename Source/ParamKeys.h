#pragma once
#include "Params.h"
#include <array>
#include <map>
#include <cmath>
#include <algorithm>

// A fixed, small-integer view of the synth parameters that shape a sound (everything the voice + FX build reads).
// Used by kk::buildVoiceParamsFrom (EVOLVE and CHROME VAULT share it) and by the CHROME VAULT bank format.
// The order of this table is FROZEN: only ever append new keys at the end of KK_VOICE_KEYS (before the mod slots
// are appended), never reorder or remove - saved banks store their own key-name table, but tools and tests use the indices.
//
// X (identifier in ID::, min, max, default, isInteger)  -- ranges and defaults mirror createLayout() in Params.h
#define KK_VOICE_KEYS(X) \
    X(engine, 0, 9, 0, 1)        X(octave, -3, 2, 0, 1)      X(semi, -12, 12, 0, 1)      X(fine, -50, 50, 0, 0) \
    X(wave, 0, 1, 0, 0)          X(unison, 1, 8, 1, 1)       X(detune, 0, 1, 0.2f, 0)    X(fmRatio, 0.5f, 12, 2, 0) \
    X(fmRatio2, 0.5f, 16, 3, 0)  X(fmAmt, 0, 1, 0.3f, 0)     X(fmAlgo, 0, 5, 0, 1)       X(warpMode, 0, 4, 0, 1) \
    X(levelA, 0, 1, 1, 0) \
    X(layerB, 0, 1, 0, 1)        X(engineB, 0, 9, 1, 1)      X(octaveB, -3, 2, 0, 1)     X(semiB, -12, 12, 0, 1) \
    X(fineB, -50, 50, 0, 0)      X(waveB, 0, 1, 0, 0)        X(unisonB, 1, 8, 1, 1)      X(detuneB, 0, 1, 0.2f, 0) \
    X(fmRatioB, 0.5f, 12, 2, 0)  X(fmRatio2B, 0.5f, 16, 3, 0) X(fmAmtB, 0, 1, 0.3f, 0)   X(fmAlgoB, 0, 5, 0, 1) \
    X(warpModeB, 0, 4, 0, 1)     X(levelB, 0, 1, 0.7f, 0) \
    X(sub, 0, 1, 0, 0)           X(filterType, 0, 6, 0, 1)   X(cutoff, 40, 20000, 12000, 0) X(reso, 0, 1, 0.1f, 0) \
    X(keyTrack, 0, 1, 0.4f, 0)   X(fenv, -1, 1, 0, 0)        X(fattack, 0.0005f, 5, 0.001f, 0) X(fdecay, 0.005f, 4, 0.4f, 0) \
    X(fsustain, 0, 1, 0, 0)      X(frelease, 0.005f, 8, 0.3f, 0) \
    X(attack, 0.001f, 5, 0.002f, 0) X(decay, 0.005f, 8, 0.6f, 0) X(sustain, 0, 1, 0.8f, 0) X(release, 0.005f, 8, 0.3f, 0) \
    X(velSens, 0, 1, 0.6f, 0) \
    X(e3attack, 0.0005f, 5, 0.01f, 0) X(e3decay, 0.005f, 8, 0.5f, 0) X(e3sustain, 0, 1, 0, 0) X(e3release, 0.005f, 8, 0.3f, 0) \
    X(lfoRate, 0.05f, 20, 5, 0)  X(lfoSync, 0, 1, 0, 1)      X(lfoDiv, 0, 8, 3, 1)       X(lfoShape, 0, 4, 0, 1) \
    X(lfoPitch, 0, 1, 0, 0)      X(lfoFilter, 0, 1, 0, 0)    X(lfoAmp, 0, 1, 0, 0) \
    X(lfo2Rate, 0.05f, 20, 0.5f, 0) X(lfo2Sync, 0, 1, 0, 1)  X(lfo2Div, 0, 8, 2, 1)      X(lfo2Shape, 0, 4, 1, 1) \
    X(wobTarget, 0, 3, 0, 1) \
    X(mono, 0, 1, 0, 1)          X(legato, 0, 1, 1, 1)       X(glide, 0, 1, 0, 0)        X(bendRange, 1, 24, 2, 1) \
    X(bassMode, 0, 1, 0, 1) \
    X(drive, 0, 1, 0, 0)         X(driveType, 0, 4, 0, 1)    X(crush, 0, 1, 0, 0)        X(wow, 0, 1, 0, 0) \
    X(chorus, 0, 1, 0, 0)        X(phaser, 0, 1, 0, 0)       X(flanger, 0, 1, 0, 0)      X(delayMix, 0, 1, 0, 0) \
    X(delayTime, 0, 4, 2, 1)     X(delayFb, 0, 0.95f, 0.35f, 0) X(delayMode, 0, 2, 0, 1) X(revMix, 0, 1, 0.15f, 0) \
    X(revSize, 0, 1, 0.6f, 0)    X(revType, 0, 2, 0, 1)      X(eqLow, -12, 12, 0, 0)     X(eqHigh, -12, 12, 0, 0) \
    X(reverse, 0, 1, 0, 0)       X(freeze, 0, 1, 0, 1)       X(width, 0, 1, 0.5f, 0)     X(gain, -24, 12, 0, 0) \
    X(m1, 0, 1, 0.5f, 0)         X(m2, 0, 1, 0.25f, 0)       X(m3, 0, 1, 0, 0)           X(m4, 0, 1, 0, 0) \
    X(m5, 0, 1, 0.2f, 0)         X(m6, 0, 1, 0.5f, 0)        X(m7, 0, 1, 0, 0)           X(m8, 0, 1, 0.5f, 0) \
    X(ghost, 0, 1, 0, 0)         X(ghostOct, 0, 3, 0, 1)     X(ghostRev, 0, 1, 1, 1)     X(ghostBlur, 0, 1, 0.6f, 0) \
    X(bend, 0, 1, 0, 0)          X(bendMode, 0, 4, 0, 1)     X(bendSemis, -24, 24, -12, 0) X(tape, 0, 1, 0, 1) \
    X(circuit, 0, 1, 0, 0)       X(circRate, 0, 2, 1, 1)     X(morphX, 0, 1, 0.5f, 0)    X(morphY, 0, 1, 0.5f, 0) \
    X(body, 0, 6, 0, 1)          X(bodyMix, 0, 1, 0.5f, 0)   X(seed, 0, 9999, 1234, 1)   X(alive, 0, 5, 0, 1) \
    X(drift, 0, 1, 0, 0)         X(timeM, 0, 1, 0.33f, 0)    X(punch, 0, 1, 0, 0)        X(halftime, 0, 1, 0, 0) \
    X(era, 0, 6, 0, 0)           X(eraHome, 0, 6, 0, 1)      X(future, 0, 1, 0, 0)       X(master, 0, 1, 0.7f, 0)

namespace kk::vk
{
enum Key : int
{
   #define KK_VK_ENUM(n, lo, hi, def, isInt) n,
    KK_VOICE_KEYS (KK_VK_ENUM)
   #undef KK_VK_ENUM
    numBase,
    mmBase = numBase,                     // then mmSrc1..8, mmDst1..8, mmAmt1..8
    count = numBase + 3 * numModSlots
};
inline constexpr int mmSrc (int slot) { return mmBase + slot; }
inline constexpr int mmDst (int slot) { return mmBase + numModSlots + slot; }
inline constexpr int mmAmt (int slot) { return mmBase + 2 * numModSlots + slot; }

struct Info { const char* id; float lo, hi, def; bool isInt; };

// info for every key 0..count-1 (parameter ID string, real range, default, integer/choice/bool)
inline const Info& info (int key)
{
    static const auto table = []
    {
        std::array<Info, (size_t) count> t {};
        int i = 0;
       #define KK_VK_INFO(n, lo, hi, def, isInt) t[(size_t) i++] = Info { ID::n, (float) (lo), (float) (hi), (float) (def), (isInt) != 0 };
        KK_VOICE_KEYS (KK_VK_INFO)
       #undef KK_VK_INFO
        static const std::array<juce::String, 3 * numModSlots> mmIds = []
        {
            std::array<juce::String, 3 * numModSlots> a;
            for (int s = 0; s < numModSlots; ++s) { a[(size_t) s] = ID::mmSrc (s); a[(size_t) (numModSlots + s)] = ID::mmDst (s); a[(size_t) (2 * numModSlots + s)] = ID::mmAmt (s); }
            return a;
        }();
        for (int s = 0; s < numModSlots; ++s)
        {
            t[(size_t) mmSrc (s)] = Info { mmIds[(size_t) s].toRawUTF8(), 0, 9, 0, true };
            t[(size_t) mmDst (s)] = Info { mmIds[(size_t) (numModSlots + s)].toRawUTF8(), 0, 12, 1, true };
            t[(size_t) mmAmt (s)] = Info { mmIds[(size_t) (2 * numModSlots + s)].toRawUTF8(), -1, 1, 0, false };
        }
        return t;
    }();
    return table[(size_t) std::clamp (key, 0, count - 1)];
}

// parameter ID -> key, -1 if the ID is not a sound parameter (performance / module / arp pattern IDs)
inline int keyOf (const juce::String& id)
{
    static const auto map = []
    {
        std::map<juce::String, int> m;
        for (int k = 0; k < count; ++k) m[juce::String (info (k).id)] = k;
        return m;
    }();
    const auto it = map.find (id);
    return it != map.end() ? it->second : -1;
}

// clamp to the range and snap integer keys, like the APVTS does with a real parameter value
inline float sanitize (int key, float v)
{
    const auto& in = info (key);
    if (! std::isfinite (v)) v = in.def;
    v = std::clamp (v, in.lo, in.hi);
    return in.isInt ? std::round (v) : v;
}

// a full value array with every key at its default
inline void fillDefaults (float* values)
{
    for (int k = 0; k < count; ++k) values[k] = info (k).def;
}
} // namespace kk::vk
