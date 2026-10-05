#pragma once
#include <juce_core/juce_core.h>
#include "Library.h"

// v0.45 SOUND PACK: a MY SOUNDS folder -> one .zip you can send / sell / upload: every WAV + a readme.txt
// (pack name, the sound list, "made with EVOLVE by TrapVST").  No dialogs here - the caller shows the message.
namespace kk::pack
{
struct Result { bool ok = false; int sounds = 0; juce::File zip; juce::String message; };

inline juce::File packsFolder() { return kk::Library::root().getParentDirectory().getChildFile ("Sound Packs"); }

inline juce::String readme (const juce::String& packName, const juce::StringArray& names)
{
    juce::String t;
    t << packName.toUpperCase() << "\r\n" << juce::String::repeatedString ("=", juce::jmax (4, packName.length())) << "\r\n\r\n";
    t << names.size() << (names.size() == 1 ? " sound" : " sounds") << " (24-bit WAV)\r\n\r\n";
    for (int i = 0; i < names.size(); ++i) t << juce::String (i + 1).paddedLeft ('0', 2) << "  " << names[i] << "\r\n";
    t << "\r\nmade with EVOLVE by TrapVST\r\n" << juce::Time::getCurrentTime().formatted ("%Y-%m-%d") << "\r\n";
    return t;
}

// zips the WAVs of a folder (not its sub-folders) into zipOut: <pack name>/<sound>.wav + <pack name>/readme.txt
inline Result exportFolder (const juce::File& folder, const juce::File& zipOut, juce::String packName)
{
    Result res; res.zip = zipOut;
    packName = juce::File::createLegalFileName (packName.trim().isEmpty() ? folder.getFileName() : packName.trim());
    if (! folder.isDirectory()) { res.message = "the folder does not exist"; return res; }
    juce::Array<juce::File> wavs;
    for (const auto& e : juce::RangedDirectoryIterator (folder, false, "*.wav", juce::File::findFiles)) wavs.add (e.getFile());
    struct ByName { static int compareElements (const juce::File& a, const juce::File& b) { return a.getFileName().compareNatural (b.getFileName()); } } cmp;
    wavs.sort (cmp);
    if (wavs.isEmpty()) { res.message = "no WAV sounds in " + folder.getFileName(); return res; }
    juce::ZipFile::Builder zb;
    juce::StringArray names;
    for (auto& f : wavs) { zb.addFile (f, 6, packName + "/" + f.getFileName()); names.add (f.getFileNameWithoutExtension()); }
    const auto txt = readme (packName, names);
    zb.addEntry (std::make_unique<juce::MemoryInputStream> (txt.toRawUTF8(), txt.getNumBytesAsUTF8(), true), 9, packName + "/readme.txt", juce::Time::getCurrentTime());
    zipOut.getParentDirectory().createDirectory();
    juce::TemporaryFile tmp (zipOut);
    {
        juce::FileOutputStream os (tmp.getFile());
        if (! os.openedOk() || ! zb.writeToStream (os, nullptr)) { res.message = "could not write the pack"; return res; }
    }
    if (! tmp.overwriteTargetFileWithTemporary()) { res.message = "could not write the pack"; return res; }
    res.ok = true; res.sounds = wavs.size();
    res.message = "sound pack ready: " + zipOut.getFileName() + "  (" + juce::String (wavs.size()) + " sounds)";
    return res;
}

// MY SOUNDS > EXPORT AS SOUND PACK: Documents/KEYS KILLA/Sound Packs/<folder>.zip (a new name if it exists)
inline Result exportMySoundsFolder (const juce::String& folderName, const juce::String& packName = {})
{
    const auto name = packName.isNotEmpty() ? packName : folderName;
    packsFolder().createDirectory();
    const auto zip = packsFolder().getNonexistentChildFile (juce::File::createLegalFileName (name + " - EVOLVE pack"), ".zip", false);
    return exportFolder (kk::Library::folder (folderName), zip, name);
}
} // namespace kk::pack
