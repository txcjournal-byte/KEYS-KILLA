#include "VaultProcessor.h"
#include "VaultEditor.h"
#include "../Presets.h"
#include <mutex>
#if CV_HAS_BANK
 #include "VaultData.h"
#endif

namespace
{
juce::AudioProcessorValueTreeState::ParameterLayout createVaultLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    l.add (std::make_unique<AudioParameterInt> (ParameterID { VaultProcessor::idPreset, 1 }, "Preset", 0, VaultProcessor::maxPresets - 1, 0));
    for (int m = 0; m < vault::numMacros; ++m)
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { VaultProcessor::idMacro[m], 1 }, "Macro " + String (m + 1), NormalisableRange<float> (0.0f, 1.0f), 0.5f,
                   AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; })));
    // v3: the MAIN page knobs (bipolar around "as designed", vibrato / tremolo from 0) and the quick switches
    for (int q = 0; q < vault::numQuick; ++q)
    {
        const auto& qi = vault::quickInfo()[(size_t) q];
        const float neutral = qi.neutral;
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { VaultProcessor::idQuick[q], 1 }, String (qi.name), NormalisableRange<float> (0.0f, 1.0f), neutral,
                   AudioParameterFloatAttributes().withStringFromValueFunction ([neutral] (float v, int)
                   {
                       if (neutral <= 0.0f) return String (roundToInt (v * 100.0f)) + " %";
                       const int d = roundToInt ((v - 0.5f) * 200.0f);
                       return d == 0 ? String ("as designed") : (d > 0 ? "+" : "") + String (d) + " %";
                   })));
    }
    l.add (std::make_unique<AudioParameterBool> (ParameterID { VaultProcessor::idFxOn, 1 }, "FX On", true));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { VaultProcessor::idArpOn, 1 }, "Arp On", false));
    l.add (std::make_unique<AudioParameterBool> (ParameterID { VaultProcessor::idGateOn, 1 }, "Gate On", false));
    l.add (std::make_unique<AudioParameterFloat> (ParameterID { VaultProcessor::idVolume, 1 }, "Volume", NormalisableRange<float> (-60.0f, 6.0f, 0.0f, 2.5f), 0.0f,
               AudioParameterFloatAttributes().withLabel ("dB")
                   .withStringFromValueFunction ([] (float v, int) { return v <= -59.9f ? String ("-inf dB") : String (v, 1) + " dB"; })));
    return l;
}

float volumeGain (float dB) { return dB <= -59.9f ? 0.0f : juce::Decibels::decibelsToGain (dB); }
} // namespace

//==============================================================================
int VaultProcessor::vaultCategoryOfFactory (int c)
{
    using namespace vault;
    switch (c)
    {
        case cPiano:                            return catPiano;
        case cKeys: case cOrgan:                return catKeysOrgan;
        case cBells: case cMallets:             return catBellsMallets;
        case cPlucks:                           return catPlucks;
        case cGuitar:                           return catGuitars;
        case cStrings:                          return catStrings;
        case cBrass:                            return catBrass;
        case cChoir:                            return catVocalsChoir;
        case cWoodwind:                         return catFlutesWoodwinds;
        case cLead:                             return catLeads;
        case cPads:                             return catPads;
        case cSynth: case cChip:                return catSynth;
        case cBass: case c808:                  return catBass;
        case cArp:                              return catSynth;
        case cWorld:                            return catWorld;
        case cCinematic:                        return catOrchestral;
        case cTexture: case cFX: case cDrums: case cGameFx: default: return catFxAtmos;
    }
}

std::vector<VaultPreset> VaultProcessor::buildFallbackBank()
{
    std::vector<VaultPreset> out;
    const auto& fps = factoryPresets();
    out.reserve (fps.size());
    for (const auto& pr : fps)
    {
        VaultPreset p;
        p.id = "cv.fb." + pr.id.fromFirstOccurrenceOf ("kk.", false, false);
        p.name = pr.name;
        p.category = vaultCategoryOfFactory (pr.cat);
        VaultPart part;
        if (pr.isBass()) { part.values.push_back ({ ID::mono, 1.0f }); part.values.push_back ({ ID::bassMode, 1.0f }); }
        for (auto& [id, v] : pr.values)
            if (kk::vk::keyOf (id) >= 0) part.values.push_back ({ id, v });
        part.label = juce::String (categoryNames()[pr.cat]).toLowerCase();
        part.label = part.label.substring (0, 1).toUpperCase() + part.label.substring (1);
        p.parts.push_back (std::move (part));
        p.name = vault::v1::cleanText (p.name);
        out.push_back (std::move (p));
    }
    std::stable_sort (out.begin(), out.end(), [] (const VaultPreset& a, const VaultPreset& b) { return a.category < b.category; });
    return out;
}

std::shared_ptr<const VaultProcessor::Bank> VaultProcessor::sharedBank()
{
    static std::mutex lock;
    static std::weak_ptr<const Bank> cache;
    const std::lock_guard<std::mutex> g (lock);
    if (auto b = cache.lock()) return b;

    auto b = std::make_shared<Bank>();
    // 1. a bank file named by the environment (bank-generator / development builds)
    if (const auto env = juce::SystemStats::getEnvironmentVariable ("CHROME_VAULT_BANK", {}); env.isNotEmpty())
        if (juce::File::isAbsolutePath (env) && vault::loadBankFile (juce::File (env), b->presets) && ! b->presets.empty())
            b->source = BankSource::file;
   #if CV_HAS_BANK
    // 2. the bank(s) built into the plugin (resources/vault/*.cvb, in file-name order)
    if (b->presets.empty())
    {
        std::vector<std::pair<juce::String, int>> files;
        for (int i = 0; i < VaultData::namedResourceListSize; ++i)
            files.push_back ({ VaultData::originalFilenames[i], i });
        std::sort (files.begin(), files.end());
        for (auto& [fname, i] : files)
        {
            int size = 0;
            if (const auto* data = VaultData::getNamedResource (VaultData::namedResourceList[i], size); data != nullptr && fname.endsWithIgnoreCase (".cvb"))
            {
                std::vector<VaultPreset> part;
                if (vault::deserializeBank (data, (size_t) size, part))
                    b->presets.insert (b->presets.end(), std::make_move_iterator (part.begin()), std::make_move_iterator (part.end()));
            }
        }
        if (! b->presets.empty()) b->source = BankSource::embedded;
    }
   #endif
    // 3. fallback: the EVOLVE factory sounds, one layer each
    if (b->presets.empty()) { b->presets = buildFallbackBank(); b->source = BankSource::factoryFallback; }
    if ((int) b->presets.size() > maxPresets) b->presets.resize ((size_t) maxPresets);
    b->library = vault::SampleLibrary::getShared();
    b->compiled.reserve (b->presets.size());
    for (auto& p : b->presets) b->compiled.push_back (VaultEngine::compile (p, b->library));
    cache = b;
    return b;
}

//==============================================================================
VaultProcessor::VaultProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CHROMEVAULT", createVaultLayout())
{
    bankData = sharedBank();
    library = bankData->library;
    presetParam = apvts.getRawParameterValue (idPreset);
    for (int m = 0; m < vault::numMacros; ++m) macroParam[m] = apvts.getRawParameterValue (idMacro[m]);
    volumeParam = apvts.getRawParameterValue (idVolume);
    for (int q = 0; q < vault::numQuick; ++q) quickParam[q] = apvts.getRawParameterValue (idQuick[q]);
    fxOnParam = apvts.getRawParameterValue (idFxOn);
    arpOnParam = apvts.getRawParameterValue (idArpOn);
    gateOnParam = apvts.getRawParameterValue (idGateOn);
    for (auto& k : keysHeldUi) k.store (0.0f);
    startTimerHz (20);
    if (! bank().empty())
    {
        const auto& d = bank().front().macroDefaults;
        for (int m = 0; m < vault::numMacros; ++m)
            if (auto* p = apvts.getParameter (idMacro[m])) p->setValueNotifyingHost (p->convertTo0to1 (d[m]));
        resetLiveTo (0, false);
        publishLive();
    }
}

VaultProcessor::~VaultProcessor()
{
    stopTimer();
    updatePins (-1);
}

void VaultProcessor::updatePins (int index)
{
    // keep the current preset's instruments in RAM (and start loading them now, before the audio thread switches)
    if (index == pinnedFor || library == nullptr) return;
    std::vector<vault::InstrumentHandle*> want;
    if (juce::isPositiveAndBelow (index, (int) bankData->compiled.size()))
    {
        const auto& c = bankData->compiled[(size_t) index];
        for (int k = 0; k < c.numParts; ++k)
            if (auto* h = c.parts[(size_t) k].instrument) want.push_back (h);
    }
    for (auto* h : want) library->pin (h);
    for (auto* h : pinned) library->unpin (h);
    pinned = std::move (want);
    pinnedFor = index;
}

void VaultProcessor::timerCallback()
{
    updatePins (currentPreset());
    engine.serviceResources();
    // the preset changed without selectPreset (host automation / a host preset recall): the current sound follows
    if (const int i = currentPreset(); i >= 0 && i != liveFor) { resetLiveTo (i, true); publishLive(); }
}

//==============================================================================
void VaultProcessor::resetLiveTo (int index, bool keepLocked)
{
    if (! juce::isPositiveAndBelow (index, (int) bank().size())) return;
    const auto prev = live;
    const bool hadPrev = liveFor >= 0;
    live = bank()[(size_t) index];
    vault::upgradeToV3 (live);
    if (keepLocked && hadPrev)
    {
        if (getLock (Section::fx)) { live.fx = prev.fx; live.finish = prev.finish; }
        if (getLock (Section::arp)) live.arp = prev.arp;
        if (getLock (Section::gate)) live.gate = prev.gate;
    }
    liveFor = index;
}

void VaultProcessor::publishLive()
{
    if (liveFor < 0) return;
    editBuf[(size_t) editBack] = VaultEngine::compile (live, library);
    editBufFor[(size_t) editBack] = liveFor;
    editBack = editMid.exchange (editBack | 4) & 3;
}

bool VaultProcessor::isSoundEdited() const
{
    if (! juce::isPositiveAndBelow (liveFor, (int) bank().size())) return false;
    auto b = bank()[(size_t) liveFor];
    vault::upgradeToV3 (b);
    return ! (b == live);
}

void VaultProcessor::editSound (const std::function<void (VaultPreset&)>& change)
{
    if (liveFor < 0 || ! change) return;
    change (live);
    vault::upgradeToV3 (live);
    publishLive();
}

void VaultProcessor::revertSound()
{
    if (liveFor < 0) return;
    const int i = liveFor;
    liveFor = -1;
    auto locked = live;
    live = bank()[(size_t) i];
    vault::upgradeToV3 (live);
    if (getLock (Section::fx)) { live.fx = locked.fx; live.finish = locked.finish; }
    if (getLock (Section::arp)) live.arp = locked.arp;
    if (getLock (Section::gate)) live.gate = locked.gate;
    liveFor = i;
    publishLive();
}

juce::String VaultProcessor::macroName (int index) const
{
    return juce::isPositiveAndBelow (index, (int) vault::numSoundMacros) ? live.macros[(size_t) index].name : juce::String();
}

float VaultProcessor::getQuick (int q) const
{
    return juce::isPositiveAndBelow (q, (int) vault::numQuick) ? quickParam[q]->load() : 0.5f;
}

void VaultProcessor::setQuick (int q, float value01)
{
    if (! juce::isPositiveAndBelow (q, (int) vault::numQuick)) return;
    if (auto* p = apvts.getParameter (idQuick[q]))
    {
        const float v = juce::jlimit (0.0f, 1.0f, value01);
        if (std::abs (p->getValue() - v) > 1.0e-6f) { p->beginChangeGesture(); p->setValueNotifyingHost (v); p->endChangeGesture(); }
    }
}

void VaultProcessor::resetQuick()
{
    for (int q = 0; q < vault::numQuick; ++q) setQuick (q, vault::quickInfo()[(size_t) q].neutral);
}

bool VaultProcessor::getSwitch (const char* id) const
{
    if (auto* v = apvts.getRawParameterValue (id)) return v->load() > 0.5f;
    return false;
}

void VaultProcessor::setSwitch (const char* id, bool on)
{
    if (auto* p = apvts.getParameter (id))
        if ((p->getValue() > 0.5f) != on) { p->beginChangeGesture(); p->setValueNotifyingHost (on ? 1.0f : 0.0f); p->endChangeGesture(); }
}

namespace
{
int quickFxType (int q)
{
    switch (q)
    {
        case vault::qReverb: return (int) vault::FxType::reverb;
        case vault::qDelay:  return (int) vault::FxType::delay;
        case vault::qChorus: return (int) vault::FxType::chorus;
        case vault::qDrive:  return (int) vault::FxType::drive;
        case vault::qWidth:  return (int) vault::FxType::width;
        case vault::qEqTilt: return (int) vault::FxType::eq;
        default: return -1;
    }
}
} // namespace

int VaultProcessor::fxSlotForQuick (int q) const
{
    const int t = quickFxType (q);
    if (t < 0) return -1;
    for (int i = 0; i < (int) live.fx.size(); ++i) if ((int) live.fx[(size_t) i].type == t) return i;
    return -1;
}

bool VaultProcessor::isQuickFxOn (int q) const
{
    const int s = fxSlotForQuick (q);
    if (s >= 0) return live.fx[(size_t) s].on;
    return quickFxType (q) >= 0 && getQuick (q) > vault::quickInfo()[(size_t) q].neutral + 0.004f;
}

void VaultProcessor::setQuickFxOn (int q, bool on)
{
    const int t = quickFxType (q);
    if (t < 0) return;
    if (const int s = fxSlotForQuick (q); s >= 0) { editSound ([s, on] (VaultPreset& p) { p.fx[(size_t) s].on = on; }); return; }
    if (! on) { setQuick (q, vault::quickInfo()[(size_t) q].neutral); return; }
    editSound ([t] (VaultPreset& p)
    {
        if ((int) p.fx.size() >= vault::maxFxSlots) return;
        auto slot = VaultFxSlot::make ((vault::FxType) t);
        if (slot.type == vault::FxType::width) slot.set ("width", 0.7f);
        const int rank = vault::fxTypeInfo (slot.type).rank;
        auto at = p.fx.end();
        for (auto it = p.fx.begin(); it != p.fx.end(); ++it) if (vault::fxTypeInfo (it->type).rank > rank) { at = it; break; }
        p.fx.insert (at, slot);
    });
}

juce::MidiFile VaultProcessor::arpPatternMidi (double bars) const
{
    std::vector<std::pair<int, float>> notes;
    for (int k = 0; k < 128; ++k) if (const float v = keysHeldUi[(size_t) k].load(); v > 0.0f) notes.push_back ({ k, v });
    if (notes.empty()) notes = { { 60, 0.8f }, { 64, 0.8f }, { 67, 0.8f } };   // the C5 major triad
    return vault::arpPatternToMidi (live.arp, notes, bars, hostBpm.load());
}

juce::File VaultProcessor::writeArpMidiFile (double bars) const
{
    const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("CHROME VAULT Arp.mid");
    f.deleteFile();
    juce::FileOutputStream os (f);
    if (! os.openedOk()) return {};
    arpPatternMidi (bars).writeTo (os);
    os.flush();
    return f;
}

bool VaultProcessor::isLoading() const
{
    const int i = currentPreset();
    if (i < 0) return false;
    const auto& c = bankData->compiled[(size_t) i];
    for (int k = 0; k < c.numParts; ++k)
        if (auto* h = c.parts[(size_t) k].instrument; h != nullptr && ! h->isLoaded() && ! h->hasFailed()) return true;
    return engine.isLoading();
}

bool VaultProcessor::isMissingSamples() const
{
    const int i = currentPreset();
    if (i < 0) return false;
    const auto& c = bankData->compiled[(size_t) i];
    for (int k = 0; k < c.numParts; ++k)
    {
        const auto& part = c.parts[(size_t) k];
        if (part.kind == VaultPartKind::sample && (part.instrument == nullptr || part.instrument->hasFailed())) return true;
    }
    return false;
}

juce::String VaultProcessor::bankDescription() const
{
    const char* src = bankSource() == BankSource::embedded ? "built-in bank" : bankSource() == BankSource::file ? "bank file" : "starter bank";
    return juce::String ((int) bank().size()) + " presets (" + src + ")";
}

int VaultProcessor::currentPreset() const
{
    const int n = (int) bank().size();
    if (n == 0) return -1;
    return juce::jlimit (0, n - 1, (int) std::round (presetParam->load()));
}

const VaultPreset* VaultProcessor::currentPresetData() const
{
    const int i = currentPreset();
    return i >= 0 ? &bank()[(size_t) i] : nullptr;
}

int VaultProcessor::findPreset (const juce::String& id) const
{
    for (int i = 0; i < (int) bank().size(); ++i) if (bank()[(size_t) i].id == id) return i;
    return -1;
}

void VaultProcessor::selectPreset (int index)
{
    if (bank().empty()) return;
    index = juce::jlimit (0, (int) bank().size() - 1, index);
    if (auto* p = apvts.getParameter (idPreset))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) index));
        p->endChangeGesture();
    }
    updatePins (index);
    const auto& d = bank()[(size_t) index].macroDefaults;
    for (int m = 0; m < vault::numMacros; ++m) setMacro (m, d[m]);
    // v3: the new sound (+ the locked sections), its arp / gate switches, the MAIN knobs back to "as designed"
    // (the FX knobs stay while the FX are locked)
    resetLiveTo (index, true);
    publishLive();
    if (! getLock (Section::arp)) setSwitch (idArpOn, live.arp.on);
    if (! getLock (Section::gate)) setSwitch (idGateOn, live.gate.on);
    for (int q = 0; q < vault::numQuick; ++q)
    {
        const bool fxKnob = q == vault::qReverb || q == vault::qDelay || q == vault::qChorus || q == vault::qDrive || q == vault::qWidth || q == vault::qEqTilt;
        if (! (fxKnob && getLock (Section::fx))) setQuick (q, vault::quickInfo()[(size_t) q].neutral);
    }
    updateHostDisplay (ChangeDetails().withProgramChanged (true));
}

float VaultProcessor::getMacro (int index) const
{
    return juce::isPositiveAndBelow (index, (int) vault::numMacros) ? macroParam[index]->load() : 0.5f;
}

void VaultProcessor::setMacro (int index, float value01)
{
    if (! juce::isPositiveAndBelow (index, (int) vault::numMacros)) return;
    if (auto* p = apvts.getParameter (idMacro[index]))
    {
        const float v = p->convertTo0to1 (juce::jlimit (0.0f, 1.0f, value01));
        if (std::abs (p->getValue() - v) > 1.0e-6f) { p->beginChangeGesture(); p->setValueNotifyingHost (v); p->endChangeGesture(); }
    }
}

float VaultProcessor::getVolumeDb() const { return volumeParam->load(); }

void VaultProcessor::setVolumeDb (float dB)
{
    if (auto* p = apvts.getParameter (idVolume)) { p->beginChangeGesture(); p->setValueNotifyingHost (p->convertTo0to1 (juce::jlimit (-60.0f, 6.0f, dB))); p->endChangeGesture(); }
}

void VaultProcessor::previewNote (int midiNote, float velocity, double seconds)
{
    previewVel = juce::jlimit (0.05f, 1.0f, velocity);
    previewLen = (int) (std::max (0.05, seconds) * 1000.0);   // ms
    previewReq = juce::jlimit (0, 127, midiNote);
}

//==============================================================================
void VaultProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    engine.prepare (sampleRate, std::max (16, samplesPerBlock));
    engine.serviceResources();
    loadedIndex = -1;
    previewActive = -1; previewOffIn = -1;
    keyboardState.reset();
}

void VaultProcessor::releaseResources() { engine.releaseResources(); }

bool VaultProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::stereo() || out == juce::AudioChannelSet::mono();
}

void VaultProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (! engine.isPrepared() || n <= 0) { buffer.clear(); return; }

    // the switches, locks and MAIN knobs (before a preset load: the locks decide what it keeps)
    engine.setLocks (getLock (Section::fx), getLock (Section::arp), getLock (Section::gate));
    engine.setFxOn (fxOnParam->load() > 0.5f);
    engine.setArpOn (arpOnParam->load() > 0.5f);
    engine.setGateOn (gateOnParam->load() > 0.5f);
    for (int q = 0; q < vault::numQuick; ++q) engine.setQuick (q, quickParam[q]->load());
    // the current sound from the editor (an edit, a restored project, a preset pick): a triple buffer, no locks
    const VaultCompiledPreset* edit = nullptr;
    if (editMid.load() & 4) { editFront = editMid.exchange (editFront) & 3; edit = &editBuf[(size_t) editFront]; editAppliedFor = editBufFor[(size_t) editFront]; }
    // preset: from the parameter (UI pick or host automation)
    if (! bank().empty())
    {
        const int fromParam = juce::jlimit (0, (int) bank().size() - 1, (int) std::round (presetParam->load()));
        const int want = fromParam;   // the APVTS stores a new value at once, so UI picks arrive here in the next block
        if (want != loadedIndex)
        {
            if (editAppliedFor == want) engine.setPreset (editBuf[(size_t) editFront], loadedIndex < 0, false);   // the current sound of that preset
            else engine.setPreset (bankData->compiled[(size_t) want], loadedIndex < 0);
            loadedIndex = want;
        }
        else if (edit != nullptr && editAppliedFor == loadedIndex) engine.updateSound (*edit);
    }
    for (int m = 0; m < vault::numMacros; ++m) engine.setMacro (m, macroParam[m]->load());
    engine.setVolume (volumeGain (volumeParam->load()));

    // UI audition
    if (previewActive >= 0 && previewOffIn >= 0)
    {
        if (previewOffIn < n) { midi.addEvent (juce::MidiMessage::noteOff (1, previewActive), previewOffIn); previewActive = -1; previewOffIn = -1; }
        else previewOffIn -= n;
    }
    if (const int pn = previewReq.exchange (-1); pn >= 0)
    {
        if (previewActive >= 0) midi.addEvent (juce::MidiMessage::noteOff (1, previewActive), 0);
        midi.addEvent (juce::MidiMessage::noteOn (1, pn, previewVel.load()), 0);
        previewActive = pn; previewOffIn = (int) (sr * previewLen.load() / 1000.0);
    }
    keyboardState.processNextMidiBuffer (midi, 0, n, true);
    for (const auto meta : midi)   // the keys down (arp pattern MIDI export)
    {
        if (meta.numBytes < 3) continue;
        const int status = meta.data[0] & 0xf0, key = meta.data[1] & 0x7f;
        if (status == 0x90 && meta.data[2] > 0) keysHeldUi[(size_t) key].store ((float) meta.data[2] / 127.0f);
        else if (status == 0x80 || status == 0x90) keysHeldUi[(size_t) key].store (0.0f);
    }

    VaultEngine::Transport t;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) t.bpm = *b;
            if (pos->getIsPlaying()) if (auto p = pos->getPpqPosition()) { t.playing = true; t.ppq = *p; }
        }
    hostBpm.store (t.bpm);
    engine.process (buffer, midi, t);
    outPeak = buffer.getMagnitude (0, n);
}

//==============================================================================
juce::AudioProcessorEditor* VaultProcessor::createEditor() { return new VaultEditor (*this); }

void VaultProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree st ("CHROMEVAULT");
    st.setProperty ("version", 2, nullptr);
    if (const auto* p = currentPresetData()) { st.setProperty ("presetId", p->id, nullptr); st.setProperty ("presetName", p->name, nullptr); }
    st.setProperty ("presetIndex", currentPreset(), nullptr);
    for (int m = 0; m < vault::numMacros; ++m) st.setProperty (idMacro[m], getMacro (m), nullptr);
    st.setProperty (idVolume, getVolumeDb(), nullptr);
    // v3: MAIN knobs, switches, locks and the current sound when it differs from the bank preset
    for (int q = 0; q < vault::numQuick; ++q) st.setProperty (idQuick[q], getQuick (q), nullptr);
    for (auto* id : { idFxOn, idArpOn, idGateOn }) st.setProperty (id, getSwitch (id), nullptr);
    st.setProperty ("lockFx", getLock (Section::fx), nullptr);
    st.setProperty ("lockArp", getLock (Section::arp), nullptr);
    st.setProperty ("lockGate", getLock (Section::gate), nullptr);
    if (liveFor == currentPreset() && isSoundEdited())
    {
        const auto mb = vault::serializeBank ({ live }, true);
        st.setProperty ("sound", mb.toBase64Encoding(), nullptr);
    }
    if (auto xml = st.createXml()) copyXmlToBinary (*xml, destData);
}

void VaultProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    const auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;
    const auto st = juce::ValueTree::fromXml (*xml);
    if (! st.hasType ("CHROMEVAULT")) return;
    int idx = findPreset (st.getProperty ("presetId").toString());   // by id first: survives bank re-ordering
    if (idx < 0) idx = juce::jlimit (0, std::max (0, (int) bank().size() - 1), (int) st.getProperty ("presetIndex", 0));
    if (! bank().empty())
    {
        if (auto* p = apvts.getParameter (idPreset)) p->setValueNotifyingHost (p->convertTo0to1 ((float) idx));
    }
    for (int m = 0; m < vault::numMacros; ++m)
        if (st.hasProperty (idMacro[m])) setMacro (m, (float) st.getProperty (idMacro[m]));
    if (st.hasProperty (idVolume)) setVolumeDb ((float) st.getProperty (idVolume));
    // v3 (a version 1 state: everything as designed)
    setLock (Section::fx, (bool) st.getProperty ("lockFx", false));
    setLock (Section::arp, (bool) st.getProperty ("lockArp", false));
    setLock (Section::gate, (bool) st.getProperty ("lockGate", false));
    for (int q = 0; q < vault::numQuick; ++q) setQuick (q, (float) st.getProperty (idQuick[q], vault::quickInfo()[(size_t) q].neutral));
    if (bank().empty()) return;
    liveFor = -1;
    resetLiveTo (idx, false);
    setSwitch (idFxOn, (bool) st.getProperty (idFxOn, true));
    setSwitch (idArpOn, (bool) st.getProperty (idArpOn, live.arp.on));
    setSwitch (idGateOn, (bool) st.getProperty (idGateOn, live.gate.on));
    if (st.hasProperty ("sound"))
    {
        juce::MemoryBlock mb;
        std::vector<VaultPreset> snd;
        if (mb.fromBase64Encoding (st.getProperty ("sound").toString()) && vault::deserializeBank (mb.getData(), mb.getSize(), snd)
            && snd.size() == 1 && vault::validatePreset (snd.front()))
            live = snd.front();
    }
    publishLive();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VaultProcessor(); }
