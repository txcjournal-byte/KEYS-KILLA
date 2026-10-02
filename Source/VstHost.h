#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

// v0.22 PAIR FROM VST: KEYS KILLA hosts another instrument (Nexus, Serum 2, Zenology ... any VST3 / AU you own),
// plays it on the keys and CAPTURES its sound into the bank. Nothing is copied out of the other plugin:
// the sound is recorded from the instance installed on this computer.
namespace kk
{
class VstHost
{
public:
    VstHost() { juce::addDefaultFormatsToManager (formats); }
    ~VstHost() { unload(); }

    // every instrument plugin file in the standard folders (message thread; it only lists files, nothing is loaded)
    juce::StringArray listInstalled() const
    {
        juce::StringArray all;
        for (auto* f : formats.getFormats())
        {
            auto ids = f->searchPathsForPlugins (f->getDefaultLocationsToSearch(), true, false);
            for (auto& id : ids)
                if (! id.containsIgnoreCase ("KEYS KILLA")) all.addIfNotAlreadyThere (id);   // never itself
        }
        all.sortNatural();
        return all;
    }
    static juce::String displayName (const juce::String& id)
    {
        return juce::File::createFileWithoutCheckingPath (id).getFileNameWithoutExtension();
    }

    // load a plugin (message thread). Returns an error message or empty on success.
    juce::String load (const juce::String& fileOrId, double rate, int block)
    {
        juce::OwnedArray<juce::PluginDescription> types;
        for (auto* f : formats.getFormats())
            if (f->fileMightContainThisPluginType (fileOrId))
                f->findAllTypesForFile (types, fileOrId);
        if (types.isEmpty()) return "No plugin found in this file.";
        const juce::PluginDescription* pick = types[0];
        for (auto* t : types) if (t->isInstrument) { pick = t; break; }
        juce::String err;
        auto inst = formats.createPluginInstance (*pick, rate, block, err);
        if (inst == nullptr) return err.isEmpty() ? juce::String ("The plugin could not be opened.") : err;
        // stereo out, no input (an instrument)
        auto layout = inst->getBusesLayout();
        for (auto& b : layout.outputBuses) b = juce::AudioChannelSet::stereo();
        inst->setBusesLayout (layout);
        inst->enableAllBuses();
        inst->setRateAndBufferSizeDetails (rate, block);
        inst->prepareToPlay (rate, block);
        unload();
        {
            const juce::SpinLock::ScopedLockType l (lock);
            plugin = std::move (inst);
        }
        desc = *pick; sampleRate = rate; blockSize = block;
        scratch.setSize (std::max (2, std::max (plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels())), std::max (64, block));
        return {};
    }
    void unload()
    {
        std::unique_ptr<juce::AudioPluginInstance> old;
        { const juce::SpinLock::ScopedLockType l (lock); old = std::move (plugin); }
        if (old != nullptr) { if (auto* ed = old->getActiveEditor()) delete ed; old->releaseResources(); }
    }
    bool loaded() const { return plugin != nullptr; }
    juce::String name() const { return plugin != nullptr ? desc.name : juce::String(); }
    juce::String programName() const
    {
        if (plugin == nullptr) return {};
        const auto p = plugin->getProgramName (plugin->getCurrentProgram());
        return p.trim().isNotEmpty() ? p.trim() : juce::String();
    }
    juce::AudioPluginInstance* instance() const { return plugin.get(); }
    void prepare (double rate, int block)
    {
        const juce::SpinLock::ScopedLockType l (lock);
        sampleRate = rate; blockSize = block;
        if (plugin != nullptr)
        {
            plugin->setRateAndBufferSizeDetails (rate, block);
            plugin->prepareToPlay (rate, block);
            scratch.setSize (std::max (2, std::max (plugin->getTotalNumInputChannels(), plugin->getTotalNumOutputChannels())), std::max (64, block));
        }
    }

    // audio thread: play the plugin live (keys) and add it to L / R
    void process (float* L, float* R, int n, juce::MidiBuffer& midi, juce::AudioPlayHead* ph)
    {
        const juce::SpinLock::ScopedTryLockType tl (lock);
        if (! tl.isLocked() || plugin == nullptr || n > scratch.getNumSamples()) return;
        juce::AudioBuffer<float> io (scratch.getArrayOfWritePointers(), scratch.getNumChannels(), n);
        io.clear();
        plugin->setPlayHead (ph);
        plugin->processBlock (io, midi);
        const float* a = io.getReadPointer (0);
        const float* b = io.getReadPointer (io.getNumChannels() > 1 ? 1 : 0);
        for (int i = 0; i < n; ++i) { L[i] += a[i]; R[i] += b[i]; }
    }

    // CAPTURE (message thread): play one note offline and record it until it dies out (max ~8 s)
    juce::AudioBuffer<float> capture (int note, float velocity, double holdSeconds)
    {
        juce::AudioBuffer<float> out;
        const juce::SpinLock::ScopedLockType l (lock);   // the audio thread skips the plugin meanwhile
        if (plugin == nullptr) return out;
        const int block = std::max (64, std::min (blockSize, 1024));
        juce::AudioBuffer<float> b (scratch.getNumChannels(), block);
        plugin->setNonRealtime (true);
        plugin->reset();
        // let the plugin settle (some instruments need a few silent blocks after a preset change)
        juce::MidiBuffer none;
        for (int k = 0; k < 8; ++k) { b.clear(); none.clear(); plugin->processBlock (b, none); }
        const int maxLen = (int) (sampleRate * 8.0), hold = (int) (sampleRate * holdSeconds);
        out.setSize (2, maxLen); out.clear();
        int pos = 0, quiet = 0;
        while (pos + block <= maxLen)
        {
            juce::MidiBuffer m;
            if (pos == 0) m.addEvent (juce::MidiMessage::noteOn (1, note, velocity), 0);
            if (pos <= hold && pos + block > hold) m.addEvent (juce::MidiMessage::noteOff (1, note), hold - pos);
            b.clear();
            plugin->processBlock (b, m);
            out.copyFrom (0, pos, b, 0, 0, block);
            out.copyFrom (1, pos, b, b.getNumChannels() > 1 ? 1 : 0, 0, block);
            pos += block;
            if (pos > hold) { quiet = b.getMagnitude (0, block) < 1.0e-4f ? quiet + block : 0; if (quiet > (int) (sampleRate * 0.2)) break; }
        }
        out.setSize (2, std::max (block, pos - quiet), true);
        // all notes off and back to realtime
        juce::MidiBuffer off; off.addEvent (juce::MidiMessage::allNotesOff (1), 0);
        b.clear(); plugin->processBlock (b, off);
        plugin->setNonRealtime (false);
        return out;
    }
    double rate() const { return sampleRate; }

private:
    juce::AudioPluginFormatManager formats;
    juce::SpinLock lock;
    std::unique_ptr<juce::AudioPluginInstance> plugin;
    juce::PluginDescription desc;
    juce::AudioBuffer<float> scratch;
    double sampleRate = 44100.0;
    int blockSize = 512;
};
} // namespace kk
