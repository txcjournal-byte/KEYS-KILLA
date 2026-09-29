#include "PluginEditor.h"
#include <numeric>

using namespace juce;

#include "UiCommon.h"
#include "BinaryData.h"

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
        imgs.bg = load (BinaryData::lab_bg_jpg, BinaryData::lab_bg_jpgSize);
        imgs.white = load (BinaryData::lab_white_png, BinaryData::lab_white_pngSize);
        imgs.black = load (BinaryData::lab_black_png, BinaryData::lab_black_pngSize);
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
            g.setColour (s.accent.withAlpha (0.16f)); g.strokePath (arc, PathStrokeType (12.0f * w, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (s.accent.withAlpha (0.4f));  g.strokePath (arc, PathStrokeType (6.0f * w, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (s.accent);                   g.strokePath (arc, PathStrokeType (3.2f * w, PathStrokeType::curved, PathStrokeType::rounded));
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
    HotButton (KKLookAndFeel& l, String label = {}) : Button (label), lnf (l), text (std::move (label)) {}
    std::function<void (Graphics&, Rectangle<float>, const Skin&)> glyph;
    bool selected = false, round = false;
    std::function<void()> onRightClick;

    void paintButton (Graphics& g, bool over, bool down) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (2);
        const float corner = round ? r.getHeight() * 0.5f : 5.0f;
        if (selected) { drawGlowFrame (g, r, s.accent, corner); g.setColour (s.accent.withAlpha (0.12f)); g.fillRoundedRectangle (r, corner); }
        if (over && ! selected) { g.setColour (s.accent.withAlpha (down ? 0.25f : 0.12f)); g.fillRoundedRectangle (r, corner); }
        if (text.isNotEmpty())
        {
            g.setColour (selected ? Colour (0xffffe9e9) : Colour (0xffd9d3d3));
            g.setFont (serif (r.getHeight() * 0.5f, false, 0.12f));
            g.drawText (text, r, Justification::centred);
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
    String text;
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
                const Colour c = (warn && i >= segs - 3) ? Colours::orange : (i >= segs - 2 ? Colour (0xffff6060) : s.accent);
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
        g.setColour (Colour (0xff0b0909));
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

// MOVEMENT: chord, key, glide and the performance controls
static const StringArray movementIds { ID::chord, ID::chordType, ID::strum, ID::keyLock, ID::key, ID::scale, ID::mono, ID::legato, ID::glide, ID::bendRange,
                                       ID::timeM, ID::alive, ID::drift, ID::punch, ID::halftime, ID::lfoRate, ID::lfoPitch, ID::lfoSync, ID::lfoDiv, ID::future };
class PlayPanel : public TabPanel
{
public:
    PlayPanel (KeysKillaProcessor& p, KKLookAndFeel& l) : TabPanel (p, l, "MOVEMENT", movementIds), grid (p, movementIds, 10) { addAndMakeVisible (grid); }
    void layout (Rectangle<int> r) override { grid.setBounds (r.withHeight (jmin (r.getHeight(), 250))); }
private:
    ParamGrid grid;
};

// ARP: step arpeggiator - 16 steps with velocity, trap scale runs, patterns
class StepGrid : public Component, public SettableTooltipClient, private Timer
{
public:
    StepGrid (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        setTooltip ("Click a step to switch it on / off, drag up / down for its velocity. Steps after ARP STEPS are skipped.");
        startTimerHz (20);
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const int steps = (int) proc.apvts.getRawParameterValue (ID::arpSteps)->load();
        const bool on = proc.apvts.getRawParameterValue (ID::arp)->load() > 0.5f;
        const int cur = proc.arpCurStep.load();
        for (int i = 0; i < 16; ++i)
        {
            auto c = cell (i);
            const float v = proc.apvts.getRawParameterValue (ID::arpStep (i))->load();
            const bool active = i < steps;
            g.setColour (Colour (0xff141010)); g.fillRoundedRectangle (c, 4);
            g.setColour (active ? Colour (0xff3b3434) : Colour (0xff201c1c)); g.drawRoundedRectangle (c, 4, 1.2f);
            if (v > 0.02f)
            {
                auto bar = c.reduced (5).withTrimmedTop ((c.getHeight() - 10) * (1.0f - v));
                g.setColour ((active ? s.accent : s.accent.withAlpha (0.3f)).withAlpha (active ? 0.9f : 0.3f));
                g.fillRoundedRectangle (bar, 3);
            }
            if (on && i == cur) { g.setColour (Colours::white.withAlpha (0.8f)); g.drawRoundedRectangle (c.expanded (2), 5, 2.0f); }
            g.setColour (Colour (0x99d9d3d3)); g.setFont (serif (13.0f));
            g.drawText (String (i + 1), c.withY (c.getBottom() + 2).withHeight (16), Justification::centred);
        }
    }
    void mouseDown (const MouseEvent& e) override
    {
        drag = -1;
        for (int i = 0; i < 16; ++i) if (cell (i).contains (e.position)) drag = i;
        if (drag < 0) return;
        auto* prm = proc.apvts.getParameter (ID::arpStep (drag));
        startVal = prm->getValue();
        prm->beginChangeGesture();
        moved = false;
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (drag < 0) return;
        moved = moved || std::abs (e.getDistanceFromDragStartY()) > 3;
        if (! moved) return;
        auto c = cell (drag);
        const float v = jlimit (0.0f, 1.0f, 1.0f - (e.position.y - c.getY()) / c.getHeight());
        proc.apvts.getParameter (ID::arpStep (drag))->setValueNotifyingHost (v);
        repaint();
    }
    void mouseUp (const MouseEvent&) override
    {
        if (drag < 0) return;
        auto* prm = proc.apvts.getParameter (ID::arpStep (drag));
        if (! moved) prm->setValueNotifyingHost (startVal > 0.02f ? 0.0f : 1.0f);
        prm->endChangeGesture();
        drag = -1; repaint();
    }
private:
    Rectangle<float> cell (int i) const
    {
        const float w = (float) getWidth() / 16.0f;
        return { w * (float) i + 4.0f, 0.0f, w - 8.0f, (float) getHeight() - 20.0f };
    }
    void timerCallback() override { const int c = proc.arpCurStep.load(); if (c != lastCur) { lastCur = c; repaint(); } }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    int drag = -1, lastCur = -2; float startVal = 0; bool moved = false;
};

static StringArray arpIds()
{
    StringArray ids { ID::arp, ID::arpRate, ID::arpMode, ID::arpOct, ID::arpGate, ID::arpSwing, ID::arpSteps, ID::keyLock, ID::key, ID::scale };
    for (int i = 0; i < 16; ++i) ids.add (ID::arpStep (i));
    return ids;
}

class ArpPanel : public TabPanel
{
public:
    ArpPanel (KeysKillaProcessor& p, KKLookAndFeel& l)
        : TabPanel (p, l, "ARP", arpIds()),
          grid (p, { ID::arp, ID::arpRate, ID::arpMode, ID::arpOct, ID::arpGate, ID::arpSwing, ID::arpSteps, ID::keyLock, ID::key, ID::scale }, 10,
                { "ARP", "SPEED", "MODE", "OCTAVES", "GATE", "SHUFFLE", "STEPS", "KEY LOCK", "KEY", "SCALE" }),
          steps (p, l)
    {
        addAndMakeVisible (grid); addAndMakeVisible (steps);
        static const char* names[] { "ALL ON", "TRAP BOUNCE", "OFFBEAT", "DOTTED", "ROLL", "RANDOM" };
        for (int i = 0; i < 6; ++i)
        {
            auto b = std::make_unique<TextButton> (names[i]);
            b->onClick = [this, i] { pattern (i); };
            addAndMakeVisible (*b);
            patterns.push_back (std::move (b));
        }
        hint.setText ("MODE Scale Up / Scale Down plays trap runs in KEY + SCALE (e.g. C Minor, Phrygian, Harmonic Minor). KEY LOCK keeps every note in the scale.",
                      dontSendNotification);
        hint.setFont (serif (15.0f, false, 0.04f));
        hint.setColour (Label::textColourId, Colour (0xffb9b0ac));
        addAndMakeVisible (hint);
    }
    void layout (Rectangle<int> r) override
    {
        grid.setBounds (r.removeFromTop (118));
        r.removeFromTop (10);
        auto row = r.removeFromBottom (40);
        hint.setBounds (r.removeFromBottom (26));
        const int w = row.getWidth() / 6;
        for (auto& b : patterns) b->setBounds (row.removeFromLeft (w).reduced (6, 2));
        steps.setBounds (r.reduced (0, 6));
    }
private:
    void pattern (int i)
    {
        static const float pat[5][16] {
            { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
            { 1, 0, 0.6f, 1, 0, 1, 0.6f, 0, 1, 0, 0.6f, 1, 0, 0.8f, 1, 0.6f },
            { 0, 1, 0, 0.9f, 0, 1, 0, 0.9f, 0, 1, 0, 0.9f, 0, 1, 0, 0.9f },
            { 1, 0, 0, 0.9f, 0, 0, 1, 0, 0, 0.9f, 0, 0, 1, 0, 0.8f, 0 },
            { 1, 0.45f, 0.7f, 0.45f, 1, 0.45f, 0.7f, 0.45f, 1, 0.5f, 0.75f, 0.5f, 1, 0.6f, 0.8f, 0.9f } };
        Random rnd;
        for (int st = 0; st < 16; ++st)
        {
            auto* prm = proc.apvts.getParameter (ID::arpStep (st));
            const float v = i < 5 ? pat[i][st] : (rnd.nextFloat() < 0.3f ? 0.0f : 0.4f + 0.6f * rnd.nextFloat());
            prm->beginChangeGesture(); prm->setValueNotifyingHost (v); prm->endChangeGesture();
        }
        steps.repaint();
    }
    ParamGrid grid;
    StepGrid steps;
    std::vector<std::unique_ptr<TextButton>> patterns;
    Label hint;
};

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
        g.setColour (Colour (0xffb9b0ac));
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
        // waveform (mirrored bars)
        const Rectangle<float> wv (14, 42, 126, 46);
        const Colour col = sel ? s.accent : Colour (0xffd8d2d2);
        for (int b = 0; b < 64; ++b)
        {
            const float v = c.waveReady ? std::pow (c.wave[(size_t) b], 0.7f) : 0.04f;
            const float h = std::max (1.0f, v * wv.getHeight() * 0.5f);
            const float x = wv.getX() + (float) b * wv.getWidth() / 64.0f;
            g.setColour (col.withAlpha (sel ? 0.95f : 0.8f));
            g.fillRect (x, wv.getCentreY() - h, 1.3f, h * 2.0f);
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
        if (e.position.getDistanceFrom ({ 160.0f, 66.0f }) < 20.0f) proc.previewChild (index);   // play button
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
            g.setColour (Colour (0xffd9d3d3)); g.setFont (serif (15.0f, false, 0.1f));
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
class MainPage : public Component, private Timer
{
public:
    explicit MainPage (KeysKillaProcessor& p)
        : proc (p), parentA (p, lnf, 0), parentB (p, lnf, 1), breedBtn (lnf), wildRail (p, lnf),
          pitchWheel (lnf), modWheel (lnf), meter (lnf), keyboard (p, lnf)
    {
        settings = openSettings();
        lnf.setSkin (Skin::all()[1]);   // BLOOD
        setLookAndFeel (&lnf);

        // ---- header
        prevBtn.onClick = [this] { step (-1); }; prevBtn.setTooltip ("Previous preset");
        nextBtn.onClick = [this] { step (1); };  nextBtn.setTooltip ("Next preset");
        saveBtn.onClick = [this] { savePreset(); }; saveBtn.setTooltip ("Save this sound as a user preset.");
        menuBtn.onClick = [this] { showMenu(); };  menuBtn.setTooltip ("Presets, A/B, undo, ADVANCED, size.");
        nameBtn.onClick = [this] { openTab (tabBrowser); }; nameBtn.setTooltip ("Click to browse and search all presets.");
        heartBtn.onClick = [this] { toggleFavourite(); }; heartBtn.setTooltip ("Add to favourites");
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
            c->setTooltip ("CHILD " + String (i + 1) + ": click to load, play button to hear it, stars to rate (4+ stars are kept in User > Bred). Right-click: use as parent.");
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
        treeBtn.onClick = [this] { familyTree(); }; treeBtn.setTooltip ("FAMILY TREE: earlier generations and their children");
        undoBtn.onClick = [this] { proc.undo(); refreshState(); }; undoBtn.setTooltip ("UNDO the last change of the sound");
        addAndMakeVisible (treeBtn); addAndMakeVisible (undoBtn);

        // ---- WILD rail + FUTURE / ALIVE / TIME
        addAndMakeVisible (wildRail);
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

        // ---- tabs
        static const char* tabNames[] { "", "", "", "", "", "ARP", "KILLA" };
        static const char* tabTips[] { "BROWSER: all sounds by category, subcategory, era, mood and more",
                                       "SOUND: engines A + B, filter and envelopes", "MOD: LFOs, modulation matrix, envelope 3",
                                       "MOVEMENT: chord, arp, glide, perform controls", "FX: effect rack (drag to reorder)",
                                       "ARP: 16-step arpeggiator with velocity, trap scale runs, patterns",
                                       "KILLA: ghost, bend, circuit, body swap, tape, future" };
        for (int i = 0; i < numTabs; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, tabNames[i]);
            b->setTooltip (tabTips[i]);
            b->onClick = [this, i] { openTab (i); };
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
        refreshState();
        startTimerHz (30);
    }

    ~MainPage() override
    {
        stopTimer();
        PopupMenu::dismissAllActiveMenus();   // never leave a menu pointing at a closed editor
        setLookAndFeel (nullptr);
    }

    int preferredScale() const { return jlimit (50, 100, settings->getIntValue ("labScale", 70)); }

    // tests / screenshots: 0 main, 1..8 advanced tab, 9 browser, 10 movement, 11 808
    void showView (int v)
    {
        if (v >= 1 && v <= 8) { ensureAdvanced(); advanced->showTab (v - 1); advanced->setVisible (true); advanced->toFront (false); }
        if (v == 9) openTab (tabBrowser);
        if (v == 10) openTab (tabMovement);
        if (v == 11) openTab (tabArp);
        if (v == 12) { proc.breed(); while (proc.renderNextThumbnail()) {} proc.selectChild (2); labChanged(); }
    }

    void paint (Graphics& g) override
    {
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImageAt (labImages().bg, 0, 0);
    }

    void resized() override
    {
        prevBtn.setBounds (R (642, 44, 684, 86)); nameBtn.setBounds (R (688, 46, 1044, 84)); nextBtn.setBounds (R (1044, 44, 1082, 86));
        heartBtn.setBounds (R (1086, 44, 1128, 86)); saveBtn.setBounds (R (1245, 42, 1346, 85)); menuBtn.setBounds (R (1365, 42, 1466, 85));
        moonBtn.setBounds (R (1483, 40, 1544, 87));

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
        treeBtn.setBounds (R (1300, 528, 1374, 604)); undoBtn.setBounds (R (1379, 528, 1442, 604));

        wildRail.setBounds (R (1466, 118, 1608, 392));
        sideKnobs[0]->place ({ 1532, 447 }, 33, 43);
        sideKnobs[1]->place ({ 1490, 552 }, 20, 28);
        sideKnobs[2]->place ({ 1575, 552 }, 20, 28);

        const int tx[numTabs][2] { { 55, 252 }, { 262, 472 }, { 480, 690 }, { 700, 907 }, { 916, 1127 }, { 1137, 1347 }, { 1356, 1622 } };
        for (int i = 0; i < numTabs; ++i) tabs[(size_t) i]->setBounds (R (tx[i][0], 619, tx[i][1], 655));

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
        if (advanced) advanced->setBounds (panelArea);
        if (browser) browser->setBounds (panelArea);
        if (playPanel) playPanel->setBounds (panelArea);
        if (arpPanel) arpPanel->setBounds (panelArea);
    }

private:
    enum { tabBrowser, tabSound, tabMod, tabMovement, tabFx, tabArp, tabKilla, numTabs, tabSettings = 100 };

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
        addChildComponent (*browser);
        noFocus (*browser);
        resized();
    }
    void hidePanels()
    {
        for (Component* c : { (Component*) advanced.get(), (Component*) browser.get(), (Component*) playPanel.get(), (Component*) arpPanel.get() })
            if (c != nullptr) c->setVisible (false);
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
                case tabSound:  ensureAdvanced(); advanced->showTab (0); advanced->setVisible (true); break;
                case tabMod:    ensureAdvanced(); advanced->showTab (3); advanced->setVisible (true); break;
                case tabFx:     ensureAdvanced(); advanced->showTab (4); advanced->setVisible (true); break;
                case tabKilla:  ensureAdvanced(); advanced->showTab (5); advanced->setVisible (true); break;
                case tabSettings: ensureAdvanced(); advanced->showTab (7); advanced->setVisible (true); break;
                case tabMovement:
                    if (! playPanel) { playPanel = std::make_unique<PlayPanel> (proc, lnf); addChildComponent (*playPanel); noFocus (*playPanel); resized(); }
                    playPanel->setVisible (true); break;
                case tabArp:
                    if (! arpPanel) { arpPanel = std::make_unique<ArpPanel> (proc, lnf); addChildComponent (*arpPanel); noFocus (*arpPanel); resized(); }
                    arpPanel->setVisible (true); break;
                default: break;
            }
            for (Component* c : { (Component*) advanced.get(), (Component*) browser.get(), (Component*) playPanel.get(), (Component*) arpPanel.get() })
                if (c != nullptr && c->isVisible()) c->toFront (false);
        }
        updateTabs();
    }
    bool isPanelVisible() const
    {
        for (Component* c : { (Component*) advanced.get(), (Component*) browser.get(), (Component*) playPanel.get(), (Component*) arpPanel.get() })
            if (c != nullptr && c->isVisible()) return true;
        return false;
    }
    void updateTabs()
    {
        if (! isPanelVisible()) openTabIndex = -1;
        for (int i = 0; i < numTabs; ++i) { tabs[(size_t) i]->selected = i == openTabIndex; tabs[(size_t) i]->repaint(); }
    }
    void setScale (int pct)
    {
        settings->setValue ("labScale", pct);
        if (auto* ed = findParentComponentOfClass<AudioProcessorEditor>())
            ed->setSize (KeysKillaEditor::designW * pct / 100, KeysKillaEditor::designH * pct / 100);
    }

    // ---------------- BREED LAB ----------------
    void labChanged() { lastLab = -1; refreshState(); }
    void parentMenu (int slot)
    {
        PopupMenu m, cats;
        m.addSectionHeader (slot == 0 ? "PARENT A" : "PARENT B");
        m.addItem (1, "Choose from the browser...");
        m.addItem (2, "Use the current sound");
        m.addItem (3, "Random sound");
        const auto& ps = factoryPresets();
        for (int c = 0; c < numCategories; ++c)
        {
            PopupMenu sub;
            for (int i = 0; i < (int) ps.size(); ++i) if (ps[(size_t) i].cat == c) sub.addItem (1000 + i, ps[(size_t) i].name);
            cats.addSubMenu (categoryNames()[c], sub);
        }
        m.addSubMenu ("By category", cats);
        m.showMenuAsync (PopupMenu::Options(), [this, slot, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1)
            {
                ensureBrowser(); hidePanels();
                browser->open (-1, -1, false, [this, slot] (int idx) { if (idx >= 0) proc.setParentPreset (slot, idx); else proc.setParentCurrent (slot); labChanged(); },
                               slot == 0 ? "CHOOSE PARENT A" : "CHOOSE PARENT B");
                openTabIndex = tabBrowser; updateTabs();
                return;
            }
            if (r == 2) proc.setParentCurrent (slot);
            else if (r == 3) proc.randomParent (slot);
            else if (r >= 1000) proc.setParentPreset (slot, r - 1000);
            labChanged();
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
        m.showMenuAsync (PopupMenu::Options(), [this, idx, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1 || r == 2) proc.setParentChild (r - 1, idx);
            if (r == 3) { proc.selectChild (idx); savePresetAs(); }
            if (r == 4) proc.previewChild (idx);
            labChanged();
        });
    }
    void familyTree()
    {
        PopupMenu m;
        const auto& hist = proc.generations();
        m.addSectionHeader ("FAMILY TREE");
        if (hist.empty()) m.addItem (-1, "Breed a few times - earlier generations show up here", false);
        for (int h = (int) hist.size(); --h >= 0;)
        {
            const auto& gen = hist[(size_t) h];
            PopupMenu kids;
            for (int k = 0; k < (int) gen.kids.size(); ++k)
            {
                String stars; for (int s = 0; s < gen.kids[(size_t) k].rating; ++s) stars << "*";
                kids.addItem (1 + h * 10 + k, gen.kids[(size_t) k].g.name + (stars.isNotEmpty() ? "   " + stars : String()));
            }
            m.addSubMenu (gen.parents[0].name + "  x  " + gen.parents[1].name, kids);
        }
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (treeBtn), [this, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r <= 0) return;
            const int h = (r - 1) / 10, k = (r - 1) % 10;
            proc.restoreGeneration (h);
            proc.selectChild (k);
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
        const auto& ps = factoryPresets();
        const int n = (int) ps.size();
        const int cur = proc.currentPresetIndex();
        proc.loadPreset (cur < 0 ? 0 : ((cur + dir) % n + n) % n);
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
        }
        wildRail.repaint();
        updateTabs();
    }

    void timerCallback() override
    {
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

    HotButton prevBtn { lnf }, nextBtn { lnf }, saveBtn { lnf }, menuBtn { lnf }, nameBtn { lnf }, heartBtn { lnf }, moonBtn { lnf };
    ParentCard parentA, parentB;
    HotButton prevA { lnf }, nextA { lnf }, diceA { lnf }, prevB { lnf }, nextB { lnf }, diceB { lnf };
    BreedButton breedBtn;
    std::vector<std::unique_ptr<ChildCard>> childCards;
    std::vector<std::unique_ptr<GeneSwitch>> genes;
    std::vector<std::unique_ptr<HotButton>> mutateBtns, tabs;
    HotButton treeBtn { lnf }, undoBtn { lnf };
    WildRail wildRail;
    std::vector<std::unique_ptr<ImageKnob>> macros, sideKnobs;
    std::vector<std::unique_ptr<MacroCaption>> captions;
    WheelSlider pitchWheel, modWheel;
    MeterOverlay meter;
    KKKeyboard keyboard;
    std::unique_ptr<AdvancedPage> advanced;
    std::unique_ptr<PresetBrowser> browser;
    std::unique_ptr<PlayPanel> playPanel;
    std::unique_ptr<ArpPanel> arpPanel;
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
