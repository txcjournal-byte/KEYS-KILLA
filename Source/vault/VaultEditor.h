#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "VaultProcessor.h"

// PLACEHOLDER editor for CHROME VAULT (the real browser UI replaces this file pair later).
// Keep the class name and constructor: VaultEditor (VaultProcessor&).
class VaultEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VaultEditor (VaultProcessor&);
    ~VaultEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void step (int delta);

    VaultProcessor& proc;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::Slider macro[vault::numMacros], volume;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> macroAtt[vault::numMacros], volumeAtt;
    juce::MidiKeyboardComponent keyboard;
    int shownPreset = -2;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VaultEditor)
};
