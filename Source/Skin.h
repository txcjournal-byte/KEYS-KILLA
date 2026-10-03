#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

// A skin changes only graphics (colours, textures, logo). Layout is shared.
// To add a new skin (e.g. ICE) add one entry to Skin::all().
struct Skin
{
    juce::String name;
    bool dark = true;
    juce::Colour bgTop, bgBottom, panel, panelEdge, text, textDim, accent;
    juce::Colour knobLight, knobDark, track, keyWhite, keyBlack, iconLight, iconDark, deco;

    static const std::vector<Skin>& all()
    {
        using C = juce::Colour;
        static const std::vector<Skin> s {
            // v0.34: GLASS (day) - light milky glass, dark text, amber accent
            { "GLASS", false,
              C (0xffe3e6ea), C (0xffc7ccd2), C (0xb3f4f6f8), C (0xff9aa2ac), C (0xff1a1e23), C (0xff59616b), C (0xffff8a3d),
              C (0xfff6f7f8), C (0xffb4bac2), C (0xffc3c8cf), C (0xfff7f8f9), C (0xff23272d), C (0xff1a1e23), C (0xff59616b), C (0xffff8a3d) },
            // v0.34: NIGHT - dark smoked glass, light text, amber accent
            { "NIGHT", true,
              C (0xff15171b), C (0xff07080a), C (0xc01d2026), C (0xff3a3f47), C (0xffeef0f2), C (0xff8c939b), C (0xffff8a3d),
              C (0xffd5d9de), C (0xff16181c), C (0xff2a2e35), C (0xffeceef1), C (0xff0d0f12), C (0xffffffff), C (0xff4a5059), C (0xffff8a3d) },
        };
        return s;
    }
};
