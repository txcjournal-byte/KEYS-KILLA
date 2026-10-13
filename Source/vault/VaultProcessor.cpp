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
    {
        const auto name = vault::macroNames()[m].substring (0, 1) + vault::macroNames()[m].substring (1).toLowerCase();
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { VaultProcessor::idMacro[m], 1 }, name, NormalisableRange<float> (0.0f, 1.0f), 0.5f,
                   AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return String (roundToInt (v * 100.0f)) + " %"; })));
    }
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
        case cPiano: case cKeys: case cOrgan:   return catPianoKeys;
        case cBells: case cMallets:             return catBellsMallets;
        case cPlucks:                           return catPlucks;
        case cGuitar:                           return catGuitars;
        case cStrings:                          return catStrings;
        case cBrass:                            return catBrassHorns;
        case cChoir:                            return catVocalsChoir;
        case cWoodwind:                         return catFlutesWinds;
        case cLead:                             return catLeads;
        case cPads:                             return catPads;
        case cSynth: case cChip:                return catSynth;
        case cBass: case c808:                  return cat808Bass;
        case cArp:                              return catArpsSequences;
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

        using namespace vault;
        uint32_t st = 1u << styleTrap, ch = 0;
        if (pr.mood == "Dark")       st |= 1u << styleDark;
        if (pr.mood == "Sad")        st |= 1u << styleEmo;
        if (pr.mood == "Dreamy")     st |= 1u << styleAmbient;
        if (pr.mood == "Aggressive") st |= 1u << styleRage;
        if (pr.mood == "Bouncy")     st |= 1u << stylePluggnb;
        if (pr.era == 3)             st |= 1u << styleLoFi;
        if (pr.brightness >= 4) ch |= 1u << charBright;
        if (pr.brightness <= 2 || pr.mood == "Dark") ch |= 1u << charDark;
        if (pr.mood == "Warm" || pr.character.contains ("Warm")) ch |= 1u << charWarm;
        if (pr.character.contains ("Dirty")) ch |= 1u << charDirty; else ch |= 1u << charClean;
        if (pr.articulation == "Short" || pr.articulation == "Pluck") ch |= 1u << charHard;
        if (pr.articulation == "Swell") ch |= 1u << charSoft;
        for (auto& [id, v] : pr.values) if (id == ID::width && v > 0.7f) ch |= 1u << charWide;
        p.styles = st; p.chars = ch;
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
    b->compiled.reserve (b->presets.size());
    for (auto& p : b->presets) b->compiled.push_back (VaultEngine::compile (p));
    cache = b;
    return b;
}

//==============================================================================
VaultProcessor::VaultProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "CHROMEVAULT", createVaultLayout())
{
    bankData = sharedBank();
    presetParam = apvts.getRawParameterValue (idPreset);
    for (int m = 0; m < vault::numMacros; ++m) macroParam[m] = apvts.getRawParameterValue (idMacro[m]);
    volumeParam = apvts.getRawParameterValue (idVolume);
    if (! bank().empty())
    {
        const auto& d = bank().front().macroDefaults;
        for (int m = 0; m < vault::numMacros; ++m)
            if (auto* p = apvts.getParameter (idMacro[m])) p->setValueNotifyingHost (p->convertTo0to1 (d[m]));
    }
}

VaultProcessor::~VaultProcessor() = default;

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
    const auto& d = bank()[(size_t) index].macroDefaults;
    for (int m = 0; m < vault::numMacros; ++m) setMacro (m, d[m]);
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

    // preset: from the parameter (UI pick or host automation)
    if (! bank().empty())
    {
        const int fromParam = juce::jlimit (0, (int) bank().size() - 1, (int) std::round (presetParam->load()));
        const int want = fromParam;   // the APVTS stores a new value at once, so UI picks arrive here in the next block
        if (want != loadedIndex)
        {
            engine.setPreset (bankData->compiled[(size_t) want], loadedIndex < 0);
            loadedIndex = want;
        }
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

    VaultEngine::Transport t;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) t.bpm = *b;
            if (pos->getIsPlaying()) if (auto p = pos->getPpqPosition()) { t.playing = true; t.ppq = *p; }
        }
    engine.process (buffer, midi, t);
    outPeak = buffer.getMagnitude (0, n);
}

//==============================================================================
juce::AudioProcessorEditor* VaultProcessor::createEditor() { return new VaultEditor (*this); }

void VaultProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::ValueTree st ("CHROMEVAULT");
    st.setProperty ("version", 1, nullptr);
    if (const auto* p = currentPresetData()) { st.setProperty ("presetId", p->id, nullptr); st.setProperty ("presetName", p->name, nullptr); }
    st.setProperty ("presetIndex", currentPreset(), nullptr);
    for (int m = 0; m < vault::numMacros; ++m) st.setProperty (idMacro[m], getMacro (m), nullptr);
    st.setProperty (idVolume, getVolumeDb(), nullptr);
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
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VaultProcessor(); }
