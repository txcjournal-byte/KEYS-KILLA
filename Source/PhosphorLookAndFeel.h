// "Analog phosphor" look for Lucky Notes: near-black green ground, phosphor green, amber LCDs, TrapWin red.
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace lnui
{
namespace col
{
const juce::Colour bg     { 0xff050806 };
const juce::Colour panel  { 0xff0b110d };
const juce::Colour border { 0xff1f3a28 };
const juce::Colour green  { 0xff33ff66 };
const juce::Colour dim    { 0xff5f8f6c };
const juce::Colour text   { 0xffc8f5d2 };
const juce::Colour amber  { 0xffffb000 };
const juce::Colour lcdBg  { 0xff170f02 };
const juce::Colour red    { 0xffe5212e };
} // namespace col

juce::Font pixelFont (float height);            // Press Start 2P (labels, logo)
juce::Font lcdFont (float height);              // VT323 (numbers)
juce::Font monoFont (float height, bool bold = false); // IBM Plex Mono (small text)

// text with a soft phosphor glow (a few faint offset copies under the sharp text)
void drawGlowText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> area, juce::Justification j,
                   juce::Colour colour, const juce::Font& font, float glow = 1.0f);

// panel: dark fill, thin dark-green border, small pixel title
void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title);

// amber LCD window
void drawLcd (juce::Graphics& g, juce::Rectangle<float> r);

// pixel four-leaf clover in a cell grid, centred in r
void drawPixelClover (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour colour, bool glow);

// CRT scanlines + vignette over an area
void drawScanlines (juce::Graphics& g, juce::Rectangle<int> r);

// button styles (TextButton property "ln-style"): "seg" (default), "red", "outline"
class PhosphorLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PhosphorLookAndFeel();

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool isMouseOverButton, bool isButtonDown) override;
    juce::Font getTextButtonFont (juce::TextButton&, int buttonHeight) override;

    void drawTooltip (juce::Graphics&, const juce::String& text, int width, int height) override;
    juce::Rectangle<int> getTooltipBounds (const juce::String& tipText, juce::Point<int> screenPos, juce::Rectangle<int> parentArea) override;

    void drawPopupMenuBackground (juce::Graphics&, int width, int height) override;
};
} // namespace lnui
