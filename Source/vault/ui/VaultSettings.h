#pragma once
#include <juce_data_structures/juce_data_structures.h>
#include <juce_events/juce_events.h>

// CHROME VAULT user settings (favorites, recent, window scale, preview on click), one file per user:
//   Windows  %APPDATA%\TrapWin\CHROME VAULT\CHROME VAULT.settings
//   macOS    ~/Library/Application Support/TrapWin/CHROME VAULT/CHROME VAULT.settings
//   Linux    ~/.config/TrapWin/CHROME VAULT/CHROME VAULT.settings
// One shared object per process (all open plugin windows see the same favorites - they get a change message).
// Message thread only.
class VaultSettings : public juce::ChangeBroadcaster
{
public:
    static std::shared_ptr<VaultSettings> get()
    {
        static std::weak_ptr<VaultSettings> cache;
        if (auto s = cache.lock()) return s;
        auto s = std::shared_ptr<VaultSettings> (new VaultSettings());
        cache = s;
        return s;
    }
    // tests / snapshots: use this file instead of the user's (call before the first get())
    static juce::File& fileOverride() { static juce::File f; return f; }

    ~VaultSettings() override { if (props != nullptr) props->saveIfNeeded(); }

    juce::StringArray favorites() const { return lines ("favorites"); }
    juce::StringArray recent() const    { return lines ("recent"); }
    void setFavorites (const juce::StringArray& ids) { setLines ("favorites", ids); sendChangeMessage(); }
    void setRecent (const juce::StringArray& ids)    { setLines ("recent", ids); }

    float scale() const { return props != nullptr ? (float) props->getDoubleValue ("uiScale", 0.0) : 0.0f; }   // 0 = never set
    void setScale (float s) { if (props != nullptr) props->setValue ("uiScale", (double) s); }
    bool previewOnClick() const { return props == nullptr || props->getBoolValue ("previewOnClick", true); }
    void setPreviewOnClick (bool on) { if (props != nullptr) props->setValue ("previewOnClick", on); sendChangeMessage(); }

    juce::File file() const { return props != nullptr ? props->getFile() : juce::File(); }
    void flush() { if (props != nullptr) props->saveIfNeeded(); }

private:
    VaultSettings()
    {
        juce::PropertiesFile::Options o;
        o.applicationName = "CHROME VAULT";
        o.folderName = "TrapWin/CHROME VAULT";
        o.filenameSuffix = ".settings";
        o.osxLibrarySubFolder = "Application Support";
        o.storageFormat = juce::PropertiesFile::storeAsXML;
        o.millisecondsBeforeSaving = 400;
        o.commonToAllUsers = false;
        const auto f = fileOverride() != juce::File() ? fileOverride() : o.getDefaultFile();
        f.getParentDirectory().createDirectory();
        props = std::make_unique<juce::PropertiesFile> (f, o);
    }
    juce::StringArray lines (const juce::String& key) const
    {
        if (props == nullptr) return {};
        auto s = juce::StringArray::fromLines (props->getValue (key));
        s.removeEmptyStrings();
        return s;
    }
    void setLines (const juce::String& key, const juce::StringArray& v) { if (props != nullptr) props->setValue (key, v.joinIntoString ("\n")); }

    std::unique_ptr<juce::PropertiesFile> props;
};
