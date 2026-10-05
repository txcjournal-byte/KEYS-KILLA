#pragma once
// v0.44 NO KNOBS. Every control that used to be a knob or a slider is drawn as a LIVING CELL: a membrane with a nucleus.
// Pull it up = feed it (the nucleus grows, more organelles, it glows); push it down = it starves. Bipolar controls lean
// cold (one way) or hot (the other). No pointer, no scale, no numbers - you feel it and you hear it.
// Linear controls become a STREAM: a channel of light whose glow gathers where the value is.
namespace kk::cell
{
inline float noise (float a, float seed) { return 0.5f * std::sin (a * 3.0f + seed) + 0.3f * std::sin (a * 5.0f - seed * 1.7f) + 0.2f * std::sin (a * 7.0f + seed * 0.6f); }

inline void draw (juce::Graphics& g, juce::Rectangle<float> area, float u, bool bipolar, juce::Colour a, juce::Colour b,
                  const juce::String& label, bool hot, bool opaque = false, float labelH = 18.0f, float seed = 1.0f)
{
    using namespace juce;
    u = jlimit (0.0f, 1.0f, u);
    auto r = area;
    auto lab = label.isNotEmpty() ? r.removeFromBottom (labelH) : Rectangle<float>();
    const auto c = r.getCentre();
    const float R = jmax (6.0f, jmin (r.getWidth(), r.getHeight()) * 0.5f - 3.0f);
    const float amount = bipolar ? std::abs (u - 0.5f) * 2.0f : u;
    const Colour core = bipolar ? (u < 0.5f ? a : b) : a.interpolatedWith (b, u);
    if (opaque) { g.setColour (Colour (0xff0b0c13)); g.fillEllipse (c.x - R - 3, c.y - R - 3, (R + 3) * 2, (R + 3) * 2); }
    // the membrane: an organic ring whose shape breathes with the value
    Path mem; const int n = 48;
    for (int i = 0; i <= n; ++i)
    {
        const float t = (float) i / (float) n * MathConstants<float>::twoPi;
        const float k = 1.0f + 0.045f * noise (t, seed + u * 4.0f) * (0.4f + amount);
        const Point<float> p (c.x + std::cos (t) * R * k * 0.96f, c.y + std::sin (t) * R * k * 0.96f);
        if (i == 0) mem.startNewSubPath (p); else mem.lineTo (p);
    }
    mem.closeSubPath();
    g.setGradientFill (ColourGradient (core.withAlpha (0.08f + 0.18f * amount), c.x, c.y - R, Colour (0xff05060a).withAlpha (0.55f), c.x, c.y + R, false));
    g.fillPath (mem);
    if (hot) { g.setColour (core.withAlpha (0.25f)); g.strokePath (mem, PathStrokeType (6.0f)); }
    g.setColour (core.withAlpha (0.35f + 0.5f * amount)); g.strokePath (mem, PathStrokeType (1.4f));
    // organelles: more life, more of them
    const int org = 2 + (int) std::round (amount * 9.0f);
    for (int i = 0; i < org; ++i)
    {
        const float t = (float) i * 2.399f + seed, d = R * (0.45f + 0.35f * std::fmod ((float) i * 0.618f, 1.0f));
        const float s = 1.2f + 1.6f * amount;
        g.setColour (b.withAlpha (0.35f + 0.5f * amount)); g.fillEllipse (c.x + std::cos (t) * d - s, c.y + std::sin (t) * d - s, s * 2, s * 2);
    }
    // the nucleus: its size is the value
    const float nr = R * (0.16f + 0.5f * amount);
    g.setColour (core.withAlpha (0.25f)); g.fillEllipse (c.x - nr * 1.6f, c.y - nr * 1.6f, nr * 3.2f, nr * 3.2f);
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.9f), c.x - nr * 0.4f, c.y - nr * 0.5f, core, c.x + nr, c.y + nr, true));
    g.fillEllipse (c.x - nr, c.y - nr, nr * 2, nr * 2);
    if (hot)   // which way it grows
    {
        g.setColour (Colours::white.withAlpha (0.6f));
        Path up; up.addTriangle (c.x - 4, c.y - R - 1, c.x + 4, c.y - R - 1, c.x, c.y - R - 7);
        Path dn; dn.addTriangle (c.x - 4, c.y + R + 1, c.x + 4, c.y + R + 1, c.x, c.y + R + 7);
        g.fillPath (up); g.fillPath (dn);
    }
    if (! lab.isEmpty())
    {
        g.setColour (hot ? core.brighter (0.5f) : Colour (0xffe6e3ff).withAlpha (0.85f));
        g.setFont (Font (FontOptions (jmin (12.5f, labelH * 0.7f), Font::bold)).withExtraKerningFactor (0.08f));
        g.drawFittedText (label, lab.toNearestInt(), Justification::centred, 1, 0.7f);
    }
}

// a stream of light instead of a slider: the glow gathers where the value is
inline void stream (juce::Graphics& g, juce::Rectangle<float> r, float u, bool vertical, juce::Colour c, bool hot)
{
    using namespace juce;
    u = jlimit (0.0f, 1.0f, u);
    const auto ch = vertical ? r.withSizeKeepingCentre (jmin (14.0f, r.getWidth()), r.getHeight()) : r.withSizeKeepingCentre (r.getWidth(), jmin (14.0f, r.getHeight()));
    g.setColour (Colour (0xff05060a).withAlpha (0.6f)); g.fillRoundedRectangle (ch, 7.0f);
    const float len = vertical ? ch.getHeight() : ch.getWidth();
    const int n = 26;
    for (int i = 0; i < n; ++i)
    {
        const float t = ((float) i + 0.5f) / (float) n;
        const float pos = vertical ? 1.0f - t : t;
        const float d = std::abs (pos - u);
        const float glow = std::exp (-d * d * 60.0f), fill = pos <= u ? 0.35f : 0.06f;
        const float s = 2.0f + 4.0f * glow;
        const Point<float> p = vertical ? Point<float> (ch.getCentreX(), ch.getY() + t * len) : Point<float> (ch.getX() + t * len, ch.getCentreY());
        g.setColour (c.withAlpha (jmin (1.0f, fill + glow))); g.fillEllipse (p.x - s, p.y - s, s * 2, s * 2);
    }
    const Point<float> at = vertical ? Point<float> (ch.getCentreX(), ch.getBottom() - u * len) : Point<float> (ch.getX() + u * len, ch.getCentreY());
    g.setColour (c.withAlpha (hot ? 0.45f : 0.25f)); g.fillEllipse (at.x - 13, at.y - 13, 26, 26);
    g.setColour (Colours::white.withAlpha (0.95f)); g.fillEllipse (at.x - 5, at.y - 5, 10, 10);
}
} // namespace kk::cell
