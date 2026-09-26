#include "PluginEditor.h"
#include <numeric>

using namespace juce;

#include "UiCommon.h"

//==============================================================================
void KKLookAndFeel::setSkin (const Skin& s)
{
    skin = &s;
    setColour (PopupMenu::backgroundColourId, s.dark ? Colour (0xff141111) : Colour (0xffeef0f3));
    setColour (PopupMenu::textColourId, s.text);
    setColour (PopupMenu::highlightedBackgroundColourId, s.accent.withAlpha (0.8f));
    setColour (PopupMenu::highlightedTextColourId, s.dark ? Colours::white : Colours::black);
    setColour (PopupMenu::headerTextColourId, s.accent);
    setColour (Label::textColourId, s.text);
    setColour (TooltipWindow::backgroundColourId, s.dark ? Colour (0xff1a1515) : Colour (0xfff2f4f6));
    setColour (TooltipWindow::textColourId, s.text);
    setColour (TooltipWindow::outlineColourId, s.accent);
    setColour (ComboBox::backgroundColourId, s.dark ? Colour (0xff1a1616) : Colour (0xfff5f6f8));
    setColour (ComboBox::textColourId, s.text);
    setColour (ComboBox::outlineColourId, s.panelEdge);
    setColour (ComboBox::arrowColourId, s.accent);
    setColour (ToggleButton::textColourId, s.text);
    setColour (ToggleButton::tickColourId, s.accent);
    setColour (ToggleButton::tickDisabledColourId, s.textDim);
    setColour (Slider::textBoxTextColourId, s.text);
    setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    setColour (TextButton::textColourOffId, s.text);
    setColour (TextButton::textColourOnId, s.accent);
    setColour (TabbedButtonBar::tabTextColourId, s.textDim);
    setColour (TabbedButtonBar::frontTextColourId, s.dark ? s.accent : s.text);
    setColour (TabbedButtonBar::tabOutlineColourId, s.panelEdge);
    setColour (TabbedButtonBar::frontOutlineColourId, s.accent);
    setColour (TabbedComponent::outlineColourId, Colours::transparentBlack);
    setColour (TextEditor::backgroundColourId, s.dark ? Colour (0xff141010) : Colour (0xfff7f8fa));
    setColour (TextEditor::textColourId, s.text);
    setColour (TextEditor::outlineColourId, s.panelEdge);
    setColour (TextEditor::focusedOutlineColourId, s.accent);
    setColour (ListBox::backgroundColourId, Colours::transparentBlack);
    setColour (ScrollBar::thumbColourId, s.accent.withAlpha (0.6f));
    setColour (Slider::trackColourId, s.accent);
    setColour (Slider::backgroundColourId, s.track);
    setColour (Slider::thumbColourId, s.knobLight);
    setColour (Slider::textBoxBackgroundColourId, Colours::transparentBlack);
}

void KKLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider&)
{
    const auto& s = *skin;
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float size = jmin (b.getWidth(), b.getHeight());
    auto r = b.withSizeKeepingCentre (size, size).reduced (size * 0.04f);
    const auto c = r.getCentre();
    const float R = r.getWidth() * 0.5f;
    const float ang = a0 + pos * (a1 - a0);

    // value arc
    Path track; track.addCentredArc (c.x, c.y, R * 0.93f, R * 0.93f, 0, a0, a1, true);
    g.setColour (s.track.withAlpha (0.6f));
    g.strokePath (track, PathStrokeType (R * 0.07f, PathStrokeType::curved, PathStrokeType::butt));
    if (pos > 0.001f)
    {
        Path val; val.addCentredArc (c.x, c.y, R * 0.93f, R * 0.93f, 0, a0, ang, true);
        g.setColour (s.accent.withAlpha (0.25f));
        g.strokePath (val, PathStrokeType (R * 0.2f, PathStrokeType::curved, PathStrokeType::butt));
        g.setColour (s.accent);
        g.strokePath (val, PathStrokeType (R * 0.08f, PathStrokeType::curved, PathStrokeType::butt));
    }

    // serrated outer ring
    const float ro = R * 0.8f;
    g.setGradientFill (ColourGradient (s.knobLight, c.x - ro, c.y - ro, s.knobDark, c.x + ro, c.y + ro, false));
    g.fillEllipse (Rectangle<float> (ro * 2, ro * 2).withCentre (c));
    g.setColour (s.dark ? Colours::black.withAlpha (0.5f) : s.knobDark.withAlpha (0.6f));
    for (int i = 0; i < 40; ++i)
    {
        const float a = MathConstants<float>::twoPi * (float) i / 40.0f;
        g.drawLine (c.x + std::sin (a) * ro * 0.86f, c.y - std::cos (a) * ro * 0.86f,
                    c.x + std::sin (a) * ro, c.y - std::cos (a) * ro, 1.0f);
    }
    // brushed cap
    const float rc = R * 0.62f;
    ColourGradient cap (s.knobDark.brighter (0.2f), c.x, c.y + rc, s.knobLight, c.x, c.y - rc, false);
    cap.addColour (0.5, s.dark ? Colour (0xff6b6f75) : Colour (0xffc5cad0));
    g.setGradientFill (cap);
    g.fillEllipse (Rectangle<float> (rc * 2, rc * 2).withCentre (c));
    for (int i = 1; i < 6; ++i)
    {
        g.setColour (Colours::white.withAlpha (0.05f * (float) (i % 2 + 1)));
        g.drawEllipse (Rectangle<float> (rc * 2 * i / 6.0f, rc * 2 * i / 6.0f).withCentre (c), 0.8f);
    }
    g.setColour (Colours::black.withAlpha (0.5f));
    g.drawEllipse (Rectangle<float> (rc * 2, rc * 2).withCentre (c), 1.2f);

    // pointer
    const Point<float> p0 (c.x + std::sin (ang) * rc * 0.25f, c.y - std::cos (ang) * rc * 0.25f);
    const Point<float> p1 (c.x + std::sin (ang) * rc * 0.92f, c.y - std::cos (ang) * rc * 0.92f);
    g.setColour (s.dark ? Colours::white : Colour (0xff15171a));
    g.drawLine ({ p0, p1 }, jmax (2.0f, R * 0.07f));
}

void KKLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float pos, float, float,
                                      Slider::SliderStyle style, Slider& sl)
{
    const auto& s = *skin;
    auto b = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    if (style == Slider::LinearVertical)   // pitch / mod wheel
    {
        auto well = b.reduced (2);
        g.setColour (Colours::black.withAlpha (0.7f));
        g.fillRoundedRectangle (well, 6);
        const float ty = jlimit (well.getY() + 6, well.getBottom() - 6, pos);
        ColourGradient wg (s.knobDark, well.getX(), 0, s.knobDark, well.getRight(), 0, false);
        wg.addColour (0.5, s.accent.withAlpha (0.8f));
        g.setGradientFill (wg);
        g.fillRoundedRectangle (well.reduced (5, 4), 5);
        for (int i = 0; i < 14; ++i)
        {
            const float yy = well.getY() + 8 + (well.getHeight() - 16) * (float) i / 13.0f;
            g.setColour (Colours::black.withAlpha (0.25f));
            g.drawHorizontalLine ((int) yy, well.getX() + 6, well.getRight() - 6);
        }
        g.setColour (Colours::white.withAlpha (0.9f));
        g.fillRoundedRectangle (Rectangle<float> (well.getWidth() - 8, 5).withCentre ({ well.getCentreX(), ty }), 2);
        ignoreUnused (sl);
        return;
    }
    // horizontal (CHAOS)
    auto tr = b.withSizeKeepingCentre (b.getWidth() - 10, 6);
    g.setColour (s.track);
    g.fillRoundedRectangle (tr, 3);
    g.setColour (s.accent);
    g.fillRoundedRectangle (tr.withRight (pos), 3);
    g.setColour (s.accent.withAlpha (0.3f));
    g.fillEllipse (Rectangle<float> (22, 22).withCentre ({ pos, b.getCentreY() }));
    g.setColour (s.knobLight);
    g.fillEllipse (Rectangle<float> (14, 14).withCentre ({ pos, b.getCentreY() }));
    g.setColour (s.accent);
    g.drawEllipse (Rectangle<float> (14, 14).withCentre ({ pos, b.getCentreY() }), 2);
}

void KKLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    const auto& s = *skin;
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();
    g.setColour (s.dark ? Colour (0xff120f0f) : Colour (0xffe9ecef));
    g.fillRoundedRectangle (r, 4);
    if (on)
    {
        g.setColour (s.accent.withAlpha (0.18f)); g.fillRoundedRectangle (r, 4);
        g.setColour (s.accent.withAlpha (0.35f)); g.drawRoundedRectangle (r.expanded (1.5f), 5, 3);
    }
    g.setColour (on ? s.accent : (over ? s.text.withAlpha (0.6f) : s.panelEdge));
    g.drawRoundedRectangle (r, 4, on ? 2.0f : 1.2f);
    if (down) { g.setColour (Colours::black.withAlpha (0.15f)); g.fillRoundedRectangle (r, 4); }
}

void KKLookAndFeel::drawButtonText (Graphics& g, TextButton& b, bool, bool)
{
    g.setFont (getTextButtonFont (b, b.getHeight()));
    g.setColour (b.getToggleState() ? (skin->dark ? skin->accent : skin->text) : skin->text);
    g.drawText (b.getButtonText(), b.getLocalBounds(), Justification::centred);
}

Font KKLookAndFeel::getTextButtonFont (TextButton&, int h) { return serif (jmin (17.0f, (float) h * 0.5f), false, 0.2f); }
Font KKLookAndFeel::getLabelFont (Label& l) { return serif (jmax (11.0f, (float) l.getHeight() * 0.72f), false, 0.2f); }
Font KKLookAndFeel::getPopupMenuFont() { return serif (15.0f, false, 0.05f); }

//==============================================================================
// Tiles
class TileButton : public Component, public SettableTooltipClient
{
public:
    TileButton (int idx, KKLookAndFeel& l) : index (idx), lnf (l) {}
    std::function<void()> onClick;
    bool selected = false;

    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (2);
        g.setColour (s.dark ? Colour (0xff0c0a0a) : Colour (0xffe6e9ec).withAlpha (0.9f));
        g.fillRoundedRectangle (r, 5);
        if (selected)
        {
            g.setColour (s.accent.withAlpha (0.25f)); g.drawRoundedRectangle (r.expanded (1), 6, 5);
            g.setColour (s.accent); g.drawRoundedRectangle (r, 5, 2.2f);
        }
        else
        {
            g.setColour (hover ? s.text.withAlpha (0.5f) : s.panelEdge);
            g.drawRoundedRectangle (r, 5, 1.1f);
        }
        auto icon = r.reduced (10).withTrimmedBottom (26);
        icon = icon.withSizeKeepingCentre (jmin (icon.getWidth(), icon.getHeight()), jmin (icon.getWidth(), icon.getHeight()));
        drawIcon (g, icon, s);
        g.setColour (s.text);
        g.setFont (serif (index == tExperimental ? 9.5f : 12.5f, false, index == tExperimental ? 0.0f : 0.2f));
        g.drawFittedText (tileNames()[index], r.removeFromBottom (26).toNearestInt().reduced (2, 0), Justification::centred, 1, 0.8f);
    }

    void mouseUp (const MouseEvent& e) override { if (e.mouseWasClicked() && onClick) onClick(); }
    void mouseEnter (const MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const MouseEvent&) override { hover = false; repaint(); }

private:
    void drawIcon (Graphics& g, Rectangle<float> b, const Skin& s)
    {
        auto P = [&] (float x, float y) { return Point<float> (b.getX() + x * b.getWidth(), b.getY() + y * b.getHeight()); };
        ColourGradient metal (s.iconLight, b.getX(), b.getY(), s.iconDark, b.getRight(), b.getBottom(), false);
        metal.addColour (0.45, s.iconDark.brighter (0.4f));
        metal.addColour (0.55, s.iconLight.withAlpha (0.9f));
        const Colour edge = s.dark ? Colours::black : Colour (0xff3c4248);
        Path p;
        switch (index)
        {
            case tBells:
                p.startNewSubPath (P (0.5f, 0.12f));
                p.cubicTo (P (0.28f, 0.14f), P (0.3f, 0.5f), P (0.14f, 0.76f));
                p.lineTo (P (0.86f, 0.76f));
                p.cubicTo (P (0.7f, 0.5f), P (0.72f, 0.14f), P (0.5f, 0.12f));
                p.closeSubPath();
                p.addEllipse (Rectangle<float> (b.getWidth() * 0.14f, b.getWidth() * 0.14f).withCentre (P (0.5f, 0.84f)));
                p.addEllipse (Rectangle<float> (b.getWidth() * 0.1f, b.getWidth() * 0.1f).withCentre (P (0.5f, 0.08f)));
                break;
            case tKeys:
            {
                p.addRectangle (Rectangle<float> (P (0.1f, 0.18f), P (0.9f, 0.82f)));
                g.setGradientFill (metal); g.fillPath (p);
                g.setColour (edge);
                for (int i = 1; i < 5; ++i) g.drawLine (Line<float> (P (0.1f + 0.16f * i, 0.18f), P (0.1f + 0.16f * i, 0.82f)), 1.2f);
                for (int i : { 1, 2, 3, 4 })
                    g.fillRect (Rectangle<float> (P (0.1f + 0.16f * i - 0.05f, 0.18f), P (0.1f + 0.16f * i + 0.05f, 0.55f)));
                g.drawRect (Rectangle<float> (P (0.1f, 0.18f), P (0.9f, 0.82f)), 1.2f);
                return;
            }
            case tPlucks:
            {
                g.setGradientFill (metal);
                Path arc; arc.startNewSubPath (P (0.15f, 0.2f)); arc.quadraticTo (P (0.5f, 0.02f), P (0.85f, 0.2f));
                g.strokePath (arc, PathStrokeType (3.0f));
                for (int i = 0; i < 9; ++i)
                    g.drawLine (Line<float> (P (0.18f + 0.08f * i, 0.18f - (i > 1 && i < 7 ? 0.04f : 0.0f)), P (0.46f + 0.01f * i, 0.8f)), 1.3f);
                g.fillEllipse (Rectangle<float> (b.getWidth() * 0.3f, b.getWidth() * 0.08f).withCentre (P (0.5f, 0.84f)));
                return;
            }
            case tFlutes:
            {
                p.addRoundedRectangle (-0.06f, -0.45f, 0.12f, 0.9f, 0.05f);
                for (int i = 0; i < 5; ++i) p.addEllipse (-0.022f, -0.25f + 0.1f * i, 0.044f, 0.044f);
                p.setUsingNonZeroWinding (false);
                p.applyTransform (AffineTransform::rotation (0.75f).scaled (b.getWidth()).translated (b.getCentreX(), b.getCentreY()));
                break;
            }
            case tChoir:
                for (int i = 0; i < 3; ++i)
                {
                    const float cx = 0.25f + 0.25f * (float) i, top = i == 1 ? 0.12f : 0.25f;
                    Path f; f.startNewSubPath (P (cx - 0.15f, 0.88f));
                    f.cubicTo (P (cx - 0.14f, top + 0.2f), P (cx - 0.08f, top), P (cx, top));
                    f.cubicTo (P (cx + 0.08f, top), P (cx + 0.14f, top + 0.2f), P (cx + 0.15f, 0.88f));
                    f.closeSubPath();
                    p.addPath (f);
                }
                break;
            case tPads:
                for (auto c : { Point<float> (0.3f, 0.55f), Point<float> (0.5f, 0.42f), Point<float> (0.7f, 0.55f), Point<float> (0.42f, 0.62f), Point<float> (0.6f, 0.64f) })
                    p.addEllipse (Rectangle<float> (b.getWidth() * 0.34f, b.getWidth() * 0.3f).withCentre (P (c.x, c.y)));
                break;
            case tLeads:
            {
                for (int i = 0; i < 16; ++i)
                {
                    const float a = MathConstants<float>::pi * 0.125f * (float) i;
                    const float rr = (i % 4 == 0) ? 0.48f : (i % 2 == 0 ? 0.22f : 0.07f);
                    auto pt = P (0.5f + std::sin (a) * rr, 0.5f - std::cos (a) * rr);
                    if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
                }
                p.closeSubPath();
                break;
            }
            case tBass:
            {
                g.setGradientFill (metal);
                g.fillEllipse (Rectangle<float> (P (0.12f, 0.12f), P (0.88f, 0.88f)));
                g.setColour (edge);
                g.fillEllipse (Rectangle<float> (P (0.22f, 0.22f), P (0.78f, 0.78f)));
                g.setGradientFill (metal);
                g.fillEllipse (Rectangle<float> (P (0.36f, 0.36f), P (0.64f, 0.64f)));
                g.setColour (edge); g.drawEllipse (Rectangle<float> (P (0.12f, 0.12f), P (0.88f, 0.88f)), 1.2f);
                return;
            }
            case tExotic:
            {
                p.addEllipse (Rectangle<float> (P (0.1f, 0.5f), P (0.5f, 0.9f)));
                Path neck; neck.addRectangle (-0.03f, -0.55f, 0.06f, 0.55f);
                neck.applyTransform (AffineTransform::rotation (0.8f).scaled (b.getWidth()).translated (P (0.32f, 0.7f)));
                p.addPath (neck);
                p.addRectangle (Rectangle<float> (P (0.72f, 0.08f), P (0.88f, 0.22f)));
                break;
            }
            default:
            {
                Random r (42);
                for (int i = 0; i < 7; ++i)
                {
                    const float cx = 0.25f + r.nextFloat() * 0.5f, cy = 0.2f + r.nextFloat() * 0.6f, sz = 0.12f + r.nextFloat() * 0.18f;
                    p.startNewSubPath (P (cx, cy - sz));
                    p.lineTo (P (cx + sz * 0.8f, cy + sz * 0.2f));
                    p.lineTo (P (cx - sz * 0.3f, cy + sz));
                    p.closeSubPath();
                }
                break;
            }
        }
        g.setGradientFill (metal);
        g.fillPath (p);
        g.setColour (edge.withAlpha (0.8f));
        g.strokePath (p, PathStrokeType (1.1f));
    }

    int index;
    KKLookAndFeel& lnf;
    bool hover = false;
};

//==============================================================================
class EraTimeline : public Component, public SettableTooltipClient
{
public:
    explicit EraTimeline (KKLookAndFeel& l) : lnf (l) { setTooltip ("Filter presets by era. Click again to show all eras."); }
    std::function<void (int)> onSelect;
    int selected = -1;

    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const float y = getHeight() * 0.68f;
        g.setColour (s.dark ? s.text.withAlpha (0.5f) : s.textDim);
        g.drawLine (xFor (0) - 10, y, (float) getWidth() - 4, y, 1.2f);
        for (int i = 0; i < 6; ++i)
        {
            const float x = xFor (i);
            const bool on = i == selected;
            if (on) { g.setColour (s.accent.withAlpha (0.35f)); g.fillEllipse (Rectangle<float> (20, 20).withCentre ({ x, y })); }
            g.setColour (on ? s.accent : (s.dark ? s.text : Colour (0xfff5f5f5)));
            g.fillEllipse (Rectangle<float> (on ? 11.0f : 10.0f, on ? 11.0f : 10.0f).withCentre ({ x, y }));
            g.setColour (on ? s.accent : s.textDim);
            g.drawEllipse (Rectangle<float> (10, 10).withCentre ({ x, y }), 1.2f);
            g.setColour (on ? s.accent : s.text);
            g.setFont (serif (15.0f, false, 0.0f));
            g.drawText (eraNames()[i], Rectangle<float> (60, 20).withCentre ({ x, y - 18 }), Justification::centred);
        }
    }

    void mouseUp (const MouseEvent& e) override
    {
        int best = 0;
        for (int i = 1; i < 6; ++i) if (std::abs (xFor (i) - e.position.x) < std::abs (xFor (best) - e.position.x)) best = i;
        if (std::abs (xFor (best) - e.position.x) > 40) return;
        selected = selected == best ? -1 : best;
        repaint();
        if (onSelect) onSelect (selected);
    }

private:
    float xFor (int i) const { return 30.0f + (float) i * ((float) getWidth() - 110.0f) / 5.0f; }
    KKLookAndFeel& lnf;
};

//==============================================================================
class DiceButton : public Component, public SettableTooltipClient
{
public:
    explicit DiceButton (KKLookAndFeel& l) : lnf (l) { setTooltip ("DICE: roll a brand-new sound in the selected category and era. Right-click: undo, history, locks, save."); }
    std::function<void()> onRoll, onUndo;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto b = getLocalBounds().toFloat().reduced (8).withTrimmedBottom (26);
        const float sz = jmin (b.getWidth(), b.getHeight());
        auto c = b.getCentre();
        const float h = sz * 0.42f;
        // isometric cube
        Point<float> top (c.x, c.y - h), l (c.x - h * 0.9f, c.y - h * 0.45f), rr (c.x + h * 0.9f, c.y - h * 0.45f), mid (c.x, c.y);
        Point<float> bl (c.x - h * 0.9f, c.y + h * 0.55f), br (c.x + h * 0.9f, c.y + h * 0.55f), bot (c.x, c.y + h);
        auto face = [&] (std::initializer_list<Point<float>> pts, Colour col)
        {
            Path p; bool first = true;
            for (auto pt : pts) { if (first) p.startNewSubPath (pt); else p.lineTo (pt); first = false; }
            p.closeSubPath();
            g.setColour (col); g.fillPath (p);
            g.setColour (s.dark ? Colours::black : Colour (0xff50575f)); g.strokePath (p, PathStrokeType (1.2f));
        };
        const Colour baseC = s.dark ? Colour (0xff1b1717) : Colour (0xffd9dde2);
        face ({ top, rr, mid, l }, baseC.brighter (0.25f));
        face ({ l, mid, bot, bl }, baseC);
        face ({ mid, rr, br, bot }, baseC.darker (0.25f));
        auto pip = [&] (Point<float> p) { g.setColour (s.accent.withAlpha (0.35f)); g.fillEllipse (Rectangle<float> (h * 0.2f, h * 0.2f).withCentre (p));
                                         g.setColour (s.accent); g.fillEllipse (Rectangle<float> (h * 0.12f, h * 0.12f).withCentre (p)); };
        pip ((top + mid) * 0.5f); pip ((l + top) * 0.5f * 0.5f + (mid + rr) * 0.5f * 0.5f);
        for (int i = 0; i < 3; ++i) pip (l + (bot - l) * (0.25f + 0.25f * i) + Point<float> (h * 0.15f, -h * 0.1f));
        for (int i = 0; i < 2; ++i) pip (mid + (br - mid) * (0.3f + 0.4f * i) + Point<float> (0, h * 0.2f));
        g.setColour (s.text);
        g.setFont (serif (19.0f, false, 0.25f));
        g.drawText ("DICE", getLocalBounds().removeFromBottom (30), Justification::centred);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (! e.mouseWasClicked()) return;
        if (e.mods.isPopupMenu()) { if (onUndo) onUndo(); }
        else if (onRoll) onRoll();
    }
private:
    KKLookAndFeel& lnf;
};

//==============================================================================
class XYPad : public Component, public SettableTooltipClient
{
public:
    XYPad (KeysKillaProcessor& p, KKLookAndFeel& l)
        : lnf (l),
          ax (*p.apvts.getParameter (ID::morphX), [this] (float v) { x = v; repaint(); }),
          ay (*p.apvts.getParameter (ID::morphY), [this] (float v) { y = v; repaint(); })
    {
        ax.sendInitialUpdate(); ay.sendInitialUpdate();
        setTooltip ("ERA MORPH: drag between classic, melodic, raw and aggressive character. Double-click to centre.");
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat();
        g.setColour (s.text); g.setFont (serif (12.0f, false, 0.1f));
        g.drawText ("CLASSIC", r.removeFromTop (18), Justification::centredLeft);
        g.drawText ("MELODIC", r.withHeight (18).translated (0, -18), Justification::centredRight);
        auto bottom = r.removeFromBottom (36);
        g.drawText ("RAW", bottom.removeFromTop (16), Justification::centredLeft);
        g.drawText ("AGGRESSIVE", bottom.withY (bottom.getY() - 16).withHeight (16), Justification::centredRight);
        g.setFont (serif (16.0f, false, 0.25f));
        g.drawText ("ERA MORPH", bottom, Justification::centred);
        pad = r.reduced (4, 2);
        g.setColour (s.dark ? Colour (0xff0a0808) : Colour (0xffd5d9de));
        g.fillRect (pad);
        g.setColour (s.panelEdge); g.drawRect (pad, 1.0f);
        g.setColour (s.dark ? s.text.withAlpha (0.35f) : s.textDim.withAlpha (0.5f));
        g.drawLine (pad.getCentreX(), pad.getY(), pad.getCentreX(), pad.getBottom(), 1.0f);
        g.drawLine (pad.getX(), pad.getCentreY(), pad.getRight(), pad.getCentreY(), 1.0f);
        const Point<float> d (pad.getX() + x * pad.getWidth(), pad.getBottom() - y * pad.getHeight());
        g.setColour (s.accent.withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (28, 28).withCentre (d));
        g.setColour (s.accent.withAlpha (0.5f)); g.fillEllipse (Rectangle<float> (16, 16).withCentre (d));
        g.setColour (Colours::white); g.fillEllipse (Rectangle<float> (7, 7).withCentre (d));
    }
    void mouseDown (const MouseEvent& e) override { ax.beginGesture(); ay.beginGesture(); drag (e); }
    void mouseDrag (const MouseEvent& e) override { drag (e); }
    void mouseUp (const MouseEvent&) override { ax.endGesture(); ay.endGesture(); }
    void mouseDoubleClick (const MouseEvent&) override { ax.setValueAsCompleteGesture (0.5f); ay.setValueAsCompleteGesture (0.5f); }
private:
    void drag (const MouseEvent& e)
    {
        if (pad.isEmpty()) return;
        ax.setValueAsPartOfGesture (jlimit (0.0f, 1.0f, (e.position.x - pad.getX()) / pad.getWidth()));
        ay.setValueAsPartOfGesture (jlimit (0.0f, 1.0f, (pad.getBottom() - e.position.y) / pad.getHeight()));
    }
    KKLookAndFeel& lnf;
    ParameterAttachment ax, ay;
    float x = 0.5f, y = 0.5f;
    Rectangle<float> pad;
};

//==============================================================================
class Meter : public Component
{
public:
    explicit Meter (KKLookAndFeel& l) : lnf (l) {}
    float l = 0, r = 0; bool warn = false;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto b = getLocalBounds().toFloat();
        g.setColour (warn ? s.accent : s.text);
        g.setFont (serif (14.0f, false, 0.25f));
        g.drawText (warn ? "BASS CLIP!" : "OUTPUT", b.removeFromTop (22), Justification::centred);
        auto lab = b.removeFromBottom (18);
        g.setFont (serif (12.0f));
        g.setColour (s.text);
        auto bars = b.withTrimmedRight (b.getWidth() * 0.45f).reduced (6, 2);
        const float bw = bars.getWidth() / 2.0f - 3;
        const float marks[] { 0, -12, -24, -36 };
        auto dbToY = [&] (float db) { return bars.getY() + bars.getHeight() * (-db / 36.0f); };
        for (float m : marks)
            g.drawText (String ((int) m), Rectangle<float> (b.getRight() - b.getWidth() * 0.45f, dbToY (m) - 7, b.getWidth() * 0.4f, 14), Justification::centred);
        g.drawText ("L", Rectangle<float> (bars.getX(), lab.getY(), bw, 16), Justification::centred);
        g.drawText ("R", Rectangle<float> (bars.getX() + bw + 6, lab.getY(), bw, 16), Justification::centred);
        for (int ch = 0; ch < 2; ++ch)
        {
            const float v = ch == 0 ? l : r;
            const float db = jlimit (-36.0f, 0.0f, Decibels::gainToDecibels (v, -60.0f));
            auto col = Rectangle<float> (bars.getX() + ch * (bw + 6), bars.getY(), bw, bars.getHeight());
            const int segs = 14;
            for (int i = 0; i < segs; ++i)
            {
                const float segDb = -36.0f + 36.0f * (float) (i + 1) / segs;
                auto seg = Rectangle<float> (col.getX(), dbToY (segDb), bw, bars.getHeight() / segs - 2);
                const bool lit = db >= segDb - 36.0f / segs;
                g.setColour (lit ? s.accent : s.accent.withAlpha (0.12f));
                g.fillRect (seg);
            }
        }
    }
private:
    KKLookAndFeel& lnf;
};

//==============================================================================
class KKKeyboard : public MidiKeyboardComponent
{
public:
    KKKeyboard (KeysKillaProcessor& p, KKLookAndFeel& l)
        : MidiKeyboardComponent (p.keyboardState, horizontalKeyboard), proc (p), lnf (l)
    {
        setAvailableRange (24, 108);
        setScrollButtonsVisible (false);
        setOctaveForMiddleC (4);
        setWantsKeyboardFocus (false);
    }
    void drawWhiteNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour, Colour) override
    {
        const auto& s = *lnf.skin;
        const bool on = isDown || proc.playing[(size_t) note].load();
        ColourGradient gr (s.keyWhite, a.getX(), a.getY(), s.keyWhite.darker (0.12f), a.getX(), a.getBottom(), false);
        g.setGradientFill (gr);
        g.fillRect (a.reduced (0.5f, 0));
        if (! inScale (note)) { g.setColour (Colours::black.withAlpha (0.28f)); g.fillRect (a.reduced (0.5f, 0)); }
        if (isRoot (note)) { g.setColour (s.accent.withAlpha (0.7f)); g.fillEllipse (Rectangle<float> (6, 6).withCentre ({ a.getCentreX(), a.getBottom() - 24 })); }
        if (on)
        {
            g.setColour (s.accent.withAlpha (0.85f));
            g.fillRect (a.reduced (2, 0).withTrimmedTop (a.getHeight() * 0.05f));
        }
        else if (isOver) { g.setColour (s.accent.withAlpha (0.15f)); g.fillRect (a); }
        g.setColour (Colours::black.withAlpha (0.45f));
        g.drawVerticalLine ((int) a.getRight(), a.getY(), a.getBottom());
        if (note % 12 == 0)
        {
            g.setColour (Colours::black.withAlpha (0.5f)); g.setFont (serif (10.0f));
            g.drawText ("C" + String (note / 12 - 1), a.withTrimmedTop (a.getHeight() - 16), Justification::centred);
        }
    }
    void drawBlackNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour) override
    {
        const auto& s = *lnf.skin;
        const bool on = isDown || proc.playing[(size_t) note].load();
        g.setColour (on ? s.accent : s.keyBlack);
        g.fillRoundedRectangle (a.withTrimmedTop (-4), 2);
        if (! on && lockOn() && inScale (note)) { g.setColour (s.accent.withAlpha (0.35f)); g.fillRect (a.reduced (a.getWidth() * 0.3f, 0).withTop (a.getBottom() - 8)); }
        if (isRoot (note)) { g.setColour (s.accent); g.fillEllipse (Rectangle<float> (5, 5).withCentre ({ a.getCentreX(), a.getBottom() - 12 })); }
        g.setColour (Colours::white.withAlpha (isOver ? 0.25f : 0.12f));
        g.fillRect (a.reduced (a.getWidth() * 0.25f, 0).withTrimmedBottom (a.getHeight() * 0.2f).withWidth (2));
    }
    void drawUpDownButton (Graphics&, int, int, bool, bool, bool) override {}
private:
    bool lockOn() const { return proc.apvts.getRawParameterValue (ID::keyLock)->load() > 0.5f; }
    bool inScale (int note) const
    {
        if (! lockOn()) return true;
        const int key = (int) proc.apvts.getRawParameterValue (ID::key)->load();
        const int mask = Choices::scaleMask ((int) proc.apvts.getRawParameterValue (ID::scale)->load());
        return (mask >> (((note - key) % 12 + 12) % 12)) & 1;
    }
    bool isRoot (int note) const { return lockOn() && ((note - (int) proc.apvts.getRawParameterValue (ID::key)->load()) % 12 + 12) % 12 == 0; }
    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
};

#include "AdvancedPage.h"
#include "PresetBrowser.h"

//==============================================================================
class GlyphButton : public Button
{
public:
    GlyphButton (KKLookAndFeel& l, bool framed) : Button ({}), lnf (l), frame (framed) {}
    std::function<void (Graphics&, Rectangle<float>, const Skin&)> drawGlyph;
    void paintButton (Graphics& g, bool over, bool down) override
    {
        if (frame) lnf.drawButtonBackground (g, *this, {}, over, down);
        if (drawGlyph) drawGlyph (g, getLocalBounds().toFloat(), *lnf.skin);
    }
private:
    KKLookAndFeel& lnf;
    bool frame;
};

//==============================================================================
class MainPage : public Component, private Timer
{
public:
    MainPage (KeysKillaProcessor& p) : proc (p), era (lnf), dice (lnf), xy (p, lnf), meter (lnf), keyboard (p, lnf)
    {
        settings = openSettings();
        applySkin (settings->getIntValue ("skin", 0));
        setLookAndFeel (&lnf);

        for (int i = 0; i < numTiles; ++i)
        {
            auto t = std::make_unique<TileButton> (i, lnf);
            t->setTooltip ("Browse " + tileNames()[i].toLowerCase() + " presets.");
            t->onClick = [this, i] { proc.uiTile = i; proc.uiExclusive = false; loadFirstMatching(); };
            addAndMakeVisible (*t);
            tiles.push_back (std::move (t));
        }
        era.selected = proc.uiEra;
        era.onSelect = [this] (int e) { proc.uiEra = e; loadFirstMatching(); };
        addAndMakeVisible (era);

        exclusiveBtn.setButtonText ("EXCLUSIVE");
        exclusiveBtn.setTooltip ("Signature sounds built on the exclusive engine features.");
        exclusiveBtn.onClick = [this] { proc.uiExclusive = ! proc.uiExclusive; if (proc.uiExclusive) proc.uiTile = -1; loadFirstMatching(); };
        addAndMakeVisible (exclusiveBtn);

        // preset bar
        for (Button* b : { (Button*) &prevBtn, (Button*) &nextBtn, (Button*) &saveBtn, (Button*) &menuBtn, (Button*) &skinBtn }) addAndMakeVisible (*b);
        prevBtn.setButtonText ("<"); nextBtn.setButtonText (">");
        prevBtn.onClick = [this] { step (-1); }; nextBtn.onClick = [this] { step (1); };
        prevBtn.setTooltip ("Previous preset"); nextBtn.setTooltip ("Next preset");
        saveBtn.setButtonText ("SAVE"); saveBtn.setTooltip ("Save your sound as a preset file.");
        saveBtn.onClick = [this] { savePreset(); };
        menuBtn.setButtonText ("MENU"); menuBtn.onClick = [this] { showMenu(); };
        skinBtn.setTooltip ("Switch light (CHROME) / dark (BLOOD) skin.");
        skinBtn.onClick = [this] { applySkin (skinIndex == 0 ? 1 : 0); settings->setValue ("skin", skinIndex); repaintAll(); };
        presetLabel.setJustificationType (Justification::centred);
        presetLabel.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (presetLabel);
        heartBtn.drawGlyph = [this] (Graphics& g, Rectangle<float> hb, const Skin& s)
        {
            hb = hb.reduced (5);
            Path heart;
            heart.startNewSubPath (hb.getCentreX(), hb.getBottom());
            heart.cubicTo (hb.getX() - 4, hb.getCentreY(), hb.getX() + 2, hb.getY() - 3, hb.getCentreX(), hb.getY() + hb.getHeight() * 0.3f);
            heart.cubicTo (hb.getRight() - 2, hb.getY() - 3, hb.getRight() + 4, hb.getCentreY(), hb.getCentreX(), hb.getBottom());
            if (isFav) { g.setColour (s.accent); g.fillPath (heart); }
            g.setColour (isFav ? s.accent : s.text);
            g.strokePath (heart, PathStrokeType (1.5f));
        };
        skinBtn.drawGlyph = [] (Graphics& g, Rectangle<float> sb, const Skin& s)
        {
            sb = sb.reduced (10);
            g.setColour (s.text);
            if (s.dark)   // moon
            {
                g.fillEllipse (sb);
                g.setColour (Colour (0xff120f0f)); g.fillEllipse (sb.translated (sb.getWidth() * 0.38f, -sb.getHeight() * 0.22f));
            }
            else          // sun
            {
                g.fillEllipse (sb.reduced (sb.getWidth() * 0.25f));
                const auto c = sb.getCentre(); const float r = sb.getWidth() * 0.5f;
                for (int i = 0; i < 8; ++i)
                {
                    const float a = MathConstants<float>::twoPi * (float) i / 8.0f;
                    g.drawLine (c.x + std::sin (a) * r * 0.62f, c.y - std::cos (a) * r * 0.62f, c.x + std::sin (a) * r, c.y - std::cos (a) * r, 1.5f);
                }
            }
        };
        heartBtn.setTooltip ("Add to favourites");
        heartBtn.onClick = [this] { toggleFavourite(); };
        addAndMakeVisible (heartBtn);

        // macros
        const char* macroIds[] { ID::m1, ID::m2, ID::m3, ID::m4, ID::m5, ID::m6 };
        for (int i = 0; i < 6; ++i)
        {
            auto& k = macros[(size_t) i];
            setupKnob (k.slider, macroIds[i]);
            k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, macroIds[i], k.slider);
            k.label.setJustificationType (Justification::centred);
            addAndMakeVisible (k.slider); addAndMakeVisible (k.label);
        }
        // exclusive knobs
        const char* exIds[] { ID::ghost, ID::bend, ID::circuit };
        const char* exNames[] { "GHOST", "BEND", "CIRCUIT" };
        const char* exTips[] { "GHOST: adds a reversed, octave-up shadow layer with a haunted tail.",
                               "BEND: melody bends - scoop in, dive at note end, broken tape above 60%.",
                               "CIRCUIT: tempo-synced broken-electronics glitches (stutter, dropout, crush)." };
        for (int i = 0; i < 3; ++i)
        {
            auto& k = exKnobs[(size_t) i];
            setupKnob (k.slider, exIds[i]);
            k.slider.setTooltip (exTips[i]);
            k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, exIds[i], k.slider);
            k.label.setText (exNames[i], dontSendNotification);
            k.label.setJustificationType (Justification::centred);
            addAndMakeVisible (k.slider); addAndMakeVisible (k.label);
        }
        chaos.setSliderStyle (Slider::LinearHorizontal);
        chaos.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        chaos.setTooltip ("CHAOS: how far DICE moves away from the current sound.");
        chaosAtt = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ID::chaos, chaos);
        addAndMakeVisible (chaos);
        chaosLabel.setText ("CHAOS", dontSendNotification);
        addAndMakeVisible (chaosLabel);

        dice.onRoll = [this] { proc.rollDice (proc.uiTile >= 0 ? proc.uiTile : tileOfCurrent()); };
        dice.onUndo = [this] { showDiceMenu(); };
        addAndMakeVisible (dice);
        addAndMakeVisible (xy);

        chordBtn.setButtonText ("CHORD"); chordBtn.setClickingTogglesState (true);
        chordBtn.setTooltip ("CHORD: one key plays a minor 7th chord.");
        arpBtn.setButtonText ("ARP"); arpBtn.setClickingTogglesState (true);
        arpBtn.setTooltip ("ARP: tempo-synced arpeggiator (right-click for rate).");
        chordAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ID::chord, chordBtn);
        arpAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ID::arp, arpBtn);
        addAndMakeVisible (chordBtn); addAndMakeVisible (arpBtn);
        arpBtn.addMouseListener (this, false);
        addAndMakeVisible (meter);

        // wheels
        for (auto* w : { &pitchWheel, &modWheel })
        {
            w->setSliderStyle (Slider::LinearVertical);
            w->setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
            addAndMakeVisible (*w);
        }
        pitchWheel.setRange (-1.0, 1.0); pitchWheel.setValue (0.0);
        pitchWheel.onValueChange = [this] { proc.guiPitch = (float) pitchWheel.getValue(); };
        pitchWheel.onDragEnd = [this] { pitchWheel.setValue (0.0); };
        pitchWheel.setTooltip ("Pitch wheel (+/- 2 semitones)");
        modWheel.setRange (0.0, 1.0); modWheel.setValue (0.0);
        modWheel.onValueChange = [this] { proc.guiMod = (float) modWheel.getValue(); };
        modWheel.setTooltip ("Mod wheel: adds vibrato");
        for (auto* l : { &pitchLabel, &modLabel }) { l->setJustificationType (Justification::centred); addAndMakeVisible (*l); }
        pitchLabel.setText ("PITCH", dontSendNotification); modLabel.setText ("MOD", dontSendNotification);
        pitchLabel.setFont (serif (10.0f, false, 0.05f)); modLabel.setFont (serif (10.0f, false, 0.05f));

        addAndMakeVisible (keyboard);
        advanced = std::make_unique<AdvancedPage> (proc, lnf, skinIndex);
        advanced->onSkin = [this] (int sk) { setSkin (sk); };
        advanced->onSize = [this] (int pct) { setWindowSize (pct); };
        addChildComponent (*advanced);
        browser = std::make_unique<PresetBrowser> (proc, lnf);
        browser->getFavourites = [this] { return favourites(); };
        browser->toggleFavourite = [this] (const String& n) { toggleFavouriteNamed (n); };
        browser->onChanged = [this] { refreshState(); };
        addChildComponent (*browser);

        setSize (KeysKillaEditor::designW, KeysKillaEditor::designH);
        refreshState();
        startTimerHz (30);
    }

    ~MainPage() override { setLookAndFeel (nullptr); }

    // used by tests / screenshots: 0 main, 1..8 advanced tab, 9 preset browser
    void showView (int v)
    {
        if (v >= 1 && v <= 8) { advanced->showTab (v - 1); advanced->setVisible (true); advanced->toFront (false); }
        if (v == 9) browser->open (-1, -1, false);
    }

    void paint (Graphics& g) override
    {
        if (bg.isNull()) renderBackground();
        g.drawImage (bg, getLocalBounds().toFloat());
        const auto& s = *lnf.skin;
        drawPanel (g, browserR, s, "BROWSER");
        drawPanel (g, exclusiveR, s, "EXCLUSIVE");
        drawPanel (g, macroR, s);
        drawPanel (g, playR, s);
        drawPanel (g, meterR, s);
        drawPanel (g, wheelR, s);
        drawPanel (g, keyR, s);
        // exclusive sub-boxes
        g.setColour (s.panelEdge.withAlpha (0.7f));
        g.drawRoundedRectangle (dice.getBounds().toFloat().expanded (4, 4).withBottom ((float) chaos.getBottom() + 18), 6, 1);
        g.drawRoundedRectangle (xy.getBounds().toFloat().expanded (6, 6), 6, 1);
        // chord-arp link glyph
        g.setColour (s.text);
        auto link = Rectangle<float> ((float) chordBtn.getRight() + 4, (float) chordBtn.getY(), (float) (arpBtn.getX() - chordBtn.getRight() - 8), (float) chordBtn.getHeight());
        g.drawRoundedRectangle (link.withSizeKeepingCentre (14, 9).translated (-4, 0), 4, 1.6f);
        g.drawRoundedRectangle (link.withSizeKeepingCentre (14, 9).translated (4, 0), 4, 1.6f);
        drawLogo (g);
        // preset bar box
        g.setColour (s.dark ? Colour (0xee0b0909) : Colour (0xeef3f4f6));
        g.fillRoundedRectangle (presetR, 4);
        g.setColour (s.panelEdge); g.drawRoundedRectangle (presetR, 4, 1.4f);
    }

    void resized() override
    {
        presetR = { 545, 84, 350, 40 };
        prevBtn.setBounds (548, 88, 38, 32);
        nextBtn.setBounds (854, 88, 38, 32);
        presetLabel.setBounds (590, 88, 225, 32);
        heartBtn.setBounds (818, 90, 30, 28);
        saveBtn.setBounds (910, 84, 70, 40);
        menuBtn.setBounds (986, 84, 76, 40);
        skinBtn.setBounds (1068, 84, 40, 40);

        browserR = { 20, 168, 810, 222 };
        for (int i = 0; i < numTiles; ++i) tiles[(size_t) i]->setBounds (30 + i * 79, 205, 77, 108);
        era.setBounds (30, 320, 610, 58);
        exclusiveBtn.setBounds (648, 338, 170, 36);

        exclusiveR = { 840, 168, 342, 290 };
        dice.setBounds (852, 200, 118, 130);
        chaos.setBounds (852, 330, 118, 22);
        chaosLabel.setBounds (852, 350, 60, 16);
        xy.setBounds (984, 206, 188, 162);
        const int ex = 862;
        for (int i = 0; i < 3; ++i)
        {
            exKnobs[(size_t) i].slider.setBounds (ex + i * 104, 372, 64, 58);
            exKnobs[(size_t) i].label.setBounds (ex - 18 + i * 104, 430, 100, 20);
        }

        macroR = { 20, 400, 810, 190 };
        for (int i = 0; i < 6; ++i)
        {
            macros[(size_t) i].slider.setBounds (40 + i * 131, 412, 118, 128);
            macros[(size_t) i].label.setBounds (30 + i * 131, 546, 138, 26);
        }

        playR = { 840, 466, 230, 76 };
        chordBtn.setBounds (852, 486, 88, 36);
        arpBtn.setBounds (972, 486, 88, 36);
        meterR = { 1080, 466, 102, 124 };
        meter.setBounds (1086, 472, 92, 114);

        wheelR = { 20, 600, 86, 138 };
        pitchWheel.setBounds (28, 608, 32, 104); modWheel.setBounds (66, 608, 32, 104);
        pitchLabel.setBounds (22, 714, 44, 18); modLabel.setBounds (60, 714, 44, 18);
        keyR = { 112, 600, 1070, 138 };
        keyboard.setBounds (120, 606, 1054, 126);
        keyboard.setKeyWidth (1054.0f / 50.0f);
        advanced->setBounds (getLocalBounds());
        browser->setBounds (getLocalBounds().withTrimmedBottom (150));
    }

    void mouseUp (const MouseEvent& e) override
    {
        if (e.eventComponent == &arpBtn && e.mods.isPopupMenu())
        {
            PopupMenu m; auto* p = proc.apvts.getParameter (ID::arpRate);
            const int cur = (int) p->convertFrom0to1 (p->getValue());
            for (int i = 0; i < Choices::arpRates.size(); ++i) m.addItem (i + 1, "Rate " + Choices::arpRates[i], true, i == cur);
            m.showMenuAsync (PopupMenu::Options().withTargetComponent (arpBtn), [p] (int r) { if (r > 0) p->setValueNotifyingHost (p->convertTo0to1 ((float) (r - 1))); });
        }
    }

private:
    struct Knob
    {
        Slider slider { Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox };
        Label label;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> att;
    };

    void setupKnob (Slider& s, const char* id)
    {
        s.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        s.setVelocityModeParameters (0.6, 1, 0.02, true, ModifierKeys::ctrlModifier);
        auto* p = proc.apvts.getParameter (id);
        s.setDoubleClickReturnValue (true, p->convertFrom0to1 (p->getDefaultValue()));
        s.setPopupDisplayEnabled (true, true, this);
    }

    void applySkin (int idx)
    {
        skinIndex = jlimit (0, (int) Skin::all().size() - 1, idx);
        lnf.setSkin (Skin::all()[(size_t) skinIndex]);
        bg = {};
    }

    void repaintAll()
    {
        std::function<void (Component&)> rp = [&] (Component& c) { c.repaint(); for (auto* ch : c.getChildren()) rp (*ch); };
        rp (*this);
        sendLookAndFeelChange();
    }

    void renderBackground()
    {
        const auto& s = *lnf.skin;
        const int W = KeysKillaEditor::designW, H = KeysKillaEditor::designH;
        bg = Image (Image::ARGB, W * 2, H * 2, true);
        Graphics g (bg);
        g.addTransform (AffineTransform::scale (2.0f));
        g.setGradientFill (ColourGradient (s.bgTop, 0, 0, s.bgBottom, (float) W * 0.3f, (float) H, false));
        g.fillAll();
        Random r (s.dark ? 666 : 777);
        if (s.dark)
        {
            // grunge: blotches and scratches
            for (int i = 0; i < 90; ++i)
            {
                g.setColour ((r.nextBool() ? Colour (0xff3a0508) : Colour (0xff201c1c)).withAlpha (0.05f + r.nextFloat() * 0.12f));
                const float sz = 20 + r.nextFloat() * 160;
                g.fillEllipse (r.nextFloat() * W, r.nextFloat() * H, sz, sz * (0.3f + r.nextFloat()));
            }
            for (int i = 0; i < 400; ++i)
            {
                g.setColour (Colours::white.withAlpha (0.015f + r.nextFloat() * 0.04f));
                const float x = r.nextFloat() * W, y = r.nextFloat() * H;
                g.drawLine (x, y, x + r.nextFloat() * 60 - 30, y + r.nextFloat() * 8 - 4, 0.6f);
            }
            // barbed wire along edges
            auto wire = [&] (Point<float> a, Point<float> b)
            {
                g.setColour (Colour (0xff8a8a8a).withAlpha (0.55f));
                const int segs = (int) (a.getDistanceFrom (b) / 14);
                Path p; p.startNewSubPath (a);
                for (int i = 1; i <= segs; ++i)
                {
                    auto pt = a + (b - a) * ((float) i / segs);
                    p.lineTo (pt + Point<float> (0, (i % 2 ? 2.5f : -2.5f)));
                }
                g.strokePath (p, PathStrokeType (1.3f));
                for (int i = 1; i < segs; i += 4)
                {
                    auto pt = a + (b - a) * ((float) i / segs);
                    g.drawLine (pt.x - 5, pt.y - 5, pt.x + 5, pt.y + 5, 1.2f);
                    g.drawLine (pt.x - 5, pt.y + 5, pt.x + 5, pt.y - 5, 1.2f);
                }
            };
            wire ({ 0, 158 }, { 520, 150 }); wire ({ 700, 8 }, { 1200, 70 }); wire ({ 6, 395 }, { 16, 745 });
            wire ({ 1190, 160 }, { 1194, 740 }); wire ({ 0, 744 }, { 1200, 746 });
            for (int i = 0; i < 26; ++i)
            {
                const bool edgeX = r.nextBool();
                Point<float> c = edgeX ? Point<float> (r.nextBool() ? r.nextFloat() * 18 : W - r.nextFloat() * 18, r.nextFloat() * H)
                                       : Point<float> (r.nextFloat() * W, r.nextBool() ? 160 + r.nextFloat() * 8 : 396 + r.nextFloat() * 4);
                drawStar (g, c, 4 + r.nextFloat() * 8, s.deco.withAlpha (0.8f), 0.12f);
            }
            g.setColour (s.textDim); g.setFont (serif (12.0f, false, 0.05f));
            g.drawText ("SOUNDS FOR A DARKER TOMORROW.", Rectangle<float> (930, 18, 250, 20), Justification::centredRight);
        }
        else
        {
            // liquid chrome highlights
            for (int i = 0; i < 26; ++i)
            {
                Path p; const float y0 = r.nextFloat() * H * 1.2f - 60;
                p.startNewSubPath (-20, y0);
                for (int x = 0; x <= W + 40; x += 60)
                    p.lineTo ((float) x, y0 + std::sin ((float) x * 0.008f + (float) i) * 40 + (float) x * 0.15f);
                g.setColour ((r.nextBool() ? Colours::white : Colour (0xff6d7580)).withAlpha (0.08f + r.nextFloat() * 0.12f));
                g.strokePath (p, PathStrokeType (6 + r.nextFloat() * 28, PathStrokeType::curved, PathStrokeType::rounded));
            }
            // diamonds + sparkles on edges only
            for (int i = 0; i < 40; ++i)
            {
                Point<float> c;
                switch (r.nextInt (4))
                {
                    case 0:  c = { r.nextFloat() * 16, r.nextFloat() * H }; break;
                    case 1:  c = { W - r.nextFloat() * 16, r.nextFloat() * H }; break;
                    case 2:  c = { 540 + r.nextFloat() * 660, r.nextFloat() * 14 }; break;
                    default: c = { r.nextFloat() * W, H - r.nextFloat() * 8 }; break;
                }
                const float sz = 5 + r.nextFloat() * 12;
                Path d; d.addPolygon (c, 4, sz, 0.3f);
                g.setGradientFill (ColourGradient (Colours::white, c.x - sz, c.y - sz, Colour (0xff9fd2ff), c.x + sz, c.y + sz, false));
                g.fillPath (d);
                g.setColour (Colour (0xff6f8fb0)); g.strokePath (d, PathStrokeType (0.8f));
                if (r.nextFloat() < 0.4f) drawStar (g, c + Point<float> (sz, -sz), sz * 0.8f, Colours::white, 0.1f);
            }
        }
    }

    void drawLogo (Graphics& g)
    {
        const auto& s = *lnf.skin;
        GlyphArrangement ga;
        ga.addLineOfText (serif (96.0f, true, 0.02f), "KEYS KILLA", 44, 128);
        Path p; ga.createPath (p);
        p.applyTransform (AffineTransform::shear (-0.18f, 0));
        p.scaleToFit (48, 44, 470, 86, false);
        // spikes on the logo (blackletter feel)
        Random r (5);
        auto pb = p.getBounds();
        Path spikes;
        for (int i = 0; i < 18; ++i)
        {
            const float x = pb.getX() + 10 + r.nextFloat() * (pb.getWidth() - 20);
            const bool up = r.nextBool();
            const float y = up ? pb.getY() + 6 : pb.getBottom() - 6;
            spikes.addTriangle (x - 3, y, x + 3, y, x + (r.nextFloat() - 0.5f) * 10, y + (up ? -16.0f : 16.0f) * (0.5f + r.nextFloat()));
        }
        p.addPath (spikes);
        DropShadow (s.dark ? s.accent.withAlpha (0.9f) : Colour (0xff4a90d9).withAlpha (0.55f), 14, {}).drawForPath (g, p);
        ColourGradient chrome (Colours::white, 0, pb.getY(), Colour (0xff2b2f35), 0, pb.getBottom(), false);
        chrome.addColour (0.45, Colour (0xffc9ced4));
        chrome.addColour (0.52, Colour (0xff50565e));
        chrome.addColour (0.7, Colour (0xffe9ecef));
        g.setGradientFill (chrome);
        g.fillPath (p);
        g.setColour (s.dark ? Colour (0xff3a0000) : Colour (0xff1f2328));
        g.strokePath (p, PathStrokeType (1.4f));
        g.setColour (s.text);
        g.setFont (serif (13.0f, false, 0.45f));
        g.drawText ("RARE & MODERN TRAP MELODIES & BASSES", Rectangle<float> (60, 138, 480, 20), Justification::centred);
    }

    // ---------- preset browsing ----------
    std::vector<int> filtered() const
    {
        std::vector<int> out;
        const auto& ps = factoryPresets();
        for (int i = 0; i < (int) ps.size(); ++i)
        {
            const auto& p = ps[(size_t) i];
            if (proc.uiExclusive && ! p.exclusive) continue;
            if (! proc.uiExclusive && proc.uiTile >= 0 && p.tile != proc.uiTile) continue;
            if (proc.uiEra >= 0 && p.era != proc.uiEra) continue;
            out.push_back (i);
        }
        return out;
    }

    int tileOfCurrent() const
    {
        const int i = proc.currentPresetIndex();
        return i >= 0 ? factoryPresets()[(size_t) i].tile : tLeads;
    }

    void loadFirstMatching()
    {
        auto list = filtered();
        if (list.empty() && proc.uiEra >= 0)   // nothing in this era: ignore the era for this category
        {
            const int keep = proc.uiEra; proc.uiEra = -1; list = filtered(); proc.uiEra = keep;
        }
        if (! list.empty()) proc.loadPreset (list.front());
        refreshState();
    }

    void step (int dir)
    {
        auto list = filtered();
        if (list.empty()) { list.resize (factoryPresets().size()); std::iota (list.begin(), list.end(), 0); }
        const int cur = proc.currentPresetIndex();
        auto it = std::find (list.begin(), list.end(), cur);
        int pos = it == list.end() ? (dir > 0 ? -1 : 0) : (int) std::distance (list.begin(), it);
        pos = (pos + dir + (int) list.size()) % (int) list.size();
        proc.loadPreset (list[(size_t) pos]);
        refreshState();
    }

    void mouseDown (const MouseEvent& e) override
    {
        if (presetR.contains (e.position) && ! heartBtn.getBounds().contains (e.getPosition()))
            browser->open (proc.uiExclusive ? -1 : proc.uiTile, proc.uiEra, proc.uiExclusive);
    }

    StringArray favourites() const { return StringArray::fromTokens (settings->getValue ("favourites"), "|", ""); }
    void toggleFavourite() { toggleFavouriteNamed (proc.currentName()); }
    void toggleFavouriteNamed (const String& n)
    {
        auto favList = favourites();
        if (favList.contains (n)) favList.removeString (n); else favList.add (n);
        favList.removeEmptyStrings();
        settings->setValue ("favourites", favList.joinIntoString ("|"));
        refreshState();
    }

    void askName (const String& title, const String& initial, std::function<void (const String&)> done)
    {
        auto* w = new AlertWindow (title, "Preset name:", MessageBoxIconType::NoIcon, this);
        w->addTextEditor ("name", initial);
        w->addButton ("OK", 1, KeyPress (KeyPress::returnKey));
        w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([w, done] (int r)
        {
            const auto name = w->getTextEditorContents ("name").trim();
            if (r == 1 && name.isNotEmpty()) done (name);
        }), true);
    }

    void savePresetAs()
    {
        askName ("Save preset as", proc.currentName().upToFirstOccurrenceOf (" *", false, false), [this] (const String& name)
        {
            proc.saveUserPreset (KeysKillaProcessor::userPresetDir().getChildFile (File::createLegalFileName (name) + ".kkpreset"));
            refreshState();
        });
    }

    void savePreset()   // SAVE: overwrite the current user preset, factory presets are read-only -> Save As
    {
        if (proc.currentUserFile().existsAsFile()) { proc.saveUserPreset (proc.currentUserFile()); refreshState(); }
        else savePresetAs();
    }

    void showMenu()
    {
        const bool user = proc.currentUserFile().existsAsFile();
        PopupMenu m, size, packs;
        m.addSectionHeader ("PRESET");
        m.addItem (1, "Save", true);
        m.addItem (2, "Save As...");
        m.addItem (3, "Rename...", user);
        m.addItem (4, "Delete", user);
        m.addItem (5, "Revert", proc.isModified());
        m.addItem (6, "Init patch");
        m.addItem (7, "Browse presets...");
        packs.addItem (20, "Import preset pack (.zip or folder)...");
        packs.addItem (21, "Export user presets as pack (.zip)...");
        packs.addItem (22, "Show user preset folder");
        m.addSubMenu ("Preset packs", packs);
        m.addSectionHeader ("EDIT");
        m.addItem (8, "Undo", proc.undoManager.canUndo());
        m.addItem (9, "Redo", proc.undoManager.canRedo());
        m.addItem (12, String ("Switch to ") + (proc.currentAB() == 0 ? "B" : "A") + "  (now " + (proc.currentAB() == 0 ? "A" : "B") + ")");
        m.addItem (13, String ("Copy ") + (proc.currentAB() == 0 ? "A > B" : "B > A"));
        m.addItem (14, "Undo last DICE roll", ! proc.diceHistoryNames().isEmpty());
        m.addSeparator();
        m.addItem (15, "ADVANCED page...");
        m.addItem (16, "Eco mode (lower CPU)", true, proc.eco.load());
        m.addItem (10, "Skin: CHROME (light)", true, skinIndex == 0);
        m.addItem (11, "Skin: BLOOD (dark)", true, skinIndex == 1);
        for (int pct : { 100, 125, 150, 175, 200 }) size.addItem (100 + pct, String (pct) + " %");
        m.addSubMenu ("Window size", size);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (menuBtn), [this] (int r)
        {
            switch (r)
            {
                case 1: savePreset(); break;
                case 2: savePresetAs(); break;
                case 3: askName ("Rename preset", proc.currentName(), [this] (const String& n) { proc.renameUserPreset (n); refreshState(); }); break;
                case 4:
                    AlertWindow::showOkCancelBox (MessageBoxIconType::WarningIcon, "Delete preset", "Delete \"" + proc.currentName() + "\"?", "Delete", "Cancel", this,
                                                  ModalCallbackFunction::create ([this] (int ok) { if (ok) proc.deleteUserPreset(); refreshState(); }));
                    break;
                case 5: proc.revert(); break;
                case 6: proc.initPatch(); break;
                case 7: browser->open (-1, -1, false); break;
                case 8: proc.undoManager.undo(); break;
                case 9: proc.undoManager.redo(); break;
                case 12: proc.switchAB(); break;
                case 13: proc.copyAtoB(); break;
                case 14: proc.undoDice(); break;
                case 15: advanced->setVisible (true); advanced->toFront (false); break;
                case 16: proc.eco = ! proc.eco.load(); break;
                case 10: case 11: setSkin (r - 10); break;
                case 20:
                    chooser = std::make_unique<FileChooser> ("Import preset pack", File::getSpecialLocation (File::userDocumentsDirectory), "*.zip");
                    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::canSelectDirectories,
                                          [this] (const FileChooser& fc)
                                          {
                                              if (fc.getResult() == File()) return;
                                              const int n = proc.importPack (fc.getResult());
                                              AlertWindow::showMessageBoxAsync (MessageBoxIconType::InfoIcon, "Import", String (n) + " presets imported.", "OK", this);
                                          });
                    break;
                case 21:
                    chooser = std::make_unique<FileChooser> ("Export preset pack", File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("KEYS KILLA presets.zip"), "*.zip");
                    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                                          [this] (const FileChooser& fc) { if (fc.getResult() != File()) proc.exportPack (fc.getResult().withFileExtension ("zip")); });
                    break;
                case 22: KeysKillaProcessor::userPresetDir().startAsProcess(); break;
                default:
                    if (r > 100) setWindowSize (r - 100);
                    break;
            }
            refreshState();
        });
    }

    void setSkin (int idx) { applySkin (idx); settings->setValue ("skin", skinIndex); repaintAll(); }
    void setWindowSize (int pct)
    {
        if (auto* ed = findParentComponentOfClass<AudioProcessorEditor>())
            ed->setSize (KeysKillaEditor::designW * pct / 100, KeysKillaEditor::designH * pct / 100);
    }

    void showDiceMenu()
    {
        PopupMenu m, hist, locks;
        m.addItem (1, "Undo last roll", ! proc.diceHistoryNames().isEmpty());
        const auto names = proc.diceHistoryNames();
        for (int i = names.size(); --i >= 0;) hist.addItem (100 + i, "Back to: " + names[i]);
        m.addSubMenu ("History (last 20)", hist, ! names.isEmpty());
        for (int i = 0; i < KeysKillaProcessor::numLocks; ++i) locks.addItem (200 + i, String ("Lock ") + KeysKillaProcessor::lockName (i), true, proc.diceLocks[(size_t) i]);
        m.addSubMenu ("Locks", locks);
        m.addItem (2, "Save this sound as preset...");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (dice), [this] (int r)
        {
            if (r == 1) proc.undoDice();
            else if (r == 2) savePresetAs();
            else if (r >= 200) proc.diceLocks[(size_t) (r - 200)] = ! proc.diceLocks[(size_t) (r - 200)];
            else if (r >= 100) proc.restoreDice (r - 100);
            refreshState();
        });
    }

    void refreshState()
    {
        presetLabel.setFont (serif (19.0f, false, 0.12f));
        presetLabel.setText (proc.currentName() + (modified ? " *" : ""), dontSendNotification);
        presetLabel.setColour (Label::textColourId, lnf.skin->text);
        const int cur = proc.currentPresetIndex();
        const int shownTile = proc.uiTile >= 0 ? proc.uiTile : (cur >= 0 && ! proc.uiExclusive ? factoryPresets()[(size_t) cur].tile : -1);
        for (int i = 0; i < numTiles; ++i) { tiles[(size_t) i]->selected = (i == shownTile) && ! proc.uiExclusive; tiles[(size_t) i]->repaint(); }
        exclusiveBtn.setToggleState (proc.uiExclusive, dontSendNotification);
        era.selected = proc.uiEra; era.repaint();
        auto favList = StringArray::fromTokens (settings->getValue ("favourites"), "|", "");
        isFav = favList.contains (proc.currentName());
        heartBtn.repaint();
        const bool bass = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f;
        static const char* normal[] { "TONE", "SPACE", "DIRT", "LOFI", "MOVE", "WIDTH" };
        static const char* bassL[] { "SUB", "WOBBLE", "DIRT", "GLIDE", "TONE", "PUNCH" };
        static const char* tipsN[] { "Darker / brighter", "Reverb and delay amount", "Distortion and saturation", "Tape wobble, vinyl and bitcrush",
                                     "Filter movement, chorus and vibrato", "Stereo width and detune" };
        static const char* tipsB[] { "Clean sine sub layer an octave down", "Tempo-synced filter wobble", "Distortion above the low end only",
                                     "Slide time between notes", "Darker / brighter", "Punchy pitch click on the attack" };
        const auto custom = proc.macroNames();
        for (int i = 0; i < 6; ++i)
        {
            macros[(size_t) i].label.setText (custom.size() == 6 ? custom[i] : String (bass ? bassL[i] : normal[i]), dontSendNotification);
            macros[(size_t) i].label.setFont (serif (20.0f, false, 0.25f));
            macros[(size_t) i].slider.setTooltip (bass ? tipsB[i] : tipsN[i]);
        }
        repaint (presetR.toNearestInt().expanded (4));
    }

    void timerCallback() override
    {
        const float l = proc.meterL.exchange (0.0f), r = proc.meterR.exchange (0.0f);
        meter.l = std::max (l, meter.l * 0.82f); meter.r = std::max (r, meter.r * 0.82f);
        if (proc.overload.exchange (false)) warnHold = 45;
        meter.warn = warnHold > 0 && proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f;
        if (warnHold > 0) --warnHold;
        meter.repaint();

        const bool bassNow = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f;
        if (++slowTick % 10 == 0) modifiedNow = proc.isModified();
        if (proc.currentName() != lastName || proc.currentPresetIndex() != lastIndex || modifiedNow != modified || bassNow != lastBass)
        {
            lastName = proc.currentName(); lastIndex = proc.currentPresetIndex(); modified = modifiedNow; lastBass = bassNow;
            refreshState();
        }
        if (! ModifierKeys::currentModifiers.isAnyMouseButtonDown()) proc.undoManager.beginNewTransaction();
        const float lockHash = proc.apvts.getRawParameterValue (ID::keyLock)->load() * 1000 + proc.apvts.getRawParameterValue (ID::key)->load() * 10
                             + proc.apvts.getRawParameterValue (ID::scale)->load();
        if (lockHash != lastLockHash) { lastLockHash = lockHash; keyboard.repaint(); }
        uint64_t hash = 0;
        for (size_t i = 0; i < 128; ++i) if (proc.playing[i].load()) hash = hash * 131 + i + 1;
        if (hash != lastPlayHash) { lastPlayHash = hash; keyboard.repaint(); }
    }

    KeysKillaProcessor& proc;
    KKLookAndFeel lnf;
    std::unique_ptr<PropertiesFile> settings;
    int skinIndex = 0;
    Image bg;

    std::vector<std::unique_ptr<TileButton>> tiles;
    EraTimeline era;
    TextButton exclusiveBtn, prevBtn, nextBtn, saveBtn, menuBtn, chordBtn, arpBtn;
    GlyphButton skinBtn { lnf, true }, heartBtn { lnf, false };
    Label presetLabel, chaosLabel, pitchLabel, modLabel;
    std::array<Knob, 6> macros;
    std::array<Knob, 3> exKnobs;
    Slider chaos, pitchWheel, modWheel;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> chaosAtt;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> chordAtt, arpAtt;
    DiceButton dice;
    XYPad xy;
    Meter meter;
    KKKeyboard keyboard;
    std::unique_ptr<AdvancedPage> advanced;
    std::unique_ptr<FileChooser> chooser;

    Rectangle<float> browserR, exclusiveR, macroR, playR, meterR, wheelR, keyR, presetR;
    bool isFav = false;
    int warnHold = 0, lastIndex = -2, slowTick = 0;
    bool modified = false, modifiedNow = false, lastBass = false;
    float lastLockHash = -1;
    std::unique_ptr<PresetBrowser> browser;
    String lastName;
    uint64_t lastPlayHash = 0;
};

//==============================================================================
KeysKillaEditor::KeysKillaEditor (KeysKillaProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    page = std::make_unique<MainPage> (p);
    addAndMakeVisible (*page);
    setResizable (true, true);
    setResizeLimits (designW, designH, designW * 2, designH * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) designW / designH);
    setSize (designW, designH);
}

KeysKillaEditor::~KeysKillaEditor() = default;

void KeysKillaEditor::showView (int v) { page->showView (v); }

void KeysKillaEditor::resized()
{
    page->setTransform (AffineTransform::scale ((float) getWidth() / designW));
    page->setBounds (0, 0, designW, designH);
}
