#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include "Skin.h"
#include "Params.h"

using namespace juce;

inline Font serif (float h, bool bold = false, float kern = 0.12f)
{
    return Font (FontOptions (Font::getDefaultSerifFontName(), h, bold ? Font::bold : Font::plain)).withExtraKerningFactor (kern);
}

inline std::unique_ptr<PropertiesFile> openSettings()
{
    PropertiesFile::Options o;
    o.applicationName = "KEYS KILLA"; o.filenameSuffix = "settings"; o.folderName = "KEYS KILLA";
    o.osxLibrarySubFolder = "Application Support";
    return std::make_unique<PropertiesFile> (o);
}

inline void drawStar (Graphics& g, Point<float> c, float r, Colour col, float thin = 0.18f)
{
    Path p;
    for (int i = 0; i < 8; ++i)
    {
        const float a = MathConstants<float>::pi * 0.25f * (float) i;
        const float rr = (i % 2 == 0) ? r : r * thin;
        const Point<float> pt (c.x + std::sin (a) * rr, c.y - std::cos (a) * rr);
        if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
    }
    p.closeSubPath();
    g.setColour (col.withMultipliedAlpha (0.35f));
    g.fillEllipse (Rectangle<float> (r * 0.8f, r * 0.8f).withCentre (c));
    g.setColour (col);
    g.fillPath (p);
}

inline void drawPanel (Graphics& g, Rectangle<float> r, const Skin& s, const String& title = {})
{
    g.setColour (s.panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (s.dark ? Colours::black.withAlpha (0.6f) : Colours::white.withAlpha (0.8f));
    g.drawRoundedRectangle (r.reduced (1.5f), 7.0f, 1.0f);
    g.setColour (s.panelEdge);
    g.drawRoundedRectangle (r, 8.0f, 1.4f);
    if (title.isNotEmpty())
    {
        drawStar (g, { r.getX() + 22, r.getY() + 20 }, 9.0f, s.dark ? s.text : s.textDim);
        g.setColour (s.text);
        g.setFont (serif (15.0f, false, 0.3f));
        g.drawText (title, Rectangle<float> (r.getX() + 40, r.getY() + 8, 300, 24), Justification::centredLeft);
    }
}

// One-sentence help for every parameter (shown as tooltip)
inline String paramTooltip (const String& id)
{
    static const std::map<String, String> t {
        { "engine", "Sound engine of layer A." }, { "engineB", "Sound engine of layer B." }, { "layerB", "Switch the second layer on." },
        { "octave", "Octave shift of layer A." }, { "octaveB", "Octave shift of layer B." }, { "semi", "Semitone shift of layer A." }, { "semiB", "Semitone shift of layer B." },
        { "fine", "Fine tuning of layer A in cents." }, { "fineB", "Fine tuning of layer B in cents." },
        { "wave", "Timbre of layer A: waveform, wavetable position, vowel, brightness or body type." }, { "waveB", "Timbre of layer B." },
        { "unison", "Number of stacked, detuned voices in layer A." }, { "unisonB", "Number of stacked voices in layer B." },
        { "detune", "How far the unison voices of layer A spread." }, { "detuneB", "How far the unison voices of layer B spread." },
        { "fmRatio", "Frequency ratio of the first FM modulator (also modal decay colour)." }, { "fmRatioB", "FM ratio of layer B." },
        { "fmRatio2", "Ratio of the second FM operator pair." }, { "fmRatio2B", "Second FM ratio of layer B." },
        { "fmAmt", "FM depth, wavetable warp amount, pitch drop or brightness depending on the engine." }, { "fmAmtB", "FM / warp amount of layer B." },
        { "fmAlgo", "How the four FM operators are connected." }, { "fmAlgoB", "FM algorithm of layer B." },
        { "warpMode", "How the wavetable phase is bent." }, { "warpModeB", "Wavetable warp mode of layer B." },
        { "levelA", "Volume of layer A." }, { "levelB", "Volume of layer B." }, { "sub", "Clean sine one octave below." },
        { "filterType", "Clean, ladder, dirty, high-pass or band-pass filter." }, { "cutoff", "Filter frequency." }, { "reso", "Filter resonance." },
        { "keyTrack", "How much the filter follows the played note." }, { "fenv", "How much envelope 2 opens or closes the filter." },
        { "fattack", "Envelope 2 attack." }, { "fdecay", "Envelope 2 decay (also FM brightness decay)." }, { "fsustain", "Envelope 2 sustain level." }, { "frelease", "Envelope 2 release." },
        { "attack", "Volume fade-in time." }, { "decay", "Time to fall to the sustain level." }, { "sustain", "Level while the key is held." }, { "release", "Fade-out after the key is released." },
        { "velSens", "How much velocity changes volume." },
        { "e3attack", "Envelope 3 attack (mod matrix only)." }, { "e3decay", "Envelope 3 decay." }, { "e3sustain", "Envelope 3 sustain." }, { "e3release", "Envelope 3 release." },
        { "lfoRate", "LFO 1 speed in Hz." }, { "lfoSync", "Lock LFO 1 to the song tempo." }, { "lfoDiv", "LFO 1 note length when synced." }, { "lfoShape", "LFO 1 waveform." },
        { "lfoPitch", "LFO 1 vibrato depth." }, { "lfoFilter", "LFO 1 filter movement." }, { "lfoAmp", "LFO 1 tremolo depth." },
        { "lfo2Rate", "LFO 2 speed in Hz." }, { "lfo2Sync", "Lock LFO 2 to the song tempo." }, { "lfo2Div", "LFO 2 note length when synced." }, { "lfo2Shape", "LFO 2 waveform." },
        { "wobTarget", "What the bass WOBBLE macro moves: filter, volume, wave or pitch." },
        { "mono", "Play one note at a time." }, { "legato", "In mono, overlapping notes slide without restarting the envelope." },
        { "glide", "Slide time between notes." }, { "bendRange", "Pitch wheel range in semitones." }, { "bassMode", "Mono low end, sub layer and clean-low distortion." },
        { "keyLock", "Force every note into the chosen key and scale." }, { "key", "Root note for Key Lock and scale chords." }, { "scale", "Scale for Key Lock." },
        { "chord", "One key plays a whole chord." }, { "chordType", "Chord shape played by CHORD." }, { "strum", "Delay between chord notes like a strummed guitar." },
        { "arp", "Arpeggiator synced to the song tempo." }, { "arpRate", "Arpeggiator note length." }, { "arpMode", "Order in which held notes are played." },
        { "arpOct", "Octave range of the arpeggio." }, { "arpSwing", "Delays every second arp note for a swung groove." }, { "arpGate", "Length of each arp note." },
        { "drive", "Distortion amount." }, { "driveType", "Distortion flavour, from soft tape to blown-out." }, { "crush", "Bit and sample-rate reduction." },
        { "wow", "Tape wobble, flutter and vinyl crackle." }, { "chorus", "Stereo chorus." }, { "phaser", "Sweeping phaser." }, { "flanger", "Jet flanger." },
        { "delayMix", "Echo level." }, { "delayTime", "Echo time in note values." }, { "delayFb", "Number of echo repeats." }, { "delayMode", "Ping-pong, stereo or tape echo." },
        { "revMix", "Reverb level." }, { "revSize", "Reverb size." }, { "revType", "Hall, plate or huge cloud reverb." }, { "freeze", "Hold the reverb tail forever." },
        { "eqLow", "Bass boost or cut." }, { "eqHigh", "Treble boost or cut." }, { "reverse", "Plays the sound through reversed slices." },
        { "width", "Stereo width." }, { "gain", "Output volume." },
        { "ghost", "Adds a reversed, octave-shifted shadow layer." }, { "ghostOct", "Pitch of the ghost layer." }, { "ghostRev", "Play the ghost layer backwards." },
        { "ghostBlur", "How washed-out the ghost layer is." }, { "bend", "Amount of melody bends." }, { "bendMode", "Dive, rise, dip, octave jump or random tuning." },
        { "bendSemis", "Size of the end-of-note bend in semitones." }, { "tape", "Unstable, sliding pitch like a stretched tape." },
        { "circuit", "Tempo-synced broken-electronics glitches." }, { "circRate", "How often CIRCUIT can glitch." },
        { "chaos", "How far DICE moves away from the current sound." }, { "morphX", "ERA MORPH horizontal position." }, { "morphY", "ERA MORPH vertical position." },
        { "body", "Plays the sound through the resonant body of another instrument." }, { "bodyMix", "How much of the swapped body you hear." },
        { "seed", "Random seed for DICE and CIRCUIT, stored with the preset." } };
    if (id.startsWith ("mmSrc")) return "Mod matrix source.";
    if (id.startsWith ("mmDst")) return "Mod matrix destination.";
    if (id.startsWith ("mmAmt")) return "Mod matrix amount (negative inverts).";
    auto it = t.find (id);
    return it != t.end() ? it->second : String();
}
