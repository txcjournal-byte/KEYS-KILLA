// CHROME VAULT - sound design for the categories "Piano" and "Keys & Organ" (VaultBankGen -recipe keys).
//
// v3 DESIGN NOTE (every preset ships finished: FX chain, movement, 4 named sound macros - namespace v3 below)
// --------------------------------------------------------------------------------------------------------
// Research summary (felt / grand piano libraries, vintage electric piano and tonewheel organ instruments, their manuals):
// * Commercial piano libraries sell one instrument heard through a finished chain: close / room / far perspectives,
//   parallel-style compression that keeps the hammer, a hint of tape saturation ("glow"), a mud cut around 300 Hz,
//   convolution room / hall / plate with pre-delay, and controls for hammer / tone, pedal (release), tape and ambience.
//   Felt pianos add mechanical intimacy and a dark top; lo-fi variants band-limit, crush and wobble the pitch.
// * Electric pianos: tine pianos live on the stereo (auto-pan) tremolo of the suitcase amp (about 4-6 Hz), chorus and
//   phaser; reed pianos on a mono amplitude tremolo (about 5.5 Hz) and the preamp's bark, which comes in with
//   velocity; the 80s FM piano always gets a chorus and an echo. Drive is the first stage of the chain.
// * Tonewheel organs: preamp overdrive is part of the sound, the rotary speaker has slow / fast speeds (the mod wheel
//   speeds it up); pipe organs: tremulant (a slow pitch + level wobble) on a stop, an expression (swell) pedal, large
//   acoustics with pre-delay. Harpsichords: no compression, baroque hall, quills that can be softened.
// * Clavinet-style funk keys: tight compression, an envelope-following wah, phaser, a little amp.
// What this file does with it: each preset's chain is FAMILY (the instrument's finished sound) + TREATMENT (its role:
// felt, lo-fi, tape, swell, chorus, rotary, echo pluck ...) + SPACE (reverb / delay from the v2 space tag), so a grand
// gets comp + tape + EQ + room / hall, a lo-fi piano tape drive + crush + wobble LFO + room, a pad-layered piano a
// slow brightness drift and a long hall, a drawbar organ drive + rotary chorus with a slow / fast rotor, a pipe organ
// EQ + church with a tremulant and an expression macro. The 4 macros are named for the sound (Hammer, Pedal, Warmth,
// Room / Bark, Tremolo, Tone, Plate / Rotor, Drive, Tone, Room / Expression, Tremulant, Tone, Nave / Quill, Release,
// Width, Hall / Wah, Funk, Phase, Room / Strings, Pad, Choir ... for layer blends), each mapped to several targets
// around the designed value (the knob's default = the sound as designed). Every macro is rendered at 0 and at 1 and
// narrowed until both extremes pass the C5 comfort rules (2-5 kHz, > 6 / 8 kHz, fundamental, no clip, no louder than
// +4 dB, no longer tail; a failing side is narrowed, the sound at the knob's default never changes); a macro that
// barely changes the sound is reported. Arp / gate stay off except for a few presets where the pattern is the point
// (music box / FM keys arpeggios - harpsichord / celeste ones when those echo presets are in the set - and gated FM /
// digital piano and dreamwave pads), named plainly ("Music Box Arpeggio", "Gated FM Piano").
// Distinctness: the family polish makes a voice's presets alike, so after the design every preset is checked against the
// earlier ones (this file's MFCC fingerprint >= 2.6, the shared one >= 3.5 = the bank assembly's threshold); one that
// came out too close is designed again without the family polish, then with its role pushed further, then in the
// neighbouring colour, light, and finally with its v2 sound; still too close = dropped (the assembly would drop it).
// The v2 stage below (voices, treatments, spaces, layers, distinctness, names, ids) is unchanged; only the few pattern
// presets are renamed. KEYS_V2_CACHE=<dir> keeps the v2 stage's result between runs while the design is tuned.
//
// v2 notes
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

//==============================================================================
// v3 SOUND DESIGN: every accepted preset gets a finished FX chain chosen for its role, movement where it adds life and 4
// sound macros named for that sound (DESIGN NOTE v3 at the top). Built from the recipe tag of the v2 stage
// ("voice/treatment/space" or "voice+layer"), so the same voice x treatment always gets the same kind of processing.
namespace v3
{
using namespace recipe;
using FT = vault::FxType;
using R = fx::Reverb;

struct Info
{
    juce::String voice, treat, space, layer;
    Kind kind = kGrand;
    int var = 0;              // a small per-preset variation (pre-delay, rates), from the preset id
    juce::String noun;        // the voice's shortest noun (arp / gate names)
};

bool isPipeLike (Kind k) { return k == kPipe || k == kChamberOrgan; }
bool isEp (Kind k) { return k == kTine || k == kReed || k == kFmEp; }
bool isPianoLike (Kind k) { return k == kGrand || k == kUpright || k == kDigital; }

Info parseInfo (const juce::String& recipeTag, const VaultPreset& p)
{
    static const auto V = voices();
    Info in;
    if (recipeTag.contains ("+")) { in.voice = recipeTag.upToFirstOccurrenceOf ("+", false, false); in.layer = recipeTag.fromFirstOccurrenceOf ("+", false, false); in.treat = "natural"; }
    else
    {
        auto t = juce::StringArray::fromTokens (recipeTag, "/", "");
        in.voice = t[0]; in.treat = t[1]; in.space = t[2];
    }
    for (auto& v : V)
        if (in.voice == v.tag)
        {
            in.kind = v.kind;
            in.noun = v.nouns[0];
            for (auto& n : v.nouns) if (n.length() < in.noun.length()) in.noun = n;
            if (in.layer.isNotEmpty())
                for (auto& l : layers()) if (in.layer == l.tag) in.space = l.spaceTag;
        }
    in.var = (int) (p.id.hashCode64() & 0x7fffffff) % 997;
    return in;
}

//------------------------------------------------------------------------------ the chain
struct Chain
{
    std::optional<VaultFxSlot> transient, comp, drive, body, lofi, filter, chorus, phaser, flanger, eq, delay, reverb, reverse, width;
    VaultFxSlot& eqSlot() { if (! eq) eq = fx::eq (0, 0, 0); return *eq; }
    std::vector<VaultFxSlot> build() const
    {
        std::vector<VaultFxSlot> v;
        for (auto* s : { &transient, &comp, &drive, &body, &lofi, &filter, &chorus, &phaser, &flanger, &eq, &delay, &reverb, &reverse, &width })
            if (s->has_value()) v.push_back (**s);
        for (auto t : { FT::transient, FT::flanger, FT::width, FT::compressor })   // at most 8 slots: the least important go first
            if (v.size() > (size_t) vault::maxFxSlots)
                for (auto it = v.begin(); it != v.end(); ++it) if (it->type == t) { v.erase (it); break; }
        return v;
    }
};

struct Mod
{
    std::optional<VaultLfo> l1, l2;
    std::vector<VaultModDest> mw, vel, at;
};

// LFO 1 / 2 roles
VaultLfo suitcase (float depth, float rate = 4.6f) { return lfo (VaultLfo::sine).rate (rate).depth (depth).to ("volume", 0.22f).to ("pan", 0.6f); }
VaultLfo ampTrem (float depth, float rate = 5.5f) { return lfo (VaultLfo::sine).rate (rate).depth (depth).to ("volume", 0.4f); }
VaultLfo tremulant (float depth) { return lfo (VaultLfo::sine).rate (6.2f).fade (0.25f).depth (depth).to ("pitch", 0.09f).to ("volume", 0.12f); }
VaultLfo rotor (float rate, float depth = 1.0f) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("pan", 0.38f).to ("volume", 0.07f); }
VaultLfo wobble (float depth, float rate = 0.55f, float semis = 0.12f) { return lfo (VaultLfo::triangle).rate (rate).free().depth (depth).to ("pitch", semis); }
VaultLfo drift (float depth, float rate = 0.2f) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", 0.8f).to ("pan", 0.4f); }
VaultLfo bellows (float depth) { return lfo (VaultLfo::sine).rate (0.45f).free().depth (depth).to ("volume", 0.22f); }

//------------------------------------------------------------------------------ spaces
void spaceFx (Chain& c, const juce::String& sp, const Info& in)
{
    const float pre = 6.0f * (float) (in.var % 4);
    auto rv = [] (R t, float size, float decay, float mix, float predelay, float damping)
    { return fx::reverb (t, size, decay, mix).set ("predelay", predelay).set ("damping", damping); };
    if (sp == "dry")         c.reverb = rv (R::room, 0.3f, 0.75f, 0.11f, 0.0f, 0.35f);
    else if (sp == "room")   c.reverb = rv (R::room, 0.38f, 0.8f, 0.15f, 0.5f * pre, 0.25f);
    else if (sp == "studio") c.reverb = rv (R::room, 0.6f, 1.05f, 0.18f, 4.0f + pre, 0.1f);
    else if (sp == "hall")   c.reverb = rv (R::hall, 0.7f, 1.15f, 0.24f, 16.0f + pre, 0.1f);
    else if (sp == "stage")  c.reverb = rv (R::hall, 0.85f, 1.25f, 0.32f, 28.0f + pre, 0.15f);
    else if (sp == "plate")  c.reverb = rv (R::plate, 0.55f, 1.0f, 0.2f, 10.0f + pre, 0.2f);
    else if (sp == "cloud")  c.reverb = rv (R::cloud, 0.66f, 1.15f, 0.3f, 18.0f + pre, 0.25f);
    else if (sp == "church") c.reverb = rv (R::hall, 0.88f, 1.35f, 0.3f, 32.0f + pre, 0.2f).set ("lowCut", 120.0f);
    else if (sp == "echo")
    {
        c.delay = fx::delay ("1/4", 0.32f, 0.14f, fx::Delay::tape).set ("hiCut", 4500.0f).set ("loCut", 180.0f).set ("flutter", 0.25f);
        c.reverb = rv (R::plate, 0.42f, 0.9f, 0.13f, 8.0f, 0.3f);
    }
    else if (sp == "ping")
    {
        c.delay = fx::delay ("1/8D", 0.3f, 0.16f).set ("hiCut", 6000.0f).set ("loCut", 200.0f);
        c.reverb = rv (R::hall, 0.5f, 0.9f, 0.14f, 10.0f + pre, 0.2f);
    }
    else c.reverb = rv (R::room, 0.42f, 0.9f, 0.16f, pre, 0.2f);
}

//------------------------------------------------------------------------------ the instrument family's finished sound
void familyFx (Chain& c, Mod& m, float& finish, const Info& in)
{
    switch (in.kind)
    {
        case kGrand:     // gentle parallel comp, a whisper of tape, mud cut, a little width
            c.comp = fx::comp (-24, 1.8f, 25, 200, 1.0f, 0.6f);
            c.drive = fx::drive (fx::Drive::tape, 0.06f, 0.5f, 12000);
            c.eq = fx::eq (0.5f, -1.0f, 0.5f, 0, 320);
            c.width = fx::width (0.58f, 150);
            finish = 0.12f;
            break;
        case kUpright:   // more tape, honky mids
            c.comp = fx::comp (-22, 2.0f, 20, 180, 1.0f, 0.65f);
            c.drive = fx::drive (fx::Drive::tape, 0.08f, 0.6f, 9000);
            c.eq = fx::eq (1.0f, 0.8f, -1.0f, 0, 1200);
            finish = 0.1f;
            break;
        case kDigital:   // the 90s workstation piano: compressed, a light chorus, wide
            c.comp = fx::comp (-20, 2.5f, 10, 150, 1.0f);
            c.eq = fx::eq (0.5f, 0, -1.0f);
            c.width = fx::width (0.62f, 150);
            finish = 0.18f;
            break;
        case kTine:      // suitcase: comp, tape warmth, stereo tremolo (subtle by default), a short dotted echo
            c.comp = fx::comp (-20, 2.5f, 15, 120, 1.0f);
            c.drive = fx::drive (fx::Drive::tape, 0.08f, 0.7f, 9000);
            c.eq = fx::eq (1.5f, -0.5f, -1.5f, 0, 500);
            c.delay = fx::delay ("1/8D", 0.22f, 0.07f).set ("hiCut", 5000.0f).set ("loCut", 200.0f);
            m.l1 = suitcase (in.treat == "natural" ? 0.12f : 0.0f, 4.4f + 0.1f * (float) (in.var % 5));
            m.vel.push_back ({ "fx.drive.amount", 0.15f });
            finish = 0.12f;
            break;
        case kReed:      // reed piano: preamp bark, mid push, mono amp tremolo (off until the macro opens it)
            c.comp = fx::comp (-20, 2.5f, 12, 120, 1.0f);
            c.drive = fx::drive (fx::Drive::tube, 0.12f, 0.8f, 7000);
            c.eq = fx::eq (1.0f, 1.5f, -2.0f, 0, 900);
            m.l1 = ampTrem (0.0f);
            m.vel.push_back ({ "fx.drive.amount", 0.2f });
            finish = 0.1f;
            break;
        case kFmEp:      // 80s FM electric piano: always chorus, a quarter echo, a slow stereo drift
            c.comp = fx::comp (-20, 2.0f, 10, 150, 0.5f);
            c.chorus = fx::chorus (0.25f, 0.5f, 0.4f);
            c.eq = fx::eq (0.5f, 0, -1.5f);
            c.delay = fx::delay ("1/4", 0.22f, 0.08f).set ("hiCut", 5000.0f).set ("loCut", 200.0f);
            c.width = fx::width (0.65f, 150);
            m.l1 = lfo (VaultLfo::sine).rate (0.2f).free().to ("pan", 0.15f);
            finish = 0.15f;
            break;
        case kToy:       // a toy piano on an old recorder: tape, darker top, a slow wobble
            c.drive = fx::drive (fx::Drive::tape, 0.1f, 0.6f, 8000);
            c.eq = fx::eq (1.5f, 0, -2.0f);
            m.l2 = wobble (0.3f, 0.45f, 0.06f);
            break;
        case kMusicBox:  // a wound spring: slight drift, an echo, wide
            c.eq = fx::eq (1.0f, 0, -1.5f);
            c.delay = fx::delay ("1/8D", 0.3f, 0.12f).set ("hiCut", 5000.0f).set ("loCut", 250.0f);
            c.width = fx::width (0.65f, 150);
            m.l2 = wobble (0.3f, 0.35f, 0.05f);
            break;
        case kPipe:      // pipe organ: weight, a slightly softened top, a tremulant ready on its macro
            c.eq = fx::eq (1.5f, -1.0f, -1.0f, 0, 400);
            c.width = fx::width (0.62f, 150);
            m.l1 = tremulant (0.0f);
            finish = 0.05f;
            break;
        case kChamberOrgan:
            c.eq = fx::eq (1.0f, 0, -1.5f);
            m.l1 = tremulant (0.0f);
            break;
        case kDrawbar:   // tonewheel style: preamp drive, rotary speaker (chorus + pan / level wobble, slow)
            c.drive = fx::drive (fx::Drive::tube, 0.08f, 0.8f, 6500).set ("cleanLow", 1.0f);
            c.chorus = fx::chorus (0.0f, 0.8f, 0.6f);
            c.eq = fx::eq (1.5f, 0, -1.5f);
            c.width = fx::width (0.65f, 150);
            m.l1 = rotor (0.8f, 0.0f);
            m.mw.push_back ({ "lfo1.rate", 0.6f });   // mod wheel: rotor towards fast
            finish = 0.1f;
            break;
        case kReedOrgan: // harmonium / accordion / calliope: gentle comp, reedy mids, breathing bellows
            c.comp = fx::comp (-22, 2.0f, 20, 200, 1.0f);
            c.eq = fx::eq (0.5f, 1.0f, -1.5f, 0, 800);
            m.l1 = bellows (0.4f);
            break;
        case kHarpsi:    // harpsichord: no comp (it has no dynamics to tame), softened quills, wide baroque room
            c.eq = fx::eq (-0.5f, 0, -1.5f);
            c.width = fx::width (0.65f, 150);
            finish = 0.05f;
            break;
        case kClav:      // funk keys: tight comp, envelope wah, a little amp
            c.comp = fx::comp (-18, 3.0f, 5, 100, 1.5f);
            c.drive = fx::drive (fx::Drive::tube, 0.06f, 1.0f, 8000);
            c.filter = fx::filter (fx::Filter::lowPass, 12000, 0.2f, 0.0f);
            c.eq = fx::eq (0.5f, 0, -1.0f);
            finish = 0.15f;
            break;
        case kFmKeys:    // glassy FM keys: chorus, dotted echo, wide
            c.eq = fx::eq (0, 0, -1.5f);
            c.delay = fx::delay ("1/8D", 0.28f, 0.1f).set ("hiCut", 5500.0f).set ("loCut", 200.0f);
            c.width = fx::width (0.65f, 150);
            finish = 0.1f;
            break;
        default: break;
    }
}

//------------------------------------------------------------------------------ treatments: the role on top of the family
void treatmentFx (Chain& c, Mod& m, float& finish, const Info& in)
{
    const auto& t = in.treat;
    const Kind k = in.kind;
    static const juce::StringArray ownMotion { "phaser", "chorus", "lofi", "vinyl", "tape", "underwater", "rotary", "tremolo", "tremulant", "swell", "reverse", "echopluck" };
    if (ownMotion.contains (t))
    {
        c.chorus.reset(); c.phaser.reset(); c.flanger.reset();
        if (k == kDrawbar || k == kFmEp) m.l1.reset();
        if (m.l1 && k == kTine) m.l1->depth = 0.0f;
        if (k == kToy || k == kMusicBox) m.l2.reset();
    }
    if (t == "felt")
    {
        c.drive = fx::drive (fx::Drive::tape, 0.08f, 0.5f, 5000);
        c.eqSlot().set ("low", 1.5f).set ("high", -2.0f);
        if (c.comp) c.comp->set ("mix", 0.5f);
        c.chorus.reset();
    }
    else if (t == "dark")
    {
        c.eqSlot().set ("low", 2.5f).set ("high", -3.0f);
        if (! c.drive) c.drive = fx::drive (fx::Drive::tape, 0.12f, 0.6f, 5000);
        else c.drive->set ("tone", 5000.0f);
    }
    else if (t == "intimate")
    {
        c.comp = fx::comp (-28, 2.2f, 30, 220, 2.0f, 0.5f);
        c.width = fx::width (0.5f, 150);
    }
    else if (t == "lofi")
    {
        c.lofi = fx::lofi (0.2f, 0.3f);
        c.drive = fx::drive (fx::Drive::tape, 0.18f, 1.0f, 6000);
        c.eqSlot().set ("low", 1.5f).set ("high", -3.0f);
        c.width = fx::width (0.35f, 150);
        c.chorus.reset();
        m.l2 = wobble (0.45f, 0.6f, 0.12f);
        finish = 0.2f;
    }
    else if (t == "tape")
    {
        c.drive = fx::drive (fx::Drive::tape, 0.28f, 1.0f, 9000);
        c.lofi = fx::lofi (0.0f, 0.16f);
        c.eqSlot().set ("low", 1.0f).set ("high", -2.5f);
        m.l2 = wobble (0.3f, 0.5f, 0.06f);
        finish = 0.2f;
    }
    else if (t == "vinyl")
    {
        c.lofi = fx::lofi (0.08f, 0.45f);
        c.drive = fx::drive (fx::Drive::tape, 0.12f, 1.0f, 7000);
        c.eqSlot().set ("low", -1.0f).set ("high", -3.5f);
        c.width = fx::width (0.4f, 150);
        c.chorus.reset();
        m.l2 = wobble (0.5f, 0.33f, 0.1f);
    }
    else if (t == "muted")
    {
        c.comp = fx::comp (-20, 3.0f, 3, 80, 1.5f);
    }
    else if (t == "swell")
    {
        c.chorus = fx::ensemble (0.3f, 0.4f, 0.5f);
        if (c.reverb) { c.reverb->set ("mix", std::min (0.45f, c.reverb->get ("mix") * 1.2f)); c.reverb->set ("decay", c.reverb->get ("decay") * 1.2f); }
        c.comp.reset();
        if (! m.l2) m.l2 = drift (0.6f);
    }
    else if (t == "bright")
    {
        c.comp = fx::comp (-18, 3.0f, 8, 120, 2.0f);
        finish = 0.35f;
    }
    else if (t == "chorus")
    {
        c.chorus = k == kFmEp || k == kFmKeys ? fx::ensemble (0.5f, 0.6f, 0.6f) : fx::chorus (0.42f, 0.42f, 0.55f);
        c.width = fx::width (0.75f, 150);
        c.eqSlot().set ("low", c.eqSlot().get ("low") - 1.0f);
    }
    else if (t == "phaser")
    {
        c.phaser = fx::phaser (0.45f, 0.3f, 0.9f, 0.5f);
        if (! c.drive) c.drive = fx::drive (fx::Drive::tape, 0.12f, 1.0f, 9000);
    }
    else if (t == "drive")
    {
        c.drive = fx::drive (fx::Drive::tube, 0.32f, 1.0f, 6000).set ("cleanLow", 1.0f);
        c.eqSlot().set ("mid", 1.5f).set ("midFreq", 1000.0f).set ("high", -2.0f);
    }
    else if (t == "tremolo")
    {
        if (k == kTine) m.l1 = suitcase (0.7f, 4.8f);
        else if (k == kDrawbar) { m.l1 = rotor (5.8f); c.chorus = fx::chorus (0.35f, 5.0f, 0.6f); }
        else m.l1 = ampTrem (0.7f, 5.2f);
    }
    else if (t == "tremulant")
    {
        m.l1 = tremulant (0.7f);
    }
    else if (t == "rotary")
    {
        c.chorus = fx::chorus (0.55f, 0.9f, 0.6f);
        c.drive = fx::drive (fx::Drive::tape, 0.22f, 1.0f, 7000).set ("cleanLow", 1.0f);
        c.width = fx::width (0.75f, 150);
        m.l1 = rotor (0.9f);
    }
    else if (t == "bell")
    {
        if (! c.delay) c.delay = fx::delay ("1/4", 0.25f, 0.08f).set ("hiCut", 5000.0f).set ("loCut", 250.0f);
    }
    else if (t == "round")
    {
        c.eqSlot().set ("low", 1.5f).set ("high", -1.5f);
    }
    else if (t == "distant")
    {
        c.reverb = fx::reverb (R::hall, 0.8f, 1.1f, 0.45f).set ("predelay", 45.0f).set ("damping", 0.3f);
        c.eqSlot().set ("low", -1.5f).set ("high", -2.0f);
        c.width = fx::width (0.7f, 150);
        c.comp.reset();
    }
    else if (t == "underwater")
    {
        c.filter = fx::filter (fx::Filter::lowPass, 900, 0.3f, 0, 0.25f, 0.12f);
        c.chorus = fx::chorus (0.5f, 0.25f, 0.7f);
        c.lofi = fx::lofi (0.0f, 0.18f);
        c.width = fx::width (0.7f, 150);
        c.drive.reset();
        m.l2 = drift (0.6f, 0.15f);
    }
    else if (t == "wooden")
    {
        c.body = VaultFxSlot::make (FT::body).set ("model", 5.0f).set ("mix", 0.38f);
    }
    else if (t == "echopluck")
    {
        c.delay = fx::delay ("1/8D", 0.42f, 0.28f).set ("hiCut", 5000.0f).set ("loCut", 200.0f);
        c.reverb = fx::reverb (R::plate, 0.4f, 0.9f, 0.14f).set ("predelay", 10.0f);
    }
    else if (t == "reverse")
    {
        c.reverse = fx::reverse (0.55f, "1/8");
    }
}

// layer recipes: what the blend needs on top
void layerFx (Chain& c, Mod& m, const Info& in, const VaultPreset& p)
{
    const auto& l = in.layer;
    if (l.isEmpty()) return;
    // pads / air / voices drift slowly in brightness so the bed breathes under the main instrument
    for (size_t k = 1; k < p.parts.size(); ++k)
    {
        const auto& lab = p.parts[k].label;
        if ((lab == "Warm Pad" || lab == "Air Pad" || lab == "Vocal Pad") && ! m.l2 && (l == "pad" || l == "air" || l == "vox"))
            m.l2 = lfo (VaultLfo::sine).rate (0.16f + 0.02f * (float) (in.var % 4)).free().depth (0.7f)
                       .to (juce::String ("layer" + juce::String ((int) k + 1) + ".cutoff").toRawUTF8(), 0.5f);
    }
    if (l == "strings" || l == "cellos" || l == "cinema" || l == "heaven" || l == "continuo" || l == "tremstr" || l == "feltstrings")
    {
        if (c.reverb) c.reverb->set ("predelay", 24.0f);
    }
    else if (l == "honky")
    {
        c.drive = fx::drive (fx::Drive::tape, 0.15f, 0.8f, 8000);
    }
    else if (l == "organpiano")
    {
        if (! m.l1) m.l1 = lfo (VaultLfo::sine).rate (0.8f).free().to ("layer2.pan", 0.4f);
    }
    else if (l == "glock" || l == "chimes" || l == "lullaby")
    {
        if (! c.delay) c.delay = fx::delay ("1/8D", 0.25f, 0.08f).set ("hiCut", 5000.0f).set ("loCut", 250.0f);
    }
    else if (l == "twinep" || l == "fmep" || l == "dreamep" || l == "loungenight")
    {
        if (! c.chorus) c.chorus = fx::chorus (0.3f, 0.45f, 0.5f);
    }
    else if (l == "ambient")
    {
        c.reverb = fx::reverb (R::cloud, 0.75f, 1.3f, 0.36f).set ("predelay", 30.0f);
        c.comp.reset();
    }
}

//------------------------------------------------------------------------------ macros
// a target around the designed value v: the knob's default d gives v, the range is `span` wide (negative: turning up lowers
// the value), narrowed so min / max stay inside lo..hi
struct MT { juce::String target; float v, span, lo, hi; };
VaultMacro makeMacro (const juce::String& name, float d, const std::vector<MT>& ts)
{
    VaultMacro m;
    m.name = name;
    for (auto& t : ts)
    {
        float span = t.span;
        if (span > 0)
        {
            if (d > 0) span = std::min (span, std::max (0.0f, t.v - t.lo) / d);
            if (d < 1) span = std::min (span, std::max (0.0f, t.hi - t.v) / (1 - d));
        }
        else if (span < 0)
        {
            if (d > 0) span = std::max (span, -std::max (0.0f, t.hi - t.v) / d);
            if (d < 1) span = std::max (span, -std::max (0.0f, t.v - t.lo) / (1 - d));
        }
        if (std::abs (span) < 1.0e-4f) continue;
        m.targets.push_back ({ t.target, t.v - d * span, t.v + (1 - d) * span, 0.0f });
    }
    return m;
}

struct MacroCtx
{
    VaultPreset& p;
    Chain& c;
    Mod& m;
    const Info& in;
    juce::StringArray used;
};

juce::String layerWord (const VaultPart& part, const VaultPart& main)
{
    const auto& l = part.label;
    if (part.source == main.source && part.octave < main.octave) return "Octave";
    if (part.source == main.source) return "Detune";
    static const std::vector<std::pair<const char*, const char*>> w {
        { "Violin Section", "Strings" }, { "Cello Section", "Cellos" }, { "Viola Section", "Shiver" }, { "Warm Pad", "Pad" }, { "Air Pad", "Air" },
        { "Vocal Pad", "Voices" }, { "Classic Choir", "Choir" }, { "Bowed Vibraphone", "Halo" }, { "Vibraphone", "Vibes" }, { "Hand Chimes", "Chimes" },
        { "Glockenspiel", "Sparkle" }, { "Concert Flute", "Flute" }, { "Tine Electric Piano", "Tines" }, { "Reed Electric Piano", "Reeds" },
        { "Bell Tine Piano", "Bells" }, { "FM Electric Piano", "Satin" }, { "Drawbar Organ", "Organ" }, { "Concert Harp", "Harp" }, { "Marimba", "Marimba" },
        { "Thumb Piano", "Thumb" }, { "French Horn", "Horns" }, { "Flemish Harpsichord", "Upper" } };
    if (part.articulation == "pizzicato") return "Pizzicato";
    if (l == "Pipe Organ" && part.articulation == "pedal") return "Bass Pipes";
    if (l == "Chamber Organ" && part.articulation == "4 foot") return "Octave";
    if (l == "Chamber Organ") return "Organ";
    for (auto& [a, b] : w) if (l == a) return b;
    return "Layer";
}

bool hasEnvLayer (const VaultPreset& p) { for (auto& part : p.parts) if (part.kind != VaultPartKind::model) return true; return false; }

juce::String spaceWord (const Info& in)
{
    const auto& s = in.space;
    if (in.treat == "distant") return "Distance";
    if (s == "hall" || s == "stage") return "Hall";
    if (s == "plate") return "Plate";
    if (s == "cloud") return "Cloud";
    if (s == "church") return isPipeLike (in.kind) || in.kind == kReedOrgan ? "Nave" : "Chapel";
    if (s == "studio") return "Studio";
    if (s == "echo" || s == "ping") return "Space";
    return "Room";
}

// one macro by its role id; empty name = not applicable to this preset
VaultMacro macroFor (const juce::String& id, MacroCtx& x, float& d)
{
    auto& c = x.c;
    auto& p = x.p;
    const auto& in = x.in;
    auto fxv = [] (const std::optional<VaultFxSlot>& s, const char* par) { return s->get (par); };
    auto layersWith = [&] (const char* what, float v, float span, float lo, float hi, std::vector<MT>& ts)
    {
        for (size_t k = 0; k < p.parts.size(); ++k)
            if (p.parts[k].kind != VaultPartKind::model)
                ts.push_back ({ "layer" + juce::String ((int) k + 1) + "." + what, v, span, lo, hi });
    };
    if (id == "space")
    {
        if (! c.reverb) return {};
        d = juce::jlimit (0.15f, 0.45f, fxv (c.reverb, "mix") / 0.5f);   // a close sound starts low on its knob, a hall further up
        std::vector<MT> ts { { "fx.reverb.mix", fxv (c.reverb, "mix"), 0.45f, 0.0f, 0.62f }, { "fx.reverb.size", fxv (c.reverb, "size"), 0.35f, 0.1f, 0.92f },
                             { "fx.reverb.decay", fxv (c.reverb, "decay"), 0.7f, 0.4f, 1.7f } };
        if (c.delay) ts.push_back ({ "fx.delay.mix", fxv (c.delay, "mix"), 0.25f, 0.0f, 0.4f });
        return makeMacro (spaceWord (in), d, ts);
    }
    if (id == "echo")
    {
        if (! c.delay) c.delay = fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 5000.0f).set ("loCut", 200.0f);
        d = fxv (c.delay, "mix") < 0.01f ? 0.0f : 0.35f;
        return makeMacro ("Echo", d, { { "fx.delay.mix", fxv (c.delay, "mix"), 0.4f, 0.0f, 0.45f }, { "fx.delay.feedback", fxv (c.delay, "feedback"), 0.3f, 0.12f, 0.6f } });
    }
    if (id == "tone" || id == "hammer" || id == "lid" || id == "shine" || id == "quill" || id == "brightness")
    {
        d = 0.6f;
        const char* name = id == "hammer" ? "Hammer" : id == "lid" ? "Lid" : id == "shine" ? "Shine" : id == "quill" ? "Quill" : "Tone";
        return makeMacro (name, d, { { "cutoff", 0.0f, 2.2f, -3.0f, 1.2f }, { "fx.eq.high", c.eqSlot().get ("high"), 5.0f, -10.0f, 2.5f },
                                     { "fx.eq.low", c.eqSlot().get ("low"), -2.5f, -3.0f, 4.0f } });
    }
    if (id == "felt")
    {
        d = 0.4f;
        return makeMacro ("Felt", d, { { "cutoff", 0.0f, -2.4f, -3.0f, 1.0f }, { "fx.eq.high", c.eqSlot().get ("high"), -4.0f, -9.0f, 2.0f },
                                       { "fx.eq.low", c.eqSlot().get ("low"), 2.0f, -3.0f, 4.0f } });
    }
    if (id == "tape")
    {
        if (! c.drive || (int) c.drive->get ("type") != (int) fx::Drive::tape) c.drive = fx::drive (fx::Drive::tape, 0.0f, 1.0f, 9000);
        const float a = fxv (c.drive, "amount");
        d = a < 0.01f ? 0.0f : 0.3f;
        std::vector<MT> ts { { "fx.drive.amount", a, 0.5f, 0.0f, 0.6f }, { "fx.eq.high", c.eqSlot().get ("high"), -2.0f, -9.0f, 2.0f }, { "volume", 0.0f, -0.2f, -0.5f, 0.2f } };
        if (c.lofi) ts.push_back ({ "fx.lofi.wow", fxv (c.lofi, "wow"), 0.3f, 0.0f, 0.45f });
        return makeMacro ("Tape", d, ts);
    }
    if (id == "warmth")
    {
        if (! c.drive) c.drive = fx::drive (fx::Drive::tape, 0.0f, 1.0f, 9000);
        d = 0.4f;
        return makeMacro ("Warmth", d, { { "fx.drive.amount", fxv (c.drive, "amount"), 0.5f, 0.0f, 0.55f }, { "fx.eq.low", c.eqSlot().get ("low"), 4.0f, -3.0f, 5.5f },
                                         { "fx.eq.high", c.eqSlot().get ("high"), -4.5f, -10.0f, 2.0f }, { "volume", 0.0f, -0.15f, -0.4f, 0.2f } });
    }
    if (id == "bark" || id == "drive" || id == "funk")
    {
        if (! c.drive) c.drive = fx::drive (fx::Drive::tube, 0.0f, 1.0f, 7000);
        const float a = fxv (c.drive, "amount");
        d = a < 0.01f ? 0.0f : 0.3f;
        return makeMacro (id == "bark" ? "Bark" : id == "funk" ? "Funk" : "Drive", d,
                          { { "fx.drive.amount", a, 0.6f, 0.0f, 0.75f }, { "fx.eq.mid", c.eqSlot().get ("mid"), 3.0f, -4.0f, 4.0f }, { "volume", 0.0f, -0.25f, -0.5f, 0.2f } });
    }
    if (id == "dust")
    {
        if (! c.lofi) c.lofi = fx::lofi (0.0f, 0.0f);
        d = 0.4f;
        return makeMacro ("Dust", d, { { "fx.lofi.crush", fxv (c.lofi, "crush"), 0.45f, 0.0f, 0.5f }, { "fx.lofi.wow", fxv (c.lofi, "wow"), 0.4f, 0.0f, 0.55f },
                                       { "cutoff", 0.0f, -1.0f, -2.0f, 0.5f } });
    }
    auto lfoDepthMacro = [&] (int i, const char* name, std::vector<MT> extra = {}) -> VaultMacro
    {
        auto& l = i == 0 ? x.m.l1 : x.m.l2;
        if (! l) return {};
        d = l->depth;
        std::vector<MT> ts { { "lfo" + juce::String (i + 1) + ".depth", l->depth, 1.0f, 0.0f, 1.0f } };
        for (auto& e : extra) ts.push_back (e);
        return makeMacro (name, d, ts);
    };
    auto isPitchLfo = [] (const std::optional<VaultLfo>& l) { if (! l) return false; for (auto& dd : l->dests) if (dd.target == "pitch") return true; return false; };
    if (id == "wobble" || id == "windup")
    {
        if (! isPitchLfo (x.m.l2)) return {};
        return lfoDepthMacro (1, id == "windup" ? "Wind-Up" : "Wobble");
    }
    if (id == "drift")
    {
        if (! x.m.l2 || isPitchLfo (x.m.l2)) return {};
        return lfoDepthMacro (1, "Drift", { { "lfo2.rate", x.m.l2->rate, 0.8f, 0.05f, 1.0f } });
    }
    if (id == "tremolo")
    {
        if (! x.m.l1 || x.in.kind == kFmEp || x.in.kind == kDrawbar || x.in.kind == kReedOrgan || isPipeLike (x.in.kind)) return {};
        return lfoDepthMacro (0, "Tremolo");
    }
    if (id == "rate")
    {
        if (! x.m.l1) return {};
        d = 0.5f;
        return makeMacro ("Rate", d, { { "lfo1.rate", x.m.l1->rate, 4.0f, 1.5f, 8.0f } });
    }
    if (id == "tremulant")
    {
        if (! x.m.l1 || ! isPitchLfo (x.m.l1)) return {};
        return lfoDepthMacro (0, "Tremulant");
    }
    if (id == "bellows")
    {
        if (! x.m.l1 || x.in.kind != kReedOrgan || isPitchLfo (x.m.l1)) return {};
        return lfoDepthMacro (0, "Bellows");
    }
    if (id == "rotor")
    {
        if (! x.m.l1 || (x.in.kind != kDrawbar && x.in.treat != "rotary")) return {};
        if (! c.chorus) return {};
        // slow (0) -> fast (1); the designed speed sits at its place on the way
        const float r = x.m.l1->rate;
        d = juce::jlimit (0.0f, 1.0f, (r - 0.8f) / 5.8f);
        VaultMacro m;
        m.name = "Rotor";
        m.targets.push_back ({ "lfo1.rate", 0.8f, 6.6f, 0.0f });
        m.targets.push_back ({ "fx.chorus.rate", 0.7f, 5.5f, 0.0f });
        if (x.m.l1->depth < 0.01f)   // stopped at the default: the first part of the knob brings the slow rotor in
        {
            d = 0.0f;
            m.targets.push_back ({ "lfo1.depth", 0.0f, 1.0f, -0.8f });
            m.targets.push_back ({ "fx.chorus.mix", 0.0f, 0.5f, -0.8f });
        }
        else m.targets.push_back ({ "fx.chorus.mix", 0.25f, 0.5f, 0.0f });
        return m;
    }
    if (id == "chorus" || id == "musette")
    {
        if (! c.chorus) c.chorus = fx::chorus (0.0f, 0.45f, 0.5f);
        const float mix = fxv (c.chorus, "mix");
        d = mix < 0.01f ? 0.0f : 0.4f;
        std::vector<MT> ts { { "fx.chorus.mix", mix, 0.6f, 0.0f, 0.7f }, { "fx.chorus.depth", fxv (c.chorus, "depth"), 0.4f, 0.15f, 0.85f } };
        if (c.width) ts.push_back ({ "fx.width.width", fxv (c.width, "width"), 0.3f, 0.3f, 0.9f });
        return makeMacro (id == "musette" ? "Musette" : "Chorus", d, ts);
    }
    if (id == "phase")
    {
        if (! c.phaser) c.phaser = fx::phaser (0.0f, 0.3f, 0.9f, 0.5f);
        const float mix = fxv (c.phaser, "mix");
        d = mix < 0.01f ? 0.0f : 0.45f;
        return makeMacro ("Phase", d, { { "fx.phaser.mix", mix, 0.6f, 0.0f, 0.75f }, { "fx.phaser.feedback", fxv (c.phaser, "feedback"), 0.4f, 0.1f, 0.75f } });
    }
    if (id == "pedal" || id == "release")
    {
        if (! hasEnvLayer (p)) return {};
        d = 0.35f;
        std::vector<MT> ts;
        layersWith ("release", 0.0f, 4.5f, -2.0f, 2.6f, ts);
        return makeMacro (id == "pedal" ? (isPianoLike (in.kind) ? "Pedal" : "Sustain") : "Release", d, ts);
    }
    if (id == "swell")
    {
        if (! hasEnvLayer (p)) return {};
        d = 0.35f;
        std::vector<MT> ts;
        layersWith ("attack", 0.0f, 4.5f, -2.0f, 3.0f, ts);
        layersWith ("release", 0.0f, 1.5f, -1.0f, 1.5f, ts);
        if (ts.size() > 8) ts.resize (8);
        return makeMacro ("Swell", d, ts);
    }
    if (id == "width")
    {
        if (! c.width) c.width = fx::width (0.5f, 150);
        d = 0.5f;
        return makeMacro ("Width", d, { { "fx.width.width", fxv (c.width, "width"), 0.6f, 0.15f, 1.0f } });
    }
    if (id == "wah")
    {
        if (! c.filter) return {};
        d = 0.0f;   // off at the default: turning it up closes the filter and lets the playing open it (envelope wah)
        return makeMacro ("Wah", d, { { "fx.filter.env", fxv (c.filter, "env"), 0.6f, 0.0f, 0.85f }, { "fx.filter.reso", fxv (c.filter, "reso"), 0.35f, 0.05f, 0.65f },
                                      { "fx.filter.cutoff", fxv (c.filter, "cutoff"), -10800.0f, 800.0f, 12000.0f } });
    }
    if (id == "depth")
    {
        if (! c.filter) return {};
        d = 0.5f;
        return makeMacro ("Depth", d, { { "fx.filter.cutoff", fxv (c.filter, "cutoff"), -1200.0f, 300.0f, 2500.0f }, { "fx.chorus.mix", c.chorus ? fxv (c.chorus, "mix") : 0.0f, 0.4f, 0.0f, 0.7f } });
    }
    if (id == "body")
    {
        if (! c.body) return {};
        d = 0.45f;
        return makeMacro ("Body", d, { { "fx.body.mix", fxv (c.body, "mix"), 0.6f, 0.0f, 0.75f } });
    }
    if (id == "reverse")
    {
        if (! c.reverse) return {};
        d = 0.5f;
        return makeMacro ("Reverse", d, { { "fx.reverse.mix", fxv (c.reverse, "mix"), 0.7f, 0.0f, 0.8f } });
    }
    if (id == "glue")
    {
        if (! c.comp) return {};
        d = 0.4f;
        return makeMacro ("Glue", d, { { "finish", p.finish, 0.5f, 0.0f, 0.6f }, { "fx.comp.threshold", fxv (c.comp, "threshold"), -10.0f, -34.0f, -8.0f } });
    }
    if (id == "expression")
    {
        d = 1.0f;
        return makeMacro ("Expression", d, { { "volume", 0.0f, 0.55f, -0.6f, 0.0f }, { "cutoff", 0.0f, 1.4f, -1.5f, 0.0f } });
    }
    if (id.startsWith ("blend"))
    {
        const int k = id.getLastCharacters (1).getIntValue();
        if (k < 1 || k >= (int) p.parts.size()) return {};
        const auto& part = p.parts[(size_t) k];
        const auto word = layerWord (part, p.parts.front());
        if (word == "Detune")
        {
            d = 0.5f;
            return makeMacro ("Detune", d, { { "layer" + juce::String (k + 1) + ".tune", 0.0f, 0.3f, -0.2f, 0.2f } });
        }
        const bool up = 12 * part.octave + part.semi > 12 * p.parts.front().octave + p.parts.front().semi;
        d = 0.5f;
        return makeMacro (word, d, { { "layer" + juce::String (k + 1) + ".level", part.level, part.level * (up ? 1.3f : 2.0f), 0.0f, 2.0f } });
    }
    return {};
}

// the macro plan: role ids, best first (the first 4 applicable with distinct names win)
juce::StringArray macroPlan (const Info& in, const VaultPreset& p)
{
    const auto& t = in.treat;
    const Kind k = in.kind;
    juce::StringArray a;
    auto add = [&] (std::initializer_list<const char*> ids) { for (auto* s : ids) a.add (s); };
    if (in.layer.isNotEmpty())
    {
        a.add (isPianoLike (k) ? "hammer" : k == kHarpsi ? "quill" : isPipeLike (k) ? "expression" : k == kDrawbar ? "rotor" : "tone");
        a.add ("blend1");
        if (p.parts.size() > 2) a.add ("blend2");
        if (p.parts.size() > 3 && in.layer != "cinema" && in.layer != "heaven") a.add ("blend3");
        a.add ("space");
        add ({ "drift", "tremolo" });
        a.add (isPianoLike (k) ? "pedal" : "warmth");
        add ({ "chorus", "width", "tone" });
        return a;
    }
    if (t == "lofi" || t == "vinyl") add ({ "dust", "wobble", "tone", "space" });
    else if (t == "tape") add ({ "tape", "wobble", "tone", "space" });
    else if (t == "swell") add ({ "swell", "drift", "tone", "space" });
    else if (t == "underwater") add ({ "depth", "drift", "chorus", "space" });
    else if (t == "wooden") add ({ "body", "tone", "pedal", "space" });
    else if (t == "echopluck") add ({ "tone", "echo", "width", "space" });
    else if (t == "reverse") add ({ "reverse", "tone", "pedal", "space" });
    else if (t == "chorus") add ({ "chorus", "tremolo", "tone", "space" });
    else if (t == "phaser") add ({ "phase", "bark", "tone", "space" });
    else if (t == "drive") add ({ "drive", "tone", "tremolo", "rotor", "space" });
    else if (t == "tremolo") add ({ "tremolo", "rotor", "rate", "bark", "space" });
    else if (t == "tremulant") add ({ "tremulant", "expression", "tone", "space" });
    else if (t == "rotary") add ({ "rotor", "drive", "tone", "space" });
    else if (t == "distant") add ({ "tone", "pedal", "width", "space" });
    else if (t == "felt") add ({ "felt", "pedal", "tape", "space" });
    else if (t == "bright") add ({ "lid", "glue", "pedal", "space" });
    else if (t == "muted") add ({ "tone", "warmth", "echo", "space" });
    // natural / dark / intimate / round / bell: the family's own controls
    if (isPianoLike (k)) add ({ k == kDigital ? "tone" : "hammer", "pedal", t == "dark" ? "tape" : "warmth", "space" });
    else if (k == kTine || k == kReed) add ({ "bark", "tremolo", "tone", "space" });
    else if (k == kFmEp) add ({ "shine", "chorus", "echo", "space" });
    else if (isPipeLike (k)) add ({ "expression", "tremulant", "tone", "space" });
    else if (k == kDrawbar) add ({ "rotor", "drive", "tone", "space" });
    else if (k == kReedOrgan) add ({ "bellows", "tone", "musette", "space" });
    else if (k == kHarpsi) add ({ "quill", "warmth", "width", "space" });
    else if (k == kClav) add ({ "wah", "funk", "phase", "space" });
    else if (k == kFmKeys) add ({ "shine", "chorus", "echo", "space" });
    else if (k == kToy || k == kMusicBox) add ({ "tone", "windup", "echo", "space" });
    add ({ "tone", "width", "chorus", "echo", "warmth" });
    return a;
}

// absolute targets: the slot / LFO / layer value the macro gives at its default, written back so the FX page shows it
void bakeMacros (VaultPreset& p)
{
    for (int i = 0; i < 4; ++i)
        for (auto& t : p.macros[(size_t) i].targets)
        {
            const float v = t.min + (t.max - t.min) * p.macroDefaults[i];
            const auto& s = t.target;
            if (s.startsWith ("fx."))
            {
                const auto type = s.fromFirstOccurrenceOf (".", false, false).upToFirstOccurrenceOf (".", false, false);
                const auto par = s.fromLastOccurrenceOf (".", false, false);
                for (auto& f : p.fx) if (type == vault::fxTypeInfo (f.type).id) { f.set (par.toRawUTF8(), v); break; }
            }
            else if (s.startsWith ("lfo"))
            {
                const int k = s.substring (3, 4).getIntValue() - 1;
                if (k >= 0 && k < 2) { if (s.endsWith ("depth")) p.mod.lfo[k].depth = v; else if (s.endsWith ("rate")) p.mod.lfo[k].rate = v; }
            }
            else if (s == "finish") p.finish = v;
            else if (s.startsWith ("layer") && s.endsWith (".level"))
            {
                const int k = s.substring (5, 6).getIntValue() - 1;
                if (k >= 0 && k < (int) p.parts.size()) p.parts[(size_t) k].level = v;
            }
        }
}

// the whole v3 design of one preset (on the v2-stage preset: parts calibrated, name / id set)
// the family's polish that every preset of a voice would share (comp, tape, width, chorus, echo, default movement): it
// makes the voice's presets alike, so a preset that came out too close to an earlier one is designed without it
void stripFamily (Chain& c, Mod& m, float& finish, const Info& in)
{
    c.comp.reset(); c.drive.reset(); c.width.reset(); c.chorus.reset(); c.delay.reset();
    if (in.kind == kClav && c.filter) c.filter = fx::filter (fx::Filter::lowPass, 12000, 0.2f, 0.0f);
    for (auto* l : { &m.l1, &m.l2 }) if (l->has_value()) (*l)->depth = 0.0f;
    m.vel.clear(); m.mw.clear();
    finish = std::min (finish, 0.08f);
}
// the role's defining effects pushed further (a preset too close to an earlier one even without the family polish)
void exaggerate (Chain& c, Mod& m)
{
    auto sc = [] (std::optional<VaultFxSlot>& s, const char* par, float k, float hi) { if (s) s->set (par, std::min (hi, s->get (par) * k)); };
    sc (c.lofi, "crush", 1.5f, 0.5f); sc (c.lofi, "wow", 1.4f, 0.55f); sc (c.chorus, "mix", 1.4f, 0.8f); sc (c.phaser, "mix", 1.4f, 0.8f);
    sc (c.flanger, "mix", 1.4f, 0.8f); sc (c.drive, "amount", 1.4f, 0.6f); sc (c.reverb, "mix", 1.3f, 0.55f); sc (c.delay, "mix", 1.4f, 0.45f);
    sc (c.reverse, "mix", 1.3f, 0.8f); sc (c.body, "mix", 1.3f, 0.75f);
    if (c.reverb) c.reverb->set ("size", std::min (0.9f, c.reverb->get ("size") + 0.1f));
    if (c.width) c.width->set ("width", juce::jlimit (0.1f, 1.0f, 0.5f + (c.width->get ("width") - 0.5f) * 1.6f));
    if (c.filter && (int) c.filter->get ("mode") == 0 && c.filter->get ("cutoff") < 5000.0f) c.filter->set ("cutoff", c.filter->get ("cutoff") * 0.75f);
    for (auto* l : { &m.l1, &m.l2 }) if (l->has_value() && (*l)->depth > 0.0f) (*l)->depth = std::min (1.0f, (*l)->depth * 1.4f);
}

// level: 0 = the full design (family polish + role + space), 1 = without the family polish, 2 = that with the role pushed
// further, 3 = the neighbouring colour (reverb type / size, a quarter echo), 4 = light (space, EQ, gentle dynamics only).
// Levels 1-4 are only used for a preset that came out too close to an earlier one (or failed the gate).
void design (VaultPreset& p, const Info& in, int level, int pattern)
{
    const bool light = level >= 4;
    const int alt = level == 3 ? 1 : 0;
    recipe::design (p);   // converts the v2 FX keys (removed from the parts); the chain is then replaced
    p.mod = VaultMod(); p.arp = VaultArp(); p.gate = VaultGate();
    Chain c;
    Mod m;
    float finish = 0;
    familyFx (c, m, finish, in);
    if (level >= 1) stripFamily (c, m, finish, in);
    spaceFx (c, in.space, in);
    treatmentFx (c, m, finish, in);
    layerFx (c, m, in, p);
    if (level == 2) exaggerate (c, m);
    if (alt == 1)
    {
        if (c.reverb)
        {
            const int t = (int) c.reverb->get ("type");
            const bool organ = isPipeLike (in.kind) || in.kind == kReedOrgan;
            const int nt = t == (int) R::room ? (int) R::plate : t == (int) R::plate ? (int) R::room : t == (int) R::hall ? (organ ? (int) R::room : (int) R::cloud) : (int) R::hall;
            c.reverb->set ("type", (float) nt).set ("size", juce::jlimit (0.2f, 0.85f, c.reverb->get ("size") + (nt == (int) R::room ? -0.15f : 0.1f)))
                     .set ("mix", juce::jlimit (0.08f, 0.4f, c.reverb->get ("mix") * 1.2f));
        }
        if (in.treat == "natural" || in.treat == "dark" || in.treat == "round" || in.treat == "felt" || in.treat == "intimate")
        {
            c.chorus.reset();
            if (in.kind == kDrawbar) m.l1.reset();
        }
        if (! c.delay) c.delay = fx::delay ("1/4", 0.25f, 0.09f).set ("hiCut", 4500.0f).set ("loCut", 250.0f);
        if (c.width) c.width->set ("width", juce::jlimit (0.3f, 0.85f, c.width->get ("width") + 0.12f));
    }
    if (light)   // fallback when the full design cannot pass the gate: the space, EQ and gentle dynamics only
    {
        c.drive.reset(); c.lofi.reset(); c.transient.reset(); c.flanger.reset();
        if (c.chorus) c.chorus->set ("mix", std::min (0.25f, c.chorus->get ("mix")));
        if (c.reverb) c.reverb->set ("mix", c.reverb->get ("mix") * 0.8f);
        finish = std::min (finish, 0.1f);
    }
    p.finish = finish;
    // macros first (they may add the slot they need), then the chain
    MacroCtx x { p, c, m, in, {} };
    VaultMacro chosen[4];
    float defs[4] { 0.5f, 0.5f, 0.5f, 0.5f };
    int n = 0;
    juce::StringArray ids;
    for (auto& id : macroPlan (in, p))
    {
        if (n >= 4) break;
        if (ids.contains (id)) continue;
        float d = 0.5f;
        auto mac = macroFor (id, x, d);
        if (mac.name.isEmpty() || mac.targets.empty() || x.used.contains (mac.name)) continue;
        ids.add (id); x.used.add (mac.name);
        chosen[n] = mac; defs[n] = d; ++n;
    }
    // the space macro always sits on the fourth knob
    for (int i = 0; i < n - 1; ++i)
        if (ids[i] == "space") { std::swap (chosen[i], chosen[n - 1]); std::swap (defs[i], defs[n - 1]); ids.getReference (i).swapWith (ids.getReference (n - 1)); }
    p.fx = c.build();
    p.mod.lfo[0] = m.l1 ? *m.l1 : VaultLfo();
    p.mod.lfo[1] = m.l2 ? *m.l2 : VaultLfo();
    // mod wheel: vibrato-free sounds open the room, organs speed the rotor, EPs deepen the tremolo
    if (m.mw.empty())
    {
        if (m.l1 && (in.kind == kTine || in.kind == kReed)) m.mw.push_back ({ "lfo1.depth", 0.8f });
        else if (m.l1 && isPipeLike (in.kind)) m.mw.push_back ({ "lfo1.depth", 0.7f });
        else if (c.reverb) m.mw.push_back ({ "fx.reverb.mix", 0.25f });
    }
    p.mod.modWheel = m.mw; p.mod.velocity = m.vel; p.mod.aftertouch = m.at;
    recipe::Design dz (p);
    dz.clearMacros();
    for (int i = 0; i < n; ++i) dz.macro (i, chosen[i], defs[i]);
    bakeMacros (p);
    // arp / gate (a handful of presets: decided in recipe order by the caller, plain musical patterns)
    if (pattern == 1)
    {
        const bool hps = in.kind == kHarpsi;
        auto a = recipe::arp (hps ? VaultArp::upDown : VaultArp::up, hps ? "1/16" : in.kind == kFmKeys ? "1/8T" : "1/16", 2, hps ? 0.7f : 0.55f, 0.0f, true);
        for (int s = 0; s < 16; ++s) a.vel[(size_t) s] = s % 4 == 0 ? 1.0f : 0.78f;
        dz.arp (a);
        for (auto& f : p.fx) if (f.type == FT::delay) f.set ("mix", std::min (0.14f, f.get ("mix")));
        for (auto& mac : p.macros) for (auto& t : mac.targets) if (t.target == "fx.delay.mix") { t.min = std::min (t.min, 0.0f); t.max = std::min (t.max, 0.3f); }
        bakeMacros (p);
        for (auto& f : p.fx) if (f.type == FT::delay) f.set ("mix", std::min (0.14f, f.get ("mix")));
    }
    else if (pattern == 2)
        dz.gate (recipe::gate (in.layer == "dreamep" ? "1/8" : "1/16", { 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0 }, 0.85f, 0.3f, 1.0f, true));
}
// which presets get a pattern (1 = arp, 2 = gate) and the plain name that says so
int patternFor (const Info& in, juce::String& name)
{
    if (in.treat == "echopluck")
    {
        if (in.voice == "musicbox") { name = "Music Box Arpeggio"; return 1; }
        if (in.voice == "fmkeys") { name = "FM Keys Arpeggio"; return 1; }
        if (in.voice == "hpsBaroque") { name = "Harpsichord Arpeggio"; return 1; }
        if (in.voice == "celeste") { name = "Celeste Arpeggio"; return 1; }
    }
    if (in.treat == "swell" && in.voice == "fmep") { name = "Gated FM Piano"; return 2; }
    if (in.treat == "swell" && in.voice == "digital") { name = "Gated Digital Piano"; return 2; }
    if (in.layer == "dreamep") { name = "Gated Dreamwave"; return 2; }
    return 0;
}

//------------------------------------------------------------------------------ repairs on the v3 sound
VaultFxSlot* slotOf (VaultPreset& p, FT t) { for (auto& f : p.fx) if (f.type == t) return &f; return nullptr; }
// scale an FX parameter and every macro range on it
void scaleParam (VaultPreset& p, FT t, const char* par, float k, float lo = -1.0e9f, float hi = 1.0e9f)
{
    if (auto* s = slotOf (p, t)) s->set (par, juce::jlimit (lo, hi, s->get (par) * k));
    const auto id = juce::String ("fx.") + vault::fxTypeInfo (t).id + "." + par;
    for (auto& m : p.macros) for (auto& x : m.targets) if (x.target == id) { x.min = juce::jlimit (lo, hi, x.min * k); x.max = juce::jlimit (lo, hi, x.max * k); }
}
void offsetParam (VaultPreset& p, FT t, const char* par, float dv, float lo, float hi)
{
    if (slotOf (p, t) == nullptr)
    {
        if (t != FT::eq) return;
        recipe::design (p).addFx (fx::eq (0, 0, 0));
    }
    auto* s = slotOf (p, t);
    s->set (par, juce::jlimit (lo, hi, s->get (par) + dv));
    const auto id = juce::String ("fx.") + vault::fxTypeInfo (t).id + "." + par;
    for (auto& m : p.macros) for (auto& x : m.targets) if (x.target == id) { x.min = juce::jlimit (lo, hi, x.min + dv); x.max = juce::jlimit (lo, hi, x.max + dv); }
}
void scaleLfoDest (VaultPreset& p, const char* target, float k)
{
    for (auto& l : p.mod.lfo) for (auto& d : l.dests) if (d.target == target) d.amount *= k;
}

bool repair (VaultPreset& p, const juce::String& fail, int attempt)
{
    auto& a = p.parts.front();
    if (fail == "fundamental" || fail == "harsh-2-5k" || fail == "piercing-highs")
    {
        for (auto& part : p.parts) lowPass (part, std::min (currentLp (part), 8000.0f) * (fail == "fundamental" ? 0.6f : 0.7f));
        if (fail == "fundamental") offsetParam (p, FT::eq, "low", 2.0f, -18, 6);
        else offsetParam (p, FT::eq, "high", -2.0f, -9, 18);
        scaleParam (p, FT::drive, "amount", 0.6f);
        if (auto* d = slotOf (p, FT::drive)) d->set ("tone", std::min (d->get ("tone"), 6000.0f));
        p.finish *= 0.5f;
        return true;
    }
    if (fail == "velocity")
    {
        for (auto& part : p.parts) velSens (part, std::min (1.0f, getVelSens (part) + 0.25f));
        if (auto* c = slotOf (p, FT::compressor)) { c->set ("ratio", std::max (1.2f, c->get ("ratio") * 0.6f)); c->set ("mix", c->get ("mix") * 0.7f); }
        p.finish *= 0.6f;
        return attempt < 3;
    }
    if (fail == "click-start")
    {
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample) part.set ("s.attack", std::max (0.004f, part.get ("s.attack", 0.0f)));
        else if (part.kind == VaultPartKind::synth) part.set ("attack", std::max (0.006f, part.get ("attack", 0.002f)));
        if (auto* c = slotOf (p, FT::compressor)) c->set ("attack", std::max (c->get ("attack"), 15.0f));
        return attempt < 2;
    }
    if (fail == "click-end" && a.get ("s.releaseSamples", 1.0f) > 0.5f)
    {
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample) part.set ("s.releaseSamples", 0.0f);
        return true;
    }
    if (fail == "click-spike" && slotOf (p, FT::lofi) != nullptr && attempt < 3)
    {
        scaleParam (p, FT::lofi, "crush", 0.4f); scaleParam (p, FT::lofi, "wow", 0.4f);
        return true;
    }
    if (fail == "click-end" || fail == "click-spike")
    {
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample) part.set ("s.release", std::max (0.2f, part.get ("s.release", 0.0f)));
        else if (part.kind == VaultPartKind::synth) part.set ("release", std::max (0.2f, part.get ("release", 0.3f)));
        else part.set ("m.release", std::max (0.25f, part.get ("m.release", 0.3f)));
        return attempt < 3;
    }
    if (fail == "tail")
    {
        scaleParam (p, FT::reverb, "mix", 0.8f); scaleParam (p, FT::reverb, "decay", 0.7f, 0.25f, 4.0f); scaleParam (p, FT::reverb, "size", 0.85f);
        scaleParam (p, FT::delay, "feedback", 0.6f);
        for (auto& part : p.parts)
            if (part.kind == VaultPartKind::sample && part.get ("s.release", 0.0f) > 0.0f) part.set ("s.release", part.get ("s.release", 0.0f) * 0.6f);
            else if (part.kind == VaultPartKind::model) part.set ("m.release", std::min (1.0f, part.get ("m.release", 0.5f)));
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample && part.get ("s.release", 0.0f) <= 0.0f) part.set ("s.release", 2.5f);
        return true;
    }
    if (fail == "octave-layer")
    {
        for (size_t k = 1; k < p.parts.size(); ++k) if (p.parts[k].octave > a.octave) p.parts[k].level *= 0.6f;
        for (auto& m : p.macros) for (auto& t : m.targets) if (t.target.endsWith (".level")) { t.min *= 0.6f; t.max *= 0.6f; }
        return true;
    }
    if (fail == "low-side" || fail == "mono-loss")
    {
        if (slotOf (p, FT::width) == nullptr) recipe::design (p).addFx (fx::width (0.5f, 200));
        scaleParam (p, FT::width, "width", 0.7f, 0.0f, 1.0f);
        slotOf (p, FT::width)->set ("monoBelow", 220.0f);
        scaleParam (p, FT::chorus, "mix", 0.5f);
        scaleLfoDest (p, "pan", 0.5f);
        for (auto& part : p.parts) { part.pan *= 0.5f; if (part.kind == VaultPartKind::sample) part.set ("s.width", 0.6f); }
        return true;
    }
    if (fail == "too-peaky" || fail == "clip")
    {
        p.finish = std::max (p.finish, 0.45f);
        if (slotOf (p, FT::compressor) == nullptr) recipe::design (p).addFx (fx::comp (-14, 3, 1, 80));
        return attempt < 3;
    }
    return false;
}

//------------------------------------------------------------------------------ macro check: every macro at 0 and at 1
struct MacroCheck { int fixes = 0; juce::StringArray quiet, harsh, why; };
// raw (not normalised) envelope distance between two renders, RMS dB, plus the modulation features
float rawDistance (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    const std::vector<std::pair<double, double>> segs { { 0.0, 0.1 }, { 0.1, 0.5 }, { 0.5, 1.5 }, { 1.5, 2.5 }, { 2.5, 4.0 } };
    float m1 = 0, m2 = 0;
    const auto fa = mfccSegments (a, segs, 0.0f, false, m1), fb = mfccSegments (b, segs, 0.0f, false, m2);
    double s = 0;
    for (size_t i = 0; i < std::min (fa.size(), fb.size()); ++i) { const double d = fa[i] - fb[i]; s += d * d; }
    return (float) std::sqrt (s / ((double) segs.size() * ownMel));
}
struct Probe { float loud = -120, peak127 = 0, share25 = 0, share6k = 0, share8k = 0, h1 = 0, tailEnd = -200, tailMax = -200; juce::AudioBuffer<float> c5; };
Probe probe (const VaultPreset& p, const float* macros, double total)
{
    Probe r;
    juce::MidiBuffer mb;
    mb.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, (juce::uint8) 102), 0);
    mb.addEvent (juce::MidiMessage::noteOff (1, vault::homeNote), (int) (recipegate::holdSeconds * recipegate::sr));
    r.c5 = VaultEngine::renderMidi (p, mb, (int) (total * recipegate::sr), recipegate::sr, macros);
    const auto a = vaultqc::analyse (r.c5, recipegate::sr, recipegate::holdSeconds);
    r.loud = a.loud;
    const auto sp = recipegate::spectrum (r.c5, 0.05);
    const double tot = std::max (1e-20, recipegate::bandPower (sp, 20, 20000));
    r.share25 = (float) (recipegate::bandPower (sp, 2000, 5000) / tot);
    r.share6k = (float) (recipegate::bandPower (sp, 6000, 20000) / tot);
    r.share8k = (float) (recipegate::bandPower (sp, 8000, 20000) / tot);
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    const float h1 = recipegate::peakDb (sp, f0 * 0.97, f0 * 1.03);
    float strongest = h1;
    for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, recipegate::peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
    r.h1 = h1 - strongest;
    // the last 100 ms against the loudest 10 ms frame (tail still ringing at the end of the render?)
    const int fr = (int) (0.01 * recipegate::sr), nf = r.c5.getNumSamples() / fr;
    for (int f = 0; f < nf; ++f)
    {
        double acc = 0;
        for (int i = f * fr; i < (f + 1) * fr; ++i) acc += 0.5 * ((double) r.c5.getSample (0, i) * r.c5.getSample (0, i) + (double) r.c5.getSample (1, i) * r.c5.getSample (1, i));
        const float e = vaultaudit::db (acc / fr + 1e-20);
        r.tailMax = std::max (r.tailMax, e);
        if (f >= nf - 10) r.tailEnd = std::max (r.tailEnd, e);
    }
    juce::MidiBuffer hb;
    hb.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, (juce::uint8) 127), 0);
    hb.addEvent (juce::MidiMessage::noteOff (1, vault::homeNote), (int) (0.7 * recipegate::sr));
    const auto hard = VaultEngine::renderMidi (p, hb, (int) (0.8 * recipegate::sr), recipegate::sr, macros);
    r.peak127 = std::max (hard.getMagnitude (0, hard.getNumSamples()), r.c5.getMagnitude (0, r.c5.getNumSamples()));
    return r;
}
// what is wrong with one macro extreme (empty = fine); lin = the preset's final gain. Relative to the sound as designed
// (which passed the gate): a macro may not push it over a comfort limit it was under.
juce::String judge (const Probe& x, const Probe& ref, float lin, bool moved)
{
    if (x.peak127 * lin >= 0.885f && x.peak127 > ref.peak127 * 1.06f) return "clip";
    if (x.share25 > 0.10f && x.share25 > ref.share25 + 0.005f) return "harsh-2-5k";
    if ((x.share6k > 0.03f && x.share6k > ref.share6k + 0.002f) || (x.share8k > 0.012f && x.share8k > ref.share8k + 0.001f)) return "piercing-highs";
    if (x.h1 < -3.0f && x.h1 < ref.h1 - 0.5f) return "fundamental";
    if (x.loud > ref.loud + 4.0f) return "louder";
    if (moved && x.tailEnd > x.tailMax - 55.0f && x.tailEnd > ref.tailEnd + 3.0f) return "tail";
    return {};
}
// a macro whose LFO wobbles the pitch by >= 5 cents at 1 is audible by construction (the envelope measures are blind to it)
bool pitchWobble (const VaultPreset& p, const VaultMacro& m)
{
    for (auto& t : m.targets)
        if (t.target.startsWith ("lfo") && t.target.endsWith ("depth"))
        {
            const int k = t.target.substring (3, 4).getIntValue() - 1;
            if (k < 0 || k > 1) continue;
            for (auto& d : p.mod.lfo[k].dests) if (d.target == "pitch" && std::abs (d.amount) * std::abs (t.max - t.min) >= 0.05f) return true;
        }
    return false;
}
MacroCheck checkMacros (VaultPreset& p, float lin)
{
    MacroCheck mc;
    bool long_ = false;
    for (auto& m : p.macros) for (auto& t : m.targets) if (t.target.contains ("reverb") || t.target.contains ("delay") || t.target.contains ("release")) long_ = true;
    const double total = long_ ? 9.0 : 4.0;
    const auto ref = probe (p, p.macroDefaults, total);
    for (int i = 0; i < 4; ++i)
    {
        auto& mac = p.macros[(size_t) i];
        if (mac.targets.empty()) continue;
        Probe lo, hi;
        for (int round = 0; round < 5; ++round)
        {
            float mv[4];
            std::copy (std::begin (p.macroDefaults), std::end (p.macroDefaults), mv);
            mv[i] = 0.0f; lo = probe (p, mv, total);
            mv[i] = 1.0f; hi = probe (p, mv, total);
            const auto badLo = judge (lo, ref, lin, long_), badHi = judge (hi, ref, lin, long_);
            if (badLo.isEmpty() && badHi.isEmpty()) break;
            if (round == 4) { mc.harsh.add (mac.name + " (" + (badLo.isNotEmpty() ? badLo : badHi) + ")"); break; }
            // narrow the side that fails (round 4: take it away); the sound at the knob's default stays exactly as designed:
            // every target keeps its value v at the default, the default moves to where v now sits on the shorter range
            mc.why.add (mac.name + (badLo.isNotEmpty() ? " 0:" + badLo : juce::String()) + (badHi.isNotEmpty() ? " 1:" + badHi : juce::String()));
            const float k = round == 3 ? 0.0f : 0.55f;
            const float d = p.macroDefaults[i];
            const float kl = badLo.isNotEmpty() ? k : 1.0f, kh = badHi.isNotEmpty() ? k : 1.0f;
            const float nd = d * kl + (1 - d) * kh > 1.0e-6f ? d * kl / (d * kl + (1 - d) * kh) : d;
            for (auto& t : mac.targets)
            {
                const float v = t.min + (t.max - t.min) * d;
                const float newMin = v + (t.min - v) * kl, newMax = v + (t.max - v) * kh;
                t.min = newMin; t.max = newMax;
            }
            p.macroDefaults[i] = nd;
            ++mc.fixes;
        }
        const auto mlo = modulation (lo.c5), mhi = modulation (hi.c5);
        float md = 0;
        for (int k = 0; k < 3; ++k) md = std::max (md, std::abs (mlo[(size_t) k] - mhi[(size_t) k]));
        if (rawDistance (lo.c5, hi.c5) < 1.0f && md < 0.5f && std::abs (lo.loud - hi.loud) < 1.0f && ! pitchWobble (p, mac)) mc.quiet.add (mac.name);
    }
    return mc;
}
} // namespace v3
} // namespace

//==============================================================================
static std::vector<VaultPreset> buildV2 (const RecipeContext& ctx, std::vector<juce::String>& tags)
{
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
        tags.push_back (d.cand.recipe);
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
    // the v3 distinctness pass uses the bank assembly's threshold (VaultBankGen -dist / -wdist 3.5) on the shared fingerprint, so
    // a designed preset that would be dropped there as a near-duplicate is re-coloured here instead
    const float ownMin = envFloat ("KEYS_OWN_MIN", 2.6f), sharedMin = envFloat ("KEYS_V3_SHARED_MIN", 3.5f);
    const float ownMax25 = 0.08f, ownMax6k = 0.02f;
    const int threads = envInt ("KEYS_THREADS", (int) std::max (1u, std::thread::hardware_concurrency()));

    // 1. the v2 stage: voices x treatments x spaces + layer recipes, gated, distinct, named (KEYS_V2_CACHE=<dir> keeps its
    //    result between runs while the v3 design is tuned; delete the directory when the v2 stage or the library changes)
    std::vector<VaultPreset> base;
    std::vector<juce::String> tags;
    juce::File cacheBank, cacheTags;
    if (auto dir = juce::SystemStats::getEnvironmentVariable ("KEYS_V2_CACHE", {}); dir.isNotEmpty())
    {
        juce::File d (dir);
        d.createDirectory();
        cacheBank = d.getChildFile ("keys_v2.cvb"); cacheTags = d.getChildFile ("keys_v2_tags.tsv");
    }
    if (cacheBank.existsAsFile() && cacheTags.existsAsFile() && vault::loadBankFile (cacheBank, base))
    {
        std::map<juce::String, juce::String> tagOf;
        juce::StringArray lines;
        cacheTags.readLines (lines);
        for (auto& l : lines) if (l.contains ("\t")) tagOf[l.upToFirstOccurrenceOf ("\t", false, false)] = l.fromFirstOccurrenceOf ("\t", false, false);
        for (auto& p : base) tags.push_back (tagOf[p.id]);
        std::printf ("keys: v2 stage from the cache (%d presets)\n", (int) base.size());
    }
    else
    {
        base = buildV2 (ctx, tags);
        if (cacheBank != juce::File())
        {
            vault::saveBankFile (cacheBank, base);
            juce::String t;
            for (size_t i = 0; i < base.size(); ++i) t << base[i].id << "\t" << tags[i] << "\n";
            cacheTags.replaceWithText (t);
        }
    }
    if (envInt ("KEYS_V3", 1) == 0) return base;   // the v2 stage alone (comparison runs)
    if (const int lim = envInt ("KEYS_V3_LIMIT", 0); lim > 0 && lim < (int) base.size()) { base.resize ((size_t) lim); tags.resize ((size_t) lim); }

    // 2. the v3 design (finished FX chain, movement, 4 sound macros, a few arp / gate patterns), gated again
    const int n = (int) base.size();
    std::vector<v3::Info> info ((size_t) n);
    std::vector<int> pattern ((size_t) n, 0);
    {
        std::set<juce::String> names, patternNames;
        for (auto& p : base) names.insert (p.name.toLowerCase());
        for (int i = 0; i < n; ++i)
        {
            info[(size_t) i] = v3::parseInfo (tags[(size_t) i], base[(size_t) i]);
            juce::String nm;
            const int pt = v3::patternFor (info[(size_t) i], nm);
            if (pt != 0 && patternNames.insert (nm).second && names.insert (nm.toLowerCase()).second)
            {
                pattern[(size_t) i] = pt;
                names.erase (base[(size_t) i].name.toLowerCase());
                base[(size_t) i].name = nm;
            }
        }
    }
    struct Out
    {
        VaultPreset p;
        recipegate::Result res;
        std::vector<float> own;
        int repairs = 0, level = 0;   // v3::design levels 0-4, 5 = the v2 sound (classic macros)
        bool dropped = false;         // still a near-duplicate of an earlier preset after every colour
        v3::MacroCheck mc;
    };
    std::vector<Out> res ((size_t) n);
    // design + gate (+ repairs) + macro check + own fingerprint of preset i, starting at design level `from`
    auto processOne = [&] (int i, int from)
    {
        auto& o = res[(size_t) i];
        o = Out();
        const auto& in = info[(size_t) i];
        bool ok = false;
        for (int level = from; level < 6 && ! ok; ++level)
        {
            VaultPreset q = base[(size_t) i];
            if (level < 5) v3::design (q, in, level, pattern[(size_t) i]);
            else recipe::design (q);
            if (juce::String why; ! vault::validatePreset (q, &why)) { std::printf ("  keys v3 INVALID %s %s: %s\n", q.id.toRawUTF8(), q.name.toRawUTF8(), why.toRawUTF8()); continue; }
            for (int attempt = 0; attempt < 6; ++attempt)
            {
                o.res = recipegate::Result();
                o.res.preset = q;
                recipegate::analyse (o.res);
                if (o.res.fail.isEmpty() && o.res.share25 > ownMax25) o.res.fail = "harsh-2-5k";
                if (o.res.fail.isEmpty() && o.res.share6k > ownMax6k) o.res.fail = "piercing-highs";
                if (o.res.fail.isEmpty() || level == 5) break;
                if (! v3::repair (q, o.res.fail, attempt)) break;
                calibrate (q);
                ++o.repairs;
            }
            if (o.res.fail.isEmpty() || level == 5) { ok = true; o.level = level; o.p = q; }
        }
        if (o.res.fail.isEmpty())
        {
            if (o.level < 5) o.mc = v3::checkMacros (o.p, o.res.preset.gain);
            o.own = ownPrint (o.p);
        }
        o.p.gain = 1.0f;
    };
    auto runAll = [&] (const std::vector<int>& which, int from)
    {
        std::atomic<int> next { 0 }, count { 0 };
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t)
            pool.emplace_back ([&]
            {
                for (int k; (k = next++) < (int) which.size();)
                {
                    processOne (which[(size_t) k], from);
                    if (++count % 50 == 0) std::printf ("  keys v3 %d / %d\n", count.load(), (int) which.size());
                }
            });
        for (auto& t : pool) t.join();
    };
    // the presets (in recipe order) that are too close to an earlier kept one
    auto nearOnes = [&]
    {
        std::vector<int> kept, near;
        for (int i = 0; i < n; ++i)
        {
            auto& o = res[(size_t) i];
            if (o.res.fail.isNotEmpty() || o.dropped) continue;
            bool close = false;
            for (int j : kept)
                if (ownDistance (o.own, res[(size_t) j].own) < ownMin || recipegate::distance (o.res.print, res[(size_t) j].res.print) < sharedMin)
                {
                    close = true;
                    std::printf ("  keys v3 close: %s (level %d) ~ %s\n", o.p.name.toRawUTF8(), o.level, res[(size_t) j].p.name.toRawUTF8());
                    break;
                }
            if (close) near.push_back (i); else kept.push_back (i);
        }
        return near;
    };
    {
        std::vector<int> all ((size_t) n);
        for (int i = 0; i < n; ++i) all[(size_t) i] = i;
        runAll (all, 0);
    }
    // distinctness after the design: a preset that came out too close to an earlier one gets the neighbouring colour,
    // then the light design, then its v2 sound; still too close = dropped (as the v2 stage would have dropped it)
    for (int pass = 0; pass < 7; ++pass)
    {
        const auto near = nearOnes();
        std::printf ("keys v3: %d presets too close to an earlier one\n", (int) near.size());
        if (near.empty()) break;
        std::map<int, std::vector<int>> byFrom;   // next design level -> presets
        for (int i : near)
        {
            if (res[(size_t) i].level >= 5 || pass == 6) res[(size_t) i].dropped = true;
            else byFrom[res[(size_t) i].level + 1].push_back (i);
        }
        for (auto& [from, list] : byFrom) runAll (list, from);
    }

    // 3. report: distinctness after the design, processing styles, macro checks
    std::vector<VaultPreset> out;
    std::map<juce::String, int> fails, styles, macroNames, levels;
    int fixes = 0, patterns = 0;
    juce::StringArray quiet, harsh;
    std::map<int, std::pair<int, int>> slotsPerCat;   // category -> (presets, slots)
    for (int i = 0; i < n; ++i)
    {
        auto& o = res[(size_t) i];
        if (o.dropped) { ++fails["near-duplicate"]; std::printf ("  keys v3 DROPPED %s %s (near-duplicate)\n", o.p.id.toRawUTF8(), o.p.name.toRawUTF8()); continue; }
        if (o.res.fail.isNotEmpty()) { ++fails[o.res.fail]; std::printf ("  keys v3 FAILED %s %s: %s\n", o.p.id.toRawUTF8(), o.p.name.toRawUTF8(), o.res.fail.toRawUTF8()); continue; }
        static const char* levelNames[] { "full design", "without family polish", "role pushed further", "neighbouring colour", "light design", "v2 sound" };
        ++levels[levelNames[juce::jlimit (0, 5, o.level)]];
        fixes += o.mc.fixes;
        for (auto& q : o.mc.quiet) quiet.add (o.p.name + ": " + q);
        for (auto& h : o.mc.harsh) harsh.add (o.p.name + ": " + h);
        juce::StringArray chain;
        for (auto& f : o.p.fx) chain.add (vault::fxTypeInfo (f.type).id);
        ++styles[vault::categoryNames()[o.p.category] + " | " + info[(size_t) i].treat + (info[(size_t) i].layer.isNotEmpty() ? "+" + info[(size_t) i].layer : juce::String()) + " | " + chain.joinIntoString (" > ")];
        for (auto& m : o.p.macros) if (m.name.isNotEmpty()) ++macroNames[m.name];
        if (o.p.arp.on || o.p.gate.on) { ++patterns; std::printf ("  keys pattern: %s %s (%s)\n", o.p.id.toRawUTF8(), o.p.name.toRawUTF8(), o.p.arp.on ? "arp" : "gate"); }
        auto& sc = slotsPerCat[o.p.category]; ++sc.first; sc.second += (int) o.p.fx.size();
        out.push_back (o.p);
    }
    float minOwn = 1e9f, minShared = 1e9f;
    juce::String ownA, ownB, shA, shB;
    int underOwn = 0, underShared = 0;
    for (int x = 0; x < n; ++x)
        for (int y = x + 1; y < n; ++y)
        {
            auto& a = res[(size_t) x]; auto& b = res[(size_t) y];
            if (a.res.fail.isNotEmpty() || b.res.fail.isNotEmpty() || a.dropped || b.dropped) continue;
            const float o = ownDistance (a.own, b.own), s = recipegate::distance (a.res.print, b.res.print);
            if (o < ownMin) { ++underOwn; std::printf ("  keys v3 near (own %.2f): %s / %s\n", o, a.p.name.toRawUTF8(), b.p.name.toRawUTF8()); }
            if (s < sharedMin) { ++underShared; std::printf ("  keys v3 near (shared %.2f): %s / %s\n", s, a.p.name.toRawUTF8(), b.p.name.toRawUTF8()); }
            if (o < minOwn) { minOwn = o; ownA = a.p.name; ownB = b.p.name; }
            if (s < minShared) { minShared = s; shA = a.p.name; shB = b.p.name; }
        }
    std::printf ("keys v3: %d of %d presets designed", (int) out.size(), n);
    for (auto& [l, c] : levels) std::printf (", %s %d", l.toRawUTF8(), c);
    std::printf ("; %d with an arp / gate pattern\n", patterns);
    for (auto& [c, v] : slotsPerCat) std::printf ("  %-14s %d presets, %.2f FX slots on average\n", vault::categoryNames()[c].toRawUTF8(), v.first, (double) v.second / std::max (1, v.first));
    std::printf ("keys v3: min pairwise own distance %.2f (%s / %s), shared %.2f (%s / %s); pairs under own %.2f: %d, under shared %.2f: %d\n",
                 minOwn, ownA.toRawUTF8(), ownB.toRawUTF8(), minShared, shA.toRawUTF8(), shB.toRawUTF8(), ownMin, underOwn, sharedMin, underShared);
    std::printf ("keys v3: macro ranges narrowed %d times; %d macro extremes still out of the comfort rules; %d macros barely audible\n", fixes, harsh.size(), quiet.size());
    for (auto& h : harsh) std::printf ("  keys macro OUT %s\n", h.toRawUTF8());
    for (auto& q : quiet) std::printf ("  keys macro QUIET %s\n", q.toRawUTF8());
    for (auto& [f, c] : fails) std::printf ("  keys v3 rejected %-16s %d\n", f.toRawUTF8(), c);
    if (envInt ("KEYS_STYLES", 0) != 0)
        for (auto& [s, c] : styles) std::printf ("  style %3d  %s\n", c, s.toRawUTF8());
    {
        juce::String s;
        for (auto& [m, c] : macroNames) s << m << " " << c << ", ";
        std::printf ("keys v3 macro names: %s\n", s.toRawUTF8());
    }
    if (auto rp = juce::SystemStats::getEnvironmentVariable ("KEYS_REPORT3", {}); rp.isNotEmpty())
    {
        juce::String s ("id\tname\trecipe\tresult\tlevel\trepairs\tchain\tmacros\tlfo1\tlfo2\tH1\t2-5k%\t>6k%\t>8k%\ttail\tvel\tgainDb\tmacroFixes\tnarrowed\tquiet\n");
        for (int i = 0; i < n; ++i)
        {
            auto& o = res[(size_t) i];
            juce::StringArray chain, macs;
            for (auto& f : o.p.fx) chain.add (vault::fxTypeInfo (f.type).id);
            for (int m = 0; m < 4; ++m) if (o.p.macros[(size_t) m].name.isNotEmpty()) macs.add (o.p.macros[(size_t) m].name + "@" + juce::String (o.p.macroDefaults[m], 2));
            auto lfoText = [] (const VaultLfo& l) { juce::StringArray d; for (auto& x : l.dests) d.add (x.target); return d.isEmpty() ? juce::String ("-") : juce::String (l.rate, 2) + "Hz x" + juce::String (l.depth, 2) + " " + d.joinIntoString ("+"); };
            s << o.p.id << "\t" << o.p.name << "\t" << tags[(size_t) i] << "\t" << (o.dropped ? juce::String ("near-duplicate") : o.res.fail.isEmpty() ? juce::String ("ok") : o.res.fail) << "\t" << o.level << "\t" << o.repairs << "\t"
              << chain.joinIntoString (">") << "\t" << macs.joinIntoString (", ") << "\t" << lfoText (o.p.mod.lfo[0]) << "\t" << lfoText (o.p.mod.lfo[1]) << "\t"
              << juce::String (o.res.h1Db, 1) << "\t" << juce::String (100 * o.res.share25, 2) << "\t" << juce::String (100 * o.res.share6k, 2) << "\t" << juce::String (100 * o.res.share8k, 2) << "\t"
              << juce::String (o.res.tail, 2) << "\t" << juce::String (o.res.velRange, 1) << "\t" << juce::String (o.res.gainDb, 1) << "\t" << o.mc.fixes << "\t" << o.mc.why.joinIntoString ("; ") << "\t" << o.mc.quiet.joinIntoString (", ") << "\n";
        }
        juce::File (rp).replaceWithText (s);
        std::printf ("keys: v3 report %s\n", rp.toRawUTF8());
    }
    return out;
}

VAULT_RECIPE (keys, build_keys);
