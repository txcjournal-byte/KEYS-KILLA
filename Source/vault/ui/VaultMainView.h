#pragma once
#include "VaultPages.h"

// the whole CHROME VAULT v3 UI at the base size 1180 x 760 (VaultEditor scales it as one piece):
//   top bar   logo | MAIN | ADVANCED: LAYERS FX MOD ARP · GATE | quick toggles FX ARP GATE | size menu | settings
//   page      MAIN (browser screen, quick knobs, sound macros) or one ADVANCED page
//   keyboard  C3..B6, C5 (MIDI 60) = the ice key
class VaultMainView : public juce::Component
{
public:
    static constexpr int topH = 54, keysH = 66, padX = 16, padTop = 14, padBottom = 14;
    enum PageId { pageMain, pageLayers, pageFx, pageMod, pageArp, numPages };

    VaultMainView (VaultProcessor&, std::shared_ptr<VaultSettings>);
    ~VaultMainView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    void showPage (int pageId);
    int currentPage() const { return page; }
    vui::Page& pageComponent (int pageId);
    vui::MainPage& mainPage() { return *mainP; }
    vui::LayersPage& layersPage() { return *layersP; }
    vui::FxPage& fxPage() { return *fxP; }
    vui::ModPage& modPage() { return *modP; }
    vui::ArpGatePage& arpPage() { return *arpP; }
    juce::Rectangle<int> pageArea() const;
    void refreshAll();                                  // every page reads the current sound again

    // browsing (MAIN page)
    void loadPreset (int bankIndex, bool audition) { mainP->loadPreset (bankIndex, audition); }
    void step (int delta, bool wrap) { mainP->step (delta, wrap); }
    void auditionCurrent() { mainP->auditionCurrent(); }
    void syncFromProcessor();                           // timer: host automation / state restore, step lights, loading state
    void setCategory (int c) { mainP->setCategory (c); }
    void setQuery (const juce::String& q) { mainP->setQuery (q); }
    void setLayers (int n) { mainP->setLayers (n); }
    void setFavoritesOnly (bool on) { mainP->setFavoritesOnly (on); }
    void setRecentOnly (bool on) { mainP->setRecentOnly (on); }
    void toggleFavorite (int idx) { mainP->toggleFavorite (idx); }
    void showStatus (const juce::String& msg) { mainP->showStatus (msg); }
    int visibleCount() const { return mainP->visibleCount(); }
    int shownPreset() const { return mainP->shownPreset(); }
    bool searchHasFocus() const { return mainP->searchHasFocus(); }
    juce::Component& searchBox() { return mainP->searchBox(); }
    vui::VaultKeyboard& keys() { return keyboard; }
    VaultBrowserModel& browser() { return mainP->browser(); }
    vui::PresetList& presetList() { return mainP->presetList(); }
    juce::File renderWavForDrag() { return mainP->renderWavForDrag(); }
    juce::String statusHint() const { return mainP->statusHint(); }
    void giveFocusBack() { mainP->giveFocusBack(); }
    int focusReturns() const { return mainP->focusReturns(); }

    void updateToggles();                               // FX / ARP / GATE toggles, size menu text
    void showScaleMenu();
    void showSettingsMenu();

    std::function<void (float)> onScaleChosen;
    std::function<float()> currentScale;

private:
    static int logoWidth();
    VaultProcessor& proc;
    std::shared_ptr<VaultSettings> settings;
    vui::UiContext ctx;
    std::unique_ptr<vui::MainPage> mainP;
    std::unique_ptr<vui::LayersPage> layersP;
    std::unique_ptr<vui::FxPage> fxP;
    std::unique_ptr<vui::ModPage> modP;
    std::unique_ptr<vui::ArpGatePage> arpP;
    std::array<vui::TabButton, numPages> tabs { vui::TabButton ("MAIN"), vui::TabButton ("LAYERS"), vui::TabButton ("FX"), vui::TabButton ("MOD"),
                                                vui::TabButton (juce::String (juce::CharPointer_UTF8 ("ARP \xc2\xb7 GATE"))) };
    vui::LedToggle fxT { "FX" }, arpT { "ARP" }, gateT { "GATE" };
    vui::Dropdown scaleBtn { "100%" };
    vui::GearButton gear;
    vui::VaultKeyboard keyboard;
    std::vector<std::unique_ptr<juce::ParameterAttachment>> switchAtts;
    int page = pageMain, ticks = 0, advLabelX = 0, sepX = 0;
    VaultPreset lastSound;                              // what the pages last showed (a change from elsewhere refreshes them)
    int lastStateSig = -1;                              // switches + locks
};
