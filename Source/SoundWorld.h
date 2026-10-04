#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include <array>
#include <cmath>
#include "Presets.h"
#include "DspUtil.h"

// v0.42 SOUND WORLD: the world as a map of dots. Every region has its own instruments and character;
// a dot = a sound of its region (the further from the region's heart, the more it is changed);
// two regions connected = a hybrid of both.
namespace kk::world
{
struct Region
{
    const char* name; juce::uint32 colour; float lon, lat;   // its heart
    std::vector<std::pair<int, const char*>> pool;          // (category, subcategory or "")
    const char* hint;
};

inline const std::vector<Region>& regions()
{
    static const std::vector<Region> r {
        { "NORTH AMERICA", 0xffff8a3d, -98, 42, { { cBells, "Trap Bell" }, { cKeys, "" }, { cLead, "" }, { cPiano, "Dark" }, { cSynth, "" } }, "trap bells, keys, leads - the modern sound" },
        { "LATIN AMERICA", 0xffffd23f, -62, -12, { { cWorld, "Winds" }, { cGuitar, "Nylon" }, { cGuitar, "Acoustic" }, { cMallets, "Marimba" }, { cBrass, "Trumpets" } }, "pan pipes, nylon guitar, marimba, trumpets" },
        { "CARIBBEAN", 0xff2ee6a6, -74, 18, { { cWorld, "Percussion" }, { cMallets, "" }, { cBells, "Glass" } }, "steel drums, hand pans, bright mallets" },
        { "EUROPE", 0xff4d7dff, 12, 47, { { cPiano, "Grand" }, { cPiano, "Felt" }, { cStrings, "" }, { cOrgan, "" }, { cWoodwind, "Flute" } }, "grand piano, strings, organ, flute" },
        { "NORDIC", 0xff9be7ff, 18, 66, { { cPads, "Dream" }, { cPads, "Space" }, { cChoir, "Air" }, { cChoir, "Ghost Choir" }, { cTexture, "" } }, "cold pads, airy choirs, northern light textures" },
        { "AFRICA", 0xffff4d6d, 20, 5, { { cMallets, "Kalimba" }, { cMallets, "Wooden" }, { cMallets, "Marimba" }, { cWorld, "Percussion" }, { cPlucks, "Guitar-like" } }, "kalimba, wooden mallets, warm plucks" },
        { "MIDDLE EAST", 0xffb04dff, 46, 28, { { cWorld, "Strings" }, { cWoodwind, "Ethnic Flute" }, { cPlucks, "Metallic" }, { cWorld, "Drone" } }, "zither, ethnic flute, metallic plucks, drones" },
        { "SILK ROAD", 0xffa78bfa, 70, 42, { { cWorld, "Strings" }, { cPlucks, "" }, { cWorld, "Drone" }, { cWoodwind, "" } }, "plucked strings over long drones" },
        { "INDIA", 0xffff4fd8, 79, 21, { { cWorld, "Drone" }, { cPlucks, "Metallic" }, { cPlucks, "Guitar-like" }, { cBells, "Metallic" } }, "temple drones, sitar-like plucks, metallic bells" },
        { "EAST ASIA", 0xffff5a5a, 115, 35, { { cWorld, "Strings" }, { cWorld, "Winds" }, { cBells, "Music Box" }, { cMallets, "Celesta" }, { cChip, "" } }, "koto, bamboo flute, music boxes, game chips" },
        { "ISLANDS", 0xff36ff6a, 115, 0, { { cBells, "Metallic" }, { cMallets, "Metallic" }, { cWorld, "Percussion" } }, "gamelan-like metallic bells and gongs" },
        { "SIBERIA", 0xff7c9cff, 100, 62, { { cChoir, "Male" }, { cPads, "Dark" }, { cBrass, "Dark Brass" }, { cCinematic, "" } }, "deep choirs, dark brass, frozen cinematic" },
        { "OCEANIA", 0xff22d3ee, 135, -25, { { cWorld, "Drone" }, { cTexture, "Field" }, { cTexture, "Atmosphere" }, { cPads, "Evolving" } }, "drone pipes, field textures, evolving pads" },
    };
    return r;
}

// which region a place belongs to
inline int regionAt (float lon, float lat)
{
    if (lon < -30)
    {
        if (lat >= 10 && lat <= 25 && lon > -90 && lon < -60) return 2;
        if (lon > -60 && lat > 58) return 4;          // Greenland -> NORDIC
        return lat > 14 ? 0 : 1;
    }
    if (lon < 40 && lat >= 55) return 4;
    if (lon >= -25 && lon < 40 && lat >= 36) return 3;
    if (lon >= 34 && lon < 62 && lat >= 12 && lat < 42) return 6;
    if (lon >= -25 && lon < 52 && lat < 36) return 5;
    if (lon >= 40 && lat >= 50) return 11;
    if (lon >= 52 && lon < 92 && lat >= 35) return 7;
    if (lon >= 62 && lon < 92 && lat < 35) return 8;
    if (lon >= 92 && lat >= 18) return 9;
    if (lon >= 92 && lat >= -10) return 10;
    return 12;
}

// a coarse land map: per latitude band (5 degrees) the longitude spans that are land
inline bool isLand (float lon, float lat)
{
    struct Band { int lat; std::vector<std::pair<int, int>> spans; };
    static const std::vector<Band> bands {
        { 75, { { -125, -65 }, { -55, -20 }, { 20, 60 }, { 70, 140 } } },
        { 70, { { -165, -60 }, { -50, -22 }, { 15, 30 }, { 50, 180 } } },
        { 65, { { -168, -60 }, { -52, -35 }, { -24, -13 }, { 8, 30 }, { 30, 180 } } },
        { 60, { { -165, -60 }, { -48, -42 }, { 5, 30 }, { 30, 170 } } },
        { 55, { { -165, -130 }, { -125, -58 }, { -8, 2 }, { 8, 40 }, { 40, 140 }, { 155, 163 } } },
        { 50, { { -128, -55 }, { -5, 2 }, { -2, 40 }, { 40, 142 } } },
        { 45, { { -125, -62 }, { -2, 40 }, { 40, 145 } } },
        { 40, { { -124, -72 }, { -9, 28 }, { 28, 120 }, { 125, 142 } } },
        { 35, { { -122, -76 }, { -6, 12 }, { 10, 40 }, { 44, 122 }, { 128, 140 } } },
        { 30, { { -116, -80 }, { -10, 35 }, { 35, 60 }, { 60, 122 } } },
        { 25, { { -112, -80 }, { -16, 38 }, { 40, 58 }, { 66, 92 }, { 98, 122 } } },
        { 20, { { -106, -87 }, { -78, -70 }, { -17, 40 }, { 42, 58 }, { 70, 88 }, { 94, 110 } } },
        { 15, { { -98, -84 }, { -17, 42 }, { 43, 52 }, { 74, 80 }, { 97, 110 }, { 120, 124 } } },
        { 10, { { -86, -60 }, { -16, 45 }, { 76, 80 }, { 98, 108 }, { 118, 126 } } },
        { 5, { { -78, -52 }, { -10, 10 }, { 10, 48 }, { 95, 105 }, { 110, 120 } } },
        { 0, { { -80, -48 }, { 9, 42 }, { 98, 105 }, { 110, 120 }, { 130, 140 } } },
        { -5, { { -81, -35 }, { 12, 40 }, { 105, 115 }, { 120, 140 } } },
        { -10, { { -78, -36 }, { 13, 40 }, { 120, 150 } } },
        { -15, { { -76, -38 }, { 12, 40 }, { 44, 50 }, { 125, 145 } } },
        { -20, { { -70, -40 }, { 13, 35 }, { 44, 49 }, { 114, 150 } } },
        { -25, { { -70, -45 }, { 15, 33 }, { 113, 153 } } },
        { -30, { { -72, -50 }, { 17, 31 }, { 115, 153 } } },
        { -35, { { -72, -56 }, { 18, 26 }, { 116, 150 }, { 172, 178 } } },
        { -40, { { -73, -62 }, { 145, 148 }, { 172, 176 } } },
        { -45, { { -74, -65 }, { 167, 171 } } },
        { -50, { { -75, -68 } } },
        { -55, { { -72, -66 } } },
    };
    for (auto& b : bands)
        if (std::abs (lat - (float) b.lat) <= 2.5f)
        {
            for (auto& sp : b.spans) if (lon >= (float) sp.first && lon <= (float) sp.second) return true;
            return false;
        }
    return false;
}

struct Dot { float lon, lat; int region; float bright; };

// the dots of the map (deterministic): land only, a little jitter
inline const std::vector<Dot>& dots()
{
    static const std::vector<Dot> d = []
    {
        std::vector<Dot> out;
        Rng r; r.seed (20261004u);
        for (int i = 0; i < 90000 && out.size() < 16000; ++i)
        {
            const float lon = -180.0f + 360.0f * r.uni(), lat = -57.0f + 135.0f * r.uni();
            if (! isLand (lon, lat)) continue;
            out.push_back ({ lon, lat, regionAt (lon, lat), 0.45f + 0.55f * r.uni() });
        }
        return out;
    }();
    return d;
}

// the presets of a region
inline std::vector<int> regionPresets (int region)
{
    std::vector<int> out;
    const auto& ps = factoryPresets();
    const auto& R = regions()[(size_t) juce::jlimit (0, (int) regions().size() - 1, region)];
    for (int i = 0; i < (int) ps.size(); ++i)
        for (auto& [cat, sub] : R.pool)
            if (ps[(size_t) i].cat == cat && (juce::String (sub).isEmpty() || ps[(size_t) i].sub == sub)) { out.push_back (i); break; }
    return out;
}

// how far a dot is from its region's heart (0 = heart, 1 = far away)
inline float distanceFromHeart (const Dot& d)
{
    const auto& R = regions()[(size_t) d.region];
    const float dx = (d.lon - R.lon) / 40.0f, dy = (d.lat - R.lat) / 25.0f;
    return juce::jlimit (0.0f, 1.0f, std::sqrt (dx * dx + dy * dy));
}
} // namespace kk::world
