#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr int kChunk = 512;
const std::array<int, 4> chordIntervals { 0, 3, 7, 10 };   // minor 7th - dark trap default
}

KeysKillaProcessor::KeysKillaProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "KEYSKILLA", createLayout())
{
    for (auto& p : playing) p = false;
    loadPreset (0);
    buildVoiceParams (vp, fp);   // caches parameter pointers off the audio thread
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
    fx.prepare (sampleRate, kChunk);
    bufL.assign (kChunk, 0.0f); bufR.assign (kChunk, 0.0f); bufG.assign (kChunk, 0.0f); lfoBuf.assign (kChunk, 0.0f);
    processedMidi.ensureSize (4096);
    arpHeld.fill (false); arpNote = -1; arpLastStep = -1;
    midiPitch = midiMod = 0;
}

static float rawValue (juce::AudioProcessorValueTreeState& s, const char* id)
{
    return s.getRawParameterValue (id)->load();
}

void KeysKillaProcessor::buildVoiceParams (kk::VoiceParams& v, kk::FxParams& f)
{
    // Parameter pointers are cached on first use (lookups by string allocate)
    static const char* ids[] {
        ID::engine, ID::octave, ID::wave, ID::unison, ID::detune, ID::sub, ID::fmRatio, ID::fmAmt,
        ID::cutoff, ID::reso, ID::fenv, ID::fdecay, ID::attack, ID::decay, ID::sustain, ID::release, ID::velSens,
        ID::lfoPitch, ID::lfoFilter, ID::lfoAmp, ID::mono, ID::glide, ID::bassMode,
        ID::drive, ID::driveType, ID::crush, ID::wow, ID::chorus, ID::delayMix, ID::delayTime, ID::delayFb,
        ID::revMix, ID::revSize, ID::width, ID::gain,
        ID::m1, ID::m2, ID::m3, ID::m4, ID::m5, ID::m6,
        ID::ghost, ID::bend, ID::circuit, ID::morphX, ID::morphY, ID::body, ID::bodyMix, ID::seed };
    enum { eEngine, eOct, eWave, eUni, eDet, eSub, eRatio, eFmAmt, eCut, eRes, eFenv, eFdec, eA, eD, eS, eR, eVel,
           eLp, eLf, eLa, eMono, eGlide, eBass, eDrive, eDType, eCrush, eWow, eChorus, eDMix, eDTime, eDFb,
           eRMix, eRSize, eWidth, eGain, eM1, eM2, eM3, eM4, eM5, eM6, eGhost, eBend, eCirc, eMX, eMY, eBody, eBodyMix, eSeed, eCount };
    if (paramCache.empty())
        for (int i = 0; i < eCount; ++i) paramCache.push_back (apvts.getRawParameterValue (ids[i]));
    auto P = [this] (int i) { return paramCache[(size_t) i]->load(); };

    const bool bass = P (eBass) > 0.5f;
    float cutOct = 0, drive = P (eDrive), crush = P (eCrush), wow = P (eWow), chorus = P (eChorus);
    float dMix = P (eDMix), rMix = P (eRMix), rSize = P (eRSize), width = P (eWidth), detune = P (eDet);
    float lfoF = P (eLf), lfoP = P (eLp), sub = P (eSub), glide = P (eGlide), punch = 0;
    const float m1 = P (eM1), m2 = P (eM2), m3 = P (eM3), m4 = P (eM4), m5 = P (eM5), m6 = P (eM6);

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
        sub += m1; lfoF += m2; drive += m3 * 0.8f; glide += m4 * 0.4f;
        cutOct += (m5 - 0.5f) * 5.0f; punch = m6;
    }

    // ERA MORPH: corners TL classic, TR melodic, BL raw, BR aggressive; neutral at centre
    const float x = P (eMX), y = P (eMY);
    const float dC = (1 - x) * y - 0.25f, dM = x * y - 0.25f, dR = (1 - x) * (1 - y) - 0.25f, dA = x * (1 - y) - 0.25f;
    crush += dC * 0.35f; width -= dC * 0.3f; chorus -= dC * 0.2f; rMix += dC * 0.1f;
    rMix += dM * 0.35f; chorus += dM * 0.5f; dMix += dM * 0.3f; cutOct -= dM * 0.5f;
    rMix -= dR * 0.4f; dMix -= dR * 0.3f; drive += dR * 0.15f; cutOct += dR * 0.3f; width -= dR * 0.2f;
    drive += dA * 0.9f; cutOct += dA * 1.0f; width += dA * 0.4f;

    auto c01 = kk::clamp01;
    v.engine = (int) P (eEngine); v.octave = (int) P (eOct); v.unison = juce::jlimit (1, 7, (int) P (eUni));
    v.wave = P (eWave); v.detune = c01 (detune); v.sub = c01 (sub); v.fmRatio = P (eRatio); v.fmAmt = P (eFmAmt);
    v.cutoff = juce::jlimit (40.0f, 20000.0f, P (eCut) * std::exp2 (cutOct));
    v.reso = P (eRes); v.fenv = P (eFenv); v.fdecay = P (eFdec);
    v.attack = P (eA); v.decay = P (eD); v.sustain = P (eS); v.release = P (eR); v.velSens = P (eVel);
    v.lfoPitch = c01 (lfoP); v.lfoFilter = c01 (lfoF); v.lfoAmp = P (eLa);
    v.mono = P (eMono) > 0.5f; v.glide = juce::jlimit (0.0f, 1.0f, glide);
    v.ghost = P (eGhost); v.bend = P (eBend); v.punch = punch;
    v.pitchWheelSemis = juce::jlimit (-1.0f, 1.0f, midiPitch + guiPitch.load()) * 2.0f;
    v.modWheel = std::max (midiMod, guiMod.load());

    f.drive = c01 (drive); f.driveType = (int) P (eDType); f.cleanLow = bass; f.monoLows = bass;
    f.crush = c01 (crush); f.wow = c01 (wow); f.chorus = c01 (chorus);
    f.delayMix = c01 (dMix); f.delayFb = P (eDFb); f.delayBeats = Choices::delayBeats ((int) P (eDTime));
    f.revMix = c01 (rMix); f.revSize = c01 (rSize); f.width = c01 (width);
    f.ghost = P (eGhost); f.circuit = P (eCirc); f.body = (int) P (eBody); f.bodyMix = P (eBodyMix);
    f.outGain = juce::Decibels::decibelsToGain (P (eGain));
    f.seed = (uint32_t) P (eSeed);
}

void KeysKillaProcessor::handleMidi (const juce::MidiMessage& m)
{
    if (m.isNoteOn())                 synth.noteOn (m.getNoteNumber(), m.getFloatVelocity(), vp);
    else if (m.isNoteOff())           synth.noteOff (m.getNoteNumber(), vp);
    else if (m.isSustainPedalOn())    synth.setSustain (true);
    else if (m.isSustainPedalOff())   synth.setSustain (false);
    else if (m.isAllNotesOff() || m.isAllSoundOff()) { synth.setSustain (false); synth.allOff (m.isAllSoundOff()); arpHeld.fill (false); }
    else if (m.isPitchWheel())        midiPitch = (float) (m.getPitchWheelValue() - 8192) / 8192.0f;
    else if (m.isController() && m.getControllerNumber() == 1) midiMod = (float) m.getControllerValue() / 127.0f;
}

void KeysKillaProcessor::expandAndArp (const juce::MidiBuffer& in, juce::MidiBuffer& out, int numSamples,
                                       double beatPos, double bpm, bool)
{
    const bool chord = rawValue (apvts, ID::chord) > 0.5f;
    const bool arp   = rawValue (apvts, ID::arp) > 0.5f;

    auto emitNote = [&] (bool on, int note, float vel, int pos)
    {
        if (arp)
        {
            if (note >= 0 && note < 128) arpHeld[(size_t) note] = on;
            return;
        }
        if (note < 0 || note > 127) return;
        out.addEvent (on ? juce::MidiMessage::noteOn (1, note, vel) : juce::MidiMessage::noteOff (1, note), pos);
    };

    for (const auto meta : in)
    {
        const auto m = meta.getMessage();
        if (m.isNoteOnOrOff())
        {
            const bool on = m.isNoteOn();
            if (chord) for (int iv : chordIntervals) emitNote (on, m.getNoteNumber() + iv, m.getFloatVelocity(), meta.samplePosition);
            else emitNote (on, m.getNoteNumber(), m.getFloatVelocity(), meta.samplePosition);
            if (arp && arpNote >= 0)
            {
                bool any = false; for (bool b : arpHeld) any |= b;
                if (! any) { out.addEvent (juce::MidiMessage::noteOff (1, arpNote), meta.samplePosition); arpNote = -1; }
            }
        }
        else
        {
            if (m.isAllNotesOff() || m.isAllSoundOff()) { arpHeld.fill (false); arpNote = -1; }
            out.addEvent (m, meta.samplePosition);
        }
    }

    if (! arp) return;

    const double stepBeats = Choices::arpBeats ((int) rawValue (apvts, ID::arpRate));
    const double bps = bpm / 60.0 / sr;
    const double endBeat = beatPos + bps * numSamples;
    int64_t step = (int64_t) std::floor (beatPos / stepBeats);
    double b = (double) step * stepBeats;
    if (step == arpLastStep) { ++step; b += stepBeats; }
    for (; b < endBeat; ++step, b += stepBeats)
    {
        const int pos = juce::jlimit (0, numSamples - 1, (int) ((b - beatPos) / bps));
        if (arpNote >= 0) { out.addEvent (juce::MidiMessage::noteOff (1, arpNote), pos); arpNote = -1; }
        int held[128]; int count = 0;
        for (int i = 0; i < 128; ++i) if (arpHeld[(size_t) i]) held[count++] = i;
        arpLastStep = step;
        if (count == 0) continue;
        arpIndex = (arpIndex + 1) % count;
        arpNote = held[arpIndex];
        out.addEvent (juce::MidiMessage::noteOn (1, arpNote, (juce::uint8) 100), pos);
    }
}

void KeysKillaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    buffer.clear();
    keyboardState.processNextMidiBuffer (midi, 0, n, true);

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

    const bool chord = rawValue (apvts, ID::chord) > 0.5f, arp = rawValue (apvts, ID::arp) > 0.5f;
    if (chord != lastChord || arp != lastArp)
    {
        synth.allOff (false); arpHeld.fill (false); arpNote = -1;
        lastChord = chord; lastArp = arp;
    }

    buildVoiceParams (vp, fp);
    fp.bpm = bpm;

    processedMidi.clear();
    expandAndArp (midi, processedMidi, n, beatPos, bpm, hostPlaying);

    const bool lfoSync = rawValue (apvts, ID::lfoSync) > 0.5f;
    const double lfoBeats = Choices::lfoDivBeats ((int) rawValue (apvts, ID::lfoDiv));
    const float lfoInc = rawValue (apvts, ID::lfoRate) / (float) sr;

    auto* outL = buffer.getWritePointer (0);
    auto* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr;
    auto midiIt = processedMidi.cbegin();
    fx.peakPre = 0;
    float peakL = 0, peakR = 0;

    for (int c0 = 0; c0 < n; c0 += kChunk)
    {
        const int len = std::min (kChunk, n - c0);
        std::fill_n (bufL.data(), len, 0.0f); std::fill_n (bufR.data(), len, 0.0f); std::fill_n (bufG.data(), len, 0.0f);

        for (int i = 0; i < len; ++i)
        {
            if (lfoSync) { const double ph = (beatPos + bps * (c0 + i)) / lfoBeats; lfoBuf[(size_t) i] = std::sin (kk::twoPi * (float) (ph - std::floor (ph))); }
            else { lfoBuf[(size_t) i] = std::sin (kk::twoPi * lfoPhase); lfoPhase += lfoInc; if (lfoPhase >= 1) lfoPhase -= 1; }
        }

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
            synth.render (bufL.data() + pos, bufR.data() + pos, bufG.data() + pos, next - pos, vp, lfoBuf.data() + pos);
            pos = next;
        }

        fp.beatPos = beatPos + bps * c0;
        fx.process (bufL.data(), bufR.data(), bufG.data(), len, fp);

        for (int i = 0; i < len; ++i)
        {
            outL[c0 + i] = bufL[(size_t) i];
            if (outR) outR[c0 + i] = bufR[(size_t) i];
            peakL = std::max (peakL, std::abs (bufL[(size_t) i]));
            peakR = std::max (peakR, std::abs (bufR[(size_t) i]));
        }
    }
    while (midiIt != processedMidi.cend()) { handleMidi ((*midiIt).getMessage()); ++midiIt; }

    meterL = std::max (peakL, meterL.load()); meterR = std::max (peakR, meterR.load());
    if (fx.peakPre > 1.0f) overload = true;

    std::array<bool, 128> act; synth.activeNotes (act);
    for (size_t i = 0; i < 128; ++i) playing[i].store (act[i], std::memory_order_relaxed);
}

//==============================================================================
void KeysKillaProcessor::applyValues (const std::vector<std::pair<const char*, float>>& values)
{
    static const juce::StringArray keep { ID::chord, ID::arp, ID::arpRate, ID::chaos };
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (! keep.contains (rp->getParameterID()))
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, val] : values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (val));
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
    applyValues (vals);
    currentPreset = index;
    presetName = pr.name;
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

void KeysKillaProcessor::initPatch()
{
    applyValues ({});
    currentPreset = -1;
    presetName = "Init";
}

const juce::String KeysKillaProcessor::getProgramName (int index)
{
    const auto& ps = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) ps.size()) ? ps[(size_t) index].name : juce::String();
}

void KeysKillaProcessor::rollDice (int tile)
{
    diceHistory.push_back (apvts.copyState());
    if (diceHistory.size() > 20) diceHistory.erase (diceHistory.begin());

    const bool bass = tile == tBass || rawValue (apvts, ID::bassMode) > 0.5f;
    const float chaos = rawValue (apvts, ID::chaos);
    juce::Random r;

    struct R { const char* id; float lo, hi; };
    std::vector<R> ranges {
        { ID::wave, 0, 1 }, { ID::detune, 0, 0.7f }, { ID::unison, 0, 1 }, { ID::fmRatio, 0, 0.6f }, { ID::fmAmt, 0, 0.7f },
        { ID::cutoff, 0.4f, 0.95f }, { ID::reso, 0, 0.55f }, { ID::fenv, 0.4f, 0.9f }, { ID::fdecay, 0.1f, 0.6f },
        { ID::attack, 0, 0.25f }, { ID::decay, 0.2f, 0.8f }, { ID::sustain, 0, 1 }, { ID::release, 0.1f, 0.55f },
        { ID::lfoRate, 0.2f, 0.8f }, { ID::lfoPitch, 0, 0.12f }, { ID::lfoFilter, 0, bass ? 0.8f : 0.4f },
        { ID::drive, 0, 0.5f }, { ID::driveType, 0, 1 }, { ID::crush, 0, 0.35f }, { ID::wow, 0, 0.4f },
        { ID::chorus, 0, bass ? 0.0f : 0.6f }, { ID::delayMix, 0, bass ? 0.05f : 0.35f }, { ID::revMix, 0, bass ? 0.08f : 0.5f },
        { ID::revSize, 0.2f, 0.9f }, { ID::width, 0.3f, bass ? 0.5f : 0.8f },
        { ID::ghost, 0, bass ? 0.1f : 0.5f }, { ID::bend, 0, 0.35f }, { ID::circuit, 0, 0.25f } };

    for (auto& rg : ranges)
        if (auto* p = apvts.getParameter (rg.id))
        {
            const float cur = p->getValue();
            const float target = rg.lo + r.nextFloat() * (rg.hi - rg.lo);
            p->setValueNotifyingHost (cur + (target - cur) * chaos);
        }

    if (chaos > 0.6f && r.nextFloat() < chaos - 0.4f)
    {
        static const std::vector<std::vector<int>> engs {
            { engFM, engPluck }, { engFM, engOrgan, engVA }, { engPluck, engVA, engFM }, { engFlute, engVox },
            { engVox }, { engVA, engVox, engOrgan }, { engVA, engFM }, { engVA, engSub, engFM },
            { engPluck, engFM, engFlute }, { engVA, engFM, engPluck, engVox, engOrgan, engFlute } };
        const auto& list = engs[(size_t) juce::jlimit (0, (int) engs.size() - 1, tile)];
        auto* p = apvts.getParameter (ID::engine);
        p->setValueNotifyingHost (p->convertTo0to1 ((float) list[(size_t) r.nextInt ((int) list.size())]));
    }
    if (auto* p = apvts.getParameter (ID::seed)) p->setValueNotifyingHost (r.nextFloat());
    if (bass) { apvts.getParameter (ID::mono)->setValueNotifyingHost (1.0f); apvts.getParameter (ID::bassMode)->setValueNotifyingHost (1.0f); }

    presetName = "DICE #" + juce::String (++diceCount);
    currentPreset = -1;
}

bool KeysKillaProcessor::undoDice()
{
    if (diceHistory.empty()) return false;
    apvts.replaceState (diceHistory.back());
    syncParamsToState();
    diceHistory.pop_back();
    presetName = diceHistory.empty() ? juce::String ("Undo") : "DICE #" + juce::String (juce::jmax (1, --diceCount));
    return true;
}

//==============================================================================
void KeysKillaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetIndex", currentPreset, nullptr);
    state.setProperty ("presetName", presetName, nullptr);
    state.setProperty ("version", 1, nullptr);
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
            apvts.replaceState (vt);
            syncParamsToState();
        }
}

// replaceState() skips parameters whose denormalised value did not change (e.g. a bool sitting at 0.28),
// so push every stored value explicitly.
void KeysKillaProcessor::syncParamsToState()
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto child = apvts.state.getChildWithProperty ("id", rp->getParameterID());
            if (child.isValid() && child.hasProperty ("value"))
            {
                const float norm = rp->convertTo0to1 ((float) child.getProperty ("value"));
                if (std::abs (rp->getValue() - norm) > 1.0e-6f) rp->setValueNotifyingHost (norm);
            }
        }
}

bool KeysKillaProcessor::saveUserPreset (const juce::File& f)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", f.getFileNameWithoutExtension(), nullptr);
    state.setProperty ("version", 1, nullptr);
    if (auto xml = state.createXml())
        if (xml->writeTo (f)) { presetName = f.getFileNameWithoutExtension(); currentPreset = -1; return true; }
    return false;
}

bool KeysKillaProcessor::loadUserPreset (const juce::File& f)
{
    if (auto xml = juce::XmlDocument::parse (f))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            syncParamsToState();
            presetName = f.getFileNameWithoutExtension(); currentPreset = -1;
            return true;
        }
    return false;
}

juce::AudioProcessorEditor* KeysKillaProcessor::createEditor() { return new KeysKillaEditor (*this); }

#if ! KK_TEST_BUILD
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KeysKillaProcessor(); }
#endif
