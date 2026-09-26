#include "Presets.h"
#include "Params.h"

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
        auto P = [&v] (const char* name, int tile, int era, bool excl, const char* sub,
                       std::vector<std::pair<const char*, float>> vals)
        { v.push_back ({ name, tile, era, excl, sub, std::move (vals) }); };

        const float FM = engFM, VA = engVA, PL = engPluck, VX = engVox, OR = engOrgan, FL = engFlute, SB = engSub;

        // ---------------- BELLS ----------------
        P ("Southside Bells 2010", tBells, 0, false, "", { { engine, FM }, { fmRatio, 3.5f }, { fmAmt, 0.45f }, { fdecay, 1.0f },
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
        P ("Bounce Piano", tKeys, 0, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.35f }, { wave, 0.2f }, { fdecay, 0.6f },
            { decay, 1.8f }, { sustain, 0.05f }, { release, 0.4f }, { cutoff, 9000 }, { fenv, 0.2f }, { revMix, 0.15f }, { gain, -5.3f } });
        P ("Church Organ Bounce", tKeys, 0, false, "Organs", { { engine, OR }, { wave, 0.55f }, { attack, 0.01f }, { sustain, 1.0f }, { release, 0.3f },
            { lfoPitch, 0.04f }, { lfoRate, 6.0f }, { revMix, 0.35f }, { revSize, 0.9f }, { gain, -8.9f } });
        P ("Dark Minimal Keys", tKeys, 1, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.2f }, { decay, 2.5f }, { sustain, 0.2f },
            { release, 0.8f }, { cutoff, 3500 }, { revMix, 0.3f }, { m1, 0.4f }, { m2, 0.35f,  }, { gain, -6.4f } });
        P ("Dark Piano Loop", tKeys, 2, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.4f }, { wave, 0.35f }, { fdecay, 0.4f },
            { decay, 2.0f }, { sustain, 0.0f }, { release, 0.6f }, { cutoff, 6000 }, { wow, 0.25f }, { crush, 0.1f }, { revMix, 0.25f }, { m1, 0.4f,  }, { gain, -4.7f } });
        P ("Detroit Keys", tKeys, 3, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.45f }, { fdecay, 0.3f }, { decay, 0.9f },
            { sustain, 0.1f }, { release, 0.3f }, { drive, 0.2f }, { driveType, 1 }, { revMix, 0.12f,  }, { gain, -9.8f } });
        P ("Pluggnb Keys", tKeys, 3, false, "", { { engine, FM }, { fmRatio, 2.0f }, { fmAmt, 0.25f }, { unison, 3 }, { detune, 0.12f },
            { decay, 1.6f }, { sustain, 0.3f }, { release, 0.9f }, { chorus, 0.5f }, { revMix, 0.35f }, { m5, 0.3f }, { m6, 0.7f,  }, { gain, -6.3f } });
        P ("UK Drill Slide Piano", tKeys, 3, false, "", { { engine, FM }, { fmRatio, 1.0f }, { fmAmt, 0.3f }, { decay, 1.8f }, { sustain, 0.1f },
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
        auto B = [&] (const char* name, int era, std::vector<std::pair<const char*, float>> vals)
        {
            std::vector<std::pair<const char*, float>> base { { m1, 0.0f }, { m2, 0.0f }, { m3, 0.0f }, { m4, 0.0f }, { m5, 0.5f }, { m6, 0.0f },
                                                              { revMix, 0.0f }, { width, 0.4f }, { octave, -1 } };
            base.insert (base.end(), vals.begin(), vals.end());
            P (name, tBass, era, false, "", base);
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
        return v;
    }();
    return presets;
}
