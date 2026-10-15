// CHROME VAULT recipes: GUITARS + BASS (sound designer "strings"). Run: VaultBankGen -recipe strings (RecipeGate.h).
//
// v3 DESIGN NOTE - every preset ships finished (namespace v3 below)
//  * Research: commercial guitar instruments ship each patch through an "amp / studio" chain the player can switch off -
//    compressor, EQ, saturation, chorus / phaser / flanger, a tempo delay and a reverb with presets, pre-delay and width
//    (nylon instruments: compression + saturation + reverb send on the guitar page; steel / 12-string libraries: variation
//    patches with delays, phasers, choruses and a stereo-width control). Clean electric: light compression, a touch of amp
//    breakup, a 100-140 ms single-repeat slapback (~15 %) and a short spring-like reverb; tremolo is the amp's. Bass mixing
//    practice: a fast + a slow compressor, saturation on the upper band only (the sub stays clean and mono so it still
//    reads on phones), a mud cut around 250-350 Hz, a little low shelf, and essentially no reverb on the low end.
//  * So each preset's chain = FAMILY (the instrument's finished sound) + ROLE (the v2 treatment, now a real processing
//    style) with the slots in the canonical rack order:
//      nylon     parallel comp, a whisper of tape, box cut at 380 Hz, small room          (macros Pick, Body, Room, Glow)
//      steel     comp, mud cut at 250 Hz, width with mono lows, plate                     (Pick, Body, Space, Double)
//      electric  clean comp, tube breakup with clean lows, mid push, slapback, short plate, amp tremolo LFO ready
//                                                                                           (Pick, Body, Space, Tremolo)
//      archtop   gentle comp + tube warmth, dark top, room;  12-string: comp, light chorus, wide plate
//      crunch    pedal drive in front of the amp model, cabinet roll-off, 1/8 delay, plate  (Tone, Body, Space, Gain)
//      banjo     comp, twang cut at 2.5 kHz, room;  harps: hall with pre-delay           (Brightness, Body, Space, Release)
//    roles: Echo = 1/8D ping-pong (electric: 1/4 tape echo) + hall; Shimmer = chorus + width + plate; Dreamy = cloud
//    reverb + 1/4D delay + width + a slow cutoff / pan drift LFO; Velvet = dark shelf + damped hall; Swirl = phaser + plate;
//    Jet = flanger + hall; Dusty = tape drive + lo-fi + a tape-wobble pitch LFO + dark room; Ghost / Undertow = ghost bus +
//    hall; Cathedral = long hall, high low-cut; Wooden = wood body + room; Reverse = reverse + hall. The role decides the
//    4th macro (Chorus, Phase, Flange, Tape, Ghost, Wood, Reverse, Drift, Shade) and the name of the space macro (Room,
//    Space, Echo, Haze, Hall). Pad-layered guitars get the slow drift LFO. Mod wheel = string vibrato on every guitar.
//    Bass families: sine subs (transient, comp, tape harmonics, low shelf - Tone, Harmonics, Weight, Punch), ladder /
//    analog (comp, tube drive with clean lows, mud cut - Cutoff, Drive, Sub, Envelope), reese / growl (drive, mid cut,
//    width with mono lows, a slow cutoff LFO - Cutoff, Drive, Sub, Motion), dirty (hard drive, mid bite - ... Bite),
//    digital / FM (comp, tape, transient - Tone, Drive, Sub, Punch), bass guitars (comp, amp blend with clean lows, mud
//    cut, slap scoop + transient, fretless chorus - Tone, Amp, Weight, Punch / Chorus), orchestral basses (gentle comp,
//    small hall with a 300 Hz low cut - Tone, Hall, Weight, Bow / Release, mod wheel vibrato), keys / mallets played low
//    (comp, small room - Tone, Room, Weight, Release). Synth basses: mod wheel opens the cutoff; velocity adds level.
//  * Macros: every target sits exactly at its designed value at the knob's default (min / max are placed around it), so
//    the preset loads as designed. Each macro is rendered at 0 and at 1 (C5) and a side that breaks a comfort rule the
//    designed sound keeps (2-5 kHz, > 6 / 8 kHz, fundamental, clipping, +4 dB louder, longer tail) is shortened until it
//    passes; drive macros trim the drive's output as they go up, so they add colour, not level.
//  * Arp / gate: off everywhere except 7 pattern candidates with plain names (Nylon Arpeggio, Harp Arpeggio, Muted Steel
//    Eighths, Gated Electric Pad, Acid Bass Sixteenths, Reese Eighths, Gated Sub Eighths), appended after the v2 presets so
//    the ids stay. Patterns start from Init and only change rate / mode / gate / accents.
//  * Result (VaultBankGen -recipe strings, library /home/user/cv-library): 2135 candidates -> 173 Guitars + 205 Bass
//    (6 of them rhythm presets; Gated Electric Pad is a near-duplicate), min pairwise distance 3.50; at C5 fundamental
//    >= -2.0 dB of the strongest partial, 2-5 kHz <= 3.9 %, > 6 kHz <= 0.38 %, > 8 kHz <= 0.30 %, velocity range >= 3.0 dB,
//    tails <= 9.2 s; bass centroid <= 443 Hz. 1,512 macros checked at 0 and 1: 282 ranges narrowed, none left outside the
//    comfort rules, 61 (mostly Tone / Pick on very dark sounds) change the sound only slightly.
//  * Sources: violetrecording.com "How to Mix Bass" / "How to Mix Bass Guitar"; musicguymixing.com "How to Mix Bass";
//    nailthemix.com (EQ - drive - EQ order); thomann product pages of nylon / steel guitar instruments (built-in comp,
//    saturation, reverb / delay pages); musicmatter.co.uk (variation patches with delays, phasers, choruses, width);
//    forum.kemper-amps.com slapback settings; guitarchalk.com clean amp settings.
//
// v2 DESIGN NOTE
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
#include <map>
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

// what a candidate is (drives the v3 processing): instrument family, treatment (role), the variant noun, layered or not,
// and an optional rhythmic pattern (arp / gate presets)
struct Role
{
    juce::String fam, treat = "plain", noun;
    bool layer = false;
    int pattern = 0;
};
struct Cand
{
    juce::String name;
    int category = vault::catGuitars;
    std::vector<VaultPart> parts;
    Role role;
};
juce::String famOf (const juce::String& source)
{
    if (source.startsWith ("guitar.")) return source.fromFirstOccurrenceOf ("guitar.", false, false);
    if (source.startsWith ("pluck.harp")) return "harp";
    return "other";
}

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
                out.push_back ({ name, vault::catGuitars, { p }, { famOf (in.source), pre0.isEmpty() ? juce::String ("plain") : pre0, v.noun } });
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
            if (name.isNotEmpty())
            {
                const auto& t0 = treats[(size_t) t].prefixes[0];
                out.push_back ({ name, vault::catGuitars, parts, { famOf (parts[0].source), t0.isEmpty() ? juce::String ("plain") : t0, l.noun, true } });
            }
        }
    // the few presets where a rhythm is the point of the sound (arp / gate on, plain musical names, appended last so the
    // ids of the v2 presets stay)
    auto pat = [&] (const char* name, std::vector<VaultPart> parts, const char* fam, int pattern)
    {
        for (auto& p : parts) if (p.kind == VaultPartKind::sample && ! ctx.hasArticulation (p.source, p.articulation)) return;
        if (names.used.insert (juce::String (name).toLowerCase()).second) out.push_back ({ name, vault::catGuitars, parts, { fam, "plain", name, parts.size() > 1, pattern } });
    };
    pat ("Nylon Arpeggio", { m ("guitar.nylon", "Nylon Guitar", { { "m.pickPos", 0.3f } }) }, "nylon", 1);
    pat ("Harp Arpeggio", { s ("pluck.harp.concert", "pluck", "Concert Harp") }, "harp", 2);
    pat ("Muted Steel Eighths", { m ("guitar.acoustic", "Steel Acoustic Guitar", { { "m.artic", 1 } }) }, "acoustic", 3);
    pat ("Gated Electric Pad", { m ("guitar.electric", "Clean Electric Guitar", { { "m.pickup", 0.2f } }), pad (0.35f) }, "electric", 4);
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

// the processing family of a bass (by its part 0)
juce::String bassFamily (const std::vector<VaultPart>& parts, const juce::String& noun)
{
    const auto& p0 = parts.front();
    if (p0.kind == VaultPartKind::model) return p0.source.fromFirstOccurrenceOf ("bass.", false, false);   // finger, pick, slap, fretless
    if (p0.kind == VaultPartKind::sample)
        return p0.source.startsWith ("strings.") || p0.source.startsWith ("brass.") || p0.source.startsWith ("winds.") || p0.source.startsWith ("perc.") ? "orch" : "keys";
    if ((int) p0.get ("engine", 0) == 6) return "sub";
    for (auto* k : { "Reese", "Growl", "Wobble" }) if (noun.containsIgnoreCase (k)) return "reese";
    for (auto* k : { "Fuzz", "Folded", "Crushed", "Bitcrushed", "Tape" }) if (noun.containsIgnoreCase (k)) return "dirty";
    return (int) p0.get ("engine", 0) == 0 ? "analog" : "digital";
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
            if (name.isNotEmpty()) out.push_back ({ name, vault::catBass, parts, { bassFamily (parts, v.noun), tr.prefixes[0].isEmpty() ? juce::String ("plain") : tr.prefixes[0], v.noun } });
        }

    // layered basses
    auto ok = [&] (const std::vector<VaultPart>& ps) { for (auto& p : ps) if (p.kind == VaultPartKind::sample && ! ctx.hasArticulation (p.source, p.articulation)) return false; return true; };
    struct L { juce::String name; std::vector<VaultPart> parts; };
    const std::vector<L> layers {
        { "Cello Octave Bass", { s ("strings.contrabass", "sustain", "Contrabass", -2, { { "s.release", 0.35f } }),
                                 lvl (s ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.release", 0.35f } }), 0.1f) } },
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
                                 lvl (s ("strings.cellos.ensemble", "sustain", "Cello Section", -1, { { "s.release", 0.4f } }), 0.11f) } },
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
        if (ok (l.parts) && names.used.insert (l.name.toLowerCase()).second) out.push_back ({ l.name, vault::catBass, l.parts, { bassFamily (l.parts, l.name), "plain", l.name, true } });
    // rhythmic basses (arp / gate on), appended last
    auto pat = [&] (const char* name, std::vector<VaultPart> parts, int pattern)
    {
        if (names.used.insert (juce::String (name).toLowerCase()).second) out.push_back ({ name, vault::catBass, parts, { bassFamily (parts, name), "plain", name, parts.size() > 1, pattern } });
    };
    pat ("Acid Bass Sixteenths", { synthBass ("Acid Bass", { { "reso", 0.55f }, { "fenv", 0.5f }, { "fdecay", 0.14f }, { "cutoff", 300 }, { "decay", 0.4f }, { "sustain", 0.3f } }),
                                   subPart (0.7f) }, 5);
    pat ("Reese Eighths", { synthBass ("Reese Bass", { { "unison", 5 }, { "detune", 0.3f }, { "filterType", 1 }, { "cutoff", 650 }, { "fenv", 0.1f } }), subPart (0.8f) }, 6);
    pat ("Gated Sub Eighths", { synthBass ("Sine Sub", { { "engine", 6 }, { "wave", 0.25f }, { "filterType", 0 }, { "cutoff", 20000 }, { "fenv", 0 }, { "sustain", 1 }, { "decay", 1 } }) }, 7);
}
//==============================================================================
// v3 SOUND DESIGN (see the v3 design note at the top): FX chain by family + role, movement, 4 named sound macros,
// arp / gate on the few pattern presets
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
    explicit Chain (const std::vector<VaultFxSlot>& converted)
    {
        for (auto& x : converted) if (! has (x.type)) put (x);
    }
    static int rank (FT t) { return vault::fxTypeInfo (t).rank; }
    bool has (FT t) const { return slots.count (rank (t)) > 0; }
    VaultFxSlot& put (const VaultFxSlot& x) { return slots[rank (x.type)] = x; }
    VaultFxSlot& at (FT t) { if (! has (t)) put (VaultFxSlot::make (t)); return slots[rank (t)]; }
    void drop (FT t) { slots.erase (rank (t)); }
    float get (FT t, const char* id) const { return has (t) ? slots.at (rank (t)).get (id) : VaultFxSlot::make (t).get (id); }
    // add dB to an EQ gain (clamped to +-18)
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
    // relative target (offset 0 at the default)
    M& rel (const char* target, float span) { m.targets.push_back ({ target, -span * d, span * (1.0f - d), 0.0f }); return *this; }
    // absolute target with designed value v, kept inside lo..hi (the span shrinks to fit, the designed point stays)
    M& abs (const char* target, float v, float span, float lo, float hi)
    {
        if (d > 1.0e-4f) span = std::min (span, (v - lo) / d);
        if (d < 1.0f - 1.0e-4f) span = std::min (span, (hi - v) / (1.0f - d));
        span = std::max (0.0f, span);
        m.targets.push_back ({ target, v - span * d, v + span * (1.0f - d), 0.0f });
        return *this;
    }
    // absolute target: designed value v at the default, `atMax` at 1 (a straight line through both; e.g. a drive's output
    // trim that keeps the level when the drive goes up)
    M& toMax (const char* target, float v, float atMax)
    {
        const float span = d < 1.0f - 1.0e-4f ? (atMax - v) / (1.0f - d) : 0.0f;
        m.targets.push_back ({ target, v - span * d, atMax, 0.0f });
        return *this;
    }
    // an FX parameter of the chain's slot of that type
    M& fxp (const Chain& c, FT t, const char* id, float span)
    {
        const auto& ti = vault::fxTypeInfo (t);
        const auto& pi = ti.params[(size_t) ti.indexOf (id)];
        const auto target = juce::String ("fx.") + ti.id + "." + id;
        return abs (target.toRawUTF8(), c.get (t, id), span, pi.lo, pi.hi);
    }
};
void setMacro (VaultPreset& p, int i, const M& m) { p.macros[(size_t) i] = m.m; p.macroDefaults[i] = m.d; }

// LFO roles
VaultLfo ampTremolo (float rate = 5.0f) { return lfo (VaultLfo::sine).rate (rate).depth (0.0f).to ("volume", 0.45f); }
VaultLfo drift (float depth, float rate, float cutoffOct, float pan)
{
    return lfo (VaultLfo::sine).rate (rate).free().depth (depth).to ("cutoff", cutoffOct).to ("pan", pan);
}
VaultLfo tapeWobble (float depth, float rate = 0.55f) { return lfo (VaultLfo::triangle).rate (rate).free().depth (depth).to ("pitch", 0.08f); }

int subLayer (const VaultPreset& p)
{
    for (int k = 1; k < (int) p.parts.size(); ++k)
        if (p.parts[(size_t) k].kind == VaultPartKind::synth && (int) p.parts[(size_t) k].get ("engine", 0) == 6) return k;
    return -1;
}
bool sustained (const juce::String& noun)
{
    for (auto* k : { "Pizz", "Spiccato", "Staccato", "Stab", "Timpani", "Marimba", "Thumb", "Piano", "Harp" }) if (noun.containsIgnoreCase (k)) return false;
    return true;
}

//------------------------------------------------------------------------------ GUITARS
void designGuitar (VaultPreset& p, const Role& r)
{
    design (p);   // v2 -> v3: the treatment's colour slots (chorus, phaser, flanger, lo-fi, body, reverse, ghost, EQ) arrive here
    Chain c (p.fx);
    const auto& f = r.fam;
    const auto& t = r.treat;
    const bool sample = p.parts.front().kind == VaultPartKind::sample;
    float finish = 0.08f;
    p.mod = {};

    // 1. the instrument's finished sound (what a commercial patch puts on the close mic)
    auto mid = [&] (float db, float hz, float q) { auto& e = c.at (FT::eq); e.set ("mid", db).set ("midFreq", hz).set ("midQ", q); };
    if (f == "nylon")
    {
        c.put (fx::comp (-24, 2.0f, 18, 160, 1.5f, 0.6f));
        c.put (fx::drive (fx::Drive::tape, 0.05f, 0.4f, 9000));
        c.eqAdd ("low", 1.0f); mid (-1.5f, 380, 0.9f); c.eqAdd ("high", -0.5f);
        c.put (rv (R::room, 0.4f, 0.9f, 0.15f, 8, 0.2f));
    }
    else if (f == "acoustic")
    {
        c.put (fx::comp (-22, 2.5f, 15, 140, 1.5f, 0.7f));
        mid (-2.0f, 250, 0.8f); c.eqAdd ("high", -1.0f);
        c.put (fx::width (0.6f, 150));
        c.put (rv (R::plate, 0.45f, 0.9f, 0.14f, 10, 0.2f));
    }
    else if (f == "electric")
    {
        c.put (fx::comp (-24, 3.0f, 6, 120, 2.0f, 1.0f));
        c.put (fx::drive (fx::Drive::tube, 0.1f, 0.6f, 6000).set ("cleanLow", 1.0f));
        c.eqAdd ("low", -1.0f); mid (1.0f, 900, 0.7f); c.eqAdd ("high", -2.0f);
        c.put (fx::delayMs (115, 0.05f, 0.11f).set ("hiCut", 4000.0f).set ("loCut", 200.0f));   // slapback
        c.put (rv (R::plate, 0.5f, 0.8f, 0.13f, 4, 0.3f));                                    // spring-like short plate
        p.mod.lfo[0] = ampTremolo();
    }
    else if (f == "jazz")
    {
        c.put (fx::comp (-22, 2.0f, 20, 200, 1.0f, 0.6f));
        c.put (fx::drive (fx::Drive::tube, 0.05f, 0.5f, 5000));
        c.eqAdd ("low", 1.0f); mid (-1.0f, 450, 0.8f); c.eqAdd ("high", -3.0f);
        c.put (rv (R::room, 0.45f, 1.0f, 0.16f, 6, 0.35f));
    }
    else if (f == "twelve")
    {
        c.put (fx::comp (-22, 2.5f, 15, 150, 1.0f, 0.7f));
        if (! c.has (FT::chorus)) c.put (fx::chorus (0.12f, 0.25f, 0.3f));
        mid (-1.5f, 300, 0.8f); c.eqAdd ("high", -1.5f);
        c.put (fx::width (0.65f, 150));
        c.put (rv (R::plate, 0.5f, 1.0f, 0.16f, 12, 0.2f));
    }
    else if (f == "crunch")
    {
        c.put (fx::drive (fx::Drive::tube, 0.04f, 1.0f, 7000));   // the pedal in front of the amp model (its "Gain" macro)
        mid (0.0f, 1100, 0.8f); c.eqAdd ("high", -4.0f); c.at (FT::eq).set ("highFreq", 5000.0f);   // speaker-cabinet roll-off
        c.put (fx::delay ("1/8", 0.25f, 0.1f, fx::Delay::straight).set ("hiCut", 3500.0f).set ("loCut", 300.0f));
        c.put (rv (R::plate, 0.45f, 0.9f, 0.12f, 8, 0.3f));
        finish = 0.05f;
    }
    else if (f == "banjo")
    {
        c.put (fx::comp (-22, 2.0f, 10, 120, 1.0f, 0.6f));
        mid (-1.0f, 2500, 1.2f); c.eqAdd ("high", -2.0f);
        c.put (rv (R::room, 0.35f, 0.85f, 0.13f, 4, 0.3f));
    }
    else   // harps
    {
        c.eqAdd ("low", 0.5f); c.eqAdd ("high", -1.0f);
        c.put (rv (R::hall, 0.6f, 1.1f, 0.2f, 20, 0.15f));
        finish = 0.1f;
    }

    // 2. the role (treatment): spaces and colours
    bool drifting = false, wobbly = false;
    if (t == "Echo")
    {
        c.put (fx::delay (f == "electric" || f == "crunch" ? "1/4" : "1/8D", 0.45f, 0.28f, f == "electric" ? fx::Delay::tape : fx::Delay::pingPong)
                   .set ("hiCut", 4500.0f).set ("loCut", 250.0f));
        c.put (rv (R::hall, 0.5f, 1.0f, 0.12f, 12, 0.25f));
    }
    else if (t == "Shimmer")
    {
        auto& ch = c.at (FT::chorus);
        ch.set ("mix", std::max (0.5f, ch.get ("mix"))).set ("rate", 0.5f).set ("depth", 0.65f);
        c.put (fx::width (0.75f, 150));
        c.put (rv (R::plate, 0.55f, 1.1f, 0.16f, 14, 0.15f));
    }
    else if (t == "Dreamy")
    {
        c.put (rv (R::cloud, 0.55f, 0.8f, 0.28f, 25, 0.35f, 220));
        c.put (fx::delay ("1/4D", 0.25f, 0.12f).set ("hiCut", 4000.0f).set ("loCut", 300.0f));
        c.put (fx::width (0.66f, 150));
        drifting = true;
    }
    else if (t == "Velvet")
    {
        c.at (FT::eq).set ("highFreq", 4000.0f);
        if (c.has (FT::compressor)) c.at (FT::compressor).set ("mix", 0.8f);
        c.put (rv (R::hall, 0.55f, 1.0f, 0.2f, 18, 0.5f));
    }
    else if (t == "Swirl")
    {
        c.at (FT::phaser).set ("rate", 0.28f).set ("feedback", 0.45f);
        c.put (rv (R::plate, 0.5f, 1.0f, 0.15f, 10, 0.2f));
    }
    else if (t == "Jet")
    {
        c.at (FT::flanger).set ("rate", 0.16f).set ("feedback", 0.5f);
        c.put (rv (R::hall, 0.55f, 1.0f, 0.15f, 12, 0.25f));
    }
    else if (t == "Dusty")
    {
        const bool ringing = ! (r.noun.containsIgnoreCase ("Muted") || r.noun.containsIgnoreCase ("Chicken") || r.noun.containsIgnoreCase ("Chug")
                                || r.noun.containsIgnoreCase ("Staccato") || r.noun.containsIgnoreCase ("Short"));
        c.put (fx::drive (fx::Drive::tape, ringing ? 0.18f : 0.08f, 0.7f, 5500));
        c.at (FT::eq).set ("highFreq", 4500.0f);
        c.put (rv (R::room, 0.4f, 0.9f, 0.13f, 6, 0.5f));
        c.drop (FT::delay);
        wobbly = ringing;
    }
    else if (t == "Ghost" || t == "Undertow")
        c.put (rv (R::hall, 0.6f, 1.1f, 0.14f, 20, 0.3f));
    else if (t == "Cathedral")
    {
        c.put (rv (R::hall, 0.85f, 1.4f, 0.34f, 35, 0.2f, 220));
        c.drop (FT::delay);
    }
    else if (t == "Wooden")
        c.put (rv (R::room, 0.35f, 0.8f, 0.12f, 4, 0.35f));
    else if (t == "Reverse")
    {
        c.put (rv (R::hall, 0.6f, 1.1f, 0.18f, 15, 0.25f));
        c.drop (FT::delay);
    }

    // layers with a pad bed: a slow brightness drift brings the bed to life
    if (r.layer && p.parts.size() > 1)
        for (auto& part : p.parts) if (part.label == "Soft Pad") drifting = true;
    if (drifting) p.mod.lfo[1] = drift (0.5f, 0.12f, 0.25f, 0.3f);
    if (wobbly) p.mod.lfo[1] = tapeWobble (0.5f);

    // 3. the rhythm presets
    if (r.pattern == 1) p.arp = arp (VaultArp::upDown, "1/16", 1, 0.85f, 0.0f, true);
    else if (r.pattern == 2) p.arp = arp (VaultArp::up, "1/16", 1, 0.9f, 0.0f, true);
    else if (r.pattern == 3)
    {
        p.arp = arp (VaultArp::asPlayed, "1/8", 1, 0.55f, 0.0f, true);
        for (int i = 0; i < 16; ++i) p.arp.vel[(size_t) i] = i % 2 == 0 ? 1.0f : 0.75f;
    }
    else if (r.pattern == 4)
    {
        p.gate = gate ("1/8", { 1, 0.15f, 1, 0.15f, 1, 1, 0.15f, 1 }, 0.9f, 0.3f, 1.0f, true);
        c.put (rv (R::plate, 0.55f, 1.1f, 0.18f, 14, 0.2f));
    }

    // the slot the family's character macro opens (silent as designed)
    const bool famMacro = ! (t == "Shimmer" || t == "Swirl" || t == "Jet" || t == "Dusty" || t == "Ghost" || t == "Undertow" || t == "Wooden"
                             || t == "Reverse" || t == "Dreamy" || t == "Velvet" || r.pattern == 4 || f == "electric" || f == "crunch" || sample);
    if (famMacro && (f == "acoustic" || f == "twelve") && ! c.has (FT::chorus)) c.put (fx::chorus (0.0f, 0.4f, 0.45f));
    if (famMacro && (f == "acoustic" || f == "twelve") && ! c.has (FT::width)) c.put (fx::width (0.5f, 150));
    if (famMacro && ! (f == "acoustic" || f == "twelve" || f == "banjo") && ! c.has (FT::drive)) c.put (fx::drive (fx::Drive::tape, 0.0f, 0.0f, 9000));
    if ((sample || f == "banjo") && ! c.has (FT::transient)) c.put (fx::transient (0.1f));
    p.fx = c.build();
    p.finish = finish;
    for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; }

    // 4. sound macros
    // Tone: brightness of the strings (model tone low-pass / sample filter) + the top shelf
    // (the model strings' tone low-pass starts at 20 kHz, so the cutoff only darkens; the EQ tilt brightens)
    setMacro (p, 0, M (f == "crunch" ? "Tone" : sample ? "Brightness" : "Pick", 0.6f).rel ("cutoff", sample ? 6.0f : 8.0f).fxp (c, FT::eq, "tilt", 6.0f));
    // Body: low shelf + the box / mud band
    setMacro (p, 1, M ("Body", 0.5f).fxp (c, FT::eq, "low", 10.0f).fxp (c, FT::eq, "mid", 4.0f));
    // Space: reverb (+ delay) amount, and the reverb gets longer with it
    {
        const char* nm = t == "Echo" ? "Echo" : t == "Dreamy" ? "Haze" : t == "Cathedral" ? "Hall" : (c.get (FT::reverb, "type") == (float) R::room ? "Room" : "Space");
        M m (nm, 0.4f);
        m.fxp (c, FT::reverb, "mix", 0.5f).fxp (c, FT::reverb, "decay", 1.2f);
        if (c.has (FT::delay)) m.fxp (c, FT::delay, "mix", t == "Echo" ? 0.4f : 0.3f);
        setMacro (p, 2, m);
    }
    // the character macro of the role / family
    {
        auto has = [&] (FT x) { return c.has (x); };
        if (t == "Shimmer" && has (FT::chorus)) setMacro (p, 3, M ("Chorus", 0.5f).fxp (c, FT::chorus, "mix", 0.8f).fxp (c, FT::chorus, "depth", 0.6f));
        else if (t == "Swirl" && has (FT::phaser)) setMacro (p, 3, M ("Phase", 0.5f).fxp (c, FT::phaser, "mix", 0.8f).fxp (c, FT::phaser, "rate", 0.8f));
        else if (t == "Jet" && has (FT::flanger)) setMacro (p, 3, M ("Flange", 0.5f).fxp (c, FT::flanger, "mix", 0.7f).fxp (c, FT::flanger, "feedback", 0.6f));
        else if (t == "Dusty" && has (FT::lofi))
            setMacro (p, 3, M ("Tape", 0.5f).fxp (c, FT::lofi, "wow", 0.8f).fxp (c, FT::lofi, "crush", 0.5f).fxp (c, FT::drive, "amount", 0.3f).abs ("lfo2.depth", 0.5f, 1.0f, 0, 1)
                                          .toMax ("fx.drive.output", 0.0f, -2.0f));
        else if ((t == "Ghost" || t == "Undertow") && has (FT::ghost)) setMacro (p, 3, M ("Ghost", 0.5f).fxp (c, FT::ghost, "mix", 0.8f).fxp (c, FT::ghost, "blur", 0.6f));
        else if (t == "Wooden" && has (FT::body)) setMacro (p, 3, M ("Wood", 0.5f).fxp (c, FT::body, "mix", 0.8f));
        else if (t == "Reverse" && has (FT::reverse)) setMacro (p, 3, M ("Reverse", 0.5f).fxp (c, FT::reverse, "mix", 0.8f));
        else if (t == "Dreamy") setMacro (p, 3, M ("Drift", 0.5f).abs ("lfo2.depth", 0.5f, 1.0f, 0, 1).fxp (c, FT::width, "width", 0.4f));
        else if (t == "Velvet") setMacro (p, 3, M ("Shade", 0.5f).fxp (c, FT::eq, "high", 8.0f).fxp (c, FT::reverb, "damping", 0.8f));
        else if (r.pattern == 4) setMacro (p, 3, M ("Gate", 0.9f).abs ("gate.depth", 0.9f, 0.9f, 0, 1));
        else if (f == "electric") setMacro (p, 3, M ("Tremolo", 0.0f).abs ("lfo1.depth", 0.0f, 1.0f, 0, 1));
        else if (f == "crunch") setMacro (p, 3, M ("Gain", 0.25f).fxp (c, FT::drive, "amount", 0.4f).fxp (c, FT::eq, "mid", 3.0f).toMax ("fx.drive.output", 0.0f, -4.0f));
        else if (sample) setMacro (p, 3, M ("Pluck", 0.2f).fxp (c, FT::transient, "attack", 0.7f));
        else if (f == "banjo") setMacro (p, 3, M ("Snap", 0.2f).fxp (c, FT::transient, "attack", 0.7f).fxp (c, FT::compressor, "attack", 30.0f));
        else if (f == "acoustic" || f == "twelve")
            setMacro (p, 3, M ("Double", 0.2f).fxp (c, FT::chorus, "mix", 0.5f).fxp (c, FT::width, "width", 0.3f));
        else setMacro (p, 3, M ("Glow", 0.2f).fxp (c, FT::drive, "amount", 0.45f).fxp (c, FT::drive, "mix", 0.5f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -5.0f));
    }
    // performance: the mod wheel adds string vibrato, velocity the pick attack's brightness
    p.mod.modWheel.push_back ({ "quick.vibrato", 0.45f });
    if (c.has (FT::compressor) || c.has (FT::drive)) p.mod.velocity.push_back ({ "volume", f == "electric" || f == "crunch" ? 0.6f : 0.4f });   // the compressor must not flatten the touch
}

//------------------------------------------------------------------------------ BASS
void designBass (VaultPreset& p, const Role& r)
{
    design (p);
    Chain c (p.fx);
    const auto& f = r.fam;
    const auto& t = r.treat;
    const int sub = subLayer (p);
    float finish = 0.1f;
    p.mod = {};
    auto mid = [&] (float db, float hz, float q) { auto& e = c.at (FT::eq); e.set ("mid", db).set ("midFreq", hz).set ("midQ", q); };
    auto lowShelfAt = [&] (float hz) { c.at (FT::eq).set ("lowFreq", hz); };
    // a drive the treatment brought along keeps its amount; the family's drive is the starting point otherwise
    auto drive = [&] (fx::Drive type, float amount, float mix, float tone, bool cleanLow)
    {
        if (c.has (FT::drive)) c.at (FT::drive).set ("cleanLow", cleanLow ? 1.0f : 0.0f).set ("tone", std::min (tone * 1.4f, c.get (FT::drive, "tone")));
        else c.put (fx::drive (type, amount, mix, tone).set ("cleanLow", cleanLow ? 1.0f : 0.0f));
    };
    const bool synth = p.parts.front().kind == VaultPartKind::synth;
    juce::String m0 = "Cutoff", m3;
    if (f == "sub")
    {
        c.put (fx::transient (0.12f));
        c.put (fx::comp (-18, 2.5f, 25, 150, 1.5f, 0.6f));
        drive (fx::Drive::tape, 0.1f, 0.5f, 2500, false);       // 2nd / 3rd harmonics so the sub reads on small speakers
        lowShelfAt (70); c.eqAdd ("low", 1.0f); c.eqAdd ("high", -3.0f); c.at (FT::eq).set ("highFreq", 3000.0f);
        m3 = "Punch";
    }
    else if (f == "analog")
    {
        c.put (fx::comp (-20, 2.5f, 12, 140, 1.5f, 0.65f));
        drive (fx::Drive::tube, 0.12f, 0.7f, 4500, true);
        lowShelfAt (80); c.eqAdd ("low", 1.0f); mid (-1.5f, 300, 0.8f); c.eqAdd ("high", -1.5f);
        m3 = "Envelope";
    }
    else if (f == "reese")
    {
        c.put (fx::comp (-20, 2.5f, 15, 150, 1.0f, 0.7f));
        drive (fx::Drive::tube, 0.15f, 0.7f, 4000, true);
        mid (-2.0f, 350, 0.8f); c.eqAdd ("high", -1.5f);
        c.put (fx::width (0.62f, 160));
        p.mod.lfo[0] = lfo (VaultLfo::sine).rate (0.18f).free().depth (0.5f).to ("cutoff", 0.35f);
        m3 = "Motion";
    }
    else if (f == "dirty")
    {
        c.put (fx::comp (-20, 2.5f, 10, 140, 1.0f, 0.7f));
        drive (fx::Drive::hard, 0.2f, 0.6f, 4000, true);
        lowShelfAt (80); c.eqAdd ("low", 1.0f); mid (1.0f, 800, 0.8f); c.eqAdd ("high", -3.0f); c.at (FT::eq).set ("highFreq", 4500.0f);
        m3 = "Bite";
    }
    else if (f == "digital")
    {
        c.put (fx::comp (-20, 2.5f, 10, 140, 1.0f, 0.7f));
        drive (fx::Drive::tape, 0.08f, 0.5f, 5000, true);
        lowShelfAt (80); c.eqAdd ("low", 1.0f); c.eqAdd ("high", -2.0f);
        m0 = "Tone"; m3 = "Punch";
        c.put (fx::transient (0.1f));
    }
    else if (f == "finger" || f == "pick" || f == "slap" || f == "fretless")
    {
        c.put (fx::comp (-22, 2.5f, 15, 160, 1.5f, 0.65f));
        drive (fx::Drive::tape, 0.1f, 0.5f, 3500, true);            // the amp / DI blend
        lowShelfAt (80);
        if (f == "finger") { c.eqAdd ("low", 1.5f); mid (-2.0f, 250, 0.8f); c.eqAdd ("high", -1.0f); }
        else if (f == "pick") { c.eqAdd ("low", 1.0f); mid (-1.5f, 300, 0.8f); c.eqAdd ("high", -1.0f); }
        else if (f == "slap") { c.eqAdd ("low", 1.0f); mid (-3.0f, 550, 0.7f); c.put (fx::transient (0.2f)); }
        else { c.eqAdd ("low", 1.0f); mid (-1.0f, 300, 0.8f); c.eqAdd ("high", -1.5f); if (! c.has (FT::chorus)) c.put (fx::chorus (0.18f, 0.3f, 0.4f)); }
        m0 = "Tone"; m3 = f == "fretless" ? "Chorus" : "Punch";
    }
    else if (f == "orch")
    {
        if (! r.layer) c.put (fx::comp (-24, 1.8f, 30, 250, 1.0f, 0.35f));   // (layers: the octave-up part keeps its balance)
        if (! c.has (FT::width)) c.put (fx::width (0.5f, 150));
        lowShelfAt (90); c.eqAdd ("low", 1.0f); mid (-1.0f, 300, 0.8f); c.eqAdd ("high", -1.0f);
        c.put (rv (R::hall, 0.6f, 0.9f, 0.1f, 15, 0.3f, 200));
        m0 = "Tone"; m3 = sustained (r.noun) ? "Bow" : "Punch";
        if (m3 == "Punch") c.put (fx::transient (0.1f));
        finish = 0.08f;
    }
    else   // keys / mallets / harp played low
    {
        c.put (fx::comp (-22, 2.0f, 20, 180, 1.0f, 0.6f));
        if (! c.has (FT::width)) c.put (fx::width (0.5f, 150)); else c.at (FT::width).set ("monoBelow", 150.0f);
        lowShelfAt (90); c.eqAdd ("low", 1.0f); c.eqAdd ("high", -1.5f);
        c.put (rv (R::room, 0.35f, 0.8f, 0.08f, 5, 0.3f, 200));
        m0 = "Tone"; m3 = sustained (r.noun) ? "Release" : "Punch";
        if (m3 == "Punch") c.put (fx::transient (0.1f));
        finish = 0.08f;
    }
    if (t == "Dark") c.at (FT::eq).set ("highFreq", 3500.0f);

    // rhythm presets
    if (r.pattern == 5)
    {
        p.arp = arp (VaultArp::up, "1/16", 1, 0.5f, 0.1f, true);
        const float acc[16] { 1, 0.6f, 0.75f, 0.6f, 1, 0.6f, 0.75f, 0.9f, 1, 0.6f, 0.75f, 0.6f, 1, 0.65f, 0.8f, 0.7f };
        for (int i = 0; i < 16; ++i) p.arp.vel[(size_t) i] = acc[i];
        p.mod.velocity.push_back ({ "cutoff", 0.6f });   // the accents open the filter
    }
    else if (r.pattern == 6) p.arp = arp (VaultArp::asPlayed, "1/8", 1, 0.7f, 0.0f, true);
    else if (r.pattern == 7) p.gate = gate ("1/8", { 1, 0.0f, 1, 1, 0.0f, 1, 1, 0.0f }, 1.0f, 0.3f, 1.0f, true);

    p.fx = c.build();
    p.finish = finish;
    for (int i = 0; i < 4; ++i) { p.macros[(size_t) i] = {}; p.macroDefaults[i] = 0.5f; }

    // 1. Cutoff / Tone
    {
        M m (m0.toRawUTF8(), 0.55f);
        m.rel ("cutoff", synth && f != "sub" ? 2.6f : 8.0f);
        if (synth && f != "sub" && f != "digital") m.rel ("reso", 0.25f);
        if (f == "sub") { m = M ("Tone", 0.5f); m.fxp (c, FT::eq, "tilt", 8.0f); }
        else if (! synth) m.fxp (c, FT::eq, "tilt", 6.0f);
        setMacro (p, 0, m);
    }
    // 2. Drive (the character's harmonics; the lows stay clean on all but the sine subs)
    if (c.has (FT::drive)) setMacro (p, 1, M (f == "sub" ? "Harmonics" : f == "finger" || f == "pick" || f == "slap" || f == "fretless" ? "Amp" : "Drive", 0.35f)
                                            .fxp (c, FT::drive, "amount", 0.6f).fxp (c, FT::drive, "mix", 0.5f).toMax ("fx.drive.output", c.get (FT::drive, "output"), -4.0f));
    else if (c.has (FT::reverb)) setMacro (p, 1, M (f == "orch" ? "Hall" : "Room", 0.25f).fxp (c, FT::reverb, "mix", 0.45f).fxp (c, FT::reverb, "decay", 1.5f));
    // 3. Sub / Weight
    if (sub > 0)
    {
        const float l = p.parts[(size_t) sub].level;
        setMacro (p, 2, M ("Sub", 0.5f).abs (("layer" + juce::String (sub + 1) + ".level").toRawUTF8(), l, 2.0f * l, 0.0f, 2.0f).fxp (c, FT::eq, "low", 3.0f));
    }
    else setMacro (p, 2, M ("Weight", 0.5f).fxp (c, FT::eq, "low", 8.0f).fxp (c, FT::eq, "mid", 3.0f));
    // 4. the family's character
    {
        M m (m3.toRawUTF8(), 0.5f);
        if (m3 == "Envelope") m.rel ("quick.envAmt", 0.8f);
        else if (m3 == "Motion") { m = M ("Motion", 0.5f); m.abs ("lfo1.depth", 0.5f, 1.0f, 0, 1); }
        else if (m3 == "Bite") m.fxp (c, FT::drive, "tone", 6000.0f).fxp (c, FT::eq, "mid", 4.0f);
        else if (m3 == "Punch")
        {
            if (c.has (FT::transient)) { m = M ("Punch", 0.3f); m.fxp (c, FT::transient, "attack", 0.6f); }
            m.fxp (c, FT::compressor, "threshold", 14.0f).fxp (c, FT::compressor, "makeup", 3.0f);
        }
        else if (m3 == "Chorus") { m = M ("Chorus", 0.3f); m.fxp (c, FT::chorus, "mix", 0.6f); }
        else if (m3 == "Bow") { m = M ("Bow", 0.0f); m.rel ("quick.attack", 0.42f).rel ("cutoff", -0.8f); }
        else { m = M ("Release", 0.0f); m.rel ("quick.release", 0.45f); }
        setMacro (p, 3, m);
    }
    // performance
    if (synth) p.mod.modWheel.push_back ({ "cutoff", 1.2f });
    else p.mod.modWheel.push_back ({ "quick.vibrato", 0.4f });
    const bool hot = f == "dirty" || f == "sub" || c.get (FT::drive, "amount") >= 0.25f;
    p.mod.velocity.push_back ({ "volume", hot ? 0.9f : 0.6f });   // the compressor must not flatten the touch (>= 3 dB from velocity 0.35 to 1.0)
}

//------------------------------------------------------------------------------ macro extremes against the comfort rules
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
// how different the two extremes sound: RMS log-mel difference (dB) over the held note and the tail
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
// tail (after the note-off) against the held note, dB, both channels: what a reverb / delay / release macro changes
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
struct MacroReport { int fixes = 0; juce::StringArray harsh, quiet, why; };
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
            if (envInt ("STRINGS_MACROLOG", 0) != 0) std::printf ("  narrow %s / %s: %s | %s\n", p.name.toRawUTF8(), mac.name.toRawUTF8(), badLo.toRawUTF8(), badHi.toRawUTF8());
            mr.why.add (mac.name + (badLo.isNotEmpty() ? " 0:" + badLo : juce::String()) + (badHi.isNotEmpty() ? " 1:" + badHi : juce::String()));
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
        if (envInt ("STRINGS_MACROLOG", 0) != 0)
            std::printf ("  macro %s / %s: dist %.2f, loud %.1f / %.1f / %.1f, 2-5k %.1f / %.1f / %.1f %%, AM %.2f / %.2f, tail %.1f / %.1f dB, default %.2f\n", p.name.toRawUTF8(), mac.name.toRawUTF8(),
                         extremeDistance (lo, hi), lo.loud, ref.loud, hi.loud, 100 * lo.s25, 100 * ref.s25, 100 * hi.s25, amDepth (lo.c5), amDepth (hi.c5), tailRatioDb (lo.c5), tailRatioDb (hi.c5), p.macroDefaults[i]);
        if (extremeDistance (lo, hi) < 1.0f && std::abs (lo.loud - hi.loud) < 1.0f && std::abs (amDepth (lo.c5) - amDepth (hi.c5)) < 0.4f && ! pitchLfo
            && std::abs (tailRatioDb (lo.c5) - tailRatioDb (hi.c5)) < 2.0f)
            mr.quiet.add (p.name + ": " + mac.name);
    }
    return mr;
}

// C5 comfort: the sounding fundamental within ~2 dB of the strongest harmonic, measured on the whole v3 sound. Basses get
// a sine part at part 0's octave (or a louder one) - the band-split practice -, guitars more low shelf on the FX EQ.
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
        for (auto& s : p.fx)
            if (s.type == FT::eq)
            {
                const float base = s.get ("low");
                s.set ("lowFreq", juce::jlimit (30.0f, 1000.0f, std::max (s.get ("lowFreq"), 1.4f * recipegate::soundingHz (p, vault::homeNote))));
                if ((h = fundamentalDb (p)) >= want) return;
                for (float add : { 3.0f, 6.0f })
                {
                    if (base + add > 10.0f) break;
                    s.set ("low", base + add);
                    // the Body macro keeps its designed point on the new value
                    for (auto& m : p.macros) for (auto& t : m.targets) if (t.target == "fx.eq.low") { t.min += 3.0f; t.max += 3.0f; }
                    if ((h = fundamentalDb (p)) >= want) return;
                }
                return;
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
    const auto target = "layer" + juce::String (sub + 1) + ".level";
    for (int i = 0; i < 10 && (h = fundamentalDb (p)) < want; ++i)
    {
        auto& l = p.parts[(size_t) sub].level;
        if (l >= 2.0f) break;
        const float nl = std::min (2.0f, l * 1.35f);
        for (auto& m : p.macros) for (auto& t : m.targets) if (t.target == target) { const float k = nl / std::max (1.0e-4f, l); t.min *= k; t.max = std::min (2.0f, t.max * k); }
        l = nl;
    }
}
} // namespace v3
} // namespace

//==============================================================================
std::vector<VaultPreset> build_strings (const RecipeContext& ctx)
{
    std::vector<Cand> cands;
    Names names;
    addGuitars (ctx, cands, names);
    addBasses (ctx, cands, names);
    std::vector<VaultPreset> out;
    std::vector<Role> roles;
    int nG = 0, nB = 0;
    for (auto& c : cands)
    {
        VaultPreset p;
        p.category = c.category;
        p.id = c.category == vault::catGuitars ? "cv.gtr." + juce::String (++nG).paddedLeft ('0', 4) : "cv.bass." + juce::String (++nB).paddedLeft ('0', 4);
        p.name = c.name;
        p.parts = c.parts;
        if (c.category == vault::catGuitars) v3::designGuitar (p, c.role); else v3::designBass (p, c.role);
        out.push_back (std::move (p));
        roles.push_back (c.role);
    }
    // STRINGS_ONLY=<id,id,...>: just these presets (demos, listening)
    if (const auto only = juce::StringArray::fromTokens (juce::SystemStats::getEnvironmentVariable ("STRINGS_ONLY", {}), ",", ""); ! only.isEmpty())
    {
        std::vector<VaultPreset> o; std::vector<Role> ro;
        for (size_t i = 0; i < out.size(); ++i) if (only.contains (out[i].id)) { o.push_back (out[i]); ro.push_back (roles[i]); }
        out.swap (o); roles.swap (ro);
    }
    if (const int lim = v3::envInt ("STRINGS_LIMIT", 0); lim > 0 && lim < (int) out.size()) { out.resize ((size_t) lim); roles.resize ((size_t) lim); }
    const int threads = v3::envInt ("STRINGS_THREADS", 3);
    auto parallel = [threads] (int count, const std::function<void (int)>& fn)
    {
        std::atomic<int> next { 0 }, done { 0 };
        std::vector<std::thread> pool;
        for (int t = 0; t < threads; ++t)
            pool.emplace_back ([&] { for (int i; (i = next++) < count;) { fn (i); if (const int d = ++done; d % 100 == 0) std::printf ("  strings %d / %d\n", d, count); } });
        for (auto& t : pool) t.join();
    };
    std::printf ("strings: %d candidates (v3 designed), fitting the fundamental\n", (int) out.size());
    parallel ((int) out.size(), [&] (int i) { v3::fitFundamental (out[(size_t) i]); });
    if (v3::envInt ("STRINGS_SELFGATE", 1) == 0) return out;

    // the bank gate's own selection (same analysis, same order, same -dist 3.5), so the macro check runs only on the presets
    // the gate will keep; the gate re-runs on what this returns and accepts the same set
    std::vector<recipegate::Result> res (out.size());
    parallel ((int) out.size(), [&] (int i) { res[(size_t) i].preset = out[(size_t) i]; recipegate::analyse (res[(size_t) i]); });
    std::vector<int> acc;
    std::map<juce::String, int> fails;
    for (int i = 0; i < (int) res.size(); ++i)
    {
        auto& r = res[(size_t) i];
        juce::String why;
        if (r.fail.isEmpty() && ! vault::validatePreset (r.preset, &why)) r.fail = "invalid: " + why;
        if (r.fail.isEmpty() && r.gainDb > recipegate::maxTrimDb) r.fail = "too-quiet";
        // the assembly QA's spectral limits (VaultBankGen -qa), so the bank assembly keeps every preset this returns
        if (r.fail.isEmpty() && r.share8k > recipegate::maxShare8k) r.fail = "air-8k";
        if (r.fail.isEmpty() && r.centroid > recipegate::centroidCeiling (r.preset.category)) r.fail = "centroid";
        if (r.fail.isEmpty())
            for (int j : acc) if (recipegate::distance (r.print, res[(size_t) j].print) < 3.5f) { r.fail = "near-duplicate"; break; }
        if (r.fail.isEmpty()) acc.push_back (i); else ++fails[r.fail.upToFirstOccurrenceOf (":", false, false)];
    }
    std::printf ("strings: %d of %d pass the gate\n", (int) acc.size(), (int) res.size());
    for (auto& [f, n] : fails) std::printf ("  strings rejected %-16s %d\n", f.toRawUTF8(), n);
    if (const auto fp = juce::SystemStats::getEnvironmentVariable ("STRINGS_FAILS", {}); fp.isNotEmpty())
    {
        juce::String s;
        for (auto& r : res) s << r.preset.id << "\t" << r.preset.name << "\t" << (r.fail.isEmpty() ? juce::String ("ok") : r.fail) << "\t" << juce::String (r.h1Db, 1) << "\t"
                              << juce::String (100 * r.share25, 2) << "\t" << juce::String (100 * r.share6k, 2) << "\t" << juce::String (r.velRange, 1) << "\t" << juce::String (r.minDist < 1e8f ? r.minDist : 0.0f, 2) << "\n";
        juce::File (fp).replaceWithText (s);
    }

    // macro extremes
    std::vector<v3::MacroReport> mr (acc.size());
    std::vector<VaultPreset> kept (acc.size());
    parallel ((int) acc.size(), [&] (int k)
    {
        kept[(size_t) k] = out[(size_t) acc[(size_t) k]];
        if (v3::envInt ("STRINGS_MACROCHECK", 1) != 0) mr[(size_t) k] = v3::checkMacros (kept[(size_t) k], res[(size_t) acc[(size_t) k]].preset.gain);
    });
    int fixes = 0;
    juce::StringArray harsh, quiet;
    for (auto& m : mr) { fixes += m.fixes; harsh.addArray (m.harsh); quiet.addArray (m.quiet); }
    std::printf ("strings: macro ranges narrowed %d times; %d macro extremes still outside the comfort rules; %d macros barely audible\n", fixes, harsh.size(), quiet.size());
    for (auto& h : harsh) std::printf ("  harsh: %s\n", h.toRawUTF8());
    for (auto& q : quiet) std::printf ("  quiet: %s\n", q.toRawUTF8());
    return kept;
}

VAULT_RECIPE (strings, build_strings);
