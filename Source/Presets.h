#pragma once
#include <juce_core/juce_core.h>
#include <vector>

// Factory sound taxonomy (TRAP 2010 -> FUTURE spec, chapter 4)
enum Category { cPiano, cKeys, cBells, cPlucks, cMallets, cGuitar, cStrings, cBrass, cChoir, cWoodwind,
                cLead, cPads, cSynth, cBass, c808, cTexture, cArp, cFX,
                cOrgan, cChip, cWorld, cDrums, cGameFx, cCinematic, numCategories };   // v0.30 SOUND LIBRARY categories

// Eras: 0 = 2010-12, 1 = 2013-15, 2 = 2016-18, 3 = 2019-21, 4 = 2022-24, 5 = 2025-26, 6 = FUTURE
enum { eraFuture = 6, numEras = 7 };

// The ten browser tiles of the skin group the categories
enum Tile { tBells, tKeys, tPlucks, tFlutes, tChoir, tPads, tLeads, tBass, tOrchestra, tExperimental, numTiles };

struct Preset
{
    juce::String name;
    int cat  = 0;              // Category
    int era  = 0;              // 0..6 (6 = FUTURE)
    bool exclusive = false;
    juce::String sub;          // subcategory, one of subcategoryNames (cat)
    std::vector<std::pair<juce::String, float>> values;   // real (denormalised) values
    juce::StringArray macroNames;                          // optional per-preset macro labels

    // metadata (derived from the sound when the table is built)
    juce::String mood, character, articulation, tempo;
    bool mono = false;
    int brightness = 3, movement = 0, cpu = 1;             // 1..5, 0..5, 1..3
    juce::String author { "KEYS KILLA Factory" }, version { "0.5" };

    bool isBass() const { return cat == cBass || cat == c808; }
    juce::String info() const;                             // one-line tag summary for the browser
    juce::String searchText() const;
};

const std::vector<Preset>& factoryPresets();
const juce::StringArray& categoryNames();
const juce::StringArray& subcategoryNames (int cat);
const juce::StringArray& eraNames();                       // "2010-12" ... "FUTURE"
const juce::StringArray& tileNames();
const std::vector<int>& tileCategories (int tile);
int tileOfCategory (int cat);
const juce::StringArray& moodNames();
const juce::StringArray& characterNames();
const juce::StringArray& articulationNames();

// ERA sound policy: what a sound gains when it moves to an era (added on top of its own values)
struct EraProfile
{
    float crush = 0, wow = 0, width = 0, rev = 0, dly = 0, chorus = 0, drive = 0, cutOct = 0, detune = 0,
          reverse = 0, ghost = 0, circuit = 0, punch = 0, drift = 0, alive = 0, relMul = 1;
};
EraProfile eraProfile (float era);   // continuous 0..6, interpolated between eras
