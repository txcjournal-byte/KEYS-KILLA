#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// v0.34 THEMES: GLASS (day, light milky glass) and NIGHT (dark smoked glass), both over a faint see-through circuit board.
// One accent: amber. The moon / sun button switches them (remembered in the settings).
// TC() maps every colour the UI code was written with (the old neon palette) onto the active theme, so the whole
// plugin follows one switch: neutral colours -> the theme's glass / text greys, saturated colours -> the amber accent.
namespace kk
{
struct Theme
{
    bool night;
    juce::Colour baseTop, baseBottom, trace, glass, glassHi, glassEdge, text, dim, accent, accentDeep, shadow, knobTop, knobBottom, well;
};

inline int& themeIndex() { static int t = 0; return t; }   // 0 GLASS (day), 1 NIGHT
inline int& themeVersion() { static int v = 0; return v; } // bumps on every switch (image caches rebuild)

inline const Theme& theme()
{
    using C = juce::Colour;
    static const Theme glass { false, C (0xffe3e6ea), C (0xffc7ccd2), C (0xff5f6974), C (0x8cffffff), C (0xd9ffffff), C (0x598a939e),
                               C (0xff1a1e23), C (0xff59616b), C (0xffff8a3d), C (0xffd8661f), C (0xff2a3038), C (0xfff6f7f8), C (0xffb4bac2), C (0x33505a66) };
    static const Theme night { true, C (0xff15171b), C (0xff07080a), C (0xff9aa4b1), C (0x9e1d2026), C (0x24ffffff), C (0x1fffffff),
                               C (0xffeef0f2), C (0xff8c939b), C (0xffff8a3d), C (0xffc9601f), C (0xff000000), C (0xff3c4148), C (0xff101215), C (0x66000000) };
    return themeIndex() == 1 ? night : glass;
}

inline juce::Colour themed (juce::uint32 argb)
{
    const auto& t = theme();
    const float a = (float) ((argb >> 24) & 0xff) / 255.0f;
    const int r = (int) ((argb >> 16) & 0xff), g = (int) ((argb >> 8) & 0xff), b = (int) (argb & 0xff);
    const int mx = std::max ({ r, g, b }), mn = std::min ({ r, g, b });
    const float chroma = (float) (mx - mn) / 255.0f;
    const float lum = (0.2126f * (float) r + 0.7152f * (float) g + 0.0722f * (float) b) / 255.0f;
    if (mx == 0) return t.shadow.withAlpha (a * (t.night ? 1.0f : 0.45f));   // shadows
    if (chroma >= 0.30f)
    {
        const float h = juce::Colour ((juce::uint8) r, (juce::uint8) g, (juce::uint8) b).getHue();
        if (h > 0.22f && h < 0.47f) return juce::Colour (argb).withMultipliedSaturation (0.7f);        // greens: status (on / ok)
        if ((h < 0.02f || h > 0.985f) && g < 0x50 && b < 0x50) return juce::Colour (argb);          // pure red: warnings
        const float v = (float) mx / 255.0f;
        return (v > 0.75f ? t.accent : t.accent.interpolatedWith (t.accentDeep, 1.0f - v / 0.75f).withMultipliedBrightness (0.35f + 0.65f * v / 0.75f)).withAlpha (a);
    }
    if (t.night)
    {
        const float l = lum < 0.25f ? lum * 1.08f : lum;
        return juce::Colour::fromHSL (0.58f, 0.05f, juce::jlimit (0.0f, 1.0f, l), a);
    }
    return juce::Colour::fromHSL (0.58f, 0.07f, juce::jlimit (0.0f, 1.0f, 0.95f - 0.85f * lum), a);
}
} // namespace kk

// short name for the drawing code
inline juce::Colour TC (juce::uint32 argb) { return kk::themed (argb); }
