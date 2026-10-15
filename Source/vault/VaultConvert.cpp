#include "VaultTypes.h"
#include "../VoiceBuild.h"

// v1 / v2 -> v3 (vault::upgradeToV3). A v2 preset's master FX were one kk::FxRack driven by the EVOLVE FX keys of part 0
// (FxParams built by kk::buildVoiceParamsFrom, including the EVOLVE macros / time macro / morph on the FX): this builds
// exactly those FxParams and turns every stage that does something into a chain slot, in the rack's order. The engine
// adds a silent default slot for a type a MAIN page knob or a classic macro needs (VaultEngine), so the converted preset
// plays exactly like the v2 one, also with its TONE / SPACE / DRIVE / MOTION macros moved (sound macros 1-4 target
// "classic.*"). The pure FX keys are then removed from the parts (they no longer do anything).
namespace vault
{
namespace
{
// EVOLVE keys that only feed the FX (FxParams), never a voice
bool pureFxKey (int k)
{
    using namespace kk::vk;
    switch (k)
    {
        case drive: case driveType: case crush: case wow: case chorus: case phaser: case flanger: case delayMix: case delayTime:
        case delayFb: case delayMode: case revMix: case revSize: case revType: case eqLow: case eqHigh: case reverse: case freeze:
        case width: case master: case punch: case halftime: case ghostBlur: case ghostRev: case circuit: case circRate:
        case body: case bodyMix:
            return true;
        default: return false;
    }
}
int nearestDivision (double beats)
{
    const auto& d = divisions();
    int best = 0;
    for (int i = 1; i < (int) d.size(); ++i)
        if (std::abs (d[(size_t) i].beats - beats) < std::abs (d[(size_t) best].beats - beats)) best = i;
    return best;
}
} // namespace

void upgradeToV3 (VaultPreset& p, int* droppedSlots)
{
    if (droppedSlots != nullptr) *droppedSlots = 0;
    if (p.v3) return;
    p.v3 = true;
    p.fx.clear();
    p.finish = 0;
    if (! p.parts.empty())
    {
        // part 0's values exactly as the v2 engine loaded them (VaultEngine::loadProgram)
        float vals[kk::vk::count];
        kk::vk::fillDefaults (vals);
        const auto& p0 = p.parts.front();
        if (p0.kind != VaultPartKind::synth)
        {
            vals[kk::vk::revMix] = 0.0f; vals[kk::vk::master] = 0.0f; vals[kk::vk::m2] = 0.0f; vals[kk::vk::m5] = 0.0f;
        }
        for (auto& [id, v] : p0.values)
            if (const int key = kk::vk::keyOf (id); key >= 0) vals[key] = kk::vk::sanitize (key, v);
        kk::VoiceBuildContext ctx;
        kk::VoiceParams vp;
        kk::FxParams f;
        kk::buildVoiceParamsFrom ([&vals] (int key) { return vals[key]; }, ctx, vp, f);
        bool anyBass = false;
        for (auto& part : p.parts)
            for (auto& [id, v] : part.values)
                if (id == ID::bassMode && v > 0.5f) anyBass = true;
        f.cleanLow = f.cleanLow || anyBass;

        auto add = [&] (FxType t) -> VaultFxSlot& { p.fx.push_back (VaultFxSlot::make (t)); return p.fx.back(); };
        // the FxRack order: punch, drive, body, lo-fi, circuit (stutter), half-time, chorus, phaser, flanger, EQ, delay, reverb, reverse, ghost, width
        if (f.punch > 0.001f) add (FxType::transient).set ("attack", f.punch);
        if (f.drive > 0.001f)
            add (FxType::drive).set ("amount", f.drive).set ("type", (float) juce::jlimit (0, 4, f.driveType)).set ("cleanLow", f.cleanLow ? 1.0f : 0.0f);
        if (f.body > 0) add (FxType::body).set ("model", (float) juce::jlimit (1, 6, f.body)).set ("mix", f.bodyMix);
        if (f.crush > 0.001f || f.wow > 0.001f) add (FxType::lofi).set ("crush", f.crush).set ("wow", f.wow).set ("mix", 1.0f);
        if (f.circuit > 0.001f)
            add (FxType::stutter).set ("amount", f.circuit).set ("rate", (float) nearestDivision (f.circBeats)).set ("seed", (float) juce::jlimit (0u, 9999u, f.seed));
        if (f.halftime > 0.001f) add (FxType::halftime).set ("mix", f.halftime);
        if (f.chorus > 0.001f) add (FxType::chorus).set ("mix", f.chorus).set ("rate", 0.33f).set ("depth", 0.5f).set ("mode", 0.0f);
        if (f.phaser > 0.001f) add (FxType::phaser).set ("mix", f.phaser);
        if (f.flanger > 0.001f) add (FxType::flanger).set ("mix", f.flanger);
        if (std::abs (f.eqLow) >= 0.05f || std::abs (f.eqHigh) >= 0.05f)
            add (FxType::eq).set ("low", f.eqLow).set ("high", f.eqHigh).set ("tilt", 0.0f).set ("lowFreq", 160.0f).set ("highFreq", 6000.0f);
        const int dDiv = nearestDivision (f.delayBeats);
        const auto defDelay = VaultFxSlot::make (FxType::delay);
        if (f.delayMix > 0.0005f || dDiv != (int) defDelay.get ("time") || ! juce::exactlyEqual (f.delayFb, defDelay.get ("feedback")) || f.delayMode != 0)
            add (FxType::delay).set ("mix", f.delayMix).set ("time", (float) dDiv).set ("feedback", f.delayFb)
                               .set ("mode", (float) juce::jlimit (0, 2, f.delayMode)).set ("sync", 1.0f);
        const auto defRev = VaultFxSlot::make (FxType::reverb);
        if (f.revMix > 0.0005f || f.freeze || ! juce::exactlyEqual (f.revSize, defRev.get ("size")) || f.revType != 0)
            add (FxType::reverb).set ("mix", f.revMix).set ("size", f.revSize).set ("type", (float) juce::jlimit (0, 2, f.revType))
                                .set ("freeze", f.freeze ? 1.0f : 0.0f);
        if (f.reverse > 0.001f) add (FxType::reverse).set ("mix", f.reverse).set ("length", (float) divisionIndex ("1/8"));
        if (f.ghost > 0.001f) add (FxType::ghost).set ("mix", f.ghost).set ("blur", f.ghostBlur).set ("reverse", f.ghostRev ? 1.0f : 0.0f);
        if (! juce::exactlyEqual (f.width, 0.5f)) add (FxType::width).set ("width", f.width).set ("monoBelow", 0.0f);
        if ((int) p.fx.size() > maxFxSlots)   // never in the v2 factory bank: keep the first 8 in rack order
        {
            if (droppedSlots != nullptr) *droppedSlots = (int) p.fx.size() - maxFxSlots;
            p.fx.resize ((size_t) maxFxSlots);
        }
        p.finish = kk::clamp01 (f.master);

        for (auto& part : p.parts)
            part.values.erase (std::remove_if (part.values.begin(), part.values.end(),
                                               [] (const std::pair<juce::String, float>& kv) { const int k = kk::vk::keyOf (kv.first); return k >= 0 && pureFxKey (k); }),
                               part.values.end());
    }
    // the classic macros
    static const char* classic[4] { "classic.tone", "classic.space", "classic.drive", "classic.motion" };
    for (int m = 0; m < 4; ++m)
    {
        p.macros[(size_t) m].name = classicMacroNames()[m];
        p.macros[(size_t) m].targets = { VaultMacroTarget { classic[m], 0.0f, 1.0f, 0.0f } };
    }
    p.mod = VaultMod();
    p.arp = VaultArp();
    p.gate = VaultGate();
}
} // namespace vault
