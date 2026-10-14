// CHROME VAULT v2 - sound design for the categories "Piano" and "Keys & Organ" (VaultBankGen -recipe keys).
//
// DESIGN NOTE (research summary - what professional keyboard libraries do, and what this recipe copies)
// --------------------------------------------------------------------------------------------------------
// * Acoustic pianos are sold as one instrument heard from several places: a close / player perspective (dry, direct,
//   percussive, for pop and beats), a mid / room perspective and a far / hall perspective ("dreamy", for scoring);
//   ambience is a convolution room / hall / plate on top. Felt pianos (felt strip between hammer and strings) are the
//   intimate, dark, quiet colour of modern scoring and lo-fi - soft attack, no top end, mechanical noise kept, long
//   pedal. Lo-fi / tape variants add saturation, wow & flutter, band-limiting and a smaller stereo image.
//   (Felt-piano and grand-piano libraries: three mic positions + ambience presets + tape / "glow" control; Montclarion-
//   type close / mid / far schemes; "felt & fog" style lo-fi piano with convolution spaces.)
// * Electric pianos: tine pianos live on the stereo (pan) tremolo of the suitcase models, chorus and phaser; reed pianos
//   on amplitude tremolo and the "bark" of a driven preamp; the 80s FM electric piano always gets a chorus. Hip-hop /
//   trap producers darken keys with a low-pass to leave room for the 808 and vocals, add tape saturation, a little
//   vibrato / pitch warp, delay throws and a short dark reverb, sometimes vinyl crackle and flanging.
// * Layering practice (workstation manuals, sampler docs): piano + a pad or strings with a slower attack and longer
//   release, kept low under the piano; piano + EP "hybrid"; EP + vibraphone / bells; octave doubling for melodies.
//   Octave-up colour layers stay at least 12 dB under the main part here (C5 comfort rule).
// * Organs: registration in pitch levels - principal 8' as the base, flutes for a softer, darker colour, 4' added one
//   step quieter for clarity, pedal 16' under it; tremulant = a slow pitch / level wobble; church / cathedral acoustics.
//   Drawbar (tonewheel-style) organs come from the EVOLVE organ engine (additive harmonics), with rotary-style chorus /
//   tremolo and overdrive for gospel and trap chords.
// * Harpsichords: 8' registers (lower / upper, coupled), the buff ("lute") stop, baroque hall acoustics, continuo with
//   strings; in trap the harpsichord is played dark and roomy.
//
// HOW THE BANK IS MADE
// * Every preset is "voice x treatment x space" or a layer recipe. Voices are real instruments of the CHROME VAULT
//   library (5 acoustic pianos, 7 harpsichord registers, 7 organ stops, 3 FM keyboards) or physical models (tine / reed
//   electric pianos, struck-string funk keys, felt toy piano / music box), plus a few EVOLVE-engine organs and pads.
//   Treatments are processing ROLES (felt, dark, lo-fi, tape, muted, swell, tremolo, chorus, drive, ...), never jitter.
// * Each part is calibrated alone (its `gain` puts it at -20 dB K-weighted at C5) so layer levels mean the same thing.
// * Each candidate goes through the shared v2 gate (RecipeGate.h, C5 = MIDI 60) with automatic repairs (darker,
//   more velocity, shorter tail, ...) and then through TWO distinctness checks against everything accepted before it:
//   the shared log-mel fingerprint (>= sharedMin) and this file's own MFCC fingerprint (>= ownMin) made of C5 at
//   velocity 0.8 (7 time segments incl. the release tail) + C5 at velocity 0.35 + C4 + C6 + stereo width.
//   Near-duplicates are dropped, never renamed. Names: evocative English, no brands, no "808".
//
// Environment (optional): KEYS_THREADS (default: all cores), KEYS_LIMIT (first N candidates, smoke test),
// KEYS_REPORT (path of a TSV measurement report), KEYS_OWN_MIN / KEYS_SHARED_MIN (distance thresholds).
#include "Recipe.h"
#include "RecipeGate.h"
#include <atomic>
#include <map>
#include <mutex>
#include <set>
#include <thread>

namespace
{
using Values = std::vector<std::pair<juce::String, float>>;

//==============================================================================
// part makers
VaultPart S (const char* instrument, const char* articulation, const char* label, Values v = {})
{
    auto p = recipe::samplePart (instrument, articulation, label);
    p.values = std::move (v);
    return p;
}
VaultPart M (const char* model, const char* label, Values v = {})
{
    auto p = recipe::modelPart (model, label);
    p.values = std::move (v);
    return p;
}
VaultPart Y (const char* label, Values v)
{
    auto p = recipe::synthPart (label);
    p.values = std::move (v);
    return p;
}
float getv (const VaultPart& p, const char* k, float def) { return p.get (k, def); }
void setv (VaultPart& p, const char* k, float v) { p.set (k, v); }
void setAll (VaultPart& p, const Values& v) { for (auto& [k, x] : v) p.set (k, x); }

// brightness in Hz, whatever the part kind (sample: s.cutoff; model: m.tone; synth: cutoff)
void lowPass (VaultPart& p, float hz)
{
    if (p.kind == VaultPartKind::sample) setv (p, "s.cutoff", juce::jlimit (300.0f, 20000.0f, std::min (hz, getv (p, "s.cutoff", 20000.0f))));
    else if (p.kind == VaultPartKind::model)
    {
        const float t = juce::jlimit (0.0f, 1.0f, 0.5f + 0.5f * std::log2 (hz / 6000.0f));
        setv (p, "m.tone", std::min (t, getv (p, "m.tone", 0.5f)));
    }
    else setv (p, "cutoff", juce::jlimit (200.0f, 20000.0f, std::min (hz, getv (p, "cutoff", 12000.0f))));
}
float currentLp (const VaultPart& p)
{
    if (p.kind == VaultPartKind::sample) return getv (p, "s.cutoff", 20000.0f);
    if (p.kind == VaultPartKind::model) return 6000.0f * std::exp2 ((getv (p, "m.tone", 0.5f) - 0.5f) * 2.0f);
    return getv (p, "cutoff", 12000.0f);
}
void velSens (VaultPart& p, float v)
{
    if (p.kind == VaultPartKind::sample) setv (p, "s.velSens", v);
    else if (p.kind == VaultPartKind::model) setv (p, "m.velSens", v);
    else setv (p, "velSens", v);
}
float getVelSens (const VaultPart& p)
{
    if (p.kind == VaultPartKind::sample) return getv (p, "s.velSens", 0.5f);
    if (p.kind == VaultPartKind::model) return getv (p, "m.velSens", 0.75f);
    return getv (p, "velSens", 0.6f);
}
void attack (VaultPart& p, float s)
{
    if (p.kind == VaultPartKind::sample) setv (p, "s.attack", s);
    else if (p.kind == VaultPartKind::synth) setv (p, "attack", s);
}
void release (VaultPart& p, float s)
{
    if (p.kind == VaultPartKind::sample) setv (p, "s.release", s);
    else if (p.kind == VaultPartKind::model) setv (p, "m.release", juce::jlimit (0.02f, 4.0f, s));
    else setv (p, "release", juce::jlimit (0.005f, 8.0f, s));
}

//==============================================================================
// spaces (FX of part 0: the engine reads the master FX from part 0 only)
struct Space { const char* tag; const char* word; Values fx; };
const Space& space (const char* tag)
{
    static const std::vector<Space> s {
        { "dry",     "Close",  { { "revMix", 0.05f }, { "revType", 1 }, { "revSize", 0.18f } } },
        { "room",    "Room",   { { "revMix", 0.13f }, { "revType", 1 }, { "revSize", 0.32f } } },
        { "studio",  "Studio", { { "revMix", 0.15f }, { "revType", 0 }, { "revSize", 0.38f } } },
        { "hall",    "Hall",   { { "revMix", 0.23f }, { "revType", 0 }, { "revSize", 0.62f } } },
        { "stage",   "Stage",  { { "revMix", 0.30f }, { "revType", 0 }, { "revSize", 0.80f } } },
        { "plate",   "Plate",  { { "revMix", 0.21f }, { "revType", 1 }, { "revSize", 0.56f } } },
        { "cloud",   "Cloud",  { { "revMix", 0.34f }, { "revType", 2 }, { "revSize", 0.62f } } },
        { "church",  "Church", { { "revMix", 0.32f }, { "revType", 0 }, { "revSize", 0.88f } } },
        { "echo",    "Echo",   { { "delayMix", 0.16f }, { "delayTime", 2 }, { "delayFb", 0.32f }, { "delayMode", 2 }, { "revMix", 0.12f }, { "revType", 1 }, { "revSize", 0.4f } } },
        { "ping",    "Delay",  { { "delayMix", 0.18f }, { "delayTime", 0 }, { "delayFb", 0.28f }, { "delayMode", 0 }, { "revMix", 0.14f }, { "revType", 0 }, { "revSize", 0.5f } } } };
    for (auto& x : s) if (juce::String (x.tag) == tag) return x;
    jassertfalse;
    return s.front();
}

//==============================================================================
// voices: the main instrument of a preset (part 0)
enum Kind { kGrand, kUpright, kDigital, kTine, kReed, kFmEp, kToy, kPipe, kChamberOrgan, kDrawbar, kReedOrgan, kHarpsi, kClav, kFmKeys, kMusicBox, numKinds };
struct Voice
{
    const char* tag;
    int category;
    Kind kind;
    VaultPart part;
    juce::StringArray nouns;      // name nouns, best first
};

std::vector<Voice> voices()
{
    using namespace vault;
    std::vector<Voice> v;
    auto add = [&] (const char* tag, int cat, Kind k, VaultPart p, juce::StringArray nouns) { v.push_back ({ tag, cat, k, std::move (p), std::move (nouns) }); };
    // ---- Piano: acoustic grands and uprights (sampled), digital, electric (modelled tine / reed + sampled FM), toy
    add ("concert", catPiano, kGrand, S ("piano.grand.concert", "sustain", "Concert Grand", { { "s.velCutoff", 0.25f } }), { "Concert Grand", "Grand", "Grand Piano" });
    add ("pedal", catPiano, kGrand, S ("piano.grand.concert", "pedal", "Concert Grand", { { "s.velCutoff", 0.25f } }), { "Pedal Grand", "Sustain Grand", "Open Grand" });
    add ("studio", catPiano, kGrand, S ("piano.grand.studio", "sustain", "Studio Grand", { { "s.cutoff", 5200.0f }, { "s.velCutoff", 0.3f } }), { "Studio Grand", "Session Grand", "Pop Grand" });
    add ("chamber", catPiano, kGrand, S ("piano.grand.chamber", "sustain", "Chamber Grand", { { "s.velCutoff", 0.25f } }), { "Chamber Grand", "Salon Grand", "Small Grand" });
    add ("upright", catPiano, kUpright, S ("piano.upright", "sustain", "Upright Piano", { { "s.cutoff", 3200.0f }, { "s.velCutoff", 0.3f } }), { "Upright", "House Upright", "Old Upright" });
    add ("parlor", catPiano, kUpright, S ("piano.upright.parlor", "sustain", "Parlor Upright", { { "s.velCutoff", 0.3f } }), { "Parlor Upright", "Parlor Piano", "Front Room Piano" });
    add ("digital", catPiano, kDigital, S ("keys.fm.piano", "sustain", "FM Digital Piano", { { "s.velSens", 0.6f } }), { "Digital Piano", "Digi Grand", "Nineties Piano" });
    add ("tine", catPiano, kTine, M ("ep.tine.mellow", "Tine Electric Piano"), { "Tine Piano", "Tine Keys", "Suitcase Tines" });
    add ("tinebell", catPiano, kTine, M ("ep.tine.bell", "Bell Tine Piano"), { "Bell Tines", "Glass Tines", "Bright Tines" });
    add ("reed", catPiano, kReed, M ("ep.reed", "Reed Electric Piano"), { "Reed Piano", "Reed Keys", "Classroom Reeds" });
    add ("fmep", catPiano, kFmEp, S ("keys.fm.epiano", "sustain", "FM Electric Piano", { { "s.velSens", 0.6f } }), { "FM Piano", "Digital EP", "Eighties EP" });
    add ("toy", catPiano, kToy, M ("keys.toy", "Felt Toy Piano"), { "Toy Piano", "Little Piano", "Nursery Keys" });
    // ---- Keys & Organ
    add ("principal", catKeysOrgan, kPipe, S ("organ.pipe", "principal", "Pipe Organ", { { "s.velSens", 0.6f } }), { "Principal Organ", "Pipe Organ", "Great Organ" });
    add ("pipesoft", catKeysOrgan, kPipe, S ("organ.pipe", "soft", "Pipe Organ", { { "s.velSens", 0.6f } }), { "Soft Flute Organ", "Quiet Pipes", "Gedackt Organ" });
    add ("pedalorgan", catKeysOrgan, kPipe, S ("organ.pipe", "pedal", "Pipe Organ", { { "s.velSens", 0.6f } }), { "Pedal Organ", "Bourdon Organ", "Deep Pipes" });
    add ("pedalsoft", catKeysOrgan, kPipe, S ("organ.pipe", "pedal soft", "Pipe Organ", { { "s.velSens", 0.6f } }), { "Soft Pedal Organ", "Hushed Pipes", "Low Flue Organ" });
    add ("chamber8", catKeysOrgan, kChamberOrgan, S ("organ.chamber", "8 foot", "Chamber Organ", { { "s.velSens", 0.6f } }), { "Chamber Organ", "Positive Organ", "Box Organ" });
    add ("chamber4", catKeysOrgan, kChamberOrgan, S ("organ.chamber", "4 foot", "Chamber Organ", { { "s.velSens", 0.6f } }), { "Four Foot Organ", "Octave Flute Organ", "Little Organ" });
    add ("chamberfull", catKeysOrgan, kChamberOrgan, S ("organ.chamber", "full", "Chamber Organ", { { "s.velSens", 0.6f } }), { "Full Chamber Organ", "Plenum Organ", "Chapel Organ" });
    add ("drawsoft", catKeysOrgan, kDrawbar, Y ("Drawbar Organ", { { "engine", 4 }, { "wave", 0.2f }, { "attack", 0.004f }, { "sustain", 1 }, { "release", 0.12f }, { "cutoff", 6000 }, { "velSens", 0.55f } }),
         { "Soft Drawbars", "Mellow Drawbars", "Ballad Organ" });
    add ("drawfull", catKeysOrgan, kDrawbar, Y ("Drawbar Organ", { { "engine", 4 }, { "wave", 0.55f }, { "attack", 0.003f }, { "sustain", 1 }, { "release", 0.1f }, { "cutoff", 4500 }, { "velSens", 0.55f },
                                                                    { "layerB", 1 }, { "engineB", 4 }, { "waveB", 0.3f }, { "octaveB", 1 }, { "levelB", 0.2f } }),
         { "Full Drawbars", "Gospel Organ", "Sunday Organ" });
    add ("drawperc", catKeysOrgan, kDrawbar, Y ("Drawbar Organ", { { "engine", 4 }, { "wave", 0.4f }, { "attack", 0.002f }, { "decay", 0.45f }, { "sustain", 0.72f }, { "release", 0.1f }, { "cutoff", 4200 },
                                                                    { "velSens", 0.6f } }),
         { "Percussive Organ", "Click Organ", "Jazz Organ" });
    add ("harmonium", catKeysOrgan, kReedOrgan, Y ("Reed Organ", { { "engine", 0 }, { "wave", 0.35f }, { "unison", 2 }, { "detune", 0.06f }, { "cutoff", 1500 }, { "keyTrack", 0.6f },
                                                                  { "attack", 0.06f }, { "sustain", 1 }, { "release", 0.18f }, { "velSens", 0.6f } }),
         { "Harmonium", "Pump Organ", "Parlor Reeds" });
    add ("hpsEnglish", catKeysOrgan, kHarpsi, S ("keys.harpsichord.english", "normal", "English Harpsichord", { { "s.cutoff", 6500.0f }, { "s.velSens", 0.6f } }), { "English Harpsichord", "Spinet", "Virginal" });
    add ("hpsLute", catKeysOrgan, kHarpsi, S ("keys.harpsichord.english", "lute", "English Harpsichord", { { "s.velSens", 0.6f } }), { "Lute Stop", "Buff Harpsichord", "Lute Harpsichord" });
    add ("hpsFlemLo", catKeysOrgan, kHarpsi, S ("keys.harpsichord.flemish", "lower 8'", "Flemish Harpsichord", { { "s.cutoff", 6500.0f }, { "s.velSens", 0.6f } }), { "Flemish Harpsichord", "Lower Manual", "Flemish Lower" });
    add ("hpsFlemUp", catKeysOrgan, kHarpsi, S ("keys.harpsichord.flemish", "upper 8'", "Flemish Harpsichord", { { "s.cutoff", 6000.0f }, { "s.velSens", 0.6f } }), { "Upper Manual", "Flemish Upper", "Bright Manual" });
    add ("hpsFrench", catKeysOrgan, kHarpsi, S ("keys.harpsichord.french", "normal", "French Harpsichord", { { "s.cutoff", 6000.0f }, { "s.velSens", 0.6f } }), { "French Harpsichord", "Court Harpsichord", "Rococo Keys" });
    add ("hpsItalian", catKeysOrgan, kHarpsi, S ("keys.harpsichord.italian", "normal", "Italian Harpsichord", { { "s.cutoff", 6000.0f }, { "s.velSens", 0.6f } }), { "Italian Harpsichord", "Cypress Harpsichord", "Venetian Keys" });
    add ("hpsBaroque", catKeysOrgan, kHarpsi, S ("keys.harpsichord.baroque", "normal", "Baroque Harpsichord", { { "s.cutoff", 5500.0f }, { "s.velSens", 0.6f } }), { "Baroque Harpsichord", "Continuo", "Fugue Keys" });
    add ("clav", catKeysOrgan, kClav, M ("ep.clav", "Funk Keys"), { "Funk Keys", "Clavi Keys", "Strut Keys" });
    add ("fmclav", catKeysOrgan, kClav, S ("keys.fm.clav", "sustain", "FM Clav", { { "s.cutoff", 5000.0f }, { "s.velSens", 0.6f } }), { "FM Clav", "Digital Clav", "Chip Clav" });
    add ("fmkeys", catKeysOrgan, kFmKeys, Y ("FM Keys", { { "engine", 1 }, { "fmAlgo", 5 }, { "fmRatio", 1 }, { "fmRatio2", 14 }, { "fmAmt", 0.18f }, { "wave", 0.0f }, { "attack", 0.002f },
                                                          { "decay", 2.2f }, { "sustain", 0.0f }, { "release", 0.5f }, { "cutoff", 5000 }, { "velSens", 0.7f } }),
         { "FM Keys", "Chrome Keys", "Glass Keys" });
    add ("fmbell", catKeysOrgan, kFmKeys, Y ("FM Bell Keys", { { "engine", 1 }, { "fmAlgo", 0 }, { "fmRatio", 2 }, { "fmAmt", 0.22f }, { "attack", 0.002f }, { "decay", 1.6f }, { "sustain", 0.0f },
                                                               { "release", 0.6f }, { "cutoff", 4000 }, { "velSens", 0.7f } }),
         { "Soft FM Keys", "Ivory FM", "Pastel Keys" });
    add ("clavneck", catKeysOrgan, kClav, M ("ep.clav", "Funk Keys", { { "m.pickup", 0.0f }, { "m.tone", 0.4f } }), { "Neck Clav", "Soft Funk Keys", "Mellow Clavi" });
    add ("clavbridge", catKeysOrgan, kClav, M ("ep.clav", "Funk Keys", { { "m.pickup", 1.0f }, { "m.tone", 0.55f }, { "m.decay", 0.7f } }), { "Bridge Clav", "Snap Keys", "Twang Keys" });
    add ("draw16", catKeysOrgan, kDrawbar, Y ("Drawbar Organ", { { "engine", 4 }, { "wave", 0.3f }, { "attack", 0.004f }, { "sustain", 1 }, { "release", 0.12f }, { "cutoff", 3500 }, { "velSens", 0.55f },
                                                                  { "layerB", 1 }, { "engineB", 4 }, { "waveB", 0.1f }, { "octaveB", -1 }, { "levelB", 0.45f } }),
         { "Deep Drawbars", "Sixteen Foot Organ", "Basement Organ" });
    add ("combo", catKeysOrgan, kDrawbar, Y ("Combo Organ", { { "engine", 0 }, { "wave", 0.5f }, { "cutoff", 1400 }, { "keyTrack", 0.7f }, { "attack", 0.003f }, { "sustain", 1 }, { "release", 0.08f },
                                                              { "lfoPitch", 0.02f }, { "lfoRate", 6.5f }, { "velSens", 0.55f } }),
         { "Combo Organ", "Garage Organ", "Transistor Organ" });
    add ("calliope", catKeysOrgan, kReedOrgan, Y ("Calliope", { { "engine", 5 }, { "wave", 0.12f }, { "cutoff", 3000 }, { "attack", 0.03f }, { "sustain", 1 }, { "release", 0.15f },
                                                              { "lfoPitch", 0.025f }, { "lfoRate", 5.8f }, { "velSens", 0.6f } }),
         { "Calliope", "Fairground Pipes", "Carousel Organ" });
    add ("musette", catKeysOrgan, kReedOrgan, Y ("Accordion", { { "engine", 0 }, { "wave", 0.3f }, { "unison", 2 }, { "detune", 0.1f }, { "cutoff", 1700 }, { "keyTrack", 0.6f },
                                                               { "attack", 0.04f }, { "sustain", 1 }, { "release", 0.12f }, { "lfoAmp", 0.06f }, { "lfoRate", 3.5f }, { "velSens", 0.6f } }),
         { "Accordion", "Musette", "Bellows Keys" });
    add ("musicbox", catKeysOrgan, kMusicBox, M ("keys.toy", "Music Box", { { "m.tone", 0.68f }, { "m.bell", 1.6f }, { "m.decay", 0.55f }, { "m.release", 1.2f } }),
         { "Music Box", "Wind-Up Box", "Jewel Box" });
    add ("celeste", catKeysOrgan, kMusicBox, M ("keys.toy", "Celeste Keys", { { "m.tone", 0.6f }, { "m.bell", 1.25f }, { "m.decay", 1.4f }, { "m.release", 0.8f } }),
         { "Celeste Keys", "Sugar Keys", "Star Keys" });
    return v;
}

//==============================================================================
// treatments: processing roles
struct Treat
{
    const char* tag;
    juce::StringArray adjectives;
    std::vector<Kind> kinds;                  // the voice kinds it suits
    std::vector<const char*> spaces;          // the spaces it is made with (first = best)
    std::function<void (VaultPreset&)> apply;
};
bool suits (const Treat& t, Kind k) { return std::find (t.kinds.begin(), t.kinds.end(), k) != t.kinds.end(); }

void fx (VaultPreset& p, const Values& v) { setAll (p.parts.front(), v); }
// a synth part 0 starts from the EVOLVE FX defaults (reverb, MASTER air, hidden macro reverb / delay): neutral first
void neutralFx (VaultPart& p)
{
    if (p.kind != VaultPartKind::synth) return;   // sample / model part 0: the FX defaults are neutral already
    setAll (p, { { "revMix", 0.0f }, { "master", 0.0f }, { "macro2", 0.0f }, { "macro5", 0.0f }, { "delayMix", 0.0f } });
}

std::vector<Treat> treatments()
{
    const std::vector<Kind> acoustic { kGrand, kUpright }, pianos { kGrand, kUpright, kDigital }, eps { kTine, kReed, kFmEp },
                            organs { kPipe, kChamberOrgan }, allOrgans { kPipe, kChamberOrgan, kDrawbar, kReedOrgan };
    auto plus = [] (std::vector<Kind> a, std::vector<Kind> b) { a.insert (a.end(), b.begin(), b.end()); return a; };
    std::vector<Treat> t;
    // natural: the instrument as recorded / modelled, heard from different places
    t.push_back ({ "natural", { "", "Classic", "Open" }, plus (plus (pianos, eps), plus (allOrgans, { kHarpsi, kClav, kFmKeys, kMusicBox, kToy })),
                   { "dry", "hall", "room", "stage", "cloud", "plate", "church", "studio" }, [] (VaultPreset&) {} });
    // felt: hammer felt / dark intimate
    t.push_back ({ "felt", { "Felt", "Velvet", "Hushed", "Muffled" }, plus (acoustic, { kTine, kToy, kHarpsi }), { "room", "cloud", "hall" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        lowPass (a, a.kind == VaultPartKind::model ? 1600.0f : 1250.0f);
        if (a.kind == VaultPartKind::sample) { setv (a, "s.velCutoff", 0.65f); setv (a, "s.velSens", 0.7f); setv (a, "s.release", 0.9f); setv (a, "s.start", 6.0f); }
        else setv (a, "m.bell", 0.4f);
        fx (p, { { "eqLow", 1.5f }, { "eqHigh", -2.0f } });
    } });
    // dark: low-passed, low end lifted - the trap "make room for vocals" piano
    t.push_back ({ "dark", { "Dark", "Midnight", "Shadow", "Nocturne" }, plus (plus (pianos, eps), plus (organs, { kHarpsi, kDrawbar, kClav, kFmKeys })), { "hall", "room", "cloud" }, [] (VaultPreset& p)
    {
        lowPass (p.parts.front(), 2000.0f);
        fx (p, { { "eqLow", 2.5f }, { "eqHigh", -3.0f } });
    } });
    // intimate: very velocity sensitive, softer top at low velocity
    t.push_back ({ "intimate", { "Intimate", "Whisper", "Tender", "Quiet" }, plus (acoustic, { kTine, kHarpsi }), { "dry", "room" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        velSens (a, 0.95f);
        if (a.kind == VaultPartKind::sample) setv (a, "s.velCutoff", 0.85f);
        else setv (a, "m.tone", 0.42f);
    } });
    // lo-fi: band-limited, crushed, wobbly, narrow
    t.push_back ({ "lofi", { "Lo-Fi", "Dusty", "Bedroom", "Crumbled" }, plus (plus (pianos, eps), { kToy, kHarpsi, kPipe, kChamberOrgan, kDrawbar, kFmKeys, kClav, kMusicBox }), { "room", "echo" }, [] (VaultPreset& p)
    {
        lowPass (p.parts.front(), 3600.0f);
        fx (p, { { "crush", 0.22f }, { "wow", 0.32f }, { "drive", 0.18f }, { "driveType", 1 }, { "width", 0.32f }, { "eqLow", 1.5f } });
    } });
    // tape: saturation, flutter, softened top
    t.push_back ({ "tape", { "Tape", "Cassette", "Reel", "Warped" }, plus (plus (pianos, eps), { kHarpsi, kDrawbar, kChamberOrgan, kClav, kReedOrgan }), { "room", "plate", "echo" }, [] (VaultPreset& p)
    {
        lowPass (p.parts.front(), 5200.0f);
        fx (p, { { "wow", 0.16f }, { "drive", 0.28f }, { "driveType", 1 }, { "eqHigh", -2.5f }, { "eqLow", 1.0f } });
    } });
    // vinyl: heavy wow + crackle, very dark
    t.push_back ({ "vinyl", { "Vinyl", "Crackle", "Dust", "Record" }, plus (pianos, { kTine, kReed, kHarpsi, kPipe, kDrawbar }), { "room", "hall" }, [] (VaultPreset& p)
    {
        lowPass (p.parts.front(), 2600.0f);
        fx (p, { { "wow", 0.62f }, { "crush", 0.08f }, { "drive", 0.12f }, { "driveType", 1 }, { "width", 0.4f } });
    } });
    // muted: palm-muted / staccato
    t.push_back ({ "muted", { "Muted", "Plucked", "Damped", "Staccato" }, plus (pianos, { kTine, kReed, kFmEp, kHarpsi, kClav, kToy }), { "room", "echo", "dry" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::sample) { setv (a, "s.decay", 0.32f); setv (a, "s.sustain", 0.0f); setv (a, "s.release", 0.14f); }
        else if (a.source == "ep.clav") setv (a, "m.artic", 1.0f);
        else { setv (a, "m.decay", 0.22f); setv (a, "m.release", 0.12f); }
        lowPass (a, 4000.0f);
    } });
    // swell: soft attack, long release - piano as a pad
    t.push_back ({ "swell", { "Swelling", "Bowed", "Rising", "Breathing" }, plus (plus (pianos, eps), plus (organs, { kHarpsi, kMusicBox, kToy })), { "cloud", "hall" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        attack (a, a.kind == VaultPartKind::sample ? 0.42f : 0.3f);
        if (a.kind == VaultPartKind::model) { setv (a, "m.release", 1.8f); setv (a, "m.bell", 0.5f); }
        else release (a, 1.6f);
        lowPass (a, 3000.0f);
    } });
    // bright: pop / stage presence (still within the comfort limits)
    t.push_back ({ "bright", { "Bright", "Pop", "Crisp", "Stage" }, { kGrand, kDigital, kTine, kReed, kHarpsi, kDrawbar }, { "dry", "plate" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::sample) { setv (a, "s.cutoff", 20000.0f); setv (a, "s.velCutoff", 0.0f); }
        else if (a.kind == VaultPartKind::model) { setv (a, "m.tone", 0.68f); setv (a, "m.voicing", 0.4f); }
        fx (p, { { "eqHigh", 1.5f }, { "master", 0.35f } });
    } });
    // chorus: classic EP chorus, wide
    t.push_back ({ "chorus", { "Chorus", "Silk", "Shimmer", "Liquid" }, plus (eps, { kDigital, kDrawbar, kFmKeys, kClav, kUpright }), { "room", "hall" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::model) { setv (a, "m.tone", 0.58f); setv (a, "m.bell", 1.4f); }
        else lowPass (a, 4500.0f);
        fx (p, { { "chorus", 0.42f }, { "width", 0.75f }, { "eqLow", -1.0f } });
    } });
    // phaser
    t.push_back ({ "phaser", { "Phased", "Swirl", "Orbit", "Spiral" }, { kTine, kReed, kClav, kDrawbar, kFmEp }, { "room", "echo" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::model && a.source != "ep.clav") { setv (a, "m.voicing", 0.35f); setv (a, "m.drive", 0.35f); }
        lowPass (a, 3800.0f);
        fx (p, { { "phaser", 0.45f }, { "drive", 0.12f }, { "driveType", 1 } });
    } });
    // drive: amp bark / overdriven organ
    t.push_back ({ "drive", { "Driven", "Barking", "Growl", "Hot" }, { kTine, kReed, kClav, kDrawbar, kReedOrgan }, { "room", "dry" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::model && a.source != "ep.clav") { setv (a, "m.drive", 0.7f); setv (a, "m.voicing", 0.6f); }
        else if (a.kind == VaultPartKind::model) setv (a, "m.drive", 0.6f);
        fx (p, { { "drive", 0.32f }, { "driveType", 0 }, { "eqHigh", -2.0f } });
        lowPass (a, 4200.0f);
    } });
    // tremolo: suitcase pan tremolo (tines), level tremolo (reeds, toy), rotary-ish on drawbars
    t.push_back ({ "tremolo", { "Tremolo", "Suitcase", "Pulse", "Wobble" }, { kTine, kReed, kDrawbar, kToy }, { "room", "hall" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::model) { setv (a, "m.trem", 0.6f); setv (a, "m.tremRate", 4.8f); setv (a, "m.voicing", -0.5f); setv (a, "m.tone", 0.42f); }
        else { setv (a, "lfoAmp", 0.22f); setv (a, "lfoRate", 6.2f); fx (p, { { "chorus", 0.35f } }); lowPass (a, 3000.0f); }
        fx (p, { { "eqLow", 1.5f } });
    } });
    // tremulant: organ stop - slow pitch + level wobble
    t.push_back ({ "tremulant", { "Tremulant", "Trembling", "Wavering", "Quivering" }, { kPipe, kChamberOrgan, kReedOrgan }, { "church", "hall" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::sample) { setv (a, "s.vibrato", 9.0f); setv (a, "s.vibRate", 6.3f); }
        else { setv (a, "lfoPitch", 0.03f); setv (a, "lfoAmp", 0.12f); setv (a, "lfoRate", 6.3f); }
        lowPass (a, 3000.0f);
        fx (p, { { "chorus", 0.18f }, { "eqLow", 1.5f } });
    } });
    // rotary: chorus + vibrato (slow / fast)
    t.push_back ({ "rotary", { "Rotary", "Spinning", "Whirl", "Cabinet" }, { kDrawbar, kChamberOrgan, kPipe }, { "room", "dry" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::sample) { setv (a, "s.vibrato", 6.0f); setv (a, "s.vibRate", 6.6f); }
        else { setv (a, "lfoPitch", 0.02f); setv (a, "lfoAmp", 0.16f); setv (a, "lfoRate", 6.8f); }
        lowPass (a, 3800.0f);
        fx (p, { { "chorus", 0.55f }, { "drive", 0.22f }, { "driveType", 1 }, { "width", 0.7f } });
    } });
    // bell-forward EP voicing
    t.push_back ({ "bell", { "Bell", "Sparkle", "Glassy", "Crystal" }, { kTine, kToy, kMusicBox }, { "plate", "hall" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        setv (a, "m.bell", 1.8f);
        setv (a, "m.voicing", 0.5f);
    } });
    // round: rounder, warmer pickup voicing
    t.push_back ({ "round", { "Round", "Warm", "Soft", "Mellow" }, { kTine, kReed, kClav, kFmEp, kDigital, kFmKeys, kDrawbar }, { "room", "plate" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::model) { setv (a, "m.voicing", -0.8f); setv (a, "m.tone", 0.38f); }
        else lowPass (a, 2600.0f);
        fx (p, { { "eqLow", 1.5f } });
    } });
    // distant: the far microphones - darker, softer onset, mostly room
    t.push_back ({ "distant", { "Distant", "Faraway", "Far", "Remote" }, plus (pianos, { kTine, kHarpsi, kPipe, kChamberOrgan, kToy, kMusicBox }), { "stage" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        lowPass (a, 2300.0f);
        attack (a, 0.015f);
        fx (p, { { "revMix", 0.5f }, { "revSize", 0.78f }, { "revType", 0 }, { "eqLow", -1.5f }, { "width", 0.7f } });
    } });
    // underwater: very low-passed, chorused, drifting
    t.push_back ({ "underwater", { "Underwater", "Submerged", "Sunken", "Deep Sea" }, plus (pianos, { kTine, kReed, kFmEp, kMusicBox, kToy, kDrawbar, kHarpsi }), { "cloud" }, [] (VaultPreset& p)
    {
        lowPass (p.parts.front(), 650.0f);
        fx (p, { { "chorus", 0.5f }, { "wow", 0.18f }, { "eqLow", 1.0f }, { "width", 0.7f } });
    } });
    // wooden: a small wooden box resonating with the strings / tines
    t.push_back ({ "wooden", { "Wooden", "Cabinet", "Timber", "Box" }, { kGrand, kUpright, kToy, kMusicBox, kHarpsi, kClav, kTine }, { "room" }, [] (VaultPreset& p)
    {
        fx (p, { { "body", 5 }, { "bodyMix", 0.38f } });
        lowPass (p.parts.front(), 5000.0f);
    } });
    // echo pluck: short notes thrown into a ping-pong delay
    t.push_back ({ "echopluck", { "Echo", "Bouncing", "Ricochet", "Ping" }, plus (pianos, { kTine, kReed, kFmEp, kClav, kHarpsi, kMusicBox, kFmKeys }), { "ping" }, [] (VaultPreset& p)
    {
        auto& a = p.parts.front();
        if (a.kind == VaultPartKind::sample) { setv (a, "s.decay", 0.22f); setv (a, "s.sustain", 0.0f); setv (a, "s.release", 0.12f); }
        else if (a.kind == VaultPartKind::model) { if (a.source == "ep.clav") setv (a, "m.artic", 1.0f); else { setv (a, "m.decay", 0.25f); setv (a, "m.release", 0.15f); } }
        else { setv (a, "decay", 0.25f); setv (a, "sustain", 0.0f); setv (a, "release", 0.15f); }
        lowPass (a, 3500.0f);
        fx (p, { { "delayMix", 0.3f }, { "delayFb", 0.42f }, { "delayTime", 1 } });
    } });
    // reverse grains: 1/8-note reversed fragments mixed in (a backwards-swelling piano)
    t.push_back ({ "reverse", { "Reversed", "Backwards", "Rewind", "Mirror" }, plus (pianos, { kTine, kFmEp, kMusicBox, kHarpsi }), { "hall" }, [] (VaultPreset& p)
    {
        lowPass (p.parts.front(), 3500.0f);
        fx (p, { { "reverse", 0.55f } });
    } });
    return t;
}

//==============================================================================
// layer recipes: what a second / third / fourth part adds. `level` relative to part 0 after both are calibrated.
VaultPart strings (const char* inst, const char* label, float att = 0.35f)
{
    return S (inst, "sustain", label, { { "s.attack", att }, { "s.release", 1.1f }, { "s.cutoff", 3200.0f }, { "s.velSens", 0.35f } });
}
VaultPart warmPad()
{
    return Y ("Warm Pad", { { "engine", 0 }, { "wave", 0.15f }, { "unison", 4 }, { "detune", 0.14f }, { "cutoff", 1100 }, { "keyTrack", 0.5f }, { "attack", 0.55f },
                            { "decay", 1.0f }, { "sustain", 1 }, { "release", 1.8f }, { "lfoFilter", 0.08f }, { "lfoRate", 0.25f }, { "velSens", 0.3f } });
}
VaultPart airPad()
{
    return Y ("Air Pad", { { "engine", 7 }, { "wave", 0.25f }, { "unison", 5 }, { "detune", 0.2f }, { "cutoff", 1800 }, { "keyTrack", 0.4f }, { "attack", 0.9f },
                           { "decay", 1.0f }, { "sustain", 1 }, { "release", 2.4f }, { "lfoFilter", 0.12f }, { "lfoRate", 0.18f }, { "velSens", 0.25f } });
}
VaultPart vocalPad() { return M ("voice.pad", "Vocal Pad", { { "m.velSens", 0.3f } }); }
VaultPart choir() { return S ("voice.choir.classic", "", "Classic Choir", { { "s.attack", 0.3f }, { "s.release", 1.2f }, { "s.cutoff", 3000.0f }, { "s.velSens", 0.3f } }); }
VaultPart bowedVibes() { return S ("mallets.vibraphone", "bowed", "Bowed Vibraphone", { { "s.attack", 0.25f }, { "s.release", 1.4f }, { "s.velSens", 0.4f } }); }
VaultPart vibes() { return S ("mallets.vibraphone", "soft mallets", "Vibraphone", { { "s.velSens", 0.6f } }); }
VaultPart chimes() { return S ("bells.handchimes", "hit", "Hand Chimes", { { "s.velSens", 0.6f }, { "s.cutoff", 5000.0f } }); }
VaultPart glock() { auto p = S ("mallets.glockenspiel", "hit", "Glockenspiel", { { "s.velSens", 0.6f }, { "s.cutoff", 6000.0f } }); p.octave = 2; return p; }
VaultPart flute() { return S ("winds.flute.concert", "sustain", "Concert Flute", { { "s.attack", 0.08f }, { "s.cutoff", 3500.0f }, { "s.velSens", 0.4f } }); }
VaultPart tineLayer() { return M ("ep.tine.mellow", "Tine Electric Piano"); }
VaultPart fmEpLayer() { return S ("keys.fm.epiano", "sustain", "FM Electric Piano", { { "s.velSens", 0.6f } }); }

struct Layer
{
    const char* tag;
    juce::StringArray words;            // name words (prefix or noun) for the layered preset
    std::vector<Kind> kinds;            // main voice kinds it suits
    const char* spaceTag;
    std::function<void (VaultPreset&)> add;
};
std::vector<Layer> layers()
{
    const std::vector<Kind> grands { kGrand, kUpright }, eps { kTine, kReed, kFmEp };
    std::vector<Layer> L;
    auto partAt = [] (VaultPart p, float level, float pan = 0) { p.level = level; p.pan = pan; return p; };
    L.push_back ({ "strings", { "& Strings", "Silver String", "Bowed String" }, { kGrand, kUpright, kTine, kHarpsi, kPipe, kChamberOrgan, kDigital }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (strings ("strings.violins.ensemble", "Violin Section"), 0.42f, 0.15f)); } });
    L.push_back ({ "cellos", { "& Cellos", "Low String", "Cello" }, { kGrand, kUpright, kHarpsi, kPipe, kTine }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (strings ("strings.cellos.ensemble", "Cello Section", 0.25f), 0.45f, -0.12f)); } });
    L.push_back ({ "pad", { "& Pad", "Dream", "Padded" }, { kGrand, kUpright, kTine, kReed, kFmEp, kDigital, kDrawbar, kPipe, kToy, kMusicBox, kFmKeys }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (warmPad(), 0.38f)); } });
    L.push_back ({ "air", { "& Air", "Haze", "Mist" }, { kGrand, kUpright, kTine, kFmEp, kMusicBox, kToy, kHarpsi, kFmKeys }, "cloud",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (airPad(), 0.34f)); } });
    L.push_back ({ "vox", { "& Voices", "Angel", "Vocal" }, { kGrand, kUpright, kTine, kPipe, kChamberOrgan, kReedOrgan, kMusicBox }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (vocalPad(), 0.36f)); } });
    L.push_back ({ "choir", { "& Choir", "Choral", "Hymnal" }, { kGrand, kPipe, kChamberOrgan, kReedOrgan, kHarpsi, kDrawbar }, "church",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (choir(), 0.4f)); } });
    L.push_back ({ "bowed", { "& Bowed Bars", "Halo", "Aura" }, { kGrand, kUpright, kTine, kFmEp, kToy }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (bowedVibes(), 0.38f)); } });
    L.push_back ({ "octave", { "Octave", "Doubled", "Twin Octave" }, { kGrand, kUpright, kTine, kReed, kHarpsi, kDigital, kFmEp }, "room",
                   [=] (VaultPreset& p) { auto b = p.parts.front(); b.octave = -1; b.level = 0.55f; p.parts.push_back (b); } });
    L.push_back ({ "hybrid", { "Hybrid", "Electric Grand", "Blend" }, { kGrand, kUpright }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (tineLayer(), 0.5f)); } });
    L.push_back ({ "chimes", { "& Chimes", "Chiming", "Belfry" }, { kGrand, kTine, kFmEp, kDigital, kMusicBox, kToy }, "plate",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (chimes(), 0.3f)); } });
    L.push_back ({ "glock", { "Sparkle", "Twinkle", "Star Dust" }, { kGrand, kTine, kFmEp, kToy, kMusicBox, kUpright }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (glock(), 0.14f)); } });
    L.push_back ({ "vibes", { "& Vibes", "Lounge", "Vibe" }, { kTine, kReed, kFmEp, kGrand }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (vibes(), 0.45f, 0.1f)); } });
    L.push_back ({ "fmep", { "Ballad", "Eighties", "Satin" }, { kTine, kGrand, kDigital }, "plate",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (fmEpLayer(), 0.45f)); p.parts.front().set ("chorus", 0.3f); } });
    L.push_back ({ "flute", { "& Flute", "Breeze", "Soul Flute" }, { kTine, kReed, kGrand, kHarpsi }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (flute(), 0.3f)); } });
    L.push_back ({ "honky", { "Honky Tonk", "Saloon", "Barroom" }, { kUpright, kGrand }, "room",
                   [=] (VaultPreset& p) { auto b = p.parts.front(); b.set ("s.tune", 14.0f); b.level = 0.8f; b.pan = 0.25f; p.parts.front().pan = -0.25f; p.parts.push_back (b); } });
    L.push_back ({ "coupled", { "Coupled", "Double Manual", "Twin Manual" }, { kHarpsi }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("keys.harpsichord.flemish", "upper 8'", "Flemish Harpsichord", { { "s.cutoff", 5500.0f }, { "s.velSens", 0.6f } }), 0.6f, 0.15f)); } });
    L.push_back ({ "pizz", { "& Pizzicato", "Plucked Court", "Minuet" }, { kHarpsi, kClav, kMusicBox }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("strings.violins.ensemble", "pizzicato", "Violin Section", { { "s.velSens", 0.5f } }), 0.4f, 0.2f)); } });
    L.push_back ({ "pedal16", { "Full Pedal", "Plenum", "Foundation" }, { kPipe, kChamberOrgan }, "church",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("organ.pipe", "pedal", "Pipe Organ", { { "s.velSens", 0.6f } }), 0.5f)); p.parts.back().octave = -1; } });
    L.push_back ({ "fourfoot", { "Bright Stop", "Clarion", "Octave Stop" }, { kPipe, kChamberOrgan }, "church",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("organ.chamber", "4 foot", "Chamber Organ", { { "s.velSens", 0.6f } }), 0.2f, 0.1f)); } });
    L.push_back ({ "organpiano", { "Gospel", "Revival", "Sunday" }, { kGrand, kUpright }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (Y ("Drawbar Organ", { { "engine", 4 }, { "wave", 0.35f }, { "attack", 0.01f }, { "sustain", 1 }, { "release", 0.15f }, { "cutoff", 4000 }, { "velSens", 0.5f } }), 0.35f)); } });
    L.push_back ({ "harp", { "& Harp", "Harpstring", "Angelic" }, { kGrand, kTine, kHarpsi, kMusicBox, kFmEp }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("pluck.harp.concert", "pluck", "Concert Harp", { { "s.velSens", 0.6f }, { "s.cutoff", 4000.0f } }), 0.42f, 0.2f)); } });
    L.push_back ({ "marimba", { "& Marimba", "Rosewood", "Timber" }, { kTine, kReed, kFmEp, kClav, kToy }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("mallets.marimba", "hit", "Marimba", { { "s.velSens", 0.6f } }), 0.4f, -0.15f)); } });
    L.push_back ({ "cellopizz", { "& Pizz Cellos", "Pizz", "Pulse" }, { kGrand, kUpright, kClav, kHarpsi }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("strings.cellos.ensemble", "pizzicato", "Cello Section", { { "s.velSens", 0.5f } }), 0.45f, -0.2f)); } });
    L.push_back ({ "tremstr", { "Trembling Strings", "Suspense", "Nervous" }, { kGrand, kUpright, kPipe, kHarpsi }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("strings.violas.ensemble", "tremolo", "Viola Section", { { "s.attack", 0.2f }, { "s.release", 0.9f }, { "s.cutoff", 3000.0f }, { "s.velSens", 0.4f } }), 0.35f, 0.15f)); } });
    L.push_back ({ "thumb", { "& Thumb Piano", "Tin Tine", "Lamella" }, { kMusicBox, kToy, kTine }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("world.kalimba.low", "pluck", "Thumb Piano", { { "s.velSens", 0.6f } }), 0.4f, 0.15f)); } });
    L.push_back ({ "twinep", { "Twin EP", "Double Tines", "Tine And Reed" }, { kTine, kReed }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (M (p.parts.front().source == "ep.reed" ? "ep.tine.bell" : "ep.reed", p.parts.front().source == "ep.reed" ? "Bell Tine Piano" : "Reed Electric Piano"), 0.55f)); } });
    L.push_back ({ "chapel", { "Chapel", "Vespers", "Cloister" }, { kHarpsi }, "church",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("organ.chamber", "8 foot", "Chamber Organ", { { "s.velSens", 0.5f }, { "s.attack", 0.05f } }), 0.4f)); } });
    L.push_back ({ "horns", { "& Horns", "Fanfare", "Herald" }, { kPipe, kChamberOrgan, kDrawbar }, "church",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (S ("brass.horn", "sustain", "French Horn", { { "s.attack", 0.12f }, { "s.cutoff", 2500.0f }, { "s.velSens", 0.5f } }), 0.38f)); } });
    L.push_back ({ "velvethybrid", { "Velvet Hybrid", "Silk Hybrid", "Twilight Keys" }, { kGrand, kUpright }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (tineLayer(), 0.4f)); p.parts.push_back (partAt (warmPad(), 0.25f)); } });
    L.push_back ({ "loungenight", { "Lounge Night", "Late Set", "Blue Smoke" }, { kTine, kReed, kFmEp }, "room",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (vibes(), 0.35f, 0.15f)); p.parts.push_back (partAt (warmPad(), 0.22f)); } });
    L.push_back ({ "feltstrings", { "Felt & Strings", "Quiet Hours", "Low Light" }, { kGrand, kUpright }, "hall",
                   [=] (VaultPreset& p) { auto& a = p.parts.front(); lowPass (a, 1300.0f); a.set ("s.velCutoff", 0.6f);
                                          p.parts.push_back (partAt (strings ("strings.cellos.ensemble", "Cello Section", 0.4f), 0.3f, -0.2f));
                                          p.parts.push_back (partAt (airPad(), 0.25f)); } });
    // 3-4 parts
    L.push_back ({ "cinema", { "Cinematic", "Film Score", "Epic" }, { kGrand, kUpright, kTine }, "stage",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (strings ("strings.violins.ensemble", "Violin Section"), 0.35f, 0.2f));
                                          p.parts.push_back (partAt (strings ("strings.cellos.ensemble", "Cello Section", 0.3f), 0.35f, -0.2f));
                                          p.parts.push_back (partAt (warmPad(), 0.25f)); } });
    L.push_back ({ "heaven", { "Heavenly", "Cathedral Light", "Sanctuary" }, { kGrand, kPipe, kChamberOrgan, kMusicBox }, "church",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (choir(), 0.32f, -0.15f));
                                          p.parts.push_back (partAt (strings ("strings.violins.ensemble", "Violin Section"), 0.28f, 0.15f));
                                          p.parts.push_back (partAt (airPad(), 0.22f)); } });
    L.push_back ({ "dreamep", { "Dreamwave", "Night Drive", "Neon Haze" }, { kTine, kFmEp, kReed }, "cloud",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (warmPad(), 0.3f));
                                          p.parts.push_back (partAt (chimes(), 0.22f, 0.2f)); } });
    L.push_back ({ "lullaby", { "Lullaby", "Cradle", "Nightlight" }, { kToy, kMusicBox, kGrand }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (airPad(), 0.3f));
                                          p.parts.push_back (partAt (glock(), 0.12f, 0.15f)); } });
    L.push_back ({ "continuo", { "Continuo", "Ensemble", "Baroque" }, { kHarpsi, kChamberOrgan }, "hall",
                   [=] (VaultPreset& p) { p.parts.push_back (partAt (strings ("strings.violins.ensemble", "Violin Section", 0.12f), 0.32f, 0.2f));
                                          p.parts.push_back (partAt (strings ("strings.cellos.ensemble", "Cello Section", 0.12f), 0.34f, -0.2f)); } });
    L.push_back ({ "ambient", { "Ambient", "Floating", "Weightless" }, { kGrand, kUpright, kTine }, "cloud",
                   [=] (VaultPreset& p) { auto s = p.parts.front(); attack (s, 0.6f); release (s, 2.0f); s.level = 0.35f; p.parts.push_back (s);
                                          p.parts.push_back (partAt (airPad(), 0.3f));
                                          p.parts.push_back (partAt (vocalPad(), 0.22f)); } });
    return L;
}

//==============================================================================
// names
juce::String joinWords (std::initializer_list<juce::String> w)
{
    juce::StringArray a;
    for (auto& s : w) if (s.trim().isNotEmpty()) a.add (s.trim());
    return a.joinIntoString (" ");
}
struct Namer
{
    std::set<juce::String> used;    // lower-case
    bool take (const juce::String& n)
    {
        if (n.length() > 22 || n.isEmpty()) return false;
        if (vaultaudit::nameProblem (n).isNotEmpty() || n.containsIgnoreCase ("808")) return false;
        // no repeated word ("Full Pedal Pedal Organ"); "&" alone is not a word
        juce::StringArray seen;
        for (auto& w : juce::StringArray::fromTokens (n.toLowerCase(), " ", ""))
        {
            if (w == "&") continue;
            if (seen.contains (w)) return false;
            seen.add (w);
        }
        return used.insert (n.toLowerCase()).second;
    }
};

//==============================================================================
// candidates
struct Cand
{
    VaultPreset preset;
    int priority = 0;               // lower first
    juce::String recipe;            // voice/treatment/space or layer (report)
    std::vector<juce::String> nameOptions;
    bool bellish = false;
};

std::vector<Cand> candidates()
{
    const auto V = voices();
    const auto T = treatments();
    const auto L = layers();
    std::vector<Cand> out;
    // 1. voice x treatment x space: round r takes the r-th combination of every voice, so every voice is explored evenly
    std::vector<std::vector<Cand>> perVoice (V.size());
    for (size_t vi = 0; vi < V.size(); ++vi)
    {
        auto& v = V[vi];
        for (auto& t : T)
        {
            if (! suits (t, v.kind)) continue;
            int si = 0;
            for (auto* sp : t.spaces)
            {
                // organs live in churches / chapels, not on plates; pianos are not played in a church
                const bool organ = v.kind == kPipe || v.kind == kChamberOrgan || v.kind == kReedOrgan;
                if (juce::String (sp) == "church" && ! organ && v.kind != kHarpsi) continue;
                if (juce::String (sp) == "plate" && organ) continue;
                Cand c;
                c.preset.category = v.category;
                c.preset.parts = { v.part };
            neutralFx (c.preset.parts.front());
                c.preset.parts.front().label = v.part.label;
                fx (c.preset, space (sp).fx);   // the space first: a treatment may reshape it (distant, underwater)
                t.apply (c.preset);
                c.recipe = juce::String (v.tag) + "/" + t.tag + "/" + sp;
                c.priority = si;
                c.bellish = v.kind == kMusicBox || v.kind == kToy;
                const auto& sw = space (sp).word;
                // adjectives rotated per voice: "Midnight Concert Grand", "Shadow Studio Grand", "Nocturne Upright" ...
                for (int ai = 0; ai < t.adjectives.size(); ++ai)
                    for (auto& noun : v.nouns)
                    {
                        const auto adj = t.adjectives[t.adjectives[0].isEmpty() ? ai : (ai + (int) vi) % t.adjectives.size()];
                        c.nameOptions.push_back (joinWords ({ adj, noun, si > 0 ? juce::String (sw) : juce::String() }));
                        c.nameOptions.push_back (joinWords ({ adj, noun }));
                        c.nameOptions.push_back (joinWords ({ adj, sw, noun }));
                        c.nameOptions.push_back (joinWords ({ noun, sw }));
                    }
                perVoice[vi].push_back (std::move (c));
                ++si;
            }
        }
        // interleave inside the voice: first space of every treatment first
        std::stable_sort (perVoice[vi].begin(), perVoice[vi].end(), [] (const Cand& a, const Cand& b) { return a.priority < b.priority; });
    }
    // 2. layers: voice x layer recipe (natural treatment, the layer's space; a darker main for the dense ones)
    std::vector<std::vector<Cand>> perLayer (L.size());
    for (size_t li = 0; li < L.size(); ++li)
    {
        auto& l = L[li];
        for (auto& v : V)
        {
            if (std::find (l.kinds.begin(), l.kinds.end(), v.kind) == l.kinds.end()) continue;
            Cand c;
            c.preset.category = v.category;
            c.preset.parts = { v.part };
            neutralFx (c.preset.parts.front());
            l.add (c.preset);
            fx (c.preset, space (l.spaceTag).fx);
            if (c.preset.parts.size() >= 3) lowPass (c.preset.parts.front(), 4500.0f);
            c.recipe = juce::String (v.tag) + "+" + l.tag;
            c.bellish = v.kind == kMusicBox || v.kind == kToy;
            const int rot = (int) (&v - &V.front());
            for (int wi = 0; wi < l.words.size(); ++wi)
                for (auto& noun : v.nouns)
                {
                    const auto w = l.words[(wi + rot) % l.words.size()];
                    if (w.startsWith ("&")) c.nameOptions.push_back (joinWords ({ noun, w }));
                    else
                    {
                        c.nameOptions.push_back (joinWords ({ w, noun }));
                        c.nameOptions.push_back (w + " " + noun.fromLastOccurrenceOf (" ", false, false));
                    }
                }
            perLayer[li].push_back (std::move (c));
        }
    }
    // merge: voice candidates round-robin over the voices, layer candidates round-robin over the layer recipes, 2 : 1
    auto flatten = [] (const std::vector<std::vector<Cand>>& lists)
    {
        std::vector<Cand> f;
        for (size_t r = 0;; ++r)
        {
            bool any = false;
            for (auto& l : lists) if (r < l.size()) { f.push_back (l[r]); any = true; }
            if (! any) break;
        }
        return f;
    };
    const auto A = flatten (perVoice), B = flatten (perLayer);
    for (size_t a = 0, b = 0; a < A.size() || b < B.size();)
    {
        for (int k = 0; k < 2 && a < A.size(); ++k) out.push_back (A[a++]);
        if (b < B.size()) out.push_back (B[b++]);
    }
    return out;
}

//==============================================================================
// calibration: every part alone at -20 dB K-weighted (C5, velocity 0.8, no FX) -> its `gain`
std::mutex calMutex;
std::map<juce::String, float> calCache;

bool isFxKey (const juce::String& id)
{
    static const std::set<juce::String> fxKeys { "drive", "driveType", "crush", "wow", "chorus", "phaser", "flanger", "delayMix", "delayTime", "delayFb", "delayMode",
                                                 "revMix", "revSize", "revType", "eqLow", "eqHigh", "reverse", "freeze", "width", "master", "gain", "macro2", "macro5" };
    return fxKeys.count (id) > 0;
}
juce::String partKey (const VaultPart& p)
{
    juce::String k;
    k << (int) p.kind << "|" << p.source << "|" << p.articulation << "|" << p.octave << "|" << p.semi;
    auto v = p.values;
    std::sort (v.begin(), v.end());
    for (auto& [id, x] : v) if (! isFxKey (id)) k << "|" << id << "=" << juce::String (x, 4);
    return k;
}
float partLoudness (const VaultPart& part)
{
    const auto key = partKey (part);
    {
        std::lock_guard<std::mutex> l (calMutex);
        if (auto it = calCache.find (key); it != calCache.end()) return it->second;
    }
    VaultPreset solo;
    solo.id = "cal"; solo.name = "cal"; solo.category = vault::catPiano;
    VaultPart p = part;
    p.level = 1.0f; p.pan = 0.0f;
    Values keep;
    for (auto& [id, x] : p.values) if (! isFxKey (id)) keep.push_back ({ id, x });
    p.values = keep;
    neutralFx (p);
    solo.parts = { p };
    const double secs = 3.0;
    auto b = recipegate::render (solo, vault::homeNote, 0.8f, 2.0, secs);
    const float loud = vaultqc::analyse (b, recipegate::sr, 2.0).loud;
    std::lock_guard<std::mutex> l (calMutex);
    calCache[key] = loud;
    return loud;
}
void calibrate (VaultPreset& p)
{
    for (auto& part : p.parts)
    {
        const float loud = partLoudness (part);
        part.set ("gain", juce::jlimit (-24.0f, 12.0f, -20.0f - (loud < -90.0f ? -20.0f : loud)));
    }
}

//==============================================================================
// own fingerprint: MFCC (c0..c12 of a 40-band log-mel spectrum, orthonormal DCT -> Euclidean distance = RMS dB of the
// smoothed spectral envelope) per time segment, over C5 (vel 0.8, 7 segments incl. the release), C5 vel 0.35 (2), C4 (3),
// C6 (3) + stereo width (side / mid, 2 segments), all relative to the C5 render's loudest segment.
constexpr int ownMel = 40, ownCeps = 13;
std::vector<float> mfccSegments (const juce::AudioBuffer<float>& b, const std::vector<std::pair<double, double>>& segs, float refDb, bool relative, float& maxDb)
{
    constexpr int order = 11, n = 1 << order;
    juce::dsp::FFT fft (order);
    const double sr = recipegate::sr;
    auto mel = [] (double hz) { return 2595.0 * std::log10 (1.0 + hz / 700.0); };
    const double m0 = mel (50.0), m1 = mel (16000.0);
    std::vector<float> d ((size_t) (2 * n));
    std::vector<std::vector<double>> bands;
    for (auto [t0, t1] : segs)
    {
        std::vector<double> band (ownMel, 1.0e-12);
        int frames = 0;
        for (int s0 = (int) (t0 * sr); s0 + n <= std::min (b.getNumSamples(), (int) (t1 * sr) + n / 2); s0 += n / 4)
        {
            std::fill (d.begin(), d.end(), 0.0f);
            for (int i = 0; i < n; ++i)
                d[(size_t) i] = 0.5f * (b.getSample (0, s0 + i) + b.getSample (1, s0 + i))
                                * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (n - 1)));
            fft.performFrequencyOnlyForwardTransform (d.data());
            for (int i = 1; i < n / 2; ++i)
            {
                const double hz = i * sr / n;
                if (hz < 50.0 || hz >= 16000.0) continue;
                const int k = juce::jlimit (0, ownMel - 1, (int) ((mel (hz) - m0) / (m1 - m0) * ownMel));
                band[(size_t) k] += (double) d[(size_t) i] * d[(size_t) i];
            }
            ++frames;
        }
        for (auto& x : band) x = 10.0 * std::log10 (x / std::max (1, frames));
        bands.push_back (band);
    }
    maxDb = -300;
    for (auto& band : bands) for (auto x : band) maxDb = std::max (maxDb, (float) x);
    const float ref = relative ? maxDb : refDb;
    std::vector<float> out;
    for (auto& band : bands)
    {
        std::vector<double> lb (ownMel);
        for (int m = 0; m < ownMel; ++m) lb[(size_t) m] = std::max (-70.0, band[(size_t) m] - ref);   // dB under the reference, floored
        for (int q = 0; q < ownCeps; ++q)
        {
            double c = 0;
            for (int m = 0; m < ownMel; ++m) c += lb[(size_t) m] * std::cos (juce::MathConstants<double>::pi * q * (m + 0.5) / ownMel);
            c *= std::sqrt ((q == 0 ? 1.0 : 2.0) / ownMel);
            out.push_back ((float) c);
        }
    }
    return out;
}
float sideToMid (const juce::AudioBuffer<float>& b, double t0, double t1)
{
    double s = 1e-12, m = 1e-12;
    for (int i = (int) (t0 * recipegate::sr); i < std::min (b.getNumSamples(), (int) (t1 * recipegate::sr)); ++i)
    {
        const double l = b.getSample (0, i), r = b.getSample (1, i);
        m += 0.25 * (l + r) * (l + r); s += 0.25 * (l - r) * (l - r);
    }
    return (float) (10.0 * std::log10 (s / m));
}
// modulation features (the spectral fingerprints are blind to them): amplitude tremolo depth, pan tremolo depth and
// spectral fluctuation (chorus / phaser / rotary), each the residual SD (dB) of a linear-detrended trajectory over the held
// note (0.25 .. 1.45 s, 10 ms frames)
float detrendedSd (const std::vector<double>& y)
{
    const int n = (int) y.size();
    if (n < 8) return 0;
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int i = 0; i < n; ++i) { sx += i; sy += y[(size_t) i]; sxx += (double) i * i; sxy += i * y[(size_t) i]; }
    const double b = (n * sxy - sx * sy) / std::max (1e-9, n * sxx - sx * sx), a = (sy - b * sx) / n;
    double e = 0;
    for (int i = 0; i < n; ++i) { const double r = y[(size_t) i] - (a + b * i); e += r * r; }
    return (float) std::sqrt (e / n);
}
std::array<float, 3> modulation (const juce::AudioBuffer<float>& b)
{
    const double sr = recipegate::sr;
    const int fr = (int) (0.01 * sr), s0 = (int) (0.25 * sr), s1 = std::min (b.getNumSamples(), (int) (1.45 * sr));
    std::vector<double> mono, pan;
    for (int s = s0; s + fr <= s1; s += fr)
    {
        double l = 1e-12, r = 1e-12;
        for (int i = s; i < s + fr; ++i) { l += (double) b.getSample (0, i) * b.getSample (0, i); r += (double) b.getSample (1, i) * b.getSample (1, i); }
        mono.push_back (10.0 * std::log10 ((l + r) / (2 * fr)));
        pan.push_back (10.0 * std::log10 (l / r));
    }
    // spectral fluctuation: 32 log bands 150 Hz .. 8 kHz, 1024-point frames every 10 ms
    constexpr int order = 10, n = 1 << order, nb = 32;
    juce::dsp::FFT fft (order);
    std::vector<float> d ((size_t) (2 * n));
    std::vector<std::vector<double>> traj (nb);
    for (int s = s0; s + n <= s1; s += fr)
    {
        std::fill (d.begin(), d.end(), 0.0f);
        for (int i = 0; i < n; ++i) d[(size_t) i] = 0.5f * (b.getSample (0, s + i) + b.getSample (1, s + i)) * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (n - 1)));
        fft.performFrequencyOnlyForwardTransform (d.data());
        std::vector<double> band (nb, 1e-14);
        for (int i = 1; i < n / 2; ++i)
        {
            const double hz = i * sr / n;
            if (hz < 150 || hz >= 8000) continue;
            band[(size_t) juce::jlimit (0, nb - 1, (int) (std::log (hz / 150.0) / std::log (8000.0 / 150.0) * nb))] += (double) d[(size_t) i] * d[(size_t) i];
        }
        for (int k = 0; k < nb; ++k) traj[(size_t) k].push_back (10.0 * std::log10 (band[(size_t) k]));
    }
    double mx = -300;
    for (auto& t : traj) for (auto v : t) mx = std::max (mx, v);
    double flux = 0; int used = 0;
    for (auto& t : traj)
    {
        if (t.empty()) continue;
        double mean = 0; for (auto v : t) mean += v; mean /= (double) t.size();
        if (mean < mx - 45) continue;
        flux += detrendedSd (t); ++used;
    }
    return { std::min (12.0f, detrendedSd (mono)), std::min (12.0f, detrendedSd (pan)), (float) std::min (12.0, used > 0 ? flux / used : 0.0) };
}

std::vector<float> ownPrint (const VaultPreset& p)
{
    const auto c5 = recipegate::render (p, vault::homeNote, 0.8f, 1.5, 3.0);
    const auto soft = recipegate::render (p, vault::homeNote, 0.35f, 1.5, 1.5);
    const auto c4 = recipegate::render (p, vault::homeNote - 12, 0.8f, 1.2, 2.0);
    const auto c6 = recipegate::render (p, vault::homeNote + 12, 0.8f, 1.2, 2.0);
    float mx = 0, dummy = 0;
    auto f = mfccSegments (c5, { { 0.0, 0.03 }, { 0.03, 0.1 }, { 0.1, 0.3 }, { 0.3, 0.7 }, { 0.7, 1.5 }, { 1.5, 2.0 }, { 2.0, 3.0 } }, 0, true, mx);
    for (auto* b : { &soft })
        for (auto x : mfccSegments (*b, { { 0.0, 0.1 }, { 0.1, 0.7 } }, mx, false, dummy)) f.push_back (x);
    for (auto* b : { &c4, &c6 })
        for (auto x : mfccSegments (*b, { { 0.0, 0.1 }, { 0.1, 0.6 }, { 0.6, 1.2 } }, mx, false, dummy)) f.push_back (x);
    // stereo image (scaled down: a width change alone is not a new sound)
    f.push_back (0.3f * juce::jlimit (-40.0f, 0.0f, sideToMid (c5, 0.05, 1.5)));
    f.push_back (0.3f * juce::jlimit (-40.0f, 0.0f, sideToMid (c5, 1.5, 3.0)));
    // modulation: weighted like a whole spectral segment (a clear tremolo / chorus counts about as much as a new tone)
    const float w = std::sqrt ((float) ownMel * 5.0f);
    for (auto m : modulation (c5)) f.push_back (w * m);
    return f;
}
float ownDistance (const std::vector<float>& a, const std::vector<float>& b)
{
    // per-segment cepstra: sum of squared differences / number of segments -> RMS dB over the smoothed envelopes
    double s = 0;
    const size_t n = std::min (a.size(), b.size());
    for (size_t i = 0; i < n; ++i) { const double d = a[i] - b[i]; s += d * d; }
    const double segs = (double) (n - 5) / ownCeps;   // 15 cepstral segments (+ 2 width + 3 modulation values)
    return (float) std::sqrt (s / std::max (1.0, segs * ownMel));
}

//==============================================================================
// repairs after a gate failure (returns false when nothing more can be done)
bool repair (VaultPreset& p, const juce::String& fail, int attempt)
{
    auto& a = p.parts.front();
    if (fail == "fundamental" || fail == "harsh-2-5k" || fail == "piercing-highs")
    {
        for (auto& part : p.parts) lowPass (part, std::min (currentLp (part), 8000.0f) * (fail == "fundamental" ? 0.55f : 0.65f));
        if (fail == "fundamental") a.set ("eqLow", std::min (6.0f, a.get ("eqLow", 0.0f) + 2.0f));
        else a.set ("eqHigh", std::max (-8.0f, a.get ("eqHigh", 0.0f) - 2.0f));
        return true;
    }
    if (fail == "velocity")
    {
        for (auto& part : p.parts) velSens (part, std::min (1.0f, getVelSens (part) + 0.25f));
        return attempt < 3;
    }
    if (fail == "click-start")
    {
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample) part.set ("s.attack", std::max (0.004f, part.get ("s.attack", 0.0f)));
        else if (part.kind == VaultPartKind::synth) part.set ("attack", std::max (0.006f, part.get ("attack", 0.002f)));
        return attempt < 2;
    }
    if (fail == "click-end" && a.get ("s.releaseSamples", 1.0f) > 0.5f)
    {
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample) part.set ("s.releaseSamples", 0.0f);
        return true;
    }
    if (fail == "click-spike" && (a.get ("wow", 0.0f) > 0.05f || a.get ("crush", 0.0f) > 0.05f))
    {
        a.set ("wow", a.get ("wow", 0.0f) * 0.4f); a.set ("crush", a.get ("crush", 0.0f) * 0.4f);
        return true;
    }
    if (fail == "click-end" || fail == "click-spike")
    {
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample) part.set ("s.release", std::max (0.2f, part.get ("s.release", 0.0f)));
        else if (part.kind == VaultPartKind::synth) part.set ("release", std::max (0.2f, part.get ("release", 0.3f)));
        else part.set ("m.release", std::max (0.25f, part.get ("m.release", 0.3f)));
        return attempt < 2;
    }
    if (fail == "tail")
    {
        a.set ("revMix", a.get ("revMix", 0.0f) * 0.7f); a.set ("revSize", a.get ("revSize", 0.0f) * 0.75f); a.set ("delayFb", a.get ("delayFb", 0.35f) * 0.6f);
        for (auto& part : p.parts)
            if (part.kind == VaultPartKind::sample && part.get ("s.release", 0.0f) > 0.0f) part.set ("s.release", part.get ("s.release", 0.0f) * 0.6f);
            else if (part.kind == VaultPartKind::model) part.set ("m.release", std::min (1.0f, part.get ("m.release", 0.5f)));
        // sustained samples ringing on: give them a release
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample && part.get ("s.release", 0.0f) <= 0.0f) part.set ("s.release", 2.5f);
        return true;
    }
    if (fail == "octave-layer")
    {
        for (size_t k = 1; k < p.parts.size(); ++k) if (p.parts[k].octave > a.octave) p.parts[k].level *= 0.6f;
        return true;
    }
    if (fail == "low-side" || fail == "mono-loss")
    {
        a.set ("width", a.get ("width", 0.5f) * 0.6f);
        a.set ("chorus", a.get ("chorus", 0.0f) * 0.5f);
        for (auto& part : p.parts) { part.pan *= 0.5f; if (part.kind == VaultPartKind::sample) part.set ("s.width", 0.6f); }
        return true;
    }
    if (fail == "too-peaky" || fail == "clip")
    {
        a.set ("master", 0.45f);   // MASTER glue / limiter stage tames the hammer transient
        return attempt < 2;
    }
    return false;
}


//==============================================================================
// KEYS_CALIB=1: distances of hand-picked pairs (jitter vs. real differences) to set the thresholds
void calibrationReport()
{
    auto V = voices();
    auto voice = [&] (const char* tag) { for (auto& v : V) if (juce::String (v.tag) == tag) return v; return V.front(); };
    auto make = [&] (const char* tag, const char* sp, std::function<void (VaultPreset&)> f = {})
    {
        VaultPreset p; p.id = "cal"; p.name = "Cal"; auto v = voice (tag); p.category = v.category; p.parts = { v.part }; neutralFx (p.parts.front());
        fx (p, space (sp).fx);
        if (f) f (p);
        calibrate (p);
        return p;
    };
    struct Pair { const char* what; VaultPreset a, b; };
    std::vector<Pair> pairs {
        { "JITTER grand dry: revMix +0.04", make ("concert", "dry"), make ("concert", "dry", [] (VaultPreset& p) { p.parts[0].set ("revMix", 0.09f); }) },
        { "JITTER grand hall: size 0.62->0.72", make ("concert", "hall"), make ("concert", "hall", [] (VaultPreset& p) { p.parts[0].set ("revSize", 0.72f); }) },
        { "JITTER grand dry: cutoff 9 kHz", make ("concert", "dry"), make ("concert", "dry", [] (VaultPreset& p) { p.parts[0].set ("s.cutoff", 9000.0f); }) },
        { "JITTER grand dry: eqHigh -1.5", make ("concert", "dry"), make ("concert", "dry", [] (VaultPreset& p) { p.parts[0].set ("eqHigh", -1.5f); }) },
        { "JITTER tine: tone 0.5->0.58", make ("tine", "dry"), make ("tine", "dry", [] (VaultPreset& p) { p.parts[0].set ("m.tone", 0.58f); }) },
        { "JITTER organ: revMix 0.32->0.27", make ("principal", "church"), make ("principal", "church", [] (VaultPreset& p) { p.parts[0].set ("revMix", 0.27f); }) },
        { "SPACE grand dry vs room", make ("concert", "dry"), make ("concert", "room") },
        { "SPACE grand dry vs hall", make ("concert", "dry"), make ("concert", "hall") },
        { "SPACE grand hall vs stage", make ("concert", "hall"), make ("concert", "stage") },
        { "SPACE grand hall vs cloud", make ("concert", "hall"), make ("concert", "cloud") },
        { "INSTR concert vs studio grand (dry)", make ("concert", "dry"), make ("studio", "dry") },
        { "INSTR concert vs chamber grand (dry)", make ("concert", "dry"), make ("chamber", "dry") },
        { "INSTR concert sustain vs pedal (dry)", make ("concert", "dry"), make ("pedal", "dry") },
        { "INSTR upright vs parlor (dry)", make ("upright", "dry"), make ("parlor", "dry") },
        { "INSTR english vs french harpsichord", make ("hpsEnglish", "dry"), make ("hpsFrench", "dry") },
        { "INSTR flemish lower vs upper", make ("hpsFlemLo", "dry"), make ("hpsFlemUp", "dry") },
        { "INSTR tine vs bell tine", make ("tine", "dry"), make ("tinebell", "dry") },
        { "INSTR principal vs chamber 8'", make ("principal", "dry"), make ("chamber8", "dry") },
        { "ROLE grand dry vs felt room", make ("concert", "dry"), make ("concert", "room", [] (VaultPreset& p) { lowPass (p.parts[0], 1250.0f); p.parts[0].set ("s.velCutoff", 0.65f); }) },
        { "ROLE tine room vs tremolo room", make ("tine", "room"), make ("tine", "room", [] (VaultPreset& p) { p.parts[0].set ("m.trem", 0.55f); }) },
        { "ROLE tine room vs chorus room", make ("tine", "room"), make ("tine", "room", [] (VaultPreset& p) { p.parts[0].set ("chorus", 0.42f); p.parts[0].set ("width", 0.75f); }) },
        { "ROLE grand room vs lofi room", make ("concert", "room"), make ("concert", "room", [] (VaultPreset& p) { lowPass (p.parts[0], 3600.0f); fx (p, { { "crush", 0.22f }, { "wow", 0.32f }, { "drive", 0.18f }, { "driveType", 1 }, { "width", 0.32f } }); }) },
        { "LAYER grand hall vs + pad", make ("concert", "hall"), make ("concert", "hall", [] (VaultPreset& p) { auto q = warmPad(); q.level = 0.38f; p.parts.push_back (q); calibrate (p); }) },
        { "LAYER grand hall vs + strings", make ("concert", "hall"), make ("concert", "hall", [] (VaultPreset& p) { auto q = strings ("strings.violins.ensemble", "Violin Section"); q.level = 0.42f; p.parts.push_back (q); calibrate (p); }) },
    };
    for (auto& pr : pairs)
    {
        recipegate::Result ra, rb;
        ra.preset = pr.a; rb.preset = pr.b;
        recipegate::analyse (ra); recipegate::analyse (rb);
        std::printf ("  calib %-42s own %5.2f  shared %5.2f   (%s | %s)\n", pr.what, ownDistance (ownPrint (pr.a), ownPrint (pr.b)), recipegate::distance (ra.print, rb.print),
                     ra.fail.toRawUTF8(), rb.fail.toRawUTF8());
    }
}

struct Done
{
    Cand cand;
    recipegate::Result res;
    std::vector<float> own;
    int repairs = 0;
};

int envInt (const char* k, int def) { auto v = juce::SystemStats::getEnvironmentVariable (k, {}); return v.isEmpty() ? def : v.getIntValue(); }
float envFloat (const char* k, float def) { auto v = juce::SystemStats::getEnvironmentVariable (k, {}); return v.isEmpty() ? def : (float) v.getDoubleValue(); }
} // namespace

//==============================================================================
std::vector<VaultPreset> build_keys (const RecipeContext& ctx)
{
    if (envInt ("KEYS_CALIB", 0) != 0) { calibrationReport(); return {}; }
    // KEYS_DEMO_BANK=<bank.cvb> KEYS_DEMO_DIR=<dir> KEYS_DEMO_IDS=<id,id,...>: C5 demos (2.5 s, note off at 1.8 s) of a gated bank
    if (auto bankPath = juce::SystemStats::getEnvironmentVariable ("KEYS_DEMO_BANK", {}); bankPath.isNotEmpty())
    {
        std::vector<VaultPreset> bank;
        if (! vault::loadBankFile (juce::File (bankPath), bank)) { std::printf ("keys: cannot read %s\n", bankPath.toRawUTF8()); return {}; }
        juce::File dir (juce::SystemStats::getEnvironmentVariable ("KEYS_DEMO_DIR", {}));
        dir.createDirectory();
        for (auto& id : juce::StringArray::fromTokens (juce::SystemStats::getEnvironmentVariable ("KEYS_DEMO_IDS", {}), ",", ""))
            for (auto& p : bank)
                if (p.id == id)
                {
                    auto b = recipegate::render (p, vault::homeNote, 0.8f, 1.8, 2.5);
                    const int fade = (int) (0.06 * recipegate::sr);
                    for (int c = 0; c < 2; ++c) b.applyGainRamp (c, b.getNumSamples() - fade, fade, 1.0f, 0.0f);
                    auto f = dir.getChildFile (juce::String (vault::categorySlug (p.category)) + " - " + p.name + " C5.wav");
                    recipegate::writeWav (f, b);
                    std::printf ("keys demo: %s (peak %.2f)\n", f.getFullPathName().toRawUTF8(), b.getMagnitude (0, b.getNumSamples()));
                }
        return {};
    }
    const float ownMin = envFloat ("KEYS_OWN_MIN", 2.6f), sharedMin = envFloat ("KEYS_SHARED_MIN", 3.05f);
    const float ownMax25 = 0.08f, ownMax6k = 0.02f;
    const int threads = envInt ("KEYS_THREADS", (int) std::max (1u, std::thread::hardware_concurrency()));
    auto cands = candidates();
    // drop candidates whose instruments the library lacks
    cands.erase (std::remove_if (cands.begin(), cands.end(), [&] (const Cand& c)
    {
        for (auto& part : c.preset.parts)
            if (part.kind == VaultPartKind::sample && ! ctx.hasInstrument (part.source)) return true;
        return false;
    }), cands.end());
    if (const int lim = envInt ("KEYS_LIMIT", 0); lim > 0 && lim < (int) cands.size()) cands.resize ((size_t) lim);
    std::printf ("keys: %d candidates, %d threads\n", (int) cands.size(), threads);

    // analyse (+ repairs) in parallel
    std::vector<Done> done (cands.size());
    std::atomic<int> next { 0 }, count { 0 };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&]
        {
            for (int i; (i = next++) < (int) cands.size();)
            {
                auto& d = done[(size_t) i];
                d.cand = cands[(size_t) i];
                auto& p = d.cand.preset;
                p.id = "cv.tmp." + juce::String (i); p.name = "Candidate " + juce::String (i);
                calibrate (p);
                if (juce::String why; ! vault::validatePreset (p, &why)) { d.res.fail = "invalid: " + why; std::printf ("  keys INVALID %s\n", why.toRawUTF8()); continue; }
                for (int attempt = 0; attempt < 5; ++attempt)
                {
                    d.res = recipegate::Result();
                    d.res.preset = p;
                    recipegate::analyse (d.res);
                    // this recipe's own, stricter comfort limits (the shared gate allows 10 % / 3 %)
                    if (d.res.fail.isEmpty() && d.res.share25 > ownMax25) d.res.fail = "harsh-2-5k";
                    if (d.res.fail.isEmpty() && d.res.share6k > ownMax6k) d.res.fail = "piercing-highs";
                    if (d.res.fail.isEmpty() || d.res.fail.startsWith ("name") || d.res.fail == "label") break;
                    if (! repair (p, d.res.fail, attempt)) break;
                    calibrate (p);
                    ++d.repairs;
                }
                if (d.res.fail.isEmpty()) d.own = ownPrint (p);
                if (++count % 50 == 0) std::printf ("  keys %d / %d\n", count.load(), (int) cands.size());
            }
        });
    for (auto& t : pool) t.join();

    // greedy selection in candidate order: gate passed, distinct from everything accepted on BOTH fingerprints
    std::vector<int> acc;
    std::map<juce::String, int> rejects;
    std::vector<float> nearShared (done.size(), 1e9f), nearOwn (done.size(), 1e9f);
    std::vector<juce::String> why (done.size());
    for (int i = 0; i < (int) done.size(); ++i)
    {
        auto& d = done[(size_t) i];
        if (d.res.fail.isNotEmpty()) { why[(size_t) i] = d.res.fail; ++rejects[d.res.fail]; continue; }
        float ms = 1e9f, mo = 1e9f;
        for (int j : acc)
        {
            ms = std::min (ms, recipegate::distance (d.res.print, done[(size_t) j].res.print));
            mo = std::min (mo, ownDistance (d.own, done[(size_t) j].own));
        }
        nearShared[(size_t) i] = ms; nearOwn[(size_t) i] = mo;
        if (ms < sharedMin || mo < ownMin) { why[(size_t) i] = "near-duplicate"; ++rejects["near-duplicate"]; continue; }
        acc.push_back (i);
    }

    // names (unique, evocative), ids, labels
    Namer namer;
    std::vector<VaultPreset> out;
    std::map<int, int> perCat;
    for (int i : acc)
    {
        auto& d = done[(size_t) i];
        VaultPreset p = d.cand.preset;
        juce::String name;
        for (auto& n : d.cand.nameOptions) if (namer.take (n)) { name = n; break; }
        if (name.isEmpty())
            for (int k = 2; k < 50 && name.isEmpty(); ++k)
                for (auto& n : d.cand.nameOptions)
                    if (namer.take (n + " " + juce::String (juce::StringArray { "II", "III", "IV", "V", "VI", "VII", "VIII" }[(k - 2) % 7]))) { name = n + " " + juce::StringArray { "II", "III", "IV", "V", "VI", "VII", "VIII" }[(k - 2) % 7]; break; }
        if (name.isEmpty()) { why[(size_t) i] = "no name"; continue; }
        p.name = name;
        const int n = ++perCat[p.category];
        p.id = juce::String ("cv.") + vault::categorySlug (p.category) + "." + juce::String (n).paddedLeft ('0', 4);
        p.gain = 1.0f;
        out.push_back (p);
        d.cand.preset = p;
    }

    // min pairwise distances among the accepted
    float minOwn = 1e9f, minShared = 1e9f;
    juce::String ownA, ownB, shA, shB;
    for (size_t x = 0; x < acc.size(); ++x)
        for (size_t y = x + 1; y < acc.size(); ++y)
        {
            auto& a = done[(size_t) acc[x]]; auto& b = done[(size_t) acc[y]];
            const float o = ownDistance (a.own, b.own), s = recipegate::distance (a.res.print, b.res.print);
            if (o < minOwn) { minOwn = o; ownA = a.cand.preset.name; ownB = b.cand.preset.name; }
            if (s < minShared) { minShared = s; shA = a.cand.preset.name; shB = b.cand.preset.name; }
        }
    std::printf ("keys: accepted %d of %d (Piano %d, Keys & Organ %d)\n", (int) out.size(), (int) done.size(), perCat[vault::catPiano], perCat[vault::catKeysOrgan]);
    std::printf ("keys: min pairwise own distance %.2f (%s / %s), shared %.2f (%s / %s); thresholds own %.2f shared %.2f\n",
                 minOwn, ownA.toRawUTF8(), ownB.toRawUTF8(), minShared, shA.toRawUTF8(), shB.toRawUTF8(), ownMin, sharedMin);
    for (auto& [w, n] : rejects) std::printf ("  keys rejected %-16s %d\n", w.toRawUTF8(), n);

    // measurement summary per category
    for (int cat : { (int) vault::catPiano, (int) vault::catKeysOrgan })
    {
        std::vector<float> h1, s25, s6, cen, vel, tail;
        for (int i : acc)
        {
            auto& r = done[(size_t) i];
            if (r.cand.preset.category != cat) continue;
            h1.push_back (r.res.h1Db); s25.push_back (100 * r.res.share25); s6.push_back (100 * r.res.share6k); cen.push_back (r.res.centroid);
            vel.push_back (r.res.velRange); tail.push_back (r.res.tail);
        }
        auto stat = [] (std::vector<float> v) { if (v.empty()) return juce::String ("-"); std::sort (v.begin(), v.end());
                                               return juce::String (v.front(), 1) + " / " + juce::String (v[v.size() / 2], 1) + " / " + juce::String (v.back(), 1); };
        std::printf ("  %s (min / median / max): H1-vs-strongest dB %s | 2-5 kHz %% %s | >6 kHz %% %s | centroid Hz %s | vel 0.35->1.0 dB %s | tail s %s\n",
                     vault::categoryNames()[cat].toRawUTF8(), stat (h1).toRawUTF8(), stat (s25).toRawUTF8(), stat (s6).toRawUTF8(), stat (cen).toRawUTF8(),
                     stat (vel).toRawUTF8(), stat (tail).toRawUTF8());
    }

    // optional TSV report
    if (auto rp = juce::SystemStats::getEnvironmentVariable ("KEYS_REPORT", {}); rp.isNotEmpty())
    {
        juce::String s ("idx\trecipe\tresult\tname\tid\tparts\trepairs\tloud\tH1\t2-5k%\t>6k%\tcentroid\ttail\tvel\toctUp\tnearShared\tnearOwn\n");
        for (int i = 0; i < (int) done.size(); ++i)
        {
            auto& d = done[(size_t) i];
            const bool ok = std::find (acc.begin(), acc.end(), i) != acc.end() && why[(size_t) i].isEmpty();
            s << i << "\t" << d.cand.recipe << "\t" << (ok ? juce::String ("ok") : why[(size_t) i]) << "\t" << (ok ? d.cand.preset.name : juce::String()) << "\t"
              << (ok ? d.cand.preset.id : juce::String()) << "\t" << (int) d.cand.preset.parts.size() << "\t" << d.repairs << "\t" << juce::String (d.res.loud, 1) << "\t"
              << juce::String (d.res.h1Db, 1) << "\t" << juce::String (100 * d.res.share25, 2) << "\t" << juce::String (100 * d.res.share6k, 2) << "\t"
              << (int) d.res.centroid << "\t" << juce::String (d.res.tail, 2) << "\t" << juce::String (d.res.velRange, 1) << "\t"
              << juce::String (d.res.octUpDb, 1) << "\t" << juce::String (nearShared[(size_t) i] < 1e8f ? nearShared[(size_t) i] : 0.0f, 2) << "\t"
              << juce::String (nearOwn[(size_t) i] < 1e8f ? nearOwn[(size_t) i] : 0.0f, 2) << "\n";
        }
        juce::File (rp).replaceWithText (s);
        std::printf ("keys: report %s\n", rp.toRawUTF8());
    }
    return out;
}

VAULT_RECIPE (keys, build_keys);
