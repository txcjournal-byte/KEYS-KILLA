// Offline render of every factory preset: checks for NaN/Inf, silence, DC and loudness spread.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cstdio>

static double maxWin = 0;
static bool renderPreset (KeysKillaProcessor& p, int idx, double sr, float& rmsDb, float& peak, float& dc, bool& finite,
                          bool chord = false, bool arp = false)
{
    p.setCurrentProgram (idx);
    if (chord) p.apvts.getParameter (ID::chord)->setValueNotifyingHost (1.0f);
    if (arp)   p.apvts.getParameter (ID::arp)->setValueNotifyingHost (1.0f);
    p.prepareToPlay (sr, 480);
    const bool bass = factoryPresets()[(size_t) idx].tile == tBass;
    const int root = bass ? 36 : 60;
    const int block = 480;
    juce::AudioBuffer<float> buf (2, block);
    double sumSq = 0, sum = 0; long count = 0; peak = 0; finite = true;
    double winSq = 0; long winN = 0; maxWin = 0;
    const int totalBlocks = (int) (sr * 3.0 / block);
    for (int b = 0; b < totalBlocks; ++b)
    {
        juce::MidiBuffer m;
        if (b == 0)
        {
            m.addEvent (juce::MidiMessage::noteOn (1, root, (juce::uint8) 100), 0);
            if (! bass) { m.addEvent (juce::MidiMessage::noteOn (1, root + 3, (juce::uint8) 100), 10); m.addEvent (juce::MidiMessage::noteOn (1, root + 7, (juce::uint8) 100), 20); }
        }
        if (b == (int) (sr * 1.5 / block))
            for (int n : { root, root + 3, root + 7 }) m.addEvent (juce::MidiMessage::noteOff (1, n), 5);
        p.processBlock (buf, m);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < block; ++i)
            {
                const float v = buf.getSample (ch, i);
                if (! std::isfinite (v)) finite = false;
                peak = std::max (peak, std::abs (v));
                if (b * block < sr * 1.5) { sumSq += (double) v * v; sum += v; ++count; }
                winSq += (double) v * v; if (++winN >= (long) (sr * 0.2)) { maxWin = std::max (maxWin, std::sqrt (winSq / (double) winN)); winSq = 0; winN = 0; }
            }
    }
    rmsDb = (float) juce::Decibels::gainToDecibels (std::sqrt (sumSq / std::max (1L, count)), -120.0);
    dc = (float) (sum / std::max (1L, count));
    if (chord) p.apvts.getParameter (ID::chord)->setValueNotifyingHost (0.0f);
    if (arp)   p.apvts.getParameter (ID::arp)->setValueNotifyingHost (0.0f);
    return finite && rmsDb > -50.0f && peak <= 1.0f && std::abs (dc) < 0.02f;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const bool verbose = argc > 1 && juce::String (argv[1]) == "-v";
    KeysKillaProcessor p;
    if (argc > 1 && juce::String (argv[1]) == "-cal")   // prints suggested output gain per preset (target -15 dB short-term RMS)
    {
        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            float rms, peak, dc; bool fin;
            renderPreset (p, i, 44100.0, rms, peak, dc, fin);
            const float g = p.apvts.getRawParameterValue (ID::gain)->load();
            const float win = (float) juce::Decibels::gainToDecibels (maxWin, -120.0);
            std::printf ("%s|%.1f|%.1f\n", p.getProgramName (i).toRawUTF8(), g, win);
        }
        return 0;
    }
    if (argc > 3 && juce::String (argv[1]) == "-shot")   // -shot <out.png> <skin 0|1> : GUI snapshot
    {
        juce::PropertiesFile::Options o; o.applicationName = "KEYS KILLA"; o.filenameSuffix = "settings"; o.folderName = "KEYS KILLA";
        juce::PropertiesFile (o).setValue ("skin", juce::String (argv[3]).getIntValue());
        p.setCurrentProgram (1);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    int failures = 0;
    std::vector<float> levels;
    for (double sr : { 44100.0, 96000.0 })
    {
        for (int i = 0; i < p.getNumPrograms(); ++i)
        {
            float rms, peak, dc; bool fin;
            const bool ok = renderPreset (p, i, sr, rms, peak, dc, fin);
            if (sr == 44100.0) levels.push_back (rms);
            if (! ok || verbose)
                std::printf ("%s %-26s sr=%6.0f rms=%6.1f dB peak=%.3f dc=%+.4f %s\n", ok ? "  " : "!!",
                             p.getProgramName (i).toRawUTF8(), sr, rms, peak, dc, fin ? "" : "NaN!");
            if (! ok) ++failures;
        }
    }
    // chord + arp + dice smoke tests
    {
        float rms, peak, dc; bool fin;
        if (! renderPreset (p, 0, 48000, rms, peak, dc, fin, true, false)) { std::printf ("!! chord mode failed (rms %.1f)\n", rms); ++failures; }
        if (! renderPreset (p, 3, 48000, rms, peak, dc, fin, false, true)) { std::printf ("!! arp mode failed (rms %.1f)\n", rms); ++failures; }
        for (int d = 0; d < 30; ++d)
        {
            p.setCurrentProgram (d % p.getNumPrograms());
            p.apvts.getParameter (ID::chaos)->setValueNotifyingHost ((float) (d % 10) / 9.0f);
            p.rollDice (factoryPresets()[(size_t) (d % p.getNumPrograms())].tile);
            juce::AudioBuffer<float> buf (2, 512); p.prepareToPlay (48000, 512);
            bool f = true; float pk = 0;
            for (int b = 0; b < 100; ++b)
            {
                juce::MidiBuffer m; if (b == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 110), 0);
                p.processBlock (buf, m);
                for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 512; ++i) { f &= std::isfinite (buf.getSample (ch, i)); pk = std::max (pk, std::abs (buf.getSample (ch, i))); }
            }
            if (! f || pk > 1.0f) { std::printf ("!! dice roll %d produced bad output\n", d); ++failures; }
        }
    }
    std::printf ("render time %.1f s for %.0f s of audio\n", (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0, p.getNumPrograms() * 6.0 + 60);
    std::vector<float> wins; // short-term loudness spread
    auto [mn, mx] = std::minmax_element (levels.begin(), levels.end());
    std::printf ("%d presets, RMS range %.1f .. %.1f dB, failures: %d\n", p.getNumPrograms(), *mn, *mx, failures);
    return failures == 0 ? 0 : 1;
}
