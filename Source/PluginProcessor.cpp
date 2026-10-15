#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

LuckyNotesProcessor::LuckyNotesProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
    settings.seed = (uint32_t) juce::Random::getSystemRandom().nextInt();
    settings.resolvedKey = { settings.root, settings.minor };
    rebuild();
}

bool LuckyNotesProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.inputBuses.size() > 0 && ! layouts.getMainInputChannelSet().isDisabled())
        return false;
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void LuckyNotesProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.synth.setWave ((ln::Wave) soundAtomic.load());
    engine.prepare (sampleRate);
    mono.assign ((size_t) std::max (samplesPerBlock, 1024), 0.0f);
    freeBeat = 0.0;
    audioPatternVersion = -1;
    audioRow = -1;
}

void LuckyNotesProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) { render (buffer, midi); }
void LuckyNotesProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi) { render (buffer, midi); }

template <typename Sample>
void LuckyNotesProcessor::render (juce::AudioBuffer<Sample>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    midi.clear();
    const int numSamples = buffer.getNumSamples();
    if (mono.empty()) mono.assign (1024, 0.0f);

    // host transport
    bool playing = false, hasPpq = false;
    double ppq = 0.0, bpmFromHost = 0.0;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpmFromHost = *b;
            playing = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) { ppq = *p; hasPpq = true; }
        }
    hostBpm.store (bpmFromHost > 0.0 && std::isfinite (bpmFromHost) ? bpmFromHost : 0.0);
    hostPlaying.store (playing);

    // new pattern from the message thread?
    {
        const juce::GenericScopedTryLock<juce::SpinLock> tl (patternLock);
        if (tl.isLocked() && pending.version != audioPatternVersion)
        {
            audioCopy.count = pending.count;
            audioCopy.length = pending.length;
            std::copy (pending.notes.begin(), pending.notes.begin() + pending.count, audioCopy.notes.begin());
            audioPatternVersion = pending.version;
            engine.seq.setPattern (audioCopy.notes.data(), audioCopy.count, audioCopy.length);
        }
    }

    const int row = previewRow.load();
    const int req = previewRequest.load();
    if (req != audioRequestSeen) { audioRequestSeen = req; freeBeat = 0.0; engine.seq.resetPosition(); }
    engine.synth.setWave ((ln::Wave) juce::jlimit (0, 2, soundAtomic.load()));

    const bool active = row >= 0 && engine.seq.hasPattern();
    const double hb = hostBpm.load();
    const double bpm = (syncAtomic.load() && hb > 0.0) ? hb : manualBpmAtomic.load();
    const double sr = getSampleRate() > 0 ? getSampleRate() : 44100.0;
    const double bps = juce::jlimit (1.0, 999.0, bpm) / 60.0 / sr;
    const bool hostSync = playing && hasPpq;
    if (! hostSync && wasHostPlaying) freeBeat = std::max (0.0, ppq); // host stopped: keep running from there
    wasHostPlaying = hostSync;
    const double beatStart = hostSync ? ppq : freeBeat;

    const int chunk = (int) mono.size();
    for (int pos = 0; pos < numSamples; pos += chunk)
    {
        const int n = std::min (chunk, numSamples - pos);
        const int ne = engine.render (mono.data(), n, beatStart + pos * bps, bps, active, events, 256);
        for (int i = 0; i < ne; ++i)
        {
            const auto& e = events[i];
            const auto msg = e.on ? juce::MidiMessage::noteOn (1, e.pitch, (juce::uint8) juce::jlimit (1, 127, e.velocity))
                                  : juce::MidiMessage::noteOff (1, e.pitch);
            midi.addEvent (msg, pos + e.sampleOffset);
        }
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            auto* d = buffer.getWritePointer (ch, pos);
            for (int i = 0; i < n; ++i) d[i] = (Sample) mono[(size_t) i];
        }
    }

    const double beatEnd = beatStart + numSamples * bps;
    if (! hostSync) freeBeat = beatEnd;
    if (active && audioCopy.length > 0)
    {
        double b = std::fmod (beatEnd, audioCopy.length);
        if (b < 0) b += audioCopy.length;
        previewBeat.store (b);
    }
    audioRow = row;
}

// ---------------------------------------------------------------------------------------------------------------
LuckySettings LuckyNotesProcessor::getSettings() const
{
    const std::lock_guard<std::mutex> g (lock);
    return settings;
}

std::shared_ptr<const ln::MelodySet> LuckyNotesProcessor::getSet() const
{
    const std::lock_guard<std::mutex> g (lock);
    return set;
}

double LuckyNotesProcessor::getEffectiveBpm() const
{
    const double hb = hostBpm.load();
    return (syncAtomic.load() && hb > 0.0) ? hb : manualBpmAtomic.load();
}

void LuckyNotesProcessor::rebuild()
{
    ln::SetSettings s;
    {
        const std::lock_guard<std::mutex> g (lock);
        s.key = settings.resolvedKey;
        s.bars = settings.bars;
        s.seed = settings.seed;
        s.rolls = settings.rolls;
    }
    auto fresh = std::make_shared<const ln::MelodySet> (ln::generateSet (s));
    {
        const std::lock_guard<std::mutex> g (lock);
        set = std::move (fresh);
        manualBpmAtomic.store (settings.manualBpm);
        syncAtomic.store (settings.sync);
        soundAtomic.store (settings.sound);
    }
    pushPattern();
    ++stateVersion;
}

void LuckyNotesProcessor::pushPattern()
{
    const int row = previewRow.load();
    auto s = getSet();
    if (s == nullptr) return;
    const juce::SpinLock::ScopedLockType sl (patternLock);
    if (row < 0) { pending.count = 0; ++pending.version; return; }
    const auto& m = s->melodies[(size_t) row];
    pending.count = (int) std::min (m.notes.size(), pending.notes.size());
    std::copy (m.notes.begin(), m.notes.begin() + pending.count, pending.notes.begin());
    pending.length = m.lengthBeats;
    ++pending.version;
}

void LuckyNotesProcessor::setKey (int root, bool minor)
{
    {
        const std::lock_guard<std::mutex> g (lock);
        settings.root = juce::jlimit (0, 11, root);
        settings.minor = minor;
        settings.lucky = false;
        settings.resolvedKey = { settings.root, settings.minor };
    }
    rebuild();
}

void LuckyNotesProcessor::setLucky (bool lucky)
{
    {
        const std::lock_guard<std::mutex> g (lock);
        settings.lucky = lucky;
    }
    ++stateVersion;
}

void LuckyNotesProcessor::setBars (int bars)
{
    {
        const std::lock_guard<std::mutex> g (lock);
        settings.bars = bars >= 16 ? 16 : 8;
    }
    rebuild();
}

void LuckyNotesProcessor::setSync (bool sync)
{
    {
        const std::lock_guard<std::mutex> g (lock);
        settings.sync = sync;
    }
    syncAtomic.store (sync);
    ++stateVersion;
}

void LuckyNotesProcessor::setManualBpm (double bpm)
{
    bpm = juce::jlimit (kMinBpm, kMaxBpm, std::round (bpm * 10.0) / 10.0);
    {
        const std::lock_guard<std::mutex> g (lock);
        settings.manualBpm = bpm;
    }
    manualBpmAtomic.store (bpm);
    ++stateVersion;
}

void LuckyNotesProcessor::setSound (int sound)
{
    {
        const std::lock_guard<std::mutex> g (lock);
        settings.sound = juce::jlimit (0, 2, sound);
    }
    soundAtomic.store (juce::jlimit (0, 2, sound));
    ++stateVersion;
}

void LuckyNotesProcessor::generate()
{
    {
        const std::lock_guard<std::mutex> g (lock);
        settings.seed = (uint32_t) juce::Random::getSystemRandom().nextInt();
        settings.rolls.fill (0);
        settings.resolvedKey = settings.lucky ? ln::luckyKey (settings.seed) : ln::Key { settings.root, settings.minor };
    }
    rebuild();
}

void LuckyNotesProcessor::reroll (int row)
{
    if (row < 0 || row >= ln::kNumMelodies) return;
    {
        const std::lock_guard<std::mutex> g (lock);
        ++settings.rolls[(size_t) row];
    }
    rebuild();
}

void LuckyNotesProcessor::startPreview (int row)
{
    previewRow.store (juce::jlimit (-1, ln::kNumMelodies - 1, row));
    ++previewRequest;
    pushPattern();
}

void LuckyNotesProcessor::stopPreview()
{
    previewRow.store (-1);
}

juce::String LuckyNotesProcessor::midiFileName (int row, bool chords) const
{
    auto s = getSet();
    const int bars = s != nullptr ? s->settings.bars : 8;
    const ln::Key key = s != nullptr ? s->settings.key : ln::Key {};
    return juce::String (ln::midiFileName (row, getEffectiveBpm(), key, bars, chords));
}

juce::File LuckyNotesProcessor::writeMidiToFolder (int row, const juce::File& folder, bool chords) const
{
    auto s = getSet();
    if (s == nullptr || row < 0 || row >= ln::kNumMelodies) return {};
    const auto& m = s->melodies[(size_t) row];
    const double bpm = getEffectiveBpm();
    const auto bytes = chords ? ln::chordsMidi (m, bpm, s->settings.key) : ln::melodyMidi (m, bpm, s->settings.key);
    folder.createDirectory();
    auto f = folder.getChildFile (juce::String (ln::midiFileName (row, bpm, s->settings.key, s->settings.bars, chords)));
    if (! f.replaceWithData (bytes.data(), bytes.size())) return {};
    return f;
}

// ---------------------------------------------------------------------------------------------------------------
void LuckyNotesProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    const auto s = getSettings();
    juce::ValueTree t ("LUCKYNOTES");
    t.setProperty ("v", 1, nullptr);
    t.setProperty ("root", s.root, nullptr);
    t.setProperty ("minor", s.minor, nullptr);
    t.setProperty ("lucky", s.lucky, nullptr);
    t.setProperty ("bars", s.bars, nullptr);
    t.setProperty ("sync", s.sync, nullptr);
    t.setProperty ("bpm", s.manualBpm, nullptr);
    t.setProperty ("sound", s.sound, nullptr);
    t.setProperty ("seed", juce::String ((juce::int64) s.seed), nullptr);
    t.setProperty ("keyRoot", s.resolvedKey.root, nullptr);
    t.setProperty ("keyMinor", s.resolvedKey.minor, nullptr);
    juce::StringArray rolls;
    for (auto r : s.rolls) rolls.add (juce::String ((juce::int64) r));
    t.setProperty ("rolls", rolls.joinIntoString (","), nullptr);
    t.setProperty ("ui", uiScale.load(), nullptr);
    if (auto xml = t.createXml())
        copyXmlToBinary (*xml, destData);
}

void LuckyNotesProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr) return;
    const auto t = juce::ValueTree::fromXml (*xml);
    if (! t.hasType ("LUCKYNOTES")) return;
    {
        const std::lock_guard<std::mutex> g (lock);
        auto& s = settings;
        s.root = juce::jlimit (0, 11, (int) t.getProperty ("root", s.root));
        s.minor = (bool) t.getProperty ("minor", s.minor);
        s.lucky = (bool) t.getProperty ("lucky", s.lucky);
        s.bars = (int) t.getProperty ("bars", s.bars) >= 16 ? 16 : 8;
        s.sync = (bool) t.getProperty ("sync", s.sync);
        s.manualBpm = juce::jlimit (kMinBpm, kMaxBpm, (double) t.getProperty ("bpm", s.manualBpm));
        s.sound = juce::jlimit (0, 2, (int) t.getProperty ("sound", s.sound));
        s.seed = (uint32_t) t.getProperty ("seed", juce::String ((juce::int64) s.seed)).toString().getLargeIntValue();
        s.resolvedKey.root = juce::jlimit (0, 11, (int) t.getProperty ("keyRoot", s.root));
        s.resolvedKey.minor = (bool) t.getProperty ("keyMinor", s.minor);
        juce::StringArray rolls;
        rolls.addTokens (t.getProperty ("rolls", "").toString(), ",", "");
        for (int i = 0; i < ln::kNumMelodies; ++i)
            s.rolls[(size_t) i] = i < rolls.size() ? (uint32_t) rolls[i].getLargeIntValue() : 0u;
    }
    uiScale.store (juce::jlimit (0.8, 1.5, (double) t.getProperty ("ui", uiScale.load())));
    rebuild();
}

juce::AudioProcessorEditor* LuckyNotesProcessor::createEditor() { return new LuckyNotesEditor (*this); }

#if ! defined (LN_SNAPSHOT_BUILD)
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LuckyNotesProcessor(); }
#endif
