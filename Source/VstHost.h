#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <memory>

// v0.22 PAIR FROM VST: KEYS KILLA hosts another instrument (any VST3 / AU installed on this computer),
// plays it on the keys and CAPTURES its sound into the bank. Nothing is copied out of the other plugin:
// the sound is recorded from the instance installed on this computer.
namespace kk
{
class VstHost
{
public:
    VstHost() { juce::addDefaultFormatsToManager (formats); }
    ~VstHost() { unload(); }

    // folders the user added (FL Studio "Manage plugins" folders etc.), one per line
    static juce::File foldersFile()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("VST folders.txt");
    }
    static juce::StringArray userFolders()
    {
        juce::StringArray a; a.addLines (foldersFile().loadFileAsString()); a.trim(); a.removeEmptyStrings(); a.removeDuplicates (true);
        return a;
    }
    static void addUserFolder (const juce::File& dir)
    {
        auto a = userFolders(); a.addIfNotAlreadyThere (dir.getFullPathName());
        foldersFile().getParentDirectory().createDirectory();
        foldersFile().replaceWithText (a.joinIntoString ("\n"));
    }
    // FL Studio's own plugin database (Documents/Image-Line/.../Plugin database/Installed/*.nfo):
    // every plugin FL Studio found, with its file path - KEYS KILLA sees the same plugins as FL
    static juce::StringArray flStudioPlugins (bool generatorsOnly, bool vst2 = false)
    {
        juce::StringArray out;
        const auto il = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Image-Line");
        if (! il.isDirectory()) return out;
        // "Plugin database" sits a few levels down (FL 20+: Image-Line/Data/FL Studio/Presets/Plugin database)
        juce::Array<juce::File> dbs;
        for (auto sub : { "FL Studio/Presets/Plugin database", "Data/FL Studio/Presets/Plugin database" })
            if (auto f = il.getChildFile (sub); f.isDirectory()) dbs.add (f);
        for (const auto& e : juce::RangedDirectoryIterator (il, false, "*", juce::File::findDirectories))
            if (auto f = e.getFile().getChildFile ("Presets/Plugin database"); f.isDirectory() && ! dbs.contains (f)) dbs.add (f);
        for (auto& db : dbs)
        for (const auto& e : juce::RangedDirectoryIterator (db.getChildFile ("Installed").isDirectory() ? db.getChildFile ("Installed") : db, true, "*.nfo", juce::File::findFiles))
        {
            const auto path = e.getFile().getFullPathName();
            if (generatorsOnly && ! path.containsIgnoreCase ("Generators")) continue;
            juce::StringArray lines; lines.addLines (e.getFile().loadFileAsString());
            for (auto& l : lines)
            {
                const auto v = l.fromFirstOccurrenceOf ("=", false, false).trim().unquoted();
                const bool wanted = vst2 ? v.endsWithIgnoreCase (".dll") : (v.endsWithIgnoreCase (".vst3") || v.endsWithIgnoreCase (".component"));
                if (wanted && juce::File::isAbsolutePath (v) && juce::File (v).exists()) out.addIfNotAlreadyThere (v);
            }
        }
        return out;
    }
    // every plugin file in the standard folders + the usual extra ones + the user's folders
    // (message thread; it only lists files, nothing is loaded)
    juce::StringArray listInstalled() const
    {
        juce::StringArray all;
        for (auto* f : formats.getFormats())
        {
            auto paths = f->getDefaultLocationsToSearch();
           #if JUCE_WINDOWS
            const auto pf = juce::File::getSpecialLocation (juce::File::globalApplicationsDirectory);
            const auto pf86 = juce::File::getSpecialLocation (juce::File::globalApplicationsDirectoryX86);
            for (auto base : { pf, pf86 })
            {
                paths.addIfNotAlreadyThere (base.getChildFile ("Common Files").getChildFile ("VST3"));
                paths.addIfNotAlreadyThere (base.getChildFile ("VSTPlugins"));
                paths.addIfNotAlreadyThere (base.getChildFile ("Steinberg").getChildFile ("VSTPlugins"));
                paths.addIfNotAlreadyThere (base.getChildFile ("Image-Line").getChildFile ("Shared").getChildFile ("VST3"));
            }
           #endif
            for (auto& u : userFolders()) if (juce::File (u).isDirectory()) paths.addIfNotAlreadyThere (juce::File (u));
            for (auto& fl : flStudioPlugins (false)) paths.addIfNotAlreadyThere (juce::File (fl).getParentDirectory());   // FL Studio's folders
            paths.removeNonExistentPaths();
            auto ids = f->searchPathsForPlugins (paths, true, false);
            for (auto& id : ids)
                if (! id.containsIgnoreCase ("KEYS KILLA")) all.addIfNotAlreadyThere (id);   // never itself
        }
        for (auto& fl : flStudioPlugins (false))
            if (! fl.containsIgnoreCase ("KEYS KILLA")) all.addIfNotAlreadyThere (fl);
        all.sort (true);
        // FL Studio's instruments (generators) first, effects after
        const auto gens = flStudioPlugins (true);
        juce::StringArray first, rest;
        for (auto& a : all) (gens.contains (a, true) ? first : rest).add (a);
        first.addArray (rest);
        // the same plugin in two folders: show it once
        juce::StringArray out, names;
        for (auto& f : first) if (! names.contains (displayName (f), true)) { names.add (displayName (f)); out.add (f); }
        return out;
    }
    // VST2-only plugins FL Studio knows (KEYS KILLA can't load VST2 - shown so you know to install their VST3 version)
    static juce::StringArray vst2Only (const juce::StringArray& loadable)
    {
        juce::StringArray out, have;
        for (auto& l : loadable) have.add (displayName (l));
        for (auto& f : flStudioPlugins (false, true))
            if (! have.contains (displayName (f), true) && ! out.contains (displayName (f), true) && ! f.containsIgnoreCase ("KEYS KILLA")) out.add (displayName (f));
        out.sort (true);
        return out;
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
    // the plugin's own sound list (its presets / programs) - KEYS KILLA steps through it, no plugin window needed
    int numPrograms() const { return plugin != nullptr ? plugin->getNumPrograms() : 0; }
    juce::String programNameAt (int i) const { return plugin != nullptr ? plugin->getProgramName (i).trim() : juce::String(); }
    void setProgram (int i)
    {
        const juce::SpinLock::ScopedLockType l (lock);
        if (plugin != nullptr && i >= 0 && i < plugin->getNumPrograms()) plugin->setCurrentProgram (i);
    }
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
