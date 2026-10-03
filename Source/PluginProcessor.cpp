#include "PluginProcessor.h"
#include <set>
#include <functional>
#include "PluginEditor.h"
#include "BinaryData.h"

static bool writeWavFile (const juce::AudioBuffer<float>& b, double rate, const juce::File& f);
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
    : AudioProcessor (juce::PluginHostType::getPluginLoadedAs() == juce::AudioProcessor::wrapperType_AudioUnit
                          ? BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                          // VST3: an optional audio input - KEYS KILLA in an FL mixer insert puts VOODOO / EFFECTOR on that track
                          : BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), false)
                                             .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
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
    // v0.34: BREED LAB starts with two empty parents - you choose (or drop) your two sounds; the keys play the first preset

    juce::ignoreUnused (withModules);   // v0.32: VOODOO / EFFECTOR / DIGGA KILLA are separate plugins again
    for (int d = 0; d < kk::numDrumSlots; ++d) generatePattern (d, 0, 2, 0.5f);
    resetSampleEdit();
    stepPreset (0);   // STEP FX starts with a pattern ready (off until you switch it on)
}

KeysKillaProcessor::~KeysKillaProcessor()
{
    vst.unload(); vstB.unload();
    harvestPool.removeAllJobs (true, 60000);
    drumPool.removeAllJobs (true, 60000);   // a running harvest finishes before the plugin goes
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
        && (layouts.getMainInputChannelSet().isDisabled() || layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo());
}

void KeysKillaProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    synth.prepare ((float) sampleRate);
    fxPtr = std::make_unique<kk::FxRack>();   // fresh DSP state on every prepare
    fxPtr->prepare (sampleRate, kChunk);
    rackFx = std::make_unique<kk::FxRack>(); rackFx->prepare (sampleRate, kChunk);
    rackL.assign (kChunk, 0.0f); rackR.assign (kChunk, 0.0f); rackG.assign (kChunk, 0.0f);
    rackTail = 0; gateEnv = 1.0f;
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
    for (int d = 0; d < kk::numDrumSlots; ++d)   // drums are stored at the plugin rate: reload when the rate changes
        if (drums[(size_t) d].hasSample() && std::abs (drums[(size_t) d].loadedRate() - sampleRate) > 0.5)
            loadDrum (d, juce::File (drums[(size_t) d].filePath()));
    modBlock = std::max (64, samplesPerBlock);
    vstMidi.ensureSize (4096);
    vst.prepare (sampleRate, std::max (64, samplesPerBlock));
    vstB.prepare (sampleRate, std::max (64, samplesPerBlock));
    chop.prepare (sampleRate);
    fxIn.setSize (2, std::max (64, samplesPerBlock) * 2);
    extBuf.setSize (2, std::max (64, samplesPerBlock) * 2);
    worldExt.prepare (sampleRate);
    extFx = std::make_unique<kk::FxRack>(); extFx->prepare (sampleRate, kChunk);
    extL.assign (kChunk, 0.0f); extR.assign (kChunk, 0.0f); extG.assign (kChunk, 0.0f); extTail = 0; extLpHz = extHpHz = -1;
    for (auto& f : extLp) f.reset();
    for (auto& f : extHp) f.reset();
    {
        int size = 1; while (size < (int) (sampleRate * 2.6)) size <<= 1;
        stepBufL.assign ((size_t) size, 0.0f); stepBufR.assign ((size_t) size, 0.0f); echoL.assign ((size_t) size, 0.0f); echoR.assign ((size_t) size, 0.0f);
        stepW = echoW = 0; stepLast = -1; for (auto& e : stepEnv) e = 0;
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
    // FX INPUT: audio coming in (KEYS KILLA in a mixer insert) joins the melody bus -> VOODOO + EFFECTOR
    const int inCh = std::min (getTotalNumInputChannels(), buffer.getNumChannels());
    bool hasInput = false;
    if (inCh > 0 && n > 0 && n <= fxIn.getNumSamples())
    {
        for (int c = 0; c < 2; ++c) fxIn.copyFrom (c, 0, buffer, std::min (c, inCh - 1), 0, n);
        hasInput = fxIn.getMagnitude (0, n) > 1.0e-7f;
    }
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
    pairPlayer.shTune = sampleEdit[seTune].load() + sampleEdit[seFine].load() / 100.0f;
    pairPlayer.shStart = sampleEdit[seStart].load(); pairPlayer.shAttack = sampleEdit[seAttack].load();
    pairPlayer.shRelease = sampleEdit[seRelease].load(); pairPlayer.shRev = sampleEdit[seReverse].load() > 0.5f;
    chop.choke = chopChoke.load();

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
    vstMidi.clear();
    {
        const int pm = (int) raw[(size_t) I.playMode]->load();
        if (pm != lastPlayMode)
        {
            if (lastPlayMode == playKeys) synth.allOff (false);
            else if (lastPlayMode == playPair) pairPlayer.allOff();
            else if (lastPlayMode == playVst) vstMidi.addEvent (juce::MidiMessage::allNotesOff (1), 0);
            else if (lastPlayMode == playChop) chop.allOff();
            else { if (const int dd = drumOfMode (lastPlayMode); dd >= 0) drums[(size_t) dd].allOff(); }
            lastPlayMode = pm;
        }
        if (pm == playChop)   // the keys play the slices: C5 = slice 1, C#5 = slice 2 ...
        {
            for (const auto meta : midi)
            {
                const auto m = meta.getMessage();
                if (m.isNoteOn()) chop.noteOn (m.getNoteNumber(), m.getFloatVelocity(), juce::jlimit (0, n - 1, meta.samplePosition));
                else if (m.isNoteOff()) chop.noteOff (m.getNoteNumber());
                else if (m.isAllNotesOff()) chop.allOff();
            }
            midi.clear();
        }
        else if (pm == playPair || pm == playVst) {}   // routed after the loop player (keys + loop notes)
        else if (drumOfMode (pm) >= 0)   // the keys play the boosted drum (808: in tune with its detected note)
        {
            auto& d = drums[(size_t) drumOfMode (pm)];
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
    renderPatterns (n, beatPos, bps, hostPlaying);
    // PAIR FROM VST: keys and loop notes play the hosted plugin
    if ((int) raw[(size_t) I.playMode]->load() == playVst) { vstMidi.addEvents (processedMidi, 0, n, 0); processedMidi.clear(); }
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

    // PAIR / CHOP / VST / the mixer input: rendered apart, coloured by their own SOUND WORLD + GATE, then added
    const bool ext = buffer.getNumChannels() > 1 && n <= extBuf.getNumSamples();
    if (ext)
    {
        extBuf.clear (0, n);
        float* eL = extBuf.getWritePointer (0); float* eR = extBuf.getWritePointer (1);
        pairPlayer.render (eL, eR, n, sr);
        if (const int h = chopPad.exchange (-1); h >= 0) chop.noteOn (kk::ChopLab::firstNote + h, 0.9f, 0);
        if (flipOn.load())   // SAMPLER FLIP: the pattern plays the chops in time (1/16 steps, 2 bars)
        {
            const juce::SpinLock::ScopedTryLockType fl (flipLock);
            if (fl.isLocked() && ! flipSeq.empty())
            {
                if (hostPlaying) flipOrigin = 0.0;
                else if (flipOrigin < 0.0) flipOrigin = std::floor (beatPos * 4.0) / 4.0;
                const int steps = (int) flipSeq.size();
                for (int i = 0; i < n; ++i)
                {
                    const double b = beatPos + bps * i - flipOrigin;
                    const int st = (int) std::floor (b * 4.0);
                    if (st == flipLastStep) continue;
                    flipLastStep = st;
                    const int k = ((st % steps) + steps) % steps;
                    flipStepNow = k;
                    const auto& fs = flipSeq[(size_t) k];
                    if (fs.slice >= 0) chop.noteOn (kk::ChopLab::firstNote + fs.slice, fs.vel, i, fs.rev, fs.semi);
                }
            }
        }
        else flipStepNow = -1;
        chop.render (eL, eR, n);
        vstNoMidi.clear();
        if (vst.loaded()) vst.process (eL, eR, n, vstKeys.load() == 0 ? vstMidi : vstNoMidi, getPlayHead());
        vstNoMidi.clear();
        if (vstB.loaded()) vstB.process (eL, eR, n, vstKeys.load() == 1 ? vstMidi : vstNoMidi, getPlayHead());
        if (hasInput) for (int c = 0; c < 2; ++c) extBuf.addFrom (c, 0, fxIn, c, 0, n);
        processSampleFx (eL, eR, n, beatPos, bps);   // v0.37: the big knobs + SAMPLE EDIT colour your sounds too
        worldExt.process (eL, eR, n, (int) raw[(size_t) I.world]->load(), raw[(size_t) I.worldAmt]->load(),
                          (int) raw[(size_t) I.gate]->load(), raw[(size_t) I.gateDepth]->load(), beatPos, bps);
        for (int c = 0; c < 2; ++c) buffer.addFrom (c, 0, extBuf, c, 0, n);
    }
    else if (buffer.getNumChannels() > 1)   // an oversized host block: play them without the world colour
    {
        float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
        pairPlayer.render (L, R, n, sr);
        chop.render (L, R, n);
        if (vst.loaded()) vst.process (L, R, n, vstMidi, getPlayHead());
    }
    processRack (buffer, n, beatPos, bps);   // FX RACK: the whole melody bus (synth, PAIR, VST, SAMPLER)
    processStepFx (buffer, n, beatPos, bps); // v0.37 STEP FX: effects in time on the melody bus
    // DRUM BOOST: the drums play after the melody effects - the FX RACK never touches them
    if (buffer.getNumChannels() > 1)
        for (int d = 0; d < kk::numDrumSlots; ++d)
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

// ---------------- FX RACK (v0.32): KEYS KILLA's own effects on the melody bus ----------------
void KeysKillaProcessor::processRack (juce::AudioBuffer<float>& buffer, int n, double beatPos, double bps)
{
    if (buffer.getNumChannels() < 2 || rackFx == nullptr || rackL.empty()) return;
    const bool any = rack.anyOn();
    if (any) rackTail = (int) (sr * 6.0);   // after the last effect goes off the rack runs on until echoes / reverb died away
    else if (rackTail <= 0) { gateEnv = 1.0f; return; }
    else rackTail -= n;
    auto on = [this] (int s) { return rack.on[(size_t) s].load(); };
    auto val = [this] (int v) { return rack.v[(size_t) v].load(); };
    kk::FxParams p;
    p.drive = on (kk::rkDrive) ? val (kk::rvDrive) : 0.0f; p.driveType = (int) val (kk::rvDriveType);
    p.crush = on (kk::rkLofi) ? val (kk::rvCrush) : 0.0f;
    p.chorus = on (kk::rkChorus) ? val (kk::rvChorus) : 0.0f;
    p.phaser = on (kk::rkPhaser) ? val (kk::rvPhaser) : 0.0f;
    p.flanger = on (kk::rkFlanger) ? val (kk::rvFlanger) : 0.0f;
    p.halftime = on (kk::rkHalf) ? val (kk::rvHalf) : 0.0f;
    p.delayMix = on (kk::rkDelay) ? val (kk::rvDelayMix) : 0.0f; p.delayFb = val (kk::rvDelayFb);
    p.delayBeats = Choices::delayBeats ((int) val (kk::rvDelayTime)); p.delayMode = 0;
    p.revMix = on (kk::rkReverb) ? val (kk::rvRevMix) : 0.0f; p.revSize = val (kk::rvRevSize); p.revType = (int) val (kk::rvRevType);
    p.eqLow = on (kk::rkEq) ? val (kk::rvEqLow) : 0.0f; p.eqHigh = on (kk::rkEq) ? val (kk::rvEqHigh) : 0.0f;
    p.width = on (kk::rkWidth) ? val (kk::rvWidth) : 0.5f;
    p.master = 0.0f; p.outGain = 1.0f; p.bpm = bps * 60.0 * sr;
    const bool gate = on (kk::rkGate);
    const double gBeats = val (kk::rvGateRate) < 0.5f ? 0.5 : val (kk::rvGateRate) < 1.5f ? 0.25 : 0.125;   // 1/8, 1/16, 1/32
    const float gDepth = val (kk::rvGateDepth), gStep = 1.0f / (0.004f * (float) sr);
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    for (int c0 = 0; c0 < n; c0 += kChunk)
    {
        const int len = std::min (kChunk, n - c0);
        std::copy (L + c0, L + c0 + len, rackL.begin()); std::copy (R + c0, R + c0 + len, rackR.begin());
        std::fill_n (rackG.begin(), len, 0.0f);
        p.beatPos = beatPos + bps * c0;
        rackFx->process (rackL.data(), rackR.data(), rackG.data(), len, p);
        for (int i = 0; i < len; ++i)
        {
            float g = 1.0f;
            if (gate)   // STUTTER: a tempo-synced gate, 4 ms ramps (no clicks)
            {
                const double x = (beatPos + bps * (c0 + i)) / gBeats;
                const float target = (x - std::floor (x)) < 0.5 ? 1.0f : 1.0f - gDepth;
                gateEnv += std::clamp (target - gateEnv, -gStep, gStep);
                g = gateEnv;
            }
            else gateEnv = std::min (1.0f, gateEnv + gStep);
            float l = rackL[(size_t) i] * g, r = rackR[(size_t) i] * g;
            if (! std::isfinite (l) || ! std::isfinite (r)) { l = L[c0 + i]; r = R[c0 + i]; }
            L[c0 + i] = l; R[c0 + i] = r;
        }
    }
}

// ---------------- PAIR YOUR OWN ----------------
// ---------------- SAMPLER / CHOP: your sample (v0.32 - KEYS KILLA's own sampler) ----------------
bool KeysKillaProcessor::chopLoadFile (const juce::File& f)
{
    juce::AudioFormatManager fm; fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
    if (r == nullptr || r->lengthInSamples < 64 || r->sampleRate <= 0) return false;
    const int len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 600.0));   // up to 10 minutes
    auto buf = std::make_shared<juce::AudioBuffer<float>> (2, len);
    r->read (buf.get(), 0, len, 0, true, true);   // mono files land on both channels
    chop.setSource (buf, r->sampleRate, f.getFileNameWithoutExtension());
    chopFile = f.getFullPathName();
    flips.clear(); flipCenter = -1; flipOn = false; ++flipVer;
    return true;
}
// v0.35 SAMPLER tools: CLEAR, MUTATE / KILL (whole sample or the selection, UNDO), the selection as a WAV / loop / parent
static bool writeWavFile (const juce::AudioBuffer<float>& b, double rate, const juce::File& f)
{
    f.getParentDirectory().createDirectory(); f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), rate, (unsigned) b.getNumChannels(), 24, {}, 0));
    return w != nullptr && w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}

void KeysKillaProcessor::chopClear()
{
    chop.stopAll(); chop.clear(); chopFile.clear(); chopUndo.clear();
}

bool KeysKillaProcessor::chopMutate (int start, int end, bool kill)
{
    auto c = chop.current();
    if (c == nullptr || c->src == nullptr) return false;
    if (end <= start) { start = 0; end = c->src->getNumSamples(); }   // nothing selected: the whole sample
    chopUndo.push_back (c->src);
    if (chopUndo.size() > 8) chopUndo.erase (chopUndo.begin());
    auto n = std::make_shared<juce::AudioBuffer<float>> (*c->src);
    kk::ChopLab::mutate (*n, start, end, c->rate, kill, (uint32_t) juce::Time::getMillisecondCounter() * 2654435761u, lastBpm.load());
    chop.replaceAudio (n);
    // keep it with the project: the new audio is a file next to your sounds
    auto f = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("Sampler")
                 .getNonexistentChildFile (juce::File::createLegalFileName (c->name) + (kill ? " KILL" : " MUTATE"), ".wav");
    if (writeWavFile (*n, c->rate, f)) chopFile = f.getFullPathName();
    return true;
}

bool KeysKillaProcessor::chopUndoMutate()
{
    if (chopUndo.empty()) return false;
    chop.replaceAudio (chopUndo.back()); chopUndo.pop_back();
    return true;
}

juce::AudioBuffer<float> KeysKillaProcessor::chopRegion (int start, int end, bool loop) const
{
    auto c = chop.current();
    if (c == nullptr || c->src == nullptr) return {};
    const auto& a = *c->src;
    start = juce::jlimit (0, a.getNumSamples(), start); end = juce::jlimit (start, a.getNumSamples(), end);
    juce::AudioBuffer<float> b (a.getNumChannels(), end - start);
    for (int ch = 0; ch < a.getNumChannels(); ++ch) b.copyFrom (ch, 0, a, ch, start, end - start);
    const int fade = std::min (b.getNumSamples() / 4, (int) (c->rate * (loop ? 0.006 : 0.003)));
    if (fade > 1) { b.applyGainRamp (0, fade, 0.0f, 1.0f); b.applyGainRamp (b.getNumSamples() - fade, fade, 1.0f, 0.0f); }
    return b;
}

juce::File KeysKillaProcessor::exportChopRegion (int start, int end, bool loop) const
{
    auto c = chop.current();
    auto b = chopRegion (start, end, loop);
    if (c == nullptr || b.getNumSamples() < 64) return {};
    const double secs = (double) b.getNumSamples() / c->rate;
    const double bpm = lastBpm.load() > 0 ? lastBpm.load() : 140.0;
    const double bars = secs / (240.0 / bpm);
    const auto tag = loop ? juce::String (" loop ") + juce::String (bars, 1) + " bars " + juce::String (juce::roundToInt (bpm)) + "BPM" : juce::String (" cut");
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Sampler")
                 .getChildFile (juce::File::createLegalFileName (c->name + tag) + ".wav");
    return writeWavFile (b, c->rate, f) ? f : juce::File();
}

bool KeysKillaProcessor::chopRegionToParent (int start, int end, int slot)
{
    auto c = chop.current();
    auto b = chopRegion (start, end, false);
    if (c == nullptr || b.getNumSamples() < 64) return false;
    auto f = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("Sampler")
                 .getNonexistentChildFile (juce::File::createLegalFileName (c->name) + " part", ".wav");
    return writeWavFile (b, c->rate, f) && labDropFile (slot, f);
}

bool KeysKillaProcessor::chopToPair (int i, int slot)
{
    auto b = chop.slice (i);
    auto c = chop.current();
    if (b.getNumSamples() < 64 || c == nullptr) return false;
    if (slot < 0) { slot = 0; while (slot < kk::PairLab::maxParents && pairParents[(size_t) slot] != nullptr) ++slot; if (slot >= kk::PairLab::maxParents) slot = 0; }
    pairParents[(size_t) slot] = kk::PairLab::fromBuffer (b, c->rate, sr > 0 ? sr : 44100.0, c->name + " chop " + juce::String (i + 1));
    pairFiles[(size_t) slot] = {};
    ++pairVer;
    return true;
}
bool KeysKillaProcessor::chopToBank (int i)
{
    auto b = chop.slice (i);
    auto c = chop.current();
    if (b.getNumSamples() < 64 || c == nullptr) return false;
    addToBank (kk::PairLab::fromBuffer (b, c->rate, sr > 0 ? sr : 44100.0, c->name + " chop " + juce::String (i + 1)), "CHOP", true);
    return true;
}

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

void KeysKillaProcessor::pairBreed (bool newChildren)
{
    std::vector<kk::PairPtr> ps (pairParents.begin(), pairParents.begin() + juce::jlimit (1, (int) pairParents.size(), pairUse));
    if (newChildren) pairSeed = kk::hash32 (pairSeed + (uint32_t) juce::Time::getMillisecondCounter());
    pairKids = kk::PairLab::breed (ps, pairSeed, pairFlavor, sr > 0 ? sr : 44100.0);
    pairSel = -1; pairLoopKid = -1;
    if (loopOn.load() && loopOwner == 2) loopOn = false;
    ++pairVer;
}

void KeysKillaProcessor::selectPairKid (int i, bool audition)
{
    if (i < 0 || i >= (int) pairKids.size()) return;
    pairSel = i;
    useSample (pairKids[(size_t) i], audition);
    ++pairVer; ++labVer;
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

// ---------------- HARVEST ----------------
void KeysKillaProcessor::harvestFile (const juce::File& f)
{
    const double rate = sr > 0 ? sr : 44100.0;
    ++harvestJobs; ++pairVer;
    harvestPool.addJob ([this, f, rate]
    {
        std::vector<kk::HarvestItem> got;
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        if (std::unique_ptr<juce::AudioFormatReader> r { fm.createReaderFor (f) })
        {
            const int len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 600.0));
            juce::AudioBuffer<float> in ((int) std::max (1u, r->numChannels), std::max (1, len));
            r->read (&in, 0, len, 0, true, true);
            got = kk::Harvest::run (in, r->sampleRate, rate, f.getFileNameWithoutExtension().substring (0, 18));
        }
        { const juce::SpinLock::ScopedLockType l (harvestLock); harvestDone.push_back (std::move (got)); }
        --harvestJobs;
    });
    harvestedFrom.addIfNotAlreadyThere (f.getFileNameWithoutExtension());
}

int KeysKillaProcessor::harvestFromChop()
{
    auto c = chop.current();
    if (c == nullptr || c->src == nullptr) return 0;
    const double rate = c->rate;
    std::vector<std::pair<juce::String, std::shared_ptr<const juce::AudioBuffer<float>>>> shots { { c->name, c->src } };
    const double target = sr > 0 ? sr : 44100.0;
    ++harvestJobs; ++pairVer;
    harvestPool.addJob ([this, shots, rate, target]
    {
        std::vector<kk::HarvestItem> all;
        for (auto& [name, audio] : shots)
            for (auto& it : kk::Harvest::run (*audio, rate, target, name.substring (0, 18)))
                all.push_back (std::move (it));
        { const juce::SpinLock::ScopedLockType l (harvestLock); harvestDone.push_back (std::move (all)); }
        --harvestJobs;
    });
    harvestedFrom.addIfNotAlreadyThere (c->name);
    return (int) shots.size();
}

void KeysKillaProcessor::bankToPair (int bankIndex, int slot)
{
    if (bankIndex < 0 || bankIndex >= (int) bank.size()) return;
    if (slot < 0) for (int k = 0; k < kk::PairLab::maxParents; ++k) if (pairParents[(size_t) k] == nullptr) { slot = k; break; }
    if (slot < 0) slot = kk::PairLab::maxParents - 1;
    pairParents[(size_t) slot] = bank[(size_t) bankIndex].sound; pairFiles[(size_t) slot].clear();
    ++pairVer;
}

void KeysKillaProcessor::auditionBank (int bankIndex)
{
    if (bankIndex < 0 || bankIndex >= (int) bank.size()) return;
    useSample (bank[(size_t) bankIndex].sound, true);
}

// ---------------- BANK on disk ----------------
juce::File KeysKillaProcessor::saveToFolder (kk::PairPtr s, const juce::String& folder)
{
    if (s == nullptr) return {};
    lastFolder = folder.trim().isEmpty() ? kk::Library::defaultFolder() : folder.trim();
    return kk::Library::save (*s, sr > 0 ? sr : 44100.0, lastFolder);
}

juce::File KeysKillaProcessor::saveToSoundKit (kk::PairPtr s, const juce::String& kit, const juce::String& category)
{
    if (s == nullptr) return {};
    lastSoundKit = kit.trim().isEmpty() ? kk::SoundKits::defaultKit() : kit.trim();
    return kk::SoundKits::save (*s, sr > 0 ? sr : 44100.0, lastSoundKit, category);
}

juce::File KeysKillaProcessor::saveDrumToKit (int d, const juce::String& kit)
{
    d = juce::jlimit (0, kk::numDrumSlots - 1, d);
    auto s = drums[(size_t) d].current();
    if (s == nullptr) return {};
    lastKit = kit.trim().isEmpty() ? kk::Kits::defaultKit() : kit.trim();
    const auto tmp = exportDrum (d);
    return kk::Kits::add (lastKit, kk::kitFolderOfSlot (d), tmp, s->name);
}

void KeysKillaProcessor::auditionFile (const juce::File& f)
{
    auto s = kk::PairLab::fromFile (f, sr > 0 ? sr : 44100.0);
    if (s == nullptr) return;
    useSample (s, true);
}

juce::File KeysKillaProcessor::bankFolder()
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("Bank");
}

void KeysKillaProcessor::addToBank (kk::PairPtr s, const juce::String& origin, bool save, int shelf)
{
    if (s == nullptr) return;
    const double rate = sr > 0 ? sr : 44100.0;
    kk::HarvestItem it; it.sound = s; it.score = 10.0f; it.origin = origin;
    const int byName = kk::harvestCatFromName (s->name);   // the preset name first ("Brass Stab"), then the ears
    it.cat = shelf >= 0 && shelf < kk::numCats ? shelf : byName >= 0 ? byName : kk::Harvest::classifySound (*s, rate);
    if (save)
    {
        auto dir = bankFolder().getChildFile (kk::harvestCatShort (it.cat));
        dir.createDirectory();
        auto f = dir.getNonexistentChildFile (juce::File::createLegalFileName (s->name).substring (0, 80), ".wav", false);
        juce::WavAudioFormat wav;
        auto os = std::make_unique<juce::FileOutputStream> (f);
        if (os->openedOk())
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), rate, 2, 24, {}, 0)))
            { os.release(); w->writeFromAudioSampleBuffer (s->audio, 0, s->audio.getNumSamples()); it.saved = true; it.file = f.getFullPathName(); }
    }
    bank.insert (bank.begin(), std::move (it));
    sortBank();
    ++pairVer;
}

// each shelf: newest sound on top
void KeysKillaProcessor::sortBank()
{
    std::stable_sort (bank.begin(), bank.end(), [] (const kk::HarvestItem& a, const kk::HarvestItem& b) { return a.cat != b.cat ? a.cat < b.cat : a.stamp > b.stamp; });
}
// remove a sound from the bank (and its WAV from the Bank folder)
void KeysKillaProcessor::removeFromBank (int i)
{
    if (i < 0 || i >= (int) bank.size()) return;
    if (bank[(size_t) i].file.isNotEmpty()) juce::File (bank[(size_t) i].file).deleteFile();
    bank.erase (bank.begin() + i);
    ++pairVer;
}
// put a sound on another shelf (its WAV moves to that folder too)
void KeysKillaProcessor::moveInBank (int i, int cat)
{
    if (i < 0 || i >= (int) bank.size() || cat < 0 || cat >= kk::numCats) return;
    auto& it = bank[(size_t) i];
    if (it.file.isNotEmpty())
    {
        const juce::File f (it.file);
        auto dir = bankFolder().getChildFile (kk::harvestCatShort (cat)); dir.createDirectory();
        auto to = dir.getNonexistentChildFile (f.getFileNameWithoutExtension(), ".wav", false);
        if (f.moveFileTo (to)) it.file = to.getFullPathName();
    }
    it.cat = cat;
    it.stamp = juce::Time::getHighResolutionTicks();
    sortBank();
    ++pairVer;
}
void KeysKillaProcessor::clearShelf (int cat)
{
    for (int i = (int) bank.size(); --i >= 0;) if (bank[(size_t) i].cat == cat) removeFromBank (i);
}

void KeysKillaProcessor::loadSavedBank()
{
    if (bankLoaded) return;
    bankLoaded = true;
    const double rate = sr > 0 ? sr : 44100.0;
    std::vector<kk::HarvestItem> items;
    for (int c = 0; c < kk::numCats; ++c)
        for (auto& f : bankFolder().getChildFile (kk::harvestCatShort (c)).findChildFiles (juce::File::findFiles, false, "*.wav"))
            if (auto s = kk::PairLab::fromFile (f, rate))
            {
                kk::HarvestItem it; it.sound = s; it.cat = c; it.saved = true; it.score = 10.0f; it.origin = "BANK"; it.file = f.getFullPathName(); it.stamp = f.getLastModificationTime().toMilliseconds();
                items.push_back (std::move (it));
            }
    for (auto& it : items) bank.push_back (std::move (it));
    sortBank();
    ++pairVer;
}

int KeysKillaProcessor::saveBank()
{
    const double rate = sr > 0 ? sr : 44100.0;
    int n = 0;
    for (auto& it : bank)
    {
        if (it.saved || it.sound == nullptr) continue;
        auto dir = bankFolder().getChildFile (kk::harvestCatShort (it.cat));
        dir.createDirectory();
        auto f = dir.getNonexistentChildFile (juce::File::createLegalFileName (it.sound->name).substring (0, 80), ".wav", false);
        juce::WavAudioFormat wav;
        auto os = std::make_unique<juce::FileOutputStream> (f);
        if (os->openedOk())
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), rate, 2, 24, {}, 0)))
            { os.release(); w->writeFromAudioSampleBuffer (it.sound->audio, 0, it.sound->audio.getNumSamples()); it.saved = true; it.file = f.getFullPathName(); ++n; }
    }
    ++pairVer;
    return n;
}

// ---------------- PAIR FROM VST ----------------
juce::String KeysKillaProcessor::loadVst (const juce::String& id)
{
    const auto err = vst.load (id, sr > 0 ? sr : 44100.0, std::max (64, modBlock > 0 ? modBlock : 512));
    ++pairVer;
    return err;
}

// ---- PAIR FROM VST: A / B sides ----
juce::String KeysKillaProcessor::loadVstSide (int side, const juce::String& id)
{
    side = juce::jlimit (0, 1, side);
    const auto err = host (side).load (id, sr > 0 ? sr : 44100.0, std::max (64, modBlock > 0 ? modBlock : 512));
    vstSounds[(size_t) side] = err.isEmpty() ? host (side).sounds() : std::vector<kk::VstHost::Sound> {};
    vstSel[(size_t) side] = -1;
    ++pairVer;
    return err;
}

void KeysKillaProcessor::unloadVstSide (int side)
{
    side = juce::jlimit (0, 1, side);
    host (side).unload();
    vstSounds[(size_t) side].clear(); vstSel[(size_t) side] = -1; vstTakes[(size_t) side] = 0;
    pairParents[(size_t) side] = nullptr; pairFiles[(size_t) side] = {};
    ++pairVer;
}

bool KeysKillaProcessor::pickVstSound (int side, int index, int note)
{
    side = juce::jlimit (0, 1, side);
    auto& h = host (side);
    const auto& list = vstSounds[(size_t) side];
    if (! h.loaded()) return false;
    juce::String nm = h.name();
    if (index >= 0 && index < (int) list.size())
    {
        if (! h.applySound (list[(size_t) index])) return false;
        nm << " " << list[(size_t) index].name;
    }
    auto buf = h.capture (note, 0.9f, 1.4);
    if (buf.getMagnitude (0, buf.getNumSamples()) < 1.0e-4f) return false;
    auto snd = kk::PairLab::fromBuffer (buf, h.rate(), sr > 0 ? sr : 44100.0, nm);
    if (snd == nullptr) return false;
    pairParents[(size_t) side] = snd; pairFiles[(size_t) side] = {};
    vstSel[(size_t) side] = index;
    if (index < 0)   // v0.35: a sound you TOOK from the plugin window joins this plugin's list (plugins without a sound list build one)
        if (auto* inst = h.instance())
        {
            kk::VstHost::Sound took;
            inst->getStateInformation (took.state);
            const auto pn = h.programName();
            took.name = "TAKE " + juce::String (++vstTakes[(size_t) side]) + (pn.isNotEmpty() ? "  " + pn : juce::String());
            vstSounds[(size_t) side].push_back (std::move (took));
            vstSel[(size_t) side] = (int) vstSounds[(size_t) side].size() - 1;
        }
    pairPlayer.setSound (snd);   // hear it
    if (auto* q = apvts.getParameter (ID::playMode))
    {
        const float v = q->convertTo0to1 ((float) playPair);
        if (std::abs (q->getValue() - v) > 1.0e-6f) { q->beginChangeGesture(); q->setValueNotifyingHost (v); q->endChangeGesture(); }
    }
    previewNote = snd->rootNote;
    ++pairVer;
    return true;
}

bool KeysKillaProcessor::vstSideToBank (int side, int shelf)
{
    side = juce::jlimit (0, 1, side);
    if (pairParents[(size_t) side] == nullptr) return false;
    addToBank (pairParents[(size_t) side], host (side).name(), true, shelf);
    return true;
}

juce::String KeysKillaProcessor::captureVst (int note, int shelf)
{
    if (! vst.loaded()) return {};
    auto buf = vst.capture (note, 0.9f, 1.4);
    if (buf.getMagnitude (0, buf.getNumSamples()) < 1.0e-4f) return {};
    static int counter = 0;
    const auto prog = vst.programName();
    const juce::String name = vst.name() + " " + (prog.isNotEmpty() ? prog : juce::String (++counter)) + " " + juce::MidiMessage::getMidiNoteName (note, true, true, 5);
    auto s = kk::PairLab::fromBuffer (buf, vst.rate(), sr > 0 ? sr : 44100.0, name);
    addToBank (s, vst.name(), true, shelf);
    return name;
}

juce::String KeysKillaProcessor::captureVstProgram (int program, int note)
{
    if (! vst.loaded() || program < 0 || program >= vst.numPrograms()) return {};
    vst.setProgram (program);
    return captureVst (note);
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
    drums[(size_t) d].render (kk::kindOfSlot (d), boostParams (d));
    drumSig[(size_t) d] = drumSignature (d);
}

bool KeysKillaProcessor::loadDrum (int d, const juce::File& f)
{
    while (drumBusy[(size_t) d].load()) juce::Thread::sleep (2);   // never load under a running render
    if (! drums[(size_t) d].load (f, sr > 0 ? sr : 44100.0)) return false;
    renderDrum (d);
    return true;
}

void KeysKillaProcessor::clearDrum (int d) { drums[(size_t) d].clear(); drumSig[(size_t) d] = -1; }

juce::File KeysKillaProcessor::exportDrum (int d) const
{
    static const char* tag[] { "808 BOOST", "SNARE BOOST", "HAT BOOST", "KICK BOOST", "OPEN HAT BOOST", "PERC BOOST", "FX BOOST" };
    return drums[(size_t) d].exportWav (tag[juce::jlimit (0, kk::numDrumSlots - 1, d)]);
}

void KeysKillaProcessor::hitDrum (int d, int note)
{
    if (note < 0) { auto s = drums[(size_t) d].current(); note = s ? s->rootNote : 60; }
    drumPad[(size_t) juce::jlimit (0, kk::numDrumSlots - 1, d)] = note + 1;
}

void KeysKillaProcessor::moduleHousekeeping()
{
    {
        std::vector<std::vector<kk::HarvestItem>> done;
        { const juce::SpinLock::ScopedLockType l (harvestLock); done.swap (harvestDone); }
        for (auto& d : done) { kk::Harvest::merge (bank, std::move (d)); sortBank(); ++pairVer; }
    }
    // drums re-render in the background once a knob has stopped moving for a moment (the UI never stutters)
    const double now = juce::Time::getMillisecondCounterHiRes();
    for (int d = 0; d < kk::numDrumSlots && ix != nullptr; ++d)
    {
        if (! drums[(size_t) d].hasSample() || drumBusy[(size_t) d].load()) continue;
        const int sig = drumSignature (d);
        if (sig == drumSig[(size_t) d]) { drumDirtyAt[(size_t) d] = 0; continue; }
        if (sig != drumSeenSig[(size_t) d]) { drumSeenSig[(size_t) d] = sig; drumDirtyAt[(size_t) d] = now; continue; }   // still moving
        if (now - drumDirtyAt[(size_t) d] < 120.0) continue;
        drumBusy[(size_t) d] = true;
        const auto params = boostParams (d);
        drumSig[(size_t) d] = sig;
        drumPool.addJob ([this, d, params]
        {
            drums[(size_t) d].render (kk::kindOfSlot (d), params);
            drumBusy[(size_t) d] = false;
        });
    }
}

// ---------------- PATTERNS: 808 lines, snare rolls, hi-hat rolls (generate, edit, preview, drag MIDI) ----------------
void KeysKillaProcessor::generatePattern (int d, int style, int bars, float density)
{
    d = juce::jlimit (0, kk::numDrumSlots - 1, d);
    patStyle[(size_t) d] = style; patBars[(size_t) d] = bars; patDensity[(size_t) d] = density;
    const auto seed = (uint32_t) juce::Random::getSystemRandom().nextInt (1 << 30);
    std::vector<kk::RollHit> pat;
    switch (d)
    {
        case kk::slot808:     pat = kk::make808 (seed, style, bars, density); break;
        case kk::slotSnare:   pat = kk::makeSnares (seed, style, bars, density); break;
        case kk::slotHat:     pat = kk::makeRolls (seed, style, bars, density); break;
        case kk::slotKick:    pat = kk::makeKicks (seed, style, bars, density); break;
        case kk::slotOpenHat: pat = kk::makeOpenHats (seed, style, bars, density); break;
        case kk::slotPerc:    pat = kk::makePercs (seed, style, bars, density); break;
        default:              pat = kk::makeFxHits (seed, style, bars, density); break;
    }
    setPattern (d, std::move (pat), bars);
}

void KeysKillaProcessor::setPattern (int d, std::vector<kk::RollHit> pat, int bars)
{
    d = juce::jlimit (0, kk::numDrumSlots - 1, d);
    std::sort (pat.begin(), pat.end(), [] (const kk::RollHit& a, const kk::RollHit& b) { return a.beat < b.beat; });
    const juce::SpinLock::ScopedLockType l (rollLock);
    patterns[(size_t) d].swap (pat);
    patBars[(size_t) d] = bars;
    ++patVer;
}

std::vector<kk::RollHit> KeysKillaProcessor::pattern (int d) const
{
    const juce::SpinLock::ScopedLockType l (rollLock);
    return patterns[(size_t) juce::jlimit (0, kk::numDrumSlots - 1, d)];
}

juce::File KeysKillaProcessor::exportPatternMidi (int d) const
{
    d = juce::jlimit (0, kk::numDrumSlots - 1, d);
    const auto pat = pattern (d);
    const double bpm = lastBpm.load();
    const int ppq = 960;
    auto smp = drums[(size_t) d].current();
    const int root = d == 0 ? (smp ? smp->rootNote : 36) : 60;   // 808: its own note, so the MIDI plays it in tune
    juce::MidiMessageSequence seq;
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)); tempo.setTimeStamp (0); seq.addEvent (tempo);
    for (auto& h : pat)
    {
        const int note = juce::jlimit (0, 127, root + h.semi);
        seq.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, (int) std::lround (h.vel * 127.0f))), std::round (h.beat * ppq));
        seq.addEvent (juce::MidiMessage::noteOff (1, note), std::round ((h.beat + std::max (0.02, h.len)) * ppq));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq); mf.addTrack (seq);
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Loops");
    dir.createDirectory();
    static const char* tag[] { "808", "Snare", "Hats", "Kick", "Open Hat", "Perc", "FX" };
    auto f = dir.getChildFile ("KK " + juce::String (tag[d]) + " pattern " + juce::String (patVer.load()) + " - " + juce::String (juce::roundToInt (bpm)) + "BPM.mid");
    f.deleteFile();
    if (juce::FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
    return f;
}

// audio thread: the open drum page's pattern - or the whole DRUM KIT - plays the boosted samples (host tempo, or free when stopped)
void KeysKillaProcessor::renderPatterns (int n, double beatPos, double bps, bool hostPlaying)
{
    int mask = 0;
    if (kitPlay.load()) { for (int d = 0; d < kk::numDrumSlots; ++d) if (drums[(size_t) d].current() != nullptr) mask |= 1 << d; }
    else if (const int one = patPlay.load(); one >= 0 && one < kk::numDrumSlots) mask = 1 << one;
    if (mask == 0)
    {
        for (int d = 0; d < kk::numDrumSlots; ++d) if (patMaskWas & (1 << d)) drums[(size_t) d].allOff();
        patMaskWas = 0; pat808Off = -1;
        return;
    }
    const juce::SpinLock::ScopedTryLockType tl (rollLock);
    if (! tl.isLocked()) return;
    if (patMaskWas == 0 || hostPlaying != patHostWas) { patOrigin = hostPlaying ? 0.0 : beatPos; patHostWas = hostPlaying; }
    for (int d = 0; d < kk::numDrumSlots; ++d)
        if ((patMaskWas & (1 << d)) && ! (mask & (1 << d))) drums[(size_t) d].allOff();
    patMaskWas = mask;
    const double start = beatPos - patOrigin, end = start + bps * n;
    double shown = 0;
    for (int d = 0; d < kk::numDrumSlots; ++d)
    {
        if (! (mask & (1 << d))) continue;
        const auto& pat = patterns[(size_t) d];
        const double len = std::max (1, patBars[(size_t) d]) * 4.0;
        auto smp = drums[(size_t) d].current();
        const int root = smp ? smp->rootNote : 60;
        for (const auto& h : pat)
        {
            const double m = std::ceil ((start - h.beat) / len);
            for (double t = h.beat + m * len; t < end; t += len)
                if (t >= start)
                {
                    const int at = juce::jlimit (0, n - 1, (int) ((t - start) / bps));
                    if (d == 0) { drums[0].allOff(); pat808Off = at + (int) (std::max (0.05, h.len) / bps); pat808Note = root + h.semi; }   // an 808 is mono and holds its length
                    drums[(size_t) d].noteOn (root + h.semi, h.vel, at);
                }
        }
        if (d == patPlay.load() || shown == 0) shown = std::fmod (std::max (0.0, start), len);
    }
    if ((mask & 1) && pat808Off >= 0)
    {
        if (pat808Off < n) { drums[0].noteOff (pat808Note); pat808Off = -1; }
        else pat808Off -= n;
    }
    patBeat = (float) shown;
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
    setPlayMode (playKeys);
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
    genomeParentSet (slot);
}
void KeysKillaProcessor::clearParent (int slot) { parents[(size_t) juce::jlimit (0, 1, slot)] = Genome(); ++labVer; genomeParentSet (slot); }
void KeysKillaProcessor::setParentCurrent (int slot) { parents[(size_t) juce::jlimit (0, 1, slot)] = genomeFromCurrent(); ++labVer; genomeParentSet (slot); }
void KeysKillaProcessor::setParentChild (int slot, int c)
{
    if (! juce::isPositiveAndBelow (c, (int) children.size())) return;
    parents[(size_t) juce::jlimit (0, 1, slot)] = children[(size_t) c].g; ++labVer;
    genomeParentSet (slot);
}

//==============================================================================
// v0.35 BREED LAB with your own sounds: a WAV dropped on PARENT A / B. Then the lab breeds audio (the PAIR engine):
// a sound from the bank in the other slot is rendered to audio first, so any mix works (bank x WAV, WAV x WAV).
juce::AudioBuffer<float> KeysKillaProcessor::renderGenomeAudio (const Genome& g, double rate, double seconds)
{
    if (thumbRenderer == nullptr) thumbRenderer = std::make_unique<KeysKillaProcessor> (false);
    auto& r = *thumbRenderer;
    for (size_t i = 0; i < r.params.size() && i < g.v.size(); ++i)
    {
        const float v = ID::isModuleParam (r.params[i]->paramID) || r.params[i]->paramID == ID::playMode ? r.params[i]->getDefaultValue() : g.v[i];
        if (std::abs (r.params[i]->getValue() - v) > 1.0e-6f) r.params[i]->setValueNotifyingHost (v);
    }
    r.eco = false;
    const int block = 512, total = (int) (rate * seconds), offAt = (int) (rate * seconds * 0.62);
    r.prepareToPlay (rate, block);
    const int note = r.raw[(size_t) r.ix->bassMode]->load() > 0.5f ? 36 : 60;
    juce::AudioBuffer<float> out (2, total), buf (2, block);
    for (int pos = 0; pos < total; pos += block)
    {
        juce::MidiBuffer m;
        if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
        if (pos <= offAt && offAt < pos + block) m.addEvent (juce::MidiMessage::noteOff (1, note), offAt - pos);
        buf.clear();
        r.processBlock (buf, m);
        const int n = std::min (block, total - pos);
        for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, buf, ch, 0, n);
    }
    return out;
}

void KeysKillaProcessor::genomeParentSet (int slot)
{
    const auto s = (size_t) juce::jlimit (0, 1, slot);
    labWav[s] = false; labAudioParents[s] = nullptr;
    labAudio = labWav[0] || labWav[1];
}

bool KeysKillaProcessor::labDropFile (int slot, const juce::File& f)
{
    const auto s = (size_t) juce::jlimit (0, 1, slot);
    auto snd = kk::PairLab::fromFile (f, sr > 0 ? sr : 44100.0);
    if (snd == nullptr) return false;
    labAudioParents[s] = snd; labWav[s] = true; labWavFile[s] = f.getFullPathName();
    parents[s] = Genome();   // the slot now holds your sound
    labAudio = true;
    ++labVer;
    return true;
}

kk::PairPtr KeysKillaProcessor::labParentAudio (int slot)
{
    const auto s = (size_t) juce::jlimit (0, 1, slot);
    if (labAudioParents[s] == nullptr && parents[s].valid())   // a bank sound: render it once
    {
        const double rate = sr > 0 ? sr : 44100.0;
        labAudioParents[s] = kk::PairLab::fromBuffer (renderGenomeAudio (parents[s], rate, 2.6), rate, rate, parents[s].name);
    }
    return labAudioParents[s];
}

juce::String KeysKillaProcessor::labParentName (int slot) const
{
    const auto s = (size_t) juce::jlimit (0, 1, slot);
    if (labWav[s] && labAudioParents[s] != nullptr) return labAudioParents[s]->name;
    return parents[s].valid() ? parents[s].name : juce::String();
}

void KeysKillaProcessor::labBreedAudio()
{
    std::vector<kk::PairPtr> ps;
    for (int k = 0; k < 2; ++k) if (auto p = labParentAudio (k)) ps.push_back (p);
    if (ps.size() < 2) return;
    pairSeed = kk::hash32 (pairSeed + (uint32_t) juce::Time::getMillisecondCounter());
    pairKids = kk::PairLab::breed (ps, pairSeed, pairFlavor, sr > 0 ? sr : 44100.0);
    pairKidGenes.assign (pairKids.size(), {});
    {
        juce::Random r ((juce::int64) pairSeed);
        for (auto& g : pairKidGenes) for (int k = 0; k < numGenes; ++k) g[(size_t) k] = geneLock[(size_t) k] ? geneLockSrc[(size_t) k] : r.nextInt (2);
    }
    pairSel = -1; pairLoopKid = -1;
    if (loopOn.load() && loopOwner == 2) loopOn = false;
    ++pairVer; ++labVer;
    if (! pairKids.empty()) selectPairKid (0, false);
}

void KeysKillaProcessor::auditionAncestor (int slot)
{
    const auto& a = ancestor (slot);
    if (! a.valid()) return;
    applyGenome (a, true);
    previewNote = raw[(size_t) ix->bassMode]->load() > 0.5f ? 36 : 60;
}

//==============================================================================
// v0.36 EVOLVE
void KeysKillaProcessor::evoReset() { evo.clear(); evoCenter = -1; evoSeedFile.clear(); ++evoVer; }

juce::String KeysKillaProcessor::evoName (const EvoNode& parent, int k, uint32_t seed) const
{
    // names say how it sounds: a character word + what it is (from the seed's category)
    static const char* words[] { "Glassy", "Dusty", "Wide", "Bright", "Dark", "Hollow", "Velvet", "Broken", "Liquid", "Frozen", "Warm", "Wild",
                                 "Airy", "Gritty", "Soft", "Metal", "Ghost", "Silk", "Neon", "Deep", "Tape", "Crystal", "Lunar", "Smoky" };
    static const char* nouns[] { "Piano", "Keys", "Bell", "Pluck", "Mallet", "Guitar", "Strings", "Brass", "Choir", "Flute", "Lead", "Pad",
                                 "Synth", "Bass", "808", "Texture", "Arp", "FX", "Organ", "Chip", "World", "Drum", "Game FX", "Score" };
    const int cat = parent.isAudio() ? -1 : parent.g.cat;
    const juce::String noun = juce::isPositiveAndBelow (cat, 24) ? nouns[cat] : juce::String ("Sound");
    const int w = (int) ((seed >> 3) + (uint32_t) k * 7u) % 24;
    return juce::String (words[w]) + " " + noun + " " + juce::String (parent.gen + 1) + "." + juce::String (k + 1);
}

void KeysKillaProcessor::evoSeedPreset (int idx)
{
    auto g = genomeFromPreset (idx);
    if (! g.valid()) return;
    evoReset();
    EvoNode n; n.g = g; n.name = g.name; n.fx = fxCurrent(); evo.push_back (n);
    evoFocus (0);
}

void KeysKillaProcessor::evoSeedCurrent()
{
    auto g = genomeFromCurrent();
    if (! g.valid()) return;
    evoReset();
    EvoNode n; n.g = g; n.name = g.name; n.fx = fxCurrent(); evo.push_back (n);
    evoFocus (0);
}

bool KeysKillaProcessor::evoSeedFromFile (const juce::File& f)
{
    auto snd = kk::PairLab::fromFile (f, sr > 0 ? sr : 44100.0);
    if (snd == nullptr) return false;
    evoReset();
    EvoNode n; n.audio = snd; n.name = snd->name; n.fx = fxCurrent(); evo.push_back (n);
    evoSeedFile = f.getFullPathName();
    evoFocus (0);
    return true;
}

void KeysKillaProcessor::evoSeedSound (kk::PairPtr snd)
{
    if (snd == nullptr) return;
    auto f = sessionDir().getChildFile (juce::File::createLegalFileName (snd->name).substring (0, 60) + " seed " + juce::String ((juce::int64) juce::Time::currentTimeMillis() % 100000) + ".wav");
    evoReset();
    EvoNode n; n.audio = snd; n.name = snd->name; evo.push_back (n);
    if (writeWavFile (snd->audio, sr > 0 ? sr : 44100.0, f)) evoSeedFile = f.getFullPathName();   // kept with the project
    evoFocus (0);
}

void KeysKillaProcessor::evoNewMelody (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return;
    auto& n = evo[(size_t) node];
    const auto base = n.g.loop.valid ? n.g.loop : kk::loopFromSeed ((uint32_t) n.name.hashCode());
    n.g.loop = kk::rerollLoop (base, kk::hash32 ((uint32_t) juce::Time::getHighResolutionTicks() ^ (uint32_t) (node * 7919 + 1)));
    n.g.loop.valid = true;
    if (node == evoCenter) setCurrentLoop (n.g.loop);
    ++evoVer;
}

void KeysKillaProcessor::evoSeedRandom()
{
    juce::Random r ((juce::int64) juce::Time::getHighResolutionTicks());
    evoSeedPreset (r.nextInt ((int) factoryPresets().size()));
}

// v0.38: a melody that changes a little (SAFE) or a lot (WILD) - always C minor
static kk::LoopGenes evoMutateLoop (const kk::LoopGenes& parent, uint32_t seed, float wild)
{
    kk::LoopGenes c = parent.valid ? parent : kk::loopFromSeed (seed);
    kk::Rng r; r.seed (kk::hash32 (seed ^ 0x3c6ef372u));
    bool changed = false;
    for (int i = 0; i < kk::numLoopGenes; ++i)
        if (r.uni() < 0.12f + 0.45f * wild) { c.g[(size_t) i] = kk::randomGene (i, r); changed = true; }
    if (! changed) c.g[kk::loopVariation] = kk::randomGene (kk::loopVariation, r);
    kk::fixLoop (c);
    c.key = 0; c.valid = true;
    return c;
}

void KeysKillaProcessor::evoGrow (int node, bool reroll)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return;
    if (! evo[(size_t) node].kids.empty() && ! reroll) return;
    if (! tasteLoaded) tasteLoad();
    evo[(size_t) node].kids.clear();
    const uint32_t base = kk::hash32 ((uint32_t) juce::Time::getMillisecondCounter() * 2654435761u + (uint32_t) node * 97u);
    juce::Random rnd ((juce::int64) base);
    const auto& ps = factoryPresets();
    const float wild = juce::jlimit (0.0f, 1.0f, evoWild);
    const auto parent = evo[(size_t) node];   // copy: evo grows below
    const bool useTaste = evoTasteAmt > 0.02f && taste.nLike >= 2.0f;
    int parentFx = 0; for (auto o : parent.fx.on) parentFx += o ? 1 : 0;
    const auto parentLoop = parent.g.loop.valid ? parent.g.loop : kk::loopFromSeed ((uint32_t) parent.name.hashCode());
    std::vector<EvoNode> cand;
    auto finishCandidate = [&] (EvoNode& n, int k, uint32_t seed)
    {
        n.parent = node; n.gen = parent.gen + 1;
        n.name = evoName (parent, k % 6, seed);
        if (! n.isAudio()) n.g.name = n.name;
        // IDEA: the melody and the effects are inherited too
        n.g.loop = evoMutateLoop (parentLoop, seed * 7u + 3u, wild);
        if (parentFx > 0) n.fx = k % 6 == 0 ? parent.fx : fxMutate (parent.fx, juce::jlimit (0.0f, 1.0f, wild * 0.8f + (k % 6 >= 4 ? 0.25f : 0.0f)), seed * 13u + 1u);
        else if (k % 6 >= 2 || wild > 0.6f) n.fx = fxSurprise (seed * 17u + 5u);
        n.fx.name = n.fx.name.isEmpty() ? juce::String() : n.fx.name;
    };
    if (! parent.isAudio())
    {
        const float keepWild = breedWild;
        const int count = useTaste ? 18 : 6;
        for (int k = 0; k < count; ++k)
        {
            // SAFE: mostly the sound itself, a little changed.  WILD: crosses with other sounds, further from home
            Genome partner = parent.g;
            const int kk6 = k % 6;
            const bool cross = kk6 >= 2 || wild > 0.75f;
            if (cross)
            {
                int pick = rnd.nextInt ((int) ps.size());
                const bool sameFamily = kk6 < 4 && wild < 0.6f;
                for (int t = 0; t < 40 && sameFamily && ps[(size_t) pick].cat != parent.g.cat; ++t) pick = rnd.nextInt ((int) ps.size());
                partner = genomeFromPreset (pick);
            }
            breedWild = juce::jlimit (0.0f, 1.0f, wild * (0.55f + 0.15f * (float) kk6));
            const uint32_t seed = kk::hash32 (base + (uint32_t) k * 7919u);
            auto c = makeChildOf (parent.g, partner, kk6, seed, nullptr, false);
            EvoNode n;
            n.g = c.g; n.g.gen = parent.g.gen + 1; n.g.cat = parent.g.cat;
            finishCandidate (n, k, seed);
            cand.push_back (n);
        }
        breedWild = keepWild;
    }
    else
    {
        const double rate = sr > 0 ? sr : 44100.0;
        const int rounds = useTaste && evoTasteAmt > 0.6f ? 2 : 1;   // your own sounds take longer to breed: a second round only for a strong taste
        for (int round = 0; round < rounds; ++round)
        {
            std::vector<kk::PairPtr> ps2 { parent.audio };
            // the partner: SAFE = the sound itself (shaped), WILD = a sound from the bank rendered to audio
            if (wild > 0.25f)
            {
                auto g = genomeFromPreset (rnd.nextInt ((int) ps.size()));
                ps2.push_back (kk::PairLab::fromBuffer (renderGenomeAudio (g, rate, 2.4), rate, rate, g.name));
            }
            else ps2.push_back (parent.audio);
            const int flavor = wild < 0.35f ? (int) kk::flavorClean : wild < 0.7f ? (int) kk::flavorAny : (rnd.nextBool() ? (int) kk::flavorBit : (int) kk::flavorAtmos);
            auto kids = kk::PairLab::breed (ps2, base + (uint32_t) round * 104729u, flavor, rate);
            for (int k = 0; k < (int) kids.size() && k < 6; ++k)
            {
                EvoNode n; n.audio = kids[(size_t) k];
                n.waveReady = true;
                for (int b = 0; b < 64 && ! n.audio->peaks.empty(); ++b) n.wave[(size_t) b] = n.audio->peaks[(size_t) (b * (int) n.audio->peaks.size() / 64)];
                finishCandidate (n, k + round * 6, base + (uint32_t) (k + round * 6));
                cand.push_back (n);
            }
        }
    }
    // MY TASTE: from the candidates keep six - the ones you would like, but never six of the same
    std::vector<int> chosen;
    if (! useTaste || (int) cand.size() <= 6) for (int i = 0; i < (int) cand.size() && i < 6; ++i) chosen.push_back (i);
    else
    {
        std::vector<std::array<float, tasteDims>> d; std::vector<float> sc;
        for (auto& c : cand) { d.push_back (evoDescribe (c)); sc.push_back (tasteScore (d.back())); }
        const float lo = *std::min_element (sc.begin(), sc.end()), hi = *std::max_element (sc.begin(), sc.end());
        for (auto& v : sc) v = hi > lo ? (v - lo) / (hi - lo) : 0.5f;
        const float T = juce::jlimit (0.0f, 1.0f, evoTasteAmt);
        std::vector<bool> used (cand.size(), false);
        for (int pick = 0; pick < 6; ++pick)
        {
            int best = -1; float bestV = -1e9f;
            for (int i = 0; i < (int) cand.size(); ++i)
            {
                if (used[(size_t) i]) continue;
                float minD = 1.0f;
                for (int j : chosen) { float dd = 0; for (int q = 0; q < tasteDims; ++q) dd += (d[(size_t) i][(size_t) q] - d[(size_t) j][(size_t) q]) * (d[(size_t) i][(size_t) q] - d[(size_t) j][(size_t) q]); minD = std::min (minD, std::sqrt (dd) * 0.5f); }
                const float v = T * sc[(size_t) i] + (1.0f - T) * rnd.nextFloat() + 0.35f * minD;
                if (v > bestV) { bestV = v; best = i; }
            }
            if (best < 0) break;
            used[(size_t) best] = true; chosen.push_back (best);
        }
    }
    for (int i : chosen)
    {
        auto n = cand[(size_t) i];
        n.name = evoName (parent, (int) evo[(size_t) node].kids.size(), base + (uint32_t) i * 31u);
        if (! n.isAudio()) n.g.name = n.name;
        evo.push_back (n);
        evo[(size_t) node].kids.push_back ((int) evo.size() - 1);
    }
    ++evoVer;
}

// ---------------- v0.38 IDEA + MY TASTE ----------------
void KeysKillaProcessor::evoApplyIdea (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return;
    fxApply (evo[(size_t) node].fx);
}

void KeysKillaProcessor::evoPick (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return;
    tasteLearn (evo[(size_t) node], 1.0f);
    const int pa = evo[(size_t) node].parent;
    if (juce::isPositiveAndBelow (pa, (int) evo.size()) && pa == evoCenter)   // the ones you passed by: a little less of them
        for (int k : evo[(size_t) pa].kids) if (k != node && juce::isPositiveAndBelow (k, (int) evo.size())) tasteLearn (evo[(size_t) k], -0.2f);
    evoFocus (node);
}

void KeysKillaProcessor::evoNotMyTaste (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size()) || node == evoCenter) return;
    tasteLearn (evo[(size_t) node], -1.5f);
    const int pa = evo[(size_t) node].parent;
    if (juce::isPositiveAndBelow (pa, (int) evo.size()))
    {
        auto& ks = evo[(size_t) pa].kids;
        ks.erase (std::remove (ks.begin(), ks.end(), node), ks.end());
        if (ks.size() < 3) evoGrow (pa, true);
    }
    ++evoVer;
}

std::array<float, KeysKillaProcessor::tasteDims> KeysKillaProcessor::evoDescribe (const EvoNode& n) const
{
    std::array<float, tasteDims> d {};
    auto c01 = [] (float x) { return juce::jlimit (0.0f, 1.0f, x); };
    if (! n.isAudio() && n.g.v.size() == params.size())
    {
        auto v = [&] (const char* id) { const int i = indexOf (id); return i >= 0 ? n.g.v[(size_t) i] : 0.0f; };
        d[0] = v (ID::cutoff); d[1] = v (ID::attack); d[2] = v (ID::release); d[3] = c01 (v (ID::revMix) + 0.5f * v (ID::delayMix));
        d[4] = c01 (std::max (v (ID::drive), v (ID::crush))); d[5] = c01 (std::max (v (ID::lfoFilter), v (ID::chorus)));
        d[6] = v (ID::width); d[7] = c01 (0.5f * (1.0f - v (ID::octave)) + 0.5f * (v (ID::bassMode) > 0.5f ? 1.0f : 0.0f));
    }
    else if (n.isAudio() && n.audio->audio.getNumSamples() > 64)
    {
        const auto& a = n.audio->audio;
        const int len = a.getNumSamples(), step = std::max (1, len / 40000);
        const double rate = sr > 0 ? sr : 44100.0;
        int cross = 0, cnt = 0, peakAt = 0; float pk = 0, last = 0; double e = 0, eTail = 0, side = 0, mid = 0, low = 0; float lp = 0;
        const float lpk = 1.0f - std::exp (-kk::twoPi * 200.0f * (float) step / (float) rate);
        for (int i = 0; i < len; i += step)
        {
            const float l = a.getSample (0, i), r = a.getSample (1, i), m = 0.5f * (l + r);
            if ((m > 0) != (last > 0)) ++cross;
            last = m; ++cnt;
            if (std::abs (m) > pk) { pk = std::abs (m); peakAt = i; }
            e += m * m; if (i > len * 6 / 10) eTail += m * m;
            side += 0.25 * (l - r) * (l - r); mid += m * m;
            lp += (m - lp) * lpk; low += lp * lp;
        }
        const float rms = (float) std::sqrt (e / std::max (1, cnt)) + 1.0e-6f;
        d[0] = c01 ((float) cross / (float) std::max (1, cnt) * (float) step * 6.0f);
        d[1] = c01 ((float) (peakAt / rate) / 0.3f);
        d[2] = c01 ((float) (len / rate) / 4.0f);
        d[3] = c01 ((float) (eTail / std::max (1.0e-9, e)) * 3.0f);
        d[4] = c01 ((8.0f - pk / rms) / 6.0f);
        d[5] = 0.3f;
        d[6] = c01 ((float) std::sqrt (side / std::max (1.0e-9, mid)));
        d[7] = c01 ((float) std::sqrt (low / std::max (1.0e-9, e)));
    }
    for (int s = 0; s < kk::numRackSlots; ++s) d[(size_t) (8 + s)] = n.fx.on[(size_t) s] ? 1.0f : 0.0f;
    const auto& l = n.g.loop;
    if (l.valid)
    {
        d[19] = (float) (l.g[kk::loopDensity] % 3) / 2.0f;
        d[20] = (float) (l.g[kk::loopRegister] % 3) / 2.0f;
        d[21] = (float) (l.g[kk::loopShape] % 5) / 4.0f;
    }
    return d;
}

static float tasteDist (const std::array<float, KeysKillaProcessor::tasteDims>& a, const std::array<float, KeysKillaProcessor::tasteDims>& b)
{
    float s = 0;
    for (int i = 0; i < KeysKillaProcessor::tasteDims; ++i)
    {
        const float w = i < 8 ? 1.0f : i < 19 ? 0.45f : 0.5f;   // the sound counts most, then the melody, then the effects
        s += w * (a[(size_t) i] - b[(size_t) i]) * (a[(size_t) i] - b[(size_t) i]);
    }
    return std::sqrt (s);
}

float KeysKillaProcessor::tasteScore (const std::array<float, tasteDims>& d) const
{
    if (taste.nLike < 1.0f) return 0.0f;
    float s = -tasteDist (d, taste.like);
    if (taste.nDislike >= 1.0f) s += 0.6f * tasteDist (d, taste.dislike);
    return s;
}

void KeysKillaProcessor::tasteLearn (const EvoNode& n, float w)
{
    if (! tasteLoaded) tasteLoad();
    const auto d = evoDescribe (n);
    auto& target = w > 0 ? taste.like : taste.dislike;
    auto& count = w > 0 ? taste.nLike : taste.nDislike;
    const float aw = std::abs (w);
    const float k = aw / (std::min (count, 40.0f) + aw);   // the last ~40 picks count: your taste can move
    for (int i = 0; i < tasteDims; ++i) target[(size_t) i] += (d[(size_t) i] - target[(size_t) i]) * k;
    count += aw;
    tasteSave();
}

static juce::File tasteFile() { return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("taste.txt"); }

void KeysKillaProcessor::tasteLoad()
{
    tasteLoaded = true;
    const auto lines = juce::StringArray::fromLines (tasteFile().loadFileAsString());
    if (lines.size() < 3) return;
    const auto head = juce::StringArray::fromTokens (lines[0], " ", "");
    const auto lk = juce::StringArray::fromTokens (lines[1], ",", ""), dk = juce::StringArray::fromTokens (lines[2], ",", "");
    if (head.size() < 2 || lk.size() != tasteDims || dk.size() != tasteDims) return;
    taste.nLike = head[0].getFloatValue(); taste.nDislike = head[1].getFloatValue();
    for (int i = 0; i < tasteDims; ++i) { taste.like[(size_t) i] = lk[i].getFloatValue(); taste.dislike[(size_t) i] = dk[i].getFloatValue(); }
}

void KeysKillaProcessor::tasteSave() const
{
    juce::String t; t << juce::String (taste.nLike, 3) << " " << juce::String (taste.nDislike, 3) << "\n";
    juce::StringArray a, b;
    for (int i = 0; i < tasteDims; ++i) { a.add (juce::String (taste.like[(size_t) i], 4)); b.add (juce::String (taste.dislike[(size_t) i], 4)); }
    t << a.joinIntoString (",") << "\n" << b.joinIntoString (",") << "\n";
    auto f = tasteFile(); f.getParentDirectory().createDirectory();
    f.replaceWithText (t);
}

namespace
{
struct FixedTempoHead : public juce::AudioPlayHead
{
    double bpm = 140.0;
    juce::Optional<PositionInfo> getPosition() const override { PositionInfo p; p.setBpm (bpm); p.setIsPlaying (false); return p; }
};
}

juce::AudioBuffer<float> KeysKillaProcessor::renderIdea (int node, double rate)
{
    juce::AudioBuffer<float> out;
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return out;
    const auto n = evo[(size_t) node];
    if (thumbRenderer == nullptr) thumbRenderer = std::make_unique<KeysKillaProcessor> (false);
    auto& r = *thumbRenderer;
    FixedTempoHead head; head.bpm = juce::jlimit (40.0, 300.0, lastBpm.load());
    if (! n.isAudio())
        for (size_t i = 0; i < r.params.size() && i < n.g.v.size(); ++i)
        {
            const float v = ID::isModuleParam (r.params[i]->paramID) || r.params[i]->paramID == ID::playMode ? r.params[i]->getDefaultValue() : n.g.v[i];
            if (std::abs (r.params[i]->getValue() - v) > 1.0e-6f) r.params[i]->setValueNotifyingHost (v);
        }
    r.eco = false;
    const int block = 512;
    r.prepareToPlay (rate, block);
    if (n.isAudio()) r.useSample (n.audio, false); else r.setPlayMode (playKeys);
    r.fxApply (n.fx);
    r.setPlayHead (&head);
    r.loopBarsN = 4;
    r.curLoop = n.g.loop.valid ? n.g.loop : kk::loopFromSeed ((uint32_t) n.name.hashCode());
    r.rebuildLoopSeq();
    r.loopOwner = 0; r.loopOn = true;
    const int body = (int) std::round (rate * 60.0 / head.bpm * 4.0 * r.loopBarsN), total = body + (int) (rate * 2.0);
    out.setSize (2, total); out.clear();
    juce::AudioBuffer<float> buf (2, block);
    for (int pos = 0; pos < total; pos += block)
    {
        if (pos >= body) r.loopOn = false;   // the last notes ring out
        juce::MidiBuffer m;
        buf.clear();
        r.processBlock (buf, m);
        const int len = std::min (block, total - pos);
        for (int c = 0; c < 2; ++c) out.copyFrom (c, pos, buf, c, 0, len);
    }
    // the renderer goes back to its quiet self (it also draws the waveforms)
    r.loopOn = false; r.rack.reset(); r.setPlayHead (nullptr); r.pairPlayer.setSound (nullptr); r.setPlayMode (playKeys); r.loopBarsN = 8;
    const float pk = out.getMagnitude (0, total);
    if (pk > 0.98f) out.applyGain (0.95f / pk);
    const int fade = (int) (rate * 0.5);
    for (int c = 0; c < 2; ++c) out.applyGainRamp (c, total - fade, fade, 1.0f, 0.0f);
    return out;
}

juce::File KeysKillaProcessor::evoExportIdea (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return {};
    const double rate = sr > 0 ? sr : 44100.0;
    auto b = renderIdea (node, rate);
    if (b.getNumSamples() < 64) return {};
    tasteLearn (evo[(size_t) node], 0.7f);
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("EVOLVE Ideas")
                 .getChildFile (juce::File::createLegalFileName ("Idea - " + evo[(size_t) node].name + " - C MIN " + juce::String (juce::roundToInt (lastBpm.load())) + "BPM") + ".wav");
    return writeWavFile (b, rate, f) ? f : juce::File();
}

void KeysKillaProcessor::evoAudition (int node, bool preview)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return;
    const auto& n = evo[(size_t) node];
    if (n.isAudio())
    {
        useSample (n.audio, preview);
        setCurrentLoop (n.g.loop.valid ? n.g.loop : kk::loopFromSeed ((uint32_t) n.name.hashCode()));
    }
    else
    {
        applyGenome (n.g, true);
        setCurrentLoop (n.g.loop.valid ? n.g.loop : kk::loopFromSeed ((uint32_t) n.name.hashCode()));
        if (preview) previewNote = raw[(size_t) ix->bassMode]->load() > 0.5f ? 36 : 60;
    }
}

void KeysKillaProcessor::evoFocus (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return;
    evoCenter = node;
    evoGrow (node, false);
    evoAudition (node);
    if (evoIdea) evoApplyIdea (node);
    if (! evo[(size_t) node].isAudio()) captureUndo();
    ++evoVer;
}

void KeysKillaProcessor::evoMorph (int a, int b, float t)
{
    if (! juce::isPositiveAndBelow (a, (int) evo.size()) || ! juce::isPositiveAndBelow (b, (int) evo.size())) return;
    const auto& A = evo[(size_t) a]; const auto& B = evo[(size_t) b];
    if (A.isAudio() || B.isAudio() || A.g.v.size() != B.g.v.size()) return;
    Genome m = A.g;
    t = juce::jlimit (0.0f, 1.0f, t);
    for (size_t i = 0; i < m.v.size(); ++i) m.v[i] = A.g.v[i] + (B.g.v[i] - A.g.v[i]) * t;
    applyGenome (m, false);
}

kk::PairPtr KeysKillaProcessor::evoAsSound (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return nullptr;
    const auto& n = evo[(size_t) node];
    if (n.isAudio()) return n.audio;
    const double rate = sr > 0 ? sr : 44100.0;
    return kk::PairLab::fromBuffer (renderGenomeAudio (n.g, rate, 3.0), rate, rate, n.name);
}

juce::File KeysKillaProcessor::evoExportWav (int node)
{
    if (juce::isPositiveAndBelow (node, (int) evo.size())) tasteLearn (evo[(size_t) node], 0.7f);   // you took it: MY TASTE learns
    auto snd = evoAsSound (node);
    return snd != nullptr ? kk::PairLab::exportWav (*snd, sr > 0 ? sr : 44100.0, snd->name) : juce::File();
}

juce::File KeysKillaProcessor::evoExportMidi (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) evo.size())) return {};
    Genome g = evo[(size_t) node].g;
    g.name = evo[(size_t) node].name;
    if (! g.loop.valid) g.loop = kk::loopFromSeed ((uint32_t) g.name.hashCode());
    tasteLearn (evo[(size_t) node], 0.5f);
    return exportLoopMidi (g);
}

std::vector<int> KeysKillaProcessor::evoPath() const
{
    std::vector<int> p;
    for (int n = evoCenter; n >= 0 && n < (int) evo.size(); n = evo[(size_t) n].parent) p.insert (p.begin(), n);
    return p;
}

kk::PairPtr KeysKillaProcessor::childAsSound (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) children.size())) return nullptr;
    const double rate = sr > 0 ? sr : 44100.0;
    return kk::PairLab::fromBuffer (renderGenomeAudio (children[(size_t) i].g, rate, 3.0), rate, rate, children[(size_t) i].g.name);
}

juce::File KeysKillaProcessor::exportChildWav (int i)
{
    auto snd = childAsSound (i);
    if (snd == nullptr) return {};
    return kk::PairLab::exportWav (*snd, sr > 0 ? sr : 44100.0, snd->name);
}

void KeysKillaProcessor::auditionParent (int slot)
{
    const auto s = (size_t) juce::jlimit (0, 1, slot);
    if (labWav[s] && labAudioParents[s] != nullptr) { useSample (labAudioParents[s], true); return; }
    if (! parents[s].valid()) return;
    applyGenome (parents[s], true);   // the parent becomes the sound on the keys - hear it, play it
    previewNote = raw[(size_t) ix->bassMode]->load() > 0.5f ? 36 : 60;
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
    setPlayMode (playKeys);   // v0.37: a bank / bred sound you pick is what the keys play
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
    if (labAudioMode())   // v0.37: your own sounds - the child is spliced from A and B, gene by gene
    {
        if (! juce::isPositiveAndBelow (c, (int) pairKids.size()) || ! juce::isPositiveAndBelow (gene, (int) numGenes)) return;
        if ((int) pairKidGenes.size() != (int) pairKids.size()) pairKidGenes.resize (pairKids.size());
        auto genes = pairKidGenes[(size_t) c];
        genes[(size_t) gene] = juce::jlimit (0, 1, src);
        auto A = labParentAudio (0), B = labParentAudio (1);
        if (A == nullptr || B == nullptr) return;
        const std::array<int, 6> g6 { genes[0], genes[1], genes[2], genes[3], genes[4], genes[5] };
        auto kid = kk::PairLab::splice (*A, *B, g6, sr > 0 ? sr : 44100.0, A->name.substring (0, 14) + " x " + B->name.substring (0, 14));
        if (kid == nullptr) return;
        pairKids[(size_t) c] = kid; pairKidGenes[(size_t) c] = genes;
        if (geneLock[(size_t) gene]) geneLockSrc[(size_t) gene] = genes[(size_t) gene];
        selectPairKid (c, true);
        return;
    }
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
    if (const int src = geneOfSelected (gene); src >= 0) geneLockSrc[(size_t) gene] = src;
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

void KeysKillaProcessor::playChild (int i)
{
    if (! juce::isPositiveAndBelow (i, (int) children.size())) return;
    if (! mainLoopMode) { previewChild (i); return; }
    if (loopIsChild (i)) { stopLoop(); ++labVer; return; }
    if (i != selChild) selectChild (i);
    setCurrentLoop (children[(size_t) i].g.loop);
    loopOwner = 3;
    if (! loopOn) toggleLoop(); else rebuildLoopSeq();
    ++labVer;
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
    juce::ignoreUnused (l);
    return 0;   // v0.37: every melody in C minor - the sounds sit on C, so melodies and sounds layer in the channel rack

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
    if (! bank.empty())   // the bank first: sounds harvested from your songs
    {
        const int b = juce::Random::getSystemRandom().nextInt ((int) bank.size());
        bankToPair (b, slot);
        return bank[(size_t) b].sound->name;
    }
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
    auto title = juce::MidiMessage::textMetaEvent (3, "BREED LAB loop"); title.setTimeStamp (0); seq.addEvent (title);
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
    {
        // the picture is always the synth itself: keys routing (PAIR / VST / drums / DIGGA) and the modules stay at default
        const float v = ID::isModuleParam (r.params[i]->paramID) ? r.params[i]->getDefaultValue() : g.v[i];
        if (std::abs (r.params[i]->getValue() - v) > 1.0e-6f) r.params[i]->setValueNotifyingHost (v);
    }
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
    if (evoActive)   // EVOLVE: the waveforms of the sounds on screen first (the middle, its kids, the path)
    {
        std::vector<int> want;
        if (juce::isPositiveAndBelow (evoCenter, (int) evo.size())) { want.push_back (evoCenter); for (int k : evo[(size_t) evoCenter].kids) want.push_back (k); }
        for (int n : evoPath()) want.push_back (n);
        for (int n : want)
            if (! evo[(size_t) n].waveReady && ! evo[(size_t) n].isAudio()) { renderWave (evo[(size_t) n].g, evo[(size_t) n].wave); evo[(size_t) n].waveReady = true; ++evoVer; return true; }
            else if (! evo[(size_t) n].waveReady && evo[(size_t) n].isAudio())
            {
                auto& nd = evo[(size_t) n];
                for (int b = 0; b < 64 && ! nd.audio->peaks.empty(); ++b) nd.wave[(size_t) b] = nd.audio->peaks[(size_t) (b * (int) nd.audio->peaks.size() / 64)];
                nd.waveReady = true; ++evoVer; return true;
            }
    }
    for (int s = 0; s < 2; ++s)   // the parents' own waveforms (shown in PARENT A / B)
    {
        const auto& pg = parents[(size_t) s];
        const auto sig = pg.valid() ? (juce::int64) pg.name.hashCode64() ^ (juce::int64) (pg.v.size() * 7919) ^ (juce::int64) (pg.v[0] * 1.0e6f) : 0;
        if (sig != parentWaveSig[(size_t) s])
        {
            parentWaveSig[(size_t) s] = sig;
            if (pg.valid()) renderWave (pg, parentWave[(size_t) s]); else parentWave[(size_t) s].fill (0.0f);
            ++labVer; return true;
        }
    }
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
        if (g.preset >= 0 && g.gen == 0) g.name = currentFactoryName (g.name);   // v0.34: renamed factory sounds
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
    for (int d = 0; d < kk::numDrumSlots; ++d) state.setProperty ("drum" + juce::String (d), drums[(size_t) d].filePath(), nullptr);
    state.setProperty ("lastFolder", lastFolder, nullptr);
    state.setProperty ("lastSoundKit", lastSoundKit, nullptr);
    state.setProperty ("lastKit", lastKit, nullptr);
    for (int k = 0; k < kk::PairLab::maxParents; ++k) state.setProperty ("pair" + juce::String (k), pairFiles[(size_t) k], nullptr);
    for (int d = 0; d < kk::numDrumSlots; ++d)   // the edited patterns: "bars|beat:vel:semi:len;..."
    {
        juce::String t (patBars[(size_t) d]); t << "|";
        for (auto& h : pattern (d)) t << juce::String (h.beat, 4) << ":" << juce::String (h.vel, 3) << ":" << h.semi << ":" << juce::String (h.len, 4) << ";";
        state.setProperty ("pattern" + juce::String (d), t, nullptr);
    }
    state.setProperty ("rack", rack.toString(), nullptr);
    if (auto c = chop.current(); c != nullptr && chopFile.isNotEmpty())   // SAMPLER: the file and your cuts
    {
        state.setProperty ("chopFile", chopFile, nullptr);
        juce::StringArray mk; for (int m : c->marks) mk.add (juce::String (m));
        state.setProperty ("chopMarks", mk.joinIntoString (","), nullptr);
    }
    for (int k = 0; k < 2; ++k) state.setProperty ("labWav" + juce::String (k), labWav[(size_t) k] ? labWavFile[(size_t) k] : juce::String(), nullptr);
    {   // v0.36 EVOLVE: the tree of bank sounds (a WAV seed comes back from its file)
        juce::ValueTree et ("EVOLVE");
        et.setProperty ("center", evoCenter, nullptr); et.setProperty ("wild", evoWild, nullptr); et.setProperty ("file", evoSeedFile, nullptr);
        et.setProperty ("idea", evoIdea, nullptr); et.setProperty ("taste", evoTasteAmt, nullptr);
        if (evoSeedFile.isEmpty())
            for (size_t i = 0; i < evo.size() && i < 400; ++i)
            {
                const auto& n = evo[i];
                juce::ValueTree t ("N");
                t.setProperty ("p", n.parent, nullptr); t.setProperty ("gen", n.gen, nullptr); t.setProperty ("name", n.name, nullptr);
                t.setProperty ("cat", n.g.cat, nullptr); t.setProperty ("v", floatsToString (n.g.v), nullptr); t.setProperty ("loop", loopToString (n.g.loop), nullptr);
                { juce::StringArray fx; for (auto o : n.fx.on) fx.add (o ? "1" : "0"); for (auto x : n.fx.v) fx.add (juce::String (x, 4)); t.setProperty ("fx", fx.joinIntoString (","), nullptr); }
                et.appendChild (t, nullptr);
            }
        state.appendChild (et, nullptr);
    }
    {   // v0.37: SAMPLE EDIT, STEP FX, MONO pads and the sample on the keys (kept as a file, so FL's notes play it after a reload)
        juce::StringArray se; for (auto& v : sampleEdit) se.add (juce::String (v.load(), 4));
        state.setProperty ("sampleEdit", se.joinIntoString (","), nullptr);
        state.setProperty ("steps", stepToString(), nullptr);
        state.setProperty ("choke", chopChoke.load(), nullptr);
        if (auto smp = pairPlayer.sound())
        {
            if (smp.get() != activeSampleSaved)
            {
                auto f = sessionDir().getChildFile (juce::File::createLegalFileName (smp->name).substring (0, 60) + " "
                                                    + juce::String::toHexString ((juce::int64) smp->audio.getNumSamples() * 31 + (juce::int64) (smp->audio.getRMSLevel (0, 0, smp->audio.getNumSamples()) * 1.0e6f)) + ".wav");
                if (f.existsAsFile() || writeWavFile (smp->audio, sr > 0 ? sr : 44100.0, f)) { activeSampleFile = f.getFullPathName(); activeSampleSaved = smp.get(); }
            }
            state.setProperty ("activeSample", activeSampleFile, nullptr);
        }
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
            if (currentPreset >= 0) presetName = currentFactoryName (presetName);   // v0.34: old genre names -> new names (same sound)
            userFile = juce::File (vt.getProperty ("userFile", "").toString());
            setFxOrder (orderFromString (vt.getProperty ("fxOrder", "").toString()));
            eco = (bool) vt.getProperty ("eco", false);
            macroLabels = juce::StringArray::fromTokens (vt.getProperty ("macroNames", "").toString(), "|", "");
            macroLabels.removeEmptyStrings();
            loadLab (vt);
            {   // v0.37
                const auto se = juce::StringArray::fromTokens (vt.getProperty ("sampleEdit", "").toString(), ",", "");
                resetSampleEdit();
                if (se.size() == numSampleEdit) for (int i = 0; i < numSampleEdit; ++i) sampleEdit[(size_t) i] = se[i].getFloatValue();
                if (vt.hasProperty ("steps")) stepFromString (vt.getProperty ("steps").toString());
                chopChoke = (bool) vt.getProperty ("choke", true);
            }
            {   // v0.36 EVOLVE
                evo.clear(); evoCenter = -1; evoSeedFile.clear();
                const auto et = vt.getChildWithName ("EVOLVE");
                if (et.isValid())
                {
                    evoWild = (float) et.getProperty ("wild", 0.35f);
                    evoIdea = (bool) et.getProperty ("idea", true); evoTasteAmt = (float) et.getProperty ("taste", 0.5f);
                    if (const juce::File ef (et.getProperty ("file", "").toString()); ef.existsAsFile()) { evoSeedFromFile (ef); }
                    else
                    {
                        for (int i = 0; i < et.getNumChildren(); ++i)
                        {
                            const auto t = et.getChild (i);
                            EvoNode n; n.parent = t.getProperty ("p", -1); n.gen = t.getProperty ("gen", 0); n.name = t.getProperty ("name").toString();
                            n.g.name = n.name; n.g.cat = t.getProperty ("cat", -1); n.g.v = stringToFloats (t.getProperty ("v").toString());
                            n.g.loop = loopFromString (t.getProperty ("loop").toString());
                            {
                                const auto fx = juce::StringArray::fromTokens (t.getProperty ("fx").toString(), ",", "");
                                if (fx.size() == kk::numRackSlots + kk::numRackValues)
                                {
                                    for (int k = 0; k < kk::numRackSlots; ++k) n.fx.on[(size_t) k] = fx[k] == "1";
                                    for (int k = 0; k < kk::numRackValues; ++k) n.fx.v[(size_t) k] = fx[kk::numRackSlots + k].getFloatValue();
                                }
                            }
                            if (n.g.v.size() != params.size()) { evo.clear(); break; }
                            evo.push_back (n);
                        }
                        for (int i = 0; i < (int) evo.size(); ++i) if (const int pp = evo[(size_t) i].parent; juce::isPositiveAndBelow (pp, (int) evo.size())) evo[(size_t) pp].kids.push_back (i);
                        evoCenter = juce::jlimit (-1, (int) evo.size() - 1, (int) et.getProperty ("center", -1));
                    }
                    vt.removeChild (et, nullptr);
                }
                ++evoVer;
            }
            for (int k = 0; k < 2; ++k)   // v0.35: your own sounds in PARENT A / B
            {
                genomeParentSet (k);
                if (const juce::File lf (vt.getProperty ("labWav" + juce::String (k), "").toString()); lf.existsAsFile()) labDropFile (k, lf);
            }
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
            rack.reset(); rack.fromString (vt.getProperty ("rack", "").toString());
            if (const juce::File af (vt.getProperty ("activeSample", "").toString()); af.existsAsFile())   // v0.37: the sample on the keys
                if (auto smp = kk::PairLab::fromFile (af, sr > 0 ? sr : 44100.0)) { pairPlayer.setSound (smp); activeSampleFile = af.getFullPathName(); activeSampleSaved = smp.get(); }
            previewNote = -1;   // restoring a project never plays a note
            if (const juce::File cf (vt.getProperty ("chopFile", "").toString()); cf.existsAsFile() && chopLoadFile (cf))
            {
                std::vector<int> mk;
                for (auto& t : juce::StringArray::fromTokens (vt.getProperty ("chopMarks", "").toString(), ",", "")) mk.push_back (t.getIntValue());
                if (mk.size() > 1) chop.setMarks (mk);
            }
            for (int d = 0; d < kk::numDrumSlots; ++d)
                if (vt.hasProperty ("pattern" + juce::String (d)))
                {
                    const auto t = vt.getProperty ("pattern" + juce::String (d)).toString();
                    std::vector<kk::RollHit> pat;
                    for (auto& e : juce::StringArray::fromTokens (t.fromFirstOccurrenceOf ("|", false, false), ";", ""))
                    {
                        auto f = juce::StringArray::fromTokens (e, ":", "");
                        if (f.size() == 4) pat.push_back ({ f[0].getDoubleValue(), juce::jlimit (0.05f, 1.0f, f[1].getFloatValue()), juce::jlimit (-24, 24, f[2].getIntValue()), juce::jmax (0.01, f[3].getDoubleValue()) });
                    }
                    setPattern (d, std::move (pat), juce::jlimit (1, 8, t.upToFirstOccurrenceOf ("|", false, false).getIntValue()));
                }
            lastFolder = vt.getProperty ("lastFolder", kk::Library::defaultFolder()).toString();
            lastSoundKit = vt.getProperty ("lastSoundKit", kk::SoundKits::defaultKit()).toString();
            lastKit = vt.getProperty ("lastKit", kk::Kits::defaultKit()).toString();
            for (int k = 0; k < kk::PairLab::maxParents; ++k)
            {
                const juce::File pf (vt.getProperty ("pair" + juce::String (k), "").toString());
                if (pf.existsAsFile() && pf.getFullPathName() != pairFiles[(size_t) k]) loadPairParent (k, pf);
            }
            for (int d = 0; d < kk::numDrumSlots; ++d)
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
    setPlayMode (playKeys);
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

bool KeysKillaProcessor::exportPack (const juce::File& zipFile, const juce::String& packName)
{
    // v0.34: your presets as a .kkpack sound pack (pack.json + the sounds)
    juce::ZipFile::Builder b;
    auto* info = new juce::DynamicObject();
    info->setProperty ("format", "KEYS KILLA pack"); info->setProperty ("version", 1);
    info->setProperty ("name", packName); info->setProperty ("author", "");
    info->setProperty ("info", "Sounds made with EVOLVE");
    const auto json = juce::JSON::toString (juce::var (info), false);
    auto tmp = juce::File::createTempFile ("json");
    tmp.replaceWithText (json);
    b.addFile (tmp, 9, "pack.json");
    for (auto& f : userPresets()) b.addFile (f, 9, "sounds/" + f.getFileName());
    zipFile.deleteFile();
    bool ok = false;
    {
        juce::FileOutputStream os (zipFile);
        ok = os.openedOk() && b.writeToStream (os, nullptr);
    }
    tmp.deleteFile();
    return ok;
}

juce::File KeysKillaProcessor::packsDir()
{
    auto d = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("Packs");
    d.createDirectory();
    return d;
}

juce::String KeysKillaProcessor::installPack (const juce::File& src, juce::String* error)
{
    auto fail = [error] (const juce::String& e) { if (error) *error = e; return juce::String(); };
    if (! src.existsAsFile()) return fail ("file not found");
    juce::ZipFile zip (src);
    if (zip.getNumEntries() == 0) return fail ("this is not a sound pack");
    juce::String name = src.getFileNameWithoutExtension(), author, info;
    for (int i = 0; i < zip.getNumEntries(); ++i)
        if (auto* e = zip.getEntry (i); e && juce::File::createFileWithoutCheckingPath (e->filename).getFileName().equalsIgnoreCase ("pack.json"))
        {
            std::unique_ptr<juce::InputStream> in (zip.createStreamForEntry (i));
            const auto v = in ? juce::JSON::parse (in->readEntireStreamAsString()) : juce::var();
            if (v["name"].toString().trim().isNotEmpty()) name = v["name"].toString().trim();
        }
    name = juce::File::createLegalFileName (name).trim();
    if (name.isEmpty() || name.equalsIgnoreCase ("FACTORY")) name = "PACK " + juce::String (juce::Time::currentTimeMillis() % 100000);
    auto dir = packsDir().getChildFile (name);
    dir.deleteRecursively(); dir.createDirectory();
    int sounds = 0;
    for (int i = 0; i < zip.getNumEntries(); ++i)
    {
        auto* e = zip.getEntry (i);
        if (e == nullptr || e->filename.endsWithChar ('/') || e->filename.contains ("..")) continue;
        const auto fn = e->filename.replaceCharacter ('\\', '/');
        const auto leaf = juce::File::createLegalFileName (fn.fromLastOccurrenceOf ("/", false, false));
        const auto ext = leaf.fromLastOccurrenceOf (".", true, false).toLowerCase();
        juce::File out;
        if (ext == ".kkpreset") { out = dir.getChildFile ("sounds").getChildFile (leaf); ++sounds; }
        else if (ext == ".wav" || ext == ".aif" || ext == ".aiff" || ext == ".flac") out = dir.getChildFile ("samples").getChildFile (leaf);
        else if (leaf.equalsIgnoreCase ("pack.json") || leaf.startsWithIgnoreCase ("cover.") || leaf.equalsIgnoreCase ("info.txt")) out = dir.getChildFile (leaf);
        else continue;
        out.getParentDirectory().createDirectory();
        std::unique_ptr<juce::InputStream> in (zip.createStreamForEntry (i));
        if (in == nullptr) continue;
        juce::FileOutputStream os (out);
        if (os.openedOk()) { os.setPosition (0); os.truncate(); os.writeFromInputStream (*in, -1); }
    }
    if (sounds == 0) { dir.deleteRecursively(); return fail ("the pack has no sounds"); }
    rescanPacks();
    return name;
}

bool KeysKillaProcessor::removePack (const juce::String& name)
{
    auto dir = packsDir().getChildFile (juce::File::createLegalFileName (name));
    if (name.isEmpty() || ! dir.isDirectory() || ! dir.deleteRecursively()) return false;
    rescanPacks();
    return true;
}

void KeysKillaProcessor::rescanPacks()
{
    packList.clear(); packSoundList.clear();
    auto dirs = packsDir().findChildFiles (juce::File::findDirectories, false);
    std::sort (dirs.begin(), dirs.end(), [] (const juce::File& a, const juce::File& b) { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
    for (auto& d : dirs)
    {
        PackInfo pi; pi.dir = d; pi.name = d.getFileName();
        const auto v = juce::JSON::parse (d.getChildFile ("pack.json"));
        if (v.isObject())
        {
            if (v["name"].toString().isNotEmpty()) pi.name = v["name"].toString();
            pi.author = v["author"].toString(); pi.info = v["info"].toString();
        }
        for (auto* c : { "cover.png", "cover.jpg" }) if (d.getChildFile (c).existsAsFile()) { pi.cover = d.getChildFile (c); break; }
        auto files = d.findChildFiles (juce::File::findFiles, true, "*.kkpreset");
        std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b) { return a.getFileName().compareIgnoreCase (b.getFileName()) < 0; });
        for (auto& f : files)
        {
            PackSound ps; ps.file = f; ps.name = f.getFileNameWithoutExtension(); ps.pack = pi.name;
            const auto pv = juce::JSON::parse (f);
            ps.cat = categoryNames().indexOf (pv["category"].toString(), true);
            packSoundList.push_back (ps); ++pi.sounds;
        }
        if (pi.sounds > 0) packList.push_back (pi);
    }
}

juce::AudioProcessorEditor* KeysKillaProcessor::createEditor() { return new KeysKillaEditor (*this); }

#if ! KK_TEST_BUILD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KeysKillaProcessor(); }
#endif

//==============================================================================
// v0.37 THE SOUND ON THE KEYS: what you picked last plays - pages never take it away
void KeysKillaProcessor::setPlayMode (int mode)
{
    if (auto* q = apvts.getParameter (ID::playMode))
    {
        const float v = q->convertTo0to1 ((float) mode);
        if (std::abs (q->getValue() - v) > 1.0e-6f) { q->beginChangeGesture(); q->setValueNotifyingHost (v); q->endChangeGesture(); }
    }
}

void KeysKillaProcessor::useSample (kk::PairPtr s, bool audition)
{
    if (s == nullptr) return;
    const bool wasSample = (int) raw[(size_t) ix->playMode]->load() == playPair;
    pairPlayer.setSound (s);
    if (! wasSample) sampleMacrosNeutral();   // the big knobs now colour this sound - start them neutral
    setPlayMode (playPair);
    if (audition) previewNote = s->rootNote;
    ++pairVer;
}

int KeysKillaProcessor::geneOfSelected (int gene) const
{
    gene = juce::jlimit (0, (int) numGenes - 1, gene);
    if (labAudioMode()) return juce::isPositiveAndBelow (pairSel, (int) pairKidGenes.size()) && pairSel < (int) pairKids.size() ? pairKidGenes[(size_t) pairSel][(size_t) gene] : -1;
    return juce::isPositiveAndBelow (selChild, (int) children.size()) ? children[(size_t) selChild].genes[(size_t) gene] : -1;
}

// ---------------- SAMPLE EDIT ----------------
const char* KeysKillaProcessor::sampleEditName (int i)
{
    static const char* n[] { "TUNE", "FINE", "START", "ATTACK", "RELEASE", "REVERSE", "TONE", "LOW CUT", "DRIVE", "CRUSH", "CHORUS", "SPACE", "ECHO", "WIDTH", "GAIN" };
    return n[juce::jlimit (0, (int) numSampleEdit - 1, i)];
}
float KeysKillaProcessor::sampleEditDefault (int i)
{
    static const float d[] { 0, 0, 0, 0, 0.12f, 0, 0, 0, 0, 0, 0, 0, 0, 0.5f, 0 };
    return d[juce::jlimit (0, (int) numSampleEdit - 1, i)];
}
void KeysKillaProcessor::resetSampleEdit() { for (int i = 0; i < numSampleEdit; ++i) sampleEdit[(size_t) i] = sampleEditDefault (i); }
void KeysKillaProcessor::sampleMacrosNeutral()
{
    const char* ids[] { ID::m1, ID::m2, ID::m3, ID::m4, ID::m5, ID::m6, ID::m7, ID::m8 };
    for (auto* id : ids)
        if (auto* q = apvts.getParameter (id))
            if (std::abs (q->getValue() - q->getDefaultValue()) > 1.0e-6f) { q->beginChangeGesture(); q->setValueNotifyingHost (q->getDefaultValue()); q->endChangeGesture(); }
}

// the knobs + SAMPLE EDIT as effect settings for samples: neutral = nothing happens (the sample already has its sound)
kk::FxParams KeysKillaProcessor::sampleFxParams (bool& any) const
{
    const auto& I = *ix;
    auto se = [this] (int i) { return sampleEdit[(size_t) i].load(); };
    const float m1 = raw[(size_t) I.m1]->load(), m2 = raw[(size_t) I.m2]->load(), m3 = raw[(size_t) I.m3]->load(), m4 = raw[(size_t) I.m4]->load();
    const float m5 = raw[(size_t) I.m5]->load(), m6 = raw[(size_t) I.m6]->load(), m7 = raw[(size_t) I.m7]->load(), m8 = raw[(size_t) I.m8]->load();
    const float wet = 0.2f + 1.6f * m8;
    kk::FxParams p;
    p.revMix = juce::jlimit (0.0f, 1.0f, (se (seSpace) * 0.55f + std::max (0.0f, m2 - 0.25f) * 0.7f) * wet);
    p.revSize = 0.55f + 0.35f * se (seSpace); p.revType = 0;
    p.delayMix = juce::jlimit (0.0f, 1.0f, se (seEcho) * 0.45f * wet); p.delayFb = 0.38f; p.delayBeats = 0.75; p.delayMode = 1;
    p.drive = juce::jlimit (0.0f, 1.0f, se (seDrive) + m3 * 0.8f); p.driveType = 0;
    p.crush = juce::jlimit (0.0f, 1.0f, se (seCrush) * 0.6f + m4 * 0.45f); p.wow = m4 * 0.5f;
    p.chorus = juce::jlimit (0.0f, 1.0f, (se (seChorus) + std::max (0.0f, m5 - 0.2f) * 0.7f) * wet);
    p.width = juce::jlimit (0.0f, 1.0f, se (seWidth) + (m6 - 0.5f));
    p.punch = m7 * 0.9f;
    const float tilt = se (seTone) + (0.5f - m1) * 2.0f;
    p.eqHigh = tilt > 0 ? tilt * 6.0f : 0.0f;
    p.outGain = juce::Decibels::decibelsToGain (se (seGain));
    p.revMix = p.revMix < 0.004f ? 0.0f : p.revMix;
    p.master = 0.0f; p.monoLows = false; p.cleanLow = false;
    any = p.revMix > 0 || p.delayMix > 0.003f || p.drive > 0.003f || p.crush > 0.003f || p.wow > 0.003f || p.chorus > 0.003f
       || std::abs (p.width - 0.5f) > 0.01f || p.punch > 0.01f || std::abs (tilt) > 0.02f || se (seLowCut) > 0.01f || std::abs (se (seGain)) > 0.05f;
    return p;
}

void KeysKillaProcessor::processSampleFx (float* L, float* R, int n, double beatPos, double bps)
{
    bool any = false;
    auto p = sampleFxParams (any);
    if (any) extTail = (int) (sr * 6.0);
    else if (extTail <= 0) return;
    else extTail -= n;
    p.bpm = bps * 60.0 * sr;
    const float tilt = sampleEdit[seTone].load() + (0.5f - raw[(size_t) ix->m1]->load()) * 2.0f;
    const float lpHz = tilt < 0 ? 20000.0f * std::exp2 (tilt * 3.2f) : 20000.0f;
    const float lc = sampleEdit[seLowCut].load(), hpHz = lc > 0.01f ? 20.0f * std::pow (50.0f, lc) : 0.0f;
    if (std::abs (lpHz - extLpHz) > 1.0f) { extLpHz = lpHz; extLpC.set (std::min (lpHz, (float) sr * 0.45f), 1.2f, (float) sr); }
    if (std::abs (hpHz - extHpHz) > 0.5f) { extHpHz = hpHz; extHpC.set (std::max (10.0f, hpHz), 1.3f, (float) sr); }
    for (int c0 = 0; c0 < n; c0 += kChunk)
    {
        const int len = std::min (kChunk, n - c0);
        std::copy (L + c0, L + c0 + len, extL.begin()); std::copy (R + c0, R + c0 + len, extR.begin());
        for (int i = 0; i < len; ++i)
        {
            float* ch[2] { &extL[(size_t) i], &extR[(size_t) i] };
            for (int c = 0; c < 2; ++c)
            {
                if (lpHz < 19000.0f) { extLp[c].tick (extLpC, *ch[c]); *ch[c] = extLp[c].lp; }
                if (hpHz > 0.0f) { extHp[c].tick (extHpC, *ch[c]); *ch[c] = extHp[c].hp; }
            }
        }
        std::fill_n (extG.begin(), len, 0.0f);
        p.beatPos = beatPos + bps * c0;
        extFx->process (extL.data(), extR.data(), extG.data(), len, p);
        for (int i = 0; i < len; ++i)
        {
            const float l = extL[(size_t) i], r = extR[(size_t) i];
            if (std::isfinite (l) && std::isfinite (r)) { L[c0 + i] = l; R[c0 + i] = r; }
        }
    }
}

juce::AudioBuffer<float> KeysKillaProcessor::renderEditedSample (kk::PairPtr s)
{
    juce::AudioBuffer<float> out;
    if (s == nullptr || s->audio.getNumSamples() < 4) return out;
    const double rate = sr > 0 ? sr : 44100.0;
    auto se = [this] (int i) { return sampleEdit[(size_t) i].load(); };
    // shape: start, reverse, tune, attack
    juce::AudioBuffer<float> a (2, s->audio.getNumSamples());
    for (int c = 0; c < 2; ++c) a.copyFrom (c, 0, s->audio, std::min (c, s->audio.getNumChannels() - 1), 0, a.getNumSamples());
    if (se (seReverse) > 0.5f) a.reverse (0, a.getNumSamples());
    const int st = (int) (juce::jlimit (0.0f, 0.95f, se (seStart)) * (float) a.getNumSamples());
    const double ratio = std::exp2 ((se (seTune) + se (seFine) / 100.0f) / 12.0);
    const int srcLen = a.getNumSamples() - st, len = std::max (8, (int) std::floor ((srcLen - 4) / ratio));
    bool any = false;
    auto p = sampleFxParams (any);
    const int tail = any && (p.revMix > 0 || p.delayMix > 0) ? (int) (rate * 2.5) : 0;
    out.setSize (2, len + tail); out.clear();
    for (int c = 0; c < 2; ++c)
    {
        juce::LagrangeInterpolator li;
        li.process (ratio, a.getReadPointer (c, st), out.getWritePointer (c), len, srcLen, 0);
    }
    const int atk = std::max (16, (int) (rate * std::max (0.0008f, se (seAttack))));
    for (int c = 0; c < 2; ++c) out.applyGainRamp (c, 0, std::min (len, atk), 0.0f, 1.0f);
    if (any)
    {
        kk::FxRack fx; fx.prepare (rate, kChunk);
        p.bpm = lastBpm.load();
        const float tilt = se (seTone) + (0.5f - raw[(size_t) ix->m1]->load()) * 2.0f;
        const float lpHz = tilt < 0 ? 20000.0f * std::exp2 (tilt * 3.2f) : 20000.0f;
        const float lc = se (seLowCut), hpHz = lc > 0.01f ? 20.0f * std::pow (50.0f, lc) : 0.0f;
        kk::SvfCoef lpC, hpC; lpC.set (std::min (lpHz, (float) rate * 0.45f), 1.2f, (float) rate); hpC.set (std::max (10.0f, hpHz), 1.3f, (float) rate);
        kk::SvfState lpS[2], hpS[2];
        std::vector<float> g ((size_t) kChunk, 0.0f);
        const double bps = p.bpm / 60.0 / rate;
        for (int c0 = 0; c0 < out.getNumSamples(); c0 += kChunk)
        {
            const int n = std::min (kChunk, out.getNumSamples() - c0);
            float* L = out.getWritePointer (0, c0); float* R = out.getWritePointer (1, c0);
            for (int i = 0; i < n; ++i)
            {
                float* ch[2] { L + i, R + i };
                for (int c = 0; c < 2; ++c)
                {
                    if (lpHz < 19000.0f) { lpS[c].tick (lpC, *ch[c]); *ch[c] = lpS[c].lp; }
                    if (hpHz > 0.0f) { hpS[c].tick (hpC, *ch[c]); *ch[c] = hpS[c].hp; }
                }
            }
            p.beatPos = bps * c0;
            fx.process (L, R, g.data(), n, p);
        }
    }
    // end: trim the silent tail, short fade
    int last = out.getNumSamples() - 1;
    while (last > (int) (rate * 0.05) && out.getMagnitude (last - 63 > 0 ? last - 63 : 0, 64) < 1.0e-4f) last -= 64;
    out.setSize (2, std::max (8, last + 1), true);
    const float pk = out.getMagnitude (0, out.getNumSamples());
    if (pk > 0.995f) out.applyGain (0.89f / pk);
    const int fade = std::min (out.getNumSamples() / 4, (int) (rate * 0.01));
    for (int c = 0; c < 2; ++c) out.applyGainRamp (c, out.getNumSamples() - fade, fade, 1.0f, 0.0f);
    return out;
}

kk::PairPtr KeysKillaProcessor::editedSample()
{
    auto s = pairPlayer.sound();
    if (s == nullptr) return nullptr;
    const double rate = sr > 0 ? sr : 44100.0;
    auto b = renderEditedSample (s);
    if (b.getNumSamples() < 8) return s;
    auto out = kk::PairLab::fromBuffer (b, rate, rate, s->name + " edit");
    return out;
}

juce::File KeysKillaProcessor::exportEditedSample()
{
    auto e = editedSample();
    return e != nullptr ? kk::PairLab::exportWav (*e, sr > 0 ? sr : 44100.0, e->name) : juce::File();
}

// ---------------- FX EVOLVE ----------------
KeysKillaProcessor::FxGenome KeysKillaProcessor::fxCurrent() const
{
    FxGenome g;
    for (int i = 0; i < kk::numRackSlots; ++i) g.on[(size_t) i] = rack.on[(size_t) i].load();
    for (int i = 0; i < kk::numRackValues; ++i) g.v[(size_t) i] = rack.v[(size_t) i].load();
    return g;
}
void KeysKillaProcessor::fxApply (const FxGenome& g)
{
    bool hasValues = false; for (auto x : g.v) hasValues = hasValues || x != 0.0f;
    if (hasValues) for (int i = 0; i < kk::numRackValues; ++i) rack.v[(size_t) i] = g.v[(size_t) i];   // a "dry" chain only switches off
    for (int i = 0; i < kk::numRackSlots; ++i) rack.on[(size_t) i] = g.on[(size_t) i];
}
namespace
{
// tasteful ranges per rack value (lo, hi): never a broken sound
const float fxRange[kk::numRackValues][2] { { 0.1f, 0.6f }, { 0, 3 }, { 0.15f, 0.55f }, { 0.2f, 0.7f }, { 0.2f, 0.6f }, { 0.25f, 0.7f }, { 0.3f, 0.7f },
                                             { 0, 2 }, { 0.4f, 0.95f }, { 0.12f, 0.42f }, { 0.2f, 0.55f }, { 0, 4 }, { 0.15f, 0.5f }, { 0.35f, 0.9f },
                                             { 0, 3 }, { -6, 6 }, { -8, 6 }, { 0.55f, 1.0f } };
const bool fxDiscrete[kk::numRackValues] { false, true, false, false, false, false, false, true, false, false, false, true, false, false, true, false, false, false };
juce::String fxNameOf (uint32_t seed)
{
    static const char* a[] { "Tape", "Glass", "Dust", "Neon", "Velvet", "Ghost", "Smoke", "Chrome", "Lunar", "Broken", "Liquid", "Midnight", "Haze", "Ice", "Rust", "Silk" };
    static const char* b[] { "Room", "Echo", "Wash", "Drive", "Bloom", "Drift", "Space", "Grit", "Halo", "Pulse", "Mirror", "Wave", "Fog", "Glow", "Trail", "Stutter" };
    return juce::String (a[seed % 16]) + " " + b[(seed >> 5) % 16];
}
}
KeysKillaProcessor::FxGenome KeysKillaProcessor::fxSurprise (uint32_t seed) const
{
    FxGenome g;
    juce::Random r ((juce::int64) seed);
    for (int i = 0; i < kk::numRackValues; ++i)
    {
        const float lo = fxRange[i][0], hi = fxRange[i][1];
        g.v[(size_t) i] = fxDiscrete[i] ? (float) (int) (lo + r.nextFloat() * (hi - lo + 0.999f)) : lo + r.nextFloat() * (hi - lo);
    }
    // 2-4 effects, a space effect most of the time (it sounds finished)
    const int count = 2 + r.nextInt (3);
    if (r.nextFloat() < 0.75f) g.on[(size_t) (r.nextBool() ? kk::rkReverb : kk::rkDelay)] = true;
    for (int k = 0, guard = 0; k < count && guard < 40; ++guard)
    {
        const int s = r.nextInt (kk::numRackSlots);
        if (g.on[(size_t) s]) continue;
        if (s == kk::rkHalf && r.nextFloat() < 0.7f) continue;   // half-time only sometimes
        g.on[(size_t) s] = true; ++k;
    }
    g.name = fxNameOf (seed);
    return g;
}
KeysKillaProcessor::FxGenome KeysKillaProcessor::fxMutate (const FxGenome& src, float wild, uint32_t seed) const
{
    FxGenome g = src;
    juce::Random r ((juce::int64) seed);
    wild = juce::jlimit (0.0f, 1.0f, wild);
    for (int i = 0; i < kk::numRackValues; ++i)
    {
        const float lo = fxRange[i][0], hi = fxRange[i][1];
        if (fxDiscrete[i]) { if (r.nextFloat() < 0.15f + 0.4f * wild) g.v[(size_t) i] = (float) (int) (lo + r.nextFloat() * (hi - lo + 0.999f)); continue; }
        const float j = (r.nextFloat() * 2.0f - 1.0f) * (hi - lo) * (0.12f + 0.45f * wild);
        g.v[(size_t) i] = juce::jlimit (std::min (lo, g.v[(size_t) i]), std::max (hi, g.v[(size_t) i]), g.v[(size_t) i] + j);
    }
    const int flips = 1 + (int) (wild * 3.0f + r.nextFloat());
    for (int k = 0; k < flips; ++k)
    {
        const int s = r.nextInt (kk::numRackSlots);
        if (s == kk::rkHalf && ! g.on[(size_t) s] && r.nextFloat() < 0.7f) continue;
        g.on[(size_t) s] = ! g.on[(size_t) s];
    }
    int count = 0; for (auto o : g.on) count += o ? 1 : 0;
    if (count == 0) g.on[(size_t) kk::rkReverb] = true;
    g.name = fxNameOf (seed);
    return g;
}

// ---------------- STEP FX ----------------
const char* KeysKillaProcessor::stepFxName (int i)
{
    static const char* n[] { "STUTTER", "REVERSE", "TAPE STOP", "FILTER", "GATE", "ECHO", "OCTAVE UP", "CRUSH" };
    return n[juce::jlimit (0, (int) numStepFx - 1, i)];
}
void KeysKillaProcessor::stepClear() { for (auto& row : stepGrid) for (auto& c : row) c = false; }
void KeysKillaProcessor::stepRandom (uint32_t seed, float density)
{
    juce::Random r ((juce::int64) seed);
    stepClear();
    // musical: effects land on the off-beats and the ends of the bar more than on the 1
    for (int st = 0; st < numSteps; ++st)
    {
        const float weight = st == 0 ? 0.15f : (st % 4 == 0 ? 0.6f : 1.0f) * (st >= 12 ? 1.5f : 1.0f);
        if (r.nextFloat() > density * 0.55f * weight) continue;
        static const int pool[] { sfStutter, sfStutter, sfReverse, sfTape, sfFilter, sfGate, sfGate, sfEcho, sfPitchUp, sfCrush };
        int fx = pool[r.nextInt (10)];
        if (fx == sfTape && st % 4 != 3) fx = sfGate;   // a tape stop only before a beat
        stepGrid[(size_t) fx][(size_t) st] = true;
        if (fx == sfStutter && st + 1 < numSteps && r.nextFloat() < 0.4f) stepGrid[(size_t) fx][(size_t) st + 1] = true;
    }
}
void KeysKillaProcessor::stepPreset (int which)
{
    stepClear();
    auto set = [this] (int fx, std::initializer_list<int> steps) { for (int s : steps) stepGrid[(size_t) fx][(size_t) s] = true; };
    switch (which)
    {
        case 0: set (sfStutter, { 6, 7, 14, 15 }); set (sfReverse, { 11 }); break;                    // ROLL
        case 1: set (sfGate, { 1, 3, 5, 7, 9, 11, 13, 15 }); break;                                    // CHOP
        case 2: set (sfTape, { 7, 15 }); set (sfFilter, { 12, 13, 14 }); break;                        // BRAKE
        case 3: set (sfReverse, { 3, 7, 11, 15 }); set (sfEcho, { 4, 12 }); break;                      // MIRROR
        case 4: set (sfPitchUp, { 10, 11 }); set (sfStutter, { 14, 15 }); set (sfCrush, { 2, 6 }); break; // GLITCH
        default: set (sfFilter, { 0, 1, 2, 3, 8, 9, 10, 11 }); set (sfEcho, { 7, 15 }); break;          // SWEEP
    }
}
juce::String KeysKillaProcessor::stepToString() const
{
    juce::String t;
    for (auto& row : stepGrid) { for (auto& c : row) t << (c.load() ? "1" : "0"); t << "."; }
    return t + juce::String (stepRate.load()) + "." + juce::String (stepOn.load() ? 1 : 0) + "." + juce::String (stepMix.load(), 3);
}
void KeysKillaProcessor::stepFromString (const juce::String& s)
{
    const auto a = juce::StringArray::fromTokens (s, ".", "");
    if (a.size() < numStepFx + 2) return;
    for (int f = 0; f < numStepFx; ++f)
        for (int st = 0; st < numSteps; ++st) stepGrid[(size_t) f][(size_t) st] = a[f].length() > st && a[f][st] == '1';
    stepRate = juce::jlimit (0, 2, a[numStepFx].getIntValue());
    stepOn = a[numStepFx + 1].getIntValue() != 0;
    if (a.size() > numStepFx + 2) stepMix = juce::jlimit (0.0f, 1.0f, a[numStepFx + 2].getFloatValue());
}

void KeysKillaProcessor::processStepFx (juce::AudioBuffer<float>& buffer, int n, double beatPos, double bps)
{
    if (buffer.getNumChannels() < 2 || stepBufL.empty()) return;
    const bool on = stepOn.load();
    float* L = buffer.getWritePointer (0); float* R = buffer.getWritePointer (1);
    const int size = (int) stepBufL.size(), mask = size - 1;
    const double stepBeats = stepRate.load() == 0 ? 0.5 : stepRate.load() == 1 ? 0.25 : 0.125;
    const int stepLen = std::max (64, (int) std::round (stepBeats / std::max (1.0e-9, bps)));
    const float ramp = 1.0f / (0.003f * (float) sr);
    const float mix = stepMix.load();
    if (! on)
    {
        stepNow = -1;
        bool idle = echoTail <= 0;
        for (auto e : stepEnv) idle = idle && e <= 0.0f;
        if (idle) { stepLast = -1; return; }
        echoTail -= n;
    }
    else echoTail = (int) (sr * 5.0);
    kk::SvfCoef fc;
    bool act[numStepFx] {}; int actStep = -99;
    for (int i = 0; i < n; ++i)
    {
        const float dl = L[i], dr = R[i];
        stepBufL[(size_t) stepW] = dl; stepBufR[(size_t) stepW] = dr;
        const double beat = beatPos + bps * i;
        const double sx = beat / stepBeats;
        const int step = ((int) std::floor (sx)) % numSteps;
        const float frac = (float) (sx - std::floor (sx));
        const int pos = (int) (frac * (float) stepLen);
        if (step != stepLast)
        {
            stepLast = step;
            stepRevStart = stepW;
            stepTapePos = 0; stepRevPos = 0;
            stepNow = on ? step : -1;
        }
        if (i == 0 || pos == 0 || step != actStep)
        {
            actStep = step;
            for (int f = 0; f < numStepFx; ++f) act[f] = on && stepGrid[(size_t) f][(size_t) (step < 0 ? 0 : step)].load();
        }
        // one "read" effect at a time (STUTTER > REVERSE > TAPE > OCTAVE UP), 3 ms crossfades
        const int read = act[sfStutter] ? sfStutter : act[sfReverse] ? sfReverse : act[sfTape] ? sfTape : act[sfPitchUp] ? sfPitchUp : -1;
        float wl = 0, wr = 0, dryW = 1.0f;
        const int start = (int) stepRevStart;
        for (int f : { (int) sfStutter, (int) sfReverse, (int) sfTape, (int) sfPitchUp })
        {
            auto& e = stepEnv[f];
            e = f == read ? std::min (1.0f, e + ramp) : std::max (0.0f, e - ramp);
            if (e <= 0.0f) continue;
            int idx = stepW;
            if (f == sfStutter) { const int slice = std::max (32, stepLen / 4); idx = (start + (pos % slice)) & mask; }
            else if (f == sfReverse) idx = (start - 1 - pos + size * 4) & mask;
            else if (f == sfTape) { stepTapePos += std::max (0.0f, 1.0f - frac * 1.05f); idx = (start + (int) stepTapePos) & mask; }
            else idx = (start - stepLen + (2 * pos) % stepLen + size * 4) & mask;   // the step before, twice as fast = an octave up
            // short window at the slice edges: no clicks
            float w = 1.0f;
            if (f == sfStutter) { const int slice = std::max (32, stepLen / 4), ph = pos % slice, ed = std::min (slice / 4, (int) (0.002 * sr)); w = std::min (1.0f, std::min ((float) ph / (float) std::max (1, ed), (float) (slice - ph) / (float) std::max (1, ed))); }
            wl += stepBufL[(size_t) idx] * e * w; wr += stepBufR[(size_t) idx] * e * w;
            dryW -= e;
        }
        float l = dl * std::max (0.0f, dryW) + wl, r = dr * std::max (0.0f, dryW) + wr;
        // FILTER: a low-pass that closes over the step
        {
            auto& e = stepEnv[sfFilter];
            e = act[sfFilter] ? std::min (1.0f, e + ramp) : std::max (0.0f, e - ramp);
            if (e > 0.0f)
            {
                fc.set (7000.0f * std::exp2 (-frac * 4.5f), 2.2f, (float) sr);
                stepLp[0].tick (fc, l); stepLp[1].tick (fc, r);
                l = l + (stepLp[0].lp - l) * e; r = r + (stepLp[1].lp - r) * e;
            }
        }
        // CRUSH: 10 bits, a quarter of the rate
        {
            auto& e = stepEnv[sfCrush];
            e = act[sfCrush] ? std::min (1.0f, e + ramp) : std::max (0.0f, e - ramp);
            if (e > 0.0f)
            {
                if ((stepW & 3) == 0) { stepHold[0] = std::round (l * 512.0f) / 512.0f; stepHold[1] = std::round (r * 512.0f) / 512.0f; }
                l = l + (stepHold[0] - l) * e; r = r + (stepHold[1] - r) * e;
            }
        }
        // GATE: the second half of the step is silent
        {
            auto& e = stepEnv[sfGate];
            const bool mute = act[sfGate] && frac >= 0.5f;
            e = mute ? std::min (1.0f, e + ramp) : std::max (0.0f, e - ramp);
            l *= 1.0f - e; r *= 1.0f - e;
        }
        // ECHO: this step is thrown into a dotted 1/8 echo that keeps ringing
        {
            auto& e = stepEnv[sfEcho];
            e = act[sfEcho] ? std::min (1.0f, e + ramp) : std::max (0.0f, e - ramp);
            const int d = std::min (size - 2, (int) std::round (0.75 / std::max (1.0e-9, bps)));
            const int ri = (echoW - d + size * 4) & mask;
            const float el = echoL[(size_t) ri], er = echoR[(size_t) ri];
            echoL[(size_t) echoW] = l * e + er * 0.45f;   // ping-pong
            echoR[(size_t) echoW] = r * e + el * 0.45f;
            echoW = (echoW + 1) & mask;
            l += el * 0.7f; r += er * 0.7f;
        }
        stepW = (stepW + 1) & mask;
        L[i] = dl + (l - dl) * mix; R[i] = dr + (r - dr) * mix;
    }
}

// ---------------- SAMPLER FLIPS: EVOLVE for beats ----------------
// A flip = 32 steps of 1/16 (2 bars). Each step plays a chop (or rests). The first flip comes from your slices;
// every flip grows 6 children: swapped chops, rolls, reversed hits, octave hits, shifted halves, more / fewer hits.
namespace
{
juce::String flipName (int gen, int k, uint32_t seed)
{
    static const char* w[] { "Bounce", "Skip", "Roll", "Flip", "Swing", "Shuffle", "Stomp", "Glide", "Chop", "Stutter", "Knock", "Drift" };
    return juce::String (w[(seed + (uint32_t) k * 5u) % 12]) + " " + juce::String (gen) + "." + juce::String (k + 1);
}
}
void KeysKillaProcessor::flipSeed()
{
    auto d = chop.current();
    flips.clear(); flipCenter = -1;
    if (d == nullptr || d->src == nullptr || d->numSlices() == 0) { ++flipVer; return; }
    juce::Random r ((juce::int64) juce::Time::getMillisecondCounter());
    Flip f; f.steps.resize (32); f.name = "Seed flip";
    const int ns = std::min (d->numSlices(), 16);
    for (int st = 0; st < 32; st += 2)   // 1/8 grid, the 1 keeps the first chop
    {
        if (st % 8 != 0 && r.nextFloat() < 0.25f) continue;
        auto& s = f.steps[(size_t) st];
        s.slice = st % 16 == 0 ? 0 : r.nextInt (ns);
        s.len = 2; s.vel = st % 8 == 0 ? 1.0f : 0.75f + 0.2f * r.nextFloat();
    }
    flips.push_back (f);
    flipCenter = 0;
    flipGrow (0, false, 0.35f);
    ++flipVer;
}

void KeysKillaProcessor::flipGrow (int node, bool reroll, float wild)
{
    if (! juce::isPositiveAndBelow (node, (int) flips.size())) return;
    if (! flips[(size_t) node].kids.empty() && ! reroll) return;
    flips[(size_t) node].kids.clear();
    auto d = chop.current();
    const int ns = d != nullptr ? std::max (1, std::min (d->numSlices(), 16)) : 1;
    const auto base = flips[(size_t) node];
    const uint32_t seed = kk::hash32 ((uint32_t) juce::Time::getMillisecondCounter() + (uint32_t) node * 131u);
    juce::Random r ((juce::int64) seed);
    wild = juce::jlimit (0.0f, 1.0f, wild);
    for (int k = 0; k < 6; ++k)
    {
        Flip f = base; f.kids.clear(); f.parent = node; f.gen = base.gen + 1;
        auto& st = f.steps;
        const int changes = 2 + (int) (wild * 6.0f) + r.nextInt (2);
        auto hits = [&] { std::vector<int> h; for (int i = 0; i < 32; ++i) if (st[(size_t) i].slice >= 0) h.push_back (i); return h; };
        switch (k)
        {
            case 0:   // SWAP: other chops on some hits
                for (int c = 0; c < changes; ++c) { auto h = hits(); if (h.empty()) break; st[(size_t) h[(size_t) r.nextInt ((int) h.size())]].slice = r.nextInt (ns); }
                break;
            case 1:   // ROLL: 1/16 doubles and a roll into the next bar
                for (int c = 0; c < changes; ++c) { const int i = r.nextInt (32); if (st[(size_t) i].slice < 0 && i > 0 && st[(size_t) i - 1].slice >= 0) { st[(size_t) i] = st[(size_t) i - 1]; st[(size_t) i].len = 1; st[(size_t) i - 1].len = 1; st[(size_t) i].vel *= 0.8f; } }
                for (int i = 12; i < 16; ++i) if (r.nextFloat() < 0.4f + wild * 0.4f) { st[(size_t) i].slice = st[(size_t) 12].slice >= 0 ? st[(size_t) 12].slice : 0; st[(size_t) i].len = 1; st[(size_t) i].vel = 0.55f + 0.1f * (float) (i - 12); }
                break;
            case 2:   // REVERSE some hits
                for (int c = 0; c < std::max (1, changes / 2); ++c) { auto h = hits(); if (h.empty()) break; auto& s = st[(size_t) h[(size_t) r.nextInt ((int) h.size())]]; s.rev = ! s.rev; s.len = std::max (s.len, 2); }
                break;
            case 3:   // OCTAVES: some hits up / down
                for (int c = 0; c < std::max (1, changes / 2); ++c) { auto h = hits(); if (h.empty()) break; auto& s = st[(size_t) h[(size_t) r.nextInt ((int) h.size())]]; static const int sm[] { 12, -12, 7, 5 }; s.semi = sm[r.nextInt (wild > 0.5f ? 4 : 2)]; }
                break;
            case 4:   // SHIFT: the second bar answers the first, shifted by a 1/16 or 1/8
            {
                const int sh = r.nextBool() ? 1 : 2;
                std::vector<FlipStep> b2 (st.begin() + 16, st.end());
                for (int i = 0; i < 16; ++i) st[(size_t) (16 + i)] = b2[(size_t) ((i + 16 - sh) % 16)];
                if (r.nextFloat() < 0.5f) for (int i = 0; i < 16; ++i) if (st[(size_t) i].slice >= 0 && r.nextFloat() < 0.3f) st[(size_t) (16 + i)] = st[(size_t) i];
                break;
            }
            default:  // DENSITY: more hits (or fewer when it is busy)
            {
                const int count = (int) hits().size();
                for (int c = 0; c < changes; ++c)
                {
                    const int i = r.nextInt (32);
                    if (count > 18) { if (i % 8 != 0) st[(size_t) i].slice = -1; }
                    else if (st[(size_t) i].slice < 0) { st[(size_t) i].slice = r.nextInt (ns); st[(size_t) i].len = r.nextBool() ? 1 : 2; st[(size_t) i].vel = 0.7f; }
                }
                break;
            }
        }
        f.name = flipName (f.gen, k, seed);
        flips.push_back (f);
        flips[(size_t) node].kids.push_back ((int) flips.size() - 1);
    }
    ++flipVer;
}

void KeysKillaProcessor::flipPlay (int node)
{
    if (! juce::isPositiveAndBelow (node, (int) flips.size())) { flipOn = false; flipPlaying = -1; chop.allOff(); ++flipVer; return; }
    {
        const juce::SpinLock::ScopedLockType sl (flipLock);
        flipSeq = flips[(size_t) node].steps;
    }
    flipPlaying = node;
    if (! flipOn.load()) { flipLastStep = -1; flipOrigin = -1.0; }
    setPlayMode (playChop);
    flipOn = true;
    ++flipVer;
}

juce::File KeysKillaProcessor::flipMidiFile (int node) const
{
    if (! juce::isPositiveAndBelow (node, (int) flips.size())) return {};
    const auto& f = flips[(size_t) node];
    const int ppq = 960;
    juce::MidiMessageSequence seq;
    const double bpm = lastBpm.load();
    auto tempo = juce::MidiMessage::tempoMetaEvent ((int) std::round (60000000.0 / bpm)); tempo.setTimeStamp (0); seq.addEvent (tempo);
    for (int i = 0; i < (int) f.steps.size(); ++i)
    {
        const auto& s = f.steps[(size_t) i];
        if (s.slice < 0 || s.slice >= 128 - kk::ChopLab::firstNote) continue;
        const double b = i * 0.25, len = s.len * 0.25 * 0.95;
        const int note = kk::ChopLab::firstNote + s.slice;
        seq.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, (int) (s.vel * 120))), std::round (b * ppq));
        seq.addEvent (juce::MidiMessage::noteOff (1, note), std::round ((b + len) * ppq));
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (ppq); mf.addTrack (seq);
    auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("EVOLVE Flips");
    dir.createDirectory();
    auto file = dir.getChildFile (juce::File::createLegalFileName ("Flip - " + f.name + " - " + juce::String (juce::roundToInt (bpm)) + "BPM") + ".mid");
    file.deleteFile();
    if (juce::FileOutputStream os { file }; os.openedOk()) mf.writeTo (os, 1);
    return file;
}

juce::AudioBuffer<float> KeysKillaProcessor::renderFlip (int node, double bpm)
{
    juce::AudioBuffer<float> out;
    auto d = chop.current();
    if (d == nullptr || d->src == nullptr || ! juce::isPositiveAndBelow (node, (int) flips.size())) return out;
    const double rate = d->rate;
    const double stepS = rate * 60.0 / juce::jlimit (40.0, 300.0, bpm) / 4.0;
    const auto& f = flips[(size_t) node];
    const int total = (int) std::ceil (stepS * (double) f.steps.size());
    out.setSize (2, total); out.clear();
    for (int i = 0; i < (int) f.steps.size(); ++i)
    {
        const auto& s = f.steps[(size_t) i];
        if (s.slice < 0 || s.slice >= d->numSlices()) continue;
        auto b = chop.slice (s.slice);
        if (b.getNumSamples() < 8) continue;
        if (s.rev) b.reverse (0, b.getNumSamples());
        double ratio = std::exp2 (s.semi / 12.0);
        // until the next hit (or its length), 6 ms fade out
        int next = i + 1; while (next < (int) f.steps.size() && f.steps[(size_t) next].slice < 0 && next < i + s.len) ++next;
        const int maxLen = (int) (stepS * std::max (1, std::min (s.len, next - i)) + rate * 0.004);
        const int n = std::min ({ maxLen, (int) ((b.getNumSamples() - 2) / ratio), total - (int) (i * stepS) });
        const int o0 = (int) (i * stepS);
        const int fade = std::min (n / 4, (int) (rate * 0.006));
        for (int c = 0; c < 2; ++c)
            for (int k = 0; k < n; ++k)
            {
                const double p = k * ratio; const int i0 = (int) p; const float fr = (float) (p - i0);
                float y = b.getSample (c, i0) + (b.getSample (c, i0 + 1) - b.getSample (c, i0)) * fr;
                if (k >= n - fade) y *= (float) (n - k) / (float) std::max (1, fade);
                out.addSample (c, o0 + k, y * s.vel);
            }
    }
    const float pk = out.getMagnitude (0, out.getNumSamples());
    if (pk > 0.99f) out.applyGain (0.95f / pk);
    return out;
}

juce::File KeysKillaProcessor::flipWavFile (int node)
{
    auto d = chop.current();
    if (d == nullptr || ! juce::isPositiveAndBelow (node, (int) flips.size())) return {};
    auto b = renderFlip (node, lastBpm.load());
    if (b.getNumSamples() < 8) return {};
    auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("EVOLVE Flips")
                 .getChildFile (juce::File::createLegalFileName ("Flip - " + flips[(size_t) node].name + " - " + juce::String (juce::roundToInt (lastBpm.load())) + "BPM") + ".wav");
    return writeWavFile (b, d->rate, f) ? f : juce::File();
}

juce::File KeysKillaProcessor::sessionDir() const
{
    return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("Session Sounds");
}
