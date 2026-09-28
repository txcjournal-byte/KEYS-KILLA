#include "Presets.h"
#include "Params.h"
#include "PresetGains.h"

// Factory presets. Values are real parameter values; anything not listed uses the default.
// Names are neutral descriptions only (instrument, scene, mood, year).

const juce::StringArray& tileNames()
{
    static const juce::StringArray t { "BELLS", "KEYS", "PLUCKS", "FLUTES", "CHOIR", "PADS", "LEADS", "BASS", "EXOTIC", "EXPERIMENTAL" };
    return t;
}

const juce::StringArray& eraNames()
{
    static const juce::StringArray e { "2010", "2013", "2016", "2019", "2022", "2026" };
    return e;
}

const std::vector<Preset>& factoryPresets()
{
    using namespace ID;
    static const std::vector<Preset> presets = []
    {
        std::vector<Preset> v;
        using Vals = std::vector<std::pair<juce::String, float>>;
        auto P = [&v] (const char* name, int tile, int era, bool excl, const char* sub, Vals vals)
        { v.push_back ({ name, tile, era, excl, sub, std::move (vals), {} }); };

        const float FM = engFM, VA = engVA, PL = engPluck, VX = engVox, OR = engOrgan, FL = engFlute, SB = engSub;
        const float WT = engWavetable, OC = engOrchestral, MD = engModal;
        // mod matrix helper: slot, source, dest, amount
        auto MM = [] (Vals& vals, int slot, int src, int dst, float amt)
        {
            vals.push_back ({ ID::mmSrc (slot), (float) src }); vals.push_back ({ ID::mmDst (slot), (float) dst }); vals.push_back ({ ID::mmAmt (slot), amt });
        };

        // ---------------- BELLS ----------------
        P ("Dark Digital Bells 2010", tBells, 0, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.45f }, { fdecay, 1.0f },
            { attack, 0.001f }, { decay, 2.2f }, { sustain, 0 }, { release, 1.2f }, { revMix, 0.2f }, { m2, 0.2f }, { gain, -7.0f } });
        P ("Ice Bells 2013", tBells, 1, false, "", { { engine, FM }, { fmRatio, 4.0f }, { fmAmt, 0.5f }, { fdecay, 1.6f }, { unison, 2 }, { detune, 0.15f },
            { decay, 3.0f }, { sustain, 0 }, { release, 2.0f }, { revMix, 0.35f }, { revSize, 0.85f }, { delayMix, 0.2f }, { m2, 0.4f }, { m6, 0.7f,  }, { gain, -7.1f } });
        P ("Sad Bells", tBells, 2, false, "", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.35f }, { fdecay, 0.8f }, { octave, 1 },
            { decay, 2.5f }, { sustain, 0 }, { release, 1.5f }, { revMix, 0.3f }, { wow, 0.2f }, { m1, 0.4f }, { m2, 0.35f,  }, { gain, -6.9f } });
        P ("Soundcloud Blown Bell", tBells, 2, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.4f }, { decay, 2.0f }, { sustain, 0 },
            { drive, 0.55f }, { driveType, 3 }, { revMix, 0.2f }, { m3, 0.3f }, { gain, -11.5f } });
        P ("Glassy Plugg", tBells, 3, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { wave, 0.3f }, { fdecay, 0.5f }, { octave, 1 },
            { decay, 1.4f }, { sustain, 0.15f }, { release, 0.8f }, { chorus, 0.3f }, { revMix, 0.3f }, { delayMix, 0.2f }, { m2, 0.35f,  }, { gain, -5.7f } });
        P ("Crushed Dark Bells", tBells, 4, false, "", { { engine, FM }, { fmRatio, 5.2f }, { fmAmt, 0.55f }, { fdecay, 0.9f },
            { decay, 2.0f }, { sustain, 0 }, { release, 1.4f }, { crush, 0.4f }, { drive, 0.35f }, { driveType, 2 }, { m1, 0.35f }, { revMix, 0.25f,  }, { gain, -13.3f } });
        P ("Music Box Void", tBells, 5, false, "", { { engine, FM }, { fmRatio, 6.0f }, { fmAmt, 0.25f }, { octave, 2 }, { decay, 1.5f }, { sustain, 0 },
            { release, 1.8f }, { revMix, 0.45f }, { revSize, 0.95f }, { ghost, 0.35f }, { wow, 0.3f }, { m2, 0.4f,  }, { gain, -4.3f } });

        // ---------------- KEYS ----------------
        P ("Bounce Piano", tKeys, 0, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.35f }, { wave, 0.2f }, { fdecay, 0.6f },
            { decay, 1.8f }, { sustain, 0.05f }, { release, 0.4f }, { cutoff, 9000 }, { fenv, 0.2f }, { revMix, 0.15f }, { gain, -5.3f } });
        P ("Church Organ Bounce", tKeys, 0, false, "Organs", { { engine, OR }, { wave, 0.55f }, { attack, 0.01f }, { sustain, 1.0f }, { release, 0.3f },
            { lfoPitch, 0.04f }, { lfoRate, 6.0f }, { revMix, 0.35f }, { revSize, 0.9f }, { gain, -8.9f } });
        P ("Dark Minimal Keys", tKeys, 1, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.2f }, { decay, 2.5f }, { sustain, 0.2f },
            { release, 0.8f }, { cutoff, 3500 }, { revMix, 0.3f }, { m1, 0.4f }, { m2, 0.35f,  }, { gain, -6.4f } });
        P ("Dark Piano Loop", tKeys, 2, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.4f }, { wave, 0.35f }, { fdecay, 0.4f },
            { decay, 2.0f }, { sustain, 0.0f }, { release, 0.6f }, { cutoff, 6000 }, { wow, 0.25f }, { crush, 0.1f }, { revMix, 0.25f }, { m1, 0.4f,  }, { gain, -4.7f } });
        P ("Detroit Keys", tKeys, 3, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.45f }, { fdecay, 0.3f }, { decay, 0.9f },
            { sustain, 0.1f }, { release, 0.3f }, { drive, 0.2f }, { driveType, 1 }, { revMix, 0.12f,  }, { gain, -9.8f } });
        P ("Pluggnb Keys", tKeys, 3, false, "", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.25f }, { unison, 3 }, { detune, 0.12f },
            { decay, 1.6f }, { sustain, 0.3f }, { release, 0.9f }, { chorus, 0.5f }, { revMix, 0.35f }, { m5, 0.3f }, { m6, 0.7f,  }, { gain, -6.3f } });
        P ("UK Drill Slide Piano", tKeys, 3, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.8f }, { sustain, 0.1f },
            { release, 0.5f }, { mono, 1 }, { glide, 0.12f }, { revMix, 0.2f,  }, { gain, -2.2f } });
        P ("Rage Keys 2024", tKeys, 4, false, "", { { engine, VA }, { wave, 0.3f }, { unison, 5 }, { detune, 0.35f }, { cutoff, 5000 }, { fenv, 0.4f }, { fdecay, 0.3f },
            { decay, 0.8f }, { sustain, 0.4f }, { release, 0.3f }, { drive, 0.3f }, { revMix, 0.2f }, { gain, -9.7f } });

        // ---------------- PLUCKS ----------------
        P ("Pizz Dark", tPlucks, 2, false, "Strings", { { engine, PL }, { wave, 0.3f }, { decay, 0.5f }, { sustain, 0 }, { release, 0.2f },
            { cutoff, 5000 }, { revMix, 0.3f }, { m1, 0.45f,  }, { gain, 5.2f } });
        P ("Emo Guitar Pick", tPlucks, 2, false, "Guitars", { { engine, PL }, { wave, 0.6f }, { decay, 2.0f }, { sustain, 0 }, { release, 0.4f },
            { chorus, 0.35f }, { revMix, 0.3f }, { drive, 0.1f }, { gain, -4.0f } });
        P ("Sample Guitar Vibe", tPlucks, 3, false, "Guitars", { { engine, PL }, { wave, 0.5f }, { decay, 2.5f }, { sustain, 0 }, { release, 0.6f },
            { wow, 0.35f }, { crush, 0.12f }, { revMix, 0.3f }, { m4, 0.2f,  }, { gain, 0.2f } });
        P ("Digicore Pluck", tPlucks, 4, false, "", { { engine, VA }, { wave, 0.2f }, { unison, 3 }, { detune, 0.25f }, { cutoff, 1500 }, { fenv, 0.7f },
            { fdecay, 0.15f }, { decay, 0.25f }, { sustain, 0 }, { release, 0.2f }, { octave, 1 }, { delayMix, 0.25f }, { revMix, 0.2f }, { gain, 9.3f } });
        P ("Hyper Pluck", tPlucks, 5, false, "", { { engine, VA }, { wave, 0.8f }, { unison, 5 }, { detune, 0.3f }, { cutoff, 2000 }, { fenv, 0.8f },
            { fdecay, 0.12f }, { decay, 0.2f }, { sustain, 0 }, { release, 0.15f }, { octave, 1 }, { drive, 0.35f }, { driveType, 2 }, { delayMix, 0.3f }, { gain, -7.7f } });
        P ("Early Plugg Pluck", tPlucks, 1, false, "", { { engine, VA }, { wave, 0.5f }, { cutoff, 2500 }, { fenv, 0.5f }, { fdecay, 0.2f },
            { decay, 0.4f }, { sustain, 0.1f }, { release, 0.3f }, { revMix, 0.3f }, { delayMix, 0.2f,  }, { gain, -8.4f } });

        // ---------------- FLUTES ----------------
        P ("Old Trap Flute", tFlutes, 0, false, "", { { engine, FL }, { wave, 0.3f }, { attack, 0.03f }, { sustain, 0.9f }, { release, 0.3f },
            { lfoPitch, 0.1f }, { lfoRate, 5.5f }, { crush, 0.15f }, { revMix, 0.25f,  }, { gain, -12.2f } });
        P ("Trap Flute 2016", tFlutes, 2, false, "", { { engine, FL }, { wave, 0.45f }, { attack, 0.04f }, { sustain, 0.85f }, { release, 0.35f },
            { lfoPitch, 0.14f }, { lfoRate, 5.0f }, { revMix, 0.3f }, { delayMix, 0.15f }, { m2, 0.35f,  }, { gain, -12.1f } });
        P ("Whistle Lead ATL", tFlutes, 0, false, "", { { engine, FL }, { wave, 0.1f }, { octave, 1 }, { attack, 0.02f }, { sustain, 1.0f },
            { mono, 1 }, { glide, 0.08f }, { lfoPitch, 0.12f }, { revMix, 0.25f }, { gain, -7.2f } });
        P ("Pan Flute Night", tFlutes, 3, false, "", { { engine, FL }, { wave, 0.7f }, { attack, 0.06f }, { sustain, 0.8f }, { release, 0.5f },
            { lfoPitch, 0.08f }, { revMix, 0.4f }, { revSize, 0.85f }, { wow, 0.2f,  }, { gain, -12.7f } });
        P ("Ocarina 2025", tFlutes, 5, false, "", { { engine, FL }, { wave, 0.2f }, { octave, 1 }, { attack, 0.02f }, { sustain, 0.9f }, { release, 0.4f },
            { bend, 0.25f }, { revMix, 0.35f }, { delayMix, 0.25f }, { ghost, 0.2f,  }, { gain, -14.1f } });

        // ---------------- CHOIR ----------------
        P ("Dark Choir Stab", tChoir, 0, false, "", { { engine, VX }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.01f },
            { decay, 0.6f }, { sustain, 0.3f }, { release, 0.5f }, { octave, -1 }, { revMix, 0.4f }, { revSize, 0.9f }, { crush, 0.1f,  }, { gain, 6.4f } });
        P ("Ethereal Vox", tChoir, 1, false, "", { { engine, VX }, { wave, 0.3f }, { unison, 5 }, { detune, 0.35f }, { attack, 0.4f },
            { sustain, 0.9f }, { release, 1.5f }, { lfoPitch, 0.05f }, { chorus, 0.5f }, { revMix, 0.5f }, { revSize, 0.95f }, { m6, 0.8f,  }, { gain, -1.3f } });
        P ("Ooh Aah Choir", tChoir, 2, false, "", { { engine, VX }, { wave, 0.75f }, { unison, 7 }, { detune, 0.25f }, { attack, 0.2f },
            { sustain, 0.9f }, { release, 1.0f }, { revMix, 0.4f }, { m5, 0.3f,  }, { gain, -2.0f } });
        P ("Ghost Vox Lead", tChoir, 5, false, "", { { engine, VX }, { wave, 0.5f }, { unison, 3 }, { detune, 0.2f }, { mono, 1 }, { glide, 0.1f },
            { attack, 0.05f }, { sustain, 0.9f }, { ghost, 0.4f }, { bend, 0.2f }, { revMix, 0.35f }, { delayMix, 0.3f,  }, { gain, 5.3f } });

        // ---------------- PADS (+Strings) ----------------
        P ("Horror Strings", tPads, 0, false, "Strings", { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { cutoff, 3500 },
            { attack, 0.3f }, { sustain, 0.9f }, { release, 1.2f }, { lfoPitch, 0.06f }, { revMix, 0.4f }, { revSize, 0.9f,  }, { gain, -10.1f } });
        P ("Cloud Pad", tPads, 1, false, "", { { engine, VA }, { wave, 0.1f }, { unison, 7 }, { detune, 0.4f }, { cutoff, 2200 }, { attack, 0.8f },
            { sustain, 1.0f }, { release, 2.5f }, { chorus, 0.5f }, { revMix, 0.55f }, { revSize, 0.95f }, { m5, 0.4f }, { m6, 0.8f,  }, { gain, -4.4f } });
        P ("Vapor Chords", tPads, 1, false, "", { { engine, VA }, { wave, 0.4f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 2800 }, { attack, 0.05f },
            { sustain, 0.8f }, { release, 1.0f }, { wow, 0.45f }, { chorus, 0.4f }, { revMix, 0.4f }, { m4, 0.3f,  }, { gain, -10.5f } });
        P ("NY Drill Strings", tPads, 3, false, "Strings", { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.25f }, { cutoff, 5000 },
            { attack, 0.08f }, { sustain, 0.9f }, { release, 0.5f }, { mono, 1 }, { glide, 0.15f }, { revMix, 0.3f,  }, { gain, -5.3f } });
        P ("Static Void Pad", tPads, 4, false, "", { { engine, VX }, { wave, 0.9f }, { unison, 7 }, { detune, 0.5f }, { attack, 1.0f }, { sustain, 1 },
            { release, 3.0f }, { crush, 0.25f }, { wow, 0.3f }, { revMix, 0.6f }, { revSize, 0.98f }, { ghost, 0.3f,  }, { gain, -7.5f } });
        P ("Neon Trance Chords", tPads, 5, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.45f }, { cutoff, 3000 }, { fenv, 0.3f },
            { fdecay, 0.25f }, { attack, 0.003f }, { decay, 0.4f }, { sustain, 0.5f }, { release, 0.3f }, { lfoAmp, 0.6f }, { lfoSync, 1 }, { lfoDiv, 4 },
            { delayMix, 0.25f }, { revMix, 0.35f }, { gain, -1.5f } });

        // ---------------- LEADS (+Brass) ----------------
        P ("Epic Brass 2010", tLeads, 0, false, "Brass", { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.15f }, { cutoff, 900 }, { fenv, 0.6f },
            { fdecay, 0.3f }, { attack, 0.02f }, { sustain, 0.8f }, { release, 0.25f }, { revMix, 0.35f }, { revSize, 0.9f }, { crush, 0.1f,  }, { gain, -7.4f } });
        P ("Block Brass 2012", tLeads, 0, false, "Brass", { { engine, VA }, { wave, 0.15f }, { unison, 3 }, { detune, 0.12f }, { cutoff, 1200 }, { fenv, 0.5f },
            { fdecay, 0.2f }, { decay, 0.3f }, { sustain, 0.4f }, { release, 0.15f }, { octave, -1 }, { revMix, 0.3f,  }, { gain, 5.9f } });
        P ("Early Plugg Synth", tLeads, 1, false, "", { { engine, VA }, { wave, 0.5f }, { cutoff, 3500 }, { sustain, 0.8f }, { release, 0.4f },
            { mono, 1 }, { glide, 0.06f }, { lfoPitch, 0.05f }, { revMix, 0.3f }, { delayMix, 0.25f,  }, { gain, -7.4f } });
        P ("Early Rage Saw", tLeads, 3, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { cutoff, 6000 },
            { sustain, 0.9f }, { release, 0.25f }, { drive, 0.25f }, { revMix, 0.25f }, { gain, -14.7f } });
        P ("Rage Supersaw", tLeads, 4, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.5f }, { cutoff, 9000 },
            { sustain, 0.9f }, { release, 0.3f }, { drive, 0.35f }, { driveType, 1 }, { chorus, 0.2f }, { revMix, 0.3f }, { m6, 0.8f }, { gain, -14.8f } });
        P ("Jerk Synth", tLeads, 4, false, "", { { engine, VA }, { wave, 0.7f }, { unison, 3 }, { detune, 0.15f }, { cutoff, 2500 }, { fenv, 0.5f },
            { fdecay, 0.1f }, { decay, 0.2f }, { sustain, 0.2f }, { release, 0.1f }, { drive, 0.3f }, { driveType, 2 }, { gain, -11.1f } });
        P ("Hyper Lead", tLeads, 4, false, "", { { engine, VA }, { wave, 0.6f }, { unison, 5 }, { detune, 0.3f }, { octave, 1 }, { cutoff, 8000 },
            { mono, 1 }, { glide, 0.05f }, { lfoPitch, 0.08f }, { drive, 0.3f }, { crush, 0.15f }, { delayMix, 0.25f }, { gain, -14.7f } });
        P ("Blown Rage Lead", tLeads, 5, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.45f }, { cutoff, 7000 },
            { sustain, 0.9f }, { release, 0.3f }, { drive, 0.7f }, { driveType, 3 }, { revMix, 0.2f }, { m3, 0.3f }, { m6, 0.8f }, { gain, -11.7f } });
        P ("Club Rage Stab", tLeads, 5, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { cutoff, 2500 }, { fenv, 0.6f },
            { fdecay, 0.15f }, { decay, 0.25f }, { sustain, 0.0f }, { release, 0.2f }, { drive, 0.3f }, { delayMix, 0.3f }, { revMix, 0.3f }, { gain, -7.8f } });
        P ("Glitch Stab", tLeads, 5, false, "", { { engine, VA }, { wave, 0.8f }, { unison, 3 }, { detune, 0.3f }, { cutoff, 3000 }, { fenv, 0.5f },
            { decay, 0.3f }, { sustain, 0.1f }, { circuit, 0.45f }, { crush, 0.2f }, { revMix, 0.2f }, { gain, -5.7f } });

        // ---------------- BASS (mono + bass mode added automatically) ----------------
        auto B = [&] (const char* name, int era, Vals vals, bool excl = false)
        {
            Vals base { { m1, 0.0f }, { m2, 0.0f }, { m3, 0.0f }, { m4, 0.0f }, { m5, 0.5f }, { m6, 0.0f },
                                                              { revMix, 0.0f }, { width, 0.4f }, { octave, -1 } };
            base.insert (base.end(), vals.begin(), vals.end());
            P (name, tBass, era, excl, "", base);
        };
        B ("Pure Sub", 0, { { engine, SB }, { wave, 0.0f }, { sustain, 1 }, { release, 0.2f,  }, { gain, -5.5f } });
        B ("Warm Sub", 2, { { engine, SB }, { wave, 0.35f }, { sustain, 1 }, { release, 0.25f }, { m3, 0.1f,  }, { gain, -8.9f } });
        B ("Synth 808 Long", 2, { { engine, SB }, { wave, 0.4f }, { fmAmt, 0.15f }, { decay, 3.5f }, { sustain, 0 }, { release, 0.5f }, { m6, 0.3f,  }, { gain, -6.6f } });
        B ("Synth 808 Punch", 3, { { engine, SB }, { wave, 0.6f }, { fmAmt, 0.3f }, { decay, 1.2f }, { sustain, 0 }, { release, 0.2f }, { m3, 0.35f }, { m6, 0.6f,  }, { gain, -10.3f } });
        B ("Slide 808", 3, { { engine, SB }, { wave, 0.5f }, { fmAmt, 0.12f }, { decay, 4.0f }, { sustain, 0.2f }, { release, 0.4f }, { glide, 0.15f }, { m4, 0.3f,  }, { gain, -6.8f } });
        B ("Dark Reese", 3, { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.35f }, { cutoff, 700 }, { sustain, 1 }, { release, 0.2f }, { m1, 0.4f,  }, { gain, -2.0f } });
        B ("Drill Reese", 3, { { engine, VA }, { wave, 0.0f }, { unison, 2 }, { detune, 0.4f }, { cutoff, 900 }, { sustain, 1 }, { glide, 0.1f }, { m1, 0.5f }, { m3, 0.2f,  }, { gain, -6.7f } });
        B ("Wobble 1/8", 4, { { engine, VA }, { wave, 0.1f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 3000 }, { reso, 0.35f }, { sustain, 1 },
                               { lfoSync, 1 }, { lfoDiv, 3 }, { m1, 0.5f }, { m2, 0.7f }, { m3, 0.25f,  }, { gain, -3.9f } });
        B ("Triplet Wobble", 4, { { engine, VA }, { wave, 0.4f }, { unison, 2 }, { detune, 0.2f }, { cutoff, 2500 }, { reso, 0.4f }, { sustain, 1 },
                               { lfoSync, 1 }, { lfoDiv, 7 }, { m1, 0.5f }, { m2, 0.75f }, { m3, 0.3f,  }, { gain, -8.5f } });
        B ("Plugg Square Bass", 3, { { engine, VA }, { wave, 0.5f }, { cutoff, 1200 }, { fenv, 0.3f }, { fdecay, 0.15f }, { decay, 0.35f }, { sustain, 0.3f },
                               { release, 0.1f }, { m1, 0.4f,  }, { gain, -3.9f } });
        B ("Pluggnb Soft Bass", 3, { { engine, VA }, { wave, 0.25f }, { cutoff, 600 }, { decay, 0.6f }, { sustain, 0.5f }, { release, 0.2f }, { glide, 0.05f }, { m1, 0.5f,  }, { gain, 3.6f } });
        B ("Blown Rage Bass", 5, { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.25f }, { cutoff, 2500 }, { sustain, 1 }, { drive, 0.5f },
                               { driveType, 3 }, { m1, 0.6f }, { m3, 0.5f }, { gain, -10.2f } });
        B ("Acid Slide", 1, { { engine, VA }, { wave, 0.0f }, { cutoff, 500 }, { reso, 0.75f }, { fenv, 0.6f }, { fdecay, 0.2f }, { sustain, 0.7f },
                               { glide, 0.12f }, { drive, 0.2f }, { m1, 0.2f,  }, { gain, -5.7f } });
        B ("FM Punch Bass", 0, { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.5f }, { fdecay, 0.15f }, { decay, 0.5f }, { sustain, 0.5f }, { release, 0.1f },
                               { m1, 0.4f }, { m6, 0.4f,  }, { gain, -0.6f } });
        B ("Formant Growl", 4, { { engine, VX }, { wave, 0.3f }, { unison, 3 }, { detune, 0.2f }, { sustain, 1 }, { lfoSync, 1 }, { lfoDiv, 4 },
                               { m2, 0.5f }, { m3, 0.4f }, { m1, 0.5f,  }, { gain, -1.1f } });
        B ("Memphis Dirt Bass", 0, { { engine, SB }, { wave, 0.8f }, { decay, 1.5f }, { sustain, 0.3f }, { crush, 0.3f }, { drive, 0.4f },
                               { driveType, 1 }, { m3, 0.3f }, { m1, 0.2f,  }, { gain, -10.9f } });

        // ---------------- EXOTIC ----------------
        P ("Koto Dark", tExotic, 2, false, "", { { engine, PL }, { wave, 0.75f }, { decay, 1.5f }, { sustain, 0 }, { release, 0.4f }, { bend, 0.15f },
            { revMix, 0.3f,  }, { gain, -0.5f } });
        P ("Sitar Night", tExotic, 3, false, "", { { engine, PL }, { wave, 0.9f }, { decay, 2.5f }, { sustain, 0 }, { body, 6 }, { bodyMix, 0.35f },
            { revMix, 0.3f }, { bend, 0.1f,  }, { gain, 0.5f } });
        P ("Kalimba Minor", tExotic, 3, false, "Mallets", { { engine, FM }, { fmRatio, 5.4f }, { fmAmt, 0.3f }, { fdecay, 0.15f }, { decay, 1.0f },
            { sustain, 0 }, { release, 0.6f }, { body, 1 }, { bodyMix, 0.3f }, { revMix, 0.25f }, { gain, -3.8f } });
        P ("Marimba Heat", tExotic, 1, false, "Mallets", { { engine, FM }, { fmRatio, 4.0f }, { fmAmt, 0.35f }, { fdecay, 0.08f }, { decay, 0.6f },
            { sustain, 0 }, { release, 0.4f }, { revMix, 0.2f }, { gain, -5.2f } });
        P ("Harp Spell", tExotic, 0, false, "", { { engine, PL }, { wave, 0.4f }, { decay, 3.0f }, { sustain, 0 }, { release, 1.0f }, { revMix, 0.45f,  }, { gain, 0.4f } });

        // ---------------- EXPERIMENTAL ----------------
        P ("Melting Piano", tExperimental, -1, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.35f }, { decay, 2.0f }, { sustain, 0.1f },
            { release, 1.0f }, { bend, 0.7f }, { wow, 0.5f }, { revMix, 0.3f,  }, { gain, -5.3f } });
        P ("Reverse Keys", tExperimental, -1, false, "", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.3f }, { attack, 0.8f }, { sustain, 0.6f },
            { release, 0.05f }, { ghost, 0.6f }, { revMix, 0.4f,  }, { gain, -9.1f } });
        P ("Frozen Void", tExperimental, -1, false, "", { { engine, VX }, { wave, 0.9f }, { unison, 7 }, { detune, 0.6f }, { attack, 1.5f },
            { sustain, 1 }, { release, 4.0f }, { revMix, 0.8f }, { revSize, 1.0f }, { ghost, 0.5f,  }, { gain, -8.9f } });
        P ("Radio Ghost", tExperimental, -1, false, "", { { engine, VA }, { wave, 0.5f }, { cutoff, 2500 }, { reso, 0.5f }, { sustain, 0.8f },
            { crush, 0.5f }, { wow, 0.6f }, { ghost, 0.4f }, { circuit, 0.2f }, { revMix, 0.3f,  }, { gain, -12.3f } });
        P ("Rubber Pluck", tExperimental, -1, false, "", { { engine, PL }, { wave, 0.3f }, { decay, 0.8f }, { sustain, 0 }, { bend, 0.55f },
            { body, 5 }, { bodyMix, 0.4f,  }, { gain, 3.8f } });
        P ("Mystery Scale Plucks", tExperimental, -1, false, "", { { engine, FM }, { fmRatio, 7.13f }, { fmAmt, 0.4f }, { decay, 0.7f }, { sustain, 0 },
            { circuit, 0.25f }, { delayMix, 0.35f }, { revMix, 0.3f,  }, { gain, -5.1f } });

        // ---------------- EXCLUSIVE (built on sections 5.2 - 5.6) ----------------
        P ("Kalimba Supersaw", tLeads, 5, true, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { decay, 0.6f }, { sustain, 0.2f },
            { body, 1 }, { bodyMix, 0.65f }, { revMix, 0.3f }, { gain, -0.3f } });
        P ("Haunted Bell Ghost", tBells, 4, true, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.45f }, { decay, 2.5f }, { sustain, 0 },
            { release, 2.0f }, { ghost, 0.75f }, { revMix, 0.35f,  }, { gain, -6.5f } });
        P ("Circuit Bent Flute", tFlutes, 5, true, "", { { engine, FL }, { wave, 0.4f }, { sustain, 0.9f }, { lfoPitch, 0.1f }, { circuit, 0.6f },
            { crush, 0.15f }, { revMix, 0.3f,  }, { gain, -12.4f } });
        P ("Bent Choir", tChoir, 4, true, "", { { engine, VX }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.1f }, { sustain, 0.9f },
            { release, 1.2f }, { bend, 0.8f }, { revMix, 0.45f,  }, { gain, -0.5f } });
        P ("2012 > 2026 Morph", tPads, 5, true, "", { { engine, VA }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { cutoff, 4000 }, { attack, 0.2f },
            { sustain, 0.9f }, { release, 1.0f }, { morphX, 0.85f }, { morphY, 0.15f }, { revMix, 0.3f }, { gain, -11.4f } });
        P ("Glass Pipe Choir", tChoir, 5, true, "", { { engine, VX }, { wave, 0.6f }, { unison, 5 }, { detune, 0.25f }, { attack, 0.3f }, { sustain, 1 },
            { release, 1.5f }, { body, 4 }, { bodyMix, 0.5f }, { ghost, 0.3f }, { revMix, 0.4f,  }, { gain, -4.0f } });
        P ("Metal Pipe Bass", tBass, 5, true, "", { { engine, VA }, { wave, 0.3f }, { octave, -1 }, { cutoff, 1500 }, { sustain, 1 }, { body, 4 },
            { bodyMix, 0.4f }, { m1, 0.5f }, { m5, 0.5f }, { revMix, 0 }, { m2, 0 }, { m6, 0,  }, { gain, 3.2f } });
        P ("Broken Tape Keys", tKeys, 5, true, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 2.0f }, { sustain, 0.2f },
            { bend, 0.9f }, { circuit, 0.2f }, { wow, 0.4f }, { revMix, 0.3f,  }, { gain, -5.6f } });

        // ================= new engines: wavetable, orchestral, modal, layers A+B =================
        P ("Modal Bell Tower", tBells, 0, false, "", { { engine, MD }, { wave, 0.85f }, { fmAmt, 0.5f }, { decay, 3.0f }, { sustain, 0 }, { release, 2.0f },
            { revMix, 0.35f }, { revSize, 0.9f } });
        P ("Glass Wavetable Bells", tBells, 3, false, "", { { engine, WT }, { wave, 0.71f }, { octave, 1 }, { decay, 1.6f }, { sustain, 0 }, { release, 1.2f },
            { cutoff, 9000 }, { fenv, 0.3f }, { delayMix, 0.25f }, { revMix, 0.3f } });
        P ("Tine Keys 2019", tKeys, 3, false, "", { { engine, FM }, { fmAlgo, 5 }, { fmRatio, 1.0f }, { fmRatio2, 14.0f }, { fmAmt, 0.35f },
            { fdecay, 0.5f }, { decay, 2.0f }, { sustain, 0.15f }, { release, 0.6f }, { chorus, 0.35f }, { revMix, 0.2f } });
        { Vals v2 { { engine, FM }, { fmAlgo, 5 }, { fmRatio, 1.0f }, { fmRatio2, 14.0f }, { fmAmt, 0.3f }, { decay, 2.2f }, { sustain, 0.2f },
                    { release, 0.8f }, { layerB, 1 }, { engineB, WT }, { waveB, 0.0f }, { octaveB, 1 }, { levelB, 0.35f }, { chorus, 0.3f }, { revMix, 0.3f } };
          P ("Layered EP Dream", tKeys, 4, false, "", v2); }
        P ("Organ Ladder Grit", tKeys, 2, false, "Organs", { { engine, OR }, { wave, 0.7f }, { filterType, 1 }, { cutoff, 2500 }, { reso, 0.3f },
            { sustain, 1 }, { release, 0.2f }, { drive, 0.25f }, { driveType, 1 }, { revMix, 0.2f } });
        P ("WT Digi Pluck", tPlucks, 4, false, "", { { engine, WT }, { wave, 1.0f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 1800 }, { fenv, 0.7f },
            { fdecay, 0.15f }, { decay, 0.3f }, { sustain, 0 }, { release, 0.2f }, { delayMix, 0.3f }, { delayMode, 0 } });
        P ("Harp Arp 2014", tPlucks, 1, false, "", { { engine, PL }, { wave, 0.45f }, { decay, 2.5f }, { sustain, 0 }, { release, 0.8f },
            { revMix, 0.4f }, { revType, 1 } });
        P ("Wavetable Whistle", tFlutes, 4, false, "", { { engine, WT }, { wave, 0.05f }, { octave, 1 }, { attack, 0.03f }, { sustain, 0.9f }, { release, 0.3f },
            { lfoPitch, 0.12f }, { mono, 1 }, { glide, 0.06f }, { revMix, 0.3f }, { delayMix, 0.2f } });
        P ("Orchestra Choir Hit 2011", tChoir, 0, false, "", { { engine, OC }, { wave, 1.0f }, { fmAmt, 0.0f }, { unison, 5 }, { detune, 0.3f },
            { attack, 0.005f }, { decay, 0.7f }, { sustain, 0.2f }, { release, 0.5f }, { revMix, 0.4f }, { revSize, 0.9f } });
        { Vals v2 { { engine, VX }, { wave, 0.35f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.3f }, { sustain, 1 }, { release, 1.5f },
                    { layerB, 1 }, { engineB, WT }, { waveB, 0.7f }, { unisonB, 3 }, { detuneB, 0.3f }, { levelB, 0.4f }, { revMix, 0.45f } };
          MM (v2, 0, srcLfo2, dstWaveB, 0.25f);
          P ("Synth Vox Layer", tChoir, 5, false, "", v2); }
        P ("Orchestral Strings 2012", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.2f },
            { sustain, 0.9f }, { release, 0.8f }, { lfoPitch, 0.05f }, { revMix, 0.4f }, { revSize, 0.9f } });
        { Vals v2 { { engine, WT }, { wave, 0.4f }, { unison, 5 }, { detune, 0.35f }, { attack, 0.6f }, { sustain, 1 }, { release, 2.0f },
                    { cutoff, 5000 }, { lfo2Rate, 0.15f }, { chorus, 0.3f }, { revMix, 0.5f }, { revType, 2 } };
          MM (v2, 0, srcLfo2, dstWaveA, 0.35f); MM (v2, 1, srcLfo1, dstPan, 0.3f);
          P ("Wavetable Motion Pad", tPads, 4, false, "", v2); }
        { Vals v2 { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.35f }, { attack, 0.5f }, { sustain, 1 }, { release, 2.0f }, { cutoff, 4000 },
                    { layerB, 1 }, { engineB, WT }, { waveB, 0.71f }, { octaveB, 1 }, { levelB, 0.45f }, { phaser, 0.35f }, { revMix, 0.5f } };
          P ("Layered Glass Pad", tPads, 3, false, "", v2); }
        P ("Orchestral Brass Stab 2010", tLeads, 0, false, "Brass", { { engine, OC }, { wave, 0.0f }, { unison, 3 }, { detune, 0.2f }, { attack, 0.01f },
            { decay, 0.5f }, { sustain, 0.5f }, { release, 0.3f }, { revMix, 0.35f }, { revSize, 0.85f } });
        P ("WT Rage Lead", tLeads, 4, false, "", { { engine, WT }, { wave, 0.285f }, { unison, 7 }, { detune, 0.45f }, { sustain, 0.9f }, { release, 0.3f },
            { drive, 0.35f }, { driveType, 1 }, { revMix, 0.25f } });
        { Vals v2 { { engine, WT }, { wave, 0.3f }, { warpMode, 1 }, { fmAmt, 0.2f }, { unison, 3 }, { detune, 0.2f }, { sustain, 0.9f },
                    { e3attack, 0.001f }, { e3decay, 0.4f }, { mono, 1 }, { glide, 0.05f }, { delayMix, 0.25f } };
          MM (v2, 0, srcEnv3, dstFmA, 0.6f);
          P ("Sync Lead 2025", tLeads, 5, false, "", v2); }
        P ("Stack FM Lead", tLeads, 3, false, "", { { engine, FM }, { fmAlgo, 1 }, { fmRatio, 2.0f }, { fmRatio2, 3.0f }, { fmAmt, 0.35f },
            { sustain, 0.8f }, { release, 0.3f }, { lfoPitch, 0.06f }, { delayMix, 0.2f } });
        P ("Tape Pad Loop", tPads, 2, false, "", { { engine, VA }, { wave, 0.35f }, { unison, 3 }, { cutoff, 2500 }, { attack, 0.3f }, { sustain, 0.9f },
            { release, 1.2f }, { delayMode, 2 }, { delayMix, 0.35f }, { delayFb, 0.55f }, { wow, 0.35f } });
        P ("Kalimba Modal", tExotic, 3, false, "Mallets", { { engine, MD }, { wave, 0.1f }, { fmAmt, 0.4f }, { decay, 1.2f }, { sustain, 0 },
            { release, 0.8f }, { revMix, 0.3f } });
        P ("Marimba Modal 2016", tExotic, 2, false, "Mallets", { { engine, MD }, { wave, 0.5f }, { fmAmt, 0.3f }, { decay, 0.8f }, { sustain, 0 },
            { release, 0.5f }, { revMix, 0.2f } });
        P ("Frozen Cloud", tExperimental, -1, false, "", { { engine, VX }, { wave, 0.6f }, { unison, 7 }, { detune, 0.5f }, { attack, 0.8f }, { sustain, 1 },
            { release, 3.0f }, { revType, 2 }, { revMix, 0.7f }, { freeze, 0 }, { phaser, 0.3f } });
        P ("Phaser Void", tExperimental, -1, false, "", { { engine, WT }, { wave, 0.9f }, { unison, 5 }, { detune, 0.4f }, { attack, 0.5f }, { sustain, 1 },
            { release, 2.0f }, { phaser, 0.8f }, { flanger, 0.3f }, { revMix, 0.5f } });
        P ("Reverse Tape Keys", tExperimental, -1, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.5f }, { sustain, 0.3f },
            { reverse, 0.6f }, { wow, 0.3f }, { revMix, 0.3f } });

        // ---------------- more bass ----------------
        B ("Sub + Click", 3, { { engine, SB }, { wave, 0.1f }, { sustain, 1 }, { release, 0.2f }, { m6, 0.7f } });
        B ("Rage Saw Bass", 4, { { engine, VA }, { wave, 0.0f }, { unison, 2 }, { detune, 0.2f }, { cutoff, 3000 }, { sustain, 1 }, { drive, 0.45f },
                              { driveType, 2 }, { m1, 0.5f }, { m3, 0.35f } });
        B ("Jerk Bounce Bass", 4, { { engine, VA }, { wave, 0.5f }, { cutoff, 1500 }, { fenv, 0.5f }, { fdecay, 0.08f }, { decay, 0.2f }, { sustain, 0.1f },
                              { release, 0.08f }, { m1, 0.4f }, { m6, 0.5f } });
        B ("Club Stab Bass", 5, { { engine, VA }, { wave, 0.1f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 1800 }, { fenv, 0.5f }, { fdecay, 0.12f },
                              { decay, 0.25f }, { sustain, 0.2f }, { m1, 0.4f }, { m3, 0.2f } });
        B ("Digital Bass 2012", 0, { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.45f }, { fdecay, 0.25f }, { sustain, 0.7f }, { crush, 0.15f }, { m1, 0.3f } });
        B ("FM Growl", 4, { { engine, FM }, { fmAlgo, 3 }, { fmRatio, 1.0f }, { fmRatio2, 2.0f }, { fmAmt, 0.5f }, { sustain, 1 }, { lfoSync, 1 }, { lfoDiv, 4 },
                              { wobTarget, 0 }, { m2, 0.4f }, { m3, 0.35f }, { m1, 0.4f } });
        { Vals v2 { { engine, WT }, { wave, 0.5f }, { warpMode, 4 }, { fmAmt, 0.4f }, { sustain, 1 }, { lfo2Sync, 1 }, { lfo2Div, 4 }, { m1, 0.4f }, { m3, 0.3f } };
          MM (v2, 0, srcLfo2, dstFmA, 0.5f);
          B ("WT Growl Bass", 5, v2); }
        B ("Ladder Acid", 1, { { engine, VA }, { wave, 0.0f }, { filterType, 1 }, { cutoff, 400 }, { reso, 0.8f }, { fenv, 0.6f }, { fdecay, 0.2f },
                              { sustain, 0.7f }, { glide, 0.1f }, { m1, 0.2f } });
        B ("Dark Acid", 2, { { engine, VA }, { wave, 0.5f }, { filterType, 2 }, { cutoff, 350 }, { reso, 0.7f }, { fenv, 0.5f }, { fdecay, 0.3f },
                              { sustain, 0.6f }, { glide, 0.12f }, { m1, 0.3f } });
        B ("Pluck Bass", 3, { { engine, PL }, { wave, 0.4f }, { decay, 1.0f }, { sustain, 0 }, { release, 0.2f }, { m1, 0.4f } });
        B ("Detroit Pluck Bass", 3, { { engine, VA }, { wave, 0.3f }, { cutoff, 700 }, { fenv, 0.6f }, { fdecay, 0.1f }, { decay, 0.3f }, { sustain, 0 },
                              { release, 0.15f }, { m1, 0.5f } });
        { Vals v2 { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.3f }, { cutoff, 800 }, { sustain, 1 }, { lfo2Rate, 0.3f }, { m1, 0.4f } };
          MM (v2, 0, srcLfo2, dstDetune, 0.4f); MM (v2, 1, srcLfo2, dstCutoff, 0.15f);
          B ("Moving Reese", 3, v2); }
        B ("Hyper Bass", 4, { { engine, WT }, { wave, 0.57f }, { unison, 3 }, { detune, 0.25f }, { cutoff, 4000 }, { sustain, 1 }, { drive, 0.4f },
                              { driveType, 2 }, { m1, 0.4f } });
        B ("Digicore Wobble", 4, { { engine, WT }, { wave, 0.43f }, { sustain, 1 }, { lfoSync, 1 }, { lfoDiv, 4 }, { wobTarget, 2 }, { m2, 0.7f }, { m1, 0.5f } });
        B ("Phonk Crush Bass", 1, { { engine, SB }, { wave, 0.9f }, { decay, 1.2f }, { sustain, 0.4f }, { crush, 0.4f }, { drive, 0.5f }, { driveType, 3 },
                              { m1, 0.2f }, { m3, 0.3f } });
        B ("Slow Growl Wobble", 3, { { engine, VX }, { wave, 0.2f }, { unison, 3 }, { detune, 0.2f }, { sustain, 1 }, { lfoSync, 1 }, { lfoDiv, 2 },
                              { m2, 0.6f }, { m3, 0.3f }, { m1, 0.5f } });
        B ("Bent Sub", 5, { { engine, SB }, { wave, 0.3f }, { sustain, 1 }, { tape, 1 }, { bend, 0.4f }, { m1, 0.2f } }, true);
        B ("Broken Bass", 5, { { engine, VA }, { wave, 0.3f }, { cutoff, 1500 }, { sustain, 1 }, { circuit, 0.5f }, { m1, 0.5f } }, true);

        // ---------------- more EXCLUSIVE ----------------
        P ("Ghost Orchestra", tPads, 0, true, "", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.3f }, { sustain, 0.9f },
            { release, 1.5f }, { ghost, 0.7f }, { ghostOct, 1 }, { revMix, 0.4f } });
        P ("Choir In A Bell", tChoir, 3, true, "", { { engine, VX }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.2f }, { sustain, 0.9f },
            { release, 1.5f }, { body, 2 }, { bodyMix, 0.6f }, { revMix, 0.35f } });
        P ("Wood Box Supersaw", tLeads, 4, true, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.45f }, { sustain, 0.8f },
            { body, 5 }, { bodyMix, 0.55f }, { revMix, 0.2f } });
        P ("Glass Flute Swap", tFlutes, 4, true, "", { { engine, FL }, { wave, 0.4f }, { sustain, 0.9f }, { lfoPitch, 0.1f }, { body, 3 }, { bodyMix, 0.5f },
            { revMix, 0.35f } });
        P ("Flute In A Metal Pipe", tFlutes, 3, true, "", { { engine, FL }, { wave, 0.5f }, { sustain, 0.9f }, { body, 4 }, { bodyMix, 0.6f }, { revMix, 0.3f } });
        P ("Circuit Organ", tKeys, 4, true, "Organs", { { engine, OR }, { wave, 0.6f }, { sustain, 1 }, { circuit, 0.5f }, { circRate, 2 }, { crush, 0.2f } });
        P ("Octave Jump Lead", tLeads, 5, true, "", { { engine, WT }, { wave, 0.3f }, { unison, 5 }, { detune, 0.3f }, { sustain, 0.9f }, { release, 0.4f },
            { bend, 0.7f }, { bendMode, 3 }, { bendSemis, -12 }, { drive, 0.3f }, { gain, -3 } });
        P ("Rise Bend Bells", tBells, 5, true, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.4f }, { decay, 2.0f }, { sustain, 0.1f }, { release, 1.0f },
            { bend, 0.5f }, { bendMode, 1 }, { bendSemis, 12 }, { revMix, 0.35f } });
        P ("Tape Choir", tChoir, 2, true, "", { { engine, VX }, { wave, 0.6f }, { unison, 5 }, { attack, 0.2f }, { sustain, 0.9f }, { release, 1.2f },
            { tape, 1 }, { bend, 0.3f }, { wow, 0.3f }, { revMix, 0.4f } });
        P ("Reverse Ghost Pad", tPads, 5, true, "", { { engine, WT }, { wave, 0.6f }, { unison, 5 }, { detune, 0.35f }, { attack, 0.6f }, { sustain, 1 },
            { release, 2.5f }, { ghost, 0.6f }, { reverse, 0.4f }, { revMix, 0.45f } });
        P ("Bent Kalimba", tExotic, 5, true, "Mallets", { { engine, MD }, { wave, 0.1f }, { fmAmt, 0.5f }, { decay, 1.2f }, { sustain, 0 },
            { bend, 0.5f }, { bendMode, 4 }, { tape, 1 }, { revMix, 0.3f } });
        P ("Haunted Music Box", tBells, 2, true, "", { { engine, MD }, { wave, 0.85f }, { octave, 1 }, { decay, 1.5f }, { sustain, 0 }, { ghost, 0.6f },
            { tape, 1 }, { wow, 0.3f }, { revMix, 0.4f } });
        P ("Aggressive Era Pad", tPads, 4, true, "", { { engine, VA }, { wave, 0.2f }, { unison, 7 }, { detune, 0.4f }, { attack, 0.2f }, { sustain, 0.9f },
            { release, 1.2f }, { morphX, 1.0f }, { morphY, 0.0f }, { gain, -4 } });
        P ("Classic Era Keys", tKeys, 0, true, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.8f }, { sustain, 0.1f },
            { morphX, 0.0f }, { morphY, 1.0f } });


        // ================= sub-categories: PIANO, ORGANS, STRINGS, BRASS, GUITARS, MALLETS, ARPS =================
        auto piano = [&] (const char* name, int era, Vals extra)
        {
            // FM hammer/tine body + string layer: bright attack that darkens, no sustain
            Vals v2 { { engine, FM }, { fmAlgo, 0 }, { fmRatio, 1.0f }, { fmAmt, 0.42f }, { wave, 0.22f }, { fdecay, 0.9f },
                      { attack, 0.001f }, { decay, 3.0f }, { sustain, 0.0f }, { release, 0.45f }, { velSens, 0.75f },
                      { layerB, 1 }, { engineB, PL }, { waveB, 0.72f }, { levelB, 0.45f }, { fineB, 4.0f },
                      { cutoff, 7000 }, { keyTrack, 0.6f }, { revType, 1 }, { revMix, 0.22f }, { chorus, 0.08f } };
            v2.insert (v2.end(), extra.begin(), extra.end());
            P (name, tKeys, era, false, "Piano", v2);
        };
        piano ("Bouncy Atlanta Piano", 0, { { decay, 1.4f }, { cutoff, 9000 } });
        piano ("Detuned Lo-Fi Piano", 1, { { fineB, 14.0f }, { wow, 0.35f }, { crush, 0.08f }, { cutoff, 4500 } });
        piano ("Trap Grand Piano", 2, {});
        piano ("Sad Piano Loop", 2, { { cutoff, 3800 }, { wow, 0.22f }, { revMix, 0.32f }, { revType, 0 } });
        piano ("Dark Trap Piano", 3, { { cutoff, 2600 }, { revMix, 0.3f }, { revType, 0 }, { m1, 0.42f } });
        piano ("Drill Slide Piano 2021", 3, { { mono, 1 }, { glide, 0.1f }, { legato, 1 } });
        piano ("Hyper Piano 2024", 4, { { drive, 0.28f }, { driveType, 1 }, { delayMix, 0.22f }, { cutoff, 9500 } });
        piano ("Ghost Piano 2026", 5, { { ghost, 0.45f }, { reverse, 0.15f }, { revMix, 0.35f } });

        P ("Dark Church Organ 2011", tKeys, 0, false, "Organs", { { engine, OR }, { wave, 0.4f }, { attack, 0.01f }, { sustain, 1 }, { release, 0.5f },
            { lfoPitch, 0.03f }, { revMix, 0.45f }, { revSize, 0.95f } });
        P ("Trap Organ Chords", tKeys, 2, false, "Organs", { { engine, OR }, { wave, 0.55f }, { sustain, 1 }, { release, 0.3f }, { chorus, 0.3f },
            { cutoff, 5000 }, { revMix, 0.25f } });
        P ("Rotary Organ 2016", tKeys, 2, false, "Organs", { { engine, OR }, { wave, 0.7f }, { sustain, 1 }, { release, 0.25f }, { chorus, 0.55f },
            { lfoAmp, 0.18f }, { lfoRate, 6.5f }, { revMix, 0.2f } });

        P ("Epic Trap Strings", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 7 }, { detune, 0.3f }, { attack, 0.15f },
            { sustain, 0.9f }, { release, 0.9f }, { lfoPitch, 0.04f }, { revMix, 0.4f }, { revSize, 0.9f } });
        P ("Staccato Strings 2012", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.25f }, { attack, 0.003f },
            { decay, 0.25f }, { sustain, 0 }, { release, 0.2f }, { revMix, 0.35f } });
        P ("Tremolo Strings", tPads, 1, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.1f },
            { sustain, 0.9f }, { release, 0.8f }, { lfoAmp, 0.7f }, { lfoSync, 1 }, { lfoDiv, 4 }, { revMix, 0.35f } });
        P ("Drill Violin Lead", tLeads, 3, false, "Strings", { { engine, VA }, { wave, 0.05f }, { unison, 2 }, { detune, 0.1f }, { cutoff, 4500 },
            { attack, 0.05f }, { sustain, 0.9f }, { release, 0.3f }, { mono, 1 }, { glide, 0.12f }, { lfoPitch, 0.12f }, { revMix, 0.3f } });
        P ("Dark Cello", tPads, 2, false, "Strings", { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.15f }, { octave, -1 }, { cutoff, 1500 },
            { attack, 0.1f }, { sustain, 0.9f }, { release, 0.6f }, { lfoPitch, 0.05f }, { revMix, 0.35f } });

        P ("Trap Horns 2011", tLeads, 0, false, "Brass", { { engine, OC }, { wave, 0.0f }, { unison, 5 }, { detune, 0.25f }, { attack, 0.01f },
            { decay, 0.6f }, { sustain, 0.6f }, { release, 0.3f }, { drive, 0.2f }, { driveType, 1 }, { revMix, 0.35f } });
        P ("Dark Brass Stab", tLeads, 1, false, "Brass", { { engine, OC }, { wave, 0.0f }, { unison, 3 }, { octave, -1 }, { decay, 0.4f },
            { sustain, 0.2f }, { release, 0.2f }, { revMix, 0.3f } });
        P ("Synth Brass 2019", tLeads, 3, false, "Brass", { { engine, VA }, { wave, 0.0f }, { unison, 3 }, { detune, 0.2f }, { cutoff, 800 },
            { fenv, 0.6f }, { fattack, 0.03f }, { fdecay, 0.35f }, { fsustain, 0.3f }, { sustain, 0.8f }, { release, 0.25f }, { revMix, 0.25f } });
        P ("Brass Swell Pad", tPads, 1, false, "Brass", { { engine, OC }, { wave, 0.1f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.45f },
            { sustain, 1 }, { release, 1.0f }, { revMix, 0.4f } });
        P ("Blown Horns 2025", tLeads, 5, false, "Brass", { { engine, OC }, { wave, 0.0f }, { unison, 5 }, { detune, 0.3f }, { decay, 0.5f },
            { sustain, 0.5f }, { drive, 0.55f }, { driveType, 3 }, { revMix, 0.2f } });

        P ("Acoustic Trap Guitar", tPlucks, 3, false, "Guitars", { { engine, PL }, { wave, 0.55f }, { decay, 2.5f }, { sustain, 0 }, { release, 0.5f },
            { body, 6 }, { bodyMix, 0.25f }, { chorus, 0.2f }, { revMix, 0.25f } });
        P ("Nylon Guitar 2017", tPlucks, 2, false, "Guitars", { { engine, PL }, { wave, 0.35f }, { decay, 2.0f }, { sustain, 0 }, { release, 0.4f },
            { body, 5 }, { bodyMix, 0.3f }, { revMix, 0.25f } });
        P ("Clean Electric 2020", tPlucks, 3, false, "Guitars", { { engine, PL }, { wave, 0.75f }, { decay, 3.0f }, { sustain, 0 }, { release, 0.5f },
            { chorus, 0.4f }, { delayMix, 0.2f }, { revMix, 0.2f } });
        P ("Dark Guitar Loop", tPlucks, 2, false, "Guitars", { { engine, PL }, { wave, 0.5f }, { decay, 2.5f }, { sustain, 0 }, { cutoff, 3000 },
            { wow, 0.3f }, { crush, 0.1f }, { revMix, 0.3f } });
        P ("Distorted Guitar 2023", tPlucks, 4, false, "Guitars", { { engine, PL }, { wave, 0.8f }, { decay, 4.0f }, { sustain, 0 }, { release, 0.4f },
            { drive, 0.5f }, { driveType, 1 }, { revMix, 0.2f } });

        P ("Vibraphone Night", tExotic, 2, false, "Mallets", { { engine, MD }, { wave, 0.5f }, { fmAmt, 0.2f }, { decay, 2.0f }, { sustain, 0 },
            { release, 1.0f }, { lfoAmp, 0.3f }, { lfoRate, 5.0f }, { revMix, 0.3f } });
        P ("Glockenspiel Ice", tBells, 1, false, "Mallets", { { engine, MD }, { wave, 0.85f }, { octave, 1 }, { fmAmt, 0.6f }, { decay, 1.5f },
            { sustain, 0 }, { release, 1.0f }, { revMix, 0.35f } });
        P ("Xylo Bounce", tExotic, 3, false, "Mallets", { { engine, MD }, { wave, 0.5f }, { fmAmt, 0.7f }, { decay, 0.35f }, { sustain, 0 },
            { release, 0.3f }, { revMix, 0.2f } });
        P ("Steel Drum 2019", tExotic, 3, false, "Mallets", { { engine, FM }, { fmRatio, 2.3f }, { fmAmt, 0.35f }, { fdecay, 0.3f }, { decay, 1.0f },
            { sustain, 0 }, { revMix, 0.25f } });

        auto arpP = [&] (const char* name, int tile, int era, Vals v2, int rate, int mode, int octs, float gate)
        {
            v2.push_back ({ arp, 1 }); v2.push_back ({ arpRate, (float) rate }); v2.push_back ({ arpMode, (float) mode });
            v2.push_back ({ arpOct, (float) octs }); v2.push_back ({ arpGate, gate });
            P (name, tile, era, false, "Arps", v2);
        };
        arpP ("Arp Bells 1-16", tBells, 1, { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.4f }, { decay, 0.8f }, { sustain, 0 }, { release, 0.4f },
              { delayMix, 0.25f }, { revMix, 0.3f } }, 1, 0, 2, 0.6f);
        arpP ("Rage Arp Saw", tLeads, 4, { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { cutoff, 3500 }, { fenv, 0.5f },
              { fdecay, 0.15f }, { decay, 0.25f }, { sustain, 0.1f }, { release, 0.15f }, { drive, 0.3f }, { delayMix, 0.2f } }, 1, 2, 2, 0.5f);
        arpP ("Plugg Arp", tPlucks, 3, { { engine, VA }, { wave, 0.5f }, { cutoff, 2500 }, { fenv, 0.4f }, { fdecay, 0.15f }, { decay, 0.3f },
              { sustain, 0.1f }, { revMix, 0.3f }, { delayMix, 0.2f } }, 0, 3, 1, 0.7f);
        arpP ("Trance Arp 2025", tLeads, 5, { { engine, VA }, { wave, 0.0f }, { unison, 5 }, { detune, 0.35f }, { cutoff, 2800 }, { fenv, 0.5f },
              { fdecay, 0.12f }, { decay, 0.2f }, { sustain, 0.1f }, { delayMix, 0.3f }, { revMix, 0.25f } }, 1, 0, 2, 0.45f);
        arpP ("Glassy Arp", tPlucks, 3, { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { wave, 0.3f }, { octave, 1 }, { decay, 0.6f },
              { sustain, 0.1f }, { chorus, 0.3f }, { delayMix, 0.25f } }, 2, 0, 1, 0.6f);
        arpP ("Dark Harp Arp", tPlucks, 2, { { engine, PL }, { wave, 0.4f }, { decay, 1.5f }, { sustain, 0 }, { revMix, 0.35f } }, 0, 0, 2, 0.8f);

        // ================= v0.4: 808 / TEXTURE / FX + era banks (TRAP 2010 -> FUTURE) =================
        auto B8 = [&] (const char* name, int era, Vals vals)
        {
            Vals base { { m1, 0.0f }, { m2, 0.0f }, { m3, 0.0f }, { m4, 0.0f }, { m5, 0.5f }, { m6, 0.0f },
                        { revMix, 0.0f }, { width, 0.4f }, { octave, -1 }, { engine, SB }, { sustain, 0 } };
            base.insert (base.end(), vals.begin(), vals.end());
            P (name, tBass, era, false, "808", base);
        };
        B8 ("808 Boom Long 2010", 0, { { wave, 0.3f }, { fmAmt, 0.1f }, { decay, 5.0f }, { release, 0.6f }, { m6, 0.25f } });
        B8 ("808 Tight Knock", 1, { { wave, 0.55f }, { fmAmt, 0.35f }, { decay, 0.7f }, { release, 0.15f }, { m3, 0.3f }, { m6, 0.8f }, { punch, 0.4f } });
        B8 ("808 Glide Legato", 2, { { wave, 0.45f }, { fmAmt, 0.15f }, { decay, 4.0f }, { sustain, 0.3f }, { release, 0.4f }, { glide, 0.25f }, { legato, 1 } });
        B8 ("808 Distorted Rage", 4, { { wave, 0.6f }, { fmAmt, 0.25f }, { decay, 2.5f }, { release, 0.3f }, { m3, 0.7f }, { driveType, 3 }, { m6, 0.5f } });
        B8 ("808 Drill Slide", 4, { { wave, 0.5f }, { fmAmt, 0.2f }, { decay, 3.5f }, { sustain, 0.25f }, { release, 0.35f }, { glide, 0.35f }, { m3, 0.35f } });
        B8 ("808 Folded Grit", 5, { { wave, 0.7f }, { fmAmt, 0.3f }, { decay, 2.0f }, { release, 0.3f }, { m3, 0.5f }, { driveType, 4 } });
        B8 ("808 Tape Warm", 2, { { wave, 0.35f }, { fmAmt, 0.1f }, { decay, 3.0f }, { release, 0.4f }, { m3, 0.25f }, { driveType, 1 }, { drift, 0.3f } });
        B8 ("808 Clean Sine Sub", 0, { { wave, 0.0f }, { decay, 6.0f }, { release, 0.5f } });

        auto TX = [&] (const char* name, int era, Vals vals) { P (name, tExperimental, era, false, "Texture", vals); };
        TX ("Sub-Harmonic Drone 2026", 5, { { engine, WT }, { wave, 0.2f }, { octave, -1 }, { unison, 5 }, { detune, 0.3f }, { attack, 2.0f }, { sustain, 1 },
            { release, 4.0f }, { sub, 0.5f }, { cutoff, 1800 }, { crush, 0.25f }, { wow, 0.35f }, { revMix, 0.45f }, { revType, 2 }, { drift, 0.5f }, { alive, 3 } });
        TX ("Granular String Cloud 2025", 5, { { engine, OC }, { wave, 0.4f }, { unison, 5 }, { detune, 0.35f }, { attack, 1.5f }, { sustain, 1 }, { release, 4.0f },
            { reverse, 0.35f }, { chorus, 0.4f }, { revMix, 0.5f }, { revType, 2 }, { revSize, 0.9f }, { alive, 4 }, { drift, 0.3f }, { timeM, 0.7f } });
        TX ("Cassette Hiss Pad", 3, { { engine, VA }, { wave, 0.5f }, { unison, 3 }, { detune, 0.3f }, { cutoff, 2200 }, { attack, 1.0f }, { sustain, 1 },
            { release, 3.0f }, { crush, 0.4f }, { wow, 0.5f }, { tape, 1 }, { revMix, 0.35f }, { drift, 0.6f } });
        TX ("Frozen Choir Air", 4, { { engine, VX }, { wave, 0.7f }, { unison, 5 }, { detune, 0.4f }, { attack, 1.2f }, { sustain, 1 }, { release, 4.0f },
            { revMix, 0.5f }, { revType, 2 }, { timeM, 0.95f }, { alive, 2 } });
        TX ("Metal Resonance Bed", 5, { { engine, WT }, { wave, 0.6f }, { unison, 3 }, { detune, 0.2f }, { attack, 0.6f }, { sustain, 0.7f }, { release, 3.0f },
            { body, 4 }, { bodyMix, 0.6f }, { phaser, 0.4f }, { revMix, 0.4f } });
        TX ("Half-Time Tape Keys", 3, { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.5f }, { sustain, 0.4f }, { release, 1.0f },
            { halftime, 1.0f }, { wow, 0.4f }, { crush, 0.2f }, { revMix, 0.3f } });

        auto FXP = [&] (const char* name, int era, Vals vals) { P (name, tExperimental, era, false, "FX", vals); };
        { Vals r { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.5f }, { cutoff, 300 }, { reso, 0.4f }, { attack, 3.0f }, { sustain, 1 },
                   { release, 2.0f }, { e3attack, 4.0f }, { e3sustain, 1 }, { revMix, 0.45f }, { delayMix, 0.25f } };
          MM (r, 0, srcEnv3, dstCutoff, 0.9f); MM (r, 1, srcEnv3, dstPitch, 0.35f); FXP ("Noise Riser 8 Bars", 4, r); }
        { Vals r { { engine, SB }, { octave, -2 }, { wave, 0.6f }, { fmAmt, 0.4f }, { decay, 2.5f }, { sustain, 0 }, { release, 1.5f }, { bend, 0.8f },
                   { bendMode, 0 }, { bendSemis, -24 }, { drive, 0.4f }, { revMix, 0.4f }, { revSize, 0.9f }, { punch, 0.7f } };
          FXP ("Cinematic Impact Hit", 5, r); }
        { Vals r { { engine, VA }, { wave, 0.2f }, { unison, 3 }, { detune, 0.3f }, { decay, 2.0f }, { sustain, 0 }, { release, 1.0f },
                   { bend, 1.0f }, { bendMode, 0 }, { bendSemis, -24 }, { revMix, 0.35f }, { delayMix, 0.3f } };
          FXP ("Tape Stop Downer", 3, r); }
        { Vals r { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.35f }, { attack, 2.0f }, { sustain, 1 }, { release, 0.2f }, { reverse, 0.6f },
                   { revMix, 0.55f }, { revType, 2 } };
          FXP ("Reverse Swell", 2, r); }
        { Vals r { { engine, MD }, { wave, 0.9f }, { fmAmt, 0.8f }, { decay, 0.6f }, { sustain, 0 }, { circuit, 0.6f }, { crush, 0.4f },
                   { delayMix, 0.35f }, { delayMode, 0 } };
          FXP ("Glitch Scatter FX", 5, r); }
        { Vals r { { engine, VA }, { wave, 0.5f }, { octave, 1 }, { cutoff, 4000 }, { reso, 0.6f }, { attack, 0.01f }, { decay, 0.8f }, { sustain, 0 },
                   { bend, 0.9f }, { bendMode, 1 }, { bendSemis, 12 }, { lfoPitch, 0.4f }, { lfoRate, 8.0f }, { delayMix, 0.4f } };
          FXP ("Laser Zap Up", 4, r); }

        // era banks from the TRAP-CORE taxonomy (neutral names)
        P ("Tutti Orchestral Stab 2011", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.5f }, { layerB, 1 }, { engineB, SB }, { octaveB, -1 },
            { levelB, 0.5f }, { attack, 0.002f }, { decay, 0.5f }, { sustain, 0 }, { release, 0.25f }, { punch, 0.6f }, { revMix, 0.15f } });
        P ("Harpsichord Stab 2012", tKeys, 1, false, "", { { engine, PL }, { wave, 0.8f }, { fmAmt, 0.2f }, { decay, 0.6f }, { sustain, 0 },
            { release, 0.2f }, { filterType, 6 }, { cutoff, 3000 }, { reso, 0.4f }, { punch, 0.35f } });
        P ("Gothic Dark Choir 2010", tChoir, 0, false, "", { { engine, VX }, { wave, 0.2f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.4f },
            { sustain, 1 }, { release, 1.5f }, { lfoPitch, 0.05f }, { lfoRate, 5.0f }, { crush, 0.15f }, { revMix, 0.35f } });
        P ("Detuned Music Box 2017", tBells, 2, false, "Mallets", { { engine, MD }, { wave, 0.9f }, { octave, 1 }, { decay, 1.8f }, { sustain, 0 },
            { release, 1.2f }, { wow, 0.45f }, { drift, 0.45f }, { alive, 3 }, { revMix, 0.4f } });
        P ("Ethnic Flute Glide 2016", tFlutes, 2, false, "", { { engine, FL }, { wave, 0.4f }, { attack, 0.08f }, { sustain, 0.9f }, { release, 1.0f },
            { glide, 0.2f }, { legato, 1 }, { mono, 1 }, { lfoPitch, 0.04f }, { revMix, 0.5f }, { revSize, 0.9f }, { alive, 2 } });
        P ("Reverse Rhodes 2018", tKeys, 3, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.25f }, { attack, 0.7f }, { sustain, 0.7f },
            { release, 0.3f }, { reverse, 0.5f }, { chorus, 0.3f }, { halftime, 0.4f }, { revMix, 0.35f } });
        P ("Cinematic Braam 2021", tLeads, 4, false, "Brass", { { engine, VA }, { wave, 0.0f }, { octave, -1 }, { unison, 5 }, { detune, 0.25f },
            { cutoff, 900 }, { fenv, 0.6f }, { fdecay, 1.2f }, { sustain, 1 }, { release, 1.5f }, { sub, 0.5f }, { drive, 0.5f }, { driveType, 3 }, { revMix, 0.35f } });
        P ("Distorted Flute 2022", tFlutes, 4, false, "", { { engine, FL }, { wave, 0.6f }, { sustain, 0.9f }, { release, 0.5f }, { drive, 0.55f },
            { driveType, 4 }, { crush, 0.3f }, { delayMix, 0.25f } });
        P ("Micro Chop Keys 2026", tKeys, 5, false, "", { { engine, VX }, { wave, 0.5f }, { decay, 0.25f }, { sustain, 0 }, { release, 0.1f },
            { circuit, 0.5f }, { circRate, 1 }, { halftime, 0.3f }, { alive, 4 } });
        P ("Staccato Pizz Strings 2013", tPlucks, 1, false, "Strings", { { engine, OC }, { wave, 0.3f }, { decay, 0.25f }, { sustain, 0 },
            { release, 0.08f }, { punch, 0.8f }, { width, 0.8f } });
        P ("Marcato Low Horns 2012", tLeads, 1, false, "Brass", { { engine, OC }, { wave, 0.8f }, { octave, -1 }, { attack, 0.01f }, { decay, 0.4f },
            { sustain, 0.3f }, { release, 0.2f }, { punch, 0.5f }, { drive, 0.2f } });

        // ================= v0.4.1: full era banks (TRAP-CORE taxonomy, original synthesized sounds) =================
        // Bank 1: 2010-2014 hard orchestral era
        P ("Timpani Hit Stab 2010", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.4f }, { layerB, 1 }, { engineB, MD }, { octaveB, -2 },
            { waveB, 0.2f }, { levelB, 0.8f }, { decay, 0.6f }, { sustain, 0 }, { release, 0.3f }, { punch, 0.7f }, { revMix, 0.2f } });
        P ("Hit Orchestra Low 2011", tPads, 0, false, "Strings", { { engine, OC }, { wave, 0.2f }, { octave, -1 }, { unison, 3 }, { detune, 0.15f },
            { layerB, 1 }, { engineB, OC }, { waveB, 0.6f }, { levelB, 0.7f }, { decay, 0.45f }, { sustain, 0 }, { release, 0.2f }, { punch, 0.6f }, { crush, 0.1f } });
        P ("Marcato Violins 2012", tPlucks, 1, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 3 }, { detune, 0.12f }, { decay, 0.35f },
            { sustain, 0.15f }, { release, 0.12f }, { punch, 0.5f }, { width, 0.75f }, { revMix, 0.12f } });
        P ("Pizzicato Arp Strings 2013", tPlucks, 1, false, "Strings", { { engine, PL }, { wave, 0.45f }, { decay, 0.3f }, { sustain, 0 }, { release, 0.08f },
            { body, 6 }, { bodyMix, 0.45f }, { punch, 0.6f }, { revMix, 0.1f } });
        P ("Low Trombones 2011", tLeads, 0, false, "Brass", { { engine, OC }, { wave, 0.1f }, { octave, -1 }, { unison, 3 }, { detune, 0.1f }, { attack, 0.02f },
            { decay, 0.6f }, { sustain, 0.5f }, { release, 0.25f }, { drive, 0.2f }, { revMix, 0.15f } });
        P ("Octave Horn Stab 2013", tLeads, 1, false, "Brass", { { engine, OC }, { wave, 0.15f }, { layerB, 1 }, { engineB, OC }, { waveB, 0.15f }, { octaveB, -1 },
            { levelB, 0.6f }, { decay, 0.3f }, { sustain, 0.1f }, { release, 0.15f }, { punch, 0.55f } });
        P ("Full Brass Section 2014", tLeads, 1, false, "Brass", { { engine, OC }, { wave, 0.25f }, { unison, 5 }, { detune, 0.2f }, { attack, 0.03f },
            { sustain, 0.8f }, { release, 0.3f }, { chorus, 0.2f }, { revMix, 0.25f } });
        P ("Aah Choir Vibrato 2010", tChoir, 0, false, "", { { engine, VX }, { wave, 0.0f }, { unison, 5 }, { detune, 0.25f }, { attack, 0.3f }, { sustain, 1 },
            { release, 1.2f }, { lfoPitch, 0.07f }, { lfoRate, 5.5f }, { crush, 0.12f }, { revMix, 0.35f } });
        P ("Ooh Choir 16-Bit 2012", tChoir, 1, false, "", { { engine, VX }, { wave, 0.95f }, { unison, 5 }, { detune, 0.2f }, { attack, 0.35f }, { sustain, 1 },
            { release, 1.3f }, { lfoPitch, 0.06f }, { lfoRate, 5.0f }, { crush, 0.2f }, { revMix, 0.3f } });
        P ("Dry Harpsichord Arp 2013", tKeys, 1, false, "", { { engine, PL }, { wave, 0.9f }, { decay, 0.8f }, { sustain, 0 }, { release, 0.15f },
            { layerB, 1 }, { engineB, PL }, { waveB, 0.9f }, { octaveB, 1 }, { levelB, 0.35f }, { filterType, 6 }, { cutoff, 2500 }, { reso, 0.3f } });
        P ("Bell Stab Cutter 2011", tBells, 0, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.55f }, { fdecay, 0.25f }, { decay, 0.4f },
            { sustain, 0 }, { release, 0.2f }, { punch, 0.5f }, { eqHigh, 3 } });
        // Bank 2: 2015-2019 melodic era
        P ("Toy Piano Wobble 2016", tKeys, 2, false, "Piano", { { engine, MD }, { wave, 0.7f }, { octave, 1 }, { decay, 0.9f }, { sustain, 0 }, { release, 0.5f },
            { wow, 0.5f }, { drift, 0.4f }, { alive, 3 }, { revMix, 0.3f } });
        P ("Celeste Detuned 2017", tBells, 2, false, "Mallets", { { engine, MD }, { wave, 0.8f }, { octave, 1 }, { layerB, 1 }, { engineB, MD }, { waveB, 0.8f },
            { octaveB, 1 }, { fineB, 18 }, { levelB, 0.6f }, { decay, 1.4f }, { sustain, 0 }, { release, 1.0f }, { wow, 0.3f }, { revMix, 0.4f } });
        P ("Antique Music Box 2015", tBells, 2, false, "Mallets", { { engine, MD }, { wave, 0.95f }, { octave, 2 }, { decay, 1.2f }, { sustain, 0 }, { release, 1.0f },
            { crush, 0.2f }, { wow, 0.45f }, { drift, 0.35f }, { revMix, 0.45f }, { revSize, 0.85f } });
        P ("Bamboo Flute Night 2016", tFlutes, 2, false, "", { { engine, FL }, { wave, 0.3f }, { attack, 0.1f }, { sustain, 0.85f }, { release, 1.2f },
            { lfoPitch, 0.05f }, { lfoRate, 4.5f }, { revMix, 0.55f }, { revType, 2 }, { alive, 2 } });
        P ("Breathy Pan Glide 2018", tFlutes, 3, false, "", { { engine, FL }, { wave, 0.6f }, { mono, 1 }, { legato, 1 }, { glide, 0.25f }, { attack, 0.06f },
            { sustain, 0.9f }, { release, 0.8f }, { delayMix, 0.3f }, { revMix, 0.45f }, { drift, 0.3f } });
        P ("Shrine Flute Echo 2019", tFlutes, 3, false, "", { { engine, FL }, { wave, 0.45f }, { octave, -1 }, { attack, 0.15f }, { sustain, 0.8f },
            { release, 1.5f }, { delayMix, 0.4f }, { delayMode, 2 }, { revMix, 0.5f }, { timeM, 0.6f } });
        P ("Half-Speed Rhodes 2017", tKeys, 2, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 2.0f }, { sustain, 0.3f },
            { release, 0.8f }, { halftime, 1.0f }, { chorus, 0.3f }, { wow, 0.3f }, { revMix, 0.3f } });
        P ("Reverse Keys Swell 2019", tKeys, 3, false, "Piano", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.25f }, { attack, 1.2f }, { sustain, 0.8f },
            { release, 0.15f }, { reverse, 0.6f }, { revMix, 0.4f } });
        P ("Clean Bell Pluck 2018", tBells, 3, false, "", { { engine, FM }, { fmRatio, 4.0f }, { fmAmt, 0.35f }, { fdecay, 0.6f }, { decay, 2.5f },
            { sustain, 0 }, { release, 2.0f }, { revMix, 0.35f }, { delayMix, 0.2f } });
        P ("Additive Glass Bell 2016", tBells, 2, false, "", { { engine, OR }, { wave, 0.8f }, { octave, 1 }, { decay, 2.0f }, { sustain, 0 },
            { release, 1.5f }, { body, 3 }, { bodyMix, 0.5f }, { revMix, 0.4f } });
        // Bank 3: 2020-2023 rage / drill / dark era
        P ("Hard Clip Supersaw 2021", tLeads, 4, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 8 }, { detune, 0.5f }, { cutoff, 6000 },
            { sustain, 1 }, { release, 0.2f }, { glide, 0.08f }, { drive, 0.6f }, { driveType, 2 }, { delayMix, 0.2f } });
        P ("Hyper Glide Lead 2022", tLeads, 4, false, "", { { engine, VA }, { wave, 0.0f }, { unison, 7 }, { detune, 0.4f }, { mono, 1 }, { legato, 1 },
            { glide, 0.2f }, { sustain, 1 }, { release, 0.15f }, { drive, 0.4f }, { driveType, 3 }, { revMix, 0.2f } });
        P ("Reese Horn Braam 2020", tLeads, 4, false, "Brass", { { engine, OC }, { wave, 0.1f }, { octave, -1 }, { layerB, 1 }, { engineB, VA }, { waveB, 0.0f },
            { unisonB, 3 }, { detuneB, 0.35f }, { octaveB, -2 }, { levelB, 0.6f }, { attack, 0.05f }, { sustain, 1 }, { release, 1.2f },
            { drive, 0.45f }, { driveType, 1 }, { revMix, 0.35f } });
        P ("Drill Staccato Strings 2021", tPlucks, 4, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 3 }, { detune, 0.2f }, { decay, 0.25f },
            { sustain, 0 }, { release, 0.1f }, { drift, 0.4f }, { driveType, 1 }, { drive, 0.25f }, { punch, 0.5f } });
        P ("Tape Warped Violins 2022", tPads, 4, false, "Strings", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.2f },
            { sustain, 1 }, { release, 1.0f }, { wow, 0.5f }, { drift, 0.6f }, { tape, 1 }, { revMix, 0.3f } });
        P ("Folded Bell Crush 2023", tBells, 4, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.4f }, { decay, 1.2f }, { sustain, 0 },
            { drive, 0.6f }, { driveType, 4 }, { crush, 0.35f }, { revMix, 0.2f } });
        P ("Bitcrushed Flute 2021", tFlutes, 4, false, "", { { engine, FL }, { wave, 0.5f }, { sustain, 0.85f }, { release, 0.4f }, { crush, 0.55f },
            { drive, 0.3f }, { delayMix, 0.25f } });
        P ("Dark Opera Pad 2022", tChoir, 4, false, "", { { engine, VX }, { wave, 0.3f }, { octave, -1 }, { unison, 6 }, { detune, 0.35f }, { attack, 0.6f },
            { sustain, 1 }, { release, 2.0f }, { drive, 0.3f }, { driveType, 1 }, { revMix, 0.45f }, { revType, 2 } });
        // Bank 4: 2024-2026 hybrid / granular era
        P ("Grain Choir Cloud 2025", tChoir, 5, false, "Texture", { { engine, VX }, { wave, 0.6f }, { unison, 7 }, { detune, 0.45f }, { attack, 1.0f },
            { sustain, 1 }, { release, 3.5f }, { reverse, 0.4f }, { alive, 4 }, { revMix, 0.5f }, { revType, 2 }, { timeM, 0.75f } });
        P ("Cello To Digital Morph 2026", tPads, 5, false, "Strings", { { engine, OC }, { wave, 0.5f }, { octave, -1 }, { layerB, 1 }, { engineB, WT },
            { waveB, 0.7f }, { warpModeB, 2 }, { levelB, 0.7f }, { attack, 0.3f }, { sustain, 1 }, { release, 1.5f }, { revMix, 0.3f },
            { mmSrc (0), (float) srcModWheel }, { mmDst (0), (float) dstLevelB }, { mmAmt (0), 0.8f } });
        P ("Hiss Sub Drone 2024", tExperimental, 5, false, "Texture", { { engine, SB }, { octave, -1 }, { wave, 0.3f }, { attack, 1.5f }, { sustain, 1 },
            { release, 3.0f }, { layerB, 1 }, { engineB, VA }, { waveB, 0.9f }, { octaveB, 2 }, { levelB, 0.25f }, { crush, 0.45f }, { wow, 0.4f }, { revMix, 0.4f } });
        P ("Formant Chop Piano 2025", tKeys, 5, false, "Piano", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 0.3f }, { sustain, 0 },
            { layerB, 1 }, { engineB, VX }, { waveB, 0.4f }, { levelB, 0.5f }, { circuit, 0.45f }, { circRate, 1 }, { alive, 3 } });
        P ("Vocal Slice Keys 2026", tKeys, 5, false, "", { { engine, VX }, { wave, 0.8f }, { decay, 0.18f }, { sustain, 0 }, { release, 0.08f },
            { circuit, 0.35f }, { circRate, 2 }, { halftime, 0.25f }, { delayMix, 0.2f } });
        P ("Supertrap Glass Keys 2026", tKeys, 5, false, "", { { engine, WT }, { wave, 0.35f }, { warpMode, 1 }, { decay, 1.0f }, { sustain, 0.2f },
            { release, 0.8f }, { body, 3 }, { bodyMix, 0.4f }, { alive, 3 }, { drift, 0.25f }, { revMix, 0.35f } });
        P ("Industrial Pipe Keys 2025", tKeys, 5, false, "", { { engine, MD }, { wave, 0.6f }, { decay, 0.9f }, { sustain, 0 }, { body, 4 }, { bodyMix, 0.6f },
            { drive, 0.4f }, { driveType, 2 }, { punch, 0.4f }, { revMix, 0.2f } });
        P ("Frozen Strings Hybrid 2026", tPads, 5, false, "Texture", { { engine, OC }, { wave, 0.5f }, { unison, 5 }, { detune, 0.3f }, { attack, 0.8f },
            { sustain, 1 }, { release, 4.0f }, { layerB, 1 }, { engineB, WT }, { waveB, 0.5f }, { levelB, 0.4f }, { timeM, 0.93f }, { alive, 3 } });

        // tag the older 808 bass patches for the 808 chip
        for (auto& pr : v) if (pr.tile == tBass && pr.name.contains ("808")) pr.sub = "808";

        // ================= variants: lo-fi, dark (old eras) / blown (new eras) =================
        const size_t numBase = v.size();
        for (size_t i = 0; i < numBase; ++i)
        {
            const Preset base = v[i];
            auto get = [] (const Vals& vals, const char* id, float def) { for (auto& [k, x] : vals) if (k == id) return x; return def; };
            auto set = [] (Vals& vals, const char* id, float x) { for (auto& [k, y] : vals) if (k == id) { y = x; return; } vals.push_back ({ id, x }); };
            const bool bassP = base.tile == tBass;

            Preset lf = base; lf.name << " Lo-Fi";
            set (lf.values, crush, std::min (1.0f, get (base.values, crush, 0) + (bassP ? 0.25f : 0.22f)));
            set (lf.values, wow, std::min (1.0f, get (base.values, wow, 0) + (bassP ? 0.12f : 0.35f)));
            if (! bassP) set (lf.values, m4, 0.3f);
            v.push_back (lf);

            const bool oldEra = base.era >= 0 && base.era <= 2;
            Preset second = base;
            if (oldEra || base.era < 0)
            {
                second.name << " Dark";
                set (second.values, cutoff, std::max (200.0f, get (base.values, cutoff, 12000.0f) * 0.4f));
                if (! bassP) { set (second.values, revMix, std::min (1.0f, get (base.values, revMix, 0.15f) + 0.15f)); set (second.values, revSize, 0.9f); }
            }
            else
            {
                second.name << " Blown";
                set (second.values, drive, bassP ? 0.5f : 0.6f);
                set (second.values, driveType, 3);
            }
            v.push_back (second);
        }

        // loudness table produced by tests/calibrate.py
        for (auto& pr : v)
            for (const auto& g : presetGains)
                if (pr.name == g.name)
                {
                    bool found = false;
                    for (auto& [k, x] : pr.values) if (k == ID::gain) { x = g.gain; found = true; }
                    if (! found) pr.values.push_back ({ ID::gain, g.gain });
                    break;
                }
        return v;
    }();
    return presets;
}
