#pragma once
// CHROME VAULT v2 preset recipes - the minimal shared framework.
//
// One recipe file per sound designer / category group: tools/vault/recipes/<name>.cpp. Each file
//   * defines   std::vector<VaultPreset> build_<name> (const RecipeContext&);
//   * registers it once:   VAULT_RECIPE (<name>, build_<name>);
// CMake compiles every tools/vault/recipes/*.cpp into VaultBankGen (file GLOB), the static registrar adds the function to
// recipeRegistry(), and VaultBankGen collects them:
//
//   VaultBankGen -recipe <name>|all [-out bank.cvb] [-report report.md] [-threads N] [-dist 3.5] [-demo <dir> <id,id,...>] [-prints f.tsv]
//
// runs the recipe(s), renders every preset at C5 (MIDI 60, FL Studio naming) + C4 / C6 through the v2 quality gate
// (RecipeGate.h: clicks, clipping, silence, DC, mono-safe lows, tails, C5 comfort, velocity response, octave-up layers),
// normalises loudness per category, rejects near-duplicates (log-mel + cepstrum fingerprint) and writes the bank.
//
// A recipe only builds presets (VaultPreset, Source/vault/VaultTypes.h). It must leave `gain` at 1 (the gate sets it)
// and give every preset a unique id ("cv.<category slug>.<nnnn>"), an English name and instrument-name part labels.
// Value keys: synth / FX keys are the EVOLVE parameter IDs of Source/Params.h (note: the macros are "macro1".."macro8",
// the time macro "timeMacro"), sample keys "s.*" (VaultTypes.h table), model keys "m.*" (docs/VAULT_MODELS.md).
//
// v3 sound design (docs/VAULT.md "v3 recipe API"): recipe::design (preset) converts the v2-style preset (FX keys of part 0
// -> chain, classic macros) and then sets the v3 sections:
//   recipe::design (p)
//       .fx ({ fx::eq (1.5f, 0, -1), fx::chorus (0.25f), fx::delay ("1/8D", 0.3f, 0.12f), fx::reverb (fx::Reverb::hall, 0.6f, 1.2f, 0.22f) })
//       .macro (0, macro ("Hammer").to ("layer1.attack", 1, -1).to ("fx.drive.amount", 0, 0.35f), 0.4f)
//       .lfo (0, lfo (VaultLfo::sine).sync ("1/4D").fade (0.8f).to ("cutoff", 0.4f))
//       .modWheel ("lfo1.depth", 1.0f);
#include "../../../Source/vault/VaultTypes.h"
#include "../../../Source/vault/VaultLibrary.h"
#include <functional>
#include <memory>
#include <vector>

struct RecipeContext
{
    std::shared_ptr<vault::SampleLibrary> library;   // the sample library the bank is built against (may be missing)

    bool hasInstrument (const juce::String& id) const
    {
        return library != nullptr && library->libraryFound() && library->find (id) != nullptr;
    }
    bool hasArticulation (const juce::String& id, const juce::String& articulation) const
    {
        if (! hasInstrument (id)) return false;
        for (auto& a : library->find (id)->desc.articulations) if (a.name == articulation) return true;
        return false;
    }
};

namespace recipe
{
using BuildFn = std::function<std::vector<VaultPreset> (const RecipeContext&)>;
struct Entry { juce::String name; BuildFn build; };

inline std::vector<Entry>& registry()
{
    static std::vector<Entry> r;
    return r;
}
struct Registrar
{
    Registrar (const char* name, BuildFn fn) { registry().push_back ({ name, std::move (fn) }); }
};

// part helpers
inline VaultPart samplePart (const juce::String& instrument, const juce::String& articulation, const juce::String& label)
{
    VaultPart p;
    p.kind = VaultPartKind::sample; p.source = instrument; p.articulation = articulation; p.label = label;
    return p;
}
inline VaultPart modelPart (const juce::String& model, const juce::String& label)
{
    VaultPart p;
    p.kind = VaultPartKind::model; p.source = model; p.label = label;
    return p;
}
inline VaultPart synthPart (const juce::String& label)
{
    VaultPart p;
    p.kind = VaultPartKind::synth; p.label = label;
    return p;
}
} // namespace recipe

//==============================================================================
// v3 builders: FX slots, LFOs, macros, arp / gate, and recipe::design() that puts them on a preset
namespace recipe
{
namespace fx
{
enum class Reverb { hall = 0, plate, cloud, room, shimmer };
enum class Delay { pingPong = 0, straight, tape };
enum class Drive { tube = 0, tape, hard, blown, fold, bit };
enum class Filter { lowPass = 0, highPass, bandPass };
inline VaultFxSlot slot (vault::FxType t) { return VaultFxSlot::make (t); }
// size 0..1, decay = x the type's natural decay (0.25..4), mix 0..1
inline VaultFxSlot reverb (Reverb type, float size, float decay, float mix)
{ return slot (vault::FxType::reverb).set ("type", (float) type).set ("size", size).set ("decay", decay).set ("mix", mix); }
// division: "1/8D", "1/4T", "1/16" ... (vault::divisions()), tempo-synced
inline VaultFxSlot delay (const char* division, float feedback, float mix, Delay mode = Delay::pingPong)
{ return slot (vault::FxType::delay).set ("time", (float) juce::jmax (0, vault::divisionIndex (division))).set ("feedback", feedback).set ("mix", mix).set ("mode", (float) mode).set ("sync", 1.0f); }
inline VaultFxSlot delayMs (float ms, float feedback, float mix, Delay mode = Delay::straight)
{ return slot (vault::FxType::delay).set ("ms", ms).set ("sync", 0.0f).set ("feedback", feedback).set ("mix", mix).set ("mode", (float) mode); }
inline VaultFxSlot chorus (float mix, float rateHz = 0.33f, float depth = 0.5f, bool ensemble = false)
{ return slot (vault::FxType::chorus).set ("mix", mix).set ("rate", rateHz).set ("depth", depth).set ("mode", ensemble ? 1.0f : 0.0f); }
inline VaultFxSlot ensemble (float mix, float rateHz = 0.5f, float depth = 0.5f) { return chorus (mix, rateHz, depth, true); }
inline VaultFxSlot phaser (float mix, float rateHz = 0.35f, float depth = 1.0f, float feedback = 0.5f)
{ return slot (vault::FxType::phaser).set ("mix", mix).set ("rate", rateHz).set ("depth", depth).set ("feedback", feedback); }
inline VaultFxSlot flanger (float mix, float rateHz = 0.22f, float depth = 1.0f, float feedback = 0.65f)
{ return slot (vault::FxType::flanger).set ("mix", mix).set ("rate", rateHz).set ("depth", depth).set ("feedback", feedback); }
// shelves at 160 Hz / 6 kHz (set "lowFreq" / "highFreq" to move them), a peak at midHz; tilt = high +t, low -t/2
inline VaultFxSlot eq (float lowDb, float midDb, float highDb, float tiltDb = 0.0f, float midHz = 1000.0f)
{ return slot (vault::FxType::eq).set ("low", lowDb).set ("mid", midDb).set ("high", highDb).set ("tilt", tiltDb).set ("midFreq", midHz); }
inline VaultFxSlot tilt (float db) { return eq (0, 0, 0, db); }
inline VaultFxSlot comp (float thresholdDb, float ratio, float attackMs = 10.0f, float releaseMs = 150.0f, float makeupDb = 0.0f, float mix = 1.0f)
{ return slot (vault::FxType::compressor).set ("threshold", thresholdDb).set ("ratio", ratio).set ("attack", attackMs).set ("release", releaseMs).set ("makeup", makeupDb).set ("mix", mix); }
inline VaultFxSlot drive (Drive type, float amount, float mix = 1.0f, float toneHz = 20000.0f)
{ return slot (vault::FxType::drive).set ("type", (float) type).set ("amount", amount).set ("mix", mix).set ("tone", toneHz); }
inline VaultFxSlot lofi (float crush, float wow, float mix = 1.0f) { return slot (vault::FxType::lofi).set ("crush", crush).set ("wow", wow).set ("mix", mix); }
// envAmt -1..1 (+-4 octaves with the input level), lfoAmt 0..1 (+-2 octaves)
inline VaultFxSlot filter (Filter mode, float cutoffHz, float reso, float envAmt = 0.0f, float lfoAmt = 0.0f, float lfoRateHz = 1.0f)
{ return slot (vault::FxType::filter).set ("mode", (float) mode).set ("cutoff", cutoffHz).set ("reso", reso).set ("env", envAmt).set ("lfo", lfoAmt).set ("lfoRate", lfoRateHz); }
inline VaultFxSlot width (float width, float monoBelowHz = 0.0f) { return slot (vault::FxType::width).set ("width", width).set ("monoBelow", monoBelowHz); }
inline VaultFxSlot transient (float attack) { return slot (vault::FxType::transient).set ("attack", attack); }
inline VaultFxSlot reverse (float mix, const char* division = "1/8") { return slot (vault::FxType::reverse).set ("mix", mix).set ("length", (float) juce::jmax (0, vault::divisionIndex (division))); }
inline VaultFxSlot off (VaultFxSlot s) { s.on = false; return s; }   // in the chain, switched off (the user can turn it on)
} // namespace fx

struct LfoBuilder
{
    VaultLfo l;
    LfoBuilder& rate (float hz) { l.sync = false; l.rate = hz; return *this; }
    LfoBuilder& sync (const char* division) { l.sync = true; l.div = (uint8_t) juce::jmax (0, vault::divisionIndex (division)); return *this; }
    LfoBuilder& delay (float s) { l.delay = s; return *this; }
    LfoBuilder& fade (float s) { l.fade = s; return *this; }
    LfoBuilder& phase (float p) { l.phase = p; return *this; }
    LfoBuilder& free() { l.retrig = false; return *this; }
    LfoBuilder& depth (float d) { l.depth = d; return *this; }
    LfoBuilder& to (const char* target, float amount) { l.dests.push_back ({ target, amount }); return *this; }
    operator VaultLfo() const { return l; }
};
inline LfoBuilder lfo (VaultLfo::Shape shape = VaultLfo::sine) { LfoBuilder b; b.l.shape = (uint8_t) shape; return b; }

struct MacroBuilder
{
    VaultMacro m;
    // min / max: the target's value (absolute targets) or offset (relative ones) at macro 0 / 1; curve -1..1
    MacroBuilder& to (const char* target, float min, float max, float curve = 0.0f) { m.targets.push_back ({ target, min, max, curve }); return *this; }
    operator VaultMacro() const { return m; }
};
inline MacroBuilder macro (const char* name) { MacroBuilder b; b.m.name = name; return b; }

// arpeggiator / gate settings ("Init" = VaultArp() / VaultGate()); they stay off unless `on`
inline VaultArp arp (VaultArp::Mode mode, const char* division, int octaves = 1, float gate = 0.8f, float swing = 0.0f, bool on = false)
{
    VaultArp a; a.mode = (uint8_t) mode; a.rate = (uint8_t) juce::jmax (0, vault::divisionIndex (division)); a.octaves = (uint8_t) juce::jlimit (1, 4, octaves);
    a.gate = gate; a.swing = swing; a.on = on;
    return a;
}
inline VaultGate gate (const char* division, std::initializer_list<float> steps, float depth = 1.0f, float smooth = 0.25f, float mix = 1.0f, bool on = false)
{
    VaultGate g; g.rate = (uint8_t) juce::jmax (0, vault::divisionIndex (division)); g.depth = depth; g.smooth = smooth; g.mix = mix; g.on = on;
    int i = 0;
    for (float v : steps) if (i < vault::numSteps) g.level[(size_t) i++] = v;
    g.steps = (uint8_t) juce::jlimit (1, (int) vault::numSteps, i);
    return g;
}

// the v3 sections of one preset (converts a v2-style preset first, so its FX keys / classic macros are kept unless replaced)
struct Design
{
    VaultPreset& p;
    explicit Design (VaultPreset& preset) : p (preset) { vault::upgradeToV3 (p); }
    Design& fx (std::vector<VaultFxSlot> chain) { p.fx = std::move (chain); return *this; }
    Design& addFx (VaultFxSlot s)   // at its canonical place (the v2 rack order)
    {
        const int rank = vault::fxTypeInfo (s.type).rank;
        auto at = p.fx.end();
        for (auto it = p.fx.begin(); it != p.fx.end(); ++it) if (vault::fxTypeInfo (it->type).rank > rank) { at = it; break; }
        p.fx.insert (at, s);
        return *this;
    }
    Design& finish (float amount) { p.finish = amount; return *this; }
    // sound macro 0..3; `value` = where the knob sits when the preset loads
    Design& macro (int index, VaultMacro m, float value = 0.5f) { p.macros[(size_t) index] = std::move (m); p.macroDefaults[index] = value; return *this; }
    Design& clearMacros() { for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; } return *this; }
    Design& lfo (int index, VaultLfo l) { p.mod.lfo[index] = std::move (l); return *this; }
    Design& modWheel (const char* target, float amount) { p.mod.modWheel.push_back ({ target, amount }); return *this; }
    Design& velocity (const char* target, float amount) { p.mod.velocity.push_back ({ target, amount }); return *this; }
    Design& aftertouch (const char* target, float amount) { p.mod.aftertouch.push_back ({ target, amount }); return *this; }
    Design& arp (VaultArp a) { p.arp = a; return *this; }
    Design& gate (VaultGate g) { p.gate = g; return *this; }
};
inline Design design (VaultPreset& p) { return Design (p); }
} // namespace recipe

#define VAULT_RECIPE(name, fn) static recipe::Registrar vaultRecipeRegistrar_##name (#name, fn)
