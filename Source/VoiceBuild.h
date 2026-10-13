#pragma once
#include "ParamKeys.h"
#include "Synth.h"
#include "Fx.h"
#include "Presets.h"

// Parameter values -> VoiceParams / FxParams. Shared by EVOLVE (KeysKillaProcessor::buildVoiceParams, values from its
// APVTS + morph corners) and CHROME VAULT (values of a preset part). Allocation-free: safe on the audio thread.
namespace kk
{
struct VoiceBuildContext
{
    bool  morphActive = false;   // EVOLVE morph corners in use (character morph is then off)
    float pitchWheel = 0, modWheel = 0, aftertouch = 0;   // pitchWheel -1..1 (already clamped by the caller)
    bool  eco = false;
    std::array<int, numFxSlots> fxOrder = defaultFxOrder();
    float* morphW = nullptr;     // optional out: the four morph weights (written first, before any value is read)
};

// Get: callable float (int key) returning the REAL value of kk::vk key `key` (see ParamKeys.h)
template <typename Get>
void buildVoiceParamsFrom (Get&& P, const VoiceBuildContext& ctx, VoiceParams& v, FxParams& f)
{
    auto B = [&P] (int k) { return P (k) > 0.5f; };

    // morph weights: TL classic, TR melodic, BL raw, BR aggressive
    float localW[4];
    float* morphW = ctx.morphW != nullptr ? ctx.morphW : localW;
    const float x = P (vk::morphX), y = P (vk::morphY);
    morphW[0] = (1 - x) * y; morphW[1] = x * y; morphW[2] = (1 - x) * (1 - y); morphW[3] = x * (1 - y);

    const bool bass = B (vk::bassMode);
    float cutOct = 0, drive = P (vk::drive), crush = P (vk::crush), wow = P (vk::wow), chorus = P (vk::chorus);
    float dMix = P (vk::delayMix), rMix = P (vk::revMix), rSize = P (vk::revSize), width = P (vk::width), detune = P (vk::detune);
    float lfoF = P (vk::lfoFilter), lfoP = P (vk::lfoPitch), sub = P (vk::sub), glide = P (vk::glide), punch = 0, wobble = 0;
    const float m1 = P (vk::m1), m2 = P (vk::m2), m3 = P (vk::m3);
    const float m4 = P (vk::m4), m5 = P (vk::m5), m6 = P (vk::m6);
    const float m7 = P (vk::m7), m8 = P (vk::m8);

    if (! bass)
    {
        cutOct += (0.5f - m1) * 5.0f;   // DARK
        rMix += m2 * 0.45f; dMix += m2 * 0.25f; rSize += m2 * 0.3f;
        drive += m3 * 0.8f;
        crush += m4 * 0.45f; wow += m4 * 0.7f;
        lfoF += m5 * 0.3f; chorus += m5 * 0.45f; lfoP += m5 * 0.06f;
        width += (m6 - 0.5f); detune += (m6 - 0.5f) * 0.4f;
    }
    else
    {
        sub += m1; wobble = m2; drive += m3 * 0.8f; glide += m4 * 0.4f;
        cutOct += (m5 - 0.5f) * 5.0f; punch = m6;
    }

    if (! ctx.morphActive)   // character morph (no corner presets assigned)
    {
        const float dC = morphW[0] - 0.25f, dM = morphW[1] - 0.25f, dR = morphW[2] - 0.25f, dA = morphW[3] - 0.25f;
        crush += dC * 0.35f; width -= dC * 0.3f; chorus -= dC * 0.2f; rMix += dC * 0.1f;
        rMix += dM * 0.35f; chorus += dM * 0.5f; dMix += dM * 0.3f; cutOct -= dM * 0.5f;
        rMix -= dR * 0.4f; dMix -= dR * 0.3f; drive += dR * 0.15f; cutOct += dR * 0.3f; width -= dR * 0.2f;
        drive += dA * 0.9f; cutOct += dA * 1.0f; width += dA * 0.4f;
    }

    // ERA: move the sound through the production eras (difference between the chosen era and the preset's own era)
    float eraRelMul = 1.0f, eraDetune = 0, eraReverse = 0, eraGhost = 0, eraCircuit = 0, eraPunch = 0, eraDrift = 0, eraAlive = 0;
    {
        const float home = P (vk::eraHome), target = P (vk::era);
        if (std::abs (target - home) > 1.0e-3f)
        {
            const auto a = eraProfile (home), b = eraProfile (target);
            const float k = bass ? 0.4f : 1.0f;   // bass keeps its low end: only a hint of the era
            crush += (b.crush - a.crush) * k; wow += (b.wow - a.wow) * k; width += (b.width - a.width) * k;
            rMix += (b.rev - a.rev) * k; dMix += (b.dly - a.dly) * k; chorus += (b.chorus - a.chorus) * k; drive += (b.drive - a.drive);
            cutOct += (b.cutOct - a.cutOct) * k; eraDetune = (b.detune - a.detune) * k; eraReverse = (b.reverse - a.reverse) * k;
            eraGhost = (b.ghost - a.ghost) * k; eraCircuit = b.circuit - a.circuit; eraPunch = b.punch - a.punch;
            eraDrift = (b.drift - a.drift) * k; eraAlive = (b.alive - a.alive) * k; eraRelMul = b.relMul / a.relMul;
            detune += eraDetune;
        }
    }
    // FUTURE: ORIGINAL -> HYBRID -> UNKNOWN. Same seed = same result, so a session reopens exactly.
    const float fut = P (vk::future);
    float futWave = 0, futFm = 0, futHalf = 0, futBodyMix = 0; int futBody = 0;
    if (fut > 1.0e-3f)
    {
        const uint32_t h = hash32 ((uint32_t) P (vk::seed) * 2654435761u + 17u);
        eraReverse += fut * 0.45f; eraGhost += fut * (bass ? 0.1f : 0.4f); eraCircuit += fut * 0.3f; chorus += fut * 0.25f;
        rMix += bass ? 0.0f : fut * 0.25f; width += bass ? 0.0f : fut * 0.2f; eraDrift += fut * 0.35f; eraAlive += fut * 3.0f;
        futWave = fut * (0.15f + 0.3f * (float) (h & 255u) / 255.0f) * ((h & 256u) ? 1.0f : -1.0f);
        futFm = fut * 0.35f;
        futHalf = std::max (0.0f, fut - 0.7f) * (bass ? 0.0f : 1.2f);
        futBody = 1 + (int) ((h >> 9) % 6u); futBodyMix = std::max (0.0f, fut - 0.25f) * 0.8f;
    }

    // TIME macro: 0 TIGHT .. 0.33 NATURAL (neutral) .. DREAM .. >0.9 FROZEN
    const float tm = P (vk::timeM);
    float relMul = 1.0f, dFb = P (vk::delayFb); bool freeze = B (vk::freeze);
    if (tm < 0.33f)
    {
        const float k = (0.33f - tm) / 0.33f;   // tighter
        relMul = 1.0f - 0.8f * k; rMix *= 1.0f - 0.7f * k; dMix *= 1.0f - 0.7f * k; rSize -= 0.3f * k;
    }
    else
    {
        const float k = std::min (1.0f, (tm - 0.33f) / 0.57f);   // dreamier
        relMul = 1.0f + 3.0f * k; rMix += 0.35f * k; rSize += 0.4f * k; dMix += 0.15f * k;
        dFb = std::min (0.9f, dFb + 0.3f * k);
        if (tm > 0.9f) freeze = true;
    }

    {   // MIX macro: overall effect balance, 0.5 = as designed
        const float wet = 0.2f + 1.6f * m8;
        rMix *= wet; dMix *= wet; chorus *= wet; eraReverse *= wet;
    }

    auto c01 = clamp01;
    auto layer = [&] (LayerParams& L, bool on, int eng, int oct, int sem, int fin, int wav, int uni, int det, int r1, int r2, int fa, int alg, int wm, int lvl, float detOverride)
    {
        L.on = on; L.engine = (int) P (eng); L.octave = (int) P (oct); L.semi = (int) P (sem); L.fine = P (fin);
        L.wave = P (wav); L.unison = std::clamp ((int) P (uni), 1, 8); L.detune = detOverride >= 0 ? detOverride : P (det);
        L.fmRatio = P (r1); L.fmRatio2 = P (r2); L.fmAmt = P (fa); L.fmAlgo = (int) P (alg); L.warpMode = (int) P (wm); L.level = P (lvl);
    };
    layer (v.layer[0], true, vk::engine, vk::octave, vk::semi, vk::fine, vk::wave, vk::unison, vk::detune, vk::fmRatio, vk::fmRatio2, vk::fmAmt, vk::fmAlgo, vk::warpMode, vk::levelA, c01 (detune));
    layer (v.layer[1], B (vk::layerB), vk::engineB, vk::octaveB, vk::semiB, vk::fineB, vk::waveB, vk::unisonB, vk::detuneB, vk::fmRatioB, vk::fmRatio2B, vk::fmAmtB, vk::fmAlgoB, vk::warpModeB, vk::levelB, -1);

    v.sub = c01 (sub); v.filterType = (int) P (vk::filterType);
    v.cutoff = std::clamp (P (vk::cutoff) * std::exp2 (cutOct), 40.0f, 20000.0f);
    v.reso = P (vk::reso); v.keyTrack = P (vk::keyTrack); v.fenv = P (vk::fenv);
    v.fA = P (vk::fattack); v.fD = P (vk::fdecay); v.fS = P (vk::fsustain); v.fR = P (vk::frelease);
    v.attack = P (vk::attack) * (1.0f - 0.6f * m7); v.decay = P (vk::decay); v.sustain = P (vk::sustain); v.release = std::min (8.0f, P (vk::release) * relMul * eraRelMul); v.velSens = P (vk::velSens);
    v.e3A = P (vk::e3attack); v.e3D = P (vk::e3decay); v.e3S = P (vk::e3sustain); v.e3R = P (vk::e3release);
    v.lfoPitch = c01 (lfoP); v.lfoFilter = c01 (lfoF); v.lfoAmp = P (vk::lfoAmp);
    v.wobTarget = (int) P (vk::wobTarget); v.wobble = c01 (wobble);
    v.mono = B (vk::mono); v.legato = B (vk::legato); v.glide = std::clamp (glide, 0.0f, 1.0f);
    v.bendRange = P (vk::bendRange);
    v.ghost = c01 (P (vk::ghost) + eraGhost); v.ghostOct = (int) P (vk::ghostOct);
    v.bend = P (vk::bend); v.bendMode = (int) P (vk::bendMode); v.bendSemis = P (vk::bendSemis); v.tape = B (vk::tape);
    v.punch = punch;
    v.alive = std::clamp (P (vk::alive) + eraAlive, 0.0f, 5.0f); v.drift = c01 (P (vk::drift) + eraDrift);
    f.punch = c01 (P (vk::punch) + eraPunch + m7 * 0.9f); f.halftime = c01 (P (vk::halftime) + futHalf);
    f.master = c01 (P (vk::master));
    for (auto& L : v.layer) { L.wave = c01 (L.wave + futWave); L.fmAmt = c01 (L.fmAmt + futFm); }
    v.pitchWheel = ctx.pitchWheel;
    v.modWheel = ctx.modWheel;
    v.aftertouch = ctx.aftertouch;
    for (int s = 0; s < numModSlots; ++s)
    {
        v.mmSrc[s] = (int) P (vk::mmSrc (s)); v.mmDst[s] = (int) P (vk::mmDst (s)); v.mmAmt[s] = P (vk::mmAmt (s));
    }
    v.eco = ctx.eco;

    f.drive = c01 (drive); f.driveType = (int) P (vk::driveType); f.cleanLow = bass; f.monoLows = bass;
    f.crush = c01 (crush); f.wow = c01 (wow); f.chorus = c01 (chorus); f.phaser = P (vk::phaser); f.flanger = P (vk::flanger);
    f.delayMix = c01 (dMix); f.delayFb = dFb; f.delayBeats = Choices::delayBeats ((int) P (vk::delayTime)); f.delayMode = (int) P (vk::delayMode);
    f.revMix = c01 (rMix); f.revSize = c01 (rSize); f.revType = (int) P (vk::revType); f.freeze = freeze;
    f.eqLow = P (vk::eqLow); f.eqHigh = P (vk::eqHigh); f.reverse = c01 (P (vk::reverse) + eraReverse);
    f.width = c01 (width);
    f.ghost = c01 (P (vk::ghost) + eraGhost); f.ghostBlur = P (vk::ghostBlur); f.ghostRev = B (vk::ghostRev);
    f.circuit = c01 (P (vk::circuit) + eraCircuit); f.circBeats = Choices::circBeats ((int) P (vk::circRate));
    f.body = (int) P (vk::body); f.bodyMix = P (vk::bodyMix);
    if (futBodyMix > 0 && f.body == 0) { f.body = futBody; f.bodyMix = futBodyMix; }
    f.outGain = juce::Decibels::decibelsToGain (P (vk::gain));
    f.seed = (uint32_t) P (vk::seed);
    f.eco = v.eco;
    f.order = ctx.fxOrder;
}
} // namespace kk
