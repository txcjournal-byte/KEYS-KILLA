// CHROME VAULT - sound design for the categories "Plucks", "Leads", "Pads", "Synth" and "FX & Atmos" (VaultBankGen -recipe synth).
//
// v3 DESIGN NOTE - every preset ships finished (namespace v3 below)
//  * Research (production guides, preset-bank walkthroughs, forum practice): commercial synth banks ship every patch through
//    its own insert chain and the player switches pieces off. Plucks: glue compression, a little tape, a tempo-synced
//    ping-pong (1/8 dotted) with high-passed / high-cut repeats, then a plate after the delay so the echoes sit in the tail,
//    low-cut on the reverb, width with mono lows. Leads: saturation for density (lows kept clean), the 3 kHz edge cut, delay
//    throws (1/4, 1/8 dotted) instead of big rooms, a small plate, delayed faded-in vibrato on the mod wheel, short glide.
//    Pads: two-line chorus / ensemble, long warm hall / cloud / shimmer with pre-delay, slow (0.05-0.2 Hz) filter and pan
//    drift, a quiet tape delay, the lows left to the bass. Poly synths: the classic bucket-brigade chorus, glue, 1/8 dotted,
//    plate; stabs: punch (transient + compression) and a short room; string machines: ensemble + slow phaser; organs:
//    tube drive and a rotor-like sweep. FX: risers / downlifters with ping-pong and phaser motion, drones with slow phaser /
//    filter drift in clouds, impacts with glue, low shelf and a long hall (tail length = size of the space), reversed
//    instruments with tape delay.
//  * Chain = FAMILY (what the instrument needs, by category and voice: real / mallet / glass / string / chip / colour / analog
//    plucks; voice / dist / soft / chip / digital / analog leads; strings / choir / organ / winds / shine / vocal / machine /
//    air / warm pads; stab / brass / keys / organ / strings / rhythm / poly synths; riser / drone / hit / reversed / swell /
//    texture atmospheres) + the v2 SPACE rebuilt with the v3 reverb types (dry / room = room, plate, hall, cloud, cathedral,
//    vast; echo = tape 1/8D, ping = ping-pong 1/8, slap = 110 ms, dub = tape 1/4D) + the v2 TREATMENT as a processing role
//    (lo-fi = tape drive + wow LFO + small room; wide = ensemble + width; muted / dark = 4 kHz top shelf, damped reverb;
//    short / stab = short tail; long / swell = longer tail; airy = long pre-delay; underwater = slow cutoff swirl; drive =
//    clean lows, darker tone). The v2 colours (drive, lo-fi, chorus, reverse, body ...) and the v2 repair EQ are kept.
//    Slots the main page needs but the sound does not use are in the chain switched off (strings / organ pad echo, brass /
//    organ synth echo, impact echo); every chain <= 8 slots, canonical order.
//  * Movement: leads LFO 1 = vibrato (5.4 Hz, 0.35 s delay, 0.6 s fade; 0 where the oscillator has its own), LFO 2 a slow
//    cutoff drift on synth leads; pads LFO 1 = slow cutoff + pan drift (sample pads + EQ tilt), LFO 2 = a slower pan /
//    breathing triangle; synth plucks a slow cutoff / pan drift (samples: pan only); keys a tremolo ready at 0; drones /
//    textures a slow drift; lo-fi a tape wobble on pitch. Mod wheel: leads vibrato, pads / atmospheres motion depth, keys
//    tremolo, else +1.2 octaves of cutoff. Velocity keeps its range through compressors (velocity -> volume).
//  * Sound macros (each target exactly at its designed value at the knob's default, so a preset loads as designed):
//      Plucks  Cutoff / Brightness (filters + tilt), Room / Plate / Hall / Haze (reverb mix, decay, size), Echo (delay mix,
//              feedback), Snap (transient + filter-envelope amount) or the role's knob
//      Leads   Cutoff, Drive / Fuzz (drive with output trim) or the added layer, Echo, Vibrato
//      Pads    Brightness, the space, Motion (LFO depth + phaser), Swell (attack / release) or the role's knob / added layer
//      Synth   Cutoff, the space, Chorus / Ensemble / Rotor or Echo, then Punch / Tremolo / Swell / Drive / Phaser / Echo
//      FX      Tone, the space, Motion, then Grit / Crush / Boom / Echo
//    role knobs: Tape (lo-fi), Depth (underwater), Reverse, Width (wide), Grit (drive), Repeats (echo spaces), Shade (muted /
//    dark), Gate / Note Length (rhythm presets); layered presets name the knob after the added layer (Harp, Sub, Strings,
//    Choir, Piano, ...: its level 0 .. 2 x). Every macro is rendered at 0 and 1 (C5) and a side that breaks a comfort rule
//    (clip, 2-5 kHz, > 6 / 8 kHz, fundamental, +4 dB louder, a longer tail) is shortened until it passes.
//  * Arp / gate: off everywhere except 6 rhythm presets with plain names and new ids after the shipped ones: Saw Pluck
//    Arpeggio (Up 1/16, 2 octaves), Glass Pluck Arpeggio (Up-Down 1/16), Soft Pluck Arpeggio (Down 1/8), Poly Saw
//    Chord Pulse (Chord 1/8 with accents), Gated Warm Pad (1/16 gate), Gated String Machine (1/16 gate). Patterns start from Init.
//  * Selection: the shipped v2 presets are rebuilt from a pinned table (id | recipe tag | name, found with SYNTH_PINMAP from
//    the shipped v2 bank: same candidates, calibration and repairs, verified identical), designed, re-fitted to this file's
//    comfort caps (2-5 kHz <= 7.5 %, > 6 kHz <= 1.8 %, > 8 kHz <= 0.6 %, centroid <= 0.9 x the category ceiling, fundamental
//    >= -2 dB), then gated in id order: the shared gate + the assembly QA + a shared fingerprint >= 3.5 (the assembly's
//    threshold; rhythm presets 3.0) from every preset kept before. One that fails is designed again (1: the v2 space with the
//    family's tone, 2: the v2 colours + movement, 3: the v2 sound with only the macros) before it is dropped. Chord headroom
//    is left to the assembly (it gives such a preset its peak control, as in v2). SYNTH_V2_CACHE=<dir> keeps the v2 stage between runs,
//    SYNTH_ONLY=<ids> designs only those, SYNTH_DUMP=<file> lists every chain / LFO / macro, SYNTH_V2_SEARCH=1 runs the v2
//    search (new material), SYNTH_V2ONLY=1 + SYNTH_VERIFY=<v2 bank> checks the rebuilt v2 stage.
//  * Result: VaultBankGen -recipe synth (library /home/user/cv-library): the 1158 shipped presets rebuilt (the pinned v2
//    stage is identical to the shipped v2 bank apart from the assembly's own peak-control keys) + 6 rhythm presets -> 1155
//    pass the gate (Plucks 274, Leads 191, Pads 216, Synth 255, FX & Atmos 219); 1149 of the 1158 shipped ids stay with
//    their names (about 960 with the design, 110 with the v2 space + family tone, 65 with the v2 colours, 3 with the v2
//    sound; 16 of them kept by the pair pass); dropped: 4 near-duplicates (Chip Lead, Echo Glass Lead, Organ Synth, Hall
//    Reed Synth), 3 tails (Gale Reversed Choir, Echoes Sync Rise, Memory Glitch Field), 1 velocity (Harmon Distorted Lead),
//    1 octave layer (Shine Fuzz Sine). Min pairwise distance 3.50. At C5: fundamental >= -2.7 dB (not FX), 2-5 kHz
//    <= 8.5 %, > 6 kHz <= 0.93 %, > 8 kHz <= 0.34 %; 49 presets needed a comfort re-fit after the FX. 4,620 macros
//    rendered at 0 and 1: 1,553 range narrowings, none left outside the comfort rules, 203 change the sound only slightly
//    (mostly Echo / Haze / Shimmer on long pads). Chains: 5 slots typical (1 .. 8).
//  * Sources: waves.com studioverse "ping pong delay" chains; audiotent.com production tips (delay as reverb on plucks);
//    kvraudio forum "What effect plugins do you always put on your synth sounds?"; edmprod.com "Synth Pads: 9 ways";
//    unison.audio synth pads; musicradar "Create the perfect synth pad from scratch"; violetrecording.com "How to design a
//    lead synth sound"; soundbridge.io "Spice up your synth leads with vibrato"; attackmagazine.com Juno-60 technique
//    (chorus depth, room bloom); blog.native-instruments.com "Sound in film" (risers, impacts, drones);
//    sounddesign.irpr.agency glossary (riser, impact).
//
// v2 DESIGN NOTE (the v2 stage below: voices, treatments, spaces, layers, gate, distinctness, names)
//
// DESIGN NOTE (research summary - how preset / sample libraries voice these sounds, and what this recipe copies)
// --------------------------------------------------------------------------------------------------------------
// * Plucks are a family of DIFFERENT excitation / resonator models, not one patch with jitter: the subtractive pluck
//   (bright oscillator + fast-closing low-pass envelope, a little resonance), the Karplus-Strong string (noise burst
//   into a damped delay loop: the loop filter sets how fast the highs die, the burst filter the hardness - Karplus &
//   Strong 1983, Jaffe & Smith 1983), the FM pluck (the MODULATOR is enveloped: a short index decay gives the bright,
//   inharmonic strike and a mellow ring - integer ratios = harmonic glass / wood / tine, non-integer = metal), modal
//   bars (marimba / kalimba / bell mode ratios) and wavetable / sync plucks. Trap and pluggnb producers play them
//   low-passed (room for the vocal and the 808), wide, soft-attacked, with synced 1/8-dotted echoes, tape wow and a
//   short dark reverb; real pizzicato / harp / mallet transients are layered under synth plucks for a "hybrid" body.
// * Leads sit mainly in 200 Hz - 2 kHz like a vocal: one strong voice, mono + legato glide (trap leads use generous
//   portamento), delayed vibrato, a dip-in scoop, square = hollow / reedy, narrow pulse = thin trap lead, sine +
//   breath = whistle, drive for the distorted lead but always low-passed (the 2-5 kHz region is where it hurts), delay
//   throws instead of big reverbs. Octave-down doubles add weight; octave-up doubles are kept >= 12 dB down.
// * Pads: long attack / release so chords bleed together, slow asynchronous modulation (filter LFO, vowel / wave
//   drift, a second LFO at another rate), a detuned second oscillator or unison for width, long reverb (high-passed
//   in the mix) plus some tape delay; layers at different octaves for depth. Libraries also make pads from REAL
//   recordings: bowed string sections with slow attacks in a hall, choir "aahs" / "oohs" with a sine / warm layer to
//   carry the fundamental, organ flue stops, glass harp, bowed vibraphone, flutes - played with long releases and
//   cloud / cathedral spaces, sometimes reversed or tape-degraded.
// * Synth (poly synths / stabs / keys): poly saw with chorus, synth brass (slow filter attack), unison / supersaw stabs,
//   square chord stabs, FM / digital keys, house organ, formant stabs, synth strings, gated / pumping chords and
//   tempo-locked two-step sequences; hybrids layer a real piano, pizzicato or horn staccato under a synth.
// * FX & Atmos: risers are tonal sweeps (pitch + filter) and noise swells that grow in brightness, often stacked;
//   downlifters / falls, impacts with huge tails, drones and textures (slow evolving, detuned, lo-fi), wind, sci-fi
//   random-pitch chatter, and reversed real instruments (reverse swells run backwards into the downbeat).
//
// HOW THE BANK IS MADE
// * Voices = distinct generators per category (synth engine architectures: VA saw / square / pulse / supersaw, Karplus-
//   Strong, FM, modal bars, wavetable frames, sync, formant, organ, flute / breath, orchestral; library recordings for
//   real-sample pads, hybrids and reversed FX; physical models where they fit). Treatments are processing ROLES
//   (short, ringing, muted, echo, lo-fi, tape, wide, scooped, octave body, fifth, reversed, resonant body, drive, ...)
//   and spaces (dry .. cathedral). Layer recipes add a second / third / fourth instrument that changes the sound.
// * Every part is calibrated alone (its `gain` puts it at -20 dB K-weighted at C5) so layer levels mean the same thing.
// * Each candidate runs the shared v2 gate (RecipeGate.h, C5 = MIDI 60) with automatic repairs, plus this file's
//   stricter comfort caps (2-5 kHz <= 8 %, > 6 kHz <= 2 %), then TWO distinctness checks against everything accepted
//   before it: the shared log-mel / harmonic / motion fingerprint (>= sharedMin) and this file's own MFCC fingerprint
//   (C5 vel 0.8 incl. the release tail, C5 vel 0.35, C4, C6, stereo, modulation) (>= ownMin). Near-duplicates are
//   dropped, never renamed. Names: evocative English, no brands, no "808"; part labels are instrument names.
// * Candidate order: every voice as designed first, then treated voices and layer recipes round robin (2 : 1), so the
//   greedy distinctness pass keeps the plain instruments and spreads the rest evenly over all voices. Plucks and Synth
//   are capped (300 / 270, SYNTH_CAP_PLUCKS / SYNTH_CAP_SYNTH) to keep the five categories balanced.
// * Sample parts must play at C4, C5 and C6 (FX & Atmos: at C5) - a pad silent below its instrument's range is broken.
//   Recorded dynamics are kept but limited: velocity 0.35 -> 1.0 may open at most 20 dB.
//
// Environment (optional): SYNTH_THREADS, SYNTH_LIMIT (first N candidates), SYNTH_CATS (comma list of category indices),
// SYNTH_REPORT (TSV report path), SYNTH_OWN_MIN / SYNTH_SHARED_MIN (thresholds).
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
using namespace vault;

//==============================================================================
// part makers
void setAll (VaultPart& p, const Values& v) { for (auto& [k, x] : v) p.set (k, x); }

// a synth part: neutral FX / hidden macros first (EVOLVE defaults add reverb, delay, chorus, vibrato and MASTER glue),
// every engine knob that has a non-neutral default set explicitly (fmAmt is FM index / warp / vowel / pitch drop)
VaultPart Y (const char* label, const Values& v)
{
    auto p = recipe::synthPart (label);
    setAll (p, { { "engine", 0 }, { "wave", 0 }, { "unison", 1 }, { "detune", 0.15f }, { "fmAmt", 0 }, { "fmRatio", 1 }, { "fmRatio2", 1 },
                 { "fmAlgo", 0 }, { "warpMode", 0 }, { "levelA", 1 }, { "layerB", 0 }, { "sub", 0 }, { "filterType", 0 }, { "cutoff", 12000 },
                 { "reso", 0.05f }, { "keyTrack", 0.5f }, { "fenv", 0 }, { "attack", 0.003f }, { "decay", 0.6f }, { "sustain", 0.8f },
                 { "release", 0.3f }, { "velSens", 0.65f }, { "lfoPitch", 0 }, { "lfoFilter", 0 }, { "lfoAmp", 0 },
                 { "macro2", 0 }, { "macro5", 0 }, { "revMix", 0 }, { "delayMix", 0 }, { "master", 0 }, { "chorus", 0 } });
    setAll (p, v);
    return p;
}
// layer B of a synth part (inside one part: one more oscillator through the same filter / envelope)
Values B (int engine, float wave, int octave, int semi, float level, Values extra = {})
{
    Values v { { "layerB", 1 }, { "engineB", (float) engine }, { "waveB", wave }, { "octaveB", (float) octave }, { "semiB", (float) semi },
               { "levelB", level }, { "fmAmtB", 0 }, { "unisonB", 1 }, { "fineB", 0 } };
    for (auto& e : extra) v.push_back (e);
    return v;
}
Values operator+ (Values a, const Values& b) { for (auto& x : b) a.push_back (x); return a; }
// a mod-matrix route in slot s (0..7): src 1 env2, 2 env3, 3 lfo1, 4 lfo2, 5 vel, 9 random; dst 0 pitch, 1 cutoff,
// 2 reso, 3 wave A, 4 wave B, 5 FM / warp A, 7 level A, 8 level B, 9 amp, 10 pan, 12 detune
Values MM (int slot, int src, int dst, float amt)
{
    const juce::String n (slot + 1);
    return { { "mmSrc" + n, (float) src }, { "mmDst" + n, (float) dst }, { "mmAmt" + n, amt } };
}
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
float getv (const VaultPart& p, const char* k, float def) { return p.get (k, def); }
void setv (VaultPart& p, const char* k, float v) { p.set (k, v); }
bool isSynth (const VaultPart& p) { return p.kind == VaultPartKind::synth; }
bool isSample (const VaultPart& p) { return p.kind == VaultPartKind::sample; }

// brightness in Hz, whatever the part kind
void lowPass (VaultPart& p, float hz)
{
    if (isSample (p)) setv (p, "s.cutoff", juce::jlimit (250.0f, 20000.0f, std::min (hz, getv (p, "s.cutoff", 20000.0f))));
    else if (p.kind == VaultPartKind::model)
    {
        const float t = juce::jlimit (0.0f, 1.0f, 0.5f + 0.5f * std::log2 (hz / 6000.0f));
        setv (p, "m.tone", std::min (t, getv (p, "m.tone", 0.5f)));
    }
    else setv (p, "cutoff", juce::jlimit (120.0f, 20000.0f, std::min (hz, getv (p, "cutoff", 12000.0f))));
}
void scaleCut (VaultPart& p, float k)
{
    if (isSample (p)) setv (p, "s.cutoff", juce::jlimit (250.0f, 20000.0f, std::min (12000.0f, getv (p, "s.cutoff", 20000.0f)) * k));
    else if (p.kind == VaultPartKind::model) setv (p, "m.tone", juce::jlimit (0.0f, 1.0f, getv (p, "m.tone", 0.5f) + 0.5f * std::log2 (k)));
    else setv (p, "cutoff", juce::jlimit (120.0f, 20000.0f, getv (p, "cutoff", 12000.0f) * k));
}
float currentLp (const VaultPart& p)
{
    if (isSample (p)) return getv (p, "s.cutoff", 20000.0f);
    if (p.kind == VaultPartKind::model) return 6000.0f * std::exp2 ((getv (p, "m.tone", 0.5f) - 0.5f) * 2.0f);
    return getv (p, "cutoff", 12000.0f);
}
void velSens (VaultPart& p, float v)
{
    if (isSample (p)) setv (p, "s.velSens", v);
    else if (p.kind == VaultPartKind::model) setv (p, "m.velSens", v);
    else setv (p, "velSens", v);
}
float getVelSens (const VaultPart& p)
{
    if (isSample (p)) return getv (p, "s.velSens", 0.5f);
    if (p.kind == VaultPartKind::model) return getv (p, "m.velSens", 0.75f);
    return getv (p, "velSens", 0.65f);
}
void attack (VaultPart& p, float s)
{
    if (isSample (p)) setv (p, "s.attack", s);
    else if (p.kind == VaultPartKind::model) { if (p.source.startsWith ("voice")) setv (p, "m.attack", s); }
    else setv (p, "attack", s);
}
void release (VaultPart& p, float s)
{
    if (isSample (p)) setv (p, "s.release", s);
    else if (p.kind == VaultPartKind::model) setv (p, "m.release", juce::jlimit (0.02f, 4.0f, s));
    else setv (p, "release", juce::jlimit (0.005f, 8.0f, s));
}
// shorten / lengthen a decaying sound (synth: amp + filter decay, KS / modal string decay; sample: ADSR override)
void decayScale (VaultPart& p, float k)
{
    if (isSynth (p))
    {
        setv (p, "decay", juce::jlimit (0.03f, 8.0f, getv (p, "decay", 0.6f) * k));
        setv (p, "fdecay", juce::jlimit (0.01f, 4.0f, getv (p, "fdecay", 0.4f) * std::sqrt (k)));
        setv (p, "release", juce::jlimit (0.03f, 8.0f, getv (p, "release", 0.3f) * k));
    }
    else if (isSample (p))
    {
        if (k < 1.0f) { setv (p, "s.decay", 0.5f * k + 0.08f); setv (p, "s.sustain", 0.0f); setv (p, "s.release", 0.15f); }
        else setv (p, "s.release", 1.2f * k);
    }
    else
    {
        setv (p, "m.decay", juce::jlimit (0.2f, 4.0f, getv (p, "m.decay", 1.0f) * k));
        setv (p, "m.release", juce::jlimit (0.03f, 3.0f, getv (p, "m.release", 0.3f) * k));
    }
}
juce::String joinWords (std::initializer_list<juce::String> w)
{
    juce::StringArray a;
    for (auto& s : w) if (s.trim().isNotEmpty()) a.add (s.trim());
    return a.joinIntoString (" ");
}

//==============================================================================
// spaces (FX of part 0: the engine reads the master FX from part 0 only)
struct Space { const char* tag; const char* word; Values fx; };
const Space& space (const char* tag)
{
    static const std::vector<Space> s {
        { "dry",       "Close",     { { "revMix", 0.06f }, { "revType", 1 }, { "revSize", 0.2f } } },
        { "room",      "Room",      { { "revMix", 0.14f }, { "revType", 1 }, { "revSize", 0.34f } } },
        { "plate",     "Plate",     { { "revMix", 0.22f }, { "revType", 1 }, { "revSize", 0.55f } } },
        { "hall",      "Hall",      { { "revMix", 0.26f }, { "revType", 0 }, { "revSize", 0.66f } } },
        { "cloud",     "Cloud",     { { "revMix", 0.36f }, { "revType", 2 }, { "revSize", 0.7f } } },
        { "cathedral", "Cathedral", { { "revMix", 0.4f },  { "revType", 0 }, { "revSize", 0.9f } } },
        { "vast",      "Infinite",  { { "revMix", 0.5f },  { "revType", 2 }, { "revSize", 0.93f } } },
        { "echo",      "Echo",      { { "delayMix", 0.2f }, { "delayTime", 2 }, { "delayFb", 0.36f }, { "delayMode", 2 }, { "revMix", 0.12f }, { "revType", 1 }, { "revSize", 0.4f } } },
        { "ping",      "Delay",     { { "delayMix", 0.22f }, { "delayTime", 0 }, { "delayFb", 0.3f }, { "delayMode", 0 }, { "revMix", 0.15f }, { "revType", 0 }, { "revSize", 0.5f } } },
        { "slap",      "Slap",      { { "delayMix", 0.16f }, { "delayTime", 3 }, { "delayFb", 0.12f }, { "delayMode", 1 }, { "revMix", 0.1f }, { "revType", 1 }, { "revSize", 0.3f } } },
        { "dub",       "Dub",       { { "delayMix", 0.28f }, { "delayTime", 2 }, { "delayFb", 0.5f }, { "delayMode", 2 }, { "revMix", 0.2f }, { "revType", 2 }, { "revSize", 0.6f } } } };
    for (auto& x : s) if (juce::String (x.tag) == tag) return x;
    jassertfalse;
    return s.front();
}
void fx (VaultPreset& p, const Values& v) { setAll (p.parts.front(), v); }

//==============================================================================
// voices: the main instrument of a preset (part 0). `kind` = generator family (treatments / layers pick by kind).
enum Kind : uint32_t
{
    kSaw = 1u << 0, kSquare = 1u << 1, kPulse = 1u << 2, kSuper = 1u << 3, kString = 1u << 4, kFM = 1u << 5, kModal = 1u << 6,
    kWave = 1u << 7, kVox = 1u << 8, kOrgan = 1u << 9, kBreath = 1u << 10, kOrch = 1u << 11, kSine = 1u << 12, kNoise = 1u << 13,
    kSampleSus = 1u << 14, kSampleShot = 1u << 15, kModel = 1u << 16, kRiser = 1u << 17, kDrone = 1u << 18, kHit = 1u << 19
};
constexpr uint32_t kAnalog = kSaw | kSquare | kPulse | kSuper;
constexpr uint32_t kSynthAll = kAnalog | kString | kFM | kModal | kWave | kVox | kOrgan | kBreath | kOrch | kSine;
constexpr uint32_t kSamples = kSampleSus | kSampleShot;
constexpr uint32_t kAll = 0xffffffffu;

struct Voice
{
    const char* tag;
    int category;
    uint32_t kind;
    VaultPart part;
    juce::StringArray nouns;      // name nouns, best first
};

// common pluck envelope: fast attack, no sustain, filter envelope from `cut` (Hz) by `fenv` octaves x6
Values pluckEnv (float cut, float fenvAmt, float fdec, float dec, float rel = 0.35f)
{
    return { { "cutoff", cut }, { "fenv", fenvAmt }, { "fattack", 0.001f }, { "fdecay", fdec }, { "fsustain", 0 }, { "frelease", rel },
             { "attack", 0.003f }, { "decay", dec }, { "sustain", 0 }, { "release", rel } };
}
// lead: mono legato with glide, delayed vibrato
Values leadEnv (float cut, float glide = 0.1f)
{
    return { { "cutoff", cut }, { "mono", 1 }, { "legato", 1 }, { "glide", glide }, { "attack", 0.006f }, { "decay", 0.5f }, { "sustain", 0.85f },
             { "release", 0.18f }, { "lfoRate", 5.4f }, { "lfoPitch", 0.05f }, { "bendRange", 2 } };
}
Values padEnv (float cut, float att = 0.7f, float rel = 2.2f)
{
    return { { "cutoff", cut }, { "attack", att }, { "decay", 1.5f }, { "sustain", 0.9f }, { "release", rel }, { "keyTrack", 0.45f },
             { "lfoRate", 0.22f }, { "lfoFilter", 0.06f }, { "velSens", 0.6f } };
}
Values polyEnv (float cut, float dec = 0.9f, float sus = 0.6f, float rel = 0.4f)
{
    return { { "cutoff", cut }, { "attack", 0.004f }, { "decay", dec }, { "sustain", sus }, { "release", rel }, { "fenv", 0.25f }, { "fdecay", 0.5f },
             { "fsustain", 0.2f } };
}
constexpr float wSine = 0.0f, wTri = 1.0f / 7, wSaw = 2.0f / 7, wSq = 3.0f / 7, wPulse = 4.0f / 7, wGlass = 5.0f / 7, wDigi = 6.0f / 7;

std::vector<Voice> voices()
{
    std::vector<Voice> v;
    auto add = [&] (const char* tag, int cat, uint32_t k, VaultPart p, juce::StringArray nouns) { v.push_back ({ tag, cat, k, std::move (p), std::move (nouns) }); };
    //------------------------------------------------------------------ PLUCKS
    add ("sawpluck", catPlucks, kSaw, Y ("Saw Pluck", pluckEnv (1400, 0.5f, 0.26f, 0.9f) + Values { { "reso", 0.15f }, { "keyTrack", 0.6f } }),
         { "Saw Pluck", "Analog Pluck", "Filter Pluck" });
    add ("sqpluck", catPlucks, kSquare, Y ("Square Pluck", pluckEnv (1500, 0.42f, 0.2f, 0.75f) + Values { { "wave", 0.5f } }),
         { "Square Pluck", "Box Pluck", "Hollow Pluck" });
    add ("pulsepluck", catPlucks, kPulse, Y ("Pulse Pluck", pluckEnv (1800, 0.35f, 0.18f, 0.6f) + Values { { "wave", 0.85f } }),
         { "Reed Pluck", "Pulse Pluck", "Nasal Pluck" });
    add ("superpluck", catPlucks, kSuper, Y ("Supersaw Pluck", pluckEnv (1500, 0.45f, 0.3f, 1.1f) + Values { { "unison", 7 }, { "detune", 0.22f } }),
         { "Supersaw Pluck", "Stack Pluck", "Wide Pluck" });
    add ("kswire", catPlucks, kString, Y ("Plucked String", Values { { "engine", 2 }, { "wave", 0.62f }, { "cutoff", 5200 }, { "attack", 0.004f }, { "decay", 1.3f },
                                                            { "sustain", 1 }, { "release", 0.6f } }),
         { "Wire Pluck", "Steel Pluck", "String Pluck" });
    add ("ksgut", catPlucks, kString, Y ("Plucked String", Values { { "engine", 2 }, { "wave", 0.2f }, { "cutoff", 3200 }, { "attack", 0.004f }, { "decay", 0.9f },
                                                           { "sustain", 1 }, { "release", 0.45f } }),
         { "Gut Pluck", "Nylon Pluck", "Soft String" });
    add ("kszither", catPlucks, kString, Y ("Zither String", Values { { "engine", 2 }, { "wave", 0.42f }, { "cutoff", 4000 }, { "attack", 0.004f }, { "decay", 3.0f },
                                                            { "sustain", 1 }, { "release", 1.4f } }),
         { "Zither Pluck", "Long String", "Koto Pluck" });
    add ("fmglass", catPlucks, kFM, Y ("FM Glass", pluckEnv (5000, 0.0f, 0.3f, 1.2f, 0.6f) + Values { { "engine", 1 }, { "fmRatio", 4 }, { "fmAmt", 0.36f } }),
         { "Glass Pluck", "Crystal Pluck", "Ice Pluck" });
    add ("fmwood", catPlucks, kFM, Y ("FM Wood", pluckEnv (3500, 0.0f, 0.07f, 0.45f, 0.25f) + Values { { "engine", 1 }, { "fmRatio", 1 }, { "fmAmt", 0.45f }, { "wave", 0.3f } }),
         { "Wood Pluck", "Timber Pluck", "Log Pluck" });
    add ("fmtine", catPlucks, kFM, Y ("FM Tine", pluckEnv (4500, 0.0f, 0.35f, 1.4f, 0.5f) + Values { { "engine", 1 }, { "fmAlgo", 5 }, { "fmRatio", 1 }, { "fmRatio2", 14 }, { "fmAmt", 0.3f } }),
         { "Tine Pluck", "Key Pluck", "Chime Pluck" });
    add ("fmnylon", catPlucks, kFM, Y ("FM Harp", pluckEnv (3000, 0.0f, 0.2f, 1.0f, 0.4f) + Values { { "engine", 1 }, { "fmRatio", 2 }, { "fmAmt", 0.3f } }),
         { "Harp Pluck", "Lyre Pluck", "Silk Pluck" });
    add ("modalmar", catPlucks, kModal, Y ("Marimba Synth", Values { { "engine", 9 }, { "wave", 0.5f }, { "fmAmt", 0.45f }, { "cutoff", 6000 }, { "attack", 0.003f },
                                                           { "decay", 0.55f }, { "sustain", 1 }, { "release", 0.35f } }),
         { "Mallet Pluck", "Rosewood Pluck", "Bar Pluck" });
    add ("modalkal", catPlucks, kModal, Y ("Thumb Piano Synth", Values { { "engine", 9 }, { "wave", 0.15f }, { "fmAmt", 0.4f }, { "cutoff", 6000 }, { "attack", 0.003f },
                                                           { "decay", 0.8f }, { "sustain", 1 }, { "release", 0.4f } }),
         { "Thumb Pluck", "Lamella Pluck", "Tine Box Pluck" });
    add ("wtprism", catPlucks, kWave, Y ("Glass Wave", pluckEnv (1800, 0.3f, 0.25f, 1.0f) + Values { { "engine", 7 }, { "wave", wGlass } }),
         { "Prism Pluck", "Shard Pluck", "Facet Pluck" });
    add ("wtdigi", catPlucks, kWave, Y ("Digital Wave", pluckEnv (2500, 0.3f, 0.2f, 0.7f) + Values { { "engine", 7 }, { "wave", wDigi } }),
         { "Digital Pluck", "Pixel Pluck", "Bit Pluck" });
    add ("wttri", catPlucks, kWave | kSine, Y ("Triangle Pluck", pluckEnv (5000, 0.1f, 0.3f, 0.9f) + Values { { "engine", 7 }, { "wave", wTri } }),
         { "Soft Pluck", "Felt Pluck", "Round Pluck" });
    add ("syncpluck", catPlucks, kWave, Y ("Sync Pluck", pluckEnv (2600, 0.25f, 0.22f, 0.7f) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 } } + MM (0, 1, 5, 0.55f)),
         { "Sync Pluck", "Zap Pluck", "Snap Pluck" });
    add ("organpluck", catPlucks, kOrgan, Y ("Organ Pluck", Values { { "engine", 4 }, { "wave", 0.35f }, { "cutoff", 4000 }, { "attack", 0.003f }, { "decay", 0.35f },
                                                          { "sustain", 0 }, { "release", 0.2f } }),
         { "Click Pluck", "Organ Pluck", "Drawbar Pluck" });
    add ("breathpluck", catPlucks, kBreath, Y ("Breath Pluck", Values { { "engine", 5 }, { "wave", 0.45f }, { "cutoff", 3000 }, { "attack", 0.004f }, { "decay", 0.5f },
                                                            { "sustain", 0 }, { "release", 0.3f } }),
         { "Breath Pluck", "Pan Pluck", "Air Pluck" });
    add ("voxpluck", catPlucks, kVox, Y ("Vocal Pluck", pluckEnv (2500, 0.15f, 0.25f, 0.6f) + Values { { "engine", 3 }, { "wave", 1.0f } }),
         { "Vocal Pluck", "Voice Pluck", "Chant Pluck" });
    add ("orchpizz", catPlucks, kOrch, Y ("Synth Pizzicato", Values { { "engine", 8 }, { "wave", 0.5f }, { "unison", 3 }, { "detune", 0.08f }, { "cutoff", 2600 }, { "attack", 0.003f },
                                                              { "decay", 0.32f }, { "sustain", 0 }, { "release", 0.22f } }),
         { "Pizz Pluck", "Synth Pizz", "Bowstring Pluck" });
    add ("ladderpluck", catPlucks, kSaw, Y ("Ladder Pluck", pluckEnv (700, 0.55f, 0.24f, 0.8f) + Values { { "filterType", 1 }, { "reso", 0.42f } }),
         { "Rubber Pluck", "Resonant Pluck", "Squelch Pluck" });
    add ("bandpluck", catPlucks, kSaw, Y ("Band Pluck", pluckEnv (1100, 0.3f, 0.22f, 0.7f) + Values { { "filterType", 4 }, { "reso", 0.25f } }),
         { "Tube Pluck", "Pipe Pluck", "Bottle Pluck" });
    add ("droplet", catPlucks, kSine, Y ("Sine Drop", pluckEnv (8000, 0.0f, 0.045f, 0.6f, 0.3f) + Values { { "engine", 7 }, { "wave", wSine } } + MM (0, 1, 0, 0.22f)),
         { "Water Pluck", "Droplet", "Bubble Pluck" });
    add ("fmmetal", catPlucks, kFM, Y ("FM Metal", pluckEnv (4000, 0.0f, 0.25f, 0.9f, 0.4f) + Values { { "engine", 1 }, { "fmRatio", 3.5f }, { "fmAmt", 0.2f } }),
         { "Metal Pluck", "Tin Pluck", "Steel Drum Pluck" });
    add ("harppluck", catPlucks, kSampleShot, S ("pluck.harp.concert", "pluck", "Concert Harp", { { "s.cutoff", 4000.0f }, { "s.velSens", 0.35f } }),
         { "Harp Pluck", "Harpstring", "Angel Pluck" });
    add ("pizzpluck", catPlucks, kSampleShot, S ("strings.violas.ensemble", "pizzicato", "Viola Section", { { "s.velSens", 0.3f } }),
         { "Pizzicato", "Pizz Section", "Plucked Bows" });
    add ("dulcpluck", catPlucks, kSampleShot, S ("pluck.dulcimer", "pluck", "Stick Dulcimer", { { "s.cutoff", 4500.0f }, { "s.velSens", 0.35f } }),
         { "Dulcimer Pluck", "Hammer Pluck", "Folk Pluck" });
    add ("sitarpluck", catPlucks, kString, Y ("Sitar Synth", Values { { "engine", 2 }, { "wave", 0.55f }, { "cutoff", 4500 }, { "attack", 0.004f }, { "decay", 1.6f },
                                                                 { "sustain", 1 }, { "release", 0.7f }, { "body", 6 }, { "bodyMix", 0.5f } }),
         { "Sitar Pluck", "Drone String Pluck", "Buzz String" });
    add ("steelpan", catPlucks, kFM, Y ("Steel Pan Synth", pluckEnv (4500, 0.0f, 0.3f, 0.9f, 0.4f) + Values { { "engine", 1 }, { "fmAlgo", 4 }, { "fmRatio", 1 }, { "fmRatio2", 2 }, { "fmAmt", 0.25f } }),
         { "Steel Pan Pluck", "Island Pluck", "Calypso Pluck" });
    add ("celestapl", catPlucks, kModal, Y ("Celesta Synth", Values { { "engine", 9 }, { "wave", 0.8f }, { "fmAmt", 0.12f }, { "cutoff", 5000 }, { "attack", 0.003f }, { "decay", 0.9f },
                                                                   { "sustain", 1 }, { "release", 0.5f } }),
         { "Celesta Pluck", "Music Box Pluck", "Star Pluck" });
    add ("mutedgtr", catPlucks, kModel, M ("guitar.electric", "Electric Guitar", { { "m.artic", 1 }, { "m.tone", 0.45f } }), { "Muted Pluck", "Palm Pluck", "Damped Pluck" });
    add ("harmonicpl", catPlucks, kModel, M ("guitar.nylon", "Nylon Guitar", { { "m.artic", 2 } }), { "Harmonic Pluck", "Flageolet Pluck", "Ghost Harmonic" });
    add ("zitherpl", catPlucks, kSampleShot, S ("world.dantranh", "pluck", "Dan Tranh", { { "s.cutoff", 4500.0f }, { "s.velSens", 0.35f } }), { "Silk Zither Pluck", "River Zither", "Bamboo Zither" });
    add ("mbirapl", catPlucks, kSampleShot, S ("world.mbira.deep", "pluck", "Deep Mbira", { { "s.cutoff", 4500.0f }, { "s.velSens", 0.35f } }), { "Mbira Pluck", "Iron Tine Pluck", "Village Mbira" });
    add ("marimbapl", catPlucks, kSampleShot, S ("mallets.marimba", "hit", "Marimba", { { "s.decay", 0.35f }, { "s.sustain", 0.0f }, { "s.release", 0.2f }, { "s.velSens", 0.35f } }),
         { "Wood Bar Pluck", "Rosewood Bar", "Marimba Pluck" });
    add ("quillpl", catPlucks, kSampleShot, S ("keys.harpsichord.italian", "normal", "Italian Harpsichord", { { "s.cutoff", 4000.0f }, { "s.decay", 0.5f }, { "s.sustain", 0.0f },
                                                                                                       { "s.release", 0.2f }, { "s.velSens", 0.35f } }),
         { "Quill Pluck", "Plectrum Pluck", "Courtly Pluck" });
    add ("duopluck", catPlucks, kSaw | kSquare, Y ("Two Osc Pluck", pluckEnv (1600, 0.45f, 0.22f, 0.8f) + B (0, 0.5f, -1, 0, 0.5f)),
         { "Duo Pluck", "Layered Pluck", "Two Osc Pluck" });
    add ("chippluck", catPlucks, kSquare, Y ("Chip Pluck", pluckEnv (4000, 0.1f, 0.2f, 0.35f, 0.15f) + Values { { "wave", 0.62f }, { "crush", 0.2f } }),
         { "8-Bit Pluck", "Arcade Pluck", "Console Pluck" });
    add ("fmbellpl", catPlucks, kFM, Y ("FM Bell", pluckEnv (5000, 0.0f, 0.5f, 1.6f, 0.8f) + Values { { "engine", 1 }, { "fmAlgo", 4 }, { "fmRatio", 3.5f }, { "fmRatio2", 1 }, { "fmAmt", 0.14f } }),
         { "Bell Pluck", "Chapel Pluck", "Glimmer Pluck" });
    add ("pwmpluck", catPlucks, kPulse, Y ("PWM Pluck", pluckEnv (1800, 0.35f, 0.22f, 0.8f) + Values { { "wave", 0.7f }, { "unison", 2 }, { "detune", 0.08f }, { "lfo2Rate", 5.5f } } + MM (0, 4, 3, 0.15f)),
         { "Liquid Pluck", "Shimmer Pluck", "Ripple Pluck" });
    add ("organks", catPlucks, kOrgan | kString, Y ("Organ String", Values { { "engine", 4 }, { "wave", 0.4f }, { "cutoff", 4000 }, { "attack", 0.003f }, { "decay", 0.5f }, { "sustain", 0 },
                                                                         { "release", 0.3f } } + B (2, 0.5f, 0, 0, 0.7f)),
         { "Hybrid Organ Pluck", "Reed String Pluck", "Harmonium Pluck" });
    add ("brasspl", catPlucks, kOrch, Y ("Brass Blip", Values { { "engine", 8 }, { "wave", 0.0f }, { "unison", 2 }, { "detune", 0.06f }, { "cutoff", 3000 }, { "attack", 0.004f },
                                                              { "decay", 0.38f }, { "sustain", 0 }, { "release", 0.25f } }),
         { "Brass Pluck", "Horn Blip", "Fanfare Pluck" });

    //------------------------------------------------------------------ LEADS
    add ("thin", catLeads, kPulse, Y ("Thin Lead", leadEnv (3800) + Values { { "wave", 0.82f }, { "lfoPitch", 0.07f } }),
         { "Thin Lead", "Needle Lead", "Wire Lead" });
    add ("square", catLeads, kSquare, Y ("Square Lead", leadEnv (3200) + Values { { "wave", 0.5f } }),
         { "Square Lead", "Hollow Lead", "Clarinet Lead" });
    add ("saw", catLeads, kSaw, Y ("Saw Lead", leadEnv (3000) + Values { { "fenv", 0.15f }, { "fdecay", 0.4f }, { "fsustain", 0.3f } }),
         { "Saw Lead", "Classic Lead", "Mono Lead" });
    add ("superlead", catLeads, kSuper, Y ("Supersaw Lead", leadEnv (3200, 0.06f) + Values { { "mono", 0 }, { "unison", 6 }, { "detune", 0.17f } }),
         { "Supersaw Lead", "Anthem Lead", "Stack Lead" });
    add ("whistle", catLeads, kSine | kBreath, Y ("Whistle", leadEnv (5000, 0.12f) + Values { { "engine", 7 }, { "wave", wSine }, { "lfoPitch", 0.09f }, { "lfoRate", 5.9f } }
                                                    + B (5, 0.6f, 0, 0, 0.35f)),
         { "Whistle Lead", "Whistle", "Tin Whistle" });
    add ("flutelead", catLeads, kBreath, Y ("Flute Synth", leadEnv (3000) + Values { { "engine", 5 }, { "wave", 0.35f }, { "attack", 0.03f } }),
         { "Pan Lead", "Flute Lead", "Breath Lead" });
    add ("glidesine", catLeads, kSine, Y ("Sine Lead", leadEnv (6000, 0.32f) + Values { { "engine", 7 }, { "wave", wSine * 0.5f + wTri * 0.15f } }),
         { "Glide Sine", "Slide Lead", "Portamento Lead" });
    add ("dist", catLeads, kSaw, Y ("Distorted Saw", leadEnv (2400) + Values { { "velSens", 0.9f }, { "drive", 0.36f }, { "driveType", 2 }, { "eqHigh", -3 } } + B (0, 0.5f, 0, 0, 0.5f)),
         { "Distorted Lead", "Fuzz Lead", "Grit Lead" });
    add ("fold", catLeads, kSine, Y ("Folded Sine", leadEnv (4000) + Values { { "engine", 7 }, { "wave", wSine }, { "drive", 0.38f }, { "driveType", 4 } }),
         { "Folded Lead", "Fold Lead", "Wavefold Lead" });
    add ("fmlead", catLeads, kFM, Y ("FM Lead", leadEnv (3500) + Values { { "engine", 1 }, { "fmRatio", 1 }, { "fmAmt", 0.16f }, { "wave", 0.3f }, { "fdecay", 0.6f }, { "fsustain", 0.5f } }),
         { "FM Lead", "Digital Horn", "Brassy FM" });
    add ("voxlead", catLeads, kVox, Y ("Talk Lead", leadEnv (3000) + Values { { "engine", 3 }, { "wave", 0.25f } } + B (7, wSine, 0, 0, 0.55f)),
         { "Talk Lead", "Vowel Lead", "Mouth Lead" });
    add ("organlead", catLeads, kOrgan, Y ("Organ Lead", leadEnv (4000) + Values { { "engine", 4 }, { "wave", 0.6f }, { "chorus", 0.3f } }),
         { "Organ Lead", "Drawbar Lead", "Rock Organ Lead" });
    add ("brasslead", catLeads, kOrch, Y ("Brass Synth", leadEnv (2500) + Values { { "engine", 8 }, { "wave", 0.0f }, { "unison", 2 }, { "detune", 0.06f } }),
         { "Brass Lead", "Horn Lead", "Synth Trumpet" });
    add ("wtlead", catLeads, kWave, Y ("Wavetable Lead", leadEnv (3000) + Values { { "engine", 7 }, { "wave", wDigi }, { "lfo2Rate", 0.4f } } + MM (0, 4, 3, 0.1f)),
         { "Digital Lead", "Wavetable Lead", "Grid Lead" });
    add ("synclead", catLeads, kWave, Y ("Sync Lead", leadEnv (3000) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "fmAmt", 0.3f } } + MM (0, 1, 5, 0.25f)
                                                       + Values { { "fdecay", 0.5f } }),
         { "Sync Lead", "Tear Lead", "Hard Sync Lead" });
    add ("chip", catLeads, kSquare, Y ("Chip Lead", leadEnv (3500, 0.04f) + Values { { "wave", 0.62f }, { "crush", 0.22f }, { "lfoPitch", 0.12f }, { "lfoRate", 7.5f } }),
         { "Chip Lead", "8-Bit Lead", "Arcade Lead" });
    add ("twin", catLeads, kSaw, Y ("Twin Saw", leadEnv (3000) + B (0, 0.0f, 0, 0, 0.8f, { { "fineB", 11 } })),
         { "Twin Lead", "Detuned Lead", "Duo Lead" });
    add ("octlead", catLeads, kSquare, Y ("Octave Square", leadEnv (3000) + Values { { "wave", 0.5f } } + B (0, 0.0f, -1, 0, 0.6f)),
         { "Octave Lead", "Deep Lead", "Low Octave Lead" });
    add ("theremin", catLeads, kSine, Y ("Theremin", leadEnv (6000, 0.5f) + Values { { "engine", 7 }, { "wave", wSine }, { "lfoPitch", 0.22f }, { "lfoRate", 6.3f }, { "attack", 0.06f } }),
         { "Theremin", "Ghost Lead", "Ether Lead" });
    add ("acidlead", catLeads, kSaw, Y ("Ladder Lead", leadEnv (1600) + Values { { "filterType", 1 }, { "reso", 0.38f }, { "fenv", 0.3f }, { "fdecay", 0.3f }, { "fsustain", 0.2f } }),
         { "Rubber Lead", "Acid Lead", "Squelch Lead" });
    add ("pwmlead", catLeads, kPulse, Y ("PWM Lead", leadEnv (3500) + Values { { "wave", 0.68f }, { "lfo2Rate", 3.2f } } + MM (0, 4, 3, 0.18f)),
         { "PWM Lead", "Pulse Width Lead", "Shimmer Lead" });
    add ("softsq", catLeads, kWave, Y ("Soft Square", leadEnv (2600) + Values { { "engine", 7 }, { "wave", wTri + 0.08f } }),
         { "Soft Lead", "Ocarina Synth", "Flutey Lead" });
    add ("radio", catLeads, kSaw, Y ("Band Lead", leadEnv (1200) + Values { { "filterType", 4 }, { "reso", 0.2f }, { "wave", 0.3f } }),
         { "Radio Lead", "Telephone Lead", "Transistor Lead" });
    add ("modellead", catLeads, kModel, M ("voice.female", "Female Voice", { { "m.vowel", 2.0f }, { "m.artic", 0 }, { "m.vibrato", 0.25f }, { "m.bright", 0.35f } }),
         { "Siren Voice Lead", "Voice Lead", "Diva Lead" });
    add ("fuzzsine", catLeads, kSine, Y ("Fuzz Sine", leadEnv (3500) + Values { { "engine", 7 }, { "wave", wTri * 0.5f }, { "drive", 0.42f }, { "driveType", 3 }, { "eqHigh", -2 } }),
         { "Fuzz Sine", "Blown Lead", "Overdrive Lead" });
    add ("glasslead", catLeads, kWave, Y ("Glass Lead", leadEnv (3200, 0.14f) + Values { { "engine", 7 }, { "wave", wGlass } }), { "Glass Lead", "Crystal Lead", "Prism Lead" });
    add ("belllead", catLeads, kFM, Y ("Bell Lead", leadEnv (4000) + Values { { "engine", 1 }, { "fmRatio", 3 }, { "fmAmt", 0.12f }, { "fenv", 0 }, { "fdecay", 0.5f }, { "fsustain", 0.3f } }),
         { "Bell Lead", "Chime Lead", "Carillon Lead" });
    add ("wahlead", catLeads, kSaw, Y ("Wah Lead", leadEnv (900) + Values { { "filterType", 4 }, { "reso", 0.35f }, { "fenv", 0.5f }, { "fattack", 0.06f }, { "fdecay", 0.3f }, { "fsustain", 0.2f } }),
         { "Wah Lead", "Quack Lead", "Funk Lead" });
    add ("lowglide", catLeads, kSine, Y ("Low Glide", leadEnv (2500, 0.4f) + Values { { "octave", -1 }, { "engine", 7 }, { "wave", wTri }, { "drive", 0.2f }, { "driveType", 1 } }),
         { "Low Glide Lead", "Basement Lead", "Deep Glide" });
    add ("ohlead", catLeads, kModel, M ("voice.female", "Female Voice", { { "m.vowel", 1.0f }, { "m.artic", 0 }, { "m.vibrato", 0.35f }, { "m.bright", 0.5f } }),
         { "Oh Voice Lead", "Aria Lead", "Opera Lead" });
    add ("tenorlead", catLeads, kModel, M ("voice.male", "Male Voice", { { "m.vowel", 0.0f }, { "m.artic", 0 }, { "m.vibrato", 0.3f } }), { "Tenor Lead", "Chant Lead", "Cantor Lead" });
    add ("breathylead", catLeads, kModel, M ("voice.female", "Female Voice", { { "m.vowel", 2.0f }, { "m.artic", 2 }, { "m.vibrato", 0.2f } }), { "Breathy Lead", "Airy Voice Lead", "Sigh Lead" });
    add ("duosqsaw", catLeads, kSquare | kSaw, Y ("Square Saw", leadEnv (2800) + Values { { "wave", 0.5f } } + B (0, 0.0f, 0, 0, 0.7f, { { "fineB", -7 } })),
         { "Duo Square Saw", "Hybrid Lead", "Blend Lead" });
    add ("pipelead", catLeads, kOrgan, Y ("Flute Stop", leadEnv (5000, 0.08f) + Values { { "engine", 4 }, { "wave", 0.08f }, { "lfoPitch", 0.06f } }), { "Pipe Lead", "Flute Stop Lead", "Calliope Lead" });
    add ("pulsechip", catLeads, kPulse, Y ("Pulse Chip", leadEnv (4000, 0.03f) + Values { { "wave", 0.88f }, { "crush", 0.28f } }), { "Pulse Chip Lead", "Bleep Lead", "Pocket Lead" });
    add ("laserlead", catLeads, kWave, Y ("Laser Lead", leadEnv (2800) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "e3attack", 1.2f }, { "e3sustain", 1 } } + MM (0, 2, 5, 0.45f)),
         { "Laser Lead", "Sweep Lead", "Beam Lead" });
    add ("cleanflute", catLeads, kBreath, Y ("Clean Flute", leadEnv (4000) + Values { { "engine", 5 }, { "wave", 0.1f }, { "chorus", 0.25f }, { "attack", 0.04f } }),
         { "Clean Flute Lead", "Bamboo Lead", "Reed Pipe Lead" });

    //------------------------------------------------------------------ PADS (synth)
    add ("warm", catPads, kSaw, Y ("Warm Pad", padEnv (1850) + Values { { "unison", 5 }, { "detune", 0.15f } }),
         { "Warm Pad", "Velvet Pad", "Blanket Pad" });
    add ("shimmer", catPads, kSuper, Y ("Supersaw Pad", padEnv (2550) + Values { { "unison", 8 }, { "detune", 0.24f } } + B (0, 0.0f, 1, 0, 0.16f, { { "unisonB", 4 }, { "detuneB", 0.2f } })),
         { "Shimmer Pad", "Glow Pad", "Aurora Pad" });
    add ("pwmpad", catPads, kPulse, Y ("PWM Pad", padEnv (2200) + Values { { "wave", 0.66f }, { "unison", 3 }, { "detune", 0.1f }, { "lfo2Rate", 0.35f } } + MM (0, 4, 3, 0.2f)),
         { "PWM Pad", "Pulse Pad", "Ripple Pad" });
    add ("softpad", catPads, kWave | kSine, Y ("Triangle Pad", padEnv (8500) + Values { { "engine", 7 }, { "wave", wTri }, { "unison", 4 }, { "detune", 0.14f } }),
         { "Soft Pad", "Cotton Pad", "Feather Pad" });
    add ("glasspad", catPads, kWave, Y ("Glass Pad", padEnv (3400) + Values { { "engine", 7 }, { "wave", wGlass }, { "unison", 3 }, { "detune", 0.12f } }),
         { "Glass Pad", "Crystal Pad", "Frost Pad" });
    add ("morphpad", catPads, kWave, Y ("Morph Pad", padEnv (2700) + Values { { "engine", 7 }, { "wave", wSq }, { "unison", 3 }, { "e3attack", 3.5f }, { "e3sustain", 1 } } + MM (0, 2, 3, 0.4f)),
         { "Morph Pad", "Sweep Pad", "Wavescan Pad" });
    add ("vowelpad", catPads, kVox, Y ("Formant Pad", padEnv (4400) + Values { { "engine", 3 }, { "wave", 0.6f }, { "unison", 3 }, { "lfo2Rate", 0.13f } } + MM (0, 4, 3, 0.35f)
                                                      + B (7, wSine, 0, 0, 0.6f, { { "unisonB", 3 }, { "detuneB", 0.1f } })),
         { "Vowel Pad", "Formant Pad", "Mouth Pad" });
    add ("synthchoir", catPads, kOrch | kVox, Y ("Synth Choir", padEnv (5100) + Values { { "engine", 8 }, { "wave", 1.0f }, { "fmAmt", 0.95f }, { "unison", 4 }, { "detune", 0.12f } }
                                                         + B (7, wSine, 0, 0, 0.5f)),
         { "Synth Choir", "Vapor Choir", "Ghost Choir" });
    add ("strmachine", catPads, kOrch, Y ("String Machine", padEnv (3750, 0.5f) + Values { { "engine", 8 }, { "wave", 0.5f }, { "unison", 6 }, { "detune", 0.13f }, { "chorus", 0.4f } }),
         { "String Machine", "Ensemble Pad", "Synth Strings" });
    add ("organpad", catPads, kOrgan, Y ("Organ Pad", padEnv (5100, 0.9f) + Values { { "engine", 4 }, { "wave", 0.22f } }),
         { "Organ Pad", "Drawbar Pad", "Hymn Pad" });
    add ("breathpad", catPads, kBreath | kNoise, Y ("Breath Pad", padEnv (2550) + Values { { "engine", 5 }, { "wave", 0.85f } }),
         { "Breath Pad", "Air Pad", "Wind Pad" });
    add ("fmpad", catPads, kFM, Y ("FM Pad", padEnv (6800) + Values { { "engine", 1 }, { "fmRatio", 2 }, { "fmAmt", 0.1f }, { "unison", 3 }, { "detune", 0.1f }, { "fsustain", 0.6f } }),
         { "FM Pad", "Satin Pad", "Silk Pad" });
    add ("bellpad", catPads, kFM, Y ("Bell Pad", padEnv (6800) + Values { { "engine", 1 }, { "fmRatio", 3.5f }, { "fmAmt", 0.1f }, { "unison", 2 }, { "fsustain", 0.4f }, { "fdecay", 1.5f } }),
         { "Bell Pad", "Chime Pad", "Temple Pad" });
    add ("halo", catPads, kSine, Y ("Sine Pad", padEnv (10200) + Values { { "engine", 7 }, { "wave", wSine }, { "unison", 6 }, { "detune", 0.28f } }),
         { "Halo Pad", "Sine Choir", "Hum Pad" });
    add ("darkpad", catPads, kSaw, Y ("Ladder Pad", padEnv (800) + Values { { "filterType", 1 }, { "reso", 0.3f }, { "unison", 3 } }),
         { "Dark Pad", "Cave Pad", "Shadow Pad" });
    add ("hollowpad", catPads, kSaw, Y ("Band Pad", padEnv (1350) + Values { { "filterType", 4 }, { "reso", 0.3f }, { "unison", 3 }, { "lfoFilter", 0.25f }, { "lfoRate", 0.09f } }),
         { "Hollow Pad", "Whale Pad", "Tunnel Pad" });
    add ("syncpad", catPads, kWave, Y ("Sync Pad", padEnv (2400) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "fmAmt", 0.12f }, { "unison", 3 },
                                                     { "e3attack", 4.0f }, { "e3sustain", 1 } } + MM (0, 2, 5, 0.3f)),
         { "Sync Pad", "Rising Sync", "Laser Pad" });
    //------------------------------------------------------------------ PADS (real recordings)
    auto spad = [] (const char* inst, const char* art, const char* label, float att, float rel, float cut)
    { return S (inst, art, label, { { "s.attack", att }, { "s.release", rel }, { "s.cutoff", cut }, { "s.velSens", 0.2f } }); };
    add ("violinpad", catPads, kSampleSus, spad ("strings.violins.ensemble", "sustain", "Violin Section", 0.9f, 2.6f, 3600), { "Violin Cloud", "Silver Strings", "String Haze" });
    add ("violapad", catPads, kSampleSus, spad ("strings.violas.ensemble", "sustain", "Viola Section", 1.0f, 2.6f, 3000), { "Viola Haze", "Viola Dusk", "Amber Strings" });
    add ("cellopad", catPads, kSampleSus, spad ("strings.cellos.ensemble", "sustain", "Cello Section", 1.0f, 2.8f, 2400), { "Cello Dusk", "Cello Glow", "Low Strings" });
    add ("tremopad", catPads, kSampleSus, spad ("strings.violas.ensemble", "tremolo", "Viola Section", 1.2f, 2.4f, 2600), { "Tremolo Mist", "Shiver Strings", "Flutter Strings" });
    add ("cellotrem", catPads, kSampleSus, spad ("strings.cellos.ensemble", "tremolo", "Cello Section", 1.2f, 2.6f, 2000), { "Tremor Pad", "Rumble Strings", "Storm Strings" });
    add ("choirpad", catPads, kSampleSus, spad ("voice.choir.classic", "oohs", "Classic Choir", 0.8f, 2.6f, 2400), { "Choir Pad", "Ooh Choir", "Chapel Voices" });
    add ("aahpad", catPads, kSampleSus, spad ("voice.choir.classic", "aahs", "Classic Choir", 0.8f, 2.6f, 1500), { "Aah Choir", "Choir Cloud", "Hymn Voices" });
    add ("voxpad", catPads, kSampleSus, spad ("voice.choir.classic", "solo_vox", "Solo Choir Voice", 0.7f, 2.4f, 2600), { "Lone Voice Pad", "Solo Voice Pad", "Vigil Voice" });
    add ("pipepad", catPads, kSampleSus, spad ("organ.pipe", "soft", "Pipe Organ", 1.0f, 2.8f, 2600), { "Cathedral Pad", "Flue Pad", "Nave Pad" });
    add ("chamberpad", catPads, kSampleSus, spad ("organ.chamber", "8 foot", "Chamber Organ", 0.9f, 2.5f, 2600), { "Chapel Pad", "Vesper Pad", "Cloister Pad" });
    add ("hornpad", catPads, kSampleSus, spad ("brass.horn", "sustain", "French Horn", 1.0f, 2.5f, 1800), { "Horn Pad", "Distant Horns", "Twilight Horns" });
    add ("clarpad", catPads, kSampleSus, spad ("winds.clarinet", "sustain", "Clarinet", 0.9f, 2.4f, 2200), { "Clarinet Haze", "Reed Mist", "Woodwind Pad" });
    {
        auto g = spad ("fx.glass", "slow", "Glass Harp", 0.8f, 2.6f, 4000);
        g.octave = 1;
        add ("glassharp", catPads, kSampleSus, g, { "Glass Harp Pad", "Rim Pad", "Crystal Rim" });
    }
    add ("vocalpadm", catPads, kModel, M ("voice.pad", "Vocal Pad", { { "m.velSens", 0.4f } }), { "Breath Choir", "Angel Pad", "Soft Vocal Pad" });
    add ("brasspad", catPads, kSaw, Y ("Brass Pad", padEnv (900, 0.5f) + Values { { "unison", 4 }, { "fenv", 0.4f }, { "fattack", 1.5f }, { "fdecay", 2.0f }, { "fsustain", 0.5f } }),
         { "Brass Pad", "Analog Brass Pad", "Horizon Brass" });
    add ("bloompad", catPads, kSaw, Y ("Bloom Pad", padEnv (500) + Values { { "unison", 4 }, { "e3attack", 5.0f }, { "e3sustain", 1 }, { "e3release", 3 } } + MM (0, 2, 1, 0.45f)),
         { "Bloom Pad", "Sunrise Pad", "Opening Pad" });
    add ("digipad", catPads, kWave, Y ("Digital Pad", padEnv (3000) + Values { { "engine", 7 }, { "wave", wDigi }, { "unison", 4 } }), { "Digital Pad", "Data Pad", "Pixel Pad" });
    add ("sqpad", catPads, kSquare, Y ("Square Pad", padEnv (2400) + Values { { "wave", 0.5f }, { "unison", 2 }, { "detune", 0.08f }, { "chorus", 0.6f } }),
         { "Square Pad", "Ensemble Square", "Retro Pad" });
    add ("noisepad", catPads, kNoise, Y ("Static Pad", padEnv (900) + Values { { "engine", 1 }, { "fmRatio", 5.31f }, { "wave", 1.0f }, { "fmAmt", 0.8f }, { "fsustain", 1 }, { "filterType", 4 },
                                                                         { "reso", 0.3f } } + B (7, wSine, 0, 0, 0.8f, { { "unisonB", 3 }, { "detuneB", 0.1f } })),
         { "Rain Pad", "Static Pad", "Noise Bed" });
    add ("oohpad", catPads, kVox, Y ("Ooh Pad", padEnv (3500) + Values { { "engine", 3 }, { "wave", 1.0f }, { "unison", 4 }, { "detune", 0.1f } }), { "Ooh Pad", "Moan Pad", "Hum Choir Pad" });
    add ("bassoonpad", catPads, kSampleSus, spad ("winds.bassoon", "sustain", "Bassoon", 0.9f, 2.4f, 2000), { "Bassoon Haze", "Low Reed Pad", "Amber Reeds" });
    add ("saxpad", catPads, kSampleSus, spad ("winds.sax.tenor", "sustain", "Tenor Sax", 0.9f, 2.4f, 2000), { "Smoky Sax Pad", "Sax Haze", "Velvet Sax Pad" });
    add ("harmpad", catPads, kSampleSus, spad ("winds.harmonica.chromatic", "normal", "Chromatic Harmonica", 0.9f, 2.4f, 2200), { "Harmonica Haze", "Mouth Organ Pad", "Porch Reeds" });
    add ("fullorgpad", catPads, kSampleSus, spad ("organ.chamber", "full", "Chamber Organ", 0.9f, 2.5f, 2200), { "Full Organ Pad", "Plenum Pad", "Choir Loft Pad" });
    add ("fourpad", catPads, kSampleSus, spad ("organ.chamber", "4 foot", "Chamber Organ", 0.9f, 2.5f, 2800), { "Four Foot Pad", "High Pipe Pad", "Fife Pad" });
    add ("principalpad", catPads, kSampleSus, spad ("organ.pipe", "principal", "Pipe Organ", 1.0f, 2.8f, 2200), { "Principal Pad", "Great Pipe Pad", "Minster Pad" });
    add ("choirmodel", catPads, kModel, M ("voice.choir", "Choir", { { "m.voices", 16 }, { "m.vowel", 0.0f }, { "m.attack", 0.8f }, { "m.release", 2.0f }, { "m.velSens", 0.4f } }),
         { "Mass Choir", "Assembly Pad", "Choir Ah Pad" });
    add ("malechoir", catPads, kModel, M ("voice.choir", "Choir", { { "m.voices", 12 }, { "m.gender", 0.0f }, { "m.vowel", 1.0f }, { "m.attack", 0.9f }, { "m.release", 2.2f }, { "m.velSens", 0.4f } }),
         { "Monk Pad", "Male Choir Pad", "Abbey Pad" });
    add ("femchoir", catPads, kModel, M ("voice.choir", "Choir", { { "m.voices", 12 }, { "m.gender", 1.0f }, { "m.vowel", 2.0f }, { "m.attack", 0.9f }, { "m.release", 2.2f }, { "m.velSens", 0.4f } }),
         { "Angel Choir", "Women Choir Pad", "Halo Choir" });
    add ("vocalpadee", catPads, kModel, M ("voice.pad", "Vocal Pad", { { "m.vowel", 3.0f }, { "m.velSens", 0.4f } }), { "Bright Vocal Pad", "Ee Vocal Pad", "Seraph Pad" });

    //------------------------------------------------------------------ SYNTH (poly synths, stabs, keys)
    add ("polysaw", catSynth, kSaw, Y ("Poly Saw", polyEnv (2200) + Values { { "unison", 3 }, { "detune", 0.1f }, { "chorus", 0.3f } }),
         { "Poly Saw", "Analog Poly", "Poly Synth" });
    add ("brass", catSynth, kSaw, Y ("Synth Brass", Values { { "unison", 3 }, { "detune", 0.12f }, { "cutoff", 900 }, { "fenv", 0.45f }, { "fattack", 0.07f }, { "fdecay", 0.6f },
                                                      { "fsustain", 0.45f }, { "attack", 0.025f }, { "decay", 1 }, { "sustain", 0.85f }, { "release", 0.35f } }),
         { "Synth Brass", "Brass Poly", "Fanfare Synth" });
    add ("sqstab", catSynth, kSquare, Y ("Square Stab", polyEnv (2000, 0.35f, 0.0f, 0.2f) + Values { { "wave", 0.5f }, { "fenv", 0.35f }, { "fdecay", 0.2f } }),
         { "Square Stab", "Chord Stab", "Block Stab" });
    add ("superstab", catSynth, kSuper, Y ("Supersaw", polyEnv (2800, 0.45f, 0.0f, 0.3f) + Values { { "unison", 8 }, { "detune", 0.22f } }),
         { "Rave Stab", "Supersaw Stab", "Trance Stab" });
    add ("unikeys", catSynth, kSaw, Y ("Unison Keys", polyEnv (2500, 1.0f, 0.3f, 0.45f) + Values { { "unison", 4 }, { "detune", 0.08f }, { "wave", 0.2f } }),
         { "Synth Keys", "Unison Keys", "Neon Keys" });
    add ("fmkeys", catSynth, kFM, Y ("FM Keys", polyEnv (5000, 1.4f, 0.2f, 0.5f) + Values { { "engine", 1 }, { "fmAlgo", 5 }, { "fmRatio", 1 }, { "fmRatio2", 14 }, { "fmAmt", 0.16f },
                                                                         { "fenv", 0 }, { "fdecay", 0.6f } }),
         { "FM Poly", "Digital Keys", "Chrome Poly" });
    add ("houseorgan", catSynth, kOrgan, Y ("House Organ", polyEnv (3500, 0.4f, 0.25f, 0.15f) + Values { { "engine", 4 }, { "wave", 0.55f }, { "fenv", 0 } }),
         { "House Organ", "Stab Organ", "Club Organ" });
    add ("wtpoly", catSynth, kWave, Y ("Wavetable Poly", polyEnv (3000) + Values { { "engine", 7 }, { "wave", wGlass * 0.6f + wDigi * 0.4f }, { "unison", 3 } }),
         { "Wavetable Poly", "Digital Poly", "Crystal Poly" });
    add ("voxstab", catSynth, kVox, Y ("Formant Stab", polyEnv (2400, 0.5f, 0.1f, 0.25f) + Values { { "engine", 3 }, { "wave", 0.95f } } + B (7, wSine, 0, 0, 0.5f)),
         { "Vocal Stab", "Vocal Synth", "Formant Stab" });
    add ("orchhit", catSynth, kOrch, Y ("Orchestra Synth", polyEnv (2400, 0.5f, 0.0f, 0.5f) + Values { { "engine", 8 }, { "wave", 0.3f }, { "unison", 4 }, { "detune", 0.1f } }),
         { "Hit Synth", "Orchestra Stab", "Tutti Synth" });
    add ("syncpoly", catSynth, kWave, Y ("Sync Poly", polyEnv (2600) + Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "fmAmt", 0.18f }, { "unison", 2 } }),
         { "Sync Poly", "Sync Keys", "Hard Sync Poly" });
    add ("fatpoly", catSynth, kSaw, Y ("Ladder Poly", polyEnv (1300) + Values { { "filterType", 1 }, { "reso", 0.25f }, { "unison", 2 }, { "detune", 0.07f } } + B (0, 0.5f, -1, 0, 0.45f)),
         { "Fat Poly", "Ladder Poly", "Heavy Poly" });
    add ("polystr", catSynth, kOrch, Y ("String Synth", Values { { "engine", 8 }, { "wave", 0.5f }, { "unison", 5 }, { "detune", 0.12f }, { "cutoff", 2400 }, { "attack", 0.08f },
                                                          { "decay", 1 }, { "sustain", 0.9f }, { "release", 0.6f }, { "chorus", 0.35f } }),
         { "Poly Strings", "Solina Keys", "Ensemble Synth" });
    add ("pwmpoly", catSynth, kPulse, Y ("PWM Poly", polyEnv (2500) + Values { { "wave", 0.7f }, { "lfo2Rate", 1.1f }, { "unison", 2 } } + MM (0, 4, 3, 0.15f)),
         { "PWM Poly", "Pulse Poly", "Woven Poly" });
    add ("hollowpoly", catSynth, kSaw, Y ("Band Poly", polyEnv (1300, 0.8f, 0.4f, 0.4f) + Values { { "filterType", 4 }, { "reso", 0.22f }, { "unison", 2 } }),
         { "Hollow Poly", "Nasal Poly", "Horn Poly" });
    add ("sinekeys", catSynth, kSine | kFM, Y ("Sine Keys", polyEnv (6000, 1.6f, 0.15f, 0.6f) + Values { { "engine", 1 }, { "fmRatio", 2 }, { "fmAmt", 0.09f }, { "fenv", 0 } }),
         { "Sine Keys", "Soft Synth Keys", "Glass Keys Synth" });
    add ("gated", catSynth, kSaw, Y ("Gated Synth", polyEnv (2400, 1.0f, 0.9f, 0.3f) + Values { { "unison", 3 }, { "lfoSync", 1 }, { "lfoDiv", 4 }, { "lfoShape", 1 }, { "lfoAmp", 0.85f } }),
         { "Gated Chords", "Pulse Synth", "Chopped Synth" });
    add ("pump", catSynth, kSquare, Y ("Pump Synth", polyEnv (2600, 1.0f, 0.9f, 0.3f) + Values { { "wave", 0.35f }, { "unison", 3 }, { "lfoSync", 1 }, { "lfoDiv", 2 }, { "lfoShape", 0 },
                                                                       { "lfoAmp", 0.7f } }),
         { "Pumping Synth", "Sidechain Chords", "Breathing Synth" });
    // the two-step sequences: the square LFO is +-1, so the pitch route swings +-half the interval around the oscillator's
    // pitch - the oscillator is lifted by half the interval (semi / fine) so the steps are the played key and key + interval
    // (without the lift a fifth sequence played two quarter-tone-off notes and the octave bounce a tritone away); the
    // square starts at +1, so the routes are negative: the first step is the played key, the second the interval above
    add ("seq5", catSynth, kSaw, Y ("Sequence Synth", polyEnv (2300, 1.0f, 0.85f, 0.3f) + Values { { "unison", 2 }, { "lfo2Sync", 1 }, { "lfo2Div", 3 }, { "lfo2Shape", 3 },
                                                                                                  { "semi", 3 }, { "fine", 50 } }
                                                         + MM (0, 4, 0, -7.0f / 24.0f) + Values { { "lfoSync", 1 }, { "lfoDiv", 4 }, { "lfoShape", 2 }, { "lfoFilter", 0.3f } }),
         { "Fifth Sequence", "Two Step Synth", "Fifth Bounce" });
    add ("seqoct", catSynth, kSquare, Y ("Sequence Synth", polyEnv (2400, 1.0f, 0.85f, 0.3f) + Values { { "wave", 0.5f }, { "lfo2Sync", 1 }, { "lfo2Div", 4 }, { "lfo2Shape", 3 },
                                                                                                     { "semi", 6 } }
                                                            + MM (0, 4, 0, -0.5f)),
         { "Octave Bounce", "Low Bounce", "Hop Synth" });
    add ("epsynth", catSynth, kModel, M ("ep.tine.mellow", "Tine Electric Piano", { { "m.tone", 0.45f } }), { "EP Synth", "Tine Synth", "Glow Keys" });
    add ("clavsynth", catSynth, kModel, M ("ep.clav", "Funk Keys", { { "m.tone", 0.45f } }), { "Funk Synth", "Clav Synth", "Strut Synth" });
    add ("fmbrass", catSynth, kFM, Y ("FM Brass", polyEnv (5000, 1.0f, 0.8f, 0.3f) + Values { { "engine", 1 }, { "fmRatio", 1 }, { "fmAmt", 0.22f }, { "fenv", 0 }, { "fattack", 0.08f },
                                                                         { "fdecay", 0.5f }, { "fsustain", 0.6f } }),
         { "FM Brass", "Digital Brass", "Steel Brass" });
    add ("chorussq", catSynth, kSquare, Y ("Square Poly", polyEnv (2400) + Values { { "wave", 0.5f }, { "unison", 2 }, { "detune", 0.06f }, { "chorus", 0.6f } }),
         { "Chorus Poly", "Square Poly", "Retro Poly" });
    add ("chippoly", catSynth, kSquare, Y ("Chip Poly", polyEnv (4000, 0.5f, 0.3f, 0.2f) + Values { { "wave", 0.55f }, { "crush", 0.25f }, { "fenv", 0 } }), { "Chip Poly", "Pixel Keys", "Bit Poly" });
    add ("glasspoly", catSynth, kWave, Y ("Glass Poly", polyEnv (3000, 1.2f, 0.3f, 0.5f) + Values { { "engine", 7 }, { "wave", wGlass }, { "unison", 2 } }), { "Glass Keys Poly", "Ice Poly", "Frost Keys" });
    add ("sawchoir", catSynth, kSaw | kVox, Y ("Saw Choir", polyEnv (2200, 1.0f, 0.8f, 0.4f) + Values { { "unison", 3 } } + B (3, 0.0f, 0, 0, 0.8f, { { "unisonB", 3 } })),
         { "Choir Synth", "Saw Choir", "Chorale Synth" });
    add ("organsaw", catSynth, kOrgan | kSaw, Y ("Organ Synth", polyEnv (3000, 0.8f, 0.7f, 0.2f) + Values { { "engine", 4 }, { "wave", 0.5f } } + B (0, 0.0f, 0, 0, 0.45f)),
         { "Organ Synth", "Combo Synth", "Bar Organ Synth" });
    add ("detkeys", catSynth, kSaw, Y ("Detuned Keys", polyEnv (2400, 1.2f, 0.3f, 0.4f) + Values { { "unison", 2 }, { "detune", 0.32f }, { "wave", 0.15f } } + MM (0, 5, 1, 0.3f)),
         { "Detuned Keys", "Seasick Keys", "Warped Poly" });
    add ("stackfm", catSynth, kFM, Y ("Stack FM", polyEnv (6000, 1.3f, 0.2f, 0.5f) + Values { { "engine", 1 }, { "fmAlgo", 1 }, { "fmRatio", 1 }, { "fmRatio2", 2 }, { "fmAmt", 0.14f }, { "fenv", 0 } }),
         { "Stack FM Poly", "Tower Keys", "Layer FM Keys" });
    add ("pulsestr", catSynth, kPulse, Y ("Pulse Strings", Values { { "wave", 0.7f }, { "unison", 6 }, { "detune", 0.12f }, { "cutoff", 2600 }, { "attack", 0.15f }, { "decay", 1 },
                                                                   { "sustain", 0.9f }, { "release", 0.7f }, { "chorus", 0.4f } }),
         { "Pulse Strings", "Vintage Strings", "String Ensemble Synth" });
    add ("reedsynth", catSynth, kPulse, Y ("Reed Synth", polyEnv (2200, 1.0f, 0.85f, 0.25f) + Values { { "wave", 0.85f }, { "filterType", 4 }, { "reso", 0.15f }, { "unison", 2 }, { "detune", 0.05f } }),
         { "Reed Synth", "Musette Synth", "Bellows Synth" });

    //------------------------------------------------------------------ FX & ATMOS
    add ("riser", catFxAtmos, kRiser | kSuper, Y ("Riser", Values { { "unison", 6 }, { "detune", 0.2f }, { "cutoff", 500 }, { "attack", 1.2f }, { "decay", 1 }, { "sustain", 1 }, { "release", 1.5f },
                                                             { "e3attack", 4.0f }, { "e3sustain", 1 }, { "e3release", 1.5f }, { "velSens", 0.6f } } + MM (0, 2, 0, 1.0f) + MM (1, 2, 1, 0.45f)),
         { "Riser", "Ascension", "Lift Off" });
    add ("noiserise", catFxAtmos, kRiser | kNoise, Y ("Noise Sweep", Values { { "engine", 1 }, { "fmRatio", 5.31f }, { "wave", 1.0f }, { "fmAmt", 0.85f }, { "fenv", 0 }, { "fsustain", 1 }, { "filterType", 4 }, { "reso", 0.35f }, { "cutoff", 400 }, { "attack", 1.5f },
                                                                    { "decay", 1 }, { "sustain", 1 }, { "release", 1.6f }, { "e3attack", 4.5f }, { "e3sustain", 1 }, { "e3release", 1.5f },
                                                                    { "velSens", 0.6f } } + MM (0, 2, 1, 0.42f)),
         { "Noise Sweep", "White Rise", "Vapor Rise" });
    add ("fall", catFxAtmos, kRiser | kSaw, Y ("Downlifter", Values { { "unison", 4 }, { "detune", 0.15f }, { "cutoff", 1600 }, { "attack", 0.01f }, { "decay", 3.5f }, { "sustain", 0 },
                                                               { "release", 1.2f }, { "fattack", 0.001f }, { "fdecay", 2.5f }, { "fsustain", 0 } } + MM (0, 1, 0, 1.0f) + MM (1, 1, 1, 0.25f)),
         { "Downlifter", "Descent", "Falling Star" });
    add ("drone", catFxAtmos, kDrone | kSaw, Y ("Drone", Values { { "octave", -1 }, { "unison", 5 }, { "detune", 0.1f }, { "cutoff", 600 }, { "reso", 0.2f }, { "attack", 1.5f }, { "sustain", 1 },
                                                            { "release", 3 }, { "lfoFilter", 0.25f }, { "lfoRate", 0.07f }, { "lfo2Rate", 0.05f } } + MM (0, 4, 12, 0.3f)),
         { "Drone", "Abyss Drone", "Monolith" });
    add ("organdrone", catFxAtmos, kDrone | kOrgan, Y ("Organ Drone", Values { { "engine", 4 }, { "wave", 0.4f }, { "octave", -1 }, { "cutoff", 1400 }, { "attack", 2.0f }, { "sustain", 1 },
                                                                       { "release", 3 }, { "lfoAmp", 0.15f }, { "lfoRate", 0.18f } } + B (4, 0.2f, -1, 7, 0.5f)),
         { "Ritual Drone", "Fifth Drone", "Temple Drone" });
    add ("texture", catFxAtmos, kWave, Y ("Grain Texture", Values { { "engine", 7 }, { "wave", wDigi }, { "unison", 3 }, { "cutoff", 1500 }, { "attack", 0.8f }, { "sustain", 1 }, { "release", 2.5f },
                                                              { "crush", 0.3f }, { "wow", 0.3f }, { "lfoShape", 4 }, { "lfoRate", 6 }, { "lfoFilter", 0.35f }, { "lfo2Rate", 0.2f } } + MM (0, 4, 3, 0.3f)),
         { "Grain Cloud", "Texture", "Static Bloom" });
    add ("scifi", catFxAtmos, kFM, Y ("Data Stream", Values { { "engine", 1 }, { "fmRatio", 2.5f }, { "fmAmt", 0.15f }, { "cutoff", 2500 }, { "attack", 0.05f }, { "sustain", 1 }, { "release", 1 },
                                                        { "lfoShape", 4 }, { "lfoRate", 9 }, { "lfoPitch", 0.35f } }),
         { "Data Stream", "Robot Chatter", "Computer Talk" });
    add ("wind", catFxAtmos, kNoise | kBreath, Y ("Wind", Values { { "engine", 1 }, { "fmRatio", 7.13f }, { "wave", 1.0f }, { "fmAmt", 0.9f }, { "fsustain", 1 }, { "filterType", 4 }, { "reso", 0.5f }, { "cutoff", 700 }, { "attack", 1.2f }, { "sustain", 1 },
                                                           { "release", 2.5f }, { "lfoFilter", 0.4f }, { "lfoRate", 0.15f }, { "lfo2Rate", 0.4f } } + MM (0, 4, 9, 0.25f)),
         { "Desert Wind", "Wind", "Gust" });
    add ("zap", catFxAtmos, kHit | kSine, Y ("Laser", Values { { "engine", 7 }, { "wave", wTri }, { "cutoff", 5000 }, { "attack", 0.002f }, { "decay", 0.5f }, { "sustain", 0 }, { "release", 0.3f },
                                                       { "fdecay", 0.15f } } + MM (0, 1, 0, 1.0f) + MM (1, 1, 0, 1.0f)),
         { "Laser", "Zap", "Ray Blaster" });
    add ("impact", catFxAtmos, kHit | kSine, Y ("Impact", Values { { "engine", 7 }, { "wave", wSine }, { "octave", -2 }, { "cutoff", 3000 }, { "attack", 0.002f }, { "decay", 2.5f },
                                                            { "sustain", 0 }, { "release", 2 }, { "fdecay", 0.12f } } + MM (0, 1, 0, 1.0f) + B (5, 1.0f, 0, 0, 0.5f)),
         { "Impact", "Boom Hit", "Thunder Hit" });
    add ("braam", catFxAtmos, kHit | kOrch, Y ("Braam", Values { { "engine", 8 }, { "wave", 0.0f }, { "octave", -1 }, { "unison", 6 }, { "detune", 0.1f }, { "cutoff", 900 }, { "attack", 0.05f },
                                                          { "decay", 2.5f }, { "sustain", 0.3f }, { "release", 2 }, { "drive", 0.25f }, { "driveType", 1 } } + B (8, 0.0f, -1, 0, 0.7f)),
         { "Braam", "Doom Horn", "Titan Horn" });
    add ("metal", catFxAtmos, kModal, Y ("Metal Resonator", Values { { "engine", 9 }, { "wave", 0.9f }, { "fmAmt", 0.5f }, { "octave", -1 }, { "cutoff", 3500 }, { "decay", 4 }, { "sustain", 1 },
                                                              { "release", 3 }, { "body", 4 }, { "bodyMix", 0.4f } }),
         { "Gong Swell", "Metal Drone", "Iron Bell" });
    add ("wail", catFxAtmos, kSine, Y ("Wail", Values { { "engine", 7 }, { "wave", wSine }, { "cutoff", 4000 }, { "attack", 0.6f }, { "sustain", 1 }, { "release", 2 }, { "lfoShape", 1 },
                                                 { "lfoRate", 0.35f }, { "lfoPitch", 0.5f } }),
         { "Wail", "Distant Siren", "Ghost Call" });
    add ("heart", catFxAtmos, kSine | kHit, Y ("Pulse", Values { { "engine", 7 }, { "wave", wSine }, { "octave", -1 }, { "cutoff", 2000 }, { "attack", 0.01f }, { "sustain", 1 }, { "release", 1 },
                                                          { "lfoSync", 1 }, { "lfoDiv", 2 }, { "lfoShape", 0 }, { "lfoAmp", 0.9f } }),
         { "Heartbeat", "Pulse Field", "Low Throb" });
    // reversed real instruments: a slow attack over the decaying recording + reversed 1/8 grains + a big space
    auto rev = [] (const char* inst, const char* art, const char* label, float att, float cut)
    { return S (inst, art, label, { { "s.attack", att }, { "s.release", 0.25f }, { "s.cutoff", cut }, { "s.velSens", 0.25f }, { "reverse", 0.6f } }); };
    add ("revpiano", catFxAtmos, kSampleShot, rev ("piano.grand.concert", "sustain", "Concert Grand", 1.3f, 3500), { "Reversed Piano", "Backward Grand", "Rewind Piano" });
    add ("revharp", catFxAtmos, kSampleShot, rev ("pluck.harp.concert", "pluck", "Concert Harp", 1.0f, 3500), { "Reversed Harp", "Backward Harp", "Rewind Harp" });
    add ("revpizz", catFxAtmos, kSampleShot, rev ("strings.cellos.ensemble", "pizzicato", "Cello Section", 0.8f, 3000), { "Reversed Pizz", "Backward Pluck", "Suck Pizz" });
    add ("revmarimba", catFxAtmos, kSampleShot, rev ("mallets.marimba", "hit", "Marimba", 0.7f, 3500), { "Reversed Marimba", "Backward Wood", "Inward Marimba" });
    add ("revvibes", catFxAtmos, kSampleShot, rev ("mallets.vibraphone", "soft mallets", "Vibraphone", 1.0f, 3500), { "Reversed Vibes", "Backward Vibes", "Mirror Vibes" });
    add ("revchoir", catFxAtmos, kSampleSus, rev ("voice.choir.classic", "aahs", "Classic Choir", 1.8f, 2200), { "Reversed Choir", "Backward Voices", "Inhale Choir" });
    add ("revbell", catFxAtmos, kSampleShot, rev ("bells.tubular", "hit", "Tubular Bells", 1.4f, 3200), { "Reversed Bell", "Backward Bell", "Bell Inhale" });
    add ("revkalimba", catFxAtmos, kSampleShot, rev ("world.kalimba.low", "pluck", "Low Thumb Piano", 0.8f, 3500), { "Reversed Thumb Piano", "Backward Tines", "Inward Tines" });
    {
        auto g = S ("fx.glass", "fast", "Glass Harp", { { "s.attack", 0.4f }, { "s.release", 3.0f }, { "s.cutoff", 3500.0f }, { "s.velSens", 0.25f } });
        g.octave = 1;
        add ("glassatmos", catFxAtmos, kSampleSus, g, { "Glass Atmosphere", "Singing Rims", "Crystal Air" });
    }
    add ("tension", catFxAtmos, kSampleSus, S ("strings.violins.ensemble", "tremolo", "Violin Section", { { "s.attack", 2.5f }, { "s.release", 2.5f }, { "s.cutoff", 3000.0f },
                                                                                                      { "s.velSens", 0.25f } }),
         { "Tension Rise", "Nerve Strings", "Suspense Swell" });
    add ("timproll", catFxAtmos, kSampleSus, [] { auto t = S ("perc.timpani", "roll", "Timpani", { { "s.attack", 1.8f }, { "s.release", 2.0f }, { "s.velSens", 0.3f } }); t.octave = -1; return t; }(),
         { "Thunder Roll", "Timpani Swell", "Storm Roll" });
    add ("whisper", catFxAtmos, kModel, M ("voice.female", "Female Voice", { { "m.artic", 3 }, { "m.vowel", 1.0f } }), { "Whisper Air", "Haunted Breath", "Ghost Whisper" });
    add ("revguitar", catFxAtmos, kModel, M ("guitar.nylon", "Nylon Guitar", { { "m.tone", 0.4f }, { "reverse", 0.7f } }), { "Reversed Guitar", "Backward Nylon", "Rewind Strings" });
    add ("revharpsi", catFxAtmos, kSampleShot, rev ("keys.harpsichord.flemish", "lower 8'", "Flemish Harpsichord", 1.0f, 3500), { "Reversed Harpsichord", "Backward Quills", "Rewind Court" });
    add ("revchimes", catFxAtmos, kSampleShot, rev ("bells.handchimes", "hit", "Hand Chimes", 1.2f, 3500), { "Reversed Chimes", "Backward Chimes", "Chime Inhale" });
    add ("revglock", catFxAtmos, kSampleShot, [&] { auto g = rev ("mallets.glockenspiel", "hit", "Glockenspiel", 0.9f, 4000); g.octave = 2; return g; }(),
         { "Reversed Chime Bars", "Backward Sparkle", "Twinkle Inhale" });
    add ("revdulc", catFxAtmos, kSampleShot, rev ("pluck.dulcimer", "pluck", "Stick Dulcimer", 0.9f, 3500), { "Reversed Dulcimer", "Backward Folk", "Rewind Dulcimer" });
    add ("pedaldrone", catFxAtmos, kDrone | kSampleSus, [] { auto t = S ("organ.pipe", "pedal", "Pipe Organ", { { "s.attack", 2.0f }, { "s.release", 3.0f }, { "s.velSens", 0.25f } }); t.octave = -1; return t; }(),
         { "Pedal Drone", "Crypt Drone", "Undercroft Drone" });
    add ("didgdrone", catFxAtmos, kDrone | kSampleSus, [] { auto t = S ("world.didgeridoo", "drone", "Didgeridoo", { { "s.attack", 1.0f }, { "s.release", 2.5f }, { "s.velSens", 0.25f } }); t.octave = -2; return t; }(),
         { "Earth Drone", "Didgeridoo Drone", "Outback Drone" });
    add ("choirdrone", catFxAtmos, kDrone | kModel, [] { auto t = M ("voice.choir", "Choir", { { "m.voices", 12 }, { "m.gender", 0.3f }, { "m.vowel", 2.0f }, { "m.attack", 1.5f }, { "m.release", 3.0f } }); t.octave = -1; return t; }(),
         { "Choir Drone", "Requiem Drone", "Vigil Drone" });
    add ("bubbles", catFxAtmos, kSine, Y ("Bubbles", Values { { "engine", 7 }, { "wave", wSine }, { "cutoff", 1500 }, { "attack", 0.02f }, { "sustain", 1 }, { "release", 1 }, { "lfoShape", 4 },
                                                             { "lfoRate", 11 }, { "lfoPitch", 0.35f }, { "lfoAmp", 0.3f } }),
         { "Bubbles", "Underwater Bubbles", "Fizz" });
    add ("storm", catFxAtmos, kNoise, Y ("Storm", Values { { "engine", 1 }, { "fmRatio", 7.13f }, { "wave", 1.0f }, { "fmAmt", 0.95f }, { "fsustain", 1 }, { "octave", -1 }, { "cutoff", 350 },
                                                         { "attack", 1.5f }, { "sustain", 1 }, { "release", 3 }, { "lfoAmp", 0.4f }, { "lfoRate", 0.2f } }),
         { "Distant Thunder", "Storm", "Rumble" });
    add ("sqriser", catFxAtmos, kRiser | kSquare, Y ("Square Riser", Values { { "wave", 0.5f }, { "unison", 3 }, { "cutoff", 800 }, { "attack", 1.0f }, { "sustain", 1 }, { "release", 1.2f },
                                                                       { "e3attack", 3.5f }, { "e3sustain", 1 }, { "e3release", 1.2f } } + MM (0, 2, 0, 1.0f) + MM (1, 2, 1, 0.3f)),
         { "Square Riser", "Climb", "Ladder Up" });
    add ("fmriser", catFxAtmos, kRiser | kFM, Y ("FM Riser", Values { { "engine", 1 }, { "fmRatio", 2 }, { "fmAmt", 0.05f }, { "fenv", 0 }, { "fsustain", 1 }, { "cutoff", 3000 }, { "attack", 1.0f },
                                                                 { "sustain", 1 }, { "release", 1.2f }, { "e3attack", 4.0f }, { "e3sustain", 1 } } + MM (0, 2, 5, 0.4f) + MM (1, 2, 0, 0.5f)),
         { "Tension Riser", "Metal Riser", "Pressure Rise" });
    add ("syncrise", catFxAtmos, kRiser | kWave, Y ("Sync Riser", Values { { "engine", 7 }, { "wave", wSaw }, { "warpMode", 1 }, { "cutoff", 1800 }, { "attack", 0.8f }, { "sustain", 1 },
                                                                     { "release", 1.2f }, { "e3attack", 4.0f }, { "e3sustain", 1 } } + MM (0, 2, 5, 0.8f)),
         { "Sync Rise", "Tearing Rise", "Engine Rise" });
    add ("beacon", catFxAtmos, kSquare, Y ("Beacon", Values { { "wave", 0.5f }, { "cutoff", 1800 }, { "attack", 0.01f }, { "sustain", 1 }, { "release", 0.6f }, { "lfoSync", 1 }, { "lfoDiv", 3 },
                                                           { "lfoShape", 1 }, { "lfoAmp", 0.85f }, { "delayMix", 0.25f }, { "delayTime", 2 }, { "delayFb", 0.4f } }),
         { "Beacon", "Signal Pulse", "Morse Call" });
    add ("spacehit", catFxAtmos, kHit | kNoise, Y ("Space Hit", Values { { "engine", 1 }, { "fmRatio", 5.31f }, { "wave", 1.0f }, { "fmAmt", 0.9f }, { "fsustain", 1 }, { "cutoff", 1200 },
                                                                   { "attack", 0.004f }, { "decay", 0.5f }, { "sustain", 0 }, { "release", 0.4f } } + B (7, wSine, 0, 0, 0.6f)),
         { "Space Hit", "Cosmic Hit", "Star Burst" });
    add ("glitch", catFxAtmos, kWave, Y ("Glitch", Values { { "engine", 7 }, { "wave", wDigi }, { "cutoff", 2200 }, { "attack", 0.01f }, { "sustain", 1 }, { "release", 0.8f }, { "lfoShape", 4 },
                                                          { "lfoSync", 1 }, { "lfoDiv", 4 }, { "crush", 0.3f } } + MM (0, 3, 3, 0.6f) + MM (1, 3, 1, 0.3f)),
         { "Glitch Field", "Data Rain", "Broken Signal" });
    add ("shimdrone", catFxAtmos, kDrone | kWave, Y ("Shimmer Drone", Values { { "engine", 7 }, { "wave", wGlass }, { "unison", 4 }, { "detune", 0.18f }, { "cutoff", 2500 }, { "attack", 2.0f },
                                                                          { "sustain", 1 }, { "release", 3 }, { "lfoFilter", 0.2f }, { "lfoRate", 0.1f } } + B (7, wSine, 1, 0, 0.15f)),
         { "Shimmer Drone", "Halo Drone", "Starlight Drone" });
    add ("rumble", catFxAtmos, kDrone | kSine, Y ("Rumble", Values { { "engine", 7 }, { "wave", wTri }, { "octave", -2 }, { "cutoff", 600 }, { "attack", 1.0f }, { "sustain", 1 }, { "release", 2.5f },
                                                                { "lfoAmp", 0.35f }, { "lfoRate", 0.3f }, { "drive", 0.3f }, { "driveType", 1 } }),
         { "Low Rumble", "Deep Earth", "Cavern Rumble" });
    return v;
}

//==============================================================================
// treatments: processing roles
struct Treat
{
    const char* tag;
    juce::StringArray adjectives;
    uint32_t cats;                            // bit per category
    uint32_t kinds;                           // the voice kinds it suits
    std::vector<const char*> spaces;          // the spaces it is made with (first = best)
    std::function<void (VaultPreset&)> apply;
};
constexpr uint32_t C (int c) { return 1u << c; }
constexpr uint32_t cPl = 1u << catPlucks, cLe = 1u << catLeads, cPa = 1u << catPads, cSy = 1u << catSynth, cFx = 1u << catFxAtmos;

VaultPart& P0 (VaultPreset& p) { return p.parts.front(); }
// a synth-part second oscillator copy of osc A (octave / semitone / level), for synth voices only
void addOscB (VaultPart& a, int octave, int semi, float level, float fine = 0)
{
    setAll (a, { { "layerB", 1 }, { "engineB", getv (a, "engine", 0) }, { "waveB", getv (a, "wave", 0) }, { "octaveB", (float) octave }, { "semiB", (float) semi },
                 { "fineB", fine }, { "levelB", level }, { "fmAmtB", getv (a, "fmAmt", 0) }, { "fmRatioB", getv (a, "fmRatio", 1) },
                 { "fmRatio2B", getv (a, "fmRatio2", 1) }, { "fmAlgoB", getv (a, "fmAlgo", 0) }, { "warpModeB", getv (a, "warpMode", 0) },
                 { "unisonB", std::min (4.0f, getv (a, "unison", 1)) }, { "detuneB", getv (a, "detune", 0.15f) } });
}

std::vector<Treat> treatments()
{
    std::vector<Treat> t;
    // natural: as designed, heard in different spaces. Measured (first full run): modulation-only roles (tremolo, vibrato,
    // phaser, flanger, slow LFO drift), resonant bodies, tape wobble, scoops and second spaces almost never reach the
    // distinctness threshold - they are parameter jitter, so they are not made. Every role below changes the sound.
    t.push_back ({ "natural", { "" }, cPl | cLe | cPa | cSy | cFx, kAll, { "room", "hall", "echo", "cloud" }, [] (VaultPreset&) {} });
    //---------------------------------------------------------------- plucks / synth
    t.push_back ({ "short", { "Tight", "Staccato", "Short", "Clipped" }, cPl | cSy, kSynthAll | kSampleShot | kModel, { "dry" }, [] (VaultPreset& p)
    { decayScale (P0 (p), 0.42f); } });
    t.push_back ({ "long", { "Ringing", "Long", "Lingering", "Singing" }, cPl | cSy, kSynthAll | kSampleShot | kModel, { "hall" }, [] (VaultPreset& p)
    { decayScale (P0 (p), 2.2f); } });
    t.push_back ({ "muted", { "Muted", "Dusky", "Velvet", "Hushed" }, cPl | cSy | cLe | cPa, kAll, { "room" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        lowPass (a, currentLp (a) * 0.45f);
        velSens (a, std::min (1.0f, getVelSens (a) + 0.15f));
        fx (p, { { "eqLow", 1.5f }, { "eqHigh", -2.5f } });
    } });
    t.push_back ({ "echo", { "Echo", "Bouncing", "Ricochet", "Repeating" }, cPl | cLe | cSy, kAll, { "echo" }, [] (VaultPreset& p)
    { lowPass (P0 (p), currentLp (P0 (p)) * 0.75f); } });
    t.push_back ({ "lofi", { "Lo-Fi", "Dusty", "Crumbled", "Bedroom" }, cPl | cLe | cPa | cSy | cFx, kAll, { "room" }, [] (VaultPreset& p)
    {
        lowPass (P0 (p), 3200.0f);
        fx (p, { { "crush", 0.2f }, { "wow", 0.3f }, { "drive", 0.15f }, { "driveType", 1 }, { "width", 0.32f }, { "eqLow", 1.5f } });
    } });
    t.push_back ({ "wide", { "Wide", "Silk", "Liquid", "Panoramic" }, cPl | cLe | cPa | cSy, kAll, { "hall" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        if (isSynth (a) && getv (a, "unison", 1) < 2 && getv (a, "engine", 0) != 2 && getv (a, "engine", 0) != 9) setAll (a, { { "unison", 3 }, { "detune", 0.1f } });
        fx (p, { { "chorus", 0.5f }, { "width", 0.85f } });
    } });
    t.push_back ({ "octbody", { "Deep", "Doubled", "Thick", "Heavy" }, cPl | cLe | cSy | cPa, kSynthAll & ~kModal, { "room" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        if (getv (a, "layerB", 0) > 0.5f) setv (a, "levelB", 0.0f);
        addOscB (a, -1, 0, 0.55f);
    } });
    t.push_back ({ "fifth", { "Fifth", "Power", "Open", "Quintal" }, cPl | cSy | cPa | cLe, kSynthAll & ~kModal, { "room" }, [] (VaultPreset& p)
    { addOscB (P0 (p), 0, 7, 0.38f); } });
    t.push_back ({ "octup", { "Shining", "Overtone", "Sparkling", "Lifted" }, cPl | cLe | cPa | cSy, kSynthAll & ~kModal & ~kString, { "hall" }, [] (VaultPreset& p)
    { addOscB (P0 (p), 1, 0, 0.2f); } });
    t.push_back ({ "reverse", { "Reversed", "Backward", "Rewind", "Mirror" }, cPl | cPa | cSy | cFx, kAll, { "hall" }, [] (VaultPreset& p)
    {
        lowPass (P0 (p), currentLp (P0 (p)) * 0.7f);
        fx (p, { { "reverse", 0.55f } });
    } });
    t.push_back ({ "drive", { "Gritty", "Driven", "Burning", "Hot" }, cPl | cLe | cSy | cFx, kAll, { "room" }, [] (VaultPreset& p)
    {
        fx (p, { { "drive", 0.36f }, { "driveType", 0 }, { "eqHigh", -2.5f } });
        lowPass (P0 (p), currentLp (P0 (p)) * 0.6f);
    } });
    t.push_back ({ "bright", { "Bright", "Open", "Clear", "Crisp" }, cPl | cLe | cPa | cSy, kAll, { "room" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        scaleCut (a, 2.2f);
        if (isSample (a)) setv (a, "s.cutoff", 20000.0f);
        fx (p, { { "master", 0.25f } });
    } });
    t.push_back ({ "stab", { "Stabbing", "Punchy", "Struck", "Snapped" }, cSy | cPl, kSynthAll, { "room" }, [] (VaultPreset& p)
    {
        setAll (P0 (p), { { "decay", 0.28f }, { "sustain", 0.0f }, { "release", 0.2f }, { "fenv", 0.35f }, { "fdecay", 0.12f }, { "fsustain", 0 } });
        fx (p, { { "punchFx", 0.4f } });
    } });
    //---------------------------------------------------------------- leads
    t.push_back ({ "swell", { "Swelling", "Breathy", "Rising", "Bowed" }, cLe | cSy, kAll, { "hall" }, [] (VaultPreset& p)
    { attack (P0 (p), 0.18f); release (P0 (p), 0.6f); } });
    t.push_back ({ "hollow", { "Hollow", "Nasal", "Honky", "Pinched" }, cLe | cSy | cPl, kAnalog | kWave | kOrch, { "room" }, [] (VaultPreset& p)
    { setAll (P0 (p), { { "filterType", 4 }, { "reso", 0.22f } }); scaleCut (P0 (p), 1.3f); } });
    //---------------------------------------------------------------- pads / fx
    t.push_back ({ "dark", { "Dark", "Nocturnal", "Midnight", "Shadowed" }, cPa | cFx | cSy | cLe, kAll, { "hall" }, [] (VaultPreset& p)
    {
        lowPass (P0 (p), currentLp (P0 (p)) * 0.4f);
        fx (p, { { "eqLow", 2.0f }, { "eqHigh", -3.0f } });
    } });
    t.push_back ({ "airy", { "Airy", "Ethereal", "Floating", "Weightless" }, cPa | cFx, kAll, { "vast" }, [] (VaultPreset& p)
    { fx (p, { { "eqLow", -3.0f }, { "width", 0.8f } }); attack (P0 (p), 1.2f); } });
    t.push_back ({ "underwater", { "Submerged", "Sunken", "Underwater", "Deep Sea" }, cPa | cFx | cPl | cSy, kAll, { "cloud" }, [] (VaultPreset& p)
    { lowPass (P0 (p), 600.0f); fx (p, { { "chorus", 0.5f }, { "wow", 0.18f }, { "eqLow", 1.0f }, { "width", 0.7f } }); } });
    t.push_back ({ "gatedpad", { "Pulsing", "Rhythmic", "Gated", "Throbbing" }, cPa | cFx, kSynthAll, { "hall" }, [] (VaultPreset& p)
    {
        auto& a = P0 (p);
        setAll (a, { { "lfoSync", 1 }, { "lfoDiv", 4 }, { "lfoShape", 1 }, { "lfoAmp", 0.9f }, { "lfoPitch", 0.0f } });
        scaleCut (a, 1.6f);
        fx (p, { { "delayMix", 0.2f }, { "delayTime", 2 }, { "delayFb", 0.35f } });
    } });
    return t;
}

//==============================================================================
// layer recipes (a second / third / fourth part). `level` relative to part 0 after both are calibrated.
struct Layer
{
    const char* tag;
    juce::StringArray words;            // name words (prefix, or "& Noun" suffix)
    uint32_t cats;
    uint32_t kinds;                     // main voice kinds it suits
    const char* spaceTag;
    std::function<void (VaultPreset&)> add;
};
// layers below / at the played pitch are mixed clearly audible (1.4 x the table level, max 0.85) so a layer changes the
// sound; octave-up layers keep their table level (>= 12 dB under part 0)
VaultPart at (VaultPart p, float level, float pan = 0.0f, int octave = 0)
{
    p.level = octave > 0 ? level : std::min (0.85f, level * 1.4f);
    p.pan = pan; p.octave = octave;
    return p;
}
VaultPart sinePad (const char* label = "Sine Pad")
{ return Y (label, padEnv (5000, 0.6f, 2.2f) + Values { { "engine", 7 }, { "wave", wSine }, { "unison", 3 }, { "detune", 0.12f }, { "lfoFilter", 0 } }); }
VaultPart warmPadL() { return Y ("Warm Pad", padEnv (900) + Values { { "unison", 4 }, { "detune", 0.14f } }); }
VaultPart airPadL() { return Y ("Air Pad", padEnv (1600, 0.9f, 2.4f) + Values { { "engine", 5 }, { "wave", 0.8f } }); }
VaultPart glassPadL() { return Y ("Glass Pad", padEnv (2200, 0.8f, 2.4f) + Values { { "engine", 7 }, { "wave", wGlass }, { "unison", 3 } }); }
VaultPart subSine() { return Y ("Sine Bass", Values { { "engine", 7 }, { "wave", wSine }, { "attack", 0.004f }, { "decay", 0.5f }, { "sustain", 0.6f }, { "release", 0.3f }, { "cutoff", 2000 } }); }
VaultPart sawPluckL() { return Y ("Saw Pluck", pluckEnv (900, 0.4f, 0.16f, 0.6f)); }
VaultPart sampleShot (const char* inst, const char* art, const char* label, float cut = 4500)
{ return S (inst, art, label, { { "s.cutoff", cut }, { "s.velSens", 0.35f } }); }
VaultPart sampleSus (const char* inst, const char* art, const char* label, float att, float rel, float cut)
{ return S (inst, art, label, { { "s.attack", att }, { "s.release", rel }, { "s.cutoff", cut }, { "s.velSens", 0.22f } }); }

std::vector<Layer> layers()
{
    std::vector<Layer> L;
    auto push = [&] (VaultPreset& p, VaultPart part) { if ((int) p.parts.size() < maxParts) p.parts.push_back (std::move (part)); };
    //---------------------------------------------------------------- plucks
    L.push_back ({ "pizzhyb", { "Pizz", "Hybrid", "Bowstring" }, cPl | cSy, kAnalog | kWave | kFM | kString, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("strings.violins.ensemble", "pizzicato", "Violin Section"), 0.55f, 0.15f)); } });
    L.push_back ({ "harphyb", { "& Harp", "Harpstring", "Angelic" }, cPl, kFM | kWave | kSine | kString | kModal, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("pluck.harp.concert", "pluck", "Concert Harp", 4000), 0.5f, -0.15f)); } });
    L.push_back ({ "marhyb", { "& Marimba", "Rosewood", "Mallet" }, cPl | cSy, kFM | kSaw | kSquare | kWave | kOrgan, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("mallets.marimba", "hit", "Marimba"), 0.5f, 0.1f)); } });
    L.push_back ({ "kalhyb", { "& Thumb Piano", "Thumb", "Tine" }, cPl, kSine | kWave | kSquare | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("world.kalimba.low", "pluck", "Low Thumb Piano"), 0.5f, -0.1f)); } });
    L.push_back ({ "vibhyb", { "& Vibes", "Vibe", "Lounge" }, cPl | cSy, kFM | kSine | kWave | kModel, "plate",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("mallets.vibraphone", "soft mallets", "Vibraphone", 4000), 0.45f, 0.15f)); } });
    L.push_back ({ "guitarhyb", { "& Nylon", "Spanish", "Guitar" }, cPl, kAnalog | kWave | kFM | kSine, "room",
                   [=] (VaultPreset& p) { push (p, at (M ("guitar.nylon", "Nylon Guitar", { { "m.tone", 0.45f } }), 0.5f, 0.12f)); } });
    L.push_back ({ "subpluck", { "Sub", "Low End", "Weighted" }, cPl, kAnalog | kWave | kFM | kString, "room",
                   [=] (VaultPreset& p) { push (p, at (subSine(), 0.45f, 0.0f, -1)); } });
    L.push_back ({ "padpluck", { "& Pad", "Cushioned", "Padded" }, cPl | cLe, kAll & ~kSampleSus, "hall",
                   [=] (VaultPreset& p) { push (p, at (warmPadL(), 0.33f)); } });
    L.push_back ({ "airpluck", { "& Air", "Misty", "Breath" }, cPl | cSy, kAll & ~kSampleSus, "cloud",
                   [=] (VaultPreset& p) { push (p, at (airPadL(), 0.3f)); } });
    L.push_back ({ "sparkle", { "Sparkle", "Twinkle", "Glint" }, cPl | cSy, kAnalog | kString | kFM | kModal | kSampleShot, "plate",
                   [=] (VaultPreset& p) { push (p, at (Y ("Glass Pluck", pluckEnv (4000, 0.0f, 0.2f, 0.6f, 0.3f) + Values { { "engine", 1 }, { "fmRatio", 4 }, { "fmAmt", 0.15f } }), 0.18f, 0.2f, 1)); } });
    L.push_back ({ "ksdouble", { "Twin String", "Double", "Paired" }, cPl, kAnalog | kWave | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (Y ("Plucked String", Values { { "engine", 2 }, { "wave", 0.45f }, { "cutoff", 4000 }, { "decay", 1.1f }, { "sustain", 1 }, { "release", 0.5f } }), 0.55f, -0.2f)); } });
    L.push_back ({ "dulchyb", { "Folk", "Hammered", "Rustic" }, cPl, kSaw | kSquare | kFM | kModal | kString, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("pluck.dulcimer", "pluck", "Stick Dulcimer"), 0.5f, 0.15f)); } });
    L.push_back ({ "ksmallet", { "Bell", "Gamelan", "Chime" }, cPl, kString | kSaw | kSquare, "plate",
                   [=] (VaultPreset& p) { push (p, at (Y ("Thumb Piano Synth", Values { { "engine", 9 }, { "wave", 0.15f }, { "fmAmt", 0.2f }, { "decay", 0.7f }, { "sustain", 1 }, { "release", 0.4f } }), 0.45f)); } });
    //---------------------------------------------------------------- leads
    L.push_back ({ "flutelead", { "& Flute", "Fluted", "Pastoral" }, cLe, kSine | kSquare | kBreath | kWave | kPulse, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("winds.flute.concert", "vibrato", "Concert Flute", 0.04f, 0.3f, 3500), 0.55f)); } });
    L.push_back ({ "harmlead", { "& Harmonica", "Blues", "Harp Lead" }, cLe, kSquare | kPulse | kSaw | kOrgan, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("winds.harmonica.chromatic", "normal", "Chromatic Harmonica", 0.02f, 0.25f, 3000), 0.45f)); } });
    L.push_back ({ "saxlead", { "& Sax", "Smoky", "Saxed" }, cLe, kSaw | kSquare | kFM | kOrch | kVox, "plate",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("winds.sax.soprano", "vibrato", "Curved Soprano Sax", 0.03f, 0.3f, 2800), 0.45f)); } });
    L.push_back ({ "violinlead", { "& Violin", "Fiddle", "Gypsy" }, cLe, kSaw | kSuper | kOrch | kSine | kWave, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("strings.violin.solo", "sustain", "Solo Violin", 0.06f, 0.4f, 3000), 0.5f)); } });
    L.push_back ({ "mutelead", { "Muted Trumpet", "Harmon", "Noir" }, cLe, kSquare | kPulse | kSaw | kOrch | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("brass.trumpet", "harmon mute", "Trumpet", 0.03f, 0.3f, 2800), 0.45f)); } });
    L.push_back ({ "voicelead", { "& Voice", "Siren", "Diva" }, cLe, kSine | kWave | kBreath | kSquare | kVox, "plate",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("voice.female.solo", "oo", "Female Solo Voice", 0.05f, 0.4f, 3000), 0.5f)); } });
    L.push_back ({ "octdown", { "Octave", "Twin Octave", "Low Double" }, cLe, kSynthAll, "room",
                   [=] (VaultPreset& p) { auto b = p.parts.front(); for (auto k : { "revMix", "delayMix", "chorus", "drive", "crush", "wow" }) b.set (k, 0.0f); push (p, at (b, 0.6f, 0.0f, -1)); } });
    L.push_back ({ "octupl", { "Shine", "Overtone", "Halo" }, cLe, kSaw | kSquare | kPulse | kSine | kWave, "plate",
                   [=] (VaultPreset& p) { push (p, at (Y ("Sine Lead", leadEnv (6000) + Values { { "engine", 7 }, { "wave", wSine } }), 0.2f, 0.0f, 1)); } });
    L.push_back ({ "whistlel", { "Whistling", "Piped", "Bird" }, cLe, kSaw | kSquare | kPulse | kOrch | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (Y ("Whistle", leadEnv (5000) + Values { { "engine", 7 }, { "wave", wSine } } + B (5, 0.6f, 0, 0, 0.35f)), 0.55f)); } });
    L.push_back ({ "sqlead2", { "Reinforced", "Doubled Square", "Twin Square" }, cLe, kSine | kBreath | kVox | kWave, "room",
                   [=] (VaultPreset& p) { push (p, at (Y ("Square Lead", leadEnv (1800) + Values { { "wave", 0.5f } }), 0.45f, 0.1f)); } });
    //---------------------------------------------------------------- pads
    L.push_back ({ "strpad", { "& Strings", "Orchestral", "Bowed" }, cPa, kSaw | kSuper | kPulse | kWave | kSine | kVox | kModel, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("strings.violins.ensemble", "sustain", "Violin Section", 0.9f, 2.4f, 3200), 0.45f, 0.15f)); } });
    L.push_back ({ "cellopadl", { "& Cellos", "Low Bowed", "Earthen" }, cPa, kSaw | kSuper | kSine | kWave | kFM | kSampleSus, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("strings.cellos.ensemble", "sustain", "Cello Section", 1.0f, 2.4f, 2200), 0.45f, -0.15f)); } });
    L.push_back ({ "choirpadl", { "& Choir", "Choral", "Hymnal" }, cPa, kSaw | kSuper | kSine | kWave | kOrgan | kSampleSus, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("voice.choir.classic", "oohs", "Classic Choir", 0.8f, 2.4f, 2200), 0.4f)); } });
    L.push_back ({ "organpadl", { "& Organ", "Sacred", "Vespers" }, cPa, kSaw | kSine | kVox | kSampleSus | kModel, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("organ.chamber", "8 foot", "Chamber Organ", 0.9f, 2.4f, 2400), 0.4f)); } });
    L.push_back ({ "glasspadl", { "Glass", "Shimmer", "Frost" }, cPa, kSaw | kSuper | kSine | kSampleSus | kVox | kModel | kOrch, "vast",
                   [=] (VaultPreset& p) { auto g = sampleSus ("fx.glass", "slow", "Glass Harp", 0.8f, 2.4f, 3500); push (p, at (g, 0.18f, 0.2f, 1)); } });
    L.push_back ({ "bowedvib", { "Bowed Bars", "Aura", "Halo" }, cPa, kSaw | kSine | kWave | kSampleSus | kFM, "hall",
                   [=] (VaultPreset& p) { auto g = sampleSus ("mallets.vibraphone", "bowed", "Bowed Vibraphone", 0.6f, 2.4f, 3500); push (p, at (g, 0.3f, -0.15f)); } });
    L.push_back ({ "sinesupport", { "Warm", "Rounded", "Velvet" }, cPa, kSampleSus | kVox | kOrch | kBreath, "hall",
                   [=] (VaultPreset& p) { push (p, at (sinePad(), 0.6f)); } });
    L.push_back ({ "subpad", { "Earth", "Grounded", "Low" }, cPa, kAll & ~kModel, "hall",
                   [=] (VaultPreset& p) { push (p, at (sinePad ("Sub Pad"), 0.4f, 0.0f, -1)); } });
    L.push_back ({ "vocpadl", { "& Voices", "Angel", "Vocal" }, cPa, kSaw | kSuper | kSine | kWave | kFM | kSampleSus, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (M ("voice.pad", "Vocal Pad", { { "m.velSens", 0.4f } }), 0.4f)); } });
    L.push_back ({ "fluteair", { "& Flute", "Woodland", "Pastoral" }, cPa, kSaw | kSine | kWave | kSampleSus, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("winds.flute.concert", "sustain", "Concert Flute", 0.8f, 2.2f, 3000), 0.35f, 0.1f)); } });
    L.push_back ({ "femalepad", { "Siren", "Lullaby", "Sung" }, cPa, kSaw | kSine | kWave | kSampleSus, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("voice.female.solo", "oo", "Female Solo Voice", 0.9f, 2.4f, 2800), 0.35f)); } });
    L.push_back ({ "hornpadl", { "& Horns", "Heroic", "Distant Horn" }, cPa, kSampleSus | kSaw | kOrch, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("brass.horn", "sustain", "French Horn", 1.0f, 2.2f, 1800), 0.35f, -0.1f)); } });
    L.push_back ({ "cinepad", { "Cinematic", "Film Score", "Epic" }, cPa, kSaw | kSuper | kSine | kWave, "cathedral",
                   [=] (VaultPreset& p) {
                       push (p, at (sampleSus ("strings.violins.ensemble", "sustain", "Violin Section", 0.9f, 2.4f, 3000), 0.35f, 0.2f));
                       push (p, at (sampleSus ("strings.cellos.ensemble", "sustain", "Cello Section", 1.0f, 2.4f, 2000), 0.35f, -0.2f));
                       push (p, at (sampleSus ("voice.choir.classic", "oohs", "Classic Choir", 0.8f, 2.4f, 2000), 0.25f)); } });
    L.push_back ({ "heaven", { "Heavenly", "Sanctuary", "Seraph" }, cPa, kSampleSus | kSine | kVox, "cathedral",
                   [=] (VaultPreset& p) {
                       push (p, at (sampleSus ("organ.pipe", "soft", "Pipe Organ", 1.0f, 2.4f, 2200), 0.35f));
                       auto g = sampleSus ("fx.glass", "slow", "Glass Harp", 0.8f, 2.4f, 3500); push (p, at (g, 0.15f, 0.2f, 1)); } });
    //---------------------------------------------------------------- synth
    L.push_back ({ "pianohyb", { "Hybrid", "Piano", "Grand" }, cSy, kSaw | kSuper | kSquare | kPulse | kWave | kFM, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("piano.grand.studio", "sustain", "Studio Grand", 3500), 0.55f)); } });
    L.push_back ({ "hornstab", { "Brass Hit", "Horn Stab", "Big Band" }, cSy, kSaw | kSuper | kOrch, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("brass.horn", "staccato", "French Horn", 2500), 0.5f)); } });
    L.push_back ({ "spicstab", { "Spiccato", "Bouncing Bow", "String Hit" }, cSy, kSaw | kSuper | kSquare | kOrch, "hall",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("strings.violins.ensemble", "spiccato", "Violin Section", 3500), 0.5f, 0.15f)); } });
    L.push_back ({ "orchhitl", { "Orchestra Hit", "Tutti", "Stinger" }, cSy, kSaw | kSuper | kOrch, "hall",
                   [=] (VaultPreset& p) {
                       push (p, at (sampleShot ("strings.violins.ensemble", "spiccato", "Violin Section", 3500), 0.45f, 0.2f));
                       push (p, at (sampleShot ("brass.trombone", "staccato", "Tenor Trombone", 2200), 0.45f, -0.2f)); } });
    L.push_back ({ "harpsihyb", { "Baroque", "Courtly", "Harpsi" }, cSy, kPulse | kSquare | kSaw, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleShot ("keys.harpsichord.flemish", "lower 8'", "Flemish Harpsichord", 4500), 0.45f, 0.15f)); } });
    L.push_back ({ "organhyb", { "Gospel", "Church", "Revival" }, cSy, kSaw | kSuper | kSquare, "room",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("organ.pipe", "principal", "Pipe Organ", 0.01f, 0.3f, 2600), 0.4f)); } });
    L.push_back ({ "eplayer", { "& EP", "Electric", "Tine" }, cSy, kSaw | kSuper | kPulse | kWave | kVox, "room",
                   [=] (VaultPreset& p) { push (p, at (M ("ep.tine.mellow", "Tine Electric Piano", { { "m.tone", 0.45f } }), 0.5f)); } });
    L.push_back ({ "padsy", { "& Pad", "Padded", "Wash" }, cSy, kAll & ~kSampleSus, "hall",
                   [=] (VaultPreset& p) { push (p, at (warmPadL(), 0.3f)); } });
    L.push_back ({ "subsy", { "Low", "Sub", "Weighted" }, cSy, kSynthAll, "room",
                   [=] (VaultPreset& p) { push (p, at (subSine(), 0.4f, 0.0f, -1)); } });
    L.push_back ({ "pluckon", { "Plucked", "Attack", "Transient" }, cSy, kSaw | kSuper | kOrch | kVox | kOrgan, "room",
                   [=] (VaultPreset& p) { push (p, at (sawPluckL(), 0.45f, 0.1f)); } });
    //---------------------------------------------------------------- fx
    L.push_back ({ "riserstack", { "Stacked", "Tidal", "Surging" }, cFx, kRiser, "cathedral",
                   [=] (VaultPreset& p) {
                       push (p, at (Y ("Noise Sweep", Values { { "engine", 1 }, { "fmRatio", 5.31f }, { "wave", 1.0f }, { "fmAmt", 0.85f }, { "fsustain", 1 }, { "filterType", 4 }, { "reso", 0.3f }, { "cutoff", 400 }, { "attack", 1.5f }, { "sustain", 1 },
                                                         { "release", 1.5f }, { "e3attack", 4.5f }, { "e3sustain", 1 } } + MM (0, 2, 1, 0.42f)), 0.5f)); } });
    L.push_back ({ "impactsub", { "Boom", "Seismic", "Quake" }, cFx, kHit | kSampleShot, "cathedral",
                   [=] (VaultPreset& p) { push (p, at (Y ("Sub Drop", Values { { "engine", 7 }, { "wave", wSine }, { "attack", 0.003f }, { "decay", 1.8f }, { "sustain", 0 }, { "release", 1.5f },
                                                                       { "fdecay", 0.25f } } + MM (0, 1, 0, 0.6f)), 0.5f, 0.0f, -2)); } });
    L.push_back ({ "timpimpact", { "Timpani", "Thunder", "War Drum" }, cFx, kHit | kRiser | kDrone, "cathedral",
                   [=] (VaultPreset& p) { auto t = sampleShot ("perc.timpani", "hit", "Timpani", 3000); push (p, at (t, 0.5f, 0.0f, -1)); } });
    L.push_back ({ "dronechoir", { "Haunted", "Spectral", "Ghostly" }, cFx, kDrone | kNoise | kSampleSus, "vast",
                   [=] (VaultPreset& p) { push (p, at (M ("voice.choir", "Choir", { { "m.voices", 8 }, { "m.vowel", 2.0f } }), 0.4f, 0.0f, -1)); } });
    L.push_back ({ "dronestr", { "Dread", "Ominous", "Brooding" }, cFx, kDrone | kNoise | kSine, "vast",
                   [=] (VaultPreset& p) { push (p, at (sampleSus ("strings.cellos.ensemble", "tremolo", "Cello Section", 1.5f, 2.5f, 1500), 0.45f, 0.0f, -1)); } });
    L.push_back ({ "revpianol", { "Memory", "Echoes", "Afterimage" }, cFx, kSampleShot | kNoise | kWave, "vast",
                   [=] (VaultPreset& p) { push (p, at (S ("piano.grand.concert", "sustain", "Concert Grand", { { "s.attack", 1.3f }, { "s.release", 0.3f }, { "s.cutoff", 3000.0f } }), 0.45f)); } });
    L.push_back ({ "windl", { "Windswept", "Stormy", "Gale" }, cFx, kDrone | kSampleSus | kModal | kSine, "vast",
                   [=] (VaultPreset& p) { push (p, at (Y ("Wind", Values { { "engine", 1 }, { "fmRatio", 7.13f }, { "wave", 1.0f }, { "fmAmt", 0.9f }, { "fsustain", 1 }, { "filterType", 4 }, { "reso", 0.5f }, { "cutoff", 700 }, { "attack", 1.2f }, { "sustain", 1 },
                                                                 { "release", 2.5f }, { "lfoFilter", 0.4f }, { "lfoRate", 0.15f } }), 0.45f)); } });
    L.push_back ({ "glassl", { "Glimmer", "Starlit", "Prism" }, cFx, kDrone | kNoise | kSampleShot | kRiser, "vast",
                   [=] (VaultPreset& p) { auto g = sampleSus ("fx.glass", "fast", "Glass Harp", 0.6f, 2.5f, 3500); push (p, at (g, 0.25f, 0.2f, 1)); } });
    return L;
}

//==============================================================================
// names: unique, <= 22 characters, no repeated word
struct Namer
{
    std::set<juce::String> used;
    bool take (const juce::String& n)
    {
        if (n.length() > 22 || n.isEmpty()) return false;
        if (vaultaudit::nameProblem (n).isNotEmpty() || n.containsIgnoreCase ("808")) return false;
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
// spare evocative words per category for names that would otherwise collide
const juce::StringArray& spareWords (int cat)
{
    static const juce::StringArray pl { "Neon", "Amber", "Velvet", "Silver", "Coral", "Lunar", "Pastel", "Ivory", "Copper", "Indigo", "Jade", "Opal", "Cobalt", "Sable" };
    static const juce::StringArray le { "Neon", "Night", "Chrome", "Solar", "Crimson", "Electric", "Royal", "Golden", "Cyan", "Scarlet", "Midnight", "Static", "Sonic" };
    static const juce::StringArray pa { "Northern", "Distant", "Endless", "Silent", "Ocean", "Polar", "Stellar", "Dusk", "Dawn", "Nebula", "Lunar", "Twilight", "Ember" };
    static const juce::StringArray sy { "Neon", "Retro", "Chrome", "City", "Night", "Arcade", "Laser", "Velvet", "Turbo", "Prism", "Vapor", "Sunset", "Metro" };
    static const juce::StringArray fx { "Void", "Orbit", "Abyss", "Eclipse", "Vortex", "Signal", "Phantom", "Rift", "Echo Chamber", "Horizon", "Nova", "Quasar" };
    switch (cat)
    {
        case catPlucks: return pl;
        case catLeads: return le;
        case catPads: return pa;
        case catSynth: return sy;
        default: return fx;
    }
}

//==============================================================================
// candidates
struct Cand
{
    VaultPreset preset;
    juce::String recipe;
    std::vector<juce::String> nameOptions;
};

// does every sample part play at C4, C5 and C6 (the gate renders them; a pad silent at C4 is a broken pad)?
bool playable (const RecipeContext& ctx, const VaultPart& p)
{
    if (! isSample (p)) return true;
    if (! ctx.hasInstrument (p.source)) return false;
    const auto& d = ctx.library->find (p.source)->desc;
    const int ai = d.articulationIndex (p.articulation);
    if (! juce::isPositiveAndBelow (ai, (int) d.articulations.size())) return false;
    int lo = 128, hi = -1;
    for (auto& z : d.articulations[(size_t) ai].zones) if (! z.releaseTrigger) { lo = std::min (lo, z.lo); hi = std::max (hi, z.hi); }
    const int t = p.octave * 12 + p.semi;
    return lo <= homeNote - 12 + t && hi >= homeNote + 12 + t;
}
bool playableMain (const RecipeContext& ctx, const VaultPart& p)
{
    if (! isSample (p)) return true;
    if (! ctx.hasInstrument (p.source)) return false;
    const auto& d = ctx.library->find (p.source)->desc;
    const int ai = d.articulationIndex (p.articulation);
    int lo = 128, hi = -1;
    for (auto& z : d.articulations[(size_t) ai].zones) if (! z.releaseTrigger) { lo = std::min (lo, z.lo); hi = std::max (hi, z.hi); }
    const int t = p.octave * 12 + p.semi;
    return lo <= homeNote + t && hi >= homeNote + t;
}

std::vector<Cand> candidates (const RecipeContext& ctx)
{
    const auto V = voices();
    const auto T = treatments();
    const auto L = layers();
    std::vector<Cand> out;
    // 1. voice x treatment x space: round robin over voices, so every voice is explored evenly
    std::vector<std::vector<Cand>> perVoice (V.size());
    for (size_t vi = 0; vi < V.size(); ++vi)
    {
        auto& v = V[vi];
        if (! (v.category == catFxAtmos ? playableMain (ctx, v.part) : playable (ctx, v.part))) continue;
        for (auto& t : T)
        {
            if ((t.cats & C (v.category)) == 0 || (t.kinds & v.kind) == 0) continue;
            int si = 0;
            for (auto* sp : t.spaces)
            {
                juce::String spT (sp);
                // pads and atmospheres live in big spaces (a treatment made in a room is heard in a hall); plucks / leads /
                // stabs not in cathedrals
                if ((v.category == catPads || v.category == catFxAtmos) && (spT == "dry" || spT == "slap" || spT == "room"))
                {
                    if (juce::String (t.tag) == "natural") { ++si; continue; }
                    spT = "hall";
                    sp = "hall";
                }
                if ((v.category == catPlucks || v.category == catLeads || v.category == catSynth) && (spT == "cathedral" || spT == "vast")) { ++si; continue; }
                if (juce::String (t.tag) == "natural" && spT != "room" && spT != "echo" && spT != "cloud" && spT != "hall") { ++si; continue; }
                Cand c;
                c.preset.category = v.category;
                c.preset.parts = { v.part };
                fx (c.preset, space (sp).fx);
                t.apply (c.preset);
                c.recipe = juce::String (v.tag) + "/" + t.tag + "/" + sp;
                const juce::String sw = space (sp).word;
                for (int ai = 0; ai < t.adjectives.size(); ++ai)
                    for (auto& noun : v.nouns)
                    {
                        const auto adj = t.adjectives[t.adjectives[0].isEmpty() ? ai : (ai + (int) vi) % t.adjectives.size()];
                        if (adj.isEmpty() && si == 0) c.nameOptions.push_back (noun);
                        c.nameOptions.push_back (joinWords ({ adj, noun }));
                        c.nameOptions.push_back (joinWords ({ adj, sw, noun }));
                        c.nameOptions.push_back (joinWords ({ sw, noun }));
                        c.nameOptions.push_back (joinWords ({ noun, sw }));
                    }
                perVoice[vi].push_back (std::move (c));
                ++si;
            }
        }
    }
    // 2. layers: voice x layer recipe (natural treatment + the layer's space)
    std::vector<std::vector<Cand>> perLayer (L.size());
    for (size_t li = 0; li < L.size(); ++li)
    {
        auto& l = L[li];
        int rot = 0;
        for (auto& v : V)
        {
            if ((l.cats & C (v.category)) == 0 || (l.kinds & v.kind) == 0) continue;
            if (! (v.category == catFxAtmos ? playableMain (ctx, v.part) : playable (ctx, v.part))) continue;
            Cand c;
            c.preset.category = v.category;
            c.preset.parts = { v.part };
            l.add (c.preset);
            bool ok = true;
            for (size_t k = 1; k < c.preset.parts.size(); ++k) ok &= playable (ctx, c.preset.parts[k]) || playableMain (ctx, c.preset.parts[k]);
            if (! ok) continue;
            fx (c.preset, space (l.spaceTag).fx);
            if (c.preset.parts.size() >= 3) lowPass (c.preset.parts.front(), currentLp (c.preset.parts.front()) * 0.8f);
            c.recipe = juce::String (v.tag) + "+" + l.tag;
            ++rot;
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
    const auto A = flatten (perVoice), Bv = flatten (perLayer);
    // every voice as designed first (round 0: its natural sound), so a plain voice is never pushed out by its own layered
    // or treated variants; then 2 treated voices : 1 layer recipe, round robin
    size_t first = 0;
    for (auto& l : perVoice) if (! l.empty()) ++first;
    for (size_t a = 0; a < first; ++a) out.push_back (A[a]);
    for (size_t a = first, b = 0; a < A.size() || b < Bv.size();)
    {
        for (int k = 0; k < 2 && a < A.size(); ++k) out.push_back (A[a++]);
        if (b < Bv.size()) out.push_back (Bv[b++]);
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
                                                 "revMix", "revSize", "revType", "eqLow", "eqHigh", "reverse", "freeze", "width", "master", "gain", "macro2", "macro5",
                                                 "body", "bodyMix", "punchFx" };
    return fxKeys.count (id) > 0;
}
void neutralFx (VaultPart& p)
{
    if (! isSynth (p)) return;
    setAll (p, { { "revMix", 0.0f }, { "master", 0.0f }, { "macro2", 0.0f }, { "macro5", 0.0f }, { "delayMix", 0.0f }, { "chorus", 0.0f } });
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
    solo.id = "cal"; solo.name = "cal"; solo.category = catSynth;
    VaultPart p = part;
    p.level = 1.0f; p.pan = 0.0f;
    Values keep;
    for (auto& [id, x] : p.values) if (! isFxKey (id)) keep.push_back ({ id, x });
    p.values = keep;
    neutralFx (p);
    solo.parts = { p };
    auto b = recipegate::render (solo, homeNote, 0.8f, 2.5, 3.5);
    const float loud = vaultqc::analyse (b, recipegate::sr, 2.5).loud;
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
// smoothed spectral envelope) per time segment over C5 (vel 0.8, 7 segments incl. the release), C5 vel 0.35 (2), C4 (3),
// C6 (3) + stereo width + modulation (amplitude / pan / spectral fluctuation), relative to the C5 render's loudest segment.
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
        for (int q = 0; q < ownCeps; ++q)
        {
            double c = 0;
            for (int m = 0; m < ownMel; ++m) c += std::max (-70.0, band[(size_t) m] - ref) * std::cos (juce::MathConstants<double>::pi * q * (m + 0.5) / ownMel);
            out.push_back ((float) (c * std::sqrt ((q == 0 ? 1.0 : 2.0) / ownMel)));
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
float detrendedSd (const std::vector<double>& y)
{
    const int n = (int) y.size();
    if (n < 8) return 0;
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int i = 0; i < n; ++i) { sx += i; sy += y[(size_t) i]; sxx += (double) i * i; sxy += i * y[(size_t) i]; }
    const double bb = (n * sxy - sx * sy) / std::max (1e-9, n * sxx - sx * sx), a = (sy - bb * sx) / n;
    double e = 0;
    for (int i = 0; i < n; ++i) { const double r = y[(size_t) i] - (a + bb * i); e += r * r; }
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
    const auto c5 = recipegate::render (p, homeNote, 0.8f, 1.5, 3.0);
    const auto soft = recipegate::render (p, homeNote, 0.35f, 1.5, 1.5);
    const auto c4 = recipegate::render (p, homeNote - 12, 0.8f, 1.2, 2.0);
    const auto c6 = recipegate::render (p, homeNote + 12, 0.8f, 1.2, 2.0);
    float mx = 0, dummy = 0;
    auto f = mfccSegments (c5, { { 0.0, 0.03 }, { 0.03, 0.1 }, { 0.1, 0.3 }, { 0.3, 0.7 }, { 0.7, 1.5 }, { 1.5, 2.0 }, { 2.0, 3.0 } }, 0, true, mx);
    for (auto x : mfccSegments (soft, { { 0.0, 0.1 }, { 0.1, 0.7 } }, mx, false, dummy)) f.push_back (x);
    for (auto* b : { &c4, &c6 })
        for (auto x : mfccSegments (*b, { { 0.0, 0.1 }, { 0.1, 0.6 }, { 0.6, 1.2 } }, mx, false, dummy)) f.push_back (x);
    f.push_back (0.3f * juce::jlimit (-40.0f, 0.0f, sideToMid (c5, 0.05, 1.5)));
    f.push_back (0.3f * juce::jlimit (-40.0f, 0.0f, sideToMid (c5, 1.5, 3.0)));
    const float w = std::sqrt ((float) ownMel * 5.0f);
    for (auto m : modulation (c5)) f.push_back (w * m);
    return f;
}
float ownDistance (const std::vector<float>& a, const std::vector<float>& b)
{
    double s = 0;
    const size_t n = std::min (a.size(), b.size());
    for (size_t i = 0; i < n; ++i) { const double d = a[i] - b[i]; s += d * d; }
    const double segs = (double) (n - 5) / ownCeps;
    return (float) std::sqrt (s / std::max (1.0, segs * ownMel));
}

//==============================================================================
// repairs after a gate failure (false when nothing more can be done)
bool repair (VaultPreset& p, const juce::String& fail, int attempt)
{
    auto& a = p.parts.front();
    if (fail == "fundamental")
    {
        // darker first; from the 2nd attempt a sine at the played pitch carries the fundamental (synth: osc B / sample: a part)
        for (auto& part : p.parts) lowPass (part, std::min (currentLp (part), 8000.0f) * 0.6f);
        a.set ("eqLow", std::min (6.0f, a.get ("eqLow", 0.0f) + 2.0f));
        if (attempt >= 1)
        {
            bool hasSine = false;
            for (auto& part : p.parts) hasSine |= part.label == "Sine";
            if (! hasSine && (int) p.parts.size() < maxParts)
            {
                auto s = Y ("Sine Support", Values { { "engine", 7 }, { "wave", wSine }, { "attack", std::max (0.004f, getv (a, isSample (a) ? "s.attack" : "attack", 0.004f)) },
                                              { "decay", getv (a, "decay", 0.6f) }, { "sustain", isSynth (a) ? getv (a, "sustain", 0.8f) : 0.8f },
                                              { "release", isSynth (a) ? getv (a, "release", 0.3f) : 0.6f }, { "cutoff", 3000 } });
                s.label = "Sine";
                s.octave = a.octave;
                s.level = 0.55f;
                p.parts.push_back (s);
            }
            else
                for (auto& part : p.parts) if (part.label == "Sine") part.level = std::min (1.2f, part.level * 1.5f);
        }
        return attempt < 4;
    }
    if (fail == "harsh-2-5k" || fail == "piercing-highs")
    {
        for (auto& part : p.parts) lowPass (part, std::min (currentLp (part), 8000.0f) * 0.65f);
        a.set ("eqHigh", std::max (-8.0f, a.get ("eqHigh", 0.0f) - 2.0f));
        a.set ("drive", a.get ("drive", 0.0f) * 0.7f);
        a.set ("crush", a.get ("crush", 0.0f) * 0.6f);
        return true;
    }
    if (fail == "velocity")
    {
        for (auto& part : p.parts) velSens (part, std::min (1.0f, getVelSens (part) + 0.25f));
        a.set ("drive", a.get ("drive", 0.0f) * 0.6f);
        if (attempt >= 1) for (auto& part : p.parts) if (isSynth (part)) part.set ("mmSrc8", 5), part.set ("mmDst8", 9), part.set ("mmAmt8", 0.5f);
        return attempt < 3;
    }
    if (fail == "velocity-wide")
    {
        // recorded dynamics (pp / ff layers) already give the range: less extra velocity -> level on top
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.velSens", attempt == 0 ? 0.05f : 0.0f);
            else velSens (part, getVelSens (part) * 0.6f);
        return attempt < 2;
    }
    if (fail == "click-start")
    {
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.attack", std::max (0.005f, part.get ("s.attack", 0.0f)));
            else if (isSynth (part)) part.set ("attack", std::max (0.008f, part.get ("attack", 0.003f) * 2.0f));
        return attempt < 3;
    }
    if (fail == "click-end" && a.get ("s.releaseSamples", 1.0f) > 0.5f && isSample (a))
    {
        for (auto& part : p.parts) if (isSample (part)) part.set ("s.releaseSamples", 0.0f);
        return true;
    }
    if (fail == "click-spike" && (a.get ("wow", 0.0f) > 0.05f || a.get ("crush", 0.0f) > 0.05f || a.get ("lfoAmp", 0.0f) > 0.05f))
    {
        a.set ("wow", a.get ("wow", 0.0f) * 0.4f); a.set ("crush", a.get ("crush", 0.0f) * 0.4f); a.set ("lfoAmp", a.get ("lfoAmp", 0.0f) * 0.5f);
        return true;
    }
    if (fail == "click-end" || fail == "click-spike")
    {
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.release", std::max (0.25f, part.get ("s.release", 0.0f)));
            else if (isSynth (part)) part.set ("release", std::max (0.25f, part.get ("release", 0.3f)));
            else part.set ("m.release", std::max (0.25f, part.get ("m.release", 0.3f)));
        return attempt < 2;
    }
    if (fail == "tail")
    {
        a.set ("revMix", a.get ("revMix", 0.0f) * 0.7f); a.set ("revSize", a.get ("revSize", 0.0f) * 0.8f); a.set ("delayFb", a.get ("delayFb", 0.35f) * 0.6f);
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.release", part.get ("s.release", 0.0f) > 0.0f ? part.get ("s.release", 0.0f) * 0.6f : 2.5f);
            else if (isSynth (part)) { part.set ("release", part.get ("release", 0.3f) * 0.6f); if (part.get ("engine", 0) == 2 || part.get ("engine", 0) == 9) part.set ("decay", part.get ("decay", 1.0f) * 0.7f); }
            else part.set ("m.release", std::min (1.0f, part.get ("m.release", 0.5f)));
        return attempt < 4;
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
        for (auto& part : p.parts)
        {
            part.pan *= 0.5f;
            if (isSample (part)) part.set ("s.width", 0.6f);
            if (isSynth (part) && part.get ("unison", 1) > 1) part.set ("detune", part.get ("detune", 0.15f) * 0.6f);
        }
        return attempt < 3;
    }
    if (fail == "too-peaky" || fail == "clip")
    {
        for (auto& part : p.parts)
            if (isSample (part)) part.set ("s.attack", std::max (0.008f, part.get ("s.attack", 0.0f)));
            else if (isSynth (part)) part.set ("attack", std::max (0.01f, part.get ("attack", 0.003f)));
        a.set ("punchFx", 0.0f);
        return attempt < 2;
    }
    if (fail == "silent") return false;
    return false;
}

int envInt (const char* n, int def) { auto s = juce::SystemStats::getEnvironmentVariable (n, {}); return s.isNotEmpty() ? s.getIntValue() : def; }
float envFloat (const char* n, float def) { auto s = juce::SystemStats::getEnvironmentVariable (n, {}); return s.isNotEmpty() ? (float) s.getDoubleValue() : def; }

struct Done
{
    Cand cand;
    recipegate::Result res;
    std::vector<float> own;
    int repairs = 0;
};

//==============================================================================
// SYNTH_PINMAP=<shipped v2 bank>: which candidate (recipe tag) each shipped preset of these categories came from. Within a
// category the v2 search numbered the ids in candidate order, so the tags are found in id order among the candidates whose
// name options contain the shipped name, with the same layers (the repair's "Sine" support part aside), at increasing
// candidate positions. Prints "id|recipe|name" lines (the pinnedV2 table below).
bool isMine (int c) { return c == catPlucks || c == catLeads || c == catPads || c == catSynth || c == catFxAtmos; }
juce::String normName (juce::String n) { return n.replace (" and ", " & ").toLowerCase(); }
juce::String layerSig (const VaultPreset& p)
{
    juce::String s;
    for (auto& part : p.parts) if (part.label != "Sine") s << (int) part.kind << ":" << part.source << ":" << part.articulation << ":" << part.label << ":" << part.octave << "/";
    return s;
}
float valueDistance (const VaultPart& a, const VaultPart& b)
{
    static const std::set<juce::String> loose { "gain", "cutoff", "s.cutoff", "m.tone", "attack", "release", "decay", "s.attack", "s.release", "velSens", "s.velSens",
                                                "m.velSens", "detune", "drive", "crush", "revMix", "revSize", "delayFb", "width", "chorus", "wow", "lfoAmp", "mmSrc8",
                                                "mmDst8", "mmAmt8", "punchFx", "s.releaseSamples", "s.width", "m.release", "s.decay", "s.sustain", "sustain", "fdecay" };
    float d = 0;
    std::set<juce::String> keys;
    for (auto& [k, v] : a.values) keys.insert (k);
    for (auto& [k, v] : b.values) keys.insert (k);
    for (auto& k : keys)
    {
        const float x = a.get (k, -12345.0f), y = b.get (k, -12345.0f);
        const float w = loose.count (k) ? 0.05f : (k == "eqLow" || k == "eqHigh") ? 0.3f : 1.0f;
        if (x < -12000.0f || y < -12000.0f) d += w;
        else d += w * std::abs (x - y) / (std::abs (x) + std::abs (y) + 1.0e-3f);
    }
    return d;
}
void pinMap (const RecipeContext& ctx, const juce::File& bankFile)
{
    juce::MemoryBlock mb;
    std::vector<VaultPreset> bank;
    if (! bankFile.loadFileAsData (mb) || ! vault::deserializeBank (mb.getData(), mb.getSize(), bank, nullptr, nullptr, true)) { std::printf ("pinmap: cannot read %s\n", bankFile.getFullPathName().toRawUTF8()); return; }
    const auto cands = candidates (ctx);
    std::map<int, int> lastIdx;
    std::vector<VaultPreset> mine;
    for (auto& p : bank) if (isMine (p.category)) mine.push_back (p);
    std::sort (mine.begin(), mine.end(), [] (const VaultPreset& a, const VaultPreset& b) { return a.id < b.id; });
    int unmatched = 0, ambiguous = 0;
    for (auto& p : mine)
    {
        const auto nn = normName (p.name);
        const auto sig = layerSig (p);
        int best = -1, count = 0;
        float bestD = 1e9f;
        for (int i = lastIdx.count (p.category) ? lastIdx[p.category] + 1 : 0; i < (int) cands.size(); ++i)
        {
            const auto& c = cands[(size_t) i];
            if (c.preset.category != p.category || layerSig (c.preset) != sig) continue;
            bool nameOk = false;
            for (auto& n : c.nameOptions) if (normName (n) == nn) { nameOk = true; break; }
            if (! nameOk)
            {
                // a spare-word name ("Neon ...") contains one of the options' nouns
                for (auto& n : c.nameOptions) if (nn.endsWith (normName (n.fromLastOccurrenceOf (" ", false, false))) && nn.contains (normName (n.upToFirstOccurrenceOf (" ", false, false)))) { nameOk = true; break; }
                if (! nameOk) continue;
            }
            ++count;
            const float d = valueDistance (c.preset.parts.front(), p.parts.front());
            if (best < 0 || d < bestD - 0.5f) { best = i; bestD = d; }
            if (count >= 6) break;
        }
        if (best < 0) { ++unmatched; std::printf ("PINMISS %s|%s\n", p.id.toRawUTF8(), p.name.toRawUTF8()); continue; }
        if (count > 1) ++ambiguous;
        lastIdx[p.category] = best;
        std::printf ("PIN    \"%s|%s|%s\",%s\n", p.id.toRawUTF8(), cands[(size_t) best].recipe.toRawUTF8(), p.name.toRawUTF8(), count > 1 ? ("   // of " + juce::String (count)).toRawUTF8() : "");
    }
    std::printf ("pinmap: %d presets, %d unmatched, %d with several options\n", (int) mine.size(), unmatched, ambiguous);
}
} // namespace

// the v2 search (SYNTH_V2_SEARCH=1): candidates -> gate + repairs -> greedy distinctness -> names / ids
static std::vector<VaultPreset> buildV2Search (const RecipeContext& ctx)
{
    const float ownMin = envFloat ("SYNTH_OWN_MIN", 2.6f), sharedMin = envFloat ("SYNTH_SHARED_MIN", 3.6f);
    const float ownMax25 = 0.08f, ownMax6k = 0.02f, ownMaxVel = 20.0f;   // velocity 0.35 -> 1.0: a soft note must stay usable
    const int threads = envInt ("SYNTH_THREADS", (int) std::max (1u, std::thread::hardware_concurrency()));
    if (auto pm = juce::SystemStats::getEnvironmentVariable ("SYNTH_PINMAP", {}); pm.isNotEmpty()) { pinMap (ctx, juce::File (pm)); return {}; }
    auto cands = candidates (ctx);
    if (auto cs = juce::SystemStats::getEnvironmentVariable ("SYNTH_CATS", {}); cs.isNotEmpty())
    {
        const auto keep = juce::StringArray::fromTokens (cs, ",", "");
        cands.erase (std::remove_if (cands.begin(), cands.end(), [&] (const Cand& c) { return ! keep.contains (juce::String (c.preset.category)); }), cands.end());
    }
    cands.erase (std::remove_if (cands.begin(), cands.end(), [&] (const Cand& c)
    {
        for (auto& part : c.preset.parts) if (isSample (part) && ! ctx.hasInstrument (part.source)) return true;
        return false;
    }), cands.end());
    if (const int lim = envInt ("SYNTH_LIMIT", 0); lim > 0 && lim < (int) cands.size()) cands.resize ((size_t) lim);
    std::printf ("synth: %d candidates, %d threads\n", (int) cands.size(), threads);

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
                p.id = "cv.tmp." + juce::String (i); p.name = "Draft " + juce::String (i).replaceCharacters ("0123456789", "QWXZJPVYHB");   // digits could spell a banned number
                calibrate (p);
                if (juce::String why; ! vault::validatePreset (p, &why)) { d.res.fail = "invalid: " + why; std::printf ("  synth INVALID %s %s\n", d.cand.recipe.toRawUTF8(), why.toRawUTF8()); continue; }
                for (int attempt = 0; attempt < 6; ++attempt)
                {
                    d.res = recipegate::Result();
                    d.res.preset = p;
                    recipegate::analyse (d.res);
                    if (d.res.fail.isEmpty() && d.res.share25 > ownMax25) d.res.fail = "harsh-2-5k";
                    if (d.res.fail.isEmpty() && d.res.share6k > ownMax6k) d.res.fail = "piercing-highs";
                    if (d.res.fail.isEmpty() && d.res.velRange > ownMaxVel) d.res.fail = "velocity-wide";
                    if (d.res.fail.isEmpty() || d.res.fail.startsWith ("name") || d.res.fail == "label") break;
                    if (! repair (p, d.res.fail, attempt)) break;
                    calibrate (p);
                    ++d.repairs;
                }
                if (d.res.fail.isEmpty()) d.own = ownPrint (p);
                if (++count % 100 == 0) std::printf ("  synth %d / %d\n", count.load(), (int) cands.size());
            }
        });
    for (auto& t : pool) t.join();

    // greedy selection in candidate order: gate passed + distinct from everything accepted on BOTH fingerprints
    std::vector<int> acc;
    std::map<juce::String, int> rejects;
    std::map<int, int> catCount;
    // category sizes: a balanced bank (candidates are interleaved over voices, so the first N accepted cover every voice)
    auto catCap = [] (int c) { return c == catPlucks ? envInt ("SYNTH_CAP_PLUCKS", 300) : c == catSynth ? envInt ("SYNTH_CAP_SYNTH", 270) : 100000; };
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
        if (++catCount[d.cand.preset.category] > catCap (d.cand.preset.category)) { why[(size_t) i] = "category full"; ++rejects["category full"]; continue; }
        acc.push_back (i);
    }

    // names, ids
    Namer namer;
    std::vector<VaultPreset> out;
    std::vector<int> outIdx;
    std::map<int, int> perCat;
    for (int i : acc)
    {
        auto& d = done[(size_t) i];
        VaultPreset p = d.cand.preset;
        juce::String name;
        for (auto& n : d.cand.nameOptions) if (namer.take (n)) { name = n; break; }
        if (name.isEmpty())
            for (auto& w : spareWords (p.category))
            {
                for (auto& n : d.cand.nameOptions)
                {
                    const auto noun = n.fromLastOccurrenceOf (" ", false, false);
                    const auto tryName = joinWords ({ w, n.upToLastOccurrenceOf (" ", false, false).fromLastOccurrenceOf (" ", false, false), noun });
                    if (namer.take (tryName)) { name = tryName; break; }
                    if (namer.take (w + " " + n)) { name = w + " " + n; break; }
                }
                if (name.isNotEmpty()) break;
            }
        if (name.isEmpty()) { why[(size_t) i] = "no name"; ++rejects["no name"]; continue; }
        p.name = name;
        const int n = ++perCat[p.category];
        p.id = juce::String ("cv.") + vault::categorySlug (p.category) + "." + juce::String (n).paddedLeft ('0', 4);
        p.gain = 1.0f;
        out.push_back (p);
        outIdx.push_back (i);
        d.cand.preset = p;
    }

    float minOwn = 1e9f, minShared = 1e9f;
    juce::String ownA, ownB, shA, shB;
    for (size_t x = 0; x < outIdx.size(); ++x)
        for (size_t y = x + 1; y < outIdx.size(); ++y)
        {
            auto& a = done[(size_t) outIdx[x]]; auto& b = done[(size_t) outIdx[y]];
            const float o = ownDistance (a.own, b.own), s = recipegate::distance (a.res.print, b.res.print);
            if (o < minOwn) { minOwn = o; ownA = a.cand.preset.name; ownB = b.cand.preset.name; }
            if (s < minShared) { minShared = s; shA = a.cand.preset.name; shB = b.cand.preset.name; }
        }
    std::printf ("synth: accepted %d of %d (Plucks %d, Leads %d, Pads %d, Synth %d, FX & Atmos %d)\n", (int) out.size(), (int) done.size(), perCat[catPlucks],
                 perCat[catLeads], perCat[catPads], perCat[catSynth], perCat[catFxAtmos]);
    std::printf ("synth: min pairwise own distance %.2f (%s / %s), shared %.2f (%s / %s); thresholds own %.2f shared %.2f\n",
                 minOwn, ownA.toRawUTF8(), ownB.toRawUTF8(), minShared, shA.toRawUTF8(), shB.toRawUTF8(), ownMin, sharedMin);
    for (auto& [w, n] : rejects) std::printf ("  synth rejected %-16s %d\n", w.toRawUTF8(), n);
    {
        std::map<int, int> layersCount;
        for (auto& p : out) ++layersCount[(int) p.parts.size()];
        std::printf ("synth: parts per preset:");
        for (auto& [k, n] : layersCount) std::printf (" %d-part %d", k, n);
        std::printf ("\n");
    }
    for (int cat : { (int) catPlucks, (int) catLeads, (int) catPads, (int) catSynth, (int) catFxAtmos })
    {
        std::vector<float> h1, s25, s6, cen, vel, tail;
        for (int i : outIdx)
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
    if (auto rp = juce::SystemStats::getEnvironmentVariable ("SYNTH_REPORT", {}); rp.isNotEmpty())
    {
        juce::String s ("idx\trecipe\tresult\tname\tid\tparts\trepairs\tloud\tH1\t2-5k%\t>6k%\tcentroid\ttail\tvel\toctUp\tnearShared\tnearOwn\n");
        for (int i = 0; i < (int) done.size(); ++i)
        {
            auto& d = done[(size_t) i];
            const bool ok = why[(size_t) i].isEmpty() && std::find (outIdx.begin(), outIdx.end(), i) != outIdx.end();
            s << i << "\t" << d.cand.recipe << "\t" << (ok ? juce::String ("ok") : why[(size_t) i]) << "\t" << (ok ? d.cand.preset.name : juce::String()) << "\t"
              << (ok ? d.cand.preset.id : juce::String()) << "\t" << (int) d.cand.preset.parts.size() << "\t" << d.repairs << "\t" << juce::String (d.res.loud, 1) << "\t"
              << juce::String (d.res.h1Db, 1) << "\t" << juce::String (100 * d.res.share25, 2) << "\t" << juce::String (100 * d.res.share6k, 2) << "\t"
              << (int) d.res.centroid << "\t" << juce::String (d.res.tail, 2) << "\t" << juce::String (d.res.velRange, 1) << "\t"
              << juce::String (d.res.octUpDb, 1) << "\t" << juce::String (nearShared[(size_t) i] < 1e8f ? nearShared[(size_t) i] : 0.0f, 2) << "\t"
              << juce::String (nearOwn[(size_t) i] < 1e8f ? nearOwn[(size_t) i] : 0.0f, 2) << "\n";
        }
        juce::File (rp).replaceWithText (s);
        std::printf ("synth: report %s\n", rp.toRawUTF8());
    }
    return out;
}

//==============================================================================
// v3 SOUND DESIGN (see the v3 design note at the top of this file)
namespace
{
namespace v3
{
using namespace recipe;
using FT = vault::FxType;
using R = fx::Reverb;

constexpr float cap25 = 0.075f, cap6k = 0.018f, cap8k = 0.006f, wantH1 = -2.0f;

// what a preset is: category, voice (tag, kind), treatment + space or layer recipe, pattern (arp / gate presets)
struct Role
{
    int cat = 0;
    juce::String voice, treat = "natural", space = "room", layer, fam;
    uint32_t kind = 0;
    bool sample = false, model = false;
    int pattern = 0;   // 1..6: the rhythm presets (see patterns())
};

// the chain, one slot per type, in the canonical (v2 rack) order
struct Chain
{
    std::map<int, VaultFxSlot> slots;
    explicit Chain (const std::vector<VaultFxSlot>& converted) { for (auto& x : converted) if (! has (x.type)) put (x); }
    static int rank (FT t) { return vault::fxTypeInfo (t).rank; }
    bool has (FT t) const { return slots.count (rank (t)) > 0; }
    VaultFxSlot& put (const VaultFxSlot& x) { return slots[rank (x.type)] = x; }
    VaultFxSlot& at (FT t) { if (! has (t)) put (VaultFxSlot::make (t)); return slots[rank (t)]; }
    void drop (FT t) { slots.erase (rank (t)); }
    float get (FT t, const char* id) const { return has (t) ? slots.at (rank (t)).get (id) : VaultFxSlot::make (t).get (id); }
    void eqAdd (const char* id, float db) { auto& e = at (FT::eq); e.set (id, juce::jlimit (-18.0f, 18.0f, e.get (id) + db)); }
    std::vector<VaultFxSlot> build() const
    {
        std::vector<VaultFxSlot> v;
        for (auto& [r, x] : slots) v.push_back (x);
        for (auto t : { FT::transient, FT::flanger, FT::width, FT::compressor })   // at most 8 slots: the least important go first
            if (v.size() > (size_t) vault::maxFxSlots)
                for (auto it = v.begin(); it != v.end(); ++it) if (it->type == t) { v.erase (it); break; }
        if (v.size() > (size_t) vault::maxFxSlots) v.resize ((size_t) vault::maxFxSlots);
        return v;
    }
};

VaultFxSlot rv (R type, float size, float decay, float mix, float predelay, float damping, float lowCut = 200.0f)
{ return fx::reverb (type, size, decay, mix).set ("predelay", predelay).set ("damping", damping).set ("lowCut", lowCut); }
VaultFxSlot dl (const char* div, float fb, float mix, fx::Delay mode = fx::Delay::pingPong, float hi = 4500.0f, float lo = 350.0f)
{ return fx::delay (div, fb, mix, mode).set ("hiCut", hi).set ("loCut", lo); }

// a sound macro whose default reproduces the designed sound exactly (every target sits at its designed value at the knob's
// default d and moves `span` in total around it)
struct M
{
    VaultMacro m;
    float d;
    M (const char* name, float dflt) : d (dflt) { m.name = name; }
    M& rel (const char* target, float span) { m.targets.push_back ({ target, -span * d, span * (1.0f - d), 0.0f }); return *this; }
    // (a value sitting on a bound - a silent slot's 0 - keeps the full span: the engine clamps, so the knob only acts on
    // the open side of its default)
    M& abs (const char* target, float v, float span, float lo, float hi)
    {
        const float edge = 0.02f * (hi - lo);
        const bool atLo = v - lo < edge, atHi = hi - v < edge;
        if (d > 1.0e-4f && ! atLo) span = std::min (span, (v - lo) / d);
        if (d < 1.0f - 1.0e-4f && ! atHi) span = std::min (span, (hi - v) / (1.0f - d));
        span = std::max (0.0f, span);
        m.targets.push_back ({ target, v - span * d, v + span * (1.0f - d), 0.0f });
        return *this;
    }
    M& toMax (const char* target, float v, float atMax)
    {
        const float span = d < 1.0f - 1.0e-4f ? (atMax - v) / (1.0f - d) : 0.0f;
        m.targets.push_back ({ target, v - span * d, atMax, 0.0f });
        return *this;
    }
    M& fxp (const Chain& c, FT t, const char* id, float span)
    {
        if (! c.has (t)) return *this;
        const auto& ti = vault::fxTypeInfo (t);
        const auto& pi = ti.params[(size_t) ti.indexOf (id)];
        const auto target = juce::String ("fx.") + ti.id + "." + id;
        return abs (target.toRawUTF8(), c.get (t, id), span, pi.lo, pi.hi);
    }
};
void setMacro (VaultPreset& p, int i, const M& m) { p.macros[(size_t) i] = m.m; p.macroDefaults[i] = m.d; }
bool lfoTo (const VaultLfo& l, const char* target) { for (auto& d : l.dests) if (d.target == target) return true; return false; }

//------------------------------------------------------------------------------ families
juce::String family (const juce::String& v, int cat, uint32_t kind)
{
    auto in = [&] (std::initializer_list<const char*> l) { for (auto* x : l) if (v == x) return true; return false; };
    switch (cat)
    {
        case catPlucks:
            if (in ({ "harppluck", "pizzpluck", "dulcpluck", "zitherpl", "mbirapl", "quillpl", "mutedgtr", "harmonicpl" })) return "real";
            if (in ({ "marimbapl", "modalmar", "modalkal", "celestapl", "steelpan", "fmwood" })) return "mallet";
            if (in ({ "fmglass", "fmtine", "fmnylon", "fmmetal", "fmbellpl", "wtprism", "droplet" })) return "glass";
            if (in ({ "kswire", "ksgut", "kszither", "sitarpluck", "organks" })) return "string";
            if (in ({ "chippluck" })) return "chip";
            if (in ({ "organpluck", "breathpluck", "voxpluck", "orchpizz", "brasspl" })) return "color";
            return "analog";   // saw / square / pulse / supersaw / ladder / band / duo / pwm / digital / triangle / sync plucks
        case catLeads:
            if (in ({ "modellead", "ohlead", "tenorlead", "breathylead" })) return "voice";
            if (in ({ "dist", "fold", "fuzzsine", "lowglide" })) return "dist";
            if (in ({ "whistle", "flutelead", "glidesine", "theremin", "softsq", "cleanflute", "pipelead", "organlead" })) return "soft";
            if (in ({ "chip", "pulsechip" })) return "chip";
            if (in ({ "fmlead", "wtlead", "synclead", "glasslead", "belllead", "laserlead", "brasslead", "voxlead" })) return "digital";
            return "analog";
        case catPads:
            if (in ({ "violinpad", "violapad", "cellopad", "tremopad", "cellotrem" })) return "strings";
            if (in ({ "choirpad", "aahpad", "voxpad", "choirmodel", "malechoir", "femchoir", "vocalpadm", "vocalpadee" })) return "choir";
            if (in ({ "pipepad", "chamberpad", "fullorgpad", "fourpad", "principalpad", "organpad" })) return "organ";
            if (in ({ "hornpad", "clarpad", "bassoonpad", "saxpad", "harmpad" })) return "winds";
            if (in ({ "glassharp", "shimmer", "glasspad", "bellpad", "halo", "digipad", "fmpad", "softpad" })) return "shine";
            if (in ({ "vowelpad", "synthchoir", "oohpad" })) return "vocal";
            if (in ({ "strmachine" })) return "machine";
            if (in ({ "breathpad", "noisepad" })) return "air";
            return "warm";   // warm / pwm / morph / dark / hollow / sync / brass / bloom / square
        case catSynth:
            if (in ({ "sqstab", "superstab", "orchhit", "voxstab", "chippoly" })) return "stab";
            if (in ({ "brass", "fmbrass" })) return "brass";
            if (in ({ "fmkeys", "sinekeys", "stackfm", "epsynth", "clavsynth", "glasspoly" })) return "keys";
            if (in ({ "houseorgan", "organsaw", "reedsynth" })) return "organ";
            if (in ({ "polystr", "pulsestr", "sawchoir" })) return "strings";
            if (in ({ "gated", "pump", "seq5", "seqoct" })) return "rhythm";
            return "poly";
        default:
            if (kind & kRiser) return "riser";
            if (kind & kDrone) return "drone";
            if (kind & kHit) return "hit";
            if (v.startsWith ("rev")) return "reversed";
            if (in ({ "glassatmos", "tension", "timproll" })) return "swell";
            return "texture";
    }
}

// the room a v2 space becomes (pads / atmospheres a little bigger and longer)
VaultFxSlot spaceReverb (const juce::String& s, int cat)
{
    const bool big = cat == catPads || cat == catFxAtmos;
    const float lc = cat == catFxAtmos ? 160.0f : big ? 280.0f : 250.0f;
    const float mk = big ? 1.12f : 1.0f;
    if (s == "dry")       return rv (R::room, 0.28f, 0.7f, 0.08f, 4, 0.4f, lc);
    if (s == "room")      return rv (R::room, 0.45f, 1.0f, 0.15f * mk, 8, 0.35f, lc);
    if (s == "plate")     return rv (R::plate, 0.5f, 1.0f, 0.19f * mk, 15, 0.3f, lc);
    if (s == "hall")      return rv (R::hall, big ? 0.7f : 0.6f, big ? 1.25f : 1.05f, 0.22f * mk, 22, 0.35f, lc);
    if (s == "cloud")     return rv (R::cloud, 0.65f, 1.1f, 0.3f * mk, 28, 0.4f, lc);
    if (s == "cathedral") return rv (R::hall, 0.88f, 1.5f, 0.32f, 35, 0.4f, lc);
    if (s == "vast")      return rv (R::cloud, 0.92f, 1.7f, 0.38f, 40, 0.45f, lc);
    if (s == "echo")      return rv (R::room, 0.42f, 0.95f, 0.12f, 8, 0.4f, lc);
    if (s == "ping")      return rv (R::hall, 0.5f, 1.0f, 0.14f, 15, 0.35f, lc);
    if (s == "slap")      return rv (R::room, 0.32f, 0.8f, 0.1f, 4, 0.4f, lc);
    if (s == "dub")       return rv (R::cloud, 0.6f, 1.1f, 0.2f, 20, 0.5f, lc);
    return rv (R::plate, 0.5f, 1.0f, 0.18f, 15, 0.3f, lc);
}
bool echoSpace (const juce::String& s) { return s == "echo" || s == "ping" || s == "slap" || s == "dub"; }
VaultFxSlot spaceDelay (const juce::String& s)
{
    if (s == "echo") return dl ("1/8D", 0.36f, 0.18f, fx::Delay::tape, 4000, 350);
    if (s == "ping") return dl ("1/8", 0.32f, 0.2f, fx::Delay::pingPong, 4500, 400);
    if (s == "slap") return fx::delayMs (110, 0.12f, 0.16f, fx::Delay::straight).set ("hiCut", 5000.0f).set ("loCut", 300.0f);
    return dl ("1/4D", 0.5f, 0.24f, fx::Delay::tape, 3500, 350);   // dub
}

// LFO roles
VaultLfo vibrato (float depth, float delay = 0.35f) { return lfo (VaultLfo::sine).rate (5.4f).delay (delay).fade (0.6f).depth (depth).to ("pitch", 0.25f); }
VaultLfo drift (float depth, float rate, float cut, float pan) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", cut).to ("pan", pan); }
VaultLfo sampleDrift (float depth, float rate = 0.12f) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", 0.3f).to ("pan", 0.22f).to ("fx.eq.tilt", 0.08f); }
VaultLfo wobble (float depth, float rate = 0.55f) { return lfo (VaultLfo::triangle).rate (rate).free().depth (depth).to ("pitch", 0.09f); }
VaultLfo tremolo (float depth, float rate = 4.6f) { return lfo (VaultLfo::sine).rate (rate).depth (depth).to ("volume", 0.45f).to ("pan", 0.35f); }

//------------------------------------------------------------------------------ 1. the chain and the movement
// variant 0 = family polish + role (the design); a preset that came out too close to an earlier one is designed again:
// 1 = the v2 space and colours with the family's tone (comp / drive / EQ) only, 2 = the v2 colours only (+ movement, macros),
// 3 = the v2 sound (+ macros)
void designSound (VaultPreset& p, const Role& r, int variant = 0)
{
    design (p);   // v2 -> v3: the space, the treatment's colour slots and the repair's EQ arrive here
    const std::vector<VaultFxSlot> converted = p.fx;
    Chain c (p.fx);
    p.mod = {};
    p.arp = {};
    p.gate = {};
    const auto& f = r.fam;
    const auto& t = r.treat;
    const int cat = r.cat;
    float finish = std::max (0.05f, p.finish);
    const bool synthMain = p.parts.front().kind == VaultPartKind::synth;
    auto mid = [&] (float db, float hz, float q) { auto& e = c.at (FT::eq); e.set ("mid", e.get ("mid") + db).set ("midFreq", hz).set ("midQ", q); };
    auto comp = [&] (float th, float ra, float at, float re, float mk, float mix) { c.put (fx::comp (th, ra, at, re, mk, mix)); };
    auto widen = [&] (float w, float mono) { if (! c.has (FT::width) || c.get (FT::width, "width") < w) c.put (fx::width (w, mono)); else c.at (FT::width).set ("monoBelow", mono); };
    auto chorusIfNone = [&] (VaultFxSlot s) { if (! c.has (FT::chorus)) c.put (s); };
    const bool space = variant == 0;              // variant 1 / 2 keep the converted space
    auto putSpace = [&] (VaultFxSlot s) { if (space) c.put (s); };
    auto putDelay = [&] (VaultFxSlot s) { if (space && ! echoSpace (r.space)) c.put (s); };

    // the room of the v2 space, rebuilt with the v3 reverb types (and the echo spaces' delay)
    if (space)
    {
        c.put (spaceReverb (r.space, cat));
        if (echoSpace (r.space)) c.put (spaceDelay (r.space));
        else c.drop (FT::delay);
    }

    //---------------------------------------------------------------- the family's finished sound
    if (cat == catPlucks)
    {
        if (f == "real") { comp (-24, 2.0f, 12, 150, 1.0f, 0.5f); mid (-1.0f, 2800, 1.0f); putDelay (dl ("1/8D", 0.24f, 0.08f, fx::Delay::pingPong, 4000, 400)); }
        else if (f == "mallet") { comp (-22, 2.5f, 8, 130, 1.0f, 0.55f); mid (-1.5f, 450, 0.9f); putDelay (dl ("1/4", 0.28f, 0.09f, fx::Delay::pingPong, 4000, 400)); }
        else if (f == "glass") { comp (-24, 2.0f, 10, 160, 0.5f, 0.5f); mid (-1.5f, 3200, 1.0f); putDelay (dl ("1/4D", 0.32f, 0.11f, fx::Delay::pingPong, 4000, 450)); widen (0.62f, 160); }
        else if (f == "string") { comp (-24, 2.0f, 12, 160, 0.5f, 0.5f); mid (-1.0f, 3000, 1.0f); putDelay (dl ("1/8D", 0.26f, 0.09f, fx::Delay::pingPong, 4000, 400)); }
        else if (f == "chip") { putDelay (dl ("1/16", 0.3f, 0.12f, fx::Delay::pingPong, 5000, 400)); c.eqAdd ("high", -1.0f); }
        else if (f == "color") { comp (-22, 2.5f, 10, 140, 1.0f, 0.55f); putDelay (dl ("1/8D", 0.26f, 0.1f, fx::Delay::pingPong, 4000, 400)); }
        else
        {   // analog / digital synth plucks: the classic pluck chain - glue, a touch of tape, ping-pong 1/8D, plate, width
            comp (-22, 2.5f, 8, 120, 1.0f, 0.55f);
            if (! c.has (FT::drive)) c.put (fx::drive (fx::Drive::tape, 0.06f, 0.5f, 7000).set ("cleanLow", 1.0f));
            mid (-1.5f, 3000, 0.9f);
            putDelay (dl ("1/8D", 0.3f, 0.12f, fx::Delay::pingPong, 4500, 400));
            widen (0.62f, 150);
        }
    }
    else if (cat == catLeads)
    {
        if (f == "voice") { comp (-22, 3.0f, 8, 120, 1.5f, 0.7f); mid (-1.5f, 3000, 1.0f); chorusIfNone (fx::chorus (0.12f, 0.4f, 0.35f)); putDelay (dl ("1/4", 0.3f, 0.12f, fx::Delay::pingPong, 4000, 400)); }
        else if (f == "dist") { comp (-20, 3.0f, 10, 120, 1.0f, 0.7f); mid (-2.0f, 3200, 0.9f); putDelay (dl ("1/8D", 0.32f, 0.12f, fx::Delay::pingPong, 3800, 400)); }
        else if (f == "soft") { comp (-24, 2.0f, 15, 150, 1.0f, 0.5f); mid (-1.0f, 2800, 1.0f); putDelay (dl ("1/4D", 0.34f, 0.14f, fx::Delay::tape, 3800, 400)); }
        else if (f == "chip") { putDelay (dl ("1/16", 0.35f, 0.12f, fx::Delay::pingPong, 5000, 400)); c.eqAdd ("high", -1.0f); }
        else if (f == "digital") { comp (-22, 2.5f, 10, 120, 1.0f, 0.6f); mid (-1.5f, 3200, 1.0f); putDelay (dl ("1/4", 0.32f, 0.13f, fx::Delay::pingPong, 4000, 400)); }
        else
        {   // analog leads: tube warmth with clean lows, the 3 kHz edge out, 1/8D throws, a small plate
            comp (-20, 3.0f, 10, 120, 1.5f, 0.7f);
            if (! c.has (FT::drive)) c.put (fx::drive (fx::Drive::tube, 0.1f, 0.5f, 6500).set ("cleanLow", 1.0f));
            mid (-1.5f, 3000, 0.9f);
            putDelay (dl ("1/8D", 0.32f, 0.13f, fx::Delay::pingPong, 4000, 400));
        }
        if (space && (r.space == "room" || r.space == "dry")) c.put (rv (R::plate, 0.4f, 0.8f, 0.12f, 20, 0.35f, 300));   // leads: a small plate, not a room
    }
    else if (cat == catPads)
    {
        c.eqAdd ("low", -1.5f);   // the pad leaves the lows to the bass
        if (f == "strings") { chorusIfNone (fx::off (fx::ensemble (0.3f, 0.4f, 0.4f))); putDelay (fx::off (dl ("1/4D", 0.35f, 0.1f, fx::Delay::tape, 3500, 400))); }
        else if (f == "choir") { chorusIfNone (fx::ensemble (0.2f, 0.35f, 0.4f)); putDelay (dl ("1/4D", 0.35f, 0.07f, fx::Delay::tape, 3500, 400)); }
        else if (f == "organ") { putDelay (fx::off (dl ("1/4D", 0.35f, 0.1f, fx::Delay::tape, 3500, 400))); }
        else if (f == "winds") { comp (-26, 1.8f, 30, 250, 0.5f, 0.4f); chorusIfNone (fx::ensemble (0.18f, 0.35f, 0.4f)); putDelay (dl ("1/4D", 0.35f, 0.07f, fx::Delay::tape, 3500, 400)); }
        else if (f == "shine")
        {
            chorusIfNone (fx::ensemble (0.25f, 0.4f, 0.45f));
            putDelay (dl ("1/4D", 0.4f, 0.1f, fx::Delay::pingPong, 3500, 400));
            if (space && r.space != "cloud" && r.space != "vast") c.put (rv (R::shimmer, 0.7f, 1.2f, 0.22f, 30, 0.45f, 300).set ("shimmer", 0.18f));
        }
        else if (f == "machine") { chorusIfNone (fx::ensemble (0.45f, 0.5f, 0.5f)); c.put (fx::phaser (0.22f, 0.12f, 0.8f, 0.35f)); putDelay (dl ("1/4D", 0.3f, 0.06f, fx::Delay::tape, 3500, 400)); }
        else if (f == "air") { c.put (fx::phaser (0.2f, 0.08f, 0.9f, 0.3f)); putDelay (dl ("1/4D", 0.45f, 0.1f, fx::Delay::tape, 3000, 400)); }
        else if (f == "vocal") { chorusIfNone (fx::ensemble (0.3f, 0.35f, 0.45f)); putDelay (dl ("1/4D", 0.35f, 0.08f, fx::Delay::tape, 3500, 400)); }
        else { chorusIfNone (fx::ensemble (0.3f, 0.4f, 0.5f)); putDelay (dl ("1/4D", 0.38f, 0.09f, fx::Delay::tape, 3500, 400)); }   // warm analog pads
        widen (r.sample || r.model ? 0.62f : 0.72f, 180);
        finish = std::max (finish, 0.08f);
    }
    else if (cat == catSynth)
    {
        if (f == "stab") { comp (-18, 3.0f, 5, 100, 2.0f, 0.75f); if (! c.has (FT::transient)) c.put (fx::transient (0.12f)); putDelay (dl ("1/8D", 0.3f, 0.12f, fx::Delay::pingPong, 4000, 400)); widen (0.62f, 150); }
        else if (f == "brass") { comp (-20, 2.5f, 15, 150, 1.0f, 0.6f); chorusIfNone (fx::chorus (0.22f, 0.4f, 0.45f)); mid (-1.5f, 2800, 0.9f); putDelay (fx::off (dl ("1/8D", 0.3f, 0.1f))); }
        else if (f == "keys") { comp (-24, 2.0f, 15, 150, 1.0f, 0.5f); chorusIfNone (fx::chorus (0.2f, 0.45f, 0.4f)); putDelay (dl ("1/8D", 0.26f, 0.08f, fx::Delay::pingPong, 4000, 400)); }
        else if (f == "organ")
        {
            if (! c.has (FT::drive)) c.put (fx::drive (fx::Drive::tube, 0.1f, 0.6f, 6000).set ("cleanLow", 1.0f));
            chorusIfNone (fx::chorus (0.3f, 0.8f, 0.3f));   // a slow rotor-like sweep
            putDelay (fx::off (dl ("1/8D", 0.25f, 0.08f)));
        }
        else if (f == "strings") { chorusIfNone (fx::ensemble (0.45f, 0.5f, 0.5f)); c.put (fx::phaser (0.2f, 0.15f, 0.8f, 0.35f)); putDelay (dl ("1/4D", 0.3f, 0.07f, fx::Delay::tape, 3500, 400)); widen (0.66f, 160); }
        else if (f == "rhythm") { comp (-20, 2.5f, 8, 120, 1.0f, 0.6f); putDelay (dl ("1/8D", 0.25f, 0.08f, fx::Delay::pingPong, 4000, 400)); widen (0.62f, 150); }
        else
        {   // poly synths: the classic two-delay-line chorus, glue, 1/8D, a plate
            comp (-22, 2.0f, 12, 140, 1.0f, 0.55f);
            chorusIfNone (fx::chorus (0.32f, 0.5f, 0.45f));
            mid (-1.5f, 3000, 0.9f);
            putDelay (dl ("1/8D", 0.28f, 0.1f, fx::Delay::pingPong, 4000, 400));
            widen (0.62f, 150);
        }
    }
    else   // FX & Atmos
    {
        if (f == "riser") { putDelay (dl ("1/4", 0.42f, 0.14f, fx::Delay::pingPong, 4000, 300)); c.put (fx::phaser (0.2f, 0.18f, 0.9f, 0.4f)); widen (0.78f, 150); }
        else if (f == "drone") { c.put (fx::phaser (0.25f, 0.06f, 1.0f, 0.4f)); putDelay (dl ("1/2", 0.45f, 0.08f, fx::Delay::tape, 3000, 300)); widen (0.75f, 150); }
        else if (f == "hit") { comp (-16, 2.5f, 20, 200, 1.5f, 0.7f); c.eqAdd ("low", 1.5f); putDelay (fx::off (dl ("1/4", 0.35f, 0.12f))); widen (0.7f, 150); }
        else if (f == "reversed") { putDelay (dl ("1/4D", 0.38f, 0.12f, fx::Delay::tape, 3500, 300)); widen (0.7f, 150); }
        else if (f == "swell") { putDelay (dl ("1/4D", 0.35f, 0.08f, fx::Delay::tape, 3500, 300)); widen (0.7f, 150); }
        else { c.put (fx::flanger (0.18f, 0.08f, 0.7f, 0.4f)); putDelay (dl ("1/4D", 0.4f, 0.12f, fx::Delay::tape, 3500, 300)); widen (0.75f, 150); }
        if (space && (r.space == "room" || r.space == "dry")) c.put (rv (R::hall, 0.7f, 1.3f, 0.26f, 25, 0.4f, 160));
    }

    //---------------------------------------------------------------- the treatment (role) on top
    bool wobbleOn = false, swirl = false;
    if (t == "lofi")
    {
        c.put (fx::drive (fx::Drive::tape, 0.18f, 0.7f, 4800).set ("cleanLow", 1.0f));
        c.at (FT::eq).set ("highFreq", 4500.0f);
        if (space) c.put (rv (R::room, 0.4f, 0.9f, 0.14f, 6, 0.5f, 220));
        if (space) c.drop (FT::delay);
        wobbleOn = true;
    }
    else if (t == "underwater") { swirl = true; c.drop (FT::transient); }
    else if (t == "wide") { if (! c.has (FT::chorus)) c.put (fx::ensemble (0.4f, 0.45f, 0.5f)); widen (0.8f, 160); }
    else if (t == "muted" || t == "dark") { c.at (FT::eq).set ("highFreq", 4000.0f); if (space) c.at (FT::reverb).set ("damping", 0.6f); }
    else if (t == "stab" || t == "short") { if (space) c.at (FT::reverb).set ("decay", std::min (0.8f, c.get (FT::reverb, "decay"))); }
    else if (t == "long" || t == "swell") { if (space) c.at (FT::reverb).set ("decay", c.get (FT::reverb, "decay") * 1.2f); }
    else if (t == "airy") { if (space) c.at (FT::reverb).set ("predelay", 45.0f); }
    else if (t == "drive" && c.has (FT::drive)) c.at (FT::drive).set ("cleanLow", 1.0f).set ("tone", std::min (6000.0f, c.get (FT::drive, "tone")));

    // variant 2: the v2 colours only (the converted chain), the movement and macros below still apply; variant 3: the v2
    // sound exactly (converted chain and finish, no movement) - only the macros, the routings and a pattern are added
    if (variant >= 2) { c = Chain (converted); finish = p.finish; }
    if (r.pattern >= 5 && c.has (FT::reverb)) c.at (FT::reverb).set ("mix", c.get (FT::reverb, "mix") * 0.6f);   // gated pads: the tail must not fill the gaps

    //---------------------------------------------------------------- movement (LFO 1 = the playing gesture, LFO 2 = colour)
    const bool synthVib = synthMain && p.parts.front().get ("lfoPitch", 0.0f) > 0.03f;
    if (variant == 3) {}
    else if (cat == catLeads)
    {
        // delayed, faded-in vibrato (leads with a built-in one start at 0: the Vibrato macro / mod wheel add it)
        p.mod.lfo[0] = vibrato (synthVib || r.model ? 0.0f : 0.3f);
        if (f == "analog" || f == "digital" || f == "dist") p.mod.lfo[1] = drift (0.3f, 0.11f, 0.2f, 0.0f);
    }
    else if (cat == catPads)
    {
        if (r.sample || r.model) p.mod.lfo[0] = sampleDrift (0.5f, f == "strings" ? 0.09f : 0.12f);
        else p.mod.lfo[0] = drift (0.55f, 0.1f, 0.35f, 0.22f);
        p.mod.lfo[1] = lfo (VaultLfo::triangle).rate (0.07f).free().phase (0.25f).depth (0.4f).to ("pan", 0.25f).to ("volume", 0.06f);
    }
    else if (cat == catPlucks)
    {
        if (! r.sample && ! r.model && f != "chip") p.mod.lfo[0] = drift (0.35f, 0.17f, 0.25f, 0.18f);
        else p.mod.lfo[0] = lfo (VaultLfo::sine).rate (0.15f).free().depth (0.3f).to ("pan", 0.2f);
    }
    else if (cat == catSynth)
    {
        if (f == "keys") p.mod.lfo[0] = tremolo (0.0f);
        else if (f != "rhythm" && f != "stab") p.mod.lfo[0] = drift (0.25f, 0.13f, 0.25f, 0.12f);
    }
    else
    {
        if (f == "drone" || f == "texture" || f == "swell") p.mod.lfo[0] = drift (0.6f, 0.07f, 0.4f, 0.3f);
        else if (f == "riser" || f == "reversed") p.mod.lfo[0] = lfo (VaultLfo::sine).rate (0.2f).free().depth (0.4f).to ("pan", 0.35f);
        else p.mod.lfo[0] = lfo (VaultLfo::sine).rate (0.1f).free().depth (0.0f).to ("cutoff", 0.5f).to ("pan", 0.3f);   // impacts: still, the Motion macro adds it
    }
    if (variant == 3) {}
    else if (wobbleOn) p.mod.lfo[1] = wobble (0.55f);
    else if (swirl) p.mod.lfo[1] = lfo (VaultLfo::sine).rate (0.3f).free().depth (0.5f).to ("cutoff", 0.4f).to ("pan", 0.2f);

    // the rhythm presets (patterns start from Init: only mode / rate / octaves / gate / accents change)
    if (r.pattern == 1) p.arp = arp (VaultArp::up, "1/16", 2, 0.6f, 0.0f, true);
    else if (r.pattern == 2) p.arp = arp (VaultArp::upDown, "1/16", 1, 0.7f, 0.0f, true);
    else if (r.pattern == 3) p.arp = arp (VaultArp::down, "1/8", 1, 0.5f, 0.0f, true);
    else if (r.pattern == 4) { p.arp = arp (VaultArp::chord, "1/8", 1, 0.45f, 0.0f, true); const float acc[16] { 1, 0.7f, 0.85f, 0.7f, 1, 0.7f, 0.85f, 0.75f, 1, 0.7f, 0.85f, 0.7f, 1, 0.7f, 0.9f, 0.75f }; for (int i = 0; i < 16; ++i) p.arp.vel[(size_t) i] = acc[i]; }
    else if (r.pattern == 5) p.gate = gate ("1/16", { 1, 0, 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0 }, 1.0f, 0.2f, 1.0f, true);
    else if (r.pattern == 6) p.gate = gate ("1/16", { 1, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 1 }, 1.0f, 0.15f, 1.0f, true);

    // performance: mod wheel and velocity
    if (cat == catLeads) p.mod.modWheel.push_back ({ "lfo1.depth", 0.8f });
    else if (cat == catPads || (cat == catFxAtmos && ! p.mod.lfo[0].dests.empty())) p.mod.modWheel.push_back ({ "lfo1.depth", 0.5f });
    else if (cat == catSynth && f == "keys") p.mod.modWheel.push_back ({ "lfo1.depth", 0.7f });
    else p.mod.modWheel.push_back ({ "cutoff", 1.2f });
    if (c.has (FT::compressor) || c.has (FT::drive)) p.mod.velocity.push_back ({ "volume", cat == catPads ? 0.4f : 0.7f });

    p.fx = c.build();
    p.finish = finish;
    for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; }
}

//------------------------------------------------------------------------------ 2. comfort on the finished sound
struct Comfort { float h1 = 0, s25 = 0, s6 = 0, s8 = 0, centroid = 0; };
Comfort comfortOf (const VaultPreset& p, float vel)
{
    Comfort m;
    const auto b = recipegate::render (p, vault::homeNote, vel, 1.0, 0.8);
    const auto sp = recipegate::spectrum (b, 0.05);
    const double tot = std::max (1e-20, recipegate::bandPower (sp, 20, 20000));
    m.s25 = (float) (recipegate::bandPower (sp, 2000, 5000) / tot);
    m.s6 = (float) (recipegate::bandPower (sp, 6000, 20000) / tot);
    m.s8 = (float) (recipegate::bandPower (sp, 8000, 20000) / tot);
    double num = 0;
    for (int i = 1; i < (int) sp.pow.size(); ++i) num += i * sp.binHz * sp.pow[(size_t) i];
    m.centroid = (float) (num / tot);
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    const float h1 = recipegate::peakDb (sp, f0 * 0.97, f0 * 1.03);
    float strongest = h1;
    for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, recipegate::peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
    m.h1 = h1 - strongest;
    return m;
}
VaultFxSlot* slotOf (VaultPreset& p, FT t) { for (auto& s : p.fx) if (s.type == t) return &s; return nullptr; }
float partHz (const VaultPart& p) { return 440.0f * std::pow (2.0f, (float) (vault::homeNote + 12 * p.octave + p.semi - 69) / 12.0f); }
bool lowerCut (VaultPreset& p, float factor)
{
    bool moved = false;
    for (auto& part : p.parts)
    {
        const float floor = std::max (700.0f, 2.5f * partHz (part));
        if (part.kind == VaultPartKind::sample || part.kind == VaultPartKind::synth)
        {
            const char* key = part.kind == VaultPartKind::sample ? "s.cutoff" : "cutoff";
            const float cur = part.get (key, part.kind == VaultPartKind::sample ? 20000.0f : 12000.0f);
            const float next = std::max (floor, cur > 7000.0f ? 7000.0f : cur * factor);
            if (next < cur - 1.0f) { part.set (key, next); moved = true; }
        }
        else
        {
            const float cur = part.get ("m.tone", 0.5f);
            if (cur > 0.12f) { part.set ("m.tone", std::max (0.1f, cur * 0.8f)); moved = true; }
        }
    }
    return moved;
}
// the FX moved the balance: top shelf / its corner, the 2-5 kHz band, the part filters back under this file's caps; the
// fundamental kept on top with a low shelf over it (not FX & Atmos: inharmonic by nature)
int fitComfort (VaultPreset& p)
{
    const float ceil = recipegate::centroidCeiling (p.category);
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    int moves = 0;
    for (int it = 0; it < 10; ++it)
    {
        const auto a = comfortOf (p, 0.8f), b = comfortOf (p, 1.0f);
        bool ok = true;
        if (std::max (a.s6, b.s6) > cap6k || std::max (a.s8, b.s8) > cap8k || a.centroid > 0.9f * ceil)
        {
            ok = false;
            if (p.fx.size() >= (size_t) vault::maxFxSlots && slotOf (p, FT::eq) == nullptr) { if (! lowerCut (p, 0.8f)) break; }
            else
            {
                if (slotOf (p, FT::eq) == nullptr) recipe::design (p).addFx (VaultFxSlot::make (FT::eq));
                auto* e = slotOf (p, FT::eq);
                if (e->get ("high") > -12.0f) e->set ("high", std::max (-12.0f, e->get ("high") - 3.0f));
                else if (e->get ("highFreq") > 3500.0f) e->set ("highFreq", std::max (3500.0f, e->get ("highFreq") * 0.8f));
                else if (! lowerCut (p, 0.8f)) break;
            }
        }
        else if (std::max (a.s25, b.s25) > cap25)
        {
            ok = false;
            auto* e = slotOf (p, FT::eq);
            if (e != nullptr && e->get ("midFreq") >= 1800.0f && e->get ("midFreq") <= 4500.0f && e->get ("mid") > -8.0f) e->set ("mid", e->get ("mid") - 2.0f);
            else if (! lowerCut (p, 0.8f)) break;
        }
        else if (p.category != catFxAtmos && a.h1 < wantH1 - 0.5f)
        {
            ok = false;
            auto* e = slotOf (p, FT::eq);
            if (e == nullptr) break;
            const float lf = juce::jlimit (30.0f, 1000.0f, std::max (e->get ("lowFreq"), 1.4f * f0));
            if (lf > e->get ("lowFreq") + 1.0f) e->set ("lowFreq", lf);
            else if (e->get ("low") < 10.0f) e->set ("low", std::min (10.0f, e->get ("low") + 2.0f));
            else break;
        }
        if (ok) break;
        ++moves;
    }
    return moves;
}

//------------------------------------------------------------------------------ 3. the four sound macros
const char* layerMacroName (const juce::String& l)
{
    static const std::map<juce::String, const char*> n {
        { "pizzhyb", "Pizz" }, { "harphyb", "Harp" }, { "marhyb", "Marimba" }, { "kalhyb", "Tines" }, { "vibhyb", "Vibes" }, { "guitarhyb", "Guitar" },
        { "subpluck", "Sub" }, { "padpluck", "Pad" }, { "airpluck", "Air" }, { "sparkle", "Sparkle" }, { "ksdouble", "Double" }, { "dulchyb", "Dulcimer" },
        { "ksmallet", "Bell" }, { "flutelead", "Flute" }, { "harmlead", "Harmonica" }, { "saxlead", "Sax" }, { "violinlead", "Violin" }, { "mutelead", "Trumpet" },
        { "voicelead", "Voice" }, { "octdown", "Octave" }, { "octupl", "Shine" }, { "whistlel", "Whistle" }, { "sqlead2", "Square" }, { "strpad", "Strings" },
        { "cellopadl", "Cellos" }, { "choirpadl", "Choir" }, { "organpadl", "Organ" }, { "glasspadl", "Glass" }, { "bowedvib", "Bowed Bars" }, { "sinesupport", "Warmth" },
        { "subpad", "Sub" }, { "vocpadl", "Voices" }, { "fluteair", "Flute" }, { "femalepad", "Voice" }, { "hornpadl", "Horns" }, { "cinepad", "Orchestra" },
        { "heaven", "Organ" }, { "pianohyb", "Piano" }, { "hornstab", "Horns" }, { "spicstab", "Strings" }, { "orchhitl", "Orchestra" }, { "harpsihyb", "Harpsichord" },
        { "organhyb", "Organ" }, { "eplayer", "Electric Piano" }, { "padsy", "Pad" }, { "subsy", "Sub" }, { "pluckon", "Pluck" }, { "riserstack", "Noise" },
        { "impactsub", "Sub" }, { "timpimpact", "Timpani" }, { "dronechoir", "Choir" }, { "dronestr", "Cellos" }, { "revpianol", "Piano" }, { "windl", "Wind" },
        { "glassl", "Glass" } };
    auto it = n.find (l);
    return it != n.end() ? it->second : "Layer";
}
// a layer macro: the added parts' level 0 .. 2 x designed (several added parts move together)
M layerMacro (const VaultPreset& p, const char* name)
{
    M m (name, 0.5f);
    for (size_t k = 1; k < p.parts.size() && k < 4; ++k)
    {
        if (p.parts[k].label == "Sine") continue;
        const auto t = "layer" + juce::String ((int) k + 1) + ".level";
        const float l = p.parts[k].level;
        m.abs (t.toRawUTF8(), l, 2.0f * l, 0.0f, 2.0f);
    }
    return m;
}
juce::String spaceName (const Chain& c, const juce::String& treat)
{
    const int type = (int) c.get (FT::reverb, "type");
    if (treat == "airy") return "Air";
    return type == (int) R::cloud ? "Haze" : type == (int) R::hall ? "Hall" : type == (int) R::room ? "Room" : type == (int) R::shimmer ? "Shimmer" : "Plate";
}
M spaceMacro (const Chain& c, const char* name, float d)
{
    M m (name, d);
    const bool room = (int) c.get (FT::reverb, "type") == (int) R::room;
    m.fxp (c, FT::reverb, "mix", room ? 0.6f : 0.55f).fxp (c, FT::reverb, "decay", room ? 1.8f : 1.5f).fxp (c, FT::reverb, "size", 0.5f).fxp (c, FT::reverb, "predelay", 40.0f);
    if ((int) c.get (FT::reverb, "type") == (int) R::shimmer) m.fxp (c, FT::reverb, "shimmer", 0.4f);   // the octave feedback is the shimmer
    return m;
}
M echoMacro (const Chain& c)
{
    const float mix = c.get (FT::delay, "mix");
    const bool on = c.has (FT::delay) && c.slots.at (Chain::rank (FT::delay)).on;
    M m ("Echo", ! on || mix < 0.001f ? 0.0f : 0.3f);
    m.fxp (c, FT::delay, "mix", 0.5f).fxp (c, FT::delay, "feedback", 0.45f);
    return m;
}
void designMacros (VaultPreset& p, const Role& r)
{
    const auto& f = r.fam;
    const auto& t = r.treat;
    const int cat = r.cat;
    const bool samp = p.parts.front().kind != VaultPartKind::synth;
    {   // the silent slots some macros open (only while the chain has room); switched-off slots go on, silent
        Chain c (p.fx);
        auto room = [&] { return c.slots.size() < (size_t) vault::maxFxSlots; };
        if (cat != catPads) for (auto& [k, s] : c.slots) if (! s.on && s.type == FT::delay) { s.on = true; s.set ("mix", 0.0f); }   // (the Echo knob needs it running)
        if (! c.has (FT::delay) && room() && cat != catPads) c.put (dl ("1/8D", 0.3f, 0.0f, fx::Delay::pingPong, 4000, 400));
        if (cat == catLeads && ! c.has (FT::drive) && room()) c.put (fx::drive (fx::Drive::tube, 0.0f, 0.6f, 6000).set ("cleanLow", 1.0f));
        if (t == "lofi" && ! c.has (FT::lofi) && room()) c.put (fx::lofi (0.0f, 0.0f, 1.0f));
        if (cat == catFxAtmos && (f == "riser" || f == "drone") && ! c.has (FT::drive) && room()) c.put (fx::drive (fx::Drive::tape, 0.0f, 0.6f, 5000).set ("cleanLow", 1.0f));
        if (cat == catFxAtmos && f == "texture" && ! c.has (FT::lofi) && room()) c.put (fx::lofi (0.0f, 0.0f, 1.0f));
        if (cat == catPlucks && ! samp && ! c.has (FT::transient) && room()) c.put (fx::transient (0.0f));
        p.fx = c.build();
    }
    Chain c (p.fx);
    for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; }
    bool layered = false;
    if (r.layer.isNotEmpty()) for (size_t k = 1; k < p.parts.size() && k < 4; ++k) layered |= p.parts[k].label != "Sine";

    // 1. tone: the part filters + EQ tilt
    {
        const char* nm = cat == catPads ? "Brightness" : cat == catFxAtmos ? "Tone" : samp ? "Brightness" : "Cutoff";
        M m (nm, 0.55f);
        m.rel ("cutoff", samp ? 3.5f : 2.5f).fxp (c, FT::eq, "tilt", 8.0f);
        // sine-like voices have little for a filter to take: the knob also adds harmonics (drive) where the chain has one
        if ((r.kind & (kSine | kFM)) != 0 && c.has (FT::drive)) m.fxp (c, FT::drive, "amount", 0.3f);
        setMacro (p, 0, m);
    }
    // 2. the second knob: space (leads: drive, layered leads: the layer)
    if (cat == catLeads)
    {
        if (layered) setMacro (p, 1, layerMacro (p, layerMacroName (r.layer)));
        else if (! c.has (FT::drive)) setMacro (p, 1, spaceMacro (c, spaceName (c, t).toRawUTF8(), 0.4f));
        else setMacro (p, 1, M (f == "dist" ? "Fuzz" : "Drive", f == "dist" ? 0.5f : 0.25f).fxp (c, FT::drive, "amount", 0.45f)
                                  .toMax ("fx.drive.output", c.get (FT::drive, "output"), -4.0f));
    }
    else setMacro (p, 1, spaceMacro (c, spaceName (c, t).toRawUTF8(), 0.4f));
    // 3. the third knob
    if (cat == catPads || cat == catFxAtmos)
    {
        const float d0 = p.mod.lfo[0].depth;
        M m ("Motion", juce::jlimit (0.0f, 1.0f, d0));
        m.abs ("lfo1.depth", d0, 1.0f, 0.0f, 1.0f);
        if (c.has (FT::phaser)) m.fxp (c, FT::phaser, "mix", 0.45f);
        setMacro (p, 2, m);
    }
    else if (cat == catSynth && (f == "poly" || f == "brass" || f == "strings" || f == "organ") && c.has (FT::chorus))
    {
        const char* nm = f == "strings" ? "Ensemble" : f == "organ" ? "Rotor" : "Chorus";
        M m (nm, c.get (FT::chorus, "mix") < 0.001f ? 0.0f : 0.35f);
        m.fxp (c, FT::chorus, "mix", 0.6f).fxp (c, FT::chorus, "depth", 0.5f);
        if (f == "organ") m.fxp (c, FT::chorus, "rate", 4.0f);
        else m.rel ("quick.width", 0.3f);
        setMacro (p, 2, m);
    }
    else setMacro (p, 2, echoMacro (c));
    // 4. the role's character, else the family's
    {
        bool done = true;
        if (cat == catLeads) setMacro (p, 3, M ("Vibrato", p.mod.lfo[0].depth).abs ("lfo1.depth", p.mod.lfo[0].depth, 1.0f, 0.0f, 1.0f));
        else if (layered) setMacro (p, 3, layerMacro (p, layerMacroName (r.layer)));
        else if (t == "lofi") setMacro (p, 3, M ("Tape", 0.5f).abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1).fxp (c, FT::lofi, "wow", 0.5f).fxp (c, FT::drive, "amount", 0.3f)
                                               .toMax ("fx.drive.output", c.get (FT::drive, "output"), -2.5f));
        else if (t == "underwater") setMacro (p, 3, M ("Depth", 0.5f).abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1).rel ("cutoff", -1.5f));
        else if (t == "reverse" && c.has (FT::reverse)) setMacro (p, 3, M ("Reverse", 0.5f).fxp (c, FT::reverse, "mix", 0.8f));
        else if ((t == "wide") && c.has (FT::chorus)) setMacro (p, 3, M ("Width", 0.5f).fxp (c, FT::chorus, "mix", 0.7f).rel ("quick.width", 0.5f));
        else if (t == "drive" && c.has (FT::drive)) setMacro (p, 3, M ("Grit", 0.4f).fxp (c, FT::drive, "amount", 0.45f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -5.0f));
        else if ((t == "echo" || echoSpace (r.space)) && c.has (FT::delay) && cat != catPads) setMacro (p, 3, M ("Repeats", 0.5f).fxp (c, FT::delay, "feedback", 0.4f).fxp (c, FT::delay, "hiCut", 3000.0f));
        else if (t == "muted" || t == "dark") setMacro (p, 3, M ("Shade", 0.5f).rel ("cutoff", -2.0f).fxp (c, FT::eq, "high", 6.0f));
        else if (r.pattern >= 5) setMacro (p, 3, M ("Gate", 1.0f).abs ("gate.depth", p.gate.depth, 1.0f, 0.0f, 1.0f).abs ("gate.smooth", p.gate.smooth, 0.4f, 0.0f, 1.0f));
        else if (r.pattern >= 1) setMacro (p, 3, M ("Note Length", 0.5f).abs ("arp.gate", p.arp.gate, 0.5f, 0.1f, 1.0f));
        else done = false;
        if (! done)
        {
            if (cat == catPlucks)
            {
                M m ("Snap", 0.3f);
                m.fxp (c, FT::transient, "attack", 0.6f);
                if (! samp) m.rel ("quick.envAmt", 0.35f);
                setMacro (p, 3, m);
            }
            else if (cat == catPads) setMacro (p, 3, M ("Swell", 0.5f).rel ("quick.attack", 0.4f).rel ("quick.release", 0.25f));
            else if (cat == catSynth)
            {
                if (f == "stab" && c.has (FT::transient)) setMacro (p, 3, M ("Punch", 0.3f).fxp (c, FT::transient, "attack", 0.6f).fxp (c, FT::compressor, "ratio", 4.0f));
                else if (f == "keys" && lfoTo (p.mod.lfo[0], "volume")) setMacro (p, 3, M ("Tremolo", 0.0f).abs ("lfo1.depth", 0.0f, 1.0f, 0, 1));
                else if (f == "brass") setMacro (p, 3, M ("Swell", 0.5f).rel ("quick.attack", 0.4f).rel ("quick.envAmt", 0.3f));
                else if (f == "organ" && c.has (FT::drive)) setMacro (p, 3, M ("Drive", 0.3f).fxp (c, FT::drive, "amount", 0.4f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -4.0f));
                else if (f == "strings" && c.has (FT::phaser)) setMacro (p, 3, M ("Phaser", 0.3f).fxp (c, FT::phaser, "mix", 0.6f).fxp (c, FT::phaser, "rate", 0.6f));
                else if (f == "poly" || f == "brass" || f == "strings" || f == "organ") setMacro (p, 3, echoMacro (c));
                else setMacro (p, 3, M ("Width", 0.5f).rel ("quick.width", 0.6f).rel ("quick.chorus", 0.4f));
            }
            else   // FX & Atmos
            {
                if ((f == "riser" || f == "drone") && c.has (FT::drive)) setMacro (p, 3, M ("Grit", 0.2f).fxp (c, FT::drive, "amount", 0.45f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -4.0f));
                else if (f == "texture" && c.has (FT::lofi)) setMacro (p, 3, M ("Crush", 0.2f).fxp (c, FT::lofi, "crush", 0.45f).fxp (c, FT::lofi, "wow", 0.4f));
                else if (f == "hit") setMacro (p, 3, M ("Boom", 0.4f).fxp (c, FT::eq, "low", 6.0f).fxp (c, FT::compressor, "ratio", 4.0f));
                else setMacro (p, 3, echoMacro (c));
            }
        }
    }
}

//------------------------------------------------------------------------------ 4. macro extremes against the comfort rules
struct Probe
{
    juce::AudioBuffer<float> c5;
    float loud = -120, s25 = 0, s6 = 0, s8 = 0, h1 = 0, peak127 = 0, tailEnd = -200, tailMax = -200;
};
Probe probe (const VaultPreset& p, const float* mv, double total)
{
    Probe r;
    const double sr = recipegate::sr;
    juce::MidiBuffer mb;
    mb.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, (juce::uint8) 102), 0);
    mb.addEvent (juce::MidiMessage::noteOff (1, vault::homeNote), (int) (recipegate::holdSeconds * sr));
    r.c5 = VaultEngine::renderMidi (p, mb, (int) (total * sr), sr, mv);
    r.loud = vaultqc::analyse (r.c5, sr, recipegate::holdSeconds).loud;
    const auto sp = recipegate::spectrum (r.c5, 0.05);
    const double tot = std::max (1e-20, recipegate::bandPower (sp, 20, 20000));
    r.s25 = (float) (recipegate::bandPower (sp, 2000, 5000) / tot);
    r.s6 = (float) (recipegate::bandPower (sp, 6000, 20000) / tot);
    r.s8 = (float) (recipegate::bandPower (sp, 8000, 20000) / tot);
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    const float h1 = recipegate::peakDb (sp, f0 * 0.97, f0 * 1.03);
    float strongest = h1;
    for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, recipegate::peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
    r.h1 = h1 - strongest;
    const int fr = (int) (0.01 * sr), nf = r.c5.getNumSamples() / fr;
    for (int fi = 0; fi < nf; ++fi)
    {
        double acc = 0;
        for (int i = fi * fr; i < (fi + 1) * fr; ++i) acc += 0.5 * ((double) r.c5.getSample (0, i) * r.c5.getSample (0, i) + (double) r.c5.getSample (1, i) * r.c5.getSample (1, i));
        const float e = vaultaudit::db (acc / fr + 1e-20);
        r.tailMax = std::max (r.tailMax, e);
        if (fi >= nf - 10) r.tailEnd = std::max (r.tailEnd, e);
    }
    juce::MidiBuffer hb;
    hb.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, (juce::uint8) 127), 0);
    hb.addEvent (juce::MidiMessage::noteOff (1, vault::homeNote), (int) (0.7 * sr));
    const auto hard = VaultEngine::renderMidi (p, hb, (int) (0.8 * sr), sr, mv);
    r.peak127 = std::max (hard.getMagnitude (0, hard.getNumSamples()), r.c5.getMagnitude (0, r.c5.getNumSamples()));
    return r;
}
juce::String judge (const Probe& x, const Probe& ref, float lin, bool longTail, bool inharmonic)
{
    if (x.peak127 * lin >= 0.885f && x.peak127 > ref.peak127 * 1.06f) return "clip";
    if (x.s25 > 0.10f && x.s25 > ref.s25 + 0.005f) return "harsh-2-5k";
    if ((x.s6 > 0.03f && x.s6 > ref.s6 + 0.002f) || (x.s8 > 0.01f && x.s8 > ref.s8 + 0.001f)) return "piercing-highs";
    if (! inharmonic && x.h1 < -3.0f && x.h1 < ref.h1 - 0.5f) return "fundamental";
    if (x.loud > ref.loud + 4.0f) return "louder";
    if (longTail && x.tailEnd > x.tailMax - 55.0f && x.tailEnd > ref.tailEnd + 3.0f) return "tail";
    return {};
}
float extremeDistance (const Probe& a, const Probe& b)
{
    const std::vector<std::pair<double, double>> segs { { 0.0, 0.15 }, { 0.15, 0.8 }, { 0.8, 1.4 }, { 1.5, 2.5 } };
    const auto x = recipegate::melFrames (a.c5, segs), y = recipegate::melFrames (b.c5, segs);
    double s = 0;
    for (size_t i = 0; i < x.size() && i < y.size(); ++i) s += (x[i] - y[i]) * (x[i] - y[i]);
    return (float) std::sqrt (s / std::max<size_t> (1, x.size()));
}
float amDepth (const juce::AudioBuffer<float>& b)
{
    const int fr = (int) (0.01 * recipegate::sr);
    std::vector<float> e;
    for (int f = 20; f < 140 && (f + 1) * fr <= b.getNumSamples(); ++f)
    {
        double acc = 0;
        for (int i = f * fr; i < (f + 1) * fr; ++i) acc += (double) b.getSample (0, i) * b.getSample (0, i) + (double) b.getSample (1, i) * b.getSample (1, i);
        e.push_back (vaultaudit::db (acc / fr + 1.0e-12));
    }
    double am = 0;
    for (int f = 7; f + 7 < (int) e.size(); ++f)
    {
        double m = 0;
        for (int g = f - 7; g <= f + 7; ++g) m += e[(size_t) g];
        am += std::pow (e[(size_t) f] - m / 15.0, 2.0);
    }
    return (float) std::sqrt (am / std::max (1, (int) e.size() - 14));
}
float tailRatioDb (const juce::AudioBuffer<float>& b)
{
    auto energy = [&b] (double t0, double t1)
    {
        double acc = 1.0e-12;
        for (int c = 0; c < 2; ++c)
            for (int i = (int) (t0 * recipegate::sr); i < std::min (b.getNumSamples(), (int) (t1 * recipegate::sr)); ++i) acc += (double) b.getSample (c, i) * b.getSample (c, i);
        return acc;
    };
    return (float) (10.0 * std::log10 (energy (1.6, 2.6) / energy (0.05, 1.4)));
}
float sideShare (const juce::AudioBuffer<float>& b)
{
    double s = 1e-12, m = 1e-12;
    for (int i = 0; i < b.getNumSamples(); ++i) { const double l = b.getSample (0, i), r = b.getSample (1, i); m += (l + r) * (l + r); s += (l - r) * (l - r); }
    return (float) (10.0 * std::log10 (s / m));
}
struct MacroReport { int fixes = 0; juce::StringArray harsh, quiet; };
MacroReport checkMacros (VaultPreset& p, float lin)
{
    MacroReport mr;
    bool longTail = false;
    for (auto& m : p.macros) for (auto& t : m.targets) if (t.target.contains ("reverb") || t.target.contains ("delay") || t.target.contains ("release")) longTail = true;
    const double total = longTail ? 8.0 : 3.5;
    const bool inharmonic = p.category == catFxAtmos;
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
            const auto badLo = judge (lo, ref, lin, longTail, inharmonic), badHi = judge (hi, ref, lin, longTail, inharmonic);
            if (badLo.isEmpty() && badHi.isEmpty()) break;
            if (round == 4) { mr.harsh.add (p.name + ": " + mac.name + " (" + (badLo.isNotEmpty() ? badLo : badHi) + ")"); break; }
            const float k = round == 3 ? 0.0f : 0.55f;   // shorten the failing side; the designed point stays
            const float d = p.macroDefaults[i];
            const float kl = badLo.isNotEmpty() ? k : 1.0f, kh = badHi.isNotEmpty() ? k : 1.0f;
            for (auto& t : mac.targets)
            {
                const float v = t.min + (t.max - t.min) * d;
                t.min = v + (t.min - v) * kl;
                t.max = v + (t.max - v) * kh;
            }
            const float den = d * kl + (1 - d) * kh;
            if (den > 1.0e-6f) p.macroDefaults[i] = d * kl / den;
            ++mr.fixes;
        }
        const bool pitchLfo = [&]
        {
            for (auto& t : mac.targets)
                if (t.target.startsWith ("lfo") && t.target.endsWith ("depth"))
                    for (auto& dd : p.mod.lfo[juce::jlimit (0, 1, t.target.substring (3, 4).getIntValue() - 1)].dests)
                        if ((dd.target == "pitch" || dd.target == "pan" || dd.target == "cutoff") && std::abs (t.max - t.min) * std::abs (dd.amount) >= 0.04f) return true;
            return false;
        }();
        if (extremeDistance (lo, hi) < 1.0f && std::abs (lo.loud - hi.loud) < 1.0f && std::abs (amDepth (lo.c5) - amDepth (hi.c5)) < 0.4f && ! pitchLfo
            && std::abs (tailRatioDb (lo.c5) - tailRatioDb (hi.c5)) < 2.0f && std::abs (sideShare (lo.c5) - sideShare (hi.c5)) < 1.5f)
            mr.quiet.add (p.name + ": " + mac.name);
    }
    return mr;
}
} // namespace v3

//==============================================================================
// THE PINNED v2 SELECTION: the presets of the shipped bank (id | recipe tag | name), found with SYNTH_PINMAP. The v2 search
// sits on its distinctness thresholds, so a rerun can swap a few presets and renumber the ids after them; ids are saved in
// host projects, so the bank rebuilds exactly these (same candidates, calibration and repairs). SYNTH_V2_SEARCH=1 runs
// the search instead (new material).
const char* const pinnedV2[] = {
    "cv.fx.0001|riser/natural/hall|Riser",
    "cv.fx.0002|noiserise/natural/hall|Noise Sweep",
    "cv.fx.0003|fall/natural/hall|Downlifter",
    "cv.fx.0004|drone/natural/hall|Drone",
    "cv.fx.0005|organdrone/natural/hall|Ritual Drone",
    "cv.fx.0006|texture/natural/hall|Grain Cloud",
    "cv.fx.0007|scifi/natural/hall|Data Stream",
    "cv.fx.0008|wind/natural/hall|Desert Wind",
    "cv.fx.0009|zap/natural/hall|Laser",
    "cv.fx.0010|impact/natural/hall|Impact",
    "cv.fx.0011|braam/natural/hall|Braam",
    "cv.fx.0012|metal/natural/hall|Gong Swell",
    "cv.fx.0013|heart/natural/hall|Heartbeat",
    "cv.fx.0015|revharp/natural/hall|Reversed Harp",
    "cv.fx.0016|revpizz/natural/hall|Reversed Pizz",
    "cv.fx.0017|revmarimba/natural/hall|Reversed Marimba",
    "cv.fx.0018|revchoir/natural/hall|Reversed Choir",
    "cv.fx.0019|revbell/natural/hall|Reversed Bell",
    "cv.fx.0020|revkalimba/natural/hall|Reversed Thumb Piano",
    "cv.fx.0022|tension/natural/hall|Tension Rise",
    "cv.fx.0024|whisper/natural/hall|Whisper Air",
    "cv.fx.0026|revharpsi/natural/hall|Reversed Harpsichord",
    "cv.fx.0028|revglock/natural/hall|Reversed Chime Bars",
    "cv.fx.0029|revdulc/natural/hall|Reversed Dulcimer",
    "cv.fx.0030|pedaldrone/natural/hall|Pedal Drone",
    "cv.fx.0031|didgdrone/natural/hall|Earth Drone",
    "cv.fx.0033|storm/natural/hall|Distant Thunder",
    "cv.fx.0034|sqriser/natural/hall|Square Riser",
    "cv.fx.0035|fmriser/natural/hall|Tension Riser",
    "cv.fx.0036|syncrise/natural/hall|Sync Rise",
    "cv.fx.0038|spacehit/natural/hall|Space Hit",
    "cv.fx.0039|glitch/natural/hall|Glitch Field",
    "cv.fx.0040|rumble/natural/hall|Low Rumble",
    "cv.fx.0041|riser+riserstack|Tidal Riser",
    "cv.fx.0042|zap+impactsub|Seismic Laser",
    "cv.fx.0043|riser+timpimpact|Thunder Riser",
    "cv.fx.0044|noiserise+dronechoir|Spectral Noise Sweep",
    "cv.fx.0045|noiserise+dronestr|Ominous Noise Sweep",
    "cv.fx.0046|noiserise+revpianol|Echoes Noise Sweep",
    "cv.fx.0047|drone+windl|Stormy Drone",
    "cv.fx.0048|riser+glassl|Starlit Riser",
    "cv.fx.0049|zap/natural/echo|Echo Laser",
    "cv.fx.0050|spacehit/natural/echo|Echo Space Hit",
    "cv.fx.0051|noiserise+timpimpact|War Drum Noise Sweep",
    "cv.fx.0052|drone+dronechoir|Ghostly Drone",
    "cv.fx.0053|drone+dronestr|Brooding Drone",
    "cv.fx.0054|texture+revpianol|Afterimage Grain Cloud",
    "cv.fx.0055|fall+riserstack|Stacked Downlifter",
    "cv.fx.0056|braam+impactsub|Boom Braam",
    "cv.fx.0057|fall+timpimpact|Timpani Downlifter",
    "cv.fx.0058|organdrone+dronechoir|Haunted Ritual Drone",
    "cv.fx.0059|zap+windl|Windswept Laser",
    "cv.fx.0060|heart+impactsub|Seismic Heartbeat",
    "cv.fx.0061|drone+timpimpact|Thunder Drone",
    "cv.fx.0062|wind+dronechoir|Spectral Desert Wind",
    "cv.fx.0063|wind+dronestr|Ominous Desert Wind",
    "cv.fx.0064|impact+windl|Stormy Impact",
    "cv.fx.0065|fmriser+riserstack|Surging Tension Riser",
    "cv.fx.0066|revpiano+impactsub|Quake Reversed Piano",
    "cv.fx.0067|revchoir+dronechoir|Ghostly Reversed Choir",
    "cv.fx.0068|zap+dronestr|Brooding Laser",
    "cv.fx.0069|revharp+revpianol|Afterimage Harp",
    "cv.fx.0070|metal+windl|Gale Gong Swell",
    "cv.fx.0071|scifi/lofi/hall|Dusty Data Stream",
    "cv.fx.0072|metal/lofi/hall|Crumbled Gong Swell",
    "cv.fx.0073|wail/lofi/hall|Bedroom Wail",
    "cv.fx.0074|heart/lofi/hall|Lo-Fi Heartbeat",
    "cv.fx.0075|revpiano/lofi/hall|Dusty Reversed Piano",
    "cv.fx.0076|revbell/lofi/hall|Bedroom Reversed Bell",
    "cv.fx.0077|whisper/lofi/hall|Lo-Fi Whisper Air",
    "cv.fx.0078|revharpsi/lofi/hall|Hall Backward Quills",
    "cv.fx.0079|sqriser/lofi/hall|Bedroom Square Riser",
    "cv.fx.0080|fmriser/lofi/hall|Lo-Fi Tension Riser",
    "cv.fx.0081|beacon/lofi/hall|Crumbled Beacon",
    "cv.fx.0082|shimdrone/lofi/hall|Dusty Shimmer Drone",
    "cv.fx.0083|syncrise+riserstack|Stacked Sync Rise",
    "cv.fx.0084|revharp+impactsub|Boom Reversed Harp",
    "cv.fx.0085|zap+timpimpact|Timpani Laser",
    "cv.fx.0086|glassatmos+dronechoir|Haunted Atmosphere",
    "cv.fx.0087|impact+dronestr|Dread Impact",
    "cv.fx.0088|revpizz+revpianol|Memory Reversed Pizz",
    "cv.fx.0089|wail+windl|Windswept Wail",
    "cv.fx.0090|revpizz+impactsub|Seismic Reversed Pizz",
    "cv.fx.0091|impact+timpimpact|Thunder Impact",
    "cv.fx.0092|tension+dronechoir|Spectral Tension Rise",
    "cv.fx.0093|riser/reverse/hall|Mirror Riser",
    "cv.fx.0094|noiserise/reverse/hall|Reversed Noise Sweep",
    "cv.fx.0095|wail+dronestr|Ominous Wail",
    "cv.fx.0096|revmarimba+revpianol|Echoes Marimba",
    "cv.fx.0097|heart+windl|Stormy Heartbeat",
    "cv.fx.0098|zap/reverse/hall|Mirror Laser",
    "cv.fx.0099|wail/reverse/hall|Mirror Wail",
    "cv.fx.0100|bubbles/reverse/hall|Backward Bubbles",
    "cv.fx.0101|sqriser/reverse/hall|Mirror Square Riser",
    "cv.fx.0102|revmarimba+impactsub|Quake Reversed Marimba",
    "cv.fx.0103|timproll+dronechoir|Ghostly Thunder Roll",
    "cv.fx.0104|heart+dronestr|Brooding Heartbeat",
    "cv.fx.0105|revchoir+windl|Gale Reversed Choir",
    "cv.fx.0106|fall/drive/hall|Driven Downlifter",
    "cv.fx.0107|drone/drive/hall|Burning Drone",
    "cv.fx.0108|impact/drive/hall|Gritty Impact",
    "cv.fx.0109|revvibes+impactsub|Boom Reversed Vibes",
    "cv.fx.0110|metal/drive/hall|Burning Gong Swell",
    "cv.fx.0111|heart+timpimpact|Timpani Heartbeat",
    "cv.fx.0112|heart/drive/hall|Gritty Heartbeat",
    "cv.fx.0113|pedaldrone+dronechoir|Haunted Pedal Drone",
    "cv.fx.0114|pedaldrone+dronestr|Dread Pedal Drone",
    "cv.fx.0115|revbell+revpianol|Memory Reversed Bell",
    "cv.fx.0116|glassatmos+windl|Windswept Atmosphere",
    "cv.fx.0117|revharpsi/drive/hall|Backward Quills Hall",
    "cv.fx.0118|revdulc/drive/hall|Hall Reversed Dulcimer",
    "cv.fx.0119|syncrise/drive/hall|Driven Sync Rise",
    "cv.fx.0120|spacehit/drive/hall|Hot Space Hit",
    "cv.fx.0121|glitch/drive/hall|Gritty Glitch Field",
    "cv.fx.0122|revbell+impactsub|Seismic Reversed Bell",
    "cv.fx.0123|pedaldrone+timpimpact|Thunder Pedal Drone",
    "cv.fx.0124|didgdrone+dronechoir|Spectral Earth Drone",
    "cv.fx.0125|didgdrone+dronestr|Ominous Earth Drone",
    "cv.fx.0126|tension+windl|Stormy Tension Rise",
    "cv.fx.0127|revmarimba+glassl|Starlit Marimba",
    "cv.fx.0128|riser/dark/hall|Shadowed Riser",
    "cv.fx.0129|noiserise/dark/hall|Dark Noise Sweep",
    "cv.fx.0130|fall/dark/hall|Nocturnal Downlifter",
    "cv.fx.0131|texture/dark/hall|Dark Grain Cloud",
    "cv.fx.0132|wind/dark/hall|Midnight Desert Wind",
    "cv.fx.0133|braam/dark/hall|Nocturnal Braam",
    "cv.fx.0134|revpiano/dark/hall|Hall Reversed Piano",
    "cv.fx.0135|revchoir/dark/hall|Hall Reversed Choir",
    "cv.fx.0136|revbell/dark/hall|Shadowed Reversed Bell",
    "cv.fx.0137|tension/dark/hall|Midnight Tension Rise",
    "cv.fx.0138|revkalimba+impactsub|Quake Piano",
    "cv.fx.0139|didgdrone+timpimpact|War Drum Earth Drone",
    "cv.fx.0140|choirdrone+dronestr|Brooding Choir Drone",
    "cv.fx.0141|revharpsi+revpianol|Afterimage Harpsichord",
    "cv.fx.0142|storm/dark/hall|Hall Distant Thunder",
    "cv.fx.0143|timproll+windl|Gale Thunder Roll",
    "cv.fx.0144|sqriser/dark/hall|Shadowed Square Riser",
    "cv.fx.0145|revvibes+glassl|Prism Reversed Vibes",
    "cv.fx.0146|shimdrone/dark/hall|Hall Shimmer Drone",
    "cv.fx.0147|revharpsi+impactsub|Boom Harpsichord",
    "cv.fx.0148|choirdrone+timpimpact|Timpani Choir Drone",
    "cv.fx.0149|storm+dronechoir|Haunted Thunder",
    "cv.fx.0150|fall/airy/vast|Ethereal Downlifter",
    "cv.fx.0151|scifi/airy/vast|Ethereal Data Stream",
    "cv.fx.0152|impact/airy/vast|Airy Impact",
    "cv.fx.0153|braam/airy/vast|Ethereal Braam",
    "cv.fx.0154|metal/airy/vast|Floating Gong Swell",
    "cv.fx.0155|whisper/airy/vast|Airy Whisper Air",
    "cv.fx.0156|revchimes+impactsub|Seismic Chimes",
    "cv.fx.0157|sqriser+timpimpact|Thunder Square Riser",
    "cv.fx.0158|spacehit+dronechoir|Spectral Space Hit",
    "cv.fx.0159|spacehit/airy/vast|Weightless Space Hit",
    "cv.fx.0160|glitch/airy/vast|Airy Glitch Field",
    "cv.fx.0161|revglock+revpianol|Echoes Bars",
    "cv.fx.0162|didgdrone+windl|Stormy Earth Drone",
    "cv.fx.0163|revglock+impactsub|Quake Bars",
    "cv.fx.0164|fmriser+timpimpact|War Drum Tension Riser",
    "cv.fx.0165|shimdrone+dronechoir|Ghostly Shimmer Drone",
    "cv.fx.0166|spacehit+dronestr|Brooding Space Hit",
    "cv.fx.0167|revdulc+revpianol|Afterimage Dulcimer",
    "cv.fx.0168|organdrone/underwater/cloud|Deep Sea Ritual Drone",
    "cv.fx.0169|scifi/underwater/cloud|Sunken Data Stream",
    "cv.fx.0170|wind/underwater/cloud|Underwater Desert Wind",
    "cv.fx.0171|zap/underwater/cloud|Deep Sea Laser",
    "cv.fx.0172|impact/underwater/cloud|Submerged Impact",
    "cv.fx.0173|braam/underwater/cloud|Sunken Braam",
    "cv.fx.0174|revpiano/underwater/cloud|Sunken Reversed Piano",
    "cv.fx.0175|revharp/underwater/cloud|Cloud Reversed Harp",
    "cv.fx.0176|revpizz/underwater/cloud|Deep Sea Reversed Pizz",
    "cv.fx.0177|revmarimba/underwater/cloud|Cloud Reversed Marimba",
    "cv.fx.0178|revbell/underwater/cloud|Deep Sea Reversed Bell",
    "cv.fx.0179|revkalimba/underwater/cloud|Cloud Backward Tines",
    "cv.fx.0180|tension/underwater/cloud|Cloud Tension Rise",
    "cv.fx.0181|timproll/underwater/cloud|Deep Sea Thunder Roll",
    "cv.fx.0182|revguitar/underwater/cloud|Sunken Reversed Guitar",
    "cv.fx.0183|revharpsi/underwater/cloud|Cloud Backward Quills",
    "cv.fx.0184|revglock/underwater/cloud|Cloud Backward Sparkle",
    "cv.fx.0185|revdulc+impactsub|Boom Reversed Dulcimer",
    "cv.fx.0186|revdulc/underwater/cloud|Sunken Backward Folk",
    "cv.fx.0187|pedaldrone/underwater/cloud|Underwater Pedal Drone",
    "cv.fx.0188|syncrise+timpimpact|Timpani Sync Rise",
    "cv.fx.0189|didgdrone/underwater/cloud|Deep Sea Earth Drone",
    "cv.fx.0190|rumble+dronechoir|Haunted Low Rumble",
    "cv.fx.0191|shimdrone+dronestr|Dread Shimmer Drone",
    "cv.fx.0192|fmriser/underwater/cloud|Cloud Tension Riser",
    "cv.fx.0193|storm+revpianol|Memory Distant Thunder",
    "cv.fx.0195|spacehit/underwater/cloud|Deep Sea Space Hit",
    "cv.fx.0196|glitch/underwater/cloud|Submerged Glitch Field",
    "cv.fx.0197|revchimes+glassl|Glimmer Chimes",
    "cv.fx.0198|shimdrone/underwater/cloud|Sunken Shimmer Drone",
    "cv.fx.0199|spacehit+impactsub|Seismic Space Hit",
    "cv.fx.0200|spacehit+timpimpact|Thunder Space Hit",
    "cv.fx.0201|rumble+dronestr|Ominous Low Rumble",
    "cv.fx.0202|syncrise+revpianol|Echoes Sync Rise",
    "cv.fx.0203|shimdrone+windl|Stormy Shimmer Drone",
    "cv.fx.0204|revglock+glassl|Starlit Bars",
    "cv.fx.0205|riser/gatedpad/hall|Throbbing Riser",
    "cv.fx.0206|shimdrone+timpimpact|War Drum Shimmer Drone",
    "cv.fx.0207|fall/gatedpad/hall|Rhythmic Downlifter",
    "cv.fx.0208|drone/gatedpad/hall|Gated Drone",
    "cv.fx.0209|spacehit+revpianol|Afterimage Space Hit",
    "cv.fx.0210|organdrone/gatedpad/hall|Throbbing Ritual Drone",
    "cv.fx.0211|texture/gatedpad/hall|Pulsing Grain Cloud",
    "cv.fx.0212|rumble+windl|Gale Low Rumble",
    "cv.fx.0213|scifi/gatedpad/hall|Rhythmic Data Stream",
    "cv.fx.0214|wind/gatedpad/hall|Gated Desert Wind",
    "cv.fx.0215|impact/gatedpad/hall|Pulsing Impact",
    "cv.fx.0216|braam/gatedpad/hall|Rhythmic Braam",
    "cv.fx.0217|sqriser/gatedpad/hall|Throbbing Square Riser",
    "cv.fx.0218|fmriser/gatedpad/hall|Pulsing Tension Riser",
    "cv.fx.0219|syncrise/gatedpad/hall|Rhythmic Sync Rise",
    "cv.fx.0220|beacon/gatedpad/hall|Gated Beacon",
    "cv.fx.0221|glitch/gatedpad/hall|Pulsing Glitch Field",
    "cv.fx.0222|shimdrone/gatedpad/hall|Rhythmic Shimmer Drone",
    "cv.fx.0223|rumble/gatedpad/hall|Gated Low Rumble",
    "cv.fx.0224|glitch+revpianol|Memory Glitch Field",
    "cv.fx.0225|shimdrone+revpianol|Echoes Shimmer Drone",
    "cv.fx.0226|didgdrone+glassl|Starlit Earth Drone",
    "cv.fx.0227|sqriser+glassl|Starlit Square Riser",
    "cv.fx.0228|fmriser+glassl|Prism Tension Riser",
    "cv.fx.0229|spacehit+glassl|Starlit Space Hit",
    "cv.fx.0230|rumble+glassl|Glimmer Low Rumble",
    "cv.lead.0001|thin/natural/room|Thin Lead",
    "cv.lead.0002|square/natural/room|Square Lead",
    "cv.lead.0003|saw/natural/room|Saw Lead",
    "cv.lead.0004|whistle/natural/room|Whistle Lead",
    "cv.lead.0005|glidesine/natural/room|Glide Sine",
    "cv.lead.0006|dist/natural/room|Distorted Lead",
    "cv.lead.0008|voxlead/natural/room|Talk Lead",
    "cv.lead.0011|wtlead/natural/room|Digital Lead",
    "cv.lead.0012|synclead/natural/room|Sync Lead",
    "cv.lead.0013|chip/natural/room|Chip Lead",
    "cv.lead.0014|octlead/natural/room|Octave Lead",
    "cv.lead.0015|theremin/natural/room|Theremin",
    "cv.lead.0016|acidlead/natural/room|Rubber Lead",
    "cv.lead.0017|softsq/natural/room|Soft Lead",
    "cv.lead.0018|radio/natural/room|Radio Lead",
    "cv.lead.0020|fuzzsine/natural/room|Fuzz Sine",
    "cv.lead.0021|glasslead/natural/room|Glass Lead",
    "cv.lead.0022|belllead/natural/room|Bell Lead",
    "cv.lead.0023|lowglide/natural/room|Low Glide Lead",
    "cv.lead.0027|pulsechip/natural/room|Pulse Chip Lead",
    "cv.lead.0028|laserlead/natural/room|Laser Lead",
    "cv.lead.0030|thin+flutelead|Fluted Thin Lead",
    "cv.lead.0031|thin+harmlead|Blues Thin Lead",
    "cv.lead.0032|square+saxlead|Smoky Square Lead",
    "cv.lead.0033|thin+mutelead|Harmon Thin Lead",
    "cv.lead.0034|square+voicelead|Siren Square Lead",
    "cv.lead.0035|thin+octdown|Twin Octave Thin Lead",
    "cv.lead.0036|whistle+sqlead2|Doubled Square Lead",
    "cv.lead.0037|duosqsaw/natural/hall|Duo Square Saw",
    "cv.lead.0038|square+flutelead|Pastoral Square Lead",
    "cv.lead.0039|square+harmlead|Blues Square Lead",
    "cv.lead.0040|square+mutelead|Noir Square Lead",
    "cv.lead.0041|square+octdown|Low Double Square Lead",
    "cv.lead.0042|square+octupl|Halo Square Lead",
    "cv.lead.0043|whistle+flutelead|Whistle Lead & Flute",
    "cv.lead.0045|dist+saxlead|Distorted Lead & Sax",
    "cv.lead.0046|whistle+violinlead|Whistle Lead & Violin",
    "cv.lead.0047|saw+mutelead|Muted Trumpet Saw Lead",
    "cv.lead.0048|saw+octdown|Octave Saw Lead",
    "cv.lead.0049|saw+whistlel|Whistling Saw Lead",
    "cv.lead.0050|pwmlead/natural/echo|PWM Lead",
    "cv.lead.0051|glidesine+sqlead2|Reinforced Glide Sine",
    "cv.lead.0052|glasslead/natural/echo|Echo Glass Lead",
    "cv.lead.0053|dist+harmlead|Blues Distorted Lead",
    "cv.lead.0054|fmlead+saxlead|Smoky FM Lead",
    "cv.lead.0055|dist+mutelead|Harmon Distorted Lead",
    "cv.lead.0056|glidesine+voicelead|Siren Glide Sine",
    "cv.lead.0057|superlead+octdown|Twin Octave Lead",
    "cv.lead.0058|dist+whistlel|Piped Distorted Lead",
    "cv.lead.0059|glidesine+flutelead|Pastoral Glide Sine",
    "cv.lead.0060|organlead+harmlead|Organ Lead & Harmonica",
    "cv.lead.0061|voxlead+saxlead|Saxed Talk Lead",
    "cv.lead.0062|fmlead+mutelead|Noir FM Lead",
    "cv.lead.0063|whistle+octdown|Low Double Lead",
    "cv.lead.0064|laserlead/natural/cloud|Cloud Laser Lead",
    "cv.lead.0065|voxlead+sqlead2|Twin Square Talk Lead",
    "cv.lead.0066|brasslead+mutelead|Muted Trumpet Lead",
    "cv.lead.0067|voxlead+voicelead|Talk Lead & Voice",
    "cv.lead.0068|flutelead+octdown|Octave Pan Lead",
    "cv.lead.0069|thin/muted/room|Hushed Thin Lead",
    "cv.lead.0071|dist/muted/room|Velvet Distorted Lead",
    "cv.lead.0072|voxlead/muted/room|Dusky Talk Lead",
    "cv.lead.0074|wtlead/muted/room|Muted Digital Lead",
    "cv.lead.0075|synclead/muted/room|Dusky Sync Lead",
    "cv.lead.0076|chip/muted/room|Velvet Chip Lead",
    "cv.lead.0077|octlead/muted/room|Muted Octave Lead",
    "cv.lead.0078|acidlead/muted/room|Velvet Rubber Lead",
    "cv.lead.0079|softsq/muted/room|Muted Soft Lead",
    "cv.lead.0080|pulsechip/muted/room|Dusky Pulse Chip Lead",
    "cv.lead.0081|laserlead/muted/room|Velvet Laser Lead",
    "cv.lead.0082|wtlead+flutelead|Fluted Digital Lead",
    "cv.lead.0083|chip+mutelead|Harmon Chip Lead",
    "cv.lead.0084|glidesine+octdown|Twin Octave Glide Sine",
    "cv.lead.0085|octlead+harmlead|Deep Lead & Harmonica",
    "cv.lead.0086|dist+octdown|Low Double Fuzz Lead",
    "cv.lead.0087|acidlead/echo/echo|Ricochet Rubber Lead",
    "cv.lead.0088|acidlead+harmlead|Acid Lead & Harmonica",
    "cv.lead.0089|octlead+saxlead|Octave Lead & Sax",
    "cv.lead.0090|octlead+mutelead|Harmon Octave Lead",
    "cv.lead.0091|fold+octdown|Octave Folded Lead",
    "cv.lead.0092|acidlead+mutelead|Harmon Rubber Lead",
    "cv.lead.0093|fmlead+octdown|Twin Octave FM Lead",
    "cv.lead.0094|square/lofi/room|Lo-Fi Square Lead",
    "cv.lead.0096|theremin+violinlead|Gypsy Theremin",
    "cv.lead.0097|voxlead+octdown|Low Double Talk Lead",
    "cv.lead.0098|wahlead+harmlead|Wah Lead & Harmonica",
    "cv.lead.0099|radio+mutelead|Harmon Radio Lead",
    "cv.lead.0100|organlead+octdown|Octave Organ Lead",
    "cv.lead.0101|thin/wide/hall|Panoramic Thin Lead",
    "cv.lead.0102|superlead/wide/hall|Liquid Supersaw Lead",
    "cv.lead.0103|whistle/wide/hall|Panoramic Whistle Lead",
    "cv.lead.0104|voxlead/wide/hall|Silk Talk Lead",
    "cv.lead.0105|wtlead/wide/hall|Wide Digital Lead",
    "cv.lead.0106|synclead/wide/hall|Silk Sync Lead",
    "cv.lead.0107|chip/wide/hall|Liquid Chip Lead",
    "cv.lead.0108|octlead/wide/hall|Wide Octave Lead",
    "cv.lead.0109|acidlead/wide/hall|Liquid Rubber Lead",
    "cv.lead.0110|pwmlead/wide/hall|Panoramic PWM Lead",
    "cv.lead.0111|softsq/wide/hall|Wide Soft Lead",
    "cv.lead.0112|radio/wide/hall|Silk Radio Lead",
    "cv.lead.0113|fuzzsine/wide/hall|Panoramic Fuzz Sine",
    "cv.lead.0114|glasslead/wide/hall|Wide Glass Lead",
    "cv.lead.0115|wahlead/wide/hall|Liquid Wah Lead",
    "cv.lead.0116|lowglide/wide/hall|Hall Low Glide Lead",
    "cv.lead.0117|duosqsaw/wide/hall|Hall Duo Square Saw",
    "cv.lead.0118|pulsechip/wide/hall|Silk Pulse Chip Lead",
    "cv.lead.0119|fuzzsine+voicelead|Siren Fuzz Sine",
    "cv.lead.0121|theremin+octupl|Overtone Theremin",
    "cv.lead.0122|belllead+whistlel|Piped Bell Lead",
    "cv.lead.0123|fuzzsine+flutelead|Pastoral Fuzz Sine",
    "cv.lead.0124|wtlead+octdown|Low Double Grid Lead",
    "cv.lead.0125|thin/octbody/room|Heavy Thin Lead",
    "cv.lead.0126|wahlead+whistlel|Bird Wah Lead",
    "cv.lead.0127|whistle/octbody/room|Heavy Whistle Lead",
    "cv.lead.0128|voxlead/octbody/room|Doubled Talk Lead",
    "cv.lead.0129|synclead/octbody/room|Doubled Sync Lead",
    "cv.lead.0130|chip/octbody/room|Thick Chip Lead",
    "cv.lead.0131|acidlead/octbody/room|Thick Rubber Lead",
    "cv.lead.0132|radio/octbody/room|Doubled Radio Lead",
    "cv.lead.0133|modellead/drive/room|Room Siren Voice Lead",
    "cv.lead.0134|fuzzsine/octbody/room|Heavy Fuzz Sine",
    "cv.lead.0135|glasslead/octbody/room|Deep Glass Lead",
    "cv.lead.0136|belllead/octbody/room|Doubled Bell Lead",
    "cv.lead.0137|wahlead/octbody/room|Thick Wah Lead",
    "cv.lead.0138|ohlead/drive/room|Gritty Oh Voice Lead",
    "cv.lead.0139|pipelead/octbody/room|Deep Pipe Lead",
    "cv.lead.0140|pulsechip/octbody/room|Room Pulse Chip Lead",
    "cv.lead.0141|laserlead/octbody/room|Thick Laser Lead",
    "cv.lead.0142|cleanflute/octbody/room|Heavy Clean Flute Lead",
    "cv.lead.0143|fuzzsine+violinlead|Fuzz Sine & Violin",
    "cv.lead.0144|lowglide+voicelead|Low Glide Lead & Voice",
    "cv.lead.0145|duosqsaw+whistlel|Whistling Saw",
    "cv.lead.0146|lowglide+flutelead|Fluted Low Glide Lead",
    "cv.lead.0147|pulsechip+mutelead|Harmon Pulse Chip Lead",
    "cv.lead.0148|square/fifth/room|Fifth Square Lead",
    "cv.lead.0149|superlead/fifth/room|Open Supersaw Lead",
    "cv.lead.0150|whistle/fifth/room|Quintal Whistle Lead",
    "cv.lead.0151|flutelead/fifth/room|Fifth Pan Lead",
    "cv.lead.0152|glidesine/fifth/room|Power Glide Sine",
    "cv.lead.0153|dist/fifth/room|Open Distorted Lead",
    "cv.lead.0154|fold/fifth/room|Quintal Folded Lead",
    "cv.lead.0155|fmlead/fifth/room|Fifth FM Lead",
    "cv.lead.0156|organlead/fifth/room|Open Organ Lead",
    "cv.lead.0157|wtlead/fifth/room|Fifth Digital Lead",
    "cv.lead.0158|chip/fifth/room|Open Chip Lead",
    "cv.lead.0159|acidlead/fifth/room|Open Rubber Lead",
    "cv.lead.0160|fuzzsine/fifth/room|Quintal Fuzz Sine",
    "cv.lead.0161|glasslead/fifth/room|Fifth Glass Lead",
    "cv.lead.0162|belllead/fifth/room|Power Bell Lead",
    "cv.lead.0163|wahlead/fifth/room|Open Wah Lead",
    "cv.lead.0164|lowglide/fifth/room|Quintal Low Glide Lead",
    "cv.lead.0165|pipelead/fifth/room|Fifth Pipe Lead",
    "cv.lead.0166|pulsechip/fifth/room|Power Pulse Chip Lead",
    "cv.lead.0167|lowglide+violinlead|Basement Lead & Violin",
    "cv.lead.0168|octlead+octdown|Octave Deep Lead",
    "cv.lead.0169|fuzzsine+octupl|Shine Fuzz Sine",
    "cv.lead.0170|square/octup/hall|Shining Square Lead",
    "cv.lead.0171|flutelead/octup/hall|Shining Pan Lead",
    "cv.lead.0173|wtlead/octup/hall|Shining Digital Lead",
    "cv.lead.0174|glasslead/octup/hall|Shining Glass Lead",
    "cv.lead.0175|belllead/octup/hall|Overtone Bell Lead",
    "cv.lead.0176|lowglide/octup/hall|Lifted Low Glide Lead",
    "cv.lead.0177|voxlead/drive/room|Driven Talk Lead",
    "cv.lead.0178|belllead/drive/room|Driven Bell Lead",
    "cv.lead.0179|lowglide/drive/room|Hot Low Glide Lead",
    "cv.lead.0180|thin/bright/room|Crisp Thin Lead",
    "cv.lead.0181|square/bright/room|Bright Square Lead",
    "cv.lead.0182|superlead/bright/room|Clear Supersaw Lead",
    "cv.lead.0183|whistle/bright/room|Crisp Whistle Lead",
    "cv.lead.0184|synclead/bright/room|Open Sync Lead",
    "cv.lead.0185|chip/bright/room|Clear Chip Lead",
    "cv.lead.0186|octlead/bright/room|Bright Octave Lead",
    "cv.lead.0187|softsq/bright/room|Bright Soft Lead",
    "cv.lead.0188|fuzzsine+octdown|Octave Fuzz Sine",
    "cv.lead.0189|pulsechip/bright/room|Open Pulse Chip Lead",
    "cv.lead.0190|wahlead+octdown|Octave Wah Lead",
    "cv.lead.0191|twin/swell/hall|Bowed Twin Lead",
    "cv.lead.0192|radio/swell/hall|Breathy Radio Lead",
    "cv.lead.0193|pulsechip/swell/hall|Hall Pulse Chip Lead",
    "cv.lead.0194|lowglide+octdown|Twin Octave Deep Glide",
    "cv.lead.0195|thin/hollow/room|Pinched Thin Lead",
    "cv.lead.0196|square/hollow/room|Hollow Square Lead",
    "cv.lead.0197|superlead/hollow/room|Honky Supersaw Lead",
    "cv.lead.0198|synclead/hollow/room|Nasal Sync Lead",
    "cv.lead.0199|chip/hollow/room|Honky Chip Lead",
    "cv.lead.0200|octlead/hollow/room|Hollow Octave Lead",
    "cv.lead.0201|pulsechip+octdown|Twin Octave Bleep Lead",
    "cv.lead.0202|duosqsaw/hollow/room|Pinched Duo Square Saw",
    "cv.lead.0203|whistle+padpluck|Whistle Lead & Pad",
    "cv.lead.0204|glidesine+padpluck|Padded Glide Sine",
    "cv.lead.0205|fuzzsine+padpluck|Padded Fuzz Sine",
    "cv.lead.0206|belllead+padpluck|Cushioned Bell Lead",
    "cv.lead.0207|ohlead+padpluck|Cushioned Lead",
    "cv.lead.0208|breathylead+padpluck|Breathy Lead & Pad",
    "cv.lead.0209|laserlead+padpluck|Cushioned Laser Lead",
    "cv.pad.0001|warm/natural/hall|Warm Pad",
    "cv.pad.0002|pwmpad/natural/hall|PWM Pad",
    "cv.pad.0003|softpad/natural/hall|Soft Pad",
    "cv.pad.0004|glasspad/natural/hall|Glass Pad",
    "cv.pad.0005|morphpad/natural/hall|Morph Pad",
    "cv.pad.0006|vowelpad/natural/hall|Vowel Pad",
    "cv.pad.0009|organpad/natural/hall|Organ Pad",
    "cv.pad.0010|breathpad/natural/hall|Breath Pad",
    "cv.pad.0011|fmpad/natural/hall|FM Pad",
    "cv.pad.0013|darkpad/natural/hall|Dark Pad",
    "cv.pad.0014|hollowpad/natural/hall|Hollow Pad",
    "cv.pad.0015|syncpad/natural/hall|Sync Pad",
    "cv.pad.0016|violapad/natural/hall|Viola Haze",
    "cv.pad.0019|choirpad/natural/hall|Choir Pad",
    "cv.pad.0020|aahpad/natural/hall|Aah Choir",
    "cv.pad.0021|voxpad/natural/hall|Lone Voice Pad",
    "cv.pad.0022|pipepad/natural/hall|Cathedral Pad",
    "cv.pad.0023|chamberpad/natural/hall|Chapel Pad",
    "cv.pad.0024|hornpad/natural/hall|Horn Pad",
    "cv.pad.0025|clarpad/natural/hall|Clarinet Haze",
    "cv.pad.0027|bloompad/natural/hall|Bloom Pad",
    "cv.pad.0028|digipad/natural/hall|Digital Pad",
    "cv.pad.0029|sqpad/natural/hall|Square Pad",
    "cv.pad.0030|noisepad/natural/hall|Rain Pad",
    "cv.pad.0031|oohpad/natural/hall|Ooh Pad",
    "cv.pad.0032|bassoonpad/natural/hall|Bassoon Haze",
    "cv.pad.0033|saxpad/natural/hall|Smoky Sax Pad",
    "cv.pad.0034|harmpad/natural/hall|Harmonica Haze",
    "cv.pad.0038|malechoir/natural/hall|Monk Pad",
    "cv.pad.0040|vowelpad+sinesupport|Rounded Vowel Pad",
    "cv.pad.0041|warm+subpad|Grounded Warm Pad",
    "cv.pad.0042|softpad+heaven|Sanctuary Soft Pad",
    "cv.pad.0043|softpad+bowedvib|Halo Soft Pad",
    "cv.pad.0044|softpad+fluteair|Pastoral Soft Pad",
    "cv.pad.0045|vowelpad+heaven|Seraph Vowel Pad",
    "cv.pad.0046|softpad+cellopadl|Soft Pad & Cellos",
    "cv.pad.0047|pwmpad+subpad|Earth PWM Pad",
    "cv.pad.0049|softpad+strpad|Orchestral Soft Pad",
    "cv.pad.0050|morphpad+bowedvib|Aura Morph Pad",
    "cv.pad.0051|softpad+subpad|Grounded Soft Pad",
    "cv.pad.0052|halo+heaven|Sanctuary Halo Pad",
    "cv.pad.0053|morphpad/muted/hall|Dusky Morph Pad",
    "cv.pad.0054|darkpad/muted/hall|Velvet Dark Pad",
    "cv.pad.0055|fmpad+bowedvib|Halo FM Pad",
    "cv.pad.0056|violapad/muted/hall|Velvet Viola Haze",
    "cv.pad.0057|violapad+sinesupport|Velvet Haze",
    "cv.pad.0058|cellopad/muted/hall|Hushed Cello Dusk",
    "cv.pad.0059|glasspad+subpad|Low Glass Pad",
    "cv.pad.0060|cellotrem/muted/hall|Dusky Tremor Pad",
    "cv.pad.0061|clarpad/muted/hall|Muted Clarinet Haze",
    "cv.pad.0062|bloompad/muted/hall|Muted Bloom Pad",
    "cv.pad.0063|digipad/muted/hall|Dusky Digital Pad",
    "cv.pad.0064|violapad+heaven|Seraph Viola Haze",
    "cv.pad.0065|noisepad/muted/hall|Hushed Rain Pad",
    "cv.pad.0066|oohpad/muted/hall|Muted Ooh Pad",
    "cv.pad.0067|harmpad/muted/hall|Hushed Harmonica Haze",
    "cv.pad.0068|principalpad/muted/hall|Velvet Principal Pad",
    "cv.pad.0069|bellpad+bowedvib|Bowed Bars Bell Pad",
    "cv.pad.0070|morphpad+subpad|Earth Morph Pad",
    "cv.pad.0071|fmpad+vocpadl|FM Pad & Voices",
    "cv.pad.0072|fmpad/lofi/hall|Bedroom FM Pad",
    "cv.pad.0073|bellpad/lofi/hall|Lo-Fi Bell Pad",
    "cv.pad.0075|vowelpad+strpad|Orchestral Vowel Pad",
    "cv.pad.0076|bellpad+cellopadl|Low Bowed Bell Pad",
    "cv.pad.0077|digipad/lofi/hall|Dusty Digital Pad",
    "cv.pad.0078|sqpad/lofi/hall|Crumbled Square Pad",
    "cv.pad.0079|halo+glasspadl|Shimmer Halo Pad",
    "cv.pad.0080|halo+bowedvib|Aura Halo Pad",
    "cv.pad.0081|vowelpad+subpad|Grounded Vowel Pad",
    "cv.pad.0082|bellpad+vocpadl|Angel Bell Pad",
    "cv.pad.0083|darkpad+cinepad|Film Score Dark Pad",
    "cv.pad.0084|synthchoir+strpad|Bowed Synth Choir",
    "cv.pad.0085|cellotrem+sinesupport|Velvet Tremor Pad",
    "cv.pad.0087|syncpad+fluteair|Pastoral Sync Pad",
    "cv.pad.0088|halo/wide/hall|Silk Halo Pad",
    "cv.pad.0089|saxpad/wide/hall|Liquid Smoky Sax Pad",
    "cv.pad.0090|strmachine+subpad|Earth String Machine",
    "cv.pad.0091|aahpad+sinesupport|Rounded Aah Choir",
    "cv.pad.0092|organpad+subpad|Grounded Organ Pad",
    "cv.pad.0093|choirpad+hornpadl|Heroic Choir Pad",
    "cv.pad.0094|shimmer/octbody/hall|Doubled Shimmer Pad",
    "cv.pad.0095|softpad/octbody/hall|Heavy Soft Pad",
    "cv.pad.0096|glasspad/octbody/hall|Deep Glass Pad",
    "cv.pad.0097|morphpad/octbody/hall|Doubled Morph Pad",
    "cv.pad.0098|vowelpad/octbody/hall|Thick Vowel Pad",
    "cv.pad.0099|synthchoir/octbody/hall|Heavy Synth Choir",
    "cv.pad.0100|breathpad/octbody/hall|Thick Breath Pad",
    "cv.pad.0101|fmpad/octbody/hall|Heavy FM Pad",
    "cv.pad.0102|bellpad/octbody/hall|Deep Bell Pad",
    "cv.pad.0103|halo/octbody/hall|Doubled Halo Pad",
    "cv.pad.0104|darkpad/octbody/hall|Thick Dark Pad",
    "cv.pad.0105|hollowpad/octbody/hall|Heavy Hollow Pad",
    "cv.pad.0106|syncpad/octbody/hall|Deep Sync Pad",
    "cv.pad.0107|brasspad/octbody/hall|Heavy Brass Pad",
    "cv.pad.0108|bloompad/octbody/hall|Deep Bloom Pad",
    "cv.pad.0109|digipad/octbody/hall|Doubled Digital Pad",
    "cv.pad.0110|sqpad/octbody/hall|Thick Square Pad",
    "cv.pad.0111|oohpad/octbody/hall|Deep Ooh Pad",
    "cv.pad.0112|fmpad+subpad|Earth FM Pad",
    "cv.pad.0113|voxpad+hornpadl|Lone Voice Pad & Horns",
    "cv.pad.0114|digipad+cinepad|Cinematic Digital Pad",
    "cv.pad.0115|shimmer/fifth/hall|Power Shimmer Pad",
    "cv.pad.0116|softpad/fifth/hall|Quintal Soft Pad",
    "cv.pad.0117|glasspad/fifth/hall|Fifth Glass Pad",
    "cv.pad.0118|morphpad/fifth/hall|Power Morph Pad",
    "cv.pad.0119|vowelpad/fifth/hall|Open Vowel Pad",
    "cv.pad.0120|synthchoir/fifth/hall|Quintal Synth Choir",
    "cv.pad.0121|organpad/fifth/hall|Power Organ Pad",
    "cv.pad.0122|fmpad/fifth/hall|Quintal FM Pad",
    "cv.pad.0123|bellpad/fifth/hall|Fifth Bell Pad",
    "cv.pad.0124|halo/fifth/hall|Power Halo Pad",
    "cv.pad.0125|darkpad/fifth/hall|Open Dark Pad",
    "cv.pad.0126|violapad/bright/hall|Clear Viola Haze",
    "cv.pad.0129|aahpad/bright/hall|Crisp Aah Choir",
    "cv.pad.0130|voxpad/bright/hall|Bright Lone Voice Pad",
    "cv.pad.0132|clarpad/bright/hall|Bright Clarinet Haze",
    "cv.pad.0133|brasspad/fifth/hall|Quintal Brass Pad",
    "cv.pad.0134|digipad/fifth/hall|Power Digital Pad",
    "cv.pad.0135|sqpad/fifth/hall|Open Square Pad",
    "cv.pad.0136|bassoonpad/bright/hall|Open Bassoon Haze",
    "cv.pad.0137|harmpad/bright/hall|Crisp Harmonica Haze",
    "cv.pad.0139|vocalpadm+strpad|Orchestral Choir",
    "cv.pad.0140|pipepad+hornpadl|Heroic Cathedral Pad",
    "cv.pad.0141|softpad/octup/hall|Lifted Soft Pad",
    "cv.pad.0142|morphpad/octup/hall|Overtone Morph Pad",
    "cv.pad.0143|organpad/octup/hall|Overtone Organ Pad",
    "cv.pad.0144|fmpad/octup/hall|Lifted FM Pad",
    "cv.pad.0145|bellpad/octup/hall|Shining Bell Pad",
    "cv.pad.0146|voxpad/dark/hall|Dark Lone Voice Pad",
    "cv.pad.0147|digipad/octup/hall|Overtone Digital Pad",
    "cv.pad.0148|sqpad/octup/hall|Sparkling Square Pad",
    "cv.pad.0149|fullorgpad/dark/hall|Dark Full Organ Pad",
    "cv.pad.0150|clarpad+sinesupport|Warm Clarinet Haze",
    "cv.pad.0151|voxpad+fluteair|Lone Voice Pad & Flute",
    "cv.pad.0152|hollowpad+subpad|Grounded Hollow Pad",
    "cv.pad.0153|pipepad+fluteair|Woodland Cathedral Pad",
    "cv.pad.0154|clarpad+hornpadl|Heroic Clarinet Haze",
    "cv.pad.0155|hollowpad/reverse/hall|Mirror Hollow Pad",
    "cv.pad.0156|aahpad+cellopadl|Earthen Aah Choir",
    "cv.pad.0158|voxpad+choirpadl|Hymnal Lone Voice Pad",
    "cv.pad.0159|voxpad+bowedvib|Halo Lone Voice Pad",
    "cv.pad.0160|aahpad+vocpadl|Vocal Aah Choir",
    "cv.pad.0162|pipepad+choirpadl|Cathedral Pad & Choir",
    "cv.pad.0163|pipepad+bowedvib|Bowed Bars Pad",
    "cv.pad.0164|violapad+subpad|Earth Viola Haze",
    "cv.pad.0165|pwmpad/bright/hall|Clear PWM Pad",
    "cv.pad.0166|morphpad/bright/hall|Open Morph Pad",
    "cv.pad.0167|strmachine/bright/hall|Bright String Machine",
    "cv.pad.0168|breathpad/bright/hall|Clear Breath Pad",
    "cv.pad.0169|malechoir+strpad|Orchestral Monk Pad",
    "cv.pad.0170|darkpad/bright/hall|Clear Dark Pad",
    "cv.pad.0173|cellopad/underwater/cloud|Deep Sea Cello Dusk",
    "cv.pad.0174|cellotrem/underwater/cloud|Sunken Tremor Pad",
    "cv.pad.0175|chamberpad+bowedvib|Aura Chapel Pad",
    "cv.pad.0176|clarpad/underwater/cloud|Cloud Clarinet Haze",
    "cv.pad.0177|cellopad+subpad|Grounded Cello Dusk",
    "cv.pad.0178|brasspad/bright/hall|Crisp Brass Pad",
    "cv.pad.0179|pipepad+vocpadl|Angel Cathedral Pad",
    "cv.pad.0180|sqpad/bright/hall|Clear Square Pad",
    "cv.pad.0181|harmpad/underwater/cloud|Cloud Harmonica Haze",
    "cv.pad.0182|principalpad/underwater/cloud|Cloud Principal Pad",
    "cv.pad.0183|tremopad+subpad|Low Tremolo Mist",
    "cv.pad.0184|hornpad+cellopadl|Horn Pad & Cellos",
    "cv.pad.0185|synthchoir/dark/hall|Shadowed Synth Choir",
    "cv.pad.0186|clarpad+cellopadl|Low Bowed Haze",
    "cv.pad.0187|principalpad+sinesupport|Rounded Principal Pad",
    "cv.pad.0188|choirpad+subpad|Grounded Choir Pad",
    "cv.pad.0189|clarpad+vocpadl|Angel Clarinet Haze",
    "cv.pad.0190|digipad+fluteair|Woodland Digital Pad",
    "cv.pad.0191|digipad+femalepad|Lullaby Digital Pad",
    "cv.pad.0192|aahpad+subpad|Low Aah Choir",
    "cv.pad.0193|bassoonpad+fluteair|Pastoral Bassoon Haze",
    "cv.pad.0194|digipad+bowedvib|Bowed Bars Digital Pad",
    "cv.pad.0195|voxpad+subpad|Earth Lone Voice Pad",
    "cv.pad.0196|pipepad+subpad|Grounded Cathedral Pad",
    "cv.pad.0197|chamberpad+subpad|Low Chapel Pad",
    "cv.pad.0198|fullorgpad+fluteair|Pastoral Pad",
    "cv.pad.0199|fourpad+organpadl|Four Foot Pad & Organ",
    "cv.pad.0200|hornpad+subpad|Earth Horn Pad",
    "cv.pad.0201|saxpad+vocpadl|Smoky Sax Pad & Voices",
    "cv.pad.0202|fourpad+fluteair|Four Foot Pad & Flute",
    "cv.pad.0203|vowelpad/underwater/cloud|Underwater Vowel Pad",
    "cv.pad.0204|synthchoir/underwater/cloud|Deep Sea Synth Choir",
    "cv.pad.0205|bellpad/underwater/cloud|Submerged Bell Pad",
    "cv.pad.0206|syncpad/underwater/cloud|Submerged Sync Pad",
    "cv.pad.0207|brasspad/underwater/cloud|Deep Sea Brass Pad",
    "cv.pad.0208|digipad/underwater/cloud|Sunken Digital Pad",
    "cv.pad.0209|oohpad/underwater/cloud|Submerged Ooh Pad",
    "cv.pad.0210|clarpad+subpad|Grounded Clarinet Haze",
    "cv.pad.0211|fullorgpad+cellopadl|Earthen Full Organ Pad",
    "cv.pad.0212|fourpad+vocpadl|Four Foot Pad & Voices",
    "cv.pad.0213|digipad+subpad|Grounded Digital Pad",
    "cv.pad.0214|warm/gatedpad/hall|Pulsing Warm Pad",
    "cv.pad.0215|pwmpad/gatedpad/hall|Gated PWM Pad",
    "cv.pad.0216|morphpad/gatedpad/hall|Rhythmic Morph Pad",
    "cv.pad.0217|vowelpad/gatedpad/hall|Gated Vowel Pad",
    "cv.pad.0218|synthchoir/gatedpad/hall|Throbbing Synth Choir",
    "cv.pad.0219|strmachine/gatedpad/hall|Pulsing String Machine",
    "cv.pad.0220|organpad/gatedpad/hall|Rhythmic Organ Pad",
    "cv.pad.0221|breathpad/gatedpad/hall|Gated Breath Pad",
    "cv.pad.0222|halo/gatedpad/hall|Rhythmic Halo Pad",
    "cv.pad.0223|darkpad/gatedpad/hall|Gated Dark Pad",
    "cv.pad.0224|hollowpad/gatedpad/hall|Throbbing Hollow Pad",
    "cv.pad.0225|bloompad/gatedpad/hall|Pulsing Bloom Pad",
    "cv.pad.0226|digipad/gatedpad/hall|Rhythmic Digital Pad",
    "cv.pad.0227|sqpad+subpad|Low Square Pad",
    "cv.pad.0228|sqpad/gatedpad/hall|Gated Square Pad",
    "cv.pad.0229|oohpad/gatedpad/hall|Pulsing Ooh Pad",
    "cv.pad.0230|noisepad+subpad|Earth Rain Pad",
    "cv.pad.0231|bassoonpad+subpad|Low Bassoon Haze",
    "cv.pad.0232|saxpad+subpad|Earth Smoky Sax Pad",
    "cv.pad.0233|harmpad+subpad|Grounded Haze",
    "cv.pad.0234|fullorgpad+subpad|Low Full Organ Pad",
    "cv.pad.0235|principalpad+subpad|Grounded Principal Pad",
    "cv.pluck.0001|sawpluck/natural/room|Saw Pluck",
    "cv.pluck.0002|sqpluck/natural/room|Square Pluck",
    "cv.pluck.0003|pulsepluck/natural/room|Reed Pluck",
    "cv.pluck.0004|kswire/natural/room|Wire Pluck",
    "cv.pluck.0005|fmglass/natural/room|Glass Pluck",
    "cv.pluck.0006|fmwood/natural/room|Wood Pluck",
    "cv.pluck.0009|modalmar/natural/room|Mallet Pluck",
    "cv.pluck.0011|wtprism/natural/room|Prism Pluck",
    "cv.pluck.0012|wtdigi/natural/room|Digital Pluck",
    "cv.pluck.0013|wttri/natural/room|Soft Pluck",
    "cv.pluck.0014|syncpluck/natural/room|Sync Pluck",
    "cv.pluck.0015|organpluck/natural/room|Click Pluck",
    "cv.pluck.0017|voxpluck/natural/room|Vocal Pluck",
    "cv.pluck.0018|orchpizz/natural/room|Pizz Pluck",
    "cv.pluck.0019|ladderpluck/natural/room|Rubber Pluck",
    "cv.pluck.0020|bandpluck/natural/room|Tube Pluck",
    "cv.pluck.0021|droplet/natural/room|Water Pluck",
    "cv.pluck.0022|fmmetal/natural/room|Metal Pluck",
    "cv.pluck.0025|dulcpluck/natural/room|Dulcimer Pluck",
    "cv.pluck.0026|steelpan/natural/room|Steel Pan Pluck",
    "cv.pluck.0027|celestapl/natural/room|Celesta Pluck",
    "cv.pluck.0030|zitherpl/natural/room|Silk Zither Pluck",
    "cv.pluck.0032|marimbapl/natural/room|Wood Bar Pluck",
    "cv.pluck.0034|duopluck/natural/room|Duo Pluck",
    "cv.pluck.0035|chippluck/natural/room|8-Bit Pluck",
    "cv.pluck.0036|fmbellpl/natural/room|Bell Pluck",
    "cv.pluck.0037|pwmpluck/natural/room|Liquid Pluck",
    "cv.pluck.0038|organks/natural/room|Hybrid Organ Pluck",
    "cv.pluck.0040|pulsepluck/natural/hall|Hall Reed Pluck",
    "cv.pluck.0041|kswire+harphyb|Harpstring Wire Pluck",
    "cv.pluck.0042|sawpluck+marhyb|Rosewood Saw Pluck",
    "cv.pluck.0043|sqpluck+kalhyb|Thumb Square Pluck",
    "cv.pluck.0044|fmwood/natural/hall|Hall Wood Pluck",
    "cv.pluck.0045|fmglass+vibhyb|Vibe Glass Pluck",
    "cv.pluck.0046|sawpluck+subpluck|Low End Saw Pluck",
    "cv.pluck.0047|sawpluck+padpluck|Cushioned Saw Pluck",
    "cv.pluck.0048|organpluck/natural/hall|Hall Click Pluck",
    "cv.pluck.0049|orchpizz/natural/hall|Hall Pizz Pluck",
    "cv.pluck.0050|ladderpluck/natural/hall|Hall Rubber Pluck",
    "cv.pluck.0051|sawpluck+ksdouble|Double Saw Pluck",
    "cv.pluck.0052|sawpluck+dulchyb|Hammered Saw Pluck",
    "cv.pluck.0054|marimbapl/natural/hall|Hall Wood Bar Pluck",
    "cv.pluck.0055|chippluck/natural/hall|Hall 8-Bit Pluck",
    "cv.pluck.0056|organks/natural/hall|Reed String Pluck",
    "cv.pluck.0058|sqpluck+pizzhyb|Bowstring Square Pluck",
    "cv.pluck.0059|sqpluck+marhyb|Mallet Square Pluck",
    "cv.pluck.0060|fmglass+kalhyb|Tine Glass Pluck",
    "cv.pluck.0061|fmwood+vibhyb|Lounge Wood Pluck",
    "cv.pluck.0062|sqpluck+guitarhyb|Guitar Square Pluck",
    "cv.pluck.0063|sqpluck+subpluck|Weighted Square Pluck",
    "cv.pluck.0064|sqpluck+padpluck|Padded Square Pluck",
    "cv.pluck.0065|sqpluck+airpluck|Breath Square Pluck",
    "cv.pluck.0066|sqpluck+sparkle|Glint Square Pluck",
    "cv.pluck.0067|sqpluck+ksdouble|Paired Square Pluck",
    "cv.pluck.0068|sqpluck+dulchyb|Rustic Square Pluck",
    "cv.pluck.0069|sawpluck/natural/echo|Echo Saw Pluck",
    "cv.pluck.0070|sqpluck/natural/echo|Echo Square Pluck",
    "cv.pluck.0071|pulsepluck/natural/echo|Echo Reed Pluck",
    "cv.pluck.0072|kswire/natural/echo|Echo Wire Pluck",
    "cv.pluck.0073|fmwood/natural/echo|Echo Wood Pluck",
    "cv.pluck.0075|modalmar/natural/echo|Echo Mallet Pluck",
    "cv.pluck.0076|modalkal/natural/echo|Echo Thumb Pluck",
    "cv.pluck.0077|wtprism/natural/echo|Echo Prism Pluck",
    "cv.pluck.0078|wtdigi/natural/echo|Echo Digital Pluck",
    "cv.pluck.0079|syncpluck/natural/echo|Echo Sync Pluck",
    "cv.pluck.0080|organpluck/natural/echo|Echo Click Pluck",
    "cv.pluck.0081|breathpluck/natural/echo|Echo Breath Pluck",
    "cv.pluck.0082|voxpluck/natural/echo|Echo Vocal Pluck",
    "cv.pluck.0083|orchpizz/natural/echo|Echo Pizz Pluck",
    "cv.pluck.0084|ladderpluck/natural/echo|Echo Rubber Pluck",
    "cv.pluck.0085|bandpluck/natural/echo|Echo Tube Pluck",
    "cv.pluck.0086|fmmetal/natural/echo|Echo Metal Pluck",
    "cv.pluck.0087|fmglass+marhyb|Glass Pluck & Marimba",
    "cv.pluck.0089|fmwood+kalhyb|Thumb Wood Pluck",
    "cv.pluck.0090|fmtine+vibhyb|Tine Pluck & Vibes",
    "cv.pluck.0091|steelpan/natural/echo|Echo Steel Pan Pluck",
    "cv.pluck.0093|pulsepluck+guitarhyb|Reed Pluck & Nylon",
    "cv.pluck.0095|pulsepluck+subpluck|Sub Reed Pluck",
    "cv.pluck.0096|mbirapl/natural/echo|Echo Mbira Pluck",
    "cv.pluck.0097|pulsepluck+padpluck|Reed Pluck & Pad",
    "cv.pluck.0098|marimbapl/natural/echo|Echo Wood Bar Pluck",
    "cv.pluck.0099|quillpl/natural/echo|Echo Quill Pluck",
    "cv.pluck.0100|duopluck/natural/echo|Echo Duo Pluck",
    "cv.pluck.0101|chippluck/natural/echo|Echo 8-Bit Pluck",
    "cv.pluck.0102|pwmpluck/natural/echo|Echo Liquid Pluck",
    "cv.pluck.0103|pulsepluck+ksdouble|Twin String Reed Pluck",
    "cv.pluck.0104|organks/natural/echo|Echo Reed String Pluck",
    "cv.pluck.0105|brasspl/natural/echo|Echo Brass Pluck",
    "cv.pluck.0106|kswire+dulchyb|Folk Wire Pluck",
    "cv.pluck.0107|superpluck+pizzhyb|Hybrid Supersaw Pluck",
    "cv.pluck.0108|fmglass+harphyb|Harpstring Glass Pluck",
    "cv.pluck.0109|fmwood+marhyb|Rosewood Wood Pluck",
    "cv.pluck.0110|fmtine+kalhyb|Thumb Tine Pluck",
    "cv.pluck.0111|fmnylon+vibhyb|Vibe Harp Pluck",
    "cv.pluck.0112|superpluck+guitarhyb|Spanish Supersaw Pluck",
    "cv.pluck.0113|superpluck+airpluck|Misty Supersaw Pluck",
    "cv.pluck.0114|superpluck+ksdouble|Double Supersaw Pluck",
    "cv.pluck.0115|ksgut+ksmallet|Gamelan Gut Pluck",
    "cv.pluck.0116|sqpluck/natural/cloud|Cloud Square Pluck",
    "cv.pluck.0117|kswire/natural/cloud|Cloud Wire Pluck",
    "cv.pluck.0118|fmnylon/natural/cloud|Cloud Harp Pluck",
    "cv.pluck.0119|modalmar/natural/cloud|Cloud Mallet Pluck",
    "cv.pluck.0121|wtprism/natural/cloud|Cloud Prism Pluck",
    "cv.pluck.0122|wtdigi/natural/cloud|Cloud Digital Pluck",
    "cv.pluck.0123|syncpluck/natural/cloud|Cloud Sync Pluck",
    "cv.pluck.0124|breathpluck/natural/cloud|Cloud Breath Pluck",
    "cv.pluck.0125|voxpluck/natural/cloud|Cloud Vocal Pluck",
    "cv.pluck.0126|bandpluck/natural/cloud|Cloud Tube Pluck",
    "cv.pluck.0127|droplet/natural/cloud|Cloud Water Pluck",
    "cv.pluck.0128|fmmetal/natural/cloud|Cloud Metal Pluck",
    "cv.pluck.0129|steelpan/natural/cloud|Cloud Steel Pan Pluck",
    "cv.pluck.0130|mutedgtr/natural/cloud|Cloud Muted Pluck",
    "cv.pluck.0131|marimbapl/natural/cloud|Cloud Wood Bar Pluck",
    "cv.pluck.0132|quillpl/natural/cloud|Cloud Quill Pluck",
    "cv.pluck.0133|duopluck/natural/cloud|Cloud Duo Pluck",
    "cv.pluck.0134|chippluck/natural/cloud|Cloud 8-Bit Pluck",
    "cv.pluck.0135|kswire+pizzhyb|Bowstring Wire Pluck",
    "cv.pluck.0136|organks/natural/cloud|Harmonium Pluck",
    "cv.pluck.0137|fmtine+marhyb|Mallet Tine Pluck",
    "cv.pluck.0138|fmnylon+kalhyb|Tine Harp Pluck",
    "cv.pluck.0139|wtprism+vibhyb|Lounge Prism Pluck",
    "cv.pluck.0140|fmglass+guitarhyb|Guitar Glass Pluck",
    "cv.pluck.0141|kswire+subpluck|Weighted Wire Pluck",
    "cv.pluck.0142|kswire+padpluck|Padded Wire Pluck",
    "cv.pluck.0143|kswire+airpluck|Breath Wire Pluck",
    "cv.pluck.0144|fmglass+ksdouble|Paired Glass Pluck",
    "cv.pluck.0145|kszither+ksmallet|Chime Zither Pluck",
    "cv.pluck.0146|fmtine+harphyb|Tine Pluck & Harp",
    "cv.pluck.0147|fmnylon+marhyb|Harp Pluck & Marimba",
    "cv.pluck.0148|wtprism+kalhyb|Thumb Prism Pluck",
    "cv.pluck.0149|wtdigi+vibhyb|Digital Pluck & Vibes",
    "cv.pluck.0150|fmwood+guitarhyb|Wood Pluck & Nylon",
    "cv.pluck.0151|fmwood+ksdouble|Twin String Wood Pluck",
    "cv.pluck.0152|sawpluck/short/dry|Tight Saw Pluck",
    "cv.pluck.0153|sqpluck/short/dry|Staccato Square Pluck",
    "cv.pluck.0154|pulsepluck/short/dry|Short Reed Pluck",
    "cv.pluck.0155|kswire/short/dry|Tight Wire Pluck",
    "cv.pluck.0156|fmglass/short/dry|Clipped Glass Pluck",
    "cv.pluck.0157|fmwood/short/dry|Tight Wood Pluck",
    "cv.pluck.0158|fmtine/short/dry|Staccato Tine Pluck",
    "cv.pluck.0159|fmnylon/short/dry|Short Harp Pluck",
    "cv.pluck.0161|wtprism/short/dry|Staccato Prism Pluck",
    "cv.pluck.0162|wtdigi/short/dry|Short Digital Pluck",
    "cv.pluck.0163|wttri/short/dry|Clipped Soft Pluck",
    "cv.pluck.0164|syncpluck/short/dry|Tight Sync Pluck",
    "cv.pluck.0165|organpluck/short/dry|Staccato Click Pluck",
    "cv.pluck.0166|voxpluck/short/dry|Clipped Vocal Pluck",
    "cv.pluck.0167|orchpizz/short/dry|Tight Pizz Pluck",
    "cv.pluck.0168|ladderpluck/short/dry|Staccato Rubber Pluck",
    "cv.pluck.0169|bandpluck/short/dry|Short Tube Pluck",
    "cv.pluck.0170|fmmetal/short/dry|Tight Metal Pluck",
    "cv.pluck.0172|pizzpluck/short/dry|Short Pizzicato",
    "cv.pluck.0173|dulcpluck/short/dry|Clipped Dulcimer Pluck",
    "cv.pluck.0174|steelpan/short/dry|Close Steel Pan Pluck",
    "cv.pluck.0175|celestapl/short/dry|Short Celesta Pluck",
    "cv.pluck.0177|zitherpl/short/dry|Staccato River Zither",
    "cv.pluck.0180|duopluck/short/dry|Staccato Duo Pluck",
    "cv.pluck.0181|chippluck/short/dry|Short 8-Bit Pluck",
    "cv.pluck.0182|fmbellpl/short/dry|Clipped Bell Pluck",
    "cv.pluck.0183|pwmpluck/short/dry|Tight Liquid Pluck",
    "cv.pluck.0184|organks/short/dry|Close Harmonium Pluck",
    "cv.pluck.0185|brasspl/short/dry|Short Brass Pluck",
    "cv.pluck.0186|kszither+pizzhyb|Hybrid Zither Pluck",
    "cv.pluck.0187|wtprism+marhyb|Rosewood Prism Pluck",
    "cv.pluck.0188|wtdigi+kalhyb|Thumb Digital Pluck",
    "cv.pluck.0189|wttri+vibhyb|Vibe Soft Pluck",
    "cv.pluck.0190|fmtine+guitarhyb|Spanish Tine Pluck",
    "cv.pluck.0191|kszither+airpluck|Misty Zither Pluck",
    "cv.pluck.0192|fmtine+ksdouble|Double Tine Pluck",
    "cv.pluck.0193|fmwood+dulchyb|Hammered Wood Pluck",
    "cv.pluck.0194|bandpluck+ksmallet|Gamelan Tube Pluck",
    "cv.pluck.0195|fmglass+pizzhyb|Bowstring Glass Pluck",
    "cv.pluck.0196|modalmar+harphyb|Angelic Mallet Pluck",
    "cv.pluck.0197|wtdigi+marhyb|Mallet Digital Pluck",
    "cv.pluck.0198|wttri+kalhyb|Tine Soft Pluck",
    "cv.pluck.0199|syncpluck+vibhyb|Lounge Sync Pluck",
    "cv.pluck.0200|fmnylon+guitarhyb|Guitar Harp Pluck",
    "cv.pluck.0201|fmglass+subpluck|Weighted Glass Pluck",
    "cv.pluck.0202|fmglass+padpluck|Padded Glass Pluck",
    "cv.pluck.0203|fmglass+airpluck|Breath Glass Pluck",
    "cv.pluck.0204|fmglass+sparkle|Glint Glass Pluck",
    "cv.pluck.0205|fmtine+dulchyb|Rustic Tine Pluck",
    "cv.pluck.0206|pulsepluck/long/hall|Lingering Reed Pluck",
    "cv.pluck.0207|superpluck/long/hall|Singing Supersaw Pluck",
    "cv.pluck.0208|modalmar/long/hall|Singing Mallet Pluck",
    "cv.pluck.0209|bandpluck/long/hall|Lingering Tube Pluck",
    "cv.pluck.0210|chippluck/long/hall|Lingering 8-Bit Pluck",
    "cv.pluck.0211|brasspl/long/hall|Lingering Brass Pluck",
    "cv.pluck.0212|fmwood+pizzhyb|Pizz Wood Pluck",
    "cv.pluck.0213|modalkal+harphyb|Thumb Pluck & Harp",
    "cv.pluck.0214|wttri+marhyb|Soft Pluck & Marimba",
    "cv.pluck.0215|syncpluck+kalhyb|Thumb Sync Pluck",
    "cv.pluck.0216|droplet+vibhyb|Water Pluck & Vibes",
    "cv.pluck.0217|wtprism+guitarhyb|Prism Pluck & Nylon",
    "cv.pluck.0218|fmwood+subpluck|Sub Wood Pluck",
    "cv.pluck.0219|fmwood+padpluck|Wood Pluck & Pad",
    "cv.pluck.0220|fmwood+airpluck|Wood Pluck & Air",
    "cv.pluck.0221|fmwood+sparkle|Sparkle Wood Pluck",
    "cv.pluck.0222|wtprism+ksdouble|Twin String Pluck",
    "cv.pluck.0223|fmtine+pizzhyb|Hybrid Tine Pluck",
    "cv.pluck.0224|droplet+kalhyb|Thumb Water Pluck",
    "cv.pluck.0225|fmmetal+vibhyb|Vibe Metal Pluck",
    "cv.pluck.0226|wtdigi+guitarhyb|Spanish Digital Pluck",
    "cv.pluck.0227|fmtine+airpluck|Misty Tine Pluck",
    "cv.pluck.0228|fmtine+sparkle|Twinkle Tine Pluck",
    "cv.pluck.0229|sawpluck/muted/room|Muted Saw Pluck",
    "cv.pluck.0230|pulsepluck/muted/room|Velvet Reed Pluck",
    "cv.pluck.0231|ksgut/muted/room|Dusky Gut Pluck",
    "cv.pluck.0232|kszither/muted/room|Velvet Zither Pluck",
    "cv.pluck.0233|wtdigi/muted/room|Velvet Digital Pluck",
    "cv.pluck.0234|syncpluck/muted/room|Muted Sync Pluck",
    "cv.pluck.0235|voxpluck/muted/room|Hushed Vocal Pluck",
    "cv.pluck.0236|orchpizz/muted/room|Muted Pizz Pluck",
    "cv.pluck.0238|dulcpluck/muted/room|Hushed Dulcimer Pluck",
    "cv.pluck.0240|quillpl/muted/room|Muted Quill Pluck",
    "cv.pluck.0241|duopluck/muted/room|Dusky Duo Pluck",
    "cv.pluck.0242|chippluck/muted/room|Velvet 8-Bit Pluck",
    "cv.pluck.0243|pwmpluck/muted/room|Muted Liquid Pluck",
    "cv.pluck.0244|organks/muted/room|Room Reed String Pluck",
    "cv.pluck.0245|brasspl/muted/room|Velvet Brass Pluck",
    "cv.pluck.0246|fmnylon+pizzhyb|Bowstring Harp Pluck",
    "cv.pluck.0247|wtdigi+harphyb|Angelic Digital Pluck",
    "cv.pluck.0248|organpluck+marhyb|Mallet Click Pluck",
    "cv.pluck.0249|fmmetal+kalhyb|Tine Metal Pluck",
    "cv.pluck.0250|steelpan+vibhyb|Lounge Steel Pan Pluck",
    "cv.pluck.0251|wttri+guitarhyb|Guitar Soft Pluck",
    "cv.pluck.0252|fmnylon+subpluck|Weighted Harp Pluck",
    "cv.pluck.0254|fmnylon+airpluck|Breath Harp Pluck",
    "cv.pluck.0255|fmnylon+sparkle|Glint Harp Pluck",
    "cv.pluck.0256|wtprism+pizzhyb|Pizz Prism Pluck",
    "cv.pluck.0257|ladderpluck+marhyb|Rubber Pluck & Marimba",
    "cv.pluck.0258|steelpan+kalhyb|Thumb Steel Pan Pluck",
    "cv.pluck.0259|superpluck/echo/echo|Echo Supersaw Pluck",
    "cv.pluck.0260|mutedgtr+vibhyb|Muted Pluck & Vibes",
    "cv.pluck.0261|wtprism+subpluck|Sub Prism Pluck",
    "cv.pluck.0262|modalmar+padpluck|Mallet Pluck & Pad",
    "cv.pluck.0263|modalmar+airpluck|Mallet Pluck & Air",
    "cv.pluck.0264|modalmar+sparkle|Sparkle Mallet Pluck",
    "cv.pluck.0265|wtdigi+pizzhyb|Hybrid Digital Pluck",
    "cv.pluck.0266|syncpluck+harphyb|Harpstring Sync Pluck",
    "cv.pluck.0267|bandpluck+marhyb|Rosewood Tube Pluck",
    "cv.pluck.0268|duopluck+kalhyb|Thumb Duo Pluck",
    "cv.pluck.0269|harmonicpl+vibhyb|Vibe Harmonic Pluck",
    "cv.pluck.0270|wtdigi+subpluck|Low End Digital Pluck",
    "cv.pluck.0271|modalkal+padpluck|Cushioned Thumb Pluck",
    "cv.pluck.0272|modalkal+airpluck|Misty Thumb Pluck",
    "cv.pluck.0273|modalkal+sparkle|Twinkle Thumb Pluck",
    "cv.pluck.0274|ladderpluck+ksdouble|Double Rubber Pluck",
    "cv.pluck.0275|bandpluck+dulchyb|Hammered Tube Pluck",
    "cv.pluck.0276|kswire/lofi/room|Lo-Fi Wire Pluck",
    "cv.pluck.0277|wttri+pizzhyb|Bowstring Soft Pluck",
    "cv.pluck.0278|kszither/lofi/room|Crumbled Zither Pluck",
    "cv.pluck.0279|fmglass/lofi/room|Bedroom Glass Pluck",
    "cv.pluck.0280|fmmetal+marhyb|Mallet Metal Pluck",
    "cv.pluck.0282|chippluck+kalhyb|Tine 8-Bit Pluck",
    "cv.pluck.0283|modalmar/lofi/room|Bedroom Mallet Pluck",
    "cv.pluck.0284|modalkal/lofi/room|Lo-Fi Thumb Pluck",
    "cv.pluck.0285|fmbellpl+vibhyb|Lounge Bell Pluck",
    "cv.pluck.0286|wtprism/lofi/room|Dusty Prism Pluck",
    "cv.pluck.0287|bandpluck+guitarhyb|Guitar Tube Pluck",
    "cv.pluck.0288|wttri+subpluck|Weighted Soft Pluck",
    "cv.pluck.0289|wtprism+padpluck|Padded Prism Pluck",
    "cv.pluck.0290|fmmetal+dulchyb|Rustic Metal Pluck",
    "cv.pluck.0292|quillpl/lofi/room|Lo-Fi Quill Pluck",
    "cv.pluck.0293|fmmetal+harphyb|Metal Pluck & Harp",
    "cv.pluck.0294|steelpan+marhyb|Island Pluck & Marimba",
    "cv.pluck.0296|wtdigi+padpluck|Digital Pluck & Pad",
    "cv.pluck.0297|wtdigi+airpluck|Digital Pluck & Air",
    "cv.pluck.0298|fmmetal+ksdouble|Twin String Tin Pluck",
    "cv.pluck.0299|sawpluck/wide/hall|Wide Saw Pluck",
    "cv.pluck.0300|pulsepluck/wide/hall|Liquid Reed Pluck",
    "cv.synth.0001|polysaw/natural/room|Poly Saw",
    "cv.synth.0002|sqstab/natural/room|Square Stab",
    "cv.synth.0003|superstab/natural/room|Rave Stab",
    "cv.synth.0004|unikeys/natural/room|Synth Keys",
    "cv.synth.0005|fmkeys/natural/room|FM Poly",
    "cv.synth.0006|wtpoly/natural/room|Wavetable Poly",
    "cv.synth.0007|voxstab/natural/room|Vocal Stab",
    "cv.synth.0008|syncpoly/natural/room|Sync Poly",
    "cv.synth.0009|fatpoly/natural/room|Fat Poly",
    "cv.synth.0010|hollowpoly/natural/room|Hollow Poly",
    "cv.synth.0011|sinekeys/natural/room|Sine Keys",
    "cv.synth.0012|gated/natural/room|Gated Chords",
    "cv.synth.0013|pump/natural/room|Pumping Synth",
    "cv.synth.0014|seq5/natural/room|Fifth Sequence",
    "cv.synth.0015|seqoct/natural/room|Octave Bounce",
    "cv.synth.0018|fmbrass/natural/room|FM Brass",
    "cv.synth.0019|chorussq/natural/room|Chorus Poly",
    "cv.synth.0020|glasspoly/natural/room|Glass Keys Poly",
    "cv.synth.0021|organsaw/natural/room|Organ Synth",
    "cv.synth.0022|stackfm/natural/room|Stack FM Poly",
    "cv.synth.0023|pulsestr/natural/room|Pulse Strings",
    "cv.synth.0024|reedsynth/natural/room|Reed Synth",
    "cv.synth.0025|polysaw+pianohyb|Piano Poly Saw",
    "cv.synth.0026|polysaw+organhyb|Church Poly Saw",
    "cv.synth.0027|polysaw+subsy|Sub Poly Saw",
    "cv.synth.0028|superstab/natural/hall|Hall Rave Stab",
    "cv.synth.0030|reedsynth/natural/hall|Hall Reed Synth",
    "cv.synth.0031|brass+orchhitl|Stinger Synth Brass",
    "cv.synth.0032|brass+harpsihyb|Harpsi Synth Brass",
    "cv.synth.0033|sqstab+pianohyb|Hybrid Square Stab",
    "cv.synth.0034|sqstab+spicstab|Spiccato Square Stab",
    "cv.synth.0035|sqstab+harpsihyb|Baroque Square Stab",
    "cv.synth.0036|sqstab+organhyb|Gospel Square Stab",
    "cv.synth.0037|superstab+eplayer|Rave Stab & EP",
    "cv.synth.0038|superstab/natural/echo|Echo Rave Stab",
    "cv.synth.0039|pwmpoly/natural/echo|PWM Poly",
    "cv.synth.0040|chippoly/natural/echo|Chip Poly",
    "cv.synth.0041|superstab+pianohyb|Piano Rave Stab",
    "cv.synth.0042|unikeys+hornstab|Horn Stab Synth Keys",
    "cv.synth.0043|unikeys+harpsihyb|Courtly Synth Keys",
    "cv.synth.0044|superstab+organhyb|Church Rave Stab",
    "cv.synth.0045|superstab+padsy|Padded Rave Stab",
    "cv.synth.0046|superstab+subsy|Sub Rave Stab",
    "cv.synth.0047|unikeys+pianohyb|Grand Synth Keys",
    "cv.synth.0048|orchhit+orchhitl|Stinger Hit Synth",
    "cv.synth.0049|fatpoly+harpsihyb|Harpsi Fat Poly",
    "cv.synth.0050|unikeys+organhyb|Revival Synth Keys",
    "cv.synth.0051|unikeys+subsy|Weighted Synth Keys",
    "cv.synth.0052|houseorgan+pluckon|Transient House Organ",
    "cv.synth.0053|orchhit/natural/cloud|Hit Synth",
    "cv.synth.0054|seq5/natural/cloud|Cloud Fifth Sequence",
    "cv.synth.0055|seqoct/natural/cloud|Cloud Octave Bounce",
    "cv.synth.0057|fmkeys+pianohyb|Hybrid FM Poly",
    "cv.synth.0058|fatpoly+orchhitl|Orchestra Hit Fat Poly",
    "cv.synth.0059|pwmpoly+harpsihyb|Baroque PWM Poly",
    "cv.synth.0060|fatpoly+organhyb|Gospel Fat Poly",
    "cv.synth.0062|fmkeys+subsy|Low FM Poly",
    "cv.synth.0063|sqstab/short/dry|Staccato Square Stab",
    "cv.synth.0064|superstab/short/dry|Short Rave Stab",
    "cv.synth.0065|unikeys/short/dry|Clipped Synth Keys",
    "cv.synth.0066|wtpoly+pianohyb|Piano Wavetable Poly",
    "cv.synth.0067|hollowpoly+harpsihyb|Courtly Hollow Poly",
    "cv.synth.0068|hollowpoly+organhyb|Church Hollow Poly",
    "cv.synth.0069|clavsynth/short/dry|Tight Funk Synth",
    "cv.synth.0070|houseorgan+subsy|Sub House Organ",
    "cv.synth.0071|hollowpoly+hornstab|Big Band Hollow Poly",
    "cv.synth.0072|hollowpoly+orchhitl|Stinger Hollow Poly",
    "cv.synth.0073|wtpoly+subsy|Weighted Poly",
    "cv.synth.0074|sqstab/long/hall|Long Square Stab",
    "cv.synth.0075|orchhit/long/hall|Ringing Hit Synth",
    "cv.synth.0077|fatpoly+pianohyb|Hybrid Fat Poly",
    "cv.synth.0078|voxstab+padsy|Vocal Stab & Pad",
    "cv.synth.0079|voxstab+subsy|Low Vocal Stab",
    "cv.synth.0080|seq5+hornstab|Horn Stab Sequence",
    "cv.synth.0081|seq5+harpsihyb|Courtly Fifth Sequence",
    "cv.synth.0082|seq5+organhyb|Church Fifth Sequence",
    "cv.synth.0083|hollowpoly+eplayer|Electric Hollow Poly",
    "cv.synth.0084|superstab/muted/room|Velvet Rave Stab",
    "cv.synth.0085|wtpoly/muted/room|Velvet Wavetable Poly",
    "cv.synth.0086|voxstab/muted/room|Hushed Vocal Stab",
    "cv.synth.0087|syncpoly/muted/room|Dusky Sync Poly",
    "cv.synth.0088|fatpoly/muted/room|Velvet Fat Poly",
    "cv.synth.0089|gated/muted/room|Hushed Gated Chords",
    "cv.synth.0091|seq5/muted/room|Dusky Fifth Sequence",
    "cv.synth.0092|chorussq/muted/room|Velvet Chorus Poly",
    "cv.synth.0093|chippoly/muted/room|Hushed Chip Poly",
    "cv.synth.0094|organsaw/muted/room|Velvet Organ Synth",
    "cv.synth.0095|pulsestr/muted/room|Dusky Pulse Strings",
    "cv.synth.0096|seqoct+harpsihyb|Harpsi Octave Bounce",
    "cv.synth.0097|seqoct+organhyb|Revival Octave Bounce",
    "cv.synth.0098|syncpoly+subsy|Weighted Sync Poly",
    "cv.synth.0099|chorussq+harpsihyb|Baroque Chorus Poly",
    "cv.synth.0100|seq5+eplayer|Fifth Sequence & EP",
    "cv.synth.0101|seq5+pluckon|Plucked Fifth Sequence",
    "cv.synth.0102|fatpoly/echo/echo|Ricochet Fat Poly",
    "cv.synth.0103|detkeys/echo/echo|Repeating Detuned Keys",
    "cv.synth.0104|gated+pianohyb|Piano Gated Chords",
    "cv.synth.0105|seqoct+spicstab|Bouncing Bow Bounce",
    "cv.synth.0106|chippoly+harpsihyb|Courtly Chip Poly",
    "cv.synth.0107|chippoly+organhyb|Church Chip Poly",
    "cv.synth.0108|polystr+subsy|Sub Poly Strings",
    "cv.synth.0109|chorussq+spicstab|String Hit Chorus Poly",
    "cv.synth.0110|sinekeys/lofi/room|Crumbled Sine Keys",
    "cv.synth.0111|seq5/lofi/room|Dusty Fifth Sequence",
    "cv.synth.0112|chorussq/lofi/room|Crumbled Chorus Poly",
    "cv.synth.0113|chippoly+spicstab|Spiccato Chip Poly",
    "cv.synth.0114|organsaw+organhyb|Gospel Organ Synth",
    "cv.synth.0115|hollowpoly+subsy|Low Hollow Poly",
    "cv.synth.0116|detkeys+harpsihyb|Courtly Detuned Keys",
    "cv.synth.0117|detkeys+organhyb|Church Detuned Keys",
    "cv.synth.0118|detkeys+eplayer|Electric Detuned Keys",
    "cv.synth.0120|sinekeys+subsy|Sub Sine Keys",
    "cv.synth.0121|seqoct/wide/hall|Liquid Octave Bounce",
    "cv.synth.0122|fmbrass+pianohyb|Grand FM Brass",
    "cv.synth.0123|chippoly/wide/hall|Panoramic Chip Poly",
    "cv.synth.0124|pulsestr+harpsihyb|Harpsi Pulse Strings",
    "cv.synth.0125|chorussq+pianohyb|Hybrid Chorus Poly",
    "cv.synth.0126|reedsynth+harpsihyb|Baroque Reed Synth",
    "cv.synth.0127|pump+subsy|Low Pumping Synth",
    "cv.synth.0129|seq5+padsy|Padded Fifth Sequence",
    "cv.synth.0130|seq5+subsy|Sub Fifth Sequence",
    "cv.synth.0131|polysaw/octbody/room|Heavy Poly Saw",
    "cv.synth.0132|sqstab/octbody/room|Doubled Square Stab",
    "cv.synth.0133|superstab/octbody/room|Thick Rave Stab",
    "cv.synth.0134|unikeys/octbody/room|Heavy Synth Keys",
    "cv.synth.0135|fmkeys/octbody/room|Deep FM Poly",
    "cv.synth.0136|wtpoly/octbody/room|Thick Wavetable Poly",
    "cv.synth.0137|voxstab/octbody/room|Heavy Vocal Stab",
    "cv.synth.0138|syncpoly/octbody/room|Doubled Sync Poly",
    "cv.synth.0140|polystr/octbody/room|Heavy Poly Strings",
    "cv.synth.0141|pwmpoly/octbody/room|Deep PWM Poly",
    "cv.synth.0142|hollowpoly/octbody/room|Doubled Hollow Poly",
    "cv.synth.0143|sinekeys/octbody/room|Thick Sine Keys",
    "cv.synth.0144|gated/octbody/room|Heavy Gated Chords",
    "cv.synth.0145|pump/octbody/room|Deep Pumping Synth",
    "cv.synth.0146|seqoct/octbody/room|Thick Octave Bounce",
    "cv.synth.0148|fmbrass/octbody/room|Doubled FM Brass",
    "cv.synth.0149|chorussq/octbody/room|Thick Chorus Poly",
    "cv.synth.0150|glasspoly/octbody/room|Deep Glass Keys Poly",
    "cv.synth.0151|stackfm/octbody/room|Deep Stack FM Poly",
    "cv.synth.0152|pulsestr/octbody/room|Doubled Pulse Strings",
    "cv.synth.0153|reedsynth/octbody/room|Thick Reed Synth",
    "cv.synth.0154|glasspoly+pianohyb|Grand Glass Keys Poly",
    "cv.synth.0155|seqoct+padsy|Wash Octave Bounce",
    "cv.synth.0156|fmbrass+vibhyb|FM Brass & Vibes",
    "cv.synth.0158|glasspoly+vibhyb|Vibe Glass Keys Poly",
    "cv.synth.0159|brass/fifth/room|Fifth Synth Brass",
    "cv.synth.0160|sqstab/fifth/room|Power Square Stab",
    "cv.synth.0161|unikeys/fifth/room|Quintal Synth Keys",
    "cv.synth.0162|fmkeys/fifth/room|Fifth FM Poly",
    "cv.synth.0163|wtpoly/fifth/room|Open Wavetable Poly",
    "cv.synth.0164|voxstab/fifth/room|Quintal Vocal Stab",
    "cv.synth.0165|orchhit/fifth/room|Fifth Hit Synth",
    "cv.synth.0166|fatpoly/fifth/room|Open Fat Poly",
    "cv.synth.0167|sinekeys/fifth/room|Open Sine Keys",
    "cv.synth.0169|fmbrass/fifth/room|Power FM Brass",
    "cv.synth.0170|chorussq/fifth/room|Open Chorus Poly",
    "cv.synth.0171|chippoly/fifth/room|Quintal Chip Poly",
    "cv.synth.0172|glasspoly/fifth/room|Fifth Glass Keys Poly",
    "cv.synth.0173|stackfm/fifth/room|Fifth Stack FM Poly",
    "cv.synth.0174|pulsestr/fifth/room|Power Pulse Strings",
    "cv.synth.0175|reedsynth/fifth/room|Open Reed Synth",
    "cv.synth.0176|clavsynth+padsy|Padded Funk Synth",
    "cv.synth.0177|chorussq+subsy|Sub Chorus Poly",
    "cv.synth.0178|stackfm+vibhyb|Lounge Stack FM Poly",
    "cv.synth.0179|fmbrass+padsy|Wash FM Brass",
    "cv.synth.0180|chippoly+subsy|Weighted Chip Poly",
    "cv.synth.0181|chorussq+padsy|Chorus Poly & Pad",
    "cv.synth.0182|glasspoly+subsy|Low Glass Keys Poly",
    "cv.synth.0183|houseorgan+marhyb|Rosewood House Organ",
    "cv.synth.0184|fmkeys/octup/hall|Shining FM Poly",
    "cv.synth.0185|wtpoly/octup/hall|Hall Wavetable Poly",
    "cv.synth.0186|sinekeys/octup/hall|Sparkling Sine Keys",
    "cv.synth.0187|fmbrass/octup/hall|Overtone FM Brass",
    "cv.synth.0188|chorussq/octup/hall|Sparkling Chorus Poly",
    "cv.synth.0189|glasspoly/octup/hall|Hall Glass Keys Poly",
    "cv.synth.0190|pulsestr+pianohyb|Piano Pulse Strings",
    "cv.synth.0191|wtpoly+marhyb|Mallet Wavetable Poly",
    "cv.synth.0192|reedsynth+pianohyb|Grand Reed Synth",
    "cv.synth.0193|organsaw+subsy|Weighted Organ Synth",
    "cv.synth.0194|syncpoly+marhyb|Sync Poly & Marimba",
    "cv.synth.0195|detkeys+subsy|Low Detuned Keys",
    "cv.synth.0196|sqstab+pizzhyb|Hybrid Stab",
    "cv.synth.0197|superstab/reverse/hall|Rewind Rave Stab",
    "cv.synth.0198|unikeys/reverse/hall|Mirror Synth Keys",
    "cv.synth.0199|orchhit/reverse/hall|Reversed Hit Synth",
    "cv.synth.0200|seqoct/reverse/hall|Rewind Octave Bounce",
    "cv.synth.0201|chippoly/reverse/hall|Mirror Chip Poly",
    "cv.synth.0202|glasspoly/reverse/hall|Glass Keys Poly Hall",
    "cv.synth.0203|stackfm/reverse/hall|Reversed Stack FM Poly",
    "cv.synth.0204|reedsynth/reverse/hall|Rewind Reed Synth",
    "cv.synth.0205|pulsestr+subsy|Weighted Pulse Strings",
    "cv.synth.0206|reedsynth+subsy|Low Reed Synth",
    "cv.synth.0207|wtpoly+pizzhyb|Bowstring Poly",
    "cv.synth.0208|pump+marhyb|Mallet Pumping Synth",
    "cv.synth.0209|seq5+marhyb|Fifth Bounce & Marimba",
    "cv.synth.0210|seqoct/drive/room|Burning Octave Bounce",
    "cv.synth.0211|seqoct+marhyb|Rosewood Octave Bounce",
    "cv.synth.0212|pulsestr/drive/room|Driven Pulse Strings",
    "cv.synth.0213|fmbrass+marhyb|Mallet FM Brass",
    "cv.synth.0214|chorussq+marhyb|Chorus Poly & Marimba",
    "cv.synth.0215|sinekeys+pizzhyb|Hybrid Sine Keys",
    "cv.synth.0216|fmkeys+sparkle|Twinkle FM Poly",
    "cv.synth.0217|pump+pizzhyb|Pizz Pumping Synth",
    "cv.synth.0218|brass/bright/room|Bright Synth Brass",
    "cv.synth.0219|fatpoly/bright/room|Clear Fat Poly",
    "cv.synth.0220|polystr/bright/room|Crisp Poly Strings",
    "cv.synth.0221|gated/bright/room|Crisp Gated Chords",
    "cv.synth.0222|pump/bright/room|Bright Pumping Synth",
    "cv.synth.0223|chippoly/bright/room|Crisp Chip Poly",
    "cv.synth.0224|sinekeys+sparkle|Glint Sine Keys",
    "cv.synth.0225|pulsestr/bright/room|Open Pulse Strings",
    "cv.synth.0226|fmbrass+pizzhyb|Pizz FM Brass",
    "cv.synth.0227|stackfm+marhyb|Tower Keys & Marimba",
    "cv.synth.0228|chorussq+pizzhyb|Hybrid Poly",
    "cv.synth.0229|polysaw/stab/room|Snapped Poly Saw",
    "cv.synth.0230|brass/stab/room|Stabbing Synth Brass",
    "cv.synth.0231|sawchoir+pizzhyb|Hybrid Choir Synth",
    "cv.synth.0232|unikeys/stab/room|Snapped Synth Keys",
    "cv.synth.0233|fmkeys/stab/room|Stabbing FM Poly",
    "cv.synth.0234|wtpoly/stab/room|Struck Wavetable Poly",
    "cv.synth.0235|voxstab/stab/room|Snapped Vocal Stab",
    "cv.synth.0236|syncpoly/stab/room|Punchy Sync Poly",
    "cv.synth.0237|fatpoly/stab/room|Struck Fat Poly",
    "cv.synth.0238|pwmpoly/stab/room|Stabbing PWM Poly",
    "cv.synth.0239|hollowpoly/stab/room|Punchy Hollow Poly",
    "cv.synth.0240|sinekeys/stab/room|Struck Sine Keys",
    "cv.synth.0241|pump/stab/room|Stabbing Pumping Synth",
    "cv.synth.0242|seq5/stab/room|Punchy Fifth Sequence",
    "cv.synth.0243|seqoct/stab/room|Struck Octave Bounce",
    "cv.synth.0244|fmbrass/stab/room|Punchy FM Brass",
    "cv.synth.0245|glasspoly/stab/room|Room Glass Keys Poly",
    "cv.synth.0246|stackfm+pizzhyb|Hybrid Stack FM Poly",
    "cv.synth.0247|detkeys/stab/room|Snapped Detuned Keys",
    "cv.synth.0248|stackfm/stab/room|Stabbing Stack FM Poly",
    "cv.synth.0249|pulsestr/stab/room|Punchy Pulse Strings",
    "cv.synth.0250|reedsynth/stab/room|Struck Reed Synth",
    "cv.synth.0251|pulsestr+pizzhyb|Bowstring Strings",
    "cv.synth.0252|sqstab/swell/hall|Breathy Square Stab",
    "cv.synth.0253|superstab/swell/hall|Rising Rave Stab",
    "cv.synth.0254|unikeys/swell/hall|Bowed Synth Keys",
    "cv.synth.0255|voxstab/swell/hall|Bowed Vocal Stab",
    "cv.synth.0256|pwmpoly/swell/hall|Swelling PWM Poly",
    "cv.synth.0257|hollowpoly/swell/hall|Breathy Hollow Poly",
    "cv.synth.0258|seqoct/swell/hall|Rising Octave Bounce",
    "cv.synth.0259|chippoly/swell/hall|Bowed Chip Poly",
    "cv.synth.0260|sinekeys+airpluck|Breath Sine Keys",
    "cv.synth.0261|reedsynth/swell/hall|Rising Reed Synth",
    "cv.synth.0262|sqstab/hollow/room|Nasal Square Stab",
    "cv.synth.0263|superstab/hollow/room|Honky Rave Stab",
    "cv.synth.0265|fatpoly/hollow/room|Honky Fat Poly",
    "cv.synth.0266|gated/hollow/room|Pinched Gated Chords",
    "cv.synth.0267|seq5/hollow/room|Nasal Fifth Sequence",
    "cv.synth.0268|seqoct/hollow/room|Honky Octave Bounce",
    "cv.synth.0269|chorussq/hollow/room|Honky Chorus Poly",
    "cv.synth.0270|chippoly/hollow/room|Pinched Chip Poly",
};

// the rhythm presets (new ids after the shipped ones): base sound (recipe tag), name, pattern (v3::designSound)
struct Pattern { const char* recipe; const char* name; int pattern; };
const Pattern patterns[] = {
    { "sawpluck/natural/room", "Saw Pluck Arpeggio", 1 },          // Up 1/16, 2 octaves
    { "fmglass/natural/room", "Glass Pluck Arpeggio", 2 },         // Up-Down 1/16
    { "wttri/natural/room", "Soft Pluck Arpeggio", 3 },            // Down 1/8
    { "polysaw/natural/room", "Poly Saw Chord Pulse", 4 },         // Chord 1/8 with accents
    { "warm/natural/hall", "Gated Warm Pad", 5 },                  // gate 1/16
    { "strmachine/natural/hall", "Gated String Machine", 6 },      // gate 1/16, another pattern
};

v3::Role roleOf (const juce::String& recipeTag, const std::map<juce::String, std::pair<int, uint32_t>>& voiceInfo, const std::map<juce::String, juce::String>& layerSpace)
{
    v3::Role r;
    if (recipeTag.contains ("+"))
    {
        r.voice = recipeTag.upToFirstOccurrenceOf ("+", false, false);
        r.layer = recipeTag.fromFirstOccurrenceOf ("+", false, false);
        r.treat = "natural";
        r.space = layerSpace.count (r.layer) ? layerSpace.at (r.layer) : juce::String ("room");
    }
    else
    {
        const auto t = juce::StringArray::fromTokens (recipeTag, "/", "");
        r.voice = t[0]; r.treat = t[1]; r.space = t[2];
    }
    if (auto it = voiceInfo.find (r.voice); it != voiceInfo.end()) { r.cat = it->second.first; r.kind = it->second.second; }
    r.sample = (r.kind & kSamples) != 0;
    r.model = (r.kind & kModel) != 0;
    r.fam = v3::family (r.voice, r.cat, r.kind);
    return r;
}

// the v2 stage of one candidate: calibration + the gate's repairs (exactly what the v2 search did with it)
void v2Make (Done& d)
{
    const float ownMax25 = 0.08f, ownMax6k = 0.02f, ownMaxVel = 20.0f;
    auto& p = d.cand.preset;
    calibrate (p);
    if (juce::String why; ! vault::validatePreset (p, &why)) { d.res.fail = "invalid: " + why; return; }
    for (int attempt = 0; attempt < 6; ++attempt)
    {
        d.res = recipegate::Result();
        d.res.preset = p;
        recipegate::analyse (d.res);
        if (d.res.fail.isEmpty() && d.res.share25 > ownMax25) d.res.fail = "harsh-2-5k";
        if (d.res.fail.isEmpty() && d.res.share6k > ownMax6k) d.res.fail = "piercing-highs";
        if (d.res.fail.isEmpty() && d.res.velRange > ownMaxVel) d.res.fail = "velocity-wide";
        if (d.res.fail.isEmpty() || d.res.fail.startsWith ("name") || d.res.fail == "label") break;
        if (! repair (p, d.res.fail, attempt)) break;
        calibrate (p);
        ++d.repairs;
    }
}
} // namespace

std::vector<VaultPreset> build_synth (const RecipeContext& ctx)
{
    if (auto pm = juce::SystemStats::getEnvironmentVariable ("SYNTH_PINMAP", {}); pm.isNotEmpty()) { pinMap (ctx, juce::File (pm)); return {}; }
    if (envInt ("SYNTH_V2_SEARCH", 0) != 0) return buildV2Search (ctx);
    const int threads = envInt ("SYNTH_THREADS", 3);
    auto parallel = [threads] (int count, const char* what, const std::function<void (int)>& fn)
    {
        std::atomic<int> next { 0 }, done { 0 };
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t)
            pool.emplace_back ([&] { for (int i; (i = next++) < count;) { fn (i); if (const int d = ++done; d % 200 == 0) std::printf ("  synth %s %d / %d\n", what, d, count); } });
        for (auto& t : pool) t.join();
    };

    // the candidates by recipe tag, the voices' category / kind, the layer recipes' spaces
    std::map<juce::String, Cand> byTag;
    for (auto& c : candidates (ctx)) byTag.emplace (c.recipe, c);
    std::map<juce::String, std::pair<int, uint32_t>> voiceInfo;
    for (auto& v : voices()) voiceInfo[v.tag] = { v.category, v.kind };
    std::map<juce::String, juce::String> layerSpace;
    for (auto& l : layers()) layerSpace[l.tag] = l.spaceTag;

    // 1. the v2 stage: the pinned presets + the bases of the rhythm presets, rebuilt (SYNTH_V2_CACHE=<dir> keeps it)
    struct Entry { juce::String id, recipe, name; int pattern = 0; };
    std::vector<Entry> entries;
    std::map<int, int> maxId;
    for (auto* row : pinnedV2)
    {
        const auto c = juce::StringArray::fromTokens (row, "|", "");
        entries.push_back ({ c[0], c[1], c[2], 0 });
        maxId[voiceInfo[juce::StringArray::fromTokens (c[1], "/+", "")[0]].first] = std::max (maxId[voiceInfo[juce::StringArray::fromTokens (c[1], "/+", "")[0]].first], c[0].fromLastOccurrenceOf (".", false, false).getIntValue());
    }
    for (auto& pt : patterns)
    {
        const int cat = voiceInfo[juce::StringArray::fromTokens (pt.recipe, "/+", "")[0]].first;
        entries.push_back ({ juce::String ("cv.") + vault::categorySlug (cat) + "." + juce::String (++maxId[cat]).paddedLeft ('0', 4), pt.recipe, pt.name, pt.pattern });
    }
    if (const auto only = juce::StringArray::fromTokens (juce::SystemStats::getEnvironmentVariable ("SYNTH_ONLY", {}), ",", ""); ! only.isEmpty())
        entries.erase (std::remove_if (entries.begin(), entries.end(), [&] (const Entry& e) { return ! only.contains (e.id); }), entries.end());

    std::vector<VaultPreset> base (entries.size());
    std::vector<char> ok (entries.size(), 0);
    juce::File cacheBank;
    if (auto dir = juce::SystemStats::getEnvironmentVariable ("SYNTH_V2_CACHE", {}); dir.isNotEmpty()) { juce::File (dir).createDirectory(); cacheBank = juce::File (dir).getChildFile ("synth_v2.cvb"); }
    std::map<juce::String, VaultPreset> cached;
    if (cacheBank.existsAsFile())
    {
        juce::MemoryBlock mb;
        std::vector<VaultPreset> v;
        if (cacheBank.loadFileAsData (mb) && vault::deserializeBank (mb.getData(), mb.getSize(), v, nullptr, nullptr, true)) for (auto& p : v) cached[p.id] = p;
        std::printf ("synth: v2 stage cache: %d presets\n", (int) cached.size());
    }
    std::atomic<int> missing { 0 }, rebuilt { 0 };
    parallel ((int) entries.size(), "v2", [&] (int i)
    {
        const auto& e = entries[(size_t) i];
        if (auto it = cached.find (e.id); it != cached.end() && it->second.name == e.name) { base[(size_t) i] = it->second; ok[(size_t) i] = 1; return; }
        ++rebuilt;
        auto ct = byTag.find (e.recipe);
        if (ct == byTag.end()) { ++missing; std::printf ("  synth: no candidate %s\n", e.recipe.toRawUTF8()); return; }
        Done d;
        d.cand = ct->second;
        d.cand.preset.id = e.id; d.cand.preset.name = e.name;
        v2Make (d);
        if (d.res.fail.isNotEmpty() && ! d.res.fail.startsWith ("name")) { std::printf ("  synth v2 %s %s: %s\n", e.id.toRawUTF8(), e.recipe.toRawUTF8(), d.res.fail.toRawUTF8()); return; }
        auto p = d.cand.preset;
        p.id = e.id; p.name = e.name; p.gain = 1.0f;
        base[(size_t) i] = p;
        ok[(size_t) i] = 1;
    });
    if (cacheBank != juce::File() && rebuilt > 0)
    {
        std::vector<VaultPreset> v;
        for (size_t i = 0; i < base.size(); ++i) if (ok[i]) v.push_back (base[i]);
        vault::saveBankFile (cacheBank, v);   // (written as v3: the cache reads them back converted - v2 keys stay in part 0 only for v2 presets)
    }
    // SYNTH_VERIFY=<v2 bank>: the rebuilt v2 sounds against the shipped ones (parts identical?)
    if (auto vf = juce::SystemStats::getEnvironmentVariable ("SYNTH_VERIFY", {}); vf.isNotEmpty())
    {
        juce::MemoryBlock mb;
        std::vector<VaultPreset> v;
        std::map<juce::String, VaultPreset> ship;
        if (juce::File (vf).loadFileAsData (mb) && vault::deserializeBank (mb.getData(), mb.getSize(), v, nullptr, nullptr, true)) for (auto& p : v) ship[p.id] = p;
        int same = 0, diff = 0;
        for (size_t i = 0; i < base.size(); ++i)
        {
            if (! ok[i] || ! ship.count (base[i].id)) continue;
            const auto& s = ship[base[i].id];
            bool eq = s.parts.size() == base[i].parts.size() && s.name.replace (" and ", " & ") == base[i].name.replace (" and ", " & ");
            for (size_t k = 0; eq && k < s.parts.size(); ++k)
            {
                auto a = s.parts[k], b = base[i].parts[k];
                for (auto* pp : { &a, &b }) pp->values.erase (std::remove_if (pp->values.begin(), pp->values.end(), [] (auto& kv) { return kv.first == "gain"; }), pp->values.end());
                std::sort (a.values.begin(), a.values.end()); std::sort (b.values.begin(), b.values.end());
                eq = a == b;
            }
            if (eq) ++same;
            else
            {
                ++diff;
                std::printf ("  verify: %s %s (%s) differs:", base[i].id.toRawUTF8(), base[i].name.toRawUTF8(), entries[i].recipe.toRawUTF8());
                for (size_t k = 0; k < std::max (s.parts.size(), base[i].parts.size()); ++k)
                {
                    if (k >= s.parts.size() || k >= base[i].parts.size()) { std::printf (" [part count %d vs %d]", (int) s.parts.size(), (int) base[i].parts.size()); break; }
                    std::set<juce::String> keys;
                    for (auto& kv : s.parts[k].values) keys.insert (kv.first);
                    for (auto& kv : base[i].parts[k].values) keys.insert (kv.first);
                    for (auto& key : keys)
                        if (key != "gain" && s.parts[k].get (key, -999.0f) != base[i].parts[k].get (key, -999.0f))
                            std::printf (" p%d.%s %g/%g", (int) k, key.toRawUTF8(), s.parts[k].get (key, -999.0f), base[i].parts[k].get (key, -999.0f));
                    if (s.parts[k].label != base[i].parts[k].label) std::printf (" p%d label %s/%s", (int) k, s.parts[k].label.toRawUTF8(), base[i].parts[k].label.toRawUTF8());
                }
                std::printf ("\n");
            }
        }
        std::printf ("synth verify: %d identical, %d different\n", same, diff);
    }

    std::vector<v3::Role> roles (entries.size());
    for (size_t i = 0; i < entries.size(); ++i) { roles[i] = roleOf (entries[i].recipe, voiceInfo, layerSpace); roles[i].pattern = entries[i].pattern; }
    if (envInt ("SYNTH_V2ONLY", 0) != 0) return {};   // (the v2 stage alone: cache / verify)

    // 2. v3: the finished sound, its comfort re-fit, then the four sound macros
    auto designed = [&] (int i, int variant)
    {
        auto p = base[(size_t) i];
        v3::designSound (p, roles[(size_t) i], variant);
        const int moves = v3::fitComfort (p);
        v3::designMacros (p, roles[(size_t) i]);
        return std::make_pair (p, moves);
    };
    std::vector<VaultPreset> out (entries.size());
    std::atomic<int> refits { 0 };
    parallel ((int) entries.size(), "designed", [&] (int i)
    {
        if (! ok[(size_t) i]) return;
        auto [p, moves] = designed (i, 0);
        out[(size_t) i] = p;
        if (moves > 0) ++refits;
    });
    std::printf ("synth: %d presets designed (v3), %d needed a comfort re-fit after the FX\n", (int) entries.size(), refits.load());

    // 3. the gate on every designed preset, then the selection: shipped ids first (in id order), the rhythm presets after them;
    //    a preset is kept if it passes the gate + the assembly QA limits + this file's caps and its fingerprint is >= 3.5 (the
    //    assembly threshold; rhythm presets 3.0 from their own base) from every preset kept before it; one that fails is designed
    //    again (variant 1, then 2) before it is dropped
    std::vector<recipegate::Result> res (entries.size());
    parallel ((int) entries.size(), "gate", [&] (int i) { if (! ok[(size_t) i]) return; res[(size_t) i].preset = out[(size_t) i]; recipegate::analyse (res[(size_t) i]); });
    const float minDist = (float) juce::SystemStats::getEnvironmentVariable ("SYNTH_MIN", "3.5").getDoubleValue();
    std::vector<int> acc;
    std::map<juce::String, int> fails;
    int redesigned[4] {};
    juce::String failLog;
    int nearIdx = -1;
    // skip: an accepted preset left out of the distance check (the pair pass below redesigns it)
    auto check = [&] (recipegate::Result& r, float md, int skip = -1)
    {
        nearIdx = -1;
        juce::String why;
        if (r.fail.isEmpty() && ! vault::validatePreset (r.preset, &why)) r.fail = "invalid: " + why;
        if (r.fail.isEmpty() && r.gainDb > recipegate::maxTrimDb) r.fail = "too-quiet";
        if (r.fail.isEmpty() && r.share25 > 0.085f) r.fail = "own-2-5k";
        if (r.fail.isEmpty() && r.share6k > 0.022f) r.fail = "own-6k";
        if (r.fail.isEmpty() && r.share8k > recipegate::maxShare8k) r.fail = "air-8k";
        if (r.fail.isEmpty() && r.centroid > recipegate::centroidCeiling (r.preset.category)) r.fail = "centroid";
        // (chord headroom: the assembly gives a preset over maxChordOver its peak control, as it did in v2)
        if (r.fail.isEmpty())
            for (int j : acc)
                if (j != skip)
                    if (const float d = recipegate::distance (r.print, res[(size_t) j].print); d < md)
                    { r.fail = "near: " + res[(size_t) j].preset.name + " " + juce::String (d, 2); nearIdx = j; break; }
        return r.fail.isEmpty();
    };
    std::vector<int> nearOf (entries.size(), -1), variantOf (entries.size(), 0);
    std::vector<int> order;
    for (int i = 0; i < (int) entries.size(); ++i) if (ok[(size_t) i] && entries[(size_t) i].pattern == 0) order.push_back (i);
    std::sort (order.begin(), order.end(), [&] (int a, int b) { return entries[(size_t) a].id < entries[(size_t) b].id; });
    for (int i = 0; i < (int) entries.size(); ++i) if (ok[(size_t) i] && entries[(size_t) i].pattern > 0) order.push_back (i);
    for (int i : order)
    {
        auto& r = res[(size_t) i];
        const bool pat = entries[(size_t) i].pattern > 0;
        const float md = pat ? 3.0f : minDist;
        bool good = check (r, md);
        nearOf[(size_t) i] = nearIdx;
        int variant = 0;
        juce::String why = r.fail;
        for (int v = 1; v <= 3 && ! good; ++v)
        {
            recipegate::Result a;
            a.preset = designed (i, v).first;
            recipegate::analyse (a);
            if (check (a, md)) { good = true; variant = v; out[(size_t) i] = a.preset; out[(size_t) i].gain = 1.0f; r = a; }
            else why << " | " << a.fail;
        }
        if (good) { acc.push_back (i); ++redesigned[variant]; variantOf[(size_t) i] = variant; }
        else { ++fails[r.fail.upToFirstOccurrenceOf (":", false, false)]; failLog << "  " << entries[(size_t) i].id << " " << entries[(size_t) i].name << ": " << why << "\n"; }
    }
    // the pair pass: a preset dropped as too close to an accepted one j gets another chance with j designed in another
    // variant too (both must then pass against everything else)
    int pairSaved = 0;
    for (int i : order)
    {
        const int j = nearOf[(size_t) i];
        if (j < 0 || std::find (acc.begin(), acc.end(), i) != acc.end() || std::find (acc.begin(), acc.end(), j) == acc.end()) continue;
        const float md = entries[(size_t) i].pattern > 0 || entries[(size_t) j].pattern > 0 ? 3.0f : minDist;
        bool saved = false;
        for (int vj = 0; vj <= 3 && ! saved; ++vj)
        {
            if (vj == variantOf[(size_t) j]) continue;
            recipegate::Result bj;
            bj.preset = designed (j, vj).first;
            recipegate::analyse (bj);
            if (! check (bj, md, j)) continue;
            for (int vi = 0; vi <= 3 && ! saved; ++vi)
            {
                recipegate::Result bi;
                bi.preset = designed (i, vi).first;
                recipegate::analyse (bi);
                if (! check (bi, md, j) || recipegate::distance (bi.print, bj.print) < md) continue;
                out[(size_t) j] = bj.preset; out[(size_t) j].gain = 1.0f; res[(size_t) j] = bj; variantOf[(size_t) j] = vj;
                out[(size_t) i] = bi.preset; out[(size_t) i].gain = 1.0f; res[(size_t) i] = bi; variantOf[(size_t) i] = vi;
                acc.push_back (i);
                saved = true;
            }
        }
        if (saved) { ++pairSaved; std::printf ("  synth pair pass: %s kept (with %s designed again)\n", entries[(size_t) i].name.toRawUTF8(), entries[(size_t) j].name.toRawUTF8()); }
    }
    std::printf ("synth: the pair pass kept %d more\n", pairSaved);
    std::printf ("synth: %d of %d pass (design %d, v2 space + family tone %d, v2 colours %d, v2 sound %d)\n", (int) acc.size(), (int) order.size(), redesigned[0], redesigned[1], redesigned[2], redesigned[3]);
    for (auto& [f, n] : fails) std::printf ("  synth rejected %-22s %d\n", f.toRawUTF8(), n);
    std::printf ("%s", failLog.toRawUTF8());
    std::sort (acc.begin(), acc.end(), [&] (int a, int b) { return entries[(size_t) a].id < entries[(size_t) b].id; });

    // 4. macro extremes (rendered at 0 and 1, the failing side narrowed until it passes the comfort rules)
    std::vector<v3::MacroReport> mr (acc.size());
    std::vector<VaultPreset> kept (acc.size());
    parallel ((int) acc.size(), "macros", [&] (int k)
    {
        kept[(size_t) k] = out[(size_t) acc[(size_t) k]];
        kept[(size_t) k].gain = 1.0f;
        if (envInt ("SYNTH_MACROCHECK", 1) != 0) mr[(size_t) k] = v3::checkMacros (kept[(size_t) k], res[(size_t) acc[(size_t) k]].preset.gain);
    });
    int fixes = 0;
    juce::StringArray harsh, quiet;
    for (auto& m : mr) { fixes += m.fixes; harsh.addArray (m.harsh); quiet.addArray (m.quiet); }
    std::printf ("synth: macro ranges narrowed %d times; %d macro extremes still outside the comfort rules; %d macros barely audible\n", fixes, harsh.size(), quiet.size());
    for (auto& h : harsh) std::printf ("  harsh: %s\n", h.toRawUTF8());
    if (envInt ("SYNTH_QUIETLOG", 0) != 0) for (auto& q : quiet) std::printf ("  quiet: %s\n", q.toRawUTF8());

    // SYNTH_DUMP=<file>: every kept preset's chain, movement and macros
    if (const auto dump = juce::SystemStats::getEnvironmentVariable ("SYNTH_DUMP", {}); dump.isNotEmpty())
    {
        juce::String t;
        for (size_t k = 0; k < kept.size(); ++k)
        {
            const auto& p = kept[k];
            t << p.id << "\t" << p.name << "\t" << entries[(size_t) acc[k]].recipe << "\t" << roles[(size_t) acc[k]].fam << "\t";
            for (auto& x : p.fx)
            {
                const auto& ti = vault::fxTypeInfo (x.type);
                t << ti.id << (x.on ? "" : "(off)") << "[";
                for (size_t q = 0; q < ti.params.size(); ++q) t << (q ? "," : "") << juce::String (x.p[q], 2);
                t << "] ";
            }
            t << "\t";
            for (int l = 0; l < 2; ++l) if (! p.mod.lfo[l].dests.empty()) { t << "lfo" << (l + 1) << " d" << juce::String (p.mod.lfo[l].depth, 2) << ":"; for (auto& d : p.mod.lfo[l].dests) t << d.target << " "; }
            t << "\t";
            for (int m = 0; m < 4; ++m) t << p.macros[(size_t) m].name << "@" << juce::String (p.macroDefaults[m], 2) << " ";
            t << "\t" << (p.arp.on ? "arp" : "") << (p.gate.on ? "gate" : "") << "\n";
        }
        juce::File (dump).replaceWithText (t);
    }

    // statistics of what ships
    {
        std::map<int, int> perCat, slots;
        int arpOn = 0, gateOn = 0;
        float minPair = 1.0e9f, h1 = 0, s25 = 0, s6 = 0, s8 = 0;
        juce::String pa, pb;
        for (size_t x = 0; x < acc.size(); ++x)
        {
            const auto& r = res[(size_t) acc[x]];
            ++perCat[r.preset.category];
            arpOn += kept[x].arp.on ? 1 : 0; gateOn += kept[x].gate.on ? 1 : 0;
            ++slots[(int) kept[x].fx.size()];
            if (r.preset.category != catFxAtmos) h1 = std::min (h1, r.h1Db);
            s25 = std::max (s25, r.share25); s6 = std::max (s6, r.share6k); s8 = std::max (s8, r.share8k);
            for (size_t y = x + 1; y < acc.size(); ++y)
                if (const float d = recipegate::distance (r.print, res[(size_t) acc[y]].print); d < minPair) { minPair = d; pa = r.preset.name; pb = res[(size_t) acc[y]].preset.name; }
        }
        std::printf ("synth: kept Plucks %d, Leads %d, Pads %d, Synth %d, FX & Atmos %d; arp on %d, gate on %d; min pairwise distance %.2f (%s / %s)\n", perCat[catPlucks],
                     perCat[catLeads], perCat[catPads], perCat[catSynth], perCat[catFxAtmos], arpOn, gateOn, minPair, pa.toRawUTF8(), pb.toRawUTF8());
        std::printf ("synth: at C5 fundamental >= %.1f dB (not FX), 2-5 kHz <= %.1f %%, > 6 kHz <= %.2f %%, > 8 kHz <= %.2f %%\n", h1, 100 * s25, 100 * s6, 100 * s8);
        juce::String sl;
        for (auto& [n, k] : slots) sl << n << " slots: " << k << "  ";
        std::printf ("synth: chain sizes %s\n", sl.toRawUTF8());
    }
    return kept;
}

VAULT_RECIPE (synth, build_synth);
