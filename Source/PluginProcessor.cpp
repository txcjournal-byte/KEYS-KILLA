#include "PluginProcessor.h"
#include <set>
#include <functional>
#include "PluginEditor.h"
#include "BinaryData.h"
#include "EmbeddedPlugins.h"

namespace
{
constexpr int kChunk = 512;

// every parameter the audio thread reads by name
#define KK_PARAMS(X) \
    X(engine) X(octave) X(semi) X(fine) X(wave) X(unison) X(detune) X(fmRatio) X(fmRatio2) X(fmAmt) X(fmAlgo) X(warpMode) X(levelA) \
    X(layerB) X(engineB) X(octaveB) X(semiB) X(fineB) X(waveB) X(unisonB) X(detuneB) X(fmRatioB) X(fmRatio2B) X(fmAmtB) X(fmAlgoB) X(warpModeB) X(levelB) \
    X(sub) X(filterType) X(cutoff) X(reso) X(keyTrack) X(fenv) X(fattack) X(fdecay) X(fsustain) X(frelease) \
    X(attack) X(decay) X(sustain) X(release) X(velSens) X(e3attack) X(e3decay) X(e3sustain) X(e3release) \
    X(lfoRate) X(lfoSync) X(lfoDiv) X(lfoShape) X(lfoPitch) X(lfoFilter) X(lfoAmp) X(lfo2Rate) X(lfo2Sync) X(lfo2Div) X(lfo2Shape) X(wobTarget) \
    X(mono) X(legato) X(glide) X(bendRange) X(bassMode) X(keyLock) X(key) X(scale) X(chord) X(chordType) X(strum) \
    X(arp) X(arpRate) X(arpMode) X(arpOct) X(arpSwing) X(arpGate) \
    X(drive) X(driveType) X(crush) X(wow) X(chorus) X(phaser) X(flanger) X(delayMix) X(delayTime) X(delayFb) X(delayMode) \
    X(revMix) X(revSize) X(revType) X(eqLow) X(eqHigh) X(reverse) X(freeze) X(width) X(gain) \
    X(m1) X(m2) X(m3) X(m4) X(m5) X(m6) X(m7) X(m8) \
    X(ghost) X(ghostOct) X(ghostRev) X(ghostBlur) X(bend) X(bendMode) X(bendSemis) X(tape) X(circuit) X(circRate) \
    X(chaos) X(morphX) X(morphY) X(body) X(bodyMix) X(seed) \
    X(alive) X(drift) X(timeM) X(punch) X(halftime) X(era) X(eraHome) X(future) X(arpSteps) X(master) \
    X(halfOn) X(efxOn) X(playMode) X(world) X(worldAmt) X(gate) X(gateDepth) X(clipMode) X(clipDrive) \
    X(rlStyle) X(rlSeed) X(rlBars) X(rlDensity)

// performance controls that never morph or get reset by presets
const juce::StringArray performanceIds { ID::chord, ID::chordType, ID::strum, ID::arp, ID::arpRate, ID::arpMode, ID::arpOct,
                                         ID::arpSwing, ID::arpGate, ID::keyLock, ID::key, ID::scale, ID::chaos,
                                         ID::morphX, ID::morphY, ID::bendRange,
                                         ID::m1, ID::m2, ID::m3, ID::m4, ID::m5, ID::m6, ID::m7, ID::m8 };
const juce::StringArray keepOnPresetLoad { ID::chord, ID::chordType, ID::strum, ID::arp, ID::arpRate, ID::arpMode, ID::arpOct,
                                           ID::arpSwing, ID::arpGate, ID::keyLock, ID::key, ID::scale, ID::chaos, ID::bendRange };
bool keeps (const juce::String& id) { return keepOnPresetLoad.contains (id) || ID::isModuleParam (id); }

// chord shapes (semitones); -1 terminates. Type 8 (scale triad) is built from the scale.
// trap voicings first: open minor (root, 5th, minor 10th), dark minor with octave, add9, sus, power, octaves, phrygian b2
const int chordTable[9][6] { { 0, 7, 15, -1 }, { 0, 3, 7, 12, -1 }, { 0, 3, 7, 14, -1 }, { 0, 5, 7, 12, -1 },
                             { 0, 7, 12, -1 }, { 0, 12, -1 }, { 0, 1, 7, -1 }, { 0, 3, 7, 10, -1 }, { 0, 4, 7, -1 } };
} // namespace

struct KeysKillaProcessor::Idx
{
   #define KK_DECL(n) int n = -1;
    KK_PARAMS (KK_DECL)
   #undef KK_DECL
    int mmSrc[numModSlots] {}, mmDst[numModSlots] {}, mmAmt[numModSlots] {};
    int arpStep[16] {}, arpNote[16] {}, arpLen[16] {};
};

KeysKillaProcessor::KeysKillaProcessor (bool withModules)
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KEYSKILLA", createLayout())
{
    for (auto& p : playing) p = false;
    { const auto d = kk::defaultFxOrder(); for (int i = 0; i < kk::numFxSlots; ++i) fxOrder[(size_t) i] = d[(size_t) i]; }

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            params.push_back (rp);
            raw.push_back (apvts.getRawParameterValue (rp->getParameterID()));
            const auto id = rp->getParameterID();
            morphable.push_back (! performanceIds.contains (id) && ! ID::isModuleParam (id) && ! ID::isArpPattern (id) && id != ID::seed);
            discrete.push_back (dynamic_cast<juce::AudioParameterChoice*> (rp) != nullptr || dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr);
        }
    for (size_t i = 0; i < params.size(); ++i)
    {
        idIndex[params[i]->getParameterID()] = (int) i;
        keepParam.push_back (keeps (params[i]->getParameterID()) || ID::isArpPattern (params[i]->getParameterID()));
    }
    ix = std::make_unique<Idx>();
   #define KK_SET(n) ix->n = indexOf (ID::n);
    KK_PARAMS (KK_SET)
   #undef KK_SET
    for (int s = 0; s < numModSlots; ++s)
    {
        ix->mmSrc[s] = indexOf (ID::mmSrc (s)); ix->mmDst[s] = indexOf (ID::mmDst (s)); ix->mmAmt[s] = indexOf (ID::mmAmt (s));
    }
    for (int st = 0; st < 16; ++st) { ix->arpStep[st] = indexOf (ID::arpStep (st)); ix->arpNote[st] = indexOf (ID::arpNote (st)); ix->arpLen[st] = indexOf (ID::arpLen (st)); }
    for (auto& bank : cornerBank) for (auto& c : bank) c.assign (params.size(), 0.0f);
    for (auto* prm : params)
    {
        const auto id = prm->getParameterID();
        auto any = [&] (std::initializer_list<const char*> ids) { for (auto* x : ids) if (id == x) return true; return false; };
        int gene = geneBody;
        if (keeps (id) || ID::isArpPattern (id) || any ({ ID::gain, ID::chaos, ID::morphX, ID::morphY, ID::bendRange, ID::master })) gene = -1;
        else if (any ({ ID::attack, ID::decay, ID::sustain, ID::release, ID::velSens, ID::fattack, ID::fdecay, ID::fsustain, ID::frelease,
                        ID::fenv, ID::punch, ID::bend, ID::bendMode, ID::bendSemis })) gene = geneAttack;
        else if (any ({ ID::crush, ID::wow, ID::drive, ID::driveType, ID::tape, ID::circuit, ID::circRate, ID::body, ID::bodyMix,
                        ID::seed, ID::halftime })) gene = geneTexture;
        else if (any ({ ID::revMix, ID::revSize, ID::revType, ID::freeze, ID::delayMix, ID::delayTime, ID::delayFb, ID::delayMode,
                        ID::width, ID::reverse, ID::timeM })) gene = geneSpace;
        else if (id.startsWith ("lfo") || id.startsWith ("e3") || id.startsWith ("mm")
                 || any ({ ID::wobTarget, ID::chorus, ID::phaser, ID::flanger, ID::alive, ID::drift })) gene = geneMovement;
        else if (id.startsWith ("ghost") || any ({ ID::filterType, ID::cutoff, ID::reso, ID::keyTrack, ID::eqLow, ID::eqHigh,
                                                     ID::era, ID::eraHome, ID::future })) gene = geneCharacter;
        geneOfParam.push_back (gene);
    }
    noteMap.fill (-1);
    shRng.seed (99); arpRng.seed (7);

    loadPreset (0);
    undoStack.clear(); redoStack.clear(); lastSnap.clear();
    setParentPreset (0, 0);
    setParentPreset (1, juce::jmin ((int) factoryPresets().size() - 1, 250));

    // v0.17 modules: Voodoo Killa (HALF), Effector Killa (EFFECTOR), Digga Killa (DIGGA)
    if (withModules)
    {
        modules[modHalf].reset (kkCreateVoodooKilla());
        modules[modEffector].reset (kkCreateEffectorKilla());
        modules[modDigga].reset (kkCreateDiggaKilla());
    }
    rebuildRolls();
}

KeysKillaProcessor::~KeysKillaProcessor()
{
    cancelPendingUpdate();
    thumbRenderer.reset();   // the offline waveform renderer goes first
    history.clear(); children.clear();
}

void KeysKillaProcessor::releaseThumbnailRenderer() { thumbRenderer.reset(); }

int KeysKillaProcessor::indexOf (const juce::String& id) const
{
    const auto it = idIndex.find (id);
    if (it != idIndex.end()) return it->second;
    for (size_t i = 0; i < params.size(); ++i) if (params[i]->getParameterID() == id) return (int) i;
    jassertfalse;
    return 0;
}

bool KeysKillaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet().isDisabled();
}

void KeysKillaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.prepare ((float) sampleRate);
    fxPtr = std::make_unique<kk::FxRack>();   // fresh DSP state on every prepare
    fxPtr->prepare (sampleRate, kChunk);
    bufL.assign (kChunk, 0.0f); bufR.assign (kChunk, 0.0f); bufG.assign (kChunk, 0.0f);
    lfoBuf.assign (kChunk, 0.0f); lfo2Buf.assign (kChunk, 0.0f);
    processedMidi.ensureSize (8192);
    arpHeld.fill (false); arpOrderN = 0; arpLastStep = -1; pendingN = 0;
    noteMap.fill (-1);
    midiPitch = midiMod = midiAT = 0;
    lfoPhase = lfo2Phase = sh1 = sh2 = lastPh1 = lastPh2 = 0; freeBeat = 0;
    kk::WavetableBank::get();
    worldStage.prepare (sampleRate);
    for (auto& d : clipDc) d = { 0, 0 };
    for (int d = 0; d < 3; ++d)   // drums are stored at the plugin rate: reload when the rate changes
        if (drums[(size_t) d].hasSample() && std::abs (drums[(size_t) d].loadedRate() - sampleRate) > 0.5)
            loadDrum (d, juce::File (drums[(size_t) d].filePath()));
    modBlock = std::max (64, samplesPerBlock);
    modBuf.setSize (2, modBlock); modDry.setSize (2, modBlock);
    modMidi.ensureSize (4096); keysForModules.ensureSize (4096); noMidi.ensureSize (64);
    modFade = { 0, 0 };
    for (auto& m : modules)
        if (m)
        {
            m->setRateAndBufferSizeDetails (sampleRate, modBlock);
            m->prepareToPlay (sampleRate, modBlock);
        }
}

float KeysKillaProcessor::value (int i) const
{
    if (morphActive.load (std::memory_order_acquire) && morphable[(size_t) i])
    {
        const auto& bank = cornerBank[(size_t) cornerBankIdx.load (std::memory_order_acquire)];
        float v;
        if (discrete[(size_t) i])
        {
            int best = 0;
            for (int c = 1; c < 4; ++c) if (morphW[c] > morphW[best]) best = c;
            v = bank[(size_t) best][(size_t) i];
        }
        else
        {
            v = 0;
            for (int c = 0; c < 4; ++c) v += morphW[c] * bank[(size_t) c][(size_t) i];
        }
        return params[(size_t) i]->convertFrom0to1 (juce::jlimit (0.0f, 1.0f, v));
    }
    return raw[(size_t) i]->load (std::memory_order_relaxed);
}

void KeysKillaProcessor::buildVoiceParams (kk::VoiceParams& v, kk::FxParams& f)
{
    const auto& I = *ix;
    auto P = [this] (int i) { return value (i); };
    auto B = [this] (int i) { return value (i) > 0.5f; };

    // morph weights: TL classic, TR melodic, BL raw, BR aggressive
    const float x = raw[(size_t) I.morphX]->load(), y = raw[(size_t) I.morphY]->load();
    morphW[0] = (1 - x) * y; morphW[1] = x * y; morphW[2] = (1 - x) * (1 - y); morphW[3] = x * (1 - y);

    const bool bass = B (I.bassMode);
    float cutOct = 0, drive = P (I.drive), crush = P (I.crush), wow = P (I.wow), chorus = P (I.chorus);
    float dMix = P (I.delayMix), rMix = P (I.revMix), rSize = P (I.revSize), width = P (I.width), detune = P (I.detune);
    float lfoF = P (I.lfoFilter), lfoP = P (I.lfoPitch), sub = P (I.sub), glide = P (I.glide), punch = 0, wobble = 0;
    const float m1 = raw[(size_t) I.m1]->load(), m2 = raw[(size_t) I.m2]->load(), m3 = raw[(size_t) I.m3]->load();
    const float m4 = raw[(size_t) I.m4]->load(), m5 = raw[(size_t) I.m5]->load(), m6 = raw[(size_t) I.m6]->load();
    const float m7 = raw[(size_t) I.m7]->load(), m8 = raw[(size_t) I.m8]->load();

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

    if (! morphActive.load())   // character morph (no corner presets assigned)
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
        const float home = P (I.eraHome), target = P (I.era);
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
    const float fut = P (I.future);
    float futWave = 0, futFm = 0, futHalf = 0, futBodyMix = 0; int futBody = 0;
    if (fut > 1.0e-3f)
    {
        const uint32_t h = kk::hash32 ((uint32_t) P (I.seed) * 2654435761u + 17u);
        eraReverse += fut * 0.45f; eraGhost += fut * (bass ? 0.1f : 0.4f); eraCircuit += fut * 0.3f; chorus += fut * 0.25f;
        rMix += bass ? 0.0f : fut * 0.25f; width += bass ? 0.0f : fut * 0.2f; eraDrift += fut * 0.35f; eraAlive += fut * 3.0f;
        futWave = fut * (0.15f + 0.3f * (float) (h & 255u) / 255.0f) * ((h & 256u) ? 1.0f : -1.0f);
        futFm = fut * 0.35f;
        futHalf = std::max (0.0f, fut - 0.7f) * (bass ? 0.0f : 1.2f);
        futBody = 1 + (int) ((h >> 9) % 6u); futBodyMix = std::max (0.0f, fut - 0.25f) * 0.8f;
    }

    // TIME macro: 0 TIGHT .. 0.33 NATURAL (neutral) .. DREAM .. >0.9 FROZEN
    const float tm = P (I.timeM);
    float relMul = 1.0f, dFb = P (I.delayFb); bool freeze = B (I.freeze);
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

    auto c01 = kk::clamp01;
    auto layer = [&] (kk::LayerParams& L, bool on, int eng, int oct, int sem, int fin, int wav, int uni, int det, int r1, int r2, int fa, int alg, int wm, int lvl, float detOverride)
    {
        L.on = on; L.engine = (int) P (eng); L.octave = (int) P (oct); L.semi = (int) P (sem); L.fine = P (fin);
        L.wave = P (wav); L.unison = juce::jlimit (1, 8, (int) P (uni)); L.detune = detOverride >= 0 ? detOverride : P (det);
        L.fmRatio = P (r1); L.fmRatio2 = P (r2); L.fmAmt = P (fa); L.fmAlgo = (int) P (alg); L.warpMode = (int) P (wm); L.level = P (lvl);
    };
    layer (v.layer[0], true, I.engine, I.octave, I.semi, I.fine, I.wave, I.unison, I.detune, I.fmRatio, I.fmRatio2, I.fmAmt, I.fmAlgo, I.warpMode, I.levelA, c01 (detune));
    layer (v.layer[1], B (I.layerB), I.engineB, I.octaveB, I.semiB, I.fineB, I.waveB, I.unisonB, I.detuneB, I.fmRatioB, I.fmRatio2B, I.fmAmtB, I.fmAlgoB, I.warpModeB, I.levelB, -1);

    v.sub = c01 (sub); v.filterType = (int) P (I.filterType);
    v.cutoff = juce::jlimit (40.0f, 20000.0f, P (I.cutoff) * std::exp2 (cutOct));
    v.reso = P (I.reso); v.keyTrack = P (I.keyTrack); v.fenv = P (I.fenv);
    v.fA = P (I.fattack); v.fD = P (I.fdecay); v.fS = P (I.fsustain); v.fR = P (I.frelease);
    v.attack = P (I.attack) * (1.0f - 0.6f * m7); v.decay = P (I.decay); v.sustain = P (I.sustain); v.release = std::min (8.0f, P (I.release) * relMul * eraRelMul); v.velSens = P (I.velSens);
    v.e3A = P (I.e3attack); v.e3D = P (I.e3decay); v.e3S = P (I.e3sustain); v.e3R = P (I.e3release);
    v.lfoPitch = c01 (lfoP); v.lfoFilter = c01 (lfoF); v.lfoAmp = P (I.lfoAmp);
    v.wobTarget = (int) P (I.wobTarget); v.wobble = c01 (wobble);
    v.mono = B (I.mono); v.legato = B (I.legato); v.glide = juce::jlimit (0.0f, 1.0f, glide);
    v.bendRange = raw[(size_t) I.bendRange]->load();
    v.ghost = c01 (P (I.ghost) + eraGhost); v.ghostOct = (int) P (I.ghostOct);
    v.bend = P (I.bend); v.bendMode = (int) P (I.bendMode); v.bendSemis = P (I.bendSemis); v.tape = B (I.tape);
    v.punch = punch;
    v.alive = juce::jlimit (0.0f, 5.0f, P (I.alive) + eraAlive); v.drift = c01 (P (I.drift) + eraDrift);
    f.punch = c01 (P (I.punch) + eraPunch + m7 * 0.9f); f.halftime = c01 (P (I.halftime) + futHalf);
    f.master = c01 (P (I.master));
    for (auto& L : v.layer) { L.wave = c01 (L.wave + futWave); L.fmAmt = c01 (L.fmAmt + futFm); }
    v.pitchWheel = juce::jlimit (-1.0f, 1.0f, midiPitch + guiPitch.load());
    v.modWheel = std::max (midiMod, guiMod.load());
    v.aftertouch = midiAT;
    for (int s = 0; s < numModSlots; ++s)
    {
        v.mmSrc[s] = (int) P (I.mmSrc[s]); v.mmDst[s] = (int) P (I.mmDst[s]); v.mmAmt[s] = P (I.mmAmt[s]);
    }
    v.eco = eco.load();

    f.drive = c01 (drive); f.driveType = (int) P (I.driveType); f.cleanLow = bass; f.monoLows = bass;
    f.crush = c01 (crush); f.wow = c01 (wow); f.chorus = c01 (chorus); f.phaser = P (I.phaser); f.flanger = P (I.flanger);
    f.delayMix = c01 (dMix); f.delayFb = dFb; f.delayBeats = Choices::delayBeats ((int) P (I.delayTime)); f.delayMode = (int) P (I.delayMode);
    f.revMix = c01 (rMix); f.revSize = c01 (rSize); f.revType = (int) P (I.revType); f.freeze = freeze;
    f.eqLow = P (I.eqLow); f.eqHigh = P (I.eqHigh); f.reverse = c01 (P (I.reverse) + eraReverse);
    f.width = c01 (width);
    f.ghost = c01 (P (I.ghost) + eraGhost); f.ghostBlur = P (I.ghostBlur); f.ghostRev = B (I.ghostRev);
    f.circuit = c01 (P (I.circuit) + eraCircuit); f.circBeats = Choices::circBeats ((int) P (I.circRate));
    f.body = (int) P (I.body); f.bodyMix = P (I.bodyMix);
    if (futBodyMix > 0 && f.body == 0) { f.body = futBody; f.bodyMix = futBodyMix; }
    f.outGain = juce::Decibels::decibelsToGain (P (I.gain));
    f.seed = (uint32_t) P (I.seed);
    f.eco = v.eco;
    for (int i = 0; i < kk::numFxSlots; ++i) f.order[(size_t) i] = fxOrder[(size_t) i].load();
}

void KeysKillaProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                 synth.noteOn (m.getNoteNumber(), m.getFloatVelocity(), vp);
    else if (m.isNoteOff())           synth.noteOff (m.getNoteNumber(), vp);
    else if (m.isSustainPedalOn())    synth.setSustain (true);
    else if (m.isSustainPedalOff())   synth.setSustain (false);
    else if (m.isAllNotesOff() || m.isAllSoundOff()) { synth.setSustain (false); synth.allOff (m.isAllSoundOff()); }
    else if (m.isPitchWheel())        midiPitch = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isChannelPressure())   midiAT = (float) m.getChannelPressureValue() / 127.0f;
    else if (m.isAftertouch())        synth.polyAftertouch (m.getNoteNumber(), (float) m.getAfterTouchValue() / 127.0f);
    else if (m.isController() && m.getControllerNumber() == 1) midiMod = (float) m.getControllerValue() / 127.0f;
}

int KeysKillaProcessor::snapToScale (int note) const
{
    return snapToScaleWith (note, (int) raw[(size_t) ix->key]->load(), Choices::scaleMask ((int) raw[(size_t) ix->scale]->load()));
}

int KeysKillaProcessor::snapToScaleWith (int note, int root, int mask)
{
    if (mask == 0) return note;
    for (int d = 0; d < 12; ++d)
        for (int sgn : { -1, 1 })
        {
            const int n = note + sgn * d;
            const int deg = ((n - root) % 12 + 12) % 12;
            if (mask & (1 << deg)) return juce::jlimit (0, 127, n);
        }
    return note;
}

void KeysKillaProcessor::addPending (int64_t due, int note, float vel, bool on)
{
    if (pendingN < (int) pending.size()) pending[(size_t) pendingN++] = { due, note, vel, on };
}

void KeysKillaProcessor::processMidi (const juce::MidiBuffer& in, juce::MidiBuffer& out, int n, double beatPos, double bpm)
{
    const auto& I = *ix;
    const bool chord = raw[(size_t) I.chord]->load() > 0.5f;
    const bool arp   = false;   // v0.14: ARP removed - the FAMILY TREE loops replace it
    const bool lock  = raw[(size_t) I.keyLock]->load() > 0.5f;
    const int  ctype = (int) raw[(size_t) I.chordType]->load();
    const float strumSec = raw[(size_t) I.strum]->load();

    auto emit = [&] (bool on, int note, float vel, int pos)
    {
        if (note < 0 || note > 127) return;
        if (arp)
        {
            if (on && ! arpHeld[(size_t) note]) { arpHeld[(size_t) note] = true; if (arpOrderN < 128) arpOrder[(size_t) arpOrderN++] = note; }
            else if (! on && arpHeld[(size_t) note])
            {
                arpHeld[(size_t) note] = false;
                int w = 0; for (int k = 0; k < arpOrderN; ++k) if (arpOrder[(size_t) k] != note) arpOrder[(size_t) w++] = arpOrder[(size_t) k];
                arpOrderN = w;
            }
            return;
        }
        out.addEvent (on ? juce::MidiMessage::noteOn (1, note, vel) : juce::MidiMessage::noteOff (1, note), pos);
    };

    auto chordNotes = [&] (int root, int* outNotes) -> int
    {
        int cnt = 0;
        if (! chord) { outNotes[cnt++] = root; return cnt; }
        if (ctype == 9)   // scale triad: stack diatonic thirds
        {
            const int mask = Choices::scaleMask ((int) raw[(size_t) I.scale]->load());
            const int key = (int) raw[(size_t) I.key]->load();
            int nn = root; outNotes[cnt++] = nn;
            for (int t = 0; t < 2; ++t)
            {
                int steps = 0;
                while (steps < 2 && nn < 127) { ++nn; const int deg = ((nn - key) % 12 + 12) % 12; if (mask & (1 << deg)) ++steps; }
                outNotes[cnt++] = nn;
            }
            return cnt;
        }
        for (const int* iv = chordTable[juce::jlimit (0, 8, ctype)]; *iv >= 0; ++iv)
        {
            int nn = root + *iv;
            if (lock) nn = snapToScale (nn);
            bool dup = false; for (int k = 0; k < cnt; ++k) dup |= outNotes[k] == nn;
            if (! dup && nn <= 127) outNotes[cnt++] = nn;
        }
        return cnt;
    };

    // pending events (strum note-ons, arp note-offs) due in this block
    for (int k = 0; k < pendingN;)
    {
        const auto& pe = pending[(size_t) k];
        if (pe.due < sampleClock + n)
        {
            const int pos = (int) juce::jlimit ((int64_t) 0, (int64_t) n - 1, pe.due - sampleClock);
            out.addEvent (pe.on ? juce::MidiMessage::noteOn (1, pe.note, pe.vel) : juce::MidiMessage::noteOff (1, pe.note), pos);
            pending[(size_t) k] = pending[(size_t) --pendingN];
        }
        else ++k;
    }

    for (const auto meta : in)
    {
        const auto m = meta.getMessage();
        const int pos = meta.samplePosition;
        if (m.isNoteOnOrOff())
        {
            const bool on = m.isNoteOn();
            const int src = m.getNoteNumber();
            int root = src;
            if (on) { root = lock ? snapToScale (src) : src; noteMap[(size_t) src] = root; }
            else if (noteMap[(size_t) src] >= 0) { root = noteMap[(size_t) src]; noteMap[(size_t) src] = -1; }
            int notes[8]; const int cnt = chordNotes (root, notes);
            for (int k = 0; k < cnt; ++k)
            {
                if (on && chord && strumSec > 0.0005f && k > 0 && ! arp)
                    addPending (sampleClock + pos + (int64_t) (strumSec * sr * k), notes[k], m.getFloatVelocity(), true);
                else
                {
                    if (! on)   // cancel strummed notes that have not sounded yet
                    {
                        for (int q = 0; q < pendingN;)
                        {
                            if (pending[(size_t) q].on && pending[(size_t) q].note == notes[k]) pending[(size_t) q] = pending[(size_t) --pendingN];
                            else ++q;
                        }
                    }
                    emit (on, notes[k], m.getFloatVelocity(), pos);
                }
            }
        }
        else
        {
            if (m.isAllNotesOff() || m.isAllSoundOff()) { arpHeld.fill (false); arpOrderN = 0; pendingN = 0; noteMap.fill (-1); }
            out.addEvent (m, pos);
        }
    }

    if (! arp) { arpCurStep = -1; return; }

    // ------------------------ arpeggiator (step sequenced) ------------------------
    int list[128 * 4]; int count = 0;
    const int mode = (int) raw[(size_t) I.arpMode]->load();
    const int octs = juce::jlimit (1, 4, (int) raw[(size_t) I.arpOct]->load());
    int base[128]; int nb = 0;
    if (mode == 4) for (int k = 0; k < arpOrderN; ++k) base[nb++] = arpOrder[(size_t) k];
    else for (int i = 0; i < 128; ++i) if (arpHeld[(size_t) i]) base[nb++] = i;
    if (mode == 5 || mode == 6)   // trap scale runs: walk the key / scale up from the lowest held note
    {
        if (nb > 0)
        {
            const int mask = Choices::scaleMask ((int) raw[(size_t) I.scale]->load());
            const int key = (int) raw[(size_t) I.key]->load();
            int nn = snapToScaleWith (base[0], key, mask);
            const int total = std::min (7 * octs, 7 * 4);
            for (int k = 0; k < total && nn < 128; ++k)
            {
                list[count++] = nn;
                do ++nn; while (nn < 128 && ! (mask & (1 << (((nn - key) % 12 + 12) % 12))));
            }
        }
    }
    else
        for (int o = 0; o < octs; ++o) for (int k = 0; k < nb; ++k) if (base[k] + 12 * o < 128) list[count++] = base[k] + 12 * o;

    const double stepBeats = Choices::arpBeats ((int) raw[(size_t) I.arpRate]->load());
    const double swing = raw[(size_t) I.arpSwing]->load();
    const double gate = raw[(size_t) I.arpGate]->load();
    const int steps = juce::jlimit (1, 16, (int) raw[(size_t) I.arpSteps]->load());
    const double bps = bpm / 60.0 / sr;
    const double endBeat = beatPos + bps * n;
    if ((int64_t) std::floor (beatPos / stepBeats) < arpLastStep - 1) arpLastStep = -1;   // transport jumped back
    for (int64_t step = (int64_t) std::floor (beatPos / stepBeats) - 1; ; ++step)
    {
        double b = (double) step * stepBeats;
        if (step % 2 != 0) b += swing * stepBeats;
        if (b >= endBeat) break;
        if (b < beatPos || step <= arpLastStep) continue;
        arpLastStep = step;
        const int stepIdx = (int) (((step % steps) + steps) % steps);
        arpCurStep = count > 0 ? stepIdx : -1;   // the playhead only moves while notes are held
        if (count == 0) continue;
        auto stepOn = [&] (int st) { return raw[(size_t) I.arpStep[st]]->load() > 0.5f; };
        auto stepLen = [&] (int st) { return juce::jlimit (1, steps - st, (int) raw[(size_t) I.arpLen[st]]->load()); };
        bool tied = false;   // inside a longer note that started earlier
        for (int st = 0; st < stepIdx; ++st) if (stepOn (st) && st + stepLen (st) > stepIdx) { tied = true; break; }
        if (tied || ! stepOn (stepIdx)) continue;
        const int transpose = (int) raw[(size_t) I.arpNote[stepIdx]]->load();
        const int pos = juce::jlimit (0, n - 1, (int) ((b - beatPos) / bps));
        const auto vel = (juce::uint8) 100;
        const auto dur = juce::jmax ((int64_t) 16, (int64_t) (((double) stepLen (stepIdx) - 1.0 + gate) * stepBeats / bps));
        if (mode == 7)   // chord: every held note on each step
        {
            for (int k = 0; k < count && k < 12; ++k)
            {
                int cn = juce::jlimit (0, 127, list[k] + transpose);
                if (lock) cn = snapToScale (cn);
                out.addEvent (juce::MidiMessage::noteOn (1, cn, vel), pos);
                addPending (sampleClock + pos + dur, cn, 0, false);
            }
            continue;
        }
        int note;
        switch (mode)
        {
            case 1: case 6: arpIndex = (arpIndex - 1 + count) % count; note = list[arpIndex]; break;
            case 2:  if (count == 1) arpIndex = 0;
                     else { arpIndex += arpDir; if (arpIndex >= count) { arpIndex = count - 2; arpDir = -1; } else if (arpIndex < 0) { arpIndex = 1; arpDir = 1; } }
                     note = list[juce::jlimit (0, count - 1, arpIndex)]; break;
            case 3:  note = list[(int) (arpRng.uni() * (float) count) % count]; break;
            default: arpIndex = (arpIndex + 1) % count; note = list[arpIndex]; break;
        }
        note = juce::jlimit (0, 127, note + transpose);
        if (lock) note = snapToScale (note);
        out.addEvent (juce::MidiMessage::noteOn (1, note, vel), pos);
        addPending (sampleClock + pos + dur, note, 0, false);
    }
}

void KeysKillaProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    bypassed = true;
    if (bypassGain > 0.0f) processBlock (buffer, midi);   // fades out
    else { buffer.clear(); synth.allOff (true); }
    bypassed = true;
}

void KeysKillaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const bool fadingOut = bypassed;
    bypassed = false;
    const int n = buffer.getNumSamples();
    buffer.clear();
    // hosts may call before prepareToPlay or with empty buffers (parameter flush) - nothing to render then
    if (n <= 0 || buffer.getNumChannels() == 0 || bufL.empty() || fxPtr == nullptr) return;
    if (previewActive >= 0 && previewOffIn >= 0)   // BREED LAB audition note
    {
        if (previewOffIn < n) { midi.addEvent (juce::MidiMessage::noteOff (1, previewActive), previewOffIn); previewActive = -1; previewOffIn = -1; }
        else previewOffIn -= n;
    }
    if (const int pn = previewNote.exchange (-1); pn >= 0)
    {
        if (previewActive >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, previewActive), 0);
        midi.addEvent (juce::MidiMessage::noteOn (1, pn, (juce::uint8) 100), 0);
        previewActive = pn; previewOffIn = (int) (sr * 0.9);
    }
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    const auto& I = *ix;

    double bpm = 140.0, ppq = 0; bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = juce::jlimit (20.0, 999.0, *b);
            if (pos->getIsPlaying()) if (auto p = pos->getPpqPosition()) { hostPlaying = true; ppq = *p; }
        }
    const double bps = bpm / 60.0 / sr;
    const double beatPos = hostPlaying ? ppq : freeBeat;
    freeBeat = beatPos + bps * n;

    const bool chord = raw[(size_t) I.chord]->load() > 0.5f, arp = false;   // ARP removed (v0.14)
    const bool lock = raw[(size_t) I.keyLock]->load() > 0.5f;
    if (chord != lastChord || arp != lastArp || lock != lastKeyLock)
    {
        synth.allOff (false); arpHeld.fill (false); arpOrderN = 0; pendingN = 0; noteMap.fill (-1);
        lastChord = chord; lastArp = arp; lastKeyLock = lock;
    }

    if (panicFlag.exchange (false))
    {
        synth.allOff (true); arpHeld.fill (false); arpOrderN = 0; pendingN = 0; noteMap.fill (-1);
    }

    buildVoiceParams (vp, fp);
    fp.bpm = bpm;

    // v0.17: the keys play KEYS KILLA or DIGGA (Digga Killa gets the notes, the synth stays silent)
    keysForModules.clear();
    {
        const int pm = (int) raw[(size_t) I.playMode]->load();
        if (pm != lastPlayMode)
        {
            if (lastPlayMode == playKeys) synth.allOff (false);
            else if (lastPlayMode == playDigga) keysForModules.addEvent (juce::MidiMessage::allNotesOff (1), 0);
            else if (lastPlayMode == playPair) pairPlayer.allOff();
            else drums[(size_t) juce::jlimit (0, 2, lastPlayMode - play808)].allOff();
            lastPlayMode = pm;
        }
        if (pm == playDigga) { keysForModules.addEvents (midi, 0, n, 0); midi.clear(); }
        else if (pm == playPair) {}   // routed after the loop player (keys + loop notes)
        else if (pm >= play808 && pm <= playHat)   // the keys play the boosted drum (808: in tune with its detected note)
        {
            auto& d = drums[(size_t) (pm - play808)];
            for (const auto meta : midi)
            {
                const auto m = meta.getMessage();
                if (m.isNoteOn()) d.noteOn (m.getNoteNumber(), m.getFloatVelocity(), juce::jlimit (0, n - 1, meta.samplePosition));
                else if (m.isNoteOff()) d.noteOff (m.getNoteNumber());
                else if (m.isAllNotesOff()) d.allOff();
            }
            midi.clear();
        }
    }
    processedMidi.clear();
    processMidi (midi, processedMidi, n, beatPos, bpm);
    lastBpm = bpm;
    renderLoop (processedMidi, n, beatPos, bps, hostPlaying);
    // PAIR YOUR OWN: keys and loop notes play the selected child instead of the synth
    if ((int) raw[(size_t) I.playMode]->load() == playPair)
    {
        for (const auto meta : processedMidi)
        {
            const auto m = meta.getMessage();
            if (m.isNoteOn()) pairPlayer.noteOn (m.getNoteNumber(), m.getFloatVelocity(), juce::jlimit (0, n - 1, meta.samplePosition));
            else if (m.isNoteOff()) pairPlayer.noteOff (m.getNoteNumber());
            else if (m.isAllNotesOff()) pairPlayer.allOff();
        }
        processedMidi.clear();
    }

    auto lfoInfo = [&] (int syncI, int divI, int rateI, int shapeI, float& phase, float& lastPh, float& sh, std::vector<float>& buf, int c0, int len)
    {
        const bool sync = raw[(size_t) syncI]->load() > 0.5f;
        const double beats = Choices::lfoDivBeats ((int) raw[(size_t) divI]->load());
        const float inc = raw[(size_t) rateI]->load() / (float) sr;
        const int shape = (int) raw[(size_t) shapeI]->load();
        for (int i = 0; i < len; ++i)
        {
            float ph;
            if (sync) { const double x = (beatPos + bps * (c0 + i)) / beats; ph = (float) (x - std::floor (x)); }
            else { ph = phase; phase += inc; if (phase >= 1) phase -= 1; }
            if (ph < lastPh) sh = shRng.bi();
            lastPh = ph;
            buf[(size_t) i] = kk::lfoShape (shape, ph, sh);
        }
    };

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    auto midiIt = processedMidi.cbegin();
    fxPtr->peakPre = 0;
    float peakL = 0, peakR = 0;
    const float bypassTarget = fadingOut ? 0.0f : 1.0f;
    if (presetJump.exchange (false)) dipPos = 0;
    const float bypassStep = 1.0f / (0.02f * (float) sr);

    for (int c0 = 0; c0 < n; c0 += kChunk)
    {
        const int len = std::min (kChunk, n - c0);
        std::fill_n (bufL.data(), len, 0.0f); std::fill_n (bufR.data(), len, 0.0f); std::fill_n (bufG.data(), len, 0.0f);
        lfoInfo (I.lfoSync, I.lfoDiv, I.lfoRate, I.lfoShape, lfoPhase, lastPh1, sh1, lfoBuf, c0, len);
        lfoInfo (I.lfo2Sync, I.lfo2Div, I.lfo2Rate, I.lfo2Shape, lfo2Phase, lastPh2, sh2, lfo2Buf, c0, len);

        int pos = 0;
        while (pos < len)
        {
            int next = len;
            while (midiIt != processedMidi.cend() && (*midiIt).samplePosition - c0 <= pos)
            {
                handleMidi ((*midiIt).getMessage());
                ++midiIt;
            }
            if (midiIt != processedMidi.cend()) next = std::min (len, (*midiIt).samplePosition - c0);
            if (next <= pos) next = pos + 1;
            synth.render (bufL.data() + pos, bufR.data() + pos, bufG.data() + pos, next - pos, vp,
                          lfoBuf.data() + pos, lfo2Buf.data() + pos);
            pos = next;
        }

        fp.beatPos = beatPos + bps * c0;
        worldStage.process (bufL.data(), bufR.data(), len, (int) raw[(size_t) I.world]->load(), raw[(size_t) I.worldAmt]->load(),
                            (int) raw[(size_t) I.gate]->load(), raw[(size_t) I.gateDepth]->load(), beatPos + bps * c0, bps);
        fxPtr->process (bufL.data(), bufR.data(), bufG.data(), len, fp);

        for (int i = 0; i < len; ++i)
        {
            bypassGain = bypassTarget > bypassGain ? std::min (1.0f, bypassGain + bypassStep) : std::max (bypassTarget, bypassGain - bypassStep);
            float dip = 1.0f;
            if (dipPos < 512) { dip = dipPos < 128 ? 1.0f - (float) dipPos / 128.0f : (float) (dipPos - 128) / 384.0f; ++dipPos; }
            const float l = bufL[(size_t) i] * bypassGain * dip, r = bufR[(size_t) i] * bypassGain * dip;
            outL[c0 + i] = l;
            if (outR) outR[c0 + i] = r;
            peakL = std::max (peakL, std::abs (l));
            peakR = std::max (peakR, std::abs (r));
        }
    }
    while (midiIt != processedMidi.cend()) { handleMidi ((*midiIt).getMessage()); ++midiIt; }
    sampleClock += n;

    if (buffer.getNumChannels() > 1) pairPlayer.render (buffer.getWritePointer (0), buffer.getWritePointer (1), n, sr);
    processModules (buffer, keysForModules, n);   // PAIR + DIGGA + HALF + EFFECTOR: the melody bus
    // DRUM BOOST: the drums play after the melody effects - HALF / EFFECTOR never touch them
    if (buffer.getNumChannels() > 1)
        for (int d = 0; d < 3; ++d)
        {
            if (const int v = drumPad[(size_t) d].exchange (0); v > 0)
            {
                drums[(size_t) d].noteOn (v - 1, 0.9f, 0);
                drumPadNote[(size_t) d] = v - 1; drumPadOff[(size_t) d] = (int) (sr * 2.5);
            }
            if (drumPadNote[(size_t) d] >= 0 && (drumPadOff[(size_t) d] -= n) <= 0) { drums[(size_t) d].noteOff (drumPadNote[(size_t) d]); drumPadNote[(size_t) d] = -1; }
            drums[(size_t) d].render (buffer.getWritePointer (0), buffer.getWritePointer (1), n);
        }
    // CLIPPER (v0.16): Soft (cubic knee), Hard (brickwall), Modern (asymmetric x - 0.2x^2 + 0.05x^3, warm even harmonics)
    const int clip = (int) raw[(size_t) I.clipMode]->load();
    const float drive = juce::Decibels::decibelsToGain (raw[(size_t) I.clipDrive]->load());
    for (int ch = 0; ch < std::min (2, buffer.getNumChannels()); ++ch)
    {
        auto* d = buffer.getWritePointer (ch);
        auto& dc = clipDc[(size_t) ch];
        for (int i = 0; i < n; ++i)
        {
            float x = d[i];
            if (clip == 1) { const float u = std::clamp (x * drive, -1.5f, 1.5f); x = (u - (4.0f / 27.0f) * u * u * u) * 0.966f; }
            else if (clip == 2) x = std::clamp (x * drive, -0.966f, 0.966f);
            else if (clip == 3)
            {
                float u = std::clamp (x * drive, -1.6f, 1.6f);
                u = u - 0.2f * u * u + 0.05f * u * u * u;
                const float o = u - dc[0] + 0.9995f * dc[1]; dc[0] = u; dc[1] = o;   // the asymmetry adds DC
                x = std::clamp (o, -0.966f, 0.966f);
            }
            d[i] = std::clamp (x, -1.0f, 1.0f);   // modules never push the output past 0 dBFS
        }
    }
    peakL = buffer.getMagnitude (0, 0, n);
    peakR = buffer.getNumChannels() > 1 ? buffer.getMagnitude (1, 0, n) : peakL;

    meterL = std::max (peakL, meterL.load()); meterR = std::max (peakR, meterR.load());
    if (fxPtr->peakPre > 1.0f) overload = true;

    std::array<bool, 128> act; synth.activeNotes (act);
    for (size_t i = 0; i < 128; ++i) playing[i].store (act[i], std::memory_order_relaxed);
}

//==============================================================================
// v0.17 modules: the melody bus = KEYS KILLA + DIGGA -> HALF (Voodoo Killa) -> EFFECTOR (Effector Killa).
// The modules are the original plugins (their own DSP, presets and design), hosted here.
void KeysKillaProcessor::runModule (int m, juce::AudioBuffer<float>& io, int len, juce::MidiBuffer& midi)
{
    auto* p = modules[(size_t) m].get();
    if (p == nullptr) return;
    p->setPlayHead (getPlayHead());
    float* chs[2] { io.getWritePointer (0), io.getWritePointer (1) };
    juce::AudioBuffer<float> view (chs, 2, len);
    const int need = std::max (p->getTotalNumInputChannels(), p->getTotalNumOutputChannels());
    if (need > 2) return;   // unexpected layout: never touch memory we do not own
    p->processBlock (view, midi);
}

void KeysKillaProcessor::processModules (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& keysMidi, int n)
{
    if (modules[modHalf] == nullptr || buffer.getNumChannels() < 2 || modBlock <= 0) return;
    const auto& I = *ix;
    const bool on[2] { raw[(size_t) I.halfOn]->load() > 0.5f, raw[(size_t) I.efxOn]->load() > 0.5f };
    const float step = 1.0f / (0.02f * (float) sr);
    for (int pos = 0; pos < n; pos += modBlock)
    {
        const int len = std::min (modBlock, n - pos);
        float* out[2] { buffer.getWritePointer (0) + pos, buffer.getWritePointer (1) + pos };
        juce::AudioBuffer<float> bus (out, 2, len);
        // DIGGA: its own sampler, notes from the keys when DIGGA plays the keys
        modMidi.clear();
        modMidi.addEvents (keysMidi, pos, len, -pos);
        modBuf.clear (0, len);
        runModule (modDigga, modBuf, len, modMidi);
        for (int c = 0; c < 2; ++c) bus.addFrom (c, 0, modBuf, c, 0, len);
        // HALF, then EFFECTOR - each with a short crossfade when it is switched on / off
        for (int k = 0; k < 2; ++k)
        {
            const int m = k == 0 ? modHalf : modEffector;
            // the plugins always run (their screens, meters and modulation live on the audio thread);
            // switched off they are faded out to the dry melody
            for (int c = 0; c < 2; ++c) modDry.copyFrom (c, 0, bus, c, 0, len);
            noMidi.clear();
            runModule (m, bus, len, noMidi);
            for (int i = 0; i < len; ++i)
            {
                float& f = modFade[(size_t) k];
                f = on[k] ? std::min (1.0f, f + step) : std::max (0.0f, f - step);
                for (int c = 0; c < 2; ++c)
                {
                    const float d = modDry.getSample (c, i);
                    float w = bus.getSample (c, i);
                    if (! std::isfinite (w)) w = d;
                    bus.setSample (c, i, d + (w - d) * f);
                }
            }
        }
    }
}

// ---------------- PAIR YOUR OWN ----------------
bool KeysKillaProcessor::loadPairParent (int slot, const juce::File& f)
{
    if (slot < 0 || slot >= kk::PairLab::maxParents) return false;
    auto s = kk::PairLab::fromFile (f, sr > 0 ? sr : 44100.0);
    if (s == nullptr) return false;
    pairParents[(size_t) slot] = s; pairFiles[(size_t) slot] = f.getFullPathName();
    ++pairVer;
    return true;
}

void KeysKillaProcessor::clearPairParent (int slot)
{
    if (slot < 0 || slot >= kk::PairLab::maxParents) return;
    pairParents[(size_t) slot] = nullptr; pairFiles[(size_t) slot].clear(); ++pairVer;
}

int KeysKillaProcessor::pairFromDigga()
{
    double rate = 44100.0;
    const auto shots = kkDiggaShots (modules[modDigga].get(), rate, false);
    int added = 0;
    size_t next = 0;
    for (int slot = 0; slot < kk::PairLab::maxParents && next < shots.size(); ++slot)
    {
        if (pairParents[(size_t) slot] != nullptr) continue;
        if (auto s = kk::PairLab::fromBuffer (*shots[next].second, rate, sr > 0 ? sr : 44100.0, "DIGGA " + shots[next].first))
        { pairParents[(size_t) slot] = s; pairFiles[(size_t) slot].clear(); ++added; }
        ++next;
    }
    ++pairVer;
    return added;
}

void KeysKillaProcessor::pairBreed()
{
    std::vector<kk::PairPtr> ps (pairParents.begin(), pairParents.end());
    pairSeed = kk::hash32 (pairSeed + (uint32_t) juce::Time::getMillisecondCounter());
    pairKids = kk::PairLab::breed (ps, pairSeed, pairFlavor, sr > 0 ? sr : 44100.0);
    pairSel = -1; pairLoopKid = -1;
    if (loopOn.load() && loopOwner == 2) loopOn = false;
    ++pairVer;
}

void KeysKillaProcessor::selectPairKid (int i, bool audition)
{
    if (i < 0 || i >= (int) pairKids.size()) return;
    pairSel = i;
    pairPlayer.setSound (pairKids[(size_t) i]);
    if (auto* q = apvts.getParameter (ID::playMode))
    {
        const float v = q->convertTo0to1 ((float) playPair);
        if (std::abs (q->getValue() - v) > 1.0e-6f) { q->beginChangeGesture(); q->setValueNotifyingHost (v); q->endChangeGesture(); }
    }
    if (audition) previewNote = pairKids[(size_t) i]->rootNote;
    ++pairVer;
}

void KeysKillaProcessor::togglePairLoop (int i)
{
    if (i < 0 || i >= (int) pairKids.size()) return;
    if (loopOn.load() && loopOwner == 2 && pairLoopKid == i) { loopOn = false; pairLoopKid = -1; ++pairVer; return; }
    selectPairKid (i, false);
    curLoop = kk::loopFromSeed (kk::hash32 ((uint32_t) juce::Time::getMillisecondCounter() * 31u + (uint32_t) i));
    loopOwner = 2; pairLoopKid = i;
    rebuildLoopSeq();
    loopOn = true;
    ++pairVer; ++labVer;
}

juce::File KeysKillaProcessor::exportPairKid (int i) const
{
    if (i < 0 || i >= (int) pairKids.size()) return {};
    return kk::PairLab::exportWav (*pairKids[(size_t) i], sr > 0 ? sr : 44100.0, "PAIR " + juce::String (i + 1));
}

juce::File KeysKillaProcessor::exportPairLoop() const
{
    Genome g; g.loop = curLoop; g.name = "PAIR " + (pairSel >= 0 && pairSel < (int) pairKids.size() ? pairKids[(size_t) pairSel]->name : juce::String ("loop"));
    return exportLoopMidi (g);
}

// ---------------- DRUM BOOST ----------------
kk::BoostParams KeysKillaProcessor::boostParams (int d) const
{
    auto P = [this, d] (int k) { return apvts.getRawParameterValue (ID::boost (d, k))->load(); };
    kk::BoostParams b;
    b.gain = P (0); b.pitch = P (1); b.punch = P (2); b.drive = P (3); b.sat = (int) P (4); b.clip = P (5);
    b.low = P (6); b.high = P (7); b.decay = P (8); b.room = P (9); b.width = P (10); b.deres = P (11);
    return b;
}

int KeysKillaProcessor::drumSignature (int d) const
{
    uint32_t h = 2166136261u;
    for (int k = 0; k < ID::boostKnobs.size(); ++k)
        h = (h ^ (uint32_t) std::lround (apvts.getRawParameterValue (ID::boost (d, k))->load() * 1000.0f)) * 16777619u;
    h = (h ^ (uint32_t) drums[(size_t) d].filePath().hashCode()) * 16777619u;
    return (int) (h & 0x7fffffff);
}

void KeysKillaProcessor::renderDrum (int d)
{
    drums[(size_t) d].render ((kk::DrumKind) d, boostParams (d));
    drumSig[(size_t) d] = drumSignature (d);
}

bool KeysKillaProcessor::loadDrum (int d, const juce::File& f)
{
    if (! drums[(size_t) d].load (f, sr > 0 ? sr : 44100.0)) return false;
    renderDrum (d);
    return true;
}

void KeysKillaProcessor::clearDrum (int d) { drums[(size_t) d].clear(); drumSig[(size_t) d] = -1; }

juce::File KeysKillaProcessor::exportDrum (int d) const
{
    static const char* tag[] { "808 BOOST", "SNARE BOOST", "HAT BOOST" };
    return drums[(size_t) d].exportWav (tag[juce::jlimit (0, 2, d)]);
}

void KeysKillaProcessor::hitDrum (int d, int note)
{
    if (note < 0) { auto s = drums[(size_t) d].current(); note = s ? s->rootNote : 60; }
    drumPad[(size_t) juce::jlimit (0, 2, d)] = note + 1;
}

void KeysKillaProcessor::moduleHousekeeping()
{
    // drums re-render a moment after a knob stops moving (keeps the knobs smooth)
    const double now = juce::Time::getMillisecondCounterHiRes();
    for (int d = 0; d < 3 && ix != nullptr; ++d)
        if (drums[(size_t) d].hasSample() && drumSignature (d) != drumSig[(size_t) d])
        {
            if (drumDirtyAt[(size_t) d] == 0) drumDirtyAt[(size_t) d] = now;
            if (now - drumDirtyAt[(size_t) d] > 90.0) { renderDrum (d); drumDirtyAt[(size_t) d] = 0; }
        }
    if (ix == nullptr || modules[modHalf] == nullptr) return;
    int lat = 0;
    if (raw[(size_t) ix->halfOn]->load() > 0.5f) lat += modules[modHalf]->getLatencySamples();
    if (raw[(size_t) ix->efxOn]->load() > 0.5f) lat += modules[modEffector]->getLatencySamples();
    if (lat != reportedLatency) { reportedLatency = lat; setLatencySamples (lat); }
}

void KeysKillaProcessor::rebuildRolls()
{
    const auto& I = *ix;
    auto P = [this] (int i) { return raw[(size_t) i]->load(); };
    const int bars = (int) P (I.rlBars) == 0 ? 1 : (int) P (I.rlBars) == 1 ? 2 : 4;
    auto pat = kk::makeRolls ((uint32_t) P (I.rlSeed), (int) P (I.rlStyle), bars, P (I.rlDensity));
    const juce::SpinLock::ScopedLockType l (rollLock);
    rolls.swap (pat);
}

std::vector<kk::RollHit> KeysKillaProcessor::rollPattern() const
{
    const juce::SpinLock::ScopedLockType l (rollLock);
    return rolls;
}

void KeysKillaProcessor::newRolls()
{
    auto* p = apvts.getParameter (ID::rlSeed);
    const int next = juce::Random::getSystemRandom().nextInt (99999) + 1;
    p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 ((float) next)); p->endChangeGesture();
    rebuildRolls();
}

juce::File KeysKillaProcessor::exportRollsMidi() const
{
    const auto pat = rollPattern();
    const double bpm = lastBpm.load();
    const int ppq = 960;
    juce::MidiMessageSequence seq;
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)); tempo.setTimeStamp (0); seq.addEvent (tempo);
    for (auto& h : pat)
    {
        const int note = juce::jlimit (0, 127, 60 + h.semi);
        seq.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, (int) std::lround (h.vel * 127.0f))), std::round (h.beat * ppq));
        seq.addEvent (juce::MidiMessage::noteOff (1, note), std::round ((h.beat + h.len) * ppq));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq); mf.addTrack (seq);
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Loops");
    dir.createDirectory();
    static const char* styles[] { "Classic", "Triplet", "Drill", "Crazy" };
    const int st = juce::jlimit (0, 3, (int) raw[(size_t) ix->rlStyle]->load());
    auto f = dir.getChildFile ("KK Rolls - " + juce::String (styles[st]) + " " + juce::String ((int) raw[(size_t) ix->rlSeed]->load()) + " - " + juce::String (juce::roundToInt (bpm)) + "BPM.mid");
    f.deleteFile();
    if (juce::FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
    return f;
}

//==============================================================================
// Builds the complete target state first and only tells the host about parameters that really change
// (a preset used to send ~480 notifications, which made hosts crawl or stall while switching sounds).
void KeysKillaProcessor::applyValues (const std::vector<std::pair<juce::String, float>>& values)
{
    std::vector<float> target (params.size());
    for (size_t i = 0; i < params.size(); ++i)
        target[i] = keepParam[i] ? params[i]->getValue() : params[i]->getDefaultValue();
    for (auto& [id, val] : values)
    {
        const auto it = idIndex.find (id);
        if (it != idIndex.end()) target[(size_t) it->second] = params[(size_t) it->second]->convertTo0to1 (val);
    }
    presetJump = true;
    for (size_t i = 0; i < params.size(); ++i)
        if (std::abs (params[i]->getValue() - target[i]) > 1.0e-6f)
            params[i]->setValueNotifyingHost (target[i]);
}

void KeysKillaProcessor::snapshotForModified()
{
    loadedSnapshot.resize (params.size());
    for (size_t i = 0; i < params.size(); ++i) loadedSnapshot[i] = params[i]->getValue();
}

void KeysKillaProcessor::resetParams (const juce::StringArray& ids)
{
    if (loadedSnapshot.size() != params.size()) return;
    for (auto& id : ids)
        if (auto it = idIndex.find (id); it != idIndex.end())
        {
            auto* prm = params[(size_t) it->second];
            const float v = loadedSnapshot[(size_t) it->second];
            if (std::abs (prm->getValue() - v) > 1.0e-6f) { prm->beginChangeGesture(); prm->setValueNotifyingHost (v); prm->endChangeGesture(); }
        }
}

bool KeysKillaProcessor::isModified() const
{
    if (loadedSnapshot.size() != params.size()) return false;
    for (size_t i = 0; i < params.size(); ++i)
        if (! keeps (params[i]->getParameterID()) && params[i]->getParameterID() != ID::morphX
            && params[i]->getParameterID() != ID::morphY && std::abs (params[i]->getValue() - loadedSnapshot[i]) > 1.0e-4f)
            return true;
    return false;
}

void KeysKillaProcessor::loadPreset (int index)
{
    const auto& ps = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size()) || loadingPreset) return;
    const juce::ScopedValueSetter<bool> guard (loadingPreset, true);   // host echoes of the program change are ignored
    const auto& pr = ps[(size_t) index];
    auto vals = pr.values;
    if (pr.isBass())
    {
        vals.insert (vals.begin(), { ID::bassMode, 1.0f });
        vals.insert (vals.begin(), { ID::mono, 1.0f });
    }
    bool setsArp = false;
    for (auto& v : vals) setsArp |= v.first == ID::arp;
    if (! setsArp && presetForcedArp) vals.push_back ({ ID::arp, 0.0f });   // leaving an ARPS preset switches the arp off again
    presetForcedArp = setsArp;
    applyValues (vals);
    currentPreset = index;
    userFile = juce::File();
    presetName = pr.name;
    macroLabels = pr.macroNames;
    snapshotForModified();
    setCurrentLoop (kk::loopFromSeed ((uint32_t) pr.name.hashCode())); loopOwner = -1;
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void KeysKillaProcessor::revert()
{
    if (currentPreset >= 0) loadPreset (currentPreset);
    else if (userFile.existsAsFile()) loadUserPreset (userFile);
}

void KeysKillaProcessor::initPatch()
{
    applyValues ({});
    currentPreset = -1; userFile = juce::File();
    presetName = "Init"; macroLabels.clear();
    snapshotForModified();
}

const juce::String KeysKillaProcessor::getProgramName (int index)
{
    const auto& ps = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) ps.size()) ? ps[(size_t) index].name : juce::String();
}

//==============================================================================
const char* KeysKillaProcessor::lockName (int i)
{
    static const char* n[] { "Engine", "Filter", "Envelopes", "Modulation", "FX", "Exclusive" };
    return n[juce::jlimit (0, (int) numLocks - 1, i)];
}

int KeysKillaProcessor::lockOf (const juce::String& id) const
{
    if (id.startsWith ("mm") || id.startsWith ("lfo") || id.startsWith ("e3") || id == ID::wobTarget) return lockMod;
    if (id == ID::cutoff || id == ID::reso || id == ID::fenv || id == ID::filterType || id == ID::keyTrack) return lockFilter;
    if (id == ID::attack || id == ID::decay || id == ID::sustain || id == ID::release || id.startsWith ("f")) return lockEnv;
    if (id == ID::ghost || id == ID::bend || id == ID::circuit || id == ID::body || id == ID::bodyMix || id.startsWith ("ghost") || id.startsWith ("bend")) return lockExclusive;
    if (id == ID::drive || id == ID::driveType || id == ID::crush || id == ID::wow || id == ID::chorus || id == ID::phaser || id == ID::flanger
        || id.startsWith ("delay") || id.startsWith ("rev") || id == ID::width || id.startsWith ("eq")) return lockFx;
    return lockEngine;
}

void KeysKillaProcessor::rollDice (int tile)
{
    diceHistory.push_back ({ presetName, apvts.copyState() });
    if (diceHistory.size() > 20) diceHistory.erase (diceHistory.begin());

    const bool bass = tile == cBass || tile == c808 || raw[(size_t) ix->bassMode]->load() > 0.5f;
    const float chaos = raw[(size_t) ix->chaos]->load();
    juce::Random r;

    struct R { juce::String id; float lo, hi; };
    std::vector<R> ranges {
        { ID::wave, 0, 1 }, { ID::detune, 0, 0.7f }, { ID::unison, 0, 0.6f }, { ID::fmRatio, 0, 0.6f }, { ID::fmAmt, 0, 0.7f },
        { ID::fmRatio2, 0, 0.6f }, { ID::warpMode, 0, 1 }, { ID::fmAlgo, 0, 1 },
        { ID::cutoff, 0.4f, 0.95f }, { ID::reso, 0, 0.55f }, { ID::fenv, 0.4f, 0.9f }, { ID::filterType, 0, 0.6f },
        { ID::fdecay, 0.1f, 0.6f }, { ID::attack, 0, 0.25f }, { ID::decay, 0.2f, 0.8f }, { ID::sustain, 0, 1 }, { ID::release, 0.1f, 0.55f },
        { ID::lfoRate, 0.2f, 0.8f }, { ID::lfoPitch, 0, 0.12f }, { ID::lfoFilter, 0, bass ? 0.8f : 0.4f },
        { ID::drive, 0, 0.5f }, { ID::driveType, 0, 1 }, { ID::crush, 0, 0.35f }, { ID::wow, 0, 0.4f },
        { ID::chorus, 0, bass ? 0.0f : 0.6f }, { ID::phaser, 0, bass ? 0.0f : 0.3f },
        { ID::delayMix, 0, bass ? 0.05f : 0.35f }, { ID::revMix, 0, bass ? 0.08f : 0.5f },
        { ID::revSize, 0.2f, 0.9f }, { ID::width, 0.3f, bass ? 0.5f : 0.8f },
        { ID::ghost, 0, bass ? 0.1f : 0.5f }, { ID::bend, 0, 0.35f }, { ID::circuit, 0, 0.25f } };

    for (auto& rg : ranges)
    {
        if (diceLocks[(size_t) lockOf (rg.id)]) continue;
        if (auto* p = apvts.getParameter (rg.id))
        {
            const float cur = p->getValue();
            const float target = rg.lo + r.nextFloat() * (rg.hi - rg.lo);
            p->setValueNotifyingHost (cur + (target - cur) * chaos);
        }
    }

    if (! diceLocks[lockEngine] && chaos > 0.6f && r.nextFloat() < chaos - 0.4f)
    {
        static const std::vector<std::vector<int>> engs {   // per Category
            { engFM, engPluck }, { engFM, engOrgan, engWavetable }, { engFM, engModal, engWavetable }, { engPluck, engVA, engFM, engWavetable },
            { engModal, engFM }, { engPluck }, { engOrchestral, engVA }, { engOrchestral, engVA }, { engVox, engOrchestral }, { engFlute, engVA },
            { engVA, engWavetable, engFM }, { engVA, engWavetable, engVox, engOrchestral }, { engVA, engFM, engWavetable }, { engVA, engSub, engFM },
            { engSub }, { engFlute, engWavetable, engVox, engOrgan }, { engFM, engPluck, engVA, engModal }, { engVA, engFM, engWavetable } };
        const auto& list = engs[(size_t) juce::jlimit (0, (int) engs.size() - 1, tile)];
        auto* p = apvts.getParameter (ID::engine);
        p->setValueNotifyingHost (p->convertTo0to1 ((float) list[(size_t) r.nextInt ((int) list.size())]));
    }
    if (auto* p = apvts.getParameter (ID::seed)) p->setValueNotifyingHost (r.nextFloat());
    if (bass) { apvts.getParameter (ID::mono)->setValueNotifyingHost (1.0f); apvts.getParameter (ID::bassMode)->setValueNotifyingHost (1.0f); }

    presetName = "DICE #" + juce::String (++diceCount);
    currentPreset = -1; userFile = juce::File();
    snapshotForModified();
}

// BREED: child of the current sound (A) and a factory preset (B). Each DNA section comes from one parent,
// continuous values inside it are blended slightly toward the other. Dice locks keep A's section.
void KeysKillaProcessor::breedWith (int presetIndex)
{
    const auto& ps = factoryPresets();
    if (! juce::isPositiveAndBelow (presetIndex, (int) ps.size())) return;
    diceHistory.push_back ({ presetName, apvts.copyState() });
    if (diceHistory.size() > 20) diceHistory.erase (diceHistory.begin());

    const auto& pr = ps[(size_t) presetIndex];
    std::vector<float> b (params.size());
    for (size_t i = 0; i < params.size(); ++i) b[i] = params[i]->getDefaultValue();
    for (auto& [id, v] : pr.values)
        if (auto it = idIndex.find (id); it != idIndex.end()) b[(size_t) it->second] = params[(size_t) it->second]->convertTo0to1 (v);

    juce::Random r ((juce::int64) presetIndex * 7919 + (juce::int64) presetName.hashCode());
    std::array<bool, numLocks> fromB {};
    bool anyB = false, anyA = false;
    for (int s = 0; s < numLocks; ++s) { fromB[(size_t) s] = ! diceLocks[(size_t) s] && r.nextBool(); anyB |= fromB[(size_t) s]; anyA |= ! fromB[(size_t) s]; }
    if (! anyB && ! diceLocks[lockFilter]) fromB[lockFilter] = true;
    if (! anyA && ! diceLocks[lockFx]) fromB[lockFx] = false;
    const float blend = 0.25f;

    presetJump = true;
    for (size_t i = 0; i < params.size(); ++i)
    {
        if (keepParam[i]) continue;
        const auto id = params[i]->paramID;
        if (id == ID::mono || id == ID::bassMode) continue;
        const int sec = lockOf (id);
        if (diceLocks[(size_t) sec]) continue;
        const float a = params[i]->getValue(), bv = b[i];
        float t = fromB[(size_t) sec] ? bv : a;
        if (! discrete[i]) t = fromB[(size_t) sec] ? bv + (a - bv) * blend : a + (bv - a) * blend;
        if (std::abs (t - a) > 1.0e-6f) params[i]->setValueNotifyingHost (t);
    }
    auto shortName = [] (juce::String n) { return n.upToFirstOccurrenceOf (" 20", false, false).substring (0, 14).trim(); };
    presetName = shortName (presetName) + " x " + shortName (pr.name);
    currentPreset = -1; userFile = juce::File();
    snapshotForModified();
}

bool KeysKillaProcessor::undoDice()
{
    if (diceHistory.empty()) return false;
    restoreDice ((int) diceHistory.size() - 1);
    return true;
}

void KeysKillaProcessor::restoreDice (int h)
{
    if (! juce::isPositiveAndBelow (h, (int) diceHistory.size())) return;
    auto e = diceHistory[(size_t) h];
    apvts.replaceState (e.state.createCopy());
    syncParamsToState();
    presetName = e.name;
    diceHistory.erase (diceHistory.begin() + h, diceHistory.end());
    currentPreset = -1;
    snapshotForModified();
}

juce::StringArray KeysKillaProcessor::diceHistoryNames() const
{
    juce::StringArray s;
    for (auto& e : diceHistory) s.add (e.name);
    return s;
}


//==============================================================================
// BREED LAB: two parents -> six children. A child takes each of its six genes from one parent,
// continuous values drift a little toward the other parent, some children get a mutation.
const char* KeysKillaProcessor::geneName (int g)
{
    static const char* n[] { "BODY", "ATTACK", "TEXTURE", "SPACE", "MOVEMENT", "CHARACTER" };
    return n[juce::jlimit (0, (int) numGenes - 1, g)];
}

KeysKillaProcessor::Genome KeysKillaProcessor::genomeFromPreset (int idx) const
{
    Genome g;
    const auto& ps = factoryPresets();
    if (! juce::isPositiveAndBelow (idx, (int) ps.size())) return g;
    const auto& pr = ps[(size_t) idx];
    g.name = pr.name; g.cat = pr.cat; g.era = pr.era; g.preset = idx;
    g.v.resize (params.size());
    for (size_t i = 0; i < params.size(); ++i) g.v[i] = params[i]->getDefaultValue();
    auto setv = [&] (const juce::String& id, float x) { if (auto it = idIndex.find (id); it != idIndex.end()) g.v[(size_t) it->second] = params[(size_t) it->second]->convertTo0to1 (x); };
    for (auto& [id, x] : pr.values) setv (id, x);
    if (pr.isBass()) { setv (ID::mono, 1); setv (ID::bassMode, 1); }
    g.loop = kk::loopFromSeed ((uint32_t) pr.name.hashCode());
    return g;
}

KeysKillaProcessor::Genome KeysKillaProcessor::genomeFromCurrent() const
{
    Genome g;
    g.name = presetName; g.v = snapshot();
    g.cat = currentPreset >= 0 ? factoryPresets()[(size_t) currentPreset].cat : -1;
    g.era = (int) std::round (params[(size_t) ix->eraHome]->convertFrom0to1 (g.v[(size_t) ix->eraHome]));
    for (auto& c : children) if (c.g.name == presetName) { g.gen = c.g.gen; g.cat = c.g.cat; }
    g.loop = curLoop.valid ? curLoop : kk::loopFromSeed ((uint32_t) presetName.hashCode());
    return g;
}

void KeysKillaProcessor::setParentPreset (int slot, int idx)
{
    auto g = genomeFromPreset (idx);
    if (! g.valid()) return;
    parents[(size_t) juce::jlimit (0, 1, slot)] = std::move (g); ++labVer;
}
void KeysKillaProcessor::setParentCurrent (int slot) { parents[(size_t) juce::jlimit (0, 1, slot)] = genomeFromCurrent(); ++labVer; }
void KeysKillaProcessor::setParentChild (int slot, int c)
{
    if (! juce::isPositiveAndBelow (c, (int) children.size())) return;
    parents[(size_t) juce::jlimit (0, 1, slot)] = children[(size_t) c].g; ++labVer;
}
void KeysKillaProcessor::randomParent (int slot)
{
    juce::Random r ((juce::int64) (juce::Time::getHighResolutionTicks() ^ (juce::int64) (slot * 7919)));
    setParentPreset (slot, r.nextInt ((int) factoryPresets().size()));
}
void KeysKillaProcessor::stepParent (int slot, int dir)
{
    const auto& p = parents[(size_t) juce::jlimit (0, 1, slot)];
    const auto& ps = factoryPresets();
    const int n = (int) ps.size();
    int i = p.preset >= 0 ? p.preset : 0;
    for (int k = 0; k < n; ++k)   // next preset of the same category
    {
        i = ((i + dir) % n + n) % n;
        if (p.cat < 0 || ps[(size_t) i].cat == p.cat) break;
    }
    setParentPreset (slot, i);
}

KeysKillaProcessor::Child KeysKillaProcessor::makeChild (int k, uint32_t seed, const std::array<int, numGenes>* forced) const
{
    return makeChildOf (parents[0], parents[1], k, seed, forced, true);
}

KeysKillaProcessor::Child KeysKillaProcessor::makeChildOf (const Genome& pa, const Genome& pb, int k, uint32_t seed,
                                                           const std::array<int, numGenes>* forced, bool useLocks) const
{
    std::array<bool, numGenes> geneLock {};   // FAMILY TREE breeds ignore the main page gene locks
    if (useLocks) geneLock = this->geneLock;
    Child c; c.seed = seed;
    const auto& A = pa.v; const auto& B = pb.v;
    kk::Rng rng; rng.seed (seed);
    static const float pA[6] { 0.85f, 0.15f, 0.5f, 0.5f, 0.7f, 0.3f };       // how much each child leans to parent A
    static const float blend[6] { 0.18f, 0.18f, 0.3f, 0.25f, 0.12f, 0.12f };  // drift toward the other parent
    static const float mutation[6] { 0.0f, 0.0f, 0.0f, 0.07f, 0.12f, 0.2f };
    const int kk_ = juce::jlimit (0, 5, k);
    const float wild = juce::jlimit (0.0f, 1.0f, breedWild);
    if (forced != nullptr) c.genes = *forced;
    else
    {
        bool fromA = false, fromB = false;
        for (int g = 0; g < numGenes; ++g)
        {
            c.genes[(size_t) g] = geneLock[(size_t) g] ? geneLockSrc[(size_t) g] : (rng.uni() < pA[kk_] ? 0 : 1);
            (c.genes[(size_t) g] == 0 ? fromA : fromB) = true;
        }
        if (! fromA || ! fromB)   // a child is always a mix of both parents
            for (int tries = 0; tries < 12; ++tries)
            {
                const int g = (int) (rng.uni() * numGenes) % numGenes;
                if (geneLock[(size_t) g]) continue;
                c.genes[(size_t) g] = fromA ? 1 : 0; break;
            }
    }
    c.g.v.resize (params.size());
    for (size_t i = 0; i < params.size(); ++i)
    {
        const int gene = geneOfParam[i];
        if (gene < 0) { c.g.v[i] = params[i]->getValue(); continue; }   // performance settings stay as they are
        bool srcA = c.genes[(size_t) gene] == 0;
        const bool locked = geneLock[(size_t) gene];
        if (! locked && rng.uni() < wild * 0.45f) srcA = rng.uni() < 0.5f;   // WILD: single parameters jump parents
        const float own = srcA ? A[i] : B[i], other = srcA ? B[i] : A[i];
        float x = own;
        const float mut = locked ? 0.0f : mutation[kk_] + wild * 0.22f;
        if (! discrete[i])
        {
            x += (other - own) * blend[kk_] * (1.0f + wild) * (0.5f + 0.5f * rng.uni());
            if (mut > 0) x += (rng.uni() * 2.0f - 1.0f) * mut;
        }
        else if (! locked && rng.uni() < wild * wild * 0.25f)
            x = rng.uni();   // WILD: switches (engine, filter type, drive type...) can flip
        c.g.v[i] = juce::jlimit (0.0f, 1.0f, x);
    }
    auto real = [&] (int idx) { return params[(size_t) idx]->convertFrom0to1 (c.g.v[(size_t) idx]); };
    auto setReal = [&] (int idx, float x) { c.g.v[(size_t) idx] = params[(size_t) idx]->convertTo0to1 (x); };
    const auto& I = *ix;
    // HYBRID child: the other parent's sound source becomes layer B (child 6 always, more when WILD)
    const bool hybrid = forced == nullptr ? (k == 5 || (k >= 3 && wild > 0.55f)) : hybridHint;
    c.hybrid = hybrid;
    if (hybrid)
    {
        const auto& other = c.genes[geneBody] == 0 ? B : A;
        const int from[] { I.engine, I.octave, I.semi, I.fine, I.wave, I.unison, I.detune, I.fmRatio, I.fmRatio2, I.fmAmt, I.fmAlgo, I.warpMode };
        const int to[]   { I.engineB, I.octaveB, I.semiB, I.fineB, I.waveB, I.unisonB, I.detuneB, I.fmRatioB, I.fmRatio2B, I.fmAmtB, I.fmAlgoB, I.warpModeB };
        for (int q = 0; q < 12; ++q)
            c.g.v[(size_t) to[q]] = params[(size_t) to[q]]->convertTo0to1 (params[(size_t) from[q]]->convertFrom0to1 (other[(size_t) from[q]]));
        setReal (I.layerB, 1.0f);
        setReal (I.levelB, 0.55f + 0.2f * rng.uni());
    }
    if (wild > 0.7f && k >= 3 && ! geneLock[geneCharacter]) setReal (I.future, std::max (real (I.future), (wild - 0.7f) * 2.0f * rng.uni()));
    // keep children playable
    const bool bass = real (I.bassMode) > 0.5f;
    if (bass)   // clean, mono low end
    {
        setReal (I.revMix, std::min (real (I.revMix), 0.12f)); setReal (I.delayMix, std::min (real (I.delayMix), 0.1f));
        setReal (I.width, std::min (real (I.width), 0.55f)); setReal (I.reverse, 0.0f); setReal (I.ghost, std::min (real (I.ghost), 0.1f));
        setReal (I.cutoff, std::max (real (I.cutoff), 250.0f)); setReal (I.freeze, 0.0f);
    }
    if (real (I.sustain) < 0.02f && real (I.decay) < 0.08f) setReal (I.decay, 0.25f);   // never a silent click
    // loudness: the body parent's level, a bit less when the child is dirtier than it
    const auto& body = c.genes[geneBody] == 0 ? A : B;
    const float bodyGain = params[(size_t) I.gain]->convertFrom0to1 (body[(size_t) I.gain]);
    const float dirt = std::max (0.0f, real (I.drive) - params[(size_t) I.drive]->convertFrom0to1 (body[(size_t) I.drive]));
    setReal (I.gain, juce::jlimit (-24.0f, 12.0f, bodyGain - 6.0f * dirt));

    auto shortName = [] (juce::String n, bool lastFamily)
    {
        n = n.upToFirstOccurrenceOf (juce::String::fromUTF8 (" \xc2\xb7 GEN"), false, false);
        const auto x = juce::String::fromUTF8 (" \xc3\x97 ");   // a bred parent: A keeps its first family, B its last
        if (n.contains (x)) n = lastFamily ? n.fromLastOccurrenceOf (x, false, false) : n.upToFirstOccurrenceOf (x, false, false);
        if (n.length() > 5 && n.substring (0, 4).containsOnly ("0123456789")) n = n.substring (5);   // drop the year tag
        if (n.startsWith ("FUTURE ")) n = n.substring (7);
        auto words = juce::StringArray::fromTokens (n, " ", "");
        if (words.size() > 1 && words[words.size() - 1].length() == 4 && words[words.size() - 1].containsOnly ("0123456789")) words.remove (words.size() - 1);
        juce::String out;
        for (auto& w : words) { if ((out + " " + w).trim().length() > 20) break; out = (out + " " + w).trim(); }
        return out.isEmpty() ? n.substring (0, 20) : out;
    };
    c.g.loop = kk::crossLoops (pa.loop, pb.loop, kk::hash32 (seed ^ 0x10095u), pA[kk_], wild);
    c.g.gen = std::max (pa.gen, pb.gen) + 1;
    c.g.cat = (c.genes[geneBody] == 0 ? pa : pb).cat;
    c.g.era = (c.genes[geneCharacter] == 0 ? pa : pb).era;
    c.g.name = shortName (pa.name, false) + juce::String::fromUTF8 (" \xc3\x97 ") + shortName (pb.name, true) + juce::String::fromUTF8 (" \xc2\xb7 GEN ") + juce::String (c.g.gen) + " #" + juce::String (k + 1);
    return c;
}

int KeysKillaProcessor::breed()
{
    if (! parents[0].valid()) setParentCurrent (0);
    if (! parents[1].valid()) randomParent (1);
    if (! children.empty()) { history.push_back ({ { parents[0], parents[1] }, children }); if (history.size() > 30) history.erase (history.begin()); }
    ++breedCount;
    const uint32_t base = kk::hash32 (((uint32_t) parents[0].name.hashCode() * 31u + (uint32_t) parents[1].name.hashCode()) ^ (breedCount * 2654435761u));
    children.clear();
    for (int k = 0; k < 6; ++k) children.push_back (makeChild (k, kk::hash32 (base + (uint32_t) k * 7919u), nullptr));
    ++labVer;
    selectChild (0);
    return (int) children.size();
}

void KeysKillaProcessor::applyGenome (const Genome& g, bool asPreset)
{
    if (! g.valid()) return;
    presetJump = true;
    for (size_t i = 0; i < params.size(); ++i)
        if (geneOfParam[i] >= 0 && std::abs (params[i]->getValue() - g.v[i]) > 1.0e-6f) params[i]->setValueNotifyingHost (g.v[i]);
    if (asPreset)
    {
        presetName = g.name; currentPreset = -1; userFile = juce::File(); macroLabels.clear();
        snapshotForModified();
        updateHostDisplay (ChangeDetails().withProgramChanged (true));
    }
}

void KeysKillaProcessor::selectChild (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) children.size())) return;
    selChild = i; ++labVer;
    applyGenome (children[(size_t) i].g, true);
    setCurrentLoop (children[(size_t) i].g.loop);
    loopOwner = -1;
    captureUndo();
}

void KeysKillaProcessor::setChildGene (int c, int gene, int src)
{
    if (! juce::isPositiveAndBelow (c, (int) children.size()) || ! juce::isPositiveAndBelow (gene, (int) numGenes)) return;
    auto genes = children[(size_t) c].genes;
    genes[(size_t) gene] = juce::jlimit (0, 1, src);
    const int rating = children[(size_t) c].rating;
    const auto loop = children[(size_t) c].g.loop;
    hybridHint = children[(size_t) c].hybrid;
    children[(size_t) c] = makeChild (c, children[(size_t) c].seed, &genes);
    children[(size_t) c].rating = rating;
    children[(size_t) c].g.loop = loop;
    if (geneLock[(size_t) gene]) geneLockSrc[(size_t) gene] = genes[(size_t) gene];
    selectChild (c);
}

void KeysKillaProcessor::toggleGeneLock (int gene)
{
    if (! juce::isPositiveAndBelow (gene, (int) numGenes)) return;
    geneLock[(size_t) gene] = ! geneLock[(size_t) gene];
    if (juce::isPositiveAndBelow (selChild, (int) children.size())) geneLockSrc[(size_t) gene] = children[(size_t) selChild].genes[(size_t) gene];
    ++labVer;
}

void KeysKillaProcessor::rateChild (int c, int stars)
{
    if (! juce::isPositiveAndBelow (c, (int) children.size())) return;
    auto& ch = children[(size_t) c];
    const bool wasGood = ch.rating >= 4;
    ch.rating = ch.rating == stars ? 0 : juce::jlimit (0, 5, stars);
    ++labVer;
    if (ch.rating >= 4 && ! wasGood)   // favourite children are kept as user presets
    {
        const auto keep = snapshot(); const auto keepName = presetName; const int keepPreset = currentPreset; const auto keepFile = userFile;
        applyGenome (ch.g, true);
        auto f = userPresetDir().getChildFile ("Bred").getChildFile (juce::File::createLegalFileName (ch.g.name) + ".kkpreset");
        f.getParentDirectory().createDirectory();
        saveUserPreset (f);
        applySnapshot (keep); presetName = keepName; currentPreset = keepPreset; userFile = keepFile;
        snapshotForModified();
    }
}

void KeysKillaProcessor::restoreGeneration (int h)
{
    if (! juce::isPositiveAndBelow (h, (int) history.size())) return;
    if (! children.empty()) history.push_back ({ { parents[0], parents[1] }, children });
    auto gen = history[(size_t) h];
    history.erase (history.begin() + h);
    parents[0] = gen.parents[0]; parents[1] = gen.parents[1]; children = gen.kids;
    ++labVer;
    selectChild (0);
}

void KeysKillaProcessor::previewChild (int i)
{
    if (i != selChild) selectChild (i);
    previewNote = raw[(size_t) ix->bassMode]->load() > 0.5f ? 36 : 60;
}

//==============================================================================
// BREED LOOPS
int KeysKillaProcessor::effectiveLoopKey (const kk::LoopGenes& l) const
{
    if (loopKeyN >= 0) return loopKeyN;
    if (raw[(size_t) ix->keyLock]->load() > 0.5f) return (int) raw[(size_t) ix->key]->load();
    return l.key;
}

std::vector<kk::LoopNote> KeysKillaProcessor::loopNotes (const Genome& g) const
{
    const bool bass = g.v.size() == params.size() && g.v[(size_t) ix->bassMode] > 0.5f;
    const bool mono = g.v.size() == params.size() && g.v[(size_t) ix->mono] > 0.5f;
    return kk::buildLoop (g.loop, effectiveLoopKey (g.loop), loopBarsN, bass, mono && ! bass);
}

void KeysKillaProcessor::rebuildLoopSeq()
{
    const bool pair = (int) raw[(size_t) ix->playMode]->load() == playPair;
    const bool bass = ! pair && raw[(size_t) ix->bassMode]->load() > 0.5f;
    const bool mono = ! pair && raw[(size_t) ix->mono]->load() > 0.5f;
    auto seq = kk::buildLoop (curLoop, effectiveLoopKey (curLoop), loopBarsN, bass, mono && ! bass);
    {
        const juce::SpinLock::ScopedLockType sl (loopLock);
        loopSeq.swap (seq);
        loopLenBeats = loopBarsN * 4.0;
    }
    loopDirty = true;
}

void KeysKillaProcessor::setCurrentLoop (const kk::LoopGenes& l)
{
    if (l == curLoop && ! loopSeq.empty()) { rebuildLoopSeq(); return; }
    curLoop = l;
    rebuildLoopSeq();
}

void KeysKillaProcessor::toggleLoop()
{
    if (! curLoop.valid) curLoop = kk::loopFromSeed ((uint32_t) presetName.hashCode());
    if (! loopOn) rebuildLoopSeq();
    loopOn = ! loopOn.load();
    ++labVer;
}
void KeysKillaProcessor::setLoopBars (int bars) { loopBarsN = bars > 8 ? 16 : 8; rebuildLoopSeq(); ++labVer; }
void KeysKillaProcessor::setLoopKey (int key) { loopKeyN = juce::jlimit (-1, 11, key); rebuildLoopSeq(); ++labVer; }


void KeysKillaProcessor::renderLoop (juce::MidiBuffer& out, int n, double beatPos, double bps, bool hostPlaying)
{
    auto allOff = [&]
    {
        for (int i = 0; i < 128; ++i)
            if (loopActive[(size_t) i]) { out.addEvent (juce::MidiMessage::noteOff (1, i), 0); loopActive[(size_t) i] = false; }
    };
    if (! loopOn.load())
    {
        if (loopRunning) { allOff(); loopRunning = false; loopBeat = -1.0f; }
        return;
    }
    if (loopDirty.exchange (false)) allOff();
    if (! loopRunning || hostPlaying != loopHostWas)
    {
        allOff();
        loopOrigin = hostPlaying ? 0.0 : beatPos;   // host playing: bar-synced to the song, stopped: start now
        loopRunning = true; loopHostWas = hostPlaying;
    }
    const juce::SpinLock::ScopedTryLockType sl (loopLock);
    if (! sl.isLocked() || loopSeq.empty() || bps <= 0.0) return;
    const double len = loopLenBeats;
    const double b0 = beatPos - loopOrigin, b1 = b0 + bps * n;
    struct Ev { int at, note; bool on; };
    std::array<Ev, 96> ev; int evN = 0;
    for (auto& nt : loopSeq)
        for (int edge = 0; edge < 2 && evN < (int) ev.size(); ++edge)
        {
            const double t = edge == 0 ? nt.start : nt.start + nt.len;
            for (double tt = t + std::ceil ((b0 - t) / len) * len; tt < b1 && evN < (int) ev.size(); tt += len)
                ev[(size_t) evN++] = { juce::jlimit (0, n - 1, (int) ((tt - b0) / bps)), nt.note, edge == 0 };
        }
    std::sort (ev.begin(), ev.begin() + evN, [] (const Ev& a, const Ev& b) { return a.at != b.at ? a.at < b.at : (! a.on && b.on); });
    for (int i = 0; i < evN; ++i)
    {
        const auto& e = ev[(size_t) i];
        auto& act = loopActive[(size_t) juce::jlimit (0, 127, e.note)];
        if (! e.on) { if (act) { out.addEvent (juce::MidiMessage::noteOff (1, e.note), e.at); act = false; } continue; }
        if (act) out.addEvent (juce::MidiMessage::noteOff (1, e.note), e.at);
        out.addEvent (juce::MidiMessage::noteOn (1, e.note, (juce::uint8) 100), e.at);
        act = true;
    }
    const double pos = std::fmod (b1, len);
    loopBeat = (float) (pos < 0 ? pos + len : pos);
}

// DRAG TO DAW: render the current sound (one note, until it dies out) offline into a 24-bit WAV
juce::AudioBuffer<float> KeysKillaProcessor::renderSound (int note, int presetIndex)
{
    juce::MemoryBlock mb; getStateInformation (mb);
    KeysKillaProcessor r (false);
    r.setStateInformation (mb.getData(), (int) mb.getSize());
    if (presetIndex >= 0) r.loadPreset (presetIndex);
    r.apvts.getParameter (ID::playMode)->setValueNotifyingHost (0.0f);   // the synth itself, not a module
    const double rate = 44100.0; const int block = 512;
    r.prepareToPlay (rate, block);
    juce::AudioBuffer<float> out (2, (int) (rate * 8.0)), b (2, block);
    out.clear();
    int pos = 0, quiet = 0;
    const int holdEnd = (int) (rate * 1.6);
    while (pos + block <= out.getNumSamples())
    {
        juce::MidiBuffer m;
        if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), 0);
        if (pos <= holdEnd && pos + block > holdEnd) m.addEvent (juce::MidiMessage::noteOff (1, note), holdEnd - pos);
        r.processBlock (b, m);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, b, c, 0, block);
        pos += block;
        if (pos > holdEnd) { quiet = b.getMagnitude (0, block) < 1.0e-4f ? quiet + block : 0; if (quiet > (int) (rate * 0.15)) break; }
    }
    out.setSize (2, std::max (block, pos - quiet), true);
    return out;
}

// PAIR YOUR OWN dice: a sound appears out of nowhere (a random KEYS KILLA sound, rendered to a sample)
juce::String KeysKillaProcessor::pairDice (int slot)
{
    if (slot < 0 || slot >= kk::PairLab::maxParents) return {};
    const int idx = juce::Random::getSystemRandom().nextInt ((int) factoryPresets().size());
    auto buf = renderSound (60, idx);
    const juce::String name = factoryPresets()[(size_t) idx].name;
    pairParents[(size_t) slot] = kk::PairLab::fromBuffer (buf, 44100.0, sr > 0 ? sr : 44100.0, name);
    pairFiles[(size_t) slot].clear();
    ++pairVer;
    return name;
}

juce::File KeysKillaProcessor::exportSoundWav (int note)
{
    const double rate = 44100.0;
    auto out = renderSound (note, -1);

    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Sounds");
    dir.createDirectory();
    auto f = dir.getChildFile (juce::File::createLegalFileName ("KK " + getProgramName (getCurrentProgram()).replace (juce::String::fromUTF8 ("\xc3\x97"), "x").replace (juce::String::fromUTF8 ("\xc2\xb7"), "-")).substring (0, 100)
                               + " " + juce::MidiMessage::getMidiNoteName (note, true, true, 4) + ".wav");
    f.deleteFile();
    juce::WavAudioFormat wav;
    auto os = std::make_unique<juce::FileOutputStream> (f);
    if (os->openedOk())
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), rate, 2, 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
        }
    return f;
}

juce::File KeysKillaProcessor::exportLoopMidi (const Genome& g) const
{
    const auto notes = loopNotes (g);
    const double bpm = lastBpm.load();
    const int ppq = 96;
    juce::MidiMessageSequence seq;
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)); tempo.setTimeStamp (0); seq.addEvent (tempo);
    auto name = g.name.replace (juce::String::fromUTF8 ("\xc3\x97"), "x").replace (juce::String::fromUTF8 ("\xc2\xb7"), "-");
    auto title = juce::MidiMessage::textMetaEvent (3, "KEYS KILLA loop"); title.setTimeStamp (0); seq.addEvent (title);
    for (auto& nt : notes)
    {
        seq.addEvent (juce::MidiMessage::noteOn (1, nt.note, (juce::uint8) 100), std::round (nt.start * ppq));
        seq.addEvent (juce::MidiMessage::noteOff (1, nt.note), std::round ((nt.start + nt.len) * ppq));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq); mf.addTrack (seq);
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Loops");
    dir.createDirectory();
    const juce::String file = "KK Loop - " + name.trim() + " - " + kk::keyName (effectiveLoopKey (g.loop)) + " MIN " + juce::String (juce::roundToInt (bpm)) + "BPM";
    auto f = dir.getChildFile (juce::File::createLegalFileName (file).substring (0, 120) + ".mid");
    f.deleteFile();
    if (juce::FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
    return f;
}

void KeysKillaProcessor::renderWave (const Genome& g, std::array<float, 64>& wave)
{
    if (thumbRenderer == nullptr) thumbRenderer = std::make_unique<KeysKillaProcessor> (false);
    auto& r = *thumbRenderer;
    for (size_t i = 0; i < r.params.size() && i < g.v.size(); ++i)
        if (std::abs (r.params[i]->getValue() - g.v[i]) > 1.0e-6f) r.params[i]->setValueNotifyingHost (g.v[i]);
    r.eco = true;
    const double rate = 16000.0; const int block = 400, total = 12000;
    r.prepareToPlay (rate, block);
    const int note = r.raw[(size_t) r.ix->bassMode]->load() > 0.5f ? 36 : 60;
    juce::AudioBuffer<float> buf (2, block);
    wave.fill (0.0f);
    float peak = 1.0e-6f;
    for (int pos = 0; pos < total; pos += block)
    {
        juce::MidiBuffer m;
        if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        if (pos == 7200) m.addEvent (juce::MidiMessage::noteOff (1, note), 0);
        r.processBlock (buf, m);
        for (int s = 0; s < block; ++s)
        {
            const float x = std::abs (buf.getSample (0, s)) + std::abs (buf.getSample (1, s));
            auto& bin = wave[(size_t) std::min (63, (pos + s) * 64 / total)];
            bin = std::max (bin, x); peak = std::max (peak, x);
        }
    }
    for (auto& b : wave) b /= peak;
}

bool KeysKillaProcessor::renderNextThumbnail()
{
    for (auto& c : children)
        if (! c.waveReady) { renderWave (c.g, c.wave); c.waveReady = true; ++labVer; return true; }
    for (auto& t : treeResults)
        if (! t.waveReady) { renderWave (t.g, t.wave); t.waveReady = true; ++labVer; return true; }
    return false;
}

//==============================================================================
// FAMILY TREE: up to 4 sounds, paired like a family tree (1 x 2, 3 x 4) and then crossed again
void KeysKillaProcessor::setAncestorGenome (int slot, const Genome& g)
{
    if (! juce::isPositiveAndBelow (slot, numAncestors) || ! g.valid()) return;
    ancestors[(size_t) slot] = g; ++labVer;
}
void KeysKillaProcessor::setAncestorPreset (int slot, int idx) { setAncestorGenome (slot, genomeFromPreset (idx)); }
void KeysKillaProcessor::setAncestorCurrent (int slot) { setAncestorGenome (slot, genomeFromCurrent()); }
void KeysKillaProcessor::clearAncestor (int slot) { if (juce::isPositiveAndBelow (slot, numAncestors)) { ancestors[(size_t) slot] = Genome(); ++labVer; } }
void KeysKillaProcessor::randomAncestor (int slot)
{
    juce::Random r ((juce::int64) (juce::Time::getHighResolutionTicks() ^ (juce::int64) (slot * 104729)));
    setAncestorPreset (slot, r.nextInt ((int) factoryPresets().size()));
}
void KeysKillaProcessor::stepAncestor (int slot, int dir)
{
    if (! juce::isPositiveAndBelow (slot, numAncestors)) return;
    const auto& a = ancestors[(size_t) slot];
    const auto& ps = factoryPresets();
    const int n = (int) ps.size();
    int i = a.preset >= 0 ? a.preset : 0;
    for (int k = 0; k < n; ++k)
    {
        i = ((i + dir) % n + n) % n;
        if (a.cat < 0 || ps[(size_t) i].cat == a.cat) break;
    }
    setAncestorPreset (slot, i);
}

int KeysKillaProcessor::treeBreed()
{
    std::vector<Genome> an;
    for (auto& a : ancestors) if (a.valid()) an.push_back (a);
    if (an.empty()) { randomAncestor (0); randomAncestor (1); for (auto& a : ancestors) if (a.valid()) an.push_back (a); }
    if (an.size() == 1) an.push_back (genomeFromCurrent());
    ++treeCount;
    const uint32_t base = kk::hash32 ((uint32_t) juce::Time::getHighResolutionTicks() ^ (treeCount * 2654435761u));   // every press is new
    const float wild = juce::jlimit (0.0f, 1.0f, breedWild);
    auto shortOf = [] (const juce::String& n)
    {
        auto x = n.upToFirstOccurrenceOf (juce::String::fromUTF8 (" \xc2\xb7"), false, false).upToFirstOccurrenceOf (juce::String::fromUTF8 (" \xc3\x97"), false, false);
        for (auto pre : { "Classic ", "Layered ", "Atmos ", "Lo-Fi ", "Rage ", "Hyper ", "Future " }) if (x.startsWith (pre)) { x = x.substring ((int) std::strlen (pre)); break; }
        return x.substring (0, 16).trim();
    };
    juce::StringArray names; for (auto& a : an) names.add (shortOf (a.name));
    treeResults.clear();
    for (int k = 0; k < 6; ++k)
    {
        const uint32_t sk = kk::hash32 (base + (uint32_t) k * 7919u);
        Genome x = an[0], y = an[1];
        if (an.size() >= 3)   // grandparents: 1 x 2 -> X, 3 (x 4) -> Y
        {
            x = makeChildOf (an[0], an[1], k, kk::hash32 (sk ^ 0x1111u), nullptr, false).g;
            y = an.size() >= 4 ? makeChildOf (an[2], an[3], 5 - k, kk::hash32 (sk ^ 0x2222u), nullptr, false).g : an[2];
        }
        TreeResult r;
        r.g = makeChildOf (x, y, k, sk, nullptr, false).g;
        // melody: phrases mixed from the chosen sounds' loops, always a new combination
        std::vector<kk::LoopGenes> from;
        for (auto& a : an) from.push_back (a.loop);
        r.g.loop = kk::mixLoops (from, sk ^ 0x5bd1e995u, wild);
        r.g.name = names.joinIntoString (" + ") + " #" + juce::String (k + 1);
        treeResults.push_back (std::move (r));
    }
    treeSel = -1;
    ++labVer;
    selectTreeResult (0);
    return (int) treeResults.size();
}

void KeysKillaProcessor::selectTreeResult (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) treeResults.size())) return;
    treeSel = i; ++labVer;
    applyGenome (treeResults[(size_t) i].g, true);
    selChild = -1;
    setCurrentLoop (treeResults[(size_t) i].g.loop);
    loopOwner = 1;
    captureUndo();
}

void KeysKillaProcessor::playTreeResult (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) treeResults.size())) return;
    if (treeMode == treeLoop)
    {
        if (loopIsTree (i)) { stopLoop(); ++labVer; return; }
        if (i != treeSel || loopOwner != 1) selectTreeResult (i);
        if (! loopOn) toggleLoop();
        return;
    }
    if (i != treeSel || loopOwner != 1) selectTreeResult (i);
    previewNote = raw[(size_t) ix->bassMode]->load() > 0.5f ? 36 : 60;
}

void KeysKillaProcessor::rateTreeResult (int i, int stars)
{
    if (! juce::isPositiveAndBelow (i, (int) treeResults.size())) return;
    auto& t = treeResults[(size_t) i];
    t.rating = t.rating == stars ? 0 : juce::jlimit (0, 5, stars);
    ++labVer;
}

void KeysKillaProcessor::newMelody (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) treeResults.size())) return;
    auto& t = treeResults[(size_t) i];
    t.g.loop = kk::rerollLoop (t.g.loop, kk::hash32 ((uint32_t) juce::Time::getHighResolutionTicks() ^ (uint32_t) (i * 7919 + 1)));
    if (i == treeSel && loopOwner == 1) setCurrentLoop (t.g.loop);
    ++labVer;
}

static juce::String floatsToString (const std::vector<float>& v)
{
    juce::String s; s.preallocateBytes (v.size() * 8);
    for (auto x : v) s << juce::String (x, 5) << ",";
    return s;
}
static std::vector<float> stringToFloats (const juce::String& s)
{
    std::vector<float> v;
    for (auto& t : juce::StringArray::fromTokens (s, ",", "")) if (t.isNotEmpty()) v.push_back (t.getFloatValue());
    return v;
}

static juce::String loopToString (const kk::LoopGenes& l)
{
    if (! l.valid) return {};
    juce::String s;
    for (auto x : l.g) s << (juce::int64) x << ",";
    return s + juce::String (l.key);
}
static kk::LoopGenes loopFromString (const juce::String& s)
{
    kk::LoopGenes l;
    const auto t = juce::StringArray::fromTokens (s, ",", "");
    if (t.size() != kk::numLoopGenes + 1) return l;
    for (int i = 0; i < kk::numLoopGenes; ++i)
    {
        const auto v = (uint32_t) t[i].getLargeIntValue();
        l.g[(size_t) i] = kk::loopdata::size[i] == 0 ? v : v % kk::loopdata::size[i];
    }
    l.key = juce::jlimit (0, 11, t[kk::numLoopGenes].getIntValue());
    l.valid = true;
    return l;
}

void KeysKillaProcessor::saveLab (juce::ValueTree& state) const
{
    juce::ValueTree lab ("BREEDLAB");
    auto genome = [] (const Genome& g, const juce::Identifier& type)
    {
        juce::ValueTree t (type);
        t.setProperty ("name", g.name, nullptr); t.setProperty ("cat", g.cat, nullptr); t.setProperty ("era", g.era, nullptr);
        t.setProperty ("gen", g.gen, nullptr); t.setProperty ("preset", g.preset, nullptr); t.setProperty ("v", floatsToString (g.v), nullptr);
        t.setProperty ("loop", loopToString (g.loop), nullptr);
        return t;
    };
    lab.setProperty ("count", (int) breedCount, nullptr);
    lab.setProperty ("wild", breedWild, nullptr);
    lab.setProperty ("sel", selChild, nullptr);
    juce::String locks;
    for (int g = 0; g < numGenes; ++g) locks << (geneLock[(size_t) g] ? "1" : "0") << geneLockSrc[(size_t) g] << ",";
    lab.setProperty ("locks", locks, nullptr);
    lab.setProperty ("loopBars", loopBarsN, nullptr);
    lab.setProperty ("loopKey", loopKeyN, nullptr);
    lab.setProperty ("curLoop", loopToString (curLoop), nullptr);
    lab.setProperty ("treeMode", treeMode, nullptr);
    for (int a = 0; a < numAncestors; ++a) if (ancestors[(size_t) a].valid()) { auto t = genome (ancestors[(size_t) a], "ANC"); t.setProperty ("slot", a, nullptr); lab.appendChild (t, nullptr); }
    for (auto& r : treeResults) { auto t = genome (r.g, "TRES"); t.setProperty ("rating", r.rating, nullptr); lab.appendChild (t, nullptr); }
    lab.appendChild (genome (parents[0], "PA"), nullptr);
    lab.appendChild (genome (parents[1], "PB"), nullptr);
    for (auto& c : children)
    {
        auto t = genome (c.g, "CHILD");
        juce::String gs; for (auto x : c.genes) gs << x;
        t.setProperty ("genes", gs, nullptr); t.setProperty ("seed", (juce::int64) c.seed, nullptr); t.setProperty ("rating", c.rating, nullptr);
        t.setProperty ("hybrid", c.hybrid, nullptr);
        lab.appendChild (t, nullptr);
    }
    state.appendChild (lab, nullptr);
}

void KeysKillaProcessor::loadLab (const juce::ValueTree& state)
{
    auto lab = state.getChildWithName ("BREEDLAB");
    if (! lab.isValid()) return;
    auto genome = [this] (const juce::ValueTree& t)
    {
        Genome g;
        g.name = t.getProperty ("name").toString(); g.cat = t.getProperty ("cat", -1); g.era = t.getProperty ("era", 0);
        g.gen = t.getProperty ("gen", 0); g.preset = t.getProperty ("preset", -1); g.v = stringToFloats (t.getProperty ("v").toString());
        if (g.v.size() != params.size()) g.v.clear();
        g.loop = loopFromString (t.getProperty ("loop").toString());
        if (! g.loop.valid) g.loop = kk::loopFromSeed ((uint32_t) g.name.hashCode());
        return g;
    };
    breedCount = (uint32_t) (int) lab.getProperty ("count", 0);
    breedWild = (float) lab.getProperty ("wild", 0.25f);
    const auto locks = juce::StringArray::fromTokens (lab.getProperty ("locks").toString(), ",", "");
    for (int g = 0; g < numGenes && g < locks.size(); ++g)
    {
        geneLock[(size_t) g] = locks[g].startsWith ("1");
        geneLockSrc[(size_t) g] = locks[g].getLastCharacter() == '1' ? 1 : 0;
    }
    children.clear(); treeResults.clear(); treeSel = -1;
    for (auto& an : ancestors) an = Genome();
    loopBarsN = (int) lab.getProperty ("loopBars", 8) > 8 ? 16 : 8;
    loopKeyN = juce::jlimit (-1, 11, (int) lab.getProperty ("loopKey", -1));
    treeMode = (int) lab.getProperty ("treeMode", 0) == 1 ? 1 : 0;
    for (auto t : lab)
    {
        if (t.hasType ("PA")) { auto g = genome (t); if (g.valid()) parents[0] = g; }
        else if (t.hasType ("ANC")) { auto g = genome (t); const int sl = t.getProperty ("slot", -1); if (g.valid() && juce::isPositiveAndBelow (sl, numAncestors)) ancestors[(size_t) sl] = g; }
        else if (t.hasType ("TRES")) { TreeResult r; r.g = genome (t); r.rating = t.getProperty ("rating", 0); if (r.g.valid()) treeResults.push_back (std::move (r)); }
        else if (t.hasType ("PB")) { auto g = genome (t); if (g.valid()) parents[1] = g; }
        else if (t.hasType ("CHILD"))
        {
            Child c; c.g = genome (t);
            if (! c.g.valid()) continue;
            const auto gs = t.getProperty ("genes").toString();
            for (int g = 0; g < numGenes && g < gs.length(); ++g) c.genes[(size_t) g] = gs[g] == '1' ? 1 : 0;
            c.seed = (uint32_t) (juce::int64) t.getProperty ("seed", 0); c.rating = t.getProperty ("rating", 0);
            c.hybrid = (bool) t.getProperty ("hybrid", false);
            children.push_back (std::move (c));
        }
    }
    selChild = juce::jlimit (-1, (int) children.size() - 1, (int) lab.getProperty ("sel", -1));
    const auto cl = loopFromString (lab.getProperty ("curLoop").toString());
    if (cl.valid) setCurrentLoop (cl);
    ++labVer;
}

//==============================================================================
void KeysKillaProcessor::rebuildCornerBank()
{
    bool any = false;
    for (int c : corners) any |= c >= 0;
    if (! any) { morphActive = false; return; }
    const int target = 1 - cornerBankIdx.load();
    auto& bank = cornerBank[(size_t) target];
    const auto& ps = factoryPresets();
    for (int c = 0; c < 4; ++c)
    {
        auto& vals = bank[(size_t) c];
        for (size_t i = 0; i < params.size(); ++i) vals[i] = params[i]->getValue();   // unset corner = current sound
        if (corners[(size_t) c] < 0) continue;
        const auto& pr = ps[(size_t) corners[(size_t) c]];
        for (size_t i = 0; i < params.size(); ++i) vals[i] = params[i]->getDefaultValue();
        auto setv = [&] (const juce::String& id, float v) { const int k = indexOf (id); vals[(size_t) k] = params[(size_t) k]->convertTo0to1 (v); };
        for (auto& [id, v] : pr.values) setv (id, v);
        if (pr.isBass()) { setv (ID::mono, 1); setv (ID::bassMode, 1); }
    }
    cornerBankIdx = target;
    morphActive = true;
}

void KeysKillaProcessor::setMorphCorner (int corner, int presetIndex)
{
    corners[(size_t) juce::jlimit (0, 3, corner)] = presetIndex;
    rebuildCornerBank();
}

//==============================================================================
void KeysKillaProcessor::switchAB()
{
    abState[abSlot] = apvts.copyState();
    abState[abSlot].setProperty ("presetName", presetName, nullptr);
    abSlot = 1 - abSlot;
    if (abState[abSlot].isValid())
    {
        apvts.replaceState (abState[abSlot].createCopy());
        syncParamsToState();
        presetName = abState[abSlot].getProperty ("presetName", presetName).toString();
    }
}

void KeysKillaProcessor::copyAtoB()
{
    abState[1 - abSlot] = apvts.copyState();
    abState[1 - abSlot].setProperty ("presetName", presetName, nullptr);
}

std::vector<float> KeysKillaProcessor::snapshot() const
{
    std::vector<float> v (params.size());
    for (size_t i = 0; i < params.size(); ++i) v[i] = params[i]->getValue();
    return v;
}

void KeysKillaProcessor::applySnapshot (const std::vector<float>& v)
{
    if (v.size() != params.size()) return;
    for (size_t i = 0; i < params.size(); ++i)
        if (std::abs (params[i]->getValue() - v[i]) > 1.0e-6f) params[i]->setValueNotifyingHost (v[i]);
    lastSnap = v;
}

void KeysKillaProcessor::captureUndo()
{
    auto now = snapshot();
    if (lastSnap.size() != now.size()) { lastSnap = now; return; }
    if (now == lastSnap) return;
    undoStack.push_back (lastSnap);
    if (undoStack.size() > 50) undoStack.erase (undoStack.begin());
    redoStack.clear();
    lastSnap = now;
}

bool KeysKillaProcessor::undo()
{
    captureUndo();
    if (undoStack.empty()) return false;
    redoStack.push_back (snapshot());
    applySnapshot (undoStack.back());
    undoStack.pop_back();
    return true;
}

bool KeysKillaProcessor::redo()
{
    if (redoStack.empty()) return false;
    undoStack.push_back (snapshot());
    applySnapshot (redoStack.back());
    redoStack.pop_back();
    return true;
}

std::array<int, kk::numFxSlots> KeysKillaProcessor::getFxOrder() const
{
    std::array<int, kk::numFxSlots> o {};
    for (int i = 0; i < kk::numFxSlots; ++i) o[(size_t) i] = fxOrder[(size_t) i].load();
    return o;
}

void KeysKillaProcessor::setFxOrder (const std::array<int, kk::numFxSlots>& o)
{
    std::array<bool, kk::numFxSlots> seen {};
    for (int v : o)
    {
        if (v < 0 || v >= kk::numFxSlots || seen[(size_t) v]) return;
        seen[(size_t) v] = true;
    }
    for (int i = 0; i < kk::numFxSlots; ++i) fxOrder[(size_t) i] = o[(size_t) i];
}

//==============================================================================
static juce::String orderToString (const std::array<int, kk::numFxSlots>& o)
{
    juce::StringArray s; for (int v : o) s.add (juce::String (v));
    return s.joinIntoString (",");
}

static std::array<int, kk::numFxSlots> orderFromString (const juce::String& str)
{
    std::array<int, kk::numFxSlots> o {};
    o = kk::defaultFxOrder();
    auto t = juce::StringArray::fromTokens (str, ",", "");
    if (t.size() == kk::numFxSlots) for (int i = 0; i < kk::numFxSlots; ++i) o[(size_t) i] = t[i].getIntValue();
    else if (t.size() == kk::fxPunch)   // v0.3 order: keep it, put PUNCH first and HALF-TIME after CIRCUIT
    {
        size_t k = 0;
        o[k++] = kk::fxPunch;
        for (auto& x : t)
        {
            if (k < o.size()) o[k++] = juce::jlimit (0, kk::numFxSlots - 1, x.getIntValue());
            if (x.getIntValue() == kk::fxCircuit && k < o.size()) o[k++] = kk::fxHalftime;
        }
        if (k != (size_t) kk::numFxSlots) o = kk::defaultFxOrder();
    }
    return o;
}

void KeysKillaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetIndex", currentPreset, nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("userFile", userFile.getFullPathName(), nullptr);
    state.setProperty ("version", 2, nullptr);
    state.setProperty ("fxOrder", orderToString (getFxOrder()), nullptr);
    state.setProperty ("corners", juce::String (corners[0]) + "," + juce::String (corners[1]) + "," + juce::String (corners[2]) + "," + juce::String (corners[3]), nullptr);
    state.setProperty ("eco", eco.load(), nullptr);
    state.setProperty ("macroNames", macroLabels.joinIntoString ("|"), nullptr);
    for (int d = 0; d < 3; ++d) state.setProperty ("drum" + juce::String (d), drums[(size_t) d].filePath(), nullptr);
    for (int k = 0; k < kk::PairLab::maxParents; ++k) state.setProperty ("pair" + juce::String (k), pairFiles[(size_t) k], nullptr);
    for (int m = 0; m < numModules; ++m)
        if (modules[(size_t) m])
        {
            juce::MemoryBlock mb; modules[(size_t) m]->getStateInformation (mb);
            state.setProperty ("module" + juce::String (m), mb.toBase64Encoding(), nullptr);
        }
    saveLab (state);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void KeysKillaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentPreset = vt.getProperty ("presetIndex", -1);
            presetName = vt.getProperty ("presetName", "Init").toString();
            userFile = juce::File (vt.getProperty ("userFile", "").toString());
            setFxOrder (orderFromString (vt.getProperty ("fxOrder", "").toString()));
            eco = (bool) vt.getProperty ("eco", false);
            macroLabels = juce::StringArray::fromTokens (vt.getProperty ("macroNames", "").toString(), "|", "");
            macroLabels.removeEmptyStrings();
            loadLab (vt);
            vt.removeChild (vt.getChildWithName ("BREEDLAB"), nullptr);
            apvts.replaceState (vt);
            syncParamsToState();
            auto cs = juce::StringArray::fromTokens (vt.getProperty ("corners", "-1,-1,-1,-1").toString(), ",", "");
            for (int c = 0; c < 4; ++c) corners[(size_t) c] = -1;   // v0.7: morph corners retired (they froze the sound)
            juce::ignoreUnused (cs);
            rebuildCornerBank();
            snapshotForModified();
            if (! curLoop.valid) curLoop = kk::loopFromSeed ((uint32_t) presetName.hashCode());
            rebuildLoopSeq();   // bass / mono of the restored sound decide which lines the loop plays
            for (int m = 0; m < numModules; ++m)
                if (modules[(size_t) m] && vt.hasProperty ("module" + juce::String (m)))
                {
                    juce::MemoryBlock mb;
                    if (mb.fromBase64Encoding (vt.getProperty ("module" + juce::String (m)).toString()) && mb.getSize() > 0)
                        modules[(size_t) m]->setStateInformation (mb.getData(), (int) mb.getSize());
                }
            rebuildRolls();
            for (int k = 0; k < kk::PairLab::maxParents; ++k)
            {
                const juce::File pf (vt.getProperty ("pair" + juce::String (k), "").toString());
                if (pf.existsAsFile() && pf.getFullPathName() != pairFiles[(size_t) k]) loadPairParent (k, pf);
            }
            for (int d = 0; d < 3; ++d)
            {
                const juce::File df (vt.getProperty ("drum" + juce::String (d), "").toString());
                if (df.existsAsFile() && df.getFullPathName() != drums[(size_t) d].filePath()) loadDrum (d, df);
            }
            moduleHousekeeping();
        }
}

// replaceState() skips parameters whose denormalised value did not change (e.g. a bool sitting at 0.28),
// so push every stored value explicitly.
void KeysKillaProcessor::syncParamsToState()
{
    for (auto* rp : params)
    {
        auto child = apvts.state.getChildWithProperty ("id", rp->getParameterID());
        if (child.isValid() && child.hasProperty ("value"))
        {
            const float norm = rp->convertTo0to1 ((float) child.getProperty ("value"));
            if (std::abs (rp->getValue() - norm) > 1.0e-6f) rp->setValueNotifyingHost (norm);
        }
    }
}

//==============================================================================
juce::File KeysKillaProcessor::userPresetDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("Presets");
    d.createDirectory();
    return d;
}

juce::Array<juce::File> KeysKillaProcessor::userPresets() const
{
    auto files = userPresetDir().findChildFiles (juce::File::findFiles, true, "*.kkpreset");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b) { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
    return files;
}

bool KeysKillaProcessor::saveUserPreset (const juce::File& f)
{
    auto* root = new juce::DynamicObject();
    root->setProperty ("format", "KEYS KILLA preset");
    root->setProperty ("version", 1);
    root->setProperty ("name", f.getFileNameWithoutExtension());
    root->setProperty ("category", currentPreset >= 0 ? categoryNames()[factoryPresets()[(size_t) currentPreset].cat] : juce::String ("USER"));
    root->setProperty ("fxOrder", orderToString (getFxOrder()));
    root->setProperty ("macroNames", macroLabels.joinIntoString ("|"));
    auto* vals = new juce::DynamicObject();
    for (auto* rp : params) vals->setProperty (rp->getParameterID(), rp->convertFrom0to1 (rp->getValue()));
    root->setProperty ("params", juce::var (vals));
    const juce::var v (root);
    if (! f.replaceWithText (juce::JSON::toString (v, false))) return false;
    presetName = f.getFileNameWithoutExtension(); currentPreset = -1; userFile = f;
    snapshotForModified();
    return true;
}

bool KeysKillaProcessor::loadUserPreset (const juce::File& f)
{
    const auto v = juce::JSON::parse (f);
    if (! v.isObject() || v["format"].toString() != "KEYS KILLA preset") return false;
    std::vector<std::pair<juce::String, float>> vals;
    if (auto* obj = v["params"].getDynamicObject())
        for (auto& prop : obj->getProperties())
            if (! keeps (prop.name.toString()))
                vals.push_back ({ prop.name.toString(), (float) (double) prop.value });
    applyValues (vals);
    setFxOrder (orderFromString (v["fxOrder"].toString()));
    macroLabels = juce::StringArray::fromTokens (v["macroNames"].toString(), "|", ""); macroLabels.removeEmptyStrings();
    presetName = f.getFileNameWithoutExtension(); currentPreset = -1; userFile = f;
    snapshotForModified();
    setCurrentLoop (kk::loopFromSeed ((uint32_t) presetName.hashCode())); loopOwner = -1;
    return true;
}

bool KeysKillaProcessor::renameUserPreset (const juce::String& newName)
{
    if (! userFile.existsAsFile() || newName.trim().isEmpty()) return false;
    auto target = userFile.getSiblingFile (juce::File::createLegalFileName (newName.trim()) + ".kkpreset");
    if (target.exists() || ! userFile.moveFileTo (target)) return false;
    userFile = target; presetName = target.getFileNameWithoutExtension();
    return true;
}

bool KeysKillaProcessor::deleteUserPreset()
{
    if (! userFile.existsAsFile() || ! userFile.deleteFile()) return false;
    userFile = juce::File(); presetName = "Init";
    return true;
}

int KeysKillaProcessor::importPack (const juce::File& src)
{
    int count = 0;
    auto dir = userPresetDir();
    if (src.isDirectory())
    {
        for (auto& f : src.findChildFiles (juce::File::findFiles, true, "*.kkpreset"))
            if (f.copyFileTo (dir.getChildFile (f.getFileName()))) ++count;
    }
    else
    {
        juce::ZipFile zip (src);
        for (int i = 0; i < zip.getNumEntries(); ++i)
            if (auto* e = zip.getEntry (i); e && e->filename.endsWithIgnoreCase (".kkpreset"))
            {
                std::unique_ptr<juce::InputStream> in (zip.createStreamForEntry (i));
                auto out = dir.getChildFile (juce::File::createLegalFileName (juce::File (e->filename).getFileName()));
                if (in && out.replaceWithText (in->readEntireStreamAsString())) ++count;
            }
    }
    return count;
}

bool KeysKillaProcessor::exportPack (const juce::File& zipFile)
{
    juce::ZipFile::Builder b;
    for (auto& f : userPresets()) b.addFile (f, 9, f.getFileName());
    zipFile.deleteFile();
    juce::FileOutputStream os (zipFile);
    return os.openedOk() && b.writeToStream (os, nullptr);
}

juce::AudioProcessorEditor* KeysKillaProcessor::createEditor() { return new KeysKillaEditor (*this); }

#if ! KK_TEST_BUILD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KeysKillaProcessor(); }
#endif
