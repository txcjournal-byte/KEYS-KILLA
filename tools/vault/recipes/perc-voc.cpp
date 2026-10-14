// CHROME VAULT v2 recipes: BELLS & MALLETS + VOCALS & CHOIR (sound designer "perc-voc").
// Run: VaultBankGen -recipe perc-voc (shared gate: RecipeGate.h).
//
// DESIGN NOTE (research summary; sources at the end of this note)
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
// PV_ROLE_CAP (default 11), PV_VERBOSE (1 = print every near-duplicate drop with its nearest kept preset).
#include "RecipeGate.h"
#include <map>
#include <mutex>
#include <algorithm>
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

struct Cand { juce::String name; int category; std::vector<VaultPart> parts; bool bell; int role; };

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
        if (name.isNotEmpty()) out.push_back ({ name, v.category, parts, v.bell, role });
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
} // namespace

std::vector<VaultPreset> build_perc_voc (const RecipeContext& ctx)
{
    std::vector<Cand> cands;
    addAll (ctx, cands);
    if (const int lim = juce::SystemStats::getEnvironmentVariable ("PV_LIMIT", "0").getIntValue(); lim > 0 && lim < (int) cands.size())
        cands.resize ((size_t) lim);
    std::vector<VaultPreset> all;
    std::vector<char> bells;
    std::vector<int> roles;
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
    int dropped = 0;
    for (size_t i = 0; i < all.size(); ++i)
    {
        if (keep[i]) { out.push_back (std::move (all[i])); outRole.push_back (roles[i]); }
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
    if (ownMin <= 0.0f) return out;
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
    for (int i : acc) kept.push_back (out[(size_t) i]);
    return kept;
}

static recipe::Registrar vaultRecipeRegistrar_perc_voc ("perc-voc", build_perc_voc);
