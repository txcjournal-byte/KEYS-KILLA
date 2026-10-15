#pragma once
#include "VaultKnob.h"

// CHROME VAULT v3 controls of the approved mockup: panels, section headers, pills, segmented switches, power buttons, the
// top-bar tabs and toggles. Nothing here takes the keyboard focus.
namespace vui
{
inline void noFocus (juce::Component& c) { c.setWantsKeyboardFocus (false); c.setMouseClickGrabsKeyboardFocus (false); }

// a panel: dark gradient, 1 px outline, rounded
inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 10.0f)
{
    g.setGradientFill (juce::ColourGradient (col::panA, 0, r.getY(), col::panB, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    g.setColour (col::line3);
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}

// small caps section header ("FILTER · AMP")
inline void drawHeader (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                        juce::Justification j = juce::Justification::centredLeft, juce::Colour c = col::dim)
{
    auto f = font (14.0f, true, 0.18f);
    if (textWidth (f, text) > r.getWidth()) f = font (14.0f, true, 0.06f);
    g.setFont (f);
    g.setColour (c);
    g.drawText (text, r, j, false);
}
inline float headerWidth (const juce::String& text) { return textWidth (font (14.0f, true, 0.18f), text); }

inline juce::String upDot() { return juce::String (juce::CharPointer_UTF8 (" \xc2\xb7 ")); }

//==============================================================================
// rounded pill button ("Favorites", "More like this", "Drag WAV", "Bypass all", "Keep while browsing").
// Toggle on = ice outline + ice text.
class Pill : public juce::Button
{
public:
    enum class Icon { none, heart, down, lock, plus };
    explicit Pill (const juce::String& t, Icon i = Icon::none, bool bold = false) : juce::Button (t), icon (i), strong (bold)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    juce::Font pillFont() const { return font (14.0f, strong); }
    int idealWidth() const
    {
        const auto t = getButtonText();
        return (int) std::ceil (textWidth (pillFont(), t) + (icon != Icon::none ? (t.isEmpty() ? 14.0f : 20.0f) : 0.0f) + 20.0f);
    }
    bool dimmed = false;         // shown, but not available (e.g. a full routing list)
    juce::Colour textColour = col::muted;

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        g.setColour (down ? col::rowHover : on ? col::segOn.withAlpha (0.55f) : juce::Colour (0xff0b111a));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (on ? col::ice2 : over && ! dimmed ? col::ice2.withAlpha (0.7f) : col::pillLine);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        const auto f = pillFont();
        const auto t = getButtonText();
        const float iw = icon == Icon::none ? 0.0f : 14.0f;
        const float w = textWidth (f, t) + (iw > 0 && t.isNotEmpty() ? iw + 6.0f : iw);
        float x = r.getCentreX() - w * 0.5f;
        const auto c = dimmed ? col::dim : on ? col::ice : (over ? col::text : textColour);
        if (icon == Icon::heart && t.isNotEmpty())
        {
            g.setColour (on ? col::ice : col::muted);
            g.fillPath (heartPath ({ x, r.getCentreY() - 6.5f, 13.0f, 13.0f }));
            x += iw + 6.0f;
        }
        else if (icon == Icon::lock)
        {
            drawLock (g, { x, r.getCentreY() - 7.0f, 12.0f, 14.0f }, on ? col::ice : col::dim);
            x += iw + 6.0f;
        }
        else if (icon == Icon::plus)
        {
            g.setColour (c);
            const float cx = x + 6.0f, cy = r.getCentreY();
            g.drawLine (cx - 5.0f, cy, cx + 5.0f, cy, 1.5f);
            g.drawLine (cx, cy - 5.0f, cx, cy + 5.0f, 1.5f);
            x += iw + 6.0f;
        }
        g.setColour (c);
        g.setFont (f);
        if (t.isNotEmpty()) g.drawText (t, juce::Rectangle<float> (x, r.getY(), r.getRight() - x, r.getHeight()), juce::Justification::centredLeft, false);
        if (icon == Icon::down) drawDownArrow (g, { x + textWidth (f, t) + 6.0f, r.getCentreY() - 6.0f, 9.0f, 12.0f }, c);
        if (icon == Icon::heart && t.isEmpty())
        {
            g.setColour (on ? col::ice : over ? col::muted : col::heartOff);
            g.fillPath (heartPath (juce::Rectangle<float> (14.0f, 14.0f).withCentre (r.getCentre())));
        }
    }
    static void drawLock (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c)
    {
        g.setColour (c);
        auto body = r.removeFromBottom (r.getHeight() * 0.55f);
        g.fillRoundedRectangle (body, 1.5f);
        juce::Path shackle;
        const float w = body.getWidth() * 0.62f;
        shackle.addCentredArc (body.getCentreX(), body.getY(), w * 0.5f, r.getHeight() * 0.85f, 0.0f, -juce::MathConstants<float>::halfPi,
                               juce::MathConstants<float>::halfPi, true);
        g.strokePath (shackle, juce::PathStrokeType (1.5f));
    }

private:
    Icon icon;
    bool strong;
};

//==============================================================================
// dropdown pill: "Pattern: Init ▾" / "Hall ▾" (the owner shows the menu in onClick)
class Dropdown : public juce::Button
{
public:
    explicit Dropdown (const juce::String& t = {}) : juce::Button (t)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    bool small = false;          // the compact header style of the FX slot cards
    int idealWidth() const { return (int) std::ceil (textWidth (font (14.0f, small), getButtonText()) + 30.0f); }
    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (down ? col::rowHover : juce::Colour (0xff0b111a));
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (over ? col::ice2.withAlpha (0.7f) : col::pillLine);
        g.drawRoundedRectangle (r, 5.0f, 1.0f);
        const auto f = font (14.0f, small);
        g.setFont (f);
        g.setColour (isEnabled() ? (over ? col::text : col::muted) : col::dim);
        auto t = r.reduced (9.0f, 0.0f);
        auto arrow = t.removeFromRight (9.0f);
        g.drawText (getButtonText(), t, juce::Justification::centredLeft, true);
        juce::Path p;
        const auto c = arrow.getCentre().translated (0.0f, 1.0f);
        p.addTriangle (c.x - 4.0f, c.y - 2.5f, c.x + 4.0f, c.y - 2.5f, c.x, c.y + 2.5f);
        g.fillPath (p);
    }
};

//==============================================================================
// segmented switch: "Up | Down | Up/Down ..." - one selected
class Segments : public juce::Component
{
public:
    std::function<void (int)> onSelect;
    explicit Segments (const juce::StringArray& o = {}, float fontPx = 14.0f) : options (o), px (fontPx)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void setOptions (const juce::StringArray& o) { options = o; repaint(); }
    void setSelected (int i) { if (i != selected) { selected = i; repaint(); } }
    int getSelected() const { return selected; }
    bool upper = false;          // SYNC | HZ style (bold caps)
    int padX = 11;
    int idealWidth() const
    {
        float w = 0;
        for (auto& o : options) w += textWidth (segFont(), o) + 2.0f * (float) padX;
        return (int) std::ceil (w) + 2;
    }
    void setEnabledLook (bool e) { if (e != enabledLook) { enabledLook = e; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colour (0xff0b111a));
        g.fillRoundedRectangle (r, 6.0f);
        const auto rects = segRects();
        for (int i = 0; i < options.size(); ++i)
        {
            const auto sr = rects[(size_t) i];
            const bool on = i == selected;
            if (on)
            {
                juce::Path p;
                p.addRoundedRectangle (sr.getX(), sr.getY(), sr.getWidth(), sr.getHeight(), 5.0f, 5.0f, i == 0, i == options.size() - 1, i == 0, i == options.size() - 1);
                g.setColour (enabledLook ? col::segOn : col::stepOff);
                g.fillPath (p);
            }
            g.setFont (segFont());
            g.setColour (! enabledLook ? col::dim : on ? col::silver : i == hover ? col::text : col::muted);
            g.drawText (options[i], sr, juce::Justification::centred, false);
        }
        g.setColour (col::pillLine);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
    }
    void mouseMove (const juce::MouseEvent& e) override { setHover (hit (e.position)); }
    void mouseExit (const juce::MouseEvent&) override { setHover (-1); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = hit (e.position);
        if (i >= 0 && enabledLook) { selected = i; repaint(); if (onSelect) onSelect (i); }
    }

private:
    juce::Font segFont() const { return font (px, upper, upper ? 0.04f : 0.0f); }
    std::vector<juce::Rectangle<float>> segRects() const
    {
        std::vector<juce::Rectangle<float>> out;
        float total = 0;
        for (auto& o : options) total += textWidth (segFont(), o) + 2.0f * (float) padX;
        const float extra = options.isEmpty() ? 0.0f : ((float) getWidth() - 1.0f - total) / (float) options.size();
        float x = 0.5f;
        for (auto& o : options)
        {
            const float w = textWidth (segFont(), o) + 2.0f * (float) padX + extra;
            out.push_back ({ x, 0.5f, w, (float) getHeight() - 1.0f });
            x += w;
        }
        return out;
    }
    int hit (juce::Point<float> p) const
    {
        const auto rs = segRects();
        for (int i = 0; i < (int) rs.size(); ++i) if (rs[(size_t) i].contains (p)) return i;
        return -1;
    }
    void setHover (int h) { if (h != hover) { hover = h; repaint(); } }

    juce::StringArray options;
    float px;
    int selected = 0, hover = -1;
    bool enabledLook = true;
};

//==============================================================================
// round power button: green ring + dot when on
class PowerButton : public juce::Button
{
public:
    PowerButton() : juce::Button ("Power")
    {
        noFocus (*this);
        setClickingTogglesState (false);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        const float d = std::min ((float) getWidth(), (float) getHeight()) - 2.0f;
        auto r = juce::Rectangle<float> (d, d).withCentre (getLocalBounds().toFloat().getCentre());
        const bool on = getToggleState();
        g.setColour (on ? col::ok : over ? col::ice2 : juce::Colour (0xff3a4660));
        g.drawEllipse (r.reduced (0.75f), 1.5f);
        if (on)
        {
            g.setColour (col::ok.withAlpha (0.25f));
            g.fillEllipse (r.reduced (d * 0.2f));
            g.setColour (col::ok);
            g.fillEllipse (r.reduced (d * 0.3f));
        }
    }
};

//==============================================================================
// top-bar page tab ("MAIN", "LAYERS" ...)
class TabButton : public juce::Button
{
public:
    explicit TabButton (const juce::String& t) : juce::Button (t) { noFocus (*this); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    static juce::Font tabFont() { return font (14.0f, true, 0.1f); }
    int idealWidth() const { return (int) std::ceil (textWidth (tabFont(), getButtonText()) + 22.0f); }
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat();
        const bool on = getToggleState();
        if (on)
        {
            g.setColour (col::tabOn);
            g.fillRoundedRectangle (r, 6.0f);
            g.setColour (col::ice);
            g.fillRoundedRectangle (r.getX() + 2.0f, r.getBottom() - 2.0f, r.getWidth() - 4.0f, 2.0f, 1.0f);
        }
        g.setFont (tabFont());
        g.setColour (on ? col::text : over ? col::muted : col::dim);
        g.drawText (getButtonText(), r, juce::Justification::centred, false);
    }
};

// top-bar quick toggle with an LED ("● FX")
class LedToggle : public juce::Button
{
public:
    explicit LedToggle (const juce::String& t) : juce::Button (t) { noFocus (*this); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    bool locked = false;         // "keep while browsing": a small lock after the text
    static juce::Font ledFont() { return font (14.0f, true, 0.08f); }
    int idealWidth() const { return (int) std::ceil (textWidth (ledFont(), getButtonText()) + 38.0f + (locked ? 16.0f : 0.0f)); }
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        const bool on = getToggleState();
        g.setColour (juce::Colour (0xff0d131c));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (over ? col::ice2.withAlpha (0.7f) : col::line3);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        auto led = juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ r.getX() + 14.0f, r.getCentreY() });
        if (on) { g.setColour (col::ok.withAlpha (0.3f)); g.fillEllipse (led.expanded (2.5f)); g.setColour (col::ok); }
        else g.setColour (col::ledOff);
        g.fillEllipse (led);
        g.setFont (ledFont());
        g.setColour (on ? col::text : col::muted);
        auto t = r.withTrimmedLeft (24.0f);
        if (locked) Pill::drawLock (g, t.removeFromRight (18.0f).withSizeKeepingCentre (10.0f, 12.0f), col::ice2);
        g.drawText (getButtonText(), t, juce::Justification::centredLeft, false);
    }
};

// small square icon button (settings gear)
class GearButton : public juce::Button
{
public:
    GearButton() : juce::Button ("Settings") { noFocus (*this); setMouseCursor (juce::MouseCursor::PointingHandCursor); }
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (juce::Colour (0xff0d131c));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (over ? col::ice2.withAlpha (0.7f) : col::line3);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        const auto c = r.getCentre();
        const float ro = 7.0f, ri = 5.0f;
        juce::Path p;
        for (int i = 0; i < 16; ++i)
        {
            const float a = (float) i / 16.0f * juce::MathConstants<float>::twoPi;
            const float rr = (i % 2 == 0) ? ro : ri;
            const auto pt = c.getPointOnCircumference (rr, a);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        g.setColour (over ? col::text : col::muted);
        g.fillPath (p);
        g.setColour (juce::Colour (0xff0d131c));
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (c));
    }
};
} // namespace vui
