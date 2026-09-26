#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Parameter IDs are stable - never rename after 1.0.
namespace ID
{
    // engine
    inline constexpr const char* engine   = "engine";
    inline constexpr const char* octave   = "octave";
    inline constexpr const char* wave     = "wave";
    inline constexpr const char* unison   = "unison";
    inline constexpr const char* detune   = "detune";
    inline constexpr const char* sub      = "sub";
    inline constexpr const char* fmRatio  = "fmRatio";
    inline constexpr const char* fmAmt    = "fmAmt";
    // filter
    inline constexpr const char* cutoff   = "cutoff";
    inline constexpr const char* reso     = "reso";
    inline constexpr const char* fenv     = "fenv";
    inline constexpr const char* fdecay   = "fdecay";
    // amp
    inline constexpr const char* attack   = "attack";
    inline constexpr const char* decay    = "decay";
    inline constexpr const char* sustain  = "sustain";
    inline constexpr const char* release  = "release";
    inline constexpr const char* velSens  = "velSens";
    // lfo
    inline constexpr const char* lfoRate  = "lfoRate";
    inline constexpr const char* lfoSync  = "lfoSync";
    inline constexpr const char* lfoDiv   = "lfoDiv";
    inline constexpr const char* lfoPitch = "lfoPitch";
    inline constexpr const char* lfoFilter= "lfoFilter";
    inline constexpr const char* lfoAmp   = "lfoAmp";
    // play
    inline constexpr const char* mono     = "mono";
    inline constexpr const char* glide    = "glide";
    inline constexpr const char* bassMode = "bassMode";
    inline constexpr const char* chord    = "chord";
    inline constexpr const char* arp      = "arp";
    inline constexpr const char* arpRate  = "arpRate";
    // fx
    inline constexpr const char* drive    = "drive";
    inline constexpr const char* driveType= "driveType";
    inline constexpr const char* crush    = "crush";
    inline constexpr const char* wow      = "wow";
    inline constexpr const char* chorus   = "chorus";
    inline constexpr const char* delayMix = "delayMix";
    inline constexpr const char* delayTime= "delayTime";
    inline constexpr const char* delayFb  = "delayFb";
    inline constexpr const char* revMix   = "revMix";
    inline constexpr const char* revSize  = "revSize";
    inline constexpr const char* width    = "width";
    inline constexpr const char* gain     = "gain";
    // macros
    inline constexpr const char* m1 = "macro1";
    inline constexpr const char* m2 = "macro2";
    inline constexpr const char* m3 = "macro3";
    inline constexpr const char* m4 = "macro4";
    inline constexpr const char* m5 = "macro5";
    inline constexpr const char* m6 = "macro6";
    // exclusive
    inline constexpr const char* ghost    = "ghost";
    inline constexpr const char* bend     = "bend";
    inline constexpr const char* circuit  = "circuit";
    inline constexpr const char* chaos    = "chaos";
    inline constexpr const char* morphX   = "morphX";
    inline constexpr const char* morphY   = "morphY";
    inline constexpr const char* body     = "body";
    inline constexpr const char* bodyMix  = "bodyMix";
    inline constexpr const char* seed     = "seed";
}

namespace Choices
{
    inline const juce::StringArray engines   { "VA", "FM", "Pluck", "Vox", "Organ", "Flute", "Sub 808" };
    inline const juce::StringArray lfoDivs   { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/4T", "1/8T", "1/16T" };
    inline const juce::StringArray arpRates  { "1/8", "1/16", "1/16T", "1/32" };
    inline const juce::StringArray drives    { "Soft", "Tape", "Hard", "Blown", "Fold" };
    inline const juce::StringArray delays    { "1/4", "1/8", "1/8D", "1/16", "1/4T" };
    inline const juce::StringArray bodies    { "Off", "Kalimba", "Bell", "Glass", "Metal Pipe", "Wood Box", "String" };

    // length in quarter notes
    inline double lfoDivBeats (int i)
    {
        static const double v[] { 4.0, 2.0, 1.0, 0.5, 0.25, 0.125, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
        return v[juce::jlimit (0, 8, i)];
    }
    inline double arpBeats (int i)
    {
        static const double v[] { 0.5, 0.25, 1.0 / 6.0, 0.125 };
        return v[juce::jlimit (0, 3, i)];
    }
    inline double delayBeats (int i)
    {
        static const double v[] { 1.0, 0.5, 0.75, 0.25, 2.0 / 3.0 };
        return v[juce::jlimit (0, 4, i)];
    }
}

enum Engine { engVA, engFM, engPluck, engVox, engOrgan, engFlute, engSub };

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout l;
    auto f = [&] (const char* id, const char* name, float lo, float hi, float def, float skewCentre = -1.0f)
    {
        NormalisableRange<float> r (lo, hi);
        if (skewCentre > 0) r.setSkewForCentre (skewCentre);
        l.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def));
    };
    auto c = [&] (const char* id, const char* name, const StringArray& ch, int def)
    { l.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, ch, def)); };
    auto b = [&] (const char* id, const char* name, bool def)
    { l.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto i = [&] (const char* id, const char* name, int lo, int hi, int def)
    { l.add (std::make_unique<AudioParameterInt> (ParameterID { id, 1 }, name, lo, hi, def)); };

    c (ID::engine, "Engine", Choices::engines, 0);
    i (ID::octave, "Octave", -3, 2, 0);
    f (ID::wave, "Wave", 0, 1, 0);
    i (ID::unison, "Unison", 1, 7, 1);
    f (ID::detune, "Detune", 0, 1, 0.2f);
    f (ID::sub, "Sub", 0, 1, 0);
    f (ID::fmRatio, "FM Ratio", 0.5f, 12.0f, 2.0f, 3.0f);
    f (ID::fmAmt, "FM Amount", 0, 1, 0.3f);

    f (ID::cutoff, "Cutoff", 40.0f, 20000.0f, 12000.0f, 1500.0f);
    f (ID::reso, "Resonance", 0, 1, 0.1f);
    f (ID::fenv, "Filter Env", -1, 1, 0);
    f (ID::fdecay, "Filter Decay", 0.005f, 4.0f, 0.4f, 0.4f);

    f (ID::attack, "Attack", 0.001f, 5.0f, 0.002f, 0.3f);
    f (ID::decay, "Decay", 0.005f, 8.0f, 0.6f, 0.6f);
    f (ID::sustain, "Sustain", 0, 1, 0.8f);
    f (ID::release, "Release", 0.005f, 8.0f, 0.3f, 0.6f);
    f (ID::velSens, "Velocity", 0, 1, 0.6f);

    f (ID::lfoRate, "LFO Rate", 0.05f, 20.0f, 5.0f, 3.0f);
    b (ID::lfoSync, "LFO Sync", false);
    c (ID::lfoDiv, "LFO Division", Choices::lfoDivs, 3);
    f (ID::lfoPitch, "LFO > Pitch", 0, 1, 0);
    f (ID::lfoFilter, "LFO > Filter", 0, 1, 0);
    f (ID::lfoAmp, "LFO > Amp", 0, 1, 0);

    b (ID::mono, "Mono", false);
    f (ID::glide, "Glide", 0, 1, 0, 0.15f);
    b (ID::bassMode, "Bass Mode", false);
    b (ID::chord, "Chord", false);
    b (ID::arp, "Arp", false);
    c (ID::arpRate, "Arp Rate", Choices::arpRates, 1);

    f (ID::drive, "Drive", 0, 1, 0);
    c (ID::driveType, "Drive Type", Choices::drives, 0);
    f (ID::crush, "Crush", 0, 1, 0);
    f (ID::wow, "Wow", 0, 1, 0);
    f (ID::chorus, "Chorus", 0, 1, 0);
    f (ID::delayMix, "Delay Mix", 0, 1, 0);
    c (ID::delayTime, "Delay Time", Choices::delays, 2);
    f (ID::delayFb, "Delay Feedback", 0, 0.95f, 0.35f);
    f (ID::revMix, "Reverb Mix", 0, 1, 0.15f);
    f (ID::revSize, "Reverb Size", 0, 1, 0.6f);
    f (ID::width, "Width", 0, 1, 0.5f);
    f (ID::gain, "Output", -24.0f, 12.0f, 0.0f);

    f (ID::m1, "Macro 1", 0, 1, 0.5f);
    f (ID::m2, "Macro 2", 0, 1, 0.25f);
    f (ID::m3, "Macro 3", 0, 1, 0.0f);
    f (ID::m4, "Macro 4", 0, 1, 0.0f);
    f (ID::m5, "Macro 5", 0, 1, 0.2f);
    f (ID::m6, "Macro 6", 0, 1, 0.5f);

    f (ID::ghost, "Ghost", 0, 1, 0);
    f (ID::bend, "Bend", 0, 1, 0);
    f (ID::circuit, "Circuit", 0, 1, 0);
    f (ID::chaos, "Chaos", 0, 1, 0.4f);
    f (ID::morphX, "Era Morph X", 0, 1, 0.5f);
    f (ID::morphY, "Era Morph Y", 0, 1, 0.5f);
    c (ID::body, "Body Swap", Choices::bodies, 0);
    f (ID::bodyMix, "Body Mix", 0, 1, 0.5f);
    i (ID::seed, "Seed", 0, 9999, 1234);
    return l;
}
