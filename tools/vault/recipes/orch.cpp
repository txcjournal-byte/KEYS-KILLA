// CHROME VAULT recipes: STRINGS + BRASS + ORCHESTRAL (sound designer "orch"). Run: VaultBankGen -recipe orch (RecipeGate.h).
//
// DARK STRINGS NOTE (owner's FL Studio test report #1, item 17: strings "dark, powerful, deep")
//  * New: the cinematic low strings (addCinematic, finished by v3::designCinematic / v3::cinematicMacros) - viola, cello
//    and contrabass sections from the library stacked in octaves (key C5 sounds C4 / C3 / C2), cellos in octaves, open
//    fifths, the contrabass vibrato section, as sustains, slow swells, sul tasto / con sordino pads, marcato (with a
//    spiccato layer on the stroke), spiccato / pizzicato ostinatos and tremolo; Orchestral: horns, trombones + tuba,
//    choir, bassoon, timpani and harp with the low sections. Each: section glue, tape power with clean lows, a warm dark
//    EQ, a long damped hall with the strings' long pre-delay, slow movement, 4 named macros (Bow / Bite / Tone / Snap /
//    Pluck, Hall, Swell / Bloom / Drift / Tension / Power / Weight, Basses / Depth / Echo). Ids cv.str.1001+ and
//    cv.orch.1001+; they lead their category in the browser (the Strings default is "Cinematic Low Octaves") and win
//    every near-duplicate pair against the older generated variants, which drop out.
//    Also two-style strokes (accent into tremolo, tremolo settling into a sustain, spiccato over a bowed bass bed), fifths
//    as tremolo and sul tasto, brass stabs over the low strings, timpani with pizzicato basses, choir over cello tremolo.
//    The sections' top is rolled off gently (sample low-pass 2.2 kHz x 4^tone, tape top 6.5 kHz - 2.5 kHz x darkness, a
//    -2 .. -5 dB high shelf): dark, but the bow still speaks (C5: centroid 100-460 Hz, 2-5 kHz <= 2.4 %, nothing above 6 kHz).
//  * The violin-led sounds keep only their plain, Distant, Swelling, Muted, Muffled and Flautando roles, and the high solo
//    violin register is gone (no crushed / taped / reversed / stabbed thin violins); the bases stay in the list so the
//    other sounds keep their names and ids. After the selection, a Strings sound that is only violins / solo violin /
//    high violas / the synth string machine (or is led by the violins an octave up) ships in its plain form only (53
//    processed variants left out), so most of the category is the low sections.
//  * Environment (development): ORCH_SPLIT=<bank>,<dir> writes the other recipes' presets of a finished bank as -cache
//    files (re-assemble around a rebuilt orch); ORCH_DEMO=<bank>|<dir>|<names> renders the owner's demos (low chord
//    progression + swell, WAV + OGG + demos.tsv); ORCH_DUMP=<file> saves the designed presets before the self gate;
//    ORCH_MERGE=<shipped>,<assembled>,<out> swaps only this recipe's presets (Strings / Brass / Orchestral) in a
//    shipped bank.
//
// v3 DESIGN NOTE - every preset ships finished (namespace v3 below)
//  * Research: orchestral libraries are recorded in a hall and mixed lightly - subtractive EQ (low-mid build-up / box at
//    250-400 Hz, no 3 kHz boosts on brass: "edgy"), much lighter compression than pop (an LA-2A-style glue on the string
//    bus; brass "3:1, fast-ish attack, never too much or it gets robotic"), tape saturation to smooth the brass top instead
//    of dulling it with EQ, and one shared hall with pre-delay so the bow / tongue attack reads before the tail (strings
//    sit nearest the audience: the longest pre-delay; percussion / far sources: short pre-delay, more and darker tail).
//    Timpani: compression for the body, a mud cut, the reverb's lows cut, parallel compression + a little distortion for
//    a hit that pops. Pop / soul horn sections: tape + glue + plate + a short slapback. The string machine is an ensemble
//    chorus + phaser into a hall. Commercial patches put vibrato, expression and the hall amount on the performance
//    controls (mod wheel / aftertouch) and name macros by playing gesture (bow, swell, vibrato, hall).
//  * So each preset's chain = FAMILY (the instrument's finished sound) + ROLE (the v2 treatment as a processing style),
//    slots in the canonical rack order; the fitted v2 EQ (low shelf / top cut that made C5 comfortable) is kept and the
//    stereo image never gets wider than the fit allowed:
//      bowed sustains   glue comp (-26 dB, 1.6:1, 40 / 300 ms, 50 % parallel), box cut 350 Hz, hall (30 ms pre-delay,
//                       220 Hz low cut), width with mono lows, a slow "bow pressure" breathing LFO (cutoff + level),
//                       a delayed 5.4 Hz vibrato LFO (off as designed)          macros Bow, Hall, Swell, Vibrato / Motion
//      pizz / spiccato  transient, fast glue, box cut, a shorter hall, a silent 1/8D echo   Snap, Hall, Body, Echo
//      string machine   ensemble chorus, slow phaser, hall, slow brightness drift            Brightness, Hall, Ensemble, Phase
//      brass sustains   glue comp (2.5:1, 20 / 200 ms), tape smoothing (clean lows, 5 kHz tone), 300 Hz warmth, hall by
//                       register (horns: bigger and longer, low brass: 260 Hz low cut, vintage trombone: jazz plate),
//                       4.8-5.4 Hz delayed vibrato                       Bite, Hall, Swell, Vibrato / Width / Warmth
//      brass stabs      transient, 3:1 glue, tape, short plate                                Bite, Plate, Punch, Echo
//      muted brass      soft comp, nasal band tamed, plate, a 1/4D throw delay ready          Bite, Plate, Swell, Echo
//      pop horns        tape, glue, plate, 110 ms slapback                                    Bite, Plate, Tape, Punch / Swell
//      synth brass      comp, tube edge, chorus, plate / hall; velocity opens the cutoff      Cutoff, Hall, Envelope, Detune
//      timpani          transient, parallel comp, mud cut, big hall with 300 Hz low cut       Mallet, Hall, Weight, Grit / Swell
//      harp             light comp, hall, a silent 1/8D echo                                  Pluck, Hall, Body, Echo
//      hits             transient, 3.5:1 parallel glue, tape grit, big hall (260 Hz low cut)  Punch, Hall, Grit, Echo
//      tutti            bus glue, a whisper of tape, mud cut, big hall, breathing             Brightness, Hall, Swell, Power
//      winds / doublings soft comp, hall, breathing, delayed vibrato                          Breath / Bow, Hall, Swell, Vibrato
//      colour (choir, organ, glass, bells, piano + strings) bigger softer hall, slow drift    Brightness, Hall, Swell, Motion
//    roles: Distant = more, darker hall, short pre-delay, narrower, top rolled off ("Distance"); Muted / Muffled = room;
//    Stab = short plate + transient; Tight = small room; Dark = tape drive into a damped plate ("Grit");
//    Reverse = reverse + hall; Dusty / Tape = tape drive + lo-fi + a tape-wobble pitch LFO + dark room ("Tape"); Crushed =
//    lo-fi + room ("Crush"); Stutter = 1/16 ping-pong repeats + room; Echoing = 1/4D ping-pong + hall ("Echo"); Floating /
//    Frozen = cloud reverb + a slow brightness / pan drift ("Drift", "Space"); Swirling = phaser ("Phase"); Marcato =
//    transient; Doubled = wider.
//  * Performance: mod wheel = vibrato depth on sustains (more hall on short sounds), aftertouch = expression swell on
//    sustains, velocity keeps the touch against the compressor (>= 3 dB velocity range).
//  * Macros: every target sits exactly at its designed value at the knob's default (the preset loads as designed). Each
//    macro is rendered at 0 and 1 (C5) and a side that breaks a comfort rule the designed sound keeps (2-5 kHz, > 6 / 8 kHz,
//    fundamental, clipping, +4 dB louder, longer tail) is shortened until it passes; drive macros trim the drive's output.
//  * The whole v3 sound is refitted at C5 before the macros are made (2-5 kHz <= 8 %, > 6 kHz <= 2.2 %, > 8 kHz <= 0.7 %,
//    centroid 80 Hz under the category ceiling, fundamental >= -2.3 dB): the top shelf comes down / the low shelf goes up.
//  * Arp / gate: off everywhere except 5 rhythm presets with plain names, patterns from Init (rate / mode / octaves / gate /
//    accents only), appended after the v2 presets so the ids stay: Pizzicato Arpeggio (up, 1/8), Spiccato
//    Ostinato (as played 1/16, accents on the beats), Gated Strings (1/16 gate), Horn Pulse Eighths (1/8 gate), Woodwind
//    Arpeggio (up-down 1/16).
//  * Selection: the v2 selection runs unchanged (so every v2 preset keeps its id), then the finished sounds go through the
//    gate's own analysis in the same order (the shared 3.5 distance, gate fails, make-up gain, assembly QA's 8 kHz share and
//    centroid ceiling) and only the accepted ones get the macro check.
//  * Result (VaultBankGen -recipe orch -qa, library /home/user/cv-library): the v2 selection gives 646 sounds + 5 rhythm
//    presets; 644 pass the gate (7 near-duplicates after the v3 processing) - Strings 268, Brass 214, Orchestral 162 - min
//    pairwise distance 3.52; 598 of the 608 shipped v2 presets keep their id and name. At C5: fundamental >= -2.3 dB of the
//    strongest partial, 2-5 kHz <= 8.0 %, > 6 kHz <= 0.56 %, > 8 kHz <= 0.55 %, centroid <= 1301 Hz, velocity range >= 3.5 dB,
//    tails <= 8.9 s; 27 sounds were refitted after the chain; peak control at assembly fits the 25 transient-heavy ones.
//    2,576 macros checked at 0 and 1: 542 ranges narrowed, none left outside the comfort rules; 179 (mostly the slow Drift /
//    Motion LFO macros and cloud / room reverb amounts, which a 2.5 s render barely shows) change the sound only slightly.
//  * Sources: forums.taxi.com "Need mixing help" (subtractive EQ, light compression, longer pre-delay for strings);
//    digitalspace.bradfordcollege.ac.uk "Mixing orchestral instruments" (pre-delay so the attack plays first; brass: gentle
//    3:1, 3 kHz boosts sound edgy); vi-control.net threads (one shared tail with 40-80 ms pre-delay; sampled brass:
//    saturation + a small reverb tail; thick horns); discussion.cambridge-mt.com (tape saturation smooths brass highs);
//    waves.com studioverse timpani chains (compression for the body, low / attack EQ, compressed reverb); forum.vsl.co.at
//    (parallel compression + a little distortion on orchestral percussion, low-cut reverb returns); soundfly flypaper
//    "Mixing and processing a virtual orchestra".
//
// v2 DESIGN NOTE (research summary; sources at the end of this note)
//  * How orchestral libraries voice these instruments: sections and soloists are recorded in a real hall and sold with
//    several mic perspectives - close / spot (dry, detailed, "in your face", the trap / pop perspective), Decca tree
//    (the balanced main pickup) and outriggers / ambient / surround (wide, distant, "cinematic"). Users either blend
//    tree + some close, or take dry close mics and add one shared algorithmic / convolution hall so every section sits in
//    the same space, with pre-delay so the bow / tongue attack reads before the tail. Here every sound gets such a
//    PERSPECTIVE: concert (hall) for the plain sound, distant (dark, big hall, narrower), muffled / behind glass, plus the
//    producer roles (cloud / frozen ambient, rhythmic delay, tape / vinyl / crushed, reverse ear candy, dark low-passed
//    trap colour).
//  * Articulations are the instrument's real playing techniques from the VSCO 2 CE / VCSL recordings: strings sustain,
//    vibrato, tremolo, pizzicato, spiccato for violin / viola / cello sections, solo violin and contrabass; brass
//    sustain, vibrato, staccato, straight / harmon mutes (trumpet), stopped / muted horn, trombone buzz / short / fall
//    (vintage trombone), tuba; timpani hits / rolls, concert harp, woodwind sustains / staccatos. Envelope roles that
//    are standard keyswitch articulations in commercial libraries (marcato = accent + decay, swell / crescendo,
//    sforzando stab, flautando / sul tasto = soft and dark, con sordino = muted and veiled, espressivo vibrato) are made
//    from the sustains with the sampler envelope, filter and designed vibrato.
//  * Sections and ensembles are built the way orchestrators and template builders double parts: brass sections are
//    2-4 players of the same solo instrument, each with its own tuning offset (+-5..11 cents), entry (up to 50 ms) and
//    seat (pan) - the natural chorus of a section; string ensembles stack sections across octaves (violins at pitch, violas
//    in unison, cellos an octave and basses two octaves below; cinematic low strings = cellos + basses; open fifths);
//    brass choirs follow the trumpet / horn / trombone / tuba registers; the classic doublings (flute + violins,
//    clarinet + violas, bassoon + cellos, horns + cellos, trombones + cellos, tuba + basses), tutti swells and the
//    hip-hop "orchestra hit" (spiccato strings + brass staccato + timpani). Trap / hip-hop practice: dark pizzicato and
//    spiccato ostinatos that cut through the 808, low brass stabs, short octave-bow strokes for hooks, string loops
//    with a lo-fi colour, hybrid brass / strings doubled with a quiet synth layer (EVOLVE orchestral engine).
//  * Comfort (C5 = MIDI 60, FL Studio naming; gate RecipeGate.h): orchestral recordings are full of 2-5 kHz energy
//    (bow noise, brass "blare", trumpet edge at forte) and the low violin / horn / trombone notes have a weak fundamental
//    (body and bell radiation favour partials 2-4). Every candidate is therefore fitted before the gate: the stereo
//    image is narrowed until the mono sum loses < 2 dB, the per-voice low-pass (12 dB/oct, velocity-tracked so soft
//    notes are darker, as real dynamics are) comes down until 2-5 kHz <= 7 % and > 6 kHz <= 2 % of the power, then a
//    low shelf and a lower cutoff bring the played fundamental within 2 dB of the strongest partial. Octave-up colour
//    layers stay >= 12 dB (K-weighted) under part 0; all doublings are at or below the main part's octave.
//  * The fundamental: where a low-pass would have to take the instrument's colour away (horns, trombones, the violin G
//    string, brass sections), the fit adds the producer's "sine for body": a quiet sine at part 0's pitch that follows
//    part 0's envelope (label "Warm Sine"), sized from a measured probe to the missing fundamental power, and refused when
//    it would raise the K-weighted loudness by more than 5 dB (then the sine would be the sound) - those registers are
//    dropped instead (e.g. low stopped horn, straight-mute trumpet, low trombone falls). Every sound is fitted once plain;
//    its roles start from the fitted sound and are refitted.
//  * Roles were chosen by measurement: a pure reverb / echo / chorus change of a sustained orchestral sound moves the
//    fingerprint by only 0.4-1.5 (it is rejected anyway), while perspective + filter (distant, muted, muffled, dark),
//    envelope (swell, stab, clipped, frozen, smooth entry), structure (a second detuned player, an octave-below
//    doubling), reverse, tape / vinyl / crushed colour and rhythmic delay change the sound.
//  * Distinctness: candidates are generated in priority order (every instrument / articulation / register plain
//    first, then sections and ensembles, then each perspective / role in turn). This recipe selects with the gate's own
//    analysis before the gate sees it: fingerprint = log-mel x time at C5 + C4 + C6, cepstrum, harmonic profile, motion,
//    width (RecipeGate.h); a plain sound must be >= 3.5 (the shared threshold) from everything accepted before it, a
//    role >= 4.3 (ORCH_DIST) - so the bank keeps every real instrument / section / ensemble that is audibly its own and
//    only the processing roles that change the sound most. Also refused: sounds that need more than +22 dB make-up
//    gain (a stab decayed away before the bow speaks would lift the recording's noise) and velocity ranges over 40 dB.
//    Measured limits of this fingerprint: a section made of one solo recording (3 detuned players) stays 1.4-1.9 from
//    the solo, so the solo is kept and the section only where it differs (e.g. trumpet section, brass choirs).
//  * Result (VaultBankGen -recipe orch, library /home/user/cv-library): 2536 candidates -> 629 presets (Strings 258,
//    Brass 214, Orchestral 157; 224 one-part, 176 two, 102 three, 127 four; 196 with a warm sine), all pass the shared
//    gate; minimum pairwise distance 3.50 (plain sounds), roles >= 4.3, median nearest neighbour 5.3. At C5: fundamental
//    >= -3.0 dB of the strongest partial, 2-5 kHz <= 7.0 %, > 6 kHz <= 1.8 %, tails <= 10 s, velocity 3.2..37 dB, octave-up
//    layers <= -13.7 dB, make-up gain <= 21.9 dB. 20 sounds were dropped by the fit (straight-mute and high muted
//    trumpets, low stopped horn, low trombone falls / buzz, ...); the rest by the selection.
//  Environment: ORCH_DIST (role distance, default 4.3), ORCH_TSV (per-candidate selection log), ORCH_LIMIT (first N
//  candidates), ORCH_VERBOSE (warm-sine levels), ORCH_TRACE=<sound> (fit iterations of one sound).
//  Sources: soundonsound.com orchestral library reviews (close / tree / outrigger / ambient mics); orchestraltools
//  helpscout "Which microphone positions should I download?"; vi-control.net threads on mic positions, close mics +
//  own reverb, brass EQ ("trumpet trouble taming highs"), orchestral doubling; violetrecording.com "How to record
//  trumpet" (2-5 kHz harshness, 200-400 Hz warmth); orchestrationonline.com "cello unison with double basses";
//  forum.vsl.co.at doubling threads (violins-trumpets, violas-horns, celli-trombones, basses-tuba); a sample-instrument
//  maker's tutorial "How to add orchestra to hip hop" (looped phrases, octave bow strokes, syncopated staccato, low brass
//  stabs); Bradford College
//  "Mixing orchestral instruments" (string pre-delay); a physically modelled trumpet's manual (ER / tail / pre-delay).
#include "RecipeGate.h"
#include <algorithm>
#include <atomic>
#include <map>
#include <set>
#include <thread>

namespace
{
using KV = std::vector<std::pair<juce::String, float>>;

enum Kind { kSus = 1, kTrem = 2, kShort = 4, kRing = 8 };
enum Fam { fStr = 1, fBrs = 2, fOrc = 4 };

VaultPart S (const char* id, const char* art, const char* label, int o = 0, KV kv = {})
{
    auto p = recipe::samplePart (id, art, label);
    p.set ("s.velSens", 0.3f);   // the velocity layers carry the dynamics; a little level on top (pp stays playable)
    for (auto& [k, v] : kv) p.set (k, v);
    p.octave = o;
    return p;
}
VaultPart lvl (VaultPart p, float l, float pan = 0.0f) { p.level = l; p.pan = pan; return p; }
VaultPart semi (VaultPart p, int s) { p.semi = s; return p; }
VaultPart with (VaultPart p, const KV& kv) { for (auto& [k, v] : kv) p.set (k, v); return p; }

// EVOLVE orchestral engine (saw ensemble through the brass / string / choir blend): the hybrid layer
VaultPart synthOrch (const char* label, float wave, int o, KV kv = {})
{
    auto p = recipe::synthPart (label);
    for (auto& [k, v] : KV { { "engine", 8 }, { "wave", wave }, { "unison", 4 }, { "detune", 0.18f }, { "filterType", 0 }, { "cutoff", 1800 },
                             { "reso", 0.05f }, { "keyTrack", 0.35f }, { "fenv", 0.15f }, { "fattack", 0.08f }, { "fdecay", 0.6f }, { "fsustain", 0.6f },
                             { "attack", 0.12f }, { "decay", 1.0f }, { "sustain", 0.9f }, { "release", 0.7f }, { "velSens", 0.6f },
                             { "macro2", 0 }, { "macro5", 0 }, { "master", 0 }, { "revMix", 0 }, { "chorus", 0 } })
        p.set (k, v);
    for (auto& [k, v] : kv) p.set (k, v);
    p.octave = o;
    return p;
}

// a section of one solo instrument: n players, own tuning, start and seat
std::vector<VaultPart> section (const char* id, const char* art, const char* label, int o, int n, KV kv = {})
{
    // players spread across the stage (the section's width), loose tuning (+-10 cents) and loose entries (up to 50 ms)
    static const float tune[] { 0.0f, -11.0f, 10.0f, -5.0f };
    static const float pan[] { 0.0f, -0.8f, 0.8f, -0.4f };
    static const float start[] { 0.0f, 35.0f, 18.0f, 50.0f };
    static const float lev[] { 0.75f, 0.7f, 0.7f, 0.6f };
    std::vector<VaultPart> v;
    for (int i = 0; i < n; ++i)
    {
        auto p = lvl (S (id, art, label, o, kv), lev[i], n == 1 ? 0.0f : pan[i]);
        p.set ("s.tune", tune[i]);
        if (start[i] > 0) p.set ("s.start", start[i]);
        p.set ("s.width", 0.35f);
        if (i > 0) p.set ("s.attack", 0.03f * (float) i);
        v.push_back (p);
    }
    return v;
}

// the cinematic low strings (owner's request, report item 17): how a preset is played and finished (v3::designCinematic)
enum Look { lNone = 0, lEpic, lSwell, lTasto, lMarcato, lSpicc, lPizz, lTrem };

struct Base
{
    juce::String noun;
    int cat, fam, kind;
    std::vector<VaultPart> parts;
    int pattern = 0;   // v3: the few rhythm presets (arp / gate on), appended after the selection
    juce::String sub;  // v3: the processing family (v3::subFamily)
    int look = lNone;  // cinematic low strings: the playing / finishing style
    float tone = 0.5f, hall = 0.5f;   // 0 darkest .. 1 most open; 0 close .. 1 cathedral
};

struct Treat
{
    juce::StringArray prefixes;   // "" = the plain sound
    int kinds, fams;
    KV kv;                         // s.* -> every sample part (synth parts get the matching amp keys), others -> part 0 FX
    int structural = 0;            // 1 = a second player (detuned, other seat), 2 = doubled an octave below
};

std::vector<Treat> treatments()
{
    const int all = kSus | kTrem | kShort | kRing, F = fStr | fBrs | fOrc;
    return {
        { { "" }, all, F, { { "revMix", 0.17f }, { "revSize", 0.62f }, { "revType", 0 } } },
        { { "Distant", "Faraway", "Gallery" }, all, F, { { "s.cutoff", 2600 }, { "revMix", 0.36f }, { "revSize", 0.9f }, { "revType", 0 }, { "width", 0.4f } } },
        { { "Swelling", "Rising", "Blooming" }, kSus | kTrem, F, { { "s.attack", 0.7f }, { "s.release", 1.3f }, { "revMix", 0.22f }, { "revSize", 0.7f }, { "revType", 0 } } },
        { { "Muted", "Sordino", "Veiled" }, kSus | kTrem | kShort, fStr | fOrc, { { "s.cutoff", 1100 }, { "revMix", 0.2f }, { "revSize", 0.55f }, { "revType", 0 } } },
        { { "Stab", "Hit", "Punch" }, kSus, F, { { "s.decay", 0.55f }, { "s.sustain", 0 }, { "s.release", 0.15f }, { "revMix", 0.12f }, { "revType", 1 } } },
        { { "Tight", "Clipped", "Snappy" }, kShort | kRing, F, { { "s.decay", 0.28f }, { "s.sustain", 0 }, { "s.release", 0.1f }, { "s.oneShot", 0 }, { "revMix", 0.06f }, { "revType", 1 } } },
        { { "Doubled", "Paired", "Twin" }, kSus | kTrem | kShort | kRing, F, { { "revMix", 0.17f }, { "revSize", 0.62f }, { "revType", 0 } }, 1 },
        { { "Octave", "Deep Octave", "Octave Doubled" }, kSus | kTrem | kShort | kRing, F, { { "revMix", 0.17f }, { "revSize", 0.62f }, { "revType", 0 } }, 2 },
        { { "Dark", "Midnight", "Shadow" }, all, F, { { "s.cutoff", 1300 }, { "drive", 0.18f }, { "driveType", 1 }, { "revMix", 0.12f }, { "revType", 1 } } },
        { { "Reverse", "Backward", "Rewind" }, all, F, { { "reverse", 0.5f }, { "revMix", 0.2f }, { "revType", 0 } } },
        { { "Muffled", "Behind Glass", "Underwater" }, all, F, { { "s.cutoff", 650 }, { "revMix", 0.16f }, { "revSize", 0.5f }, { "revType", 0 } } },
        { { "Smooth", "Bowless", "Soft Entry" }, kSus | kTrem, F, { { "s.start", 160 }, { "s.attack", 0.12f }, { "revMix", 0.18f }, { "revType", 0 } } },
        { { "Dusty", "Vinyl", "Faded" }, all, F, { { "wow", 0.5f }, { "crush", 0.22f }, { "eqHigh", -8 }, { "revMix", 0.12f }, { "revSize", 0.4f }, { "revType", 1 } } },
        { { "Tape", "Saturated", "Warm Tape" }, all, F, { { "drive", 0.35f }, { "driveType", 1 }, { "eqHigh", -4 }, { "wow", 0.2f }, { "revMix", 0.1f }, { "revType", 1 } } },
        { { "Flautando", "Whispered", "Feathered" }, kSus | kTrem, fStr, { { "s.cutoff", 1600 }, { "s.attack", 0.25f }, { "s.velSens", 0.5f }, { "revMix", 0.22f }, { "revType", 0 } } },
        { { "Stutter", "Pulsing", "Repeating" }, kShort | kRing | kSus, F, { { "delayMix", 0.42f }, { "delayTime", 3 }, { "delayFb", 0.5f }, { "delayMode", 0 }, { "revMix", 0.1f }, { "revType", 1 } } },
        { { "Floating", "Aurora", "Ethereal" }, kSus | kTrem | kRing, F, { { "revType", 2 }, { "revMix", 0.36f }, { "revSize", 0.45f }, { "s.attack", 0.3f }, { "s.release", 1.8f } } },
        { { "Frozen", "Suspended", "Glacial" }, kSus | kTrem, F, { { "s.attack", 1.4f }, { "s.release", 2.5f }, { "revType", 2 }, { "revMix", 0.42f }, { "revSize", 0.5f }, { "s.cutoff", 2200 } } },
        { { "Crushed", "Grainy", "Broken" }, kSus | kShort, F, { { "crush", 0.5f }, { "eqHigh", -6 }, { "revMix", 0.1f }, { "revType", 1 } } },
        { { "Swirling", "Spiral", "Orbit" }, kSus | kTrem, F, { { "phaser", 0.75f }, { "revMix", 0.15f }, { "revType", 0 } } },
        { { "Marcato", "Accented", "Bold" }, kSus | kTrem, F, { { "s.decay", 0.5f }, { "s.sustain", 0.35f }, { "s.release", 0.3f }, { "revMix", 0.15f }, { "revType", 0 } } },
        { { "Echoing", "Canyon", "Ricochet" }, all, F, { { "delayMix", 0.3f }, { "delayTime", 2 }, { "delayFb", 0.4f }, { "delayMode", 0 }, { "revMix", 0.12f }, { "revType", 0 } } },
    };
}

//==============================================================================
// the sounds
void addStrings (std::vector<Base>& b)
{
    struct In { const char* id; const char* label; std::vector<std::pair<const char*, const char*>> arts; std::vector<std::pair<int, const char*>> regs; float width; };
    const std::vector<In> ins {
        { "strings.violins.ensemble", "Violin Section", { { "sustain", "Violins" }, { "tremolo", "Violin Tremolo" }, { "pizzicato", "Violin Pizzicato" }, { "spiccato", "Violin Spiccato" } },
          { { 1, "" }, { 0, "Sul G" } }, 0.6f },
        { "strings.violas.ensemble", "Viola Section", { { "sustain", "Violas" }, { "tremolo", "Viola Tremolo" }, { "pizzicato", "Viola Pizzicato" }, { "spiccato", "Viola Spiccato" } },
          { { 0, "" }, { 1, "High" } }, 0.8f },
        { "strings.cellos.ensemble", "Cello Section", { { "sustain", "Cellos" }, { "tremolo", "Cello Tremolo" }, { "pizzicato", "Cello Pizzicato" }, { "spiccato", "Cello Spiccato" } },
          { { -1, "" }, { 0, "Tenor" } }, 0.8f },
        { "strings.violin.solo", "Solo Violin", { { "sustain", "Solo Violin" }, { "tremolo", "Solo Violin Tremolo" }, { "pizzicato", "Solo Violin Pizz" }, { "spiccato", "Solo Violin Spiccato" } },
          { { 0, "" }, { 1, "High" } }, 0.8f },
        { "strings.contrabass", "Contrabass", { { "sustain", "Double Basses" }, { "vibrato", "Double Bass Vibrato" }, { "tremolo", "Double Bass Tremolo" },
                                                { "pizzicato", "Double Bass Pizzicato" }, { "spiccato", "Double Bass Spiccato" } },
          { { -1, "" }, { 0, "High" } }, 0.8f },
    };
    for (auto& in : ins)
        for (auto& [r, rn] : in.regs)
            for (auto& [art, noun] : in.arts)
            {
                const juce::String a (art);
                const int kind = a == "sustain" || a == "vibrato" ? kSus : a == "tremolo" ? kTrem : kShort;
                const juce::String n = juce::String (rn).isEmpty() ? juce::String (noun) : juce::String (rn) + " " + noun;
                KV kv { { "s.width", in.width }, { "s.velCutoff", 0.3f } };
                if (kind != kShort) kv.push_back ({ "s.release", 0.45f });
                b.push_back ({ n, vault::catStrings, fStr, kind, { S (in.id, art, in.label, r, kv) } });
            }

    // ensembles: sections stacked across octaves (violins at pitch, violas unison, cellos -1, basses -2)
    auto V = [] (const char* art, int o, float l, KV kv = {}) { return lvl (S ("strings.violins.ensemble", art, "Violin Section", o, kv), l, -0.35f); };
    auto A = [] (const char* art, int o, float l, KV kv = {}) { return lvl (S ("strings.violas.ensemble", art, "Viola Section", o, kv), l, 0.15f); };
    auto C = [] (const char* art, int o, float l, KV kv = {}) { return lvl (S ("strings.cellos.ensemble", art, "Cello Section", o, kv), l, 0.35f); };
    auto B = [] (const char* art, int o, float l, KV kv = {}) { return lvl (S ("strings.contrabass", art, "Contrabass", o, kv), l, 0.2f); };
    auto SV = [] (const char* art, int o, float l, KV kv = {}) { return lvl (S ("strings.violin.solo", art, "Solo Violin", o, kv), l, 0.0f); };
    const KV sus { { "s.release", 0.5f }, { "s.width", 0.6f }, { "s.velCutoff", 0.3f } };
    const KV sh { { "s.width", 0.6f }, { "s.velCutoff", 0.3f } };
    auto add = [&] (const char* noun, int kind, std::vector<VaultPart> parts) { b.push_back ({ noun, vault::catStrings, fStr, kind, std::move (parts) }); };
    add ("Full Strings", kSus, { A ("sustain", 0, 0.75f, sus), V ("sustain", 0, 0.7f, sus), C ("sustain", -1, 0.6f, sus), B ("sustain", -2, 0.4f, sus) });
    add ("High Strings", kSus, { V ("sustain", 1, 0.8f, sus), A ("sustain", 0, 0.5f, sus), C ("sustain", -1, 0.4f, sus) });
    add ("Chamber Strings", kSus, { SV ("sustain", 0, 0.8f, sus), A ("sustain", 0, 0.45f, sus), C ("sustain", -1, 0.4f, sus) });
    add ("Cinematic Low Strings", kSus, { lvl (C ("sustain", -1, 0.8f, sus), 0.8f, -0.4f), lvl (B ("sustain", -2, 0.9f, sus), 2.0f, 0.4f) });
    add ("Low String Ostinato", kShort, { lvl (C ("spiccato", -1, 0.8f, sh), 0.8f, -0.4f), lvl (B ("spiccato", -2, 0.95f, sh), 0.95f, 0.4f) });
    add ("Violins and Cellos", kSus, { V ("sustain", 1, 0.8f, sus), C ("sustain", 0, 0.55f, sus) });
    add ("Violin Viola Unison", kSus, { A ("sustain", 0, 0.75f, sus), V ("sustain", 0, 0.7f, sus) });
    add ("Pizzicato Ensemble", kShort, { V ("pizzicato", 0, 0.75f, sh), A ("pizzicato", 0, 0.6f, sh), C ("pizzicato", -1, 0.6f, sh), B ("pizzicato", -2, 0.45f, sh) });
    add ("Spiccato Ensemble", kShort, { V ("spiccato", 0, 0.75f, sh), A ("spiccato", 0, 0.6f, sh), C ("spiccato", -1, 0.6f, sh), B ("spiccato", -2, 0.4f, sh) });
    add ("Tremolo Ensemble", kTrem, { V ("tremolo", 0, 0.75f, sus), A ("tremolo", 0, 0.6f, sus), C ("tremolo", -1, 0.6f, sus), B ("tremolo", -2, 0.4f, sus) });
    add ("Soloist and Section", kSus, { SV ("sustain", 1, 0.75f, sus), lvl (V ("sustain", 1, 0.75f, sus), 0.75f, -0.6f), lvl (V ("sustain", 0, 0.5f, sus), 0.5f, 0.6f) });
    add ("Cello Bass Unison", kSus, { lvl (C ("sustain", -1, 0.8f, sus), 0.75f, -0.45f), lvl (B ("sustain", -1, 0.9f, sus), 0.9f, 0.45f) });
    add ("Warm Tenor Strings", kSus, { A ("sustain", 0, 0.8f, sus), C ("sustain", 0, 0.7f, sus) });
    add ("Open Fifths Strings", kSus, { C ("sustain", -1, 0.8f, sus), semi (lvl (A ("sustain", -1, 0.8f, sus), 0.8f, -0.4f), 7) });
    add ("Divisi Violins", kSus, { V ("sustain", 1, 0.8f, sus), V ("sustain", 0, 0.6f, sus) });
    add ("Trap Pizzicato", kShort, { V ("pizzicato", 0, 0.8f, sh), C ("pizzicato", -1, 0.55f, sh) });
    add ("Octave Bow Strokes", kShort, { V ("spiccato", 1, 0.8f, sh), C ("spiccato", 0, 0.6f, sh), B ("spiccato", -1, 0.45f, sh) });
    add ("Shimmer Tremolo", kTrem, { V ("tremolo", 1, 0.8f, sus), A ("tremolo", 0, 0.5f, sus) });
    add ("Hybrid Strings", kSus, { V ("sustain", 0, 0.8f, sus), C ("sustain", -1, 0.5f, sus), lvl (synthOrch ("String Machine", 0.5f, 0), 0.35f) });
    add ("String Machine", kSus, { synthOrch ("String Machine", 0.5f, 0, { { "revMix", 0.2f }, { "revType", 0 }, { "chorus", 0.35f }, { "width", 0.7f } }) });
    add ("Low String Machine", kSus, { synthOrch ("String Machine", 0.45f, -1, { { "revMix", 0.18f }, { "revType", 0 }, { "chorus", 0.3f }, { "cutoff", 1200 } }) });
    add ("Pizz and Tremolo", kTrem, { V ("pizzicato", 0, 0.8f, sh), V ("tremolo", 0, 0.45f, sus) });
    add ("Solo Violin Duet", kSus, { lvl (SV ("sustain", 0, 0.75f, sus), 0.75f, -0.4f), semi (lvl (SV ("sustain", 0, 0.55f, sus), 0.55f, 0.4f), 4) });
}

void addBrass (std::vector<Base>& b)
{
    struct In { const char* id; const char* label; std::vector<std::pair<const char*, const char*>> arts; std::vector<std::pair<int, const char*>> regs; float width; };
    const std::vector<In> ins {
        { "brass.trumpet", "Trumpet", { { "sustain", "Trumpet" }, { "vibrato", "Vibrato Trumpet" }, { "staccato", "Trumpet Staccato" }, { "straight mute", "Straight Mute Trumpet" },
                                        { "harmon mute", "Harmon Trumpet" } },
          { { 0, "" }, { 1, "High" } }, 0.7f },
        { "brass.horn", "French Horn", { { "sustain", "French Horn" }, { "staccato", "Horn Staccato" }, { "muted", "Stopped Horn" } }, { { 0, "" }, { -1, "Low" } }, 0.7f },
        { "brass.trombone", "Tenor Trombone", { { "sustain", "Trombone" }, { "vibrato", "Vibrato Trombone" }, { "staccato", "Trombone Staccato" } }, { { 0, "" }, { -1, "Low" } }, 0.7f },
        { "brass.trombone.vintage", "Vintage Trombone", { { "sustain", "Vintage Trombone" }, { "vibrato", "Crooning Trombone" }, { "short", "Vintage Trombone Short" },
                                                          { "buzz", "Buzzing Trombone" }, { "fall", "Trombone Fall" } },
          { { 0, "" }, { -1, "Low" } }, 0.7f },
        { "brass.tuba", "Tuba", { { "sustain", "Tuba" }, { "staccato", "Tuba Staccato" } }, { { -1, "Concert" }, { 0, "High" } }, 0.4f },
    };
    for (auto& in : ins)
        for (auto& [r, rn] : in.regs)
            for (auto& [art, noun] : in.arts)
            {
                const juce::String a (art);
                const int kind = a == "staccato" || a == "short" || a == "fall" || a == "buzz" ? kShort : kSus;
                const juce::String n = juce::String (rn).isEmpty() ? juce::String (noun) : juce::String (rn) + " " + noun;
                KV kv { { "s.width", in.width }, { "s.velCutoff", 0.35f } };
                if (kind != kShort) kv.push_back ({ "s.release", 0.4f });
                b.push_back ({ n, vault::catBrass, fBrs, kind, { S (in.id, art, in.label, r, kv) } });
            }
    auto add = [&] (const char* noun, int kind, std::vector<VaultPart> parts) { b.push_back ({ noun, vault::catBrass, fBrs, kind, std::move (parts) }); };
    const KV sus { { "s.release", 0.4f }, { "s.velCutoff", 0.35f } };
    const KV sh { { "s.velCutoff", 0.35f } };
    add ("Trumpet Section", kSus, section ("brass.trumpet", "sustain", "Trumpet", 0, 3, sus));
    add ("Trumpet Section Stabs", kShort, section ("brass.trumpet", "staccato", "Trumpet", 0, 3, sh));
    add ("Muted Trumpet Section", kSus, section ("brass.trumpet", "straight mute", "Trumpet", 0, 3, sus));
    add ("Harmon Section", kSus, section ("brass.trumpet", "harmon mute", "Trumpet", 0, 3, sus));
    add ("Horn Section", kSus, section ("brass.horn", "sustain", "French Horn", 0, 3, sus));
    add ("Low Horn Section", kSus, section ("brass.horn", "sustain", "French Horn", -1, 3, sus));
    add ("Stopped Horn Section", kSus, section ("brass.horn", "muted", "French Horn", 0, 3, sus));
    add ("Horn Section Stabs", kShort, section ("brass.horn", "staccato", "French Horn", 0, 3, sh));
    add ("Trombone Section", kSus, section ("brass.trombone", "sustain", "Tenor Trombone", 0, 3, sus));
    add ("Low Trombone Section", kSus, section ("brass.trombone", "sustain", "Tenor Trombone", -1, 3, sus));
    add ("Trombone Section Stabs", kShort, section ("brass.trombone", "staccato", "Tenor Trombone", 0, 3, sh));
    add ("Low Trombone Stabs", kShort, section ("brass.trombone", "staccato", "Tenor Trombone", -1, 3, sh));
    add ("Buzzing Trombones", kShort, section ("brass.trombone.vintage", "buzz", "Vintage Trombone", 0, 2, sh));
    add ("Trombone Section Falls", kShort, section ("brass.trombone.vintage", "fall", "Vintage Trombone", 0, 3, sh));
    add ("Vibrato Trombone Section", kSus, section ("brass.trombone", "vibrato", "Tenor Trombone", -1, 3, sus));
    add ("Tuba Pair", kSus, section ("brass.tuba", "sustain", "Tuba", -1, 2, sus));

    auto Tp = [] (const char* art, int o, float l, float pan = -0.4f) { return lvl (S ("brass.trumpet", art, "Trumpet", o, { { "s.width", 0.6f }, { "s.release", 0.4f }, { "s.velCutoff", 0.35f } }), l, pan); };
    auto Hn = [] (const char* art, int o, float l, float pan = -0.15f) { return lvl (S ("brass.horn", art, "French Horn", o, { { "s.width", 0.6f }, { "s.release", 0.4f }, { "s.velCutoff", 0.35f } }), l, pan); };
    auto Tb = [] (const char* art, int o, float l, float pan = 0.3f) { return lvl (S ("brass.trombone", art, "Tenor Trombone", o, { { "s.width", 0.6f }, { "s.release", 0.4f }, { "s.velCutoff", 0.35f } }), l, pan); };
    auto Tu = [] (const char* art, int o, float l) { return lvl (S ("brass.tuba", art, "Tuba", o, { { "s.width", 0.3f }, { "s.release", 0.4f }, { "s.velCutoff", 0.35f } }), l, 0.2f); };
    add ("Full Brass", kSus, { Tp ("sustain", 0, 0.75f), Hn ("sustain", 0, 0.65f), Tb ("sustain", -1, 0.6f), Tu ("sustain", -2, 0.45f) });
    add ("Low Brass Choir", kSus, { Tb ("sustain", -1, 0.75f), Hn ("sustain", -1, 0.65f), Tu ("sustain", -2, 0.5f) });
    add ("Brass Stab Ensemble", kShort, { Tp ("staccato", 0, 0.75f), Hn ("staccato", 0, 0.6f), Tb ("staccato", -1, 0.6f) });
    add ("Epic Horns and Trombones", kSus, { Hn ("sustain", 0, 0.75f, -0.3f), Tb ("sustain", 0, 0.6f), Hn ("sustain", -1, 0.5f, 0.1f) });
    add ("Trumpet and Horn", kSus, { Tp ("sustain", 0, 0.75f), Hn ("sustain", 0, 0.65f, 0.3f) });
    add ("Hybrid Trap Brass", kShort, { Tp ("staccato", 0, 0.75f), Tb ("staccato", -1, 0.55f), lvl (synthOrch ("Synth Brass", 0.0f, 0, { { "attack", 0.01f }, { "decay", 0.35f }, { "sustain", 0.1f }, { "release", 0.2f } }), 0.3f) });
    add ("Muted Brass Choir", kSus, { Tp ("straight mute", 0, 0.75f), Hn ("muted", 0, 0.6f), Tb ("sustain", -1, 0.4f, 0.3f) });
    add ("Fanfare Brass", kSus, { Tp ("sustain", 0, 0.75f, -0.4f), semi (Tp ("sustain", 0, 0.55f, 0.4f), -5), Tb ("sustain", -1, 0.5f) });
    add ("Synth Brass", kSus, { synthOrch ("Synth Brass", 0.0f, 0, { { "attack", 0.02f }, { "unison", 2 }, { "fenv", 0.45f }, { "fdecay", 0.35f }, { "fsustain", 0.3f }, { "cutoff", 1000 }, { "revMix", 0.15f }, { "revType", 0 }, { "width", 0.6f } }) });
    add ("Synth Brass Stab", kShort, { synthOrch ("Synth Brass", 0.0f, 0, { { "attack", 0.005f }, { "decay", 0.3f }, { "sustain", 0.0f }, { "release", 0.15f }, { "cutoff", 1300 },
                                                                         { "revMix", 0.1f }, { "revType", 1 } }) });
    add ("Hybrid Horns", kSus, { Hn ("sustain", 0, 0.8f), lvl (synthOrch ("Synth Brass", 0.05f, 0, { { "cutoff", 1000 } }), 0.3f) });
    add ("Horns in Octaves", kSus, { Hn ("sustain", 0, 0.75f), Hn ("sustain", -1, 0.6f, 0.25f) });
    add ("Trombone and Tuba", kSus, { Tb ("sustain", -1, 0.75f), Tu ("sustain", -2, 0.55f) });
    add ("Low Brass Stabs", kShort, { Tb ("staccato", -1, 0.75f), Tu ("staccato", -2, 0.6f), Hn ("staccato", -1, 0.45f) });
    add ("Trumpet Trombone Octaves", kSus, { Tp ("sustain", 0, 0.75f), Tb ("sustain", -1, 0.6f) });
    auto Sx = [] (const char* art, int o, float l, float pan) { return lvl (S ("winds.sax.tenor", art, "Tenor Sax", o, { { "s.width", 0.6f }, { "s.release", 0.35f }, { "s.velCutoff", 0.35f } }), l, pan); };
    add ("Soul Horn Section", kShort, { Tp ("staccato", 0, 0.75f, -0.4f), Sx ("staccato", 0, 0.6f, 0.1f), Tb ("staccato", -1, 0.6f, 0.4f) });
    add ("Pop Horn Section", kSus, { Tp ("sustain", 0, 0.75f, -0.4f), Sx ("sustain", 0, 0.6f, 0.1f), Tb ("sustain", -1, 0.55f, 0.4f) });
    add ("Sax and Trombone Unison", kSus, { Tb ("sustain", 0, 0.75f, -0.2f), Sx ("sustain", 0, 0.6f, 0.2f) });
}

void addOrchestral (std::vector<Base>& b)
{
    auto add = [&] (const char* noun, int kind, std::vector<VaultPart> parts) { b.push_back ({ noun, vault::catOrchestral, fOrc, kind, std::move (parts) }); };
    const KV sus { { "s.release", 0.45f }, { "s.width", 0.6f }, { "s.velCutoff", 0.3f } };
    const KV sh { { "s.width", 0.6f }, { "s.velCutoff", 0.3f } };
    auto X = [&] (const char* id, const char* art, const char* label, int o, float l, float pan, const KV& kv) { return lvl (S (id, art, label, o, kv), l, pan); };
    // timpani and harp
    add ("Timpani", kRing, { S ("perc.timpani", "hit", "Timpani", -1, sh) });
    add ("Soft Timpani", kRing, { S ("perc.timpani", "hit warm", "Timpani", -1, sh) });
    add ("Timpani Roll", kTrem, { S ("perc.timpani", "roll", "Timpani", -1, sus) });
    add ("Low Timpani", kRing, { S ("perc.timpani", "hit", "Timpani", -2, sh) });
    add ("Orchestral Harp", kRing, { S ("pluck.harp.concert", "pluck", "Concert Harp", 0, sh) });
    add ("Low Orchestral Harp", kRing, { S ("pluck.harp.concert", "pluck", "Concert Harp", -1, sh) });
    // woodwind + string doublings
    add ("Flute and Violins", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 1, 0.75f, -0.3f, sus), X ("winds.flute.concert", "sustain", "Concert Flute", 1, 0.6f, 0.2f, sus) });
    add ("Clarinet and Violas", kSus, { X ("strings.violas.ensemble", "sustain", "Viola Section", 0, 0.75f, 0.2f, sus), X ("winds.clarinet", "sustain", "Clarinet", 0, 0.6f, -0.2f, sus) });
    add ("Oboe and Violins", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 1, 0.75f, -0.3f, sus), X ("winds.oboe", "sustain", "Oboe", 1, 0.45f, 0.1f, sus) });
    add ("Bassoon and Cellos", kSus, { X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.75f, 0.3f, sus), X ("winds.bassoon", "sustain", "Bassoon", -1, 0.55f, -0.1f, sus) });
    add ("Woodwind Quartet", kSus, { X ("winds.clarinet", "sustain", "Clarinet", 0, 0.7f, -0.2f, sus), X ("winds.flute.concert", "sustain", "Concert Flute", 0, 0.5f, -0.4f, sus),
                                   X ("winds.oboe", "sustain", "Oboe", 0, 0.35f, 0.2f, sus), X ("winds.bassoon", "sustain", "Bassoon", -1, 0.5f, 0.4f, sus) });
    add ("Pastoral Winds", kSus, { X ("winds.flute.concert", "sustain", "Concert Flute", 0, 0.75f, -0.2f, sus), X ("winds.clarinet", "sustain", "Clarinet", -1, 0.55f, 0.2f, sus) });
    add ("Woodwind Quartet Staccato", kShort, { X ("winds.clarinet", "staccato", "Clarinet", 0, 0.7f, -0.2f, sh), X ("winds.flute.concert", "staccato", "Concert Flute", 0, 0.5f, -0.4f, sh),
                                        X ("winds.bassoon", "staccato", "Bassoon", -1, 0.5f, 0.4f, sh) });
    add ("Winds and Strings", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.7f, -0.3f, sus), X ("winds.clarinet", "sustain", "Clarinet", 0, 0.5f, 0.1f, sus),
                                      X ("winds.flute.concert", "sustain", "Concert Flute", 0, 0.45f, -0.1f, sus), X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.5f, 0.3f, sus) });
    // tutti
    add ("Orchestral Tutti", kSus, { X ("brass.horn", "sustain", "French Horn", 0, 0.75f, -0.1f, sus), X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.6f, -0.45f, sus),
                                     X ("brass.trombone", "sustain", "Tenor Trombone", -1, 0.6f, 0.35f, sus) });
    add ("Grand Tutti", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 1, 0.7f, -0.35f, sus), X ("brass.trumpet", "sustain", "Trumpet", 0, 0.45f, 0.1f, sus),
                                X ("brass.trombone", "sustain", "Tenor Trombone", -1, 0.45f, 0.25f, sus), X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.55f, 0.35f, sus) });
    add ("Tutti Swell", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.7f, -0.35f, { { "s.attack", 0.8f }, { "s.release", 1.0f }, { "s.width", 0.6f } }),
                                X ("strings.violas.ensemble", "sustain", "Viola Section", 0, 0.55f, 0.1f, { { "s.attack", 0.8f }, { "s.release", 1.0f }, { "s.width", 0.6f } }),
                                X ("brass.horn", "sustain", "French Horn", -1, 0.5f, -0.1f, { { "s.attack", 0.9f }, { "s.release", 1.0f }, { "s.width", 0.6f } }),
                                X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.5f, 0.35f, { { "s.attack", 0.8f }, { "s.release", 1.0f }, { "s.width", 0.6f } }) });
    add ("Orchestra Hit", kShort, { X ("strings.violins.ensemble", "spiccato", "Violin Section", 1, 0.75f, -0.3f, sh), X ("brass.trumpet", "staccato", "Trumpet", 0, 0.55f, 0.1f, sh),
                                    X ("brass.trombone", "staccato", "Tenor Trombone", -1, 0.55f, 0.3f, sh), X ("perc.timpani", "hit", "Timpani", -1, 0.55f, 0.0f, sh) });
    add ("Classic Orchestra Stab", kShort, { X ("strings.violins.ensemble", "spiccato", "Violin Section", 0, 0.75f, -0.3f, sh), X ("brass.horn", "staccato", "French Horn", 0, 0.6f, 0.2f, sh),
                                             X ("strings.cellos.ensemble", "spiccato", "Cello Section", -1, 0.6f, 0.3f, sh), X ("brass.tuba", "staccato", "Tuba", -2, 0.45f, 0.1f, sh) });
    add ("Horns and Strings", kSus, { X ("brass.horn", "sustain", "French Horn", 0, 0.95f, -0.3f, sus), X ("strings.violas.ensemble", "sustain", "Viola Section", 0, 0.4f, 0.5f, sus),
                                      X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.5f, 0.35f, sus) });
    add ("Thunder Strings", kTrem, { X ("strings.cellos.ensemble", "tremolo", "Cello Section", -1, 0.75f, 0.3f, sus), X ("strings.contrabass", "tremolo", "Contrabass", -2, 0.9f, -0.3f, sus),
                                     X ("perc.timpani", "roll", "Timpani", -1, 1.2f, 0.0f, sus) });
    add ("War Drum Brass", kShort, { X ("brass.trombone", "staccato", "Tenor Trombone", -1, 0.75f, 0.2f, sh), X ("perc.timpani", "hit", "Timpani", -1, 0.65f, 0.0f, sh),
                                     X ("brass.tuba", "staccato", "Tuba", -2, 0.5f, -0.1f, sh) });
    add ("Harp and Pizzicato", kRing, { X ("pluck.harp.concert", "pluck", "Concert Harp", 0, 0.75f, -0.2f, sh), X ("strings.violins.ensemble", "pizzicato", "Violin Section", 0, 0.85f, 0.5f, sh) });
    add ("Fairy Tale Pizzicato", kShort, { X ("strings.violins.ensemble", "pizzicato", "Violin Section", 1, 0.75f, -0.2f, sh), X ("mallets.glockenspiel", "hit", "Glockenspiel", 2, 0.05f, 0.3f, sh),
                                           X ("pluck.harp.concert", "pluck", "Concert Harp", 0, 0.45f, 0.1f, sh) });
    add ("Strings and Choir", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.7f, -0.3f, sus), X ("voice.choir.classic", "aahs", "Choir", 0, 0.8f, 0.3f, sus),
                                      X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.5f, 0.35f, sus) });
    add ("Harp and Flute", kRing, { X ("pluck.harp.concert", "pluck", "Concert Harp", 0, 0.75f, -0.2f, sh), X ("winds.flute.concert", "sustain", "Concert Flute", 0, 0.4f, 0.25f, sus) });
    add ("Clockwork Winds", kShort, { X ("winds.clarinet", "staccato", "Clarinet", 0, 0.75f, -0.2f, sh), X ("strings.violins.ensemble", "pizzicato", "Violin Section", 0, 0.55f, 0.3f, sh) });
    add ("Mischief Bassoon", kShort, { X ("winds.bassoon", "staccato", "Bassoon", -1, 0.75f, -0.1f, sh), X ("strings.cellos.ensemble", "pizzicato", "Cello Section", -1, 0.6f, 0.3f, sh) });
    add ("Oboe and Harp", kRing, { X ("pluck.harp.concert", "pluck", "Concert Harp", 0, 0.7f, 0.2f, sh), X ("winds.oboe", "sustain", "Oboe", 0, 0.35f, -0.2f, sus) });
    add ("Cathedral Strings and Bells", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.75f, -0.3f, sus), X ("bells.tubular", "hit", "Tubular Bells", 0, 0.3f, 0.25f, sh),
                                                X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.5f, 0.3f, sus) });
    add ("Marimba and Pizzicato", kShort, { X ("strings.violins.ensemble", "pizzicato", "Violin Section", 0, 0.75f, -0.3f, sh), X ("mallets.marimba", "hit", "Marimba", 0, 0.45f, 0.3f, sh) });
    add ("Warm Horn and Bassoon", kSus, { X ("brass.horn", "sustain", "French Horn", 0, 0.75f, -0.2f, sus), X ("winds.bassoon", "sustain", "Bassoon", -1, 0.5f, 0.2f, sus) });
    add ("Epic Low Orchestra", kSus, { X ("brass.horn", "sustain", "French Horn", -1, 0.75f, -0.2f, sus), X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.6f, 0.3f, sus),
                                       X ("brass.trombone", "sustain", "Tenor Trombone", -1, 0.45f, 0.1f, sus), X ("strings.contrabass", "sustain", "Contrabass", -2, 0.45f, 0.0f, sus) });
    add ("Timpani and Contrabass", kRing, { X ("perc.timpani", "hit", "Timpani", -1, 0.75f, 0.0f, sh), X ("strings.contrabass", "pizzicato", "Contrabass", -2, 0.95f, 0.1f, sh) });
    add ("Brass and Strings Swell", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.75f, -0.3f, { { "s.attack", 0.6f }, { "s.release", 0.8f }, { "s.width", 0.6f } }),
                                            X ("brass.trombone", "sustain", "Tenor Trombone", -1, 0.55f, 0.25f, { { "s.attack", 0.7f }, { "s.release", 0.8f }, { "s.width", 0.6f } }) });
    add ("Flute and Clarinet Duet", kSus, { X ("winds.flute.concert", "sustain", "Concert Flute", 0, 0.7f, -0.3f, sus), semi (X ("winds.clarinet", "sustain", "Clarinet", 0, 0.55f, 0.3f, sus), -4) });
    add ("Strings and Timpani Hit", kShort, { X ("strings.violins.ensemble", "spiccato", "Violin Section", 0, 0.7f, -0.3f, sh), X ("perc.timpani", "hit", "Timpani", -1, 0.6f, 0.0f, sh),
                                              X ("strings.cellos.ensemble", "spiccato", "Cello Section", -1, 0.65f, 0.3f, sh) });
    add ("Choir and Brass", kSus, { X ("brass.horn", "sustain", "French Horn", 0, 0.7f, -0.2f, sus), X ("voice.choir.classic", "oohs", "Choir", 0, 0.5f, 0.2f, sus),
                                    X ("brass.trombone", "sustain", "Tenor Trombone", -1, 0.45f, 0.3f, sus) });
    add ("Piccolo and Strings", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 1, 0.75f, -0.2f, sus), X ("winds.flute.concert", "sustain", "Concert Flute", 1, 0.45f, 0.2f, sus) });
    add ("Piano and Strings", kRing, { X ("piano.grand.concert", "sustain", "Concert Grand", 0, 0.8f, 0.0f, { { "s.velCutoff", 0.3f } }),
                                       X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.4f, -0.4f, { { "s.attack", 0.35f }, { "s.release", 0.8f }, { "s.width", 0.6f } }),
                                       X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.35f, 0.4f, { { "s.attack", 0.35f }, { "s.release", 0.8f }, { "s.width", 0.6f } }) });
    add ("Organ and Strings", kSus, { X ("organ.pipe", "soft", "Pipe Organ", 0, 0.7f, 0.0f, { { "s.release", 0.6f } }), X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.55f, -0.35f, sus),
                                      X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.45f, 0.35f, sus) });
    add ("Glass and Strings", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 1, 0.75f, -0.3f, sus), X ("fx.glass", "slow", "Glass Harp", 1, 0.3f, 0.3f, { { "s.release", 0.8f } }),
                                      X ("strings.violas.ensemble", "sustain", "Viola Section", 0, 0.45f, 0.1f, sus) });
    add ("Heavenly Strings", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.7f, -0.3f, sus), X ("voice.choir.classic", "oohs", "Choir", 0, 0.5f, 0.3f, sus) });
    add ("Psaltery and Violins", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 1, 0.75f, -0.3f, sus), X ("pluck.psaltery", "bowed", "Bowed Psaltery", 1, 0.3f, 0.3f, { { "s.release", 0.6f } }) });
    add ("Hybrid Orchestra", kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.7f, -0.3f, sus), X ("brass.horn", "sustain", "French Horn", 0, 0.5f, 0.2f, sus),
                                     lvl (synthOrch ("String Machine", 0.35f, -1, { { "cutoff", 1100 } }), 0.35f) });
}

//==============================================================================
// CINEMATIC LOW STRINGS (owner's FL Studio test report #1, item 17: "dark, powerful, deep"). Hand-voiced presets from the
// real section recordings - viola, cello and contrabass sections stacked in octaves (violas at the key, cellos an octave
// and basses two octaves under it: key C5 sounds C4 / C3 / C2, so chords played around C4..C5 sit in the C2..C4 sweet
// spot), cellos doubled in octaves, open fifths, the contrabass vibrato section under the cellos. Every style changes how
// the sections are played, not just an effect: sustain (glue, long warm hall, slow bow-pressure breathing), slow swells
// (0.6 - 2.4 s bow crescendo), sul tasto / con sordino pads (the bow's attack skipped, low-passed under ~1.5 kHz), marcato
// (accent + decay, a spiccato layer on the stroke, tape power with clean lows, transient), spiccato / pizzicato
// ostinatos, tremolo tension; Orchestral: horns, trombones + tuba, choir, bassoon, timpani or harp with the low sections.
// Soft notes are darker (velocity -> cutoff, as real dynamics), the top is shelved off, the low mids boxed out, lows kept
// mono. They lead their category's list (the first Strings preset is the dark octave stack, not a solo violin) and win
// any near-duplicate pair against the older generated variants. Finished by v3::designCinematic (chain, movement, 4
// named macros). Variants that measured as near-duplicates of an earlier one (a hybrid synth layer or a sub sine under
// the stack, tape colour, a close perspective, a second marcato) were taken out: only what the gate hears as its own.
void addCinematic (std::vector<Base>& b)
{
    // tone 0..1: per-part low-pass from dark (sul tasto) to open; basses darker than the upper sections
    auto cut = [] (float tone, float scale = 1.0f) { return juce::jlimit (400.0f, 9000.0f, scale * 2200.0f * std::pow (2.0f, 2.0f * tone)); };
    struct Env { float attack = 0.08f, decay = 0.0f, sustain = 1.0f, release = 1.2f, start = 0.0f, velSens = 0.3f; bool shortS = false; float cutMul = 1.0f; };
    auto kvOf = [&] (const Env& e, float tone, float scale)
    {
        KV kv { { "s.width", 0.55f }, { "s.velCutoff", 0.4f }, { "s.velSens", e.velSens }, { "s.cutoff", cut (tone, scale * e.cutMul) } };
        if (! e.shortS) { kv.push_back ({ "s.attack", e.attack }); kv.push_back ({ "s.release", e.release }); }
        if (e.decay > 0.0f) { kv.push_back ({ "s.decay", e.decay }); kv.push_back ({ "s.sustain", e.sustain }); }
        if (e.start > 0.0f) kv.push_back ({ "s.start", e.start });
        return kv;
    };
    struct P { const char* id; const char* label; const char* art; int o; float level, pan; int semi = 0; KV extra = {}; Env* env = nullptr; float scale = 1.0f; };
    // section shorthands
    auto Vn = [] (const char* art, int o, float l, float pan = -0.35f) { return P { "strings.violins.ensemble", "Violin Section", art, o, l, pan, 0, {}, nullptr, 0.8f }; };
    auto Va = [] (const char* art, int o, float l, float pan = -0.25f) { return P { "strings.violas.ensemble", "Viola Section", art, o, l, pan }; };
    auto Vc = [] (const char* art, int o, float l, float pan = 0.25f) { return P { "strings.cellos.ensemble", "Cello Section", art, o, l, pan }; };
    auto Cb = [] (const char* art, int o, float l, float pan = 0.08f) { return P { "strings.contrabass", "Contrabass", art, o, l, pan, 0, {}, nullptr, 0.75f }; };
    auto X = [] (const char* id, const char* label, const char* art, int o, float l, float pan) { return P { id, label, art, o, l, pan }; };
    auto fifth = [] (P p) { p.semi = 7; return p; };
    auto with = [] (P p, KV kv) { for (auto& x : kv) p.extra.push_back (x); return p; };

    auto add = [&] (const char* noun, int cat, int kind, int look, float tone, float hall, Env env, std::vector<P> ps)
    {
        Base x { noun, cat, cat == vault::catStrings ? fStr : fOrc, kind, {} };
        x.look = look; x.tone = tone; x.hall = hall; x.sub = "cinematic";
        for (auto& q : ps)
        {
            const Env& e = q.env != nullptr ? *q.env : env;
            auto part = lvl (S (q.id, q.art, q.label, q.o, kvOf (e, tone, q.scale)), q.level, q.pan);
            part.semi = q.semi;
            for (auto& [k, v] : q.extra) part.set (k, v);
            x.parts.push_back (part);
        }
        b.push_back (std::move (x));
    };
    const int S_ = vault::catStrings, O_ = vault::catOrchestral;
    Env sus;                                                        // bowed sustain: a soft bow start, long release
    Env swell { 1.8f, 0, 1, 2.2f };                                 // bow crescendo
    Env slowSwell { 2.4f, 0, 1, 2.6f };
    Env cello14 { 1.4f, 0, 1, 2.0f };
    Env tasto { 0.45f, 0, 1, 1.8f, 120.0f, 0.45f, false, 0.45f };                 // sul tasto: the bow's bite skipped, softer touch
    Env sordino { 0.2f, 0, 1, 1.5f, 40.0f, 0.4f, false, 0.7f };
    Env marc { 0.0f, 0.45f, 0.3f, 0.6f };                           // marcato: accent, then the bow eases off
    Env marcHard { 0.0f, 0.35f, 0.22f, 0.55f };
    Env sfz { 0.0f, 0.35f, 0.0f, 0.6f };                            // sforzando stroke
    Env shortE; shortE.shortS = true;
    Env trem { 0.2f, 0, 1, 1.0f };
    Env tremSwell { 1.0f, 0, 1, 1.6f };
    Env breath { 0.9f, 0, 1, 1.8f };
    Env under { 1.0f, 0, 1, 1.6f };                                // the bowed bed under a pizzicato

    // -------- Strings: sustains (the octave stack first: the category's default sound)
    add ("Cinematic Low Octaves", S_, kSus, lEpic, 0.45f, 0.65f, sus, { Va ("sustain", 0, 0.55f), Vc ("sustain", -1, 0.85f), Cb ("sustain", -2, 0.9f) });
    add ("Dark Cello Choir", S_, kSus, lEpic, 0.35f, 0.6f, sus, { with (Vc ("sustain", 0, 0.7f, -0.35f), { { "s.vibrato", 9.0f }, { "s.vibRate", 4.6f } }),
                                                                 with (Vc ("sustain", -1, 0.85f, 0.35f), { { "s.tune", -6.0f }, { "s.start", 20.0f } }) });
    add ("Requiem Strings", S_, kSus, lEpic, 0.4f, 0.75f, sus, { Vn ("sustain", 0, 0.3f), Va ("sustain", 0, 0.6f, 0.1f), Vc ("sustain", -1, 0.8f), Cb ("sustain", -2, 0.7f) });
    add ("Abyss Low Strings", S_, kSus, lEpic, 0.2f, 0.7f, sus, { Vc ("sustain", -1, 0.7f, -0.25f), Cb ("sustain", -1, 0.85f, 0.2f), Cb ("sustain", -2, 0.8f, 0.0f) });
    add ("Power Fifths Low", S_, kSus, lEpic, 0.55f, 0.6f, sus, { Vc ("sustain", -1, 0.85f), fifth (Va ("sustain", -1, 0.55f)), Cb ("vibrato", -2, 0.8f) });
    add ("Tenor Cello Lament", S_, kSus, lEpic, 0.5f, 0.55f, sus, { with (Vc ("sustain", 0, 0.85f, 0.1f), { { "s.vibrato", 14.0f }, { "s.vibRate", 4.8f } }),
                                                                   Va ("sustain", -1, 0.45f), Cb ("sustain", -2, 0.45f) });
    add ("Shimmering Low Strings", S_, kSus, lEpic, 0.6f, 0.65f, sus, { Va ("tremolo", 0, 1.0f), Vc ("sustain", -1, 0.7f), Cb ("sustain", -2, 0.65f) });
    add ("Expressive Bass Section", S_, kSus, lEpic, 0.45f, 0.55f, sus, { Cb ("vibrato", -1, 0.95f, 0.1f), Vc ("sustain", -1, 0.4f, 0.3f) });
    add ("Contrabass Choir", S_, kSus, lEpic, 0.3f, 0.55f, sus, { Cb ("sustain", -1, 0.85f, -0.15f), with (Cb ("vibrato", -2, 0.7f, 0.15f), { { "s.tune", -5.0f } }) });
    // swells
    add ("Rising Dark Octaves", S_, kSus, lSwell, 0.4f, 0.75f, swell, { Va ("sustain", 0, 0.55f), Vc ("sustain", -1, 0.85f), Cb ("sustain", -2, 0.9f) });
    add ("Slow Cello Swell", S_, kSus, lSwell, 0.45f, 0.7f, cello14, { Vc ("sustain", 0, 0.65f, -0.3f), Vc ("sustain", -1, 0.85f, 0.3f) });
    add ("Abyss Swell", S_, kSus, lSwell, 0.25f, 0.85f, slowSwell, { Vc ("sustain", -1, 0.8f), Cb ("sustain", -2, 0.95f) });
    add ("Shadow Ensemble", S_, kSus, lSwell, 0.25f, 0.7f, breath, { Vn ("tremolo", 0, 0.28f), Va ("sustain", 0, 0.55f, 0.1f), Vc ("sustain", -1, 0.8f), Cb ("sustain", -2, 0.65f) });
    // sul tasto / con sordino pads
    add ("Sul Tasto Cellos", S_, kSus, lTasto, 0.15f, 0.75f, tasto, { Vc ("sustain", 0, 0.6f, -0.3f), Vc ("sustain", -1, 0.85f, 0.3f) });
    add ("Velvet Abyss Pad", S_, kSus, lTasto, 0.0f, 0.9f, tasto, { Vc ("sustain", -1, 0.8f), Cb ("sustain", -2, 0.9f) });
    add ("Con Sordino Lows", S_, kSus, lTasto, 0.2f, 0.4f, sordino, { with (Va ("sustain", 0, 0.7f), { { "s.reso", 0.5f } }), with (Vc ("sustain", -1, 0.8f), { { "s.reso", 0.5f } }),
                                                                      Cb ("tremolo", -2, 0.55f) });
    add ("Hushed Viola Pad", S_, kSus, lTasto, 0.2f, 0.8f, tasto, { Va ("sustain", 0, 0.75f), Vc ("sustain", -1, 0.6f) });
    // marcato power
    { Env sp = shortE;   // the marcato stroke: a spiccato layer on the accent of the bowed notes
      add ("Marcato Low Octaves", S_, kSus, lMarcato, 0.55f, 0.55f, marc, { Vc ("sustain", -1, 0.8f), P { "strings.cellos.ensemble", "Cello Section", "spiccato", -1, 0.75f, -0.25f, 0, {}, &sp },
                                                                           Cb ("sustain", -2, 0.85f) }); }
    add ("Heavy Bow Strikes", S_, kSus, lMarcato, 0.5f, 0.6f, sfz, { Vc ("sustain", -1, 0.85f), Cb ("sustain", -2, 0.9f) });
    // spiccato / pizzicato ostinatos
    add ("Low Spiccato Octaves", S_, kShort, lSpicc, 0.55f, 0.45f, shortE, { Va ("spiccato", 0, 0.5f), Vc ("spiccato", -1, 0.85f), Cb ("spiccato", -2, 0.85f) });
    add ("Cello Bass Spiccato", S_, kShort, lSpicc, 0.5f, 0.4f, shortE, { Vc ("spiccato", -1, 0.85f), Cb ("spiccato", -2, 0.9f) });
    add ("Chase Spiccato Cellos", S_, kShort, lSpicc, 0.6f, 0.3f, shortE, { Vc ("spiccato", 0, 0.7f, -0.3f), Vc ("spiccato", -1, 0.85f, 0.3f) });
    add ("Low Pizzicato Octaves", S_, kShort, lPizz, 0.5f, 0.5f, shortE, { Va ("pizzicato", 0, 0.45f), Vc ("pizzicato", -1, 0.85f), Cb ("pizzicato", -2, 0.9f) });
    add ("Deep Pizzicato Basses", S_, kShort, lPizz, 0.4f, 0.45f, shortE, { Vc ("pizzicato", -1, 0.55f, -0.3f), Cb ("pizzicato", -1, 0.9f, 0.2f), Cb ("pizzicato", -2, 0.6f) });
    { Env pz = shortE;
      add ("Pizz and Bowed Lows", S_, kSus, lPizz, 0.5f, 0.55f, under, { P { "strings.cellos.ensemble", "Cello Section", "pizzicato", -1, 1.2f, 0.25f, 0, {}, &pz },
                                                                           Vc ("sustain", -1, 0.3f, -0.3f), Cb ("sustain", -2, 0.35f) }); }
    // tremolo tension
    add ("Low Tremolo Tension", S_, kTrem, lTrem, 0.45f, 0.6f, trem, { Va ("tremolo", 0, 0.5f), Vc ("tremolo", -1, 0.85f), Cb ("tremolo", -2, 0.85f) });
    add ("Dark Tremolo Cellos", S_, kTrem, lTrem, 0.15f, 0.65f, Env { 0.4f, 0, 1, 1.4f, 0, 0.3f, false, 0.7f }, { Vc ("tremolo", 0, 0.65f, -0.3f), Vc ("tremolo", -1, 0.85f, 0.3f) });
    add ("Tremolo Abyss", S_, kTrem, lTrem, 0.15f, 0.8f, tremSwell, { Vc ("tremolo", -1, 0.8f), Cb ("tremolo", -2, 0.9f) });
    // spaces and colours of the octave stack (each also played differently)
    add ("Cathedral Low Strings", S_, kSus, lSwell, 0.3f, 1.0f, Env { 0.6f, 0, 1, 2.4f }, { Va ("sustain", 0, 0.5f), Vc ("tremolo", -1, 0.6f), Cb ("vibrato", -2, 0.85f) });
    add ("Cello Tremolo Swell", S_, kTrem, lTrem, 0.35f, 0.75f, Env { 1.6f, 0, 1, 2.0f }, { Vc ("tremolo", 0, 0.65f, -0.3f), Vc ("tremolo", -1, 0.85f, 0.3f) });
    add ("Low Strings Sforzando", S_, kSus, lMarcato, 0.6f, 0.6f, Env { 0.0f, 0.15f, 0.07f, 0.9f }, { Va ("sustain", 0, 0.55f), Vc ("sustain", -1, 0.85f), Cb ("vibrato", -2, 0.8f) });
    // two playing styles in one stroke (the layers have their own envelopes): accent then crescendo, tremolo settling
    // into a sustain, a spiccato stroke over a bowed bass bed; fifths as tremolo, spiccato and sul tasto
    {
        Env stab { 0.0f, 0.3f, 0.0f, 0.5f }, grow { 2.0f, 0, 1, 2.2f }, fade { 0.05f, 1.6f, 0.15f, 1.2f }, rise { 1.2f, 0, 1, 1.8f }, bed { 0.3f, 0, 1, 1.4f };
        Env sp = shortE;
        add ("Sforzando into Tremolo", S_, kTrem, lTrem, 0.45f, 0.7f, grow, { P { "strings.cellos.ensemble", "Cello Section", "sustain", -1, 1.0f, -0.25f, 0, {}, &stab },
                                                                             Vc ("tremolo", -1, 0.75f, 0.3f), Cb ("tremolo", -2, 0.85f) });
        add ("Tremolo into Sustain", S_, kSus, lSwell, 0.4f, 0.7f, rise, { P { "strings.cellos.ensemble", "Cello Section", "tremolo", -1, 0.85f, -0.3f, 0, {}, &fade },
                                                                           Vc ("sustain", -1, 0.7f, 0.3f), Cb ("sustain", -2, 0.75f) });
        add ("Spiccato over Bowed Basses", S_, kSus, lMarcato, 0.5f, 0.55f, bed, { P { "strings.cellos.ensemble", "Cello Section", "spiccato", -1, 0.95f, -0.25f, 0, {}, &sp },
                                                                                    Cb ("sustain", -2, 0.8f, 0.15f) });
        add ("Dark Fifths Tremolo", S_, kTrem, lTrem, 0.3f, 0.7f, tremSwell, { Vc ("tremolo", -1, 0.85f), fifth (Va ("tremolo", -1, 0.55f)), Cb ("tremolo", -2, 0.75f) });
        add ("Sul Tasto Fifths", S_, kSus, lTasto, 0.1f, 0.85f, tasto, { Vc ("sustain", -1, 0.85f), fifth (Va ("sustain", -1, 0.6f)) });
    }

    // -------- Orchestral: the string-heavy low orchestra
    add ("Horns over Low Strings", O_, kSus, lEpic, 0.45f, 0.7f, sus, { X ("brass.horn", "French Horn", "sustain", 0, 0.75f, -0.2f), Vc ("sustain", -1, 0.8f), Cb ("sustain", -2, 0.7f) });
    add ("Timpani Roll and Low Strings", O_, kTrem, lTrem, 0.45f, 0.65f, sus, { X ("perc.timpani", "Timpani", "roll", -1, 1.4f, 0.0f), Vc ("sustain", -1, 0.6f), Cb ("sustain", -2, 0.6f) });
    add ("Choir over Low Strings", O_, kSus, lSwell, 0.35f, 0.8f, swell, { X ("voice.choir.classic", "Choir", "oohs", 0, 0.9f, 0.2f), Vc ("sustain", -1, 0.75f), Cb ("sustain", -2, 0.6f) });
    add ("Bassoon and Low Strings", O_, kSus, lEpic, 0.45f, 0.6f, sus, { Vc ("sustain", -1, 0.8f), X ("winds.bassoon", "Bassoon", "sustain", -1, 0.6f, -0.15f), Cb ("sustain", -2, 0.65f) });
    add ("Spiccato and Timpani", O_, kShort, lSpicc, 0.5f, 0.5f, shortE, { Vc ("spiccato", -1, 0.85f), Cb ("spiccato", -2, 0.85f), X ("perc.timpani", "Timpani", "hit", -1, 0.9f, 0.0f) });
    add ("Dark Pizzicato and Harp", O_, kShort, lPizz, 0.45f, 0.55f, shortE, { Vc ("pizzicato", -1, 0.85f), X ("pluck.harp.concert", "Concert Harp", "pluck", -1, 0.5f, -0.2f), Cb ("pizzicato", -2, 0.7f) });
    { Env st = shortE;   // short brass strokes over the bowed low sections
      add ("Brass Stabs over Low Strings", O_, kSus, lMarcato, 0.45f, 0.6f, Env { 0.15f, 0, 1, 1.4f }, { P { "brass.trombone", "Tenor Trombone", "staccato", 0, 0.8f, -0.25f, 0, {}, &st },
                                                                                                     P { "brass.tuba", "Tuba", "staccato", -1, 0.6f, 0.2f, 0, {}, &st }, Vc ("sustain", -1, 0.7f), Cb ("sustain", -2, 0.7f) }); }
    add ("Timpani and Pizzicato Basses", O_, kShort, lPizz, 0.4f, 0.55f, shortE, { X ("perc.timpani", "Timpani", "hit", -1, 0.8f, 0.0f), Vc ("pizzicato", -1, 0.7f, -0.25f), Cb ("pizzicato", -2, 0.85f, 0.2f) });
    add ("Choir and Cello Tremolo", O_, kTrem, lTrem, 0.3f, 0.8f, tremSwell, { X ("voice.choir.classic", "Choir", "aahs", 0, 0.8f, -0.2f), Vc ("tremolo", -1, 0.8f, 0.25f), Cb ("tremolo", -2, 0.7f) });
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
            const auto n = pre.isEmpty() ? noun : pre + " " + noun;
            if (n.length() <= 30 && used.insert (n.toLowerCase()).second) return n;
        }
        return {};
    }
};

std::vector<VaultPart> applyTreat (const Base& b, const Treat& t)
{
    auto parts = b.parts;
    for (auto& [k, v] : t.kv)
    {
        if (k.startsWith ("s."))
        {
            for (auto& p : parts)
            {
                if (p.kind == VaultPartKind::sample)
                {
                    if (k == "s.cutoff") p.set (k, std::min (v, p.get (k, 20000.0f)));
                    else if (k == "s.width") p.set (k, std::min (v, p.get (k, 1.0f)));
                    else if (k == "s.release") p.set (k, std::max (v, p.get (k, 0.0f)));
                    else p.set (k, v);
                }
                else if (p.kind == VaultPartKind::synth)
                {
                    const auto sk = k.substring (2);
                    if (sk == "attack" || sk == "decay" || sk == "sustain" || sk == "release") p.set (sk, sk == "release" ? std::max (v, p.get (sk, 0.0f)) : v);
                    else if (sk == "cutoff") p.set ("cutoff", std::min (v, p.get ("cutoff", 20000.0f)));
                }
            }
        }
        else if (k == "eqHigh" || k == "width") parts[0].set (k, std::min (v, parts[0].get (k, k == "width" ? 0.5f : 0.0f)));
        else parts[0].set (k, v);
    }
    if (t.structural != 0 && parts.size() < (size_t) vault::maxParts)
    {
        auto c = parts[0];
        if (t.structural == 1)
        {
            parts[0].pan = juce::jlimit (-1.0f, 1.0f, parts[0].pan - 0.4f);
            c.pan = juce::jlimit (-1.0f, 1.0f, c.pan + 0.4f);
            c.level *= 0.85f;
            if (c.kind == VaultPartKind::sample) { c.set ("s.tune", c.get ("s.tune", 0.0f) + 9.0f); c.set ("s.start", c.get ("s.start", 0.0f) + 18.0f); }
            else c.set ("fine", c.get ("fine", 0.0f) + 9.0f);
        }
        else { c.octave -= 1; c.level *= 0.6f; }
        // part-0-only FX keys stay on part 0
        for (auto k : { "revMix", "revSize", "revType", "eqLow", "eqHigh", "width", "delayMix", "delayTime", "delayFb", "delayMode", "drive", "driveType",
                        "crush", "wow", "reverse", "phaser", "chorus" })
            c.values.erase (std::remove_if (c.values.begin(), c.values.end(), [&] (auto& kv) { return kv.first == k; }), c.values.end());
        parts.push_back (c);
    }
    return parts;
}

//==============================================================================
// C5 comfort fit (measured like the gate)
struct Meas { float h1 = 0, s25 = 0, s6k = 0, mono = 0, loud = -120; };
Meas measure (const VaultPreset& p)
{
    const auto b = recipegate::render (p, vault::homeNote, 0.8f, 1.5, 1.6);
    Meas m;
    const auto a = vaultqc::analyse (b, recipegate::sr, 1.5);
    m.mono = a.monoLossDb;
    m.loud = a.loud;
    const auto sp = recipegate::spectrum (b, 0.05);
    const double tot = recipegate::bandPower (sp, 20, 20000);
    m.s25 = (float) (recipegate::bandPower (sp, 2000, 5000) / std::max (1e-20, tot));
    m.s6k = (float) (recipegate::bandPower (sp, 6000, 20000) / std::max (1e-20, tot));
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    const float h1 = recipegate::peakDb (sp, f0 * 0.97, f0 * 1.03);
    float strongest = h1;
    for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, recipegate::peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
    m.h1 = h1 - strongest;
    return m;
}
float cutoffOf (const VaultPart& p) { return p.kind == VaultPartKind::sample ? p.get ("s.cutoff", 20000.0f) : p.kind == VaultPartKind::synth ? p.get ("cutoff", 20000.0f) : 20000.0f; }
// scale every filtered part's cutoff (floor relative to the part's own sounding pitch at C5)
bool darken (VaultPreset& p, float factor, float floorHarm)
{
    bool any = false;
    for (auto& pt : p.parts)
    {
        if (pt.kind == VaultPartKind::model || pt.label == "Warm Sine") continue;
        const float f0 = 440.0f * std::pow (2.0f, (float) (vault::homeNote + 12 * pt.octave + pt.semi - 69) / 12.0f);
        const float floor = std::max (450.0f, f0 * floorHarm);
        const float c = std::min (cutoffOf (pt), 9000.0f);
        const float n = std::max (floor, c * factor);
        if (n < cutoffOf (pt) - 1.0f)
        {
            pt.set (pt.kind == VaultPartKind::sample ? "s.cutoff" : "cutoff", n);
            if (pt.kind == VaultPartKind::sample && pt.get ("s.velCutoff", 0.0f) < 0.2f) pt.set ("s.velCutoff", 0.25f);
            any = true;
        }
    }
    return any;
}
// the warmth layer: a quiet sine at part 0's pitch with part 0's envelope (the "add a sine for body" practice)
VaultPart sineBody (const VaultPart& ref, bool shortSound)
{
    auto p = recipe::synthPart ("Warm Sine");
    for (auto& [k, v] : KV { { "engine", 6 }, { "wave", 0 }, { "unison", 1 }, { "detune", 0 }, { "fmAmt", 0 }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 },
                             { "keyTrack", 0 }, { "velSens", 0.6f }, { "macro2", 0 }, { "macro5", 0 }, { "master", 0 }, { "revMix", 0 } })
        p.set (k, v);
    const float att = ref.get ("s.attack", 0.0f), rel = ref.get ("s.release", 0.0f);
    p.set ("attack", std::max (shortSound ? 0.004f : 0.04f, att));
    if (shortSound || ref.get ("s.decay", 0.0f) > 0.0f)
    {
        p.set ("decay", ref.get ("s.decay", 0.0f) > 0.0f ? ref.get ("s.decay", 0.3f) : 0.45f);
        p.set ("sustain", ref.get ("s.decay", 0.0f) > 0.0f ? ref.get ("s.sustain", 0.0f) : 0.0f);
    }
    else { p.set ("decay", 1.0f); p.set ("sustain", 1.0f); }
    p.set ("release", std::max (shortSound ? 0.12f : 0.3f, rel));
    p.octave = ref.octave; p.semi = ref.semi; p.level = 0.12f;
    return p;
}
int sineIndex (const VaultPreset& p)
{
    for (int k = 1; k < (int) p.parts.size(); ++k) if (p.parts[(size_t) k].label == "Warm Sine") return k;
    return -1;
}
// returns false when the sound cannot be made comfortable without losing it
bool fit (VaultPreset& p, bool shortSound, juce::String* why = nullptr)
{
    float baseLoud = -120.0f;
    float sineCap = 1.2f;   // a refit (after a treatment) may raise a fitted sound's sine by half at most
    if (const int si0 = sineIndex (p); si0 > 0) sineCap = std::min (1.2f, 1.5f * p.parts[(size_t) si0].level);
    for (int it = 0; it < 28; ++it)
    {
        const auto m = measure (p);
        if (why != nullptr) *why = "h1 " + juce::String (m.h1, 1) + " 2-5k " + juce::String (100 * m.s25, 1) + " 6k " + juce::String (100 * m.s6k, 2) + " loud " + juce::String (m.loud, 1)
                                  + " parts " + juce::String ((int) p.parts.size());
        if (const char* tr = std::getenv ("ORCH_TRACE"); tr != nullptr && p.name == tr)
        {
            const int si = sineIndex (p);
            std::printf ("trace %s it %d: h1 %.1f s25 %.3f s6k %.3f mono %.1f loud %.1f eqLow %.1f cut0 %.0f sine %.3f\n", p.name.toRawUTF8(), it, m.h1, m.s25, m.s6k, m.mono, m.loud,
                         p.parts[0].get ("eqLow", 0.0f), cutoffOf (p.parts[0]), si > 0 ? p.parts[(size_t) si].level : 0.0f);
        }
        if (m.loud < -44.0f) return false;   // filtered to nothing: not this instrument any more
        if (baseLoud > -100.0f && m.loud > baseLoud + 5.0f) return false;   // the sine would be the sound: not this instrument any more
        if (m.mono > 2.0f)
        {
            bool any = false;
            for (auto& pt : p.parts)
                if (pt.kind == VaultPartKind::sample && pt.get ("s.width", 1.0f) > 0.15f) { pt.set ("s.width", pt.get ("s.width", 1.0f) * 0.7f); any = true; }
            if (p.parts[0].get ("width", 0.5f) > 0.3f) { p.parts[0].set ("width", 0.3f); any = true; }
            if (any) continue;
        }
        if (m.s25 > 0.07f || m.s6k > 0.02f)
        {
            if (darken (p, 0.72f, 2.0f)) continue;
            const float eh = p.parts[0].get ("eqHigh", 0.0f);
            if (eh > -9.0f) { p.parts[0].set ("eqHigh", eh - 3.0f); continue; }
            return false;
        }
        if (m.h1 < -2.0f)
        {
            const float el = p.parts[0].get ("eqLow", 0.0f);
            if (el < 6.0f) { p.parts[0].set ("eqLow", std::min (6.0f, el + 3.0f)); continue; }
            int si = sineIndex (p);
            if (si < 0 && m.h1 > -16.0f && p.parts.size() < (size_t) vault::maxParts)
            {
                // size the sine to the missing fundamental power (target: 1.5 dB under the strongest partial), measured
                // against a fresh baseline with a clearly audible probe (the fundamental of a vibrato section is a broad peak)
                const float probe = 0.15f;
                p.parts.push_back (sineBody (p.parts[0], shortSound));
                p.parts.back().level = 0.0f;
                const auto m0 = measure (p);
                p.parts.back().level = probe;
                const auto m2 = measure (p);
                const double cur = std::pow (10.0, m0.h1 / 10.0), with = std::pow (10.0, m2.h1 / 10.0), want = std::pow (10.0, -1.5 / 10.0);
                const double perUnit = std::max (1.0e-6, (with - cur) / (probe * probe));   // fundamental power (re strongest) per level^2
                p.parts.back().level = (float) juce::jlimit (0.02, 1.2, std::sqrt (std::max (0.0, want - cur) / perUnit));
                baseLoud = m0.loud;
                continue;
            }
            if (si > 0 && p.parts[(size_t) si].level < sineCap) { p.parts[(size_t) si].level = std::min (sineCap, p.parts[(size_t) si].level * 1.15f); continue; }
            if (el < 9.0f) { p.parts[0].set ("eqLow", 9.0f); continue; }
            if (darken (p, 0.8f, 1.8f)) continue;
            return false;
        }
        return true;
    }
    return false;
}

template <typename Fn> void parallel (int n, Fn fn)
{
    std::atomic<int> next { 0 };
    std::vector<std::thread> pool;
    const int nt = (int) std::max (1u, std::thread::hardware_concurrency());
    for (int t = 0; t < nt; ++t) pool.emplace_back ([&] { for (int i; (i = next++) < n;) fn (i); });
    for (auto& t : pool) t.join();
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

// what a preset is: the base sound (noun, processing family, articulation kind), its treatment (role) and rhythm pattern
struct Role
{
    juce::String noun, sub, treat = "plain";
    int kind = kSus, pattern = 0;
    int look = lNone;                                // cinematic low strings (designCinematic)
    float tone = 0.5f, hall = 0.5f;
};

bool isSine (const VaultPart& p) { return p.label == "Warm Sine"; }
// the processing family of a base sound
juce::String subFamily (const Base& b)
{
    auto has = [&] (const char* pre) { for (auto& p : b.parts) if (p.kind == VaultPartKind::sample && p.source.startsWith (pre)) return true; return false; };
    bool synth = false, allSame = true, muted = false;
    const auto& p0 = b.parts.front();
    for (auto& p : b.parts)
    {
        if (isSine (p)) continue;
        if (p.kind == VaultPartKind::synth) synth = true;
        if (p.source != p0.source) allSame = false;
        if (p.articulation.contains ("mute") || p.articulation == "muted") muted = true;
    }
    int n = 0;
    for (auto& p : b.parts) if (! isSine (p)) ++n;
    if (b.cat == vault::catStrings)
    {
        if (p0.kind == VaultPartKind::synth) return "machine";
        if (synth) return "hybrid";
        if (n == 1) return p0.source == "strings.contrabass" ? "bass" : p0.source == "strings.violin.solo" ? "solo" : "section";
        return "ensemble";
    }
    if (b.cat == vault::catBrass)
    {
        if (p0.kind == VaultPartKind::synth) return "synth";
        if (synth) return "hybrid";
        if (has ("winds.sax")) return "pop";
        if (muted) return "muted";
        if (allSame && n == 1)
            return p0.source == "brass.trumpet" ? "trumpet" : p0.source == "brass.horn" ? "horn" : p0.source == "brass.tuba" ? "tuba"
                 : p0.source == "brass.trombone.vintage" ? "vintage" : "trombone";
        return allSame ? "section" : "choir";
    }
    if (synth) return "hybrid";
    if (n == 1 && p0.source == "perc.timpani") return "timpani";
    if (n == 1 && p0.source.startsWith ("pluck.harp")) return "harp";
    if (b.kind == kShort && (has ("perc.timpani") || has ("brass."))) return "hit";
    if (has ("voice.") || has ("organ.") || has ("fx.glass") || has ("pluck.psaltery") || has ("bells.") || has ("piano.")) return "colour";
    if (b.kind == kShort || b.kind == kRing)
    {
        if (has ("perc.timpani")) return "timpani";
        if (has ("pluck.harp")) return "harp";
        return "playful";
    }
    if (has ("brass.")) return "tutti";
    if (! has ("strings.")) return "winds";
    return "doubling";
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
Chain chainOf (const VaultPreset& p) { return Chain (p.fx); }

VaultFxSlot rv (R type, float size, float decay, float mix, float predelay, float damping, float lowCut = 200.0f)
{
    return fx::reverb (type, size, decay, mix).set ("predelay", predelay).set ("damping", damping).set ("lowCut", lowCut);
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
    // designed value v at the default, `atMax` at 1 (e.g. a drive's output trim that keeps the level as the drive goes up)
    M& toMax (const char* target, float v, float atMax)
    {
        const float span = d < 1.0f - 1.0e-4f ? (atMax - v) / (1.0f - d) : 0.0f;
        m.targets.push_back ({ target, v - span * d, atMax, 0.0f });
        return *this;
    }
    M& fxp (const Chain& c, FT t, const char* id, float span)
    {
        const auto& ti = vault::fxTypeInfo (t);
        const auto& pi = ti.params[(size_t) ti.indexOf (id)];
        const auto target = juce::String ("fx.") + ti.id + "." + id;
        return abs (target.toRawUTF8(), c.get (t, id), span, pi.lo, pi.hi);
    }
};
void setMacro (VaultPreset& p, int i, const M& m) { p.macros[(size_t) i] = m.m; p.macroDefaults[i] = m.d; }

// movement
VaultLfo vibrato (float rate, float cents) { return lfo (VaultLfo::sine).rate (rate).delay (0.35f).fade (0.6f).depth (0.0f).to ("pitch", cents / 100.0f); }
VaultLfo breathing (float depth, float rate, float cutoffOct, float vol) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", cutoffOct).to ("volume", vol); }
VaultLfo drift (float depth, float rate, float cutoffOct, float pan) { return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", cutoffOct).to ("pan", pan); }
VaultLfo tapeWobble (float depth, float rate = 0.55f) { return lfo (VaultLfo::triangle).rate (rate).free().depth (depth).to ("pitch", 0.08f); }

bool bowed (const Role& r) { return r.sub == "section" || r.sub == "solo" || r.sub == "bass" || r.sub == "ensemble" || r.sub == "hybrid" || r.sub == "doubling"; }
bool isBrass (const juce::String& s)
{
    return s == "trumpet" || s == "horn" || s == "trombone" || s == "vintage" || s == "tuba" || s == "section" || s == "choir" || s == "muted" || s == "pop";
}

//------------------------------------------------------------------------------ 1. the chain + movement (no macros yet)
// what the macros will need, decided with the chain
struct Plan
{
    juce::String tone = "Brightness", space = "Hall", third, fourth;
    bool shortS = false;
};
Plan designChain (VaultPreset& p, const Role& r)
{
    design (p);   // v2 -> v3: the role's colour slots (delay, lo-fi, drive, phaser, reverse, width) and the fitted EQ arrive here
    Chain c (p.fx);
    const auto& f = r.sub;
    const auto& t = r.treat;
    const bool cat = p.category == vault::catStrings, brass = p.category == vault::catBrass;
    const bool sh = r.kind == kShort || r.kind == kRing;
    const bool sus = ! sh;
    Plan pl;
    pl.shortS = sh;
    p.mod = {};
    float finish = 0.06f;
    // the fitted stereo image is kept (a narrowed sound stays narrow)
    const float fittedWidth = c.has (FT::width) ? c.get (FT::width, "width") : 1.0f;
    auto width = [&] (float w, float mono) { c.put (fx::width (std::min (w, std::max (fittedWidth, 0.3f)), mono)); };
    auto mid = [&] (float db, float hz, float q) { auto& e = c.at (FT::eq); e.set ("mid", db).set ("midFreq", hz).set ("midQ", q); };
    auto drive = [&] (fx::Drive type, float amount, float mix, float tone)
    {
        // a drive the role brought along (Dark / Tape) keeps its amount, gets the clean lows and a gentle tone
        if (c.has (FT::drive)) c.at (FT::drive).set ("cleanLow", 1.0f).set ("tone", std::min (tone, c.get (FT::drive, "tone")));
        else c.put (fx::drive (type, amount, mix, tone).set ("cleanLow", 1.0f));
    };

    // 1. the family's finished sound
    if (cat)
    {
        if (f == "machine")
        {   // the string machine: ensemble chorus + phaser into a hall, a slow brightness drift
            c.put (fx::ensemble (0.38f, 0.6f, 0.5f));
            c.put (fx::phaser (0.22f, 0.12f, 0.8f, 0.35f));
            c.eqAdd ("low", 0.5f); mid (-1.5f, 400, 0.8f);
            c.put (rv (R::hall, 0.75f, 1.3f, 0.24f, 25, 0.25f, 220));
            width (0.7f, 150);
            p.mod.lfo[1] = drift (0.5f, 0.08f, 0.25f, 0.0f);
            pl.tone = "Brightness"; pl.third = "Ensemble"; pl.fourth = "Phase";
            finish = 0.08f;
        }
        else if (sh)
        {   // pizzicato / spiccato: a touch of transient + fast glue, a shorter hall so the rhythm stays clear
            c.put (fx::transient (0.08f));
            c.put (fx::comp (-22, 2.5f, 8, 120, 1.0f, 0.55f));
            mid (-1.0f, 380, 0.8f);
            c.put (rv (R::hall, 0.55f, 0.9f, 0.17f, 15, 0.3f, 220));
            width (0.6f, 120);
            pl.tone = "Snap"; pl.third = "Body"; pl.fourth = "Echo";
        }
        else
        {   // bowed sustains: LA-2A-style light glue, box cut, a hall with pre-delay so the bow reads first, slow bow breathing
            c.put (fx::comp (-26, 1.6f, 40, 300, 1.0f, 0.5f));
            mid (f == "bass" ? -1.5f : -1.5f, f == "bass" ? 250.0f : 350.0f, 0.8f);
            c.put (rv (R::hall, 0.72f, 1.25f, 0.24f, 30, 0.25f, f == "bass" ? 260.0f : 220.0f));
            width (f == "solo" ? 0.5f : 0.6f, 120);
            p.mod.lfo[1] = breathing (0.5f, 0.09f, 0.45f, 0.1f);
            if (f == "hybrid" && ! c.has (FT::chorus)) c.put (fx::ensemble (0.15f, 0.5f, 0.4f));
            pl.tone = "Bow"; pl.third = "Swell"; pl.fourth = r.kind == kTrem ? "Motion" : f == "hybrid" ? "Synth" : "Vibrato";
            finish = 0.07f;
        }
    }
    else if (brass)
    {
        if (f == "synth")
        {   // synth brass: chorus, a tube edge with clean lows, plate / hall
            c.put (fx::comp (-20, 2.5f, 15, 150, 1.0f, 0.6f));
            drive (fx::Drive::tube, 0.1f, 0.6f, 5000);
            c.put (fx::chorus (0.2f, 0.4f, 0.4f));
            mid (1.0f, 400, 0.8f);
            c.put (sh ? rv (R::plate, 0.45f, 0.8f, 0.14f, 8, 0.3f) : rv (R::hall, 0.65f, 1.1f, 0.2f, 20, 0.3f));
            width (0.65f, 150);
            pl.tone = "Cutoff"; pl.third = "Envelope"; pl.fourth = "Detune";
            if (sh) pl.space = "Plate";
            p.mod.velocity.push_back ({ "cutoff", 0.6f });   // the blat opens with velocity
        }
        else if (f == "pop")
        {   // pop / soul horns: tape, glue, a plate and a short slapback
            c.put (fx::comp (-20, 2.5f, 10, 120, 1.0f, 0.5f));
            drive (fx::Drive::tape, 0.14f, 0.6f, 5500);
            mid (1.0f, 500, 0.8f);
            c.put (fx::delayMs (110, 0.05f, 0.08f).set ("hiCut", 4000.0f).set ("loCut", 250.0f));
            c.put (rv (R::plate, 0.45f, 0.85f, 0.16f, 15, 0.3f));
            width (0.6f, 120);
            pl.tone = "Bite"; pl.space = "Plate"; pl.third = "Tape"; pl.fourth = sh ? "Punch" : "Swell";
        }
        else if (f == "muted")
        {   // muted brass: nasal colour kept, a plate and a delay ready to throw
            c.put (fx::comp (-22, 2.0f, 20, 160, 1.0f, 0.5f));
            mid (-1.0f, 900, 0.9f);
            if (! c.has (FT::delay)) c.put (fx::delay ("1/4D", 0.3f, 0.0f).set ("hiCut", 3500.0f).set ("loCut", 300.0f));
            c.put (rv (R::plate, 0.5f, 0.95f, 0.2f, 12, 0.3f));
            width (0.55f, 120);
            pl.tone = "Bite"; pl.space = "Plate"; pl.third = "Swell"; pl.fourth = "Echo";
        }
        else if (sh)
        {   // brass stabs: transient, glue, tape warmth, short plate
            c.put (fx::transient (0.15f));
            c.put (fx::comp (-20, 3.0f, 5, 100, 1.0f, 0.5f));
            drive (fx::Drive::tape, 0.1f, 0.5f, 5000);
            mid (1.0f, 300, 0.8f);
            c.put (rv (R::plate, 0.42f, 0.75f, 0.14f, 8, 0.3f));
            width (0.6f, 120);
            pl.tone = "Bite"; pl.space = "Plate"; pl.third = "Punch"; pl.fourth = "Echo";
        }
        else
        {   // sustained brass: gentle 3:1-ish glue, tape smoothing of the top (not EQ), low-mid warmth, the hall by register
            c.put (fx::comp (-22, 2.5f, 20, 200, 1.0f, 0.6f));
            drive (fx::Drive::tape, 0.08f, 0.5f, 5000);
            mid (1.0f, 300, 0.8f);
            if (f == "horn") c.put (rv (R::hall, 0.78f, 1.3f, 0.26f, 25, 0.25f));        // bell to the back wall: more hall
            else if (f == "vintage") c.put (rv (R::plate, 0.5f, 0.9f, 0.18f, 12, 0.3f));  // the jazz plate
            else if (f == "trombone" || f == "tuba") c.put (rv (R::hall, 0.65f, 1.1f, 0.2f, 20, 0.3f, 260));
            else c.put (rv (R::hall, 0.65f, 1.05f, 0.21f, 20, 0.3f));
            width (f == "section" || f == "choir" ? 0.65f : 0.55f, 120);
            pl.tone = "Bite"; pl.third = "Swell";
            pl.fourth = r.noun.containsIgnoreCase ("Vibrato") || r.noun.containsIgnoreCase ("Crooning") ? "Warmth"
                      : f == "section" || f == "choir" ? "Width" : f == "hybrid" ? "Synth" : "Vibrato";
            if (f == "vintage") pl.space = "Plate";
        }
    }
    else   // orchestral
    {
        if (f == "timpani")
        {   // compressed body, mud cut, the low end out of the hall; hip-hop grit ready
            c.put (fx::transient (0.1f));
            c.put (fx::comp (-24, 3.0f, 15, 250, 2.0f, 0.5f));
            if (! c.has (FT::drive)) c.put (fx::drive (fx::Drive::tape, 0.0f, 0.6f, 4000).set ("cleanLow", 1.0f));
            mid (-2.0f, 400, 0.8f);
            c.put (rv (R::hall, 0.8f, 1.2f, 0.22f, 10, 0.25f, 300));
            width (0.55f, 150);
            pl.tone = "Mallet"; pl.third = "Weight"; pl.fourth = r.kind == kTrem ? "Swell" : "Grit";
        }
        else if (f == "harp")
        {
            c.put (fx::transient (0.08f));
            c.put (fx::comp (-24, 2.0f, 15, 200, 1.0f, 0.4f));
            mid (-1.0f, 350, 0.8f);
            if (! c.has (FT::delay)) c.put (fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 4000.0f).set ("loCut", 250.0f));
            c.put (rv (R::hall, 0.68f, 1.2f, 0.24f, 20, 0.2f));
            width (0.6f, 120);
            pl.tone = "Pluck"; pl.third = "Body"; pl.fourth = "Echo";
        }
        else if (f == "hit")
        {   // orchestra hits: transient, parallel-style glue, tape grit, a big hall with its lows cut
            c.put (fx::transient (0.18f));
            c.put (fx::comp (-20, 3.5f, 5, 120, 2.0f, 0.65f));
            drive (fx::Drive::tape, 0.1f, 0.6f, 5000);
            mid (-1.5f, 350, 0.8f);
            c.put (rv (R::hall, 0.72f, 1.1f, 0.2f, 12, 0.25f, 260));
            width (0.65f, 120);
            pl.tone = "Punch"; pl.third = "Grit"; pl.fourth = "Echo";
        }
        else if (f == "playful")
        {
            c.put (fx::transient (0.12f));
            c.put (fx::comp (-22, 2.5f, 8, 120, 1.0f, 0.55f));
            mid (-1.0f, 400, 0.8f);
            if (! c.has (FT::delay)) c.put (fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 4000.0f).set ("loCut", 250.0f));
            c.put (rv (R::hall, 0.55f, 0.9f, 0.17f, 12, 0.3f));
            width (0.6f, 120);
            pl.tone = "Snap"; pl.third = "Body"; pl.fourth = "Echo";
        }
        else if (f == "colour")
        {   // strings with choir / organ / glass / bells / piano: a bigger, softer hall and a slow drift
            c.put (fx::comp (-26, 1.6f, 40, 300, 1.0f, 0.4f));
            mid (-1.5f, 350, 0.8f);
            c.put (rv (R::hall, 0.8f, 1.4f, 0.26f, 25, 0.2f, 220));
            width (0.65f, 120);
            p.mod.lfo[1] = drift (0.5f, 0.07f, 0.3f, 0.12f);
            pl.third = "Swell"; pl.fourth = "Motion";
            if (sh) { pl.tone = "Brightness"; pl.third = "Body"; pl.fourth = "Echo"; c.put (fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 4000.0f).set ("loCut", 250.0f)); }
        }
        else if (f == "tutti" || f == "hybrid")
        {   // tutti: bus glue, a whisper of tape, mud cut, a big hall
            c.put (fx::comp (-24, 2.0f, 30, 300, 1.5f, 0.6f));
            drive (fx::Drive::tape, 0.06f, 0.4f, 5000);
            mid (-1.5f, 300, 0.8f);
            c.put (rv (R::hall, 0.8f, 1.35f, 0.26f, 30, 0.25f, 220));
            width (0.65f, 120);
            p.mod.lfo[1] = breathing (0.5f, 0.08f, 0.4f, 0.08f);
            pl.third = "Swell"; pl.fourth = f == "hybrid" ? "Synth" : "Power";
        }
        else   // woodwinds and woodwind + string doublings
        {
            c.put (fx::comp (-26, 1.6f, 40, 300, 1.0f, 0.45f));
            mid (-1.0f, 500, 0.8f);
            c.put (rv (R::hall, 0.68f, 1.15f, 0.23f, 22, 0.25f));
            width (0.6f, 120);
            p.mod.lfo[1] = breathing (0.5f, 0.1f, 0.4f, 0.08f);
            pl.tone = f == "winds" ? "Breath" : "Bow"; pl.third = "Swell"; pl.fourth = "Vibrato";
        }
    }

    // 2. the role (v2 treatment) as a processing style
    auto& rvb = c.at (FT::reverb);
    if (t == "Distant")
    {   // far away: more and darker hall, less pre-delay, narrower, the top rolled off
        rvb.set ("size", 0.92f).set ("decay", 1.45f).set ("mix", std::max (0.34f, rvb.get ("mix"))).set ("predelay", 8.0f).set ("damping", 0.45f);
        c.eqAdd ("high", -2.0f);
        if (c.has (FT::width)) c.at (FT::width).set ("width", std::min (0.42f, c.get (FT::width, "width")));
        pl.space = "Distance";
    }
    else if (t == "Muted" || t == "Muffled")
    {
        rvb.set ("type", (float) R::room).set ("size", 0.45f).set ("decay", 0.9f).set ("mix", 0.18f).set ("predelay", 6.0f).set ("damping", 0.4f);
        pl.space = "Room";
    }
    else if (t == "Stab")
    {
        rvb.set ("type", (float) R::plate).set ("size", 0.42f).set ("decay", 0.7f).set ("mix", 0.14f).set ("predelay", 8.0f);
        if (! c.has (FT::transient)) c.put (fx::transient (0.1f));
        pl.space = "Plate";
        if (pl.third == "Swell") pl.third = "Punch";
    }
    else if (t == "Tight")
    {
        rvb.set ("type", (float) R::room).set ("size", 0.35f).set ("decay", 0.7f).set ("mix", 0.11f).set ("predelay", 4.0f).set ("damping", 0.35f);
        pl.space = "Room";
    }
    else if (t == "Dark")
    {   // dark tape colour into a damped plate
        drive (fx::Drive::tape, 0.18f, 0.7f, 3500);
        c.at (FT::eq).set ("highFreq", 4000.0f);
        rvb.set ("type", (float) R::plate).set ("size", 0.5f).set ("decay", 0.9f).set ("mix", 0.14f).set ("damping", 0.5f);
        pl.space = "Plate"; pl.fourth = "Grit";
    }
    else if (t == "Reverse")
    {
        rvb.set ("type", (float) R::hall).set ("mix", std::max (0.2f, rvb.get ("mix")));
        pl.fourth = "Reverse";
    }
    else if (t == "Dusty" || t == "Tape")
    {   // vinyl / tape: tape drive, a dark small room, the tape wobble on pitch
        drive (fx::Drive::tape, t == "Tape" ? 0.3f : 0.1f, 0.7f, 5000);
        c.at (FT::eq).set ("highFreq", 4500.0f);
        if (! c.has (FT::lofi)) c.put (fx::lofi (0.0f, 0.15f, 1.0f));
        rvb.set ("type", (float) R::room).set ("size", 0.4f).set ("decay", 0.9f).set ("mix", 0.13f).set ("predelay", 6.0f).set ("damping", 0.5f);
        p.mod.lfo[1] = tapeWobble (t == "Tape" ? 0.35f : 0.5f);
        pl.space = "Room"; pl.fourth = "Tape";
    }
    else if (t == "Crushed")
    {
        rvb.set ("type", (float) R::room).set ("size", 0.4f).set ("decay", 0.8f).set ("mix", 0.12f).set ("damping", 0.5f);
        pl.space = "Room"; pl.fourth = "Crush";
    }
    else if (t == "Stutter")
    {   // a 1/16 ping-pong repeat as the rhythm of the sound
        c.put (fx::delay ("1/16", 0.45f, 0.3f).set ("hiCut", 4500.0f).set ("loCut", 250.0f));
        rvb.set ("type", (float) R::room).set ("size", 0.45f).set ("decay", 0.8f).set ("mix", 0.12f);
        pl.space = "Room"; pl.fourth = "Echo";
    }
    else if (t == "Echoing")
    {
        c.put (fx::delay ("1/4D", 0.4f, 0.24f).set ("hiCut", 4000.0f).set ("loCut", 250.0f));
        pl.fourth = "Echo";
    }
    else if (t == "Floating" || t == "Frozen")
    {   // ambient: cloud reverb, a slow drift of brightness and pan
        rvb.set ("type", (float) R::cloud).set ("size", t == "Frozen" ? 0.55f : 0.5f).set ("decay", t == "Frozen" ? 0.8f : 0.9f)
           .set ("mix", t == "Frozen" ? 0.38f : 0.34f).set ("predelay", 20.0f).set ("damping", 0.3f);
        p.mod.lfo[1] = drift (0.5f, t == "Frozen" ? 0.06f : 0.1f, 0.25f, 0.25f);
        pl.space = "Space"; pl.fourth = "Drift";
    }
    else if (t == "Swirling")
    {
        if (c.has (FT::phaser)) c.at (FT::phaser).set ("rate", 0.2f);
        pl.fourth = "Phase";
    }
    else if (t == "Marcato")
    {
        if (! c.has (FT::transient)) c.put (fx::transient (0.12f));
    }
    else if (t == "Doubled")
    {
        if (c.has (FT::width) && fittedWidth >= 0.5f) c.at (FT::width).set ("width", std::max (c.get (FT::width, "width"), 0.68f));
    }
    if (rvb.get ("type") == (float) R::room && pl.space == "Hall") pl.space = "Room";
    if (rvb.get ("type") == (float) R::plate && pl.space == "Hall") pl.space = "Plate";

    // the slots the macros open (silent as designed)
    if ((pl.fourth == "Echo" || pl.third == "Echo") && ! c.has (FT::delay)) c.put (fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 4000.0f).set ("loCut", 250.0f));
    if ((pl.fourth == "Grit" || pl.third == "Grit" || pl.fourth == "Power" || pl.tone == "Bite" || pl.third == "Tape") && ! c.has (FT::drive))
        c.put (fx::drive (fx::Drive::tape, 0.0f, 0.6f, 5000).set ("cleanLow", 1.0f));
    if ((pl.third == "Punch" || pl.fourth == "Punch" || pl.tone == "Snap" || pl.tone == "Punch" || pl.tone == "Pluck" || pl.tone == "Mallet") && ! c.has (FT::transient))
        c.put (fx::transient (0.1f));
    if (pl.fourth == "Width" && ! c.has (FT::width)) width (0.6f, 120);
    if (pl.fourth == "Detune" && ! c.has (FT::chorus)) c.put (fx::chorus (0.2f, 0.4f, 0.4f));
    if (pl.fourth == "Phase" && ! c.has (FT::phaser)) pl.fourth = "Vibrato";
    if (pl.fourth == "Reverse" && ! c.has (FT::reverse)) pl.fourth = "Echo";
    if (pl.fourth == "Crush" && ! c.has (FT::lofi)) pl.fourth = "Grit";
    if (pl.fourth == "Synth")
    {
        bool any = false;
        for (auto& part : p.parts) if (part.kind == VaultPartKind::synth && ! isSine (part)) any = true;
        if (! any) pl.fourth = "Vibrato";
    }
    if (pl.fourth == "Vibrato" || pl.fourth == "Swell" || pl.third == "Swell")
        if (sh) { pl.fourth = "Echo"; if (! c.has (FT::delay)) c.put (fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 4000.0f).set ("loCut", 250.0f)); }
    if (pl.fourth == "Echo" && ! c.has (FT::delay)) c.put (fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 4000.0f).set ("loCut", 250.0f));
    if (pl.fourth == "Grit" && ! c.has (FT::drive)) c.put (fx::drive (fx::Drive::tape, 0.0f, 0.6f, 5000).set ("cleanLow", 1.0f));

    // vibrato LFO: bowed / blown sustains get a natural, delayed vibrato (off as designed; the macro and mod wheel bring it in)
    const bool vib = sus && (bowed (r) || isBrass (f) || f == "winds" || f == "machine" || f == "synth" || f == "colour" || f == "tutti");
    if (vib) p.mod.lfo[0] = vibrato (f == "horn" || f == "tuba" || f == "trombone" ? 4.8f : 5.4f, f == "bass" || f == "tuba" ? 14.0f : 18.0f);

    // 3. the rhythm presets (plain musical patterns from Init)
    if (r.pattern == 1)   // Pizzicato Arpeggio: up, eighths, two octaves
        p.arp = arp (VaultArp::up, "1/8", 1, 1.0f, 0.0f, true);
    else if (r.pattern == 2)   // Spiccato Ostinato: as played sixteenths, accents on the beats
    {
        p.arp = arp (VaultArp::asPlayed, "1/16", 1, 0.55f, 0.0f, true);
        for (int i = 0; i < 16; ++i) p.arp.vel[(size_t) i] = i % 4 == 0 ? 1.0f : i % 2 == 0 ? 0.8f : 0.68f;
    }
    else if (r.pattern == 3)   // Gated Strings: sixteenth gate
    {
        p.gate = gate ("1/16", { 1, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 0, 1, 0 }, 1.0f, 0.2f, 1.0f, true);
        pl.fourth = "Gate";
    }
    else if (r.pattern == 4)   // Horn Pulse Eighths: eighth gate with a longer last pulse
    {
        p.gate = gate ("1/8", { 1, 0, 1, 0, 1, 0, 1, 1 }, 1.0f, 0.2f, 1.0f, true);
        pl.fourth = "Gate";
    }
    else if (r.pattern == 5)   // Woodwind Arpeggio: up-down sixteenths over two octaves
        p.arp = arp (VaultArp::upDown, "1/16", 1, 0.6f, 0.0f, true);

    p.fx = c.build();
    p.finish = finish;
    for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; }
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
// the designed v3 sound keeps a margin under the gate's comfort limits (the macros may move from there)
juce::String comfort (VaultPreset& p)
{
    const float ceil = recipegate::centroidCeiling (p.category) - 80.0f;
    juce::String log;
    for (int it = 0; it < 10; ++it)
    {
        const auto x = probe (p, p.macroDefaults, 3.0, false);
        const bool bright = x.s25 > 0.08f || x.s6 > 0.022f || x.s8 > 0.007f || x.centroid > ceil;
        const bool weak = x.h1 < -2.3f;
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
void designMacros (VaultPreset& p, const Role& r, const Plan& pl)
{
    Chain c (p.fx);
    auto has = [&] (FT x) { return c.has (x); };
    const bool synth0 = p.parts.front().kind == VaultPartKind::synth;
    int synthLayer = -1;
    for (int k = 0; k < (int) p.parts.size(); ++k) if (p.parts[(size_t) k].kind == VaultPartKind::synth && ! isSine (p.parts[(size_t) k]) && k > 0) synthLayer = k;

    // 1. tone: the bow / breath / brass bite, or the attack (snap, punch, pluck, mallet) on short sounds
    {
        const auto& n = pl.tone;
        M m (n.toRawUTF8(), 0.5f);
        if (n == "Snap" || n == "Punch" || n == "Pluck" || n == "Mallet")
        {
            m = M (n.toRawUTF8(), 0.3f);
            m.fxp (c, FT::transient, "attack", 0.6f).rel ("cutoff", 1.2f);
            if (n == "Punch" && has (FT::compressor)) m.fxp (c, FT::compressor, "threshold", 12.0f);
        }
        else if (n == "Cutoff") { m.rel ("cutoff", 2.4f).rel ("reso", 0.2f); }
        else
        {
            m.rel ("cutoff", 1.8f).fxp (c, FT::eq, "tilt", 4.0f);
            if (n == "Bite" && has (FT::drive)) m.fxp (c, FT::drive, "amount", 0.3f).toMax ("fx.drive.output", c.get (FT::drive, "output"), c.get (FT::drive, "output") - 2.0f);
        }
        setMacro (p, 0, m);
    }
    // 2. space: the reverb (and a delay that is part of the sound) - amount, length, pre-delay
    {
        M m (pl.space.toRawUTF8(), 0.4f);
        const bool small = c.get (FT::reverb, "type") == (float) R::room;
        m.fxp (c, FT::reverb, "mix", small ? 0.55f : 0.45f).fxp (c, FT::reverb, "decay", small ? 2.0f : 1.2f);
        if (pl.space != "Room" && pl.space != "Distance") m.fxp (c, FT::reverb, "predelay", 30.0f);
        if (has (FT::delay) && c.get (FT::delay, "mix") > 0.05f && pl.fourth != "Echo") m.fxp (c, FT::delay, "mix", 0.25f);
        setMacro (p, 1, m);
    }
    // 3 / 4. the family / role characters
    auto make = [&] (const juce::String& n) -> M
    {
        if (n == "Swell") { M m ("Swell", 0.0f); m.rel ("quick.attack", 0.42f).rel ("quick.release", 0.22f); return m; }
        if (n == "Vibrato") { M m ("Vibrato", 0.0f); m.abs ("lfo1.depth", 0.0f, 1.0f, 0, 1); return m; }
        if (n == "Motion")
        {   // from the designed slow breathing to a faster, deeper swell (tremolo-like on the ensemble)
            const float rate = p.mod.lfo[1].rate, dep = p.mod.lfo[1].depth;
            M m ("Motion", 0.0f); m.abs ("lfo2.depth", dep, 1.0f - dep, 0, 1).abs ("lfo2.rate", rate, std::max (0.0f, 1.6f - rate), 0.02f, 2.0f);
            return m;
        }
        if (n == "Drift")
        {   // from the designed slow drift to a deeper, faster one, wider
            const float rate = p.mod.lfo[1].rate, dep = p.mod.lfo[1].depth;
            M m ("Drift", 0.0f); m.abs ("lfo2.depth", dep, 1.0f - dep, 0, 1).abs ("lfo2.rate", rate, std::max (0.0f, 0.6f - rate), 0.02f, 2.0f);
            if (has (FT::width)) m.fxp (c, FT::width, "width", 0.25f);
            return m;
        }
        if (n == "Body" || n == "Weight" || n == "Warmth")
        {
            M m (n.toRawUTF8(), 0.5f); m.fxp (c, FT::eq, "low", n == "Weight" ? 8.0f : 6.0f).fxp (c, FT::eq, "mid", 3.0f); return m;
        }
        if (n == "Grit" || n == "Tape" || n == "Power")
        {
            M m (n.toRawUTF8(), n == "Tape" ? 0.5f : 0.2f);
            m.fxp (c, FT::drive, "amount", n == "Grit" ? 0.6f : 0.45f).toMax ("fx.drive.output", c.get (FT::drive, "output"), c.get (FT::drive, "output") - 4.0f);
            if (n == "Tape" && has (FT::lofi)) m.fxp (c, FT::lofi, "wow", 0.6f);
            if (n == "Tape" && p.mod.lfo[1].dests.size() == 1 && p.mod.lfo[1].dests[0].target == "pitch") m.abs ("lfo2.depth", p.mod.lfo[1].depth, 1.0f, 0, 1);
            if (n == "Power" && has (FT::compressor)) m.fxp (c, FT::compressor, "threshold", 10.0f);
            return m;
        }
        if (n == "Echo")
        {
            const float dm = c.get (FT::delay, "mix");
            M m ("Echo", dm > 0.05f ? 0.5f : 0.0f);
            m.fxp (c, FT::delay, "mix", 0.4f).fxp (c, FT::delay, "feedback", 0.3f);
            return m;
        }
        if (n == "Punch") { M m ("Punch", 0.3f); m.fxp (c, FT::transient, "attack", 0.6f); if (has (FT::compressor)) m.fxp (c, FT::compressor, "threshold", 12.0f); return m; }
        if (n == "Width") { M m ("Width", 0.5f); m.fxp (c, FT::width, "width", 0.5f); return m; }
        if (n == "Ensemble") { M m ("Ensemble", 0.5f); m.fxp (c, FT::chorus, "mix", 0.7f).fxp (c, FT::chorus, "depth", 0.5f); return m; }
        if (n == "Detune") { M m ("Detune", 0.4f); m.fxp (c, FT::chorus, "mix", 0.6f).fxp (c, FT::chorus, "depth", 0.5f); return m; }
        if (n == "Phase") { M m ("Phase", 0.4f); m.fxp (c, FT::phaser, "mix", 0.7f).fxp (c, FT::phaser, "rate", 0.4f); return m; }
        if (n == "Envelope") { M m ("Envelope", 0.5f); m.rel ("quick.envAmt", 0.8f); return m; }
        if (n == "Reverse") { M m ("Reverse", 0.5f); m.fxp (c, FT::reverse, "mix", 0.8f); return m; }
        if (n == "Crush") { M m ("Crush", 0.5f); m.fxp (c, FT::lofi, "crush", 0.6f).fxp (c, FT::lofi, "mix", 0.6f); return m; }
        if (n == "Gate") { M m ("Gate", 1.0f); m.abs ("gate.depth", 1.0f, 1.0f, 0, 1); return m; }
        if (n == "Synth" && synthLayer > 0)
        {
            const float l = p.parts[(size_t) synthLayer].level;
            M m ("Synth", 0.5f); m.abs (("layer" + juce::String (synthLayer + 1) + ".level").toRawUTF8(), l, 2.0f * l, 0.0f, 2.0f); return m;
        }
        M m ("Vibrato", 0.0f); m.abs ("lfo1.depth", 0.0f, 1.0f, 0, 1); return m;
    };
    auto third = pl.third, fourth = pl.fourth;
    if (third == fourth) fourth = "Echo";
    if ((third == "Vibrato" || fourth == "Vibrato") && p.mod.lfo[0].dests.empty())
    {
        if (third == "Vibrato") third = "Body";
        if (fourth == "Vibrato") fourth = pl.shortS ? "Echo" : "Motion";
    }
    if ((fourth == "Motion") && p.mod.lfo[1].dests.empty()) p.mod.lfo[1] = breathing (0.5f, 0.09f, 0.45f, 0.1f);
    if (fourth == "Echo" && ! has (FT::delay)) fourth = "Body";
    setMacro (p, 2, make (third));
    setMacro (p, 3, make (fourth));

    // performance: mod wheel = vibrato on sustains (more hall on short sounds), the touch kept against the compressor
    if (! p.mod.lfo[0].dests.empty()) p.mod.modWheel.push_back ({ "lfo1.depth", 0.8f });
    else p.mod.modWheel.push_back ({ "fx.reverb.mix", 0.25f });
    if (! pl.shortS) p.mod.aftertouch.push_back ({ "volume", 0.25f });   // expression swell
    if (has (FT::compressor) || has (FT::drive))
        p.mod.velocity.push_back ({ "volume", r.sub == "pop" ? 0.9f : pl.shortS ? 0.85f : 0.45f });
    juce::ignoreUnused (synth0, r);
}

//------------------------------------------------------------------------------ the cinematic low strings (addCinematic)
// One finished sound per preset, voiced by its look (how the sections are played), its tone (dark .. open) and its hall
// (close .. cathedral): section-bus glue, tape power with clean lows, a warm dark EQ (low shelf, 330 Hz box cut, top
// shelved off), a long hall with the strings' long pre-delay and a damped tail, lows mono; slow movement on every preset;
// four macros named for what they do to these sections.
Plan designCinematic (VaultPreset& p, const Role& r)
{
    design (p);   // the fitted v2 EQ / width arrive as slots
    Chain c (p.fx);
    const float dk = 1.0f - r.tone, h = r.hall;
    const bool sh = r.look == lSpicc || r.look == lPizz;
    const bool pad = r.look == lSwell || r.look == lTasto;
    p.mod = {};
    Plan pl;
    pl.shortS = r.kind == kShort;
    // 1. dynamics: section-bus glue; accents get a transient and faster glue
    if (sh) { c.put (fx::transient (r.look == lSpicc ? 0.14f : 0.1f)); c.put (fx::comp (-22, 2.5f, 6, 110, 1.5f, 0.6f)); }
    else if (r.look == lMarcato) { c.put (fx::transient (0.16f)); c.put (fx::comp (-24, 2.5f, 12, 180, 1.5f, 0.6f)); }
    else c.put (fx::comp (-26, 1.8f, 35, 320, 1.0f, 0.5f));
    // 2. power: tape saturation on the upper band only (the lows stay clean), its top rolled off
    {
        const float amt = r.look == lMarcato ? 0.2f : 0.07f;
        c.put (fx::drive (fx::Drive::tape, amt, 0.6f, 6500.0f - 2500.0f * dk).set ("cleanLow", 1.0f));
    }
    // 3. the warm dark EQ (kept at least as warm / dark as the C5 fit made it)
    {
        auto& e = c.at (FT::eq);
        e.set ("low", juce::jlimit (-18.0f, 12.0f, std::max (e.get ("low"), 2.0f + 1.5f * dk)));
        e.set ("mid", -2.0f).set ("midFreq", 330.0f).set ("midQ", 0.9f);
        e.set ("high", std::min (e.get ("high"), -2.0f - 3.0f * dk)).set ("highFreq", std::min (e.get ("highFreq"), 5000.0f));
    }
    // 4. the warm long hall: strings sit nearest the audience - the longest pre-delay, a damped (dark) tail, lows cut
    {
        const float mix = sh ? 0.17f + 0.12f * h : 0.2f + 0.16f * h;
        auto room = rv (h < 0.2f ? R::room : R::hall, 0.8f + 0.15f * h, 1.25f + 0.6f * h, mix, 26.0f + 24.0f * h, 0.3f + 0.35f * dk, 200.0f);
        c.put (room);
    }
    // the echo the ostinatos' macro opens: its low cut under the cellos' and basses' range, so the repeats are heard
    if (sh && ! c.has (FT::delay)) c.put (fx::delay ("1/8D", 0.3f, 0.0f).set ("hiCut", 3500.0f).set ("loCut", 90.0f));
    c.put (fx::width (pad ? 0.7f : 0.62f, 160));
    // 5. movement: a delayed bowed vibrato (mod wheel / macro), slow bow-pressure breathing, drift on pads, tension surges
    if (! sh) p.mod.lfo[0] = vibrato (5.0f, 16.0f);
    switch (r.look)
    {
        case lSwell:  p.mod.lfo[1] = breathing (0.6f, 0.06f, 0.5f, 0.12f); break;
        case lTasto:  p.mod.lfo[1] = drift (0.6f, 0.05f, 0.35f, 0.15f); break;
        case lTrem:   p.mod.lfo[1] = lfo (VaultLfo::sine).rate (0.11f).free().depth (0.6f).to ("cutoff", 0.6f).to ("volume", 0.1f); break;
        case lSpicc: case lPizz: p.mod.lfo[1] = drift (0.35f, 0.13f, 0.2f, 0.1f); break;
        default:      p.mod.lfo[1] = breathing (0.5f, 0.08f, 0.35f, 0.08f); break;
    }
    p.fx = c.build();
    p.finish = 0.07f;
    for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; }
    return pl;
}
void cinematicMacros (VaultPreset& p, const Role& r)
{
    Chain c (p.fx);
    // the lowest bowed layer (the basses' knob)
    int low = -1;
    for (int k = 0; k < (int) p.parts.size(); ++k)
    {
        const auto& pt = p.parts[(size_t) k];
        if (isSine (pt)) continue;
        if (pt.kind != VaultPartKind::sample) continue;
        if (low < 0 || pt.octave * 12 + pt.semi < p.parts[(size_t) low].octave * 12 + p.parts[(size_t) low].semi) low = k;
    }
    const bool sh = r.look == lSpicc || r.look == lPizz;
    // 1. the bow / the attack
    if (sh)
    {
        M m (r.look == lPizz ? "Pluck" : "Snap", 0.3f);
        m.fxp (c, FT::transient, "attack", 0.6f).rel ("cutoff", 1.2f);
        setMacro (p, 0, m);
    }
    else if (r.look == lTasto)
    {
        M m ("Tone", 0.4f); m.rel ("cutoff", 2.0f).fxp (c, FT::eq, "tilt", 4.0f); setMacro (p, 0, m);
    }
    else
    {
        M m (r.look == lMarcato ? "Bite" : "Bow", 0.5f);
        m.rel ("cutoff", 1.6f).fxp (c, FT::eq, "tilt", 3.0f).fxp (c, FT::drive, "amount", 0.3f)
         .toMax ("fx.drive.output", c.get (FT::drive, "output"), c.get (FT::drive, "output") - 2.0f);
        if (r.look == lMarcato) m.fxp (c, FT::transient, "attack", 0.5f);
        setMacro (p, 0, m);
    }
    // 2. the hall
    {
        M m (c.get (FT::reverb, "type") == (float) R::room ? "Room" : "Hall", 0.4f);
        m.fxp (c, FT::reverb, "mix", 0.45f).fxp (c, FT::reverb, "decay", 1.2f).fxp (c, FT::reverb, "predelay", 30.0f);
        setMacro (p, 1, m);
    }
    // 3. the style's own gesture
    {
        M m ("Swell", 0.0f);
        if (sh) { m = M ("Weight", 0.5f); m.fxp (c, FT::eq, "low", 8.0f).fxp (c, FT::eq, "mid", 3.0f); }
        else if (r.look == lMarcato) { m = M ("Power", 0.3f); m.fxp (c, FT::drive, "amount", 0.45f).toMax ("fx.drive.output", c.get (FT::drive, "output"), c.get (FT::drive, "output") - 4.0f)
                                                               .fxp (c, FT::compressor, "threshold", 10.0f); }
        else if (r.look == lSwell) { m = M ("Bloom", 0.5f); m.rel ("quick.attack", 0.5f).rel ("quick.release", 0.3f); }
        else if (r.look == lTasto)
        {
            const float rate = p.mod.lfo[1].rate, dep = p.mod.lfo[1].depth;
            m = M ("Drift", 0.0f); m.abs ("lfo2.depth", dep, 1.0f - dep, 0, 1).abs ("lfo2.rate", rate, std::max (0.0f, 0.6f - rate), 0.02f, 2.0f).fxp (c, FT::width, "width", 0.25f);
        }
        else if (r.look == lTrem)
        {
            const float rate = p.mod.lfo[1].rate, dep = p.mod.lfo[1].depth;
            m = M ("Tension", 0.0f); m.abs ("lfo2.depth", dep, 1.0f - dep, 0, 1).abs ("lfo2.rate", rate, std::max (0.0f, 1.8f - rate), 0.02f, 2.0f);
        }
        else m.rel ("quick.attack", 0.42f).rel ("quick.release", 0.22f);
        setMacro (p, 2, m);
    }
    // 4. the low end (the basses / the lowest section's level), or the echo on the ostinatos
    if (sh)
    {
        M m ("Echo", 0.0f); m.fxp (c, FT::delay, "mix", 0.4f).fxp (c, FT::delay, "feedback", 0.3f); setMacro (p, 3, m);
    }
    else if (low > 0)
    {
        const float l = p.parts[(size_t) low].level;
        M m (p.parts[(size_t) low].source == "strings.contrabass" ? "Basses" : "Depth", 0.5f);
        m.abs (("layer" + juce::String (low + 1) + ".level").toRawUTF8(), l, std::min (2.0f * l, 2.0f), 0.0f, 2.0f);
        setMacro (p, 3, m);
    }
    else
    {
        M m ("Depth", 0.5f); m.fxp (c, FT::eq, "low", 8.0f).fxp (c, FT::eq, "mid", 3.0f); setMacro (p, 3, m);
    }
    // performance: mod wheel = vibrato (sustains) / more hall (ostinatos), aftertouch = expression swell, velocity = bow
    // pressure (level against the glue + a brighter bow)
    if (! p.mod.lfo[0].dests.empty()) p.mod.modWheel.push_back ({ "lfo1.depth", 0.8f });
    else p.mod.modWheel.push_back ({ "fx.reverb.mix", 0.25f });
    if (! sh) p.mod.aftertouch.push_back ({ "volume", 0.25f });
    p.mod.velocity.push_back ({ "volume", sh ? 0.85f : 0.45f });
    p.mod.velocity.push_back ({ "cutoff", 0.35f });
}

//------------------------------------------------------------------------------ macro extremes against the comfort rules
juce::String judge (const Probe& x, const Probe& ref, float lin, bool longTail)
{
    if (x.peak127 * lin >= 0.885f && x.peak127 > ref.peak127 * 1.06f) return "clip";
    if (x.s25 > 0.10f && x.s25 > ref.s25 + 0.005f) return "harsh-2-5k";
    if ((x.s6 > 0.03f && x.s6 > ref.s6 + 0.002f) || (x.s8 > 0.01f && x.s8 > ref.s8 + 0.001f)) return "piercing-highs";
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
            if (envInt ("ORCH_MACROLOG", 0) != 0) std::printf ("  narrow %s / %s: %s | %s\n", p.name.toRawUTF8(), mac.name.toRawUTF8(), badLo.toRawUTF8(), badHi.toRawUTF8());
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
        if (envInt ("ORCH_MACROLOG", 0) != 0)
            std::printf ("  macro %s / %s: dist %.2f, loud %.1f / %.1f / %.1f, AM %.2f / %.2f, tail %.1f / %.1f dB\n", p.name.toRawUTF8(), mac.name.toRawUTF8(), dist,
                         lo.loud, ref.loud, hi.loud, amDepth (lo.c5), amDepth (hi.c5), tailRatioDb (lo.c5), tailRatioDb (hi.c5));
        if (dist < 1.0f && std::abs (lo.loud - hi.loud) < 1.0f && std::abs (amDepth (lo.c5) - amDepth (hi.c5)) < 0.4f && ! pitchLfo
            && std::abs (tailRatioDb (lo.c5) - tailRatioDb (hi.c5)) < 2.0f)
            mr.quiet.add (p.name + ": " + mac.name);
    }
    return mr;
}
} // namespace v3
// the ids the shipped bank gave these sounds (the v2 bank as assembled: 608 presets). A v2 sound keeps its id and name;
// a sound the v2 selection picks today that the shipped bank did not have gets the next free id of its category, so an
// id never changes its sound (the v2 selection itself is order-sensitive and moves by a preset or two between builds).
const std::vector<std::pair<const char*, const char*>>& shippedIds()
{
    static const std::vector<std::pair<const char*, const char*>> t {
    { "cv.str.0001", "Violins" }, { "cv.str.0002", "Violin Tremolo" }, { "cv.str.0003", "Violin Pizzicato" }, { "cv.str.0004", "Violin Spiccato" },
    { "cv.str.0005", "Sul G Violin Tremolo" }, { "cv.str.0006", "Sul G Violin Pizzicato" }, { "cv.str.0007", "Violas" },
    { "cv.str.0008", "Viola Tremolo" }, { "cv.str.0009", "Viola Pizzicato" }, { "cv.str.0010", "Viola Spiccato" }, { "cv.str.0011", "High Violas" },
    { "cv.str.0012", "High Viola Tremolo" }, { "cv.str.0013", "High Viola Pizzicato" }, { "cv.str.0014", "High Viola Spiccato" },
    { "cv.str.0018", "Cello Spiccato" }, { "cv.str.0019", "Tenor Cellos" }, { "cv.str.0020", "Tenor Cello Tremolo" },
    { "cv.str.0021", "Tenor Cello Pizzicato" }, { "cv.str.0022", "Tenor Cello Spiccato" }, { "cv.str.0023", "Solo Violin" },
    { "cv.str.0024", "Solo Violin Tremolo" }, { "cv.str.0025", "Solo Violin Pizz" }, { "cv.str.0026", "Solo Violin Spiccato" },
    { "cv.str.0027", "High Solo Violin" }, { "cv.str.0028", "High Solo Violin Tremolo" }, { "cv.str.0029", "High Solo Violin Pizz" },
    { "cv.str.0030", "High Solo Violin Spiccato" }, { "cv.str.0031", "Double Basses" }, { "cv.str.0032", "Double Bass Vibrato" },
    { "cv.str.0033", "Double Bass Tremolo" }, { "cv.str.0035", "Double Bass Spiccato" }, { "cv.str.0036", "High Double Basses" },
    { "cv.str.0037", "High Double Bass Tremolo" }, { "cv.str.0038", "High Double Bass Pizzicato" }, { "cv.str.0039", "High Double Bass Spiccato" },
    { "cv.str.0040", "Full Strings" }, { "cv.str.0041", "High Strings" }, { "cv.str.0042", "Chamber Strings" },
    { "cv.str.0044", "Low String Ostinato" }, { "cv.str.0045", "Violins and Cellos" }, { "cv.str.0046", "Violin Viola Unison" },
    { "cv.str.0047", "Pizzicato Ensemble" }, { "cv.str.0048", "Spiccato Ensemble" }, { "cv.str.0049", "Tremolo Ensemble" },
    { "cv.str.0050", "Soloist and Section" }, { "cv.str.0051", "Warm Tenor Strings" }, { "cv.str.0052", "Open Fifths Strings" },
    { "cv.str.0053", "Divisi Violins" }, { "cv.str.0054", "Trap Pizzicato" }, { "cv.str.0055", "Octave Bow Strokes" },
    { "cv.str.0056", "Shimmer Tremolo" }, { "cv.str.0057", "Hybrid Strings" }, { "cv.str.0058", "String Machine" },
    { "cv.str.0059", "Low String Machine" }, { "cv.str.0060", "Pizz and Tremolo" }, { "cv.str.0061", "Solo Violin Duet" },
    { "cv.str.0062", "Gallery Violin Pizzicato" }, { "cv.str.0063", "Distant Violas" }, { "cv.str.0064", "Faraway Viola Tremolo" },
    { "cv.str.0065", "Distant Viola Spiccato" }, { "cv.str.0066", "Distant High Viola Pizzicato" }, { "cv.str.0067", "Faraway High Viola Spiccato" },
    { "cv.str.0068", "Gallery Cellos" }, { "cv.str.0069", "Distant Cello Tremolo" }, { "cv.str.0070", "Gallery Cello Spiccato" },
    { "cv.str.0071", "Distant Tenor Cellos" }, { "cv.str.0072", "Distant Tenor Cello Spiccato" }, { "cv.str.0073", "Faraway Solo Violin" },
    { "cv.str.0074", "Gallery Solo Violin Tremolo" }, { "cv.str.0075", "Faraway Solo Violin Spiccato" },
    { "cv.str.0076", "Gallery High Solo Violin" }, { "cv.str.0077", "Faraway High Solo Violin Pizz" }, { "cv.str.0078", "Gallery High Strings" },
    { "cv.str.0079", "Distant Chamber Strings" }, { "cv.str.0080", "Gallery Soloist & Section" }, { "cv.str.0081", "Faraway Trap Pizzicato" },
    { "cv.str.0082", "Gallery Octave Bow Strokes" }, { "cv.str.0083", "Gallery Solo Violin Duet" }, { "cv.str.0084", "Swelling Violins" },
    { "cv.str.0085", "Rising Violin Tremolo" }, { "cv.str.0086", "Rising Sul G Violin Tremolo" }, { "cv.str.0087", "Swelling Violas" },
    { "cv.str.0088", "Rising Viola Tremolo" }, { "cv.str.0090", "Swelling Cello Tremolo" }, { "cv.str.0091", "Swelling Tenor Cellos" },
    { "cv.str.0092", "Rising Tenor Cello Tremolo" }, { "cv.str.0093", "Rising Solo Violin" }, { "cv.str.0094", "Blooming Solo Violin Tremolo" },
    { "cv.str.0095", "Blooming High Solo Violin" }, { "cv.str.0096", "Swelling Double Basses" }, { "cv.str.0097", "Blooming Double Bass Tremolo" },
    { "cv.str.0098", "Rising Full Strings" }, { "cv.str.0099", "Blooming High Strings" }, { "cv.str.0100", "Swelling Violins & Cellos" },
    { "cv.str.0101", "Rising Tremolo Ensemble" }, { "cv.str.0102", "Blooming Soloist & Section" }, { "cv.str.0103", "Swelling Divisi Violins" },
    { "cv.str.0104", "Swelling Shimmer Tremolo" }, { "cv.str.0105", "Blooming String Machine" }, { "cv.str.0106", "Swelling Low String Machine" },
    { "cv.str.0107", "Blooming Solo Violin Duet" }, { "cv.str.0108", "Muted Violins" }, { "cv.str.0109", "Sordino Violin Tremolo" },
    { "cv.str.0110", "Veiled Violin Pizzicato" }, { "cv.str.0111", "Muted Violin Spiccato" }, { "cv.str.0112", "Sordino Sul G Violin Tremolo" },
    { "cv.str.0113", "Veiled Sul G Violin Pizzicato" }, { "cv.str.0114", "Muted Violas" }, { "cv.str.0115", "Sordino Viola Tremolo" },
    { "cv.str.0116", "Veiled Viola Pizzicato" }, { "cv.str.0117", "Muted Viola Spiccato" }, { "cv.str.0118", "Sordino High Viola Spiccato" },
    { "cv.str.0122", "Veiled Cello Spiccato" }, { "cv.str.0123", "Muted Tenor Cellos" }, { "cv.str.0124", "Sordino Tenor Cello Tremolo" },
    { "cv.str.0125", "Veiled Tenor Cello Pizzicato" }, { "cv.str.0126", "Muted Tenor Cello Spiccato" }, { "cv.str.0127", "Sordino Solo Violin" },
    { "cv.str.0128", "Veiled High Solo Violin" }, { "cv.str.0129", "Muted Double Basses" }, { "cv.str.0130", "Veiled Double Bass Tremolo" },
    { "cv.str.0131", "Veiled High Double Basses" }, { "cv.str.0132", "Muted High Double Bass Tremolo" }, { "cv.str.0133", "Veiled High Strings" },
    { "cv.str.0134", "Veiled Pizzicato Ensemble" }, { "cv.str.0135", "Muted Spiccato Ensemble" }, { "cv.str.0136", "Veiled Soloist & Section" },
    { "cv.str.0137", "Muted Divisi Violins" }, { "cv.str.0138", "Veiled Octave Bow Strokes" }, { "cv.str.0139", "Muted Shimmer Tremolo" },
    { "cv.str.0140", "Veiled Solo Violin Duet" }, { "cv.str.0141", "Stab Violas" }, { "cv.str.0142", "Stab Tenor Cellos" },
    { "cv.str.0143", "Hit Solo Violin" }, { "cv.str.0144", "Punch High Solo Violin" }, { "cv.str.0145", "Hit Double Bass Vibrato" },
    { "cv.str.0146", "Hit Full Strings" }, { "cv.str.0147", "Stab Chamber Strings" }, { "cv.str.0148", "Hit Cinematic Low Strings" },
    { "cv.str.0149", "Stab Violins & Cellos" }, { "cv.str.0150", "Hit Warm Tenor Strings" }, { "cv.str.0151", "Hit Hybrid Strings" },
    { "cv.str.0152", "Punch String Machine" }, { "cv.str.0153", "Stab Low String Machine" }, { "cv.str.0154", "Punch Solo Violin Duet" },
    { "cv.str.0155", "Snappy Violin Pizzicato" }, { "cv.str.0156", "Snappy Sul G Violin Pizzicato" }, { "cv.str.0158", "Tight High Viola Pizzicato" },
    { "cv.str.0159", "Clipped High Viola Spiccato" }, { "cv.str.0161", "Clipped High Solo Violin Pizz" },
    { "cv.str.0162", "Snappy Low String Ostinato" }, { "cv.str.0163", "Snappy Pizzicato Ensemble" }, { "cv.str.0164", "Clipped Trap Pizzicato" },
    { "cv.str.0165", "Snappy Octave Bow Strokes" }, { "cv.str.0166", "Doubled Cello Bass Unison" }, { "cv.str.0167", "Octave High Viola Spiccato" },
    { "cv.str.0168", "Octave Tenor Cello Spiccato" }, { "cv.str.0169", "Octave High Solo Violin" }, { "cv.str.0170", "Octave High Double Basses" },
    { "cv.str.0171", "Shadow Sul G Violin Pizzicato" }, { "cv.str.0172", "Dark High Viola Pizzicato" }, { "cv.str.0173", "Midnight Solo Violin" },
    { "cv.str.0174", "Shadow Solo Violin Tremolo" }, { "cv.str.0175", "Dark Solo Violin Pizz" }, { "cv.str.0176", "Midnight Solo Violin Spiccato" },
    { "cv.str.0177", "Midnight High Solo Violin Pizz" }, { "cv.str.0178", "Dark High Solo Violin Spiccato" },
    { "cv.str.0179", "Midnight Double Bass Spiccato" }, { "cv.str.0180", "Shadow High Double Basses" },
    { "cv.str.0181", "Dark High Double Bass Tremolo" }, { "cv.str.0182", "Dark High Double Bass Spiccato" },
    { "cv.str.0183", "Rewind Violin Pizzicato" }, { "cv.str.0184", "Reverse Violin Spiccato" }, { "cv.str.0185", "Rewind Sul G Violin Pizzicato" },
    { "cv.str.0186", "Reverse Viola Spiccato" }, { "cv.str.0187", "Reverse High Viola Pizzicato" }, { "cv.str.0188", "Rewind Cello Spiccato" },
    { "cv.str.0189", "Rewind Tenor Cello Pizzicato" }, { "cv.str.0190", "Reverse Tenor Cello Spiccato" },
    { "cv.str.0191", "Reverse Solo Violin Pizz" }, { "cv.str.0192", "Backward Solo Violin Spiccato" },
    { "cv.str.0193", "Backward High Solo Violin Pizz" }, { "cv.str.0194", "Rewind Low String Ostinato" },
    { "cv.str.0195", "Rewind Pizzicato Ensemble" }, { "cv.str.0196", "Backward Trap Pizzicato" }, { "cv.str.0197", "Rewind Octave Bow Strokes" },
    { "cv.str.0198", "Behind Glass High Violas" }, { "cv.str.0199", "Muffled Tenor Cellos" }, { "cv.str.0200", "Underwater Tenor Cello Tremolo" },
    { "cv.str.0201", "Muffled Solo Violin Pizz" }, { "cv.str.0202", "Muffled Solo Violin Spiccato" },
    { "cv.str.0203", "Underwater Double Bass Vibrato" }, { "cv.str.0205", "Muffled Cello Bass Unison" },
    { "cv.str.0206", "Behind Glass Trap Pizzicato" }, { "cv.str.0207", "Behind Glass Pizz & Tremolo" }, { "cv.str.0208", "Bowless Viola Tremolo" },
    { "cv.str.0209", "Tape Sul G Violin Pizzicato" }, { "cv.str.0210", "Saturated Solo Violin" }, { "cv.str.0211", "Warm Tape Solo Violin Tremolo" },
    { "cv.str.0212", "Saturated Solo Violin Spiccato" }, { "cv.str.0213", "Warm Tape High Double Basses" },
    { "cv.str.0214", "Tape High Double Bass Tremolo" }, { "cv.str.0215", "Feathered High Viola Tremolo" },
    { "cv.str.0216", "Feathered Solo Violin Tremolo" }, { "cv.str.0217", "Flautando Chamber Strings" },
    { "cv.str.0218", "Flautando Violins & Cellos" }, { "cv.str.0219", "Whispered Violin Viola Unison" },
    { "cv.str.0220", "Pulsing High Viola Spiccato" }, { "cv.str.0221", "Aurora Violin Viola Unison" },
    { "cv.str.0222", "Ethereal Open Fifths Strings" }, { "cv.str.0223", "Suspended Viola Tremolo" }, { "cv.str.0224", "Glacial High Viola Tremolo" },
    { "cv.str.0225", "Glacial Cellos" }, { "cv.str.0226", "Frozen Cello Tremolo" }, { "cv.str.0227", "Frozen Tenor Cellos" },
    { "cv.str.0228", "Suspended Solo Violin" }, { "cv.str.0229", "Glacial High Solo Violin" }, { "cv.str.0230", "Frozen Double Basses" },
    { "cv.str.0232", "Glacial High Strings" }, { "cv.str.0233", "Suspended Pizz & Tremolo" }, { "cv.str.0234", "Glacial Solo Violin Duet" },
    { "cv.str.0235", "Crushed Violins" }, { "cv.str.0236", "Crushed Violin Spiccato" }, { "cv.str.0237", "Crushed Violas" },
    { "cv.str.0238", "Crushed Viola Spiccato" }, { "cv.str.0239", "Grainy High Violas" }, { "cv.str.0240", "Crushed High Viola Pizzicato" },
    { "cv.str.0241", "Grainy High Viola Spiccato" }, { "cv.str.0242", "Broken Cellos" }, { "cv.str.0243", "Crushed Tenor Cellos" },
    { "cv.str.0244", "Grainy Solo Violin" }, { "cv.str.0245", "Broken High Solo Violin" }, { "cv.str.0246", "Grainy High Solo Violin Pizz" },
    { "cv.str.0247", "Crushed Double Bass Pizzicato" }, { "cv.str.0248", "Grainy Full Strings" }, { "cv.str.0249", "Broken High Strings" },
    { "cv.str.0250", "Crushed Chamber Strings" }, { "cv.str.0251", "Crushed Violins & Cellos" }, { "cv.str.0252", "Crushed Spiccato Ensemble" },
    { "cv.str.0253", "Broken Soloist & Section" }, { "cv.str.0254", "Crushed Divisi Violins" }, { "cv.str.0255", "Grainy Trap Pizzicato" },
    { "cv.str.0256", "Broken Octave Bow Strokes" }, { "cv.str.0257", "Broken String Machine" }, { "cv.str.0258", "Broken Solo Violin Duet" },
    { "cv.brass.0001", "Trumpet" }, { "cv.brass.0002", "Trumpet Staccato" }, { "cv.brass.0003", "Harmon Trumpet" },
    { "cv.brass.0004", "High Trumpet" }, { "cv.brass.0005", "High Trumpet Staccato" }, { "cv.brass.0006", "French Horn" },
    { "cv.brass.0007", "Horn Staccato" }, { "cv.brass.0008", "Low French Horn" }, { "cv.brass.0009", "Trombone" },
    { "cv.brass.0010", "Vibrato Trombone" }, { "cv.brass.0011", "Trombone Staccato" }, { "cv.brass.0012", "Low Vibrato Trombone" },
    { "cv.brass.0013", "Low Trombone Staccato" }, { "cv.brass.0014", "Vintage Trombone" }, { "cv.brass.0015", "Vintage Trombone Short" },
    { "cv.brass.0016", "Low Vintage Trombone" }, { "cv.brass.0017", "Low Vintage Trombone Short" }, { "cv.brass.0018", "Low Buzzing Trombone" },
    { "cv.brass.0019", "Concert Tuba" }, { "cv.brass.0021", "High Tuba" }, { "cv.brass.0022", "High Tuba Staccato" },
    { "cv.brass.0023", "Trumpet Section" }, { "cv.brass.0024", "Harmon Section" }, { "cv.brass.0026", "Stopped Horn Section" },
    { "cv.brass.0027", "Trombone Section Stabs" }, { "cv.brass.0028", "Trombone Section Falls" }, { "cv.brass.0029", "Full Brass" },
    { "cv.brass.0030", "Low Brass Choir" }, { "cv.brass.0031", "Brass Stab Ensemble" }, { "cv.brass.0032", "Epic Horns & Trombones" },
    { "cv.brass.0033", "Trumpet and Horn" }, { "cv.brass.0034", "Hybrid Trap Brass" }, { "cv.brass.0035", "Muted Brass Choir" },
    { "cv.brass.0036", "Fanfare Brass" }, { "cv.brass.0037", "Synth Brass" }, { "cv.brass.0038", "Synth Brass Stab" },
    { "cv.brass.0039", "Hybrid Horns" }, { "cv.brass.0040", "Trombone and Tuba" }, { "cv.brass.0041", "Low Brass Stabs" },
    { "cv.brass.0042", "Soul Horn Section" }, { "cv.brass.0043", "Pop Horn Section" }, { "cv.brass.0044", "Distant Trumpet" },
    { "cv.brass.0045", "Distant Horn Staccato" }, { "cv.brass.0046", "Gallery Trumpet Section" }, { "cv.brass.0047", "Faraway Stopped Horn Section" },
    { "cv.brass.0048", "Distant Full Brass" }, { "cv.brass.0049", "Gallery Brass Stab Ensemble" }, { "cv.brass.0050", "Faraway Trumpet & Horn" },
    { "cv.brass.0051", "Faraway Fanfare Brass" }, { "cv.brass.0052", "Faraway Low Brass Stabs" }, { "cv.brass.0053", "Faraway Pop Horn Section" },
    { "cv.brass.0054", "Swelling Trumpet" }, { "cv.brass.0055", "Blooming Harmon Trumpet" }, { "cv.brass.0056", "Swelling High Trumpet" },
    { "cv.brass.0057", "Rising Vintage Trombone" }, { "cv.brass.0058", "Rising Low Vintage Trombone" },
    { "cv.brass.0059", "Blooming Trumpet Section" }, { "cv.brass.0060", "Swelling Low Horn Section" },
    { "cv.brass.0061", "Rising Stopped Horn Section" }, { "cv.brass.0062", "Swelling Full Brass" }, { "cv.brass.0063", "Rising Trumpet & Horn" },
    { "cv.brass.0064", "Swelling Muted Brass Choir" }, { "cv.brass.0065", "Rising Fanfare Brass" }, { "cv.brass.0066", "Blooming Horns in Octaves" },
    { "cv.brass.0067", "Swelling Trombone & Tuba" }, { "cv.brass.0068", "Rising Pop Horn Section" }, { "cv.brass.0069", "Stab Trumpet" },
    { "cv.brass.0070", "Stab High Trumpet" }, { "cv.brass.0071", "Punch French Horn" }, { "cv.brass.0072", "Hit Low French Horn" },
    { "cv.brass.0073", "Punch Trombone" }, { "cv.brass.0074", "Hit Vintage Trombone" }, { "cv.brass.0075", "Hit Low Vintage Trombone" },
    { "cv.brass.0076", "Hit Concert Tuba" }, { "cv.brass.0077", "Punch Trumpet Section" }, { "cv.brass.0078", "Stab Low Horn Section" },
    { "cv.brass.0079", "Hit Stopped Horn Section" }, { "cv.brass.0080", "Stab Full Brass" }, { "cv.brass.0081", "Stab Epic Horns & Trombones" },
    { "cv.brass.0082", "Hit Fanfare Brass" }, { "cv.brass.0083", "Punch Synth Brass" }, { "cv.brass.0084", "Hit Hybrid Horns" },
    { "cv.brass.0085", "Hit Pop Horn Section" }, { "cv.brass.0086", "Clipped High Trumpet Staccato" }, { "cv.brass.0087", "Tight Horn Staccato" },
    { "cv.brass.0088", "Clipped Trombone Staccato" }, { "cv.brass.0089", "Tight Low Trombone Staccato" },
    { "cv.brass.0090", "Tight Vintage Trombone Short" }, { "cv.brass.0091", "Tight Low Buzzing Trombone" },
    { "cv.brass.0092", "Snappy Low Trombone Stabs" }, { "cv.brass.0093", "Snappy Brass Stab Ensemble" },
    { "cv.brass.0094", "Snappy Hybrid Trap Brass" }, { "cv.brass.0095", "Clipped Low Brass Stabs" }, { "cv.brass.0096", "Tight Soul Horn Section" },
    { "cv.brass.0097", "Twin Crooning Trombone" }, { "cv.brass.0098", "Doubled Low Buzzing Trombone" }, { "cv.brass.0099", "Octave High Trumpet" },
    { "cv.brass.0100", "Octave High Trumpet Staccato" }, { "cv.brass.0101", "Octave Doubled Trombone" },
    { "cv.brass.0102", "Octave Vibrato Trombone" }, { "cv.brass.0103", "Deep Octave Trombone Staccato" },
    { "cv.brass.0104", "Deep Octave Vintage Trombone" }, { "cv.brass.0105", "Octave Crooning Trombone" },
    { "cv.brass.0106", "Octave Vintage Trombone Short" }, { "cv.brass.0107", "Octave High Tuba" },
    { "cv.brass.0108", "Deep Octave High Tuba Staccato" }, { "cv.brass.0109", "Octave Doubled Synth Brass" },
    { "cv.brass.0110", "Octave Synth Brass Stab" }, { "cv.brass.0111", "Deep Octave Hybrid Horns" }, { "cv.brass.0112", "Dark Trumpet" },
    { "cv.brass.0113", "Midnight Trumpet Staccato" }, { "cv.brass.0114", "Shadow Harmon Trumpet" }, { "cv.brass.0115", "Shadow Trombone" },
    { "cv.brass.0116", "Midnight Trombone Staccato" }, { "cv.brass.0117", "Shadow Low Vibrato Trombone" },
    { "cv.brass.0118", "Dark Low Trombone Staccato" }, { "cv.brass.0119", "Midnight Vintage Trombone" },
    { "cv.brass.0120", "Dark Vintage Trombone Short" }, { "cv.brass.0121", "Midnight Low Vintage Trombone" },
    { "cv.brass.0122", "Dark Low Buzzing Trombone" }, { "cv.brass.0123", "Dark High Tuba" }, { "cv.brass.0124", "Midnight High Tuba Staccato" },
    { "cv.brass.0125", "Shadow Trumpet Section" }, { "cv.brass.0126", "Midnight Stopped Horn Section" },
    { "cv.brass.0127", "Shadow Horn Section Stabs" }, { "cv.brass.0128", "Dark Trombone Section Falls" }, { "cv.brass.0129", "Shadow Tuba Pair" },
    { "cv.brass.0130", "Dark Full Brass" }, { "cv.brass.0131", "Shadow Brass Stab Ensemble" }, { "cv.brass.0132", "Midnight Trumpet & Horn" },
    { "cv.brass.0133", "Shadow Hybrid Trap Brass" }, { "cv.brass.0134", "Dark Muted Brass Choir" }, { "cv.brass.0135", "Dark Trombone and Tuba" },
    { "cv.brass.0136", "Dark Soul Horn Section" }, { "cv.brass.0137", "Midnight Pop Horn Section" },
    { "cv.brass.0138", "Backward High Trumpet Staccato" }, { "cv.brass.0139", "Reverse Horn Staccato" },
    { "cv.brass.0140", "Backward Trombone Staccato" }, { "cv.brass.0141", "Reverse Low Trombone Staccato" },
    { "cv.brass.0142", "Reverse Vintage Trombone Short" }, { "cv.brass.0143", "Reverse Trombone Section Falls" },
    { "cv.brass.0144", "Rewind Brass Stab Ensemble" }, { "cv.brass.0145", "Reverse Synth Brass Stab" },
    { "cv.brass.0146", "Backward Low Brass Stabs" }, { "cv.brass.0147", "Reverse Soul Horn Section" }, { "cv.brass.0148", "Muffled Trumpet" },
    { "cv.brass.0149", "Underwater Harmon Trumpet" }, { "cv.brass.0150", "Muffled High Trumpet" }, { "cv.brass.0151", "Underwater French Horn" },
    { "cv.brass.0152", "Behind Glass Trombone Staccato" }, { "cv.brass.0153", "Muffled Low Trombone Staccato" },
    { "cv.brass.0154", "Behind Glass Vintage Trombone" }, { "cv.brass.0155", "Muffled Low Vintage Trombone" },
    { "cv.brass.0156", "Muffled Concert Tuba Staccato" }, { "cv.brass.0157", "Muffled High Tuba" }, { "cv.brass.0158", "Underwater Trumpet Section" },
    { "cv.brass.0159", "Muffled Stopped Horn Section" }, { "cv.brass.0160", "Muffled Trombone Section Stabs" },
    { "cv.brass.0161", "Muffled Trombone Section Falls" }, { "cv.brass.0162", "Underwater Tuba Pair" }, { "cv.brass.0163", "Muffled Full Brass" },
    { "cv.brass.0164", "Behind Glass Trumpet & Horn" }, { "cv.brass.0165", "Muffled Muted Brass Choir" },
    { "cv.brass.0166", "Behind Glass Fanfare Brass" }, { "cv.brass.0167", "Underwater Horns in Octaves" },
    { "cv.brass.0168", "Muffled Trombone & Tuba" }, { "cv.brass.0169", "Muffled Soul Horn Section" }, { "cv.brass.0170", "Tape High Trumpet" },
    { "cv.brass.0171", "Warm Tape Low Vibrato Trombone" }, { "cv.brass.0172", "Warm Tape Crooning Trombone" },
    { "cv.brass.0173", "Pulsing High Trumpet Staccato" }, { "cv.brass.0174", "Repeating Low Trombone Stabs" }, { "cv.brass.0175", "Frozen Trumpet" },
    { "cv.brass.0176", "Glacial French Horn" }, { "cv.brass.0177", "Suspended Low French Horn" }, { "cv.brass.0178", "Frozen Vibrato Trombone" },
    { "cv.brass.0179", "Suspended Vintage Trombone" }, { "cv.brass.0180", "Frozen High Tuba" }, { "cv.brass.0181", "Glacial Trumpet Section" },
    { "cv.brass.0182", "Suspended Harmon Section" }, { "cv.brass.0183", "Suspended Stopped Horn Section" },
    { "cv.brass.0184", "Frozen Trombone Section" }, { "cv.brass.0185", "Frozen Full Brass" }, { "cv.brass.0186", "Suspended Trumpet & Horn" },
    { "cv.brass.0187", "Suspended Fanfare Brass" }, { "cv.brass.0188", "Suspended Hybrid Horns" }, { "cv.brass.0189", "Crushed Trumpet" },
    { "cv.brass.0190", "Grainy Trumpet Staccato" }, { "cv.brass.0191", "Broken Harmon Trumpet" }, { "cv.brass.0192", "Crushed High Trumpet" },
    { "cv.brass.0193", "Grainy High Trumpet Staccato" }, { "cv.brass.0194", "Grainy Trombone Staccato" },
    { "cv.brass.0195", "Broken Low Vibrato Trombone" }, { "cv.brass.0196", "Crushed Low Trombone Staccato" },
    { "cv.brass.0197", "Grainy Vintage Trombone" }, { "cv.brass.0198", "Grainy Low Vintage Trombone" },
    { "cv.brass.0199", "Crushed Low Buzzing Trombone" }, { "cv.brass.0200", "Broken Trumpet Section" },
    { "cv.brass.0201", "Grainy Stopped Horn Section" }, { "cv.brass.0202", "Crushed Trombone Section Falls" },
    { "cv.brass.0203", "Broken Tuba Pair" }, { "cv.brass.0204", "Crushed Full Brass" }, { "cv.brass.0205", "Grainy Trumpet & Horn" },
    { "cv.brass.0206", "Broken Hybrid Trap Brass" }, { "cv.brass.0207", "Crushed Muted Brass Choir" }, { "cv.brass.0208", "Grainy Fanfare Brass" },
    { "cv.brass.0209", "Broken Synth Brass" }, { "cv.brass.0210", "Crushed Synth Brass Stab" }, { "cv.brass.0211", "Crushed Trombone & Tuba" },
    { "cv.brass.0212", "Crushed Soul Horn Section" }, { "cv.brass.0213", "Grainy Pop Horn Section" },
    { "cv.brass.0214", "Ricochet Low Trombone Stabs" }, { "cv.orch.0001", "Timpani" }, { "cv.orch.0003", "Timpani Roll" },
    { "cv.orch.0004", "Low Timpani" }, { "cv.orch.0006", "Low Orchestral Harp" }, { "cv.orch.0007", "Flute and Violins" },
    { "cv.orch.0008", "Clarinet and Violas" }, { "cv.orch.0009", "Oboe and Violins" }, { "cv.orch.0010", "Bassoon and Cellos" },
    { "cv.orch.0011", "Woodwind Quartet" }, { "cv.orch.0012", "Pastoral Winds" }, { "cv.orch.0013", "Woodwind Quartet Staccato" },
    { "cv.orch.0014", "Winds and Strings" }, { "cv.orch.0015", "Orchestral Tutti" }, { "cv.orch.0016", "Grand Tutti" },
    { "cv.orch.0017", "Tutti Swell" }, { "cv.orch.0018", "Orchestra Hit" }, { "cv.orch.0019", "Classic Orchestra Stab" },
    { "cv.orch.0020", "Horns and Strings" }, { "cv.orch.0021", "Thunder Strings" }, { "cv.orch.0022", "War Drum Brass" },
    { "cv.orch.0023", "Fairy Tale Pizzicato" }, { "cv.orch.0024", "Strings and Choir" }, { "cv.orch.0025", "Harp and Flute" },
    { "cv.orch.0026", "Clockwork Winds" }, { "cv.orch.0027", "Oboe and Harp" }, { "cv.orch.0028", "Marimba and Pizzicato" },
    { "cv.orch.0029", "Warm Horn and Bassoon" }, { "cv.orch.0030", "Brass & Strings Swell" }, { "cv.orch.0031", "Flute & Clarinet Duet" },
    { "cv.orch.0032", "Choir and Brass" }, { "cv.orch.0033", "Piano and Strings" }, { "cv.orch.0034", "Organ and Strings" },
    { "cv.orch.0035", "Glass and Strings" }, { "cv.orch.0036", "Heavenly Strings" }, { "cv.orch.0037", "Hybrid Orchestra" },
    { "cv.orch.0038", "Distant Clarinet & Violas" }, { "cv.orch.0039", "Faraway Oboe & Violins" }, { "cv.orch.0040", "Distant Woodwind Quartet" },
    { "cv.orch.0041", "Distant Tutti Swell" }, { "cv.orch.0042", "Gallery War Drum Brass" }, { "cv.orch.0043", "Faraway Fairy Tale Pizzicato" },
    { "cv.orch.0044", "Gallery Strings & Choir" }, { "cv.orch.0045", "Faraway Clockwork Winds" }, { "cv.orch.0046", "Faraway Timpani & Contrabass" },
    { "cv.orch.0047", "Gallery Choir & Brass" }, { "cv.orch.0048", "Faraway Piano & Strings" }, { "cv.orch.0049", "Distant Glass & Strings" },
    { "cv.orch.0050", "Faraway Heavenly Strings" }, { "cv.orch.0051", "Swelling Clarinet & Violas" }, { "cv.orch.0052", "Rising Oboe & Violins" },
    { "cv.orch.0053", "Blooming Bassoon & Cellos" }, { "cv.orch.0054", "Swelling Woodwind Quartet" }, { "cv.orch.0055", "Swelling Winds & Strings" },
    { "cv.orch.0056", "Rising Orchestral Tutti" }, { "cv.orch.0057", "Swelling Horns & Strings" }, { "cv.orch.0058", "Blooming Strings & Choir" },
    { "cv.orch.0059", "Blooming Choir & Brass" }, { "cv.orch.0060", "Swelling Piccolo & Strings" }, { "cv.orch.0061", "Swelling Glass & Strings" },
    { "cv.orch.0062", "Rising Heavenly Strings" }, { "cv.orch.0063", "Muted Clarinet & Violas" }, { "cv.orch.0064", "Sordino Oboe & Violins" },
    { "cv.orch.0065", "Veiled Bassoon & Cellos" }, { "cv.orch.0066", "Muted Woodwind Quartet" }, { "cv.orch.0067", "Sordino Pastoral Winds" },
    { "cv.orch.0068", "Muted Winds & Strings" }, { "cv.orch.0069", "Sordino Orchestra Hit" }, { "cv.orch.0070", "Muted Horns & Strings" },
    { "cv.orch.0071", "Sordino Fairy Tale Pizzicato" }, { "cv.orch.0072", "Sordino Clockwork Winds" },
    { "cv.orch.0073", "Sordino Marimba & Pizzicato" }, { "cv.orch.0074", "Veiled Warm Horn & Bassoon" },
    { "cv.orch.0075", "Veiled Brass & Strings Swell" }, { "cv.orch.0076", "Muted Flute & Clarinet Duet" },
    { "cv.orch.0077", "Muted Glass & Strings" }, { "cv.orch.0078", "Sordino Heavenly Strings" }, { "cv.orch.0079", "Stab Clarinet & Violas" },
    { "cv.orch.0080", "Hit Oboe and Violins" }, { "cv.orch.0081", "Punch Bassoon & Cellos" }, { "cv.orch.0082", "Punch Grand Tutti" },
    { "cv.orch.0083", "Punch Strings & Choir" }, { "cv.orch.0084", "Punch Warm Horn & Bassoon" }, { "cv.orch.0085", "Punch Choir and Brass" },
    { "cv.orch.0086", "Hit Heavenly Strings" }, { "cv.orch.0087", "Snappy Timpani" }, { "cv.orch.0088", "Tight Soft Timpani" },
    { "cv.orch.0089", "Snappy Low Timpani" }, { "cv.orch.0090", "Tight Orchestral Harp" }, { "cv.orch.0091", "Clipped Low Orchestral Harp" },
    { "cv.orch.0092", "Clipped Orchestra Hit" }, { "cv.orch.0093", "Snappy War Drum Brass" }, { "cv.orch.0094", "Clipped Fairy Tale Pizzicato" },
    { "cv.orch.0095", "Clipped Clockwork Winds" }, { "cv.orch.0097", "Snappy Strings & Timpani Hit" }, { "cv.orch.0098", "Clipped Piano & Strings" },
    { "cv.orch.0099", "Doubled Soft Timpani" }, { "cv.orch.0100", "Twin Low Timpani" }, { "cv.orch.0101", "Octave Soft Timpani" },
    { "cv.orch.0102", "Deep Octave Timpani Roll" }, { "cv.orch.0103", "Octave Orchestral Harp" }, { "cv.orch.0104", "Octave Harp and Flute" },
    { "cv.orch.0105", "Dark Soft Timpani" }, { "cv.orch.0106", "Midnight Timpani Roll" }, { "cv.orch.0107", "Shadow Low Timpani" },
    { "cv.orch.0109", "Midnight Pastoral Winds" }, { "cv.orch.0110", "Dark Woodwind Quartet Staccato" }, { "cv.orch.0111", "Shadow Oboe and Harp" },
    { "cv.orch.0112", "Midnight Piano & Strings" }, { "cv.orch.0114", "Backward Orchestra Hit" }, { "cv.orch.0115", "Rewind War Drum Brass" },
    { "cv.orch.0116", "Backward Fairy Tale Pizzicato" }, { "cv.orch.0117", "Backward Clockwork Winds" },
    { "cv.orch.0118", "Backward Marimba & Pizzicato" }, { "cv.orch.0119", "Underwater Timpani" }, { "cv.orch.0121", "Behind Glass Timpani Roll" },
    { "cv.orch.0122", "Underwater Low Timpani" }, { "cv.orch.0123", "Underwater Low Orchestral Harp" },
    { "cv.orch.0124", "Behind Glass Thunder Strings" }, { "cv.orch.0125", "Underwater Strings & Choir" },
    { "cv.orch.0126", "Behind Glass Clockwork Winds" }, { "cv.orch.0127", "Underwater Oboe & Harp" }, { "cv.orch.0128", "Muffled Hybrid Orchestra" },
    { "cv.orch.0129", "Smooth Brass & Strings Swell" }, { "cv.orch.0130", "Saturated Pastoral Winds" }, { "cv.orch.0131", "Ethereal Timpani" },
    { "cv.orch.0132", "Ethereal Grand Tutti" }, { "cv.orch.0133", "Ethereal Oboe and Harp" }, { "cv.orch.0134", "Suspended Timpani Roll" },
    { "cv.orch.0135", "Frozen Clarinet & Violas" }, { "cv.orch.0136", "Suspended Oboe & Violins" }, { "cv.orch.0137", "Frozen Woodwind Quartet" },
    { "cv.orch.0138", "Suspended Pastoral Winds" }, { "cv.orch.0139", "Frozen Winds & Strings" }, { "cv.orch.0140", "Suspended Orchestral Tutti" },
    { "cv.orch.0141", "Glacial Strings & Choir" }, { "cv.orch.0142", "Frozen Flute & Clarinet Duet" }, { "cv.orch.0143", "Frozen Glass & Strings" },
    { "cv.orch.0144", "Broken Flute & Violins" }, { "cv.orch.0145", "Crushed Clarinet & Violas" }, { "cv.orch.0146", "Grainy Oboe & Violins" },
    { "cv.orch.0147", "Broken Bassoon & Cellos" }, { "cv.orch.0148", "Crushed Woodwind Quartet" }, { "cv.orch.0149", "Crushed Winds & Strings" },
    { "cv.orch.0150", "Grainy Orchestral Tutti" }, { "cv.orch.0151", "Crushed Horns & Strings" }, { "cv.orch.0152", "Grainy Fairy Tale Pizzicato" },
    { "cv.orch.0153", "Broken Strings & Choir" }, { "cv.orch.0154", "Grainy Marimba & Pizzicato" },
    { "cv.orch.0155", "Grainy Strings & Timpani Hit" }, { "cv.orch.0156", "Crushed Glass & Strings" }, { "cv.orch.0157", "Canyon Clockwork Winds" },
    };
    return t;
}
juce::String nameKey (const juce::String& n) { return n.replace (" & ", " and ").toLowerCase(); }
// gives every preset its shipped id (by name) or the next free one, then sorts in id order (the shipped browser order,
// new sounds after it)
void assignShippedIds (std::vector<VaultPreset>& sel, std::vector<v3::Role>& roles)
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
    std::vector<VaultPreset> s2; std::vector<v3::Role> r2;
    for (auto i : order) { s2.push_back (sel[i]); r2.push_back (roles[i]); }
    sel.swap (s2); roles.swap (r2);
}

// the few presets where a rhythm is the point of the sound (arp / gate on, plain musical names; appended after the v2
// selection so the ids of the v2 presets stay)
void addPatterns (std::vector<Base>& b)
{
    const KV sus { { "s.release", 0.45f }, { "s.width", 0.6f }, { "s.velCutoff", 0.3f } };
    const KV sh { { "s.width", 0.6f }, { "s.velCutoff", 0.3f } };
    auto X = [&] (const char* id, const char* art, const char* label, int o, float l, float pan, const KV& kv) { return lvl (S (id, art, label, o, kv), l, pan); };
    auto add = [&] (const char* noun, int cat, int fam, int kind, std::vector<VaultPart> parts, int pattern)
    {
        Base x { noun, cat, fam, kind, std::move (parts) };
        x.pattern = pattern;
        b.push_back (std::move (x));
    };
    add ("Pizzicato Arpeggio", vault::catStrings, fStr, kShort, { X ("strings.violas.ensemble", "pizzicato", "Viola Section", 0, 0.8f, -0.2f, sh),
                                                                  X ("strings.cellos.ensemble", "pizzicato", "Cello Section", -1, 0.4f, 0.3f, sh) }, 1);
    add ("Spiccato Ostinato", vault::catStrings, fStr, kShort, { X ("strings.cellos.ensemble", "spiccato", "Cello Section", -1, 0.8f, -0.3f, sh),
                                                                 X ("strings.contrabass", "spiccato", "Contrabass", -2, 0.6f, 0.3f, sh) }, 2);
    add ("Gated Strings", vault::catStrings, fStr, kSus, { X ("strings.violins.ensemble", "sustain", "Violin Section", 0, 0.7f, -0.35f, sus),
                                                          X ("strings.violas.ensemble", "sustain", "Viola Section", 0, 0.55f, 0.15f, sus),
                                                          X ("strings.cellos.ensemble", "sustain", "Cello Section", -1, 0.5f, 0.35f, sus) }, 3);
    add ("Horn Pulse Eighths", vault::catBrass, fBrs, kSus, section ("brass.horn", "sustain", "French Horn", 0, 3, { { "s.release", 0.3f }, { "s.velCutoff", 0.35f } }), 4);
    add ("Woodwind Arpeggio", vault::catOrchestral, fOrc, kShort, { X ("winds.clarinet", "staccato", "Clarinet", 0, 0.75f, -0.2f, sh),
                                                                    X ("winds.flute.concert", "staccato", "Concert Flute", 0, 0.45f, 0.25f, sh) }, 5);
}

// development: the other recipes' presets of a finished bank as their -cache files, so `VaultBankGen -recipe all -cache <dir>
// -rebuild orch` re-assembles the bank around a rebuilt orch without rebuilding the other recipes (ORCH_SPLIT=<bank>,<dir>)
bool splitBank (const juce::String& spec)
{
    const auto bankPath = spec.upToFirstOccurrenceOf (",", false, false), dirPath = spec.fromFirstOccurrenceOf (",", false, false);
    std::vector<VaultPreset> bank;
    if (! vault::loadBankFile (juce::File (bankPath), bank)) { std::printf ("orch split: cannot load %s\n", bankPath.toRawUTF8()); return false; }
    const juce::File dir (dirPath);
    dir.createDirectory();
    const std::map<juce::String, juce::String> recipeOf { { "piano", "keys" }, { "keys", "keys" }, { "gtr", "strings" }, { "bass", "strings" }, { "wind", "winds" },
                                                          { "world", "winds" }, { "bell", "perc-voc" }, { "vox", "perc-voc" }, { "pluck", "synth" }, { "lead", "synth" },
                                                          { "pad", "synth" }, { "synth", "synth" }, { "fx", "synth" },
                                                          { "str", "orch" }, { "brass", "orch" }, { "orch", "orch" } };   // orch.cvb: with -rebuild orch it is rebuilt anyway
    std::map<juce::String, std::vector<VaultPreset>> out;
    for (auto& p : bank)
    {
        const auto slug = p.id.fromFirstOccurrenceOf ("cv.", false, false).upToFirstOccurrenceOf (".", false, false);
        if (auto it = recipeOf.find (slug); it != recipeOf.end()) out[it->second].push_back (p);
    }
    for (auto& [name, v] : out)
    {
        vault::saveBankFile (dir.getChildFile (name + ".cvb"), v);
        std::printf ("orch split: %s.cvb %d presets\n", name.toRawUTF8(), (int) v.size());
    }
    return true;
}

// development: replace only this recipe's presets in a shipped bank (ORCH_MERGE=<shipped.cvb>,<assembled.cvb>,<out.cvb>):
// Strings / Brass / Orchestral (ids cv.str / cv.brass / cv.orch) come from a bank the full assembly wrote around a rebuilt
// orch (-cache with the ORCH_SPLIT files, -rebuild orch), every other recipe's preset stays exactly as shipped
bool mergeBank (const juce::String& spec)
{
    const auto f = juce::StringArray::fromTokens (spec, ",", "");
    if (f.size() < 3) return false;
    std::vector<VaultPreset> shipped, made, out;
    if (! vault::loadBankFile (juce::File (f[0]), shipped) || ! vault::loadBankFile (juce::File (f[1]), made)) { std::printf ("orch merge: cannot load\n"); return false; }
    auto ours = [] (const VaultPreset& p) { return p.id.startsWith ("cv.str.") || p.id.startsWith ("cv.brass.") || p.id.startsWith ("cv.orch."); };
    std::set<int> done;
    for (auto& p : shipped)
    {
        if (! ours (p)) { out.push_back (p); continue; }
        if (! done.insert (p.category).second) continue;
        for (auto& q : made) if (ours (q) && q.category == p.category) out.push_back (q);
    }
    for (auto& q : made)   // a category the shipped bank had no orch preset in
        if (ours (q) && done.count (q.category) == 0) out.push_back (q);
    vault::saveBankFile (juce::File (f[2]), out);
    std::printf ("orch merge: %d shipped + %d assembled -> %d presets\n", (int) shipped.size(), (int) made.size(), (int) out.size());
    return true;
}

// development: the owner's demos of the cinematic strings (ORCH_DEMO=<bank>|<dir>|<name or id,...>): a low-register
// chord progression (C minor - A flat - F minor - G, keys C4..F5 so the stack sounds C2..F4), then the tonic held as a
// swell (expression from soft to full through aftertouch, vibrato brought in on the mod wheel); short articulations play
// the chords as an eighth-note ostinato that crescendos. 24-bit WAV + OGG per preset, at the preset's bank gain.
bool renderDemos (const juce::String& spec)
{
    const auto parts = juce::StringArray::fromTokens (spec, "|", "");
    if (parts.size() < 3) return false;
    std::vector<VaultPreset> bank;
    if (! vault::loadBankFile (juce::File (parts[0]), bank)) { std::printf ("orch demo: cannot load %s\n", parts[0].toRawUTF8()); return false; }
    const juce::File dir (parts[1]);
    dir.createDirectory();
    const double sr = recipegate::sr;
    const std::vector<std::vector<int>> chords { { 48, 55, 60, 63 }, { 51, 56, 60, 63 }, { 53, 56, 60, 65 }, { 50, 55, 59, 62 } };
    int n = 0;
    for (auto& want : juce::StringArray::fromTokens (parts[2], ",", ""))
        for (auto& p : bank)
        {
            if (p.id != want && p.name != want) continue;
            bool shortS = true;   // a sound that has died away after a second plays the ostinato
            {
                const auto t = recipegate::render (p, vault::homeNote, 0.8f, 2.0, 2.0);
                const int a = (int) (0.1 * sr), b0 = (int) (1.6 * sr);
                shortS = t.getRMSLevel (0, b0, (int) (0.3 * sr)) < 0.12f * t.getRMSLevel (0, a, (int) (0.3 * sr));
            }
            juce::MidiBuffer mb;
            auto at = [&] (double s) { return (int) std::round (s * sr); };
            const double chordLen = 2.4, swellStart = 4 * chordLen, swellLen = 4.4;
            for (int c = 0; c < 4; ++c)
            {
                const double t0 = c * chordLen;
                if (shortS)
                    for (int e = 0; e < 8; ++e)
                    {
                        const double s = t0 + e * 0.3;
                        const int v = (e % 4 == 0 ? 112 : e % 2 == 0 ? 92 : 80) - (c == 0 ? 6 : 0);
                        for (int k : chords[(size_t) c]) { mb.addEvent (juce::MidiMessage::noteOn (1, k, (juce::uint8) v), at (s)); mb.addEvent (juce::MidiMessage::noteOff (1, k), at (s + 0.2)); }
                    }
                else
                    for (int k : chords[(size_t) c])
                    {
                        mb.addEvent (juce::MidiMessage::noteOn (1, k, (juce::uint8) (c == 0 ? 92 : 100)), at (t0));
                        mb.addEvent (juce::MidiMessage::noteOff (1, k), at (t0 + chordLen - 0.05));
                    }
            }
            const std::vector<int> tonic { 48, 55, 60, 63 };
            if (shortS)
                for (int e = 0; e < 14; ++e)
                {
                    const double s = swellStart + e * 0.3;
                    const int v = juce::jlimit (30, 127, 40 + e * 6);
                    for (int k : tonic) { mb.addEvent (juce::MidiMessage::noteOn (1, k, (juce::uint8) v), at (s)); mb.addEvent (juce::MidiMessage::noteOff (1, k), at (s + 0.2)); }
                }
            else
            {
                for (int k : tonic) { mb.addEvent (juce::MidiMessage::noteOn (1, k, (juce::uint8) 64), at (swellStart)); mb.addEvent (juce::MidiMessage::noteOff (1, k), at (swellStart + swellLen)); }
                for (int s = 0; s <= 40; ++s)   // expression: soft -> full, vibrato in from halfway
                {
                    const double t = swellStart + swellLen * 0.85 * s / 40.0;
                    mb.addEvent (juce::MidiMessage::channelPressureChange (1, juce::jlimit (0, 127, s * 127 / 40)), at (t));
                    mb.addEvent (juce::MidiMessage::controllerEvent (1, 1, juce::jlimit (0, 127, std::max (0, s - 20) * 4)), at (t));
                }
            }
            mb.addEvent (juce::MidiMessage::channelPressureChange (1, 0), at (swellStart + swellLen + 0.5));
            mb.addEvent (juce::MidiMessage::controllerEvent (1, 1, 0), at (swellStart + swellLen + 0.5));
            const auto buf = VaultEngine::renderMidi (p, mb, at (swellStart + swellLen + 1.2), sr);
            const auto base = juce::String (++n).paddedLeft ('0', 2) + "_" + p.name.replaceCharacters (" &/", "__-");
            recipegate::writeWav (dir.getChildFile (base + ".wav"), buf);
            recipegate::writeOgg (dir.getChildFile (base + ".ogg"), buf, 5);
            {   // demos.tsv: what the listening page shows (file, id, name, layers, FX chain, macros)
                juce::StringArray lab, fxs, mac;
                for (auto& pt : p.parts) lab.add (pt.label + (pt.octave != 0 ? juce::String::formatted (" (%+d oct)", pt.octave) : juce::String())
                                                  + (pt.semi != 0 ? juce::String::formatted (" +%d st", pt.semi) : juce::String()));
                for (auto& f : p.fx) if (f.on) fxs.add (vault::fxTypeInfo (f.type).name);
                for (auto& m : p.macros) if (m.name.isNotEmpty()) mac.add (m.name);
                dir.getChildFile ("demos.tsv").appendText (base + ".wav\t" + p.id + "\t" + p.name + "\t" + lab.joinIntoString (" + ") + "\t"
                                                           + fxs.joinIntoString (" > ") + "\t" + mac.joinIntoString (", ") + "\n");
            }
            std::printf ("orch demo: %s %s (%s) peak %.3f\n", base.toRawUTF8(), p.id.toRawUTF8(), shortS ? "ostinato" : "sustained", buf.getMagnitude (0, buf.getNumSamples()));
            break;
        }
    return true;
}
} // namespace

std::vector<VaultPreset> build_orch (const RecipeContext& ctx)
{
    if (const auto s = juce::SystemStats::getEnvironmentVariable ("ORCH_SPLIT", {}); s.isNotEmpty()) { splitBank (s); return {}; }
    if (const auto s = juce::SystemStats::getEnvironmentVariable ("ORCH_DEMO", {}); s.isNotEmpty()) { renderDemos (s); return {}; }
    if (const auto s = juce::SystemStats::getEnvironmentVariable ("ORCH_MERGE", {}); s.isNotEmpty()) { mergeBank (s); return {}; }
    std::vector<VaultPreset> sel;
    std::vector<v3::Role> roles;
    const auto cachePath = juce::SystemStats::getEnvironmentVariable ("ORCH_V2CACHE", {});   // development: the v2 selection, saved once
    if (cachePath.isNotEmpty() && juce::File (cachePath).existsAsFile())
    {
        vault::loadBankFile (juce::File (cachePath), sel);
        juce::StringArray lines;
        lines.addLines (juce::File (cachePath + ".roles").loadFileAsString());
        std::map<juce::String, v3::Role> byId;
        for (auto& l : lines)
        {
            auto f = juce::StringArray::fromTokens (l, "\t", "");
            if (f.size() >= 6) byId[f[0]] = { f[1], f[2], f[3], f[4].getIntValue(), f[5].getIntValue() };
        }
        for (auto& p : sel) roles.push_back (byId[p.id]);
        std::printf ("orch: %d v2 presets from the cache\n", (int) sel.size());
    }
    else
    {
    std::vector<Base> bases;
    addStrings (bases);
    addBrass (bases);
    addOrchestral (bases);
    addPatterns (bases);
    // drop anything the library cannot play
    std::vector<Base> ok;
    for (auto& b : bases)
    {
        bool good = true;
        for (auto& p : b.parts) if (p.kind == VaultPartKind::sample && ! ctx.hasArticulation (p.source, p.articulation)) good = false;
        if (good) { ok.push_back (b); ok.back().sub = v3::subFamily (b); }
    }
    const auto treats = treatments();
    // 1. fit every sound once in its plain form; the treatments start from the fitted sound
    {
        std::vector<char> keep (ok.size(), 1);
        std::vector<juce::String> why (ok.size());
        parallel ((int) ok.size(), [&] (int i)
        {
            VaultPreset p;
            p.category = ok[(size_t) i].cat;
            p.name = ok[(size_t) i].noun;
            p.parts = applyTreat (ok[(size_t) i], treats[0]);
            if (fit (p, ok[(size_t) i].kind == kShort, &why[(size_t) i]))
            {
                ok[(size_t) i].parts = p.parts;
                if (const int si = sineIndex (p); si > 0 && std::getenv ("ORCH_VERBOSE") != nullptr)
                    std::printf ("orch: %s gets a warm sine at %.2f (%s)\n", ok[(size_t) i].noun.toRawUTF8(), p.parts[(size_t) si].level, why[(size_t) i].toRawUTF8());
            }
            else keep[(size_t) i] = 0;
        });
        std::vector<Base> kept;
        for (size_t i = 0; i < ok.size(); ++i)
            if (keep[i]) kept.push_back (ok[i]);
            else std::printf ("orch: dropped %s (cannot be made comfortable at C5: %s)\n", ok[i].noun.toRawUTF8(), why[i].toRawUTF8());
        ok = kept;
    }
    Names names;
    struct Cand { juce::String name; int cat; int kind; std::vector<VaultPart> parts; bool plain; int base; int treat; };
    std::vector<Cand> cands;
    // priority: every sound plain, then each treatment in turn across all sounds
    for (int t = 0; t < (int) treats.size(); ++t)
        for (int bi = 0; bi < (int) ok.size(); ++bi)
        {
            const auto& b = ok[(size_t) bi];
            if (b.pattern != 0) continue;
            const auto& tr = treats[(size_t) t];
            if ((tr.kinds & b.kind) == 0 || (tr.fams & b.fam) == 0) continue;
            // owner's report item 17: the strings lean dark and deep. Violin-led sounds keep only their plain and their
            // soft / far / dark roles (no crushed, taped, reversed, stabbed, doubled ... violins), and the thin high solo
            // violin register is gone (the base stays in the list so every other sound keeps its name)
            if (const auto& s0 = b.parts.front().source; s0.startsWith ("strings.violin"))
            {
                if (s0 == "strings.violin.solo" && b.parts.front().octave > 0) continue;
                static const juce::StringArray soft { "", "Distant", "Swelling", "Muted", "Muffled", "Flautando" };
                if (! soft.contains (tr.prefixes[0])) continue;
            }
            if (tr.structural != 0 && b.parts.size() >= (size_t) vault::maxParts) continue;
            // no contradictions: a vibrato articulation is not given designed vibrato, a swell / fall is not swelled again
            if (tr.prefixes[0] == "Singing" && (b.noun.containsIgnoreCase ("Vibrato") || b.noun.containsIgnoreCase ("Crooning"))) continue;
            if ((tr.prefixes[0] == "Swelling" || tr.prefixes[0] == "Frozen") && b.noun.containsIgnoreCase ("Swell")) continue;
            if (tr.prefixes[0] == "Stab" && b.noun.containsIgnoreCase ("Swell")) continue;
            const auto name = names.make (tr.prefixes, b.noun, bi);
            if (name.isEmpty()) continue;
            cands.push_back ({ name, b.cat, b.kind, applyTreat (b, tr), t == 0, bi, t });
        }
    std::vector<VaultPreset> out;
    std::vector<char> shortSound, plain;
    for (auto& c : cands)
    {
        VaultPreset p;
        p.category = c.cat;
        p.name = c.name;
        p.parts = c.parts;
        out.push_back (std::move (p));
        shortSound.push_back (c.kind == kShort ? 1 : 0);
        plain.push_back (c.plain ? 1 : 0);
    }
    if (const char* lim = std::getenv ("ORCH_LIMIT")) out.resize (std::min (out.size(), (size_t) juce::String (lim).getIntValue()));
    // 2. refit after the treatment (most pass at once)
    parallel ((int) out.size(), [&] (int i) { fit (out[(size_t) i], shortSound[(size_t) i] != 0); });

    // 3. the v2 selection (kept as it was; assignShippedIds gives the shipped ids back): the gate's own analysis (C5 / C4 / C6 log-mel x
    //    time, cepstrum, harmonic profile, motion, width) of the plain sound; every plain instrument / articulation / section /
    //    ensemble must be the shared 3.5 away from everything before it, every processing role a stricter 4.3 (ORCH_DIST),
    //    and no sound that needs more than +22 dB of make-up gain or whose velocity range is over 40 dB.
    const float own = std::getenv ("ORCH_DIST") != nullptr ? (float) juce::String (std::getenv ("ORCH_DIST")).getDoubleValue() : 4.3f;
    const float ownBase = 3.5f;
    std::vector<recipegate::Result> res (out.size());
    parallel ((int) out.size(), [&] (int i) { res[(size_t) i].preset = out[(size_t) i]; recipegate::analyse (res[(size_t) i]); });
    std::vector<int> acc;
    int nFail = 0, nGain = 0, nVel = 0, nNear = 0;
    float minPair = 1.0e9f;
    juce::String log;
    for (int i = 0; i < (int) res.size(); ++i)
    {
        auto& r = res[(size_t) i];
        float d = 1.0e9f;
        juce::String nearest;
        for (int j : acc)
            if (const float dj = recipegate::distance (r.print, res[(size_t) j].print); dj < d) { d = dj; nearest = res[(size_t) j].preset.name; }
        const juce::String why = r.fail.isNotEmpty() ? r.fail : r.gainDb > 22.0f ? juce::String ("make-up gain") : r.velRange > 40.0f ? juce::String ("velocity")
                               : d < (plain[(size_t) i] != 0 ? ownBase : own) ? juce::String ("near") : juce::String ("ok");
        log << r.preset.name << "\t" << why << "\t" << juce::String (r.gainDb, 1) << "\t" << juce::String (r.velRange, 1) << "\t" << juce::String (d, 2) << "\t" << nearest << "\n";
        if (r.fail.isNotEmpty()) { ++nFail; continue; }
        if (r.gainDb > 22.0f) { ++nGain; continue; }
        if (r.velRange > 40.0f) { ++nVel; continue; }
        if (d < (plain[(size_t) i] != 0 ? ownBase : own)) { ++nNear; continue; }
        minPair = std::min (minPair, d);
        acc.push_back (i);
    }
    if (const char* t = std::getenv ("ORCH_TSV")) juce::File (t).replaceWithText (log);
    std::printf ("orch: v2 selection %d candidates -> %d (own distance %.2f roles / %.2f sounds, min accepted %.2f; gate fails %d, make-up gain %d, velocity %d, near %d)\n",
                 (int) res.size(), (int) acc.size(), own, ownBase, minPair, nFail, nGain, nVel, nNear);
    int counters[vault::numCategories] {};
    for (int i : acc)
    {
        auto p = out[(size_t) i];
        p.gain = 1.0f;
        p.id = juce::String ("cv.") + vault::categorySlug (p.category) + "." + juce::String (++counters[p.category]).paddedLeft ('0', 4);
        const auto& b = ok[(size_t) cands[(size_t) i].base];
        const auto& t0 = treats[(size_t) cands[(size_t) i].treat].prefixes[0];
        roles.push_back ({ b.noun, b.sub, t0.isEmpty() ? juce::String ("plain") : t0, b.kind, 0 });
        sel.push_back (std::move (p));
    }
    // the rhythm presets (fitted plain), appended with the next ids
    for (auto& b : ok)
        if (b.pattern != 0 && names.used.insert (b.noun.toLowerCase()).second)
        {
            VaultPreset p;
            p.category = b.cat;
            p.name = b.noun;
            p.parts = b.parts;
            p.gain = 1.0f;
            p.id = juce::String ("cv.") + vault::categorySlug (p.category) + "." + juce::String (++counters[p.category]).paddedLeft ('0', 4);
            roles.push_back ({ b.noun, b.sub, "plain", b.kind, b.pattern });
            sel.push_back (std::move (p));
        }
    if (cachePath.isNotEmpty())
    {
        vault::saveBankFile (juce::File (cachePath), sel);
        juce::String rl;
        for (size_t i = 0; i < sel.size(); ++i)
            rl << sel[i].id << "\t" << roles[i].noun << "\t" << roles[i].sub << "\t" << roles[i].treat << "\t" << roles[i].kind << "\t" << roles[i].pattern << "\n";
        juce::File (cachePath + ".roles").replaceWithText (rl);
    }
    }

    assignShippedIds (sel, roles);
    // 3b. the cinematic low strings (report item 17): fitted plain like every sound, then placed first in their category
    //     (the browser's order: the category opens on them) so they also win every near-duplicate pair in step 5. Ids
    //     from a reserved range per category (<slug>.1001 + the place in addCinematic), stable whatever the gate drops.
    {
        std::vector<Base> cb;
        addCinematic (cb);
        std::vector<int> place (cb.size());
        {
            int k[vault::numCategories] {};
            for (size_t i = 0; i < cb.size(); ++i) place[i] = k[cb[i].cat]++;
        }
        std::vector<VaultPreset> made (cb.size());
        std::vector<char> keep (cb.size(), 0);
        std::vector<juce::String> why (cb.size());
        const auto t0 = treatments().front();
        parallel ((int) cb.size(), [&] (int i)
        {
            const auto& b = cb[(size_t) i];
            for (auto& pt : b.parts)
                if (pt.kind == VaultPartKind::sample && ! ctx.hasArticulation (pt.source, pt.articulation)) { why[(size_t) i] = "not in the library"; return; }
            auto& p = made[(size_t) i];
            p.category = b.cat;
            p.name = b.noun;
            p.parts = applyTreat (b, t0);
            p.gain = 1.0f;
            p.id = juce::String ("cv.") + vault::categorySlug (b.cat) + "." + juce::String (1001 + place[(size_t) i]);
            keep[(size_t) i] = fit (p, b.kind == kShort, &why[(size_t) i]) ? 1 : 0;
        });
        // the Strings category leans dark and deep (report item 17): an older sound that is only violins / the solo
        // violin / high violas, or is led by the violins an octave up, or is the synth string machine, stays in its plain
        // form only - its processed variants (swelling, distant, muted, underwater, stabbed ... violins) are left out, so
        // most of the category is the low sections (and the plain violins are still there to be found)
        auto thinHigh = [] (const VaultPreset& p, const v3::Role& r)
        {
            if (p.category != vault::catStrings || r.look != lNone || r.treat == "plain" || p.parts.empty()) return false;
            auto high = [] (const VaultPart& pt) { return pt.source.startsWith ("strings.violin") || (pt.source == "strings.violas.ensemble" && pt.octave > 0); };
            bool allHigh = true;
            for (auto& pt : p.parts)
            {
                if (v3::isSine (pt)) continue;
                if (pt.kind == VaultPartKind::sample ? ! high (pt) : pt.label != "String Machine") allHigh = false;
            }
            const auto& lead = p.parts.front();
            return allHigh || (lead.source.startsWith ("strings.violin") && lead.octave > 0);
        };
        int thin = 0;
        std::vector<VaultPreset> s2;
        std::vector<v3::Role> r2;
        int added = 0;
        for (int c = 0; c < vault::numCategories; ++c)
        {
            for (size_t i = 0; i < cb.size(); ++i)
            {
                if (cb[i].cat != c) continue;
                if (keep[i] == 0) { std::printf ("orch: cinematic %s dropped (%s)\n", cb[i].noun.toRawUTF8(), why[i].toRawUTF8()); continue; }
                v3::Role r { cb[i].noun, "cinematic", "plain", cb[i].kind, 0 };
                r.look = cb[i].look; r.tone = cb[i].tone; r.hall = cb[i].hall;
                s2.push_back (made[i]); r2.push_back (r); ++added;
            }
            for (size_t i = 0; i < sel.size(); ++i)
                if (sel[i].category == c)
                {
                    bool clash = false;   // an older sound with a cinematic name gives the name up
                    for (size_t j = 0; j < cb.size(); ++j) clash |= keep[j] != 0 && nameKey (cb[j].noun) == nameKey (sel[i].name);
                    if (clash) continue;
                    if (thinHigh (sel[i], roles[i])) { ++thin; continue; }
                    s2.push_back (sel[i]); r2.push_back (roles[i]);
                }
        }
        sel.swap (s2); roles.swap (r2);
        std::printf ("orch: %d cinematic low string presets lead their categories; %d thin / high string variants left out\n", added, thin);
    }
    // ORCH_ONLY=<id or name,...>: just these presets (demos, listening)
    if (const auto only = juce::StringArray::fromTokens (juce::SystemStats::getEnvironmentVariable ("ORCH_ONLY", {}), ",", ""); ! only.isEmpty())
    {
        std::vector<VaultPreset> o; std::vector<v3::Role> ro;
        for (size_t i = 0; i < sel.size(); ++i) if (only.contains (sel[i].id) || only.contains (sel[i].name)) { o.push_back (sel[i]); ro.push_back (roles[i]); }
        sel.swap (o); roles.swap (ro);
    }

    // 4. v3: every preset finished - chain + movement, C5 comfort of the whole sound, sound macros
    std::vector<juce::String> fitLog (sel.size());
    parallel ((int) sel.size(), [&] (int i)
    {
        auto& p = sel[(size_t) i];
        const auto& r = roles[(size_t) i];
        if (r.look != lNone)
        {
            v3::designCinematic (p, r);
            fitLog[(size_t) i] = v3::comfort (p);
            v3::cinematicMacros (p, r);
            return;
        }
        const auto plan = v3::designChain (p, r);
        fitLog[(size_t) i] = v3::comfort (p);
        v3::designMacros (p, r, plan);
    });
    {
        int nf = 0, nbad = 0;
        for (auto& l : fitLog) { if (l.isNotEmpty()) ++nf; if (l.endsWith ("!")) ++nbad; }
        std::printf ("orch: v3 designed %d presets; comfort refit %d (%d at the EQ limit)\n", (int) sel.size(), nf, nbad);
    }
    if (const auto dump = juce::SystemStats::getEnvironmentVariable ("ORCH_DUMP", {}); dump.isNotEmpty()) vault::saveBankFile (juce::File (dump), sel);   // development
    if (v3::envInt ("ORCH_SELFGATE", 1) == 0) return sel;

    // 5. the bank gate's own selection on the finished sounds (same analysis, same order, the shared 3.5), plus the
    //    assembly QA's spectral limits, so the bank assembly keeps every preset this returns
    std::vector<recipegate::Result> res (sel.size());
    parallel ((int) sel.size(), [&] (int i) { res[(size_t) i].preset = sel[(size_t) i]; recipegate::analyse (res[(size_t) i]); });
    std::vector<int> acc;
    std::map<juce::String, int> fails;
    float minPair = 1.0e9f;
    for (int i = 0; i < (int) res.size(); ++i)
    {
        auto& r = res[(size_t) i];
        juce::String why;
        if (r.fail.isEmpty() && ! vault::validatePreset (r.preset, &why)) r.fail = "invalid: " + why;
        if (r.fail.isEmpty() && r.gainDb > recipegate::maxTrimDb) r.fail = "too-quiet";
        if (r.fail.isEmpty() && r.share8k > recipegate::maxShare8k) r.fail = "air-8k";
        if (r.fail.isEmpty() && r.centroid > recipegate::centroidCeiling (r.preset.category)) r.fail = "centroid";
        float d = 1.0e9f;
        juce::String nearest;
        for (int j : acc)
            if (const float dj = recipegate::distance (r.print, res[(size_t) j].print); dj < d) { d = dj; nearest = res[(size_t) j].preset.name; }
        if (r.fail.isEmpty() && d < 3.5f) r.fail = "near-duplicate: " + nearest + " " + juce::String (d, 2);
        if (r.fail.isEmpty()) { acc.push_back (i); minPair = std::min (minPair, d); } else ++fails[r.fail.upToFirstOccurrenceOf (":", false, false)];
    }
    std::printf ("orch: %d of %d pass the gate (min distance %.2f)\n", (int) acc.size(), (int) res.size(), minPair);
    for (auto& [f, n] : fails) std::printf ("  orch rejected %-16s %d\n", f.toRawUTF8(), n);
    if (const auto fp = juce::SystemStats::getEnvironmentVariable ("ORCH_FAILS", {}); fp.isNotEmpty())
    {
        juce::String s;
        for (size_t i = 0; i < res.size(); ++i)
        {
            const auto& r = res[i];
            s << r.preset.id << "\t" << r.preset.name << "\t" << roles[i].sub << "\t" << roles[i].treat << "\t" << (r.fail.isEmpty() ? juce::String ("ok") : r.fail) << "\t"
              << juce::String (r.h1Db, 1) << "\t" << juce::String (100 * r.share25, 2) << "\t" << juce::String (100 * r.share6k, 2) << "\t" << juce::String (100 * r.share8k, 2)
              << "\t" << juce::String (r.centroid, 0) << "\t" << juce::String (r.velRange, 1) << "\t" << juce::String (r.tail, 1) << "\t" << juce::String (r.gainDb, 1) << "\t" << fitLog[i] << "\n";
        }
        juce::File (fp).replaceWithText (s);
    }

    // 6. macro extremes
    std::vector<v3::MacroReport> mr (acc.size());
    std::vector<VaultPreset> kept (acc.size());
    parallel ((int) acc.size(), [&] (int k)
    {
        kept[(size_t) k] = sel[(size_t) acc[(size_t) k]];
        if (v3::envInt ("ORCH_MACROCHECK", 1) != 0) mr[(size_t) k] = v3::checkMacros (kept[(size_t) k], res[(size_t) acc[(size_t) k]].preset.gain);
    });
    int fixes = 0;
    juce::StringArray harsh, quiet;
    for (auto& m : mr) { fixes += m.fixes; harsh.addArray (m.harsh); quiet.addArray (m.quiet); }
    std::printf ("orch: macro ranges narrowed %d times; %d macro extremes still outside the comfort rules; %d macros barely audible\n", fixes, harsh.size(), quiet.size());
    for (auto& h : harsh) std::printf ("  harsh: %s\n", h.toRawUTF8());
    for (auto& q : quiet) std::printf ("  quiet: %s\n", q.toRawUTF8());
    return kept;
}

VAULT_RECIPE (orch, build_orch);
