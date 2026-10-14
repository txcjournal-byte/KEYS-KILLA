// CHROME VAULT v2 recipes: GUITARS + BASS (sound designer "strings"). Run: VaultBankGen -recipe strings (RecipeGate.h).
//
// DESIGN NOTE (research summary; the sources are listed at the end of this note)
//  * How commercial libraries voice guitars: a close mic (12th fret / soundhole: warmer, more body) blended with room or
//    DI; dry close samples get a plate or small room to sit "in a space", and ambient / lo-fi guitars use long halls,
//    clouds, tape wobble and ping-pong echoes. The trap / lo-fi guitar loop practice is the same: nylon or steel acoustic
//    with reverb + delay (and chorus / tape) for melodies, muted and harmonic articulations for rhythm and ear candy.
//    So every guitar instrument here gets a set of real *roles* - playing style (pick position, finger / thumb / pick,
//    palm mute, harmonics, baritone / requinto register) x room (dry room, plate, hall, cloud, echo) x colour (tape,
//    radio, chorus, phaser, flanger, felt-dark) - and layers only where doubling is real practice: double-tracked steel
//    (two takes panned), nylon + harp, steel + 12-string, octave jazz lines, guitar + bowed psaltery shimmer (octave up,
//    >= 12 dB under), guitar + soft pad, jazz guitar + upright bass duo.
//  * Bass: the professional low end is built in bands - a clean, mono sine / triangle sub owns everything under
//    ~100 Hz while the character layer (saw / square through a 24 dB ladder low-pass with a filter envelope, a detuned
//    reese, a distorted growl, a bass guitar, an orchestral bass) supplies the 200 Hz - 2 kHz harmonics that let the bass
//    read on phones and laptops. Distortion goes on the character, never on the sub (bassMode keeps the FX lows clean
//    and mono). Basses are dry (no reverb tails smearing the low end). Orchestral basses: contrabass arco / pizz /
//    spiccato / tremolo, tuba, bassoon, low trombone, pedal organ, piano and harp low registers, and the classic
//    cello-an-octave-above doubling (cellos >= 12 dB under the basses). Trap / hip-hop practice: subs and slides with
//    glide, piano or orchestral pizz doubled by a sub, distorted mid layers that are high-passed against the kick.
//  * Register: FL Studio calls MIDI 60 "C5" and every preview plays C5. Guitars play at pitch (the models and the
//    plucked samples sit naturally around C5). Basses are transposed down so C5 plays a real bass note: synth / model /
//    contrabass / tuba / piano / harp basses -2 octaves (C5 key = 65 Hz), instruments whose range stops above MIDI 24
//    (cellos, bassoon, trombone, pedal organ) -1 octave, so C4..C6 keys stay inside the sampled range.
//  * Comfort (checked by the gate on every preset): the sounding fundamental is the loudest partial at C5 (within 3 dB),
//    2-5 kHz <= 10 % of the power, > 6 kHz <= 3 %, octave-up layers >= 12 dB under the main part, velocity opens the
//    sound >= 3 dB, no clicks / clipping, mono-safe lows, tails <= 10 s. Basses that would need a resonant / bright
//    character over 65 Hz get a clean sub part so the fundamental stays on top.
//  * Distinctness: candidates are generated in priority order (core sound of each instrument first, then its roles) and
//    the gate drops anything whose log-mel / cepstral fingerprint (C5 + C4 + C6) is closer than the threshold to an
//    accepted preset - no parameter jitter presets.
//  * Calibration of the distinctness threshold (VaultBankGen -prints): the same guitar dry vs. with only a hall / tape /
//    radio colour measures 1.1-2.5, vs. echo / chorus / phaser / flanger / dark 3.5-4.5, a different playing style of the
//    same instrument 4-9, a different instrument 7-16 -> threshold 3.5 (an effect counts only when it changes the role).
//  * Result (VaultBankGen -recipe strings -dist 3.5, library /home/user/cv-library): 2128 candidates -> 210 Guitars +
//    211 Bass, minimum pairwise distance 3.51; all at C5: fundamental >= -2.9 dB of the strongest partial, 2-5 kHz
//    <= 6.5 %, > 6 kHz <= 2.1 %, tails <= 8.8 s, velocity range >= 3.1 dB, loudness at the category target (muted plucks
//    up to 10 dB under it, limited by their peak).
//  Sources: violetrecording.com "How to Design a Bass Sound"; create.routenote.com "3 tips to help your bassline cut
//  through"; dnbforum.com threads on sub + mid layering; soundonsound.com "Recording Acoustic Guitar"; audient.com
//  "5 essential tips for recording acoustic guitar"; vi-control.net "reverb vs room mics"; gearnews.com / urbankits /
//  memusic lo-fi nylon guitar packs (nylon + reverb / delay / chorus / tape in trap and lo-fi); the model research in
//  docs/VAULT_MODELS.md (Valimaki et al., Karjalainen et al., Smith).
#include "RecipeGate.h"
#include <set>

namespace
{
using KV = std::vector<std::pair<juce::String, float>>;

VaultPart with (VaultPart p, const KV& kv)
{
    for (auto& [k, v] : kv) p.set (k, v);
    return p;
}
VaultPart oct (VaultPart p, int o) { p.octave = o; return p; }
VaultPart lvl (VaultPart p, float l, float pan = 0.0f) { p.level = l; p.pan = pan; return p; }

struct Cand
{
    juce::String name;
    int category = vault::catGuitars;
    std::vector<VaultPart> parts;
};

// a role / treatment: FX keys for part 0 (+ optional part-0 overrides) and name prefixes
struct Treat
{
    juce::StringArray prefixes;   // "" = the plain name
    KV fx;
};

struct Names
{
    std::set<juce::String> used;
    // first free "<prefix> <noun>" (falls back to the next prefix)
    juce::String make (const juce::StringArray& prefixes, const juce::String& noun, int rotate)
    {
        for (int i = 0; i < prefixes.size(); ++i)
        {
            const auto pre = prefixes[(i + rotate) % prefixes.size()];
            const auto n = pre.isEmpty() ? noun : pre + " " + noun;
            if (n.length() <= 26 && used.insert (n.toLowerCase()).second) return n;
        }
        return {};
    }
};

//==============================================================================
// GUITARS
struct Variant { juce::String noun; KV kv; int octave = 0; };
struct Instrument { bool model; juce::String source, articulation, label; std::vector<Variant> variants; };

std::vector<Instrument> guitarInstruments()
{
    // playing styles / registers that change the instrument's sound itself (not just an effect)
    return {
        { true, "guitar.nylon", "", "Nylon Guitar", {
            { "Classical Nylon", {} },
            { "Thumbed Nylon", { { "m.pickPos", 0.35f }, { "m.tone", 0.22f } } },
            { "Flamenco Nylon", { { "m.pickPos", 0.1f }, { "m.tone", 0.68f }, { "m.decay", 0.7f } } },
            { "Muted Nylon", { { "m.artic", 1 } } },
            { "Nylon Harmonics", { { "m.artic", 2 } } },
            { "Low Nylon", { { "eqLow", 6.0f } }, -1 },
            { "Requinto Nylon", { { "m.tone", 0.35f } }, 1 },
            { "Sul Tasto Nylon", { { "m.pickPos", 0.48f }, { "m.tone", 0.3f }, { "m.body", 1.2f } } },
            { "Ponticello Nylon", { { "m.pickPos", 0.05f }, { "m.tone", 0.58f } } },
            { "Ringing Nylon", { { "m.decay", 2.5f }, { "m.release", 1.5f } } },
            { "Staccato Nylon", { { "m.decay", 0.35f }, { "m.release", 0.08f } } },
            { "Boxy Nylon", { { "m.body", 1.5f }, { "m.tone", 0.35f } } },
            { "Bare String Nylon", { { "m.body", 0.0f } } },
            { "High Nylon Harmonics", { { "m.artic", 2 } }, 1 },
            { "High Muted Nylon", { { "m.artic", 1 } }, 1 },
            { "Low Nylon Harmonics", { { "m.artic", 2 }, { "eqLow", 4.0f } }, -1 } } },
        { true, "guitar.acoustic", "", "Steel Acoustic Guitar", {
            { "Steel Acoustic", {} },
            { "Fingerpicked Steel", { { "m.pickPos", 0.22f }, { "m.tone", 0.38f }, { "m.body", 1.1f } } },
            { "Soundhole Steel", { { "m.pickPos", 0.3f }, { "m.tone", 0.28f }, { "m.body", 1.4f } } },
            { "Bright Steel Pick", { { "m.pickPos", 0.08f }, { "m.tone", 0.68f } } },
            { "Palm Muted Steel", { { "m.artic", 1 } } },
            { "Steel Harmonics", { { "m.artic", 2 } } },
            { "Baritone Steel", { { "m.tone", 0.45f }, { "eqLow", 6.0f } }, -1 },
            { "Slack Steel", { { "m.decay", 1.6f }, { "m.stiff", 0.4f }, { "m.detune", 7.0f }, { "m.tone", 0.4f } } },
            { "Thin Bridge Steel", { { "m.pickPos", 0.05f }, { "m.tone", 0.5f } } },
            { "Ringing Steel", { { "m.decay", 2.4f }, { "m.release", 1.5f } } },
            { "Short Steel", { { "m.decay", 0.4f }, { "m.release", 0.1f } } },
            { "Wire Steel", { { "m.stiff", 3.5f }, { "m.tone", 0.45f } } },
            { "Bare String Steel", { { "m.body", 0.25f }, { "m.tone", 0.4f } } },
            { "High Steel", { { "m.tone", 0.35f } }, 1 },
            { "Muted Baritone", { { "m.artic", 1 }, { "eqLow", 5.0f } }, -1 },
            { "High Muted Steel", { { "m.artic", 1 } }, 1 },
            { "Low Steel Harmonics", { { "m.artic", 2 }, { "eqLow", 4.0f } }, -1 } } },
        { true, "guitar.electric", "", "Clean Electric Guitar", {
            { "Neck Electric", { { "m.pickup", 0.0f }, { "m.tone", 0.45f } } },
            { "Bridge Electric", { { "m.pickup", 1.0f }, { "m.tone", 0.55f } } },
            { "Middle Electric", { { "m.pickup", 0.5f } } },
            { "Chicken Pick", { { "m.artic", 1 }, { "m.pickup", 0.8f } } },
            { "Electric Harmonics", { { "m.artic", 2 } } },
            { "Baritone Electric", { { "m.pickup", 0.3f }, { "eqLow", 5.0f } }, -1 },
            { "Glass Electric", { { "m.tone", 0.7f }, { "m.pickup", 0.6f } } },
            { "Edge of Breakup", { { "m.drive", 0.35f }, { "m.pickup", 0.5f } } },
            { "Muted Neck Electric", { { "m.artic", 1 }, { "m.pickup", 0.0f } } },
            { "Ringing Neck Electric", { { "m.decay", 2.2f }, { "m.pickup", 0.0f }, { "m.release", 1.2f } } },
            { "High Electric", { { "m.pickup", 0.2f }, { "m.tone", 0.4f } }, 1 },
            { "Short Bridge Electric", { { "m.decay", 0.4f }, { "m.pickup", 1.0f }, { "m.release", 0.1f } } },
            { "High Electric Harmonics", { { "m.artic", 2 } }, 1 } } },
        { true, "guitar.jazz", "", "Jazz Archtop Guitar", {
            { "Jazz Archtop", {} },
            { "Brushed Archtop", { { "m.tone", 0.25f } } },
            { "Muted Archtop", { { "m.artic", 1 } } },
            { "Archtop Harmonics", { { "m.artic", 2 } } },
            { "Bright Archtop", { { "m.tone", 0.65f }, { "m.pickPos", 0.15f } } },
            { "Low Archtop", { { "eqLow", 4.0f } }, -1 },
            { "Ringing Archtop", { { "m.decay", 2.0f }, { "m.release", 1.0f } } },
            { "High Archtop", { { "m.tone", 0.4f } }, 1 },
            { "Thumb Archtop", { { "m.pickPos", 0.4f }, { "m.tone", 0.2f } } },
            { "Low Muted Archtop", { { "m.artic", 1 }, { "eqLow", 4.0f } }, -1 } } },
        { true, "guitar.twelve", "", "12-String Guitar", {
            { "Twelve String", {} },
            { "Soft Twelve", { { "m.tone", 0.3f }, { "m.pickPos", 0.25f } } },
            { "Wide Twelve", { { "m.detune", 12.0f } } },
            { "Twelve Harmonics", { { "m.artic", 2 } } },
            { "Muted Twelve", { { "m.artic", 1 } } },
            { "High Twelve", { { "m.detune", 6.0f } }, 1 },
            { "Ringing Twelve", { { "m.decay", 2.3f }, { "m.detune", 8.0f }, { "m.release", 1.4f } } },
            { "Bright Twelve", { { "m.tone", 0.65f }, { "m.pickPos", 0.08f } } },
            { "High Muted Twelve", { { "m.artic", 1 } }, 1 } } },
        { true, "guitar.crunch", "", "Crunch Electric Guitar", {
            { "Blues Crunch", { { "m.drive", 0.35f } } },
            { "Grit Crunch", { { "m.drive", 0.6f } } },
            { "Crunch Chug", { { "m.artic", 1 }, { "m.drive", 0.7f }, { "eqLow", 6.0f } }, -1 },
            { "Pinched Crunch", { { "m.artic", 2 }, { "m.drive", 0.5f } } },
            { "Lead Crunch", { { "m.drive", 0.8f }, { "m.tone", 0.42f } } },
            { "Muted Crunch", { { "m.artic", 1 }, { "m.drive", 0.45f } } },
            { "Low Crunch", { { "m.drive", 0.5f }, { "eqLow", 5.0f } }, -1 },
            { "High Lead Crunch", { { "m.drive", 0.7f }, { "m.tone", 0.35f } }, 1 },
            { "Neck Crunch", { { "m.pickup", 0.0f }, { "m.drive", 0.5f } } },
            { "Bridge Crunch", { { "m.pickup", 1.0f }, { "m.drive", 0.5f }, { "m.tone", 0.45f } } } } },
        { true, "guitar.banjo", "", "Banjo", {
            { "Bluegrass Banjo", {} },
            { "Clawhammer Banjo", { { "m.tone", 0.35f }, { "m.pickPos", 0.25f } } },
            { "Muted Banjo", { { "m.artic", 1 } } },
            { "Banjo Chimes", { { "m.artic", 2 } } },
            { "Cello Banjo", { { "eqLow", 6.0f } }, -1 },
            { "High Banjo", { { "m.tone", 0.3f } }, 1 },
            { "Ringing Banjo", { { "m.decay", 2.2f }, { "m.release", 1.0f } } },
            { "Open Back Banjo", { { "m.body", 0.4f }, { "m.tone", 0.4f } } },
            { "Bridge Banjo", { { "m.pickPos", 0.05f } } },
            { "High Banjo Chimes", { { "m.artic", 2 } }, 1 } } },
        { false, "pluck.harp.concert", "pluck", "Concert Harp", {
            { "Concert Harp", {} },
            { "Damped Harp", { { "s.oneShot", 0 }, { "s.release", 0.35f } } },
            { "High Harp", { { "s.width", 0.5f } }, 1 },
            { "Dark Harp", { { "s.cutoff", 1500 } } },
            { "Harp Swell", { { "s.attack", 0.25f }, { "s.oneShot", 0 }, { "s.release", 1.2f } } } } },
        { false, "pluck.harp.folk", "pluck", "Folk Harp", {
            { "Folk Harp", { { "s.width", 0.6f } } },
            { "Low Folk Harp", { { "eqLow", 6.0f }, { "s.width", 0.6f } }, -1 },
            { "Folk Harp Swell", { { "s.attack", 0.3f }, { "s.oneShot", 0 }, { "s.release", 1.2f }, { "s.width", 0.6f } } },
            { "High Folk Harp", { { "s.width", 0.5f } }, 1 } } },
    };
}

std::vector<Treat> guitarTreatments()
{
    return {
        { { "" },                                  { { "revMix", 0.1f }, { "revSize", 0.3f }, { "revType", 1 } } },
        { { "Echo", "Canyon", "Ping" },            { { "delayMix", 0.3f }, { "delayTime", 2 }, { "delayFb", 0.4f }, { "delayMode", 0 }, { "revMix", 0.14f }, { "revType", 0 } } },
        { { "Shimmer", "Wide", "Silken" },         { { "chorus", 0.55f }, { "width", 0.65f }, { "revMix", 0.15f }, { "revType", 1 } } },
        { { "Dreamy", "Haze", "Drifting" },        { { "revMix", 0.36f }, { "revSize", 0.5f }, { "revType", 2 }, { "delayMix", 0.12f }, { "delayTime", 2 }, { "delayFb", 0.3f }, { "width", 0.65f } } },
        { { "Velvet", "Midnight", "Hushed" },      { { "eqHigh", -9.0f }, { "m.tone", 0.18f }, { "s.cutoff", 1400 }, { "revMix", 0.2f }, { "revSize", 0.55f }, { "revType", 0 } } },
        { { "Swirl", "Spiral", "Orbit" },          { { "phaser", 0.55f }, { "revMix", 0.15f }, { "revType", 1 } } },
        { { "Jet", "Flanged", "Tidal" },           { { "flanger", 0.45f }, { "revMix", 0.15f }, { "revType", 0 } } },
        { { "Dusty", "Cassette", "Faded" },        { { "wow", 0.6f }, { "crush", 0.3f }, { "eqHigh", -9.0f }, { "revMix", 0.12f }, { "revSize", 0.4f }, { "revType", 1 } } },
        { { "Ghost", "Phantom", "Mirage" },        { { "ghost", 0.5f }, { "ghostRev", 1 }, { "ghostOct", 3 }, { "ghostBlur", 0.6f }, { "revMix", 0.12f }, { "revType", 0 } } },
        { { "Cathedral", "Chapel", "Hall" },       { { "revMix", 0.38f }, { "revSize", 0.85f }, { "revType", 0 } } },
        { { "Wooden", "Cabin", "Timber" },         { { "body", 5 }, { "bodyMix", 0.45f }, { "revMix", 0.1f }, { "revType", 1 } } },
        { { "Reverse", "Backward", "Rewind" },     { { "reverse", 0.5f }, { "revMix", 0.18f }, { "revType", 0 } } },
        { { "Undertow", "Deep Water", "Abyss" },   { { "ghost", 0.45f }, { "ghostRev", 0 }, { "ghostOct", 1 }, { "ghostBlur", 0.5f }, { "revMix", 0.12f }, { "revType", 0 } } },
    };
}
VaultPart makePart (const Instrument& in, const Variant& v)
{
    auto p = in.model ? recipe::modelPart (in.source, in.label) : recipe::samplePart (in.source, in.articulation, in.label);
    p = with (p, v.kv);
    p.octave = v.octave;
    return p;
}

// soft synth pad for guitar beds (EVOLVE VA, slow, dark, quiet)
VaultPart softPad()
{
    return with (recipe::synthPart ("Soft Pad"), { { "engine", 0 }, { "wave", 0.1f }, { "unison", 4 }, { "detune", 0.18f }, { "filterType", 0 }, { "cutoff", 1100 },
                                                  { "reso", 0.05f }, { "keyTrack", 0.3f }, { "attack", 0.7f }, { "decay", 1.0f }, { "sustain", 0.85f },
                                                  { "release", 1.4f }, { "velSens", 0.3f }, { "macro2", 0 }, { "macro5", 0 }, { "master", 0 } });
}

void addGuitars (const RecipeContext& ctx, std::vector<Cand>& out, Names& names)
{
    const auto insts = guitarInstruments();
    const auto treats = guitarTreatments();
    auto usable = [&] (const Instrument& in) { return in.model || ctx.hasArticulation (in.source, in.articulation); };
    // priority: every instrument's variants dry first, then each treatment in turn (rooms before colours)
    for (int t = 0; t < (int) treats.size(); ++t)
        for (auto& in : insts)
        {
            if (! usable (in)) continue;
            for (int vi = 0; vi < (int) in.variants.size(); ++vi)
            {
                const auto& v = in.variants[(size_t) vi];
                // ambient roles (cloud, hall, reverse, ghost, undertow) only on notes that ring: a reversed or cathedral palm
                // mute / staccato pluck is not a musical sound
                const bool rings = ! v.noun.containsIgnoreCase ("Muted") && ! v.noun.containsIgnoreCase ("Staccato") && ! v.noun.containsIgnoreCase ("Short")
                                   && ! v.noun.containsIgnoreCase ("Chicken") && ! v.noun.containsIgnoreCase ("Chug") && ! v.noun.containsIgnoreCase ("Damped");
                const auto& pre0 = treats[(size_t) t].prefixes[0];
                if (! rings && (pre0 == "Dreamy" || pre0 == "Cathedral" || pre0 == "Reverse" || pre0 == "Ghost" || pre0 == "Undertow")) continue;
                auto p = makePart (in, v);
                for (auto& [k, val] : treats[(size_t) t].fx)
                    if ((! k.startsWith ("m.") || in.model) && (! k.startsWith ("s.") || ! in.model)) p.set (k, val);
                const auto name = names.make (treats[(size_t) t].prefixes, v.noun, vi);
                if (name.isEmpty()) continue;
                out.push_back ({ name, vault::catGuitars, { p } });
            }
        }

    // layers (real doubling practice), each with a few rooms
    auto m = [] (const char* id, const char* label, KV kv = {}) { return with (recipe::modelPart (id, label), kv); };
    auto s = [] (const char* id, const char* art, const char* label, KV kv = {}) { return with (recipe::samplePart (id, art, label), kv); };
    struct L { juce::String noun; std::vector<VaultPart> parts; bool needsSamples; };
    auto pad = [] (float l) { return lvl (softPad(), l); };
    std::vector<L> layers {
        { "Double Tracked Steel", { lvl (m ("guitar.acoustic", "Steel Acoustic Guitar", { { "m.pickPos", 0.12f } }), 0.8f, -0.6f),
                                    lvl (m ("guitar.acoustic", "Steel Acoustic Guitar", { { "m.pickPos", 0.19f }, { "m.detune", 5 }, { "m.tone", 0.44f } }), 0.8f, 0.6f) }, false },
        { "Harp and Nylon", { m ("guitar.nylon", "Nylon Guitar"), lvl (s ("pluck.harp.concert", "pluck", "Concert Harp"), 0.5f) }, true },
        { "Steel and Twelve", { m ("guitar.acoustic", "Steel Acoustic Guitar"), lvl (m ("guitar.twelve", "12-String Guitar", { { "m.tone", 0.35f } }), 0.55f, 0.3f) }, false },
        { "Octave Jazz Lines", { m ("guitar.jazz", "Jazz Archtop Guitar"), lvl (oct (m ("guitar.jazz", "Jazz Archtop Guitar", { { "m.tone", 0.35f } }), -1), 0.7f) }, false },
        { "Strumstick Nylon", { m ("guitar.nylon", "Nylon Guitar"), lvl (s ("pluck.dulcimer", "pluck", "Stick Dulcimer", { { "s.cutoff", 1800 } }), 0.3f, 0.25f) }, true },
        { "Glow Electric", { m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.1f } }),
                             lvl (oct (s ("pluck.psaltery", "bowed", "Bowed Psaltery", { { "s.attack", 0.25f }, { "s.cutoff", 2500 } }), 1), 0.12f) }, true },
        { "Banjo and Nylon", { m ("guitar.nylon", "Nylon Guitar"), lvl (m ("guitar.banjo", "Banjo", { { "m.tone", 0.35f } }), 0.45f, -0.3f) }, false },
        { "Nylon Over Pad", { m ("guitar.nylon", "Nylon Guitar"), pad (0.3f) }, false },
        { "Steel Over Pad", { m ("guitar.acoustic", "Steel Acoustic Guitar", { { "m.tone", 0.4f } }), pad (0.3f) }, false },
        { "Clean Octaves", { m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.3f } }),
                             lvl (oct (m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.0f }, { "m.tone", 0.35f } }), -1), 0.7f) }, false },
        { "Guitar and Upright Duo", { m ("guitar.jazz", "Jazz Archtop Guitar"), lvl (oct (s ("strings.contrabass", "pizzicato", "Contrabass"), -2), 0.6f) }, true },
        { "Fourfold Strings", { m ("guitar.nylon", "Nylon Guitar"), lvl (s ("pluck.harp.concert", "pluck", "Concert Harp"), 0.4f, -0.3f),
                                lvl (oct (s ("pluck.psaltery", "bowed", "Bowed Psaltery", { { "s.attack", 0.3f }, { "s.cutoff", 2500 } }), 1), 0.1f, 0.3f), pad (0.22f) }, true },
        { "Harp and Steel Bed", { s ("pluck.harp.concert", "pluck", "Concert Harp"), lvl (m ("guitar.acoustic", "Steel Acoustic Guitar", { { "m.tone", 0.35f } }), 0.5f), pad (0.25f) }, true },
        { "Muted Steel and Nylon", { lvl (m ("guitar.acoustic", "Steel Acoustic Guitar", { { "m.artic", 1 } }), 0.85f, -0.4f),
                                     lvl (m ("guitar.nylon", "Nylon Guitar", { { "m.artic", 1 } }), 0.85f, 0.4f) }, false },
        { "Nylon and Steel Pair", { lvl (m ("guitar.nylon", "Nylon Guitar"), 0.85f, -0.5f), lvl (m ("guitar.acoustic", "Steel Acoustic Guitar", { { "m.tone", 0.4f } }), 0.7f, 0.5f) }, false },
        { "Electric over Acoustic", { m ("guitar.acoustic", "Steel Acoustic Guitar", { { "m.tone", 0.4f } }), lvl (m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.0f } }), 0.6f, 0.4f) }, false },
        { "Vibes and Archtop", { m ("guitar.jazz", "Jazz Archtop Guitar"), lvl (s ("mallets.vibraphone", "soft mallets", "Vibraphone"), 0.45f, 0.3f) }, true },
        { "Wooden Nylon", { m ("guitar.nylon", "Nylon Guitar"), lvl (s ("mallets.marimba", "hit", "Marimba"), 0.4f, -0.3f) }, true },
        { "Nylon and Thumb Piano", { m ("guitar.nylon", "Nylon Guitar", { { "m.tone", 0.4f } }), lvl (s ("world.kalimba", "pluck", "Thumb Piano"), 0.45f, 0.3f) }, true },
        { "Sparkle Steel", { m ("guitar.acoustic", "Steel Acoustic Guitar"), lvl (oct (s ("mallets.glockenspiel", "hit", "Glockenspiel"), 2), 0.08f, 0.2f) }, true },
        { "Mountain Strings", { m ("guitar.banjo", "Banjo", { { "m.tone", 0.4f } }), lvl (s ("pluck.dulcimer", "pluck", "Stick Dulcimer", { { "s.cutoff", 1800 } }), 0.35f, 0.3f) }, true },
        { "Angel Harp", { s ("pluck.harp.concert", "pluck", "Concert Harp"),
                          lvl (oct (s ("pluck.psaltery", "bowed", "Bowed Psaltery", { { "s.attack", 0.35f }, { "s.cutoff", 2200 } }), 1), 0.1f) }, true },
        { "Electric Over Pad", { m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.2f } }), pad (0.3f) }, false },
        { "Amp Blend", { lvl (m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.4f } }), 0.8f, -0.5f), lvl (m ("guitar.crunch", "Crunch Electric Guitar", { { "m.drive", 0.4f } }), 0.55f, 0.5f) }, false },
        { "Twelve and Harp", { m ("guitar.twelve", "12-String Guitar"), lvl (s ("pluck.harp.concert", "pluck", "Concert Harp"), 0.45f, 0.3f) }, true },
        { "Bossa Duo", { m ("guitar.nylon", "Nylon Guitar", { { "m.pickPos", 0.3f }, { "m.tone", 0.3f } }), lvl (oct (s ("strings.contrabass", "pizzicato", "Contrabass"), -2), 0.55f) }, true },
        { "Folk Cello Guitar", { m ("guitar.acoustic", "Steel Acoustic Guitar"), lvl (oct (s ("strings.cellos.ensemble", "sustain", "Cello Section", { { "s.attack", 0.2f } }), -1), 0.35f) }, true },
        { "Power Chord Crunch", { m ("guitar.crunch", "Crunch Electric Guitar", { { "m.drive", 0.55f } }),
                                  [&] { auto p = lvl (m ("guitar.crunch", "Crunch Electric Guitar", { { "m.drive", 0.55f } }), 0.7f, 0.2f); p.semi = 7; return p; }() }, false },
        { "Muted Power Chug", { oct (m ("guitar.crunch", "Crunch Electric Guitar", { { "m.artic", 1 }, { "m.drive", 0.6f }, { "eqLow", 5.0f } }), -1),
                                [&] { auto p = lvl (oct (m ("guitar.crunch", "Crunch Electric Guitar", { { "m.artic", 1 }, { "m.drive", 0.6f } }), -1), 0.7f, 0.2f); p.semi = 7; return p; }() }, false },
        { "Open Fifths Nylon", { m ("guitar.nylon", "Nylon Guitar"),
                                 [&] { auto p = lvl (m ("guitar.nylon", "Nylon Guitar", { { "m.tone", 0.4f } }), 0.55f, 0.3f); p.semi = 7; return p; }() }, false },
        { "Clean Fourths", { m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.2f } }),
                             [&] { auto p = lvl (m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.2f } }), 0.55f, -0.3f); p.semi = 5; return p; }() }, false },
        { "Banjo Harp Trio", { m ("guitar.banjo", "Banjo"), lvl (s ("pluck.harp.folk", "pluck", "Folk Harp"), 0.5f, -0.3f), lvl (m ("guitar.nylon", "Nylon Guitar"), 0.5f, 0.3f) }, true },
    };
    const std::vector<int> layerTreats { 0, 9, 3, 1, 2, 7, 8 };
    for (int t : layerTreats)
        for (int li = 0; li < (int) layers.size(); ++li)
        {
            auto& l = layers[(size_t) li];
            bool ok = true;
            for (auto& p : l.parts) if (p.kind == VaultPartKind::sample && ! ctx.hasArticulation (p.source, p.articulation)) ok = false;
            if (! ok) continue;
            auto parts = l.parts;
            for (auto& [k, val] : treats[(size_t) t].fx) if (! k.startsWith ("m.") && ! k.startsWith ("s.")) parts[0].set (k, val);
            const auto name = names.make (treats[(size_t) t].prefixes, l.noun, li);
            if (name.isNotEmpty()) out.push_back ({ name, vault::catGuitars, parts });
        }
}

//==============================================================================
// BASS
KV synthBassBase()
{
    return { { "engine", 0 }, { "wave", 0 }, { "unison", 1 }, { "detune", 0.1f }, { "mono", 1 }, { "legato", 1 }, { "glide", 0.06f }, { "bassMode", 1 },
             { "macro1", 0 }, { "macro2", 0 }, { "macro3", 0 }, { "macro4", 0 }, { "macro5", 0.5f }, { "macro6", 0 }, { "macro7", 0 }, { "macro8", 0.5f },
             { "revMix", 0 }, { "chorus", 0 }, { "width", 0.3f }, { "master", 0.15f },
             { "attack", 0.002f }, { "decay", 0.8f }, { "sustain", 0.85f }, { "release", 0.12f }, { "velSens", 0.6f },
             { "filterType", 1 }, { "cutoff", 600 }, { "reso", 0.15f }, { "keyTrack", 0.3f }, { "fenv", 0.3f }, { "fattack", 0.001f },
             { "fdecay", 0.25f }, { "fsustain", 0.2f }, { "frelease", 0.15f } };
}
VaultPart synthBass (const juce::String& label, const KV& kv)
{
    auto p = with (with (recipe::synthPart (label), synthBassBase()), kv);
    p.octave = -2;
    return p;
}
// the clean mono sine sub that owns the lows under a character layer
VaultPart subPart (float level = 0.75f, float sat = 0.0f)
{
    auto p = synthBass ("Sub Bass", { { "engine", 6 }, { "wave", sat }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "sustain", 1.0f },
                                      { "decay", 1.0f }, { "release", 0.1f } });
    p.level = level;
    return p;
}

struct BassVariant { juce::String noun; std::vector<VaultPart> parts; };

std::vector<BassVariant> synthBasses()
{
    auto sb = [] (const char* label, KV kv) { return synthBass (label, kv); };
    return {
        { "Deep Sine Sub", { sb ("Sine Sub", { { "engine", 6 }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "sustain", 1 }, { "decay", 1 } }) } },
        { "Warm Sub", { sb ("Warm Sub", { { "engine", 6 }, { "wave", 0.3f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "sustain", 1 } }) } },
        { "Round Sub Pluck", { sb ("Sub Pluck", { { "engine", 6 }, { "wave", 0.2f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "decay", 0.9f }, { "sustain", 0 } }) } },
        { "Knock Sub", { sb ("Knock Sub", { { "engine", 6 }, { "wave", 0.35f }, { "fmAmt", 0.22f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 },
                                            { "decay", 1.6f }, { "sustain", 0.25f }, { "macro6", 0.35f } }) } },
        { "Saturated Sub", { sb ("Saturated Sub", { { "velSens", 0.95f }, { "engine", 6 }, { "wave", 0.7f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "sustain", 1 },
                                                    { "drive", 0.25f }, { "driveType", 1 } }) } },
        { "Ladder Saw Bass", { sb ("Ladder Saw Bass", { { "cutoff", 450 }, { "reso", 0.2f }, { "fenv", 0.35f }, { "fdecay", 0.3f } }) } },
        { "Ladder Square Bass", { sb ("Ladder Square Bass", { { "wave", 0.5f }, { "cutoff", 500 }, { "fenv", 0.3f } }) } },
        { "Pulse Bass", { sb ("Pulse Bass", { { "wave", 0.8f }, { "cutoff", 650 }, { "fenv", 0.25f } }) } },
        { "Plucked Ladder Bass", { sb ("Ladder Pluck Bass", { { "decay", 0.45f }, { "sustain", 0.15f }, { "fdecay", 0.15f }, { "fenv", 0.45f }, { "cutoff", 380 } }) } },
        { "Three Oscillator Bass", { sb ("Three Oscillator Bass", { { "unison", 3 }, { "detune", 0.1f }, { "wave", 0.2f }, { "cutoff", 520 }, { "fenv", 0.3f } }) } },
        { "Acid Line Bass", { sb ("Acid Bass", { { "reso", 0.62f }, { "fenv", 0.55f }, { "fdecay", 0.18f }, { "cutoff", 280 }, { "decay", 0.5f }, { "sustain", 0.4f },
                                                 { "glide", 0.15f } }), subPart (0.7f) } },
        { "Reese Bass", { sb ("Reese Bass", { { "unison", 5 }, { "detune", 0.32f }, { "filterType", 0 }, { "cutoff", 900 }, { "fenv", 0.1f } }), subPart (0.8f) } },
        { "Dark Reese", { sb ("Dark Reese", { { "unison", 5 }, { "detune", 0.28f }, { "filterType", 1 }, { "cutoff", 380 }, { "fenv", 0.05f } }), subPart (0.8f) } },
        { "Growl Bass", { sb ("Growl Bass", { { "unison", 3 }, { "detune", 0.2f }, { "filterType", 2 }, { "cutoff", 600 }, { "reso", 0.4f }, { "macro2", 0.35f },
                                              { "lfoRate", 2.0f }, { "wobTarget", 0 } }), subPart (0.8f) } },
        { "FM Bass", { sb ("FM Bass", { { "engine", 1 }, { "fmAlgo", 0 }, { "fmRatio", 1 }, { "fmAmt", 0.35f }, { "decay", 0.7f }, { "sustain", 0.5f },
                                        { "filterType", 0 }, { "cutoff", 3000 }, { "fenv", 0 } }) } },
        { "FM Pluck Bass", { sb ("FM Pluck Bass", { { "engine", 1 }, { "fmAlgo", 0 }, { "fmRatio", 2 }, { "fmAmt", 0.5f }, { "decay", 0.35f }, { "sustain", 0.1f },
                                                    { "filterType", 0 }, { "cutoff", 2500 }, { "fenv", 0 } }) } },
        { "Wavetable Bass", { sb ("Wavetable Bass", { { "engine", 7 }, { "wave", 0.3f }, { "filterType", 0 }, { "cutoff", 1200 }, { "fenv", 0.2f } }) } },
        { "Organ Bass", { sb ("Organ Bass", { { "engine", 4 }, { "wave", 0.2f }, { "filterType", 0 }, { "cutoff", 3000 }, { "fenv", 0 }, { "sustain", 1 } }) } },
        { "String Pluck Bass", { sb ("Plucked Synth Bass", { { "engine", 2 }, { "wave", 0.4f }, { "filterType", 0 }, { "cutoff", 2500 }, { "fenv", 0 } }) } },
        { "Hollow Bass", { sb ("Hollow Bass", { { "velSens", 0.95f }, { "wave", 0.5f }, { "filterType", 4 }, { "cutoff", 400 }, { "reso", 0.2f }, { "fenv", 0.2f } }), subPart (0.8f) } },
        { "Bitcrushed Bass", { sb ("Crushed Bass", { { "velSens", 0.95f }, { "cutoff", 700 }, { "fenv", 0.2f }, { "crush", 0.35f } }) } },
        { "Fuzz Bass", { sb ("Fuzz Bass", { { "velSens", 0.95f }, { "cutoff", 650 }, { "drive", 0.55f }, { "driveType", 2 } }) } },
        { "Folded Bass", { sb ("Folded Bass", { { "velSens", 0.95f }, { "cutoff", 600 }, { "drive", 0.45f }, { "driveType", 4 } }) } },
        { "Tape Bass", { sb ("Tape Bass", { { "wave", 0.3f }, { "cutoff", 520 }, { "wow", 0.2f }, { "drive", 0.2f }, { "driveType", 1 } }) } },
        { "Talking Bass", { sb ("Vowel Bass", { { "engine", 3 }, { "wave", 0.3f }, { "filterType", 0 }, { "cutoff", 1800 }, { "fenv", 0 } }), subPart (0.8f) } },
        { "Woody Bass", { sb ("Modal Bass", { { "engine", 9 }, { "wave", 0.5f }, { "fmAmt", 0.2f }, { "filterType", 0 }, { "cutoff", 4000 }, { "fenv", 0 },
                                              { "decay", 1.2f }, { "sustain", 0 } }) } },

        { "Long Decay Sub", { sb ("Sine Sub", { { "engine", 6 }, { "wave", 0.15f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "decay", 3.0f }, { "sustain", 0 }, { "release", 0.4f } }) } },
        { "Short Sub Stab", { sb ("Sine Sub", { { "engine", 6 }, { "wave", 0.25f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "decay", 0.3f }, { "sustain", 0 }, { "release", 0.05f } }) } },
        { "Click Sub", { sb ("Knock Sub", { { "engine", 6 }, { "wave", 0.3f }, { "fmAmt", 0.5f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "decay", 1.2f }, { "sustain", 0.4f } }) } },
        { "Swelling Sub", { sb ("Sine Sub", { { "engine", 6 }, { "wave", 0.1f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "attack", 0.25f }, { "sustain", 1 }, { "release", 0.3f } }) } },
        { "Squelch Bass", { sb ("Squelch Bass", { { "reso", 0.5f }, { "fenv", 0.5f }, { "fdecay", 0.12f }, { "cutoff", 260 }, { "decay", 0.3f }, { "sustain", 0.1f } }), subPart (0.7f) } },
        { "Rubber Bass", { sb ("Rubber Bass", { { "wave", 0.5f }, { "fenv", 0.5f }, { "fdecay", 0.08f }, { "cutoff", 200 }, { "reso", 0.3f } }), subPart (0.7f) } },
        { "Open Saw Bass", { sb ("Open Saw Bass", { { "cutoff", 1500 }, { "fenv", 0 }, { "reso", 0.05f } }), subPart (0.6f) } },
        { "Muffled Saw Bass", { sb ("Muffled Bass", { { "cutoff", 170 }, { "fenv", 0.05f }, { "reso", 0.1f } }) } },
        { "Swell Bass", { sb ("Swell Bass", { { "attack", 0.2f }, { "fattack", 0.35f }, { "fenv", 0.4f }, { "fdecay", 0.8f }, { "cutoff", 300 } }) } },
        { "Two Oscillator Bass", { sb ("Two Oscillator Bass", { { "unison", 2 }, { "detune", 0.15f }, { "wave", 0.35f }, { "cutoff", 600 } }) } },
        { "Moving Reese", { sb ("Reese Bass", { { "unison", 5 }, { "detune", 0.3f }, { "filterType", 1 }, { "cutoff", 600 }, { "fenv", 0 }, { "lfoFilter", 0.35f }, { "lfoRate", 0.35f } }), subPart (0.8f) } },
        { "Wide Reese", { sb ("Reese Bass", { { "unison", 7 }, { "detune", 0.45f }, { "filterType", 0 }, { "cutoff", 1100 }, { "fenv", 0 }, { "width", 0.6f } }), subPart (0.85f) } },
        { "Grit Wavetable Bass", { sb ("Wavetable Bass", { { "engine", 7 }, { "wave", 0.75f }, { "filterType", 1 }, { "cutoff", 700 }, { "fenv", 0.25f } }), subPart (0.7f) } },
        { "Square Pluck Bass", { sb ("Square Pluck Bass", { { "wave", 0.5f }, { "decay", 0.3f }, { "sustain", 0 }, { "fenv", 0.35f }, { "fdecay", 0.1f }, { "cutoff", 350 } }) } },
        { "Soft Round Bass", { sb ("Round Bass", { { "engine", 5 }, { "wave", 0.0f }, { "filterType", 0 }, { "cutoff", 2000 }, { "fenv", 0 }, { "sustain", 0.9f } }) } },
        { "Brass Synth Bass", { sb ("Brass Synth Bass", { { "engine", 8 }, { "wave", 0.0f }, { "filterType", 1 }, { "cutoff", 700 }, { "fenv", 0.3f }, { "fattack", 0.04f } }), subPart (0.6f) } },
        { "Wobble Bass", { sb ("Wobble Bass", { { "unison", 3 }, { "detune", 0.15f }, { "filterType", 1 }, { "cutoff", 500 }, { "reso", 0.35f }, { "macro2", 0.6f },
                                                { "lfoRate", 4.0f }, { "wobTarget", 0 } }), subPart (0.85f) } },
        { "Tremolo Bass", { sb ("Pulse Bass", { { "wave", 0.7f }, { "cutoff", 600 }, { "lfoAmp", 0.6f }, { "lfoRate", 6.0f }, { "lfoShape", 0 } }) } },
        { "Vibrato Sub", { sb ("Sine Sub", { { "engine", 6 }, { "wave", 0.2f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "sustain", 1 }, { "lfoPitch", 0.12f }, { "lfoRate", 5.0f } }) } },
        { "Bell FM Bass", { sb ("FM Bell Bass", { { "engine", 1 }, { "fmAlgo", 0 }, { "fmRatio", 3.5f }, { "fmAmt", 0.25f }, { "decay", 1.2f }, { "sustain", 0 },
                                                  { "filterType", 0 }, { "cutoff", 3000 }, { "fenv", 0 } }), subPart (0.6f) } },
    };
}

// bass treatments: apply to part 0 (synth keys only when part 0 is a synth)
struct BassTreat { juce::StringArray prefixes; KV fx; bool addSub = false; };
std::vector<BassTreat> bassTreatments()
{
    return {
        { { "" }, {} },
        { { "Short", "Tight", "Staccato" }, { { "decay", 0.3f }, { "sustain", 0.0f }, { "release", 0.06f }, { "s.decay", 0.35f }, { "s.sustain", 0.0f }, { "s.release", 0.08f },
                                             { "m.decay", 0.35f } } },
        { { "Driven", "Hot", "Burnt" }, { { "drive", 0.35f }, { "driveType", 1 }, { "velSens", 0.95f }, { "s.velSens", 0.9f }, { "m.velSens", 0.95f } } },
        { { "Dark", "Deep", "Shadow" }, { { "eqHigh", -7.0f }, { "s.cutoff", 900 }, { "m.tone", 0.2f } } },
        { { "Sub", "Heavy", "Floor" }, {}, true },
        { { "Bright", "Snappy", "Punchy" }, { { "eqHigh", 3.0f }, { "m.tone", 0.72f }, { "s.velCutoff", 0.3f } } },
        { { "Wide", "Chorus", "Lush" }, { { "chorus", 0.35f }, { "width", 0.7f } } },
        { { "Swelling", "Rising", "Blooming" }, { { "attack", 0.2f }, { "s.attack", 0.25f } } },
        { { "Ringing", "Sustained", "Long" }, { { "release", 0.9f }, { "s.release", 1.2f }, { "m.release", 1.0f }, { "m.decay", 2.0f } } },
    };
}

void addBasses (const RecipeContext& ctx, std::vector<Cand>& out, Names& names)
{
    std::vector<BassVariant> vars = synthBasses();
    auto m = [] (const char* id, const char* label, KV kv = {}) { auto p = with (recipe::modelPart (id, label), kv); p.octave = -2; return p; };
    auto s = [] (const char* id, const char* art, const char* label, int o, KV kv = {}) { auto p = with (recipe::samplePart (id, art, label), kv); p.octave = o; return p; };
    // bass guitars (models)
    const std::vector<BassVariant> models {
        { "Finger Bass", { m ("bass.finger", "Finger Bass") } },
        { "Muted Finger Bass", { m ("bass.finger", "Finger Bass", { { "m.artic", 1 } }) } },
        { "Bass Harmonics", { m ("bass.finger", "Finger Bass", { { "m.artic", 2 } }) } },
        { "Bridge Finger Bass", { m ("bass.finger", "Finger Bass", { { "m.pickup", 1.0f }, { "m.tone", 0.6f } }) } },
        { "Pick Bass", { m ("bass.pick", "Pick Bass") } },
        { "Muted Pick Bass", { m ("bass.pick", "Pick Bass", { { "m.artic", 1 } }) } },
        { "Overdriven Pick Bass", { m ("bass.pick", "Pick Bass", { { "m.drive", 0.6f } }) } },
        { "Slap Bass", { m ("bass.slap", "Slap Bass") } },
        { "Muted Slap Bass", { m ("bass.slap", "Slap Bass", { { "m.artic", 1 } }) } },
        { "Fretless Bass", { m ("bass.fretless", "Fretless Bass") } },
        { "Fretless Harmonics", { m ("bass.fretless", "Fretless Bass", { { "m.artic", 2 } }) } },
        { "Singing Fretless", { m ("bass.fretless", "Fretless Bass", { { "m.decay", 2.0f }, { "m.tone", 0.6f } }) } },
        { "Neck Finger Bass", { m ("bass.finger", "Finger Bass", { { "m.pickup", 0.0f }, { "m.tone", 0.3f } }) } },
        { "Ringing Finger Bass", { m ("bass.finger", "Finger Bass", { { "m.decay", 2.2f }, { "m.release", 0.6f } }) } },
        { "Thumb Bass", { m ("bass.finger", "Finger Bass", { { "m.pickPos", 0.4f }, { "m.tone", 0.22f } }) } },
        { "Bright Pick Bass", { m ("bass.pick", "Pick Bass", { { "m.tone", 0.75f }, { "m.pickPos", 0.06f } }) } },
        { "Fuzz Pick Bass", { m ("bass.pick", "Pick Bass", { { "m.drive", 1.0f } }) } },
        { "Popped Slap Bass", { m ("bass.slap", "Slap Bass", { { "m.tone", 0.72f } }) } },
        { "Dark Fretless", { m ("bass.fretless", "Fretless Bass", { { "m.tone", 0.22f } }) } },
        { "Growling Fretless", { m ("bass.fretless", "Fretless Bass", { { "m.drive", 0.45f } }) } },
        { "Tenor Finger Bass", { oct (m ("bass.finger", "Finger Bass"), -1) } },
        { "High Slap Bass", { oct (m ("bass.slap", "Slap Bass"), -1) } },
        { "Fretless Melody", { oct (m ("bass.fretless", "Fretless Bass", { { "m.decay", 1.6f } }), -1) } },
        { "Tenor Pick Bass", { oct (m ("bass.pick", "Pick Bass", { { "m.tone", 0.4f } }), -1) } },
    };
    vars.insert (vars.end(), models.begin(), models.end());
    // orchestral and acoustic basses (samples)
    struct SB { juce::String noun; VaultPart part; };
    std::vector<SB> samples {
        { "Contrabass Arco", s ("strings.contrabass", "sustain", "Contrabass", -2, { { "s.release", 0.35f } }) },
        { "Contrabass Vibrato", s ("strings.contrabass", "vibrato", "Contrabass", -2, { { "s.release", 0.35f } }) },
        { "Contrabass Tremolo", s ("strings.contrabass", "tremolo", "Contrabass", -2, { { "s.release", 0.3f } }) },
        { "Contrabass Pizzicato", s ("strings.contrabass", "pizzicato", "Contrabass", -2) },
        { "Contrabass Spiccato", s ("strings.contrabass", "spiccato", "Contrabass", -2) },
        { "Tuba", s ("brass.tuba", "sustain", "Tuba", -2, { { "s.release", 0.3f }, { "s.width", 0.4f } }) },
        { "Tuba Staccato", s ("brass.tuba", "staccato", "Tuba", -1, { { "s.width", 0.4f } }) },
        { "Bassoon", s ("winds.bassoon", "sustain", "Bassoon", -1, { { "s.release", 0.3f } }) },
        { "Bassoon Vibrato", s ("winds.bassoon", "vibrato", "Bassoon", -1, { { "s.release", 0.3f } }) },
        { "Bassoon Staccato", s ("winds.bassoon", "staccato", "Bassoon", -1) },
        { "Cello Pizzicato Bass", s ("strings.cellos.ensemble", "pizzicato", "Cello Section", -1) },
        { "Cello Spiccato Bass", s ("strings.cellos.ensemble", "spiccato", "Cello Section", -1) },
        { "Low Cello Section", s ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.release", 0.35f } }) },
        { "Low Trombone", s ("brass.trombone", "sustain", "Tenor Trombone", -1, { { "s.release", 0.3f } }) },
        { "Trombone Stab Bass", s ("brass.trombone", "staccato", "Tenor Trombone", -1, { { "s.cutoff", 2200 } }) },
        { "Pedal Organ Bass", s ("organ.pipe", "pedal", "Pipe Organ", -1, { { "s.release", 0.4f } }) },
        { "Soft Pedal Organ", s ("organ.pipe", "pedal soft", "Pipe Organ", -1, { { "s.release", 0.4f } }) },
        { "Grand Piano Bass", s ("piano.grand.concert", "sustain", "Concert Grand", -2) },
        { "Upright Piano Bass", s ("piano.upright", "sustain", "Upright Piano", -2) },
        { "Harp Bass", s ("pluck.harp.concert", "pluck", "Concert Harp", -2, { { "s.oneShot", 0 }, { "s.release", 0.5f } }) },
        { "Contrabass Swell", s ("strings.contrabass", "sustain", "Contrabass", -2, { { "s.attack", 0.4f }, { "s.release", 0.6f } }) },
        { "Tenor Contrabass Pizz", s ("strings.contrabass", "pizzicato", "Contrabass", -1) },
        { "Cello Tremolo Bass", s ("strings.cellos.ensemble", "tremolo", "Cello Section", -1, { { "s.release", 0.3f } }) },
        { "Studio Grand Bass", s ("piano.grand.studio", "sustain", "Studio Grand", -2) },
        { "Timpani Bass", s ("perc.timpani", "hit warm", "Timpani", -1) },
        { "Horn Bass", s ("brass.horn", "sustain", "French Horn", -1, { { "s.release", 0.3f } }) },
        { "Low Marimba Bass", s ("mallets.marimba", "hit", "Marimba", -1) },
        { "Thumb Piano Bass", s ("world.kalimba.low", "pluck", "Low Thumb Piano", -1) },
        { "Chamber Organ Bass", s ("organ.chamber", "8 foot", "Chamber Organ", -1, { { "s.release", 0.3f } }) },
    };
    for (auto& sb : samples)
        if (ctx.hasArticulation (sb.part.source, sb.part.articulation)) vars.push_back ({ sb.noun, { sb.part } });

    const auto treats = bassTreatments();
    for (int t = 0; t < (int) treats.size(); ++t)
        for (int vi = 0; vi < (int) vars.size(); ++vi)
        {
            const auto& v = vars[(size_t) vi];
            const auto& tr = treats[(size_t) t];
            // no contradictions: swells are not shortened, plucks / staccatos do not swell or ring on
            const bool shortSound = v.noun.containsIgnoreCase ("Pluck") || v.noun.containsIgnoreCase ("Staccato") || v.noun.containsIgnoreCase ("Spiccato")
                                    || v.noun.containsIgnoreCase ("Pizz") || v.noun.containsIgnoreCase ("Stab") || v.noun.containsIgnoreCase ("Short")
                                    || v.noun.containsIgnoreCase ("Knock") || v.noun.containsIgnoreCase ("Click") || v.noun.containsIgnoreCase ("Muted");
            const bool slowSound = v.noun.containsIgnoreCase ("Swell") || v.noun.containsIgnoreCase ("Long") || v.noun.containsIgnoreCase ("Ringing");
            if (tr.prefixes[0] == "Short" && slowSound) continue;
            if ((tr.prefixes[0] == "Swelling" || tr.prefixes[0] == "Ringing") && (shortSound || slowSound)) continue;
            auto parts = v.parts;
            auto& p0 = parts[0];
            bool hasSub = false;
            for (auto& p : parts) if (p.label == "Sub Bass" || p.source.isEmpty() && p.get ("engine", 0) == 6) hasSub = true;
            if (tr.addSub && (hasSub || parts.size() >= 4)) continue;
            bool applied = tr.addSub;
            for (auto& [k, val] : tr.fx)
            {
                const bool sKey = k.startsWith ("s."), mKey = k.startsWith ("m.");
                const bool synthOnly = ! sKey && ! mKey && (k == "decay" || k == "sustain" || k == "release" || k == "velSens" || k == "attack");
                if (sKey && p0.kind != VaultPartKind::sample) continue;
                if (mKey && p0.kind != VaultPartKind::model) continue;
                if (synthOnly && p0.kind != VaultPartKind::synth) continue;
                p0.set (k, val); applied = true;
            }
            if (! applied && t > 0) continue;
            for (auto& [k, val] : tr.fx)
                if (k == "velSens") for (auto& p : parts) if (p.kind == VaultPartKind::synth) p.set (k, val);
            if (tr.addSub) parts.push_back (oct (subPart (p0.kind == VaultPartKind::synth ? 0.7f : 0.55f), p0.octave));
            const auto name = names.make (tr.prefixes, v.noun, vi);
            if (name.isNotEmpty()) out.push_back ({ name, vault::catBass, parts });
        }

    // layered basses
    auto ok = [&] (const std::vector<VaultPart>& ps) { for (auto& p : ps) if (p.kind == VaultPartKind::sample && ! ctx.hasArticulation (p.source, p.articulation)) return false; return true; };
    struct L { juce::String name; std::vector<VaultPart> parts; };
    const std::vector<L> layers {
        { "Cello Octave Bass", { s ("strings.contrabass", "sustain", "Contrabass", -2, { { "s.release", 0.35f } }),
                                 lvl (s ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.release", 0.35f } }), 0.14f) } },
        { "Orchestral Pizz Sub", { s ("strings.contrabass", "pizzicato", "Contrabass", -2), subPart (0.55f) } },
        { "Low Brass Choir Bass", { s ("brass.tuba", "sustain", "Tuba", -2, { { "s.release", 0.3f }, { "s.width", 0.4f } }),
                                    lvl (s ("brass.trombone", "sustain", "Tenor Trombone", -1, { { "s.release", 0.3f } }), 0.2f) } },
        { "Bassoon and Pizz", { s ("strings.contrabass", "pizzicato", "Contrabass", -2), lvl (s ("winds.bassoon", "staccato", "Bassoon", -2), 0.6f) } },
        { "Piano Sub Bass", { s ("piano.grand.concert", "sustain", "Concert Grand", -2), subPart (0.6f) } },
        { "Organ Pedal Sub", { s ("organ.pipe", "pedal", "Pipe Organ", -1, { { "s.release", 0.4f } }), oct (subPart (0.5f), -1) } },
        { "Reese and Finger", { synthBass ("Reese Bass", { { "unison", 5 }, { "detune", 0.3f }, { "filterType", 1 }, { "cutoff", 600 }, { "fenv", 0.1f } }),
                                lvl (m ("bass.finger", "Finger Bass"), 0.7f), subPart (0.6f) } },
        { "Ladder and Pick", { synthBass ("Ladder Pluck Bass", { { "decay", 0.5f }, { "sustain", 0.2f }, { "fenv", 0.4f }, { "cutoff", 400 } }),
                               lvl (m ("bass.pick", "Pick Bass"), 0.7f) } },
        { "Tuba and Sub", { s ("brass.tuba", "staccato", "Tuba", -1), oct (subPart (0.55f), -1) } },
        { "Contrabass Stack", { s ("strings.contrabass", "pizzicato", "Contrabass", -2), lvl (m ("bass.finger", "Finger Bass", { { "m.tone", 0.35f } }), 0.6f), subPart (0.5f) } },
        { "Cinematic Low End", { s ("strings.contrabass", "sustain", "Contrabass", -2, { { "s.release", 0.4f } }),
                                 lvl (s ("brass.tuba", "sustain", "Tuba", -2, { { "s.release", 0.4f }, { "s.width", 0.4f } }), 0.55f), subPart (0.5f),
                                 lvl (s ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.release", 0.4f } }), 0.15f) } },
        { "Harp and Fretless", { m ("bass.fretless", "Fretless Bass"), lvl (s ("pluck.harp.concert", "pluck", "Concert Harp", -2, { { "s.oneShot", 0 }, { "s.release", 0.5f } }), 0.5f) } },
        { "Fretless and Cello", { m ("bass.fretless", "Fretless Bass"), lvl (s ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.attack", 0.15f }, { "s.release", 0.4f } }), 0.15f) } },
        { "Slap and Synth", { m ("bass.slap", "Slap Bass"), lvl (synthBass ("Ladder Square Bass", { { "wave", 0.5f }, { "cutoff", 400 }, { "decay", 0.4f }, { "sustain", 0.3f } }), 0.5f) } },
        { "Upright Piano Pizz", { s ("piano.upright", "sustain", "Upright Piano", -2), lvl (s ("strings.contrabass", "pizzicato", "Contrabass", -2), 0.6f) } },
        { "Growl Over Finger", { m ("bass.finger", "Finger Bass", { { "m.tone", 0.3f } }),
                                 lvl (synthBass ("Growl Bass", { { "unison", 3 }, { "detune", 0.2f }, { "filterType", 2 }, { "cutoff", 500 }, { "reso", 0.35f }, { "macro2", 0.3f }, { "lfoRate", 3 } }), 0.45f) } },
        { "Spiccato Tuba Bass", { s ("strings.contrabass", "spiccato", "Contrabass", -2), lvl (s ("brass.tuba", "staccato", "Tuba", -1), 0.25f) } },
        { "Reese and Contrabass", { s ("strings.contrabass", "sustain", "Contrabass", -2, { { "s.release", 0.35f } }),
                                    lvl (synthBass ("Reese Bass", { { "unison", 5 }, { "detune", 0.3f }, { "filterType", 1 }, { "cutoff", 500 }, { "fenv", 0 } }), 0.5f) } },
        { "Tuba and Ladder", { s ("brass.tuba", "staccato", "Tuba", -1, { { "s.width", 0.4f } }),
                               oct (lvl (synthBass ("Ladder Saw Bass", { { "cutoff", 400 }, { "fenv", 0.3f }, { "decay", 0.5f }, { "sustain", 0.3f } }), 0.5f), -1) } },
        { "Cello Pizz and Sub", { s ("strings.cellos.ensemble", "pizzicato", "Cello Section", -1), oct (subPart (0.6f), -1) } },
        { "Muted Pick and Sub", { m ("bass.pick", "Pick Bass", { { "m.artic", 1 } }), subPart (0.6f) } },
        { "Finger and Ladder Stack", { m ("bass.finger", "Finger Bass"), lvl (synthBass ("Ladder Square Bass", { { "wave", 0.5f }, { "cutoff", 420 } }), 0.45f), subPart (0.5f) } },
        { "Marimba Sub Bass", { s ("mallets.marimba", "hit", "Marimba", -1), oct (subPart (0.6f), -1) } },
        { "Organ and Contrabass", { s ("organ.pipe", "pedal soft", "Pipe Organ", -1, { { "s.release", 0.4f } }), lvl (s ("strings.contrabass", "sustain", "Contrabass", -2, { { "s.release", 0.4f } }), 0.5f) } },
        { "Bassoon and Cello Line", { s ("winds.bassoon", "sustain", "Bassoon", -1, { { "s.release", 0.3f } }), lvl (s ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.release", 0.3f } }), 0.5f) } },
        { "Slap and Sub", { m ("bass.slap", "Slap Bass"), subPart (0.55f) } },
        { "Timpani and Sub", { s ("perc.timpani", "hit warm", "Timpani", -1), oct (subPart (0.5f), -1) } },
        { "FM and Sub", { synthBass ("FM Bass", { { "engine", 1 }, { "fmRatio", 1 }, { "fmAmt", 0.45f }, { "decay", 0.5f }, { "sustain", 0.3f }, { "filterType", 0 }, { "cutoff", 2500 }, { "fenv", 0 } }),
                          subPart (0.7f) } },
    };
    for (auto& l : layers)
        if (ok (l.parts) && names.used.insert (l.name.toLowerCase()).second) out.push_back ({ l.name, vault::catBass, l.parts });
}
// C5 comfort: the sounding fundamental within ~2 dB of the strongest harmonic. Measured on a C5 render (as the gate does);
// basses get a sine part at part 0's octave (or a louder one) - the band-split practice - guitars a low shelf on the FX.
float fundamentalDb (const VaultPreset& p)
{
    const auto b = recipegate::render (p, vault::homeNote, 0.8f, 1.0, 0.8);
    const auto sp = recipegate::spectrum (b, 0.05);
    const float f0 = recipegate::soundingHz (p, vault::homeNote);
    const float h1 = recipegate::peakDb (sp, f0 * 0.97, f0 * 1.03);
    float strongest = h1;
    for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, recipegate::peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
    return h1 - strongest;
}
void fitFundamental (VaultPreset& p)
{
    constexpr float want = -2.0f;
    float h = fundamentalDb (p);
    if (h >= want) return;
    auto& p0 = p.parts[0];
    if (p.category != vault::catBass)
    {
        const float base = p0.get ("eqLow", 0.0f);
        for (float add : { 3.0f, 6.0f })
        {
            if (base + add > 9.0f) break;
            p0.set ("eqLow", base + add);
            if ((h = fundamentalDb (p)) >= want) return;
        }
        return;
    }
    int sub = -1;
    for (int k = 1; k < (int) p.parts.size(); ++k)
        if (p.parts[(size_t) k].kind == VaultPartKind::synth && p.parts[(size_t) k].get ("engine", 0) == 6 && p.parts[(size_t) k].octave == p0.octave) sub = k;
    if (sub < 0)
    {
        if (p.parts.size() >= vault::maxParts || (p0.kind == VaultPartKind::synth && p0.get ("engine", 0) == 6)) return;
        auto sp = oct (subPart (0.25f), p0.octave);
        if (p0.octave > -2) sp.label = "Sine Bass";
        p.parts.push_back (sp);
        sub = (int) p.parts.size() - 1;
    }
    for (int i = 0; i < 10 && (h = fundamentalDb (p)) < want; ++i)
    {
        auto& l = p.parts[(size_t) sub].level;
        if (l >= 2.0f) break;
        l = std::min (2.0f, l * 1.35f);
    }
}
} // namespace

std::vector<VaultPreset> build_strings (const RecipeContext& ctx)
{
    std::vector<Cand> cands;
    Names names;
    addGuitars (ctx, cands, names);
    addBasses (ctx, cands, names);
    std::vector<VaultPreset> out;
    int nG = 0, nB = 0;
    for (auto& c : cands)
    {
        VaultPreset p;
        p.category = c.category;
        p.id = c.category == vault::catGuitars ? "cv.gtr." + juce::String (++nG).paddedLeft ('0', 4) : "cv.bass." + juce::String (++nB).paddedLeft ('0', 4);
        p.name = c.name;
        p.parts = c.parts;
        out.push_back (std::move (p));
    }
    // fit the fundamental in parallel (each render has its own engine)
    std::atomic<int> next { 0 };
    std::vector<std::thread> pool;
    for (int t = 0; t < 3; ++t)
        pool.emplace_back ([&] { for (int i; (i = next++) < (int) out.size();) fitFundamental (out[(size_t) i]); });
    for (auto& t : pool) t.join();
    return out;
}

VAULT_RECIPE (strings, build_strings);
