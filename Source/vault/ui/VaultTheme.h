#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// CHROME VAULT UI theme: the "chrome night" palette of the approved mockup, fonts, small vector icons and the LookAndFeel
// (chrome knobs, slim scrollbars, popup menus). Everything is vector drawing - crisp at every scale, no bitmaps.
namespace vui
{
namespace col
{
inline const juce::Colour ink       { 0xff0b0f16 };   // base background (never pure black)
inline const juce::Colour panel     { 0xff111722 };
inline const juce::Colour panel2    { 0xff151c28 };
inline const juce::Colour line      { 0xff222b3a };
inline const juce::Colour silver    { 0xffe6eaf2 };
inline const juce::Colour text      { 0xffe3e8f0 };   // primary text (never pure white)
inline const juce::Colour muted     { 0xffa9b6cc };   // secondary text
inline const juce::Colour dim       { 0xff8593ab };   // tertiary text - the darkest colour used for ANY text
inline const juce::Colour ice       { 0xff9fc3ff };   // accent: selection, focus, active chips only
inline const juce::Colour ice2      { 0xff7fa6e8 };
inline const juce::Colour deep      { 0xff4676c4 };
inline const juce::Colour field     { 0xff05080d };   // inputs, waveform, keyboard bed
inline const juce::Colour fieldLine { 0xff243049 };
inline const juce::Colour btn       { 0xff0b111c };
inline const juce::Colour btnLine   { 0xff2a3548 };
inline const juce::Colour rowLine   { 0xff0f1623 };
inline const juce::Colour rowHover  { 0xff0c1320 };
inline const juce::Colour catHover  { 0xff0f1726 };
inline const juce::Colour layerBg   { 0xff0e1522 };
inline const juce::Colour layerLine { 0xff1c2638 };
inline const juce::Colour heartOff  { 0xff3a4660 };   // icon only, never text
inline const juce::Colour topA      { 0xff121a28 }, topB { 0xff0a0f18 };
inline const juce::Colour infoA     { 0xff0b111c }, infoB { 0xff070b12 };
inline const juce::Colour plugA     { 0xff0e1522 }, plugB { 0xff06090f };
} // namespace col

// Readability: the smallest text of the base layout is 14 px -> 11.2 px at the smallest window scale (80 %), 14 px at 100 %.
inline constexpr float minTextPx = 14.0f;
// test hook: the smallest font size asked for since the last reset (VaultTests checks it after painting at 80 / 100 %)
inline float& smallestFontPx() { static float s = 1000.0f; return s; }

// font size in CSS-like pixels (point height) - the mockup's sizes map 1:1
inline juce::Font font (float px, bool bold = false, float tracking = 0.0f)
{
    jassert (px >= minTextPx);
    smallestFontPx() = std::min (smallestFontPx(), px);
    auto o = juce::FontOptions (px, bold ? juce::Font::bold : juce::Font::plain).withPointHeight (px);
    if (tracking != 0.0f) o = o.withKerningFactor (tracking);
    return juce::Font (o);
}
inline float textWidth (const juce::Font& f, const juce::String& s) { return juce::GlyphArrangement::getStringWidth (f, s); }

// "5,000"
inline juce::String thousands (int n)
{
    juce::String s (std::abs (n)), out;
    for (int i = 0; i < s.length(); ++i)
    {
        if (i > 0 && (s.length() - i) % 3 == 0) out << ",";
        out << s[i];
    }
    return n < 0 ? "-" + out : out;
}

inline juce::String midDot() { return juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")); }   // " · "

//==============================================================================
// icons
inline juce::Path heartPath (juce::Rectangle<float> r)
{
    const float s = std::min (r.getWidth(), r.getHeight());
    const auto c = r.getCentre();
    const float x = c.x - s * 0.5f, y = c.y - s * 0.46f;
    juce::Path p;
    p.startNewSubPath (x + s * 0.5f, y + s * 0.92f);
    p.cubicTo (x + s * 0.12f, y + s * 0.64f, x - s * 0.02f, y + s * 0.42f, x + s * 0.06f, y + s * 0.2f);
    p.cubicTo (x + s * 0.14f, y + s * 0.0f,  x + s * 0.42f, y - s * 0.02f, x + s * 0.5f,  y + s * 0.2f);
    p.cubicTo (x + s * 0.58f, y - s * 0.02f, x + s * 0.86f, y + s * 0.0f,  x + s * 0.94f, y + s * 0.2f);
    p.cubicTo (x + s * 1.02f, y + s * 0.42f, x + s * 0.88f, y + s * 0.64f, x + s * 0.5f,  y + s * 0.92f);
    p.closeSubPath();
    return p;
}
inline juce::Path trianglePath (juce::Rectangle<float> r, bool pointsRight)
{
    juce::Path p;
    if (pointsRight) p.addTriangle (r.getX(), r.getY(), r.getRight(), r.getCentreY(), r.getX(), r.getBottom());
    else             p.addTriangle (r.getRight(), r.getY(), r.getX(), r.getCentreY(), r.getRight(), r.getBottom());
    return p;
}
inline void drawSearchIcon (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    const float s = std::min (r.getWidth(), r.getHeight());
    const auto ring = juce::Rectangle<float> (s * 0.66f, s * 0.66f).withPosition (r.getX(), r.getCentreY() - s * 0.42f);
    g.setColour (c);
    g.drawEllipse (ring.reduced (0.75f), 1.6f);
    const auto a = ring.getCentre() + juce::Point<float> (s * 0.24f, s * 0.24f);
    g.drawLine (a.x, a.y, a.x + s * 0.26f, a.y + s * 0.26f, 1.8f);
}
inline void drawDice (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);
    g.drawRoundedRectangle (r.reduced (0.8f), r.getWidth() * 0.22f, 1.4f);
    const float d = r.getWidth() * 0.16f;
    for (auto p : { juce::Point<float> (0.3f, 0.3f), { 0.7f, 0.7f }, { 0.5f, 0.5f }, { 0.7f, 0.3f }, { 0.3f, 0.7f } })
        g.fillEllipse (r.getX() + r.getWidth() * p.x - d * 0.5f, r.getY() + r.getHeight() * p.y - d * 0.5f, d, d);
}
inline void drawClock (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);
    g.drawEllipse (r.reduced (0.8f), 1.4f);
    const auto m = r.getCentre();
    g.drawLine (m.x, m.y, m.x, r.getY() + r.getHeight() * 0.25f, 1.4f);
    g.drawLine (m.x, m.y, r.getX() + r.getWidth() * 0.7f, m.y, 1.4f);
}
inline void drawDownArrow (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
{
    g.setColour (c);
    const float cx = r.getCentreX();
    g.drawLine (cx, r.getY(), cx, r.getBottom() - 1.0f, 1.6f);
    juce::Path p;
    p.startNewSubPath (r.getX() + 1.0f, r.getBottom() - r.getHeight() * 0.42f);
    p.lineTo (cx, r.getBottom());
    p.lineTo (r.getRight() - 1.0f, r.getBottom() - r.getHeight() * 0.42f);
    g.strokePath (p, juce::PathStrokeType (1.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

// chrome text gradient of the logo (white -> silver -> steel -> silver)
inline juce::ColourGradient chromeGradient (float top, float bottom)
{
    juce::ColourGradient g (juce::Colours::white, 0.0f, top, juce::Colour (0xffe6eaf2), 0.0f, bottom, false);
    g.addColour (0.45, juce::Colour (0xffc9d2e2));
    g.addColour (0.52, juce::Colour (0xff7d8ba3));
    return g;
}

//==============================================================================
class VaultLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VaultLookAndFeel()
    {
        using namespace juce;
        setColour (ResizableWindow::backgroundColourId, col::ink);
        setColour (TextEditor::backgroundColourId, Colours::transparentBlack);
        setColour (TextEditor::textColourId, col::text);
        setColour (TextEditor::highlightColourId, col::deep.withAlpha (0.55f));
        setColour (TextEditor::highlightedTextColourId, col::silver);
        setColour (TextEditor::outlineColourId, Colours::transparentBlack);
        setColour (TextEditor::focusedOutlineColourId, Colours::transparentBlack);
        setColour (CaretComponent::caretColourId, col::ice);
        setColour (PopupMenu::backgroundColourId, col::panel);
        setColour (PopupMenu::textColourId, col::text);
        setColour (PopupMenu::highlightedBackgroundColourId, col::deep.withAlpha (0.5f));
        setColour (PopupMenu::highlightedTextColourId, col::silver);
        setColour (ListBox::backgroundColourId, Colours::transparentBlack);
        setColour (ListBox::outlineColourId, Colours::transparentBlack);
        setColour (ScrollBar::thumbColourId, col::fieldLine);
        setColour (Slider::rotarySliderFillColourId, col::ice);
        setColour (Slider::rotarySliderOutlineColourId, col::line);
    }

    juce::Font getPopupMenuFont() override { return font (15.0f); }

    void drawPopupMenuBackground (juce::Graphics& g, int w, int h) override
    {
        g.fillAll (col::panel);
        g.setColour (col::btnLine);
        g.drawRect (0, 0, w, h);
    }

    void drawScrollbar (juce::Graphics& g, juce::ScrollBar&, int x, int y, int width, int height, bool vertical,
                        int thumbStart, int thumbSize, bool isMouseOver, bool isMouseDown) override
    {
        if (thumbSize <= 0) return;
        juce::Rectangle<float> t = vertical ? juce::Rectangle<float> ((float) x, (float) thumbStart, (float) width, (float) thumbSize)
                                            : juce::Rectangle<float> ((float) thumbStart, (float) y, (float) thumbSize, (float) height);
        t = vertical ? t.reduced ((float) width * 0.3f, 2.0f) : t.reduced (2.0f, (float) height * 0.3f);
        g.setColour (isMouseDown ? col::ice2 : isMouseOver ? col::btnLine.brighter (0.4f) : col::btnLine);
        g.fillRoundedRectangle (t, std::min (t.getWidth(), t.getHeight()) * 0.5f);
    }
    int getDefaultScrollbarWidth() override { return 10; }

    void drawCornerResizer (juce::Graphics& g, int w, int h, bool isMouseOver, bool isMouseDragging) override
    {
        g.setColour ((isMouseOver || isMouseDragging) ? col::ice2 : col::btnLine);
        const float s = (float) std::min (w, h);
        for (float i = 0.25f; i < 1.0f; i += 0.3f)
            g.drawLine ((float) w - s * i, (float) h - 1.5f, (float) w - 1.5f, (float) h - s * i, 1.3f);
    }

    // big chrome macro knob; the slider's name is drawn under it (its value while the mouse is on it)
    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float pos, float startAngle, float endAngle,
                           juce::Slider& s) override
    {
        using namespace juce;
        const float labelH = 20.0f;
        auto area = Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        auto label = area.removeFromBottom (labelH);
        const float d = std::min (area.getWidth(), area.getHeight()) - 10.0f;
        auto knob = Rectangle<float> (d, d).withCentre (area.getCentre());
        const auto c = knob.getCentre();

        // value ring: bipolar around 12 o'clock (0.5 = as designed)
        const float ringR = d * 0.5f + 5.0f;
        Path track; track.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, startAngle, endAngle, true);
        g.setColour (col::line);
        g.strokePath (track, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));
        const float mid = (startAngle + endAngle) * 0.5f, ang = startAngle + pos * (endAngle - startAngle);
        if (std::abs (ang - mid) > 0.01f)
        {
            Path v; v.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, std::min (mid, ang), std::max (mid, ang), true);
            g.setColour (s.isEnabled() ? col::ice : col::dim);
            g.strokePath (v, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }

        // body: dark rim, then the chrome dome
        g.setColour (col::btnLine);
        g.fillEllipse (knob.expanded (1.5f));
        g.setColour (Colour (0xff0a0f18));
        g.fillEllipse (knob.expanded (0.5f));
        auto dome = knob.reduced (2.5f);
        ColourGradient chrome (Colour (0xfff4f7fc), dome.getX() + dome.getWidth() * 0.35f, dome.getY() + dome.getHeight() * 0.3f,
                               Colour (0xff1a2130), dome.getRight(), dome.getBottom(), true);
        chrome.addColour (0.45, Colour (0xff9aa6bb));
        chrome.addColour (0.75, Colour (0xff3b4558));
        g.setGradientFill (chrome);
        g.fillEllipse (dome);
        if (s.isMouseOverOrDragging())
        {
            g.setColour (col::ice.withAlpha (0.35f));
            g.drawEllipse (knob.expanded (1.0f), 1.0f);
        }
        // pointer
        const float r0 = dome.getWidth() * 0.12f, r1 = dome.getWidth() * 0.42f;
        const Point<float> a = c.getPointOnCircumference (r0, ang), b = c.getPointOnCircumference (r1, ang);
        g.setColour (col::ink);
        g.drawLine ({ a, b }, 2.6f);

        // label / value
        const bool showValue = s.isMouseButtonDown() || s.isMouseOverOrDragging();
        g.setColour (showValue ? col::ice : col::dim);
        g.setFont (font (14.0f, true, 0.04f));
        const auto txt = showValue ? String (roundToInt (s.getValue() * 100.0)) + " %" : s.getName();
        g.drawText (txt, label, Justification::centred, false);
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        using namespace juce;
        auto r = Rectangle<float> ((float) x, (float) y, (float) width, (float) height);
        auto track = r.withSizeKeepingCentre (r.getWidth(), 6.0f);
        g.setColour (col::field);
        g.fillRoundedRectangle (track, 3.0f);
        g.setColour (col::fieldLine);
        g.drawRoundedRectangle (track, 3.0f, 1.0f);
        auto fill = track.withRight (sliderPos);
        g.setGradientFill (ColourGradient (col::deep, track.getX(), 0, col::ice, track.getRight(), 0, false));
        g.fillRoundedRectangle (fill, 3.0f);
        const float d = 16.0f;
        auto thumb = Rectangle<float> (d, d).withCentre ({ sliderPos, r.getCentreY() });
        ColourGradient chrome (Colour (0xfff4f7fc), thumb.getX() + d * 0.35f, thumb.getY() + d * 0.3f, Colour (0xff3b4558), thumb.getRight(), thumb.getBottom(), true);
        chrome.addColour (0.5, Colour (0xff9aa6bb));
        g.setColour (Colour (0xff0a0f18));
        g.fillEllipse (thumb.expanded (1.5f));
        g.setGradientFill (chrome);
        g.fillEllipse (thumb);
        if (s.isMouseOverOrDragging()) { g.setColour (col::ice.withAlpha (0.5f)); g.drawEllipse (thumb.expanded (1.0f), 1.0f); }
    }
};
} // namespace vui
