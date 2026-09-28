#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"
#include "Skin.h"

class KKLookAndFeel : public juce::LookAndFeel_V4
{
public:
    const Skin* skin = &Skin::all()[0];
    void setSkin (const Skin& s);
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool over, bool down) override;
    juce::Font getTextButtonFont (juce::TextButton&, int h) override;
    juce::Font getLabelFont (juce::Label&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
};

// Main page content, laid out at a fixed design size and scaled as a whole.
class MainPage;

class KeysKillaEditor : public juce::AudioProcessorEditor
{
public:
    explicit KeysKillaEditor (KeysKillaProcessor&);
    ~KeysKillaEditor() override;
    void resized() override;
    void paint (juce::Graphics&) override {}
    void parentHierarchyChanged() override;
    void showView (int v);   // 0 main, 1..8 advanced tabs, 9 preset browser

    static constexpr int designW = 1586, designH = 992;   // = design mockup size

private:
    KeysKillaProcessor& proc;
    std::unique_ptr<MainPage> page;
    juce::TooltipWindow tooltips { this, 600 };
};
