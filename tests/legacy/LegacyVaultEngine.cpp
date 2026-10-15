#include "LegacyVaultEngine.h"

namespace
{
inline float lowHalf (float m) { return std::min (1.0f, 2.0f * m); }          // 0 .. 0.5 -> 0 .. 1 (scales the designed amount)
inline float upHalf (float m)  { return std::max (0.0f, 2.0f * m - 1.0f); }   // 0.5 .. 1 -> 0 .. 1 (adds on top)
constexpr float limThresh = 0.89f;   // about -1 dBFS
}

// the voices of one model for one part (created off the audio thread by serviceResources, then reused)
struct LegacyVaultEngine::ModelPool
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

struct LegacyVaultEngine::Part
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
};

LegacyVaultEngine::LegacyVaultEngine()
{
    for (auto& m : macroTarget) m.store (0.5f);
    for (auto& sl : slots)
    {
        sl.prog = std::make_unique<Program>();
        for (int k = 0; k < vault::maxParts; ++k) kk::vk::fillDefaults (sl.prog->values[k]);
        sl.prog->numParts = 0;
    }
    pending = std::make_unique<Program>();
}

LegacyVaultEngine::~LegacyVaultEngine()
{
    for (auto& sl : slots) slotRelease (sl);
}

LegacyCompiledPreset LegacyVaultEngine::compile (const VaultPreset& preset, std::shared_ptr<vault::SampleLibrary> library)
{
    LegacyCompiledPreset c;
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
    return c;
}

void LegacyVaultEngine::prepare (double sampleRate, int maxBlockSize, int numPartsToPrepare, bool crossfadeSlot)
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
            sl.fx = std::make_unique<kkv2::FxRack>();
            sl.fx->prepare (sr, chunk);
            sl.busL.assign (chunk, 0.0f); sl.busR.assign (chunk, 0.0f); sl.busG.assign (chunk, 0.0f);
            sl.partL.assign ((size_t) (vault::maxParts * chunk), 0.0f); sl.partR.assign ((size_t) (vault::maxParts * chunk), 0.0f);
            sl.lfo1.assign ((size_t) (vault::maxParts * chunk), 0.0f); sl.lfo2.assign ((size_t) (vault::maxParts * chunk), 0.0f);
        }
        else
        {
            sl.fx.reset();
            for (auto* v : { &sl.busL, &sl.busR, &sl.busG, &sl.partL, &sl.partR, &sl.lfo1, &sl.lfo2 }) { v->clear(); v->shrink_to_fit(); }
        }
        for (auto& v : sl.vp) v = {};
        sl.fp = {};
    }
    if (cur != 0) { std::swap (slots[0].prog, slots[1].prog); std::swap (slots[0].held, slots[1].held); cur = 0; }   // the current program lives in the prepared slot 0
    slotRelease (slots[1]);                   // no crossfade running: only slot 0 holds instruments
    slotRelease (slots[0]);
    slotAcquire (slots[0]);

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
    held.fill (0.0f); pitchWheel = modWheel = aftertouch = 0; sustain = false; freeBeat = 0; lastBpm = 120.0; lastVelocity = 0.8f;
    for (int m = 0; m < vault::numMacros; ++m) macroSm[m] = macroTarget[m].load();
    volumeSm = volumeTarget.load();
}

void LegacyVaultEngine::releaseResources()
{
    const std::lock_guard<std::mutex> g (resourceLock);
    for (auto& sl : slots)
    {
        slotRelease (sl);
        for (auto& p : sl.parts) p.reset();
        sl.fx.reset();
        sl.prepared = false;
    }
    preparedParts = 0;
    xfading = false;
}

void LegacyVaultEngine::loadProgram (Program& dst, const LegacyCompiledPreset& p)
{
    dst.numParts = juce::jlimit (0, (int) vault::maxParts, p.numParts);
    for (int k = 0; k < vault::maxParts; ++k)
    {
        kk::vk::fillDefaults (dst.values[k]);
        if (k >= dst.numParts)
        {
            dst.level[k] = 0; dst.panL[k] = dst.panR[k] = 0; dst.transpose[k] = 0;
            dst.kind[k] = VaultPartKind::synth; dst.instrument[k] = nullptr; dst.model[k] = -1; dst.numModelParams[k] = 0;
            continue;
        }
        const auto& part = p.parts[(size_t) k];
        dst.kind[k] = part.kind;
        if (part.kind != VaultPartKind::synth)
        {
            // FX defaults for a sample / model part 0: neutral (no reverb, no MASTER colour) unless the preset sets them
            // (the EVOLVE macros m2 / m5 default to a little reverb, delay and chorus: off as well)
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
}

void LegacyVaultEngine::setPreset (const LegacyCompiledPreset& p, bool instant)
{
    loadProgram (*pending, p);
    hasPending = true;
    if (instant || preparedParts == 0 || ! haveXfadeSlot) switchInstant();
    // otherwise process() starts the crossfade at its next chunk (or when the running crossfade has finished)
}

void LegacyVaultEngine::resetSlot (Slot& s, bool clearFx)
{
    // the slot is silent (unused, or its crossfade just ended): voices off, LFOs restarted, FX rack cleared (no allocation).
    // Clearing the FX matters: the next sound must not start inside the old reverb / delay content.
    for (int k = 0; k < vault::maxParts; ++k)
        if (auto& p = s.parts[(size_t) k])
        {
            p->synth.allOff (true); p->synth.setSustain (false); p->synth.setSustain (sustain); p->ph1 = p->ph2 = 0;
            p->sampler.allOff (true); p->sampler.setSustain (sustain);
            for (int m = 0; m < p->numPools; ++m) if (auto* mp = p->pools[(size_t) m].load()) mp->allOff (true);
            for (auto& t : p->toneS) t.reset();
        }
    if (clearFx && s.fx != nullptr) s.fx->clearState();
    s.peakEnv = 0; s.peakRaw = 0; s.peakThrSm = 0; s.peakHold = 0; s.peakPos = 0;
    for (auto& d : s.peakDl) d.fill (0.0f);
}

void LegacyVaultEngine::switchInstant()
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
    updateParams (s, freeBeat, lastBpm);
    for (int n = 0; n < 128; ++n) if (held[(size_t) n] > 0.0f) noteOnAll (s, n, held[(size_t) n]);   // keys still down play the new sound
}

void LegacyVaultEngine::beginSwitch()
{
    // the free slot (silent, FX cleared when its last crossfade ended) takes the new sound; the current one becomes the
    // outgoing sound and keeps playing - voices, LFOs and its own reverb / delay / chorus tails - while it fades out
    const int next = 1 - cur;
    auto& s = slots[next];
    resetSlot (s, false);
    slotRelease (s);
    std::swap (s.prog, pending);
    hasPending = false;
    slotAcquire (s);
    cur = next;
    xfading = true;
    xfPos = 0.0f;
    updateParams (s, freeBeat, lastBpm);
    for (int n = 0; n < 128; ++n) if (held[(size_t) n] > 0.0f) noteOnAll (s, n, held[(size_t) n]);   // keys still down: new sound
}

void LegacyVaultEngine::setMacro (int index, float value01)
{
    if (juce::isPositiveAndBelow (index, (int) vault::numMacros))
        macroTarget[index].store (std::isfinite (value01) ? juce::jlimit (0.0f, 1.0f, value01) : 0.5f);
}

float LegacyVaultEngine::getMacro (int index) const
{
    return juce::isPositiveAndBelow (index, (int) vault::numMacros) ? macroTarget[index].load() : 0.5f;
}

void LegacyVaultEngine::setVolume (float g) { volumeTarget.store (std::isfinite (g) ? juce::jlimit (0.0f, 2.0f, g) : 1.0f); }

void LegacyVaultEngine::noteOnAll (Slot& s, int note, float vel)
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

void LegacyVaultEngine::noteOffAll (Slot& s, int note)
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

LegacyVaultEngine::ModelPool* LegacyVaultEngine::poolFor (Slot& s, int k) const
{
    const int m = s.prog->model[k];
    auto& part = s.parts[(size_t) k];
    if (m < 0 || part == nullptr || m >= part->numPools) return nullptr;
    return part->pools[(size_t) m].load (std::memory_order_acquire);
}

void LegacyVaultEngine::slotAcquire (Slot& s)
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

void LegacyVaultEngine::slotRelease (Slot& s)
{
    for (auto& h : s.held) if (h != nullptr) { h->release(); h = nullptr; }
}

void LegacyVaultEngine::serviceResources()
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

void LegacyVaultEngine::allNotesOff (bool hard)
{
    held.fill (0.0f);
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

void LegacyVaultEngine::handleMidi (const juce::MidiMessage& m)
{
    auto& s = slots[cur];
    auto& old = slots[1 - cur];
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        held[(size_t) n] = std::max (0.01f, m.getFloatVelocity());
        lastVelocity = held[(size_t) n];
        noteOnAll (s, n, held[(size_t) n]);            // new notes only on the current sound
    }
    else if (m.isNoteOff())
    {
        held[(size_t) m.getNoteNumber()] = 0.0f;
        noteOffAll (s, m.getNoteNumber());
        if (xfading) noteOffAll (old, m.getNoteNumber());
    }
    else if (m.isSustainPedalOn() || m.isSustainPedalOff())
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

void LegacyVaultEngine::updateParams (Slot& s, double beatPos, double bpm)
{
    const auto& pr = *s.prog;
    auto& fp = s.fp;
    kk::VoiceBuildContext ctx;
    ctx.pitchWheel = juce::jlimit (-1.0f, 1.0f, pitchWheel);
    ctx.modWheel = modWheel; ctx.aftertouch = aftertouch;
    const float tone = macroSm[vault::macroTone], space = macroSm[vault::macroSpace];
    const float drv = macroSm[vault::macroDrive], mot = macroSm[vault::macroMotion];
    const float toneD = 2.0f * tone - 1.0f;
    bool anyBass = false;
    for (int k = 0; k < pr.numParts; ++k)
    {
        const float* vals = pr.values[k];
        kk::FxParams dummy;
        kk::buildVoiceParamsFrom ([vals] (int key) { return vals[key]; }, ctx, s.vp[k], k == 0 ? fp : dummy);
        anyBass |= vals[kk::vk::bassMode] > 0.5f;
        auto& v = s.vp[k];
        // TONE: filter tilt (+-1.5 octaves of cutoff)
        v.cutoff = juce::jlimit (40.0f, 20000.0f, v.cutoff * std::exp2 (toneD * 1.5f));
        // MOTION: LFO depth (designed amount below 0.5, extra filter / pitch movement and drift above)
        v.lfoFilter = kk::clamp01 (v.lfoFilter * lowHalf (mot) + upHalf (mot) * 0.35f);
        v.lfoPitch = kk::clamp01 (v.lfoPitch * lowHalf (mot) + upHalf (mot) * 0.04f);
        v.lfoAmp *= lowHalf (mot);
        v.drift = kk::clamp01 (v.drift + upHalf (mot) * 0.3f);
    }
    // sample / model parts: TONE = cutoff tilt, MOTION = vibrato (+ the FX chorus below), the pitch wheel
    bool waiting = false;
    for (int k = 0; k < numParts (s); ++k)
    {
        auto& part = *s.parts[(size_t) k];
        if (pr.kind[k] == VaultPartKind::sample)
        {
            const auto* h = pr.instrument[k];
            const auto* d = h != nullptr ? h->data() : nullptr;
            waiting |= h != nullptr && d == nullptr && ! h->hasFailed();
            part.sampler.setSource (d, pr.articulation[k]);
            part.sampler.setParams (pr.sp[k]);
            const float vib = pr.sp[k][vault::spVibrato] * lowHalf (mot) + upHalf (mot) * 14.0f;
            part.sampler.setControl (std::exp2 (toneD * 1.5f), vib, ctx.pitchWheel);
        }
        else if (pr.kind[k] == VaultPartKind::model)
        {
            auto* mp = poolFor (s, k);
            if (mp == nullptr && pr.model[k] >= 0) { waiting = true; part.modelWanted.store (pr.model[k]); }
            const float cut = juce::jlimit (40.0f, 20000.0f, 20000.0f * std::exp2 (toneD * 1.5f));
            part.toneOn = cut < 19000.0f;
            if (part.toneOn) part.toneC.set (cut, 1.414f, (float) sr);
            if (mp != nullptr)
            {
                const float vibC = upHalf (mot) * 14.0f;
                const float vib = vibC * std::sin (part.vibPhase) * 0.01f;
                part.vibPhase += juce::MathConstants<float>::twoPi * 5.2f * (float) chunk / (float) sr;
                if (part.vibPhase > juce::MathConstants<float>::twoPi) part.vibPhase -= juce::MathConstants<float>::twoPi;
                const float bend = ctx.pitchWheel * pr.values[k][kk::vk::bendRange] + vib;
                for (size_t i = 0; i < mp->voices.size(); ++i)
                    if (mp->voices[i]->isActive() && mp->note[i] >= 0) mp->voices[i]->setPitch (kk::semisToHz ((float) mp->note[i] + bend));
            }
        }
    }
    if (&s == &slots[cur]) loading.store (waiting, std::memory_order_relaxed);
    if (pr.numParts == 0) fp = {};
    fp.cleanLow = fp.cleanLow || anyBass;
    // TONE on the master: high / low shelf tilt
    fp.eqHigh = juce::jlimit (-12.0f, 12.0f, fp.eqHigh + toneD * 5.0f);
    fp.eqLow = juce::jlimit (-12.0f, 12.0f, fp.eqLow - toneD * 2.5f);
    // SPACE: reverb + delay amount and stereo width
    fp.revMix = kk::clamp01 (fp.revMix * lowHalf (space) + upHalf (space) * 0.45f);
    fp.delayMix = kk::clamp01 (fp.delayMix * lowHalf (space) + upHalf (space) * 0.25f);
    fp.revSize = kk::clamp01 (fp.revSize + upHalf (space) * 0.3f);
    fp.width = kk::clamp01 (fp.width * (0.5f + 0.5f * lowHalf (space)) + upHalf (space) * 0.4f);
    // DRIVE: saturation
    fp.drive = kk::clamp01 (fp.drive * lowHalf (drv) + upHalf (drv) * 0.7f);
    // MOTION on the FX: chorus / phaser / flanger
    fp.chorus = kk::clamp01 (fp.chorus * lowHalf (mot) + upHalf (mot) * 0.5f);
    fp.phaser *= lowHalf (mot); fp.flanger *= lowHalf (mot);
    fp.bpm = bpm; fp.beatPos = beatPos;
}

void LegacyVaultEngine::renderLfos (Slot& s, int len, double beat0, double bps, int c0)
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

void LegacyVaultEngine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Transport& transport)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nCh = buffer.getNumChannels();
    if (n <= 0 || nCh == 0) return;
    if (preparedParts == 0 || slots[0].fx == nullptr) { buffer.clear(); return; }

    const double bpm = juce::jlimit (20.0, 999.0, transport.bpm > 0 ? transport.bpm : 120.0);
    lastBpm = bpm;
    const double bps = bpm / 60.0 / sr;
    const double beat0 = transport.playing ? transport.ppq : freeBeat;
    freeBeat = beat0 + bps * n;
    auto* outL = buffer.getWritePointer (0);
    auto* outR = nCh > 1 ? buffer.getWritePointer (1) : nullptr;
    auto midiIt = midi.cbegin();
    const float smMacro = 1.0f - std::exp (-(float) chunk / (0.03f * (float) sr));
    const float smVol = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));
    constexpr float halfPi = juce::MathConstants<float>::halfPi;

    for (int c0 = 0; c0 < n; c0 += chunk)
    {
        const int len = std::min (chunk, n - c0);
        if (hasPending && ! xfading) beginSwitch();
        for (int m = 0; m < vault::numMacros; ++m) macroSm[m] += (macroTarget[m].load (std::memory_order_relaxed) - macroSm[m]) * smMacro;

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
            while (midiIt != midi.cend() && (*midiIt).samplePosition - c0 <= pos)
            {
                handleMidi ((*midiIt).getMessage());
                ++midiIt;
            }
            int next = len;
            if (midiIt != midi.cend()) next = std::min (len, (*midiIt).samplePosition - c0);
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
            for (int k = 0; k < numParts (s); ++k)
            {
                const float gl = s.prog->level[k] * s.prog->panL[k], gr = s.prog->level[k] * s.prog->panR[k];
                float* pl = s.partL.data() + k * chunk; float* prr = s.partR.data() + k * chunk;
                if (s.prog->kind[k] == VaultPartKind::model && s.parts[(size_t) k]->toneOn)
                {
                    auto& part = *s.parts[(size_t) k];
                    for (int i = 0; i < len; ++i)
                    {
                        part.toneS[0].tick (part.toneC, pl[i]); part.toneS[1].tick (part.toneC, prr[i]);
                        pl[i] = part.toneS[0].lp; prr[i] = part.toneS[1].lp;
                    }
                }
                for (int i = 0; i < len; ++i) { s.busL[(size_t) i] += pl[i] * gl; s.busR[(size_t) i] += prr[i] * gr; }
            }
            s.fx->process (s.busL.data(), s.busR.data(), s.busG.data(), len, s.fp);
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
    }
    for (int c = 2; c < nCh; ++c) buffer.clear (c, 0, n);
}

juce::AudioBuffer<float> LegacyVaultEngine::render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate)
{
    return render (preset, midiNote, velocity, seconds, sampleRate, nullptr);
}

juce::AudioBuffer<float> LegacyVaultEngine::render (const VaultPreset& preset, int midiNote, float velocity, double seconds, double sampleRate,
                                              const float* macros)
{
    const int total = std::max (1, (int) std::round (seconds * sampleRate));
    juce::MidiBuffer midi;
    const auto vel = (juce::uint8) juce::jlimit (1, 127, (int) std::round (velocity * 127.0f));
    midi.addEvent (juce::MidiMessage::noteOn (1, juce::jlimit (0, 127, midiNote), vel), 0);
    midi.addEvent (juce::MidiMessage::noteOff (1, juce::jlimit (0, 127, midiNote)), std::max (1, (int) (total * 0.7)));
    return renderMidi (preset, midi, total, sampleRate, macros);
}

juce::AudioBuffer<float> LegacyVaultEngine::renderMidi (const VaultPreset& preset, const juce::MidiBuffer& midi, int numSamples, double sampleRate,
                                                  const float* macros)
{
    return renderMidi (preset, midi, numSamples, sampleRate, macros, nullptr);
}

juce::AudioBuffer<float> LegacyVaultEngine::renderMidi (const VaultPreset& preset, const juce::MidiBuffer& midi, int numSamples, double sampleRate,
                                                  const float* macros, std::shared_ptr<vault::SampleLibrary> library)
{
    juce::AudioBuffer<float> out (2, std::max (1, numSamples));
    out.clear();
    const auto compiled = compile (preset, std::move (library));
    auto eng = std::make_unique<LegacyVaultEngine>();
    for (int m = 0; m < vault::numMacros; ++m) eng->setMacro (m, macros != nullptr ? macros[m] : compiled.macroDefaults[m]);
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

float LegacyVaultEngine::measureLoudnessDb (const juce::AudioBuffer<float>& b, double sampleRate)
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
