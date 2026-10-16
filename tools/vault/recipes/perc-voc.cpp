// CHROME VAULT recipes: BELLS & MALLETS + VOCALS & CHOIR (sound designer "perc-voc").
// Run: VaultBankGen -recipe perc-voc (shared gate: RecipeGate.h).
//
// v3 DESIGN NOTE - every preset ships finished (namespace v3 below)
//  * Research: commercial mallet instruments ship with their own chorus / reverb / EQ / drive on macros and the
//    vibraphone motor as a free-running amplitude tremolo (fans open / closed crossfaded); the marimba's "mud" sits around
//    300-500 Hz (a cut there, never a steep high-pass: the low bars carry the instrument); a pre-delay lets the mallet
//    read before a short tail. Bells want a big hall with the lows taken out of the return. Vocals: a plate (on an aux,
//    20-40 ms pre-delay, the return high-passed at 200-300 Hz and its top tamed), tempo delay "throws" into the reverb with
//    a steep high / low cut on the repeats, pop-vocal compression (3:1, fast); choirs get light bus glue, a cut of the
//    boxy band, one shared hall with pre-delay, the boom kept out of the tail; trap chops: short syllables, a punchy
//    attack, a dotted-eighth ping-pong throw, a plate, grit on demand; vocal pads: ensemble chorus + a long hall + slow
//    drift. Commercial patches name macros by playing gesture (mallet, motor, swell, vibrato, throw) and put vibrato /
//    expression on the mod wheel and aftertouch.
//  * So each preset's chain = FAMILY (the instrument's finished sound) + ROLE (the v2 treatment as a processing style),
//    slots in the canonical rack order; the fitted v2 EQ (the low shelf / top cut that made C5 comfortable) is kept and
//    the stereo image never gets wider than the fit allowed:
//      wood (marimba, xylophone)  transient, 2:1 ring glue, mud cut (~500 Hz, marimba only), stage hall 15 ms, an echo
//                                 ready (1/8D, silent)                                      macros Mallet, Hall, Body, Echo
//      vibes                      transient, glue, a whisper of chorus, plate 20 ms, the motor = LFO 1 on level + pan
//                                 (3.2 / 5.2 / 7 Hz, off as designed; the mod wheel plays it)   Mallet, Plate, Motor, Echo
//      glock / celesta / music box  top tamed, 1/8D ping-pong echo (3.5 kHz / 300 Hz cuts), dark plate, a tape wobble
//                                 ready on LFO 2                                             Tone, Plate, Wobble, Echo
//      bells / chimes / handbells clang band cut at 700 Hz, big hall with a 250 Hz low cut, a 1/4D echo ready (silent)
//                                 (Bloom = a slower strike and a longer ring)                Strike, Hall, Bloom, Echo
//      bowed / glass (held)       slow ensemble, long hall 35 ms, slow brightness + pan drift  Tone, Hall, Swell, Drift
//      choir                      1.6:1 bus glue, box cut (1.9 x f0), hall 30 ms / 250 Hz low cut, slow breathing LFO,
//                                 delayed 5.2 Hz vibrato (off as designed)                   Tone, Hall, Swell, Vibrato
//      solo voice                 3:1 vocal comp, box cut, 1/4 ping-pong throw, plate 30 ms, delayed vibrato
//                                                                                            Tone, Plate, Throw, Vibrato
//      breathy / whisper          soft comp, air tamed, throw, plate                         Tone, Plate, Throw, Width
//      chops / "ha" hits          transient, fast comp, tape grit (silent), 1/8D ping-pong throw, short plate
//                                                                                            Snap, Plate, Throw, Grit
//      vocal pads                 ensemble chorus, long hall 40 ms, slow drift (cutoff + pan) Tone, Space, Motion, Ensemble
//      synth choir                ensemble, hall, breathing, delayed vibrato                 Tone, Hall, Ensemble, Vibrato
//    roles: Concert Hall / Cathedral = their big hall kept, short pre-delay, darker, narrower; Felt / Intimate / Behind the
//    Wall = room ("Warmth"); Night / Trap = their delay with cut repeats ("Echo" / "Throw"); Dusty / Warped / Lo-Fi = tape
//    wobble LFO + room ("Tape" / "Crush"); Dampened / Stab = small room (a fast compressor when a chord of strikes peaks
//    too far); Blooming / Swelling = "Swell"; Reverse; Dream / Ambient / Haunted = cloud reverb + slow drift ("Drift");
//    Heavy / Twin / Glimmer / Glowing / Airy = the added layer on a macro ("Sub", "Octave", "Sparkle" (down only), "Halo",
//    "Breath"); Shimmer / Wide = "Chorus" / "Ensemble"; Gritty = drive with clean lows ("Grit"); Shadow = "Shadow".
//  * Performance: mod wheel = vibrato / motor depth (more reverb where there is none), aftertouch = expression on held
//    sounds, velocity keeps the touch against the compressor.
//  * Macros: every target sits exactly at its designed value at the knob's default (the preset loads as designed). Each
//    macro is rendered at 0 and 1 (C5) and a side that breaks a comfort rule the designed sound keeps (2-5 kHz, > 6 / 8 kHz,
//    fundamental (not on the inharmonic bells), clipping, +4 dB louder, longer tail) is shortened until it passes.
//  * The whole v3 sound is refitted at C5 before the macros are made (2-5 kHz <= 7.5 %, > 6 kHz <= 2 %, > 8 kHz <= 0.7 %,
//    centroid 80 Hz under the category ceiling, fundamental >= -2.5 dB on everything but the bells).
//  * Arp / gate: off everywhere except 4 rhythm presets with plain names, patterns from Init (rate / mode / octaves /
//    length / steps only), appended after the shipped presets: Marimba Arpeggio (up, 1/16), Music Box Eighths
//    (up-down, 1/8), Gated Choir (1/16 gate), Gated Vocal Pad (1/16 gate).
//  * Selection: the shipped v2 selection is pinned by name (the greedy search below is order-sensitive; PV_FULL=1 runs it
//    again), every shipped preset keeps its id and name; then the finished sounds go through the gate's own analysis
//    in order (the shared 3.5 distance, gate fails, make-up gain, the assembly QA's 8 kHz share and centroid ceiling).
//  * Result (VaultBankGen -recipe perc-voc -qa, library /home/user/cv-library): the 385 pinned shipped presets + 4 rhythm
//    presets -> 388 pass the gate and the assembly QA (Bells & Mallets 197, Vocals & Choir 191), min pairwise distance
//    3.53; 384 of the 385 shipped presets keep their id and name (Wide Gong Bell stays a near-duplicate of Gong Bell after
//    its role was pushed and is dropped); the rhythm presets get the next free ids. 11 finished sounds were fixed after a
//    gate failure (5 mono-loss: less ensemble chorus / width; 3 tails: a shorter hall / ring; 2 dead strokes whose chord
//    peaked > 6 dB over the limiter: a fast compressor; Reverse Glockenspiel: the 2-5 kHz band cut), 3 near-duplicates
//    got their role pushed; peak control at assembly fits 4. At C5: 2-5 kHz <= 9.1 % (bells) / 5.9 % (voices), > 8 kHz
//    <= 0.17 %, voices' fundamental >= -2.4 dB, centroid <= 1458 / 975 Hz (ceilings 2200 / 1500), velocity 0.35 -> 1.0
//    >= 5.3 / 3.3 dB, tails <= 10 / 8.9 s, phrase peak <= 1.9 dB over the limiter ceiling. 1,552 macros checked at 0 and
//    1: 371 range narrowings, none left outside the comfort rules; 90 (mostly slow drift / width / reverb amounts that a
//    2.5 s probe barely shows) measure as small changes.
//  * Sources: gearspace.com "Delay into reverb" (short reverbs into a high-passed multi-tap / ping-pong delay);
//    uaudio.com "Producer Presets Unpacked - Pure Plate Reverb" (vocal plate); sageaudio.com "How to mix vocal reverb"
//    (pre-delay 20-50 ms, high-passing the return at 200-300 Hz); soundverse.ai "How to mix choir vocals" and
//    forum.vsl.co.at (choir bus glue, per-section pre-delay, low cut on the tail, the boomy band); vi-control.net "EQ
//    muddy marimba" (cut around 400 Hz, no high-pass at 150 Hz); versilian-studios.com vVibraphone manual and
//    soniccouture vibraphone (motor tremolo from fans open / closed, chorus / reverb / drive macros).
//
// v2 DESIGN NOTE (research summary; sources at the end of this note)
//  * How professional mallet libraries are voiced: every bar instrument is sold as the same player in several
//    perspectives - close (attack, mallet "tock", dry: the pop / trap perspective), a stage / room pair (the default
//    balance) and far / hall (scoring). Vibraphones come motor OFF and motor ON (the rotating fans in the resonators give
//    an amplitude tremolo of ~3-7 Hz; libraries either record the motor or record fans open / closed and crossfade with an
//    LFO so the tremolo speed is free), soft / medium / hard mallets, open vs. "secco" / dead stroke, bowed notes and
//    rolls; marimba and xylophone come in soft / medium / hard mallets, sometimes rolls. Glockenspiel and xylophone sound
//    two / one octave above the written note, the celesta one octave: the C5 key (MIDI 60, 262 Hz) plays them where they
//    really sound - glockenspiel 1047 Hz (its 2.8x bar mode lands in 2-5 kHz: low-passed), xylophone and celesta 523 Hz.
//    Every other part keeps the written octave; the octave-up "sparkle" layers stay >= 12 dB under the main part.
//  * How trap / hip-hop producers use bells: a bright mallet or bell melody duplicated onto a second sound mixed in "just
//    loud enough to be noticed" and panned apart, an eighth or dotted-eighth delay plus a long reverb tail for the dark,
//    dreamy bell loop, octave doubling to thicken, a sub under a marimba for the "trap marimba", distortion -> EQ ->
//    reverb chains, tape / lo-fi wobble. Here those are ROLES (echo, dream, trap, heavy, twin, glimmer, cassette, gritty)
//    and LAYERS (trap bells = hard vibraphone + glockenspiel sparkle + low marimba body; marimba + vibes; celesta + harp).
//  * Bell acoustics behind the comfort rules: bars (marimba 1:4:10, vibraphone 1:4:10, xylophone 1:3:6) have a strong
//    fundamental - they must keep it the loudest partial at C5 (repaired by low-pass / a tone-bar support part).
//    Tubular bells and chimes are the intentionally inharmonic exception (their strike pitch is a virtual pitch an octave
//    under the 4th partial), so only the brightness caps apply. Hand chimes / handbells are tuned with fundamental + 12th.
//    The celesta (felt hammers on steel bars over wooden resonators, sounds an octave up) and the music box (clamped
//    steel comb teeth, 1 : ~6 overtone, strongly stretched tuning) come from the struck-rod keys model and the modal engine.
//  * How vocal libraries are organised (docs/VAULT_VOCAL_DESIGN.md): choirs by section (S/A/T/B, women / men / mixed),
//    ensemble size (solo, chamber, symphonic), vowel (ah / oh / oo / ee / closed hum), sustains vs. shorts ("ha" hits,
//    staccato syllables), swells, whispers / breaths, mic distance; pop vocal instruments split notes ("ooh", "ah",
//    "hey" one-shots), phrases and chops. Trap chops: single syllables triggered like drums, short = punchy, plate or
//    huge wet reverb, tempo-synced delay, formant shift instead of plain pitch shift, grit / bit-crush for aggressive
//    beats. Velocity should open the voice (brightness), not only the level.
//  * Material: real samples everywhere they exist (VCSL marimba, vibraphone incl. bowed, xylophone, glockenspiel, tubular
//    bells, concert + church chimes, hand chimes, glass harp; Fluid choir aahs / oohs (the solo "doo" voice is left out: its loops pulse ~10 dB), VocalSet female "eh"
//    chops straight + breathy, MTG female oo / soprano ah, vocobox low male "ah"); the voice models (female, male, choir
//    8-16 singers, vocal pad; articulations sung / "ha" / breathy / whisper; vowels ah / oh / oo / ee; formant shift)
//    for everything the recordings cannot do (sections, ensembles, hums, whispers, other vowels).
//
// HOW THE BANK IS MADE
//  * VOICES (instrument x articulation x register) and LAYERS (orchestration / production pairings, 2-4 parts) per
//    family, then ROLES per voice kind (struck, held, chop, pad): studio, hall, felt / intimate, echo, cassette, dead
//    stroke, bloom, behind the wall, reverse, dream, heavy sub, octave twin, glimmer octave, halo pad, shimmer chorus,
//    gritty, warped, ghost octave, trap delay; for voices also swell, cathedral, breath air, formant (young / giant),
//    straight tone. Roles change the sound's function, not just a parameter: no jitter variants.
//  * Every part is calibrated alone (-20 dB K at C5), then the candidate is REPAIRED by measurement (C5 at velocity 0.8
//    and 1.0): mono-safe width, high shelf / low-pass until 2-5 kHz <= 7 % and > 6 kHz <= 1.5 %, and for every harmonic
//    sound (all voices, every bar instrument except the bells) low shelf / low-pass / a unison support part (choir oohs,
//    a soft "oo" voice or a tone bar) until the fundamental is within 2.5 dB of the strongest partial. Then the shared
//    gate analysis runs, gate failures that have a mechanical fix (tail, velocity, octave layer, width, end click) are
//    fixed once and re-analysed.
//  * Distinctness: candidates come in priority order (plain voices, plain layers, then role after role). A candidate is
//    kept only when its fingerprint (RecipeGate.h: log-mel 24 bands x 6 segments at C5 + 3 at C4 and C6, 12 cepstral
//    coefficients of the held C5 spectrum = the MFCC-like part, C5 harmonic profile, AM depth, width) is >= PV_OWN_MIN
//    (default 4.6) from every preset kept before it - stricter than the shared -dist 3.5. Two more own rules: one role may
//    colour at most PV_ROLE_CAP (default 11) presets per category (no bank of "gritty everything"), and a preset the
//    loudness normalisation would lift by more than 18 dB (a faint fragment, e.g. a stab cut from a slow-onset note) is
//    dropped.
//  * Result (VaultBankGen -recipe perc-voc, library /home/user/cv-library): 2653 candidates -> 2629 repaired -> 398 kept
//    (Bells & Mallets 205, Vocals & Choir 193; 176 one-part, 179 two, 39 three, 4 four), all 398 pass the shared gate,
//    minimum pairwise distance 4.61. At C5: fundamental >= -2.5 dB of the strongest partial for every voice and every
//    harmonic bar (28 tubular / chime / bell-tower presets are the inharmonic exception), 2-5 kHz <= 7.9 %, > 6 kHz
//    <= 0.7 %, octave-up layers <= -12.8 dB, tails <= 10 s, velocity 0.35 -> 1.0 opens 4.7-30.7 dB, loudness at the
//    category target (normalisation gain <= 17.6 dB).
//  Measured choices: the voice-model choir alone beats 7-10 dB at ~2.5 Hz on "oo" (unison partials), so every "oo"
//  choir is led by the recorded choir; the recorded solo "doo" voice pulses ~10 dB at its loop points and is not used;
//  the model choir's "ha" does not decay, so choir hits are the recorded aahs / oohs with a 0.38 s decay plus a model
//  "ha" attack; vibraphone motor = the real vibraphone attack over a modal bar body with a 2.8-7.5 Hz amplitude LFO
//  (the sampler has no tremolo), measured 6.5 dB AM at 5 Hz.
//  Sources: vsl.co.at Synchron Mallets (motor on / off, secco, rolls, multi-mic); soniccouture / rootsounds vibraphone
//  pages (fans open / closed crossfade, phase-locked motor speed); fluffy audio "My Vibes" (mallets, mic positions);
//  bonedo.de trap crash course (bell melody, layer duplication, 1/8 delay + long reverb); create.routenote.com
//  "Mastering vocal chops in trap beats" and voxbooster trap chop guide (short syllables, wet reverb, synced delay,
//  formant shift, grit); hibberts.co.uk tubular bells + Wikipedia "Strike tone" (virtual strike pitch); vsl.co.at
//  celesta; mmdigest.com music box comb tuning (stretched combs); docs/VAULT_VOCAL_DESIGN.md (choir / pop vocal library
//  organisation, ear-safety rules for voices); UNSW music acoustics (bar mode ratios).
//
// Environment (optional): PV_LIMIT (first N candidates, smoke test), PV_THREADS (default 3), PV_OWN_MIN (default 4.6),
// PV_ROLE_CAP (default 11), PV_VERBOSE (1 = print every near-duplicate drop with its nearest kept preset); v3: PV_FULL (1 =
// the whole v2 search instead of the pinned shipped selection), PV_V2CACHE (<file.cvb>: the v2 selection saved once with
// its roles), PV_V3 (0 = stop after the v2 selection), PV_ONLY (ids / names), PV_SELFGATE / PV_MACROCHECK (0 = skip),
// PV_MACROLOG (1 = every macro measurement), PV_TSV (<file>: the gate numbers of every finished preset).
#include "RecipeGate.h"
#include <map>
#include <mutex>
#include <algorithm>
#include <set>
#include <atomic>
#include <thread>

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

//==============================================================================
// parts
const KV bar { { "s.velSens", 0.6f }, { "s.velCutoff", 0.3f } };
VaultPart marimba (int o = 0)    { return S ("mallets.marimba", "hit", "Marimba", o, bar); }
VaultPart vibSoft (int o = 0)    { return S ("mallets.vibraphone", "soft mallets", "Vibraphone", o, bar); }
VaultPart vibHard (int o = 0)    { return S ("mallets.vibraphone", "hard mallets", "Vibraphone", o, bar); }
VaultPart vibBowed (int o = 0)   { return S ("mallets.vibraphone", "bowed", "Bowed Vibraphone", o, { { "s.velSens", 0.6f } }); }
VaultPart xylo (const char* art, int o = 1) { return S ("mallets.xylophone", art, "Xylophone", o, bar); }
VaultPart glockMain (float cut = 6000.0f) { return S ("mallets.glockenspiel", "hit", "Glockenspiel", 2, { { "s.velSens", 0.6f }, { "s.velCutoff", 0.3f }, { "s.cutoff", cut } }); }
VaultPart glock (float l, float pan = 0.2f) { return lvl (S ("mallets.glockenspiel", "hit", "Glockenspiel", 2, { { "s.velSens", 0.6f }, { "s.cutoff", 7000.0f } }), l, pan); }
VaultPart tubular (int o = 0)    { return S ("bells.tubular", "hit", "Tubular Bells", o, bar); }
VaultPart chimes (int o = 0)     { return S ("bells.chimes", "hit", "Concert Chimes", o, bar); }
VaultPart church (int o = 0)     { return S ("bells.chimes.chromatic", "hit", "Church Chimes", o, bar); }
VaultPart handChimes (int o = 0) { return S ("bells.handchimes", "hit", "Hand Chimes", o, bar); }
VaultPart glass (const char* art = "slow", int o = 1) { return S ("fx.glass", art, "Glass Harp", o, { { "s.velSens", 0.6f } }); }
VaultPart celesta (int o = 1, const char* label = "Celesta")
{
    return M ("keys.toy", label, o, { { "m.tone", 0.5f }, { "m.bell", 1.0f }, { "m.decay", 2.0f }, { "m.release", 0.9f }, { "m.velSens", 0.7f } });
}
VaultPart steelBars (int o = 1)
{
    return M ("keys.toy", "Glockenspiel Bars", o, { { "m.tone", 0.62f }, { "m.bell", 1.3f }, { "m.decay", 2.2f }, { "m.release", 1.4f }, { "m.velSens", 0.7f } });
}
// music box comb: clamped steel teeth (modal engine, 1 : 5.4 : 11.3 modes), short ring, a touch of stretch
VaultPart musicBox (int o = 1)
{
    return Y ("Music Box", o, { { "engine", 9 }, { "wave", 0.1f }, { "fmAmt", 0.22f }, { "filterType", 0 }, { "cutoff", 5200 }, { "keyTrack", 0.5f },
                                { "attack", 0.001f }, { "decay", 1.1f }, { "sustain", 0.0f }, { "release", 0.9f }, { "velSens", 0.7f }, { "fine", 4.0f } });
}
// tone bar: a soft modal bar (vibraphone-like 1 : 3.9 : 9.2 modes, upper modes quiet) - motor vibes body, support part
VaultPart toneBar (int o, float decay = 4.5f, float trem = 0.0f, float rate = 5.0f, const char* label = "Tone Bar")
{
    return Y (label, o, { { "engine", 9 }, { "wave", 0.5f }, { "fmAmt", 0.02f }, { "filterType", 0 }, { "cutoff", 1900 }, { "keyTrack", 0.6f },
                          { "attack", 0.004f }, { "decay", decay }, { "sustain", 1.0f }, { "release", 0.7f }, { "velSens", 0.7f },
                          { "lfoRate", rate }, { "lfoAmp", trem }, { "lfoShape", 0 } });
}
VaultPart bellTower (int o = 0)
{
    return Y ("Bell Tower", o, { { "engine", 9 }, { "wave", 0.9f }, { "fmAmt", 0.15f }, { "filterType", 0 }, { "cutoff", 2600 }, { "keyTrack", 0.4f },
                                 { "attack", 0.002f }, { "decay", 3.2f }, { "sustain", 0.0f }, { "release", 2.2f }, { "velSens", 0.7f } });
}
VaultPart fmBell (int o = 0, float ratio = 3.5f, float amt = 0.16f, const char* label = "FM Bell")
{
    return Y (label, o, { { "engine", 1 }, { "fmAlgo", 0 }, { "fmRatio", ratio }, { "fmAmt", amt }, { "wave", 0.0f }, { "filterType", 0 }, { "cutoff", 3600 },
                          { "keyTrack", 0.5f }, { "fenv", 0.0f }, { "attack", 0.002f }, { "decay", 2.4f }, { "sustain", 0.0f }, { "release", 1.6f },
                          { "velSens", 0.7f } });
}
VaultPart softPad (float l, int oct = 0)
{
    return lvl (Y ("Soft Pad", oct, { { "engine", 0 }, { "wave", 0.1f }, { "unison", 4 }, { "detune", 0.18f }, { "filterType", 0 }, { "cutoff", 1000 },
                                    { "reso", 0.05f }, { "keyTrack", 0.3f }, { "attack", 0.7f }, { "decay", 1.0f }, { "sustain", 0.85f },
                                    { "release", 1.3f }, { "velSens", 0.3f } }), l);
}
VaultPart subSine (float l, int oct)
{
    return lvl (Y ("Sine Bass", oct, { { "engine", 6 }, { "wave", 0.0f }, { "fmAmt", 0.0f }, { "filterType", 0 }, { "cutoff", 2000 },
                                       { "attack", 0.004f }, { "decay", 0.9f }, { "sustain", 0.0f }, { "release", 0.25f }, { "velSens", 0.5f } }), l);
}
VaultPart synthChoir (int o = 0)
{
    return Y ("Synth Choir", o, { { "engine", 3 }, { "wave", 0.0f }, { "unison", 3 }, { "detune", 0.12f }, { "filterType", 0 }, { "cutoff", 2400 },
                                  { "keyTrack", 0.4f }, { "attack", 0.35f }, { "decay", 1.0f }, { "sustain", 0.9f }, { "release", 1.2f },
                                  { "velSens", 0.85f }, { "lfoRate", 5.0f }, { "lfoPitch", 0.05f } });
}

// voices (model): vowel 0 ah, 1 oh, 2 oo, 3 ee; artic 0 sung, 1 "ha", 2 breathy, 3 whisper
VaultPart female (float vowel, float artic = 0, int o = 0, const KV& kv = {})
{
    return with (M ("voice.female", "Female Voice", o, { { "m.vowel", vowel }, { "m.artic", artic }, { "m.velSens", 1.0f } }), kv);
}
VaultPart male (float vowel, float artic = 0, int o = -1, const KV& kv = {})
{
    return with (M ("voice.male", "Male Voice", o, { { "m.vowel", vowel }, { "m.artic", artic }, { "m.velSens", 1.0f } }), kv);
}
VaultPart choir (float vowel, float gender, float voices, int o = 0, const KV& kv = {})
{
    return with (M ("voice.choir", "Choir", o, { { "m.vowel", vowel }, { "m.gender", gender }, { "m.voices", voices }, { "m.velSens", 1.0f } }), kv);
}
VaultPart vpad (float vowel, float gender, int o = 0, const KV& kv = {})
{
    return with (M ("voice.pad", "Vocal Pad", o, { { "m.vowel", vowel }, { "m.gender", gender }, { "m.velSens", 1.0f } }), kv);
}
const KV vox { { "s.velSens", 0.6f }, { "s.velCutoff", 0.25f } };
VaultPart oohs (int o = 0)   { return S ("voice.choir.classic", "oohs", "Choir Oohs", o, vox); }
VaultPart aahs (int o = 0)   { return S ("voice.choir.classic", "aahs", "Choir Aahs", o, vox); }
VaultPart chop (const char* art, int o = 0) { return S ("voice.female.chops", art, "Female Vocal Chop", o, vox); }
VaultPart femOo (int o = 0)  { return S ("voice.female.solo", "oo", "Female Solo Voice", o, vox); }
VaultPart sopAh (int o = 0)  { return S ("voice.female.solo", "ah", "Soprano Voice", o, vox); }
VaultPart maleLow (int o = -1) { return S ("voice.male.low", "ah", "Low Male Voice", o, vox); }

// the voice-model choir over the real choir recording (oohs for oh / oo, aahs for ah): the recording steadies the
// model's unison beating (measured 7-10 dB amplitude fluctuation at ~2.5 Hz on an "oo" alone) and adds real air
std::vector<VaultPart> choirBlend (float vowel, float gender, float voices, int o = 0, const KV& kv = {}, float sampleLevel = -1.0f)
{
    if (vowel >= 1.5f)   // an "oo" is nearly one partial per singer: the model alone beats 10 dB, so the recording leads
        return { oohs (o), lvl (choir (vowel, gender, voices, o, kv), 0.45f, -0.15f) };
    if (sampleLevel < 0.0f) sampleLevel = vowel < 0.5f ? 0.6f : 1.0f;
    return { choir (vowel, gender, voices, o, kv), lvl (vowel < 0.5f ? aahs (o) : oohs (o), sampleLevel, 0.15f) };
}
// a real choir "hit": the recorded aahs + oohs with a short decay, the aspirated "ha" of the voice model on top
const KV stab { { "s.decay", 0.38f }, { "s.sustain", 0.0f }, { "s.oneShot", 0 }, { "s.release", 0.15f } };
std::vector<VaultPart> choirHit (int o, bool men)
{
    return { with (aahs (o), stab), lvl (with (oohs (o), stab), 0.6f, 0.2f), lvl (men ? male (0, 1, o) : female (0, 1, o), 0.35f, -0.15f) };
}

//==============================================================================
enum Kind { kStrike = 1, kHeld = 2, kChop = 4, kPad = 8 };

struct Voice
{
    juce::String noun;
    int category;
    std::vector<VaultPart> parts;
    int kind;
    bool bell = false;   // intentionally inharmonic (tubular bells, chimes, bell towers): no fundamental repair
};

std::vector<Voice> malletVoices()
{
    const int B = vault::catBellsMallets;
    return {
        { "Marimba",               B, { marimba (0) }, kStrike },
        { "Vibraphone",            B, { vibSoft (0) }, kStrike },
        { "Xylophone",             B, { xylo ("medium mallets") }, kStrike },
        { "Glockenspiel",          B, { glockMain() }, kStrike },
        { "Tubular Bells",         B, { tubular (0) }, kStrike, true },
        { "Celesta",               B, { celesta (1) }, kStrike },
        { "Hand Chimes",           B, { handChimes (0) }, kStrike },
        { "Glass Harp",            B, { glass ("slow") }, kHeld },
        { "Music Box Comb",        B, { musicBox (1) }, kStrike },
        { "Bowed Vibraphone",      B, { vibBowed (0) }, kHeld },
        { "Motor Vibes",           B, { with (vibSoft (0), { { "s.decay", 0.7f }, { "s.sustain", 0.0f }, { "s.oneShot", 0 }, { "s.release", 0.4f } }),
                                        lvl (toneBar (0, 5.0f, 0.55f, 5.2f, "Vibraphone Motor"), 0.95f) }, kStrike },
        { "Deep Marimba",          B, { marimba (-1) }, kStrike },
        { "Hard Vibraphone",       B, { vibHard (0) }, kStrike },
        { "Hard Xylophone",        B, { xylo ("hard mallets") }, kStrike },
        { "Concert Chimes",        B, { chimes (0) }, kStrike, true },
        { "Church Chimes",         B, { church (0) }, kStrike, true },
        { "Wine Glass Tremolo",    B, { glass ("fast") }, kHeld },
        { "High Marimba",          B, { marimba (1) }, kStrike },
        { "High Vibraphone",       B, { vibSoft (1) }, kStrike },
        { "Soft Xylophone",        B, { xylo ("soft mallets") }, kStrike },
        { "High Hand Chimes",      B, { handChimes (1) }, kStrike },
        { "Low Celesta",           B, { celesta (0) }, kStrike },
        { "Low Music Box Comb",    B, { musicBox (0) }, kStrike },
        { "High Bowed Vibraphone", B, { vibBowed (1) }, kHeld },
        { "Slow Motor Vibes",      B, { with (vibSoft (0), { { "s.decay", 0.9f }, { "s.sustain", 0.0f }, { "s.oneShot", 0 }, { "s.release", 0.5f } }),
                                        lvl (toneBar (0, 5.5f, 0.7f, 2.8f, "Vibraphone Motor"), 0.95f) }, kStrike },
        { "Fast Motor Vibes",      B, { with (vibHard (0), { { "s.decay", 0.5f }, { "s.sustain", 0.0f }, { "s.oneShot", 0 }, { "s.release", 0.3f } }),
                                        lvl (toneBar (0, 4.0f, 0.45f, 7.5f, "Vibraphone Motor"), 0.9f) }, kStrike },
        { "Shimmer Vibes",         B, { lvl (with (vibSoft (0), { { "s.tune", -7.0f } }), 0.9f, -0.5f), lvl (with (vibSoft (0), { { "s.tune", 7.0f } }), 0.9f, 0.5f) }, kStrike },
        { "High Hard Vibraphone",  B, { vibHard (1) }, kStrike },
        { "Bell Tower",            B, { bellTower (0) }, kStrike, true },
        { "Crystal Bell",          B, { fmBell (1, 3.5f, 0.14f, "FM Bell") }, kStrike, true },
        { "Handbell",              B, { handChimes (0), lvl (fmBell (0, 3.0f, 0.08f, "Handbell"), 0.45f) }, kStrike },
        { "Steel Glockenspiel",    B, { steelBars (1), glock (0.2f) }, kStrike },
        { "Felt Glockenspiel",     B, { glockMain (2600.0f) }, kStrike },
        { "High Music Box Comb",   B, { musicBox (2) }, kStrike },
        { "Dead Stroke Vibes",     B, { with (vibHard (0), { { "s.decay", 0.16f }, { "s.sustain", 0.0f }, { "s.oneShot", 0 }, { "s.release", 0.08f } }) }, kStrike },
        { "Low Bell Tower",        B, { bellTower (-1) }, kStrike, true },
        { "Ice Crystal Bell",      B, { fmBell (1, 7.0f, 0.1f, "FM Bell") }, kStrike, true },
        { "Gong Bell",             B, { with (fmBell (0, 1.41f, 0.14f, "FM Bell"), { { "decay", 3.5f }, { "release", 2.4f } }) }, kStrike, true },
        { "Toy Celesta",           B, { with (celesta (1, "Celesta"), { { "m.tone", 0.6f }, { "m.bell", 1.4f }, { "m.decay", 0.6f } }) }, kStrike },
    };
}

std::vector<Voice> malletLayers()
{
    const int B = vault::catBellsMallets;
    return {
        { "Marimba and Vibes",      B, { marimba (0), lvl (vibSoft (0), 0.6f, 0.3f) }, kStrike },
        { "Trap Bells",             B, { vibHard (0), glock (0.12f, 0.3f), lvl (marimba (-1), 0.35f, -0.2f) }, kStrike },
        { "Celesta and Harp",       B, { celesta (1), lvl (S ("pluck.harp.concert", "pluck", "Concert Harp", 0), 0.45f, -0.25f) }, kStrike },
        { "Sugar Plum",             B, { celesta (1), lvl (glass ("slow", 1), 0.18f, 0.3f) }, kStrike },
        { "Vibes and Tines",        B, { vibSoft (0), lvl (M ("ep.tine.mellow", "Tine Electric Piano"), 0.45f, -0.2f) }, kStrike },
        { "Piano and Vibes",        B, { vibHard (0), lvl (S ("piano.grand.studio", "sustain", "Studio Grand"), 0.5f, -0.2f) }, kStrike },
        { "Chimes and Marimba",     B, { handChimes (0), lvl (marimba (0), 0.55f, -0.3f) }, kStrike },
        { "Bowed Glass",            B, { vibBowed (0), lvl (glass ("slow", 1), 0.15f, 0.3f) }, kHeld },
        { "Handbell Choir",         B, { handChimes (0), lvl (handChimes (1), 0.12f, 0.35f), lvl (fmBell (0, 3.0f, 0.08f, "Handbell"), 0.35f, -0.3f) }, kStrike },
        { "Cathedral Bells",        B, { tubular (0), lvl (church (0), 0.55f, 0.35f), lvl (oohs (-1), 0.18f, -0.2f) }, kStrike, true },
        { "Bells and Choir",        B, { tubular (0), lvl (choir (2, 0.6f, 12), 0.3f) }, kStrike, true },
        { "Music Box and Strings",  B, { musicBox (1), lvl (S ("strings.violins.ensemble", "sustain", "Violin Section", 0, { { "s.attack", 0.3f } }), 0.3f) }, kStrike },
        { "Vibes and Flute",        B, { vibSoft (0), lvl (S ("winds.flute.concert", "sustain", "Concert Flute", 0, { { "s.width", 0.6f } }), 0.35f, 0.25f) }, kStrike },
        { "Midnight Bells",         B, { vibBowed (0), lvl (handChimes (0), 0.45f, 0.3f) }, kHeld },
        { "Twin Marimbas",          B, { lvl (marimba (0), 0.85f, -0.45f), lvl (with (marimba (0), { { "s.tune", 7.0f } }), 0.75f, 0.45f) }, kStrike },
        { "Marimba and Thumb Piano", B, { marimba (0), lvl (S ("world.kalimba", "pluck", "Thumb Piano"), 0.5f, 0.3f) }, kStrike },
        { "Marimba and Nylon",      B, { marimba (0), lvl (M ("guitar.nylon", "Nylon Guitar"), 0.4f, -0.3f) }, kStrike },
        { "Vibes and Upright Bass", B, { vibSoft (0), lvl (S ("strings.contrabass", "pizzicato", "Contrabass", -2), 0.5f) }, kStrike },
        { "Xylophone and Marimba",  B, { marimba (0), lvl (xylo ("medium mallets"), 0.2f, 0.3f) }, kStrike },
        { "Xylophone and Glockenspiel", B, { xylo ("soft mallets"), glock (0.12f, -0.25f) }, kStrike },
        { "Ice Bells",              B, { glass ("slow", 1), lvl (celesta (1), 0.5f, -0.3f), glock (0.08f, 0.3f) }, kHeld },
        { "Bells and Timpani",      B, { tubular (0), lvl (S ("perc.timpani", "hit", "Timpani", -1), 0.4f) }, kStrike, true },
        { "Marimba over Pizzicato", B, { marimba (0), lvl (S ("strings.violins.ensemble", "pizzicato", "Violin Section"), 0.4f, 0.3f) }, kStrike },
        { "Vibes and Choir",        B, { vibSoft (0), lvl (oohs (0), 0.3f, -0.2f) }, kStrike },
        { "Celesta and Strings",    B, { celesta (1), lvl (S ("strings.violins.ensemble", "sustain", "Violin Section", 0, { { "s.attack", 0.25f } }), 0.3f) }, kStrike },
        { "Music Box and Celesta",  B, { musicBox (1), lvl (celesta (1), 0.5f, 0.3f) }, kStrike },
        { "Trap Mallet Stack",      B, { marimba (0), lvl (vibHard (0), 0.5f, 0.3f), glock (0.1f, -0.3f), subSine (0.3f, -1) }, kStrike },
        { "Dark Trap Bells",        B, { tubular (0), lvl (vibSoft (0), 0.45f, -0.25f), subSine (0.3f, -1) }, kStrike, true },
        { "Glass Celesta",          B, { celesta (1), lvl (glass ("fast", 1), 0.14f, 0.3f) }, kStrike },
        { "Bell Pad",               B, { handChimes (0), lvl (vpad (2, 0.8f), 0.3f) }, kStrike },
        { "Chimes and Celesta",     B, { chimes (0), lvl (celesta (1), 0.35f, 0.3f) }, kStrike, true },
        { "Wooden Bells",           B, { marimba (0), lvl (handChimes (0), 0.4f, 0.3f), lvl (bellTower (0), 0.2f, -0.3f) }, kStrike },
        { "Music Box Lullaby",      B, { musicBox (1), lvl (S ("pluck.harp.folk", "pluck", "Folk Harp"), 0.4f, -0.3f), softPad (0.2f) }, kStrike },
        { "Vibes and Celesta",      B, { vibSoft (0), lvl (celesta (1), 0.3f, 0.3f) }, kStrike },
        { "Glockenspiel and Harp",  B, { glockMain(), lvl (S ("pluck.harp.concert", "pluck", "Concert Harp", 2), 0.4f, -0.3f) }, kStrike },
        { "Marimba and Glockenspiel", B, { marimba (0), glock (0.1f, 0.3f) }, kStrike },
        { "Snowfall Bells",         B, { handChimes (0), lvl (glass ("slow", 1), 0.12f, 0.3f), softPad (0.2f) }, kStrike },
        { "Struck and Bowed Vibes", B, { vibSoft (0), lvl (vibBowed (0), 0.5f, 0.25f) }, kStrike },
        { "Marimba Swell",          B, { marimba (0), lvl (with (vibBowed (0), { { "s.attack", 0.35f } }), 0.45f, -0.25f) }, kStrike },
        { "Celestial Choir Bells",  B, { celesta (1), lvl (oohs (0), 0.3f, -0.2f) }, kStrike },
        { "Glass and Choir",        B, { glass ("slow", 1), lvl (oohs (0), 0.35f, 0.2f) }, kHeld },
        { "Music Box and Glass",    B, { musicBox (1), lvl (glass ("slow", 1), 0.2f, -0.3f) }, kStrike },
        { "Bell Vibes",             B, { vibHard (0), glock (0.14f, 0.25f) }, kStrike },
        { "Xylophone and Pizzicato", B, { xylo ("soft mallets"), lvl (S ("strings.violins.ensemble", "pizzicato", "Violin Section", 1), 0.45f, -0.25f) }, kStrike },
        { "Sunday Bells",           B, { church (0), lvl (S ("organ.pipe", "soft", "Pipe Organ"), 0.25f, -0.2f) }, kStrike, true },
        { "Bell Strings",           B, { tubular (0), lvl (S ("strings.violins.ensemble", "sustain", "Violin Section", 0, { { "s.attack", 0.4f } }), 0.3f, 0.25f) }, kStrike, true },
        { "Vibes and Nylon",        B, { vibSoft (0), lvl (M ("guitar.nylon", "Nylon Guitar"), 0.4f, -0.3f) }, kStrike },
        { "Chimes and Glockenspiel", B, { handChimes (0), glock (0.12f, -0.25f) }, kStrike },
        { "Vibes and Strings",      B, { vibSoft (0), lvl (S ("strings.violas.ensemble", "sustain", "Viola Section", 0, { { "s.attack", 0.3f } }), 0.3f, -0.2f) }, kStrike },
    };
}

std::vector<Voice> vocalVoices()
{
    const int V = vault::catVocalsChoir;
    return {
        { "Female Eh Chop",        V, { chop ("eh") }, kChop },
        { "Choir Oohs",            V, { oohs (0) }, kHeld },
        { "Female Ah",             V, { female (0) }, kHeld },
        { "Male Ah",               V, { male (0) }, kHeld },
        { "Chamber Choir",         V, choirBlend (0, 0.6f, 8), kHeld },
        { "Vocal Pad",             V, { vpad (0, 0.8f) }, kPad },
        { "Female Oo",             V, { femOo (0) }, kHeld },
        { "Soprano Ah",            V, { sopAh (0) }, kHeld },
        { "Female Ha",             V, { female (0, 1) }, kChop },
        { "Breathy Eh Chop",       V, { chop ("eh_breathy") }, kChop },
        { "Choir Aahs",            V, { aahs (0) }, kHeld },
        { "Male Ha",               V, { male (0, 1) }, kChop },
        { "Female Whisper",        V, { female (0, 3) }, kHeld },
        { "Women's Choir",         V, choirBlend (2, 1.0f, 12), kHeld },
        { "Men's Choir",           V, choirBlend (1, 0.0f, 12, -1), kHeld },
        { "Choir Hits",            V, choirHit (0, false), kChop },
        { "Breathy Female",        V, { female (2, 2) }, kHeld },
        { "Male Oh",               V, { male (1) }, kHeld },
        { "Female Oh",             V, { female (1) }, kHeld },
        { "Female Hum",            V, { female (2, 0, 0, { { "m.bright", 0.2f }, { "m.vibrato", 0.15f } }) }, kHeld },
        { "Male Hum",              V, { maleLow (-1), lvl (male (2, 0, -1, { { "m.bright", 0.25f } }), 0.6f) }, kHeld },
        { "Oo Vocal Pad",          V, { vpad (2, 0.9f) }, kPad },
        { "Male Vocal Pad",        V, { vpad (1, 0.1f, -1) }, kPad },
        { "Low Choir Oohs",        V, { oohs (-1) }, kHeld },
        { "High Choir Oohs",       V, { oohs (1) }, kHeld },
        { "Low Choir Aahs",        V, { aahs (-1) }, kHeld },
        { "High Choir Aahs",       V, { aahs (1) }, kHeld },
        { "High Eh Chop",          V, { chop ("eh", 1) }, kChop },
        { "High Breathy Chop",     V, { chop ("eh_breathy", 1) }, kChop },
        { "Tenor Ah",              V, { male (0, 0, 0) }, kHeld },
        { "Male Oo",               V, { male (2) }, kHeld },
        { "Female Ee",             V, { female (3, 0, 0, { { "m.bright", 0.3f } }) }, kHeld },
        { "Breathy Male",          V, { male (1, 2) }, kHeld },
        { "Male Whisper",          V, { male (0, 3) }, kHeld },
        { "Female Ho",             V, { female (1, 1) }, kChop },
        { "Female Hoo",            V, { female (2, 1) }, kChop },
        { "Male Ho",               V, { male (1, 1) }, kChop },
        { "Symphonic Choir",       V, choirBlend (0, 0.6f, 16, 0, { { "m.spread", 0.9f } }, 0.8f), kHeld },
        { "Mixed Choir Oh",        V, choirBlend (1, 0.5f, 12), kHeld },
        { "Choir Hum",             V, choirBlend (2, 0.5f, 12, 0, { { "m.bright", 0.15f } }), kHeld },
        { "Boys Choir",            V, choirBlend (2, 1.0f, 10, 0, { { "m.formant", 0.55f }, { "m.vibrato", 0.0f }, { "m.detune", 6.0f } }), kHeld },
        { "Men's Choir Ah",        V, choirBlend (0, 0.0f, 12, -1), kHeld },
        { "Breathy Choir",         V, { choir (2, 0.6f, 12, 0, { { "m.artic", 2 } }) }, kHeld },
        { "Whisper Choir",         V, { choir (0, 0.6f, 12, 0, { { "m.artic", 3 } }) }, kHeld },
        { "Men's Choir Hits",      V, choirHit (-1, true), kChop },
        { "Women's Choir Ah",      V, choirBlend (0, 1.0f, 12), kHeld },
        { "Drifting Vowels",       V, { vpad (1, 0.6f, 0, { { "m.motion", 1.0f } }) }, kPad },
        { "Synth Choir",           V, { synthChoir (0) }, kHeld },
        { "Low Synth Choir",       V, { synthChoir (-1) }, kHeld },
        { "Female Ah Vibrato",     V, { female (0, 0, 0, { { "m.vibrato", 0.75f } }) }, kHeld },
        { "Straight Female Oo",    V, { female (2, 0, 0, { { "m.vibrato", 0.0f } }) }, kHeld },
        { "Low Female Ah",         V, { female (0, 0, -1) }, kHeld },
        { "Female Hee",            V, { female (3, 1, 0, { { "m.bright", 0.3f } }) }, kChop },
        { "Tenor Ha",              V, { male (0, 1, 0) }, kChop },
        { "High Choir Hits",       V, choirHit (1, false), kChop },
        { "Choir Ee",              V, { choir (3, 0.6f, 12, 0, { { "m.bright", 0.25f } }) }, kHeld },
        { "Tenor Oh",              V, { male (1, 0, 0) }, kHeld },
        { "Female Aw",             V, { female (0.5f) }, kHeld },
        { "Breathy Female Ah",     V, { female (0, 2) }, kHeld },
        { "Whispered Oo",          V, { female (2, 3) }, kHeld },
        { "Ee Vocal Pad",          V, { vpad (3, 0.9f, 0, { { "m.bright", 0.3f } }) }, kPad },
        { "Low Vocal Pad",         V, { vpad (0, 0.5f, -1) }, kPad },
        { "Bass Voice",            V, { male (1, 0, -2, { { "m.formant", -0.5f } }) }, kHeld },
    };
}

std::vector<Voice> vocalLayers()
{
    const int V = vault::catVocalsChoir;
    auto violins = [] (float a = 0.3f) { return S ("strings.violins.ensemble", "sustain", "Violin Section", 0, { { "s.attack", a } }); };
    auto cellos = [] (int o = -1) { return S ("strings.cellos.ensemble", "sustain", "Cello Section", o, { { "s.attack", 0.3f } }); };
    return {
        { "Choir and Strings",      V, { oohs (0), lvl (violins(), 0.4f, 0.25f) }, kHeld },
        { "Voice and Strings",      V, { female (2), lvl (S ("strings.violas.ensemble", "sustain", "Viola Section", 0, { { "s.attack", 0.3f } }), 0.35f, -0.25f) }, kHeld },
        { "Angel Choir",            V, { oohs (0), lvl (choir (2, 1.0f, 12), 0.45f, -0.15f), lvl (femOo (0), 0.4f, 0.2f), lvl (glass ("slow", 1), 0.08f, -0.3f) }, kHeld },
        { "SATB Choir",             V, { choir (0, 1.0f, 8), lvl (choir (1, 0.0f, 8, -1), 0.7f), lvl (oohs (0), 0.35f, 0.2f) }, kHeld },
        { "Choir Stab",             V, { with (aahs (0), stab), lvl (with (oohs (0), stab), 0.5f, 0.2f), lvl (S ("brass.horn", "staccato", "French Horn", -1), 0.45f, -0.2f) }, kChop },
        { "Ha Stack",               V, { female (0, 1), lvl (male (0, 1), 0.6f) }, kChop },
        { "Chop and Bell",          V, { chop ("eh"), lvl (vibHard (0), 0.3f, 0.3f) }, kChop },
        { "Whisper Pad",            V, { vpad (2, 0.7f), lvl (female (0, 3), 0.4f, 0.2f) }, kPad },
        { "Choir and Organ",        V, { oohs (0), lvl (S ("organ.chamber", "8 foot", "Chamber Organ"), 0.3f) }, kHeld },
        { "Choir and Harp",         V, { oohs (0), lvl (S ("pluck.harp.concert", "pluck", "Concert Harp"), 0.45f, 0.3f) }, kHeld },
        { "Soprano and Choir",      V, { sopAh (0), lvl (oohs (0), 0.5f, 0.25f) }, kHeld },
        { "Male Hum and Choir",     V, { maleLow (-1), lvl (oohs (-1), 0.6f, 0.15f), lvl (choir (2, 0.0f, 12, -1), 0.35f, -0.15f) }, kHeld },
        { "Vocal Pad and Strings",  V, { vpad (0, 0.8f), lvl (violins (0.6f), 0.3f, -0.25f) }, kPad },
        { "Duet Ah",                V, { female (0), lvl (male (0), 0.6f, 0.0f) }, kHeld },
        { "Choir and Horn",         V, { oohs (0), lvl (S ("brass.horn", "sustain", "French Horn", -1), 0.35f, -0.2f) }, kHeld },
        { "Choir and Piano",        V, { oohs (0), lvl (S ("piano.grand.studio", "sustain", "Studio Grand"), 0.4f, 0.2f) }, kHeld },
        { "Voice and Flute",        V, { femOo (0), lvl (S ("winds.flute.concert", "sustain", "Concert Flute", 0, { { "s.width", 0.6f } }), 0.3f, 0.25f) }, kHeld },
        { "Choir and Bells",        V, { choir (1, 0.6f, 12), lvl (tubular (0), 0.25f, 0.3f) }, kHeld },
        { "Breath Choir",           V, { choir (2, 0.6f, 12, 0, { { "m.artic", 2 } }), lvl (female (2, 3), 0.3f, -0.3f) }, kHeld },
        { "Oo Pad and Glass",       V, { vpad (2, 0.9f), lvl (glass ("slow", 1), 0.1f, 0.3f) }, kPad },
        { "Basso Profundo",         V, { choir (1, 0.0f, 12, -1), lvl (S ("strings.contrabass", "sustain", "Contrabass", -2, { { "s.attack", 0.2f } }), 0.3f) }, kHeld },
        { "Choir Aahs and Cellos",  V, { aahs (0), lvl (oohs (0), 0.5f), lvl (cellos (-1), 0.3f, 0.3f) }, kHeld },
        { "Chops and Choir",        V, { chop ("eh_breathy"), lvl (oohs (0), 0.35f, 0.25f) }, kChop },
        { "Two Sopranos",           V, { lvl (sopAh (0), 0.85f, -0.4f), lvl (female (0, 0, 0, { { "m.vibrato", 0.6f } }), 0.75f, 0.4f) }, kHeld },
        { "Voices and Synth Choir", V, { choir (0, 0.6f, 8), lvl (synthChoir (0), 0.35f, 0.25f) }, kHeld },
        { "Female Oo and Pad",      V, { femOo (0), softPad (0.3f) }, kHeld },
        { "Requiem",                V, { aahs (-1), lvl (choir (1, 0.3f, 12, -1), 0.6f), lvl (cellos (-1), 0.25f, 0.3f), lvl (S ("perc.timpani", "roll", "Timpani", -1), 0.12f) }, kHeld },
        { "Ghost Duet",             V, { female (2, 2), lvl (male (2, 2), 0.55f) }, kHeld },
        { "Choir and Glass",        V, { oohs (0), lvl (choir (2, 0.6f, 12), 0.45f, -0.15f), lvl (glass ("fast", 1), 0.1f, 0.3f) }, kHeld },
        { "Hum Duet",               V, { female (2, 0, 0, { { "m.bright", 0.25f } }), lvl (male (2, 0, -1, { { "m.bright", 0.25f } }), 0.6f) }, kHeld },
        { "Chop Pad",               V, { chop ("eh"), lvl (vpad (0, 0.8f), 0.35f, 0.2f) }, kChop },
        { "Whispered Chop",         V, { chop ("eh_breathy"), lvl (female (0, 3), 0.3f, -0.25f) }, kChop },
        { "Aria and Strings",       V, { sopAh (0), lvl (S ("strings.violins.ensemble", "sustain", "Violin Section", 0, { { "s.attack", 0.3f } }), 0.35f, 0.25f) }, kHeld },
        { "Tremolo Choir",          V, { aahs (0), lvl (oohs (0), 0.5f), lvl (S ("strings.violins.ensemble", "tremolo", "Violin Section"), 0.3f, 0.3f) }, kHeld },
        { "Epic Choir",             V, { aahs (-1), lvl (oohs (-1), 0.6f), lvl (S ("perc.timpani", "roll", "Timpani", -1), 0.2f, 0.0f) }, kHeld },
        { "Baritone and Cellos",    V, { male (0), lvl (S ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.attack", 0.3f } }), 0.35f, 0.25f) }, kHeld },
        { "Voice and Harp",         V, { female (0), lvl (S ("pluck.harp.concert", "pluck", "Concert Harp"), 0.45f, -0.3f) }, kHeld },
        { "Choir Pad",              V, { vpad (0, 0.7f), lvl (aahs (0), 0.4f), lvl (oohs (0), 0.3f, 0.2f) }, kPad },
        { "Ha Chop Stack",          V, { female (0, 1), lvl (chop ("eh"), 0.55f, 0.2f) }, kChop },
        { "Synth and Voice",        V, { femOo (0), lvl (synthChoir (0), 0.35f, -0.25f) }, kHeld },
        { "Whisper Choir Pad",      V, { vpad (2, 0.6f), lvl (choir (0, 0.6f, 12, 0, { { "m.artic", 3 } }), 0.35f, 0.2f) }, kPad },
        { "Hum and Nylon",          V, { female (2, 0, 0, { { "m.bright", 0.2f } }), lvl (M ("guitar.nylon", "Nylon Guitar"), 0.4f, 0.3f) }, kHeld },
    };
}

//==============================================================================
// roles (treatments)
enum Add { addNone, addPad, addSub, addOctDown, addGlimmer, addAir, addShadowVoice };

struct Treat
{
    juce::StringArray prefixes;   // "" = the plain name
    KV fx;                        // FX keys (part 0)
    int kinds;                    // voice kinds this role fits
    float dark = 0;               // low-pass every part (Hz)
    float shortDecay = 0;         // decay to silence (s): sample / synth / keys model parts only
    float slowAttack = 0;         // attack (s): sample / synth / voice model parts only
    float longRelease = 0;        // release (s)
    KV model;                     // voice model keys ("m.*") on every voice model part; the role needs one
    int add = addNone;
};

const int kAll = kStrike | kHeld | kChop | kPad;

std::vector<Treat> malletTreats()
{
    return {
        { { "" }, { { "revMix", 0.14f }, { "revSize", 0.3f }, { "revType", 1 } }, kAll },
        { { "Concert Hall", "Distant", "Stage" }, { { "revMix", 0.32f }, { "revSize", 0.65f }, { "revType", 0 }, { "width", 0.6f } }, kAll },
        { { "Felt", "Velvet", "Soft Felt" }, { { "eqHigh", -5.0f }, { "eqLow", 2.0f }, { "revMix", 0.16f }, { "revSize", 0.4f }, { "revType", 1 } }, kAll, 1400.0f },
        { { "Night", "Echo", "Midnight" }, { { "revMix", 0.14f }, { "revSize", 0.35f }, { "revType", 1 }, { "delayMix", 0.2f }, { "delayTime", 2 },
                                             { "delayFb", 0.32f }, { "delayMode", 0 } }, kAll },
        { { "Dusty", "Cassette", "Faded" }, { { "wow", 0.5f }, { "eqHigh", -8.0f }, { "revMix", 0.12f }, { "revSize", 0.35f }, { "revType", 1 } }, kAll, 2600.0f },
        { { "Dampened", "Muted", "Stopped" }, { { "revMix", 0.12f }, { "revSize", 0.3f }, { "revType", 1 } }, kStrike, 0, 0.18f },
        { { "Blooming", "Unfolding", "Bowed" }, { { "s.oneShot", 0 }, { "revMix", 0.28f }, { "revSize", 0.55f }, { "revType", 0 } }, kStrike, 0, 0, 0.3f, 1.2f },
        { { "Behind the Wall", "Next Room", "Muffled" }, { { "eqHigh", -12.0f }, { "revMix", 0.22f }, { "revSize", 0.45f }, { "revType", 1 } }, kAll, 650.0f },
        { { "Reverse", "Backward", "Rewind" }, { { "reverse", 0.5f }, { "revMix", 0.18f }, { "revType", 0 } }, kAll },
        { { "Dream", "Cloud", "Haze" }, { { "revMix", 0.4f }, { "revSize", 0.6f }, { "revType", 2 }, { "width", 0.75f }, { "delayMix", 0.1f },
                                          { "delayTime", 2 }, { "delayFb", 0.3f } }, kAll },
        { { "Heavy", "Weighted", "Deep" }, { { "revMix", 0.12f }, { "revSize", 0.3f }, { "revType", 1 } }, kStrike, 0, 0, 0, 0, {}, addSub },
        { { "Twin", "Octave", "Doubled" }, { { "revMix", 0.16f }, { "revSize", 0.4f }, { "revType", 1 }, { "width", 0.65f } }, kAll, 0, 0, 0, 0, {}, addOctDown },
        { { "Glimmer", "Sparkle", "Starlit" }, { { "revMix", 0.2f }, { "revSize", 0.45f }, { "revType", 1 } }, kAll, 0, 0, 0, 0, {}, addGlimmer },
        { { "Glowing", "Halo", "Aurora" }, { { "revMix", 0.22f }, { "revSize", 0.5f }, { "revType", 0 } }, kAll, 0, 0, 0, 0, {}, addPad },
        { { "Shimmer", "Wide", "Silken" }, { { "chorus", 0.55f }, { "width", 0.8f }, { "revMix", 0.15f }, { "revType", 1 } }, kAll },
        { { "Gritty", "Smoky", "Saturated" }, { { "drive", 0.3f }, { "driveType", 1 }, { "eqHigh", -4.0f }, { "revMix", 0.12f }, { "revType", 1 } }, kAll, 3000.0f },
        { { "Warped", "Seasick", "Melted" }, { { "wow", 0.8f }, { "eqHigh", -8.0f }, { "revMix", 0.2f }, { "revSize", 0.45f }, { "revType", 2 } }, kAll, 2000.0f },
        { { "Shadow", "Undertow", "Deep Water" }, { { "ghost", 0.45f }, { "ghostRev", 0 }, { "ghostOct", 1 }, { "ghostBlur", 0.5f }, { "revMix", 0.14f },
                                                    { "revType", 0 } }, kAll },
        { { "Trap", "Drill", "Street" }, { { "delayMix", 0.18f }, { "delayTime", 1 }, { "delayFb", 0.28f }, { "delayMode", 0 }, { "eqLow", -3.0f },
                                           { "drive", 0.12f }, { "driveType", 0 }, { "revMix", 0.2f }, { "revSize", 0.4f }, { "revType", 1 } }, kAll },
        { { "Lo-Fi", "Worn", "Crushed" }, { { "crush", 0.22f }, { "wow", 0.2f }, { "eqHigh", -6.0f }, { "revMix", 0.12f }, { "revType", 1 } }, kAll, 3200.0f },
    };
}

std::vector<Treat> vocalTreats()
{
    return {
        { { "" }, { { "revMix", 0.16f }, { "revSize", 0.35f }, { "revType", 1 } }, kAll },
        { { "Cathedral", "Distant", "Chapel" }, { { "revMix", 0.36f }, { "revSize", 0.75f }, { "revType", 0 }, { "width", 0.65f } }, kAll },
        { { "Intimate", "Close", "Hushed" }, { { "eqHigh", -5.0f }, { "eqLow", 1.5f }, { "revMix", 0.1f }, { "revSize", 0.25f }, { "revType", 1 } }, kAll, 1600.0f,
          0, 0, 0, { { "m.bright", 0.2f } } },
        { { "Night", "Echo", "Midnight" }, { { "revMix", 0.16f }, { "revSize", 0.35f }, { "revType", 1 }, { "delayMix", 0.22f }, { "delayTime", 2 },
                                             { "delayFb", 0.34f }, { "delayMode", 0 } }, kAll },
        { { "Dusty", "Cassette", "Faded" }, { { "wow", 0.5f }, { "eqHigh", -8.0f }, { "revMix", 0.14f }, { "revSize", 0.35f }, { "revType", 1 } }, kAll, 2600.0f },
        { { "Swelling", "Rising", "Dawn" }, { { "revMix", 0.26f }, { "revSize", 0.55f }, { "revType", 0 } }, kHeld | kPad, 0, 0, 0.8f, 1.6f },
        { { "Ambient", "Floating", "Weightless" }, { { "revMix", 0.42f }, { "revSize", 0.7f }, { "revType", 2 }, { "width", 0.8f } }, kHeld | kPad, 0, 0, 0.45f, 2.4f },
        { { "Reverse", "Backward", "Rewind" }, { { "reverse", 0.5f }, { "revMix", 0.2f }, { "revType", 0 } }, kAll },
        { { "Wide", "Ensemble", "Chorus" }, { { "chorus", 0.55f }, { "width", 0.8f }, { "revMix", 0.16f }, { "revType", 1 } }, kAll },
        { { "Shadow", "Undertow", "Deep Water" }, { { "ghost", 0.45f }, { "ghostRev", 0 }, { "ghostOct", 1 }, { "ghostBlur", 0.5f }, { "revMix", 0.16f },
                                                    { "revType", 0 } }, kAll },
        { { "Twin", "Octave", "Doubled" }, { { "revMix", 0.18f }, { "revSize", 0.4f }, { "revType", 1 }, { "width", 0.65f } }, kAll, 0, 0, 0, 0, {}, addOctDown },
        { { "Glowing", "Halo", "Heavenly" }, { { "revMix", 0.24f }, { "revSize", 0.55f }, { "revType", 0 } }, kHeld | kChop, 0, 0, 0, 0, {}, addPad },
        { { "Stab", "Clipped", "Short" }, { { "revMix", 0.14f }, { "revSize", 0.3f }, { "revType", 1 } }, kHeld | kChop, 0, 0.22f },
        { { "Young", "Small", "Little" }, { { "revMix", 0.16f }, { "revSize", 0.35f }, { "revType", 1 } }, kAll, 0, 0, 0, 0, { { "m.formant", 0.75f } } },
        { { "Giant", "Titan", "Colossal" }, { { "revMix", 0.22f }, { "revSize", 0.5f }, { "revType", 0 } }, kAll, 0, 0, 0, 0, { { "m.formant", -0.8f } } },
        { { "Pure", "Straight", "Still" }, { { "revMix", 0.16f }, { "revSize", 0.4f }, { "revType", 1 } }, kHeld | kPad, 0, 0, 0, 0, { { "m.vibrato", 0.0f }, { "m.detune", 3.0f } } },
        { { "Airy", "Breathy", "Windswept" }, { { "revMix", 0.2f }, { "revSize", 0.45f }, { "revType", 1 } }, kAll, 0, 0, 0, 0, {}, addAir },
        { { "Gritty", "Smoky", "Saturated" }, { { "drive", 0.3f }, { "driveType", 1 }, { "eqHigh", -4.0f }, { "revMix", 0.14f }, { "revType", 1 } }, kAll, 3000.0f },
        { { "Warped", "Seasick", "Melted" }, { { "wow", 0.8f }, { "eqHigh", -8.0f }, { "revMix", 0.22f }, { "revSize", 0.45f }, { "revType", 2 } }, kAll, 2000.0f },
        { { "Haunted", "Phantom", "Spectral" }, { { "revMix", 0.3f }, { "revSize", 0.6f }, { "revType", 2 } }, kHeld | kChop | kPad, 0, 0, 0, 0, {}, addShadowVoice },
        { { "Trap", "Drill", "Street" }, { { "delayMix", 0.2f }, { "delayTime", 1 }, { "delayFb", 0.28f }, { "delayMode", 0 }, { "eqLow", -2.0f },
                                           { "revMix", 0.24f }, { "revSize", 0.45f }, { "revType", 1 } }, kChop | kHeld },
        { { "Lo-Fi", "Worn", "Crushed" }, { { "crush", 0.2f }, { "wow", 0.2f }, { "eqHigh", -6.0f }, { "revMix", 0.12f }, { "revType", 1 } }, kAll, 3200.0f },
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
            bool clash = false;   // never "Deep Deep Marimba" / "Twin Twin Marimbas"
            for (auto& w : juce::StringArray::fromTokens (pre, " ", ""))
                if (juce::StringArray::fromTokens (noun.toLowerCase(), " ", "").contains (w.toLowerCase())) clash = true;
            if (clash) continue;
            const auto n = pre.isEmpty() ? noun : pre + " " + noun;
            if (n.length() <= 28 && used.insert (n.toLowerCase()).second) return n;
        }
        return {};
    }
};

struct Cand { juce::String name; int category; std::vector<VaultPart> parts; bool bell; int role; int kind = kStrike; juce::String noun, roleTag; int addIdx = -1; };

// what a kept preset is (for the v3 design): its base voice, articulation kind, role, the part a role added, rhythm pattern
struct Meta { juce::String noun, role = "plain", fam; int kind = kStrike, addIdx = -1, pattern = 0; bool bell = false; };

bool isFxKey (const juce::String& k) { return ! k.startsWith ("s.") && ! k.startsWith ("m."); }
bool isVoiceModel (const VaultPart& p) { return p.kind == VaultPartKind::model && p.source.startsWith ("voice."); }
bool isKeysModel (const VaultPart& p) { return p.kind == VaultPartKind::model && ! p.source.startsWith ("voice."); }

// the key C5 of a sample part (+ extra octaves) lands inside its articulation's zones
bool inRange (const RecipeContext& ctx, const VaultPart& p, int extraOct)
{
    if (p.kind != VaultPartKind::sample) return true;
    if (! ctx.hasInstrument (p.source)) return false;
    const int note = vault::homeNote + 12 * (p.octave + extraOct) + p.semi;
    for (auto& a : ctx.library->find (p.source)->desc.articulations)
        if (a.name == p.articulation || (p.articulation.isEmpty() && &a == &ctx.library->find (p.source)->desc.articulations.front()))
            for (auto& z : a.zones)
                if (note >= z.lo && note <= z.hi) return true;
    return false;
}

// a role on a part list; empty = the role does not fit these parts
std::vector<VaultPart> treat (const RecipeContext& ctx, std::vector<VaultPart> parts, const Treat& t)
{
    if (! t.model.empty())
    {
        bool any = false;
        for (auto& p : parts) if (isVoiceModel (p)) { any = true; for (auto& [k, v] : t.model) if (k != "m.bright" || p.get (k, 0.5f) > v) p.set (k, v); }
        if (! any && t.dark <= 0) return {};
    }
    if (t.shortDecay > 0)
    {
        bool any = false;
        for (auto& p : parts)
        {
            if (p.kind == VaultPartKind::sample)
            {
                p.set ("s.decay", t.shortDecay); p.set ("s.sustain", 0.0f); p.set ("s.oneShot", 0); p.set ("s.release", std::min (0.15f, t.shortDecay)); any = true;
            }
            else if (p.kind == VaultPartKind::synth)
            {
                p.set ("decay", t.shortDecay); p.set ("sustain", 0.0f); p.set ("release", std::min (0.15f, t.shortDecay)); any = true;
            }
            else if (isKeysModel (p)) { p.set ("m.decay", 0.25f); p.set ("m.release", 0.1f); any = true; }
            else return {};   // a sung voice model cannot be cut short this way
        }
        if (! any) return {};
    }
    if (t.slowAttack > 0)
        for (auto& p : parts)
        {
            if (p.kind == VaultPartKind::sample) p.set ("s.attack", std::max (t.slowAttack, p.get ("s.attack", 0.0f)));
            else if (p.kind == VaultPartKind::synth) p.set ("attack", std::max (t.slowAttack, p.get ("attack", 0.002f)));
            else if (isVoiceModel (p)) p.set ("m.attack", std::max (t.slowAttack, p.get ("m.attack", 0.06f)));
            else return {};
        }
    if (t.longRelease > 0)
        for (auto& p : parts)
        {
            if (p.kind == VaultPartKind::sample) p.set ("s.release", std::max (t.longRelease, p.get ("s.release", 0.0f)));
            else if (p.kind == VaultPartKind::synth) p.set ("release", std::max (t.longRelease, p.get ("release", 0.3f)));
            else p.set ("m.release", std::max (t.longRelease, p.get ("m.release", 0.3f)));
        }
    if (t.dark > 0)
        for (auto& p : parts)
        {
            if (p.kind == VaultPartKind::sample) p.set ("s.cutoff", std::min (t.dark, p.get ("s.cutoff", 20000.0f)));
            else if (p.kind == VaultPartKind::synth) p.set ("cutoff", std::min (t.dark, p.get ("cutoff", 12000.0f)));
            else if (isKeysModel (p)) p.set ("m.tone", std::min (p.get ("m.tone", 0.5f), t.dark < 1000 ? 0.15f : t.dark < 2000 ? 0.28f : 0.38f));
            else p.set ("m.bright", std::min (p.get ("m.bright", 0.5f), t.dark < 1000 ? 0.1f : t.dark < 2000 ? 0.2f : 0.35f));
        }
    if (t.add != addNone)
    {
        if ((int) parts.size() >= vault::maxParts) return {};
        const auto p0 = parts[0];
        const int sounding = vault::homeNote + 12 * p0.octave + p0.semi;
        switch (t.add)
        {
            case addPad:  parts.push_back (softPad (0.25f, p0.octave)); break;
            case addSub:  if (sounding < 48) return {}; parts.push_back (subSine (0.4f, p0.octave - 1)); break;
            case addAir:  parts.push_back (lvl (female (0, 3, p0.octave), 0.22f, 0.2f)); break;
            case addShadowVoice: parts.push_back (lvl (with (vpad (2, 0.5f, p0.octave - 1), { { "m.attack", 1.5f } }), 0.28f, -0.2f)); break;
            case addOctDown:
            {
                if (p0.kind == VaultPartKind::synth || ! inRange (ctx, p0, -1) || sounding < 48) return {};
                auto d = lvl (p0, 0.5f, 0.0f);
                d.octave -= 1;
                parts.push_back (d);
                break;
            }
            case addGlimmer:
            {
                if (p0.kind == VaultPartKind::synth || ! inRange (ctx, p0, 1) || sounding > 84) return {};
                auto d = lvl (p0, 0.08f, 0.25f);
                d.octave += 1;
                if (d.kind == VaultPartKind::sample) d.set ("s.cutoff", std::min (d.get ("s.cutoff", 20000.0f), 4500.0f));
                parts.push_back (d);
                break;
            }
            default: break;
        }
    }
    for (auto& [k, v] : t.fx)
    {
        if (isFxKey (k)) { parts[0].set (k, v); continue; }
        for (auto& p : parts) if (p.kind == VaultPartKind::sample) p.set (k, v);
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
    const auto mv = malletVoices(), ml = malletLayers(), vv = vocalVoices(), vl = vocalLayers();
    const auto mt = malletTreats(), vt = vocalTreats();
    // the roles a layer is played with (plain, hall, dark, echo, tape, dream / ambient, chorus / wide, trap)
    const std::set<int> malletLayerTreats { 0, 1, 2, 3, 4, 9, 14, 18 }, vocalLayerTreats { 0, 1, 2, 3, 4, 6, 8, 20 };

    auto add = [&] (const Voice& v, const Treat& t, int role, int rot, bool layer)
    {
        if ((v.kind & t.kinds) == 0) return;
        if (layer && t.add != addNone) return;
        if (! usable (v.parts)) return;
        auto parts = treat (ctx, v.parts, t);
        if (parts.empty()) return;
        const auto name = names.make (t.prefixes, v.noun, rot);
        if (name.isNotEmpty())
            out.push_back ({ name, v.category, parts, v.bell, role, v.kind, v.noun, t.prefixes[0].isEmpty() ? juce::String ("plain") : t.prefixes[0],
                             t.add != addNone ? (int) v.parts.size() : -1 });
    };
    const int nt = (int) std::max (mt.size(), vt.size());
    for (int t = 0; t < nt; ++t)
    {
        const size_t nv = std::max (mv.size(), vv.size());
        for (size_t i = 0; i < nv; ++i)
        {
            if (t < (int) mt.size() && i < mv.size()) add (mv[i], mt[(size_t) t], t, (int) i, false);
            if (t < (int) vt.size() && i < vv.size()) add (vv[i], vt[(size_t) t], t, (int) i, false);
        }
        const size_t nl = std::max (ml.size(), vl.size());
        for (size_t i = 0; i < nl; ++i)
        {
            if (malletLayerTreats.count (t) && i < ml.size()) add (ml[i], mt[(size_t) t], t, (int) i, true);
            if (vocalLayerTreats.count (t) && i < vl.size()) add (vl[i], vt[(size_t) t], t, (int) i, true);
        }
    }
}

//==============================================================================
// measurement + repair
constexpr float cap25 = 0.07f, cap6k = 0.015f, wantH1 = -2.5f;

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
        else if (isKeysModel (part))
        {
            const float cur = part.get ("m.tone", 0.5f);
            if (cur > 0.12f) { part.set ("m.tone", std::max (0.1f, cur * 0.8f)); moved = true; }
        }
        else
        {
            const float cur = part.get ("m.bright", 0.5f);
            if (cur > 0.08f) { part.set ("m.bright", std::max (0.05f, cur * 0.75f)); moved = true; }
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
        for (auto& [a, b] : v) if (a != "gain" && ! isFxKey (a) == true) k << "|" << a << "=" << b;
        for (auto& [a, b] : v) if (a != "gain" && p.kind == VaultPartKind::synth && isFxKey (a)) k << "|" << a << "=" << b;
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
        solo.id = "cv.calib"; solo.name = "Calib"; solo.category = vault::catBellsMallets;
        auto p = part;
        p.level = 1.0f; p.pan = 0.0f;
        KV keep;
        for (auto& kv : p.values)   // sample / model parts: their own keys only (no FX); synth parts: everything but gain
            if (kv.first != "gain" && (p.kind == VaultPartKind::synth || ! isFxKey (kv.first))) keep.push_back (kv);
        p.values = keep;
        if (p.kind == VaultPartKind::synth) for (auto& [a, b] : synthBase) p.set (a, b);
        solo.parts = { p };
        const auto b = recipegate::render (solo, vault::homeNote, 0.8f, 1.0, 1.2);
        const auto a = vaultqc::analyse (b, recipegate::sr, 1.0);
        const float l = a.finite ? a.loud : -140.0f;
        std::lock_guard<std::mutex> g (lock);
        loud[k] = l;
        return l;
    }
};

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

// the unison part that carries the fundamental: choir oohs (voices, inside its range), a soft "oo" voice, or a tone bar
VaultPart supportPart (const RecipeContext& ctx, const VaultPreset& p)
{
    const auto& p0 = p.parts[0];
    if (p.category == vault::catVocalsChoir)
    {
        auto o = oohs (p0.octave);
        if (inRange (ctx, o, 0)) return o;
        return female (2, 0, p0.octave, { { "m.bright", 0.2f } });
    }
    return toneBar (p0.octave, 2.0f);
}

bool repair (const RecipeContext& ctx, VaultPreset& p, Calib& c, bool bell)
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
        if (! bell && m8.h1 < wantH1)
        {
            ok = false;
            const float el = p0.get ("eqLow", 0.0f);
            if (f0 < 420.0f && el < 9.0f) p0.set ("eqLow", std::min (9.0f, el + 3.0f));
            else if (support < 0)
            {
                bool cut = false;   // a gentle low-pass first (keeps the instrument's own colour), then the unison support
                for (auto& part : p.parts)
                {
                    if (part.kind == VaultPartKind::model) continue;
                    const char* key = part.kind == VaultPartKind::sample ? "s.cutoff" : "cutoff";
                    const float cur = part.get (key, part.kind == VaultPartKind::sample ? 20000.0f : 12000.0f);
                    const float floor = std::max (900.0f, 3.5f * partHz (part));
                    const float next = std::max (floor, cur > 6000.0f ? 6000.0f : cur * 0.75f);
                    if (next < cur - 1.0f) { part.set (key, next); cut = true; }
                }
                if (! cut)
                {
                    if ((int) p.parts.size() >= vault::maxParts) return false;
                    auto sp = lvl (supportPart (ctx, p), 0.4f);
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

// a mechanical fix for a shared-gate failure; false = no fix for it
bool fixGate (VaultPreset& p, const juce::String& fail)
{
    auto& p0 = p.parts[0];
    if (fail == "tail")
    {
        p0.set ("revMix", p0.get ("revMix", 0.0f) * 0.6f);
        p0.set ("revSize", p0.get ("revSize", 0.6f) * 0.7f);
        p0.set ("delayFb", std::min (0.2f, p0.get ("delayFb", 0.35f)));
        for (auto& part : p.parts)
        {
            if (part.kind == VaultPartKind::sample)
            {
                if (part.get ("s.oneShot", -1.0f) != 0.0f)   // a ringing bar / bell keeps ringing after the key is up: a shorter natural decay
                {
                    const float d = part.get ("s.decay", 0.0f);
                    part.set ("s.decay", d > 0.0f ? d * 0.7f : 5.5f);
                    part.set ("s.sustain", 0.0f);
                }
                else part.set ("s.release", std::min (part.get ("s.release", 1.0f) > 0 ? part.get ("s.release", 1.0f) : 1.0f, 0.8f));
            }
            else if (part.kind == VaultPartKind::synth) part.set ("release", std::min (part.get ("release", 0.3f), 0.8f));
            else part.set ("m.release", std::min (part.get ("m.release", 1.0f), 1.0f));
        }
        return true;
    }
    if (fail == "velocity")
    {
        const bool already = p0.get ("drive", 0.0f) < 0.01f
                             && std::all_of (p.parts.begin(), p.parts.end(), [] (const VaultPart& q)
                                             { return q.get (q.kind == VaultPartKind::sample ? "s.velSens" : q.kind == VaultPartKind::synth ? "velSens" : "m.velSens", 0.0f) >= 1.0f; });
        if (already) return false;
        p0.set ("drive", p0.get ("drive", 0.0f) * 0.5f);   // saturation flattens the dynamics
        for (auto& part : p.parts)
        {
            if (part.kind == VaultPartKind::sample) part.set ("s.velSens", 1.0f);
            else if (part.kind == VaultPartKind::synth) part.set ("velSens", 1.0f);
            else part.set ("m.velSens", 1.0f);
        }
        return true;
    }
    if (fail == "octave-layer")
    {
        const int t0 = 12 * p0.octave + p0.semi;
        for (size_t k = 1; k < p.parts.size(); ++k)
            if (12 * p.parts[k].octave + p.parts[k].semi >= t0 + 12) p.parts[k].level *= 0.45f;
        return true;
    }
    if (fail == "mono-loss" || fail == "low-side")
    {
        p0.set ("width", 0.25f);
        for (auto& part : p.parts) { part.pan *= 0.4f; if (part.kind == VaultPartKind::sample) part.set ("s.width", 0.35f); }
        return true;
    }
    if (fail == "click-end")
    {
        for (auto& part : p.parts)
            if (part.kind == VaultPartKind::sample) part.set ("s.release", std::max (0.12f, part.get ("s.release", 0.0f)));
            else if (part.kind == VaultPartKind::synth) part.set ("release", std::max (0.12f, part.get ("release", 0.3f)));
        return true;
    }
    if (fail == "click-start")
    {
        for (auto& part : p.parts)
            if (part.kind == VaultPartKind::sample) part.set ("s.attack", std::max (0.004f, part.get ("s.attack", 0.0f)));
            else if (part.kind == VaultPartKind::synth) part.set ("attack", std::max (0.004f, part.get ("attack", 0.002f)));
        return true;
    }
    return false;
}
// the v2 selection (unchanged since the v2 bank): candidates -> repair -> shared gate analysis -> own distinctness
// pinned (the shipped bank's names): only those candidates, every one that passes the gate is kept (they were distinct
// when the bank was assembled), so the shipped sounds come back exactly without the order-sensitive greedy search
std::vector<VaultPreset> v2Selection (const RecipeContext& ctx, std::vector<Meta>& metas, const std::set<juce::String>* pinned = nullptr)
{
    std::vector<Cand> cands;
    addAll (ctx, cands);
    if (pinned != nullptr)
    {
        std::vector<Cand> keepC;
        for (auto& c : cands) if (pinned->count (c.name.replace (" & ", " and ").toLowerCase()) > 0) keepC.push_back (c);
        std::printf ("perc-voc: %d of %d candidates are pinned (%d pinned names)\n", (int) keepC.size(), (int) cands.size(), (int) pinned->size());
        cands.swap (keepC);
    }
    if (const int lim = juce::SystemStats::getEnvironmentVariable ("PV_LIMIT", "0").getIntValue(); lim > 0 && lim < (int) cands.size())
        cands.resize ((size_t) lim);
    std::vector<VaultPreset> all;
    std::vector<char> bells;
    std::vector<int> roles;
    std::vector<Meta> candMeta;
    int nB = 0, nV = 0;
    for (auto& c : cands)
    {
        VaultPreset p;
        p.category = c.category;
        p.id = c.category == vault::catBellsMallets ? "cv.bell." + juce::String (++nB).paddedLeft ('0', 4) : "cv.vox." + juce::String (++nV).paddedLeft ('0', 4);
        p.name = c.name;
        p.parts = c.parts;
        all.push_back (std::move (p));
        bells.push_back (c.bell ? 1 : 0);
        roles.push_back (c.role);
        Meta m; m.noun = c.noun; m.role = c.roleTag; m.kind = c.kind; m.addIdx = c.addIdx; m.bell = c.bell;
        candMeta.push_back (m);
    }
    Calib calib;
    std::vector<char> keep (all.size(), 0);
    std::atomic<int> next { 0 }, done { 0 };
    std::vector<std::thread> pool;
    const int threads = juce::jmax (1, juce::SystemStats::getEnvironmentVariable ("PV_THREADS", "3").getIntValue());
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&]
        {
            for (int i; (i = next++) < (int) all.size();)
            {
                keep[(size_t) i] = repair (ctx, all[(size_t) i], calib, bells[(size_t) i] != 0) ? 1 : 0;
                if (++done % 100 == 0) std::printf ("  repaired %d / %d\n", done.load(), (int) all.size());
            }
        });
    for (auto& t : pool) t.join();
    std::vector<VaultPreset> out;
    std::vector<int> outRole;
    std::vector<Meta> outMeta;
    int dropped = 0;
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (keep[i]) { out.push_back (std::move (all[i])); outRole.push_back (roles[i]); outMeta.push_back (candMeta[i]); }
        else { ++dropped; std::printf ("  unrepairable: %s %s\n", all[i].id.toRawUTF8(), all[i].name.toRawUTF8()); }
    }
    std::printf ("perc-voc: %d candidates, %d unrepairable\n", (int) all.size(), dropped);

    // shared gate analysis of every repaired candidate (+ one mechanical fix round), then greedy distinctness
    const float ownMin = (float) juce::SystemStats::getEnvironmentVariable ("PV_OWN_MIN", "4.6").getDoubleValue();
    std::vector<recipegate::Result> rs (out.size());
    for (size_t i = 0; i < out.size(); ++i) rs[i].preset = out[i];
    next = 0; done = 0;
    pool.clear();
    int fixedCount = 0;
    constexpr float minVelDb = 4.5f;
    std::mutex fixLock;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&]
        {
            for (int i; (i = next++) < (int) rs.size();)
            {
                auto& r = rs[(size_t) i];
                recipegate::analyse (r);
                if (r.fail.isEmpty() && r.velRange < minVelDb) r.fail = "velocity";   // own floor: a clearly velocity-sensitive preset
                if (r.fail.isNotEmpty())
                {
                    auto p = out[(size_t) i];
                    juce::String fail = r.fail;
                    for (int round = 0; round < 4 && fail.isNotEmpty(); ++round)
                    {
                        if (! fixGate (p, fail)) break;
                        recipegate::Result r2;
                        r2.preset = p;
                        recipegate::analyse (r2);
                        fail = r2.fail;
                        if (fail.isEmpty() && r2.velRange < minVelDb) fail = "velocity";
                        if (fail.isEmpty())
                        {
                            out[(size_t) i] = p;
                            r = std::move (r2);
                            std::lock_guard<std::mutex> g (fixLock);
                            ++fixedCount;
                        }
                    }
                }
                if (++done % 100 == 0) std::printf ("  pre-gate %d / %d\n", done.load(), (int) rs.size());
            }
        });
    for (auto& t : pool) t.join();
    std::printf ("perc-voc: %d presets fixed after a gate failure\n", fixedCount);
    if (ownMin <= 0.0f) { metas = outMeta; return out; }
    if (pinned != nullptr)
    {
        std::vector<VaultPreset> kept;
        metas.clear();
        for (size_t i = 0; i < rs.size(); ++i)
            if (rs[i].fail.isEmpty()) { kept.push_back (out[i]); metas.push_back (outMeta[i]); }
            else std::printf ("  pinned preset fails the gate: %s %s -> %s\n", out[i].id.toRawUTF8(), out[i].name.toRawUTF8(), rs[i].fail.toRawUTF8());
        return kept;
    }
    constexpr float maxGainDb = 18.0f;
    const int roleCap = juce::SystemStats::getEnvironmentVariable ("PV_ROLE_CAP", "11").getIntValue();
    std::map<std::pair<int, int>, int> roleCount;
    const bool verbose = juce::SystemStats::getEnvironmentVariable ("PV_VERBOSE", "0").getIntValue() != 0;
    std::vector<int> acc;
    std::map<juce::String, int> fails;
    for (int i = 0; i < (int) rs.size(); ++i)
    {
        if (rs[(size_t) i].fail.isEmpty() && rs[(size_t) i].gainDb > maxGainDb) rs[(size_t) i].fail = "too-quiet";   // a faint fragment, lifted too far
        if (rs[(size_t) i].fail.isNotEmpty())
        {
            ++fails[rs[(size_t) i].fail.upToFirstOccurrenceOf (":", false, false)];
            std::printf ("  gate: %s %s -> %s\n", out[(size_t) i].id.toRawUTF8(), out[(size_t) i].name.toRawUTF8(), rs[(size_t) i].fail.toRawUTF8());
            continue;
        }
        // one role (gritty, halo pad, sub, ...) may colour at most roleCap presets per category: the bank stays a set of
        // different instruments and articulations, not one effect applied to everything
        const auto rk = std::make_pair (out[(size_t) i].category, outRole[(size_t) i]);
        if (outRole[(size_t) i] > 0 && roleCount[rk] >= roleCap) { ++fails["role cap"]; continue; }
        bool near = false;
        for (int j : acc)
            if (const float d = recipegate::distance (rs[(size_t) i].print, rs[(size_t) j].print); d < ownMin)
            {
                near = true;
                if (verbose) std::printf ("  near: %s ~ %s (%.2f)\n", out[(size_t) i].name.toRawUTF8(), out[(size_t) j].name.toRawUTF8(), d);
                break;
            }
        if (near) { ++fails["near (own " + juce::String (ownMin, 1) + ")"]; continue; }
        acc.push_back (i);
        ++roleCount[rk];
    }
    float minPair = 1.0e9f;
    for (size_t x = 0; x < acc.size(); ++x)
        for (size_t y = x + 1; y < acc.size(); ++y)
            minPair = std::min (minPair, recipegate::distance (rs[(size_t) acc[x]].print, rs[(size_t) acc[y]].print));
    std::printf ("perc-voc own pass: %d of %d kept, min pairwise distance %.2f\n", (int) acc.size(), (int) rs.size(), minPair);
    for (auto& [f, n] : fails) std::printf ("  own pass dropped %-20s %d\n", f.toRawUTF8(), n);
    std::vector<VaultPreset> kept;
    metas.clear();
    for (int i : acc) { kept.push_back (out[(size_t) i]); metas.push_back (outMeta[(size_t) i]); }
    return kept;
}

//==============================================================================
// v3 SOUND DESIGN (see the v3 design note at the top): FX chain by family + role, movement, 4 named sound macros,
// performance routings, arp / gate on the few rhythm presets
namespace v3
{
using namespace recipe;
using FT = vault::FxType;
using R = fx::Reverb;

int envInt (const char* k, int def) { auto v = juce::SystemStats::getEnvironmentVariable (k, {}); return v.isEmpty() ? def : v.getIntValue(); }

// the processing family of a preset (its lead part decides)
juce::String family (const VaultPreset& p, const Meta& m)
{
    const auto& p0 = p.parts.front();
    if (p.category == vault::catBellsMallets)
    {
        if (m.kind == kHeld) return "bowed";
        if (m.bell || p0.source.startsWith ("bells.") || p0.label.contains ("Bell")) return "bells";
        if (p0.source == "mallets.vibraphone") return "vibes";
        if (p0.source == "mallets.glockenspiel" || p0.source == "keys.toy" || p0.label == "Music Box") return "glock";
        return "wood";   // marimba, xylophone
    }
    if (m.kind == kPad) return "pad";
    if (m.kind == kChop) return "chop";
    if (p0.kind == VaultPartKind::synth) return "synthchoir";
    if (p0.source.startsWith ("voice.choir") || m.noun.containsIgnoreCase ("Choir") || m.noun == "Requiem" || m.noun == "Basso Profundo") return "choir";
    if (p0.kind == VaultPartKind::model && p0.get ("m.artic", 0.0f) >= 1.5f) return "breath";
    return "solo";
}

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
        for (auto t : { FT::transient, FT::width, FT::compressor })   // at most 8 slots: the least important go first
            if (v.size() > (size_t) vault::maxFxSlots)
                for (auto it = v.begin(); it != v.end(); ++it) if (it->type == t) { v.erase (it); break; }
        if (v.size() > (size_t) vault::maxFxSlots) v.resize ((size_t) vault::maxFxSlots);
        return v;
    }
};

VaultFxSlot rv (R type, float size, float decay, float mix, float predelay, float damping, float lowCut)
{
    return fx::reverb (type, size, decay, mix).set ("predelay", predelay).set ("damping", damping).set ("lowCut", lowCut);
}
VaultFxSlot echo (const char* div, float fb, float mix, fx::Delay mode = fx::Delay::pingPong, float hiCut = 3800.0f, float loCut = 300.0f)
{
    return fx::delay (div, fb, mix, mode).set ("hiCut", hiCut).set ("loCut", loCut);
}

// a sound macro whose knob default reproduces the designed sound exactly: every target has its designed value at the knob's
// default d and moves `span` in total (span x d below it at 0, span x (1 - d) above it at 1)
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
    // designed value v at the default, `atMax` at 1 (a drive's output trim that holds the level as the drive goes up)
    M& toMax (const char* target, float v, float atMax)
    {
        const float span = d < 1.0f - 1.0e-4f ? (atMax - v) / (1.0f - d) : 0.0f;
        m.targets.push_back ({ target, v - span * d, atMax, 0.0f });
        return *this;
    }
    template <typename C> M& fxp (const C& c, FT t, const char* id, float span)
    {
        const auto& ti = vault::fxTypeInfo (t);
        const auto& pi = ti.params[(size_t) ti.indexOf (id)];
        const auto target = juce::String ("fx.") + ti.id + "." + id;
        return abs (target.toRawUTF8(), c.get (t, id), span, pi.lo, pi.hi);
    }
};
void setMacro (VaultPreset& p, int i, const M& m) { p.macros[(size_t) i] = m.m; p.macroDefaults[i] = m.d; }

// movement
VaultLfo vibrato (float rate, float cents) { return lfo (VaultLfo::sine).rate (rate).delay (0.4f).fade (0.7f).depth (0.0f).to ("pitch", cents / 100.0f); }
VaultLfo breathing (float depth, float rate, float cutoffOct, float vol) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", cutoffOct).to ("volume", vol); }
VaultLfo drift (float depth, float rate, float cutoffOct, float pan) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", cutoffOct).to ("pan", pan).to ("volume", 0.2f); }
VaultLfo tapeWobble (float depth, float rate = 0.55f) { return lfo (VaultLfo::triangle).rate (rate).free().depth (depth).to ("pitch", 0.09f); }
VaultLfo motor (float rate) { return lfo (VaultLfo::sine).rate (rate).depth (0.0f).to ("volume", 0.55f).to ("pan", 0.12f); }

struct Plan
{
    juce::String tone = "Tone", space = "Hall", third, fourth;
    bool shortS = false;
};

//------------------------------------------------------------------------------ 1. the chain + movement (no macros yet)
Plan designChain (VaultPreset& p, const Meta& m)
{
    design (p);   // v2 -> v3: the role's colour slots (delay, lo-fi, drive, chorus, reverse, ghost, width) and the fitted EQ arrive here
    Chain c (p.fx);
    const auto& f = m.fam;
    const auto& t = m.role;
    const bool vox = p.category == vault::catVocalsChoir;
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    Plan pl;
    pl.shortS = m.kind == kStrike || m.kind == kChop;
    p.mod = {};
    // the fitted stereo image is kept (a narrowed sound stays narrow)
    const float fittedWidth = c.has (FT::width) ? c.get (FT::width, "width") : 1.0f;
    auto width = [&] (float w, float mono) { c.put (fx::width (std::min (w, std::max (fittedWidth, 0.3f)), mono)); };
    auto mid = [&] (float db, float hz, float q) { auto& e = c.at (FT::eq); e.set ("mid", db).set ("midFreq", hz).set ("midQ", q); };
    const float mudHz = juce::jlimit (250.0f, 800.0f, 1.9f * f0);
    // roles whose space IS the sound keep their reverb (type / size / mix) and only get the family's pre-delay / low cut
    const bool roleSpace = t == "Concert Hall" || t == "Cathedral" || t == "Dream" || t == "Ambient" || t == "Haunted" || t == "Warped"
                        || t == "Shadow" || t == "Reverse";
    auto space = [&] (const VaultFxSlot& famRev)
    {
        if (roleSpace && c.has (FT::reverb))
        {
            auto& r = c.at (FT::reverb);
            r.set ("predelay", t == "Concert Hall" || t == "Cathedral" ? 12.0f : famRev.get ("predelay")).set ("lowCut", famRev.get ("lowCut"))
             .set ("damping", std::max (0.3f, famRev.get ("damping")));
        }
        else c.put (famRev);
    };

    // 1. the family's finished sound
    if (f == "wood")
    {   // marimba / xylophone: mallet attack, mud cut, gentle compression for the ring, a stage hall, an echo ready
        c.put (fx::transient (0.05f));
        c.put (fx::comp (-24, 2.0f, 15, 200, 1.0f, 0.5f));
        if (f0 < 400.0f) mid (-1.5f, mudHz, 0.9f);
        space (rv (R::hall, 0.55f, 0.9f, 0.18f, 15, 0.3f, 200));
        if (! c.has (FT::delay)) c.put (echo ("1/8D", 0.3f, 0.0f));
        width (0.6f, 120);
        pl.tone = "Mallet"; pl.third = "Body"; pl.fourth = "Echo";
    }
    else if (f == "vibes")
    {   // vibraphone: soft glue, a whisper of chorus, a plate, the motor (amplitude tremolo) ready on a macro
        c.put (fx::transient (0.04f));
        c.put (fx::comp (-22, 2.0f, 20, 250, 1.0f, 0.5f));
        if (! c.has (FT::chorus)) c.put (fx::chorus (0.12f, 0.3f, 0.35f));
        c.eqAdd ("high", -1.0f);
        space (rv (R::plate, 0.5f, 1.0f, 0.2f, 20, 0.2f, 220));
        if (! c.has (FT::delay)) c.put (echo ("1/8D", 0.3f, 0.0f));
        width (0.65f, 120);
        p.mod.lfo[0] = motor (m.noun.contains ("Slow") ? 3.2f : m.noun.contains ("Fast") ? 7.0f : 5.2f);
        pl.tone = "Mallet"; pl.space = "Plate"; pl.third = "Motor"; pl.fourth = "Echo";
    }
    else if (f == "glock")
    {   // glockenspiel / celesta / music box: the top tamed, a short dotted-eighth echo, a dark plate
        c.eqAdd ("high", -2.0f);
        if (! c.has (FT::delay)) c.put (echo ("1/8D", 0.25f, 0.1f, fx::Delay::pingPong, 3500, 300));
        space (rv (R::plate, 0.45f, 0.9f, 0.2f, 10, 0.4f, 300));
        width (0.6f, 150);
        p.mod.lfo[1] = tapeWobble (0.0f, 0.6f);
        pl.space = "Plate"; pl.third = "Wobble"; pl.fourth = "Echo";
    }
    else if (f == "bells")
    {   // tubular bells / chimes / handbells / bell towers: clang tamed, a big hall with its lows cut
        mid (-1.5f, 700, 0.9f);
        c.eqAdd ("high", -1.0f);
        space (rv (R::hall, 0.75f, 1.2f, 0.24f, 25, 0.3f, 250));
        if (! c.has (FT::delay)) c.put (echo ("1/4D", 0.3f, 0.0f));
        width (0.65f, 150);
        pl.tone = "Strike"; pl.third = "Bloom"; pl.fourth = "Echo";
    }
    else if (f == "bowed")
    {   // bowed vibes / glass harp / held layers: a slow ensemble, a long hall, a slow brightness + pan drift
        if (! c.has (FT::chorus)) c.put (fx::ensemble (0.18f, 0.35f, 0.4f));
        c.eqAdd ("high", -1.0f);
        space (rv (R::hall, 0.8f, 1.4f, 0.28f, 35, 0.25f, 220));
        width (0.7f, 150);
        p.mod.lfo[1] = drift (0.25f, 0.07f, 0.6f, 0.4f);
        pl.third = "Swell"; pl.fourth = "Drift";
    }
    else if (f == "choir")
    {   // choir: light bus glue, the boxy band cut, a hall with pre-delay (the consonants first), slow breathing, delayed vibrato
        c.put (fx::comp (-26, 1.6f, 40, 300, 1.0f, 0.5f));
        mid (-1.5f, mudHz, 0.8f);
        space (rv (R::hall, 0.75f, 1.3f, 0.26f, 30, 0.25f, 250));
        width (0.7f, 120);
        p.mod.lfo[0] = vibrato (5.2f, 18.0f);
        p.mod.lfo[1] = breathing (0.5f, 0.09f, 0.3f, 0.06f);
        pl.third = "Swell"; pl.fourth = "Vibrato";
    }
    else if (f == "solo")
    {   // a sung voice: pop vocal compression, the box cut, a plate with 30 ms pre-delay, a quarter-note throw
        c.put (fx::comp (-20, 3.0f, 8, 120, 2.0f, 0.7f));
        mid (-1.0f, mudHz, 0.8f);
        c.eqAdd ("high", -1.0f);
        if (! c.has (FT::delay)) c.put (echo ("1/4", 0.3f, 0.08f, fx::Delay::pingPong, 4000, 300));
        space (rv (R::plate, 0.5f, 1.0f, 0.2f, 30, 0.25f, 250));
        width (0.55f, 120);
        p.mod.lfo[0] = vibrato (5.4f, 25.0f);
        pl.space = "Plate"; pl.third = "Throw"; pl.fourth = "Vibrato";
    }
    else if (f == "breath")
    {   // breathy / whispered voices: soft compression, the air tamed, a plate, a throw
        c.put (fx::comp (-22, 2.0f, 15, 150, 1.0f, 0.5f));
        c.eqAdd ("high", -2.0f);
        if (! c.has (FT::delay)) c.put (echo ("1/4", 0.3f, 0.06f, fx::Delay::pingPong, 3500, 300));
        space (rv (R::plate, 0.5f, 1.0f, 0.2f, 25, 0.35f, 250));
        width (0.6f, 120);
        pl.space = "Plate"; pl.third = "Throw"; pl.fourth = "Width";
    }
    else if (f == "chop")
    {   // vocal chops / "ha" hits: a punchy attack, fast compression, a short plate, a dotted-eighth ping-pong throw, grit ready
        c.put (fx::transient (0.12f));
        c.put (fx::comp (-20, 3.0f, 5, 100, 1.0f, 0.6f));
        if (! c.has (FT::drive)) c.put (fx::drive (fx::Drive::tape, 0.0f, 0.6f, 5000).set ("cleanLow", 1.0f));
        c.eqAdd ("high", -1.0f);
        if (! c.has (FT::delay)) c.put (echo ("1/8D", 0.3f, 0.12f, fx::Delay::pingPong, 3500, 350));
        space (rv (R::plate, 0.45f, 0.85f, 0.18f, 10, 0.25f, 300));
        width (0.6f, 150);
        pl.tone = "Snap"; pl.space = "Plate"; pl.third = "Throw"; pl.fourth = "Grit";
    }
    else if (f == "pad")
    {   // vocal pads: ensemble chorus, a long hall, a slow drift of brightness and pan
        if (! c.has (FT::chorus)) c.put (fx::ensemble (0.3f, 0.4f, 0.5f));
        c.eqAdd ("low", 0.5f); c.eqAdd ("high", -1.0f);
        space (rv (R::hall, 0.85f, 1.5f, 0.3f, 40, 0.3f, 220));
        width (0.75f, 150);
        p.mod.lfo[1] = drift (0.25f, 0.07f, 0.8f, 0.5f);
        pl.space = "Space"; pl.third = "Motion"; pl.fourth = "Ensemble";
    }
    else   // synth choir
    {
        if (! c.has (FT::chorus)) c.put (fx::ensemble (0.3f, 0.5f, 0.45f));
        space (rv (R::hall, 0.75f, 1.3f, 0.26f, 30, 0.3f, 220));
        width (0.7f, 150);
        p.mod.lfo[0] = vibrato (5.2f, 20.0f);
        p.mod.lfo[1] = breathing (0.5f, 0.09f, 0.3f, 0.05f);
        pl.third = "Ensemble"; pl.fourth = "Vibrato";
    }

    // 2. the role as a processing style
    juce::String roleMacro;
    if (t == "Concert Hall" || t == "Cathedral")
    {
        c.eqAdd ("high", -1.5f);
        width (0.6f, 150);
    }
    else if (t == "Felt" || t == "Intimate" || t == "Behind the Wall")
    {
        c.put (rv (R::room, 0.4f, 0.9f, t == "Behind the Wall" ? 0.22f : 0.16f, 5, 0.4f, 200));
        pl.space = "Room"; roleMacro = "Warmth";
    }
    else if (t == "Night" || t == "Trap")
    {
        if (c.has (FT::delay)) c.at (FT::delay).set ("hiCut", 3800.0f).set ("loCut", 300.0f);
        roleMacro = vox ? "Throw" : "Echo";
        if (t == "Trap" && c.has (FT::drive)) c.at (FT::drive).set ("cleanLow", 1.0f).set ("tone", std::min (5000.0f, c.get (FT::drive, "tone")));
    }
    else if (t == "Dusty" || t == "Warped" || t == "Lo-Fi")
    {
        p.mod.lfo[1] = tapeWobble (t == "Warped" ? 0.7f : 0.45f);
        if (t != "Warped") { c.put (rv (R::room, 0.4f, 0.9f, 0.16f, 5, 0.45f, 200)); pl.space = "Room"; }
        roleMacro = t == "Lo-Fi" ? "Crush" : "Tape";
    }
    else if (t == "Dampened" || t == "Stab")
    {   // a dead stroke in a small room (a chord of strikes that peaks too far gets a fast compressor in the gate fixes)
        c.put (rv (R::room, 0.35f, 0.8f, 0.15f, 5, 0.35f, 220));
        pl.space = "Room";
    }
    else if (t == "Blooming" || t == "Swelling") roleMacro = "Swell";
    else if (t == "Reverse") roleMacro = "Reverse";
    else if (t == "Dream" || t == "Ambient" || t == "Haunted")
    {
        p.mod.lfo[1] = drift (0.3f, 0.06f, 0.7f, 0.6f);
        pl.space = "Space"; roleMacro = "Drift";
    }
    else if (t == "Heavy") roleMacro = "Sub";
    else if (t == "Twin") roleMacro = "Octave";
    else if (t == "Glimmer") roleMacro = "Sparkle";
    else if (t == "Glowing") roleMacro = "Halo";
    else if (t == "Airy") roleMacro = "Breath";
    else if (t == "Shimmer" || t == "Wide") roleMacro = vox ? "Ensemble" : "Chorus";
    else if (t == "Gritty")
    {
        if (c.has (FT::drive)) c.at (FT::drive).set ("cleanLow", 1.0f).set ("tone", std::min (5000.0f, c.get (FT::drive, "tone")));
        roleMacro = "Grit";
    }
    else if (t == "Shadow") roleMacro = "Shadow";
    // a role that brings its own LFO 2 (tape wobble, drift) takes the family's LFO 2 macro away; a short role has no swell
    const bool roleLfo2 = t == "Dusty" || t == "Warped" || t == "Lo-Fi" || t == "Dream" || t == "Ambient" || t == "Haunted";
    if (roleLfo2 && (pl.third == "Motion" || pl.third == "Drift" || pl.third == "Wobble")) pl.third = pl.shortS ? "Width" : "Swell";
    if ((t == "Dampened" || t == "Stab") && (pl.third == "Swell" || pl.third == "Bloom")) pl.third = "Width";
    if (roleMacro.isNotEmpty())
    {
        if (roleMacro == pl.third) pl.third = pl.fourth;
        pl.fourth = roleMacro;
    }
    if (pl.third == pl.fourth) pl.fourth = pl.third == "Width" ? "Body" : "Width";

    // 3. rhythm presets
    if (m.pattern == 1)
    {   // marimba arpeggio: up, sixteenths, one octave
        p.arp = arp (VaultArp::up, "1/16", 1, 0.6f, 0.0f, true);
        pl.third = "Length";
    }
    else if (m.pattern == 2)
    {   // music box arpeggio: up-down eighths
        p.arp = arp (VaultArp::upDown, "1/8", 1, 0.8f, 0.0f, true);
        pl.third = "Length";
    }
    else if (m.pattern == 3)
    {   // gated choir: sixteenths
        p.gate = gate ("1/16", { 1, 0, 1, 0.6f, 1, 0, 1, 0, 1, 0, 1, 0.6f, 1, 0, 1, 1 }, 1.0f, 0.3f, 1.0f, true);
        pl.third = "Gate";
    }
    else if (m.pattern == 4)
    {   // gated vocal pad: sixteenths
        p.gate = gate ("1/16", { 1, 0, 0, 1, 0, 0, 1, 0, 1, 0, 0, 1, 0, 0, 1, 0 }, 1.0f, 0.3f, 1.0f, true);
        pl.third = "Gate";
    }
    if (pl.third == pl.fourth) pl.fourth = pl.third == "Width" ? "Body" : "Width";

    p.fx = c.build();
    p.finish = 0.04f;
    return pl;
}

//------------------------------------------------------------------------------ 2. C5 comfort of the whole v3 sound
struct Probe
{
    juce::AudioBuffer<float> c5;
    float loud = -120, s25 = 0, s6 = 0, s8 = 0, h1 = 0, centroid = 0, peak127 = 0, tailEnd = -200, tailMax = -200;
};
Probe probe (const VaultPreset& p, const float* mv, double total, bool withPeak = true)
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
    double num = 0;
    for (int i = 1; i < (int) sp.pow.size(); ++i) num += i * sp.binHz * sp.pow[(size_t) i];
    r.centroid = (float) (num / tot);
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
    r.peak127 = r.c5.getMagnitude (0, r.c5.getNumSamples());
    if (withPeak)
    {
        juce::MidiBuffer hb;
        hb.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, (juce::uint8) 127), 0);
        hb.addEvent (juce::MidiMessage::noteOff (1, vault::homeNote), (int) (0.7 * sr));
        const auto hard = VaultEngine::renderMidi (p, hb, (int) (0.8 * sr), sr, mv);
        r.peak127 = std::max (hard.getMagnitude (0, hard.getNumSamples()), r.peak127);
    }
    return r;
}
// the designed v3 sound keeps a margin under the gate's comfort limits (the macros may move from there); the inharmonic
// bells are exempt from the fundamental rule, every voice and bar keeps it
juce::String comfort (VaultPreset& p, bool bell)
{
    const float ceil = recipegate::centroidCeiling (p.category) - 80.0f;
    juce::String log;
    for (int it = 0; it < 10; ++it)
    {
        const auto x = probe (p, p.macroDefaults, 3.0, false);
        const bool bright = x.s25 > 0.075f || x.s6 > 0.02f || x.s8 > 0.007f || x.centroid > ceil;
        const bool weak = ! bell && x.h1 < -2.5f;
        if (! bright && ! weak) return log;
        int e = -1;
        for (int i = 0; i < (int) p.fx.size(); ++i) if (p.fx[(size_t) i].type == FT::eq) e = i;
        if (e < 0)
        {   // an EQ at its canonical place (before delay / reverb)
            Chain c (p.fx); c.at (FT::eq); p.fx = c.build();
            continue;
        }
        auto& q = p.fx[(size_t) e];
        if (bright)
        {
            log << "h";
            q.set ("high", std::max (-16.0f, q.get ("high") - 2.0f)).set ("highFreq", std::min (q.get ("highFreq"), 5000.0f));
            for (auto& s : p.fx) if (s.type == FT::drive) s.set ("tone", std::max (2500.0f, std::min (s.get ("tone"), 6000.0f) * 0.85f));
            if (q.get ("high") <= -15.9f) return log + "!";
            continue;
        }
        log << "l";
        const float f0 = recipegate::soundingHz (p, vault::homeNote);
        q.set ("lowFreq", juce::jlimit (30.0f, 1000.0f, std::max (q.get ("lowFreq"), 1.4f * f0)));
        if (q.get ("low") >= 12.0f) return log + "!";
        q.set ("low", std::min (12.0f, q.get ("low") + 1.5f));
    }
    return log;
}

//------------------------------------------------------------------------------ 3. the sound macros
void designMacros (VaultPreset& p, const Meta& meta, const Plan& pl)
{
    Chain c (p.fx);
    auto has = [&] (FT x) { return c.has (x); };
    const bool vox = p.category == vault::catVocalsChoir;

    // 1. tone: the mallet / strike / snap (attack + brightness), or the brightness of a held sound
    {
        const auto& n = pl.tone;
        M m (n.toRawUTF8(), 0.6f);
        if (n == "Mallet" || n == "Strike" || n == "Snap")
        {   // soft felt .. hard mallet: the attack and the brightness together (darker has more room than brighter)
            m = M (n.toRawUTF8(), 0.6f);
            if (has (FT::transient)) m.fxp (c, FT::transient, "attack", 0.7f);
            m.rel ("cutoff", 4.0f);
            if (n == "Snap" && has (FT::compressor)) m.fxp (c, FT::compressor, "threshold", 10.0f);
        }
        else m.rel ("cutoff", 5.0f).fxp (c, FT::eq, "tilt", 10.0f);
        setMacro (p, 0, m);
    }
    // 2. space: the reverb amount, length and pre-delay (a delay that is part of the sound comes along)
    {
        M m (pl.space.toRawUTF8(), 0.4f);
        const bool small = c.get (FT::reverb, "type") == (float) R::room;
        m.fxp (c, FT::reverb, "mix", 0.6f).fxp (c, FT::reverb, "decay", small ? 2.0f : 1.6f);
        if (! small) m.fxp (c, FT::reverb, "predelay", 40.0f);
        if (has (FT::delay) && c.get (FT::delay, "mix") > 0.05f && pl.third != "Echo" && pl.fourth != "Echo" && pl.third != "Throw" && pl.fourth != "Throw")
            m.fxp (c, FT::delay, "mix", 0.25f);
        setMacro (p, 1, m);
    }
    auto layerMacro = [&] (const char* name, bool upToo) -> M
    {
        const int k = meta.addIdx >= 0 && meta.addIdx < (int) p.parts.size() ? meta.addIdx : (int) p.parts.size() - 1;
        const float l = p.parts[(size_t) k].level;
        M m (name, upToo ? 0.5f : 1.0f);
        m.abs (("layer" + juce::String (k + 1) + ".level").toRawUTF8(), l, upToo ? 2.0f * l : l, 0.0f, 2.0f);
        return m;
    };
    // 3 / 4. the family / role characters
    auto make = [&] (const juce::String& n) -> M
    {
        if (n == "Swell" || n == "Bloom") { M m (n.toRawUTF8(), 0.0f); m.rel ("quick.attack", 0.42f).rel ("quick.release", 0.22f); return m; }
        if (n == "Vibrato") { M m ("Vibrato", 0.0f); m.abs ("lfo1.depth", 0.0f, 1.0f, 0, 1); return m; }
        if (n == "Motor") { M m ("Motor", 0.0f); m.abs ("lfo1.depth", 0.0f, 1.0f, 0, 1); return m; }
        if (n == "Wobble") { M m ("Wobble", 0.0f); m.abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1); return m; }
        if (n == "Motion" || n == "Drift")
        {   // from the designed slow drift to a deeper, faster one, wider
            const float rate = p.mod.lfo[1].rate, dep = p.mod.lfo[1].depth;
            M m (n.toRawUTF8(), 0.0f); m.abs ("lfo2.depth", dep, 1.0f - dep, 0, 1).abs ("lfo2.rate", rate, std::max (0.0f, 1.5f - rate), 0.02f, 2.0f);
            if (has (FT::width)) m.fxp (c, FT::width, "width", 0.25f);
            return m;
        }
        if (n == "Body" || n == "Warmth") { M m (n.toRawUTF8(), 0.5f); m.fxp (c, FT::eq, "low", 9.0f).fxp (c, FT::eq, "mid", 4.0f); return m; }
        if (n == "Grit" || n == "Tape")
        {
            // a drive that is silent as designed starts the knob at 0
            M m (n.toRawUTF8(), c.get (FT::drive, "amount") < 0.01f ? 0.0f : n == "Tape" ? 0.5f : 0.2f);
            if (has (FT::drive)) m.fxp (c, FT::drive, "amount", n == "Grit" ? 0.8f : 0.5f).toMax ("fx.drive.output", c.get (FT::drive, "output"), c.get (FT::drive, "output") - 6.0f);
            if (n == "Tape" && has (FT::lofi)) m.fxp (c, FT::lofi, "wow", 0.6f);
            if (n == "Tape" && p.mod.lfo[1].dests.size() == 1 && p.mod.lfo[1].dests[0].target == "pitch") m.abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1);
            return m;
        }
        if (n == "Crush")
        {
            M m ("Crush", 0.5f); m.fxp (c, FT::lofi, "crush", 0.6f).fxp (c, FT::lofi, "mix", 0.6f);
            if (p.mod.lfo[1].dests.size() == 1 && p.mod.lfo[1].dests[0].target == "pitch") m.abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1);
            return m;
        }
        if (n == "Echo" || n == "Throw")
        {
            const float dm = c.get (FT::delay, "mix");
            M m (n.toRawUTF8(), dm > 0.05f ? 0.2f : 0.0f);
            m.fxp (c, FT::delay, "mix", 0.5f).fxp (c, FT::delay, "feedback", 0.4f);
            return m;
        }
        if (n == "Width") { M m ("Width", 0.5f); m.fxp (c, FT::width, "width", 0.5f); return m; }
        if (n == "Chorus" || n == "Ensemble") { M m (n.toRawUTF8(), c.get (FT::chorus, "mix") < 0.01f ? 0.0f : 0.4f); m.fxp (c, FT::chorus, "mix", 0.7f).fxp (c, FT::chorus, "depth", 0.5f); return m; }
        if (n == "Reverse") { M m ("Reverse", 0.5f); m.fxp (c, FT::reverse, "mix", 0.8f); return m; }
        if (n == "Shadow") { M m ("Shadow", 0.5f); m.fxp (c, FT::ghost, "mix", 0.8f); return m; }
        if (n == "Gate") { M m ("Gate", 1.0f); m.abs ("gate.depth", p.gate.depth, 1.0f, 0, 1); return m; }
        if (n == "Length") { M m ("Length", 0.5f); m.abs ("arp.gate", p.arp.gate, 0.8f, 0.1f, 1.0f); return m; }
        if (n == "Sub") return layerMacro ("Sub", true);
        if (n == "Octave") return layerMacro ("Octave", true);
        if (n == "Halo") return layerMacro ("Halo", true);
        if (n == "Breath") return layerMacro ("Breath", true);
        if (n == "Sparkle") return layerMacro ("Sparkle", false);   // an octave-up layer: only down from the design
        M m ("Width", 0.5f); m.fxp (c, FT::width, "width", 0.5f); return m;
    };
    setMacro (p, 2, make (pl.third));
    setMacro (p, 3, make (pl.fourth));

    // performance: mod wheel = vibrato / motor where there is one (more space otherwise), aftertouch = expression on held
    // sounds, velocity keeps the touch against the compressor
    if (! p.mod.lfo[0].dests.empty()) p.mod.modWheel.push_back ({ "lfo1.depth", 0.8f });
    else p.mod.modWheel.push_back ({ "fx.reverb.mix", 0.25f });
    if (! pl.shortS) p.mod.aftertouch.push_back ({ "volume", 0.25f });
    const bool deadStroke = meta.role == "Dampened" || meta.role == "Stab";   // their compressor holds the peaks: no velocity lift
    if ((has (FT::compressor) || has (FT::drive)) && ! deadStroke) p.mod.velocity.push_back ({ "volume", pl.shortS ? 0.7f : 0.45f });
    juce::ignoreUnused (vox);
}

//------------------------------------------------------------------------------ macro extremes against the comfort rules
juce::String judge (const Probe& x, const Probe& ref, float lin, bool longTail, bool bell)
{
    if (x.peak127 * lin >= 0.885f && x.peak127 > ref.peak127 * 1.06f) return "clip";
    if (x.s25 > 0.10f && x.s25 > ref.s25 + 0.005f) return "harsh-2-5k";
    if ((x.s6 > 0.03f && x.s6 > ref.s6 + 0.002f) || (x.s8 > 0.01f && x.s8 > ref.s8 + 0.001f)) return "piercing-highs";
    if (! bell && x.h1 < -3.0f && x.h1 < ref.h1 - 0.5f) return "fundamental";
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
struct MacroReport { int fixes = 0, checked = 0; juce::StringArray harsh, quiet; };
MacroReport checkMacros (VaultPreset& p, float lin, bool bell)
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
        ++mr.checked;
        Probe lo, hi;
        for (int round = 0; round < 5; ++round)
        {
            float mv[4];
            std::copy (std::begin (p.macroDefaults), std::end (p.macroDefaults), mv);
            mv[i] = 0.0f; lo = probe (p, mv, total);
            mv[i] = 1.0f; hi = probe (p, mv, total);
            const auto badLo = judge (lo, ref, lin, longTail, bell), badHi = judge (hi, ref, lin, longTail, bell);
            if (badLo.isEmpty() && badHi.isEmpty()) break;
            if (round == 4) { mr.harsh.add (p.name + ": " + mac.name + " (" + (badLo.isNotEmpty() ? badLo : badHi) + ")"); break; }
            if (envInt ("PV_MACROLOG", 0) != 0) std::printf ("  narrow %s / %s: %s | %s\n", p.name.toRawUTF8(), mac.name.toRawUTF8(), badLo.toRawUTF8(), badHi.toRawUTF8());
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
        const float dist = extremeDistance (lo, hi);
        if (envInt ("PV_MACROLOG", 0) != 0)
            std::printf ("  macro %s / %s: dist %.2f, loud %.1f / %.1f / %.1f, AM %.2f / %.2f, tail %.1f / %.1f dB\n", p.name.toRawUTF8(), mac.name.toRawUTF8(), dist,
                         lo.loud, ref.loud, hi.loud, amDepth (lo.c5), amDepth (hi.c5), tailRatioDb (lo.c5), tailRatioDb (hi.c5));
        if (dist < 1.0f && std::abs (lo.loud - hi.loud) < 1.0f && std::abs (amDepth (lo.c5) - amDepth (hi.c5)) < 0.4f && ! pitchLfo
            && std::abs (tailRatioDb (lo.c5) - tailRatioDb (hi.c5)) < 2.0f)
            mr.quiet.add (p.name + ": " + mac.name);
    }
    return mr;
}

//------------------------------------------------------------------------------ 4. gate failures of the finished sound
// the macros / routings come off before a fix (their designed points are the chain's values) and are made again after
void strip (VaultPreset& p)
{
    for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; }
    p.mod.modWheel.clear(); p.mod.velocity.clear(); p.mod.aftertouch.clear();
}
// a mechanical fix of the chain (or the parts) for a gate failure; false = no fix for it
bool fixV3 (VaultPreset& p, const juce::String& fail)
{
    Chain c (p.fx);
    if (fail == "mono-loss" || fail == "low-side")
    {
        if (c.has (FT::chorus)) c.at (FT::chorus).set ("mix", c.get (FT::chorus, "mix") * 0.4f);
        auto& w = c.at (FT::width);
        w.set ("width", std::min (0.5f, w.get ("width")) * 0.7f).set ("monoBelow", std::max (200.0f, w.get ("monoBelow")));
        for (auto& part : p.parts) { part.pan *= 0.6f; if (part.kind == VaultPartKind::sample) part.set ("s.width", std::min (0.5f, part.get ("s.width", 1.0f))); }
    }
    else if (fail == "tail")
    {
        if (c.has (FT::reverb)) c.at (FT::reverb).set ("decay", std::max (0.25f, c.get (FT::reverb, "decay") * 0.7f)).set ("mix", c.get (FT::reverb, "mix") * 0.85f);
        if (c.has (FT::delay)) c.at (FT::delay).set ("feedback", std::min (0.2f, c.get (FT::delay, "feedback")));
        for (auto& part : p.parts)
        {
            if (part.kind == VaultPartKind::sample)
            {
                if (part.get ("s.oneShot", -1.0f) != 0.0f) { const float d = part.get ("s.decay", 0.0f); part.set ("s.decay", d > 0.0f ? d * 0.7f : 5.5f); part.set ("s.sustain", 0.0f); }
                else part.set ("s.release", std::min (part.get ("s.release", 1.0f) > 0 ? part.get ("s.release", 1.0f) : 1.0f, 0.8f));
            }
            else if (part.kind == VaultPartKind::synth) part.set ("release", std::min (part.get ("release", 0.3f), 0.8f));
            else part.set ("m.release", std::min (part.get ("m.release", 1.0f), 1.0f));
        }
    }
    else if (fail == "harsh-2-5k" || fail == "piercing-highs" || fail == "air-8k" || fail == "centroid")
    {
        auto& e = c.at (FT::eq);
        e.set ("high", std::max (-16.0f, e.get ("high") - 3.0f)).set ("highFreq", std::min (4500.0f, e.get ("highFreq"))).set ("tilt", std::max (-6.0f, e.get ("tilt") - 1.0f));
        if (c.has (FT::drive)) c.at (FT::drive).set ("tone", std::max (2500.0f, std::min (6000.0f, c.get (FT::drive, "tone")) * 0.8f));
        if (c.has (FT::reverb)) c.at (FT::reverb).set ("damping", std::min (1.0f, c.get (FT::reverb, "damping") + 0.2f));
        if (std::abs (e.get ("mid")) < 0.01f) e.set ("mid", -3.0f).set ("midFreq", 3200.0f).set ("midQ", 0.8f);   // the 2-5 kHz band itself
        const bool cut = lowerCut (p, 0.8f);
        if (e.get ("high") <= -15.9f && ! cut) return false;
    }
    else if (fail == "chord-peak")
    {   // the strike of a chord peaks far over the limiter: a faster, deeper compressor on the attack
        if (c.has (FT::compressor)) c.at (FT::compressor).set ("threshold", std::max (-48.0f, c.get (FT::compressor, "threshold") - 6.0f)).set ("attack", 1.0f)
                                                       .set ("ratio", std::max (3.0f, c.get (FT::compressor, "ratio"))).set ("mix", 1.0f);
        else c.put (fx::comp (-28, 3.0f, 1.0f, 120, 2.0f, 1.0f));
    }
    else if (fail == "too-quiet")
    {   // a short, quiet sound: every part's calibrated gain up 3 dB (the trim then lifts less)
        bool any = false;
        for (auto& part : p.parts) if (part.get ("gain", 0.0f) < 11.9f) { part.set ("gain", std::min (12.0f, part.get ("gain", 0.0f) + 3.0f)); any = true; }
        if (! any) return false;
    }
    else if (fail == "velocity")
    {
        if (c.has (FT::compressor)) c.at (FT::compressor).set ("ratio", 1.5f).set ("mix", c.get (FT::compressor, "mix") * 0.6f);
        for (auto& part : p.parts) part.set (part.kind == VaultPartKind::sample ? "s.velSens" : part.kind == VaultPartKind::synth ? "velSens" : "m.velSens", 1.0f);
    }
    else if (fail == "click-start" || fail == "click-end")
    {
        for (auto& part : p.parts)
            if (fail == "click-start") { if (part.kind == VaultPartKind::sample) part.set ("s.attack", std::max (0.004f, part.get ("s.attack", 0.0f))); else if (part.kind == VaultPartKind::synth) part.set ("attack", std::max (0.004f, part.get ("attack", 0.002f))); }
            else { if (part.kind == VaultPartKind::sample) part.set ("s.release", std::max (0.12f, part.get ("s.release", 0.0f))); else if (part.kind == VaultPartKind::synth) part.set ("release", std::max (0.12f, part.get ("release", 0.3f))); }
        if (fail == "click-start" && c.has (FT::transient)) c.at (FT::transient).set ("attack", c.get (FT::transient, "attack") * 0.5f);
    }
    else return false;
    p.fx = c.build();
    return true;
}
// a near-duplicate of an earlier preset: the role is pushed further (more of its space, echo, chorus and movement)
void pushRole (VaultPreset& p)
{
    Chain c (p.fx);
    if (c.has (FT::reverb))
        c.at (FT::reverb).set ("mix", std::min (0.5f, c.get (FT::reverb, "mix") * 1.35f + 0.04f)).set ("size", std::min (1.0f, c.get (FT::reverb, "size") + 0.12f))
                         .set ("decay", std::min (2.5f, c.get (FT::reverb, "decay") * 1.2f));
    if (c.has (FT::delay) && c.get (FT::delay, "mix") > 0.05f) c.at (FT::delay).set ("mix", std::min (0.35f, c.get (FT::delay, "mix") * 1.4f));
    if (c.has (FT::chorus) && c.get (FT::chorus, "mix") > 0.05f) c.at (FT::chorus).set ("mix", std::min (0.7f, c.get (FT::chorus, "mix") * 1.5f));
    if (! p.mod.lfo[1].dests.empty()) p.mod.lfo[1].depth = std::min (1.0f, std::max (0.3f, p.mod.lfo[1].depth * 1.5f));
    p.fx = c.build();
}
} // namespace v3

// the ids the shipped bank gave these sounds (the v2 bank as assembled: 385 presets). A v2 sound keeps its id and name;
// a sound the selection picks today that the shipped bank did not have gets the next free id of its category, so an
// id never changes its sound.
const std::vector<std::pair<const char*, const char*>>& shippedIds()
{
    static const std::vector<std::pair<const char*, const char*>> t {
{ "cv.bell.0001", "Marimba" }, { "cv.bell.0002", "Vibraphone" }, { "cv.bell.0003", "Xylophone" }, 
{ "cv.bell.0004", "Glockenspiel" }, { "cv.bell.0005", "Tubular Bells" }, { "cv.bell.0006", "Celesta" }, 
{ "cv.bell.0007", "Hand Chimes" }, { "cv.bell.0008", "Glass Harp" }, { "cv.bell.0009", "Music Box Comb" }, 
{ "cv.bell.0010", "Bowed Vibraphone" }, { "cv.bell.0011", "Motor Vibes" }, { "cv.bell.0013", "Hard Vibraphone" }, 
{ "cv.bell.0015", "Concert Chimes" }, { "cv.bell.0016", "Church Chimes" }, { "cv.bell.0017", "Wine Glass Tremolo" }, 
{ "cv.bell.0018", "High Marimba" }, { "cv.bell.0019", "High Vibraphone" }, { "cv.bell.0020", "Soft Xylophone" }, 
{ "cv.bell.0021", "High Hand Chimes" }, { "cv.bell.0023", "Low Music Box Comb" }, { "cv.bell.0024", "High Bowed Vibraphone" }, 
{ "cv.bell.0028", "High Hard Vibraphone" }, { "cv.bell.0029", "Bell Tower" }, { "cv.bell.0030", "Crystal Bell" }, 
{ "cv.bell.0031", "Handbell" }, { "cv.bell.0034", "High Music Box Comb" }, { "cv.bell.0036", "Low Bell Tower" }, 
{ "cv.bell.0037", "Ice Crystal Bell" }, { "cv.bell.0038", "Gong Bell" }, { "cv.bell.0041", "Trap Bells" }, 
{ "cv.bell.0042", "Celesta and Harp" }, { "cv.bell.0045", "Piano and Vibes" }, { "cv.bell.0046", "Chimes and Marimba" }, 
{ "cv.bell.0047", "Bowed Glass" }, { "cv.bell.0049", "Cathedral Bells" }, { "cv.bell.0050", "Bells and Choir" }, 
{ "cv.bell.0051", "Music Box and Strings" }, { "cv.bell.0052", "Vibes and Flute" }, { "cv.bell.0053", "Midnight Bells" }, 
{ "cv.bell.0056", "Marimba and Nylon" }, { "cv.bell.0057", "Vibes and Upright Bass" }, { "cv.bell.0058", "Xylophone and Marimba" }, 
{ "cv.bell.0059", "Xylophone & Glockenspiel" }, { "cv.bell.0060", "Ice Bells" }, { "cv.bell.0061", "Bells and Timpani" }, 
{ "cv.bell.0063", "Vibes and Choir" }, { "cv.bell.0064", "Celesta and Strings" }, { "cv.bell.0065", "Music Box and Celesta" }, 
{ "cv.bell.0066", "Trap Mallet Stack" }, { "cv.bell.0067", "Dark Trap Bells" }, { "cv.bell.0069", "Bell Pad" }, 
{ "cv.bell.0070", "Chimes and Celesta" }, { "cv.bell.0071", "Wooden Bells" }, { "cv.bell.0072", "Music Box Lullaby" }, 
{ "cv.bell.0073", "Vibes and Celesta" }, { "cv.bell.0074", "Glockenspiel and Harp" }, { "cv.bell.0075", "Marimba & Glockenspiel" }, 
{ "cv.bell.0076", "Snowfall Bells" }, { "cv.bell.0078", "Marimba Swell" }, { "cv.bell.0079", "Celestial Choir Bells" }, 
{ "cv.bell.0080", "Glass and Choir" }, { "cv.bell.0083", "Xylophone & Pizzicato" }, { "cv.bell.0084", "Sunday Bells" }, 
{ "cv.bell.0085", "Bell Strings" }, { "cv.bell.0086", "Vibes and Nylon" }, { "cv.bell.0087", "Chimes & Glockenspiel" }, 
{ "cv.bell.0088", "Vibes and Strings" }, { "cv.bell.0166", "Stage Music Box & Glass" }, { "cv.bell.0175", "Velvet Glockenspiel" }, 
{ "cv.bell.0186", "Soft Felt Concert Chimes" }, { "cv.bell.0187", "Felt Church Chimes" }, { "cv.bell.0202", "Felt Handbell" }, 
{ "cv.bell.0212", "Velvet Trap Bells" }, { "cv.bell.0216", "Soft Felt Piano & Vibes" }, { "cv.bell.0217", "Felt Chimes & Marimba" }, 
{ "cv.bell.0220", "Felt Cathedral Bells" }, { "cv.bell.0222", "Felt Music Box & Strings" }, { "cv.bell.0223", "Felt Vibes and Flute" }, 
{ "cv.bell.0224", "Velvet Midnight Bells" }, { "cv.bell.0271", "Echo Hard Xylophone" }, { "cv.bell.0296", "Midnight Toy Celesta" }, 
{ "cv.bell.0302", "Midnight Piano & Vibes" }, { "cv.bell.0358", "Faded Concert Chimes" }, { "cv.bell.0359", "Dusty Church Chimes" }, 
{ "cv.bell.0388", "Faded Piano and Vibes" }, { "cv.bell.0393", "Cassette Bells & Choir" }, { "cv.bell.0408", "Dusty Dark Trap Bells" }, 
{ "cv.bell.0411", "Dusty Chimes & Celesta" }, { "cv.bell.0415", "Faded Glockenspiel & Harp" }, { "cv.bell.0423", "Faded Sunday Bells" }, 
{ "cv.bell.0427", "Dampened Marimba" }, { "cv.bell.0429", "Stopped Xylophone" }, { "cv.bell.0433", "Dampened Hand Chimes" }, 
{ "cv.bell.0434", "Stopped Music Box Comb" }, { "cv.bell.0435", "Muted Motor Vibes" }, { "cv.bell.0436", "Stopped Deep Marimba" }, 
{ "cv.bell.0439", "Stopped Concert Chimes" }, { "cv.bell.0440", "Dampened Church Chimes" }, { "cv.bell.0442", "Dampened High Vibraphone" }, 
{ "cv.bell.0444", "Stopped High Hand Chimes" }, { "cv.bell.0466", "Unfolding Tubular Bells" }, { "cv.bell.0467", "Blooming Hand Chimes" }, 
{ "cv.bell.0469", "Unfolding Motor Vibes" }, { "cv.bell.0473", "Bowed Concert Chimes" }, { "cv.bell.0478", "Bowed High Hand Chimes" }, 
{ "cv.bell.0493", "Behind the Wall Marimba" }, { "cv.bell.0494", "Next Room Vibraphone" }, { "cv.bell.0495", "Muffled Xylophone" }, 
{ "cv.bell.0497", "Next Room Tubular Bells" }, { "cv.bell.0501", "Muffled Music Box Comb" }, { "cv.bell.0503", "Next Room Motor Vibes" }, 
{ "cv.bell.0510", "Muffled High Marimba" }, { "cv.bell.0511", "Next Room High Vibraphone" }, { "cv.bell.0528", "Next Room Ice Crystal Bell" }, 
{ "cv.bell.0531", "Reverse Marimba" }, { "cv.bell.0533", "Rewind Xylophone" }, { "cv.bell.0534", "Reverse Glockenspiel" }, 
{ "cv.bell.0537", "Reverse Hand Chimes" }, { "cv.bell.0539", "Rewind Music Box Comb" }, { "cv.bell.0541", "Backward Motor Vibes" }, 
{ "cv.bell.0542", "Rewind Deep Marimba" }, { "cv.bell.0545", "Rewind Concert Chimes" }, { "cv.bell.0548", "Rewind High Marimba" }, 
{ "cv.bell.0553", "Backward Low Music Box Comb" }, { "cv.bell.0561", "Reverse Handbell" }, { "cv.bell.0592", "Cloud Low Music Box Comb" }, 
{ "cv.bell.0595", "Cloud Fast Motor Vibes" }, { "cv.bell.0601", "Cloud Steel Glockenspiel" }, { "cv.bell.0603", "Dream High Music Box Comb" }, 
{ "cv.bell.0617", "Haze Handbell Choir" }, { "cv.bell.0650", "Haze Xylophone & Pizzicato" }, { "cv.bell.0656", "Heavy Marimba" }, 
{ "cv.bell.0658", "Deep Xylophone" }, { "cv.bell.0659", "Heavy Glockenspiel" }, { "cv.bell.0660", "Weighted Tubular Bells" }, 
{ "cv.bell.0661", "Deep Celesta" }, { "cv.bell.0662", "Heavy Hand Chimes" }, { "cv.bell.0663", "Deep Music Box Comb" }, 
{ "cv.bell.0664", "Weighted Motor Vibes" }, { "cv.bell.0665", "Heavy Deep Marimba" }, { "cv.bell.0666", "Heavy Hard Vibraphone" }, 
{ "cv.bell.0668", "Deep Concert Chimes" }, { "cv.bell.0691", "Twin Marimba" }, { "cv.bell.0692", "Doubled Celesta" }, 
{ "cv.bell.0693", "Doubled High Marimba" }, { "cv.bell.0694", "Twin High Vibraphone" }, { "cv.bell.0695", "Doubled High Hand Chimes" }, 
{ "cv.bell.0696", "Twin Low Celesta" }, { "cv.bell.0697", "Twin High Bowed Vibraphone" }, { "cv.bell.0698", "Twin High Hard Vibraphone" }, 
{ "cv.bell.0700", "Doubled Toy Celesta" }, { "cv.bell.0702", "Sparkle Vibraphone" }, { "cv.bell.0703", "Starlit Xylophone" }, 
{ "cv.bell.0704", "Glimmer Glockenspiel" }, { "cv.bell.0705", "Sparkle Tubular Bells" }, { "cv.bell.0707", "Glimmer Hand Chimes" }, 
{ "cv.bell.0708", "Sparkle Glass Harp" }, { "cv.bell.0711", "Starlit Deep Marimba" }, { "cv.bell.0712", "Glimmer Hard Vibraphone" }, 
{ "cv.bell.0714", "Starlit Concert Chimes" }, { "cv.bell.0715", "Glimmer Church Chimes" }, { "cv.bell.0719", "Sparkle Soft Xylophone" }, 
{ "cv.bell.0731", "Glowing Marimba" }, { "cv.bell.0732", "Halo Vibraphone" }, { "cv.bell.0733", "Aurora Xylophone" }, 
{ "cv.bell.0734", "Glowing Glockenspiel" }, { "cv.bell.0735", "Halo Tubular Bells" }, { "cv.bell.0736", "Aurora Celesta" }, 
{ "cv.bell.0738", "Halo Glass Harp" }, { "cv.bell.0739", "Aurora Music Box Comb" }, { "cv.bell.0740", "Glowing Bowed Vibraphone" }, 
{ "cv.bell.0742", "Aurora Deep Marimba" }, { "cv.bell.0743", "Glowing Hard Vibraphone" }, { "cv.bell.0779", "Shimmer Bowed Vibraphone" }, 
{ "cv.bell.0807", "Wide Gong Bell" }, { "cv.bell.0816", "Wide Bowed Glass" }, { "cv.bell.0857", "Smoky Vibraphone" }, 
{ "cv.bell.0858", "Saturated Xylophone" }, { "cv.bell.0859", "Gritty Glockenspiel" }, { "cv.bell.0860", "Smoky Tubular Bells" }, 
{ "cv.bell.0861", "Saturated Celesta" }, { "cv.bell.0862", "Gritty Hand Chimes" }, { "cv.bell.0863", "Smoky Glass Harp" }, 
{ "cv.bell.0864", "Saturated Music Box Comb" }, { "cv.bell.0865", "Gritty Bowed Vibraphone" }, { "cv.bell.0868", "Gritty Hard Vibraphone" }, 
{ "cv.bell.0899", "Seasick Tubular Bells" }, { "cv.bell.0905", "Seasick Motor Vibes" }, { "cv.bell.0907", "Warped Hard Vibraphone" }, 
{ "cv.bell.0923", "Seasick Bell Tower" }, { "cv.bell.0928", "Warped High Music Box Comb" }, { "cv.bell.0942", "Deep Water Music Box Comb" }, 
{ "cv.bell.0963", "Deep Water Crystal Bell" }, { "cv.bell.0967", "Shadow High Music Box Comb" }, { "cv.bell.0970", "Shadow Ice Crystal Bell" }, 
{ "cv.bell.1082", "Lo-Fi High Bowed Vibraphone" }, { "cv.vox.0001", "Female Eh Chop" }, { "cv.vox.0002", "Choir Oohs" }, 
{ "cv.vox.0003", "Female Ah" }, { "cv.vox.0004", "Male Ah" }, { "cv.vox.0005", "Chamber Choir" }, 
{ "cv.vox.0006", "Vocal Pad" }, { "cv.vox.0007", "Female Oo" }, { "cv.vox.0008", "Soprano Ah" }, 
{ "cv.vox.0009", "Female Ha" }, { "cv.vox.0010", "Breathy Eh Chop" }, { "cv.vox.0011", "Choir Aahs" }, 
{ "cv.vox.0012", "Male Ha" }, { "cv.vox.0013", "Female Whisper" }, { "cv.vox.0015", "Men's Choir" }, 
{ "cv.vox.0016", "Choir Hits" }, { "cv.vox.0017", "Breathy Female" }, { "cv.vox.0019", "Female Oh" }, 
{ "cv.vox.0021", "Male Hum" }, { "cv.vox.0022", "Oo Vocal Pad" }, { "cv.vox.0023", "Male Vocal Pad" }, 
{ "cv.vox.0024", "Low Choir Oohs" }, { "cv.vox.0025", "High Choir Oohs" }, { "cv.vox.0026", "Low Choir Aahs" }, 
{ "cv.vox.0027", "High Choir Aahs" }, { "cv.vox.0028", "High Eh Chop" }, { "cv.vox.0029", "High Breathy Chop" }, 
{ "cv.vox.0030", "Tenor Ah" }, { "cv.vox.0031", "Male Oo" }, { "cv.vox.0032", "Female Ee" }, 
{ "cv.vox.0033", "Breathy Male" }, { "cv.vox.0034", "Male Whisper" }, { "cv.vox.0035", "Female Ho" }, 
{ "cv.vox.0036", "Female Hoo" }, { "cv.vox.0037", "Male Ho" }, { "cv.vox.0039", "Mixed Choir Oh" }, 
{ "cv.vox.0042", "Men's Choir Ah" }, { "cv.vox.0045", "Men's Choir Hits" }, { "cv.vox.0048", "Synth Choir" }, 
{ "cv.vox.0049", "Low Synth Choir" }, { "cv.vox.0052", "Low Female Ah" }, { "cv.vox.0053", "Female Hee" }, 
{ "cv.vox.0055", "High Choir Hits" }, { "cv.vox.0058", "Female Aw" }, { "cv.vox.0060", "Whispered Oo" }, 
{ "cv.vox.0061", "Ee Vocal Pad" }, { "cv.vox.0062", "Low Vocal Pad" }, { "cv.vox.0063", "Bass Voice" }, 
{ "cv.vox.0065", "Voice and Strings" }, { "cv.vox.0067", "SATB Choir" }, { "cv.vox.0068", "Choir Stab" }, 
{ "cv.vox.0069", "Ha Stack" }, { "cv.vox.0071", "Whisper Pad" }, { "cv.vox.0074", "Soprano and Choir" }, 
{ "cv.vox.0076", "Vocal Pad and Strings" }, { "cv.vox.0077", "Duet Ah" }, { "cv.vox.0079", "Choir and Piano" }, 
{ "cv.vox.0080", "Voice and Flute" }, { "cv.vox.0081", "Choir and Bells" }, { "cv.vox.0082", "Breath Choir" }, 
{ "cv.vox.0084", "Basso Profundo" }, { "cv.vox.0085", "Choir Aahs and Cellos" }, { "cv.vox.0086", "Chops and Choir" }, 
{ "cv.vox.0087", "Two Sopranos" }, { "cv.vox.0089", "Female Oo and Pad" }, { "cv.vox.0091", "Ghost Duet" }, 
{ "cv.vox.0094", "Chop Pad" }, { "cv.vox.0095", "Whispered Chop" }, { "cv.vox.0097", "Tremolo Choir" }, 
{ "cv.vox.0098", "Epic Choir" }, { "cv.vox.0102", "Ha Chop Stack" }, { "cv.vox.0103", "Synth and Voice" }, 
{ "cv.vox.0149", "Distant Whisper Choir" }, { "cv.vox.0159", "Chapel Tenor Ha" }, { "cv.vox.0162", "Chapel Tenor Oh" }, 
{ "cv.vox.0180", "Chapel Male Hum & Choir" }, { "cv.vox.0205", "Distant Choir Pad" }, { "cv.vox.0210", "Intimate Female Eh Chop" }, 
{ "cv.vox.0211", "Close Choir Oohs" }, { "cv.vox.0216", "Intimate Female Oo" }, { "cv.vox.0217", "Close Soprano Ah" }, 
{ "cv.vox.0219", "Intimate Breathy Eh Chop" }, { "cv.vox.0225", "Intimate Choir Hits" }, { "cv.vox.0230", "Hushed Male Hum" }, 
{ "cv.vox.0233", "Hushed Low Choir Oohs" }, { "cv.vox.0234", "Intimate High Choir Oohs" }, { "cv.vox.0237", "Intimate High Eh Chop" }, 
{ "cv.vox.0238", "Close High Breathy Chop" }, { "cv.vox.0432", "Dusty Female Whisper" }, { "cv.vox.0446", "Faded High Choir Aahs" }, 
{ "cv.vox.0458", "Faded Mixed Choir Oh" }, { "cv.vox.0493", "Cassette Soprano & Choir" }, { "cv.vox.0494", "Faded Male Hum & Choir" }, 
{ "cv.vox.0498", "Dusty Choir and Piano" }, { "cv.vox.0504", "Dusty Choir Aahs & Cellos" }, { "cv.vox.0505", "Cassette Chops & Choir" }, 
{ "cv.vox.0506", "Faded Two Sopranos" }, { "cv.vox.0508", "Cassette Female Oo & Pad" }, { "cv.vox.0525", "Rising Choir Oohs" }, 
{ "cv.vox.0531", "Rising Soprano Ah" }, { "cv.vox.0540", "Dawn Male Hum" }, { "cv.vox.0544", "Swelling High Choir Oohs" }, 
{ "cv.vox.0629", "Ambient Choir and Harp" }, { "cv.vox.0646", "Weightless Aria & Strings" }, { "cv.vox.0653", "Floating Whisper Choir Pad" }, 
{ "cv.vox.0662", "Rewind Female Ha" }, { "cv.vox.0669", "Reverse Choir Hits" }, { "cv.vox.0671", "Rewind Male Oh" }, 
{ "cv.vox.0698", "Rewind Men's Choir Hits" }, { "cv.vox.0706", "Backward Female Hee" }, { "cv.vox.0708", "Reverse High Choir Hits" }, 
{ "cv.vox.0775", "Ensemble Breathy Female Ah" }, { "cv.vox.0809", "Chorus Hum Duet" }, { "cv.vox.0869", "Deep Water Synth Choir" }, 
{ "cv.vox.0885", "Octave Choir Oohs" }, { "cv.vox.0888", "Octave Chamber Choir" }, { "cv.vox.0889", "Doubled Vocal Pad" }, 
{ "cv.vox.0899", "Twin Female Oh" }, { "cv.vox.0902", "Twin Oo Vocal Pad" }, { "cv.vox.0904", "Twin High Choir Oohs" }, 
{ "cv.vox.0906", "Doubled High Choir Aahs" }, { "cv.vox.0907", "Twin High Eh Chop" }, { "cv.vox.0908", "Octave High Breathy Chop" }, 
{ "cv.vox.0909", "Doubled Tenor Ah" }, { "cv.vox.0911", "Octave Female Ee" }, { "cv.vox.0947", "Heavenly Female Ha" }, 
{ "cv.vox.0948", "Glowing Breathy Eh Chop" }, { "cv.vox.0950", "Heavenly Male Ha" }, { "cv.vox.0954", "Glowing Choir Hits" }, 
{ "cv.vox.0960", "Heavenly Low Choir Oohs" }, { "cv.vox.0964", "Glowing High Eh Chop" }, { "cv.vox.0965", "Halo High Breathy Chop" }, 
{ "cv.vox.0967", "Glowing Male Oo" }, { "cv.vox.0971", "Halo Female Ho" }, { "cv.vox.0973", "Glowing Male Ho" }, 
{ "cv.vox.0981", "Heavenly Men's Choir Hits" }, { "cv.vox.0997", "Stab Female Eh Chop" }, { "cv.vox.0998", "Clipped Choir Oohs" }, 
{ "cv.vox.1003", "Short Low Choir Oohs" }, { "cv.vox.1004", "Stab High Choir Oohs" }, { "cv.vox.1005", "Clipped Low Choir Aahs" }, 
{ "cv.vox.1007", "Stab High Eh Chop" }, { "cv.vox.1008", "Clipped High Breathy Chop" }, { "cv.vox.1009", "Short Synth Choir" }, 
{ "cv.vox.1010", "Stab Low Synth Choir" }, { "cv.vox.1035", "Young Male Ho" }, { "cv.vox.1055", "Small Breathy Female Ah" }, 
{ "cv.vox.1077", "Colossal Tenor Ah" }, { "cv.vox.1099", "Colossal Tenor Ha" }, { "cv.vox.1140", "Straight Choir Ee" }, 
{ "cv.vox.1148", "Airy Female Eh Chop" }, { "cv.vox.1149", "Breathy Choir Oohs" }, { "cv.vox.1154", "Airy Female Oo" }, 
{ "cv.vox.1155", "Breathy Soprano Ah" }, { "cv.vox.1158", "Breathy Choir Aahs" }, { "cv.vox.1162", "Windswept Men's Choir" }, 
{ "cv.vox.1164", "Windswept Breathy Female" }, { "cv.vox.1170", "Breathy Male Vocal Pad" }, { "cv.vox.1171", "Windswept Low Choir Oohs" }, 
{ "cv.vox.1172", "Airy High Choir Oohs" }, { "cv.vox.1174", "Windswept High Choir Aahs" }, { "cv.vox.1211", "Gritty Female Eh Chop" }, 
{ "cv.vox.1212", "Smoky Choir Oohs" }, { "cv.vox.1217", "Gritty Female Oo" }, { "cv.vox.1218", "Smoky Soprano Ah" }, 
{ "cv.vox.1219", "Saturated Female Ha" }, { "cv.vox.1220", "Gritty Breathy Eh Chop" }, { "cv.vox.1221", "Smoky Choir Aahs" }, 
{ "cv.vox.1234", "Saturated Low Choir Oohs" }, { "cv.vox.1235", "Gritty High Choir Oohs" }, { "cv.vox.1238", "Gritty High Eh Chop" }, 
{ "cv.vox.1239", "Smoky High Breathy Chop" }, { "cv.vox.1282", "Melted Female Ha" }, { "cv.vox.1289", "Warped Choir Hits" }, 
{ "cv.vox.1310", "Warped Male Ho" }, { "cv.vox.1318", "Melted Men's Choir Hits" }, { "cv.vox.1322", "Warped Low Synth Choir" }, 
{ "cv.vox.1328", "Warped High Choir Hits" }, { "cv.vox.1333", "Melted Whispered Oo" }, { "cv.vox.1337", "Haunted Female Eh Chop" }, 
{ "cv.vox.1343", "Haunted Female Oo" }, { "cv.vox.1344", "Phantom Soprano Ah" }, { "cv.vox.1345", "Spectral Female Ha" }, 
{ "cv.vox.1346", "Haunted Breathy Eh Chop" }, { "cv.vox.1347", "Phantom Choir Aahs" }, { "cv.vox.1349", "Haunted Female Whisper" }, 
{ "cv.vox.1352", "Haunted Choir Hits" }, { "cv.vox.1363", "Spectral High Choir Aahs" }, { "cv.vox.1364", "Haunted High Eh Chop" }, 
{ "cv.vox.1365", "Phantom High Breathy Chop" }
    };
    return t;
}
juce::String nameKey (const juce::String& n) { return n.replace (" & ", " and ").toLowerCase(); }
void assignShippedIds (std::vector<VaultPreset>& sel, std::vector<Meta>& metas)
{
    std::map<juce::String, juce::String> byName;
    int next[vault::numCategories] {};
    for (auto& [id, name] : shippedIds())
    {
        byName[nameKey (name)] = id;
        const juce::String sid (id);
        for (int c = 0; c < vault::numCategories; ++c)
            if (sid.startsWith (juce::String ("cv.") + vault::categorySlug (c) + "."))
                next[c] = std::max (next[c], sid.fromLastOccurrenceOf (".", false, false).getIntValue());
    }
    std::set<juce::String> used;
    for (auto& p : sel)
    {
        auto it = byName.find (nameKey (p.name));
        p.id = it != byName.end() && used.count (it->second) == 0 ? it->second
             : juce::String ("cv.") + vault::categorySlug (p.category) + "." + juce::String (++next[p.category]).paddedLeft ('0', 4);
        used.insert (p.id);
    }
    std::vector<size_t> order (sel.size());
    for (size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort (order.begin(), order.end(), [&] (size_t a, size_t b)
    {
        if (sel[a].category != sel[b].category) return sel[a].category < sel[b].category;
        return sel[a].id.fromLastOccurrenceOf (".", false, false).getIntValue() < sel[b].id.fromLastOccurrenceOf (".", false, false).getIntValue();
    });
    std::vector<VaultPreset> s2; std::vector<Meta> m2;
    for (auto i : order) { s2.push_back (sel[i]); m2.push_back (metas[i]); }
    sel.swap (s2); metas.swap (m2);
}

// the few presets where a rhythm is the point of the sound (arp / gate on, plain names), appended after the selection
struct Pattern { const char* name; int category; std::vector<VaultPart> parts; int kind; int pattern; };
std::vector<Pattern> patterns()
{
    return {
        { "Marimba Arpeggio", vault::catBellsMallets, { with (marimba (0), { { "s.cutoff", 3000.0f } }) }, kStrike, 1 },
        { "Music Box Eighths", vault::catBellsMallets, { musicBox (1) }, kStrike, 2 },
        { "Gated Choir", vault::catVocalsChoir, choirBlend (0, 0.6f, 8), kHeld, 3 },
        { "Gated Vocal Pad", vault::catVocalsChoir, { vpad (2, 0.9f) }, kPad, 4 },
    };
}

template <typename Fn> void parallel (int n, Fn fn)
{
    std::atomic<int> next { 0 };
    std::vector<std::thread> pool;
    const int nt = std::max (1, std::min (n, juce::SystemStats::getEnvironmentVariable ("PV_THREADS", "3").getIntValue()));
    for (int t = 0; t < nt; ++t) pool.emplace_back ([&] { for (int i; (i = next++) < n;) fn (i); });
    for (auto& t : pool) t.join();
}
} // namespace

std::vector<VaultPreset> build_perc_voc (const RecipeContext& ctx)
{
    std::vector<VaultPreset> sel;
    std::vector<Meta> metas;
    // 1. the v2 selection (PV_V2CACHE=<file.cvb>: saved once with its roles, for design iterations)
    const auto cachePath = juce::SystemStats::getEnvironmentVariable ("PV_V2CACHE", {});
    if (cachePath.isNotEmpty() && juce::File (cachePath).existsAsFile())
    {
        vault::loadBankFile (juce::File (cachePath), sel);
        juce::StringArray lines;
        lines.addLines (juce::File (cachePath + ".roles").loadFileAsString());
        std::map<juce::String, Meta> byId;
        for (auto& l : lines)
        {
            auto f = juce::StringArray::fromTokens (l, "\t", "");
            if (f.size() < 6) continue;
            Meta m; m.noun = f[1]; m.role = f[2]; m.kind = f[3].getIntValue(); m.addIdx = f[4].getIntValue(); m.bell = f[5].getIntValue() != 0;
            byId[f[0]] = m;
        }
        for (auto& p : sel) metas.push_back (byId[p.id]);
        std::printf ("perc-voc: %d v2 presets from the cache\n", (int) sel.size());
    }
    else
    {
        // PV_FULL=1: the whole v2 search (2,653 candidates); default: the shipped selection, pinned by name
        std::set<juce::String> pin;
        for (auto& [id, name] : shippedIds()) pin.insert (nameKey (name));
        sel = v2Selection (ctx, metas, v3::envInt ("PV_FULL", 0) != 0 ? nullptr : &pin);
        if (cachePath.isNotEmpty())
        {
            vault::saveBankFile (juce::File (cachePath), sel);
            juce::String rl;
            for (size_t i = 0; i < sel.size(); ++i)
                rl << sel[i].id << "\t" << metas[i].noun << "\t" << metas[i].role << "\t" << metas[i].kind << "\t" << metas[i].addIdx << "\t" << (metas[i].bell ? 1 : 0) << "\n";
            juce::File (cachePath + ".roles").replaceWithText (rl);
        }
    }
    if (v3::envInt ("PV_V3", 1) == 0) return sel;

    // 2. the rhythm presets (calibrated and repaired like every other sound), appended
    {
        Calib calib;
        std::set<juce::String> names;
        for (auto& p : sel) names.insert (p.name.toLowerCase());
        for (auto& pt : patterns())
        {
            bool ok = names.count (juce::String (pt.name).toLowerCase()) == 0;
            for (auto& part : pt.parts)
                if (part.kind == VaultPartKind::sample && (! ctx.hasArticulation (part.source, part.articulation) || ! inRange (ctx, part, 0))) ok = false;
            if (! ok) continue;
            VaultPreset p;
            p.category = pt.category; p.name = pt.name; p.parts = pt.parts; p.id = "cv.pattern";
            for (auto& part : p.parts) if (part.kind == VaultPartKind::sample) part.set ("revMix", 0.0f);
            if (! repair (ctx, p, calib, false)) { std::printf ("perc-voc: pattern %s cannot be made comfortable\n", pt.name); continue; }
            Meta m; m.noun = pt.name; m.kind = pt.kind; m.pattern = pt.pattern;
            sel.push_back (p); metas.push_back (m);
        }
    }
    assignShippedIds (sel, metas);
    for (size_t i = 0; i < sel.size(); ++i) metas[i].fam = v3::family (sel[i], metas[i]);
    // PV_ONLY=<id or name,...>: just these presets (demos, listening)
    if (const auto only = juce::StringArray::fromTokens (juce::SystemStats::getEnvironmentVariable ("PV_ONLY", {}), ",", ""); ! only.isEmpty())
    {
        std::vector<VaultPreset> o; std::vector<Meta> mo;
        for (size_t i = 0; i < sel.size(); ++i) if (only.contains (sel[i].id) || only.contains (sel[i].name)) { o.push_back (sel[i]); mo.push_back (metas[i]); }
        sel.swap (o); metas.swap (mo);
    }

    // 3. v3: every preset finished - chain + movement, C5 comfort of the whole sound, sound macros
    std::vector<juce::String> fitLog (sel.size());
    std::vector<v3::Plan> plans (sel.size());
    parallel ((int) sel.size(), [&] (int i)
    {
        auto& p = sel[(size_t) i];
        p.gain = 1.0f;
        plans[(size_t) i] = v3::designChain (p, metas[(size_t) i]);
        fitLog[(size_t) i] = v3::comfort (p, metas[(size_t) i].bell);
        v3::designMacros (p, metas[(size_t) i], plans[(size_t) i]);
    });
    {
        int nf = 0, nbad = 0;
        for (auto& l : fitLog) { if (l.isNotEmpty()) ++nf; if (l.endsWith ("!")) ++nbad; }
        std::printf ("perc-voc: v3 designed %d presets; comfort refit %d (%d at the EQ limit)\n", (int) sel.size(), nf, nbad);
    }
    if (v3::envInt ("PV_SELFGATE", 1) == 0) return sel;

    // 4. the bank gate's own selection on the finished sounds (same analysis, same order, the shared 3.5), plus the
    //    assembly QA's spectral limits, so the bank assembly keeps every preset this returns
    auto gateFail = [] (recipegate::Result& r)
    {
        juce::String why;
        if (r.fail.isEmpty() && ! vault::validatePreset (r.preset, &why)) r.fail = "invalid: " + why;
        // the assembly's peak control lifts a peaky sound by its quietBy: the trim must stay under the limit with that lift
        if (r.fail.isEmpty() && r.gainDb + (r.quietBy > 1.3f ? r.quietBy : 0.0f) > recipegate::maxTrimDb - 1.0f) r.fail = "too-quiet";
        if (r.fail.isEmpty() && r.chordOver > 6.0f) r.fail = "chord-peak";   // more than the assembly's peak control can hold
        if (r.fail.isEmpty() && r.share8k > recipegate::maxShare8k) r.fail = "air-8k";
        if (r.fail.isEmpty() && r.centroid > recipegate::centroidCeiling (r.preset.category)) r.fail = "centroid";
    };
    std::vector<recipegate::Result> res (sel.size());
    std::vector<juce::String> fixLog (sel.size());
    parallel ((int) sel.size(), [&] (int i)
    {
        auto& r = res[(size_t) i];
        r.preset = sel[(size_t) i];
        recipegate::analyse (r);
        gateFail (r);
        // a gate failure of the finished sound: a mechanical fix of the chain, then the macros again (up to 3 rounds)
        auto p = sel[(size_t) i];
        for (int round = 0; round < 3 && r.fail.isNotEmpty(); ++round)
        {
            const auto fail = r.fail;
            v3::strip (p);
            if (! v3::fixV3 (p, fail)) break;
            v3::designMacros (p, metas[(size_t) i], plans[(size_t) i]);
            recipegate::Result r2;
            r2.preset = p;
            recipegate::analyse (r2);
            gateFail (r2);
            fixLog[(size_t) i] << fail << (r2.fail.isEmpty() ? " fixed; " : " -> " + r2.fail + "; ");
            if (r2.fail.isEmpty()) { sel[(size_t) i] = p; r = std::move (r2); }
            else r.fail = r2.fail;
        }
    });
    // in order: a near-duplicate of an earlier preset gets its role pushed further once, then the order is run again
    auto select = [&] (std::vector<int>& acc, float& minPair, bool quiet)
    {
        acc.clear(); minPair = 1.0e9f;
        std::vector<int> near;
        for (int i = 0; i < (int) res.size(); ++i)
        {
            auto& r = res[(size_t) i];
            if (r.fail.isNotEmpty() && ! r.fail.startsWith ("near-duplicate")) continue;
            float d = 1.0e9f;
            juce::String nearest;
            for (int j : acc) if (const float dj = recipegate::distance (r.print, res[(size_t) j].print); dj < d) { d = dj; nearest = res[(size_t) j].preset.name; }
            r.fail = d < 3.5f ? "near-duplicate (" + nearest + ", " + juce::String (d, 2) + ")" : juce::String();
            if (r.fail.isEmpty()) { acc.push_back (i); minPair = std::min (minPair, d); }
            else near.push_back (i);
        }
        if (! quiet) for (int i : near) std::printf ("  near: %s %s\n", res[(size_t) i].preset.name.toRawUTF8(), res[(size_t) i].fail.toRawUTF8());
        return near;
    };
    std::vector<int> acc;
    float minPair = 1.0e9f;
    const auto near = select (acc, minPair, false);
    parallel ((int) near.size(), [&] (int k)
    {
        const int i = near[(size_t) k];
        auto p = sel[(size_t) i];
        v3::strip (p);
        v3::pushRole (p);
        v3::designMacros (p, metas[(size_t) i], plans[(size_t) i]);
        recipegate::Result r2;
        r2.preset = p;
        recipegate::analyse (r2);
        gateFail (r2);
        fixLog[(size_t) i] << "near: role pushed; ";
        if (r2.fail.isEmpty()) { sel[(size_t) i] = p; res[(size_t) i] = std::move (r2); }
    });
    if (! near.empty()) select (acc, minPair, true);
    std::map<juce::String, int> fails;
    for (int i = 0; i < (int) res.size(); ++i)
    {
        const auto& r = res[(size_t) i];
        if (fixLog[(size_t) i].isNotEmpty()) std::printf ("  fix %s %s: %s\n", r.preset.id.toRawUTF8(), r.preset.name.toRawUTF8(), fixLog[(size_t) i].toRawUTF8());
        if (r.fail.isNotEmpty())
        {
            ++fails[r.fail.upToFirstOccurrenceOf (" ", false, false).upToFirstOccurrenceOf (":", false, false)];
            std::printf ("  rejected %s %s: %s\n", r.preset.id.toRawUTF8(), r.preset.name.toRawUTF8(), r.fail.toRawUTF8());
        }
    }
    std::printf ("perc-voc: %d of %d pass the gate (min distance %.2f)\n", (int) acc.size(), (int) res.size(), minPair);
    for (auto& [f, n] : fails) std::printf ("  perc-voc rejected %-16s %d\n", f.toRawUTF8(), n);
    if (const auto fp = juce::SystemStats::getEnvironmentVariable ("PV_TSV", {}); fp.isNotEmpty())
    {
        juce::String s;
        for (size_t i = 0; i < res.size(); ++i)
        {
            const auto& r = res[i];
            s << r.preset.id << "\t" << r.preset.name << "\t" << metas[i].fam << "\t" << metas[i].role << "\t" << (r.fail.isEmpty() ? juce::String ("ok") : r.fail) << "\t"
              << juce::String (r.h1Db, 1) << "\t" << juce::String (100 * r.share25, 2) << "\t" << juce::String (100 * r.share6k, 2) << "\t" << juce::String (100 * r.share8k, 2)
              << "\t" << juce::String (r.centroid, 0) << "\t" << juce::String (r.velRange, 1) << "\t" << juce::String (r.tail, 1) << "\t" << juce::String (r.gainDb, 1) << "\t" << fitLog[i] << "\n";
        }
        juce::File (fp).replaceWithText (s);
    }

    // 5. macro extremes
    std::vector<v3::MacroReport> mr (acc.size());
    std::vector<VaultPreset> kept (acc.size());
    parallel ((int) acc.size(), [&] (int k)
    {
        kept[(size_t) k] = sel[(size_t) acc[(size_t) k]];
        if (v3::envInt ("PV_MACROCHECK", 1) != 0) mr[(size_t) k] = v3::checkMacros (kept[(size_t) k], res[(size_t) acc[(size_t) k]].preset.gain, metas[(size_t) acc[(size_t) k]].bell);
    });
    int fixes = 0, checked = 0;
    juce::StringArray harsh, quiet;
    for (auto& m : mr) { fixes += m.fixes; checked += m.checked; harsh.addArray (m.harsh); quiet.addArray (m.quiet); }
    std::printf ("perc-voc: %d macros checked at 0 and 1; ranges narrowed %d times; %d macro extremes still outside the comfort rules; %d macros barely audible\n",
                 checked, fixes, harsh.size(), quiet.size());
    for (auto& h : harsh) std::printf ("  harsh: %s\n", h.toRawUTF8());
    for (auto& q : quiet) std::printf ("  quiet: %s\n", q.toRawUTF8());
    return kept;
}

static recipe::Registrar vaultRecipeRegistrar_perc_voc ("perc-voc", build_perc_voc);
