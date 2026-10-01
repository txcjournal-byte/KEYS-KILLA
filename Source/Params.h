#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs are stable - never rename after 1.0.
namespace ID
{
    // layer A (ids kept short for compatibility) and layer B (suffix B)
    inline constexpr const char* engine   = "engine";
    inline constexpr const char* octave   = "octave";
    inline constexpr const char* semi     = "semi";
    inline constexpr const char* fine     = "fine";
    inline constexpr const char* wave     = "wave";
    inline constexpr const char* unison   = "unison";
    inline constexpr const char* detune   = "detune";
    inline constexpr const char* fmRatio  = "fmRatio";
    inline constexpr const char* fmRatio2 = "fmRatio2";
    inline constexpr const char* fmAmt    = "fmAmt";
    inline constexpr const char* fmAlgo   = "fmAlgo";
    inline constexpr const char* warpMode = "warpMode";
    inline constexpr const char* levelA   = "levelA";

    inline constexpr const char* layerB   = "layerB";
    inline constexpr const char* engineB  = "engineB";
    inline constexpr const char* octaveB  = "octaveB";
    inline constexpr const char* semiB    = "semiB";
    inline constexpr const char* fineB    = "fineB";
    inline constexpr const char* waveB    = "waveB";
    inline constexpr const char* unisonB  = "unisonB";
    inline constexpr const char* detuneB  = "detuneB";
    inline constexpr const char* fmRatioB = "fmRatioB";
    inline constexpr const char* fmRatio2B= "fmRatio2B";
    inline constexpr const char* fmAmtB   = "fmAmtB";
    inline constexpr const char* fmAlgoB  = "fmAlgoB";
    inline constexpr const char* warpModeB= "warpModeB";
    inline constexpr const char* levelB   = "levelB";

    inline constexpr const char* sub      = "sub";
    // filter + env 2
    inline constexpr const char* filterType = "filterType";
    inline constexpr const char* cutoff   = "cutoff";
    inline constexpr const char* reso     = "reso";
    inline constexpr const char* keyTrack = "keyTrack";
    inline constexpr const char* fenv     = "fenv";
    inline constexpr const char* fattack  = "fattack";
    inline constexpr const char* fdecay   = "fdecay";
    inline constexpr const char* fsustain = "fsustain";
    inline constexpr const char* frelease = "frelease";
    // amp env 1
    inline constexpr const char* attack   = "attack";
    inline constexpr const char* decay    = "decay";
    inline constexpr const char* sustain  = "sustain";
    inline constexpr const char* release  = "release";
    inline constexpr const char* velSens  = "velSens";
    // env 3 (mod)
    inline constexpr const char* e3attack = "e3attack";
    inline constexpr const char* e3decay  = "e3decay";
    inline constexpr const char* e3sustain= "e3sustain";
    inline constexpr const char* e3release= "e3release";
    // lfos
    inline constexpr const char* lfoRate  = "lfoRate";
    inline constexpr const char* lfoSync  = "lfoSync";
    inline constexpr const char* lfoDiv   = "lfoDiv";
    inline constexpr const char* lfoShape = "lfoShape";
    inline constexpr const char* lfoPitch = "lfoPitch";
    inline constexpr const char* lfoFilter= "lfoFilter";
    inline constexpr const char* lfoAmp   = "lfoAmp";
    inline constexpr const char* lfo2Rate = "lfo2Rate";
    inline constexpr const char* lfo2Sync = "lfo2Sync";
    inline constexpr const char* lfo2Div  = "lfo2Div";
    inline constexpr const char* lfo2Shape= "lfo2Shape";
    inline constexpr const char* wobTarget= "wobTarget";
    // play
    inline constexpr const char* mono     = "mono";
    inline constexpr const char* legato   = "legato";
    inline constexpr const char* glide    = "glide";
    inline constexpr const char* bendRange= "bendRange";
    inline constexpr const char* bassMode = "bassMode";
    inline constexpr const char* keyLock  = "keyLock";
    inline constexpr const char* key      = "key";
    inline constexpr const char* scale    = "scale";
    inline constexpr const char* chord    = "chord";
    inline constexpr const char* chordType= "chordType";
    inline constexpr const char* strum    = "strum";
    inline constexpr const char* arp      = "arp";
    inline constexpr const char* arpRate  = "arpRate";
    inline constexpr const char* arpMode  = "arpMode";
    inline constexpr const char* arpOct   = "arpOct";
    inline constexpr const char* arpSwing = "arpSwing";
    inline constexpr const char* arpGate  = "arpGate";
    // fx
    inline constexpr const char* drive    = "drive";
    inline constexpr const char* driveType= "driveType";
    inline constexpr const char* crush    = "crush";
    inline constexpr const char* wow      = "wow";
    inline constexpr const char* chorus   = "chorus";
    inline constexpr const char* phaser   = "phaser";
    inline constexpr const char* flanger  = "flanger";
    inline constexpr const char* delayMix = "delayMix";
    inline constexpr const char* delayTime= "delayTime";
    inline constexpr const char* delayFb  = "delayFb";
    inline constexpr const char* delayMode= "delayMode";
    inline constexpr const char* revMix   = "revMix";
    inline constexpr const char* revSize  = "revSize";
    inline constexpr const char* revType  = "revType";
    inline constexpr const char* eqLow    = "eqLow";
    inline constexpr const char* eqHigh   = "eqHigh";
    inline constexpr const char* reverse  = "reverse";
    inline constexpr const char* freeze   = "freeze";
    inline constexpr const char* width    = "width";
    inline constexpr const char* gain     = "gain";
    // macros
    inline constexpr const char* m1 = "macro1";
    inline constexpr const char* m2 = "macro2";
    inline constexpr const char* m3 = "macro3";
    inline constexpr const char* m4 = "macro4";
    inline constexpr const char* m5 = "macro5";
    inline constexpr const char* m6 = "macro6";
    inline constexpr const char* m7 = "macro7";   // PUNCH (bass: KNOCK)
    inline constexpr const char* m8 = "macro8";   // MIX (effects wet balance)
    // exclusive
    inline constexpr const char* ghost    = "ghost";
    inline constexpr const char* ghostOct = "ghostOct";
    inline constexpr const char* ghostRev = "ghostRev";
    inline constexpr const char* ghostBlur= "ghostBlur";
    inline constexpr const char* bend     = "bend";
    inline constexpr const char* bendMode = "bendMode";
    inline constexpr const char* bendSemis= "bendSemis";
    inline constexpr const char* tape     = "tape";
    inline constexpr const char* circuit  = "circuit";
    inline constexpr const char* circRate = "circRate";
    inline constexpr const char* chaos    = "chaos";
    inline constexpr const char* morphX   = "morphX";
    inline constexpr const char* morphY   = "morphY";
    inline constexpr const char* body     = "body";
    inline constexpr const char* bodyMix  = "bodyMix";
    inline constexpr const char* seed     = "seed";
    // v0.4 (TRAP 2010 -> FUTURE spec)
    inline constexpr const char* alive    = "alive";      // 0..5 per-note micro variation
    inline constexpr const char* drift    = "drift";      // tape / ROMpler pitch drift
    inline constexpr const char* timeM    = "timeMacro";  // TIGHT - NATURAL - DREAM - FROZEN
    inline constexpr const char* punch    = "punchFx";    // transient shaper
    inline constexpr const char* halftime = "halftime";   // half-speed buffer mix
    inline constexpr const char* master   = "master";     // MASTER stage: glue, air, saturation, limiter (v0.13)
    // v0.14 HALF module (Voodoo Killa engine): time FX on the whole output
    inline constexpr const char* halfOn     = "halfOn";
    inline constexpr const char* halfPreset = "halfPreset";   // 12 categories x 8 presets
    inline constexpr const char* halfAmount = "halfAmount";
    inline constexpr const char* halfSpeed  = "halfSpeed";
    inline constexpr const char* halfTrig   = "halfTrig";
    inline constexpr const char* halfMix    = "halfMix";
    // v0.15 modules: drums (808 / SNARE / CLAP / hats), ROLLS, EFFECTOR, DIGGA
    inline constexpr const char* playMode = "playMode";   // what the keys / MIDI play: KEYS, 808, SNARE, CLAP, HATS, DIGGA
    inline constexpr const char* b8Tune = "b8Tune";  inline constexpr const char* b8Decay = "b8Decay"; inline constexpr const char* b8Punch = "b8Punch";
    inline constexpr const char* b8Glide = "b8Glide"; inline constexpr const char* b8Tone = "b8Tone";  inline constexpr const char* b8Drive = "b8Drive";
    inline constexpr const char* b8Sat = "b8Sat";    inline constexpr const char* b8Clip = "b8Clip";  inline constexpr const char* b8Level = "b8Level";
    inline constexpr const char* b8Click = "b8Click"; inline constexpr const char* b8Width = "b8Width"; inline constexpr const char* b8Sub = "b8Sub";
    inline constexpr const char* htPan = "htPan";
    // v0.16 SOUND WORLD, TRANCE GATE, master CLIPPER
    inline constexpr const char* world = "world";     inline constexpr const char* worldAmt = "worldAmt";
    inline constexpr const char* gate = "gate";       inline constexpr const char* gateDepth = "gateDepth";
    inline constexpr const char* clipMode = "clipMode"; inline constexpr const char* clipDrive = "clipDrive";
    inline constexpr const char* snTune = "snTune";  inline constexpr const char* snBody = "snBody";  inline constexpr const char* snSnap = "snSnap";
    inline constexpr const char* snDecay = "snDecay"; inline constexpr const char* snTone = "snTone"; inline constexpr const char* snLevel = "snLevel";
    inline constexpr const char* clTune = "clTune";  inline constexpr const char* clSpread = "clSpread"; inline constexpr const char* clDecay = "clDecay";
    inline constexpr const char* clTone = "clTone";  inline constexpr const char* clWidth = "clWidth"; inline constexpr const char* clLevel = "clLevel";
    inline constexpr const char* htTune = "htTune";  inline constexpr const char* htDecay = "htDecay"; inline constexpr const char* htTone = "htTone";
    inline constexpr const char* htLevel = "htLevel";
    inline constexpr const char* rlOn = "rlOn";      inline constexpr const char* rlStyle = "rlStyle"; inline constexpr const char* rlSeed = "rlSeed";
    inline constexpr const char* rlBars = "rlBars";  inline constexpr const char* rlDensity = "rlDensity";
    inline constexpr const char* efxOn = "efxOn";    inline constexpr const char* efxPreset = "efxPreset"; inline constexpr const char* efxBlend = "efxBlend";
    inline juce::String efxMacro (int i) { return "efxM" + juce::String (i + 1); }
    inline constexpr const char* dgMode = "dgMode";  inline constexpr const char* dgSlices = "dgSlices"; inline constexpr const char* dgChop = "dgChop";
    inline constexpr const char* dgPitch = "dgPitch"; inline constexpr const char* dgRev = "dgRev";   inline constexpr const char* dgLevel = "dgLevel";
    // module parameters belong to the modules: presets, BREED, morph and dice never touch them
    inline bool isModuleParam (const juce::String& id)
    {
        static const juce::StringArray ids { halfOn, halfPreset, halfAmount, halfSpeed, halfTrig, halfMix, playMode,
            b8Tune, b8Decay, b8Punch, b8Glide, b8Tone, b8Drive, b8Sat, b8Clip, b8Level, b8Click, b8Width, b8Sub, htPan, world, worldAmt, gate, gateDepth, clipMode, clipDrive,
            snTune, snBody, snSnap, snDecay, snTone, snLevel, clTune, clSpread, clDecay, clTone, clWidth, clLevel,
            htTune, htDecay, htTone, htLevel, rlOn, rlStyle, rlSeed, rlBars, rlDensity, efxOn, efxPreset, efxBlend,
            "efxM1", "efxM2", "efxM3", "efxM4", "efxM5", dgMode, dgSlices, dgChop, dgPitch, dgRev, dgLevel };
        return ids.contains (id);
    }
    // v0.5
    inline constexpr const char* era      = "era";        // ERA morph 0..6 (2010-12 ... FUTURE)
    inline constexpr const char* eraHome  = "eraHome";    // the era the preset was designed in
    inline constexpr const char* future   = "future";     // FUTURE: ORIGINAL -> HYBRID -> UNKNOWN
    // v0.7: step arpeggiator (length + 16 step velocities, 0 = rest)
    inline constexpr const char* arpSteps = "arpSteps";
    inline juce::String arpStep (int i) { return "arpStep" + juce::String (i + 1); }   // step on (> 0.5) / rest
    inline juce::String arpNote (int i) { return "arpNote" + juce::String (i + 1); }   // semitones -12 ... +12
    inline juce::String arpLen (int i)  { return "arpLen" + juce::String (i + 1); }    // length in steps (tie)
    inline bool isArpPattern (const juce::String& id) { return id.startsWith ("arpStep") || id.startsWith ("arpNote") || id.startsWith ("arpLen"); }

    // mod matrix slots: mmSrc1..8, mmDst1..8, mmAmt1..8
    inline juce::String mmSrc (int i) { return "mmSrc" + juce::String (i + 1); }
    inline juce::String mmDst (int i) { return "mmDst" + juce::String (i + 1); }
    inline juce::String mmAmt (int i) { return "mmAmt" + juce::String (i + 1); }
}

inline constexpr int numModSlots = 8;

namespace Choices
{
    inline const juce::StringArray halfSpeeds { "1/4x", "1/2x", "1x", "2x", "4x" };
    inline const juce::StringArray halfTrigs  { "Always", "Every 4 bars", "Every 8 bars", "Last beat" };
    inline const juce::StringArray worlds     { "Off", "XV", "Moog", "Serum", "Zenology", "Omni", "Kontakt", "Diva", "Nexus" };
    inline const juce::StringArray gates      { "Off", "1/8", "1/16", "1/8 Triplet", "Stutter A", "Stutter B" };
    inline const juce::StringArray clipModes  { "Off", "Soft", "Hard", "Modern" };
    inline const juce::StringArray playModes  { "Keys", "808", "Snare", "Clap", "Hats", "Digga" };
    inline const juce::StringArray satModes   { "Tape", "Tube", "Foldback" };
    inline const juce::StringArray rollStyles { "Classic", "Triplet", "Drill", "Crazy" };
    inline const juce::StringArray rollBars   { "1 bar", "2 bars", "4 bars" };
    inline const juce::StringArray diggaModes { "Chop", "Keys" };
    inline const juce::StringArray diggaSlices { "8", "16" };
    inline const juce::StringArray diggaChops { "Transients", "Even" };
    inline const juce::StringArray engines   { "VA", "FM", "Pluck", "Vox", "Organ", "Flute", "Sub 808", "Wavetable", "Orchestral", "Modal" };
    inline const juce::StringArray fmAlgos   { "2-Op", "4-Stack", "2x2 Pairs", "3 > 1", "Bell", "E-Piano" };
    inline const juce::StringArray warps     { "Bend", "Sync", "Mirror", "Quantize", "FM" };
    inline const juce::StringArray filters   { "Clean", "Ladder", "Dirty", "High Pass", "Band Pass", "Notch", "Peak" };
    inline const juce::StringArray lfoDivs   { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T" };
    inline const juce::StringArray lfoShapes { "Sine", "Triangle", "Saw", "Square", "S&H" };
    inline const juce::StringArray wobTargets{ "Filter", "Volume", "Wave", "Pitch" };
    inline const juce::StringArray keys      { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    inline const juce::StringArray scales    { "Minor", "Major", "Harmonic Minor", "Phrygian", "Dorian", "Minor Pent.", "Blues", "Chromatic" };
    inline const juce::StringArray chordTypes{ "Trap Minor", "Dark Minor", "Minor Add9", "Dark Sus", "Power", "Octaves", "Phrygian", "Minor 7", "Major", "Scale Triad" };
    inline const juce::StringArray arpRates  { "1/8", "1/16", "1/16T", "1/32", "1/4", "1/8T", "1/32T" };
    inline const juce::StringArray arpModes  { "Up", "Down", "Up/Down", "Random", "As Played", "Scale Up", "Scale Down", "Chord" };
    inline const juce::StringArray drives    { "Soft", "Tape", "Hard", "Blown", "Fold" };
    inline const juce::StringArray delays    { "1/4", "1/8", "1/8D", "1/16", "1/4T" };
    inline const juce::StringArray delayModes{ "Ping-Pong", "Stereo", "Tape" };
    inline const juce::StringArray revTypes  { "Hall", "Plate", "Cloud" };
    inline const juce::StringArray bodies    { "Off", "Kalimba", "Bell", "Glass", "Metal Pipe", "Wood Box", "String" };
    inline const juce::StringArray bendModes { "Dive", "Rise", "Dip", "Octave Jump", "Random" };
    inline const juce::StringArray ghostOcts { "+1 Oct", "-1 Oct", "+2 Oct", "Unison" };
    inline const juce::StringArray circRates { "1/8", "1/16", "1/32" };
    inline const juce::StringArray modSources{ "Off", "Env 2", "Env 3", "LFO 1", "LFO 2", "Velocity", "Mod Wheel", "Aftertouch", "Key Track", "Random" };
    inline const juce::StringArray modDests  { "Pitch", "Cutoff", "Resonance", "Wave A", "Wave B", "FM/Warp A", "FM/Warp B", "Level A", "Level B", "Amp", "Pan", "Sub", "Detune" };

    inline double lfoDivBeats (int i)
    {
        static const double v[] { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
        return v[juce::jlimit (0, 8, i)];
    }
    inline double arpBeats (int i)
    {
        static const double v[] { 0.5, 0.25, 1.0 / 6.0, 0.125, 1.0, 1.0 / 3.0, 1.0 / 12.0 };
        return v[juce::jlimit (0, 6, i)];
    }
    inline double delayBeats (int i)
    {
        static const double v[] { 1.0, 0.5, 0.75, 0.25, 2.0 / 3.0 };
        return v[juce::jlimit (0, 4, i)];
    }
    inline double circBeats (int i)
    {
        static const double v[] { 0.5, 0.25, 0.125 };
        return v[juce::jlimit (0, 2, i)];
    }
    // scale intervals as 12-bit masks, bit n = semitone n above the key
    inline int scaleMask (int i)
    {
        static const int m[] { 0b010110101101, 0b101010110101, 0b100110101101, 0b010110101011,
                               0b011010101101, 0b010010101001, 0b010011101001, 0b111111111111 };
        return m[juce::jlimit (0, 7, i)];
    }
}

enum Engine { engVA, engFM, engPluck, engVox, engOrgan, engFlute, engSub, engWavetable, engOrchestral, engModal };
enum ModSrc { srcOff, srcEnv2, srcEnv3, srcLfo1, srcLfo2, srcVel, srcModWheel, srcAftertouch, srcKey, srcRandom };
enum ModDst { dstPitch, dstCutoff, dstReso, dstWaveA, dstWaveB, dstFmA, dstFmB, dstLevelA, dstLevelB, dstAmp, dstPan, dstSub, dstDetune, numDests };

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto f = [&] (const String& id, const String& name, float lo, float hi, float def, float skewCentre = -1.0f)
    {
        NormalisableRange<float> r (lo, hi);
        if (skewCentre > 0) r.setSkewForCentre (skewCentre);
        const bool hz = id == "cutoff", db = id == "gain" || id.startsWith ("eq"), cents = id.startsWith ("fine");
        const bool time = name.contains ("Attack") || name.contains ("Decay") || name.contains ("Release") || id == "glide" || id == "strum";
        const bool semis = id == "bendSemis", rate = id.startsWith ("lfo") && id.contains ("Rate");
        auto toText = [=] (float v, int) -> String
        {
            if (hz)    return v >= 1000.0f ? String (v / 1000.0f, 2) + " kHz" : String (juce::roundToInt (v)) + " Hz";
            if (db)    return String (v, 1) + " dB";
            if (cents) return String (juce::roundToInt (v)) + " ct";
            if (time)  return v < 1.0f ? String (juce::roundToInt (v * 1000.0f)) + " ms" : String (v, 2) + " s";
            if (semis) return String (juce::roundToInt (v)) + " st";
            if (rate)  return String (v, 2) + " Hz";
            if (hi <= 1.0f && lo >= -1.0f) return String (juce::roundToInt (v * 100.0f)) + " %";
            return String (v, 2);
        };
        auto fromText = [=] (const String& t) -> float
        {
            const float x = t.retainCharacters ("-0123456789.").getFloatValue();
            if (hz && t.containsIgnoreCase ("k")) return x * 1000.0f;
            if (time && t.containsIgnoreCase ("ms")) return x / 1000.0f;
            if (hi <= 1.0f && lo >= -1.0f && t.contains ("%")) return x / 100.0f;
            return x;
        };
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                   AudioParameterFloatAttributes().withStringFromValueFunction (toText).withValueFromStringFunction (fromText)));
    };
    auto c = [&] (const String& id, const String& name, const StringArray& ch, int def)
    { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, ch, def)); };
    auto b = [&] (const String& id, const String& name, bool def)
    { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto i = [&] (const String& id, const String& name, int lo, int hi, int def)
    { l.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def)); };

    // layer A
    c (ID::engine, "A Engine", Choices::engines, 0);
    i (ID::octave, "A Octave", -3, 2, 0);
    i (ID::semi, "A Semi", -12, 12, 0);
    f (ID::fine, "A Fine", -50, 50, 0);
    f (ID::wave, "A Wave", 0, 1, 0);
    i (ID::unison, "A Unison", 1, 8, 1);
    f (ID::detune, "A Detune", 0, 1, 0.2f);
    f (ID::fmRatio, "A FM Ratio", 0.5f, 12.0f, 2.0f, 3.0f);
    f (ID::fmRatio2, "A FM Ratio 2", 0.5f, 16.0f, 3.0f, 4.0f);
    f (ID::fmAmt, "A FM/Warp", 0, 1, 0.3f);
    c (ID::fmAlgo, "A FM Algo", Choices::fmAlgos, 0);
    c (ID::warpMode, "A Warp Mode", Choices::warps, 0);
    f (ID::levelA, "A Level", 0, 1, 1.0f);
    // layer B
    b (ID::layerB, "B On", false);
    c (ID::engineB, "B Engine", Choices::engines, 1);
    i (ID::octaveB, "B Octave", -3, 2, 0);
    i (ID::semiB, "B Semi", -12, 12, 0);
    f (ID::fineB, "B Fine", -50, 50, 0);
    f (ID::waveB, "B Wave", 0, 1, 0);
    i (ID::unisonB, "B Unison", 1, 8, 1);
    f (ID::detuneB, "B Detune", 0, 1, 0.2f);
    f (ID::fmRatioB, "B FM Ratio", 0.5f, 12.0f, 2.0f, 3.0f);
    f (ID::fmRatio2B, "B FM Ratio 2", 0.5f, 16.0f, 3.0f, 4.0f);
    f (ID::fmAmtB, "B FM/Warp", 0, 1, 0.3f);
    c (ID::fmAlgoB, "B FM Algo", Choices::fmAlgos, 0);
    c (ID::warpModeB, "B Warp Mode", Choices::warps, 0);
    f (ID::levelB, "B Level", 0, 1, 0.7f);
    f (ID::sub, "Sub", 0, 1, 0);

    c (ID::filterType, "Filter Type", Choices::filters, 0);
    f (ID::cutoff, "Cutoff", 40.0f, 20000.0f, 12000.0f, 1500.0f);
    f (ID::reso, "Resonance", 0, 1, 0.1f);
    f (ID::keyTrack, "Key Track", 0, 1, 0.4f);
    f (ID::fenv, "Filter Env", -1, 1, 0);
    f (ID::fattack, "Env2 Attack", 0.0005f, 5.0f, 0.001f, 0.3f);
    f (ID::fdecay, "Env2 Decay", 0.005f, 4.0f, 0.4f, 0.4f);
    f (ID::fsustain, "Env2 Sustain", 0, 1, 0);
    f (ID::frelease, "Env2 Release", 0.005f, 8.0f, 0.3f, 0.6f);

    f (ID::attack, "Attack", 0.001f, 5.0f, 0.002f, 0.3f);
    f (ID::decay, "Decay", 0.005f, 8.0f, 0.6f, 0.6f);
    f (ID::sustain, "Sustain", 0, 1, 0.8f);
    f (ID::release, "Release", 0.005f, 8.0f, 0.3f, 0.6f);
    f (ID::velSens, "Velocity", 0, 1, 0.6f);

    f (ID::e3attack, "Env3 Attack", 0.0005f, 5.0f, 0.01f, 0.3f);
    f (ID::e3decay, "Env3 Decay", 0.005f, 8.0f, 0.5f, 0.6f);
    f (ID::e3sustain, "Env3 Sustain", 0, 1, 0.0f);
    f (ID::e3release, "Env3 Release", 0.005f, 8.0f, 0.3f, 0.6f);

    f (ID::lfoRate, "LFO1 Rate", 0.05f, 20.0f, 5.0f, 3.0f);
    b (ID::lfoSync, "LFO1 Sync", false);
    c (ID::lfoDiv, "LFO1 Division", Choices::lfoDivs, 3);
    c (ID::lfoShape, "LFO1 Shape", Choices::lfoShapes, 0);
    f (ID::lfoPitch, "LFO1 > Pitch", 0, 1, 0);
    f (ID::lfoFilter, "LFO1 > Filter", 0, 1, 0);
    f (ID::lfoAmp, "LFO1 > Amp", 0, 1, 0);
    f (ID::lfo2Rate, "LFO2 Rate", 0.05f, 20.0f, 0.5f, 3.0f);
    b (ID::lfo2Sync, "LFO2 Sync", false);
    c (ID::lfo2Div, "LFO2 Division", Choices::lfoDivs, 2);
    c (ID::lfo2Shape, "LFO2 Shape", Choices::lfoShapes, 1);
    c (ID::wobTarget, "Wobble Target", Choices::wobTargets, 0);

    b (ID::mono, "Mono", false);
    b (ID::legato, "Legato", true);
    f (ID::glide, "Glide", 0, 1, 0, 0.15f);
    i (ID::bendRange, "Bend Range", 1, 24, 2);
    b (ID::bassMode, "Bass Mode", false);
    b (ID::keyLock, "Key Lock", false);
    c (ID::key, "Key", Choices::keys, 0);
    c (ID::scale, "Scale", Choices::scales, 0);
    b (ID::chord, "Chord", false);
    c (ID::chordType, "Chord Type", Choices::chordTypes, 0);
    f (ID::strum, "Strum", 0, 0.12f, 0);
    b (ID::arp, "Arp", false);
    c (ID::arpRate, "Arp Rate", Choices::arpRates, 1);
    c (ID::arpMode, "Arp Mode", Choices::arpModes, 0);
    i (ID::arpOct, "Arp Octaves", 1, 4, 1);
    f (ID::arpSwing, "Arp Swing", 0, 0.5f, 0);
    f (ID::arpGate, "Arp Gate", 0.1f, 1.0f, 0.8f);

    f (ID::drive, "Drive", 0, 1, 0);
    c (ID::driveType, "Drive Type", Choices::drives, 0);
    f (ID::crush, "Crush", 0, 1, 0);
    f (ID::wow, "Wow", 0, 1, 0);
    f (ID::chorus, "Chorus", 0, 1, 0);
    f (ID::phaser, "Phaser", 0, 1, 0);
    f (ID::flanger, "Flanger", 0, 1, 0);
    f (ID::delayMix, "Delay Mix", 0, 1, 0);
    c (ID::delayTime, "Delay Time", Choices::delays, 2);
    f (ID::delayFb, "Delay Feedback", 0, 0.95f, 0.35f);
    c (ID::delayMode, "Delay Mode", Choices::delayModes, 0);
    f (ID::revMix, "Reverb Mix", 0, 1, 0.15f);
    f (ID::revSize, "Reverb Size", 0, 1, 0.6f);
    c (ID::revType, "Reverb Type", Choices::revTypes, 0);
    f (ID::eqLow, "EQ Low", -12, 12, 0);
    f (ID::eqHigh, "EQ High", -12, 12, 0);
    f (ID::reverse, "Reverse", 0, 1, 0);
    b (ID::freeze, "Freeze", false);
    f (ID::width, "Width", 0, 1, 0.5f);
    f (ID::gain, "Output", -24.0f, 12.0f, 0.0f);

    f (ID::m1, "Macro 1", 0, 1, 0.5f);
    f (ID::m2, "Macro 2", 0, 1, 0.25f);
    f (ID::m3, "Macro 3", 0, 1, 0.0f);
    f (ID::m4, "Macro 4", 0, 1, 0.0f);
    f (ID::m5, "Macro 5", 0, 1, 0.2f);
    f (ID::m6, "Macro 6", 0, 1, 0.5f);
    f (ID::m7, "Macro 7", 0, 1, 0.0f);
    f (ID::m8, "Macro 8", 0, 1, 0.5f);

    f (ID::ghost, "Ghost", 0, 1, 0);
    c (ID::ghostOct, "Ghost Octave", Choices::ghostOcts, 0);
    b (ID::ghostRev, "Ghost Reverse", true);
    f (ID::ghostBlur, "Ghost Blur", 0, 1, 0.6f);
    f (ID::bend, "Bend", 0, 1, 0);
    c (ID::bendMode, "Bend Mode", Choices::bendModes, 0);
    f (ID::bendSemis, "Bend Range", -24, 24, -12);
    b (ID::tape, "Broken Tape", false);
    f (ID::circuit, "Circuit", 0, 1, 0);
    c (ID::circRate, "Circuit Rate", Choices::circRates, 1);
    f (ID::chaos, "Chaos", 0, 1, 0.4f);
    f (ID::morphX, "Era Morph X", 0, 1, 0.5f);
    f (ID::morphY, "Era Morph Y", 0, 1, 0.5f);
    c (ID::body, "Body Swap", Choices::bodies, 0);
    f (ID::bodyMix, "Body Mix", 0, 1, 0.5f);
    i (ID::seed, "Seed", 0, 9999, 1234);
    i (ID::alive, "Alive", 0, 5, 0);
    f (ID::drift, "Pitch Drift", 0, 1, 0);
    f (ID::timeM, "Time", 0, 1, 0.33f);
    f (ID::punch, "Punch", 0, 1, 0);
    f (ID::halftime, "Half-Time", 0, 1, 0);
    {
        auto eraText = [] (float v, int) { static const char* n[] { "2010", "2013", "2016", "2019", "2022", "2025", "FUTURE" };
                                           const int i = juce::jlimit (0, 6, (int) std::round (v)); return juce::String (n[i]) + (std::abs (v - (float) i) > 0.05f ? "~" : ""); };
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { ID::era, 1 }, "ERA", NormalisableRange<float> (0.0f, 6.0f), 0.0f,
                                                     AudioParameterFloatAttributes().withStringFromValueFunction (eraText)));
    }
    i (ID::eraHome, "ERA Home", 0, 6, 0);
    f (ID::future, "FUTURE", 0, 1, 0);
    i (ID::arpSteps, "Arp Steps", 1, 16, 16);
    for (int st = 0; st < 16; ++st)
        f (ID::arpStep (st), "Arp Step " + String (st + 1), 0, 1, 1.0f);
    for (int st = 0; st < 16; ++st)
        i (ID::arpNote (st), "Arp Note " + String (st + 1), -12, 12, 0);
    for (int st = 0; st < 16; ++st)
        i (ID::arpLen (st), "Arp Length " + String (st + 1), 1, 16, 1);

    for (int s = 0; s < numModSlots; ++s)
    {
        c (ID::mmSrc (s), "Mod " + String (s + 1) + " Source", Choices::modSources, 0);
        c (ID::mmDst (s), "Mod " + String (s + 1) + " Dest", Choices::modDests, 1);
        f (ID::mmAmt (s), "Mod " + String (s + 1) + " Amount", -1, 1, 0);
    }
    f (ID::master, "Master", 0, 1, 0.7f);
    b (ID::halfOn, "Half On", false);
    i (ID::halfPreset, "Half Preset", 0, 95, 0);
    f (ID::halfAmount, "Half Amount", 0, 1, 1.0f);
    c (ID::halfSpeed, "Half Speed", Choices::halfSpeeds, 2);
    c (ID::halfTrig, "Half Trigger", Choices::halfTrigs, 0);
    f (ID::halfMix, "Half Mix", 0, 1, 1.0f);
    // v0.15 modules
    c (ID::playMode, "Keys Play", Choices::playModes, 0);
    f (ID::b8Tune, "808 Tune", -12, 12, 0); f (ID::b8Decay, "808 Decay", 0.1f, 4.0f, 1.6f, 1.0f); f (ID::b8Punch, "808 Punch", 0, 1, 0.4f);
    f (ID::b8Glide, "808 Glide", 0, 0.5f, 0.08f); f (ID::b8Tone, "808 Tone", 0, 1, 0.25f); f (ID::b8Drive, "808 Drive", 0, 100, 30);
    c (ID::b8Sat, "808 Saturation", Choices::satModes, 1); f (ID::b8Clip, "808 Clip", 0, 12, 3); f (ID::b8Level, "808 Level", -24, 6, 0);
    f (ID::b8Click, "808 Click", 0, 1, 0.3f); f (ID::b8Width, "808 Width", 0, 1, 0.3f); f (ID::b8Sub, "808 Sub Octave", 0, 1, 0);
    f (ID::htPan, "Hat Auto-Pan", 0, 1, 0.3f);
    c (ID::world, "Sound World", Choices::worlds, 0); f (ID::worldAmt, "World Amount", 0, 1, 1.0f);
    c (ID::gate, "Trance Gate", Choices::gates, 0); f (ID::gateDepth, "Gate Depth", 0, 1, 1.0f);
    c (ID::clipMode, "Clipper", Choices::clipModes, 0); f (ID::clipDrive, "Clip Drive", 0, 12, 3);
    f (ID::snTune, "Snare Tune", -12, 12, 0); f (ID::snBody, "Snare Body", 0, 1, 0.5f); f (ID::snSnap, "Snare Snap", 0, 1, 0.6f);
    f (ID::snDecay, "Snare Decay", 0, 1, 0.4f); f (ID::snTone, "Snare Tone", 0, 1, 0.5f); f (ID::snLevel, "Snare Level", -24, 6, 0);
    f (ID::clTune, "Clap Tune", -12, 12, 0); f (ID::clSpread, "Clap Spread", 0, 1, 0.5f); f (ID::clDecay, "Clap Decay", 0, 1, 0.4f);
    f (ID::clTone, "Clap Tone", 0, 1, 0.5f); f (ID::clWidth, "Clap Width", 0, 1, 0.5f); f (ID::clLevel, "Clap Level", -24, 6, 0);
    f (ID::htTune, "Hat Tune", -12, 12, 0); f (ID::htDecay, "Hat Decay", 0.01f, 0.6f, 0.07f, 0.12f); f (ID::htTone, "Hat Tone", 0, 1, 0.5f);
    f (ID::htLevel, "Hat Level", -24, 6, -3);
    b (ID::rlOn, "Rolls On", false); c (ID::rlStyle, "Rolls Style", Choices::rollStyles, 0); i (ID::rlSeed, "Rolls Pattern", 0, 99999, 1);
    c (ID::rlBars, "Rolls Length", Choices::rollBars, 1); f (ID::rlDensity, "Rolls Density", 0, 1, 0.5f);
    b (ID::efxOn, "Effector On", false); i (ID::efxPreset, "Effector Preset", 0, 299, 0); f (ID::efxBlend, "Effector Blend", 0, 1, 1.0f);
    for (int m = 0; m < 5; ++m) f (ID::efxMacro (m), "Effector Macro " + String (m + 1), 0, 1, 0.5f);
    c (ID::dgMode, "Digga Mode", Choices::diggaModes, 0); c (ID::dgSlices, "Digga Slices", Choices::diggaSlices, 1);
    c (ID::dgChop, "Digga Chop", Choices::diggaChops, 0); f (ID::dgPitch, "Digga Pitch", -24, 24, 0); b (ID::dgRev, "Digga Reverse", false);
    f (ID::dgLevel, "Digga Level", -24, 6, 0);
    return l;
}
