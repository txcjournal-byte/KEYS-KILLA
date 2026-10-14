#pragma once
// CHROME VAULT v2 preset recipes - the minimal shared framework.
//
// One recipe file per sound designer / category group: tools/vault/recipes/<name>.cpp. Each file
//   * defines   std::vector<VaultPreset> build_<name> (const RecipeContext&);
//   * registers it once:   VAULT_RECIPE (<name>, build_<name>);
// CMake compiles every tools/vault/recipes/*.cpp into VaultBankGen (file GLOB), the static registrar adds the function to
// recipeRegistry(), and VaultBankGen collects them:
//
//   VaultBankGen -recipe <name>|all [-out bank.cvb] [-report report.md] [-threads N] [-demo <dir> <id,id,...>]
//
// runs the recipe(s), renders every preset at C5 (MIDI 60, FL Studio naming) + C4 / C6 through the v2 quality gate
// (RecipeGate.h: clicks, clipping, silence, DC, mono-safe lows, tails, C5 comfort, velocity response, octave-up layers),
// normalises loudness per category, rejects near-duplicates (log-mel + cepstrum fingerprint) and writes the bank.
//
// A recipe only builds presets (VaultPreset, Source/vault/VaultTypes.h). It must leave `gain` at 1 (the gate sets it)
// and give every preset a unique id ("cv.<category slug>.<nnnn>"), an English name and instrument-name part labels.
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

#define VAULT_RECIPE(name, fn) static recipe::Registrar vaultRecipeRegistrar_##name (#name, fn)
