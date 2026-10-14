// CHROME VAULT v2 - sound design for the categories "Plucks", "Leads", "Pads", "Synth" and "FX & Atmos"
// (VaultBankGen -recipe synth).
//
// DESIGN NOTE (research summary - how preset / sample libraries voice these sounds, and what this recipe copies)
// --------------------------------------------------------------------------------------------------------------
// * Plucks are a family of DIFFERENT excitation / resonator models, not one patch with jitter: the subtractive pluck
//   (bright oscillator + fast-closing low-pass envelope, a little resonance), the Karplus-Strong string (noise burst
//   into a damped delay loop: the loop filter sets how fast the highs die, the burst filter the hardness - Karplus &
//   Strong 1983, Jaffe & Smith 1983), the FM pluck (the MODULATOR is enveloped: a short index decay gives the bright,
//   inharmonic strike and a mellow ring - integer ratios = harmonic glass / wood / tine, non-integer = metal), modal
//   bars (marimba / kalimba / bell mode ratios) and wavetable / sync plucks. Trap and pluggnb producers play them
//   low-passed (room for the vocal and the 808), wide, soft-attacked, with synced 1/8-dotted echoes, tape wow and a
//   short dark reverb; real pizzicato / harp / mallet transients are layered under synth plucks for a "hybrid" body.
// * Leads sit mainly in 200 Hz - 2 kHz like a vocal: one strong voice, mono + legato glide (trap leads use generous
//   portamento), delayed vibrato, a dip-in scoop, square = hollow / reedy, narrow pulse = thin trap lead, sine +
//   breath = whistle, drive for the distorted lead but always low-passed (the 2-5 kHz region is where it hurts), delay
//   throws instead of big reverbs. Octave-down doubles add weight; octave-up doubles are kept >= 12 dB down.
// * Pads: long attack / release so chords bleed together, slow asynchronous modulation (filter LFO, vowel / wave
//   drift, a second LFO at another rate), a detuned second oscillator or unison for width, long reverb (high-passed
//   in the mix) plus some tape delay; layers at different octaves for depth. Libraries also make pads from REAL
//   recordings: bowed string sections with slow attacks in a hall, choir "aahs" / "oohs" with a sine / warm layer to
//   carry the fundamental, organ flue stops, glass harp, bowed vibraphone, flutes - played with long releases and
//   cloud / cathedral spaces, sometimes reversed or tape-degraded.
// * Synth (poly synths / stabs / keys): poly saw with chorus, synth brass (slow filter attack), unison / supersaw stabs,
//   square chord stabs, FM / digital keys, house organ, formant stabs, synth strings, gated / pumping chords and
//   tempo-locked two-step sequences; hybrids layer a real piano, pizzicato or horn staccato under a synth.
// * FX & Atmos: risers are tonal sweeps (pitch + filter) and noise swells that grow in brightness, often stacked;
//   downlifters / falls, impacts with huge tails, drones and textures (slow evolving, detuned, lo-fi), wind, sci-fi
//   random-pitch chatter, and reversed real instruments (reverse swells run backwards into the downbeat).
//
// HOW THE BANK IS MADE
// * Voices = distinct generators per category (synth engine architectures: VA saw / square / pulse / supersaw, Karplus-
//   Strong, FM, modal bars, wavetable frames, sync, formant, organ, flute / breath, orchestral; library recordings for
//   real-sample pads, hybrids and reversed FX; physical models where they fit). Treatments are processing ROLES
//   (short, ringing, muted, echo, lo-fi, tape, wide, scooped, octave body, fifth, reversed, resonant body, drive, ...)
//   and spaces (dry .. cathedral). Layer recipes add a second / third / fourth instrument that changes the sound.
// * Every part is calibrated alone (its `gain` puts it at -20 dB K-weighted at C5) so layer levels mean the same thing.
// * Each candidate runs the shared v2 gate (RecipeGate.h, C5 = MIDI 60) with automatic repairs, plus this file's
//   stricter comfort caps (2-5 kHz <= 8 %, > 6 kHz <= 2 %), then TWO distinctness checks against everything accepted
//   before it: the shared log-mel / harmonic / motion fingerprint (>= sharedMin) and this file's own MFCC fingerprint
//   (C5 vel 0.8 incl. the release tail, C5 vel 0.35, C4, C6, stereo, modulation) (>= ownMin). Near-duplicates are
//   dropped, never renamed. Names: evocative English, no brands, no "808"; part labels are instrument names.
// * Candidate order: every voice as designed first, then treated voices and layer recipes round robin (2 : 1), so the
//   greedy distinctness pass keeps the plain instruments and spreads the rest evenly over all voices. Plucks and Synth
//   are capped (300 / 270, SYNTH_CAP_PLUCKS / SYNTH_CAP_SYNTH) to keep the five categories balanced.
// * Sample parts must play at C4, C5 and C6 (FX & Atmos: at C5) - a pad silent below its instrument's range is broken.
//   Recorded dynamics are kept but limited: velocity 0.35 -> 1.0 may open at most 20 dB.
//
// Environment (optional): SYNTH_THREADS, SYNTH_LIMIT (first N candidates), SYNTH_CATS (comma list of category indices),
// SYNTH_REPORT (TSV report path), SYNTH_OWN_MIN / SYNTH_SHARED_MIN (thresholds).
#include "Recipe.h"
#include "RecipeGate.h"
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <thread>

namespace
{
using Values = std::vector<std::pair<juce::String, float>>;
using namespace vault;

//==============================================================================
// part makers
void setAll (VaultPart& p, const Values& v) { for (auto& [k, x] : v) p.set (k, x); }

// a synth part: neutral FX / hidden macros first (EVOLVE defaults add reverb, delay, chorus, vibrato and MASTER glue),
// every engine knob that has a non-neutral default set explicitly (fmAmt is FM index / warp / vowel / pitch drop)
VaultPart Y (const char* label, const Values& v)
{
    auto p = recipe::synthPart (label);
    setAll (p, { { "engine", 0 }, { "wave", 0 }, { "unison", 1 }, { "detune", 0.15f }, { "fmAmt", 0 }, { "fmRatio", 1 }, { "fmRatio2", 1 },
                 { "fmAlgo", 0 }, { "warpMode", 0 }, { "levelA", 1 }, { "layerB", 0 }, { "sub", 0 }, { "filterType", 0 }, { "cutoff", 12000 },
                 { "reso", 0.05f }, { "keyTrack", 0.5f }, { "fenv", 0 }, { "attack", 0.003f }, { "decay", 0.6f }, { "sustain", 0.8f },
                 { "release", 0.3f }, { "velSens", 0.65f }, { "lfoPitch", 0 }, { "lfoFilter", 0 }, { "lfoAmp", 0 },
                 { "macro2", 0 }, { "macro5", 0 }, { "revMix", 0 }, { "delayMix", 0 }, { "master", 0 }, { "chorus", 0 } });
    setAll (p, v);
    return p;
}
// layer B of a synth part (inside one part: one more oscillator through the same filter / envelope)
Values B (int engine, float wave, int octave, int semi, float level, Values extra = {})
{
    Values v { { "layerB", 1 }, { "engineB", (float) engine }, { "waveB", wave }, { "octaveB", (float) octave }, { "semiB", (float) semi },
               { "levelB", level }, { "fmAmtB", 0 }, { "unisonB", 1 }, { "fineB", 0 } };
    for (auto& e : extra) v.push_back (e);
    return v;
}
Values operator+ (Values a, const Values& b) { for (auto& x : b) a.push_back (x); return a; }
// a mod-matrix route in slot s (0..7): src 1 env2, 2 env3, 3 lfo1, 4 lfo2, 5 vel, 9 random; dst 0 pitch, 1 cutoff,
// 2 reso, 3 wave A, 4 wave B, 5 FM / warp A, 7 level A, 8 level B, 9 amp, 10 pan, 12 detune
Values MM (int slot, int src, int dst, float amt)
{
    const juce::String n (slot + 1);
    return { { "mmSrc" + n, (float) src }, { "mmDst" + n, (float) dst }, { "mmAmt" + n, amt } };
}
VaultPart S (const char* instrument, const char* articulation, const char* label, Values v = {})
{
    auto p = recipe::samplePart (instrument, articulation, label);
    p.values = std::move (v);
    return p;
}
VaultPart M (const char* model, const char* label, Values v = {})
{
    auto p = recipe::modelPart (model, label);
    p.values = std::move (v);
    return p;
}
float getv (const VaultPart& p, const char* k, float def) { return p.get (k, def); }
void setv (VaultPart& p, const char* k, float v) { p.set (k, v); }
bool isSynth (const VaultPart& p) { return p.kind == VaultPartKind::synth; }
bool isSample (const VaultPart& p) { return p.kind == VaultPartKind::sample; }

// brightness in Hz, whatever the part kind
void lowPass (VaultPart& p, float hz)
{
    if (isSample (p)) setv (p, "s.cutoff", juce::jlimit (250.0f, 20000.0f, std::min (hz, getv (p, "s.cutoff", 20000.0f))));
    else if (p.kind == VaultPartKind::model)
    {
        const float t = juce::jlimit (0.0f, 1.0f, 0.5f + 0.5f * std::log2 (hz / 6000.0f));
        setv (p, "m.tone", std::min (t, getv (p, "m.tone", 0.5f)));
    }
    else setv (p, "cutoff", juce::jlimit (120.0f, 20000.0f, std::min (hz, getv (p, "cutoff", 12000.0f))));
}
void scaleCut (VaultPart& p, float k)
{
    if (isSample (p)) setv (p, "s.cutoff", juce::jlimit (250.0f, 20000.0f, std::min (12000.0f, getv (p, "s.cutoff", 20000.0f)) * k));
    else if (p.kind == VaultPartKind::model) setv (p, "m.tone", juce::jlimit (0.0f, 1.0f, getv (p, "m.tone", 0.5f) + 0.5f * std::log2 (k)));
    else setv (p, "cutoff", juce::jlimit (120.0f, 20000.0f, getv (p, "cutoff", 12000.0f) * k));
}
float currentLp (const VaultPart& p)
{
    if (isSample (p)) return getv (p, "s.cutoff", 20000.0f);
    if (p.kind == VaultPartKind::model) return 6000.0f * std::exp2 ((getv (p, "m.tone", 0.5f) - 0.5f) * 2.0f);
    return getv (p, "cutoff", 12000.0f);
}
void velSens (VaultPart& p, float v)
{
    if (isSample (p)) setv (p, "s.velSens", v);
    else if (p.kind == VaultPartKind::model) setv (p, "m.velSens", v);
    else setv (p, "velSens", v);
}
float getVelSens (const VaultPart& p)
{
    if (isSample (p)) return getv (p, "s.velSens", 0.5f);
    if (p.kind == VaultPartKind::model) return getv (p, "m.velSens", 0.75f);
    return getv (p, "velSens", 0.65f);
}
void attack (VaultPart& p, float s)
{
    if (isSample (p)) setv (p, "s.attack", s);
    else if (p.kind == VaultPartKind::model) { if (p.source.startsWith ("voice")) setv (p, "m.attack", s); }
    else setv (p, "attack", s);
}
void release (VaultPart& p, float s)
{
    if (isSample (p)) setv (p, "s.release", s);
    else if (p.kind == VaultPartKind::model) setv (p, "m.release", juce::jlimit (0.02f, 4.0f, s));
    else setv (p, "release", juce::jlimit (0.005f, 8.0f, s));
}
// shorten / lengthen a decaying sound (synth: amp + filter decay, KS / modal string decay; sample: ADSR override)
void decayScale (VaultPart& p, float k)
{
    if (isSynth (p))
    {
        setv (p, "decay", juce::jlimit (0.03f, 8.0f, getv (p, "decay", 0.6f) * k));
        setv (p, "fdecay", juce::jlimit (0.01f, 4.0f, getv (p, "fdecay", 0.4f) * std::sqrt (k)));
        setv (p, "release", juce::jlimit (0.03f, 8.0f, getv (p, "release", 0.3f) * k));
    }
    else if (isSample (p))
    {
        if (k < 1.0f) { setv (p, "s.decay", 0.5f * k + 0.08f); setv (p, "s.sustain", 0.0f); setv (p, "s.release", 0.15f); }
        else setv (p, "s.release", 1.2f * k);
    }
    else
    {
        setv (p, "m.decay", juce::jlimit (0.2f, 4.0f, getv (p, "m.decay", 1.0f) * k));
        setv (p, "m.release", juce::jlimit (0.03f, 3.0f, getv (p, "m.release", 0.3f) * k));
    }
}
juce::String joinWords (std::initializer_list<juce::String> w)
{
    juce::StringArray a;
    for (auto& s : w) if (s.trim().isNotEmpty()) a.add (s.trim());
    return a.joinIntoString (" ");
}

//==============================================================================
// spaces (FX of part 0: the engine reads the master FX from part 0 only)
struct Space { const char* tag; const char* word; Values fx; };
const Space& space (const char* tag)
{
    static const std::vector<Space> s {
        { "dry",       "Close",     { { "revMix", 0.06f }, { "revType", 1 }, { "revSize", 0.2f } } },
        { "room",      "Room",      { { "revMix", 0.14f }, { "revType", 1 }, { "revSize", 0.34f } } },
        { "plate",     "Plate",     { { "revMix", 0.22f }, { "revType", 1 }, { "revSize", 0.55f } } },
        { "hall",      "Hall",      { { "revMix", 0.26f }, { "revType", 0 }, { "revSize", 0.66f } } },
        { "cloud",     "Cloud",     { { "revMix", 0.36f }, { "revType", 2 }, { "revSize", 0.7f } } },
        { "cathedral", "Cathedral", { { "revMix", 0.4f },  { "revType", 0 }, { "revSize", 0.9f } } },
        { "vast",      "Infinite",  { { "revMix", 0.5f },  { "revType", 2 }, { "revSize", 0.93f } } },
        { "echo",      "Echo",      { { "delayMix", 0.2f }, { "delayTime", 2 }, { "delayFb", 0.36f }, { "delayMode", 2 }, { "revMix", 0.12f }, { "revType", 1 }, { "revSize", 0.4f } } },
        { "ping",      "Delay",     { { "delayMix", 0.22f }, { "delayTime", 0 }, { "delayFb", 0.3f }, { "delayMode", 0 }, { "revMix", 0.15f }, { "revType", 0 }, { "revSize", 0.5f } } },
        { "slap",      "Slap",      { { "delayMix", 0.16f }, { "delayTime", 3 }, { "delayFb", 0.12f }, { "delayMode", 1 }, { "revMix", 0.1f }, { "revType", 1 }, { "revSize", 0.3f } } },
        { "dub",       "Dub",       { { "delayMix", 0.28f }, { "delayTime", 2 }, { "delayFb", 0.5f }, { "delayMode", 2 }, { "revMix", 0.2f }, { "revType", 2 }, { "revSize", 0.6f } } } };
    for (auto& x : s) if (juce::String (x.tag) == tag) return x;
    jassertfalse;
    return s.front();
}
void fx (VaultPreset& p, const Values& v) { setAll (p.parts.front(), v); }

//==============================================================================
// voices: the main instrument of a preset (part 0). `kind` = generator family (treatments / layers pick by kind).
enum Kind : uint32_t
{
    kSaw = 1u << 0, kSquare = 1u << 1, kPulse = 1u << 2, kSuper = 1u << 3, kString = 1u << 4, kFM = 1u << 5, kModal = 1u << 6,
    kWave = 1u << 7, kVox = 1u << 8, kOrgan = 1u << 9, kBreath = 1u << 10, kOrch = 1u << 11, kSine = 1u << 12, kNoise = 1u << 13,
    kSampleSus = 1u << 14, kSampleShot = 1u << 15, kModel = 1u << 16, kRiser = 1u << 17, kDrone = 1u << 18, kHit = 1u << 19
};
constexpr uint32_t kAnalog = kSaw | kSquare | kPulse | kSuper;
constexpr uint32_t kSynthAll = kAnalog | kString | kFM | kModal | kWave | kVox | kOrgan | kBreath | kOrch | kSine;
constexpr uint32_t kSamples = kSampleSus | kSampleShot;
constexpr uint32_t kAll = 0xffffffffu;

struct Voice
{
    const char* tag;
    int category;
    uint32_t kind;
    VaultPart part;
    juce::StringArray nouns;      // name nouns, best first
};

// common pluck envelope: fast attack, no sustain, filter envelope from `cut` (Hz) by `fenv` octaves x6
Values pluckEnv (float cut, float fenvAmt, float fdec, float dec, float rel = 0.35f)
{
    return { { "cutoff", cut }, { "fenv", fenvAmt }, { "fattack", 0.001f }, { "fdecay", fdec }, { "fsustain", 0 }, { "frelease", rel },
             { "attack", 0.003f }, { "decay", dec }, { "sustain", 0 }, { "release", rel } };
}
// lead: mono legato with glide, delayed vibrato
Values leadEnv (float cut, float glide = 0.1f)
{
    return { { "cutoff", cut }, { "mono", 1 }, { "legato", 1 }, { "glide", glide }, { "attack", 0.006f }, { "decay", 0.5f }, { "sustain", 0.85f },
             { "release", 0.18f }, { "lfoRate", 5.4f }, { "lfoPitch", 0.05f }, { "bendRange", 2 } };
}
Values padEnv (float cut, float att = 0.7f, float rel = 2.2f)
{
    return { { "cutoff", cut }, { "attack", att }, { "decay", 1.5f }, { "sustain", 0.9f }, { "release", rel }, { "keyTrack", 0.45f },
             { "lfoRate", 0.22f }, { "lfoFilter", 0.06f }, { "velSens", 0.6f } };
}
Values polyEnv (float cut, float dec = 0.9f, float sus = 0.6f, float rel = 0.4f)
{
    return { { "cutoff", cut }, { "attack", 0.004f }, { "decay", dec }, { "sustain", sus }, { "release", rel }, { "fenv", 0.25f }, { "fdecay", 0.5f },
             { "fsustain", 0.2f } };
}
constexpr float wSine = 0.0f, wTri = 1.0f / 7, wSaw = 2.0f / 7, wSq = 3.0f / 7, wPulse = 4.0f / 7, wGlass = 5.0f / 7, wDigi = 6.0f / 7;

std::vector<Voice> voices()
{
    std::vector<Voice> v;
    auto add = [&] (const char* tag, int cat, uint32_t k, VaultPart p, juce::StringArray nouns) { v.push_back ({ tag, cat, k, std::move (p), std::move (nouns) }); };
    //------------------------------------------------------------------ PLUCKS
    add ("sawpluck", catPlucks, kSaw, Y ("Saw Pluck", pluckEnv (1400, 0.5f, 0.26f, 0.9f) + Values { { "reso", 0.15f }, { "keyTrack", 0.6f } }),
         { "Saw Pluck", "Analog Pluck", "Filter Pluck" });
    add ("sqpluck", catPlucks, kSquare, Y ("Square Pluck", pluckEnv (1500, 0.42f, 0.2f, 0.75f) + Values { { "wave", 0.5f } }),
         { "Square Pluck", "Box Pluck", "Hollow Pluck" });
    add ("pulsepluck", catPlucks, kPulse, Y ("Pulse Pluck", pluckEnv (1800, 0.35f, 0.18f, 0.6f) + Values { { "wave", 0.85f } }),
         { "Reed Pluck", "Pulse Pluck", "Nasal Pluck" });
    add ("superpluck", catPlucks, kSuper, Y ("Supersaw Pluck", pluckEnv (1500, 0.45f, 0.3f, 1.1f) + Values { { "unison", 7 }, { "detune", 0.22f } }),
         { "Supersaw Pluck", "Stack Pluck", "Wide Pluck" });
    add ("kswire", catPlucks, kString, Y ("Plucked String", Values { { "engine", 2 }, { "wave", 0.62f }, { "cutoff", 5200 }, { "attack", 0.004f }, { "decay", 1.3f },
                                                            { "sustain", 1 }, { "release", 0.6f } }),
         { "Wire Pluck", "Steel Pluck", "String Pluck" });
    add ("ksgut", catPlucks, kString, Y ("Plucked String", Values { { "engine", 2 }, { "wave", 0.2f }, { "cutoff", 3200 }, { "attack", 0.004f }, { "decay", 0.9f },
                                                           { "sustain", 1 }, { "release", 0.45f } }),
         { "Gut Pluck", "Nylon Pluck", "Soft String" });
    add ("kszither", catPlucks, kString, Y ("Zither String", Values { { "engine", 2 }, { "wave", 0.42f }, { "cutoff", 4000 }, { "attack", 0.004f }, { "decay", 3.0f },
                                                            { "sustain", 1 }, { "release", 1.4f } }),
         { "Zither Pluck", "Long String", "Koto Pluck" });
    add ("fmglass", catPlucks, kFM, Y ("FM Glass", pluckEnv (5000, 0.0f, 0.3f, 1.2f, 0.6f) + Values { { "engine", 1 }, { "fmRatio", 4 }, { "fmAmt", 0.36f } }),
         { "Glass Pluck", "Crystal Pluck", "Ice Pluck" });
    add ("fmwood", catPlucks, kFM, Y ("FM Wood", pluckEnv (3500, 0.0f, 0.07f, 0.45f, 0.25f) + Values { { "engine", 1 }, { "fmRatio", 1 }, { "fmAmt", 0.45f }, { "wave", 0.3f } }),
         { "Wood Pluck", "Timber Pluck", "Log Pluck" });
    add ("fmtine", catPlucks, kFM, Y ("FM Tine", pluckEnv (4500, 0.0f, 0.35f, 1.4f, 0.5f) + Values { { "engine", 1 }, { "fmAlgo", 5 }, { "fmRatio", 1 }, { "fmRatio2", 14 }, { "fmAmt", 0.3f } }),
         { "Tine Pluck", "Key Pluck", "Chime Pluck" });
    add ("fmnylon", catPlucks, kFM, Y ("FM Harp", pluckEnv (3000, 0.0f, 0.2f, 1.0f, 0.4f) + Values { { "engine", 1 }, { "fmRatio", 2 }, { "fmAmt", 0.3f } }),
         { "Harp Pluck", "Lyre Pluck", "Silk Pluck" });
    add ("modalmar", catPlucks, kModal, Y ("Marimba Synth", Values { { "engine", 9 }, { "wave", 0.5f }, { "fmAmt", 0.45f }, { "cutoff", 6000 }, { "attack", 0.003f },
                                                           { "decay", 0.55f }, { "sustain", 1 }, { "release", 0.35f } }),
         { "Mallet Pluck", "Rosewood Pluck", "Bar Pluck" });
    add ("modalkal", catPlucks, kModal, Y ("Thumb Piano Synth", Values { { "engine", 9 }, { "wave", 0.15f }, { "fmAmt", 0.4f }, { "cutoff", 6000 }, { "attack", 0.003f },
                                                           { "decay", 0.8f }, { "sustain", 1 }, { "release", 0.4f } }),
         { "Thumb Pluck", "Lamella Pluck", "Tine Box Pluck" });
    add ("wtprism", catPlucks, kWave, Y ("Glass Wave", pluckEnv (1800, 0.3f, 0.25f, 1.0f) + Values { { "engine", 7 }, { "wave", wGlass } }),
         { "Prism Pluck", "Shard Pluck", "Facet Pluck" });
    add ("wtdigi", catPlucks, kWave, Y ("Digital Wave", pluckEnv (2500, 0.3f, 0.2f, 0.7f) + Values { { "engine", 7 }, { "wave", wDigi } }),
         { "Digital Pluck", "Pixel Pluck", "Bit Pluck" });
    add ("wttri", catPlucks, kWave | kSine, Y ("Triangle Pluck", pluckEnv (5000, 0.1f, 0.3f, 0.9f) + Values { { "engine", 7 }, { "wave", wTri } }),
         { "Soft Pluck", "Felt Pluck", "Round Pluck" });
    add ("syncpluck", catPlucks, kWave, Y ("Sync Pluck", pluckEnv (2600, 0.25f, 0.22f, 0.7f) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 } } + MM (0, 1, 5, 0.55f)),
         { "Sync Pluck", "Zap Pluck", "Snap Pluck" });
    add ("organpluck", catPlucks, kOrgan, Y ("Organ Pluck", Values { { "engine", 4 }, { "wave", 0.35f }, { "cutoff", 4000 }, { "attack", 0.003f }, { "decay", 0.35f },
                                                          { "sustain", 0 }, { "release", 0.2f } }),
         { "Click Pluck", "Organ Pluck", "Drawbar Pluck" });
    add ("breathpluck", catPlucks, kBreath, Y ("Breath Pluck", Values { { "engine", 5 }, { "wave", 0.45f }, { "cutoff", 3000 }, { "attack", 0.004f }, { "decay", 0.5f },
                                                            { "sustain", 0 }, { "release", 0.3f } }),
         { "Breath Pluck", "Pan Pluck", "Air Pluck" });
    add ("voxpluck", catPlucks, kVox, Y ("Vocal Pluck", pluckEnv (2500, 0.15f, 0.25f, 0.6f) + Values { { "engine", 3 }, { "wave", 1.0f } }),
         { "Vocal Pluck", "Voice Pluck", "Chant Pluck" });
    add ("orchpizz", catPlucks, kOrch, Y ("Synth Pizzicato", Values { { "engine", 8 }, { "wave", 0.5f }, { "unison", 3 }, { "detune", 0.08f }, { "cutoff", 2600 }, { "attack", 0.003f },
                                                              { "decay", 0.32f }, { "sustain", 0 }, { "release", 0.22f } }),
         { "Pizz Pluck", "Synth Pizz", "Bowstring Pluck" });
    add ("ladderpluck", catPlucks, kSaw, Y ("Ladder Pluck", pluckEnv (700, 0.55f, 0.24f, 0.8f) + Values { { "filterType", 1 }, { "reso", 0.42f } }),
         { "Rubber Pluck", "Resonant Pluck", "Squelch Pluck" });
    add ("bandpluck", catPlucks, kSaw, Y ("Band Pluck", pluckEnv (1100, 0.3f, 0.22f, 0.7f) + Values { { "filterType", 4 }, { "reso", 0.25f } }),
         { "Tube Pluck", "Pipe Pluck", "Bottle Pluck" });
    add ("droplet", catPlucks, kSine, Y ("Sine Drop", pluckEnv (8000, 0.0f, 0.045f, 0.6f, 0.3f) + Values { { "engine", 7 }, { "wave", wSine } } + MM (0, 1, 0, 0.22f)),
         { "Water Pluck", "Droplet", "Bubble Pluck" });
    add ("fmmetal", catPlucks, kFM, Y ("FM Metal", pluckEnv (4000, 0.0f, 0.25f, 0.9f, 0.4f) + Values { { "engine", 1 }, { "fmRatio", 3.5f }, { "fmAmt", 0.2f } }),
         { "Metal Pluck", "Tin Pluck", "Steel Drum Pluck" });
    add ("harppluck", catPlucks, kSampleShot, S ("pluck.harp.concert", "pluck", "Concert Harp", { { "s.cutoff", 4000.0f }, { "s.velSens", 0.35f } }),
         { "Harp Pluck", "Harpstring", "Angel Pluck" });
    add ("pizzpluck", catPlucks, kSampleShot, S ("strings.violas.ensemble", "pizzicato", "Viola Section", { { "s.velSens", 0.3f } }),
         { "Pizzicato", "Pizz Section", "Plucked Bows" });
    add ("dulcpluck", catPlucks, kSampleShot, S ("pluck.dulcimer", "pluck", "Stick Dulcimer", { { "s.cutoff", 4500.0f }, { "s.velSens", 0.35f } }),
         { "Dulcimer Pluck", "Hammer Pluck", "Folk Pluck" });
    add ("sitarpluck", catPlucks, kString, Y ("Sitar Synth", Values { { "engine", 2 }, { "wave", 0.55f }, { "cutoff", 4500 }, { "attack", 0.004f }, { "decay", 1.6f },
                                                                 { "sustain", 1 }, { "release", 0.7f }, { "body", 6 }, { "bodyMix", 0.5f } }),
         { "Sitar Pluck", "Drone String Pluck", "Buzz String" });
    add ("steelpan", catPlucks, kFM, Y ("Steel Pan Synth", pluckEnv (4500, 0.0f, 0.3f, 0.9f, 0.4f) + Values { { "engine", 1 }, { "fmAlgo", 4 }, { "fmRatio", 1 }, { "fmRatio2", 2 }, { "fmAmt", 0.25f } }),
         { "Steel Pan Pluck", "Island Pluck", "Calypso Pluck" });
    add ("celestapl", catPlucks, kModal, Y ("Celesta Synth", Values { { "engine", 9 }, { "wave", 0.8f }, { "fmAmt", 0.12f }, { "cutoff", 5000 }, { "attack", 0.003f }, { "decay", 0.9f },
                                                                   { "sustain", 1 }, { "release", 0.5f } }),
         { "Celesta Pluck", "Music Box Pluck", "Star Pluck" });
    add ("mutedgtr", catPlucks, kModel, M ("guitar.electric", "Electric Guitar", { { "m.artic", 1 }, { "m.tone", 0.45f } }), { "Muted Pluck", "Palm Pluck", "Damped Pluck" });
    add ("harmonicpl", catPlucks, kModel, M ("guitar.nylon", "Nylon Guitar", { { "m.artic", 2 } }), { "Harmonic Pluck", "Flageolet Pluck", "Ghost Harmonic" });
    add ("zitherpl", catPlucks, kSampleShot, S ("world.dantranh", "pluck", "Dan Tranh", { { "s.cutoff", 4500.0f }, { "s.velSens", 0.35f } }), { "Silk Zither Pluck", "River Zither", "Bamboo Zither" });
    add ("mbirapl", catPlucks, kSampleShot, S ("world.mbira.deep", "pluck", "Deep Mbira", { { "s.cutoff", 4500.0f }, { "s.velSens", 0.35f } }), { "Mbira Pluck", "Iron Tine Pluck", "Village Mbira" });
    add ("marimbapl", catPlucks, kSampleShot, S ("mallets.marimba", "hit", "Marimba", { { "s.decay", 0.35f }, { "s.sustain", 0.0f }, { "s.release", 0.2f }, { "s.velSens", 0.35f } }),
         { "Wood Bar Pluck", "Rosewood Bar", "Marimba Pluck" });
    add ("quillpl", catPlucks, kSampleShot, S ("keys.harpsichord.italian", "normal", "Italian Harpsichord", { { "s.cutoff", 4000.0f }, { "s.decay", 0.5f }, { "s.sustain", 0.0f },
                                                                                                       { "s.release", 0.2f }, { "s.velSens", 0.35f } }),
         { "Quill Pluck", "Plectrum Pluck", "Courtly Pluck" });
    add ("duopluck", catPlucks, kSaw | kSquare, Y ("Two Osc Pluck", pluckEnv (1600, 0.45f, 0.22f, 0.8f) + B (0, 0.5f, -1, 0, 0.5f)),
         { "Duo Pluck", "Layered Pluck", "Two Osc Pluck" });
    add ("chippluck", catPlucks, kSquare, Y ("Chip Pluck", pluckEnv (4000, 0.1f, 0.2f, 0.35f, 0.15f) + Values { { "wave", 0.62f }, { "crush", 0.2f } }),
         { "8-Bit Pluck", "Arcade Pluck", "Console Pluck" });
    add ("fmbellpl", catPlucks, kFM, Y ("FM Bell", pluckEnv (5000, 0.0f, 0.5f, 1.6f, 0.8f) + Values { { "engine", 1 }, { "fmAlgo", 4 }, { "fmRatio", 3.5f }, { "fmRatio2", 1 }, { "fmAmt", 0.14f } }),
         { "Bell Pluck", "Chapel Pluck", "Glimmer Pluck" });
    add ("pwmpluck", catPlucks, kPulse, Y ("PWM Pluck", pluckEnv (1800, 0.35f, 0.22f, 0.8f) + Values { { "wave", 0.7f }, { "unison", 2 }, { "detune", 0.08f }, { "lfo2Rate", 5.5f } } + MM (0, 4, 3, 0.15f)),
         { "Liquid Pluck", "Shimmer Pluck", "Ripple Pluck" });
    add ("organks", catPlucks, kOrgan | kString, Y ("Organ String", Values { { "engine", 4 }, { "wave", 0.4f }, { "cutoff", 4000 }, { "attack", 0.003f }, { "decay", 0.5f }, { "sustain", 0 },
                                                                         { "release", 0.3f } } + B (2, 0.5f, 0, 0, 0.7f)),
         { "Hybrid Organ Pluck", "Reed String Pluck", "Harmonium Pluck" });
    add ("brasspl", catPlucks, kOrch, Y ("Brass Blip", Values { { "engine", 8 }, { "wave", 0.0f }, { "unison", 2 }, { "detune", 0.06f }, { "cutoff", 3000 }, { "attack", 0.004f },
                                                              { "decay", 0.38f }, { "sustain", 0 }, { "release", 0.25f } }),
         { "Brass Pluck", "Horn Blip", "Fanfare Pluck" });

    //------------------------------------------------------------------ LEADS
    add ("thin", catLeads, kPulse, Y ("Thin Lead", leadEnv (3800) + Values { { "wave", 0.82f }, { "lfoPitch", 0.07f } }),
         { "Thin Lead", "Needle Lead", "Wire Lead" });
    add ("square", catLeads, kSquare, Y ("Square Lead", leadEnv (3200) + Values { { "wave", 0.5f } }),
         { "Square Lead", "Hollow Lead", "Clarinet Lead" });
    add ("saw", catLeads, kSaw, Y ("Saw Lead", leadEnv (3000) + Values { { "fenv", 0.15f }, { "fdecay", 0.4f }, { "fsustain", 0.3f } }),
         { "Saw Lead", "Classic Lead", "Mono Lead" });
    add ("superlead", catLeads, kSuper, Y ("Supersaw Lead", leadEnv (3200, 0.06f) + Values { { "mono", 0 }, { "unison", 6 }, { "detune", 0.17f } }),
         { "Supersaw Lead", "Anthem Lead", "Stack Lead" });
    add ("whistle", catLeads, kSine | kBreath, Y ("Whistle", leadEnv (5000, 0.12f) + Values { { "engine", 7 }, { "wave", wSine }, { "lfoPitch", 0.09f }, { "lfoRate", 5.9f } }
                                                    + B (5, 0.6f, 0, 0, 0.35f)),
         { "Whistle Lead", "Whistle", "Tin Whistle" });
    add ("flutelead", catLeads, kBreath, Y ("Flute Synth", leadEnv (3000) + Values { { "engine", 5 }, { "wave", 0.35f }, { "attack", 0.03f } }),
         { "Pan Lead", "Flute Lead", "Breath Lead" });
    add ("glidesine", catLeads, kSine, Y ("Sine Lead", leadEnv (6000, 0.32f) + Values { { "engine", 7 }, { "wave", wSine * 0.5f + wTri * 0.15f } }),
         { "Glide Sine", "Slide Lead", "Portamento Lead" });
    add ("dist", catLeads, kSaw, Y ("Distorted Saw", leadEnv (2400) + Values { { "velSens", 0.9f }, { "drive", 0.36f }, { "driveType", 2 }, { "eqHigh", -3 } } + B (0, 0.5f, 0, 0, 0.5f)),
         { "Distorted Lead", "Fuzz Lead", "Grit Lead" });
    add ("fold", catLeads, kSine, Y ("Folded Sine", leadEnv (4000) + Values { { "engine", 7 }, { "wave", wSine }, { "drive", 0.38f }, { "driveType", 4 } }),
         { "Folded Lead", "Fold Lead", "Wavefold Lead" });
    add ("fmlead", catLeads, kFM, Y ("FM Lead", leadEnv (3500) + Values { { "engine", 1 }, { "fmRatio", 1 }, { "fmAmt", 0.16f }, { "wave", 0.3f }, { "fdecay", 0.6f }, { "fsustain", 0.5f } }),
         { "FM Lead", "Digital Horn", "Brassy FM" });
    add ("voxlead", catLeads, kVox, Y ("Talk Lead", leadEnv (3000) + Values { { "engine", 3 }, { "wave", 0.25f } } + B (7, wSine, 0, 0, 0.55f)),
         { "Talk Lead", "Vowel Lead", "Mouth Lead" });
    add ("organlead", catLeads, kOrgan, Y ("Organ Lead", leadEnv (4000) + Values { { "engine", 4 }, { "wave", 0.6f }, { "chorus", 0.3f } }),
         { "Organ Lead", "Drawbar Lead", "Rock Organ Lead" });
    add ("brasslead", catLeads, kOrch, Y ("Brass Synth", leadEnv (2500) + Values { { "engine", 8 }, { "wave", 0.0f }, { "unison", 2 }, { "detune", 0.06f } }),
         { "Brass Lead", "Horn Lead", "Synth Trumpet" });
    add ("wtlead", catLeads, kWave, Y ("Wavetable Lead", leadEnv (3000) + Values { { "engine", 7 }, { "wave", wDigi }, { "lfo2Rate", 0.4f } } + MM (0, 4, 3, 0.1f)),
         { "Digital Lead", "Wavetable Lead", "Grid Lead" });
    add ("synclead", catLeads, kWave, Y ("Sync Lead", leadEnv (3000) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "fmAmt", 0.3f } } + MM (0, 1, 5, 0.25f)
                                                       + Values { { "fdecay", 0.5f } }),
         { "Sync Lead", "Tear Lead", "Hard Sync Lead" });
    add ("chip", catLeads, kSquare, Y ("Chip Lead", leadEnv (3500, 0.04f) + Values { { "wave", 0.62f }, { "crush", 0.22f }, { "lfoPitch", 0.12f }, { "lfoRate", 7.5f } }),
         { "Chip Lead", "8-Bit Lead", "Arcade Lead" });
    add ("twin", catLeads, kSaw, Y ("Twin Saw", leadEnv (3000) + B (0, 0.0f, 0, 0, 0.8f, { { "fineB", 11 } })),
         { "Twin Lead", "Detuned Lead", "Duo Lead" });
    add ("octlead", catLeads, kSquare, Y ("Octave Square", leadEnv (3000) + Values { { "wave", 0.5f } } + B (0, 0.0f, -1, 0, 0.6f)),
         { "Octave Lead", "Deep Lead", "Low Octave Lead" });
    add ("theremin", catLeads, kSine, Y ("Theremin", leadEnv (6000, 0.5f) + Values { { "engine", 7 }, { "wave", wSine }, { "lfoPitch", 0.22f }, { "lfoRate", 6.3f }, { "attack", 0.06f } }),
         { "Theremin", "Ghost Lead", "Ether Lead" });
    add ("acidlead", catLeads, kSaw, Y ("Ladder Lead", leadEnv (1600) + Values { { "filterType", 1 }, { "reso", 0.38f }, { "fenv", 0.3f }, { "fdecay", 0.3f }, { "fsustain", 0.2f } }),
         { "Rubber Lead", "Acid Lead", "Squelch Lead" });
    add ("pwmlead", catLeads, kPulse, Y ("PWM Lead", leadEnv (3500) + Values { { "wave", 0.68f }, { "lfo2Rate", 3.2f } } + MM (0, 4, 3, 0.18f)),
         { "PWM Lead", "Pulse Width Lead", "Shimmer Lead" });
    add ("softsq", catLeads, kWave, Y ("Soft Square", leadEnv (2600) + Values { { "engine", 7 }, { "wave", wTri + 0.08f } }),
         { "Soft Lead", "Ocarina Synth", "Flutey Lead" });
    add ("radio", catLeads, kSaw, Y ("Band Lead", leadEnv (1200) + Values { { "filterType", 4 }, { "reso", 0.2f }, { "wave", 0.3f } }),
         { "Radio Lead", "Telephone Lead", "Transistor Lead" });
    add ("modellead", catLeads, kModel, M ("voice.female", "Female Voice", { { "m.vowel", 2.0f }, { "m.artic", 0 }, { "m.vibrato", 0.25f }, { "m.bright", 0.35f } }),
         { "Siren Voice Lead", "Voice Lead", "Diva Lead" });
    add ("fuzzsine", catLeads, kSine, Y ("Fuzz Sine", leadEnv (3500) + Values { { "engine", 7 }, { "wave", wTri * 0.5f }, { "drive", 0.42f }, { "driveType", 3 }, { "eqHigh", -2 } }),
         { "Fuzz Sine", "Blown Lead", "Overdrive Lead" });
    add ("glasslead", catLeads, kWave, Y ("Glass Lead", leadEnv (3200, 0.14f) + Values { { "engine", 7 }, { "wave", wGlass } }), { "Glass Lead", "Crystal Lead", "Prism Lead" });
    add ("belllead", catLeads, kFM, Y ("Bell Lead", leadEnv (4000) + Values { { "engine", 1 }, { "fmRatio", 3 }, { "fmAmt", 0.12f }, { "fenv", 0 }, { "fdecay", 0.5f }, { "fsustain", 0.3f } }),
         { "Bell Lead", "Chime Lead", "Carillon Lead" });
    add ("wahlead", catLeads, kSaw, Y ("Wah Lead", leadEnv (900) + Values { { "filterType", 4 }, { "reso", 0.35f }, { "fenv", 0.5f }, { "fattack", 0.06f }, { "fdecay", 0.3f }, { "fsustain", 0.2f } }),
         { "Wah Lead", "Quack Lead", "Funk Lead" });
    add ("lowglide", catLeads, kSine, Y ("Low Glide", leadEnv (2500, 0.4f) + Values { { "octave", -1 }, { "engine", 7 }, { "wave", wTri }, { "drive", 0.2f }, { "driveType", 1 } }),
         { "Low Glide Lead", "Basement Lead", "Deep Glide" });
    add ("ohlead", catLeads, kModel, M ("voice.female", "Female Voice", { { "m.vowel", 1.0f }, { "m.artic", 0 }, { "m.vibrato", 0.35f }, { "m.bright", 0.5f } }),
         { "Oh Voice Lead", "Aria Lead", "Opera Lead" });
    add ("tenorlead", catLeads, kModel, M ("voice.male", "Male Voice", { { "m.vowel", 0.0f }, { "m.artic", 0 }, { "m.vibrato", 0.3f } }), { "Tenor Lead", "Chant Lead", "Cantor Lead" });
    add ("breathylead", catLeads, kModel, M ("voice.female", "Female Voice", { { "m.vowel", 2.0f }, { "m.artic", 2 }, { "m.vibrato", 0.2f } }), { "Breathy Lead", "Airy Voice Lead", "Sigh Lead" });
    add ("duosqsaw", catLeads, kSquare | kSaw, Y ("Square Saw", leadEnv (2800) + Values { { "wave", 0.5f } } + B (0, 0.0f, 0, 0, 0.7f, { { "fineB", -7 } })),
         { "Duo Square Saw", "Hybrid Lead", "Blend Lead" });
    add ("pipelead", catLeads, kOrgan, Y ("Flute Stop", leadEnv (5000, 0.08f) + Values { { "engine", 4 }, { "wave", 0.08f }, { "lfoPitch", 0.06f } }), { "Pipe Lead", "Flute Stop Lead", "Calliope Lead" });
    add ("pulsechip", catLeads, kPulse, Y ("Pulse Chip", leadEnv (4000, 0.03f) + Values { { "wave", 0.88f }, { "crush", 0.28f } }), { "Pulse Chip Lead", "Bleep Lead", "Pocket Lead" });
    add ("laserlead", catLeads, kWave, Y ("Laser Lead", leadEnv (2800) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "e3attack", 1.2f }, { "e3sustain", 1 } } + MM (0, 2, 5, 0.45f)),
         { "Laser Lead", "Sweep Lead", "Beam Lead" });
    add ("cleanflute", catLeads, kBreath, Y ("Clean Flute", leadEnv (4000) + Values { { "engine", 5 }, { "wave", 0.1f }, { "chorus", 0.25f }, { "attack", 0.04f } }),
         { "Clean Flute Lead", "Bamboo Lead", "Reed Pipe Lead" });

    //------------------------------------------------------------------ PADS (synth)
    add ("warm", catPads, kSaw, Y ("Warm Pad", padEnv (1850) + Values { { "unison", 5 }, { "detune", 0.15f } }),
         { "Warm Pad", "Velvet Pad", "Blanket Pad" });
    add ("shimmer", catPads, kSuper, Y ("Supersaw Pad", padEnv (2550) + Values { { "unison", 8 }, { "detune", 0.24f } } + B (0, 0.0f, 1, 0, 0.16f, { { "unisonB", 4 }, { "detuneB", 0.2f } })),
         { "Shimmer Pad", "Glow Pad", "Aurora Pad" });
    add ("pwmpad", catPads, kPulse, Y ("PWM Pad", padEnv (2200) + Values { { "wave", 0.66f }, { "unison", 3 }, { "detune", 0.1f }, { "lfo2Rate", 0.35f } } + MM (0, 4, 3, 0.2f)),
         { "PWM Pad", "Pulse Pad", "Ripple Pad" });
    add ("softpad", catPads, kWave | kSine, Y ("Triangle Pad", padEnv (8500) + Values { { "engine", 7 }, { "wave", wTri }, { "unison", 4 }, { "detune", 0.14f } }),
         { "Soft Pad", "Cotton Pad", "Feather Pad" });
    add ("glasspad", catPads, kWave, Y ("Glass Pad", padEnv (3400) + Values { { "engine", 7 }, { "wave", wGlass }, { "unison", 3 }, { "detune", 0.12f } }),
         { "Glass Pad", "Crystal Pad", "Frost Pad" });
    add ("morphpad", catPads, kWave, Y ("Morph Pad", padEnv (2700) + Values { { "engine", 7 }, { "wave", wSq }, { "unison", 3 }, { "e3attack", 3.5f }, { "e3sustain", 1 } } + MM (0, 2, 3, 0.4f)),
         { "Morph Pad", "Sweep Pad", "Wavescan Pad" });
    add ("vowelpad", catPads, kVox, Y ("Formant Pad", padEnv (4400) + Values { { "engine", 3 }, { "wave", 0.6f }, { "unison", 3 }, { "lfo2Rate", 0.13f } } + MM (0, 4, 3, 0.35f)
                                                      + B (7, wSine, 0, 0, 0.6f, { { "unisonB", 3 }, { "detuneB", 0.1f } })),
         { "Vowel Pad", "Formant Pad", "Mouth Pad" });
    add ("synthchoir", catPads, kOrch | kVox, Y ("Synth Choir", padEnv (5100) + Values { { "engine", 8 }, { "wave", 1.0f }, { "fmAmt", 0.95f }, { "unison", 4 }, { "detune", 0.12f } }
                                                         + B (7, wSine, 0, 0, 0.5f)),
         { "Synth Choir", "Vapor Choir", "Ghost Choir" });
    add ("strmachine", catPads, kOrch, Y ("String Machine", padEnv (3750, 0.5f) + Values { { "engine", 8 }, { "wave", 0.5f }, { "unison", 6 }, { "detune", 0.13f }, { "chorus", 0.4f } }),
         { "String Machine", "Ensemble Pad", "Synth Strings" });
    add ("organpad", catPads, kOrgan, Y ("Organ Pad", padEnv (5100, 0.9f) + Values { { "engine", 4 }, { "wave", 0.22f } }),
         { "Organ Pad", "Drawbar Pad", "Hymn Pad" });
    add ("breathpad", catPads, kBreath | kNoise, Y ("Breath Pad", padEnv (2550) + Values { { "engine", 5 }, { "wave", 0.85f } }),
         { "Breath Pad", "Air Pad", "Wind Pad" });
    add ("fmpad", catPads, kFM, Y ("FM Pad", padEnv (6800) + Values { { "engine", 1 }, { "fmRatio", 2 }, { "fmAmt", 0.1f }, { "unison", 3 }, { "detune", 0.1f }, { "fsustain", 0.6f } }),
         { "FM Pad", "Satin Pad", "Silk Pad" });
    add ("bellpad", catPads, kFM, Y ("Bell Pad", padEnv (6800) + Values { { "engine", 1 }, { "fmRatio", 3.5f }, { "fmAmt", 0.1f }, { "unison", 2 }, { "fsustain", 0.4f }, { "fdecay", 1.5f } }),
         { "Bell Pad", "Chime Pad", "Temple Pad" });
    add ("halo", catPads, kSine, Y ("Sine Pad", padEnv (10200) + Values { { "engine", 7 }, { "wave", wSine }, { "unison", 6 }, { "detune", 0.28f } }),
         { "Halo Pad", "Sine Choir", "Hum Pad" });
    add ("darkpad", catPads, kSaw, Y ("Ladder Pad", padEnv (800) + Values { { "filterType", 1 }, { "reso", 0.3f }, { "unison", 3 } }),
         { "Dark Pad", "Cave Pad", "Shadow Pad" });
    add ("hollowpad", catPads, kSaw, Y ("Band Pad", padEnv (1350) + Values { { "filterType", 4 }, { "reso", 0.3f }, { "unison", 3 }, { "lfoFilter", 0.25f }, { "lfoRate", 0.09f } }),
         { "Hollow Pad", "Whale Pad", "Tunnel Pad" });
    add ("syncpad", catPads, kWave, Y ("Sync Pad", padEnv (2400) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "fmAmt", 0.12f }, { "unison", 3 },
                                                     { "e3attack", 4.0f }, { "e3sustain", 1 } } + MM (0, 2, 5, 0.3f)),
         { "Sync Pad", "Rising Sync", "Laser Pad" });
    //------------------------------------------------------------------ PADS (real recordings)
    auto spad = [] (const char* inst, const char* art, const char* label, float att, float rel, float cut)
    { return S (inst, art, label, { { "s.attack", att }, { "s.release", rel }, { "s.cutoff", cut }, { "s.velSens", 0.2f } }); };
    add ("violinpad", catPads, kSampleSus, spad ("strings.violins.ensemble", "sustain", "Violin Section", 0.9f, 2.6f, 3600), { "Violin Cloud", "Silver Strings", "String Haze" });
    add ("violapad", catPads, kSampleSus, spad ("strings.violas.ensemble", "sustain", "Viola Section", 1.0f, 2.6f, 3000), { "Viola Haze", "Viola Dusk", "Amber Strings" });
    add ("cellopad", catPads, kSampleSus, spad ("strings.cellos.ensemble", "sustain", "Cello Section", 1.0f, 2.8f, 2400), { "Cello Dusk", "Cello Glow", "Low Strings" });
    add ("tremopad", catPads, kSampleSus, spad ("strings.violas.ensemble", "tremolo", "Viola Section", 1.2f, 2.4f, 2600), { "Tremolo Mist", "Shiver Strings", "Flutter Strings" });
    add ("cellotrem", catPads, kSampleSus, spad ("strings.cellos.ensemble", "tremolo", "Cello Section", 1.2f, 2.6f, 2000), { "Tremor Pad", "Rumble Strings", "Storm Strings" });
    add ("choirpad", catPads, kSampleSus, spad ("voice.choir.classic", "oohs", "Classic Choir", 0.8f, 2.6f, 2400), { "Choir Pad", "Ooh Choir", "Chapel Voices" });
    add ("aahpad", catPads, kSampleSus, spad ("voice.choir.classic", "aahs", "Classic Choir", 0.8f, 2.6f, 1500), { "Aah Choir", "Choir Cloud", "Hymn Voices" });
    add ("voxpad", catPads, kSampleSus, spad ("voice.choir.classic", "solo_vox", "Solo Choir Voice", 0.7f, 2.4f, 2600), { "Lone Voice Pad", "Solo Voice Pad", "Vigil Voice" });
    add ("pipepad", catPads, kSampleSus, spad ("organ.pipe", "soft", "Pipe Organ", 1.0f, 2.8f, 2600), { "Cathedral Pad", "Flue Pad", "Nave Pad" });
    add ("chamberpad", catPads, kSampleSus, spad ("organ.chamber", "8 foot", "Chamber Organ", 0.9f, 2.5f, 2600), { "Chapel Pad", "Vesper Pad", "Cloister Pad" });
    add ("hornpad", catPads, kSampleSus, spad ("brass.horn", "sustain", "French Horn", 1.0f, 2.5f, 1800), { "Horn Pad", "Distant Horns", "Twilight Horns" });
    add ("clarpad", catPads, kSampleSus, spad ("winds.clarinet", "sustain", "Clarinet", 0.9f, 2.4f, 2200), { "Clarinet Haze", "Reed Mist", "Woodwind Pad" });
    {
        auto g = spad ("fx.glass", "slow", "Glass Harp", 0.8f, 2.6f, 4000);
        g.octave = 1;
        add ("glassharp", catPads, kSampleSus, g, { "Glass Harp Pad", "Rim Pad", "Crystal Rim" });
    }
    add ("vocalpadm", catPads, kModel, M ("voice.pad", "Vocal Pad", { { "m.velSens", 0.4f } }), { "Breath Choir", "Angel Pad", "Soft Vocal Pad" });
    add ("brasspad", catPads, kSaw, Y ("Brass Pad", padEnv (900, 0.5f) + Values { { "unison", 4 }, { "fenv", 0.4f }, { "fattack", 1.5f }, { "fdecay", 2.0f }, { "fsustain", 0.5f } }),
         { "Brass Pad", "Analog Brass Pad", "Horizon Brass" });
    add ("bloompad", catPads, kSaw, Y ("Bloom Pad", padEnv (500) + Values { { "unison", 4 }, { "e3attack", 5.0f }, { "e3sustain", 1 }, { "e3release", 3 } } + MM (0, 2, 1, 0.45f)),
         { "Bloom Pad", "Sunrise Pad", "Opening Pad" });
    add ("digipad", catPads, kWave, Y ("Digital Pad", padEnv (3000) + Values { { "engine", 7 }, { "wave", wDigi }, { "unison", 4 } }), { "Digital Pad", "Data Pad", "Pixel Pad" });
    add ("sqpad", catPads, kSquare, Y ("Square Pad", padEnv (2400) + Values { { "wave", 0.5f }, { "unison", 2 }, { "detune", 0.08f }, { "chorus", 0.6f } }),
         { "Square Pad", "Ensemble Square", "Retro Pad" });
    add ("noisepad", catPads, kNoise, Y ("Static Pad", padEnv (900) + Values { { "engine", 1 }, { "fmRatio", 5.31f }, { "wave", 1.0f }, { "fmAmt", 0.8f }, { "fsustain", 1 }, { "filterType", 4 },
                                                                         { "reso", 0.3f } } + B (7, wSine, 0, 0, 0.8f, { { "unisonB", 3 }, { "detuneB", 0.1f } })),
         { "Rain Pad", "Static Pad", "Noise Bed" });
    add ("oohpad", catPads, kVox, Y ("Ooh Pad", padEnv (3500) + Values { { "engine", 3 }, { "wave", 1.0f }, { "unison", 4 }, { "detune", 0.1f } }), { "Ooh Pad", "Moan Pad", "Hum Choir Pad" });
    add ("bassoonpad", catPads, kSampleSus, spad ("winds.bassoon", "sustain", "Bassoon", 0.9f, 2.4f, 2000), { "Bassoon Haze", "Low Reed Pad", "Amber Reeds" });
    add ("saxpad", catPads, kSampleSus, spad ("winds.sax.tenor", "sustain", "Tenor Sax", 0.9f, 2.4f, 2000), { "Smoky Sax Pad", "Sax Haze", "Velvet Sax Pad" });
    add ("harmpad", catPads, kSampleSus, spad ("winds.harmonica.chromatic", "normal", "Chromatic Harmonica", 0.9f, 2.4f, 2200), { "Harmonica Haze", "Mouth Organ Pad", "Porch Reeds" });
    add ("fullorgpad", catPads, kSampleSus, spad ("organ.chamber", "full", "Chamber Organ", 0.9f, 2.5f, 2200), { "Full Organ Pad", "Plenum Pad", "Choir Loft Pad" });
    add ("fourpad", catPads, kSampleSus, spad ("organ.chamber", "4 foot", "Chamber Organ", 0.9f, 2.5f, 2800), { "Four Foot Pad", "High Pipe Pad", "Fife Pad" });
    add ("principalpad", catPads, kSampleSus, spad ("organ.pipe", "principal", "Pipe Organ", 1.0f, 2.8f, 2200), { "Principal Pad", "Great Pipe Pad", "Minster Pad" });
    add ("choirmodel", catPads, kModel, M ("voice.choir", "Choir", { { "m.voices", 16 }, { "m.vowel", 0.0f }, { "m.attack", 0.8f }, { "m.release", 2.0f }, { "m.velSens", 0.4f } }),
         { "Mass Choir", "Assembly Pad", "Choir Ah Pad" });
    add ("malechoir", catPads, kModel, M ("voice.choir", "Choir", { { "m.voices", 12 }, { "m.gender", 0.0f }, { "m.vowel", 1.0f }, { "m.attack", 0.9f }, { "m.release", 2.2f }, { "m.velSens", 0.4f } }),
         { "Monk Pad", "Male Choir Pad", "Abbey Pad" });
    add ("femchoir", catPads, kModel, M ("voice.choir", "Choir", { { "m.voices", 12 }, { "m.gender", 1.0f }, { "m.vowel", 2.0f }, { "m.attack", 0.9f }, { "m.release", 2.2f }, { "m.velSens", 0.4f } }),
         { "Angel Choir", "Women Choir Pad", "Halo Choir" });
    add ("vocalpadee", catPads, kModel, M ("voice.pad", "Vocal Pad", { { "m.vowel", 3.0f }, { "m.velSens", 0.4f } }), { "Bright Vocal Pad", "Ee Vocal Pad", "Seraph Pad" });

    //------------------------------------------------------------------ SYNTH (poly synths, stabs, keys)
    add ("polysaw", catSynth, kSaw, Y ("Poly Saw", polyEnv (2200) + Values { { "unison", 3 }, { "detune", 0.1f }, { "chorus", 0.3f } }),
         { "Poly Saw", "Analog Poly", "Poly Synth" });
    add ("brass", catSynth, kSaw, Y ("Synth Brass", Values { { "unison", 3 }, { "detune", 0.12f }, { "cutoff", 900 }, { "fenv", 0.45f }, { "fattack", 0.07f }, { "fdecay", 0.6f },
                                                      { "fsustain", 0.45f }, { "attack", 0.025f }, { "decay", 1 }, { "sustain", 0.85f }, { "release", 0.35f } }),
         { "Synth Brass", "Brass Poly", "Fanfare Synth" });
    add ("sqstab", catSynth, kSquare, Y ("Square Stab", polyEnv (2000, 0.35f, 0.0f, 0.2f) + Values { { "wave", 0.5f }, { "fenv", 0.35f }, { "fdecay", 0.2f } }),
         { "Square Stab", "Chord Stab", "Block Stab" });
    add ("superstab", catSynth, kSuper, Y ("Supersaw", polyEnv (2800, 0.45f, 0.0f, 0.3f) + Values { { "unison", 8 }, { "detune", 0.22f } }),
         { "Rave Stab", "Supersaw Stab", "Trance Stab" });
    add ("unikeys", catSynth, kSaw, Y ("Unison Keys", polyEnv (2500, 1.0f, 0.3f, 0.45f) + Values { { "unison", 4 }, { "detune", 0.08f }, { "wave", 0.2f } }),
         { "Synth Keys", "Unison Keys", "Neon Keys" });
    add ("fmkeys", catSynth, kFM, Y ("FM Keys", polyEnv (5000, 1.4f, 0.2f, 0.5f) + Values { { "engine", 1 }, { "fmAlgo", 5 }, { "fmRatio", 1 }, { "fmRatio2", 14 }, { "fmAmt", 0.16f },
                                                                         { "fenv", 0 }, { "fdecay", 0.6f } }),
         { "FM Poly", "Digital Keys", "Chrome Poly" });
    add ("houseorgan", catSynth, kOrgan, Y ("House Organ", polyEnv (3500, 0.4f, 0.25f, 0.15f) + Values { { "engine", 4 }, { "wave", 0.55f }, { "fenv", 0 } }),
         { "House Organ", "Stab Organ", "Club Organ" });
    add ("wtpoly", catSynth, kWave, Y ("Wavetable Poly", polyEnv (3000) + Values { { "engine", 7 }, { "wave", wGlass * 0.6f + wDigi * 0.4f }, { "unison", 3 } }),
         { "Wavetable Poly", "Digital Poly", "Crystal Poly" });
    add ("voxstab", catSynth, kVox, Y ("Formant Stab", polyEnv (2400, 0.5f, 0.1f, 0.25f) + Values { { "engine", 3 }, { "wave", 0.95f } } + B (7, wSine, 0, 0, 0.5f)),
         { "Vocal Stab", "Vocal Synth", "Formant Stab" });
    add ("orchhit", catSynth, kOrch, Y ("Orchestra Synth", polyEnv (2400, 0.5f, 0.0f, 0.5f) + Values { { "engine", 8 }, { "wave", 0.3f }, { "unison", 4 }, { "detune", 0.1f } }),
         { "Hit Synth", "Orchestra Stab", "Tutti Synth" });
    add ("syncpoly", catSynth, kWave, Y ("Sync Poly", polyEnv (2600) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "fmAmt", 0.18f }, { "unison", 2 } }),
         { "Sync Poly", "Sync Keys", "Hard Sync Poly" });
    add ("fatpoly", catSynth, kSaw, Y ("Ladder Poly", polyEnv (1300) + Values { { "filterType", 1 }, { "reso", 0.25f }, { "unison", 2 }, { "detune", 0.07f } } + B (0, 0.5f, -1, 0, 0.45f)),
         { "Fat Poly", "Ladder Poly", "Heavy Poly" });
    add ("polystr", catSynth, kOrch, Y ("String Synth", Values { { "engine", 8 }, { "wave", 0.5f }, { "unison", 5 }, { "detune", 0.12f }, { "cutoff", 2400 }, { "attack", 0.08f },
                                                          { "decay", 1 }, { "sustain", 0.9f }, { "release", 0.6f }, { "chorus", 0.35f } }),
         { "Poly Strings", "Solina Keys", "Ensemble Synth" });
    add ("pwmpoly", catSynth, kPulse, Y ("PWM Poly", polyEnv (2500) + Values { { "wave", 0.7f }, { "lfo2Rate", 1.1f }, { "unison", 2 } } + MM (0, 4, 3, 0.15f)),
         { "PWM Poly", "Pulse Poly", "Woven Poly" });
    add ("hollowpoly", catSynth, kSaw, Y ("Band Poly", polyEnv (1300, 0.8f, 0.4f, 0.4f) + Values { { "filterType", 4 }, { "reso", 0.22f }, { "unison", 2 } }),
         { "Hollow Poly", "Nasal Poly", "Horn Poly" });
    add ("sinekeys", catSynth, kSine | kFM, Y ("Sine Keys", polyEnv (6000, 1.6f, 0.15f, 0.6f) + Values { { "engine", 1 }, { "fmRatio", 2 }, { "fmAmt", 0.09f }, { "fenv", 0 } }),
         { "Sine Keys", "Soft Synth Keys", "Glass Keys Synth" });
    add ("gated", catSynth, kSaw, Y ("Gated Synth", polyEnv (2400, 1.0f, 0.9f, 0.3f) + Values { { "unison", 3 }, { "lfoSync", 1 }, { "lfoDiv", 4 }, { "lfoShape", 1 }, { "lfoAmp", 0.85f } }),
         { "Gated Chords", "Pulse Synth", "Chopped Synth" });
    add ("pump", catSynth, kSquare, Y ("Pump Synth", polyEnv (2600, 1.0f, 0.9f, 0.3f) + Values { { "wave", 0.35f }, { "unison", 3 }, { "lfoSync", 1 }, { "lfoDiv", 2 }, { "lfoShape", 0 },
                                                                       { "lfoAmp", 0.7f } }),
         { "Pumping Synth", "Sidechain Chords", "Breathing Synth" });
    add ("seq5", catSynth, kSaw, Y ("Sequence Synth", polyEnv (2300, 1.0f, 0.85f, 0.3f) + Values { { "unison", 2 }, { "lfo2Sync", 1 }, { "lfo2Div", 3 }, { "lfo2Shape", 3 } }
                                                         + MM (0, 4, 0, 7.0f / 24.0f) + Values { { "lfoSync", 1 }, { "lfoDiv", 4 }, { "lfoShape", 2 }, { "lfoFilter", 0.3f } }),
         { "Fifth Sequence", "Two Step Synth", "Fifth Bounce" });
    add ("seqoct", catSynth, kSquare, Y ("Sequence Synth", polyEnv (2400, 1.0f, 0.85f, 0.3f) + Values { { "wave", 0.5f }, { "lfo2Sync", 1 }, { "lfo2Div", 4 }, { "lfo2Shape", 3 } }
                                                            + MM (0, 4, 0, -0.5f)),
         { "Octave Bounce", "Low Bounce", "Hop Synth" });
    add ("epsynth", catSynth, kModel, M ("ep.tine.mellow", "Tine Electric Piano", { { "m.tone", 0.45f } }), { "EP Synth", "Tine Synth", "Glow Keys" });
    add ("clavsynth", catSynth, kModel, M ("ep.clav", "Funk Keys", { { "m.tone", 0.45f } }), { "Funk Synth", "Clav Synth", "Strut Synth" });
    add ("fmbrass", catSynth, kFM, Y ("FM Brass", polyEnv (5000, 1.0f, 0.8f, 0.3f) + Values { { "engine", 1 }, { "fmRatio", 1 }, { "fmAmt", 0.22f }, { "fenv", 0 }, { "fattack", 0.08f },
                                                                         { "fdecay", 0.5f }, { "fsustain", 0.6f } }),
         { "FM Brass", "Digital Brass", "Steel Brass" });
    add ("chorussq", catSynth, kSquare, Y ("Square Poly", polyEnv (2400) + Values { { "wave", 0.5f }, { "unison", 2 }, { "detune", 0.06f }, { "chorus", 0.6f } }),
         { "Chorus Poly", "Square Poly", "Retro Poly" });
    add ("chippoly", catSynth, kSquare, Y ("Chip Poly", polyEnv (4000, 0.5f, 0.3f, 0.2f) + Values { { "wave", 0.55f }, { "crush", 0.25f }, { "fenv", 0 } }), { "Chip Poly", "Pixel Keys", "Bit Poly" });
    add ("glasspoly", catSynth, kWave, Y ("Glass Poly", polyEnv (3000, 1.2f, 0.3f, 0.5f) + Values { { "engine", 7 }, { "wave", wGlass }, { "unison", 2 } }), { "Glass Keys Poly", "Ice Poly", "Frost Keys" });
    add ("sawchoir", catSynth, kSaw | kVox, Y ("Saw Choir", polyEnv (2200, 1.0f, 0.8f, 0.4f) + Values { { "unison", 3 } } + B (3, 0.0f, 0, 0, 0.8f, { { "unisonB", 3 } })),
         { "Choir Synth", "Saw Choir", "Chorale Synth" });
    add ("organsaw", catSynth, kOrgan | kSaw, Y ("Organ Synth", polyEnv (3000, 0.8f, 0.7f, 0.2f) + Values { { "engine", 4 }, { "wave", 0.5f } } + B (0, 0.0f, 0, 0, 0.45f)),
         { "Organ Synth", "Combo Synth", "Bar Organ Synth" });
    add ("detkeys", catSynth, kSaw, Y ("Detuned Keys", polyEnv (2400, 1.2f, 0.3f, 0.4f) + Values { { "unison", 2 }, { "detune", 0.32f }, { "wave", 0.15f } } + MM (0, 5, 1, 0.3f)),
         { "Detuned Keys", "Seasick Keys", "Warped Poly" });
    add ("stackfm", catSynth, kFM, Y ("Stack FM", polyEnv (6000, 1.3f, 0.2f, 0.5f) + Values { { "engine", 1 }, { "fmAlgo", 1 }, { "fmRatio", 1 }, { "fmRatio2", 2 }, { "fmAmt", 0.14f }, { "fenv", 0 } }),
         { "Stack FM Poly", "Tower Keys", "Layer FM Keys" });
    add ("pulsestr", catSynth, kPulse, Y ("Pulse Strings", Values { { "wave", 0.7f }, { "unison", 6 }, { "detune", 0.12f }, { "cutoff", 2600 }, { "attack", 0.15f }, { "decay", 1 },
                                                                   { "sustain", 0.9f }, { "release", 0.7f }, { "chorus", 0.4f } }),
         { "Pulse Strings", "Vintage Strings", "String Ensemble Synth" });
    add ("reedsynth", catSynth, kPulse, Y ("Reed Synth", polyEnv (2200, 1.0f, 0.85f, 0.25f) + Values { { "wave", 0.85f }, { "filterType", 4 }, { "reso", 0.15f }, { "unison", 2 }, { "detune", 0.05f } }),
         { "Reed Synth", "Musette Synth", "Bellows Synth" });

    //------------------------------------------------------------------ FX & ATMOS
    add ("riser", catFxAtmos, kRiser | kSuper, Y ("Riser", Values { { "unison", 6 }, { "detune", 0.2f }, { "cutoff", 500 }, { "attack", 1.2f }, { "decay", 1 }, { "sustain", 1 }, { "release", 1.5f },
                                                             { "e3attack", 4.0f }, { "e3sustain", 1 }, { "e3release", 1.5f }, { "velSens", 0.6f } } + MM (0, 2, 0, 1.0f) + MM (1, 2, 1, 0.45f)),
         { "Riser", "Ascension", "Lift Off" });
    add ("noiserise", catFxAtmos, kRiser | kNoise, Y ("Noise Sweep", Values { { "engine", 1 }, { "fmRatio", 5.31f }, { "wave", 1.0f }, { "fmAmt", 0.85f }, { "fenv", 0 }, { "fsustain", 1 }, { "filterType", 4 }, { "reso", 0.35f }, { "cutoff", 400 }, { "attack", 1.5f },
                                                                    { "decay", 1 }, { "sustain", 1 }, { "release", 1.6f }, { "e3attack", 4.5f }, { "e3sustain", 1 }, { "e3release", 1.5f },
                                                                    { "velSens", 0.6f } } + MM (0, 2, 1, 0.42f)),
         { "Noise Sweep", "White Rise", "Vapor Rise" });
    add ("fall", catFxAtmos, kRiser | kSaw, Y ("Downlifter", Values { { "unison", 4 }, { "detune", 0.15f }, { "cutoff", 1600 }, { "attack", 0.01f }, { "decay", 3.5f }, { "sustain", 0 },
                                                               { "release", 1.2f }, { "fattack", 0.001f }, { "fdecay", 2.5f }, { "fsustain", 0 } } + MM (0, 1, 0, 1.0f) + MM (1, 1, 1, 0.25f)),
         { "Downlifter", "Descent", "Falling Star" });
    add ("drone", catFxAtmos, kDrone | kSaw, Y ("Drone", Values { { "octave", -1 }, { "unison", 5 }, { "detune", 0.1f }, { "cutoff", 600 }, { "reso", 0.2f }, { "attack", 1.5f }, { "sustain", 1 },
                                                            { "release", 3 }, { "lfoFilter", 0.25f }, { "lfoRate", 0.07f }, { "lfo2Rate", 0.05f } } + MM (0, 4, 12, 0.3f)),
         { "Drone", "Abyss Drone", "Monolith" });
    add ("organdrone", catFxAtmos, kDrone | kOrgan, Y ("Organ Drone", Values { { "engine", 4 }, { "wave", 0.4f }, { "octave", -1 }, { "cutoff", 1400 }, { "attack", 2.0f }, { "sustain", 1 },
                                                                       { "release", 3 }, { "lfoAmp", 0.15f }, { "lfoRate", 0.18f } } + B (4, 0.2f, -1, 7, 0.5f)),
         { "Ritual Drone", "Fifth Drone", "Temple Drone" });
    add ("texture", catFxAtmos, kWave, Y ("Grain Texture", Values { { "engine", 7 }, { "wave", wDigi }, { "unison", 3 }, { "cutoff", 1500 }, { "attack", 0.8f }, { "sustain", 1 }, { "release", 2.5f },
                                                              { "crush", 0.3f }, { "wow", 0.3f }, { "lfoShape", 4 }, { "lfoRate", 6 }, { "lfoFilter", 0.35f }, { "lfo2Rate", 0.2f } } + MM (0, 4, 3, 0.3f)),
         { "Grain Cloud", "Texture", "Static Bloom" });
    add ("scifi", catFxAtmos, kFM, Y ("Data Stream", Values { { "engine", 1 }, { "fmRatio", 2.5f }, { "fmAmt", 0.15f }, { "cutoff", 2500 }, { "attack", 0.05f }, { "sustain", 1 }, { "release", 1 },
                                                        { "lfoShape", 4 }, { "lfoRate", 9 }, { "lfoPitch", 0.35f } }),
         { "Data Stream", "Robot Chatter", "Computer Talk" });
    add ("wind", catFxAtmos, kNoise | kBreath, Y ("Wind", Values { { "engine", 1 }, { "fmRatio", 7.13f }, { "wave", 1.0f }, { "fmAmt", 0.9f }, { "fsustain", 1 }, { "filterType", 4 }, { "reso", 0.5f }, { "cutoff", 700 }, { "attack", 1.2f }, { "sustain", 1 },
                                                           { "release", 2.5f }, { "lfoFilter", 0.4f }, { "lfoRate", 0.15f }, { "lfo2Rate", 0.4f } } + MM (0, 4, 9, 0.25f)),
         { "Desert Wind", "Wind", "Gust" });
    add ("zap", catFxAtmos, kHit | kSine, Y ("Laser", Values { { "engine", 7 }, { "wave", wTri }, { "cutoff", 5000 }, { "attack", 0.002f }, { "decay", 0.5f }, { "sustain", 0 }, { "release", 0.3f },
                                                       { "fdecay", 0.15f } } + MM (0, 1, 0, 1.0f) + MM (1, 1, 0, 1.0f)),
         { "Laser", "Zap", "Ray Blaster" });
    add ("impact", catFxAtmos, kHit | kSine, Y ("Impact", Values { { "engine", 7 }, { "wave", wSine }, { "octave", -2 }, { "cutoff", 3000 }, { "attack", 0.002f }, { "decay", 2.5f },
                                                            { "sustain", 0 }, { "release", 2 }, { "fdecay", 0.12f } } + MM (0, 1, 0, 1.0f) + B (5, 1.0f, 0, 0, 0.5f)),
         { "Impact", "Boom Hit", "Thunder Hit" });
    add ("braam", catFxAtmos, kHit | kOrch, Y ("Braam", Values { { "engine", 8 }, { "wave", 0.0f }, { "octave", -1 }, { "unison", 6 }, { "detune", 0.1f }, { "cutoff", 900 }, { "attack", 0.05f },
                                                          { "decay", 2.5f }, { "sustain", 0.3f }, { "release", 2 }, { "drive", 0.25f }, { "driveType", 1 } } + B (8, 0.0f, -1, 0, 0.7f)),
         { "Braam", "Doom Horn", "Titan Horn" });
    add ("metal", catFxAtmos, kModal, Y ("Metal Resonator", Values { { "engine", 9 }, { "wave", 0.9f }, { "fmAmt", 0.5f }, { "octave", -1 }, { "cutoff", 3500 }, { "decay", 4 }, { "sustain", 1 },
                                                              { "release", 3 }, { "body", 4 }, { "bodyMix", 0.4f } }),
         { "Gong Swell", "Metal Drone", "Iron Bell" });
    add ("wail", catFxAtmos, kSine, Y ("Wail", Values { { "engine", 7 }, { "wave", wSine }, { "cutoff", 4000 }, { "attack", 0.6f }, { "sustain", 1 }, { "release", 2 }, { "lfoShape", 1 },
                                                 { "lfoRate", 0.35f }, { "lfoPitch", 0.5f } }),
         { "Wail", "Distant Siren", "Ghost Call" });
    add ("heart", catFxAtmos, kSine | kHit, Y ("Pulse", Values { { "engine", 7 }, { "wave", wSine }, { "octave", -1 }, { "cutoff", 2000 }, { "attack", 0.01f }, { "sustain", 1 }, { "release", 1 },
                                                          { "lfoSync", 1 }, { "lfoDiv", 2 }, { "lfoShape", 0 }, { "lfoAmp", 0.9f } }),
         { "Heartbeat", "Pulse Field", "Low Throb" });
    // reversed real instruments: a slow attack over the decaying recording + reversed 1/8 grains + a big space
    auto rev = [] (const char* inst, const char* art, const char* label, float att, float cut)
    { return S (inst, art, label, { { "s.attack", att }, { "s.release", 0.25f }, { "s.cutoff", cut }, { "s.velSens", 0.25f }, { "reverse", 0.6f } }); };
    add ("revpiano", catFxAtmos, kSampleShot, rev ("piano.grand.concert", "sustain", "Concert Grand", 1.3f, 3500), { "Reversed Piano", "Backward Grand", "Rewind Piano" });
    add ("revharp", catFxAtmos, kSampleShot, rev ("pluck.harp.concert", "pluck", "Concert Harp", 1.0f, 3500), { "Reversed Harp", "Backward Harp", "Rewind Harp" });
    add ("revpizz", catFxAtmos, kSampleShot, rev ("strings.cellos.ensemble", "pizzicato", "Cello Section", 0.8f, 3000), { "Reversed Pizz", "Backward Pluck", "Suck Pizz" });
    add ("revmarimba", catFxAtmos, kSampleShot, rev ("mallets.marimba", "hit", "Marimba", 0.7f, 3500), { "Reversed Marimba", "Backward Wood", "Inward Marimba" });
    add ("revvibes", catFxAtmos, kSampleShot, rev ("mallets.vibraphone", "soft mallets", "Vibraphone", 1.0f, 3500), { "Reversed Vibes", "Backward Vibes", "Mirror Vibes" });
    add ("revchoir", catFxAtmos, kSampleSus, rev ("voice.choir.classic", "aahs", "Classic Choir", 1.8f, 2200), { "Reversed Choir", "Backward Voices", "Inhale Choir" });
    add ("revbell", catFxAtmos, kSampleShot, rev ("bells.tubular", "hit", "Tubular Bells", 1.4f, 3200), { "Reversed Bell", "Backward Bell", "Bell Inhale" });
    add ("revkalimba", catFxAtmos, kSampleShot, rev ("world.kalimba.low", "pluck", "Low Thumb Piano", 0.8f, 3500), { "Reversed Thumb Piano", "Backward Tines", "Inward Tines" });
    {
        auto g = S ("fx.glass", "fast", "Glass Harp", { { "s.attack", 0.4f }, { "s.release", 3.0f }, { "s.cutoff", 3500.0f }, { "s.velSens", 0.25f } });
        g.octave = 1;
        add ("glassatmos", catFxAtmos, kSampleSus, g, { "Glass Atmosphere", "Singing Rims", "Crystal Air" });
    }
    add ("tension", catFxAtmos, kSampleSus, S ("strings.violins.ensemble", "tremolo", "Violin Section", { { "s.attack", 2.5f }, { "s.release", 2.5f }, { "s.cutoff", 3000.0f },
                                                                                                      { "s.velSens", 0.25f } }),
         { "Tension Rise", "Nerve Strings", "Suspense Swell" });
    add ("timproll", catFxAtmos, kSampleSus, [] { auto t = S ("perc.timpani", "roll", "Timpani", { { "s.attack", 1.8f }, { "s.release", 2.0f }, { "s.velSens", 0.3f } }); t.octave = -1; return t; }(),
         { "Thunder Roll", "Timpani Swell", "Storm Roll" });
    add ("whisper", catFxAtmos, kModel, M ("voice.female", "Female Voice", { { "m.artic", 3 }, { "m.vowel", 1.0f } }), { "Whisper Air", "Haunted Breath", "Ghost Whisper" });
    add ("revguitar", catFxAtmos, kModel, M ("guitar.nylon", "Nylon Guitar", { { "m.tone", 0.4f }, { "reverse", 0.7f } }), { "Reversed Guitar", "Backward Nylon", "Rewind Strings" });
    add ("revharpsi", catFxAtmos, kSampleShot, rev ("keys.harpsichord.flemish", "lower 8'", "Flemish Harpsichord", 1.0f, 3500), { "Reversed Harpsichord", "Backward Quills", "Rewind Court" });
    add ("revchimes", catFxAtmos, kSampleShot, rev ("bells.handchimes", "hit", "Hand Chimes", 1.2f, 3500), { "Reversed Chimes", "Backward Chimes", "Chime Inhale" });
    add ("revglock", catFxAtmos, kSampleShot, [&] { auto g = rev ("mallets.glockenspiel", "hit", "Glockenspiel", 0.9f, 4000); g.octave = 2; return g; }(),
         { "Reversed Chime Bars", "Backward Sparkle", "Twinkle Inhale" });
    add ("revdulc", catFxAtmos, kSampleShot, rev ("pluck.dulcimer", "pluck", "Stick Dulcimer", 0.9f, 3500), { "Reversed Dulcimer", "Backward Folk", "Rewind Dulcimer" });
    add ("pedaldrone", catFxAtmos, kDrone | kSampleSus, [] { auto t = S ("organ.pipe", "pedal", "Pipe Organ", { { "s.attack", 2.0f }, { "s.release", 3.0f }, { "s.velSens", 0.25f } }); t.octave = -1; return t; }(),
         { "Pedal Drone", "Crypt Drone", "Undercroft Drone" });
    add ("didgdrone", catFxAtmos, kDrone | kSampleSus, [] { auto t = S ("world.didgeridoo", "drone", "Didgeridoo", { { "s.attack", 1.0f }, { "s.release", 2.5f }, { "s.velSens", 0.25f } }); t.octave = -2; return t; }(),
         { "Earth Drone", "Didgeridoo Drone", "Outback Drone" });
    add ("choirdrone", catFxAtmos, kDrone | kModel, [] { auto t = M ("voice.choir", "Choir", { { "m.voices", 12 }, { "m.gender", 0.3f }, { "m.vowel", 2.0f }, { "m.attack", 1.5f }, { "m.release", 3.0f } }); t.octave = -1; return t; }(),
         { "Choir Drone", "Requiem Drone", "Vigil Drone" });
    add ("bubbles", catFxAtmos, kSine, Y ("Bubbles", Values { { "engine", 7 }, { "wave", wSine }, { "cutoff", 1500 }, { "attack", 0.02f }, { "sustain", 1 }, { "release", 1 }, { "lfoShape", 4 },
                                                             { "lfoRate", 11 }, { "lfoPitch", 0.35f }, { "lfoAmp", 0.3f } }),
         { "Bubbles", "Underwater Bubbles", "Fizz" });
    add ("storm", catFxAtmos, kNoise, Y ("Storm", Values { { "engine", 1 }, { "fmRatio", 7.13f }, { "wave", 1.0f }, { "fmAmt", 0.95f }, { "fsustain", 1 }, { "octave", -1 }, { "cutoff", 350 },
                                                         { "attack", 1.5f }, { "sustain", 1 }, { "release", 3 }, { "lfoAmp", 0.4f }, { "lfoRate", 0.2f } }),
         { "Distant Thunder", "Storm", "Rumble" });
    add ("sqriser", catFxAtmos, kRiser | kSquare, Y ("Square Riser", Values { { "wave", 0.5f }, { "unison", 3 }, { "cutoff", 800 }, { "attack", 1.0f }, { "sustain", 1 }, { "release", 1.2f },
                                                                       { "e3attack", 3.5f }, { "e3sustain", 1 }, { "e3release", 1.2f } } + MM (0, 2, 0, 1.0f) + MM (1, 2, 1, 0.3f)),
         { "Square Riser", "Climb", "Ladder Up" });
    add ("fmriser", catFxAtmos, kRiser | kFM, Y ("FM Riser", Values { { "engine", 1 }, { "fmRatio", 2 }, { "fmAmt", 0.05f }, { "fenv", 0 }, { "fsustain", 1 }, { "cutoff", 3000 }, { "attack", 1.0f },
                                                                 { "sustain", 1 }, { "release", 1.2f }, { "e3attack", 4.0f }, { "e3sustain", 1 } } + MM (0, 2, 5, 0.4f) + MM (1, 2, 0, 0.5f)),
         { "Tension Riser", "Metal Riser", "Pressure Rise" });
    add ("syncrise", catFxAtmos, kRiser | kWave, Y ("Sync Riser", Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "cutoff", 1800 }, { "attack", 0.8f }, { "sustain", 1 },
                                                                     { "release", 1.2f }, { "e3attack", 4.0f }, { "e3sustain", 1 } } + MM (0, 2, 5, 0.8f)),
         { "Sync Rise", "Tearing Rise", "Engine Rise" });
    add ("beacon", catFxAtmos, kSquare, Y ("Beacon", Values { { "wave", 0.5f }, { "cutoff", 1800 }, { "attack", 0.01f }, { "sustain", 1 }, { "release", 0.6f }, { "lfoSync", 1 }, { "lfoDiv", 3 },
                                                           { "lfoShape", 1 }, { "lfoAmp", 0.85f }, { "delayMix", 0.25f }, { "delayTime", 2 }, { "delayFb", 0.4f } }),
         { "Beacon", "Signal Pulse", "Morse Call" });
    add ("spacehit", catFxAtmos, kHit | kNoise, Y ("Space Hit", Values { { "engine", 1 }, { "fmRatio", 5.31f }, { "wave", 1.0f }, { "fmAmt", 0.9f }, { "fsustain", 1 }, { "cutoff", 1200 },
                                                                   { "attack", 0.004f }, { "decay", 0.5f }, { "sustain", 0 }, { "release", 0.4f } } + B (7, wSine, 0, 0, 0.6f)),
         { "Space Hit", "Cosmic Hit", "Star Burst" });
    add ("glitch", catFxAtmos, kWave, Y ("Glitch", Values { { "engine", 7 }, { "wave", wDigi }, { "cutoff", 2200 }, { "attack", 0.01f }, { "sustain", 1 }, { "release", 0.8f }, { "lfoShape", 4 },
                                                          { "lfoSync", 1 }, { "lfoDiv", 4 }, { "crush", 0.3f } } + MM (0, 3, 3, 0.6f) + MM (1, 3, 1, 0.3f)),
         { "Glitch Field", "Data Rain", "Broken Signal" });
    add ("shimdrone", catFxAtmos, kDrone | kWave, Y ("Shimmer Drone", Values { { "engine", 7 }, { "wave", wGlass }, { "unison", 4 }, { "detune", 0.18f }, { "cutoff", 2500 }, { "attack", 2.0f },
                                                                          { "sustain", 1 }, { "release", 3 }, { "lfoFilter", 0.2f }, { "lfoRate", 0.1f } } + B (7, wSine, 1, 0, 0.15f)),
         { "Shimmer Drone", "Halo Drone", "Starlight Drone" });
    add ("rumble", catFxAtmos, kDrone | kSine, Y ("Rumble", Values { { "engine", 7 }, { "wave", wTri }, { "octave", -2 }, { "cutoff", 600 }, { "attack", 1.0f }, { "sustain", 1 }, { "release", 2.5f },
                                                                { "lfoAmp", 0.35f }, { "lfoRate", 0.3f }, { "drive", 0.3f }, { "driveType", 1 } }),
         { "Low Rumble", "Deep Earth", "Cavern Rumble" });
    return v;
}

//==============================================================================
// treatments: processing roles
struct Treat
{
    const char* tag;
    juce::StringArray adjectives;
    uint32_t cats;                            // bit per category
    uint32_t kinds;                           // the voice kinds it suits
    std::vector<const char*> spaces;          // the spaces it is made with (first = best)
    std::function<void (VaultPreset&)> apply;
};
constexpr uint32_t C (int c) { return 1u << c; }
constexpr uint32_t cPl = 1u << catPlucks, cLe = 1u << catLeads, cPa = 1u << catPads, cSy = 1u << catSynth, cFx = 1u << catFxAtmos;

VaultPart& P0 (VaultPreset& p) { return p.parts.front(); }
// a synth-part second oscillator copy of osc A (octave / semitone / level), for synth voices only
void addOscB (VaultPart& a, int octave, int semi, float level, float fine = 0)
{
    setAll (a, { { "layerB", 1 }, { "engineB", getv (a, "engine", 0) }, { "waveB", getv (a, "wave", 0) }, { "octaveB", (float) octave }, { "semiB", (float) semi },
                 { "fineB", fine }, { "levelB", level }, { "fmAmtB", getv (a, "fmAmt", 0) }, { "fmRatioB", getv (a, "fmRatio", 1) },
                 { "fmRatio2B", getv (a, "fmRatio2", 1) }, { "fmAlgoB", getv (a, "fmAlgo", 0) }, { "warpModeB", getv (a, "warpMode", 0) },
                 { "unisonB", std::min (4.0f, getv (a, "unison", 1)) }, { "detuneB", getv (a, "detune", 0.15f) } });
}

std::vector<Treat> treatments()
{
    std::vector<Treat> t;
    // natural: as designed, heard in different spaces. Measured (first full run): modulation-only roles (tremolo, vibrato,
    // phaser, flanger, slow LFO drift), resonant bodies, tape wobble, scoops and second spaces almost never reach the
    // distinctness threshold - they are parameter jitter, so they are not made. Every role below changes the sound.
    t.push_back ({ "natural", { "" }, cPl | cLe | cPa | cSy | cFx, kAll, { "room", "hall", "echo", "cloud" }, [] (VaultPreset&) {} });
    //---------------------------------------------------------------- plucks / synth
    t.push_back ({ "short", { "Tight", "Staccato", "Short", "Clipped" }, cPl | cSy, kSynthAll | kSampleShot | kModel, { "dry" }, [] (VaultPreset& p)
    { decayScale (P0 (p), 0.42f); } });
    t.push_back ({ "long", { "Ringing", "Long", "Lingering", "Singing" }, cPl | cSy, kSynthAll | kSampleShot | kModel, { "hall" }, [] (VaultPreset& p)
    { decayScale (P0 (p), 2.2f); } });
    t.push_back ({ "muted", { "Muted", "Dusky", "Velvet", "Hushed" }, cPl | cSy | cLe | cPa, kAll, { "room" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        lowPass (a, currentLp (a) * 0.45f);
        velSens (a, std::min (1.0f, getVelSens (a) + 0.15f));
        fx (p, { { "eqLow", 1.5f }, { "eqHigh", -2.5f } });
    } });
    t.push_back ({ "echo", { "Echo", "Bouncing", "Ricochet", "Repeating" }, cPl | cLe | cSy, kAll, { "echo" }, [] (VaultPreset& p)
    { lowPass (P0 (p), currentLp (P0 (p)) * 0.75f); } });
    t.push_back ({ "lofi", { "Lo-Fi", "Dusty", "Crumbled", "Bedroom" }, cPl | cLe | cPa | cSy | cFx, kAll, { "room" }, [] (VaultPreset& p)
    {
        lowPass (P0 (p), 3200.0f);
        fx (p, { { "crush", 0.2f }, { "wow", 0.3f }, { "drive", 0.15f }, { "driveType", 1 }, { "width", 0.32f }, { "eqLow", 1.5f } });
    } });
    t.push_back ({ "wide", { "Wide", "Silk", "Liquid", "Panoramic" }, cPl | cLe | cPa | cSy, kAll, { "hall" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        if (isSynth (a) && getv (a, "unison", 1) < 2 && getv (a, "engine", 0) != 2 && getv (a, "engine", 0) != 9) setAll (a, { { "unison", 3 }, { "detune", 0.1f } });
        fx (p, { { "chorus", 0.5f }, { "width", 0.85f } });
    } });
    t.push_back ({ "octbody", { "Deep", "Doubled", "Thick", "Heavy" }, cPl | cLe | cSy | cPa, kSynthAll & ~kModal, { "room" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        if (getv (a, "layerB", 0) > 0.5f) setv (a, "levelB", 0.0f);
        addOscB (a, -1, 0, 0.55f);
    } });
    t.push_back ({ "fifth", { "Fifth", "Power", "Open", "Quintal" }, cPl | cSy | cPa | cLe, kSynthAll & ~kModal, { "room" }, [] (VaultPreset& p)
    { addOscB (P0 (p), 0, 7, 0.38f); } });
    t.push_back ({ "octup", { "Shining", "Overtone", "Sparkling", "Lifted" }, cPl | cLe | cPa | cSy, kSynthAll & ~kModal & ~kString, { "hall" }, [] (VaultPreset& p)
    { addOscB (P0 (p), 1, 0, 0.2f); } });
    t.push_back ({ "reverse", { "Reversed", "Backward", "Rewind", "Mirror" }, cPl | cPa | cSy | cFx, kAll, { "hall" }, [] (VaultPreset& p)
    {
        lowPass (P0 (p), currentLp (P0 (p)) * 0.7f);
        fx (p, { { "reverse", 0.55f } });
    } });
    t.push_back ({ "drive", { "Gritty", "Driven", "Burning", "Hot" }, cPl | cLe | cSy | cFx, kAll, { "room" }, [] (VaultPreset& p)
    {
        fx (p, { { "drive", 0.36f }, { "driveType", 0 }, { "eqHigh", -2.5f } });
        lowPass (P0 (p), currentLp (P0 (p)) * 0.6f);
    } });
    t.push_back ({ "bright", { "Bright", "Open", "Clear", "Crisp" }, cPl | cLe | cPa | cSy, kAll, { "room" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        scaleCut (a, 2.2f);
        if (isSample (a)) setv (a, "s.cutoff", 20000.0f);
        fx (p, { { "master", 0.25f } });
    } });
    t.push_back ({ "stab", { "Stabbing", "Punchy", "Struck", "Snapped" }, cSy | cPl, kSynthAll, { "room" }, [] (VaultPreset& p)
    {
        setAll (P0 (p), { { "decay", 0.28f }, { "sustain", 0.0f }, { "release", 0.2f }, { "fenv", 0.35f }, { "fdecay", 0.12f }, { "fsustain", 0 } });
        fx (p, { { "punchFx", 0.4f } });
    } });
    //---------------------------------------------------------------- leads
    t.push_back ({ "swell", { "Swelling", "Breathy", "Rising", "Bowed" }, cLe | cSy, kAll, { "hall" }, [] (VaultPreset& p)
    { attack (P0 (p), 0.18f); release (P0 (p), 0.6f); } });
    t.push_back ({ "hollow", { "Hollow", "Nasal", "Honky", "Pinched" }, cLe | cSy | cPl, kAnalog | kWave | kOrch, { "room" }, [] (VaultPreset& p)
    { setAll (P0 (p), { { "filterType", 4 }, { "reso", 0.22f } }); scaleCut (P0 (p), 1.3f); } });
    //---------------------------------------------------------------- pads / fx
    t.push_back ({ "dark", { "Dark", "Nocturnal", "Midnight", "Shadowed" }, cPa | cFx | cSy | cLe, kAll, { "hall" }, [] (VaultPreset& p)
    {
        lowPass (P0 (p), currentLp (P0 (p)) * 0.4f);
        fx (p, { { "eqLow", 2.0f }, { "eqHigh", -3.0f } });
    } });
    t.push_back ({ "airy", { "Airy", "Ethereal", "Floating", "Weightless" }, cPa | cFx, kAll, { "vast" }, [] (VaultPreset& p)
    { fx (p, { { "eqLow", -3.0f }, { "width", 0.8f } }); attack (P0 (p), 1.2f); } });
    t.push_back ({ "underwater", { "Submerged", "Sunken", "Underwater", "Deep Sea" }, cPa | cFx | cPl | cSy, kAll, { "cloud" }, [] (VaultPreset& p)
    { lowPass (P0 (p), 600.0f); fx (p, { { "chorus", 0.5f }, { "wow", 0.18f }, { "eqLow", 1.0f }, { "width", 0.7f } }); } });
    t.push_back ({ "gatedpad", { "Pulsing", "Rhythmic", "Gated", "Throbbing" }, cPa | cFx, kSynthAll, { "hall" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        setAll (a, { { "lfoSync", 1 }, { "lfoDiv", 4 }, { "lfoShape", 1 }, { "lfoAmp", 0.9f }, { "lfoPitch", 0.0f } });
        scaleCut (a, 1.6f);
        fx (p, { { "delayMix", 0.2f }, { "delayTime", 2 }, { "delayFb", 0.35f } });
    } });
    return t;
}

//==============================================================================
// layer recipes (a second / third / fourth part). `level` relative to part 0 after both are calibrated.
struct Layer
{
    const char* tag;
    juce::StringArray words;            // name words (prefix, or "& Noun" suffix)
    uint32_t cats;
    uint32_t kinds;                     // main voice kinds it suits
    const char* spaceTag;
    std::function<void (VaultPreset&)> add;
};
// layers below / at the played pitch are mixed clearly audible (1.4 x the table level, max 0.85) so a layer changes the
// sound; octave-up layers keep their table level (>= 12 dB under part 0)
VaultPart at (VaultPart p, float level, float pan = 0.0f, int octave = 0)
{
    p.level = octave > 0 ? level : std::min (0.85f, level * 1.4f);
    p.pan = pan; p.octave = octave;
    return p;
}
VaultPart sinePad (const char* label = "Sine Pad")
{ return Y (label, padEnv (5000, 0.6f, 2.2f) + Values { { "engine", 7 }, { "wave", wSine }, { "unison", 3 }, { "detune", 0.12f }, { "lfoFilter", 0 } }); }
VaultPart warmPadL() { return Y ("Warm Pad", padEnv (900) + Values { { "unison", 4 }, { "detune", 0.14f } }); }
VaultPart airPadL() { return Y ("Air Pad", padEnv (1600, 0.9f, 2.4f) + Values { { "engine", 5 }, { "wave", 0.8f } }); }
VaultPart glassPadL() { return Y ("Glass Pad", padEnv (2200, 0.8f, 2.4f) + Values { { "engine", 7 }, { "wave", wGlass }, { "unison", 3 } }); }
VaultPart subSine() { return Y ("Sine Bass", Values { { "engine", 7 }, { "wave", wSine }, { "attack", 0.004f }, { "decay", 0.5f }, { "sustain", 0.6f }, { "release", 0.3f }, { "cutoff", 2000 } }); }
VaultPart sawPluckL() { return Y ("Saw Pluck", pluckEnv (900, 0.4f, 0.16f, 0.6f)); }
VaultPart sampleShot (const char* inst, const char* art, const char* label, float cut = 4500)
{ return S (inst, art, label, { { "s.cutoff", cut }, { "s.velSens", 0.35f } }); }
VaultPart sampleSus (const char* inst, const char* art, const char* label, float att, float rel, float cut)
{ return S (inst, art, label, { { "s.attack", att }, { "s.release", rel }, { "s.cutoff", cut }, { "s.velSens", 0.22f } }); }

std::vector<Layer> layers()
{
    std::vector<Layer> L;
    auto push = [&] (VaultPreset& p, VaultPart part) { if ((int) p.parts.size() < maxParts) p.parts.push_back (std::move (part)); };
    //---------------------------------------------------------------- plucks
    L.push_back ({ "pizzhyb", { "Pizz", "Hybrid", "Bowstring" }, cPl | cSy, kAnalog | kWave | kFM | kString, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("strings.violins.ensemble", "pizzicato", "Violin Section"), 0.55f, 0.15f)); } });
    L.push_back ({ "harphyb", { "& Harp", "Harpstring", "Angelic" }, cPl, kFM | kWave | kSine | kString | kModal, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("pluck.harp.concert", "pluck", "Concert Harp", 4000), 0.5f, -0.15f)); } });
    L.push_back ({ "marhyb", { "& Marimba", "Rosewood", "Mallet" }, cPl | cSy, kFM | kSaw | kSquare | kWave | kOrgan, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("mallets.marimba", "hit", "Marimba"), 0.5f, 0.1f)); } });
    L.push_back ({ "kalhyb", { "& Thumb Piano", "Thumb", "Tine" }, cPl, kSine | kWave | kSquare | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("world.kalimba.low", "pluck", "Low Thumb Piano"), 0.5f, -0.1f)); } });
    L.push_back ({ "vibhyb", { "& Vibes", "Vibe", "Lounge" }, cPl | cSy, kFM | kSine | kWave | kModel, "plate",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("mallets.vibraphone", "soft mallets", "Vibraphone", 4000), 0.45f, 0.15f)); } });
    L.push_back ({ "guitarhyb", { "& Nylon", "Spanish", "Guitar" }, cPl, kAnalog | kWave | kFM | kSine, "room",
                   [=] (VaultPreset& p) { push (p, at (M ("guitar.nylon", "Nylon Guitar", { { "m.tone", 0.45f } }), 0.5f, 0.12f)); } });
    L.push_back ({ "subpluck", { "Sub", "Low End", "Weighted" }, cPl, kAnalog | kWave | kFM | kString, "room",
                   [=] (VaultPreset& p) { push (p, at (subSine(), 0.45f, 0.0f, -1)); } });
    L.push_back ({ "padpluck", { "& Pad", "Cushioned", "Padded" }, cPl | cLe, kAll & ~kSampleSus, "hall",
                   [=] (VaultPreset& p) { push (p, at (warmPadL(), 0.33f)); } });
    L.push_back ({ "airpluck", { "& Air", "Misty", "Breath" }, cPl | cSy, kAll & ~kSampleSus, "cloud",
                   [=] (VaultPreset& p) { push (p, at (airPadL(), 0.3f)); } });
    L.push_back ({ "sparkle", { "Sparkle", "Twinkle", "Glint" }, cPl | cSy, kAnalog | kString | kFM | kModal | kSampleShot, "plate",
                   [=] (VaultPreset& p) { push (p, at (Y ("Glass Pluck", pluckEnv (4000, 0.0f, 0.2f, 0.6f, 0.3f) + Values { { "engine", 1 }, { "fmRatio", 4 }, { "fmAmt", 0.15f } }), 0.18f, 0.2f, 1)); } });
    L.push_back ({ "ksdouble", { "Twin String", "Double", "Paired" }, cPl, kAnalog | kWave | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (Y ("Plucked String", Values { { "engine", 2 }, { "wave", 0.45f }, { "cutoff", 4000 }, { "decay", 1.1f }, { "sustain", 1 }, { "release", 0.5f } }), 0.55f, -0.2f)); } });
    L.push_back ({ "dulchyb", { "Folk", "Hammered", "Rustic" }, cPl, kSaw | kSquare | kFM | kModal | kString, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("pluck.dulcimer", "pluck", "Stick Dulcimer"), 0.5f, 0.15f)); } });
    L.push_back ({ "ksmallet", { "Bell", "Gamelan", "Chime" }, cPl, kString | kSaw | kSquare, "plate",
                   [=] (VaultPreset& p) { push (p, at (Y ("Thumb Piano Synth", Values { { "engine", 9 }, { "wave", 0.15f }, { "fmAmt", 0.2f }, { "decay", 0.7f }, { "sustain", 1 }, { "release", 0.4f } }), 0.45f)); } });
    //---------------------------------------------------------------- leads
    L.push_back ({ "flutelead", { "& Flute", "Fluted", "Pastoral" }, cLe, kSine | kSquare | kBreath | kWave | kPulse, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("winds.flute.concert", "vibrato", "Concert Flute", 0.04f, 0.3f, 3500), 0.55f)); } });
    L.push_back ({ "harmlead", { "& Harmonica", "Blues", "Harp Lead" }, cLe, kSquare | kPulse | kSaw | kOrgan, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("winds.harmonica.chromatic", "normal", "Chromatic Harmonica", 0.02f, 0.25f, 3000), 0.45f)); } });
    L.push_back ({ "saxlead", { "& Sax", "Smoky", "Saxed" }, cLe, kSaw | kSquare | kFM | kOrch | kVox, "plate",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("winds.sax.soprano", "vibrato", "Curved Soprano Sax", 0.03f, 0.3f, 2800), 0.45f)); } });
    L.push_back ({ "violinlead", { "& Violin", "Fiddle", "Gypsy" }, cLe, kSaw | kSuper | kOrch | kSine | kWave, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("strings.violin.solo", "sustain", "Solo Violin", 0.06f, 0.4f, 3000), 0.5f)); } });
    L.push_back ({ "mutelead", { "Muted Trumpet", "Harmon", "Noir" }, cLe, kSquare | kPulse | kSaw | kOrch | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("brass.trumpet", "harmon mute", "Trumpet", 0.03f, 0.3f, 2800), 0.45f)); } });
    L.push_back ({ "voicelead", { "& Voice", "Siren", "Diva" }, cLe, kSine | kWave | kBreath | kSquare | kVox, "plate",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("voice.female.solo", "oo", "Female Solo Voice", 0.05f, 0.4f, 3000), 0.5f)); } });
    L.push_back ({ "octdown", { "Octave", "Twin Octave", "Low Double" }, cLe, kSynthAll, "room",
                   [=] (VaultPreset& p) { auto b = p.parts.front(); for (auto k : { "revMix", "delayMix", "chorus", "drive", "crush", "wow" }) b.set (k, 0.0f); push (p, at (b, 0.6f, 0.0f, -1)); } });
    L.push_back ({ "octupl", { "Shine", "Overtone", "Halo" }, cLe, kSaw | kSquare | kPulse | kSine | kWave, "plate",
                   [=] (VaultPreset& p) { push (p, at (Y ("Sine Lead", leadEnv (6000) + Values { { "engine", 7 }, { "wave", wSine } }), 0.2f, 0.0f, 1)); } });
    L.push_back ({ "whistlel", { "Whistling", "Piped", "Bird" }, cLe, kSaw | kSquare | kPulse | kOrch | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (Y ("Whistle", leadEnv (5000) + Values { { "engine", 7 }, { "wave", wSine } } + B (5, 0.6f, 0, 0, 0.35f)), 0.55f)); } });
    L.push_back ({ "sqlead2", { "Reinforced", "Doubled Square", "Twin Square" }, cLe, kSine | kBreath | kVox | kWave, "room",
                   [=] (VaultPreset& p) { push (p, at (Y ("Square Lead", leadEnv (1800) + Values { { "wave", 0.5f } }), 0.45f, 0.1f)); } });
    //---------------------------------------------------------------- pads
    L.push_back ({ "strpad", { "& Strings", "Orchestral", "Bowed" }, cPa, kSaw | kSuper | kPulse | kWave | kSine | kVox | kModel, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("strings.violins.ensemble", "sustain", "Violin Section", 0.9f, 2.4f, 3200), 0.45f, 0.15f)); } });
    L.push_back ({ "cellopadl", { "& Cellos", "Low Bowed", "Earthen" }, cPa, kSaw | kSuper | kSine | kWave | kFM | kSampleSus, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("strings.cellos.ensemble", "sustain", "Cello Section", 1.0f, 2.4f, 2200), 0.45f, -0.15f)); } });
    L.push_back ({ "choirpadl", { "& Choir", "Choral", "Hymnal" }, cPa, kSaw | kSuper | kSine | kWave | kOrgan | kSampleSus, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("voice.choir.classic", "oohs", "Classic Choir", 0.8f, 2.4f, 2200), 0.4f)); } });
    L.push_back ({ "organpadl", { "& Organ", "Sacred", "Vespers" }, cPa, kSaw | kSine | kVox | kSampleSus | kModel, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("organ.chamber", "8 foot", "Chamber Organ", 0.9f, 2.4f, 2400), 0.4f)); } });
    L.push_back ({ "glasspadl", { "Glass", "Shimmer", "Frost" }, cPa, kSaw | kSuper | kSine | kSampleSus | kVox | kModel | kOrch, "vast",
                   [=] (VaultPreset& p) { auto g = sampleSus ("fx.glass", "slow", "Glass Harp", 0.8f, 2.4f, 3500); push (p, at (g, 0.18f, 0.2f, 1)); } });
    L.push_back ({ "bowedvib", { "Bowed Bars", "Aura", "Halo" }, cPa, kSaw | kSine | kWave | kSampleSus | kFM, "hall",
                   [=] (VaultPreset& p) { auto g = sampleSus ("mallets.vibraphone", "bowed", "Bowed Vibraphone", 0.6f, 2.4f, 3500); push (p, at (g, 0.3f, -0.15f)); } });
    L.push_back ({ "sinesupport", { "Warm", "Rounded", "Velvet" }, cPa, kSampleSus | kVox | kOrch | kBreath, "hall",
                   [=] (VaultPreset& p) { push (p, at (sinePad(), 0.6f)); } });
    L.push_back ({ "subpad", { "Earth", "Grounded", "Low" }, cPa, kAll & ~kModel, "hall",
                   [=] (VaultPreset& p) { push (p, at (sinePad ("Sub Pad"), 0.4f, 0.0f, -1)); } });
    L.push_back ({ "vocpadl", { "& Voices", "Angel", "Vocal" }, cPa, kSaw | kSuper | kSine | kWave | kFM | kSampleSus, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (M ("voice.pad", "Vocal Pad", { { "m.velSens", 0.4f } }), 0.4f)); } });
    L.push_back ({ "fluteair", { "& Flute", "Woodland", "Pastoral" }, cPa, kSaw | kSine | kWave | kSampleSus, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("winds.flute.concert", "sustain", "Concert Flute", 0.8f, 2.2f, 3000), 0.35f, 0.1f)); } });
    L.push_back ({ "femalepad", { "Siren", "Lullaby", "Sung" }, cPa, kSaw | kSine | kWave | kSampleSus, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("voice.female.solo", "oo", "Female Solo Voice", 0.9f, 2.4f, 2800), 0.35f)); } });
    L.push_back ({ "hornpadl", { "& Horns", "Heroic", "Distant Horn" }, cPa, kSampleSus | kSaw | kOrch, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("brass.horn", "sustain", "French Horn", 1.0f, 2.2f, 1800), 0.35f, -0.1f)); } });
    L.push_back ({ "cinepad", { "Cinematic", "Film Score", "Epic" }, cPa, kSaw | kSuper | kSine | kWave, "cathedral",
                   [=] (VaultPreset& p) {
                       push (p, at (sampleSus ("strings.violins.ensemble", "sustain", "Violin Section", 0.9f, 2.4f, 3000), 0.35f, 0.2f));
                       push (p, at (sampleSus ("strings.cellos.ensemble", "sustain", "Cello Section", 1.0f, 2.4f, 2000), 0.35f, -0.2f));
                       push (p, at (sampleSus ("voice.choir.classic", "oohs", "Classic Choir", 0.8f, 2.4f, 2000), 0.25f)); } });
    L.push_back ({ "heaven", { "Heavenly", "Sanctuary", "Seraph" }, cPa, kSampleSus | kSine | kVox, "cathedral",
                   [=] (VaultPreset& p) {
                       push (p, at (sampleSus ("organ.pipe", "soft", "Pipe Organ", 1.0f, 2.4f, 2200), 0.35f));
                       auto g = sampleSus ("fx.glass", "slow", "Glass Harp", 0.8f, 2.4f, 3500); push (p, at (g, 0.15f, 0.2f, 1)); } });
    //---------------------------------------------------------------- synth
    L.push_back ({ "pianohyb", { "Hybrid", "Piano", "Grand" }, cSy, kSaw | kSuper | kSquare | kPulse | kWave | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("piano.grand.studio", "sustain", "Studio Grand", 3500), 0.55f)); } });
    L.push_back ({ "hornstab", { "Brass Hit", "Horn Stab", "Big Band" }, cSy, kSaw | kSuper | kOrch, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("brass.horn", "staccato", "French Horn", 2500), 0.5f)); } });
    L.push_back ({ "spicstab", { "Spiccato", "Bouncing Bow", "String Hit" }, cSy, kSaw | kSuper | kSquare | kOrch, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("strings.violins.ensemble", "spiccato", "Violin Section", 3500), 0.5f, 0.15f)); } });
    L.push_back ({ "orchhitl", { "Orchestra Hit", "Tutti", "Stinger" }, cSy, kSaw | kSuper | kOrch, "hall",
                   [=] (VaultPreset& p) {
                       push (p, at (sampleShot ("strings.violins.ensemble", "spiccato", "Violin Section", 3500), 0.45f, 0.2f));
                       push (p, at (sampleShot ("brass.trombone", "staccato", "Tenor Trombone", 2200), 0.45f, -0.2f)); } });
    L.push_back ({ "harpsihyb", { "Baroque", "Courtly", "Harpsi" }, cSy, kPulse | kSquare | kSaw, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("keys.harpsichord.flemish", "lower 8'", "Flemish Harpsichord", 4500), 0.45f, 0.15f)); } });
    L.push_back ({ "organhyb", { "Gospel", "Church", "Revival" }, cSy, kSaw | kSuper | kSquare, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("organ.pipe", "principal", "Pipe Organ", 0.01f, 0.3f, 2600), 0.4f)); } });
    L.push_back ({ "eplayer", { "& EP", "Electric", "Tine" }, cSy, kSaw | kSuper | kPulse | kWave | kVox, "room",
                   [=] (VaultPreset& p) { push (p, at (M ("ep.tine.mellow", "Tine Electric Piano", { { "m.tone", 0.45f } }), 0.5f)); } });
    L.push_back ({ "padsy", { "& Pad", "Padded", "Wash" }, cSy, kAll & ~kSampleSus, "hall",
                   [=] (VaultPreset& p) { push (p, at (warmPadL(), 0.3f)); } });
    L.push_back ({ "subsy", { "Low", "Sub", "Weighted" }, cSy, kSynthAll, "room",
                   [=] (VaultPreset& p) { push (p, at (subSine(), 0.4f, 0.0f, -1)); } });
    L.push_back ({ "pluckon", { "Plucked", "Attack", "Transient" }, cSy, kSaw | kSuper | kOrch | kVox | kOrgan, "room",
                   [=] (VaultPreset& p) { push (p, at (sawPluckL(), 0.45f, 0.1f)); } });
    //---------------------------------------------------------------- fx
    L.push_back ({ "riserstack", { "Stacked", "Tidal", "Surging" }, cFx, kRiser, "cathedral",
                   [=] (VaultPreset& p) {
                       push (p, at (Y ("Noise Sweep", Values { { "engine", 1 }, { "fmRatio", 5.31f }, { "wave", 1.0f }, { "fmAmt", 0.85f }, { "fsustain", 1 }, { "filterType", 4 }, { "reso", 0.3f }, { "cutoff", 400 }, { "attack", 1.5f }, { "sustain", 1 },
                                                         { "release", 1.5f }, { "e3attack", 4.5f }, { "e3sustain", 1 } } + MM (0, 2, 1, 0.42f)), 0.5f)); } });
    L.push_back ({ "impactsub", { "Boom", "Seismic", "Quake" }, cFx, kHit | kSampleShot, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (Y ("Sub Drop", Values { { "engine", 7 }, { "wave", wSine }, { "attack", 0.003f }, { "decay", 1.8f }, { "sustain", 0 }, { "release", 1.5f },
                                                                       { "fdecay", 0.25f } } + MM (0, 1, 0, 0.6f)), 0.5f, 0.0f, -2)); } });
    L.push_back ({ "timpimpact", { "Timpani", "Thunder", "War Drum" }, cFx, kHit | kRiser | kDrone, "cathedral",
                   [=] (VaultPreset& p) { auto t = sampleShot ("perc.timpani", "hit", "Timpani", 3000); push (p, at (t, 0.5f, 0.0f, -1)); } });
    L.push_back ({ "dronechoir", { "Haunted", "Spectral", "Ghostly" }, cFx, kDrone | kNoise | kSampleSus, "vast",
                   [=] (VaultPreset& p) { push (p, at (M ("voice.choir", "Choir", { { "m.voices", 8 }, { "m.vowel", 2.0f } }), 0.4f, 0.0f, -1)); } });
    L.push_back ({ "dronestr", { "Dread", "Ominous", "Brooding" }, cFx, kDrone | kNoise | kSine, "vast",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("strings.cellos.ensemble", "tremolo", "Cello Section", 1.5f, 2.5f, 1500), 0.45f, 0.0f, -1)); } });
    L.push_back ({ "revpianol", { "Memory", "Echoes", "Afterimage" }, cFx, kSampleShot | kNoise | kWave, "vast",
                   [=] (VaultPreset& p) { push (p, at (S ("piano.grand.concert", "sustain", "Concert Grand", { { "s.attack", 1.3f }, { "s.release", 0.3f }, { "s.cutoff", 3000.0f } }), 0.45f)); } });
    L.push_back ({ "windl", { "Windswept", "Stormy", "Gale" }, cFx, kDrone | kSampleSus | kModal | kSine, "vast",
                   [=] (VaultPreset& p) { push (p, at (Y ("Wind", Values { { "engine", 1 }, { "fmRatio", 7.13f }, { "wave", 1.0f }, { "fmAmt", 0.9f }, { "fsustain", 1 }, { "filterType", 4 }, { "reso", 0.5f }, { "cutoff", 700 }, { "attack", 1.2f }, { "sustain", 1 },
                                                                 { "release", 2.5f }, { "lfoFilter", 0.4f }, { "lfoRate", 0.15f } }), 0.45f)); } });
    L.push_back ({ "glassl", { "Glimmer", "Starlit", "Prism" }, cFx, kDrone | kNoise | kSampleShot | kRiser, "vast",
                   [=] (VaultPreset& p) { auto g = sampleSus ("fx.glass", "fast", "Glass Harp", 0.6f, 2.5f, 3500); push (p, at (g, 0.25f, 0.2f, 1)); } });
    return L;
}

//==============================================================================
// names: unique, <= 22 characters, no repeated word
struct Namer
{
    std::set<juce::String> used;
    bool take (const juce::String& n)
    {
        if (n.length() > 22 || n.isEmpty()) return false;
        if (vaultaudit::nameProblem (n).isNotEmpty() || n.containsIgnoreCase ("808")) return false;
        juce::StringArray seen;
        for (auto& w : juce::StringArray::fromTokens (n.toLowerCase(), " ", ""))
        {
            if (w == "&") continue;
            if (seen.contains (w)) return false;
            seen.add (w);
        }
        return used.insert (n.toLowerCase()).second;
    }
};
// spare evocative words per category for names that would otherwise collide
const juce::StringArray& spareWords (int cat)
{
    static const juce::StringArray pl { "Neon", "Amber", "Velvet", "Silver", "Coral", "Lunar", "Pastel", "Ivory", "Copper", "Indigo", "Jade", "Opal", "Cobalt", "Sable" };
    static const juce::StringArray le { "Neon", "Night", "Chrome", "Solar", "Crimson", "Electric", "Royal", "Golden", "Cyan", "Scarlet", "Midnight", "Static", "Sonic" };
    static const juce::StringArray pa { "Northern", "Distant", "Endless", "Silent", "Ocean", "Polar", "Stellar", "Dusk", "Dawn", "Nebula", "Lunar", "Twilight", "Ember" };
    static const juce::StringArray sy { "Neon", "Retro", "Chrome", "City", "Night", "Arcade", "Laser", "Velvet", "Turbo", "Prism", "Vapor", "Sunset", "Metro" };
    static const juce::StringArray fx { "Void", "Orbit", "Abyss", "Eclipse", "Vortex", "Signal", "Phantom", "Rift", "Echo Chamber", "Horizon", "Nova", "Quasar" };
    switch (cat)
    {
        case catPlucks: return pl;
        case catLeads: return le;
        case catPads: return pa;
        case catSynth: return sy;
        default: return fx;
    }
}

//==============================================================================
// candidates
struct Cand
{
    VaultPreset preset;
    juce::String recipe;
    std::vector<juce::String> nameOptions;
};

// does every sample part play at C4, C5 and C6 (the gate renders them; a pad silent at C4 is a broken pad)?
bool playable (const RecipeContext& ctx, const VaultPart& p)
{
    if (! isSample (p)) return true;
    if (! ctx.hasInstrument (p.source)) return false;
    const auto& d = ctx.library->find (p.source)->desc;
    const int ai = d.articulationIndex (p.articulation);
    if (! juce::isPositiveAndBelow (ai, (int) d.articulations.size())) return false;
    int lo = 128, hi = -1;
    for (auto& z : d.articulations[(size_t) ai].zones) if (! z.releaseTrigger) { lo = std::min (lo, z.lo); hi = std::max (hi, z.hi); }
    const int t = p.octave * 12 + p.semi;
    return lo <= homeNote - 12 + t && hi >= homeNote + 12 + t;
}
bool playableMain (const RecipeContext& ctx, const VaultPart& p)
{
    if (! isSample (p)) return true;
    if (! ctx.hasInstrument (p.source)) return false;
    const auto& d = ctx.library->find (p.source)->desc;
    const int ai = d.articulationIndex (p.articulation);
    int lo = 128, hi = -1;
    for (auto& z : d.articulations[(size_t) ai].zones) if (! z.releaseTrigger) { lo = std::min (lo, z.lo); hi = std::max (hi, z.hi); }
    const int t = p.octave * 12 + p.semi;
    return lo <= homeNote + t && hi >= homeNote + t;
}

std::vector<Cand> candidates (const RecipeContext& ctx)
{
    const auto V = voices();
    const auto T = treatments();
    const auto L = layers();
    std::vector<Cand> out;
    // 1. voice x treatment x space: round robin over voices, so every voice is explored evenly
    std::vector<std::vector<Cand>> perVoice (V.size());
    for (size_t vi = 0; vi < V.size(); ++vi)
    {
        auto& v = V[vi];
        if (! (v.category == catFxAtmos ? playableMain (ctx, v.part) : playable (ctx, v.part))) continue;
        for (auto& t : T)
        {
            if ((t.cats & C (v.category)) == 0 || (t.kinds & v.kind) == 0) continue;
            int si = 0;
            for (auto* sp : t.spaces)
            {
                juce::String spT (sp);
                // pads and atmospheres live in big spaces (a treatment made in a room is heard in a hall); plucks / leads /
                // stabs not in cathedrals
                if ((v.category == catPads || v.category == catFxAtmos) && (spT == "dry" || spT == "slap" || spT == "room"))
                {
                    if (juce::String (t.tag) == "natural") { ++si; continue; }
                    spT = "hall";
                    sp = "hall";
                }
                if ((v.category == catPlucks || v.category == catLeads || v.category == catSynth) && (spT == "cathedral" || spT == "vast")) { ++si; continue; }
                if (juce::String (t.tag) == "natural" && spT != "room" && spT != "echo" && spT != "cloud" && spT != "hall") { ++si; continue; }
                Cand c;
                c.preset.category = v.category;
                c.preset.parts = { v.part };
                fx (c.preset, space (sp).fx);
                t.apply (c.preset);
                c.recipe = juce::String (v.tag) + "/" + t.tag + "/" + sp;
                const juce::String sw = space (sp).word;
                for (int ai = 0; ai < t.adjectives.size(); ++ai)
                    for (auto& noun : v.nouns)
                    {
                        const auto adj = t.adjectives[t.adjectives[0].isEmpty() ? ai : (ai + (int) vi) % t.adjectives.size()];
                        if (adj.isEmpty() && si == 0) c.nameOptions.push_back (noun);
                        c.nameOptions.push_back (joinWords ({ adj, noun }));
                        c.nameOptions.push_back (joinWords ({ adj, sw, noun }));
                        c.nameOptions.push_back (joinWords ({ sw, noun }));
                        c.nameOptions.push_back (joinWords ({ noun, sw }));
                    }
                perVoice[vi].push_back (std::move (c));
                ++si;
            }
        }
    }
    // 2. layers: voice x layer recipe (natural treatment + the layer's space)
    std::vector<std::vector<Cand>> perLayer (L.size());
    for (size_t li = 0; li < L.size(); ++li)
    {
        auto& l = L[li];
        int rot = 0;
        for (auto& v : V)
        {
            if ((l.cats & C (v.category)) == 0 || (l.kinds & v.kind) == 0) continue;
            if (! (v.category == catFxAtmos ? playableMain (ctx, v.part) : playable (ctx, v.part))) continue;
            Cand c;
            c.preset.category = v.category;
            c.preset.parts = { v.part };
            l.add (c.preset);
            bool ok = true;
            for (size_t k = 1; k < c.preset.parts.size(); ++k) ok &= playable (ctx, c.preset.parts[k]) || playableMain (ctx, c.preset.parts[k]);
            if (! ok) continue;
            fx (c.preset, space (l.spaceTag).fx);
            if (c.preset.parts.size() >= 3) lowPass (c.preset.parts.front(), currentLp (c.preset.parts.front()) * 0.8f);
            c.recipe = juce::String (v.tag) + "+" + l.tag;
            ++rot;
            for (int wi = 0; wi < l.words.size(); ++wi)
                for (auto& noun : v.nouns)
                {
                    const auto w = l.words[(wi + rot) % l.words.size()];
                    if (w.startsWith ("&")) c.nameOptions.push_back (joinWords ({ noun, w }));
                    else
                    {
                        c.nameOptions.push_back (joinWords ({ w, noun }));
                        c.nameOptions.push_back (w + " " + noun.fromLastOccurrenceOf (" ", false, false));
                    }
                }
            perLayer[li].push_back (std::move (c));
        }
    }
    auto flatten = [] (const std::vector<std::vector<Cand>>& lists)
    {
        std::vector<Cand> f;
        for (size_t r = 0;; ++r)
        {
            bool any = false;
            for (auto& l : lists) if (r < l.size()) { f.push_back (l[r]); any = true; }
            if (! any) break;
        }
        return f;
    };
    const auto A = flatten (perVoice), Bv = flatten (perLayer);
    // every voice as designed first (round 0: its natural sound), so a plain voice is never pushed out by its own layered
    // or treated variants; then 2 treated voices : 1 layer recipe, round robin
    size_t first = 0;
    for (auto& l : perVoice) if (! l.empty()) ++first;
    for (size_t a = 0; a < first; ++a) out.push_back (A[a]);
    for (size_t a = first, b = 0; a < A.size() || b < Bv.size();)
    {
        for (int k = 0; k < 2 && a < A.size(); ++k) out.push_back (A[a++]);
        if (b < Bv.size()) out.push_back (Bv[b++]);
    }
    return out;
}

//==============================================================================
// calibration: every part alone at -20 dB K-weighted (C5, velocity 0.8, no FX) -> its `gain`
std::mutex calMutex;
std::map<juce::String, float> calCache;

bool isFxKey (const juce::String& id)
{
    static const std::set<juce::String> fxKeys { "drive", "driveType", "crush", "wow", "chorus", "phaser", "flanger", "delayMix", "delayTime", "delayFb", "delayMode",
                                                 "revMix", "revSize", "revType", "eqLow", "eqHigh", "reverse", "freeze", "width", "master", "gain", "macro2", "macro5",
                                                 "body", "bodyMix", "punchFx" };
    return fxKeys.count (id) > 0;
}
void neutralFx (VaultPart& p)
{
    if (! isSynth (p)) return;
    setAll (p, { { "revMix", 0.0f }, { "master", 0.0f }, { "macro2", 0.0f }, { "macro5", 0.0f }, { "delayMix", 0.0f }, { "chorus", 0.0f } });
}
juce::String partKey (const VaultPart& p)
{
    juce::String k;
    k << (int) p.kind << "|" << p.source << "|" << p.articulation << "|" << p.octave << "|" << p.semi;
    auto v = p.values;
    std::sort (v.begin(), v.end());
    for (auto& [id, x] : v) if (! isFxKey (id)) k << "|" << id << "=" << juce::String (x, 4);
    return k;
}
float partLoudness (const VaultPart& part)
{
    const auto key = partKey (part);
    {
        std::lock_guard<std::mutex> l (calMutex);
        if (auto it = calCache.find (key); it != calCache.end()) return it->second;
    }
    VaultPreset solo;
    solo.id = "cal"; solo.name = "cal"; solo.category = catSynth;
    VaultPart p = part;
    p.level = 1.0f; p.pan = 0.0f;
    Values keep;
    for (auto& [id, x] : p.values) if (! isFxKey (id)) keep.push_back ({ id, x });
    p.values = keep;
    neutralFx (p);
    solo.parts = { p };
    auto b = recipegate::render (solo, homeNote, 0.8f, 2.5, 3.5);
    const float loud = vaultqc::analyse (b, recipegate::sr, 2.5).loud;
    std::lock_guard<std::mutex> l (calMutex);
    calCache[key] = loud;
    return loud;
}
void calibrate (VaultPreset& p)
{
    for (auto& part : p.parts)
    {
        const float loud = partLoudness (part);
        part.set ("gain", juce::jlimit (-24.0f, 12.0f, -20.0f - (loud < -90.0f ? -20.0f : loud)));
    }
}

//==============================================================================
// own fingerprint: MFCC (c0..c12 of a 40-band log-mel spectrum, orthonormal DCT -> Euclidean distance = RMS dB of the
// smoothed spectral envelope) per time segment over C5 (vel 0.8, 7 segments incl. the release), C5 vel 0.35 (2), C4 (3),
// C6 (3) + stereo width + modulation (amplitude / pan / spectral fluctuation), relative to the C5 render's loudest segment.
constexpr int ownMel = 40, ownCeps = 13;
std::vector<float> mfccSegments (const juce::AudioBuffer<float>& b, const std::vector<std::pair<double, double>>& segs, float refDb, bool relative, float& maxDb)
{
    constexpr int order = 11, n = 1 << order;
    juce::dsp::FFT fft (order);
    const double sr = recipegate::sr;
    auto mel = [] (double hz) { return 2595.0 * std::log10 (1.0 + hz / 700.0); };
    const double m0 = mel (50.0), m1 = mel (16000.0);
    std::vector<float> d ((size_t) (2 * n));
    std::vector<std::vector<double>> bands;
    for (auto [t0, t1] : segs)
    {
        std::vector<double> band (ownMel, 1.0e-12);
        int frames = 0;
        for (int s0 = (int) (t0 * sr); s0 + n <= std::min (b.getNumSamples(), (int) (t1 * sr) + n / 2); s0 += n / 4)
        {
            std::fill (d.begin(), d.end(), 0.0f);
            for (int i = 0; i < n; ++i)
                d[(size_t) i] = 0.5f * (b.getSample (0, s0 + i) + b.getSample (1, s0 + i))
                                * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (n - 1)));
            fft.performFrequencyOnlyForwardTransform (d.data());
            for (int i = 1; i < n / 2; ++i)
            {
                const double hz = i * sr / n;
                if (hz < 50.0 || hz >= 16000.0) continue;
                const int k = juce::jlimit (0, ownMel - 1, (int) ((mel (hz) - m0) / (m1 - m0) * ownMel));
                band[(size_t) k] += (double) d[(size_t) i] * d[(size_t) i];
            }
            ++frames;
        }
        for (auto& x : band) x = 10.0 * std::log10 (x / std::max (1, frames));
        bands.push_back (band);
    }
    maxDb = -300;
    for (auto& band : bands) for (auto x : band) maxDb = std::max (maxDb, (float) x);
    const float ref = relative ? maxDb : refDb;
    std::vector<float> out;
    for (auto& band : bands)
        for (int q = 0; q < ownCeps; ++q)
        {
            double c = 0;
            for (int m = 0; m < ownMel; ++m) c += std::max (-70.0, band[(size_t) m] - ref) * std::cos (juce::MathConstants<double>::pi * q * (m + 0.5) / ownMel);
            out.push_back ((float) (c * std::sqrt ((q == 0 ? 1.0 : 2.0) / ownMel)));
        }
    return out;
}
float sideToMid (const juce::AudioBuffer<float>& b, double t0, double t1)
{
    double s = 1e-12, m = 1e-12;
    for (int i = (int) (t0 * recipegate::sr); i < std::min (b.getNumSamples(), (int) (t1 * recipegate::sr)); ++i)
    {
        const double l = b.getSample (0, i), r = b.getSample (1, i);
        m += 0.25 * (l + r) * (l + r); s += 0.25 * (l - r) * (l - r);
    }
    return (float) (10.0 * std::log10 (s / m));
}
float detrendedSd (const std::vector<double>& y)
{
    const int n = (int) y.size();
    if (n < 8) return 0;
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int i = 0; i < n; ++i) { sx += i; sy += y[(size_t) i]; sxx += (double) i * i; sxy += i * y[(size_t) i]; }
    const double bb = (n * sxy - sx * sy) / std::max (1e-9, n * sxx - sx * sx), a = (sy - bb * sx) / n;
    double e = 0;
    for (int i = 0; i < n; ++i) { const double r = y[(size_t) i] - (a + bb * i); e += r * r; }
    return (float) std::sqrt (e / n);
}
std::array<float, 3> modulation (const juce::AudioBuffer<float>& b)
{
    const double sr = recipegate::sr;
    const int fr = (int) (0.01 * sr), s0 = (int) (0.25 * sr), s1 = std::min (b.getNumSamples(), (int) (1.45 * sr));
    std::vector<double> mono, pan;
    for (int s = s0; s + fr <= s1; s += fr)
    {
        double l = 1e-12, r = 1e-12;
        for (int i = s; i < s + fr; ++i) { l += (double) b.getSample (0, i) * b.getSample (0, i); r += (double) b.getSample (1, i) * b.getSample (1, i); }
        mono.push_back (10.0 * std::log10 ((l + r) / (2 * fr)));
        pan.push_back (10.0 * std::log10 (l / r));
    }
    constexpr int order = 10, n = 1 << order, nb = 32;
    juce::dsp::FFT fft (order);
    std::vector<float> d ((size_t) (2 * n));
    std::vector<std::vector<double>> traj (nb);
    for (int s = s0; s + n <= s1; s += fr)
    {
        std::fill (d.begin(), d.end(), 0.0f);
        for (int i = 0; i < n; ++i) d[(size_t) i] = 0.5f * (b.getSample (0, s + i) + b.getSample (1, s + i)) * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (n - 1)));
        fft.performFrequencyOnlyForwardTransform (d.data());
        std::vector<double> band (nb, 1e-14);
        for (int i = 1; i < n / 2; ++i)
        {
            const double hz = i * sr / n;
            if (hz < 150 || hz >= 8000) continue;
            band[(size_t) juce::jlimit (0, nb - 1, (int) (std::log (hz / 150.0) / std::log (8000.0 / 150.0) * nb))] += (double) d[(size_t) i] * d[(size_t) i];
        }
        for (int k = 0; k < nb; ++k) traj[(size_t) k].push_back (10.0 * std::log10 (band[(size_t) k]));
    }
    double mx = -300;
    for (auto& t : traj) for (auto v : t) mx = std::max (mx, v);
    double flux = 0; int used = 0;
    for (auto& t : traj)
    {
        if (t.empty()) continue;
        double mean = 0; for (auto v : t) mean += v; mean /= (double) t.size();
        if (mean < mx - 45) continue;
        flux += detrendedSd (t); ++used;
    }
    return { std::min (12.0f, detrendedSd (mono)), std::min (12.0f, detrendedSd (pan)), (float) std::min (12.0, used > 0 ? flux / used : 0.0) };
}
std::vector<float> ownPrint (const VaultPreset& p)
{
    const auto c5 = recipegate::render (p, homeNote, 0.8f, 1.5, 3.0);
    const auto soft = recipegate::render (p, homeNote, 0.35f, 1.5, 1.5);
    const auto c4 = recipegate::render (p, homeNote - 12, 0.8f, 1.2, 2.0);
    const auto c6 = recipegate::render (p, homeNote + 12, 0.8f, 1.2, 2.0);
    float mx = 0, dummy = 0;
    auto f = mfccSegments (c5, { { 0.0, 0.03 }, { 0.03, 0.1 }, { 0.1, 0.3 }, { 0.3, 0.7 }, { 0.7, 1.5 }, { 1.5, 2.0 }, { 2.0, 3.0 } }, 0, true, mx);
    for (auto x : mfccSegments (soft, { { 0.0, 0.1 }, { 0.1, 0.7 } }, mx, false, dummy)) f.push_back (x);
    for (auto* b : { &c4, &c6 })
        for (auto x : mfccSegments (*b, { { 0.0, 0.1 }, { 0.1, 0.6 }, { 0.6, 1.2 } }, mx, false, dummy)) f.push_back (x);
    f.push_back (0.3f * juce::jlimit (-40.0f, 0.0f, sideToMid (c5, 0.05, 1.5)));
    f.push_back (0.3f * juce::jlimit (-40.0f, 0.0f, sideToMid (c5, 1.5, 3.0)));
    const float w = std::sqrt ((float) ownMel * 5.0f);
    for (auto m : modulation (c5)) f.push_back (w * m);
    return f;
}
float ownDistance (const std::vector<float>& a, const std::vector<float>& b)
{
    double s = 0;
    const size_t n = std::min (a.size(), b.size());
    for (size_t i = 0; i < n; ++i) { const double d = a[i] - b[i]; s += d * d; }
    const double segs = (double) (n - 5) / ownCeps;
    return (float) std::sqrt (s / std::max (1.0, segs * ownMel));
}

//==============================================================================
// repairs after a gate failure (false when nothing more can be done)
bool repair (VaultPreset& p, const juce::String& fail, int attempt)
{
    auto& a = p.parts.front();
    if (fail == "fundamental")
    {
        // darker first; from the 2nd attempt a sine at the played pitch carries the fundamental (synth: osc B / sample: a part)
        for (auto& part : p.parts) lowPass (part, std::min (currentLp (part), 8000.0f) * 0.6f);
        a.set ("eqLow", std::min (6.0f, a.get ("eqLow", 0.0f) + 2.0f));
        if (attempt >= 1)
        {
            bool hasSine = false;
            for (auto& part : p.parts) hasSine |= part.label == "Sine";
            if (! hasSine && (int) p.parts.size() < maxParts)
            {
                auto s = Y ("Sine Support", Values { { "engine", 7 }, { "wave", wSine }, { "attack", std::max (0.004f, getv (a, isSample (a) ? "s.attack" : "attack", 0.004f)) },
                                              { "decay", getv (a, "decay", 0.6f) }, { "sustain", isSynth (a) ? getv (a, "sustain", 0.8f) : 0.8f },
                                              { "release", isSynth (a) ? getv (a, "release", 0.3f) : 0.6f }, { "cutoff", 3000 } });
                s.label = "Sine";
                s.octave = a.octave;
                s.level = 0.55f;
                p.parts.push_back (s);
            }
            else
                for (auto& part : p.parts) if (part.label == "Sine") part.level = std::min (1.2f, part.level * 1.5f);
        }
        return attempt < 4;
    }
    if (fail == "harsh-2-5k" || fail == "piercing-highs")
    {
        for (auto& part : p.parts) lowPass (part, std::min (currentLp (part), 8000.0f) * 0.65f);
        a.set ("eqHigh", std::max (-8.0f, a.get ("eqHigh", 0.0f) - 2.0f));
        a.set ("drive", a.get ("drive", 0.0f) * 0.7f);
        a.set ("crush", a.get ("crush", 0.0f) * 0.6f);
        return true;
    }
    if (fail == "velocity")
    {
        for (auto& part : p.parts) velSens (part, std::min (1.0f, getVelSens (part) + 0.25f));
        a.set ("drive", a.get ("drive", 0.0f) * 0.6f);
        if (attempt >= 1) for (auto& part : p.parts) if (isSynth (part)) part.set ("mmSrc8", 5), part.set ("mmDst8", 9), part.set ("mmAmt8", 0.5f);
        return attempt < 3;
    }
    if (fail == "velocity-wide")
    {
        // recorded dynamics (pp / ff layers) already give the range: less extra velocity -> level on top
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.velSens", attempt == 0 ? 0.05f : 0.0f);
            else velSens (part, getVelSens (part) * 0.6f);
        return attempt < 2;
    }
    if (fail == "click-start")
    {
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.attack", std::max (0.005f, part.get ("s.attack", 0.0f)));
            else if (isSynth (part)) part.set ("attack", std::max (0.008f, part.get ("attack", 0.003f) * 2.0f));
        return attempt < 3;
    }
    if (fail == "click-end" && a.get ("s.releaseSamples", 1.0f) > 0.5f && isSample (a))
    {
        for (auto& part : p.parts) if (isSample (part)) part.set ("s.releaseSamples", 0.0f);
        return true;
    }
    if (fail == "click-spike" && (a.get ("wow", 0.0f) > 0.05f || a.get ("crush", 0.0f) > 0.05f || a.get ("lfoAmp", 0.0f) > 0.05f))
    {
        a.set ("wow", a.get ("wow", 0.0f) * 0.4f); a.set ("crush", a.get ("crush", 0.0f) * 0.4f); a.set ("lfoAmp", a.get ("lfoAmp", 0.0f) * 0.5f);
        return true;
    }
    if (fail == "click-end" || fail == "click-spike")
    {
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.release", std::max (0.25f, part.get ("s.release", 0.0f)));
            else if (isSynth (part)) part.set ("release", std::max (0.25f, part.get ("release", 0.3f)));
            else part.set ("m.release", std::max (0.25f, part.get ("m.release", 0.3f)));
        return attempt < 2;
    }
    if (fail == "tail")
    {
        a.set ("revMix", a.get ("revMix", 0.0f) * 0.7f); a.set ("revSize", a.get ("revSize", 0.0f) * 0.8f); a.set ("delayFb", a.get ("delayFb", 0.35f) * 0.6f);
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.release", part.get ("s.release", 0.0f) > 0.0f ? part.get ("s.release", 0.0f) * 0.6f : 2.5f);
            else if (isSynth (part)) { part.set ("release", part.get ("release", 0.3f) * 0.6f); if (part.get ("engine", 0) == 2 || part.get ("engine", 0) == 9) part.set ("decay", part.get ("decay", 1.0f) * 0.7f); }
            else part.set ("m.release", std::min (1.0f, part.get ("m.release", 0.5f)));
        return attempt < 4;
    }
    if (fail == "octave-layer")
    {
        for (size_t k = 1; k < p.parts.size(); ++k) if (p.parts[k].octave > a.octave) p.parts[k].level *= 0.6f;
        return true;
    }
    if (fail == "low-side" || fail == "mono-loss")
    {
        a.set ("width", a.get ("width", 0.5f) * 0.6f);
        a.set ("chorus", a.get ("chorus", 0.0f) * 0.5f);
        for (auto& part : p.parts)
        {
            part.pan *= 0.5f;
            if (isSample (part)) part.set ("s.width", 0.6f);
            if (isSynth (part) && part.get ("unison", 1) > 1) part.set ("detune", part.get ("detune", 0.15f) * 0.6f);
        }
        return attempt < 3;
    }
    if (fail == "too-peaky" || fail == "clip")
    {
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.attack", std::max (0.008f, part.get ("s.attack", 0.0f)));
            else if (isSynth (part)) part.set ("attack", std::max (0.01f, part.get ("attack", 0.003f)));
        a.set ("punchFx", 0.0f);
        return attempt < 2;
    }
    if (fail == "silent") return false;
    return false;
}

int envInt (const char* n, int def) { auto s = juce::SystemStats::getEnvironmentVariable (n, {}); return s.isNotEmpty() ? s.getIntValue() : def; }
float envFloat (const char* n, float def) { auto s = juce::SystemStats::getEnvironmentVariable (n, {}); return s.isNotEmpty() ? (float) s.getDoubleValue() : def; }

struct Done
{
    Cand cand;
    recipegate::Result res;
    std::vector<float> own;
    int repairs = 0;
};
} // namespace

std::vector<VaultPreset> build_synth (const RecipeContext& ctx)
{
    const float ownMin = envFloat ("SYNTH_OWN_MIN", 2.6f), sharedMin = envFloat ("SYNTH_SHARED_MIN", 3.6f);
    const float ownMax25 = 0.08f, ownMax6k = 0.02f, ownMaxVel = 20.0f;   // velocity 0.35 -> 1.0: a soft note must stay usable
    const int threads = envInt ("SYNTH_THREADS", (int) std::max (1u, std::thread::hardware_concurrency()));
    auto cands = candidates (ctx);
    if (auto cs = juce::SystemStats::getEnvironmentVariable ("SYNTH_CATS", {}); cs.isNotEmpty())
    {
        const auto keep = juce::StringArray::fromTokens (cs, ",", "");
        cands.erase (std::remove_if (cands.begin(), cands.end(), [&] (const Cand& c) { return ! keep.contains (juce::String (c.preset.category)); }), cands.end());
    }
    cands.erase (std::remove_if (cands.begin(), cands.end(), [&] (const Cand& c)
    {
        for (auto& part : c.preset.parts) if (isSample (part) && ! ctx.hasInstrument (part.source)) return true;
        return false;
    }), cands.end());
    if (const int lim = envInt ("SYNTH_LIMIT", 0); lim > 0 && lim < (int) cands.size()) cands.resize ((size_t) lim);
    std::printf ("synth: %d candidates, %d threads\n", (int) cands.size(), threads);

    std::vector<Done> done (cands.size());
    std::atomic<int> next { 0 }, count { 0 };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&]
        {
            for (int i; (i = next++) < (int) cands.size();)
            {
                auto& d = done[(size_t) i];
                d.cand = cands[(size_t) i];
                auto& p = d.cand.preset;
                p.id = "cv.tmp." + juce::String (i); p.name = "Draft " + juce::String (i).replaceCharacters ("0123456789", "QWXZJPVYHB");   // digits could spell a banned number
                calibrate (p);
                if (juce::String why; ! vault::validatePreset (p, &why)) { d.res.fail = "invalid: " + why; std::printf ("  synth INVALID %s %s\n", d.cand.recipe.toRawUTF8(), why.toRawUTF8()); continue; }
                for (int attempt = 0; attempt < 6; ++attempt)
                {
                    d.res = recipegate::Result();
                    d.res.preset = p;
                    recipegate::analyse (d.res);
                    if (d.res.fail.isEmpty() && d.res.share25 > ownMax25) d.res.fail = "harsh-2-5k";
                    if (d.res.fail.isEmpty() && d.res.share6k > ownMax6k) d.res.fail = "piercing-highs";
                    if (d.res.fail.isEmpty() && d.res.velRange > ownMaxVel) d.res.fail = "velocity-wide";
                    if (d.res.fail.isEmpty() || d.res.fail.startsWith ("name") || d.res.fail == "label") break;
                    if (! repair (p, d.res.fail, attempt)) break;
                    calibrate (p);
                    ++d.repairs;
                }
                if (d.res.fail.isEmpty()) d.own = ownPrint (p);
                if (++count % 100 == 0) std::printf ("  synth %d / %d\n", count.load(), (int) cands.size());
            }
        });
    for (auto& t : pool) t.join();

    // greedy selection in candidate order: gate passed + distinct from everything accepted on BOTH fingerprints
    std::vector<int> acc;
    std::map<juce::String, int> rejects;
    std::map<int, int> catCount;
    // category sizes: a balanced bank (candidates are interleaved over voices, so the first N accepted cover every voice)
    auto catCap = [] (int c) { return c == catPlucks ? envInt ("SYNTH_CAP_PLUCKS", 300) : c == catSynth ? envInt ("SYNTH_CAP_SYNTH", 270) : 100000; };
    std::vector<float> nearShared (done.size(), 1e9f), nearOwn (done.size(), 1e9f);
    std::vector<juce::String> why (done.size());
    for (int i = 0; i < (int) done.size(); ++i)
    {
        auto& d = done[(size_t) i];
        if (d.res.fail.isNotEmpty()) { why[(size_t) i] = d.res.fail; ++rejects[d.res.fail]; continue; }
        float ms = 1e9f, mo = 1e9f;
        for (int j : acc)
        {
            ms = std::min (ms, recipegate::distance (d.res.print, done[(size_t) j].res.print));
            mo = std::min (mo, ownDistance (d.own, done[(size_t) j].own));
        }
        nearShared[(size_t) i] = ms; nearOwn[(size_t) i] = mo;
        if (ms < sharedMin || mo < ownMin) { why[(size_t) i] = "near-duplicate"; ++rejects["near-duplicate"]; continue; }
        if (++catCount[d.cand.preset.category] > catCap (d.cand.preset.category)) { why[(size_t) i] = "category full"; ++rejects["category full"]; continue; }
        acc.push_back (i);
    }

    // names, ids
    Namer namer;
    std::vector<VaultPreset> out;
    std::vector<int> outIdx;
    std::map<int, int> perCat;
    for (int i : acc)
    {
        auto& d = done[(size_t) i];
        VaultPreset p = d.cand.preset;
        juce::String name;
        for (auto& n : d.cand.nameOptions) if (namer.take (n)) { name = n; break; }
        if (name.isEmpty())
            for (auto& w : spareWords (p.category))
            {
                for (auto& n : d.cand.nameOptions)
                {
                    const auto noun = n.fromLastOccurrenceOf (" ", false, false);
                    const auto tryName = joinWords ({ w, n.upToLastOccurrenceOf (" ", false, false).fromLastOccurrenceOf (" ", false, false), noun });
                    if (namer.take (tryName)) { name = tryName; break; }
                    if (namer.take (w + " " + n)) { name = w + " " + n; break; }
                }
                if (name.isNotEmpty()) break;
            }
        if (name.isEmpty()) { why[(size_t) i] = "no name"; ++rejects["no name"]; continue; }
        p.name = name;
        const int n = ++perCat[p.category];
        p.id = juce::String ("cv.") + vault::categorySlug (p.category) + "." + juce::String (n).paddedLeft ('0', 4);
        p.gain = 1.0f;
        out.push_back (p);
        outIdx.push_back (i);
        d.cand.preset = p;
    }

    float minOwn = 1e9f, minShared = 1e9f;
    juce::String ownA, ownB, shA, shB;
    for (size_t x = 0; x < outIdx.size(); ++x)
        for (size_t y = x + 1; y < outIdx.size(); ++y)
        {
            auto& a = done[(size_t) outIdx[x]]; auto& b = done[(size_t) outIdx[y]];
            const float o = ownDistance (a.own, b.own), s = recipegate::distance (a.res.print, b.res.print);
            if (o < minOwn) { minOwn = o; ownA = a.cand.preset.name; ownB = b.cand.preset.name; }
            if (s < minShared) { minShared = s; shA = a.cand.preset.name; shB = b.cand.preset.name; }
        }
    std::printf ("synth: accepted %d of %d (Plucks %d, Leads %d, Pads %d, Synth %d, FX & Atmos %d)\n", (int) out.size(), (int) done.size(), perCat[catPlucks],
                 perCat[catLeads], perCat[catPads], perCat[catSynth], perCat[catFxAtmos]);
    std::printf ("synth: min pairwise own distance %.2f (%s / %s), shared %.2f (%s / %s); thresholds own %.2f shared %.2f\n",
                 minOwn, ownA.toRawUTF8(), ownB.toRawUTF8(), minShared, shA.toRawUTF8(), shB.toRawUTF8(), ownMin, sharedMin);
    for (auto& [w, n] : rejects) std::printf ("  synth rejected %-16s %d\n", w.toRawUTF8(), n);
    {
        std::map<int, int> layersCount;
        for (auto& p : out) ++layersCount[(int) p.parts.size()];
        std::printf ("synth: parts per preset:");
        for (auto& [k, n] : layersCount) std::printf (" %d-part %d", k, n);
        std::printf ("\n");
    }
    for (int cat : { (int) catPlucks, (int) catLeads, (int) catPads, (int) catSynth, (int) catFxAtmos })
    {
        std::vector<float> h1, s25, s6, cen, vel, tail;
        for (int i : outIdx)
        {
            auto& r = done[(size_t) i];
            if (r.cand.preset.category != cat) continue;
            h1.push_back (r.res.h1Db); s25.push_back (100 * r.res.share25); s6.push_back (100 * r.res.share6k); cen.push_back (r.res.centroid);
            vel.push_back (r.res.velRange); tail.push_back (r.res.tail);
        }
        auto stat = [] (std::vector<float> v) { if (v.empty()) return juce::String ("-"); std::sort (v.begin(), v.end());
                                               return juce::String (v.front(), 1) + " / " + juce::String (v[v.size() / 2], 1) + " / " + juce::String (v.back(), 1); };
        std::printf ("  %s (min / median / max): H1-vs-strongest dB %s | 2-5 kHz %% %s | >6 kHz %% %s | centroid Hz %s | vel 0.35->1.0 dB %s | tail s %s\n",
                     vault::categoryNames()[cat].toRawUTF8(), stat (h1).toRawUTF8(), stat (s25).toRawUTF8(), stat (s6).toRawUTF8(), stat (cen).toRawUTF8(),
                     stat (vel).toRawUTF8(), stat (tail).toRawUTF8());
    }
    if (auto rp = juce::SystemStats::getEnvironmentVariable ("SYNTH_REPORT", {}); rp.isNotEmpty())
    {
        juce::String s ("idx\trecipe\tresult\tname\tid\tparts\trepairs\tloud\tH1\t2-5k%\t>6k%\tcentroid\ttail\tvel\toctUp\tnearShared\tnearOwn\n");
        for (int i = 0; i < (int) done.size(); ++i)
        {
            auto& d = done[(size_t) i];
            const bool ok = why[(size_t) i].isEmpty() && std::find (outIdx.begin(), outIdx.end(), i) != outIdx.end();
            s << i << "\t" << d.cand.recipe << "\t" << (ok ? juce::String ("ok") : why[(size_t) i]) << "\t" << (ok ? d.cand.preset.name : juce::String()) << "\t"
              << (ok ? d.cand.preset.id : juce::String()) << "\t" << (int) d.cand.preset.parts.size() << "\t" << d.repairs << "\t" << juce::String (d.res.loud, 1) << "\t"
              << juce::String (d.res.h1Db, 1) << "\t" << juce::String (100 * d.res.share25, 2) << "\t" << juce::String (100 * d.res.share6k, 2) << "\t"
              << (int) d.res.centroid << "\t" << juce::String (d.res.tail, 2) << "\t" << juce::String (d.res.velRange, 1) << "\t"
              << juce::String (d.res.octUpDb, 1) << "\t" << juce::String (nearShared[(size_t) i] < 1e8f ? nearShared[(size_t) i] : 0.0f, 2) << "\t"
              << juce::String (nearOwn[(size_t) i] < 1e8f ? nearOwn[(size_t) i] : 0.0f, 2) << "\n";
        }
        juce::File (rp).replaceWithText (s);
        std::printf ("synth: report %s\n", rp.toRawUTF8());
    }
    return out;
}

VAULT_RECIPE (synth, build_synth);
