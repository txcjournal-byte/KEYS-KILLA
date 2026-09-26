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
            { "CHROME", false,
              C (0xffdfe3e8), C (0xffa3aab3), C (0xd9e9ecf0), C (0xff8d949c), C (0xff111316), C (0xff3a3f46), C (0xff3fa8ff),
              C (0xfff4f6f8), C (0xff7c848d), C (0xff9aa2ab), C (0xfff4f5f7), C (0xff15171a), C (0xffffffff), C (0xff5d656e), C (0xffffffff) },
            { "BLOOD", true,
              C (0xff141010), C (0xff070606), C (0xd90d0b0b), C (0xff3b3434), C (0xffe9e4e4), C (0xffa39c9c), C (0xffff1f2d),
              C (0xffbfc3c7), C (0xff1c1d20), C (0xff2e2a2a), C (0xffd6d3cf), C (0xff0c0b0b), C (0xffe8e8e8), C (0xff4a4a4a), C (0xffd4101c) },
        };
        return s;
    }
};
