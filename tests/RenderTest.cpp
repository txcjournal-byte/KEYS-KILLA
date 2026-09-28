#include <map>
// Offline render of every factory preset: checks for NaN/Inf, silence, DC and loudness spread.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cstdio>
#include <set>

static double maxWin = 0;
static bool renderPreset (KeysKillaProcessor& p, int idx, double sr, float& rmsDb, float& peak, float& dc, bool& finite,
                          bool chord = false, bool arp = false)
{
    p.setCurrentProgram (idx);
    if (chord) p.apvts.getParameter (ID::chord)->setValueNotifyingHost (1.0f);
    if (arp)   p.apvts.getParameter (ID::arp)->setValueNotifyingHost (1.0f);
    p.prepareToPlay (sr, 480);
    const bool bass = factoryPresets()[(size_t) idx].isBass();
    const int root = bass ? 36 : 60;
    const int block = 480;
    juce::AudioBuffer<float> buf (2, block);
    double sumSq = 0, sum = 0; long count = 0, dcCount = 0; peak = 0; finite = true;
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
                if (b * block < sr * 1.5) { sumSq += (double) v * v; ++count; }
                if (b * block >= sr * 1.0 && b * block < sr * 1.5) { sum += v; ++dcCount; }   // DC on the settled part only
                winSq += (double) v * v; if (++winN >= (long) (sr * 0.2)) { maxWin = std::max (maxWin, std::sqrt (winSq / (double) winN)); winSq = 0; winN = 0; }
            }
    }
    rmsDb = (float) juce::Decibels::gainToDecibels (std::sqrt (sumSq / std::max (1L, count)), -120.0);
    dc = (float) (sum / std::max (1L, dcCount));
    if (chord) p.apvts.getParameter (ID::chord)->setValueNotifyingHost (0.0f);
    if (arp)   p.apvts.getParameter (ID::arp)->setValueNotifyingHost (0.0f);
    // true DC is removed by the output DC blocker; the mean can still move with sub-audio swells of slow pads
    return finite && rmsDb > -50.0f && peak <= 1.0f && std::abs (dc) < 0.03f;
}


// ---------------------------------------------------------------- DSP unit tests
static int unitTests()
{
    int fails = 0;
    auto check = [&] (bool ok, const char* what) { if (! ok) { std::printf ("!! unit: %s\n", what); ++fails; } };
    auto set = [] (KeysKillaProcessor& p, const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
    auto countNotes = [] (KeysKillaProcessor& p) { int c = 0; for (auto& b : p.playing) c += b.load() ? 1 : 0; return c; };

    // wavetable: frame 0 is a sine
    {
        const auto& wt = kk::WavetableBank::get();
        float err = 0;
        for (int i = 0; i < 64; ++i) { const float ph = (float) i / 64.0f; err = std::max (err, std::abs (wt.read (ph, 0.0f, 0.001f) - std::sin (kk::twoPi * ph))); }
        check (err < 0.02f, "wavetable sine frame");
    }
    // BREED LAB: six playable children, deterministic genes, gene switch + lock, state round trip
    {
        KeysKillaProcessor p; p.prepareToPlay (48000, 256);
        const auto& ps = factoryPresets();
        juce::AudioBuffer<float> b (2, 256);
        bool finite = true; float worst = 0;
        for (int round = 0; round < 12; ++round)
        {
            p.setParentPreset (0, (round * 37) % (int) ps.size()); p.setParentPreset (1, (round * 91 + 11) % (int) ps.size());
            check (p.breed() == 6, "breed makes 6 children");
            for (int c = 0; c < 6; ++c)
            {
                p.previewChild (c);
                for (int k = 0; k < 60; ++k)
                {
                    juce::MidiBuffer m; p.processBlock (b, m);
                    for (int ch = 0; ch < 2; ++ch) for (int n = 0; n < 256; ++n) { const float x = b.getSample (ch, n); finite &= std::isfinite (x); worst = std::max (worst, std::abs (x)); }
                }
            }
        }
        check (finite && worst <= 1.01f, "children finite and bounded");
        const auto before = p.kids()[2].g.v;
        p.setChildGene (2, KeysKillaProcessor::geneSpace, 1 - p.kids()[2].genes[KeysKillaProcessor::geneSpace]);
        check (p.kids()[2].g.v != before, "gene switch changes the child");
        p.toggleGeneLock (KeysKillaProcessor::geneBody);
        const int src = p.kids()[p.selectedChild()].genes[KeysKillaProcessor::geneBody];
        p.breed();
        bool held = true; for (auto& c : p.kids()) held &= c.genes[KeysKillaProcessor::geneBody] == src;
        check (held, "locked gene is inherited by every child");
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (q.kids().size() == 6 && q.kids()[3].g.name == p.kids()[3].g.name && q.parent (1).name == p.parent (1).name, "breed lab state round trip");
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        int n = 0; while (p.renderNextThumbnail()) ++n;
        std::printf ("BREED: %d thumbnails in %.1f ms, children peak %.3f, e.g. %s\n", n, juce::Time::getMillisecondCounterHiRes() - t0, worst,
                     p.kids()[0].g.name.toRawUTF8());
    }
    // ERA / FUTURE / BREED extremes stay finite and bounded on every category
    {
        KeysKillaProcessor p; p.prepareToPlay (48000, 256);
        juce::AudioBuffer<float> b (2, 256);
        float worst = 0; bool finite = true;
        const auto& ps = factoryPresets();
        for (int i = 0; i < (int) ps.size(); i += 7)
        {
            p.setCurrentProgram (i);
            if (i % 3 == 0) p.breedWith ((i * 13) % (int) ps.size());
            set (p, ID::era, (float) ((i / 7) % 7)); set (p, ID::future, (i % 2) ? 1.0f : 0.5f);
            for (int k = 0; k < 120; ++k)
            {
                juce::MidiBuffer m;
                if (k == 0) { m.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 127), 0); m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 127), 0); }
                if (k == 100) m.addEvent (juce::MidiMessage::allNotesOff (1), 0);
                p.processBlock (b, m);
                for (int ch = 0; ch < 2; ++ch) for (int n = 0; n < 256; ++n) { const float x = b.getSample (ch, n); finite &= std::isfinite (x); worst = std::max (worst, std::abs (x)); }
            }
            juce::MidiBuffer m; m.addEvent (juce::MidiMessage::allNotesOff (1), 0);
            for (int k = 0; k < 400; ++k) { p.processBlock (b, m); m.clear(); }   // let tails die
        }
        check (finite, "ERA/FUTURE finite");
        check (worst <= 1.01f, "ERA/FUTURE output bounded");
        std::printf ("ERA/FUTURE sweep peak %.3f\n", worst);
    }
    // render determinism (offline render == playback)
    {
        auto render = []
        {
            KeysKillaProcessor p; p.setCurrentProgram (5); p.prepareToPlay (48000, 256);
            juce::AudioBuffer<float> b (2, 256); double acc = 0;
            for (int k = 0; k < 200; ++k) { juce::MidiBuffer m; if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 90), 3); p.processBlock (b, m); acc += b.getSample (0, 100) * (k + 1); }
            return acc;
        };
        check (render() == render(), "deterministic render");
    }
    {   // hosts calling process before prepareToPlay, with no channels or no samples must not crash
        KeysKillaProcessor fresh;
        juce::AudioBuffer<float> b (2, 256), none (0, 256), empty (2, 0);
        juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        fresh.processBlock (b, m); fresh.processBlockBypassed (b, m);
        fresh.prepareToPlay (48000, 256);
        fresh.processBlock (none, m); fresh.processBlock (empty, m);
        check (true, "process before prepare / empty buffers");
    }
    KeysKillaProcessor p;
    p.setCurrentProgram (0);
    p.prepareToPlay (48000, 256);
    juce::AudioBuffer<float> buf (2, 256);
    auto run = [&] (juce::MidiBuffer m, int blocks = 1) { for (int k = 0; k < blocks; ++k) { p.processBlock (buf, m); m.clear(); } };
    auto noteOn = [] (int n) { juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0); return m; };
    auto noteOff = [] (int n) { juce::MidiBuffer m; m.addEvent (juce::MidiMessage::noteOff (1, n), 0); return m; };

    // chord: one key -> minor 7 (4 notes)
    set (p, ID::chord, 1); set (p, ID::chordType, 1); run ({}, 1);
    run (noteOn (60), 2); check (countNotes (p) == 4, "chord minor7 plays 4 notes");
    run (noteOff (60), 2); check (countNotes (p) == 0, "chord releases all notes");
    set (p, ID::chord, 0); run ({}, 1);
    // key lock: C# in C minor snaps to a scale note
    set (p, ID::keyLock, 1); set (p, ID::key, 0); set (p, ID::scale, 0); run ({}, 1);
    run (noteOn (61), 2); check (! p.playing[61].load() && (p.playing[60].load() || p.playing[62].load()), "key lock snaps C# to scale");
    run (noteOff (61), 2); check (countNotes (p) == 0, "key lock note-off follows mapping");
    set (p, ID::keyLock, 0); run ({}, 1);
    // arp produces a changing note from a held chord
    set (p, ID::arp, 1); run ({}, 1);
    { juce::MidiBuffer m; for (int n : { 60, 63, 67 }) m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0); run (m, 1); }
    std::set<int> seen;
    for (int k = 0; k < 200; ++k) { run ({}, 1); for (int n : { 60, 63, 67 }) if (p.playing[(size_t) n].load()) seen.insert (n); }
    check (seen.size() >= 2, "arp cycles through held notes");
    { juce::MidiBuffer m; for (int n : { 60, 63, 67 }) m.addEvent (juce::MidiMessage::noteOff (1, n), 0); run (m, 40); }
    check (countNotes (p) == 0, "arp stops after release");
    set (p, ID::arp, 0); run ({}, 1);
    // all notes off: nothing hangs
    { juce::MidiBuffer m; for (int n = 40; n < 80; n += 3) m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0); run (m, 2); }
    { juce::MidiBuffer m; m.addEvent (juce::MidiMessage::allNotesOff (1), 0); run (m, 2); }
    check (countNotes (p) == 0, "all notes off");
    // sustain pedal holds, release frees
    run (noteOn (64), 1);
    { juce::MidiBuffer m; m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0); run (m, 1); }
    run (noteOff (64), 2); check (p.playing[64].load(), "sustain holds note");
    { juce::MidiBuffer m; m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0); run (m, 2); }
    check (! p.playing[64].load(), "sustain release");
    // bypass fades instead of clicking
    run (noteOn (60), 20);
    {
        juce::MidiBuffer m; float maxJump = 0, last = buf.getSample (0, 255);
        for (int k = 0; k < 10; ++k)
        {
            p.processBlockBypassed (buf, m);
            for (int i = 0; i < 256; ++i) { maxJump = std::max (maxJump, std::abs (buf.getSample (0, i) - last)); last = buf.getSample (0, i); }
        }
        check (maxJump < 0.2f && std::abs (last) < 1.0e-4f, "bypass fades out without a click");
    }
    // 192 kHz works
    p.prepareToPlay (192000, 1024);
    {
        juce::AudioBuffer<float> b (2, 1024); bool fin = true; float pk = 0;
        for (int k = 0; k < 100; ++k) { juce::MidiBuffer m; if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 48, (juce::uint8) 100), 0); p.processBlock (b, m);
            for (int i = 0; i < 1024; ++i) { fin &= std::isfinite (b.getSample (0, i)); pk = std::max (pk, std::abs (b.getSample (0, i))); } }
        check (fin && pk > 0.001f && pk <= 1.0f, "192 kHz render");
    }
    return fails;
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
    if (argc > 1 && juce::String (argv[1]) == "-loadtime")   // preset switching cost + host notifications
    {
        struct Counter : juce::AudioProcessorListener
        {
            int params = 0, changes = 0;
            void audioProcessorParameterChanged (juce::AudioProcessor*, int, float) override { ++params; }
            void audioProcessorChanged (juce::AudioProcessor*, const ChangeDetails&) override { ++changes; }
        } counter;
        p.addListener (&counter);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < 200; ++i) p.setCurrentProgram ((i * 37) % p.getNumPrograms());
        const auto t1 = juce::Time::getMillisecondCounterHiRes();
        std::printf ("preset load: %.3f ms avg, %.1f host parameter notifications per load, %.1f program notifications\n",
                     (t1 - t0) / 200.0, counter.params / 200.0, counter.changes / 200.0);
        p.removeListener (&counter);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "-edtime")   // editor open / close time
    {
        for (int k = 0; k < 5; ++k)
        {
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);   // first full paint
            const auto t1 = juce::Time::getMillisecondCounterHiRes();
            ed.reset();
            const auto t2 = juce::Time::getMillisecondCounterHiRes();
            std::printf ("open+paint %.1f ms, close %.1f ms\n", t1 - t0, t2 - t1);
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "-stats")
    {
        int ex = 0, bass = 0; std::array<int, numCategories> perTile {}; std::array<int, numEras> perEra {};
        std::map<juce::String, int> perSub;
        for (auto& pr : factoryPresets()) { ex += pr.exclusive; bass += pr.isBass(); ++perTile[(size_t) pr.cat]; ++perEra[(size_t) pr.era];
                                            ++perSub[categoryNames()[pr.cat] + " / " + pr.sub]; }
        std::printf ("total %d, exclusive %d, bass %d\n", (int) factoryPresets().size(), ex, bass);
        for (int t = 0; t < numCategories; ++t) std::printf ("  %s %d\n", categoryNames()[t].toRawUTF8(), perTile[(size_t) t]);
        for (int e = 0; e < numEras; ++e) std::printf ("  era %s %d\n", eraNames()[e].toRawUTF8(), perEra[(size_t) e]);
        int missing = 0;
        for (int c = 0; c < numCategories; ++c)
            for (auto& sname : subcategoryNames (c))
                if (perSub[categoryNames()[c] + " / " + sname] == 0) { std::printf ("  EMPTY SUBCATEGORY %s / %s\n", categoryNames()[c].toRawUTF8(), sname.toRawUTF8()); ++missing; }
        for (auto& [k, n] : perSub)
        {
            bool known = false;
            for (int c = 0; c < numCategories; ++c) for (auto& sname : subcategoryNames (c)) known |= k == categoryNames()[c] + " / " + sname;
            if (! known) std::printf ("  UNKNOWN SUBCATEGORY %s (%d)\n", k.toRawUTF8(), n);
        }
        std::printf ("  empty subcategories: %d\n", missing);
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "-bench")   // CPU: 8 held notes, 20 s of audio at 48 kHz, normal and eco
    {
        for (const char* name : { "Ice Bells 2013", "Rage Supersaw", "Layered EP Dream", "Wavetable Motion Pad", "Blown Rage Lead", "Reverse Ghost Pad" })
            for (bool eco : { false, true })
            {
                int idx = 0; for (int i = 0; i < p.getNumPrograms(); ++i) if (p.getProgramName (i) == name) idx = i;
                p.setCurrentProgram (idx); p.eco = eco; p.prepareToPlay (48000, 512);
                juce::AudioBuffer<float> b (2, 512);
                const auto t = juce::Time::getMillisecondCounterHiRes();
                for (int k = 0; k < 48000 * 20 / 512; ++k)
                {
                    juce::MidiBuffer m;
                    if (k == 0) for (int n : { 48, 51, 55, 58, 60, 63, 67, 70 }) m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), 0);
                    p.processBlock (b, m);
                }
                const double ms = juce::Time::getMillisecondCounterHiRes() - t;
                std::printf ("%-22s %s  CPU %.1f %% of one core\n", name, eco ? "eco   " : "normal", ms / 200.0);
            }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "-dc")   // -dc <idx>: mean per 250 ms while a note is held for 6 s
    {
        const int idx = juce::String (argv[2]).getIntValue();
        p.setCurrentProgram (idx); p.prepareToPlay (44100.0, 441);
        juce::AudioBuffer<float> buf (2, 441);
        double sum = 0; int cnt = 0;
        for (int b = 0; b < 600; ++b)
        {
            juce::MidiBuffer m; if (b == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            p.processBlock (buf, m);
            for (int i = 0; i < 441; ++i) { sum += buf.getSample (0, i); ++cnt; }
            if (cnt >= 11025) { std::printf ("%.4f ", sum / cnt); sum = 0; cnt = 0; }
        }
        std::printf ("\n");
        return 0;
    }
    if (argc > 3 && juce::String (argv[1]) == "-dump")   // -dump <prev idx> <idx>: parameter values after loading idx
    {
        const int a = juce::String (argv[2]).getIntValue(), b = juce::String (argv[3]).getIntValue();
        if (a >= 0) p.setCurrentProgram (a);
        p.setCurrentProgram (b);
        for (auto* prm : p.getParameters())
            if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                std::printf ("%s=%.4f\n", rp->getParameterID().toRawUTF8(), p.apvts.getRawParameterValue (rp->getParameterID())->load());
        return 0;
    }
    if (argc > 3 && juce::String (argv[1]) == "-seq")   // -seq <from> <to>: render a range, print loudness
    {
        for (int i = juce::String (argv[2]).getIntValue(); i <= juce::String (argv[3]).getIntValue(); ++i)
        {
            float rms, peak, dc; bool fin;
            renderPreset (p, i, 44100.0, rms, peak, dc, fin);
            std::printf ("%3d %-32s win %.1f dB\n", i, p.getProgramName (i).toRawUTF8(), juce::Decibels::gainToDecibels ((float) maxWin));
        }
        return 0;
    }
    if (argc > 2 && juce::String (argv[1]) == "-probe")   // -probe <preset name>: which parameter makes it loud?
    {
        int idx = -1;
        for (int i = 0; i < p.getNumPrograms(); ++i) if (p.getProgramName (i) == juce::String (argv[2])) idx = i;
        if (idx < 0) return 1;
        float rms, peak, dc; bool fin;
        if (argc > 3)
            for (int i = 0; i < p.getNumPrograms(); ++i)
                if (p.getProgramName (i) == juce::String (argv[3])) { renderPreset (p, i, 44100.0, rms, peak, dc, fin); std::printf ("prev: win %.1f dB\n", juce::Decibels::gainToDecibels ((float) maxWin)); }
        renderPreset (p, idx, 44100.0, rms, peak, dc, fin);
        std::printf ("base: win %.1f dB\n", juce::Decibels::gainToDecibels ((float) maxWin));
        for (auto* id : { ID::crush, ID::wow, ID::m4, ID::phaser, ID::reverse, ID::tape, ID::bend, ID::revMix, ID::delayMix, ID::chorus, ID::ghost, ID::gain })
        {
            auto vals = factoryPresets()[(size_t) idx].values;
            p.setCurrentProgram (idx);
            auto* prm = p.apvts.getParameter (id);
            const float keep = prm->getValue();
            prm->setValueNotifyingHost (juce::String (id) == ID::gain ? 0.0f : prm->getDefaultValue() * 0.0f);
            p.prepareToPlay (44100.0, 480);
            // render without reloading the preset
            juce::AudioBuffer<float> buf (2, 480); double mw = 0, ws = 0; long wn = 0;
            for (int b = 0; b < 300; ++b)
            {
                juce::MidiBuffer m; if (b == 0) for (int nn : { 60, 63, 67 }) m.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
                if (b == 138) for (int nn : { 60, 63, 67 }) m.addEvent (juce::MidiMessage::noteOff (1, nn), 0);
                p.processBlock (buf, m);
                for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 480; ++i) { ws += buf.getSample (ch, i) * buf.getSample (ch, i); if (++wn >= 8820) { mw = std::max (mw, std::sqrt (ws / wn)); ws = 0; wn = 0; } }
            }
            std::printf ("%-10s zeroed (was %.2f): win %.1f dB\n", id, keep, juce::Decibels::gainToDecibels ((float) mw));
        }
        return 0;
    }
    if (argc > 3 && juce::String (argv[1]) == "-shot")   // -shot <out.png> <skin 0|1> : GUI snapshot
    {
        juce::PropertiesFile::Options o; o.applicationName = "KEYS KILLA"; o.filenameSuffix = "settings"; o.folderName = "KEYS KILLA";
        juce::PropertiesFile (o).setValue ("skin", juce::String (argv[3]).getIntValue());
        juce::PropertiesFile (o).setValue ("scale", argc > 5 ? juce::String (argv[5]).getIntValue() : 60);
        p.setCurrentProgram (1);
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        if (argc > 4) dynamic_cast<KeysKillaEditor*> (ed.get())->showView (juce::String (argv[4]).getIntValue());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
        out.deleteFile();
        juce::FileOutputStream os (out);
        juce::PNGImageFormat().writeImageToStream (img, os);
        return 0;
    }
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    int failures = unitTests();
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
            p.rollDice (factoryPresets()[(size_t) (d % p.getNumPrograms())].cat);
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
