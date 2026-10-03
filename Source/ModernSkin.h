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
// v0.34b: the BACK layer is a real circuit board (chips with height and shadows, copper traces, capacitors, parts)
// and the FRONT layer is milky, see-through glass floating above it (frosted, thick edges, shadows on the board).
inline void softShadow (Graphics& g, Rectangle<float> r, float corner, Point<float> off, float spread, float alpha)
{
    const int n = 7;
    for (int k = 0; k < n; ++k)
    {
        const float e = spread * (float) (k + 1) / (float) n;
        g.setColour (Colours::black.withAlpha (alpha / (float) n));
        g.fillRoundedRectangle (r.translated (off.x, off.y).expanded (e - spread * 0.35f), corner + e);
    }
}

inline Image makeGrain (uint32 seed, bool night)
{
    Image img (Image::ARGB, 192, 192, true);
    Random rnd ((int64) seed);
    for (int y = 0; y < 192; ++y)
        for (int x = 0; x < 192; ++x)
        {
            const float v = rnd.nextFloat();
            img.setPixelAt (x, y, (v > 0.5f ? Colours::white : Colours::black).withAlpha ((night ? 0.05f : 0.06f) * std::abs (v - 0.5f) * 2.0f));
        }
    return img;
}

struct BoardLook
{
    Colour baseA, baseB, copper, copperHi, chipTop, chipBottom, chipEdge, metal, metalDark, ink, led;
};
inline BoardLook boardLook (bool night)
{
    if (night) return { Colour (0xff15181c), Colour (0xff090a0c), Colour (0xff7a5a33), Colour (0xffc08a4a), Colour (0xff2b2f35), Colour (0xff121417),
                        Colour (0xff3d424a), Colour (0xff8a8780), Colour (0xff3c3b38), Colour (0xff6c727a), Colour (0xffff8a3d) };
    return { Colour (0xff9aa1a9), Colour (0xff747b84), Colour (0xffb08a52), Colour (0xffe2c48e), Colour (0xff3b4047), Colour (0xff1c1f23),
             Colour (0xff59606a), Colour (0xffd9d6cf), Colour (0xff8e8b84), Colour (0xff9aa0a8), Colour (0xffff8a3d) };
}

inline void chip (Graphics& g, Rectangle<float> r, const BoardLook& L, bool night, Random& rnd, bool bga, const String& code)
{
    // pins (gull-wing): first, so the body sits on them
    if (! bga)
    {
        const float pl = 7.0f, pw = 2.2f, gap = 5.5f;
        for (int side = 0; side < 4; ++side)
        {
            const bool horiz = side < 2;
            const float len = horiz ? r.getWidth() : r.getHeight();
            for (float t = 7.0f; t < len - 5.0f; t += gap)
            {
                Rectangle<float> p = side == 0 ? Rectangle<float> (r.getX() + t, r.getY() - pl, pw, pl)
                                   : side == 1 ? Rectangle<float> (r.getX() + t, r.getBottom(), pw, pl)
                                   : side == 2 ? Rectangle<float> (r.getX() - pl, r.getY() + t, pl, pw)
                                               : Rectangle<float> (r.getRight(), r.getY() + t, pl, pw);
                g.setColour (Colours::black.withAlpha (night ? 0.5f : 0.3f)); g.fillRect (p.translated (1.5f, 2.0f));
                g.setGradientFill (ColourGradient (L.metal, p.getX(), p.getY(), L.metalDark, p.getRight(), p.getBottom(), false)); g.fillRect (p);
            }
        }
    }
    softShadow (g, r, 4.0f, { 6.0f, 9.0f }, 14.0f, night ? 0.85f : 0.55f);
    // body: dark epoxy, light from the top left - the lower and right side shows its height
    g.setGradientFill (ColourGradient (L.chipTop, r.getX(), r.getY(), L.chipBottom, r.getRight(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    auto top = r.reduced (3.0f).withTrimmedBottom (2.5f).withTrimmedRight (2.0f);
    g.setGradientFill (ColourGradient (L.chipTop.brighter (0.18f), top.getX(), top.getY(), L.chipTop.darker (0.25f), top.getX(), top.getBottom(), false));
    g.fillRoundedRectangle (top, 3.0f);
    g.setColour (Colours::white.withAlpha (night ? 0.10f : 0.22f)); g.drawLine (top.getX() + 3, top.getY() + 0.5f, top.getRight() - 3, top.getY() + 0.5f, 1.0f);
    g.setColour (Colours::black.withAlpha (0.45f)); g.drawRoundedRectangle (r, 4.0f, 1.0f);
    // a soft sheen across the top
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (night ? 0.05f : 0.10f), top.getX(), top.getY(), Colours::white.withAlpha (0.0f), top.getCentreX(), top.getCentreY(), false));
    g.fillRoundedRectangle (top, 3.0f);
    if (bga)
    {   // heat spreader: a metal lid with a brushed gradient
        auto lid = top.reduced (top.getWidth() * 0.12f);
        softShadow (g, lid, 3.0f, { 2.0f, 3.0f }, 5.0f, 0.4f);
        g.setGradientFill (ColourGradient (L.metal.brighter (0.1f), lid.getX(), lid.getY(), L.metalDark, lid.getRight(), lid.getBottom(), false));
        g.fillRoundedRectangle (lid, 3.0f);
        for (float y = lid.getY() + 2; y < lid.getBottom() - 1; y += 2.0f)
        { g.setColour (Colours::white.withAlpha (rnd.nextFloat() * 0.05f)); g.drawHorizontalLine ((int) y, lid.getX() + 2, lid.getRight() - 2); }
        g.setColour (Colours::white.withAlpha (0.35f)); g.drawLine (lid.getX() + 3, lid.getY() + 0.6f, lid.getRight() - 3, lid.getY() + 0.6f, 1.0f);
        g.setColour (Colours::black.withAlpha (0.35f)); g.drawRoundedRectangle (lid, 3.0f, 1.0f);
        g.setColour (Colours::black.withAlpha (0.28f)); g.setFont (Font (FontOptions (lid.getHeight() * 0.11f, Font::bold)).withExtraKerningFactor (0.2f));
        g.drawText (code, lid.reduced (6), Justification::centred);
    }
    else
    {
        g.setColour (Colours::white.withAlpha (0.12f)); g.fillEllipse (top.getX() + 5, top.getY() + 5, 5, 5);   // pin-1 mark
        g.setColour (Colours::white.withAlpha (night ? 0.20f : 0.30f)); g.setFont (Font (FontOptions (std::min (10.0f, top.getHeight() * 0.22f), Font::bold)).withExtraKerningFactor (0.15f));
        g.drawText (code, top.reduced (6, 2), Justification::centred);
    }
}

inline void capacitor (Graphics& g, Point<float> c, float rad, const BoardLook& L, bool night)
{
    softShadow (g, Rectangle<float> (rad * 2, rad * 2).withCentre (c), rad, { rad * 0.45f, rad * 0.7f }, rad * 0.8f, night ? 0.8f : 0.5f);
    g.setColour (L.metalDark.darker (0.4f)); g.fillEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (c));
    g.setGradientFill (ColourGradient (L.metal.brighter (0.15f), c.x - rad, c.y - rad, L.metalDark, c.x + rad, c.y + rad, false));
    g.fillEllipse (Rectangle<float> (rad * 1.8f, rad * 1.8f).withCentre (c));
    g.setColour (Colours::black.withAlpha (0.3f));   // vent cross
    g.drawLine (c.x - rad * 0.55f, c.y, c.x + rad * 0.55f, c.y, 1.0f); g.drawLine (c.x, c.y - rad * 0.55f, c.x, c.y + rad * 0.55f, 1.0f);
    Path band; band.addCentredArc (c.x, c.y, rad * 0.9f, rad * 0.9f, 0, 2.2f, 4.0f, true);
    g.setColour (Colours::black.withAlpha (0.35f)); g.strokePath (band, PathStrokeType (rad * 0.18f));
    g.setColour (Colours::white.withAlpha (0.45f)); g.fillEllipse (Rectangle<float> (rad * 0.5f, rad * 0.35f).withCentre (c.translated (-rad * 0.35f, -rad * 0.4f)));
}

inline void smd (Graphics& g, Point<float> c, bool vertical, const BoardLook& L, bool night)
{
    auto r = vertical ? Rectangle<float> (5, 11).withCentre (c) : Rectangle<float> (11, 5).withCentre (c);
    g.setColour (Colours::black.withAlpha (night ? 0.6f : 0.35f)); g.fillRect (r.translated (1.5f, 2.0f));
    g.setColour (Colour (0xff1c1e21)); g.fillRect (r);
    g.setColour (L.metal);
    if (vertical) { g.fillRect (r.withHeight (2.5f)); g.fillRect (r.withTrimmedTop (r.getHeight() - 2.5f)); }
    else          { g.fillRect (r.withWidth (2.5f));  g.fillRect (r.withTrimmedLeft (r.getWidth() - 2.5f)); }
}

inline Image makeBoard (bool night, uint32 seed)
{
    const int W = 1672, H = 941;
    const auto L = boardLook (night);
    Image img (Image::ARGB, W, H, true);
    Graphics g (img);
    Random rnd ((int64) seed);
    // board: soldermask with a fine fibreglass weave
    g.setGradientFill (ColourGradient (L.baseA, 0, 0, L.baseB, (float) W * 0.4f, (float) H, false)); g.fillAll();
    auto pool = [&g] (Point<float> c, float rad, Colour col)
    { g.setGradientFill (ColourGradient (col, c.x, c.y, col.withAlpha (0.0f), c.x + rad, c.y, true)); g.fillEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (c)); };
    pool ({ 837, 252 }, 520, L.led.withAlpha (night ? 0.20f : 0.16f));          // warm light behind BREED
    pool ({ 120, 860 }, 600, Colours::white.withAlpha (night ? 0.03f : 0.18f));
    pool ({ 1560, 80 }, 520, Colours::white.withAlpha (night ? 0.04f : 0.22f));

    // components: fixed big ones (under BREED and the panels) + random small ones, no overlaps
    struct Part { Rectangle<float> r; int kind; };   // 0 qfp, 1 bga (lid), 2 cap, 3 smd
    std::vector<Part> parts;
    auto freeAt = [&parts] (Rectangle<float> r) { for (auto& p : parts) if (p.r.expanded (14).intersects (r)) return false; return true; };
    parts.push_back ({ Rectangle<float> (250, 250).withCentre ({ 837, 252 }), 1 });   // the processor under the reactor
    // v0.34c: calm - behind the glass is mostly colour; only the processor under BREED and its lines
    for (int k = 0; k < 0; ++k)
    {
        const float w = 40.0f + rnd.nextFloat() * 70.0f, h = 30.0f + rnd.nextFloat() * 50.0f;
        Rectangle<float> r (14.0f + rnd.nextFloat() * ((float) W - w - 28.0f), 14.0f + rnd.nextFloat() * ((float) H - h - 28.0f), w, h);
        if (freeAt (r)) parts.push_back ({ r, 0 });
    }
    for (int k = 0; k < 0; ++k)
    {
        const float rad = 7.0f + rnd.nextFloat() * 8.0f;
        auto r = Rectangle<float> (rad * 2, rad * 2).withCentre ({ 20.0f + rnd.nextFloat() * (float) (W - 40), 20.0f + rnd.nextFloat() * (float) (H - 40) });
        if (freeAt (r)) parts.push_back ({ r, 2 });
    }
    for (int k = 0; k < 0; ++k)
    {
        auto r = Rectangle<float> (12, 12).withCentre ({ 10.0f + rnd.nextFloat() * (float) (W - 20), 10.0f + rnd.nextFloat() * (float) (H - 20) });
        if (freeAt (r.reduced (-2))) parts.push_back ({ r, 3 });
    }

    // copper traces: leave the chips on all sides, bend 45 degrees, end in a via
    auto via = [&g, &L] (Point<float> p) { g.setColour (L.copper); g.fillEllipse (p.x - 3.2f, p.y - 3.2f, 6.4f, 6.4f); g.setColour (L.baseB.darker (0.5f)); g.fillEllipse (p.x - 1.3f, p.y - 1.3f, 2.6f, 2.6f); };
    auto trace = [&] (Point<float> a, Point<float> d, float l1, float l2, int bend)
    {
        const Point<float> side (-d.y * (float) bend, d.x * (float) bend);
        const auto b = a + d * l1, c = b + (d + side) * (0.7071f * l2), e = c + d * (l1 * 0.5f);
        Path p; p.startNewSubPath (a); p.lineTo (b); p.lineTo (c); p.lineTo (e);
        g.setColour (Colours::black.withAlpha (night ? 0.35f : 0.18f)); g.strokePath (p, PathStrokeType (2.4f), AffineTransform::translation (0.6f, 1.0f));
        g.setColour (L.copper.withAlpha (night ? 0.75f : 0.8f)); g.strokePath (p, PathStrokeType (1.7f, PathStrokeType::mitered, PathStrokeType::rounded));
        g.setColour (L.copperHi.withAlpha (night ? 0.25f : 0.35f)); g.strokePath (p, PathStrokeType (0.6f), AffineTransform::translation (-0.4f, -0.4f));
        via (e);
    };
    for (auto& p : parts)
    {
        if (p.kind > 1) continue;
        const int n = 3 + rnd.nextInt (7);
        for (int i = 0; i < n; ++i)
        {
            const int side = rnd.nextInt (4);
            const float t = 0.15f + 0.7f * rnd.nextFloat();
            const Point<float> a = side == 0 ? Point<float> (p.r.getX() + p.r.getWidth() * t, p.r.getY() - 8) : side == 1 ? Point<float> (p.r.getX() + p.r.getWidth() * t, p.r.getBottom() + 8)
                                 : side == 2 ? Point<float> (p.r.getX() - 8, p.r.getY() + p.r.getHeight() * t) : Point<float> (p.r.getRight() + 8, p.r.getY() + p.r.getHeight() * t);
            const Point<float> d = side == 0 ? Point<float> (0, -1) : side == 1 ? Point<float> (0, 1) : side == 2 ? Point<float> (-1, 0) : Point<float> (1, 0);
            const float l1 = 14.0f + rnd.nextFloat() * 60.0f, l2 = 10.0f + rnd.nextFloat() * 40.0f;
            const int bend = rnd.nextBool() ? 1 : -1;
            for (int b = 0; b < (p.kind == 1 ? 4 : 2); ++b)   // small buses of parallel lines
                trace (a + Point<float> (-d.y, d.x) * (float) b * 5.0f, d, l1 + (float) b * 5.0f * (float) bend, l2, bend);
        }
    }
    // long buses across the board
    for (int k = 0; k < 3; ++k)
    {
        const Point<float> a (rnd.nextFloat() * (float) W, rnd.nextFloat() * (float) H);
        const Point<float> d = rnd.nextBool() ? Point<float> (1, 0) : Point<float> (0, 1);
        const float l1 = 120.0f + rnd.nextFloat() * 300.0f, l2 = 20.0f + rnd.nextFloat() * 60.0f;
        const int bend = rnd.nextBool() ? 1 : -1;
        for (int b = 0; b < 6; ++b) trace (a + Point<float> (-d.y, d.x) * (float) b * 5.0f, d, l1 + (float) b * 5.0f * (float) bend, l2, bend);
    }

    // parts on top of the copper
    static const char* codes[] { "KK-7720", "KX 3341", "A0 1180", "KK-2026", "B7 0451", "KX 9902", "M4 2210", "KK-0034" };
    for (auto& p : parts)
    {
        if (p.kind == 0) chip (g, p.r, L, night, rnd, false, codes[rnd.nextInt (8)]);
        else if (p.kind == 1) chip (g, p.r, L, night, rnd, true, p.r.getWidth() > 200 ? "BREED LAB  BL-1" : codes[rnd.nextInt (8)]);
        else if (p.kind == 2) capacitor (g, p.r.getCentre(), p.r.getWidth() * 0.5f, L, night);
        else smd (g, p.r.getCentre(), rnd.nextBool(), L, night);
    }
    // a few status lights (amber), brighter at night
    for (auto pt : { Point<float> (700, 120), Point<float> (975, 120), Point<float> (700, 384), Point<float> (975, 384) })
    {
        g.setGradientFill (ColourGradient (L.led.withAlpha (night ? 0.55f : 0.35f), pt.x, pt.y, L.led.withAlpha (0.0f), pt.x + 22, pt.y, true));
        g.fillEllipse (pt.x - 22, pt.y - 22, 44, 44);
        g.setColour (L.led.brighter (0.4f)); g.fillEllipse (pt.x - 2.5f, pt.y - 2.5f, 5, 5);
    }
    // depth: the board sits back - a soft vignette
    g.setGradientFill (ColourGradient (Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.45f, Colours::black.withAlpha (night ? 0.55f : 0.28f), 0, 0, true));
    g.fillAll();
    return img;
}

inline Image blurred (const Image& src, int factor = 3, int radius = 4)
{
    Image small (Image::ARGB, src.getWidth() / factor, src.getHeight() / factor, true);
    {
        Graphics g (small);
        g.setImageResamplingQuality (Graphics::highResamplingQuality);
        g.drawImage (src, small.getBounds().toFloat(), RectanglePlacement::stretchToFit);
    }
    ImageConvolutionKernel k (radius * 2 + 1);
    k.createGaussianBlur ((float) radius * 0.9f);
    k.applyToImage (small, small, small.getBounds());
    return small;
}

//==============================================================================
struct Backdrop { Image sharp, soft, grain; };
// owned by the editor's image cache (images must die with the editor - static images froze FL Studio on exit)
inline Backdrop*& activeBackdrop() { static Backdrop* b = nullptr; return b; }
inline Backdrop makeBackdrop()
{
    const bool night = theme().night;
    Backdrop b;
    b.sharp = makeBoard (night, 20260934);
    b.soft = blurred (b.sharp);
    b.grain = makeGrain (77, night);
    return b;
}

inline void base (Graphics& g, Rectangle<float> area)
{
    if (auto* bd = activeBackdrop()) { g.drawImage (bd->sharp, area, RectanglePlacement::fillDestination); return; }
    const auto& t = theme();
    g.setGradientFill (ColourGradient (t.baseTop, 0, area.getY(), t.baseBottom, 0, area.getBottom(), false));
    g.fillRect (area);
}

// FRONT layer: a pane of milky glass floating above the board - its shadow falls on the board, the board shows
// through frosted (slightly shifted = refraction), a thick bright edge on top, a darker one below, a soft reflection
inline void plate (Graphics& g, Rectangle<float> r, float corner = 12.0f, bool accentEdge = false)
{
    const auto& t = theme();
    const auto* bd = activeBackdrop();
    softShadow (g, r, corner, { 10.0f, 16.0f }, 26.0f, t.night ? 0.9f : 0.42f);
    {
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (r, corner);
        g.reduceClipRegion (clip);
        if (bd != nullptr)
        {
            g.setImageResamplingQuality (Graphics::highResamplingQuality);
            g.drawImage (bd->soft, Rectangle<float> (-5, -7, 1672, 941), RectanglePlacement::stretchToFit);   // refraction shift
        }
        // milk: whiter (day) / smoked (night), a little denser towards the bottom
        if (t.night)
        {
            g.setGradientFill (ColourGradient (Colour (0x6c1a1e24), 0, r.getY(), Colour (0x9010131a), 0, r.getBottom(), false)); g.fillRect (r);
            g.setColour (Colours::white.withAlpha (0.045f)); g.fillRect (r);
        }
        else
        {
            g.setGradientFill (ColourGradient (Colour (0xa8ffffff), 0, r.getY(), Colour (0xbcf2f5f8), 0, r.getBottom(), false)); g.fillRect (r);
        }
        if (bd != nullptr) { g.setTiledImageFill (bd->grain, 0, 0, 1.0f); g.fillRect (r); }
        // reflection: one wide soft diagonal band
        Path band;
        band.startNewSubPath (r.getX(), r.getY() + r.getHeight() * 0.15f);
        band.lineTo (r.getX() + r.getWidth() * 0.32f, r.getY()); band.lineTo (r.getX() + r.getWidth() * 0.55f, r.getY());
        band.lineTo (r.getX(), r.getY() + r.getHeight() * 0.75f); band.closeSubPath();
        g.setGradientFill (ColourGradient (Colours::white.withAlpha (t.night ? 0.06f : 0.22f), r.getX(), r.getY(), Colours::white.withAlpha (0.0f), r.getX() + r.getWidth() * 0.4f, r.getY() + r.getHeight() * 0.6f, false));
        g.fillPath (band);
        // inner rim: the glass thickness catches light at the edge
        g.setColour (Colours::white.withAlpha (t.night ? 0.05f : 0.30f)); g.drawRoundedRectangle (r.reduced (3.0f), corner - 2.0f, 3.0f);
    }
    // edges: bright top-left, darker bottom-right
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (t.night ? 0.30f : 0.95f), r.getX(), r.getY(),
                                       (t.night ? Colours::white.withAlpha (0.06f) : Colour (0x80707a85)), r.getRight(), r.getBottom(), false));
    g.drawRoundedRectangle (r.reduced (0.6f), corner, 1.3f);
    g.setColour (Colours::black.withAlpha (t.night ? 0.6f : 0.16f)); g.drawRoundedRectangle (r.expanded (0.8f), corner + 0.8f, 0.8f);
    if (accentEdge) { g.setColour (t.accent.withAlpha (0.75f)); g.drawRoundedRectangle (r.reduced (0.5f), corner, 1.4f); }
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
    GlyphArrangement a; a.addLineOfText (f1, "BREED", 0, 0);
    const float w1 = a.getBoundingBox (0, -1, true).getWidth();
    g.setColour (t.text); g.setFont (f1); g.drawText ("BREED", r, Justification::centredLeft);
    g.setFont (f2); g.drawText ("LAB", r.withTrimmedLeft (w1 + size * 0.7f), Justification::centredLeft);
}

//==============================================================================
// the whole page background, 1672 x 941 (the BREED LAB design size)
inline Image makeBackground()
{
    const auto& t = theme();
    Image img (Image::RGB, 1672, 941, true);
    Graphics g (img);
    base (g, { 0, 0, 1672, 941 });

    // ---- header: one long pane of glass, then wordmark, preset pill, buttons
    plate (g, { 10, 12, 1652, 80 }, 18.0f);
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
