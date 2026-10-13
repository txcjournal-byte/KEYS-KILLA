#include "VaultEditor.h"

namespace
{
const juce::Colour ink (0xff04060a), panel (0xff0a0f18), panel2 (0xff0b111c), line (0xff1a2333), silver (0xffe6eaf2),
                   text (0xffe6ecf7), muted (0xffa9b6cc), dim (0xff6b7890), ice (0xff9fc3ff), ice2 (0xff7fa6e8), deep (0xff4676c4);
}

VaultEditor::VaultEditor (VaultProcessor& p)
    : AudioProcessorEditor (p), proc (p), keyboard (p.getKeyboardState(), juce::MidiKeyboardComponent::horizontalKeyboard)
{
    for (auto* b : { &prevButton, &nextButton })
    {
        b->setColour (juce::TextButton::buttonColourId, panel2);
        b->setColour (juce::TextButton::textColourOffId, text);
        b->setColour (juce::ComboBox::outlineColourId, line);
        addAndMakeVisible (*b);
    }
    prevButton.onClick = [this] { step (-1); };
    nextButton.onClick = [this] { step (1); };
    auto style = [] (juce::Slider& s)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        s.setColour (juce::Slider::rotarySliderFillColourId, ice);
        s.setColour (juce::Slider::rotarySliderOutlineColourId, line);
        s.setColour (juce::Slider::thumbColourId, silver);
        s.setColour (juce::Slider::textBoxTextColourId, muted);
        s.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    };
    for (int m = 0; m < vault::numMacros; ++m)
    {
        style (macro[m]);
        addAndMakeVisible (macro[m]);
        macroAtt[m] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, VaultProcessor::idMacro[m], macro[m]);
    }
    style (volume);
    addAndMakeVisible (volume);
    volumeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, VaultProcessor::idVolume, volume);
    keyboard.setColour (juce::MidiKeyboardComponent::whiteNoteColourId, silver);
    keyboard.setColour (juce::MidiKeyboardComponent::blackNoteColourId, panel);
    keyboard.setColour (juce::MidiKeyboardComponent::keyDownOverlayColourId, ice2);
    keyboard.setLowestVisibleKey (36);
    addAndMakeVisible (keyboard);
    setSize (900, 560);
    startTimerHz (15);
}

VaultEditor::~VaultEditor() { stopTimer(); }

void VaultEditor::step (int delta)
{
    const int n = (int) proc.bank().size();
    if (n == 0) return;
    proc.selectPreset (((proc.currentPreset() + delta) % n + n) % n);
    repaint();
}

void VaultEditor::timerCallback()
{
    if (proc.currentPreset() != shownPreset) { shownPreset = proc.currentPreset(); repaint(); }
}

void VaultEditor::paint (juce::Graphics& g)
{
    g.fillAll (ink);
    auto r = getLocalBounds().reduced (24);
    auto top = r.removeFromTop (70);
    {   // chrome logo
        juce::ColourGradient chrome (juce::Colours::white, 0.0f, (float) top.getY(), deep, 0.0f, (float) top.getY() + 46.0f, false);
        chrome.addColour (0.45, silver); chrome.addColour (0.55, ice2);
        g.setGradientFill (chrome);
        g.setFont (juce::Font (juce::FontOptions (40.0f, juce::Font::bold)));
        g.drawText ("CHROME VAULT", top.removeFromTop (46), juce::Justification::centredLeft);
        g.setColour (dim);
        g.setFont (juce::Font (juce::FontOptions (12.0f)));
        g.drawText ("BY TRAPWIN   |   " + proc.bankDescription(), top, juce::Justification::centredLeft);
    }
    auto card = r.removeFromTop (110).toFloat();
    g.setColour (panel);
    g.fillRoundedRectangle (card, 16.0f);
    g.setColour (line);
    g.drawRoundedRectangle (card, 16.0f, 1.0f);
    if (const auto* p = proc.currentPresetData())
    {
        auto t = card.reduced (70.0f, 14.0f).toNearestInt();
        g.setColour (ice);
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        g.drawText (vault::categoryNames()[p->category].toUpperCase() + "   " + juce::String (p->parts.size()) + (p->parts.size() == 1 ? " LAYER" : " LAYERS"),
                    t.removeFromTop (22), juce::Justification::centred);
        g.setColour (text);
        g.setFont (juce::Font (juce::FontOptions (30.0f, juce::Font::bold)));
        g.drawText (p->name, t.removeFromTop (40), juce::Justification::centred);
        g.setColour (muted);
        g.setFont (juce::Font (juce::FontOptions (13.0f)));
        auto tags = vault::namesOfMask (p->styles, vault::styleNames());
        tags.addArray (vault::namesOfMask (p->chars, vault::characterNames()));
        g.drawText (tags.joinIntoString ("  /  "), t, juce::Justification::centred);
    }
    g.setColour (dim);
    g.setFont (juce::Font (juce::FontOptions (12.0f)));
    auto labels = getLocalBounds().reduced (24).withTrimmedTop (184).removeFromTop (20);
    const int w = labels.getWidth() / 5;
    for (int m = 0; m < vault::numMacros; ++m) g.drawText (vault::macroNames()[m], labels.removeFromLeft (w), juce::Justification::centred);
    g.drawText ("VOLUME", labels.removeFromLeft (w), juce::Justification::centred);
}

void VaultEditor::resized()
{
    auto r = getLocalBounds().reduced (24);
    r.removeFromTop (70);
    auto card = r.removeFromTop (110);
    prevButton.setBounds (card.removeFromLeft (56).reduced (8, 30));
    nextButton.setBounds (card.removeFromRight (56).reduced (8, 30));
    r.removeFromTop (26);
    auto knobs = r.removeFromTop (150);
    const int w = knobs.getWidth() / 5;
    for (auto& m : macro) m.setBounds (knobs.removeFromLeft (w).reduced (14, 0));
    volume.setBounds (knobs.removeFromLeft (w).reduced (14, 0));
    r.removeFromTop (16);
    keyboard.setBounds (r);
}
