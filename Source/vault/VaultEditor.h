#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "VaultProcessor.h"
#include "ui/VaultTheme.h"
#include <array>

class VaultMainView;
class VaultSettings;

// CHROME VAULT editor: the preset browser (search, categories, filter chips, list, info panel with macros, keyboard).
// Everything is laid out at the base size 1180 x 760 (= 100 %) and scaled as one piece (80 % .. 200 %, aspect kept).
// 80 % = 944 x 608 (1366 x 768 laptop next to a DAW), 200 % = 2360 x 1520.
// Size on open: first time the largest preset <= 100 % that fits 85 % of the screen; later the remembered scale, shrunk
// (not forgotten) when the screen it opens on is too small for it.
class VaultEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int baseWidth = 1180, baseHeight = 760;
    static constexpr float minScale = 0.8f, maxScale = 2.0f;
    static const std::vector<float>& scalePresets();   // 80 90 100 115 130 150 175 200 %
    static bool fits (float scale, juce::Rectangle<int> area);                // the window at this scale fits inside the area
    static float firstOpenScale (juce::Rectangle<int> usableArea);          // largest preset <= 100 % that fits 85 % of the area
    static float restoredScale (float saved, juce::Rectangle<int> usableArea);// saved, or the largest smaller preset that fits
    // tests: pretend to be on a display with this user area (empty = the real displays)
    static juce::Rectangle<int>& simulatedDisplay() { static juce::Rectangle<int> r; return r; }

    explicit VaultEditor (VaultProcessor&);
    ~VaultEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    // Keyboard: nothing in the editor takes the keyboard except the search box (only while typing in it). Keys are never
    // used by the editor itself, so the DAW keeps its computer-keyboard piano / shortcuts whenever the search box is not
    // being typed in. A click anywhere else, Return or Escape hands the keyboard back.
    void mouseDown (const juce::MouseEvent&) override;   // (also from all children)

    void setUiScale (float scale);                       // clamps to 80 % .. 200 %, resizes the window and remembers it
    float getUiScale() const { return uiScale; }
    juce::Rectangle<int> displayArea() const;            // user area of the display showing the editor, else the main display

    // test / snapshot hooks
    VaultMainView& view() { return *main; }

private:
    void timerCallback() override;
    void fitToDisplay();                                 // once shown: re-check the size against the display it is really on
    void applyScale (float scale);                       // resize without remembering (first-open / display fit)

    VaultProcessor& proc;
    vui::VaultLookAndFeel lnf;
    std::shared_ptr<VaultSettings> settings;
    std::unique_ptr<VaultMainView> main;
    float uiScale = 1.0f;
    bool settingSize = false;
    bool displayChecked = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VaultEditor)
};
