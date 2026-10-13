#pragma once
#include "VaultWidgets.h"
#include "VaultThumbs.h"
#include "VaultSettings.h"
#include "../VaultProcessor.h"

// right column: preset name, category, tags, waveform thumbnail, layer stack, the 4 macro knobs and volume
class VaultInfoPanel : public juce::Component
{
public:
    VaultInfoPanel (VaultProcessor&, VaultBrowserModel&, VaultThumbs&);
    ~VaultInfoPanel() override;

    void setPreset (int bankIndex);
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;

    std::function<void (int bankIndex)> onToggleFavorite;
    vui::DragPill dragPill;
    juce::Slider knob[vault::numMacros], volume;

private:
    juce::Rectangle<float> heartRect() const;
    VaultProcessor& proc;
    VaultBrowserModel& model;
    VaultThumbs& thumbs;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> knobAtt[vault::numMacros], volumeAtt;
    int preset = -1;
    bool heartOver = false;
    // fixed layout (base coordinates) - nothing moves while browsing
    static constexpr int volumeLabelW = 78, volumeValueW = 70;
    juce::Rectangle<int> nameR, catR, tagsR, waveR, layersLabelR, stackR, knobsR, volumeR;
};

// the whole UI at the base size 1180 x 760 (VaultEditor scales it as one piece)
class VaultMainView : public juce::Component, private juce::ChangeListener
{
public:
    static constexpr int topH = 66, statusH = 36, keysH = 112, catW = 210, infoW = 320;

    VaultMainView (VaultProcessor&, std::shared_ptr<VaultSettings>);
    ~VaultMainView() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // browsing (all message thread)
    void loadPreset (int bankIndex, bool audition);     // select in the processor + recent + optional preview note
    void step (int delta, bool wrap);                   // through the CURRENT filtered list
    void stepToEnd (bool last);                         // first / last sound of the filtered list (Home / End)
    int pageRows() const;                               // rows per page of the list (Page Up / Page Down)
    void randomSound();
    void auditionCurrent();
    void syncFromProcessor();                           // follows host automation / state restore (timer)
    void setCategory (int c);                           // -1 = All Sounds
    void setQuery (const juce::String& q);              // also updates the search box
    void setStyle (int s);
    void setCharacter (int c);
    void setLayers (int n);
    void setFavoritesOnly (bool);
    void setRecentOnly (bool);
    void toggleFavorite (int bankIndex);
    void showStatus (const juce::String& msg);
    int visibleCount() const { return (int) model.visible().size(); }
    int shownPreset() const { return shown; }
    bool searchHasFocus() const { return search.hasKeyboardFocus (true); }
    juce::Component& searchBox() { return search; }
    vui::VaultKeyboard& keys() { return keyboard; }
    VaultThumbs& thumbnails() { return thumbs; }
    VaultBrowserModel& browser() { return model; }
    vui::PresetList& presetList() { return list; }
    VaultInfoPanel& infoPanel() { return info; }
    juce::File renderWavForDrag();                      // the current sound (C4; 808 & Bass C3), 2 s, with the macros, as a WAV in the temp folder

    void updateToggles();                               // RECENT / FAVORITES / preview / scale pills follow the state
    void giveFocusBack();                               // keyboard focus back to the editor (arrows, PC keys)

    std::function<void (float)> onScaleChosen;
    std::function<float()> currentScale;

private:
    struct SearchKeys : juce::KeyListener
    {
        VaultMainView& v;
        explicit SearchKeys (VaultMainView& o) : v (o) {}
        bool keyPressed (const juce::KeyPress& k, juce::Component*) override
        {
            if (k == juce::KeyPress::downKey) { v.step (1, false); return true; }
            if (k == juce::KeyPress::upKey)   { v.step (-1, false); return true; }
            return false;
        }
    } searchKeys { *this };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void filterChanged (bool scrollToCurrent = true, bool keepScroll = false);
    void setShown (int bankIndex);
    void showScaleMenu();
    static int logoWidth();
    juce::Rectangle<int> searchRect() const, curRect() const, statusRect() const, keysRect() const;

    VaultProcessor& proc;
    std::shared_ptr<VaultSettings> settings;
    VaultBrowserModel model;
    VaultThumbs thumbs;

    juce::TextEditor search;
    vui::ArrowButton prevBtn { false }, nextBtn { true };
    vui::PillButton randomBtn { "RANDOM", vui::PillButton::Icon::dice }, recentBtn { "RECENT", vui::PillButton::Icon::clock },
                    favBtn { "FAVORITES", vui::PillButton::Icon::heart };
    vui::CategoryList categories;
    vui::ChipBar chips;
    vui::PresetList list;
    VaultInfoPanel info;
    vui::PillButton previewBtn { "Preview on click", vui::PillButton::Icon::none, 14.0f }, scaleBtn { "100 %", vui::PillButton::Icon::none, 14.0f };
    vui::VaultKeyboard keyboard;

    int shown = -2;
    juce::String statusMsg;
    juce::uint32 statusUntil = 0;
    juce::Random rng;
};
