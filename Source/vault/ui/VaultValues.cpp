#include "VaultValues.h"
#include "../../VoiceBuild.h"

// CHROME VAULT editor: effective values (see VaultValues.h). Every formula here mirrors Source/vault/VaultEngine.cpp
// (computeEffective / updateParams) and Source/VoiceBuild.h; VaultTests -uitest checks them against the running engine.
namespace vui::val
{
namespace
{
using T = vault::TargetRef;
inline float lowHalf (float m) { return std::min (1.0f, 2.0f * m); }
inline float upHalf (float m)  { return std::max (0.0f, 2.0f * m - 1.0f); }
inline bool nz (float x) { return std::abs (x) > 0.0f; }
inline float clamp01 (float x) { return juce::jlimit (0.0f, 1.0f, x); }

bool isAuxType (vault::FxType t)   // the types the engine adds a default slot for (VaultEngine.cpp auxTypes)
{
    using F = vault::FxType;
    return t == F::drive || t == F::chorus || t == F::eq || t == F::delay || t == F::reverb || t == F::width;
}
int quickOfType (vault::FxType t)
{
    using F = vault::FxType;
    switch (t)
    {
        case F::reverb: return vault::qReverb;
        case F::delay:  return vault::qDelay;
        case F::chorus: return vault::qChorus;
        case F::drive:  return vault::qDrive;
        case F::width:  return vault::qWidth;
        case F::eq:     return vault::qEqTilt;
        default:        return -1;
    }
}
float quickTop (int q)
{
    switch (q)
    {
        case vault::qReverb: return 0.7f;
        case vault::qDelay:  return 0.6f;
        case vault::qChorus: return 1.0f;
        case vault::qDrive:  return 0.8f;
        case vault::qWidth:  return 1.0f;
        default:             return 1.0f;
    }
}
// the MAIN knob value the engine uses (the parameter + sound macro offsets, clamped)
float quickNow (const Ctx& c, const MacroEffect& me, int q) { return clamp01 (c.quick[q] + me.quickAdd[q]); }

// a macro target resolved against the sound (like VaultEngine compileTarget / entryFor); false = the engine drops it
struct Resolved { T ref; int slot = -1; vault::FxType type = vault::FxType::reverb; int param = -1; };
bool resolve (const VaultPreset& p, const juce::String& id, Resolved& r)
{
    r.ref = vault::parseTarget (id);
    if (! r.ref.valid()) return false;
    if (r.ref.kind == T::fxSlot)
    {
        if (r.ref.a >= p.fx.size()) return false;
        r.slot = r.ref.a;
        r.type = p.fx[(size_t) r.slot].type;
        r.param = vault::fxTypeInfo (r.type).indexOf (id.fromFirstOccurrenceOf (".", false, false).toRawUTF8());
        return r.param >= 0;
    }
    if (r.ref.kind == T::fxType)
    {
        r.type = (vault::FxType) r.ref.a;
        r.param = r.ref.b;
        r.slot = firstSlotOfType (p, r.type);
        return r.slot >= 0 || isAuxType (r.type);
    }
    if (r.ref.kind == T::layer) return r.ref.a < p.parts.size();
    return true;
}

// the stored values of a synth layer as the engine has them (defaults, then the part's values, sanitised)
void synthValues (const VaultPart& part, float* vals)
{
    kk::vk::fillDefaults (vals);
    for (auto& [id, v] : part.values)
        if (const int key = kk::vk::keyOf (id); key >= 0) vals[key] = kk::vk::sanitize (key, v);
}
int synthKey (LKey k)
{
    switch (k)
    {
        case LKey::cutoff:  return kk::vk::cutoff;
        case LKey::reso:    return kk::vk::reso;
        case LKey::attack:  return kk::vk::attack;
        case LKey::release: return kk::vk::release;
        case LKey::fenv:    return kk::vk::fenv;
        case LKey::glide:   return kk::vk::glide;
        case LKey::level: case LKey::pan: default: return -1;
    }
}
int layerParamOf (LKey k)   // TargetRef layer parameter of a key (-1: none)
{
    switch (k)
    {
        case LKey::level:   return T::lLevel;
        case LKey::pan:     return T::lPan;
        case LKey::cutoff:  return T::lCutoff;
        case LKey::reso:    return T::lReso;
        case LKey::attack:  return T::lAttack;
        case LKey::release: return T::lRelease;
        case LKey::fenv: case LKey::glide: default: return -1;
    }
}

// bisection on a non-decreasing function over [0, 1]: the input whose output is nearest to `want`
template <typename F>
float solve01 (F&& f, float want)
{
    float lo = 0.0f, hi = 1.0f;
    const float flo = f (lo), fhi = f (hi);
    if (want <= flo) return lo;
    if (want >= fhi) return hi;
    for (int i = 0; i < 40; ++i)
    {
        const float mid = 0.5f * (lo + hi);
        if (f (mid) < want) lo = mid; else hi = mid;
    }
    return std::abs (f (lo) - want) <= std::abs (f (hi) - want) ? lo : hi;
}

VaultMacroTarget* targetOf (VaultPreset& p, MacroRef r)
{
    if (! r.valid() || ! juce::isPositiveAndBelow (r.macro, (int) vault::numSoundMacros)) return nullptr;
    auto& t = p.macros[(size_t) r.macro].targets;
    return juce::isPositiveAndBelow (r.index, (int) t.size()) ? &t[(size_t) r.index] : nullptr;
}
} // namespace

//==============================================================================
Ctx::Ctx() { for (int q = 0; q < vault::numQuick; ++q) quick[q] = vault::quickInfo()[(size_t) q].neutral; }

Ctx Ctx::of (const VaultProcessor& p)
{
    Ctx c;
    for (int m = 0; m < vault::numMacros; ++m) c.macro[m] = p.getMacro (m);
    for (int q = 0; q < vault::numQuick; ++q) c.quick[q] = p.getQuick (q);
    return c;
}

float macroCurve (float m, float c)
{
    m = clamp01 (m);
    if (c > 0.001f) return std::pow (m, 1.0f + 3.0f * c);
    if (c < -0.001f) return 1.0f - std::pow (1.0f - m, 1.0f - 3.0f * c);
    return m;
}

float quickAmount (float designed, float q, float top)
{
    if (juce::exactlyEqual (q, 0.5f)) return designed;
    if (q < 0.5f) return designed * (2.0f * q);
    return designed + (std::max (designed, top) - designed) * (2.0f * q - 1.0f);
}

MacroEffect evalMacros (const VaultPreset& p, const Ctx& c)
{
    MacroEffect e;
    for (int m = 0; m < vault::numSoundMacros; ++m)
    {
        int n = 0;
        const auto& targets = p.macros[(size_t) m].targets;
        for (int i = 0; i < (int) targets.size(); ++i)
        {
            const auto& tg = targets[(size_t) i];
            Resolved r;
            if (n >= vault::maxMacroTargets || ! std::isfinite (tg.min) || ! std::isfinite (tg.max) || ! resolve (p, tg.target, r)) continue;
            ++n;
            const float curve = std::isfinite (tg.curve) ? juce::jlimit (-1.0f, 1.0f, tg.curve) : 0.0f;
            const float v = tg.min + (tg.max - tg.min) * macroCurve (c.macro[m], curve);
            const MacroRef ref { m, i };
            switch (r.ref.kind)
            {
                case T::pitch:  e.glob[0] += v; break;
                case T::cutoff: e.glob[1] += v; break;
                case T::reso:   e.glob[2] += v; break;
                case T::volume: e.glob[3] += v; break;
                case T::pan:    e.glob[4] += v; break;
                case T::quick:  e.quickAdd[r.ref.a] += v; break;
                case T::classic: e.classic[r.ref.a] = v; break;
                case T::layer:
                    if (r.ref.b <= T::lPan) { e.layerAbs[r.ref.a][r.ref.b] = v; e.layerAbsRef[r.ref.a][r.ref.b] = ref; }
                    else e.layerAdd[r.ref.a][r.ref.b] += v;
                    break;
                case T::fxType: case T::fxSlot: e.fx.push_back ({ r.slot, r.type, r.param, v, ref }); break;
                case T::lfo: e.lfo[r.ref.a][r.ref.b] = v; e.lfoRef[r.ref.a][r.ref.b] = ref; break;
                case T::none: case T::gate: case T::arpGate: case T::finish: default: break;
            }
        }
    }
    return e;
}

//==============================================================================
int firstSlotOfType (const VaultPreset& p, vault::FxType t)
{
    for (int i = 0; i < (int) p.fx.size(); ++i) if (p.fx[(size_t) i].type == t) return i;
    return -1;
}

float fxStored (const VaultPreset& p, int slot, vault::FxType t, int param)
{
    if (! juce::isPositiveAndBelow (param, (int) vault::maxFxParams)) return 0.0f;
    if (juce::isPositiveAndBelow (slot, (int) p.fx.size())) return p.fx[(size_t) slot].p[(size_t) param];
    // the engine's default slot (VaultEngine.cpp: a silent slot until the MAIN knob is turned up)
    auto def = VaultFxSlot::make (t);
    if (param == 0 && (t == vault::FxType::chorus || t == vault::FxType::delay || t == vault::FxType::reverb || t == vault::FxType::drive)) return 0.0f;
    return def.p[(size_t) param];
}

int fxQuickKnob (const VaultPreset& p, int slot, vault::FxType t, int param)
{
    const int q = quickOfType (t);
    if (q < 0 || param != vault::fxMainParam (t)) return -1;
    return slot == firstSlotOfType (p, t) ? q : -1;   // the first slot of the type, or the default slot (-1 == -1)
}

MacroRef fxMacro (const VaultPreset& p, const Ctx& c, int slot, vault::FxType t, int param)
{
    MacroRef out;
    for (auto& f : evalMacros (p, c).fx)
        if (f.slot == slot && f.type == t && f.param == param) out = f.ref;
    return out;
}

float fxValue (const VaultPreset& p, const Ctx& c, int slot, vault::FxType t, int param, const float* stored, const float* macroSet)
{
    const auto& ti = vault::fxTypeInfo (t);
    if (! juce::isPositiveAndBelow (param, (int) ti.params.size())) return 0.0f;
    const auto me = evalMacros (p, c);
    // every parameter of the entry is needed for the classic macros (they read / write a few at once)
    std::array<float, vault::maxFxParams> v {};
    for (int i = 0; i < (int) ti.params.size(); ++i) v[(size_t) i] = fxStored (p, slot, t, i);
    if (stored != nullptr) v[(size_t) param] = *stored;
    bool touched = false;
    for (auto& f : me.fx)
        if (f.slot == slot && f.type == t) { v[(size_t) f.param] = f.value; touched = true; }
    if (macroSet != nullptr) { v[(size_t) param] = *macroSet; touched = true; }
    const bool first = slot == firstSlotOfType (p, t);
    if (first)
    {
        using F = vault::FxType;
        const float tone = me.classic[0], space = me.classic[1], drv = me.classic[2], mot = me.classic[3];
        const float toneD = 2.0f * tone - 1.0f;
        if (t == F::eq && nz (toneD)) { v[6] = juce::jlimit (-12.0f, 12.0f, v[6] + toneD * 5.0f); v[1] = juce::jlimit (-12.0f, 12.0f, v[1] - toneD * 2.5f); }
        if (t == F::reverb) { v[0] = clamp01 (v[0] * lowHalf (space) + upHalf (space) * 0.45f); v[1] = clamp01 (v[1] + upHalf (space) * 0.3f); }
        if (t == F::delay) v[0] = clamp01 (v[0] * lowHalf (space) + upHalf (space) * 0.25f);
        if (t == F::width) v[0] = clamp01 (v[0] * (0.5f + 0.5f * lowHalf (space)) + upHalf (space) * 0.4f);
        if (t == F::drive) v[0] = clamp01 (v[0] * lowHalf (drv) + upHalf (drv) * 0.7f);
        if (t == F::chorus) v[0] = clamp01 (v[0] * lowHalf (mot) + upHalf (mot) * 0.5f);
        if (t == F::phaser || t == F::flanger) v[0] *= lowHalf (mot);
        if (const int q = quickOfType (t); q >= 0)
        {
            const float qv = quickNow (c, me, q);
            if (t == F::eq) { if (! juce::exactlyEqual (qv, 0.5f)) v[0] = juce::jlimit (-12.0f, 12.0f, v[0] + (qv - 0.5f) * 18.0f); }
            else v[0] = quickAmount (v[0], qv, quickTop (q));
        }
    }
    const auto& in = ti.params[(size_t) param];
    float out = v[(size_t) param];
    if (touched || first) out = juce::jlimit (in.lo, in.hi, out);
    if (touched && (in.flags & (vault::FxParamInfo::integer | vault::FxParamInfo::toggle))) out = std::round (out);
    return out;
}

void moveMacroTarget (VaultPreset& p, const Ctx& c, MacroRef r, float want, float lo, float hi)
{
    auto* t = targetOf (p, r);
    if (t == nullptr) return;
    const float k = macroCurve (c.macro[r.macro], std::isfinite (t->curve) ? juce::jlimit (-1.0f, 1.0f, t->curve) : 0.0f);
    want = juce::jlimit (lo, hi, want);
    const float now = t->min + (t->max - t->min) * k;
    float mn = t->min + (want - now), mx = t->max + (want - now);
    // keep the macro's sweep where possible; at the edges of the range give up some of it, the value at the current
    // macro position is what counts
    if (mn < lo || mn > hi) { mn = juce::jlimit (lo, hi, mn); if (k > 1.0e-4f) mx = (want - mn * (1.0f - k)) / k; }
    if (mx < lo || mx > hi) { mx = juce::jlimit (lo, hi, mx); if (k < 1.0f - 1.0e-4f) mn = (want - mx * k) / (1.0f - k); }
    t->min = juce::jlimit (lo, hi, mn);
    t->max = juce::jlimit (lo, hi, mx);
}

void setFxValue (VaultPreset& p, const Ctx& c, int slot, int param, float want)
{
    if (! juce::isPositiveAndBelow (slot, (int) p.fx.size())) return;
    const auto t = p.fx[(size_t) slot].type;
    const auto& ti = vault::fxTypeInfo (t);
    if (! juce::isPositiveAndBelow (param, (int) ti.params.size())) return;
    const auto& in = ti.params[(size_t) param];
    const float wantN = in.toNorm (juce::jlimit (in.lo, in.hi, want));
    if (const auto ref = fxMacro (p, c, slot, t, param); ref.valid())
    {
        // a sound macro sets it: find the value the macro must give, move the macro's range there
        const auto snap = p;
        const float n = solve01 ([&] (float x) { const float s = in.fromNorm (x); return in.toNorm (fxValue (snap, c, slot, t, param, nullptr, &s)); }, wantN);
        moveMacroTarget (p, c, ref, in.fromNorm (n), in.lo, in.hi);
        return;
    }
    const auto snap = p;
    const float n = solve01 ([&] (float x) { const float s = in.fromNorm (x); return in.toNorm (fxValue (snap, c, slot, t, param, &s)); }, wantN);
    float s = in.fromNorm (n);
    if (in.flags & (vault::FxParamInfo::integer | vault::FxParamInfo::toggle)) s = std::round (s);
    p.fx[(size_t) slot].p[(size_t) param] = s;
}

//==============================================================================
bool layerHas (VaultPartKind k, LKey key)
{
    switch (key)
    {
        case LKey::level: case LKey::pan: return true;
        case LKey::cutoff: case LKey::reso: return true;               // models: the tone low-pass
        case LKey::attack: case LKey::release: return k != VaultPartKind::model;
        case LKey::fenv: case LKey::glide: default: return k == VaultPartKind::synth;
    }
}

juce::String layerStoredKey (VaultPartKind k, LKey key)
{
    if (k == VaultPartKind::synth)
        switch (key)
        {
            case LKey::cutoff: return "cutoff"; case LKey::reso: return "reso"; case LKey::attack: return "attack";
            case LKey::release: return "release"; case LKey::fenv: return "fenv"; case LKey::glide: return "glide";
            case LKey::level: case LKey::pan: default: return {};
        }
    if (k == VaultPartKind::sample)
        switch (key)
        {
            case LKey::cutoff: return "s.cutoff"; case LKey::reso: return "s.reso"; case LKey::attack: return "s.attack";
            case LKey::release: return "s.release";
            case LKey::level: case LKey::pan: case LKey::fenv: case LKey::glide: default: return {};
        }
    return {};
}

std::pair<float, float> layerRange (VaultPartKind k, LKey key)
{
    if (k == VaultPartKind::synth)
        if (const int sk = synthKey (key); sk >= 0) return { kk::vk::info (sk).lo, kk::vk::info (sk).hi };
    switch (key)
    {
        case LKey::level: return { 0.0f, 2.0f };
        case LKey::pan: return { -1.0f, 1.0f };
        case LKey::cutoff: return k == VaultPartKind::sample ? std::pair<float, float> { 20.0f, 20000.0f } : std::pair<float, float> { 40.0f, 20000.0f };
        case LKey::reso: return { 0.0f, 1.0f };
        case LKey::attack: return k == VaultPartKind::sample ? std::pair<float, float> { 0.0f, 10.0f } : std::pair<float, float> { 0.001f, 5.0f };
        case LKey::release: return k == VaultPartKind::sample ? std::pair<float, float> { 0.0f, 20.0f } : std::pair<float, float> { 0.005f, 8.0f };
        case LKey::fenv: return { -1.0f, 1.0f };
        case LKey::glide: default: return { 0.0f, 1.0f };
    }
}

float layerStored (const VaultPart& part, LKey key)
{
    if (key == LKey::level) return part.level;
    if (key == LKey::pan) return part.pan;
    if (part.kind == VaultPartKind::synth)
    {
        const int k = synthKey (key);
        return k >= 0 ? part.get (kk::vk::info (k).id, kk::vk::info (k).def) : 0.0f;
    }
    if (part.kind == VaultPartKind::sample)
    {
        const auto id = layerStoredKey (part.kind, key);
        const int i = vault::sampleParamIndex (id);
        return i >= 0 ? part.get (id, vault::sampleParams()[(size_t) i].def) : 0.0f;
    }
    if (key == LKey::cutoff) return 20000.0f;   // a modelled layer: the tone low-pass starts open
    return 0.0f;
}

MacroRef layerMacro (const VaultPreset& p, const Ctx& c, int layer, LKey key)
{
    if (! juce::isPositiveAndBelow (layer, (int) vault::maxParts) || (key != LKey::level && key != LKey::pan)) return {};
    return evalMacros (p, c).layerAbsRef[layer][key == LKey::level ? 0 : 1];
}

float layerValue (const VaultPreset& p, const Ctx& c, int layer, LKey key, const float* stored, const float* macroSet)
{
    if (! juce::isPositiveAndBelow (layer, (int) p.parts.size())) return 0.0f;
    const auto& part = p.parts[(size_t) layer];
    const auto me = evalMacros (p, c);
    if (key == LKey::level || key == LKey::pan)
    {
        const int i = key == LKey::level ? 0 : 1;
        float v = stored != nullptr ? *stored : layerStored (part, key);
        if (macroSet != nullptr) v = *macroSet;
        else if (me.layerAbsRef[layer][i].valid()) v = me.layerAbs[layer][i];
        return key == LKey::level ? juce::jlimit (0.0f, 2.0f, v) : juce::jlimit (-1.0f, 1.0f, v);
    }
    const float* la = me.layerAdd[layer];
    const float toneD = 2.0f * me.classic[0] - 1.0f;
    const float qCut = (quickNow (c, me, vault::qCutoff) - 0.5f) * 8.0f, qRes = (quickNow (c, me, vault::qReso) - 0.5f) * 2.0f;
    const float qEnv = (quickNow (c, me, vault::qEnvAmt) - 0.5f) * 2.0f, qGlide = (quickNow (c, me, vault::qGlide) - 0.5f) * 2.0f;
    const float qa = (quickNow (c, me, vault::qAttack) - 0.5f) * 2.0f, qr = (quickNow (c, me, vault::qRelease) - 0.5f) * 2.0f;
    const float atkMul = std::exp2 (qa * 4.0f), atkAdd = qa > 0 ? qa * qa * 2.0f : 0.0f;
    const float relMul = std::exp2 (qr * 4.0f), relAdd = qr > 0 ? qr * qr * 4.0f : 0.0f;
    const float cutOct = qCut + me.glob[1] + la[T::lCutoff], resoAdd = qRes + me.glob[2] + la[T::lReso];

    if (part.kind == VaultPartKind::synth)
    {
        float vals[kk::vk::count];
        synthValues (part, vals);
        if (stored != nullptr) if (const int k = synthKey (key); k >= 0) vals[k] = kk::vk::sanitize (k, *stored);
        kk::VoiceBuildContext vctx;
        kk::VoiceParams v;
        kk::FxParams dummy;
        kk::buildVoiceParamsFrom ([&vals] (int k) { return vals[k]; }, vctx, v, dummy);
        switch (key)
        {
            case LKey::cutoff:
            {
                float co = juce::jlimit (40.0f, 20000.0f, v.cutoff * std::exp2 (toneD * 1.5f));
                if (nz (cutOct)) co = juce::jlimit (40.0f, 20000.0f, co * std::exp2 (cutOct));
                return co;
            }
            case LKey::reso:    return nz (resoAdd) ? clamp01 (v.reso + resoAdd) : v.reso;
            case LKey::fenv:    return nz (qEnv) ? juce::jlimit (-1.0f, 1.0f, v.fenv + qEnv) : v.fenv;
            case LKey::glide:   return nz (qGlide) ? clamp01 (v.glide + qGlide) : v.glide;
            case LKey::attack:
                return nz (qa) || nz (la[T::lAttack]) ? juce::jlimit (0.001f, 10.0f, v.attack * atkMul * std::exp2 (la[T::lAttack]) + atkAdd) : v.attack;
            case LKey::release:
                return nz (qr) || nz (la[T::lRelease]) ? juce::jlimit (0.005f, 12.0f, v.release * relMul * std::exp2 (la[T::lRelease]) + relAdd) : v.release;
            case LKey::level: case LKey::pan: default: return 0.0f;
        }
    }
    if (part.kind == VaultPartKind::sample)
    {
        const auto id = layerStoredKey (part.kind, key);
        const int i = vault::sampleParamIndex (id);
        if (i < 0) return 0.0f;
        const auto& in = vault::sampleParams()[(size_t) i];
        float s = juce::jlimit (in.lo, in.hi, stored != nullptr ? *stored : part.get (id, in.def));
        const bool any = nz (resoAdd) || nz (qa) || nz (qr) || nz (la[T::lAttack]) || nz (la[T::lRelease]);
        switch (key)
        {
            case LKey::cutoff:
            {
                float mul = std::exp2 (toneD * 1.5f);
                if (nz (cutOct)) mul *= std::exp2 (cutOct);
                return juce::jlimit (20.0f, 20000.0f, s * mul);
            }
            case LKey::reso:   return any ? clamp01 (s + resoAdd) : s;
            case LKey::attack: return any ? juce::jlimit (0.0f, 10.0f, s * atkMul * std::exp2 (la[T::lAttack]) + atkAdd) : s;
            case LKey::release:
                if (any && (nz (qr) || nz (la[T::lRelease]))) return juce::jlimit (0.0f, 20.0f, (s > 0 ? s : 0.3f) * relMul * std::exp2 (la[T::lRelease]) + relAdd);
                return s;
            case LKey::level: case LKey::pan: case LKey::fenv: case LKey::glide: default: return s;
        }
    }
    // modelled layer: the tone low-pass after the model (cutoff / resonance only)
    if (key == LKey::cutoff)
    {
        float cut = juce::jlimit (40.0f, 20000.0f, 20000.0f * std::exp2 (toneD * 1.5f));
        if (cutOct < 0.0f || (cutOct > 0.0f && cut < 20000.0f)) cut = juce::jlimit (40.0f, 20000.0f, cut * std::exp2 (cutOct));
        return cut;
    }
    if (key == LKey::reso) return clamp01 (resoAdd);
    return 0.0f;
}

void setLayerValue (VaultPreset& p, const Ctx& c, int layer, LKey key, float want)
{
    if (! juce::isPositiveAndBelow (layer, (int) p.parts.size())) return;
    auto& part = p.parts[(size_t) layer];
    const auto [lo, hi] = layerRange (part.kind, key);
    want = juce::jlimit (lo, hi, want);
    if (key == LKey::level || key == LKey::pan)
    {
        if (const auto ref = layerMacro (p, c, layer, key); ref.valid()) { moveMacroTarget (p, c, ref, want, lo, hi); return; }
        if (key == LKey::level) part.level = want; else part.pan = want;
        return;
    }
    const auto id = layerStoredKey (part.kind, key);
    if (id.isEmpty()) return;
    // solve the stored value (log scale for the cutoff, cubic for times) so the effective value is the wanted one
    const bool lg = key == LKey::cutoff;
    const bool time = key == LKey::attack || key == LKey::release;
    auto fromN = [&] (float n) { return lg ? lo * std::pow (hi / lo, n) : time ? lo + (hi - lo) * n * n * n : lo + (hi - lo) * n; };
    auto toN = [&] (float v)
    {
        v = juce::jlimit (lo, hi, v);
        return lg ? std::log (v / lo) / std::log (hi / lo) : time ? std::cbrt ((v - lo) / (hi - lo)) : (v - lo) / (hi - lo);
    };
    const auto snap = p;
    const float n = solve01 ([&] (float x) { const float s = fromN (x); return toN (layerValue (snap, c, layer, key, &s)); }, toN (want));
    part.set (id, fromN (n));
}

//==============================================================================
float lfoValue (const VaultPreset& p, const Ctx& c, int lfo, bool rate)
{
    if (! juce::isPositiveAndBelow (lfo, (int) vault::numLfos)) return 0.0f;
    const auto me = evalMacros (p, c);
    const auto& L = p.mod.lfo[lfo];
    const int i = rate ? 1 : 0;
    if (me.lfoRef[lfo][i].valid()) return me.lfo[lfo][i];
    return rate ? L.rate : L.depth;
}

void setLfoValue (VaultPreset& p, const Ctx& c, int lfo, bool rate, float v)
{
    if (! juce::isPositiveAndBelow (lfo, (int) vault::numLfos)) return;
    const auto me = evalMacros (p, c);
    const int i = rate ? 1 : 0;
    const float lo = rate ? 0.005f : 0.0f, hi = rate ? 80.0f : 1.0f;
    if (me.lfoRef[lfo][i].valid()) { moveMacroTarget (p, c, me.lfoRef[lfo][i], v, lo, hi); return; }
    auto& L = p.mod.lfo[lfo];
    if (rate) L.rate = juce::jlimit (lo, hi, v); else L.depth = juce::jlimit (lo, hi, v);
}

//==============================================================================
float QuickView::toNorm (float v) const
{
    if (hi <= lo) return 0.0f;
    v = juce::jlimit (lo, hi, v);
    if (logScale && lo > 0) return std::log (v / lo) / std::log (hi / lo);
    const float n = (v - lo) / (hi - lo);
    return std::abs (skew - 1.0f) > 1.0e-6f ? std::pow (n, 1.0f / skew) : n;
}

float QuickView::fromNorm (float n) const
{
    n = clamp01 (n);
    if (logScale && lo > 0) return lo * std::pow (hi / lo, n);
    return lo + (hi - lo) * (std::abs (skew - 1.0f) > 1.0e-6f ? std::pow (n, skew) : n);
}

QuickView quickView (const VaultPreset& p, int q)
{
    QuickView v;
    if (q == vault::qVibrato || q == vault::qTremolo) return v;                  // plain 0..1 (0 = off)
    if (q == vault::qPan) { v.arcOrigin = 0.5f; return v; }
    if (q >= vault::qReverb && q <= vault::qEqTilt)
    {
        using F = vault::FxType;
        static const F types[] { F::reverb, F::delay, F::chorus, F::drive, F::width, F::eq };
        v.kind = QuickView::fxKnob;
        v.type = types[q - vault::qReverb];
        v.slot = firstSlotOfType (p, v.type);
        const auto& in = vault::fxTypeInfo (v.type).params[(size_t) vault::fxMainParam (v.type)];
        v.lo = in.lo; v.hi = in.hi;
        v.arcOrigin = in.lo < 0.0f ? 0.5f : 0.0f;
        return v;
    }
    // FILTER · AMP: the effective value of the first layer the knob acts on (it moves every layer the same way)
    v.kind = QuickView::layerKnob;
    v.key = q == vault::qCutoff ? LKey::cutoff : q == vault::qReso ? LKey::reso : q == vault::qAttack ? LKey::attack
          : q == vault::qRelease ? LKey::release : q == vault::qEnvAmt ? LKey::fenv : LKey::glide;
    for (int pass = 0; pass < 2 && v.layer < 0; ++pass)
        for (int i = 0; i < (int) p.parts.size() && v.layer < 0; ++i)
        {
            const auto k = p.parts[(size_t) i].kind;
            if (! layerHas (k, v.key)) continue;
            if (pass == 0 && k == VaultPartKind::model) continue;     // a played (synth / sampled) layer first
            v.layer = i;
        }
    if (v.layer < 0)
    {
        v.kind = QuickView::offKnob;
        v.why = v.key == LKey::fenv || v.key == LKey::glide ? "only for synth layers - this sound has none" : "not used by the layers of this sound";
        v.arcOrigin = 0.5f;
        return v;
    }
    const auto kind = p.parts[(size_t) v.layer].kind;
    switch (v.key)
    {
        case LKey::cutoff:  v.lo = 20.0f; v.hi = 20000.0f; v.logScale = true; break;
        case LKey::reso:    v.lo = 0.0f; v.hi = 1.0f; break;
        case LKey::attack:  v.lo = 0.0f; v.hi = 10.0f; v.skew = 3.0f; break;
        case LKey::release: v.lo = 0.0f; v.hi = kind == VaultPartKind::sample ? 20.0f : 12.0f; v.skew = 3.0f; break;
        case LKey::fenv:    v.lo = -1.0f; v.hi = 1.0f; v.arcOrigin = 0.5f; break;
        case LKey::glide: case LKey::level: case LKey::pan: default: v.lo = 0.0f; v.hi = 1.0f; break;
    }
    return v;
}

float quickEffective (const VaultPreset& p, const Ctx& c, int q, const QuickView& v)
{
    switch (v.kind)
    {
        case QuickView::fxKnob:    return fxValue (p, c, v.slot, v.type, vault::fxMainParam (v.type));
        case QuickView::layerKnob: return layerValue (p, c, v.layer, v.key);
        case QuickView::offKnob:   return c.quick[q];
        case QuickView::plainKnob: default: return clamp01 (c.quick[q] + evalMacros (p, c).quickAdd[q]);
    }
}

float quickPosition (const VaultPreset& p, const Ctx& c, int q)
{
    const auto v = quickView (p, q);
    if (v.kind == QuickView::offKnob) return c.quick[q];
    return v.toNorm (quickEffective (p, c, q, v));
}

float quickParamFor (const VaultPreset& p, const Ctx& c, int q, float pos)
{
    const auto v = quickView (p, q);
    pos = clamp01 (pos);
    if (v.kind == QuickView::offKnob) return pos;
    const float neutral = vault::quickInfo()[(size_t) q].neutral;
    // the sound as designed stays exactly neutral (the knob's tick)
    if (std::abs (pos - quickPosition (p, c.withQuick (q, neutral), q)) < 0.0025f) return neutral;
    if (v.kind == QuickView::plainKnob) return clamp01 (pos - evalMacros (p, c).quickAdd[q]);
    return solve01 ([&] (float x) { return v.toNorm (quickEffective (p, c.withQuick (q, x), q, v)); }, pos);
}

juce::String quickText (const VaultPreset& p, const Ctx& c, int q, float pos, bool fxOn, bool slotOn)
{
    const auto v = quickView (p, q);
    if (v.kind == QuickView::offKnob) return juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94"));
    const float x = v.fromNorm (pos);
    auto pctS = [] (float a) { return juce::String (juce::roundToInt (a * 100.0f)) + "%"; };
    auto secs = [] (float s)
    {
        if (s < 0.0995f) return juce::String (juce::roundToInt (s * 1000.0f)) + " ms";
        if (s < 9.95f) return juce::String (s, 1) + " s";
        return juce::String (juce::roundToInt (s)) + " s";
    };
    switch (q)
    {
        case vault::qVibrato: return x < 0.004f ? juce::String ("off") : juce::String (juce::roundToInt (x * 60.0f)) + " ct";
        case vault::qTremolo: return x < 0.004f ? juce::String ("off") : pctS (x);
        case vault::qPan:
        {
            const int pp = juce::roundToInt ((x - 0.5f) * 200.0f);
            return pp == 0 ? juce::String ("C") : (pp < 0 ? "L " : "R ") + juce::String (std::abs (pp));
        }
        default: break;
    }
    if (v.kind == QuickView::layerKnob)
    {
        const auto kind = p.parts[(size_t) v.layer].kind;
        switch (v.key)
        {
            case LKey::cutoff:
                if (x >= 19950.0f) return "open";
                if (x >= 9950.0f) return juce::String (juce::roundToInt (x / 1000.0f)) + " kHz";
                if (x >= 995.0f) return juce::String (x / 1000.0f, 1) + " kHz";
                return juce::String (juce::roundToInt (x)) + " Hz";
            case LKey::reso:    return pctS (x);
            case LKey::attack:  return kind == VaultPartKind::sample && x < 0.0005f ? juce::String ("natural") : secs (x);
            case LKey::release: return kind == VaultPartKind::sample && x < 0.0005f ? juce::String ("natural") : secs (x);
            case LKey::fenv:    { const int pp = juce::roundToInt (x * 100.0f); return (pp > 0 ? "+" : "") + juce::String (pp) + "%"; }
            case LKey::glide:   return x < 0.004f ? juce::String ("off") : pctS (x);
            case LKey::level: case LKey::pan: default: return pctS (x);
        }
    }
    // EFFECTS: the effective amount of the chain's slot + its type ("14% Plate", "22% 1/8D")
    if (! fxOn || ! slotOn) return "off";
    const auto& ti = vault::fxTypeInfo (v.type);
    auto choice = [&] (int param)
    {
        const auto& in = ti.params[(size_t) param];
        if (in.choices == nullptr) return juce::String();
        const auto names = juce::StringArray::fromTokens (in.choices, "|", "");
        const int i = juce::roundToInt (fxValue (p, c, v.slot, v.type, param) - in.lo);
        return juce::isPositiveAndBelow (i, names.size()) ? names[i] : juce::String();
    };
    switch (q)
    {
        case vault::qReverb: return pctS (x) + " " + choice (ti.indexOf ("type"));
        case vault::qDelay:
        {
            const bool sync = fxValue (p, c, v.slot, v.type, ti.indexOf ("sync")) > 0.5f;
            const auto tm = sync ? choice (ti.indexOf ("time")) : juce::String (juce::roundToInt (fxValue (p, c, v.slot, v.type, ti.indexOf ("ms")))) + "ms";
            return pctS (x) + " " + tm;
        }
        case vault::qDrive: return pctS (x) + " " + choice (ti.indexOf ("type"));
        case vault::qEqTilt: return (x > 0.05f ? "+" : "") + juce::String (x, 1) + " dB";
        default: return pctS (x);
    }
}
} // namespace vui::val
