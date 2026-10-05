#pragma once
#include <juce_core/juce_core.h>
#include <vector>
#include "Presets.h"

// v0.43 ALCHEMY: no presets to browse. A sound is made from what EXCITES the matter (a metal strike, biological friction,
// an electric short ...), the BODY that resonates (a hollow bone, compressed gas, a magnetic liquid ...) and the
// CONSISTENCY of the matter (glass ... mud) and its SIZE (tiny ... giant). The factory bank only lives inside as hidden DNA.
namespace kk::alc
{
struct Part { const char* name; const char* word; juce::uint32 colour; std::vector<std::pair<int, const char*>> dna; const char* hint; };

inline const std::vector<Part>& exciters()
{
    static const std::vector<Part> e {
        { "METAL STRIKE", "Strike", 0xffd8dde6, { { cBells, "Metallic" }, { cMallets, "Metallic" }, { cBells, "Trap Bell" }, { cBells, "Tubular" } }, "a hammer on metal - hard attack, ringing partials" },
        { "BIO FRICTION", "Friction", 0xffff6b8a, { { cStrings, "" }, { cWorld, "Strings" }, { cChoir, "" }, { cGuitar, "" } }, "hair, skin and gut rubbing - bowed, breathing, alive" },
        { "ELECTRIC SHORT", "Short", 0xff5ad1ff, { { cSynth, "" }, { cLead, "" }, { cChip, "" }, { cArp, "" } }, "a spark jumping a gap - buzzing, digital, sharp" },
        { "BREATH", "Breath", 0xffb4f0c8, { { cWoodwind, "" }, { cWorld, "Winds" }, { cPads, "Dream" } }, "air through a gap - soft, airy, flute-like" },
        { "PRESSURE WAVE", "Pressure", 0xffff8a3d, { { c808, "" }, { cBass, "" } }, "a shock wave under the floor - subs, 808s, basses" },
    };
    return e;
}

inline const std::vector<Part>& bodies()
{
    static const std::vector<Part> b {
        { "HOLLOW BONE", "Bone", 0xfff1e3c6, { { cMallets, "Wooden" }, { cMallets, "Marimba" }, { cMallets, "Kalimba" }, { cPlucks, "" } }, "dry, woody, knocking - a short warm resonance" },
        { "COMPRESSED GAS", "Gas", 0xffa78bfa, { { cPads, "" }, { cTexture, "" }, { cChoir, "Air" } }, "pressure that swells and hisses - pads and air" },
        { "MAGNETIC LIQUID", "Liquid", 0xff3dd6c6, { { cKeys, "" }, { cPads, "Evolving" }, { cPiano, "" } }, "a fluid pulled by magnets - soft, moving, keys" },
        { "CRYSTAL LATTICE", "Crystal", 0xff9be7ff, { { cBells, "Glass" }, { cBells, "Music Box" }, { cMallets, "Celesta" } }, "a lattice that sings - glassy, bright, chiming" },
        { "MOLTEN CORE", "Core", 0xffff3b5c, { { cBrass, "" }, { cCinematic, "" }, { cLead, "Distorted" } }, "hot, heavy, roaring - brass, braams, dirt" },
    };
    return b;
}

// the (hidden) bank sounds that carry the DNA of a part
inline std::vector<int> dnaPool (const Part& p)
{
    std::vector<int> out;
    const auto& ps = factoryPresets();
    for (int i = 0; i < (int) ps.size(); ++i)
        for (auto& [cat, sub] : p.dna)
            if (ps[(size_t) i].cat == cat && (juce::String (sub).isEmpty() || ps[(size_t) i].sub == sub)) { out.push_back (i); break; }
    if (out.empty()) out.push_back (0);
    return out;
}

inline juce::String matterWord (float m)
{
    static const char* w[] { "Glass", "Ice", "Silk", "Wet", "Velvet", "Wax", "Clay", "Tar", "Mud" };
    return w[juce::jlimit (0, 8, (int) std::round (m * 8.0f))];
}
inline juce::String sizeWord (float s) { return s < 0.2f ? "Tiny " : s > 0.8f ? "Giant " : ""; }
} // namespace kk::alc
