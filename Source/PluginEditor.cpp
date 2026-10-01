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
    HotButton (KKLookAndFeel& l, String label = {}) : Button (label), lnf (l) {}
    std::function<void (Graphics&, Rectangle<float>, const Skin&)> glyph;
    bool selected = false, round = false, framed = false;   // framed: a visible button (module pages), not a hot-spot over the bitmap
    std::function<void()> onRightClick;

    void paintButton (Graphics& g, bool over, bool down) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (2);
        const float corner = round ? r.getHeight() * 0.5f : 5.0f;
        if (framed && ! selected)
        {
            g.setColour (Colour (0xff181111)); g.fillRoundedRectangle (r, corner);
            g.setColour (Colour (0xff4a3a3a)); g.drawRoundedRectangle (r, corner, 1.2f);
        }
        if (selected) { drawGlowFrame (g, r, s.accent, corner); g.setColour (s.accent.withAlpha (0.12f)); g.fillRoundedRectangle (r, corner); }
        if (over && ! selected) { g.setColour (s.accent.withAlpha (down ? 0.25f : 0.12f)); g.fillRoundedRectangle (r, corner); }
        if (const auto text = getButtonText(); text.isNotEmpty())
        {
            g.setColour (selected ? Colour (0xffffe9e9) : Colour (0xffd9d3d3));
            g.setFont (serif (framed ? std::min (r.getHeight() * 0.5f, 20.0f) : r.getHeight() * 0.5f, false, 0.12f));
            g.drawFittedText (text, r.reduced (4, 0).toNearestInt(), Justification::centred, 1, 0.7f);
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
// SOUNDS: always-open sound list on the right (like FL Studio FLEX) - pick a category, click a sound, play
class SoundList : public Component, public SettableTooltipClient, private ListBoxModel, private Timer
{
public:
    SoundList (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        setTooltip ("SOUNDS: click a sound to load it.  Arrows / click the name: category.");
        list.setModel (this);
        list.setRowHeight (23);
        list.setColour (ListBox::backgroundColourId, Colours::transparentBlack);
        list.setOutlineThickness (0);
        list.getVerticalScrollBar().setColour (ScrollBar::thumbColourId, Colour (0x88d01818));
        list.setWantsKeyboardFocus (false);
        addAndMakeVisible (list);
        for (int c = 0; c < numCategories; ++c) if (c != c808) cats.push_back (c);   // 808 sounds live in BASS
        const auto& ps = factoryPresets();
        const int cur = proc.getCurrentProgram();
        catPos = 0;
        if (cur >= 0 && cur < (int) ps.size())
            for (int i = 0; i < (int) cats.size(); ++i) if (cats[(size_t) i] == (ps[(size_t) cur].cat == c808 ? cBass : ps[(size_t) cur].cat)) catPos = i;
        rebuild();
        startTimerHz (8);
    }
    std::function<void()> onLoaded;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.setColour (Colour (0xe00a0707)); g.fillRoundedRectangle (getLocalBounds().toFloat(), 6);
        g.setColour (s.panelEdge); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (0.5f), 6, 1.0f);
        auto h = header();
        g.setColour (Colours::white); g.setFont (serif (14.0f, false, 0.18f));
        g.drawFittedText (categoryNames()[cats[(size_t) catPos]].toUpperCase(), h.reduced (22, 0), Justification::centred, 1);
        g.setColour (s.accent);
        auto arrow = [&] (Rectangle<int> r, bool left)
        {
            Path p; const auto c = r.getCentre().toFloat();
            p.addTriangle (c.x + (left ? 3.0f : -3.0f), c.y - 5.0f, c.x + (left ? 3.0f : -3.0f), c.y + 5.0f, c.x + (left ? -4.0f : 4.0f), c.y);
            g.fillPath (p);
        };
        arrow (h.withWidth (22), true); arrow (h.withTrimmedLeft (h.getWidth() - 22), false);
        g.setColour (s.panelEdge); g.drawHorizontalLine (h.getBottom(), 6.0f, (float) getWidth() - 6.0f);
    }
    void resized() override { list.setBounds (getLocalBounds().withTrimmedTop (header().getBottom() + 2).reduced (2, 2)); }
    void mouseUp (const MouseEvent& e) override
    {
        const auto h = header();
        if (! h.contains (e.getPosition())) return;
        const int n = (int) cats.size();
        if (e.x < 24) { catPos = (catPos + n - 1) % n; rebuild(); return; }
        if (e.x > h.getWidth() - 24) { catPos = (catPos + 1) % n; rebuild(); return; }
        PopupMenu m;   // click the name: every category at once
        for (int i = 0; i < n; ++i) m.addItem (i + 1, categoryNames()[cats[(size_t) i]], true, i == catPos);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (this), [safe = Component::SafePointer<SoundList> (this)] (int r)
        {
            if (safe != nullptr && r > 0) { safe->catPos = r - 1; safe->rebuild(); }
        });
    }
private:
    Rectangle<int> header() const { return { 0, 0, getWidth(), 28 }; }
    void rebuild()
    {
        rows.clear();
        const auto& ps = factoryPresets();
        const int c = cats[(size_t) catPos];
        for (int i = 0; i < (int) ps.size(); ++i)
            if (ps[(size_t) i].cat == c || (c == cBass && ps[(size_t) i].cat == c808)) rows.push_back (i);
        list.updateContent(); list.setVerticalPosition (0); repaint();
        lastCur = -2;
        timerCallback();
    }
    int getNumRows() override { return (int) rows.size(); }
    void paintListBoxItem (int row, Graphics& g, int w, int h, bool) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        const bool cur = rows[(size_t) row] == proc.getCurrentProgram();
        if (cur) { g.setColour (lnf.skin->accent.withAlpha (0.28f)); g.fillRoundedRectangle (2.0f, 1.0f, (float) w - 4.0f, (float) h - 2.0f, 4.0f); }
        g.setColour (cur ? Colours::white : Colour (0xffc9c0bd)); g.setFont (Font (FontOptions (14.5f)));
        g.drawFittedText (factoryPresets()[(size_t) rows[(size_t) row]].name, 7, 0, w - 10, h, Justification::centredLeft, 1, 0.8f);
    }
    void listBoxItemClicked (int row, const MouseEvent&) override
    {
        if (row < 0 || row >= (int) rows.size()) return;
        proc.loadPreset (rows[(size_t) row]);
        list.repaint();
        if (onLoaded) onLoaded();
    }
    String getTooltipForRow (int row) override
    {
        return row >= 0 && row < (int) rows.size() ? factoryPresets()[(size_t) rows[(size_t) row]].info() : String();
    }
    void timerCallback() override
    {
        const int cur = proc.getCurrentProgram();
        if (cur == lastCur) return;
        lastCur = cur;
        for (int r = 0; r < (int) rows.size(); ++r) if (rows[(size_t) r] == cur) { list.scrollToEnsureRowIsOnscreen (r); break; }
        list.repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    ListBox list;
    std::vector<int> cats, rows;
    int catPos = 0, lastCur = -2;
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
        g.setColour (Colour (an.valid() ? 0xff181212 : 0xff110d0d)); g.fillRoundedRectangle (r, 8);
        if (an.valid()) drawGlowFrame (g, r, s.accent.withAlpha (over ? 1.0f : 0.55f), 8);
        else
        {
            Path o; o.addRoundedRectangle (r, 8); const float dash[] { 6, 5 };
            PathStrokeType (1.2f).createDashedStroke (o, o, dash, 2);
            g.setColour (over ? s.accent : Colour (0xff4a3c3c)); g.fillPath (o);
        }
        g.setColour (Colour (0xffd9d3d3)); g.setFont (serif (15.0f, false, 0.3f));
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
        g.setGradientFill (ColourGradient (Colour (0xff3a0c0c), c.x, c.y - R, Colour (0xff120606), c.x, c.y + R, false));
        g.fillEllipse (Rectangle<float> (2 * R * 0.84f, 2 * R * 0.84f).withCentre (c));
        g.setColour (s.accent); g.drawEllipse (Rectangle<float> (2 * R * 0.84f, 2 * R * 0.84f).withCentre (c), 3.0f);
        g.setColour (s.accent.withAlpha (0.5f)); g.drawEllipse (Rectangle<float> (2 * R, 2 * R).withCentre (c), 1.5f);
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
        g.setColour (Colour (sel ? 0xff221212 : 0xff151010)); g.fillRoundedRectangle (r, 7);
        if (sel) drawGlowFrame (g, r, s.accent, 7);
        else { g.setColour (over && has ? s.accent.withAlpha (0.7f) : Colour (0xff3a3030)); g.drawRoundedRectangle (r, 7, 1.2f); }
        g.setColour (Colour (0xffd9d3d3)); g.setFont (serif (15.0f, false, 0.3f));
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
            g.setColour (Colour (0xff0c0909)); g.fillRoundedRectangle (area, 4);
            drawLoopRoll (g, area.reduced (5, 5), proc.loopNotes (t.g), (float) proc.loopBars() * 4.0f, sel ? s.accent : Colour (0xffcfc6c6),
                          proc.loopIsTree (index) ? proc.loopBeat.load() : -1.0f);
        }
        else
            for (int b = 0; b < 64; ++b)
            {
                const float v = t.waveReady ? std::pow (t.wave[(size_t) b], 0.7f) : 0.04f;
                const float h = std::max (1.0f, v * area.getHeight() * 0.5f);
                g.setColour ((sel ? s.accent : Colour (0xffd8d2d2)).withAlpha (0.9f));
                g.fillRect (area.getX() + (float) b * area.getWidth() / 64.0f, area.getCentreY() - h, 1.6f, h * 2.0f);
            }
        // play / loop button
        const auto pc = playCentre();
        const bool playing = proc.loopIsTree (index);
        g.setColour (playing ? s.accent : Colour (0xff2a2020)); g.fillEllipse (Rectangle<float> (30, 30).withCentre (pc));
        g.setColour (s.accent.withAlpha (0.8f)); g.drawEllipse (Rectangle<float> (30, 30).withCentre (pc), 1.5f);
        g.setColour (Colours::white);
        if (playing) g.fillRect (Rectangle<float> (10, 10).withCentre (pc));
        else { Path tri; tri.addTriangle (pc.x - 4, pc.y - 7, pc.x - 4, pc.y + 7, pc.x + 8, pc.y); g.fillPath (tri); }
        // name + stars
        g.setColour (Colour (0xffc9c0c0)); g.setFont (serif (13.0f, false, 0.02f));
        auto info = t.g.name;
        if (loopMode) info = String (kk::keyName (proc.effectiveLoopKey (t.g.loop))) + " MIN  .  " + info;
        g.drawFittedText (info, Rectangle<int> (14, (int) r.getBottom() - 42, (int) r.getWidth() - 20, 18), Justification::centredLeft, 1, 0.7f);
        for (int st = 0; st < 5; ++st)
        {
            auto sr = starRect (st);
            g.setColour (st < t.rating ? s.accent : Colour (0xff6a6060));
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
        g.fillAll (Colour (0xff0a0707));
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
        g.setColour (Colour (0xff9c9494)); g.setFont (serif (13.0f, false, 0.25f));
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

// HALF = Voodoo Killa: 12 categories x 8 time effects on everything KEYS KILLA plays
class HalfPanel : public Component, private Timer
{
public:
    HalfPanel (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        viewCat = current() / vk::kPresetsPerCategory;
        onBtn.setClickingTogglesState (false);
        onBtn.onClick = [this] { setParamFromUi (proc, ID::halfOn, isOn() ? 0.0f : 1.0f); refresh(); };
        onBtn.setTooltip ("HALF on / off. It works on everything KEYS KILLA plays - your notes and the FAMILY TREE loops.");
        onBtn.framed = true; addAndMakeVisible (onBtn);
        const auto& cats = proc.halfLibrary().getCategories();
        for (int c = 0; c < (int) cats.size(); ++c)
        {
            auto b = std::make_unique<HotButton> (lnf, cats[(size_t) c].name);
            b->setTooltip (cats[(size_t) c].desc);
            b->onClick = [this, c] { viewCat = c; refresh(); };
            addAndMakeVisible (*b); catBtns.push_back (std::move (b));
        }
        const char* knobIds[] { ID::halfAmount, ID::halfMix };
        const char* knobTips[] { "AMOUNT: how hard the effect hits", "MIX: dry <-> HALF" };
        for (int k = 0; k < 2; ++k)
        {
            auto& kn = knobs[(size_t) k];
            kn.setSliderStyle (Slider::RotaryHorizontalVerticalDrag); kn.setTextBoxStyle (Slider::NoTextBox, false, 0, 0);
            kn.setPopupDisplayEnabled (true, true, nullptr); kn.setTooltip (knobTips[k]);
            auto* prm = proc.apvts.getParameter (knobIds[k]);
            kn.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
            atts.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, knobIds[k], kn));
            addAndMakeVisible (kn);
        }
        for (int i = 0; i < Choices::halfSpeeds.size(); ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, Choices::halfSpeeds[i]);
            b->setTooltip ("SPEED: how fast the effect runs (1x = as designed, 1/2x = twice as long)");
            b->onClick = [this, i] { setParamFromUi (proc, ID::halfSpeed, (float) i); refresh(); };
            addAndMakeVisible (*b); speedBtns.push_back (std::move (b));
        }
        static const char* trigNames[] { "ALWAYS", "EVERY 4", "EVERY 8", "LAST BEAT" };
        for (int i = 0; i < 4; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, trigNames[i]);
            b->setTooltip ("WHEN: always, once every 4 or 8 bars (like a producer's fill), or on the last beat of each bar");
            b->onClick = [this, i] { setParamFromUi (proc, ID::halfTrig, (float) i); refresh(); };
            addAndMakeVisible (*b); trigBtns.push_back (std::move (b));
        }
        refresh();
        startTimerHz (10);
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.fillAll (Colour (0xff0a0707));
        g.setColour (s.panelEdge); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8, 1.4f);
        g.setColour (Colours::white); g.setFont (serif (38.0f, true, 0.3f));
        g.drawText ("HALF", 24, 10, 160, 48, Justification::centredLeft);
        g.setColour (Colour (0xff9c9494)); g.setFont (serif (14.0f, false, 0.2f));
        g.drawText ("VOODOO KILLA ENGINE  -  HALFTIME, TAPE STOP, GLITCH, BACKMASK ... ON EVERYTHING KEYS KILLA PLAYS", 200, 22, 900, 24, Justification::centredLeft);
        const auto& f = proc.halfLibrary().getFactory();
        for (int c = 0; c < vk::kPresetsPerCategory; ++c)
        {
            const int idx = viewCat * vk::kPresetsPerCategory + c;
            if (idx >= (int) f.size()) break;
            const auto r = card (c).toFloat();
            const bool sel = idx == current();
            g.setColour (Colour (sel ? 0xff2a0d0d : 0xff141010)); g.fillRoundedRectangle (r, 8);
            if (sel) drawGlowFrame (g, r, isOn() ? s.accent : s.accent.withAlpha (0.45f), 8);
            else { g.setColour (Colour (0xff3a2e2e)); g.drawRoundedRectangle (r, 8, 1.2f); }
            g.setColour (sel ? Colours::white : Colour (0xffe0d8d6)); g.setFont (serif (21.0f, false, 0.12f));
            g.drawText (f[(size_t) idx].name, r.reduced (16, 10).withHeight (30), Justification::centredLeft);
            g.setColour (Colour (0xff9c9494)); g.setFont (serif (13.5f, false, 0.05f));
            g.drawFittedText (f[(size_t) idx].desc, r.reduced (16, 10).withTrimmedTop (34).toNearestInt(), Justification::topLeft, 3);
        }
        g.setColour (Colour (0xffb9b0ac)); g.setFont (serif (14.0f, false, 0.25f));
        const int by = getHeight() - 118;
        g.drawText ("AMOUNT", knobs[0].getBounds().withY (by + 92).withHeight (18), Justification::centred);
        g.drawText ("MIX", knobs[1].getBounds().withY (by + 92).withHeight (18), Justification::centred);
        g.drawText ("SPEED", speedBtns.front()->getX(), by + 4, 200, 18, Justification::centredLeft);
        g.drawText ("WHEN", trigBtns.front()->getX(), by + 4, 200, 18, Justification::centredLeft);
        g.setColour (Colour (0xff9c9494)); g.setFont (serif (13.0f, false, 0.25f));
        g.drawText ("CLICK A CARD = HALF ON.  SYNCED TO THE FL STUDIO TEMPO, FREE-RUNNING WHEN FL IS STOPPED.",
                    Rectangle<int> (20, getHeight() - 24, getWidth() - 40, 18), Justification::centredRight);
    }
    void resized() override
    {
        const int w = getWidth(), h = getHeight();
        onBtn.setBounds (w - 170, 14, 146, 42);
        const int n = (int) catBtns.size(), perRow = (n + 1) / 2, cw = perRow > 0 ? (w - 40 - (perRow - 1) * 6) / perRow : 0;
        for (int c = 0; c < n; ++c) catBtns[(size_t) c]->setBounds (20 + (c % perRow) * (cw + 6), 64 + (c / perRow) * 36, cw, 34);
        const int by = h - 118;
        knobs[0].setBounds (30, by + 4, 90, 88); knobs[1].setBounds (140, by + 4, 90, 88);
        for (int i = 0; i < (int) speedBtns.size(); ++i) speedBtns[(size_t) i]->setBounds (290 + i * 82, by + 28, 78, 40);
        for (int i = 0; i < (int) trigBtns.size(); ++i) trigBtns[(size_t) i]->setBounds (740 + i * 124, by + 28, 120, 40);
    }
    void refresh()
    {
        viewCat = jlimit (0, std::max (0, (int) catBtns.size() - 1), viewCat);
        onBtn.setButtonText (isOn() ? "HALF  ON" : "HALF  OFF");
        onBtn.selected = isOn();
        for (int c = 0; c < (int) catBtns.size(); ++c) { catBtns[(size_t) c]->selected = c == viewCat; catBtns[(size_t) c]->repaint(); }
        const int sp = (int) proc.apvts.getRawParameterValue (ID::halfSpeed)->load(), tr = (int) proc.apvts.getRawParameterValue (ID::halfTrig)->load();
        for (int i = 0; i < (int) speedBtns.size(); ++i) { speedBtns[(size_t) i]->selected = i == sp; speedBtns[(size_t) i]->repaint(); }
        for (int i = 0; i < (int) trigBtns.size(); ++i) { trigBtns[(size_t) i]->selected = i == tr; trigBtns[(size_t) i]->repaint(); }
        lastState = state();
        repaint();
    }
    void mouseUp (const MouseEvent& e) override
    {
        for (int c = 0; c < vk::kPresetsPerCategory; ++c)
            if (card (c).contains (e.getPosition()))
            {
                const int idx = viewCat * vk::kPresetsPerCategory + c;
                if (idx == current() && isOn()) setParamFromUi (proc, ID::halfOn, 0.0f);   // click the lit card again = off
                else { setParamFromUi (proc, ID::halfPreset, (float) idx); setParamFromUi (proc, ID::halfOn, 1.0f); }
                refresh(); return;
            }
    }
private:
    Rectangle<int> card (int c) const
    {
        const int w = getWidth(), cw = (w - 40 - 3 * 14) / 4, chh = (getHeight() - 118 - 148 - 14) / 2;
        return { 20 + (c % 4) * (cw + 14), 148 + (c / 4) * (chh + 12), cw, chh };
    }
    int current() const { return (int) proc.apvts.getRawParameterValue (ID::halfPreset)->load(); }
    bool isOn() const { return proc.apvts.getRawParameterValue (ID::halfOn)->load() > 0.5f; }
    int state() const { return current() * 1000 + (isOn() ? 500 : 0) + (int) proc.apvts.getRawParameterValue (ID::halfSpeed)->load() * 10
                               + (int) proc.apvts.getRawParameterValue (ID::halfTrig)->load(); }
    void timerCallback() override { if (isVisible() && state() != lastState) refresh(); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf, "HALF" };
    std::vector<std::unique_ptr<HotButton>> catBtns, speedBtns, trigBtns;
    std::array<Slider, 2> knobs;
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> atts;
    int viewCat = 0, lastState = -1;
};

// ---------------- shared building blocks for the module pages ----------------
class ModulePage : public Component, protected Timer
{
public:
    ModulePage (KeysKillaProcessor& p, KKLookAndFeel& l, String t, String sub) : proc (p), lnf (l), title (std::move (t)), subtitle (std::move (sub)) {}
    ~ModulePage() override { stopTimer(); }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.fillAll (Colour (0xff0a0707));
        g.setColour (s.panelEdge); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8, 1.4f);
        g.setColour (Colours::white); g.setFont (serif (38.0f, true, 0.3f));
        const int tw = (int) std::ceil (GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 30;
        g.drawText (title, 24, 10, tw, 48, Justification::centredLeft);
        g.setColour (Colour (0xff9c9494)); g.setFont (serif (14.0f, false, 0.2f));
        g.drawText (subtitle, 24 + tw + 10, 22, 760, 24, Justification::centredLeft);
        g.setColour (Colour (0xffb9b0ac)); g.setFont (serif (13.0f, false, 0.25f));
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

// 808 / SNARE / CLAP: kits, knobs, BREED six variants, a pad, the keys switch
class DrumPanel : public ModulePage
{
public:
    DrumPanel (KeysKillaProcessor& p, KKLookAndFeel& l, int playMode) : ModulePage (p, l, titleOf (playMode), subOf (playMode)), mode (playMode)
    {
        for (auto& kit : KeysKillaProcessor::drumKits (mode))
        {
            auto b = std::make_unique<HotButton> (lnf, kit.name);
            const auto values = kit.values;
            b->onClick = [this, values] { proc.applyDrumKit (values); proc.hitPad (mode); };
            b->setTooltip ("Load this sound (and hear it)");
            b->framed = true; addAndMakeVisible (*b); kitBtns.push_back (std::move (b));
        }
        if (mode == KeysKillaProcessor::play808)
        {
            addKnob (ID::b8Tune, "TUNE", "Tune in semitones"); addKnob (ID::b8Decay, "DECAY", "How long the 808 rings");
            addKnob (ID::b8Punch, "PUNCH", "Pitch drop at the start: the knock"); addKnob (ID::b8Glide, "GLIDE", "Slide time: overlap two notes in FL and the 808 slides");
            addKnob (ID::b8Tone, "TONE", "Harmonics: clean sub -> gritty 808 that cuts through phone speakers");
            addKnob (ID::b8Click, "CLICK", "Transient layer: a click on the first sample of the note (phase-locked with the sub)");
            addKnob (ID::b8Drive, "DRIVE", "808 Killa drive - only on the mids / highs, the sub stays clean and mono"); addKnob (ID::b8Clip, "CLIP", "808 Killa soft clipper");
            addKnob (ID::b8Width, "WIDTH", "Stereo only above 300 Hz - the sub stays mono"); addKnob (ID::b8Level, "LEVEL", "Output level");
            addChips (ID::b8Sat, "SATURATION", Choices::satModes, "808 Killa saturation: Tape (warm), Tube (punchy), Foldback (aggressive)");
        }
        else if (mode == KeysKillaProcessor::playSnare)
        {
            addKnob (ID::snTune, "TUNE", "Tune in semitones"); addKnob (ID::snBody, "BODY", "The drum: low punch");
            addKnob (ID::snSnap, "SNAP", "The wires: noise and crack"); addKnob (ID::snDecay, "DECAY", "Length");
            addKnob (ID::snTone, "TONE", "Dark -> bright"); addKnob (ID::snLevel, "LEVEL", "Output level");
        }
        else
        {
            addKnob (ID::clTune, "TUNE", "Tune in semitones"); addKnob (ID::clSpread, "SPREAD", "Distance between the hands");
            addKnob (ID::clDecay, "DECAY", "Room tail"); addKnob (ID::clTone, "TONE", "Dark -> bright");
            addKnob (ID::clWidth, "WIDTH", "Stereo width"); addKnob (ID::clLevel, "LEVEL", "Output level");
        }
        hit.setButtonText ("HIT"); hit.setTooltip ("Hear it"); hit.onClick = [this] { proc.hitPad (mode); }; hit.framed = true; addAndMakeVisible (hit);
        breed.setButtonText ("BREED"); breed.setTooltip ("BREED: six new " + title + " sounds from this one and a random kit");
        breed.onClick = [this] { breedKids(); }; breed.framed = true; addAndMakeVisible (breed);
        for (int i = 0; i < 6; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, "CHILD " + String (i + 1));
            b->onClick = [this, i] { if (i < (int) kids.size()) { proc.applyDrumKit (kids[(size_t) i]); proc.hitPad (mode); selKid = i; refresh(); } };
            b->setTooltip ("Load this child (and hear it)");
            b->framed = true; addAndMakeVisible (*b); kidBtns.push_back (std::move (b));
        }
        setupKeysButton (keysBtn, mode, title);
        refresh();
        startTimerHz (10);
    }
    void resized() override
    {
        const int w = getWidth(), h = getHeight();
        keysBtn.setBounds (w - 300, 14, 276, 42);
        const int n = (int) kitBtns.size(), per = n > 6 ? (n + 1) / 2 : n, cw = per > 0 ? (w - 40 - (per - 1) * 6) / per : 0;
        for (int i = 0; i < n; ++i) kitBtns[(size_t) i]->setBounds (20 + (i % per) * (cw + 6), 66 + (i / per) * 40, cw, 36);
        layoutKnobs ({ 20, 170, w - 230, 96 }, 84);
        hit.setBounds (w - 190, 168, 166, 100);
        if (! chips.empty()) layoutChips (0, { 30, 304, 0, 34 }, 120);
        breed.setBounds (20, h - 112, 160, 70);
        const int kw = (w - 220 - 5 * 10) / 6;
        for (int i = 0; i < 6; ++i) kidBtns[(size_t) i]->setBounds (200 + i * (kw + 10), h - 112, kw, 70);
    }
protected:
    void paintPage (Graphics& g) override
    {
        g.setColour (Colour (0xff9c9494)); g.setFont (serif (13.0f, false, 0.25f));
        g.drawText ("BREED = SIX NEW SOUNDS.  CLICK A CHILD TO LOAD IT.", 200, getHeight() - 36, getWidth() - 220, 18, Justification::centredLeft);
        if (mode == KeysKillaProcessor::play808)
        {
            g.drawText ("SLIDES: OVERLAP TWO NOTES IN THE FL PIANO ROLL", getWidth() - 640, 312, 616, 18, Justification::centredRight);
            const int nt = proc.last808Note();
            const float tune = proc.apvts.getRawParameterValue (ID::b8Tune)->load();
            g.setColour (Colours::white); g.setFont (serif (22.0f, false, 0.2f));
            const String txt = nt >= 0 ? "NOTE  " + MidiMessage::getMidiNoteName (nt, true, true, 5) + (std::abs (tune) > 0.05f ? "  (" + String (tune, 1) + " st)" : String())
                                       : String ("NOTE  -");
            g.drawText (txt, getWidth() - 640, 280, 616, 26, Justification::centredRight);
        }
    }
    void refreshPage() override
    {
        refreshKeysButton (keysBtn, mode, title);
        for (int i = 0; i < 6; ++i) { kidBtns[(size_t) i]->setEnabled (i < (int) kids.size()); kidBtns[(size_t) i]->selected = i == selKid; kidBtns[(size_t) i]->repaint(); }
    }
    int extraSignature() override { return param (ID::playMode) + (mode == KeysKillaProcessor::play808 ? (proc.last808Note() + 2) * 10 : 0); }
private:
    static String titleOf (int m) { return m == KeysKillaProcessor::play808 ? "808" : m == KeysKillaProcessor::playSnare ? "SNARE" : "CLAP"; }
    static String subOf (int m)
    {
        return m == KeysKillaProcessor::play808 ? "TUNED 808 WITH SLIDES  -  808 KILLA DRIVE, SATURATION AND CLIPPER INSIDE"
             : m == KeysKillaProcessor::playSnare ? "TRAP SNARES  -  PICK A KIT, SHAPE IT, BREED IT" : "TRAP CLAPS  -  PICK A KIT, SHAPE IT, BREED IT";
    }
    void breedKids()
    {
        const auto& kits = KeysKillaProcessor::drumKits (mode);
        if (kits.empty()) return;
        auto& rnd = Random::getSystemRandom();
        kids.clear();
        for (int k = 0; k < 6; ++k)
        {
            const auto& other = kits[(size_t) rnd.nextInt ((int) kits.size())].values;
            std::vector<std::pair<String, float>> v;
            for (auto& [id, val] : other)
            {
                auto* prm = proc.apvts.getParameter (id);
                const float cur = prm->getValue(), oth = prm->convertTo0to1 (val);
                float x = cur + (oth - cur) * rnd.nextFloat() + (rnd.nextFloat() - 0.5f) * 0.18f;
                if (dynamic_cast<AudioParameterChoice*> (prm) != nullptr) x = rnd.nextBool() ? cur : oth;
                v.push_back ({ id, prm->convertFrom0to1 (jlimit (0.0f, 1.0f, x)) });
            }
            kids.push_back (std::move (v));
        }
        selKid = -1;
        refresh();
    }
    int mode;
    std::vector<std::unique_ptr<HotButton>> kitBtns, kidBtns;
    HotButton keysBtn { lnf }, hit { lnf }, breed { lnf };
    std::vector<std::vector<std::pair<String, float>>> kids;
    int selKid = -1;
};

// ROLLS: hi-hat roll generator - styles, density, GENERATE, preview, play with FL, drag the MIDI out
class RollsPanel : public ModulePage
{
public:
    RollsPanel (KeysKillaProcessor& p, KKLookAndFeel& l) : ModulePage (p, l, "ROLLS", "TRAP HI-HAT ROLLS  -  TRIPLETS, 1/32, 1/64, PITCH RAMPS")
    {
        addChips (ID::rlStyle, "STYLE", { "CLASSIC", "TRIPLET", "DRILL", "CRAZY" }, "Roll style");
        addChips (ID::rlBars, "LENGTH", { "1 BAR", "2 BARS", "4 BARS" }, "Pattern length");
        addKnob (ID::rlDensity, "ROLLS", "How many rolls");
        addKnob (ID::htTune, "HAT TUNE", "Hat pitch"); addKnob (ID::htDecay, "HAT DECAY", "Closed -> open");
        addKnob (ID::htTone, "HAT TONE", "Dark -> bright"); addKnob (ID::htLevel, "HAT LEVEL", "Hat level");
        gen.setButtonText ("GENERATE"); gen.setTooltip ("A new roll pattern"); gen.onClick = [this] { proc.newRolls(); refresh(); }; gen.framed = true; addAndMakeVisible (gen);
        playBtn.setTooltip ("Hear the pattern now (also with FL stopped)");
        playBtn.onClick = [this] { proc.setRollsPreview (! proc.rollsPreviewing()); refresh(); }; playBtn.framed = true; addAndMakeVisible (playBtn);
        withFl.setTooltip ("The hats play the pattern whenever FL plays (in the song tempo)");
        withFl.onClick = [this] { setParamFromUi (proc, ID::rlOn, param (ID::rlOn) ? 0.0f : 1.0f); refresh(); }; withFl.framed = true; addAndMakeVisible (withFl);
        for (auto& kit : KeysKillaProcessor::drumKits (KeysKillaProcessor::playHats))
        {
            auto b = std::make_unique<HotButton> (lnf, kit.name);
            const auto values = kit.values;
            b->onClick = [this, values] { proc.applyDrumKit (values); proc.hitPad (KeysKillaProcessor::playHats); };
            b->setTooltip ("Hat sound");
            b->framed = true; addAndMakeVisible (*b); kitBtns.push_back (std::move (b));
        }
        setupKeysButton (keysBtn, KeysKillaProcessor::playHats, "HATS");
        refresh();
        startTimerHz (24);
    }
    ~RollsPanel() override { proc.setRollsPreview (false); }
    void resized() override
    {
        const int w = getWidth();
        keysBtn.setBounds (w - 300, 14, 276, 42);
        roll = { 20, 66, w - 40, 150 };
        layoutChips (0, { 30, 244, 0, 36 }, 104);
        layoutChips (1, { 490, 244, 0, 36 }, 96);
        gen.setBounds (w - 470, 232, 200, 52); playBtn.setBounds (w - 262, 232, 116, 52); withFl.setBounds (w - 140, 232, 116, 52);
        layoutKnobs ({ 20, 312, 760, 90 }, 80);
        const int n = (int) kitBtns.size();
        for (int i = 0; i < n; ++i) kitBtns[(size_t) i]->setBounds (800 + (i % 3) * 228, 316 + (i / 3) * 42, 222, 38);
    }
    void mouseDown (const MouseEvent& e) override { dragging = roll.contains (e.getPosition()); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (! dragging || e.getDistanceFromDragStart() < 6) return;
        dragging = false;
        const auto f = proc.exportRollsMidi();
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
protected:
    void paintPage (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.setColour (Colour (0xff120d0d)); g.fillRoundedRectangle (roll.toFloat(), 6);
        g.setColour (Colour (0xff3a2e2e)); g.drawRoundedRectangle (roll.toFloat(), 6, 1.2f);
        const int bars = param (ID::rlBars) == 0 ? 1 : param (ID::rlBars) == 1 ? 2 : 4;
        const double len = bars * 4.0;
        auto r = roll.toFloat().reduced (10, 12);
        for (int b = 0; b <= bars * 4; ++b)
        {
            const float x = r.getX() + r.getWidth() * (float) (b / len);
            g.setColour (Colour (b % 4 == 0 ? 0xff5a4646 : 0xff2a2020)); g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
        }
        for (auto& h : pattern)
        {
            const float x = r.getX() + r.getWidth() * (float) (h.beat / len);
            const float bh = r.getHeight() * (0.25f + 0.65f * h.vel);
            const float y = r.getBottom() - bh - (float) h.semi * 3.0f;
            g.setColour (h.semi != 0 ? Colour (0xffff7a50) : s.accent.brighter (0.2f));
            g.fillRect (x, y, std::max (1.5f, r.getWidth() * (float) (h.len / len)), bh);
        }
        if (const float pb = proc.rollsBeat(); pb >= 0)
        {
            g.setColour (Colours::white.withAlpha (0.7f));
            g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) (pb / len)), r.getY(), r.getBottom());
        }
        g.setColour (Colour (0xff9c9494)); g.setFont (serif (13.0f, false, 0.25f));
        g.drawText ("DRAG THE PATTERN INTO FL STUDIO = MIDI CLIP   (" + String ((int) pattern.size()) + " HITS)", roll.withY (roll.getBottom() + 4).withHeight (18), Justification::centredRight);
        g.drawText ("HAT SOUND", 802, 296, 300, 18, Justification::centredLeft);
    }
    void refreshPage() override
    {
        pattern = proc.rollPattern();
        playBtn.setButtonText (proc.rollsPreviewing() ? "STOP" : "PLAY"); playBtn.selected = proc.rollsPreviewing(); playBtn.repaint();
        withFl.setButtonText ("WITH FL"); withFl.selected = param (ID::rlOn) != 0; withFl.repaint();
        refreshKeysButton (keysBtn, KeysKillaProcessor::playHats, "HATS");
    }
    int extraSignature() override { return param (ID::rlSeed) * 3 + param (ID::rlOn) + param (ID::playMode) * 7 + (proc.rollsPreviewing() ? 11 : 0) + (int) (proc.apvts.getRawParameterValue (ID::rlDensity)->load() * 100); }
    void tick() override { ModulePage::tick(); if (proc.rollsBeat() >= 0 || lastBeatShown >= 0) { lastBeatShown = proc.rollsBeat(); repaint (roll); } }
private:
    Rectangle<int> roll;
    std::vector<kk::RollHit> pattern;
    HotButton gen { lnf }, playBtn { lnf }, withFl { lnf }, keysBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> kitBtns;
    bool dragging = false;
    float lastBeatShown = -1;
};

// EFFECTOR = Effector Killa: channels x presets, 5 macros, blend
class EffectorPanel : public ModulePage
{
public:
    EffectorPanel (KeysKillaProcessor& p, KKLookAndFeel& l) : ModulePage (p, l, "EFFECTOR", "EFFECTOR KILLA ENGINE  -  14 EFFECTS IN 8 SLOTS, READY-MADE CHAINS ON EVERYTHING KEYS KILLA PLAYS")
    {
        onBtn.onClick = [this] { setParamFromUi (proc, ID::efxOn, param (ID::efxOn) ? 0.0f : 1.0f); refresh(); };
        onBtn.setTooltip ("EFFECTOR on / off"); onBtn.framed = true; addAndMakeVisible (onBtn);
        const auto& chs = proc.effectorChannels();
        for (int c = 0; c < (int) chs.size(); ++c)
        {
            auto b = std::make_unique<HotButton> (lnf, chs[(size_t) c].name.toUpperCase());
            b->setTooltip (chs[(size_t) c].description);
            b->onClick = [this, c] { viewCh = c; refresh(); };
            b->framed = true; addAndMakeVisible (*b); chBtns.push_back (std::move (b));
        }
        const auto names = proc.effectorMacroNames();
        for (int m = 0; m < 5; ++m) addKnob (ID::efxMacro (m), names[m], "Macro " + names[m]);
        addKnob (ID::efxBlend, "BLEND", "Dry <-> EFFECTOR");
        const int cur = param (ID::efxPreset);
        if (cur < (int) proc.effectorPresets().size()) viewCh = proc.effectorPresets()[(size_t) cur].channel;
        refresh();
        startTimerHz (10);
    }
    void resized() override
    {
        const int w = getWidth(), h = getHeight();
        onBtn.setBounds (w - 210, 14, 186, 42);
        const int n = (int) chBtns.size(), per = (n + 1) / 2, cw = per > 0 ? (w - 40 - (per - 1) * 6) / per : 0;
        for (int c = 0; c < n; ++c) chBtns[(size_t) c]->setBounds (20 + (c % per) * (cw + 6), 64 + (c / per) * 36, cw, 34);
        layoutKnobs ({ 20, h - 116, w - 40, 86 }, 80);
    }
    void mouseUp (const MouseEvent& e) override
    {
        const auto list = presetsOfView();
        for (int k = 0; k < (int) list.size(); ++k)
            if (card (k).contains (e.getPosition()))
            {
                if (list[(size_t) k] == param (ID::efxPreset) && param (ID::efxOn)) setParamFromUi (proc, ID::efxOn, 0.0f);
                else { proc.setEffectorPreset (list[(size_t) k]); setParamFromUi (proc, ID::efxOn, 1.0f); }
                refresh(); return;
            }
    }
protected:
    void paintPage (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto list = presetsOfView();
        const auto& all = proc.effectorPresets();
        for (int k = 0; k < (int) list.size(); ++k)
        {
            const auto r = card (k).toFloat();
            const bool sel = list[(size_t) k] == param (ID::efxPreset);
            g.setColour (Colour (sel ? 0xff2a0d0d : 0xff141010)); g.fillRoundedRectangle (r, 8);
            if (sel) drawGlowFrame (g, r, param (ID::efxOn) ? s.accent : s.accent.withAlpha (0.45f), 8);
            else { g.setColour (Colour (0xff3a2e2e)); g.drawRoundedRectangle (r, 8, 1.2f); }
            const auto& e = all[(size_t) list[(size_t) k]];
            g.setColour (sel ? Colours::white : Colour (0xffe0d8d6)); g.setFont (serif (19.0f, false, 0.1f));
            g.drawText (e.name, r.reduced (14, 8).withHeight (28), Justification::centredLeft);
            g.setColour (Colour (0xff9c9494)); g.setFont (serif (13.0f, false, 0.05f));
            g.drawFittedText (e.desc, r.reduced (14, 8).withTrimmedTop (30).toNearestInt(), Justification::topLeft, 2);
        }
    }
    void refreshPage() override
    {
        onBtn.setButtonText (param (ID::efxOn) ? "EFFECTOR  ON" : "EFFECTOR  OFF"); onBtn.selected = param (ID::efxOn) != 0;
        for (int c = 0; c < (int) chBtns.size(); ++c) { chBtns[(size_t) c]->selected = c == viewCh; chBtns[(size_t) c]->repaint(); }
    }
    int extraSignature() override { return param (ID::efxPreset) * 2 + param (ID::efxOn) + viewCh * 1000; }
private:
    std::vector<int> presetsOfView() const
    {
        std::vector<int> v; const auto& all = proc.effectorPresets();
        for (int i = 0; i < (int) all.size(); ++i) if (all[(size_t) i].channel == viewCh) v.push_back (i);
        if (v.size() > 8) v.resize (8);
        return v;
    }
    Rectangle<int> card (int k) const
    {
        const int w = getWidth(), cw = (w - 40 - 3 * 14) / 4, chh = (getHeight() - 140 - 140) / 2;
        return { 20 + (k % 4) * (cw + 14), 142 + (k / 4) * (chh + 10), cw, chh };
    }
    HotButton onBtn { lnf, "EFFECTOR" };
    std::vector<std::unique_ptr<HotButton>> chBtns;
    int viewCh = 0;
};

// DIGGA: drop a sample, it is chopped, the pads / keys play the slices
class DiggaPanel : public ModulePage, public FileDragAndDropTarget
{
public:
    DiggaPanel (KeysKillaProcessor& p, KKLookAndFeel& l) : ModulePage (p, l, "DIGGA", "DIGGA KILLA  -  DROP A SAMPLE, IT GETS CHOPPED, PLAY THE CHOPS ON THE KEYS")
    {
        loadBtn.setButtonText ("LOAD SAMPLE"); loadBtn.setTooltip ("WAV, AIFF, FLAC, MP3, OGG - or drag a file onto this page");
        loadBtn.onClick = [this] { browse(); }; loadBtn.framed = true; addAndMakeVisible (loadBtn);
        addChips (ID::dgMode, "MODE", { "CHOP", "KEYS" }, "CHOP: every key plays a slice (C5 = slice 1).  KEYS: the whole sample plays chromatically.");
        addChips (ID::dgSlices, "SLICES", { "8", "16" }, "Number of chops");
        addChips (ID::dgChop, "CHOP BY", { "HITS", "EVEN" }, "HITS: cut on the transients.  EVEN: equal pieces.");
        addKnob (ID::dgPitch, "PITCH", "Pitch in semitones"); addKnob (ID::dgLevel, "LEVEL", "Level");
        revBtn.setButtonText ("REVERSE"); revBtn.setTooltip ("Play the chops backwards");
        revBtn.onClick = [this] { setParamFromUi (proc, ID::dgRev, param (ID::dgRev) ? 0.0f : 1.0f); refresh(); }; revBtn.framed = true; addAndMakeVisible (revBtn);
        for (int i = 0; i < 16; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, String (i + 1));
            b->onClick = [this, i] { proc.hitPad (KeysKillaProcessor::playDigga, 60 + i); };
            b->framed = true; addAndMakeVisible (*b); pads.push_back (std::move (b));
        }
        setupKeysButton (keysBtn, KeysKillaProcessor::playDigga, "DIGGA");
        refresh();
        startTimerHz (20);
    }
    bool isInterestedInFileDrag (const StringArray& files) override
    {
        for (auto& f : files) if (File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true;
        return false;
    }
    void filesDropped (const StringArray& files, int, int) override
    {
        for (auto& f : files) if (File (f).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) { load (File (f)); break; }
    }
    void resized() override
    {
        const int w = getWidth();
        keysBtn.setBounds (w - 300, 14, 276, 42);
        wave = { 20, 66, w - 40, 160 };
        loadBtn.setBounds (20, 240, 180, 40);
        layoutChips (0, { 240, 254, 0, 34 }, 80);
        layoutChips (1, { 430, 254, 0, 34 }, 60);
        layoutChips (2, { 580, 254, 0, 34 }, 80);
        revBtn.setBounds (770, 250, 120, 40);
        layoutKnobs ({ 910, 232, 260, 80 }, 74);
        const int count = sliceCount(), pw = (w - 40 - 7 * 8) / 8;
        for (int i = 0; i < 16; ++i)
        {
            pads[(size_t) i]->setVisible (i < count);
            pads[(size_t) i]->setBounds (20 + (i % 8) * (pw + 8), 330 + (i / 8) * 64, pw, 56);
        }
    }
protected:
    void paintPage (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.setColour (Colour (0xff120d0d)); g.fillRoundedRectangle (wave.toFloat(), 6);
        g.setColour (Colour (0xff3a2e2e)); g.drawRoundedRectangle (wave.toFloat(), 6, 1.2f);
        auto smp = proc.diggaSample();
        auto r = wave.toFloat().reduced (8, 10);
        if (smp == nullptr || smp->peaks.empty())
        {
            g.setColour (Colour (0xff9c9494)); g.setFont (serif (18.0f, false, 0.2f));
            g.drawText ("DRAG A SAMPLE HERE  (OR LOAD SAMPLE)", wave, Justification::centred);
            return;
        }
        const int cols = (int) smp->peaks.size();
        g.setColour (s.accent.withAlpha (0.85f));
        for (int c = 0; c < cols; ++c)
        {
            const float x = r.getX() + r.getWidth() * (float) c / (float) cols, a = smp->peaks[(size_t) c] * r.getHeight() * 0.48f;
            g.drawVerticalLine ((int) x, r.getCentreY() - a, r.getCentreY() + a + 1);
        }
        const int total = std::max (1, smp->audio.getNumSamples());
        g.setFont (serif (12.0f, false, 0.1f));
        for (int k = 0; k < (int) smp->starts.size(); ++k)
        {
            const float x = r.getX() + r.getWidth() * (float) smp->starts[(size_t) k] / (float) total;
            g.setColour (Colours::white.withAlpha (0.55f)); g.drawVerticalLine ((int) x, r.getY(), r.getBottom());
            g.setColour (Colours::white); g.drawText (String (k + 1), (int) x + 3, (int) r.getY(), 24, 14, Justification::centredLeft);
        }
        if (const float ph = proc.diggaPlayhead(); ph >= 0)
        {
            g.setColour (Colours::white.withAlpha (0.2f));
            g.fillRect (r.getX() + r.getWidth() * ph, r.getY(), 3.0f, r.getHeight());
        }
        g.setColour (Colour (0xffb9b0ac)); g.setFont (serif (13.0f, false, 0.2f));
        g.drawText (smp->name.toUpperCase(), wave.reduced (12, 4), Justification::bottomLeft);
        g.drawText ("PADS = C5, C#5, D5 ...  (CHOP MODE)", 20, getHeight() - 26, 600, 18, Justification::centredLeft);
    }
    void refreshPage() override
    {
        revBtn.selected = param (ID::dgRev) != 0; revBtn.repaint();
        refreshKeysButton (keysBtn, KeysKillaProcessor::playDigga, "DIGGA");
        resized();
    }
    int extraSignature() override
    {
        auto smp = proc.diggaSample();
        return param (ID::dgRev) + param (ID::playMode) * 3 + (smp ? (int) smp->starts.size() * 17 + smp->name.hashCode() : 0);
    }
    void tick() override { ModulePage::tick(); repaint (wave); }
private:
    int sliceCount() const { auto smp = proc.diggaSample(); return smp ? std::min (16, (int) smp->starts.size()) : (param (ID::dgSlices) == 0 ? 8 : 16); }
    void browse()
    {
        chooser = std::make_unique<FileChooser> ("Load a sample", File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [safe = Component::SafePointer<DiggaPanel> (this)] (const FileChooser& fc)
                              { if (safe != nullptr && fc.getResult().existsAsFile()) safe->load (fc.getResult()); });
    }
    void load (const File& f)
    {
        if (! proc.loadDiggaFile (f)) AlertWindow::showMessageBoxAsync (MessageBoxIconType::WarningIcon, "DIGGA", "This file could not be read.");
        refresh();
    }
    Rectangle<int> wave;
    HotButton loadBtn { lnf }, revBtn { lnf }, keysBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> pads;
    std::unique_ptr<FileChooser> chooser;
};

// left switch: BREED LAB / FAMILY TREE / PARAMS (-1 = a module of the bottom row is open)
class LabSwitch : public Component, public SettableTooltipClient
{
public:
    explicit LabSwitch (KKLookAndFeel& l) : lnf (l)
    {
        setTooltip ("BREED LAB: 2 parents -> 6 sounds.  FAMILY TREE: up to 4 sounds -> new sounds or melody loops.  PARAMS: every parameter of the sound.");
    }
    std::function<void (int)> onSwitch;
    int sel = 0;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        static const char* names[] { "BREED\nLAB", "FAMILY\nTREE", "PARAMS" };
        for (int k = 0; k < 3; ++k)
        {
            auto r = part (k);
            const bool on = k == sel;
            g.setColour (Colour (on ? 0xee2a0d0d : 0xcc0e0a0a)); g.fillRoundedRectangle (r, 6);
            if (on) drawGlowFrame (g, r, s.accent, 6);
            else { g.setColour (Colour (0xff4a3c3c)); g.drawRoundedRectangle (r, 6, 1.2f); }
            g.setColour (on ? Colours::white : Colour (0xffb9b0ac)); g.setFont (serif (15.0f, false, 0.2f));
            g.drawFittedText (names[k], r.toNearestInt(), Justification::centred, 2);
        }
    }
    void mouseUp (const MouseEvent& e) override
    {
        for (int k = 0; k < 3; ++k)
            if (part (k).contains (e.position) && onSwitch) { onSwitch (k); return; }
    }
private:
    Rectangle<float> part (int k) const { const float h = ((float) getHeight() - 14.0f) / 3.0f; return { 3.0f, 3.0f + (float) k * (h + 4.0f), (float) getWidth() - 6.0f, h }; }
    KKLookAndFeel& lnf;
};

//==============================================================================
class MainPage : public Component, private Timer
{
public:
    explicit MainPage (KeysKillaProcessor& p)
        : proc (p), parentA (p, lnf, 0), parentB (p, lnf, 1), breedBtn (lnf), wildRail (p, lnf), soundList (p, lnf),
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
        treeBtn.onClick = [this] { openTab (tabTree); }; treeBtn.setTooltip ("FAMILY TREE: breed up to 4 sounds into new sounds or melody loops");
        labSwitch.onSwitch = [this] (int k)
        {
            if (k == 0) { hidePanels(); openTabIndex = -1; updateTabs(); }
            else { const int t = k == 1 ? tabTree : tabParams; if (! (openTabIndex == t && isPanelVisible())) openTab (t); }
        };
        addAndMakeVisible (labSwitch);
        undoBtn.onClick = [this] { proc.undo(); refreshState(); }; undoBtn.setTooltip ("UNDO the last change of the sound");
        addAndMakeVisible (treeBtn); addAndMakeVisible (undoBtn);

        // ---- WILD rail + FUTURE / ALIVE / TIME
        addChildComponent (wildRail);   // v0.14: the WILD rail is gone from the main page (BREED keeps its last setting)
        soundList.onLoaded = [this] { refreshState(); };
        addAndMakeVisible (soundList);
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
        static const char* tabNames[] { "808", "SNARE", "CLAP", "ROLLS", "HALF", "EFFECTOR", "DIGGA" };
        static const char* tabTips[] { "808: tuned 808 with slides + the 808 Killa drive / clipper",
                                       "SNARE: trap snares - kits, knobs, BREED", "CLAP: trap claps - kits, knobs, BREED",
                                       "ROLLS: hi-hat roll generator - triplets, 1/32, 1/64, pitch ramps, drag the MIDI into FL",
                                       "HALF: Voodoo Killa - halftime, tape stop, glitch, backmask on everything KEYS KILLA plays",
                                       "EFFECTOR: Effector Killa - ready-made effect chains on everything KEYS KILLA plays",
                                       "DIGGA: Digga Killa - drop a sample, chop it, play the chops on the keys" };
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
        applyKeyMode();
        focusGrabber.page = this;
        addMouseListener (&focusGrabber, true);   // a click anywhere in the plugin gives it the PC keyboard
        refreshState();
        startTimerHz (30);
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

    int preferredScale() const { return jlimit (50, 100, settings->getIntValue ("labScale", 70)); }

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
        return false;
    }
    static bool typingText() { return dynamic_cast<TextEditor*> (Component::getCurrentlyFocusedComponent()) != nullptr; }

    // tests / screenshots: 0 main, 1..8 advanced tab, 9 browser, 10 movement, 11 808
    void showView (int v)
    {
        if (v >= 1 && v <= 8) { ensureAdvanced(); advanced->showTab (v - 1); advanced->setVisible (true); advanced->toFront (false); }
        if (v == 9) openTab (tabBrowser);
        if (v == 10) openTab (tabParams);
        if (v == 11) openTab (tabHalf);
        if (v == 15) openTab (tab808);
        if (v == 16) { proc.newRolls(); openTab (tabRolls); }
        if (v == 17) openTab (tabEffector);
        if (v == 18) openTab (tabDigga);
        if (v == 19) openTab (tabSnare);
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
        soundList.setBounds (R (1458, 104, 1614, 398));
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
        if (advanced) advanced->setBounds (R (150, 8, 1662, 612));   // the left switch stays visible
        if (browser) browser->setBounds (panelArea);
        if (treePanel) treePanel->setBounds (R (150, 96, 1662, 612));
        for (auto& m : modules) if (m) m->setBounds (R (150, 96, 1662, 612));
        labSwitch.setBounds (R (16, 108, 138, 372));
    }

private:
    enum { tab808, tabSnare, tabClap, tabRolls, tabHalf, tabEffector, tabDigga, numTabs, tabBrowser = 99, tabSettings = 100, tabTree = 101, tabParams = 102 };
    Component* module (int t)
    {
        auto& m = modules[(size_t) t];
        if (! m)
        {
            switch (t)
            {
                case tab808:      m = std::make_unique<DrumPanel> (proc, lnf, KeysKillaProcessor::play808); break;
                case tabSnare:    m = std::make_unique<DrumPanel> (proc, lnf, KeysKillaProcessor::playSnare); break;
                case tabClap:     m = std::make_unique<DrumPanel> (proc, lnf, KeysKillaProcessor::playClap); break;
                case tabRolls:    m = std::make_unique<RollsPanel> (proc, lnf); break;
                case tabHalf:     m = std::make_unique<HalfPanel> (proc, lnf); break;
                case tabEffector: m = std::make_unique<EffectorPanel> (proc, lnf); break;
                default:          m = std::make_unique<DiggaPanel> (proc, lnf); break;
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
                    if (t >= 0 && t < numTabs) module (t)->setVisible (true);
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
        const int sw = openTabIndex < 0 ? 0 : treeOn ? 1 : openTabIndex == tabParams ? 2 : -1;
        if (labSwitch.sel != sw) { labSwitch.sel = sw; labSwitch.repaint(); }
        if (! treeOn && proc.loopPlaying()) proc.stopLoop();   // loops belong to the FAMILY TREE
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
        }
        wildRail.repaint();
        updateTabs();
    }

    void timerCallback() override
    {
        proc.moduleHousekeeping();
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
    SoundList soundList;
    std::vector<std::unique_ptr<ImageKnob>> macros, sideKnobs;
    std::vector<std::unique_ptr<MacroCaption>> captions;
    WheelSlider pitchWheel, modWheel;
    MeterOverlay meter;
    KKKeyboard keyboard;
    std::unique_ptr<AdvancedPage> advanced;
    std::unique_ptr<PresetBrowser> browser;
    std::array<std::unique_ptr<Component>, 7> modules;
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
