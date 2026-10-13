#include "VaultEngine.h"

namespace
{
inline float lowHalf (float m) { return std::min (1.0f, 2.0f * m); }          // 0 .. 0.5 -> 0 .. 1 (scales the designed amount)
inline float upHalf (float m)  { return std::max (0.0f, 2.0f * m - 1.0f); }   // 0.5 .. 1 -> 0 .. 1 (adds on top)
constexpr float fadeSeconds = 0.010f;
constexpr float limThresh = 0.89f;   // about -1 dBFS
}

struct VaultEngine::Part
{
    kk::SynthEngine synth;
    float ph1 = 0, ph2 = 0, last1 = 0, last2 = 0, sh1 = 0, sh2 = 0;
    kk::Rng rng;
};

VaultEngine::VaultEngine()
{
    for (auto& m : macroTarget) m.store (0.5f);
    active = std::make_unique<Program>();
    pending = std::make_unique<Program>();
    for (int k = 0; k < vault::maxParts; ++k) kk::vk::fillDefaults (active->values[k]);
    active->numParts = 0;
}

VaultEngine::~VaultEngine() = default;

VaultCompiledPreset VaultEngine::compile (const VaultPreset& preset)
{
    VaultCompiledPreset c;
    c.numParts = juce::jlimit (0, (int) vault::maxParts, (int) preset.parts.size());
    for (int k = 0; k < c.numParts; ++k)
    {
        const auto& src = preset.parts[(size_t) k];
        auto& dst = c.parts[(size_t) k];
        dst.kv.clear();
        dst.kv.reserve (src.values.size());
        for (auto& [id, v] : src.values)
            if (const int key = kk::vk::keyOf (id); key >= 0)
                dst.kv.push_back ({ key, kk::vk::sanitize (key, v) });
        dst.level = std::isfinite (src.level) ? juce::jlimit (0.0f, 2.0f, src.level) : 1.0f;
        dst.pan = std::isfinite (src.pan) ? juce::jlimit (-1.0f, 1.0f, src.pan) : 0.0f;
        dst.transpose = juce::jlimit (-3, 3, src.octave) * 12 + juce::jlimit (-12, 12, src.semi);
    }
    c.gain = std::isfinite (preset.gain) ? juce::jlimit (0.0f, 16.0f, preset.gain) : 1.0f;
    for (int m = 0; m < vault::numMacros; ++m)
        c.macroDefaults[m] = std::isfinite (preset.macroDefaults[m]) ? juce::jlimit (0.0f, 1.0f, preset.macroDefaults[m]) : 0.5f;
    return c;
}

void VaultEngine::prepare (double sampleRate, int maxBlockSize, int numPartsToPrepare)
{
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    maxBlock = std::max (1, maxBlockSize);
    preparedParts = juce::jlimit (1, (int) vault::maxParts, numPartsToPrepare);
    kk::WavetableBank::get();
    for (int k = 0; k < vault::maxParts; ++k)
    {
        if (k < preparedParts)
        {
            if (! parts[(size_t) k]) parts[(size_t) k] = std::make_unique<Part>();
            parts[(size_t) k]->synth.prepare ((float) sr);
            parts[(size_t) k]->rng.seed (101u + (uint32_t) k * 7u);
        }
        else parts[(size_t) k].reset();
    }
    fx = std::make_unique<kk::FxRack>();
    fx->prepare (sr, chunk);
    busL.assign (chunk, 0.0f); busR.assign (chunk, 0.0f); busG.assign (chunk, 0.0f);
    partL.assign ((size_t) (vault::maxParts * chunk), 0.0f); partR.assign ((size_t) (vault::maxParts * chunk), 0.0f);
    lfo1.assign ((size_t) (vault::maxParts * chunk), 0.0f); lfo2.assign ((size_t) (vault::maxParts * chunk), 0.0f);

    {   // 6th-order Butterworth high-pass at 150 Hz on the side signal: -12 dB at 120 Hz, -21 dB at 100 Hz, -36 dB at 75 Hz
        static const double qs[3] { 0.51763809, 0.70710678, 1.93185165 };
        const double w = juce::MathConstants<double>::twoPi * sideHighPassHz / sr, cs = std::cos (w);
        for (int i = 0; i < 3; ++i)
        {
            auto& h = sideHp[i];
            const double al = std::sin (w) / (2.0 * qs[i]), a0 = 1.0 + al;
            h.b0 = (float) ((1 + cs) / 2 / a0); h.b1 = (float) (-(1 + cs) / a0); h.b2 = (float) ((1 + cs) / 2 / a0);
            h.a1 = (float) (-2 * cs / a0); h.a2 = (float) ((1 - al) / a0); h.z1 = h.z2 = 0;
        }
    }
    limEnv = 0; limRel = (float) std::exp (-1.0 / (0.12 * sr));
    fade = Fade::none; fadeGain = 1.0f; fadeStep = (float) (1.0 / (fadeSeconds * sr));
    held.fill (0.0f); pitchWheel = modWheel = aftertouch = 0; sustain = false; freeBeat = 0;
    for (int m = 0; m < vault::numMacros; ++m) macroSm[m] = macroTarget[m].load();
    volumeSm = volumeTarget.load(); gainSm = active->gain;
    for (int k = 0; k < vault::maxParts; ++k) { vp[k] = {}; }
}

void VaultEngine::releaseResources()
{
    for (auto& p : parts) p.reset();
    fx.reset();
    preparedParts = 0;
}

void VaultEngine::loadProgram (Program& dst, const VaultCompiledPreset& p)
{
    dst.numParts = juce::jlimit (0, (int) vault::maxParts, p.numParts);
    for (int k = 0; k < vault::maxParts; ++k)
    {
        kk::vk::fillDefaults (dst.values[k]);
        if (k >= dst.numParts) { dst.level[k] = 0; dst.panL[k] = dst.panR[k] = 0; dst.transpose[k] = 0; continue; }
        const auto& part = p.parts[(size_t) k];
        for (auto& [key, v] : part.kv) dst.values[k][key] = v;
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
}

void VaultEngine::setPreset (const VaultCompiledPreset& p, bool instant)
{
    loadProgram (*pending, p);
    hasPending = true;
    if (instant || preparedParts == 0) { switchToPending(); fade = Fade::none; fadeGain = 1.0f; gainSm = active->gain; }
    else if (fade != Fade::out) fade = Fade::out;
}

void VaultEngine::switchToPending()
{
    if (! hasPending) return;
    std::swap (active, pending);
    hasPending = false;
    for (auto& p : parts) if (p) { p->synth.allOff (true); p->synth.setSustain (false); p->synth.setSustain (sustain); }
    gainSm = active->gain;
    for (auto& p : parts) if (p) { p->ph1 = p->ph2 = 0; }
    updateParams (freeBeat, 120.0);
    for (int n = 0; n < 128; ++n) if (held[(size_t) n] > 0.0f) noteOnAll (n, held[(size_t) n]);   // keys still down play the new sound
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

void VaultEngine::setVolume (float g) { volumeTarget.store (std::isfinite (g) ? juce::jlimit (0.0f, 2.0f, g) : 1.0f); }

void VaultEngine::noteOnAll (int note, float vel)
{
    for (int k = 0; k < std::min (active->numParts, preparedParts); ++k)
        parts[(size_t) k]->synth.noteOn (juce::jlimit (0, 127, note + active->transpose[k]), vel, vp[k]);
}

void VaultEngine::noteOffAll (int note)
{
    for (int k = 0; k < std::min (active->numParts, preparedParts); ++k)
        parts[(size_t) k]->synth.noteOff (juce::jlimit (0, 127, note + active->transpose[k]), vp[k]);
}

void VaultEngine::allNotesOff (bool hard)
{
    held.fill (0.0f);
    for (auto& p : parts) if (p) { p->synth.setSustain (false); p->synth.allOff (hard); }
    sustain = false;
}

void VaultEngine::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())
    {
        const int n = m.getNoteNumber();
        held[(size_t) n] = std::max (0.01f, m.getFloatVelocity());
        if (fade != Fade::out || fadeGain > 0.0f) noteOnAll (n, held[(size_t) n]);   // during a switch the new sound strikes it
    }
    else if (m.isNoteOff())       { held[(size_t) m.getNoteNumber()] = 0.0f; noteOffAll (m.getNoteNumber()); }
    else if (m.isSustainPedalOn())  { sustain = true;  for (auto& p : parts) if (p) p->synth.setSustain (true); }
    else if (m.isSustainPedalOff()) { sustain = false; for (auto& p : parts) if (p) p->synth.setSustain (false); }
    else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff (m.isAllSoundOff());
    else if (m.isPitchWheel())      pitchWheel = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isChannelPressure()) aftertouch = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAftertouch())      { for (int k = 0; k < std::min (active->numParts, preparedParts); ++k)
                                          parts[(size_t) k]->synth.polyAftertouch (juce::jlimit (0, 127, m.getNoteNumber() + active->transpose[k]),
                                                                                   (float) m.getAfterTouchValue() / 127.0f); }
    else if (m.isController() && m.getControllerNumber() == 1) modWheel = (float) m.getControllerValue() / 127.0f;
}

void VaultEngine::updateParams (double beatPos, double bpm)
{
    const auto& pr = *active;
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
        kk::buildVoiceParamsFrom ([vals] (int key) { return vals[key]; }, ctx, vp[k], k == 0 ? fp : dummy);
        anyBass |= vals[kk::vk::bassMode] > 0.5f;
        auto& v = vp[k];
        // TONE: filter tilt (+-1.5 octaves of cutoff)
        v.cutoff = juce::jlimit (40.0f, 20000.0f, v.cutoff * std::exp2 (toneD * 1.5f));
        // MOTION: LFO depth (designed amount below 0.5, extra filter / pitch movement and drift above)
        v.lfoFilter = kk::clamp01 (v.lfoFilter * lowHalf (mot) + upHalf (mot) * 0.35f);
        v.lfoPitch = kk::clamp01 (v.lfoPitch * lowHalf (mot) + upHalf (mot) * 0.04f);
        v.lfoAmp *= lowHalf (mot);
        v.drift = kk::clamp01 (v.drift + upHalf (mot) * 0.3f);
    }
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

void VaultEngine::process (juce::AudioBuffer<float>& buffer, const juce::MidiBuffer& midi, const Transport& transport)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    const int nCh = buffer.getNumChannels();
    if (n <= 0 || nCh == 0) return;
    if (preparedParts == 0 || fx == nullptr) { buffer.clear(); return; }

    const double bpm = juce::jlimit (20.0, 999.0, transport.bpm > 0 ? transport.bpm : 120.0);
    const double bps = bpm / 60.0 / sr;
    const double beat0 = transport.playing ? transport.ppq : freeBeat;
    freeBeat = beat0 + bps * n;
    auto* outL = buffer.getWritePointer (0);
    auto* outR = nCh > 1 ? buffer.getWritePointer (1) : nullptr;
    auto midiIt = midi.cbegin();
    const float smMacro = 1.0f - std::exp (-(float) chunk / (0.03f * (float) sr));
    const float smVol = 1.0f - std::exp (-1.0f / (0.02f * (float) sr));

    for (int c0 = 0; c0 < n; c0 += chunk)
    {
        const int len = std::min (chunk, n - c0);
        if (fade == Fade::out && fadeGain <= 0.0f) { switchToPending(); fade = Fade::in; }
        if (hasPending && fade == Fade::none) fade = Fade::out;
        for (int m = 0; m < vault::numMacros; ++m) macroSm[m] += (macroTarget[m].load (std::memory_order_relaxed) - macroSm[m]) * smMacro;
        updateParams (beat0 + bps * c0, bpm);

        const int np = std::min (active->numParts, preparedParts);
        // LFOs per part
        for (int k = 0; k < np; ++k)
        {
            auto& p = *parts[(size_t) k];
            const float* vals = active->values[k];
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
            gen (kk::vk::lfoSync, kk::vk::lfoDiv, kk::vk::lfoRate, kk::vk::lfoShape, p.ph1, p.last1, p.sh1, lfo1.data() + k * chunk);
            gen (kk::vk::lfo2Sync, kk::vk::lfo2Div, kk::vk::lfo2Rate, kk::vk::lfo2Shape, p.ph2, p.last2, p.sh2, lfo2.data() + k * chunk);
        }

        std::fill_n (busL.data(), len, 0.0f); std::fill_n (busR.data(), len, 0.0f); std::fill_n (busG.data(), len, 0.0f);
        std::fill (partL.begin(), partL.end(), 0.0f); std::fill (partR.begin(), partR.end(), 0.0f);
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
            for (int k = 0; k < np; ++k)
                parts[(size_t) k]->synth.render (partL.data() + k * chunk + pos, partR.data() + k * chunk + pos, busG.data() + pos, next - pos, vp[k],
                                                 lfo1.data() + k * chunk + pos, lfo2.data() + k * chunk + pos);
            pos = next;
        }
        for (int k = 0; k < np; ++k)
        {
            const float gl = active->level[k] * active->panL[k], gr = active->level[k] * active->panR[k];
            const float* pl = partL.data() + k * chunk; const float* prr = partR.data() + k * chunk;
            for (int i = 0; i < len; ++i) { busL[(size_t) i] += pl[i] * gl; busR[(size_t) i] += prr[i] * gr; }
        }

        fx->process (busL.data(), busR.data(), busG.data(), len, fp);

        const float gainTarget = active->gain;
        const float volTarget = volumeTarget.load (std::memory_order_relaxed);
        for (int i = 0; i < len; ++i)
        {
            // mono below 120 Hz: the side signal is high-passed (36 dB/oct)
            const float m = (busL[(size_t) i] + busR[(size_t) i]) * 0.5f;
            float s = (busL[(size_t) i] - busR[(size_t) i]) * 0.5f;
            s = sideHp[2].tick (sideHp[1].tick (sideHp[0].tick (s)));
            // declick fades on preset switches
            if (fade == Fade::out) fadeGain = std::max (0.0f, fadeGain - fadeStep);
            else if (fade == Fade::in) { fadeGain = std::min (1.0f, fadeGain + fadeStep); if (fadeGain >= 1.0f) fade = Fade::none; }
            gainSm += (gainTarget - gainSm) * smVol;
            volumeSm += (volTarget - volumeSm) * smVol;
            const float g = gainSm * volumeSm * fadeGain;
            float l = (m + s) * g, r = (m - s) * g;
            // safety limiter (instant attack, 120 ms release) + soft ceiling
            const float pk = std::max (std::abs (l), std::abs (r));
            limEnv = pk > limEnv ? pk : limEnv * limRel + pk * (1.0f - limRel);
            if (limEnv > limThresh) { const float lg = limThresh / limEnv; l *= lg; r *= lg; }
            auto soft = [] (float x) { const float a = std::abs (x); return a < 0.9f ? x : std::copysign (0.9f + 0.08f * std::tanh ((a - 0.9f) / 0.08f), x); };
            l = soft (l); r = soft (r);
            if (! std::isfinite (l) || ! std::isfinite (r)) { l = r = 0; limEnv = 0; for (auto& h : sideHp) h.z1 = h.z2 = 0; }
            if (outR != nullptr) { outL[c0 + i] = l; outR[c0 + i] = r; }
            else outL[c0 + i] = 0.5f * (l + r);
        }
    }
    for (int c = 2; c < nCh; ++c) buffer.clear (c, 0, n);
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
    juce::AudioBuffer<float> out (2, std::max (1, numSamples));
    out.clear();
    const auto compiled = compile (preset);
    auto eng = std::make_unique<VaultEngine>();
    for (int m = 0; m < vault::numMacros; ++m) eng->setMacro (m, macros != nullptr ? macros[m] : compiled.macroDefaults[m]);
    constexpr int block = 512;
    eng->prepare (sampleRate, block, std::max (1, compiled.numParts));
    eng->setPreset (compiled, true);
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
