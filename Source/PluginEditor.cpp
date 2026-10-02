#include "PluginEditor.h"
#include <numeric>
#include <map>

using namespace juce;

#include "UiCommon.h"
#include "BinaryData.h"
#include "EmbeddedPlugins.h"
#include "ModernSkin.h"

//==============================================================================
void KKLookAndFeel::setSkin (const Skin& s)
{
    skin = &s;
    setColour (PopupMenu::backgroundColourId, s.dark ? Colour (0xff15132e) : Colour (0xffeef0f3));
    setColour (PopupMenu::textColourId, s.text);
    setColour (PopupMenu::highlightedBackgroundColourId, s.accent.withAlpha (0.8f));
    setColour (PopupMenu::highlightedTextColourId, s.dark ? Colours::white : Colours::black);
    setColour (PopupMenu::headerTextColourId, s.accent);
    setColour (Label::textColourId, s.text);
    setColour (TooltipWindow::backgroundColourId, s.dark ? Colour (0xff1c1a3a) : Colour (0xfff2f4f6));
    setColour (TooltipWindow::textColourId, s.text);
    setColour (TooltipWindow::outlineColourId, s.accent);
    setColour (ComboBox::backgroundColourId, s.dark ? Colour (0xff1c1a3a) : Colour (0xfff5f6f8));
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
    setColour (TextEditor::backgroundColourId, s.dark ? Colour (0xff131130) : Colour (0xfff7f8fa));
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
    g.setColour (s.dark ? Colour (0xff12102a) : Colour (0xffe9ecef));
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

Font KKLookAndFeel::getTextButtonFont (TextButton&, int h) { return serif (jmin (26.0f, (float) h * 0.52f), false, 0.2f); }
Font KKLookAndFeel::getComboBoxFont (ComboBox& c) { return serif (jmin (24.0f, (float) c.getHeight() * 0.62f), false, 0.08f); }
Font KKLookAndFeel::getLabelFont (Label& l)
{
    if (dynamic_cast<Slider*> (l.getParentComponent()) != nullptr)   // knob value boxes scale with their box
        return serif (jmax (11.0f, (float) l.getHeight() * 0.72f), false, 0.2f);
    return l.getFont();
}
Font KKLookAndFeel::getPopupMenuFont() { return serif (25.0f, false, 0.05f); }   // menus scale with the 60 % window



//==============================================================================
// BREED LAB skin: one bitmap made from the design (tools/make_breed_assets.py), live parts drawn on top.
struct LabImages { Image bg, white, black, icons; };

// Images live only while an editor is open: on Windows they are native images and must be released
// before the host shuts its graphics down (static images froze FL Studio on exit).
struct LabImageCache
{
    LabImages imgs;
    LabImageCache()
    {
        auto load = [] (const void* d, int n) { return ImageFileFormat::loadFrom (d, (size_t) n); };
        // v0.26: the clean graphite look, drawn in code (the old horror bitmap is gone)
        imgs.bg = kk::modern::makeBackground();
        imgs.white = kk::modern::makeWhiteKey();
        imgs.black = kk::modern::makeBlackKey();
        ignoreUnused (load);
        imgs.icons = load (BinaryData::lab_icons_png, BinaryData::lab_icons_pngSize);
    }
};
static LabImageCache*& activeLabCache() { static LabImageCache* c = nullptr; return c; }
struct LabCacheHolder
{
    SharedResourcePointer<LabImageCache> cache;
    LabCacheHolder() { activeLabCache() = &cache.get(); }
    ~LabCacheHolder() { if (cache.getReferenceCount() <= 1) activeLabCache() = nullptr; }
};
static const LabImages& labImages()
{
    static LabImages none;
    auto* c = activeLabCache();
    return c != nullptr ? c->imgs : none;
}

static Rectangle<int> R (int x0, int y0, int x1, int y1) { return { x0, y0, x1 - x0, y1 - y0 }; }

// keep the host's keyboard shortcuts (FL: space = play) - nothing in the plugin takes focus
static void noFocus (Component& c)
{
    c.setWantsKeyboardFocus (false);
    c.setMouseClickGrabsKeyboardFocus (false);
    for (auto* ch : c.getChildren())
        if (dynamic_cast<TextEditor*> (ch) == nullptr) noFocus (*ch);
}

// maker tag: "by TrapVST"
static void drawMaker (Graphics& g, Rectangle<float> r, Justification j)
{
    const Font f1 (FontOptions (10.0f)), f2 (FontOptions (11.5f, Font::bold));
    GlyphArrangement ga; ga.addLineOfText (f1, "by ", 0, 0); const float w1 = ga.getBoundingBox (0, -1, true).getWidth();
    GlyphArrangement gb; gb.addLineOfText (f2.withExtraKerningFactor (0.06f), "TrapVST", 0, 0); const float w2 = gb.getBoundingBox (0, -1, true).getWidth();
    const float tw = w1 + w2;
    float x = j.testFlags (Justification::horizontallyCentred) ? r.getCentreX() - tw * 0.5f : j.testFlags (Justification::right) ? r.getRight() - tw : r.getX();
    g.setColour (Colour (0xffaaa4cf)); g.setFont (f1); g.drawText ("by", Rectangle<float> (x, r.getY(), w1, r.getHeight()), Justification::centredLeft);
    g.setGradientFill (ColourGradient (Colour (0xffff8a3d), x + w1, 0, Colour (0xffff3fd2), x + tw, 0, false));
    g.setFont (f2.withExtraKerningFactor (0.06f)); g.drawText ("TrapVST", Rectangle<float> (x + w1, r.getY(), w2 + 4, r.getHeight()), Justification::centredLeft);
}
static void drawGlowFrame (Graphics& g, Rectangle<float> r, Colour accent, float corner = 5.0f)
{
    g.setColour (accent.withAlpha (0.18f)); g.drawRoundedRectangle (r.expanded (3), corner + 3, 6.0f);
    g.setColour (accent.withAlpha (0.35f)); g.drawRoundedRectangle (r.expanded (1), corner + 1, 3.0f);
    g.setColour (accent); g.drawRoundedRectangle (r, corner, 1.8f);
}

static int iconOfCategory (int cat)
{
    //                        Piano Keys Bells Plucks Mallets Guitar Strings Brass Choir Wind Lead Pads Synth Bass 808 Texture Arp FX
    static const int icon[] { 1,    1,   0,    2,     0,      8,     8,      6,    4,    3,   6,   5,   6,    7,   7,  5,      2,  9 };
    return juce::isPositiveAndBelow (cat, (int) numCategories) ? icon[cat] : 9;
}

//==============================================================================
// Knob drawn from the design: the metal cap is cut out of the bitmap and rotated, the value arc is live.
class ImageKnob : public Slider
{
public:
    explicit ImageKnob (KKLookAndFeel& l) : Slider (RotaryHorizontalVerticalDrag, NoTextBox), lnf (l)
    {
        setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        setVelocityModeParameters (0.6, 1, 0.02, true, ModifierKeys::ctrlModifier);
        setPopupDisplayEnabled (true, true, nullptr);
    }
    void place (Point<int> designCentre, int capRadius, int arcRadius)
    {
        centre = designCentre; capR = capRadius; arcR = arcRadius;
        const int half = arcR + 12;
        setBounds (centre.x - half, centre.y - half, half * 2, half * 2);
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const float pos = (float) valueToProportionOfLength (getValue());
        const float ang = MathConstants<float>::pi * (-0.75f + 1.5f * pos);
        const Point<float> c ((float) getWidth() * 0.5f, (float) getHeight() * 0.5f);
        const auto src = labImages().bg.getClippedImage ({ centre.x - capR, centre.y - capR, capR * 2, capR * 2 });
        {
            Graphics::ScopedSaveState ss (g);
            Path clip; clip.addEllipse (c.x - (float) capR, c.y - (float) capR, (float) capR * 2, (float) capR * 2);
            g.reduceClipRegion (clip);
            g.setImageResamplingQuality (Graphics::highResamplingQuality);
            g.drawImageTransformed (src, AffineTransform::translation (-(float) capR, -(float) capR).rotated (ang).translated (c));
        }
        if (pos > 0.002f)
        {
            Path arc; arc.addCentredArc (c.x, c.y, (float) arcR, (float) arcR, 0, -MathConstants<float>::pi * 0.75f, ang, true);
            const float w = capR > 26 ? 1.0f : 0.7f;
            // every knob its own neon pair (pink -> orange, violet -> pink, orange -> pink ...)
            static const Colour pairs[][2] { { Colour (0xffff2f6d), Colour (0xffff8a3d) }, { Colour (0xffff8a3d), Colour (0xffff2f6d) },
                                             { Colour (0xff9b4dff), Colour (0xffff2f6d) }, { Colour (0xffff2f6d), Colour (0xff9b4dff) },
                                             { Colour (0xff4d7dff), Colour (0xff9b4dff) } };
            const auto& pr = pairs[(size_t) ((centre.x / 37 + centre.y / 11) % 5)];
            const ColourGradient grad (pr[0], c.x - (float) arcR, c.y + (float) arcR, pr[1], c.x + (float) arcR, c.y - (float) arcR, false);
            ColourGradient soft = grad; soft.multiplyOpacity (0.18f);
            ColourGradient mid = grad;  mid.multiplyOpacity (0.45f);
            g.setGradientFill (soft); g.strokePath (arc, PathStrokeType (13.0f * w, PathStrokeType::curved, PathStrokeType::rounded));
            g.setGradientFill (mid);  g.strokePath (arc, PathStrokeType (7.0f * w, PathStrokeType::curved, PathStrokeType::rounded));
            g.setGradientFill (grad); g.strokePath (arc, PathStrokeType (3.6f * w, PathStrokeType::curved, PathStrokeType::rounded));
            ignoreUnused (s);
            g.setColour (Colours::white.withAlpha (0.5f)); g.strokePath (arc, PathStrokeType (1.0f * w));
        }
    }
    bool hitTest (int x, int y) override { return Point<int> (x, y).getDistanceFrom ({ getWidth() / 2, getHeight() / 2 }) <= arcR + 6; }
private:
    KKLookAndFeel& lnf;
    Point<int> centre; int capR = 30, arcR = 40;
};

//==============================================================================
// Invisible hot-spot button that paints only its live state over the bitmap
class HotButton : public Button
{
public:
    HotButton (KKLookAndFeel& l, String label = {}) : Button (label), lnf (l) {}
    std::function<void (Graphics&, Rectangle<float>, const Skin&)> glyph;
    bool selected = false, round = false, framed = false;   // framed: a visible button (module pages), not a hot-spot over the bitmap
    Colour tint;   // optional own colour (drum tabs): a coloured underline, and its glow when selected
    std::function<void()> onRightClick;

    void paintButton (Graphics& g, bool over, bool down) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (2);
        const float corner = round ? r.getHeight() * 0.5f : 5.0f;
        if (framed && ! selected)
        {
            g.setGradientFill (ColourGradient (Colour (0xff221f44), 0, r.getY(), Colour (0xff14132c), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, corner);
            g.setColour (Colour (0xff3f3870)); g.drawRoundedRectangle (r, corner, 1.2f);
        }
        const bool tinted = ! tint.isTransparent();
        if (selected)
        {
            drawGlowFrame (g, r, tinted ? tint : s.accent, corner);
            g.setGradientFill (tinted ? ColourGradient (tint.withAlpha (0.45f), r.getX(), r.getY(), tint.withAlpha (0.12f), r.getRight(), r.getBottom(), false)
                                      : ColourGradient (Colour (0x55ff2f6d), r.getX(), r.getY(), Colour (0x339b4dff), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, corner);
        }
        else if (tinted && framed)
        {
            const auto bar = r.reduced (r.getWidth() * 0.18f, 0).withTop (r.getBottom() - 3.0f).translated (0, -2.0f);
            g.setGradientFill (ColourGradient (tint.withAlpha (0.0f), bar.getX(), 0, tint, bar.getCentreX(), 0, false)); g.fillRect (bar.withWidth (bar.getWidth() * 0.5f));
            g.setGradientFill (ColourGradient (tint, bar.getCentreX(), 0, tint.withAlpha (0.0f), bar.getRight(), 0, false)); g.fillRect (bar.withTrimmedLeft (bar.getWidth() * 0.5f));
        }
        if (over && ! selected) { g.setColour (s.accent.withAlpha (down ? 0.25f : 0.12f)); g.fillRoundedRectangle (r, corner); }
        if (const auto text = getButtonText(); text.isNotEmpty())
        {
            g.setColour (selected ? Colours::white : Colour (0xffe6e3ff));
            g.setFont (serif (framed ? std::min (r.getHeight() * 0.5f, 20.0f) : r.getHeight() * 0.5f, false, 0.12f));
            g.drawFittedText (text, r.reduced (4, 0).toNearestInt(), Justification::centred, text.containsChar ('\n') ? 2 : 1, 0.7f);
        }
        if (glyph) glyph (g, r, s);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onRightClick) { onRightClick(); return; }
        Button::mouseUp (e);
    }
private:
    KKLookAndFeel& lnf;
};

//==============================================================================
class WheelSlider : public Slider
{
public:
    explicit WheelSlider (KKLookAndFeel& l) : Slider (LinearVertical, NoTextBox), lnf (l) {}
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (5, 8);
        const float py = r.getBottom() - (float) valueToProportionOfLength (getValue()) * r.getHeight();
        g.setColour (s.accent.withAlpha (0.35f)); g.fillRoundedRectangle (Rectangle<float> (r.getWidth() + 4, 9).withCentre ({ r.getCentreX(), py }), 4);
        g.setColour (Colours::white.withAlpha (0.95f)); g.fillRoundedRectangle (Rectangle<float> (r.getWidth(), 4).withCentre ({ r.getCentreX(), py }), 2);
    }
private:
    KKLookAndFeel& lnf;
};

//==============================================================================
// OUTPUT: two horizontal LED bars, -60 ... 0 dB
class MeterOverlay : public Component
{
public:
    explicit MeterOverlay (KKLookAndFeel& l) : lnf (l) { setInterceptsMouseClicks (false, false); }
    float l = 0, r = 0; bool warn = false;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const int segs = 42;
        const float segW = (float) getWidth() / (float) segs;
        for (int ch = 0; ch < 2; ++ch)
        {
            const float y = ch == 0 ? 2.0f : (float) getHeight() - 13.0f;
            const float db = jlimit (-60.0f, 0.0f, Decibels::gainToDecibels (ch == 0 ? l : r, -80.0f));
            const int lit = (int) std::round ((db + 60.0f) / 60.0f * segs);
            for (int i = 0; i < lit; ++i)
            {
                auto seg = Rectangle<float> ((float) i * segW + 1.0f, y, segW - 2.0f, 11.0f);
                const Colour c = (warn && i >= segs - 3) ? Colours::orange : Colour (0xffff2f6d).interpolatedWith (Colour (0xffffa53d), (float) i / (float) segs);
                ignoreUnused (s);
                g.setColour (c.withAlpha (0.3f)); g.fillRect (seg.expanded (1.0f));
                g.setColour (c); g.fillRect (seg);
            }
        }
    }
private:
    KKLookAndFeel& lnf;
};

//==============================================================================
// Keyboard: real piano layout, key textures cut from the design
class KKKeyboard : public MidiKeyboardComponent
{
public:
    KKKeyboard (KeysKillaProcessor& p, KKLookAndFeel& l)
        : MidiKeyboardComponent (p.keyboardState, horizontalKeyboard), proc (p), lnf (l)
    {
        setAvailableRange (24, 91);
        setScrollButtonsVisible (false);
        setOctaveForMiddleC (4);
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
    }
    void paint (Graphics& g) override
    {
        g.setColour (Colour (0xff0d0b20));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 3);
        MidiKeyboardComponent::paint (g);
    }
    void drawWhiteNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour, Colour) override
    {
        const auto& s = *lnf.skin;
        const bool on = isDown || proc.playing[(size_t) note].load();
        auto k = a.reduced (0.8f, 0).withTrimmedBottom (1);
        g.setImageResamplingQuality (Graphics::mediumResamplingQuality);
        g.drawImage (labImages().white, k, RectanglePlacement::stretchToFit);
        g.setColour (Colours::black.withAlpha (0.45f)); g.drawRoundedRectangle (k, 2, 1);
        if (! inScale (note)) { g.setColour (Colours::black.withAlpha (0.3f)); g.fillRect (k); }
        if (on)
        {
            g.setGradientFill (ColourGradient (s.accent.withAlpha (0.95f), 0, k.getY(), s.accent.withAlpha (0.55f), 0, k.getBottom(), false));
            g.fillRect (k.reduced (1.5f, 0));
            g.setColour (Colours::white.withAlpha (0.45f)); g.fillRect (k.reduced (k.getWidth() * 0.35f, 2).withHeight (k.getHeight() * 0.5f));
        }
        else if (isOver) { g.setColour (s.accent.withAlpha (0.18f)); g.fillRect (k); }
        if (isRoot (note)) { g.setColour (s.accent); g.fillEllipse (Rectangle<float> (6, 6).withCentre ({ k.getCentreX(), k.getBottom() - 12 })); }
    }
    void drawBlackNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour) override
    {
        const auto& s = *lnf.skin;
        const bool on = isDown || proc.playing[(size_t) note].load();
        auto k = a.withTrimmedTop (-2);
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (k.translated (1.5f, 2), 2);
        g.drawImage (labImages().black, k, RectanglePlacement::stretchToFit);
        if (on)
        {
            g.setColour (s.accent.withAlpha (0.35f)); g.fillRoundedRectangle (k.expanded (2), 3);
            g.setGradientFill (ColourGradient (s.accent.brighter (0.2f), 0, k.getY(), s.accent.darker (0.2f), 0, k.getBottom(), false));
            g.fillRoundedRectangle (k.reduced (1.5f), 2);
        }
        else if (isOver) { g.setColour (s.accent.withAlpha (0.3f)); g.fillRoundedRectangle (k, 2); }
        if (lockOn() && inScale (note) && ! on) { g.setColour (s.accent.withAlpha (0.7f)); g.fillRect (k.reduced (k.getWidth() * 0.3f, 0).withTop (k.getBottom() - 6)); }
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
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
};

//==============================================================================
// Caption painted over the baked macro label only when it differs (bass mode / preset names)
class MacroCaption : public Component
{
public:
    explicit MacroCaption (KKLookAndFeel& l) : lnf (l) { setInterceptsMouseClicks (false, false); }
    String text; bool custom = false; Point<int> designPos;
    void paint (Graphics& g) override
    {
        if (! custom) return;
        const Colour fill = labImages().bg.getPixelAt (designPos.x - getWidth() / 2 + 3, designPos.y);
        g.setColour (fill); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1), 3);
        g.setColour (Colour (0xffe6e1e1));
        g.setFont (serif ((float) getHeight() * 0.66f, false, 0.08f));
        g.drawText (text, getLocalBounds(), Justification::centred);
        ignoreUnused (lnf);
    }
private:
    KKLookAndFeel& lnf;
};

#include "AdvancedPage.h"
#include "PresetBrowser.h"

//==============================================================================
// Panel frame used by the tab panels
class TabPanel : public Component
{
public:
    TabPanel (KeysKillaProcessor& p, KKLookAndFeel& l, String t, StringArray ids) : proc (p), lnf (l), title (std::move (t)), resetIds (std::move (ids))
    {
        close.setButtonText ("CLOSE");
        close.onClick = [this] { setVisible (false); if (onClose) onClose(); };
        addAndMakeVisible (close);
        reset.setButtonText ("RESET");
        reset.setTooltip ("RESET: put every control of this panel back to how the sound was loaded");
        reset.onClick = [this] { proc.resetParams (resetIds); };
        addAndMakeVisible (reset);
    }
    std::function<void()> onClose;
    void paint (Graphics& g) override
    {
        g.fillAll (Colour (0xf4080606));
        drawPanel (g, getLocalBounds().toFloat().reduced (8), *lnf.skin, title);
    }
    void resized() override
    {
        close.setBounds (getWidth() - 140, 14, 116, 36);
        reset.setBounds (getWidth() - 268, 14, 116, 36);
        layout (getLocalBounds().reduced (24).withTrimmedTop (40));
    }
    virtual void layout (Rectangle<int>) {}
protected:
    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
    String title;
    StringArray resetIds;
    TextButton close, reset;
};

// (v0.14: the ARP and MOVEMENT panels are gone - the FAMILY TREE loops replace the arp, PLAY lives under PARAMS)

//==============================================================================
// BREED LOOPS helpers
// piano roll of a loop: low line darker, riff bright
static void drawLoopRoll (Graphics& g, Rectangle<float> r, const std::vector<kk::LoopNote>& notes, float lenBeats, Colour col, float playBeat = -1.0f)
{
    if (notes.empty()) return;
    // two bands: the riff on top (its own pitch range, so the melody shape reads), the low line below
    int lo[2] { 127, 127 }, hi[2] { 0, 0 };
    for (auto& n : notes) { lo[n.low] = std::min (lo[n.low], n.note); hi[n.low] = std::max (hi[n.low], n.note); }
    const bool both = hi[0] > 0 && hi[1] > 0;
    const Rectangle<float> band[2] { both ? r.withTrimmedBottom (r.getHeight() * 0.3f) : r, both ? r.withTrimmedTop (r.getHeight() * 0.78f) : r };
    for (auto& n : notes)
    {
        const auto& b = band[n.low];
        const float span = (float) std::max (n.low ? 6 : 10, hi[n.low] - lo[n.low]);
        const float nh = std::max (1.5f, std::min (b.getHeight() / (span + 1.0f), 5.0f));
        const float x = r.getX() + r.getWidth() * n.start / lenBeats;
        const float w = std::max (1.5f, r.getWidth() * n.len / lenBeats);
        const float y = b.getBottom() - nh - (float) (n.note - lo[n.low]) / span * (b.getHeight() - nh);
        const bool lit = playBeat >= n.start && playBeat < n.start + n.len;
        g.setColour (lit ? Colours::white : (n.low ? col.withAlpha (0.45f) : col));
        g.fillRect (x, y, w, nh);
    }
    if (playBeat >= 0)
    {
        g.setColour (Colours::white.withAlpha (0.6f));
        g.drawVerticalLine ((int) (r.getX() + r.getWidth() * playBeat / lenBeats), r.getY(), r.getBottom());
    }
}

// drag a loop out of the plugin: FL Studio (and other hosts) take it as a MIDI clip
static void dragLoopOut (KeysKillaProcessor& proc, const KeysKillaProcessor::Genome& g, Component* from)
{
    const auto f = proc.exportLoopMidi (g);
    if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, from);
}

//==============================================================================
// PARENT A / B card: category picture, name, tags
class ParentCard : public Component, public SettableTooltipClient
{
public:
    ParentCard (KeysKillaProcessor& p, KKLookAndFeel& l, int s) : proc (p), lnf (l), slot (s)
    {
        setTooltip ("PARENT " + String (s == 0 ? "A" : "B") + ": click to choose a sound. Arrows: next sound of this category. Dice: random parent.");
    }
    std::function<void()> onClick;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto& pg = proc.parent (slot);
        auto r = getLocalBounds().toFloat();
        const Point<float> c (r.getCentreX(), 96.0f);
        ColourGradient glow (s.accent.withAlpha (0.38f), c.x, c.y + 12, s.accent.withAlpha (0.0f), c.x + 110, c.y + 12, true);
        g.setGradientFill (glow); g.fillEllipse (Rectangle<float> (230, 170).withCentre (c.translated (0, 12)));
        const auto& icons = labImages().icons;
        const int ic = iconOfCategory (pg.valid() ? pg.cat : -1);
        if (icons.isValid())
        {
            g.setImageResamplingQuality (Graphics::highResamplingQuality);
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (Rectangle<float> (150, 156).withCentre (c).toNearestInt());
            g.drawImage (icons, Rectangle<float> (150.0f * 10, 156).withPosition (c.x - 75 - 150.0f * (float) ic, c.y - 78), RectanglePlacement::stretchToFit);
        }
        if (over) { g.setColour (s.accent.withAlpha (0.08f)); g.fillRoundedRectangle (r, 6); }
        g.setColour (Colour (0xffeee8e4));
        g.setFont (serif (29.0f, false, 0.02f));
        g.drawFittedText (pg.valid() ? pg.name : String ("choose a sound"), Rectangle<int> (0, 176, getWidth(), 32), Justification::centred, 1, 0.6f);
        String tags;
        if (pg.valid())
        {
            tags = pg.cat >= 0 ? categoryNames()[pg.cat] : String ("USER");
            if (pg.preset >= 0) tags << "  .  " << factoryPresets()[(size_t) pg.preset].mood.toUpperCase();
            if (pg.gen > 0) tags << "  .  GEN " << pg.gen;
        }
        g.setColour (Colour (0xffc8c4e8));
        g.setFont (serif (15.0f, false, 0.2f));
        g.drawFittedText (tags, Rectangle<int> (0, 208, getWidth(), 20), Justification::centred, 1, 0.7f);
    }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; repaint(); }
    void mouseUp (const MouseEvent& e) override { if (! e.mouseWasDraggedSinceMouseDown() && onClick) onClick(); }
private:
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int slot; bool over = false;
};

//==============================================================================
// The big BREED button: glows on hover, flashes when it breeds
class BreedButton : public Component, public SettableTooltipClient
{
public:
    explicit BreedButton (KKLookAndFeel& l) : lnf (l) { setTooltip ("BREED: make 6 new sounds (children) from PARENT A and PARENT B."); }
    std::function<void()> onBreed;
    float flash = 0;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto c = getLocalBounds().toFloat().getCentre();
        const float a = jlimit (0.0f, 1.0f, (over ? 0.35f : 0.0f) + flash);
        if (a <= 0.0f) return;
        g.setGradientFill (ColourGradient (s.accent.withAlpha (0.55f * a), c.x, c.y, s.accent.withAlpha (0.0f), c.x + 100, c.y, true));
        g.fillEllipse (getLocalBounds().toFloat());
        g.setColour (Colours::white.withAlpha (0.35f * a)); g.drawEllipse (Rectangle<float> (170, 170).withCentre (c), 2.0f);
    }
    bool hitTest (int x, int y) override { return Point<int> (x, y).getDistanceFrom ({ getWidth() / 2, getHeight() / 2 }) <= getWidth() / 2; }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; repaint(); }
    void mouseUp (const MouseEvent& e) override { if (contains (e.getPosition()) && onBreed) { flash = 1.0f; onBreed(); repaint(); } }
private:
    KKLookAndFeel& lnf; bool over = false;
};

//==============================================================================
// CHILD card: waveform of the child's real sound, play, stars
class ChildCard : public Component, public SettableTooltipClient
{
public:
    ChildCard (KeysKillaProcessor& p, KKLookAndFeel& l, int i) : proc (p), lnf (l), index (i) {}
    std::function<void (int)> onMenu;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto& kids = proc.kids();
        const bool has = index < (int) kids.size();
        const bool sel = has && proc.selectedChild() == index;
        auto r = getLocalBounds().toFloat();
        if (sel) drawGlowFrame (g, r.reduced (3), s.accent, 6.0f);
        else if (over && has) { g.setColour (s.accent.withAlpha (0.1f)); g.fillRoundedRectangle (r.reduced (3), 6); }
        if (! has)
        {
            g.setColour (Colour (0x55ffffff)); g.setFont (serif (13.0f, false, 0.2f));
            g.drawText (index == 0 ? "press BREED" : "", Rectangle<float> (12, 44, 130, 50), Justification::centred);
            return;
        }
        const auto& c = kids[(size_t) index];
        // waveform (mirrored bars) - or, in LOOP mode, the child's melody loop
        const Rectangle<float> wv (14, 42, 126, 46);
        const Colour col = sel ? s.accent : Colour (0xffd9b8ff);
        if (proc.mainLoopMode)
        {
            const bool playing = proc.loopIsChild (index);
            drawLoopRoll (g, wv, proc.loopNotes (c.g), (float) proc.loopBars() * 4.0f, col, playing ? proc.loopBeat.load() : -1.0f);
            g.setColour (playing ? s.accent : Colour (0xffd9d4f5)); g.setFont (serif (11.0f, true, 0.2f));
            g.drawText (playing ? "LOOP PLAYING" : "LOOP", Rectangle<float> (14, 24, 126, 14), Justification::centredLeft);
        }
        else
        for (int b = 0; b < 64; ++b)
        {
            const float v = c.waveReady ? std::pow (c.wave[(size_t) b], 0.7f) : 0.04f;
            const float h = std::max (1.0f, v * wv.getHeight() * 0.5f);
            const float x = wv.getX() + (float) b * wv.getWidth() / 64.0f;
            g.setColour ((sel ? s.accent.interpolatedWith (Colour (0xffff8a3d), (float) b / 64.0f) : col.interpolatedWith (Colour (0xffff6aa0), (float) b / 80.0f)).withAlpha (sel ? 0.95f : 0.85f));
            g.fillRect (x, wv.getCentreY() - h, 1.6f, h * 2.0f);
        }
        if (sel) { g.setColour (s.accent.withAlpha (0.15f)); g.fillRect (wv.withHeight (8).withCentre (wv.getCentre())); }
        // stars
        for (int st = 0; st < 5; ++st)
        {
            auto sr = starRect (st);
            drawStar (g, sr.getCentre(), sr.getWidth() * 0.5f, st < c.rating ? s.accent : Colour (0x00000000), st < c.rating ? s.accent : Colour (0x88a09a9a));
        }
    }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; repaint(); }
    void mouseUp (const MouseEvent& e) override
    {
        if (index >= (int) proc.kids().size()) return;
        if (e.mods.isPopupMenu()) { if (onMenu) onMenu (index); return; }
        for (int st = 0; st < 5; ++st) if (starRect (st).expanded (3).contains (e.position)) { proc.rateChild (index, st + 1); repaint(); return; }
        if (e.position.getDistanceFrom ({ 160.0f, 66.0f }) < 20.0f) { proc.playChild (index); repaint(); }   // play button (sound or loop)
        else proc.selectChild (index);
    }
private:
    Rectangle<float> starRect (int i) const { return { 44.0f + (float) i * 18.5f, 97.0f, 15.0f, 15.0f }; }
    static void drawStar (Graphics& g, Point<float> c, float r, Colour fill, Colour line)
    {
        Path p;
        for (int k = 0; k < 10; ++k)
        {
            const float a = MathConstants<float>::pi * (float) k / 5.0f;
            const float rr = (k % 2 == 0) ? r : r * 0.45f;
            const Point<float> pt (c.x + std::sin (a) * rr, c.y - std::cos (a) * rr);
            if (k == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        p.closeSubPath();
        if (! fill.isTransparent()) { g.setColour (fill.withAlpha (0.35f)); g.strokePath (p, PathStrokeType (3.0f)); g.setColour (fill); g.fillPath (p); }
        else { g.setColour (line); g.strokePath (p, PathStrokeType (1.1f)); }
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int index; bool over = false;
};

//==============================================================================
// GENES: A/B switch and lock per gene, showing the selected child
class GeneSwitch : public Component, public SettableTooltipClient
{
public:
    GeneSwitch (KeysKillaProcessor& p, KKLookAndFeel& l, int g) : proc (p), lnf (l), gene (g)
    {
        static const char* what[] { "the sound source (engine, oscillators, layers, macros)", "the envelopes and the attack / punch",
                                    "the grit (drive, crush, tape, circuit, body)", "the room (reverb, delay, width, reverse)",
                                    "the motion (LFOs, mod matrix, chorus, alive)", "the colour (filter, EQ, ghost, era, future)" };
        setTooltip (String (KeysKillaProcessor::geneName (g)) + " = " + what[g] + ". Click A or B: take it from that parent. Lock: every new child keeps it.");
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto& kids = proc.kids();
        const int sel = proc.selectedChild();
        const int src = juce::isPositiveAndBelow (sel, (int) kids.size()) ? kids[(size_t) sel].genes[(size_t) gene] : -1;
        for (int k = 0; k < 2; ++k)
        {
            if (src != k) continue;
            auto hr = half (k).reduced (1.5f, 2.0f);
            g.setColour (s.accent.withAlpha (0.3f)); g.fillRoundedRectangle (hr.expanded (2), 6);
            g.setGradientFill (ColourGradient (s.accent.brighter (0.15f), 0, hr.getY(), s.accent.darker (0.3f), 0, hr.getBottom(), false));
            g.fillRoundedRectangle (hr, 5);
            g.setColour (Colours::white); g.setFont (serif (15.0f, false, 0.0f));
            g.drawText (k == 0 ? "A" : "B", hr, Justification::centred);
        }
        if (proc.geneLocked (gene))
        {
            auto lr = lockRect();
            g.setColour (s.accent.withAlpha (0.35f)); g.fillEllipse (lr.expanded (4));
            g.setColour (s.accent); g.drawRoundedRectangle (lr.reduced (2, 5).translated (0, 3), 2, 2.0f);
            g.drawRoundedRectangle (Rectangle<float> (lr.getCentreX() - 4, lr.getY() + 2, 8, 9), 3, 1.8f);
        }
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (lockRect().expanded (4).contains (e.position)) { proc.toggleGeneLock (gene); getParentComponent()->repaint(); return; }
        const int sel = proc.selectedChild();
        if (sel < 0) return;
        for (int k = 0; k < 2; ++k) if (half (k).contains (e.position)) proc.setChildGene (sel, gene, k);
        if (auto* p = getParentComponent()) p->repaint();
    }
private:
    Rectangle<float> half (int k) const { return { 4.0f + (float) k * 37.0f, 3.0f, 37.0f, 27.0f }; }
    Rectangle<float> lockRect() const { return { 84.0f, 5.0f, 22.0f, 22.0f }; }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int gene;
};

//==============================================================================
// WILD rail: how crazy BREED is allowed to get (safe siblings ... crazy mutants)
class WildRail : public Component, public SettableTooltipClient
{
public:
    WildRail (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        setTooltip ("WILD: how far BREED may go. SAFE = close relatives, CRAZY = genes jump, switches flip, more HYBRID layers and FUTURE.");
    }
    static constexpr float top = 52.0f, bottom = 232.0f;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.setColour (Colour (0xffe6e0dc)); g.setFont (serif (24.0f, false, 0.12f));
        g.drawText ("WILD", Rectangle<float> (0, 4, (float) getWidth(), 30), Justification::centred);
        const float x = 26.0f;
        g.setColour (Colour (0xff5a5252)); g.fillRect (x - 1.0f, top, 2.0f, bottom - top);
        static const char* labels[] { "CRAZY", "WILD", "MIXED", "SOFT", "SAFE" };
        for (int i = 0; i < 5; ++i)
        {
            const float y = top + (bottom - top) * (float) i / 4.0f;
            g.setColour (Colour (0xff8e8686)); g.fillEllipse (Rectangle<float> (9, 9).withCentre ({ x, y }));
            g.drawLine (x + 8, y, x + 18, y, 1.0f);
            g.setColour (Colour (0xffe6e3ff)); g.setFont (serif (15.0f, false, 0.1f));
            g.drawText (labels[i], Rectangle<float> (x + 24, y - 9, 90, 18), Justification::centredLeft);
        }
        const float y = bottom - (bottom - top) * proc.breedWild;
        g.setColour (s.accent.withAlpha (0.7f)); g.fillRect (x - 1.5f, y, 3.0f, bottom - y);
        g.setColour (s.accent.withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (30, 30).withCentre ({ x, y }));
        g.setColour (s.accent.withAlpha (0.5f)); g.fillEllipse (Rectangle<float> (19, 19).withCentre ({ x, y }));
        g.setColour (s.accent); g.fillEllipse (Rectangle<float> (12, 12).withCentre ({ x, y }));
        g.setColour (Colours::white); g.fillEllipse (Rectangle<float> (4, 4).withCentre ({ x, y }));
    }
    void mouseDown (const MouseEvent& e) override { mouseDrag (e); }
    void mouseDrag (const MouseEvent& e) override
    {
        proc.breedWild = jlimit (0.0f, 1.0f, (bottom - e.position.y) / (bottom - top));
        repaint();
    }
private:
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
};

//==============================================================================
// FAMILY TREE: pick up to 4 sounds, BREED in the middle -> 6 new sounds or 6 melody loops
class AncestorCard : public Component, public SettableTooltipClient
{
public:
    AncestorCard (KeysKillaProcessor& p, KKLookAndFeel& l, int s) : proc (p), lnf (l), slot (s)
    {
        setTooltip ("SOUND " + String (s + 1) + ": click to choose a sound. Arrows: next sound of the category. Dice: random. X: empty.");
        for (auto* b : { &prev, &next, &dice, &clear }) { addAndMakeVisible (*b); b->setWantsKeyboardFocus (false); }
        prev.setButtonText ("<"); next.setButtonText (">"); dice.setButtonText ("?"); clear.setButtonText ("X");
        dice.setTooltip ("Random sound"); clear.setTooltip ("Empty this slot");
        prev.onClick = [this] { proc.stepAncestor (slot, -1); changed(); };
        next.onClick = [this] { proc.stepAncestor (slot, 1); changed(); };
        dice.onClick = [this] { proc.randomAncestor (slot); changed(); };
        clear.onClick = [this] { proc.clearAncestor (slot); changed(); };
    }
    std::function<void (int)> onChoose;
    std::function<void()> onChanged;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto& an = proc.ancestor (slot);
        auto r = getLocalBounds().toFloat().reduced (2);
        g.setColour (Colour (an.valid() ? 0xff181634 : 0xff110f26)); g.fillRoundedRectangle (r, 8);
        if (an.valid()) drawGlowFrame (g, r, s.accent.withAlpha (over ? 1.0f : 0.55f), 8);
        else
        {
            Path o; o.addRoundedRectangle (r, 8); const float dash[] { 6, 5 };
            PathStrokeType (1.2f).createDashedStroke (o, o, dash, 2);
            g.setColour (over ? s.accent : Colour (0xff4a4478)); g.fillPath (o);
        }
        g.setColour (Colour (0xffe6e3ff)); g.setFont (serif (15.0f, false, 0.3f));
        g.drawText ("SOUND " + String (slot + 1), Rectangle<float> (14, 8, 200, 20), Justification::centredLeft);
        const auto& icons = labImages().icons;
        if (an.valid() && icons.isValid())
        {
            const Point<float> c (58.0f, r.getCentreY() + 10.0f);
            Graphics::ScopedSaveState ss (g);
            g.reduceClipRegion (Rectangle<float> (80, 84).withCentre (c).toNearestInt());
            const int ic = iconOfCategory (an.cat);
            g.drawImage (icons, Rectangle<float> (80.0f * 10, 84).withPosition (c.x - 40 - 80.0f * (float) ic, c.y - 42), RectanglePlacement::stretchToFit);
        }
        auto text = Rectangle<int> (108, 34, getWidth() - 120, 56);
        g.setColour (Colour (an.valid() ? 0xffeee8e4 : 0x88eee8e4)); g.setFont (serif (an.valid() ? 21.0f : 17.0f, false, 0.02f));
        g.drawFittedText (an.valid() ? an.name : String ("click: choose a sound"), text, Justification::centredLeft, 2, 0.7f);
        if (an.valid())
        {
            g.setColour (Colour (0xffa39a9a)); g.setFont (serif (13.0f, false, 0.2f));
            g.drawText (an.cat >= 0 ? categoryNames()[an.cat].toUpperCase() : String ("USER"), Rectangle<int> (108, 90, getWidth() - 120, 18), Justification::centredLeft);
        }
    }
    void resized() override
    {
        const int y = getHeight() - 36, w = 34;
        prev.setBounds (108, y, w, 28); next.setBounds (108 + w + 4, y, w, 28);
        dice.setBounds (getWidth() - 2 * w - 18, y, w, 28); clear.setBounds (getWidth() - w - 12, y, w, 28);
    }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; repaint(); }
    void mouseUp (const MouseEvent& e) override { if (! e.mouseWasDraggedSinceMouseDown() && onChoose) onChoose (slot); }
private:
    void changed() { repaint(); if (onChanged) onChanged(); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int slot; bool over = false;
    TextButton prev, next, dice, clear;
};

class TreeBreedButton : public Component, public SettableTooltipClient
{
public:
    explicit TreeBreedButton (KKLookAndFeel& l) : lnf (l) { setTooltip ("BREED: 6 new results from the chosen sounds - a new mix and new melodies every time."); }
    std::function<void()> onBreed;
    float flash = 0;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto c = getLocalBounds().toFloat().getCentre();
        const float R = (float) std::min (getWidth(), getHeight()) * 0.5f - 6.0f;
        g.setGradientFill (ColourGradient (s.accent.withAlpha (0.35f + 0.4f * flash + (over ? 0.15f : 0.0f)), c.x, c.y, s.accent.withAlpha (0.0f), c.x + R + 6, c.y, true));
        g.fillEllipse (Rectangle<float> (2 * R + 12, 2 * R + 12).withCentre (c));
        g.setColour (Colour (0xff0d0b20)); g.fillEllipse (Rectangle<float> (2 * R, 2 * R).withCentre (c));
        g.setGradientFill (ColourGradient (Colour (0xff9b4dff), c.x - R, c.y - R, Colour (0xffff2f6d), c.x + R, c.y + R, false));
        g.drawEllipse (Rectangle<float> (2 * R - 2, 2 * R - 2).withCentre (c), 2.5f);
        g.setGradientFill (ColourGradient (Colour (0xffff4d6d).brighter (over ? 0.2f : 0.0f), c.x, c.y - R * 0.8f, Colour (0xffff8a3d), c.x, c.y + R * 0.8f, false));
        g.fillEllipse (Rectangle<float> (2 * R * 0.8f, 2 * R * 0.8f).withCentre (c));
        g.setColour (Colours::white.withAlpha (0.22f)); g.fillEllipse (Rectangle<float> (R * 1.0f, R * 0.32f).withCentre (c.translated (0, -R * 0.52f)));
        g.setColour (Colours::white); g.setFont (serif (R * 0.36f, false, 0.12f));
        g.drawText ("BREED", Rectangle<float> (2 * R, R * 0.5f).withCentre (c), Justification::centred);
    }
    bool hitTest (int x, int y) override { return Point<int> (x, y).getDistanceFrom ({ getWidth() / 2, getHeight() / 2 }) <= std::min (getWidth(), getHeight()) / 2; }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; repaint(); }
    void mouseUp (const MouseEvent& e) override { if (contains (e.getPosition()) && onBreed) { flash = 1.0f; onBreed(); repaint(); } }
private:
    KKLookAndFeel& lnf; bool over = false;
};

class ResultCard : public Component, public SettableTooltipClient
{
public:
    ResultCard (KeysKillaProcessor& p, KKLookAndFeel& l, int i) : proc (p), lnf (l), index (i) {}
    std::function<void (int)> onMenu;
    std::function<void()> onChanged;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto& res = proc.treeKids();
        const bool has = index < (int) res.size();
        const bool loopMode = proc.getTreeMode() == KeysKillaProcessor::treeLoop;
        auto r = getLocalBounds().toFloat().reduced (3);
        const bool sel = has && proc.treeSelected() == index;
        g.setColour (Colour (sel ? 0xff221a3e : 0xff15132e)); g.fillRoundedRectangle (r, 7);
        if (sel) drawGlowFrame (g, r, s.accent, 7);
        else { g.setColour (over && has ? s.accent.withAlpha (0.7f) : Colour (0xff3a3264)); g.drawRoundedRectangle (r, 7, 1.2f); }
        g.setColour (Colour (0xffe6e3ff)); g.setFont (serif (15.0f, false, 0.3f));
        g.drawText ((loopMode ? "LOOP " : "SOUND ") + String (index + 1), Rectangle<float> (14, 8, 150, 20), Justification::centredLeft);
        if (! has)
        {
            g.setColour (Colour (0x55ffffff)); g.setFont (serif (14.0f));
            g.drawText (index == 0 ? "press BREED" : "", r, Justification::centred);
            return;
        }
        const auto& t = res[(size_t) index];
        auto area = Rectangle<float> (14, 32, r.getWidth() - 56, r.getHeight() - 76);
        if (loopMode)
        {
            g.setColour (Colour (0xff0e0c22)); g.fillRoundedRectangle (area, 4);
            drawLoopRoll (g, area.reduced (5, 5), proc.loopNotes (t.g), (float) proc.loopBars() * 4.0f, sel ? s.accent : Colour (0xffcfc6c6),
                          proc.loopIsTree (index) ? proc.loopBeat.load() : -1.0f);
        }
        else
            for (int b = 0; b < 64; ++b)
            {
                const float v = t.waveReady ? std::pow (t.wave[(size_t) b], 0.7f) : 0.04f;
                const float h = std::max (1.0f, v * area.getHeight() * 0.5f);
                g.setColour ((sel ? s.accent.interpolatedWith (Colour (0xffff8a3d), (float) b / 64.0f) : Colour (0xffd9b8ff).interpolatedWith (Colour (0xffff6aa0), (float) b / 80.0f)).withAlpha (0.9f));
                g.fillRect (area.getX() + (float) b * area.getWidth() / 64.0f, area.getCentreY() - h, 1.6f, h * 2.0f);
            }
        // play / loop button
        const auto pc = playCentre();
        const bool playing = proc.loopIsTree (index);
        g.setColour (playing ? s.accent : Colour (0xff2a2450)); g.fillEllipse (Rectangle<float> (30, 30).withCentre (pc));
        g.setColour (s.accent.withAlpha (0.8f)); g.drawEllipse (Rectangle<float> (30, 30).withCentre (pc), 1.5f);
        g.setColour (Colours::white);
        if (playing) g.fillRect (Rectangle<float> (10, 10).withCentre (pc));
        else { Path tri; tri.addTriangle (pc.x - 4, pc.y - 7, pc.x - 4, pc.y + 7, pc.x + 8, pc.y); g.fillPath (tri); }
        // name + stars
        g.setColour (Colour (0xffc8c4e8)); g.setFont (serif (13.0f, false, 0.02f));
        auto info = t.g.name;
        if (loopMode) info = String (kk::keyName (proc.effectiveLoopKey (t.g.loop))) + " MIN  .  " + info;
        g.drawFittedText (info, Rectangle<int> (14, (int) r.getBottom() - 42, (int) r.getWidth() - 20, 18), Justification::centredLeft, 1, 0.7f);
        for (int st = 0; st < 5; ++st)
        {
            auto sr = starRect (st);
            g.setColour (st < t.rating ? s.accent : Colour (0xff6a6290));
            if (st < t.rating) g.fillEllipse (sr.reduced (2)); else g.drawEllipse (sr.reduced (2), 1.1f);
        }
    }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; repaint(); }
    void mouseDown (const MouseEvent&) override { dragged = false; }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragged || index >= (int) proc.treeKids().size() || e.getDistanceFromDragStart() < 12) return;
        dragged = true;   // the loop as a MIDI clip
        dragLoopOut (proc, proc.treeKids()[(size_t) index].g, this);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (dragged || index >= (int) proc.treeKids().size()) return;
        if (e.mods.isPopupMenu()) { if (onMenu) onMenu (index); return; }
        for (int st = 0; st < 5; ++st) if (starRect (st).expanded (2).contains (e.position)) { proc.rateTreeResult (index, st + 1); changed(); return; }
        if (e.position.getDistanceFrom (playCentre()) < 20.0f) proc.playTreeResult (index);
        else proc.selectTreeResult (index);
        changed();
    }
private:
    void changed() { if (onChanged) onChanged(); }
    Point<float> playCentre() const { return { (float) getWidth() - 28.0f, (float) getHeight() * 0.5f - 12.0f }; }
    Rectangle<float> starRect (int i) const { return { 14.0f + (float) i * 15.0f, (float) getHeight() - 24.0f, 13.0f, 13.0f }; }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int index; bool over = false, dragged = false;
};

class FamilyTreePanel : public Component, private Timer
{
public:
    FamilyTreePanel (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l), breedBtn (l)
    {
        for (int i = 0; i < KeysKillaProcessor::numAncestors; ++i)
        {
            auto c = std::make_unique<AncestorCard> (p, l, i);
            c->onChoose = [this] (int s) { if (onChoose) onChoose (s); };
            c->onChanged = [this] { refresh(); };
            addAndMakeVisible (*c); cards.push_back (std::move (c));
        }
        for (int i = 0; i < 6; ++i)
        {
            auto c = std::make_unique<ResultCard> (p, l, i);
            c->onMenu = [this] (int k) { if (onResultMenu) onResultMenu (k); };
            c->onChanged = [this] { refresh(); if (onLab) onLab(); };
            c->setTooltip ("Click: load it.  Play: hear it (LOOP: the melody plays in the host tempo).  Stars: rate.  Drag into FL Studio: the melody as MIDI.  Right-click: more.");
            addAndMakeVisible (*c); results.push_back (std::move (c));
        }
        breedBtn.onBreed = [this] { proc.treeBreed(); if (proc.getTreeMode() == KeysKillaProcessor::treeLoop && ! proc.loopPlaying()) proc.playTreeResult (0); refresh(); if (onLab) onLab(); };
        addAndMakeVisible (breedBtn);
        soundBtn.setButtonText ("SOUND"); loopBtn.setButtonText ("LOOP");
        soundBtn.setTooltip ("SOUND: BREED makes 6 new sounds from the chosen sounds.");
        loopBtn.setTooltip ("LOOP: BREED makes 6 melody loops (with 6 new sounds) - a new melody every time. Drag them into FL Studio.");
        soundBtn.onClick = [this] { proc.setTreeMode (KeysKillaProcessor::treeSound); proc.stopLoop(); refresh(); };
        loopBtn.onClick = [this] { proc.setTreeMode (KeysKillaProcessor::treeLoop); refresh(); };
        bars8.setButtonText ("8 BARS"); bars16.setButtonText ("16 BARS");
        bars8.onClick = [this] { proc.setLoopBars (8); refresh(); };
        bars16.onClick = [this] { proc.setLoopBars (16); refresh(); };
        keyBox.addItem ("KEY: AUTO", 1);
        for (int k = 0; k < 12; ++k) keyBox.addItem (String ("KEY: ") + kk::keyName (k), k + 2);
        keyBox.setTooltip ("Key of the loops. AUTO = every loop picks its own key (or the KEY of Key Lock when it is on).");
        keyBox.onChange = [this] { proc.setLoopKey (keyBox.getSelectedId() - 2); refresh(); };
        stopBtn.setButtonText ("STOP"); stopBtn.onClick = [this] { proc.stopLoop(); refresh(); };
        for (Component* c : { (Component*) &soundBtn, (Component*) &loopBtn, (Component*) &bars8, (Component*) &bars16, (Component*) &keyBox, (Component*) &stopBtn })
            addAndMakeVisible (c);
        refresh();
        startTimerHz (24);
    }
    std::function<void (int)> onChoose, onResultMenu;
    std::function<void()> onLab;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.fillAll (Colour (0xff12112a));
        g.setColour (s.panelEdge); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8, 1.4f);
        // family lines: 1 + 2 and 3 + 4 join, both run into BREED, BREED feeds the results
        g.setColour (s.accent.withAlpha (0.5f));
        auto bottomOf = [] (Component& c) { return Point<float> ((float) c.getBounds().getCentreX(), (float) c.getBottom()); };
        const auto bc = breedBtn.getBounds().toFloat();
        const float joinY = (float) cards[0]->getBottom() + 22.0f;
        for (int pr = 0; pr < 2; ++pr)
        {
            auto a = bottomOf (*cards[(size_t) pr * 2]), b = bottomOf (*cards[(size_t) pr * 2 + 1]);
            const float mx = (a.x + b.x) * 0.5f;
            g.drawLine (a.x, a.y, a.x, joinY, 1.6f); g.drawLine (b.x, b.y, b.x, joinY, 1.6f); g.drawLine (a.x, joinY, b.x, joinY, 1.6f);
            Path p; p.startNewSubPath (mx, joinY);
            p.cubicTo (mx, bc.getCentreY(), mx, bc.getCentreY(), pr == 0 ? bc.getX() + 10 : bc.getRight() - 10, bc.getCentreY());
            g.strokePath (p, PathStrokeType (1.6f));
        }
        const float busY = (float) results[0]->getY() - 14.0f;
        g.drawLine (bc.getCentreX(), bc.getBottom() - 6, bc.getCentreX(), busY, 1.6f);
        g.drawLine ((float) results.front()->getBounds().getCentreX(), busY, (float) results.back()->getBounds().getCentreX(), busY, 1.6f);
        for (auto& r : results) g.drawLine ((float) r->getBounds().getCentreX(), busY, (float) r->getBounds().getCentreX(), (float) r->getY() + 3, 1.6f);
        g.setColour (Colour (0xffaaa4cf)); g.setFont (serif (13.0f, false, 0.25f));
        g.drawText (proc.getTreeMode() == KeysKillaProcessor::treeLoop ? "DRAG A LOOP INTO FL STUDIO = MIDI CLIP" : "RIGHT-CLICK A RESULT: USE IT AS PARENT A / B, SAVE IT, BREED ON",
                    Rectangle<int> (20, getHeight() - 22, getWidth() - 40, 18), Justification::centredRight);
    }
    void resized() override
    {
        const int w = getWidth();
        const int cw = (w - 40 - 3 * 18) / 4;
        for (int i = 0; i < 4; ++i) cards[(size_t) i]->setBounds (20 + i * (cw + 18), 12, cw, 140);
        breedBtn.setBounds (w / 2 - 75, 168, 150, 150);
        soundBtn.setBounds (w / 2 - 330, 206, 110, 40); loopBtn.setBounds (w / 2 - 214, 206, 110, 40);
        bars8.setBounds (w / 2 + 104, 206, 100, 40); bars16.setBounds (w / 2 + 208, 206, 110, 40);
        keyBox.setBounds (w / 2 + 104, 252, 214, 36); stopBtn.setBounds (w / 2 - 214, 252, 110, 36);
        const int rw = (w - 40 - 5 * 12) / 6;
        for (int i = 0; i < 6; ++i) results[(size_t) i]->setBounds (20 + i * (rw + 12), 340, rw, getHeight() - 340 - 28);
    }
    void refresh()
    {
        const bool loopMode = proc.getTreeMode() == KeysKillaProcessor::treeLoop;
        soundBtn.setToggleState (! loopMode, dontSendNotification); loopBtn.setToggleState (loopMode, dontSendNotification);
        bars8.setToggleState (proc.loopBars() == 8, dontSendNotification); bars16.setToggleState (proc.loopBars() == 16, dontSendNotification);
        keyBox.setSelectedId (proc.loopKey() + 2, dontSendNotification);
        for (Component* c : { (Component*) &bars8, (Component*) &bars16, (Component*) &keyBox, (Component*) &stopBtn }) c->setVisible (loopMode);
        lastLab = proc.labVersion();
        repaint();
        for (auto& c : cards) c->repaint();
        for (auto& r : results) r->repaint();
    }
private:
    void timerCallback() override
    {
        if (! isVisible()) return;
        if (breedBtn.flash > 0) { breedBtn.flash = std::max (0.0f, breedBtn.flash - 0.08f); breedBtn.repaint(); }
        proc.renderNextThumbnail();
        if (proc.labVersion() != lastLab) { refresh(); return; }
        if (proc.loopPlaying()) for (int i = 0; i < (int) results.size(); ++i) if (proc.loopIsTree (i)) results[(size_t) i]->repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    std::vector<std::unique_ptr<AncestorCard>> cards;
    std::vector<std::unique_ptr<ResultCard>> results;
    TreeBreedButton breedBtn;
    TextButton soundBtn, loopBtn, bars8, bars16, stopBtn;
    ComboBox keyBox;
    int lastLab = -1;
};

// ---------------- modules ("10 in 1"): each tab of the bottom row opens one KILLA plugin inside KEYS KILLA ----------------
static void setParamFromUi (KeysKillaProcessor& p, const char* id, float plain)
{
    if (auto* q = p.apvts.getParameter (id))
    {
        const float v = q->convertTo0to1 (plain);
        if (std::abs (q->getValue() - v) < 1.0e-6f) return;
        q->beginChangeGesture(); q->setValueNotifyingHost (v); q->endChangeGesture();
    }
}

// ---------------- shared building blocks for the module pages ----------------
class ModulePage : public Component, protected Timer
{
public:
    ModulePage (KeysKillaProcessor& p, KKLookAndFeel& l, String t, String sub) : proc (p), lnf (l), title (std::move (t)), subtitle (std::move (sub)) {}
    ~ModulePage() override { stopTimer(); }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.fillAll (Colour (0xff12112a));
        g.setColour (s.panelEdge); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8, 1.4f);
        g.setColour (Colours::white); g.setFont (serif (38.0f, true, 0.3f));
        const int tw = (int) std::ceil (GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 30;
        g.drawText (title, 24, 10, tw, 48, Justification::centredLeft);
        g.setColour (Colour (0xffaaa4cf)); g.setFont (serif (14.0f, false, 0.2f));
        g.drawText (subtitle, 24 + tw + 10, 22, 760, 24, Justification::centredLeft);
        g.setColour (Colour (0xffc8c4e8)); g.setFont (serif (13.0f, false, 0.25f));
        for (auto& k : knobs) g.drawText (k.label, k.slider->getBounds().withY (k.slider->getBottom() - 2).withHeight (18).expanded (12, 0), Justification::centred);
        for (auto& c : chips) if (c.caption.isNotEmpty() && ! c.buttons.empty())
            g.drawText (c.caption, c.buttons.front()->getX() + 2, c.buttons.front()->getY() - 20, 300, 18, Justification::centredLeft);
        paintPage (g);
    }
protected:
    virtual void paintPage (Graphics&) {}
    virtual void refreshPage() {}
    struct Knob { std::unique_ptr<Slider> slider; std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> att; String label; };
    struct Chips { String id, caption; std::vector<std::unique_ptr<HotButton>> buttons; };
    Slider& addKnob (const String& id, const String& label, const String& tip)
    {
        Knob k; k.slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::NoTextBox);
        k.slider->setPopupDisplayEnabled (true, true, nullptr); k.slider->setTooltip (tip);
        auto* prm = proc.apvts.getParameter (id);
        k.slider->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
        k.att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *k.slider);
        k.label = label;
        addAndMakeVisible (*k.slider);
        knobs.push_back (std::move (k));
        return *knobs.back().slider;
    }
    void addChips (const String& id, const String& caption, const StringArray& labels, const String& tip)
    {
        Chips c; c.id = id; c.caption = caption;
        for (int i = 0; i < labels.size(); ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, labels[i]);
            b->setTooltip (tip);
            b->onClick = [this, id, i] { setParamFromUi (proc, id.toRawUTF8(), (float) i); refresh(); };
            b->framed = true; addAndMakeVisible (*b); c.buttons.push_back (std::move (b));
        }
        chips.push_back (std::move (c));
    }
    void layoutKnobs (Rectangle<int> row, int size = 84)
    {
        const int n = (int) knobs.size(); if (n == 0) return;
        const int gap = std::max (0, (row.getWidth() - n * size) / std::max (1, n));
        for (int i = 0; i < n; ++i) knobs[(size_t) i].slider->setBounds (row.getX() + gap / 2 + i * (size + gap), row.getY(), size, size);
    }
    void layoutChips (int idx, Rectangle<int> r, int w)
    {
        auto& c = chips[(size_t) idx];
        for (int i = 0; i < (int) c.buttons.size(); ++i) c.buttons[(size_t) i]->setBounds (r.getX() + i * (w + 4), r.getY(), w, r.getHeight());
    }
    int param (const char* id) const { return (int) proc.apvts.getRawParameterValue (id)->load(); }
    void refresh()
    {
        for (auto& c : chips)
        {
            const int v = (int) proc.apvts.getRawParameterValue (c.id)->load();
            for (int i = 0; i < (int) c.buttons.size(); ++i) { c.buttons[(size_t) i]->selected = i == v; c.buttons[(size_t) i]->repaint(); }
        }
        refreshPage();
        repaint();
    }
    void timerCallback() override { if (isVisible()) tick(); }
    virtual void tick()
    {
        int sig = 0;
        for (auto& c : chips) sig = sig * 7 + (int) proc.apvts.getRawParameterValue (c.id)->load();
        sig = sig * 13 + extraSignature();
        if (sig != lastSig) { lastSig = sig; refresh(); }
    }
    virtual int extraSignature() { return 0; }
    // "the keys play this" switch
    void setupKeysButton (HotButton& b, int mode, const String& what)
    {
        b.framed = true;
        b.setTooltip ("KEYS: your MIDI keyboard / FL piano roll plays " + what + " instead of the KEYS KILLA sound. Tip: one KEYS KILLA per instrument in FL.");
        b.onClick = [this, mode] { setParamFromUi (proc, ID::playMode, param (ID::playMode) == mode ? 0.0f : (float) mode); refresh(); };
        addAndMakeVisible (b);
    }
    void refreshKeysButton (HotButton& b, int mode, const String& what)
    {
        const bool on = param (ID::playMode) == mode;
        b.selected = on; b.setButtonText (on ? "KEYS PLAY " + what : "PLAY " + what + " ON KEYS"); b.repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    String title, subtitle;
    std::vector<Knob> knobs;
    std::vector<Chips> chips;
    int lastSig = -1;
};

// drag a file out of KEYS KILLA (MIDI pattern / WAV) - looks like a button, works by dragging
class DragFileButton : public Component, public SettableTooltipClient
{
public:
    DragFileButton (String t, Colour c) : text (std::move (t)), col (c) { setMouseCursor (MouseCursor::DraggingHandCursor); }
    std::function<File()> makeFile;
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1);
        g.setColour (col.withAlpha (hover ? 0.32f : 0.18f)); g.fillRoundedRectangle (r, 6);
        g.setColour (col); g.drawRoundedRectangle (r, 6, 1.6f);
        // three grip lines: "grab me"
        for (int k = 0; k < 3; ++k) g.fillRect (r.getX() + 12, r.getCentreY() - 6 + k * 5.0f, 14.0f, 2.0f);
        g.setColour (Colours::white); g.setFont (Font (FontOptions (14.0f, Font::bold)).withExtraKerningFactor (0.08f));
        g.drawText (text, r.withTrimmedLeft (30), Justification::centred);
    }
    void mouseEnter (const MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const MouseEvent&) override { hover = false; repaint(); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (fired || e.getDistanceFromDragStart() < 5 || ! makeFile) return;
        fired = true;
        const auto f = makeFile();
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const MouseEvent&) override { fired = false; }
private:
    String text; Colour col;
    bool hover = false, fired = false;
};

// ---------------- SAVE TO FOLDER: one menu everywhere a sound is made (pair children, VST A / B, chops, DIGGA) ----------------
static void askNewFolder (std::function<void (String)> done)
{
    auto* w = new AlertWindow ("NEW FOLDER", "Name of the new folder for your sounds:", MessageBoxIconType::NoIcon);
    w->addTextEditor ("name", "", "Folder");
    w->addButton ("CREATE", 1, KeyPress (KeyPress::returnKey));
    w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
    w->enterModalState (true, ModalCallbackFunction::create ([w, done] (int r)
    {
        const auto name = w->getTextEditorContents ("name").trim();
        if (r == 1 && name.isNotEmpty() && kk::Library::createFolder (name)) done (name);
    }), true);
}
// sounds: one or more sounds to save; done gets a short message ("saved to MOJE ZVUKY")
static void saveToFolderMenu (KeysKillaProcessor& proc, std::vector<kk::PairPtr> sounds, Component* target, std::function<void (String)> done)
{
    sounds.erase (std::remove (sounds.begin(), sounds.end(), nullptr), sounds.end());
    if (sounds.empty()) { if (done) done ("nothing to save yet"); return; }
    const auto folders = kk::Library::folders();
    PopupMenu m;
    m.addSectionHeader (sounds.size() > 1 ? "SAVE " + String ((int) sounds.size()) + " SOUNDS TO" : "SAVE TO");
    m.addItem (1, "> " + proc.lastFolder + "   (last used)");
    m.addSeparator();
    for (int i = 0; i < folders.size(); ++i) m.addItem (10 + i, folders[i]);
    m.addSeparator();
    m.addItem (2, "+ new folder ...");
    auto save = [&proc, sounds, done] (const String& folder)
    {
        int n = 0; for (auto& s : sounds) n += proc.saveToFolder (s, folder).existsAsFile() ? 1 : 0;
        if (done) done (n > 0 ? "saved to " + folder + (n > 1 ? "  (" + String (n) + ")" : String()) : String ("could not save"));
    };
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (target), [save, folders, &proc] (int r)
    {
        if (r == 1) save (proc.lastFolder);
        else if (r == 2) askNewFolder ([save] (String name) { save (name); });
        else if (r >= 10 && r - 10 < folders.size()) save (folders[r - 10]);
    });
}

// ---------------- SAVE TO KIT: the boosted drums into your own drum kit (Documents/KEYS KILLA/Drum Kits/<kit>/...) ----------------
static void saveToKitMenu (KeysKillaProcessor& proc, std::vector<int> drums, Component* target, std::function<void (String)> done)
{
    drums.erase (std::remove_if (drums.begin(), drums.end(), [&proc] (int d) { return ! proc.drum (d).hasSample(); }), drums.end());
    if (drums.empty()) { if (done) done ("load a sound first"); return; }
    const auto kits = kk::Kits::kits();
    PopupMenu m;
    m.addSectionHeader (drums.size() > 1 ? "SAVE " + String ((int) drums.size()) + " DRUMS INTO KIT" : "SAVE INTO KIT");
    m.addItem (1, "> " + proc.lastKit + "   (last used)");
    m.addSeparator();
    for (int i = 0; i < kits.size(); ++i) m.addItem (10 + i, kits[i] + "   (" + String (kk::Kits::count (kits[i])) + " sounds)");
    m.addSeparator();
    m.addItem (2, "+ new drum kit ...");
    auto save = [&proc, drums, done] (const String& kit)
    {
        int n = 0; for (int d : drums) n += proc.saveDrumToKit (d, kit).existsAsFile() ? 1 : 0;
        if (done) done (n > 0 ? "saved into " + kit + (n > 1 ? "  (" + String (n) + ")" : String()) : String ("could not save"));
    };
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (target), [save, kits, &proc] (int r)
    {
        if (r == 1) save (proc.lastKit);
        else if (r == 2)
        {
            auto* w = new AlertWindow ("NEW DRUM KIT", "Name of your drum kit:", MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", "MY DRUM KIT " + String (kits.size() + 1), "Kit");
            w->addButton ("CREATE", 1, KeyPress (KeyPress::returnKey));
            w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
            w->enterModalState (true, ModalCallbackFunction::create ([w, save] (int res)
            {
                const auto name = w->getTextEditorContents ("name").trim();
                if (res == 1 && kk::Kits::createKit (name)) save (name);
            }), true);
        }
        else if (r >= 10 && r - 10 < kits.size()) save (kits[r - 10]);
    });
}

// ---------------- MY SOUNDS: your folders, your sounds (hear, into PAIR, drag into FL, delete) ----------------
class MySoundsPage : public Component, public DragAndDropContainer, private Timer
{
public:
    MySoundsPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        folderModel.page = this; soundModel.page = this;   // before the lists ask for rows
        folderList.setModel (&folderModel); folderList.setRowHeight (30);
        soundList.setModel (&soundModel); soundList.setRowHeight (26);
        for (auto* lb : { &folderList, &soundList }) { lb->setColour (ListBox::backgroundColourId, Colour (0xff100e26)); addAndMakeVisible (*lb); }
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (newBtn, "+ NEW FOLDER", "Create a folder for your sounds", [this] { askNewFolder ([this] (String n) { current = n; reload(); }); });
        btn (renameBtn, "RENAME", "Rename this folder", [this] { renameFolder(); });
        btn (delFolderBtn, "DELETE FOLDER", "Delete this folder with its sounds (they go to the recycle bin)", [this] { deleteFolder(); });
        btn (factoryBtn, "FACTORY SOUNDS", "The sounds that come with KEYS KILLA (kept apart from yours)", [this] { if (onFactory) onFactory(); });
        btn (pairBtn, "INTO PAIR", "This sound into PAIR YOUR OWN (next free slot) - breed it with others", [this] { toPair(); });
        btn (delBtn, "DELETE", "Delete this sound (Del) - it goes to the recycle bin", [this] { deleteSound(); });
        btn (showBtn, "SHOW IN EXPLORER", "Open the folder on your computer", [this] { kk::Library::folder (current).revealToUser(); });
        setWantsKeyboardFocus (true);
        setOpaque (true);
        reload();
        startTimerHz (2);
    }
    std::function<void()> onFactory, onPair;
    void visibilityChanged() override { if (isVisible()) reload(); }
    void paint (Graphics& g) override
    {
        g.fillAll (Colour (0xff12112a));
        g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f), 8, 1.2f);
        g.setColour (Colours::white); g.setFont (Font (FontOptions (24.0f, Font::bold)).withExtraKerningFactor (0.08f));
        g.drawText ("MY SOUNDS", 20, 12, 300, 32, Justification::centredLeft);
        g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (12.0f, Font::bold)));
        g.drawText ("YOUR FOLDERS", folderList.getX(), folderList.getY() - 18, 200, 16, Justification::centredLeft);
        g.drawText (current.toUpperCase() + "  -  " + String (files.size()) + " SOUNDS", soundList.getX(), soundList.getY() - 18, 500, 16, Justification::centredLeft);
        g.drawText ("CLICK = HEAR (AND PLAY ON THE KEYS)    DOUBLE-CLICK = INTO PAIR    DRAG = INTO FL STUDIO    DEL = DELETE",
                    soundList.getX(), soundList.getBottom() + 6, soundList.getWidth(), 16, Justification::centredLeft);
        g.setColour (lnf.skin->accent); g.drawText (note, 330, 20, 700, 20, Justification::centredLeft);
        if (files.isEmpty())
        {
            g.setColour (Colour (0xff7d77a8)); g.setFont (Font (FontOptions (16.0f)));
            g.drawFittedText ("This folder is empty.\nUse SAVE TO FOLDER under PAIR children, VST A / B, CHOP slices or DIGGA.", soundList.getBounds().reduced (20), Justification::centred, 3);
        }
    }
    void resized() override
    {
        const int w = getWidth(), h = getHeight();
        factoryBtn.setBounds (w - 200, 14, 184, 32);
        folderList.setBounds (16, 74, 280, h - 74 - 60);
        newBtn.setBounds (16, h - 50, 136, 34); renameBtn.setBounds (158, h - 50, 64, 34); delFolderBtn.setBounds (226, h - 50, 70, 34);
        renameBtn.setButtonText ("REN."); delFolderBtn.setButtonText ("DEL.");
        soundList.setBounds (312, 74, w - 312 - 16, h - 74 - 82);
        pairBtn.setBounds (w - 16 - 3 * 150 - 12, h - 50, 150, 34); delBtn.setBounds (w - 16 - 2 * 150 - 6, h - 50, 150, 34); showBtn.setBounds (w - 16 - 150, h - 50, 150, 34);
    }
    bool keyPressed (const KeyPress& k) override
    {
        if (k == KeyPress::deleteKey || k == KeyPress::backspaceKey) { deleteSound(); return true; }
        return false;
    }
    // drag a sound out = its WAV file into FL
    bool shouldDropFilesWhenDraggedExternally (const DragAndDropTarget::SourceDetails& d, StringArray& out, bool& canMove) override
    {
        const int row = (int) d.description;
        if (row < 0 || row >= files.size()) return false;
        out.add (files[row].getFullPathName()); canMove = false;
        return true;
    }
private:
    struct FolderModel : public ListBoxModel
    {
        MySoundsPage* page = nullptr;
        int getNumRows() override { return page->folders.size(); }
        void paintListBoxItem (int row, Graphics& g, int w, int h, bool) override
        {
            const bool sel = page->folders[row] == page->current;
            if (sel) { g.setColour (page->lnf.skin->accent.withAlpha (0.35f)); g.fillRoundedRectangle (2, 2, (float) w - 4, (float) h - 4, 4); }
            g.setColour (sel ? Colours::white : Colour (0xffc9ced6)); g.setFont (Font (FontOptions (14.0f, Font::bold)));
            g.drawText (page->folders[row], 12, 0, w - 24, h, Justification::centredLeft);
        }
        void listBoxItemClicked (int row, const MouseEvent&) override { page->current = page->folders[row]; page->proc.lastFolder = page->current; page->reload(); }
    } folderModel;
    struct SoundModel : public ListBoxModel
    {
        MySoundsPage* page = nullptr;
        int getNumRows() override { return page->files.size(); }
        void paintListBoxItem (int row, Graphics& g, int w, int h, bool) override
        {
            const bool sel = row == page->sel;
            if (sel) { g.setColour (page->lnf.skin->accent.withAlpha (0.3f)); g.fillRect (0, 0, w, h); }
            else if (row % 2) { g.setColour (Colour (0xff131130)); g.fillRect (0, 0, w, h); }
            g.setColour (Colours::white.withAlpha (sel ? 1.0f : 0.85f)); g.setFont (Font (FontOptions (13.5f)));
            g.drawText (page->files[row].getFileNameWithoutExtension(), 12, 0, w - 140, h, Justification::centredLeft);
            g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (11.0f)));
            g.drawText (page->files[row].getLastModificationTime().formatted ("%d.%m.%Y"), w - 120, 0, 110, h, Justification::centredRight);
        }
        void listBoxItemClicked (int row, const MouseEvent&) override { page->sel = row; page->proc.auditionFile (page->files[row]); page->soundList.repaint(); page->grabKeyboardFocus(); }
        void listBoxItemDoubleClicked (int row, const MouseEvent&) override { page->sel = row; page->toPair(); }
        var getDragSourceDescription (const SparseSet<int>& rows) override { return rows.isEmpty() ? var() : var (rows[0]); }
    } soundModel;
    void reload()
    {
        folders = kk::Library::folders();
        if (! folders.contains (current)) current = folders.contains (proc.lastFolder) ? proc.lastFolder : folders[0];
        files = kk::Library::sounds (current);
        sel = jlimit (-1, files.size() - 1, sel);
        folderList.updateContent(); soundList.updateContent();
        folderList.repaint(); soundList.repaint();
        sig = signature();
        repaint();
    }
    int signature() const { return files.size() * 7919 + kk::Library::sounds (current).size() * 31 + kk::Library::folders().size(); }
    void timerCallback() override { if (isVisible() && signature() != sig) reload(); }   // sounds saved from other pages show up
    void toPair()
    {
        if (sel < 0 || sel >= files.size()) return;
        int slot = 0; while (slot < kk::PairLab::maxParents && proc.pairParents[(size_t) slot] != nullptr) ++slot;
        if (slot >= kk::PairLab::maxParents) slot = 0;
        if (proc.loadPairParent (slot, files[sel])) { note = files[sel].getFileNameWithoutExtension() + " -> PAIR, SOUND " + String (slot + 1); if (onPair) onPair(); }
        repaint();
    }
    void deleteSound()
    {
        if (sel < 0 || sel >= files.size()) return;
        kk::Library::deleteSound (files[sel]);
        reload();
    }
    void deleteFolder()
    {
        const auto name = current;
        AlertWindow::showOkCancelBox (MessageBoxIconType::WarningIcon, "DELETE FOLDER", "Delete the folder \"" + name + "\" and its " + String (files.size()) + " sounds?\n(They go to the recycle bin.)",
                                      "DELETE", "Cancel", this, ModalCallbackFunction::create ([safe = Component::SafePointer<MySoundsPage> (this), name] (int r)
                                      { if (safe != nullptr && r == 1) { kk::Library::deleteFolder (name); safe->current = {}; safe->reload(); } }));
    }
    void renameFolder()
    {
        auto* w = new AlertWindow ("RENAME FOLDER", "New name:", MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", current, "Folder");
        w->addButton ("RENAME", 1, KeyPress (KeyPress::returnKey));
        w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([w, safe = Component::SafePointer<MySoundsPage> (this), from = current] (int r)
        {
            const auto to = w->getTextEditorContents ("name").trim();
            if (safe != nullptr && r == 1 && kk::Library::renameFolder (from, to)) { safe->current = to; safe->reload(); }
        }), true);
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    ListBox folderList, soundList;
    HotButton newBtn { lnf }, renameBtn { lnf }, delFolderBtn { lnf }, factoryBtn { lnf }, pairBtn { lnf }, delBtn { lnf }, showBtn { lnf };
    StringArray folders;
    Array<File> files;
    String current, note;
    int sel = -1, sig = 0;
};

// ---------------- CHOP / SLICE: DIGGA's sample on a big waveform, movable slices, MPC pads, keys, drag out ----------------
class ChopPanel : public Component, private Timer
{
public:
    ChopPanel (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        static const char* modes[] { "TRANSIENTS", "NOTES", "1/4", "1/8", "1/16", "8", "16" };
        static const char* tips[] { "Cut at every hit - drums, chords", "Cut where the pitch moves - vocal chops, melodies",
                                    "Cut on the project grid: 1/4 notes", "Cut on the project grid: 1/8 notes", "Cut on the project grid: 1/16 notes",
                                    "8 equal slices", "16 equal slices" };
        for (int i = 0; i < 7; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, modes[i]);
            b->framed = true; b->setTooltip (tips[i]);
            b->onClick = [this, i]
            {
                if (i == 0) proc.chop.autoSlice (0);
                else if (i == 1) proc.chop.autoSlice (-1);
                else if (i <= 4) proc.chop.autoSlice (1, proc.lastBpm.load(), i == 2 ? 4 : i == 3 ? 8 : 16);
                else proc.chop.autoSlice (i == 5 ? 8 : 16);
                mode = i; sel = 0; refresh();
            };
            addAndMakeVisible (*b); modeBtns.push_back (std::move (b));
        }
        // per slice: PITCH / REVERSE / VOLUME / PAN / FADE
        auto bar = [this] (Slider& sl, double lo, double hi, double step, std::function<String (double)> txt, const String& tip)
        {
            sl.setSliderStyle (Slider::LinearBar); sl.setRange (lo, hi, step); sl.setTooltip (tip);
            sl.textFromValueFunction = std::move (txt);
            sl.setColour (Slider::trackColourId, lnf.skin->accent.withAlpha (0.45f));
            sl.setColour (Slider::textBoxTextColourId, Colours::white);
            sl.onValueChange = [this] { pushFx(); };
            addAndMakeVisible (sl);
        };
        bar (pitchSl, -24, 24, 1, [] (double v) { return "PITCH " + String (v > 0 ? "+" : "") + String ((int) v); }, "Pitch of this slice in semitones (double-click = 0)");
        bar (volSl, 0, 2, 0.01, [] (double v) { return "VOL " + String (roundToInt (v * 100)) + "%"; }, "Volume of this slice");
        bar (panSl, -1, 1, 0.01, [] (double v) { return std::abs (v) < 0.01 ? String ("PAN C") : "PAN " + String (v < 0 ? "L" : "R") + String (roundToInt (std::abs (v) * 100)); }, "Pan of this slice");
        bar (fadeSl, 0.001, 0.2, 0.001, [] (double v) { return "FADE " + String (roundToInt (v * 1000)) + " ms"; }, "Fade in / out - no clicks when you trigger fast");
        pitchSl.setDoubleClickReturnValue (true, 0); volSl.setDoubleClickReturnValue (true, 1); panSl.setDoubleClickReturnValue (true, 0); fadeSl.setDoubleClickReturnValue (true, 0.004);
        revBtn.setButtonText ("REVERSE"); revBtn.framed = true; revBtn.setTooltip ("Play this slice backwards");
        revBtn.onClick = [this] { revBtn.selected = ! revBtn.selected; revBtn.repaint(); pushFx(); proc.chopPad = sel; };
        addAndMakeVisible (revBtn);
        flipBtn.setButtonText ("FLIP IT"); flipBtn.framed = true;
        flipBtn.setTooltip ("A new trap pattern from your chops - then drag it into FL (FLIP MIDI)");
        flipBtn.onClick = [this] { flipSeed = (uint32_t) Random::getSystemRandom().nextInt (1 << 30); flipFile = proc.chop.flipMidi (proc.lastBpm.load(), flipSeed); note = "new flip ready - drag FLIP MIDI into FL"; repaint(); };
        addAndMakeVisible (flipBtn);
        dragFlip.makeFile = [this] { if (! flipFile.existsAsFile()) flipFile = proc.chop.flipMidi (proc.lastBpm.load(), flipSeed); return flipFile; };
        dragFlip.setTooltip ("Drag the FLIP pattern into FL as MIDI (put KEYS KILLA on PLAY CHOPS ON KEYS)");
        addAndMakeVisible (dragFlip);
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (keysBtn, "", "KEYS: the keyboard / FL piano roll plays the slices - C5 = slice 1, C#5 = slice 2 ...",
             [this] { setParamFromUi (proc, ID::playMode, keysOn() ? 0.0f : (float) KeysKillaProcessor::playChop); refresh(); });
        btn (bankAllBtn, "SAVE ALL", "Every slice into one of your MY SOUNDS folders", [this]
        {
            std::vector<kk::PairPtr> all;
            if (auto c = proc.chop.current()) for (int i = 0; i < c->numSlices(); ++i) all.push_back (sliceSound (i));
            saveToFolderMenu (proc, all, &bankAllBtn, [safe = Component::SafePointer<ChopPanel> (this)] (String msg) { if (safe != nullptr) { safe->note = msg; safe->repaint(); } });
        });
        btn (backBtn, "BACK TO DIGGA", "Back to Digga Killa", [this] { if (onBack) onBack(); });
        btn (pairBtn, "SLICE > PAIR", "This slice into PAIR YOUR OWN (next free slot)", [this] { note = proc.chopToPair (sel) ? "slice " + String (sel + 1) + " is in PAIR" : String(); repaint(); });
        btn (bankBtn, "SAVE SLICE", "This slice into one of your MY SOUNDS folders", [this]
        {
            saveToFolderMenu (proc, { sliceSound (sel) }, &bankBtn, [safe = Component::SafePointer<ChopPanel> (this)] (String msg) { if (safe != nullptr) { safe->note = msg; safe->repaint(); } });
        });
        dragWav.makeFile = [this] { return proc.chop.exportSlice (sel); };
        dragWav.setTooltip ("Drag this slice into FL Studio (or onto the PAIR tile on the left)");
        dragMidi.makeFile = [this] { return proc.chop.exportMidi (proc.lastBpm.load()); };
        dragMidi.setTooltip ("Drag the chop pattern into FL: a MIDI clip that plays the slices in order - move the notes = flip the sample");
        addAndMakeVisible (dragWav); addAndMakeVisible (dragMidi);
        setOpaque (true);
        startTimerHz (30);
    }
    ~ChopPanel() override { stopTimer(); }
    std::function<void()> onBack;
    void visibilityChanged() override { if (isVisible()) { proc.chopFromDigga(); refresh(); } }

    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.fillAll (Colour (0xff0a0a1a));
        g.setColour (Colour (0xff1a1838)); g.fillRect (0, 0, getWidth(), 64);
        g.setColour (s.accent); g.fillEllipse (22, 26, 10, 10);
        g.setColour (Colours::white); g.setFont (Font (FontOptions (26.0f, Font::bold)).withExtraKerningFactor (0.08f));
        g.drawText ("CHOP / SLICE", 40, 12, 260, 40, Justification::centredLeft);
        auto c = proc.chop.current();
        g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (12.0f, Font::bold)));
        g.drawText (c != nullptr ? c->name.toUpperCase() + "   " + String (c->numSlices()) + " SLICES" : String(), 42, 44, 400, 16, Justification::centredLeft);
        // waveform with slices
        const auto w = wave.toFloat();
        g.setColour (Colour (0xff0e0c22)); g.fillRoundedRectangle (w, 8);
        g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (w, 8, 1.2f);
        if (c == nullptr || c->src == nullptr)
        {
            g.setColour (Colours::white.withAlpha (0.8f)); g.setFont (Font (FontOptions (24.0f, Font::bold)));
            g.drawText ("LOAD A SAMPLE IN DIGGA FIRST", wave, Justification::centred);
            return;
        }
        const int len = c->src->getNumSamples();
        auto xOf = [&] (int smp) { return w.getX() + 10 + (w.getWidth() - 20) * (float) smp / (float) std::max (1, len); };
        for (int i = 0; i < c->numSlices(); ++i)
        {
            const float x0 = xOf (c->sliceStart (i)), x1 = xOf (c->sliceEnd (i));
            g.setColour (i == sel ? s.accent.withAlpha (0.22f) : (i % 2 ? Colour (0xff17153a) : Colour (0xff131130)));
            g.fillRect (x0, w.getY() + 2, x1 - x0, w.getHeight() - 4);
        }
        const float mid = w.getCentreY(), half = w.getHeight() * 0.42f;
        for (int k = 0; k < (int) peaks.size(); ++k)
        {
            const float x = w.getX() + 10 + (w.getWidth() - 20) * (float) k / (float) peaks.size();
            const int smp = (int) ((juce::int64) len * k / (juce::int64) peaks.size());
            int si = 0; while (si + 1 < c->numSlices() && c->sliceStart (si + 1) <= smp) ++si;
            g.setColour (si == sel ? s.accent : Colour (0xffc9ced6));
            g.drawVerticalLine ((int) x, mid - peaks[(size_t) k] * half, mid + peaks[(size_t) k] * half);
        }
        for (int i = 0; i < c->numSlices(); ++i)
        {
            const float x = xOf (c->sliceStart (i));
            g.setColour (i == hoverMark ? Colours::white : Colour (0xffffc23d));
            g.fillRect (x - 1.0f, w.getY() + 2, i == 0 ? 1.0f : 2.0f, w.getHeight() - 4);
            if (i > 0) { Path tri; tri.addTriangle (x - 7, w.getY() + 2, x + 7, w.getY() + 2, x, w.getY() + 12); g.fillPath (tri); }
            g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
            g.drawText (String (i + 1) + "  " + MidiMessage::getMidiNoteName (kk::ChopLab::firstNote + i, true, true, 5), Rectangle<float> (x + 4, w.getBottom() - 18, 80, 14), Justification::centredLeft);
        }
        if (const float ph = proc.chop.playhead(); ph >= 0)
        {
            g.setColour (Colours::white); g.drawVerticalLine ((int) (w.getX() + 10 + (w.getWidth() - 20) * ph), w.getY(), w.getBottom());
        }
        g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
        g.drawText ("CLICK = PLAY A SLICE   DRAG A YELLOW MARKER = MOVE IT   DOUBLE-CLICK = NEW CUT   RIGHT-CLICK A MARKER = REMOVE", wave.withY (wave.getBottom() + 4).withHeight (16), Justification::centredLeft);
        // selected slice
        g.setColour (Colours::white); g.setFont (Font (FontOptions (18.0f, Font::bold)));
        if (sel < c->numSlices())
            g.drawText ("SLICE " + String (sel + 1) + "  " + MidiMessage::getMidiNoteName (kk::ChopLab::firstNote + sel, true, true, 5) + "  "
                        + String ((double) (c->sliceEnd (sel) - c->sliceStart (sel)) / c->rate, 2) + " s", selRow.withWidth (240), Justification::centredLeft);
        g.setColour (s.accent); g.setFont (Font (FontOptions (13.0f, Font::bold)));
        g.drawText (note, selRow.withY (selRow.getBottom() + 6).withHeight (18), Justification::centredLeft);
        // MPC pads (16)
        for (int i = 0; i < 16; ++i)
        {
            const auto r = pad (i).toFloat();
            const bool has = i < c->numSlices();
            const float lit = padLit[(size_t) i];
            g.setColour (has ? Colour (0xff1f1d3e).interpolatedWith (s.accent, 0.25f * (i == sel) + 0.6f * lit) : Colour (0xff16152e));
            g.fillRoundedRectangle (r, 8);
            g.setColour (has ? Colour (0xff4a4478) : Colour (0xff1e1c3c)); g.drawRoundedRectangle (r, 8, 1.4f);
            if (! has) continue;
            g.setColour (Colours::white.withAlpha (0.9f)); g.setFont (Font (FontOptions (15.0f, Font::bold)));
            g.drawText (String (i + 1), r.reduced (10, 6).toNearestInt(), Justification::topLeft);
            g.setColour (Colour (0xffb4aed8)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
            g.drawText (MidiMessage::getMidiNoteName (kk::ChopLab::firstNote + i, true, true, 5), r.reduced (10, 6).toNearestInt(), Justification::topRight);
        }
    }
    void resized() override
    {
        const int W = getWidth(), H = getHeight();
        int x = 300;
        for (int i = 0; i < (int) modeBtns.size(); ++i) { const int bw = i == 0 ? 116 : i == 1 ? 74 : 52; modeBtns[(size_t) i]->setBounds (x, 16, bw, 32); x += bw + 5; }
        keysBtn.setBounds (x + 10, 14, 220, 36);
        backBtn.setBounds (W - 196, 14, 182, 36);
        bankAllBtn.setBounds (W - 196 - 146, 14, 136, 36);
        wave = { 14, 78, W - 28, std::max (200, H - 78 - 360) };
        selRow = { 18, wave.getBottom() + 26, W - 36, 40 };
        int cx = selRow.getX() + 250;
        revBtn.setBounds (cx, selRow.getY() + 2, 92, 34); cx += 100;
        for (auto* sl : { &pitchSl, &volSl, &panSl, &fadeSl }) { sl->setBounds (cx, selRow.getY() + 2, 118, 34); cx += 124; }
        dragWav.setBounds (W - 18 - 180, selRow.getY() - 4, 180, 46);
        bankBtn.setBounds (dragWav.getX() - 136, selRow.getY(), 128, 38);
        pairBtn.setBounds (bankBtn.getX() - 136, selRow.getY(), 128, 38);
        padArea = { 14, selRow.getBottom() + 34, W - 28 - 210, H - selRow.getBottom() - 48 };
        const int colX = padArea.getRight() + 12, colW = W - 14 - colX;
        dragMidi.setBounds (colX, padArea.getY(), colW, 48);
        flipBtn.setBounds (colX, padArea.getY() + 58, colW, 40);
        dragFlip.setBounds (colX, padArea.getY() + 106, colW, 48);
    }
    void mouseMove (const MouseEvent& e) override
    {
        const int m = markAt (e.position);
        if (m != hoverMark) { hoverMark = m; repaint (wave); }
        setMouseCursor (m > 0 ? MouseCursor::LeftRightResizeCursor : MouseCursor::NormalCursor);
    }
    void mouseDown (const MouseEvent& e) override
    {
        dragMark = -1;
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return;
        for (int i = 0; i < 16; ++i) if (i < c->numSlices() && pad (i).contains (e.getPosition())) { sel = i; loadFx(); proc.chopPad = i; repaint(); return; }
        if (! wave.contains (e.getPosition())) return;
        const int m = markAt (e.position);
        if (m > 0 && e.mods.isPopupMenu()) { auto mk = c->marks; mk.erase (mk.begin() + m); proc.chop.setMarks (mk); refresh(); return; }
        if (m > 0) { dragMark = m; return; }
        const int smp = sampleAt (e.position.x);
        if (e.getNumberOfClicks() > 1) { auto mk = c->marks; mk.push_back (smp); proc.chop.setMarks (mk); refresh(); return; }
        int si = 0; while (si + 1 < c->numSlices() && c->sliceStart (si + 1) <= smp) ++si;
        sel = si; loadFx(); proc.chopPad = si; repaint();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragMark <= 0) return;
        auto c = proc.chop.current(); if (c == nullptr) return;
        auto mk = c->marks;
        if (dragMark >= (int) mk.size()) return;
        const int lo = mk[(size_t) dragMark - 1] + (int) (c->rate * 0.02), hi = dragMark + 1 < (int) mk.size() ? mk[(size_t) dragMark + 1] - (int) (c->rate * 0.02) : c->src->getNumSamples() - (int) (c->rate * 0.02);
        mk[(size_t) dragMark] = jlimit (lo, std::max (lo, hi), sampleAt (e.position.x));
        proc.chop.setMarks (mk);
        repaint (wave);
    }
    void mouseUp (const MouseEvent&) override { if (dragMark > 0) refresh(); dragMark = -1; }
private:
    kk::PairPtr sliceSound (int i) const
    {
        auto c = proc.chop.current(); if (c == nullptr) return {};
        auto b = proc.chop.slice (i);
        if (b.getNumSamples() < 64) return {};
        return kk::PairLab::fromBuffer (b, c->rate, c->rate, c->name + " chop " + String (i + 1));
    }
    bool keysOn() const { return (int) proc.apvts.getRawParameterValue (ID::playMode)->load() == KeysKillaProcessor::playChop; }
    void refresh()
    {
        peaks = proc.chop.overview (std::max (200, wave.getWidth() - 20));
        for (int i = 0; i < (int) modeBtns.size(); ++i) { modeBtns[(size_t) i]->selected = i == mode; modeBtns[(size_t) i]->repaint(); }
        keysBtn.setButtonText (keysOn() ? "KEYS PLAY THE CHOPS" : "PLAY CHOPS ON KEYS"); keysBtn.selected = keysOn(); keysBtn.repaint();
        if (auto c = proc.chop.current()) sel = jlimit (0, std::max (0, c->numSlices() - 1), sel);
        loadFx();
        repaint();
    }
    Rectangle<int> pad (int i) const
    {
        const int cols = 8, gap = 10;
        const int pw = (padArea.getWidth() - gap * (cols - 1)) / cols, ph = (padArea.getHeight() - gap) / 2;
        return { padArea.getX() + (i % cols) * (pw + gap), padArea.getY() + (i / cols) * (ph + gap), pw, ph };
    }
    int sampleAt (float x) const
    {
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return 0;
        const auto w = wave.toFloat();
        return jlimit (0, c->src->getNumSamples() - 1, (int) ((x - w.getX() - 10) / (w.getWidth() - 20) * (float) c->src->getNumSamples()));
    }
    int markAt (Point<float> p) const
    {
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr || ! wave.toFloat().contains (p)) return -1;
        const auto w = wave.toFloat();
        for (int i = 1; i < c->numSlices(); ++i)
            if (std::abs (w.getX() + 10 + (w.getWidth() - 20) * (float) c->sliceStart (i) / (float) c->src->getNumSamples() - p.x) < 6.0f) return i;
        return -1;
    }
    void timerCallback() override
    {
        if (! isVisible()) return;
        if (const int h = proc.chop.lastHit.exchange (-1); h >= 0 && h < 16) padLit[(size_t) h] = 1.0f;
        bool any = false;
        for (auto& v : padLit) if (v > 0.01f) { v *= 0.85f; any = true; } else v = 0;
        if (any) repaint (padArea);
        if (proc.chop.playhead() >= 0 || wasPlaying) { wasPlaying = proc.chop.playhead() >= 0; repaint (wave); }
        if (keysOn() != keysBtn.selected) refresh();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    std::vector<std::unique_ptr<HotButton>> modeBtns;
    HotButton keysBtn { lnf }, bankAllBtn { lnf }, backBtn { lnf }, pairBtn { lnf }, bankBtn { lnf };
    DragFileButton dragWav { "DRAG SLICE WAV", Colour (0xff36ff6a) }, dragMidi { "DRAG CHOP MIDI", Colour (0xff36ff6a) }, dragFlip { "DRAG FLIP MIDI", Colour (0xff36ff6a) };
    Slider pitchSl, volSl, panSl, fadeSl;
    HotButton revBtn { lnf }, flipBtn { lnf };
    uint32_t flipSeed = 1; File flipFile;
    bool loadingFx = false;
    void pushFx()
    {
        if (loadingFx) return;
        kk::SliceFx f; f.semi = (int) pitchSl.getValue(); f.rev = revBtn.selected; f.vol = (float) volSl.getValue(); f.pan = (float) panSl.getValue(); f.fade = (float) fadeSl.getValue();
        proc.chop.setFx (sel, f);
    }
    void loadFx()
    {
        auto c = proc.chop.current(); if (c == nullptr) return;
        const auto f = c->fxOf (sel);
        loadingFx = true;
        pitchSl.setValue (f.semi, dontSendNotification); volSl.setValue (f.vol, dontSendNotification); panSl.setValue (f.pan, dontSendNotification); fadeSl.setValue (f.fade, dontSendNotification);
        revBtn.selected = f.rev; revBtn.repaint();
        loadingFx = false;
    }
    Rectangle<int> wave, selRow, padArea;
    std::vector<float> peaks;
    std::array<float, 16> padLit {};
    String note;
    int sel = 0, mode = 0, dragMark = -1, hoverMark = -1;
    bool wasPlaying = false;
};

// HALF / EFFECTOR / DIGGA: the original plugin (its own design and every function) inside KEYS KILLA.
// A slim strip on top: what it does, ON / OFF (HALF, EFFECTOR - melodies only), KEYS -> DIGGA.
class EmbeddedPage : public Component, private Timer
{
public:
    static constexpr int kRail = 148;   // the left tiles stay visible
    EmbeddedPage (KeysKillaProcessor& p, KKLookAndFeel& l, int moduleIndex, String t, String sub, const char* onParamId)
        : proc (p), lnf (l), mod (moduleIndex), title (std::move (t)), subtitle (std::move (sub)), onId (onParamId)
    {
        if (onId != nullptr)
        {
            onBtn.framed = true;
            onBtn.setTooltip (title + " on / off - it works on the melodies only (KEYS KILLA + DIGGA), never on drums");
            onBtn.onClick = [this] { setParamFromUi (proc, onId, isOn() ? 0.0f : 1.0f); refresh(); };
        }
        if (mod == KeysKillaProcessor::modDigga)
        {
            pairBtn.framed = true; pairBtn.setButtonText ("PAIR THE\nONE-SHOTS");
            pairBtn.setTooltip ("Send the one-shots DIGGA cut to PAIR YOUR OWN - breed them into new sounds and melody loops");
            pairBtn.onClick = [this] { proc.pairFromDigga(); if (onPair) onPair(); };
            addAndMakeVisible (pairBtn);
            chopBtn.framed = true; chopBtn.setButtonText ("CHOP /\nSLICE");
            chopBtn.setTooltip ("CHOP / SLICE the sample: move the cuts, MPC pads, play the chops on the keys, drag slices and the MIDI out");
            chopBtn.onClick = [this] { showChop (! chopOn); };
            addAndMakeVisible (chopBtn);
            saveShotsBtn.framed = true; saveShotsBtn.setButtonText ("SAVE THE\nSHOTS");
            saveShotsBtn.setTooltip ("Save DIGGA's one-shots into one of your MY SOUNDS folders");
            saveShotsBtn.onClick = [this]
            {
                double rate = 44100.0;
                std::vector<kk::PairPtr> all;
                for (auto& [name, buf] : kkDiggaShots (proc.module (mod), rate, false))
                    if (buf != nullptr && buf->getNumSamples() > 64) all.push_back (kk::PairLab::fromBuffer (*buf, rate, rate, "DIGGA " + name));
                saveToFolderMenu (proc, all, &saveShotsBtn, [safe = Component::SafePointer<EmbeddedPage> (this)] (String msg)
                { if (safe != nullptr) { safe->saveShotsBtn.setTooltip (msg); safe->saveNote = msg; safe->repaint(); } });
            };
            addAndMakeVisible (saveShotsBtn);
            chop = std::make_unique<ChopPanel> (proc, lnf);
            chop->onBack = [this] { showChop (false); };
            addChildComponent (*chop);
            keysBtn.framed = true;
            keysBtn.setTooltip ("KEYS: your MIDI keyboard / FL piano roll plays DIGGA instead of the KEYS KILLA sound");
            keysBtn.onClick = [this] { setParamFromUi (proc, ID::playMode, keysToDigga() ? 0.0f : 1.0f); refresh(); };
            addAndMakeVisible (keysBtn);
        }
        tryCreateEditor();
        setOpaque (true);
        refresh();
        startTimerHz (10);
    }
    ~EmbeddedPage() override { stopTimer(); releaseEditor(); }
    std::function<void()> onPair;
    void openChop() { showChop (true); }
    void visibilityChanged() override { if (isVisible()) { wantSteal = true; tryCreateEditor (true); } }
    void mouseUp (const MouseEvent& e) override
    {
        if (onId != nullptr && power.expanded (6).contains (e.getPosition())) { setParamFromUi (proc, onId, isOn() ? 0.0f : 1.0f); refresh(); repaint(); }
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.fillAll (Colour (0xff0f0e22));
        // left rail behind the BREED LAB / FAMILY TREE / KILLA tiles
        g.setColour (Colour (0xff12112a)); g.fillRect (0, 0, kRail, getHeight());
        g.setColour (s.panelEdge.withAlpha (0.6f)); g.drawVerticalLine (kRail - 1, 0.0f, (float) getHeight());
        g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (serif (22.0f, true, 0.25f));
        g.drawFittedText ("KEYS\nKILLA", Rectangle<int> (0, 22, kRail, 60), Justification::centred, 2);
        drawMaker (g, Rectangle<float> (0, 80, (float) kRail, 14), Justification::centred);
        if (onId != nullptr)   // big power switch under the tiles
        {
            const auto pr = power.toFloat();
            const bool on = isOn();
            const Colour c = on ? Colour (0xff36ff6a) : Colour (0xffff3030);
            g.setColour (c.withAlpha (on ? 0.25f : 0.12f)); g.fillEllipse (pr.expanded (10));
            g.setGradientFill (ColourGradient (Colour (0xff2c2858), pr.getCentreX(), pr.getY(), Colour (0xff0a0918), pr.getCentreX(), pr.getBottom(), false));
            g.fillEllipse (pr);
            g.setColour (c); g.drawEllipse (pr.reduced (2), 3.0f);
            const auto cc = pr.getCentre(); const float rr = pr.getWidth() * 0.22f;
            Path arc; arc.addCentredArc (cc.x, cc.y, rr, rr, 0, 0.6f, MathConstants<float>::twoPi - 0.6f, true);
            g.strokePath (arc, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
            g.drawLine (cc.x, cc.y - rr * 1.25f, cc.x, cc.y - rr * 0.2f, 3.5f);
            g.setFont (Font (FontOptions (18.0f, Font::bold)).withExtraKerningFactor (0.15f));
            g.drawText (on ? "ON" : "OFF", Rectangle<float> (0, pr.getBottom() + 8, (float) kRail, 24).toNearestInt(), Justification::centred);
            g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (11.0f)));
            g.drawFittedText (title + "\non the melodies", Rectangle<float> (4, pr.getBottom() + 32, (float) kRail - 8, 30).toNearestInt(), Justification::centred, 2);
        }
        if (editor == nullptr)
        {
            g.setColour (Colour (0xffc8c4e8)); g.setFont (serif (18.0f, false, 0.2f));
            g.drawText ("opening " + title + " ...", getLocalBounds(), Justification::centred);
        }
    }
    void resized() override
    {
        const int w = getWidth();
        juce::ignoreUnused (w);
        power = { kRail / 2 - 44, getHeight() - 260, 88, 88 };
        chopBtn.setBounds (8, getHeight() - 336, kRail - 16, 70);
        if (chop) chop->setBounds (getLocalBounds().withTrimmedLeft (kRail));
        keysBtn.setBounds (8, getHeight() - 260, kRail - 16, 70);
        pairBtn.setBounds (8, getHeight() - 184, kRail - 16, 70);
        saveShotsBtn.setBounds (8, getHeight() - 108, kRail - 16, 70);
        if (editor != nullptr)
        {
            // the plugin keeps its own look and lays itself out for the bigger size (no transform: crisp, OpenGL-safe)
            auto area = getLocalBounds().withTrimmedLeft (kRail).reduced (8);
            const float sc = std::min ((float) area.getWidth() / (float) native.getWidth(), (float) area.getHeight() / (float) native.getHeight());
            const int ew = roundToInt (native.getWidth() * sc), eh = roundToInt (native.getHeight() * sc);
            placed = { area.getX() + (area.getWidth() - ew) / 2, area.getY(), ew, eh };
            editor->setBounds (placed);
        }
    }
private:
    bool isOn() const { return onId != nullptr && proc.apvts.getRawParameterValue (onId)->load() > 0.5f; }
    bool keysToDigga() const { return (int) proc.apvts.getRawParameterValue (ID::playMode)->load() == KeysKillaProcessor::playDigga; }
    void refresh()
    {
        onBtn.setButtonText (isOn() ? title + "  ON" : title + "  OFF"); onBtn.selected = isOn(); onBtn.repaint();
        keysBtn.setButtonText (keysToDigga() ? "KEYS\nPLAY DIGGA" : "PLAY DIGGA\nON KEYS"); keysBtn.selected = keysToDigga(); keysBtn.repaint();
        repaint (power.expanded (20).withWidth (kRail).withX (0).withHeight (160));
        lastSig = (isOn() ? 1 : 0) + (keysToDigga() ? 2 : 0);
    }
    // the host can open a second KEYS KILLA window before closing the old one: wait until the plugin's
    // previous editor is gone, then create ours (never two editors for one plugin)
    // a plugin has one editor: if another KEYS KILLA window (FL keeps old ones alive) still holds it and is not
    // on screen, it hands it over to the window you are looking at
    static std::map<AudioProcessor*, EmbeddedPage*>& owners() { static std::map<AudioProcessor*, EmbeddedPage*> o; return o; }
    void releaseEditor()
    {
        if (editor == nullptr) return;
        if (auto* m = proc.module (mod); m != nullptr && owners()[m] == this) owners().erase (m);
        removeChildComponent (editor.get()); editor.reset(); repaint();
    }
    void tryCreateEditor (bool steal = false)
    {
        auto* m = proc.module (mod);
        if (m == nullptr || editor != nullptr) return;
        if (m->getActiveEditor() != nullptr)
        {
            // the page you are looking at always wins: the other (older / hidden) window hands its editor over
            auto it = owners().find (m);
            if (! steal || it == owners().end() || it->second == this) return;
            it->second->releaseEditor();
            if (m->getActiveEditor() != nullptr) return;
        }
        editor.reset (m->createEditorIfNeeded());
        if (editor != nullptr) { owners()[m] = this; addAndMakeVisible (*editor); editor->setVisible (! chopOn); native = editor->getBounds().withPosition (0, 0); resized(); if (chop) chop->toFront (false); repaint(); }
    }
    void timerCallback() override
    {
        if (editor == nullptr && isShowing()) tryCreateEditor (wantSteal);
        wantSteal = false;
        if (editor != nullptr && editor->getBounds() != placed) resized();   // the plugin changed its own size: fit it again
        if ((isOn() ? 1 : 0) + (keysToDigga() ? 2 : 0) != lastSig) refresh();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    int mod;
    String title, subtitle;
    const char* onId;
    std::unique_ptr<AudioProcessorEditor> editor;
    Rectangle<int> native, placed, power;
    HotButton onBtn { lnf }, keysBtn { lnf }, pairBtn { lnf }, chopBtn { lnf }, saveShotsBtn { lnf };
    String saveNote;
    std::unique_ptr<ChopPanel> chop;
    bool chopOn = false, wantSteal = false;
    void showChop (bool on)
    {
        chopOn = on && chop != nullptr;
        if (chop) { chop->setVisible (chopOn); if (chopOn) chop->toFront (false); }
        if (editor) editor->setVisible (! chopOn);
        chopBtn.selected = chopOn; chopBtn.repaint();
    }
    int lastSig = -1;
};

// ---------------- DRUM BOOST pages: 808 / SNARE-CLAP / HI-HAT, every one with its own trap look ----------------
static void drawPeaks (Graphics& g, Rectangle<float> r, const std::vector<float>& pk, Colour c)
{
    if (pk.empty()) return;
    Path p; const float mid = r.getCentreY(), half = r.getHeight() * 0.5f;
    p.startNewSubPath (r.getX(), mid);
    for (size_t i = 0; i < pk.size(); ++i) p.lineTo (r.getX() + r.getWidth() * (float) i / (float) pk.size(), mid - pk[i] * half);
    for (size_t i = pk.size(); i-- > 0;) p.lineTo (r.getX() + r.getWidth() * (float) i / (float) pk.size(), mid + pk[i] * half);
    p.closeSubPath();
    g.setColour (c); g.fillPath (p);
}

struct DrumTheme
{
    String title, sub, dropText, patTitle;
    Colour top, bottom, accent, accent2, text;
    StringArray styles;
};
// v0.22: futuristic MPC grey, one neon accent per drum
static const DrumTheme& drumTheme (int d)
{
    static const DrumTheme t[kk::numDrumSlots] {
        { "808", "SUB BOOSTER  -  DROP YOUR 808, MAKE IT KNOCK, WRITE THE 808 LINE, DRAG IT ALL BACK INTO FL", "DROP YOUR 808 HERE", "808 PATTERNS",
          Colour (0xff1d1a3c), Colour (0xff0a0a1a), Colour (0xffff5a1f), Colour (0xffffc23d), Colour (0xffeef0f2), { "ATLANTA", "DRILL", "PLUGG", "RAGE" } },
        { "SNARE / CLAP", "CRACK LAB  -  DROP A SNARE OR CLAP, SHARPEN IT, GENERATE TRAP SNARE ROLLS", "DROP YOUR SNARE / CLAP HERE", "SNARE ROLLS",
          Colour (0xff1b1a3a), Colour (0xff0a0a1a), Colour (0xff1fe0ff), Colour (0xffff3fd2), Colour (0xffeef0f2), { "TRAP", "TRIPLET", "DRILL", "BUILD-UP" } },
        { "HI-HAT", "HAT FACTORY  -  DROP A HI-HAT, MAKE IT SHINE, TRAP ROLLS IN THE PIANO ROLL", "DROP YOUR HI-HAT HERE", "HI-HAT ROLLS",
          Colour (0xff1f1a3e), Colour (0xff0a0a1a), Colour (0xffffd23f), Colour (0xffb070ff), Colour (0xffeef0f2), { "ATLANTA", "TRIPLET", "DRILL", "CRAZY" } },
        { "KICK", "KICK LAB  -  DROP A KICK, MAKE IT HIT, SAVE IT INTO YOUR DRUM KIT", "DROP YOUR KICK HERE", "MY DRUM KIT",
          Colour (0xff1d1a3c), Colour (0xff0a0a1a), Colour (0xffff3b30), Colour (0xffff9f0a), Colour (0xffeef0f2), { "", "", "", "" } },
        { "OPEN HAT", "OPEN HAT  -  DROP AN OPEN HAT / CRASH, SHAPE ITS TAIL, SAVE IT INTO YOUR DRUM KIT", "DROP YOUR OPEN HAT HERE", "MY DRUM KIT",
          Colour (0xff1f1a3e), Colour (0xff0a0a1a), Colour (0xffffe066), Colour (0xff64d2ff), Colour (0xffeef0f2), { "", "", "", "" } },
        { "PERC", "PERCUSSION  -  RIMS, TOMS, SHAKERS, BONGOS ... SAVE THEM INTO YOUR DRUM KIT", "DROP YOUR PERC HERE", "MY DRUM KIT",
          Colour (0xff1b1a3a), Colour (0xff0a0a1a), Colour (0xff30d158), Colour (0xffffd60a), Colour (0xffeef0f2), { "", "", "", "" } },
        { "FX", "DRUM FX  -  RISERS, IMPACTS, VOX TAGS, REVERSES ... SAVE THEM INTO YOUR DRUM KIT", "DROP YOUR FX HERE", "MY DRUM KIT",
          Colour (0xff211a40), Colour (0xff0a0a1a), Colour (0xffbf5af2), Colour (0xff64d2ff), Colour (0xffeef0f2), { "", "", "", "" } } };
    return t[jlimit (0, kk::numDrumSlots - 1, d)];
}

class ThemedKnob : public Slider
{
public:
    ThemedKnob (const DrumTheme& t) : Slider (RotaryHorizontalVerticalDrag, NoTextBox), th (t)
    {
        setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        setPopupDisplayEnabled (true, true, nullptr);
    }
    String label;
    void paint (Graphics& g) override
    {
        const auto b = getLocalBounds().toFloat().withTrimmedBottom (20);
        const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f - 4.0f;
        const auto c = b.getCentre();
        const float pos = (float) valueToProportionOfLength (getValue());
        const float a0 = MathConstants<float>::pi * 1.25f, a1 = a0 + MathConstants<float>::pi * 1.5f * pos;
        Path track; track.addCentredArc (c.x, c.y, r, r, 0, a0, MathConstants<float>::pi * 2.75f, true);
        g.setColour (Colours::white.withAlpha (0.08f)); g.strokePath (track, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        Path arc; arc.addCentredArc (c.x, c.y, r, r, 0, a0, a1, true);
        g.setColour (th.accent.withAlpha (0.25f)); g.strokePath (arc, PathStrokeType (11.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setGradientFill (ColourGradient (th.accent2, c.x - r, c.y, th.accent, c.x + r, c.y, false));
        g.strokePath (arc, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
        const float cr = r * 0.68f;
        g.setGradientFill (ColourGradient (Colour (0xff4a4670), c.x, c.y - cr, Colour (0xff16152e), c.x, c.y + cr, false));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2);
        g.setColour (Colours::white.withAlpha (0.14f)); g.drawEllipse (c.x - cr, c.y - cr, cr * 2, cr * 2, 1.2f);
        const float ang = a1 - MathConstants<float>::halfPi;
        g.setColour (th.text);
        g.drawLine (c.x + std::cos (ang) * cr * 0.25f, c.y + std::sin (ang) * cr * 0.25f, c.x + std::cos (ang) * cr * 0.9f, c.y + std::sin (ang) * cr * 0.9f, 2.6f);
        g.setColour (th.text.withAlpha (0.85f)); g.setFont (Font (FontOptions (13.0f, Font::bold)));
        g.drawText (label, getLocalBounds().removeFromBottom (18), Justification::centred);
    }
private:
    const DrumTheme& th;
};

// ---------------- PIANO ROLL for the drum patterns (808 line / snare rolls / hi-hat rolls) ----------------
// click = add, drag = move, drag the right edge = length, double-click / right-click = delete, bottom lane = velocity
class PatternEditor : public Component
{
public:
    PatternEditor (KeysKillaProcessor& p, int drum, const DrumTheme& t) : proc (p), d (drum), th (t)
    {
        lo = d == 0 ? -12 : d == 1 ? -7 : -8;
        hi = d == 0 ? 12 : 12;
        if (d == 2) hi = 8;
        setWantsKeyboardFocus (true);
        reload(); setBaseline(); before = { pat, bars };
    }
    int stepsPerBeat = 4; bool triplet = false;
    std::function<void()> onEdit;
    void reload() { pat = proc.pattern (d); bars = proc.patBars[(size_t) d]; sel = -1; repaint(); }
    // ---- UNDO / REDO / RESET (Ctrl+Z, Ctrl+Y) ----
    struct State { std::vector<kk::RollHit> pat; int bars = 2; };
    void remember()   // call BEFORE a change (generate, clear, an edit)
    {
        undoStack.push_back ({ proc.pattern (d), proc.patBars[(size_t) d] });
        if (undoStack.size() > 100) undoStack.erase (undoStack.begin());
        redoStack.clear();
    }
    void setBaseline() { baseline = { proc.pattern (d), proc.patBars[(size_t) d] }; }   // RESET goes back here (the last generated pattern)
    bool undo() { return step (undoStack, redoStack); }
    bool redo() { return step (redoStack, undoStack); }
    void reset() { remember(); apply (baseline); }
    bool canUndo() const { return ! undoStack.empty(); }
    bool canRedo() const { return ! redoStack.empty(); }
    float playBeat = -1.0f;

    void paint (Graphics& g) override
    {
        const auto all = getLocalBounds().toFloat();
        g.setColour (Colour (0xff0b0a18)); g.fillRoundedRectangle (all, 8);
        g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (all.reduced (0.5f), 8, 1.2f);
        const auto gr = grid(), vl = velLane();
        const int rows = hi - lo + 1;
        const float rh = gr.getHeight() / (float) rows;
        const int root = rootNote();
        // rows: the root row glows, octaves lighter
        for (int r = 0; r < rows; ++r)
        {
            const int semi = hi - r;
            const float y = gr.getY() + r * rh;
            const bool black = MidiMessage::isMidiNoteBlack (root + semi);
            g.setColour (semi == 0 ? th.accent.withAlpha (0.12f) : Colour (black ? 0xff12112a : 0xff17152f)); g.fillRect (gr.getX(), y, gr.getWidth(), rh);
            g.setColour (Colour (0xff221f40)); g.drawHorizontalLine ((int) y, gr.getX(), gr.getRight());
            if (rh >= 9.0f)
            {
                g.setColour (semi == 0 ? th.accent : Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (std::min (12.0f, rh - 1.0f), semi == 0 ? Font::bold : Font::plain)));
                const String lab = d == 0 ? MidiMessage::getMidiNoteName (root + semi, true, true, 5) : (semi == 0 ? String ("ROOT") : (semi > 0 ? "+" : "") + String (semi));
                g.drawText (lab, Rectangle<float> (all.getX() + 4, y, gr.getX() - all.getX() - 8, rh), Justification::centredRight);
            }
        }
        // beat grid
        const double len = bars * 4.0;
        const int sub = stepsPerBeat * (triplet ? 3 : 2) / 2 * 1;
        for (int k = 0; k <= (int) (len * sub); ++k)
        {
            const double b = k / (double) sub;
            const float x = beatX (b);
            const bool beat = std::abs (b - std::round (b)) < 1.0e-6, bar = beat && ((int) std::round (b)) % 4 == 0;
            g.setColour (Colour (bar ? 0xff7d77a8 : beat ? 0xff3a3462 : 0xff1d1b38));
            g.drawVerticalLine ((int) x, gr.getY(), vl.getBottom());
            if (bar && b < len)
            {
                g.setColour (Colour (0xffb4aed8)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
                g.drawText (String ((int) std::round (b) / 4 + 1), Rectangle<float> (x + 4, all.getY() + 2, 30, 14), Justification::centredLeft);
            }
        }
        // notes
        for (int i = 0; i < (int) pat.size(); ++i)
        {
            const auto& h = pat[(size_t) i];
            if (h.semi < lo || h.semi > hi) continue;
            auto r = noteRect (h);
            const Colour c = (h.semi == 0 ? th.accent : th.accent2).withMultipliedBrightness (0.55f + 0.45f * h.vel);
            g.setColour (c.withAlpha (0.35f)); g.fillRoundedRectangle (r.expanded (1.5f), 3);
            g.setColour (c); g.fillRoundedRectangle (r, 2.5f);
            g.setColour (i == sel ? Colours::white : Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (r, 2.5f, i == sel ? 1.8f : 1.0f);
            // velocity lane
            const float vx = beatX (h.beat);
            g.setColour (c); g.fillRect (vx, vl.getBottom() - vl.getHeight() * h.vel, 3.0f, vl.getHeight() * h.vel);
        }
        g.setColour (Colour (0xff3a3264)); g.drawHorizontalLine ((int) vl.getY() - 1, gr.getX(), gr.getRight());
        g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (10.0f, Font::bold)));
        g.drawText ("VEL", Rectangle<float> (all.getX() + 4, vl.getY(), gr.getX() - all.getX() - 8, vl.getHeight()), Justification::centredRight);
        if (playBeat >= 0)
        {
            const float x = beatX (playBeat);
            g.setColour (Colours::white.withAlpha (0.9f)); g.drawVerticalLine ((int) x, gr.getY(), vl.getBottom());
        }
        if (pat.empty())
        {
            g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (18.0f, Font::bold)));
            g.drawText ("CLICK TO DRAW NOTES  -  OR HIT GENERATE", gr.toNearestInt(), Justification::centred);
        }
    }

    void mouseDown (const MouseEvent& e) override
    {
        const auto p = e.position;
        mode = none;
        grabKeyboardFocus();
        before = { proc.pattern (d), bars };
        if (velLane().contains (p)) { mode = vel; setVel (p); return; }
        if (! grid().contains (p)) return;
        const int hit = noteAt (p);
        if (hit >= 0 && (e.mods.isPopupMenu() || e.getNumberOfClicks() > 1))
        {
            pat.erase (pat.begin() + hit); sel = -1; commit(); return;
        }
        if (hit >= 0)
        {
            sel = hit;
            const auto r = noteRect (pat[(size_t) hit]);
            mode = p.x > r.getRight() - 6 ? resize : move;
            grabBeat = xBeat (p.x) - pat[(size_t) hit].beat; grabSemi = semiAt (p.y) - pat[(size_t) hit].semi;
            repaint(); return;
        }
        if (e.mods.isPopupMenu()) return;
        kk::RollHit h; h.beat = snap (xBeat (p.x), true); h.semi = semiAt (p.y); h.vel = 0.85f; h.len = step();
        if (d == 0) h.len = std::max (0.5, step());
        pat.push_back (h); sel = (int) pat.size() - 1; mode = resize; grabBeat = 0;
        commit();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        const auto p = e.position;
        if (mode == vel) { setVel (p); return; }
        if (sel < 0 || sel >= (int) pat.size()) return;
        auto& h = pat[(size_t) sel];
        const double len = bars * 4.0;
        if (mode == move)
        {
            h.beat = jlimit (0.0, len - step() * 0.5, snap (xBeat (p.x) - grabBeat, false));
            h.semi = jlimit (lo, hi, semiAt (p.y) - grabSemi);
        }
        else if (mode == resize)
            h.len = jlimit (step() * 0.25, len - h.beat, std::max (step() * 0.25, snap (xBeat (p.x), false) - h.beat));
        repaint();
        live();
    }
    void mouseUp (const MouseEvent&) override { if (mode == move || mode == resize || mode == vel) commit(); mode = none; }
    void mouseMove (const MouseEvent& e) override
    {
        const int hit = noteAt (e.position);
        setMouseCursor (hit >= 0 && e.position.x > noteRect (pat[(size_t) hit]).getRight() - 6 ? MouseCursor::LeftRightResizeCursor
                        : hit >= 0 ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor);
    }
    bool keyPressed (const KeyPress& k) override
    {
        const bool cmd = k.getModifiers().isCommandDown() || k.getModifiers().isCtrlDown();
        if (cmd && (k.getKeyCode() == 'Z' || k.getKeyCode() == 'z') && ! k.getModifiers().isShiftDown()) { undo(); return true; }
        if (cmd && (k.getKeyCode() == 'Y' || k.getKeyCode() == 'y' || ((k.getKeyCode() == 'Z' || k.getKeyCode() == 'z') && k.getModifiers().isShiftDown()))) { redo(); return true; }
        if ((k == KeyPress::deleteKey || k == KeyPress::backspaceKey) && sel >= 0 && sel < (int) pat.size()) { pat.erase (pat.begin() + sel); sel = -1; commit(); return true; }
        return false;
    }
    int rootNote() const { auto s = proc.drum (d).current(); return d == 0 ? (s ? s->rootNote : 36) : 60; }

private:
    enum Mode { none, move, resize, vel };
    Rectangle<float> grid() const { return getLocalBounds().toFloat().withTrimmedLeft (58).withTrimmedRight (8).withTrimmedTop (18).withTrimmedBottom (54); }
    Rectangle<float> velLane() const { auto g = grid(); return { g.getX(), g.getBottom() + 4, g.getWidth(), 44.0f }; }
    float beatX (double b) const { const auto g = grid(); return g.getX() + g.getWidth() * (float) (b / (bars * 4.0)); }
    double xBeat (float x) const { const auto g = grid(); return (x - g.getX()) / g.getWidth() * bars * 4.0; }
    int semiAt (float y) const { const auto g = grid(); const float rh = g.getHeight() / (float) (hi - lo + 1); return jlimit (lo, hi, hi - (int) std::floor ((y - g.getY()) / rh)); }
    double step() const { return 1.0 / (stepsPerBeat * (triplet ? 1.5 : 1.0)); }
    double snap (double b, bool floorIt) const { const double s = step(); return (floorIt ? std::floor (b / s) : std::round (b / s)) * s; }
    Rectangle<float> noteRect (const kk::RollHit& h) const
    {
        const auto g = grid(); const float rh = g.getHeight() / (float) (hi - lo + 1);
        const float x0 = beatX (h.beat), x1 = beatX (h.beat + h.len);
        return { x0, g.getY() + (hi - h.semi) * rh + 1, std::max (4.0f, x1 - x0), std::max (2.0f, rh - 2) };
    }
    int noteAt (Point<float> p) const
    {
        for (int i = (int) pat.size(); --i >= 0;) if (noteRect (pat[(size_t) i]).expanded (1, 0).contains (p)) return i;
        return -1;
    }
    void setVel (Point<float> p)
    {
        const auto v = velLane();
        const float val = jlimit (0.05f, 1.0f, (v.getBottom() - p.y) / v.getHeight());
        bool any = false;
        for (auto& h : pat) if (std::abs (beatX (h.beat) - p.x) < 5.0f) { h.vel = val; any = true; }
        if (any) { repaint(); live(); }
    }
    void live() { proc.setPattern (d, pat, bars); }
    void commit()
    {
        const kk::RollHit keep = sel >= 0 && sel < (int) pat.size() ? pat[(size_t) sel] : kk::RollHit {};
        const bool had = sel >= 0 && sel < (int) pat.size();
        std::sort (pat.begin(), pat.end(), [] (const kk::RollHit& a, const kk::RollHit& b) { return a.beat < b.beat; });
        sel = -1;
        if (had) for (int i = 0; i < (int) pat.size(); ++i) if (pat[(size_t) i].beat == keep.beat && pat[(size_t) i].semi == keep.semi) { sel = i; break; }
        if (before.pat.size() != pat.size() || ! same (before.pat, pat) || before.bars != bars)
        {
            undoStack.push_back (before); if (undoStack.size() > 100) undoStack.erase (undoStack.begin());
            redoStack.clear();
        }
        before = { pat, bars };
        proc.setPattern (d, pat, bars);
        repaint();
        if (onEdit) onEdit();
    }
    static bool same (const std::vector<kk::RollHit>& a, const std::vector<kk::RollHit>& b)
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i)
            if (std::abs (a[i].beat - b[i].beat) > 1.0e-9 || a[i].semi != b[i].semi || std::abs (a[i].len - b[i].len) > 1.0e-9 || std::abs (a[i].vel - b[i].vel) > 1.0e-6f) return false;
        return true;
    }
    bool step (std::vector<State>& from, std::vector<State>& to)
    {
        if (from.empty()) return false;
        to.push_back ({ proc.pattern (d), proc.patBars[(size_t) d] });
        auto st = from.back(); from.pop_back();
        apply (st);
        return true;
    }
    void apply (const State& st)
    {
        proc.setPattern (d, st.pat, st.bars);
        reload(); before = { pat, bars };
        if (onEdit) onEdit();
    }
    std::vector<State> undoStack, redoStack;
    State baseline, before;
    KeysKillaProcessor& proc;
    int d;
    const DrumTheme& th;
    std::vector<kk::RollHit> pat;
    int bars = 2, lo = -12, hi = 12, sel = -1;
    Mode mode = none;
    double grabBeat = 0; int grabSemi = 0;
};

// ---------------- DRUM pages: 808 / SNARE-CLAP / HI-HAT over the whole window ----------------
// top: your sample + its own reactive art, the boost knobs; bottom: the pattern generator + piano roll + drag MIDI
class DrumPage : public Component, public FileDragAndDropTarget, private Timer
{
public:
    static constexpr int kRail = 148;   // the left tiles stay visible
    DrumPage (KeysKillaProcessor& p, KKLookAndFeel& l, int drum) : proc (p), lnf (l), d (drum), th (drumTheme (drum)), editor (p, drum, drumTheme (drum))
    {
        // knob set per drum: { knob index, label }
        std::vector<std::pair<int, const char*>> ks;
        const auto kind = kk::kindOfSlot (d);
        if (kind == kk::drum808) ks = { { 0, "GAIN" }, { 1, "PITCH" }, { 2, "PUNCH" }, { 6, "SUB" }, { 7, "TONE" }, { 3, "DRIVE" }, { 5, "CLIPPER" }, { 8, "LENGTH" }, { 10, "WIDTH" } };
        else if (kind == kk::drumSnare) ks = { { 0, "GAIN" }, { 1, "PITCH" }, { 2, "PUNCH" }, { 6, "BODY" }, { 7, "SNAP" }, { 3, "DRIVE" }, { 5, "CLIPPER" }, { 9, "ROOM" }, { 8, "LENGTH" }, { 10, "WIDTH" } };
        else ks = { { 0, "GAIN" }, { 1, "PITCH" }, { 2, "PUNCH" }, { 7, "AIR" }, { 11, "DE-RES" }, { 3, "DRIVE" }, { 5, "CLIPPER" }, { 8, "LENGTH" }, { 10, "WIDTH" } };
        for (auto& [k, name] : ks)
        {
            auto kn = std::make_unique<ThemedKnob> (th);
            kn->label = name;
            const auto id = ID::boost (d, k);
            auto* prm = proc.apvts.getParameter (id);
            kn->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
            atts.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, *kn));
            addAndMakeVisible (*kn); knobs.push_back (std::move (kn));
        }
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, Choices::satModes[i].toUpperCase());
            b->framed = true; b->setTooltip ("Drive flavour: TAPE warm, TUBE punchy, FOLD aggressive" + String (d == kk::drum808 ? " - the sub stays clean (808 Killa style)" : ""));
            b->onClick = [this, i] { setParamFromUi (proc, ID::boost (d, 4).toRawUTF8(), (float) i); refresh(); };
            addAndMakeVisible (*b); satBtns.push_back (std::move (b));
        }
        auto btn = [this] (HotButton& b, const String& text, const String& tip, std::function<void()> fn)
        { b.setButtonText (text); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (loadBtn, "LOAD WAV", "Load your drum (WAV, AIFF, FLAC, MP3) - or just drag it onto this page", [this] { browse(); });
        btn (hitBtn, "PLAY", "Hear the boosted drum", [this] { proc.hitDrum (d); });
        btn (keysBtn, "", "KEYS: the keyboard / FL piano roll plays this drum" + String (d == kk::drum808 ? " - in tune: the 808's own note sits on its key" : ""),
             [this] { setParamFromUi (proc, ID::playMode, keysOn() ? 0.0f : (float) KeysKillaProcessor::modeOfDrum (d)); refresh(); });
        btn (dragBtn, "SAVE WAV", "Save the boosted drum next to the original (or drag the waveform straight into FL)", [this] { saveNextToOriginal(); });
        dragBtn.setVisible (false);
        dragWav.makeFile = [this] { return proc.exportDrum (d); };
        dragWav.setTooltip ("Drag the boosted " + th.title + " into FL Studio (channel rack / playlist)");
        addAndMakeVisible (dragWav);
        btn (clearBtn, "CLEAR", "Remove the sample", [this] { proc.clearDrum (d); refresh(); });

        // switch between the three drum pages
        static const char* names[] { "808", "SNARE/CLAP", "HI-HAT", "KICK", "OPEN HAT", "PERC", "FX" };
        for (int i = 0; i < kk::numDrumSlots; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, names[i]);
            b->framed = true; b->selected = i == d; b->tint = drumTheme (i).accent;
            b->onClick = [this, i] { if (i != d && onSwitch) onSwitch (i); };
            addAndMakeVisible (*b); switchBtns.push_back (std::move (b));
        }

        // PATTERN generator
        for (int i = 0; i < 4; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, th.styles[i]);
            b->framed = true; b->setTooltip (th.patTitle + " style - click for a fresh one in this style");
            b->onClick = [this, i] { editor.remember(); proc.generatePattern (d, i, proc.patBars[(size_t) d], (float) density.getValue()); afterGenerate(); };
            addAndMakeVisible (*b); styleBtns.push_back (std::move (b));
        }
        static const int barsOf[] { 1, 2, 4 };
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, String (barsOf[i]) + (i == 0 ? " BAR" : " BARS"));
            b->framed = true; b->setTooltip ("Pattern length");
            b->onClick = [this, i] { editor.remember(); proc.generatePattern (d, proc.patStyle[(size_t) d], barsOf[i], (float) density.getValue()); afterGenerate(); };
            addAndMakeVisible (*b); barBtns.push_back (std::move (b));
        }
        static const char* snaps[] { "1/8", "1/16", "1/32", "1/16T" };
        for (int i = 0; i < 4; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, snaps[i]);
            b->framed = true; b->setTooltip ("Grid for drawing notes");
            b->onClick = [this, i] { editor.stepsPerBeat = i == 0 ? 2 : i == 1 ? 4 : i == 2 ? 8 : 4; editor.triplet = i == 3; snapSel = i; refreshPattern(); editor.repaint(); };
            addAndMakeVisible (*b); snapBtns.push_back (std::move (b));
        }
        snapSel = d == kk::drumHat ? 2 : 1;
        editor.stepsPerBeat = d == kk::drumHat ? 8 : 4;
        density.setSliderStyle (Slider::LinearHorizontal); density.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
        density.setRange (0.0, 1.0); density.setValue (proc.patDensity[(size_t) d], dontSendNotification);
        density.setColour (Slider::thumbColourId, th.accent); density.setColour (Slider::trackColourId, th.accent.withAlpha (0.5f));
        density.setTooltip ("How busy the pattern is (more rolls, more notes)");
        density.onDragEnd = [this] { editor.remember(); proc.generatePattern (d, proc.patStyle[(size_t) d], proc.patBars[(size_t) d], (float) density.getValue()); afterGenerate(); };
        addAndMakeVisible (density);
        btn (genBtn, "GENERATE", "A brand new " + th.patTitle + " pattern", [this] { editor.remember(); proc.generatePattern (d, proc.patStyle[(size_t) d], proc.patBars[(size_t) d], (float) density.getValue()); afterGenerate(); });
        btn (playPatBtn, "PLAY", "Hear the pattern on your drum (with FL playing it follows the song tempo)", [this] { proc.patPlay = proc.patPlay.load() == d ? -1 : d; refreshPattern(); });
        btn (clearPatBtn, "CLEAR", "Empty pattern - draw your own", [this] { editor.remember(); proc.setPattern (d, {}, proc.patBars[(size_t) d]); editor.reload(); refreshPattern(); });
        btn (undoBtn, "UNDO", "Undo the last change (Ctrl+Z)", [this] { editor.undo(); });
        btn (redoBtn, "REDO", "Redo (Ctrl+Y)", [this] { editor.redo(); });
        btn (resetBtn, "RESET", "Back to the pattern as it was generated", [this] { editor.reset(); });
        btn (kitBtn, "SAVE TO KIT", "Save this boosted drum into your own drum kit (a folder FL Studio can browse)", [this]
        { saveToKitMenu (proc, { d }, &kitBtn, [safe = Component::SafePointer<DrumPage> (this)] (String m) { if (safe != nullptr) { safe->kitNote = m; safe->repaint(); } }); });
        btn (saveAllBtn, "SAVE WHOLE KIT", "Save every loaded drum (808, snare, hats, kick, open hat, perc, FX) into one kit", [this]
        {
            std::vector<int> all; for (int i = 0; i < kk::numDrumSlots; ++i) all.push_back (i);
            saveToKitMenu (proc, all, &saveAllBtn, [safe = Component::SafePointer<DrumPage> (this)] (String m) { if (safe != nullptr) { safe->kitNote = m; safe->repaint(); } });
        });
        btn (openKitBtn, "OPEN KITS FOLDER", "Your drum kits on the computer - add this folder to FL Studio's browser once", [this] { kk::Kits::kit (proc.lastKit).createDirectory(); kk::Kits::kit (proc.lastKit).revealToUser(); });
        btn (resetKnobsBtn, "RESET KNOBS", "All knobs of this drum back to their start values", [this]
        {
            for (int k = 0; k < ID::boostKnobs.size(); ++k)
                if (auto* q = proc.apvts.getParameter (ID::boost (d, k))) { q->beginChangeGesture(); q->setValueNotifyingHost (q->getDefaultValue()); q->endChangeGesture(); }
            refresh();
        });
        dragMidi.makeFile = [this] { return proc.exportPatternMidi (d); };
        dragMidi.setTooltip ("Drag the pattern into FL Studio: a MIDI clip for your " + th.title + " channel");
        addAndMakeVisible (dragMidi);
        editor.onEdit = [this] { refreshPattern(); };
        addAndMakeVisible (editor);

        loadArt();
        setOpaque (true);
        refresh(); refreshPattern();
        startTimerHz (30);
    }
    ~DrumPage() override { stopTimer(); if (proc.patPlay.load() == d) proc.patPlay = -1; }
    std::function<void (int)> onSwitch;

    bool isInterestedInFileDrag (const StringArray& files) override { for (auto& f : files) if (isAudio (f)) return true; return false; }
    void fileDragEnter (const StringArray&, int, int) override { dragHover = true; repaint(); }
    void fileDragExit (const StringArray&) override { dragHover = false; repaint(); }
    void filesDropped (const StringArray& files, int, int) override
    {
        dragHover = false;
        for (auto& f : files) if (isAudio (f)) { load (File (f)); break; }
        repaint();
    }
    void visibilityChanged() override
    {
        if (! isVisible() && proc.patPlay.load() == d) proc.patPlay = -1;
        if (isVisible()) { loadArt(); editor.reload(); refreshPattern(); repaint(); }   // the kit overview shows the other drum pages too
    }

    void paint (Graphics& g) override
    {
        const auto w = (float) getWidth(), h = (float) getHeight();
        // brushed MPC grey + a faint neon horizon
        g.setGradientFill (ColourGradient (th.top, 0, 0, th.bottom, 0, h, false)); g.fillAll();
        for (int y = 0; y < (int) h; y += 3) { g.setColour (Colours::white.withAlpha (y % 6 == 0 ? 0.012f : 0.0f)); g.drawHorizontalLine (y, 0, w); }
        g.setGradientFill (ColourGradient (th.accent.withAlpha (0.10f), w * 0.6f, 0, Colours::transparentBlack, w * 0.6f, 340, false));
        g.fillRect (0.0f, 0.0f, w, 340.0f);
        kk::modern::waves (g, { w * 0.30f, 74.0f }, { w, 8.0f }, 40.0f, th.accent, th.accent2, 6, 0.22f);
        // the rail behind the tiles
        g.setColour (Colour (0xff12112a)); g.fillRect (0, 0, kRail, getHeight());
        g.setColour (Colour (0xff3a3264)); g.drawVerticalLine (kRail - 1, 0.0f, h);
        g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (serif (22.0f, true, 0.25f));
        g.drawFittedText ("KEYS\nKILLA", Rectangle<int> (0, 22, kRail, 60), Justification::centred, 2);
        drawMaker (g, Rectangle<float> (0, 80, (float) kRail, 14), Justification::centred);
        // title
        g.setFont (Font (FontOptions (44.0f, Font::bold)).withExtraKerningFactor (0.08f));
        for (int k = 3; k >= 1; --k) { g.setColour (th.accent.withAlpha (0.12f)); g.drawText (th.title, kRail + 22 - k, 10 - k, 600, 56, Justification::centredLeft); }
        g.setColour (th.text); g.drawText (th.title, kRail + 22, 10, 600, 56, Justification::centredLeft);
        g.setColour (th.accent2); g.setFont (Font (FontOptions (12.5f, Font::bold)).withExtraKerningFactor (0.12f));
        g.drawText (th.sub, kRail + 24, 62, 1000, 18, Justification::centredLeft);
        // section panels (MPC style: dark inset plates with screws)
        for (auto r : { boostPanel, patPanel })
        {
            g.setColour (Colour (0xff16152e).withAlpha (0.9f)); g.fillRoundedRectangle (r.toFloat(), 10);
            g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10, 1.2f);
            for (auto c : { r.getTopLeft().translated (9, 9), r.getTopRight().translated (-9, 9), r.getBottomLeft().translated (9, -9), r.getBottomRight().translated (-9, -9) })
            { g.setColour (Colour (0xff4a4478)); g.fillEllipse ((float) c.x - 3, (float) c.y - 3, 6, 6); }
        }
        drawArt (g, art);
        drawWave (g);
        g.setColour (th.text.withAlpha (0.7f)); g.setFont (Font (FontOptions (12.0f, Font::bold)).withExtraKerningFactor (0.15f));
        g.drawText ("DRIVE FLAVOUR", satBtns.front()->getX(), satBtns.front()->getY() - 18, 200, 16, Justification::centredLeft);
        if (kitNote.isNotEmpty()) { g.setColour (Colour (0xff36ff6a)); g.setFont (Font (FontOptions (12.0f, Font::bold))); g.drawText (kitNote, kitBtn.getX() - 20, kitBtn.getBottom() + 2, 220, 16, Justification::centred); }
        g.setColour (th.text.withAlpha (0.7f)); g.setFont (Font (FontOptions (12.0f, Font::bold)).withExtraKerningFactor (0.15f));
        // pattern title with an LED
        g.setColour (th.accent); g.fillEllipse ((float) patPanel.getX() + 22, (float) patPanel.getY() + 22, 10, 10);
        g.setColour (th.text); g.setFont (Font (FontOptions (22.0f, Font::bold)).withExtraKerningFactor (0.08f));
        g.drawText (hasPattern() ? th.patTitle : "MY DRUM KIT", patPanel.getX() + 40, patPanel.getY() + 12, 300, 30, Justification::centredLeft);
        if (! hasPattern())
        {
            paintKit (g);
        }
        else
        {
        g.setColour (th.text.withAlpha (0.7f)); g.setFont (Font (FontOptions (12.0f, Font::bold)).withExtraKerningFactor (0.15f));
        g.drawText ("STYLE", styleBtns.front()->getX(), styleBtns.front()->getY() - 18, 200, 16, Justification::centredLeft);
        g.drawText ("LENGTH", barBtns.front()->getX(), barBtns.front()->getY() - 18, 200, 16, Justification::centredLeft);
        g.drawText ("DENSITY", density.getX(), density.getY() - 18, 200, 16, Justification::centredLeft);
        g.drawText ("GRID", snapBtns.front()->getX(), snapBtns.front()->getY() - 18, 200, 16, Justification::centredLeft);
        g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
        g.drawText (proc.drum (d).hasSample() ? String ((int) proc.pattern (d).size()) + " NOTES  -  CLICK DRAW / DRAG MOVE / DOUBLE-CLICK DELETE"
                                               : "LOAD YOUR " + th.title + " TO HEAR IT  -  THE MIDI DRAGS ANYWAY",
                    patPanel.getX() + 40, patPanel.getY() + 40, 520, 16, Justification::centredLeft);
        }
        if (auto s = proc.drum (d).current(); s != nullptr && d == kk::drum808)
        {
            g.setColour (th.accent2); g.setFont (Font (FontOptions (22.0f, Font::bold)));
            const String note = s->rootHz > 0 ? MidiMessage::getMidiNoteName (s->rootNote, true, true, 5) + "  " + String (s->rootHz, 1) + " Hz" : String ("NOTE  ?");
            g.drawText (note, art.getX(), art.getBottom() + 2, art.getWidth(), 26, Justification::centred);
        }
    }
    void resized() override
    {
        const int w = getWidth(), h = getHeight(), x0 = kRail + 14;
        for (int i = 0; i < kk::numDrumSlots; ++i) switchBtns[(size_t) i]->setBounds (w - kk::numDrumSlots * 122 - 14 + i * 122, 20, 116, 40);
        boostPanel = { x0, 88, w - x0 - 14, 372 };
        art = { x0 + 26, 106, 222, 222 };
        wave = { x0 + 290, 100, w - x0 - 290 - 30, 160 };
        int x = wave.getX();
        for (auto* b : { &loadBtn, &hitBtn, &keysBtn, &clearBtn })
        {
            const int bw = b == &keysBtn ? 270 : 120;
            b->setBounds (x, 270, bw, 36); x += bw + 10;
            if (b == &keysBtn) { dragWav.setBounds (x, 266, 190, 44); x += 200; }
        }
        for (int i = 0; i < 3; ++i) satBtns[(size_t) i]->setBounds (wave.getRight() - 3 * 106 - 8 + i * 106, 276, 100, 28);
        const int n = (int) knobs.size(), area = wave.getRight() - wave.getX(), kw = std::min (98, area / n);
        for (int i = 0; i < n; ++i) knobs[(size_t) i]->setBounds (wave.getX() + i * (area / n) + (area / n - kw) / 2, 318, kw, kw + 22);
        patPanel = { x0, 470, w - x0 - 14, h - 470 - 12 };
        const int py = patPanel.getY() + 82;
        int px = patPanel.getX() + 22;
        for (auto& b : styleBtns) { b->setBounds (px, py, 104, 32); px += 108; }
        px += 16;
        for (auto& b : barBtns) { b->setBounds (px, py, 72, 32); px += 76; }
        px += 16;
        density.setBounds (px, py, 150, 32); px += 166;
        for (auto& b : snapBtns) { b->setBounds (px, py, 58, 32); px += 62; }
        const int right = patPanel.getRight() - 18;
        dragMidi.setBounds (right - 190, patPanel.getY() + 14, 190, 52);
        clearPatBtn.setBounds (right - 190 - 10 - 90, patPanel.getY() + 20, 90, 40);
        playPatBtn.setBounds (right - 190 - 10 - 90 - 10 - 110, patPanel.getY() + 20, 110, 40);
        genBtn.setBounds (right - 190 - 10 - 90 - 10 - 110 - 10 - 150, patPanel.getY() + 20, 150, 40);
        resetBtn.setBounds (right - 100, py, 100, 32);
        redoBtn.setBounds (right - 100 - 8 - 86, py, 86, 32);
        undoBtn.setBounds (right - 100 - 8 - 86 - 8 - 86, py, 86, 32);
        resetKnobsBtn.setBounds (art.getX() + 21, 380, 180, 30);
        kitBtn.setBounds (art.getX() + 21, 416, 180, 34);
        if (! hasPattern())
        {
            saveAllBtn.setBounds (patPanel.getRight() - 18 - 200, patPanel.getY() + 16, 200, 40);
            openKitBtn.setBounds (patPanel.getRight() - 18 - 200 - 10 - 190, patPanel.getY() + 16, 190, 40);
        }
        else
        {   // on the pattern pages the kit buttons sit in the boost panel
            saveAllBtn.setBounds (boostPanel.getX() + 12, boostPanel.getBottom() + 0, 0, 0);
            openKitBtn.setBounds (boostPanel.getX() + 12, boostPanel.getBottom() + 0, 0, 0);
        }
        for (Component* c : std::initializer_list<Component*> { &editor, &genBtn, &playPatBtn, &clearPatBtn, &undoBtn, &redoBtn, &resetBtn, &dragMidi, &density })
            c->setVisible (hasPattern());
        for (auto& b : styleBtns) b->setVisible (hasPattern());
        for (auto& b : barBtns) b->setVisible (hasPattern());
        for (auto& b : snapBtns) b->setVisible (hasPattern());
        saveAllBtn.setVisible (! hasPattern()); openKitBtn.setVisible (! hasPattern());
        editor.setBounds (patPanel.getX() + 14, py + 44, patPanel.getWidth() - 28, patPanel.getBottom() - py - 44 - 14);
    }
    void mouseDown (const MouseEvent& e) override { dragFromWave = wave.contains (e.getPosition()); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (! dragFromWave || e.getDistanceFromDragStart() < 6) return;
        dragFromWave = false;
        dragOut();
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (art.contains (e.getPosition())) proc.hitDrum (d);
        else if (wave.contains (e.getPosition()) && ! proc.drum (d).hasSample() && e.getDistanceFromDragStart() < 4) browse();
    }
    // MY DRUM KIT overview: what is loaded in each of the seven slots + how the kit reaches FL Studio
    void paintKit (Graphics& g)
    {
        auto r = patPanel.reduced (22).withTrimmedTop (50);
        g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (12.0f, Font::bold)));
        g.drawText ("KIT: " + proc.lastKit.toUpperCase() + "   -   " + String (kk::Kits::count (proc.lastKit)) + " SOUNDS SAVED", r.removeFromTop (18), Justification::centredLeft);
        r.removeFromTop (8);
        static const char* slotNames[] { "808", "SNARE / CLAP", "HI-HAT", "KICK", "OPEN HAT", "PERC", "FX" };
        const int cw = (r.getWidth() - 6 * 10) / kk::numDrumSlots;
        auto cards = r.removeFromTop (130);
        for (int i = 0; i < kk::numDrumSlots; ++i)
        {
            auto c = Rectangle<int> (cards.getX() + i * (cw + 10), cards.getY(), cw, cards.getHeight()).toFloat();
            const auto& t = drumTheme (i);
            auto smp = proc.drum (i).current();
            g.setColour (Colour (0xff0b0a18)); g.fillRoundedRectangle (c, 8);
            g.setColour (i == d ? t.accent : Colour (0xff3a3264)); g.drawRoundedRectangle (c.reduced (0.5f), 8, i == d ? 2.0f : 1.0f);
            g.setColour (t.accent); g.setFont (Font (FontOptions (13.0f, Font::bold)).withExtraKerningFactor (0.08f));
            g.drawText (slotNames[i], c.reduced (10, 8).withHeight (18).toNearestInt(), Justification::centredLeft);
            if (smp != nullptr)
            {
                drawPeaks (g, c.reduced (10, 34).withTrimmedBottom (20), smp->peaks, t.accent.withAlpha (0.8f));
                g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (Font (FontOptions (11.0f)));
                g.drawText (smp->name, c.reduced (10, 8).removeFromBottom (16).toNearestInt(), Justification::centredLeft);
            }
            else { g.setColour (Colour (0xff7d77a8)); g.setFont (Font (FontOptions (12.0f))); g.drawText ("empty", c.toNearestInt(), Justification::centred); }
        }
        r.removeFromTop (16);
        g.setColour (Colour (0xffc8c4e8)); g.setFont (Font (FontOptions (13.0f)));
        g.drawFittedText ("1. Load a sound on any drum page and shape it.   2. SAVE TO KIT (or SAVE WHOLE KIT).   3. Your kit is a folder with 808s, Kicks, Snares & Claps, Hi-Hats, Open Hats, Percussion and FX.\n"
                          "In FL Studio once: Options > File settings > Browser extra search folders > add  Documents / KEYS KILLA / Drum Kits  -  your kits then sit in FL's browser like any drum kit.",
                          r.removeFromTop (44), Justification::centredLeft, 2);
    }
    // your own picture for this page: Documents/KEYS KILLA/Art/808.png, snare.png, hat.png (it pulses on every hit)
    static File artFile (int d)
    {
        static const char* n[] { "808", "snare", "hat", "kick", "openhat", "perc", "fx" };
        return File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("Art").getChildFile (String (n[jlimit (0, kk::numDrumSlots - 1, d)]) + ".png");
    }
private:
    static bool isAudio (const String& f) { return File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg"); }
    bool keysOn() const { return (int) proc.apvts.getRawParameterValue (ID::playMode)->load() == KeysKillaProcessor::modeOfDrum (d); }
    void loadArt()
    {
        const auto f = artFile (d);
        const auto t = f.getLastModificationTime();
        if (f.existsAsFile() && t != artTime) { artImg = ImageFileFormat::loadFrom (f); artTime = t; }
        else if (! f.existsAsFile()) artImg = {};
    }
    void refresh()
    {
        const int sat = (int) proc.apvts.getRawParameterValue (ID::boost (d, 4))->load();
        for (int i = 0; i < 3; ++i) { satBtns[(size_t) i]->selected = i == sat; satBtns[(size_t) i]->repaint(); }
        keysBtn.setButtonText (keysOn() ? "KEYS PLAY " + th.title : "PLAY " + th.title + " ON KEYS"); keysBtn.selected = keysOn(); keysBtn.repaint();
        const bool has = proc.drum (d).hasSample();
        for (auto* b : { &hitBtn, &dragBtn, &clearBtn }) b->setEnabled (has);
        dragWav.setVisible (has);
        lastSig = sat * 7 + (keysOn() ? 1 : 0) + (has ? 2 : 0);
        repaint();
    }
    void afterGenerate() { editor.reload(); editor.setBaseline(); refreshPattern(); }
    void refreshPattern()
    {
        for (int i = 0; i < 4; ++i) { styleBtns[(size_t) i]->selected = i == proc.patStyle[(size_t) d]; styleBtns[(size_t) i]->repaint(); }
        static const int barsOf[] { 1, 2, 4 };
        for (int i = 0; i < 3; ++i) { barBtns[(size_t) i]->selected = barsOf[i] == proc.patBars[(size_t) d]; barBtns[(size_t) i]->repaint(); }
        for (int i = 0; i < 4; ++i) { snapBtns[(size_t) i]->selected = i == snapSel; snapBtns[(size_t) i]->repaint(); }
        const bool playing = proc.patPlay.load() == d;
        playPatBtn.selected = playing; playPatBtn.setButtonText (playing ? "STOP" : "PLAY"); playPatBtn.repaint();
        repaint (patPanel.withHeight (64));
    }
    void timerCallback() override
    {
        if (! isVisible()) return;
        const int sat = (int) proc.apvts.getRawParameterValue (ID::boost (d, 4))->load();
        if (sat * 7 + (keysOn() ? 1 : 0) + (proc.drum (d).hasSample() ? 2 : 0) != lastSig) refresh();
        auto s = proc.drum (d).current();
        const float ph = proc.drum (d).playhead();
        // a hit = the play head jumps back to the start
        const bool hit = ph >= 0 && (lastPh < 0 || ph < lastPh - 0.02f);
        lastPh = ph;
        if (hit) { pulse = 1.0f; shake = { Random::getSystemRandom().nextFloat() * 2 - 1, Random::getSystemRandom().nextFloat() * 2 - 1 }; }
        else pulse *= 0.86f;
        if (s.get() != shown || ph >= 0 || pulse > 0.01f) { shown = s.get(); repaint (art.expanded (40)); repaint (wave); }
        const float pb = proc.patPlay.load() == d ? proc.patBeat.load() : -1.0f;
        if (pb != editor.playBeat) { editor.playBeat = pb; editor.repaint(); }
    }
    void drawArt (Graphics& g, Rectangle<int> r)
    {
        const auto c = r.toFloat().getCentre();
        if (artImg.isValid())   // the user's own picture: bounces and shakes on the beat
        {
            const float sc = 1.0f + 0.07f * pulse;
            auto dst = r.toFloat().withSizeKeepingCentre (r.getWidth() * sc, r.getHeight() * sc).translated (shake.x * 6 * pulse, shake.y * 6 * pulse);
            g.setColour (th.accent.withAlpha (0.12f + 0.3f * pulse)); g.fillRoundedRectangle (dst.expanded (8), 16);
            g.drawImage (artImg, dst, RectanglePlacement::centred);
            return;
        }
        const float rad = (float) r.getWidth() * 0.5f * (1.0f + 0.05f * pulse);
        g.setColour (th.accent.withAlpha (0.14f + 0.3f * pulse)); g.fillEllipse (c.x - rad - 12, c.y - rad - 12, (rad + 12) * 2, (rad + 12) * 2);
        const auto kind = kk::kindOfSlot (d);
        if (kind == kk::drum808)   // speaker cone
        {
            g.setGradientFill (ColourGradient (Colour (0xff36306a), c.x, c.y - rad, Colour (0xff0b0a1c), c.x, c.y + rad, false));
            g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
            for (int i = 1; i <= 5; ++i)
            {
                const float rr = rad * (1.0f - 0.16f * (float) i) * (1.0f + 0.04f * pulse * (float) i / 5.0f);
                g.setColour ((i % 2 ? th.accent : th.accent2).withAlpha (0.25f + 0.45f * pulse));
                g.drawEllipse (c.x - rr, c.y - rr, rr * 2, rr * 2, i == 1 ? 4.0f : 1.6f);
            }
            const float dc = rad * (0.22f + 0.05f * pulse);
            g.setGradientFill (ColourGradient (th.accent2, c.x - dc, c.y - dc, th.accent, c.x + dc, c.y + dc, true));
            g.fillEllipse (c.x - dc, c.y - dc, dc * 2, dc * 2);
            for (int k = 0; k < 8; ++k)   // bolts
            {
                const float a = MathConstants<float>::twoPi * (float) k / 8.0f;
                g.setColour (Colours::white.withAlpha (0.5f)); g.fillEllipse (c.x + std::cos (a) * rad * 0.93f - 3, c.y + std::sin (a) * rad * 0.93f - 3, 6, 6);
            }
        }
        else if (kind == kk::drumSnare)   // snare from above: head, rim, lugs, wires
        {
            g.setColour (Colour (0xff141230)); g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
            g.setColour (th.accent.withAlpha (0.8f)); g.drawEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2, 5.0f);
            const float hr = rad * 0.86f;
            g.setGradientFill (ColourGradient (Colour (0xffe9f0f2), c.x - hr * 0.3f, c.y - hr * 0.4f, Colour (0xff98a2a8), c.x + hr, c.y + hr, true));
            g.fillEllipse (c.x - hr, c.y - hr, hr * 2, hr * 2);
            for (int k = 0; k < 10; ++k)
            {
                const float a = MathConstants<float>::twoPi * (float) k / 10.0f;
                g.setColour (th.accent2); g.fillRoundedRectangle (c.x + std::cos (a) * rad - 5, c.y + std::sin (a) * rad - 5, 10, 10, 3);
            }
            g.setColour (th.accent.withAlpha (0.35f + 0.6f * pulse));
            for (int k = -4; k <= 4; ++k) g.drawLine (c.x - hr * 0.7f, c.y + (float) k * 6 + shake.y * 3 * pulse, c.x + hr * 0.7f, c.y + (float) k * 6 - shake.y * 3 * pulse, 1.2f);
        }
        else   // cymbal: grooves and shine, it wobbles on a hit
        {
            const float tilt = 0.55f + 0.08f * pulse * shake.x;
            g.setGradientFill (ColourGradient (Colour (0xfffff0b0), c.x - rad * 0.3f, c.y - rad * 0.3f, Colour (0xff8a5a10), c.x + rad, c.y + rad, true));
            g.fillEllipse (c.x - rad, c.y - rad * tilt, rad * 2, rad * 2 * tilt);
            for (int i = 1; i < 14; ++i)
            {
                const float rr = rad * (float) i / 14.0f;
                g.setColour (Colour (0xff5a3a08).withAlpha (0.25f)); g.drawEllipse (c.x - rr, c.y - rr * tilt, rr * 2, rr * 2 * tilt, 1.0f);
            }
            g.setColour (th.accent2.withAlpha (0.4f + 0.5f * pulse)); g.drawEllipse (c.x - rad, c.y - rad * tilt, rad * 2, rad * 2 * tilt, 3.0f);
            g.setColour (Colour (0xff3a2a10)); g.fillEllipse (c.x - 10, c.y - 6, 20, 12);
        }
    }
    void drawWave (Graphics& g)
    {
        const auto r = wave.toFloat();
        g.setGradientFill (ColourGradient (Colour (0xff1a1638), r.getX(), r.getY(), Colour (0xff0b0a1c), r.getX(), r.getBottom(), false)); g.fillRoundedRectangle (r, 10);
        for (int k = 3; k >= 1; --k) { g.setColour (th.accent.withAlpha (0.06f * (float) k)); g.drawRoundedRectangle (r.expanded ((float) (4 - k) * 2.0f), 12, 2.0f); }
        g.setGradientFill (ColourGradient ((dragHover ? th.accent2 : th.accent).withAlpha (dragHover ? 1.0f : 0.8f), r.getX(), r.getY(), th.accent2.withAlpha (dragHover ? 1.0f : 0.55f), r.getRight(), r.getBottom(), false));
        g.drawRoundedRectangle (r, 10, dragHover ? 3.0f : 1.8f);
        auto s = proc.drum (d).current();
        if (s == nullptr)
        {
            g.setColour (th.text.withAlpha (0.8f)); g.setFont (Font (FontOptions (26.0f, Font::bold)).withExtraKerningFactor (0.1f));
            g.drawText (th.dropText, wave.withTrimmedBottom (30), Justification::centred);
            g.setColour (th.text.withAlpha (0.5f)); g.setFont (Font (FontOptions (14.0f)));
            g.drawText ("drag a WAV from FL Studio (browser / channel rack) or click to load", wave.withTrimmedTop (60), Justification::centred);
            return;
        }
        auto in = r.reduced (12, 16);
        const float mid = in.getCentreY(), half = in.getHeight() * 0.5f;
        const int cols = (int) s->peaks.size();
        for (int i = 0; i < cols; ++i)   // dry (ghost) + boosted
        {
            const float x = in.getX() + in.getWidth() * (float) i / (float) cols;
            const float dv = i < (int) s->peaksDry.size() ? s->peaksDry[(size_t) i] : 0.0f;
            g.setColour (Colours::white.withAlpha (0.12f)); g.drawVerticalLine ((int) x, mid - dv * half, mid + dv * half);
        }
        Path p; p.startNewSubPath (in.getX(), mid);
        for (int i = 0; i < cols; ++i) p.lineTo (in.getX() + in.getWidth() * (float) i / (float) cols, mid - s->peaks[(size_t) i] * half);
        for (int i = cols - 1; i >= 0; --i) p.lineTo (in.getX() + in.getWidth() * (float) i / (float) cols, mid + s->peaks[(size_t) i] * half);
        p.closeSubPath();
        g.setGradientFill (ColourGradient (th.accent2, in.getX(), mid - half, th.accent, in.getX(), mid, false));
        g.fillPath (p);
        if (const float ph = proc.drum (d).playhead(); ph >= 0)
        {
            g.setColour (Colours::white.withAlpha (0.85f));
            g.drawVerticalLine ((int) (in.getX() + in.getWidth() * ph), in.getY(), in.getBottom());
        }
        g.setColour (th.text); g.setFont (Font (FontOptions (14.0f, Font::bold)));
        g.drawText (s->name.toUpperCase() + "   " + String ((double) s->audio.getNumSamples() / 44100.0, 2) + " s", wave.reduced (14, 6), Justification::topLeft);
        g.setColour (th.accent2); g.setFont (Font (FontOptions (12.0f, Font::bold)));
        g.drawText ("DRAG ME INTO FL STUDIO  >>", wave.reduced (14, 6), Justification::bottomRight);
    }
    void browse()
    {
        chooser = std::make_unique<FileChooser> ("Load your " + th.title, File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [safe = Component::SafePointer<DrumPage> (this)] (const FileChooser& fc)
                              { if (safe != nullptr && fc.getResult().existsAsFile()) safe->load (fc.getResult()); });
    }
    void load (const File& f)
    {
        if (! proc.loadDrum (d, f)) { AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, th.title, "This file could not be read."); return; }
        proc.hitDrum (d);
        refresh(); editor.repaint();
    }
    void dragOut()
    {
        const auto f = proc.exportDrum (d);
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void saveNextToOriginal()
    {
        const auto tmp = proc.exportDrum (d);
        const File orig (proc.drum (d).filePath());
        if (! tmp.existsAsFile()) return;
        auto target = orig.getParentDirectory().getNonexistentChildFile (tmp.getFileNameWithoutExtension(), ".wav");
        if (tmp.copyFileTo (target)) AlertWindow::showMessageBoxAsync (MessageBoxIconType::InfoIcon, th.title, "Saved:\n" + target.getFullPathName() + "\n\nTip: drag the waveform straight into FL.");
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    int d;
    const DrumTheme& th;
    std::vector<std::unique_ptr<ThemedKnob>> knobs;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> atts;
    std::vector<std::unique_ptr<HotButton>> satBtns, switchBtns, styleBtns, barBtns, snapBtns;
    HotButton loadBtn { lnf }, hitBtn { lnf }, keysBtn { lnf }, dragBtn { lnf }, clearBtn { lnf }, genBtn { lnf }, playPatBtn { lnf }, clearPatBtn { lnf }, undoBtn { lnf }, redoBtn { lnf }, resetBtn { lnf }, resetKnobsBtn { lnf },
              kitBtn { lnf }, saveAllBtn { lnf }, openKitBtn { lnf };
    String kitNote;
    bool hasPattern() const { return d < 3; }   // 808 / snare / hats have pattern generators; kick, open hat, perc, FX show the kit
    Slider density;
    DragFileButton dragMidi { "DRAG MIDI TO FL", Colour (0xff36ff6a) }, dragWav { "DRAG WAV TO FL", Colour (0xff36ff6a) };
    PatternEditor editor;
    std::unique_ptr<FileChooser> chooser;
    Rectangle<int> art, wave, boostPanel, patPanel;
    Image artImg; Time artTime;
    bool dragHover = false, dragFromWave = false;
    float pulse = 0, lastPh = -1;
    Point<float> shake;
    int snapSel = 1;
    const kk::DrumSample* shown = nullptr;
    int lastSig = -1;
};

// ---------------- PAIR YOUR OWN: your sounds -> BREED -> 6 children -> keys, loops, drag to DAW ----------------

// the hosted plugin's own editor in its own window (it looks exactly like itself)
// the hosted plugin's editor shown INSIDE KEYS KILLA (over the whole window, BACK bar on top) - never a
// separate desktop window, so nothing is left behind when FL Studio closes
class VstOverlay : public Component
{
public:
    explicit VstOverlay (juce::AudioPluginInstance& p, KKLookAndFeel& l) : lnf (l)
    {
        back.setButtonText ("BACK TO KEYS KILLA"); back.framed = true;
        back.onClick = [this] { setVisible (false); if (onBack) onBack(); };
        addAndMakeVisible (back);
        title = p.getName();
        editor.reset (p.createEditorIfNeeded());
        if (editor != nullptr) { native = editor->getBounds().withPosition (0, 0); addAndMakeVisible (*editor); }
        setOpaque (true);
    }
    ~VstOverlay() override { if (editor != nullptr) { removeChildComponent (editor.get()); editor.reset(); } }
    std::function<void()> onBack;   // PAIR FROM VST: the sound you changed in the plugin is taken again
    bool hasEditor() const { return editor != nullptr; }
    void paint (Graphics& g) override
    {
        g.fillAll (Colour (0xff0e0c22));
        g.setColour (Colour (0xff16152e)); g.fillRect (0, 0, getWidth(), 48);
        g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (Font (FontOptions (16.0f, Font::bold)).withExtraKerningFactor (0.1f));
        g.drawText (title.toUpperCase() + "   -   choose a sound, then CAPTURE in PAIR FROM VST", 20, 0, getWidth() - 300, 48, Justification::centredLeft);
    }
    void resized() override
    {
        back.setBounds (getWidth() - 250, 8, 236, 32);
        if (editor == nullptr) return;
        auto area = getLocalBounds().withTrimmedTop (52).reduced (6);
        const float sc = std::min (1.0f, std::min ((float) area.getWidth() / (float) std::max (1, native.getWidth()), (float) area.getHeight() / (float) std::max (1, native.getHeight())));
        editor->setTransform (AffineTransform::scale (sc));
        const int ew = roundToInt (native.getWidth() * sc), eh = roundToInt (native.getHeight() * sc);
        editor->setBounds (native.withPosition (roundToInt ((area.getX() + (area.getWidth() - ew) / 2) / sc), roundToInt (area.getY() / sc)));
        juce::ignoreUnused (eh);
    }
private:
    KKLookAndFeel& lnf;
    HotButton back { lnf };
    std::unique_ptr<AudioProcessorEditor> editor;
    Rectangle<int> native;
    String title;
};

// ---------------- PAIR FROM VST: one side (A or B) - a plugin and its sounds, in KEYS KILLA's own design ----------------
class VstSide : public Component, private ListBoxModel
{
public:
    VstSide (KeysKillaProcessor& p, KKLookAndFeel& l, int s) : proc (p), lnf (l), side (s)
    {
        pluginBox.setTextWhenNothingSelected ("choose a plugin ...");
        pluginBox.setTooltip ("The VST3 instruments on this computer (the same ones FL Studio uses)");
        pluginBox.onChange = [this] { choosePlugin(); };
        addAndMakeVisible (pluginBox);
        search.setTextToShowWhenEmpty ("search sounds ...", Colour (0xff6a6a6a));
        search.onTextChange = [this] { filter(); };
        addAndMakeVisible (search);
        list.setModel (this); list.setRowHeight (22);
        list.setColour (ListBox::backgroundColourId, Colour (0xff100e26));
        addAndMakeVisible (list);
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (prevBtn, "<", "Previous sound", [this] { step (-1); });
        btn (nextBtn, ">", "Next sound", [this] { step (1); });
        btn (bankBtn, "SAVE TO FOLDER", "Keep this sound in one of your MY SOUNDS folders", [this]
        {
            saveToFolderMenu (proc, { proc.pairParents[(size_t) side] }, &bankBtn, [safe = Component::SafePointer<VstSide> (this)] (String msg) { if (safe != nullptr) { safe->status = msg; safe->repaint(); } });
        });
        btn (takeBtn, "TAKE CURRENT", "Take the sound the plugin plays right now (after you chose one in SHOW PLUGIN)", [this] { pick (-1); });
        btn (showBtn, "SHOW PLUGIN", "Show the plugin inside KEYS KILLA: pick or tweak a sound there - BACK takes it into KEYS KILLA", [this] { if (onShowPlugin) onShowPlugin (side); });
        btn (keysBtn, "KEYS", "Play this plugin live on the keys", [this] { proc.vstKeys = side; setParamFromUi (proc, ID::playMode, (float) KeysKillaProcessor::playVst); refreshButtons(); });
    }
    std::function<void (int)> onShowPlugin;
    std::function<void()> onChanged, onAddFolder;
    void setPlugins (const StringArray& ids, const StringArray& vst2)
    {
        pluginBox.clear (dontSendNotification);
        if (! ids.isEmpty()) pluginBox.addSectionHeading ("VST3 - ready");
        for (int i = 0; i < ids.size(); ++i) pluginBox.addItem (kk::VstHost::displayName (ids[i]), i + 1);
        if (! vst2.isEmpty())
        {
            pluginBox.addSeparator(); pluginBox.addSectionHeading ("VST2 only - install their VST3 version");
            for (int i = 0; i < vst2.size(); ++i) { pluginBox.addItem (vst2[i] + "   (VST2)", 10000 + i); pluginBox.setItemEnabled (10000 + i, false); }
        }
        pluginBox.addSeparator();
        pluginBox.addItem ("+ my plugin is missing: add its folder ...", 9999);
        if (proc.host (side).loaded())
            for (int i = 0; i < ids.size(); ++i) if (kk::VstHost::displayName (ids[i]) == proc.host (side).name() || ids[i].containsIgnoreCase (proc.host (side).name())) { pluginBox.setSelectedId (i + 1, dontSendNotification); break; }
        filter();
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat();
        const Colour c1 = side == 0 ? Colour (0xffff2f6d) : Colour (0xff9b4dff), c2 = side == 0 ? Colour (0xffff8a3d) : Colour (0xff4d7dff);
        g.setGradientFill (ColourGradient (Colour (0xff1c1940), 0, 0, Colour (0xff100e26), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 10);
        g.setGradientFill (ColourGradient (c1, 0, 0, c2, r.getRight(), r.getBottom(), false)); g.drawRoundedRectangle (r.reduced (1.0f), 10, 1.8f);
        ignoreUnused (s);
        g.setGradientFill (ColourGradient (c1, 12, 6, c2, 48, 42, false)); g.setFont (Font (FontOptions (34.0f, Font::bold)));
        g.drawText (side == 0 ? "A" : "B", 12, 6, 36, 36, Justification::centred);
        g.setColour (Colour (0xffc8c4e8)); g.setFont (Font (FontOptions (11.5f)));
        const auto& h = proc.host (side);
        String info = ! h.loaded() ? String ("choose a plugin")
                    : proc.vstSounds[(size_t) side].empty() ? String ("this plugin shares no sound list: SHOW PLUGIN, pick a sound, TAKE CURRENT")
                    : String ((int) proc.vstSounds[(size_t) side].size()) + " sounds - click one = hear it and use it";
        if (status.isNotEmpty()) info = status;
        g.drawFittedText (info, statusArea, Justification::centredLeft, 2);
        if (auto& snd = proc.pairParents[(size_t) side]; snd != nullptr)
        {
            g.setColour (Colours::white); g.setFont (Font (FontOptions (13.0f, Font::bold)));
            g.drawText (snd->name, soundArea.withTrimmedLeft (84), Justification::centredLeft);
            drawPeaks (g, soundArea.withWidth (78).toFloat().reduced (2), snd->peaks, s.accent);
        }
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (10);
        auto top = r.removeFromTop (34);
        top.removeFromLeft (44);
        keysBtn.setBounds (top.removeFromRight (70)); top.removeFromRight (6);
        showBtn.setBounds (top.removeFromRight (120)); top.removeFromRight (6);
        pluginBox.setBounds (top);
        r.removeFromTop (6);
        search.setBounds (r.removeFromTop (26));
        r.removeFromTop (4);
        auto bottom = r.removeFromBottom (32);
        prevBtn.setBounds (bottom.removeFromLeft (36)); bottom.removeFromLeft (4);
        nextBtn.setBounds (bottom.removeFromLeft (36)); bottom.removeFromLeft (8);
        bankBtn.setBounds (bottom.removeFromRight (130)); bottom.removeFromRight (6);
        takeBtn.setBounds (bottom.removeFromRight (124));
        soundArea = bottom.withTrimmedLeft (4);
        statusArea = r.removeFromBottom (30);
        list.setBounds (r);
    }
    void retake() { pick (-1); }   // the sound the plugin plays now (after you changed it in SHOW PLUGIN)
    void refreshButtons() { keysBtn.selected = proc.vstKeys.load() == side && (int) proc.apvts.getRawParameterValue (ID::playMode)->load() == KeysKillaProcessor::playVst; keysBtn.repaint(); }
private:
    // ListBoxModel
    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, Graphics& g, int w, int h, bool) override
    {
        if (row < 0 || row >= (int) shown.size()) return;
        const int idx = shown[(size_t) row];
        const bool sel = idx == proc.vstSel[(size_t) side];
        if (sel) { g.setColour (lnf.skin->accent.withAlpha (0.35f)); g.fillRect (0, 0, w, h); }
        else if (row % 2) { g.setColour (Colour (0xff12112a)); g.fillRect (0, 0, w, h); }
        g.setColour (sel ? Colours::white : Colour (0xffd9d4f5)); g.setFont (Font (FontOptions (13.0f)));
        g.drawText (proc.vstSounds[(size_t) side][(size_t) idx].name, 8, 0, w - 16, h, Justification::centredLeft);
    }
    void listBoxItemClicked (int row, const MouseEvent&) override { if (row >= 0 && row < (int) shown.size()) pick (shown[(size_t) row]); }
    void choosePlugin()
    {
        if (pluginBox.getSelectedId() == 9999) { pluginBox.setSelectedId (0, dontSendNotification); if (onAddFolder) onAddFolder(); return; }
        const int i = pluginBox.getSelectedId() - 1;
        if (i < 0 || i >= proc.vstList.size()) return;
        if (onShowPlugin) onShowPlugin (-1 - side);   // close a shown editor of this side first
        status = "loading ...";
        repaint();
        const auto err = proc.loadVstSide (side, proc.vstList[i]);
        status = err;
        search.clear();
        filter();
        if (err.isEmpty()) pick (proc.vstSounds[(size_t) side].empty() ? -1 : 0);
        repaint();
    }
    void filter()
    {
        shown.clear();
        const auto q = search.getText().trim().toLowerCase();
        const auto& all = proc.vstSounds[(size_t) side];
        for (int i = 0; i < (int) all.size(); ++i) if (q.isEmpty() || all[(size_t) i].name.toLowerCase().contains (q)) shown.push_back (i);
        list.updateContent(); list.repaint();
    }
    void step (int dir)
    {
        if (shown.empty()) return;
        int row = 0;
        for (int r = 0; r < (int) shown.size(); ++r) if (shown[(size_t) r] == proc.vstSel[(size_t) side]) row = r + dir;
        row = (row + (int) shown.size()) % (int) shown.size();
        list.scrollToEnsureRowIsOnscreen (row);
        pick (shown[(size_t) row]);
    }
    void pick (int index)
    {
        if (! proc.host (side).loaded()) { status = "choose a plugin first"; repaint(); return; }
        status = proc.pickVstSound (side, index) ? String() : String ("no sound came out - try another one");
        list.repaint(); repaint();
        if (onChanged) onChanged();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int side;
    ComboBox pluginBox; TextEditor search; ListBox list;
    HotButton prevBtn { lnf }, nextBtn { lnf }, bankBtn { lnf }, takeBtn { lnf }, showBtn { lnf }, keysBtn { lnf };
    std::vector<int> shown;
    String status;
    Rectangle<int> soundArea, statusArea;
};

class PairPage : public Component, public FileDragAndDropTarget, private Timer
{
public:
    PairPage (KeysKillaProcessor& p, KKLookAndFeel& l, bool vstMode = false) : proc (p), lnf (l), breedBtn (l), vstPage (vstMode)
    {
        setWantsKeyboardFocus (true);
        proc.loadSavedBank();
        saveBtn.setButtonText ("SAVE TO BANK"); saveBtn.framed = true;
        saveBtn.setTooltip ("Keep the harvested sounds: they are saved to Documents / KEYS KILLA / Bank and come back next time");
        saveBtn.onClick = [this] { const int n = proc.saveBank(); saveBtn.setButtonText (n > 0 ? "SAVED " + String (n) : "SAVE TO BANK"); repaint(); };
        if (vstMode)
        {
            for (int k = 0; k < 2; ++k)
            {
                sides[(size_t) k] = std::make_unique<VstSide> (proc, lnf, k);
                sides[(size_t) k]->onShowPlugin = [this] (int sd) { if (sd < 0) { if (vstWin != nullptr && vstWinSide == -1 - sd) vstWin.reset(); } else openVstUi (sd); };
                sides[(size_t) k]->onChanged = [this] { repaint(); };
                sides[(size_t) k]->onAddFolder = [this]
                {
                    chooser = std::make_unique<FileChooser> ("Folder with your VST3 plugins", File::getSpecialLocation (File::globalApplicationsDirectory));
                    chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectDirectories,
                                          [safe = Component::SafePointer<PairPage> (this)] (const FileChooser& fc)
                                          {
                                              if (safe == nullptr || ! fc.getResult().isDirectory()) return;
                                              kk::VstHost::addUserFolder (fc.getResult());
                                              safe->proc.vstList.clear(); safe->scanVsts();
                                          });
                };
                addAndMakeVisible (*sides[(size_t) k]);
            }
            Component::SafePointer<PairPage> safe (this);
            Timer::callAfterDelay (50, [safe] { if (safe != nullptr) safe->scanVsts(); });
        }
        else addAndMakeVisible (saveBtn);
        breedBtn.onBreed = [this] { proc.pairBreed(); if (! proc.pairKids.empty()) proc.selectPairKid (0, true); repaint(); };
        breedBtn.setTooltip ("BREED: six children from your sounds - morph, cross, layer, shape (+ BIT / ATMOS / AMBIENT). Again = new ones.");
        addAndMakeVisible (breedBtn);
        static const char* fl[] { "ANY", "CLEAN", "BIT", "ATMOS", "AMBIENT" };
        static const char* tips[] { "a mix of everything", "pure hybrids", "crushed 8-bit / lo-fi", "dark, wide, reverb atmosphere", "slow evolving ambient pad" };
        for (int i = 0; i < kk::numFlavors; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, fl[i]);
            b->framed = true; b->setTooltip (String ("Children: ") + tips[i]);
            b->onClick = [this, i] { proc.pairFlavor = i; if (! proc.pairKids.empty()) proc.pairBreed (false); repaint(); for (auto& x : flavorBtns) x->repaint(); };   // same children, new flavour - hear the difference
            addAndMakeVisible (*b); flavorBtns.push_back (std::move (b));
        }
        diceAll.setButtonText ("ROLL ALL 4"); diceAll.framed = true;
        diceAll.setTooltip ("Dice: four random sounds from your HARVEST bank fly into the slots - then BREED");
        diceAll.onClick = [this] { for (int k = 0; k < kk::PairLab::maxParents; ++k) proc.pairDice (k); repaint(); };
        addAndMakeVisible (diceAll);
        diggaBtn.setButtonText ("HARVEST DIGGA"); diggaBtn.framed = true;
        diggaBtn.setTooltip ("Collect sounds from everything DIGGA KILLA cut (loops + one-shots)");
        diggaBtn.onClick = [this]
        {
            if (proc.harvestFromDigga() == 0)
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::InfoIcon, "HARVEST", "DIGGA has nothing yet.\nOpen DIGGA KILLA, drop a sample or a song, then come back.");
            repaint();
        };
        addAndMakeVisible (diggaBtn);
        setOpaque (true);
        startTimerHz (20);
    }
    ~PairPage() override { stopTimer(); vstWin.reset(); }   // the hosted editor goes before the plugin
    void visibilityChanged() override
    {
        if (! isVisible()) return;
        proc.pairUse = vstPage ? 2 : kk::PairLab::maxParents;
        for (auto& sd : sides) if (sd) sd->refreshButtons();
    }

    bool isInterestedInFileDrag (const StringArray& files) override { for (auto& f : files) if (isAudio (f)) return true; return false; }
    void fileDragMove (const StringArray&, int x, int y) override { hoverSlot = slotAt ({ x, y }); dropHover = hoverSlot < 0; repaint(); }
    void fileDragExit (const StringArray&) override { hoverSlot = -1; dropHover = false; repaint(); }
    void filesDropped (const StringArray& files, int x, int y) override
    {
        int slot = slotAt ({ x, y });
        hoverSlot = -1;
        if (slot < 0)   // anywhere else: HARVEST the file into the bank
        {
            for (auto& f : files) if (isAudio (f)) proc.harvestFile (File (f));
            repaint();
            return;
        }
        for (auto& f : files)
        {
            if (! isAudio (f)) continue;
            if (slot < 0 || slot >= kk::PairLab::maxParents) slot = firstFree();
            if (slot < 0) break;
            proc.loadPairParent (slot, File (f));
            slot = firstFree();
        }
        repaint();
    }

    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.fillAll (Colour (0xff12112a));
        g.setColour (s.panelEdge); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8, 1.4f);
        // ---- HARVEST: drop zone + bank by character
        if (! vstPage)
        {
            const auto dz = dropZone().toFloat();
            g.setColour (Colour (dropHover ? 0xff3a1640 : 0xff16152e)); g.fillRoundedRectangle (dz, 10);
            Path d; d.addRoundedRectangle (dz.reduced (1), 10);
            const float pat[] { 7.0f, 5.0f };
            PathStrokeType (1.6f).createDashedStroke (d, d, pat, 2);
            g.setColour (s.accent.withAlpha (dropHover ? 1.0f : 0.6f)); g.fillPath (d);
            g.setColour (Colours::white); g.setFont (serif (20.0f, true, 0.2f));
            g.drawText (vstPage ? "PAIR FROM VST" : "HARVEST", dz.withHeight (40).translated (0, 4).toNearestInt(), Justification::centred);
            g.setColour (Colour (0xffc9c0bd)); g.setFont (serif (12.5f, false, 0.08f));
            if (vstPage) g.drawFittedText (status, dz.reduced (6).withTrimmedTop (150).toNearestInt(), Justification::centred, 1);
            else g.drawFittedText (proc.harvesting() ? "listening ..." : "drop a song, a sample,\na vinyl rip - KEYS KILLA\npulls the sounds out", dz.reduced (10).withTrimmedTop (42).withHeight (60).toNearestInt(), Justification::centred, 3);
            for (int c = 0; c < kk::numCats; ++c)
            {
                const auto col = bankCol (c).toFloat();
                g.setColour (Colour (0xff0e0d20)); g.fillRoundedRectangle (col, 6);
                g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (col, 6, 1.0f);
                g.setColour (s.accent); g.setFont (Font (FontOptions (11.0f, Font::bold)).withExtraKerningFactor (0.08f));
                g.drawText (kk::harvestCatShort (c), col.withHeight (20).toNearestInt(), Justification::centred);
            }
            int row[kk::numCats] {};
            for (int i = 0; i < (int) proc.bank.size(); ++i)
            {
                const auto& it = proc.bank[(size_t) i];
                if (row[it.cat] >= kk::Harvest::perCategory) continue;
                const auto r = chip (it.cat, row[it.cat]++).toFloat();
                g.setColour (i == armed ? s.accent.withAlpha (0.6f) : Colour (0xff1e1c3c)); g.fillRoundedRectangle (r, 4);
                drawPeaks (g, r.reduced (3, 2).withWidth (r.getWidth() * 0.45f), it.sound->peaks, s.accent.withAlpha (0.8f));
                g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (Font (FontOptions (10.5f)));
                g.drawText (it.sound->pitched && it.cat != kk::catDrum && it.cat != kk::catFx ? MidiMessage::getMidiNoteName (it.sound->rootNote, true, true, 5) : String (i + 1),
                            r.withTrimmedLeft (r.getWidth() * 0.5f).toNearestInt(), Justification::centred);
            }
            if (proc.bank.empty())
            {
                g.setColour (Colour (0xff6a6290)); g.setFont (serif (13.0f, false, 0.1f));
                g.drawFittedText (vstPage ? "your sound bank is empty - choose a plugin, GRAB ALL ITS SOUNDS" : "your sound bank is empty - drop a song on HARVEST", bankArea().withTrimmedTop (60), Justification::centredTop, 2);
            }
        }
        // parents
        for (int k = 0; k < (vstPage ? 0 : kk::PairLab::maxParents); ++k)
        {
            const auto r = slot (k).toFloat();
            const auto& p = proc.pairParents[(size_t) k];
            g.setColour (Colour (0xff16152e)); g.fillRoundedRectangle (r, 8);
            if (hoverSlot == k) drawGlowFrame (g, r, s.accent, 8);
            else
            {
                Path d; d.addRoundedRectangle (r.reduced (1), 8);
                const float pat[] { 6.0f, 4.0f };
                if (p == nullptr) PathStrokeType (1.2f).createDashedStroke (d, d, pat, 2), g.setColour (Colour (0xff4a4478)), g.fillPath (d);
                else { g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (r, 8, 1.2f); }
            }
            g.setColour (Colour (0xffaaa4cf)); g.setFont (serif (12.0f, false, 0.25f));
            g.drawText ("SOUND " + String (k + 1), r.reduced (12, 6).withHeight (16).toNearestInt(), Justification::centredLeft);
            {   // dice: a random sound flies in
                const auto db = diceBox (k).toFloat();
                g.setColour (Colour (0xff2e1846)); g.fillRoundedRectangle (db, 5);
                g.setColour (s.accent); g.drawRoundedRectangle (db, 5, 1.2f);
                g.setColour (Colours::white);
                for (auto pt : { Point<float> (0.3f, 0.3f), Point<float> (0.7f, 0.3f), Point<float> (0.5f, 0.5f), Point<float> (0.3f, 0.7f), Point<float> (0.7f, 0.7f) })
                    g.fillEllipse (db.getX() + db.getWidth() * pt.x - 2.2f, db.getY() + db.getHeight() * pt.y - 2.2f, 4.4f, 4.4f);
            }
            if (p == nullptr)
            {
                g.setColour (Colour (0xffc8c4e8)); g.setFont (serif (15.0f, false, 0.12f));
                g.drawFittedText ("drop a WAV here\nor roll the dice", r.toNearestInt(), Justification::centred, 2);
                continue;
            }
            drawPeaks (g, r.reduced (12, 22).withTrimmedBottom (4).withTrimmedRight (30), p->peaks, s.accent.withAlpha (0.8f));
            g.setColour (Colours::white); g.setFont (serif (13.5f, false, 0.05f));
            g.drawText (p->name, r.reduced (12, 6).removeFromBottom (18).toNearestInt(), Justification::centredLeft);
            g.setColour (s.accent); g.setFont (serif (12.0f, false, 0.1f));
            g.drawText (p->pitched ? MidiMessage::getMidiNoteName (p->rootNote, true, true, 5) : String ("DRUM / FX"), r.reduced (12, 6).withHeight (16).withTrimmedRight (26).toNearestInt(), Justification::centredRight);
            g.setColour (Colour (0xffaaa4cf)); g.drawText ("x", closeBox (k), Justification::centred);
        }
        // the family line
        g.setColour (s.accent.withAlpha (0.45f));
        const auto bc = breedBtn.getBounds().toFloat();
        if (vstPage && sides[0] && sides[1])   // A -> BREED <- B
        {
            g.drawLine ((float) sides[0]->getRight(), bc.getCentreY(), bc.getX() + 4, bc.getCentreY(), 2.0f);
            g.drawLine (bc.getRight() - 4, bc.getCentreY(), (float) sides[1]->getX(), bc.getCentreY(), 2.0f);
        }
        else
        {
            const float joinY = (float) slot (0).getBottom() + 4.0f;
            g.drawLine ((float) slot (0).getCentreX(), joinY, (float) slot (3).getCentreX(), joinY, 1.5f);
            for (int k = 0; k < 4; ++k) g.drawLine ((float) slot (k).getCentreX(), (float) slot (k).getBottom(), (float) slot (k).getCentreX(), joinY, 1.5f);
            g.drawLine (bc.getCentreX(), joinY, bc.getCentreX(), bc.getY() + 6, 1.5f);
        }
        const float busY = (float) kid (0).getY() - 2.0f;
        g.drawLine (bc.getCentreX(), bc.getBottom() - 6, bc.getCentreX(), busY, 1.5f);
        g.drawLine ((float) kid (0).getCentreX(), busY, (float) kid (5).getCentreX(), busY, 1.5f);
        for (int k = 0; k < 6; ++k) g.drawLine ((float) kid (k).getCentreX(), busY, (float) kid (k).getCentreX(), (float) kid (k).getY(), 1.5f);
        g.setColour (Colour (0xffaaa4cf)); g.setFont (serif (12.0f, false, 0.25f));
        if (! vstPage) g.drawText ("CHILDREN", flavorBtns.front()->getX(), flavorBtns.front()->getY() - 16, 200, 14, Justification::centredLeft);
        if (note.isNotEmpty()) { g.setColour (Colour (0xff36ff6a)); g.setFont (Font (FontOptions (13.0f, Font::bold))); g.drawText (note, getWidth() / 2 + 110, kid (0).getY() - 26, getWidth() / 2 - 130, 18, Justification::centredRight); }
        for (int i = 0; i < (int) flavorBtns.size(); ++i) { flavorBtns[(size_t) i]->selected = i == proc.pairFlavor; }
        // children
        for (int k = 0; k < 6; ++k)
        {
            const auto r = kid (k).toFloat();
            const bool has = k < (int) proc.pairKids.size();
            const bool sel = has && k == proc.pairSel;
            g.setGradientFill (ColourGradient (Colour (sel ? 0xff3a1640 : 0xff1c1940), r.getX(), r.getY(), Colour (sel ? 0xff1a0c26 : 0xff100e26), r.getX(), r.getBottom(), false)); g.fillRoundedRectangle (r, 8);
            if (sel) drawGlowFrame (g, r, s.accent, 8); else { g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (r, 8, 1.2f); }
            g.setColour (Colour (0xffaaa4cf)); g.setFont (serif (12.0f, false, 0.25f));
            g.drawText ("CHILD " + String (k + 1), r.reduced (10, 6).withHeight (16).toNearestInt(), Justification::centredLeft);
            if (! has)
            {
                g.setColour (Colour (0xff6a6290)); g.setFont (serif (13.0f, false, 0.12f));
                g.drawText (proc.pairParents[0] || proc.pairParents[1] || proc.pairParents[2] || proc.pairParents[3] ? "press BREED" : (vstPage ? "choose a sound in A and B" : "drop your sounds"), r.toNearestInt(), Justification::centred);
                continue;
            }
            const auto& c = *proc.pairKids[(size_t) k];
            drawPeaks (g, r.reduced (10, 26).withTrimmedBottom (40), c.peaks, (sel ? s.accent : s.accent.withAlpha (0.6f)));
            g.setColour (Colours::white); g.setFont (serif (12.5f, false, 0.08f));
            g.drawFittedText (c.method, r.reduced (10, 6).removeFromBottom (54).removeFromTop (18).toNearestInt(), Justification::centredLeft, 1);
            g.setColour (s.accent); g.setFont (serif (11.5f, false, 0.1f));
            g.drawText (c.pitched ? MidiMessage::getMidiNoteName (c.rootNote, true, true, 5) : String ("FX"), r.reduced (10, 6).withHeight (16).toNearestInt(), Justification::centredRight);
            // LOOP button + drag hint
            const auto lb = loopBox (k).toFloat();
            const bool looping = proc.loopPlaying() && proc.pairLoopKid == k;
            g.setColour (looping ? s.accent : Colour (0xff2a2450)); g.fillRoundedRectangle (lb, 5);
            g.setColour (Colours::white); g.setFont (serif (12.0f, false, 0.2f));
            g.drawText (looping ? "STOP LOOP" : "LOOP", lb.toNearestInt(), Justification::centred);
            g.setColour (Colour (0xffaaa4cf)); g.setFont (serif (10.5f, false, 0.15f));
            g.drawText (looping ? "drag: MIDI" : "drag: WAV", r.reduced (10, 4).removeFromBottom (14).toNearestInt(), Justification::centredRight);
            {   // SAVE to a folder
                const auto sb = saveBox (k).toFloat();
                g.setColour (Colour (0xff1c2a1e)); g.fillRoundedRectangle (sb, 4);
                g.setColour (Colour (0xff36ff6a)); g.drawRoundedRectangle (sb, 4, 1.0f);
                g.setFont (Font (FontOptions (10.5f, Font::bold))); g.drawText ("SAVE", sb.toNearestInt(), Justification::centred);
            }
        }
        g.setColour (Colour (0xffaaa4cf)); g.setFont (serif (12.0f, false, 0.25f));
        g.drawText (vstPage ? "1. A + B: CHOOSE A PLUGIN   2. CLICK A SOUND = HEAR IT   3. BREED   4. CHILD: CLICK = KEYS, LOOP = TRAP MELODY, DRAG INTO FL = WAV / MIDI"
                            : "BANK: CLICK = HEAR, DOUBLE-CLICK = INTO A SLOT, DEL = DELETE.   CHILD: CLICK = PLAY ON KEYS, LOOP = TRAP MELODY, DRAG INTO FL = WAV / MIDI",
                    Rectangle<int> (20, getHeight() - 22, getWidth() - 40, 18), Justification::centred);
    }
    void resized() override
    {
        breedBtn.setBounds (getWidth() / 2 - 34, 282, 68, 68);
        for (int i = 0; i < (int) flavorBtns.size(); ++i) flavorBtns[(size_t) i]->setBounds (14 + i * 84, 306, 80, 28);
        diggaBtn.setBounds (24, 104, 210, 30); saveBtn.setBounds (24, 138, 210, 30);
        diggaBtn.setVisible (! vstPage);
        diceAll.setBounds (getWidth() - 214, 304, 200, 32);
        diceAll.setVisible (! vstPage);
        saveBtn.setVisible (! vstPage);
        if (vstPage)   // A  -  BREED  -  B
        {
            const int w = getWidth(), sw = (w - 28 - 200) / 2;
            if (sides[0]) sides[0]->setBounds (14, 10, sw, 284);
            if (sides[1]) sides[1]->setBounds (w - 14 - sw, 10, sw, 284);
            breedBtn.setBounds (w / 2 - 60, 92, 120, 120);
            for (int i = 0; i < (int) flavorBtns.size(); ++i) flavorBtns[(size_t) i]->setBounds (w / 2 - 92 + (i % 3) * 62 - (i >= 3 ? -31 : 0), 222 + (i / 3) * 32, 58, 26);
        }
    }
    void mouseDown (const MouseEvent& e) override { downKid = kidAt (e.getPosition()); dragDone = false; }
    void mouseDrag (const MouseEvent& e) override
    {
        if (downKid < 0 || dragDone || e.getDistanceFromDragStart() < 6) return;
        dragDone = true;
        const bool looping = proc.loopPlaying() && proc.pairLoopKid == downKid;
        const auto f = looping ? proc.exportPairLoop() : proc.exportPairKid (downKid);
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    int bankAt (Point<int> p) const
    {
        if (vstPage) return -1;
        int row[kk::numCats] {};
        for (int i = 0; i < (int) proc.bank.size(); ++i)
        {
            const int c = proc.bank[(size_t) i].cat;
            if (row[c] >= kk::Harvest::perCategory) continue;
            if (chip (c, row[c]++).contains (p)) return i;
        }
        return -1;
    }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        if (const int b = bankAt (e.getPosition()); b >= 0) { proc.bankToPair (b); armed = -1; repaint(); }
    }
    // DELETE / BACKSPACE: the selected bank sound goes, the next one is selected (fast clean-up)
    bool keyPressed (const KeyPress& k) override
    {
        if ((k == KeyPress::deleteKey || k == KeyPress::backspaceKey) && armed >= 0 && armed < (int) proc.bank.size())
        {
            const int cat = proc.bank[(size_t) armed].cat;
            proc.removeFromBank (armed);
            if (armed >= (int) proc.bank.size() || proc.bank[(size_t) armed].cat != cat) armed = -1;
            repaint();
            return true;
        }
        return false;
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (dragDone) return;
        const auto pos = e.getPosition();
        if (const int b = bankAt (pos); b >= 0)
        {
            if (e.mods.isPopupMenu())   // right-click a sound: where to, or delete it
            {
                PopupMenu m;
                m.addItem (1, "Hear it");
                for (int k = 0; k < kk::PairLab::maxParents; ++k) m.addItem (10 + k, "Into SOUND " + String (k + 1));
                PopupMenu mv;
                for (int c = 0; c < kk::numCats; ++c) { const int cat = kk::harvestCatAt (c); mv.addItem (100 + cat, kk::harvestCatName (cat), cat != proc.bank[(size_t) b].cat); }
                m.addSubMenu ("Move to", mv);
                m.addSeparator();
                m.addItem (2, "Delete from the bank   (Del)");
                m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe = Component::SafePointer<PairPage> (this), b] (int r)
                {
                    if (safe == nullptr || r == 0 || b >= (int) safe->proc.bank.size()) return;
                    if (r == 1) safe->proc.auditionBank (b);
                    else if (r == 2) { safe->proc.removeFromBank (b); safe->armed = -1; }
                    else if (r >= 100) { safe->proc.moveInBank (b, r - 100); safe->armed = -1; }
                    else if (r >= 10) safe->proc.bankToPair (b, r - 10);
                    safe->repaint();
                });
                return;
            }
            armed = b; proc.auditionBank (b); grabKeyboardFocus(); repaint(); return;
        }
        for (int c = 0; c < kk::numCats; ++c)   // click a shelf name: clear that shelf
            if (bankCol (c).withHeight (20).contains (pos))
            {
                PopupMenu m;
                m.addItem (1, "Clear " + String (kk::harvestCatShort (c)) + " (deletes its sounds from the bank)");
                m.showMenuAsync (PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe = Component::SafePointer<PairPage> (this), c] (int r)
                { if (safe != nullptr && r == 1) { safe->proc.clearShelf (c); safe->armed = -1; safe->repaint(); } });
                return;
            }
        if (! vstPage && dropZone().contains (pos) && ! diggaBtn.getBounds().contains (pos) && ! saveBtn.getBounds().contains (pos)) { browseHarvest(); return; }
        for (int k = 0; k < (vstPage ? 0 : kk::PairLab::maxParents); ++k)
        {
            if (armed >= 0 && slot (k).contains (pos)) { proc.bankToPair (armed, k); armed = -1; repaint(); return; }
            if (proc.pairParents[(size_t) k] != nullptr && closeBox (k).contains (pos)) { proc.clearPairParent (k); repaint(); return; }
            if (diceBox (k).contains (pos)) { proc.pairDice (k); repaint(); return; }
            if (slot (k).contains (pos)) { browse (k); return; }
        }
        for (int k = 0; k < (int) proc.pairKids.size() && k < 6; ++k)
        {
            if (loopBox (k).contains (pos)) { proc.togglePairLoop (k); repaint(); return; }
            if (saveBox (k).contains (pos))
            {
                saveToFolderMenu (proc, { proc.pairKids[(size_t) k] }, this, [safe = Component::SafePointer<PairPage> (this)] (String msg) { if (safe != nullptr) { safe->note = msg; safe->repaint(); } });
                return;
            }
            if (kid (k).contains (pos)) { proc.selectPairKid (k, true); repaint(); return; }
        }
    }
private:
    static bool isAudio (const String& f) { return File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg"); }
    // HARVEST bank on top, the pairing below
    Rectangle<int> dropZone() const { return { 14, 10, vstPage ? 430 : 230, 168 }; }
    Rectangle<int> bankArea() const { const int x = dropZone().getRight() + 14; return { x, 10, getWidth() - x - 14, 168 }; }
    void scanVsts()
    {
        if (proc.vstList.isEmpty()) proc.vstList = proc.vst.listInstalled();
        const auto old2 = kk::VstHost::vst2Only (proc.vstList);
        for (auto& sd : sides) if (sd) sd->setPlugins (proc.vstList, old2);
        repaint();
    }
    // a plugin's own window, shown INSIDE KEYS KILLA (only for plugins that share no sound list)
    void openVstUi (int side)
    {
        auto* top = getTopLevelComponent();
        if (auto* inst = proc.host (side).instance(); inst != nullptr && top != nullptr)
        {
            if (vstWin != nullptr && vstWinSide != side) vstWin.reset();
            if (vstWin == nullptr && inst->hasEditor())
            {
                vstWin = std::make_unique<VstOverlay> (*inst, lnf);
                vstWinSide = side;
                vstWin->onBack = [safe = Component::SafePointer<PairPage> (this), side]
                { if (safe != nullptr && safe->sides[(size_t) side]) safe->sides[(size_t) side]->retake(); };
                top->addChildComponent (*vstWin);
            }
            if (vstWin != nullptr) { vstWin->setBounds (top->getLocalBounds()); vstWin->setVisible (true); vstWin->toFront (true); }
        }
    }
    // c = category; its column follows the display order (BASS KEYS PLUCK PAD STRINGS BRASS LEAD VOX DRUMS FX)
    Rectangle<int> bankCol (int c) const { const auto b = bankArea(); const int w = (b.getWidth() - (kk::numCats - 1) * 6) / kk::numCats; const int col = kk::harvestColumnOf (c); return { b.getX() + col * (w + 6), b.getY(), w, b.getHeight() }; }
    Rectangle<int> chip (int c, int row) const { const auto col = bankCol (c); return { col.getX() + 4, col.getY() + 22 + row * 18, col.getWidth() - 8, 16 }; }
    Rectangle<int> slot (int k) const { const int w = (getWidth() - 28 - 3 * 12) / 4; return { 14 + k * (w + 12), 190, w, 88 }; }
    Rectangle<int> closeBox (int k) const { const auto r = slot (k); return { r.getRight() - 26, r.getY() + 4, 22, 18 }; }
    Rectangle<int> diceBox (int k) const { const auto r = slot (k); return { r.getRight() - 34, r.getBottom() - 30, 26, 24 }; }
    Rectangle<int> kid (int k) const { const int w = (getWidth() - 28 - 5 * 10) / 6; return { 14 + k * (w + 10), 352, w, getHeight() - 352 - 24 }; }
    Rectangle<int> loopBox (int k) const { const auto r = kid (k); return { r.getX() + 10, r.getBottom() - 40, r.getWidth() - 20, 22 }; }
    Rectangle<int> saveBox (int k) const { const auto r = kid (k); return { r.getRight() - 62, r.getY() + 22, 54, 18 }; }
    int slotAt (Point<int> p) const { if (vstPage) return -1; for (int k = 0; k < kk::PairLab::maxParents; ++k) if (slot (k).contains (p)) return k; return -1; }
    int kidAt (Point<int> p) const { for (int k = 0; k < 6; ++k) if (kid (k).contains (p) && ! loopBox (k).contains (p) && ! saveBox (k).contains (p)) return k < (int) proc.pairKids.size() ? k : -1; return -1; }
    int firstFree() const { for (int k = 0; k < kk::PairLab::maxParents; ++k) if (proc.pairParents[(size_t) k] == nullptr) return k; return -1; }
    void browse (int k)
    {
        chooser = std::make_unique<FileChooser> ("Your sound for SOUND " + String (k + 1), File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [safe = Component::SafePointer<PairPage> (this), k] (const FileChooser& fc)
                              { if (safe != nullptr && fc.getResult().existsAsFile()) { safe->proc.loadPairParent (k, fc.getResult()); safe->repaint(); } });
    }
    void browseHarvest()
    {
        chooser = std::make_unique<FileChooser> ("HARVEST: a song or sample", File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::canSelectMultipleItems,
                              [safe = Component::SafePointer<PairPage> (this)] (const FileChooser& fc)
                              { if (safe != nullptr) { for (auto& f : fc.getResults()) safe->proc.harvestFile (f); safe->repaint(); } });
    }
    void timerCallback() override
    {
        if (! isVisible()) return;
        if (proc.harvesting()) repaint (dropZone());
        if (grabNext >= 0)   // GRAB SOUNDS: one preset per tick, the UI stays alive
        {
            if (grabNext >= grabTotal || ! proc.vst.loaded())
            {
                grabNext = -1; grabBtn.setButtonText ("GRAB ALL ITS SOUNDS");
                status = String (grabbed) + " sounds from " + proc.vst.name() + " in your bank - pair them below";
            }
            else
            {
                const auto n = proc.captureVstProgram (grabNext++, noteBox.getSelectedId());
                if (n.isNotEmpty()) ++grabbed;
                status = "grabbing " + String (grabNext) + " / " + String (grabTotal) + "   " + n;
            }
            repaint();
        }
        if (breedBtn.flash > 0) { breedBtn.flash = std::max (0.0f, breedBtn.flash - 0.08f); breedBtn.repaint(); }
        const int v = proc.pairVer.load() * 3 + (proc.loopPlaying() ? 1 : 0) + (int) proc.bank.size() * 7919;
        if (v != lastVer) { lastVer = v; repaint(); }
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    TreeBreedButton breedBtn;
    HotButton diggaBtn { lnf }, diceAll { lnf }, saveBtn { lnf }, uiBtn { lnf }, capBtn { lnf }, keysBtn { lnf };
    ComboBox vstBox, noteBox, shelfBox;
    HotButton folderBtn { lnf }, grabBtn { lnf };
    int grabNext = -1, grabTotal = 0, grabbed = 0;
    std::unique_ptr<VstOverlay> vstWin;
    int vstWinSide = -1;
    std::array<std::unique_ptr<VstSide>, 2> sides;
    String status;
    bool vstPage = false;
    std::vector<std::unique_ptr<HotButton>> flavorBtns;
    std::unique_ptr<FileChooser> chooser;
    int hoverSlot = -1, downKid = -1, lastVer = -1, armed = -1;
    String note;
    bool dropHover = false;
    bool dragDone = false;
};

// DRAG TO DAW: grab the current sound and drop it into FL as a WAV (the empty right column put to work)
class DragToDaw : public Component, public SettableTooltipClient
{
public:
    DragToDaw (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        setTooltip ("DRAG TO DAW: drag this into FL Studio - you get the current sound as a WAV (one note, C5). Click: hear it.");
        setMouseCursor (MouseCursor::DraggingHandCursor);
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (6);
        g.setColour (Colour (0xcc0b0808)); g.fillRoundedRectangle (r, 10);
        Path dash; dash.addRoundedRectangle (r.reduced (3), 9);
        const float pat[] { 7.0f, 5.0f };
        PathStrokeType (1.6f).createDashedStroke (dash, dash, pat, 2);
        g.setColour (s.accent.withAlpha (hover ? 0.95f : 0.55f)); g.fillPath (dash);
        // a little waveform that "walks" out of the box
        auto w = r.reduced (18).removeFromTop (r.getHeight() * 0.42f);
        Path wave;
        for (int i = 0; i <= 40; ++i)
        {
            const float x = w.getX() + w.getWidth() * (float) i / 40.0f;
            const float env = std::exp (-(float) i / 14.0f);
            const float y = w.getCentreY() - std::sin ((float) i * 0.9f) * env * w.getHeight() * 0.45f;
            if (i == 0) wave.startNewSubPath (x, y); else wave.lineTo (x, y);
        }
        g.setColour (Colours::white.withAlpha (0.9f)); g.strokePath (wave, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
        // arrow
        const float ax = r.getCentreX(), ay = w.getBottom() + 14 + (hover ? 4.0f : 0.0f);
        Path arrow; arrow.addArrow ({ ax, ay, ax, ay + 30 }, 4.0f, 16.0f, 12.0f);
        g.setColour (s.accent); g.fillPath (arrow);
        g.setColour (Colours::white); g.setFont (Font (FontOptions (19.0f, Font::bold)).withExtraKerningFactor (0.12f));
        g.drawFittedText ("DRAG\nTO DAW", Rectangle<float> (r.getX(), ay + 40, r.getWidth(), 46).toNearestInt(), Justification::centred, 2);
        g.setColour (Colour (0xffaaa4cf)); g.setFont (Font (FontOptions (11.5f, Font::italic)));
        g.drawFittedText (busy ? "printing..." : "it's yours now", Rectangle<float> (r.getX(), r.getBottom() - 26, r.getWidth(), 18).toNearestInt(), Justification::centred, 1);
    }
    void mouseEnter (const MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const MouseEvent&) override { hover = false; repaint(); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragged || e.getDistanceFromDragStart() < 5) return;
        dragged = true; busy = true; repaint();
        const auto f = proc.exportSoundWav (72);
        busy = false; repaint();
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (! dragged && e.getDistanceFromDragStart() < 5) proc.previewNote = 72;
        dragged = false;
    }
private:
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    bool hover = false, dragged = false, busy = false;
};

// left column: BREED LAB / FAMILY TREE, then the melody extras VOODOO KILLA / EFFECTOR KILLA / DIGGA KILLA
// (-1 = a drum page of the bottom row is open)
// left tiles: each one a small colour badge (its own two colours) with a white symbol
static Colour tileCol (int k, int which)
{
    static const uint32 c[8][2] { { 0xffff2f6d, 0xffff8a3d },   // BREED LAB    pink -> orange
                                  { 0xffb04dff, 0xffff4fa8 },   // FAMILY TREE  violet -> pink
                                  { 0xff22d3ee, 0xff4d7dff },   // PAIR OWN     cyan -> blue
                                  { 0xff4d7dff, 0xff9b4dff },   // PAIR VST     blue -> violet
                                  { 0xffffb020, 0xffff6a3d },   // MY SOUNDS    gold -> orange
                                  { 0xff7c4dff, 0xff2b2f9e },   // VOODOO       violet -> night
                                  { 0xffff3fd2, 0xff9b4dff },   // EFFECTOR     magenta -> violet
                                  { 0xff2ee6a6, 0xff1a9dff } }; // DIGGA        mint -> blue
    return Colour (c[jlimit (0, 7, k)][jlimit (0, 1, which)]);
}
static void drawTileIcon (Graphics& g, int k, Rectangle<float> r, bool lit)
{
    // badge
    const auto c1 = tileCol (k, 0), c2 = tileCol (k, 1);
    if (lit) { g.setColour (c1.withAlpha (0.35f)); g.fillRoundedRectangle (r.expanded (3), 10); }
    g.setGradientFill (ColourGradient (c1, r.getX(), r.getY(), c2, r.getRight(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 8);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.35f), 0, r.getY(), Colours::white.withAlpha (0.0f), 0, r.getCentreY(), false));
    g.fillRoundedRectangle (r.reduced (1.5f).withHeight (r.getHeight() * 0.5f), 6);
    g.setColour (Colours::white.withAlpha (0.25f)); g.drawRoundedRectangle (r.reduced (0.5f), 8, 1.0f);

    // symbol
    const auto q = r.reduced (r.getWidth() * 0.22f);
    const float cx = q.getCentreX(), cy = q.getCentreY(), w = q.getWidth();
    PathStrokeType st (1.8f, PathStrokeType::curved, PathStrokeType::rounded);
    Path p;
    g.setColour (Colours::white);
    switch (k)
    {
        case 0:   // BREED LAB: two sounds melt into one, with a spark
        {
            const float rr = w * 0.30f;
            p.addEllipse (cx - rr * 1.55f, cy - rr + 1, rr * 2, rr * 2); p.addEllipse (cx - rr * 0.45f, cy - rr + 1, rr * 2, rr * 2);
            g.strokePath (p, st);
            g.setColour (Colours::white.withAlpha (0.55f));
            g.fillEllipse (cx - rr * 0.45f, cy - rr * 0.55f + 1, rr * 0.9f, rr * 1.1f);
            g.setColour (Colours::white);
            Path sp; const Point<float> sc (q.getRight() - 1, q.getY() + 1); const float sr = 4.0f;
            sp.startNewSubPath (sc.x, sc.y - sr); sp.quadraticTo (sc, { sc.x + sr, sc.y }); sp.quadraticTo (sc, { sc.x, sc.y + sr }); sp.quadraticTo (sc, { sc.x - sr, sc.y }); sp.quadraticTo (sc, { sc.x, sc.y - sr });
            g.fillPath (sp);
            break;
        }
        case 1:   // FAMILY TREE: one sound, three children, soft branches
        {
            const Point<float> top (cx, q.getY() + 2);
            const Point<float> kids[] { { q.getX() + 1, q.getBottom() - 2 }, { cx, q.getBottom() - 2 }, { q.getRight() - 1, q.getBottom() - 2 } };
            for (auto& kd : kids) { p.startNewSubPath (top); p.cubicTo ({ top.x, cy + 1 }, { kd.x, cy - 1 }, { kd.x, kd.y - 3 }); }
            g.strokePath (p, st);
            g.fillEllipse (top.x - 3.5f, top.y - 3.5f, 7, 7);
            for (auto& kd : kids) g.fillEllipse (kd.x - 2.6f, kd.y - 2.6f, 5.2f, 5.2f);
            break;
        }
        case 2:   // PAIR YOUR OWN: two waves crossing
        {
            Path p2;
            for (int i = 0; i <= 24; ++i)
            {
                const float t = (float) i / 24.0f, x = q.getX() + t * w;
                const float y1 = cy + std::sin (t * 6.283f) * w * 0.32f, y2 = cy - std::sin (t * 6.283f) * w * 0.32f;
                if (i == 0) { p.startNewSubPath (x, y1); p2.startNewSubPath (x, y2); } else { p.lineTo (x, y1); p2.lineTo (x, y2); }
            }
            g.strokePath (p, st);
            g.setColour (Colours::white.withAlpha (0.55f)); g.strokePath (p2, st);
            break;
        }
        case 3:   // PAIR FROM VST: a plug with its cable
        {
            const auto body = Rectangle<float> (cx - w * 0.30f, q.getY() + w * 0.22f, w * 0.60f, w * 0.40f);
            g.fillRoundedRectangle (body, 3);
            g.fillRoundedRectangle (body.getX() + w * 0.10f, q.getY() - 1, 2.6f, w * 0.26f, 1);
            g.fillRoundedRectangle (body.getRight() - w * 0.10f - 2.6f, q.getY() - 1, 2.6f, w * 0.26f, 1);
            p.startNewSubPath (cx, body.getBottom()); p.cubicTo ({ cx, q.getBottom() }, { q.getX(), q.getBottom() - 4 }, { q.getX() - 1, q.getBottom() + 1 });
            g.strokePath (p, st);
            break;
        }
        case 4:   // MY SOUNDS: folder with a waveform
        {
            p.startNewSubPath (q.getX(), q.getBottom()); p.lineTo (q.getX(), q.getY() + 2); p.lineTo (q.getX() + w * 0.35f, q.getY() + 2);
            p.lineTo (q.getX() + w * 0.45f, q.getY() + w * 0.16f); p.lineTo (q.getRight(), q.getY() + w * 0.16f); p.lineTo (q.getRight(), q.getBottom()); p.closeSubPath();
            g.strokePath (p, st);
            const float h[] { 0.10f, 0.22f, 0.32f, 0.16f, 0.26f, 0.08f };
            for (int i = 0; i < 6; ++i)
            {
                const float x = q.getX() + w * (0.18f + 0.13f * (float) i), my = cy + w * 0.16f;
                g.fillRoundedRectangle (x - 0.9f, my - h[i] * w * 0.6f, 1.8f, h[i] * w * 1.2f, 0.9f);
            }
            break;
        }
        case 5:   // VOODOO KILLA (half time): crescent moon and stars
        {
            Path moon; moon.addEllipse (q.getX(), q.getY() + 1, w * 0.86f, w * 0.86f);
            Path both (moon); both.addEllipse (q.getX() + w * 0.32f, q.getY() - w * 0.12f, w * 0.80f, w * 0.80f);
            both.setUsingNonZeroWinding (false);
            g.saveState(); g.reduceClipRegion (moon); g.fillPath (both); g.restoreState();
            auto star = [&g] (float x, float y, float sr) { Path sp; sp.startNewSubPath (x, y - sr); sp.quadraticTo ({ x, y }, { x + sr, y }); sp.quadraticTo ({ x, y }, { x, y + sr }); sp.quadraticTo ({ x, y }, { x - sr, y }); sp.quadraticTo ({ x, y }, { x, y - sr }); g.fillPath (sp); };
            star (q.getRight() - 1, q.getBottom() - 4, 3.4f); star (q.getRight() - 4, q.getY() + 3, 2.4f);
            break;
        }
        case 6:   // EFFECTOR KILLA: a knob with its scale
        {
            const float rr = w * 0.36f;
            g.drawEllipse (cx - rr, cy - rr + 1, rr * 2, rr * 2, 1.8f);
            const float ang = -0.8f;
            g.drawLine (cx, cy + 1, cx + std::sin (ang) * rr * 0.8f, cy + 1 - std::cos (ang) * rr * 0.8f, 2.0f);
            for (int i = 0; i < 7; ++i)
            {
                const float a = -2.4f + 0.8f * (float) i, r1 = rr + 3.0f, r2 = rr + 5.0f;
                g.setColour (Colours::white.withAlpha (i <= 2 ? 1.0f : 0.45f));
                g.drawLine (cx + std::sin (a) * r1, cy + 1 - std::cos (a) * r1, cx + std::sin (a) * r2, cy + 1 - std::cos (a) * r2, 1.4f);
            }
            break;
        }
        default:  // DIGGA KILLA: a record from the crate
        {
            const float rr = w * 0.50f;
            g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 1.8f);
            g.setColour (Colours::white.withAlpha (0.45f));
            g.drawEllipse (cx - rr * 0.72f, cy - rr * 0.72f, rr * 1.44f, rr * 1.44f, 1.0f);
            g.setColour (Colours::white); g.fillEllipse (cx - rr * 0.36f, cy - rr * 0.36f, rr * 0.72f, rr * 0.72f);
            g.setColour (tileCol (k, 1)); g.fillEllipse (cx - 1.5f, cy - 1.5f, 3, 3);
            g.setColour (Colours::white);
            p.addCentredArc (cx, cy, rr * 0.86f, rr * 0.86f, 0.0f, -0.9f, -0.2f, true);
            g.strokePath (p, PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded));
            break;
        }
    }
}

class LabSwitch : public Component, public SettableTooltipClient, public FileDragAndDropTarget, private Timer
{
public:
    // drag a sound (from DIGGA, FL's browser, a folder) onto the tiles: it lands in PAIR YOUR OWN - from any page
    std::function<int (const File&)> onDropToPair;   // returns the slot (1..4) or 0
    bool isInterestedInFileDrag (const StringArray& f) override { for (auto& x : f) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true; return false; }
    void fileDragEnter (const StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const StringArray&) override { dropHover = false; repaint(); }
    void filesDropped (const StringArray& files, int, int) override
    {
        dropHover = false;
        int n = 0, last = 0;
        for (auto& x : files) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg") && onDropToPair) if (const int sl = onDropToPair (File (x)); sl > 0) { ++n; last = sl; }
        flashText = n > 1 ? String (n) + " SOUNDS\nIN PAIR" : n == 1 ? "IN PAIR\nSLOT " + String (last) : "COULD NOT\nREAD IT";
        flash = 1.0f; startTimerHz (30); repaint();
    }
    explicit LabSwitch (KKLookAndFeel& l) : lnf (l)
    {
        setTooltip ("BREED LAB: 2 parents -> 6 sounds.  FAMILY TREE: sounds and melody loops.  VOODOO / EFFECTOR / DIGGA KILLA: extras for the melodies only.  TIP: drag any sound (a DIGGA shot too) onto these tiles = it goes into PAIR YOUR OWN.");
    }
    std::function<void (int)> onSwitch;
    std::function<bool (int)> isOn;   // VOODOO / EFFECTOR: lit when switched on
    int sel = 0;
    static constexpr int numTiles = 8;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        static const char* names[] { "BREED\nLAB", "FAMILY\nTREE", "PAIR\nYOUR OWN", "PAIR\nFROM VST", "MY\nSOUNDS", "VOODOO\nKILLA", "EFFECTOR\nKILLA", "DIGGA\nKILLA" };
        // each extra wears its plugin's colours
        static const Colour face[] { Colour (0), Colour (0) };
        static const Colour ink[] { Colour (0), Colour (0) };
        for (int k = 0; k < numTiles; ++k)
        {
            auto r = part (k);
            const bool on = k == sel || ((dropHover || flash > 0) && k == 2);
            if (k == 2 && (dropHover || flash > 0))   // the PAIR tile becomes the drop target
            {
                g.setColour (s.accent.withAlpha (0.35f + 0.3f * flash)); g.fillRoundedRectangle (r.expanded (2), 7);
                drawGlowFrame (g, r, s.accent, 6);
                g.setColour (Colours::white); g.setFont (serif (14.0f, true, 0.2f));
                g.drawFittedText (dropHover ? "DROP\n= PAIR" : flashText, r.toNearestInt(), Justification::centred, 2);
                continue;
            }
            if (k == 5)
            {
                const auto lr = r.withY (r.getY() - 15).withHeight (13);
                g.setColour (Colour (0xffc8c4e8)); g.setFont (serif (10.0f, true, 0.3f));
                g.drawText ("FX RACK", lr.toNearestInt(), Justification::centred);
                g.setGradientFill (ColourGradient (Colour (0x00ff3fd2), lr.getX(), 0, Colour (0xccff3fd2), lr.getCentreX() - 30, 0, false));
                g.fillRect (lr.getX() + 4, lr.getCentreY(), lr.getWidth() * 0.5f - 34, 1.2f);
                g.setGradientFill (ColourGradient (Colour (0xcc9b4dff), lr.getCentreX() + 30, 0, Colour (0x009b4dff), lr.getRight(), 0, false));
                g.fillRect (lr.getCentreX() + 30, lr.getCentreY(), lr.getWidth() * 0.5f - 34, 1.2f);
            }
            juce::ignoreUnused (face);
            if (on)
            {
                const bool drop = k == 2 && (dropHover || flash > 0);
                const auto c1 = drop ? s.accent : tileCol (k, 0), c2 = drop ? s.accent : tileCol (k, 1);
                drawGlowFrame (g, r, c1, 8);
                g.setGradientFill (ColourGradient (c1.withAlpha (0.42f), r.getX(), r.getY(), c2.withAlpha (0.16f), r.getRight(), r.getBottom(), false));
                g.fillRoundedRectangle (r, 8);
                g.setGradientFill (ColourGradient (c1, r.getX(), r.getY(), c2, r.getRight(), r.getBottom(), false));
                g.drawRoundedRectangle (r, 8, 1.6f);
            }
            else
            {
                g.setGradientFill (ColourGradient (Colour (0xff1e1b40), 0, r.getY(), Colour (0xff131230), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 8);
                g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (r, 8, 1.2f);
            }
            juce::ignoreUnused (ink);
            // icon + name (left aligned) - each tile its own neon colour
            const float bs = jmin (30.0f, r.getHeight() - 12.0f);
            const auto ic = Rectangle<float> (r.getX() + 7, r.getCentreY() - bs * 0.5f, bs, bs);
            drawTileIcon (g, k, ic, on);
            g.setColour (on ? Colours::white : Colour (0xffe6e3ff));
            g.setFont (serif (13.0f, true, 0.12f));
            g.drawFittedText (String (names[k]), r.withTrimmedLeft (bs + 14).toNearestInt(), Justification::centredLeft, 2);
            if ((k == 5 || k == 6) && isOn && isOn (k))
            {
                g.setColour (Colour (0xff36ff6a)); g.fillEllipse (r.getRight() - 14, r.getY() + 6, 8, 8);
            }
        }
    }
    void mouseUp (const MouseEvent& e) override
    {
        for (int k = 0; k < numTiles; ++k)
            if (part (k).contains (e.position) && onSwitch) { onSwitch (k); return; }
    }
private:
    Rectangle<float> part (int k) const
    {
        const float gap = 5.0f, extra = 18.0f;
        const float h = ((float) getHeight() - gap * (numTiles - 1) - extra) / (float) numTiles;
        return { 3.0f, (float) k * (h + gap) + (k >= 5 ? extra : 0.0f), (float) getWidth() - 6.0f, h };
    }
    void timerCallback() override { flash -= 0.012f; if (flash <= 0) { flash = 0; stopTimer(); } repaint (part (2).expanded (4).toNearestInt()); }
    KKLookAndFeel& lnf;
    bool dropHover = false;
    float flash = 0;
    String flashText;
};

//==============================================================================
class MainPage : public Component, private Timer
{
public:
    explicit MainPage (KeysKillaProcessor& p)
        : proc (p), parentA (p, lnf, 0), parentB (p, lnf, 1), breedBtn (lnf), wildRail (p, lnf), dragDaw (p, lnf),
          pitchWheel (lnf), modWheel (lnf), meter (lnf), keyboard (p, lnf)
    {
        settings = openSettings();
        lnf.setSkin (Skin::all()[1]);   // BLOOD
        setLookAndFeel (&lnf);

        // ---- header
        prevBtn.onClick = [this] { step (-1); }; prevBtn.setTooltip ("Previous preset (inside the category chosen in the browser)");
        nextBtn.onClick = [this] { step (1); };  nextBtn.setTooltip ("Next preset (inside the category chosen in the browser)");
        saveBtn.onClick = [this] { savePreset(); }; saveBtn.setTooltip ("Save this sound as a user preset.");
        menuBtn.onClick = [this] { showMenu(); };  menuBtn.setTooltip ("Presets, A/B, undo, ADVANCED, size.");
        nameBtn.onClick = [this] { openTab (tabBrowser); }; nameBtn.setTooltip ("Click to browse and search all presets.");
        heartBtn.onClick = [this] { toggleFavourite(); }; heartBtn.setTooltip ("Add to favourites");
        worldBtn.framed = true; worldBtn.setButtonText ("WORLD"); worldBtn.onClick = [this] { worldMenu(); };
        worldBtn.setTooltip ("SOUND WORLD: one click colours the whole sound (rompler, analog, glassy, hi-fi, organic ...).  TRANCE GATE and CLIPPER are here too.");
        addAndMakeVisible (worldBtn);
        moonBtn.onClick = [this] { openTab (tabSettings); }; moonBtn.setTooltip ("Settings: eco mode, window size");
        heartBtn.glyph = [this] (Graphics& g, Rectangle<float> hb, const Skin& s)
        {
            if (! isFav) return;
            hb = hb.reduced (8, 10);
            Path heart;
            heart.startNewSubPath (hb.getCentreX(), hb.getBottom());
            heart.cubicTo (hb.getX() - 3, hb.getCentreY(), hb.getX() + 2, hb.getY() - 3, hb.getCentreX(), hb.getY() + hb.getHeight() * 0.3f);
            heart.cubicTo (hb.getRight() - 2, hb.getY() - 3, hb.getRight() + 3, hb.getCentreY(), hb.getCentreX(), hb.getBottom());
            g.setColour (s.accent.withAlpha (0.35f)); g.strokePath (heart, PathStrokeType (5.0f));
            g.setColour (s.accent); g.fillPath (heart);
        };
        nameBtn.glyph = [this] (Graphics& g, Rectangle<float> r, const Skin&)
        {
            g.setColour (Colour (0xffece6e6));
            g.setFont (serif (r.getHeight() * 0.55f, false, 0.06f));
            g.drawFittedText (proc.currentName() + (modified ? " *" : ""), r.reduced (8, 0).toNearestInt(), Justification::centred, 1, 0.6f);
        };
        for (Component* c : { (Component*) &prevBtn, (Component*) &nextBtn, (Component*) &saveBtn, (Component*) &menuBtn,
                              (Component*) &nameBtn, (Component*) &heartBtn, (Component*) &moonBtn })
            addAndMakeVisible (c);

        // ---- parents + BREED
        parentA.onClick = [this] { parentMenu (0); };
        parentB.onClick = [this] { parentMenu (1); };
        addAndMakeVisible (parentA); addAndMakeVisible (parentB);
        for (int sl = 0; sl < 2; ++sl)
        {
            auto& pv = sl == 0 ? prevA : prevB; auto& nx = sl == 0 ? nextA : nextB; auto& dc = sl == 0 ? diceA : diceB;
            pv.onClick = [this, sl] { proc.stepParent (sl, -1); labChanged(); }; pv.setTooltip ("Previous sound of this category");
            nx.onClick = [this, sl] { proc.stepParent (sl, 1); labChanged(); };  nx.setTooltip ("Next sound of this category");
            dc.onClick = [this, sl] { proc.randomParent (sl); labChanged(); };   dc.setTooltip ("Random parent from the whole library");
            addAndMakeVisible (pv); addAndMakeVisible (nx); addAndMakeVisible (dc);
        }
        breedBtn.onBreed = [this] { proc.breed(); labChanged(); };
        addAndMakeVisible (breedBtn);

        // ---- children, genes, mutate
        for (int i = 0; i < 6; ++i)
        {
            auto c = std::make_unique<ChildCard> (proc, lnf, i);
            c->onMenu = [this] (int idx) { childMenu (idx); };
            c->setTooltip ("CHILD " + String (i + 1) + ": click to load, play button to hear it, stars to rate (4+ stars are kept in User > Bred). Right-click: use as parent or put it into the FAMILY TREE.");
            addAndMakeVisible (*c);
            childCards.push_back (std::move (c));
        }
        for (int gI = 0; gI < KeysKillaProcessor::numGenes; ++gI)
        {
            auto gs = std::make_unique<GeneSwitch> (proc, lnf, gI);
            addAndMakeVisible (*gs);
            genes.push_back (std::move (gs));
        }
        static const float amt[] { 0.05f, 0.15f, 0.3f, 0.6f, 1.0f };
        static const char* mtip[] { "MUTATE 5 %: a near-identical variation", "MUTATE 15 %: a recognisable sibling", "MUTATE 30 %: a new sound from the same family",
                                    "MUTATE 60 %: a strong reinterpretation", "CHAOS: extreme, but kept safe" };
        for (int i = 0; i < 5; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf);
            b->setTooltip (String (mtip[i]) + ". Right-click: history, locks.");
            b->onClick = [this, i] { mutate (amt[i]); lastMutate = i; for (auto& m : mutateBtns) m->selected = false; mutateBtns[(size_t) i]->selected = true; repaint(); };
            b->onRightClick = [this] { showDiceMenu(); };
            addAndMakeVisible (*b);
            mutateBtns.push_back (std::move (b));
        }
        soundModeBtn.setButtonText ("SOUND"); loopModeBtn.setButtonText ("LOOP");
        soundModeBtn.framed = loopModeBtn.framed = true; soundModeBtn.selected = true;
        soundModeBtn.setTooltip ("Children play as SOUNDS (one note)");
        loopModeBtn.setTooltip ("Children play as LOOPS: every child gets its own melody loop (host tempo) - drag it into FL as MIDI from the LOOP panel");
        soundModeBtn.onClick = [this] { proc.mainLoopMode = false; if (proc.loopOwnerId() == 3) proc.stopLoop(); proc.touchLab(); };
        loopModeBtn.onClick = [this] { proc.mainLoopMode = true; proc.touchLab(); const int c = std::max (0, proc.selectedChild()); if (! proc.kids().empty()) proc.playChild (c); };
        addAndMakeVisible (soundModeBtn); addAndMakeVisible (loopModeBtn);
        treeBtn.onClick = [this] { openTab (tabTree); }; treeBtn.setTooltip ("FAMILY TREE: breed up to 4 sounds into new sounds or melody loops");
        labSwitch.onSwitch = [this] (int k)
        {
            if (k == 0) { hidePanels(); openTabIndex = -1; updateTabs(); return; }
            static constexpr int target[] { 0, tabTree, tabPair, tabVst, tabSounds, tabHalf, tabEffector, tabDigga };
            if (! (openTabIndex == target[k] && isPanelVisible())) openTab (target[k]);
        };
        labSwitch.isOn = [this] (int k) { return proc.apvts.getRawParameterValue (k == 5 ? ID::halfOn : ID::efxOn)->load() > 0.5f; };
        labSwitch.onDropToPair = [this] (const File& f)
        {
            int slot = 0;
            while (slot < (int) proc.pairParents.size() && proc.pairParents[(size_t) slot] != nullptr) ++slot;
            if (slot >= (int) proc.pairParents.size()) slot = (dropRound++) % (int) proc.pairParents.size();   // all full: replace in turn
            return proc.loadPairParent (slot, f) ? slot + 1 : 0;
        };
        addAndMakeVisible (labSwitch);
        undoBtn.onClick = [this] { proc.undo(); refreshState(); }; undoBtn.setTooltip ("UNDO the last change of the sound");
        addAndMakeVisible (treeBtn); addAndMakeVisible (undoBtn);

        // ---- WILD rail + FUTURE / ALIVE / TIME
        addChildComponent (wildRail);   // v0.14: the WILD rail is gone from the main page (BREED keeps its last setting)
        addAndMakeVisible (dragDaw);
        const char* sideIds[] { ID::future, ID::alive, ID::timeM };
        const char* sideTips[] { "FUTURE: ORIGINAL -> HYBRID -> UNKNOWN. Turns the sound into a new hybrid (same seed = same result).",
                                 "ALIVE 0-5: every note a little different, like a real player.",
                                 "TIME: TIGHT <- NATURAL -> DREAM -> FROZEN (release, reverb, delay, freeze)." };
        for (int i = 0; i < 3; ++i)
        {
            auto k = std::make_unique<ImageKnob> (lnf);
            auto* prm = proc.apvts.getParameter (sideIds[i]);
            k->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
            k->setTooltip (sideTips[i]);
            attachments.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, sideIds[i], *k));
            addAndMakeVisible (*k);
            sideKnobs.push_back (std::move (k));
        }

        // ---- module row: every tab opens one KILLA plugin inside KEYS KILLA ("10 in 1")
        static const char* tabNames[] { "808", "SNARE / CLAP", "HI-HAT", "KICK", "OPEN HAT", "PERC", "FX" };
        static const char* tabTips[] { "808: drop your 808, boost it (punch, sub, drive, clipper, pitch), drag it back into FL",
                                       "SNARE / CLAP: drop a snare or clap, make it crack, drag it back into FL",
                                       "HI-HAT: drop a hat, make it shine - plus the roll generator",
                                       "KICK: drop a kick, make it hit - SAVE TO KIT",
                                       "OPEN HAT: open hats and crashes for your drum kit",
                                       "PERC: rims, toms, shakers, bongos for your drum kit",
                                       "FX: risers, impacts, vox tags for your drum kit" };
        for (int i = 0; i < numTabs; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, tabNames[i]);
            b->setTooltip (tabTips[i]);
            b->onClick = [this, i] { openTab (i); };
            b->framed = true; b->tint = drumTheme (i).accent;
            addAndMakeVisible (*b);
            tabs.push_back (std::move (b));
        }

        // ---- macros: DARK SPACE MOVEMENT WIDTH TEXTURE PUNCH DIRT MIX
        const char* macroIds[] { ID::m1, ID::m2, ID::m5, ID::m6, ID::m4, ID::m7, ID::m3, ID::m8 };
        for (int i = 0; i < 8; ++i)
        {
            auto k = std::make_unique<ImageKnob> (lnf);
            auto* prm = proc.apvts.getParameter (macroIds[i]);
            k->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
            attachments.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, macroIds[i], *k));
            addAndMakeVisible (*k);
            macros.push_back (std::move (k));
            auto cap = std::make_unique<MacroCaption> (lnf);
            addAndMakeVisible (*cap);
            captions.push_back (std::move (cap));
        }

        // ---- meter, wheels, keyboard
        addAndMakeVisible (meter);
        pitchWheel.setRange (-1.0, 1.0); pitchWheel.setValue (0.0);
        pitchWheel.onValueChange = [this] { proc.guiPitch = (float) pitchWheel.getValue(); };
        pitchWheel.onDragEnd = [this] { pitchWheel.setValue (0.0); };
        pitchWheel.setTooltip ("Pitch wheel");
        modWheel.setRange (0.0, 1.0); modWheel.setValue (0.0);
        modWheel.onValueChange = [this] { proc.guiMod = (float) modWheel.getValue(); };
        modWheel.setTooltip ("Mod wheel: adds vibrato");
        addAndMakeVisible (pitchWheel); addAndMakeVisible (modWheel);
        addAndMakeVisible (keyboard);

        setSize (KeysKillaEditor::designW, KeysKillaEditor::designH);
        noFocus (*this);
        applyKeyMode();
        focusGrabber.page = this;
        addMouseListener (&focusGrabber, true);   // a click anywhere in the plugin gives it the PC keyboard
        refreshState();
        startTimerHz (30);
        // warm up the KILLA plugins in the background (one after another) - a click on their tile opens them at once
        Component::SafePointer<MainPage> safe (this);
        Timer::callAfterDelay (1200, [safe] { if (safe != nullptr) safe->module (tabDigga); });
        Timer::callAfterDelay (1700, [safe] { if (safe != nullptr) safe->module (tabHalf); });
        Timer::callAfterDelay (2200, [safe] { if (safe != nullptr) safe->module (tabEffector); });
    }

    ~MainPage() override
    {
        stopTimer();
        PopupMenu::dismissAllActiveMenus();   // never leave a menu pointing at a closed editor
        proc.releaseThumbnailRenderer();      // nothing heavy stays alive after the window is closed
        proc.stopLoop();                      // the audition loop never keeps playing in a closed plugin
        removeMouseListener (&focusGrabber);
        setLookAndFeel (nullptr);
    }

    int preferredScale() const { return jlimit (50, 100, settings->getIntValue ("labScale18", 85)); }

    // ---------------- SPACE = play / stop in the plugin (while the plugin window has focus), other keys go to the host ----------------
    bool keysToPlugin() const { return settings->getBoolValue ("keysToPlugin", true); }
    void applyKeyMode()
    {
        setWantsKeyboardFocus (keysToPlugin());
        setMouseClickGrabsKeyboardFocus (false);
        if (! keysToPlugin() && hasKeyboardFocus (false)) giveAwayKeyboardFocus();
    }
    bool keyPressed (const KeyPress& k) override
    {
        if (! keysToPlugin() || typingText()) return false;
        const auto mods = k.getModifiers();
        if (mods.isCommandDown() || mods.isCtrlDown() || mods.isAltDown()) return false;   // host shortcuts (save, undo...) stay with the host
        if (k.getKeyCode() == KeyPress::spaceKey) { spaceAction(); return true; }
        // the computer keys play notes while KEYS KILLA has the focus (like FL's typing keyboard)
        if (const int n = qwertyNote (k.getKeyCode()); n >= 0)
        {
            if (! qwertyHeld[(size_t) n]) { qwertyHeld[(size_t) n] = true; heldCode[(size_t) n] = k.getKeyCode(); proc.keyboardState.noteOn (1, n, 0.8f); }
            return true;
        }
        return false;
    }
    bool keyStateChanged (bool isKeyDown) override
    {
        bool any = false;
        for (int n = 0; n < 128; ++n)
            if (qwertyHeld[(size_t) n] && ! KeyPress::isKeyCurrentlyDown (heldCode[(size_t) n]))
            { qwertyHeld[(size_t) n] = false; proc.keyboardState.noteOff (1, n, 0.0f); any = true; }
        juce::ignoreUnused (isKeyDown);
        return any;
    }
    // FL Studio layout: Z S X D C V G B H N J M = C3..B3, Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P = C4..E5
    int qwertyNote (int code) const
    {
        static const char* low = "ZSXDCVGBHNJM";
        static const char* high = "Q2W3ER5T6Y7UI9O0P";
        const int c = CharacterFunctions::toUpperCase ((juce_wchar) code);
        for (int i = 0; low[i] != 0; ++i) if (c == low[i]) return 48 + i;
        for (int i = 0; high[i] != 0; ++i) if (c == high[i]) return 60 + i;
        return -1;
    }
    std::array<bool, 128> qwertyHeld {};
    std::array<int, 128> heldCode {};
    static bool typingText() { return dynamic_cast<TextEditor*> (Component::getCurrentlyFocusedComponent()) != nullptr; }

    // tests / screenshots: 0 main, 1..8 advanced tab, 9 browser, 10 movement, 11 808
    void showView (int v)
    {
        if (v >= 1 && v <= 8) { ensureAdvanced(); advanced->showTab (v - 1); advanced->setVisible (true); advanced->toFront (false); }
        if (v == 9) openTab (tabBrowser);
        if (v == 10) openTab (tabParams);
        if (v == 11) openTab (tabHalf);
        if (v == 15) openTab (tab808);
        if (v == 16) openTab (tabHat);
        if (v == 17) openTab (tabEffector);
        if (v == 18) openTab (tabDigga);
        if (v == 19) openTab (tabSnare);
        if (v == 20) openTab (tabHalf);
        if (v == 22) openTab (tabVst);
        if (v == 24) openTab (tabSounds);
        if (v == 25) openTab (tabKick);
        if (v == 23) { openTab (tabDigga); if (auto* pg = dynamic_cast<EmbeddedPage*> (module (tabDigga))) pg->openChop(); }
        if (v == 21)
        {
            if (auto f = File::getSpecialLocation (File::tempDirectory).getChildFile ("kk_harvest_demo.wav"); f.existsAsFile())
            { proc.harvestFile (f); while (proc.harvesting()) Thread::sleep (20); proc.moduleHousekeeping(); }
            proc.pairDice (0); proc.pairDice (1); proc.pairDice (2); proc.pairBreed(); proc.selectPairKid (1, false); openTab (tabPair);
        }
        if (v == 12) { proc.breed(); while (proc.renderNextThumbnail()) {} proc.selectChild (2); labChanged(); }
        if (v == 13 || v == 14)   // FAMILY TREE with 4 sounds: 13 = SOUND results, 14 = LOOP results
        {
            proc.setAncestorPreset (0, 0); proc.setAncestorPreset (1, 60); proc.setAncestorPreset (2, 200); proc.setAncestorPreset (3, 330);
            proc.setTreeMode (v == 14 ? KeysKillaProcessor::treeLoop : KeysKillaProcessor::treeSound);
            proc.treeBreed(); while (proc.renderNextThumbnail()) {}
            proc.rateTreeResult (2, 4);
            openTab (tabTree);
        }
    }

    void paint (Graphics& g) override
    {
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImageAt (labImages().bg, 0, 0);
        // module bar (v0.17: six modules, drawn over the bitmap's old tab row)
        const auto bar = R (46, 613, 1632, 661).toFloat();
        g.setColour (Colour (0xff12112a)); g.fillRoundedRectangle (bar, 6);
        g.setColour (Colour (0xff3a3264)); g.drawRoundedRectangle (bar, 6, 1.2f);
    }

    void resized() override
    {
        prevBtn.setBounds (R (642, 44, 684, 86)); nameBtn.setBounds (R (688, 46, 1044, 84)); nextBtn.setBounds (R (1044, 44, 1082, 86));
        heartBtn.setBounds (R (1086, 44, 1128, 86)); saveBtn.setBounds (R (1245, 42, 1346, 85)); menuBtn.setBounds (R (1365, 42, 1466, 85));
        moonBtn.setBounds (R (1483, 40, 1544, 87));
        worldBtn.setBounds (R (1132, 46, 1240, 84));

        parentA.setBounds (R (268, 150, 545, 372)); parentB.setBounds (R (1128, 150, 1372, 372));
        prevA.setBounds (R (228, 228, 266, 274)); nextA.setBounds (R (545, 228, 583, 274)); diceA.setBounds (R (580, 144, 623, 186));
        prevB.setBounds (R (1088, 228, 1126, 274)); nextB.setBounds (R (1380, 228, 1418, 274)); diceB.setBounds (R (1373, 144, 1416, 188));
        breedBtn.setBounds (R (737, 152, 937, 352));

        const int cx[6] { 220, 422, 623, 827, 1030, 1233 };
        for (int i = 0; i < 6; ++i) childCards[(size_t) i]->setBounds (cx[i], 385, 190, 128);
        const int gx[6] { 311, 441, 572, 697, 830, 952 };
        for (int i = 0; i < 6; ++i) genes[(size_t) i]->setBounds (gx[i] - 41, 556, 116, 33);
        const int mx[5][2] { { 1049, 1094 }, { 1095, 1142 }, { 1142, 1189 }, { 1189, 1236 }, { 1236, 1288 } };
        for (int i = 0; i < 5; ++i) mutateBtns[(size_t) i]->setBounds (R (mx[i][0], 555, mx[i][1], 595));
        soundModeBtn.setBounds (R (151, 404, 216, 438)); loopModeBtn.setBounds (R (151, 446, 216, 480));
        treeBtn.setBounds (R (1300, 528, 1374, 604)); undoBtn.setBounds (R (1379, 528, 1442, 604));

        wildRail.setBounds (R (1466, 118, 1608, 392));
        dragDaw.setBounds (R (1462, 110, 1612, 398));
        sideKnobs[0]->place ({ 1532, 447 }, 33, 43);
        sideKnobs[1]->place ({ 1490, 552 }, 20, 28);
        sideKnobs[2]->place ({ 1575, 552 }, 20, 28);

        const int tw = (1622 - 55 - (numTabs - 1) * 8) / numTabs;
        for (int i = 0; i < numTabs; ++i) tabs[(size_t) i]->setBounds (R (55 + i * (tw + 8), 619, 55 + i * (tw + 8) + tw, 655));

        const int kx[8] { 132, 284, 435, 587, 742, 895, 1047, 1202 };
        for (int i = 0; i < 8; ++i)
        {
            macros[(size_t) i]->place ({ kx[i], 715 }, 29, 41);
            captions[(size_t) i]->designPos = { kx[i], 768 };
            captions[(size_t) i]->setBounds (kx[i] - 62, 757, 124, 22);
        }
        meter.setBounds (R (1337, 716, 1595, 761));
        pitchWheel.setBounds (R (70, 808, 110, 894)); modWheel.setBounds (R (124, 808, 164, 894));
        keyboard.setBounds (R (200, 815, 1640, 923));
        keyboard.setKeyWidth (1440.0f / 40.0f);

        const auto panelArea = R (10, 8, 1662, 612);
        if (advanced) advanced->setBounds (R (150, 8, 1662, 612));   // the left switch stays visible
        if (browser) browser->setBounds (R (150, 8, 1662, 612));   // the left tiles never cover the preset names
        if (treePanel) treePanel->setBounds (R (150, 96, 1662, 612));
        for (int i = 0; i < numPages; ++i)
            if (modules[(size_t) i]) modules[(size_t) i]->setBounds (i == tabPair || i == tabVst || i == tabSounds ? R (150, 96, 1662, 612) : R (0, 0, 1672, 941));   // the KILLA plugins and the drum pages get the whole window
        labSwitch.setBounds (R (16, 100, 138, 600));
    }

private:
    enum { tab808, tabSnare, tabHat, tabKick, tabOpenHat, tabPerc, tabDrumFx, numTabs, tabHalf, tabEffector, tabDigga, tabPair, tabVst, tabSounds, numPages, tabBrowser = 99, tabSettings = 100, tabTree = 101, tabParams = 102 };
    Component* module (int t)
    {
        auto& m = modules[(size_t) t];
        if (! m)
        {
            switch (t)
            {
                case tab808: case tabSnare: case tabHat: case tabKick: case tabOpenHat: case tabPerc: case tabDrumFx:
                {
                    auto pg = std::make_unique<DrumPage> (proc, lnf, t - tab808);   // tab order = drum slot order
                    pg->onSwitch = [this] (int dd) { MessageManager::callAsync ([safe = Component::SafePointer<Component> (this), this, dd] { if (safe != nullptr) openTab (tab808 + dd); }); };
                    m = std::move (pg); break;
                }
                case tabPair:     m = std::make_unique<PairPage> (proc, lnf); break;
                case tabVst:      m = std::make_unique<PairPage> (proc, lnf, true); break;
                case tabSounds:
                {
                    auto pg = std::make_unique<MySoundsPage> (proc, lnf);
                    pg->onFactory = [this] { openTab (tabBrowser); };
                    pg->onPair = [this] { MessageManager::callAsync ([safe = Component::SafePointer<MainPage> (this)] { if (safe != nullptr) safe->openTab (tabPair); }); };
                    m = std::move (pg); break;
                }
                case tabHalf:     m = std::make_unique<EmbeddedPage> (proc, lnf, KeysKillaProcessor::modHalf, "HALF", "VOODOO KILLA  -  ON THE MELODIES ONLY (KEYS KILLA + DIGGA)", ID::halfOn); break;
                case tabEffector: m = std::make_unique<EmbeddedPage> (proc, lnf, KeysKillaProcessor::modEffector, "EFFECTOR", "EFFECTOR KILLA  -  ON THE MELODIES ONLY (KEYS KILLA + DIGGA)", ID::efxOn); break;
                default:
                {
                    auto pg = std::make_unique<EmbeddedPage> (proc, lnf, KeysKillaProcessor::modDigga, "DIGGA", "DIGGA KILLA  -  SAMPLE, CHOP, FLIP", nullptr);
                    pg->onPair = [this] { openTab (tabPair); };
                    m = std::move (pg); break;
                }
            }
            addChildComponent (*m); noFocus (*m); resized();
        }
        return m.get();
    }
    std::vector<Component*> panels() const
    {
        std::vector<Component*> v { advanced.get(), browser.get(), treePanel.get() };
        for (auto& m : modules) v.push_back (m.get());
        return v;
    }

    // ---------------- tab panels (created on first use -> fast editor open/close) ----------------
    void ensureAdvanced()
    {
        if (advanced) return;
        advanced = std::make_unique<AdvancedPage> (proc, lnf, 1);
        advanced->onSize = [this] (int pct) { setScale (pct); };
        addChildComponent (*advanced);
        noFocus (*advanced);
        resized();
    }
    void ensureBrowser()
    {
        if (browser) return;
        browser = std::make_unique<PresetBrowser> (proc, lnf);
        browser->getFavourites = [this] { return favourites(); };
        browser->toggleFavourite = [this] (const String& n) { toggleFavouriteNamed (n); };
        browser->onChanged = [this] { refreshState(); };
        browser->onBred = [this] { labChanged(); };
        browser->focusKeys = [] { Component::unfocusAllComponents(); };   // no text box holds the keys: FL's typing keyboard and your MIDI keys play the sound
        addChildComponent (*browser);
        noFocus (*browser);
        resized();
    }
    void hidePanels()
    {
        for (auto* c : panels()) if (c != nullptr) c->setVisible (false);
    }
    void openTab (int t)
    {
        const bool wasOpen = t == openTabIndex && isPanelVisible();
        hidePanels();
        openTabIndex = -1;
        if (! wasOpen)
        {
            openTabIndex = t;
            switch (t)
            {
                case tabBrowser: ensureBrowser(); browser->open (proc.uiCat, proc.uiEra, false); break;
                case tabParams: ensureAdvanced(); advanced->showTab (0); advanced->setVisible (true); break;
                case tabSettings: ensureAdvanced(); advanced->showTab (7); advanced->setVisible (true); break;
                case tabTree:
                    if (! treePanel)
                    {
                        treePanel = std::make_unique<FamilyTreePanel> (proc, lnf);
                        treePanel->onLab = [this] { labChanged(); };
                        treePanel->onChoose = [this] (int slot) { soundMenu (slot, true); };
                        treePanel->onResultMenu = [this] (int k) { resultMenu (k); };
                        addChildComponent (*treePanel); noFocus (*treePanel); resized();
                    }
                    treePanel->refresh(); treePanel->setVisible (true); break;
                default:
                    if (t >= 0 && t < numPages) module (t)->setVisible (true);
                    break;
            }
            for (auto* c : panels()) if (c != nullptr && c->isVisible()) c->toFront (false);
            labSwitch.toFront (false);
        }
        updateTabs();
    }
    bool isPanelVisible() const
    {
        for (auto* c : panels()) if (c != nullptr && c->isVisible()) return true;
        return false;
    }
    void updateTabs()
    {
        if (! isPanelVisible()) openTabIndex = -1;
        const bool treeOn = openTabIndex == tabTree;
        const int sw = openTabIndex < 0 ? 0 : treeOn ? 1 : openTabIndex == tabPair ? 2 : openTabIndex == tabVst ? 3 : openTabIndex == tabSounds ? 4 : openTabIndex == tabHalf ? 5 : openTabIndex == tabEffector ? 6 : openTabIndex == tabDigga ? 7 : -1;
        if (labSwitch.sel != sw) { labSwitch.sel = sw; labSwitch.repaint(); }
        if (proc.loopPlaying())   // a loop belongs to the page that started it (FAMILY TREE / PAIR / BREED LAB)
        {
            const int owner = proc.loopOwnerId();
            const bool keep = (owner == 1 && treeOn) || (owner == 2 && (openTabIndex == tabPair || openTabIndex == tabVst))
                           || (owner == 3 && openTabIndex < 0) || (owner != 1 && owner != 2 && owner != 3 && openTabIndex < 0);
            if (! keep) proc.stopLoop();
        }
        for (int i = 0; i < numTabs; ++i) { tabs[(size_t) i]->selected = i == openTabIndex; tabs[(size_t) i]->repaint(); }
    }
    void setScale (int pct)
    {
        settings->setValue ("labScale18", pct);
        if (auto* ed = findParentComponentOfClass<AudioProcessorEditor>())
            ed->setSize (KeysKillaEditor::designW * pct / 100, KeysKillaEditor::designH * pct / 100);
    }

    // ---------------- BREED LAB ----------------
    void labChanged() { lastLab = -1; refreshState(); }
    void parentMenu (int slot) { soundMenu (slot, false); }
    void soundMenu (int slot, bool ancestor)
    {
        PopupMenu m, cats;
        m.addSectionHeader (ancestor ? "SOUND " + String (slot + 1) : String (slot == 0 ? "PARENT A" : "PARENT B"));
        m.addItem (1, "Choose from the browser...");
        m.addItem (2, "Use the current sound");
        m.addItem (3, "Random sound");
        if (ancestor)
        {
            m.addItem (4, "Use PARENT A", proc.parent (0).valid());
            m.addItem (5, "Use PARENT B", proc.parent (1).valid());
            m.addItem (6, "Empty");
        }
        const auto& ps = factoryPresets();
        for (int c = 0; c < numCategories; ++c)
        {
            if (c == c808) continue;
            PopupMenu sub;
            for (int i = 0; i < (int) ps.size(); ++i) if (ps[(size_t) i].cat == c) sub.addItem (1000 + i, ps[(size_t) i].name);
            cats.addSubMenu (categoryNames()[c], sub);
        }
        m.addSubMenu ("By category", cats);
        m.showMenuAsync (PopupMenu::Options(), [this, slot, ancestor, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            auto setPreset = [this, slot, ancestor] (int idx)
            {
                if (ancestor) { if (idx >= 0) proc.setAncestorPreset (slot, idx); else proc.setAncestorCurrent (slot); }
                else { if (idx >= 0) proc.setParentPreset (slot, idx); else proc.setParentCurrent (slot); }
            };
            if (r == 1)
            {
                ensureBrowser(); hidePanels();
                browser->open (-1, -1, false, [this, setPreset, ancestor] (int idx) { setPreset (idx); labChanged(); if (ancestor) openTab (tabTree); },
                               ancestor ? "CHOOSE SOUND " + String (slot + 1) : String (slot == 0 ? "CHOOSE PARENT A" : "CHOOSE PARENT B"));
                openTabIndex = tabBrowser; updateTabs();
                return;
            }
            if (r == 2) setPreset (-1);
            else if (r == 3) { if (ancestor) proc.randomAncestor (slot); else proc.randomParent (slot); }
            else if (r == 4 || r == 5) proc.setAncestorGenome (slot, proc.parent (r - 4));
            else if (r == 6) proc.clearAncestor (slot);
            else if (r >= 1000) setPreset (r - 1000);
            labChanged();
            if (treePanel) treePanel->refresh();
        });
    }
    void resultMenu (int k)
    {
        if (k >= (int) proc.treeKids().size()) return;
        const bool loopMode = proc.getTreeMode() == KeysKillaProcessor::treeLoop;
        PopupMenu m, toTree;
        m.addSectionHeader (proc.treeKids()[(size_t) k].g.name);
        m.addItem (1, loopMode ? "Play / stop the loop" : "Play");
        if (loopMode) m.addItem (2, "New melody (same sound)");
        m.addItem (3, "Use as PARENT A  (BREED LAB)");
        m.addItem (4, "Use as PARENT B  (BREED LAB)");
        for (int a = 0; a < KeysKillaProcessor::numAncestors; ++a) toTree.addItem (10 + a, "SOUND " + String (a + 1));
        m.addSubMenu ("Breed on: put it into", toTree);
        m.addItem (5, "Load and save as preset...");
        m.showMenuAsync (PopupMenu::Options(), [this, k, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0 || k >= (int) proc.treeKids().size()) return;
            const auto g = proc.treeKids()[(size_t) k].g;
            if (r == 1) proc.playTreeResult (k);
            if (r == 2) { proc.newMelody (k); if (! proc.loopIsTree (k)) proc.playTreeResult (k); }
            if (r == 3 || r == 4) proc.setParentGenome (r - 3, g);
            if (r >= 10) proc.setAncestorGenome (r - 10, g);
            if (r == 5) { proc.selectTreeResult (k); savePresetAs(); }
            labChanged();
            if (treePanel) treePanel->refresh();
        });
    }
    void childMenu (int idx)
    {
        PopupMenu m;
        m.addSectionHeader (proc.kids()[(size_t) idx].g.name);
        m.addItem (1, "Use as PARENT A  (next generation)");
        m.addItem (2, "Use as PARENT B  (next generation)");
        m.addItem (3, "Load and save as preset...");
        m.addItem (4, "Play");
        PopupMenu toTree;
        for (int a = 0; a < KeysKillaProcessor::numAncestors; ++a) toTree.addItem (10 + a, "SOUND " + String (a + 1));
        m.addSubMenu ("Put into the FAMILY TREE", toTree);
        m.showMenuAsync (PopupMenu::Options(), [this, idx, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1 || r == 2) proc.setParentChild (r - 1, idx);
            if (r == 3) { proc.selectChild (idx); savePresetAs(); }
            if (r == 4) proc.previewChild (idx);
            if (r >= 10 && r < 10 + KeysKillaProcessor::numAncestors) { proc.setAncestorGenome (r - 10, proc.kids()[(size_t) idx].g); openTab (tabTree); }
            labChanged();
        });
    }
    void mutate (float amount)
    {
        if (auto* c = proc.apvts.getParameter (ID::chaos)) c->setValueNotifyingHost (c->convertTo0to1 (amount));
        proc.rollDice (catOfCurrent());
        proc.captureUndo();
        refreshState();
    }

    // ---------------- browsing ----------------
    int catOfCurrent() const
    {
        const int i = proc.currentPresetIndex();
        return i >= 0 ? factoryPresets()[(size_t) i].cat : (int) cLead;
    }
    void step (int dir)
    {
        // stay inside the category / subcategory picked in the browser
        const auto& ps = factoryPresets();
        std::vector<int> list;
        for (int i = 0; i < (int) ps.size(); ++i)
        {
            if (proc.uiCat >= 0 && ps[(size_t) i].cat != proc.uiCat) continue;
            if (proc.uiCat >= 0 && proc.uiSub >= 0 && ps[(size_t) i].sub != subcategoryNames (proc.uiCat)[proc.uiSub]) continue;
            list.push_back (i);
        }
        if (list.empty()) { list.resize (ps.size()); std::iota (list.begin(), list.end(), 0); }
        auto it = std::find (list.begin(), list.end(), proc.currentPresetIndex());
        int pos = it == list.end() ? (dir > 0 ? -1 : 0) : (int) std::distance (list.begin(), it);
        pos = (pos + dir + (int) list.size()) % (int) list.size();
        proc.loadPreset (list[(size_t) pos]);
        proc.captureUndo();
        refreshState();
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
        w->enterModalState (true, ModalCallbackFunction::create ([w, done, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr) return;
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
    void savePreset()
    {
        if (proc.currentUserFile().existsAsFile()) { proc.saveUserPreset (proc.currentUserFile()); refreshState(); }
        else savePresetAs();
    }

    void showMenu()
    {
        const bool user = proc.currentUserFile().existsAsFile();
        PopupMenu m, size, packs;
        m.addSectionHeader ("PRESET");
        m.addItem (1, "Save");
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
        m.addItem (8, "Undo", proc.canUndo());
        m.addItem (9, "Redo", proc.canRedo());
        m.addItem (12, String ("Switch to ") + (proc.currentAB() == 0 ? "B" : "A") + "  (now " + (proc.currentAB() == 0 ? "A" : "B") + ")");
        m.addItem (13, String ("Copy ") + (proc.currentAB() == 0 ? "A > B" : "B > A"));
        m.addSeparator();
        m.addItem (15, "ADVANCED page...");
        m.addItem (16, "Eco mode (lower CPU)", true, proc.eco.load());
        m.addItem (18, "PANIC (all notes off)");
        m.addItem (19, "SPACE plays / stops KEYS KILLA (not the host)", true, keysToPlugin());
        for (int pct : { 50, 60, 70, 85, 100 }) size.addItem (100 + pct, String (pct) + " %", true, preferredScale() == pct);
        m.addSubMenu ("Window size", size);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (menuBtn), [this, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;   // editor closed (host shutting down) or menu dismissed
            switch (r)
            {
                case 1: savePreset(); break;
                case 2: savePresetAs(); break;
                case 3: askName ("Rename preset", proc.currentName(), [this] (const String& n) { proc.renameUserPreset (n); refreshState(); }); break;
                case 4:
                    AlertWindow::showOkCancelBox (MessageBoxIconType::WarningIcon, "Delete preset", "Delete \"" + proc.currentName() + "\"?", "Delete", "Cancel", this,
                                                  ModalCallbackFunction::create ([this, safe = SafePointer<MainPage> (this)] (int ok) { if (safe == nullptr) return; if (ok) proc.deleteUserPreset(); refreshState(); }));
                    break;
                case 5: proc.revert(); break;
                case 6: proc.initPatch(); break;
                case 7: openTab (tabBrowser); break;
                case 8: proc.undo(); break;
                case 9: proc.redo(); break;
                case 12: proc.switchAB(); break;
                case 13: proc.copyAtoB(); break;
                case 15: hidePanels(); ensureAdvanced(); advanced->setVisible (true); advanced->toFront (false); break;
                case 16: proc.eco = ! proc.eco.load(); break;
                case 18: proc.panic(); break;
                case 19: settings->setValue ("keysToPlugin", ! keysToPlugin()); applyKeyMode(); break;
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
                default: if (r > 100) setScale (r - 100); break;
            }
            refreshState();
        });
    }

    void showDiceMenu()
    {
        PopupMenu m, hist, locks;
        m.addItem (1, "Undo last mutation", ! proc.diceHistoryNames().isEmpty());
        const auto names = proc.diceHistoryNames();
        for (int i = names.size(); --i >= 0;) hist.addItem (100 + i, "Back to: " + names[i]);
        m.addSubMenu ("History (last 20)", hist, ! names.isEmpty());
        for (int i = 0; i < KeysKillaProcessor::numLocks; ++i) locks.addItem (200 + i, String ("Lock ") + KeysKillaProcessor::lockName (i), true, proc.diceLocks[(size_t) i]);
        m.addSubMenu ("MUTATE locks", locks);
        m.addItem (2, "Save this sound as preset...");
        m.showMenuAsync (PopupMenu::Options(), [this, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1) proc.undoDice();
            else if (r == 2) savePresetAs();
            else if (r >= 200) proc.diceLocks[(size_t) (r - 200)] = ! proc.diceLocks[(size_t) (r - 200)];
            else if (r >= 100) proc.restoreDice (r - 100);
            refreshState();
        });
    }

    void refreshState()
    {
        isFav = favourites().contains (proc.currentName());
        heartBtn.repaint(); nameBtn.repaint();
        const bool bass = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f;
        static const char* normal[] { "DARK", "SPACE", "MOVEMENT", "WIDTH", "TEXTURE", "PUNCH", "DIRT", "MIX" };
        static const char* bassL[] { "SUB", "WOBBLE", "TONE", "CLICK", "GLIDE", "KNOCK", "DIRT", "MIX" };
        static const char* tipsN[] { "DARK: turn right for a darker, warmer tone", "SPACE: reverb and delay", "MOVEMENT: filter motion, chorus, vibrato",
                                     "WIDTH: stereo width and detune", "TEXTURE: tape wobble, vinyl, bitcrush", "PUNCH: harder attack, transient boost",
                                     "DIRT: saturation and distortion", "MIX: overall effect balance (middle = as designed)" };
        static const char* tipsB[] { "SUB: clean sine sub an octave down", "WOBBLE: tempo-synced wobble", "TONE: darker / brighter",
                                     "CLICK: punchy pitch click on the attack", "GLIDE: slide time between notes", "KNOCK: transient boost",
                                     "DIRT: distortion above the clean low end", "MIX: effect balance" };
        const auto custom = proc.macroNames();
        for (int i = 0; i < 8; ++i)
        {
            const String t = custom.size() == 8 ? custom[i] : String (bass ? bassL[i] : normal[i]);
            captions[(size_t) i]->text = t;
            captions[(size_t) i]->custom = t != normal[i];
            captions[(size_t) i]->repaint();
            macros[(size_t) i]->setTooltip (bass ? tipsB[i] : tipsN[i]);
        }
        if (proc.labVersion() != lastLab)
        {
            lastLab = proc.labVersion();
            parentA.repaint(); parentB.repaint();
            for (auto& c : childCards) c->repaint();
            for (auto& gsw : genes) gsw->repaint();
            soundModeBtn.selected = ! proc.mainLoopMode; loopModeBtn.selected = proc.mainLoopMode; soundModeBtn.repaint(); loopModeBtn.repaint();
        }
        if (proc.mainLoopMode && proc.loopPlaying() && proc.loopOwnerId() == 3) for (auto& c : childCards) c->repaint();   // loop playhead
        wildRail.repaint();
        updateTabs();
    }

    void worldMenu()
    {
        auto val = [this] (const char* id) { return (int) proc.apvts.getRawParameterValue (id)->load(); };
        PopupMenu m;
        m.addSectionHeader ("SOUND WORLD  (the whole sound, one click)");
        static const char* tips[] { "Off", "ROMPLER 90  -  dry, hard mids: pizzicato / bells / rap piano", "FAT ANALOG  -  driven ladder warmth, drift",
                                    "GLASS SQUASH  -  multiband squash: glassy, aggressive", "HI-FI SHINE  -  modern shine + chorus",
                                    "ORGANIC  -  breath / foley layer, wide shimmer", "VELOCITY DEEP  -  felt when soft, open and saturated when hard",
                                    "DRIFT ANALOG  -  drive + per-side pitch drift", "MIX READY  -  glued, bright, finished" };
        for (int w = 0; w < 9; ++w) m.addItem (1 + w, tips[w], true, val (ID::world) == w);
        m.addSectionHeader ("TRANCE GATE  (in the song tempo)");
        for (int g = 0; g < Choices::gates.size(); ++g) m.addItem (20 + g, Choices::gates[g], true, val (ID::gate) == g);
        m.addSectionHeader ("CLIPPER  (whole output)");
        for (int c = 0; c < Choices::clipModes.size(); ++c) m.addItem (40 + c, Choices::clipModes[c] + String (c == 3 ? "  (warm even harmonics)" : ""), true, val (ID::clipMode) == c);
        m.addSeparator();
        m.addItem (60, "World amount, gate depth, clip drive...  (PARAMS > FX)");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&worldBtn), [safe = Component::SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            if (r >= 1 && r <= 9) setParamFromUi (safe->proc, ID::world, (float) (r - 1));
            else if (r >= 20 && r < 40) setParamFromUi (safe->proc, ID::gate, (float) (r - 20));
            else if (r >= 40 && r < 60) setParamFromUi (safe->proc, ID::clipMode, (float) (r - 40));
            else if (r == 60) { safe->openTab (tabParams); safe->advanced->showTab (4); }
            safe->lastWorldSig = -1;
        });
    }

    void timerCallback() override
    {
        proc.moduleHousekeeping();
        {
            const int w = (int) proc.apvts.getRawParameterValue (ID::world)->load(), gt = (int) proc.apvts.getRawParameterValue (ID::gate)->load();
            const int sig = w * 10 + gt;
            if (sig != lastWorldSig)
            {
                lastWorldSig = sig;
                worldBtn.setButtonText (w == 0 ? String ("WORLD") : String (kk::WorldStage::name (w)));
                worldBtn.selected = w != 0 || gt != 0; worldBtn.repaint();
            }
        }
        const float l = proc.meterL.exchange (0.0f), r = proc.meterR.exchange (0.0f);
        const float nl = std::max (l, meter.l * 0.8f), nr = std::max (r, meter.r * 0.8f);
        if (proc.overload.exchange (false)) warnHold = 45;
        const bool warn = warnHold > 0 && proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f;
        if (warnHold > 0) --warnHold;
        if (std::abs (nl - meter.l) > 1.0e-4f || std::abs (nr - meter.r) > 1.0e-4f || warn != meter.warn)
        { meter.l = nl < 1.0e-4f ? 0.0f : nl; meter.r = nr < 1.0e-4f ? 0.0f : nr; meter.warn = warn; meter.repaint(); }

        if (breedBtn.flash > 0) { breedBtn.flash = std::max (0.0f, breedBtn.flash - 0.08f); breedBtn.repaint(); }
        proc.renderNextThumbnail();   // one child waveform per tick keeps the UI smooth

        const bool mouseDown = ModifierKeys::currentModifiers.isAnyMouseButtonDown();
        if (++slowTick % 10 == 0)
        {
            modifiedNow = proc.isModified();
            if (! mouseDown) proc.captureUndo();
            updateTabs();
        }
        const bool bassNow = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f;
        const float eraNow = proc.breedWild;
        if (proc.currentName() != lastName || proc.currentPresetIndex() != lastIndex || modifiedNow != modified || bassNow != lastBass
            || proc.labVersion() != lastLab || eraNow != lastEra)
        {
            lastName = proc.currentName(); lastIndex = proc.currentPresetIndex(); modified = modifiedNow; lastBass = bassNow; lastEra = eraNow;
            refreshState();
        }
        uint64_t hash = 0;
        for (size_t i = 0; i < 128; ++i) if (proc.playing[i].load()) hash = hash * 131 + i + 1;
        const float lockHash = proc.apvts.getRawParameterValue (ID::keyLock)->load() * 1000 + proc.apvts.getRawParameterValue (ID::key)->load() * 10
                             + proc.apvts.getRawParameterValue (ID::scale)->load();
        if (hash != lastPlayHash || lockHash != lastLockHash) { lastPlayHash = hash; lastLockHash = lockHash; keyboard.repaint(); }
    }

    LabCacheHolder imageCache;   // first member: images outlive every component that paints them
    KeysKillaProcessor& proc;
    KKLookAndFeel lnf;
    std::unique_ptr<PropertiesFile> settings;

    HotButton prevBtn { lnf }, nextBtn { lnf }, saveBtn { lnf }, menuBtn { lnf }, nameBtn { lnf }, heartBtn { lnf }, moonBtn { lnf }, worldBtn { lnf };
    int lastWorldSig = -1;
    ParentCard parentA, parentB;
    HotButton prevA { lnf }, nextA { lnf }, diceA { lnf }, prevB { lnf }, nextB { lnf }, diceB { lnf };
    BreedButton breedBtn;
    std::vector<std::unique_ptr<ChildCard>> childCards;
    std::vector<std::unique_ptr<GeneSwitch>> genes;
    std::vector<std::unique_ptr<HotButton>> mutateBtns, tabs;
    HotButton treeBtn { lnf }, undoBtn { lnf }, soundModeBtn { lnf }, loopModeBtn { lnf };
    int dropRound = 0;
    WildRail wildRail;
    DragToDaw dragDaw;
    std::vector<std::unique_ptr<ImageKnob>> macros, sideKnobs;
    std::vector<std::unique_ptr<MacroCaption>> captions;
    WheelSlider pitchWheel, modWheel;
    MeterOverlay meter;
    KKKeyboard keyboard;
    std::unique_ptr<AdvancedPage> advanced;
    std::unique_ptr<PresetBrowser> browser;
    std::array<std::unique_ptr<Component>, numPages> modules;
    std::unique_ptr<FamilyTreePanel> treePanel;
    LabSwitch labSwitch { lnf };

    void spaceAction()
    {
        if (treePanel != nullptr && treePanel->isVisible() && ! proc.treeKids().empty())
        {
            if (proc.loopPlaying()) proc.stopLoop();
            else if (proc.getTreeMode() == KeysKillaProcessor::treeLoop) proc.playTreeResult (jmax (0, proc.treeSelected()));
            else proc.previewNote = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f ? 36 : 60;
            treePanel->refresh();
            return;
        }
        proc.previewNote = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f ? 36 : 60;   // hear the current sound
    }
    struct FocusGrabber : public MouseListener
    {
        MainPage* page = nullptr;
        void mouseDown (const MouseEvent& e) override
        {
            if (page == nullptr || ! page->keysToPlugin() || dynamic_cast<TextEditor*> (e.eventComponent) != nullptr) return;
            if (! page->hasKeyboardFocus (true)) page->grabKeyboardFocus();
        }
    } focusGrabber;
    std::unique_ptr<FileChooser> chooser;

    bool isFav = false, modified = false, modifiedNow = false, lastBass = false;
    int warnHold = 0, lastIndex = -2, slowTick = 0, lastLab = -1, openTabIndex = -1, lastMutate = -1;
    float lastEra = -1;
    String lastName;
    uint64_t lastPlayHash = 0;
    float lastLockHash = -1;

    // attachments last: they must be destroyed before the controls they point to
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> attachments;
};

//==============================================================================
KeysKillaEditor::KeysKillaEditor (KeysKillaProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    page = std::make_unique<MainPage> (p);
    addAndMakeVisible (*page);
    setResizable (true, true);
    setResizeLimits (designW / 2, designH / 2, designW, designH);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) designW / designH);
    const int pct = page->preferredScale();
    setSize (designW * pct / 100, designH * pct / 100);
}

KeysKillaEditor::~KeysKillaEditor() = default;

void KeysKillaEditor::showView (int v) { page->showView (v); }

// Windows: draw with the software renderer. The GUI is bitmap based, so it costs nothing, and it keeps the
// plugin out of the host's Direct2D device - hosts can hang on exit when plugin windows hold GPU resources.
void KeysKillaEditor::parentHierarchyChanged()
{
   #if JUCE_WINDOWS
    if (auto* peer = getPeer())
        if (peer->getCurrentRenderingEngine() != 0)
            peer->setCurrentRenderingEngine (0);
   #endif
}

void KeysKillaEditor::resized()
{
    page->setTransform (AffineTransform::scale ((float) getWidth() / designW));
    page->setBounds (0, 0, designW, designH);
}
