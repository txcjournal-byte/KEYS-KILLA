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
Font KKLookAndFeel::getLabelFont (Label& l) { return serif (jmax (11.0f, (float) l.getHeight() * 0.72f), false, 0.2f); }
Font KKLookAndFeel::getPopupMenuFont() { return serif (25.0f, false, 0.05f); }   // menus scale with the 60 % window


//==============================================================================
// Skin bitmaps (from the design mockups, see tools/make_skin_assets.py) + element geometry.
// Coordinates are in design pixels (1586 x 992). Skin 0 = CHROME, 1 = BLOOD.
struct Geo
{
    Rectangle<int> prev, name, heart, next, save, menu, skin;
    std::array<Rectangle<int>, 10> tiles;
    Rectangle<int> subBar;
    std::array<Point<int>, 6> era;
    Rectangle<int> exclusive, dice, chaos, xy;
    std::array<Point<int>, 6> macro; int macroCap, macroArc, macroLabelY;
    std::array<Point<int>, 3> small; int smallCap, smallArc;
    Rectangle<int> chord, link, arp, meter, keyboard, pitch, mod;
};

static Rectangle<int> R (int x0, int y0, int x1, int y1) { return { x0, y0, x1 - x0, y1 - y0 }; }

static const Geo& geo (int skin)
{
    static const Geo chrome = []
    {
        Geo g;
        g.prev = R (737, 105, 787, 158); g.name = R (790, 105, 1068, 158); g.heart = R (1068, 110, 1102, 154); g.next = R (1115, 105, 1162, 158);
        g.save = R (1180, 105, 1253, 175); g.menu = R (1273, 105, 1362, 175); g.skin = R (1380, 120, 1418, 158);
        const int tx[10][2] { { 47, 128 }, { 135, 224 }, { 231, 319 }, { 325, 415 }, { 421, 515 }, { 521, 619 }, { 626, 722 }, { 729, 826 }, { 832, 927 }, { 933, 1052 } };
        for (int i = 0; i < 10; ++i) g.tiles[(size_t) i] = R (tx[i][0], 297, tx[i][1], 437);
        g.subBar = R (60, 440, 1040, 461);
        const int ex[6] { 86, 203, 321, 442, 561, 678 };
        for (int i = 0; i < 6; ++i) g.era[(size_t) i] = { ex[i], 494 };
        g.exclusive = R (833, 465, 1043, 507); g.dice = R (1112, 290, 1262, 446); g.chaos = R (1110, 468, 1262, 489); g.xy = R (1312, 300, 1508, 443);
        const int mx[6] { 158, 319, 480, 639, 797, 958 };
        for (int i = 0; i < 6; ++i) g.macro[(size_t) i] = { mx[i], 657 };
        g.macroCap = 46; g.macroArc = 63; g.macroLabelY = 728;
        const int sx[3] { 1190, 1329, 1472 };
        for (int i = 0; i < 3; ++i) g.small[(size_t) i] = { sx[i], 547 };
        g.smallCap = 31; g.smallArc = 45;
        g.chord = R (1112, 666, 1222, 708); g.link = R (1234, 668, 1270, 704); g.arp = R (1279, 663, 1395, 710);
        g.meter = R (1456, 658, 1510, 772); g.keyboard = R (163, 797, 1533, 922); g.pitch = R (48, 808, 84, 900); g.mod = R (102, 808, 138, 900);
        return g;
    }();
    static const Geo blood = []
    {
        Geo g;
        g.prev = R (720, 112, 768, 162); g.name = R (770, 112, 1085, 162); g.heart = R (1085, 118, 1117, 158); g.next = R (1137, 112, 1185, 162);
        g.save = R (1200, 112, 1262, 178); g.menu = R (1264, 112, 1350, 178); g.skin = R (1362, 124, 1400, 162);
        const int tx[10][2] { { 43, 127 }, { 133, 231 }, { 237, 330 }, { 336, 433 }, { 440, 539 }, { 546, 649 }, { 654, 751 }, { 757, 856 }, { 862, 960 }, { 966, 1076 } };
        for (int i = 0; i < 10; ++i) g.tiles[(size_t) i] = R (tx[i][0], 290, tx[i][1], 428);
        g.subBar = R (56, 431, 1066, 452);
        const int ex[6] { 94, 211, 331, 451, 568, 686 };
        for (int i = 0; i < 6; ++i) g.era[(size_t) i] = { ex[i], 487 };
        g.exclusive = R (849, 457, 1067, 497); g.dice = R (1125, 280, 1275, 450); g.chaos = R (1133, 455, 1269, 476); g.xy = R (1320, 300, 1506, 440);
        const int mx[6] { 166, 330, 494, 655, 816, 977 };
        for (int i = 0; i < 6; ++i) g.macro[(size_t) i] = { mx[i], 650 };
        g.macroCap = 47; g.macroArc = 65; g.macroLabelY = 730;
        const int sx[3] { 1206, 1335, 1461 };
        for (int i = 0; i < 3; ++i) g.small[(size_t) i] = { sx[i], 540 };
        g.smallCap = 25; g.smallArc = 37;
        g.chord = R (1105, 653, 1235, 693); g.link = R (1242, 655, 1276, 691); g.arp = R (1280, 653, 1400, 693);
        g.meter = R (1455, 652, 1505, 778); g.keyboard = R (158, 800, 1510, 938); g.pitch = R (43, 810, 80, 895); g.mod = R (98, 810, 136, 895);
        return g;
    }();
    return skin == 1 ? blood : chrome;
}

struct SkinImages { Image bg, white, black; };

// Skin bitmaps live only while an editor is open: they are native (Direct2D) images on Windows and
// must be released before the host shuts its graphics down - static images froze FL Studio on exit.
struct SkinCache { SkinImages imgs[2]; };
static SkinCache*& activeSkinCache() { static SkinCache* c = nullptr; return c; }
struct SkinCacheHolder
{
    SharedResourcePointer<SkinCache> cache;
    SkinCacheHolder() { activeSkinCache() = &cache.get(); }
    ~SkinCacheHolder() { if (cache.getReferenceCount() <= 1) activeSkinCache() = nullptr; }
};

static const SkinImages& skinImages (int skin)
{
    static SkinImages none;
    auto* cache = activeSkinCache();
    if (cache == nullptr) return none;
    auto& s = cache->imgs[skin == 1 ? 1 : 0];
    if (s.bg.isNull())
    {
        auto load = [] (const void* d, int n) { return ImageFileFormat::loadFrom (d, (size_t) n); };
        if (skin == 1)
        {
            s.bg = load (BinaryData::blood_bg_jpg, BinaryData::blood_bg_jpgSize);
            s.white = load (BinaryData::blood_white_png, BinaryData::blood_white_pngSize);
            s.black = load (BinaryData::blood_black_png, BinaryData::blood_black_pngSize);
        }
        else
        {
            s.bg = load (BinaryData::chrome_bg_jpg, BinaryData::chrome_bg_jpgSize);
            s.white = load (BinaryData::chrome_white_png, BinaryData::chrome_white_pngSize);
            s.black = load (BinaryData::chrome_black_png, BinaryData::chrome_black_pngSize);
        }
    }
    return s;
}

// Keyboard focus must stay with the host (space = play/stop in FL Studio)
static void noFocus (Component& c)
{
    if (dynamic_cast<TextEditor*> (&c) == nullptr)
    {
        c.setWantsKeyboardFocus (false);
        c.setMouseClickGrabsKeyboardFocus (false);
    }
    for (auto* ch : c.getChildren()) noFocus (*ch);
}

static void drawGlowFrame (Graphics& g, Rectangle<float> r, Colour accent, float corner = 5.0f)
{
    g.setColour (accent.withAlpha (0.18f)); g.drawRoundedRectangle (r.expanded (3), corner + 3, 6.0f);
    g.setColour (accent.withAlpha (0.35f)); g.drawRoundedRectangle (r.expanded (1), corner + 1, 3.0f);
    g.setColour (accent); g.drawRoundedRectangle (r, corner, 1.8f);
}

//==============================================================================
// Knob drawn from the design: the metal cap is cut out of the skin bitmap and rotated,
// the value arc is drawn live over the (cleaned) track.
class ImageKnob : public Slider
{
public:
    ImageKnob (KKLookAndFeel& l, const int& skinRef) : Slider (RotaryHorizontalVerticalDrag, NoTextBox), lnf (l), skin (skinRef)
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
        const auto& bg = skinImages (skin).bg;
        const float pos = (float) valueToProportionOfLength (getValue());
        const float ang = MathConstants<float>::pi * (-0.75f + 1.5f * pos);
        const Point<float> c ((float) getWidth() * 0.5f, (float) getHeight() * 0.5f);

        // rotated metal cap
        const auto src = bg.getClippedImage ({ centre.x - capR, centre.y - capR, capR * 2, capR * 2 });
        {
            Graphics::ScopedSaveState ss (g);
            Path clip; clip.addEllipse (c.x - (float) capR, c.y - (float) capR, (float) capR * 2, (float) capR * 2);
            g.reduceClipRegion (clip);
            g.setImageResamplingQuality (Graphics::highResamplingQuality);
            g.drawImageTransformed (src, AffineTransform::translation (-(float) capR, -(float) capR).rotated (ang).translated (c));
        }
        if (! s.dark)   // chrome caps have no painted pointer
        {
            const Point<float> a (c.x + std::sin (ang) * capR * 0.3f, c.y - std::cos (ang) * capR * 0.3f);
            const Point<float> b (c.x + std::sin (ang) * capR * 0.86f, c.y - std::cos (ang) * capR * 0.86f);
            g.setColour (Colours::black.withAlpha (0.25f)); g.drawLine ({ a.translated (1, 1), b.translated (1, 1) }, capR > 35 ? 4.0f : 3.0f);
            g.setColour (Colour (0xff20252b)); g.drawLine ({ a, b }, capR > 35 ? 3.0f : 2.2f);
        }
        // value arc
        if (pos > 0.002f)
        {
            Path arc; arc.addCentredArc (c.x, c.y, (float) arcR, (float) arcR, 0, -MathConstants<float>::pi * 0.75f, ang, true);
            const float w = capR > 35 ? 1.0f : 0.7f;
            g.setColour (s.accent.withAlpha (0.16f)); g.strokePath (arc, PathStrokeType (14.0f * w, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (s.accent.withAlpha (0.4f));  g.strokePath (arc, PathStrokeType (7.0f * w, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (s.accent);                   g.strokePath (arc, PathStrokeType (3.6f * w, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colours::white.withAlpha (s.dark ? 0.55f : 0.8f)); g.strokePath (arc, PathStrokeType (1.2f * w));
        }
    }
    bool hitTest (int x, int y) override { return Point<int> (x, y).getDistanceFrom ({ getWidth() / 2, getHeight() / 2 }) <= arcR + 6; }
private:
    KKLookAndFeel& lnf;
    const int& skin;
    Point<int> centre; int capR = 40, arcR = 60;
};

//==============================================================================
// Invisible hot-spot button that paints only its live state over the bitmap
class HotButton : public Button
{
public:
    enum Kind { plain, tile, toggleFace };
    HotButton (KKLookAndFeel& l, Kind k, String label = {}) : Button (label), lnf (l), kind (k), text (std::move (label)) {}
    std::function<void (Graphics&, Rectangle<float>, const Skin&)> glyph;
    bool selected = false;
    std::function<void()> onRightClick;

    void paintButton (Graphics& g, bool over, bool down) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (2);
        const bool on = kind == toggleFace ? getToggleState() : selected;
        if (kind == toggleFace)
        {
            auto face = r.reduced (4);
            if (s.dark) g.setColour (Colour (0xff120d0d));
            else g.setGradientFill (ColourGradient (Colour (0xfff1f3f5), 0, face.getY(), Colour (0xffc6ccd2), 0, face.getBottom(), false));
            g.fillRoundedRectangle (face, 3);
            g.setFont (serif (face.getHeight() * 0.5f, s.dark ? false : true, s.dark ? 0.18f : 0.08f));
            g.setColour (on ? (s.dark ? s.accent.brighter (0.2f) : Colour (0xff0b3d73)) : (s.dark ? Colour (0xffd8d2d2) : Colour (0xff1a1d21)));
            g.drawText (text, face, Justification::centred);
        }
        if (on) drawGlowFrame (g, r, s.accent, kind == tile ? 5.0f : 4.0f);
        if (on && kind == tile) { g.setColour (s.accent.withAlpha (0.07f)); g.fillRoundedRectangle (r, 5); }
        if (over && ! on) { g.setColour (s.accent.withAlpha (down ? 0.25f : 0.12f)); g.fillRoundedRectangle (r, 5); }
        if (glyph) glyph (g, r, s);
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() && onRightClick) { onRightClick(); return; }
        Button::mouseUp (e);
    }
private:
    KKLookAndFeel& lnf;
    Kind kind;
    String text;
};

//==============================================================================
class EraOverlay : public Component, public SettableTooltipClient
{
public:
    EraOverlay (KKLookAndFeel& l, const int& s) : lnf (l), skin (s) { setTooltip ("Filter presets by era. Click again to show all eras."); }
    std::function<void (int)> onSelect;
    int selected = -1;
    Point<int> origin;
    void paint (Graphics& g) override
    {
        if (selected < 0) return;
        const auto& s = *lnf.skin;
        const auto p = (geo (skin).era[(size_t) selected] - origin).toFloat();
        g.setColour (s.accent.withAlpha (0.25f)); g.fillEllipse (Rectangle<float> (30, 30).withCentre (p));
        g.setColour (s.accent.withAlpha (0.5f));  g.fillEllipse (Rectangle<float> (18, 18).withCentre (p));
        g.setColour (s.accent); g.fillEllipse (Rectangle<float> (11, 11).withCentre (p));
        g.setColour (Colours::white); g.fillEllipse (Rectangle<float> (4, 4).withCentre (p));
        g.setColour (s.accent); g.fillRoundedRectangle (Rectangle<float> (34, 3).withCentre (p.translated (0, -12)), 1.5f);
    }
    void mouseUp (const MouseEvent& e) override
    {
        int best = 0;
        for (int i = 1; i < 6; ++i)
            if (std::abs (geo (skin).era[(size_t) i].x - origin.x - e.x) < std::abs (geo (skin).era[(size_t) best].x - origin.x - e.x)) best = i;
        selected = selected == best ? -1 : best;
        repaint();
        if (onSelect) onSelect (selected);
    }
private:
    KKLookAndFeel& lnf; const int& skin;
};

//==============================================================================
// Sub-categories from the spec that have no tile of their own
struct SubCat { const char* label; const char* sub; };
static const std::array<SubCat, 10> subCats { { { "PIANO", "Piano" }, { "ORGANS", "Organs" }, { "STRINGS", "Strings" }, { "BRASS", "Brass" },
                                                { "GUITARS", "Guitars" }, { "MALLETS", "Mallets" }, { "ARPS", "Arps" },
                                                { "808", "808" }, { "TEXTURE", "Texture" }, { "FX", "FX" } } };

class SubChips : public Component, public SettableTooltipClient
{
public:
    explicit SubChips (KKLookAndFeel& l) : lnf (l) { setTooltip ("Sub-categories: piano, organs, strings, brass, guitars, mallets, arps, 808, textures and FX."); }
    std::function<void (int)> onSelect;
    int selected = -1;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        const float w = (float) getWidth() / (float) subCats.size();
        for (size_t i = 0; i < subCats.size(); ++i)
        {
            auto r = Rectangle<float> (w * (float) i, 0, w, (float) getHeight()).reduced (5, 1);
            const bool on = (int) i == selected;
            g.setColour (s.dark ? Colour (0xcc0c0909) : Colour (0xccf4f6f8));
            g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
            if (on) drawGlowFrame (g, r, s.accent, r.getHeight() * 0.5f);
            else { g.setColour (s.dark ? Colour (0x55ffffff) : Colour (0x66202428)); g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.0f); }
            g.setColour (on ? (s.dark ? s.accent.brighter (0.3f) : Colour (0xff0b3d73)) : (s.dark ? Colour (0xffd9d4d4) : Colour (0xff1a1d21)));
            g.setFont (serif (12.0f, ! s.dark, 0.1f));
            g.drawText (subCats[i].label, r, Justification::centred);
        }
    }
    void mouseUp (const MouseEvent& e) override
    {
        const int i = jlimit (0, (int) subCats.size() - 1, e.x * (int) subCats.size() / jmax (1, getWidth()));
        selected = selected == i ? -1 : i;
        repaint();
        if (onSelect) onSelect (selected);
    }
private:
    KKLookAndFeel& lnf;
};

//==============================================================================
class XYOverlay : public Component, public SettableTooltipClient
{
public:
    XYOverlay (KeysKillaProcessor& p, KKLookAndFeel& l)
        : lnf (l),
          ax (*p.apvts.getParameter (ID::morphX), [this] (float v) { x = v; repaint(); }),
          ay (*p.apvts.getParameter (ID::morphY), [this] (float v) { y = v; repaint(); })
    {
        ax.sendInitialUpdate(); ay.sendInitialUpdate();
        setTooltip ("ERA MORPH: drag between classic, melodic, raw and aggressive. Double-click to centre. Corner presets: ADVANCED > EXCLUSIVE.");
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (8);
        const Point<float> d (r.getX() + x * r.getWidth(), r.getBottom() - y * r.getHeight());
        g.setColour (s.accent.withAlpha (0.12f)); g.fillEllipse (Rectangle<float> (46, 46).withCentre (d));
        g.setColour (s.accent.withAlpha (0.3f));  g.fillEllipse (Rectangle<float> (28, 28).withCentre (d));
        g.setColour (s.accent);                   g.fillEllipse (Rectangle<float> (15, 15).withCentre (d));
        g.setColour (Colours::white);             g.fillEllipse (Rectangle<float> (6, 6).withCentre (d));
    }
    void mouseDown (const MouseEvent& e) override { ax.beginGesture(); ay.beginGesture(); drag (e); }
    void mouseDrag (const MouseEvent& e) override { drag (e); }
    void mouseUp (const MouseEvent&) override { ax.endGesture(); ay.endGesture(); }
    void mouseDoubleClick (const MouseEvent&) override { ax.setValueAsCompleteGesture (0.5f); ay.setValueAsCompleteGesture (0.5f); }
private:
    void drag (const MouseEvent& e)
    {
        auto r = getLocalBounds().toFloat().reduced (8);
        ax.setValueAsPartOfGesture (jlimit (0.0f, 1.0f, (e.position.x - r.getX()) / r.getWidth()));
        ay.setValueAsPartOfGesture (jlimit (0.0f, 1.0f, (r.getBottom() - e.position.y) / r.getHeight()));
    }
    KKLookAndFeel& lnf;
    ParameterAttachment ax, ay;
    float x = 0.5f, y = 0.5f;
};

//==============================================================================
class ChaosSlider : public Slider
{
public:
    explicit ChaosSlider (KKLookAndFeel& l) : Slider (LinearHorizontal, NoTextBox), lnf (l) {}
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat().reduced (8, 0);
        const float cy = r.getCentreY();
        const float px = r.getX() + (float) valueToProportionOfLength (getValue()) * r.getWidth();
        g.setColour (s.accent.withAlpha (0.3f)); g.fillRoundedRectangle (Rectangle<float> (r.getX(), cy - 3.5f, px - r.getX(), 7), 3.5f);
        g.setColour (s.accent); g.fillRoundedRectangle (Rectangle<float> (r.getX(), cy - 2, px - r.getX(), 4), 2);
        g.setColour (s.accent.withAlpha (0.35f)); g.fillEllipse (Rectangle<float> (24, 24).withCentre ({ px, cy }));
        g.setGradientFill (ColourGradient (Colours::white, px - 6, cy - 6, Colour (0xff8a9098), px + 6, cy + 6, false));
        g.fillEllipse (Rectangle<float> (15, 15).withCentre ({ px, cy }));
        g.setColour (s.accent); g.drawEllipse (Rectangle<float> (15, 15).withCentre ({ px, cy }), 2.2f);
    }
private:
    KKLookAndFeel& lnf;
};

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
class MeterOverlay : public Component
{
public:
    explicit MeterOverlay (KKLookAndFeel& l) : lnf (l) {}
    float l = 0, r = 0; bool warn = false;
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto b = getLocalBounds().toFloat();
        const float bw = b.getWidth() * 0.36f;
        const Rectangle<float> bars[2] { { b.getX() + 2, b.getY(), bw, b.getHeight() }, { b.getRight() - bw - 2, b.getY(), bw, b.getHeight() } };
        const int segs = 12;
        for (int ch = 0; ch < 2; ++ch)
        {
            const float db = jlimit (-36.0f, 0.0f, Decibels::gainToDecibels (ch == 0 ? l : r, -60.0f));
            const int lit = (int) std::round ((db + 36.0f) / 36.0f * segs);
            for (int i = 0; i < lit; ++i)
            {
                const float h = bars[ch].getHeight() / segs;
                auto seg = Rectangle<float> (bars[ch].getX(), bars[ch].getBottom() - h * (float) (i + 1), bw, h - 2.0f);
                const Colour c = (warn && i >= segs - 2) ? Colours::orange : s.accent;
                g.setColour (c.withAlpha (0.35f)); g.fillRect (seg.expanded (1.5f));
                g.setColour (c); g.fillRect (seg);
            }
        }
    }
private:
    KKLookAndFeel& lnf;
};

//==============================================================================
// Keyboard: real piano layout (the mockup keys are decorative), textures cut from the design
class KKKeyboard : public MidiKeyboardComponent
{
public:
    KKKeyboard (KeysKillaProcessor& p, KKLookAndFeel& l, const int& s)
        : MidiKeyboardComponent (p.keyboardState, horizontalKeyboard), proc (p), lnf (l), skin (s)
    {
        setAvailableRange (24, 107);
        setScrollButtonsVisible (false);
        setOctaveForMiddleC (4);
        setWantsKeyboardFocus (false);
        setMouseClickGrabsKeyboardFocus (false);
    }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        g.setColour (s.dark ? Colour (0xff0b0909) : Colour (0xff2a2e33));
        g.fillRoundedRectangle (getLocalBounds().toFloat(), 4);
        MidiKeyboardComponent::paint (g);
    }
    void drawWhiteNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour, Colour) override
    {
        const auto& s = *lnf.skin;
        const auto& tex = skinImages (skin).white;
        const bool on = isDown || proc.playing[(size_t) note].load();
        auto k = a.reduced (1.0f, 0).withTrimmedBottom (2);
        g.setImageResamplingQuality (Graphics::mediumResamplingQuality);
        g.drawImage (tex, k, RectanglePlacement::stretchToFit);
        g.setColour (Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (k, 2, 1);
        if (! inScale (note)) { g.setColour (Colours::black.withAlpha (0.3f)); g.fillRect (k); }
        if (on)
        {
            g.setGradientFill (ColourGradient (s.accent.withAlpha (0.95f), 0, k.getY(), s.accent.withAlpha (0.55f), 0, k.getBottom(), false));
            g.fillRect (k.reduced (1.5f, 0));
            g.setColour (Colours::white.withAlpha (0.5f)); g.fillRect (k.reduced (k.getWidth() * 0.35f, 2).withHeight (k.getHeight() * 0.5f));
        }
        else if (isOver) { g.setColour (s.accent.withAlpha (0.18f)); g.fillRect (k); }
        if (isRoot (note)) { g.setColour (s.accent); g.fillEllipse (Rectangle<float> (6, 6).withCentre ({ k.getCentreX(), k.getBottom() - 14 })); }
    }
    void drawBlackNote (int note, Graphics& g, Rectangle<float> a, bool isDown, bool isOver, Colour) override
    {
        const auto& s = *lnf.skin;
        const bool on = isDown || proc.playing[(size_t) note].load();
        auto k = a.withTrimmedTop (-2);
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (k.translated (1.5f, 2), 2);
        g.drawImage (skinImages (skin).black, k, RectanglePlacement::stretchToFit);
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
    KeysKillaProcessor& proc; KKLookAndFeel& lnf; const int& skin;
};

//==============================================================================
// Label drawn over the baked macro caption only when it differs (bass mode / preset names)
class MacroCaption : public Component
{
public:
    MacroCaption (KKLookAndFeel& l, const int& s) : lnf (l), skin (s) { setInterceptsMouseClicks (false, false); }
    String text; bool custom = false; Point<int> designPos;
    void paint (Graphics& g) override
    {
        if (! custom) return;
        const auto& sk = *lnf.skin;
        const auto& bg = skinImages (skin).bg;
        const Colour fill = bg.getPixelAt (designPos.x - getWidth() / 2 + 4, designPos.y);
        g.setColour (fill); g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (2), 4);
        g.setColour (sk.dark ? Colour (0xffe6e1e1) : Colour (0xff15181c));
        g.setFont (serif ((float) getHeight() * 0.62f, ! sk.dark, 0.26f));
        g.drawText (text, getLocalBounds(), Justification::centred);
    }
private:
    KKLookAndFeel& lnf; const int& skin;
};

#include "AdvancedPage.h"
#include "PresetBrowser.h"

//==============================================================================
// CHORD / ARP settings panel (opened from the link icon between the buttons)
class PlayPanel : public Component
{
public:
    PlayPanel (KeysKillaProcessor& p, KKLookAndFeel& l)
        : lnf (l), grid (p, { ID::chord, ID::chordType, ID::strum, ID::keyLock, ID::key, ID::scale,
                              ID::arp, ID::arpRate, ID::arpMode, ID::arpOct, ID::arpGate, ID::arpSwing,
                              ID::timeM, ID::alive, ID::drift, ID::punch, ID::halftime, ID::glide }, 6)
    {
        addAndMakeVisible (grid);
        close.setButtonText ("CLOSE");
        close.onClick = [this] { setVisible (false); };
        addAndMakeVisible (close);
    }
    void paint (Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat().reduced (4), *lnf.skin, "PERFORM / CHORD / ARP");
        g.setColour (lnf.skin->dark ? Colour (0xf0080606) : Colour (0xf0e3e6ea));
        g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (6).withTrimmedTop (34), 6);
    }
    void resized() override
    {
        close.setBounds (getWidth() - 100, 12, 84, 26);
        grid.setBounds (getLocalBounds().reduced (16).withTrimmedTop (34));
    }
private:
    KKLookAndFeel& lnf;
    ParamGrid grid;
    TextButton close;
};

//==============================================================================
class MainPage : public Component, private Timer
{
public:
    explicit MainPage (KeysKillaProcessor& p) : proc (p), era (lnf, skinIndex), subChips (lnf), xy (p, lnf), chaos (lnf),
                                                pitchWheel (lnf), modWheel (lnf), meter (lnf), keyboard (p, lnf, skinIndex)
    {
        settings = openSettings();
        skinIndex = jlimit (0, 1, settings->getIntValue ("skin", 1));
        lnf.setSkin (Skin::all()[(size_t) skinIndex]);
        setLookAndFeel (&lnf);

        for (int i = 0; i < numTiles; ++i)
        {
            auto t = std::make_unique<HotButton> (lnf, HotButton::tile);
            t->setTooltip ("Browse " + tileNames()[i].toLowerCase() + " presets.");
            t->onClick = [this, i] { proc.uiTile = i; proc.uiExclusive = false; proc.uiSub = -1; subChips.selected = -1; loadFirstMatching(); };
            addAndMakeVisible (*t);
            tiles.push_back (std::move (t));
        }
        era.selected = proc.uiEra;
        era.onSelect = [this] (int e) { proc.uiEra = e; loadFirstMatching(); };
        addAndMakeVisible (era);
        subChips.selected = proc.uiSub;
        subChips.onSelect = [this] (int sc) { proc.uiSub = sc; if (sc >= 0) { proc.uiTile = -1; proc.uiExclusive = false; } loadFirstMatching(); };
        addAndMakeVisible (subChips);

        exclusiveBtn.setTooltip ("Signature sounds built on the exclusive engine features.");
        exclusiveBtn.setClickingTogglesState (false);
        exclusiveBtn.onClick = [this] { proc.uiExclusive = ! proc.uiExclusive; if (proc.uiExclusive) { proc.uiTile = -1; proc.uiSub = -1; subChips.selected = -1; } loadFirstMatching(); };
        addAndMakeVisible (exclusiveBtn);

        // top bar
        prevBtn.onClick = [this] { step (-1); }; prevBtn.setTooltip ("Previous preset");
        nextBtn.onClick = [this] { step (1); };  nextBtn.setTooltip ("Next preset");
        saveBtn.onClick = [this] { savePreset(); }; saveBtn.setTooltip ("Save your sound as a user preset.");
        menuBtn.onClick = [this] { showMenu(); };  menuBtn.setTooltip ("Presets, A/B, undo, ADVANCED page, skin and size.");
        nameBtn.onClick = [this] { openBrowser(); }; nameBtn.setTooltip ("Click to browse and search all presets.");
        heartBtn.onClick = [this] { toggleFavourite(); }; heartBtn.setTooltip ("Add to favourites");
        heartBtn.glyph = [this] (Graphics& g, Rectangle<float> hb, const Skin& s)
        {
            if (! isFav) return;
            hb = hb.reduced (7, 9);
            Path heart;
            heart.startNewSubPath (hb.getCentreX(), hb.getBottom());
            heart.cubicTo (hb.getX() - 3, hb.getCentreY(), hb.getX() + 2, hb.getY() - 3, hb.getCentreX(), hb.getY() + hb.getHeight() * 0.3f);
            heart.cubicTo (hb.getRight() - 2, hb.getY() - 3, hb.getRight() + 3, hb.getCentreY(), hb.getCentreX(), hb.getBottom());
            g.setColour (s.accent.withAlpha (0.35f)); g.strokePath (heart, PathStrokeType (5.0f));
            g.setColour (s.accent); g.fillPath (heart);
        };
        nameBtn.glyph = [this] (Graphics& g, Rectangle<float> r, const Skin& s)
        {
            g.setColour (s.dark ? Colour (0xffece6e6) : Colour (0xff15181c));
            g.setFont (serif (r.getHeight() * 0.5f, false, 0.14f));
            g.drawFittedText (proc.currentName() + (modified ? " *" : ""), r.reduced (10, 0).toNearestInt(), Justification::centred, 1, 0.7f);
        };
        skinBtn.setTooltip ("Switch light (CHROME) / dark (BLOOD) skin.");
        skinBtn.onClick = [this] { setSkin (skinIndex == 0 ? 1 : 0); };
        skinBtn.glyph = [] (Graphics& g, Rectangle<float> r, const Skin& s)
        {
            auto c = r.reduced (2);
            g.setColour (s.dark ? Colour (0xdd100c0c) : Colour (0xddf0f2f4)); g.fillEllipse (c);
            g.setColour (s.dark ? Colour (0x88ffffff) : Colour (0x88202428)); g.drawEllipse (c, 1.2f);
            auto sb = c.reduced (c.getWidth() * 0.27f);
            g.setColour (s.dark ? Colour (0xffe8e2e2) : Colour (0xff1a1d21));
            if (s.dark) { g.fillEllipse (sb); g.setColour (Colour (0xff100c0c)); g.fillEllipse (sb.translated (sb.getWidth() * 0.38f, -sb.getHeight() * 0.22f)); }
            else
            {
                g.fillEllipse (sb.reduced (sb.getWidth() * 0.25f));
                const auto cc = sb.getCentre(); const float rr = sb.getWidth() * 0.5f;
                for (int i = 0; i < 8; ++i)
                {
                    const float a = MathConstants<float>::twoPi * (float) i / 8.0f;
                    g.drawLine (cc.x + std::sin (a) * rr * 0.62f, cc.y - std::cos (a) * rr * 0.62f, cc.x + std::sin (a) * rr, cc.y - std::cos (a) * rr, 1.5f);
                }
            }
        };
        for (Component* c : { (Component*) &prevBtn, (Component*) &nextBtn, (Component*) &saveBtn, (Component*) &menuBtn,
                              (Component*) &nameBtn, (Component*) &heartBtn, (Component*) &skinBtn })
            addAndMakeVisible (c);

        // macros + exclusive knobs
        const char* macroIds[] { ID::m1, ID::m2, ID::m3, ID::m4, ID::m5, ID::m6 };
        for (int i = 0; i < 6; ++i)
        {
            auto k = std::make_unique<ImageKnob> (lnf, skinIndex);
            auto* prm = proc.apvts.getParameter (macroIds[i]);
            k->setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
            attachments.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, macroIds[i], *k));
            addAndMakeVisible (*k);
            macros.push_back (std::move (k));
            auto cap = std::make_unique<MacroCaption> (lnf, skinIndex);
            addAndMakeVisible (*cap);
            captions.push_back (std::move (cap));
        }
        const char* exIds[] { ID::ghost, ID::bend, ID::circuit };
        const char* exTips[] { "GHOST: reversed, octave-shifted shadow layer with a haunted tail.",
                               "BEND: melody bends - dive, rise, dip, octave jump or random (mode in ADVANCED > EXCLUSIVE).",
                               "CIRCUIT: tempo-synced broken-electronics glitches." };
        for (int i = 0; i < 3; ++i)
        {
            auto k = std::make_unique<ImageKnob> (lnf, skinIndex);
            k->setTooltip (exTips[i]);
            k->setDoubleClickReturnValue (true, 0.0);
            attachments.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, exIds[i], *k));
            addAndMakeVisible (*k);
            smallKnobs.push_back (std::move (k));
        }

        chaos.setTooltip ("CHAOS: how far DICE moves away from the current sound.");
        attachments.push_back (std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, ID::chaos, chaos));
        addAndMakeVisible (chaos);
        diceBtn.setTooltip ("DICE: roll a brand-new sound in this category. Right-click: undo, history, locks, save.");
        diceBtn.onClick = [this] { proc.rollDice (proc.uiTile >= 0 ? proc.uiTile : tileOfCurrent()); proc.captureUndo(); };
        diceBtn.onRightClick = [this] { showDiceMenu(); };
        addAndMakeVisible (diceBtn);
        addAndMakeVisible (xy);

        chordBtn.setClickingTogglesState (true); arpBtn.setClickingTogglesState (true);
        chordBtn.setTooltip ("CHORD: one key plays a trap chord. Right-click or the link icon for chord type, strum and key lock.");
        arpBtn.setTooltip ("ARP: tempo-synced arpeggiator. Right-click or the link icon for rate, mode, octaves, gate and swing.");
        chordAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ID::chord, chordBtn);
        arpAtt = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, ID::arp, arpBtn);
        chordBtn.onRightClick = [this] { openPlayPanel(); };
        arpBtn.onRightClick = [this] { openPlayPanel(); };
        linkBtn.setTooltip ("CHORD / ARP settings");
        linkBtn.onClick = [this] { openPlayPanel(); };
        for (Component* c : { (Component*) &chordBtn, (Component*) &arpBtn, (Component*) &linkBtn, (Component*) &meter }) addAndMakeVisible (c);
        meter.setInterceptsMouseClicks (false, false);

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

    ~MainPage() override { setLookAndFeel (nullptr); }

    int preferredScale() const { return jlimit (50, 100, settings->getIntValue ("scale", 60)); }

    void showView (int v)   // used by tests / screenshots: 0 main, 1..8 advanced tab, 9 browser, 10 chord/arp panel
    {
        if (v >= 1 && v <= 8) { ensureAdvanced(); advanced->showTab (v - 1); advanced->setVisible (true); advanced->toFront (false); }
        if (v == 9) openBrowser();
        if (v == 10) openPlayPanel();
    }

    void paint (Graphics& g) override
    {
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImageAt (skinImages (skinIndex).bg, 0, 0);
    }

    void resized() override
    {
        const auto& G = geo (skinIndex);
        prevBtn.setBounds (G.prev); nextBtn.setBounds (G.next); nameBtn.setBounds (G.name); heartBtn.setBounds (G.heart);
        saveBtn.setBounds (G.save); menuBtn.setBounds (G.menu); skinBtn.setBounds (G.skin);
        for (int i = 0; i < numTiles; ++i) tiles[(size_t) i]->setBounds (G.tiles[(size_t) i]);
        subChips.setBounds (G.subBar);
        auto eraBounds = Rectangle<int> (G.era[0].x - 40, G.era[0].y - 42, G.era[5].x - G.era[0].x + 80, 56);
        era.origin = eraBounds.getPosition();
        era.setBounds (eraBounds);
        exclusiveBtn.setBounds (G.exclusive);
        for (int i = 0; i < 6; ++i)
        {
            macros[(size_t) i]->place (G.macro[(size_t) i], G.macroCap, G.macroArc);
            captions[(size_t) i]->designPos = { G.macro[(size_t) i].x, G.macroLabelY };
            captions[(size_t) i]->setBounds (G.macro[(size_t) i].x - 70, G.macroLabelY - 16, 140, 32);
        }
        for (int i = 0; i < 3; ++i) smallKnobs[(size_t) i]->place (G.small[(size_t) i], G.smallCap, G.smallArc);
        diceBtn.setBounds (G.dice); chaos.setBounds (G.chaos); xy.setBounds (G.xy);
        chordBtn.setBounds (G.chord); arpBtn.setBounds (G.arp); linkBtn.setBounds (G.link);
        meter.setBounds (G.meter);
        pitchWheel.setBounds (G.pitch); modWheel.setBounds (G.mod);
        keyboard.setBounds (G.keyboard);
        keyboard.setKeyWidth ((float) G.keyboard.getWidth() / 49.0f);
        if (advanced) advanced->setBounds (getLocalBounds().reduced (30));
        if (browser) browser->setBounds (getLocalBounds().reduced (30).withTrimmedBottom (170));
        if (playPanel) playPanel->setBounds (Rectangle<int> (40, 330, 1080, 470));
    }

private:
    // ---------------- panels (created on first use -> fast editor open/close) ----------------
    void ensureAdvanced()
    {
        if (advanced) return;
        advanced = std::make_unique<AdvancedPage> (proc, lnf, skinIndex);
        advanced->onSkin = [this] (int sk) { setSkin (sk); };
        advanced->onSize = [this] (int pct) { setScale (pct); };
        addChildComponent (*advanced);
        noFocus (*advanced);
        resized();
    }
    void openBrowser()
    {
        if (! browser)
        {
            browser = std::make_unique<PresetBrowser> (proc, lnf);
            browser->getFavourites = [this] { return favourites(); };
            browser->toggleFavourite = [this] (const String& n) { toggleFavouriteNamed (n); };
            browser->onChanged = [this] { refreshState(); };
            addChildComponent (*browser);
            noFocus (*browser);
            resized();
        }
        browser->open (proc.uiExclusive ? -1 : proc.uiTile, proc.uiEra, proc.uiExclusive);
    }
    void openPlayPanel()
    {
        if (! playPanel)
        {
            playPanel = std::make_unique<PlayPanel> (proc, lnf);
            addChildComponent (*playPanel);
            noFocus (*playPanel);
            resized();
        }
        playPanel->setVisible (! playPanel->isVisible());
        playPanel->toFront (false);
    }

    void setSkin (int idx)
    {
        skinIndex = jlimit (0, 1, idx);
        lnf.setSkin (Skin::all()[(size_t) skinIndex]);
        settings->setValue ("skin", skinIndex);
        resized();
        std::function<void (Component&)> rp = [&] (Component& c) { c.repaint(); for (auto* ch : c.getChildren()) rp (*ch); };
        rp (*this);
        sendLookAndFeelChange();
    }
    void setScale (int pct)
    {
        settings->setValue ("scale", pct);
        if (auto* ed = findParentComponentOfClass<AudioProcessorEditor>())
            ed->setSize (KeysKillaEditor::designW * pct / 100, KeysKillaEditor::designH * pct / 100);
    }

    // ---------------- browsing ----------------
    std::vector<int> filtered() const
    {
        std::vector<int> out;
        const auto& ps = factoryPresets();
        for (int i = 0; i < (int) ps.size(); ++i)
        {
            const auto& p = ps[(size_t) i];
            if (proc.uiExclusive && ! p.exclusive) continue;
            if (proc.uiSub >= 0 && p.sub != subCats[(size_t) proc.uiSub].sub) continue;
            if (! proc.uiExclusive && proc.uiSub < 0 && proc.uiTile >= 0 && p.tile != proc.uiTile) continue;
            if (proc.uiEra >= 0 && p.era != proc.uiEra) continue;
            out.push_back (i);
        }
        return out;
    }
    int tileOfCurrent() const { const int i = proc.currentPresetIndex(); return i >= 0 ? factoryPresets()[(size_t) i].tile : tLeads; }
    void loadFirstMatching()
    {
        auto list = filtered();
        if (list.empty() && proc.uiEra >= 0) { const int keep = proc.uiEra; proc.uiEra = -1; list = filtered(); proc.uiEra = keep; }
        if (! list.empty()) proc.loadPreset (list.front());
        proc.captureUndo();
        refreshState();
    }
    void step (int dir)
    {
        auto list = filtered();
        if (list.empty()) { list.resize (factoryPresets().size()); std::iota (list.begin(), list.end(), 0); }
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
        m.addItem (14, "Undo last DICE roll", ! proc.diceHistoryNames().isEmpty());
        m.addSeparator();
        m.addItem (15, "ADVANCED page...");
        m.addItem (17, "PERFORM / CHORD / ARP...");
        m.addItem (16, "Eco mode (lower CPU)", true, proc.eco.load());
        m.addItem (18, "PANIC (all notes off)");
        m.addItem (10, "Skin: CHROME (light)", true, skinIndex == 0);
        m.addItem (11, "Skin: BLOOD (dark)", true, skinIndex == 1);
        for (int pct : { 50, 60, 70, 85, 100 }) size.addItem (100 + pct, String (pct) + " %", true, preferredScale() == pct);
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
                case 7: openBrowser(); break;
                case 8: proc.undo(); break;
                case 9: proc.redo(); break;
                case 12: proc.switchAB(); break;
                case 13: proc.copyAtoB(); break;
                case 14: proc.undoDice(); break;
                case 15: ensureAdvanced(); advanced->setVisible (true); advanced->toFront (false); break;
                case 17: openPlayPanel(); break;
                case 16: proc.eco = ! proc.eco.load(); break;
                case 18: proc.panic(); break;
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
                default: if (r > 100) setScale (r - 100); break;
            }
            refreshState();
        });
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
        m.addSeparator();
        m.addSectionHeader ("MUTATE (roll with fixed amount)");
        m.addItem (301, "Mutate 5 %");
        m.addItem (302, "Mutate 15 %");
        m.addItem (303, "Mutate 30 %");
        m.addItem (304, "Mutate 60 %");
        m.addItem (305, "CHAOS 100 %");
        m.addItem (3, "BREED with a preset...  (right-click a preset in the browser)");
        m.addSeparator();
        m.addItem (2, "Save this sound as preset...");
        m.showMenuAsync (PopupMenu::Options().withTargetComponent (diceBtn), [this] (int r)
        {
            if (r > 300 && r <= 305)
            {
                static const float amt[] { 0.05f, 0.15f, 0.3f, 0.6f, 1.0f };
                if (auto* c = proc.apvts.getParameter (ID::chaos)) c->setValueNotifyingHost (c->convertTo0to1 (amt[r - 301]));
                proc.rollDice (proc.uiTile >= 0 ? proc.uiTile : tileOfCurrent());
                proc.captureUndo();
            }
            else if (r == 3) openBrowser();
            else if (r == 1) proc.undoDice();
            else if (r == 2) savePresetAs();
            else if (r >= 200) proc.diceLocks[(size_t) (r - 200)] = ! proc.diceLocks[(size_t) (r - 200)];
            else if (r >= 100) proc.restoreDice (r - 100);
            refreshState();
        });
    }

    void refreshState()
    {
        const int cur = proc.currentPresetIndex();
        const int shownTile = proc.uiTile >= 0 ? proc.uiTile : (cur >= 0 && ! proc.uiExclusive && proc.uiSub < 0 ? factoryPresets()[(size_t) cur].tile : -1);
        for (int i = 0; i < numTiles; ++i) { tiles[(size_t) i]->selected = (i == shownTile) && ! proc.uiExclusive; tiles[(size_t) i]->repaint(); }
        exclusiveBtn.setToggleState (proc.uiExclusive, dontSendNotification);
        era.selected = proc.uiEra; era.repaint();
        subChips.selected = proc.uiSub; subChips.repaint();
        isFav = favourites().contains (proc.currentName());
        heartBtn.repaint(); nameBtn.repaint();

        const bool bass = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f;
        static const char* normal[] { "TONE", "SPACE", "DIRT", "LOFI", "MOVE", "WIDTH" };
        static const char* bassL[] { "SUB", "WOBBLE", "DIRT", "GLIDE", "TONE", "PUNCH" };
        static const char* tipsN[] { "Darker / brighter", "Reverb and delay amount", "Distortion and saturation", "Tape wobble, vinyl and bitcrush",
                                     "Filter movement, chorus and vibrato", "Stereo width and detune" };
        static const char* tipsB[] { "Clean sine sub layer an octave down", "Tempo-synced wobble (target in ADVANCED > MOD)", "Distortion above the low end only",
                                     "Slide time between notes", "Darker / brighter", "Punchy pitch click on the attack" };
        const auto custom = proc.macroNames();
        for (int i = 0; i < 6; ++i)
        {
            const String t = custom.size() == 6 ? custom[i] : String (bass ? bassL[i] : normal[i]);
            captions[(size_t) i]->text = t;
            captions[(size_t) i]->custom = t != normal[i];
            captions[(size_t) i]->repaint();
            macros[(size_t) i]->setTooltip (bass ? tipsB[i] : tipsN[i]);
        }
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

        const bool mouseDown = ModifierKeys::currentModifiers.isAnyMouseButtonDown();
        if (++slowTick % 10 == 0)
        {
            modifiedNow = proc.isModified();
            if (! mouseDown) proc.captureUndo();
        }
        const bool bassNow = proc.apvts.getRawParameterValue (ID::bassMode)->load() > 0.5f;
        if (proc.currentName() != lastName || proc.currentPresetIndex() != lastIndex || modifiedNow != modified || bassNow != lastBass)
        {
            lastName = proc.currentName(); lastIndex = proc.currentPresetIndex(); modified = modifiedNow; lastBass = bassNow;
            refreshState();
        }
        uint64_t hash = 0;
        for (size_t i = 0; i < 128; ++i) if (proc.playing[i].load()) hash = hash * 131 + i + 1;
        const float lockHash = proc.apvts.getRawParameterValue (ID::keyLock)->load() * 1000 + proc.apvts.getRawParameterValue (ID::key)->load() * 10
                             + proc.apvts.getRawParameterValue (ID::scale)->load();
        if (hash != lastPlayHash || lockHash != lastLockHash) { lastPlayHash = hash; lastLockHash = lockHash; keyboard.repaint(); }
    }

    SkinCacheHolder skinCache;   // first member: images outlive every component that paints them
    KeysKillaProcessor& proc;
    KKLookAndFeel lnf;
    std::unique_ptr<PropertiesFile> settings;
    int skinIndex = 1;

    std::vector<std::unique_ptr<HotButton>> tiles;
    EraOverlay era;
    SubChips subChips;
    HotButton exclusiveBtn { lnf, HotButton::toggleFace, "EXCLUSIVE" };
    HotButton prevBtn { lnf, HotButton::plain }, nextBtn { lnf, HotButton::plain }, saveBtn { lnf, HotButton::plain }, menuBtn { lnf, HotButton::plain };
    HotButton nameBtn { lnf, HotButton::plain }, heartBtn { lnf, HotButton::plain }, skinBtn { lnf, HotButton::plain }, diceBtn { lnf, HotButton::plain };
    HotButton chordBtn { lnf, HotButton::toggleFace, "CHORD" }, arpBtn { lnf, HotButton::toggleFace, "ARP" }, linkBtn { lnf, HotButton::plain };
    std::vector<std::unique_ptr<ImageKnob>> macros, smallKnobs;
    std::vector<std::unique_ptr<MacroCaption>> captions;
    XYOverlay xy;
    ChaosSlider chaos;
    WheelSlider pitchWheel, modWheel;
    MeterOverlay meter;
    KKKeyboard keyboard;
    std::unique_ptr<AdvancedPage> advanced;
    std::unique_ptr<PresetBrowser> browser;
    std::unique_ptr<PlayPanel> playPanel;
    std::unique_ptr<FileChooser> chooser;

    bool isFav = false, modified = false, modifiedNow = false, lastBass = false;
    int warnHold = 0, lastIndex = -2, slowTick = 0;
    String lastName;
    uint64_t lastPlayHash = 0;
    float lastLockHash = -1;

    // attachments last: they must be destroyed before the controls they point to
    std::vector<std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>> attachments;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> chordAtt, arpAtt;
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

void KeysKillaEditor::resized()
{
    page->setTransform (AffineTransform::scale ((float) getWidth() / designW));
    page->setBounds (0, 0, designW, designH);
}
