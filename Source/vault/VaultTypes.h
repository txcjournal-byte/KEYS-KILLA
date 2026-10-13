#pragma once
#include <juce_core/juce_core.h>
#include <array>
#include <cstdint>
#include <utility>
#include <vector>

// CHROME VAULT by TrapWin - preset bank data model and the compact "CVB1" bank file format.
// Light header: only juce_core. Parameter IDs are the EVOLVE synth parameter IDs (Source/Params.h, Source/ParamKeys.h).

//==============================================================================
// One sound layer ("part") of a preset: a complete synth patch for one kk::SynthEngine.
struct VaultPart
{
    std::vector<std::pair<juce::String, float>> values;   // REAL parameter values (like kk Preset::values); anything missing = default
    float level = 1.0f;     // linear gain of this part (0..2)
    float pan = 0.0f;       // -1 (left) .. +1 (right), equal-power balance
    int octave = 0;         // MIDI transpose of this part in octaves (-3..3)
    int semi = 0;           // and semitones (-12..12)
    juce::String label;     // UI name of the layer, e.g. "Ice Bell"

    bool operator== (const VaultPart& o) const
    {
        return values == o.values && juce::exactlyEqual (level, o.level) && juce::exactlyEqual (pan, o.pan) && octave == o.octave && semi == o.semi && label == o.label;
    }
};

struct VaultPreset
{
    juce::String id;                     // stable unique id, e.g. "cv.808.0001" (saved in host projects - never change)
    juce::String name;                   // display name (English, no real brand / product / company names)
    int category = 0;                    // 0..vault::numCategories-1 (vault::categoryNames())
    uint32_t styles = 0;                 // bitmask over vault::styleNames()      (bit i = style i)
    uint32_t chars = 0;                  // bitmask over vault::characterNames()  (bit i = character i)
    std::vector<VaultPart> parts;        // 1..4 layers
    float gain = 1.0f;                   // loudness trim, linear, applied after the FX (set by the bank generator)
    float macroDefaults[4] { 0.5f, 0.5f, 0.5f, 0.5f };   // TONE, SPACE, DRIVE, MOTION (0..1, 0.5 = exactly as designed)

    bool operator== (const VaultPreset& o) const
    {
        return id == o.id && name == o.name && category == o.category && styles == o.styles && chars == o.chars && parts == o.parts
            && juce::exactlyEqual (gain, o.gain)
            && std::equal (std::begin (macroDefaults), std::end (macroDefaults), std::begin (o.macroDefaults), [] (float a, float b) { return juce::exactlyEqual (a, b); });
    }
};

namespace vault
{
enum { numCategories = 16, numStyles = 8, numCharacters = 8, maxParts = 4, numMacros = 4 };
enum Macro { macroTone, macroSpace, macroDrive, macroMotion };

enum CategoryIndex { cat808Bass, catBellsMallets, catPianoKeys, catPlucks, catLeads, catPads, catStrings, catBrassHorns,
                     catFlutesWinds, catGuitars, catVocalsChoir, catSynth, catArpsSequences, catOrchestral, catWorld, catFxAtmos };
enum StyleIndex { styleTrap, styleRage, styleDrill, stylePluggnb, styleDark, styleEmo, styleLoFi, styleAmbient };
enum CharIndex  { charBright, charDark, charWarm, charWide, charHard, charSoft, charDirty, charClean };

// "808 & Bass", "Bells & Mallets", ... (16, in CategoryIndex order)
inline const juce::StringArray& categoryNames()
{
    static const juce::StringArray n { "808 & Bass", "Bells & Mallets", "Piano & Keys", "Plucks", "Leads", "Pads", "Strings", "Brass & Horns",
                                       "Flutes & Winds", "Guitars", "Vocals & Choir", "Synth", "Arps & Sequences", "Orchestral", "World", "FX & Atmos" };
    return n;
}
// planned number of presets per category in the full bank (sum = 5000)
inline int categoryTarget (int c)
{
    static const int t[numCategories] { 520, 480, 420, 420, 420, 400, 300, 240, 220, 240, 280, 300, 200, 160, 160, 240 };
    return t[juce::jlimit (0, (int) numCategories - 1, c)];
}
// planned layer mix of the full bank: index = number of parts - 1 (sum = 5000)
inline int layerCountTarget (int numParts)
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
inline const juce::StringArray& macroNames()
{
    static const juce::StringArray n { "TONE", "SPACE", "DRIVE", "MOTION" };
    return n;
}
inline juce::StringArray namesOfMask (uint32_t mask, const juce::StringArray& names)
{
    juce::StringArray out;
    for (int i = 0; i < names.size(); ++i) if (mask & (1u << i)) out.add (names[i]);
    return out;
}
inline uint32_t maskOfNames (const juce::StringArray& tags, const juce::StringArray& names)
{
    uint32_t m = 0;
    for (auto& t : tags) if (const int i = names.indexOf (t, true); i >= 0) m |= 1u << i;
    return m;
}

//==============================================================================
// Bank file "CVB1" (see docs/VAULT.md):
//   "CVB1" | uint16 version (1) | uint16 flags (bit0 = payload gzip-compressed) | uint32 presetCount | uint32 rawPayloadSize | payload
//   payload: uint16 keyCount, keyCount x UTF-8 param-ID strings (the file's own key table, parts index into it by uint8),
//            then per preset: id, name (UTF-8 strings), uint8 category, uint32 styles, uint32 chars, float gain, 4 x float macros,
//            uint8 partCount, per part: label (string), float level, float pan, int8 octave, int8 semi,
//            uint16 valueCount, valueCount x (uint8 key, float value).   All little-endian.
// Unknown parameter IDs are kept (the key table is written from the IDs actually used), so a bank always round-trips exactly.
juce::MemoryBlock serializeBank (const std::vector<VaultPreset>& bank, bool compress = true);
// false (and `out` left empty) on a damaged / foreign file
bool deserializeBank (const void* data, size_t size, std::vector<VaultPreset>& out, juce::String* error = nullptr);
bool saveBankFile (const juce::File& file, const std::vector<VaultPreset>& bank);
bool loadBankFile (const juce::File& file, std::vector<VaultPreset>& out, juce::String* error = nullptr);

// checks one preset against the format rules (1..4 parts, category range, finite values, id/name not empty)
bool validatePreset (const VaultPreset& p, juce::String* why = nullptr);
} // namespace vault
