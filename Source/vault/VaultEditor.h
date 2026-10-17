#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "VaultProcessor.h"
#include "ui/VaultTheme.h"
#include "ui/VaultComputerKeys.h"
#include <array>

class VaultMainView;
class VaultSettings;

// CHROME VAULT editor (v3): MAIN (browser screen, quick knobs, sound macros) + the ADVANCED pages LAYERS / FX / MOD /
// ARP · GATE, the top bar and the keyboard (VaultMainView, Source/vault/ui).
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
    // Keyboard: the editor takes the keyboard when the user clicks into it (never on open: until then the DAW keeps its
    // own computer-keyboard piano), and then the computer keyboard plays notes (vui::ComputerKeys, FL Studio's layout:
    // Z = C4, Q = C5, Left / Right = octave, Up / Down = previous / next sound). No child keeps the keyboard: after any
    // click it is the editor's again. Only a click INTO the search box types there; Return, Escape or a click anywhere
    // else hands the keyboard back to the editor (and the keys play again).
    void mouseDown (const juce::MouseEvent&) override;   // (also from all children)
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;
    void focusLost (FocusChangeType) override;
    void focusGained (FocusChangeType) override;
    vui::ComputerKeys& computerKeys() { return keys; }
    void takeKeyboard();                                  // the editor (not a child) gets the keyboard, if it is showing
    int hostKeyDown = 0;                                  // Windows key code passed on to the DAW and not yet released

    void setUiScale (float scale);                       // clamps to 80 % .. 200 %, resizes the window and remembers it
    float getUiScale() const { return uiScale; }
    juce::Rectangle<int> displayArea() const;            // user area of the display showing the editor, else the main display

    // test / snapshot hooks
    VaultMainView& view() { return *main; }
    void checkDisplay() { fitToDisplay(); }

private:
    void timerCallback() override;
    void fitToDisplay();                                 // once shown: re-check the size against the display it is really on
    void applyScale (float scale);                       // resize without remembering (first-open / display fit)
    void limitToDisplay (juce::Rectangle<int> area);     // the corner drag's maximum = the screen

    VaultProcessor& proc;
    vui::ComputerKeys keys { proc.getKeyboardState() };
    vui::VaultLookAndFeel lnf;
    std::shared_ptr<VaultSettings> settings;
    std::unique_ptr<VaultMainView> main;
    float uiScale = 1.0f;
    bool settingSize = false;
    bool displayChecked = false;
    bool keysHintShown = false;
    int displayTicks = 0;
    bool clickedIn = false;                              // the user clicked into the plugin (from then on it keeps the keys)
    juce::Rectangle<int> lastArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VaultEditor)
};
