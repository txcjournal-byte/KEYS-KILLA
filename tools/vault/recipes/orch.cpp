// CHROME VAULT v2 recipes: STRINGS + BRASS + ORCHESTRAL (sound designer "orch"). Run: VaultBankGen -recipe orch (RecipeGate.h).
//
// DESIGN NOTE (research summary; sources at the end of this note)
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

struct Base
{
    juce::String noun;
    int cat, fam, kind;
    std::vector<VaultPart> parts;
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
} // namespace

std::vector<VaultPreset> build_orch (const RecipeContext& ctx)
{
    std::vector<Base> bases;
    addStrings (bases);
    addBrass (bases);
    addOrchestral (bases);
    // drop anything the library cannot play
    std::vector<Base> ok;
    for (auto& b : bases)
    {
        bool good = true;
        for (auto& p : b.parts) if (p.kind == VaultPartKind::sample && ! ctx.hasArticulation (p.source, p.articulation)) good = false;
        if (good) ok.push_back (b);
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
    struct Cand { juce::String name; int cat; int kind; std::vector<VaultPart> parts; bool plain; };
    std::vector<Cand> cands;
    // priority: every sound plain, then each treatment in turn across all sounds
    for (int t = 0; t < (int) treats.size(); ++t)
        for (int bi = 0; bi < (int) ok.size(); ++bi)
        {
            const auto& b = ok[(size_t) bi];
            const auto& tr = treats[(size_t) t];
            if ((tr.kinds & b.kind) == 0 || (tr.fams & b.fam) == 0) continue;
            if (tr.structural != 0 && b.parts.size() >= (size_t) vault::maxParts) continue;
            // no contradictions: a vibrato articulation is not given designed vibrato, a swell / fall is not swelled again
            if (tr.prefixes[0] == "Singing" && (b.noun.containsIgnoreCase ("Vibrato") || b.noun.containsIgnoreCase ("Crooning"))) continue;
            if ((tr.prefixes[0] == "Swelling" || tr.prefixes[0] == "Frozen") && b.noun.containsIgnoreCase ("Swell")) continue;
            if (tr.prefixes[0] == "Stab" && b.noun.containsIgnoreCase ("Swell")) continue;
            const auto name = names.make (tr.prefixes, b.noun, bi);
            if (name.isEmpty()) continue;
            cands.push_back ({ name, b.cat, b.kind, applyTreat (b, tr), t == 0 });
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

    // 3. this designer's own selection, stricter than the shared gate: the gate's own analysis (C5 / C4 / C6 log-mel x time,
    //    cepstrum, harmonic profile, motion, width); every plain instrument / articulation / section / ensemble must be the
    //    shared 3.5 away from everything before it, every processing role a stricter 4.3 (ORCH_DIST), and no sound that needs
    //    more than +22 dB of make-up gain (a decayed-away stab would bring the recording's noise up with it) or whose
    //    velocity range is over 40 dB (pp would vanish).
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
    std::printf ("orch: %d candidates -> %d (own distance %.2f roles / %.2f sounds, min accepted %.2f; gate fails %d, make-up gain %d, velocity %d, near %d)\n",
                 (int) res.size(), (int) acc.size(), own, ownBase, minPair, nFail, nGain, nVel, nNear);
    std::vector<VaultPreset> sel;
    int counters[vault::numCategories] {};
    for (int i : acc)
    {
        auto p = out[(size_t) i];
        p.gain = 1.0f;
        p.id = juce::String ("cv.") + vault::categorySlug (p.category) + "." + juce::String (++counters[p.category]).paddedLeft ('0', 4);
        sel.push_back (std::move (p));
    }
    return sel;
}

VAULT_RECIPE (orch, build_orch);
