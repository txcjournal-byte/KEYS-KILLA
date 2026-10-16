// CHROME VAULT recipes: FLUTES & WOODWINDS + WORLD (sound designer "winds"). Run: VaultBankGen -recipe winds (RecipeGate.h).
//
// v3 DESIGN NOTE - every preset ships finished (namespace v3 below)
//  * Research: commercial wind and world instruments ship every patch through an insert chain the player can switch off.
//    Flute / woodwind libraries: close + room perspectives with EQ, compression and reverb built in; producers put a SHORT
//    room or plate on a close flute plus a quiet dotted-8th delay, cut the breathy 2.5-3.5 kHz edge and the lows, and keep
//    compression light so the breath dynamics stay. Sax: light compression (3:1, a few dB), a body push / boxy cut, a
//    1.2 kHz "squeal" cut, plate or room with damped highs. Harmonica: light compression, plate rather than hall; the blues
//    harp's sound is the cupped bullet mic into a small overdriven amp; players add hand tremolo. Zither / guzheng
//    instruments: 6-8 insert slots per mic (EQ, comp, saturation, modulation, algorithmic + convolution spaces) and a
//    concert-hall perspective. Kalimba instruments: EQ, compression, delay, ten reverb types and a lo-fi / tape / saturation
//    section - the lo-fi kalimba (tape wobble, saturation, plate) is the classic beat hook.
//  * So each preset's chain = FAMILY (the instrument's finished sound) + ROLE (the v2 treatment, now a real processing
//    style), slots in the canonical rack order; the v2 repair EQ (comfort) is kept and re-fitted after the FX:
//      flute       light comp, breath cut at 2.8 kHz, plate, quiet 1/8D ping-pong      (macros Air, Space, Vibrato, Ensemble)
//      piccolo     light comp, cut at 3.5 kHz, damped hall                             (Air, Hall, Vibrato, Ensemble)
//      clarinet    light comp, warm low shelf, 1.4 kHz dip, chamber room              (Tone, Room, Vibrato, Ensemble)
//      oboe        light comp, nasal cut at 1.3 kHz, hall;  bassoon: boxy cut at 500 Hz, hall      (Reed, Hall, ...)
//      recorder    chiff transient, cut at 2.5 kHz, baroque room                      (Tone, Room, Vibrato, Chiff)
//      ocarina     soft 2 kHz dip, plate with pre-delay, 1/4 ping-pong                (Tone, Space, Vibrato, Ensemble)
//      sax         3:1 comp, tube warmth with clean lows, squeal cut, plate           (Bite, Space, Vibrato, Grit)
//      harmonica   comp, plate (hand-tremolo LFO ready);  blues harp: amp drive, mid push, short plate  (Tone, ..., Amp)
//      synth winds 1/8D delay, plate, slow filter drift                               (Cutoff, Space, Vibrato, Motion)
//      dan tranh   light comp, pick cut at 2.8 kHz, plate, quiet 1/8D delay           (Brightness, Space, Echo, Chorus)
//      balafon / slit drum  strike transient, comp, wood-box cut, short room, wood body ready  (Mallet, Room, Echo, Wood)
//      thumb piano comp, a whisper of tape, plate, 1/8D delay, tape wobble ready      (Tine, Space, Echo, Tape)
//      mbira       comp, buzz cut at 3.2 kHz, room                                    (Buzz, Room, Echo, Tape)
//      psaltery    hall with pre-delay; bowed: long hall + vibrato;  didgeridoo: comp, tape, slow low-pass
//                  "throat" sweep, room                                               (Tone, Room, Growl, Throat)
//    roles: Distant / Temple = long hall, no delay (macro Distance); Velvet / Felt / Behind the Wall = dark shelf + damped
//    hall / room (Shade); Night = 1/8D ping-pong echo + plate (Repeats); Dusty = tape drive + lo-fi + tape-wobble pitch LFO
//    (Tape); Warped = cloud + deep wobble (Warp); Floating / Dream = cloud reverb + 1/4D delay + width + a slow cutoff / pan
//    drift LFO (Haze, Drift); Swelling / Blooming = hall + drift (Swell); Gritty = tape drive with clean lows (Grit); Puff /
//    Muted = short ping-pong + plate + transient (Wobble: a detune LFO); Wide / Shimmer = ensemble + width (Chorus); Shadow (Ghost),
//    Reverse (Reverse); the production layers (Airy, Twin, Glowing, Heavy, Glimmer, Desert) get a level macro for their
//    added part (Breath, Octave, Pad, Sub, Glimmer, Drone).
//  * Movement: held winds get a delayed (0.3 s), faded-in 5.3 Hz vibrato LFO on pitch (+-8 cents as designed on the straight
//    articulations, 0 on those that already have vibrato; the Vibrato macro and the mod wheel open it to +-22 cents), the
//    bowed psaltery a gentle one; the roles add drift / wobble LFOs. Mod wheel: winds = vibrato, zithers = string vibrato,
//    struck / plucked tongues = a gentle tremolo shimmer, didgeridoo = the throat sweep. Velocity keeps >= 3 dB of range
//    through the compressors.
//  * Macros: every target sits exactly at its designed value at the knob's default, so the preset loads as designed; each
//    macro is rendered at 0 and 1 (C5) and a side that breaks a comfort rule (2-5 kHz, > 6 / 8 kHz, fundamental, clipping,
//    +4 dB louder, a longer tail) is shortened until it passes.
//  * Arp / gate: off everywhere except 5 pattern presets with plain names, appended so every v2 id stays: Thumb Piano
//    Arpeggio (Up-Down 1/16), Dan Tranh Arpeggio (Up 1/16), Balafon Sixteenths (As Played 1/16 with accents), Flute
//    Arpeggio (Up 1/8, short gate), Gated Flute Choir (1/8 gate, 3 steps closed). Patterns start from Init.
//  * Selection: the shipped v2 presets are accepted first (ids and names stay), then the other candidates; the own
//    distinctness threshold stays 4.8 (WINDS_OWN_MIN) and the assembly QA limits (8 kHz share, centroid, chord headroom)
//    are checked here, so the bank assembly keeps what this recipe returns.
//    A shipped preset that comes out too close is designed again (1: role in front without the family space / echo,
//    2: its v2 colours with the v3 movement and macros) before it is dropped. WINDS_V2_CACHE=<dir> keeps the v2 stage
//    between runs, WINDS_ONLY=<ids> designs only those presets, WINDS_DUMP=<file> lists every chain / LFO / macro.
//  * Result (VaultBankGen -recipe winds, library /home/user/cv-library): 1807 repaired candidates -> 463 presets
//    (Flutes & Woodwinds 249, World 214), all pass the shared gate; 407 of the 441 shipped ids kept with their names
//    (426 with the design, 4 role-forward, 33 with their v2 colours), 5 pattern presets (4 arp, 1 gate); min pairwise
//    distance 4.80 (4.20 for Gated Flute Choir vs Choir of Flutes, its rhythm version, >= the shared 3.5). At C5:
//    fundamental >= -2.3 dB of the strongest partial, 2-5 kHz <= 7.0 %, > 6 kHz <= 0.93 %, > 8 kHz <= 0.59 %; 71 presets
//    needed a comfort re-fit after the FX. 1,852 macros checked at 0 and 1: 379 ranges narrowed, none left outside the
//    comfort rules, 77 (mostly Echo / Room / Ensemble on dark or long-ringing sounds) change the sound only slightly.
//  * Sources: loststoriesacademy.com "How to use flute samples in modern music production"; musictech / Computer Music
//    "synth flute" walkthrough (EQ - compression - room - ping-pong); flypaper.soundfly.com "How to temper the mighty
//    squeal of a saxophone"; sax.co.uk "In the mix"; recording.org harmonica threads; wavesfactory / soniccouture /
//    impactsoundworks guzheng instrument pages (insert slots, hall perspective); bedroomproducersblog ESW Kalimba review,
//    originsofaudio Dark Kalimba, Kalimba Keys (tape delay, lo-fi, saturation sections).
//
// v2 DESIGN NOTE (research summary; sources at the end of this note)
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

// what a candidate is (drives the v3 processing): instrument family, role (the treatment's first prefix, "plain" for the
// close studio sound), layered or not, sustained or short, articulation with its own vibrato, rhythm pattern (arp / gate)
struct Role
{
    juce::String fam, role = "plain";
    bool layer = false, sustained = true, vib = false;
    int pattern = 0;
};
struct Cand { juce::String name; int category; std::vector<VaultPart> parts; Role role; };

juce::String famOf (const VaultPart& p)
{
    if (p.kind == VaultPartKind::synth) return "synth";
    const auto& s = p.source;
    if (s == "winds.piccolo") return "piccolo";
    if (s.startsWith ("winds.flute")) return "flute";
    if (s == "winds.clarinet") return "clarinet";
    if (s == "winds.oboe") return "oboe";
    if (s == "winds.bassoon") return "bassoon";
    if (s.startsWith ("winds.recorder")) return "recorder";
    if (s.startsWith ("winds.ocarina")) return "ocarina";
    if (s.startsWith ("winds.sax")) return "sax";
    if (s == "winds.harmonica.chromatic") return "harmonica";
    if (s.startsWith ("winds.harmonica")) return "bluesharp";
    if (s == "world.dantranh") return "zither";
    if (s == "world.balafon") return "balafon";
    if (s.startsWith ("world.kalimba")) return "kalimba";
    if (s.startsWith ("world.mbira")) return "mbira";
    if (s == "world.slitdrum") return "slitdrum";
    if (s == "pluck.psaltery") return p.articulation == "bowed" ? "bowedpsaltery" : "psaltery";
    if (s == "world.didgeridoo") return "didge";
    if (s.startsWith ("pluck.harp")) return "harp";
    return "other";
}
juce::String roleOf (const Treat& t) { return t.prefixes[0].isEmpty() ? juce::String ("plain") : t.prefixes[0]; }
bool vibArt (const VaultPart& p) { return p.kind == VaultPartKind::synth || p.articulation.containsIgnoreCase ("vibrato"); }

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
        if (name.isNotEmpty()) out.push_back ({ name, v.category, parts, { famOf (v.part), roleOf (t), false, v.sustained, v.vibrato } });
    };
    auto layerRole = [&] (const Layer& l, const Treat& t, int rot)
    {
        if (l.sustained ? ! t.forSustained : ! t.forShort) return;
        if (t.needsPlainVibrato || t.add != addNone) return;
        if (! usable (l.parts)) return;
        auto parts = treat (ctx, l.parts, t);
        if (parts.empty()) return;
        const auto name = names.make (t.prefixes, l.noun, rot);
        if (name.isNotEmpty()) out.push_back ({ name, l.category, parts, { famOf (l.parts[0]), roleOf (t), true, l.sustained, vibArt (l.parts[0]) } });
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

    // v3: the few presets where a rhythm is the point of the sound (arp / gate on, plain musical names). Appended last so
    // the ids of every v2 candidate stay; they get the close studio role (its v2 FX keys) like every plain preset.
    auto pat = [&] (const char* name, int cat, std::vector<VaultPart> parts, bool sus, int pattern)
    {
        if (! usable (parts)) return;
        parts = treat (ctx, parts, cat == vault::catWorld ? xt[0] : wt[0]);
        if (names.used.insert (juce::String (name).toLowerCase()).second)
            out.push_back ({ name, cat, parts, { famOf (parts[0]), "plain", parts.size() > 1, sus, vibArt (parts[0]), pattern } });
    };
    const KV dark { { "s.velCutoff", 0.3f } }, soft { { "s.velCutoff", 0.3f } }, fl { { "s.width", 0.6f }, { "s.velCutoff", 0.35f } };
    pat ("Thumb Piano Arpeggio", vault::catWorld, { S ("world.kalimba", "pluck", "Thumb Piano") }, false, 1);
    pat ("Dan Tranh Arpeggio", vault::catWorld, { S ("world.dantranh", "pluck", "Dan Tranh", 0, dark) }, false, 2);
    pat ("Balafon Sixteenths", vault::catWorld, { S ("world.balafon", "soft mallet", "Balafon", 0, { { "s.width", 0.5f } }) }, false, 3);
    pat ("Flute Arpeggio", vault::catFlutesWoodwinds, { S ("winds.flute.concert", "staccato", "Concert Flute", 1, fl),
                                                           lvl (S ("winds.clarinet", "staccato", "Clarinet", 1, soft), 0.55f, -0.3f) }, false, 4);
    pat ("Gated Flute Choir", vault::catFlutesWoodwinds, { S ("winds.flute.concert", "sustain", "Concert Flute", 0, fl),
                                                           lvl (S ("winds.recorder.tenor", "sustain", "Tenor Recorder", 0, soft), 0.5f, -0.35f),
                                                           lvl (S ("winds.recorder.bass", "sustain", "Bass Recorder", 0, soft), 0.45f, 0.35f) }, true, 5);
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

//==============================================================================
// v3 SOUND DESIGN (see the v3 design note at the top): FX chain by family + role, movement, 4 named sound macros,
// arp / gate on the few pattern presets, comfort re-fit on the finished sound, macro extremes checked
namespace v3
{
using namespace recipe;
using FT = vault::FxType;
using R = fx::Reverb;

int envInt (const char* k, int def) { auto v = juce::SystemStats::getEnvironmentVariable (k, {}); return v.isEmpty() ? def : v.getIntValue(); }

// the chain, one slot per type, kept in the canonical (v2 rack) order
struct Chain
{
    std::map<int, VaultFxSlot> slots;   // rank -> slot
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
        for (auto t : { FT::transient, FT::width, FT::compressor, FT::flanger })   // at most 8 slots: the least important go first
            if (v.size() > (size_t) vault::maxFxSlots)
                for (auto it = v.begin(); it != v.end(); ++it) if (it->type == t) { v.erase (it); break; }
        if (v.size() > (size_t) vault::maxFxSlots) v.resize ((size_t) vault::maxFxSlots);
        return v;
    }
};

VaultFxSlot rv (R type, float size, float decay, float mix, float predelay, float damping, float lowCut = 160.0f)
{
    return fx::reverb (type, size, decay, mix).set ("predelay", predelay).set ("damping", damping).set ("lowCut", lowCut);
}

// a sound macro whose knob default reproduces the designed sound exactly: every target has its designed value at the
// knob's default d and moves `span` in total (span x d below it at 0, span x (1 - d) above it at 1)
struct M
{
    VaultMacro m;
    float d;
    M (const char* name, float dflt) : d (dflt) { m.name = name; }
    M& rel (const char* target, float span) { m.targets.push_back ({ target, -span * d, span * (1.0f - d), 0.0f }); return *this; }
    M& abs (const char* target, float v, float span, float lo, float hi)
    {
        if (d > 1.0e-4f) span = std::min (span, (v - lo) / d);
        if (d < 1.0f - 1.0e-4f) span = std::min (span, (hi - v) / (1.0f - d));
        span = std::max (0.0f, span);
        m.targets.push_back ({ target, v - span * d, v + span * (1.0f - d), 0.0f });
        return *this;
    }
    // designed value v at the default, `atMax` at 1 (e.g. a drive's output trim that keeps the level as the drive goes up)
    M& toMax (const char* target, float v, float atMax)
    {
        const float span = d < 1.0f - 1.0e-4f ? (atMax - v) / (1.0f - d) : 0.0f;
        m.targets.push_back ({ target, v - span * d, atMax, 0.0f });
        return *this;
    }
    M& fxp (const Chain& c, FT t, const char* id, float span)
    {
        if (! c.has (t)) return *this;   // a target into a missing slot would be dropped by the engine
        const auto& ti = vault::fxTypeInfo (t);
        const auto& pi = ti.params[(size_t) ti.indexOf (id)];
        const auto target = juce::String ("fx.") + ti.id + "." + id;
        return abs (target.toRawUTF8(), c.get (t, id), span, pi.lo, pi.hi);
    }
};
void setMacro (VaultPreset& p, int i, const M& m) { p.macros[(size_t) i] = m.m; p.macroDefaults[i] = m.d; }

// LFO roles
VaultLfo vibrato (float depth) { return lfo (VaultLfo::sine).rate (5.3f).delay (0.3f).fade (0.7f).depth (depth).to ("pitch", 0.22f); }
// (the tilt keeps the drift audible on samples played without a low-pass)
VaultLfo drift (float depth, float rate = 0.13f) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", 0.3f).to ("pan", 0.25f).to ("fx.eq.tilt", 0.08f); }
VaultLfo wobble (float depth, float rate = 0.55f) { return lfo (VaultLfo::triangle).rate (rate).free().depth (depth).to ("pitch", 0.09f); }
VaultLfo handTremolo() { return lfo (VaultLfo::sine).rate (4.8f).depth (0.0f).to ("volume", 0.5f); }

bool isWind (const juce::String& f)
{
    for (auto* w : { "flute", "piccolo", "clarinet", "oboe", "bassoon", "recorder", "ocarina", "sax", "harmonica", "bluesharp", "synth" }) if (f == w) return true;
    return false;
}
bool lfoTo (const VaultLfo& l, const char* target) { for (auto& d : l.dests) if (d.target == target) return true; return false; }

//------------------------------------------------------------------------------ 1. the chain and the movement
// variant 0 = family polish + role (the design); a preset that came out too close to an earlier one is designed again:
// 1 = the role in front (no family space / echo, the role's own space), 2 = the v2 colours only (+ movement, macros)
void designSound (VaultPreset& p, const Role& r, int variant = 0)
{
    design (p);   // v2 -> v3: the role's colour slots (chorus, lo-fi, reverse, ghost, width) and the repair's EQ arrive here
    Chain c (p.fx);
    p.mod = {};
    p.arp = {};
    p.gate = {};
    const auto& f = r.fam;
    const auto& t = r.role;
    const bool wind = isWind (f);
    float finish = 0.08f;
    auto mid = [&] (float db, float hz, float q) { auto& e = c.at (FT::eq); e.set ("mid", db).set ("midFreq", hz).set ("midQ", q); };
    auto comp = [&] (float th, float ra, float at, float re, float mk, float mix) { c.put (fx::comp (th, ra, at, re, mk, mix)); };
    auto dly = [&] (const char* div, float fb, float mix, fx::Delay mode = fx::Delay::pingPong, float hi = 4500.0f, float lo = 350.0f)
    { c.put (fx::delay (div, fb, mix, mode).set ("hiCut", hi).set ("loCut", lo)); };
    // variant 1: the family keeps its tone (comp, EQ, drive) but not its space / echo
    const std::vector<VaultFxSlot> converted = p.fx;
    auto famSpace = [&] (VaultFxSlot s) { if (variant == 0) c.put (s); };
    auto famDly = [&] (const char* div, float fb, float mix, fx::Delay mode = fx::Delay::pingPong, float hi = 4500.0f, float lo = 350.0f)
    { if (variant == 0) dly (div, fb, mix, mode, hi, lo); };

    // the instrument's finished sound (what a commercial patch puts on the close mics)
    if (f == "flute" || f == "piccolo")
    {
        comp (-24, 2.0f, 20, 180, 1.0f, 0.5f);                                        // evens the breath swells, keeps the touch
        mid (-1.5f, f == "piccolo" ? 3500.0f : 2800.0f, 0.9f);                         // the breathy edge
        if (f == "piccolo") famSpace (rv (R::hall, 0.55f, 1.0f, 0.18f, 20, 0.45f, 250));
        else
        {
            famSpace (rv (R::plate, 0.45f, 1.0f, 0.16f, 18, 0.3f, 250));
            famDly ("1/8D", 0.25f, 0.06f, fx::Delay::pingPong, 4000, 450);                  // the quiet contemporary echo
        }
    }
    else if (f == "clarinet") { comp (-24, 1.8f, 25, 200, 0.5f, 0.5f); c.eqAdd ("low", 0.5f); mid (-1.0f, 1400, 0.8f); famSpace (rv (R::room, 0.5f, 1.1f, 0.17f, 12, 0.35f, 200)); }
    else if (f == "oboe") { comp (-24, 2.0f, 20, 180, 0.5f, 0.5f); mid (-2.0f, 1300, 1.0f); famSpace (rv (R::hall, 0.55f, 1.0f, 0.18f, 20, 0.35f, 250)); }
    else if (f == "bassoon") { comp (-24, 2.0f, 25, 200, 0.5f, 0.5f); mid (-1.5f, 500, 0.8f); c.eqAdd ("high", -1.0f); famSpace (rv (R::hall, 0.55f, 1.0f, 0.16f, 15, 0.4f, 160)); }
    else if (f == "recorder") { c.put (fx::transient (0.08f)); mid (-1.5f, 2500, 0.9f); famSpace (rv (R::room, 0.45f, 1.0f, 0.17f, 8, 0.3f, 200)); }
    else if (f == "ocarina")
    {
        mid (-1.0f, 2000, 0.8f);
        famSpace (rv (R::plate, 0.5f, 1.1f, 0.18f, 25, 0.3f, 250));
        famDly ("1/4", 0.3f, 0.07f, fx::Delay::pingPong, 4000, 400);
    }
    else if (f == "sax")
    {
        comp (-22, 3.0f, 12, 140, 1.5f, 0.75f);
        c.put (fx::drive (fx::Drive::tube, 0.06f, 0.5f, 6000).set ("cleanLow", 1.0f));
        mid (-1.5f, 1200, 1.0f); c.eqAdd ("high", -1.0f);                               // the nasal squeal
        famSpace (rv (R::plate, 0.5f, 1.0f, 0.15f, 15, 0.3f, 200));
    }
    else if (f == "harmonica") { comp (-22, 2.5f, 15, 150, 1.0f, 0.6f); mid (-1.5f, 1500, 0.9f); famSpace (rv (R::plate, 0.4f, 0.9f, 0.14f, 10, 0.35f, 220)); }
    else if (f == "bluesharp")
    {
        comp (-20, 3.0f, 10, 120, 1.5f, 0.7f);
        c.put (fx::drive (fx::Drive::tube, 0.12f, 0.6f, 4000).set ("cleanLow", 1.0f));   // the cupped "bullet" mic into a small amp
        mid (1.0f, 900, 0.7f); c.eqAdd ("high", -2.0f);
        famSpace (rv (R::plate, 0.35f, 0.7f, 0.13f, 6, 0.4f, 250));                      // short, spring-like
    }
    else if (f == "synth") { famDly ("1/8D", 0.3f, 0.1f); famSpace (rv (R::plate, 0.5f, 1.0f, 0.18f, 20, 0.3f, 250)); }
    else if (f == "zither")
    {
        comp (-24, 2.0f, 15, 160, 1.0f, 0.5f);
        mid (-1.5f, 2800, 1.0f);
        famSpace (rv (R::plate, 0.5f, 1.1f, 0.18f, 20, 0.25f, 250));
        famDly ("1/8D", 0.22f, 0.06f, fx::Delay::pingPong, 4000, 400);
    }
    else if (f == "balafon") { c.put (fx::transient (0.0f)); comp (-22, 2.5f, 8, 120, 1.0f, 0.6f); mid (-1.5f, 450, 0.9f); famSpace (rv (R::room, 0.4f, 0.9f, 0.14f, 6, 0.3f, 200)); }
    else if (f == "kalimba")
    {
        comp (-24, 2.5f, 10, 150, 1.0f, 0.6f);
        c.put (fx::drive (fx::Drive::tape, 0.06f, 0.5f, 7000));
        c.eqAdd ("low", 0.5f); c.eqAdd ("high", -1.0f);
        famSpace (rv (R::plate, 0.45f, 1.0f, 0.17f, 15, 0.3f, 250));
        famDly ("1/8D", 0.25f, 0.06f, fx::Delay::pingPong, 4000, 400);
    }
    else if (f == "mbira") { comp (-24, 2.0f, 10, 150, 1.0f, 0.5f); mid (-2.0f, 3200, 1.0f); famSpace (rv (R::room, 0.45f, 1.0f, 0.15f, 8, 0.4f, 200)); }
    else if (f == "slitdrum") { c.put (fx::transient (0.05f)); comp (-20, 3.0f, 5, 120, 1.5f, 0.7f); c.eqAdd ("low", 1.0f); famSpace (rv (R::room, 0.35f, 0.8f, 0.12f, 4, 0.4f, 150)); finish = 0.1f; }
    else if (f == "psaltery") { comp (-24, 2.0f, 15, 160, 0.5f, 0.4f); mid (-1.5f, 3000, 1.0f); famSpace (rv (R::hall, 0.6f, 1.1f, 0.2f, 25, 0.3f, 250)); }
    else if (f == "bowedpsaltery") { famSpace (rv (R::hall, 0.65f, 1.2f, 0.22f, 30, 0.35f, 250)); }
    else if (f == "didge")
    {
        comp (-20, 2.5f, 20, 200, 1.0f, 0.6f);
        c.put (fx::drive (fx::Drive::tape, 0.08f, 0.5f, 5000).set ("cleanLow", 1.0f));
        c.put (fx::filter (fx::Filter::lowPass, 1800, 0.25f, 0.0f, 0.2f, 0.3f));       // the slow "throat" sweep of a drone
        famSpace (rv (R::room, 0.45f, 1.0f, 0.14f, 10, 0.35f, 120));
    }
    else if (f == "harp") famSpace (rv (R::hall, 0.6f, 1.1f, 0.18f, 20, 0.25f, 250));
    else famSpace (rv (R::plate, 0.45f, 1.0f, 0.15f, 15, 0.3f, 250));

    // the role: spaces and colours
    bool driftOn = false, wobbleOn = false;
    float wobbleDepth = 0.5f;
    if (t == "Distant" || t == "Temple") { c.put (rv (R::hall, 0.75f, 1.25f, 0.3f, 12, 0.35f, 250)); c.drop (FT::delay); }
    else if (t == "Velvet" || t == "Felt") { c.at (FT::eq).set ("highFreq", 4000.0f); c.drop (FT::drive); c.put (rv (R::hall, 0.55f, 1.0f, 0.2f, 18, 0.55f, 220)); }
    else if (t == "Night") { dly ("1/8D", 0.42f, 0.26f, fx::Delay::pingPong, 4000, 300); c.put (rv (R::plate, 0.5f, 1.0f, 0.13f, 12, 0.3f, 250)); }
    else if (t == "Dusty")
    {
        c.put (fx::drive (fx::Drive::tape, 0.15f, 0.7f, 5000));
        c.at (FT::eq).set ("highFreq", 4500.0f);
        c.put (rv (R::room, 0.4f, 0.9f, 0.13f, 6, 0.5f, 200));
        c.drop (FT::delay);
        wobbleOn = true;
    }
    else if (t == "Behind the Wall") { c.put (rv (R::room, 0.45f, 1.0f, 0.22f, 10, 0.6f, 200)); c.drop (FT::delay); c.drop (FT::chorus); }
    else if (t == "Floating" || t == "Dream")
    {
        c.put (rv (R::cloud, 0.55f, 0.9f, 0.3f, 25, 0.35f, 220));
        dly ("1/4D", 0.3f, 0.1f, fx::Delay::pingPong, 3500, 350);
        c.put (fx::width (0.66f, 150));
        driftOn = true;
    }
    else if (t == "Puff" || t == "Muted")
    {
        dly ("1/8", 0.25f, 0.12f, fx::Delay::pingPong, 4000, 350);
        c.put (rv (R::plate, 0.4f, 0.9f, 0.14f, 5, 0.3f, 250));
        if (! c.has (FT::transient)) c.put (fx::transient (0.1f));
    }
    else if (t == "Swelling" || t == "Blooming") { c.put (rv (R::hall, 0.6f, 1.1f, 0.24f, 20, 0.35f, 250)); driftOn = true; }
    else if (t == "Gritty")
    {
        c.put (fx::drive (fx::Drive::tape, 0.3f, 0.8f, 4500).set ("cleanLow", 1.0f).set ("output", -2.0f));
        c.put (rv (R::room, 0.4f, 0.9f, 0.12f, 6, 0.4f, 200));
        c.drop (FT::delay);
    }
    else if (t == "Reverse") { c.put (rv (R::hall, 0.6f, 1.1f, 0.18f, 15, 0.3f, 250)); c.drop (FT::delay); }
    else if (t == "Airy") c.put (rv (R::plate, 0.5f, 1.1f, 0.18f, 20, 0.3f, 250));
    else if (t == "Glowing") { c.put (rv (R::hall, 0.6f, 1.1f, 0.22f, 20, 0.3f, 250)); driftOn = true; }
    else if (t == "Desert") { c.put (rv (R::hall, 0.6f, 1.1f, 0.2f, 20, 0.35f, 250)); driftOn = true; }
    else if (t == "Singing") c.put (rv (R::hall, 0.55f, 1.1f, 0.2f, 20, 0.3f, 250));
    else if (t == "Wide" || t == "Shimmer") { c.put (fx::ensemble (0.5f, 0.45f, 0.55f)); c.put (fx::width (0.75f, 150)); c.put (rv (R::plate, 0.5f, 1.0f, 0.15f, 14, 0.25f, 250)); }
    else if (t == "Shadow") c.put (rv (R::hall, 0.6f, 1.1f, 0.15f, 18, 0.3f, 250));
    else if (t == "Warped") { c.put (rv (R::cloud, 0.45f, 0.9f, 0.2f, 15, 0.5f, 220)); c.drop (FT::delay); wobbleOn = true; wobbleDepth = 0.75f; }
    else if (t == "Glimmer") c.put (rv (R::plate, 0.5f, 1.0f, 0.18f, 18, 0.25f, 250));
    // Twin, Heavy: the family's space

    // variant 2: the v2 colours only (the converted chain), the movement and macros below still apply
    if (variant == 2) c = Chain (converted);

    // movement. LFO 1 = the player's vibrato (delayed, faded in; the vibrato articulations already have their own, so
    // theirs starts at 0 for the Vibrato macro / mod wheel), LFO 2 = the role's drift / tape wobble or the family's colour
    const bool sampleWind = wind && f != "synth" && p.parts.front().kind == VaultPartKind::sample;
    if (wind && r.sustained && r.pattern == 0) p.mod.lfo[0] = vibrato (sampleWind && ! r.vib && t != "Singing" ? 0.35f : 0.0f);
    else if (f == "bowedpsaltery") p.mod.lfo[0] = vibrato (0.25f);
    if (t == "Puff" || t == "Muted") p.mod.lfo[1] = wobble (0.0f, 0.7f);   // short notes: a detune wobble for the Wobble macro
    else if (driftOn) p.mod.lfo[1] = drift (0.5f);
    else if (wobbleOn) p.mod.lfo[1] = wobble (wobbleDepth);
    else if (f == "harmonica" || f == "bluesharp") p.mod.lfo[1] = handTremolo();
    else if (f == "kalimba" || f == "mbira") p.mod.lfo[1] = wobble (0.0f);
    else if (f == "synth") p.mod.lfo[1] = drift (0.4f, 0.11f);

    // the rhythm presets (patterns start from Init: only mode / rate / gate / accents change)
    if (r.pattern == 1) p.arp = arp (VaultArp::upDown, "1/16", 1, 0.7f, 0.0f, true);
    else if (r.pattern == 2) p.arp = arp (VaultArp::up, "1/16", 1, 0.8f, 0.0f, true);
    else if (r.pattern == 3)
    {
        for (auto& part : p.parts) if (part.kind == VaultPartKind::sample) part.set ("s.cutoff", std::min (2200.0f, part.get ("s.cutoff", 20000.0f)));   // 16 strikes a bar: softer
        p.arp = arp (VaultArp::asPlayed, "1/16", 1, 0.6f, 0.0f, true);
        const float acc[16] { 1, 0.6f, 0.75f, 0.6f, 1, 0.6f, 0.75f, 0.85f, 1, 0.6f, 0.75f, 0.6f, 1, 0.65f, 0.8f, 0.7f };
        for (int i = 0; i < 16; ++i) p.arp.vel[(size_t) i] = acc[i];
    }
    else if (r.pattern == 4) p.arp = arp (VaultArp::up, "1/8", 1, 0.5f, 0.0f, true);
    else if (r.pattern == 5) p.gate = gate ("1/8", { 1, 0, 1, 0, 1, 1, 0, 1 }, 1.0f, 0.2f, 1.0f, true);

    // performance: mod wheel = vibrato (winds: the vibrato LFO; strings of the zither family: bends of the built-in
    // vibrato; struck / plucked tongues: a gentle tremolo shimmer; drone: the throat sweep), velocity keeps its range
    // through the compressor
    if (wind && lfoTo (p.mod.lfo[0], "pitch")) p.mod.modWheel.push_back ({ "lfo1.depth", 0.8f });
    else if (wind || f == "zither" || f == "psaltery" || f == "bowedpsaltery" || f == "harp") p.mod.modWheel.push_back ({ "quick.vibrato", 0.35f });
    else if (f == "didge") p.mod.modWheel.push_back ({ "fx.filter.lfo", 0.4f });
    else p.mod.modWheel.push_back ({ "quick.tremolo", 0.45f });
    if (c.has (FT::compressor) || c.has (FT::drive)) p.mod.velocity.push_back ({ "volume", r.sustained ? 0.4f : 0.6f });

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

// the FX added highs (drive) or moved the balance: pull the top shelf / its corner, the 2-5 kHz band and the part filters
// back under the caps of the v2 repair, and keep the fundamental on top with a low shelf placed over it
int fitComfort (VaultPreset& p)
{
    const float ceil = recipegate::centroidCeiling (p.category);
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    int moves = 0;
    for (int it = 0; it < 10; ++it)
    {
        const auto a = comfortOf (p, 0.8f), b = comfortOf (p, 1.0f);
        bool ok = true;
        if (std::max (a.s6, b.s6) > cap6k || std::max (a.s8, b.s8) > 0.006f || a.centroid > 0.9f * ceil)
        {
            ok = false;
            if (p.fx.size() >= (size_t) vault::maxFxSlots && slotOf (p, FT::eq) == nullptr) { if (! lowerCut (p, 0.8f)) break; }
            else
            {
                if (slotOf (p, FT::eq) == nullptr) { recipe::design (p).addFx (VaultFxSlot::make (FT::eq)); }
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
        else if (a.h1 < wantH1 - 0.5f)
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
juce::String toneName (const juce::String& f)
{
    if (f == "flute" || f == "piccolo") return "Air";
    if (f == "oboe" || f == "bassoon") return "Reed";
    if (f == "sax") return "Bite";
    if (f == "synth") return "Cutoff";
    if (f == "zither" || f == "psaltery" || f == "bowedpsaltery" || f == "harp") return "Brightness";
    if (f == "balafon" || f == "slitdrum") return "Mallet";
    if (f == "kalimba") return "Tine";
    if (f == "mbira") return "Buzz";
    return "Tone";
}
void designMacros (VaultPreset& p, const Role& r)
{
    const auto& f = r.fam;
    const auto& t = r.role;
    const bool wind = isWind (f);
    const bool sample = p.parts.front().kind == VaultPartKind::sample;
    {   // the silent slots some macros open (only while the chain has room)
        Chain c (p.fx);
        const bool echoMacro = ! (wind && r.sustained) && f != "didge" && f != "bowedpsaltery";
        auto room = [&] { return c.slots.size() < (size_t) vault::maxFxSlots; };
        if (echoMacro && ! c.has (FT::delay) && room()) c.put (fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 4000.0f).set ("loCut", 350.0f));
        if ((f == "balafon" || f == "slitdrum") && ! c.has (FT::body) && room()) c.put (VaultFxSlot::make (FT::body).set ("model", 5.0f).set ("mix", 0.0f));
        if (! c.has (FT::chorus) && room() && (f == "flute" || f == "piccolo" || f == "clarinet" || f == "oboe" || f == "bassoon" || f == "ocarina"
                                                 || f == "zither" || f == "psaltery" || f == "harp"))
            c.put (fx::chorus (0.0f, 0.4f, 0.45f, f != "zither" && f != "psaltery" && f != "harp"));
        if ((f == "kalimba" || f == "mbira" || f == "other") && ! c.has (FT::drive) && room()) c.put (fx::drive (fx::Drive::tape, 0.0f, 0.0f, 7000));
        p.fx = c.build();
    }
    Chain c (p.fx);
    for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; }

    // 1. tone: the part filters + the EQ tilt (mallets also the strike, the drone its filter)
    {
        const auto nm = toneName (f);
        M m (nm.toRawUTF8(), 0.6f);
        if (f == "didge") { m = M ("Tone", 0.5f); m.fxp (c, FT::filter, "cutoff", 2400.0f).fxp (c, FT::eq, "tilt", 4.0f); }
        else
        {
            m.rel ("cutoff", sample ? 4.0f : 2.5f).fxp (c, FT::eq, "tilt", 6.0f);
            if (nm == "Mallet") m.fxp (c, FT::transient, "attack", 0.6f);
            if (nm == "Bite") m.fxp (c, FT::drive, "amount", 0.3f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -3.0f);
        }
        setMacro (p, 0, m);
    }
    // 2. space: the reverb (+ the delay on held winds), named after the room
    {
        const int type = (int) c.get (FT::reverb, "type");
        const char* nm = t == "Distant" || t == "Temple" ? "Distance" : type == (int) R::cloud ? "Haze" : type == (int) R::hall ? "Hall"
                       : type == (int) R::room ? "Room" : "Space";
        M m (nm, 0.4f);
        const bool room = type == (int) R::room;   // a short room needs a longer throw to be heard
        m.fxp (c, FT::reverb, "mix", room ? 0.6f : 0.5f).fxp (c, FT::reverb, "decay", room ? 1.8f : 1.2f).fxp (c, FT::reverb, "size", 0.5f);
        if (wind && r.sustained) m.fxp (c, FT::delay, "mix", 0.25f);
        setMacro (p, 1, m);
    }
    // 3. expression: vibrato on held winds, the echo throw on plucks and short notes, the drone's growl
    {
        if (lfoTo (p.mod.lfo[0], "pitch"))
        {
            const float d0 = p.mod.lfo[0].depth;
            setMacro (p, 2, M ("Vibrato", d0).abs ("lfo1.depth", d0, 1.0f, 0.0f, 1.0f));
        }
        else if (f == "didge")
            setMacro (p, 2, M ("Growl", 0.25f).fxp (c, FT::drive, "amount", 0.45f).fxp (c, FT::filter, "reso", 0.4f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -4.0f));
        else if (c.has (FT::delay))
        {
            const float mix = c.get (FT::delay, "mix");
            M m ("Echo", mix < 0.001f ? 0.0f : 0.3f);
            m.fxp (c, FT::delay, "mix", 0.45f).fxp (c, FT::delay, "feedback", 0.45f).fxp (c, FT::delay, "hiCut", 4000.0f);
            setMacro (p, 2, m);
        }
        else setMacro (p, 2, M ("Echo", 0.0f).rel ("quick.delay", 0.4f));
    }
    // 4. the role's character, else the family's
    {
        auto layerMacro = [&] (const char* nm) -> bool
        {
            if (p.parts.size() < 2) return false;
            const float l = p.parts[1].level;
            setMacro (p, 3, M (nm, 0.5f).abs ("layer2.level", l, 2.0f * l, 0.0f, 2.0f));
            return true;
        };
        bool done = true;
        if (t == "Dusty")
            setMacro (p, 3, M ("Tape", 0.5f).abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1).fxp (c, FT::lofi, "wow", 0.6f).fxp (c, FT::drive, "amount", 0.3f)
                                           .toMax ("fx.drive.output", c.get (FT::drive, "output"), -2.5f));
        else if (t == "Warped") setMacro (p, 3, M ("Warp", 0.6f).abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1).fxp (c, FT::lofi, "wow", 0.6f));
        else if ((t == "Wide" || t == "Shimmer") && c.has (FT::chorus)) setMacro (p, 3, M ("Chorus", 0.5f).fxp (c, FT::chorus, "mix", 0.8f).fxp (c, FT::chorus, "depth", 0.6f));
        else if (t == "Shadow" && c.has (FT::ghost)) setMacro (p, 3, M ("Ghost", 0.5f).fxp (c, FT::ghost, "mix", 0.8f).fxp (c, FT::ghost, "blur", 0.6f));
        else if (t == "Reverse" && c.has (FT::reverse)) setMacro (p, 3, M ("Reverse", 0.5f).fxp (c, FT::reverse, "mix", 0.8f));
        else if (t == "Floating" || t == "Dream") setMacro (p, 3, M ("Drift", 0.5f).abs ("lfo2.depth", 0.5f, 1.0f, 0, 1).fxp (c, FT::width, "width", 0.4f));
        else if (t == "Swelling" || t == "Blooming") setMacro (p, 3, M ("Swell", 0.5f).rel ("quick.attack", 0.4f).abs ("lfo2.depth", 0.5f, 1.0f, 0, 1));
        // (the dark roles low-pass the parts, so a top shelf alone would not be heard: the macro moves the part filters)
        else if (t == "Velvet" || t == "Felt" || t == "Behind the Wall") setMacro (p, 3, M ("Shade", 0.5f).rel ("cutoff", -2.0f).fxp (c, FT::eq, "high", 6.0f).fxp (c, FT::reverb, "damping", 0.8f));
        else if (t == "Gritty" && c.has (FT::drive)) setMacro (p, 3, M ("Grit", 0.4f).fxp (c, FT::drive, "amount", 0.45f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -5.0f));
        else if (t == "Night" && c.has (FT::delay)) setMacro (p, 3, M ("Repeats", 0.5f).fxp (c, FT::delay, "feedback", 0.45f));
        else if ((t == "Puff" || t == "Muted") && lfoTo (p.mod.lfo[1], "pitch")) setMacro (p, 3, M ("Wobble", 0.0f).abs ("lfo2.depth", 0.0f, 1.0f, 0, 1));
        else if (t == "Airy") done = layerMacro ("Breath");
        else if (t == "Twin") done = layerMacro ("Octave");
        else if (t == "Glowing") done = layerMacro ("Pad");
        else if (t == "Heavy") done = layerMacro ("Sub");
        else if (t == "Glimmer") done = layerMacro ("Glimmer");
        else if (t == "Desert") done = layerMacro ("Drone");
        else done = false;
        if (! done)
        {
            if (r.pattern == 5) setMacro (p, 3, M ("Gate", 1.0f).abs ("gate.depth", 1.0f, 1.0f, 0.0f, 1.0f));
            else if (f == "recorder" && c.has (FT::transient)) setMacro (p, 3, M ("Chiff", 0.3f).fxp (c, FT::transient, "attack", 0.7f));
            else if (f == "sax" && c.has (FT::drive)) setMacro (p, 3, M ("Grit", 0.25f).fxp (c, FT::drive, "amount", 0.4f).fxp (c, FT::eq, "mid", 4.0f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -4.0f));
            else if (f == "bluesharp" && c.has (FT::drive)) setMacro (p, 3, M ("Amp", 0.3f).fxp (c, FT::drive, "amount", 0.45f).fxp (c, FT::eq, "mid", 4.0f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -4.0f));
            else if ((f == "harmonica" || f == "bluesharp") && lfoTo (p.mod.lfo[1], "volume")) setMacro (p, 3, M ("Tremolo", 0.0f).abs ("lfo2.depth", 0.0f, 1.0f, 0, 1));
            else if (f == "synth" && lfoTo (p.mod.lfo[1], "cutoff")) setMacro (p, 3, M ("Motion", 0.4f).abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1));
            else if (f == "didge" && c.has (FT::filter)) setMacro (p, 3, M ("Throat", 0.3f).fxp (c, FT::filter, "lfo", 0.6f).fxp (c, FT::filter, "lfoRate", 1.2f));
            else if ((f == "kalimba" || f == "mbira") && c.has (FT::drive) && lfoTo (p.mod.lfo[1], "pitch"))
                setMacro (p, 3, M ("Tape", 0.0f).abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1).fxp (c, FT::drive, "amount", 0.3f).fxp (c, FT::drive, "mix", 0.6f)
                                               .toMax ("fx.drive.output", c.get (FT::drive, "output"), -2.5f));
            else if ((f == "balafon" || f == "slitdrum") && c.has (FT::body)) setMacro (p, 3, M ("Wood", 0.0f).fxp (c, FT::body, "mix", 0.6f));
            else if (c.has (FT::chorus))
            {
                const bool ens = c.get (FT::chorus, "mode") > 0.5f;
                M m (ens ? "Ensemble" : "Chorus", c.get (FT::chorus, "mix") < 0.001f ? 0.0f : 0.25f);
                setMacro (p, 3, m.fxp (c, FT::chorus, "mix", 0.7f).fxp (c, FT::chorus, "depth", 0.6f).rel ("quick.width", 0.4f));
            }
            else if (c.has (FT::drive)) setMacro (p, 3, M ("Warmth", 0.2f).fxp (c, FT::drive, "amount", 0.35f).fxp (c, FT::drive, "mix", 0.5f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -3.0f));
            else setMacro (p, 3, M ("Width", 0.5f).rel ("quick.width", 0.6f).rel ("quick.chorus", 0.4f));
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
// a macro extreme may not push the sound over a comfort limit the designed sound is under (lin = the preset's final gain)
juce::String judge (const Probe& x, const Probe& ref, float lin, bool longTail)
{
    if (x.peak127 * lin >= 0.885f && x.peak127 > ref.peak127 * 1.06f) return "clip";
    if (x.s25 > 0.10f && x.s25 > ref.s25 + 0.005f) return "harsh-2-5k";
    if ((x.s6 > 0.03f && x.s6 > ref.s6 + 0.002f) || (x.s8 > 0.012f && x.s8 > ref.s8 + 0.001f)) return "piercing-highs";
    if (x.h1 < -3.0f && x.h1 < ref.h1 - 0.5f) return "fundamental";
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
struct MacroReport { int fixes = 0; juce::StringArray harsh, quiet; };
MacroReport checkMacros (VaultPreset& p, float lin)
{
    MacroReport mr;
    bool longTail = false;
    for (auto& m : p.macros) for (auto& t : m.targets) if (t.target.contains ("reverb") || t.target.contains ("delay") || t.target.contains ("release")) longTail = true;
    const double total = longTail ? 8.0 : 3.5;
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
            const auto badLo = judge (lo, ref, lin, longTail), badHi = judge (hi, ref, lin, longTail);
            if (badLo.isEmpty() && badHi.isEmpty()) break;
            if (round == 4) { mr.harsh.add (p.name + ": " + mac.name + " (" + (badLo.isNotEmpty() ? badLo : badHi) + ")"); break; }
            if (envInt ("WINDS_MACROLOG", 0) != 0) std::printf ("  narrow %s / %s: %s | %s\n", p.name.toRawUTF8(), mac.name.toRawUTF8(), badLo.toRawUTF8(), badHi.toRawUTF8());
            // shorten the failing side (the last round takes it away); the designed point stays where it is
            const float k = round == 3 ? 0.0f : 0.55f;
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
                        if (dd.target == "pitch" && std::abs (t.max - t.min) * std::abs (dd.amount) >= 0.04f) return true;
            return false;
        }();
        if (extremeDistance (lo, hi) < 1.0f && std::abs (lo.loud - hi.loud) < 1.0f && std::abs (amDepth (lo.c5) - amDepth (hi.c5)) < 0.4f && ! pitchLfo
            && std::abs (tailRatioDb (lo.c5) - tailRatioDb (hi.c5)) < 2.0f)
            mr.quiet.add (p.name + ": " + mac.name);
    }
    return mr;
}
} // namespace v3

//==============================================================================
// THE SHIPPED v2 SELECTION (id | name of the winds / world presets in the shipped bank). Candidate ids are positions in
// the candidate list, so they are stable; these presets are accepted first so the shipped ids and names stay.
const char* const shippedV2[] = {
    "cv.wind.0001|Concert Flute",
    "cv.wind.0002|Flute Vibrato",
    "cv.wind.0003|Expressive Flute",
    "cv.wind.0004|Flute Staccato",
    "cv.wind.0005|High Flute",
    "cv.wind.0006|High Flute Vibrato",
    "cv.wind.0007|High Expressive Flute",
    "cv.wind.0008|Piccolo",
    "cv.wind.0009|Piccolo Staccato",
    "cv.wind.0010|Clarinet",
    "cv.wind.0011|Clarinet Staccato",
    "cv.wind.0012|High Clarinet Staccato",
    "cv.wind.0013|Clarion Clarinet",
    "cv.wind.0014|Chalumeau Clarinet",
    "cv.wind.0015|Oboe",
    "cv.wind.0018|High Oboe Staccato",
    "cv.wind.0019|High Oboe",
    "cv.wind.0023|Soprano Recorder",
    "cv.wind.0024|Tongued Soprano Recorder",
    "cv.wind.0026|Alto Recorder Vibrato",
    "cv.wind.0027|Tongued Alto Recorder",
    "cv.wind.0028|Tenor Recorder",
    "cv.wind.0029|Tenor Recorder Vibrato",
    "cv.wind.0030|Tongued Tenor Recorder",
    "cv.wind.0031|Bass Recorder",
    "cv.wind.0033|Tongued Bass Recorder",
    "cv.wind.0034|Small Ocarina",
    "cv.wind.0035|Small Ocarina Staccato",
    "cv.wind.0036|Ocarina",
    "cv.wind.0038|Tenor Sax",
    "cv.wind.0039|Tenor Sax Vibrato",
    "cv.wind.0040|Tenor Sax Staccato",
    "cv.wind.0041|Tenor Sax Stabs",
    "cv.wind.0042|Altissimo Tenor Sax",
    "cv.wind.0043|Soprano Sax",
    "cv.wind.0044|Soprano Sax Vibrato",
    "cv.wind.0045|Soprano Sax Staccato",
    "cv.wind.0046|Chromatic Harmonica",
    "cv.wind.0047|Accented Harmonica",
    "cv.wind.0049|Low Chromatic Harmonica",
    "cv.wind.0050|Blues Harp",
    "cv.wind.0053|Hand Wah Harp",
    "cv.wind.0054|High Blues Harp",
    "cv.wind.0055|High Blues Harp Vibrato",
    "cv.wind.0056|Low Harmonica Vibrato",
    "cv.wind.0057|Blues Harp Chops",
    "cv.wind.0059|Breath Pipe",
    "cv.wind.0060|Square Reed",
    "cv.wind.0061|Hollow Pipe",
    "cv.wind.0062|Flute and Clarinet",
    "cv.wind.0063|Pastoral Pair",
    "cv.wind.0064|Oboe and Clarinet",
    "cv.wind.0065|Flute over Clarinet",
    "cv.wind.0066|Clarinet over Bassoon",
    "cv.wind.0067|Woodwind Choir",
    "cv.wind.0068|Recorder Consort",
    "cv.wind.0069|Flute and Harp",
    "cv.wind.0074|Harmonica and Organ",
    "cv.wind.0075|Vocal Flute",
    "cv.wind.0077|Oboe and Bassoon",
    "cv.wind.0080|Late Night Sax",
    "cv.wind.0085|Bossa Flute",
    "cv.wind.0087|Baroque Recorder",
    "cv.wind.0088|Oboe over Strings",
    "cv.wind.0090|Folk Harmonica",
    "cv.wind.0091|Clarinet Shadows",
    "cv.wind.0093|Staccato Winds",
    "cv.wind.0094|Bassoon and Pizzicato",
    "cv.wind.0095|Ocarina and Pad",
    "cv.wind.0097|Harmonica Duo",
    "cv.wind.0099|Recorder and Nylon",
    "cv.wind.0102|Flute and Toy Piano",
    "cv.wind.0104|Flute and Marimba",
    "cv.wind.0106|Choir of Flutes",
    "cv.wind.0117|Far Clarinet Staccato",
    "cv.wind.0124|Hall High Oboe Staccato",
    "cv.wind.0148|Distant Soprano Sax",
    "cv.wind.0153|Hall Harmonica Vibrato",
    "cv.wind.0205|Hall Tenor Section",
    "cv.wind.0211|Intimate Expressive Flute",
    "cv.wind.0212|Velvet Flute Staccato",
    "cv.wind.0213|Hushed High Flute",
    "cv.wind.0217|Velvet Clarinet",
    "cv.wind.0218|Hushed Clarinet Staccato",
    "cv.wind.0219|Velvet Clarion Clarinet",
    "cv.wind.0224|Velvet High Oboe Staccato",
    "cv.wind.0229|Hushed Soprano Recorder",
    "cv.wind.0231|Velvet Tenor Recorder",
    "cv.wind.0232|Velvet Bass Recorder",
    "cv.wind.0233|Velvet Small Ocarina",
    "cv.wind.0236|Hushed Tenor Sax",
    "cv.wind.0237|Intimate Tenor Sax Vibrato",
    "cv.wind.0238|Velvet Tenor Sax Staccato",
    "cv.wind.0239|Hushed Tenor Sax Stabs",
    "cv.wind.0244|Hushed Accented Harmonica",
    "cv.wind.0247|Intimate Soft Blues Harp",
    "cv.wind.0256|Velvet Flute & Clarinet",
    "cv.wind.0257|Hushed Pastoral Pair",
    "cv.wind.0258|Intimate Oboe & Clarinet",
    "cv.wind.0259|Velvet Flute over Clarinet",
    "cv.wind.0260|Intimate Woodwind Choir",
    "cv.wind.0262|Hushed Flute and Harp",
    "cv.wind.0263|Velvet Flute over Strings",
    "cv.wind.0267|Hushed Vocal Flute",
    "cv.wind.0272|Velvet Late Night Sax",
    "cv.wind.0278|Hushed Baroque Recorder",
    "cv.wind.0281|Hushed Folk Harmonica",
    "cv.wind.0282|Intimate Clarinet Shadows",
    "cv.wind.0283|Hushed Staccato Winds",
    "cv.wind.0288|Hushed Recorder & Nylon",
    "cv.wind.0291|Hushed Flute & Toy Piano",
    "cv.wind.0293|Velvet Flute & Marimba",
    "cv.wind.0306|Echo Clarinet Staccato",
    "cv.wind.0330|Night Tenor Sax Staccato",
    "cv.wind.0331|Echo Tenor Sax Stabs",
    "cv.wind.0335|Night Soprano Sax Staccato",
    "cv.wind.0345|Midnight Blues Harp Chops",
    "cv.wind.0379|Echo Staccato Winds",
    "cv.wind.0404|Faded Chalumeau Clarinet",
    "cv.wind.0453|Faded Breath and Flute",
    "cv.wind.0465|Dusty Sax & Upright Duo",
    "cv.wind.0474|Faded Flute and Choir",
    "cv.wind.0482|Next Room Flute Vibrato",
    "cv.wind.0486|Muffled High Flute Vibrato",
    "cv.wind.0488|Muffled Piccolo Staccato",
    "cv.wind.0489|Behind the Wall Clarinet",
    "cv.wind.0490|Muffled Clarinet Staccato",
    "cv.wind.0495|Next Room Oboe Staccato",
    "cv.wind.0497|Behind the Wall High Oboe",
    "cv.wind.0512|Next Room Soprano Sax",
    "cv.wind.0513|Muffled Accented Harmonica",
    "cv.wind.0516|Muffled Soft Blues Harp",
    "cv.wind.0518|Next Room Hand Wah Harp",
    "cv.wind.0519|Muffled High Blues Harp",
    "cv.wind.0520|Muffled Blues Harp Chops",
    "cv.wind.0552|Cloud Accented Harmonica",
    "cv.wind.0555|Drifting Soft Blues Harp",
    "cv.wind.0575|Drifting Breath & Flute",
    "cv.wind.0585|Cloud Baroque Recorder",
    "cv.wind.0593|Floating Ocarina & Flute",
    "cv.wind.0604|Breath High Flute",
    "cv.wind.0605|Short High Flute Vibrato",
    "cv.wind.0613|Puff High Oboe",
    "cv.wind.0614|Breath Bassoon",
    "cv.wind.0625|Breath Tenor Sax",
    "cv.wind.0639|Puff Synth Flute",
    "cv.wind.0640|Short Breath Pipe",
    "cv.wind.0641|Short Square Reed",
    "cv.wind.0642|Puff Hollow Pipe",
    "cv.wind.0659|Swelling Alto Recorder",
    "cv.wind.0667|Rising Tenor Sax",
    "cv.wind.0670|Swelling Soprano Sax",
    "cv.wind.0671|Rising Soprano Sax Vibrato",
    "cv.wind.0680|Dawn Low Harmonica Vibrato",
    "cv.wind.0685|Gritty Concert Flute",
    "cv.wind.0686|Smoky Flute Vibrato",
    "cv.wind.0687|Saturated Expressive Flute",
    "cv.wind.0688|Gritty Flute Staccato",
    "cv.wind.0691|Smoky Piccolo",
    "cv.wind.0692|Saturated Piccolo Staccato",
    "cv.wind.0695|Gritty Clarion Clarinet",
    "cv.wind.0697|Saturated Oboe",
    "cv.wind.0701|Gritty High Oboe",
    "cv.wind.0705|Smoky Soprano Recorder",
    "cv.wind.0707|Gritty Tenor Recorder",
    "cv.wind.0708|Gritty Bass Recorder",
    "cv.wind.0710|Saturated Ocarina",
    "cv.wind.0712|Smoky Tenor Sax",
    "cv.wind.0713|Gritty Tenor Sax Vibrato",
    "cv.wind.0714|Gritty Tenor Sax Staccato",
    "cv.wind.0715|Smoky Tenor Sax Stabs",
    "cv.wind.0718|Smoky Soprano Sax Vibrato",
    "cv.wind.0719|Smoky Soprano Sax Staccato",
    "cv.wind.0721|Smoky Accented Harmonica",
    "cv.wind.0723|Smoky Blues Harp",
    "cv.wind.0726|Smoky Hand Wah Harp",
    "cv.wind.0731|Saturated Square Reed",
    "cv.wind.0740|Rewind Piccolo Staccato",
    "cv.wind.0748|Rewind High Oboe Staccato",
    "cv.wind.0764|Rewind Altissimo Tenor Sax",
    "cv.wind.0782|Windswept Expressive Flute",
    "cv.wind.0783|Airy Flute Staccato",
    "cv.wind.0787|Breathy Piccolo",
    "cv.wind.0788|Windswept Piccolo Staccato",
    "cv.wind.0790|Breathy Clarinet Staccato",
    "cv.wind.0793|Windswept Oboe",
    "cv.wind.0794|Airy Oboe Vibrato",
    "cv.wind.0795|Breathy Oboe Staccato",
    "cv.wind.0796|Airy High Oboe Staccato",
    "cv.wind.0801|Breathy Soprano Recorder",
    "cv.wind.0804|Airy Tongued Alto Recorder",
    "cv.wind.0805|Airy Tenor Recorder",
    "cv.wind.0806|Airy Bass Recorder",
    "cv.wind.0808|Airy Tongued Bass Recorder",
    "cv.wind.0810|Windswept Ocarina",
    "cv.wind.0812|Breathy Tenor Sax",
    "cv.wind.0814|Airy Tenor Sax Staccato",
    "cv.wind.0815|Breathy Tenor Sax Stabs",
    "cv.wind.0817|Airy Soprano Sax",
    "cv.wind.0819|Airy Soprano Sax Staccato",
    "cv.wind.0823|Breathy Blues Harp",
    "cv.wind.0824|Windswept Soft Blues Harp",
    "cv.wind.0826|Breathy Hand Wah Harp",
    "cv.wind.0828|Airy Low Harmonica Vibrato",
    "cv.wind.0829|Windswept Blues Harp Chops",
    "cv.wind.0834|Octave High Flute",
    "cv.wind.0835|Doubled High Flute Vibrato",
    "cv.wind.0836|Twin High Expressive Flute",
    "cv.wind.0838|Octave Clarinet Staccato",
    "cv.wind.0839|Twin Clarion Clarinet",
    "cv.wind.0840|Doubled High Oboe Staccato",
    "cv.wind.0841|Twin High Oboe",
    "cv.wind.0844|Octave Tenor Sax Stabs",
    "cv.wind.0845|Twin Altissimo Tenor Sax",
    "cv.wind.0846|Twin Chromatic Harmonica",
    "cv.wind.0847|Octave Accented Harmonica",
    "cv.wind.0848|Doubled Harmonica Vibrato",
    "cv.wind.0865|Glowing Alto Recorder",
    "cv.wind.0870|Glowing Small Ocarina",
    "cv.wind.0877|Halo Soprano Sax Vibrato",
    "cv.wind.0883|Glowing Blues Harp Vibrato",
    "cv.wind.0885|Aurora High Blues Harp",
    "cv.wind.0896|Lyrical Chalumeau Clarinet",
    "cv.wind.0951|Wide Soprano Sax Staccato",
    "cv.wind.0957|Wide Blues Harp Vibrato",
    "cv.wind.0962|Wide Synth Flute",
    "cv.wind.0963|Ensemble Breath Pipe",
    "cv.wind.1056|Warped Concert Flute",
    "cv.wind.1065|Seasick Clarinet Staccato",
    "cv.wind.1074|Melted Bassoon Vibrato",
    "cv.wind.1085|Warped Tenor Sax Staccato",
    "cv.wind.1086|Seasick Tenor Sax Stabs",
    "cv.world.0001|Dan Tranh",
    "cv.world.0002|High Dan Tranh",
    "cv.world.0003|Dan Tranh Bends",
    "cv.world.0004|High Dan Tranh Bends",
    "cv.world.0005|Dan Tranh Tremolo",
    "cv.world.0006|High Dan Tranh Tremolo",
    "cv.world.0007|Low Dan Tranh Bends",
    "cv.world.0009|Balafon",
    "cv.world.0011|Soft Balafon",
    "cv.world.0012|High Balafon",
    "cv.world.0014|High Soft Balafon",
    "cv.world.0015|Thumb Piano",
    "cv.world.0016|High Thumb Piano",
    "cv.world.0017|Low Thumb Piano",
    "cv.world.0019|Bright Low Thumb Piano",
    "cv.world.0020|Mbira",
    "cv.world.0021|Low Mbira",
    "cv.world.0022|Deep Mbira",
    "cv.world.0023|Low Deep Mbira",
    "cv.world.0024|Nyunga Mbira",
    "cv.world.0025|Slit Drum",
    "cv.world.0026|Log Drum",
    "cv.world.0027|Psaltery",
    "cv.world.0028|Bowed Psaltery",
    "cv.world.0029|Psaltery Spiccato",
    "cv.world.0030|Didgeridoo",
    "cv.world.0032|Thumb Piano & Marimba",
    "cv.world.0033|Thumb Piano Music Box",
    "cv.world.0035|Zither and Flute",
    "cv.world.0037|Balafon and Marimba",
    "cv.world.0038|Thumb Piano Sub",
    "cv.world.0040|Didgeridoo Dreamtime",
    "cv.world.0041|Harp and Thumb Piano",
    "cv.world.0044|Ocarina Island",
    "cv.world.0046|Balafon & Thumb Piano",
    "cv.world.0048|Psaltery and Harp",
    "cv.world.0050|Interlocking Mbiras",
    "cv.world.0051|Zither and Cellos",
    "cv.world.0052|Thumb Piano over Pad",
    "cv.world.0055|Minstrel",
    "cv.world.0057|Marimba Mbira",
    "cv.world.0058|Dan Tranh Duet",
    "cv.world.0059|Thumb Piano Trio",
    "cv.world.0060|Slit Drum Marimba",
    "cv.world.0062|Didgeridoo Choir",
    "cv.world.0064|Harp and Mbira",
    "cv.world.0065|Thumb Piano and Steel",
    "cv.world.0067|Flute and Thumb Piano",
    "cv.world.0068|Ocarina and Harp",
    "cv.world.0070|Desert Strings",
    "cv.world.0093|Distant Log Drum",
    "cv.world.0124|Courtyard Desert Strings",
    "cv.world.0125|Felt Dan Tranh",
    "cv.world.0126|Velvet High Dan Tranh",
    "cv.world.0134|Velvet Soft Balafon",
    "cv.world.0137|Velvet High Soft Balafon",
    "cv.world.0138|Hushed Thumb Piano",
    "cv.world.0139|Felt High Thumb Piano",
    "cv.world.0140|Velvet Low Thumb Piano",
    "cv.world.0142|Velvet Mbira",
    "cv.world.0143|Hushed Low Mbira",
    "cv.world.0144|Felt Deep Mbira",
    "cv.world.0145|Velvet Low Deep Mbira",
    "cv.world.0146|Hushed Nyunga Mbira",
    "cv.world.0152|Hushed Didgeridoo",
    "cv.world.0154|Felt Thumb Piano Music Box",
    "cv.world.0158|Hushed Balafon & Marimba",
    "cv.world.0159|Felt Thumb Piano Sub",
    "cv.world.0161|Felt Didgeridoo Dreamtime",
    "cv.world.0162|Felt Harp & Thumb Piano",
    "cv.world.0164|Felt Ocarina Island",
    "cv.world.0166|Felt Thumb Piano & Vibes",
    "cv.world.0170|Velvet Zither & Cellos",
    "cv.world.0171|Felt Thumb Piano over Pad",
    "cv.world.0173|Hushed Minstrel",
    "cv.world.0174|Felt Mountain Zither",
    "cv.world.0175|Velvet Marimba Mbira",
    "cv.world.0176|Hushed Dan Tranh Duet",
    "cv.world.0177|Felt Thumb Piano Trio",
    "cv.world.0178|Velvet Slit Drum Marimba",
    "cv.world.0180|Felt Didgeridoo Choir",
    "cv.world.0182|Hushed Harp and Mbira",
    "cv.world.0185|Felt Flute & Thumb Piano",
    "cv.world.0188|Hushed Desert Strings",
    "cv.world.0191|Midnight Dan Tranh Bends",
    "cv.world.0196|Midnight Balafon",
    "cv.world.0200|Night High Hard Balafon",
    "cv.world.0203|Night High Thumb Piano",
    "cv.world.0211|Night Slit Drum",
    "cv.world.0222|Night Balafon & Marimba",
    "cv.world.0231|Echo Psaltery and Harp",
    "cv.world.0242|Echo Slit Drum Marimba",
    "cv.world.0245|Echo Psaltery & Chimes",
    "cv.world.0255|Faded Dan Tranh Bends",
    "cv.world.0256|Dusty High Dan Tranh Bends",
    "cv.world.0270|Cassette Mbira",
    "cv.world.0274|Faded Nyunga Mbira",
    "cv.world.0283|Dusty Zither and Flute",
    "cv.world.0296|Cassette Zither & Cellos",
    "cv.world.0312|Faded Desert Strings",
    "cv.world.0313|Muted Dan Tranh",
    "cv.world.0314|Damped High Dan Tranh",
    "cv.world.0315|Stopped Dan Tranh Bends",
    "cv.world.0316|Muted High Dan Tranh Bends",
    "cv.world.0317|Muted Low Dan Tranh Bends",
    "cv.world.0326|Muted High Thumb Piano",
    "cv.world.0330|Stopped Low Mbira",
    "cv.world.0333|Stopped Nyunga Mbira",
    "cv.world.0335|Damped Log Drum",
    "cv.world.0338|Blooming Dan Tranh",
    "cv.world.0339|Bowed High Dan Tranh",
    "cv.world.0341|Bowed High Dan Tranh Bends",
    "cv.world.0344|Unfolding Balafon",
    "cv.world.0347|Unfolding High Balafon",
    "cv.world.0350|Unfolding Thumb Piano",
    "cv.world.0351|Blooming High Thumb Piano",
    "cv.world.0354|Bowed Mbira",
    "cv.world.0359|Blooming Slit Drum",
    "cv.world.0365|Muffled Dan Tranh Bends",
    "cv.world.0385|Muffled Psaltery",
    "cv.world.0386|Next Room Bowed Psaltery",
    "cv.world.0387|Muffled Psaltery Spiccato",
    "cv.world.0390|Reverse Dan Tranh",
    "cv.world.0391|Backward High Dan Tranh",
    "cv.world.0395|Backward Low Dan Tranh",
    "cv.world.0398|Backward Soft Balafon",
    "cv.world.0402|Rewind Thumb Piano",
    "cv.world.0405|Rewind Bass Thumb Piano",
    "cv.world.0406|Backward Mbira",
    "cv.world.0410|Rewind Nyunga Mbira",
    "cv.world.0412|Backward Log Drum",
    "cv.world.0427|Cloud Soft Balafon",
    "cv.world.0453|Cloud Log Drum Thumb Piano",
    "cv.world.0482|Heavy Dan Tranh",
    "cv.world.0483|Deep High Dan Tranh",
    "cv.world.0485|Heavy High Dan Tranh Bends",
    "cv.world.0489|Weighted Balafon",
    "cv.world.0491|Deep Soft Balafon",
    "cv.world.0492|Weighted High Balafon",
    "cv.world.0495|Weighted Thumb Piano",
    "cv.world.0498|Weighted Bass Thumb Piano",
    "cv.world.0500|Weighted Low Mbira",
    "cv.world.0501|Heavy Deep Mbira",
    "cv.world.0502|Weighted Low Deep Mbira",
    "cv.world.0503|Weighted Nyunga Mbira",
    "cv.world.0504|Heavy Slit Drum",
    "cv.world.0505|Deep Log Drum",
    "cv.world.0506|Weighted Psaltery",
    "cv.world.0507|Heavy Bowed Psaltery",
    "cv.world.0511|Doubled Dan Tranh Bends",
    "cv.world.0516|Octave High Soft Balafon",
    "cv.world.0517|Twin High Thumb Piano",
    "cv.world.0518|Octave Low Thumb Piano",
    "cv.world.0519|Octave Mbira",
    "cv.world.0526|Starlit Balafon",
    "cv.world.0528|Sparkle Soft Balafon",
    "cv.world.0532|Starlit Thumb Piano",
    "cv.world.0535|Starlit Bass Thumb Piano",
    "cv.world.0537|Starlit Low Mbira",
    "cv.world.0539|Sparkle Low Deep Mbira",
    "cv.world.0540|Starlit Nyunga Mbira",
    "cv.world.0549|Halo Low Dan Tranh Bends",
    "cv.world.0551|Aurora Balafon",
    "cv.world.0554|Aurora High Balafon",
    "cv.world.0558|Glowing High Thumb Piano",
    "cv.world.0559|Halo Low Thumb Piano",
    "cv.world.0560|Aurora Bass Thumb Piano",
    "cv.world.0561|Halo Mbira",
    "cv.world.0562|Aurora Low Mbira",
    "cv.world.0564|Halo Low Deep Mbira",
    "cv.world.0565|Aurora Nyunga Mbira",
    "cv.world.0566|Glowing Slit Drum",
    "cv.world.0567|Halo Log Drum",
    "cv.world.0571|Aurora Didgeridoo",
    "cv.world.0574|Dune High Dan Tranh",
    "cv.world.0576|Dune High Dan Tranh Bends",
    "cv.world.0579|Dune Low Dan Tranh",
    "cv.world.0580|Mirage Balafon",
    "cv.world.0582|Dune Soft Balafon",
    "cv.world.0583|Mirage High Balafon",
    "cv.world.0584|Desert High Hard Balafon",
    "cv.world.0587|Desert High Thumb Piano",
    "cv.world.0589|Mirage Bass Thumb Piano",
    "cv.world.0593|Dune Low Deep Mbira",
    "cv.world.0595|Desert Slit Drum",
    "cv.world.0596|Dune Log Drum",
    "cv.world.0597|Mirage Psaltery",
    "cv.world.0598|Desert Bowed Psaltery",
    "cv.world.0606|Wide Low Dan Tranh",
    "cv.world.0630|Silken Mbira and Nylon",
    "cv.world.0656|Wide Psaltery & Chimes",
    "cv.world.0664|Gritty Dan Tranh",
    "cv.world.0671|Saturated Balafon",
    "cv.world.0673|Smoky Soft Balafon",
    "cv.world.0677|Saturated Thumb Piano",
    "cv.world.0680|Saturated Bass Thumb Piano",
    "cv.world.0682|Saturated Low Mbira",
    "cv.world.0684|Smoky Low Deep Mbira",
    "cv.world.0685|Saturated Nyunga Mbira",
    "cv.world.0686|Gritty Slit Drum",
    "cv.world.0687|Smoky Log Drum",
    "cv.world.0691|Saturated Didgeridoo",
    "cv.world.0694|Seasick High Dan Tranh",
    "cv.world.0698|Seasick Low Dan Tranh",
    "cv.world.0699|Melted Balafon",
    "cv.world.0702|Melted High Balafon",
    "cv.world.0706|Warped High Thumb Piano",
    "cv.world.0711|Warped Deep Mbira",
    "cv.world.0714|Warped Slit Drum",
};
} // namespace

std::vector<VaultPreset> build_winds (const RecipeContext& ctx)
{
    std::vector<Cand> cands;
    addAll (ctx, cands);
    if (const int lim = juce::SystemStats::getEnvironmentVariable ("WINDS_LIMIT", "0").getIntValue(); lim > 0 && lim < (int) cands.size())
        cands.resize ((size_t) lim);
    std::vector<VaultPreset> all;
    std::vector<Role> roles;
    int nW = 0, nX = 0;
    for (auto& c : cands)
    {
        VaultPreset p;
        p.category = c.category;
        p.id = c.category == vault::catWorld ? "cv.world." + juce::String (++nX).paddedLeft ('0', 4) : "cv.wind." + juce::String (++nW).paddedLeft ('0', 4);
        p.name = c.name;
        p.parts = c.parts;
        all.push_back (std::move (p));
        roles.push_back (c.role);
    }
    const int threads = juce::jmax (1, juce::SystemStats::getEnvironmentVariable ("WINDS_THREADS", "4").getIntValue());
    auto parallel = [threads] (int count, const char* what, const std::function<void (int)>& fn)
    {
        std::atomic<int> next { 0 }, done { 0 };
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t)
            pool.emplace_back ([&] { for (int i; (i = next++) < count;) { fn (i); if (const int d = ++done; d % 200 == 0) std::printf ("  %s %d / %d\n", what, d, count); } });
        for (auto& t : pool) t.join();
    };

    // 1. the v2 stage: every candidate repaired by measurement (part calibration, mono-safe width, highs, fundamental).
    //    WINDS_V2_CACHE=<dir> keeps its result between runs while the v3 design is tuned (delete it when the v2 stage or
    //    the library changes).
    std::vector<VaultPreset> out;
    std::vector<Role> outRoles;
    juce::File cacheBank, cacheRoles;
    if (auto dir = juce::SystemStats::getEnvironmentVariable ("WINDS_V2_CACHE", {}); dir.isNotEmpty())
    {
        juce::File d (dir);
        d.createDirectory();
        cacheBank = d.getChildFile ("winds_v2.cvb"); cacheRoles = d.getChildFile ("winds_v2_ids.txt");
    }
    juce::StringArray cachedIds;
    if (cacheBank.existsAsFile() && cacheRoles.existsAsFile() && vault::loadBankFile (cacheBank, out))
    {
        cacheRoles.readLines (cachedIds);
        std::map<juce::String, size_t> at;
        for (size_t i = 0; i < all.size(); ++i) at[all[i].id] = i;
        for (auto& p : out) { outRoles.push_back (roles[at[p.id]]); p.name = all[at[p.id]].name; }   // names follow the current list
        std::printf ("winds: v2 stage from the cache (%d presets)\n", (int) out.size());
    }
    else
    {
        Calib calib;
        std::vector<char> keep (all.size(), 0);
        parallel ((int) all.size(), "repaired", [&] (int i) { keep[(size_t) i] = repair (all[(size_t) i], calib) ? 1 : 0; });
        int dropped = 0;
        for (size_t i = 0; i < all.size(); ++i)
        {
            if (keep[i]) { out.push_back (all[i]); outRoles.push_back (roles[i]); }
            else { ++dropped; std::printf ("  unrepairable: %s %s\n", all[i].id.toRawUTF8(), all[i].name.toRawUTF8()); }
        }
        std::printf ("winds: %d candidates, %d unrepairable\n", (int) all.size(), dropped);
        if (cacheBank != juce::File())
        {
            vault::saveBankFile (cacheBank, out);
            juce::String t;
            for (auto& p : out) t << p.id << "\n";
            cacheRoles.replaceWithText (t);
        }
    }
    // WINDS_ONLY=<id,id,...>: just these presets (demos, listening)
    if (const auto only = juce::StringArray::fromTokens (juce::SystemStats::getEnvironmentVariable ("WINDS_ONLY", {}), ",", ""); ! only.isEmpty())
    {
        std::vector<VaultPreset> o; std::vector<Role> ro;
        for (size_t i = 0; i < out.size(); ++i) if (only.contains (out[i].id)) { o.push_back (out[i]); ro.push_back (outRoles[i]); }
        out.swap (o); outRoles.swap (ro);
    }

    // 2. v3: the finished sound (chain, movement, patterns), its comfort re-fit, then the four sound macros
    const std::vector<VaultPreset> base = out;   // the v2 stage's sounds (the redesign variants start from them)
    auto designed = [&] (int i, int variant)
    {
        auto p = base[(size_t) i];
        v3::designSound (p, outRoles[(size_t) i], variant);
        const int moves = v3::fitComfort (p);
        v3::designMacros (p, outRoles[(size_t) i]);
        return std::make_pair (p, moves);
    };
    std::atomic<int> refits { 0 };
    parallel ((int) out.size(), "designed", [&] (int i)
    {
        auto [p, moves] = designed (i, 0);
        out[(size_t) i] = p;
        if (moves > 0) ++refits;
    });
    std::printf ("winds: %d presets designed (v3), %d needed a comfort re-fit after the FX\n", (int) out.size(), refits.load());
    if (v3::envInt ("WINDS_SELFGATE", 1) == 0) return out;

    // 3. the gate's analysis of every designed preset, then the selection: the shipped presets first (their ids and names
    //    stay), then the rest in candidate order; a preset is kept only if it passes the gate + the assembly QA limits and
    //    its fingerprint is >= WINDS_OWN_MIN (4.8, stricter than the shared 3.5) from every preset kept before it
    std::vector<recipegate::Result> res (out.size());
    parallel ((int) out.size(), "gate", [&] (int i) { res[(size_t) i].preset = out[(size_t) i]; recipegate::analyse (res[(size_t) i]); });
    std::map<juce::String, juce::String> shipped;
    for (auto* row : shippedV2) { const auto c = juce::StringArray::fromTokens (row, "|", ""); shipped[c[0]] = c[1]; }
    int nameMismatch = 0;
    // (the gate writes long names with "&" for "and", as the shipped bank has them)
    for (auto& r : res)
        if (auto it = shipped.find (r.preset.id); it != shipped.end() && it->second.replace (" & ", " and ") != r.preset.name.replace (" & ", " and "))
        { ++nameMismatch; std::printf ("  shipped id %s is now '%s' (was '%s')\n", r.preset.id.toRawUTF8(), r.preset.name.toRawUTF8(), it->second.toRawUTF8()); }
    std::vector<int> order;
    // shipped presets first, then the pattern presets (a rhythm version of a sound is meant to be close to it: they need
    // only the shared distance), then the rest
    for (int i = 0; i < (int) out.size(); ++i) if (shipped.count (out[(size_t) i].id)) order.push_back (i);
    for (int i = 0; i < (int) out.size(); ++i) if (! shipped.count (out[(size_t) i].id) && outRoles[(size_t) i].pattern > 0) order.push_back (i);
    for (int i = 0; i < (int) out.size(); ++i) if (! shipped.count (out[(size_t) i].id) && outRoles[(size_t) i].pattern == 0) order.push_back (i);
    const float ownMin = (float) juce::SystemStats::getEnvironmentVariable ("WINDS_OWN_MIN", "4.8").getDoubleValue();
    const float pinMin = (float) juce::SystemStats::getEnvironmentVariable ("WINDS_PIN_MIN", juce::String (ownMin)).getDoubleValue();
    std::vector<int> acc;
    std::map<juce::String, int> fails;
    int shippedKept = 0;
    juce::String failLog;
    int redesigned[3] {};
    auto check = [&] (recipegate::Result& r, float minDist)
    {
        juce::String why;
        if (r.fail.isEmpty() && ! vault::validatePreset (r.preset, &why)) r.fail = "invalid: " + why;
        if (r.fail.isEmpty() && r.gainDb > recipegate::maxTrimDb) r.fail = "too-quiet";
        if (r.fail.isEmpty() && r.share8k > recipegate::maxShare8k) r.fail = "air-8k";
        if (r.fail.isEmpty() && r.centroid > recipegate::centroidCeiling (r.preset.category)) r.fail = "centroid";
        if (r.fail.isEmpty() && r.chordOver > recipegate::maxChordOver) r.fail = "chord-over";
        if (r.fail.isEmpty())
            for (int j : acc)
                if (const float d = recipegate::distance (r.print, res[(size_t) j].print); d < minDist)
                { r.fail = "near: " + res[(size_t) j].preset.name + " " + juce::String (d, 2); break; }
        return r.fail.isEmpty();
    };
    for (int i : order)
    {
        auto& r = res[(size_t) i];
        const bool pin = shipped.count (r.preset.id) > 0;
        const float minDist = pin ? pinMin : outRoles[(size_t) i].pattern > 0 ? 3.5f : ownMin;
        bool ok = check (r, minDist);
        int variant = 0;
        // a shipped preset that fails with the design (too close to a neighbour, chord headroom ...) is designed again
        for (int v = 1; v <= 2 && ! ok && pin && v3::envInt ("WINDS_VARIANTS", 1) != 0; ++v)
        {
            recipegate::Result a;
            a.preset = designed (i, v).first;
            recipegate::analyse (a);
            if (check (a, minDist)) { ok = true; variant = v; out[(size_t) i] = designed (i, v).first; r = a; }
        }
        if (ok) { acc.push_back (i); if (pin) ++shippedKept; ++redesigned[variant]; }
        else
        {
            ++fails[(pin ? "shipped " : "new ") + r.fail.upToFirstOccurrenceOf (":", false, false)];
            if (pin || outRoles[(size_t) i].pattern > 0) failLog << (pin ? "  shipped " : "  pattern ") << r.preset.id << " " << r.preset.name << ": " << r.fail << "\n";
        }
    }
    std::printf ("winds: %d of %d pass the gate (%d of %d shipped ids kept), %d shipped names changed; design %d, role-forward redesign %d, v2-colour redesign %d\n",
                 (int) acc.size(), (int) res.size(), shippedKept, (int) shipped.size(), nameMismatch, redesigned[0], redesigned[1], redesigned[2]);
    for (auto& [f, n] : fails) std::printf ("  winds rejected %-22s %d\n", f.toRawUTF8(), n);
    if (v3::envInt ("WINDS_FAILLOG", 0) != 0) std::printf ("%s", failLog.toRawUTF8());
    std::sort (acc.begin(), acc.end());

    // 4. macro extremes (rendered at 0 and 1, narrowed until they pass the comfort rules) on the kept presets
    std::vector<v3::MacroReport> mr (acc.size());
    std::vector<VaultPreset> kept (acc.size());
    parallel ((int) acc.size(), "macros", [&] (int k)
    {
        kept[(size_t) k] = out[(size_t) acc[(size_t) k]];
        if (v3::envInt ("WINDS_MACROCHECK", 1) != 0) mr[(size_t) k] = v3::checkMacros (kept[(size_t) k], res[(size_t) acc[(size_t) k]].preset.gain);
    });
    int fixes = 0;
    juce::StringArray harsh, quiet;
    for (auto& m : mr) { fixes += m.fixes; harsh.addArray (m.harsh); quiet.addArray (m.quiet); }
    std::printf ("winds: macro ranges narrowed %d times; %d macro extremes still outside the comfort rules; %d macros barely audible\n", fixes, harsh.size(), quiet.size());
    for (auto& h : harsh) std::printf ("  harsh: %s\n", h.toRawUTF8());
    for (auto& q : quiet) std::printf ("  quiet: %s\n", q.toRawUTF8());

    // WINDS_DUMP=<file>: every kept preset's chain, movement and macros (for listening notes / review)
    if (const auto dump = juce::SystemStats::getEnvironmentVariable ("WINDS_DUMP", {}); dump.isNotEmpty())
    {
        juce::String t;
        for (auto& p : kept)
        {
            t << p.id << "\t" << p.name << "\t";
            for (auto& x : p.fx)
            {
                const auto& ti = vault::fxTypeInfo (x.type);
                t << ti.id << (x.on ? "" : "(off)") << "[";
                for (size_t k = 0; k < ti.params.size(); ++k) t << (k ? "," : "") << juce::String (x.p[k], 2);
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
        int nWk = 0, nXk = 0, arpOn = 0, gateOn = 0;
        float minPair = 1.0e9f, h1 = 0, s25 = 0, s6 = 0, s8 = 0;
        std::map<int, int> slots;
        for (size_t x = 0; x < acc.size(); ++x)
        {
            const auto& r = res[(size_t) acc[x]];
            (r.preset.category == vault::catWorld ? nXk : nWk)++;
            arpOn += kept[x].arp.on ? 1 : 0; gateOn += kept[x].gate.on ? 1 : 0;
            ++slots[(int) kept[x].fx.size()];
            h1 = std::min (h1, r.h1Db); s25 = std::max (s25, r.share25); s6 = std::max (s6, r.share6k); s8 = std::max (s8, r.share8k);
            for (size_t y = x + 1; y < acc.size(); ++y) minPair = std::min (minPair, recipegate::distance (r.print, res[(size_t) acc[y]].print));
        }
        std::printf ("winds: kept Flutes & Woodwinds %d, World %d; arp on %d, gate on %d; min pairwise distance %.2f\n", nWk, nXk, arpOn, gateOn, minPair);
        std::printf ("winds: at C5 fundamental >= %.1f dB, 2-5 kHz <= %.1f %%, > 6 kHz <= %.2f %%, > 8 kHz <= %.2f %%\n", h1, 100 * s25, 100 * s6, 100 * s8);
        juce::String sl;
        for (auto& [n, k] : slots) sl << n << " slots: " << k << "  ";
        std::printf ("winds: chain sizes %s\n", sl.toRawUTF8());
    }
    return kept;
}

VAULT_RECIPE (winds, build_winds);
