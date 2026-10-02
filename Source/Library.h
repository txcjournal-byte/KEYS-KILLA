#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include "PairLab.h"

// v0.26 MY SOUNDS: your own sounds in your own folders (Documents/KEYS KILLA/My Sounds/<folder>/*.wav).
// Kept apart from the factory sounds and from the HARVEST bank - you create, rename and delete the folders.
namespace kk
{
struct Library
{
    static juce::File root()
    {
        return juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("My Sounds");
    }
    static juce::String defaultFolder() { return "MOJE ZVUKY"; }

    static juce::StringArray folders()
    {
        auto r = root();
        if (! r.getChildFile (defaultFolder()).isDirectory()) r.getChildFile (defaultFolder()).createDirectory();
        juce::StringArray out;
        for (const auto& e : juce::RangedDirectoryIterator (r, false, "*", juce::File::findDirectories))
            out.add (e.getFile().getFileName());
        out.sortNatural();
        out.removeString (defaultFolder()); out.insert (0, defaultFolder());   // yours first
        return out;
    }
    static juce::File folder (const juce::String& name) { return root().getChildFile (juce::File::createLegalFileName (name.trim().isEmpty() ? defaultFolder() : name.trim())); }
    static bool createFolder (const juce::String& name)
    {
        if (name.trim().isEmpty()) return false;
        return folder (name).createDirectory().wasOk();
    }
    static bool renameFolder (const juce::String& from, const juce::String& to)
    {
        if (to.trim().isEmpty() || folder (to).exists()) return false;
        return folder (from).moveFileTo (folder (to));
    }
    // folders and sounds go to the recycle bin (you can get them back)
    static bool deleteFolder (const juce::String& name) { auto f = folder (name); return f.isDirectory() && (f.moveToTrash() || f.deleteRecursively()); }
    static bool deleteSound (const juce::File& f) { return f.existsAsFile() && (f.moveToTrash() || f.deleteFile()); }

    static juce::Array<juce::File> sounds (const juce::String& name)
    {
        juce::Array<juce::File> out;
        for (const auto& e : juce::RangedDirectoryIterator (folder (name), false, "*.wav;*.aif;*.aiff;*.flac", juce::File::findFiles))
            out.add (e.getFile());
        struct Newest { static int compareElements (const juce::File& a, const juce::File& b)
        { const auto ta = a.getLastModificationTime(), tb = b.getLastModificationTime(); return ta > tb ? -1 : ta < tb ? 1 : 0; } } cmp;
        out.sort (cmp);
        return out;
    }
    // write a sound as a 24-bit WAV into a folder; returns the file
    static juce::File save (const PairSound& s, double rate, const juce::String& folderName)
    {
        auto dir = folder (folderName);
        dir.createDirectory();
        auto f = dir.getNonexistentChildFile (juce::File::createLegalFileName (s.name.isEmpty() ? juce::String ("KK sound") : s.name).substring (0, 90), ".wav", false);
        juce::WavAudioFormat wav;
        auto os = std::make_unique<juce::FileOutputStream> (f);
        if (! os->openedOk()) return {};
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), rate, 2, 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (s.audio, 0, s.audio.getNumSamples());
            return f;
        }
        return {};
    }
};
} // namespace kk
