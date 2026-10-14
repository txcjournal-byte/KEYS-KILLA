// CHROME VAULT v2 recipes: FLUTES & WOODWINDS + WORLD (sound designer "winds"). Run: VaultBankGen -recipe winds (RecipeGate.h).
//
// DESIGN NOTE (research summary; sources at the end of this note)
//  * How professional wind libraries are voiced: every instrument is sold as one player heard from several distances -
//    close mics (dry, upfront, breath and key noise audible: the pop / beat perspective), a tree / mid pair (the default
//    "in the room" balance) and far / surround mics (scoring, mostly hall). Flute libraries typically offer two close
//    colours (crisp and formal vs. darker and intimate) plus a mid-side room, with EQ, compression and reverb built in.
//    Here that becomes ROLES: studio (close + small plate), distant hall, dark / intimate (low-pass, felt-like), airy
//    cloud (slow bloom, long algorithmic tail), tape / lo-fi, and the producer roles below.
//  * How producers use winds in trap / hip-hop: a dry close flute or recorder with a SHORT room or chamber (not a long
//    hall, which pushes it back), plus a quiet dotted-8th / quarter delay ("the room keeps it natural, the delay makes it
//    feel contemporary"), high-mid cut so it stays warm, low cut so it never fights the 808 / bass, ping-pong throws,
//    reversed and "ghost" octave shadows for ear candy, tape wobble for lo-fi. Kalimba / mbira / zither melodies are the
//    classic dark-trap and lo-fi hooks: dry close plus plate, cassette / tape wobble, room versions built from the close
//    signal, and the finger / wood scuffs kept because they make the instrument believable.
//  * Orchestration (why the layers are the layers they are): flute + clarinet unison = a neutral, round blend; oboe +
//    clarinet unison = the clarinet smooths the oboe's nasal tone; flute + oboe = the pastoral unison; octave doubling is
//    clearer than unison (flute over clarinet / bassoon an octave below, bassoon + horn, tenor + soprano sax); a full
//    woodwind choir (flute, oboe, clarinet, bassoon); recorder consorts; flute + harp, flute + strings, sax + upright bass
//    and clarinet + vibraphone from jazz; dan tranh + flute (East Asian zither + flute pairing), interlocking mbiras,
//    kalimba + marimba, balafon + marimba, slit / log drum under thumb pianos.
//  * Acoustics behind the comfort rules: a flute played softly is close to a sine and gets louder mainly by growing upper
//    harmonics (UNSW music acoustics) - so velocity also opens a gentle low-pass (s.velCutoff) and the brightness caps
//    are checked at full velocity. Double reeds (oboe, bassoon) and harmonicas put their energy in a formant above the
//    fundamental: at C5 their fundamental is 10-22 dB under the strongest partial (measured). The owner's C5 rule
//    ("the fundamental is the loudest partial, within 3 dB") is met the way an orchestrator would: a unison CONCERT FLUTE
//    doubling (oboe / harmonica / sax) or, below the flute's range, a soft synth flute tone (bassoon, low clarinet),
//    plus a low shelf and a gentle low-pass. Mbira buzzers, the bowed psaltery and harmonica reeds are very bright
//    (up to 60 % of the power over 6 kHz at C5): they are low-passed until 2-5 kHz <= 7 % and > 6 kHz <= 1.5 %.
//  * Register (FL Studio: MIDI 60 = C5, every preview plays C5): each articulation sits in the octave where C5 is inside
//    its sampled range (keys outside a sample range are silent): piccolo / small ocarina +2, soprano and alto recorders,
//    ocarina, psaltery +1, bassoon / tenor sax / log drum -1, didgeridoo -2 (drone in C).
//
// HOW THE BANK IS MADE
//  * Voices: 56 sampled wind voices + 4 EVOLVE-engine winds (synth flute, breath pipe, square reed, hollow pipe) and 31
//    world voices = instrument x articulation x register of the CHROME VAULT library (VSCO 2 CE flute, piccolo, clarinet,
//    oboe, bassoon; VCSL recorders, ocarinas, saxophones, harmonicas, didgeridoo, dan tranh, balafon, thumb pianos,
//    mbiras, slit / log drum, psaltery). 18 roles per family: processing roles (studio, hall, velvet, echo, tape,
//    behind-the-wall, cloud, puff, swell, saturated, reverse, chorus, ghost, warp, muted, bloom) and production layering
//    roles (breath air, octave double, glimmer octave >= 20 dB under, pad bed, sine sub, drone bed) - never parameter
//    jitter. 45 wind and 39 world layers are the orchestration / production pairings above (1-4 parts).
//  * Every part is calibrated alone (its `gain` puts it at -20 dB K at C5) so layer levels mean the same thing, then
//    every candidate is REPAIRED by measurement (C5 at velocity 0.8 and 1.0): mono-safe width, high shelf / low-pass
//    until 2-5 kHz <= 7 % and > 6 kHz <= 1.5 % at full velocity, low shelf / low-pass / unison flute support until the
//    fundamental is within 2 dB of the strongest partial. Unrepairable candidates are dropped.
//  * Distinctness: candidates come in priority order (every voice in the studio role, the core layers, then each further
//    role). This file runs the shared gate analysis itself and keeps a candidate only if its fingerprint (log-mel C5 x 6
//    segments + C4 / C6, cepstrum, C5 harmonic profile, motion, width; RecipeGate.h distance) is >= 4.8 from every
//    preset kept before it (WINDS_OWN_MIN) - stricter than the shared -dist 3.5. Calibration (VaultBankGen -prints):
//    the same instrument dry vs hall / echo / chorus measures 1-3.5, vs a dark / tape / puff / layer role 4-8, another
//    instrument 7-16; at 3.5 the candidates gave 719 presets, at 4.8 about 470 - the owner asked for every sound to be
//    clearly different, so fewer, more distinct presets.
//  * Result (VaultBankGen -recipe winds, library /home/user/cv-library): 1824 candidates -> 1802 repaired -> 468 kept
//    (Flutes & Woodwinds 250, World 218; 240 one-part, 206 two, 20 three, 2 four), all 468 pass the shared gate, minimum
//    pairwise distance 4.80. At C5: fundamental >= -2.0 dB of the strongest partial, 2-5 kHz <= 6.8 %, > 6 kHz <= 1.3 %,
//    octave-up layers <= -15 dB, tails <= 8.9 s, velocity 0.35 -> 1.0 opens 3.2-25 dB, loudness at the category target.
//  Sources: orchestraltools.helpscoutdocs.com "Which microphone positions should I download?"; orangetreesamples.com
//  Flute Flight user guide (two close colours + M/S room, built-in EQ / reverb); vsl.co.at Synchron bass / contrabass
//  flute (close / mid / tree / surround); loststoriesacademy.com "How to use flute samples in modern music production"
//  (short room + subtle delay, high-mid cut, low cut); musiclibraryreport.com critiques (high-passed reverb, pre-delay);
//  pasttofuturereverbs "Cassette Kalimba" and pianobook kalimba (close / room / tape groups, plate); KVR review of a
//  sampled kalimba (keep the scuffs); andrewhugill.com Orchestra Manual woodwind combinations, orchestrationonline.com
//  "Woodwinds doubling pros and cons", paws.wcu.edu MUS 470 scoring guidelines (unison vs octave doubling, oboe +
//  clarinet); phys.unsw.edu.au/music/flute (spectra vs dynamics, open-hole cutoff).
//
// Environment (optional): WINDS_LIMIT (first N candidates, smoke test), WINDS_THREADS (repair threads, default 4).
#include "RecipeGate.h"
#include <map>
#include <mutex>
#include <set>

namespace
{
using KV = std::vector<std::pair<juce::String, float>>;

VaultPart with (VaultPart p, const KV& kv) { for (auto& [k, v] : kv) p.set (k, v); return p; }
VaultPart lvl (VaultPart p, float l, float pan = 0.0f) { p.level = l; p.pan = pan; return p; }
VaultPart S (const char* id, const char* art, const char* label, int oct = 0, const KV& kv = {})
{
    auto p = with (recipe::samplePart (id, art, label), kv);
    p.octave = oct;
    return p;
}
VaultPart M (const char* id, const char* label, int oct = 0, const KV& kv = {})
{
    auto p = with (recipe::modelPart (id, label), kv);
    p.octave = oct;
    return p;
}
const KV synthBase { { "macro2", 0 }, { "macro5", 0 }, { "master", 0 }, { "revMix", 0 }, { "delayMix", 0 }, { "chorus", 0 }, { "unison", 1 } };
VaultPart Y (const char* label, int oct, const KV& kv)
{
    auto p = with (with (recipe::synthPart (label), synthBase), kv);
    p.octave = oct;
    return p;
}

// EVOLVE-engine wind / pad parts
VaultPart synthFlute (float breath, int oct = 0, const char* label = "Synth Flute", float vib = 0.06f)
{
    return Y (label, oct, { { "engine", 5 }, { "wave", breath }, { "filterType", 0 }, { "cutoff", 3200 }, { "reso", 0.05f }, { "keyTrack", 0.5f },
                            { "attack", 0.07f }, { "decay", 0.6f }, { "sustain", 0.9f }, { "release", 0.3f }, { "velSens", 0.6f },
                            { "lfoRate", 5.0f }, { "lfoPitch", vib } });
}
VaultPart softPad (float l, int oct = 0)
{
    return lvl (Y ("Soft Pad", oct, { { "engine", 0 }, { "wave", 0.1f }, { "unison", 4 }, { "detune", 0.18f }, { "filterType", 0 }, { "cutoff", 1000 },
                                    { "reso", 0.05f }, { "keyTrack", 0.3f }, { "attack", 0.7f }, { "decay", 1.0f }, { "sustain", 0.85f },
                                    { "release", 1.3f }, { "velSens", 0.3f } }), l);
}
VaultPart organTone (float l)
{
    return lvl (Y ("Reed Organ", 0, { { "engine", 4 }, { "wave", 0.25f }, { "filterType", 0 }, { "cutoff", 2400 }, { "keyTrack", 0.4f },
                                       { "attack", 0.03f }, { "sustain", 1.0f }, { "release", 0.25f }, { "velSens", 0.4f } }), l);
}
VaultPart subSine (float l, int oct)
{
    return lvl (Y ("Sine Bass", oct, { { "engine", 6 }, { "wave", 0.0f }, { "fmAmt", 0.0f }, { "filterType", 0 }, { "cutoff", 2000 },
                                       { "attack", 0.004f }, { "decay", 0.9f }, { "sustain", 0.0f }, { "release", 0.25f }, { "velSens", 0.5f } }), l);
}
VaultPart dronePad (float l, int oct)
{
    return lvl (Y ("Drone Pad", oct, { { "engine", 0 }, { "wave", 0.0f }, { "unison", 3 }, { "detune", 0.12f }, { "filterType", 1 }, { "cutoff", 520 },
                                       { "reso", 0.25f }, { "keyTrack", 0.2f }, { "attack", 0.9f }, { "sustain", 1.0f }, { "release", 1.2f },
                                       { "velSens", 0.3f }, { "lfoRate", 0.18f }, { "lfoFilter", 0.15f } }), l);
}

//==============================================================================
struct Voice
{
    juce::String noun;
    int category;
    VaultPart part;
    bool sustained;   // a held wind / bowed / drone note (false: one-shot pluck / hit / staccato)
    bool vibrato;     // the articulation already has vibrato
};

std::vector<Voice> windVoices()
{
    const int W = vault::catFlutesWoodwinds;
    const KV fl { { "s.width", 0.6f }, { "s.velCutoff", 0.35f } };
    const KV soft { { "s.velCutoff", 0.3f } };
    const KV mono5 { { "s.width", 0.5f }, { "s.velCutoff", 0.3f } };
    return {
        { "Concert Flute",         W, S ("winds.flute.concert", "sustain", "Concert Flute", 0, fl), true, false },
        { "Flute Vibrato",         W, S ("winds.flute.concert", "vibrato", "Concert Flute", 0, fl), true, true },
        { "Expressive Flute",      W, S ("winds.flute.concert", "expressive", "Concert Flute", 0, fl), true, true },
        { "Flute Staccato",        W, S ("winds.flute.concert", "staccato", "Concert Flute", 1, fl), false, false },
        { "High Flute",            W, S ("winds.flute.concert", "sustain", "Concert Flute", 1, fl), true, false },
        { "High Flute Vibrato",    W, S ("winds.flute.concert", "vibrato", "Concert Flute", 1, fl), true, true },
        { "High Expressive Flute", W, S ("winds.flute.concert", "expressive", "Concert Flute", 1, fl), true, true },
        { "Piccolo",               W, S ("winds.piccolo", "sustain", "Piccolo", 2, fl), true, false },
        { "Piccolo Staccato",      W, S ("winds.piccolo", "staccato", "Piccolo", 2, fl), false, false },
        { "Clarinet",              W, S ("winds.clarinet", "sustain", "Clarinet", 0, soft), true, false },
        { "Clarinet Staccato",     W, S ("winds.clarinet", "staccato", "Clarinet", 0, soft), false, false },
        { "High Clarinet Staccato", W, S ("winds.clarinet", "staccato", "Clarinet", 1, soft), false, false },
        { "Clarion Clarinet",      W, S ("winds.clarinet", "sustain", "Clarinet", 1, soft), true, false },
        { "Chalumeau Clarinet",    W, S ("winds.clarinet", "sustain", "Clarinet", -1, soft), true, false },
        { "Oboe",                  W, S ("winds.oboe", "sustain", "Oboe", 0, soft), true, false },
        { "Oboe Vibrato",          W, S ("winds.oboe", "vibrato", "Oboe", 0, soft), true, true },
        { "Oboe Staccato",         W, S ("winds.oboe", "staccato", "Oboe", 0, soft), false, false },
        { "High Oboe Staccato",    W, S ("winds.oboe", "staccato", "Oboe", 1, soft), false, false },
        { "High Oboe",             W, S ("winds.oboe", "vibrato", "Oboe", 1, soft), true, true },
        { "Bassoon",               W, S ("winds.bassoon", "sustain", "Bassoon", -1, soft), true, false },
        { "Bassoon Vibrato",       W, S ("winds.bassoon", "vibrato", "Bassoon", -1, soft), true, true },
        { "Bassoon Staccato",      W, S ("winds.bassoon", "staccato", "Bassoon", -1, soft), false, false },
        { "Soprano Recorder",      W, S ("winds.recorder.soprano", "sustain", "Soprano Recorder", 1, soft), true, false },
        { "Tongued Soprano Recorder", W, S ("winds.recorder.soprano", "staccato", "Soprano Recorder", 1, soft), false, false },
        { "Alto Recorder",         W, S ("winds.recorder.alto", "sustain", "Alto Recorder", 1, soft), true, false },
        { "Alto Recorder Vibrato", W, S ("winds.recorder.alto", "vibrato", "Alto Recorder", 1, soft), true, true },
        { "Tongued Alto Recorder", W, S ("winds.recorder.alto", "staccato", "Alto Recorder", 1, soft), false, false },
        { "Tenor Recorder",        W, S ("winds.recorder.tenor", "sustain", "Tenor Recorder", 0, soft), true, false },
        { "Tenor Recorder Vibrato", W, S ("winds.recorder.tenor", "vibrato", "Tenor Recorder", 0, soft), true, true },
        { "Tongued Tenor Recorder", W, S ("winds.recorder.tenor", "staccato", "Tenor Recorder", 0, soft), false, false },
        { "Bass Recorder",         W, S ("winds.recorder.bass", "sustain", "Bass Recorder", 0, soft), true, false },
        { "Bass Recorder Vibrato", W, S ("winds.recorder.bass", "vibrato", "Bass Recorder", 0, soft), true, true },
        { "Tongued Bass Recorder", W, S ("winds.recorder.bass", "staccato", "Bass Recorder", 0, soft), false, false },
        { "Small Ocarina",         W, S ("winds.ocarina.small", "sustain", "Small Ocarina", 2, soft), true, false },
        { "Small Ocarina Staccato", W, S ("winds.ocarina.small", "staccato", "Small Ocarina", 2, soft), false, false },
        { "Ocarina",               W, S ("winds.ocarina", "sustain", "Ocarina", 1, soft), true, false },
        { "Ocarina Vibrato",       W, S ("winds.ocarina", "vibrato", "Ocarina", 1, soft), true, true },
        { "Tenor Sax",             W, S ("winds.sax.tenor", "sustain", "Tenor Sax", -1, soft), true, false },
        { "Tenor Sax Vibrato",     W, S ("winds.sax.tenor", "vibrato", "Tenor Sax", -1, soft), true, true },
        { "Tenor Sax Staccato",    W, S ("winds.sax.tenor", "staccato", "Tenor Sax", -1, soft), false, false },
        { "Tenor Sax Stabs",       W, S ("winds.sax.tenor", "staccato", "Tenor Sax", 0, soft), false, false },
        { "Altissimo Tenor Sax",   W, S ("winds.sax.tenor", "vibrato", "Tenor Sax", 0, soft), true, true },
        { "Soprano Sax",           W, S ("winds.sax.soprano", "sustain", "Soprano Sax", 0, soft), true, false },
        { "Soprano Sax Vibrato",   W, S ("winds.sax.soprano", "vibrato", "Soprano Sax", 0, soft), true, true },
        { "Soprano Sax Staccato",  W, S ("winds.sax.soprano", "staccato", "Soprano Sax", 0, soft), false, false },
        { "Chromatic Harmonica",   W, S ("winds.harmonica.chromatic", "normal", "Chromatic Harmonica", 0, mono5), true, false },
        { "Accented Harmonica",    W, S ("winds.harmonica.chromatic", "accented", "Chromatic Harmonica", 0, mono5), true, false },
        { "Harmonica Vibrato",     W, S ("winds.harmonica.chromatic", "vibrato", "Chromatic Harmonica", 0, mono5), true, true },
        { "Low Chromatic Harmonica", W, S ("winds.harmonica.chromatic", "normal", "Chromatic Harmonica", -1, mono5), true, false },
        { "Blues Harp",            W, S ("winds.harmonica.c", "normal", "Blues Harmonica", 0, mono5), true, false },
        { "Soft Blues Harp",       W, S ("winds.harmonica.c", "soft", "Blues Harmonica", 0, mono5), true, false },
        { "Blues Harp Vibrato",    W, S ("winds.harmonica.c", "vibrato", "Blues Harmonica", 0, mono5), true, true },
        { "Hand Wah Harp",         W, S ("winds.harmonica.f", "hand vibrato", "Blues Harmonica", 1, mono5), true, true },
        { "High Blues Harp",       W, S ("winds.harmonica.f", "normal", "Blues Harmonica", 1, mono5), true, false },
        { "High Blues Harp Vibrato", W, S ("winds.harmonica.f", "vibrato", "Blues Harmonica", 1, mono5), true, true },
        { "Low Harmonica Vibrato", W, S ("winds.harmonica.chromatic", "vibrato", "Chromatic Harmonica", -1, mono5), true, true },        { "Blues Harp Chops",      W, S ("winds.harmonica.f", "staccato", "Blues Harmonica", 1, mono5), false, false },
        { "Synth Flute",           W, synthFlute (0.25f), true, true },
        { "Breath Pipe",           W, with (synthFlute (0.85f, 0, "Breath Pipe", 0.09f), { { "attack", 0.12f }, { "cutoff", 2400 } }), true, true },
        { "Square Reed",           W, Y ("Square Reed", 0, { { "engine", 0 }, { "wave", 0.5f }, { "filterType", 1 }, { "cutoff", 1300 }, { "reso", 0.1f },
                                                             { "keyTrack", 0.6f }, { "fenv", 0.15f }, { "fdecay", 0.3f }, { "attack", 0.04f },
                                                             { "sustain", 0.9f }, { "release", 0.25f }, { "velSens", 0.6f },
                                                             { "lfoRate", 5.2f }, { "lfoPitch", 0.06f } }), true, true },
        { "Hollow Pipe",           W, Y ("Hollow Pipe", 0, { { "engine", 4 }, { "wave", 0.05f }, { "filterType", 0 }, { "cutoff", 2600 },
                                                             { "attack", 0.09f }, { "sustain", 1.0f }, { "release", 0.35f }, { "velSens", 0.6f },
                                                             { "lfoRate", 4.6f }, { "lfoPitch", 0.05f }, { "lfoAmp", 0.08f } }), true, true },
    };
}

std::vector<Voice> worldVoices()
{
    const int X = vault::catWorld;
    const KV w5 { { "s.width", 0.5f } };
    const KV dark { { "s.velCutoff", 0.3f } };
    return {
        { "Dan Tranh",             X, S ("world.dantranh", "pluck", "Dan Tranh", 0, dark), false, false },
        { "High Dan Tranh",        X, S ("world.dantranh", "pluck", "Dan Tranh", 1, dark), false, false },
        { "Dan Tranh Bends",       X, S ("world.dantranh", "vibrato", "Dan Tranh", 0, dark), false, true },
        { "High Dan Tranh Bends",  X, S ("world.dantranh", "vibrato", "Dan Tranh", 1, dark), false, true },
        { "Dan Tranh Tremolo",     X, S ("world.dantranh", "tremolo", "Dan Tranh", 0, dark), true, false },
        { "High Dan Tranh Tremolo", X, S ("world.dantranh", "tremolo", "Dan Tranh", 1, dark), true, false },
        { "Low Dan Tranh Bends",   X, S ("world.dantranh", "vibrato", "Dan Tranh", -1, dark), false, true },
        { "Low Dan Tranh",         X, S ("world.dantranh", "pluck", "Dan Tranh", -1, dark), false, false },
        { "Balafon",               X, S ("world.balafon", "traditional mallet", "Balafon", 0, w5), false, false },
        { "Hard Balafon",          X, S ("world.balafon", "hard mallet", "Balafon", 0, w5), false, false },
        { "Soft Balafon",          X, S ("world.balafon", "soft mallet", "Balafon", 0, w5), false, false },
        { "High Balafon",          X, S ("world.balafon", "traditional mallet", "Balafon", 1, w5), false, false },
        { "High Hard Balafon",     X, S ("world.balafon", "hard mallet", "Balafon", 1, w5), false, false },
        { "High Soft Balafon",     X, S ("world.balafon", "soft mallet", "Balafon", 1, w5), false, false },
        { "Thumb Piano",           X, S ("world.kalimba", "pluck", "Thumb Piano"), false, false },
        { "High Thumb Piano",      X, S ("world.kalimba", "pluck", "Thumb Piano", 1), false, false },
        { "Low Thumb Piano",       X, S ("world.kalimba.low", "pluck", "Low Thumb Piano"), false, false },
        { "Bass Thumb Piano",      X, S ("world.kalimba.low", "pluck", "Low Thumb Piano", -1), false, false },
        { "Bright Low Thumb Piano", X, S ("world.kalimba.low", "pluck", "Low Thumb Piano", 1), false, false },
        { "Mbira",                 X, S ("world.mbira", "pluck", "Mbira", 0, dark), false, false },
        { "Low Mbira",             X, S ("world.mbira", "pluck", "Mbira", -1, dark), false, false },
        { "Deep Mbira",            X, S ("world.mbira.deep", "pluck", "Deep Mbira", 0, dark), false, false },
        { "Low Deep Mbira",        X, S ("world.mbira.deep", "pluck", "Deep Mbira", -1, dark), false, false },
        { "Nyunga Mbira",          X, S ("world.mbira.nyunga", "pluck", "Nyunga Mbira", 0, dark), false, false },
        { "Slit Drum",             X, S ("world.slitdrum", "hit", "Slit Drum", 0, dark), false, false },
        { "Log Drum",              X, S ("world.slitdrum", "log", "Log Drum", -1), false, false },
        { "Psaltery",              X, S ("pluck.psaltery", "pluck", "Psaltery", 1, dark), false, false },
        { "Bowed Psaltery",        X, S ("pluck.psaltery", "bowed", "Bowed Psaltery", 1, dark), true, false },
        { "Psaltery Spiccato",     X, S ("pluck.psaltery", "spiccato", "Psaltery", 1, dark), false, false },
        { "Didgeridoo",            X, S ("world.didgeridoo", "drone", "Didgeridoo", -2, { { "s.release", 0.5f } }), true, false },
        { "Didgeridoo Groove",     X, S ("world.didgeridoo", "phrase", "Didgeridoo", -2, { { "s.oneShot", 0 }, { "s.release", 0.4f } }), true, false },
    };
}

//==============================================================================
// roles (treatments): FX on part 0, "s.*" / envelope keys on every part
struct Treat
{
    juce::StringArray prefixes;   // "" = the plain name
    KV fx;
    bool forSustained = true, forShort = true;
    bool needsPlainVibrato = false;   // adds a designed vibrato: only on articulations without one
    int add = 0;                      // an extra part that makes the role (see Add)
};
// extra parts a role adds to part 0 (production layering, not effects)
enum Add { addNone, addPad, addSub, addOctDown, addAir, addGlimmer, addBedDrone };

// winds: close / room / hall / producer roles - ordered by how much they change the sound
std::vector<Treat> windTreats()
{
    return {
        { { "" }, { { "revMix", 0.12f }, { "revSize", 0.3f }, { "revType", 1 } } },
        { { "Distant", "Far", "Hall" }, { { "revMix", 0.32f }, { "revSize", 0.6f }, { "revType", 0 }, { "width", 0.6f } } },
        { { "Velvet", "Hushed", "Intimate" }, { { "s.cutoff", 1300 }, { "cutoff", 1300 }, { "eqHigh", -6.0f }, { "eqLow", 2.0f }, { "revMix", 0.16f },
                                                { "revSize", 0.4f }, { "revType", 1 } } },
        { { "Night", "Echo", "Midnight" }, { { "revMix", 0.12f }, { "revSize", 0.3f }, { "revType", 1 }, { "delayMix", 0.2f }, { "delayTime", 2 },
                                             { "delayFb", 0.32f }, { "delayMode", 1 } } },
        { { "Dusty", "Cassette", "Faded" }, { { "wow", 0.55f }, { "s.cutoff", 2400 }, { "cutoff", 2400 }, { "eqHigh", -9.0f },
                                              { "revMix", 0.12f }, { "revSize", 0.35f }, { "revType", 1 } } },
        { { "Behind the Wall", "Next Room", "Muffled" }, { { "s.cutoff", 650 }, { "cutoff", 650 }, { "eqHigh", -12.0f }, { "revMix", 0.22f },
                                                           { "revSize", 0.45f }, { "revType", 1 } } },
        { { "Floating", "Cloud", "Drifting" }, { { "s.attack", 0.28f }, { "attack", 0.3f }, { "s.release", 0.9f }, { "release", 0.9f },
                                                 { "revMix", 0.36f }, { "revSize", 0.45f }, { "revType", 2 }, { "width", 0.75f } }, true, false },
        { { "Puff", "Breath", "Short" }, { { "s.decay", 0.22f }, { "s.sustain", 0.0f }, { "s.release", 0.12f }, { "decay", 0.22f }, { "sustain", 0.0f },
                                           { "release", 0.12f }, { "revMix", 0.16f }, { "revSize", 0.4f }, { "revType", 1 } }, true, false },
        { { "Swelling", "Rising", "Dawn" }, { { "s.attack", 0.65f }, { "attack", 0.65f }, { "s.release", 0.9f }, { "release", 0.9f },
                                              { "revMix", 0.26f }, { "revSize", 0.5f }, { "revType", 0 } }, true, false },
        { { "Gritty", "Smoky", "Saturated" }, { { "drive", 0.3f }, { "driveType", 1 }, { "s.cutoff", 3000 }, { "cutoff", 3000 }, { "eqHigh", -4.0f },
                                                { "revMix", 0.12f }, { "revType", 1 } } },
        { { "Reverse", "Backward", "Rewind" }, { { "reverse", 0.5f }, { "revMix", 0.18f }, { "revType", 0 } } },
        { { "Airy", "Breathy", "Windswept" }, { { "revMix", 0.18f }, { "revSize", 0.45f }, { "revType", 1 } }, true, true, false, addAir },
        { { "Twin", "Octave", "Doubled" }, { { "revMix", 0.16f }, { "revSize", 0.4f }, { "revType", 1 }, { "width", 0.65f } }, true, true, false, addOctDown },
        { { "Glowing", "Halo", "Aurora" }, { { "revMix", 0.22f }, { "revSize", 0.5f }, { "revType", 0 } }, true, false, false, addPad },
        { { "Singing", "Lyrical", "Aria" }, { { "s.vibrato", 22.0f }, { "s.vibRate", 5.3f }, { "lfoPitch", 0.12f }, { "revMix", 0.18f },
                                              { "revSize", 0.45f }, { "revType", 0 } }, true, false, true },
        { { "Wide", "Ensemble", "Chorus" }, { { "chorus", 0.55f }, { "width", 0.8f }, { "revMix", 0.15f }, { "revType", 1 } } },
        { { "Shadow", "Undertow", "Deep Water" }, { { "ghost", 0.45f }, { "ghostRev", 0 }, { "ghostOct", 1 }, { "ghostBlur", 0.5f }, { "revMix", 0.14f },
                                                    { "revType", 0 } } },
        { { "Warped", "Seasick", "Melted" }, { { "wow", 0.8f }, { "s.cutoff", 2000 }, { "cutoff", 2000 }, { "eqHigh", -8.0f },
                                               { "revMix", 0.2f }, { "revSize", 0.45f }, { "revType", 2 } } },
    };
}

// world plucks / hits: close / room / producer roles
std::vector<Treat> worldTreats()
{
    return {
        { { "" }, { { "revMix", 0.12f }, { "revSize", 0.3f }, { "revType", 1 } } },
        { { "Temple", "Distant", "Courtyard" }, { { "revMix", 0.32f }, { "revSize", 0.6f }, { "revType", 0 }, { "width", 0.6f } } },
        { { "Felt", "Velvet", "Hushed" }, { { "s.cutoff", 1200 }, { "eqHigh", -6.0f }, { "eqLow", 2.0f }, { "revMix", 0.16f }, { "revSize", 0.4f }, { "revType", 1 } } },
        { { "Night", "Echo", "Midnight" }, { { "revMix", 0.12f }, { "revSize", 0.3f }, { "revType", 1 }, { "delayMix", 0.22f }, { "delayTime", 2 },
                                             { "delayFb", 0.32f }, { "delayMode", 1 } } },
        { { "Dusty", "Cassette", "Faded" }, { { "wow", 0.55f }, { "s.cutoff", 2400 }, { "eqHigh", -9.0f }, { "revMix", 0.12f },
                                              { "revSize", 0.35f }, { "revType", 1 } } },
        { { "Muted", "Damped", "Stopped" }, { { "s.oneShot", 0 }, { "s.decay", 0.16f }, { "s.sustain", 0.0f }, { "s.release", 0.08f }, { "revMix", 0.12f },
                                              { "revSize", 0.3f }, { "revType", 1 } }, false, true },
        { { "Blooming", "Bowed", "Unfolding" }, { { "s.attack", 0.3f }, { "s.oneShot", 0 }, { "s.release", 1.2f }, { "revMix", 0.28f }, { "revSize", 0.55f },
                                                  { "revType", 0 } }, false, true },
        { { "Behind the Wall", "Next Room", "Muffled" }, { { "s.cutoff", 650 }, { "eqHigh", -12.0f }, { "revMix", 0.22f }, { "revSize", 0.45f }, { "revType", 1 } } },
        { { "Reverse", "Backward", "Rewind" }, { { "reverse", 0.5f }, { "revMix", 0.18f }, { "revType", 0 } } },
        { { "Dream", "Cloud", "Haze" }, { { "revMix", 0.4f }, { "revSize", 0.6f }, { "revType", 2 }, { "width", 0.75f }, { "delayMix", 0.1f },
                                          { "delayTime", 2 }, { "delayFb", 0.3f } } },
        { { "Heavy", "Deep", "Weighted" }, { { "revMix", 0.12f }, { "revSize", 0.3f }, { "revType", 1 } }, true, true, false, addSub },
        { { "Twin", "Octave", "Doubled" }, { { "revMix", 0.16f }, { "revSize", 0.4f }, { "revType", 1 }, { "width", 0.65f } }, true, true, false, addOctDown },
        { { "Glimmer", "Sparkle", "Starlit" }, { { "revMix", 0.2f }, { "revSize", 0.45f }, { "revType", 1 } }, true, true, false, addGlimmer },
        { { "Glowing", "Halo", "Aurora" }, { { "revMix", 0.22f }, { "revSize", 0.5f }, { "revType", 0 } }, true, true, false, addPad },
        { { "Desert", "Dune", "Mirage" }, { { "revMix", 0.2f }, { "revSize", 0.5f }, { "revType", 0 } }, true, true, false, addBedDrone },
        { { "Shimmer", "Wide", "Silken" }, { { "chorus", 0.55f }, { "width", 0.8f }, { "revMix", 0.15f }, { "revType", 1 } } },
        { { "Gritty", "Smoky", "Saturated" }, { { "drive", 0.3f }, { "driveType", 1 }, { "s.cutoff", 3000 }, { "eqHigh", -4.0f }, { "revMix", 0.12f }, { "revType", 1 } } },
        { { "Warped", "Seasick", "Melted" }, { { "wow", 0.8f }, { "s.cutoff", 2000 }, { "eqHigh", -8.0f }, { "revMix", 0.2f },
                                               { "revSize", 0.45f }, { "revType", 2 } } },
    };
}

//==============================================================================
struct Layer { juce::String noun; int category; std::vector<VaultPart> parts; bool sustained; };

std::vector<Layer> windLayers()
{
    const int W = vault::catFlutesWoodwinds;
    const KV fl { { "s.width", 0.6f }, { "s.velCutoff", 0.35f } };
    const KV soft { { "s.velCutoff", 0.3f } };
    auto flute = [&] (int o = 0, const char* art = "sustain") { return S ("winds.flute.concert", art, "Concert Flute", o, fl); };
    auto clar = [&] (int o = 0) { return S ("winds.clarinet", "sustain", "Clarinet", o, soft); };
    auto oboe = [&] (int o = 0) { return S ("winds.oboe", "vibrato", "Oboe", o, soft); };
    auto bsn = [&] (int o = -1) { return S ("winds.bassoon", "sustain", "Bassoon", o, soft); };
    auto tsx = [&] (int o = -1, const char* art = "sustain") { return S ("winds.sax.tenor", art, "Tenor Sax", o, soft); };
    auto ssx = [&] (int o = 0) { return S ("winds.sax.soprano", "vibrato", "Soprano Sax", o, soft); };
    return {
        { "Flute and Clarinet",     W, { flute(), lvl (clar(), 0.6f) }, true },
        { "Pastoral Pair",          W, { flute (0, "vibrato"), lvl (oboe(), 0.45f) }, true },
        { "Oboe and Clarinet",      W, { clar(), lvl (oboe(), 0.55f) }, true },
        { "Flute over Clarinet",    W, { flute(), lvl (clar (-1), 0.5f) }, true },
        { "Clarinet over Bassoon",  W, { clar(), lvl (bsn(), 0.5f) }, true },
        { "Woodwind Choir",         W, { flute(), lvl (clar(), 0.55f, -0.3f), lvl (bsn(), 0.5f, 0.3f), lvl (oboe(), 0.3f, 0.15f) }, true },
        { "Recorder Consort",       W, { S ("winds.recorder.tenor", "sustain", "Tenor Recorder", 0, soft), lvl (S ("winds.recorder.bass", "sustain", "Bass Recorder", 0, soft), 0.6f, 0.3f),
                                         lvl (S ("winds.recorder.alto", "sustain", "Alto Recorder", 1, soft), 0.18f, -0.3f) }, true },
        { "Flute and Harp",         W, { flute(), lvl (S ("pluck.harp.concert", "pluck", "Concert Harp"), 0.5f, 0.2f) }, true },
        { "Flute over Strings",     W, { flute (0, "vibrato"), lvl (S ("strings.violins.ensemble", "sustain", "Violin Section", 0, { { "s.attack", 0.25f } }), 0.35f) }, true },
        { "Sax and Flute Section",  W, { tsx (0), lvl (flute(), 0.5f, 0.25f) }, true },
        { "Sax Octaves",            W, { tsx (-1, "vibrato"), lvl (ssx(), 0.2f, 0.2f) }, true },
        { "Clarinet and Vibes",     W, { clar(), lvl (S ("mallets.vibraphone", "soft mallets", "Vibraphone"), 0.4f, 0.25f) }, true },
        { "Harmonica and Organ",    W, { S ("winds.harmonica.chromatic", "vibrato", "Chromatic Harmonica", 0, { { "s.width", 0.5f } }), organTone (0.35f) }, true },
        { "Vocal Flute",            W, { flute (0, "vibrato"), lvl (M ("voice.pad", "Vocal Pad"), 0.3f) }, true },
        { "Breath and Flute",       W, { flute(), lvl (synthFlute (0.9f, 0, "Breath Pipe", 0.0f), 0.3f) }, true },
        { "Oboe and Bassoon",       W, { oboe(), lvl (bsn(), 0.5f) }, true },
        { "Piccolo and Flute",      W, { flute (1), lvl (S ("winds.piccolo", "sustain", "Piccolo", 2, fl), 0.15f) }, true },
        { "Flute over Pad",         W, { flute(), softPad (0.3f) }, true },
        { "Late Night Sax",         W, { tsx (-1, "vibrato"), softPad (0.3f, -1) }, true },
        { "Low Woodwinds",          W, { bsn(), lvl (clar (-1), 0.55f) }, true },
        { "Horn and Bassoon",       W, { bsn(), lvl (S ("brass.horn", "sustain", "French Horn", -1), 0.4f) }, true },
        { "Glass Flute",            W, { flute(), lvl (S ("fx.glass", "slow", "Glass Harp", 1), 0.12f) }, true },
        { "Clarinet and Cellos",    W, { clar(), lvl (S ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.attack", 0.2f } }), 0.3f) }, true },
        { "Bossa Flute",            W, { flute (0, "vibrato"), lvl (M ("guitar.nylon", "Nylon Guitar"), 0.45f, 0.25f) }, true },
        { "Soprano Sax and Strings", W, { ssx(), lvl (S ("strings.violins.ensemble", "sustain", "Violin Section", 0, { { "s.attack", 0.3f } }), 0.3f) }, true },
        { "Baroque Recorder",       W, { S ("winds.recorder.alto", "vibrato", "Alto Recorder", 1, soft), lvl (S ("keys.harpsichord.baroque", "normal", "Harpsichord", 0), 0.45f, 0.3f) }, true },
        { "Oboe over Strings",      W, { oboe(), lvl (S ("strings.violas.ensemble", "sustain", "Viola Section", 0, { { "s.attack", 0.25f } }), 0.35f) }, true },
        { "Sax and Upright Duo",    W, { tsx (-1, "vibrato"), lvl (S ("strings.contrabass", "pizzicato", "Contrabass", -2), 0.55f) }, true },
        { "Folk Harmonica",         W, { S ("winds.harmonica.c", "soft", "Blues Harmonica", 0, { { "s.width", 0.5f } }), lvl (M ("guitar.acoustic", "Steel Acoustic Guitar"), 0.5f, 0.3f) }, true },
        { "Clarinet Shadows",       W, { clar(), lvl (clar (-1), 0.35f, 0.3f) }, true },
        { "Flute and Glockenspiel", W, { flute(), lvl (S ("mallets.glockenspiel", "hit", "Glockenspiel", 2), 0.07f, 0.2f) }, true },
        { "Staccato Winds",         W, { S ("winds.flute.concert", "staccato", "Concert Flute", 1, fl), lvl (S ("winds.clarinet", "staccato", "Clarinet", 1, soft), 0.6f, -0.3f),
                                         lvl (S ("winds.oboe", "staccato", "Oboe", 1, soft), 0.3f, 0.3f) }, false },
        { "Bassoon and Pizzicato",  W, { S ("winds.bassoon", "staccato", "Bassoon", -1, soft), lvl (S ("strings.cellos.ensemble", "pizzicato", "Cello Section", -1), 0.5f) }, false },
        { "Ocarina and Pad",        W, { S ("winds.ocarina", "vibrato", "Ocarina", 1, soft), softPad (0.25f) }, true },
        { "Twin Recorders",         W, { lvl (S ("winds.recorder.tenor", "vibrato", "Tenor Recorder", 0, soft), 0.85f, -0.4f),
                                         lvl (S ("winds.recorder.bass", "vibrato", "Bass Recorder", 0, soft), 0.75f, 0.4f) }, true },
        { "Harmonica Duo",          W, { lvl (S ("winds.harmonica.chromatic", "normal", "Chromatic Harmonica", 0, { { "s.width", 0.5f } }), 0.85f, -0.4f),
                                         lvl (S ("winds.harmonica.c", "soft", "Blues Harmonica", 0, { { "s.width", 0.5f } }), 0.75f, 0.4f) }, true },
        { "Ocarina and Flute",      W, { S ("winds.ocarina", "vibrato", "Ocarina", 1, soft), lvl (flute (1), 0.45f, 0.25f) }, true },
        { "Recorder and Nylon",     W, { S ("winds.recorder.alto", "vibrato", "Alto Recorder", 1, soft), lvl (M ("guitar.nylon", "Nylon Guitar"), 0.5f, 0.3f) }, true },
        { "Flute and Choir",        W, { flute(), lvl (M ("voice.choir", "Choir"), 0.25f) }, true },
        { "Bassoon and Tuba",       W, { bsn(), lvl (S ("brass.tuba", "sustain", "Tuba", -1), 0.35f) }, true },
        { "Flute and Toy Piano",    W, { flute(), lvl (M ("keys.toy", "Felt Toy Piano"), 0.35f, 0.25f) }, true },
        { "Tenor Section",          W, { lvl (tsx (-1, "sustain"), 0.85f, -0.45f), lvl (with (tsx (-1, "vibrato"), { { "s.tune", 8.0f } }), 0.75f, 0.45f) }, true },
        { "Flute and Marimba",      W, { S ("winds.flute.concert", "staccato", "Concert Flute", 1, fl), lvl (S ("mallets.marimba", "hit", "Marimba", 1), 0.45f, 0.25f) }, false },
        { "Oboe and Horn",          W, { oboe(), lvl (S ("brass.horn", "sustain", "French Horn", -1), 0.3f) }, true },
        { "Choir of Flutes",        W, { flute(), lvl (S ("winds.recorder.tenor", "sustain", "Tenor Recorder", 0, soft), 0.5f, -0.35f),
                                         lvl (S ("winds.recorder.bass", "sustain", "Bass Recorder", 0, soft), 0.45f, 0.35f) }, true },
    };
}

std::vector<Layer> worldLayers()
{
    const int X = vault::catWorld;
    const KV dark { { "s.velCutoff", 0.3f } };
    auto kal = [] (int o = 0) { return S ("world.kalimba", "pluck", "Thumb Piano", o); };
    auto tranh = [&] (int o = 0, const char* art = "pluck") { return S ("world.dantranh", art, "Dan Tranh", o, dark); };
    return {
        { "Thumb Piano and Marimba",     X, { kal(), lvl (S ("mallets.marimba", "hit", "Marimba"), 0.5f, 0.25f) }, false },
        { "Thumb Piano Music Box",   X, { kal(), lvl (M ("keys.toy", "Felt Toy Piano"), 0.4f, -0.25f) }, false },
        { "Mbira and Nylon",         X, { S ("world.mbira", "pluck", "Mbira", 0, dark), lvl (M ("guitar.nylon", "Nylon Guitar"), 0.5f, 0.3f) }, false },
        { "Zither and Flute",        X, { tranh(), lvl (S ("winds.flute.concert", "vibrato", "Concert Flute", 0, { { "s.width", 0.6f } }), 0.35f, 0.2f) }, false },
        { "Silk Strings",            X, { tranh(), lvl (S ("pluck.psaltery", "bowed", "Bowed Psaltery", 1, { { "s.attack", 0.3f }, { "s.cutoff", 1800 } }), 0.05f) }, false },
        { "Balafon and Marimba",     X, { S ("world.balafon", "traditional mallet", "Balafon", 0, { { "s.width", 0.5f } }), lvl (S ("mallets.marimba", "hit", "Marimba"), 0.5f) }, false },
        { "Thumb Piano Sub",             X, { S ("world.kalimba.low", "pluck", "Low Thumb Piano"), subSine (0.5f, -1) }, false },
        { "Log Drum Thumb Piano",   X, { kal(), lvl (S ("world.slitdrum", "log", "Log Drum", -1), 0.5f, -0.2f) }, false },
        { "Didgeridoo Dreamtime",    X, { S ("world.didgeridoo", "drone", "Didgeridoo", -2, { { "s.release", 0.6f } }), dronePad (0.35f, -1) }, true },
        { "Harp and Thumb Piano",        X, { S ("pluck.harp.folk", "pluck", "Folk Harp"), lvl (kal(), 0.5f, 0.3f) }, false },
        { "Silk Harp",               X, { tranh(), lvl (S ("pluck.harp.folk", "pluck", "Folk Harp"), 0.5f, -0.3f) }, false },
        { "Thumb Piano and Chimes",      X, { kal(), lvl (S ("bells.handchimes", "hit", "Hand Chimes"), 0.25f, 0.3f) }, false },
        { "Ocarina Island",          X, { S ("winds.ocarina", "vibrato", "Ocarina", 1), lvl (kal(), 0.55f, 0.3f) }, true },
        { "Zither and Nylon",        X, { tranh(), lvl (M ("guitar.nylon", "Nylon Guitar"), 0.45f, 0.3f) }, false },
        { "Balafon and Thumb Piano", X, { S ("world.balafon", "soft mallet", "Balafon", 0, { { "s.width", 0.5f } }), lvl (S ("world.kalimba.low", "pluck", "Low Thumb Piano"), 0.6f, 0.3f) }, false },
        { "Thumb Piano and Vibes",       X, { kal(), lvl (S ("mallets.vibraphone", "soft mallets", "Vibraphone"), 0.4f, -0.3f) }, false },
        { "Psaltery and Harp",       X, { S ("pluck.psaltery", "pluck", "Psaltery", 1, dark), lvl (S ("pluck.harp.concert", "pluck", "Concert Harp", 0), 0.6f) }, false },
        { "Glass Thumb Piano",           X, { kal(), lvl (S ("fx.glass", "slow", "Glass Harp", 1), 0.1f) }, false },
        { "Interlocking Mbiras",     X, { lvl (S ("world.mbira", "pluck", "Mbira", 0, dark), 0.85f, -0.45f), lvl (S ("world.mbira.deep", "pluck", "Deep Mbira", 0, dark), 0.8f, 0.45f) }, false },
        { "Zither and Cellos",       X, { tranh(), lvl (S ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.attack", 0.4f }, { "s.release", 0.8f } }), 0.25f) }, false },
        { "Thumb Piano over Pad",        X, { kal(), softPad (0.25f) }, false },
        { "Log Drum Didgeridoo",     X, { S ("world.didgeridoo", "drone", "Didgeridoo", -2, { { "s.release", 0.5f } }), lvl (S ("world.slitdrum", "log", "Log Drum", -1), 0.15f) }, true },
        { "Tremolo Zither Strings",  X, { tranh (0, "tremolo"), lvl (S ("strings.violins.ensemble", "sustain", "Violin Section", 0, { { "s.attack", 0.35f } }), 0.25f) }, true },
        { "Minstrel",                X, { S ("winds.recorder.alto", "vibrato", "Alto Recorder", 1), lvl (S ("pluck.harp.folk", "pluck", "Folk Harp", 1), 0.6f, 0.25f) }, true },
        { "Mountain Zither",         X, { tranh(), lvl (S ("pluck.dulcimer", "pluck", "Stick Dulcimer", 0, { { "s.cutoff", 2000 } }), 0.4f, -0.3f) }, false },
        { "Marimba Mbira",           X, { S ("world.mbira.nyunga", "pluck", "Nyunga Mbira", 0, dark), lvl (S ("mallets.marimba", "hit", "Marimba"), 0.45f, 0.3f) }, false },
        { "Dan Tranh Duet",          X, { tranh(), lvl (tranh (0, "tremolo"), 0.35f, 0.3f) }, false },
        { "Thumb Piano Trio",        X, { kal(), lvl (S ("world.kalimba.low", "pluck", "Low Thumb Piano"), 0.45f, -0.35f), lvl (S ("world.mbira", "pluck", "Mbira", 0, dark), 0.3f, 0.35f) }, false },
        { "Slit Drum Marimba",       X, { S ("world.slitdrum", "hit", "Slit Drum", 0, dark), lvl (S ("mallets.marimba", "hit", "Marimba"), 0.5f, 0.2f) }, false },
        { "Zither over Pad",         X, { tranh(), softPad (0.25f) }, false },
        { "Didgeridoo Choir",        X, { S ("world.didgeridoo", "drone", "Didgeridoo", -2, { { "s.release", 0.6f } }), lvl (M ("voice.choir", "Choir", -1), 0.12f) }, true },
        { "Psaltery and Chimes",     X, { S ("pluck.psaltery", "pluck", "Psaltery", 1, dark), lvl (S ("bells.handchimes", "hit", "Hand Chimes", 1), 0.25f, 0.3f) }, false },
        { "Harp and Mbira",          X, { S ("pluck.harp.folk", "pluck", "Folk Harp"), lvl (S ("world.mbira", "pluck", "Mbira", 0, dark), 0.45f, 0.3f) }, false },
        { "Thumb Piano and Steel",   X, { kal(), lvl (M ("guitar.acoustic", "Steel Acoustic Guitar"), 0.4f, -0.3f) }, false },
        { "Balafon and Log Drum",    X, { S ("world.balafon", "traditional mallet", "Balafon", 0, { { "s.width", 0.5f } }), lvl (S ("world.slitdrum", "log", "Log Drum", -1), 0.4f) }, false },
        { "Flute and Thumb Piano",   X, { kal(), lvl (S ("winds.flute.concert", "vibrato", "Concert Flute", 0, { { "s.width", 0.6f } }), 0.3f, 0.2f) }, false },
        { "Ocarina and Harp",        X, { S ("winds.ocarina", "vibrato", "Ocarina", 1), lvl (S ("pluck.harp.folk", "pluck", "Folk Harp", 1), 0.5f, -0.25f) }, true },
        { "Zither and Violin",       X, { tranh(), lvl (S ("strings.violin.solo", "sustain", "Solo Violin", 0, { { "s.attack", 0.2f }, { "s.vibrato", 15.0f } }), 0.25f, 0.2f) }, false },
        { "Desert Strings",          X, { tranh (0, "vibrato"), dronePad (0.25f, -1) }, false },
    };
}

//==============================================================================
struct Names
{
    std::set<juce::String> used;
    juce::String make (const juce::StringArray& prefixes, const juce::String& noun, int rotate)
    {
        for (int i = 0; i < prefixes.size(); ++i)
        {
            const auto pre = prefixes[(i + rotate) % prefixes.size()];
            bool clash = false;   // never "Breath Breath Pipe" / "Deep Deep Mbira"
            for (auto& w : juce::StringArray::fromTokens (pre, " ", ""))
                if (juce::StringArray::fromTokens (noun.toLowerCase(), " ", "").contains (w.toLowerCase())) clash = true;
            if (clash) continue;
            const auto n = pre.isEmpty() ? noun : pre + " " + noun;
            if (n.length() <= 26 && used.insert (n.toLowerCase()).second) return n;
        }
        return {};
    }
};

struct Cand { juce::String name; int category; std::vector<VaultPart> parts; };

bool isFxKey (const juce::String& k) { return ! k.startsWith ("s.") && k != "attack" && k != "release" && k != "decay" && k != "sustain" && k != "cutoff" && k != "lfoPitch"; }

// sampled key range check: the key C5 of a part (+ extra octaves) lands inside its articulation's zones
bool inRange (const RecipeContext& ctx, const VaultPart& p, int extraOct)
{
    if (p.kind != VaultPartKind::sample) return true;
    if (! ctx.hasInstrument (p.source)) return false;
    const int note = vault::homeNote + 12 * (p.octave + extraOct) + p.semi;
    for (auto& a : ctx.library->find (p.source)->desc.articulations)
        if (a.name == p.articulation)
            for (auto& z : a.zones)
                if (note >= z.lo && note <= z.hi) return true;
    return false;
}

// a treatment on a part list: FX keys on part 0, envelope / filter / vibrato keys on every part of the matching kind,
// plus the role's extra part. Empty = the role does not fit these parts.
std::vector<VaultPart> treat (const RecipeContext& ctx, std::vector<VaultPart> parts, const Treat& t)
{
    if (t.add != addNone)
    {
        if ((int) parts.size() >= vault::maxParts) return {};
        const auto& p0 = parts[0];
        const int sounding = vault::homeNote + 12 * p0.octave + p0.semi;
        switch (t.add)
        {
            case addPad:      parts.push_back (softPad (0.25f, p0.octave)); break;
            case addSub:      if (sounding < 48) return {}; parts.push_back (subSine (0.45f, p0.octave - 1)); break;
            case addBedDrone: if (sounding < 48) return {}; parts.push_back (dronePad (0.25f, p0.octave - 1)); break;
            case addAir:      parts.push_back (lvl (with (synthFlute (0.95f, p0.octave, "Breath Pipe", 0.0f), { { "cutoff", 2400 } }), 0.22f)); break;
            case addOctDown:
            {
                if (p0.kind != VaultPartKind::sample || ! inRange (ctx, p0, -1)) return {};
                auto d = lvl (p0, 0.5f, 0.0f);
                d.octave -= 1;
                parts.push_back (d);
                break;
            }
            case addGlimmer:
            {
                if (p0.kind != VaultPartKind::sample || ! inRange (ctx, p0, 1)) return {};
                auto d = lvl (p0, 0.07f, 0.25f);
                d.octave += 1;
                d.set ("s.cutoff", std::min (d.get ("s.cutoff", 20000.0f), 3500.0f));
                parts.push_back (d);
                break;
            }
            default: break;
        }
    }
    for (auto& [k, v] : t.fx)
    {
        if (isFxKey (k)) { parts[0].set (k, v); continue; }
        for (auto& p : parts)
        {
            if (k.startsWith ("s.") && p.kind == VaultPartKind::sample)
            {
                if (k == "s.cutoff") p.set (k, std::min (v, p.get (k, 20000.0f)));
                else if (k == "s.attack") p.set (k, std::max (v, p.get (k, 0.0f)));
                else p.set (k, v);
            }
            else if (! k.startsWith ("s.") && p.kind == VaultPartKind::synth)
            {
                if (k == "cutoff") p.set (k, std::min (v, p.get (k, 12000.0f)));
                else if (k == "attack") p.set (k, std::max (v, p.get (k, 0.002f)));
                else if (k == "lfoPitch") p.set (k, std::max (v, p.get (k, 0.0f)));
                else p.set (k, v);
            }
        }
    }
    return parts;
}

void addAll (const RecipeContext& ctx, std::vector<Cand>& out)
{
    Names names;
    auto usable = [&] (const std::vector<VaultPart>& parts)
    {
        for (auto& p : parts)
            if (p.kind == VaultPartKind::sample && (! ctx.hasArticulation (p.source, p.articulation) || ! inRange (ctx, p, 0))) return false;
        return true;
    };
    const auto wv = windVoices(), xv = worldVoices();
    const auto wt = windTreats(), xt = worldTreats();
    const auto wl = windLayers(), xl = worldLayers();
    // layers: the roles a doubling is played with (plain, hall, dark, echo, tape, cloud / dream, chorus)
    const std::vector<int> windLayerTreats { 0, 1, 2, 3, 4, 6, 15 }, worldLayerTreats { 0, 1, 2, 3, 4, 9, 15 };

    auto voiceRole = [&] (const Voice& v, const Treat& t, int rot)
    {
        if (v.sustained ? ! t.forSustained : ! t.forShort) return;
        if (t.needsPlainVibrato && v.vibrato) return;
        if (! usable ({ v.part })) return;
        auto parts = treat (ctx, { v.part }, t);
        if (parts.empty()) return;
        const auto name = names.make (t.prefixes, v.noun, rot);
        if (name.isNotEmpty()) out.push_back ({ name, v.category, parts });
    };
    auto layerRole = [&] (const Layer& l, const Treat& t, int rot)
    {
        if (l.sustained ? ! t.forSustained : ! t.forShort) return;
        if (t.needsPlainVibrato || t.add != addNone) return;
        if (! usable (l.parts)) return;
        auto parts = treat (ctx, l.parts, t);
        if (parts.empty()) return;
        const auto name = names.make (t.prefixes, l.noun, rot);
        if (name.isNotEmpty()) out.push_back ({ name, l.category, parts });
    };
    const int nt = (int) std::max (wt.size(), xt.size());
    for (int t = 0; t < nt; ++t)
    {
        if (t < (int) wt.size()) for (int i = 0; i < (int) wv.size(); ++i) voiceRole (wv[(size_t) i], wt[(size_t) t], i);
        if (t < (int) xt.size()) for (int i = 0; i < (int) xv.size(); ++i) voiceRole (xv[(size_t) i], xt[(size_t) t], i);
        if (std::find (windLayerTreats.begin(), windLayerTreats.end(), t) != windLayerTreats.end())
            for (int i = 0; i < (int) wl.size(); ++i) layerRole (wl[(size_t) i], wt[(size_t) t], i);
        if (std::find (worldLayerTreats.begin(), worldLayerTreats.end(), t) != worldLayerTreats.end())
            for (int i = 0; i < (int) xl.size(); ++i) layerRole (xl[(size_t) i], xt[(size_t) t], i);
    }
}

//==============================================================================
// measurement + repair
constexpr float cap25 = 0.07f, cap6k = 0.015f, wantH1 = -2.0f;

struct Meas { float h1 = 0, s25 = 0, s6k = 0, mono = 0, loud = -140; };
Meas measure (const VaultPreset& p, float vel)
{
    Meas m;
    const auto b = recipegate::render (p, vault::homeNote, vel, 1.0, 0.8);
    const auto a = vaultqc::analyse (b, recipegate::sr, 1.0);
    m.loud = a.finite ? a.loud : -140.0f;
    m.mono = a.monoLossDb;
    const auto sp = recipegate::spectrum (b, 0.05);
    const double tot = std::max (1e-20, recipegate::bandPower (sp, 20, 20000));
    m.s25 = (float) (recipegate::bandPower (sp, 2000, 5000) / tot);
    m.s6k = (float) (recipegate::bandPower (sp, 6000, 20000) / tot);
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    const float h1 = recipegate::peakDb (sp, f0 * 0.97, f0 * 1.03);
    float strongest = h1;
    for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, recipegate::peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
    m.h1 = h1 - strongest;
    return m;
}

float partHz (const VaultPart& p) { return 440.0f * std::pow (2.0f, (float) (vault::homeNote + 12 * p.octave + p.semi - 69) / 12.0f); }

// lower every part's low-pass by `factor` (never under 2.5 x its own fundamental / 700 Hz); false = nothing left to lower
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

// part loudness (alone, at C5, no FX) - cached
struct Calib
{
    std::mutex lock;
    std::map<juce::String, float> loud;
    static juce::String key (const VaultPart& p)
    {
        juce::String k;
        k << (int) p.kind << "|" << p.source << "|" << p.articulation << "|" << p.octave << "|" << p.semi;
        auto v = p.values;
        std::sort (v.begin(), v.end());
        for (auto& [a, b] : v) if (a != "gain") k << "|" << a << "=" << b;
        return k;
    }
    float get (const VaultPart& part)
    {
        const auto k = key (part);
        {
            std::lock_guard<std::mutex> g (lock);
            if (auto it = loud.find (k); it != loud.end()) return it->second;
        }
        VaultPreset solo;
        solo.id = "cv.calib"; solo.name = "Calib"; solo.category = vault::catFlutesWoodwinds;
        auto p = part;
        p.level = 1.0f; p.pan = 0.0f;
        std::vector<std::pair<juce::String, float>> keep;
        for (auto& kv : p.values) if (kv.first != "gain") keep.push_back (kv);
        p.values = keep;
        solo.parts = { p };
        const auto b = recipegate::render (solo, vault::homeNote, 0.8f, 1.0, 1.2);
        const auto a = vaultqc::analyse (b, recipegate::sr, 1.0);
        const float l = a.finite ? a.loud : -140.0f;
        std::lock_guard<std::mutex> g (lock);
        loud[k] = l;
        return l;
    }
};

// each part at -20 dB K at C5 (part 0's gain is the FX output gain: the gate normalises the whole preset afterwards)
bool calibrate (VaultPreset& p, Calib& c)
{
    for (auto& part : p.parts)
    {
        const float l = c.get (part);
        if (l < -70.0f) return false;
        part.set ("gain", juce::jlimit (-24.0f, 12.0f, -20.0f - l));
    }
    return true;
}

VaultPart supportPart (const VaultPart& p0)
{
    const int note = vault::homeNote + 12 * p0.octave + p0.semi;
    if (note >= 60 && note <= 96)   // a unison concert flute: the orchestral doubling that carries the fundamental
        return S ("winds.flute.concert", "sustain", "Concert Flute", p0.octave, { { "s.width", 0.6f }, { "s.velCutoff", 0.35f } });
    return synthFlute (0.08f, p0.octave, "Synth Flute", 0.0f);
}

bool repair (VaultPreset& p, Calib& c)
{
    if (! calibrate (p, c)) return false;
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    int support = -1;
    for (int it = 0; it < 18; ++it)
    {
        const auto m8 = measure (p, 0.8f), m10 = measure (p, 1.0f);
        if (m8.loud < -60.0f) return false;
        bool ok = true;
        auto& p0 = p.parts[0];
        if (m8.mono > 2.0f)
        {
            ok = false;
            bool any = false;
            for (auto& part : p.parts)
                if (part.kind == VaultPartKind::sample && part.get ("s.width", 1.0f) > 0.15f) { part.set ("s.width", part.get ("s.width", 1.0f) * 0.6f); any = true; }
            if (! any) { if (p0.get ("width", 0.5f) > 0.2f) p0.set ("width", p0.get ("width", 0.5f) * 0.6f); else return false; }
        }
        if (m10.s6k > cap6k || m8.s6k > cap6k)
        {
            ok = false;
            const float eh = p0.get ("eqHigh", 0.0f);
            if (eh > -12.0f) p0.set ("eqHigh", std::max (-12.0f, eh - 4.0f));
            else if (! lowerCut (p, 0.75f)) return false;
        }
        if (m10.s25 > cap25 || m8.s25 > cap25)
        {
            ok = false;
            if (! lowerCut (p, 0.75f)) return false;
        }
        if (m8.h1 < wantH1)
        {
            ok = false;
            const float el = p0.get ("eqLow", 0.0f);
            if (f0 < 420.0f && el < 9.0f) p0.set ("eqLow", std::min (9.0f, el + 3.0f));
            else if (support < 0)
            {
                // a gentle low-pass first (keeps the instrument's own colour), then the unison support
                bool cut = false;
                for (auto& part : p.parts)
                {
                    const char* key = part.kind == VaultPartKind::sample ? "s.cutoff" : "cutoff";
                    if (part.kind == VaultPartKind::model) continue;
                    const float cur = part.get (key, part.kind == VaultPartKind::sample ? 20000.0f : 12000.0f);
                    const float floor = std::max (900.0f, 4.0f * partHz (part));
                    const float next = std::max (floor, cur > 6000.0f ? 6000.0f : cur * 0.75f);
                    if (next < cur - 1.0f) { part.set (key, next); cut = true; }
                }
                if (! cut)
                {
                    if ((int) p.parts.size() >= vault::maxParts) return false;
                    auto sp = lvl (supportPart (p0), 0.35f);
                    sp.set ("gain", juce::jlimit (-24.0f, 12.0f, -20.0f - c.get (sp)));
                    p.parts.push_back (sp);
                    support = (int) p.parts.size() - 1;
                }
            }
            else
            {
                auto& l = p.parts[(size_t) support].level;
                if (l >= 1.6f) return false;
                l = std::min (1.6f, l * 1.3f);
            }
        }
        if (ok) return true;
    }
    return false;
}
} // namespace

std::vector<VaultPreset> build_winds (const RecipeContext& ctx)
{
    std::vector<Cand> cands;
    addAll (ctx, cands);
    if (const int lim = juce::SystemStats::getEnvironmentVariable ("WINDS_LIMIT", "0").getIntValue(); lim > 0 && lim < (int) cands.size())
        cands.resize ((size_t) lim);
    std::vector<VaultPreset> all;
    int nW = 0, nX = 0;
    for (auto& c : cands)
    {
        VaultPreset p;
        p.category = c.category;
        p.id = c.category == vault::catWorld ? "cv.world." + juce::String (++nX).paddedLeft ('0', 4) : "cv.wind." + juce::String (++nW).paddedLeft ('0', 4);
        p.name = c.name;
        p.parts = c.parts;
        all.push_back (std::move (p));
    }
    Calib calib;
    std::vector<char> keep (all.size(), 0);
    std::atomic<int> next { 0 }, done { 0 };
    std::vector<std::thread> pool;
    const int threads = juce::jmax (1, juce::SystemStats::getEnvironmentVariable ("WINDS_THREADS", "4").getIntValue());
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&]
        {
            for (int i; (i = next++) < (int) all.size();)
            {
                keep[(size_t) i] = repair (all[(size_t) i], calib) ? 1 : 0;
                if (++done % 100 == 0) std::printf ("  repaired %d / %d\n", done.load(), (int) all.size());
            }
        });
    for (auto& t : pool) t.join();
    std::vector<VaultPreset> out;
    int dropped = 0;
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (keep[i]) out.push_back (std::move (all[i]));
        else { ++dropped; std::printf ("  unrepairable: %s %s\n", all[i].id.toRawUTF8(), all[i].name.toRawUTF8()); }
    }
    std::printf ("winds: %d candidates, %d unrepairable\n", (int) all.size(), dropped);

    // own distinctness pass (stricter than the shared -dist): the shared gate analysis of every repaired candidate,
    // then greedy acceptance in priority order with the shared fingerprint distance >= ownMin. Gate failures never
    // block a later candidate. The gate run after this re-checks everything at its own threshold.
    const float ownMin = (float) juce::SystemStats::getEnvironmentVariable ("WINDS_OWN_MIN", "4.8").getDoubleValue();
    if (ownMin <= 0.0f) return out;
    std::vector<recipegate::Result> rs (out.size());
    for (size_t i = 0; i < out.size(); ++i) rs[i].preset = out[i];
    next = 0; done = 0;
    pool.clear();
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&]
        {
            for (int i; (i = next++) < (int) rs.size();)
            {
                recipegate::analyse (rs[(size_t) i]);
                if (++done % 100 == 0) std::printf ("  pre-gate %d / %d\n", done.load(), (int) rs.size());
            }
        });
    for (auto& t : pool) t.join();
    std::vector<int> acc;
    std::map<juce::String, int> fails;
    for (int i = 0; i < (int) rs.size(); ++i)
    {
        if (rs[(size_t) i].fail.isNotEmpty()) { ++fails[rs[(size_t) i].fail.upToFirstOccurrenceOf (":", false, false)]; continue; }
        bool near = false;
        for (int j : acc) if (recipegate::distance (rs[(size_t) i].print, rs[(size_t) j].print) < ownMin) { near = true; break; }
        if (near) { ++fails["near (own " + juce::String (ownMin, 1) + ")"]; continue; }
        acc.push_back (i);
    }
    float minPair = 1.0e9f;
    for (size_t x = 0; x < acc.size(); ++x)
        for (size_t y = x + 1; y < acc.size(); ++y)
            minPair = std::min (minPair, recipegate::distance (rs[(size_t) acc[x]].print, rs[(size_t) acc[y]].print));
    std::printf ("winds own pass: %d of %d kept, min pairwise distance %.2f\n", (int) acc.size(), (int) rs.size(), minPair);
    for (auto& [f, n] : fails) std::printf ("  own pass dropped %-20s %d\n", f.toRawUTF8(), n);
    std::vector<VaultPreset> kept;
    for (int i : acc) kept.push_back (out[(size_t) i]);
    return kept;
}

VAULT_RECIPE (winds, build_winds);
