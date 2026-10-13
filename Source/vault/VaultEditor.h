#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "VaultProcessor.h"
#include "ui/VaultTheme.h"
#include <array>

class VaultMainView;
class VaultSettings;

// CHROME VAULT editor: the preset browser (search, categories, filter chips, list, info panel with macros, keyboard).
// Everything is laid out at the base size 1180 x 760 and scaled as one piece (70 % .. 175 %, aspect kept, remembered).
// 70 % = 826 x 532 (small laptop next to a DAW), 175 % = 2065 x 1330 (4K screens).
class VaultEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    static constexpr int baseWidth = 1180, baseHeight = 760;
    static constexpr float minScale = 0.7f, maxScale = 1.75f;
    static const std::vector<float>& scalePresets();   // 70 80 90 100 115 125 150 175 %
    static float firstOpenScale (juce::Rectangle<int> usableArea);   // largest preset <= 100 % that fits 85 % of the area

    explicit VaultEditor (VaultProcessor&);
    ~VaultEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    bool keyStateChanged (bool isKeyDown) override;
    void focusLost (FocusChangeType) override;
    void mouseDown (const juce::MouseEvent&) override;   // (also from all children) - clicks outside the search box return focus here
    void visibilityChanged() override;

    void setUiScale (float scale);                       // clamps to 70 % .. 175 %, resizes the window and remembers it
    float getUiScale() const { return uiScale; }

    // test / snapshot hooks
    VaultMainView& view() { return *main; }

private:
    void timerCallback() override;
    void releasePcNotes();

    VaultProcessor& proc;
    vui::VaultLookAndFeel lnf;
    std::shared_ptr<VaultSettings> settings;
    std::unique_ptr<VaultMainView> main;
    float uiScale = 1.0f;
    bool settingSize = false;
    // PC keyboard -> notes ("A W S E D F T G Y H U J K O L P ;" from C, Z / X = octave down / up)
    int pcOctaveBase = 60;
    std::array<int, 17> pcNoteDown;   // note playing per key (-1 = up)
    bool focusGrabbed = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VaultEditor)
};
