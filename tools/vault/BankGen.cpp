// CHROME VAULT factory bank generator (target VaultBankGen).
//
//   VaultBankGen [-out resources/vault/factory.cvb] [-report docs/VAULT_BANK_REPORT.md] [-threads N] [-scale 0.1]
//   VaultBankGen -audit <bank.cvb> [-csv out.tsv] [-every N]   independent audio audit of a bank (BankAudit.h), exit 0 = no failures
//   VaultBankGen -check <bank.cvb> | -list <bank.cvb>           re-run the gate / list ids, names, layers, tags
//   VaultBankGen -recipe <name>|all [-out b.cvb] [-report r.md] [-dist 3] [-demo dir ids]   v2 recipes (tools/vault/recipes)
//   VaultBankGen -env <bank.cvb> <id> | -switch <bank.cvb> <first> [count]   debugging: level curve / preset-switch clicks
//
// Deterministic: the same source tree gives the same bank (own RNG, every candidate seeded by its slot; candidates are
// rendered in parallel but accepted in slot order). Builds exactly v1c::target(c) presets per category and the
// layer mix v1c::layerTarget(n), from
//   * the EVOLVE factory sounds (Source/Presets.cpp, loudness-calibrated by PresetGains.h) and
//   * procedural recipes (808s, basses, bells, pads, ...) written below,
// turned into musically sensible variants (octave, brightness, envelope, era, space, detune, glide, drive, lo-fi, motion)
// and layered presets (2-4 parts with sensible roles: one bass-ish part at most, balanced levels, octave spread).
// Every preset passes the quality gate of tools/vault/BankQC.h (two test notes at 48 kHz: no silence / NaN / clipping /
// DC / clicks / stereo collapse / wide lows / over-long tails), is loudness-normalised to its category target (+-1 dB)
// and must not be a near-duplicate of an accepted preset (spectral + envelope fingerprint). Names and tags are invented
// from the measured sound. Writes the CVB1 bank and a markdown QC report.
#include "BankQC.h"
#include "BankAudit.h"
#include "recipes/RecipeGate.h"
#include "../../Source/Presets.h"
#include "../../Source/Params.h"
#include <atomic>
#include <bitset>
#include <functional>
#include <cstdio>
#include <map>
#include <set>
#include <thread>

namespace
{
namespace K = kk::vk;

// The v1 generator plans, tags and names in the OLD (v1) category list; presets are written with the v2 categories
// (vault::v1::toV2Category). Styles / characters are internal now (they only steer the names), never stored.
namespace v1c
{
using namespace vault::v1;   // the old CategoryIndex enum
enum StyleIndex { styleTrap, styleRage, styleDrill, stylePluggnb, styleDark, styleEmo, styleLoFi, styleAmbient };
enum CharIndex  { charBright, charDark, charWarm, charWide, charHard, charSoft, charDirty, charClean };
enum { numCategories = 16, numStyles = 8, numCharacters = 8, maxParts = 4 };
inline const juce::StringArray& names()
{
    static const juce::StringArray n { "Bass", "Bells & Mallets", "Piano & Keys", "Plucks", "Leads", "Pads", "Strings", "Brass & Horns",
                                       "Flutes & Winds", "Guitars", "Vocals & Choir", "Synth", "Arps & Sequences", "Orchestral", "World", "FX & Atmos" };
    return n;
}
inline int target (int c)
{
    static const int t[numCategories] { 520, 480, 420, 420, 420, 400, 300, 240, 220, 240, 280, 300, 200, 160, 160, 240 };
    return t[juce::jlimit (0, (int) numCategories - 1, c)];
}
inline int layerTarget (int numParts)
{
    static const int t[maxParts] { 1600, 1900, 1100, 400 };
    return t[juce::jlimit (1, (int) maxParts, numParts) - 1];
}
inline const juce::StringArray& styleNames()
{
    static const juce::StringArray n { "Trap", "Rage", "Drill", "Pluggnb", "Dark", "Emo", "Lo-Fi", "Ambient" };
    return n;
}
inline const juce::StringArray& characterNames()
{
    static const juce::StringArray n { "Bright", "Dark", "Warm", "Wide", "Hard", "Soft", "Dirty", "Clean" };
    return n;
}
inline juce::StringArray namesOfMask (uint32_t mask, const juce::StringArray& n)
{
    juce::StringArray out;
    for (int i = 0; i < n.size(); ++i) if (mask & (1u << i)) out.add (n[i]);
    return out;
}
inline int toV2 (int c) { return vault::v1::toV2Category (c, VaultPreset()); }
} // namespace v1c

//==============================================================================
// deterministic RNG (splitmix64) - no std distributions (they differ between standard libraries)
struct Rng
{
    uint64_t s;
    explicit Rng (uint64_t seed) : s (seed * 0x9E3779B97F4A7C15ull + 0x632BE59BD9B4E019ull) { next(); }
    uint64_t next()
    {
        uint64_t z = (s += 0x9E3779B97F4A7C15ull);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    float uni() { return (float) ((double) (next() >> 40) / 16777216.0); }
    float range (float a, float b) { return a + (b - a) * uni(); }
    float logRange (float a, float b) { return a * std::pow (b / a, uni()); }
    int below (int n) { return n <= 1 ? 0 : (int) (next() % (uint64_t) n); }
    int between (int a, int b) { return a + below (b - a + 1); }
    bool chance (float p) { return uni() < p; }
    template <typename T> const T& pick (const std::vector<T>& v) { return v[(size_t) below ((int) v.size())]; }
    template <typename T, size_t N> const T& pick (const T (&a)[N]) { return a[below ((int) N)]; }
};
inline uint64_t seedOf (uint64_t a, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0)
{
    Rng r (a * 1000003ull ^ (b + 0x51ED27ull) * 7919ull ^ (c + 0x1234ull) * 104729ull ^ (d + 77ull) * 15485863ull);
    return r.next();
}

//==============================================================================
// a sound patch: every kk::vk key, with the keys set explicitly marked (only those are written to the bank)
struct Patch
{
    std::array<float, K::count> v {};
    std::bitset<K::count> on;
    Patch() { K::fillDefaults (v.data()); }
    float operator[] (int k) const { return v[(size_t) k]; }
    Patch& set (int k, float x) { v[(size_t) k] = K::sanitize (k, x); on.set ((size_t) k); return *this; }
    Patch& mul (int k, float f) { return set (k, v[(size_t) k] * f); }
    Patch& add (int k, float d) { return set (k, v[(size_t) k] + d); }
    void reset (int k) { v[(size_t) k] = K::info (k).def; on.reset ((size_t) k); }
    std::vector<std::pair<juce::String, float>> values() const
    {
        std::vector<std::pair<juce::String, float>> out;
        for (int k = 0; k < K::count; ++k)
            if (on[(size_t) k] && ! juce::exactlyEqual (v[(size_t) k], K::info (k).def)) out.push_back ({ K::info (k).id, v[(size_t) k] });
        return out;
    }
    static Patch fromValues (const std::vector<std::pair<juce::String, float>>& vals)
    {
        Patch p;
        for (auto& [id, x] : vals) if (const int k = K::keyOf (id); k >= 0) p.set (k, x);
        return p;
    }
    bool addMod (int src, int dst, float amt)
    {
        for (int s = 0; s < numModSlots; ++s)
            if ((int) v[(size_t) K::mmSrc (s)] == srcOff || juce::exactlyEqual (v[(size_t) K::mmAmt (s)], 0.0f))
            { set (K::mmSrc (s), (float) src); set (K::mmDst (s), (float) dst); set (K::mmAmt (s), amt); return true; }
        return false;
    }
    int eng() const { return (int) v[(size_t) K::engine]; }
};

// keys only the master FX reads (part 0) - stripped from parts 1-3, the engine ignores them there
const std::vector<int>& fxOnlyKeys()
{
    static const std::vector<int> k { K::drive, K::driveType, K::crush, K::wow, K::chorus, K::phaser, K::flanger, K::delayMix, K::delayTime,
                                      K::delayFb, K::delayMode, K::revMix, K::revSize, K::revType, K::eqLow, K::eqHigh, K::reverse, K::freeze,
                                      K::width, K::ghostBlur, K::ghostRev, K::circuit, K::circRate, K::body, K::bodyMix, K::punch, K::halftime,
                                      K::master };
    return k;
}

//==============================================================================
enum Role { rBass, rBell, rMallet, rKeys, rPluck, rLead, rPad, rStrings, rBrass, rWind, rGuitar, rChoir, rSynth, rArp, rTexture, rFx,
            rWorld, rCine, rGrit, rKnock, numRoles };
const char* roleName (int r)
{
    static const char* n[numRoles] { "bass", "bell", "mallet", "keys", "pluck", "lead", "pad", "strings", "brass", "wind", "guitar", "choir",
                                     "synth", "arp", "texture", "fx", "world", "cine", "grit", "knock" };
    return n[juce::jlimit (0, (int) numRoles - 1, r)];
}
bool isSustainedRole (int r) { return r == rPad || r == rStrings || r == rBrass || r == rWind || r == rChoir || r == rLead || r == rTexture; }

struct Src
{
    juce::String name;          // factory name or recipe tag (for the report only - never shown)
    int role = rPad;
    int home = -1;              // vault category it can lead (-1: layer only)
    int kkCat = -1;             // factory category (-1: recipe)
    juce::String kind;          // finer type word for names (e.g. "organ", "808", "ep")
    Patch patch;
    int weight = 1;             // how often it is picked as a main source
    bool ok = true;
    float calLoud = 0;          // single-part loudness after calibration
};

//==============================================================================
// procedural recipes
using namespace ID;
Patch base (int engineId)
{
    Patch p;
    p.set (K::engine, (float) engineId);
    return p;
}
void setEnv (Patch& p, float a, float d, float s, float r) { p.set (K::attack, a).set (K::decay, d).set (K::sustain, s).set (K::release, r); }
void space (Patch& p, Rng& r, float revLo, float revHi, float dlyChance = 0.3f)
{
    p.set (K::revMix, r.range (revLo, revHi)).set (K::revSize, r.range (0.45f, 0.85f)).set (K::revType, (float) r.below (3));
    if (r.chance (dlyChance)) p.set (K::delayMix, r.range (0.08f, 0.25f)).set (K::delayTime, (float) r.below (5)).set (K::delayFb, r.range (0.2f, 0.45f));
}

Patch recipe808 (Rng& r, juce::String& kind)
{
    Patch p = base (engSub);
    const int flavour = r.below (5);   // 0 clean, 1 warm, 2 hard, 3 distorted, 4 long glide
    const float sat = flavour == 0 ? r.range (0.0f, 0.15f) : flavour == 1 ? r.range (0.2f, 0.4f) : flavour == 2 ? r.range (0.4f, 0.65f) : r.range (0.55f, 0.85f);
    p.set (K::wave, sat).set (K::fmAmt, r.range (0.06f, 0.4f));
    const bool longOne = flavour == 4 || r.chance (0.35f);
    setEnv (p, r.range (0.001f, 0.004f), longOne ? r.range (1.8f, 3.6f) : r.range (0.5f, 1.6f), r.chance (0.35f) ? r.range (0.1f, 0.45f) : 0.0f, r.range (0.08f, 0.3f));
    p.set (K::mono, 1).set (K::legato, 1).set (K::bassMode, 1);
    p.set (K::glide, flavour == 4 ? r.range (0.12f, 0.35f) : r.chance (0.45f) ? r.range (0.03f, 0.2f) : 0.0f);
    if (flavour >= 2) p.set (K::drive, r.range (0.15f, flavour == 3 ? 0.65f : 0.4f)).set (K::driveType, (float) r.below (5));
    p.set (K::m1, r.chance (0.3f) ? r.range (0.05f, 0.3f) : 0.0f);        // sub one octave down (bass macro 1)
    p.set (K::m2, 0.0f);                                                  // no wobble
    p.set (K::m5, r.range (0.4f, 0.7f));                                  // tone
    p.set (K::m6, r.range (0.0f, 0.45f));                                 // punch
    if (r.chance (0.3f)) p.set (K::layerB, 1).set (K::engineB, engVA).set (K::waveB, r.range (0.0f, 1.0f)).set (K::octaveB, (float) r.between (0, 1))
                          .set (K::levelB, r.range (0.06f, 0.2f));
    p.set (K::revMix, 0).set (K::chorus, 0).set (K::width, 0.4f);
    kind = flavour == 4 ? "slide" : flavour >= 2 ? "hard808" : "808";
    return p;
}
Patch recipeSynthBass (Rng& r, juce::String& kind)
{
    Patch p = base (r.chance (0.75f) ? engVA : engWavetable);
    p.set (K::wave, r.range (0.0f, 0.6f)).set (K::unison, (float) r.between (1, 5)).set (K::detune, r.range (0.08f, 0.35f));
    p.set (K::filterType, r.chance (0.6f) ? 1.0f : 0.0f).set (K::cutoff, r.logRange (280.0f, 1400.0f)).set (K::reso, r.range (0.05f, 0.4f));
    p.set (K::fenv, r.range (0.15f, 0.6f)).set (K::fdecay, r.range (0.12f, 0.5f));
    setEnv (p, r.range (0.001f, 0.006f), r.range (0.3f, 1.0f), r.range (0.6f, 1.0f), r.range (0.08f, 0.25f));
    p.set (K::mono, 1).set (K::legato, 1).set (K::bassMode, 1).set (K::glide, r.chance (0.5f) ? r.range (0.02f, 0.15f) : 0.0f);
    p.set (K::m1, r.range (0.0f, 0.5f)).set (K::m2, r.chance (0.25f) ? r.range (0.1f, 0.4f) : 0.0f).set (K::m5, 0.5f).set (K::m6, r.range (0.0f, 0.3f));
    if (r.chance (0.3f)) p.set (K::drive, r.range (0.1f, 0.45f)).set (K::driveType, (float) r.below (5));
    p.set (K::revMix, 0).set (K::chorus, 0).set (K::width, 0.4f);
    kind = p[K::unison] >= 3 && p[K::detune] > 0.2f ? "reese" : "bass";
    return p;
}
Patch recipeBell (Rng& r, juce::String& kind)
{
    static const float ratios[] { 2.0f, 3.0f, 3.5f, 4.0f, 5.0f, 7.0f, 1.41f, 2.76f };
    Patch p = base (engFM);
    p.set (K::fmAlgo, (float) r.below (6)).set (K::fmRatio, r.pick (ratios)).set (K::fmRatio2, r.pick (ratios)).set (K::fmAmt, r.range (0.2f, 0.7f));
    p.set (K::wave, r.range (0.0f, 0.25f)).set (K::octave, (float) r.between (0, 1));
    setEnv (p, 0.001f, r.range (0.8f, 3.0f), 0.0f, r.range (0.4f, 1.5f));
    p.set (K::cutoff, r.logRange (5000.0f, 15000.0f)).set (K::width, r.range (0.5f, 0.8f));
    if (r.chance (0.35f)) p.set (K::chorus, r.range (0.1f, 0.35f));
    space (p, r, 0.2f, 0.45f, 0.4f);
    kind = "bell";
    return p;
}
Patch recipeMallet (Rng& r, juce::String& kind)
{
    Patch p = base (engModal);
    const float w = r.uni();
    p.set (K::wave, w).set (K::fmAmt, r.range (0.1f, 0.7f));
    setEnv (p, 0.001f, r.range (0.4f, 2.2f), 0.0f, r.range (0.25f, 0.9f));
    p.set (K::cutoff, r.logRange (4000.0f, 14000.0f));
    if (r.chance (0.3f)) p.set (K::body, (float) r.between (1, 6)).set (K::bodyMix, r.range (0.2f, 0.45f));
    space (p, r, 0.12f, 0.35f, 0.25f);
    kind = w < 0.34f ? "thumb" : w < 0.67f ? "marimba" : "bell";
    return p;
}
Patch recipeKeys (Rng& r, juce::String& kind)
{
    const int t = r.below (3);
    Patch p;
    if (t == 0)        // electric piano: FM tines
    {
        p = base (engFM);
        p.set (K::fmAlgo, r.chance (0.5f) ? 0.0f : 2.0f).set (K::fmRatio, r.chance (0.7f) ? 1.0f : 2.0f).set (K::fmRatio2, 14.0f).set (K::fmAmt, r.range (0.12f, 0.35f));
        setEnv (p, 0.001f, r.range (1.5f, 3.2f), r.range (0.15f, 0.4f), r.range (0.3f, 0.6f));
        p.set (K::chorus, r.range (0.15f, 0.4f)).set (K::cutoff, r.logRange (2500.0f, 8000.0f));
        if (r.chance (0.4f)) p.set (K::lfoAmp, r.range (0.05f, 0.15f)).set (K::lfoRate, r.range (3.5f, 6.0f));
        kind = "ep";
    }
    else if (t == 1)   // piano-ish: FM body + pluck hammer
    {
        p = base (engFM);
        p.set (K::fmAlgo, 2).set (K::fmRatio, 1.0f).set (K::fmRatio2, 2.0f).set (K::fmAmt, r.range (0.18f, 0.32f));
        setEnv (p, 0.001f, r.range (2.0f, 4.0f), r.range (0.0f, 0.1f), r.range (0.4f, 0.8f));
        p.set (K::layerB, 1).set (K::engineB, engPluck).set (K::waveB, r.range (0.5f, 0.9f)).set (K::levelB, r.range (0.2f, 0.4f));
        p.set (K::cutoff, r.logRange (3000.0f, 9000.0f));
        kind = "piano";
    }
    else               // organ
    {
        p = base (engOrgan);
        p.set (K::wave, r.uni());
        setEnv (p, r.range (0.004f, 0.02f), 0.6f, 1.0f, r.range (0.08f, 0.3f));
        p.set (K::chorus, r.range (0.15f, 0.5f));
        if (r.chance (0.5f)) p.set (K::lfoAmp, r.range (0.03f, 0.1f)).set (K::lfoRate, r.range (5.0f, 7.0f));
        kind = "organ";
    }
    space (p, r, 0.12f, 0.35f, 0.2f);
    return p;
}
Patch recipePluck (Rng& r, juce::String& kind)
{
    Patch p;
    if (r.chance (0.55f))
    {
        p = base (r.chance (0.7f) ? engVA : engWavetable);
        p.set (K::wave, r.uni()).set (K::unison, (float) r.between (1, 5)).set (K::detune, r.range (0.08f, 0.3f));
        p.set (K::cutoff, r.logRange (350.0f, 1800.0f)).set (K::fenv, r.range (0.4f, 0.85f)).set (K::fdecay, r.range (0.08f, 0.3f)).set (K::reso, r.range (0.05f, 0.35f));
        setEnv (p, 0.001f, r.range (0.2f, 0.6f), 0.0f, r.range (0.15f, 0.35f));
    }
    else
    {
        p = base (engPluck);
        p.set (K::wave, r.range (0.3f, 0.95f));
        setEnv (p, 0.001f, r.range (0.4f, 1.4f), 0.0f, r.range (0.2f, 0.4f));
        p.set (K::cutoff, r.logRange (2500.0f, 9000.0f));
        if (r.chance (0.4f)) p.set (K::body, (float) r.between (1, 6)).set (K::bodyMix, r.range (0.2f, 0.45f));
    }
    space (p, r, 0.15f, 0.35f, 0.55f);
    kind = "pluck";
    return p;
}
Patch recipeLead (Rng& r, juce::String& kind)
{
    const int e = r.below (4);
    Patch p = base (e == 0 ? engVA : e == 1 ? engWavetable : e == 2 ? engFlute : engVA);
    p.set (K::wave, e == 2 ? r.range (0.0f, 0.15f) : r.uni()).set (K::unison, (float) (e == 2 ? 1 : r.between (1, 8))).set (K::detune, r.range (0.1f, 0.35f));
    p.set (K::cutoff, r.logRange (1800.0f, 9000.0f)).set (K::reso, r.range (0.0f, 0.3f));
    setEnv (p, r.range (0.002f, 0.02f), 0.6f, r.range (0.85f, 1.0f), r.range (0.15f, 0.35f));
    p.set (K::mono, 1).set (K::legato, 1).set (K::glide, r.range (0.02f, 0.14f));
    p.set (K::lfoPitch, r.range (0.0f, 0.07f)).set (K::lfoRate, r.range (4.5f, 6.0f));
    if (e != 2 && r.chance (0.35f)) p.set (K::drive, r.range (0.1f, 0.4f)).set (K::driveType, (float) r.below (5));
    if (e == 2) p.set (K::octave, 1);
    space (p, r, 0.15f, 0.35f, 0.6f);
    kind = e == 2 ? "whistle" : "lead";
    return p;
}
Patch recipePad (Rng& r, juce::String& kind)
{
    static const int engines[] { engVA, engWavetable, engVox, engOrchestral, engVA };
    Patch p = base (r.pick (engines));
    p.set (K::wave, r.uni()).set (K::unison, (float) r.between (4, 8)).set (K::detune, r.range (0.12f, 0.35f));
    p.set (K::cutoff, r.logRange (1200.0f, 6000.0f)).set (K::reso, r.range (0.0f, 0.25f));
    setEnv (p, r.range (0.25f, 1.6f), 1.0f, 1.0f, r.range (1.0f, 3.0f));
    p.set (K::lfoFilter, r.range (0.05f, 0.3f)).set (K::lfoRate, r.logRange (0.08f, 0.6f));
    p.set (K::chorus, r.range (0.1f, 0.4f)).set (K::width, r.range (0.7f, 0.95f));
    if (r.chance (0.3f)) p.set (K::layerB, 1).set (K::engineB, (float) r.pick (engines)).set (K::waveB, r.uni()).set (K::octaveB, (float) r.between (-1, 1))
                          .set (K::levelB, r.range (0.3f, 0.6f)).set (K::unisonB, (float) r.between (2, 6)).set (K::detuneB, r.range (0.1f, 0.3f));
    space (p, r, 0.35f, 0.55f, 0.25f);
    kind = "pad";
    return p;
}
Patch recipeStrings (Rng& r, juce::String& kind)
{
    Patch p = base (r.chance (0.7f) ? engOrchestral : engVA);
    const bool stacc = r.chance (0.3f);
    p.set (K::wave, p.eng() == engOrchestral ? r.range (0.35f, 0.6f) : r.range (0.0f, 0.15f)).set (K::unison, (float) r.between (3, 7)).set (K::detune, r.range (0.08f, 0.2f));
    p.set (K::cutoff, r.logRange (2000.0f, 6500.0f));
    if (stacc) setEnv (p, r.range (0.003f, 0.015f), r.range (0.2f, 0.45f), 0.0f, r.range (0.15f, 0.3f));
    else setEnv (p, r.range (0.08f, 0.5f), 1.0f, 1.0f, r.range (0.5f, 1.2f));
    p.set (K::lfoPitch, r.range (0.02f, 0.05f)).set (K::lfoRate, r.range (4.8f, 5.6f));
    if (r.chance (0.4f)) p.set (K::octave, -1);
    space (p, r, 0.28f, 0.45f, 0.15f);
    kind = stacc ? "staccato" : "strings";
    return p;
}
Patch recipeBrass (Rng& r, juce::String& kind)
{
    Patch p = base (r.chance (0.65f) ? engOrchestral : engVA);
    const bool stab = r.chance (0.35f);
    p.set (K::wave, p.eng() == engOrchestral ? r.range (0.0f, 0.2f) : r.range (0.0f, 0.1f)).set (K::unison, (float) r.between (2, 5)).set (K::detune, r.range (0.08f, 0.18f));
    p.set (K::cutoff, r.logRange (1200.0f, 4000.0f)).set (K::fenv, r.range (0.25f, 0.6f)).set (K::fdecay, r.range (0.2f, 0.5f)).set (K::fsustain, r.range (0.2f, 0.5f));
    if (stab) setEnv (p, r.range (0.004f, 0.015f), r.range (0.25f, 0.5f), r.range (0.1f, 0.3f), r.range (0.15f, 0.3f));
    else setEnv (p, r.range (0.03f, 0.12f), 0.6f, r.range (0.8f, 1.0f), r.range (0.25f, 0.5f));
    if (r.chance (0.3f)) p.set (K::octave, -1);
    space (p, r, 0.18f, 0.35f, 0.15f);
    kind = stab ? "stab" : "brass";
    return p;
}
Patch recipeWind (Rng& r, juce::String& kind)
{
    Patch p;
    if (r.chance (0.7f))
    {
        p = base (engFlute);
        p.set (K::wave, r.range (0.03f, 0.35f)).set (K::octave, (float) r.between (0, 1));
        setEnv (p, r.range (0.03f, 0.12f), 0.6f, 0.95f, r.range (0.18f, 0.4f));
        p.set (K::lfoPitch, r.range (0.02f, 0.07f)).set (K::lfoRate, r.range (4.5f, 5.8f));
        kind = "flute";
    }
    else
    {
        p = base (engVA);
        p.set (K::wave, r.range (0.45f, 0.7f)).set (K::filterType, 4).set (K::cutoff, r.logRange (900.0f, 2200.0f)).set (K::reso, r.range (0.1f, 0.3f));
        setEnv (p, r.range (0.03f, 0.08f), 0.6f, 0.95f, r.range (0.15f, 0.3f));
        p.set (K::lfoPitch, r.range (0.02f, 0.05f));
        kind = "reed";
    }
    if (r.chance (0.4f)) p.set (K::mono, 1).set (K::glide, r.range (0.02f, 0.08f));
    space (p, r, 0.22f, 0.42f, 0.3f);
    return p;
}
Patch recipeGuitar (Rng& r, juce::String& kind)
{
    Patch p = base (engPluck);
    const bool electric = r.chance (0.35f);
    p.set (K::wave, r.range (0.4f, 0.9f));
    setEnv (p, 0.001f, r.range (1.4f, 3.0f), 0.0f, r.range (0.2f, 0.4f));
    p.set (K::cutoff, r.logRange (2500.0f, 7000.0f));
    if (! electric) p.set (K::body, (float) r.between (5, 6)).set (K::bodyMix, r.range (0.3f, 0.5f));
    else p.set (K::drive, r.range (0.2f, 0.55f)).set (K::driveType, 2);
    if (r.chance (0.45f)) p.set (K::chorus, r.range (0.15f, 0.45f));
    space (p, r, 0.15f, 0.3f, 0.35f);
    kind = electric ? "electric" : p[K::wave] < 0.55f ? "nylon" : "guitar";
    return p;
}
Patch recipeChoir (Rng& r, juce::String& kind)
{
    Patch p = base (r.chance (0.65f) ? engVox : engOrchestral);
    p.set (K::wave, p.eng() == engOrchestral ? r.range (0.85f, 1.0f) : r.uni()).set (K::unison, (float) r.between (4, 7)).set (K::detune, r.range (0.12f, 0.25f));
    setEnv (p, r.range (0.15f, 0.6f), 1.0f, 1.0f, r.range (0.9f, 2.0f));
    p.set (K::width, r.range (0.75f, 0.95f)).set (K::lfoPitch, r.range (0.0f, 0.04f));
    space (p, r, 0.38f, 0.55f, 0.15f);
    kind = "choir";
    return p;
}
Patch recipeSynth (Rng& r, juce::String& kind)
{
    static const int engines[] { engVA, engWavetable, engFM, engVA };
    Patch p = base (r.pick (engines));
    p.set (K::wave, r.uni()).set (K::unison, (float) r.between (2, 6)).set (K::detune, r.range (0.1f, 0.3f));
    if (p.eng() == engFM) p.set (K::fmAmt, r.range (0.15f, 0.5f)).set (K::fmRatio, (float) r.between (1, 4)).set (K::fmAlgo, (float) r.below (6));
    p.set (K::cutoff, r.logRange (1200.0f, 6000.0f)).set (K::fenv, r.range (0.15f, 0.5f)).set (K::fdecay, r.range (0.2f, 0.7f));
    setEnv (p, r.range (0.002f, 0.03f), r.range (0.4f, 1.2f), r.range (0.4f, 0.8f), r.range (0.3f, 0.8f));
    p.set (K::chorus, r.range (0.0f, 0.35f));
    space (p, r, 0.15f, 0.35f, 0.35f);
    kind = "synth";
    return p;
}
Patch recipeTexture (Rng& r, juce::String& kind)
{
    Patch p;
    if (r.chance (0.5f))
    {
        p = base (engFlute);
        static const float filters[] { 0.0f, 3.0f, 4.0f };
        p.set (K::wave, r.range (0.75f, 1.0f)).set (K::filterType, r.pick (filters));
        p.set (K::cutoff, r.logRange (1500.0f, 7000.0f)).set (K::octave, (float) r.between (0, 1));
    }
    else
    {
        p = base (engWavetable);
        p.set (K::wave, r.uni()).set (K::unison, (float) r.between (3, 7)).set (K::detune, r.range (0.2f, 0.45f)).set (K::cutoff, r.logRange (1000.0f, 5000.0f));
        p.set (K::phaser, r.range (0.0f, 0.35f));
    }
    setEnv (p, r.range (0.4f, 1.8f), 1.0f, 1.0f, r.range (1.5f, 3.5f));
    p.set (K::lfoFilter, r.range (0.1f, 0.35f)).set (K::lfoRate, r.logRange (0.06f, 0.5f)).set (K::width, r.range (0.8f, 1.0f));
    if (r.chance (0.35f)) p.set (K::crush, r.range (0.05f, 0.25f)).set (K::wow, r.range (0.1f, 0.4f));
    space (p, r, 0.4f, 0.6f, 0.3f);
    kind = "texture";
    return p;
}
Patch recipeFx (Rng& r, juce::String& kind)
{
    Patch p = base (r.chance (0.5f) ? engVA : engWavetable);
    p.set (K::wave, r.uni()).set (K::unison, (float) r.between (3, 7)).set (K::detune, r.range (0.2f, 0.5f)).set (K::cutoff, r.logRange (600.0f, 3000.0f)).set (K::reso, r.range (0.1f, 0.4f));
    const int t = r.below (3);
    if (t == 0)        // riser: env3 opens the filter and lifts the pitch
    {
        setEnv (p, r.range (0.6f, 1.2f), 1.0f, 1.0f, r.range (0.8f, 1.6f));
        p.set (K::e3attack, r.range (1.0f, 2.5f)).set (K::e3sustain, 1.0f);
        p.addMod (srcEnv3, dstCutoff, r.range (0.4f, 0.8f));
        p.addMod (srcEnv3, dstPitch, r.range (0.3f, 1.0f));
        kind = "riser";
    }
    else if (t == 1)   // falling sweep
    {
        setEnv (p, r.range (0.005f, 0.03f), r.range (1.2f, 2.4f), 0.0f, r.range (0.5f, 1.2f));
        p.set (K::e3decay, r.range (0.8f, 1.8f)).set (K::e3sustain, 0.0f);
        p.addMod (srcEnv3, dstPitch, r.range (0.3f, 1.0f));
        kind = "sweep";
    }
    else               // pulsing atmosphere
    {
        setEnv (p, r.range (0.3f, 1.0f), 1.0f, 1.0f, r.range (1.2f, 2.5f));
        p.set (K::lfoSync, 1).set (K::lfoDiv, (float) r.between (2, 5)).set (K::lfoShape, 1).set (K::lfoAmp, r.range (0.25f, 0.6f));
        p.set (K::lfoFilter, r.range (0.1f, 0.3f));
        kind = "pulse";
    }
    p.set (K::width, r.range (0.8f, 1.0f)).set (K::phaser, r.range (0.0f, 0.3f));
    space (p, r, 0.35f, 0.55f, 0.45f);
    return p;
}
Patch recipeWorld (Rng& r, juce::String& kind)
{
    const int t = r.below (5);
    Patch p;
    if (t == 0) { p = base (engModal); p.set (K::wave, r.range (0.0f, 0.3f)).set (K::fmAmt, r.range (0.2f, 0.6f)); setEnv (p, 0.001f, r.range (0.6f, 1.8f), 0, r.range (0.3f, 0.8f)); kind = "thumb"; }
    else if (t == 1) { p = base (engPluck); p.set (K::wave, r.range (0.5f, 0.95f)).set (K::body, (float) r.between (5, 6)).set (K::bodyMix, r.range (0.35f, 0.55f));
                       setEnv (p, 0.001f, r.range (0.8f, 3.0f), 0, r.range (0.3f, 1.0f)); kind = r.chance (0.5f) ? "zither" : "lute"; }
    else if (t == 2) { p = base (engFlute); p.set (K::wave, r.range (0.15f, 0.45f)).set (K::octave, (float) r.between (0, 1));
                       setEnv (p, r.range (0.04f, 0.15f), 0.6f, 0.95f, r.range (0.25f, 0.5f)); p.set (K::lfoPitch, r.range (0.03f, 0.08f)); kind = "pipe"; }
    else if (t == 3) { p = base (engOrgan); p.set (K::wave, r.uni()).set (K::octave, -1); setEnv (p, r.range (0.4f, 1.2f), 1, 1, r.range (1.2f, 2.5f));
                       p.set (K::layerB, 1).set (K::engineB, engOrgan).set (K::semiB, 7).set (K::levelB, r.range (0.3f, 0.5f)); kind = "drone"; }
    else { p = base (engVA); p.set (K::wave, r.range (0.5f, 0.7f)).set (K::filterType, 4).set (K::cutoff, r.logRange (1000.0f, 2400.0f)).set (K::reso, r.range (0.2f, 0.4f));
           setEnv (p, r.range (0.03f, 0.08f), 0.6f, 0.95f, r.range (0.2f, 0.4f)); p.set (K::lfoPitch, r.range (0.03f, 0.06f)).set (K::mono, 1).set (K::glide, r.range (0.03f, 0.1f));
           p.set (K::bend, r.range (0.1f, 0.3f)).set (K::bendMode, 2).set (K::bendSemis, 1); kind = "reed"; }
    space (p, r, 0.2f, 0.4f, 0.3f);
    return p;
}
Patch recipeCine (Rng& r, juce::String& kind)
{
    Patch p;
    if (r.chance (0.6f))   // braam / tutti swell
    {
        p = base (r.chance (0.5f) ? engVA : engOrchestral);
        p.set (K::wave, r.range (0.0f, 0.25f)).set (K::unison, (float) r.between (4, 8)).set (K::detune, r.range (0.12f, 0.3f)).set (K::octave, -1);
        p.set (K::cutoff, r.logRange (500.0f, 1600.0f)).set (K::fenv, r.range (0.3f, 0.6f)).set (K::fattack, r.range (0.05f, 0.3f)).set (K::fdecay, r.range (0.6f, 1.5f));
        setEnv (p, r.range (0.02f, 0.15f), r.range (1.5f, 3.0f), r.range (0.4f, 0.8f), r.range (0.8f, 1.6f));
        p.set (K::drive, r.range (0.1f, 0.35f)).set (K::driveType, 1);
        kind = "braam";
    }
    else                    // low impact
    {
        p = base (engSub);
        p.set (K::wave, r.range (0.2f, 0.5f)).set (K::fmAmt, r.range (0.3f, 0.6f)).set (K::octave, -1);
        setEnv (p, 0.002f, r.range (1.2f, 2.4f), 0.0f, r.range (0.6f, 1.2f));
        p.set (K::layerB, 1).set (K::engineB, engVA).set (K::waveB, r.range (0.0f, 0.3f)).set (K::unisonB, 6).set (K::detuneB, 0.4f).set (K::levelB, r.range (0.2f, 0.4f));
        kind = "hit";
    }
    space (p, r, 0.35f, 0.5f, 0.1f);
    return p;
}
Patch recipeGrit (Rng& r, juce::String& kind)   // 808 top layer: high-passed buzz, never touches the sub
{
    Patch p = base (engVA);
    p.set (K::wave, r.uni()).set (K::unison, (float) r.between (1, 4)).set (K::detune, r.range (0.1f, 0.4f));
    p.set (K::filterType, 3).set (K::cutoff, r.logRange (280.0f, 700.0f)).set (K::reso, r.range (0.0f, 0.2f));
    setEnv (p, r.range (0.002f, 0.006f), r.range (0.4f, 1.5f), r.range (0.0f, 0.4f), r.range (0.08f, 0.25f));
    p.set (K::mono, 1).set (K::legato, 1).set (K::glide, r.range (0.0f, 0.15f));
    kind = "grit";
    return p;
}
Patch recipeKnock (Rng& r, juce::String& kind)  // short tuned click / knock on top of a bass
{
    Patch p = base (r.chance (0.5f) ? engFM : engPluck);
    if (p.eng() == engFM) p.set (K::fmRatio, r.range (1.0f, 3.0f)).set (K::fmAmt, r.range (0.3f, 0.7f));
    else p.set (K::wave, r.range (0.6f, 1.0f));
    setEnv (p, 0.002f, r.range (0.08f, 0.25f), 0.0f, r.range (0.06f, 0.15f));
    p.set (K::filterType, 3).set (K::cutoff, r.logRange (300.0f, 900.0f));
    kind = "knock";
    return p;
}
Patch recipeArpBase (Rng& r, juce::String& kind)
{
    return r.chance (0.5f) ? recipePluck (r, kind) : recipeSynth (r, kind);
}

using RecipeFn = Patch (*) (Rng&, juce::String&);
RecipeFn recipeFor (int role)
{
    switch (role)
    {
        case rBass:    return recipe808;
        case rBell:    return recipeBell;
        case rMallet:  return recipeMallet;
        case rKeys:    return recipeKeys;
        case rPluck:   return recipePluck;
        case rLead:    return recipeLead;
        case rPad:     return recipePad;
        case rStrings: return recipeStrings;
        case rBrass:   return recipeBrass;
        case rWind:    return recipeWind;
        case rGuitar:  return recipeGuitar;
        case rChoir:   return recipeChoir;
        case rSynth:   return recipeSynth;
        case rArp:     return recipeArpBase;
        case rTexture: return recipeTexture;
        case rFx:      return recipeFx;
        case rWorld:   return recipeWorld;
        case rCine:    return recipeCine;
        case rGrit:    return recipeGrit;
        case rKnock:   return recipeKnock;
        default:       return recipePad;
    }
}

// Arps & Sequences: a held note becomes a tempo-synced sequence - LFO2 (square, synced) steps the pitch between two
// in-key notes (octave / fifth / fourth), LFO1 (synced ramp) plucks the filter every step, synced delay echoes.
void makeSequence (Patch& p, Rng& r)
{
    static const int intervals[] { 12, 12, 12, 7, 5, 24 };
    const int iv = r.pick (intervals);
    // square LFO is +-1: pitch = centre +- amt * 12 semitones, centre lifted by iv / 2 so the two steps are root and root + iv
    p.set (K::lfo2Sync, 1).set (K::lfo2Div, (float) r.between (2, 4)).set (K::lfo2Shape, 3);
    const float half = (float) iv * 0.5f;
    p.set (K::semi, std::floor (half)).set (K::fine, (half - std::floor (half)) * 100.0f);
    p.addMod (srcLfo2, dstPitch, half / 12.0f);
    p.set (K::lfoSync, 1).set (K::lfoDiv, (float) r.between (4, 5)).set (K::lfoShape, 2).set (K::lfoFilter, r.range (0.25f, 0.55f));
    p.set (K::lfoPitch, 0).set (K::lfoAmp, 0);
    if (p[K::cutoff] > 6000.0f) p.set (K::cutoff, r.logRange (1500.0f, 5000.0f));
    if (p[K::sustain] < 0.3f) setEnv (p, std::max (0.002f, p[K::attack]), 0.8f, r.range (0.5f, 0.8f), std::max (0.15f, p[K::release]));
    p.set (K::delayMix, r.range (0.15f, 0.32f)).set (K::delayTime, r.chance (0.5f) ? 2.0f : 1.0f).set (K::delayFb, r.range (0.25f, 0.45f));
    p.set (K::mono, 0);
}

//==============================================================================
// variant treatments
enum Group { gBass, gSustain, gPerc, gAtmos };
int groupOf (int role)
{
    switch (role)
    {
        case rBass: return gBass;
        case rPad: case rStrings: case rBrass: case rWind: case rChoir: case rLead: case rSynth: return gSustain;
        case rTexture: case rFx: case rCine: return gAtmos;
        default: return gPerc;
    }
}
bool hasUnison (const Patch& p) { const int e = p.eng(); return e == engVA || e == engVox || e == engFM || e == engWavetable || e == engOrchestral; }

juce::String applyTreatment (Patch& p, Rng& r, int which, int role)
{
    const int grp = groupOf (role);
    switch (which)
    {
        case 0:   // brightness
        {
            const float oct = (r.chance (0.5f) ? 1.0f : -1.0f) * r.range (0.5f, 1.4f);
            const float c = p[K::cutoff];
            p.set (K::cutoff, juce::jlimit (grp == gBass ? 250.0f : 500.0f, 18000.0f, (c >= 11999.0f && oct > 0 ? 6000.0f : c) * std::exp2 (oct)));
            if (grp != gBass && r.chance (0.4f)) p.set (K::eqHigh, juce::jlimit (-6.0f, 6.0f, oct * 3.0f));
            return oct > 0 ? "brighter" : "darker";
        }
        case 1:   // envelope
        {
            if (grp == gBass)
            {
                p.mul (K::decay, r.range (0.6f, 1.7f));
                if (r.chance (0.4f)) p.set (K::sustain, p[K::sustain] > 0.05f ? 0.0f : r.range (0.15f, 0.4f));
                return "decay";
            }
            if (grp == gSustain || grp == gAtmos)
            {
                const bool slower = r.chance (0.5f);
                p.set (K::attack, slower ? std::max (p[K::attack], 0.02f) * r.range (2.0f, 5.0f) : std::max (0.003f, p[K::attack] * r.range (0.2f, 0.5f)));
                p.mul (K::release, r.range (0.6f, 1.8f));
                if (p[K::attack] > 2.0f) p.set (K::attack, 2.0f);
                return slower ? "slow attack" : "fast attack";
            }
            p.mul (K::decay, r.range (0.5f, 2.0f));
            p.mul (K::release, r.range (0.6f, 2.0f));
            return "decay";
        }
        case 2:   // era (EVOLVE era profile: workstation / layered / atmospheric / lo-fi / rage / fast / future)
        {
            const int home = (int) p[K::eraHome];
            int e = r.below (7);
            if (e == home) e = (e + 1 + r.below (6)) % 7;
            p.set (K::era, (float) e);
            p.set (K::eraHome, (float) home);
            return "era " + juce::String (e);
        }
        case 3:   // space
        {
            if (grp == gBass) { p.set (K::revMix, r.range (0.0f, 0.08f)); return "room"; }
            const bool more = r.chance (0.55f);
            p.set (K::revMix, more ? juce::jlimit (0.15f, 0.6f, p[K::revMix] + r.range (0.12f, 0.3f)) : p[K::revMix] * r.range (0.3f, 0.6f));
            p.set (K::revSize, r.range (0.4f, 0.9f)).set (K::revType, (float) r.below (3));
            if (r.chance (0.5f)) p.set (K::delayMix, r.range (0.1f, 0.3f)).set (K::delayTime, (float) r.below (5)).set (K::delayFb, r.range (0.2f, 0.5f));
            return more ? "more space" : "drier";
        }
        case 4:   // chorus / phaser / width
        {
            if (grp == gBass) { p.set (K::m6, r.range (0.0f, 0.5f)); return "punch"; }
            const int t = r.below (3);
            if (t == 0) p.set (K::chorus, r.range (0.2f, 0.55f));
            else if (t == 1) p.set (K::phaser, r.range (0.15f, 0.4f));
            else p.set (K::flanger, r.range (0.1f, 0.3f));
            p.set (K::width, r.range (0.6f, 0.95f));
            return t == 0 ? "chorus" : t == 1 ? "phaser" : "flanger";
        }
        case 5:   // detune / unison
        {
            if (grp == gBass)
            {
                p.set (K::glide, p[K::glide] > 0.05f ? r.range (0.0f, 0.03f) : r.range (0.08f, 0.3f));
                return "glide";
            }
            if (! hasUnison (p)) { p.set (K::drift, r.range (0.15f, 0.4f)); return "drift"; }
            p.set (K::unison, (float) (p[K::unison] >= 4 ? r.between (1, 3) : r.between (4, 8))).set (K::detune, r.range (0.1f, 0.4f));
            return "unison";
        }
        case 6:   // drive
        {
            p.set (K::drive, grp == gBass ? r.range (0.2f, 0.7f) : r.range (0.12f, 0.45f)).set (K::driveType, (float) r.below (5));
            return "drive";
        }
        case 7:   // lo-fi
        {
            if (grp == gBass) { p.set (K::wave, juce::jlimit (0.0f, 1.0f, p[K::wave] + r.range (-0.3f, 0.3f))); p.set (K::fmAmt, r.range (0.05f, 0.45f)); return "body"; }
            p.set (K::crush, r.range (0.08f, 0.3f)).set (K::wow, r.range (0.1f, 0.45f));
            return "lo-fi";
        }
        case 8:   // motion
        {
            if (grp == gBass) { p.set (K::m1, r.range (0.0f, 0.4f)); return "sub"; }
            p.set (K::lfoFilter, r.range (0.1f, 0.35f)).set (K::lfoRate, r.logRange (0.15f, 4.0f));
            if (r.chance (0.4f)) p.set (K::lfoPitch, r.range (0.02f, 0.06f));
            if (r.chance (0.4f)) p.set (K::alive, (float) r.between (1, 3));
            return "motion";
        }
        case 9:   // timbre
        {
            p.set (K::wave, juce::jlimit (0.0f, 1.0f, p[K::wave] + (r.chance (0.5f) ? 1.0f : -1.0f) * r.range (0.15f, 0.4f)));
            if (p.eng() == engFM || p.eng() == engModal) p.set (K::fmAmt, juce::jlimit (0.05f, 0.85f, p[K::fmAmt] * r.range (0.5f, 1.8f)));
            if (r.chance (0.35f)) p.set (K::reso, r.range (0.05f, 0.45f));
            return "timbre";
        }
        case 10:  // octave
        {
            if (grp == gBass) { p.set (K::m5, r.range (0.3f, 0.8f)); return "tone"; }
            const float o = p[K::octave];
            const float d = o >= 1 ? -1.0f : o <= -1 ? 1.0f : (r.chance (0.5f) ? 1.0f : -1.0f);
            p.set (K::octave, juce::jlimit (-1.0f, 2.0f, o + d));
            return d > 0 ? "octave up" : "octave down";
        }
        default:  // second oscillator layer
        {
            if (grp == gBass)
            {
                p.set (K::layerB, 1).set (K::engineB, engVA).set (K::waveB, r.uni()).set (K::octaveB, (float) r.between (0, 1)).set (K::levelB, r.range (0.06f, 0.2f));
                return "harmonics";
            }
            static const int engines[] { engVA, engWavetable, engFM, engVox, engFlute, engOrgan };
            p.set (K::layerB, 1).set (K::engineB, (float) r.pick (engines)).set (K::waveB, r.uni()).set (K::octaveB, (float) r.between (-1, 1))
             .set (K::levelB, r.range (0.2f, 0.45f)).set (K::unisonB, (float) r.between (1, 4)).set (K::detuneB, r.range (0.1f, 0.3f));
            return "layer B";
        }
    }
}
constexpr int numTreatments = 12;

//==============================================================================
// categories
const char* catSlug (int c)
{
    static const char* s[vault::numCategories] { "bass", "bell", "keys", "pluck", "lead", "pad", "str", "brass", "wind", "gtr", "vox", "synth",
                                                 "arp", "orch", "world", "fx" };
    return s[c];
}
int mainRoleOfCategory (int c)
{
    static const int r[vault::numCategories] { rBass, rBell, rKeys, rPluck, rLead, rPad, rStrings, rBrass, rWind, rGuitar, rChoir, rSynth,
                                               rArp, rStrings, rWorld, rTexture };
    return r[c];
}
struct LayerOpt { int role, effLo, effHi; float lvLo, lvHi, weight; };
const std::vector<LayerOpt>& layerOptions (int c)
{
    static const std::array<std::vector<LayerOpt>, vault::numCategories> t { {
        /* 808 */   { { rGrit, 0, 1, 0.25f, 0.5f, 3 }, { rKnock, 1, 2, 0.25f, 0.45f, 2 }, { rLead, 1, 2, 0.12f, 0.25f, 1 }, { rSynth, 1, 1, 0.15f, 0.3f, 1 },
                      { rBell, 2, 2, 0.12f, 0.25f, 0.5f } },
        /* bells */ { { rPad, -1, 0, 0.3f, 0.5f, 2 }, { rTexture, 0, 1, 0.2f, 0.4f, 1.5f }, { rPluck, 0, 0, 0.35f, 0.6f, 1.5f }, { rBell, 0, 1, 0.3f, 0.5f, 2 },
                      { rMallet, 0, 1, 0.3f, 0.55f, 1.5f }, { rKeys, 0, 0, 0.3f, 0.5f, 1 }, { rChoir, 0, 0, 0.25f, 0.4f, 1 }, { rBass, -2, -1, 0.35f, 0.55f, 1 },
                      { rStrings, -1, 0, 0.25f, 0.4f, 0.7f } },
        /* keys */  { { rPad, 0, 0, 0.3f, 0.45f, 2 }, { rStrings, 0, 0, 0.3f, 0.45f, 1.5f }, { rBell, 1, 1, 0.2f, 0.35f, 1.5f }, { rKeys, 0, 1, 0.35f, 0.5f, 1 },
                      { rChoir, 0, 0, 0.2f, 0.35f, 1 }, { rTexture, 0, 1, 0.15f, 0.3f, 1 }, { rBass, -2, -1, 0.3f, 0.5f, 0.8f }, { rMallet, 1, 1, 0.2f, 0.35f, 0.7f } },
        /* pluck */ { { rBass, -2, -1, 0.4f, 0.6f, 2.5f }, { rPad, 0, 0, 0.25f, 0.4f, 1.5f }, { rBell, 1, 1, 0.25f, 0.4f, 1.5f }, { rPluck, 0, 1, 0.35f, 0.5f, 1.5f },
                      { rKeys, 0, 0, 0.3f, 0.45f, 1 }, { rTexture, 1, 1, 0.15f, 0.3f, 1 }, { rMallet, 0, 1, 0.3f, 0.45f, 1 }, { rGuitar, 0, 0, 0.3f, 0.45f, 0.6f } },
        /* lead */  { { rLead, 1, 1, 0.35f, 0.55f, 2.5f }, { rLead, -1, -1, 0.35f, 0.5f, 1.5f }, { rBass, -2, -1, 0.35f, 0.5f, 1.2f }, { rPad, 0, 0, 0.2f, 0.35f, 1 },
                      { rBell, 1, 1, 0.2f, 0.35f, 1 }, { rBrass, 0, 0, 0.3f, 0.45f, 0.8f }, { rChoir, 0, 0, 0.2f, 0.35f, 0.8f }, { rSynth, 0, 0, 0.3f, 0.45f, 1 },
                      { rWind, 1, 1, 0.25f, 0.4f, 0.7f } },
        /* pad */   { { rPad, 1, 1, 0.35f, 0.5f, 2 }, { rPad, -1, -1, 0.35f, 0.5f, 1.5f }, { rChoir, 0, 0, 0.3f, 0.5f, 2 }, { rStrings, 0, 0, 0.3f, 0.45f, 1.5f },
                      { rTexture, 0, 1, 0.25f, 0.4f, 2 }, { rBell, 1, 1, 0.15f, 0.3f, 1 }, { rSynth, 0, 0, 0.25f, 0.4f, 0.7f }, { rWind, 1, 1, 0.2f, 0.35f, 0.6f } },
        /* str */   { { rStrings, 1, 1, 0.35f, 0.5f, 1.5f }, { rStrings, -1, -1, 0.35f, 0.5f, 1.5f }, { rBrass, 0, 0, 0.3f, 0.45f, 1.5f }, { rChoir, 0, 0, 0.3f, 0.45f, 1.5f },
                      { rPad, 0, 0, 0.25f, 0.4f, 1.2f }, { rPluck, 0, 0, 0.3f, 0.45f, 0.8f }, { rWind, 1, 1, 0.25f, 0.4f, 0.8f }, { rBass, -2, -1, 0.3f, 0.45f, 0.7f },
                      { rBell, 1, 1, 0.2f, 0.3f, 0.6f } },
        /* brass */ { { rBrass, -1, 1, 0.35f, 0.5f, 2 }, { rStrings, 0, 0, 0.3f, 0.45f, 1.5f }, { rLead, 0, 0, 0.25f, 0.4f, 1 }, { rChoir, 0, 0, 0.25f, 0.4f, 0.8f },
                      { rBass, -2, -1, 0.3f, 0.45f, 0.8f }, { rSynth, 0, 0, 0.25f, 0.4f, 0.8f }, { rPad, 0, 0, 0.2f, 0.35f, 0.6f } },
        /* wind */  { { rPad, 0, 0, 0.3f, 0.45f, 1.5f }, { rStrings, 0, 0, 0.3f, 0.45f, 1.2f }, { rBell, 1, 1, 0.2f, 0.35f, 1 }, { rWind, -1, 1, 0.35f, 0.5f, 1.5f },
                      { rChoir, 0, 0, 0.25f, 0.4f, 1 }, { rTexture, 0, 1, 0.2f, 0.35f, 1 }, { rPluck, 0, 0, 0.25f, 0.4f, 0.7f } },
        /* gtr */   { { rGuitar, -1, 1, 0.35f, 0.5f, 1.5f }, { rPad, 0, 0, 0.25f, 0.4f, 1.5f }, { rStrings, 0, 0, 0.25f, 0.4f, 1 }, { rBell, 1, 1, 0.2f, 0.35f, 1 },
                      { rPluck, 0, 1, 0.3f, 0.45f, 1 }, { rKeys, 0, 0, 0.25f, 0.4f, 0.8f }, { rBass, -2, -1, 0.3f, 0.45f, 0.8f }, { rTexture, 0, 1, 0.2f, 0.3f, 0.6f } },
        /* vox */   { { rPad, 0, 0, 0.3f, 0.45f, 2 }, { rChoir, -1, 1, 0.35f, 0.5f, 1.5f }, { rStrings, 0, 0, 0.3f, 0.45f, 1.2f }, { rBell, 1, 1, 0.2f, 0.35f, 0.8f },
                      { rTexture, 0, 1, 0.25f, 0.4f, 1.2f }, { rWind, 1, 1, 0.2f, 0.35f, 0.5f }, { rKeys, 0, 0, 0.25f, 0.4f, 0.5f } },
        /* synth */ { { rSynth, -1, 1, 0.35f, 0.5f, 1.5f }, { rPad, 0, 0, 0.25f, 0.4f, 1.2f }, { rLead, 0, 1, 0.3f, 0.45f, 1 }, { rPluck, 0, 1, 0.3f, 0.45f, 1 },
                      { rBass, -2, -1, 0.35f, 0.5f, 1 }, { rBell, 1, 1, 0.2f, 0.35f, 0.8f }, { rKeys, 0, 0, 0.25f, 0.4f, 0.6f } },
        /* arp */   { { rPad, 0, 0, 0.25f, 0.4f, 2 }, { rBass, -2, -1, 0.35f, 0.5f, 1.5f }, { rArp, 0, 1, 0.35f, 0.5f, 1.5f }, { rBell, 1, 1, 0.2f, 0.35f, 1 },
                      { rPluck, 0, 1, 0.3f, 0.45f, 1 }, { rTexture, 0, 1, 0.15f, 0.3f, 0.8f } },
        /* orch */  { { rStrings, -1, 1, 0.4f, 0.6f, 3 }, { rBrass, -1, 0, 0.35f, 0.5f, 2.5f }, { rWind, 0, 1, 0.3f, 0.45f, 1.5f }, { rChoir, 0, 0, 0.3f, 0.45f, 1.5f },
                      { rCine, -1, 0, 0.25f, 0.4f, 0.8f }, { rBell, 1, 1, 0.2f, 0.3f, 0.7f }, { rBass, -2, -1, 0.3f, 0.45f, 0.6f } },
        /* world */ { { rPad, -1, 0, 0.25f, 0.4f, 1.5f }, { rWorld, -1, 1, 0.35f, 0.5f, 2 }, { rWind, 0, 1, 0.3f, 0.45f, 1.2f }, { rPluck, 0, 1, 0.3f, 0.45f, 1 },
                      { rBell, 1, 1, 0.2f, 0.35f, 0.8f }, { rTexture, 0, 1, 0.2f, 0.35f, 0.8f }, { rMallet, 0, 1, 0.3f, 0.45f, 0.8f }, { rBass, -2, -1, 0.3f, 0.45f, 0.5f } },
        /* fx */    { { rPad, -1, 1, 0.3f, 0.5f, 2 }, { rTexture, -1, 1, 0.3f, 0.5f, 2 }, { rFx, 0, 1, 0.25f, 0.4f, 1.2f }, { rChoir, 0, 0, 0.25f, 0.4f, 1 },
                      { rBell, 1, 1, 0.15f, 0.3f, 1 }, { rCine, -1, 0, 0.25f, 0.4f, 0.7f }, { rWind, 1, 1, 0.2f, 0.35f, 0.5f } } } };
    return t[(size_t) c];
}
// relative weights of 1..4 layers per category (fitted to the global layer mix below)
const std::array<float, 4>& layerWeights (int c)
{
    static const std::array<std::array<float, 4>, vault::numCategories> w { {
        { 0.50f, 0.36f, 0.12f, 0.02f },   // 808 & Bass: mostly single 808s, some with grit / knock on top
        { 0.30f, 0.38f, 0.22f, 0.10f }, { 0.34f, 0.38f, 0.20f, 0.08f }, { 0.32f, 0.40f, 0.20f, 0.08f }, { 0.32f, 0.40f, 0.20f, 0.08f },
        { 0.26f, 0.38f, 0.25f, 0.11f },   // pads
        { 0.30f, 0.38f, 0.22f, 0.10f }, { 0.32f, 0.38f, 0.22f, 0.08f }, { 0.36f, 0.38f, 0.20f, 0.06f }, { 0.36f, 0.38f, 0.20f, 0.06f },
        { 0.28f, 0.38f, 0.24f, 0.10f }, { 0.30f, 0.38f, 0.22f, 0.10f }, { 0.34f, 0.38f, 0.20f, 0.08f },
        { 0.16f, 0.36f, 0.32f, 0.16f },   // orchestral: ensembles
        { 0.34f, 0.38f, 0.20f, 0.08f }, { 0.28f, 0.38f, 0.24f, 0.10f } } };
    return w[(size_t) c];
}

// integer matrix [category][layers] with exact row sums (category targets) and column sums (layer targets)
std::array<std::array<int, 4>, vault::numCategories> layerPlan()
{
    double m[vault::numCategories][4];
    for (int c = 0; c < vault::numCategories; ++c) for (int l = 0; l < 4; ++l) m[c][l] = layerWeights (c)[(size_t) l];
    for (int it = 0; it < 200; ++it)   // iterative proportional fitting
    {
        for (int c = 0; c < vault::numCategories; ++c)
        {
            double s = 0; for (int l = 0; l < 4; ++l) s += m[c][l];
            for (int l = 0; l < 4; ++l) m[c][l] *= v1c::target (c) / s;
        }
        for (int l = 0; l < 4; ++l)
        {
            double s = 0; for (int c = 0; c < vault::numCategories; ++c) s += m[c][l];
            for (int c = 0; c < vault::numCategories; ++c) m[c][l] *= v1c::layerTarget (l + 1) / s;
        }
    }
    std::array<std::array<int, 4>, vault::numCategories> out {};
    int rowNeed[vault::numCategories], colNeed[4];
    for (int c = 0; c < vault::numCategories; ++c) rowNeed[c] = v1c::target (c);
    for (int l = 0; l < 4; ++l) colNeed[l] = v1c::layerTarget (l + 1);
    std::vector<std::tuple<double, int, int>> fr;
    for (int c = 0; c < vault::numCategories; ++c)
        for (int l = 0; l < 4; ++l)
        {
            out[(size_t) c][(size_t) l] = (int) std::floor (m[c][l]);
            rowNeed[c] -= out[(size_t) c][(size_t) l]; colNeed[l] -= out[(size_t) c][(size_t) l];
            fr.push_back ({ m[c][l] - std::floor (m[c][l]), c, l });
        }
    std::sort (fr.begin(), fr.end(), [] (auto& a, auto& b) { return std::get<0> (a) != std::get<0> (b) ? std::get<0> (a) > std::get<0> (b)
                                                                   : std::get<1> (a) * 4 + std::get<2> (a) < std::get<1> (b) * 4 + std::get<2> (b); });
    for (auto& [f, c, l] : fr) if (rowNeed[c] > 0 && colNeed[l] > 0) { ++out[(size_t) c][(size_t) l]; --rowNeed[c]; --colNeed[l]; }
    for (int c = 0; c < vault::numCategories; ++c)
        for (int l = 0; l < 4; ++l)
            while (rowNeed[c] > 0 && colNeed[l] > 0) { ++out[(size_t) c][(size_t) l]; --rowNeed[c]; --colNeed[l]; }
    return out;
}

//==============================================================================
// the source library
int factoryRole (const Preset& p, int& home)
{
    const auto n = p.name.toLowerCase();
    switch (p.cat)
    {
        case cPiano: case cKeys: case cOrgan: home = v1c::catPianoKeys;   return rKeys;
        case cBells:                          home = v1c::catBellsMallets; return rBell;
        case cMallets:                        home = v1c::catBellsMallets; return rMallet;
        case cPlucks:                         home = v1c::catPlucks;       return rPluck;
        case cGuitar:                         home = v1c::catGuitars;      return rGuitar;
        case cStrings:                        home = v1c::catStrings;      return rStrings;
        case cBrass:                          home = v1c::catBrassHorns;   return rBrass;
        case cChoir:                          home = v1c::catVocalsChoir;  return rChoir;
        case cWoodwind:                       home = v1c::catFlutesWinds;  return rWind;
        case cLead:                           home = v1c::catLeads;        return rLead;
        case cPads:                           home = v1c::catPads;         return rPad;
        case cSynth:                          home = v1c::catSynth;        return rSynth;
        case cChip:   if (p.sub == "Bass") { home = v1c::cat808Bass; return rBass; }
                      home = v1c::catSynth; return rSynth;
        case cBass: case c808:                home = v1c::cat808Bass;      return rBass;
        case cArp:                            home = v1c::catArpsSequences; return rArp;
        case cWorld:                          home = v1c::catWorld;        return rWorld;
        case cCinematic:
            if (p.sub == "Braam" || p.sub == "Impact") { home = v1c::catOrchestral; return rCine; }
            home = v1c::catFxAtmos; return n.contains ("drone") ? rTexture : rFx;
        case cTexture:                        home = v1c::catFxAtmos;      return rTexture;
        case cFX: case cGameFx:               home = v1c::catFxAtmos;      return rFx;
        default:                              home = -1;                     return -1;
    }
}
juce::String factoryKind (const Preset& p, int role)
{
    const auto n = p.name.toLowerCase();
    if (p.cat == cOrgan) return "organ";
    if (role == rKeys) return n.contains ("piano") || p.cat == cPiano ? "piano" : "ep";
    if (role == rMallet) return n.contains ("marimba") ? "marimba" : n.contains ("music box") ? "musicbox" : "mallet";
    if (role == rGuitar) return n.contains ("nylon") ? "nylon" : n.contains ("electric") || n.contains ("power") || n.contains ("distort") ? "electric" : "guitar";
    if (role == rBass) return n.contains ("reese") ? "reese" : "bass";
    if (role == rStrings) return p.sub == "Pizzicato" ? "pizz" : p.sub == "Staccato" ? "staccato" : "strings";
    if (role == rBrass) return p.sub == "Hits" || n.contains ("stab") ? "stab" : "brass";
    if (role == rWind) return n.contains ("whistle") ? "whistle" : n.contains ("clarinet") || n.contains ("reed") ? "reed" : "flute";
    if (role == rLead) return n.contains ("whistle") ? "whistle" : "lead";
    if (role == rCine) return p.sub == "Impact" ? "hit" : "braam";
    if (role == rFx) return p.sub == "Riser" ? "riser" : "sweep";
    if (role == rWorld)
    {
        if (n.contains ("drone")) return "drone";
        if (n.contains ("pipe") || n.contains ("flute") || n.contains ("ocarina") || n.contains ("breeze")) return "pipe";
        for (auto& [id, v] : p.values)
            if (id == ID::engine) return v == engModal ? "thumb" : v == engFlute ? "pipe" : v == engOrgan ? "drone" : v == engPluck ? "zither" : "reed";
        return "reed";
    }
    return roleName (role);
}

// render one part alone and set its `gain` so it plays at the reference loudness (layers then balance by `level`)
constexpr float refLoud = -15.0f;
float calibrate (Src& s)
{
    VaultPreset p;
    p.id = "cal"; p.name = "cal"; p.category = 0;
    VaultPart part; part.values = s.patch.values(); part.label = "x";
    p.parts.push_back (part);
    const int note = s.role == rBass ? 36 : 60;
    double off = 0;
    const auto b = vaultqc::renderNote (p, note, off);
    const auto a = vaultqc::analyse (b, vaultqc::sampleRate, off);
    if (! a.finite || a.loud < -70.0f) { s.ok = false; return a.loud; }
    s.patch.set (K::gain, juce::jlimit (-24.0f, 12.0f, s.patch[K::gain] + (refLoud - a.loud)));
    return a.loud;
}

template <typename Fn>
void parallelFor (int n, int threads, Fn&& fn)
{
    std::atomic<int> next { 0 };
    std::vector<std::thread> pool;
    for (int t = 0; t < std::max (1, threads); ++t)
        pool.emplace_back ([&] { for (int i = next++; i < n; i = next++) fn (i); });
    for (auto& th : pool) th.join();
}

struct Library
{
    std::vector<Src> all;
    std::vector<int> byRole[numRoles];       // layer pools
    std::vector<int> mainOf[vault::numCategories];   // main pools (index repeated by weight)
    std::vector<int> order[vault::numCategories];    // main pools, shuffled per category (even use of every source)
};

Library buildLibrary (int threads)
{
    Library lib;
    // factory sounds
    for (const auto& pr : factoryPresets())
    {
        int home = -1;
        const int role = factoryRole (pr, home);
        if (role < 0) continue;
        Src s;
        s.name = pr.name; s.role = role; s.home = home; s.kkCat = pr.cat; s.kind = factoryKind (pr, role);
        s.patch = Patch::fromValues (pr.values);
        if (role == rBass) s.patch.set (K::mono, 1).set (K::bassMode, 1).set (K::octave, 0);
        if (role == rArp) { Rng r (seedOf (11, (uint64_t) lib.all.size())); makeSequence (s.patch, r); }
        s.weight = 2;
        lib.all.push_back (std::move (s));
    }
    // procedural recipes: how many per role (main pools of thin categories get more)
    const std::pair<int, int> perRole[] {
        { rBass, 150 }, { rBell, 70 }, { rMallet, 40 }, { rKeys, 60 }, { rPluck, 90 }, { rLead, 80 }, { rPad, 80 }, { rStrings, 60 }, { rBrass, 50 },
        { rWind, 50 }, { rGuitar, 50 }, { rChoir, 50 }, { rSynth, 70 }, { rArp, 50 }, { rTexture, 50 }, { rFx, 40 }, { rWorld, 70 }, { rCine, 35 },
        { rGrit, 30 }, { rKnock, 24 } };
    for (auto [role, count] : perRole)
        for (int i = 0; i < count; ++i)
        {
            Rng r (seedOf (100 + (uint64_t) role, (uint64_t) i));
            Src s;
            s.role = role;
            if (role == rBass && i >= 110) s.patch = recipeSynthBass (r, s.kind);
            else s.patch = recipeFor (role) (r, s.kind);
            if (role == rArp) makeSequence (s.patch, r);
            s.name = juce::String ("recipe.") + roleName (role) + "." + juce::String (i);
            static const int homes[numRoles] { v1c::cat808Bass, v1c::catBellsMallets, v1c::catBellsMallets, v1c::catPianoKeys, v1c::catPlucks,
                                               v1c::catLeads, v1c::catPads, v1c::catStrings, v1c::catBrassHorns, v1c::catFlutesWinds,
                                               v1c::catGuitars, v1c::catVocalsChoir, v1c::catSynth, v1c::catArpsSequences, v1c::catFxAtmos,
                                               v1c::catFxAtmos, v1c::catWorld, v1c::catOrchestral, -1, -1 };
            s.home = homes[role];
            s.weight = role == rBass && i < 110 ? 3 : 1;   // 808 & Bass is mostly 808s
            lib.all.push_back (std::move (s));
        }
    // calibrate every part (parallel; each call is independent)
    parallelFor ((int) lib.all.size(), threads, [&] (int i) { lib.all[(size_t) i].calLoud = calibrate (lib.all[(size_t) i]); });
    for (int i = 0; i < (int) lib.all.size(); ++i)
    {
        const auto& s = lib.all[(size_t) i];
        if (! s.ok) continue;
        lib.byRole[s.role].push_back (i);
        if (s.home >= 0) for (int w = 0; w < s.weight; ++w) lib.mainOf[s.home].push_back (i);
        // orchestral ensembles also lead with strings / brass / choir sections
        if (s.role == rStrings || s.role == rBrass) lib.mainOf[v1c::catOrchestral].push_back (i);
    }
    for (int c = 0; c < vault::numCategories; ++c)
    {
        auto& o = lib.order[c];
        o = lib.mainOf[c];
        Rng r (seedOf (900, (uint64_t) c));
        for (int i = (int) o.size() - 1; i > 0; --i) std::swap (o[(size_t) i], o[(size_t) r.below (i + 1)]);
    }
    return lib;
}

//==============================================================================
// candidate
struct Candidate
{
    VaultPreset preset;
    int category = 0, layers = 1, slot = 0, attempt = 0;   // category: OLD (v1) index; preset.category is the v2 one
    uint32_t styles = 0, chars = 0;                         // internal tags (names only)
    std::vector<int> roles, srcs;
    juce::StringArray notes;          // treatments (report)
    // results
    bool ok = false;
    juce::String reason;
    int repairs = 0;
    float loud = 0, loudLow = 0, loudHigh = 0, peak = 0, tail = 0;
    vaultqc::Analysis aLow, aHigh;
    vaultaudit::Result audit;         // the audit's spec render at the final gain (real tail, low band, own fingerprint)
};

int effOctave (const Patch& p, const VaultPart& part) { return (int) p[K::octave] + part.octave; }

Candidate buildCandidate (const Library& lib, int cat, int layers, int slot, int attempt)
{
    Candidate cd;
    cd.category = cat; cd.layers = layers; cd.slot = slot; cd.attempt = attempt;
    Rng r (seedOf ((uint64_t) cat + 1, (uint64_t) layers, (uint64_t) slot, (uint64_t) attempt));
    // spread usage evenly: walk the category-shuffled order of the main pool
    const auto& order = lib.order[cat];
    const int mainIdx = order[(size_t) ((slot + attempt * 37) % (int) order.size())];
    const Src& main = lib.all[(size_t) mainIdx];
    Patch p0 = main.patch;
    const int mainRole = main.role;
    cd.roles.push_back (mainRole); cd.srcs.push_back (mainIdx);

    // variants: 2..4 treatments on a single sound, 1..3 under layers
    const int nT = layers == 1 ? r.between (2, 4) : r.between (1, 3);
    std::set<int> used;
    for (int t = 0; t < nT; ++t)
    {
        int w = r.below (numTreatments);
        for (int k = 0; k < 4 && used.count (w); ++k) w = r.below (numTreatments);
        used.insert (w);
        if (cat == v1c::cat808Bass && w == 10 && mainRole != rBass) continue;
        cd.notes.add (applyTreatment (p0, r, w, mainRole));
    }
    // main octave limits
    if (mainRole == rBass) p0.set (K::octave, 0);
    else p0.set (K::octave, juce::jlimit (-1.0f, 2.0f, p0[K::octave]));

    VaultPart part0; part0.label = {};
    std::vector<Patch> patches { p0 };
    std::vector<VaultPart> parts { part0 };

    bool haveBass = mainRole == rBass;
    std::set<int> srcUsed { mainIdx };
    const auto& opts = layerOptions (cat);
    for (int k = 1; k < layers; ++k)
    {
        // weighted pick of a layer role (one bass-ish part per preset)
        float tot = 0;
        for (auto& o : opts) if (! (o.role == rBass && haveBass)) tot += o.weight;
        float x = r.uni() * tot;
        const LayerOpt* opt = &opts.front();
        for (auto& o : opts) { if (o.role == rBass && haveBass) continue; if ((x -= o.weight) <= 0) { opt = &o; break; } }
        if (opt->role == rBass && haveBass) opt = &opts.back();
        const auto& rp = lib.byRole[opt->role];
        if (rp.empty()) continue;
        int si = rp[(size_t) r.below ((int) rp.size())];
        for (int tries = 0; tries < 6 && srcUsed.count (si); ++tries) si = rp[(size_t) r.below ((int) rp.size())];
        srcUsed.insert (si);
        const Src& s = lib.all[(size_t) si];
        Patch pk = s.patch;
        if (r.chance (0.5f))   // a light touch on the layer too
        {
            const int w = r.chance (0.5f) ? 0 : 9;
            applyTreatment (pk, r, w, s.role);
        }
        for (int fxk : fxOnlyKeys()) pk.reset (fxk);
        pk.reset (K::era); pk.reset (K::eraHome);
        VaultPart part;
        const int eff = r.between (opt->effLo, opt->effHi);
        part.octave = juce::jlimit (-3, 3, eff - (int) pk[K::octave] + (mainRole == rBass ? 0 : 0));
        part.level = r.range (opt->lvLo, opt->lvHi);
        part.pan = opt->role == rBass || opt->role == rGrit || opt->role == rKnock ? 0.0f : (k % 2 == 1 ? -1.0f : 1.0f) * r.range (0.15f, 0.5f);
        if (opt->role == rBass) { haveBass = true; pk.set (K::mono, 1).set (K::bassMode, 1); }
        patches.push_back (pk); parts.push_back (part);
        cd.roles.push_back (opt->role); cd.srcs.push_back (si);
    }

    // FX of the layers live in part 0: lift the space a little when airy layers join a dry lead sound
    if (mainRole != rBass)
        for (size_t k = 1; k < cd.roles.size(); ++k)
        {
            const int role = cd.roles[k];
            const auto& sp = lib.all[(size_t) cd.srcs[k]].patch;
            if (role == rPad || role == rChoir || role == rTexture || role == rStrings)
            {
                patches[0].set (K::revMix, std::max (patches[0][K::revMix], 0.7f * sp[K::revMix]));
                patches[0].set (K::width, std::max (patches[0][K::width], 0.65f));
            }
        }

    // assemble
    VaultPreset& vp = cd.preset;
    vp.category = v1c::toV2 (cat);
    vp.id = juce::String ("cand.") + catSlug (cat) + "." + juce::String (slot) + "." + juce::String (attempt);
    vp.name = vp.id;
    for (size_t k = 0; k < patches.size(); ++k)
    {
        parts[k].values = patches[k].values();
        vp.parts.push_back (parts[k]);
    }
    vp.gain = 0.25f;
    cd.layers = (int) vp.parts.size();
    return cd;
}

//==============================================================================
// evaluation with repairs
void forAllParts (VaultPreset& p, const std::function<void (Patch&, int)>& fn)
{
    for (size_t k = 0; k < p.parts.size(); ++k)
    {
        Patch pt = Patch::fromValues (p.parts[k].values);
        fn (pt, (int) k);
        if (k > 0) for (int fxk : fxOnlyKeys()) pt.reset (fxk);
        p.parts[k].values = pt.values();
    }
}

bool repair (Candidate& cd, const juce::String& why)
{
    auto& p = cd.preset;
    if (why == "click-start")
        forAllParts (p, [] (Patch& pt, int) { pt.set (K::attack, std::max (pt[K::attack], 0.006f)); pt.set (K::fattack, std::max (pt[K::fattack], 0.003f));
                                               if (pt[K::bassMode] > 0.5f) pt.set (K::m6, std::min (pt[K::m6], 0.2f)); pt.set (K::punch, 0.0f); });
    else if (why == "click-end")
        forAllParts (p, [] (Patch& pt, int) { pt.set (K::release, std::max (pt[K::release], 0.08f)); pt.set (K::frelease, std::max (pt[K::frelease], 0.05f)); });
    else if (why == "long-tail")
    {
        // progressively shorter: 1st time trims the release / reverb / delay feedback, 2nd time also caps the reverb decay time
        // (cloud reverb RT60 = 5 + 15 x size s, hall 1 + 5 x size s), 3rd time swaps a cloud reverb for a hall
        int level = 0;
        for (auto& n : cd.notes) level += n == "repair long-tail";
        const bool padLike = cd.category == v1c::catPads || cd.category == v1c::catFxAtmos;
        forAllParts (p, [&] (Patch& pt, int k)
        {
            pt.set (K::release, std::min (pt[K::release], (padLike ? 3.0f : 1.6f) * (level >= 1 ? 0.7f : 1.0f)));
            if (k != 0) return;
            pt.set (K::revSize, std::min (pt[K::revSize], level == 0 ? 0.6f : 0.45f)); pt.mul (K::revMix, 0.8f);
            pt.set (K::delayFb, std::min (pt[K::delayFb], level == 0 ? 0.35f : 0.25f)); pt.set (K::freeze, 0); pt.set (K::timeM, 0.33f);
            if ((int) pt[K::revType] == 2) pt.set (K::revSize, std::min (pt[K::revSize], level == 0 ? 0.3f : 0.12f));   // cloud: RT60 <= 9.5 / 6.8 s
            if (level >= 2 && (int) pt[K::revType] == 2) { pt.set (K::revType, 0); pt.set (K::revSize, 0.8f); }
        });
    }
    else if (why == "mono-collapse" || why == "low-side")
    {
        forAllParts (p, [] (Patch& pt, int k) { if (k == 0) { pt.set (K::width, std::min (pt[K::width], 0.5f)); pt.mul (K::chorus, 0.5f); pt.set (K::phaser, 0); pt.set (K::flanger, 0); }
                                                pt.set (K::detune, std::min (pt[K::detune], 0.2f)); });
        for (auto& part : p.parts) part.pan *= 0.5f;
    }
    else if (why == "clip")
        forAllParts (p, [] (Patch& pt, int k) { if (k == 0) { pt.set (K::master, 1.0f); pt.set (K::punch, 0.0f); }
                                                pt.set (K::attack, std::max (pt[K::attack], 0.004f)); pt.mul (K::reso, 0.6f); });
    else if (why == "loudness")
        forAllParts (p, [] (Patch& pt, int k) { if (k == 0) pt.add (K::gain, 9.0f); });
    else return false;
    ++cd.repairs;
    cd.notes.add ("repair " + why);
    return true;
}

void evaluate (Candidate& cd)
{
    for (int round = 0; round < 5; ++round)
    {
        auto& p = cd.preset;
        p.gain = 0.25f;
        double offL = 0, offH = 0;
        auto bL = vaultqc::renderNote (p, vaultqc::noteLow (v1c::toV2 (cd.category)), offL);
        auto aL = vaultqc::analyse (bL, vaultqc::sampleRate, offL);
        if (aL.finite && aL.peak > 0.8f)   // very hot FX output: measure at a lower trim so the safety limiter stays out of it
        {
            p.gain = 0.25f * 0.6f / aL.peak;
            bL = vaultqc::renderNote (p, vaultqc::noteLow (v1c::toV2 (cd.category)), offL);
            aL = vaultqc::analyse (bL, vaultqc::sampleRate, offL);
        }
        const auto bH = vaultqc::renderNote (p, vaultqc::noteHigh (v1c::toV2 (cd.category)), offH);
        const auto aH = vaultqc::analyse (bH, vaultqc::sampleRate, offH);
        const float g0 = p.gain;
        if (! aL.finite || ! aH.finite) { cd.reason = "nan"; return; }
        const float target = vaultqc::targetLoudness (v1c::toV2 (cd.category));
        const float loud0 = 0.5f * (aL.loud + aH.loud);
        float g = g0 * juce::Decibels::decibelsToGain (target - loud0);
        juce::String why;
        if (aL.loud < -70.0f || aH.loud < -70.0f) why = "silent";
        else
        {
            const float pk = std::max (aL.peak, aH.peak);
            if (pk * g / g0 >= vaultqc::peakCeiling * 0.98f) g = vaultqc::peakCeiling * 0.97f * g0 / pk;   // allowed -1 dB under the target
            g = std::min (g, 16.0f);
            {   // the low test note is the note the audit (and most players) hit: pull it toward the target, keeping the mean of
                // both notes within 0.85 dB of the target and the peak under the ceiling
                const float meanDev = loud0 + juce::Decibels::gainToDecibels (g / g0) - target;
                const float lowDev = aL.loud + juce::Decibels::gainToDecibels (g / g0) - target;
                const float headroom = juce::Decibels::gainToDecibels (vaultqc::peakCeiling * 0.97f * g0 / (pk * g));
                const float d = std::min (headroom, std::max (-0.85f - meanDev, std::min (0.85f - meanDev, -lowDev)));
                g = std::min (16.0f, g * juce::Decibels::decibelsToGain (d));
            }
            const float scale = g / g0;
            why = vaultqc::gate (aL, v1c::toV2 (cd.category), scale, false);
            if (why.isEmpty()) why = vaultqc::gate (aH, v1c::toV2 (cd.category), scale, false);
            const float loud = loud0 + juce::Decibels::gainToDecibels (scale);
            if (why.isEmpty() && std::abs (loud - target) > vaultqc::loudnessTolerance - 0.1f)   // 0.1 dB margin for the shipped-bank test
                why = g >= 16.0f ? "loudness" : "clip";
            // the low note itself within 1.5 dB of the target (the audit flags 2 dB from the category median)
            if (why.isEmpty() && std::abs (aL.loud + juce::Decibels::gainToDecibels (scale) - target) > 1.5f) why = "loudness-spread";
            if (why.isEmpty())
            {   // the audit's spec render at the final gain: real tail, low band, clicks, DC, mono loss (with a little margin)
                p.gain = g;
                vaultaudit::Result ar;
                vaultaudit::Limits lim;
                lim.tail = 7.6f; lim.monoLoss = 2.7f; lim.lowSideTot -= 3.0f; lim.lowSideVsMid -= 3.0f; lim.spike = 6.0f;
                if (! vaultaudit::measureSpec (p, ar)) why = "nan";
                else
                {
                    vaultaudit::specFails (ar, lim);
                    if (! ar.fails.isEmpty())
                    {
                        const auto& f = ar.fails[0];
                        why = f == "tail>8s" ? "long-tail" : f == "mono-loss" || f == "low-side" ? "mono-collapse" : f == "limiter" || f == "peak>-0.5dB" ? "clip"
                            : f == "click-spike" ? "click-end" : f;
                    }
                }
                cd.audit = std::move (ar);
            }
            if (why.isEmpty())
            {
                p.gain = g;
                cd.ok = true;
                cd.loud = loud;
                cd.loudLow = aL.loud + juce::Decibels::gainToDecibels (scale);
                cd.loudHigh = aH.loud + juce::Decibels::gainToDecibels (scale);
                cd.peak = std::max (aL.peak, aH.peak) * scale;
                cd.tail = cd.audit.tail;
                cd.aLow = aL; cd.aHigh = aH;
                return;
            }
        }
        cd.reason = why;
        if (why == "silent" || why == "nan" || why == "dc" || round == 4 || ! repair (cd, why)) return;
    }
}

float printDist (const Candidate& a, const Candidate& b)
{
    return 0.5f * (vaultqc::printDistance (a.aLow.print, b.aLow.print) + vaultqc::printDistance (a.aHigh.print, b.aHigh.print));
}

//==============================================================================
// tags
struct TagInput { float centroid, width, attack, dirty, warmth, space, lofi, glide, chorus; int engine, mainRole; };

TagInput tagInput (const Candidate& c, const Library& lib)
{
    TagInput t {};
    const Patch p0 = Patch::fromValues (c.preset.parts[0].values);
    t.centroid = std::sqrt (c.aLow.centroidHz * c.aHigh.centroidHz);
    t.width = 0.5f * (c.aLow.sideToMidDb + c.aHigh.sideToMidDb);
    t.attack = c.aLow.attackMs;
    t.dirty = p0[K::drive] + p0[K::crush] + (p0[K::bassMode] > 0.5f && p0.eng() == engSub ? std::max (0.0f, p0[K::wave] - 0.35f) : 0.0f);
    // warmth: share of 150..900 Hz in the spectrum print (bands 4..9 of 20 log bands 50 Hz..16 kHz)
    float w = 0; for (int b = 4; b <= 9; ++b) w += c.aLow.print[(size_t) b]; t.warmth = w / 6.0f;
    t.space = p0[K::revMix] + 0.6f * p0[K::delayMix];
    t.lofi = p0[K::crush] + p0[K::wow] + (p0[K::tape] > 0.5f ? 0.3f : 0.0f) + ((int) p0[K::era] == 3 && (int) p0[K::eraHome] != 3 ? 0.3f : 0.0f);
    t.glide = p0[K::glide];
    t.chorus = p0[K::chorus] + p0[K::phaser];
    t.engine = p0.eng(); t.mainRole = c.roles.empty() ? 0 : c.roles[0];
    juce::ignoreUnused (lib);
    return t;
}

float percentile (std::vector<float> v, float x)
{
    if (v.empty()) return 0.5f;
    int below = 0; for (float y : v) below += y < x;
    return (float) below / (float) v.size();
}

void assignTags (std::vector<Candidate>& bank, const Library& lib)
{
    using namespace v1c;
    std::vector<TagInput> in;
    for (auto& c : bank) in.push_back (tagInput (c, lib));
    std::array<std::vector<float>, vault::numCategories> cen, war, wid;
    for (size_t i = 0; i < bank.size(); ++i)
    {
        const auto c = (size_t) bank[i].category;
        cen[c].push_back (in[i].centroid); war[c].push_back (in[i].warmth); wid[c].push_back (in[i].width);
    }
    for (auto* arr : { &cen, &war, &wid }) for (auto& v : *arr) std::sort (v.begin(), v.end());
    std::vector<float> trapScore (bank.size(), 0.0f);
    std::array<std::vector<float>, vault::numCategories> trapSorted;
    for (size_t i = 0; i < bank.size(); ++i)
    {
        auto& c = bank[i];
        const auto& t = in[i];
        const int cat = c.category;
        const float cp = percentile (cen[(size_t) cat], t.centroid), wp = percentile (war[(size_t) cat], t.warmth), dp = percentile (wid[(size_t) cat], t.width);
        std::vector<std::pair<float, int>> ch;
        if (cp >= 0.68f) ch.push_back ({ cp, charBright });
        if (cp <= 0.32f) ch.push_back ({ 1.0f - cp, charDark });
        if (wp >= 0.6f && cp < 0.75f && t.dirty < 0.35f) ch.push_back ({ 0.6f * wp, charWarm });
        if ((dp >= 0.55f && t.width >= -14.0f) || t.width >= -3.0f) ch.push_back ({ 0.5f + 0.4f * dp, charWide });
        if (t.attack <= 15.0f && (groupOf (t.mainRole) == gPerc || groupOf (t.mainRole) == gBass || t.dirty > 0.3f)) ch.push_back ({ 0.7f, charHard });
        if (t.attack >= 80.0f) ch.push_back ({ 0.65f + std::min (0.3f, t.attack / 3000.0f), charSoft });
        if (t.dirty >= 0.3f) ch.push_back ({ 0.6f + t.dirty * 0.3f, charDirty });
        else if (t.dirty < 0.08f && t.lofi < 0.15f) ch.push_back ({ 0.45f, charClean });
        std::sort (ch.begin(), ch.end(), [] (auto& a, auto& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
        uint32_t chars = 0; int n = 0;
        for (auto& [s, k] : ch)
        {
            if (n >= 3) break;
            if ((k == charBright && (chars & (1u << charDark))) || (k == charDark && (chars & (1u << charBright)))) continue;
            if ((k == charHard && (chars & (1u << charSoft))) || (k == charSoft && (chars & (1u << charHard)))) continue;
            if ((k == charDirty && (chars & (1u << charClean))) || (k == charClean && (chars & (1u << charDirty)))) continue;
            chars |= 1u << k; ++n;
        }
        if (chars == 0) chars = 1u << (t.dirty > 0.2f ? charDirty : charWarm);
        c.chars = chars;
        // how "trap" a sound is (hard / dirty / dark up, soft / roomy down), small deterministic jitter breaks ties
        Rng tr (seedOf (778, (uint64_t) i));
        trapScore[i] = ((chars & (1u << charHard)) ? 0.18f : 0.0f) + ((chars & (1u << charDirty)) ? 0.05f : 0.0f)
                     + ((chars & (1u << charDark)) ? 0.05f : 0.0f) - ((chars & (1u << charSoft)) ? 0.12f : 0.0f)
                     - (t.space > 0.42f ? 0.1f : 0.0f) + tr.range (0.0f, 0.04f);
        trapSorted[(size_t) cat].push_back (trapScore[i]);
    }
    for (auto& v : trapSorted) std::sort (v.begin(), v.end());

    for (size_t i = 0; i < bank.size(); ++i)
    {
        auto& c = bank[i];
        const auto& t = in[i];
        const int cat = c.category;
        const uint32_t chars = c.chars;
        // styles: scored from the category and the measured character
        const bool dark = chars & (1u << charDark), bright = chars & (1u << charBright), dirty = chars & (1u << charDirty),
                   wide = chars & (1u << charWide), soft = chars & (1u << charSoft), warm = chars & (1u << charWarm);
        auto catIs = [&] (std::initializer_list<int> cs) { for (int x : cs) if (x == cat) return true; return false; };
        // Trap is the house style but must stay a real filter: in the trap categories only the most trap-like share keeps it
        float st[v1c::numStyles] {};
        const float trapRaw = trapScore[i];
        if (catIs ({ cat808Bass, catBellsMallets, catBrassHorns, catFlutesWinds, catStrings, catPlucks, catOrchestral }))
            st[styleTrap] = percentile (trapSorted[(size_t) cat], trapRaw) >= (cat == cat808Bass ? 0.1f : 0.3f) ? 0.9f : 0.3f;
        else
            st[styleTrap] = 0.4f + trapRaw;
        st[styleRage] = (catIs ({ catLeads, catSynth, catArpsSequences, cat808Bass }) ? 0.3f : 0.0f) + (bright ? 0.25f : 0.0f) + (dirty ? 0.3f : 0.0f)
                      + (wide ? 0.15f : 0.0f) - (soft ? 0.4f : 0.0f);
        st[styleDrill] = (catIs ({ catStrings, catPianoKeys, catVocalsChoir, catBellsMallets, catOrchestral, catBrassHorns }) ? 0.3f : 0.0f)
                       + (dark ? 0.35f : 0.0f) + (cat == cat808Bass && t.glide > 0.08f ? 0.6f : 0.0f) - (bright ? 0.2f : 0.0f);
        st[stylePluggnb] = (catIs ({ catPlucks, catPianoKeys, catBellsMallets, catSynth, catPads, catGuitars }) ? 0.3f : 0.0f) + (soft || warm ? 0.25f : 0.0f)
                         + (t.chorus > 0.2f ? 0.2f : 0.0f) - (dirty ? 0.3f : 0.0f) + (cat == cat808Bass && ! dirty ? 0.2f : 0.0f);
        st[styleDark] = (dark ? 0.6f : 0.0f) + (catIs ({ catFxAtmos, catOrchestral }) ? 0.15f : 0.0f);
        st[styleEmo] = (catIs ({ catGuitars, catPianoKeys, catPads, catVocalsChoir }) ? 0.35f : 0.0f) + (warm || soft ? 0.2f : 0.0f) + (t.space > 0.35f ? 0.15f : 0.0f)
                     - (dirty ? 0.25f : 0.0f);
        st[styleLoFi] = (t.lofi > 0.15f ? 0.5f + t.lofi : 0.0f) + (warm && t.chorus > 0.25f ? 0.15f : 0.0f);
        st[styleAmbient] = (t.space > 0.42f ? 0.45f + t.space * 0.3f : 0.0f) + (catIs ({ catPads, catFxAtmos }) ? 0.3f : 0.0f) + (soft ? 0.15f : 0.0f);
        // small deterministic jitter so equal scores do not always resolve the same way
        Rng jr (seedOf (777, (uint64_t) i));
        for (auto& x : st) x += jr.range (0.0f, 0.06f);
        std::vector<std::pair<float, int>> sv;
        for (int s = 0; s < v1c::numStyles; ++s) sv.push_back ({ st[s], s });
        std::sort (sv.begin(), sv.end(), [] (auto& a, auto& b) { return a.first != b.first ? a.first > b.first : a.second < b.second; });
        uint32_t styles = 1u << sv[0].second;
        for (int k = 1; k < 3; ++k) if (sv[(size_t) k].first >= 0.55f) styles |= 1u << sv[(size_t) k].second;
        c.styles = styles;
    }
}

//==============================================================================
// names: invented, English, <= 22 characters, unique across the bank
const juce::StringArray& adjFor (int charIdx)
{
    using namespace v1c;
    static const std::array<juce::StringArray, numCharacters + 1> a { {
        { "Crystal", "Glassy", "Golden", "Solar", "Shiny", "Radiant", "Gleaming", "Brilliant", "Polished", "Sunlit", "Prism", "Silver", "Halo", "Lucid", "Dazzling", "Diamond" },
        { "Midnight", "Obsidian", "Shadow", "Grim", "Hollow", "Ashen", "Sunken", "Cursed", "Haunted", "Raven", "Onyx", "Moonless", "Somber", "Phantom", "Murky", "Eerie", "Gothic" },
        { "Velvet", "Amber", "Honey", "Dusty", "Mellow", "Cozy", "Sepia", "Copper", "Cinnamon", "Tender", "Faded", "Plush", "Autumn", "Smoky", "Caramel", "Rosy" },
        { "Vast", "Endless", "Open", "Panoramic", "Boundless", "Floating", "Orbital", "Stellar", "Drifting", "Cosmic", "Astral", "Wide", "Skyward", "Spacious" },
        { "Hard", "Sharp", "Brutal", "Savage", "Iron", "Steel", "Razor", "Punchy", "Heavy", "Feral", "Jagged", "Brash", "Fierce", "Rugged", "Blunt" },
        { "Soft", "Gentle", "Silken", "Feather", "Hushed", "Misty", "Dreamy", "Cloudy", "Mild", "Pillow", "Calm", "Quiet", "Sleepy", "Hazy", "Lull" },
        { "Gritty", "Rusty", "Toxic", "Blown", "Crushed", "Filthy", "Burnt", "Fried", "Smashed", "Scorched", "Ragged", "Grimy", "Molten", "Wrecked", "Busted" },
        { "Clean", "Pure", "Clear", "Fresh", "Pristine", "Crisp", "Neat", "Spotless", "Simple", "Bare", "Plain", "Sleek", "Vivid", "Precise" },
        { "Frozen", "Chrome", "Neon", "Lunar", "Electric", "Icy", "Frosted", "Wired", "Royal", "Sacred", "Lost", "Secret", "Hidden", "Distant", "Fallen",
          "Rising", "Burning", "Liquid", "Static", "Stormy", "Rainy", "Polar", "Arctic", "Desert", "Ocean", "Twilight", "Dawn", "Dusk", "Eternal", "Vintage",
          "Retro", "Future", "Hyper", "Ultra", "Lonely", "Bitter", "Sweet", "Wild", "Silent", "Broken", "Glitch", "Pixel", "Mirror", "Ghost", "Blue", "Violet",
          "Crimson", "Scarlet", "Cobalt", "Indigo", "Jade", "Pearl", "Opal", "Ivory", "Night", "Velour", "Saint", "Venom", "Holy", "Mystic", "Nova", "Zero" } } };
    return a[(size_t) juce::jlimit (0, (int) numCharacters, charIdx)];
}
const juce::StringArray& nouns()
{
    static const juce::StringArray n { "Halo", "Glide", "Vault", "Ember", "Echo", "Mirage", "Prism", "Frost", "Ash", "Smoke", "Dust", "Storm", "Rain", "Tide",
                                       "Orbit", "Comet", "Pulse", "Flare", "Spark", "Shard", "Veil", "Haze", "Mist", "Dream", "Memory", "Heart", "Soul", "Spirit",
                                       "Angel", "Crown", "Throne", "Empire", "Street", "Block", "City", "Avenue", "Alley", "Rooftop", "Skyline", "Harbor", "Garden",
                                       "Temple", "Chapel", "Tower", "Castle", "Cave", "Canyon", "Valley", "River", "Lake", "Island", "Moon", "Sun", "Star", "Planet",
                                       "Galaxy", "Nebula", "Void", "Abyss", "Horizon", "Zenith", "Eclipse", "Aurora", "Lotus", "Rose", "Thorn", "Petal", "Ivy",
                                       "Willow", "Wolf", "Viper", "Cobra", "Tiger", "Lion", "Falcon", "Phoenix", "Dragon", "Serpent", "Moth", "Ruby", "Sapphire",
                                       "Emerald", "Gold", "Iron", "Glass", "Stone", "Marble", "Candle", "Lantern", "Signal", "Circuit", "Engine", "Rocket", "Satellite",
                                       "Comet", "Meteor", "Cipher", "Code", "Oracle", "Prophecy", "Ritual", "Relic", "Legend", "Myth", "Riddle", "Mercy", "Fury",
                                       "Glory", "Honor", "Karma", "Bloom", "Feather", "Wing", "Siren", "Muse", "Saint", "Rebel", "Outlaw", "Hustle", "Drip", "Ice",
                                       "Velvet", "Chrome", "Neon", "Vapor", "Ozone", "Tundra", "Glacier", "Lagoon", "Mosaic", "Palace", "Arcade", "Bridge", "Station",
                                       "Window", "Mirror", "Shadow", "Ghost", "Phantom", "Pearl", "Opal", "Jade", "Onyx", "Amber", "Cloud", "Thunder", "Lightning" };
    return n;
}
juce::StringArray typeWords (const Candidate& c, const Library& lib)
{
    const auto& k = lib.all[(size_t) c.srcs[0]].kind;
    switch (c.category)
    {
        case v1c::cat808Bass:
            if (k == "808" || k == "hard808" || k == "slide") return k == "slide" ? juce::StringArray { "Slide", "Glide 808", "808", "Slide 808" }
                                                                                : juce::StringArray { "808", "Boom", "808", "Sub", "Low End", "Knock" };
            return k == "reese" ? juce::StringArray { "Reese", "Bass", "Growl" } : juce::StringArray { "Bass", "Sub", "Low", "Bassline", "Growl" };
        case v1c::catBellsMallets:
            if (k == "marimba") return { "Marimba", "Mallet", "Wood" };
            if (k == "musicbox") return { "Music Box", "Lullaby", "Box" };
            if (k == "thumb") return { "Thumb Keys", "Tines", "Mallet" };
            if (k == "mallet") return { "Mallet", "Vibes", "Celesta", "Chime" };
            return { "Bell", "Bells", "Chime", "Chimes", "Glockenspiel", "Toll" };
        case v1c::catPianoKeys:
            if (k == "organ") return { "Organ", "Chapel Keys", "Drawbar" };
            if (k == "ep") return { "EP", "Keys", "Tines", "E-Piano" };
            return { "Piano", "Keys", "Grand", "Upright", "Ivory" };
        case v1c::catPlucks: return { "Pluck", "Plucks", "Pick", "Ping", "Snap", "Flick" };
        case v1c::catLeads: return k == "whistle" ? juce::StringArray { "Whistle", "Lead", "Siren" } : juce::StringArray { "Lead", "Hook", "Line", "Solo", "Saw", "Topline" };
        case v1c::catPads: return { "Pad", "Haze", "Cloud", "Swell", "Wash", "Bed", "Drift" };
        case v1c::catStrings:
            if (k == "pizz") return { "Pizz", "Pizzicato", "Plucked Strings" };
            if (k == "staccato") return { "Staccato", "Spiccato", "Short Strings" };
            return { "Strings", "Violins", "Cellos", "Ensemble", "Bows", "Quartet" };
        case v1c::catBrassHorns: return k == "stab" ? juce::StringArray { "Stab", "Horn Stab", "Brass Hit" } : juce::StringArray { "Horn", "Horns", "Brass", "Trumpet", "Fanfare" };
        case v1c::catFlutesWinds:
            if (k == "reed") return { "Reed", "Clarinet", "Oboe" };
            if (k == "whistle") return { "Whistle", "Pipe" };
            return { "Flute", "Flutes", "Pipe", "Breath", "Piccolo" };
        case v1c::catGuitars:
            if (k == "nylon") return { "Nylon", "Guitar", "Strum" };
            if (k == "electric") return { "Guitar", "Riff", "Twang", "Crunch" };
            return { "Guitar", "Strum", "Pick", "Steel", "Acoustic" };
        case v1c::catVocalsChoir: return { "Choir", "Voices", "Aah", "Ooh", "Chant", "Hymn", "Vocal" };
        case v1c::catSynth: return { "Synth", "Poly", "Stack", "Wave", "Chord" };
        case v1c::catArpsSequences: return { "Arp", "Sequence", "Pattern", "Pulse", "Cycle", "Run" };
        case v1c::catOrchestral: return { "Ensemble", "Tutti", "Orchestra", "Score", "Overture", "Anthem" };
        case v1c::catWorld:
            if (k == "thumb") return { "Thumb Keys", "Tines" };
            if (k == "zither" || k == "lute") return { "Zither", "Lute", "Dulcimer", "Lyre" };
            if (k == "pipe") return { "Pipes", "Pan Flute", "Reed Pipe" };
            if (k == "drone") return { "Drone", "Temple Drone", "Hum" };
            if (k == "reed") return { "Reed", "Chanter", "Pipe Reed" };
            return { "Reed", "Zither", "Pipes", "Drone", "Lyre", "Gong" };
        default: return { "Atmos", "Drone", "Texture", "Field", "Rift", "Sweep", "Riser", "Static" };
    }
}

juce::String makeName (const Candidate& c, const Library& lib, std::set<juce::String>& used, uint64_t seed)
{
    Rng r (seed);
    const auto types = typeWords (c, lib);
    // characters of the sound pick the adjective family
    std::vector<int> fams;
    for (int k = 0; k < v1c::numCharacters; ++k) if (c.chars & (1u << k)) fams.push_back (k);
    for (int attempt = 0; attempt < 400; ++attempt)
    {
        const int fam = fams.empty() || r.chance (0.35f) ? v1c::numCharacters : fams[(size_t) r.below ((int) fams.size())];
        const auto& adjs = adjFor (fam);
        const juce::String adj = adjs[r.below (adjs.size())], noun = nouns()[r.below (nouns().size())], type = types[r.below (types.size())];
        const float form = r.uni();
        juce::String name = form < 0.22f ? adj + " " + type : form < 0.8f ? adj + " " + noun + " " + type : noun + " " + type;
        {   // no two words of the name may share a stem ("Cloudy ... Cloud")
            juce::StringArray words; words.addTokens (name, " ", {});
            bool clash = false;
            for (int a = 0; a < words.size() && ! clash; ++a)
                for (int b = a + 1; b < words.size() && ! clash; ++b)
                    clash = words[a].substring (0, 4).equalsIgnoreCase (words[b].substring (0, 4));
            if (clash) continue;
        }
        if (name.length() > 22 || used.count (name.toLowerCase()) || vaultaudit::nameProblem (name).isNotEmpty()) continue;
        used.insert (name.toLowerCase());
        return name;
    }
    jassertfalse;
    return {};
}

juce::String partLabel (const Patch& p, int role, const juce::String& kind, Rng& r)
{
    auto pick = [&] (juce::StringArray a) { return a[r.below (a.size())]; };
    const float cut = p[K::cutoff];
    const bool dark = cut < 1500.0f, bright = cut > 9000.0f;
    switch (role)
    {
        case rBass:    return kind == "808" || kind == "hard808" || kind == "slide" ? pick ({ "808", "Sub 808", "Boom" }) : kind == "reese" ? juce::String ("Reese") : pick ({ "Sub", "Bass", "Low Sub" });
        case rBell:    return bright ? pick ({ "Ice Bell", "Glass Bell" }) : dark ? juce::String ("Dark Bell") : pick ({ "Bell", "Chime" });
        case rMallet:  return kind == "marimba" ? juce::String ("Marimba") : kind == "musicbox" ? juce::String ("Music Box") : pick ({ "Mallet", "Vibes" });
        case rKeys:    return kind == "organ" ? juce::String ("Organ") : kind == "ep" ? juce::String ("E-Piano") : pick ({ "Piano", "Keys" });
        case rPluck:   return dark ? juce::String ("Soft Pluck") : pick ({ "Pluck", "Snap Pluck" });
        case rLead:    return kind == "whistle" ? juce::String ("Whistle") : pick ({ "Lead", "Saw Lead" });
        case rPad:     return dark ? juce::String ("Dark Pad") : bright ? juce::String ("Air Pad") : pick ({ "Pad", "Warm Pad" });
        case rStrings: return kind == "pizz" ? juce::String ("Pizz") : kind == "staccato" ? juce::String ("Staccato") : pick ({ "Strings", "Violins", "Cellos" });
        case rBrass:   return kind == "stab" ? juce::String ("Brass Stab") : pick ({ "Horns", "Brass" });
        case rWind:    return kind == "reed" ? juce::String ("Reed") : pick ({ "Flute", "Breath" });
        case rGuitar:  return kind == "nylon" ? juce::String ("Nylon") : kind == "electric" ? juce::String ("E-Guitar") : juce::String ("Guitar");
        case rChoir:   return pick ({ "Choir", "Voices", "Aah" });
        case rSynth:   return pick ({ "Synth", "Poly" });
        case rArp:     return pick ({ "Arp", "Sequence" });
        case rTexture: return pick ({ "Air", "Texture", "Dust" });
        case rFx:      return kind == "riser" ? juce::String ("Riser") : pick ({ "Sweep", "FX" });
        case rWorld:   return kind == "thumb" ? juce::String ("Thumb Keys") : kind == "drone" ? juce::String ("Drone") : kind == "pipe" ? juce::String ("Pipes")
                            : kind == "reed" ? juce::String ("Reed") : juce::String ("Zither");
        case rCine:    return kind == "hit" ? juce::String ("Impact") : juce::String ("Braam");
        case rGrit:    return pick ({ "Grit", "Buzz" });
        case rKnock:   return pick ({ "Knock", "Click" });
        default:       return "Layer";
    }
}

//==============================================================================
struct Stats
{
    std::map<juce::String, int> rejects;
    std::map<juce::String, int> repairsOk;
    int evaluated = 0, duplicates = 0;
};

juce::String fmt (float x, int d = 1) { return juce::String (x, d); }

//==============================================================================
// -audit: every preset through BankAudit.h + bank-wide checks (loudness outliers, CPU outliers, near-duplicates, names)
int runAudit (const juce::File& bankFile, const juce::File& csv, int threads, int every)
{
    using namespace vaultaudit;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    std::vector<VaultPreset> b;
    juce::String err;
    if (! vault::loadBankFile (bankFile, b, &err)) { std::printf ("cannot load: %s\n", err.toRawUTF8()); return 1; }
    if (every > 1) { std::vector<VaultPreset> s; for (size_t i = 0; i < b.size(); i += (size_t) every) s.push_back (b[i]); b.swap (s); }
    const int n = (int) b.size();
    std::vector<Result> res ((size_t) n);
    std::atomic<int> done { 0 };
    parallelFor (n, threads, [&] (int i)
    {
        res[(size_t) i] = analysePreset (b[(size_t) i]);
        if (const int d = ++done; d % 500 == 0) std::printf ("  %d / %d (%.0f s)\n", d, n, (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
    });

    auto median = [] (std::vector<float> v) { if (v.empty()) return 0.0f; std::sort (v.begin(), v.end()); return v[v.size() / 2]; };
    // loudness outliers per category (full-note loudness = how the sound plays; the fixed 1.5 s value is reported too)
    float catMed[vault::numCategories] {}, catMedSpec[vault::numCategories] {};
    for (int c = 0; c < vault::numCategories; ++c)
    {
        std::vector<float> l, ls;
        for (int i = 0; i < n; ++i) if (b[(size_t) i].category == c && res[(size_t) i].finite) { l.push_back (res[(size_t) i].loudFull); ls.push_back (res[(size_t) i].loud); }
        catMed[c] = median (l); catMedSpec[c] = median (ls);
    }
    int specLoudOutliers = 0;
    for (int i = 0; i < n; ++i)
    {
        auto& r = res[(size_t) i];
        if (! r.finite) continue;
        const int c = b[(size_t) i].category;
        if (std::abs (r.loudFull - catMed[c]) > loudnessOutlierDb) r.fails.add (juce::String::formatted ("loudness%+.1f", r.loudFull - catMed[c]));
        if (std::abs (r.loud - catMedSpec[c]) > loudnessOutlierDb) { ++specLoudOutliers; r.infos.add ("loudness-1.5s"); }
    }
    // CPU: against the median of the presets with the same number of layers, and against the global median
    float cpuMed[vault::maxParts + 1] {};
    std::vector<float> all;
    for (int k = 1; k <= vault::maxParts; ++k)
    {
        std::vector<float> v;
        for (int i = 0; i < n; ++i) if ((int) b[(size_t) i].parts.size() == k) v.push_back (res[(size_t) i].cpu);
        cpuMed[k] = median (v);
        all.insert (all.end(), v.begin(), v.end());
    }
    const float cpuMedAll = median (all);
    int cpuGlobalOutliers = 0;
    for (int i = 0; i < n; ++i)
    {
        auto& r = res[(size_t) i];
        const int k = (int) b[(size_t) i].parts.size();
        if (r.cpu > cpuOutlierFactor * cpuMed[k]) r.fails.add (juce::String::formatted ("cpu x%.1f", r.cpu / cpuMed[k]));
        if (r.cpu > cpuOutlierFactor * cpuMedAll) { ++cpuGlobalOutliers; r.infos.add ("cpu>3x-global-median"); }
    }
    // near-duplicates (all pairs, any category)
    std::vector<float> nn ((size_t) n, 1.0e9f);
    std::vector<int> nnIdx ((size_t) n, -1);
    parallelFor (n, threads, [&] (int i)
    {
        for (int j = 0; j < n; ++j)
            if (j != i) { const float d = distance (res[(size_t) i].print, res[(size_t) j].print); if (d < nn[(size_t) i]) { nn[(size_t) i] = d; nnIdx[(size_t) i] = j; } }
    });
    for (int i = 0; i < n; ++i)
        if (nn[(size_t) i] < dupDistance && i > nnIdx[(size_t) i])   // the later one of the pair is the duplicate
            res[(size_t) i].fails.add ("duplicate-of " + b[(size_t) nnIdx[(size_t) i]].id + juce::String::formatted (" (%.2f)", nn[(size_t) i]));
    // names
    std::map<juce::String, int> seen;
    for (int i = 0; i < n; ++i)
    {
        const auto& p = b[(size_t) i];
        auto& r = res[(size_t) i];
        if (const auto w = nameProblem (p.name); w.isNotEmpty()) r.fails.add ("name: " + w);
        if (const auto it = seen.find (p.name.toLowerCase()); it != seen.end()) r.fails.add ("name: duplicate of " + b[(size_t) it->second].id);
        seen[p.name.toLowerCase()] = i;
        for (auto& part : p.parts) if (const auto w = nameProblem (part.label); w.isNotEmpty()) r.fails.add ("label '" + part.label + "': " + w);
    }

    // report
    std::map<juce::String, int> failCount, infoCount;
    int failed = 0;
    for (int i = 0; i < n; ++i)
    {
        auto& r = res[(size_t) i];
        for (auto& f : r.fails)
            ++failCount[f.startsWith ("loudness") ? juce::String ("loudness") : f.startsWith ("name") ? juce::String ("name")
                        : f.startsWith ("label") ? juce::String ("label") : f.upToFirstOccurrenceOf (" ", false, false)];
        for (auto& f : r.infos) ++infoCount[f];
        if (! r.fails.isEmpty() && failed++ < 60)
            std::printf ("FAIL %s %-24s %s\n", b[(size_t) i].id.toRawUTF8(), b[(size_t) i].name.toRawUTF8(), r.fails.joinIntoString ("; ").toRawUTF8());
    }
    auto stat = [&] (const char* what, auto get)
    {
        std::vector<float> v; for (int i = 0; i < n; ++i) if (res[(size_t) i].finite) v.push_back (get (res[(size_t) i]));
        std::sort (v.begin(), v.end());
        if (v.empty()) return;
        std::printf ("  %-26s min %8.3f  p50 %8.3f  p99 %8.3f  max %8.3f\n", what, v.front(), v[v.size() / 2], v[v.size() * 99 / 100], v.back());
    };
    std::printf ("\nmetrics over %d presets:\n", n);
    stat ("peak (vel 0.8)", [] (const Result& r) { return r.peak; });
    stat ("peak (vel 1.0)", [] (const Result& r) { return r.peak127; });
    stat ("peak extremes (24/96)", [] (const Result& r) { return r.extremePeak; });
    stat ("DC / rms", [] (const Result& r) { return r.dcRel; });
    stat ("first sample", [] (const Result& r) { return r.firstSample; });
    stat ("onset jump / later", [] (const Result& r) { return r.onJump / std::max (1.0e-6f, r.laterJump); });
    stat ("note-off jump / before", [] (const Result& r) { return r.offJump / std::max (1.0e-6f, r.preOffJump); });
    stat ("spike ratio", [] (const Result& r) { return r.spike; });
    stat ("mono-sum loss dB", [] (const Result& r) { return r.monoLossDb; });
    stat ("side<120 vs total dB", [] (const Result& r) { return r.lowSideTotDb; });
    stat ("side<120 vs mid<120 dB", [] (const Result& r) { return r.lowSideMidDb; });
    stat ("tail s", [] (const Result& r) { return r.tail; });
    stat ("cpu (s per s audio)", [] (const Result& r) { return r.cpu; });
    std::printf ("  cpu medians: 1 layer %.4f, 2 layers %.4f, 3 layers %.4f, 4 layers %.4f, all %.4f; %d presets > 3x the global median (report only)\n",
                 cpuMed[1], cpuMed[2], cpuMed[3], cpuMed[4], cpuMedAll, cpuGlobalOutliers);
    {
        std::vector<float> s = nn; std::sort (s.begin(), s.end());
        std::printf ("  nearest-neighbour distance: min %.2f, p1 %.2f, p5 %.2f, median %.2f (duplicate below %.1f)\n", s.front(), s[s.size() / 100], s[s.size() / 20],
                     s[s.size() / 2], dupDistance);
    }
    std::printf ("  loudness: per-category medians (full note):");
    for (int c = 0; c < vault::numCategories; ++c) std::printf (" %.1f", catMed[c]);
    std::printf ("\n  fixed 1.5 s loudness outliers (report only, slow attacks): %d\n", specLoudOutliers);
    std::printf ("\nfailures by check:");
    for (auto& [k, v] : failCount) std::printf ("  %s %d", k.toRawUTF8(), v);
    std::printf ("\nreport-only:");
    for (auto& [k, v] : infoCount) std::printf ("  %s %d", k.toRawUTF8(), v);
    std::printf ("\naudited %d presets: %d failed (%.0f s)\n", n, failed, (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);

    if (csv != juce::File())
    {
        juce::String s ("idx\tid\tname\tcat\tlayers\tpeak\tpeak127\tloud\tloudFull\tdcRel\tfirst\tonJump\tlaterJump\toffJump\tpreOffJump\tspike\tmonoLoss\t"
                        "lowSideTot\tlowSideMid\tlowMidTot\ttail\tcpu\textLow\textHigh\textPeak\tnn\tnnId\tfails\tinfos\n");
        for (int i = 0; i < n; ++i)
        {
            const auto& r = res[(size_t) i]; const auto& p = b[(size_t) i];
            s << i << "\t" << p.id << "\t" << p.name << "\t" << p.category << "\t" << (int) p.parts.size();
            for (float x : { r.peak, r.peak127, r.loud, r.loudFull, r.dcRel, r.firstSample, r.onJump, r.laterJump, r.offJump, r.preOffJump, r.spike,
                             r.monoLossDb, r.lowSideTotDb, r.lowSideMidDb, r.lowMidTotDb, r.tail, r.cpu, r.extremeLowLoud, r.extremeHighLoud, r.extremePeak,
                             nn[(size_t) i] })
                s << "\t" << juce::String (x, 5);
            s << "\t" << (nnIdx[(size_t) i] >= 0 ? b[(size_t) nnIdx[(size_t) i]].id : juce::String()) << "\t" << r.fails.joinIntoString ("; ") << "\t"
              << r.infos.joinIntoString ("; ") << "\n";
        }
        csv.replaceWithText (s);
        std::printf ("csv -> %s\n", csv.getFullPathName().toRawUTF8());
    }
    return failed == 0 ? 0 : 1;
}
} // namespace

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::ScopedJuceInitialiser_GUI init;
    if (argc > 2 && juce::String (argv[1]) == "-recipe")   // v2 recipes (tools/vault/recipes/*.cpp) through the v2 gate (RecipeGate.h)
        return recipegate::runRecipes (argc, argv);
    juce::String outPath = "resources/vault/factory.cvb", reportPath = "docs/VAULT_BANK_REPORT.md";
    int threads = (int) std::max (1u, std::thread::hardware_concurrency());
    double scale = 1.0;
    float dupThreshold = 3.0f;
    for (int i = 1; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a == "-out" && i + 1 < argc) outPath = argv[++i];
        else if (a == "-report" && i + 1 < argc) reportPath = argv[++i];
        else if (a == "-threads" && i + 1 < argc) threads = juce::jmax (1, juce::String (argv[++i]).getIntValue());
        else if (a == "-scale" && i + 1 < argc) scale = juce::jlimit (0.001, 1.0, juce::String (argv[++i]).getDoubleValue());
        else if (a == "-dup" && i + 1 < argc) dupThreshold = (float) juce::String (argv[++i]).getDoubleValue();
    }
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto t0 = juce::Time::getMillisecondCounterHiRes();

    if (argc > 2 && juce::String (argv[1]) == "-list")   // -list <bank.cvb>: id, name, layers, tags
    {
        std::vector<VaultPreset> b;
        if (! vault::loadBankFile (cwd.getChildFile (argv[2]), b)) return 1;
        for (auto& p : b)
        {
            juce::StringArray lab;
            for (auto& pt : p.parts) lab.add (pt.label + (pt.octave != 0 ? juce::String::formatted (" (%+d)", pt.octave) : juce::String()));
            std::printf ("%s\t%s\t%s\t%s\n", p.id.toRawUTF8(), p.name.toRawUTF8(), lab.joinIntoString (" + ").toRawUTF8(),
                         vault::categoryNames()[juce::jlimit (0, (int) vault::numCategories - 1, p.category)].toRawUTF8());
        }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "-check")   // -check <bank.cvb> [first count]: re-render presets at their final gain through the gate
    {
        std::vector<VaultPreset> b;
        juce::String err;
        if (! vault::loadBankFile (cwd.getChildFile (argv[2]), b, &err)) { std::printf ("cannot load: %s\n", err.toRawUTF8()); return 1; }
        const int first = argc > 3 ? juce::String (argv[3]).getIntValue() : 0;
        const int count = argc > 4 ? juce::String (argv[4]).getIntValue() : (int) b.size();
        const int n = juce::jlimit (0, (int) b.size() - first, count);
        std::vector<juce::String> why ((size_t) n);
        parallelFor (n, threads, [&] (int k)
        {
            const auto& p = b[(size_t) (first + k)];
            const auto aL = vaultqc::renderAndAnalyse (p, vaultqc::noteLow (p.category));
            const auto aH = vaultqc::renderAndAnalyse (p, vaultqc::noteHigh (p.category));
            auto w = vaultqc::gate (aL, p.category, 1.0f, false);
            const auto& a = w.isEmpty() ? aH : aL;
            if (w.isEmpty()) w = vaultqc::gate (aH, p.category, 1.0f, false);
            const float loud = 0.5f * (aL.loud + aH.loud);
            if (w.isEmpty() && std::abs (loud - vaultqc::targetLoudness (p.category)) > vaultqc::loudnessTolerance) w = "loudness";
            if (w.isNotEmpty())
                w << juce::String::formatted (" (loud %.2f peak %.3f first %.4f on %.4f later %.4f off %.4f preOff %.4f mono %.1f lowSide %.1f tail %.1f)",
                                              loud, a.peak, a.firstSample, a.onsetJump, a.laterJump, a.offJump, a.preOffJump, a.monoLossDb, a.lowSideDb, a.tailSeconds);
            why[(size_t) k] = w;
        });
        int failed = 0;
        for (int k = 0; k < n; ++k)
            if (why[(size_t) k].isNotEmpty() && failed++ < 30)
                std::printf ("%s %s: %s\n", b[(size_t) (first + k)].id.toRawUTF8(), b[(size_t) (first + k)].name.toRawUTF8(), why[(size_t) k].toRawUTF8());
        std::printf ("checked %d presets: %d failed (%.0f s)\n", n, failed, (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
        return failed == 0 ? 0 : 1;
    }

    if (argc > 3 && juce::String (argv[1]) == "-env")   // -env <bank.cvb> <id>: 100 ms level curve of the audit note (debugging tails)
    {
        std::vector<VaultPreset> b;
        if (! vault::loadBankFile (cwd.getChildFile (argv[2]), b)) return 1;
        for (auto& p : b)
            if (p.id == juce::String (argv[3]))
            {
                juce::AudioBuffer<float> buf;
                vaultaudit::renderNote (p, vaultaudit::specNote (p.category), 0.8f, 10.0, buf, nullptr);
                for (int s0 = 0; s0 + 4800 <= buf.getNumSamples(); s0 += 4800)
                    std::printf ("%.1f s %6.1f dB\n", s0 / 48000.0, juce::Decibels::gainToDecibels (std::max (buf.getRMSLevel (0, s0, 4800), buf.getRMSLevel (1, s0, 4800)), -150.0f));
                for (auto& part : p.parts) { std::printf ("part %s:", part.label.toRawUTF8()); for (auto& [k, v] : part.values) std::printf (" %s=%g", k.toRawUTF8(), v); std::printf ("\n"); }
                return 0;
            }
        return 1;
    }
    if (argc > 3 && juce::String (argv[1]) == "-switch")   // -switch <bank.cvb> <first index> [count]: browse presets with a held chord (clicks)
    {
        std::vector<VaultPreset> b;
        if (! vault::loadBankFile (cwd.getChildFile (argv[2]), b)) return 1;
        const int first = juce::String (argv[3]).getIntValue(), count = argc > 4 ? juce::String (argv[4]).getIntValue() : 24;
        constexpr double sr = 48000.0; constexpr int block = 256;
        VaultEngine eng;
        eng.prepare (sr, block);
        eng.setPreset (VaultEngine::compile (b[(size_t) first]), true);
        const int every = (int) (0.25 * sr / block);
        juce::AudioBuffer<float> all (2, block * every * (count + 1)), buf (2, block);
        for (int k = 0; k < every * (count + 1); ++k)
        {
            juce::MidiBuffer m;
            if (k == 0) for (int nn : { 60, 63, 67 }) m.addEvent (juce::MidiMessage::noteOn (1, nn, 0.8f), 0);
            if (k > 0 && k % every == 0) eng.setPreset (VaultEngine::compile (b[(size_t) (first + k / every)]), false);
            eng.process (buf, m);
            for (int c = 0; c < 2; ++c) all.copyFrom (c, k * block, buf, c, 0, block);
        }
        for (int sw = 1; sw <= count; ++sw)
        {
            const int s0 = sw * every * block;
            float mj = 0, own = 0; int at = 0;
            for (int i = s0 - 64; i < s0 + 1200; ++i) for (int c = 0; c < 2; ++c) { const float j = std::abs (all.getSample (c, i) - all.getSample (c, i - 1)); if (j > mj) { mj = j; at = i - s0; } }
            for (int i = s0 + 2400; i < s0 + every * block - 64; ++i) for (int c = 0; c < 2; ++c) own = std::max (own, std::abs (all.getSample (c, i) - all.getSample (c, i - 1)));
            juce::MidiBuffer chord;
            for (int nn : { 60, 63, 67 }) chord.addEvent (juce::MidiMessage::noteOn (1, nn, 0.8f), 0);
            const auto fresh = VaultEngine::renderMidi (b[(size_t) (first + sw)], chord, (int) (0.03 * sr), sr);
            float fj = 0; int fat = 0;
            for (int i = 1; i < fresh.getNumSamples(); ++i) for (int c = 0; c < 2; ++c) { const float j = std::abs (fresh.getSample (c, i) - fresh.getSample (c, i - 1)); if (j > fj) { fj = j; fat = i; } }
            std::printf ("switch -> %d %-24s jump %.4f at %+d, own %.4f, fresh attack %.4f at %d  x%.2f\n", first + sw, b[(size_t) (first + sw)].name.toRawUTF8(),
                         mj, at, own, fj, fat, mj / std::max (1e-4f, std::max (own, fj)));
        }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "-audit")   // -audit <bank.cvb> [-csv file]: the independent audio audit (BankAudit.h)
    {
        juce::File csv; int every = 1;
        for (int i = 3; i + 1 < argc; ++i)
        {
            if (juce::String (argv[i]) == "-csv") csv = cwd.getChildFile (argv[i + 1]);
            if (juce::String (argv[i]) == "-every") every = juce::jmax (1, juce::String (argv[i + 1]).getIntValue());   // quick look: every N-th preset
        }
        return runAudit (cwd.getChildFile (argv[2]), csv, threads, every);
    }

    std::printf ("CHROME VAULT bank generator: %d threads\n", threads);
    Library lib = buildLibrary (threads);
    {
        int ok = 0; for (auto& s : lib.all) ok += s.ok;
        std::printf ("library: %d sources (%d usable) calibrated in %.1f s\n", (int) lib.all.size(), ok, (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
        for (int c = 0; c < vault::numCategories; ++c) std::printf ("   main pool %-18s %4d\n", v1c::names()[c].toRawUTF8(), (int) lib.mainOf[c].size());
    }

    // slots: per category the planned layer counts, interleaved
    const auto plan = layerPlan();
    struct Slot { int cat, layers, idx, attempt = 0; bool done = false; Candidate result; };
    std::vector<Slot> slots;
    for (int c = 0; c < vault::numCategories; ++c)
    {
        std::array<int, 4> n {};
        int total = 0;
        for (int l = 0; l < 4; ++l) { n[(size_t) l] = scale < 1.0 ? (int) std::ceil (plan[(size_t) c][(size_t) l] * scale) : plan[(size_t) c][(size_t) l]; total += n[(size_t) l]; }
        std::array<double, 4> acc {};
        for (int i = 0; i < total; ++i)   // even interleave (largest deficit first)
        {
            int best = 0; double bestD = -1e9;
            for (int l = 0; l < 4; ++l)
            {
                const double d = (double) n[(size_t) l] * (i + 1) / total - acc[(size_t) l];
                if (n[(size_t) l] > acc[(size_t) l] && d > bestD) { bestD = d; best = l; }
            }
            acc[(size_t) best] += 1;
            slots.push_back ({ c, best + 1, i });
        }
    }
    std::printf ("slots: %d\n", (int) slots.size());

    Stats stats;
    std::vector<int> acceptedIdx;      // slot indices in acceptance order
    std::vector<const Candidate*> accepted;
    for (int round = 0;; ++round)
    {
        std::vector<int> open;
        for (int i = 0; i < (int) slots.size(); ++i) if (! slots[(size_t) i].done) open.push_back (i);
        if (open.empty()) break;
        if (round > 200) { std::printf ("!! giving up: %d slots open\n", (int) open.size()); return 2; }
        std::vector<Candidate> cands (open.size());
        for (size_t j = 0; j < open.size(); ++j)
        {
            auto& s = slots[(size_t) open[j]];
            cands[j] = buildCandidate (lib, s.cat, s.layers, s.idx, s.attempt);
        }
        const auto tr = juce::Time::getMillisecondCounterHiRes();
        parallelFor ((int) cands.size(), threads, [&] (int j) { evaluate (cands[(size_t) j]); });
        stats.evaluated += (int) cands.size();
        // accept in slot order (deterministic), dedupe against everything accepted so far
        const float thr = dupThreshold * (round < 6 ? 1.0f : round < 12 ? 0.85f : 0.7f);
        int acc = 0;
        for (size_t j = 0; j < open.size(); ++j)
        {
            auto& s = slots[(size_t) open[j]];
            auto& cd = cands[j];
            ++s.attempt;
            if (! cd.ok) { ++stats.rejects[cd.reason]; continue; }
            bool dup = false;
            for (auto* a : accepted)
                if (printDist (*a, cd) < thr || vaultaudit::distance (a->audit.print, cd.audit.print) < vaultaudit::dupDistance + 0.1f) { dup = true; break; }
            if (dup) { ++stats.duplicates; ++stats.rejects["duplicate"]; continue; }
            for (auto& n : cd.notes) if (n.startsWith ("repair ")) ++stats.repairsOk[n.fromFirstOccurrenceOf ("repair ", false, false)];
            s.result = std::move (cd);
            s.done = true;
            accepted.push_back (&s.result);
            ++acc;
        }
        std::printf ("round %d: %d candidates, %d accepted (%.1f s), total %d / %d\n", round, (int) open.size(), acc,
                     (juce::Time::getMillisecondCounterHiRes() - tr) / 1000.0, (int) accepted.size(), (int) slots.size());
    }

    // bank in category order (slot order inside a category), ids, tags, names, labels
    std::vector<Candidate> bank;
    for (auto& s : slots) bank.push_back (std::move (s.result));
    assignTags (bank, lib);
    std::set<juce::String> usedNames;
    int perCat[vault::numCategories] {};
    for (size_t i = 0; i < bank.size(); ++i)
    {
        auto& c = bank[i];
        auto& p = c.preset;
        p.id = juce::String ("cv.") + catSlug (c.category) + "." + juce::String (++perCat[c.category]).paddedLeft ('0', 4);
        p.name = makeName (c, lib, usedNames, seedOf (4242, (uint64_t) i));
        Rng lr (seedOf (5151, (uint64_t) i));
        std::set<juce::String> labels;
        for (size_t k = 0; k < p.parts.size(); ++k)
        {
            const auto& src = lib.all[(size_t) c.srcs[k]];
            auto label = partLabel (Patch::fromValues (p.parts[k].values), c.roles[k], src.kind, lr);
            // the same instrument twice: name it by its register against the first one
            if (labels.count (label))
            {
                const Patch pk = Patch::fromValues (p.parts[k].values), p0 = Patch::fromValues (p.parts[0].values);
                const int effK = (int) pk[K::octave] + p.parts[k].octave, eff0 = (int) p0[K::octave] + p.parts[0].octave;
                const juce::String pre = effK > eff0 ? "High " : effK < eff0 ? "Low " : "Twin ";
                juce::String alt = pre + label;
                if (alt.length() > 16 || labels.count (alt)) alt = pre + label.fromLastOccurrenceOf (" ", false, false);
                if (labels.count (alt)) alt = "Layer " + juce::String ((int) k + 1);
                label = alt;
            }
            labels.insert (label);
            p.parts[k].label = label;
        }
        // v2: no "808" anywhere in what the user sees; the category follows the final layer labels (Piano vs Keys & Organ)
        p.name = vault::v1::cleanText (p.name);
        for (auto& part : p.parts) part.label = vault::v1::cleanText (part.label);
        p.category = vault::v1::toV2Category (c.category, p);
    }

    // final validation
    int invalid = 0;
    for (auto& c : bank) { juce::String why; if (! vault::validatePreset (c.preset, &why)) { ++invalid; std::printf ("!! invalid: %s\n", why.toRawUTF8()); } }
    std::vector<VaultPreset> out;
    for (auto& c : bank) out.push_back (c.preset);
    const auto outFile = cwd.getChildFile (outPath);
    outFile.getParentDirectory().createDirectory();
    const bool saved = vault::saveBankFile (outFile, out);
    std::vector<VaultPreset> back;
    const bool reloaded = vault::loadBankFile (outFile, back) && back == out;
    const double secs = (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0;
    std::printf ("bank: %d presets -> %s (%d KB) saved %d, round trip %d, invalid %d, %.0f s\n", (int) out.size(), outPath.toRawUTF8(),
                 (int) (outFile.getSize() / 1024), (int) saved, (int) reloaded, invalid, secs);

    // ---- report
    juce::String md;
    md << "# CHROME VAULT factory bank - QC report\n\n";
    md << "Generated by `VaultBankGen` (tools/vault/BankGen.cpp) - do not edit by hand. Bank: `" << outPath << "`, "
       << (int) out.size() << " presets, " << (int) (outFile.getSize() / 1024) << " KB. Generation took " << fmt ((float) secs, 0) << " s on "
       << threads << " threads.\n\n";
    md << "Quality gate (tools/vault/BankQC.h), every preset, two notes at 48 kHz (808 & Bass at C1 / C2 = MIDI 36 / 48, everything else C3 / C5 = MIDI 60 / 84),"
          " velocity 0.8, " << fmt ((float) vaultqc::baseSeconds) << " s render (longer for slow attacks), note off at 70 %: not silent, no NaN, peak < "
       << fmt (vaultqc::peakCeiling, 2) << " (the safety limiter never acts), no DC, no click at note start / end, mono-sum loss <= 3 dB, side under 90 Hz < -30 dB,"
          " loudness (K-weighted, loudest 400 ms, mean of both notes) within +-"
       << fmt (vaultqc::loudnessTolerance) << " dB of the category target and the low note within 1.5 dB of it, no near-duplicate (fingerprint distance >= "
       << fmt (dupThreshold) << "). Then the audit render at the final gain (tools/vault/BankAudit.h: C4, 808 & Bass C2, note off at 1.05 s, rendered"
          " until the tail is 60 dB down): real release tail <= 7.6 s, side power under 120 Hz (FFT) < -33 dB of the signal and < -23 dB of the mid"
          " under 120 Hz, mono-sum loss <= 2.7 dB, no DC, no click at note start / end / anywhere in the tail, and no near-duplicate by the audit's own"
          " time-frequency fingerprint (distance >= " << fmt (vaultaudit::dupDistance + 0.1f) << "). `VaultBankGen -audit` re-checks the shipped bank"
          " with the plain limits (tail 8 s, mono loss 3 dB, loudness 2 dB from the category median, CPU, names).\n\n";
    md << "## Counts\n\n| Category | Target | Presets | 1 layer | 2 layers | 3 layers | 4 layers | Loudness target | Loudness min / median / max | Max peak | Longest tail |\n";
    md << "| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | --- | ---: | ---: |\n";
    int layerTot[5] {};
    std::array<int, vault::numCategories> catTot {};
    for (int c = 0; c < vault::numCategories; ++c)
    {
        int lay[5] {}; std::vector<float> l; float pk = 0, tl = 0;
        for (auto& b : bank) if (b.category == c) { ++lay[b.preset.parts.size()]; l.push_back (b.loud); pk = std::max (pk, b.peak); tl = std::max (tl, b.tail); }
        std::sort (l.begin(), l.end());
        catTot[(size_t) c] = (int) l.size();
        for (int k = 1; k <= 4; ++k) layerTot[k] += lay[k];
        md << "| " << v1c::names()[c] << " | " << v1c::target (c) << " | " << (int) l.size() << " | " << lay[1] << " | " << lay[2] << " | "
           << lay[3] << " | " << lay[4] << " | " << fmt (vaultqc::targetLoudness (c)) << " | "
           << (l.empty() ? juce::String ("-") : fmt (l.front()) + " / " + fmt (l[l.size() / 2]) + " / " + fmt (l.back())) << " | " << fmt (pk, 3) << " | "
           << fmt (tl) << " s |\n";
    }
    md << "| **Total** | 5000 | " << (int) bank.size() << " | " << layerTot[1] << " | " << layerTot[2] << " | " << layerTot[3] << " | " << layerTot[4] << " | | | | |\n";
    md << "| Layer target | | | " << v1c::layerTarget (1) << " | " << v1c::layerTarget (2) << " | " << v1c::layerTarget (3) << " | "
       << v1c::layerTarget (4) << " | | | | |\n\n";

    md << "## Candidates and rejects\n\n" << stats.evaluated << " candidates rendered for " << (int) bank.size() << " presets.\n\n| Reason | Rejected |\n| --- | ---: |\n";
    for (auto& [k, v] : stats.rejects) md << "| " << k << " | " << v << " |\n";
    md << "\nRepaired and then accepted (the gate failed once, the generator fixed the preset and it passed):\n\n| Repair | Presets |\n| --- | ---: |\n";
    for (auto& [k, v] : stats.repairsOk) md << "| " << k << " | " << v << " |\n";

    {   // loudness spread
        std::vector<float> dev; float worst = 0;
        for (auto& b : bank) { const float d = b.loud - vaultqc::targetLoudness (v1c::toV2 (b.category)); dev.push_back (d); worst = std::max (worst, std::abs (d)); }
        std::sort (dev.begin(), dev.end());
        int within05 = 0; for (float d : dev) within05 += std::abs (d) <= 0.5f;
        std::vector<float> spread; for (auto& b : bank) spread.push_back (std::abs (b.loudLow - b.loudHigh));
        std::sort (spread.begin(), spread.end());
        md << "\n## Loudness\n\nDeviation from the category target: min " << fmt (dev.front(), 2) << " dB, median " << fmt (dev[dev.size() / 2], 2) << " dB, max "
           << fmt (dev.back(), 2) << " dB (worst |" << fmt (worst, 2) << "| dB); " << within05 << " presets within +-0.5 dB (presets whose peaks would reach the"
              " limiter sit up to 1 dB under the target). Difference between the two test notes: median " << fmt (spread[spread.size() / 2]) << " dB, 95th percentile "
           << fmt (spread[spread.size() * 95 / 100]) << " dB.\n";
    }
    {   // tags
        int st[8] {}, ch[8] {};
        for (auto& b : bank) for (int k = 0; k < 8; ++k) { st[k] += (b.styles >> k) & 1; ch[k] += (b.chars >> k) & 1; }
        md << "\n## Tags\n\n| Style | Presets | | Character | Presets |\n| --- | ---: | --- | --- | ---: |\n";
        for (int k = 0; k < 8; ++k)
            md << "| " << v1c::styleNames()[k] << " | " << st[k] << " | | " << v1c::characterNames()[k] << " | " << ch[k] << " |\n";
    }
    {   // sources
        std::map<int, int> roleUse; int factoryMain = 0;
        for (auto& b : bank) { for (int r : b.roles) ++roleUse[r]; factoryMain += lib.all[(size_t) b.srcs[0]].kkCat >= 0; }
        md << "\n## Sources\n\n" << factoryMain << " presets lead with an EVOLVE factory sound (variant), " << (int) bank.size() - factoryMain
           << " with a procedural recipe. Parts per role:";
        for (auto& [r, n] : roleUse) md << " " << roleName (r) << " " << n << ",";
        md = md.trimCharactersAtEnd (",") + ".\n";
    }
    {   // near-duplicate check: nearest-neighbour fingerprint distances over the final bank
        std::vector<float> nn (bank.size(), 1.0e9f);
        std::vector<int> nnIdx (bank.size(), -1);
        parallelFor ((int) bank.size(), threads, [&] (int i)
        {
            for (int j = 0; j < (int) bank.size(); ++j)
                if (j != i) { const float d = printDist (bank[(size_t) i], bank[(size_t) j]); if (d < nn[(size_t) i]) { nn[(size_t) i] = d; nnIdx[(size_t) i] = j; } }
        });
        std::vector<float> sorted = nn;
        std::sort (sorted.begin(), sorted.end());
        md << "\n## Uniqueness\n\nNearest-neighbour fingerprint distance (spectrum, attack spectrum, envelope and width of both test notes; dB-like units): min "
           << fmt (sorted.front(), 2) << ", 5th percentile " << fmt (sorted[sorted.size() / 20], 2) << ", median " << fmt (sorted[sorted.size() / 2], 2)
           << ". Closest pairs:\n\n| Distance | Preset | Nearest |\n| ---: | --- | --- |\n";
        std::vector<int> ord ((size_t) bank.size());
        for (int i = 0; i < (int) ord.size(); ++i) ord[(size_t) i] = i;
        std::sort (ord.begin(), ord.end(), [&] (int x, int y) { return nn[(size_t) x] < nn[(size_t) y]; });
        for (int k = 0; k < std::min (12, (int) ord.size()); k += 2)
        {
            const int i = ord[(size_t) k], j = nnIdx[(size_t) i];
            auto desc = [&] (int x) { auto& b = bank[(size_t) x]; return b.preset.name + " (" + b.preset.id + ", " + lib.all[(size_t) b.srcs[0]].name + ", "
                                                                    + b.notes.joinIntoString ("/") + ")"; };
            md << "| " << fmt (nn[(size_t) i], 2) << " | " << desc (i) << " | " << desc (j) << " |\n";
        }
    }
    md << "\n## Examples\n\n| Id | Name | Category | Layers | Styles | Character |\n| --- | --- | --- | --- | --- | --- |\n";
    for (size_t i = 0; i < bank.size(); i += 250)
    {
        auto& p = bank[i].preset;
        juce::StringArray lab; for (auto& pt : p.parts) lab.add (pt.label);
        md << "| " << p.id << " | " << p.name << " | " << vault::categoryNames()[p.category] << " | " << lab.joinIntoString (" + ") << " | "
           << v1c::namesOfMask (bank[i].styles, v1c::styleNames()).joinIntoString (", ") << " | " << v1c::namesOfMask (bank[i].chars, v1c::characterNames()).joinIntoString (", ") << " |\n";
    }
    const auto rep = cwd.getChildFile (reportPath);
    rep.getParentDirectory().createDirectory();
    rep.replaceWithText (md);
    std::printf ("report -> %s\n", reportPath.toRawUTF8());
    return saved && reloaded && invalid == 0 ? 0 : 1;
}
