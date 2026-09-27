#include "PluginProcessor.h"
#include "PluginEditor.h"

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
    X(m1) X(m2) X(m3) X(m4) X(m5) X(m6) \
    X(ghost) X(ghostOct) X(ghostRev) X(ghostBlur) X(bend) X(bendMode) X(bendSemis) X(tape) X(circuit) X(circRate) \
    X(chaos) X(morphX) X(morphY) X(body) X(bodyMix) X(seed)

// performance controls that never morph or get reset by presets
const juce::StringArray performanceIds { ID::chord, ID::chordType, ID::strum, ID::arp, ID::arpRate, ID::arpMode, ID::arpOct,
                                         ID::arpSwing, ID::arpGate, ID::keyLock, ID::key, ID::scale, ID::chaos,
                                         ID::morphX, ID::morphY, ID::bendRange,
                                         ID::m1, ID::m2, ID::m3, ID::m4, ID::m5, ID::m6 };
const juce::StringArray keepOnPresetLoad { ID::chord, ID::chordType, ID::strum, ID::arp, ID::arpRate, ID::arpMode, ID::arpOct,
                                           ID::arpSwing, ID::arpGate, ID::keyLock, ID::key, ID::scale, ID::chaos, ID::bendRange };

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
};

KeysKillaProcessor::KeysKillaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KEYSKILLA", createLayout())
{
    for (auto& p : playing) p = false;
    for (int i = 0; i < kk::numFxSlots; ++i) fxOrder[(size_t) i] = i;

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            params.push_back (rp);
            raw.push_back (apvts.getRawParameterValue (rp->getParameterID()));
            const auto id = rp->getParameterID();
            morphable.push_back (! performanceIds.contains (id) && id != ID::seed);
            discrete.push_back (dynamic_cast<juce::AudioParameterChoice*> (rp) != nullptr || dynamic_cast<juce::AudioParameterBool*> (rp) != nullptr);
        }
    ix = std::make_unique<Idx>();
   #define KK_SET(n) ix->n = indexOf (ID::n);
    KK_PARAMS (KK_SET)
   #undef KK_SET
    for (int s = 0; s < numModSlots; ++s)
    {
        ix->mmSrc[s] = indexOf (ID::mmSrc (s)); ix->mmDst[s] = indexOf (ID::mmDst (s)); ix->mmAmt[s] = indexOf (ID::mmAmt (s));
    }
    for (auto& bank : cornerBank) for (auto& c : bank) c.assign (params.size(), 0.0f);
    noteMap.fill (-1);
    shRng.seed (99); arpRng.seed (7);

    loadPreset (0);
    undoStack.clear(); redoStack.clear(); lastSnap.clear();
}

KeysKillaProcessor::~KeysKillaProcessor() { cancelPendingUpdate(); }

int KeysKillaProcessor::indexOf (const juce::String& id) const
{
    for (size_t i = 0; i < params.size(); ++i) if (params[i]->getParameterID() == id) return (int) i;
    jassertfalse;
    return 0;
}

bool KeysKillaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet().isDisabled();
}

void KeysKillaProcessor::prepareToPlay (double sampleRate, int)
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

    if (! bass)
    {
        cutOct += (m1 - 0.5f) * 5.0f;
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
    v.attack = P (I.attack); v.decay = P (I.decay); v.sustain = P (I.sustain); v.release = P (I.release); v.velSens = P (I.velSens);
    v.e3A = P (I.e3attack); v.e3D = P (I.e3decay); v.e3S = P (I.e3sustain); v.e3R = P (I.e3release);
    v.lfoPitch = c01 (lfoP); v.lfoFilter = c01 (lfoF); v.lfoAmp = P (I.lfoAmp);
    v.wobTarget = (int) P (I.wobTarget); v.wobble = c01 (wobble);
    v.mono = B (I.mono); v.legato = B (I.legato); v.glide = juce::jlimit (0.0f, 1.0f, glide);
    v.bendRange = raw[(size_t) I.bendRange]->load();
    v.ghost = P (I.ghost); v.ghostOct = (int) P (I.ghostOct);
    v.bend = P (I.bend); v.bendMode = (int) P (I.bendMode); v.bendSemis = P (I.bendSemis); v.tape = B (I.tape);
    v.punch = punch;
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
    f.delayMix = c01 (dMix); f.delayFb = P (I.delayFb); f.delayBeats = Choices::delayBeats ((int) P (I.delayTime)); f.delayMode = (int) P (I.delayMode);
    f.revMix = c01 (rMix); f.revSize = c01 (rSize); f.revType = (int) P (I.revType); f.freeze = B (I.freeze);
    f.eqLow = P (I.eqLow); f.eqHigh = P (I.eqHigh); f.reverse = P (I.reverse);
    f.width = c01 (width);
    f.ghost = P (I.ghost); f.ghostBlur = P (I.ghostBlur); f.ghostRev = B (I.ghostRev);
    f.circuit = P (I.circuit); f.circBeats = Choices::circBeats ((int) P (I.circRate));
    f.body = (int) P (I.body); f.bodyMix = P (I.bodyMix);
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
    const int mask = Choices::scaleMask ((int) raw[(size_t) ix->scale]->load());
    const int root = (int) raw[(size_t) ix->key]->load();
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
    const bool arp   = raw[(size_t) I.arp]->load() > 0.5f;
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

    if (! arp) return;

    // ------------------------ arpeggiator ------------------------
    int list[128 * 3]; int count = 0;
    const int mode = (int) raw[(size_t) I.arpMode]->load();
    const int octs = juce::jlimit (1, 3, (int) raw[(size_t) I.arpOct]->load());
    int base[128]; int nb = 0;
    if (mode == 4) for (int k = 0; k < arpOrderN; ++k) base[nb++] = arpOrder[(size_t) k];
    else for (int i = 0; i < 128; ++i) if (arpHeld[(size_t) i]) base[nb++] = i;
    for (int o = 0; o < octs; ++o) for (int k = 0; k < nb; ++k) if (base[k] + 12 * o < 128) list[count++] = base[k] + 12 * o;

    const double stepBeats = Choices::arpBeats ((int) raw[(size_t) I.arpRate]->load());
    const double swing = raw[(size_t) I.arpSwing]->load();
    const double gate = raw[(size_t) I.arpGate]->load();
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
        if (count == 0) continue;
        const int pos = juce::jlimit (0, n - 1, (int) ((b - beatPos) / bps));
        int note;
        switch (mode)
        {
            case 1:  arpIndex = (arpIndex - 1 + count) % count; note = list[arpIndex]; break;
            case 2:  if (count == 1) arpIndex = 0;
                     else { arpIndex += arpDir; if (arpIndex >= count) { arpIndex = count - 2; arpDir = -1; } else if (arpIndex < 0) { arpIndex = 1; arpDir = 1; } }
                     note = list[juce::jlimit (0, count - 1, arpIndex)]; break;
            case 3:  note = list[(int) (arpRng.uni() * (float) count) % count]; break;
            default: arpIndex = (arpIndex + 1) % count; note = list[arpIndex]; break;
        }
        out.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), pos);
        addPending (sampleClock + pos + juce::jmax ((int64_t) 16, (int64_t) (gate * stepBeats / bps)), note, 0, false);
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

    const bool chord = raw[(size_t) I.chord]->load() > 0.5f, arp = raw[(size_t) I.arp]->load() > 0.5f;
    const bool lock = raw[(size_t) I.keyLock]->load() > 0.5f;
    if (chord != lastChord || arp != lastArp || lock != lastKeyLock)
    {
        synth.allOff (false); arpHeld.fill (false); arpOrderN = 0; pendingN = 0; noteMap.fill (-1);
        lastChord = chord; lastArp = arp; lastKeyLock = lock;
    }

    buildVoiceParams (vp, fp);
    fp.bpm = bpm;

    processedMidi.clear();
    processMidi (midi, processedMidi, n, beatPos, bpm);

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

    meterL = std::max (peakL, meterL.load()); meterR = std::max (peakR, meterR.load());
    if (fxPtr->peakPre > 1.0f) overload = true;

    std::array<bool, 128> act; synth.activeNotes (act);
    for (size_t i = 0; i < 128; ++i) playing[i].store (act[i], std::memory_order_relaxed);
}

//==============================================================================
void KeysKillaProcessor::applyValues (const std::vector<std::pair<juce::String, float>>& values)
{
    presetJump = true;
    for (auto* rp : params)
        if (! keepOnPresetLoad.contains (rp->getParameterID()))
            rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, val] : values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (val));
}

void KeysKillaProcessor::snapshotForModified()
{
    loadedSnapshot.resize (params.size());
    for (size_t i = 0; i < params.size(); ++i) loadedSnapshot[i] = params[i]->getValue();
}

bool KeysKillaProcessor::isModified() const
{
    if (loadedSnapshot.size() != params.size()) return false;
    for (size_t i = 0; i < params.size(); ++i)
        if (! keepOnPresetLoad.contains (params[i]->getParameterID()) && params[i]->getParameterID() != ID::morphX
            && params[i]->getParameterID() != ID::morphY && std::abs (params[i]->getValue() - loadedSnapshot[i]) > 1.0e-4f)
            return true;
    return false;
}

void KeysKillaProcessor::loadPreset (int index)
{
    const auto& ps = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) ps.size())) return;
    const auto& pr = ps[(size_t) index];
    auto vals = pr.values;
    if (pr.tile == tBass)
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

    const bool bass = tile == tBass || raw[(size_t) ix->bassMode]->load() > 0.5f;
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
        static const std::vector<std::vector<int>> engs {
            { engFM, engPluck, engModal }, { engFM, engOrgan, engVA }, { engPluck, engVA, engFM, engWavetable }, { engFlute, engVox },
            { engVox, engOrchestral }, { engVA, engVox, engOrgan, engWavetable }, { engVA, engFM, engWavetable }, { engVA, engSub, engFM, engWavetable },
            { engPluck, engFM, engFlute, engModal }, { engVA, engFM, engPluck, engVox, engOrgan, engFlute, engWavetable, engModal } };
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
        if (pr.tile == tBass) { setv (ID::mono, 1); setv (ID::bassMode, 1); }
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
    for (int i = 0; i < kk::numFxSlots; ++i) o[(size_t) i] = i;
    auto t = juce::StringArray::fromTokens (str, ",", "");
    if (t.size() == kk::numFxSlots) for (int i = 0; i < kk::numFxSlots; ++i) o[(size_t) i] = t[i].getIntValue();
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
            apvts.replaceState (vt);
            syncParamsToState();
            auto cs = juce::StringArray::fromTokens (vt.getProperty ("corners", "-1,-1,-1,-1").toString(), ",", "");
            for (int c = 0; c < 4; ++c) corners[(size_t) c] = cs.size() == 4 ? cs[c].getIntValue() : -1;
            rebuildCornerBank();
            snapshotForModified();
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
    root->setProperty ("category", currentPreset >= 0 ? tileNames()[factoryPresets()[(size_t) currentPreset].tile] : juce::String ("USER"));
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
            if (! keepOnPresetLoad.contains (prop.name.toString()))
                vals.push_back ({ prop.name.toString(), (float) (double) prop.value });
    applyValues (vals);
    setFxOrder (orderFromString (v["fxOrder"].toString()));
    macroLabels = juce::StringArray::fromTokens (v["macroNames"].toString(), "|", ""); macroLabels.removeEmptyStrings();
    presetName = f.getFileNameWithoutExtension(); currentPreset = -1; userFile = f;
    snapshotForModified();
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
