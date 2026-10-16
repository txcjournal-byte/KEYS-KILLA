#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

// CHROME VAULT by TrapWin - preset bank data model and the compact "CVB" bank file format (v3; v1 / v2 files are still read).
// Light header: only juce_core. See docs/VAULT.md.

//==============================================================================
// What plays a part (layer):
//   synth  - a complete EVOLVE synth patch (kk::SynthEngine), `values` = EVOLVE parameter IDs (Source/Params.h)
//   sample - a multi-sample instrument of the sample library: `source` = instrument id (library.json), `articulation` =
//            articulation name (empty = the instrument's first), `values` = sample parameters ("s.*", vault::sampleParams())
//   model  - a physical / modelled instrument: `source` = model id (vault::models registry), `values` = model parameters ("m.*")
// Any part kind may also carry EVOLVE FX keys (revMix, delayMix, chorus, drive, eqLow, eqHigh, width, master, gain, ...):
// as in v1 the master FX rack is built from PART 0's values, and every part's `gain` (dB) is relative to part 0's.
enum class VaultPartKind : uint8_t { synth = 0, sample = 1, model = 2 };

struct VaultPart
{
    VaultPartKind kind = VaultPartKind::synth;
    juce::String source;                                    // sample: instrument id, model: model id, synth: empty
    juce::String articulation;                              // sample: articulation name (empty = first)
    std::vector<std::pair<juce::String, float>> values;     // REAL parameter values; anything missing = default
    float level = 1.0f;     // linear gain of this part (0..2)
    float pan = 0.0f;       // -1 (left) .. +1 (right), equal-power balance
    int octave = 0;         // MIDI transpose of this part in octaves (-3..3)
    int semi = 0;           // and semitones (-12..12)
    juce::String label;     // UI name of the layer, e.g. "Concert Grand"

    float get (const juce::String& id, float fallback) const
    {
        for (auto& [k, v] : values) if (k == id) return v;
        return fallback;
    }
    void set (const juce::String& id, float v)
    {
        for (auto& kv : values) if (kv.first == id) { kv.second = v; return; }
        values.push_back ({ id, v });
    }
    bool operator== (const VaultPart& o) const
    {
        return kind == o.kind && source == o.source && articulation == o.articulation && values == o.values
            && juce::exactlyEqual (level, o.level) && juce::exactlyEqual (pan, o.pan) && octave == o.octave && semi == o.semi && label == o.label;
    }
};

//==============================================================================
// v3: the per-preset sound design around the layers - FX chain, modulation, 4 named sound macros, arpeggiator, gate.
// Parameter ranges / names of every FX type: vault::fxTypeInfo(). Modulation and macro targets are text ids
// (vault::describeTarget, docs/VAULT.md "Targets"), resolved by the engine when a preset loads.
namespace vault
{
// FX slot types (stored as numbers in banks: append only, never renumber)
enum class FxType : uint8_t { eq = 0, compressor, drive, lofi, chorus, phaser, flanger, filter, delay, reverb, width, reverse,
                              transient, body, ghost, stutter, halftime, numTypes };
enum { numFxTypes = (int) FxType::numTypes, maxFxSlots = 8, maxFxParams = 10, maxSameFxType = 2,
       numLfos = 2, maxLfoDests = 4, maxRoutings = 4, numSoundMacros = 4, maxMacroTargets = 8, numSteps = 32 };   // numSteps: arp / gate lanes (CVB v3 stored 16)
} // namespace vault

struct VaultFxSlot
{
    vault::FxType type = vault::FxType::reverb;
    bool on = true;
    std::array<float, vault::maxFxParams> p {};      // real values in the order of vault::fxTypeInfo (type).params

    static VaultFxSlot make (vault::FxType t);       // every parameter at its default
    float get (const char* id) const;                // the default for an unknown id
    VaultFxSlot& set (const char* id, float v);      // unknown ids are ignored (jassert)
    bool operator== (const VaultFxSlot& o) const;
};

struct VaultModDest                                  // one modulation route: target id + amount (units: docs/VAULT.md "Targets")
{
    juce::String target;
    float amount = 0;
    bool operator== (const VaultModDest& o) const { return target == o.target && juce::exactlyEqual (amount, o.amount); }
};

struct VaultLfo
{
    enum Shape : uint8_t { sine, triangle, square, saw, sampleHold, numShapes };
    uint8_t shape = sine;
    float rate = 2.0f;                               // Hz (free running)
    bool sync = false;                               // true: `div` (vault::divisions) follows the host tempo
    uint8_t div = 0;                                 // index into vault::divisions(), set to "1/4" by the constructor
    float delay = 0, fade = 0;                       // seconds after the note-on before the LFO starts / reaches full depth
    float phase = 0;                                 // start phase 0..1
    bool retrig = true;                              // true: restarts (phase, delay, fade) with the first note; false: free / bar-aligned
    float depth = 1.0f;                              // 0..1 scales every destination (macro / mod target "lfo1.depth")
    std::vector<VaultModDest> dests;                 // up to vault::maxLfoDests
    VaultLfo();
    bool operator== (const VaultLfo& o) const;
};

struct VaultMod
{
    VaultLfo lfo[vault::numLfos];
    std::vector<VaultModDest> modWheel, velocity, aftertouch;   // up to vault::maxRoutings each
    bool operator== (const VaultMod& o) const;
};

struct VaultMacroTarget
{
    juce::String target;
    float min = 0, max = 1;                          // the target's value (absolute targets) or offset (relative targets) at macro 0 / 1
    float curve = 0;                                 // -1..1: 0 linear, > 0 slow start (exponential), < 0 fast start
    bool operator== (const VaultMacroTarget& o) const
    { return target == o.target && juce::exactlyEqual (min, o.min) && juce::exactlyEqual (max, o.max) && juce::exactlyEqual (curve, o.curve); }
};
struct VaultMacro
{
    juce::String name;                               // shown on the MAIN page ("Hammer", "Air", ...); empty = unused knob
    std::vector<VaultMacroTarget> targets;           // up to vault::maxMacroTargets
    bool operator== (const VaultMacro& o) const { return name == o.name && targets == o.targets; }
};

struct VaultArp
{
    // note order (stored as numbers: append only). Up / Down / Up-Down / Down-Up / Up & Down (ends repeated) / Converge
    // (outside in) / Diverge (inside out) / Thumb (lowest note between the others) / Pinky (highest note between) run over
    // the held notes sorted by pitch; As Played in the order the keys went down; Random picks from them (never the same
    // note twice in a row when there is a choice); Chord plays every held note on each step.
    enum Mode : uint8_t { up, down, upDown, asPlayed, random, chord, downUp, upAndDown, converge, diverge, thumb, pinky, numModes };
    // how OCTAVES (> 1) extend the notes: Up / Down = the order runs over the held notes copied 1..3 octaves up / down;
    // Up-Down = each pass of the order one octave higher, then back down (0 1 2 1 ...); Alternate = every note followed by
    // its octave copies (C C' E E' G G')
    enum OctMode : uint8_t { octUp, octDown, octUpDown, octAlternate, numOctModes };
    // the velocity of a step: played x the VELOCITY lane (the classic), the lane alone, the played velocity alone
    enum VelMode : uint8_t { velPlayedPattern, velPattern, velPlayed, numVelModes };
    // when the order starts again from its first note: never (it runs on through chord changes), on a new chord (keys
    // pressed after all were up; also the free clock restarts from the key), on every new key, on every bar line
    enum Retrig : uint8_t { retrigOff, retrigChord, retrigNote, retrigBar, numRetrigs };
    enum LatchMode : uint8_t { latchReplace, latchAdd };   // latch: a new chord replaces the held one / adds to it
    enum StepType : uint8_t { rest = 0, play = 1, tie = 2 };
    bool on = false;
    uint8_t mode = up;
    uint8_t rate = 0;                                // vault::divisions() index, "1/16" by the constructor
    uint8_t octaves = 1;                             // 1..4
    float gate = 0.8f;                               // note length, share of a step (0.01..1; x the LENGTH lane; ties extend over steps)
    float swing = 0;                                 // 0..1: every second step later by up to half a step (50 % -> 75 %)
    bool latch = false;
    uint8_t steps = 16;                              // pattern length 1..32
    std::array<float, vault::numSteps> vel;          // 0..1 (see velMode)
    std::array<uint8_t, vault::numSteps> type;       // StepType
    std::array<int8_t, vault::numSteps> oct;         // -2..2 octaves
    // CVB v4
    uint8_t octMode = octUp, velMode = velPlayedPattern, retrig = retrigChord, latchMode = latchReplace;
    std::array<float, vault::numSteps> len;          // 0.05..1: the step's length x GATE (1 = GATE as set)
    std::array<uint8_t, vault::numSteps> slide;      // 1 = the note lasts into the next note (legato: mono / legato layers glide)
    VaultArp();                                      // "Init": Up, 1/16, 1 octave, 80 %, every step on at velocity 1
    bool operator== (const VaultArp& o) const;
};

struct VaultGate
{
    bool on = false;
    uint8_t rate = 0;                                // vault::divisions() index, "1/16" by the constructor
    uint8_t steps = 16;                              // 1..32
    std::array<float, vault::numSteps> level;        // 0..1 per step (Init: on / off alternating); steps 1..32
    float depth = 1.0f;                              // 0..1: how far a closed step closes
    float smooth = 0.25f;                            // 0..1: edge time, share of a step
    float swing = 0;                                 // 0..1 like the arpeggiator
    float mix = 1.0f;                                // 0..1 dry / gated
    VaultGate();
    bool operator== (const VaultGate& o) const;
};

struct VaultPreset
{
    juce::String id;                     // stable unique id, e.g. "cv.piano.0001" (saved in host projects - never change)
    juce::String name;                   // display name (English, no real brand / product / company names)
    int category = 0;                    // 0..vault::numCategories-1 (vault::categoryNames())
    std::vector<VaultPart> parts;        // 1..4 layers
    float gain = 1.0f;                   // loudness trim, linear, applied after the FX
    float macroDefaults[4] { 0.5f, 0.5f, 0.5f, 0.5f };   // the 4 sound macros' values when the preset loads (0..1)

    // ---- v3 (`v3` = true: these sections define the sound; false = a v1 / v2 style preset whose FX come from part 0's
    //      EVOLVE FX keys and whose 4 macros are the classic TONE / SPACE / DRIVE / MOTION - vault::upgradeToV3 converts it,
    //      the engine does that by itself when it compiles one)
    bool v3 = false;
    std::vector<VaultFxSlot> fx;         // the FX chain after the layers, in order, up to vault::maxFxSlots
    float finish = 0;                    // output "finish" (glue compression, air, mono-safe widening) 0..1
    VaultMod mod;
    std::array<VaultMacro, 4> macros;    // the 4 sound macros (name + targets), values: macroDefaults / the plugin's macro knobs
    VaultArp arp;
    VaultGate gate;

    bool usesSamples() const { for (auto& p : parts) if (p.kind == VaultPartKind::sample) return true; return false; }
    bool operator== (const VaultPreset& o) const
    {
        return id == o.id && name == o.name && category == o.category && parts == o.parts
            && juce::exactlyEqual (gain, o.gain)
            && std::equal (std::begin (macroDefaults), std::end (macroDefaults), std::begin (o.macroDefaults), [] (float a, float b) { return juce::exactlyEqual (a, b); })
            && v3 == o.v3 && fx == o.fx && juce::exactlyEqual (finish, o.finish) && mod == o.mod && macros == o.macros && arp == o.arp && gate == o.gate;
    }
};

namespace vault
{
enum { numCategories = 16, maxParts = 4, numMacros = 4 };
enum Macro { macroTone, macroSpace, macroDrive, macroMotion };

// the 16 categories (v2), in this order
enum CategoryIndex { catPiano, catKeysOrgan, catGuitars, catBass, catStrings, catBrass, catFlutesWoodwinds, catOrchestral,
                     catBellsMallets, catVocalsChoir, catPlucks, catLeads, catPads, catSynth, catWorld, catFxAtmos };

inline const juce::StringArray& categoryNames()
{
    static const juce::StringArray n { "Piano", "Keys & Organ", "Guitars", "Bass", "Strings", "Brass", "Flutes & Woodwinds", "Orchestral",
                                       "Bells & Mallets", "Vocals & Choir", "Plucks", "Leads", "Pads", "Synth", "World", "FX & Atmos" };
    return n;
}
// short id slug per category (preset ids "cv.<slug>.<nnnn>")
inline const char* categorySlug (int c)
{
    static const char* s[numCategories] { "piano", "keys", "gtr", "bass", "str", "brass", "wind", "orch",
                                          "bell", "vox", "pluck", "lead", "pad", "synth", "world", "fx" };
    return s[juce::jlimit (0, (int) numCategories - 1, c)];
}
inline const juce::StringArray& macroNames()
{
    static const juce::StringArray n { "TONE", "SPACE", "DRIVE", "MOTION" };
    return n;
}
// The home note: everything is designed, previewed, rendered and labelled at MIDI 60, which FL Studio (and the
// CHROME VAULT keyboard) calls C5.
constexpr int homeNote = 60;
constexpr int octaveForMiddleC = 5;
inline juce::String noteName (int midi)   // MIDI 60 -> "C5"
{
    static const char* n[12] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (n[((midi % 12) + 12) % 12]) + juce::String (midi / 12 - 5 + octaveForMiddleC);
}

//==============================================================================
// Parameters of a SAMPLE part (`VaultPart::values`, keys "s.*"). Missing = default.
struct ParamInfo { const char* id; float lo, hi, def; const char* what; };
inline const std::vector<ParamInfo>& sampleParams()
{
    static const std::vector<ParamInfo> t {
        { "s.attack",    0.0f,   10.0f,  0.0f,    "extra attack time, s (0 = the sample's own onset)" },
        { "s.decay",     0.0f,   30.0f,  0.0f,    "decay time to s.sustain, s (0 = no decay stage: natural envelope)" },
        { "s.sustain",   0.0f,   1.0f,   1.0f,    "sustain level of the ADSR override (used when s.decay > 0)" },
        { "s.release",   0.0f,   20.0f,  0.0f,    "release time, s (0 = the articulation's release)" },
        { "s.velSens",   0.0f,   1.0f,   0.5f,    "velocity -> level on top of the velocity layers (0 = layers only)" },
        { "s.cutoff",    20.0f,  20000.0f, 20000.0f, "filter cutoff, Hz (20000 with low-pass = filter off)" },
        { "s.reso",      0.0f,   1.0f,   0.0f,    "filter resonance" },
        { "s.filterType",0.0f,   2.0f,   0.0f,    "0 low-pass, 1 high-pass, 2 band-pass (12 dB/oct SVF)" },
        { "s.velCutoff", 0.0f,   1.0f,   0.0f,    "soft notes darker: cutoff x 2^(-4 * amount * (1 - velocity))" },
        { "s.tune",      -100.0f, 100.0f, 0.0f,   "fine tune, cents" },
        { "s.start",     0.0f,   1000.0f, 0.0f,   "sample start offset, ms (skips the attack)" },
        { "s.bendRange", 0.0f,   24.0f,  2.0f,    "pitch-bend range, semitones" },
        { "s.vibrato",   0.0f,   50.0f,  0.0f,    "designed vibrato depth, cents (MOTION scales / adds)" },
        { "s.vibRate",   0.5f,   9.0f,   5.2f,    "vibrato rate, Hz" },
        { "s.width",     0.0f,   2.0f,   1.0f,    "stereo width of the part (0 mono, 1 as recorded)" },
        { "s.releaseSamples", 0.0f, 1.0f, 1.0f,   "play the articulation's release samples (1) or not (0)" },
        { "s.polyphony", 1.0f,   64.0f,  64.0f,   "voices of this part (the oldest is faded out when exceeded)" },
        { "s.oneShot",   -1.0f,  1.0f,   -1.0f,   "-1 = articulation default (a one-shot with s.attack >= 0.3 s releases), 0 = note-off releases, 1 = plays to the end" },
    };
    return t;
}
// indices into sampleParams() (same order)
enum SampleParam { spAttack, spDecay, spSustain, spRelease, spVelSens, spCutoff, spReso, spFilterType, spVelCutoff, spTune, spStart,
                   spBendRange, spVibrato, spVibRate, spWidth, spReleaseSamples, spPolyphony, spOneShot, numSampleParams };
// preset peak control (part 0, any kind): threshold in dBFS before the preset trim (VaultPreset::gain), -60..0, 0 = off.
// A 1 ms look-ahead peak compressor on the preset's output (hold 15 ms, release 50 ms, ratio 6; the preset plays 1 ms later) that softens the first
// milliseconds of very percussive sounds so they reach the category loudness without touching the safety limiter. The
// bank gate sets it (tools/vault/recipes/RecipeGate.h); designers normally leave it out.
constexpr const char* peakKey = "cv.peak";
// with it: the threshold follows the last note-on velocity, thr * vel^exp (0..4, 0 = fixed). The gate sets exp from the measured
// velocity response, so soft and hard notes lose the same share of their attack and the velocity response stays as designed.
constexpr const char* peakVelKey = "cv.peakvel";

inline int sampleParamIndex (const juce::String& id)
{
    const auto& t = sampleParams();
    for (int i = 0; i < (int) t.size(); ++i) if (id == t[(size_t) i].id) return i;
    return -1;
}

//==============================================================================
// v3 tables

// tempo divisions (LFO sync, delay time, arpeggiator / gate rate): name + length in beats (quarter notes)
struct Division { const char* name; double beats; };
const std::vector<Division>& divisions();
int divisionIndex (const char* name);            // -1 if unknown ("1/16", "1/8T", "1/4D", "1 bar", ...)
inline double divisionBeats (int index)
{
    const auto& d = divisions();
    return d[(size_t) juce::jlimit (0, (int) d.size() - 1, index)].beats;
}

// FX types: id (recipes, targets "fx.<id>.<param>"), display name, rank (the canonical chain position: the order the v2
// master FX ran in; a default slot a MAIN page knob needs is inserted by it) and the parameters
struct FxParamInfo
{
    const char* id; const char* name;
    float lo, hi, def;
    enum Flags : uint8_t { linear = 0, logScale = 1, integer = 2, toggle = 4 };
    uint8_t flags = linear;
    const char* choices = nullptr;               // integer parameters with names: "Hall|Plate|Cloud|Room|Shimmer"
    const char* unit = "";
    float toNorm (float v) const;                // 0..1 (log parameters on a log scale)
    float fromNorm (float n) const;
};
struct FxTypeInfo
{
    FxType type; const char* id; const char* name; int rank;
    std::vector<FxParamInfo> params;
    int indexOf (const char* paramId) const;     // -1 if unknown
};
const FxTypeInfo& fxTypeInfo (FxType t);
int fxTypeOf (const juce::String& id);           // "reverb" -> (int) FxType::reverb, -1 if unknown

// the main amount parameter of a type that a MAIN page EFFECTS knob drives (reverb / delay / chorus "mix", drive "amount",
// width "width", eq "tilt"); -1 for other types
int fxMainParam (FxType t);

// the MAIN page quick parameters (global, for any preset): 0..1, `neutral` = the sound as designed
enum Quick { qCutoff, qReso, qAttack, qRelease, qEnvAmt, qGlide, qVibrato, qTremolo, qReverb, qDelay, qChorus, qDrive,
             qWidth, qEqTilt, qPan, numQuick };
struct QuickInfo { const char* id; const char* name; float neutral; const char* what; };
const std::array<QuickInfo, numQuick>& quickInfo();

// The classic macros of v1 / v2 presets (TONE, SPACE, DRIVE, MOTION; 0.5 = as designed): converted presets keep them as
// sound macros with the single target "classic.tone" etc.
inline const juce::StringArray& classicMacroNames()
{
    static const juce::StringArray n { "Tone", "Space", "Drive", "Motion" };
    return n;
}

// Modulation / macro target ids (text). Absolute targets are set by a macro to its min..max; LFOs / routings add.
//   pitch | cutoff | reso | volume | pan                  global (semitones | octaves | +0..1 | gain x(1 + a) | +-1)
//   quick.<id>                                             a MAIN page knob (quick.reverb ...): 0..1
//   classic.tone|space|drive|motion                        the v2 macros (0..1, 0.5 = as designed)
//   layer<1-4>.level|pan|cutoff|reso|attack|release|tune   one layer (level x, pan +, cutoff / tune in octaves / semitones, ...)
//   fx.<type>.<param> | fx<1-8>.<param>                    the first slot of a type (or the default slot a MAIN knob uses) /
//                                                          the slot at a chain position; LFO amounts in 0..1 of the range
//   lfo<1-2>.depth | lfo<1-2>.rate                         depth 0..1 / rate (x 2^(4 a) for LFO amounts)
//   gate.depth | gate.mix | gate.smooth | arp.gate         0..1
//   finish                                                 the output finish amount 0..1
struct TargetRef
{
    enum Kind : uint8_t { none, pitch, cutoff, reso, volume, pan, quick, classic, layer, fxType, fxSlot, lfo, gate, arpGate, finish };
    enum LayerParam : uint8_t { lLevel, lPan, lCutoff, lReso, lAttack, lRelease, lTune, numLayerParams };
    enum LfoParam : uint8_t { lfoDepth, lfoRate };
    enum GateParam : uint8_t { gDepth, gMix, gSmooth };
    Kind kind = none;
    uint8_t a = 0, b = 0;                        // quick / classic / layer / lfo / gate index, fx type or slot ; param index
    bool valid() const { return kind != none; }
    bool absolute() const;                       // a macro sets it (else adds an offset)
};
TargetRef parseTarget (const juce::String& id);  // kind none for an unknown / malformed id
juce::String describeTarget (const juce::String& id);   // "Reverb Mix", "Layer 2 Level", "Pitch" ... (UI)

// v1 / v2 -> v3: the FX that part 0's EVOLVE FX keys made (FX keys removed from the parts), the classic macros as sound
// macros 1-4 ("classic.*" targets), arp / gate off. The converted preset sounds exactly like the old one. No-op for v3.
// A v2 preset whose FX used more than 8 stages (only EVOLVE sounds with the FUTURE macro far up) keeps the first 8 in rack
// order; `droppedSlots` (optional) receives how many stages were left out (0 for every preset of the v2 factory bank).
void upgradeToV3 (VaultPreset& p, int* droppedSlots = nullptr);

//==============================================================================
// Bank file "CVB" (see docs/VAULT.md). Version 3 is written (v1 / v2 presets are converted with upgradeToV3 first);
// version 2 and version 1 (synth-only, style / character tags, old category list) are still read and come back as v3.
//   "CVB1" | uint16 version | uint16 flags (bit0 = payload zlib/gzip) | uint32 presetCount | uint32 rawPayloadSize | payload
juce::MemoryBlock serializeBank (const std::vector<VaultPreset>& bank, bool compress = true);
// false (and `out` left empty) on a damaged / foreign file. `version` (optional) receives the file's format version.
// keepLegacy: v1 / v2 presets stay as they were stored (v3 = false) instead of being converted (tests / tools)
bool deserializeBank (const void* data, size_t size, std::vector<VaultPreset>& out, juce::String* error = nullptr, int* version = nullptr,
                      bool keepLegacy = false);
bool saveBankFile (const juce::File& file, const std::vector<VaultPreset>& bank);
bool loadBankFile (const juce::File& file, std::vector<VaultPreset>& out, juce::String* error = nullptr);
constexpr int bankFormatVersion = 4;   // 4: the v3 layout + the arp's octave / velocity / retrigger / latch modes, 32-step lanes, LENGTH + SLIDE lanes

// checks one preset against the format rules (1..4 parts, category range, finite values, known keys per part kind,
// sample parts name an instrument, model parts name a model, id / name not empty; v3: FX chain <= 8 slots, <= 2 of a type,
// parameters in range, valid targets, <= 4 LFO destinations / routings, <= 8 macro targets, arp / gate in range)
bool validatePreset (const VaultPreset& p, juce::String* why = nullptr);

//==============================================================================
// v1 compatibility (reading old banks)
namespace v1
{
enum CategoryIndex { cat808Bass, catBellsMallets, catPianoKeys, catPlucks, catLeads, catPads, catStrings, catBrassHorns,
                     catFlutesWinds, catGuitars, catVocalsChoir, catSynth, catArpsSequences, catOrchestral, catWorld, catFxAtmos };
// old category -> v2 category ("Piano & Keys" splits by the layer labels / name: piano words -> Piano, else Keys & Organ)
int toV2Category (int v1Category, const VaultPreset& p);
// v1 names / labels used the word "808": it is replaced by "Sub" (no "808" anywhere in v2)
juce::String cleanText (const juce::String& s);
} // namespace v1
} // namespace vault
