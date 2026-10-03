#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

// v0.34: the BREED LAB page drawn in code - milky frosted glass over a faint see-through circuit board.
// GLASS (day): light milky glass, dark text.  NIGHT: dark smoked glass, light text.  One amber accent.
// Every live control sits where it did (same 1672 x 941 design).
namespace kk
{
namespace modern
{
using namespace juce;

inline Font font (float size, bool bold = true, float kern = 0.1f)
{
    return Font (FontOptions (Font::getDefaultSansSerifFontName(), size, bold ? Font::bold : Font::plain)).withExtraKerningFactor (kern);
}

//==============================================================================
// the circuit board: thin traces, bus lines, pads and a few chips - drawn once per theme
inline Image makeCircuit (int W, int H, Colour ink, Colour lit, uint32 seed)
{
    Image img (Image::ARGB, W, H, true);
    Graphics g (img);
    Random rnd ((int64) seed);
    const float grid = 12.0f;
    auto snap = [grid] (float v) { return std::round (v / grid) * grid; };
    auto pad = [&g] (Point<float> p, float r, Colour c) { g.setColour (c); g.drawEllipse (p.x - r, p.y - r, r * 2, r * 2, 1.0f); g.fillEllipse (p.x - r * 0.4f, p.y - r * 0.4f, r * 0.8f, r * 0.8f); };
    // a trace: straight, one 45 degree bend, straight again
    auto trace = [&] (Point<float> a, float len1, float len2, int dir, int bend, Colour c, float w)
    {
        static const Point<float> dirs[] { { 1, 0 }, { 0, 1 }, { -1, 0 }, { 0, -1 } };
        const auto d1 = dirs[dir & 3];
        const auto b = a + d1 * len1;
        const auto d2 = (d1 + dirs[(dir + (bend > 0 ? 1 : 3)) & 3]) * 0.7071f;
        const auto c2 = b + d2 * len2;
        const auto e = c2 + d1 * (len1 * 0.6f);
        Path p; p.startNewSubPath (a); p.lineTo (b); p.lineTo (c2); p.lineTo (e);
        g.setColour (c); g.strokePath (p, PathStrokeType (w, PathStrokeType::mitered, PathStrokeType::rounded));
        pad (a, 2.6f, c); pad (e, 2.6f, c);
    };
    // bus bundles (the long parallel lines of a motherboard)
    for (int k = 0; k < 16; ++k)
    {
        const Point<float> a (snap (rnd.nextFloat() * (float) W), snap (rnd.nextFloat() * (float) H));
        const int n = 4 + rnd.nextInt (6), dir = rnd.nextInt (4), bend = rnd.nextBool() ? 1 : -1;
        const float l1 = 80.0f + rnd.nextFloat() * 260.0f, l2 = 30.0f + rnd.nextFloat() * 90.0f;
        for (int i = 0; i < n; ++i)
        {
            const Point<float> off = (dir & 1) ? Point<float> ((float) i * 6.0f, 0) : Point<float> (0, (float) i * 6.0f);
            trace (a + off, l1 + (float) i * 6.0f * (float) bend, l2, dir, bend, ink, 1.1f);
        }
    }
    // single traces
    for (int k = 0; k < 150; ++k)
        trace ({ snap (rnd.nextFloat() * (float) W), snap (rnd.nextFloat() * (float) H) }, 20.0f + rnd.nextFloat() * 140.0f, 10.0f + rnd.nextFloat() * 50.0f,
               rnd.nextInt (4), rnd.nextBool() ? 1 : -1, ink.withMultipliedAlpha (0.8f), 1.0f);
    // chips: a body and fine pins on two or four sides
    for (int k = 0; k < 14; ++k)
    {
        const float w = 40.0f + rnd.nextFloat() * 90.0f, h = 30.0f + rnd.nextFloat() * 70.0f;
        const Rectangle<float> r (snap (rnd.nextFloat() * ((float) W - w)), snap (rnd.nextFloat() * ((float) H - h)), w, h);
        g.setColour (ink.withMultipliedAlpha (0.35f)); g.fillRoundedRectangle (r, 3.0f);
        g.setColour (ink); g.drawRoundedRectangle (r, 3.0f, 1.0f);
        const bool four = rnd.nextBool();
        for (float x = r.getX() + 6; x < r.getRight() - 4; x += 5) { g.drawLine (x, r.getY() - 5, x, r.getY(), 1.0f); g.drawLine (x, r.getBottom(), x, r.getBottom() + 5, 1.0f); }
        if (four) for (float y = r.getY() + 6; y < r.getBottom() - 4; y += 5) { g.drawLine (r.getX() - 5, y, r.getX(), y, 1.0f); g.drawLine (r.getRight(), y, r.getRight() + 5, y, 1.0f); }
        g.setColour (ink.withMultipliedAlpha (0.6f)); g.drawEllipse (r.getX() + 5, r.getY() + 5, 5, 5, 1.0f);
    }
    // vias
    for (int k = 0; k < 260; ++k) pad ({ snap (rnd.nextFloat() * (float) W), snap (rnd.nextFloat() * (float) H) }, 1.6f, ink.withMultipliedAlpha (0.7f));
    // the signal paths into BREED: two quiet amber lines from each parent
    if (! lit.isTransparent())
        for (int s = 0; s < 2; ++s)
            for (int i = 0; i < 3; ++i)
            {
                const float y = 228.0f + (float) i * 12.0f;
                const float x0 = s == 0 ? 640.0f : 1022.0f, x1 = s == 0 ? 742.0f - (float) i * 4 : 932.0f + (float) i * 4;
                Path p; p.startNewSubPath (x0, y); p.lineTo ((x0 + x1) * 0.5f, y); p.lineTo (x1, 252.0f + ((float) i - 1.0f) * 8.0f);
                g.setColour (lit); g.strokePath (p, PathStrokeType (1.2f));
                pad ({ x0, y }, 2.4f, lit);
            }
    return img;
}

inline Image blurred (const Image& src, int factor = 4, int radius = 5)
{
    Image small (Image::ARGB, src.getWidth() / factor, src.getHeight() / factor, true);
    {
        Graphics g (small);
        g.setImageResamplingQuality (Graphics::mediumResamplingQuality);
        g.drawImage (src, small.getBounds().toFloat(), RectanglePlacement::stretchToFit);
    }
    ImageConvolutionKernel k (radius * 2 + 1);
    k.createGaussianBlur ((float) radius * 0.8f);
    k.applyToImage (small, small, small.getBounds());
    return small;
}

//==============================================================================
// the background layers every page shares: base gradient + sharp circuit, and the frosted (blurred) circuit for glass
struct Backdrop { Image sharp, soft; };
// owned by the editor's image cache (images must die with the editor - static images froze FL Studio on exit)
inline Backdrop*& activeBackdrop() { static Backdrop* b = nullptr; return b; }
inline Backdrop makeBackdrop()
{
    const auto& th = theme();
    Backdrop b;
    b.sharp = makeCircuit (1672, 941, th.trace.withAlpha (th.night ? 0.22f : 0.30f), th.accent.withAlpha (th.night ? 0.35f : 0.45f), 20260934);
    b.soft = blurred (b.sharp);
    return b;
}

inline void base (Graphics& g, Rectangle<float> area)
{
    const auto& t = theme();
    g.setGradientFill (ColourGradient (t.baseTop, 0, area.getY(), t.baseBottom, 0, area.getBottom(), false));
    g.fillRect (area);
    // a soft light from the top centre, like a lamp over the desk
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (t.night ? 0.035f : 0.28f), area.getCentreX(), area.getY(),
                                       Colours::white.withAlpha (0.0f), area.getCentreX(), area.getY() + area.getHeight() * 0.75f, true));
    g.fillRect (area);
}

// frosted glass: the circuit behind it goes soft, milky tint, a bright top edge and a quiet border
inline void plate (Graphics& g, Rectangle<float> r, float corner = 12.0f, bool accentEdge = false)
{
    const auto& t = theme();
    const auto* bd = activeBackdrop();
    for (int k = 3; k >= 1; --k)   // soft shadow
    {
        g.setColour (t.shadow.withAlpha ((t.night ? 0.16f : 0.035f) * (float) k));
        g.fillRoundedRectangle (r.translated (0, (float) (5 - k)).expanded ((float) (4 - k)), corner + (float) (4 - k));
    }
    {
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (r, corner);
        g.reduceClipRegion (clip);
        g.setImageResamplingQuality (Graphics::mediumResamplingQuality);
        g.setOpacity (1.0f);
        if (bd != nullptr) g.drawImage (bd->soft, Rectangle<float> (0, 0, 1672, 941), RectanglePlacement::stretchToFit);
        g.setColour (t.glass); g.fillRoundedRectangle (r, corner);
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (t.night ? 0.05f : 0.30f), 0, r.getY(), Colours::white.withAlpha (0.0f), 0, r.getY() + std::min (60.0f, r.getHeight() * 0.5f), false));
        g.fillRoundedRectangle (r, corner);
    }
    g.setColour (accentEdge ? t.accent.withAlpha (0.75f) : t.glassEdge); g.drawRoundedRectangle (r.reduced (0.5f), corner, accentEdge ? 1.4f : 1.0f);
    g.setColour (t.glassHi); g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + corner, r.getRight() - corner);
}

// a recessed well (meters, slots)
inline void well (Graphics& g, Rectangle<float> r, float corner)
{
    const auto& t = theme();
    g.setColour (t.well); g.fillRoundedRectangle (r, corner);
    g.setColour (t.night ? Colours::black.withAlpha (0.5f) : Colour (0x40505a66)); g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.0f);
}

// the old neon light waves are gone (calm look) - kept as a no-op for the pages that still call it
inline void waves (Graphics&, Point<float>, Point<float>, float, Colour, Colour, int, float) {}

inline void label (Graphics& g, const String& t, Rectangle<float> r, float size = 12.0f, Colour c = {}, Justification j = Justification::centred)
{
    g.setColour (c.isTransparent() ? theme().dim : c); g.setFont (font (size, true, 0.14f)); g.drawText (t, r, j);
}

// knob cap: the knob class cuts it out of this picture and turns it (pointer up = middle)
inline void knobCap (Graphics& g, Point<float> c, float r, float arcR)
{
    const auto& t = theme();
    for (int i = 0; i <= 10; ++i)   // scale ticks
    {
        const float a = MathConstants<float>::pi * (-0.75f + 1.5f * (float) i / 10.0f) - MathConstants<float>::halfPi;
        g.setColour (t.dim.withAlpha (0.55f));
        g.fillEllipse (c.x + std::cos (a) * (arcR + 7) - 1.2f, c.y + std::sin (a) * (arcR + 7) - 1.2f, 2.4f, 2.4f);
    }
    Path track; track.addCentredArc (c.x, c.y, arcR, arcR, 0, -MathConstants<float>::pi * 0.75f, MathConstants<float>::pi * 0.75f, true);
    g.setColour (t.night ? Colours::black.withAlpha (0.55f) : Colour (0x30505a66)); g.strokePath (track, PathStrokeType (4.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (t.shadow.withAlpha (t.night ? 0.6f : 0.22f)); g.fillEllipse (Rectangle<float> (r * 2 + 6, r * 2 + 6).withCentre (c.translated (0, 3)));
    // brushed metal cap
    g.setGradientFill (ColourGradient (t.knobTop, c.x, c.y - r, t.knobBottom, c.x, c.y + r, false));
    g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c));
    g.setGradientFill (ColourGradient (t.knobTop.brighter (0.05f), c.x - r * 0.5f, c.y - r * 0.7f, t.knobBottom.interpolatedWith (t.knobTop, 0.45f), c.x + r * 0.4f, c.y + r * 0.8f, false));
    g.fillEllipse (Rectangle<float> (r * 1.7f, r * 1.7f).withCentre (c));
    g.setColour (Colours::white.withAlpha (t.night ? 0.10f : 0.7f)); g.drawEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c).reduced (0.5f), 1.0f);
    g.setColour (t.night ? Colours::white.withAlpha (0.92f) : t.text.withAlpha (0.85f));
    g.drawLine (c.x, c.y - r * 0.34f, c.x, c.y - r * 0.84f, std::max (2.0f, r * 0.08f));
}
inline void chevron (Graphics& g, Rectangle<float> r, bool left)
{
    Path p; const auto c = r.getCentre(); const float s = r.getHeight() * 0.16f;
    p.startNewSubPath (c.x + (left ? s : -s), c.y - s * 1.6f); p.lineTo (c.x + (left ? -s : s), c.y); p.lineTo (c.x + (left ? s : -s), c.y + s * 1.6f);
    g.setColour (theme().text.withAlpha (0.85f)); g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
}
inline void dice (Graphics& g, Rectangle<float> r)
{
    const auto& t = theme();
    g.setColour (t.text.withAlpha (0.55f)); g.drawRoundedRectangle (r.reduced (3), 6, 1.3f);
    g.setColour (t.text.withAlpha (0.8f));
    for (auto pt : { Point<float> (0.33f, 0.33f), Point<float> (0.67f, 0.33f), Point<float> (0.5f, 0.5f), Point<float> (0.33f, 0.67f), Point<float> (0.67f, 0.67f) })
        g.fillEllipse (r.getX() + r.getWidth() * pt.x - 2.0f, r.getY() + r.getHeight() * pt.y - 2.0f, 4, 4);
}
inline void button (Graphics& g, Rectangle<float> r, const String& txt, float size = 12.0f)
{
    plate (g, r, 8.0f);
    if (txt.isNotEmpty()) label (g, txt, r, size, theme().text);
}
inline void moon (Graphics& g, Point<float> c, Colour col, Colour cutCol)
{
    Path m; m.addEllipse (c.x - 9, c.y - 9, 18, 18);
    g.setColour (col); g.fillPath (m);
    g.setColour (cutCol); g.fillEllipse (c.x - 4, c.y - 13, 18, 18);
}
inline void sun (Graphics& g, Point<float> c, Colour col)
{
    g.setColour (col); g.fillEllipse (c.x - 5.5f, c.y - 5.5f, 11, 11);
    for (int i = 0; i < 8; ++i)
    {
        const float a = MathConstants<float>::twoPi * (float) i / 8.0f;
        g.drawLine (c.x + std::cos (a) * 8.5f, c.y + std::sin (a) * 8.5f, c.x + std::cos (a) * 12.0f, c.y + std::sin (a) * 12.0f, 1.6f);
    }
}

// spaced wordmark "K E Y S  K I L L A"
inline void wordmark (Graphics& g, Rectangle<float> r, float size)
{
    const auto& t = theme();
    const auto f1 = font (size, false, 0.55f), f2 = font (size, true, 0.55f);
    GlyphArrangement a; a.addLineOfText (f1, "KEYS", 0, 0);
    const float w1 = a.getBoundingBox (0, -1, true).getWidth();
    g.setColour (t.text); g.setFont (f1); g.drawText ("KEYS", r, Justification::centredLeft);
    g.setFont (f2); g.drawText ("KILLA", r.withTrimmedLeft (w1 + size * 0.7f), Justification::centredLeft);
}

//==============================================================================
// the whole page background, 1672 x 941 (the BREED LAB design size)
inline Image makeBackground()
{
    const auto& t = theme();
    Image img (Image::RGB, 1672, 941, true);
    Graphics g (img);
    base (g, { 0, 0, 1672, 941 });
    if (auto* bd = activeBackdrop()) g.drawImageAt (bd->sharp, 0, 0);

    // ---- header: wordmark, preset pill, buttons
    wordmark (g, { 140, 26, 470, 44 }, 34.0f);
    label (g, "DON'T BROWSE SOUNDS, BREED THEM.", { 141, 70, 440, 18 }, 10.5f, t.dim, Justification::centredLeft);
    plate (g, { 616, 30, 520, 52 }, 26.0f);
    chevron (g, { 626, 34, 42, 44 }, true); chevron (g, { 1044, 34, 40, 44 }, false);
    {   // heart
        Path h; const float cx = 1107, cy = 57, s = 8;
        h.startNewSubPath (cx, cy + s);
        h.cubicTo (cx - s * 2.2f, cy - s * 0.3f, cx - s * 0.9f, cy - s * 1.9f, cx, cy - s * 0.6f);
        h.cubicTo (cx + s * 0.9f, cy - s * 1.9f, cx + s * 2.2f, cy - s * 0.3f, cx, cy + s);
        g.setColour (t.accent); g.strokePath (h, PathStrokeType (1.8f));
    }
    button (g, { 1247, 44, 97, 40 }, "SAVE");
    button (g, { 1367, 44, 97, 40 }, "MENU");
    button (g, { 1485, 42, 57, 44 }, "");
    if (t.night) sun (g, { 1513, 64 }, t.text.withAlpha (0.9f));   // NIGHT: the sun takes you back to day
    else moon (g, { 1513, 64 }, t.text.withAlpha (0.85f), Colour (0xffeef0f2));

    // ---- left column (the tiles draw themselves on it)
    plate (g, { 8, 96, 138, 512 }, 14.0f);

    // ---- parents (cards draw their wave / name)
    for (int k = 0; k < 2; ++k)
    {
        const Rectangle<float> r = k == 0 ? Rectangle<float> (214, 128, 428, 248) : Rectangle<float> (1020, 128, 414, 248);
        plate (g, r, 16.0f);
        label (g, k == 0 ? "PARENT A" : "PARENT B", { r.getX() + 24, r.getY() + 12, 200, 22 }, 12.5f, t.dim, Justification::centredLeft);
        dice (g, k == 0 ? Rectangle<float> (584, 147, 36, 36) : Rectangle<float> (1377, 148, 36, 36));
    }
    // ---- BREED socket: the reactor tube draws itself on top
    {
        const Point<float> c (837, 252);
        g.setColour (t.shadow.withAlpha (t.night ? 0.55f : 0.10f)); g.fillEllipse (Rectangle<float> (232, 232).withCentre (c.translated (0, 5)));
        plate (g, Rectangle<float> (224, 224).withCentre (c), 112.0f);
        g.setColour (t.well); g.fillEllipse (Rectangle<float> (196, 196).withCentre (c));
        g.setColour (t.glassEdge); g.drawEllipse (Rectangle<float> (196, 196).withCentre (c), 1.0f);
    }

    // ---- children
    const int cx[6] { 220, 422, 623, 827, 1030, 1233 };
    for (int i = 0; i < 6; ++i)
    {
        const Rectangle<float> r ((float) cx[i] - 2, 385, 194, 128);
        plate (g, r, 12.0f);
        label (g, "CHILD " + String (i + 1), { r.getX(), r.getY() + 8, r.getWidth(), 18 }, 11.0f, t.dim);
        const Point<float> pc ((float) cx[i] + 160, 451);
        g.setColour (t.text.withAlpha (0.45f)); g.drawEllipse (Rectangle<float> (28, 28).withCentre (pc), 1.2f);
        Path tri; tri.addTriangle (pc.x - 3.5f, pc.y - 6, pc.x - 3.5f, pc.y + 6, pc.x + 6.5f, pc.y);
        g.setColour (t.text.withAlpha (0.85f)); g.fillPath (tri);
    }

    // ---- genes, mutate, family tree, undo
    plate (g, { 176, 524, 862, 82 }, 14.0f);
    label (g, "GENES", { 182, 538, 74, 18 }, 11.5f, t.text);
    {   // helix
        Path dna; for (int k = 0; k < 2; ++k) { dna.startNewSubPath (209.0f, 563.0f); for (int i = 0; i <= 20; ++i) { const float u = (float) i / 20.0f; dna.lineTo (219.0f + std::sin (u * 6.283f + (float) k * 3.1416f) * 8.0f, 563.0f + u * 28.0f); } }
        g.setColour (t.accent); g.strokePath (dna, PathStrokeType (1.4f));
    }
    static const char* geneNames[] { "BODY", "ATTACK", "TEXTURE", "SPACE", "MOVEMENT", "CHARACTER" };
    const int gx[6] { 311, 441, 572, 697, 830, 952 };
    for (int i = 0; i < 6; ++i)
    {
        label (g, geneNames[i], { (float) gx[i] - 50, 534, 120, 18 }, 10.5f, t.dim);
        const float x0 = (float) gx[i] - 41;
        well (g, { x0 + 4, 559, 74, 27 }, 13.5f);
        label (g, "A", { x0 + 4, 559, 37, 27 }, 12.0f, t.dim); label (g, "B", { x0 + 41, 559, 37, 27 }, 12.0f, t.dim);
        g.setColour (t.dim); g.drawRoundedRectangle (x0 + 89, 570, 12, 10, 2, 1.2f); g.drawRoundedRectangle (x0 + 91, 564, 8, 9, 3, 1.2f);
    }
    plate (g, { 1042, 524, 254, 82 }, 14.0f);
    label (g, "MUTATE", { 1054, 530, 120, 20 }, 11.5f, t.text, Justification::centredLeft);
    const int mx[5][2] { { 1049, 1094 }, { 1095, 1142 }, { 1142, 1189 }, { 1189, 1236 }, { 1236, 1288 } };
    static const char* mutNames[] { "5%", "15%", "30%", "60%", "CHAOS" };
    for (int i = 0; i < 5; ++i)
    {
        const Rectangle<float> r ((float) mx[i][0] + 2, 557, (float) (mx[i][1] - mx[i][0]) - 4, 36);
        well (g, r, 8.0f); label (g, mutNames[i], r, 10.5f, t.text);
    }
    button (g, { 1300, 528, 74, 76 }, "");
    label (g, "FAMILY", { 1300, 570, 74, 14 }, 10.0f, t.text); label (g, "TREE", { 1300, 584, 74, 14 }, 10.0f, t.text);
    g.setColour (t.accent);
    for (auto p : { Point<float> (1337, 542), Point<float> (1328, 558), Point<float> (1346, 558) }) g.drawEllipse (p.x - 3.5f, p.y - 3.5f, 7, 7, 1.4f);
    g.drawLine (1337, 545.5f, 1329.5f, 555, 1.3f); g.drawLine (1337, 545.5f, 1344.5f, 555, 1.3f);
    button (g, { 1379, 528, 63, 76 }, "");
    label (g, "UNDO", { 1379, 576, 63, 16 }, 10.0f, t.text);
    {
        Path u; u.addCentredArc (1410, 556, 9, 9, 0, -2.6f, 1.6f, true);
        g.setColour (t.text); g.strokePath (u, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        Path ah; ah.addTriangle (1400, 546, 1408, 548, 1402, 554); g.fillPath (ah);
    }

    // ---- right column: drag to DAW + FUTURE / ALIVE / TIME
    plate (g, { 1448, 104, 176, 504 }, 16.0f);
    knobCap (g, { 1532, 447 }, 33, 43); label (g, "FUTURE", { 1482, 494, 100, 18 }, 11.0f, t.text);
    knobCap (g, { 1490, 552 }, 20, 28); label (g, "ALIVE", { 1450, 584, 80, 16 }, 10.0f, t.text);
    knobCap (g, { 1575, 552 }, 20, 28); label (g, "TIME", { 1535, 584, 80, 16 }, 10.0f, t.text);

    // ---- macros, output, wheels, keyboard bed
    plate (g, { 58, 668, 1226, 124 }, 16.0f);
    const int kx[8] { 132, 284, 435, 587, 742, 895, 1047, 1202 };
    static const char* macroNames[] { "DARK", "SPACE", "MOVEMENT", "WIDTH", "TEXTURE", "PUNCH", "DIRT", "MIX" };
    for (int i = 0; i < 8; ++i) { knobCap (g, { (float) kx[i], 715 }, 29, 41); label (g, macroNames[i], { (float) kx[i] - 62, 757, 124, 22 }, 11.0f, t.text); }
    plate (g, { 1300, 668, 326, 124 }, 16.0f);
    label (g, "OUTPUT", { 1318, 678, 120, 22 }, 11.5f, t.text, Justification::centredLeft);
    label (g, "L", { 1318, 714, 16, 16 }, 10.0f, t.dim); label (g, "R", { 1318, 746, 16, 16 }, 10.0f, t.dim);
    well (g, { 1336, 715, 260, 15 }, 4); well (g, { 1336, 747, 260, 15 }, 4);
    static const char* scale[] { "-60", "-36", "-24", "-12", "-6", "0" };
    const float sx[] { 1341, 1393, 1443, 1493, 1543, 1593 };
    for (int i = 0; i < 6; ++i) label (g, scale[i], { sx[i] - 18, 768, 36, 14 }, 9.0f, t.dim);
    plate (g, { 50, 800, 140, 132 }, 14.0f);
    for (float x : { 72.0f, 126.0f }) well (g, { x, 808, 36, 86 }, 8);
    label (g, "PITCH", { 58, 898, 64, 16 }, 10.0f, t.text); label (g, "MOD", { 112, 898, 64, 16 }, 10.0f, t.text);
    plate (g, { 196, 804, 1450, 128 }, 14.0f);
    return img;
}
// full-page backdrop for the other pages (base + circuit, no panels)
inline Image makePageBackdrop()
{
    Image img (Image::RGB, 1672, 941, true);
    Graphics g (img);
    base (g, { 0, 0, 1672, 941 });
    if (auto* bd = activeBackdrop()) g.drawImageAt (bd->sharp, 0, 0);
    return img;
}
// piano keys for the keyboard component (stretched to every key)
inline Image makeWhiteKey()
{
    const auto& t = theme();
    Image img (Image::ARGB, 72, 220, true);
    Graphics g (img);
    auto r = Rectangle<float> (1, 0, 70, 219);
    g.setGradientFill (ColourGradient (t.night ? Colour (0xffe9ebee) : Colour (0xfffbfbfc), 0, 0, t.night ? Colour (0xffc4c8ce) : Colour (0xffdfe2e6), 0, 220, false));
    g.fillRoundedRectangle (r, 5);
    g.setColour (t.night ? Colour (0xff6d737b) : Colour (0xffa5abb3)); g.drawRoundedRectangle (r, 5, 1.0f);
    g.setColour (Colours::white.withAlpha (0.6f)); g.fillRect (4.0f, 0.0f, 2.0f, 210.0f);
    return img;
}
inline Image makeBlackKey()
{
    const auto& t = theme();
    Image img (Image::ARGB, 44, 140, true);
    Graphics g (img);
    auto r = Rectangle<float> (1, 0, 42, 139);
    g.setGradientFill (ColourGradient (t.night ? Colour (0xff2f333a) : Colour (0xff3a3f46), 0, 0, Colour (0xff0d0f12), 0, 140, false));
    g.fillRoundedRectangle (r, 4);
    g.setColour (Colours::white.withAlpha (0.10f)); g.fillRoundedRectangle (r.reduced (5, 3).withTrimmedBottom (14), 3);
    g.setColour (Colours::black); g.drawRoundedRectangle (r, 4, 1.0f);
    return img;
}
} // namespace modern
} // namespace kk
