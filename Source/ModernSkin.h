#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// v0.26: the BREED LAB main page drawn in code - clean graphite studio hardware with one red accent
// (same style as the drum pages). It replaces the old horror bitmap; every live control sits where it did.
namespace kk
{
namespace modern
{
using namespace juce;
// v0.28 NEON: deep night blue, hot pink, orange and violet light (the user's own design)
inline const Colour bgTop { 0xff141030 }, bgBottom { 0xff07081a }, panel { 0xff15142c }, edge { 0xff3a3264 },
                    text { 0xffffffff }, dim { 0xffaaa4cf }, accent { 0xffff2f6d }, orange { 0xffff8a3d }, violet { 0xff9b4dff }, blue { 0xff4d7dff };

inline Font font (float size, bool bold = true, float kern = 0.1f)
{
    return Font (FontOptions (size, bold ? Font::bold : Font::plain)).withExtraKerningFactor (kern);
}
inline void plate (Graphics& g, Rectangle<float> r, float corner = 10.0f, bool screws = false, Colour c1 = Colour (0xff3a3264), Colour c2 = Colour (0xff3a3264), float glow = 0.0f)
{
    if (glow > 0)
        for (int k = 4; k >= 1; --k)
        {
            g.setGradientFill (ColourGradient (c1.withAlpha (glow * 0.09f), r.getX(), r.getY(), c2.withAlpha (glow * 0.09f), r.getRight(), r.getBottom(), false));
            g.drawRoundedRectangle (r.expanded ((float) k * 2.0f), corner + (float) k * 2.0f, 3.0f);
        }
    g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r.translated (0, 3), corner);
    g.setGradientFill (ColourGradient (Colour (0xff1c1a3a), 0, r.getY(), Colour (0xff0f0e22), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, corner);
    g.setGradientFill (ColourGradient (c1, r.getX(), r.getY(), c2, r.getRight(), r.getBottom(), false));
    g.drawRoundedRectangle (r.reduced (0.5f), corner, glow > 0 ? 2.0f : 1.2f);
    g.setColour (Colours::white.withAlpha (0.06f)); g.drawHorizontalLine ((int) r.getY() + 2, r.getX() + corner, r.getRight() - corner);
    if (screws)
        for (auto c : { r.getTopLeft().translated (9, 9), r.getTopRight().translated (-9, 9), r.getBottomLeft().translated (9, -9), r.getBottomRight().translated (-9, -9) })
        { g.setColour (Colour (0xff4a4478)); g.fillEllipse (c.x - 2.5f, c.y - 2.5f, 5, 5); }
}
// neon light waves (the flowing lines of the design)
inline void waves (Graphics& g, Point<float> from, Point<float> to, float spread, Colour a, Colour b, int lines, float alpha)
{
    for (int i = 0; i < lines; ++i)
    {
        const float t = lines > 1 ? (float) i / (float) (lines - 1) - 0.5f : 0.0f;
        Path p; p.startNewSubPath (from.translated (0, t * spread * 0.3f));
        const float dx = to.x - from.x;
        p.cubicTo (from.x + dx * 0.3f, from.y - spread * (0.6f + t), from.x + dx * 0.7f, to.y + spread * (0.6f - t), to.x, to.y + t * spread * 0.3f);
        g.setGradientFill (ColourGradient (a.withAlpha (alpha * (1.0f - std::abs (t))), from.x, from.y, b.withAlpha (alpha * (1.0f - std::abs (t))), to.x, to.y, false));
        g.strokePath (p, PathStrokeType (1.3f));
    }
}
inline void label (Graphics& g, const String& t, Rectangle<float> r, float size = 12.0f, Colour c = dim, Justification j = Justification::centred)
{
    g.setColour (c); g.setFont (font (size)); g.drawText (t, r, j);
}
// knob cap: the knob class cuts it out of this picture and turns it (pointer up = middle)
inline void knobCap (Graphics& g, Point<float> c, float r, float arcR)
{
    for (int i = 0; i <= 10; ++i)   // scale ticks
    {
        const float a = MathConstants<float>::pi * (-0.75f + 1.5f * (float) i / 10.0f) - MathConstants<float>::halfPi;
        g.setColour (Colour (0xff5a5490));
        g.fillEllipse (c.x + std::cos (a) * (arcR + 7) - 1.5f, c.y + std::sin (a) * (arcR + 7) - 1.5f, 3, 3);
    }
    Path track; track.addCentredArc (c.x, c.y, arcR, arcR, 0, -MathConstants<float>::pi * 0.75f, MathConstants<float>::pi * 0.75f, true);
    g.setColour (Colours::black.withAlpha (0.55f)); g.strokePath (track, PathStrokeType (5.0f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (Colours::black.withAlpha (0.5f)); g.fillEllipse (Rectangle<float> (r * 2 + 8, r * 2 + 8).withCentre (c.translated (0, 3)));
    g.setGradientFill (ColourGradient (Colour (0xff4a4670), c.x, c.y - r, Colour (0xff0b0a18), c.x, c.y + r, false));
    g.fillEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c));
    g.setGradientFill (ColourGradient (Colour (0xff2a2748), c.x, c.y - r * 0.8f, Colour (0xff121128), c.x, c.y + r * 0.8f, false));
    g.fillEllipse (Rectangle<float> (r * 1.62f, r * 1.62f).withCentre (c));
    g.setColour (Colours::white.withAlpha (0.12f)); g.drawEllipse (Rectangle<float> (r * 2, r * 2).withCentre (c), 1.0f);
    g.setColour (Colours::white.withAlpha (0.92f));
    g.drawLine (c.x, c.y - r * 0.32f, c.x, c.y - r * 0.86f, std::max (2.0f, r * 0.09f));
}
inline void chevron (Graphics& g, Rectangle<float> r, bool left)
{
    Path p; const auto c = r.getCentre(); const float s = r.getHeight() * 0.18f;
    p.startNewSubPath (c.x + (left ? s : -s), c.y - s * 1.6f); p.lineTo (c.x + (left ? -s : s), c.y); p.lineTo (c.x + (left ? s : -s), c.y + s * 1.6f);
    g.setColour (text); g.strokePath (p, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));
}
inline void dice (Graphics& g, Rectangle<float> r)
{
    g.setColour (Colour (0xff1e1c3c)); g.fillRoundedRectangle (r, 6);
    g.setColour (edge.brighter (0.3f)); g.drawRoundedRectangle (r, 6, 1.2f);
    g.setColour (text);
    for (auto pt : { Point<float> (0.3f, 0.3f), Point<float> (0.7f, 0.3f), Point<float> (0.5f, 0.5f), Point<float> (0.3f, 0.7f), Point<float> (0.7f, 0.7f) })
        g.fillEllipse (r.getX() + r.getWidth() * pt.x - 2.5f, r.getY() + r.getHeight() * pt.y - 2.5f, 5, 5);
}
inline void button (Graphics& g, Rectangle<float> r, const String& t, float size = 13.0f)
{
    g.setGradientFill (ColourGradient (Colour (0xff24214a), 0, r.getY(), Colour (0xff14132c), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7);
    g.setColour (edge.brighter (0.25f)); g.drawRoundedRectangle (r.reduced (0.5f), 7, 1.2f);
    if (t.isNotEmpty()) label (g, t, r, size, text);
}

// the whole page background, 1672 x 941 (the BREED LAB design size)
inline Image makeBackground()
{
    Image img (Image::RGB, 1672, 941, true);
    Graphics g (img);
    const float W = 1672.0f, H = 941.0f;
    g.setGradientFill (ColourGradient (bgTop, 0, 0, bgBottom, 0, H, false)); g.fillAll();
    // coloured light: magenta top-left, violet right, pink behind BREED
    auto glow = [&g] (Point<float> c, float rad, Colour col, float a)
    { g.setGradientFill (ColourGradient (col.withAlpha (a), c.x, c.y, col.withAlpha (0.0f), c.x + rad, c.y, true)); g.fillEllipse (Rectangle<float> (rad * 2, rad * 2).withCentre (c)); };
    glow ({ 60, 40 }, 420, accent, 0.30f);
    glow ({ 1660, 140 }, 380, violet, 0.28f);
    glow ({ 837, 250 }, 360, accent, 0.22f);
    glow ({ 1000, 230 }, 300, violet, 0.18f);
    glow ({ 1650, 900 }, 300, accent, 0.12f);
    // neon waves: from the parents into BREED and across the top
    waves (g, { 0, 120 }, { 640, 30 }, 120, accent, orange, 7, 0.35f);
    waves (g, { 1060, 40 }, { 1672, 160 }, 110, violet, accent, 7, 0.30f);
    waves (g, { 600, 250 }, { 760, 250 }, 70, accent, orange, 9, 0.45f);
    waves (g, { 915, 250 }, { 1080, 250 }, 70, violet, accent, 9, 0.45f);

    // ---- header: logo, preset bar, buttons
    g.setColour (text); g.setFont (font (56.0f, true, 0.06f));
    g.drawText ("KEYS", Rectangle<float> (138, 14, 200, 64), Justification::centredLeft);
    g.setGradientFill (ColourGradient (accent, 320, 30, Colour (0xffff6aa0), 480, 70, false));
    g.drawText ("KILLA", Rectangle<float> (314, 14, 260, 64), Justification::centredLeft);
    label (g, "DON'T BROWSE SOUNDS, BREED THEM.", { 140, 76, 440, 18 }, 11.5f, dim, Justification::centredLeft);
    {   // maker tag
        g.setColour (dim); g.setFont (font (11.0f, false, 0.1f)); g.drawText ("by", Rectangle<float> (404, 76, 20, 18), Justification::centredLeft);
        g.setGradientFill (ColourGradient (orange, 420, 0, Colour (0xffff3fd2), 486, 0, false));
        g.setFont (font (13.0f, true, 0.06f)); g.drawText ("TrapVST", Rectangle<float> (420, 75, 90, 20), Justification::centredLeft);
    }
    plate (g, { 616, 30, 520, 52 }, 26.0f, false, edge.brighter (0.4f), edge.brighter (0.4f));
    chevron (g, { 626, 34, 42, 44 }, true); chevron (g, { 1044, 34, 40, 44 }, false);
    {   // heart
        Path h; const float cx = 1107, cy = 57, s = 9;
        h.startNewSubPath (cx, cy + s);
        h.cubicTo (cx - s * 2.2f, cy - s * 0.3f, cx - s * 0.9f, cy - s * 1.9f, cx, cy - s * 0.6f);
        h.cubicTo (cx + s * 0.9f, cy - s * 1.9f, cx + s * 2.2f, cy - s * 0.3f, cx, cy + s);
        g.setColour (accent); g.strokePath (h, PathStrokeType (2.4f));
    }
    button (g, { 1247, 44, 97, 40 }, "SAVE");
    button (g, { 1367, 44, 97, 40 }, "MENU");
    button (g, { 1485, 42, 57, 44 }, "");
    {   // moon
        const Point<float> c (1513, 64);
        Path m; m.addEllipse (c.x - 11, c.y - 11, 22, 22);
        Path cut; cut.addEllipse (c.x - 5, c.y - 15, 22, 22);
        g.setColour (Colour (0xffd9d6ff)); g.fillPath (m);
        g.setColour (Colour (0xff1c1a3c)); g.fillPath (cut);
    }

    // ---- left column (the tiles draw themselves on it)
    plate (g, { 8, 96, 138, 512 }, 12.0f, false, edge, edge);

    // ---- parents + BREED
    for (int k = 0; k < 2; ++k)
    {
        const Rectangle<float> r = k == 0 ? Rectangle<float> (214, 128, 428, 248) : Rectangle<float> (1020, 128, 414, 248);
        if (k == 0) plate (g, r, 14.0f, false, accent, orange, 1.0f);
        else        plate (g, r, 14.0f, false, violet, blue, 1.0f);
        label (g, k == 0 ? "PARENT A" : "PARENT B", { r.getX() + 26, r.getY() + 10, 200, 26 }, 17.0f, text, Justification::centredLeft);
        dice (g, k == 0 ? Rectangle<float> (584, 147, 36, 36) : Rectangle<float> (1377, 148, 36, 36));
        chevron (g, k == 0 ? Rectangle<float> (228, 228, 38, 46) : Rectangle<float> (1088, 228, 38, 46), true);
        chevron (g, k == 0 ? Rectangle<float> (545, 228, 38, 46) : Rectangle<float> (1380, 228, 38, 46), false);
    }
    {
        const Point<float> c (837, 252);
        glow (c, 170, accent, 0.45f);
        g.setColour (Colour (0xff0d0c1e)); g.fillEllipse (Rectangle<float> (210, 210).withCentre (c));
        g.setGradientFill (ColourGradient (violet, c.x - 100, c.y, accent, c.x + 100, c.y, false));
        g.drawEllipse (Rectangle<float> (210, 210).withCentre (c), 3.0f);
        g.setGradientFill (ColourGradient (Colour (0xffff4f7b), c.x - 30, c.y - 70, orange, c.x + 40, c.y + 80, true));
        g.fillEllipse (Rectangle<float> (172, 172).withCentre (c));
        g.setGradientFill (ColourGradient (Colour (0xffff3a6a), c.x, c.y - 70, Colour (0xffff7a45), c.x, c.y + 86, false));
        g.fillEllipse (Rectangle<float> (164, 164).withCentre (c));
        g.setColour (Colours::white.withAlpha (0.16f)); g.fillEllipse (Rectangle<float> (118, 46).withCentre (c.translated (0, -50)));
        g.setColour (Colours::white); g.setFont (font (36.0f, true, 0.12f));
        g.drawText ("BREED", Rectangle<float> (200, 50).withCentre (c), Justification::centred);
    }

    // ---- children
    const int cx[6] { 220, 422, 623, 827, 1030, 1233 };
    for (int i = 0; i < 6; ++i)
    {
        const Rectangle<float> r ((float) cx[i] - 2, 385, 194, 128);
        plate (g, r, 10.0f, false, edge.brighter (0.2f), edge.brighter (0.2f));
        label (g, "CHILD " + String (i + 1), { r.getX(), r.getY() + 8, r.getWidth(), 20 }, 13.5f, text);
        const Point<float> pc ((float) cx[i] + 160, 451);
        g.setColour (Colour (0xff0b0a18)); g.fillEllipse (Rectangle<float> (30, 30).withCentre (pc));
        g.setColour (Colours::white.withAlpha (0.5f)); g.drawEllipse (Rectangle<float> (30, 30).withCentre (pc), 1.5f);
        Path tri; tri.addTriangle (pc.x - 4, pc.y - 7, pc.x - 4, pc.y + 7, pc.x + 7, pc.y);
        g.setColour (text); g.fillPath (tri);
    }

    // ---- genes, mutate, family tree, undo
    plate (g, { 176, 524, 862, 82 }, 12.0f);
    plate (g, { 182, 530, 74, 70 }, 9.0f, false, accent.withAlpha (0.6f), violet.withAlpha (0.6f));
    label (g, "GENES", { 182, 538, 74, 18 }, 13.0f, text);
    {   // helix
        Path dna; for (int k = 0; k < 2; ++k) { dna.startNewSubPath (209.0f, 563.0f); for (int i = 0; i <= 20; ++i) { const float t = (float) i / 20.0f; dna.lineTo (219.0f + std::sin (t * 6.283f + (float) k * 3.1416f) * 8.0f, 563.0f + t * 28.0f); } }
        g.setColour (accent); g.strokePath (dna, PathStrokeType (1.8f));
    }
    static const char* geneNames[] { "BODY", "ATTACK", "TEXTURE", "SPACE", "MOVEMENT", "CHARACTER" };
    const int gx[6] { 311, 441, 572, 697, 830, 952 };
    for (int i = 0; i < 6; ++i)
    {
        label (g, geneNames[i], { (float) gx[i] - 50, 534, 120, 18 }, 11.5f, dim);
        const float x0 = (float) gx[i] - 41;
        g.setColour (Colour (0xff0b0a18)); g.fillRoundedRectangle (x0 + 4, 559, 74, 27, 13);
        g.setColour (edge.brighter (0.2f)); g.drawRoundedRectangle (x0 + 4, 559, 74, 27, 13, 1.0f);
        label (g, "A", { x0 + 4, 559, 37, 27 }, 13.0f, dim); label (g, "B", { x0 + 41, 559, 37, 27 }, 13.0f, dim);
        g.setColour (dim); g.drawRoundedRectangle (x0 + 89, 570, 12, 10, 2, 1.4f); g.drawRoundedRectangle (x0 + 91, 564, 8, 9, 3, 1.4f);
    }
    plate (g, { 1042, 524, 254, 82 }, 12.0f);
    label (g, "MUTATE", { 1054, 530, 120, 20 }, 13.0f, text, Justification::centredLeft);
    const int mx[5][2] { { 1049, 1094 }, { 1095, 1142 }, { 1142, 1189 }, { 1189, 1236 }, { 1236, 1288 } };
    static const char* mutNames[] { "5%", "15%", "30%", "60%", "CHAOS" };
    for (int i = 0; i < 5; ++i) button (g, Rectangle<float> ((float) mx[i][0] + 2, 557, (float) (mx[i][1] - mx[i][0]) - 4, 36), mutNames[i], 11.5f);
    button (g, { 1300, 528, 74, 76 }, "");
    label (g, "FAMILY", { 1300, 570, 74, 14 }, 11.0f, text); label (g, "TREE", { 1300, 584, 74, 14 }, 11.0f, text);
    g.setColour (accent);
    for (auto p : { Point<float> (1337, 542), Point<float> (1328, 558), Point<float> (1346, 558) }) g.fillEllipse (p.x - 3.5f, p.y - 3.5f, 7, 7);
    g.drawLine (1337, 542, 1328, 558, 1.6f); g.drawLine (1337, 542, 1346, 558, 1.6f);
    button (g, { 1379, 528, 63, 76 }, "");
    label (g, "UNDO", { 1379, 576, 63, 16 }, 11.0f, text);
    {
        Path u; u.addCentredArc (1410, 556, 9, 9, 0, -2.6f, 1.6f, true);
        g.setColour (text); g.strokePath (u, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
        Path ah; ah.addTriangle (1400, 546, 1408, 548, 1402, 554); g.fillPath (ah);
    }

    // ---- right column: drag to DAW + FUTURE / ALIVE / TIME
    plate (g, { 1448, 104, 176, 504 }, 14.0f, false, violet, accent, 0.7f);
    knobCap (g, { 1532, 447 }, 33, 43); label (g, "FUTURE", { 1482, 494, 100, 18 }, 12.5f, text);
    knobCap (g, { 1490, 552 }, 20, 28); label (g, "ALIVE", { 1450, 584, 80, 16 }, 11.5f, text);
    knobCap (g, { 1575, 552 }, 20, 28); label (g, "TIME", { 1535, 584, 80, 16 }, 11.5f, text);

    // ---- macros, output, wheels, keyboard bed
    plate (g, { 58, 668, 1226, 124 }, 14.0f, true, edge.brighter (0.2f), edge.brighter (0.2f));
    const int kx[8] { 132, 284, 435, 587, 742, 895, 1047, 1202 };
    static const char* macroNames[] { "DARK", "SPACE", "MOVEMENT", "WIDTH", "TEXTURE", "PUNCH", "DIRT", "MIX" };
    for (int i = 0; i < 8; ++i) { knobCap (g, { (float) kx[i], 715 }, 29, 41); label (g, macroNames[i], { (float) kx[i] - 62, 757, 124, 22 }, 13.0f, text); }
    plate (g, { 1300, 668, 326, 124 }, 14.0f, false, edge.brighter (0.2f), edge.brighter (0.2f));
    label (g, "OUTPUT", { 1318, 678, 120, 22 }, 14.0f, text, Justification::centredLeft);
    label (g, "L", { 1318, 714, 16, 16 }, 11.0f, dim); label (g, "R", { 1318, 746, 16, 16 }, 11.0f, dim);
    g.setColour (Colour (0xff0a0918)); g.fillRoundedRectangle (1336, 715, 260, 15, 4); g.fillRoundedRectangle (1336, 747, 260, 15, 4);
    static const char* scale[] { "-60", "-36", "-24", "-12", "-6", "0" };
    const float sx[] { 1341, 1393, 1443, 1493, 1543, 1593 };
    for (int i = 0; i < 6; ++i) label (g, scale[i], { sx[i] - 18, 768, 36, 14 }, 10.0f, dim);
    plate (g, { 50, 800, 140, 132 }, 12.0f);
    for (float x : { 72.0f, 126.0f }) { g.setColour (Colour (0xff0a0918)); g.fillRoundedRectangle (x, 808, 36, 86, 8); g.setColour (edge.brighter (0.2f)); g.drawRoundedRectangle (x, 808, 36, 86, 8, 1.0f); }
    label (g, "PITCH", { 58, 898, 64, 16 }, 11.0f, text); label (g, "MOD", { 112, 898, 64, 16 }, 11.0f, text);
    plate (g, { 196, 804, 1450, 128 }, 12.0f, false, accent.withAlpha (0.5f), violet.withAlpha (0.5f));
    return img;
}
// piano keys for the keyboard component (stretched to every key)
inline Image makeWhiteKey()
{
    Image img (Image::ARGB, 72, 220, true);
    Graphics g (img);
    auto r = Rectangle<float> (1, 0, 70, 219);
    g.setGradientFill (ColourGradient (Colour (0xfff7f6ff), 0, 0, Colour (0xffd6d3ea), 0, 220, false));
    g.fillRoundedRectangle (r, 5);
    g.setColour (Colour (0xff9a9c9f)); g.drawRoundedRectangle (r, 5, 1.2f);
    g.setColour (Colours::white.withAlpha (0.6f)); g.fillRect (4.0f, 0.0f, 3.0f, 210.0f);
    return img;
}
inline Image makeBlackKey()
{
    Image img (Image::ARGB, 44, 140, true);
    Graphics g (img);
    auto r = Rectangle<float> (1, 0, 42, 139);
    g.setGradientFill (ColourGradient (Colour (0xff302c52), 0, 0, Colour (0xff0b0a18), 0, 140, false));
    g.fillRoundedRectangle (r, 4);
    g.setColour (Colours::white.withAlpha (0.12f)); g.fillRoundedRectangle (r.reduced (5, 3).withTrimmedBottom (14), 3);
    g.setColour (Colours::black); g.drawRoundedRectangle (r, 4, 1.0f);
    return img;
}
} // namespace modern
} // namespace kk
