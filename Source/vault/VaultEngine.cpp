#include "VaultEngine.h"

namespace
{
inline float lowHalf (float m) { return std::min (1.0f, 2.0f * m); }          // 0 .. 0.5 -> 0 .. 1 (scales the designed amount)
inline float upHalf (float m)  { return std::max (0.0f, 2.0f * m - 1.0f); }   // 0.5 .. 1 -> 0 .. 1 (adds on top)
constexpr float limThresh = 0.89f;   // about -1 dBFS
inline bool nz (float x) { return std::abs (x) > 0.0f; }                   // exactly non-zero (no -Wfloat-equal)
inline bool same (float a, float b) { return juce::exactlyEqual (a, b); }
inline float macroCurve (float m, float c)
{
    m = juce::jlimit (0.0f, 1.0f, m);
    if (c > 0.001f) return std::pow (m, 1.0f + 3.0f * c);
    if (c < -0.001f) return 1.0f - std::pow (1.0f - m, 1.0f - 3.0f * c);
    return m;
}
// a MAIN page knob around a designed amount: below the neutral point it scales the amount down to 0, above it moves it up to `top`
inline float quickAmount (float designed, float q, float top)
{
    if (same (q, 0.5f)) return designed;
    if (q < 0.5f) return designed * (2.0f * q);
    return designed + (std::max (designed, top) - designed) * (2.0f * q - 1.0f);
}
// the default slots the MAIN page knobs and the classic macros need when the preset has none (canonical order)
constexpr vault::FxType auxTypes[] { vault::FxType::drive, vault::FxType::chorus, vault::FxType::eq, vault::FxType::delay,
                                     vault::FxType::reverb, vault::FxType::width };
} // namespace

// the voices of one model for one part (created off the audio thread by serviceResources, then reused)
struct VaultEngine::ModelPool
{
    const vault::models::ModelInfo* info = nullptr;
    std::vector<std::unique_ptr<vault::models::ModelVoice>> voices;
    std::vector<int> note;
    std::vector<uint32_t> age;
    std::vector<uint8_t> held, sustained;
    uint32_t ageCounter = 0;

    void prepare (const vault::models::ModelInfo& mi, double sr, int maxBlock)
    {
        info = &mi;
        const auto n = (size_t) mi.maxVoices;
        voices.clear();
        for (size_t i = 0; i < n; ++i) { voices.push_back (mi.create()); voices.back()->prepare (sr, maxBlock); }
        note.assign (n, -1); age.assign (n, 0); held.assign (n, 0); sustained.assign (n, 0);
    }
    void noteOn (int n, float vel, float freq, const float* params, int numParams)
    {
        size_t pick = 0; bool found = false;
        for (size_t i = 0; i < voices.size() && ! found; ++i) if (! voices[i]->isActive()) { pick = i; found = true; }
        if (! found) for (size_t i = 1; i < voices.size(); ++i) if (age[i] < age[pick]) pick = i;   // steal the oldest
        auto& v = *voices[pick];
        if (v.isActive()) v.kill();
        v.setParams (params, numParams);
        v.noteOn (n, vel, freq);
        note[pick] = n; age[pick] = ++ageCounter; held[pick] = 1; sustained[pick] = 0;
    }
    void noteOff (int n, bool pedal)
    {
        for (size_t i = 0; i < voices.size(); ++i)
            if (held[i] && note[i] == n)
            {
                held[i] = 0;
                if (pedal) sustained[i] = 1; else voices[i]->noteOff (0.5f);
            }
    }
    void pedalUp() { for (size_t i = 0; i < voices.size(); ++i) if (sustained[i]) { sustained[i] = 0; voices[i]->noteOff (0.5f); } }
    void allOff (bool hard)
    {
        for (size_t i = 0; i < voices.size(); ++i)
        {
            if (hard) voices[i]->kill(); else if (held[i] || sustained[i]) voices[i]->noteOff (0.5f);
            held[i] = sustained[i] = 0;
        }
    }
};

struct VaultEngine::Part
{
    kk::SynthEngine synth;
    vault::SamplerPart sampler;
    float ph1 = 0, ph2 = 0, last1 = 0, last2 = 0, sh1 = 0, sh2 = 0;
    kk::Rng rng;
    // model voices: one pool per registered model, created on demand off the audio thread (serviceResources)
    std::unique_ptr<std::atomic<ModelPool*>[]> pools;
    std::vector<std::unique_ptr<ModelPool>> ownedPools;
    int numPools = 0;
    std::atomic<int> modelWanted { -1 };
    float vibPhase = 0;
    kk::SvfCoef toneC; kk::SvfState toneS[2]; bool toneOn = false;   // TONE low-pass for model parts
    float spTmp[vault::numSampleParams] {};
};

VaultEngine::VaultEngine()
{
    for (auto& m : macroTarget) m.store (0.5f);
    for (int q = 0; q < vault::numQuick; ++q) { quickTarget[q].store (vault::quickInfo()[(size_t) q].neutral); quickSm[q] = vault::quickInfo()[(size_t) q].neutral; }
    for (auto& a : lfoPhaseUi) a.store (0.0f);
    for (auto& a : lfoValueUi) a.store (0.0f);
    for (auto& sl : slots)
    {
        sl.prog = std::make_unique<Program>();
        for (int k = 0; k < vault::maxParts; ++k) kk::vk::fillDefaults (sl.prog->values[k]);
        sl.prog->numParts = 0;
        resolveProgram (*sl.prog);
    }
    pending = std::make_unique<Program>();
    editProg = std::make_unique<Program>();
}

VaultEngine::~VaultEngine()
{
    for (auto& sl : slots) slotRelease (sl);
}

//==============================================================================
namespace
{
bool compileTarget (const juce::String& id, const VaultPreset& q, VaultCompiledTarget& out)
{
    out.ref = vault::parseTarget (id);
    if (! out.ref.valid()) return false;
    if (out.ref.kind == vault::TargetRef::fxSlot)
    {
        if (out.ref.a >= q.fx.size()) return false;
        out.fxType = q.fx[out.ref.a].type;
        const int pi = vault::fxTypeInfo (out.fxType).indexOf (id.fromFirstOccurrenceOf (".", false, false).toRawUTF8());
        if (pi < 0) return false;
        out.ref.b = (uint8_t) pi;
    }
    else if (out.ref.kind == vault::TargetRef::fxType) out.fxType = (vault::FxType) out.ref.a;
    else if (out.ref.kind == vault::TargetRef::layer && out.ref.a >= q.parts.size()) return false;
    return true;
}
template <size_t N>
int compileDests (const std::vector<VaultModDest>& src, const VaultPreset& q, std::array<VaultCompiledTarget, N>& dst)
{
    int n = 0;
    for (auto& d : src)
        if (n < (int) N && std::isfinite (d.amount) && compileTarget (d.target, q, dst[(size_t) n]))
            dst[(size_t) n++].amount = d.amount;
    return n;
}
} // namespace

VaultCompiledPreset VaultEngine::compile (const VaultPreset& presetIn, std::shared_ptr<vault::SampleLibrary> library)
{
    VaultPreset upgraded;
    const VaultPreset* pp = &presetIn;
    if (! presetIn.v3) { upgraded = presetIn; vault::upgradeToV3 (upgraded); pp = &upgraded; }
    const VaultPreset& preset = *pp;

    VaultCompiledPreset c;
    c.numParts = juce::jlimit (0, (int) vault::maxParts, (int) preset.parts.size());
    if (library == nullptr && preset.usesSamples()) library = vault::SampleLibrary::getShared();
    c.library = library;
    const auto& reg = vault::models::ModelRegistry::get();
    for (int k = 0; k < c.numParts; ++k)
    {
        const auto& src = preset.parts[(size_t) k];
        auto& dst = c.parts[(size_t) k];
        dst.kind = src.kind;
        dst.kv.clear();
        dst.kv.reserve (src.values.size());
        for (auto& [id, v] : src.values)
            if (const int key = kk::vk::keyOf (id); key >= 0)
                dst.kv.push_back ({ key, kk::vk::sanitize (key, v) });
        dst.level = std::isfinite (src.level) ? juce::jlimit (0.0f, 2.0f, src.level) : 1.0f;
        dst.pan = std::isfinite (src.pan) ? juce::jlimit (-1.0f, 1.0f, src.pan) : 0.0f;
        dst.transpose = juce::jlimit (-3, 3, src.octave) * 12 + juce::jlimit (-12, 12, src.semi);
        // sample parameters (defaults, then the part's own "s.*" values, clamped)
        for (int i = 0; i < vault::numSampleParams; ++i)
        {
            const auto& in = vault::sampleParams()[(size_t) i];
            const float v = src.get (in.id, in.def);
            dst.sp[(size_t) i] = std::isfinite (v) ? juce::jlimit (in.lo, in.hi, v) : in.def;
        }
        if (src.kind == VaultPartKind::sample && library != nullptr)
            if ((dst.instrument = library->find (src.source)) != nullptr)
                dst.articulation = dst.instrument->desc.articulationIndex (src.articulation);
        if (src.kind == VaultPartKind::model)
            if ((dst.model = reg.indexOf (src.source)) >= 0)
            {
                const auto& mi = reg.at (dst.model);
                dst.numModelParams = (int) mi.params.size();
                for (int i = 0; i < dst.numModelParams; ++i)
                {
                    const auto& in = mi.params[(size_t) i];
                    const float v = src.get (in.id, in.def);
                    dst.mp[(size_t) i] = std::isfinite (v) ? juce::jlimit (in.lo, in.hi, v) : in.def;
                }
            }
    }
    c.gain = std::isfinite (preset.gain) ? juce::jlimit (0.0f, 16.0f, preset.gain) : 1.0f;
    if (! preset.parts.empty())
        if (const float pk = preset.parts.front().get (vault::peakKey, 0.0f); std::isfinite (pk) && pk < -0.01f)
        {
            c.peakThresh = juce::Decibels::decibelsToGain (juce::jlimit (-60.0f, 0.0f, pk));
            if (const float ve = preset.parts.front().get (vault::peakVelKey, 0.0f); std::isfinite (ve)) c.peakVelExp = juce::jlimit (0.0f, 4.0f, ve);
        }
    for (int m = 0; m < vault::numMacros; ++m)
        c.macroDefaults[m] = std::isfinite (preset.macroDefaults[m]) ? juce::jlimit (0.0f, 1.0f, preset.macroDefaults[m]) : 0.5f;

    // ---- v3 sections
    auto& ctl = c.ctl;
    ctl.numFx = 0;
    int perType[vault::numFxTypes] {};
    for (auto& f : preset.fx)
    {
        if (ctl.numFx >= vault::maxFxSlots || (int) f.type >= vault::numFxTypes || perType[(int) f.type] >= vault::maxSameFxType) continue;
        ++perType[(int) f.type];
        auto& d = ctl.fx[(size_t) ctl.numFx++];
        d = f;
        const auto& ti = vault::fxTypeInfo (f.type);
        for (size_t i = 0; i < ti.params.size(); ++i)
            d.p[i] = std::isfinite (d.p[i]) ? juce::jlimit (ti.params[i].lo, ti.params[i].hi, d.p[i]) : ti.params[i].def;
    }
    ctl.finish = std::isfinite (preset.finish) ? juce::jlimit (0.0f, 1.0f, preset.finish) : 0.0f;
    for (int i = 0; i < vault::numLfos; ++i)
    {
        const auto& s = preset.mod.lfo[i];
        auto& d = ctl.lfo[(size_t) i];
        d.shape = (uint8_t) juce::jlimit (0, (int) VaultLfo::numShapes - 1, (int) s.shape);
        d.rate = std::isfinite (s.rate) ? juce::jlimit (0.01f, 50.0f, s.rate) : 1.0f;
        d.sync = s.sync; d.div = (uint8_t) juce::jlimit (0, (int) vault::divisions().size() - 1, (int) s.div);
        d.delay = juce::jlimit (0.0f, 10.0f, std::isfinite (s.delay) ? s.delay : 0.0f);
        d.fade = juce::jlimit (0.0f, 10.0f, std::isfinite (s.fade) ? s.fade : 0.0f);
        d.phase = juce::jlimit (0.0f, 1.0f, std::isfinite (s.phase) ? s.phase : 0.0f);
        d.retrig = s.retrig;
        d.depth = juce::jlimit (0.0f, 1.0f, std::isfinite (s.depth) ? s.depth : 1.0f);
        d.numDests = compileDests (s.dests, preset, d.dests);
    }
    ctl.numMw = compileDests (preset.mod.modWheel, preset, ctl.mw);
    ctl.numVel = compileDests (preset.mod.velocity, preset, ctl.vel);
    ctl.numAt = compileDests (preset.mod.aftertouch, preset, ctl.at);
    for (int m = 0; m < vault::numSoundMacros; ++m)
    {
        int n = 0;
        for (auto& t : preset.macros[(size_t) m].targets)
            if (n < vault::maxMacroTargets && std::isfinite (t.min) && std::isfinite (t.max) && compileTarget (t.target, preset, ctl.macro[(size_t) m][(size_t) n]))
            {
                auto& d = ctl.macro[(size_t) m][(size_t) n++];
                d.min = t.min; d.max = t.max; d.curve = std::isfinite (t.curve) ? juce::jlimit (-1.0f, 1.0f, t.curve) : 0.0f;
            }
        ctl.numMacroTargets[(size_t) m] = n;
    }
    ctl.arp = preset.arp;
    ctl.arp.rate = (uint8_t) juce::jlimit (0, (int) vault::divisions().size() - 1, (int) ctl.arp.rate);
    ctl.arp.mode = (uint8_t) juce::jlimit (0, (int) VaultArp::numModes - 1, (int) ctl.arp.mode);
    ctl.arp.octaves = (uint8_t) juce::jlimit (1, 4, (int) ctl.arp.octaves);
    ctl.arp.steps = (uint8_t) juce::jlimit (1, (int) vault::numSteps, (int) ctl.arp.steps);
    ctl.gate = preset.gate;
    ctl.gate.rate = (uint8_t) juce::jlimit (0, (int) vault::divisions().size() - 1, (int) ctl.gate.rate);
    ctl.gate.steps = (uint8_t) juce::jlimit (1, (int) vault::numSteps, (int) ctl.gate.steps);
    return c;
}

void VaultEngine::prepare (double sampleRate, int maxBlockSize, int numPartsToPrepare, bool crossfadeSlot)
{
    const std::lock_guard<std::mutex> g (resourceLock);
    const int numModels = vault::models::ModelRegistry::get().size();
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    maxBlock = std::max (1, maxBlockSize);
    preparedParts = juce::jlimit (1, (int) vault::maxParts, numPartsToPrepare);
    haveXfadeSlot = crossfadeSlot;
    kk::WavetableBank::get();
    for (int si = 0; si < 2; ++si)
    {
        auto& sl = slots[si];
        sl.prepared = si == 0 || crossfadeSlot;
        for (int k = 0; k < vault::maxParts; ++k)
        {
            auto& part = sl.parts[(size_t) k];
            if (sl.prepared && k < preparedParts)
            {
                if (! part) part = std::make_unique<Part>();
                part->synth.prepare ((float) sr);
                part->sampler.prepare (sr);
                part->rng.seed (101u + (uint32_t) k * 7u + (uint32_t) si * 1009u);
                // model pools are rebuilt on demand for this sample rate
                part->ownedPools.clear();
                part->numPools = numModels;
                part->pools = std::make_unique<std::atomic<ModelPool*>[]> ((size_t) std::max (1, numModels));
                for (int m = 0; m < std::max (1, numModels); ++m) part->pools[(size_t) m].store (nullptr);
                part->modelWanted = -1;
            }
            else part.reset();
        }
        if (sl.prepared)
        {
            if (sl.chain == nullptr) sl.chain = std::make_unique<vault::FxChain>();
            sl.chain->prepare (sr, chunk);
            if (sl.eff == nullptr) sl.eff = std::make_unique<Effective>();
            sl.busL.assign (chunk, 0.0f); sl.busR.assign (chunk, 0.0f); sl.busG.assign (chunk, 0.0f);
            sl.partL.assign ((size_t) (vault::maxParts * chunk), 0.0f); sl.partR.assign ((size_t) (vault::maxParts * chunk), 0.0f);
            sl.lfo1.assign ((size_t) (vault::maxParts * chunk), 0.0f); sl.lfo2.assign ((size_t) (vault::maxParts * chunk), 0.0f);
            sl.gate.prepare (sr);
        }
        else
        {
            sl.chain.reset(); sl.eff.reset();
            for (auto* v : { &sl.busL, &sl.busR, &sl.busG, &sl.partL, &sl.partR, &sl.lfo1, &sl.lfo2 }) { v->clear(); v->shrink_to_fit(); }
        }
        for (auto& v : sl.vp) v = {};
        for (auto& l : sl.lfo) l = {};
        sl.trigBeat = 0; sl.trigTime = -1.0e9; sl.gainsValid = false;
        sl.volMod = sl.volModPrev = 1; sl.panMod = sl.panModPrev = 0; sl.vibPhase = sl.tremPhase = 0; sl.gateMix = 0;
    }
    if (cur != 0) { std::swap (slots[0].prog, slots[1].prog); std::swap (slots[0].held, slots[1].held); cur = 0; }   // the current program lives in the prepared slot 0
    slotRelease (slots[1]);                   // no crossfade running: only slot 0 holds instruments
    slotRelease (slots[0]);
    slotAcquire (slots[0]);
    if (slots[0].chain != nullptr) slots[0].chain->setLayout (slots[0].prog->entries, slots[0].prog->numEntries);

    {   // 8th-order Butterworth high-pass at 160 Hz on the side signal: -20 dB at 120 Hz, -33 dB at 100 Hz, -48 dB at 80 Hz
        static const double qs[4] { 0.50979558, 0.60134489, 0.89997622, 2.56291545 };
        const double w = juce::MathConstants<double>::twoPi * sideHighPassHz / sr, cs = std::cos (w);
        for (int i = 0; i < 4; ++i)
        {
            auto& h = sideHp[i];
            const double al = std::sin (w) / (2.0 * qs[i]), a0 = 1.0 + al;
            h.b0 = (float) ((1 + cs) / 2 / a0); h.b1 = (float) (-(1 + cs) / a0); h.b2 = (float) ((1 + cs) / 2 / a0);
            h.a1 = (float) (-2 * cs / a0); h.a2 = (float) ((1 - al) / a0); h.z1 = h.z2 = 0;
        }
    }
    dcX = dcY = 0; dcA = (float) std::exp (-juce::MathConstants<double>::twoPi * dcBlockHz / sr);
    limEnv = 0; limRel = (float) std::exp (-1.0 / (0.12 * sr));
    xfading = false; xfPos = 0.0f; xfStep = (float) (1.0 / (xfadeSeconds * sr));
    held.fill (0.0f); keysDown.fill (0.0f); pitchWheel = modWheel = aftertouch = 0; sustain = false; freeBeat = 0; lastBpm = 120.0; lastVelocity = 0.8f;
    for (int m = 0; m < vault::numMacros; ++m) macroSm[m] = macroTarget[m].load();
    for (int q = 0; q < vault::numQuick; ++q) quickSm[q] = quickTarget[q].load();
    volumeSm = volumeTarget.load();
    panSmL = panSmR = 1.0f;
    arp.reset(); arpWasOn = false;
    evs.clear(); evs.reserve (4096);
    arpEvs.resize ((size_t) vault::Arp::maxEvents);
    inEvs.clear(); inEvs.reserve (2048);
    sampleClock = 0; noteLogCount = 0;
}

void VaultEngine::releaseResources()
{
    const std::lock_guard<std::mutex> g (resourceLock);
    for (auto& sl : slots)
    {
        slotRelease (sl);
        for (auto& p : sl.parts) p.reset();
        sl.chain.reset();
        sl.prepared = false;
    }
    preparedParts = 0;
    xfading = false;
}

void VaultEngine::loadProgram (Program& dst, const VaultCompiledPreset& p, bool applyLocks)
{
    dst.numParts = juce::jlimit (0, (int) vault::maxParts, p.numParts);
    for (int k = 0; k < vault::maxParts; ++k)
    {
        kk::vk::fillDefaults (dst.values[k]);
        if (k >= dst.numParts)
        {
            dst.level[k] = 0; dst.pan[k] = 0; dst.panL[k] = dst.panR[k] = 0; dst.transpose[k] = 0;
            dst.kind[k] = VaultPartKind::synth; dst.instrument[k] = nullptr; dst.model[k] = -1; dst.numModelParams[k] = 0;
            continue;
        }
        const auto& part = p.parts[(size_t) k];
        dst.kind[k] = part.kind;
        if (part.kind != VaultPartKind::synth)
        {
            // EVOLVE macro defaults for a sample / model part: neutral (m2 / m5 default to a little reverb, delay and
            // chorus, and m5 to LFO movement: off)
            dst.values[k][kk::vk::revMix] = 0.0f;
            dst.values[k][kk::vk::master] = 0.0f;
            dst.values[k][kk::vk::m2] = 0.0f;
            dst.values[k][kk::vk::m5] = 0.0f;
        }
        for (auto& [key, v] : part.kv) dst.values[k][key] = v;
        dst.instrument[k] = part.kind == VaultPartKind::sample ? part.instrument : nullptr;
        dst.articulation[k] = part.articulation;
        std::copy (part.sp.begin(), part.sp.end(), dst.sp[k]);
        dst.model[k] = part.kind == VaultPartKind::model ? part.model : -1;
        dst.numModelParams[k] = part.numModelParams;
        std::copy (part.mp.begin(), part.mp.end(), dst.mp[k]);
        const float a = (part.pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;   // equal power, unity at centre
        dst.pan[k] = part.pan;
        dst.panL[k] = std::cos (a) * juce::MathConstants<float>::sqrt2;
        dst.panR[k] = std::sin (a) * juce::MathConstants<float>::sqrt2;
        dst.level[k] = part.level;
        dst.transpose[k] = part.transpose;
    }
    // part 0's "gain" (dB) is the FX output gain; the other parts keep their own gain relative to it
    for (int k = 1; k < dst.numParts; ++k)
        dst.level[k] *= juce::Decibels::decibelsToGain (dst.values[k][kk::vk::gain] - dst.values[0][kk::vk::gain]);
    dst.gain = p.gain;
    dst.peakThresh = p.peakThresh;
    dst.peakVelExp = p.peakVelExp;
    dst.ctl = p.ctl;
    if (applyLocks && slots[cur].prog->numParts > 0)
    {
        const auto& now = slots[cur].prog->ctl;   // "keep while browsing": the locked sections of the sound that plays now
        if (lockFx.load()) { dst.ctl.numFx = now.numFx; dst.ctl.fx = now.fx; dst.ctl.finish = now.finish; }
        if (lockArp.load()) dst.ctl.arp = now.arp;
        if (lockGate.load()) dst.ctl.gate = now.gate;
    }
    dst.monoLows = dst.numParts > 0 && dst.values[0][kk::vk::bassMode] > 0.5f;
    dst.anyBass = false;
    for (int k = 0; k < dst.numParts; ++k) dst.anyBass |= dst.values[k][kk::vk::bassMode] > 0.5f;
    resolveProgram (dst);
}

void VaultEngine::resolveProgram (Program& dst)
{
    // the effective chain: the preset's slots + a silent default slot for each type a MAIN knob / classic macro needs,
    // inserted at its canonical position (the v2 rack order)
    int n = 0;
    auto& c = dst.ctl;
    for (int i = 0; i < c.numFx; ++i)
    {
        auto& e = dst.entries[n++];
        e.type = c.fx[(size_t) i].type; e.slot = i; e.on = c.fx[(size_t) i].on;
        std::copy (c.fx[(size_t) i].p.begin(), c.fx[(size_t) i].p.end(), e.p);
    }
    for (auto t : auxTypes)
    {
        bool have = false;
        for (int i = 0; i < n; ++i) have |= dst.entries[i].type == t;
        if (have || n >= maxEntries) continue;
        const int rank = vault::fxTypeInfo (t).rank;
        int at = n;
        for (int i = 0; i < n; ++i) if (vault::fxTypeInfo (dst.entries[i].type).rank > rank) { at = i; break; }
        for (int i = n; i > at; --i) dst.entries[i] = dst.entries[i - 1];
        ++n;
        auto& e = dst.entries[at];
        const auto def = VaultFxSlot::make (t);
        e.type = t; e.slot = -1; e.on = true;
        std::copy (def.p.begin(), def.p.end(), e.p);
        if (t == vault::FxType::drive) { e.p[0] = 0.0f; e.p[4] = dst.anyBass ? 1.0f : 0.0f; }
        else if (t == vault::FxType::chorus || t == vault::FxType::delay || t == vault::FxType::reverb) e.p[0] = 0.0f;
    }
    dst.numEntries = n;
    for (auto& f : dst.firstOfType) f = -1;
    for (auto& f : dst.entryOfSlot) f = -1;
    for (int i = 0; i < n; ++i)
    {
        if (dst.firstOfType[(int) dst.entries[i].type] < 0) dst.firstOfType[(int) dst.entries[i].type] = i;
        if (dst.entries[i].slot >= 0) dst.entryOfSlot[dst.entries[i].slot] = i;
    }
    auto entryFor = [&] (const VaultCompiledTarget& t) -> int8_t
    {
        if (t.ref.kind == vault::TargetRef::fxSlot)
        {
            const int e = dst.entryOfSlot[t.ref.a];
            if (e >= 0 && dst.entries[e].type == t.fxType) return (int8_t) e;
            return (int8_t) dst.firstOfType[(int) t.fxType];   // a locked chain: the first slot of the same type
        }
        if (t.ref.kind == vault::TargetRef::fxType) return (int8_t) dst.firstOfType[(int) t.fxType];
        return 0;
    };
    for (int l = 0; l < vault::numLfos; ++l)
        for (int d = 0; d < c.lfo[(size_t) l].numDests; ++d) dst.lfoEntry[l][d] = entryFor (c.lfo[(size_t) l].dests[(size_t) d]);
    for (int i = 0; i < c.numMw; ++i) dst.mwEntry[i] = entryFor (c.mw[(size_t) i]);
    for (int i = 0; i < c.numVel; ++i) dst.velEntry[i] = entryFor (c.vel[(size_t) i]);
    for (int i = 0; i < c.numAt; ++i) dst.atEntry[i] = entryFor (c.at[(size_t) i]);
    for (int m = 0; m < vault::numSoundMacros; ++m)
        for (int t = 0; t < c.numMacroTargets[(size_t) m]; ++t) dst.macroEntry[m][t] = entryFor (c.macro[(size_t) m][(size_t) t]);
}

void VaultEngine::setPreset (const VaultCompiledPreset& p, bool instant, bool applyLocks)
{
    loadProgram (*pending, p, applyLocks);
    hasPending = true;
    hasEdit = false;
    if (instant || preparedParts == 0 || ! haveXfadeSlot) switchInstant();
    // otherwise process() starts the crossfade at its next chunk (or when the running crossfade has finished)
}

void VaultEngine::updateSound (const VaultCompiledPreset& p)
{
    loadProgram (*editProg, p, false);
    hasEdit = true;
    if (preparedParts == 0) applyEdit();
}

void VaultEngine::applyEdit()
{
    hasEdit = false;
    auto& s = slots[cur];
    bool same = editProg->numParts == s.prog->numParts && s.prepared && preparedParts > 0;
    for (int k = 0; same && k < editProg->numParts; ++k)
        same = editProg->kind[k] == s.prog->kind[k] && editProg->instrument[k] == s.prog->instrument[k] && editProg->model[k] == s.prog->model[k];
    if (! same)
    {
        // other layers: a crossfaded switch (the sections are already what the edit wants: no locks)
        std::swap (pending, editProg);
        hasPending = true;
        if (preparedParts == 0 || ! haveXfadeSlot) switchInstant();
        return;
    }
    vault::InstrumentHandle* old[vault::maxParts];
    std::copy (std::begin (s.held), std::end (s.held), old);
    for (auto& h : s.held) h = nullptr;
    std::swap (s.prog, editProg);
    slotAcquire (s);                                   // the same instruments: acquired again before the old hold goes
    for (auto* h : old) if (h != nullptr) h->release();
    if (s.chain != nullptr) s.chain->setLayout (s.prog->entries, s.prog->numEntries);
}

void VaultEngine::resetSlot (Slot& s, bool clearFx)
{
    // the slot is silent (unused, or its crossfade just ended): voices off, LFOs restarted, FX chain cleared (no allocation).
    // Clearing the FX matters: the next sound must not start inside the old reverb / delay content.
    for (int k = 0; k < vault::maxParts; ++k)
        if (auto& p = s.parts[(size_t) k])
        {
            p->synth.allOff (true); p->synth.setSustain (false); p->synth.setSustain (sustain); p->ph1 = p->ph2 = 0;
            p->sampler.allOff (true); p->sampler.setSustain (sustain);
            for (int m = 0; m < p->numPools; ++m) if (auto* mp = p->pools[(size_t) m].load()) mp->allOff (true);
            for (auto& t : p->toneS) t.reset();
        }
    if (clearFx && s.chain != nullptr) s.chain->clear();
    s.peakEnv = 0; s.peakRaw = 0; s.peakThrSm = 0; s.peakHold = 0; s.peakPos = 0;
    for (auto& d : s.peakDl) d.fill (0.0f);
    s.gainsValid = false;
    s.gate.reset();
}

void VaultEngine::switchInstant()
{
    if (! hasPending) return;
    auto& s = slots[cur];
    resetSlot (s, true);
    resetSlot (slots[1 - cur], true);
    slotRelease (s);
    slotRelease (slots[1 - cur]);
    std::swap (s.prog, pending);
    hasPending = false;
    xfading = false;
    slotAcquire (s);
    dcX = dcY = 0;
    for (auto& h : sideHp) h.z1 = h.z2 = 0;
    if (preparedParts == 0 || ! s.prepared) return;
    s.chain->setLayout (s.prog->entries, s.prog->numEntries);
    s.chain->clear();
    s.gateMix = gateOn.load() ? 1.0f : 0.0f;
    updateParams (s, freeBeat, lastBpm, false);
    for (int n = 0; n < 128; ++n) if (held[(size_t) n] > 0.0f) noteOnAll (s, n, held[(size_t) n]);   // keys still down play the new sound
}

void VaultEngine::beginSwitch()
{
    // the free slot (silent, FX cleared when its last crossfade ended) takes the new sound; the current one becomes the
    // outgoing sound and keeps playing - voices, LFOs and its own reverb / delay / chorus tails - while it fades out
    const int next = 1 - cur;
    auto& s = slots[next];
    const auto& o = slots[cur];
    resetSlot (s, false);
    slotRelease (s);
    std::swap (s.prog, pending);
    hasPending = false;
    slotAcquire (s);
    s.chain->setLayout (s.prog->entries, s.prog->numEntries);
    s.trigBeat = o.trigBeat; s.trigTime = o.trigTime;
    for (int i = 0; i < vault::numLfos; ++i) s.lfo[i] = o.lfo[i];
    s.gateMix = o.gateMix;
    cur = next;
    xfading = true;
    xfPos = 0.0f;
    updateParams (s, freeBeat, lastBpm, false);
    for (int n = 0; n < 128; ++n) if (held[(size_t) n] > 0.0f) noteOnAll (s, n, held[(size_t) n]);   // keys still down: new sound
}

void VaultEngine::setMacro (int index, float value01)
{
    if (juce::isPositiveAndBelow (index, (int) vault::numMacros))
        macroTarget[index].store (std::isfinite (value01) ? juce::jlimit (0.0f, 1.0f, value01) : 0.5f);
}
float VaultEngine::getMacro (int index) const
{
    return juce::isPositiveAndBelow (index, (int) vault::numMacros) ? macroTarget[index].load() : 0.5f;
}
void VaultEngine::setQuick (int index, float value01)
{
    if (juce::isPositiveAndBelow (index, (int) vault::numQuick))
        quickTarget[index].store (std::isfinite (value01) ? juce::jlimit (0.0f, 1.0f, value01) : vault::quickInfo()[(size_t) index].neutral);
}
float VaultEngine::getQuick (int index) const
{
    return juce::isPositiveAndBelow (index, (int) vault::numQuick) ? quickTarget[index].load() : 0.5f;
}
void VaultEngine::setVolume (float g) { volumeTarget.store (std::isfinite (g) ? juce::jlimit (0.0f, 2.0f, g) : 1.0f); }
void VaultEngine::setFxOn (bool on) { fxOn.store (on); }
void VaultEngine::setArpOn (bool on) { arpOn.store (on); }

int VaultEngine::arpHeldNotes (std::vector<std::pair<int, float>>& out) const
{
    out.clear();
    const int c = juce::jlimit (0, 128, arpHeldCount.load (std::memory_order_acquire));
    for (int i = 0; i < c; ++i)
    {
        const int v = arpHeldUi[(size_t) i].load (std::memory_order_relaxed);
        out.push_back ({ v & 0x7f, (float) ((v >> 8) & 0x7f) / 127.0f });
    }
    return c;
}

void VaultEngine::soundingNotes (uint64_t mask[2]) const
{
    for (int w = 0; w < 2; ++w)
        mask[w] = soundingUi[w].load (std::memory_order_relaxed) | startedUi[w].exchange (0, std::memory_order_relaxed);
}

bool VaultEngine::isNoteSounding (int note) const
{
    return juce::isPositiveAndBelow (note, 128) && ((soundingUi[note >> 6].load (std::memory_order_relaxed) >> (note & 63)) & 1u) != 0;
}

void VaultEngine::setGateOn (bool on) { gateOn.store (on); }
void VaultEngine::setLocks (bool fx, bool a, bool g) { lockFx.store (fx); lockArp.store (a); lockGate.store (g); }

int VaultEngine::chainSize() const { return slots[cur].chain != nullptr ? slots[cur].chain->size() : 0; }
vault::FxType VaultEngine::chainType (int e) const { return juce::isPositiveAndBelow (e, chainSize()) ? slots[cur].chain->entry (e).type : vault::FxType::reverb; }
int VaultEngine::chainSlot (int e) const { return juce::isPositiveAndBelow (e, chainSize()) ? slots[cur].chain->entry (e).slot : -1; }
float VaultEngine::chainParam (int e, int p) const
{
    return juce::isPositiveAndBelow (e, chainSize()) && juce::isPositiveAndBelow (p, (int) vault::maxFxParams) ? slots[cur].chain->entry (e).p[p] : 0.0f;
}
float VaultEngine::chainOnGain (int e) const { return juce::isPositiveAndBelow (e, chainSize()) ? slots[cur].chain->onGain (e) : 0.0f; }
float VaultEngine::effectiveFinish() const { return slots[cur].eff != nullptr ? slots[cur].eff->finish : 0.0f; }

int VaultEngine::readNoteLog (NoteLog* out, int maxCount) const
{
    const int n = std::min ({ noteLogCount, maxCount, noteLogSize });
    for (int i = 0; i < n; ++i) out[i] = noteLog[i];
    return n;
}
void VaultEngine::logNote (int note, float vel, bool on)
{
    if (noteLogCount < noteLogSize) noteLog[noteLogCount++] = { eventClock, note, vel, on };
}

void VaultEngine::noteOnAll (Slot& s, int note, float vel)
{
    const auto& pr = *s.prog;
    for (int k = 0; k < numParts (s); ++k)
    {
        auto& part = *s.parts[(size_t) k];
        const int n = juce::jlimit (0, 127, note + pr.transpose[k]);
        switch (pr.kind[k])
        {
            case VaultPartKind::synth:  part.synth.noteOn (n, vel, s.vp[k]); break;
            case VaultPartKind::sample: part.sampler.noteOn (n, vel); break;
            case VaultPartKind::model:
                if (auto* mp = poolFor (s, k))
                    mp->noteOn (n, vel, kk::semisToHz ((float) n + pitchWheel * pr.values[k][kk::vk::bendRange]), pr.mp[k], pr.numModelParams[k]);
                break;
        }
    }
}

void VaultEngine::noteOffAll (Slot& s, int note)
{
    const auto& pr = *s.prog;
    for (int k = 0; k < numParts (s); ++k)
    {
        auto& part = *s.parts[(size_t) k];
        const int n = juce::jlimit (0, 127, note + pr.transpose[k]);
        switch (pr.kind[k])
        {
            case VaultPartKind::synth:  part.synth.noteOff (n, s.vp[k]); break;
            case VaultPartKind::sample: part.sampler.noteOff (n); break;
            case VaultPartKind::model:  if (auto* mp = poolFor (s, k)) mp->noteOff (n, sustain); break;
        }
    }
}

VaultEngine::ModelPool* VaultEngine::poolFor (Slot& s, int k) const
{
    const int m = s.prog->model[k];
    auto& part = s.parts[(size_t) k];
    if (m < 0 || part == nullptr || m >= part->numPools) return nullptr;
    return part->pools[(size_t) m].load (std::memory_order_acquire);
}

void VaultEngine::slotAcquire (Slot& s)
{
    // the slot's program becomes audible: hold its instruments (no eviction while it plays), ask for missing ones
    for (int k = 0; k < vault::maxParts; ++k)
    {
        auto* h = k < s.prog->numParts ? s.prog->instrument[k] : nullptr;
        if (h != nullptr) { h->acquire(); if (! h->isLoaded()) h->request(); }
        s.held[k] = h;
        if (k < s.prog->numParts && s.prog->kind[k] == VaultPartKind::model && s.parts[(size_t) k] != nullptr && poolFor (s, k) == nullptr)
            s.parts[(size_t) k]->modelWanted.store (s.prog->model[k]);
    }
}

void VaultEngine::slotRelease (Slot& s)
{
    for (auto& h : s.held) if (h != nullptr) { h->release(); h = nullptr; }
}

void VaultEngine::serviceResources()
{
    const std::lock_guard<std::mutex> g (resourceLock);
    if (preparedParts == 0) return;
    const auto& reg = vault::models::ModelRegistry::get();
    for (auto& sl : slots)
        for (auto& part : sl.parts)
        {
            if (part == nullptr) continue;
            const int m = part->modelWanted.exchange (-1);
            if (m < 0 || m >= part->numPools || part->pools[(size_t) m].load() != nullptr) continue;
            auto pool = std::make_unique<ModelPool>();
            pool->prepare (reg.at (m), sr, chunk);
            part->pools[(size_t) m].store (pool.get(), std::memory_order_release);
            part->ownedPools.push_back (std::move (pool));
        }
}

void VaultEngine::allNotesOff (bool hard)
{
    held.fill (0.0f);
    for (auto& m : soundingUi) m.store (0, std::memory_order_relaxed);
    keysDown.fill (0.0f);
    arp.reset();
    for (auto& sl : slots)
        for (auto& p : sl.parts)
            if (p)
            {
                p->synth.setSustain (false); p->synth.allOff (hard);
                p->sampler.setSustain (false); p->sampler.allOff (hard);
                for (int mi = 0; mi < p->numPools; ++mi) if (auto* mp = p->pools[(size_t) mi].load()) mp->allOff (hard);
            }
    sustain = false;
}

void VaultEngine::handleNote (bool on, int n, float vel)
{
    auto& s = slots[cur];
    auto& old = slots[1 - cur];
    n = juce::jlimit (0, 127, n);
    if (on)
    {
        bool anyHeld = false;
        for (float h : held) anyHeld |= h > 0.0f;
        // the first note: LFO retrigger (phase, delay, fade) and the gate's start while the host is stopped. With the arp
        // on, its notes have gaps - the keys decide instead (a new chord, type 3 events: the arp's free clock starts there too)
        if (! anyHeld && ! arpWasOn) retriggerClock (clockBeat + (double) (eventClock - sampleClock) * clockBps);
        held[(size_t) n] = std::max (0.01f, vel);
        lastVelocity = held[(size_t) n];
        noteOnAll (s, n, held[(size_t) n]);            // new notes only on the current sound
        logNote (n, vel, true);
    }
    else
    {
        if (held[(size_t) n] <= 0.0f) { noteOffAll (s, n); if (xfading) noteOffAll (old, n); return; }
        held[(size_t) n] = 0.0f;
        noteOffAll (s, n);
        if (xfading) noteOffAll (old, n);
        logNote (n, 0.0f, false);
    }
}

void VaultEngine::retriggerClock (double beat)
{
    for (auto* sl : { &slots[cur], &slots[1 - cur] })
    {
        sl->trigBeat = beat; sl->trigTime = (double) eventClock / sr;
        for (int i = 0; i < vault::numLfos; ++i)
            if (sl->prog->ctl.lfo[(size_t) i].retrig) sl->lfo[i].phase = sl->prog->ctl.lfo[(size_t) i].phase;
    }
}

void VaultEngine::handleOther (const juce::MidiMessage& m)
{
    auto& s = slots[cur];
    if (m.isSustainPedalOn() || m.isSustainPedalOff())
    {
        sustain = m.isSustainPedalOn();
        for (auto& sl : slots)
            for (auto& p : sl.parts)
                if (p)
                {
                    p->synth.setSustain (sustain);
                    p->sampler.setSustain (sustain);
                    if (! sustain) for (int mi = 0; mi < p->numPools; ++mi) if (auto* mp = p->pools[(size_t) mi].load()) mp->pedalUp();
                }
    }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff (m.isAllSoundOff());
    else if (m.isPitchWheel())      pitchWheel = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isChannelPressure()) aftertouch = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAftertouch())      { for (int k = 0; k < numParts (s); ++k)
                                          s.parts[(size_t) k]->synth.polyAftertouch (juce::jlimit (0, 127, m.getNoteNumber() + s.prog->transpose[k]),
                                                                                     (float) m.getAfterTouchValue() / 127.0f); }
    else if (m.isController() && m.getControllerNumber() == 1) modWheel = (float) m.getControllerValue() / 127.0f;
}

//==============================================================================
// the effective parameters of one block: base -> sound macros -> LFOs / routings -> classic macros -> quick knobs
void VaultEngine::computeEffective (Slot& s, double beat, double bpm, bool advance)
{
    auto& E = *s.eff;
    const auto& pr = *s.prog;
    const auto& c = pr.ctl;
    const int ne = pr.numEntries;
    std::fill (std::begin (E.glob), std::end (E.glob), 0.0f);
    for (int q = 0; q < vault::numQuick; ++q) E.quick[q] = quickSm[q];
    for (auto& v : E.classic) v = 0.5f;
    for (auto& a : E.layerAbsSet) a[0] = a[1] = false;
    for (auto& a : E.layerAdd) std::fill (std::begin (a), std::end (a), 0.0f);
    for (int e = 0; e < ne; ++e)
    {
        std::copy (pr.entries[e].p, pr.entries[e].p + vault::maxFxParams, E.fx[e]);
        std::fill (std::begin (E.fxNorm[e]), std::end (E.fxNorm[e]), 0.0f);
        E.fxTouched[e] = false;
    }
    for (int i = 0; i < vault::numLfos; ++i) { E.lfoDepth[i] = c.lfo[(size_t) i].depth; E.lfoRate[i] = c.lfo[(size_t) i].rate; E.lfoDepthAdd[i] = E.lfoRateAdd[i] = 0; }
    E.gate[0] = c.gate.depth; E.gate[1] = c.gate.mix; E.gate[2] = c.gate.smooth;
    E.arpGate = c.arp.gate; E.finish = c.finish;

    using T = vault::TargetRef;
    // macros: absolute targets are set to min..max, relative ones get the offset
    auto setTarget = [&] (const VaultCompiledTarget& t, int entry, float v)
    {
        switch (t.ref.kind)
        {
            case T::pitch: E.glob[0] += v; break;
            case T::cutoff: E.glob[1] += v; break;
            case T::reso: E.glob[2] += v; break;
            case T::volume: E.glob[3] += v; break;
            case T::pan: E.glob[4] += v; break;
            case T::quick: E.quick[t.ref.a] += v; break;
            case T::classic: E.classic[t.ref.a] = v; break;
            case T::layer:
                if (t.ref.b <= T::lPan) { E.layerAbs[t.ref.a][t.ref.b] = v; E.layerAbsSet[t.ref.a][t.ref.b] = true; }
                else E.layerAdd[t.ref.a][t.ref.b] += v;
                break;
            case T::fxType: case T::fxSlot:
                if (entry >= 0 && entry < ne) { E.fx[entry][t.ref.b] = v; E.fxTouched[entry] = true; }
                break;
            case T::lfo: if (t.ref.b == T::lfoDepth) E.lfoDepth[t.ref.a] = v; else E.lfoRate[t.ref.a] = v; break;
            case T::gate: E.gate[t.ref.a] = v; break;
            case T::arpGate: E.arpGate = v; break;
            case T::finish: E.finish = v; break;
            case T::none: default: break;
        }
    };
    // modulation (LFOs, mod wheel, velocity, aftertouch): adds in the target's modulation units
    auto modTarget = [&] (const VaultCompiledTarget& t, int entry, float x)
    {
        const float v = x * t.amount;
        if (! nz (v)) return;
        const auto k = t.ref.kind;
        if (k == T::fxType || k == T::fxSlot) { if (entry >= 0 && entry < ne) { E.fxNorm[entry][t.ref.b] += v; E.fxTouched[entry] = true; } }
        else if (k == T::layer) E.layerAdd[t.ref.a][t.ref.b] += v;   // level: x (1 + v), pan: +, the others in their units
        else if (k == T::lfo) { if (t.ref.b == T::lfoDepth) E.lfoDepthAdd[t.ref.a] += v; else E.lfoRateAdd[t.ref.a] += v; }
        else if (k == T::classic) E.classic[t.ref.a] += v;
        else if (k == T::gate) E.gate[t.ref.a] += v;
        else if (k == T::arpGate) E.arpGate += v;
        else if (k == T::finish) E.finish += v;
        else setTarget (t, entry, v);   // pitch / cutoff / reso / volume / pan / quick: relative, they add the same way
    };
    for (int m = 0; m < vault::numSoundMacros; ++m)
        for (int t = 0; t < c.numMacroTargets[(size_t) m]; ++t)
        {
            const auto& tg = c.macro[(size_t) m][(size_t) t];
            const float v = tg.min + (tg.max - tg.min) * macroCurve (macroSm[m], tg.curve);
            setTarget (tg, pr.macroEntry[m][t], v);   // absolute targets are set, relative ones offset (TargetRef::absolute)
        }
    for (int i = 0; i < c.numMw; ++i) modTarget (c.mw[(size_t) i], pr.mwEntry[i], modWheel);
    for (int i = 0; i < c.numVel; ++i) modTarget (c.vel[(size_t) i], pr.velEntry[i], lastVelocity);
    for (int i = 0; i < c.numAt; ++i) modTarget (c.at[(size_t) i], pr.atEntry[i], aftertouch);

    // LFOs (control rate)
    const double now = (double) sampleClock / sr;
    for (int i = 0; i < vault::numLfos; ++i)
    {
        const auto& L = c.lfo[(size_t) i];
        auto& st = s.lfo[i];
        st.prevValue = st.value;
        if (L.numDests == 0) { st.value = 0; continue; }
        const float depth = juce::jlimit (0.0f, 1.0f, E.lfoDepth[i] + E.lfoDepthAdd[i]);
        const float rate = juce::jlimit (0.005f, 80.0f, E.lfoRate[i] * std::exp2 (4.0f * E.lfoRateAdd[i]));
        double ph;
        if (L.sync)
        {
            const double cyc = vault::divisionBeats (L.div);
            ph = (L.retrig ? (beat - s.trigBeat) : beat) / cyc + L.phase;
        }
        else
        {
            ph = st.phase;
            if (advance) { st.phase += rate * (double) chunk / sr; st.phase -= std::floor (st.phase); }
        }
        const float f = (float) (ph - std::floor (ph));
        if (f < st.lastPh) st.sh = s.parts[0] != nullptr ? s.parts[0]->rng.bi() : 0.0f;
        st.lastPh = f;
        const double t = now - s.trigTime;
        float env = 1.0f;
        if (L.delay > 0 || L.fade > 0)
        {
            if (t < L.delay) env = 0.0f;
            else if (L.fade > 0) env = (float) std::min (1.0, (t - L.delay) / L.fade);
        }
        st.value = vault::lfoValue (L.shape, f, st.sh) * env * depth;
        if (&s == &slots[cur]) { lfoPhaseUi[i].store (f, std::memory_order_relaxed); lfoValueUi[i].store (st.value, std::memory_order_relaxed); }
        for (int d = 0; d < L.numDests; ++d) modTarget (L.dests[(size_t) d], pr.lfoEntry[i][d], st.value);
    }
    juce::ignoreUnused (bpm);

    // built-in vibrato (MAIN page, LFO-free): 5.5 Hz, up to +-0.6 semitones
    if (const float vq = E.quick[vault::qVibrato]; vq > 0.0005f)
    {
        E.glob[0] += vq * 0.6f * std::sin (s.vibPhase);
        s.vibPhase += kk::twoPi * 5.5f * (float) chunk / (float) sr;
        if (s.vibPhase > kk::twoPi) s.vibPhase -= kk::twoPi;
    }
    for (auto& q : E.quick) q = juce::jlimit (0.0f, 1.0f, q);

    // classic macros (v1 / v2 presets) on the first slot of each type - exactly the v2 master FX macros
    {
        const float tone = E.classic[0], space = E.classic[1], drv = E.classic[2], mot = E.classic[3];
        const float toneD = 2.0f * tone - 1.0f;
        auto first = [&] (vault::FxType t) { return pr.firstOfType[(int) t]; };
        if (const int e = first (vault::FxType::eq); e >= 0 && nz (toneD))
        {
            E.fx[e][6] = juce::jlimit (-12.0f, 12.0f, E.fx[e][6] + toneD * 5.0f);
            E.fx[e][1] = juce::jlimit (-12.0f, 12.0f, E.fx[e][1] - toneD * 2.5f);
        }
        if (const int e = first (vault::FxType::reverb); e >= 0)
        {
            E.fx[e][0] = kk::clamp01 (E.fx[e][0] * lowHalf (space) + upHalf (space) * 0.45f);
            E.fx[e][1] = kk::clamp01 (E.fx[e][1] + upHalf (space) * 0.3f);
        }
        if (const int e = first (vault::FxType::delay); e >= 0) E.fx[e][0] = kk::clamp01 (E.fx[e][0] * lowHalf (space) + upHalf (space) * 0.25f);
        if (const int e = first (vault::FxType::width); e >= 0) E.fx[e][0] = kk::clamp01 (E.fx[e][0] * (0.5f + 0.5f * lowHalf (space)) + upHalf (space) * 0.4f);
        if (const int e = first (vault::FxType::drive); e >= 0) E.fx[e][0] = kk::clamp01 (E.fx[e][0] * lowHalf (drv) + upHalf (drv) * 0.7f);
        if (const int e = first (vault::FxType::chorus); e >= 0) E.fx[e][0] = kk::clamp01 (E.fx[e][0] * lowHalf (mot) + upHalf (mot) * 0.5f);
        if (const int e = first (vault::FxType::phaser); e >= 0) E.fx[e][0] *= lowHalf (mot);
        if (const int e = first (vault::FxType::flanger); e >= 0) E.fx[e][0] *= lowHalf (mot);
        // the MAIN page EFFECTS knobs
        if (const int e = first (vault::FxType::reverb); e >= 0) E.fx[e][0] = quickAmount (E.fx[e][0], E.quick[vault::qReverb], 0.7f);
        if (const int e = first (vault::FxType::delay); e >= 0) E.fx[e][0] = quickAmount (E.fx[e][0], E.quick[vault::qDelay], 0.6f);
        if (const int e = first (vault::FxType::chorus); e >= 0) E.fx[e][0] = quickAmount (E.fx[e][0], E.quick[vault::qChorus], 1.0f);
        if (const int e = first (vault::FxType::drive); e >= 0) E.fx[e][0] = quickAmount (E.fx[e][0], E.quick[vault::qDrive], 0.8f);
        if (const int e = first (vault::FxType::width); e >= 0) E.fx[e][0] = quickAmount (E.fx[e][0], E.quick[vault::qWidth], 1.0f);
        if (const int e = first (vault::FxType::eq); e >= 0 && ! same (E.quick[vault::qEqTilt], 0.5f))
            E.fx[e][0] = juce::jlimit (-12.0f, 12.0f, E.fx[e][0] + (E.quick[vault::qEqTilt] - 0.5f) * 18.0f);
    }
    // modulation of FX parameters (0..1 of the range) and macro-set values back into range
    for (int e = 0; e < ne; ++e)
    {
        if (! E.fxTouched[e]) continue;
        const auto& ti = vault::fxTypeInfo (pr.entries[e].type);
        for (size_t p = 0; p < ti.params.size(); ++p)
        {
            const auto& in = ti.params[p];
            float v = juce::jlimit (in.lo, in.hi, E.fx[e][p]);
            if (nz (E.fxNorm[e][p])) v = in.fromNorm (in.toNorm (v) + E.fxNorm[e][p]);
            else if (in.flags & (vault::FxParamInfo::integer | vault::FxParamInfo::toggle)) v = std::round (v);
            E.fx[e][p] = v;
        }
    }
    for (auto& g : E.gate) g = juce::jlimit (0.0f, 1.0f, g);
    E.arpGate = juce::jlimit (0.01f, 1.0f, E.arpGate);
    E.finish = juce::jlimit (0.0f, 1.0f, E.finish);
    if (s.chain != nullptr)
        for (int e = 0; e < std::min (ne, s.chain->size()); ++e)
        {
            auto& ce = s.chain->entry (e);
            std::copy (E.fx[e], E.fx[e] + vault::maxFxParams, ce.p);
            ce.on = pr.entries[e].on;
        }
}

void VaultEngine::updateParams (Slot& s, double beatPos, double bpm, bool advance)
{
    computeEffective (s, beatPos, bpm, advance);
    const auto& E = *s.eff;
    const auto& pr = *s.prog;
    kk::VoiceBuildContext ctx;
    ctx.pitchWheel = juce::jlimit (-1.0f, 1.0f, pitchWheel);
    ctx.modWheel = modWheel; ctx.aftertouch = aftertouch;
    const float tone = E.classic[0], mot = E.classic[3];
    const float toneD = 2.0f * tone - 1.0f;
    using T = vault::TargetRef;
    // quick knobs on the layers
    const float qCut = (E.quick[vault::qCutoff] - 0.5f) * 8.0f, qRes = (E.quick[vault::qReso] - 0.5f) * 2.0f;
    const float qEnv = (E.quick[vault::qEnvAmt] - 0.5f) * 2.0f, qGlide = (E.quick[vault::qGlide] - 0.5f) * 2.0f;
    const float qa = (E.quick[vault::qAttack] - 0.5f) * 2.0f, qr = (E.quick[vault::qRelease] - 0.5f) * 2.0f;
    const float atkMul = std::exp2 (qa * 4.0f), atkAdd = qa > 0 ? qa * qa * 2.0f : 0.0f;
    const float relMul = std::exp2 (qr * 4.0f), relAdd = qr > 0 ? qr * qr * 4.0f : 0.0f;
    for (int k = 0; k < pr.numParts; ++k)
    {
        const float* vals = pr.values[k];
        kk::FxParams dummy;
        kk::buildVoiceParamsFrom ([vals] (int key) { return vals[key]; }, ctx, s.vp[k], dummy);
        auto& v = s.vp[k];
        v.glideLegatoOnly = true;   // glide only between overlapping notes: a detached note (piano roll) starts on its own pitch
        v.noteSeeded = true;        // every note starts its drift / unison phases the same way: C5 after B4 sounds as C5 alone
        // TONE: filter tilt (+-1.5 octaves of cutoff)
        v.cutoff = juce::jlimit (40.0f, 20000.0f, v.cutoff * std::exp2 (toneD * 1.5f));
        // MOTION: LFO depth (designed amount below 0.5, extra filter / pitch movement and drift above)
        v.lfoFilter = kk::clamp01 (v.lfoFilter * lowHalf (mot) + upHalf (mot) * 0.35f);
        v.lfoPitch = kk::clamp01 (v.lfoPitch * lowHalf (mot) + upHalf (mot) * 0.04f);
        v.lfoAmp *= lowHalf (mot);
        v.drift = kk::clamp01 (v.drift + upHalf (mot) * 0.3f);
        // v3: quick knobs + modulation
        const float* la = E.layerAdd[k];
        if (const float co = qCut + E.glob[1] + la[T::lCutoff]; nz (co)) v.cutoff = juce::jlimit (40.0f, 20000.0f, v.cutoff * std::exp2 (co));
        if (const float ro = qRes + E.glob[2] + la[T::lReso]; nz (ro)) v.reso = kk::clamp01 (v.reso + ro);
        if (nz (qEnv)) v.fenv = juce::jlimit (-1.0f, 1.0f, v.fenv + qEnv);
        if (nz (qGlide)) v.glide = kk::clamp01 (v.glide + qGlide);
        if (nz (qa) || nz (la[T::lAttack])) v.attack = juce::jlimit (0.001f, 10.0f, v.attack * atkMul * std::exp2 (la[T::lAttack]) + atkAdd);
        if (nz (qr) || nz (la[T::lRelease])) v.release = juce::jlimit (0.005f, 12.0f, v.release * relMul * std::exp2 (la[T::lRelease]) + relAdd);
        if (const float st = E.glob[0] + la[T::lTune]; nz (st)) v.pitchWheel = ctx.pitchWheel + st / std::max (1.0f, v.bendRange);
    }
    // sample / model parts: TONE = cutoff tilt, MOTION = vibrato (+ the FX chorus), the pitch wheel, quick knobs, modulation
    bool waiting = false;
    for (int k = 0; k < numParts (s); ++k)
    {
        auto& part = *s.parts[(size_t) k];
        const float* la = E.layerAdd[k];
        const float cutOct = qCut + E.glob[1] + la[T::lCutoff], resoAdd = qRes + E.glob[2] + la[T::lReso], semis = E.glob[0] + la[T::lTune];
        if (pr.kind[k] == VaultPartKind::sample)
        {
            const auto* h = pr.instrument[k];
            const auto* d = h != nullptr ? h->data() : nullptr;
            waiting |= h != nullptr && d == nullptr && ! h->hasFailed();
            part.sampler.setSource (d, pr.articulation[k]);
            if (nz (resoAdd) || nz (qa) || nz (qr) || nz (la[T::lAttack]) || nz (la[T::lRelease]))
            {
                std::copy (pr.sp[k], pr.sp[k] + vault::numSampleParams, part.spTmp);
                part.spTmp[vault::spReso] = kk::clamp01 (part.spTmp[vault::spReso] + resoAdd);
                part.spTmp[vault::spAttack] = juce::jlimit (0.0f, 10.0f, part.spTmp[vault::spAttack] * atkMul * std::exp2 (la[T::lAttack]) + atkAdd);
                if (nz (qr) || nz (la[T::lRelease]))
                    part.spTmp[vault::spRelease] = juce::jlimit (0.0f, 20.0f, (part.spTmp[vault::spRelease] > 0 ? part.spTmp[vault::spRelease] : 0.3f) * relMul * std::exp2 (la[T::lRelease]) + relAdd);
                part.sampler.setParams (part.spTmp);
            }
            else part.sampler.setParams (pr.sp[k]);
            const float vib = pr.sp[k][vault::spVibrato] * lowHalf (mot) + upHalf (mot) * 14.0f;
            float cutMul = std::exp2 (toneD * 1.5f);
            if (nz (cutOct)) cutMul *= std::exp2 (cutOct);
            part.sampler.setControl (cutMul, vib, ctx.pitchWheel, semis);
        }
        else if (pr.kind[k] == VaultPartKind::model)
        {
            auto* mp = poolFor (s, k);
            if (mp == nullptr && pr.model[k] >= 0) { waiting = true; part.modelWanted.store (pr.model[k]); }
            float cut = juce::jlimit (40.0f, 20000.0f, 20000.0f * std::exp2 (toneD * 1.5f));
            if (cutOct < 0.0f || (cutOct > 0.0f && cut < 20000.0f)) cut = juce::jlimit (40.0f, 20000.0f, cut * std::exp2 (cutOct));
            part.toneOn = cut < 19000.0f;
            if (part.toneOn) part.toneC.set (cut, resoAdd > 0.0f ? std::max (0.1f, 1.414f - 1.3f * kk::clamp01 (resoAdd)) : 1.414f, (float) sr);
            if (mp != nullptr)
            {
                const float vibC = upHalf (mot) * 14.0f;
                const float vib = vibC * std::sin (part.vibPhase) * 0.01f;
                part.vibPhase += juce::MathConstants<float>::twoPi * 5.2f * (float) chunk / (float) sr;
                if (part.vibPhase > juce::MathConstants<float>::twoPi) part.vibPhase -= juce::MathConstants<float>::twoPi;
                float bend = ctx.pitchWheel * pr.values[k][kk::vk::bendRange] + vib;
                if (nz (semis)) bend += semis;
                for (size_t i = 0; i < mp->voices.size(); ++i)
                    if (mp->voices[i]->isActive() && mp->note[i] >= 0) mp->voices[i]->setPitch (kk::semisToHz ((float) mp->note[i] + bend));
            }
        }
    }
    if (&s == &slots[cur]) loading.store (waiting, std::memory_order_relaxed);
}

void VaultEngine::renderLfos (Slot& s, int len, double beat0, double bps, int c0)
{
    for (int k = 0; k < numParts (s); ++k)
    {
        auto& p = *s.parts[(size_t) k];
        const float* vals = s.prog->values[k];
        auto gen = [&] (int syncK, int divK, int rateK, int shapeK, float& phase, float& last, float& sh, float* out)
        {
            const bool sync = vals[syncK] > 0.5f;
            const double beats = Choices::lfoDivBeats ((int) vals[divK]);
            const float inc = vals[rateK] / (float) sr;
            const int shape = (int) vals[shapeK];
            for (int i = 0; i < len; ++i)
            {
                float ph;
                if (sync) { const double x = (beat0 + bps * (c0 + i)) / beats; ph = (float) (x - std::floor (x)); }
                else { ph = phase; phase += inc; if (phase >= 1) phase -= 1; }
                if (ph < last) sh = p.rng.bi();
                last = ph;
                out[i] = kk::lfoShape (shape, ph, sh);
            }
        };
        gen (kk::vk::lfoSync, kk::vk::lfoDiv, kk::vk::lfoRate, kk::vk::lfoShape, p.ph1, p.last1, p.sh1, s.lfo1.data() + k * chunk);
        gen (kk::vk::lfo2Sync, kk::vk::lfo2Div, kk::vk::lfo2Rate, kk::vk::lfo2Shape, p.ph2, p.last2, p.sh2, s.lfo2.data() + k * chunk);
    }
}

void VaultEngine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Transport& transport)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nCh = buffer.getNumChannels();
    if (n <= 0 || nCh == 0) return;
    if (preparedParts == 0 || slots[0].chain == nullptr) { buffer.clear(); return; }

    const double bpm = juce::jlimit (20.0, 999.0, transport.bpm > 0 ? transport.bpm : 120.0);
    lastBpm = bpm;
    const double bps = bpm / 60.0 / sr;
    const double beat0 = transport.playing ? transport.ppq : freeBeat;
    freeBeat = beat0 + bps * n;
    auto* outL = buffer.getWritePointer (0);
    auto* outR = nCh > 1 ? buffer.getWritePointer (1) : nullptr;
    const float smMacro = 1.0f - std::exp (-(float) chunk / (0.03f * (float) sr));
    const float smVol = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
    constexpr float halfPi = juce::MathConstants<float>::halfPi;

    // ---- the block's note events: the input, through the arpeggiator when it runs
    evs.clear();
    auto addEv = [&] (int sample, uint8_t type, int note, float vel)
    {
        if (evs.size() < evs.capacity()) evs.push_back ({ juce::jlimit (0, n - 1, sample), type, note, vel, { 0, 0, 0 }, 0 });
    };
    {
        if (hasEdit) applyEdit();   // the arp settings of an edit apply from this block on
        const bool arpNow = arpOn.load();
        auto a = slots[cur].prog->ctl.arp;
        if (slots[cur].eff != nullptr) a.gate = slots[cur].eff->arpGate;
        arp.setSettings (a);
        if (arpNow != arpWasOn)
        {
            if (arpNow)
            {
                for (int k = 0; k < 128; ++k) if (held[(size_t) k] > 0.0f) addEv (0, 1, k, 0.0f);   // the played notes stop, the arp takes the keys
                arp.reset(); arp.setSettings (a);
                for (int k = 0; k < 128; ++k) if (keysDown[(size_t) k] > 0.0f) arp.keyDown (k, keysDown[(size_t) k]);
            }
            else
            {
                const int c = arp.allNotesOff (arpEvs.data());
                for (int i = 0; i < c; ++i) addEv (0, 1, arpEvs[(size_t) i].note, 0.0f);
                arp.reset();
                for (int k = 0; k < 128; ++k) if (keysDown[(size_t) k] > 0.0f) addEv (0, 0, k, keysDown[(size_t) k]);   // keys still down play
            }
            arpWasOn = arpNow;
        }
        int seg = 0;
        vault::Arp::Clock clk;
        clk.bps = bps; clk.hostPlaying = transport.playing;
        clk.barStart = transport.barPpq; clk.barBeats = transport.barBeats > 0 ? transport.barBeats : 4.0;
        if (transport.playing) { clk.loopStart = transport.loopStart; clk.loopEnd = transport.loopEnd; }
        auto runArp = [&] (int upTo)
        {
            if (! arpNow || upTo <= seg) return;
            clk.beat0 = beat0 + bps * seg;
            const int c = arp.process (upTo - seg, clk, arpEvs.data());
            for (int i = 0; i < c; ++i) addEv (seg + arpEvs[(size_t) i].sample, arpEvs[(size_t) i].on ? 0 : 1, arpEvs[(size_t) i].note, arpEvs[(size_t) i].velocity);
            seg = upTo;
        };
        // the block's MIDI (short messages); with the arp on, events on the same sample go offs -> others -> ons, so a
        // chord change on a grid line (old chord's offs + new chord's ons in any order) is one clean new chord
        inEvs.clear();
        for (const auto meta : midi)
        {
            if (meta.numBytes > 3 || inEvs.size() >= inEvs.capacity()) continue;
            const auto m = meta.getMessage();
            Ev e { juce::jlimit (0, n - 1, meta.samplePosition), 2, 0, 0.0f, { 0, 0, 0 }, meta.numBytes };
            for (int b = 0; b < meta.numBytes; ++b) e.b[b] = meta.data[b];
            if (m.isNoteOn()) { e.type = 0; e.note = m.getNoteNumber(); e.vel = m.getFloatVelocity(); }
            else if (m.isNoteOff()) { e.type = 1; e.note = m.getNoteNumber(); }
            inEvs.push_back (e);
        }
        if (arpNow)
        {
            auto rank = [] (const Ev& e) { return e.type == 1 ? 0 : e.type == 2 ? 1 : 2; };
            for (size_t i = 1; i < inEvs.size(); ++i)   // insertion sort: stable, no allocation
            {
                const Ev x = inEvs[i];
                size_t j = i;
                while (j > 0 && (inEvs[j - 1].sample > x.sample || (inEvs[j - 1].sample == x.sample && rank (inEvs[j - 1]) > rank (x)))) { inEvs[j] = inEvs[j - 1]; --j; }
                inEvs[j] = x;
            }
        }
        for (const auto& e : inEvs)
        {
            const int pos = e.sample;
            if (e.type == 0)
            {
                if (arpNow)
                {
                    bool anyDown = false;
                    for (float k : keysDown) anyDown |= k > 0.0f;
                    if (! anyDown) { runArp (pos); addEv (pos, 3, e.note, 0.0f); }   // a new chord: LFO retrigger / gate clock (free) at the key
                }
                keysDown[(size_t) e.note] = std::max (0.01f, e.vel);
                if (arpNow) { runArp (pos); arp.keyDown (e.note, keysDown[(size_t) e.note]); lastVelocity = keysDown[(size_t) e.note]; }
                else addEv (pos, 0, e.note, e.vel);
            }
            else if (e.type == 1)
            {
                keysDown[(size_t) e.note] = 0.0f;
                if (arpNow) { runArp (pos); arp.keyUp (e.note); }
                else addEv (pos, 1, e.note, 0.0f);
            }
            else if (evs.size() < evs.capacity())
            {
                const auto m = e.len == 3 ? juce::MidiMessage (e.b[0], e.b[1], e.b[2]) : e.len == 2 ? juce::MidiMessage (e.b[0], e.b[1]) : juce::MidiMessage (e.b[0]);
                if (arpNow) runArp (pos);
                if (arpNow && (m.isAllNotesOff() || m.isAllSoundOff()))
                {
                    const int c = arp.allNotesOff (arpEvs.data());
                    for (int i = 0; i < c; ++i) addEv (pos, 1, arpEvs[(size_t) i].note, 0.0f);
                    arp.reset();
                }
                if (arpNow && m.isSustainPedalOn()) { arp.setPedal (true); continue; }   // the pedal holds the arp's keys, not its notes
                if (arpNow && m.isSustainPedalOff()) arp.setPedal (false);
                evs.push_back (e);
            }
        }
        runArp (n);
        arpStepUi.store (arpNow ? arp.stepIndex() : -1, std::memory_order_relaxed);
        if (arp.heldVersion() != arpHeldVer)   // the arp's notes for the MIDI export (message thread reads)
        {
            arpHeldVer = arp.heldVersion();
            int notes[128]; float vels[128];
            const int c = arpNow ? arp.heldNotes (notes, vels, 128) : 0;
            for (int i = 0; i < c; ++i) arpHeldUi[(size_t) i].store (notes[i] | (juce::jlimit (1, 127, juce::roundToInt (vels[i] * 127.0f)) << 8), std::memory_order_relaxed);
            arpHeldCount.store (c, std::memory_order_release);
        }
    }
    size_t evIt = 0;
    auto dispatch = [&] (const Ev& e)
    {
        if (e.type == 0) handleNote (true, e.note, e.vel);
        else if (e.type == 1) handleNote (false, e.note, 0.0f);
        else if (e.type == 3) retriggerClock (clockBeat + (double) (eventClock - sampleClock) * clockBps);
        else
        {
            const auto m = e.len == 3 ? juce::MidiMessage (e.b[0], e.b[1], e.b[2]) : e.len == 2 ? juce::MidiMessage (e.b[0], e.b[1]) : juce::MidiMessage (e.b[0]);
            handleOther (m);
        }
    };

    for (int c0 = 0; c0 < n; c0 += chunk)
    {
        const int len = std::min (chunk, n - c0);
        clockBeat = beat0 + bps * c0; clockBps = bps; clockPlaying = transport.playing;
        if (hasEdit) applyEdit();
        if (hasPending && ! xfading) beginSwitch();
        for (int m = 0; m < vault::numMacros; ++m) macroSm[m] += (macroTarget[m].load (std::memory_order_relaxed) - macroSm[m]) * smMacro;
        for (int q = 0; q < vault::numQuick; ++q)
        {
            const float t = quickTarget[q].load (std::memory_order_relaxed);
            quickSm[q] += (t - quickSm[q]) * smMacro;
            if (std::abs (t - quickSm[q]) < 1.0e-5f) quickSm[q] = t;   // lands exactly (neutral = untouched sound)
        }

        // the slots that sound in this chunk: the current one, plus the outgoing one during a crossfade
        Slot* live[2] { &slots[cur], xfading ? &slots[1 - cur] : nullptr };
        const int numLive = xfading ? 2 : 1;
        for (int li = 0; li < numLive; ++li)
        {
            auto& s = *live[li];
            updateParams (s, beat0 + bps * c0, bpm);
            renderLfos (s, len, beat0, bps, c0);
            std::fill_n (s.busL.data(), len, 0.0f); std::fill_n (s.busR.data(), len, 0.0f); std::fill_n (s.busG.data(), len, 0.0f);
            std::fill (s.partL.begin(), s.partL.end(), 0.0f); std::fill (s.partR.begin(), s.partR.end(), 0.0f);
        }

        int pos = 0;
        while (pos < len)
        {
            while (evIt < evs.size() && evs[evIt].sample - c0 <= pos) { eventClock = sampleClock + std::max (0, evs[evIt].sample - c0); dispatch (evs[evIt++]); }
            int next = len;
            if (evIt < evs.size()) next = std::min (len, evs[evIt].sample - c0);
            if (next <= pos) next = pos + 1;
            for (int li = 0; li < numLive; ++li)
            {
                auto& s = *live[li];
                for (int k = 0; k < numParts (s); ++k)
                {
                    auto& part = *s.parts[(size_t) k];
                    float* pl = s.partL.data() + k * chunk + pos;
                    float* prr = s.partR.data() + k * chunk + pos;
                    switch (s.prog->kind[k])
                    {
                        case VaultPartKind::synth:
                            part.synth.render (pl, prr, s.busG.data() + pos, next - pos, s.vp[k], s.lfo1.data() + k * chunk + pos, s.lfo2.data() + k * chunk + pos);
                            break;
                        case VaultPartKind::sample: part.sampler.render (pl, prr, next - pos); break;
                        case VaultPartKind::model:
                            if (auto* mp = poolFor (s, k))
                                for (auto& v : mp->voices) if (v->isActive()) v->render (pl, prr, next - pos);
                            break;
                    }
                }
            }
            pos = next;
        }
        for (int li = 0; li < numLive; ++li)
        {
            auto& s = *live[li];
            const auto& E = *s.eff;
            const auto& pr = *s.prog;
            using T = vault::TargetRef;
            for (int k = 0; k < numParts (s); ++k)
            {
                // layer gain: level (or a macro's) x level modulation, equal-power pan (+ modulation); ramps when it moves
                float lev = pr.level[k], gl, gr;
                if (E.layerAbsSet[k][0]) lev = E.layerAbs[k][0] * (k > 0 ? juce::Decibels::decibelsToGain (pr.values[k][kk::vk::gain] - pr.values[0][kk::vk::gain]) : 1.0f);
                if (nz (E.layerAdd[k][T::lLevel])) lev *= std::max (0.0f, 1.0f + E.layerAdd[k][T::lLevel]);
                if (E.layerAbsSet[k][1] || nz (E.layerAdd[k][T::lPan]))
                {
                    const float pan = juce::jlimit (-1.0f, 1.0f, (E.layerAbsSet[k][1] ? E.layerAbs[k][1] : pr.pan[k]) + E.layerAdd[k][T::lPan]);
                    const float a = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
                    gl = lev * std::cos (a) * juce::MathConstants<float>::sqrt2; gr = lev * std::sin (a) * juce::MathConstants<float>::sqrt2;
                }
                else { gl = lev * pr.panL[k]; gr = lev * pr.panR[k]; }
                float* pl = s.partL.data() + k * chunk; float* prr = s.partR.data() + k * chunk;
                if (pr.kind[k] == VaultPartKind::model && s.parts[(size_t) k]->toneOn)
                {
                    auto& part = *s.parts[(size_t) k];
                    for (int i = 0; i < len; ++i)
                    {
                        part.toneS[0].tick (part.toneC, pl[i]); part.toneS[1].tick (part.toneC, prr[i]);
                        pl[i] = part.toneS[0].lp; prr[i] = part.toneS[1].lp;
                    }
                }
                if (! s.gainsValid || (same (s.gl[k], gl) && same (s.gr[k], gr)))
                    for (int i = 0; i < len; ++i) { s.busL[(size_t) i] += pl[i] * gl; s.busR[(size_t) i] += prr[i] * gr; }
                else
                {
                    const float dl = (gl - s.gl[k]) / (float) len, dr = (gr - s.gr[k]) / (float) len;
                    for (int i = 0; i < len; ++i)
                    {
                        const float a = s.gl[k] + dl * (float) (i + 1), b = s.gr[k] + dr * (float) (i + 1);
                        s.busL[(size_t) i] += pl[i] * a; s.busR[(size_t) i] += prr[i] * b;
                    }
                }
                s.gl[k] = gl; s.gr[k] = gr;
            }
            s.gainsValid = true;
            // tremolo (MAIN page) + volume / pan modulation, ramped over the chunk
            {
                float vol = std::max (0.0f, 1.0f + E.glob[3]);
                if (const float tq = E.quick[vault::qTremolo]; tq > 0.0005f)
                {
                    vol *= 1.0f - tq * 0.5f * (1.0f - std::sin (s.tremPhase));
                    s.tremPhase += kk::twoPi * 4.5f * (float) len / (float) sr;
                    if (s.tremPhase > kk::twoPi) s.tremPhase -= kk::twoPi;
                }
                const float pan = juce::jlimit (-1.0f, 1.0f, E.glob[4]);
                s.volModPrev = s.volMod; s.panModPrev = s.panMod;
                s.volMod = vol; s.panMod = pan;
                if (! same (s.volMod, 1.0f) || ! same (s.volModPrev, 1.0f) || nz (s.panMod) || nz (s.panModPrev))
                    for (int i = 0; i < len; ++i)
                    {
                        const float t = (float) (i + 1) / (float) len;
                        const float v = s.volModPrev + (s.volMod - s.volModPrev) * t, p = s.panModPrev + (s.panMod - s.panModPrev) * t;
                        s.busL[(size_t) i] *= v * std::min (1.0f, 1.0f - p); s.busR[(size_t) i] *= v * std::min (1.0f, 1.0f + p);
                    }
            }
            // gate (before the FX, so reverb / delay tails fill the gaps)
            {
                const float target = gateOn.load (std::memory_order_relaxed) ? E.gate[1] : 0.0f;
                const float maxStep = (float) len / (0.010f * (float) sr);
                const float to = s.gateMix + juce::jlimit (-maxStep, maxStep, target - s.gateMix);
                if (s.gateMix > 0.0f || to > 0.0f)
                {
                    const double gb = transport.playing ? beat0 + bps * c0 : beat0 + bps * c0 - s.trigBeat;
                    s.gate.process (s.busL.data(), s.busR.data(), len, pr.ctl.gate, gb, bps, E.gate[0], s.gateMix, to, E.gate[2],
                                    transport.playing ? transport.loopStart : 0.0, transport.playing ? transport.loopEnd : 0.0);
                }
                s.gateMix = to;
            }
            // the FX chain + output stage
            {
                vault::FxContext fc;
                fc.bpm = bpm; fc.beatPos = beat0 + bps * c0; fc.ghostSend = s.busG.data();
                vault::FxChain::Output out;
                out.outGain = juce::Decibels::decibelsToGain (pr.values[0][kk::vk::gain]);
                out.finish = E.finish;
                out.monoLows = pr.monoLows;
                const int we = pr.firstOfType[(int) vault::FxType::width];
                out.width = we >= 0 ? E.fx[we][0] : 0.5f;
                s.chain->process (s.busL.data(), s.busR.data(), len, fc, fxOn.load (std::memory_order_relaxed), out);
            }
            if (const float thr0 = s.prog->peakThresh; thr0 > 0.0f)
            {
                // the threshold follows the playing velocity (thr * vel^exp, smoothed over ~5 ms), so soft and hard notes lose the
                // same share of their attack peak and the velocity response stays as designed
                const float thrTarget = thr0 * std::pow (juce::jlimit (0.01f, 1.0f, lastVelocity), s.prog->peakVelExp);
                const float thrA = 1.0f - std::exp (-1.0f / (0.005f * (float) sr));
                if (s.peakThrSm <= 0.0f) s.peakThrSm = thrTarget;
                // peak control (vault::peakKey): a 1 ms look-ahead peak compressor, ratio 6 above thr. The detector sees the
                // signal 1 ms before it is played (the preset's output is delayed by 1 ms), peaks are held 15 ms and released
                // over 50 ms, and the gain glides down over the look-ahead (time constant 1/5 of it), so the first transient is
                // already under control when it is played: no overshoot, no onset click.
                const int la = juce::jlimit (1, Slot::peakDelayMax - 1, (int) std::round (0.001 * sr));
                const float atk = 1.0f - std::exp (-5.0f / (float) la), rel = std::exp (-1.0f / (0.050f * (float) sr));
                const int holdN = (int) (0.015 * sr) + la;
                constexpr float slope = 1.0f - 1.0f / 6.0f;
                for (int i = 0; i < len; ++i)
                {
                    const float xl = s.busL[(size_t) i], xr = s.busR[(size_t) i];
                    const float pk = std::max (std::abs (xl), std::abs (xr));
                    if (pk >= s.peakRaw) { s.peakRaw = pk; s.peakHold = holdN; }
                    else if (s.peakHold > 0) --s.peakHold;
                    else s.peakRaw *= rel;
                    if (s.peakRaw > s.peakEnv) s.peakEnv += (s.peakRaw - s.peakEnv) * atk;
                    else s.peakEnv = s.peakRaw;
                    s.peakThrSm += (thrTarget - s.peakThrSm) * thrA;
                    const float thr = s.peakThrSm;
                    const int rd = (s.peakPos - la + Slot::peakDelayMax) % Slot::peakDelayMax;
                    float g = 1.0f;
                    if (s.peakEnv > thr) g = std::exp (-slope * std::log (s.peakEnv / thr));
                    s.busL[(size_t) i] = s.peakDl[0][(size_t) rd] * g; s.busR[(size_t) i] = s.peakDl[1][(size_t) rd] * g;
                    s.peakDl[0][(size_t) s.peakPos] = xl; s.peakDl[1][(size_t) s.peakPos] = xr;
                    s.peakPos = (s.peakPos + 1) % Slot::peakDelayMax;
                }
            }
        }

        const auto& sc = slots[cur];
        const auto& so = slots[1 - cur];
        const float gainCur = sc.prog->gain, gainOld = so.prog->gain;
        const float volTarget = volumeTarget.load (std::memory_order_relaxed);
        const float qp = (sc.eff != nullptr ? sc.eff->quick[vault::qPan] : 0.5f);
        const float panTL = same (qp, 0.5f) ? 1.0f : std::min (1.0f, 2.0f * (1.0f - qp)), panTR = same (qp, 0.5f) ? 1.0f : std::min (1.0f, 2.0f * qp);
        for (int i = 0; i < len; ++i)
        {
            float inL, inR;
            if (xfading)
            {
                // equal-power crossfade: the new sound in on sin, the old one (with its own FX tails) out on cos
                xfPos = std::min (1.0f, xfPos + xfStep);
                const float gn = std::sin (halfPi * xfPos) * gainCur, go = std::cos (halfPi * xfPos) * gainOld;
                inL = sc.busL[(size_t) i] * gn + so.busL[(size_t) i] * go;
                inR = sc.busR[(size_t) i] * gn + so.busR[(size_t) i] * go;
            }
            else { inL = sc.busL[(size_t) i] * gainCur; inR = sc.busR[(size_t) i] * gainCur; }
            // mono below 120 Hz: the side signal is high-passed (48 dB/oct); DC blocker on the mid
            const float mIn = (inL + inR) * 0.5f;
            const float m = mIn - dcX + dcA * dcY;
            dcX = mIn; dcY = m;
            float s = (inL - inR) * 0.5f;
            s = sideHp[3].tick (sideHp[2].tick (sideHp[1].tick (sideHp[0].tick (s))));
            volumeSm += (volTarget - volumeSm) * smVol;
            float l = (m + s) * volumeSm, r = (m - s) * volumeSm;
            // output balance (MAIN page pan)
            panSmL += (panTL - panSmL) * smVol; panSmR += (panTR - panSmR) * smVol;
            if (std::abs (panSmL - panTL) < 1.0e-6f) panSmL = panTL;
            if (std::abs (panSmR - panTR) < 1.0e-6f) panSmR = panTR;
            l *= panSmL; r *= panSmR;
            // safety limiter (instant attack, 120 ms release) + soft ceiling
            const float pk = std::max (std::abs (l), std::abs (r));
            limEnv = pk > limEnv ? pk : limEnv * limRel + pk * (1.0f - limRel);
            if (limEnv > limThresh) { const float lg = limThresh / limEnv; l *= lg; r *= lg; }
            auto soft = [] (float x) { const float a = std::abs (x); return a < 0.9f ? x : std::copysign (0.9f + 0.08f * std::tanh ((a - 0.9f) / 0.08f), x); };
            l = soft (l); r = soft (r);
            if (! std::isfinite (l) || ! std::isfinite (r)) { l = r = 0; limEnv = 0; dcX = dcY = 0; for (auto& h : sideHp) h.z1 = h.z2 = 0; }
            if (outR != nullptr) { outL[c0 + i] = l; outR[c0 + i] = r; }
            else outL[c0 + i] = 0.5f * (l + r);
        }
        if (xfading && xfPos >= 1.0f)
        {
            // crossfade done: the outgoing sound is at zero gain - silence it (voices off, FX tails cleared now, not in the
            // switch block) so only one slot renders until the next switch
            xfading = false;
            resetSlot (slots[1 - cur], true);
            slotRelease (slots[1 - cur]);
        }
        sampleClock += len;
    }
    while (evIt < evs.size()) dispatch (evs[evIt++]);
    for (int c = 2; c < nCh; ++c) buffer.clear (c, 0, n);
    {
        // the on-screen keyboard: what the voices play now (arp output while it runs) + every note started in this block
        uint64_t now[2] {}, started[2] {};
        for (int k = 0; k < 128; ++k) if (held[(size_t) k] > 0.0f) now[k >> 6] |= uint64_t (1) << (k & 63);
        for (const auto& e : evs) if (e.type == 0 && juce::isPositiveAndBelow (e.note, 128)) started[e.note >> 6] |= uint64_t (1) << (e.note & 63);
        for (int w = 0; w < 2; ++w)
        {
            soundingUi[w].store (now[w], std::memory_order_relaxed);
            if (started[w] != 0) startedUi[w].fetch_or (started[w], std::memory_order_relaxed);
        }
    }
}

juce::AudioBuffer<float> VaultEngine::render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate)
{
    return render (preset, midiNote, velocity, seconds, sampleRate, nullptr);
}

juce::AudioBuffer<float> VaultEngine::render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate,
                                              const float* macros)
{
    const int total = std::max (1, (int) std::round (seconds * sampleRate));
    juce::MidiBuffer midi;
    const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::round (velocity * 127.0f));
    midi.addEvent (juce::MidiMessage::noteOn (1, juce::jlimit (0, 127, midiNote), vel), 0);
    midi.addEvent (juce::MidiMessage::noteOff (1, juce::jlimit (0, 127, midiNote)), std::max (1, (int) (total * 0.7)));
    return renderMidi (preset, midi, total, sampleRate, macros);
}

juce::AudioBuffer<float> VaultEngine::renderMidi (const VaultPreset& preset, const juce::MidiBuffer& midi, int numSamples, double sampleRate,
                                                  const float* macros)
{
    return renderMidi (preset, midi, numSamples, sampleRate, macros, nullptr);
}

juce::AudioBuffer<float> VaultEngine::renderMidi (const VaultPreset& preset, const juce::MidiBuffer& midi, int numSamples, double sampleRate,
                                                  const float* macros, std::shared_ptr<vault::SampleLibrary> library)
{
    juce::AudioBuffer<float> out (2, std::max (1, numSamples));
    out.clear();
    const auto compiled = compile (preset, std::move (library));
    auto eng = std::make_unique<VaultEngine>();
    for (int m = 0; m < vault::numMacros; ++m) eng->setMacro (m, macros != nullptr ? macros[m] : compiled.macroDefaults[m]);
    eng->setArpOn (compiled.ctl.arp.on);
    eng->setGateOn (compiled.ctl.gate.on);
    constexpr int block = 512;
    eng->prepare (sampleRate, block, std::max (1, compiled.numParts), false);   // one sound, no switches: no crossfade slot
    eng->setPreset (compiled, true);
    // offline: everything the preset needs is ready before the first sample (instruments decoded, model voices made)
    for (int k = 0; k < compiled.numParts; ++k)
        if (auto* h = compiled.parts[(size_t) k].instrument; h != nullptr && compiled.library != nullptr)
            compiled.library->loadNow (h);
    eng->serviceResources();
    juce::AudioBuffer<float> buf (2, block);
    for (int s0 = 0; s0 < numSamples; s0 += block)
    {
        const int len = std::min (block, numSamples - s0);
        juce::MidiBuffer slice;
        for (const auto meta : midi)
            if (meta.samplePosition >= s0 && meta.samplePosition < s0 + len)
                slice.addEvent (meta.getMessage(), meta.samplePosition - s0);
        buf.setSize (2, len, false, false, true);
        eng->process (buf, slice);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, s0, buf, c, 0, len);
    }
    return out;
}

float VaultEngine::measureLoudnessDb (const juce::AudioBuffer<float>& b, double sampleRate)
{
    const int n = b.getNumSamples(), nCh = std::max (1, b.getNumChannels());
    const int win = std::max (1, (int) (0.4 * sampleRate)), hop = std::max (1, win / 4);
    double best = 0;
    for (int s0 = 0; s0 < std::max (1, n - win + 1); s0 += hop)
    {
        double sum = 0;
        const int len = std::min (win, n - s0);
        for (int c = 0; c < b.getNumChannels(); ++c)
        {
            const float* x = b.getReadPointer (c, s0);
            for (int i = 0; i < len; ++i) sum += (double) x[i] * x[i];
        }
        best = std::max (best, sum / (double) std::max (1, len * nCh));
    }
    return (float) (10.0 * std::log10 (std::max (1.0e-12, best)));
}
