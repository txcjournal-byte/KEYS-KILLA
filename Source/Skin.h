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
              C (0xff141030), C (0xff07081a), C (0xd915142c), C (0xff3a3264), C (0xffffffff), C (0xffaaa4cf), C (0xffff2f6d),
              C (0xffc8c4e8), C (0xff15142c), C (0xff2a2650), C (0xfff7f6ff), C (0xff0b0a18), C (0xffffffff), C (0xff4a4478), C (0xffff2f6d) },
        };
        return s;
    }
};
