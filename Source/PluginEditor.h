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
    void paint (juce::Graphics&) override;
    void parentHierarchyChanged() override;
    void showView (int v);   // 0 main, 1..8 advanced tabs, 9 preset browser

    static constexpr int designW = 1672, designH = 941;   // = BREED LAB design size
    int fitScale() const;   // largest size in % that fits the screen under the host's toolbars
    // v0.34: the plugin sits in a thin metal case (frame) - it can be switched off in MENU
    int frame() const { return frameOn ? 26 : 0; }
    int outerW() const { return designW + 2 * frame(); }
    int outerH() const { return designH + 2 * frame(); }
    void setScalePct (int pct);
    void setFrame (bool on);
    void themeChanged() { frameImg = {}; repaint(); }
    // v0.35 PAIR FROM VST: the plugin's own window opens INSIDE BREED LAB, fitted so all of it is visible
    // (scaled down when the plugin can, or the BREED LAB window grows for it). false = it cannot fit: use a separate window.
    bool showHostedEditor (juce::AudioPluginInstance& inst, const juce::String& title, std::function<void()> onTake, std::function<void()> onClosed);
    void closeHostedEditor();
    bool hostedEditorOpen() const { return hostedPanel != nullptr; }

private:
    KeysKillaProcessor& proc;
    std::unique_ptr<MainPage> page;
    juce::TooltipWindow tooltips { this, 600 };
    bool fitted = false, frameOn = true;
    int lastPct = 85;
    juce::Image frameImg;   // the case, drawn once per theme / size (released with the editor)
    std::unique_ptr<juce::Component> hostedPanel;
    int pctBeforeHosted = -1;
};
