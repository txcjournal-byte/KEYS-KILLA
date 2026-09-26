#pragma once
#include <juce_core/juce_core.h>
#include <vector>

enum Tile { tBells, tKeys, tPlucks, tFlutes, tChoir, tPads, tLeads, tBass, tExotic, tExperimental, numTiles };

struct Preset
{
    juce::String name;
    int tile = 0;
    int era  = 0;              // 0..5 = 2010,2013,2016,2019,2022,2026 buckets; -1 = no era
    bool exclusive = false;
    juce::String sub;          // sub-category (Strings, Brass, Organs, ...)
    std::vector<std::pair<const char*, float>> values;   // real (denormalised) values
};

const std::vector<Preset>& factoryPresets();
const juce::StringArray& tileNames();
const juce::StringArray& eraNames();
