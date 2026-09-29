#include "Presets.h"
#include "Params.h"
#include "PresetGains.h"
#include <array>
#include <algorithm>
#include <map>

// Factory presets. Values are real parameter values; anything not listed uses the default.
// Names are neutral descriptions only (instrument, scene, mood, year).

const juce::StringArray& categoryNames()
{
    static const juce::StringArray c { "PIANO", "KEYS", "BELLS", "PLUCKS", "MALLETS", "GUITAR", "STRINGS", "BRASS", "CHOIR / VOCAL", "WOODWIND",
                                       "LEAD", "PADS", "SYNTH", "BASS", "808", "TEXTURE", "ARP / SEQUENCE", "FX" };
    return c;
}

const juce::StringArray& subcategoryNames (int cat)
{
    static const std::array<juce::StringArray, numCategories> s {
        juce::StringArray { "Grand", "Bright", "Dark", "Soft", "Felt", "Detuned", "Lo-Fi", "Digital", "Ambient", "Processed" },
        juce::StringArray { "Electric", "Digital", "Workstation", "Dreamy", "Dark", "Ambient", "Glass", "Broken", "Hybrid", "Organ" },
        juce::StringArray { "Trap Bell", "Glass", "Tubular", "Music Box", "Digital", "Dark", "Metallic", "Distorted", "Ambient", "Hybrid" },
        juce::StringArray { "Short", "Soft", "Digital", "Metallic", "Guitar-like", "Synthetic", "Rage", "Ambient", "Hybrid" },
        juce::StringArray { "Marimba", "Kalimba", "Xylophone", "Celesta", "Metallic", "Wooden", "Hybrid" },
        juce::StringArray { "Nylon", "Acoustic", "Electric", "Clean", "Muted", "Processed", "Reverse", "Ambient", "Synth Hybrid" },
        juce::StringArray { "Ensemble", "Solo", "Pizzicato", "Staccato", "Sustained", "Synthetic", "Dark", "Cinematic", "Hybrid" },
        juce::StringArray { "Horns", "Trumpets", "Sections", "Synth Brass", "Dark Brass", "Trap Brass", "Hybrid Brass", "Hits" },
        juce::StringArray { "Male", "Female", "Mixed", "Vowels", "Air", "Vocal Texture", "Reverse Vocal", "Synthetic Vocal", "Ghost Choir" },
        juce::StringArray { "Flute", "Piccolo", "Clarinet-like", "Breath", "Ethnic Flute", "Synthetic Wind" },
        juce::StringArray { "Analog", "Digital", "Mono", "Portamento", "Rage", "Detuned", "Distorted", "Metallic", "Future" },
        juce::StringArray { "Dark", "Space", "Dream", "Analog", "Digital", "Choir", "Evolving", "Granular", "Spectral", "Future" },
        juce::StringArray { "Poly", "Analog", "Digital", "FM", "Wavetable", "Hybrid", "Spectral", "Granular" },
        juce::StringArray { "Sub", "Synth Bass", "Reese", "Distorted", "Pluck Bass", "Hybrid Bass" },
        juce::StringArray { "Clean", "Short", "Long", "Punch", "Clipped", "Distorted", "Saturated", "Glide", "Textured", "Hybrid", "Future" },
        juce::StringArray { "Vinyl", "Tape", "Noise", "Granular", "Reverse", "Atmosphere", "Field", "Mechanical", "Digital Artifacts" },
        juce::StringArray { "Melodic", "Gated", "Rhythmic", "Pulsing", "Triplet", "Polyrhythmic", "Generative" },
        juce::StringArray { "Riser", "Impact", "Reverse", "Transition", "Atmosphere", "Downer", "Noise", "Tonal FX" } };
    return s[(size_t) juce::jlimit (0, (int) numCategories - 1, cat)];
}

const juce::StringArray& eraNames()
{
    static const juce::StringArray e { "2010-12", "2013-15", "2016-18", "2019-21", "2022-24", "2025-26", "FUTURE" };
    return e;
}

const juce::StringArray& tileNames()
{
    static const juce::StringArray t { "BELLS", "KEYS", "PLUCKS", "WOODWIND", "CHOIR", "PADS", "LEADS", "808 / BASS", "ORCHESTRA", "FX" };
    return t;
}

const std::vector<int>& tileCategories (int tile)
{
    static const std::array<std::vector<int>, numTiles> t { {
        { cBells, cMallets }, { cPiano, cKeys }, { cPlucks, cGuitar }, { cWoodwind }, { cChoir }, { cPads, cTexture },
        { cLead, cSynth, cArp }, { c808, cBass }, { cStrings, cBrass }, { cFX } } };
    return t[(size_t) juce::jlimit (0, (int) numTiles - 1, tile)];
}

int tileOfCategory (int cat)
{
    for (int t = 0; t < numTiles; ++t)
        for (int c : tileCategories (t)) if (c == cat) return t;
    return tExperimental;
}

const juce::StringArray& moodNames()
{
    static const juce::StringArray m { "Dark", "Sad", "Dreamy", "Eerie", "Aggressive", "Epic", "Bouncy", "Cold", "Warm", "Bright" };
    return m;
}
const juce::StringArray& characterNames()
{
    static const juce::StringArray c { "Clean", "Digital", "Analog", "Dirty", "Broken", "Metal", "Ghost", "Dream", "Warm", "Cold", "Future" };
    return c;
}
const juce::StringArray& articulationNames()
{
    static const juce::StringArray a { "Short", "Pluck", "Sustain", "Swell", "Sequence", "One-Shot" };
    return a;
}

juce::String Preset::info() const
{
    juce::String s = categoryNames()[cat];
    if (sub.isNotEmpty()) s << " / " << sub.toUpperCase();
    s << "   " << mood;
    if (character.isNotEmpty()) s << " . " << character;
    s << " . " << articulation << (mono ? " . MONO" : "");
    if (exclusive) s << "   EXCLUSIVE";
    return s;
}

juce::String Preset::searchText() const
{
    return (name + " " + info() + " " + tempo + " " + author).toLowerCase();
}

// ERA policy - how trap sound design moved through the years (creative tags, not claims about any producer)
EraProfile eraProfile (float era)
{
    static const EraProfile p[numEras] {
        //  crush  wow  width  rev   dly  chorus drive  cut   detune rvrs ghost circ punch drift alive relMul
        {  0.12f, 0.0f, -0.15f, -0.05f, -0.05f, 0.0f, 0.0f,  0.0f, -0.1f, 0.0f, 0.0f, 0.0f, 0.15f, 0.0f, 0.0f, 0.9f },  // 2010-12 workstation: dry, 12-bit, direct
        {  0.05f, 0.0f,  0.0f,  0.08f, 0.12f, 0.1f, 0.0f, -0.5f,  0.0f, 0.0f, 0.0f, 0.0f, 0.1f,  0.0f, 0.0f, 1.1f },  // 2013-15 layered digital: darker, delays
        {  0.0f,  0.1f,  0.1f,  0.18f, 0.08f, 0.2f, 0.0f, -0.3f,  0.05f, 0.12f, 0.05f, 0.0f, 0.0f, 0.1f, 1.0f, 1.4f },  // 2016-18 atmospheric: space, reverse, chorus
        {  0.25f, 0.35f, 0.0f,  0.08f, 0.0f, 0.1f, 0.05f, -0.7f,  0.05f, 0.0f, 0.0f, 0.0f, 0.0f, 0.25f, 1.0f, 1.2f },  // 2019-21 textural: lo-fi, wow, degraded
        {  0.0f,  0.0f,  0.2f,  0.0f,  0.05f, 0.0f, 0.4f,  0.5f,  0.15f, 0.0f, 0.0f, 0.0f, 0.2f,  0.0f, 0.0f, 0.9f },  // 2022-24 rage: bright, wide, driven
        {  0.0f,  0.0f,  0.1f,  0.0f,  0.05f, 0.0f, 0.2f,  0.3f,  0.05f, 0.0f, 0.0f, 0.06f, 0.45f, 0.05f, 2.0f, 0.7f }, // 2025-26 fast: tight, punchy, animated
        {  0.0f,  0.1f,  0.2f,  0.2f,  0.05f, 0.15f, 0.0f, 0.0f,  0.1f, 0.25f, 0.25f, 0.2f, 0.0f, 0.2f, 3.0f, 1.5f } }; // FUTURE: reverse, ghost, bent, alive
    era = juce::jlimit (0.0f, 6.0f, era);
    const int a = std::min (5, (int) era); const float t = era - (float) a;
    const auto& x = p[a]; const auto& y = p[a + 1];
    auto L = [t] (float u, float w) { return u + (w - u) * t; };
    return { L (x.crush, y.crush), L (x.wow, y.wow), L (x.width, y.width), L (x.rev, y.rev), L (x.dly, y.dly), L (x.chorus, y.chorus),
             L (x.drive, y.drive), L (x.cutOct, y.cutOct), L (x.detune, y.detune), L (x.reverse, y.reverse), L (x.ghost, y.ghost),
             L (x.circuit, y.circuit), L (x.punch, y.punch), L (x.drift, y.drift), L (x.alive, y.alive), L (x.relMul, y.relMul) };
}

namespace
{
using Vals = std::vector<std::pair<juce::String, float>>;
float getV (const Vals& vals, const juce::String& id, float def) { for (auto& [k, x] : vals) if (k == id) return x; return def; }
void setV (Vals& vals, const juce::String& id, float x) { for (auto& [k, y] : vals) if (k == id) { y = x; return; } vals.push_back ({ id, x }); }

// bake an era's character into a neutral recipe
void bakeEra (Vals& v, int era, bool bass)
{
    using namespace ID;
    const auto p = eraProfile ((float) era);
    auto add = [&] (const char* id, float def, float d, float lo = 0.0f, float hi = 1.0f) { setV (v, id, juce::jlimit (lo, hi, getV (v, id, def) + d)); };
    add (crush, 0, p.crush); add (wow, 0, bass ? p.wow * 0.3f : p.wow); add (width, bass ? 0.4f : 0.5f, bass ? 0.0f : p.width);
    add (revMix, bass ? 0.0f : 0.15f, bass ? 0.0f : p.rev); add (delayMix, 0, bass ? 0.0f : p.dly); add (chorus, 0, bass ? 0.0f : p.chorus);
    add (drive, 0, p.drive); add (detune, 0.2f, p.detune); add (reverse, 0, bass ? 0.0f : p.reverse); add (ghost, 0, bass ? 0.0f : p.ghost);
    add (circuit, 0, p.circuit); add (punch, 0, p.punch); add (drift, 0, bass ? 0.0f : p.drift);
    add (alive, 0, bass ? 0.0f : std::round (p.alive), 0.0f, 5.0f);
    setV (v, cutoff, juce::jlimit (60.0f, 20000.0f, getV (v, cutoff, 12000.0f) * std::exp2 (p.cutOct)));
    setV (v, release, juce::jlimit (0.005f, 8.0f, getV (v, release, 0.3f) * p.relMul));
    if (era == 2 || era == eraFuture) if (getV (v, revType, 0) == 0 && ! bass) setV (v, revType, 2);   // cloud reverb
    setV (v, ID::era, (float) era); setV (v, eraHome, (float) era);
}

// derive browser metadata from the sound itself
void deriveTags (Preset& p)
{
    using namespace ID;
    const auto& v = p.values;
    const auto n = p.name.toLowerCase();
    auto has = [&] (std::initializer_list<const char*> words) { for (auto* w : words) if (n.contains (w)) return true; return false; };
    const float cut = getV (v, cutoff, 12000), drv = getV (v, drive, 0) + (p.isBass() ? getV (v, m3, 0) : 0), cr = getV (v, crush, 0);
    const float rv = getV (v, revMix, 0.15f), att = getV (v, attack, 0.002f), sus = getV (v, sustain, 0.7f), dec = getV (v, decay, 0.4f);
    const int eng = (int) getV (v, engine, 0);

    if (has ({ "dark", "horror", "gothic", "midnight", "basement", "night", "void", "underground" }) || cut < 2000) p.mood = "Dark";
    else if (has ({ "sad", "emo", "lonely", "cry" })) p.mood = "Sad";
    else if (has ({ "haunted", "ghost", "eerie", "mystery", "spell", "music box" })) p.mood = "Eerie";
    else if (has ({ "rage", "blown", "hyper", "aggressive", "distort", "crush", "clip", "fuzz", "grit", "jerk" }) || drv > 0.45f) p.mood = "Aggressive";
    else if (has ({ "epic", "cinematic", "braam", "tutti", "orchestra", "horn", "brass" })) p.mood = "Epic";
    else if (has ({ "dream", "vapor", "cloud", "ethereal", "heaven", "space", "air", "ambient" }) || rv > 0.4f) p.mood = "Dreamy";
    else if (has ({ "bounce", "bouncy", "plugg", "club" })) p.mood = "Bouncy";
    else if (has ({ "glass", "ice", "cold", "frozen", "metal" })) p.mood = "Cold";
    else if (has ({ "warm", "soft", "felt", "tape", "nylon" })) p.mood = "Warm";
    else p.mood = cut > 6000 ? "Bright" : "Warm";

    juce::StringArray ch;
    if (getV (v, ghost, 0) > 0.2f) ch.add ("Ghost");
    if (getV (v, circuit, 0) > 0.2f || getV (v, tape, 0) > 0.5f || getV (v, wow, 0) > 0.4f) ch.add ("Broken");
    if (drv > 0.35f || cr > 0.3f) ch.add ("Dirty");
    if (getV (v, body, 0) == 4 || eng == engModal) ch.add ("Metal");
    if (p.era == eraFuture) ch.add ("Future");
    if (eng == engFM || eng == engWavetable) ch.add ("Digital");
    else if (eng == engVA) ch.add ("Analog");
    if (rv > 0.45f) ch.add ("Dream");
    if (ch.isEmpty()) ch.add (drv < 0.05f && cr < 0.05f ? "Clean" : "Warm");
    while (ch.size() > 2) ch.remove (2);
    p.character = ch.joinIntoString (" . ");

    const bool arp = getV (v, ID::arp, 0) > 0.5f;
    if (arp) p.articulation = "Sequence";
    else if (att > 0.25f) p.articulation = "Swell";
    else if (sus < 0.05f && dec < 0.6f) p.articulation = "Short";
    else if (sus < 0.05f) p.articulation = "Pluck";
    else p.articulation = "Sustain";
    if (p.cat == cFX) p.articulation = "One-Shot";
    p.tempo = p.articulation == "Short" || p.articulation == "Sequence" ? "Fast" : p.articulation == "Swell" ? "Slow" : "Any";

    p.mono = getV (v, mono, 0) > 0.5f || p.isBass();
    p.brightness = juce::jlimit (1, 5, (int) std::round (1.0f + 4.0f * juce::jlimit (0.0f, 1.0f, std::log2 (cut / 500.0f) / 5.0f) + drv));
    const float mv = getV (v, lfoPitch, 0) * 4 + getV (v, lfoFilter, 0) + getV (v, lfoAmp, 0) + getV (v, wow, 0) + getV (v, chorus, 0)
                   + getV (v, alive, 0) * 0.2f + getV (v, drift, 0) + getV (v, circuit, 0) + (arp ? 1.5f : 0.0f) + getV (v, reverse, 0);
    p.movement = juce::jlimit (0, 5, (int) std::round (mv * 2.0f));
    const float uni = getV (v, unison, 1) + (getV (v, layerB, 0) > 0.5f ? getV (v, unisonB, 1) : 0);
    const float heavy = (eng == engOrchestral || eng == engVox ? 2.0f : 0.0f) + getV (v, reverse, 0) * 2 + getV (v, ghost, 0) * 2;
    p.cpu = juce::jlimit (1, 3, 1 + (int) ((uni + heavy) / 5.0f));
}

// place the v0.4 presets into the new taxonomy
void classify (Preset& p, int oldTile)
{
    const auto n = p.name.toLowerCase();
    const auto old = p.sub;
    auto has = [&] (std::initializer_list<const char*> words) { for (auto* w : words) if (n.contains (w)) return true; return false; };
    auto set = [&] (int c, const char* s) { p.cat = c; p.sub = s; };
    if (p.era < 0) p.era = eraFuture;

    if (old == "Arps") { set (cArp, has ({ "harp" }) ? "Melodic" : has ({ "rage", "trance" }) ? "Pulsing" : "Melodic"); return; }
    if (old == "808" || (oldTile == tBass && n.contains ("808")))
    {
        set (c808, has ({ "clean", "sine" }) ? "Clean" : has ({ "tight", "punch", "knock" }) ? "Punch" : has ({ "glide", "slide" }) ? "Glide"
                 : has ({ "distorted", "rage" }) ? "Distorted" : has ({ "folded", "grit" }) ? "Clipped" : has ({ "tape", "warm" }) ? "Saturated" : "Long");
        return;
    }
    if (oldTile == tBass)
    {
        set (cBass, has ({ "reese" }) ? "Reese" : has ({ "sub" }) ? "Sub" : has ({ "pluck" }) ? "Pluck Bass"
                  : has ({ "blown", "crush", "dirt", "broken", "bent", "growl", "metal" }) ? "Distorted"
                  : has ({ "wobble", "wt ", "fm ", "formant", "acid" }) ? "Hybrid Bass" : "Synth Bass");
        return;
    }
    if (old == "Piano")
    {
        set (cPiano, has ({ "lo-fi", "loop" }) ? "Lo-Fi" : has ({ "detuned" }) ? "Detuned" : has ({ "dark", "sad", "minimal" }) ? "Dark"
                   : has ({ "bounc", "hyper" }) ? "Bright" : has ({ "ghost", "reverse" }) ? "Ambient"
                   : has ({ "chop", "formant", "half", "rhodes", "wobble", "toy", "slide" }) ? "Processed" : "Grand");
        if (has ({ "rhodes" })) set (cKeys, has ({ "reverse" }) ? "Broken" : "Electric");
        if (has ({ "toy" })) set (cPiano, "Digital");
        return;
    }
    if (old == "Organs") { set (cKeys, "Organ"); return; }
    if (old == "Strings")
    {
        set (cStrings, has ({ "pizz" }) ? "Pizzicato" : has ({ "staccato", "marcato", "hit", "stab" }) ? "Staccato"
                     : has ({ "cello", "violin lead" }) ? "Solo" : has ({ "epic", "tutti", "timpani" }) ? "Cinematic"
                     : has ({ "horror", "drill" }) ? "Dark" : has ({ "morph" }) ? "Hybrid" : has ({ "tape", "warped" }) ? "Synthetic"
                     : has ({ "tremolo" }) ? "Sustained" : "Ensemble");
        return;
    }
    if (old == "Brass")
    {
        set (cBrass, has ({ "braam", "reese", "blown" }) ? "Hybrid Brass" : has ({ "synth" }) ? "Synth Brass"
                   : has ({ "dark", "low" }) ? "Dark Brass" : has ({ "trap", "stab", "octave" }) ? "Trap Brass"
                   : has ({ "horn" }) ? "Horns" : "Sections");
        return;
    }
    if (old == "Guitars")
    {
        set (cGuitar, has ({ "nylon" }) ? "Nylon" : has ({ "acoustic" }) ? "Acoustic" : has ({ "distorted" }) ? "Electric"
                    : has ({ "clean", "emo" }) ? "Clean" : "Processed");
        return;
    }
    if (old == "Mallets")
    {
        if (has ({ "music box" })) { set (cBells, "Music Box"); return; }
        set (cMallets, has ({ "kalimba" }) ? (has ({ "bent" }) ? "Hybrid" : "Kalimba") : has ({ "marimba" }) ? "Marimba"
                     : has ({ "xylo" }) ? "Xylophone" : has ({ "celeste", "celesta" }) ? "Celesta" : "Metallic");
        return;
    }
    if (old == "Texture")
    {
        if (has ({ "choir" })) { set (cChoir, "Vocal Texture"); return; }
        if (has ({ "strings" })) { set (cPads, "Evolving"); return; }
        set (cTexture, has ({ "granular", "grain" }) ? "Granular" : has ({ "cassette", "tape", "half" }) ? "Tape" : has ({ "hiss" }) ? "Noise"
                     : has ({ "metal" }) ? "Mechanical" : "Atmosphere");
        return;
    }
    if (old == "FX")
    {
        set (cFX, has ({ "riser" }) ? "Riser" : has ({ "impact" }) ? "Impact" : has ({ "downer", "stop" }) ? "Downer"
                : has ({ "reverse" }) ? "Reverse" : has ({ "glitch" }) ? "Noise" : "Tonal FX");
        return;
    }
    switch (oldTile)
    {
        case tBells:
            set (cBells, has ({ "music box" }) ? "Music Box" : has ({ "glass", "ice", "glassy", "glock" }) ? "Glass"
                       : has ({ "crush", "blown", "folded" }) ? "Distorted" : has ({ "tower", "modal" }) ? "Tubular"
                       : has ({ "wavetable", "plugg", "digital" }) ? "Digital" : has ({ "dark" }) ? "Dark"
                       : has ({ "sad", "haunted" }) ? "Ambient" : has ({ "ghost", "rise", "arp" }) ? "Hybrid" : "Trap Bell");
            if (has ({ "glockenspiel" })) set (cMallets, "Metallic");
            break;
        case tKeys:
            set (cKeys, has ({ "tine", "ep ", "detroit" }) ? "Electric" : has ({ "harpsichord", "classic" }) ? "Workstation"
                      : has ({ "dream" }) ? "Dreamy" : has ({ "broken", "tape", "chop", "slice", "reverse" }) ? "Broken"
                      : has ({ "glass" }) ? "Glass" : has ({ "pipe", "supertrap", "vocal", "formant" }) ? "Hybrid"
                      : has ({ "dark" }) ? "Dark" : "Digital");
            if (has ({ "melting" })) set (cPiano, "Processed");
            break;
        case tPlucks:
            set (cPlucks, has ({ "harp" }) ? "Guitar-like" : has ({ "digicore", "wt ", "glassy" }) ? "Digital" : has ({ "hyper" }) ? "Rage"
                        : has ({ "plugg" }) ? "Soft" : has ({ "rubber" }) ? "Synthetic" : has ({ "mystery" }) ? "Digital" : "Short");
            break;
        case tFlutes:
            set (cWoodwind, has ({ "pan", "bamboo", "shrine", "ethnic", "ocarina" }) ? "Ethnic Flute"
                          : has ({ "whistle", "glass", "pipe", "circuit", "bitcrush", "distorted" }) ? "Synthetic Wind" : "Flute");
            break;
        case tChoir:
            set (cChoir, has ({ "ooh", "aah" }) ? "Vowels" : has ({ "ghost", "gothic", "dark choir" }) ? "Ghost Choir"
                       : has ({ "synth", "glass", "bent" }) ? "Synthetic Vocal" : has ({ "opera" }) ? "Male"
                       : has ({ "ethereal", "tape", "bell" }) ? "Vocal Texture" : "Mixed");
            break;
        case tPads:
            set (cPads, has ({ "cloud", "frozen" }) ? "Space" : has ({ "vapor", "dream" }) ? "Dream" : has ({ "motion", "morph", "era" }) ? "Evolving"
                      : has ({ "trance", "neon", "glass" }) ? "Digital" : has ({ "tape" }) ? "Analog" : has ({ "reverse", "phaser" }) ? "Future" : "Dark");
            if (has ({ "orchestra" })) set (cStrings, "Dark");
            break;
        case tLeads:
            set (cLead, has ({ "rage" }) ? "Rage" : has ({ "supersaw" }) ? "Detuned" : has ({ "blown", "clip" }) ? "Distorted"
                      : has ({ "glide", "octave jump" }) ? "Portamento" : has ({ "hyper", "sync", "wt " }) ? "Digital" : "Analog");
            if (has ({ "jerk", "glitch" })) set (cSynth, "Digital");
            if (has ({ "early plugg synth" })) set (cSynth, "Poly");
            if (has ({ "stack fm" })) set (cSynth, "FM");
            if (has ({ "kalimba supersaw", "wood box" })) set (cSynth, "Hybrid");
            break;
        case tOrchestra:
            set (cPlucks, "Guitar-like");
            if (has ({ "harp" })) set (cPlucks, "Guitar-like");
            if (has ({ "vibraphone", "steel" })) set (cMallets, "Metallic");
            if (has ({ "xylo" })) set (cMallets, "Xylophone");
            break;
        default:   // experimental
            set (cTexture, "Atmosphere");
            if (has ({ "melting" })) set (cPiano, "Processed");
            if (has ({ "keys" })) set (cKeys, "Broken");
            if (has ({ "void", "cloud" })) set (cPads, "Future");
            if (has ({ "radio" })) set (cTexture, "Digital Artifacts");
            if (has ({ "pluck" })) set (cPlucks, n.contains ("rubber") ? "Synthetic" : "Digital");
            break;
    }
}
} // namespace

const std::vector<Preset>& factoryPresets()
{
    using namespace ID;
    static const std::vector<Preset> presets = []
    {
        std::vector<Preset> v;
        using Vals = std::vector<std::pair<juce::String, float>>;
        auto P = [&v] (const char* name, int tile, int era, bool excl, const char* sub, Vals vals)
        { v.push_back ({ name, tile, era, excl, sub, std::move (vals), {} }); };

        const float FM = engFM, VA = engVA, PL = engPluck, VX = engVox, OR = engOrgan, FL = engFlute, SB = engSub;
        const float WT = engWavetable, OC = engOrchestral, MD = engModal;
        // mod matrix helper: slot, source, dest, amount
        auto MM = [] (Vals& vals, int slot, int src, int dst, float amt)
        {
            vals.push_back ({ ID::mmSrc (slot), (float) src }); vals.push_back ({ ID::mmDst (slot), (float) dst }); vals.push_back ({ ID::mmAmt (slot), amt });
        };

        // ---------------- BELLS ----------------
        P ("Dark Digital Bells 2010", tBells, 0, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.45f }, { fdecay, 1.0f },
            { attack, 0.001f }, { decay, 2.2f }, { sustain, 0 }, { release, 1.2f }, { revMix, 0.2f }, { m2, 0.2f }, { gain, -7.0f } });
        P ("Ice Bells 2013", tBells, 1, false, "", { { engine, FM }, { fmRatio, 4.0f }, { fmAmt, 0.5f }, { fdecay, 1.6f }, { unison, 2 }, { detune, 0.15f },
            { decay, 3.0f }, { sustain, 0 }, { release, 2.0f }, { revMix, 0.35f }, { revSize, 0.85f }, { delayMix, 0.2f }, { m2, 0.4f }, { m6, 0.7f,  }, { gain, -7.1f } });
        P ("Sad Bells", tBells, 2, false, "", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.35f }, { fdecay, 0.8f }, { octave, 1 },
            { decay, 2.5f }, { sustain, 0 }, { release, 1.5f }, { revMix, 0.3f }, { wow, 0.2f }, { m1, 0.4f }, { m2, 0.35f,  }, { gain, -6.9f } });
        P ("Internet Blown Bell", tBells, 2, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.4f }, { decay, 2.0f }, { sustain, 0 },
            { drive, 0.55f }, { driveType, 3 }, { revMix, 0.2f }, { m3, 0.3f }, { gain, -11.5f } });
        P ("Glassy Plugg", tBells, 3, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { wave, 0.3f }, { fdecay, 0.5f }, { octave, 1 },
            { decay, 1.4f }, { sustain, 0.15f }, { release, 0.8f }, { chorus, 0.3f }, { revMix, 0.3f }, { delayMix, 0.2f }, { m2, 0.35f,  }, { gain, -5.7f } });
        P ("Crushed Dark Bells", tBells, 4, false, "", { { engine, FM }, { fmRatio, 5.2f }, { fmAmt, 0.55f }, { fdecay, 0.9f },
            { decay, 2.0f }, { sustain, 0 }, { release, 1.4f }, { crush, 0.4f }, { drive, 0.35f }, { driveType, 2 }, { m1, 0.35f }, { revMix, 0.25f,  }, { gain, -13.3f } });
        P ("Music Box Void", tBells, 5, false, "", { { engine, FM }, { fmRatio, 6.0f }, { fmAmt, 0.25f }, { octave, 2 }, { decay, 1.5f }, { sustain, 0 },
            { release, 1.8f }, { revMix, 0.45f }, { revSize, 0.95f }, { ghost, 0.35f }, { wow, 0.3f }, { m2, 0.4f,  }, { gain, -4.3f } });

        // ---------------- KEYS ----------------
        P ("Bounce Piano", tKeys, 0, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.35f }, { wave, 0.2f }, { fdecay, 0.6f },
            { decay, 1.8f }, { sustain, 0.05f }, { release, 0.4f }, { cutoff, 9000 }, { fenv, 0.2f }, { revMix, 0.15f }, { gain, -5.3f } });
        P ("Church Organ Bounce", tKeys, 0, false, "Organs", { { engine, OR }, { wave, 0.55f }, { attack, 0.01f }, { sustain, 1.0f }, { release, 0.3f },
            { lfoPitch, 0.04f }, { lfoRate, 6.0f }, { revMix, 0.35f }, { revSize, 0.9f }, { gain, -8.9f } });
        P ("Dark Minimal Keys", tKeys, 1, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.2f }, { decay, 2.5f }, { sustain, 0.2f },
            { release, 0.8f }, { cutoff, 3500 }, { revMix, 0.3f }, { m1, 0.4f }, { m2, 0.35f,  }, { gain, -6.4f } });
        P ("Dark Piano Loop", tKeys, 2, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.4f }, { wave, 0.35f }, { fdecay, 0.4f },
            { decay, 2.0f }, { sustain, 0.0f }, { release, 0.6f }, { cutoff, 6000 }, { wow, 0.25f }, { crush, 0.1f }, { revMix, 0.25f }, { m1, 0.4f,  }, { gain, -4.7f } });
        P ("Detroit Keys", tKeys, 3, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.45f }, { fdecay, 0.3f }, { decay, 0.9f },
            { sustain, 0.1f }, { release, 0.3f }, { drive, 0.2f }, { driveType, 1 }, { revMix, 0.12f,  }, { gain, -9.8f } });
        P ("Pluggnb Keys", tKeys, 3, false, "", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.25f }, { unison, 3 }, { detune, 0.12f },
            { decay, 1.6f }, { sustain, 0.3f }, { release, 0.9f }, { chorus, 0.5f }, { revMix, 0.35f }, { m5, 0.3f }, { m6, 0.7f,  }, { gain, -6.3f } });
        P ("UK Drill Slide Piano", tKeys, 3, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.8f }, { sustain, 0.1f },
            { release, 0.5f }, { mono, 1 }, { glide, 0.12f }, { revMix, 0.2f,  }, { gain, -2.2f } });
        P ("Rage Keys 2024", tKeys, 4, false, "", { { engine, VA }, { wave, 0.3f }, { unison, 5 }, { detune, 0.35f }, { cutoff, 5000 }, { fenv, 0.4f }, { fdecay, 0.3f },
            { decay, 0.8f }, { sustain, 0.4f }, { release, 0.3f }, { drive, 0.3f }, { revMix, 0.2f }, { gain, -9.7f } });

        // ---------------- PLUCKS ----------------
        P ("Pizz Dark", tPlucks, 2, false, "Strings", { { engine, PL }, { wave, 0.3f }, { decay, 0.5f }, { sustain, 0 }, { release, 0.2f },
            { cutoff, 5000 }, { revMix, 0.3f }, { m1, 0.45f,  }, { gain, 5.2f } });
        P ("Emo Guitar Pick", tPlucks, 2, false, "Guitars", { { engine, PL }, { wave, 0.6f }, { decay, 2.0f }, { sustain, 0 }, { release, 0.4f },
            { chorus, 0.35f }, { revMix, 0.3f }, { drive, 0.1f }, { gain, -4.0f } });
        P ("Sample Guitar Vibe", tPlucks, 3, false, "Guitars", { { engine, PL }, { wave, 0.5f }, { decay, 2.5f }, { sustain, 0 }, { release, 0.6f },
            { wow, 0.35f }, { crush, 0.12f }, { revMix, 0.3f }, { m4, 0.2f,  }, { gain, 0.2f } });
        P ("Digicore Pluck", tPlucks, 4, false, "", { { engine, VA }, { wave, 0.2f }, { unison, 3 }, { detune, 0.25f }, { cutoff, 1500 }, { fenv, 0.7f },
            { fdecay, 0.15f }, { decay, 0.25f }, { sustain, 0 }, { release, 0.2f }, { octave, 1 }, { delayMix, 0.25f }, { revMix, 0.2f }, { gain, 9.3f } });
        P ("Hyper Pluck", tPlucks, 5, false, "", { { engine, VA }, { wave, 0.8f }, { unison, 5 }, { detune, 0.3f }, { cutoff, 2000 }, { fenv, 0.8f },
            { fdecay, 0.12f }, { decay, 0.2f }, { sustain, 0 }, { release, 0.15f }, { octave, 1 }, { drive, 0.35f }, { driveType, 2 }, { delayMix, 0.3f }, { gain, -7.7f } });
        P ("Early Plugg Pluck", tPlucks, 1, false, "", { { engine, VA }, { wave, 0.5f }, { cutoff, 2500 }, { fenv, 0.5f }, { fdecay, 0.2f },
            { decay, 0.4f }, { sustain, 0.1f }, { release, 0.3f }, { revMix, 0.3f }, { delayMix, 0.2f,  }, { gain, -8.4f } });

        // ---------------- FLUTES ----------------
        P ("Old Trap Flute", tFlutes, 0, false, "", { { engine, FL }, { wave, 0.3f }, { attack, 0.03f }, { sustain, 0.9f }, { release, 0.3f },
            { lfoPitch, 0.1f }, { lfoRate, 5.5f }, { crush, 0.15f }, { revMix, 0.25f,  }, { gain, -12.2f } });
        P ("Trap Flute 2016", tFlutes, 2, false, "", { { engine, FL }, { wave, 0.45f }, { attack, 0.04f }, { sustain, 0.85f }, { release, 0.35f },
            { lfoPitch, 0.14f }, { lfoRate, 5.0f }, { revMix, 0.3f }, { delayMix, 0.15f }, { m2, 0.35f,  }, { gain, -12.1f } });
        P ("Whistle Lead ATL", tFlutes, 0, false, "", { { engine, FL }, { wave, 0.1f }, { octave, 1 }, { attack, 0.02f }, { sustain, 1.0f },
            { mono, 1 }, { glide, 0.08f }, { lfoPitch, 0.12f }, { revMix, 0.25f }, { gain, -7.2f } });
        P ("Pan Flute Night", tFlutes, 3, false, "", { { engine, FL }, { wave, 0.7f }, { attack, 0.06f }, { sustain, 0.8f }, { release, 0.5f },
            { lfoPitch, 0.08f }, { revMix, 0.4f }, { revSize, 0.85f }, { wow, 0.2f,  }, { gain, -12.7f } });
        P ("Ocarina 2025", tFlutes, 5, false, "", { { engine, FL }, { wave, 0.2f }, { octave, 1 }, { attack, 0.02f }, { sustain, 0.9f }, { release, 0.4f },
            { bend, 0.25f }, { revMix, 0.35f }, { delayMix, 0.25f }, { ghost, 0.2f,  }, { gain, -14.1f } });

        // ---------------- CHOIR ----------------
        P ("Dark Choir Stab", tChoir, 0, false, "", { { engine, VX }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.01f },
            { decay, 0.6f }, { sustain, 0.3f }, { release, 0.5f }, { octave, -1 }, { revMix, 0.4f }, { revSize, 0.9f }, { crush, 0.1f,  }, { gain, 6.4f } });
        P ("Ethereal Vox", tChoir, 1, false, "", { { engine, VX }, { wave, 0.3f }, { unison, 5 }, { detune, 0.35f }, { attack, 0.4f },
            { sustain, 0.9f }, { release, 1.5f }, { lfoPitch, 0.05f }, { chorus, 0.5f }, { revMix, 0.5f }, { revSize, 0.95f }, { m6, 0.8f,  }, { gain, -1.3f } });
        P ("Ooh Aah Choir", tChoir, 2, false, "", { { engine, VX }, { wave, 0.75f }, { unison, 7 }, { detune, 0.25f }, { attack, 0.2f },
            { sustain, 0.9f }, { release, 1.0f }, { revMix, 0.4f }, { m5, 0.3f,  }, { gain, -2.0f } });
        P ("Ghost Vox Lead", tChoir, 5, false, "", { { engine, VX }, { wave, 0.5f }, { unison, 3 }, { detune, 0.2f }, { mono, 1 }, { glide, 0.1f },
            { attack, 0.05f }, { sustain, 0.9f }, { ghost, 0.4f }, { bend, 0.2f }, { revMix, 0.35f }, { delayMix, 0.3f,  }, { gain, 5.3f } });

        // ---------------- PADS (+Strings) ----------------
        P ("Horror Strings", tPads, 0, false, "Strings", { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { cutoff, 3500 },
            { attack, 0.3f }, { sustain, 0.9f }, { release, 1.2f }, { lfoPitch, 0.06f }, { revMix, 0.4f }, { revSize, 0.9f,  }, { gain, -10.1f } });
        P ("Cloud Pad", tPads, 1, false, "", { { engine, VA }, { wave, 0.1f }, { unison, 7 }, { detune, 0.4f }, { cutoff, 2200 }, { attack, 0.8f },
            { sustain, 1.0f }, { release, 2.5f }, { chorus, 0.5f }, { revMix, 0.55f }, { revSize, 0.95f }, { m5, 0.4f }, { m6, 0.8f,  }, { gain, -4.4f } });
        P ("Vapor Chords", tPads, 1, false, "", { { engine, VA }, { wave, 0.4f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 2800 }, { attack, 0.05f },
            { sustain, 0.8f }, { release, 1.0f }, { wow, 0.45f }, { chorus, 0.4f }, { revMix, 0.4f }, { m4, 0.3f,  }, { gain, -10.5f } });
        P ("NY Drill Strings", tPads, 3, false, "Strings", { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.25f }, { cutoff, 5000 },
            { attack, 0.08f }, { sustain, 0.9f }, { release, 0.5f }, { mono, 1 }, { glide, 0.15f }, { revMix, 0.3f,  }, { gain, -5.3f } });
        P ("Static Void Pad", tPads, 4, false, "", { { engine, VX }, { wave, 0.9f }, { unison, 7 }, { detune, 0.5f }, { attack, 1.0f }, { sustain, 1 },
            { release, 3.0f }, { crush, 0.25f }, { wow, 0.3f }, { revMix, 0.6f }, { revSize, 0.98f }, { ghost, 0.3f,  }, { gain, -7.5f } });
        P ("Neon Trance Chords", tPads, 5, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.45f }, { cutoff, 3000 }, { fenv, 0.3f },
            { fdecay, 0.25f }, { attack, 0.003f }, { decay, 0.4f }, { sustain, 0.5f }, { release, 0.3f }, { lfoAmp, 0.6f }, { lfoSync, 1 }, { lfoDiv, 4 },
            { delayMix, 0.25f }, { revMix, 0.35f }, { gain, -1.5f } });

        // ---------------- LEADS (+Brass) ----------------
        P ("Epic Brass 2010", tLeads, 0, false, "Brass", { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.15f }, { cutoff, 900 }, { fenv, 0.6f },
            { fdecay, 0.3f }, { attack, 0.02f }, { sustain, 0.8f }, { release, 0.25f }, { revMix, 0.35f }, { revSize, 0.9f }, { crush, 0.1f,  }, { gain, -7.4f } });
        P ("Block Brass 2012", tLeads, 0, false, "Brass", { { engine, VA }, { wave, 0.15f }, { unison, 3 }, { detune, 0.12f }, { cutoff, 1200 }, { fenv, 0.5f },
            { fdecay, 0.2f }, { decay, 0.3f }, { sustain, 0.4f }, { release, 0.15f }, { octave, -1 }, { revMix, 0.3f,  }, { gain, 5.9f } });
        P ("Early Plugg Synth", tLeads, 1, false, "", { { engine, VA }, { wave, 0.5f }, { cutoff, 3500 }, { sustain, 0.8f }, { release, 0.4f },
            { mono, 1 }, { glide, 0.06f }, { lfoPitch, 0.05f }, { revMix, 0.3f }, { delayMix, 0.25f,  }, { gain, -7.4f } });
        P ("Early Rage Saw", tLeads, 3, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { cutoff, 6000 },
            { sustain, 0.9f }, { release, 0.25f }, { drive, 0.25f }, { revMix, 0.25f }, { gain, -14.7f } });
        P ("Rage Supersaw", tLeads, 4, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.5f }, { cutoff, 9000 },
            { sustain, 0.9f }, { release, 0.3f }, { drive, 0.35f }, { driveType, 1 }, { chorus, 0.2f }, { revMix, 0.3f }, { m6, 0.8f }, { gain, -14.8f } });
        P ("Jerk Synth", tLeads, 4, false, "", { { engine, VA }, { wave, 0.7f }, { unison, 3 }, { detune, 0.15f }, { cutoff, 2500 }, { fenv, 0.5f },
            { fdecay, 0.1f }, { decay, 0.2f }, { sustain, 0.2f }, { release, 0.1f }, { drive, 0.3f }, { driveType, 2 }, { gain, -11.1f } });
        P ("Hyper Lead", tLeads, 4, false, "", { { engine, VA }, { wave, 0.6f }, { unison, 5 }, { detune, 0.3f }, { octave, 1 }, { cutoff, 8000 },
            { mono, 1 }, { glide, 0.05f }, { lfoPitch, 0.08f }, { drive, 0.3f }, { crush, 0.15f }, { delayMix, 0.25f }, { gain, -14.7f } });
        P ("Blown Rage Lead", tLeads, 5, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.45f }, { cutoff, 7000 },
            { sustain, 0.9f }, { release, 0.3f }, { drive, 0.7f }, { driveType, 3 }, { revMix, 0.2f }, { m3, 0.3f }, { m6, 0.8f }, { gain, -11.7f } });
        P ("Club Rage Stab", tLeads, 5, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { cutoff, 2500 }, { fenv, 0.6f },
            { fdecay, 0.15f }, { decay, 0.25f }, { sustain, 0.0f }, { release, 0.2f }, { drive, 0.3f }, { delayMix, 0.3f }, { revMix, 0.3f }, { gain, -7.8f } });
        P ("Glitch Stab", tLeads, 5, false, "", { { engine, VA }, { wave, 0.8f }, { unison, 3 }, { detune, 0.3f }, { cutoff, 3000 }, { fenv, 0.5f },
            { decay, 0.3f }, { sustain, 0.1f }, { circuit, 0.45f }, { crush, 0.2f }, { revMix, 0.2f }, { gain, -5.7f } });

        // ---------------- BASS (mono + bass mode added automatically) ----------------
        auto B = [&] (const char* name, int era, Vals vals, bool excl = false)
        {
            Vals base { { m1, 0.0f }, { m2, 0.0f }, { m3, 0.0f }, { m4, 0.0f }, { m5, 0.5f }, { m6, 0.0f },
                                                              { revMix, 0.0f }, { width, 0.4f }, { octave, -1 } };
            base.insert (base.end(), vals.begin(), vals.end());
            P (name, tBass, era, excl, "", base);
        };
        B ("Pure Sub", 0, { { engine, SB }, { wave, 0.0f }, { sustain, 1 }, { release, 0.2f,  }, { gain, -5.5f } });
        B ("Warm Sub", 2, { { engine, SB }, { wave, 0.35f }, { sustain, 1 }, { release, 0.25f }, { m3, 0.1f,  }, { gain, -8.9f } });
        B ("Synth 808 Long", 2, { { engine, SB }, { wave, 0.4f }, { fmAmt, 0.15f }, { decay, 3.5f }, { sustain, 0 }, { release, 0.5f }, { m6, 0.3f,  }, { gain, -6.6f } });
        B ("Synth 808 Punch", 3, { { engine, SB }, { wave, 0.6f }, { fmAmt, 0.3f }, { decay, 1.2f }, { sustain, 0 }, { release, 0.2f }, { m3, 0.35f }, { m6, 0.6f,  }, { gain, -10.3f } });
        B ("Slide 808", 3, { { engine, SB }, { wave, 0.5f }, { fmAmt, 0.12f }, { decay, 4.0f }, { sustain, 0.2f }, { release, 0.4f }, { glide, 0.15f }, { m4, 0.3f,  }, { gain, -6.8f } });
        B ("Dark Reese", 3, { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.35f }, { cutoff, 700 }, { sustain, 1 }, { release, 0.2f }, { m1, 0.4f,  }, { gain, -2.0f } });
        B ("Drill Reese", 3, { { engine, VA }, { wave, 0.0f }, { unison, 2 }, { detune, 0.4f }, { cutoff, 900 }, { sustain, 1 }, { glide, 0.1f }, { m1, 0.5f }, { m3, 0.2f,  }, { gain, -6.7f } });
        B ("Wobble 1/8", 4, { { engine, VA }, { wave, 0.1f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 3000 }, { reso, 0.35f }, { sustain, 1 },
                               { lfoSync, 1 }, { lfoDiv, 3 }, { m1, 0.5f }, { m2, 0.7f }, { m3, 0.25f,  }, { gain, -3.9f } });
        B ("Triplet Wobble", 4, { { engine, VA }, { wave, 0.4f }, { unison, 2 }, { detune, 0.2f }, { cutoff, 2500 }, { reso, 0.4f }, { sustain, 1 },
                               { lfoSync, 1 }, { lfoDiv, 7 }, { m1, 0.5f }, { m2, 0.75f }, { m3, 0.3f,  }, { gain, -8.5f } });
        B ("Plugg Square Bass", 3, { { engine, VA }, { wave, 0.5f }, { cutoff, 1200 }, { fenv, 0.3f }, { fdecay, 0.15f }, { decay, 0.35f }, { sustain, 0.3f },
                               { release, 0.1f }, { m1, 0.4f,  }, { gain, -3.9f } });
        B ("Pluggnb Soft Bass", 3, { { engine, VA }, { wave, 0.25f }, { cutoff, 600 }, { decay, 0.6f }, { sustain, 0.5f }, { release, 0.2f }, { glide, 0.05f }, { m1, 0.5f,  }, { gain, 3.6f } });
        B ("Blown Rage Bass", 5, { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.25f }, { cutoff, 2500 }, { sustain, 1 }, { drive, 0.5f },
                               { driveType, 3 }, { m1, 0.6f }, { m3, 0.5f }, { gain, -10.2f } });
        B ("Acid Slide", 1, { { engine, VA }, { wave, 0.0f }, { cutoff, 500 }, { reso, 0.75f }, { fenv, 0.6f }, { fdecay, 0.2f }, { sustain, 0.7f },
                               { glide, 0.12f }, { drive, 0.2f }, { m1, 0.2f,  }, { gain, -5.7f } });
        B ("FM Punch Bass", 0, { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.5f }, { fdecay, 0.15f }, { decay, 0.5f }, { sustain, 0.5f }, { release, 0.1f },
                               { m1, 0.4f }, { m6, 0.4f,  }, { gain, -0.6f } });
        B ("Formant Growl", 4, { { engine, VX }, { wave, 0.3f }, { unison, 3 }, { detune, 0.2f }, { sustain, 1 }, { lfoSync, 1 }, { lfoDiv, 4 },
                               { m2, 0.5f }, { m3, 0.4f }, { m1, 0.5f,  }, { gain, -1.1f } });
        B ("Memphis Dirt Bass", 0, { { engine, SB }, { wave, 0.8f }, { decay, 1.5f }, { sustain, 0.3f }, { crush, 0.3f }, { drive, 0.4f },
                               { driveType, 1 }, { m3, 0.3f }, { m1, 0.2f,  }, { gain, -10.9f } });

        // ---------------- EXOTIC ----------------
        P ("Koto Dark", tOrchestra, 2, false, "", { { engine, PL }, { wave, 0.75f }, { decay, 1.5f }, { sustain, 0 }, { release, 0.4f }, { bend, 0.15f },
            { revMix, 0.3f,  }, { gain, -0.5f } });
        P ("Sitar Night", tOrchestra, 3, false, "", { { engine, PL }, { wave, 0.9f }, { decay, 2.5f }, { sustain, 0 }, { body, 6 }, { bodyMix, 0.35f },
            { revMix, 0.3f }, { bend, 0.1f,  }, { gain, 0.5f } });
        P ("Kalimba Minor", tOrchestra, 3, false, "Mallets", { { engine, FM }, { fmRatio, 5.4f }, { fmAmt, 0.3f }, { fdecay, 0.15f }, { decay, 1.0f },
            { sustain, 0 }, { release, 0.6f }, { body, 1 }, { bodyMix, 0.3f }, { revMix, 0.25f }, { gain, -3.8f } });
        P ("Marimba Heat", tOrchestra, 1, false, "Mallets", { { engine, FM }, { fmRatio, 4.0f }, { fmAmt, 0.35f }, { fdecay, 0.08f }, { decay, 0.6f },
            { sustain, 0 }, { release, 0.4f }, { revMix, 0.2f }, { gain, -5.2f } });
        P ("Harp Spell", tOrchestra, 0, false, "", { { engine, PL }, { wave, 0.4f }, { decay, 3.0f }, { sustain, 0 }, { release, 1.0f }, { revMix, 0.45f,  }, { gain, 0.4f } });

        // ---------------- EXPERIMENTAL ----------------
        P ("Melting Piano", tExperimental, -1, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.35f }, { decay, 2.0f }, { sustain, 0.1f },
            { release, 1.0f }, { bend, 0.7f }, { wow, 0.5f }, { revMix, 0.3f,  }, { gain, -5.3f } });
        P ("Reverse Keys", tExperimental, -1, false, "", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.3f }, { attack, 0.8f }, { sustain, 0.6f },
            { release, 0.05f }, { ghost, 0.6f }, { revMix, 0.4f,  }, { gain, -9.1f } });
        P ("Frozen Void", tExperimental, -1, false, "", { { engine, VX }, { wave, 0.9f }, { unison, 7 }, { detune, 0.6f }, { attack, 1.5f },
            { sustain, 1 }, { release, 4.0f }, { revMix, 0.8f }, { revSize, 1.0f }, { ghost, 0.5f,  }, { gain, -8.9f } });
        P ("Radio Ghost", tExperimental, -1, false, "", { { engine, VA }, { wave, 0.5f }, { cutoff, 2500 }, { reso, 0.5f }, { sustain, 0.8f },
            { crush, 0.5f }, { wow, 0.6f }, { ghost, 0.4f }, { circuit, 0.2f }, { revMix, 0.3f,  }, { gain, -12.3f } });
        P ("Rubber Pluck", tExperimental, -1, false, "", { { engine, PL }, { wave, 0.3f }, { decay, 0.8f }, { sustain, 0 }, { bend, 0.55f },
            { body, 5 }, { bodyMix, 0.4f,  }, { gain, 3.8f } });
        P ("Mystery Scale Plucks", tExperimental, -1, false, "", { { engine, FM }, { fmRatio, 7.13f }, { fmAmt, 0.4f }, { decay, 0.7f }, { sustain, 0 },
            { circuit, 0.25f }, { delayMix, 0.35f }, { revMix, 0.3f,  }, { gain, -5.1f } });

        // ---------------- EXCLUSIVE (built on sections 5.2 - 5.6) ----------------
        P ("Kalimba Supersaw", tLeads, 5, true, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { decay, 0.6f }, { sustain, 0.2f },
            { body, 1 }, { bodyMix, 0.65f }, { revMix, 0.3f }, { gain, -0.3f } });
        P ("Haunted Bell Ghost", tBells, 4, true, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.45f }, { decay, 2.5f }, { sustain, 0 },
            { release, 2.0f }, { ghost, 0.75f }, { revMix, 0.35f,  }, { gain, -6.5f } });
        P ("Circuit Bent Flute", tFlutes, 5, true, "", { { engine, FL }, { wave, 0.4f }, { sustain, 0.9f }, { lfoPitch, 0.1f }, { circuit, 0.6f },
            { crush, 0.15f }, { revMix, 0.3f,  }, { gain, -12.4f } });
        P ("Bent Choir", tChoir, 4, true, "", { { engine, VX }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.1f }, { sustain, 0.9f },
            { release, 1.2f }, { bend, 0.8f }, { revMix, 0.45f,  }, { gain, -0.5f } });
        P ("2012 > 2026 Morph", tPads, 5, true, "", { { engine, VA }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { cutoff, 4000 }, { attack, 0.2f },
            { sustain, 0.9f }, { release, 1.0f }, { morphX, 0.85f }, { morphY, 0.15f }, { revMix, 0.3f }, { gain, -11.4f } });
        P ("Glass Pipe Choir", tChoir, 5, true, "", { { engine, VX }, { wave, 0.6f }, { unison, 5 }, { detune, 0.25f }, { attack, 0.3f }, { sustain, 1 },
            { release, 1.5f }, { body, 4 }, { bodyMix, 0.5f }, { ghost, 0.3f }, { revMix, 0.4f,  }, { gain, -4.0f } });
        P ("Metal Pipe Bass", tBass, 5, true, "", { { engine, VA }, { wave, 0.3f }, { octave, -1 }, { cutoff, 1500 }, { sustain, 1 }, { body, 4 },
            { bodyMix, 0.4f }, { m1, 0.5f }, { m5, 0.5f }, { revMix, 0 }, { m2, 0 }, { m6, 0,  }, { gain, 3.2f } });
        P ("Broken Tape Keys", tKeys, 5, true, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 2.0f }, { sustain, 0.2f },
            { bend, 0.9f }, { circuit, 0.2f }, { wow, 0.4f }, { revMix, 0.3f,  }, { gain, -5.6f } });

        // ================= new engines: wavetable, orchestral, modal, layers A+B =================
        P ("Modal Bell Tower", tBells, 0, false, "", { { engine, MD }, { wave, 0.85f }, { fmAmt, 0.5f }, { decay, 3.0f }, { sustain, 0 }, { release, 2.0f },
            { revMix, 0.35f }, { revSize, 0.9f } });
        P ("Glass Wavetable Bells", tBells, 3, false, "", { { engine, WT }, { wave, 0.71f }, { octave, 1 }, { decay, 1.6f }, { sustain, 0 }, { release, 1.2f },
            { cutoff, 9000 }, { fenv, 0.3f }, { delayMix, 0.25f }, { revMix, 0.3f } });
        P ("Tine Keys 2019", tKeys, 3, false, "", { { engine, FM }, { fmAlgo, 5 }, { fmRatio, 1.0f }, { fmRatio2, 14.0f }, { fmAmt, 0.35f },
            { fdecay, 0.5f }, { decay, 2.0f }, { sustain, 0.15f }, { release, 0.6f }, { chorus, 0.35f }, { revMix, 0.2f } });
        { Vals v2 { { engine, FM }, { fmAlgo, 5 }, { fmRatio, 1.0f }, { fmRatio2, 14.0f }, { fmAmt, 0.3f }, { decay, 2.2f }, { sustain, 0.2f },
                    { release, 0.8f }, { layerB, 1 }, { engineB, WT }, { waveB, 0.0f }, { octaveB, 1 }, { levelB, 0.35f }, { chorus, 0.3f }, { revMix, 0.3f } };
          P ("Layered EP Dream", tKeys, 4, false, "", v2); }
        P ("Organ Ladder Grit", tKeys, 2, false, "Organs", { { engine, OR }, { wave, 0.7f }, { filterType, 1 }, { cutoff, 2500 }, { reso, 0.3f },
            { sustain, 1 }, { release, 0.2f }, { drive, 0.25f }, { driveType, 1 }, { revMix, 0.2f } });
        P ("WT Digi Pluck", tPlucks, 4, false, "", { { engine, WT }, { wave, 1.0f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 1800 }, { fenv, 0.7f },
            { fdecay, 0.15f }, { decay, 0.3f }, { sustain, 0 }, { release, 0.2f }, { delayMix, 0.3f }, { delayMode, 0 } });
        P ("Harp Arp 2014", tPlucks, 1, false, "", { { engine, PL }, { wave, 0.45f }, { decay, 2.5f }, { sustain, 0 }, { release, 0.8f },
            { revMix, 0.4f }, { revType, 1 } });
        P ("Wavetable Whistle", tFlutes, 4, false, "", { { engine, WT }, { wave, 0.05f }, { octave, 1 }, { attack, 0.03f }, { sustain, 0.9f }, { release, 0.3f },
            { lfoPitch, 0.12f }, { mono, 1 }, { glide, 0.06f }, { revMix, 0.3f }, { delayMix, 0.2f } });
        P ("Orchestra Choir Hit 2011", tChoir, 0, false, "", { { engine, OC }, { wave, 1.0f }, { fmAmt, 0.0f }, { unison, 5 }, { detune, 0.3f },
            { attack, 0.005f }, { decay, 0.7f }, { sustain, 0.2f }, { release, 0.5f }, { revMix, 0.4f }, { revSize, 0.9f } });
        { Vals v2 { { engine, VX }, { wave, 0.35f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.3f }, { sustain, 1 }, { release, 1.5f },
                    { layerB, 1 }, { engineB, WT }, { waveB, 0.7f }, { unisonB, 3 }, { detuneB, 0.3f }, { levelB, 0.4f }, { revMix, 0.45f } };
          MM (v2, 0, srcLfo2, dstWaveB, 0.25f);
          P ("Synth Vox Layer", tChoir, 5, false, "", v2); }
        P ("Orchestral Strings 2012", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.2f },
            { sustain, 0.9f }, { release, 0.8f }, { lfoPitch, 0.05f }, { revMix, 0.4f }, { revSize, 0.9f } });
        { Vals v2 { { engine, WT }, { wave, 0.4f }, { unison, 5 }, { detune, 0.35f }, { attack, 0.6f }, { sustain, 1 }, { release, 2.0f },
                    { cutoff, 5000 }, { lfo2Rate, 0.15f }, { chorus, 0.3f }, { revMix, 0.5f }, { revType, 2 } };
          MM (v2, 0, srcLfo2, dstWaveA, 0.35f); MM (v2, 1, srcLfo1, dstPan, 0.3f);
          P ("Wavetable Motion Pad", tPads, 4, false, "", v2); }
        { Vals v2 { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.35f }, { attack, 0.5f }, { sustain, 1 }, { release, 2.0f }, { cutoff, 4000 },
                    { layerB, 1 }, { engineB, WT }, { waveB, 0.71f }, { octaveB, 1 }, { levelB, 0.45f }, { phaser, 0.35f }, { revMix, 0.5f } };
          P ("Layered Glass Pad", tPads, 3, false, "", v2); }
        P ("Orchestral Brass Stab 2010", tLeads, 0, false, "Brass", { { engine, OC }, { wave, 0.0f }, { unison, 3 }, { detune, 0.2f }, { attack, 0.01f },
            { decay, 0.5f }, { sustain, 0.5f }, { release, 0.3f }, { revMix, 0.35f }, { revSize, 0.85f } });
        P ("WT Rage Lead", tLeads, 4, false, "", { { engine, WT }, { wave, 0.285f }, { unison, 7 }, { detune, 0.45f }, { sustain, 0.9f }, { release, 0.3f },
            { drive, 0.35f }, { driveType, 1 }, { revMix, 0.25f } });
        { Vals v2 { { engine, WT }, { wave, 0.3f }, { warpMode, 1 }, { fmAmt, 0.2f }, { unison, 3 }, { detune, 0.2f }, { sustain, 0.9f },
                    { e3attack, 0.001f }, { e3decay, 0.4f }, { mono, 1 }, { glide, 0.05f }, { delayMix, 0.25f } };
          MM (v2, 0, srcEnv3, dstFmA, 0.6f);
          P ("Sync Lead 2025", tLeads, 5, false, "", v2); }
        P ("Stack FM Lead", tLeads, 3, false, "", { { engine, FM }, { fmAlgo, 1 }, { fmRatio, 2.0f }, { fmRatio2, 3.0f }, { fmAmt, 0.35f },
            { sustain, 0.8f }, { release, 0.3f }, { lfoPitch, 0.06f }, { delayMix, 0.2f } });
        P ("Tape Pad Loop", tPads, 2, false, "", { { engine, VA }, { wave, 0.35f }, { unison, 3 }, { cutoff, 2500 }, { attack, 0.3f }, { sustain, 0.9f },
            { release, 1.2f }, { delayMode, 2 }, { delayMix, 0.35f }, { delayFb, 0.55f }, { wow, 0.35f } });
        P ("Kalimba Modal", tOrchestra, 3, false, "Mallets", { { engine, MD }, { wave, 0.1f }, { fmAmt, 0.4f }, { decay, 1.2f }, { sustain, 0 },
            { release, 0.8f }, { revMix, 0.3f } });
        P ("Marimba Modal 2016", tOrchestra, 2, false, "Mallets", { { engine, MD }, { wave, 0.5f }, { fmAmt, 0.3f }, { decay, 0.8f }, { sustain, 0 },
            { release, 0.5f }, { revMix, 0.2f } });
        P ("Frozen Cloud", tExperimental, -1, false, "", { { engine, VX }, { wave, 0.6f }, { unison, 7 }, { detune, 0.5f }, { attack, 0.8f }, { sustain, 1 },
            { release, 3.0f }, { revType, 2 }, { revMix, 0.7f }, { freeze, 0 }, { phaser, 0.3f } });
        P ("Phaser Void", tExperimental, -1, false, "", { { engine, WT }, { wave, 0.9f }, { unison, 5 }, { detune, 0.4f }, { attack, 0.5f }, { sustain, 1 },
            { release, 2.0f }, { phaser, 0.8f }, { flanger, 0.3f }, { revMix, 0.5f } });
        P ("Reverse Tape Keys", tExperimental, -1, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.5f }, { sustain, 0.3f },
            { reverse, 0.6f }, { wow, 0.3f }, { revMix, 0.3f } });

        // ---------------- more bass ----------------
        B ("Sub + Click", 3, { { engine, SB }, { wave, 0.1f }, { sustain, 1 }, { release, 0.2f }, { m6, 0.7f } });
        B ("Rage Saw Bass", 4, { { engine, VA }, { wave, 0.0f }, { unison, 2 }, { detune, 0.2f }, { cutoff, 3000 }, { sustain, 1 }, { drive, 0.45f },
                              { driveType, 2 }, { m1, 0.5f }, { m3, 0.35f } });
        B ("Jerk Bounce Bass", 4, { { engine, VA }, { wave, 0.5f }, { cutoff, 1500 }, { fenv, 0.5f }, { fdecay, 0.08f }, { decay, 0.2f }, { sustain, 0.1f },
                              { release, 0.08f }, { m1, 0.4f }, { m6, 0.5f } });
        B ("Club Stab Bass", 5, { { engine, VA }, { wave, 0.1f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 1800 }, { fenv, 0.5f }, { fdecay, 0.12f },
                              { decay, 0.25f }, { sustain, 0.2f }, { m1, 0.4f }, { m3, 0.2f } });
        B ("Digital Bass 2012", 0, { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.45f }, { fdecay, 0.25f }, { sustain, 0.7f }, { crush, 0.15f }, { m1, 0.3f } });
        B ("FM Growl", 4, { { engine, FM }, { fmAlgo, 3 }, { fmRatio, 1.0f }, { fmRatio2, 2.0f }, { fmAmt, 0.5f }, { sustain, 1 }, { lfoSync, 1 }, { lfoDiv, 4 },
                              { wobTarget, 0 }, { m2, 0.4f }, { m3, 0.35f }, { m1, 0.4f } });
        { Vals v2 { { engine, WT }, { wave, 0.5f }, { warpMode, 4 }, { fmAmt, 0.4f }, { sustain, 1 }, { lfo2Sync, 1 }, { lfo2Div, 4 }, { m1, 0.4f }, { m3, 0.3f } };
          MM (v2, 0, srcLfo2, dstFmA, 0.5f);
          B ("WT Growl Bass", 5, v2); }
        B ("Ladder Acid", 1, { { engine, VA }, { wave, 0.0f }, { filterType, 1 }, { cutoff, 400 }, { reso, 0.8f }, { fenv, 0.6f }, { fdecay, 0.2f },
                              { sustain, 0.7f }, { glide, 0.1f }, { m1, 0.2f } });
        B ("Dark Acid", 2, { { engine, VA }, { wave, 0.5f }, { filterType, 2 }, { cutoff, 350 }, { reso, 0.7f }, { fenv, 0.5f }, { fdecay, 0.3f },
                              { sustain, 0.6f }, { glide, 0.12f }, { m1, 0.3f } });
        B ("Pluck Bass", 3, { { engine, PL }, { wave, 0.4f }, { decay, 1.0f }, { sustain, 0 }, { release, 0.2f }, { m1, 0.4f } });
        B ("Detroit Pluck Bass", 3, { { engine, VA }, { wave, 0.3f }, { cutoff, 700 }, { fenv, 0.6f }, { fdecay, 0.1f }, { decay, 0.3f }, { sustain, 0 },
                              { release, 0.15f }, { m1, 0.5f } });
        { Vals v2 { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.3f }, { cutoff, 800 }, { sustain, 1 }, { lfo2Rate, 0.3f }, { m1, 0.4f } };
          MM (v2, 0, srcLfo2, dstDetune, 0.4f); MM (v2, 1, srcLfo2, dstCutoff, 0.15f);
          B ("Moving Reese", 3, v2); }
        B ("Hyper Bass", 4, { { engine, WT }, { wave, 0.57f }, { unison, 3 }, { detune, 0.25f }, { cutoff, 4000 }, { sustain, 1 }, { drive, 0.4f },
                              { driveType, 2 }, { m1, 0.4f } });
        B ("Digicore Wobble", 4, { { engine, WT }, { wave, 0.43f }, { sustain, 1 }, { lfoSync, 1 }, { lfoDiv, 4 }, { wobTarget, 2 }, { m2, 0.7f }, { m1, 0.5f } });
        B ("Phonk Crush Bass", 1, { { engine, SB }, { wave, 0.9f }, { decay, 1.2f }, { sustain, 0.4f }, { crush, 0.4f }, { drive, 0.5f }, { driveType, 3 },
                              { m1, 0.2f }, { m3, 0.3f } });
        B ("Slow Growl Wobble", 3, { { engine, VX }, { wave, 0.2f }, { unison, 3 }, { detune, 0.2f }, { sustain, 1 }, { lfoSync, 1 }, { lfoDiv, 2 },
                              { m2, 0.6f }, { m3, 0.3f }, { m1, 0.5f } });
        B ("Bent Sub", 5, { { engine, SB }, { wave, 0.3f }, { sustain, 1 }, { tape, 1 }, { bend, 0.4f }, { m1, 0.2f } }, true);
        B ("Broken Bass", 5, { { engine, VA }, { wave, 0.3f }, { cutoff, 1500 }, { sustain, 1 }, { circuit, 0.5f }, { m1, 0.5f } }, true);

        // ---------------- more EXCLUSIVE ----------------
        P ("Ghost Orchestra", tPads, 0, true, "", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.3f }, { sustain, 0.9f },
            { release, 1.5f }, { ghost, 0.7f }, { ghostOct, 1 }, { revMix, 0.4f } });
        P ("Choir In A Bell", tChoir, 3, true, "", { { engine, VX }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.2f }, { sustain, 0.9f },
            { release, 1.5f }, { body, 2 }, { bodyMix, 0.6f }, { revMix, 0.35f } });
        P ("Wood Box Supersaw", tLeads, 4, true, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.45f }, { sustain, 0.8f },
            { body, 5 }, { bodyMix, 0.55f }, { revMix, 0.2f } });
        P ("Glass Flute Swap", tFlutes, 4, true, "", { { engine, FL }, { wave, 0.4f }, { sustain, 0.9f }, { lfoPitch, 0.1f }, { body, 3 }, { bodyMix, 0.5f },
            { revMix, 0.35f } });
        P ("Flute In A Metal Pipe", tFlutes, 3, true, "", { { engine, FL }, { wave, 0.5f }, { sustain, 0.9f }, { body, 4 }, { bodyMix, 0.6f }, { revMix, 0.3f } });
        P ("Circuit Organ", tKeys, 4, true, "Organs", { { engine, OR }, { wave, 0.6f }, { sustain, 1 }, { circuit, 0.5f }, { circRate, 2 }, { crush, 0.2f } });
        P ("Octave Jump Lead", tLeads, 5, true, "", { { engine, WT }, { wave, 0.3f }, { unison, 5 }, { detune, 0.3f }, { sustain, 0.9f }, { release, 0.4f },
            { bend, 0.7f }, { bendMode, 3 }, { bendSemis, -12 }, { drive, 0.3f }, { gain, -3 } });
        P ("Rise Bend Bells", tBells, 5, true, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.4f }, { decay, 2.0f }, { sustain, 0.1f }, { release, 1.0f },
            { bend, 0.5f }, { bendMode, 1 }, { bendSemis, 12 }, { revMix, 0.35f } });
        P ("Tape Choir", tChoir, 2, true, "", { { engine, VX }, { wave, 0.6f }, { unison, 5 }, { attack, 0.2f }, { sustain, 0.9f }, { release, 1.2f },
            { tape, 1 }, { bend, 0.3f }, { wow, 0.3f }, { revMix, 0.4f } });
        P ("Reverse Ghost Pad", tPads, 5, true, "", { { engine, WT }, { wave, 0.6f }, { unison, 5 }, { detune, 0.35f }, { attack, 0.6f }, { sustain, 1 },
            { release, 2.5f }, { ghost, 0.6f }, { reverse, 0.4f }, { revMix, 0.45f } });
        P ("Bent Kalimba", tOrchestra, 5, true, "Mallets", { { engine, MD }, { wave, 0.1f }, { fmAmt, 0.5f }, { decay, 1.2f }, { sustain, 0 },
            { bend, 0.5f }, { bendMode, 4 }, { tape, 1 }, { revMix, 0.3f } });
        P ("Haunted Music Box", tBells, 2, true, "", { { engine, MD }, { wave, 0.85f }, { octave, 1 }, { decay, 1.5f }, { sustain, 0 }, { ghost, 0.6f },
            { tape, 1 }, { wow, 0.3f }, { revMix, 0.4f } });
        P ("Aggressive Era Pad", tPads, 4, true, "", { { engine, VA }, { wave, 0.2f }, { unison, 7 }, { detune, 0.4f }, { attack, 0.2f }, { sustain, 0.9f },
            { release, 1.2f }, { morphX, 1.0f }, { morphY, 0.0f }, { gain, -4 } });
        P ("Classic Era Keys", tKeys, 0, true, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.8f }, { sustain, 0.1f },
            { morphX, 0.0f }, { morphY, 1.0f } });


        // ================= sub-categories: PIANO, ORGANS, STRINGS, BRASS, GUITARS, MALLETS, ARPS =================
        auto piano = [&] (const char* name, int era, Vals extra)
        {
            // FM hammer/tine body + string layer: bright attack that darkens, no sustain
            Vals v2 { { engine, FM }, { fmAlgo, 0 }, { fmRatio, 1.0f }, { fmAmt, 0.42f }, { wave, 0.22f }, { fdecay, 0.9f },
                      { attack, 0.001f }, { decay, 3.0f }, { sustain, 0.0f }, { release, 0.45f }, { velSens, 0.75f },
                      { layerB, 1 }, { engineB, PL }, { waveB, 0.72f }, { levelB, 0.45f }, { fineB, 4.0f },
                      { cutoff, 7000 }, { keyTrack, 0.6f }, { revType, 1 }, { revMix, 0.22f }, { chorus, 0.08f } };
            v2.insert (v2.end(), extra.begin(), extra.end());
            P (name, tKeys, era, false, "Piano", v2);
        };
        piano ("Bouncy Atlanta Piano", 0, { { decay, 1.4f }, { cutoff, 9000 } });
        piano ("Detuned Lo-Fi Piano", 1, { { fineB, 14.0f }, { wow, 0.35f }, { crush, 0.08f }, { cutoff, 4500 } });
        piano ("Trap Grand Piano", 2, {});
        piano ("Sad Piano Loop", 2, { { cutoff, 3800 }, { wow, 0.22f }, { revMix, 0.32f }, { revType, 0 } });
        piano ("Dark Trap Piano", 3, { { cutoff, 2600 }, { revMix, 0.3f }, { revType, 0 }, { m1, 0.42f } });
        piano ("Drill Slide Piano 2021", 3, { { mono, 1 }, { glide, 0.1f }, { legato, 1 } });
        piano ("Hyper Piano 2024", 4, { { drive, 0.28f }, { driveType, 1 }, { delayMix, 0.22f }, { cutoff, 9500 } });
        piano ("Ghost Piano 2026", 5, { { ghost, 0.45f }, { reverse, 0.15f }, { revMix, 0.35f } });

        P ("Dark Church Organ 2011", tKeys, 0, false, "Organs", { { engine, OR }, { wave, 0.4f }, { attack, 0.01f }, { sustain, 1 }, { release, 0.5f },
            { lfoPitch, 0.03f }, { revMix, 0.45f }, { revSize, 0.95f } });
        P ("Trap Organ Chords", tKeys, 2, false, "Organs", { { engine, OR }, { wave, 0.55f }, { sustain, 1 }, { release, 0.3f }, { chorus, 0.3f },
            { cutoff, 5000 }, { revMix, 0.25f } });
        P ("Rotary Organ 2016", tKeys, 2, false, "Organs", { { engine, OR }, { wave, 0.7f }, { sustain, 1 }, { release, 0.25f }, { chorus, 0.55f },
            { lfoAmp, 0.18f }, { lfoRate, 6.5f }, { revMix, 0.2f } });

        P ("Epic Trap Strings", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 7 }, { detune, 0.3f }, { attack, 0.15f },
            { sustain, 0.9f }, { release, 0.9f }, { lfoPitch, 0.04f }, { revMix, 0.4f }, { revSize, 0.9f } });
        P ("Staccato Strings 2012", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.25f }, { attack, 0.003f },
            { decay, 0.25f }, { sustain, 0 }, { release, 0.2f }, { revMix, 0.35f } });
        P ("Tremolo Strings", tPads, 1, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.1f },
            { sustain, 0.9f }, { release, 0.8f }, { lfoAmp, 0.7f }, { lfoSync, 1 }, { lfoDiv, 4 }, { revMix, 0.35f } });
        P ("Drill Violin Lead", tLeads, 3, false, "Strings", { { engine, VA }, { wave, 0.05f }, { unison, 2 }, { detune, 0.1f }, { cutoff, 4500 },
            { attack, 0.05f }, { sustain, 0.9f }, { release, 0.3f }, { mono, 1 }, { glide, 0.12f }, { lfoPitch, 0.12f }, { revMix, 0.3f } });
        P ("Dark Cello", tPads, 2, false, "Strings", { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.15f }, { octave, -1 }, { cutoff, 1500 },
            { attack, 0.1f }, { sustain, 0.9f }, { release, 0.6f }, { lfoPitch, 0.05f }, { revMix, 0.35f } });

        P ("Trap Horns 2011", tLeads, 0, false, "Brass", { { engine, OC }, { wave, 0.0f }, { unison, 5 }, { detune, 0.25f }, { attack, 0.01f },
            { decay, 0.6f }, { sustain, 0.6f }, { release, 0.3f }, { drive, 0.2f }, { driveType, 1 }, { revMix, 0.35f } });
        P ("Dark Brass Stab", tLeads, 1, false, "Brass", { { engine, OC }, { wave, 0.0f }, { unison, 3 }, { octave, -1 }, { decay, 0.4f },
            { sustain, 0.2f }, { release, 0.2f }, { revMix, 0.3f } });
        P ("Synth Brass 2019", tLeads, 3, false, "Brass", { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 800 },
            { fenv, 0.6f }, { fattack, 0.03f }, { fdecay, 0.35f }, { fsustain, 0.3f }, { sustain, 0.8f }, { release, 0.25f }, { revMix, 0.25f } });
        P ("Brass Swell Pad", tPads, 1, false, "Brass", { { engine, OC }, { wave, 0.1f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.45f },
            { sustain, 1 }, { release, 1.0f }, { revMix, 0.4f } });
        P ("Blown Horns 2025", tLeads, 5, false, "Brass", { { engine, OC }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { decay, 0.5f },
            { sustain, 0.5f }, { drive, 0.55f }, { driveType, 3 }, { revMix, 0.2f } });

        P ("Acoustic Trap Guitar", tPlucks, 3, false, "Guitars", { { engine, PL }, { wave, 0.55f }, { decay, 2.5f }, { sustain, 0 }, { release, 0.5f },
            { body, 6 }, { bodyMix, 0.25f }, { chorus, 0.2f }, { revMix, 0.25f } });
        P ("Nylon Guitar 2017", tPlucks, 2, false, "Guitars", { { engine, PL }, { wave, 0.35f }, { decay, 2.0f }, { sustain, 0 }, { release, 0.4f },
            { body, 5 }, { bodyMix, 0.3f }, { revMix, 0.25f } });
        P ("Clean Electric 2020", tPlucks, 3, false, "Guitars", { { engine, PL }, { wave, 0.75f }, { decay, 3.0f }, { sustain, 0 }, { release, 0.5f },
            { chorus, 0.4f }, { delayMix, 0.2f }, { revMix, 0.2f } });
        P ("Dark Guitar Loop", tPlucks, 2, false, "Guitars", { { engine, PL }, { wave, 0.5f }, { decay, 2.5f }, { sustain, 0 }, { cutoff, 3000 },
            { wow, 0.3f }, { crush, 0.1f }, { revMix, 0.3f } });
        P ("Distorted Guitar 2023", tPlucks, 4, false, "Guitars", { { engine, PL }, { wave, 0.8f }, { decay, 4.0f }, { sustain, 0 }, { release, 0.4f },
            { drive, 0.5f }, { driveType, 1 }, { revMix, 0.2f } });

        P ("Vibraphone Night", tOrchestra, 2, false, "Mallets", { { engine, MD }, { wave, 0.5f }, { fmAmt, 0.2f }, { decay, 2.0f }, { sustain, 0 },
            { release, 1.0f }, { lfoAmp, 0.3f }, { lfoRate, 5.0f }, { revMix, 0.3f } });
        P ("Glockenspiel Ice", tBells, 1, false, "Mallets", { { engine, MD }, { wave, 0.85f }, { octave, 1 }, { fmAmt, 0.6f }, { decay, 1.5f },
            { sustain, 0 }, { release, 1.0f }, { revMix, 0.35f } });
        P ("Xylo Bounce", tOrchestra, 3, false, "Mallets", { { engine, MD }, { wave, 0.5f }, { fmAmt, 0.7f }, { decay, 0.35f }, { sustain, 0 },
            { release, 0.3f }, { revMix, 0.2f } });
        P ("Steel Drum 2019", tOrchestra, 3, false, "Mallets", { { engine, FM }, { fmRatio, 2.3f }, { fmAmt, 0.35f }, { fdecay, 0.3f }, { decay, 1.0f },
            { sustain, 0 }, { revMix, 0.25f } });

        auto arpP = [&] (const char* name, int tile, int era, Vals v2, int rate, int mode, int octs, float gate)
        {
            v2.push_back ({ arp, 1 }); v2.push_back ({ arpRate, (float) rate }); v2.push_back ({ arpMode, (float) mode });
            v2.push_back ({ arpOct, (float) octs }); v2.push_back ({ arpGate, gate });
            P (name, tile, era, false, "Arps", v2);
        };
        arpP ("Arp Bells 1-16", tBells, 1, { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.4f }, { decay, 0.8f }, { sustain, 0 }, { release, 0.4f },
              { delayMix, 0.25f }, { revMix, 0.3f } }, 1, 0, 2, 0.6f);
        arpP ("Rage Arp Saw", tLeads, 4, { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { cutoff, 3500 }, { fenv, 0.5f },
              { fdecay, 0.15f }, { decay, 0.25f }, { sustain, 0.1f }, { release, 0.15f }, { drive, 0.3f }, { delayMix, 0.2f } }, 1, 2, 2, 0.5f);
        arpP ("Plugg Arp", tPlucks, 3, { { engine, VA }, { wave, 0.5f }, { cutoff, 2500 }, { fenv, 0.4f }, { fdecay, 0.15f }, { decay, 0.3f },
              { sustain, 0.1f }, { revMix, 0.3f }, { delayMix, 0.2f } }, 0, 3, 1, 0.7f);
        arpP ("Trance Arp 2025", tLeads, 5, { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.35f }, { cutoff, 2800 }, { fenv, 0.5f },
              { fdecay, 0.12f }, { decay, 0.2f }, { sustain, 0.1f }, { delayMix, 0.3f }, { revMix, 0.25f } }, 1, 0, 2, 0.45f);
        arpP ("Glassy Arp", tPlucks, 3, { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { wave, 0.3f }, { octave, 1 }, { decay, 0.6f },
              { sustain, 0.1f }, { chorus, 0.3f }, { delayMix, 0.25f } }, 2, 0, 1, 0.6f);
        arpP ("Dark Harp Arp", tPlucks, 2, { { engine, PL }, { wave, 0.4f }, { decay, 1.5f }, { sustain, 0 }, { revMix, 0.35f } }, 0, 0, 2, 0.8f);

        // ================= v0.4: 808 / TEXTURE / FX + era banks (TRAP 2010 -> FUTURE) =================
        auto B8 = [&] (const char* name, int era, Vals vals)
        {
            Vals base { { m1, 0.0f }, { m2, 0.0f }, { m3, 0.0f }, { m4, 0.0f }, { m5, 0.5f }, { m6, 0.0f },
                        { revMix, 0.0f }, { width, 0.4f }, { octave, -1 }, { engine, SB }, { sustain, 0 } };
            base.insert (base.end(), vals.begin(), vals.end());
            P (name, tBass, era, false, "808", base);
        };
        B8 ("808 Boom Long 2010", 0, { { wave, 0.3f }, { fmAmt, 0.1f }, { decay, 5.0f }, { release, 0.6f }, { m6, 0.25f } });
        B8 ("808 Tight Knock", 1, { { wave, 0.55f }, { fmAmt, 0.35f }, { decay, 0.7f }, { release, 0.15f }, { m3, 0.3f }, { m6, 0.8f }, { punch, 0.4f } });
        B8 ("808 Glide Legato", 2, { { wave, 0.45f }, { fmAmt, 0.15f }, { decay, 4.0f }, { sustain, 0.3f }, { release, 0.4f }, { glide, 0.25f }, { legato, 1 } });
        B8 ("808 Distorted Rage", 4, { { wave, 0.6f }, { fmAmt, 0.25f }, { decay, 2.5f }, { release, 0.3f }, { m3, 0.7f }, { driveType, 3 }, { m6, 0.5f } });
        B8 ("808 Drill Slide", 4, { { wave, 0.5f }, { fmAmt, 0.2f }, { decay, 3.5f }, { sustain, 0.25f }, { release, 0.35f }, { glide, 0.35f }, { m3, 0.35f } });
        B8 ("808 Folded Grit", 5, { { wave, 0.7f }, { fmAmt, 0.3f }, { decay, 2.0f }, { release, 0.3f }, { m3, 0.5f }, { driveType, 4 } });
        B8 ("808 Tape Warm", 2, { { wave, 0.35f }, { fmAmt, 0.1f }, { decay, 3.0f }, { release, 0.4f }, { m3, 0.25f }, { driveType, 1 }, { drift, 0.3f } });
        B8 ("808 Clean Sine Sub", 0, { { wave, 0.0f }, { decay, 6.0f }, { release, 0.5f } });

        auto TX = [&] (const char* name, int era, Vals vals) { P (name, tExperimental, era, false, "Texture", vals); };
        TX ("Sub-Harmonic Drone 2026", 5, { { engine, WT }, { wave, 0.2f }, { octave, -1 }, { unison, 5 }, { detune, 0.3f }, { attack, 2.0f }, { sustain, 1 },
            { release, 4.0f }, { sub, 0.5f }, { cutoff, 1800 }, { crush, 0.25f }, { wow, 0.35f }, { revMix, 0.45f }, { revType, 2 }, { drift, 0.5f }, { alive, 3 } });
        TX ("Granular String Cloud 2025", 5, { { engine, OC }, { wave, 0.4f }, { unison, 5 }, { detune, 0.35f }, { attack, 1.5f }, { sustain, 1 }, { release, 4.0f },
            { reverse, 0.35f }, { chorus, 0.4f }, { revMix, 0.5f }, { revType, 2 }, { revSize, 0.9f }, { alive, 4 }, { drift, 0.3f }, { timeM, 0.7f } });
        TX ("Cassette Hiss Pad", 3, { { engine, VA }, { wave, 0.5f }, { unison, 3 }, { detune, 0.3f }, { cutoff, 2200 }, { attack, 1.0f }, { sustain, 1 },
            { release, 3.0f }, { crush, 0.4f }, { wow, 0.5f }, { tape, 1 }, { revMix, 0.35f }, { drift, 0.6f } });
        TX ("Frozen Choir Air", 4, { { engine, VX }, { wave, 0.7f }, { unison, 5 }, { detune, 0.4f }, { attack, 1.2f }, { sustain, 1 }, { release, 4.0f },
            { revMix, 0.5f }, { revType, 2 }, { timeM, 0.95f }, { alive, 2 } });
        TX ("Metal Resonance Bed", 5, { { engine, WT }, { wave, 0.6f }, { unison, 3 }, { detune, 0.2f }, { attack, 0.6f }, { sustain, 0.7f }, { release, 3.0f },
            { body, 4 }, { bodyMix, 0.6f }, { phaser, 0.4f }, { revMix, 0.4f } });
        TX ("Half-Time Tape Keys", 3, { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.5f }, { sustain, 0.4f }, { release, 1.0f },
            { halftime, 1.0f }, { wow, 0.4f }, { crush, 0.2f }, { revMix, 0.3f } });

        auto FXP = [&] (const char* name, int era, Vals vals) { P (name, tExperimental, era, false, "FX", vals); };
        { Vals r { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.5f }, { cutoff, 300 }, { reso, 0.4f }, { attack, 3.0f }, { sustain, 1 },
                   { release, 2.0f }, { e3attack, 4.0f }, { e3sustain, 1 }, { revMix, 0.45f }, { delayMix, 0.25f } };
          MM (r, 0, srcEnv3, dstCutoff, 0.9f); MM (r, 1, srcEnv3, dstPitch, 0.35f); FXP ("Noise Riser 8 Bars", 4, r); }
        { Vals r { { engine, SB }, { octave, -2 }, { wave, 0.6f }, { fmAmt, 0.4f }, { decay, 2.5f }, { sustain, 0 }, { release, 1.5f }, { bend, 0.8f },
                   { bendMode, 0 }, { bendSemis, -24 }, { drive, 0.4f }, { revMix, 0.4f }, { revSize, 0.9f }, { punch, 0.7f } };
          FXP ("Cinematic Impact Hit", 5, r); }
        { Vals r { { engine, VA }, { wave, 0.2f }, { unison, 3 }, { detune, 0.3f }, { decay, 2.0f }, { sustain, 0 }, { release, 1.0f },
                   { bend, 1.0f }, { bendMode, 0 }, { bendSemis, -24 }, { revMix, 0.35f }, { delayMix, 0.3f } };
          FXP ("Tape Stop Downer", 3, r); }
        { Vals r { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.35f }, { attack, 2.0f }, { sustain, 1 }, { release, 0.2f }, { reverse, 0.6f },
                   { revMix, 0.55f }, { revType, 2 } };
          FXP ("Reverse Swell", 2, r); }
        { Vals r { { engine, MD }, { wave, 0.9f }, { fmAmt, 0.8f }, { decay, 0.6f }, { sustain, 0 }, { circuit, 0.6f }, { crush, 0.4f },
                   { delayMix, 0.35f }, { delayMode, 0 } };
          FXP ("Glitch Scatter FX", 5, r); }
        { Vals r { { engine, VA }, { wave, 0.5f }, { octave, 1 }, { cutoff, 4000 }, { reso, 0.6f }, { attack, 0.01f }, { decay, 0.8f }, { sustain, 0 },
                   { bend, 0.9f }, { bendMode, 1 }, { bendSemis, 12 }, { lfoPitch, 0.4f }, { lfoRate, 8.0f }, { delayMix, 0.4f } };
          FXP ("Laser Zap Up", 4, r); }

        // era banks from the TRAP-CORE taxonomy (neutral names)
        P ("Tutti Orchestral Stab 2011", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.5f }, { layerB, 1 }, { engineB, SB }, { octaveB, -1 },
            { levelB, 0.5f }, { attack, 0.002f }, { decay, 0.5f }, { sustain, 0 }, { release, 0.25f }, { punch, 0.6f }, { revMix, 0.15f } });
        P ("Harpsichord Stab 2012", tKeys, 1, false, "", { { engine, PL }, { wave, 0.8f }, { fmAmt, 0.2f }, { decay, 0.6f }, { sustain, 0 },
            { release, 0.2f }, { filterType, 6 }, { cutoff, 3000 }, { reso, 0.4f }, { punch, 0.35f } });
        P ("Gothic Dark Choir 2010", tChoir, 0, false, "", { { engine, VX }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.4f },
            { sustain, 1 }, { release, 1.5f }, { lfoPitch, 0.05f }, { lfoRate, 5.0f }, { crush, 0.15f }, { revMix, 0.35f } });
        P ("Detuned Music Box 2017", tBells, 2, false, "Mallets", { { engine, MD }, { wave, 0.9f }, { octave, 1 }, { decay, 1.8f }, { sustain, 0 },
            { release, 1.2f }, { wow, 0.45f }, { drift, 0.45f }, { alive, 3 }, { revMix, 0.4f } });
        P ("Ethnic Flute Glide 2016", tFlutes, 2, false, "", { { engine, FL }, { wave, 0.4f }, { attack, 0.08f }, { sustain, 0.9f }, { release, 1.0f },
            { glide, 0.2f }, { legato, 1 }, { mono, 1 }, { lfoPitch, 0.04f }, { revMix, 0.5f }, { revSize, 0.9f }, { alive, 2 } });
        P ("Reverse Rhodes 2018", tKeys, 3, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.25f }, { attack, 0.7f }, { sustain, 0.7f },
            { release, 0.3f }, { reverse, 0.5f }, { chorus, 0.3f }, { halftime, 0.4f }, { revMix, 0.35f } });
        P ("Cinematic Braam 2021", tLeads, 4, false, "Brass", { { engine, VA }, { wave, 0.0f }, { octave, -1 }, { unison, 5 }, { detune, 0.25f },
            { cutoff, 900 }, { fenv, 0.6f }, { fdecay, 1.2f }, { sustain, 1 }, { release, 1.5f }, { sub, 0.5f }, { drive, 0.5f }, { driveType, 3 }, { revMix, 0.35f } });
        P ("Distorted Flute 2022", tFlutes, 4, false, "", { { engine, FL }, { wave, 0.6f }, { sustain, 0.9f }, { release, 0.5f }, { drive, 0.55f },
            { driveType, 4 }, { crush, 0.3f }, { delayMix, 0.25f } });
        P ("Micro Chop Keys 2026", tKeys, 5, false, "", { { engine, VX }, { wave, 0.5f }, { decay, 0.25f }, { sustain, 0 }, { release, 0.1f },
            { circuit, 0.5f }, { circRate, 1 }, { halftime, 0.3f }, { alive, 4 } });
        P ("Staccato Pizz Strings 2013", tPlucks, 1, false, "Strings", { { engine, OC }, { wave, 0.3f }, { decay, 0.25f }, { sustain, 0 },
            { release, 0.08f }, { punch, 0.8f }, { width, 0.8f } });
        P ("Marcato Low Horns 2012", tLeads, 1, false, "Brass", { { engine, OC }, { wave, 0.8f }, { octave, -1 }, { attack, 0.01f }, { decay, 0.4f },
            { sustain, 0.3f }, { release, 0.2f }, { punch, 0.5f }, { drive, 0.2f } });

        // ================= v0.4.1: full era banks (TRAP-CORE taxonomy, original synthesized sounds) =================
        // Bank 1: 2010-2014 hard orchestral era
        P ("Timpani Hit Stab 2010", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.4f }, { layerB, 1 }, { engineB, MD }, { octaveB, -2 },
            { waveB, 0.2f }, { levelB, 0.8f }, { decay, 0.6f }, { sustain, 0 }, { release, 0.3f }, { punch, 0.7f }, { revMix, 0.2f } });
        P ("Hit Orchestra Low 2011", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.2f }, { octave, -1 }, { unison, 3 }, { detune, 0.15f },
            { layerB, 1 }, { engineB, OC }, { waveB, 0.6f }, { levelB, 0.7f }, { decay, 0.45f }, { sustain, 0 }, { release, 0.2f }, { punch, 0.6f }, { crush, 0.1f } });
        P ("Marcato Violins 2012", tPlucks, 1, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 3 }, { detune, 0.12f }, { decay, 0.35f },
            { sustain, 0.15f }, { release, 0.12f }, { punch, 0.5f }, { width, 0.75f }, { revMix, 0.12f } });
        P ("Pizzicato Arp Strings 2013", tPlucks, 1, false, "Strings", { { engine, PL }, { wave, 0.45f }, { decay, 0.3f }, { sustain, 0 }, { release, 0.08f },
            { body, 6 }, { bodyMix, 0.45f }, { punch, 0.6f }, { revMix, 0.1f } });
        P ("Low Trombones 2011", tLeads, 0, false, "Brass", { { engine, OC }, { wave, 0.1f }, { octave, -1 }, { unison, 3 }, { detune, 0.1f }, { attack, 0.02f },
            { decay, 0.6f }, { sustain, 0.5f }, { release, 0.25f }, { drive, 0.2f }, { revMix, 0.15f } });
        P ("Octave Horn Stab 2013", tLeads, 1, false, "Brass", { { engine, OC }, { wave, 0.15f }, { layerB, 1 }, { engineB, OC }, { waveB, 0.15f }, { octaveB, -1 },
            { levelB, 0.6f }, { decay, 0.3f }, { sustain, 0.1f }, { release, 0.15f }, { punch, 0.55f } });
        P ("Full Brass Section 2014", tLeads, 1, false, "Brass", { { engine, OC }, { wave, 0.25f }, { unison, 5 }, { detune, 0.2f }, { attack, 0.03f },
            { sustain, 0.8f }, { release, 0.3f }, { chorus, 0.2f }, { revMix, 0.25f } });
        P ("Aah Choir Vibrato 2010", tChoir, 0, false, "", { { engine, VX }, { wave, 0.0f }, { unison, 5 }, { detune, 0.25f }, { attack, 0.3f }, { sustain, 1 },
            { release, 1.2f }, { lfoPitch, 0.07f }, { lfoRate, 5.5f }, { crush, 0.12f }, { revMix, 0.35f } });
        P ("Ooh Choir 16-Bit 2012", tChoir, 1, false, "", { { engine, VX }, { wave, 0.95f }, { unison, 5 }, { detune, 0.2f }, { attack, 0.35f }, { sustain, 1 },
            { release, 1.3f }, { lfoPitch, 0.06f }, { lfoRate, 5.0f }, { crush, 0.2f }, { revMix, 0.3f } });
        P ("Dry Harpsichord Arp 2013", tKeys, 1, false, "", { { engine, PL }, { wave, 0.9f }, { decay, 0.8f }, { sustain, 0 }, { release, 0.15f },
            { layerB, 1 }, { engineB, PL }, { waveB, 0.9f }, { octaveB, 1 }, { levelB, 0.35f }, { filterType, 6 }, { cutoff, 2500 }, { reso, 0.3f } });
        P ("Bell Stab Cutter 2011", tBells, 0, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.55f }, { fdecay, 0.25f }, { decay, 0.4f },
            { sustain, 0 }, { release, 0.2f }, { punch, 0.5f }, { eqHigh, 3 } });
        // Bank 2: 2015-2019 melodic era
        P ("Toy Piano Wobble 2016", tKeys, 2, false, "Piano", { { engine, MD }, { wave, 0.7f }, { octave, 1 }, { decay, 0.9f }, { sustain, 0 }, { release, 0.5f },
            { wow, 0.5f }, { drift, 0.4f }, { alive, 3 }, { revMix, 0.3f } });
        P ("Celeste Detuned 2017", tBells, 2, false, "Mallets", { { engine, MD }, { wave, 0.8f }, { octave, 1 }, { layerB, 1 }, { engineB, MD }, { waveB, 0.8f },
            { octaveB, 1 }, { fineB, 18 }, { levelB, 0.6f }, { decay, 1.4f }, { sustain, 0 }, { release, 1.0f }, { wow, 0.3f }, { revMix, 0.4f } });
        P ("Antique Music Box 2015", tBells, 2, false, "Mallets", { { engine, MD }, { wave, 0.95f }, { octave, 2 }, { decay, 1.2f }, { sustain, 0 }, { release, 1.0f },
            { crush, 0.2f }, { wow, 0.45f }, { drift, 0.35f }, { revMix, 0.45f }, { revSize, 0.85f } });
        P ("Bamboo Flute Night 2016", tFlutes, 2, false, "", { { engine, FL }, { wave, 0.3f }, { attack, 0.1f }, { sustain, 0.85f }, { release, 1.2f },
            { lfoPitch, 0.05f }, { lfoRate, 4.5f }, { revMix, 0.55f }, { revType, 2 }, { alive, 2 } });
        P ("Breathy Pan Glide 2018", tFlutes, 3, false, "", { { engine, FL }, { wave, 0.6f }, { mono, 1 }, { legato, 1 }, { glide, 0.25f }, { attack, 0.06f },
            { sustain, 0.9f }, { release, 0.8f }, { delayMix, 0.3f }, { revMix, 0.45f }, { drift, 0.3f } });
        P ("Shrine Flute Echo 2019", tFlutes, 3, false, "", { { engine, FL }, { wave, 0.45f }, { octave, -1 }, { attack, 0.15f }, { sustain, 0.8f },
            { release, 1.5f }, { delayMix, 0.4f }, { delayMode, 2 }, { revMix, 0.5f }, { timeM, 0.6f } });
        P ("Half-Speed Rhodes 2017", tKeys, 2, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 2.0f }, { sustain, 0.3f },
            { release, 0.8f }, { halftime, 1.0f }, { chorus, 0.3f }, { wow, 0.3f }, { revMix, 0.3f } });
        P ("Reverse Keys Swell 2019", tKeys, 3, false, "Piano", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.25f }, { attack, 1.2f }, { sustain, 0.8f },
            { release, 0.15f }, { reverse, 0.6f }, { revMix, 0.4f } });
        P ("Clean Bell Pluck 2018", tBells, 3, false, "", { { engine, FM }, { fmRatio, 4.0f }, { fmAmt, 0.35f }, { fdecay, 0.6f }, { decay, 2.5f },
            { sustain, 0 }, { release, 2.0f }, { revMix, 0.35f }, { delayMix, 0.2f } });
        P ("Additive Glass Bell 2016", tBells, 2, false, "", { { engine, OR }, { wave, 0.8f }, { octave, 1 }, { decay, 2.0f }, { sustain, 0 },
            { release, 1.5f }, { body, 3 }, { bodyMix, 0.5f }, { revMix, 0.4f } });
        // Bank 3: 2020-2023 rage / drill / dark era
        P ("Hard Clip Supersaw 2021", tLeads, 4, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 8 }, { detune, 0.5f }, { cutoff, 6000 },
            { sustain, 1 }, { release, 0.2f }, { glide, 0.08f }, { drive, 0.6f }, { driveType, 2 }, { delayMix, 0.2f } });
        P ("Hyper Glide Lead 2022", tLeads, 4, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { mono, 1 }, { legato, 1 },
            { glide, 0.2f }, { sustain, 1 }, { release, 0.15f }, { drive, 0.4f }, { driveType, 3 }, { revMix, 0.2f } });
        P ("Reese Horn Braam 2020", tLeads, 4, false, "Brass", { { engine, OC }, { wave, 0.1f }, { octave, -1 }, { layerB, 1 }, { engineB, VA }, { waveB, 0.0f },
            { unisonB, 3 }, { detuneB, 0.35f }, { octaveB, -2 }, { levelB, 0.6f }, { attack, 0.05f }, { sustain, 1 }, { release, 1.2f },
            { drive, 0.45f }, { driveType, 1 }, { revMix, 0.35f } });
        P ("Drill Staccato Strings 2021", tPlucks, 4, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 3 }, { detune, 0.2f }, { decay, 0.25f },
            { sustain, 0 }, { release, 0.1f }, { drift, 0.4f }, { driveType, 1 }, { drive, 0.25f }, { punch, 0.5f } });
        P ("Tape Warped Violins 2022", tPads, 4, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.2f },
            { sustain, 1 }, { release, 1.0f }, { wow, 0.5f }, { drift, 0.6f }, { tape, 1 }, { revMix, 0.3f } });
        P ("Folded Bell Crush 2023", tBells, 4, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.4f }, { decay, 1.2f }, { sustain, 0 },
            { drive, 0.6f }, { driveType, 4 }, { crush, 0.35f }, { revMix, 0.2f } });
        P ("Bitcrushed Flute 2021", tFlutes, 4, false, "", { { engine, FL }, { wave, 0.5f }, { sustain, 0.85f }, { release, 0.4f }, { crush, 0.55f },
            { drive, 0.3f }, { delayMix, 0.25f } });
        P ("Dark Opera Pad 2022", tChoir, 4, false, "", { { engine, VX }, { wave, 0.3f }, { octave, -1 }, { unison, 6 }, { detune, 0.35f }, { attack, 0.6f },
            { sustain, 1 }, { release, 2.0f }, { drive, 0.3f }, { driveType, 1 }, { revMix, 0.45f }, { revType, 2 } });
        // Bank 4: 2024-2026 hybrid / granular era
        P ("Grain Choir Cloud 2025", tChoir, 5, false, "Texture", { { engine, VX }, { wave, 0.6f }, { unison, 7 }, { detune, 0.45f }, { attack, 1.0f },
            { sustain, 1 }, { release, 3.5f }, { reverse, 0.4f }, { alive, 4 }, { revMix, 0.5f }, { revType, 2 }, { timeM, 0.75f } });
        P ("Cello To Digital Morph 2026", tPads, 5, false, "Strings", { { engine, OC }, { wave, 0.5f }, { octave, -1 }, { layerB, 1 }, { engineB, WT },
            { waveB, 0.7f }, { warpModeB, 2 }, { levelB, 0.7f }, { attack, 0.3f }, { sustain, 1 }, { release, 1.5f }, { revMix, 0.3f },
            { mmSrc (0), (float) srcModWheel }, { mmDst (0), (float) dstLevelB }, { mmAmt (0), 0.8f } });
        P ("Hiss Sub Drone 2024", tExperimental, 5, false, "Texture", { { engine, SB }, { octave, -1 }, { wave, 0.3f }, { attack, 1.5f }, { sustain, 1 },
            { release, 3.0f }, { layerB, 1 }, { engineB, VA }, { waveB, 0.9f }, { octaveB, 2 }, { levelB, 0.25f }, { crush, 0.45f }, { wow, 0.4f }, { revMix, 0.4f } });
        P ("Formant Chop Piano 2025", tKeys, 5, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 0.3f }, { sustain, 0 },
            { layerB, 1 }, { engineB, VX }, { waveB, 0.4f }, { levelB, 0.5f }, { circuit, 0.45f }, { circRate, 1 }, { alive, 3 } });
        P ("Vocal Slice Keys 2026", tKeys, 5, false, "", { { engine, VX }, { wave, 0.8f }, { decay, 0.18f }, { sustain, 0 }, { release, 0.08f },
            { circuit, 0.35f }, { circRate, 2 }, { halftime, 0.25f }, { delayMix, 0.2f } });
        P ("Supertrap Glass Keys 2026", tKeys, 5, false, "", { { engine, WT }, { wave, 0.35f }, { warpMode, 1 }, { decay, 1.0f }, { sustain, 0.2f },
            { release, 0.8f }, { body, 3 }, { bodyMix, 0.4f }, { alive, 3 }, { drift, 0.25f }, { revMix, 0.35f } });
        P ("Industrial Pipe Keys 2025", tKeys, 5, false, "", { { engine, MD }, { wave, 0.6f }, { decay, 0.9f }, { sustain, 0 }, { body, 4 }, { bodyMix, 0.6f },
            { drive, 0.4f }, { driveType, 2 }, { punch, 0.4f }, { revMix, 0.2f } });
        P ("Frozen Strings Hybrid 2026", tPads, 5, false, "Texture", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.8f },
            { sustain, 1 }, { release, 4.0f }, { layerB, 1 }, { engineB, WT }, { waveB, 0.5f }, { levelB, 0.4f }, { timeM, 0.93f }, { alive, 3 } });

        // ================= v0.5: place the earlier presets into the new taxonomy =================
        const size_t numLegacy = v.size();
        for (size_t i = 0; i < numLegacy; ++i)
        {
            auto& pr = v[i];
            classify (pr, pr.cat);
            setV (pr.values, ID::era, (float) pr.era); setV (pr.values, eraHome, (float) pr.era);
            // v0.7: no years in names
            auto words = juce::StringArray::fromTokens (pr.name, " ", "");
            if (words.size() > 1 && words[words.size() - 1].length() == 4 && words[words.size() - 1].containsOnly ("0123456789")) words.remove (words.size() - 1);
            if (words.size() > 1 && words[0].length() == 4 && words[0].containsOnly ("0123456789")) words.remove (0);
            pr.name = words.joinIntoString (" ");
        }

        // ================= v0.5: TRAP 2010 -> FUTURE factory bank =================
        // Every recipe is a neutral sound; each era it is released in bakes that era's production policy into it
        // (2010 dry workstation, 2013 layered digital, 2016 atmospheric, 2019 lo-fi texture, 2022 rage, 2025 fast, FUTURE).
        static const char* yearTag[numEras] { "Classic", "Layered", "Atmos", "Lo-Fi", "Rage", "Hyper", "Future" };   // production style of the release
        auto R = [&] (int cat, const char* sub, const char* name, std::initializer_list<int> eras, Vals vals)
        {
            const bool bass = cat == cBass || cat == c808;
            if (bass)
            {
                Vals base { { m1, 0.0f }, { m2, 0.0f }, { m3, 0.0f }, { m4, 0.0f }, { m5, 0.5f }, { m6, 0.0f }, { revMix, 0.0f }, { width, 0.4f }, { octave, -1 } };
                base.insert (base.end(), vals.begin(), vals.end());
                vals = base;
            }
            for (int e : eras)
            {
                Preset p;
                p.name = juce::String (yearTag[e]) + " " + name;
                p.cat = cat; p.era = e; p.sub = sub; p.values = vals;
                bakeEra (p.values, e, bass);
                v.push_back (std::move (p));
            }
        };
        auto with = [] (Vals base, Vals extra) { for (auto& [k, x] : extra) setV (base, k, x); return base; };
        auto mm = [] (Vals vals, int slot, int src, int dst, float amt)
        {
            setV (vals, ID::mmSrc (slot), (float) src); setV (vals, ID::mmDst (slot), (float) dst); setV (vals, ID::mmAmt (slot), amt); return vals;
        };
        auto arpOn = [] (Vals vals, int rate, int mode, int octs, float gate)
        {
            setV (vals, arp, 1); setV (vals, arpRate, (float) rate); setV (vals, arpMode, (float) mode); setV (vals, arpOct, (float) octs); setV (vals, arpGate, gate);
            return vals;
        };

        const Vals pianoB { { engine, FM }, { fmAlgo, 0 }, { fmRatio, 1.0f }, { fmAmt, 0.42f }, { wave, 0.22f }, { fdecay, 0.9f },
                            { attack, 0.001f }, { decay, 3.0f }, { sustain, 0.0f }, { release, 0.45f }, { velSens, 0.75f },
                            { layerB, 1 }, { engineB, PL }, { waveB, 0.72f }, { levelB, 0.45f }, { fineB, 4.0f },
                            { cutoff, 7000 }, { keyTrack, 0.6f }, { revType, 1 }, { revMix, 0.22f }, { chorus, 0.08f } };
        const Vals epB { { engine, FM }, { fmAlgo, 5 }, { fmRatio, 1.0f }, { fmRatio2, 14.0f }, { fmAmt, 0.35f }, { fdecay, 1.2f },
                         { decay, 2.5f }, { sustain, 0.2f }, { release, 0.5f }, { chorus, 0.25f }, { lfoAmp, 0.12f }, { lfoRate, 4.5f } };
        const Vals bellB { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.45f }, { fdecay, 0.8f }, { decay, 1.8f }, { sustain, 0 }, { release, 1.2f }, { revMix, 0.25f } };
        const Vals orchB { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.2f }, { attack, 0.15f }, { sustain, 1 }, { release, 0.8f }, { revMix, 0.3f } };
        const Vals brassB { { engine, OC }, { wave, 0.15f }, { unison, 3 }, { detune, 0.12f }, { attack, 0.04f }, { sustain, 0.8f }, { release, 0.35f }, { revMix, 0.22f } };
        const Vals choirB { { engine, VX }, { wave, 0.4f }, { unison, 5 }, { detune, 0.22f }, { attack, 0.3f }, { sustain, 1 }, { release, 1.2f }, { revMix, 0.35f } };
        const Vals fluteB { { engine, FL }, { wave, 0.35f }, { attack, 0.06f }, { sustain, 0.9f }, { release, 0.6f }, { lfoPitch, 0.03f }, { lfoRate, 5.0f }, { revMix, 0.3f } };
        const Vals guitB { { engine, PL }, { wave, 0.6f }, { decay, 1.8f }, { sustain, 0 }, { release, 0.4f }, { body, 6 }, { bodyMix, 0.35f } };
        const Vals padB { { engine, VA }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.8f }, { sustain, 1 }, { release, 2.5f }, { revMix, 0.4f } };
        const Vals s808 { { engine, SB }, { sustain, 0 } };

        // ---- PIANO
        R (cPiano, "Grand", "Grand Trap Piano", { 0, 2, 4 }, pianoB);
        R (cPiano, "Bright", "Bright Stage Piano", { 1, 4 }, with (pianoB, { { cutoff, 11000 }, { fmAmt, 0.5f }, { eqHigh, 3 } }));
        R (cPiano, "Dark", "Midnight Piano", { 0, 3, 5 }, with (pianoB, { { cutoff, 2200 }, { revMix, 0.3f } }));
        R (cPiano, "Soft", "Soft Pillow Piano", { 2, 3 }, with (pianoB, { { fmAmt, 0.25f }, { velSens, 0.9f }, { cutoff, 3500 }, { decay, 4.0f }, { revMix, 0.3f } }));
        R (cPiano, "Felt", "Felt Room Piano", { 3, 5 }, with (pianoB, { { cutoff, 1800 }, { fmAmt, 0.2f }, { waveB, 0.4f }, { levelB, 0.6f }, { eqLow, 2 }, { revType, 0 } }));
        R (cPiano, "Detuned", "Detuned Upright", { 1, 3 }, with (pianoB, { { fineB, 22.0f }, { wow, 0.25f } }));
        R (cPiano, "Lo-Fi", "Dusty Tape Piano", { 3 }, with (pianoB, { { crush, 0.3f }, { wow, 0.45f }, { cutoff, 3000 } }));
        R (cPiano, "Digital", "Digital Workstation Piano", { 0, 1 }, with (pianoB, { { fmAmt, 0.55f }, { wave, 0.1f }, { levelB, 0.2f }, { cutoff, 9000 } }));
        R (cPiano, "Ambient", "Ambient Glow Piano", { 2, 6 }, with (pianoB, { { revMix, 0.5f }, { revType, 2 }, { revSize, 0.9f }, { delayMix, 0.2f } }));
        R (cPiano, "Processed", "Pitched Chop Piano", { 5, 6 }, with (pianoB, { { circuit, 0.35f }, { halftime, 0.2f }, { crush, 0.1f } }));

        // ---- KEYS
        R (cKeys, "Electric", "Tine Electric Keys", { 1, 3, 5 }, epB);
        R (cKeys, "Digital", "Digital Bell Keys", { 0, 4 }, { { engine, FM }, { fmAlgo, 2 }, { fmRatio, 2.0f }, { fmAmt, 0.4f }, { decay, 1.5f }, { sustain, 0.1f } });
        R (cKeys, "Workstation", "Workstation Keys", { 0, 1 }, { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { layerB, 1 }, { engineB, OR }, { waveB, 0.5f },
                                                                   { levelB, 0.4f }, { decay, 1.5f }, { sustain, 0.5f }, { crush, 0.1f } });
        R (cKeys, "Dreamy", "Dream Haze Keys", { 2, 3 }, with (epB, { { chorus, 0.5f }, { wow, 0.3f }, { revMix, 0.45f }, { revType, 2 }, { attack, 0.02f } }));
        R (cKeys, "Dark", "Basement Keys", { 1, 3 }, with (epB, { { cutoff, 1500 }, { reso, 0.2f }, { revMix, 0.3f } }));
        R (cKeys, "Ambient", "Ambient Pulse Keys", { 2, 6 }, { { engine, WT }, { wave, 0.3f }, { unison, 3 }, { detune, 0.2f }, { attack, 0.05f }, { sustain, 0.6f },
                                                              { release, 2.0f }, { revMix, 0.5f }, { delayMix, 0.3f }, { lfoFilter, 0.2f } });
        R (cKeys, "Glass", "Glass Keys", { 2, 5 }, { { engine, FM }, { fmAlgo, 4 }, { fmRatio, 3.0f }, { fmAmt, 0.3f }, { decay, 1.5f }, { sustain, 0.1f }, { body, 3 }, { bodyMix, 0.35f } });
        R (cKeys, "Broken", "Broken Cassette Keys", { 3, 6 }, with (epB, { { wow, 0.6f }, { crush, 0.35f }, { tape, 1 }, { circuit, 0.15f } }));
        R (cKeys, "Hybrid", "Hybrid Keys Stack", { 4, 5 }, with (epB, { { layerB, 1 }, { engineB, WT }, { waveB, 0.5f }, { levelB, 0.5f }, { drive, 0.2f } }));

        // ---- BELLS (the trap bell through every era)
        R (cBells, "Trap Bell", "Trap Bell", { 0, 1, 2, 3, 4, 5, 6 }, bellB);
        R (cBells, "Glass", "Glass Bell", { 2, 5 }, with (bellB, { { fmRatio, 4.0f }, { fmAlgo, 4 }, { fmAmt, 0.3f }, { decay, 2.5f }, { body, 3 }, { bodyMix, 0.3f } }));
        R (cBells, "Tubular", "Tubular Bell", { 0, 2 }, { { engine, MD }, { wave, 0.9f }, { octave, -1 }, { fmAmt, 0.5f }, { decay, 4.0f }, { sustain, 0 }, { release, 3.0f }, { revMix, 0.35f } });
        R (cBells, "Music Box", "Music Box", { 1, 3, 6 }, { { engine, MD }, { wave, 0.95f }, { octave, 2 }, { decay, 1.2f }, { sustain, 0 }, { release, 1.0f }, { wow, 0.3f }, { revMix, 0.35f } });
        R (cBells, "Digital", "Digital Bell", { 1, 4 }, { { engine, WT }, { wave, 0.7f }, { warpMode, 3 }, { fmAmt, 0.3f }, { decay, 1.0f }, { sustain, 0 }, { release, 0.8f }, { delayMix, 0.25f } });
        R (cBells, "Dark", "Dark Bell", { 0, 2, 5 }, with (bellB, { { fmAmt, 0.4f }, { cutoff, 1800 }, { revMix, 0.35f }, { revSize, 0.9f } }));
        R (cBells, "Metallic", "Metal Bell", { 3, 4 }, with (bellB, { { fmRatio, 1.41f }, { fmAlgo, 4 }, { fmAmt, 0.6f }, { body, 4 }, { bodyMix, 0.4f } }));
        R (cBells, "Distorted", "Blown Bell", { 4, 5 }, with (bellB, { { fmAmt, 0.5f }, { drive, 0.6f }, { driveType, 3 }, { crush, 0.2f } }));
        R (cBells, "Ambient", "Ambient Bell", { 2, 6 }, with (bellB, { { fmRatio, 5.0f }, { fmAmt, 0.25f }, { attack, 0.01f }, { decay, 3.0f }, { revMix, 0.55f }, { revType, 2 }, { delayMix, 0.3f } }));
        R (cBells, "Hybrid", "Hybrid Bell Pluck", { 4, 5 }, with (bellB, { { layerB, 1 }, { engineB, PL }, { waveB, 0.7f }, { levelB, 0.6f } }));

        // ---- PLUCKS
        const Vals pluckB { { engine, VA }, { wave, 0.3f }, { cutoff, 2000 }, { fenv, 0.6f }, { fdecay, 0.12f }, { decay, 0.25f }, { sustain, 0 }, { release, 0.1f } };
        R (cPlucks, "Short", "Short Trap Pluck", { 0, 3, 5 }, pluckB);
        R (cPlucks, "Soft", "Soft Pluck", { 2, 3 }, with (pluckB, { { wave, 0.5f }, { cutoff, 1500 }, { fenv, 0.4f }, { fdecay, 0.25f }, { decay, 0.5f }, { revMix, 0.3f } }));
        R (cPlucks, "Digital", "Digital Pluck", { 1, 4 }, { { engine, WT }, { wave, 0.5f }, { cutoff, 3000 }, { fenv, 0.5f }, { fdecay, 0.15f }, { decay, 0.35f }, { sustain, 0 }, { delayMix, 0.25f } });
        R (cPlucks, "Metallic", "Metallic Pluck", { 4, 5 }, { { engine, FM }, { fmRatio, 7.0f }, { fmAmt, 0.5f }, { decay, 0.4f }, { sustain, 0 }, { body, 4 }, { bodyMix, 0.3f } });
        R (cPlucks, "Guitar-like", "Pick Pluck", { 1, 3 }, with (guitB, { { decay, 1.0f }, { bodyMix, 0.4f } }));
        R (cPlucks, "Synthetic", "Synthetic Pluck", { 2, 6 }, with (pluckB, { { wave, 0.8f }, { unison, 3 }, { detune, 0.25f }, { fenv, 0.7f }, { fdecay, 0.1f }, { reso, 0.35f } }));
        R (cPlucks, "Rage", "Rage Pluck", { 4, 5 }, with (pluckB, { { wave, 0.0f }, { unison, 7 }, { detune, 0.45f }, { decay, 0.3f }, { sustain, 0.05f }, { drive, 0.4f } }));
        R (cPlucks, "Ambient", "Ambient Pluck", { 2, 6 }, { { engine, PL }, { wave, 0.5f }, { decay, 1.5f }, { sustain, 0 }, { revMix, 0.5f }, { revType, 2 }, { delayMix, 0.35f } });
        R (cPlucks, "Hybrid", "Hybrid Pluck", { 5, 6 }, { { engine, PL }, { wave, 0.6f }, { decay, 0.9f }, { sustain, 0 }, { layerB, 1 }, { engineB, FM }, { fmRatioB, 3.5f },
                                                          { fmAmtB, 0.4f }, { levelB, 0.5f } });

        // ---- MALLETS
        R (cMallets, "Marimba", "Marimba", { 1, 3 }, { { engine, MD }, { wave, 0.5f }, { fmAmt, 0.5f }, { decay, 0.8f }, { sustain, 0 }, { revMix, 0.2f } });
        R (cMallets, "Kalimba", "Kalimba", { 2, 5 }, { { engine, MD }, { wave, 0.2f }, { fmAmt, 0.4f }, { decay, 1.0f }, { sustain, 0 }, { wow, 0.15f } });
        R (cMallets, "Xylophone", "Xylophone", { 0, 4 }, { { engine, MD }, { wave, 0.5f }, { octave, 1 }, { fmAmt, 0.8f }, { decay, 0.35f }, { sustain, 0 } });
        R (cMallets, "Celesta", "Celesta", { 0, 2 }, { { engine, MD }, { wave, 0.8f }, { octave, 1 }, { fmAmt, 0.3f }, { decay, 1.2f }, { sustain, 0 }, { revMix, 0.3f } });
        R (cMallets, "Metallic", "Metal Mallet", { 3, 5 }, { { engine, MD }, { wave, 0.85f }, { fmAmt, 0.7f }, { decay, 0.9f }, { sustain, 0 }, { body, 4 }, { bodyMix, 0.4f } });
        R (cMallets, "Wooden", "Wood Mallet", { 1, 3 }, { { engine, MD }, { wave, 0.45f }, { fmAmt, 0.3f }, { decay, 0.6f }, { sustain, 0 }, { body, 5 }, { bodyMix, 0.5f } });
        R (cMallets, "Hybrid", "Hybrid Mallet", { 4, 6 }, { { engine, MD }, { wave, 0.2f }, { decay, 1.0f }, { sustain, 0 }, { layerB, 1 }, { engineB, FM }, { fmRatioB, 3.5f },
                                                             { fmAmtB, 0.3f }, { levelB, 0.5f } });

        // ---- GUITAR
        R (cGuitar, "Nylon", "Nylon Guitar", { 0, 2, 3 }, with (guitB, { { wave, 0.45f }, { bodyMix, 0.5f }, { revMix, 0.2f } }));
        R (cGuitar, "Acoustic", "Acoustic Guitar", { 1, 3 }, with (guitB, { { wave, 0.7f }, { decay, 2.0f }, { body, 5 }, { bodyMix, 0.45f } }));
        R (cGuitar, "Electric", "Electric Guitar", { 3, 4 }, with (guitB, { { wave, 0.8f }, { decay, 2.2f }, { body, 0 }, { drive, 0.35f }, { driveType, 1 }, { chorus, 0.2f } }));
        R (cGuitar, "Clean", "Clean Chorus Guitar", { 2, 5 }, with (guitB, { { wave, 0.75f }, { decay, 2.0f }, { body, 0 }, { chorus, 0.45f }, { delayMix, 0.2f } }));
        R (cGuitar, "Muted", "Muted Guitar", { 1, 4 }, with (guitB, { { wave, 0.35f }, { decay, 0.25f }, { release, 0.08f }, { bodyMix, 0.4f } }));
        R (cGuitar, "Processed", "Processed Guitar Loop", { 3, 5 }, with (guitB, { { decay, 1.5f }, { wow, 0.4f }, { crush, 0.2f }, { delayMix, 0.25f }, { halftime, 0.2f } }));
        R (cGuitar, "Reverse", "Reverse Guitar", { 2, 6 }, with (guitB, { { wave, 0.65f }, { decay, 2.0f }, { reverse, 0.6f }, { revMix, 0.4f } }));
        R (cGuitar, "Ambient", "Ambient Guitar", { 2, 3 }, with (guitB, { { decay, 2.5f }, { revMix, 0.55f }, { revType, 2 }, { delayMix, 0.35f }, { chorus, 0.3f } }));
        R (cGuitar, "Synth Hybrid", "Synth Guitar", { 4, 6 }, with (guitB, { { wave, 0.7f }, { layerB, 1 }, { engineB, VA }, { waveB, 0.5f }, { levelB, 0.4f }, { fenv, 0.5f } }));

        // ---- STRINGS
        R (cStrings, "Ensemble", "String Ensemble", { 0, 2, 4 }, orchB);
        R (cStrings, "Solo", "Solo Violin", { 1, 3 }, with (orchB, { { unison, 1 }, { attack, 0.08f }, { release, 0.5f }, { mono, 1 }, { legato, 1 }, { glide, 0.08f } }));
        R (cStrings, "Pizzicato", "Pizzicato", { 0, 1, 5 }, with (orchB, { { unison, 3 }, { attack, 0.002f }, { decay, 0.18f }, { sustain, 0 }, { release, 0.06f }, { punch, 0.6f }, { revMix, 0.15f } }));
        R (cStrings, "Staccato", "Staccato Strings", { 0, 4 }, with (orchB, { { unison, 3 }, { attack, 0.005f }, { decay, 0.3f }, { sustain, 0.1f }, { release, 0.1f }, { punch, 0.5f } }));
        R (cStrings, "Sustained", "Long Strings", { 2, 5 }, with (orchB, { { attack, 0.6f }, { release, 2.0f }, { revMix, 0.4f } }));
        R (cStrings, "Synthetic", "Synth Strings", { 1, 3 }, { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.3f }, { cutoff, 3500 }, { attack, 0.3f },
                                                                { sustain, 1 }, { release, 1.0f }, { chorus, 0.4f } });
        R (cStrings, "Dark", "Dark Low Strings", { 0, 3 }, with (orchB, { { octave, -1 }, { cutoff, 2000 }, { attack, 0.2f }, { revMix, 0.35f } }));
        R (cStrings, "Cinematic", "Cinematic Strings", { 2, 4, 6 }, with (orchB, { { layerB, 1 }, { engineB, OC }, { waveB, 0.5f }, { octaveB, -1 }, { levelB, 0.6f },
                                                                                  { attack, 0.3f }, { revMix, 0.45f }, { revSize, 0.9f } }));
        R (cStrings, "Hybrid", "Hybrid String Stack", { 5, 6 }, with (orchB, { { layerB, 1 }, { engineB, WT }, { waveB, 0.4f }, { detuneB, 0.2f }, { levelB, 0.5f } }));

        // ---- BRASS
        R (cBrass, "Horns", "French Horns", { 0, 2 }, with (brassB, { { attack, 0.05f }, { release, 0.4f }, { revMix, 0.25f } }));
        R (cBrass, "Trumpets", "Trumpet Section", { 0, 1 }, with (brassB, { { wave, 0.05f }, { attack, 0.02f }, { sustain, 0.7f }, { release, 0.2f }, { punch, 0.3f } }));
        R (cBrass, "Sections", "Brass Section", { 1, 3 }, with (brassB, { { unison, 5 }, { detune, 0.15f } }));
        R (cBrass, "Synth Brass", "Synth Brass", { 1, 4 }, { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 1200 }, { fenv, 0.7f },
                                                             { fattack, 0.05f }, { fdecay, 0.4f }, { fsustain, 0.5f }, { sustain, 0.9f } });
        R (cBrass, "Dark Brass", "Dark Brass", { 0, 3, 5 }, with (brassB, { { wave, 0.1f }, { octave, -1 }, { cutoff, 1800 }, { revMix, 0.3f } }));
        R (cBrass, "Trap Brass", "Trap Brass Stab", { 0, 1, 4 }, with (brassB, { { attack, 0.005f }, { decay, 0.45f }, { sustain, 0.1f }, { release, 0.2f }, { punch, 0.6f } }));
        R (cBrass, "Hybrid Brass", "Hybrid Brass", { 4, 6 }, with (brassB, { { wave, 0.1f }, { layerB, 1 }, { engineB, VA }, { waveB, 0.0f }, { unisonB, 5 },
                                                                             { detuneB, 0.3f }, { levelB, 0.5f }, { drive, 0.3f } }));

        // ---- CHOIR / VOCAL
        R (cChoir, "Male", "Male Choir", { 0, 2 }, with (choirB, { { wave, 0.8f }, { octave, -1 } }));
        R (cChoir, "Female", "Female Choir", { 1, 3 }, with (choirB, { { wave, 0.1f }, { lfoPitch, 0.04f } }));
        R (cChoir, "Mixed", "Mixed Choir", { 0, 4 }, with (choirB, { { unison, 6 }, { layerB, 1 }, { engineB, VX }, { waveB, 0.8f }, { octaveB, -1 }, { levelB, 0.6f } }));
        R (cChoir, "Vowels", "Vowel Choir", { 2, 5 }, mm (with (choirB, { { unison, 4 }, { lfo2Rate, 0.3f } }), 0, srcLfo2, dstWaveA, 0.4f));
        R (cChoir, "Air", "Air Choir", { 2, 6 }, with (choirB, { { wave, 0.3f }, { unison, 7 }, { detune, 0.4f }, { attack, 1.0f }, { release, 3.0f }, { revMix, 0.5f },
                                                                { revType, 2 }, { cutoff, 6000 } }));
        R (cChoir, "Vocal Texture", "Vocal Texture", { 3, 5 }, with (choirB, { { wave, 0.6f }, { circuit, 0.3f }, { crush, 0.2f }, { halftime, 0.15f } }));
        R (cChoir, "Reverse Vocal", "Reverse Vocal", { 2, 6 }, with (choirB, { { wave, 0.5f }, { reverse, 0.7f }, { revMix, 0.4f } }));
        R (cChoir, "Synthetic Vocal", "Synthetic Vocal", { 4, 5 }, with (choirB, { { unison, 3 }, { drive, 0.3f }, { driveType, 4 } }));
        R (cChoir, "Ghost Choir", "Ghost Choir", { 0, 2, 6 }, with (choirB, { { wave, 0.9f }, { unison, 6 }, { attack, 0.8f }, { ghost, 0.4f }, { revMix, 0.5f }, { revSize, 0.95f } }));

        // ---- WOODWIND
        R (cWoodwind, "Flute", "Trap Flute", { 0, 1, 2, 4 }, fluteB);
        R (cWoodwind, "Piccolo", "Piccolo", { 0, 2 }, with (fluteB, { { wave, 0.25f }, { octave, 1 }, { attack, 0.04f }, { release, 0.4f } }));
        R (cWoodwind, "Clarinet-like", "Reed Clarinet", { 1, 3 }, { { engine, VA }, { wave, 0.5f }, { cutoff, 1600 }, { reso, 0.1f }, { attack, 0.05f }, { sustain, 0.9f },
                                                                     { release, 0.3f }, { lfoPitch, 0.02f }, { revMix, 0.25f } });
        R (cWoodwind, "Breath", "Breath Flute", { 2, 6 }, with (fluteB, { { wave, 1.0f }, { attack, 0.1f }, { sustain, 0.8f }, { revMix, 0.45f } }));
        R (cWoodwind, "Ethnic Flute", "Temple Flute", { 2, 3, 5 }, with (fluteB, { { wave, 0.5f }, { glide, 0.15f }, { mono, 1 }, { legato, 1 }, { lfoPitch, 0.05f },
                                                                                  { revMix, 0.45f }, { delayMix, 0.25f } }));
        R (cWoodwind, "Synthetic Wind", "Synth Wind", { 4, 5 }, { { engine, WT }, { wave, 0.15f }, { cutoff, 3000 }, { attack, 0.05f }, { sustain, 0.9f }, { drive, 0.3f },
                                                                  { lfoPitch, 0.03f } });

        // ---- LEAD
        const Vals leadB { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { sustain, 1 }, { release, 0.2f }, { mono, 1 }, { legato, 1 } };
        R (cLead, "Analog", "Analog Lead", { 1, 3 }, with (leadB, { { unison, 2 }, { detune, 0.1f }, { cutoff, 3000 }, { reso, 0.3f }, { fenv, 0.4f }, { glide, 0.08f } }));
        R (cLead, "Digital", "Digital Lead", { 1, 4, 5 }, with (leadB, { { engine, WT }, { wave, 0.6f }, { unison, 3 }, { detune, 0.2f }, { delayMix, 0.2f } }));
        R (cLead, "Mono", "Mono Square Lead", { 0, 3 }, with (leadB, { { wave, 0.5f }, { unison, 1 }, { cutoff, 4000 } }));
        R (cLead, "Portamento", "Glide Lead", { 3, 4, 5 }, with (leadB, { { glide, 0.3f } }));
        R (cLead, "Rage", "Rage Lead", { 4, 5 }, with (leadB, { { unison, 8 }, { detune, 0.5f }, { drive, 0.5f }, { driveType, 3 }, { width, 0.9f }, { mono, 0 } }));
        R (cLead, "Detuned", "Detuned Stack Lead", { 3, 4 }, with (leadB, { { unison, 7 }, { detune, 0.6f }, { chorus, 0.2f }, { mono, 0 } }));
        R (cLead, "Distorted", "Fuzz Lead", { 4, 5 }, with (leadB, { { wave, 0.5f }, { unison, 3 }, { drive, 0.7f }, { driveType, 4 }, { cutoff, 5000 } }));
        R (cLead, "Metallic", "Metallic Lead", { 4, 6 }, { { engine, FM }, { fmAlgo, 1 }, { fmRatio, 1.41f }, { fmAmt, 0.5f }, { sustain, 1 }, { release, 0.3f },
                                                           { body, 4 }, { bodyMix, 0.3f }, { mono, 1 } });
        R (cLead, "Future", "Future Lead", { 5, 6 }, mm ({ { engine, WT }, { wave, 0.8f }, { warpMode, 1 }, { fmAmt, 0.5f }, { sustain, 1 }, { release, 0.3f },
                                                            { circuit, 0.15f }, { alive, 2 }, { mono, 1 }, { glide, 0.1f } }, 0, srcEnv2, dstFmA, 0.4f));

        // ---- PADS
        R (cPads, "Dark", "Dark Pad", { 0, 3, 5 }, with (padB, { { cutoff, 900 } }));
        R (cPads, "Space", "Space Pad", { 2, 6 }, with (padB, { { engine, WT }, { wave, 0.4f }, { detune, 0.35f }, { attack, 1.2f }, { release, 3.0f }, { revMix, 0.55f },
                                                               { revType, 2 }, { delayMix, 0.3f } }));
        R (cPads, "Dream", "Dream Pad", { 2, 3 }, with (padB, { { wave, 0.3f }, { unison, 6 }, { detune, 0.4f }, { chorus, 0.5f }, { wow, 0.3f }, { attack, 0.6f }, { revMix, 0.45f } }));
        R (cPads, "Analog", "Analog Pad", { 1, 3 }, with (padB, { { wave, 0.1f }, { unison, 4 }, { cutoff, 2500 }, { lfoFilter, 0.15f }, { attack, 0.5f }, { release, 2.0f } }));
        R (cPads, "Digital", "Digital Pad", { 1, 4 }, with (padB, { { engine, WT }, { wave, 0.6f }, { unison, 4 }, { attack, 0.4f }, { release, 2.0f }, { phaser, 0.3f } }));
        R (cPads, "Choir", "Choir Pad", { 0, 2 }, with (orchB, { { wave, 1.0f }, { attack, 0.6f }, { release, 2.0f }, { revMix, 0.4f } }));
        R (cPads, "Evolving", "Evolving Pad", { 3, 5 }, mm (with (padB, { { engine, WT }, { wave, 0.3f }, { lfo2Rate, 0.1f }, { attack, 1.0f }, { release, 3.0f } }), 0, srcLfo2, dstWaveA, 0.4f));
        R (cPads, "Granular", "Grain Pad", { 5, 6 }, with (choirB, { { wave, 0.5f }, { unison, 6 }, { reverse, 0.4f }, { alive, 4 }, { attack, 1.0f }, { release, 3.0f },
                                                                    { revType, 2 }, { revMix, 0.5f } }));
        R (cPads, "Spectral", "Spectral Pad", { 6 }, mm (with (padB, { { engine, WT }, { wave, 0.5f }, { warpMode, 2 }, { fmAmt, 0.6f }, { phaser, 0.4f }, { flanger, 0.2f },
                                                                      { lfo2Rate, 0.2f } }), 0, srcLfo2, dstFmA, 0.4f));
        R (cPads, "Future", "Future Pad", { 5, 6 }, with (orchB, { { wave, 0.7f }, { layerB, 1 }, { engineB, WT }, { waveB, 0.5f }, { levelB, 0.5f }, { attack, 0.8f },
                                                                   { release, 3.0f }, { reverse, 0.3f }, { ghost, 0.3f }, { alive, 3 } }));

        // ---- SYNTH
        R (cSynth, "Poly", "Poly Synth", { 1, 3 }, { { engine, VA }, { wave, 0.2f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 3500 }, { fenv, 0.3f }, { decay, 1.0f }, { sustain, 0.5f } });
        R (cSynth, "Analog", "Analog Poly", { 0, 2 }, { { engine, VA }, { wave, 0.5f }, { unison, 2 }, { cutoff, 2000 }, { fenv, 0.5f }, { sustain, 0.6f }, { chorus, 0.3f } });
        R (cSynth, "Digital", "Digital Stab Synth", { 1, 4 }, { { engine, WT }, { wave, 0.5f }, { unison, 3 }, { decay, 0.5f }, { sustain, 0.2f }, { release, 0.2f } });
        R (cSynth, "FM", "FM Poly", { 0, 3 }, { { engine, FM }, { fmAlgo, 2 }, { fmRatio, 2.0f }, { fmAmt, 0.45f }, { decay, 1.0f }, { sustain, 0.4f } });
        R (cSynth, "Wavetable", "Wavetable Poly", { 4, 5 }, mm ({ { engine, WT }, { wave, 0.3f }, { warpMode, 1 }, { fmAmt, 0.3f }, { unison, 4 }, { detune, 0.25f },
                                                                  { sustain, 0.7f } }, 0, srcEnv2, dstFmA, 0.3f));
        R (cSynth, "Hybrid", "Hybrid Synth", { 4, 5 }, { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.25f }, { sustain, 0.6f }, { layerB, 1 }, { engineB, FM },
                                                          { fmRatioB, 2.0f }, { fmAmtB, 0.4f }, { levelB, 0.5f } });
        R (cSynth, "Spectral", "Spectral Synth", { 6 }, { { engine, WT }, { wave, 0.5f }, { warpMode, 4 }, { fmAmt, 0.6f }, { sustain, 0.7f }, { phaser, 0.3f } });
        R (cSynth, "Granular", "Granular Synth", { 5, 6 }, with (choirB, { { attack, 0.02f }, { sustain, 0.6f }, { reverse, 0.5f }, { alive, 4 }, { circuit, 0.1f } }));

        // ---- BASS
        R (cBass, "Sub", "Sub Bass", { 0, 4 }, { { engine, SB }, { wave, 0.1f }, { sustain, 1 }, { release, 0.2f } });
        R (cBass, "Synth Bass", "Synth Bass", { 1, 3 }, { { engine, VA }, { wave, 0.3f }, { cutoff, 800 }, { fenv, 0.5f }, { fdecay, 0.2f }, { sustain, 0.8f } });
        R (cBass, "Reese", "Reese Bass", { 3, 4 }, { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.4f }, { cutoff, 800 }, { sustain, 1 } });
        R (cBass, "Distorted", "Distorted Bass", { 4, 5 }, { { engine, VA }, { wave, 0.5f }, { unison, 2 }, { m3, 0.6f }, { driveType, 3 }, { cutoff, 1200 }, { sustain, 1 } });
        R (cBass, "Pluck Bass", "Pluck Bass", { 1, 3 }, { { engine, VA }, { wave, 0.5f }, { cutoff, 600 }, { fenv, 0.7f }, { fdecay, 0.15f }, { decay, 0.3f }, { sustain, 0 } });
        R (cBass, "Hybrid Bass", "Hybrid Bass", { 5, 6 }, { { engine, SB }, { wave, 0.3f }, { sustain, 1 }, { layerB, 1 }, { engineB, VA }, { waveB, 0.0f }, { octaveB, 0 },
                                                             { levelB, 0.35f }, { cutoff, 1500 } });

        // ---- 808 (Sub engine: WAVE = body drive, FM AMT = pitch drop)
        R (c808, "Clean", "Clean 808", { 0, 2 }, with (s808, { { wave, 0.1f }, { fmAmt, 0.1f }, { decay, 4.0f }, { release, 0.4f } }));
        R (c808, "Short", "Short 808", { 1, 5 }, with (s808, { { wave, 0.4f }, { fmAmt, 0.25f }, { decay, 0.5f }, { release, 0.1f }, { punch, 0.4f } }));
        R (c808, "Long", "Long 808", { 0, 3 }, with (s808, { { wave, 0.3f }, { fmAmt, 0.1f }, { decay, 7.0f }, { release, 0.6f } }));
        R (c808, "Punch", "Punch 808", { 2, 4 }, with (s808, { { wave, 0.5f }, { fmAmt, 0.4f }, { decay, 1.5f }, { m6, 0.8f }, { punch, 0.6f } }));
        R (c808, "Clipped", "Clipped 808", { 4, 5 }, with (s808, { { wave, 0.8f }, { fmAmt, 0.2f }, { decay, 2.5f }, { m3, 0.5f }, { driveType, 2 } }));
        R (c808, "Distorted", "Distorted 808", { 4, 5 }, with (s808, { { wave, 0.6f }, { fmAmt, 0.25f }, { decay, 2.5f }, { m3, 0.7f }, { driveType, 3 } }));
        R (c808, "Saturated", "Saturated 808", { 2, 3 }, with (s808, { { wave, 0.45f }, { fmAmt, 0.15f }, { decay, 3.0f }, { m3, 0.35f }, { driveType, 1 } }));
        R (c808, "Glide", "Glide 808", { 3, 4, 5 }, with (s808, { { wave, 0.45f }, { fmAmt, 0.15f }, { decay, 4.0f }, { sustain, 0.3f }, { glide, 0.3f }, { legato, 1 } }));
        R (c808, "Textured", "Textured 808", { 3, 6 }, with (s808, { { wave, 0.4f }, { fmAmt, 0.15f }, { decay, 3.0f }, { crush, 0.3f }, { wow, 0.2f } }));
        R (c808, "Hybrid", "Hybrid 808", { 5 }, with (s808, { { wave, 0.5f }, { fmAmt, 0.2f }, { decay, 3.0f }, { layerB, 1 }, { engineB, VA }, { waveB, 0.0f },
                                                              { unisonB, 3 }, { detuneB, 0.3f }, { octaveB, 0 }, { levelB, 0.3f } }));
        R (c808, "Future", "Future 808", { 6 }, with (s808, { { wave, 0.5f }, { fmAmt, 0.3f }, { decay, 3.0f }, { circuit, 0.2f }, { body, 4 }, { bodyMix, 0.3f } }));

        // ---- TEXTURE
        const Vals texB { { sustain, 1 }, { attack, 1.0f }, { release, 3.0f }, { revMix, 0.45f } };
        R (cTexture, "Vinyl", "Vinyl Dust", { 0, 3 }, with (texB, { { engine, FL }, { wave, 1.0f }, { cutoff, 3000 }, { crush, 0.6f }, { wow, 0.3f } }));
        R (cTexture, "Tape", "Tape Hiss Bed", { 3, 5 }, with (texB, { { engine, VA }, { wave, 0.5f }, { cutoff, 1500 }, { crush, 0.45f }, { wow, 0.6f }, { tape, 1 } }));
        R (cTexture, "Noise", "Noise Wind", { 2, 6 }, with (texB, { { engine, FL }, { wave, 1.0f }, { octave, 1 }, { cutoff, 5000 }, { reso, 0.4f }, { lfoFilter, 0.5f },
                                                                     { lfoRate, 0.2f }, { attack, 1.5f }, { revMix, 0.5f } }));
        R (cTexture, "Granular", "Grain Cloud", { 5, 6 }, with (texB, { { engine, OC }, { wave, 0.7f }, { reverse, 0.6f }, { alive, 5 }, { revType, 2 }, { revMix, 0.6f } }));
        R (cTexture, "Reverse", "Reverse Air", { 2, 4 }, with (texB, { { engine, VX }, { wave, 0.4f }, { reverse, 0.8f }, { revMix, 0.5f } }));
        R (cTexture, "Atmosphere", "Night Atmosphere", { 2, 5, 6 }, with (texB, { { engine, WT }, { wave, 0.5f }, { unison, 6 }, { detune, 0.4f }, { cutoff, 2000 },
                                                                                   { attack, 2.0f }, { release, 4.0f }, { revMix, 0.6f }, { revType, 2 }, { delayMix, 0.3f } }));
        R (cTexture, "Field", "Rain Field", { 3, 6 }, with (texB, { { engine, FL }, { wave, 1.0f }, { octave, 2 }, { crush, 0.5f }, { lfoAmp, 0.4f }, { lfoRate, 7.0f } }));
        R (cTexture, "Mechanical", "Machine Hum", { 4, 6 }, with (texB, { { engine, OR }, { wave, 0.9f }, { octave, -1 }, { body, 4 }, { bodyMix, 0.6f }, { circuit, 0.3f } }));
        R (cTexture, "Digital Artifacts", "Digital Glitch Bed", { 5, 6 }, with (texB, { { engine, WT }, { wave, 0.8f }, { warpMode, 3 }, { circuit, 0.6f }, { crush, 0.4f } }));

        // ---- ARP / SEQUENCE
        R (cArp, "Melodic", "Melodic Bell Arp", { 1, 3, 5 }, arpOn (with (bellB, { { decay, 0.8f }, { release, 0.4f }, { delayMix, 0.25f } }), 1, 0, 2, 0.6f));
        R (cArp, "Gated", "Gated Chord Pad", { 2, 4, 5 }, with (padB, { { attack, 0.01f }, { lfoSync, 1 }, { lfoDiv, 4 }, { lfoShape, 3 }, { lfoAmp, 0.9f } }));
        R (cArp, "Rhythmic", "Rhythmic Pluck Seq", { 3, 4 }, arpOn (pluckB, 1, 2, 1, 0.4f));
        R (cArp, "Pulsing", "Pulse Arp", { 4, 5 }, arpOn (with (leadB, { { mono, 0 }, { unison, 3 }, { decay, 0.2f }, { sustain, 0.2f } }), 3, 0, 1, 0.3f));
        R (cArp, "Triplet", "Triplet Arp", { 3, 5 }, arpOn (with (bellB, { { decay, 0.6f } }), 2, 0, 2, 0.5f));
        R (cArp, "Polyrhythmic", "Polyrhythm Arp", { 5, 6 }, arpOn (with (pluckB, { { lfoSync, 1 }, { lfoDiv, 4 }, { lfoFilter, 0.5f } }), 2, 2, 2, 0.5f));
        R (cArp, "Generative", "Generative Arp", { 6, 5 }, arpOn (with (bellB, { { alive, 3 }, { delayMix, 0.3f } }), 1, 3, 3, 0.55f));

        // ---- FX
        R (cFX, "Riser", "Tonal Riser", { 4, 6 }, mm (mm ({ { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.5f }, { cutoff, 300 }, { reso, 0.4f }, { attack, 3.0f },
                                                            { sustain, 1 }, { release, 2.0f }, { e3attack, 4.0f }, { e3sustain, 1 }, { revMix, 0.45f } },
                                                          0, srcEnv3, dstCutoff, 0.9f), 1, srcEnv3, dstPitch, 0.5f));
        R (cFX, "Impact", "Sub Impact", { 3, 5 }, { { engine, SB }, { octave, -2 }, { wave, 0.6f }, { fmAmt, 0.4f }, { decay, 2.5f }, { sustain, 0 }, { release, 1.5f },
                                                     { bend, 0.8f }, { bendMode, 0 }, { bendSemis, -24 }, { revMix, 0.4f }, { punch, 0.7f } });
        R (cFX, "Reverse", "Reverse Hit", { 2, 4 }, { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.4f }, { attack, 1.5f }, { sustain, 1 }, { release, 0.05f }, { reverse, 0.8f } });
        R (cFX, "Transition", "Sweep Transition", { 3, 5 }, { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { cutoff, 400 }, { reso, 0.5f },
                                                               { lfoFilter, 0.8f }, { lfoRate, 0.15f }, { sustain, 1 }, { release, 1.5f }, { revMix, 0.4f } });
        R (cFX, "Atmosphere", "Haunted Atmosphere", { 2, 6 }, with (choirB, { { wave, 0.9f }, { ghost, 0.5f }, { revType, 2 }, { revMix, 0.7f }, { attack, 2.0f } }));
        R (cFX, "Downer", "Pitch Downer", { 4 }, { { engine, VA }, { wave, 0.3f }, { unison, 3 }, { detune, 0.3f }, { decay, 3.0f }, { sustain, 0 }, { bend, 1.0f },
                                                   { bendMode, 0 }, { bendSemis, -24 }, { delayMix, 0.3f } });
        R (cFX, "Noise", "White Noise Wash", { 3, 6 }, { { engine, FL }, { wave, 1.0f }, { octave, 2 }, { crush, 0.3f }, { attack, 0.5f }, { sustain, 1 }, { release, 2.0f },
                                                          { revMix, 0.5f } });
        R (cFX, "Tonal FX", "Tonal Zap", { 5, 6 }, { { engine, FM }, { fmRatio, 7.0f }, { fmAmt, 0.6f }, { decay, 0.6f }, { sustain, 0 }, { bend, 0.9f }, { bendMode, 1 },
                                                      { bendSemis, 12 }, { delayMix, 0.35f } });


        // ---- v0.7 trap essentials (what trap producers reach for most: brass, bells, strings, flutes, hits)
        R (cBrass, "Trap Brass", "Hard Horn Stab", { 0, 4, 5 }, with (brassB, { { attack, 0.003f }, { decay, 0.35f }, { sustain, 0 }, { release, 0.15f }, { punch, 0.7f }, { drive, 0.2f } }));
        R (cBrass, "Hits", "Orchestra Brass Hit", { 0, 1, 4 }, with (brassB, { { layerB, 1 }, { engineB, OC }, { waveB, 0.5f }, { octaveB, -1 }, { levelB, 0.7f },
                                                                                { attack, 0.002f }, { decay, 0.6f }, { sustain, 0 }, { release, 0.3f }, { punch, 0.8f } }));
        R (cBrass, "Hits", "Epic Stab Hit", { 0, 5 }, with (brassB, { { wave, 0.1f }, { unison, 5 }, { layerB, 1 }, { engineB, SB }, { octaveB, -2 }, { levelB, 0.5f },
                                                                      { attack, 0.002f }, { decay, 0.5f }, { sustain, 0 }, { punch, 0.9f }, { drive, 0.25f } }));
        R (cBrass, "Sections", "Anthem Brass", { 0, 3, 4 }, with (brassB, { { unison, 6 }, { detune, 0.2f }, { attack, 0.03f }, { sustain, 0.9f }, { chorus, 0.15f }, { revMix, 0.3f } }));
        R (cBrass, "Dark Brass", "Low Brass Swell", { 1, 3 }, with (brassB, { { octave, -1 }, { attack, 0.5f }, { sustain, 1 }, { release, 1.0f }, { cutoff, 1600 } }));
        R (cBrass, "Horns", "Drill Horn Riff", { 4, 5 }, with (brassB, { { wave, 0.2f }, { mono, 1 }, { legato, 1 }, { glide, 0.12f }, { sustain, 0.8f }, { punch, 0.4f } }));
        R (cBrass, "Synth Brass", "Saw Brass Stab", { 4, 5 }, { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.25f }, { cutoff, 900 }, { fenv, 0.8f },
                                                                { fdecay, 0.25f }, { decay, 0.4f }, { sustain, 0.2f }, { punch, 0.5f } });
        R (cBells, "Trap Bell", "Icy Trap Bell", { 0, 3, 5 }, with (bellB, { { fmRatio, 4.0f }, { fmAmt, 0.5f }, { fdecay, 0.5f }, { decay, 1.2f }, { revMix, 0.3f }, { delayMix, 0.2f } }));
        R (cBells, "Trap Bell", "Minor Bell Lead", { 1, 4 }, with (bellB, { { fmRatio, 3.0f }, { sustain, 0.3f }, { decay, 2.0f }, { release, 1.5f } }));
        R (cBells, "Dark", "Horror Bell", { 0, 6 }, { { engine, MD }, { wave, 0.9f }, { fmAmt, 0.6f }, { decay, 3.0f }, { sustain, 0 }, { release, 2.0f }, { wow, 0.2f },
                                                     { cutoff, 2500 }, { revMix, 0.45f }, { ghost, 0.3f } });
        R (cStrings, "Staccato", "Trap Staccato Violins", { 0, 1, 4 }, with (orchB, { { attack, 0.003f }, { decay, 0.22f }, { sustain, 0 }, { release, 0.1f }, { punch, 0.7f }, { width, 0.8f } }));
        R (cStrings, "Ensemble", "Dark Trap Strings", { 0, 3, 5 }, with (orchB, { { cutoff, 3000 }, { attack, 0.12f }, { revMix, 0.35f } }));
        R (cStrings, "Pizzicato", "Pizz Bounce", { 1, 5 }, with (orchB, { { unison, 3 }, { attack, 0.002f }, { decay, 0.12f }, { sustain, 0 }, { punch, 0.7f }, { delayMix, 0.2f } }));
        R (cStrings, "Cinematic", "Movie Strings Arp", { 2, 4 }, arpOn (with (orchB, { { attack, 0.01f }, { decay, 0.3f }, { sustain, 0.2f } }), 1, 0, 2, 0.5f));
        R (cWoodwind, "Flute", "Trap Flute Lead", { 0, 1, 4, 5 }, with (fluteB, { { wave, 0.3f }, { mono, 1 }, { legato, 1 }, { glide, 0.06f }, { lfoPitch, 0.04f }, { delayMix, 0.2f } }));
        R (cWoodwind, "Ethnic Flute", "Pan Flute Riff", { 2, 4 }, with (fluteB, { { wave, 0.55f }, { attack, 0.03f }, { sustain, 0.8f }, { revMix, 0.4f } }));
        R (cGuitar, "Acoustic", "Dark Acoustic Riff", { 1, 3 }, with (guitB, { { wave, 0.7f }, { decay, 1.6f }, { cutoff, 5000 }, { revMix, 0.25f } }));
        R (cGuitar, "Electric", "Dark Electric Guitar", { 3, 5 }, with (guitB, { { wave, 0.85f }, { body, 0 }, { drive, 0.45f }, { driveType, 1 }, { cutoff, 3500 }, { revMix, 0.3f } }));
        R (cPiano, "Grand", "Trap Piano Keys", { 0, 3, 4 }, with (pianoB, { { cutoff, 6000 }, { revMix, 0.25f } }));
        R (cLead, "Rage", "Anthem Lead", { 4, 5 }, with (leadB, { { unison, 8 }, { detune, 0.55f }, { drive, 0.45f }, { driveType, 3 }, { delayMix, 0.25f }, { mono, 0 } }));
        R (cLead, "Digital", "Whistle Lead", { 0, 1, 5 }, { { engine, FL }, { wave, 0.1f }, { octave, 1 }, { sustain, 0.9f }, { release, 0.3f }, { mono, 1 }, { legato, 1 },
                                                           { glide, 0.08f }, { lfoPitch, 0.05f }, { revMix, 0.3f } });
        R (cPlucks, "Soft", "Plugg Pluck", { 2, 3 }, with (pluckB, { { wave, 0.45f }, { cutoff, 1800 }, { decay, 0.35f }, { revMix, 0.3f }, { delayMix, 0.2f } }));
        R (cPads, "Dark", "Vinyl Dark Pad", { 3 }, with (padB, { { cutoff, 1200 }, { crush, 0.35f }, { wow, 0.4f } }));
        R (cArp, "Melodic", "Trap Bell Arp", { 0, 4 }, arpOn (with (bellB, { { decay, 0.5f }, { delayMix, 0.2f } }), 1, 5, 2, 0.5f));
        R (cArp, "Triplet", "Triplet Flute Arp", { 1, 5 }, arpOn (with (fluteB, { { attack, 0.01f }, { sustain, 0.5f }, { release, 0.2f } }), 2, 5, 2, 0.45f));


        // ---- v0.8 basses: analog-style mono basses instead of 808s
        const Vals moogB { { engine, VA }, { wave, 0.1f }, { filterType, 1 }, { cutoff, 700 }, { reso, 0.35f }, { fenv, 0.65f }, { fdecay, 0.35f },
                           { fsustain, 0.2f }, { attack, 0.002f }, { decay, 0.8f }, { sustain, 0.8f }, { release, 0.15f }, { glide, 0.06f }, { drive, 0.15f } };
        R (cBass, "Synth Bass", "Moog Bass", { 0, 3, 4 }, moogB);
        R (cBass, "Synth Bass", "Round Moog Bass", { 1, 5 }, with (moogB, { { wave, 0.45f }, { cutoff, 500 }, { reso, 0.2f }, { fenv, 0.4f } }));
        R (cBass, "Pluck Bass", "Moog Pluck Bass", { 2, 4 }, with (moogB, { { decay, 0.35f }, { sustain, 0 }, { fdecay, 0.18f }, { fenv, 0.8f }, { cutoff, 450 } }));
        R (cBass, "Distorted", "Growl Moog", { 4, 5 }, with (moogB, { { unison, 2 }, { detune, 0.15f }, { drive, 0.5f }, { driveType, 3 }, { reso, 0.5f },
                                                                      { lfoFilter, 0.2f }, { lfoSync, 1 }, { lfoDiv, 4 } }));
        R (cBass, "Synth Bass", "Acid Moog", { 1, 3 }, with (moogB, { { wave, 0.0f }, { reso, 0.7f }, { fenv, 0.9f }, { fdecay, 0.2f }, { cutoff, 400 }, { glide, 0.12f } }));
        R (cBass, "Synth Bass", "Deep Saw Bass", { 0, 2 }, with (moogB, { { cutoff, 350 }, { reso, 0.1f }, { fenv, 0.3f }, { sub, 0.5f } }));
        R (cBass, "Reese", "Moog Reese", { 3, 5 }, with (moogB, { { unison, 3 }, { detune, 0.35f }, { cutoff, 900 }, { fenv, 0.2f }, { sustain, 1 } }));
        R (cBass, "Distorted", "Fuzz Bass", { 4, 5 }, with (moogB, { { wave, 0.5f }, { drive, 0.65f }, { driveType, 4 }, { cutoff, 1200 } }));
        R (cBass, "Sub", "Square Sub Bass", { 3, 5 }, { { engine, VA }, { wave, 0.5f }, { filterType, 1 }, { cutoff, 600 }, { sustain, 1 }, { release, 0.15f } });
        R (cBass, "Pluck Bass", "Finger Bass", { 0, 3 }, { { engine, PL }, { wave, 0.35f }, { decay, 1.2f }, { sustain, 0 }, { release, 0.15f }, { body, 5 },
                                                          { bodyMix, 0.35f }, { cutoff, 1500 } });
        R (cBass, "Pluck Bass", "Upright Bass", { 1, 2 }, { { engine, PL }, { wave, 0.4f }, { decay, 1.6f }, { sustain, 0 }, { release, 0.2f }, { body, 5 }, { sub, 0.35f },
                                                           { bodyMix, 0.5f }, { cutoff, 1200 } });
        R (cBass, "Hybrid Bass", "FM Bass", { 1, 4 }, { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.45f }, { fdecay, 0.3f }, { decay, 0.6f }, { sustain, 0.5f },
                                                       { cutoff, 3000 } });

        // ---- v0.6: macro 1 is DARK (turn right = darker) on melodic sounds
        for (auto& pr : v)
            if (! pr.isBass())
                for (auto& [k, x] : pr.values) if (k == ID::m1) x = 1.0f - x;

        // ---- v0.8: no 808 sounds at all (decaying sub hits) - the bass section is synth basses
        v.erase (std::remove_if (v.begin(), v.end(), [] (const Preset& pr)
        {
            if (pr.cat == c808 || pr.name.contains ("808")) return true;
            return pr.isBass() && (int) getV (pr.values, ID::engine, 0) == engSub && getV (pr.values, ID::sustain, 1.0f) < 0.05f;
        }), v.end());

        // ---- unique names
        {
            std::map<juce::String, int> seen;
            for (auto& pr : v)
                if (const int nSeen = seen[pr.name]++; nSeen > 0) pr.name << (nSeen == 1 ? " II" : nSeen == 2 ? " III" : " IV");
        }
        // ---- first impression: trap essentials open the library
        {
            static const char* heroes[] { "Classic Trap Bell", "Classic Hard Horn Stab", "Classic Orchestra Brass Hit", "Classic Trap Staccato Violins",
                                          "Classic Trap Flute Lead", "Classic Trap Piano Keys", "Anthem Lead", "Rage Anthem Lead", "Classic Moog Bass",
                                          "Atmos Plugg Pluck", "Classic Trap Bell Arp", "Classic Dark Trap Strings", "Classic Icy Trap Bell",
                                          "Rage Drill Horn Riff", "Classic Epic Stab Hit", "Classic Whistle Lead" };
            std::vector<Preset> front, rest;
            for (auto* h : heroes) for (auto& pr : v) if (pr.name == h) { front.push_back (pr); break; }
            for (auto& pr : v) { bool isHero = false; for (auto& f : front) isHero |= f.name == pr.name; if (! isHero) rest.push_back (pr); }
            v = front; v.insert (v.end(), rest.begin(), rest.end());
        }

        // ---- metadata for the whole library
        for (auto& pr : v) deriveTags (pr);

        // loudness table produced by tests/calibrate.py
        for (auto& pr : v)
            for (const auto& g : presetGains)
                if (pr.name == g.name)
                {
                    bool found = false;
                    for (auto& [k, x] : pr.values) if (k == ID::gain) { x = g.gain; found = true; }
                    if (! found) pr.values.push_back ({ ID::gain, g.gain });
                    break;
                }
        return v;
    }();
    return presets;
}
