#pragma once
#include "../VaultProcessor.h"

// CHROME VAULT editor: the values the audio engine really uses, for the knobs.
//
// A stored value is only where the engine starts: per block it applies the 4 sound macros (absolute targets SET a value -
// e.g. "fx.reverb.mix" in most factory presets -, relative ones add an offset), the classic v2 macros, the EVOLVE macros of
// a synth layer and the MAIN page knobs (vault::quickInfo) on top. A knob that showed (and wrote) only the stored value
// would sit somewhere else than what is heard and could even be "dead" (a macro overwrites it every block). These helpers
// mirror the engine (VaultEngine::computeEffective / updateParams, no LFO / wheel / velocity modulation - that is the green
// ring) so every knob shows the effective value, and they write a wanted effective value back:
//   - a value a sound macro sets absolutely: that macro target's range is moved (the macro keeps working around it)
//   - otherwise the stored value is solved so the effective value is the wanted one (MAIN knob offsets stay)
namespace vui::val
{
// what the processor's host parameters are right now
struct Ctx
{
    float macro[vault::numMacros] { 0.5f, 0.5f, 0.5f, 0.5f };
    float quick[vault::numQuick] {};
    Ctx();
    static Ctx of (const VaultProcessor&);
    Ctx withQuick (int q, float v) const { auto c = *this; c.quick[q] = v; return c; }
};

float macroCurve (float m, float curve);                 // the engine's macro curve
float quickAmount (float designed, float q, float top);  // the engine's MAIN EFFECTS knob law

// the effect of the 4 sound macros at the current macro values (engine order: macro 1..4, each target in order, the last
// absolute set wins)
struct MacroRef { int macro = -1, index = -1; bool valid() const { return macro >= 0; } };
struct MacroEffect
{
    float glob[5] {};                                    // pitch (st), cutoff (oct), reso, volume, pan offsets
    float quickAdd[vault::numQuick] {};
    float classic[4] { 0.5f, 0.5f, 0.5f, 0.5f };
    float layerAdd[vault::maxParts][vault::TargetRef::numLayerParams] {};
    float layerAbs[vault::maxParts][2] {};                // level, pan
    MacroRef layerAbsRef[vault::maxParts][2];
    struct Fx { int slot; vault::FxType type; int param; float value; MacroRef ref; };   // slot -1 = the default slot of `type`
    std::vector<Fx> fx;
    float lfo[vault::numLfos][2] {};                      // depth, rate (when set)
    MacroRef lfoRef[vault::numLfos][2];
};
MacroEffect evalMacros (const VaultPreset&, const Ctx&);

//==============================================================================
// FX chain. `slot` = index into VaultPreset::fx, or -1 for the silent default slot the engine adds for a MAIN EFFECTS knob
// (reverb, delay, chorus, drive, width, EQ) when the sound has no slot of that type.
int firstSlotOfType (const VaultPreset&, vault::FxType);
float fxStored (const VaultPreset&, int slot, vault::FxType, int param);           // the slot's value, or the default slot's
int fxQuickKnob (const VaultPreset&, int slot, vault::FxType, int param);          // the MAIN knob that moves it, -1 = none
// the effective value (real units). `stored` / `macroSet` override the stored value / the value an absolute macro sets.
float fxValue (const VaultPreset&, const Ctx&, int slot, vault::FxType, int param, const float* stored = nullptr, const float* macroSet = nullptr);
MacroRef fxMacro (const VaultPreset&, const Ctx&, int slot, vault::FxType, int param);   // the macro target that sets it (or none)
// edit: make the effective value `v` (with `ctx` - set the MAIN knob to neutral first when it should be folded in)
void setFxValue (VaultPreset&, const Ctx&, int slot, int param, float v);

//==============================================================================
// layers
enum class LKey { level, pan, cutoff, reso, attack, release, fenv, glide };
bool layerHas (VaultPartKind, LKey);                     // the engine applies it to this kind of layer
juce::String layerStoredKey (VaultPartKind, LKey);       // "cutoff" / "s.cutoff" ... ("" for level / pan = VaultPart fields)
float layerStored (const VaultPart&, LKey);
float layerValue (const VaultPreset&, const Ctx&, int layer, LKey, const float* stored = nullptr, const float* macroSet = nullptr);
MacroRef layerMacro (const VaultPreset&, const Ctx&, int layer, LKey);              // level / pan set by a macro
void setLayerValue (VaultPreset&, const Ctx&, int layer, LKey, float v);
// the range a layer value is solved in (stored units)
std::pair<float, float> layerRange (VaultPartKind, LKey);

//==============================================================================
// LFO depth / rate (macro targets "lfo<n>.depth" / ".rate" set them absolutely)
float lfoValue (const VaultPreset&, const Ctx&, int lfo, bool rate);
void setLfoValue (VaultPreset&, const Ctx&, int lfo, bool rate, float v);

// move the range of an absolute macro target so that at the current macro value it gives `v` (clamped to lo..hi)
void moveMacroTarget (VaultPreset&, const Ctx&, MacroRef, float v, float lo, float hi);

//==============================================================================
// MAIN page knobs: what each one shows (the effective value of what it moves) and the mapping knob position <-> parameter
struct QuickView
{
    enum Kind { plainKnob, fxKnob, layerKnob, offKnob };
    Kind kind = plainKnob;
    int layer = -1;                       // layer kind: the reference layer (layer A when it can, see quickView)
    LKey key = LKey::cutoff;
    vault::FxType type = vault::FxType::reverb;
    int slot = -1;
    float lo = 0, hi = 1;                 // display range (real units)
    bool logScale = false;
    float skew = 1;                       // v = lo + (hi - lo) * n^skew
    float arcOrigin = 0;                  // where the value arc starts (0.5 = bipolar)
    juce::String why;                     // kind off: why the knob does nothing for this sound
    float toNorm (float v) const;
    float fromNorm (float n) const;
};
QuickView quickView (const VaultPreset&, int q);
float quickEffective (const VaultPreset&, const Ctx&, int q, const QuickView&);       // real units (plain: 0..1)
float quickPosition (const VaultPreset&, const Ctx&, int q);                           // the knob position 0..1
float quickParamFor (const VaultPreset&, const Ctx&, int q, float position);           // the parameter value for a position
juce::String quickText (const VaultPreset&, const Ctx&, int q, float position, bool fxOn, bool slotOn);
} // namespace vui::val
