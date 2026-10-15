// Lucky Notes engine: melody generator (plain C++).
// 12 melodies per set, each with its own character. Everything is derived from (seed, key, bars, row, roll),
// so a set is fully reproducible from those values.
#pragma once

#include "Theory.h"

namespace ln
{
struct Melody
{
    int index = 0;                  // 0..11 (row)
    int character = 0;              // 0..11
    std::string characterName;      // "STEPWISE" ...
    std::vector<Note> notes;        // monophonic, sorted, all inside [0, lengthBeats]
    std::vector<Note> chords;       // sustained triads, one per bar (the optional "_Chords.mid")
    std::array<int, 4> progression {}; // chord roots as scale degrees (one per bar, repeating)
    double lengthBeats = 32.0;
};

struct SetSettings
{
    Key key;
    int bars = 8;          // 8 or 16
    uint32_t seed = 1;
    std::array<uint32_t, kNumMelodies> rolls {}; // re-roll counter per row
};

struct MelodySet
{
    SetSettings settings;
    std::array<Melody, kNumMelodies> melodies;
};

const char* characterName (int character);

Melody generateMelody (const SetSettings& s, int row);
MelodySet generateSet (const SetSettings& s);

// Key picked by LUCKY mode for a given seed.
Key luckyKey (uint32_t seed);
} // namespace ln
