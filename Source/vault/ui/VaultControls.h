#pragma once
#include "VaultKnob.h"

// CHROME VAULT v3 controls of the approved mockup: panels, section headers, pills, segmented switches, power buttons, the
// top-bar tabs and toggles. Nothing here takes the keyboard focus.
namespace vui
{
inline void noFocus (juce::Component& c) { c.setWantsKeyboardFocus (false); c.setMouseClickGrabsKeyboardFocus (false); }

// style A "Machined" - the studio-hardware surfaces every page shares (docs/qa/design/README.md)
// a panel: an anodised plate - dark gradient, a fine lit top edge, 1 px outline, a soft shadow under it
inline void drawPanel (juce::Graphics& g, juce::Rectangle<float> r, float radius = 10.0f)
{
    using juce::Colour;
    g.setColour (Colour (0x40000000));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius);
    g.setGradientFill (juce::ColourGradient (Colour (0xff171e2a), 0, r.getY(), Colour (0xff0f141c), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    g.setColour (Colour (0xff252e3e));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    // the lit top edge of the plate
    g.setGradientFill (juce::ColourGradient (Colour (0x14ffffff), r.getCentreX(), r.getY(), Colour (0x00ffffff), r.getX() + 8.0f, r.getY(), true));
    g.fillRect (r.getX() + radius, r.getY() + 1.0f, r.getWidth() - 2.0f * radius, 1.0f);
}

// a raised hardware button face (pills, dropdowns, toggles, tabs): dark metal, lit top edge, soft shadow; `on` = an
// ice-lit outline, `down` = pressed in
inline void drawButtonFace (juce::Graphics& g, juce::Rectangle<float> r, float radius, bool over, bool down, bool on, bool enabled = true)
{
    using juce::Colour;
    if (! down)
    {
        g.setColour (Colour (0x55000000));
        g.fillRoundedRectangle (r.translated (0.0f, 1.0f), radius);
    }
    const Colour top (down ? 0xff10151c : on ? 0xff253046 : over ? 0xff252e3c : 0xff1e2531);
    const Colour bot (down ? 0xff171d27 : on ? 0xff172031 : over ? 0xff161c26 : 0xff131820);
    g.setGradientFill (juce::ColourGradient (top, 0, r.getY(), bot, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    if (! down)
    {
        g.setColour (Colour (on ? 0x1f9fc3ff : 0x14ffffff));
        g.fillRect (r.getX() + radius * 0.8f, r.getY() + 1.0f, r.getWidth() - radius * 1.6f, 1.0f);
    }
    g.setColour (on ? col::ice2.withAlpha (enabled ? 0.75f : 0.35f) : over && enabled ? Colour (0xff42506a) : Colour (0xff2a3445));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
}

// a recessed well (segment tracks, the search field, step lanes): dark, shadowed at the top, a faint lit lower edge
inline void drawWell (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    using juce::Colour;
    g.setColour (Colour (0xff080b11));
    g.fillRoundedRectangle (r, radius);
    g.setGradientFill (juce::ColourGradient (Colour (0x66000000), 0, r.getY(), Colour (0x00000000), 0, r.getY() + 5.0f, false));
    g.fillRoundedRectangle (r.withHeight (std::min (r.getHeight(), 6.0f)), radius);
    g.setColour (Colour (0xff1c2432));
    g.drawRoundedRectangle (r.reduced (0.5f), radius, 1.0f);
    g.setColour (Colour (0x0dffffff));
    g.fillRect (r.getX() + radius, r.getBottom() - 0.5f, r.getWidth() - 2.0f * radius, 1.0f);
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
        auto r = getLocalBounds().toFloat().reduced (0.5f, 0.5f).withTrimmedBottom (1.0f);
        const bool on = getToggleState();
        drawButtonFace (g, r, 6.0f, over && ! dimmed, down, on, ! dimmed);
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
        auto r = getLocalBounds().toFloat().reduced (0.5f, 0.5f).withTrimmedBottom (1.0f);
        drawButtonFace (g, r, 5.0f, over && isEnabled(), down, false, isEnabled());
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
        // a recessed track; the chosen segment is a raised key with an ice-lit edge
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        drawWell (g, r, 6.0f);
        const auto rects = segRects();
        for (int i = 0; i < options.size(); ++i)
        {
            const auto sr = rects[(size_t) i];
            const bool on = i == selected;
            if (on) drawButtonFace (g, sr.reduced (2.0f, 2.0f), 4.5f, false, false, enabledLook, enabledLook);
            else if (i > 0 && i - 1 != selected)
            {
                g.setColour (juce::Colour (0xff1c2432));
                g.fillRect (sr.getX(), sr.getY() + 7.0f, 1.0f, sr.getHeight() - 14.0f);
            }
            g.setFont (segFont());
            g.setColour (! enabledLook ? col::dim : on ? col::ice : i == hover ? col::text : col::muted);
            g.drawText (options[i], sr, juce::Justification::centred, false);
        }
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
        // a round hardware push button with an LED in its middle
        const float d = std::min ((float) getWidth(), (float) getHeight()) - 2.0f;
        auto r = juce::Rectangle<float> (d, d).withCentre (getLocalBounds().toFloat().getCentre());
        const bool on = getToggleState();
        g.setColour (juce::Colour (0x66000000));
        g.fillEllipse (r.translated (0.0f, 1.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (over ? 0xff8693a8 : 0xff6b778b), r.getCentreX(), r.getY(), juce::Colour (0xff0d1118), r.getCentreX(), r.getBottom(), false));
        g.fillEllipse (r);
        const auto face = r.reduced (1.4f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a4454), face.getX() + face.getWidth() * 0.4f, face.getY() + face.getHeight() * 0.3f,
                                                 juce::Colour (0xff151a22), face.getRight(), face.getBottom(), true));
        g.fillEllipse (face);
        drawLed (g, juce::Rectangle<float> (d * 0.36f, d * 0.36f).withCentre (r.getCentre()), on);
    }
};

//==============================================================================
// top-bar page tab ("MAIN", "LAYERS" ...)
class TabButton : public juce::Button
{
public:
    explicit TabButton (const juce::String& t) : juce::Button (t)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTriggeredOnMouseDown (true);   // a page switch must never get lost in a small drag
    }
    static juce::Font tabFont() { return font (14.0f, true, 0.1f); }
    int idealWidth() const { return (int) std::ceil (textWidth (tabFont(), getButtonText()) + 22.0f); }
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        // the page selector: the chosen page is a raised key with an ice LED strip under its name
        auto r = getLocalBounds().toFloat().reduced (0.5f, 0.5f).withTrimmedBottom (1.0f);
        const bool on = getToggleState();
        if (on)
        {
            drawButtonFace (g, r, 6.0f, false, false, false);
            const auto strip = juce::Rectangle<float> (r.getX() + 9.0f, r.getBottom() - 4.0f, r.getWidth() - 18.0f, 2.0f);
            g.setColour (col::ice.withAlpha (0.25f));
            g.fillRoundedRectangle (strip.expanded (1.5f, 1.0f), 2.0f);
            g.setColour (col::ice);
            g.fillRoundedRectangle (strip, 1.0f);
        }
        else if (over)
        {
            g.setColour (juce::Colour (0x0affffff));
            g.fillRoundedRectangle (r, 6.0f);
        }
        g.setFont (tabFont());
        g.setColour (on ? col::text : over ? col::muted : col::dim);
        g.drawText (getButtonText(), r, juce::Justification::centred, false);
    }
};

// top-bar group header ("ADVANCED"): clickable - opens the group's first page; lit while a page of the group is shown
class GroupButton : public juce::Button
{
public:
    explicit GroupButton (const juce::String& t) : juce::Button (t)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        setTriggeredOnMouseDown (true);
    }
    static juce::Font groupFont() { return font (14.0f, false, 0.16f); }
    int idealWidth() const { return (int) std::ceil (textWidth (groupFont(), getButtonText()) + 20.0f); }
    void paintButton (juce::Graphics& g, bool over, bool) override
    {
        auto r = getLocalBounds().toFloat();
        const bool on = getToggleState();
        if (over) { g.setColour (col::tabOn.withAlpha (0.6f)); g.fillRoundedRectangle (r, 6.0f); }
        g.setFont (groupFont());
        g.setColour (on ? col::ice : over ? col::text : col::dim);
        g.drawText (getButtonText(), r, juce::Justification::centred, false);
        // a small chevron: the group continues to the right
        const float cx = r.getRight() - 4.0f, cy = r.getCentreY();
        juce::Path p;
        p.startNewSubPath (cx - 2.5f, cy - 3.5f); p.lineTo (cx + 0.5f, cy); p.lineTo (cx - 2.5f, cy + 3.5f);
        g.strokePath (p, juce::PathStrokeType (1.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
        auto r = getLocalBounds().toFloat().reduced (0.5f, 0.5f).withTrimmedBottom (1.0f);
        const bool on = getToggleState();
        drawButtonFace (g, r, 6.0f, over, false, false);
        drawLed (g, juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ r.getX() + 14.0f, r.getCentreY() }), on);
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
        auto r = getLocalBounds().toFloat().reduced (0.5f, 0.5f).withTrimmedBottom (1.0f);
        drawButtonFace (g, r, 6.0f, over, false, false);
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
        g.setColour (juce::Colour (0xff1b222d));
        g.fillEllipse (juce::Rectangle<float> (5.0f, 5.0f).withCentre (c));
    }
};
} // namespace vui
