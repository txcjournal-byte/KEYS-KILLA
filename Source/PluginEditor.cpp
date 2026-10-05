#include "PluginEditor.h"
#include <numeric>
#include <map>

using namespace juce;

#include "UiCommon.h"
#include "BinaryData.h"
#include "Theme.h"
#include "ModernSkin.h"
#include "LivingCell.h"

//==============================================================================
void KKLookAndFeel::setSkin (const Skin& s)
{
    skin = &s;
    setColour (PopupMenu::backgroundColourId, TC (0xff15132e));
    setColour (PopupMenu::textColourId, s.text);
    setColour (PopupMenu::highlightedBackgroundColourId, s.accent.withAlpha (0.8f));
    setColour (PopupMenu::highlightedTextColourId, TC (0xffffffff));
    setColour (PopupMenu::headerTextColourId, s.accent);
    setColour (Label::textColourId, s.text);
    setColour (TooltipWindow::backgroundColourId, TC (0xff1c1a3a));
    setColour (TooltipWindow::textColourId, s.text);
    setColour (TooltipWindow::outlineColourId, s.accent);
    setColour (ComboBox::backgroundColourId, TC (0xff1c1a3a));
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
    setColour (TextEditor::backgroundColourId, TC (0xff131130));
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

void KKLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float, float, Slider& sl)
{
    // v0.44: no knobs - a living cell
    kk::cell::draw (g, { (float) x, (float) y, (float) w, (float) h }, pos, sl.getMinimum() < 0 && sl.getMaximum() > 0, skin->accent, TC (0xff9b4dff), {}, sl.isMouseOverOrDragging());
}

void KKLookAndFeel::drawLinearSlider (Graphics& g, int x, int y, int w, int h, float, float, float,
                                      Slider::SliderStyle style, Slider& sl)
{
    // v0.44: no sliders - a stream of light, the glow gathers where the value is
    const bool vert = style == Slider::LinearVertical || style == Slider::LinearBarVertical;
    kk::cell::stream (g, { (float) x, (float) y, (float) w, (float) h }, (float) sl.valueToProportionOfLength (sl.getValue()), vert, skin->accent, sl.isMouseOverOrDragging());
}

void KKLookAndFeel::drawButtonBackground (Graphics& g, Button& b, const Colour&, bool over, bool down)
{
    const auto& s = *skin;
    auto r = b.getLocalBounds().toFloat().reduced (1.5f);
    const bool on = b.getToggleState();
    g.setColour (TC (0xff12102a));
    g.fillRoundedRectangle (r, 4);
    if (on)
    {
        g.setColour (s.accent.withAlpha (0.18f)); g.fillRoundedRectangle (r, 4);
        g.setColour (s.accent.withAlpha (0.35f)); g.drawRoundedRectangle (r.expanded (1.5f), 5, 3);
    }
    g.setColour (on ? s.accent : (over ? s.text.withAlpha (0.6f) : s.panelEdge));
    g.drawRoundedRectangle (r, 4, on ? 2.0f : 1.2f);
    if (down) { g.setColour (TC (0xff000000).withAlpha (0.15f)); g.fillRoundedRectangle (r, 4); }
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
    if (auto* sl = dynamic_cast<Slider*> (l.getParentComponent()))   // knob value boxes scale with their box
        return sl->getSliderStyle() == Slider::LinearBar ? serif (13.0f, true, 0.12f) : serif (jmax (11.0f, (float) l.getHeight() * 0.72f), false, 0.2f);
    return l.getFont();
}
Font KKLookAndFeel::getPopupMenuFont() { return serif (25.0f, false, 0.05f); }   // menus scale with the 60 % window



//==============================================================================
// BREED LAB skin: one bitmap made from the design (tools/make_breed_assets.py), live parts drawn on top.
struct LabImages { Image bg, page, white, black, icons; };

// Images live only while an editor is open: on Windows they are native images and must be released
// before the host shuts its graphics down (static images froze FL Studio on exit).
struct LabImageCache
{
    LabImages imgs;
    kk::modern::Backdrop backdrop;
    int builtFor = -1;
    LabImageCache() { rebuild(); imgs.icons = ImageFileFormat::loadFrom (BinaryData::lab_icons_png, (size_t) BinaryData::lab_icons_pngSize); }
    ~LabImageCache() { if (kk::modern::activeBackdrop() == &backdrop) kk::modern::activeBackdrop() = nullptr; }
    void rebuild()   // v0.34: GLASS / NIGHT - every picture is drawn again for the theme
    {
        builtFor = kk::themeIndex();
        backdrop = kk::modern::makeBackdrop();
        kk::modern::activeBackdrop() = &backdrop;
        imgs.bg = kk::modern::makeBackground();
        imgs.page = kk::modern::makePageBackdrop();
        imgs.white = kk::modern::makeWhiteKey();
        imgs.black = kk::modern::makeBlackKey();
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

// v0.34: full pages sit on the same see-through circuit board, under one big pane of milky glass
static void pageBackdrop (Graphics& g, Component& c, float glass = 0.6f)
{
    Point<int> o;
    if (auto* ed = c.findParentComponentOfClass<AudioProcessorEditor>())
        if (auto* mp = ed->getChildComponent (0)) o = mp->getLocalPoint (&c, Point<int>());
    // frosted: the board behind a page is blurred, so nothing behind it competes with the page
    if (auto* bd = kk::modern::activeBackdrop(); bd != nullptr && bd->soft.isValid())
    {
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImage (bd->soft, Rectangle<float> ((float) -o.x, (float) -o.y, 1672.0f, 941.0f), RectanglePlacement::stretchToFit);
    }
    else if (const auto& img = labImages().page; img.isValid()) g.drawImageAt (img, -o.x, -o.y);
    else g.fillAll (kk::theme().baseBottom);
    g.setColour (kk::theme().glass.withMultipliedAlpha (glass)); g.fillAll();
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
    g.setColour (TC (0xffaaa4cf)); g.setFont (f1); g.drawText ("by", Rectangle<float> (x, r.getY(), w1, r.getHeight()), Justification::centredLeft);
    g.setGradientFill (ColourGradient (TC (0xffff8a3d), x + w1, 0, TC (0xffff3fd2), x + tw, 0, false));
    g.setFont (f2.withExtraKerningFactor (0.06f)); g.drawText ("TrapVST", Rectangle<float> (x + w1, r.getY(), w2 + 4, r.getHeight()), Justification::centredLeft);
}
static void drawGlowFrame (Graphics& g, Rectangle<float> r, Colour accent, float corner = 5.0f)
{
    g.setColour (accent.withAlpha (0.14f)); g.drawRoundedRectangle (r.expanded (1.5f), corner + 1.5f, 3.0f);   // v0.34: a quiet frame
    g.setColour (accent); g.drawRoundedRectangle (r, corner, 1.4f);
}

static int iconOfCategory (int cat)
{
    //                        Piano Keys Bells Plucks Mallets Guitar Strings Brass Choir Wind Lead Pads Synth Bass 808 Texture Arp FX Organ Chip World Drums Game Cine
    static const int icon[] { 1,    1,   0,    2,     0,      8,     8,      6,    4,    3,   6,   5,   6,    7,   7,  5,      2,  9, 1,    6,   3,    7,    9,   9 };
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
    }
    void mouseEnter (const MouseEvent& e) override { Slider::mouseEnter (e); repaint(); }
    void mouseExit (const MouseEvent& e) override { Slider::mouseExit (e); repaint(); }
    void place (Point<int> designCentre, int capRadius, int arcRadius)
    {
        centre = designCentre; capR = capRadius; arcR = arcRadius;
        const int half = arcR + 12;
        setBounds (centre.x - half, centre.y - half, half * 2, half * 2);
    }
    void paint (Graphics& g) override
    {
        // v0.44: no knob - a living cell covers the old metal cap
        const float pos = (float) valueToProportionOfLength (getValue());
        kk::cell::draw (g, getLocalBounds().toFloat().reduced (2), pos, false, TC (0xffff2f6d), TC (0xffff8a3d), {}, isMouseOverOrDragging(), true, 0.0f, (float) (centre.x % 7));
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
    bool hero = false;   // a call-to-action button: always a glowing gradient (EDIT)
    std::function<void()> onRightClick;

    void paintButton (Graphics& g, bool over, bool down) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (2);
        const float corner = round ? r.getHeight() * 0.5f : 5.0f;
        if (framed && ! selected)
        {
            const auto& th = kk::theme();   // v0.34: a small pane of the same milky glass
            g.setColour (th.glass); g.fillRoundedRectangle (r, corner);
            g.setColour (th.glassEdge); g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
            g.setColour (th.glassHi); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + corner, r.getRight() - corner);
        }
        const bool tinted = ! tint.isTransparent();
        if (hero && ! selected)
        {
            // v0.34: amber outline on glass (calm call-to-action)
            const auto& th = kk::theme();
            g.setColour (th.accent.withAlpha (down ? 0.28f : over ? 0.18f : 0.08f)); g.fillRoundedRectangle (r, corner);
            g.setColour (th.accent); g.drawRoundedRectangle (r.reduced (0.6f), corner, 1.6f);
        }
        else if (selected)
        {
            drawGlowFrame (g, r, tinted ? tint : s.accent, corner);
            g.setGradientFill (tinted ? ColourGradient (tint.withAlpha (0.45f), r.getX(), r.getY(), tint.withAlpha (0.12f), r.getRight(), r.getBottom(), false)
                                      : ColourGradient (s.accent.withAlpha (0.16f), r.getX(), r.getY(), s.accent.withAlpha (0.06f), r.getRight(), r.getBottom(), false));
            g.fillRoundedRectangle (r, corner);
        }
        else if (tinted && framed && ! hero)
        {
            const auto bar = r.reduced (r.getWidth() * 0.30f, 0).withTop (r.getBottom() - 2.0f).translated (0, -3.0f);
            g.setColour (kk::theme().dim.withAlpha (0.35f)); g.fillRoundedRectangle (bar, 1.0f);
        }
        if (over && ! selected) { g.setColour (s.accent.withAlpha (down ? 0.25f : 0.12f)); g.fillRoundedRectangle (r, corner); }
        if (const auto text = getButtonText(); text.isNotEmpty())
        {
            g.setColour (hero || selected ? kk::accentText() : kk::theme().text);
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
        g.setColour (Colour (0xff07080d)); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8);   // covers the old wheel
        kk::cell::stream (g, getLocalBounds().toFloat().reduced (4, 8), (float) valueToProportionOfLength (getValue()), true, s.accent, isMouseOverOrDragging());
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
                const Colour c = (warn && i >= segs - 3) ? Colours::orange : TC (0xffff2f6d).interpolatedWith (TC (0xffffa53d), (float) i / (float) segs);
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
        g.setColour (TC (0xff0d0b20));
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
        g.setColour (TC (0xff000000).withAlpha (0.45f)); g.drawRoundedRectangle (k, 2, 1);
        if (! inScale (note)) { g.setColour (TC (0xff000000).withAlpha (0.3f)); g.fillRect (k); }
        if (on)
        {
            g.setGradientFill (ColourGradient (s.accent.withAlpha (0.95f), 0, k.getY(), s.accent.withAlpha (0.55f), 0, k.getBottom(), false));
            g.fillRect (k.reduced (1.5f, 0));
            g.setColour (TC (0xffffffff).withAlpha (0.45f)); g.fillRect (k.reduced (k.getWidth() * 0.35f, 2).withHeight (k.getHeight() * 0.5f));
        }
        else if (isOver) { g.setColour (s.accent.withAlpha (0.18f)); g.fillRect (k); }
        if (isRoot (note)) { g.setColour (s.accent); g.fillEllipse (Rectangle<float> (6, 6).withCentre ({ k.getCentreX(), k.getBottom() - 12 })); }
    }
    void drawBlackNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour) override
    {
        const auto& s = *lnf.skin;
        const bool on = isDown || proc.playing[(size_t) note].load();
        auto k = a.withTrimmedTop (-2);
        g.setColour (TC (0xff000000).withAlpha (0.5f)); g.fillRoundedRectangle (k.translated (1.5f, 2), 2);
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
        g.setColour (TC (0xffe6e1e1));
        g.setFont (serif ((float) getHeight() * 0.66f, false, 0.08f));
        g.drawText (text, getLocalBounds(), Justification::centred);
        ignoreUnused (lnf);
    }
private:
    KKLookAndFeel& lnf;
};

#include "AdvancedPage.h"
#include "SoundEditor.h"
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
        g.fillAll (TC (0xf4080606));
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
        g.setColour (lit ? TC (0xffffffff) : (n.low ? col.withAlpha (0.45f) : col));
        g.fillRect (x, y, w, nh);
    }
    if (playBeat >= 0)
    {
        g.setColour (TC (0xffffffff).withAlpha (0.6f));
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
// PARENT A / B card (v0.35): the sound's name and its real waveform, a play button - or an empty slot to fill.
// Drop your own WAV on it (or pick a sound from the bank).
class ParentCard : public Component, public SettableTooltipClient, public FileDragAndDropTarget
{
public:
    ParentCard (KeysKillaProcessor& p, KKLookAndFeel& l, int s) : proc (p), lnf (l), slot (s)
    {
        setTooltip ("PARENT " + String (s == 0 ? "A" : "B") + ": click = choose a sound from the bank, or DROP YOUR OWN WAV here. Play button = hear it. Dice = random.");
    }
    std::function<void()> onClick, onChanged;
    bool isInterestedInFileDrag (const StringArray& f) override { for (auto& x : f) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true; return false; }
    void fileDragEnter (const StringArray&, int, int) override { dropHot = true; repaint(); }
    void fileDragExit (const StringArray&) override { dropHot = false; repaint(); }
    void filesDropped (const StringArray& files, int, int) override
    {
        dropHot = false;
        for (auto& x : files) if (proc.labDropFile (slot, File (x))) break;
        repaint();
        if (onChanged) onChanged();
    }
    Rectangle<float> playRect() const { return { 24.0f, 174.0f, 34.0f, 34.0f }; }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto& th = kk::theme();
        auto r = getLocalBounds().toFloat().reduced (10, 14);
        const auto wv = Rectangle<float> (r.getX() + 8, 44, r.getWidth() - 16, 120);
        const bool wav = proc.labWav[(size_t) slot];
        const bool filled = proc.labSlotFilled (slot);
        if (dropHot)
        {
            g.setColour (th.accent.withAlpha (0.12f)); g.fillRoundedRectangle (r, 10);
            g.setColour (th.accent); g.drawRoundedRectangle (r, 10, 2.0f);
            g.setFont (serif (16.0f, true, 0.2f)); g.drawText ("DROP = YOUR SOUND AS PARENT", r.toNearestInt(), Justification::centred);
            return;
        }
        if (! filled)
        {
            Path frame; frame.addRoundedRectangle (wv.expanded (0, 18), 10.0f);
            Path dashed; const float dl[] { 7.0f, 6.0f };
            PathStrokeType (1.4f).createDashedStroke (dashed, frame, dl, 2);
            g.setColour (s.accent.withAlpha (over ? 0.9f : 0.55f)); g.fillPath (dashed);
            if (over) { g.setColour (s.accent.withAlpha (0.07f)); g.fillRoundedRectangle (wv.expanded (0, 18), 10.0f); }
            const auto c = wv.getCentre().translated (0, -16);
            g.setColour (s.accent.withAlpha (over ? 1.0f : 0.8f));
            g.fillRoundedRectangle (Rectangle<float> (34, 3.2f).withCentre (c), 1.6f);
            g.fillRoundedRectangle (Rectangle<float> (3.2f, 34).withCentre (c), 1.6f);
            g.setColour (th.text.withAlpha (0.85f)); g.setFont (serif (14.0f, true, 0.18f));
            g.drawFittedText ("CLICK = CHOOSE A SOUND", wv.withTrimmedTop (wv.getHeight() * 0.55f).withHeight (20).reduced (8, 0).toNearestInt(), Justification::centred, 1, 0.8f);
            g.setColour (th.dim); g.setFont (serif (12.5f, true, 0.18f));
            g.drawFittedText ("or drop your own WAV here", wv.withTrimmedTop (wv.getHeight() * 0.55f + 22).withHeight (18).reduced (8, 0).toNearestInt(), Justification::centred, 1, 0.8f);
            return;
        }
        if (over) { g.setColour (s.accent.withAlpha (0.07f)); g.fillRoundedRectangle (r, 8); }
        // waveform: the real sound (bank: rendered thumbnail, your WAV: its peaks)
        std::array<float, 64> w {};
        if (wav) { if (auto a = proc.labAudioParents[(size_t) slot]) for (int b = 0; b < 64 && ! a->peaks.empty(); ++b) w[(size_t) b] = a->peaks[(size_t) (b * (int) a->peaks.size() / 64)]; }
        else w = proc.parentWave[(size_t) slot];
        float peak = 0.0001f; for (auto v : w) peak = std::max (peak, v);
        const float bw = wv.getWidth() / 64.0f;
        for (int b = 0; b < 64; ++b)
        {
            const float v = std::pow (w[(size_t) b] / peak, 0.7f);
            const float h = std::max (1.0f, v * wv.getHeight() * 0.48f);
            g.setColour (th.text.interpolatedWith (s.accent, (float) b / 110.0f).withAlpha (0.88f));
            g.fillRoundedRectangle (wv.getX() + (float) b * bw + bw * 0.2f, wv.getCentreY() - h, bw * 0.6f, h * 2.0f, bw * 0.3f);
        }
        g.setColour (th.text);
        g.setFont (serif (27.0f, false, 0.02f));
        g.drawFittedText (proc.labParentName (slot), Rectangle<int> (64, 176, getWidth() - 84, 32), Justification::centred, 1, 0.6f);
        const auto& pg = proc.parent (slot);
        String tags = wav ? String ("YOUR SOUND") : pg.cat >= 0 ? categoryNames()[pg.cat] : String ("USER");
        if (! wav && pg.gen > 0) tags << "  .  GEN " << pg.gen;
        g.setColour (s.accent.withAlpha (0.85f));
        g.setFont (serif (13.0f, true, 0.24f));
        g.drawFittedText (tags.toUpperCase(), Rectangle<int> (0, 208, getWidth(), 20), Justification::centred, 1, 0.7f);
        // play button
        const auto pr = playRect();
        g.setColour (playOver ? s.accent : th.text.withAlpha (0.6f)); g.drawEllipse (pr.reduced (1), 1.4f);
        Path tri; const auto pc = pr.getCentre(); tri.addTriangle (pc.x - 4.5f, pc.y - 7, pc.x - 4.5f, pc.y + 7, pc.x + 7.5f, pc.y);
        g.setColour (playOver ? s.accent : th.text.withAlpha (0.9f)); g.fillPath (tri);
    }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; playOver = false; repaint(); }
    void mouseMove (const MouseEvent& e) override { const bool p = proc.labSlotFilled (slot) && playRect().expanded (4).contains (e.position); if (p != playOver) { playOver = p; repaint(); } }
    void mouseUp (const MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown()) return;
        if (proc.labSlotFilled (slot) && playRect().expanded (4).contains (e.position)) { proc.auditionParent (slot); if (onChanged) onChanged(); return; }
        if (onClick) onClick();
    }
private:
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int slot; bool over = false, playOver = false, dropHot = false;
};

//==============================================================================
// v0.34 BREED REACTOR: a vacuum tube with an amber core. It charges with every chosen sound, each sound streams in
// in its category colour (the colours mix in the core), more sounds = more rings, MUTATE / CHAOS makes it restless.
// BREED: implosion, flash, a shock ring - and the sparks fly into the children (SparkOverlay).
struct ReactorState
{
    int sounds = 0, maxSounds = 2;
    Colour stream[4];
    float turbulence = 0.0f;   // 0..1
};
struct ReactorAnim { float phase = 0.0f, charge = 0.0f, boom = -1.0f; bool over = false; };

static Colour streamColour (int cat)
{
    // the category colour, calmed down and pulled a little towards the amber of the theme
    const auto c = categoryColour (cat);
    return c.withMultipliedSaturation (0.75f).interpolatedWith (kk::theme().accent, 0.25f);
}

static void drawReactor (Graphics& g, Point<float> c, float R, const ReactorState& st, const ReactorAnim& an, bool ready, const String& word)
{
    const auto& th = kk::theme();
    const float turb = jlimit (0.0f, 1.0f, st.turbulence);
    const float boom = an.boom;
    const float implode = boom >= 0.0f && boom < 0.22f ? boom / 0.22f : 0.0f;
    const float flash = boom >= 0.18f && boom < 0.55f ? 1.0f - (boom - 0.18f) / 0.37f : 0.0f;
    // glass bulb: dark smoked interior in both themes, so the amber reads
    g.setGradientFill (ColourGradient (Colour (0xff23262b), c.x, c.y - R * 0.3f, Colour (0xff08090b), c.x, c.y + R, true));
    g.fillEllipse (Rectangle<float> (R * 2, R * 2).withCentre (c));
    // tube internals: the anode plates and the mica discs, faint
    g.setColour (Colour (0xff8d949c).withAlpha (0.16f));
    for (float sx : { -1.0f, 1.0f })
        g.drawRoundedRectangle (Rectangle<float> (R * 0.16f, R * 1.15f).withCentre (c.translated (sx * R * 0.5f, 0)), 4.0f, 1.0f);
    g.drawEllipse (Rectangle<float> (R * 1.3f, R * 0.22f).withCentre (c.translated (0, -R * 0.66f)), 1.0f);
    g.drawEllipse (Rectangle<float> (R * 1.3f, R * 0.22f).withCentre (c.translated (0, R * 0.66f)), 1.0f);

    const float charge = an.charge * (1.0f - 0.75f * implode);
    // core colour: the streams mixed, then warmed by the amber
    Colour mix = th.accent;
    if (st.sounds > 0)
    {
        float rr = 0, gg = 0, bb = 0;
        for (int i = 0; i < std::min (st.sounds, 4); ++i) { rr += st.stream[i].getFloatRed(); gg += st.stream[i].getFloatGreen(); bb += st.stream[i].getFloatBlue(); }
        const float n = (float) std::min (st.sounds, 4);
        mix = Colour::fromFloatRGBA (rr / n, gg / n, bb / n, 1.0f).interpolatedWith (th.accent, 0.45f);
    }

    // rings: one per sound slot, lit when the slot holds a sound
    const int rings = std::max (st.maxSounds, 2);
    for (int i = 0; i < rings; ++i)
    {
        const float rad = R * (0.34f + 0.13f * (float) i);
        const bool lit = i < st.sounds;
        const float spin = an.phase * (0.25f + 0.12f * (float) i) * (1.0f + turb * 2.5f) * (i % 2 ? -1.0f : 1.0f);
        Path ring;
        const int segs = 3 + i;
        for (int k = 0; k < segs; ++k)
        {
            const float a0 = spin + MathConstants<float>::twoPi * (float) k / (float) segs;
            ring.addCentredArc (c.x, c.y, rad, rad, 0, a0, a0 + MathConstants<float>::twoPi / (float) segs * 0.62f, true);
        }
        g.setColour ((lit ? (i < 4 ? st.stream[i] : mix) : Colour (0xff8d949c)).withAlpha (lit ? 0.55f : 0.12f));
        g.strokePath (ring, PathStrokeType (lit ? 1.4f : 1.0f));
    }

    // streams: each sound flows into the core from its own side
    for (int sIdx = 0; sIdx < std::min (st.sounds, 4); ++sIdx)
    {
        const float base = sIdx == 0 ? MathConstants<float>::pi : sIdx == 1 ? 0.0f : sIdx == 2 ? -MathConstants<float>::halfPi : MathConstants<float>::halfPi;
        const float dir = sIdx % 2 ? -1.0f : 1.0f;
        for (int j = 0; j < 9; ++j)
        {
            float t = std::fmod (an.phase * (0.35f + 0.25f * charge) + (float) j / 9.0f + (float) sIdx * 0.13f, 1.0f);
            if (implode > 0) t = std::min (1.0f, t + implode);
            const float wob = turb * 0.35f * std::sin (an.phase * 7.0f + (float) j * 2.1f);
            const float rad = R * 0.92f * (1.0f - t) + R * 0.12f * t;
            const float ang = base + dir * (1.0f - t) * 1.1f + wob;
            const Point<float> pt (c.x + std::cos (ang) * rad, c.y + std::sin (ang) * rad);
            const float a = std::sin (t * MathConstants<float>::pi) * 0.9f;
            g.setColour (st.stream[sIdx].withAlpha (a * 0.35f)); g.fillEllipse (Rectangle<float> (7, 7).withCentre (pt));
            g.setColour (st.stream[sIdx].brighter (0.3f).withAlpha (a)); g.fillEllipse (Rectangle<float> (2.6f, 2.6f).withCentre (pt));
        }
    }

    // the core
    const float coreR = (ready ? 9.0f + 17.0f * charge : 5.0f) * (1.0f + 0.04f * std::sin (an.phase * 3.0f));
    const float glowA = ready ? 0.22f + 0.4f * charge : 0.10f;
    g.setGradientFill (ColourGradient (mix.withAlpha (glowA), c.x, c.y, mix.withAlpha (0.0f), c.x + coreR * 3.6f, c.y, true));
    g.fillEllipse (Rectangle<float> (coreR * 7.2f, coreR * 7.2f).withCentre (c));
    Path core;
    for (int k = 0; k <= 48; ++k)
    {
        const float a = MathConstants<float>::twoPi * (float) k / 48.0f;
        const float wob = 1.0f + turb * 0.18f * std::sin (a * 5.0f + an.phase * 6.0f) * std::sin (a * 3.0f - an.phase * 4.0f);
        const Point<float> pt (c.x + std::cos (a) * coreR * wob, c.y + std::sin (a) * coreR * wob);
        if (k == 0) core.startNewSubPath (pt); else core.lineTo (pt);
    }
    core.closeSubPath();
    g.setGradientFill (ColourGradient (Colour (0xfffff3e6).withAlpha (ready ? 0.95f : 0.25f), c.x, c.y - coreR * 0.2f, mix.withAlpha (ready ? 0.9f : 0.3f), c.x + coreR, c.y + coreR, true));
    g.fillPath (core);

    // shock ring and flash
    if (boom >= 0.2f && boom < 0.95f)
    {
        const float k = (boom - 0.2f) / 0.75f;
        const float rr = coreR + k * (R - coreR);
        g.setColour (th.accent.withAlpha (0.8f * (1.0f - k))); g.drawEllipse (Rectangle<float> (rr * 2, rr * 2).withCentre (c), 2.5f * (1.0f - k) + 0.6f);
        g.setColour (Colours::white.withAlpha (0.35f * (1.0f - k))); g.drawEllipse (Rectangle<float> (rr * 1.8f, rr * 1.8f).withCentre (c), 1.0f);
    }
    if (flash > 0)
    {
        g.setGradientFill (ColourGradient (Colour (0xfffff1dd).withAlpha (0.85f * flash), c.x, c.y, th.accent.withAlpha (0.0f), c.x + R, c.y, true));
        g.fillEllipse (Rectangle<float> (R * 2, R * 2).withCentre (c));
    }

    // glass: reflection and rim
    {
        Path refl; refl.addCentredArc (c.x, c.y, R * 0.86f, R * 0.86f, 0, -2.3f, -0.9f, true);
        g.setColour (Colours::white.withAlpha (0.16f)); g.strokePath (refl, PathStrokeType (R * 0.07f, PathStrokeType::curved, PathStrokeType::rounded));
    }
    g.setColour (an.over && ready ? th.accent.withAlpha (0.9f) : Colours::white.withAlpha (th.night ? 0.12f : 0.45f));
    g.drawEllipse (Rectangle<float> (R * 2, R * 2).withCentre (c).reduced (0.5f), an.over && ready ? 1.6f : 1.0f);

    // word
    if (ready)
    {
        g.setColour ((an.over ? th.accent : Colour (0xffeef0f2)).withAlpha (0.92f));
        g.setFont (Font (FontOptions (Font::getDefaultSansSerifFontName(), R * 0.16f, Font::bold)).withExtraKerningFactor (0.42f));
        g.drawText (word, Rectangle<float> (R * 2, R * 0.3f).withCentre (c.translated (R * 0.03f, R * 0.62f)), Justification::centred);
    }
    else
    {
        g.setColour (Colour (0xffd8dce0).withAlpha (0.8f));
        g.setFont (Font (FontOptions (Font::getDefaultSansSerifFontName(), R * 0.13f, Font::bold)).withExtraKerningFactor (0.3f));
        g.drawFittedText ("CHOOSE\n2 SOUNDS", Rectangle<float> (R * 1.4f, R * 0.5f).withCentre (c.translated (0, R * 0.55f)).toNearestInt(), Justification::centred, 2);
    }
}

// sparks from BREED into the child cards (drawn over the whole page, never takes a click)
class SparkOverlay : public Component, private Timer
{
public:
    SparkOverlay() { setInterceptsMouseClicks (false, false); }
    void fire (Point<float> from, const Array<Point<float>>& to)
    {
        src = from; dst = to; t = -0.22f;   // they leave right after the flash
        startTimerHz (60);
    }
    void paint (Graphics& g) override
    {
        if (t < 0.0f || dst.isEmpty()) return;
        const auto& th = kk::theme();
        for (int i = 0; i < dst.size(); ++i)
        {
            const float local = jlimit (0.0f, 1.0f, (t - (float) i * 0.025f) / 0.55f);
            const auto d = dst[i];
            const Point<float> ctrl ((src.x + d.x) * 0.5f, std::min (src.y, d.y) - 60.0f - 18.0f * (float) (i % 3));
            auto at = [&] (float u) { const float v = 1.0f - u; return src * (v * v) + ctrl * (2.0f * v * u) + d * (u * u); };
            if (local > 0.0f && local < 1.0f)
            {
                for (int k = 6; k >= 0; --k)
                {
                    const float u = std::max (0.0f, local - (float) k * 0.035f);
                    const auto p = at (u);
                    const float a = (1.0f - (float) k / 7.0f);
                    g.setColour (th.accent.withAlpha (0.55f * a)); g.fillEllipse (Rectangle<float> (3.0f + 4.0f * a, 3.0f + 4.0f * a).withCentre (p));
                }
                g.setColour (Colour (0xfffff1dd)); g.fillEllipse (Rectangle<float> (4, 4).withCentre (at (local)));
            }
            if (local >= 1.0f)   // arrival: a small ring on the card
            {
                const float k = jlimit (0.0f, 1.0f, (t - (float) i * 0.025f - 0.55f) / 0.35f);
                if (k < 1.0f)
                {
                    g.setColour (th.accent.withAlpha (0.7f * (1.0f - k)));
                    g.drawEllipse (Rectangle<float> (10 + 50 * k, 10 + 50 * k).withCentre (d), 1.6f);
                }
            }
        }
    }
private:
    void timerCallback() override
    {
        t += 1.0f / 60.0f;
        if (t > 1.2f) { stopTimer(); t = -1.0f; dst.clear(); }
        repaint();
    }
    Point<float> src; Array<Point<float>> dst; float t = -1.0f;
public:
    void freezeAt (float tt) { t = tt; stopTimer(); repaint(); }
};

//==============================================================================
// The big BREED button: glows on hover, flashes when it breeds
class BreedButton : public Component, public SettableTooltipClient, private Timer
{
public:
    explicit BreedButton (KKLookAndFeel& l) : lnf (l) { setTooltip ("BREED: make 6 new sounds (children) from PARENT A and PARENT B."); }
    std::function<void()> onBreed;
    std::function<ReactorState()> state;   // set = the big reactor (BREED LAB); not set = a small glowing button
    float flash = 0;
    bool ready = true;   // v0.34: false until both parents are chosen (dimmed, "choose 2 sounds")
    void boom() { an.boom = 0.0f; }
    void prime (float boomAt = -1.0f, float turb = -1.0f)   // snapshots: settle the reactor without waiting for the timer
    {
        if (! state) return;
        cur = state(); if (turb >= 0) cur.turbulence = turb;
        an.charge = cur.maxSounds > 0 ? (float) cur.sounds / (float) cur.maxSounds : 0.0f; an.phase = 1.7f; an.boom = boomAt;
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto c = getLocalBounds().toFloat().getCentre();
        if (state)
        {
            drawReactor (g, c, (float) std::min (getWidth(), getHeight()) * 0.5f - 2.0f, cur, an, ready, "BREED");
            return;
        }
        if (! ready)
        {
            g.setColour (TC (0xe0141010)); g.fillEllipse (getLocalBounds().toFloat().reduced (getWidth() * 0.16f));
            g.setColour (TC (0xffd8d2ce).withAlpha (0.8f)); g.setFont (serif (13.0f, true, 0.2f));
            g.drawFittedText ("CHOOSE\n2 SOUNDS", getLocalBounds().reduced (getWidth() / 4), Justification::centred, 2);
            return;
        }
        const float a = jlimit (0.0f, 1.0f, (over ? 0.35f : 0.0f) + flash);
        if (a <= 0.0f) return;
        g.setGradientFill (ColourGradient (s.accent.withAlpha (0.55f * a), c.x, c.y, s.accent.withAlpha (0.0f), c.x + 100, c.y, true));
        g.fillEllipse (getLocalBounds().toFloat());
        g.setColour (TC (0xffffffff).withAlpha (0.35f * a)); g.drawEllipse (Rectangle<float> (170, 170).withCentre (c), 2.0f);
    }
    bool hitTest (int x, int y) override { return Point<int> (x, y).getDistanceFrom ({ getWidth() / 2, getHeight() / 2 }) <= getWidth() / 2; }
    void mouseEnter (const MouseEvent&) override { over = true; an.over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; an.over = false; repaint(); }
    void mouseUp (const MouseEvent& e) override { if (contains (e.getPosition()) && onBreed) { flash = 1.0f; onBreed(); repaint(); } }
    void visibilityChanged() override { syncTimer(); }
    void parentHierarchyChanged() override { syncTimer(); }
private:
    void syncTimer() { if (state && isShowing()) startTimerHz (30); else stopTimer(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        cur = state();
        const float dt = 1.0f / 30.0f;
        an.phase += dt * (1.0f + cur.turbulence * 1.5f);
        const float target = cur.maxSounds > 0 ? (float) cur.sounds / (float) cur.maxSounds : 0.0f;
        an.charge += (target - an.charge) * 0.08f;
        if (an.boom >= 0.0f) { an.boom += dt; if (an.boom > 1.05f) an.boom = -1.0f; }
        repaint();
    }
    KKLookAndFeel& lnf; bool over = false;
    ReactorState cur; ReactorAnim an;
};

//==============================================================================
// CHILD card: waveform of the child's real sound, play, stars, SAVE - and drag it out (WAV, or the loop as MIDI)
// v0.35: with your own sound in a parent the children are audio kids (PAIR engine) - same card, same buttons
class ChildCard : public Component, public SettableTooltipClient
{
public:
    ChildCard (KeysKillaProcessor& p, KKLookAndFeel& l, int i) : proc (p), lnf (l), index (i)
    {
        setTooltip ("Click = load it on the keys.  Play = hear it.  SAVE = into your folders / presets.  DRAG = into FL Studio as a WAV.  Right-click = more");
    }
    std::function<void (int)> onMenu;
    std::function<void (int, Component*)> onSave;
    bool audio() const { return proc.labAudioMode(); }
    int count() const { return audio() ? (int) proc.pairKids.size() : (int) proc.kids().size(); }
    bool has() const { return index < count(); }
    bool selected() const { return has() && (audio() ? proc.pairSel == index : proc.selectedChild() == index); }
    Rectangle<float> saveRect() const { return { 144.0f, 93.0f, 44.0f, 20.0f }; }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const auto& th = kk::theme();
        const bool sel = selected();
        auto r = getLocalBounds().toFloat();
        if (sel) drawGlowFrame (g, r.reduced (3), s.accent, 6.0f);
        else if (over && has()) { g.setColour (s.accent.withAlpha (0.1f)); g.fillRoundedRectangle (r.reduced (3), 6); }
        if (! has())
        {
            g.setColour (th.dim.withAlpha (0.8f)); g.setFont (serif (13.0f, false, 0.2f));
            g.drawText (index == 0 ? "press BREED" : "", Rectangle<float> (12, 44, 130, 50), Justification::centred);
            return;
        }
        const Rectangle<float> wv (14, 42, 126, 46);
        const Colour col = sel ? s.accent : th.text.withAlpha (0.8f);
        std::array<float, 64> w {};
        bool ready = true;
        if (audio())
        {
            const auto& k = proc.pairKids[(size_t) index];
            for (int b = 0; b < 64 && ! k->peaks.empty(); ++b) w[(size_t) b] = k->peaks[(size_t) (b * (int) k->peaks.size() / 64)];
        }
        else { const auto& c = proc.kids()[(size_t) index]; ready = c.waveReady; if (ready) w = c.wave; }
        if (audio() && proc.mainLoopMode && proc.loopPlaying() && proc.pairLoopKid == index)   // v0.37: your sound's melody loop
        {
            KeysKillaProcessor::Genome lg; lg.loop = proc.currentLoop();
            drawLoopRoll (g, wv, proc.loopNotes (lg), (float) proc.loopBars() * 4.0f, col, proc.loopBeat.load());
            g.setColour (s.accent); g.setFont (serif (11.0f, true, 0.2f));
            g.drawText ("LOOP PLAYING", Rectangle<float> (14, 24, 126, 14), Justification::centredLeft);
        }
        else if (! audio() && proc.mainLoopMode)
        {
            const auto& c = proc.kids()[(size_t) index];
            const bool playing = proc.loopIsChild (index);
            drawLoopRoll (g, wv, proc.loopNotes (c.g), (float) proc.loopBars() * 4.0f, col, playing ? proc.loopBeat.load() : -1.0f);
            g.setColour (playing ? s.accent : th.dim); g.setFont (serif (11.0f, true, 0.2f));
            g.drawText (playing ? "LOOP PLAYING" : "LOOP", Rectangle<float> (14, 24, 126, 14), Justification::centredLeft);
        }
        else
        {
            float peak = 0.0001f; for (auto v : w) peak = std::max (peak, v);
            for (int b = 0; b < 64; ++b)
            {
                const float v = ready ? std::pow (w[(size_t) b] / peak, 0.7f) : 0.04f;
                const float h = std::max (1.0f, v * wv.getHeight() * 0.5f);
                const float x = wv.getX() + (float) b * wv.getWidth() / 64.0f;
                g.setColour ((sel ? s.accent : col.interpolatedWith (s.accent, (float) b / 90.0f)).withAlpha (sel ? 0.95f : 0.85f));
                g.fillRect (x, wv.getCentreY() - h, 1.6f, h * 2.0f);
            }
        }
        if (audio())
        {
            g.setColour (th.dim); g.setFont (serif (10.5f, true, 0.12f));
            g.drawFittedText (proc.mainLoopMode && ! (proc.loopPlaying() && proc.pairLoopKid == index) ? String ("LOOP - press play") : proc.pairKids[(size_t) index]->method,
                              Rectangle<int> (12, 95, 128, 16), Justification::centredLeft, 1, 0.7f);
        }
        else
            for (int st = 0; st < 5; ++st)
            {
                auto sr = starRect (st);
                const int rating = proc.kids()[(size_t) index].rating;
                drawStar (g, sr.getCentre(), sr.getWidth() * 0.5f, st < rating ? s.accent : Colour (0x00000000), st < rating ? s.accent : th.dim);
            }
        // SAVE
        const auto sv = saveRect();
        g.setColour (saveOver ? s.accent.withAlpha (0.18f) : th.glass); g.fillRoundedRectangle (sv, 9);
        g.setColour (saveOver ? s.accent : th.text.withAlpha (0.45f)); g.drawRoundedRectangle (sv, 9, 1.0f);
        g.setColour (saveOver ? s.accent : th.text); g.setFont (serif (11.0f, true, 0.18f));
        g.drawText ("SAVE", sv, Justification::centred);
    }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; saveOver = false; repaint(); }
    void mouseMove (const MouseEvent& e) override { const bool o = has() && saveRect().contains (e.position); if (o != saveOver) { saveOver = o; repaint(); } }
    void mouseDrag (const MouseEvent& e) override
    {
        if (! has() || dragging || e.getDistanceFromDragStart() < 8) return;
        dragging = true;
        File f;
        if (audio()) f = proc.exportPairKid (index);
        else if (proc.mainLoopMode) f = proc.exportLoopMidi (proc.kids()[(size_t) index].g);
        else f = proc.exportChildWav (index);
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const MouseEvent& e) override
    {
        const bool wasDrag = dragging; dragging = false;
        if (! has() || wasDrag || e.mouseWasDraggedSinceMouseDown()) return;
        if (saveRect().expanded (2).contains (e.position)) { if (onSave) onSave (index, this); return; }
        if (e.mods.isPopupMenu()) { if (onMenu) onMenu (index); return; }
        const bool playHit = e.position.getDistanceFrom ({ 160.0f, 66.0f }) < 20.0f;
        if (audio())
        {
            if (playHit && proc.mainLoopMode) proc.togglePairLoop (index);
            else proc.selectPairKid (index, playHit);
            repaint(); return;
        }
        for (int st = 0; st < 5; ++st) if (starRect (st).expanded (3).contains (e.position)) { proc.rateChild (index, st + 1); repaint(); return; }
        if (playHit) { proc.playChild (index); repaint(); }   // play button (sound or loop)
        else proc.selectChild (index);
    }
private:
    Rectangle<float> starRect (int i) const { return { 30.0f + (float) i * 18.5f, 97.0f, 15.0f, 15.0f }; }
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
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int index; bool over = false, saveOver = false, dragging = false;
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
        const int src = proc.geneOfSelected (gene);   // v0.37: works for your own sounds too (the child is spliced)
        for (int k = 0; k < 2; ++k)
        {
            if (src != k) continue;
            auto hr = half (k).reduced (1.5f, 2.0f);
            g.setColour (s.accent.withAlpha (0.3f)); g.fillRoundedRectangle (hr.expanded (2), 6);
            g.setGradientFill (ColourGradient (s.accent.brighter (0.15f), 0, hr.getY(), s.accent.darker (0.3f), 0, hr.getBottom(), false));
            g.fillRoundedRectangle (hr, 5);
            g.setColour (TC (0xffffffff)); g.setFont (serif (15.0f, false, 0.0f));
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
        const int sel = proc.labAudioMode() ? proc.pairSel : proc.selectedChild();
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
        g.setColour (TC (0xffe6e0dc)); g.setFont (serif (24.0f, false, 0.12f));
        g.drawText ("WILD", Rectangle<float> (0, 4, (float) getWidth(), 30), Justification::centred);
        const float x = 26.0f;
        g.setColour (TC (0xff5a5252)); g.fillRect (x - 1.0f, top, 2.0f, bottom - top);
        static const char* labels[] { "CRAZY", "WILD", "MIXED", "SOFT", "SAFE" };
        for (int i = 0; i < 5; ++i)
        {
            const float y = top + (bottom - top) * (float) i / 4.0f;
            g.setColour (TC (0xff8e8686)); g.fillEllipse (Rectangle<float> (9, 9).withCentre ({ x, y }));
            g.drawLine (x + 8, y, x + 18, y, 1.0f);
            g.setColour (TC (0xffe6e3ff)); g.setFont (serif (15.0f, false, 0.1f));
            g.drawText (labels[i], Rectangle<float> (x + 24, y - 9, 90, 18), Justification::centredLeft);
        }
        const float y = bottom - (bottom - top) * proc.breedWild;
        g.setColour (s.accent.withAlpha (0.7f)); g.fillRect (x - 1.5f, y, 3.0f, bottom - y);
        g.setColour (s.accent.withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (30, 30).withCentre ({ x, y }));
        g.setColour (s.accent.withAlpha (0.5f)); g.fillEllipse (Rectangle<float> (19, 19).withCentre ({ x, y }));
        g.setColour (s.accent); g.fillEllipse (Rectangle<float> (12, 12).withCentre ({ x, y }));
        g.setColour (TC (0xffffffff)); g.fillEllipse (Rectangle<float> (4, 4).withCentre ({ x, y }));
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
        g.setColour (TC (an.valid() ? 0xff181634 : 0xff110f26)); g.fillRoundedRectangle (r, 8);
        if (an.valid()) drawGlowFrame (g, r, s.accent.withAlpha (over ? 1.0f : 0.55f), 8);
        else
        {
            Path o; o.addRoundedRectangle (r, 8); const float dash[] { 6, 5 };
            PathStrokeType (1.2f).createDashedStroke (o, o, dash, 2);
            g.setColour (over ? s.accent : TC (0xff4a4478)); g.fillPath (o);
        }
        g.setColour (TC (0xffe6e3ff)); g.setFont (serif (15.0f, false, 0.3f));
        g.drawText ("SOUND " + String (slot + 1), Rectangle<float> (14, 8, 200, 20), Justification::centredLeft);
        // v0.34: no instrument pictures - a thin category line, the name says the rest
        if (an.valid()) { g.setColour (streamColour (an.cat)); g.fillRoundedRectangle (14.0f, 36.0f, 3.0f, 50.0f, 1.5f); }
        auto text = Rectangle<int> (28, 34, getWidth() - 40, 56);
        g.setColour (TC (an.valid() ? 0xffeee8e4 : 0x88eee8e4)); g.setFont (serif (an.valid() ? 21.0f : 17.0f, false, 0.02f));
        g.drawFittedText (an.valid() ? an.name : String ("click: choose a sound"), text, Justification::centredLeft, 2, 0.7f);
        if (an.valid())
        {
            g.setColour (TC (0xffa39a9a)); g.setFont (serif (13.0f, false, 0.2f));
            g.drawText (an.cat >= 0 ? categoryNames()[an.cat].toUpperCase() : String ("USER"), Rectangle<int> (28, 90, getWidth() - 40, 18), Justification::centredLeft);
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
    Rectangle<float> playRect() const { return { (float) getWidth() - 44.0f, 8.0f, 30.0f, 30.0f }; }
    void paintOverChildren (Graphics& g) override
    {
        if (! proc.ancestor (slot).valid()) return;
        const auto& th = kk::theme();
        const auto pr = playRect();
        g.setColour (playOver ? th.accent : th.text.withAlpha (0.6f)); g.drawEllipse (pr.reduced (1), 1.3f);
        Path tri; const auto pc = pr.getCentre(); tri.addTriangle (pc.x - 4, pc.y - 6.5f, pc.x - 4, pc.y + 6.5f, pc.x + 7, pc.y);
        g.setColour (playOver ? th.accent : th.text.withAlpha (0.9f)); g.fillPath (tri);
    }
    void mouseMove (const MouseEvent& e) override { const bool p = proc.ancestor (slot).valid() && playRect().expanded (4).contains (e.position); if (p != playOver) { playOver = p; repaint(); } }
    void mouseUp (const MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown()) return;
        if (proc.ancestor (slot).valid() && playRect().expanded (4).contains (e.position)) { proc.auditionAncestor (slot); return; }   // hear it
        if (onChoose) onChoose (slot);
    }
private:
    bool playOver = false;
    void changed() { repaint(); if (onChanged) onChanged(); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; int slot; bool over = false;
    TextButton prev, next, dice, clear;
};

class TreeBreedButton : public Component, public SettableTooltipClient, private Timer
{
public:
    explicit TreeBreedButton (KKLookAndFeel& l) : lnf (l) { setTooltip ("BREED: 6 new results from the chosen sounds - a new mix and new melodies every time."); }
    std::function<void()> onBreed;
    std::function<ReactorState()> state;   // v0.34: the BREED reactor (3-4 sounds = more rings)
    float flash = 0;
    void paint (Graphics& g) override
    {
        const auto c = getLocalBounds().toFloat().getCentre();
        const float R = (float) std::min (getWidth(), getHeight()) * 0.5f - 4.0f;
        if (state) cur = state();
        if (an.phase == 0.0f) an.charge = cur.maxSounds > 0 ? (float) cur.sounds / (float) cur.maxSounds : 0.0f;   // first paint: already settled
        drawReactor (g, c, R, cur, an, cur.sounds >= 2, "BREED");
    }
    bool hitTest (int x, int y) override { return Point<int> (x, y).getDistanceFrom ({ getWidth() / 2, getHeight() / 2 }) <= std::min (getWidth(), getHeight()) / 2; }
    void mouseEnter (const MouseEvent&) override { an.over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { an.over = false; repaint(); }
    void mouseUp (const MouseEvent& e) override { if (contains (e.getPosition()) && onBreed) { flash = 1.0f; an.boom = 0.0f; onBreed(); repaint(); } }
    void visibilityChanged() override { syncTimer(); }
    void parentHierarchyChanged() override { syncTimer(); }
    void prime() { if (state) { cur = state(); an.charge = cur.maxSounds > 0 ? (float) cur.sounds / (float) cur.maxSounds : 0.0f; an.phase = 1.7f; } }
private:
    void syncTimer() { if (isShowing()) startTimerHz (30); else stopTimer(); }
    void timerCallback() override
    {
        if (! isShowing()) { stopTimer(); return; }
        if (state) cur = state();
        const float dt = 1.0f / 30.0f;
        an.phase += dt * (1.0f + cur.turbulence * 1.5f);
        const float target = cur.maxSounds > 0 ? (float) cur.sounds / (float) cur.maxSounds : 0.0f;
        an.charge += (target - an.charge) * 0.08f;
        if (an.boom >= 0.0f) { an.boom += dt; if (an.boom > 1.05f) an.boom = -1.0f; }
        repaint();
    }
    KKLookAndFeel& lnf;
    ReactorState cur; ReactorAnim an;
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
        g.setColour (TC (sel ? 0xff221a3e : 0xff15132e)); g.fillRoundedRectangle (r, 7);
        if (sel) drawGlowFrame (g, r, s.accent, 7);
        else { g.setColour (over && has ? s.accent.withAlpha (0.7f) : TC (0xff3a3264)); g.drawRoundedRectangle (r, 7, 1.2f); }
        g.setColour (TC (0xffe6e3ff)); g.setFont (serif (15.0f, false, 0.3f));
        g.drawText ((loopMode ? "LOOP " : "SOUND ") + String (index + 1), Rectangle<float> (14, 8, 150, 20), Justification::centredLeft);
        if (! has)
        {
            g.setColour (TC (0x55ffffff)); g.setFont (serif (14.0f));
            g.drawText (index == 0 ? "press BREED" : "", r, Justification::centred);
            return;
        }
        const auto& t = res[(size_t) index];
        auto area = Rectangle<float> (14, 32, r.getWidth() - 56, r.getHeight() - 76);
        if (loopMode)
        {
            g.setColour (TC (0xff0e0c22)); g.fillRoundedRectangle (area, 4);
            drawLoopRoll (g, area.reduced (5, 5), proc.loopNotes (t.g), (float) proc.loopBars() * 4.0f, sel ? s.accent : TC (0xffcfc6c6),
                          proc.loopIsTree (index) ? proc.loopBeat.load() : -1.0f);
        }
        else
            for (int b = 0; b < 64; ++b)
            {
                const float v = t.waveReady ? std::pow (t.wave[(size_t) b], 0.7f) : 0.04f;
                const float h = std::max (1.0f, v * area.getHeight() * 0.5f);
                g.setColour ((sel ? s.accent.interpolatedWith (TC (0xffff8a3d), (float) b / 64.0f) : TC (0xffd9b8ff).interpolatedWith (TC (0xffff6aa0), (float) b / 80.0f)).withAlpha (0.9f));
                g.fillRect (area.getX() + (float) b * area.getWidth() / 64.0f, area.getCentreY() - h, 1.6f, h * 2.0f);
            }
        // play / loop button
        const auto pc = playCentre();
        const bool playing = proc.loopIsTree (index);
        g.setColour (playing ? s.accent : TC (0xff2a2450)); g.fillEllipse (Rectangle<float> (30, 30).withCentre (pc));
        g.setColour (s.accent.withAlpha (0.8f)); g.drawEllipse (Rectangle<float> (30, 30).withCentre (pc), 1.5f);
        g.setColour (TC (0xffffffff));
        if (playing) g.fillRect (Rectangle<float> (10, 10).withCentre (pc));
        else { Path tri; tri.addTriangle (pc.x - 4, pc.y - 7, pc.x - 4, pc.y + 7, pc.x + 8, pc.y); g.fillPath (tri); }
        // name + stars
        g.setColour (TC (0xffc8c4e8)); g.setFont (serif (13.0f, false, 0.02f));
        auto info = t.g.name;
        if (loopMode) info = String (kk::keyName (proc.effectiveLoopKey (t.g.loop))) + " MIN  .  " + info;
        g.drawFittedText (info, Rectangle<int> (14, (int) r.getBottom() - 42, (int) r.getWidth() - 20, 18), Justification::centredLeft, 1, 0.7f);
        for (int st = 0; st < 5; ++st)
        {
            auto sr = starRect (st);
            g.setColour (st < t.rating ? s.accent : TC (0xff6a6290));
            if (st < t.rating) g.fillEllipse (sr.reduced (2)); else g.drawEllipse (sr.reduced (2), 1.1f);
        }
    }
    void mouseEnter (const MouseEvent&) override { over = true; repaint(); }
    void mouseExit (const MouseEvent&) override { over = false; repaint(); }
    void mouseDown (const MouseEvent&) override { dragged = false; }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragged || index >= (int) proc.treeKids().size() || e.getDistanceFromDragStart() < 12) return;
        dragged = true;   // v0.40: the sound as a WAV
        const auto f = proc.exportGenomeWav (proc.treeKids()[(size_t) index].g);
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (dragged || index >= (int) proc.treeKids().size()) return;
        if (e.mods.isPopupMenu()) { if (onMenu) onMenu (index); return; }
        for (int st = 0; st < 5; ++st) if (starRect (st).expanded (2).contains (e.position)) { proc.rateTreeResult (index, st + 1); changed(); return; }
        // v0.35: a click plays it (SOUND) - in LOOP mode the play button starts / stops its loop
        if (e.position.getDistanceFrom (playCentre()) < 20.0f || proc.getTreeMode() != KeysKillaProcessor::treeLoop) proc.playTreeResult (index);
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
            c->setTooltip ("Click: hear it and load it.  Stars: rate.  Drag into FL Studio: the sound as a WAV.  Right-click: more.");
            addAndMakeVisible (*c); results.push_back (std::move (c));
        }
        breedBtn.onBreed = [this] { proc.treeBreed(); if (proc.getTreeMode() == KeysKillaProcessor::treeLoop && ! proc.loopPlaying()) proc.playTreeResult (0); refresh(); if (onLab) onLab(); };
        breedBtn.state = [this]
        {
            ReactorState st; st.maxSounds = KeysKillaProcessor::numAncestors;
            for (int k = 0; k < KeysKillaProcessor::numAncestors; ++k)
                if (const auto& an = proc.ancestor (k); an.valid()) st.stream[st.sounds++] = streamColour (an.cat);
            return st;
        };
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
            addChildComponent (c);   // v0.40: the FAMILY TREE makes sounds - melodies have their own page (MELODY)
        proc.setTreeMode (KeysKillaProcessor::treeSound);
        refresh();
        startTimerHz (24);
    }
    std::function<void (int)> onChoose, onResultMenu;
    std::function<void()> onLab;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        pageBackdrop (g, *this);
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
        g.setColour (TC (0xffaaa4cf)); g.setFont (serif (13.0f, false, 0.25f));
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
        for (Component* c : { (Component*) &bars8, (Component*) &bars16, (Component*) &keyBox, (Component*) &stopBtn, (Component*) &soundBtn, (Component*) &loopBtn }) c->setVisible (false);
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
        pageBackdrop (g, *this);
        g.setColour (s.panelEdge); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8, 1.4f);
        g.setColour (TC (0xffffffff)); g.setFont (serif (38.0f, true, 0.3f));
        const int tw = (int) std::ceil (GlyphArrangement::getStringWidth (g.getCurrentFont(), title)) + 30;
        g.drawText (title, 24, 10, tw, 48, Justification::centredLeft);
        g.setColour (TC (0xffaaa4cf)); g.setFont (serif (14.0f, false, 0.2f));
        g.drawText (subtitle, 24 + tw + 10, 22, 760, 24, Justification::centredLeft);
        g.setColour (TC (0xffc8c4e8)); g.setFont (serif (13.0f, false, 0.25f));
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
        k.slider->setTooltip (tip);
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
        b.setTooltip ("KEYS: your MIDI keyboard / FL piano roll plays " + what + " instead of the BREED LAB sound. Tip: one BREED LAB per instrument in FL.");
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
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (14.0f, Font::bold)).withExtraKerningFactor (0.08f));
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

// ---------------- SAVE TO FOLDER: one menu everywhere a sound is made (pair children, VST A / B, SAMPLER slices) ----------------
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
// SAVE TO: one menu everywhere - your folders and your SOUND KITS (a kit sorts every sound into Bass / Keys / Plucks / Pads ... by itself)
static void saveToFolderMenu (KeysKillaProcessor& proc, std::vector<kk::PairPtr> sounds, Component* target, std::function<void (String)> done)
{
    sounds.erase (std::remove (sounds.begin(), sounds.end(), nullptr), sounds.end());
    if (sounds.empty()) { if (done) done ("nothing to save yet"); return; }
    const auto folders = kk::Library::folders();
    const auto kits = kk::SoundKits::kits();
    PopupMenu m, folderMenu, kitMenu;
    m.addSectionHeader (sounds.size() > 1 ? "SAVE " + String ((int) sounds.size()) + " SOUNDS TO" : "SAVE TO");
    m.addItem (1, "> FOLDER  " + proc.lastFolder + "   (last used)");
    m.addItem (3, "> SOUND KIT  " + proc.lastSoundKit + "   (sorted by sound)");
    m.addSeparator();
    for (int i = 0; i < folders.size(); ++i) folderMenu.addItem (10 + i, folders[i] + "   (" + String (kk::Library::sounds (folders[i]).size()) + ")");
    folderMenu.addSeparator(); folderMenu.addItem (2, "+ new folder ...");
    m.addSubMenu ("MY FOLDERS", folderMenu);
    for (int i = 0; i < kits.size(); ++i)
    {
        PopupMenu one;
        one.addItem (1000 + i * 20, "AUTO  (sorted by the sound: Bass, Keys, Plucks ...)");
        one.addSeparator();
        for (int c = 0; c < kk::SoundKits::categories().size(); ++c) one.addItem (1000 + i * 20 + 1 + c, kk::SoundKits::categories()[c]);
        kitMenu.addSubMenu (kits[i] + "   (" + String (kk::SoundKits::count (kits[i])) + ")", one);
    }
    kitMenu.addSeparator(); kitMenu.addItem (4, "+ new sound kit ...");
    m.addSubMenu ("SOUND KITS", kitMenu);
    auto save = [&proc, sounds, done] (const String& folder)
    {
        int n = 0; for (auto& s : sounds) n += proc.saveToFolder (s, folder).existsAsFile() ? 1 : 0;
        if (done) done (n > 0 ? "saved to " + folder + (n > 1 ? "  (" + String (n) + ")" : String()) : String ("could not save"));
    };
    auto saveKit = [&proc, sounds, done] (const String& kit, const String& cat)
    {
        int n = 0; for (auto& s : sounds) n += proc.saveToSoundKit (s, kit, cat).existsAsFile() ? 1 : 0;
        if (done) done (n > 0 ? "saved into the sound kit " + kit + (n > 1 ? "  (" + String (n) + ")" : String()) : String ("could not save"));
    };
    m.showMenuAsync (PopupMenu::Options().withTargetComponent (target), [save, saveKit, folders, kits, &proc] (int r)
    {
        if (r == 1) save (proc.lastFolder);
        else if (r == 3) saveKit (proc.lastSoundKit, {});
        else if (r == 2) askNewFolder ([save] (String name) { save (name); });
        else if (r == 4)
        {
            auto* w = new AlertWindow ("NEW SOUND KIT", "Name of the new sound kit:", MessageBoxIconType::NoIcon);
            w->addTextEditor ("name", "", "Kit");
            w->addButton ("CREATE", 1, KeyPress (KeyPress::returnKey));
            w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
            w->enterModalState (true, ModalCallbackFunction::create ([w, saveKit] (int res)
            {
                const auto name = w->getTextEditorContents ("name").trim();
                if (res == 1 && kk::SoundKits::createKit (name)) saveKit (name, {});
            }), true);
        }
        else if (r >= 1000 && (r - 1000) / 20 < kits.size())
        {
            const int k = (r - 1000) / 20, c = (r - 1000) % 20;
            saveKit (kits[k], c == 0 ? String() : kk::SoundKits::categories()[c - 1]);
        }
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

// ---------------- MY SOUNDS (v0.33): your folders and SOUND KITS in one clear place - search, colour filters, hear, drag, PAIR ----------------
static Colour soundCatColour (const String& c)
{
    static const std::pair<const char*, uint32> m[] { { "Bass", 0xff3d8bff }, { "Keys", 0xffff8a3d }, { "Plucks", 0xffffd23f }, { "Pads", 0xff8b8bff },
                                                       { "Leads", 0xffff3b5c }, { "Vox", 0xfff0a6ff }, { "Brass & Strings", 0xffffc23d }, { "FX", 0xffff3fd2 },
                                                       { "Loops", 0xff34d399 }, { "Other", 0xff9ca3af } };
    for (auto& [n, col] : m) if (c == n) return Colour (col).withMultipliedSaturation (0.55f).withMultipliedBrightness (kk::theme().night ? 0.95f : 0.8f);
    return TC (0xff9ca3af);
}
class MySoundsPage : public Component, public DragAndDropContainer, private Timer
{
public:
    MySoundsPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        placeModel.page = this; soundModel.page = this;   // before the lists ask for rows
        placeList.setModel (&placeModel); placeList.setRowHeight (30);
        soundList.setModel (&soundModel); soundList.setRowHeight (30);
        for (auto* lb : { &placeList, &soundList }) { lb->setColour (ListBox::backgroundColourId, Colours::transparentBlack); addAndMakeVisible (*lb); }
        search.setTextToShowWhenEmpty ("search your sounds ...", TC (0xff7d77a8));
        search.setFont (Font (FontOptions (16.0f)));
        search.onTextChange = [this] { filter(); };
        addAndMakeVisible (search);
        auto btn = [this] (HotButton& b, const String& t, const String& tip, Colour tint, std::function<void()> fn)
        { b.setButtonText (t); b.framed = true; b.tint = tint; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (newFolderBtn, "+ FOLDER", "A new folder for your sounds", TC (0xffff8a3d), [this] { askNewFolder ([this] (String n) { place = { 1, n }; reload(); }); });
        btn (newKitBtn, "+ SOUND KIT", "A new sound kit: a folder FL Studio can browse, your sounds sorted into Bass, Keys, Plucks, Pads ...", TC (0xff22d3ee), [this] { newKit(); });
        btn (renameBtn, "RENAME", "Rename the chosen folder / kit", TC (0xffaaa4cf), [this] { renamePlace(); });
        btn (delPlaceBtn, "DELETE", "Delete the chosen folder / kit with its sounds (they go to the recycle bin)", TC (0xffff2f6d), [this] { deletePlace(); });
        btn (factoryBtn, "FACTORY SOUNDS", "The sounds that come with BREED LAB (the SOUND LIBRARY)", TC (0xffff2f6d), [this] { if (onFactory) onFactory(); });
        btn (pairBtn, "INTO BREED LAB", "This sound becomes a parent in BREED LAB (next free slot) - breed it", TC (0xff4d9dff), [this] { toPair(); });
        btn (moveBtn, "MOVE TO ...", "Move this sound into another folder or sound kit", TC (0xffffd23f), [this] { moveMenu(); });
        btn (delBtn, "DELETE SOUND", "Delete this sound (Del) - it goes to the recycle bin", TC (0xffff3b5c), [this] { deleteSound(); });
        btn (showBtn, "SHOW IN EXPLORER", "Open it on your computer (add the Documents / KEYS KILLA folder to FL's browser once)", TC (0xff9b4dff), [this] { placeDir().revealToUser(); });
        setWantsKeyboardFocus (true);
        setOpaque (true);
        reload();
        startTimerHz (2);
    }
    std::function<void()> onFactory, onPair;
    void visibilityChanged() override { if (isVisible()) reload(); }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setGradientFill (ColourGradient (TC (0xff17143c), 0, 0, TC (0xff0c0b22), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 10);
        g.setGradientFill (ColourGradient (TC (0xffff8a3d), 0, 0, TC (0xff22d3ee), r.getRight(), 0, false)); g.drawRoundedRectangle (r.reduced (1), 10, 1.4f);
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (24.0f, Font::bold)).withExtraKerningFactor (0.08f));
        g.drawText ("MY", 20, 12, 60, 32, Justification::centredLeft);
        g.setGradientFill (ColourGradient (TC (0xffff8a3d), 56, 0, TC (0xffffd23f), 180, 0, false));
        g.drawText ("SOUNDS", 56, 12, 160, 32, Justification::centredLeft);
        // left column
        const auto pl = placeList.getBounds().toFloat().expanded (4);
        g.setColour (TC (0xff100e26)); g.fillRoundedRectangle (pl, 8);
        const auto sl = soundList.getBounds().toFloat().expanded (4);
        g.setColour (TC (0xff100e26)); g.fillRoundedRectangle (sl, 8);
        // category filter chips
        for (int i = 0; i < numChips(); ++i)
        {
            auto c = chipRect (i);
            const String name = i == 0 ? String ("ALL") : kk::SoundKits::categories()[i - 1];
            const Colour col = i == 0 ? TC (0xffff2f6d) : soundCatColour (name);
            const bool selc = i == catSel;
            const int n = countOf (i);
            if (selc) { g.setGradientFill (ColourGradient (col, c.getX(), 0, col.darker (0.4f), c.getRight(), 0, false)); g.fillRoundedRectangle (c, 7); }
            else { g.setColour (col.withAlpha (n > 0 ? 0.18f : 0.06f)); g.fillRoundedRectangle (c, 7); g.setColour (col.withAlpha (n > 0 ? 0.7f : 0.25f)); g.drawRoundedRectangle (c, 7, 1.0f); }
            g.setColour (selc ? TC (0xffffffff) : n > 0 ? TC (0xffeeeaff) : TC (0xff6a6290)); g.setFont (Font (FontOptions (11.5f, Font::bold)));
            g.drawFittedText (name.toUpperCase() + (n > 0 ? "  " + String (n) : String()), c.reduced (6, 0).toNearestInt(), Justification::centred, 1, 0.6f);
        }
        g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (12.0f, Font::bold)));
        g.drawText (placeTitle() + "  -  " + String (shown.size()) + " SOUNDS", soundList.getX(), soundList.getY() - 22, 600, 16, Justification::centredLeft);
        g.drawText ("CLICK = HEAR + PLAY ON THE KEYS     DOUBLE-CLICK = PARENT IN BREED LAB     DRAG = INTO FL STUDIO     DEL = DELETE",
                    soundList.getX(), soundList.getBottom() + 8, soundList.getWidth(), 16, Justification::centredLeft);
        g.setColour (TC (0xff36ff6a)); g.setFont (Font (FontOptions (13.0f, Font::bold)));
        g.drawText (note, 230, 18, getWidth() - 230 - 500, 20, Justification::centredLeft);
        if (shown.isEmpty())
        {
            g.setColour (TC (0xff7d77a8)); g.setFont (Font (FontOptions (16.0f)));
            g.drawFittedText (all.isEmpty() ? "Nothing here yet.\nUse SAVE TO under PAIR children, VST A / B, SAMPLER slices - into a folder or a SOUND KIT."
                                            : "No sound matches.", soundList.getBounds().reduced (20), Justification::centred, 3);
        }
    }
    void resized() override
    {
        const int w = getWidth(), h = getHeight();
        factoryBtn.setBounds (w - 196, 14, 180, 32);
        search.setBounds (w - 196 - 12 - 300, 14, 300, 32);
        placeList.setBounds (20, 64, 270, h - 64 - 100);
        newFolderBtn.setBounds (16, h - 84, 136, 32); newKitBtn.setBounds (158, h - 84, 136, 32);
        renameBtn.setBounds (16, h - 46, 136, 32); delPlaceBtn.setBounds (158, h - 46, 136, 32);
        const int x = 314;
        chipArea = { x, 60, w - x - 16, 26 };
        soundList.setBounds (x + 4, 122, w - x - 24, h - 122 - 92);
        const int bw = 150;
        showBtn.setBounds (w - 16 - bw, h - 46, bw, 32); delBtn.setBounds (showBtn.getX() - 8 - bw, h - 46, bw, 32);
        moveBtn.setBounds (delBtn.getX() - 8 - bw, h - 46, bw, 32); pairBtn.setBounds (moveBtn.getX() - 8 - bw, h - 46, bw, 32);
    }
    void mouseDown (const MouseEvent& e) override
    {
        for (int i = 0; i < numChips(); ++i) if (chipRect (i).contains (e.position)) { catSel = i; filter(); return; }
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
        if (row < 0 || row >= shown.size()) return false;
        out.add (shown[row].getFullPathName()); canMove = false;
        return true;
    }
private:
    struct Place { int type = 0; String name; };   // type 0 = all, 1 = folder, 2 = sound kit, -1 / -2 = section headers
    struct PlaceModel : public ListBoxModel
    {
        MySoundsPage* page = nullptr;
        int getNumRows() override { return (int) page->places.size(); }
        void paintListBoxItem (int row, Graphics& g, int w, int h, bool) override
        {
            if (! isPositiveAndBelow (row, (int) page->places.size())) return;
            const auto& pl = page->places[(size_t) row];
            if (pl.type < 0)
            {
                g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (11.5f, Font::bold)).withExtraKerningFactor (0.12f));
                g.drawText (pl.type == -1 ? "MY FOLDERS" : "SOUND KITS", 10, 0, w - 20, h, Justification::bottomLeft);
                return;
            }
            const bool sel = pl.type == page->place.type && pl.name == page->place.name;
            const Colour col = pl.type == 0 ? TC (0xffff2f6d) : pl.type == 1 ? TC (0xffff8a3d) : TC (0xff22d3ee);
            auto r = Rectangle<float> (2, 2, (float) w - 4, (float) h - 4);
            if (sel) { g.setGradientFill (ColourGradient (col.withAlpha (0.5f), r.getX(), 0, col.withAlpha (0.1f), r.getRight(), 0, false)); g.fillRoundedRectangle (r, 6); }
            // icon: a folder or a kit box
            auto ic = Rectangle<float> (10, (float) h * 0.5f - 8, 18, 16);
            g.setColour (col);
            if (pl.type == 2) { g.fillRoundedRectangle (ic, 3); g.setColour (TC (0xff100e26)); g.fillRect (ic.reduced (4, 6)); }
            else if (pl.type == 1) { g.fillRoundedRectangle (ic.withTrimmedTop (3), 3); g.fillRoundedRectangle (ic.withWidth (8).withHeight (5), 2); }
            else { for (int k = 0; k < 3; ++k) g.fillRoundedRectangle (ic.getX(), ic.getY() + k * 6, ic.getWidth(), 3.5f, 1.5f); }
            g.setColour (sel ? TC (0xffffffff) : TC (0xffd9d4f5)); g.setFont (Font (FontOptions (14.0f, Font::bold)));
            g.drawFittedText (pl.type == 0 ? String ("ALL MY SOUNDS") : pl.name, 36, 0, w - 36 - 44, h, Justification::centredLeft, 1, 0.7f);
            g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (12.0f)));
            g.drawText (String (page->countIn (pl)), w - 44, 0, 36, h, Justification::centredRight);
        }
        void listBoxItemClicked (int row, const MouseEvent&) override
        {
            if (! isPositiveAndBelow (row, (int) page->places.size()) || page->places[(size_t) row].type < 0) return;
            page->place = page->places[(size_t) row];
            if (page->place.type == 1) page->proc.lastFolder = page->place.name;
            if (page->place.type == 2) page->proc.lastSoundKit = page->place.name;
            page->sel = -1; page->reload();
        }
    } placeModel;
    struct SoundModel : public ListBoxModel
    {
        MySoundsPage* page = nullptr;
        int getNumRows() override { return page->shown.size(); }
        void paintListBoxItem (int row, Graphics& g, int w, int h, bool) override
        {
            if (! isPositiveAndBelow (row, page->shown.size())) return;
            const auto& f = page->shown[row];
            const auto cat = page->catOf (f);
            const Colour col = soundCatColour (cat);
            const bool sel = row == page->sel;
            auto r = Rectangle<float> (2, 1, (float) w - 4, (float) h - 2);
            g.setGradientFill (ColourGradient (col.withAlpha (sel ? 0.35f : row % 2 ? 0.07f : 0.11f), r.getX(), 0, TC (0x00100e26), r.getRight() * 0.7f, 0, false));
            g.fillRoundedRectangle (r, 6);
            if (sel) { g.setColour (col); g.drawRoundedRectangle (r, 6, 1.4f); }
            g.setColour (col); g.fillRoundedRectangle (r.getX(), r.getY() + 4, 4, r.getHeight() - 8, 2);
            auto pill = Rectangle<float> (14, (float) h * 0.5f - 10, 120, 20);
            g.setColour (col.withAlpha (0.22f)); g.fillRoundedRectangle (pill, 10);
            g.setColour (col); g.drawRoundedRectangle (pill, 10, 1.0f);
            g.setFont (Font (FontOptions (10.5f, Font::bold))); g.drawFittedText (cat.toUpperCase(), pill.reduced (6, 0).toNearestInt(), Justification::centred, 1, 0.6f);
            g.setColour (TC (0xffffffff).withAlpha (sel ? 1.0f : 0.9f)); g.setFont (Font (FontOptions (15.0f, Font::bold)));
            g.drawFittedText (f.getFileNameWithoutExtension(), 146, 0, w - 146 - 290, h, Justification::centredLeft, 1, 0.8f);
            g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (12.0f)));
            g.drawText (page->placeOf (f), w - 284, 0, 180, h, Justification::centredRight);
            g.drawText (f.getLastModificationTime().formatted ("%d.%m.%Y"), w - 96, 0, 86, h, Justification::centredRight);
        }
        void listBoxItemClicked (int row, const MouseEvent&) override
        {
            if (! isPositiveAndBelow (row, page->shown.size())) return;
            page->sel = row; page->proc.auditionFile (page->shown[row]); page->soundList.repaint(); page->grabKeyboardFocus();
        }
        void listBoxItemDoubleClicked (int row, const MouseEvent&) override { page->sel = row; page->toPair(); }
        var getDragSourceDescription (const SparseSet<int>& rows) override { return rows.isEmpty() ? var() : var (rows[0]); }
    } soundModel;

    // ---- data
    File placeDir() const
    {
        if (place.type == 1) return kk::Library::folder (place.name);
        if (place.type == 2) return kk::SoundKits::kit (place.name);
        return kk::Library::root().getParentDirectory();
    }
    String placeTitle() const { return place.type == 0 ? String ("ALL MY SOUNDS") : (place.type == 2 ? "SOUND KIT  " : "FOLDER  ") + place.name.toUpperCase(); }
    bool inKit (const File& f) const { return f.isAChildOf (kk::SoundKits::root()); }
    String catOf (const File& f) const
    {
        if (inKit (f) && kk::SoundKits::categories().contains (f.getParentDirectory().getFileName())) return f.getParentDirectory().getFileName();
        return kk::SoundKits::categoryFor (f.getFileNameWithoutExtension());
    }
    String placeOf (const File& f) const
    {
        if (inKit (f)) { auto k = f.getParentDirectory(); while (k.getParentDirectory() != kk::SoundKits::root() && k != File()) k = k.getParentDirectory(); return "kit  " + k.getFileName(); }
        return f.getParentDirectory().getFileName();
    }
    Array<File> filesIn (const Place& pl) const
    {
        Array<File> out;
        if (pl.type == 1 || pl.type == 0) for (auto& fo : (pl.type == 1 ? StringArray { pl.name } : kk::Library::folders())) out.addArray (kk::Library::sounds (fo));
        if (pl.type == 2 || pl.type == 0) for (auto& k : (pl.type == 2 ? StringArray { pl.name } : kk::SoundKits::kits())) out.addArray (kk::SoundKits::sounds (k));
        return out;
    }
    int countIn (const Place& pl) const
    {
        if (pl.type == 1) return kk::Library::sounds (pl.name).size();
        if (pl.type == 2) return kk::SoundKits::count (pl.name);
        return totalCount;
    }
    int numChips() const { return kk::SoundKits::categories().size() + 1; }
    Rectangle<float> chipRect (int i) const
    {
        const float w = (float) chipArea.getWidth() / (float) numChips();
        return { (float) chipArea.getX() + w * (float) i + 2.0f, (float) chipArea.getY(), w - 4.0f, 28.0f };
    }
    int countOf (int chip) const
    {
        if (chip == 0) return all.size();
        const auto c = kk::SoundKits::categories()[chip - 1];
        int n = 0; for (auto& f : all) n += catOf (f) == c ? 1 : 0;
        return n;
    }
    void reload()
    {
        places.clear();
        places.push_back ({ 0, {} });
        places.push_back ({ -1, {} });
        for (auto& f : kk::Library::folders()) places.push_back ({ 1, f });
        places.push_back ({ -2, {} });
        for (auto& k : kk::SoundKits::kits()) places.push_back ({ 2, k });
        bool found = false; for (auto& pl : places) found |= pl.type == place.type && pl.name == place.name;
        if (! found) place = { 0, {} };
        totalCount = filesIn ({ 0, {} }).size();
        all = filesIn (place);
        filter();
        placeList.updateContent(); placeList.repaint();
        sig = signature();
    }
    void filter()
    {
        shown.clear();
        const auto q = search.getText().trim().toLowerCase();
        const String c = catSel > 0 ? kk::SoundKits::categories()[catSel - 1] : String();
        for (auto& f : all)
            if ((q.isEmpty() || f.getFileNameWithoutExtension().toLowerCase().contains (q)) && (c.isEmpty() || catOf (f) == c)) shown.add (f);
        sel = jlimit (-1, shown.size() - 1, sel);
        soundList.updateContent(); soundList.repaint();
        repaint();
    }
    int signature() const { return filesIn ({ 0, {} }).size() * 7919 + kk::Library::folders().size() * 31 + kk::SoundKits::kits().size(); }
    void timerCallback() override { if (isVisible() && signature() != sig) reload(); }   // sounds saved from other pages show up
    File selected() const { return isPositiveAndBelow (sel, shown.size()) ? shown[sel] : File(); }
    void toPair()
    {
        const auto f = selected(); if (! f.existsAsFile()) return;
        const int slot = ! proc.labSlotFilled (0) ? 0 : ! proc.labSlotFilled (1) ? 1 : (pairRound++ % 2);
        if (proc.labDropFile (slot, f)) { note = f.getFileNameWithoutExtension() + " -> BREED LAB, PARENT " + String (slot == 0 ? "A" : "B"); if (onPair) onPair(); }
        repaint();
    }
    int pairRound = 0;
    void deleteSound()
    {
        const auto f = selected(); if (! f.existsAsFile()) return;
        kk::Library::deleteSound (f);
        note = f.getFileNameWithoutExtension() + " deleted (recycle bin)";
        reload();
    }
    void moveMenu()
    {
        const auto f = selected(); if (! f.existsAsFile()) { note = "pick a sound first"; repaint(); return; }
        PopupMenu m, fm, km;
        const auto folders = kk::Library::folders(); const auto kits = kk::SoundKits::kits();
        for (int i = 0; i < folders.size(); ++i) fm.addItem (10 + i, folders[i]);
        for (int i = 0; i < kits.size(); ++i) km.addItem (500 + i, kits[i] + "   (" + catOf (f) + ")");
        m.addSubMenu ("MY FOLDERS", fm); m.addSubMenu ("SOUND KITS", km);
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&moveBtn), [safe = Component::SafePointer<MySoundsPage> (this), f, folders, kits] (int r)
        {
            if (safe == nullptr || r == 0) return;
            File dir = r >= 500 ? kk::SoundKits::kit (kits[r - 500]).getChildFile (safe->catOf (f)) : kk::Library::folder (folders[r - 10]);
            dir.createDirectory();
            const auto to = dir.getNonexistentChildFile (f.getFileNameWithoutExtension(), f.getFileExtension(), false);
            safe->note = f.moveFileTo (to) ? f.getFileNameWithoutExtension() + " -> " + (r >= 500 ? "kit " + kits[r - 500] : folders[r - 10]) : String ("could not move");
            safe->reload();
        });
    }
    void newKit()
    {
        auto* w = new AlertWindow ("NEW SOUND KIT", "Name of the new sound kit:", MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", "", "Kit");
        w->addButton ("CREATE", 1, KeyPress (KeyPress::returnKey));
        w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([w, safe = Component::SafePointer<MySoundsPage> (this)] (int r)
        {
            const auto name = w->getTextEditorContents ("name").trim();
            if (safe != nullptr && r == 1 && kk::SoundKits::createKit (name)) { safe->place = { 2, name }; safe->proc.lastSoundKit = name; safe->reload(); }
        }), true);
    }
    void deletePlace()
    {
        if (place.type <= 0) { note = "pick a folder or a kit on the left"; repaint(); return; }
        const auto pl = place;
        AlertWindow::showOkCancelBox (MessageBoxIconType::WarningIcon, pl.type == 2 ? "DELETE SOUND KIT" : "DELETE FOLDER",
                                      "Delete \"" + pl.name + "\" and its " + String (countIn (pl)) + " sounds?\n(They go to the recycle bin.)",
                                      "DELETE", "Cancel", this, ModalCallbackFunction::create ([safe = Component::SafePointer<MySoundsPage> (this), pl] (int r)
        {
            if (safe == nullptr || r != 1) return;
            if (pl.type == 2) kk::SoundKits::deleteKit (pl.name); else kk::Library::deleteFolder (pl.name);
            safe->place = { 0, {} }; safe->reload();
        }));
    }
    void renamePlace()
    {
        if (place.type <= 0) { note = "pick a folder or a kit on the left"; repaint(); return; }
        auto* w = new AlertWindow (place.type == 2 ? "RENAME SOUND KIT" : "RENAME FOLDER", "New name:", MessageBoxIconType::NoIcon);
        w->addTextEditor ("name", place.name, "Name");
        w->addButton ("RENAME", 1, KeyPress (KeyPress::returnKey));
        w->addButton ("Cancel", 0, KeyPress (KeyPress::escapeKey));
        w->enterModalState (true, ModalCallbackFunction::create ([w, safe = Component::SafePointer<MySoundsPage> (this), from = place] (int r)
        {
            const auto to = w->getTextEditorContents ("name").trim();
            if (safe == nullptr || r != 1) return;
            const bool ok = from.type == 2 ? kk::SoundKits::renameKit (from.name, to) : kk::Library::renameFolder (from.name, to);
            if (ok) { safe->place = { from.type, to }; safe->reload(); }
        }), true);
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    ListBox placeList, soundList;
    TextEditor search;
    HotButton newFolderBtn { lnf }, newKitBtn { lnf }, renameBtn { lnf }, delPlaceBtn { lnf }, factoryBtn { lnf }, pairBtn { lnf }, moveBtn { lnf }, delBtn { lnf }, showBtn { lnf };
    std::vector<Place> places;
    Place place;
    Array<File> all, shown;
    Rectangle<int> chipArea;
    String note;
    int sel = -1, sig = 0, catSel = 0, totalCount = 0;
};

// ---------------- SAMPLER / CHOP (v0.32): your sample on a big waveform, movable slices, MPC pads, keys, drag out ----------------
//==============================================================================
// v0.37 FLIPS: EVOLVE for beats. Your chops in a 2-bar pattern in the middle, six children around it.
// Hover = hear it (in time).  Click = it becomes the middle and six new flips grow.  Drag = MIDI / WAV into FL.
class FlipView : public Component, private Timer
{
public:
    FlipView (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (seedBtn, "NEW FLIP", "A new first flip from your chops", [this] { proc.flipSeed(); repaint(); });
        btn (againBtn, "6 NEW KIDS", "Six new children of the middle flip", [this] { if (proc.flipCenter >= 0) proc.flipGrow (proc.flipCenter, true, wild); repaint(); });
        btn (playBtn, "PLAY", "Play / stop the middle flip (in time with FL)", [this]
        {
            if (proc.flipOn.load() && proc.flipPlaying == proc.flipCenter) proc.flipPlay (-1); else proc.flipPlay (proc.flipCenter);
            latched = proc.flipOn.load(); repaint();
        });
        btn (backBtn, "BACK", "Back to the flip it came from", [this]
        {
            if (! isPositiveAndBelow (proc.flipCenter, (int) proc.flips.size())) return;
            const int pa = proc.flips[(size_t) proc.flipCenter].parent;
            if (pa >= 0) { proc.flipCenter = pa; ++proc.flipVer; if (proc.flipOn.load()) proc.flipPlay (pa); repaint(); }
        });
        wildSl.setSliderStyle (Slider::LinearBar); wildSl.setRange (0.0, 1.0, 0.01); wildSl.setValue (0.35, dontSendNotification);
        wildSl.textFromValueFunction = [] (double v) { return v < 0.34 ? String ("SAFE") : v < 0.67 ? String ("MIXED") : String ("WILD"); };
        wildSl.setColour (Slider::trackColourId, lnf.skin->accent.withAlpha (0.5f));
        wildSl.setColour (Slider::textBoxTextColourId, kk::theme().text);
        wildSl.onValueChange = [this] { wild = (float) wildSl.getValue(); };
        wildSl.setTooltip ("SAFE = the children stay close to the middle flip.  WILD = they change a lot");
        wildSl.updateText();
        addAndMakeVisible (wildSl);
        startTimerHz (30);
    }
    // the PADS / FLIPS switch of the sampler sits in our top-left corner: clicks there go to it, not to us
    bool hitTest (int x, int y) override { return ! (x < 290 && y < 34) && Component::hitTest (x, y); }
    void resized() override
    {
        int x = 290;   // the PADS / FLIPS / MELODY switch sits left of the buttons
        for (auto* b : { &seedBtn, &againBtn, &playBtn, &backBtn }) { b->setBounds (x, 0, b == &againBtn ? 120 : 92, 32); x += b->getWidth() + 6; }
        wildSl.setBounds (x + 6, 2, 150, 28);
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        if (proc.flips.empty() || ! isPositiveAndBelow (proc.flipCenter, (int) proc.flips.size()))
        {
            g.setColour (t.text); g.setFont (kk::modern::font (20.0f, true, 0.12f));
            g.drawText ("FLIPS: press NEW FLIP", getLocalBounds().withTrimmedTop (40).withTrimmedBottom (20), Justification::centred);
            g.setColour (t.dim); g.setFont (kk::modern::font (13.0f, false, 0.05f));
            g.drawText ("a beat from your chops grows in the middle - hover a child to hear it, click it to keep evolving", getLocalBounds().withTrimmedTop (100), Justification::centred);
            return;
        }
        const auto& c = proc.flips[(size_t) proc.flipCenter];
        drawFlip (g, centreRect(), proc.flipCenter, true);
        for (int k = 0; k < (int) c.kids.size(); ++k) drawFlip (g, kidRect (k), c.kids[(size_t) k], false);
        ignoreUnused (lnf);
    }
    void mouseMove (const MouseEvent& e) override
    {
        const int h = nodeAt (e.position);
        if (h != hovered) { hovered = h; hoverSince = Time::getMillisecondCounter(); heard = false; repaint(); }
    }
    void mouseExit (const MouseEvent&) override
    {
        hovered = -1; repaint();
        if (proc.flipOn.load() && proc.flipPlaying != proc.flipCenter) { if (latched) proc.flipPlay (proc.flipCenter); else proc.flipPlay (-1); }
    }
    void mouseDown (const MouseEvent& e) override { downNode = nodeAt (e.position); dragged = false; }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragged || downNode < 0 || e.getDistanceFromDragStart() < 8) return;
        dragged = true;
        const auto f = proc.flipWavFile (downNode);
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (dragged || e.mouseWasDraggedSinceMouseDown()) return;
        const int n = nodeAt (e.position);
        if (n < 0) return;
        if (n == proc.flipCenter) { playBtn.triggerClick(); return; }
        proc.flipCenter = n;
        proc.flipGrow (n, false, wild);
        proc.flipPlay (n); latched = true;
        ++proc.flipVer; repaint();
    }
    int centreNode() const { return proc.flipCenter; }
private:
    Rectangle<float> area() const { return getLocalBounds().toFloat().withTrimmedTop (42); }
    Rectangle<float> centreRect() const { auto a = area(); return a.removeFromLeft (a.getWidth() * 0.36f).reduced (4); }
    Rectangle<float> kidRect (int k) const
    {
        auto a = area(); a.removeFromLeft (a.getWidth() * 0.36f + 8);
        const float w = a.getWidth() / 3.0f, h = a.getHeight() / 2.0f;
        return { a.getX() + w * (float) (k % 3), a.getY() + h * (float) (k / 3), w, h };
    }
    int nodeAt (Point<float> p) const
    {
        if (! isPositiveAndBelow (proc.flipCenter, (int) proc.flips.size())) return -1;
        if (centreRect().contains (p)) return proc.flipCenter;
        const auto& c = proc.flips[(size_t) proc.flipCenter];
        for (int k = 0; k < (int) c.kids.size(); ++k) if (kidRect (k).reduced (4).contains (p)) return c.kids[(size_t) k];
        return -1;
    }
    void drawFlip (Graphics& g, Rectangle<float> r, int node, bool centre)
    {
        const auto& t = kk::theme();
        const auto& f = proc.flips[(size_t) node];
        const bool hot = node == hovered, playing = proc.flipOn.load() && proc.flipPlaying == node;
        r = r.reduced (4);
        g.setColour (t.glass.withMultipliedAlpha (centre ? 1.4f : 1.0f)); g.fillRoundedRectangle (r, 10);
        g.setColour (centre || playing ? t.accent : hot ? t.accent.withAlpha (0.7f) : t.text.withAlpha (0.25f));
        g.drawRoundedRectangle (r, 10, centre ? 2.0f : 1.3f);
        g.setColour (t.text); g.setFont (kk::modern::font (centre ? 17.0f : 13.0f, true, 0.06f));
        g.drawText (f.name, r.reduced (12, 6).removeFromTop (22), Justification::centredLeft);
        if (playing) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (11.0f, true, 0.2f)); g.drawText ("PLAYING", r.reduced (12, 6).removeFromTop (22), Justification::centredRight); }
        // the steps like a piano roll: 32 columns (1/16), one row per chop - you see the beat
        auto grid = r.reduced (12, 8).withTrimmedTop (24).withTrimmedBottom (centre ? 22 : 4);
        const int steps = (int) f.steps.size();
        auto dch = proc.chop.current();
        const int rows = jlimit (1, 8, dch != nullptr ? dch->numSlices() : 8);
        const float cw = grid.getWidth() / (float) std::max (1, steps), rh = grid.getHeight() / (float) rows;
        g.setColour (t.well); g.fillRoundedRectangle (grid, 4);
        for (int i = 0; i < steps; ++i)
        {
            const float x = grid.getX() + cw * (float) i;
            if (i % 4 == 0) { g.setColour (t.text.withAlpha (i % 16 == 0 ? 0.3f : 0.1f)); g.fillRect (x, grid.getY(), 1.0f, grid.getHeight()); }
            const auto& s = f.steps[(size_t) i];
            if (s.slice < 0) continue;
            const int row = s.slice % rows;
            const float y = grid.getBottom() - rh * (float) (row + 1);
            g.setColour ((s.rev ? t.text : t.accent).withAlpha (0.45f + 0.55f * s.vel));
            g.fillRoundedRectangle (x + 1, y + 1, std::max (2.0f, cw * (float) s.len - 2.0f), std::max (2.0f, rh - 2.0f), 2.0f);
            if (s.semi != 0) { g.setColour (Colours::white.withAlpha (0.9f)); g.fillRect (x + 1, s.semi > 0 ? y + 1 : y + rh - 3, std::max (2.0f, cw - 2.0f), 2.0f); }
        }
        if (playing && proc.flipStepNow.load() >= 0)
        {
            g.setColour (t.text); g.fillRect (grid.getX() + cw * (float) proc.flipStepNow.load(), grid.getY(), 2.0f, grid.getHeight());
        }
        if (centre)
        {
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.1f));
            g.drawText ("GEN " + String (f.gen) + "   click = play / stop   drag = WAV into FL", r.reduced (12, 6).removeFromBottom (16), Justification::centredLeft);
        }
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        const auto now = Time::getMillisecondCounter();
        if (hovered >= 0 && hovered != proc.flipCenter && ! heard && now - hoverSince > 220) { heard = true; proc.flipPlay (hovered); }
        if (proc.flipVer.load() != lastVer || proc.flipOn.load()) { lastVer = proc.flipVer.load(); repaint(); }
        playBtn.setButtonText (proc.flipOn.load() && proc.flipPlaying == proc.flipCenter ? "STOP" : "PLAY");
        backBtn.setEnabled (isPositiveAndBelow (proc.flipCenter, (int) proc.flips.size()) && proc.flips[(size_t) proc.flipCenter].parent >= 0);
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton seedBtn { lnf }, againBtn { lnf }, playBtn { lnf }, backBtn { lnf };
    Slider wildSl;
    float wild = 0.35f;
    int hovered = -1, downNode = -1, lastVer = -1;
    uint32 hoverSince = 0;
    bool heard = false, dragged = false, latched = false;
};

// v0.42 SAMPLER MELODY: the sound in the sampler becomes an instrument and plays melodies that grow (genre, key, tempo)
class SamplerMelodyView : public Component, private Timer
{
public:
    SamplerMelodyView (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (newBtn, "NEW MELODIES", "Melodies for the sound in the sampler (genre, key and tempo you choose)", [this] { generate(); });
        btn (againBtn, "6 NEW KIDS", "Six new children of the middle melody", [this] { if (centre >= 0) { proc.melEvolve (centre, true); proc.melPlay (centre); } repaint(); });
        btn (playBtn, "PLAY", "Play / stop the middle melody (in time with FL)", [this] { if (proc.loopPlaying() && proc.melPlaying == centre) proc.melPlay (-1); else if (centre >= 0) proc.melPlay (centre); });
        btn (backBtn, "BACK", "Back to the melody it came from", [this]
        {
            if (! isPositiveAndBelow (centre, (int) proc.mels.size())) return;
            const int pa = proc.mels[(size_t) centre].parent;
            if (pa >= 0) { centre = pa; proc.melEvolve (pa, false); proc.melPlay (pa); repaint(); }
        });
        for (int g = -1; g < kk::mel::numGenres; ++g) genreBox.addItem (kk::mel::genreName (g), g + 2);
        genreBox.onChange = [this] { proc.melSetGenre (genreBox.getSelectedId() - 2); };
        genreBox.setTooltip ("The style of the melodies");
        addAndMakeVisible (genreBox);
        for (int k = 0; k < 12; ++k) keyBox.addItem (String (kk::mel::keyName (k)) + " MINOR", k + 1);
        for (int k = 0; k < 12; ++k) keyBox.addItem (String (kk::mel::keyName (k)) + " MAJOR", k + 13);
        keyBox.onChange = [this] { const int id = keyBox.getSelectedId() - 1; proc.melKey = id % 12; proc.melScale = id >= 12 ? kk::mel::scMajor : kk::mel::scMinor; };
        keyBox.setTooltip ("The key of the melodies");
        addAndMakeVisible (keyBox);
        startTimerHz (30);
    }
    std::function<kk::PairPtr()> soundToUse;   // the selected chop / part
    std::function<void (const String&)> onNote;
    void syncBoxes()
    {
        genreBox.setSelectedId (proc.melGenre + 2, dontSendNotification);
        keyBox.setSelectedId (proc.melKey + (proc.melScale == kk::mel::scMajor ? 13 : 1), dontSendNotification);
    }
    // the chop / part goes on the keys (tuned to C) - the melodies play it
    bool takeSound()
    {
        auto s = soundToUse ? soundToUse() : kk::PairPtr();
        if (s == nullptr) return false;
        proc.useSample (kk::PairLab::tuned (s, proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0), false);
        soundName = s->name;
        return true;
    }
    void generate()
    {
        if (! takeSound()) { if (onNote) onNote ("select a chop (or a part) first - it becomes the instrument"); return; }
        proc.melFromMine = false;
        proc.melGenerate();
        centre = proc.melShown.empty() ? -1 : proc.melShown[0];
        if (centre >= 0) { proc.melEvolve (centre, false); proc.melPlay (centre); }
        repaint();
    }
    int centreMelody() const { return centre; }
    void visibilityChanged() override
    {
        if (! isVisible()) { if (proc.melPlaying >= 0) proc.melPlay (-1); return; }
        syncBoxes();
        if (centre < 0 || ! isPositiveAndBelow (centre, (int) proc.mels.size())) generate(); else takeSound();
    }
    bool hitTest (int x, int y) override { return ! (x < 290 && y < 34) && Component::hitTest (x, y); }
    void resized() override
    {
        int x = 290;
        for (auto* b : { &newBtn, &againBtn, &playBtn, &backBtn }) { b->setBounds (x, 0, b == &newBtn ? 130 : b == &againBtn ? 112 : 76, 32); x += b->getWidth() + 6; }
        genreBox.setBounds (x + 4, 2, 120, 28); keyBox.setBounds (x + 130, 2, 120, 28);
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        if (! isPositiveAndBelow (centre, (int) proc.mels.size()))
        {
            g.setColour (t.text); g.setFont (kk::modern::font (20.0f, true, 0.12f));
            g.drawText ("MELODY: select a chop, then NEW MELODIES", getLocalBounds().withTrimmedTop (40).withTrimmedBottom (20), Justification::centred);
            return;
        }
        draw (g, centreRect(), centre, true);
        const auto& kids = proc.mels[(size_t) centre].kids;
        for (int k = 0; k < (int) kids.size() && k < 6; ++k) draw (g, kidRect (k), kids[(size_t) k], false);
    }
    void mouseMove (const MouseEvent& e) override { const int h = nodeAt (e.position); if (h != hovered) { hovered = h; hoverSince = Time::getMillisecondCounter(); heard = false; repaint(); } }
    void mouseExit (const MouseEvent&) override { hovered = -1; repaint(); if (proc.loopPlaying() && proc.melPlaying != centre && centre >= 0) proc.melPlay (centre); }
    void mouseDown (const MouseEvent& e) override { downNode = nodeAt (e.position); dragged = false; }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragged || downNode < 0 || e.getDistanceFromDragStart() < 8) return;
        dragged = true;
        const auto f = e.mods.isAltDown() ? proc.melExportWav (downNode) : proc.melExport (downNode);   // drag = MIDI, ALT + drag = WAV
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (dragged || e.mouseWasDraggedSinceMouseDown()) return;
        const int n = nodeAt (e.position);
        if (n < 0) return;
        if (n == centre) { playBtn.triggerClick(); return; }
        centre = n; proc.melEvolve (n, false); proc.melPlay (n);
        repaint();
    }
private:
    Rectangle<float> area() const { return getLocalBounds().toFloat().withTrimmedTop (42); }
    Rectangle<float> centreRect() const { auto a = area(); return a.removeFromLeft (a.getWidth() * 0.36f).reduced (4); }
    Rectangle<float> kidRect (int k) const
    {
        auto a = area(); a.removeFromLeft (a.getWidth() * 0.36f + 8);
        const float w = a.getWidth() / 3.0f, h = a.getHeight() / 2.0f;
        return { a.getX() + w * (float) (k % 3), a.getY() + h * (float) (k / 3), w, h };
    }
    int nodeAt (Point<float> p) const
    {
        if (! isPositiveAndBelow (centre, (int) proc.mels.size())) return -1;
        if (centreRect().contains (p)) return centre;
        const auto& kids = proc.mels[(size_t) centre].kids;
        for (int k = 0; k < (int) kids.size() && k < 6; ++k) if (kidRect (k).reduced (4).contains (p)) return kids[(size_t) k];
        return -1;
    }
    void draw (Graphics& g, Rectangle<float> r, int idx, bool isCentre)
    {
        const auto& t = kk::theme();
        const auto& m = proc.mels[(size_t) idx];
        const bool hot = idx == hovered, playing = proc.loopPlaying() && proc.melPlaying == idx;
        r = r.reduced (4);
        g.setColour (t.glass.withMultipliedAlpha (isCentre ? 1.4f : 1.0f)); g.fillRoundedRectangle (r, 10);
        g.setColour (isCentre || playing ? t.accent : hot ? t.accent.withAlpha (0.7f) : t.text.withAlpha (0.25f));
        g.drawRoundedRectangle (r, 10, isCentre ? 2.0f : 1.3f);
        g.setColour (t.text); g.setFont (kk::modern::font (isCentre ? 17.0f : 13.0f, true, 0.06f));
        g.drawText (m.name, r.reduced (12, 6).removeFromTop (22), Justification::centredLeft);
        g.setColour (kk::accentText()); g.setFont (kk::modern::font (10.5f, true, 0.18f));
        g.drawText (playing ? String ("PLAYING") : m.how, r.reduced (12, 6).removeFromTop (22), Justification::centredRight);
        auto roll = r.reduced (12, 8).withTrimmedTop (24).withTrimmedBottom (isCentre ? 22 : 4);
        g.setColour (t.well); g.fillRoundedRectangle (roll, 4);
        if (m.notes.empty()) return;
        int lo = 127, hi = 0; for (auto& n : m.notes) { lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); }
        lo -= 1; hi += 1;
        const float beats = m.beats(), rows = (float) std::max (6, hi - lo + 1);
        for (int b = 0; b <= m.bars; ++b) { g.setColour (t.text.withAlpha (b % 4 == 0 ? 0.2f : 0.07f)); g.fillRect (roll.getX() + roll.getWidth() * (float) b * 4.0f / beats, roll.getY(), 1.0f, roll.getHeight()); }
        const float now = playing ? proc.loopBeat.load() : -1.0f;
        for (auto& n : m.notes)
        {
            const float x = roll.getX() + roll.getWidth() * n.start / beats, w = std::max (2.0f, roll.getWidth() * n.len / beats - 1.0f);
            const float y = roll.getBottom() - roll.getHeight() * (float) (n.pitch - lo + 1) / rows, h = std::max (2.5f, roll.getHeight() / rows - 1.0f);
            g.setColour (now >= n.start && now < n.start + n.len ? t.text : t.accent.withAlpha (0.55f + 0.4f * n.vel));
            g.fillRoundedRectangle (x, y, w, h, 1.5f);
        }
        if (now >= 0) { g.setColour (t.text.withAlpha (0.8f)); g.fillRect (roll.getX() + roll.getWidth() * now / beats, roll.getY(), 1.5f, roll.getHeight()); }
        if (isCentre)
        {
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.1f));
            g.drawText (String (kk::mel::keyName (m.key)) + " " + kk::mel::scaleName (m.scale) + "   plays: " + soundName + "   drag = MIDI  /  ALT + drag = WAV", r.reduced (12, 6).removeFromBottom (16), Justification::centredLeft);
        }
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        const auto now = Time::getMillisecondCounter();
        if (hovered >= 0 && hovered != centre && ! heard && now - hoverSince > 220) { heard = true; proc.melPlay (hovered); }
        playBtn.setButtonText (proc.loopPlaying() && proc.melPlaying == centre ? "STOP" : "PLAY");
        if (proc.loopPlaying()) repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton newBtn { lnf }, againBtn { lnf }, playBtn { lnf }, backBtn { lnf };
    ComboBox genreBox, keyBox;
    String soundName;
    int centre = -1, hovered = -1, downNode = -1;
    uint32 hoverSince = 0; bool heard = false, dragged = false;
};

class ChopPanel : public Component, public FileDragAndDropTarget, private Timer
{
public:
    bool isInterestedInFileDrag (const StringArray& f) override { for (auto& x : f) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true; return false; }
    void fileDragEnter (const StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const StringArray&) override { dropHover = false; repaint(); }
    void filesDropped (const StringArray& files, int, int) override
    {
        dropHover = false;
        for (auto& x : files) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) { loadSample (File (x)); break; }
        repaint();
    }
    void loadSample (const File& f)
    {
        const bool ok = proc.chopLoadFile (f);
        if (ok) proc.setPlayMode (KeysKillaProcessor::playChop);   // the keys / FL's notes play the chops now
        note = ok ? "loaded - cut at the hits.  Wheel = zoom,  drag in the wave = select a part,  SPACE = play / stop" : "could not read " + f.getFileName();
        mode = 0; sel = 0; selA = selB = -1; resetView(); refresh();
    }
    void browse()
    {
        chooser = std::make_unique<FileChooser> ("Load a sample", File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
        chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles,
                              [safe = Component::SafePointer<ChopPanel> (this)] (const FileChooser& fc)
                              { if (safe != nullptr && fc.getResult().existsAsFile()) safe->loadSample (fc.getResult()); });
    }
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
            sl.setColour (Slider::textBoxTextColourId, TC (0xffffffff));
            sl.onValueChange = [this] { pushFx(); };
            sl.updateText();
            addChildComponent (sl);
        };
        bar (pitchSl, -24, 24, 1, [] (double v) { return "PITCH " + String (v > 0 ? "+" : "") + String ((int) v); }, "Pitch of this slice in semitones (double-click = 0)");
        bar (volSl, 0, 2, 0.01, [] (double v) { return "VOL " + String (roundToInt (v * 100)) + "%"; }, "Volume of this slice");
        bar (panSl, -1, 1, 0.01, [] (double v) { return std::abs (v) < 0.01 ? String ("PAN C") : "PAN " + String (v < 0 ? "L" : "R") + String (roundToInt (std::abs (v) * 100)); }, "Pan of this slice");
        bar (fadeSl, 0.001, 0.2, 0.001, [] (double v) { return "FADE " + String (roundToInt (v * 1000)) + " ms"; }, "Fade in / out - no clicks when you trigger fast");
        pitchSl.setDoubleClickReturnValue (true, 0); volSl.setDoubleClickReturnValue (true, 1); panSl.setDoubleClickReturnValue (true, 0); fadeSl.setDoubleClickReturnValue (true, 0.004);
        revBtn.setButtonText ("REVERSE"); revBtn.framed = true; revBtn.setTooltip ("Play this slice backwards");
        revBtn.onClick = [this] { revBtn.selected = ! revBtn.selected; revBtn.repaint(); pushFx(); hitPad (sel); };
        addChildComponent (revBtn);
        flipBtn.setButtonText ("FLIP IT"); flipBtn.framed = true;
        flipBtn.setTooltip ("A new trap pattern from your chops - then drag it into FL (FLIP MIDI)");
        flipBtn.onClick = [this] { setView (1); if (proc.flips.empty()) proc.flipSeed(); };
        addChildComponent (flipBtn);
        dragFlip.makeFile = [this] { return proc.flipMidiFile (proc.flipCenter); };
        dragFlip.setTooltip ("Drag the middle flip into FL as MIDI - it plays the chops on this SAMPLER (C5 = slice 1)");
        addChildComponent (dragFlip);
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (keysBtn, "", "KEYS: the keyboard / FL piano roll plays the slices - C5 = slice 1, C#5 = slice 2 ...",
             [this] { setParamFromUi (proc, ID::playMode, keysOn() ? 0.0f : (float) KeysKillaProcessor::playChop); refresh(); });
        btn (bankAllBtn, "SAVE ALL", "Every slice into one of your MY SOUNDS folders", [this]
        {
            std::vector<kk::PairPtr> all;
            if (auto c = proc.chop.current()) for (int i = 0; i < c->numSlices(); ++i) all.push_back (sliceSound (i));
            saveToFolderMenu (proc, all, &bankAllBtn, [safe = Component::SafePointer<ChopPanel> (this)] (String msg) { if (safe != nullptr) { safe->note = msg; safe->repaint(); } });
        });
        btn (backBtn, "LOAD SAMPLE", "Load a sample from your computer (or drag a WAV / MP3 from FL's browser onto the waveform)", [this] { browse(); });
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
        // v0.35: CLEAR + the selection tools (work on the selected part - nothing selected = the whole sample)
        btn (clearBtn, "CLEAR", "Remove the sample from the SAMPLER", [this] { proc.chopClear(); selA = selB = -1; note = "empty - drop a new sample"; resetView(); refresh(); });
        btn (playSelBtn, "PLAY PART", "Play the selected part (nothing selected = the whole sample).  SPACE = play / stop", [this] { playSelection(); });
        btn (loopSelBtn, "LOOP OFF", "LOOP ON: PLAY PART repeats the part, and DRAG PART / SAVE PART give a clean loop.  LOOP OFF: once", [this] { loopOn = ! loopOn; loopSelBtn.selected = loopOn; loopSelBtn.setButtonText (loopOn ? "LOOP ON" : "LOOP OFF"); loopSelBtn.repaint(); if (proc.chop.regionPlaying()) playSelection(); });
        btn (savePartBtn, "SAVE PART", "Save the selected part (or the selected slice) as a sound into your folders / sound kits", [this]
        {
            saveToFolderMenu (proc, { partSound() }, &savePartBtn, [safe = Component::SafePointer<ChopPanel> (this)] (String msg) { if (safe != nullptr) { safe->note = msg; safe->repaint(); } });
        });
        btn (padsTab, "PADS", "The 16 pads: click = play a slice, right-click = save it, drag a pad = the slice into FL as a WAV", [this] { setView (0); });
        btn (flipsTab, "FLIPS", "FLIPS: beats from your chops that evolve - hover = hear, click = grow", [this] { setView (1); });
        btn (melTab, "MELODY", "MELODY: the selected chop becomes an instrument and plays melodies that grow (genre, key, tempo) - drag MIDI or WAV", [this] { setView (2); });
        btn (monoBtn, "MONO PADS", "MONO PADS on: a new pad stops the one that plays (no overlapping).  Off: pads ring out over each other", [this]
        { proc.chopChoke = ! proc.chopChoke.load(); monoBtn.selected = proc.chopChoke.load(); monoBtn.repaint(); });
        monoBtn.selected = proc.chopChoke.load();
        flipView = std::make_unique<FlipView> (proc, lnf);
        addChildComponent (*flipView);
        melView = std::make_unique<SamplerMelodyView> (proc, lnf);
        melView->soundToUse = [this] { return partSound(); };
        melView->onNote = [this] (const String& m) { note = m; repaint(); };
        addChildComponent (*melView);
        dragMelMidi.makeFile = [this] { return proc.melExport (melView->centreMelody()); };
        dragMelMidi.setTooltip ("Drag the middle melody into FL as MIDI (put it on EVOLVE - it plays this sound)");
        addChildComponent (dragMelMidi);
        dragMelWav.makeFile = [this] { return proc.melExportWav (melView->centreMelody()); };
        dragMelWav.setTooltip ("Drag the middle melody into FL as audio - played by this sound, at the project tempo");
        addChildComponent (dragMelWav);
        dragFlipWav.makeFile = [this] { return proc.flipWavFile (proc.flipCenter); };
        dragFlipWav.setTooltip ("Drag the middle flip into FL as audio (a WAV at the project tempo)");
        addChildComponent (dragFlipWav);
        btn (toABtn, "> PARENT A", "The selected part becomes PARENT A in BREED LAB - breed it", [this] { toParent (0); });
        btn (toBBtn, "> PARENT B", "The selected part becomes PARENT B in BREED LAB - breed it", [this] { toParent (1); });
        btn (mutateBtn, "MUTATE", "MUTATE: a few 1/8 pieces get reversed, stuttered, filtered - the melody stays. Again = a new one", [this] { doMutate (false); });
        btn (killBtn, "KILL", "KILL: most 1/8 pieces get a move - backwards, stutters, tape stops, octaves - the groove stays. Again = a new one", [this] { doMutate (true); });
        btn (undoBtn, "UNDO", "Back to the sample before the last MUTATE / KILL", [this] { if (proc.chopUndoMutate()) { note = "undone"; peaksKey = {}; repaint(); } });
        btn (allBtn, "SELECT ALL", "Select the whole sample (again = no selection)", [this] { auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return; if (hasSel()) selA = selB = -1; else { selA = 0; selB = c->src->getNumSamples(); } repaint(); refreshSelButtons(); });
        dragSel.makeFile = [this] { auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return File(); return hasSel() ? proc.exportChopRegion (selA, selB, loopOn) : proc.exportChopRegion (0, c->src->getNumSamples(), loopOn); };
        dragSel.setTooltip ("Drag the selected part into FL Studio as a WAV (LOOP on = a clean loop)");
        addAndMakeVisible (dragSel);
        setOpaque (true);
        startTimerHz (30);
    }
    ~ChopPanel() override { stopTimer(); }
    void visibilityChanged() override { if (isVisible()) refresh(); }

    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        pageBackdrop (g, *this);
        g.setColour (TC (0xff1a1838)); g.fillRect (0, 0, getWidth(), 64);
        g.setColour (s.accent); g.fillEllipse (22, 26, 10, 10);
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (26.0f, Font::bold)).withExtraKerningFactor (0.08f));
        g.drawText ("SAMPLER", 40, 12, 260, 40, Justification::centredLeft);
        auto c = proc.chop.current();
        g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (12.0f, Font::bold)));
        g.drawText (c != nullptr ? c->name.toUpperCase() + "   " + String (c->numSlices()) + " SLICES" : String(), 42, 44, 400, 16, Justification::centredLeft);
        // waveform with slices
        const auto w = wave.toFloat();
        g.setGradientFill (ColourGradient (TC (0xff1a1638), 0, w.getY(), TC (0xff0b0a1c), 0, w.getBottom(), false)); g.fillRoundedRectangle (w, 8);
        g.setGradientFill (ColourGradient (TC (0xffffb020).withAlpha (dropHover ? 1.0f : 0.7f), w.getX(), 0, TC (0xffff6a3d).withAlpha (dropHover ? 1.0f : 0.5f), w.getRight(), 0, false));
        g.drawRoundedRectangle (w, 8, dropHover ? 3.0f : 1.6f);
        if (c == nullptr || c->src == nullptr)
        {
            g.setColour (TC (0xffffffff).withAlpha (0.9f)); g.setFont (Font (FontOptions (26.0f, Font::bold)).withExtraKerningFactor (0.08f));
            g.drawText (dropHover ? "DROP IT" : "DROP A SAMPLE HERE", wave.withTrimmedBottom (40), Justification::centred);
            g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (14.0f)));
            g.drawText ("a loop, a song, a vocal, a one-shot - WAV, MP3, FLAC, AIFF from FL's browser or a folder  -  or click LOAD SAMPLE",
                        wave.withTrimmedTop (50), Justification::centred);
            g.setColour (TC (0xffffb020)); g.setFont (Font (FontOptions (13.0f, Font::bold)));
            g.drawText (note, wave.withTrimmedTop (wave.getHeight() - 40), Justification::centred);
            return;
        }
        const int len = c->src->getNumSamples();
        if (viewLen <= 0 || viewStart + viewLen > len) resetView();
        updatePeaks();
        g.saveState();
        g.reduceClipRegion (wave.reduced (2));
        auto xOf = [&] (int smp) { return w.getX() + 10 + (w.getWidth() - 20) * (float) (smp - viewStart) / (float) std::max (1, viewLen); };
        for (int i = 0; i < c->numSlices(); ++i)
        {
            const float x0 = xOf (c->sliceStart (i)), x1 = xOf (c->sliceEnd (i));
            g.setColour (i == sel ? s.accent.withAlpha (0.22f) : (i % 2 ? TC (0xff17153a) : TC (0xff131130)));
            g.fillRect (x0, w.getY() + 2, x1 - x0, w.getHeight() - 4);
        }
        const float mid = w.getCentreY(), half = w.getHeight() * 0.42f;
        for (int k = 0; k < (int) peaks.size(); ++k)
        {
            const float x = w.getX() + 10 + (w.getWidth() - 20) * (float) k / (float) peaks.size();
            const int smp = viewStart + (int) ((juce::int64) viewLen * k / (juce::int64) peaks.size());
            int si = 0; while (si + 1 < c->numSlices() && c->sliceStart (si + 1) <= smp) ++si;
            g.setColour (si == sel ? s.accent : TC (0xffc9ced6));
            g.drawVerticalLine ((int) x, mid - peaks[(size_t) k] * half, mid + peaks[(size_t) k] * half);
        }
        if (hasSel())
        {
            const float x0 = jlimit (w.getX(), w.getRight(), xOf (selA)), x1 = jlimit (w.getX(), w.getRight(), xOf (selB));
            g.setColour (s.accent.withAlpha (0.22f)); g.fillRect (x0, w.getY() + 2, x1 - x0, w.getHeight() - 4);
            g.setColour (s.accent); g.drawVerticalLine ((int) x0, w.getY() + 2, w.getBottom() - 2); g.drawVerticalLine ((int) x1, w.getY() + 2, w.getBottom() - 2);
            g.setFont (Font (FontOptions (11.0f, Font::bold)));
            g.drawText (String ((double) (selB - selA) / c->rate, 2) + " s" + (loopOn ? "  LOOP" : ""), Rectangle<float> (x0 + 4, w.getY() + 4, 140, 14), Justification::centredLeft);
        }
        for (int i = 0; i < c->numSlices(); ++i)
        {
            const float x = xOf (c->sliceStart (i));
            if (x < w.getX() || x > w.getRight()) continue;
            g.setColour (i == hoverMark ? TC (0xffffffff) : TC (0xffffc23d));
            g.fillRect (x - 1.0f, w.getY() + 2, i == 0 ? 1.0f : 2.0f, w.getHeight() - 4);
            if (i > 0) { Path tri; tri.addTriangle (x - 7, w.getY() + 2, x + 7, w.getY() + 2, x, w.getY() + 12); g.fillPath (tri); }
            g.setColour (TC (0xffffffff).withAlpha (0.85f)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
            g.drawText (String (i + 1) + "  " + MidiMessage::getMidiNoteName (kk::ChopLab::firstNote + i, true, true, 5), Rectangle<float> (x + 4, w.getBottom() - 18, 80, 14), Justification::centredLeft);
        }
        if (const float ph = proc.chop.playhead(); ph >= 0)
        {
            g.setColour (TC (0xffffffff)); g.drawVerticalLine ((int) xOf ((int) (ph * (float) len)), w.getY(), w.getBottom());
        }
        g.restoreState();
        g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
        // scroll bar: where you are in the sample (drag it)
        {
            const auto sb = scrollBar().toFloat();
            g.setColour (kk::theme().well); g.fillRoundedRectangle (sb, 4);
            const float x0 = sb.getX() + sb.getWidth() * (float) viewStart / (float) len, x1 = sb.getX() + sb.getWidth() * (float) (viewStart + viewLen) / (float) len;
            g.setColour (s.accent.withAlpha (viewLen < len ? 0.75f : 0.3f)); g.fillRoundedRectangle (x0, sb.getY() + 1, std::max (6.0f, x1 - x0), sb.getHeight() - 2, 3);
        }
        g.setColour (kk::theme().dim); g.setFont (Font (FontOptions (11.0f, Font::bold)));
        g.drawText ("WHEEL = ZOOM   SHIFT+WHEEL = SCROLL   DRAG IN THE WAVE = SELECT A PART   CLICK = PLAY A SLICE   DOUBLE-CLICK = NEW CUT   DRAG A MARKER = MOVE   RIGHT-CLICK MARKER = REMOVE",
                    scrollBar().withY (scrollBar().getBottom() + 2).withHeight (14), Justification::centredLeft);
        // selected slice
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (18.0f, Font::bold)));
        if (sel < c->numSlices())
            g.drawText ("SLICE " + String (sel + 1) + "  " + MidiMessage::getMidiNoteName (kk::ChopLab::firstNote + sel, true, true, 5) + "  "
                        + String ((double) (c->sliceEnd (sel) - c->sliceStart (sel)) / c->rate, 2) + " s", selRow.withWidth (240), Justification::centredLeft);
        g.setColour (kk::accentText()); g.setFont (Font (FontOptions (13.0f, Font::bold)));
        g.drawText (note, selRow.withY (selRow.getBottom() + 6).withHeight (18), Justification::centredLeft);
        // MPC pads (16)
        for (int i = 0; i < 16 && viewMode == 0; ++i)
        {
            const auto r = pad (i).toFloat();
            const bool has = i < c->numSlices();
            const float lit = padLit[(size_t) i];
            g.setColour (has ? TC (0xff1f1d3e).interpolatedWith (s.accent, 0.25f * (i == sel) + 0.6f * lit) : TC (0xff16152e));
            g.fillRoundedRectangle (r, 8);
            g.setColour (has ? TC (0xff4a4478) : TC (0xff1e1c3c)); g.drawRoundedRectangle (r, 8, 1.4f);
            if (! has) continue;
            g.setColour (TC (0xffffffff).withAlpha (0.9f)); g.setFont (Font (FontOptions (15.0f, Font::bold)));
            g.drawText (String (i + 1), r.reduced (10, 6).toNearestInt(), Justification::topLeft);
            g.setColour (TC (0xffb4aed8)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
            g.drawText (MidiMessage::getMidiNoteName (kk::ChopLab::firstNote + i, true, true, 5), r.reduced (10, 6).toNearestInt(), Justification::topRight);
        }
    }
    void resized() override
    {
        const int W = getWidth(), H = getHeight();
        int x = 300;
        for (int i = 0; i < (int) modeBtns.size(); ++i) { const int bw = i == 0 ? 116 : i == 1 ? 74 : 52; modeBtns[(size_t) i]->setBounds (x, 16, bw, 32); x += bw + 5; }
        keysBtn.setBounds (x + 10, 14, 220, 36); keysBtn.setVisible (false);   // v0.35: the SAMPLER page plays the chops by itself
        backBtn.setBounds (W - 196, 14, 182, 36);
        clearBtn.setBounds (W - 196 - 98, 14, 90, 36);
        bankAllBtn.setBounds (clearBtn.getX() - 146, 14, 136, 36);
        wave = { 14, 78, W - 28, std::max (180, H - 78 - 410) };
        // selection tools row
        {
            auto row = Rectangle<int> (14, wave.getBottom() + 34, W - 28, 36);
            for (auto* b : { &allBtn, &playSelBtn, &loopSelBtn }) { b->setBounds (row.removeFromLeft (b == &allBtn ? 112 : 100)); row.removeFromLeft (6); }
            dragSel.setBounds (row.removeFromLeft (180)); row.removeFromLeft (6);
            savePartBtn.setBounds (row.removeFromLeft (104)); row.removeFromLeft (14);
            for (auto* b : { &toABtn, &toBBtn }) { b->setBounds (row.removeFromLeft (112)); row.removeFromLeft (6); }
            row.removeFromLeft (14);
            for (auto* b : { &mutateBtn, &killBtn, &undoBtn }) { b->setBounds (row.removeFromLeft (b == &undoBtn ? 76 : 110)); row.removeFromLeft (6); }
        }
        selRow = { 18, wave.getBottom() + 34 + 36 + 12, W - 36, 40 };
        int cx = selRow.getX() + 250;
        revBtn.setBounds (cx, selRow.getY() + 2, 92, 34); cx += 100;
        for (auto* sl : { &pitchSl, &volSl, &panSl, &fadeSl }) { sl->setBounds (cx, selRow.getY() + 2, 118, 34); cx += 124; }
        dragWav.setBounds (W - 18 - 180, selRow.getY() - 4, 180, 46);
        bankBtn.setBounds (dragWav.getX() - 136, selRow.getY(), 128, 38);
        pairBtn.setBounds (bankBtn.getX() - 136, selRow.getY(), 128, 38); pairBtn.setVisible (false);   // v0.35: > PARENT A / B above does it
        padsTab.setBounds (14, selRow.getBottom() + 28, 90, 32);
        flipsTab.setBounds (110, selRow.getBottom() + 28, 90, 32);
        melTab.setBounds (206, selRow.getBottom() + 28, 90, 32);
        padArea = { 14, selRow.getBottom() + 68, W - 28 - 230, H - selRow.getBottom() - 80 };
        flipView->setBounds (padArea.withTop (selRow.getBottom() + 28));
        melView->setBounds (padArea.withTop (selRow.getBottom() + 28));
        const int colX = padArea.getRight() + 14, colW = W - 14 - colX;
        monoBtn.setBounds (colX, padArea.getY(), colW, 38);
        dragMidi.setBounds (colX, padArea.getY() + 48, colW, 46);
        dragFlip.setBounds (colX, padArea.getY() + 48, colW, 46);
        dragFlipWav.setBounds (colX, padArea.getY() + 100, colW, 46);
        dragMelMidi.setBounds (colX, padArea.getY() + 48, colW, 46);
        dragMelWav.setBounds (colX, padArea.getY() + 100, colW, 46);
        flipBtn.setBounds (0, 0, 0, 0);
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
        for (int i = 0; i < 16 && viewMode == 0; ++i)
            if (i < c->numSlices() && pad (i).contains (e.getPosition()))
            {
                sel = i; loadFx();
                if (e.mods.isPopupMenu()) { padMenu (i); repaint(); return; }
                hitPad (i); padDown = i; repaint(); return;
            }
        if (! wave.contains (e.getPosition())) return;
        const int m = markAt (e.position);
        if (m > 0 && e.mods.isPopupMenu()) { auto mk = c->marks; mk.erase (mk.begin() + m); proc.chop.setMarks (mk); refresh(); return; }
        if (m > 0) { dragMark = m; return; }
        const int smp = sampleAt (e.position.x);
        if (e.getNumberOfClicks() > 1) { auto mk = c->marks; mk.push_back (smp); proc.chop.setMarks (mk); refresh(); return; }
        pressSmp = smp; selecting = false; inWave = true;
    }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& wd) override
    {
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr || ! wave.contains (e.getPosition())) return;
        const int len = c->src->getNumSamples();
        if (e.mods.isShiftDown() || std::abs (wd.deltaX) > std::abs (wd.deltaY))   // scroll
        {
            const float d = std::abs (wd.deltaX) > std::abs (wd.deltaY) ? wd.deltaX : wd.deltaY;
            viewStart = jlimit (0, std::max (0, len - viewLen), viewStart - (int) (d * (float) viewLen * 0.5f));
        }
        else   // zoom around the mouse
        {
            const int at = sampleAt (e.position.x);
            const float f = wd.deltaY > 0 ? 0.8f : 1.25f;
            const int minLen = std::max (256, (int) (c->rate * 0.02));
            const int nl = jlimit (minLen, len, (int) ((float) viewLen * f));
            const float rel = (float) (at - viewStart) / (float) std::max (1, viewLen);
            viewLen = nl; viewStart = jlimit (0, std::max (0, len - viewLen), at - (int) (rel * (float) nl));
        }
        repaint();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (padDown >= 0)   // drag a pad = that slice into FL as a WAV
        {
            if (e.getDistanceFromDragStart() > 8)
            {
                const int i = padDown; padDown = -1;
                const auto f = proc.chop.exportSlice (i);
                if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
            }
            return;
        }
        if (draggingBar || (dragMark <= 0 && scrollBar().contains (e.getMouseDownPosition())))
        {
            auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return;
            draggingBar = true;
            const auto sb = scrollBar();
            const int len = c->src->getNumSamples();
            const int centre = (int) ((e.position.x - (float) sb.getX()) / (float) sb.getWidth() * (float) len);
            viewStart = jlimit (0, std::max (0, len - viewLen), centre - viewLen / 2);
            repaint(); return;
        }
        if (dragMark <= 0)
        {
            if (! inWave || e.getDistanceFromDragStart() < 4) return;
            selecting = true;
            const int smp = sampleAt (e.position.x);
            selA = std::min (pressSmp, smp); selB = std::max (pressSmp, smp);
            refreshSelButtons(); repaint (wave); return;
        }
        auto c = proc.chop.current(); if (c == nullptr) return;
        auto mk = c->marks;
        if (dragMark >= (int) mk.size()) return;
        const int lo = mk[(size_t) dragMark - 1] + (int) (c->rate * 0.02), hi = dragMark + 1 < (int) mk.size() ? mk[(size_t) dragMark + 1] - (int) (c->rate * 0.02) : c->src->getNumSamples() - (int) (c->rate * 0.02);
        mk[(size_t) dragMark] = jlimit (lo, std::max (lo, hi), sampleAt (e.position.x));
        proc.chop.setMarks (mk);
        repaint (wave);
    }
    void mouseUp (const MouseEvent& e) override
    {
        padDown = -1;
        if (draggingBar) { draggingBar = false; return; }
        if (dragMark > 0) refresh();
        else if (inWave && ! selecting && ! e.mods.isPopupMenu() && e.getNumberOfClicks() == 1)   // a click: play that slice
        {
            if (auto c = proc.chop.current(); c != nullptr && c->src != nullptr)
            {
                const int smp = sampleAt (e.position.x);
                if (! (hasSel() && smp >= selA && smp <= selB)) { selA = selB = -1; refreshSelButtons(); }
                int si = 0; while (si + 1 < c->numSlices() && c->sliceStart (si + 1) <= smp) ++si;
                sel = si; loadFx(); hitPad (si); repaint();
            }
        }
        dragMark = -1; inWave = false; selecting = false;
    }
public:
    // v0.35: SPACE on the SAMPLER page: stop whatever plays - or play the selection / the selected slice
    void debugSelect (int a0, int a1, int v0, int vl) { selA = a0; selB = a1; viewStart = v0; viewLen = vl; peaksKey = {}; refreshSelButtons(); repaint(); }
    void debugMelody() { setView (2); proc.melPlay (-1); }
    void debugFlips() { setView (1); proc.flipGrow (proc.flipCenter, true, 0.5f); }
    void spacePressed()
    {
        if (proc.chop.anyPlaying()) { proc.chop.stopAll(); repaint(); return; }
        if (hasSel()) playSelection(); else hitPad (sel);
    }
private:
    void hitPad (int i) { proc.setPlayMode (KeysKillaProcessor::playChop); proc.chopPad = i; }
    void padMenu (int i)
    {
        PopupMenu m;
        m.addSectionHeader ("SLICE " + String (i + 1));
        m.addItem (1, "Save this slice as a sound...");
        m.addItem (2, "This slice > PARENT A (BREED LAB)");
        m.addItem (3, "This slice > PARENT B (BREED LAB)");
        m.addItem (4, "Play it");
        m.showMenuAsync (PopupMenu::Options(), [this, i, safe = Component::SafePointer<ChopPanel> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1) saveToFolderMenu (proc, { sliceSound (i) }, this, [safe] (String msg) { if (safe != nullptr) { safe->note = msg; safe->repaint(); } });
            if (r == 2 || r == 3) { auto c = proc.chop.current(); if (c != nullptr) { note = proc.chopRegionToParent (c->sliceStart (i), c->sliceEnd (i), r - 2) ? "slice " + String (i + 1) + " is in BREED LAB" : String(); repaint(); } }
            if (r == 4) hitPad (i);
        });
    }
    void setView (int v)
    {
        viewMode = v;
        padsTab.selected = v == 0; flipsTab.selected = v == 1; melTab.selected = v == 2; padsTab.repaint(); flipsTab.repaint(); melTab.repaint();
        const bool has = proc.chop.hasSource();
        flipView->setVisible (v == 1 && has);
        melView->setVisible (v == 2 && has);
        padsTab.toFront (false); flipsTab.toFront (false); melTab.toFront (false);   // never under the FLIPS / MELODY views
        dragMidi.setVisible (false);
        dragFlip.setVisible (v == 1 && has); dragFlipWav.setVisible (v == 1 && has);   // v0.42: the flip as MIDI again (C5 = chop 1)
        dragMelMidi.setVisible (v == 2 && has); dragMelWav.setVisible (v == 2 && has);
        if (v == 1 && has && proc.flips.empty()) proc.flipSeed();
        if (v == 0 && proc.flipOn.load()) proc.flipPlay (-1);
        if (v != 2 && has && viewWasMelody) proc.setPlayMode (KeysKillaProcessor::playChop);   // back from MELODY: the keys play the pads again
        viewWasMelody = v == 2;
        repaint();
    }
    kk::PairPtr partSound() const
    {
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return {};
        const int a0 = hasSel() ? selA : c->sliceStart (sel), a1 = hasSel() ? selB : c->sliceEnd (sel);
        auto b = proc.chopRegion (a0, a1, loopOn);
        if (b.getNumSamples() < 64) return {};
        return kk::PairLab::fromBuffer (b, c->rate, c->rate, c->name + (hasSel() ? " part" : " chop " + String (sel + 1)));
    }
    bool hasSel() const { return selA >= 0 && selB > selA + 64; }
    void playSelection()
    {
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return;
        if (hasSel()) proc.chop.playRegion (selA, selB, loopOn);
        else proc.chop.playRegion (0, c->src->getNumSamples(), loopOn);
    }
    void toParent (int slot)
    {
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return;
        const int a0 = hasSel() ? selA : c->sliceStart (sel), a1 = hasSel() ? selB : c->sliceEnd (sel);
        note = proc.chopRegionToParent (a0, a1, slot) ? String ("in BREED LAB as PARENT ") + (slot == 0 ? "A" : "B") + " - open BREED LAB and press BREED" : String ("could not use it");
        repaint();
    }
    void doMutate (bool kill)
    {
        if (! proc.chopMutate (hasSel() ? selA : 0, hasSel() ? selB : 0, kill)) return;
        note = String (kill ? "KILLED" : "MUTATED") + (hasSel() ? " the selection" : " the whole sample") + " - again = a new one, UNDO = back";
        peaksKey = {}; refreshSelButtons(); repaint();
        playSelection();
    }
    void refreshSelButtons()
    {
        const bool any = proc.chop.current() != nullptr && proc.chop.current()->src != nullptr;
        for (auto* b : { &playSelBtn, &loopSelBtn, &toABtn, &toBBtn, &mutateBtn, &killBtn, &undoBtn, &allBtn, &clearBtn, &savePartBtn, &padsTab, &flipsTab, &monoBtn }) b->setVisible (any);
        dragSel.setVisible (any);
        mutateBtn.setButtonText (hasSel() ? "MUTATE" : "MUTATE ALL"); killBtn.setButtonText (hasSel() ? "KILL" : "KILL ALL");
        undoBtn.setEnabled (proc.chopCanUndo());
        allBtn.setButtonText (hasSel() ? "UNSELECT" : "SELECT ALL");
    }
    void resetView() { auto c = proc.chop.current(); viewStart = 0; viewLen = c != nullptr && c->src != nullptr ? c->src->getNumSamples() : 0; peaksKey = {}; }
    Rectangle<int> scrollBar() const { return { wave.getX() + 10, wave.getBottom() + 4, wave.getWidth() - 20, 10 }; }
    void updatePeaks()
    {
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr) return;
        const int cols = std::max (200, wave.getWidth() - 20);
        const auto key = std::make_tuple ((const void*) c->src.get(), viewStart, viewLen, cols);
        if (key == peaksKey) return;
        peaksKey = key;
        peaks.assign ((size_t) cols, 0.0f);
        const auto& a = *c->src;
        const int len = a.getNumSamples();
        for (int k = 0; k < cols; ++k)
        {
            const int s0 = jlimit (0, len - 1, viewStart + (int) ((juce::int64) viewLen * k / cols));
            const int s1 = jlimit (s0 + 1, len, viewStart + (int) ((juce::int64) viewLen * (k + 1) / cols));
            const int step = std::max (1, (s1 - s0) / 256);   // long views: sample every n-th value (fast)
            float m = 0;
            for (int ch = 0; ch < a.getNumChannels(); ++ch) { const float* x = a.getReadPointer (ch); for (int i = s0; i < s1; i += step) m = std::max (m, std::abs (x[i])); }
            peaks[(size_t) k] = m;
        }
        float mx = 1.0e-6f; for (auto v : peaks) mx = std::max (mx, v);   // normalised to the whole file's view
        for (auto& v : peaks) v /= std::max (mx, 0.05f);
    }
    std::tuple<const void*, int, int, int> peaksKey {};
    int viewStart = 0, viewLen = 0, selA = -1, selB = -1, pressSmp = 0;
    bool selecting = false, inWave = false, draggingBar = false, loopOn = false;
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
        const bool has = proc.chop.current() != nullptr && proc.chop.current()->src != nullptr;   // slice controls only with a sample
        for (Component* c : { (Component*) &pitchSl, (Component*) &volSl, (Component*) &panSl, (Component*) &fadeSl, (Component*) &revBtn,
                              (Component*) &bankBtn, (Component*) &dragWav, (Component*) &bankAllBtn })
            c->setVisible (has);
        setView (viewMode);
        peaksKey = {};
        refreshSelButtons();
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
        return jlimit (0, c->src->getNumSamples() - 1, viewStart + (int) ((x - w.getX() - 10) / (w.getWidth() - 20) * (float) viewLen));
    }
    int markAt (Point<float> p) const
    {
        auto c = proc.chop.current(); if (c == nullptr || c->src == nullptr || ! wave.toFloat().contains (p)) return -1;
        const auto w = wave.toFloat();
        for (int i = 1; i < c->numSlices(); ++i)
            if (std::abs (w.getX() + 10 + (w.getWidth() - 20) * (float) (c->sliceStart (i) - viewStart) / (float) std::max (1, viewLen) - p.x) < 6.0f) return i;
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
        if (loopSelBtn.selected != loopOn) { loopSelBtn.selected = loopOn; loopSelBtn.repaint(); }
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    std::vector<std::unique_ptr<HotButton>> modeBtns;
    HotButton keysBtn { lnf }, bankAllBtn { lnf }, backBtn { lnf }, pairBtn { lnf }, bankBtn { lnf };
    std::unique_ptr<FileChooser> chooser;
    bool dropHover = false;
    DragFileButton dragWav { "DRAG SLICE WAV", TC (0xff36ff6a) }, dragMidi { "DRAG CHOP MIDI", TC (0xff36ff6a) }, dragFlip { "DRAG FLIP MIDI", TC (0xff36ff6a) };
    Slider pitchSl, volSl, panSl, fadeSl;
    HotButton revBtn { lnf }, flipBtn { lnf };
    HotButton savePartBtn { lnf }, padsTab { lnf }, flipsTab { lnf }, monoBtn { lnf };
    std::unique_ptr<FlipView> flipView;
    std::unique_ptr<SamplerMelodyView> melView;
    HotButton melTab { lnf }; bool viewWasMelody = false;
    DragFileButton dragMelMidi { "DRAG MELODY MIDI", TC (0xff36ff6a) }, dragMelWav { "DRAG MELODY WAV", TC (0xff36ff6a) };
    DragFileButton dragFlipWav { "DRAG FLIP WAV", TC (0xff36ff6a) };
    int viewMode = 0;
    HotButton clearBtn { lnf }, playSelBtn { lnf }, loopSelBtn { lnf }, toABtn { lnf }, toBBtn { lnf }, mutateBtn { lnf }, killBtn { lnf }, undoBtn { lnf }, allBtn { lnf };
    DragFileButton dragSel { "DRAG PART WAV", TC (0xff36ff6a) };
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
        for (auto* sl : { &pitchSl, &volSl, &panSl, &fadeSl }) sl->updateText();
        revBtn.selected = f.rev; revBtn.repaint();
        loadingFx = false;
    }
    Rectangle<int> wave, selRow, padArea;
    std::vector<float> peaks;
    std::array<float, 16> padLit {};
    String note;
    int sel = 0, mode = 0, dragMark = -1, hoverMark = -1, padDown = -1;
    bool wasPlaying = false;
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
static std::vector<DrumTheme> makeDrumThemes()
{
    return {
        { "808", "SUB BOOSTER  -  DROP YOUR 808, MAKE IT KNOCK, WRITE THE 808 LINE, DRAG IT ALL BACK INTO FL", "DROP YOUR 808 HERE", "808 PATTERNS",
          TC (0xff1d1a3c), TC (0xff0a0a1a), TC (0xffff5a1f), TC (0xffffc23d), TC (0xffeef0f2), { "SIMPLE", "SLIDES", "SOFT", "HARD" } },
        { "SNARE / CLAP", "CRACK LAB  -  DROP A SNARE OR CLAP, SHARPEN IT, GENERATE TRAP SNARE ROLLS", "DROP YOUR SNARE / CLAP HERE", "SNARE ROLLS",
          TC (0xff1b1a3a), TC (0xff0a0a1a), TC (0xff1fe0ff), TC (0xffff3fd2), TC (0xffeef0f2), { "SIMPLE", "TRIPLET", "BUSY", "BUILD-UP" } },
        { "HI-HAT", "HAT FACTORY  -  DROP A HI-HAT, MAKE IT SHINE, TRAP ROLLS IN THE PIANO ROLL", "DROP YOUR HI-HAT HERE", "HI-HAT ROLLS",
          TC (0xff1f1a3e), TC (0xff0a0a1a), TC (0xffffd23f), TC (0xffb070ff), TC (0xffeef0f2), { "SIMPLE", "TRIPLET", "BUSY", "CRAZY" } },
        { "KICK", "KICK LAB  -  DROP A KICK, MAKE IT HIT, SAVE IT INTO YOUR DRUM KIT", "DROP YOUR KICK HERE", "KICK PATTERNS",
          TC (0xff1d1a3c), TC (0xff0a0a1a), TC (0xffff3b30), TC (0xffff9f0a), TC (0xffeef0f2), { "SIMPLE", "BUSY", "BOUNCE", "HALF-TIME" } },
        { "OPEN HAT", "OPEN HAT  -  DROP AN OPEN HAT / CRASH, SHAPE ITS TAIL, SAVE IT INTO YOUR DRUM KIT", "DROP YOUR OPEN HAT HERE", "OPEN HAT PATTERNS",
          TC (0xff1f1a3e), TC (0xff0a0a1a), TC (0xffffe066), TC (0xff64d2ff), TC (0xffeef0f2), { "OFFBEAT", "SPARSE", "SYNCOPATED", "BUSY" } },
        { "PERC", "PERCUSSION  -  RIMS, TOMS, SHAKERS, BONGOS ... SAVE THEM INTO YOUR DRUM KIT", "DROP YOUR PERC HERE", "PERC PATTERNS",
          TC (0xff1b1a3a), TC (0xff0a0a1a), TC (0xff30d158), TC (0xffffd60a), TC (0xffeef0f2), { "RIMS", "TRIPLET", "BOUNCE", "SHAKER" } },
        { "FX", "DRUM FX  -  RISERS, IMPACTS, VOX TAGS, REVERSES ... SAVE THEM INTO YOUR DRUM KIT", "DROP YOUR FX HERE", "FX PATTERNS",
          TC (0xff211a40), TC (0xff0a0a1a), TC (0xffbf5af2), TC (0xff64d2ff), TC (0xffeef0f2), { "INTRO", "PHRASE", "RISER", "STUTTER" } } };
}
static const DrumTheme& drumTheme (int d)   // v0.34: one table per theme (GLASS / NIGHT)
{
    static std::vector<DrumTheme> t[2];
    auto& v = t[kk::themeIndex() == 1 ? 1 : 0];
    if (v.empty()) v = makeDrumThemes();
    return v[(size_t) jlimit (0, (int) v.size() - 1, d)];
}

class ThemedKnob : public Slider
{
public:
    ThemedKnob (const DrumTheme& t) : Slider (RotaryHorizontalVerticalDrag, NoTextBox), th (t)
    {
        setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    }
    String label;
    void paint (Graphics& g) override
    {
        kk::cell::draw (g, getLocalBounds().toFloat(), (float) valueToProportionOfLength (getValue()), false, th.accent, th.accent2, label, isMouseOverOrDragging(), false, 20.0f);
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
        g.setColour (TC (0xff0b0a18)); g.fillRoundedRectangle (all, 8);
        g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (all.reduced (0.5f), 8, 1.2f);
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
            g.setColour (semi == 0 ? th.accent.withAlpha (0.12f) : TC (black ? 0xff12112a : 0xff17152f)); g.fillRect (gr.getX(), y, gr.getWidth(), rh);
            g.setColour (TC (0xff221f40)); g.drawHorizontalLine ((int) y, gr.getX(), gr.getRight());
            if (rh >= 9.0f)
            {
                g.setColour (semi == 0 ? th.accent : TC (0xffaaa4cf)); g.setFont (Font (FontOptions (std::min (12.0f, rh - 1.0f), semi == 0 ? Font::bold : Font::plain)));
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
            g.setColour (TC (bar ? 0xff7d77a8 : beat ? 0xff3a3462 : 0xff1d1b38));
            g.drawVerticalLine ((int) x, gr.getY(), vl.getBottom());
            if (bar && b < len)
            {
                g.setColour (TC (0xffb4aed8)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
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
            g.setColour (i == sel ? TC (0xffffffff) : TC (0xff000000).withAlpha (0.5f)); g.drawRoundedRectangle (r, 2.5f, i == sel ? 1.8f : 1.0f);
            // velocity lane
            const float vx = beatX (h.beat);
            g.setColour (c); g.fillRect (vx, vl.getBottom() - vl.getHeight() * h.vel, 3.0f, vl.getHeight() * h.vel);
        }
        g.setColour (TC (0xff3a3264)); g.drawHorizontalLine ((int) vl.getY() - 1, gr.getX(), gr.getRight());
        g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (10.0f, Font::bold)));
        g.drawText ("VEL", Rectangle<float> (all.getX() + 4, vl.getY(), gr.getX() - all.getX() - 8, vl.getHeight()), Justification::centredRight);
        if (playBeat >= 0)
        {
            const float x = beatX (playBeat);
            g.setColour (TC (0xffffffff).withAlpha (0.9f)); g.drawVerticalLine ((int) x, gr.getY(), vl.getBottom());
        }
        if (pat.empty())
        {
            g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (18.0f, Font::bold)));
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
            addChildComponent (*b); switchBtns.push_back (std::move (b));   // v0.32: the KIT STRIP pads switch drums
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
        btn (playPatBtn, "PLAY", "Hear the pattern on your drum (with FL playing it follows the song tempo)", [this] { proc.kitPlay = false; proc.patPlay = proc.patPlay.load() == d ? -1 : d; refreshPattern(); });
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
        btn (playKitBtn, "PLAY KIT", "Every drum with a sound plays its pattern together - hear the whole beat (with FL playing it follows the song)",
             [this] { proc.kitPlay = ! proc.kitPlay.load(); if (proc.kitPlay.load()) proc.patPlay = -1; refreshPattern(); repaint(); });
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
        pageBackdrop (g, *this); ignoreUnused (h);
        for (int y = 0; y < (int) h; y += 3) { g.setColour (TC (0xffffffff).withAlpha (y % 6 == 0 ? 0.012f : 0.0f)); g.drawHorizontalLine (y, 0, w); }
        g.setGradientFill (ColourGradient (th.accent.withAlpha (0.10f), w * 0.6f, 0, Colours::transparentBlack, w * 0.6f, 340, false));
        g.fillRect (0.0f, 0.0f, w, 340.0f);
        kk::modern::waves (g, { w * 0.30f, 74.0f }, { w, 8.0f }, 40.0f, th.accent, th.accent2, 6, 0.22f);
        // the rail behind the tiles
        g.setColour (TC (0xff12112a)); g.fillRect (0, 0, kRail, getHeight());
        g.setColour (TC (0xff3a3264)); g.drawVerticalLine (kRail - 1, 0.0f, h);
        g.setColour (TC (0xffffffff).withAlpha (0.85f)); g.setFont (serif (22.0f, true, 0.25f));
        g.drawFittedText ("KEYS\nKILLA", Rectangle<int> (0, 22, kRail, 60), Justification::centred, 2);
        drawMaker (g, Rectangle<float> (0, 80, (float) kRail, 14), Justification::centred);
        // title
        g.setFont (Font (FontOptions (44.0f, Font::bold)).withExtraKerningFactor (0.08f));
        for (int k = 3; k >= 1; --k) { g.setColour (th.accent.withAlpha (0.12f)); g.drawText (th.title, kRail + 22 - k, 10 - k, 600, 56, Justification::centredLeft); }
        g.setColour (th.text); g.drawText (th.title, kRail + 22, 10, 600, 56, Justification::centredLeft);
        g.setColour (th.accent2); g.setFont (Font (FontOptions (12.5f, Font::bold)).withExtraKerningFactor (0.12f));
        g.drawText (th.sub, kRail + 24, 62, 1000, 18, Justification::centredLeft);
        // KIT STRIP: the seven drums of your kit - click one to edit it
        static const char* kitNames[] { "808", "SNARE / CLAP", "HI-HAT", "KICK", "OPEN HAT", "PERC", "FX" };
        for (int i = 0; i < kk::numDrumSlots; ++i)
        {
            const auto c = kitPad (i).toFloat();
            const auto& t = drumTheme (i);
            auto smp = proc.drum (i).current();
            const bool selPad = i == d;
            if (selPad) { g.setColour (t.accent.withAlpha (0.28f)); g.fillRoundedRectangle (c.expanded (3), 11); }
            g.setGradientFill (ColourGradient (selPad ? t.accent.withAlpha (0.35f) : TC (0xff1c1940), c.getX(), c.getY(), TC (0xff0e0c22), c.getRight(), c.getBottom(), false));
            g.fillRoundedRectangle (c, 9);
            g.setGradientFill (ColourGradient (t.accent, c.getX(), 0, t.accent2, c.getRight(), 0, false));
            g.drawRoundedRectangle (c.reduced (0.5f), 9, selPad ? 2.2f : 1.0f);
            g.setColour (selPad ? TC (0xffffffff) : t.accent); g.setFont (Font (FontOptions (13.0f, Font::bold)).withExtraKerningFactor (0.08f));
            g.drawText (kitNames[i], c.reduced (10, 6).withHeight (16).toNearestInt(), Justification::centredLeft);
            if (smp != nullptr)
            {
                drawPeaks (g, c.reduced (10, 0).withTrimmedTop (24).withTrimmedBottom (18), smp->peaks, t.accent2.withAlpha (0.85f));
                g.setColour (TC (0xffffffff).withAlpha (0.85f)); g.setFont (Font (FontOptions (10.5f)));
                g.drawText (smp->name, c.reduced (10, 4).removeFromBottom (14).toNearestInt(), Justification::centredLeft);
            }
            else { g.setColour (TC (0xff7d77a8)); g.setFont (Font (FontOptions (11.0f))); g.drawText ("empty - drop a sound", c.withTrimmedTop (22).toNearestInt(), Justification::centred); }
            if (proc.kitPlay.load() && smp != nullptr) { g.setColour (TC (0xff36ff6a)); g.fillEllipse (c.getRight() - 16, c.getY() + 8, 8, 8); }
        }
        // section panels (MPC style: dark inset plates with screws)
        for (auto r : { boostPanel, patPanel })
        {
            g.setColour (TC (0xff16152e).withAlpha (0.9f)); g.fillRoundedRectangle (r.toFloat(), 10);
            g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (r.toFloat().reduced (0.5f), 10, 1.2f);
            for (auto c : { r.getTopLeft().translated (9, 9), r.getTopRight().translated (-9, 9), r.getBottomLeft().translated (9, -9), r.getBottomRight().translated (-9, -9) })
            { g.setColour (TC (0xff4a4478)); g.fillEllipse ((float) c.x - 3, (float) c.y - 3, 6, 6); }
        }
        drawArt (g, art);
        drawWave (g);
        g.setColour (th.text.withAlpha (0.7f)); g.setFont (Font (FontOptions (12.0f, Font::bold)).withExtraKerningFactor (0.15f));
        g.drawText ("DRIVE FLAVOUR", satBtns.front()->getX(), satBtns.front()->getY() - 18, 200, 16, Justification::centredLeft);
        if (kitNote.isNotEmpty()) { g.setColour (TC (0xff36ff6a)); g.setFont (Font (FontOptions (12.0f, Font::bold))); g.drawText (kitNote, kitBtn.getX() - 20, kitBtn.getBottom() + 2, 220, 16, Justification::centred); }
        g.setColour (th.text.withAlpha (0.7f)); g.setFont (Font (FontOptions (12.0f, Font::bold)).withExtraKerningFactor (0.15f));
        // pattern title with an LED
        g.setColour (th.accent); g.fillEllipse ((float) patPanel.getX() + 22, (float) patPanel.getY() + 22, 10, 10);
        g.setColour (th.text); g.setFont (Font (FontOptions (22.0f, Font::bold)).withExtraKerningFactor (0.08f));
        g.drawText (th.patTitle, patPanel.getX() + 40, patPanel.getY() + 12, 300, 30, Justification::centredLeft);
        {
        g.setColour (th.text.withAlpha (0.7f)); g.setFont (Font (FontOptions (12.0f, Font::bold)).withExtraKerningFactor (0.15f));
        g.drawText ("STYLE", styleBtns.front()->getX(), styleBtns.front()->getY() - 18, 200, 16, Justification::centredLeft);
        g.drawText ("LENGTH", barBtns.front()->getX(), barBtns.front()->getY() - 18, 200, 16, Justification::centredLeft);
        g.drawText ("DENSITY", density.getX(), density.getY() - 18, 200, 16, Justification::centredLeft);
        g.drawText ("GRID", snapBtns.front()->getX(), snapBtns.front()->getY() - 18, 200, 16, Justification::centredLeft);
        g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (11.0f, Font::bold)));
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
        const int oy = 72;   // the KIT STRIP sits above the boost panel
        playKitBtn.setBounds (w - 14 - 150, 22, 150, 40);
        saveAllBtn.setBounds (playKitBtn.getX() - 10 - 180, 22, 180, 40);
        openKitBtn.setBounds (saveAllBtn.getX() - 10 - 180, 22, 180, 40);
        boostPanel = { x0, 88 + oy, w - x0 - 14, 372 };
        art = { x0 + 26, 106 + oy, 222, 222 };
        wave = { x0 + 290, 100 + oy, w - x0 - 290 - 30, 160 };
        int x = wave.getX();
        for (auto* b : { &loadBtn, &hitBtn, &keysBtn, &clearBtn })
        {
            const int bw = b == &keysBtn ? 270 : 120;
            b->setBounds (x, 270 + oy, bw, 36); x += bw + 10;
            if (b == &keysBtn) { dragWav.setBounds (x, 266 + oy, 190, 44); x += 200; }
        }
        for (int i = 0; i < 3; ++i) satBtns[(size_t) i]->setBounds (wave.getRight() - 3 * 106 - 8 + i * 106, 276 + oy, 100, 28);
        const int n = (int) knobs.size(), area = wave.getRight() - wave.getX(), kw = std::min (98, area / n);
        for (int i = 0; i < n; ++i) knobs[(size_t) i]->setBounds (wave.getX() + i * (area / n) + (area / n - kw) / 2, 318 + oy, kw, kw + 22);
        patPanel = { x0, 470 + oy, w - x0 - 14, h - 470 - oy - 12 };
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
        resetKnobsBtn.setBounds (art.getX() + 21, 380 + oy, 180, 30);
        kitBtn.setBounds (art.getX() + 21, 416 + oy, 180, 34);
        for (Component* c : std::initializer_list<Component*> { &editor, &genBtn, &playPatBtn, &clearPatBtn, &undoBtn, &redoBtn, &resetBtn, &dragMidi, &density })
            c->setVisible (hasPattern());
        for (auto& b : styleBtns) b->setVisible (hasPattern());
        for (auto& b : barBtns) b->setVisible (hasPattern());
        for (auto& b : snapBtns) b->setVisible (hasPattern());
        saveAllBtn.setVisible (true); openKitBtn.setVisible (true);
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
        for (int i = 0; i < kk::numDrumSlots; ++i)
            if (kitPad (i).contains (e.getPosition()) && e.getDistanceFromDragStart() < 4)
            {
                if (i != d) { if (onSwitch) onSwitch (i); }
                else proc.hitDrum (d);
                return;
            }
        if (art.contains (e.getPosition())) proc.hitDrum (d);
        else if (wave.contains (e.getPosition()) && ! proc.drum (d).hasSample() && e.getDistanceFromDragStart() < 4) browse();
    }
    // MY DRUM KIT overview: what is loaded in each of the seven slots + how the kit reaches FL Studio
    void paintKit (Graphics& g)
    {
        auto r = patPanel.reduced (22).withTrimmedTop (50);
        g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (12.0f, Font::bold)));
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
            g.setColour (TC (0xff0b0a18)); g.fillRoundedRectangle (c, 8);
            g.setColour (i == d ? t.accent : TC (0xff3a3264)); g.drawRoundedRectangle (c.reduced (0.5f), 8, i == d ? 2.0f : 1.0f);
            g.setColour (t.accent); g.setFont (Font (FontOptions (13.0f, Font::bold)).withExtraKerningFactor (0.08f));
            g.drawText (slotNames[i], c.reduced (10, 8).withHeight (18).toNearestInt(), Justification::centredLeft);
            if (smp != nullptr)
            {
                drawPeaks (g, c.reduced (10, 34).withTrimmedBottom (20), smp->peaks, t.accent.withAlpha (0.8f));
                g.setColour (TC (0xffffffff).withAlpha (0.85f)); g.setFont (Font (FontOptions (11.0f)));
                g.drawText (smp->name, c.reduced (10, 8).removeFromBottom (16).toNearestInt(), Justification::centredLeft);
            }
            else { g.setColour (TC (0xff7d77a8)); g.setFont (Font (FontOptions (12.0f))); g.drawText ("empty", c.toNearestInt(), Justification::centred); }
        }
        r.removeFromTop (16);
        g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (13.0f)));
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
        playKitBtn.selected = proc.kitPlay.load(); playKitBtn.setButtonText (proc.kitPlay.load() ? "STOP KIT" : "PLAY KIT"); playKitBtn.repaint();
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
        const float pb = proc.patPlay.load() == d || proc.kitPlay.load() ? proc.patBeat.load() : -1.0f;
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
            g.setGradientFill (ColourGradient (TC (0xff36306a), c.x, c.y - rad, TC (0xff0b0a1c), c.x, c.y + rad, false));
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
                g.setColour (TC (0xffffffff).withAlpha (0.5f)); g.fillEllipse (c.x + std::cos (a) * rad * 0.93f - 3, c.y + std::sin (a) * rad * 0.93f - 3, 6, 6);
            }
        }
        else if (kind == kk::drumSnare)   // snare from above: head, rim, lugs, wires
        {
            g.setColour (TC (0xff141230)); g.fillEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2);
            g.setColour (th.accent.withAlpha (0.8f)); g.drawEllipse (c.x - rad, c.y - rad, rad * 2, rad * 2, 5.0f);
            const float hr = rad * 0.86f;
            g.setGradientFill (ColourGradient (TC (0xffe9f0f2), c.x - hr * 0.3f, c.y - hr * 0.4f, TC (0xff98a2a8), c.x + hr, c.y + hr, true));
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
            g.setGradientFill (ColourGradient (TC (0xfffff0b0), c.x - rad * 0.3f, c.y - rad * 0.3f, TC (0xff8a5a10), c.x + rad, c.y + rad, true));
            g.fillEllipse (c.x - rad, c.y - rad * tilt, rad * 2, rad * 2 * tilt);
            for (int i = 1; i < 14; ++i)
            {
                const float rr = rad * (float) i / 14.0f;
                g.setColour (TC (0xff5a3a08).withAlpha (0.25f)); g.drawEllipse (c.x - rr, c.y - rr * tilt, rr * 2, rr * 2 * tilt, 1.0f);
            }
            g.setColour (th.accent2.withAlpha (0.4f + 0.5f * pulse)); g.drawEllipse (c.x - rad, c.y - rad * tilt, rad * 2, rad * 2 * tilt, 3.0f);
            g.setColour (TC (0xff3a2a10)); g.fillEllipse (c.x - 10, c.y - 6, 20, 12);
        }
    }
    void drawWave (Graphics& g)
    {
        const auto r = wave.toFloat();
        g.setGradientFill (ColourGradient (TC (0xff1a1638), r.getX(), r.getY(), TC (0xff0b0a1c), r.getX(), r.getBottom(), false)); g.fillRoundedRectangle (r, 10);
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
            g.setColour (TC (0xffffffff).withAlpha (0.12f)); g.drawVerticalLine ((int) x, mid - dv * half, mid + dv * half);
        }
        Path p; p.startNewSubPath (in.getX(), mid);
        for (int i = 0; i < cols; ++i) p.lineTo (in.getX() + in.getWidth() * (float) i / (float) cols, mid - s->peaks[(size_t) i] * half);
        for (int i = cols - 1; i >= 0; --i) p.lineTo (in.getX() + in.getWidth() * (float) i / (float) cols, mid + s->peaks[(size_t) i] * half);
        p.closeSubPath();
        g.setGradientFill (ColourGradient (th.accent2, in.getX(), mid - half, th.accent, in.getX(), mid, false));
        g.fillPath (p);
        if (const float ph = proc.drum (d).playhead(); ph >= 0)
        {
            g.setColour (TC (0xffffffff).withAlpha (0.85f));
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
              kitBtn { lnf }, saveAllBtn { lnf }, openKitBtn { lnf }, playKitBtn { lnf };
    Rectangle<int> kitPad (int i) const
    {
        const int x0 = kRail + 14, w = getWidth() - x0 - 14, gap = 8, pw = (w - gap * (kk::numDrumSlots - 1)) / kk::numDrumSlots;
        return { x0 + i * (pw + gap), 86, pw, 62 };
    }
    String kitNote;
    bool hasPattern() const { return true; }   // v0.32: every drum has its generator + piano roll
    Slider density;
    DragFileButton dragMidi { "DRAG MIDI TO FL", TC (0xff36ff6a) }, dragWav { "DRAG WAV TO FL", TC (0xff36ff6a) };
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
// v0.32: a hosted plugin's own window opens as its own floating window at its real size - always fully visible
// (a plugin's native view cannot be shrunk inside KEYS KILLA). It closes together with KEYS KILLA.
class VstWindow : public DocumentWindow
{
public:
    VstWindow (juce::AudioPluginInstance& p, Component* near)
        : DocumentWindow (p.getName() + "   -   pick a sound, then close this window: BREED LAB takes it", TC (0xff15132e), DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        editor.reset (p.createEditorIfNeeded());
        if (editor != nullptr) setContentNonOwned (editor.get(), true);
        setResizable (false, false);
        setAlwaysOnTop (true);
        if (near != nullptr && near->isShowing())
        {
            const auto c = near->getScreenBounds().getCentre();
            setTopLeftPosition (jmax (0, c.x - getWidth() / 2), jmax (0, c.y - getHeight() / 2));
        }
        else centreWithSize (getWidth(), getHeight());
    }
    ~VstWindow() override { clearContentComponent(); editor.reset(); }
    std::function<void()> onBack;   // PAIR FROM VST: the sound you picked in the plugin is taken
    bool hasEditor() const { return editor != nullptr; }
    void closeButtonPressed() override { setVisible (false); if (onBack) onBack(); }
private:
    std::unique_ptr<AudioProcessorEditor> editor;
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
        search.setTextToShowWhenEmpty ("search sounds ...", TC (0xff6a6a6a));
        search.onTextChange = [this] { filter(); };
        addAndMakeVisible (search);
        list.setModel (this); list.setRowHeight (22);
        list.setColour (ListBox::backgroundColourId, TC (0xff100e26));
        addAndMakeVisible (list);
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (prevBtn, "<", "Previous sound", [this] { step (-1); });
        btn (nextBtn, ">", "Next sound", [this] { step (1); });
        btn (bankBtn, "SAVE TO FOLDER", "Keep this sound in one of your MY SOUNDS folders", [this]
        {
            saveToFolderMenu (proc, { proc.pairParents[(size_t) side] }, &bankBtn, [safe = Component::SafePointer<VstSide> (this)] (String msg) { if (safe != nullptr) { safe->status = msg; safe->repaint(); } });
        });
        btn (takeBtn, "TAKE THIS SOUND", "Take the sound the plugin plays right now - it joins the list below", [this] { pick (-1); });
        btn (removeBtn, "REMOVE", "Remove this plugin from side " + String (s == 0 ? "A" : "B") + " (0, 1 or 2 plugins - your choice)", [this]
        {
            if (onShowPlugin) onShowPlugin (-1 - side);   // its window closes first
            proc.unloadVstSide (side);
            pluginBox.setSelectedId (0, dontSendNotification); search.clear(); status = {}; filter(); repaint();
            if (onChanged) onChanged();
        });
        btn (showBtn, "SHOW PLUGIN", "Show the plugin inside BREED LAB: pick a sound there, then TAKE THIS SOUND", [this] { if (onShowPlugin) onShowPlugin (side); });
        btn (keysBtn, "PLAY", "The keys play this plugin (on this page the keys always play a VST - choose which one)", [this] { proc.vstKeys = side; refreshButtons(); });
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
        const Colour c1 = side == 0 ? TC (0xffff2f6d) : TC (0xff9b4dff), c2 = side == 0 ? TC (0xffff8a3d) : TC (0xff4d7dff);
        g.setGradientFill (ColourGradient (TC (0xff1c1940), 0, 0, TC (0xff100e26), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 10);
        g.setGradientFill (ColourGradient (c1, 0, 0, c2, r.getRight(), r.getBottom(), false)); g.drawRoundedRectangle (r.reduced (1.0f), 10, 1.8f);
        ignoreUnused (s);
        g.setGradientFill (ColourGradient (c1, 12, 6, c2, 48, 42, false)); g.setFont (Font (FontOptions (34.0f, Font::bold)));
        g.drawText (side == 0 ? "A" : "B", 12, 6, 36, 36, Justification::centred);
        g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (11.5f)));
        const auto& h = proc.host (side);
        String info = ! h.loaded() ? String ("choose a plugin")
                    : proc.vstSounds[(size_t) side].empty() ? String ("This plugin keeps its sounds in its own window: SHOW PLUGIN, pick a sound there, TAKE THIS SOUND - every take joins this list")
                    : String ((int) proc.vstSounds[(size_t) side].size()) + " sounds - click one = hear it and use it";
        if (status.isNotEmpty()) info = status;
        g.drawFittedText (info, statusArea, Justification::centredLeft, 2);
        if (auto& snd = proc.pairParents[(size_t) side]; snd != nullptr)
        {
            g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (13.0f, Font::bold)));
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
        removeBtn.setBounds (top.removeFromRight (84)); top.removeFromRight (6);
        showBtn.setBounds (top.removeFromRight (120)); top.removeFromRight (6);
        pluginBox.setBounds (top);
        r.removeFromTop (6);
        search.setBounds (r.removeFromTop (26));
        r.removeFromTop (4);
        auto bottom = r.removeFromBottom (32);
        prevBtn.setBounds (bottom.removeFromLeft (36)); bottom.removeFromLeft (4);
        nextBtn.setBounds (bottom.removeFromLeft (36)); bottom.removeFromLeft (8);
        bankBtn.setBounds (bottom.removeFromRight (130)); bottom.removeFromRight (6);
        takeBtn.setBounds (bottom.removeFromRight (150));
        soundArea = bottom.withTrimmedLeft (4);
        statusArea = r.removeFromBottom (30);
        list.setBounds (r);
    }
    void retake() { pick (-1); }   // the sound the plugin plays now (after you changed it in SHOW PLUGIN)
    void refreshButtons() { keysBtn.selected = proc.vstKeys.load() == side; keysBtn.repaint(); }
private:
    // ListBoxModel
    int getNumRows() override { return (int) shown.size(); }
    void paintListBoxItem (int row, Graphics& g, int w, int h, bool) override
    {
        if (row < 0 || row >= (int) shown.size()) return;
        const int idx = shown[(size_t) row];
        const bool sel = idx == proc.vstSel[(size_t) side];
        if (sel) { g.setColour (lnf.skin->accent.withAlpha (0.35f)); g.fillRect (0, 0, w, h); }
        else if (row % 2) { g.setColour (TC (0xff12112a)); g.fillRect (0, 0, w, h); }
        g.setColour (sel ? TC (0xffffffff) : TC (0xffd9d4f5)); g.setFont (Font (FontOptions (13.0f)));
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
    HotButton prevBtn { lnf }, nextBtn { lnf }, bankBtn { lnf }, takeBtn { lnf }, showBtn { lnf }, keysBtn { lnf }, removeBtn { lnf };
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
                sides[(size_t) k]->onShowPlugin = [this] (int sd) { if (sd < 0) { if (vstWinSide == -1 - sd) closeVstUi(); } else openVstUi (sd); };
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
        breedBtn.state = [this]
        {
            ReactorState st; st.maxSounds = jlimit (2, 4, proc.pairUse);
            for (int k = 0; k < st.maxSounds; ++k)
                if (proc.pairParents[(size_t) k] != nullptr) st.stream[st.sounds++] = kk::theme().accent.withRotatedHue (0.03f * (float) k);
            return st;
        };
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
        diggaBtn.setButtonText ("HARVEST SAMPLER"); diggaBtn.framed = true;
        diggaBtn.setTooltip ("Collect sounds from the sample loaded in the SAMPLER");
        diggaBtn.onClick = [this]
        {
            if (proc.harvestFromChop() == 0)
                AlertWindow::showMessageBoxAsync (MessageBoxIconType::InfoIcon, "HARVEST", "The SAMPLER is empty.\nOpen SAMPLER, drop a sample or a song, then come back.");
            repaint();
        };
        addAndMakeVisible (diggaBtn);
        setOpaque (true);
        startTimerHz (20);
    }
    ~PairPage() override { stopTimer(); vstWin.reset(); }   // the hosted editor goes before the plugin
    void visibilityChanged() override
    {
        if (! isVisible()) { if (vstPage) closeVstUi(); return; }   // the plugin window belongs to this page
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
        pageBackdrop (g, *this);
        g.setColour (s.panelEdge); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (2), 8, 1.4f);
        // ---- HARVEST: drop zone + bank by character
        if (! vstPage)
        {
            const auto dz = dropZone().toFloat();
            g.setColour (TC (dropHover ? 0xff3a1640 : 0xff16152e)); g.fillRoundedRectangle (dz, 10);
            Path d; d.addRoundedRectangle (dz.reduced (1), 10);
            const float pat[] { 7.0f, 5.0f };
            PathStrokeType (1.6f).createDashedStroke (d, d, pat, 2);
            g.setColour (s.accent.withAlpha (dropHover ? 1.0f : 0.6f)); g.fillPath (d);
            g.setColour (TC (0xffffffff)); g.setFont (serif (20.0f, true, 0.2f));
            g.drawText (vstPage ? "PAIR FROM VST" : "HARVEST", dz.withHeight (40).translated (0, 4).toNearestInt(), Justification::centred);
            g.setColour (TC (0xffc9c0bd)); g.setFont (serif (12.5f, false, 0.08f));
            if (vstPage) g.drawFittedText (status, dz.reduced (6).withTrimmedTop (150).toNearestInt(), Justification::centred, 1);
            else g.drawFittedText (proc.harvesting() ? "listening ..." : "drop a song, a sample,\na vinyl rip - BREED LAB\npulls the sounds out", dz.reduced (10).withTrimmedTop (42).withHeight (60).toNearestInt(), Justification::centred, 3);
            for (int c = 0; c < kk::numCats; ++c)
            {
                const auto col = bankCol (c).toFloat();
                g.setColour (TC (0xff0e0d20)); g.fillRoundedRectangle (col, 6);
                g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (col, 6, 1.0f);
                g.setColour (s.accent); g.setFont (Font (FontOptions (11.0f, Font::bold)).withExtraKerningFactor (0.08f));
                g.drawText (kk::harvestCatShort (c), col.withHeight (20).toNearestInt(), Justification::centred);
            }
            int row[kk::numCats] {};
            for (int i = 0; i < (int) proc.bank.size(); ++i)
            {
                const auto& it = proc.bank[(size_t) i];
                if (row[it.cat] >= kk::Harvest::perCategory) continue;
                const auto r = chip (it.cat, row[it.cat]++).toFloat();
                g.setColour (i == armed ? s.accent.withAlpha (0.6f) : TC (0xff1e1c3c)); g.fillRoundedRectangle (r, 4);
                drawPeaks (g, r.reduced (3, 2).withWidth (r.getWidth() * 0.45f), it.sound->peaks, s.accent.withAlpha (0.8f));
                g.setColour (TC (0xffffffff).withAlpha (0.85f)); g.setFont (Font (FontOptions (10.5f)));
                g.drawText (it.sound->pitched && it.cat != kk::catDrum && it.cat != kk::catFx ? MidiMessage::getMidiNoteName (it.sound->rootNote, true, true, 5) : String (i + 1),
                            r.withTrimmedLeft (r.getWidth() * 0.5f).toNearestInt(), Justification::centred);
            }
            if (proc.bank.empty())
            {
                g.setColour (TC (0xff6a6290)); g.setFont (serif (13.0f, false, 0.1f));
                g.drawFittedText (vstPage ? "your sound bank is empty - choose a plugin, GRAB ALL ITS SOUNDS" : "your sound bank is empty - drop a song on HARVEST", bankArea().withTrimmedTop (60), Justification::centredTop, 2);
            }
        }
        // parents
        for (int k = 0; k < (vstPage ? 0 : kk::PairLab::maxParents); ++k)
        {
            const auto r = slot (k).toFloat();
            const auto& p = proc.pairParents[(size_t) k];
            g.setColour (TC (0xff16152e)); g.fillRoundedRectangle (r, 8);
            if (hoverSlot == k) drawGlowFrame (g, r, s.accent, 8);
            else
            {
                Path d; d.addRoundedRectangle (r.reduced (1), 8);
                const float pat[] { 6.0f, 4.0f };
                if (p == nullptr) PathStrokeType (1.2f).createDashedStroke (d, d, pat, 2), g.setColour (TC (0xff4a4478)), g.fillPath (d);
                else { g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (r, 8, 1.2f); }
            }
            g.setColour (TC (0xffaaa4cf)); g.setFont (serif (12.0f, false, 0.25f));
            g.drawText ("SOUND " + String (k + 1), r.reduced (12, 6).withHeight (16).toNearestInt(), Justification::centredLeft);
            {   // dice: a random sound flies in
                const auto db = diceBox (k).toFloat();
                g.setColour (TC (0xff2e1846)); g.fillRoundedRectangle (db, 5);
                g.setColour (s.accent); g.drawRoundedRectangle (db, 5, 1.2f);
                g.setColour (TC (0xffffffff));
                for (auto pt : { Point<float> (0.3f, 0.3f), Point<float> (0.7f, 0.3f), Point<float> (0.5f, 0.5f), Point<float> (0.3f, 0.7f), Point<float> (0.7f, 0.7f) })
                    g.fillEllipse (db.getX() + db.getWidth() * pt.x - 2.2f, db.getY() + db.getHeight() * pt.y - 2.2f, 4.4f, 4.4f);
            }
            if (p == nullptr)
            {
                g.setColour (TC (0xffc8c4e8)); g.setFont (serif (15.0f, false, 0.12f));
                g.drawFittedText ("drop a WAV here\nor roll the dice", r.toNearestInt(), Justification::centred, 2);
                continue;
            }
            drawPeaks (g, r.reduced (12, 22).withTrimmedBottom (4).withTrimmedRight (30), p->peaks, s.accent.withAlpha (0.8f));
            g.setColour (TC (0xffffffff)); g.setFont (serif (13.5f, false, 0.05f));
            g.drawText (p->name, r.reduced (12, 6).removeFromBottom (18).toNearestInt(), Justification::centredLeft);
            g.setColour (s.accent); g.setFont (serif (12.0f, false, 0.1f));
            g.drawText (p->pitched ? MidiMessage::getMidiNoteName (p->rootNote, true, true, 5) : String ("DRUM / FX"), r.reduced (12, 6).withHeight (16).withTrimmedRight (26).toNearestInt(), Justification::centredRight);
            g.setColour (TC (0xffaaa4cf)); g.drawText ("x", closeBox (k), Justification::centred);
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
        g.setColour (TC (0xffaaa4cf)); g.setFont (serif (12.0f, false, 0.25f));
        if (! vstPage) g.drawText ("CHILDREN", flavorBtns.front()->getX(), flavorBtns.front()->getY() - 16, 200, 14, Justification::centredLeft);
        if (note.isNotEmpty()) { g.setColour (TC (0xff36ff6a)); g.setFont (Font (FontOptions (13.0f, Font::bold))); g.drawText (note, getWidth() / 2 + 110, kid (0).getY() - 26, getWidth() / 2 - 130, 18, Justification::centredRight); }
        for (int i = 0; i < (int) flavorBtns.size(); ++i) { flavorBtns[(size_t) i]->selected = i == proc.pairFlavor; }
        // children
        for (int k = 0; k < 6; ++k)
        {
            const auto r = kid (k).toFloat();
            const bool has = k < (int) proc.pairKids.size();
            const bool sel = has && k == proc.pairSel;
            g.setGradientFill (ColourGradient (TC (sel ? 0xff3a1640 : 0xff1c1940), r.getX(), r.getY(), TC (sel ? 0xff1a0c26 : 0xff100e26), r.getX(), r.getBottom(), false)); g.fillRoundedRectangle (r, 8);
            if (sel) drawGlowFrame (g, r, s.accent, 8); else { g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (r, 8, 1.2f); }
            g.setColour (TC (0xffaaa4cf)); g.setFont (serif (12.0f, false, 0.25f));
            g.drawText ("CHILD " + String (k + 1), r.reduced (10, 6).withHeight (16).toNearestInt(), Justification::centredLeft);
            if (! has)
            {
                g.setColour (TC (0xff6a6290)); g.setFont (serif (13.0f, false, 0.12f));
                g.drawText (proc.pairParents[0] || proc.pairParents[1] || proc.pairParents[2] || proc.pairParents[3] ? "press BREED" : (vstPage ? "choose a sound in A and B" : "drop your sounds"), r.toNearestInt(), Justification::centred);
                continue;
            }
            const auto& c = *proc.pairKids[(size_t) k];
            drawPeaks (g, r.reduced (10, 26).withTrimmedBottom (40), c.peaks, (sel ? s.accent : s.accent.withAlpha (0.6f)));
            g.setColour (TC (0xffffffff)); g.setFont (serif (12.5f, false, 0.08f));
            g.drawFittedText (c.method, r.reduced (10, 6).removeFromBottom (54).removeFromTop (18).toNearestInt(), Justification::centredLeft, 1);
            g.setColour (s.accent); g.setFont (serif (11.5f, false, 0.1f));
            g.drawText (c.pitched ? MidiMessage::getMidiNoteName (c.rootNote, true, true, 5) : String ("FX"), r.reduced (10, 6).withHeight (16).toNearestInt(), Justification::centredRight);
            // LOOP button + drag hint
            const auto lb = loopBox (k).toFloat();
            const bool looping = proc.loopPlaying() && proc.pairLoopKid == k;
            g.setColour (looping ? s.accent : TC (0xff2a2450)); g.fillRoundedRectangle (lb, 5);
            g.setColour (TC (0xffffffff)); g.setFont (serif (12.0f, false, 0.2f));
            g.drawText (looping ? "STOP LOOP" : "LOOP", lb.toNearestInt(), Justification::centred);
            g.setColour (TC (0xffaaa4cf)); g.setFont (serif (10.5f, false, 0.15f));
            g.drawText (looping ? "drag: MIDI" : "drag: WAV", r.reduced (10, 4).removeFromBottom (14).toNearestInt(), Justification::centredRight);
            {   // SAVE to a folder
                const auto sb = saveBox (k).toFloat();
                g.setColour (TC (0xff1c2a1e)); g.fillRoundedRectangle (sb, 4);
                g.setColour (TC (0xff36ff6a)); g.drawRoundedRectangle (sb, 4, 1.0f);
                g.setFont (Font (FontOptions (10.5f, Font::bold))); g.drawText ("SAVE", sb.toNearestInt(), Justification::centred);
            }
        }
        g.setColour (TC (0xffaaa4cf)); g.setFont (serif (12.0f, false, 0.25f));
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
                saveToFolderMenu (proc, { proc.withEdits (proc.pairKids[(size_t) k]) }, this, [safe = Component::SafePointer<PairPage> (this)] (String msg) { if (safe != nullptr) { safe->note = msg; safe->repaint(); } });
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
    // a plugin's own window (floating, full size)
    void openVstUi (int side)
    {
        auto* top = getTopLevelComponent();
        if (auto* inst = proc.host (side).instance(); inst != nullptr && top != nullptr)
        {
            // v0.35: inside BREED LAB first (fitted, all of it visible) - a separate window only when it cannot fit
            if (auto* ed = findParentComponentOfClass<KeysKillaEditor>())
            {
                vstWin.reset();
                auto safe = Component::SafePointer<PairPage> (this);
                if (ed->showHostedEditor (*inst, String (side == 0 ? "A  " : "B  ") + proc.host (side).name(),
                                          [safe, side] { if (safe != nullptr && safe->sides[(size_t) side]) safe->sides[(size_t) side]->retake(); },
                                          [safe] { if (safe != nullptr) safe->repaint(); }))
                { vstWinSide = side; return; }
            }
            if (vstWin != nullptr && vstWinSide != side) vstWin.reset();
            if (vstWin == nullptr && inst->hasEditor())
            {
                vstWin = std::make_unique<VstWindow> (*inst, top);
                vstWinSide = side;
                vstWin->onBack = [safe = Component::SafePointer<PairPage> (this), side]
                { if (safe != nullptr && safe->sides[(size_t) side]) safe->sides[(size_t) side]->retake(); };
            }
            if (vstWin != nullptr) { vstWin->setVisible (true); vstWin->toFront (true); }
        }
    }
    void closeVstUi()
    {
        vstWin.reset();
        if (auto* ed = findParentComponentOfClass<KeysKillaEditor>()) ed->closeHostedEditor();
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
    std::unique_ptr<VstWindow> vstWin;
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
        setTooltip ("DRAG TO DAW: drag this into FL Studio - the selected child / sound as a WAV (in LOOP mode its melody as MIDI). Click: hear it.");
        setMouseCursor (MouseCursor::DraggingHandCursor);
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (6);
        g.setColour (TC (0xcc0b0808)); g.fillRoundedRectangle (r, 10);
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
        g.setColour (TC (0xffffffff).withAlpha (0.9f)); g.strokePath (wave, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));
        // arrow
        const float ax = r.getCentreX(), ay = w.getBottom() + 14 + (hover ? 4.0f : 0.0f);
        Path arrow; arrow.addArrow ({ ax, ay, ax, ay + 30 }, 4.0f, 16.0f, 12.0f);
        g.setColour (s.accent); g.fillPath (arrow);
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (19.0f, Font::bold)).withExtraKerningFactor (0.12f));
        g.drawFittedText ("DRAG\nTO DAW", Rectangle<float> (r.getX(), ay + 40, r.getWidth(), 46).toNearestInt(), Justification::centred, 2);
        g.setColour (TC (0xffaaa4cf)); g.setFont (Font (FontOptions (11.5f, Font::italic)));
        g.drawFittedText (busy ? "printing..." : "it's yours now", Rectangle<float> (r.getX(), r.getBottom() - 26, r.getWidth(), 18).toNearestInt(), Justification::centred, 1);
    }
    void mouseEnter (const MouseEvent&) override { hover = true; repaint(); }
    void mouseExit (const MouseEvent&) override { hover = false; repaint(); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragged || e.getDistanceFromDragStart() < 5) return;
        dragged = true; busy = true; repaint();
        // v0.35: what you hear is what you drag - your audio child, the selected child's loop (LOOP), or the sound
        File f;
        if (proc.labAudioMode() && isPositiveAndBelow (proc.pairSel, (int) proc.pairKids.size())) f = proc.exportPairKid (proc.pairSel);
        else if (proc.mainLoopMode && isPositiveAndBelow (proc.selectedChild(), (int) proc.kids().size())) f = proc.exportLoopMidi (proc.kids()[(size_t) proc.selectedChild()].g);
        else f = proc.exportSoundWav (72);
        busy = false; repaint();
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (! dragged && e.getDistanceFromDragStart() < 5)
            proc.previewNote = proc.labAudioMode() && isPositiveAndBelow (proc.pairSel, (int) proc.pairKids.size()) ? proc.pairKids[(size_t) proc.pairSel]->rootNote : 72;
        dragged = false;
    }
private:
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    bool hover = false, dragged = false, busy = false;
};

// left column: BREED LAB / FAMILY TREE / PAIR / VST / MY SOUNDS, then STUDIO: SAMPLER / DRUM KIT / FX RACK
// (-1 = a drum page of the bottom row is open)
// a full page that also covers the strip under the left tiles (the tiles stay on top)
class InsetPage : public Component
{
public:
    InsetPage (std::unique_ptr<Component> c) : inner (std::move (c)) { addChildComponent (*inner); setOpaque (true); }   // shown with the page: its visibilityChanged refreshes it
    void paint (Graphics& g) override { pageBackdrop (g, *this); }
    void resized() override { inner->setBounds (getLocalBounds().withTrimmedLeft (getWidth() * 140 / 1652)); }
    void visibilityChanged() override { inner->setVisible (isVisible()); }
    Component* page() const { return inner.get(); }
private:
    std::unique_ptr<Component> inner;
};

// ---------------- FX RACK (v0.32): KEYS KILLA's own effects on every melody (synth, PAIR, VST, SAMPLER) ----------------
class RackKnob : public Slider
{
public:
    RackKnob (std::atomic<float>& t, String name, double lo, double hi, double def, Colour a, Colour b)
        : Slider (RotaryHorizontalVerticalDrag, NoTextBox), target (t), label (std::move (name)), ca (a), cb (b)
    {
        setRange (lo, hi, 0.0); setValue (t.load(), dontSendNotification);
        setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        setDoubleClickReturnValue (true, def);
        onValueChange = [this] { target = (float) getValue(); };
    }
    void sync() { setValue (target.load(), dontSendNotification); }
    std::function<String (double)> valueText;
    void paint (Graphics& g) override
    {
        // v0.44: no knob - a living cell (no numbers: you hear it)
        kk::cell::draw (g, getLocalBounds().toFloat(), (float) valueToProportionOfLength (getValue()), getMinimum() < 0 && getMaximum() > 0, ca, cb, label, isMouseOverOrDragging(), false, 18.0f, (float) label.hashCode() * 0.001f);
    }
    void mouseEnter (const MouseEvent& e) override { Slider::mouseEnter (e); repaint(); }
    void mouseExit (const MouseEvent& e) override { Slider::mouseExit (e); repaint(); }
private:
    std::atomic<float>& target;
    String label; Colour ca, cb;
};

class RackCard : public Component, public SettableTooltipClient
{
public:
    RackCard (KeysKillaProcessor& p, int s) : proc (p), slot (s)
    {
        using namespace kk;
        static const uint32 cols[numRackSlots][2] { { 0xffff5a1f, 0xffffc23d }, { 0xffa78bfa, 0xff6366f1 }, { 0xff22d3ee, 0xff4d7dff }, { 0xffff3fd2, 0xff9b4dff },
                                                    { 0xff2ee6a6, 0xff22d3ee }, { 0xff7c4dff, 0xffff3fd2 }, { 0xffff2f6d, 0xffff8a3d }, { 0xff4dd2ff, 0xff9b4dff },
                                                    { 0xff8b8bff, 0xffc77dff }, { 0xffffd23f, 0xffff8a3d }, { 0xff34d399, 0xff4dd2ff } };
        ca = TC (cols[s][0]); cb = TC (cols[s][1]);   // v0.34: theme colours (amber)
        auto k = [this] (int v, const char* n, double lo, double hi, double def)
        { knobs.push_back (std::make_unique<RackKnob> (proc.rack.v[(size_t) v], n, lo, hi, def, ca, cb)); addAndMakeVisible (*knobs.back()); };
        static const char* tips[numRackSlots] {
            "DRIVE: warm it up or smash it - SOFT, TAPE, HARD, BLOWN, FOLD", "LO-FI: fewer bits, old sampler grit",
            "CHORUS: wide, moving stereo", "PHASER: sweeping, swirling", "FLANGER: jet sweep", "HALF-TIME: the melody at half speed, mixed in",
            "STUTTER: a tempo-synced gate that chops the melody", "DELAY: tempo echoes", "REVERB: room, plate or huge cloud",
            "EQ: more or less bass and air", "WIDTH: narrower or wider stereo" };
        setTooltip (String (tips[s]) + "   (click the name = on / off)");
        switch (s)
        {
            case rkDrive:   k (rvDrive, "DRIVE", 0, 1, 0.35); chips = { "SOFT", "TAPE", "HARD", "BLOWN", "FOLD" }; chipVal = rvDriveType; break;
            case rkLofi:    k (rvCrush, "CRUSH", 0, 1, 0.3); break;
            case rkChorus:  k (rvChorus, "AMOUNT", 0, 1, 0.4); break;
            case rkPhaser:  k (rvPhaser, "AMOUNT", 0, 1, 0.4); break;
            case rkFlanger: k (rvFlanger, "AMOUNT", 0, 1, 0.35); break;
            case rkHalf:    k (rvHalf, "MIX", 0, 1, 0.3); break;
            case rkGate:    k (rvGateDepth, "DEPTH", 0, 1, 0.8); chips = { "1/8", "1/16", "1/32" }; chipVal = rvGateRate; break;
            case rkDelay:   k (rvDelayMix, "MIX", 0, 1, 0.25); k (rvDelayFb, "FEEDBACK", 0, 0.9, 0.35); chips = { "1/4", "1/8", "1/8D", "1/16", "1/4T" }; chipVal = rvDelayTime; break;
            case rkReverb:  k (rvRevMix, "MIX", 0, 1, 0.3); k (rvRevSize, "SIZE", 0, 1, 0.6); chips = { "HALL", "PLATE", "CLOUD" }; chipVal = rvRevType; break;
            case rkEq:      k (rvEqLow, "LOW", -12, 12, 0); k (rvEqHigh, "HIGH", -12, 12, 0); break;
            default:        k (rvWidth, "WIDTH", 0, 1, 0.75); break;
        }
    }
    void sync() { for (auto& kn : knobs) kn->sync(); repaint(); }
    void paint (Graphics& g) override
    {
        const bool on = proc.rack.on[(size_t) slot].load();
        auto r = getLocalBounds().toFloat().reduced (3);
        if (on) { g.setColour (ca.withAlpha (0.22f)); g.fillRoundedRectangle (r.expanded (2), 12); }
        g.setGradientFill (ColourGradient (on ? TC (0xff1b1940).interpolatedWith (ca, 0.18f) : TC (0xff17153a),
                                           0, r.getY(), TC (0xff0e0c22), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 10);
        g.setGradientFill (ColourGradient (on ? ca : ca.withAlpha (0.35f), r.getX(), 0, on ? cb : cb.withAlpha (0.25f), r.getRight(), 0, false));
        g.drawRoundedRectangle (r, 10, on ? 2.0f : 1.2f);
        // header = power switch
        auto h = head();
        g.setColour (on ? ca : TC (0xff4a4478)); g.fillEllipse (h.getX() + 4, h.getCentreY() - 6, 12, 12);
        if (on) { g.setColour (ca.withAlpha (0.35f)); g.fillEllipse (h.getX(), h.getCentreY() - 10, 20, 20); }
        g.setColour (on ? TC (0xffffffff) : TC (0xffaaa4cf)); g.setFont (Font (FontOptions (15.0f, Font::bold)).withExtraKerningFactor (0.1f));
        g.drawText (kk::rackSlotName (slot), h.withTrimmedLeft (26), Justification::centredLeft);
        g.setFont (Font (FontOptions (10.5f, Font::bold))); g.setColour (on ? TC (0xff36ff6a) : TC (0xff6a6290));
        g.drawText (on ? "ON" : "OFF", h, Justification::centredRight);
        for (int i = 0; i < chips.size(); ++i)
        {
            auto cr = chipRect (i);
            const bool sel = roundToInt (proc.rack.v[(size_t) chipVal].load()) == i;
            if (sel) { g.setGradientFill (ColourGradient (ca, cr.getX(), 0, cb, cr.getRight(), 0, false)); g.fillRoundedRectangle (cr, 5); }
            else { g.setColour (TC (0xff17153a)); g.fillRoundedRectangle (cr, 5); g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (cr, 5, 1.0f); }
            g.setColour (sel ? TC (0xffffffff) : TC (0xffc8c4e8)); g.setFont (Font (FontOptions (10.5f, Font::bold)));
            g.drawFittedText (chips[i], cr.toNearestInt(), Justification::centred, 1, 0.6f);
        }
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (12).withTrimmedTop (34);
        if (! chips.isEmpty()) r.removeFromBottom (30);
        const int n = (int) knobs.size(), w = jmin (110, r.getWidth() / jmax (1, n));
        int x = r.getCentreX() - w * n / 2;
        for (auto& kn : knobs) { kn->setBounds (x, r.getY(), w, r.getHeight()); x += w; }
    }
    void mouseDown (const MouseEvent& e) override
    {
        if (head().contains (e.position)) { auto& o = proc.rack.on[(size_t) slot]; o = ! o.load(); repaint(); if (onChange) onChange(); return; }
        for (int i = 0; i < chips.size(); ++i)
            if (chipRect (i).contains (e.position)) { proc.rack.v[(size_t) chipVal] = (float) i; repaint(); return; }
    }
    std::function<void()> onChange;
private:
    Rectangle<float> head() const { return { 14.0f, 10.0f, (float) getWidth() - 28.0f, 24.0f }; }
    Rectangle<float> chipRect (int i) const
    {
        const float w = ((float) getWidth() - 24.0f) / (float) chips.size();
        return { 12.0f + w * (float) i + 2.0f, (float) getHeight() - 36.0f, w - 4.0f, 22.0f };
    }
    KeysKillaProcessor& proc;
    int slot;
    Colour ca, cb;
    std::vector<std::unique_ptr<RackKnob>> knobs;
    StringArray chips;
    int chipVal = 0;
};

//==============================================================================
// v0.37 SURPRISE FX: EVOLVE for effects. The effect chain on your sound sits in the middle, six other chains around it.
// Hover = hear your sound through it.  Click = keep it, six new chains grow from it.  SURPRISE ME = six wild new ones.
class FxEvolveView : public Component, private Timer
{
public:
    FxEvolveView (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (surpriseBtn, "SURPRISE ME", "Six brand-new effect chains around your sound", [this] { surprise(); });
        btn (againBtn, "6 NEW KIDS", "Six new chains grown from the middle one", [this] { grow (centre, true); });
        btn (dryBtn, "DRY", "No effects: your sound as it is (a new start)", [this] { KeysKillaProcessor::FxGenome g; g.name = "Dry"; startFrom (g); });
        btn (hearBtn, "HEAR IT", "Play a melody loop with your sound (in time) so you hear every chain - again = stop", [this] { proc.toggleLoop(); });
        wildSl.setSliderStyle (Slider::LinearBar); wildSl.setRange (0.0, 1.0, 0.01); wildSl.setValue (0.4, dontSendNotification);
        wildSl.textFromValueFunction = [] (double v) { return v < 0.34 ? String ("SAFE") : v < 0.67 ? String ("MIXED") : String ("WILD"); };
        wildSl.setColour (Slider::trackColourId, lnf.skin->accent.withAlpha (0.5f));
        wildSl.setColour (Slider::textBoxTextColourId, kk::theme().text);
        wildSl.setTooltip ("SAFE = the new chains stay close.  WILD = they change a lot");
        wildSl.updateText();
        addAndMakeVisible (wildSl);
        startTimerHz (30);
    }
    void visibilityChanged() override
    {
        if (isVisible() && nodes.empty()) { auto g = proc.fxCurrent(); g.name = "Your FX now"; startFrom (g); }
        if (! isVisible() && hovered >= 0) { proc.fxApply (nodes[(size_t) centre].g); hovered = -1; }
    }
    void resized() override
    {
        int x = 0;
        for (auto* b : { &surpriseBtn, &againBtn, &dryBtn, &hearBtn }) { b->setBounds (x, 0, b == &dryBtn ? 70 : 130, 34); x += b->getWidth() + 8; }
        wildSl.setBounds (x + 8, 3, 160, 28);
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        if (nodes.empty()) return;
        const auto c = centrePos();
        const auto& cn = nodes[(size_t) centre];
        for (int k = 0; k < (int) cn.kids.size(); ++k)
        {
            const auto kp = kidPos (k);
            g.setGradientFill (ColourGradient (t.accent.withAlpha (0.5f), c.x, c.y, t.accent.withAlpha (0.1f), kp.x, kp.y, false));
            g.drawLine (Line<float> (c, kp), 1.5f);
        }
        for (int k = 0; k < (int) cn.kids.size(); ++k) bubble (g, cn.kids[(size_t) k], kidPos (k), kidR, false);
        bubble (g, centre, c, centreR, true);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.08f));
        g.drawText ("play the keys (or HEAR IT) - hover a bubble = hear your sound through it - click = keep it and grow",
                    Rectangle<float> (0, (float) getHeight() - 22, (float) getWidth(), 20), Justification::centred);
    }
    void mouseMove (const MouseEvent& e) override
    {
        const int h = nodeAt (e.position);
        if (h == hovered) return;
        hovered = h;
        if (h >= 0 && h != centre) proc.fxApply (nodes[(size_t) h].g);   // hear it now
        else proc.fxApply (nodes[(size_t) centre].g);
        repaint();
    }
    void mouseExit (const MouseEvent&) override { if (hovered >= 0) { hovered = -1; proc.fxApply (nodes[(size_t) centre].g); repaint(); } }
    void mouseUp (const MouseEvent& e) override
    {
        const int n = nodeAt (e.position);
        if (n < 0 || n == centre) return;
        centre = n; proc.fxApply (nodes[(size_t) n].g);
        grow (n, false);
    }
    std::function<void()> onApplied;
private:
    struct Node { KeysKillaProcessor::FxGenome g; int parent = -1; std::vector<int> kids; };
    std::vector<Node> nodes;
    int centre = 0, hovered = -1;
    static constexpr float centreR = 92.0f, kidR = 66.0f;
    Point<float> centrePos() const { return { (float) getWidth() * 0.5f, 44.0f + ((float) getHeight() - 70.0f) * 0.5f }; }
    Point<float> kidPos (int k) const
    {
        const float ang = MathConstants<float>::twoPi * (float) k / 6.0f - MathConstants<float>::halfPi + 0.52f;
        const float ry = std::min (230.0f, ((float) getHeight() - 70.0f) * 0.5f - kidR - 4.0f), rx = std::min ((float) getWidth() * 0.36f, ry * 1.55f);
        return { centrePos().x + std::cos (ang) * rx, centrePos().y + std::sin (ang) * ry };
    }
    int nodeAt (Point<float> p) const
    {
        if (nodes.empty()) return -1;
        if (p.getDistanceFrom (centrePos()) < centreR) return centre;
        const auto& cn = nodes[(size_t) centre];
        for (int k = 0; k < (int) cn.kids.size(); ++k) if (p.getDistanceFrom (kidPos (k)) < kidR) return cn.kids[(size_t) k];
        return -1;
    }
    void startFrom (const KeysKillaProcessor::FxGenome& g)
    {
        nodes.clear(); Node n; n.g = g; nodes.push_back (n); centre = 0; hovered = -1;
        proc.fxApply (g);
        grow (0, false);
    }
    void surprise()
    {
        auto& cn = nodes[(size_t) centre];
        cn.kids.clear();
        const uint32_t base = (uint32_t) Time::getMillisecondCounter() * 2654435761u;
        for (int k = 0; k < 6; ++k) { Node n; n.g = proc.fxSurprise (kk::hash32 (base + (uint32_t) k * 7919u)); n.parent = centre; nodes.push_back (n); nodes[(size_t) centre].kids.push_back ((int) nodes.size() - 1); }
        repaint(); if (onApplied) onApplied();
    }
    void grow (int node, bool reroll)
    {
        auto& cn = nodes[(size_t) node];
        if (! cn.kids.empty() && ! reroll) { repaint(); return; }
        nodes[(size_t) node].kids.clear();
        const uint32_t base = (uint32_t) Time::getMillisecondCounter() * 2246822519u + (uint32_t) node;
        const auto parentG = nodes[(size_t) node].g;
        int empty = 0; for (auto o : parentG.on) empty += o ? 0 : 1;
        for (int k = 0; k < 6; ++k)
        {
            Node n;
            // a dry middle gets surprises, a chain gets its variations (two of them wilder)
            n.g = empty == kk::numRackSlots ? proc.fxSurprise (kk::hash32 (base + (uint32_t) k * 31u))
                                            : proc.fxMutate (parentG, jlimit (0.0f, 1.0f, (float) wildSl.getValue() + (k >= 4 ? 0.3f : 0.0f)), kk::hash32 (base + (uint32_t) k * 131u));
            n.parent = node; nodes.push_back (n);
            nodes[(size_t) node].kids.push_back ((int) nodes.size() - 1);
        }
        if (nodes.size() > 2000) { auto keep = nodes[(size_t) centre].g; startFrom (keep); return; }
        repaint(); if (onApplied) onApplied();
    }
    void bubble (Graphics& g, int node, Point<float> c, float r, bool isCentre)
    {
        const auto& t = kk::theme();
        const auto& n = nodes[(size_t) node];
        const bool hot = node == hovered;
        g.setColour (Colours::black.withAlpha (t.night ? 0.4f : 0.14f)); g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c.translated (5, 10)));
        g.setGradientFill (ColourGradient (t.night ? Colour (0xd8262a31) : Colour (0xe6ffffff), c.x - r * 0.4f, c.y - r * 0.6f,
                                           t.night ? Colour (0xd0101215) : Colour (0xd6d9dde2), c.x + r * 0.5f, c.y + r, true));
        g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c));
        if (isCentre || hot) { g.setGradientFill (ColourGradient (t.accent.withAlpha (0.22f), c.x, c.y, t.accent.withAlpha (0.0f), c.x + r * 1.4f, c.y, true)); g.fillEllipse (Rectangle<float> (r * 2.8f, r * 2.8f).withCentre (c)); }
        g.setColour (isCentre || hot ? t.accent : t.text.withAlpha (0.3f));
        g.drawEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c).reduced (0.5f), isCentre ? 2.4f : hot ? 2.0f : 1.2f);
        g.setColour (t.text); g.setFont (kk::modern::font (isCentre ? 16.0f : 13.5f, true, 0.04f));
        g.drawFittedText (n.g.name.isEmpty() ? String ("FX") : n.g.name, Rectangle<int> ((int) (c.x - r), (int) (c.y - r * 0.62f), (int) (r * 2), 22), Justification::centred, 1, 0.7f);
        // the effects in it, as little chips
        StringArray on;
        for (int s = 0; s < kk::numRackSlots; ++s) if (n.g.on[(size_t) s]) on.add (kk::rackSlotName (s));
        g.setFont (kk::modern::font (isCentre ? 11.5f : 10.5f, true, 0.06f));
        float y = c.y - r * 0.22f;
        for (int i = 0; i < on.size() && i < 4; ++i)
        {
            const auto cr = Rectangle<float> (c.x - r * 0.7f, y, r * 1.4f, isCentre ? 18.0f : 15.0f);
            g.setColour (t.accent.withAlpha (0.16f)); g.fillRoundedRectangle (cr, 7);
            g.setColour (t.text.withAlpha (0.9f)); g.drawText (on[i], cr, Justification::centred);
            y += cr.getHeight() + 3;
        }
        if (on.isEmpty()) { g.setColour (t.dim); g.drawText ("DRY", Rectangle<float> (c.x - r, c.y - 8, r * 2, 16), Justification::centred); }
        if (isCentre) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (10.5f, true, 0.3f)); g.drawText ("ON YOUR SOUND", Rectangle<float> (c.x - r, c.y + r * 0.48f, r * 2, 14), Justification::centred); }
    }
    void timerCallback() override { if (isShowing()) hearBtn.setButtonText (proc.loopPlaying() ? "STOP" : "HEAR IT"); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton surpriseBtn { lnf }, againBtn { lnf }, dryBtn { lnf }, hearBtn { lnf };
    Slider wildSl;
};

//==============================================================================
// v0.37 STEP FX: effects that play in time - paint them into 16 steps (stutter, reverse, tape stop, filter, gate, echo, octave, crush)
class StepFxView : public Component, private Timer
{
public:
    StepFxView (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (powerBtn, "OFF", "Switch STEP FX on / off (it runs in time with FL)", [this] { proc.stepOn = ! proc.stepOn.load(); refresh(); });
        btn (diceBtn, "DICE", "A new pattern", [this] { proc.stepRandom ((uint32_t) Time::getMillisecondCounter(), 0.55f); proc.stepOn = true; refresh(); });
        btn (mutateBtn, "MUTATE", "Change a few steps of the pattern", [this] { mutate(); });
        btn (clearBtn, "CLEAR", "Empty grid", [this] { proc.stepClear(); refresh(); });
        btn (hearBtn, "HEAR IT", "Play a melody loop with your sound so you hear the steps - again = stop", [this] { proc.toggleLoop(); });
        static const char* presets[] { "ROLL", "CHOP", "BRAKE", "MIRROR", "GLITCH", "SWEEP", "BUILD", "BOUNCE", "DARK" };
        for (int i = 0; i < 9; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, presets[i]); b->framed = true;
            b->setTooltip ("A ready pattern: " + String (presets[i]));
            b->onClick = [this, i] { proc.stepPreset (i); proc.stepOn = true; refresh(); };
            addAndMakeVisible (*b); presetBtns.push_back (std::move (b));
        }
        static const char* rates[] { "1/8", "1/16", "1/32" };
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, rates[i]); b->framed = true;
            b->setTooltip ("How long one step is");
            b->onClick = [this, i] { proc.stepRate = i; refresh(); };
            addAndMakeVisible (*b); rateBtns.push_back (std::move (b));
        }
        mixSl.setSliderStyle (Slider::LinearBar); mixSl.setRange (0.0, 1.0, 0.01); mixSl.setValue (proc.stepMix.load(), dontSendNotification);
        mixSl.textFromValueFunction = [] (double v) { return "MIX " + String (roundToInt (v * 100)) + "%"; };
        mixSl.setColour (Slider::trackColourId, lnf.skin->accent.withAlpha (0.5f));
        mixSl.setColour (Slider::textBoxTextColourId, kk::theme().text);
        mixSl.onValueChange = [this] { proc.stepMix = (float) mixSl.getValue(); };
        mixSl.updateText();
        addAndMakeVisible (mixSl);
        startTimerHz (30);
        refresh();
    }
    void resized() override
    {
        int x = 0;
        for (auto* b : { &powerBtn, &diceBtn, &mutateBtn, &clearBtn, &hearBtn }) { b->setBounds (x, 0, b == &hearBtn ? 104 : 84, 34); x += b->getWidth() + 6; }
        x += 10;
        for (auto& b : rateBtns) { b->setBounds (x, 0, 58, 34); x += 62; }
        mixSl.setBounds (x + 10, 3, 150, 28);
        x = 0;
        const int pw = jmin (96, (getWidth() - 8 * 6) / 9);
        for (auto& b : presetBtns) { b->setBounds (x, 44, pw, 30); x += pw + 6; }
    }
    static Colour rowColour (int f)
    {
        static const uint32 c[] { 0xffff4d6d, 0xffa78bfa, 0xffff8a3d, 0xff22d3ee, 0xffffd23f, 0xff4d7dff, 0xff36ff6a, 0xffff4fd8, 0xffff3b5c, 0xff7c4dff, 0xff2ee6a6, 0xfff5f56a };
        return Colour (c[jlimit (0, 11, f)]);
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const auto gr = grid();
        const float cw = gr.getWidth() / (float) KeysKillaProcessor::numSteps, rh = gr.getHeight() / (float) KeysKillaProcessor::numStepFx;
        const int now = proc.stepNow.load();
        const bool running = proc.stepOn.load();
        // the playing column glows
        if (running && now >= 0)
        {
            const auto col = Rectangle<float> (gr.getX() + cw * (float) now, gr.getY(), cw, gr.getHeight());
            g.setGradientFill (ColourGradient (t.text.withAlpha (0.10f), col.getCentreX(), gr.getY(), t.text.withAlpha (0.02f), col.getCentreX(), gr.getBottom(), false));
            g.fillRoundedRectangle (col, 6);
        }
        for (int st = 0; st < KeysKillaProcessor::numSteps; st += 4)
        {
            g.setColour (t.text.withAlpha (0.05f));
            g.fillRoundedRectangle (Rectangle<float> (gr.getX() + cw * (float) st, gr.getY() - 2, cw * 4, gr.getHeight() + 4).reduced (2, 0), 6);
            g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.1f));
            g.drawText (String (st / 4 + 1), Rectangle<float> (gr.getX() + cw * (float) st, gr.getY() - 16, cw, 14), Justification::centredLeft);
        }
        for (int f = 0; f < KeysKillaProcessor::numStepFx; ++f)
        {
            const float y = gr.getY() + rh * (float) f;
            const auto c = rowColour (f);
            g.setColour (c); g.fillRoundedRectangle (4, y + rh * 0.5f - 4, 8, 8, 3);
            g.setColour (t.text); g.setFont (kk::modern::font (12.5f, true, 0.08f));
            g.drawText (KeysKillaProcessor::stepFxName (f), Rectangle<float> (16, y, labelW - 24, rh), Justification::centredLeft);
            for (int st = 0; st < KeysKillaProcessor::numSteps; ++st)
            {
                const auto cell = Rectangle<float> (gr.getX() + cw * (float) st, y, cw, rh).reduced (2.5f);
                g.setColour (t.glass.withMultipliedAlpha (st % 4 == 0 ? 1.5f : 1.0f)); g.fillRoundedRectangle (cell, 4);
            }
            // runs of steps become one long coloured block (an effect that lasts)
            for (int st = 0; st < KeysKillaProcessor::numSteps;)
            {
                if (! proc.stepGrid[(size_t) f][(size_t) st].load()) { ++st; continue; }
                int e = st; while (e + 1 < KeysKillaProcessor::numSteps && proc.stepGrid[(size_t) f][(size_t) e + 1].load()) ++e;
                const auto blk = Rectangle<float> (gr.getX() + cw * (float) st, y, cw * (float) (e - st + 1), rh).reduced (2.5f);
                const bool live = running && now >= st && now <= e;
                if (live) { g.setColour (c.withAlpha (0.35f)); g.fillRoundedRectangle (blk.expanded (3), 7); }
                g.setGradientFill (ColourGradient (c.brighter (live ? 0.5f : 0.15f), blk.getX(), blk.getY(), c.darker (0.25f), blk.getX(), blk.getBottom(), false));
                g.fillRoundedRectangle (blk, 5);
                g.setColour (Colours::white.withAlpha (live ? 0.9f : 0.35f)); g.drawRoundedRectangle (blk, 5, live ? 1.6f : 1.0f);
                // a tiny picture of what it does
                g.setColour (Colours::black.withAlpha (0.45f));
                const float mx = blk.getX() + 6, my = blk.getCentreY(), w = jmin (blk.getWidth() - 12, 26.0f), hh = blk.getHeight() * 0.28f;
                Path p;
                switch (f)
                {
                    case KeysKillaProcessor::sfStutter: case KeysKillaProcessor::sfRoll: { const int nn = f == KeysKillaProcessor::sfRoll ? 6 : 3; for (int k = 0; k < nn; ++k) g.fillRect (mx + w * (float) k / (float) nn, my - hh, w / (float) nn * 0.6f, hh * 2); break; }
                    case KeysKillaProcessor::sfReverse: p.startNewSubPath (mx, my + hh); p.lineTo (mx + w, my - hh); p.lineTo (mx + w, my + hh); p.closeSubPath(); g.fillPath (p); break;
                    case KeysKillaProcessor::sfTape: p.startNewSubPath (mx, my - hh); p.quadraticTo (mx + w * 0.6f, my - hh, mx + w, my + hh); g.strokePath (p, PathStrokeType (2.0f)); break;
                    case KeysKillaProcessor::sfFilter: p.startNewSubPath (mx, my - hh); p.lineTo (mx + w * 0.5f, my - hh); p.lineTo (mx + w, my + hh); g.strokePath (p, PathStrokeType (2.0f)); break;
                    case KeysKillaProcessor::sfRiser: p.startNewSubPath (mx, my + hh); p.lineTo (mx + w * 0.5f, my - hh); p.lineTo (mx + w, my - hh); g.strokePath (p, PathStrokeType (2.0f)); break;
                    case KeysKillaProcessor::sfGate: g.fillRect (mx, my - hh, w * 0.45f, hh * 2); break;
                    case KeysKillaProcessor::sfEcho: for (int k = 0; k < 3; ++k) g.fillRect (mx + w * 0.35f * (float) k, my - hh * (1.0f - 0.3f * (float) k), 3.0f, hh * 2 * (1.0f - 0.3f * (float) k)); break;
                    case KeysKillaProcessor::sfPitchUp: case KeysKillaProcessor::sfPitchDown:
                    { const float d = f == KeysKillaProcessor::sfPitchUp ? -1.0f : 1.0f; p.startNewSubPath (mx + w * 0.5f, my + d * hh); p.lineTo (mx + w * 0.5f, my - d * hh); p.lineTo (mx + w * 0.2f, my - d * hh * 0.4f); g.strokePath (p, PathStrokeType (2.0f)); break; }
                    case KeysKillaProcessor::sfPan: g.fillEllipse (mx, my - 3, 6, 6); g.fillEllipse (mx + w - 6, my - 3, 6, 6); break;
                    default: for (int k = 0; k < 4; ++k) g.fillRect (mx + w * (float) k / 4.0f, my + hh - hh * 2 * (float) ((k * 3) % 4) / 3.0f, w / 4.0f, 2.5f); break;
                }
                st = e + 1;
            }
            if (running && now >= 0)
            {
                const auto cell = Rectangle<float> (gr.getX() + cw * (float) now, y, cw, rh).reduced (2.5f);
                g.setColour (t.text.withAlpha (0.25f)); g.drawRoundedRectangle (cell, 4, 1.2f);
            }
        }
        g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.06f));
        g.drawText ("click / drag = paint steps (neighbours join into one long effect)   -   in time with FL on every melody   -   right-click a row = clear it",
                    Rectangle<float> (0, (float) getHeight() - 20, (float) getWidth(), 18), Justification::centred);
    }
    void mouseDown (const MouseEvent& e) override
    {
        const auto c = cellAt (e.position);
        if (c.x < 0) return;
        if (e.mods.isPopupMenu()) { for (int st = 0; st < KeysKillaProcessor::numSteps; ++st) proc.stepGrid[(size_t) c.y][(size_t) st] = false; refresh(); return; }
        paintValue = ! proc.stepGrid[(size_t) c.y][(size_t) c.x].load();
        proc.stepGrid[(size_t) c.y][(size_t) c.x] = paintValue;
        if (paintValue) proc.stepOn = true;
        refresh();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        const auto c = cellAt (e.position);
        if (c.x < 0) return;
        if (proc.stepGrid[(size_t) c.y][(size_t) c.x].load() != paintValue) { proc.stepGrid[(size_t) c.y][(size_t) c.x] = paintValue; repaint(); }
    }
private:
    static constexpr float labelW = 136.0f;
    Rectangle<float> grid() const { return { labelW, 100.0f, (float) getWidth() - labelW, (float) getHeight() - 100.0f - 26.0f }; }
    Point<int> cellAt (Point<float> p) const
    {
        const auto gr = grid();
        if (! gr.contains (p)) return { -1, -1 };
        return { jlimit (0, KeysKillaProcessor::numSteps - 1, (int) ((p.x - gr.getX()) / gr.getWidth() * KeysKillaProcessor::numSteps)),
                 jlimit (0, KeysKillaProcessor::numStepFx - 1, (int) ((p.y - gr.getY()) / gr.getHeight() * KeysKillaProcessor::numStepFx)) };
    }
    void mutate()
    {
        Random r;
        for (int k = 0; k < 4; ++k)
        {
            const int f = r.nextInt (KeysKillaProcessor::numStepFx), st = r.nextInt (KeysKillaProcessor::numSteps);
            proc.stepGrid[(size_t) f][(size_t) st] = ! proc.stepGrid[(size_t) f][(size_t) st].load();
        }
        proc.stepOn = true; refresh();
    }
    void refresh()
    {
        powerBtn.setButtonText (proc.stepOn.load() ? "ON" : "OFF"); powerBtn.selected = proc.stepOn.load(); powerBtn.repaint();
        for (int i = 0; i < 3; ++i) { rateBtns[(size_t) i]->selected = proc.stepRate.load() == i; rateBtns[(size_t) i]->repaint(); }
        repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        const int now = proc.stepNow.load();
        if (now != lastNow) { lastNow = now; repaint (grid().toNearestInt().expanded (4)); }
        hearBtn.setButtonText (proc.loopPlaying() ? "STOP" : "HEAR IT");
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton powerBtn { lnf }, diceBtn { lnf }, mutateBtn { lnf }, clearBtn { lnf }, hearBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> presetBtns, rateBtns;
    Slider mixSl;
    bool paintValue = true;
    int lastNow = -2;
};

class FxRackPage : public Component
{
public:
    FxRackPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        for (int s = 0; s < kk::numRackSlots; ++s)
        {
            cards.push_back (std::make_unique<RackCard> (proc, s));
            cards.back()->onChange = [this] { repaint(); };
            addAndMakeVisible (*cards.back());
        }
        allOff.setButtonText ("ALL OFF"); allOff.framed = true; allOff.tint = TC (0xffff2f6d);
        allOff.setTooltip ("Switch every effect of the rack off");
        allOff.onClick = [this] { for (auto& o : proc.rack.on) o = false; for (auto& c : cards) c->repaint(); repaint(); };
        addAndMakeVisible (allOff);
        reset.setButtonText ("RESET"); reset.framed = true; reset.tint = TC (0xff22d3ee);
        reset.setTooltip ("Every effect off, every knob back to its start");
        reset.onClick = [this] { proc.rack.reset(); for (auto& c : cards) c->sync(); repaint(); };
        addAndMakeVisible (reset);
        static const char* tabNames[] { "SURPRISE FX", "STEP FX", "RACK" };
        static const char* tabTips[] { "EVOLVE for effects: hover = hear a chain on your sound, click = keep it and grow",
                                       "Effects in time: stutter, reverse, tape stop, filter, gate, echo - paint them into 16 steps",
                                       "Every effect with its knobs" };
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, tabNames[i]); b->framed = true; b->setTooltip (tabTips[i]);
            b->onClick = [this, i] { setMode (i); };
            addAndMakeVisible (*b); tabs.push_back (std::move (b));
        }
        evolveView = std::make_unique<FxEvolveView> (proc, lnf);
        evolveView->onApplied = [this] { for (auto& c : cards) c->sync(); repaint(); };
        addChildComponent (*evolveView);
        stepView = std::make_unique<StepFxView> (proc, lnf);
        addChildComponent (*stepView);
        setOpaque (true);
        setMode (lastMode());
    }
    void visibilityChanged() override { if (isVisible()) { for (auto& c : cards) c->sync(); setMode (lastMode()); } }
    static int& lastMode() { static int m = 0; return m; }
    void setMode (int m)
    {
        lastMode() = m;
        for (int i = 0; i < 3; ++i) { tabs[(size_t) i]->selected = i == m; tabs[(size_t) i]->repaint(); }
        evolveView->setVisible (m == 0); stepView->setVisible (m == 1);
        for (auto& c : cards) { c->setVisible (m == 2); if (m == 2) c->sync(); }
        allOff.setVisible (m == 2); reset.setVisible (m == 2);
        repaint();
    }
    void paint (Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        pageBackdrop (g, *this);
        auto glow = [&] (Point<float> c, float rad, Colour col) { g.setGradientFill (ColourGradient (col.withAlpha (0.16f), c.x, c.y, col.withAlpha (0.0f), c.x + rad, c.y, true)); g.fillEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (c)); };
        glow ({ r.getWidth() * 0.2f, 40 }, 360, TC (0xffff3fd2)); glow ({ r.getWidth() * 0.85f, r.getBottom() }, 380, TC (0xff22d3ee));
        kk::modern::waves (g, { r.getWidth() * 0.35f, 46.0f }, { r.getWidth(), 6.0f }, 20.0f, TC (0xffff3fd2), TC (0xff9b4dff), 5, 0.22f);
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (30.0f, Font::bold)).withExtraKerningFactor (0.06f));
        g.drawText ("FX", 24, 10, 60, 40, Justification::centredLeft);
        int on = 0; for (auto& o : proc.rack.on) on += o.load() ? 1 : 0;
        g.setColour (on > 0 ? TC (0xff36ff6a) : TC (0xff6a6290)); g.setFont (Font (FontOptions (14.0f, Font::bold)));
        g.drawText (String (on) + (on == 1 ? " EFFECT ON" : " EFFECTS ON") + (proc.stepOn.load() ? "   +   STEP FX ON" : ""), 470, 16, getWidth() - 740, 26, Justification::centredLeft);
    }
    void resized() override
    {
        reset.setBounds (getWidth() - 132, 14, 116, 36);
        allOff.setBounds (getWidth() - 256, 14, 116, 36);
        int tx = 84;
        for (auto& b : tabs) { b->setBounds (tx, 14, b.get() == tabs[1].get() ? 104 : 124, 36); tx += b->getWidth() + 8; }
        evolveView->setBounds (getLocalBounds().reduced (16).withTrimmedTop (56));
        stepView->setBounds (getLocalBounds().reduced (16).withTrimmedTop (60));
        auto r = getLocalBounds().reduced (14).withTrimmedTop (56);
        const int cols = 4, rows = 3, gap = 10;
        const int cw = (r.getWidth() - gap * (cols - 1)) / cols, ch = (r.getHeight() - gap * (rows - 1)) / rows;
        for (int i = 0; i < (int) cards.size(); ++i)
            cards[(size_t) i]->setBounds (r.getX() + (i % cols) * (cw + gap), r.getY() + (i / cols) * (ch + gap), cw, ch);
    }
private:
    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
    std::vector<std::unique_ptr<RackCard>> cards;
    HotButton allOff { lnf }, reset { lnf };
    std::vector<std::unique_ptr<HotButton>> tabs;
    std::unique_ptr<FxEvolveView> evolveView;
    std::unique_ptr<StepFxView> stepView;
};

//==============================================================================
// v0.37 EDIT for samples: when the sound on the keys is a sample (your sound, an audio child, a bank sound you clicked,
// the SAMPLER), EDIT shows this page. Turn a knob, play the keys: you hear it at once. SAVE / DRAG give the edited sound.
class SampleEditPage : public Component, private Timer
{
public:
    SampleEditPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        using P = KeysKillaProcessor;
        const auto a = lnf.skin->accent, b = lnf.skin->accent.brighter (0.3f);
        auto knob = [&] (int id, double lo, double hi, std::function<String (double)> txt)
        {
            auto k = std::make_unique<RackKnob> (proc.sampleEdit[(size_t) id], P::sampleEditName (id), lo, hi, P::sampleEditDefault (id), a, b);
            k->valueText = std::move (txt);
            addAndMakeVisible (*k);
            knobs.push_back (std::move (k)); knobIds.push_back (id);
        };
        auto pct = [] (double v) { return String (roundToInt (v * 100)) + " %"; };
        knob (P::seTune, -24, 24, [] (double v) { return (v > 0 ? "+" : "") + String (roundToInt (v)) + " st"; });
        knob (P::seFine, -100, 100, [] (double v) { return (v > 0 ? "+" : "") + String (roundToInt (v)) + " ct"; });
        knob (P::seStart, 0, 0.9, pct);
        knob (P::seAttack, 0, 1.0, [] (double v) { return String (roundToInt (v * 1000)) + " ms"; });
        knob (P::seRelease, 0.01, 4.0, [] (double v) { return String (v, 2) + " s"; });
        knob (P::seTone, -1, 1, [] (double v) { return v < -0.02 ? "DARK " + String (roundToInt (-v * 100)) : v > 0.02 ? "BRIGHT " + String (roundToInt (v * 100)) : String ("FLAT"); });
        knob (P::seLowCut, 0, 1, [] (double v) { return v < 0.01 ? String ("OFF") : String (roundToInt (20.0 * std::pow (50.0, v))) + " Hz"; });
        knob (P::seDrive, 0, 1, pct);
        knob (P::seCrush, 0, 1, pct);
        knob (P::seChorus, 0, 1, pct);
        knob (P::seSpace, 0, 1, pct);
        knob (P::seEcho, 0, 1, pct);
        knob (P::seWidth, 0, 1, pct);
        knob (P::seGain, -12, 12, [] (double v) { return String (v, 1) + " dB"; });
        auto btn = [this] (HotButton& bt, const String& t, const String& tip, std::function<void()> fn) { bt.setButtonText (t); bt.framed = true; bt.setTooltip (tip); bt.onClick = std::move (fn); addAndMakeVisible (bt); };
        btn (revBtn, "REVERSE", "Play the sample backwards", [this] { proc.sampleEdit[P::seReverse] = proc.sampleEdit[P::seReverse].load() > 0.5f ? 0.0f : 1.0f; sync(); });
        btn (resetBtn, "RESET", "Every control back to the start (the sound as it was)", [this] { proc.resetSampleEdit(); sync(); });
        btn (surpriseBtn, "SURPRISE ME", "Random settings that still sound good - again = another one", [this] { surprise(); });
        btn (saveBtn, "SAVE", "Save the edited sound into your folders / sound kits", [this]
        { saveToFolderMenu (proc, { proc.editedSample() }, &saveBtn, [safe = Component::SafePointer<SampleEditPage> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->repaint(); } }); });
        btn (evolveBtn, "EVOLVE IT", "The edited sound becomes a new seed in EVOLVE - six children grow from it", [this]
        { if (auto e = proc.editedSample()) { proc.evoSeedSound (e); if (onEvolve) onEvolve(); } });
        btn (closeBtn, "CLOSE", "Back", [this] { if (onClose) onClose(); });
        dragWav.makeFile = [this] { return proc.exportEditedSample(); };
        dragWav.setTooltip ("Drag the edited sound into FL Studio as a WAV (tuned to C: FL's sampler plays it at C5)");
        addAndMakeVisible (dragWav);
        startTimerHz (15);
    }
    std::function<void()> onClose, onEvolve;
    void visibilityChanged() override { if (isVisible()) sync(); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        g.setColour (t.text); g.setFont (kk::modern::font (28.0f, true, 0.08f));
        g.drawText ("EDIT", 24, 12, 100, 40, Justification::centredLeft);
        const bool chop = proc.chopActive();
        auto smp = proc.activeSample();
        g.setColour (kk::accentText()); g.setFont (kk::modern::font (16.0f, true, 0.06f));
        g.drawText (chop ? String ("THE SAMPLER") : smp != nullptr ? smp->name : String ("no sample on the keys"), 112, 14, 520, 22, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, false, 0.04f));
        g.drawText (chop ? "TONE and SPACE colour the chops - play the pads or the keys" : "turn a knob and play the keys - you hear it at once.  SAVE / DRAG give you the edited sound",
                    112, 36, 640, 18, Justification::centredLeft);
        // the sample, with START and REVERSE shown
        const auto w = waveRect();
        kk::modern::well (g, w, 10.0f);
        if (smp != nullptr && ! smp->peaks.empty())
        {
            const bool rev = proc.sampleEdit[KeysKillaProcessor::seReverse].load() > 0.5f;
            const float st = proc.sampleEdit[KeysKillaProcessor::seStart].load();
            const int n = (int) smp->peaks.size();
            for (int i = 0; i < n; ++i)
            {
                const float v = smp->peaks[(size_t) (rev ? n - 1 - i : i)];
                const float x = w.getX() + 8 + (w.getWidth() - 16) * (float) i / (float) n, h = std::max (1.0f, v * (w.getHeight() * 0.42f));
                g.setColour ((float) i / (float) n < st ? t.dim.withAlpha (0.35f) : t.accent.withAlpha (0.9f));
                g.fillRect (x, w.getCentreY() - h, std::max (1.0f, (w.getWidth() - 16) / (float) n - 0.5f), h * 2);
            }
            const float sx = w.getX() + 8 + (w.getWidth() - 16) * st;
            g.setColour (t.text); g.fillRect (sx - 1, w.getY() + 4, 2.0f, w.getHeight() - 8);
            g.setFont (kk::modern::font (11.0f, true, 0.1f)); g.drawText ("START", Rectangle<float> (sx + 4, w.getY() + 4, 60, 14), Justification::centredLeft);
            if (rev) { g.setColour (kk::accentText()); g.drawText ("REVERSED", w.reduced (10, 6), Justification::topRight); }
        }
        // group captions
        static const char* groups[] { "SHAPE", "TONE", "SPACE" };
        for (int gI = 0; gI < 3; ++gI)
        {
            const auto gr = groupRect (gI);
            kk::modern::plate (g, gr, 14.0f);
            g.setColour (t.text); g.setFont (kk::modern::font (13.0f, true, 0.3f));
            g.drawText (groups[gI], gr.reduced (16, 8).removeFromTop (20), Justification::centredLeft);
        }
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (13.0f, true, 0.05f)); g.drawText (note, 24, getHeight() - 28, getWidth() - 48, 20, Justification::centredLeft); }
    }
    void resized() override
    {
        const int W = getWidth();
        closeBtn.setBounds (W - 110, 14, 96, 38);
        dragWav.setBounds (W - 110 - 196, 12, 186, 42);
        saveBtn.setBounds (dragWav.getX() - 94, 14, 86, 38);
        evolveBtn.setBounds (saveBtn.getX() - 124, 14, 116, 38);
        surpriseBtn.setBounds (evolveBtn.getX() - 140, 14, 132, 38);
        resetBtn.setBounds (surpriseBtn.getX() - 92, 14, 84, 38);
        // knobs: SHAPE (tune, fine, start, attack, release + reverse) / TONE (tone, low cut, drive, crush) / SPACE (chorus, space, echo, width, gain)
        const int per[3] { 5, 4, 5 };
        int k = 0;
        for (int gI = 0; gI < 3; ++gI)
        {
            auto gr = groupRect (gI).reduced (12, 10).withTrimmedTop (24).toNearestInt();
            if (gI == 0) { revBtn.setBounds (gr.removeFromBottom (40).withSizeKeepingCentre (150, 34)); }
            const int kw = gr.getWidth() / per[gI], kh = std::min (gr.getHeight(), std::min (kw, 120) + 26);
            const int ky = gr.getY() + (gr.getHeight() - kh) / 2;
            for (int i = 0; i < per[gI]; ++i, ++k) knobs[(size_t) k]->setBounds (gr.getX() + i * kw, ky, kw, kh);
        }
    }
private:
    Rectangle<float> waveRect() const { return { 20.0f, 66.0f, (float) getWidth() - 40.0f, 210.0f }; }
    Rectangle<float> groupRect (int gI) const
    {
        const float top = 292.0f, h = std::min (300.0f, (float) getHeight() - top - 34.0f), W = (float) getWidth() - 40.0f;
        const float w0 = W * 0.38f, w1 = W * 0.28f, w2 = W - w0 - w1 - 20.0f;
        const float x0 = 20.0f, x1 = x0 + w0 + 10.0f, x2 = x1 + w1 + 10.0f;
        return gI == 0 ? Rectangle<float> (x0, top, w0, h) : gI == 1 ? Rectangle<float> (x1, top, w1, h) : Rectangle<float> (x2, top, w2, h);
    }
    void sync()
    {
        for (auto& k : knobs) k->sync();
        revBtn.selected = proc.sampleEdit[KeysKillaProcessor::seReverse].load() > 0.5f; revBtn.repaint();
        const bool chop = proc.chopActive();
        for (size_t i = 0; i < knobs.size(); ++i)   // the SAMPLER: only the colour / space controls do something
        {
            const int id = knobIds[i];
            knobs[i]->setEnabled (! chop || id >= KeysKillaProcessor::seTone);
            knobs[i]->setAlpha (! chop || id >= KeysKillaProcessor::seTone ? 1.0f : 0.35f);
        }
        revBtn.setEnabled (! chop); saveBtn.setEnabled (! chop); evolveBtn.setEnabled (! chop); dragWav.setVisible (! chop);
        repaint();
    }
    void surprise()
    {
        Random r;
        using P = KeysKillaProcessor;
        auto set = [this] (int id, float v) { proc.sampleEdit[(size_t) id] = v; };
        proc.resetSampleEdit();
        static const float tunes[] { 0, 0, 0, 12, -12, 7, -5, 5 };
        set (P::seTune, tunes[r.nextInt (8)]);
        if (r.nextFloat() < 0.25f) set (P::seReverse, 1.0f);
        if (r.nextFloat() < 0.35f) set (P::seStart, r.nextFloat() * 0.3f);
        if (r.nextFloat() < 0.4f) set (P::seAttack, r.nextFloat() * 0.25f);
        set (P::seRelease, 0.08f + r.nextFloat() * 1.2f);
        set (P::seTone, (r.nextFloat() * 2.0f - 1.0f) * 0.7f);
        if (r.nextFloat() < 0.4f) set (P::seDrive, r.nextFloat() * 0.45f);
        if (r.nextFloat() < 0.3f) set (P::seCrush, r.nextFloat() * 0.5f);
        if (r.nextFloat() < 0.4f) set (P::seChorus, r.nextFloat() * 0.6f);
        set (P::seSpace, r.nextFloat() * 0.55f);
        if (r.nextFloat() < 0.4f) set (P::seEcho, r.nextFloat() * 0.45f);
        set (P::seWidth, 0.4f + r.nextFloat() * 0.5f);
        sync();
        if (! proc.chopActive()) if (auto smp = proc.activeSample()) proc.previewNote = smp->rootNote;   // hear it
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        auto smp = proc.activeSample().get();
        const bool chop = proc.chopActive();
        if (smp != lastSmp || chop != lastChop) { lastSmp = smp; lastChop = chop; sync(); }
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    std::vector<std::unique_ptr<RackKnob>> knobs; std::vector<int> knobIds;
    HotButton revBtn { lnf }, resetBtn { lnf }, surpriseBtn { lnf }, saveBtn { lnf }, evolveBtn { lnf }, closeBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    String note;
    const void* lastSmp = nullptr; bool lastChop = false;
};

// left tiles: each one a small colour badge (its own two colours) with a white symbol
static Colour tileCol (int k, int which)
{
    static const uint32 c[8][2] { { 0xffff2f6d, 0xffff8a3d },   // BREED LAB    pink -> orange
                                  { 0xffb04dff, 0xffff4fa8 },   // FAMILY TREE  violet -> pink
                                  { 0xff22d3ee, 0xff4d7dff },   // PAIR OWN     cyan -> blue
                                  { 0xff4d7dff, 0xff9b4dff },   // PAIR VST     blue -> violet
                                  { 0xffffb020, 0xffff6a3d },   // MY SOUNDS    gold -> orange
                                  { 0xff2ee6a6, 0xff1a9dff },   // SAMPLER      mint -> blue
                                  { 0xffff5a1f, 0xffff2f6d },   // DRUM KIT     orange -> red
                                  { 0xffff3fd2, 0xff9b4dff } }; // FX RACK      magenta -> violet
    return TC (c[jlimit (0, 7, k)][jlimit (0, 1, which)]);   // v0.34: theme colours
}
static void drawTileIcon (Graphics& g, int k, Rectangle<float> r, bool lit)
{
    // v0.34: a quiet glass badge with a line icon - amber when the page is open
    const auto& th = kk::theme();
    const Colour ink = lit ? th.accent : th.text.withAlpha (0.85f);
    if (lit) { g.setColour (th.accent.withAlpha (0.14f)); g.fillRoundedRectangle (r, 8); }
    g.setColour (lit ? th.accent.withAlpha (0.8f) : th.text.withAlpha (0.22f)); g.drawRoundedRectangle (r.reduced (0.5f), 8, 1.0f);

    // symbol
    const auto q = r.reduced (r.getWidth() * 0.22f);
    const float cx = q.getCentreX(), cy = q.getCentreY(), w = q.getWidth();
    PathStrokeType st (1.5f, PathStrokeType::curved, PathStrokeType::rounded);
    Path p;
    g.setColour (ink);
    switch (k)
    {
        case 0:   // BREED LAB: two sounds melt into one, with a spark
        {
            const float rr = w * 0.30f;
            p.addEllipse (cx - rr * 1.55f, cy - rr + 1, rr * 2, rr * 2); p.addEllipse (cx - rr * 0.45f, cy - rr + 1, rr * 2, rr * 2);
            g.strokePath (p, st);
            g.setColour (ink.withAlpha (0.55f));
            g.fillEllipse (cx - rr * 0.45f, cy - rr * 0.55f + 1, rr * 0.9f, rr * 1.1f);
            g.setColour (ink);
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
            g.setColour (ink.withAlpha (0.55f)); g.strokePath (p2, st);
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
        case 5:   // SAMPLER / CHOP: a waveform with two cuts
        {
            for (int i = 0; i <= 20; ++i)
            {
                const float x = q.getX() + w * (float) i / 20.0f;
                const float h = w * 0.42f * std::abs (std::sin ((float) i * 0.9f)) * (0.4f + 0.6f * std::exp (-(float) (i % 7) * 0.35f));
                g.fillRoundedRectangle (x - 0.8f, cy - h, 1.6f, h * 2, 0.8f);
            }
            g.setColour (ink.withAlpha (0.9f));
            for (float fx : { 0.33f, 0.66f })
            {
                const float x = q.getX() + w * fx;
                g.drawLine (x, q.getY() - 2, x, q.getBottom() + 2, 1.4f);
                Path t; t.addTriangle (x - 3, q.getY() - 2, x + 3, q.getY() - 2, x, q.getY() + 3); g.fillPath (t);
            }
            break;
        }
        case 6:   // DRUM KIT: six pads, two of them lit
        {
            const float pw = w * 0.28f, gp = w * 0.08f;
            for (int i = 0; i < 6; ++i)
            {
                const auto pr = Rectangle<float> (q.getX() + (float) (i % 3) * (pw + gp), cy - pw - gp * 0.5f + (float) (i / 3) * (pw + gp), pw, pw);
                if (i == 0 || i == 4) g.fillRoundedRectangle (pr, 2.0f);
                else g.drawRoundedRectangle (pr, 2.0f, 1.3f);
            }
            break;
        }
        case 9:   // MELODY: two notes with a beam
        {
            const float rr = w * 0.13f;
            const Point<float> n1 (q.getX() + w * 0.22f, q.getBottom() - rr), n2 (q.getRight() - w * 0.12f, q.getBottom() - rr * 2.2f);
            g.fillEllipse (n1.x - rr * 1.2f, n1.y - rr, rr * 2.4f, rr * 2.0f);
            g.fillEllipse (n2.x - rr * 1.2f, n2.y - rr, rr * 2.4f, rr * 2.0f);
            g.drawLine (n1.x + rr * 1.1f, n1.y, n1.x + rr * 1.1f, q.getY() + 2, 1.6f);
            g.drawLine (n2.x + rr * 1.1f, n2.y, n2.x + rr * 1.1f, q.getY() - 2 + rr, 1.6f);
            g.drawLine (n1.x + rr * 1.1f, q.getY() + 2, n2.x + rr * 1.1f, q.getY() - 2 + rr, 3.0f);
            break;
        }
        case 11:  // SOUND WORLD: a globe
        {
            const float rr = w * 0.48f;
            g.drawEllipse (cx - rr, cy - rr, rr * 2, rr * 2, 1.5f);
            g.drawEllipse (cx - rr * 0.45f, cy - rr, rr * 0.9f, rr * 2, 1.0f);
            g.drawLine (cx - rr, cy, cx + rr, cy, 1.0f);
            g.drawLine (cx - rr * 0.85f, cy - rr * 0.5f, cx + rr * 0.85f, cy - rr * 0.5f, 0.8f);
            g.drawLine (cx - rr * 0.85f, cy + rr * 0.5f, cx + rr * 0.85f, cy + rr * 0.5f, 0.8f);
            break;
        }
        case 10:  // MIX LAB: an EQ curve over three faders
        {
            Path c;
            c.startNewSubPath (q.getX(), cy + w * 0.1f);
            c.cubicTo (q.getX() + w * 0.3f, cy - w * 0.45f, q.getX() + w * 0.55f, cy + w * 0.45f, q.getRight(), cy - w * 0.15f);
            g.strokePath (c, PathStrokeType (1.8f, PathStrokeType::curved, PathStrokeType::rounded));
            for (int i = 0; i < 3; ++i)
            {
                const float x = q.getX() + w * (0.2f + 0.3f * (float) i);
                g.drawLine (x, q.getBottom() - w * 0.28f, x, q.getBottom(), 1.0f);
                g.fillRoundedRectangle (x - 2.5f, q.getBottom() - w * (0.12f + 0.08f * (float) i), 5.0f, 3.0f, 1.0f);
            }
            break;
        }
        case 12:  // ALCHEMY: a flask with bubbles
        {
            Path f; f.startNewSubPath (cx - w * 0.18f, cy - w * 0.5f); f.lineTo (cx - w * 0.18f, cy - w * 0.1f); f.lineTo (cx - w * 0.45f, cy + w * 0.45f);
            f.lineTo (cx + w * 0.45f, cy + w * 0.45f); f.lineTo (cx + w * 0.18f, cy - w * 0.1f); f.lineTo (cx + w * 0.18f, cy - w * 0.5f);
            g.strokePath (f, PathStrokeType (1.5f, PathStrokeType::curved, PathStrokeType::rounded));
            g.drawLine (cx - w * 0.26f, cy - w * 0.5f, cx + w * 0.26f, cy - w * 0.5f, 1.5f);
            g.fillEllipse (cx - w * 0.15f, cy + w * 0.15f, w * 0.14f, w * 0.14f); g.drawEllipse (cx + w * 0.05f, cy + w * 0.02f, w * 0.12f, w * 0.12f, 1.0f);
            break;
        }
        case 13:  // LIFE: a ball bouncing along an arc
        {
            Path a; a.startNewSubPath (cx - w * 0.5f, cy + w * 0.4f); a.quadraticTo (cx - w * 0.25f, cy - w * 0.4f, cx, cy + w * 0.4f); a.quadraticTo (cx + w * 0.18f, cy - w * 0.05f, cx + w * 0.36f, cy + w * 0.4f);
            g.strokePath (a, PathStrokeType (1.2f, PathStrokeType::curved, PathStrokeType::rounded));
            g.drawLine (cx - w * 0.5f, cy + w * 0.46f, cx + w * 0.5f, cy + w * 0.46f, 1.2f);
            g.fillEllipse (cx + w * 0.3f, cy + w * 0.18f, w * 0.18f, w * 0.18f);
            break;
        }
        case 14:  // FEED: two cards, the front one swiped
        {
            g.drawRoundedRectangle (cx - w * 0.42f, cy - w * 0.38f, w * 0.6f, w * 0.8f, 3.0f, 1.0f);
            g.addTransform (AffineTransform::rotation (0.3f, cx, cy + w * 0.4f));
            g.drawRoundedRectangle (cx - w * 0.2f, cy - w * 0.45f, w * 0.6f, w * 0.8f, 3.0f, 1.5f);
            g.addTransform (AffineTransform::rotation (-0.3f, cx, cy + w * 0.4f));
            break;
        }
        case 8:   // EVOLVE: a seed with six children around it
        {
            const float rr = w * 0.42f;
            g.fillEllipse (cx - 3.5f, cy - 3.5f, 7, 7);
            for (int i = 0; i < 6; ++i)
            {
                const float a = MathConstants<float>::twoPi * (float) i / 6.0f;
                const Point<float> pt (cx + std::cos (a) * rr, cy + std::sin (a) * rr);
                g.drawLine (cx, cy, pt.x, pt.y, 1.0f);
                g.fillEllipse (pt.x - 2.2f, pt.y - 2.2f, 4.4f, 4.4f);
            }
            break;
        }
        default:  // FX RACK: a knob with its scale
        {
            const float rr = w * 0.36f;
            g.drawEllipse (cx - rr, cy - rr + 1, rr * 2, rr * 2, 1.8f);
            const float ang = -0.8f;
            g.drawLine (cx, cy + 1, cx + std::sin (ang) * rr * 0.8f, cy + 1 - std::cos (ang) * rr * 0.8f, 2.0f);
            for (int i = 0; i < 7; ++i)
            {
                const float a = -2.4f + 0.8f * (float) i, r1 = rr + 3.0f, r2 = rr + 5.0f;
                g.setColour (ink.withAlpha (i <= 2 ? 1.0f : 0.45f));
                g.drawLine (cx + std::sin (a) * r1, cy + 1 - std::cos (a) * r1, cx + std::sin (a) * r2, cy + 1 - std::cos (a) * r2, 1.4f);
            }
            break;
        }
    }
}

class LabSwitch : public Component, public SettableTooltipClient, public FileDragAndDropTarget, private Timer
{
public:
    // drag a sound (from the SAMPLER, FL's browser, a folder) onto the tiles: it lands in PAIR YOUR OWN - from any page
    std::function<int (const File&)> onDropToPair;   // returns the slot (1..4) or 0
    bool isInterestedInFileDrag (const StringArray& f) override { for (auto& x : f) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true; return false; }
    void fileDragEnter (const StringArray&, int, int) override { dropHover = true; repaint(); }
    void fileDragExit (const StringArray&) override { dropHover = false; repaint(); }
    void filesDropped (const StringArray& files, int, int) override
    {
        dropHover = false;
        int n = 0, last = 0;
        for (auto& x : files) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg") && onDropToPair) if (const int sl = onDropToPair (File (x)); sl > 0) { ++n; last = sl; }
        flashText = n >= 1 ? "PARENT " + String (last == 1 ? "A" : "B") : "COULD NOT\nREAD IT";
        flash = 1.0f; startTimerHz (30); repaint();
    }
    explicit LabSwitch (KKLookAndFeel& l) : lnf (l)
    {
        setTooltip ("BREED LAB: 2 parents -> 6 sounds.  FAMILY TREE: sounds and melody loops.  SAMPLER: chop your samples.  DRUM KIT: your drums and rolls.  FX RACK: effects on every melody.  TIP: drag any sound onto these tiles = it goes into PAIR YOUR OWN.");
    }
    std::function<void (int)> onSwitch;
    std::function<bool (int)> isOn;   // VOODOO / EFFECTOR: lit when switched on
    int sel = 0;
    static constexpr int numTiles = 12, studioAt = 9;
    // v0.44: EVOLVE / FEED / ALCHEMY / LIFE / MELODY / SOUND WORLD / BREED LAB / FAMILY TREE / MY SOUNDS, STUDIO: SAMPLER / FX / MIX LAB
    static int tileId (int v) { static const int ids[numTiles] { 8, 14, 12, 13, 9, 11, 0, 1, 4, 5, 7, 10 }; return ids[jlimit (0, numTiles - 1, v)]; }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        static const char* names[] { "BREED\nLAB", "FAMILY\nTREE", "PAIR\nYOUR OWN", "PAIR\nFROM VST", "MY\nSOUNDS", "SAMPLER", "DRUM\nKIT", "FX", "EVOLVE", "MELODY", "MIX\nLAB", "SOUND\nWORLD", "ALCHEMY", "LIFE", "FEED" };
        // each extra wears its plugin's colours
        static const Colour face[] { Colour (0), Colour (0) };
        static const Colour ink[] { Colour (0), Colour (0) };
        for (int v = 0; v < numTiles; ++v)
        {
            const int k = tileId (v);
            auto r = part (v);
            const bool on = k == sel || ((dropHover || flash > 0) && k == 0);
            if (k == 0 && (dropHover || flash > 0))   // the BREED LAB tile becomes the drop target
            {
                g.setColour (s.accent.withAlpha (0.35f + 0.3f * flash)); g.fillRoundedRectangle (r.expanded (2), 7);
                drawGlowFrame (g, r, s.accent, 6);
                g.setColour (TC (0xffffffff)); g.setFont (serif (14.0f, true, 0.2f));
                g.drawFittedText (dropHover ? "DROP\n= PARENT" : flashText, r.toNearestInt(), Justification::centred, 2);
                continue;
            }
            if (v == studioAt)
            {
                const auto lr = r.withY (r.getY() - 15).withHeight (13);
                g.setColour (TC (0xffc8c4e8)); g.setFont (serif (11.5f, true, 0.3f));
                g.drawText ("STUDIO", lr.toNearestInt(), Justification::centred);
                g.setGradientFill (ColourGradient (TC (0x00ff3fd2), lr.getX(), 0, TC (0xccff3fd2), lr.getCentreX() - 30, 0, false));
                g.fillRect (lr.getX() + 4, lr.getCentreY(), lr.getWidth() * 0.5f - 34, 1.2f);
                g.setGradientFill (ColourGradient (TC (0xcc9b4dff), lr.getCentreX() + 30, 0, TC (0x009b4dff), lr.getRight(), 0, false));
                g.fillRect (lr.getCentreX() + 30, lr.getCentreY(), lr.getWidth() * 0.5f - 34, 1.2f);
            }
            juce::ignoreUnused (face);
            if (on)
            {
                const bool drop = k == 0 && (dropHover || flash > 0);
                const auto c1 = drop ? s.accent : tileCol (k, 0), c2 = drop ? s.accent : tileCol (k, 1);
                g.setGradientFill (ColourGradient (c1.withAlpha (0.16f), r.getX(), r.getY(), c2.withAlpha (0.05f), r.getRight(), r.getBottom(), false));
                g.fillRoundedRectangle (r, 8);
                g.setColour (c1); g.drawRoundedRectangle (r.reduced (0.5f), 8, 1.4f);
            }
            else
            {
                g.setGradientFill (ColourGradient (TC (0xff1e1b40), 0, r.getY(), TC (0xff131230), 0, r.getBottom(), false)); g.fillRoundedRectangle (r, 8);
                g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (r, 8, 1.2f);
            }
            juce::ignoreUnused (ink);
            // icon + name (left aligned) - each tile its own neon colour
            const float bs = jmin (30.0f, r.getHeight() - 10.0f);
            const auto ic = Rectangle<float> (r.getX() + 7, r.getCentreY() - bs * 0.5f, bs, bs);
            drawTileIcon (g, k, ic, on);
            g.setColour (on ? TC (0xffffffff) : TC (0xffe6e3ff));
            g.setFont (serif (r.getHeight() < 40.0f ? 14.0f : 16.0f, true, 0.1f));   // v0.37: readable at FL's usual size
            g.drawFittedText (String (names[k]), r.withTrimmedLeft (bs + 12).toNearestInt(), Justification::centredLeft, 2, 0.8f);
            if (k == 7 && isOn && isOn (k))   // FX RACK: lit when an effect is on
            {
                g.setColour (TC (0xff36ff6a)); g.fillEllipse (r.getRight() - 14, r.getY() + 6, 8, 8);
            }
        }
    }
    void mouseUp (const MouseEvent& e) override
    {
        for (int v = 0; v < numTiles; ++v)
            if (part (v).contains (e.position) && onSwitch) { onSwitch (tileId (v)); return; }
    }
private:
    Rectangle<float> part (int k) const
    {
        const float gap = 5.0f, extra = 18.0f;
        const float h = ((float) getHeight() - gap * (numTiles - 1) - extra) / (float) numTiles;
        return { 3.0f, (float) k * (h + gap) + (k >= studioAt ? extra : 0.0f), (float) getWidth() - 6.0f, h };
    }
    void timerCallback() override { flash -= 0.012f; if (flash <= 0) { flash = 0; stopTimer(); } repaint (part (0).expanded (4).toNearestInt()); }
    KKLookAndFeel& lnf;
    bool dropHover = false;
    float flash = 0;
    String flashText;
};

//==============================================================================
// v0.36 EVOLVE: the whole plugin on one screen. A seed in the middle, its six children around it as glass bubbles.
// Hover = hear it (the keys play it).  Click = it becomes the middle and the next generation grows.
// Drag from the middle towards a child = blend the two live.  SAFE <-> WILD = how far the children may wander.
// Every bubble: SAVE, drag WAV, drag MELODY.  The path you took stays on top - click any step to go back.
//==============================================================================
// v0.39 MAP: the whole family of ideas you grew - every generation a column, your way lit up.
// Click a dot = back to that idea.  Wheel = zoom, drag = move.
class EvoMapView : public Component
{
public:
    explicit EvoMapView (KeysKillaProcessor& p) : proc (p) {}
    std::function<void()> onClose;
    void build()
    {
        const int n = (int) proc.evo.size();
        pos.assign ((size_t) n, { 0.0f, 0.0f });
        // depth-first leaf order: each node sits in the middle of its children
        std::vector<std::vector<int>> kids ((size_t) n);
        std::vector<int> roots;
        for (int i = 0; i < n; ++i)
        {
            const int pa = proc.evo[(size_t) i].parent;
            if (isPositiveAndBelow (pa, n) && pa != i) kids[(size_t) pa].push_back (i); else roots.push_back (i);
        }
        float leaf = 0;
        std::function<float (int, int)> place = [&] (int i, int depth) -> float
        {
            float y;
            if (kids[(size_t) i].empty() || depth > 200) y = leaf++;
            else { float sum = 0; for (int k : kids[(size_t) i]) sum += place (k, depth + 1); y = sum / (float) kids[(size_t) i].size(); }
            pos[(size_t) i] = { (float) proc.evo[(size_t) i].gen, y };
            return y;
        };
        for (int r : roots) place (r, 0);
        maxGen = 1; for (auto& pp : pos) maxGen = std::max (maxGen, (int) pp.x);
        leaves = std::max (1.0f, leaf);
        zoom = 1.0f; pan = {};
        repaint();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        g.setColour (t.night ? Colour (0xff0d0f12) : Colour (0xffe9ecef)); g.fillRoundedRectangle (getLocalBounds().toFloat(), 18);
        g.setColour (t.accent.withAlpha (0.5f)); g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1), 18, 1.2f);
        g.setColour (t.text); g.setFont (kk::modern::font (13.0f, true, 0.2f));
        g.drawText ("CLOSE  X", closeRect(), Justification::centred);
        g.setColour (t.text); g.setFont (kk::modern::font (22.0f, true, 0.2f));
        g.drawText ("MAP", 28, 14, 200, 32, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.06f));
        g.drawText (String ((int) proc.evo.size()) + " ideas, " + String (maxGen) + " generations   -   click a dot = go back there   -   wheel = zoom, drag = move",
                    110, 18, getWidth() - 260, 24, Justification::centredLeft);
        const int n = (int) proc.evo.size();
        if (n == 0) return;
        std::vector<bool> onPath ((size_t) n, false);
        for (int i : proc.evoPath()) if (isPositiveAndBelow (i, n)) onPath[(size_t) i] = true;
        for (int i = 0; i < n; ++i)
        {
            const int pa = proc.evo[(size_t) i].parent;
            if (! isPositiveAndBelow (pa, n)) continue;
            g.setColour (onPath[(size_t) i] ? t.accent : t.text.withAlpha (proc.evo[(size_t) i].picked ? 0.45f : 0.15f));
            g.drawLine (Line<float> (screen (pa), screen (i)), onPath[(size_t) i] ? 2.4f : 1.0f);
        }
        for (int i = 0; i < n; ++i)
        {
            const auto c = screen (i);
            if (! getLocalBounds().toFloat().expanded (20).contains (c)) continue;
            const auto& e = proc.evo[(size_t) i];
            const bool centre = i == proc.evoCenter, hot = i == hovered;
            const float r = (centre ? 9.0f : e.picked ? 6.5f : 4.0f) * std::sqrt (zoom);
            g.setColour (centre ? t.accent : e.picked ? t.accent.withAlpha (0.75f) : t.text.withAlpha (0.4f));
            g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c));
            if (hot || centre || (e.picked && zoom > 1.6f))
            {
                g.setColour (t.text); g.setFont (kk::modern::font (11.5f, true, 0.04f));
                g.drawText (e.name, Rectangle<float> (c.x + r + 4, c.y - 8, 200, 16), Justification::centredLeft);
            }
        }
    }
    void mouseMove (const MouseEvent& e) override { const int h = nodeAt (e.position); if (h != hovered) { hovered = h; repaint(); } }
    void mouseDown (const MouseEvent&) override { panStart = pan; }
    void mouseDrag (const MouseEvent& e) override { pan = panStart + e.getOffsetFromDragStart().toFloat(); repaint(); }
    void mouseUp (const MouseEvent& e) override
    {
        if (e.mouseWasDraggedSinceMouseDown()) return;
        if (closeRect().contains (e.position)) { setVisible (false); if (onClose) onClose(); return; }
        const int h = nodeAt (e.position);
        if (h >= 0) { proc.evoFocus (h); if (onClose) onClose(); }
    }
    Rectangle<float> closeRect() const { return { (float) getWidth() - 130.0f, 14.0f, 110.0f, 30.0f }; }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        const float old = zoom;
        zoom = jlimit (0.3f, 6.0f, zoom * (w.deltaY > 0 ? 1.15f : 1.0f / 1.15f));
        const auto m = e.position - Point<float> (60.0f, 70.0f);
        pan = m - (m - pan) * (zoom / old);
        repaint();
    }
private:
    Point<float> screen (int i) const
    {
        const float w = (float) getWidth() - 140.0f, h = (float) getHeight() - 110.0f;
        const auto& p = pos[(size_t) i];
        const float x = 60.0f + w * p.x / (float) std::max (1, maxGen), y = 70.0f + h * (p.y + 0.5f) / leaves;
        return { 60.0f + (x - 60.0f) * zoom + pan.x, 70.0f + (y - 70.0f) * zoom + pan.y };
    }
    int nodeAt (Point<float> p) const
    {
        int best = -1; float bd = 12.0f;
        for (int i = 0; i < (int) pos.size() && i < (int) proc.evo.size(); ++i) { const float d = p.getDistanceFrom (screen (i)); if (d < bd) { bd = d; best = i; } }
        return best;
    }
    KeysKillaProcessor& proc;
    std::vector<Point<float>> pos;
    int maxGen = 1, hovered = -1;
    float leaves = 1, zoom = 1;
    Point<float> pan, panStart;
};

// v0.41 MATCH: drop a sound, four strands of synth sounds grow toward it - hear any of them, plant one as the seed
class MatchView : public Component, public FileDragAndDropTarget, private Timer
{
public:
    MatchView (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (fileBtn, "CHOOSE A SOUND", "A WAV / MP3 / AIFF the synth should grow toward", [this]
        {
            chooser = std::make_unique<FileChooser> ("The sound to match", File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc) { if (fc.getResult().existsAsFile()) start (fc.getResult()); });
        });
        btn (pluginBtn, "FROM THE PLUGIN", "A sound from EVOLVE itself: the sound on the keys right now, or one from MY SOUNDS / the BANK / SOUND KITS", [this] { pluginMenu(); });
        btn (stopBtn, "STOP", "Stop growing (the strands stay)", [this] { proc.evoMatchStop(); repaint(); });
        btn (closeBtn, "CLOSE", "Back to EVOLVE", [this] { if (onClose) onClose(); });
        for (int i = 0; i < 4; ++i)
        {
            auto h = std::make_unique<HotButton> (lnf, "HEAR"); h->framed = true; h->setTooltip ("Hear this strand on the keys");
            h->onClick = [this, i] { auto st = proc.matchStrands(); if (i < (int) st.size()) { KeysKillaProcessor::EvoNode n; n.g = st[(size_t) i].g; n.name = st[(size_t) i].g.name; proc.evoAuditionNode (n, false); heard = i; repaint(); } };
            addChildComponent (*h); hearBtns.push_back (std::move (h));
            auto pl = std::make_unique<HotButton> (lnf, "PLANT"); pl->framed = true; pl->hero = true; pl->setTooltip ("Plant it: this sound becomes the seed of a new EVOLVE tree");
            pl->onClick = [this, i] { auto st = proc.matchStrands(); if (i < (int) st.size()) { proc.evoSeedGenome (st[(size_t) i].g); if (onPlanted) onPlanted(); } };
            addChildComponent (*pl); plantBtns.push_back (std::move (pl));
        }
        startTimerHz (20);
    }
    std::function<void()> onClose, onPlanted;
    bool isInterestedInFileDrag (const StringArray& f) override { for (auto& x : f) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true; return false; }
    void fileDragEnter (const StringArray&, int, int) override { dropHot = true; repaint(); }
    void fileDragExit (const StringArray&) override { dropHot = false; repaint(); }
    void filesDropped (const StringArray& f, int, int) override { dropHot = false; for (auto& x : f) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) { start (File (x)); break; } }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        auto r = getLocalBounds().toFloat();
        g.setColour (t.night ? Colour (0xff0d1014) : Colour (0xfff3f5f7)); g.fillRoundedRectangle (r, 16);   // solid: nothing of EVOLVE shows through
        g.setColour (Colour (0xff36ff6a).withAlpha (0.6f)); g.drawRoundedRectangle (r.reduced (1), 16, 1.6f);
        g.setColour (t.text); g.setFont (kk::modern::font (26.0f, true, 0.12f));
        g.drawText ("MATCH", 28, 18, 200, 34, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.0f, true, 0.03f));
        g.drawText ("drop any sound - four strands of synth sounds grow toward it.  you do not need to wait: hear any strand, plant the one you like (60% often sounds better than a copy)",
                    150, 22, getWidth() - 470, 28, Justification::centredLeft);
        // the target
        const auto tg = Rectangle<float> (28, 70, 300, (float) getHeight() - 100);
        g.setColour (Colours::black.withAlpha (0.25f)); g.fillRoundedRectangle (tg, 12);
        g.setColour (t.text); g.setFont (kk::modern::font (12.0f, true, 0.2f));
        g.drawText ("THE SOUND", tg.reduced (14, 10).removeFromTop (16), Justification::centredLeft);
        if (proc.matchTargetName.isNotEmpty())
        {
            g.setColour (kk::accentText()); g.setFont (kk::modern::font (15.0f, true, 0.03f));
            g.drawFittedText (proc.matchTargetName, tg.reduced (14, 30).removeFromTop (22).toNearestInt(), Justification::centredLeft, 1, 0.7f);
            drawWave (g, tg.reduced (14, 64).withHeight (110), proc.matchTargetWave, Colour (0xffffd23f));
            const float pr = proc.matchProgress.load();
            const auto bar = Rectangle<float> (tg.getX() + 14, tg.getY() + 200, tg.getWidth() - 28, 10);
            g.setColour (t.text.withAlpha (0.12f)); g.fillRoundedRectangle (bar, 5);
            g.setColour (Colour (0xff36ff6a)); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * pr), 5);
            g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.05f));
            g.drawText (proc.matchRunning.load() ? (pr < 0.4f ? "listening to the whole bank ... " : "growing ... ") + String (roundToInt (pr * 100)) + "%" : String ("done - plant one, or drop another sound"),
                        bar.translated (0, 14).withHeight (16).toNearestInt(), Justification::centredLeft);
        }
        else
        {
            g.setColour (t.dim); g.setFont (kk::modern::font (15.0f, true, 0.05f));
            g.drawFittedText ("DROP A SOUND HERE\n(a sample, a vocal chop,\na sound from another plugin)", tg.reduced (14, 40).toNearestInt(), Justification::centred, 4);
        }
        // the strands: green vines that grow with the match
        const auto st = proc.matchStrands();
        for (int i = 0; i < 4; ++i)
        {
            const auto lane = strandRect (i);
            g.setColour (Colours::black.withAlpha (0.18f)); g.fillRoundedRectangle (lane, 12);
            if (i >= (int) st.size()) { g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.1f)); g.drawText ("STRAND " + String (i + 1), lane.reduced (10).removeFromTop (16), Justification::centred); continue; }
            const auto& ms = st[(size_t) i];
            const float m = jlimit (0.0f, 100.0f, ms.match) / 100.0f;
            const float baseY = lane.getBottom() - 70, topY = lane.getY() + 60, h = (baseY - topY) * m;
            const float cx = lane.getCentreX();
            Path vine; vine.startNewSubPath (cx, baseY);
            for (int k = 1; k <= 24; ++k) { const float u = (float) k / 24.0f; vine.lineTo (cx + std::sin (u * 7.0f + (float) i + phase * 0.6f) * 9.0f * u, baseY - h * u); }
            g.setColour (Colour (0xff36ff6a).withAlpha (0.25f)); g.strokePath (vine, PathStrokeType (7.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colour (0xff36ff6a)); g.strokePath (vine, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));
            for (int k = 1; k <= jmin (6, 1 + ms.gen / 4); ++k)
            {
                const float u = (float) k / 7.0f; const auto pt = Point<float> (cx + std::sin (u * 7.0f + (float) i + phase * 0.6f) * 9.0f * u, baseY - h * u);
                g.setColour (Colour (0xff7cf56a)); g.fillEllipse (pt.x + (k % 2 ? 4.0f : -14.0f), pt.y - 4, 10, 8);
            }
            const auto bud = Point<float> (cx + std::sin (7.0f + (float) i + phase * 0.6f) * 9.0f, baseY - h);
            g.setColour (heard == i ? Colour (0xffffd23f) : Colour (0xff36ff6a)); g.fillEllipse (bud.x - 18, bud.y - 18, 36, 36);
            g.setColour (Colours::black); g.setFont (kk::modern::font (12.5f, true, 0.0f));
            g.drawText (String (roundToInt (ms.match)) + "%", Rectangle<float> (bud.x - 18, bud.y - 18, 36, 36), Justification::centred);
            drawWave (g, Rectangle<float> (lane.getX() + 12, lane.getY() + 12, lane.getWidth() - 24, 40), ms.wave, Colour (0xff36ff6a));
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.1f));
            g.drawText ("GEN " + String (ms.gen), lane.toNearestInt().withTrimmedBottom (54).removeFromBottom (16), Justification::centred);
        }
        if (dropHot) { g.setColour (Colour (0xff36ff6a).withAlpha (0.12f)); g.fillRoundedRectangle (r, 16); }
    }
    void resized() override
    {
        closeBtn.setBounds (getWidth() - 120, 18, 96, 36); stopBtn.setBounds (getWidth() - 210, 18, 84, 36);
        fileBtn.setBounds (42, getHeight() - 84, 270, 40);
        pluginBtn.setBounds (42, getHeight() - 132, 270, 40);
        for (int i = 0; i < 4; ++i)
        {
            const auto lane = strandRect (i).toNearestInt();
            hearBtns[(size_t) i]->setBounds (lane.getX() + 10, lane.getBottom() - 50, lane.getWidth() / 2 - 14, 38);
            plantBtns[(size_t) i]->setBounds (lane.getCentreX() + 4, lane.getBottom() - 50, lane.getWidth() / 2 - 14, 38);
        }
    }
private:
    Rectangle<float> strandRect (int i) const { const float x0 = 350, w = ((float) getWidth() - x0 - 28 - 3 * 14) / 4.0f; return { x0 + (float) i * (w + 14), 70, w, (float) getHeight() - 100 }; }
    static void drawWave (Graphics& g, Rectangle<float> r, const std::array<float, 64>& w, Colour c)
    {
        g.setColour (c.withAlpha (0.85f));
        const float bw = r.getWidth() / 64.0f;
        for (int k = 0; k < 64; ++k) { const float h = jmax (1.0f, r.getHeight() * w[(size_t) k]); g.fillRect (r.getX() + bw * (float) k, r.getCentreY() - h * 0.5f, jmax (1.0f, bw - 1.0f), h); }
    }
    void pluginMenu()
    {
        PopupMenu m;
        m.addItem (1, "THE SOUND ON THE KEYS  (" + (proc.sampleActive() && proc.activeSample() != nullptr ? proc.activeSample()->name : proc.currentName()) + ")");
        m.addItem (2, "FROM MY SOUNDS ...");
        m.addItem (3, "FROM THE BANK (harvested / VST sounds) ...");
        m.addItem (4, "FROM SOUND KITS ...");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&pluginBtn), [this, safe = SafePointer<MatchView> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1) { heard = -1; if (! proc.evoMatchFromKeys()) proc.matchTargetName = "nothing on the keys"; repaint(); return; }
            const File dir = r == 2 ? kk::Library::root() : r == 3 ? KeysKillaProcessor::bankFolder() : kk::SoundKits::root();
            chooser = std::make_unique<FileChooser> ("The sound to match", dir, "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc) { if (fc.getResult().existsAsFile()) start (fc.getResult()); });
        });
    }
    void start (const File& f)
    {
        heard = -1;
        if (! proc.evoMatchStart (f)) { proc.matchTargetName = "could not read " + f.getFileName(); }
        repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f;
        const int v = proc.matchVer.load();
        const auto st = proc.matchStrands();
        for (int i = 0; i < 4; ++i) { hearBtns[(size_t) i]->setVisible (i < (int) st.size()); plantBtns[(size_t) i]->setVisible (i < (int) st.size()); }
        stopBtn.setEnabled (proc.matchRunning.load());
        if (v != lastVer || proc.matchRunning.load()) { lastVer = v; repaint(); }
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton fileBtn { lnf }, stopBtn { lnf }, closeBtn { lnf }, pluginBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> hearBtns, plantBtns;
    std::unique_ptr<FileChooser> chooser;
    bool dropHot = false; int heard = -1, lastVer = -1; float phase = 0;
};

class EvolvePage : public Component, public FileDragAndDropTarget, private Timer
{
public:
    EvolvePage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn)
        { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (studioBtn, "STUDIO", "The full studio: BREED LAB with two parents, FAMILY TREE, SAMPLER, MY SOUNDS, FX RACK", [this] { if (onStudio) onStudio(); });
        btn (themeBtn, "", "Day / night", [this] { if (onTheme) onTheme(); });
        themeBtn.glyph = [] (Graphics& g, Rectangle<float> r, const Skin&)
        {
            const auto& t = kk::theme(); const auto c = r.getCentre();
            if (t.night) kk::modern::sun (g, c, t.text.withAlpha (0.9f)); else kk::modern::moon (g, c, t.text.withAlpha (0.85f), t.night ? Colour (0xff1d2026) : Colour (0xffeef0f2));
        };
        btn (ideaBtn, "SOUND + FX", "SOUND + FX: every bubble is a sound with its own effects - both grow together.  Click to switch to SOUND ONLY (the effects stay as they are)", [this]
        { proc.evoIdea = ! proc.evoIdea; refreshIdeaBtn(); if (proc.evoIdea && proc.evoCenter >= 0) proc.evoApplyIdea (proc.evoCenter); repaint(); });
        btn (seedBtn, "NEW SEED", "Start a new tree: a sound from the bank, a surprise, the current sound - or drop any WAV onto the page", [this] { seedMenu (&seedBtn); });
        btn (againBtn, "EVOLVE AGAIN", "Six new children of the middle sound", [this] { if (proc.evoCenter >= 0) { proc.evoGrow (proc.evoCenter, true); startAnim(); } });
        btn (saveBtn, "SAVE", "Keep the middle sound: as a preset, or as a sound in your folders / kits", [this] { saveMenu (proc.evoCenter, &saveBtn); });
        btn (bankBtn, "PICK FROM THE BANK", "Choose the seed from the sound library", [this] { if (onPickSeed) onPickSeed(); });
        btn (diceBtn, "SURPRISE ME", "A random seed from the bank", [this] { proc.evoSeedRandom(); startAnim(); });
        btn (currentBtn, "USE THE CURRENT SOUND", "The sound on the keys right now becomes the seed", [this] { proc.evoSeedCurrent(); startAnim(); });
        dragWav.makeFile = [this] { return proc.evoIdea ? proc.evoExportIdea (proc.evoCenter) : proc.evoExportWav (proc.evoCenter); };
        dragWav.setTooltip ("Drag the middle sound into FL Studio as a WAV (tuned to C: FL's sampler plays it at C5) - with its effects in SOUND + FX");
        addAndMakeVisible (dragWav);
        btn (melodyBtn, "PLAY", "Hear the middle sound", [this] { if (proc.evoCenter >= 0) { proc.evoAudition (proc.evoCenter); if (proc.evoIdea) proc.evoApplyIdea (proc.evoCenter); } });
        // v0.39 LAYERS: what the next children change
        static const char* layerNames[] { "ALL", "SOUND", "FX" };
        static const int layerIds[] { KeysKillaProcessor::layerAll, KeysKillaProcessor::layerSound, KeysKillaProcessor::layerFx };
        static const char* layerTips[] { "The children change the sound and the effects a little",
                                         "Only the SOUND changes - the effects stay",
                                         "Only the EFFECTS change - the same sound in new colours" };
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, layerNames[i]); b->framed = true; b->setTooltip (String (layerTips[i]) + "  (then EVOLVE AGAIN or click a bubble)");
            const int id = layerIds[i];
            b->onClick = [this, id] { proc.evoLayer = id; refreshLayers(); if (proc.evoCenter >= 0) { proc.evoGrow (proc.evoCenter, true); startAnim(); } };
            addAndMakeVisible (*b); layerBtns.push_back (std::move (b));
        }
        // v0.39 ALIVE / CATCH / MAP / WORLD
        btn (aliveBtn, "ALIVE", "ALIVE: the middle sound slowly changes by itself while you play the keys (WILD = how fast). When you like the moment: CATCH", [this]
        { alive = ! alive; aliveBtn.selected = alive; aliveBtn.repaint(); });
        btn (catchBtn, "CATCH", "Keep this moment: what you hear right now becomes the middle idea (and grows)", [this] { proc.evoCatch(); alive = false; aliveBtn.selected = false; aliveBtn.repaint(); startAnim(); });
        btn (mapBtn, "MAP", "The whole family you grew - click any idea to go back to it", [this] { showMap (! map.isVisible()); });
        btn (worldBtn, "WORLD", "Save this whole world of ideas to a file - or open one", [this] { worldMenu(); });
        btn (matchBtn, "MATCH", "MATCH: drop any sound - the synth grows toward it (four strands), plant the one you like as the seed", [this] { showMatch (! match.isVisible()); });
        addChildComponent (match);
        match.onClose = [this] { showMatch (false); };
        match.onPlanted = [this] { showMatch (false); layoutButtons(); startAnim(); };
        addChildComponent (map);
        map.onClose = [this] { showMap (false); startAnim(); };
        refreshLayers();
        setWantsKeyboardFocus (false);
        startTimerHz (30);
    }
    void refreshLayers()
    {
        static const int ids[] { KeysKillaProcessor::layerAll, KeysKillaProcessor::layerSound, KeysKillaProcessor::layerFx };
        for (int i = 0; i < (int) layerBtns.size(); ++i) { layerBtns[(size_t) i]->selected = proc.evoLayer == ids[i]; layerBtns[(size_t) i]->repaint(); }
    }
    void showMatch (bool on)
    {
        if (on) { showMap (false); match.setBounds (getLocalBounds().reduced (16).withTrimmedTop (80).withTrimmedBottom (84)); match.toFront (false); }
        match.setVisible (on);
        matchBtn.selected = on; matchBtn.repaint();
    }
    void showMap (bool on)
    {
        if (on) { map.setBounds (getLocalBounds().reduced (16).withTrimmedTop (80).withTrimmedBottom (84)); map.build(); map.toFront (false); }
        map.setVisible (on);
        mapBtn.selected = on; mapBtn.repaint();
    }
    void worldMenu()
    {
        PopupMenu m;
        m.addItem (1, "Save this world (all ideas, your path, the POCKET)...", ! proc.evo.empty());
        m.addItem (2, "Open a world...");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (&worldBtn), [this, safe = SafePointer<EvolvePage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            auto dir = File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("Worlds");
            dir.createDirectory();
            chooser = std::make_unique<FileChooser> (r == 1 ? "Save the world" : "Open a world", dir, "*.evolve");
            if (r == 1)
                chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting, [this, safe] (const FileChooser& fc)
                { if (safe != nullptr && fc.getResult() != File()) proc.evoSaveWorld (fc.getResult().withFileExtension ("evolve")); });
            else
                chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, safe] (const FileChooser& fc)
                { if (safe != nullptr && fc.getResult().existsAsFile() && proc.evoLoadWorld (fc.getResult())) startAnim(); });
        });
    }
    std::function<void()> onStudio, onTheme, onPickSeed, onChanged;
    std::function<void (int)> onSavePreset;   // node -> save as preset (the page asks for a name)

    // ---- FileDragAndDropTarget: any sound becomes the seed
    bool isInterestedInFileDrag (const StringArray& f) override { for (auto& x : f) if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg")) return true; return false; }
    void fileDragEnter (const StringArray&, int, int) override { dropHot = true; repaint(); }
    void fileDragExit (const StringArray&) override { dropHot = false; repaint(); }
    void filesDropped (const StringArray& files, int, int) override
    {
        dropHot = false;
        for (auto& x : files) if (proc.evoSeedFromFile (File (x))) { startAnim(); break; }
        repaint();
    }

    void visibilityChanged() override { proc.evoActive = isVisible(); if (isVisible()) { layoutButtons(); repaint(); } }

    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this, 0.7f);
        // header
        kk::modern::plate (g, { 12, 12, (float) getWidth() - 24, 70 }, 18.0f);
        kk::modern::wordmark (g, { 40, 22, 420, 44 }, 34.0f);
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (16.5f, true, 0.06f));
        g.drawText (proc.evo.empty() ? "drop any sound  -  or pick a seed from the bank" : "hover = hear   click = grow   drag onto a bubble = cross",
                    Rectangle<float> (420, 26, (float) getWidth() - 420 - 500, 36), Justification::centredLeft);
        // bottom bar
        kk::modern::plate (g, bottomBar().toFloat(), 18.0f);
        drawWild (g);
        if (proc.evo.empty()) { paintEmpty (g); paintPocket (g); return; }

        const auto path = proc.evoPath();
        paintPath (g, path);
        const int c = proc.evoCenter;
        if (! isPositiveAndBelow (c, (int) proc.evo.size())) return;
        const auto& centre = proc.evo[(size_t) c];
        const float a = anim();
        // links
        if (centre.parent >= 0)
        {
            g.setColour (t.accent.withAlpha (0.35f)); g.drawLine (Line<float> (parentPos(), centrePos()), 1.4f);
        }
        for (int k = 0; k < (int) centre.kids.size(); ++k)
        {
            const auto kp = kidPos (k, a);
            ColourGradient lg (t.accent.withAlpha (0.55f), centrePos().x, centrePos().y, t.accent.withAlpha (0.12f), kp.x, kp.y, false);
            g.setGradientFill (lg); g.drawLine (Line<float> (centrePos(), kp), 1.6f);
        }
        // morph line
        if (morphKid >= 0 && isPositiveAndBelow (morphKid, (int) centre.kids.size()))
        {
            const auto kp = kidPos (morphKid, 1.0f);
            const auto m = centrePos() + (kp - centrePos()) * morphT;
            g.setColour (t.accent); g.drawLine (Line<float> (centrePos(), m), 4.0f);
            g.setColour (t.accent.withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (40, 40).withCentre (m));
            g.setColour (Colours::white); g.fillEllipse (Rectangle<float> (12, 12).withCentre (m));
            g.setColour (t.text); g.setFont (kk::modern::font (12.0f, true, 0.1f));
            g.drawText ("BLEND " + String (roundToInt (morphT * 100)) + "%", Rectangle<float> (120, 20).withCentre (m.translated (0, -30)), Justification::centred);
        }
        if (centre.parent >= 0) drawBubble (g, centre.parent, parentPos(), 50.0f, false, "BACK");
        for (int k = 0; k < (int) centre.kids.size(); ++k)
        {
            const float s = 0.35f + 0.65f * a;
            drawBubble (g, centre.kids[(size_t) k], kidPos (k, a), kidR * s, false, {});
        }
        drawBubble (g, c, centrePos(), centreR, true, {});
        // actions under the hovered bubble
        if (isPositiveAndBelow (hovered, (int) proc.evo.size()) && hovered != c && carry < 0 && carryPocket < 0) drawActions (g, hovered);
        if (alive)
        {
            const float pulse = 0.5f + 0.5f * std::sin ((float) Time::getMillisecondCounter() * 0.004f);
            g.setColour (t.accent.withAlpha (0.25f + 0.35f * pulse)); g.drawEllipse (Rectangle<float> (centreR * 2.0f + 18, centreR * 2.0f + 18).withCentre (centrePos()), 2.5f);
            g.setColour (kk::accentText()); g.setFont (kk::modern::font (11.0f, true, 0.3f));
            g.drawText ("ALIVE", Rectangle<float> (100, 16).withCentre (centrePos().translated (0, -centreR - 22)), Justification::centred);
        }
        paintPocket (g);
        if ((carry >= 0 || carryPocket >= 0) && carryAt.x > 0)   // carrying a bubble
        {
            g.setColour (t.accent.withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (60, 60).withCentre (carryAt));
            g.setColour (t.accent); g.drawEllipse (Rectangle<float> (60, 60).withCentre (carryAt), 2.0f);
            const int tgt = nodeAt (carryAt);
            g.setColour (t.text); g.setFont (kk::modern::font (12.0f, true, 0.1f));
            g.drawText (pocketArea().contains (carryAt) ? "KEEP IN POCKET" : tgt >= 0 && tgt != carry ? "DROP = CROSS THEM" : "drop onto a bubble",
                        Rectangle<float> (220, 18).withCentre (carryAt.translated (0, -44)), Justification::centred);
        }
        if (dropHot)
        {
            g.setColour (t.accent.withAlpha (0.12f)); g.fillRect (getLocalBounds());
            g.setColour (t.accent); g.setFont (kk::modern::font (28.0f, true, 0.2f));
            g.drawText ("DROP = A NEW SEED", getLocalBounds(), Justification::centred);
        }
    }
    void resized() override { layoutButtons(); }

    // ---- mouse
    void mouseMove (const MouseEvent& e) override
    {
        const int h = nodeAt (e.position);
        const int pa = actionAt (e.position);
        if (h != hovered && pa < 0) { hovered = h; hoverSince = Time::getMillisecondCounter(); auditioned = false; repaint(); }
        const int ph = pocketAt (e.position) < (int) proc.pocket.size() ? pocketAt (e.position) : -1;
        if (ph != pocketHover) { pocketHover = ph; pocketSince = Time::getMillisecondCounter(); pocketHeard = false; repaint(); }
    }
    void mouseExit (const MouseEvent&) override { hovered = -1; pocketHover = -1; hoverSince = Time::getMillisecondCounter(); repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        carry = -1; carryPocket = -1; carryAt = {};
        const int pk = pocketAt (e.position);
        downPocket = pk >= 0 && pk < (int) proc.pocket.size() ? pk : -1;
        downNode = nodeAt (e.position); downAction = actionAt (e.position); dragFired = false;
        if (wildRect().expanded (6, 8).contains (e.position)) { draggingWild = true; setWild (e.position.x); }
        else if (tasteRect().expanded (6, 8).contains (e.position)) { draggingTaste = true; setTaste (e.position.x); }
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (draggingWild) { setWild (e.position.x); return; }
        if (draggingTaste) { setTaste (e.position.x); return; }
        // v0.39: carry a pocket idea or a bubble (not the middle) - onto another bubble = a cross, onto the POCKET = keep it
        if (downPocket >= 0 && e.getDistanceFromDragStart() > 8) { carryPocket = downPocket; carryAt = e.position; repaint(); return; }
        if (downAction < 0 && downNode >= 0 && downNode != proc.evoCenter && e.getDistanceFromDragStart() > 10) { carry = downNode; carryAt = e.position; repaint(); return; }
        if (downAction >= 0 && ! dragFired && e.getDistanceFromDragStart() > 6)   // drag WAV / MELODY out of a bubble
        {
            dragFired = true;
            const int node = actionNode, kind = downAction;
            if (kind == 1)
            {
                const auto f = proc.evoIdea ? proc.evoExportIdea (node) : proc.evoExportWav (node);
                if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
            }
            return;
        }
        // drag from the middle towards a child = blend (onto the POCKET = keep the middle)
        if (downNode == proc.evoCenter && downNode >= 0 && pocketArea().contains (e.position)) { carry = downNode; carryAt = e.position; morphKid = -1; repaint(); return; }
        if (downNode == proc.evoCenter && downNode >= 0 && e.getDistanceFromDragStart() > 12)
        {
            const auto& kids = proc.evo[(size_t) proc.evoCenter].kids;
            if (kids.empty() || proc.evo[(size_t) proc.evoCenter].isAudio()) return;
            const auto d = e.position - centrePos();
            int best = 0; float bestDot = -1e9f;
            for (int k = 0; k < (int) kids.size(); ++k)
            {
                const auto v = kidPos (k, 1.0f) - centrePos();
                const float dot = (v.x * d.x + v.y * d.y) / std::max (1.0f, v.getDistanceFromOrigin());
                if (dot > bestDot) { bestDot = dot; best = k; }
            }
            const auto v = kidPos (best, 1.0f) - centrePos();
            morphKid = best; morphT = jlimit (0.0f, 1.0f, bestDot / std::max (1.0f, v.getDistanceFromOrigin()));
            const auto now = Time::getMillisecondCounter();
            if (now - lastMorph > 60) { lastMorph = now; proc.evoMorph (proc.evoCenter, kids[(size_t) best], morphT); }
            repaint();
        }
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (draggingWild) { draggingWild = false; return; }
        if (draggingTaste) { draggingTaste = false; return; }
        if (carry >= 0 || carryPocket >= 0)
        {
            const int tgt = nodeAt (e.position);
            if (pocketArea().contains (e.position) && carry >= 0) proc.evoPocketAdd (carry);
            else if (tgt >= 0 && tgt != carry)
            {
                const auto src = carry >= 0 ? proc.evo[(size_t) carry] : proc.pocket[(size_t) carryPocket];
                proc.evoCross (src, tgt); startAnim();
            }
            carry = -1; carryPocket = -1; carryAt = {}; repaint(); return;
        }
        if (downPocket >= 0 && ! e.mouseWasDraggedSinceMouseDown())
        {
            if (e.mods.isPopupMenu()) { pocketMenu (downPocket); return; }
            proc.evoAuditionNode (proc.pocket[(size_t) downPocket]);
            if (proc.evoIdea) proc.fxApply (proc.pocket[(size_t) downPocket].fx);
            return;
        }
        if (morphKid >= 0)
        {
            const auto& kids = proc.evo[(size_t) proc.evoCenter].kids;
            const int target = isPositiveAndBelow (morphKid, (int) kids.size()) ? kids[(size_t) morphKid] : -1;
            const bool onKid = target >= 0 && e.position.getDistanceFrom (kidPos (morphKid, 1.0f)) < kidR;
            morphKid = -1;
            if (onKid) { proc.evoPick (target); startAnim(); }
            else proc.evoAudition (proc.evoCenter, false);   // let go: back to the middle sound
            repaint(); return;
        }
        if (dragFired || e.mouseWasDraggedSinceMouseDown()) return;
        if (downAction == 0) { saveMenu (actionNode, nullptr); return; }
        const int n = nodeAt (e.position);
        if (n < 0) return;
        if (e.mods.isPopupMenu()) { nodeMenu (n); return; }
        if (n == proc.evoCenter) { proc.evoAudition (n); return; }
        proc.evoPick (n); startAnim();   // v0.38: you chose it - it grows, MY TASTE learns
        if (onChanged) onChanged();
    }
    void spacePressed()
    {
        if (proc.loopPlaying()) proc.stopLoop();
        else if (proc.evoCenter >= 0) { proc.evoAudition (proc.evoCenter); if (proc.evoIdea) proc.evoApplyIdea (proc.evoCenter); }
    }
    void pocketMenu (int i)
    {
        PopupMenu m;
        m.addSectionHeader (proc.pocket[(size_t) i].name);
        m.addItem (1, "Hear it");
        m.addItem (2, "Cross it with the middle idea");
        m.addItem (3, "Start a new tree from it");
        m.addSeparator();
        m.addItem (4, "Take it out of the POCKET");
        m.showMenuAsync (PopupMenu::Options(), [this, i, safe = SafePointer<EvolvePage> (this)] (int r)
        {
            if (safe == nullptr || r == 0 || ! isPositiveAndBelow (i, (int) proc.pocket.size())) return;
            const auto n = proc.pocket[(size_t) i];
            if (r == 1) proc.evoAuditionNode (n);
            if (r == 2 && proc.evoCenter >= 0) { proc.evoCross (n, proc.evoCenter); startAnim(); }
            if (r == 3) { proc.evoSeedNode (n); startAnim(); }
            if (r == 4) proc.evoPocketRemove (i);
            repaint();
        });
    }
    void refreshIdeaBtn()
    {
        ideaBtn.setButtonText (proc.evoIdea ? "SOUND + FX" : "SOUND ONLY");
        ideaBtn.selected = proc.evoIdea; ideaBtn.repaint();
    }
    void showDebugHover (int kidIndex)   // snapshots
    {
        if (proc.evoCenter >= 0 && isPositiveAndBelow (kidIndex, (int) proc.evo[(size_t) proc.evoCenter].kids.size()))
            hovered = proc.evo[(size_t) proc.evoCenter].kids[(size_t) kidIndex];
        animStart = 0; wasEmpty = proc.evo.empty(); layoutButtons(); repaint();
    }
    void showDebugMorph (int kidIndex, float t) { morphKid = kidIndex; morphT = t; animStart = 0; wasEmpty = proc.evo.empty(); layoutButtons(); repaint(); }
    void refreshLayout() { wasEmpty = proc.evo.empty(); layoutButtons(); repaint(); }
    void debugMap() { showMap (true); }

private:
    // ---- geometry (design pixels)
    Point<float> centrePos() const { return { (float) getWidth() * 0.5f + 40.0f, 400.0f }; }
    Point<float> parentPos() const { return { 250.0f, 330.0f }; }
    Point<float> kidPos (int k, float a) const
    {
        const float ang = MathConstants<float>::twoPi * (float) k / 6.0f - MathConstants<float>::halfPi + 0.52f;
        const auto c = centrePos();
        const float r = ringR * a;
        return { c.x + std::cos (ang) * r * 1.18f, c.y + std::sin (ang) * r };
    }
    Rectangle<int> bottomBar() const { return { 12, getHeight() - 84, getWidth() - 24, 76 }; }
    Rectangle<float> wildRect() const { const auto b = bottomBar(); return { (float) b.getX() + 120, (float) b.getY() + 15, 290, 10 }; }
    Rectangle<float> tasteRect() const { const auto b = bottomBar(); return { (float) b.getX() + 120, (float) b.getY() + 44, 290, 10 }; }
    static constexpr float centreR = 104.0f, kidR = 64.0f, ringR = 245.0f;
    float anim() const
    {
        if (animStart == 0) return 1.0f;
        const float x = jlimit (0.0f, 1.0f, (float) (Time::getMillisecondCounter() - animStart) / 420.0f);
        return 1.0f - (1.0f - x) * (1.0f - x) * (1.0f - x);
    }
    void startAnim() { animStart = Time::getMillisecondCounter(); hovered = -1; wasEmpty = proc.evo.empty(); layoutButtons(); repaint(); if (onChanged) onChanged(); }

    int nodeAt (Point<float> p) const
    {
        if (proc.evoCenter < 0 || proc.evoCenter >= (int) proc.evo.size()) return -1;
        const auto& c = proc.evo[(size_t) proc.evoCenter];
        if (p.getDistanceFrom (centrePos()) < centreR) return proc.evoCenter;
        for (int k = 0; k < (int) c.kids.size(); ++k) if (p.getDistanceFrom (kidPos (k, 1.0f)) < kidR) return c.kids[(size_t) k];
        if (c.parent >= 0 && p.getDistanceFrom (parentPos()) < 50.0f) return c.parent;
        const auto path = proc.evoPath();
        for (int i = 0; i < (int) path.size(); ++i) if (pathDot (i, (int) path.size()).expanded (4).contains (p)) return path[(size_t) i];
        return -1;
    }
    Point<float> posOf (int node) const
    {
        if (node == proc.evoCenter) return centrePos();
        const auto& c = proc.evo[(size_t) proc.evoCenter];
        for (int k = 0; k < (int) c.kids.size(); ++k) if (c.kids[(size_t) k] == node) return kidPos (k, 1.0f);
        if (node == c.parent) return parentPos();
        return { -1000, -1000 };
    }
    // SAVE / WAV / MIDI pills under the hovered bubble (0 / 1 / 2)
    Rectangle<float> actionRect (int node, int i) const
    {
        const auto p = posOf (node);
        const float r = node == proc.evoCenter ? centreR : kidR;
        return { p.x - 52.0f + (float) i * 54.0f, p.y + r + 34.0f, 50.0f, 22.0f };
    }
    int actionAt (Point<float> p)
    {
        if (! isPositiveAndBelow (hovered, (int) proc.evo.size()) || hovered == proc.evoCenter) return -1;
        for (int i = 0; i < 2; ++i) if (actionRect (hovered, i).expanded (3).contains (p)) { actionNode = hovered; return i; }
        return -1;
    }
    void drawActions (Graphics& g, int node)
    {
        const auto& t = kk::theme();
        static const char* names[] { "SAVE", "WAV" };
        for (int i = 0; i < 2; ++i)
        {
            const auto r = actionRect (node, i);
            g.setColour (t.glass.withMultipliedAlpha (1.4f)); g.fillRoundedRectangle (r, 11);
            g.setColour (t.accent); g.drawRoundedRectangle (r, 11, 1.2f);
            g.setFont (kk::modern::font (10.5f, true, 0.12f)); g.drawText (names[i], r, Justification::centred);
        }
    }
    Rectangle<float> pathDot (int i, int n) const
    {
        ignoreUnused (n);
        return { 40.0f + (float) i * 30.0f, 146.0f, 18.0f, 18.0f };
    }
    void paintPath (Graphics& g, const std::vector<int>& path)
    {
        const auto& t = kk::theme();
        if (path.empty()) return;
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.2f));
        g.drawText ("YOUR PATH", Rectangle<float> (40, 168, 200, 16), Justification::centredLeft);
        for (int i = 0; i < (int) path.size(); ++i)
        {
            const auto r = pathDot (i, (int) path.size());
            if (i > 0) { g.setColour (t.accent.withAlpha (0.4f)); g.drawLine (r.getX() - 12, r.getCentreY(), r.getX(), r.getCentreY(), 1.2f); }
            const bool last = i == (int) path.size() - 1;
            g.setColour (last ? t.accent : t.text.withAlpha (path[(size_t) i] == hovered ? 0.9f : 0.45f));
            if (last) g.fillEllipse (r); else g.drawEllipse (r.reduced (1), 1.4f);
        }
        if (isPositiveAndBelow (hovered, (int) proc.evo.size()))
            for (int i = 0; i < (int) path.size(); ++i)
                if (path[(size_t) i] == hovered && hovered != proc.evoCenter)
                {
                    g.setColour (t.text); g.setFont (kk::modern::font (11.0f, true, 0.06f));
                    g.drawText (proc.evo[(size_t) hovered].name + "  - click = back here", Rectangle<float> (40, 186, 500, 16), Justification::centredLeft);
                }
    }
    void drawBubble (Graphics& g, int node, Point<float> c, float r, bool isCentre, const String& tag)
    {
        const auto& t = kk::theme();
        const auto& n = proc.evo[(size_t) node];
        const bool hot = node == hovered;
        const Colour ring = n.isAudio() ? t.accent : streamColour (n.g.cat);
        // shadow, glass sphere, highlight
        g.setColour (Colours::black.withAlpha (t.night ? 0.45f : 0.16f)); g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c.translated (6, 12)));
        g.setGradientFill (ColourGradient (t.night ? Colour (0xd8262a31) : Colour (0xe6ffffff), c.x - r * 0.4f, c.y - r * 0.6f,
                                           t.night ? Colour (0xd0101215) : Colour (0xd6d9dde2), c.x + r * 0.5f, c.y + r, true));
        g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c));
        if (isCentre || hot)
        {
            g.setGradientFill (ColourGradient (t.accent.withAlpha (isCentre ? 0.28f : 0.18f), c.x, c.y, t.accent.withAlpha (0.0f), c.x + r * 1.4f, c.y, true));
            g.fillEllipse (Rectangle<float> (r * 2.8f, r * 2.8f).withCentre (c));
        }
        // waveform inside
        const float ww = r * 1.3f, wh = r * 0.62f;
        float peak = 0.0001f; for (auto v : n.wave) peak = std::max (peak, v);
        for (int b = 0; b < 64; ++b)
        {
            const float v = n.waveReady ? std::pow (n.wave[(size_t) b] / peak, 0.7f) : 0.05f + 0.04f * std::sin ((float) b * 0.6f + (float) Time::getMillisecondCounter() * 0.01f);
            const float h = std::max (1.0f, v * wh * 0.5f);
            const float x = c.x - ww * 0.5f + ww * (float) b / 64.0f;
            g.setColour ((isCentre ? t.accent : t.text.withAlpha (0.8f)).interpolatedWith (ring, (float) b / 140.0f).withAlpha (0.9f));
            g.fillRect (x, c.y - h - r * 0.08f, std::max (1.0f, ww / 90.0f), h * 2.0f);
        }
        // rim: category colour, amber on hover / middle
        g.setColour (isCentre ? t.accent : hot ? t.accent.withAlpha (0.9f) : ring.withAlpha (0.75f));
        g.drawEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c).reduced (0.5f), isCentre ? 2.4f : hot ? 2.0f : 1.4f);
        g.setColour (Colours::white.withAlpha (t.night ? 0.12f : 0.7f));
        Path refl; refl.addCentredArc (c.x, c.y, r * 0.86f, r * 0.86f, 0, -2.4f, -1.0f, true);
        g.strokePath (refl, PathStrokeType (r * 0.06f, PathStrokeType::curved, PathStrokeType::rounded));
        // IDEA: the effects it carries
        if (proc.evoIdea)
        {
            // the two that colour it most
            static const int prio[] { kk::rkHalf, kk::rkGate, kk::rkFlanger, kk::rkPhaser, kk::rkLofi, kk::rkDelay, kk::rkReverb, kk::rkChorus, kk::rkDrive, kk::rkEq, kk::rkWidth };
            StringArray on;
            for (int sl : prio) if (n.fx.on[(size_t) sl]) on.add (kk::rackSlotName (sl));
            const String txt = on.isEmpty() ? String ("DRY") : on.size() == 1 ? on[0] : on[0] + " + " + on[1] + (on.size() > 2 ? " +" : "");
            g.setColour (on.isEmpty() ? t.dim : kk::accentText()); g.setFont (kk::modern::font (isCentre ? 11.5f : 9.5f, true, 0.06f));
            g.drawFittedText (txt, Rectangle<int> ((int) (c.x - r * 0.9f), (int) (c.y + r * 0.3f), (int) (r * 1.8f), 14), Justification::centred, 1, 0.7f);
        }
        // name
        g.setColour (t.text); g.setFont (kk::modern::font (isCentre ? 17.0f : 12.5f, true, 0.04f));
        g.drawFittedText (n.name, Rectangle<int> ((int) (c.x - r * 1.5f), (int) (c.y + r + 6), (int) (r * 3.0f), isCentre ? 24 : 30), Justification::centredTop, 2, 0.75f);
        if (isCentre)
        {
            g.setColour (kk::accentText()); g.setFont (kk::modern::font (11.0f, true, 0.3f));
            g.drawText ("GEN " + String (n.gen) + (n.isAudio() ? "  .  YOUR SOUND" : ""), Rectangle<float> (c.x - 100, c.y + r * 0.52f, 200, 16), Justification::centred);
        }
        if (tag.isNotEmpty())
        {
            g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.3f));
            g.drawText (tag, Rectangle<float> (c.x - 60, c.y - r - 18, 120, 14), Justification::centred);
        }
    }
    void paintEmpty (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto c = centrePos();
        Path circ; circ.addEllipse (Rectangle<float> (300, 300).withCentre (c));
        Path dashed; const float dl[] { 9.0f, 7.0f };
        PathStrokeType (1.6f).createDashedStroke (dashed, circ, dl, 2);
        const float pulse = 0.5f + 0.5f * std::sin ((float) Time::getMillisecondCounter() * 0.003f);
        g.setGradientFill (ColourGradient (t.accent.withAlpha (0.10f + 0.08f * pulse), c.x, c.y, t.accent.withAlpha (0.0f), c.x + 240, c.y, true));
        g.fillEllipse (Rectangle<float> (480, 480).withCentre (c));
        g.setColour (t.accent.withAlpha (0.8f)); g.fillPath (dashed);
        g.setColour (t.text); g.setFont (kk::modern::font (24.0f, true, 0.18f));
        g.drawText ("DROP ANY SOUND HERE", Rectangle<float> (600, 40).withCentre (c.translated (0, -26)), Justification::centred);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.0f, false, 0.08f));
        g.drawText ("a WAV, a vocal, a loop, a one-shot - it becomes the seed of a new family of sounds", Rectangle<float> (700, 20).withCentre (c.translated (0, 8)), Justification::centred);
    }
    void drawRail (Graphics& g, Rectangle<float> r, float value, const String& left, const String& right)
    {
        const auto& t = kk::theme();
        g.setColour (t.text); g.setFont (kk::modern::font (11.5f, true, 0.16f));
        g.drawText (left, Rectangle<float> (r.getX() - 104, r.getY() - 8, 94, 26), Justification::centredRight);
        g.drawText (right, Rectangle<float> (r.getRight() + 10, r.getY() - 8, 90, 26), Justification::centredLeft);
        kk::modern::well (g, r, 5.0f);
        const float x = r.getX() + r.getWidth() * value;
        g.setGradientFill (ColourGradient (t.accent.withAlpha (0.3f), r.getX(), 0, t.accent, x, 0, false));
        g.fillRoundedRectangle (r.withRight (x), 5.0f);
        g.setColour (t.night ? Colour (0xffe8eaed) : Colours::white); g.fillEllipse (Rectangle<float> (18, 18).withCentre ({ x, r.getCentreY() }));
        g.setColour (t.accent); g.drawEllipse (Rectangle<float> (18, 18).withCentre ({ x, r.getCentreY() }), 1.6f);
    }
    void drawWild (Graphics& g)
    {
        drawRail (g, wildRect(), proc.evoWild, "SAFE", "WILD");
        drawRail (g, tasteRect(), proc.evoTasteAmt, "ANY", "MY TASTE");
        const int picks = proc.tastePicks();
        g.setColour (kk::theme().dim); g.setFont (kk::modern::font (10.0f, true, 0.06f));
        g.drawText (picks < 2 ? String ("pick a few - it learns") : "learned from " + String (picks) + " picks",
                    Rectangle<float> (tasteRect().getX(), tasteRect().getBottom() + 3, tasteRect().getWidth(), 13), Justification::centred);
    }
    void setWild (float x) { const auto r = wildRect(); proc.evoWild = jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth()); repaint (bottomBar()); }
    void setTaste (float x) { const auto r = tasteRect(); proc.evoTasteAmt = jlimit (0.0f, 1.0f, (x - r.getX()) / r.getWidth()); repaint (bottomBar()); }
    void layoutButtons()
    {
        const int W = getWidth();
        themeBtn.setBounds (W - 92, 24, 56, 46);
        studioBtn.setBounds (W - 92 - 132, 26, 124, 42);
        ideaBtn.setBounds (studioBtn.getX() - 262, 26, 254, 42);
        refreshIdeaBtn();
        {
            int lx = 40;
            for (auto& lb : layerBtns) { lb->setBounds (lx, 96, 80, 32); lx += lb->getWidth() + 5; }
            const int rx = W - 176;
            aliveBtn.setBounds (rx, 110, 150, 40); catchBtn.setBounds (rx, 156, 150, 40);
            mapBtn.setBounds (rx, 230, 150, 40); worldBtn.setBounds (rx, 276, 150, 40); matchBtn.setBounds (rx, 322, 150, 40);
        }
        const auto b = bottomBar();
        int x = b.getX() + 540;
        againBtn.setBounds (x, b.getY() + 18, 146, 40); x += 152;
        seedBtn.setBounds (x, b.getY() + 18, 112, 40); x += 122;
        saveBtn.setBounds (x, b.getY() + 18, 84, 40); x += 92;
        dragWav.setBounds (x, b.getY() + 16, 140, 44); x += 148;
        melodyBtn.setBounds (x, b.getY() + 18, 90, 40); x += 96;
        melodyRoll = {};
        const auto c = centrePos();
        bankBtn.setBounds ((int) c.x - 330, (int) c.y + 190, 210, 42);
        diceBtn.setBounds ((int) c.x - 105, (int) c.y + 190, 210, 42);
        currentBtn.setBounds ((int) c.x + 120, (int) c.y + 190, 230, 42);
        const bool empty = proc.evo.empty();
        for (auto* bt : { &bankBtn, &diceBtn, &currentBtn }) bt->setVisible (empty);
        for (auto* bt : { &againBtn, &saveBtn }) bt->setVisible (! empty);
        dragWav.setVisible (! empty);
        melodyBtn.setVisible (! empty);
        for (auto* c : { (Component*) &aliveBtn, (Component*) &catchBtn, (Component*) &mapBtn }) c->setVisible (! empty);
        for (auto& lb : layerBtns) lb->setVisible (! empty);
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        // hover long enough = hear it (the keys play it); leave = back to the middle
        const auto now = Time::getMillisecondCounter();
        if (hovered >= 0 && hovered != proc.evoCenter && ! auditioned && now - hoverSince > 260 && morphKid < 0)
        {
            auditioned = true; lastHeard = hovered;
            if (proc.evoIdea) proc.evoApplyIdea (hovered);   // its effects first, then you hear it
            proc.evoAudition (hovered);
        }
        if (hovered < 0 && lastHeard >= 0 && lastHeard != proc.evoCenter && now - hoverSince > 400 && morphKid < 0)
        { proc.evoAudition (proc.evoCenter, false); if (proc.evoIdea) proc.evoApplyIdea (proc.evoCenter); lastHeard = proc.evoCenter; }
        const bool empty = proc.evo.empty();
        if (empty != wasEmpty) { wasEmpty = empty; layoutButtons(); }
        if (pocketHover >= 0 && ! pocketHeard && now - pocketSince > 280 && isPositiveAndBelow (pocketHover, (int) proc.pocket.size()) && carry < 0 && carryPocket < 0)
        {
            pocketHeard = true;
            const auto& pn = proc.pocket[(size_t) pocketHover];
            if (proc.evoIdea) proc.fxApply (pn.fx);
            proc.evoAuditionNode (pn, true);
            lastHeard = -2;
        }
        if (pocketHover < 0 && lastHeard == -2 && hovered < 0 && now - pocketSince > 400)
        { proc.evoAudition (proc.evoCenter, false); if (proc.evoIdea) proc.evoApplyIdea (proc.evoCenter); lastHeard = proc.evoCenter; }
        if (alive)   // ALIVE: a small step every 1.6 s (faster when WILD)
        {
            const uint32 every = (uint32) (2200 - 1400 * proc.evoWild);
            if (now - lastAlive > every && hovered < 0) { lastAlive = now; proc.evoAliveStep (proc.evoWild); }
            repaint (Rectangle<float> (centreR * 2.0f + 60, centreR * 2.0f + 80).withCentre (centrePos()).toNearestInt());
        }
        if (proc.evoVer.load() != lastVer || animStart != 0 || empty)
        {
            lastVer = proc.evoVer.load();
            if (animStart != 0 && now - animStart > 450) animStart = 0;
            repaint();
        }
    }
    void seedMenu (Component* from)
    {
        PopupMenu m;
        m.addItem (1, "A sound from the bank...");
        m.addItem (2, "Surprise me (random seed)");
        m.addItem (3, "The current sound");
        m.addItem (4, "Your own sound (WAV, MP3 ...)...");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (from), [this, safe = SafePointer<EvolvePage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1 && onPickSeed) onPickSeed();
            if (r == 2) { proc.evoSeedRandom(); startAnim(); }
            if (r == 3) { proc.evoSeedCurrent(); startAnim(); }
            if (r == 4)
            {
                chooser = std::make_unique<FileChooser> ("Your sound as the seed", File::getSpecialLocation (File::userMusicDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
                chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, safe] (const FileChooser& fc)
                { if (safe != nullptr && fc.getResult().existsAsFile() && proc.evoSeedFromFile (fc.getResult())) startAnim(); });
            }
        });
    }
    void saveMenu (int node, Component* from)
    {
        if (! isPositiveAndBelow (node, (int) proc.evo.size())) return;
        const bool audio = proc.evo[(size_t) node].isAudio();
        PopupMenu m;
        m.addSectionHeader ("SAVE  " + proc.evo[(size_t) node].name);
        m.addItem (1, "As a PRESET (in the sound library)...", ! audio);
        m.addItem (2, "As a SOUND into a folder / sound kit (WAV)...");
        auto opts = from != nullptr ? PopupMenu::Options().withTargetComponent (from) : PopupMenu::Options();
        m.showMenuAsync (opts, [this, node, from, safe = SafePointer<EvolvePage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1 && onSavePreset) onSavePreset (node);
            if (r == 2) if (auto snd = proc.evoAsSound (node)) saveToFolderMenu (proc, { snd }, from != nullptr ? from : (Component*) this, [] (String) {});
        });
    }
    void nodeMenu (int node)
    {
        const auto& n = proc.evo[(size_t) node];
        PopupMenu m;
        m.addSectionHeader (n.name);
        m.addItem (1, "Make it the middle (grow from it)");
        m.addItem (2, "Hear it");
        m.addItem (3, "Save...");
        m.addItem (4, "To STUDIO as PARENT A", ! n.isAudio());
        m.addItem (5, "To STUDIO as PARENT B", ! n.isAudio());
        m.addItem (7, "Keep in the POCKET");
        m.addSeparator();
        m.addItem (6, "Not my taste (fewer like this)", node != proc.evoCenter);
        m.showMenuAsync (PopupMenu::Options(), [this, node, safe = SafePointer<EvolvePage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1) { proc.evoFocus (node); startAnim(); }
            if (r == 2) proc.evoAudition (node);
            if (r == 3) saveMenu (node, nullptr);
            if (r == 4 || r == 5) { auto g = proc.evo[(size_t) node].g; g.name = proc.evo[(size_t) node].name; proc.setParentGenome (r - 4, g); }
            if (r == 6) { proc.evoNotMyTaste (node); hovered = -1; repaint(); }
            if (r == 7) { proc.evoPocketAdd (node); repaint(); }
        });
    }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton studioBtn { lnf }, themeBtn { lnf }, seedBtn { lnf }, againBtn { lnf }, saveBtn { lnf }, bankBtn { lnf }, diceBtn { lnf }, currentBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    HotButton melodyBtn { lnf };
    Rectangle<float> melodyRoll;
    std::unique_ptr<FileChooser> chooser;
    int hovered = -1, lastHeard = -1, downNode = -1, downAction = -1, actionNode = -1, morphKid = -1, lastVer = -1;
    uint32 hoverSince = 0, animStart = 0, lastMorph = 0;
    float morphT = 0.0f;
    bool auditioned = false, dropHot = false, dragFired = false, draggingWild = false, draggingTaste = false, wasEmpty = true;
    HotButton ideaBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> layerBtns;
    HotButton aliveBtn { lnf }, catchBtn { lnf }, mapBtn { lnf }, worldBtn { lnf }, matchBtn { lnf };
    EvoMapView map { proc };
    MatchView match { proc, lnf };
    bool alive = false; uint32 lastAlive = 0;
    // POCKET + carrying a bubble (drop it onto another = a cross, onto the POCKET = keep it)
    int carry = -1, carryPocket = -1, pocketHover = -1, downPocket = -1; Point<float> carryAt;
    uint32 pocketSince = 0; bool pocketHeard = false;
    static constexpr int pocketSlots = 8;
    Rectangle<float> pocketSlot (int i) const { return { 46.0f, 240.0f + (float) i * 56.0f, 48.0f, 48.0f }; }
    Rectangle<float> pocketArea() const { return { 30.0f, 214.0f, 80.0f, 26.0f + pocketSlots * 56.0f }; }
    int pocketAt (Point<float> p) const { for (int i = 0; i < pocketSlots; ++i) if (pocketSlot (i).expanded (4).contains (p)) return i; return -1; }
    void paintPocket (Graphics& g)
    {
        const auto& t = kk::theme();
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.25f));
        g.drawText ("POCKET", Rectangle<float> (20, 216, 100, 16), Justification::centred);
        for (int i = 0; i < pocketSlots; ++i)
        {
            const auto r = pocketSlot (i);
            if (i < (int) proc.pocket.size())
            {
                const auto& n = proc.pocket[(size_t) i];
                g.setColour (t.night ? Colour (0xd8262a31) : Colour (0xe6ffffff)); g.fillEllipse (r);
                float pk = 0.0001f; for (auto v : n.wave) pk = std::max (pk, v);
                for (int b = 0; b < 64; b += 2)
                {
                    const float h = (n.waveReady ? std::pow (n.wave[(size_t) b] / pk, 0.7f) : 0.15f) * r.getHeight() * 0.22f;
                    g.setColour (t.accent.withAlpha (0.85f)); g.fillRect (r.getX() + 8 + (r.getWidth() - 16) * (float) b / 64.0f, r.getCentreY() - h, 1.2f, h * 2);
                }
                g.setColour (i == pocketHover ? t.accent : t.text.withAlpha (0.35f)); g.drawEllipse (r, i == pocketHover ? 2.0f : 1.2f);
                if (i == pocketHover) { g.setColour (t.text); g.setFont (kk::modern::font (11.5f, true, 0.04f)); g.drawText (n.name, Rectangle<float> (r.getRight() + 8, r.getCentreY() - 8, 260, 16), Justification::centredLeft); }
            }
            else
            {
                Path c; c.addEllipse (r.reduced (4)); Path d; const float dl[] { 4.0f, 4.0f };
                PathStrokeType (1.0f).createDashedStroke (d, c, dl, 2);
                g.setColour (t.text.withAlpha (carry >= 0 ? 0.6f : 0.2f)); g.fillPath (d);
            }
        }
        if (proc.pocket.empty()) { g.setColour (t.dim.withAlpha (0.8f)); g.setFont (kk::modern::font (9.5f, true, 0.05f)); g.drawFittedText ("drag a\nbubble\nhere", pocketSlot (0).toNearestInt(), Justification::centred, 3, 0.8f); }
    }
};

//==============================================================================
// v0.40 MELODY: the melody page. SURPRISE ME = 8 melodies in your key / scale / bars, played with the sound you choose.
// FROM MY MELODY = your melody from FL (LISTEN while FL plays it, or drop a .mid) -> its key is found -> 8 variations.
// Hover = hear it in time.  Click = it becomes the parent and 8 children grow.  Drag = the MIDI into FL.
// v0.42 MIDI SHRED: the loop goes into the shredder, notes fall out into the bin
class ShredView : public Component, private Timer
{
public:
    void start (const juce::AudioBuffer<float>* audio, const std::vector<kk::mel::Note>& notes, const String& name)
    {
        wave.fill (0.0f);
        if (audio != nullptr && audio->getNumSamples() > 0)
        {
            const int n = audio->getNumSamples();
            float mx = 1e-6f;
            for (int k = 0; k < 64; ++k) { float pk = 0; for (int i = k * n / 64; i < (k + 1) * n / 64; ++i) pk = std::max (pk, std::abs (audio->getSample (0, i))); wave[(size_t) k] = pk; mx = std::max (mx, pk); }
            for (auto& v : wave) v /= mx;
        }
        noteCount = (int) notes.size(); title = name;
        pitches.clear(); for (auto& x : notes) pitches.push_back (x.pitch);
        t0 = Time::getMillisecondCounter();
        setVisible (true); toFront (false); startTimerHz (60);
    }
    std::function<void()> onDone;
    void paint (Graphics& g) override
    {
        const auto& th = kk::theme();
        const float t = (float) (Time::getMillisecondCounter() - t0) / 2400.0f;
        auto r = getLocalBounds().toFloat();
        g.setColour (th.night ? Colour (0xf00d1014) : Colour (0xf0eef0f2)); g.fillRoundedRectangle (r, 14);
        const float cx = r.getCentreX();
        // the shredder
        const auto body = Rectangle<float> (cx - 170, r.getY() + r.getHeight() * 0.36f, 340, 70);
        // the paper with the loop's wave goes down into it
        const float feed = jlimit (0.0f, 1.0f, t / 0.55f);
        const auto paper = Rectangle<float> (cx - 120, body.getY() - 150 + 150 * feed, 240, 150);
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (Rectangle<float> (r.getX(), r.getY(), r.getWidth(), body.getY() - r.getY()).toNearestInt());
            g.setColour (Colour (0xffeef7ea)); g.fillRect (paper);
            g.setColour (Colour (0xff1b5e20)); g.setFont (kk::modern::font (13.0f, true, 0.12f));
            g.drawText ("MIDI SHRED", paper.withHeight (26), Justification::centred);
            const auto wr = paper.reduced (14, 34);
            for (int k = 0; k < 64; ++k) { const float h = std::max (1.0f, wr.getHeight() * wave[(size_t) k]); g.fillRect (wr.getX() + wr.getWidth() * (float) k / 64.0f, wr.getCentreY() - h * 0.5f, wr.getWidth() / 64.0f - 0.5f, h); }
        }
        g.setColour (Colour (0xff2b2f36)); g.fillRoundedRectangle (body, 10);
        g.setColour (Colour (0xff111316)); g.fillRoundedRectangle (body.reduced (40, 28).withHeight (8), 3);
        g.setColour (Colour (0xff3a3f48)); g.drawRoundedRectangle (body, 10, 2.0f);
        g.setColour (Colours::white.withAlpha (0.8f)); g.setFont (kk::modern::font (12.0f, true, 0.3f));
        g.drawText (title.isEmpty() ? String ("SHREDDING ...") : title, body.withTrimmedTop (44), Justification::centred);
        // strips and notes fall
        const auto bin = Rectangle<float> (cx - 110, r.getBottom() - 120, 220, 100);
        const float fall = jlimit (0.0f, 1.0f, (t - 0.35f) / 0.6f);
        for (int k = 0; k < 14 && t > 0.3f; ++k)
        {
            const float x = body.getX() + 50 + (float) k * 17.0f, y = body.getBottom() + 4 + (bin.getY() - body.getBottom() - 30) * jlimit (0.0f, 1.0f, fall * (0.7f + 0.03f * (float) (k % 5)));
            g.setColour (Colour (0xffeef7ea).withAlpha (0.8f)); g.fillRect (x, body.getBottom() + 2, 9.0f, std::max (4.0f, (y - body.getBottom()) * 0.4f));
        }
        Random rr (noteCount * 31 + 7);
        const int shown = std::min (noteCount, 40);
        for (int i = 0; i < shown; ++i)
        {
            const float delay = 0.4f + 0.5f * (float) i / (float) std::max (1, shown);
            const float u = jlimit (0.0f, 1.0f, (t - delay) / 0.45f);
            if (u <= 0) continue;
            const float x0 = body.getX() + 40 + rr.nextFloat() * 260, xt = bin.getX() + 20 + rr.nextFloat() * 180;
            const float y = body.getBottom() + (bin.getY() + 30 + rr.nextFloat() * 50 - body.getBottom()) * u * u;
            g.setColour (Colour (0xff36ff6a).withAlpha (0.9f)); g.fillRoundedRectangle (x0 + (xt - x0) * u, y, 16, 7, 2);
        }
        g.setColour (Colour (0xff1d4ed8)); g.fillRoundedRectangle (bin, 12);
        g.setColour (Colours::white); g.setFont (kk::modern::font (16.0f, true, 0.1f));
        g.drawText (t > 1.0f ? String (noteCount) + " NOTES" : String ("MIDI"), bin.withTrimmedTop (60), Justification::centred);
    }
    void mouseUp (const MouseEvent&) override { finish(); }
private:
    void timerCallback() override { repaint(); if (Time::getMillisecondCounter() - t0 > 2900) finish(); }
    void finish() { stopTimer(); setVisible (false); if (onDone) onDone(); }
    std::array<float, 64> wave {};
    std::vector<int> pitches;
    int noteCount = 0; uint32 t0 = 0; String title;
};

class MelodyPage : public Component, public FileDragAndDropTarget, private Timer
{
public:
    MelodyPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (surpriseTab, "SURPRISE ME", "New melodies from nothing - in the key, scale and length you choose", [this] { proc.melFromMine = false; refresh(); });
        btn (mineTab, "FROM MY MELODY", "Your own melody: LISTEN while FL plays it, drop a .mid - or drop AUDIO (a vocal, a sample, a recorded melody) and it becomes MIDI. Then GENERATE makes variations of it", [this] { proc.melFromMine = true; refresh(); });
        for (int k = 0; k < 12; ++k)
        {
            auto b = std::make_unique<HotButton> (lnf, kk::mel::keyName (k)); b->framed = true; b->setTooltip ("Key: " + String (kk::mel::keyName (k)));
            b->onClick = [this, k] { setKey (k); };
            addAndMakeVisible (*b); keyBtns.push_back (std::move (b));
        }
        for (int sc = 0; sc < kk::mel::numScales; ++sc) scaleBox.addItem (kk::mel::scaleName (sc), sc + 1);
        scaleBox.onChange = [this] { setScale (scaleBox.getSelectedId() - 1); };
        scaleBox.setTooltip ("The scale: minor, major, dorian, phrygian ...");
        addAndMakeVisible (scaleBox);
        static const int bars[] { 4, 8, 16 };
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, String (bars[i]) + " BARS"); b->framed = true; b->setTooltip ("How long the melodies are");
            b->onClick = [this, i] { proc.melBars = bars[i]; refresh(); };
            addAndMakeVisible (*b); barBtns.push_back (std::move (b));
        }
        static const char* ranges[] { "LOW", "MID", "HIGH" };
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, ranges[i]); b->framed = true; b->setTooltip ("Where the melodies sit: low, middle or high");
            b->onClick = [this, i] { proc.melStyle.range = (float) i * 0.5f; refresh(); };
            addAndMakeVisible (*b); rangeBtns.push_back (std::move (b));
        }
        auto bar = [this] (Slider& sl, double v, std::function<String (double)> txt, const String& tip, std::function<void (double)> set)
        {
            sl.setSliderStyle (Slider::LinearBar); sl.setRange (0.0, 1.0, 0.01); sl.setValue (v, dontSendNotification);
            sl.textFromValueFunction = std::move (txt);
            sl.setColour (Slider::trackColourId, lnf.skin->accent.withAlpha (0.5f));
            sl.setColour (Slider::textBoxTextColourId, kk::theme().text);
            sl.setTooltip (tip); sl.onValueChange = [&sl, set] { set (sl.getValue()); };
            sl.updateText(); addAndMakeVisible (sl);
        };
        bar (busySl, proc.melStyle.density, [] (double v) { return v < 0.34 ? String ("CALM") : v < 0.67 ? String ("FLOWING") : String ("BUSY"); },
             "CALM = few long notes ... BUSY = many quick notes", [this] (double v) { proc.melStyle.density = (float) v; });
        bar (wildSl, proc.melStyle.wild, [] (double v) { return v < 0.34 ? String ("SAFE") : v < 0.67 ? String ("MIXED") : String ("WILD"); },
             "SAFE = the children stay close to the parent ... WILD = they change a lot", [this] (double v) { proc.melStyle.wild = (float) v; });
        // v0.41 GENRES: the style + its tempo, and what the MIDI holds (melody / chords)
        for (int gi = -1; gi < kk::mel::numGenres; ++gi)
        {
            auto b = std::make_unique<HotButton> (lnf, kk::mel::genreName (gi)); b->framed = true;
            b->setTooltip (gi < 0 ? String ("FREE: no style rules - any key, any scale, any feel")
                                  : String (kk::mel::genreName (gi)) + ": " + kk::mel::genreInfo (gi).hint + "  (" + String (kk::mel::genreInfo (gi).lo) + "-" + String (kk::mel::genreInfo (gi).hi) + " BPM)");
            b->onClick = [this, gi] { proc.melSetGenre (gi); bpmSl.setValue (proc.melBpm, dontSendNotification); refresh(); };
            addAndMakeVisible (*b); genreBtns.push_back (std::move (b));
        }
        bpmSl.setSliderStyle (Slider::LinearBar); bpmSl.setRange (60.0, 180.0, 1.0); bpmSl.setValue (proc.melBpm, dontSendNotification);
        bpmSl.textFromValueFunction = [] (double v) { return String (roundToInt (v)) + " BPM"; };
        bpmSl.setColour (Slider::trackColourId, lnf.skin->accent.withAlpha (0.5f));
        bpmSl.setColour (Slider::textBoxTextColourId, kk::theme().text);
        bpmSl.setTooltip ("The tempo of the melodies: the rhythm follows it (slow trap gets a finer grid) and the .mid carries it");
        bpmSl.onValueChange = [this] { proc.melBpm = (float) bpmSl.getValue(); };
        bpmSl.updateText(); addAndMakeVisible (bpmSl);
        static const char* layerNames[] { "MELODY", "+ CHORDS", "CHORDS" };
        static const char* layerTips[] { "Hear and drag only the melody", "The melody and its chords together (pad / stabs / arps of the genre)", "Only the chords" };
        for (int i = 0; i < 3; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, layerNames[i]); b->framed = true; b->setTooltip (layerTips[i]);
            b->onClick = [this, i] { proc.melLayers = i; if (proc.melPlaying >= 0) proc.melPlay (proc.melPlaying); refresh(); };
            addAndMakeVisible (*b); layerBtns.push_back (std::move (b));
        }
        // v0.42 quick sound: a family + < > right here (the melody keeps playing while you flip through sounds)
        // v0.43: no preset families - what EXCITES the matter, then < > mutates it (ALCHEMY makes the whole sound)
        for (int i = 0; i < (int) kk::alc::exciters().size(); ++i) catBox.addItem (kk::alc::exciters()[(size_t) i].name, i + 1);
        catBox.setSelectedId (proc.alcExc.load() + 1, dontSendNotification);
        catBox.onChange = [this] { stepSound (0); };
        catBox.setTooltip ("What excites the matter of the melody sound - then < > mutates it while the melody plays");
        addAndMakeVisible (catBox);
        btn (prevSnd, "<", "The previous mutation", [this] { stepSound (-1); });
        btn (nextSnd, ">", "A new mutation of this matter", [this] { stepSound (1); });
        btn (curSoundBtn, "", "", [this] {}); curSoundBtn.setVisible (false);
        btn (pickSoundBtn, "ALCHEMY", "Make the sound the melodies play with in ALCHEMY (then USE IT)", [this] { if (onPickSound) onPickSound(); });
        btn (listenBtn, "LISTEN", "LISTEN: press it, then play your melody in FL - press it again when it has played once.  Notes on EVOLVE's channel are caught as MIDI.  Another plugin (Nexus ...)? Put EVOLVE as an EFFECT on that plugin's mixer track: LISTEN hears its sound and turns it into notes", [this]
        {
            if (proc.melListening()) { proc.melListen (false); note = proc.melHasMine ? "got it" + String (proc.melListenSource == "AUDIO" ? " (from the sound)" : "") + ": " + String ((int) proc.melMine.notes.size()) + " notes, " + kk::mel::keyName (proc.melMine.key) + " " + kk::mel::scaleName (proc.melMine.scale) : String ("nothing heard - play your melody in FL while LISTEN is on (or EVOLVE as an effect on the plugin's track)"); }
            else { proc.melListen (true); note = "listening... press PLAY in FL (your pattern in this plugin's piano roll)"; }
            refresh();
        });
        btn (hearMineBtn, "HEAR MINE", "Hear your melody with the sound on the keys", [this] { if (proc.melPlaying == -2) proc.melPlay (-1); else proc.melPlayMine(); });
        btn (shredBtn, "MIDI SHRED", "MIDI SHRED: feed it a loop (a WAV / MP3 of a melody) - it shreds it into MIDI notes", [this]
        {
            chooser = std::make_unique<FileChooser> ("A loop to shred into MIDI", File(), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
            chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this] (const FileChooser& fc) { if (fc.getResult().existsAsFile()) shredFile (fc.getResult()); });
        });
        addChildComponent (shredView);
        shredView.onDone = [this] { refresh(); };
        btn (generateBtn, "GENERATE", "8 new melodies (SURPRISE ME) - or 8 variations of your melody (FROM MY MELODY)", [this] { generate(); });
        generateBtn.hero = true;
        btn (backBtn, "BACK", "Back to the melody this one came from", [this] { proc.melBack(); startAnim(); });
        btn (againBtn, "8 NEW", "Eight new children of the parent melody", [this] { if (proc.melParent >= 0) { proc.melEvolve (proc.melParent, true); startAnim(); } else generate(); });
        btn (stopBtn, "STOP", "Stop the melody (SPACE)", [this] { proc.melPlay (-1); });
        dragMine.makeFile = [this] { return proc.melExport (-2); };
        dragMine.setTooltip ("Your melody (cleaned up, on the grid) as MIDI");
        addAndMakeVisible (dragMine);
        setWantsKeyboardFocus (false);
        startTimerHz (30);
        refresh();
    }
    std::function<void()> onPickSound;
    void visibilityChanged() override { if (isVisible()) refresh(); else if (proc.melPlaying != -1) proc.melPlay (-1); }
    void spacePressed() { if (proc.loopPlaying()) proc.melPlay (-1); else if (lastHover >= 0) proc.melPlay (lastHover); }

    bool isInterestedInFileDrag (const StringArray& f) override { for (auto& x : f) if (File (x).hasFileExtension ("mid;midi;wav;aif;aiff;flac;mp3;ogg")) return true; return false; }
    void fileDragEnter (const StringArray&, int, int) override { dropHot = true; repaint(); }
    void fileDragExit (const StringArray&) override { dropHot = false; repaint(); }
    void filesDropped (const StringArray& files, int, int) override
    {
        dropHot = false;
        for (auto& x : files)
            if (File (x).hasFileExtension ("mid;midi"))
            {
                note = proc.melLoadMidiFile (File (x)) ? "your melody: " + String ((int) proc.melMine.notes.size()) + " notes, " + kk::mel::keyName (proc.melMine.key) + " " + kk::mel::scaleName (proc.melMine.scale) + " - press GENERATE"
                                                       : String ("could not read melody notes from ") + File (x).getFileName();
                break;
            }
            else if (File (x).hasFileExtension ("wav;aif;aiff;flac;mp3;ogg"))   // v0.41 AUDIO -> MIDI, v0.42 with the shredder
            {
                shredFile (File (x));
                break;
            }
        refresh();
    }

    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("MELODY", 24, 12, 200, 40, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (14.0f, true, 0.04f));
        g.drawText ("melodies that grow - in your key, with your sound.  hover = hear   click = it grows   drag = MIDI into FL", 200, 18, getWidth() - 220, 28, Justification::centredLeft);
        // left panel
        kk::modern::plate (g, left().toFloat(), 16.0f);
        auto cap = [&] (const String& txt, int y) { g.setColour (t.text); g.setFont (kk::modern::font (11.5f, true, 0.25f)); g.drawText (txt, left().getX() + 16, y, 280, 16, Justification::centredLeft); };
        cap ("KEY", keyBtns[0]->getY() - 20);
        cap ("SCALE", scaleBox.getY() - 20);
        cap ("LENGTH", barBtns[0]->getY() - 20);
        cap ("NOTES", busySl.getY() - 20);
        cap ("RANGE", rangeBtns[0]->getY() - 20);
        cap ("CHILDREN", wildSl.getY() - 20);
        cap ("PLAYS WITH", curSoundBtn.getY() - 40);
        g.setColour (kk::accentText()); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawFittedText (soundName(), Rectangle<int> (left().getX() + 16, curSoundBtn.getY() - 24, left().getWidth() - 32, 20), Justification::centredLeft, 1, 0.7f);
        if (proc.melFromMine)
        {
            cap ("YOUR MELODY", listenBtn.getY() - 20);
            const auto r = mineRoll().toFloat();
            kk::modern::well (g, r, 8.0f);
            if (proc.melHasMine) drawRoll (g, r.reduced (6, 5), proc.melMine, proc.melPlaying == -2, t.accent);
            else { g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.04f)); g.drawFittedText ("LISTEN + play it in FL\nor drop a .mid or AUDIO file here", r.toNearestInt(), Justification::centred, 2); }
            if (proc.melHasMine) { g.setColour (t.text); g.setFont (kk::modern::font (11.5f, true, 0.1f)); g.drawText (String (kk::mel::keyName (proc.melMine.key)) + " " + kk::mel::scaleName (proc.melMine.scale) + "  .  " + String (proc.melMine.bars) + " BARS", r.toNearestInt().translated (0, (int) r.getHeight() + 2).withHeight (16), Justification::centredLeft); }
        }
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (12.0f, true, 0.03f)); g.drawFittedText (note, Rectangle<int> (left().getX() + 16, generateBtn.getY() - 40, left().getWidth() - 32, 34), Justification::centredLeft, 2, 0.8f); }
        // parent strip
        const auto ps = parentStrip();
        if (isPositiveAndBelow (proc.melParent, (int) proc.mels.size()))
        {
            const auto& pm = proc.mels[(size_t) proc.melParent];
            kk::modern::plate (g, ps.toFloat(), 14.0f);
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f));
            g.drawText ("PARENT", ps.getX() + 16, ps.getY() + 8, 100, 14, Justification::centredLeft);
            g.setColour (t.text); g.setFont (kk::modern::font (16.0f, true, 0.04f));
            g.drawText (pm.name + "   GEN " + String (pm.gen), ps.getX() + 16, ps.getY() + 22, 380, 22, Justification::centredLeft);
            drawRoll (g, Rectangle<float> ((float) ps.getX() + 400, (float) ps.getY() + 8, (float) ps.getWidth() - 640, (float) ps.getHeight() - 16), pm, proc.melPlaying == proc.melParent, t.accent);
        }
        else
        {
            g.setColour (t.dim); g.setFont (kk::modern::font (14.0f, true, 0.04f));
            const String gh = proc.melGenre >= 0 ? String (kk::mel::genreName (proc.melGenre)) + ": " + kk::mel::genreInfo (proc.melGenre).hint + "  -  " : String();
            g.drawFittedText (gh + (proc.melShown.empty() ? "choose the key, then GENERATE" : "click a melody = it becomes the parent and 8 children grow"),
                              ps.withTrimmedRight (240).withTrimmedLeft (8), Justification::centredLeft, 2, 0.8f);
        }
        // the 8 melodies
        const auto now = Time::getMillisecondCounter();
        for (int i = 0; i < (int) proc.melShown.size() && i < 8; ++i)
        {
            const int idx = proc.melShown[(size_t) i];
            if (! isPositiveAndBelow (idx, (int) proc.mels.size())) continue;
            const float a = jlimit (0.0f, 1.0f, (float) ((int) (now - animStart) - i * 70) / 260.0f);
            if (a <= 0.0f) continue;
            const float e = 1.0f - (1.0f - a) * (1.0f - a);
            auto r = card (i).toFloat();
            r = r.withSizeKeepingCentre (r.getWidth() * (0.85f + 0.15f * e), r.getHeight() * (0.85f + 0.15f * e));
            Graphics::ScopedSaveState ss (g);
            g.setOpacity (e);
            drawCard (g, r, proc.mels[(size_t) idx], idx);
        }
        if (dropHot) { g.setColour (t.accent.withAlpha (0.12f)); g.fillRect (getLocalBounds()); g.setColour (t.accent); g.setFont (kk::modern::font (26.0f, true, 0.2f)); g.drawText ("DROP = YOUR MELODY  (MIDI, or audio -> MIDI)", getLocalBounds(), Justification::centred); }
    }
    void resized() override
    {
        const auto L = left();
        int y = L.getY() + 14;
        surpriseTab.setBounds (L.getX() + 14, y, 142, 36); mineTab.setBounds (L.getX() + 162, y, 150, 36); y += 66;
        for (int k = 0; k < 12; ++k) keyBtns[(size_t) k]->setBounds (L.getX() + 14 + (k % 6) * 49, y + (k / 6) * 34, 45, 30);
        y += 68 + 26;
        scaleBox.setBounds (L.getX() + 14, y, L.getWidth() - 28, 30); y += 30 + 26;
        for (int i = 0; i < 3; ++i) barBtns[(size_t) i]->setBounds (L.getX() + 14 + i * 100, y, 94, 30);
        y += 30 + 26;
        busySl.setBounds (L.getX() + 14, y, L.getWidth() - 28, 26); y += 26 + 26;
        for (int i = 0; i < 3; ++i) rangeBtns[(size_t) i]->setBounds (L.getX() + 14 + i * 100, y, 94, 30);
        y += 30 + 26;
        wildSl.setBounds (L.getX() + 14, y, L.getWidth() - 28, 26); y += 26 + 50;
        prevSnd.setBounds (L.getX() + 14, y, 34, 32); catBox.setBounds (L.getX() + 52, y + 2, 132, 28); nextSnd.setBounds (L.getX() + 188, y, 34, 32);
        pickSoundBtn.setBounds (L.getX() + 228, y, 84, 32);
        curSoundBtn.setBounds (L.getX() + 14, y, 180, 32);
        y += 32 + 30;
        listenBtn.setBounds (L.getX() + 14, y, 100, 32); hearMineBtn.setBounds (L.getX() + 118, y, 96, 32); dragMine.setBounds (L.getX() + 218, y - 2, 94, 36);
        mineY = y + 40;
        generateBtn.setBounds (L.getX() + 14, L.getBottom() - 64, L.getWidth() - 28, 50);
        shredBtn.setBounds (getWidth() - 170, 14, 150, 34);
        shredView.setBounds (360, 110, getWidth() - 376, getHeight() - 126);
        {   // the genre row
            int x = 360; const int y = 66, h = 34;
            const int avail = getWidth() - 16 - x, lw = 92, bw = 112;
            const int cw = jmax (56, (avail - 3 * lw - bw - 24) / (int) genreBtns.size());
            for (auto& b : genreBtns) { b->setBounds (x, y, cw - 4, h); x += cw; }
            x += 8; bpmSl.setBounds (x, y + 4, bw, h - 8); x += bw + 12;
            for (auto& b : layerBtns) { b->setBounds (x, y, lw - 4, h); x += lw; }
        }
        const auto ps = parentStrip();
        stopBtn.setBounds (ps.getRight() - 84, ps.getCentreY() - 17, 72, 34);
        againBtn.setBounds (stopBtn.getX() - 82, ps.getCentreY() - 17, 76, 34);
        backBtn.setBounds (againBtn.getX() - 76, ps.getCentreY() - 17, 70, 34);
        refresh();
    }
    void mouseMove (const MouseEvent& e) override
    {
        const int h = cardAt (e.position);
        if (h != hoverCard) { hoverCard = h; hoverSince = Time::getMillisecondCounter(); heard = false; repaint(); }
    }
    void mouseExit (const MouseEvent&) override { hoverCard = -1; hoverSince = Time::getMillisecondCounter(); repaint(); }
    void mouseDown (const MouseEvent& e) override { downCard = cardAt (e.position); dragged = false; }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragged || downCard < 0 || e.getDistanceFromDragStart() < 8) return;
        dragged = true;
        const auto f = proc.melExport (proc.melShown[(size_t) downCard]);
        if (f.existsAsFile()) DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (dragged || e.mouseWasDraggedSinceMouseDown()) return;
        const int c = cardAt (e.position);
        if (c < 0) return;
        const int idx = proc.melShown[(size_t) c];
        if (e.mods.isPopupMenu()) { cardMenu (idx); return; }
        proc.melEvolve (idx, false);
        proc.melPlay (idx); lastHover = idx;
        startAnim();
    }
    void debugGenerate() { generate(); animStart = 0; repaint(); }
private:
    Rectangle<int> left() const { return { 16, 66, 328, getHeight() - 82 }; }
    Rectangle<int> mineRoll() const { return { left().getX() + 14, mineY, left().getWidth() - 28, 62 }; }
    Rectangle<int> parentStrip() const { return { 360, 110, getWidth() - 376, 62 }; }
    Rectangle<int> card (int i) const
    {
        const int x0 = 360, y0 = 184, w = getWidth() - 376, h = getHeight() - 200;
        const int cw = (w - 3 * 12) / 4, ch = (h - 12) / 2;
        return { x0 + (i % 4) * (cw + 12), y0 + (i / 4) * (ch + 12), cw, ch };
    }
    int cardAt (Point<float> p) const { for (int i = 0; i < (int) proc.melShown.size() && i < 8; ++i) if (card (i).toFloat().contains (p)) return i; return -1; }
    String soundName() const
    {
        if (proc.chopActive()) return "THE SAMPLER";
        if (proc.sampleActive()) if (auto s = proc.activeSample()) return s->name;
        return proc.currentName();
    }
    void drawRoll (Graphics& g, Rectangle<float> r, const kk::mel::Melody& m, bool playing, Colour col)
    {
        const auto& t = kk::theme();
        if (m.notes.empty()) return;
        const bool withChords = proc.melLayers > 0 && ! m.chords.empty() && &m != &proc.melMine;
        int lo = 127, hi = 0; for (auto& n : m.notes) { lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); }
        if (withChords) for (auto& n : m.chords) { lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); }
        lo -= 1; hi += 1;
        const float beats = m.beats(), rows = (float) std::max (6, hi - lo + 1);
        for (int b = 0; b <= m.bars; ++b) { g.setColour (t.text.withAlpha (b % 4 == 0 ? 0.18f : 0.07f)); g.fillRect (r.getX() + r.getWidth() * (float) b * 4.0f / beats, r.getY(), 1.0f, r.getHeight()); }
        const float beatNow = playing ? proc.loopBeat.load() : -1.0f;
        if (withChords)
            for (auto& n : m.chords)
            {
                const float x = r.getX() + r.getWidth() * n.start / beats, w = std::max (2.0f, r.getWidth() * n.len / beats - 1.0f);
                const float y = r.getBottom() - r.getHeight() * (float) (n.pitch - lo + 1) / rows, h = std::max (2.5f, r.getHeight() / rows - 1.0f);
                g.setColour (TC (0xff9b6bff).withAlpha (proc.melLayers == 2 ? 0.85f : 0.45f));
                g.fillRoundedRectangle (x, y, w, h, 1.5f);
            }
        for (auto& n : m.notes)
        {
            const float x = r.getX() + r.getWidth() * n.start / beats, w = std::max (2.0f, r.getWidth() * n.len / beats - 1.0f);
            const float y = r.getBottom() - r.getHeight() * (float) (n.pitch - lo + 1) / rows, h = std::max (2.5f, r.getHeight() / rows - 1.0f);
            const bool lit = beatNow >= n.start && beatNow < n.start + n.len;
            g.setColour (lit ? t.text : col.withAlpha ((withChords && proc.melLayers == 2 ? 0.2f : 0.55f) + 0.4f * n.vel));
            g.fillRoundedRectangle (x, y, w, h, 1.5f);
        }
        if (beatNow >= 0) { g.setColour (t.text.withAlpha (0.8f)); g.fillRect (r.getX() + r.getWidth() * beatNow / beats, r.getY(), 1.5f, r.getHeight()); }
    }
    void drawCard (Graphics& g, Rectangle<float> r, const kk::mel::Melody& m, int idx)
    {
        const auto& t = kk::theme();
        const bool playing = proc.melPlaying == idx, hot = cardAt (r.getCentre()) == hoverCard && hoverCard >= 0;
        g.setColour (t.glass.withMultipliedAlpha (playing ? 1.6f : 1.2f)); g.fillRoundedRectangle (r, 12);
        g.setColour (playing ? t.accent : hot ? t.accent.withAlpha (0.7f) : t.text.withAlpha (0.2f)); g.drawRoundedRectangle (r, 12, playing ? 2.2f : 1.2f);
        g.setColour (t.text); g.setFont (kk::modern::font (14.5f, true, 0.03f));
        g.drawFittedText (m.name, r.reduced (12, 8).removeFromTop (20).toNearestInt(), Justification::centredLeft, 1, 0.8f);
        g.setColour (kk::accentText()); g.setFont (kk::modern::font (10.5f, true, 0.18f));
        g.drawText (m.how, r.reduced (12, 8).removeFromTop (20), Justification::centredRight);
        drawRoll (g, r.reduced (12, 8).withTrimmedTop (24).withTrimmedBottom (20), m, playing, t.accent);
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.1f));
        g.drawText (String (kk::mel::keyName (m.key)) + " " + kk::mel::scaleName (m.scale) + "  .  " + String (m.bars) + " BARS  .  " + (m.genre >= 0 ? String (roundToInt (m.bpm)) + " BPM" : String ((int) m.notes.size()) + " NOTES"),
                    r.reduced (12, 6).removeFromBottom (16), Justification::centredLeft);
        if (hot) { g.setColour (t.dim); g.drawText ("drag = MIDI", r.reduced (12, 6).removeFromBottom (16), Justification::centredRight); }
    }
    void cardMenu (int idx)
    {
        PopupMenu m;
        m.addSectionHeader (proc.mels[(size_t) idx].name);
        m.addItem (1, "Hear it");
        m.addItem (2, "Make it the parent (8 children)");
        m.addItem (3, "Save the MIDI into Documents / KEYS KILLA / Melodies");
        m.showMenuAsync (PopupMenu::Options(), [this, idx, safe = SafePointer<MelodyPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1) proc.melPlay (idx);
            if (r == 2) { proc.melEvolve (idx, false); startAnim(); }
            if (r == 3) { const auto f = proc.melSave (idx); note = f.existsAsFile() ? "saved: " + f.getFileName() : String ("could not save"); repaint(); }
        });
    }
    void shredFile (const File& f)
    {
        AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<AudioFormatReader> rd (fm.createReaderFor (f));
        AudioBuffer<float> peek;
        if (rd != nullptr) { const int n = (int) std::min<juce::int64> (rd->lengthInSamples, (juce::int64) (rd->sampleRate * 20)); peek.setSize (1, jmax (1, n)); rd->read (&peek, 0, n, 0, true, false); }
        const bool ok = proc.melLoadAudioFile (f);
        note = ok ? "shredded: " + String ((int) proc.melMine.notes.size()) + " notes, " + kk::mel::keyName (proc.melMine.key) + " " + kk::mel::scaleName (proc.melMine.scale) + " - drag MIDI, or GENERATE variations"
                  : String ("no clear melody in ") + f.getFileName() + " (one voice / one instrument works best)";
        shredView.start (rd != nullptr ? &peek : nullptr, ok ? proc.melMine.notes : std::vector<kk::mel::Note>(), f.getFileNameWithoutExtension().substring (0, 28));
        refresh();
    }
    void setKey (int k)
    {
        if (proc.melFromMine && proc.melHasMine)   // your melody moves into the new key
        {
            const int d = ((k - proc.melMine.key) % 12 + 12) % 12, shift = d > 6 ? d - 12 : d;
            for (auto& n : proc.melMine.notes) n.pitch = jlimit (24, 108, n.pitch + shift);
            proc.melMine.key = k;
        }
        proc.melKey = k; refresh();
    }
    void setScale (int sc)
    {
        sc = jlimit (0, kk::mel::numScales - 1, sc);
        if (proc.melFromMine && proc.melHasMine && sc != proc.melMine.scale)   // your melody bends into the new scale
        {
            const auto& st = kk::mel::scaleSteps (sc);
            for (auto& n : proc.melMine.notes)
            {
                int best = n.pitch, bd = 99;
                for (int d = -2; d <= 2; ++d) { const int pc = ((n.pitch + d - proc.melMine.key) % 12 + 12) % 12; if (std::find (st.begin(), st.end(), pc) != st.end() && std::abs (d) < bd) { bd = std::abs (d); best = n.pitch + d; } }
                n.pitch = best;
            }
            proc.melMine.scale = sc;
        }
        proc.melScale = sc; refresh();
    }
    void generate()
    {
        if (proc.melFromMine && ! proc.melHasMine) { note = "first your melody: LISTEN and play it in FL, or drop a .mid file"; repaint(); return; }
        proc.melGenerate();
        note = {};
        if (! proc.melShown.empty()) { proc.melPlay (proc.melShown[0]); lastHover = proc.melShown[0]; }
        startAnim();
    }
    void startAnim() { animStart = Time::getMillisecondCounter(); refresh(); }
    void stepSound (int dir)
    {
        proc.alcExc = jmax (0, catBox.getSelectedId() - 1);
        proc.alcSeed = (uint32) ((int) proc.alcSeed + dir);
        const int playing = proc.melPlaying;
        proc.alcUse (proc.alchemy (proc.alcExc.load(), proc.alcBody.load(), proc.alcMatter.load(), proc.alcSize.load(), proc.alcSeed), playing == -1);
        if (playing >= 0) proc.melPlay (playing); else if (playing == -2) proc.melPlayMine();   // keep the melody going on the new sound
        repaint();
    }
    void refresh()
    {
        surpriseTab.selected = ! proc.melFromMine; mineTab.selected = proc.melFromMine; surpriseTab.repaint(); mineTab.repaint();
        for (int k = 0; k < 12; ++k) { keyBtns[(size_t) k]->selected = proc.melKey == k; keyBtns[(size_t) k]->repaint(); }
        scaleBox.setSelectedId (proc.melScale + 1, dontSendNotification);
        for (int i = 0; i < (int) genreBtns.size(); ++i) { genreBtns[(size_t) i]->selected = proc.melGenre == i - 1; genreBtns[(size_t) i]->setEnabled (! proc.melFromMine); genreBtns[(size_t) i]->repaint(); }
        for (int i = 0; i < 3; ++i) { layerBtns[(size_t) i]->selected = proc.melLayers == i; layerBtns[(size_t) i]->repaint(); }
        bpmSl.setValue (proc.melBpm, dontSendNotification);
        static const int bars[] { 4, 8, 16 };
        for (int i = 0; i < 3; ++i) { barBtns[(size_t) i]->selected = proc.melBars == bars[i]; barBtns[(size_t) i]->repaint(); barBtns[(size_t) i]->setEnabled (! proc.melFromMine); }
        for (int i = 0; i < 3; ++i) { rangeBtns[(size_t) i]->selected = std::abs (proc.melStyle.range - (float) i * 0.5f) < 0.1f; rangeBtns[(size_t) i]->repaint(); }
        for (Component* c : { (Component*) &listenBtn, (Component*) &hearMineBtn, (Component*) &dragMine }) c->setVisible (proc.melFromMine);
        hearMineBtn.setEnabled (proc.melHasMine); dragMine.setVisible (proc.melFromMine && proc.melHasMine);
        listenBtn.selected = proc.melListening(); listenBtn.setButtonText (proc.melListening() ? "STOP" : "LISTEN"); listenBtn.repaint();
        generateBtn.setButtonText (proc.melFromMine ? "GENERATE FROM MY MELODY" : "GENERATE 8 MELODIES");
        backBtn.setVisible (proc.melParent >= 0); againBtn.setVisible (! proc.melShown.empty());
        repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        const auto now = Time::getMillisecondCounter();
        if (hoverCard >= 0 && ! heard && now - hoverSince > 220 && hoverCard < (int) proc.melShown.size())
        { heard = true; const int idx = proc.melShown[(size_t) hoverCard]; if (proc.melPlaying != idx) proc.melPlay (idx); lastHover = idx; }
        if (proc.melListening() && proc.melHeardNotes() != lastHeardCount) { lastHeardCount = proc.melHeardNotes(); note = "listening... " + String (lastHeardCount) + " notes - press LISTEN again when your melody has played once"; repaint(); }
        if (proc.melListening() && lastHeardCount <= 0 && proc.melHeardAudio() > 0.01f && ! heardAudioShown) { heardAudioShown = true; note = "listening to the SOUND coming in (EVOLVE as an effect) - press LISTEN again when the melody has played once"; repaint(); }
        if (! proc.melListening()) heardAudioShown = false;
        if (proc.melVer.load() != lastVer) { lastVer = proc.melVer.load(); refresh(); }
        if (proc.loopPlaying() || now - animStart < 900) repaint();
        stopBtn.setEnabled (proc.loopPlaying());
        if ((now / 500) % 2 != (uint32) blink) { blink = (int) ((now / 500) % 2); repaint (curSoundBtn.getBounds().translated (0, -26).withHeight (24)); }
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton surpriseTab { lnf }, mineTab { lnf }, curSoundBtn { lnf }, pickSoundBtn { lnf }, listenBtn { lnf }, hearMineBtn { lnf }, generateBtn { lnf }, backBtn { lnf }, againBtn { lnf }, stopBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> keyBtns, barBtns, rangeBtns, genreBtns, layerBtns;
    ComboBox scaleBox, catBox;
    HotButton shredBtn { lnf };
    ShredView shredView;
    std::unique_ptr<FileChooser> chooser;
    HotButton prevSnd { lnf }, nextSnd { lnf };
    std::map<int, int> catPos;
    Slider busySl, wildSl, bpmSl;
    DragFileButton dragMine { "MIDI", TC (0xff36ff6a) };
    String note;
    int mineY = 600, hoverCard = -1, downCard = -1, lastHover = -1, lastVer = -1, lastHeardCount = -1, blink = 0;
    uint32 hoverSince = 0, animStart = 0;
    bool heard = false, dragged = false, dropHot = false, heardAudioShown = false;
};

#include "MixLabPage.h"
#include "SoundWorldPage.h"
#include "AlchemyPage.h"
#include "LifePage.h"
#include "MatterStrip.h"
#include "FeedPage.h"

//==============================================================================
class MainPage : public Component, private Timer
{
public:
    explicit MainPage (KeysKillaProcessor& p)
        : proc (p), parentA (p, lnf, 0), parentB (p, lnf, 1), breedBtn (lnf), wildRail (p, lnf), dragDaw (p, lnf),
          pitchWheel (lnf), modWheel (lnf), meter (lnf), keyboard (p, lnf)
    {
        settings = openSettings();
        lnf.setSkin (Skin::all()[(size_t) kk::themeIndex()]);   // v0.34: GLASS or NIGHT
        if (auto* c = activeLabCache(); c != nullptr && c->builtFor != kk::themeIndex()) c->rebuild();
        setLookAndFeel (&lnf);

        // ---- header
        prevBtn.onClick = [this] { mutateMatter (-1); }; prevBtn.setTooltip ("The previous mutation of this matter");
        nextBtn.onClick = [this] { mutateMatter (1); };  nextBtn.setTooltip ("A new mutation of this matter (ALCHEMY)");
        saveBtn.onClick = [this] { savePreset(); }; saveBtn.setTooltip ("Save this sound as a user preset.");
        menuBtn.onClick = [this] { showMenu(); };  menuBtn.setTooltip ("Presets, A/B, undo, ADVANCED, size.");
        nameBtn.onClick = [this] { openTab (tabAlchemy); }; nameBtn.setTooltip ("The sound on your keys.  Click = ALCHEMY: make a new one from matter (no presets to browse)");
        heartBtn.onClick = [this] { savePresetAs(); }; heartBtn.setTooltip ("Love it?  Keep this sound (it goes into MY SOUNDS)");
        editBtn.framed = true; editBtn.hero = true; editBtn.setButtonText ("SCULPT");
        editBtn.setTooltip ("SCULPT: the sound on your keys becomes matter in your hands - stretch it, rub it, hold it, tear it.  Every detail: MENU > SOUND EDIT");
        editBtn.onClick = [this]   // v0.44: the sound is shaped with your hands (every detail stays in MENU > SOUND EDIT)
        {
            if (proc.sampleActive() || proc.chopActive()) { openTab (tabEdit); return; }
            if (! (openTabIndex == tabAlchemy && isPanelVisible())) openTab (tabAlchemy);
            if (auto* ip = dynamic_cast<InsetPage*> (module (tabAlchemy))) if (auto* ap = dynamic_cast<AlchemyPage*> (ip->page())) ap->sculptCurrent();
        };
        addAndMakeVisible (editBtn);
        keysPill.framed = true; keysPill.setTooltip ("What the keys play now.  Playing a sample / a child?  Click = back to your sound");
        keysPill.onClick = [this] { if (proc.sampleActive() || proc.chopActive()) { proc.loadPreset (proc.getCurrentProgram()); labChanged(); } };
        addAndMakeVisible (keysPill);
        worldBtn.framed = true; worldBtn.setButtonText ("WORLD"); worldBtn.onClick = [this] { worldMenu(); };
        worldBtn.setTooltip ("SOUND WORLD: one click colours the whole sound (rompler, analog, glassy, hi-fi, organic ...).  TRANCE GATE and CLIPPER are here too.");
        addAndMakeVisible (worldBtn);
        moonBtn.onClick = [this] { setTheme (1 - kk::themeIndex()); };
        moonBtn.setTooltip ("Day / night: GLASS (light milky glass) or NIGHT (dark smoked glass). Window size and eco mode are in MENU.");
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
            g.setColour (TC (0xffece6e6));
            g.setFont (serif (r.getHeight() * 0.55f, false, 0.06f));
            g.drawFittedText (proc.currentName() + (modified ? " *" : ""), r.reduced (8, 0).toNearestInt(), Justification::centred, 1, 0.6f);
        };
        for (Component* c : { (Component*) &prevBtn, (Component*) &nextBtn, (Component*) &saveBtn, (Component*) &menuBtn,
                              (Component*) &nameBtn, (Component*) &heartBtn, (Component*) &moonBtn })
            addAndMakeVisible (c);

        // ---- parents + BREED
        parentA.onClick = [this] { parentMenu (0); };
        parentB.onClick = [this] { parentMenu (1); };
        parentA.onChanged = [this] { labChanged(); };
        parentB.onChanged = [this] { labChanged(); };
        addAndMakeVisible (parentA); addAndMakeVisible (parentB);
        for (int sl = 0; sl < 2; ++sl)
        {
            auto& pv = sl == 0 ? prevA : prevB; auto& nx = sl == 0 ? nextA : nextB; auto& dc = sl == 0 ? diceA : diceB;
            pv.onClick = [this, sl] { randomMatterParent (sl, false); }; pv.setTooltip ("Another mutation of this matter");
            nx.onClick = [this, sl] { randomMatterParent (sl, false); };  nx.setTooltip ("Another mutation of this matter");
            dc.onClick = [this, sl] { randomMatterParent (sl, true); };   dc.setTooltip ("A random matter: a random exciter, body and consistency");
            addAndMakeVisible (pv); addAndMakeVisible (nx); addAndMakeVisible (dc);
        }
        breedBtn.onBreed = [this]
        {
            if (! proc.parentsReady()) { parentMenu (proc.labSlotFilled (0) ? 1 : 0); return; }   // empty slot: choose it first
            if (proc.labAudioMode()) proc.labBreedAudio();   // your own sound in a parent: breed the audio
            else proc.breed();
            labChanged();
            breedBtn.boom();
            Array<Point<float>> to;
            const int cx[6] { 220, 422, 623, 827, 1030, 1233 };
            for (int i = 0; i < 6; ++i) to.add ({ (float) cx[i] + 75.0f, 449.0f });
            sparks.fire ({ 837.0f, 252.0f }, to);
        };
        breedBtn.state = [this]
        {
            ReactorState st;
            for (int k = 0; k < 2; ++k)
                if (proc.labWav[(size_t) k]) st.stream[st.sounds++] = kk::theme().accent;
                else if (const auto& pg = proc.parent (k); pg.valid()) st.stream[st.sounds++] = streamColour (pg.cat);
            st.turbulence = lastMutate >= 0 ? (float) (lastMutate + 1) / 5.0f : 0.0f;
            return st;
        };
        addAndMakeVisible (breedBtn);

        // ---- children, genes, mutate
        for (int i = 0; i < 6; ++i)
        {
            auto c = std::make_unique<ChildCard> (proc, lnf, i);
            c->onMenu = [this] (int idx) { childMenu (idx); };
            c->onSave = [this] (int idx, Component* from) { saveChildMenu (idx, from); };
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
        addChildComponent (soundModeBtn); addChildComponent (loopModeBtn);   // v0.40: the children are sounds - melodies have their own page
        proc.mainLoopMode = false;
        treeBtn.onClick = [this] { openTab (tabTree); }; treeBtn.setTooltip ("FAMILY TREE: breed up to 4 sounds into new sounds or melody loops");
        labSwitch.onSwitch = [this] (int k)
        {
            if (k == 8) { showEvolve (true); return; }
            if (k == 9) { if (! (openTabIndex == tabMelody && isPanelVisible())) openTab (tabMelody); return; }
            if (k == 10) { if (! (openTabIndex == tabMix && isPanelVisible())) openTab (tabMix); return; }
            if (k == 11) { if (! (openTabIndex == tabWorld && isPanelVisible())) openTab (tabWorld); return; }
            if (k == 12) { if (! (openTabIndex == tabAlchemy && isPanelVisible())) openTab (tabAlchemy); return; }
            if (k == 13) { if (! (openTabIndex == tabLife && isPanelVisible())) openTab (tabLife); return; }
            if (k == 14) { if (! (openTabIndex == tabFeed && isPanelVisible())) openTab (tabFeed); return; }
            if (k == 0) { hidePanels(); openTabIndex = -1; updateTabs(); return; }
            const int target[] { 0, tabTree, tabPair, tabVst, tabSounds, tabSampler, tab808 + lastDrum, tabFxRack };
            if (! (openTabIndex == target[k] && isPanelVisible())) openTab (target[k]);
        };
        labSwitch.isOn = [this] (int k) { return k == 7 && proc.rack.anyOn(); };
        labSwitch.onDropToPair = [this] (const File& f)   // v0.35: a sound dropped on the tiles = a parent in BREED LAB
        {
            int slot = ! proc.labSlotFilled (0) ? 0 : ! proc.labSlotFilled (1) ? 1 : (dropRound++) % 2;
            if (! proc.labDropFile (slot, f)) return 0;
            if (isPanelVisible()) { hidePanels(); openTabIndex = -1; updateTabs(); }
            labChanged();
            return slot + 1;
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
            tabs.back()->setVisible (false);   // v0.35: no drums in BREED LAB (they go into their own plugin)
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
        // v0.44: no knobs - the eight macros are one landscape you paint (the knobs stay hidden, their attachments keep the host in sync)
        for (auto& k : macros) k->setVisible (false);
        for (auto& c : captions) c->setVisible (false);
        matterStrip = std::make_unique<MatterStrip> (proc, std::vector<const char*> { macroIds, macroIds + 8 });
        addAndMakeVisible (*matterStrip);

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

        // v0.36 EVOLVE: the first thing you see (STUDIO = everything else)
        evolve = std::make_unique<EvolvePage> (proc, lnf);
        evolve->onStudio = [this] { showEvolve (false); };
        evolve->onTheme = [this] { setTheme (1 - kk::themeIndex()); };
        evolve->onPickSeed = [this]
        {
            showEvolve (false);
            openAlchemy ("THE SEED OF A NEW TREE", [this] { proc.evoSeedCurrent(); showEvolve (true); if (evolve) evolve->repaint(); });
        };
        evolve->onSavePreset = [this] (int node)
        {
            if (! isPositiveAndBelow (node, (int) proc.evo.size()) || proc.evo[(size_t) node].isAudio()) return;
            auto g = proc.evo[(size_t) node].g; g.name = proc.evo[(size_t) node].name;
            proc.applyGenomePublic (g);
            savePresetAs();
        };
        evolve->onChanged = [this] { refreshState(); };
        addChildComponent (*evolve);

        setSize (KeysKillaEditor::designW, KeysKillaEditor::designH);
        noFocus (*this);
        applyKeyMode();
        showEvolve (settings->getBoolValue ("evolveOpen", true));
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

    int preferredScale() const { return jlimit (50, 100, settings->getIntValue ("labScale18", 85)); }

    // ---------------- SPACE = play / stop in the plugin (while the plugin window has focus), other keys go to the host ----------------
    bool keysToPlugin() const { return true; }   // v0.35: the PC keys and SPACE always work in the open page
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
        if (k == KeyPress::deleteKey || k == KeyPress::backspaceKey)   // v0.35: Del works on the open page (MY SOUNDS, library ...)
        {
            std::function<bool (Component&)> fwd = [&fwd, &k] (Component& c)
            {
                if (! c.isVisible()) return false;
                for (auto* ch : c.getChildren()) if (fwd (*ch)) return true;
                return dynamic_cast<MainPage*> (&c) == nullptr && c.keyPressed (k);
            };
            for (auto* p : panels()) if (p != nullptr && p->isVisible() && fwd (*p)) return true;
            if (isPanelVisible() && openTabIndex >= 0) if (auto* m = module (openTabIndex)) if (m->isVisible() && fwd (*m)) return true;
            return true;
        }
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
        showEvolve ((v >= 32 && v <= 35) || v == 40);
        if (v == 32 || v == 33 || v == 35 || v == 40)   // EVOLVE: a seed, a few generations, the kids (32 hover, 33 blend, 35 night look = same)
        {
            proc.evoSeedPreset (0);
            proc.evoFocus (proc.evo[0].kids[2]);
            proc.evoFocus (proc.evo[(size_t) proc.evoCenter].kids[4]);
            for (int i = 0; i < 30; ++i) proc.renderNextThumbnail();
            if (v == 32) { proc.evoPocketAdd (proc.evo[(size_t) proc.evoCenter].kids[1]); proc.evoPocketAdd (proc.evo[(size_t) proc.evoCenter].kids[3]); evolve->showDebugHover (1); }
            if (v == 40) { for (int k = 0; k < 4; ++k) proc.evoFocus (proc.evo[(size_t) proc.evoCenter].kids[(size_t) (k % 6)]); evolve->debugMap(); }
            if (v == 33) evolve->showDebugMorph (0, 0.55f);
        }
        if (v == 34) proc.evoReset();
        if ((v >= 32 && v <= 35) || v == 40) evolve->refreshLayout();
        if (v >= 1 && v <= 8) { ensureAdvanced(); advanced->showTab (v - 1); advanced->setVisible (true); advanced->toFront (false); }
        if (v == 9) openTab (tabBrowser);
        if (v == 10) openTab (tabParams);
        if (v == 11) openTab (tabFxRack);
        if (v == 15) openTab (tab808);
        if (v == 16) openTab (tabHat);
        if (v == 17) openTab (tabFxRack);
        if (v == 18) openTab (tabSampler);
        if (v == 19) openTab (tabSnare);
        if (v == 20) openTab (tabSampler);
        if (v == 22) openTab (tabVst);
        if (v == 24) openTab (tabSounds);
        if (v == 25) openTab (tabKick);
        if (v == 26) openTab (tabEdit);
        if (v == 23) openTab (tabSampler);
        if (v == 21)
        {
            if (auto f = File::getSpecialLocation (File::tempDirectory).getChildFile ("kk_harvest_demo.wav"); f.existsAsFile())
            { proc.harvestFile (f); while (proc.harvesting()) Thread::sleep (20); proc.moduleHousekeeping(); }
            proc.pairDice (0); proc.pairDice (1); proc.pairDice (2); proc.pairBreed(); proc.selectPairKid (1, false); openTab (tabPair);
        }
        if (v == 12) { proc.breed(); while (proc.renderNextThumbnail()) {} proc.selectChild (2); labChanged(); }
        if (v == 27) { proc.setParentPreset (0, 3); while (proc.renderNextThumbnail()) {} labChanged(); breedBtn.prime(); }
        if (v == 12) breedBtn.prime();
        if (v == 55) { openTab (tabFeed); if (auto* ip = dynamic_cast<InsetPage*> (module (tabFeed))) if (auto* fp = dynamic_cast<FeedPage*> (ip->page())) fp->debugSwipes(); }
        if (v == 56) { openTab (tabAlchemy); if (auto* ip = dynamic_cast<InsetPage*> (module (tabAlchemy))) if (auto* ap = dynamic_cast<AlchemyPage*> (ip->page())) ap->debugSculpt(); }
        if (v >= 50 && v <= 54)   // v0.43 ALCHEMY (50, 54 = as the chooser), LIFE (51 gravity, 52 predator, 53 swarm)
        {
            if (v == 50 || v == 54)
            {
                openTab (tabAlchemy);
                if (auto* ip = dynamic_cast<InsetPage*> (module (tabAlchemy))) if (auto* ap = dynamic_cast<AlchemyPage*> (ip->page())) ap->debugSet (0, 3, v == 54 ? 0.8f : 0.25f, 0.6f);
                if (v == 54) openAlchemy ("PARENT A", [] {});
            }
            else
            {
                openTab (tabLife);
                if (auto* ip = dynamic_cast<InsetPage*> (module (tabLife))) if (auto* lp = dynamic_cast<LifePage*> (ip->page())) lp->debugMode (v - 51);
            }
        }
        if (v == 41 || v == 42)   // v0.40 MELODY: 8 melodies (42: from your melody)
        {
            openTab (tabMelody);
            if (auto* ip = dynamic_cast<InsetPage*> (module (tabMelody))) if (auto* mp = dynamic_cast<MelodyPage*> (ip->page()))
            {
                if (v == 42)
                {
                    std::vector<kk::mel::Note> mine;
                    const int pitches[] { 69, 72, 76, 74, 72, 71, 69, 64, 69, 72, 76, 79, 77, 76, 74, 72 };
                    for (int i = 0; i < 16; ++i) mine.push_back ({ (float) i * 0.5f, 0.45f, pitches[i], 0.8f });
                    proc.melMine = kk::mel::fromNotes (mine, 0); proc.melHasMine = true; proc.melFromMine = true; proc.melKey = proc.melMine.key; proc.melScale = proc.melMine.scale;
                }
                mp->debugGenerate();
                proc.melPlay (-1);
            }
        }
        if (v == 48 || v == 49)   // v0.42 SOUND WORLD: a kept dot (49: two regions connected)
        {
            if (proc.getSampleRate() <= 0) proc.prepareToPlay (44100, 512);
            const auto& ds = kk::world::dots();
            int dot = 0; for (int i = 0; i < (int) ds.size(); ++i) if (ds[(size_t) i].region == 3) { dot = i; break; }
            proc.worldDot = dot; proc.worldRegion = 3; proc.worldRegionB = v == 49 ? 9 : -1;
            proc.worldPlay (v == 49 ? proc.worldConnect (3, 9, 7) : proc.worldSound (dot), false);
            openTab (tabWorld);
        }
        if (v >= 43 && v <= 46)   // v0.41 MIX LAB: EQ / COMP / TIME MACHINE / SPACE with a melody playing through it
        {
            if (proc.getSampleRate() <= 0) proc.prepareToPlay (44100, 512);
            proc.melSetGenre (kk::mel::gTrap); proc.melGenerate(); proc.melPlay (proc.melShown[0]);
            proc.mixLab.band[2].on = true; proc.mixLab.band[2].gain = -4.0f; proc.mixLab.band[2].freq = 320.0f;
            proc.mixLab.band[5].on = true; proc.mixLab.band[5].gain = -3.0f; proc.mixLab.band[5].dyn = 0.6f; proc.mixLab.band[5].freq = 3600.0f; proc.mixLab.band[5].q = 2.0f;
            proc.mixLab.band[6].on = true; proc.mixLab.band[6].gain = 2.5f;
            proc.mixLab.compOn = true; proc.mixLab.thresh = -24.0f;
            if (v == 45) { kk::applyEra (proc.mixLab, 0.25f); proc.mixLab.tmOn = true; }
            if (v == 46) { proc.mixLab.spOn = true; proc.mixLab.spMode = kk::spCloud; proc.mixLab.dlOn = true; proc.mixLab.dlMode = kk::dlPingPong; }
            openTab (tabMix);
            if (auto* ip = dynamic_cast<InsetPage*> (module (tabMix))) if (auto* mp = dynamic_cast<MixLabPage*> (ip->page()))
            {
                AudioBuffer<float> b (2, 512); MidiBuffer mb;
                mp->debugShow (v == 46 ? 3 : v - 43);
                for (int i = 0; i < 400; ++i) { proc.processBlock (b, mb); if (i % 6 == 0) mp->debugTick(); }
            }
            proc.melPlay (-1);
        }
        if (v == 37) { FxRackPage::lastMode() = 0; openTab (tabFxRack); }   // v0.37 SURPRISE FX
        if (v == 38) { FxRackPage::lastMode() = 1; proc.stepPreset (4); proc.stepOn = true; openTab (tabFxRack); }   // STEP FX
        if (v == 31 || v == 36 || v == 39 || v == 47)   // SAMPLER with a sample, zoomed in, a part selected (36: FLIPS, 39: EDIT of a sample)
        {
            auto f = File::getSpecialLocation (File::tempDirectory).getChildFile ("kk_shot_sample.wav");
            AudioBuffer<float> b (2, 44100 * 4);
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const int beat = i % 11025;
                const float x = (beat < 3000 ? std::sin ((float) beat * 0.02f) * std::exp (-(float) beat / 900.0f) : 0.0f) + 0.2f * std::sin ((float) i * 0.03f) * (0.5f + 0.5f * std::sin ((float) i * 0.0002f));
                b.setSample (0, i, x); b.setSample (1, i, x);
            }
            f.deleteFile();
            { WavAudioFormat wav; std::unique_ptr<AudioFormatWriter> w (wav.createWriterFor (new FileOutputStream (f), 44100, 2, 16, {}, 0)); if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples()); }
            proc.chopLoadFile (f);
            openTab (tabSampler);
            std::function<ChopPanel* (Component*)> find = [&find] (Component* c) -> ChopPanel*
            { if (auto* cp = dynamic_cast<ChopPanel*> (c)) return cp; for (auto* ch : c->getChildren()) if (auto* r = find (ch)) return r; return nullptr; };
            if (auto* m = module (tabSampler)) if (auto* cp = find (m)) { cp->debugSelect (44100 / 2, 44100 * 2, 0, 44100 * 3); if (v == 36) cp->debugFlips(); if (v == 47) { if (proc.getSampleRate() <= 0) proc.prepareToPlay (44100, 512); cp->debugMelody(); } }
            if (v == 39) { proc.useSample (kk::PairLab::fromFile (f, 44100.0), false); proc.sampleEdit[KeysKillaProcessor::seStart] = 0.12f; proc.sampleEdit[KeysKillaProcessor::seSpace] = 0.4f; openTab (tabEdit); }
        }
        if (v == 30) { openTab (0); setTheme (1 - kk::themeIndex()); }          // ... and with a drum page open
        if (v == 29) { openTab (tabEdit); setTheme (1 - kk::themeIndex()); }   // switch the skin with SOUND EDIT open
        if (v == 28) { proc.breed(); while (proc.renderNextThumbnail()) {} labChanged(); breedBtn.onBreed(); breedBtn.prime (0.32f, 0.8f); sparks.freezeAt (0.22f); }   // fresh instance: one parent chosen, one empty
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
        // drum row (v0.17: drawn over the old tab row)
        // v0.35: the old drum row is a quiet 1-2-3 guide - the step you are on lights up
        {
            const auto bar = R (46, 613, 1632, 661).toFloat();
            kk::modern::plate (g, bar, 10.0f);
            const auto& th = kk::theme();
            const bool haveKids = proc.labAudioMode() ? ! proc.pairKids.empty() : ! proc.kids().empty();
            const int step = ! proc.parentsReady() ? 0 : ! haveKids ? 1 : 2;
            static const char* steps[] { "CHOOSE 2 SOUNDS  -  click a parent, or drop your own WAV", "PRESS  BREED", "PLAY THE CHILDREN  -  SAVE them  /  DRAG them into FL" };
            const float w = bar.getWidth() / 3.0f;
            for (int i = 0; i < 3; ++i)
            {
                auto cell = Rectangle<float> (bar.getX() + w * (float) i, bar.getY(), w, bar.getHeight()).reduced (10, 8);
                const bool on = i == step;
                const auto num = cell.removeFromLeft (cell.getHeight()).reduced (2);
                g.setColour (on ? th.accent : th.dim.withAlpha (0.5f)); g.drawEllipse (num, 1.4f);
                if (on) { g.setColour (th.accent.withAlpha (0.15f)); g.fillEllipse (num); }
                g.setFont (kk::modern::font (14.0f, true, 0.1f)); g.drawText (String (i + 1), num, Justification::centred);
                g.setColour (on ? th.text : th.dim.withAlpha (0.7f)); g.setFont (kk::modern::font (12.5f, on, 0.12f));
                g.drawFittedText (steps[i], cell.reduced (10, 0).toNearestInt(), Justification::centredLeft, 1, 0.75f);
            }
        }
        // parent arrows only when the slot holds a sound
        for (int k = 0; k < 2; ++k)
            if (proc.parent (k).valid())
            {
                kk::modern::chevron (g, k == 0 ? Rectangle<float> (228, 228, 38, 46) : Rectangle<float> (1088, 228, 38, 46), true);
                kk::modern::chevron (g, k == 0 ? Rectangle<float> (545, 228, 38, 46) : Rectangle<float> (1380, 228, 38, 46), false);
            }
    }
    void setTheme (int t)
    {
        kk::themeIndex() = jlimit (0, 1, t); ++kk::themeVersion();
        settings->setValue ("theme", kk::themeIndex());
        lnf.setSkin (Skin::all()[(size_t) kk::themeIndex()]);
        if (auto* c = activeLabCache()) c->rebuild();
        // pages built on demand keep colours from when they were made: build them again in the new theme,
        // and reopen the page that was open (SOUND EDIT, drum pages, library, tree ...)
        const int open = isPanelVisible() ? openTabIndex : -1;
        hidePanels(); openTabIndex = -1;
        soundEdit.reset(); sampleEdit.reset(); advanced.reset(); browser.reset(); treePanel.reset();
        for (auto& m : modules) m.reset();
        for (int i = 0; i < (int) tabs.size() && i < numTabs; ++i) tabs[(size_t) i]->tint = drumTheme (i).accent;
        if (open >= 0) openTab (open);
        if (auto* ed = findParentComponentOfClass<KeysKillaEditor>()) ed->themeChanged();
        sendLookAndFeelChange();
        std::function<void (Component&)> all = [&all] (Component& c) { c.repaint(); for (auto* ch : c.getChildren()) all (*ch); };
        all (*this);
    }

    void resized() override
    {
        // v0.34: centred in the preset pill (pill = y 30..82)
        prevBtn.setBounds (R (626, 35, 670, 77)); nameBtn.setBounds (R (688, 37, 1040, 75)); nextBtn.setBounds (R (1042, 35, 1086, 77));
        heartBtn.setBounds (R (1088, 35, 1128, 77)); saveBtn.setBounds (R (1245, 42, 1346, 85)); menuBtn.setBounds (R (1365, 42, 1466, 85));
        moonBtn.setBounds (R (1483, 40, 1544, 87));
        worldBtn.setBounds (R (1132, 46, 1240, 84));

        parentA.setBounds (R (268, 150, 545, 372)); parentB.setBounds (R (1128, 150, 1372, 372));
        prevA.setBounds (R (228, 228, 266, 274)); nextA.setBounds (R (545, 228, 583, 274)); diceA.setBounds (R (580, 144, 623, 186));
        prevB.setBounds (R (1088, 228, 1126, 274)); nextB.setBounds (R (1380, 228, 1418, 274)); diceB.setBounds (R (1373, 144, 1416, 188));
        breedBtn.setBounds (R (737, 152, 937, 352));
        if (sparks.getParentComponent() == nullptr) addAndMakeVisible (sparks);
        sparks.setBounds (getLocalBounds()); sparks.toFront (false);

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
        if (matterStrip) matterStrip->setBounds (R (86, 676, 1266, 792));
        pitchWheel.setBounds (R (70, 808, 110, 894)); modWheel.setBounds (R (124, 808, 164, 894));
        keyboard.setBounds (R (200, 815, 1640, 923));
        keysPill.setBounds (R (1180, 921, 1640, 941));
        updateKeysPill();
        keyboard.setKeyWidth (1440.0f / 40.0f);

        const auto panelArea = R (10, 8, 1662, 612);
        if (advanced) advanced->setBounds (R (150, 8, 1662, 612));   // the left switch stays visible
        if (browser) browser->setBounds (R (150, 8, 1662, 612));   // the left tiles never cover the preset names
        if (treePanel) treePanel->setBounds (R (150, 96, 1662, 612));
        if (soundEdit) soundEdit->setBounds (R (10, 8, 1662, 806));   // SOUND EDIT: everything above the keyboard, the tiles stay
        if (sampleEdit) sampleEdit->setBounds (R (10, 8, 1662, 806));
        editBtn.setBounds (R (1556, 40, 1660, 87));
        for (int i = 0; i < numPages; ++i)
            if (modules[(size_t) i]) modules[(size_t) i]->setBounds (i == tabPair || i == tabVst ? R (150, 96, 1662, 612)
                                                                 : i == tabSampler || i == tabFxRack || i == tabSounds || i == tabMelody || i == tabMix || i == tabWorld || i == tabAlchemy || i == tabLife || i == tabFeed ? R (10, 8, 1662, 806) : R (0, 0, 1672, 941));   // drum pages get the whole window; SAMPLER / FX RACK keep the keys
        labSwitch.setBounds (R (16, 98, 138, 606));
        if (evolve != nullptr) evolve->setBounds (R (0, 0, 1672, 806));
    }

private:
    enum { tab808, tabSnare, tabHat, tabKick, tabOpenHat, tabPerc, tabDrumFx, numTabs, tabSampler, tabFxRack, tabPair, tabVst, tabSounds, tabMelody, tabMix, tabWorld, tabAlchemy, tabLife, tabFeed, numPages, tabBrowser = 99, tabSettings = 100, tabTree = 101, tabParams = 102, tabEdit = 103 };
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
                    pg->onFactory = [this] { openTab (tabAlchemy); };
                    pg->onPair = [this] { MessageManager::callAsync ([safe = Component::SafePointer<MainPage> (this)] { if (safe != nullptr) { safe->hidePanels(); safe->openTabIndex = -1; safe->updateTabs(); safe->labChanged(); } }); };
                    m = std::make_unique<InsetPage> (std::move (pg)); break;   // v0.33: MY SOUNDS gets the whole page
                }
                case tabSampler:  m = std::make_unique<InsetPage> (std::make_unique<ChopPanel> (proc, lnf)); break;
                case tabMelody:
                {
                    auto pg = std::make_unique<MelodyPage> (proc, lnf);
                    pg->onPickSound = [this] { openAlchemy ("THE SOUND FOR THE MELODIES", [this] { openTab (tabMelody); }); };
                    m = std::make_unique<InsetPage> (std::move (pg)); break;
                }
                case tabMix:      m = std::make_unique<InsetPage> (std::make_unique<MixLabPage> (proc, lnf)); break;   // v0.41
                case tabWorld:    m = std::make_unique<InsetPage> (std::make_unique<SoundWorldPage> (proc, lnf)); break;   // v0.42
                case tabAlchemy:  m = std::make_unique<InsetPage> (std::make_unique<AlchemyPage> (proc, lnf)); break;     // v0.43
                case tabLife:     m = std::make_unique<InsetPage> (std::make_unique<LifePage> (proc, lnf)); break;        // v0.43
                case tabFeed:     m = std::make_unique<InsetPage> (std::make_unique<FeedPage> (proc, lnf)); break;        // v0.44
                default:          m = std::make_unique<InsetPage> (std::make_unique<FxRackPage> (proc, lnf)); break;
            }
            addChildComponent (*m); noFocus (*m); resized();
        }
        return m.get();
    }
    std::vector<Component*> panels() const
    {
        std::vector<Component*> v { advanced.get(), browser.get(), treePanel.get(), soundEdit.get(), sampleEdit.get() };
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
        if (t == tabBrowser) t = tabAlchemy;   // v0.43: no preset browser any more
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
                case tabEdit:
                    if (proc.sampleActive() || proc.chopActive())   // v0.37: a sample plays - EDIT edits that sample
                    {
                        if (! sampleEdit)
                        {
                            auto pg = std::make_unique<SampleEditPage> (proc, lnf);
                            pg->onClose = [this] { MessageManager::callAsync ([safe = Component::SafePointer<MainPage> (this)] { if (safe != nullptr && safe->openTabIndex == tabEdit) safe->openTab (tabEdit); }); };
                            pg->onEvolve = [this] { MessageManager::callAsync ([safe = Component::SafePointer<MainPage> (this)] { if (safe != nullptr) safe->showEvolve (true); }); };
                            sampleEdit = std::make_unique<InsetPage> (std::move (pg));
                            addChildComponent (*sampleEdit); noFocus (*sampleEdit); resized();
                        }
                        sampleEdit->setVisible (true); break;
                    }
                    if (! soundEdit)
                    {
                        soundEdit = std::make_unique<kkedit::SoundEditPage> (proc, lnf);
                        soundEdit->onClose = [this] { MessageManager::callAsync ([safe = Component::SafePointer<MainPage> (this)] { if (safe != nullptr && safe->openTabIndex == tabEdit) safe->openTab (tabEdit); }); };
                        soundEdit->onSave = [this] { savePreset(); };
                        soundEdit->onMatrix = [this] { MessageManager::callAsync ([safe = Component::SafePointer<MainPage> (this)] { if (safe != nullptr) { safe->openTab (tabParams); safe->advanced->showTab (3); } }); };
                        addChildComponent (*soundEdit); noFocus (*soundEdit); resized();
                    }
                    soundEdit->setVisible (true); break;
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
        const int sw = openTabIndex < 0 ? 0 : treeOn ? 1 : openTabIndex == tabPair ? 2 : openTabIndex == tabVst ? 3 : openTabIndex == tabSounds ? 4 : openTabIndex == tabSampler ? 5
                     : (openTabIndex >= tab808 && openTabIndex < numTabs) ? 6 : openTabIndex == tabFxRack ? 7 : openTabIndex == tabMelody ? 9 : openTabIndex == tabMix ? 10 : openTabIndex == tabWorld ? 11 : openTabIndex == tabAlchemy ? 12 : openTabIndex == tabLife ? 13 : openTabIndex == tabFeed ? 14 : -1;
        if (openTabIndex >= tab808 && openTabIndex < numTabs) lastDrum = openTabIndex - tab808;
        else proc.kitPlay = false;   // PLAY KIT is a preview on the drum pages
        if (labSwitch.sel != sw) { labSwitch.sel = sw; labSwitch.repaint(); }
        if (proc.loopPlaying())   // a loop belongs to the page that started it (FAMILY TREE / PAIR / BREED LAB)
        {
            const int owner = proc.loopOwnerId();
            const bool keep = (owner == 1 && treeOn) || (owner == 2 && (openTabIndex == tabPair || openTabIndex == tabVst))
                           || (owner == 3 && openTabIndex < 0) || (owner != 1 && owner != 2 && owner != 3 && openTabIndex < 0)
                           || openTabIndex == tabEdit    // SOUND EDIT: keep the loop running while you tweak the sound
                           || openTabIndex == tabMix     // v0.41 MIX LAB: you mix what plays
                           || (owner == 4 && (openTabIndex == tabMelody || openTabIndex == tabLife || openTabIndex == tabAlchemy || openTabIndex == tabFeed));
            if (! keep) proc.stopLoop();
        }
        for (int i = 0; i < numTabs; ++i) { tabs[(size_t) i]->selected = i == openTabIndex; tabs[(size_t) i]->repaint(); }
    }
    void setScale (int pct)
    {
        settings->setValue ("labScale18", pct);
        if (auto* ed = findParentComponentOfClass<KeysKillaEditor>())
        {
            ed->setScalePct (jmin (pct, ed->fitScale()));
        }
    }

    // ---------------- BREED LAB ----------------
    void labChanged() { lastLab = -1; refreshState(); }
    // ---------------- v0.43 ALCHEMY instead of the preset browser ----------------
    void openAlchemy (const String& title, std::function<void()> then)
    {
        if (! (openTabIndex == tabAlchemy && isPanelVisible())) openTab (tabAlchemy);
        if (auto* ip = dynamic_cast<InsetPage*> (module (tabAlchemy))) if (auto* ap = dynamic_cast<AlchemyPage*> (ip->page())) ap->pick (title, std::move (then));
    }
    KeysKillaProcessor::Genome randomMatter()
    {
        auto& r = Random::getSystemRandom();
        return proc.alchemy (r.nextInt ((int) kk::alc::exciters().size() - 1), r.nextInt ((int) kk::alc::bodies().size()), r.nextFloat(), 0.2f + 0.6f * r.nextFloat(), (uint32) r.nextInt() | 1u);
    }
    void randomMatterParent (int slot, bool anyMatter)
    {
        auto g = anyMatter ? randomMatter() : proc.alchemy (proc.alcExc.load(), proc.alcBody.load(), proc.alcMatter.load(), proc.alcSize.load(), (uint32) Random::getSystemRandom().nextInt() | 1u);
        proc.setParentGenome (slot, g); labChanged();
    }
    void mutateMatter (int dir)
    {
        proc.alcSeed = (uint32) ((int) proc.alcSeed + dir);
        proc.alcUse (proc.alchemy (proc.alcExc.load(), proc.alcBody.load(), proc.alcMatter.load(), proc.alcSize.load(), proc.alcSeed), true);
        proc.captureUndo(); refreshState();
    }
    void parentMenu (int slot) { soundMenu (slot, false); }
    void soundMenu (int slot, bool ancestor)
    {
        PopupMenu m, cats;
        m.addSectionHeader (ancestor ? "SOUND " + String (slot + 1) : String (slot == 0 ? "PARENT A" : "PARENT B"));
        m.addItem (1, "Make it in ALCHEMY...");
        m.addItem (2, "Use the sound on the keys");
        m.addItem (3, "Random matter");
        if (ancestor)
        {
            m.addItem (4, "Use PARENT A", proc.parent (0).valid());
            m.addItem (5, "Use PARENT B", proc.parent (1).valid());
            m.addItem (6, "Empty");
        }
        else
        {
            m.addItem (7, "Your own sound (WAV, MP3 ...)...");
            m.addItem (6, "Empty", proc.labSlotFilled (slot));
        }
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
                openAlchemy (ancestor ? "SOUND " + String (slot + 1) + " OF THE FAMILY TREE" : String (slot == 0 ? "PARENT A" : "PARENT B"),
                             [this, setPreset, ancestor] { setPreset (-1); labChanged(); if (ancestor) openTab (tabTree); else { hidePanels(); openTabIndex = -1; updateTabs(); } });
                return;
            }
            if (r == 2) setPreset (-1);
            else if (r == 3) { auto g = randomMatter(); if (ancestor) proc.setAncestorGenome (slot, g); else proc.setParentGenome (slot, g); }
            else if (r == 4 || r == 5) proc.setAncestorGenome (slot, proc.parent (r - 4));
            else if (r == 6) { if (ancestor) proc.clearAncestor (slot); else proc.clearParent (slot); }
            else if (r == 7)
            {
                chooser = std::make_unique<FileChooser> ("Your sound as PARENT " + String (slot == 0 ? "A" : "B"), File::getSpecialLocation (File::userDocumentsDirectory), "*.wav;*.aif;*.aiff;*.flac;*.mp3;*.ogg");
                chooser->launchAsync (FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles, [this, slot, safe = SafePointer<MainPage> (this)] (const FileChooser& fc)
                { if (safe != nullptr && fc.getResult() != File() && proc.labDropFile (slot, fc.getResult())) labChanged(); });
                return;
            }
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
        m.addItem (6, "EDIT this sound  (SOUND EDIT)");
        m.showMenuAsync (PopupMenu::Options(), [this, k, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0 || k >= (int) proc.treeKids().size()) return;
            const auto g = proc.treeKids()[(size_t) k].g;
            if (r == 1) proc.playTreeResult (k);
            if (r == 2) { proc.newMelody (k); if (! proc.loopIsTree (k)) proc.playTreeResult (k); }
            if (r == 3 || r == 4) proc.setParentGenome (r - 3, g);
            if (r >= 10) proc.setAncestorGenome (r - 10, g);
            if (r == 5) { proc.selectTreeResult (k); savePresetAs(); }
            if (r == 6) { proc.selectTreeResult (k); openTab (tabEdit); }
            labChanged();
            if (treePanel) treePanel->refresh();
        });
    }
    // v0.35: SAVE on every child - into your folders / sound kits (as a sound) or as a preset (bank children)
    void saveChildMenu (int idx, Component* from)
    {
        if (proc.labAudioMode())
        {
            if (! isPositiveAndBelow (idx, (int) proc.pairKids.size())) return;
            saveToFolderMenu (proc, { proc.withEdits (proc.pairKids[(size_t) idx]) }, from, [safe = SafePointer<MainPage> (this)] (String) { if (safe != nullptr) safe->refreshState(); });
            return;
        }
        if (! isPositiveAndBelow (idx, (int) proc.kids().size())) return;
        PopupMenu m;
        m.addSectionHeader ("SAVE " + proc.kids()[(size_t) idx].g.name);
        m.addItem (1, "As a PRESET (plays and edits like every sound in the bank)...");
        m.addItem (2, "As a SOUND into a folder / sound kit (WAV)...");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (from), [this, idx, from, safe = SafePointer<MainPage> (this)] (int r)
        {
            if (safe == nullptr || r == 0) return;
            if (r == 1) { proc.selectChild (idx); savePresetAs(); }
            if (r == 2) if (auto snd = proc.childAsSound (idx)) saveToFolderMenu (proc, { snd }, from, [] (String) {});
        });
    }
    void childMenu (int idx)
    {
        if (proc.labAudioMode()) { saveChildMenu (idx, nullptr); return; }
        PopupMenu m;
        m.addSectionHeader (proc.kids()[(size_t) idx].g.name);
        m.addItem (1, "Use as PARENT A  (next generation)");
        m.addItem (2, "Use as PARENT B  (next generation)");
        m.addItem (5, "EDIT this sound  (SOUND EDIT)");
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
            if (r == 5) { proc.selectChild (idx); if (! (openTabIndex == tabEdit && isPanelVisible())) openTab (tabEdit); }
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

    StringArray favourites() const
    {
        // v0.34: favourites saved under the old (genre) names follow their sound to its new name
        auto f = StringArray::fromTokens (settings->getValue ("favourites"), "|", "");
        bool changed = false;
        for (auto& n : f) if (const auto nn = currentFactoryName (n); nn != n) { n = nn; changed = true; }
        if (changed) { f.removeDuplicates (false); settings->setValue ("favourites", f.joinIntoString ("|")); }
        return f;
    }
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
        m.addItem (7, "ALCHEMY: make a new sound...");
        packs.addItem (22, "Show my sound folder");
        m.addSubMenu ("Files", packs);
        m.addSectionHeader ("EDIT");
        m.addItem (8, "Undo", proc.canUndo());
        m.addItem (9, "Redo", proc.canRedo());
        m.addItem (12, String ("Switch to ") + (proc.currentAB() == 0 ? "B" : "A") + "  (now " + (proc.currentAB() == 0 ? "A" : "B") + ")");
        m.addItem (13, String ("Copy ") + (proc.currentAB() == 0 ? "A > B" : "B > A"));
        m.addSeparator();
        m.addItem (24, "Settings: window size, eco mode...");
        m.addItem (25, kk::themeIndex() == 0 ? "Switch to NIGHT (dark glass)" : "Switch to GLASS (day)");
        if (auto* ed = findParentComponentOfClass<KeysKillaEditor>()) m.addItem (26, "Metal case around the plugin", true, ed->frame() > 0);
        m.addItem (27, "SOUND EDIT: every detail of the sound...");
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
                case 27: openTab (tabEdit); break;
                case 24: openTab (tabSettings); break;
                case 25: setTheme (1 - kk::themeIndex()); break;
                case 26: if (auto* ed = findParentComponentOfClass<KeysKillaEditor>()) ed->setFrame (ed->frame() == 0); break;
                case 16: proc.eco = ! proc.eco.load(); break;
                case 18: proc.panic(); break;
                case 19: settings->setValue ("keysToPlugin", ! keysToPlugin()); applyKeyMode(); break;
                case 20:
                    ensureBrowser(); hidePanels();
                    browser->open (-1, -1, false); openTabIndex = tabBrowser; updateTabs();
                    browser->chooseAndInstall();   // v0.34: sound packs install into the library (PACKS)
                    break;
                case 21:
                    chooser = std::make_unique<FileChooser> ("Export my presets as a sound pack", File::getSpecialLocation (File::userDocumentsDirectory).getChildFile ("MY PACK.kkpack"), "*.kkpack");
                    chooser->launchAsync (FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::warnAboutOverwriting,
                                          [this] (const FileChooser& fc) { if (fc.getResult() != File()) proc.exportPack (fc.getResult().withFileExtension ("kkpack"), fc.getResult().getFileNameWithoutExtension()); });
                    break;
                case 22: KeysKillaProcessor::userPresetDir().startAsProcess(); break;
                case 23: KeysKillaProcessor::packsDir().startAsProcess(); break;
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
        breedBtn.ready = proc.parentsReady(); breedBtn.repaint();
        prevA.setVisible (proc.parent (0).valid()); nextA.setVisible (proc.parent (0).valid());
        prevB.setVisible (proc.parent (1).valid()); nextB.setVisible (proc.parent (1).valid());
        if (const int pv = (proc.parent (0).valid() ? 1 : 0) | (proc.parent (1).valid() ? 2 : 0) | (proc.labWav[0] ? 4 : 0) | (proc.labWav[1] ? 8 : 0); pv != parentsShown) { parentsShown = pv; repaint (R (214, 128, 1434, 376)); }
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
        const bool sampleMode = proc.sampleActive() || proc.chopActive();   // v0.37: the knobs colour your sample / the SAMPLER too
        lastSampleMode = sampleMode;
        for (int i = 0; i < 8; ++i)
        {
            const String t = sampleMode ? String (normal[i]) : custom.size() == 8 ? custom[i] : String (bass ? bassL[i] : normal[i]);
            captions[(size_t) i]->text = t;
            captions[(size_t) i]->custom = t != normal[i];
            captions[(size_t) i]->repaint();
            macros[(size_t) i]->setTooltip (sampleMode ? String (tipsN[i]) + "  (on the sample that plays)" : String (bass ? tipsB[i] : tipsN[i]));
        }
        if (matterStrip) { StringArray nm; for (auto& c : captions) nm.add (c->text); matterStrip->setNames (nm); }
        if (proc.labVersion() != lastLab || proc.pairVer.load() != lastPairVer)
        {
            lastLab = proc.labVersion(); lastPairVer = proc.pairVer.load();
            repaint (R (46, 613, 1632, 661));   // the 1-2-3 guide
            parentA.repaint(); parentB.repaint();
            for (auto& c : childCards) c->repaint();
            for (auto& gsw : genes) gsw->repaint();
            soundModeBtn.selected = ! proc.mainLoopMode; loopModeBtn.selected = proc.mainLoopMode; soundModeBtn.repaint(); loopModeBtn.repaint();
        }
        if (proc.mainLoopMode && proc.loopPlaying() && (proc.loopOwnerId() == 3 || proc.loopOwnerId() == 2)) for (auto& c : childCards) c->repaint();   // loop playhead
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
        updateKeysPill();
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

        if (breedBtn.ready != proc.parentsReady()) { breedBtn.ready = proc.parentsReady(); breedBtn.repaint(); }
        if (breedBtn.flash > 0) { breedBtn.flash = std::max (0.0f, breedBtn.flash - 0.08f); breedBtn.repaint(); }
        proc.renderNextThumbnail();   // one child waveform per tick keeps the UI smooth
        syncKeysToPage();
        if (! isPanelVisible() && ! evolveOpen() && proc.mainLoopMode && proc.loopPlaying() && (proc.loopOwnerId() == 3 || proc.loopOwnerId() == 2) && (++loopTick & 1))
            for (auto& c : childCards) c->repaint();   // the loop playhead in the cards

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
            || proc.labVersion() != lastLab || eraNow != lastEra || (proc.sampleActive() || proc.chopActive()) != lastSampleMode)
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
    int parentsShown = -1;
    BreedButton breedBtn;
    SparkOverlay sparks;
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
    std::unique_ptr<kkedit::SoundEditPage> soundEdit;
    std::unique_ptr<InsetPage> sampleEdit;
    HotButton editBtn { lnf };
    std::unique_ptr<PresetBrowser> browser;
    std::unique_ptr<MatterStrip> matterStrip;
    std::array<std::unique_ptr<Component>, numPages> modules;
    std::unique_ptr<FamilyTreePanel> treePanel;
    LabSwitch labSwitch { lnf };
    std::unique_ptr<EvolvePage> evolve;
    bool evolveOpen() const { return evolve != nullptr && evolve->isVisible(); }
    void showEvolve (bool on)
    {
        if (evolve == nullptr) return;
        if (on) { hidePanels(); openTabIndex = -1; updateTabs(); }
        evolve->setVisible (on);
        if (on) evolve->toFront (false);
        settings->setValue ("evolveOpen", on);
    }

    // v0.37 THE SOUND ON THE KEYS: what you picked last plays (the PC keys, your MIDI keyboard, FL's piano roll).
    // Opening a page that HAS its own sound hands the keys to it once (BREED LAB, FAMILY TREE, SAMPLER, EVOLVE);
    // EDIT, FX, MY SOUNDS and the bank never take the keys away - they work on the sound that plays.
    int currentPageId() const { return isPanelVisible() ? openTabIndex : evolveOpen() ? -50 : -1; }
    void takeKeysForPage (int page)
    {
        switch (page)
        {
            case -50:   // EVOLVE: the middle sound
                if (isPositiveAndBelow (proc.evoCenter, (int) proc.evo.size()))
                {
                    const auto& n = proc.evo[(size_t) proc.evoCenter];
                    if (n.isAudio()) { if (proc.activeSample() != n.audio || ! proc.sampleActive()) proc.useSample (n.audio, false); }
                    else proc.setPlayMode (KeysKillaProcessor::playKeys);
                }
                break;
            case -1:    // BREED LAB: its selected child (your sounds = the audio child)
                if (proc.labAudioMode() && isPositiveAndBelow (proc.pairSel, (int) proc.pairKids.size()))
                { if (proc.activeSample() != proc.pairKids[(size_t) proc.pairSel] || ! proc.sampleActive()) proc.useSample (proc.pairKids[(size_t) proc.pairSel], false); }
                else if (! proc.labAudioMode() && proc.selectedChild() >= 0) proc.setPlayMode (KeysKillaProcessor::playKeys);
                break;
            case tabTree:
                if (! proc.treeKids().empty() && proc.treeSelected() >= 0) proc.setPlayMode (KeysKillaProcessor::playKeys);
                break;
            case tabSampler:
                if (proc.chop.hasSource()) proc.setPlayMode (KeysKillaProcessor::playChop);
                break;
            case tabVst: proc.setPlayMode (KeysKillaProcessor::playVst); break;
            default: break;   // EDIT, FX, MY SOUNDS, the bank: the sound stays
        }
    }
    void syncKeysToPage()
    {
        const int page = currentPageId();
        if (page == lastKeysPage) return;
        if (lastKeysPage == -99) { lastKeysPage = page; return; }   // the editor just opened: the sound on the keys stays
        lastKeysPage = page;
        const int before = (int) proc.apvts.getRawParameterValue (ID::playMode)->load();
        takeKeysForPage (page);
        if ((int) proc.apvts.getRawParameterValue (ID::playMode)->load() != before) proc.panic();   // nothing keeps ringing from the sound you left
    }
    int lastKeysPage = -99, lastPairVer = -1;
    bool lastSampleMode = false;
    int loopTick = 0;

    void spaceAction()
    {
        if (evolveOpen() && ! isPanelVisible()) { evolve->spacePressed(); return; }
        if (openTabIndex == tabMelody && isPanelVisible())   // v0.40: MELODY - stop / play the melody
        {
            if (auto* m = module (tabMelody)) if (auto* ip = dynamic_cast<InsetPage*> (m)) if (auto* mp = dynamic_cast<MelodyPage*> (ip->page())) { mp->spacePressed(); return; }
        }
        if (openTabIndex == tabSampler && isPanelVisible())   // v0.35: SAMPLER - stop / play the part
        {
            std::function<ChopPanel* (Component*)> find = [&find] (Component* c) -> ChopPanel*
            { if (auto* cp = dynamic_cast<ChopPanel*> (c)) return cp; for (auto* ch : c->getChildren()) if (auto* r = find (ch)) return r; return nullptr; };
            if (auto* m = module (tabSampler)) if (auto* cp = find (m)) { cp->spacePressed(); return; }
        }
        if (treePanel != nullptr && treePanel->isVisible() && ! proc.treeKids().empty())
        {
            if (proc.loopPlaying()) proc.stopLoop();
            else if (proc.getTreeMode() == KeysKillaProcessor::treeLoop) proc.playTreeResult (jmax (0, proc.treeSelected()));
            else proc.previewNote = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f ? 36 : 60;
            treePanel->refresh();
            return;
        }
        // v0.31: SPACE = stop / start the loop, wherever it was started (BREED LAB, PAIR)
        if (proc.loopPlaying()) { spaceStopped = proc.loopOwnerId(); proc.stopLoop(); proc.touchLab(); refreshState(); return; }
        const bool pairOpen = openTabIndex == tabPair || openTabIndex == tabVst;
        if (pairOpen && spaceStopped == 2 && isPositiveAndBelow (proc.pairLoopKid, (int) proc.pairKids.size()))
        {
            proc.resumeLoop();
            if (auto* m = module (openTabIndex)) m->repaint();
            return;
        }
        if (pairOpen && ! proc.pairKids.empty())
        {
            proc.togglePairLoop (jlimit (0, (int) proc.pairKids.size() - 1, proc.pairSel));
            if (auto* m = module (openTabIndex)) m->repaint();
            return;
        }
        if (openTabIndex < 0 && proc.mainLoopMode && ! proc.kids().empty())   // BREED LAB in LOOP mode: the selected child's loop
        {
            proc.playChild (jmax (0, proc.selectedChild()));
            refreshState();
            return;
        }
        proc.previewNote = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f ? 36 : 60;   // hear the current sound
    }
    HotButton keysPill { lnf };
    void updateKeysPill()   // v0.42 KEYS: what the keys play now - click = back to the preset
    {
        const bool other = proc.sampleActive() || proc.chopActive();
        const String txt = "KEYS:  " + (proc.chopActive() ? String ("THE SAMPLER") : proc.sampleActive() && proc.activeSample() != nullptr ? proc.activeSample()->name : proc.currentName())
                         + (other ? String ("      < BACK TO THE PRESET") : String());
        if (txt != keysPill.getButtonText()) { keysPill.setButtonText (txt); keysPill.repaint(); }
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
    int spaceStopped = 0;
    int lastDrum = 0;       // DRUM KIT tile opens the drum you used last   // whose loop SPACE stopped last (2 PAIR, 3 BREED LAB)
    int warnHold = 0, lastIndex = -2, slowTick = 0, lastLab = -1, openTabIndex = -1, lastMutate = -1;
    float lastEra = -1;
    String lastName;
    uint64_t lastPlayHash = 0;
    float lastLockHash = -1;

    // attachments last: they must be destroyed before the controls they point to
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> attachments;
};

//==============================================================================
#include "FxMainPage.h"

#if KK_TEST_BUILD
// test snapshots of the EVOLVE FX page (the test app is built as EVOLVE)
juce::Image kkFxSnapshot (KeysKillaProcessor& p, int view)
{
    auto pg = std::make_unique<FxMainPage> (p);
    pg->setBounds (0, 0, KeysKillaEditor::designW, KeysKillaEditor::designH);
    pg->showView (view);
    return pg->createComponentSnapshot (pg->getLocalBounds(), true, 0.8f);
}
#endif

KeysKillaEditor::KeysKillaEditor (KeysKillaProcessor& p) : AudioProcessorEditor (p), proc (p)
{
    setWantsKeyboardFocus (false);
    setMouseClickGrabsKeyboardFocus (false);
    kk::themeIndex() = jlimit (0, 1, openSettings()->getIntValue ("theme", 0));   // v0.34: GLASS (day) / NIGHT, remembered
    frameOn = openSettings()->getBoolValue ("frame", true);
   #if KK_FX_BUILD
    auto fx = std::make_unique<FxMainPage> (p);   // v0.42 EVOLVE FX: the mixer plugin
    const int pref = fx->preferredScale();
    page = std::move (fx);
   #else
    auto mp = std::make_unique<MainPage> (p);
    const int pref = mp->preferredScale();
    page = std::move (mp);
   #endif
    addAndMakeVisible (*page);
    setResizable (true, true);
    setOpaque (true);
    setScalePct (jmin (pref, fitScale()));
}

// v0.31: the window always fits the screen - the host puts its own toolbars above and its browser beside the plugin
int KeysKillaEditor::fitScale() const
{
    const auto& displays = Desktop::getInstance().getDisplays();
    const Displays::Display* d = isShowing() ? displays.getDisplayForRect (getScreenBounds()) : displays.getPrimaryDisplay();
    if (d == nullptr) return 85;
    const auto area = d->userArea;   // logical pixels: Windows display scaling is already taken into account
    const double byW = (area.getWidth() * 0.78) / outerW();
    const double byH = (area.getHeight() - 190.0) / outerH();
    return jlimit (40, 100, (int) std::floor (std::min (byW, byH) * 100.0));
}

KeysKillaEditor::~KeysKillaEditor() { closeHostedEditor(); }

void KeysKillaEditor::showView (int v)
{
    if (auto* mp = dynamic_cast<MainPage*> (page.get())) mp->showView (v);
    else if (auto* fp = dynamic_cast<FxMainPage*> (page.get())) fp->showView (v);
}

// Windows: draw with the software renderer. The GUI is bitmap based, so it costs nothing, and it keeps the
// plugin out of the host's Direct2D device - hosts can hang on exit when plugin windows hold GPU resources.
void KeysKillaEditor::parentHierarchyChanged()
{
    if (! fitted && getPeer() != nullptr)   // now we know the real screen: shrink if the window would not fit on it
    {
        fitted = true;
        const int fit = fitScale();
        if (getWidth() > outerW() * fit / 100 + 2)
            MessageManager::callAsync ([safe = Component::SafePointer<KeysKillaEditor> (this), fit]
            { if (safe != nullptr) safe->setScalePct (fit); });
    }
   #if JUCE_WINDOWS
    if (auto* peer = getPeer())
        if (peer->getCurrentRenderingEngine() != 0)
            peer->setCurrentRenderingEngine (0);
   #endif
}

// the hosted plugin window inside BREED LAB: our header (name, TAKE THIS SOUND, CLOSE) above the plugin's own editor
class HostedVstPanel : public Component, private ComponentListener
{
public:
    HostedVstPanel (AudioPluginInstance& inst, const String& t) : title (t)
    {
        editor.reset (inst.createEditorIfNeeded());
        if (editor != nullptr) { addAndMakeVisible (*editor); editor->addComponentListener (this); }
        auto setup = [this] (TextButton& b, const String& txt) { b.setButtonText (txt); addAndMakeVisible (b); };
        setup (takeBtn, "TAKE THIS SOUND"); setup (closeBtn, "CLOSE");
        takeBtn.onClick = [this] { if (onTake) onTake(); };
        closeBtn.onClick = [this] { if (onClose) onClose(); };
        setOpaque (true);
    }
    ~HostedVstPanel() override { if (editor != nullptr) editor->removeComponentListener (this); editor.reset(); }
    std::function<void()> onTake, onClose, onEditorResized;
    bool ok() const { return editor != nullptr; }
    static constexpr int header = 46, margin = 8;
    Rectangle<int> wanted() const { return editor != nullptr ? Rectangle<int> (editor->getWidth() + margin * 2, editor->getHeight() + header + margin) : Rectangle<int>(); }
    // try to make the plugin fit in w x h (plugins that support it scale their own window)
    void fitTo (int w, int h)
    {
        if (editor == nullptr) return;
        const float sx = (float) (w - margin * 2) / (float) editor->getWidth(), sy = (float) (h - header - margin) / (float) editor->getHeight();
        if (const float f = std::min (sx, sy); f < 1.0f) editor->setScaleFactor (jmax (0.4f, f));
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        g.fillAll (t.night ? Colour (0xff0d0f12) : Colour (0xffdfe2e6));
        g.setColour (t.text); g.setFont (kk::modern::font (16.0f, true, 0.12f));
        g.drawText (title, Rectangle<int> (16, 0, getWidth() / 2, header), Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, false, 0.08f));
        g.drawText ("pick a sound in the plugin  ->  TAKE THIS SOUND  (it joins the list)", Rectangle<int> (16, 0, getWidth() - 360, header).withTrimmedLeft (getWidth() / 4), Justification::centred);
        g.setColour (t.accent.withAlpha (0.6f)); g.drawHorizontalLine (header - 1, 0, (float) getWidth());
    }
    void resized() override
    {
        closeBtn.setBounds (getWidth() - 110, 8, 96, header - 16);
        takeBtn.setBounds (closeBtn.getX() - 190, 8, 180, header - 16);
        if (editor != nullptr) editor->setTopLeftPosition (jmax (margin, (getWidth() - editor->getWidth()) / 2), header + jmax (0, (getHeight() - header - editor->getHeight()) / 2));
    }
private:
    void componentMovedOrResized (Component& c, bool, bool wasResized) override { if (&c == editor.get() && wasResized) { resized(); if (onEditorResized) onEditorResized(); } }
    std::unique_ptr<AudioProcessorEditor> editor;
    TextButton takeBtn, closeBtn;
    String title;
};

bool KeysKillaEditor::showHostedEditor (AudioPluginInstance& inst, const String& title, std::function<void()> onTake, std::function<void()> onClosed)
{
    closeHostedEditor();
    if (! inst.hasEditor()) return false;
    auto panel = std::make_unique<HostedVstPanel> (inst, title);
    if (! panel->ok()) return false;
    auto* p = panel.get();
    // the area under the header and right of the tiles, in real pixels (the plugin window itself is never scaled by us)
    auto area = [this]
    {
        const float s = (float) getWidth() / (float) outerW();
        const int x = roundToInt ((float) (frame() + 150) * s), y = roundToInt ((float) (frame() + 96) * s);
        return Rectangle<int> (x, y, getWidth() - x - roundToInt ((float) frame() * s) - 4, getHeight() - y - roundToInt ((float) frame() * s) - 4);
    };
    auto a = area();
    if (p->wanted().getWidth() > a.getWidth() || p->wanted().getHeight() > a.getHeight())
    {
        p->fitTo (a.getWidth(), a.getHeight());   // plugins that can scale themselves
        if (p->wanted().getWidth() > a.getWidth() || p->wanted().getHeight() > a.getHeight())
        {
            // still too big: BREED LAB grows (as far as the screen allows) while the plugin is shown
            pctBeforeHosted = lastPct;
            const float needX = (float) p->wanted().getWidth() / (float) a.getWidth(), needY = (float) p->wanted().getHeight() / (float) a.getHeight();
            const int want = (int) std::ceil ((float) lastPct * std::max (needX, needY)) + 1;
            setScalePct (jmin (100, jmin (want, fitScale() + 15)));
            a = area();
            if (p->wanted().getWidth() > a.getWidth() || p->wanted().getHeight() > a.getHeight()) { panel.reset(); if (pctBeforeHosted > 0) setScalePct (pctBeforeHosted); pctBeforeHosted = -1; return false; }
        }
    }
    p->onTake = std::move (onTake);
    p->onClose = [this, onClosed] { closeHostedEditor(); if (onClosed) onClosed(); };
    p->onEditorResized = [this, area] { if (hostedPanel != nullptr) hostedPanel->setBounds (area()); };
    hostedPanel = std::move (panel);
    addAndMakeVisible (*hostedPanel);
    hostedPanel->setBounds (a);
    hostedPanel->toFront (false);
    return true;
}

void KeysKillaEditor::closeHostedEditor()
{
    if (hostedPanel == nullptr) return;
    auto old = std::move (hostedPanel);   // the plugin's editor goes first, our window size comes back after
    old.reset();
    if (pctBeforeHosted > 0) { setScalePct (pctBeforeHosted); pctBeforeHosted = -1; }
}

void KeysKillaEditor::setScalePct (int pct)
{
    lastPct = pct;
    setResizeLimits (outerW() * 2 / 5, outerH() * 2 / 5, outerW(), outerH());
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) outerW() / outerH());
    setSize (outerW() * pct / 100, outerH() * pct / 100);
}

void KeysKillaEditor::setFrame (bool on)
{
    if (on == frameOn) return;
    const int pct = jmax (40, (int) std::round ((double) getWidth() * 100.0 / outerW()));
    frameOn = on; frameImg = {};
    openSettings()->setValue ("frame", on);
    setScalePct (jmin (pct, fitScale()));
    resized(); repaint();
}

void KeysKillaEditor::resized()
{
    const float s = (float) getWidth() / (float) outerW();
    page->setTransform (AffineTransform::scale (s).translated ((float) frame() * s, (float) frame() * s));
    page->setBounds (0, 0, designW, designH);
}

// v0.34 THE CASE: a thin milled-metal frame with chamfered corners, corner bumpers, grip grooves on the sides and
// one amber status light - the glass display sits inside it. Light aluminium by day, dark anodised metal at night.
void KeysKillaEditor::paint (Graphics& g)
{
    if (frame() == 0) { g.fillAll (Colours::black); return; }
    const int W = outerW(), H = outerH();
    if (! frameImg.isValid() || frameImg.getWidth() != getWidth() || frameImg.getHeight() != getHeight())
    {
        frameImg = Image (Image::ARGB, getWidth(), getHeight(), true);
        Graphics fg (frameImg);
        fg.addTransform (AffineTransform::scale ((float) getWidth() / (float) W));
        const auto& t = kk::theme();
        const bool night = t.night;
        const float F = (float) frame(), ch = 46.0f;
        auto chamfered = [] (Rectangle<float> r, float c)
        {
            Path p;
            p.startNewSubPath (r.getX() + c, r.getY()); p.lineTo (r.getRight() - c, r.getY()); p.lineTo (r.getRight(), r.getY() + c);
            p.lineTo (r.getRight(), r.getBottom() - c); p.lineTo (r.getRight() - c, r.getBottom()); p.lineTo (r.getX() + c, r.getBottom());
            p.lineTo (r.getX(), r.getBottom() - c); p.lineTo (r.getX(), r.getY() + c); p.closeSubPath();
            return p;
        };
        const Rectangle<float> outer (0, 0, (float) W, (float) H);
        const Colour mTop = night ? Colour (0xff474c54) : Colour (0xffeef0f2), mBot = night ? Colour (0xff15171a) : Colour (0xff9aa0a8);
        // body of the case
        auto body = chamfered (outer, ch);
        fg.setGradientFill (ColourGradient (mTop, 0, 0, mBot, 0, (float) H, false)); fg.fillPath (body);
        // brushed metal
        Random rnd (11);
        {
            Graphics::ScopedSaveState ss (fg);
            fg.reduceClipRegion (body);
            for (float y = 0; y < (float) H; y += 1.5f)
            { fg.setColour ((rnd.nextBool() ? Colours::white : Colours::black).withAlpha (rnd.nextFloat() * (night ? 0.05f : 0.06f))); fg.drawHorizontalLine ((int) y, 0, (float) W); }
            // light catching the top-left
            fg.setGradientFill (ColourGradient (Colours::white.withAlpha (night ? 0.10f : 0.35f), 0, 0, Colours::white.withAlpha (0.0f), (float) W * 0.35f, (float) H * 0.6f, false));
            fg.fillPath (body);
        }
        // outer bevel: bright upper edge, dark lower edge
        fg.setGradientFill (ColourGradient (Colours::white.withAlpha (night ? 0.35f : 0.95f), 0, 0, Colours::black.withAlpha (night ? 0.8f : 0.35f), 0, (float) H, false));
        fg.strokePath (chamfered (outer.reduced (0.8f), ch - 0.4f), PathStrokeType (1.6f));
        fg.setColour (Colours::white.withAlpha (night ? 0.06f : 0.25f));
        fg.strokePath (chamfered (outer.reduced (4.0f), ch - 2.0f), PathStrokeType (1.0f));
        auto softShadowPath = [night] (Graphics& gg, const Path& p)
        { for (int k = 1; k <= 3; ++k) { gg.setColour (Colours::black.withAlpha (night ? 0.18f : 0.08f)); gg.fillPath (p, AffineTransform::translation (0.0f, (float) k * 1.2f)); } };
        // corner bumpers: darker pieces over the chamfers
        const Colour bump = night ? Colour (0xff0e1012) : Colour (0xff4a5058);
        for (int k = 0; k < 4; ++k)
        {
            const bool right = k % 2 == 1, bottom = k >= 2;
            const float x0 = right ? (float) W : 0.0f, y0 = bottom ? (float) H : 0.0f, sx = right ? -1.0f : 1.0f, sy = bottom ? -1.0f : 1.0f;
            Path b;
            const float d = F * 0.78f, L = ch + 90.0f;
            b.startNewSubPath (x0 + sx * ch, y0); b.lineTo (x0 + sx * L, y0); b.lineTo (x0 + sx * (L - 10), y0 + sy * d);
            b.lineTo (x0 + sx * (d + 12), y0 + sy * d); b.lineTo (x0 + sx * d, y0 + sy * (d + 12));
            b.lineTo (x0 + sx * d, y0 + sy * (L - 10)); b.lineTo (x0, y0 + sy * L); b.lineTo (x0, y0 + sy * ch); b.closeSubPath();
            softShadowPath (fg, b);
            fg.setGradientFill (ColourGradient (bump.brighter (0.35f), x0, y0, bump, x0 + sx * 40, y0 + sy * 40, false)); fg.fillPath (b);
            fg.setColour (Colours::white.withAlpha (night ? 0.10f : 0.30f)); fg.strokePath (b, PathStrokeType (0.8f));
        }
        // grips on the sides: a raised dark block with grooves
        for (int side = 0; side < 2; ++side)
        {
            const float x = side == 0 ? F * 0.5f : (float) W - F * 0.5f;
            const Rectangle<float> blk (x - F * 0.36f, (float) H * 0.5f - 90.0f, F * 0.72f, 180.0f);
            fg.setColour (Colours::black.withAlpha (night ? 0.6f : 0.3f)); fg.fillRoundedRectangle (blk.translated (1.5f, 3.0f), 6.0f);
            fg.setGradientFill (ColourGradient (bump.brighter (0.4f), blk.getX(), blk.getY(), bump, blk.getRight(), blk.getBottom(), false)); fg.fillRoundedRectangle (blk, 6.0f);
            fg.setColour (Colours::white.withAlpha (night ? 0.12f : 0.35f)); fg.drawRoundedRectangle (blk.reduced (0.5f), 6.0f, 0.8f);
            for (int i = 0; i < 8; ++i)
            {
                const float y = (float) H * 0.5f - 70.0f + (float) i * 18.0f;
                const Rectangle<float> gr (x - F * 0.22f, y, F * 0.44f, 7.0f);
                fg.setColour (Colours::black.withAlpha (night ? 0.7f : 0.35f)); fg.fillRoundedRectangle (gr, 2.5f);
                fg.setColour (Colours::white.withAlpha (night ? 0.12f : 0.6f)); fg.drawLine (gr.getX() + 1.5f, gr.getBottom() + 0.6f, gr.getRight() - 1.5f, gr.getBottom() + 0.6f, 0.8f);
            }
        }
        // the recess the glass sits in
        const auto inner = Rectangle<float> (F, F, (float) designW, (float) designH);
        fg.setColour (Colours::black.withAlpha (night ? 0.9f : 0.45f)); fg.drawRoundedRectangle (inner.expanded (1.5f), 6.0f, 3.0f);
        fg.setColour (Colours::white.withAlpha (night ? 0.10f : 0.7f)); fg.drawRoundedRectangle (inner.expanded (3.5f), 7.0f, 1.0f);
        // amber status light, bottom centre
        const Rectangle<float> led ((float) W * 0.5f - 22.0f, (float) H - F * 0.5f - 1.5f, 44.0f, 3.0f);
        fg.setGradientFill (ColourGradient (t.accent.withAlpha (0.45f), led.getCentreX(), led.getCentreY(), t.accent.withAlpha (0.0f), led.getCentreX() + 40, led.getCentreY(), true));
        fg.fillEllipse (led.expanded (24, 8));
        fg.setColour (t.accent); fg.fillRoundedRectangle (led, 1.5f);
        fg.setColour (Colours::white.withAlpha (0.6f)); fg.fillRoundedRectangle (led.reduced (8, 1), 0.5f);
    }
    g.fillAll (kk::theme().night ? Colour (0xff050607) : Colour (0xff5d636b));   // behind the chamfers (host background)
    g.drawImageAt (frameImg, 0, 0);
}
