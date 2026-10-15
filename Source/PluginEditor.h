#pragma once

#include "PhosphorLookAndFeel.h"
#include "PluginProcessor.h"

class LuckyNotesContent;

class LuckyNotesEditor : public juce::AudioProcessorEditor,
                         public juce::DragAndDropContainer,
                         private juce::Timer
{
public:
    static constexpr int kBaseWidth = 1000, kBaseHeight = 720;

    explicit LuckyNotesEditor (LuckyNotesProcessor&);
    ~LuckyNotesEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    // pulls everything from the processor now (the timer does it continuously; the snapshot tool calls it directly)
    void refreshNow();

private:
    void timerCallback() override;

    LuckyNotesProcessor& proc;
    lnui::PhosphorLookAndFeel lnf;
    std::unique_ptr<LuckyNotesContent> content;
    juce::TooltipWindow tooltips { this, 700 };
    int seenVersion = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LuckyNotesEditor)
};
