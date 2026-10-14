#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

// CHROME VAULT by TrapWin - preset bank data model and the compact "CVB" bank file format (v2; v1 files are still read).
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

struct VaultPreset
{
    juce::String id;                     // stable unique id, e.g. "cv.piano.0001" (saved in host projects - never change)
    juce::String name;                   // display name (English, no real brand / product / company names)
    int category = 0;                    // 0..vault::numCategories-1 (vault::categoryNames())
    std::vector<VaultPart> parts;        // 1..4 layers
    float gain = 1.0f;                   // loudness trim, linear, applied after the FX
    float macroDefaults[4] { 0.5f, 0.5f, 0.5f, 0.5f };   // TONE, SPACE, DRIVE, MOTION (0..1, 0.5 = exactly as designed)

    bool usesSamples() const { for (auto& p : parts) if (p.kind == VaultPartKind::sample) return true; return false; }
    bool operator== (const VaultPreset& o) const
    {
        return id == o.id && name == o.name && category == o.category && parts == o.parts
            && juce::exactlyEqual (gain, o.gain)
            && std::equal (std::begin (macroDefaults), std::end (macroDefaults), std::begin (o.macroDefaults), [] (float a, float b) { return juce::exactlyEqual (a, b); });
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
        { "s.oneShot",   -1.0f,  1.0f,   -1.0f,   "-1 = articulation default, 0 = note-off releases, 1 = plays to the end" },
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
// Bank file "CVB" (see docs/VAULT.md). Version 2 is written; version 1 (synth-only, style / character tags,
// old category list) is still read: tags are dropped, categories mapped (v1::toV2Category).
//   "CVB1" | uint16 version | uint16 flags (bit0 = payload zlib/gzip) | uint32 presetCount | uint32 rawPayloadSize | payload
juce::MemoryBlock serializeBank (const std::vector<VaultPreset>& bank, bool compress = true);
// false (and `out` left empty) on a damaged / foreign file. `version` (optional) receives the file's format version.
bool deserializeBank (const void* data, size_t size, std::vector<VaultPreset>& out, juce::String* error = nullptr, int* version = nullptr);
bool saveBankFile (const juce::File& file, const std::vector<VaultPreset>& bank);
bool loadBankFile (const juce::File& file, std::vector<VaultPreset>& out, juce::String* error = nullptr);
constexpr int bankFormatVersion = 2;

// checks one preset against the format rules (1..4 parts, category range, finite values, known keys per part kind,
// sample parts name an instrument, model parts name a model, id / name not empty)
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
