#include <thread>
#include <atomic>
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
    // step arp: rests are silent, steps play; RESET restores the loaded sound
    {
        auto run = [&] (float stepValue)
        {
            KeysKillaProcessor p; p.setCurrentProgram (0); p.prepareToPlay (48000, 256);
            set (p, ID::arp, 1); set (p, ID::arpRate, 1); set (p, ID::arpSteps, 4);
            for (int st = 0; st < 16; ++st) set (p, ID::arpStep (st).toRawUTF8(), stepValue);
            juce::AudioBuffer<float> b (2, 256); float peak = 0;
            for (int k = 0; k < 200; ++k)
            {
                juce::MidiBuffer m; if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
                p.processBlock (b, m); peak = std::max (peak, b.getMagnitude (0, 256));
            }
            return std::make_pair (peak, p.arpCurStep.load());
        };
        const auto rest = run (0.0f), play = run (1.0f);
        check (rest.first < 1.0e-4f, "arp rests are silent");
        check (play.first > 0.01f && play.second >= 0 && play.second < 4, "arp steps play within the step length");
        KeysKillaProcessor p; p.setCurrentProgram (3);
        const float before = p.apvts.getParameter (ID::cutoff)->getValue();
        set (p, ID::cutoff, 200.0f);
        p.resetParams ({ ID::cutoff });
        check (std::abs (p.apvts.getParameter (ID::cutoff)->getValue() - before) < 1.0e-5f, "RESET restores the loaded value");
    }
    // BREED LAB: six playable children, deterministic genes, gene switch + lock, state round trip
    {
        KeysKillaProcessor p; p.prepareToPlay (48000, 256);
        const auto& ps = factoryPresets();
        juce::AudioBuffer<float> b (2, 256);
        bool finite = true; float worst = 0;
        for (int round = 0; round < 12; ++round)
        {
            p.breedWild = (round % 2) ? 1.0f : 0.0f;   // SAFE and CRAZY
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
    // BREED LOOPS: a new melody every time, always in key and in range
    {
        std::set<std::vector<int>> seen; bool ok = true, inKey = true;
        for (uint32_t sd = 1; sd <= 2000; ++sd)
        {
            const auto l = kk::loopFromSeed (sd * 7919u);
            const auto notes = kk::buildLoop (l, -1, sd % 2 ? 8 : 16, false, false);
            std::vector<int> sig;
            for (auto& n : notes)
            {
                ok &= n.note >= 20 && n.note <= 100 && n.start >= 0 && n.len > 0 && n.start + n.len <= (sd % 2 ? 32.05f : 64.05f);
                sig.push_back (n.note * 1000 + (int) (n.start * 12));
                if (! n.low)
                {
                    const int deg = ((n.note - l.key) % 12 + 12) % 12;   // natural minor + phrygian b2 + leading tone
                    inKey &= deg != 4 && deg != 6 && deg != 9;
                }
            }
            ok &= notes.size() >= 8;
            seen.insert (sig);
        }
        check (ok, "loops in range and inside their bars");
        check (inKey, "melody notes stay in the scale");
        check (seen.size() >= 1900, "2000 seeds give (almost) 2000 different melodies");
        bool fitOk = true;
        for (uint32_t sd = 1; sd <= 500; ++sd)
        {
            const auto x = kk::loopFromSeed (sd);
            fitOk &= kk::loopdata::answerOk ((int) x.g[kk::loopOpener], (int) x.g[kk::loopAnswer], (int) x.g[kk::loopBass])
                  && kk::loopdata::turnOk ((int) x.g[kk::loopTurn], (int) x.g[kk::loopBass]);
        }
        check (fitOk, "phrases always fit the chords and each other");
        const auto l = kk::loopFromSeed (99);
        bool lowOnly = true; for (auto& n : kk::buildLoop (l, -1, 8, true, false)) lowOnly &= n.low;
        bool riffOnly = true; for (auto& n : kk::buildLoop (l, -1, 8, false, true)) riffOnly &= ! n.low;
        check (lowOnly && riffOnly && ! kk::buildLoop (l, -1, 8, true, false).empty(), "bass sounds play the low line, mono leads the melody");
        const auto c1 = kk::crossLoops (l, kk::loopFromSeed (5), 11, 0.5f, 0.0f), c2 = kk::crossLoops (l, kk::loopFromSeed (5), 12, 0.5f, 0.0f);
        check (! (c1 == l) && ! (c2 == l) && kk::buildLoop (c1, 0, 8, false, false).size() > 0, "a child's melody differs from its parents");
        std::printf ("LOOPS: %d different melodies from 2000 seeds\n", (int) seen.size());
    }
    // FAMILY TREE: 4 sounds -> 6 sounds or 6 loops, new every press, loop playback, MIDI export, state
    {
        KeysKillaProcessor p; p.prepareToPlay (48000, 256);
        const auto& ps = factoryPresets();
        p.setAncestorPreset (0, 0); p.setAncestorPreset (1, 60); p.setAncestorPreset (2, 200); p.setAncestorPreset (3, (int) ps.size() - 1);
        check (p.treeBreed() == 6, "family tree breeds 6 results");
        std::set<std::vector<int>> mel;
        auto sig = [&] (const KeysKillaProcessor::Genome& g) { std::vector<int> v; for (auto& n : kk::buildLoop (g.loop, 0, 8, false, false)) v.push_back (n.note * 1000 + (int) (n.start * 4)); return v; };
        for (auto& r : p.treeKids()) mel.insert (sig (r.g));
        p.treeBreed(); for (auto& r : p.treeKids()) mel.insert (sig (r.g));
        check (mel.size() >= 10, "every BREED gives new melodies");
        std::set<uint32_t> openers; for (auto& r : p.treeKids()) openers.insert (r.g.loop.g[kk::loopOpener]);
        check (openers.size() == 6, "six different openings in one BREED");
        bool valid = true; for (auto& r : p.treeKids()) valid &= r.g.valid() && r.g.loop.valid && r.g.name.contains ("+");
        check (valid, "results are full sounds named after their family");
        p.clearAncestor (2); p.clearAncestor (3);
        check (p.treeBreed() == 6, "family tree works with 2 sounds");
        p.setTreeMode (KeysKillaProcessor::treeLoop);
        p.treeBreed();
        p.playTreeResult (1);
        check (p.loopIsTree (1) && p.treeSelected() == 1, "LOOP result plays its loop");
        juce::AudioBuffer<float> b (2, 256); float peakOn = 0, peakOff = 0;
        bool finite = true;
        for (int k = 0; k < 375; ++k) { juce::MidiBuffer m; p.processBlock (b, m); peakOn = std::max (peakOn, b.getMagnitude (0, 256)); finite &= std::isfinite (b.getSample (0, 0)); }
        p.newMelody (1);
        for (int k = 0; k < 200; ++k) { juce::MidiBuffer m; p.processBlock (b, m); finite &= std::isfinite (b.getSample (0, 0)); }
        p.playTreeResult (1);
        check (! p.loopPlaying(), "second press stops the loop");
        for (int k = 0; k < 1500; ++k) { juce::MidiBuffer m; p.processBlock (b, m); if (k > 1100) peakOff = std::max (peakOff, b.getMagnitude (0, 256)); }
        check (finite && peakOn > 0.01f && peakOff < 1.0e-3f, "loop plays and stops");
        const auto f = p.exportLoopMidi (p.treeKids()[1].g);
        juce::MidiFile mf; int notes = 0;
        if (juce::FileInputStream in { f }; in.openedOk() && mf.readFrom (in))
            for (int t = 0; t < mf.getNumTracks(); ++t) for (auto* e : *mf.getTrack (t)) notes += e->message.isNoteOn() ? 1 : 0;
        check (f.existsAsFile() && notes == (int) p.loopNotes (p.treeKids()[1].g).size() && notes > 0, "loop exports as a MIDI file");
        p.setLoopBars (16); p.setLoopKey (5);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        bool same = q.treeKids().size() == p.treeKids().size() && q.ancestor (0).name == p.ancestor (0).name && ! q.ancestor (2).valid();
        for (size_t i = 0; same && i < p.treeKids().size(); ++i) same &= q.treeKids()[i].g.loop == p.treeKids()[i].g.loop;
        check (same && q.getTreeMode() == KeysKillaProcessor::treeLoop && q.loopBars() == 16 && q.loopKey() == 5, "family tree survives save / load");
        std::printf ("FAMILY TREE: %s, %d notes exported\n", f.getFileName().toRawUTF8(), notes);
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
    // -rendermidi in.mid out.wav <sound per track>...   sound = "Preset Name@gain" or "breed:Parent A|Parent B|child|wild@gain"
    if (argc > 4 && juce::String (argv[1]) == "-rendermidi")
    {
        juce::MidiFile mf;
        { const juce::File midFile { juce::String (argv[2]) }; juce::FileInputStream in { midFile }; if (! in.openedOk() || ! mf.readFrom (in)) return 1; }
        const double rate = 32000.0;
        double bpm = 140;
        juce::MidiMessageSequence tempo; mf.findAllTempoEvents (tempo);
        if (tempo.getNumEvents() > 0) bpm = 60.0 / tempo.getEventPointer (0)->message.getTempoSecondsPerQuarterNote();
        const double secPerTick = 60.0 / bpm / mf.getTimeFormat();
        auto findPreset = [] (const juce::String& n) { const auto& ps = factoryPresets(); for (int i = 0; i < (int) ps.size(); ++i) if (ps[(size_t) i].name == n) return i; std::printf ("?? preset %s\n", n.toRawUTF8()); return 0; };
        std::vector<float> mixL, mixR;
        for (int tr = 0; tr < juce::jmin (argc - 4, mf.getNumTracks()); ++tr)
        {
            juce::String spec (argv[4 + tr]);
            const float g = spec.contains ("@") ? spec.fromLastOccurrenceOf ("@", false, false).getFloatValue() : 1.0f;
            spec = spec.upToLastOccurrenceOf ("@", false, false);
            KeysKillaProcessor p;
            juce::String used = spec;
            if (spec.startsWith ("breed:"))
            {
                auto parts = juce::StringArray::fromTokens (spec.fromFirstOccurrenceOf ("breed:", false, false), "|", "");
                p.setParentPreset (0, findPreset (parts[0])); p.setParentPreset (1, findPreset (parts[1]));
                p.breedWild = parts.size() > 3 ? parts[3].getFloatValue() : 0.3f;
                p.breed(); p.selectChild (juce::jlimit (0, 5, parts[2].getIntValue() - 1));
                used = p.currentName();
            }
            else p.setCurrentProgram (findPreset (spec));
            p.prepareToPlay (rate, 512);
            const auto* seq = mf.getTrack (tr);
            double endSec = 0; for (auto* e : *seq) endSec = std::max (endSec, e->message.getTimeStamp() * secPerTick);
            const int total = (int) ((endSec + 2.5) * rate);
            if ((int) mixL.size() < total) { mixL.resize ((size_t) total, 0.0f); mixR.resize ((size_t) total, 0.0f); }
            juce::AudioBuffer<float> b (2, 512);
            int ei = 0;
            for (int pos = 0; pos < total; pos += 512)
            {
                juce::MidiBuffer m;
                while (ei < seq->getNumEvents())
                {
                    const auto& msg = seq->getEventPointer (ei)->message;
                    const int at = (int) (msg.getTimeStamp() * secPerTick * rate);
                    if (at >= pos + 512) break;
                    if (msg.isNoteOnOrOff()) m.addEvent (msg, juce::jmax (0, at - pos));
                    ++ei;
                }
                p.processBlock (b, m);
                for (int i = 0; i < 512 && pos + i < total; ++i) { mixL[(size_t) (pos + i)] += g * b.getSample (0, i); mixR[(size_t) (pos + i)] += g * b.getSample (1, i); }
            }
            std::printf ("  track %d: %s (gain %.2f)\n", tr + 1, used.toRawUTF8(), g);
        }
        float peak = 1.0e-6f; for (size_t i = 0; i < mixL.size(); ++i) peak = std::max ({ peak, std::abs (mixL[i]), std::abs (mixR[i]) });
        juce::AudioBuffer<float> out (2, (int) mixL.size());
        for (int i = 0; i < (int) mixL.size(); ++i)   // gentle glue: soft clip then normalise
        {
            out.setSample (0, i, std::tanh (1.4f * mixL[(size_t) i] / peak) * 0.93f);
            out.setSample (1, i, std::tanh (1.4f * mixR[(size_t) i] / peak) * 0.93f);
        }
        const juce::File f { juce::String (argv[3]) }; f.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), rate, 2, 16, {}, 0));
        if (w) w->writeFromAudioSampleBuffer (out, 0, out.getNumSamples());
        std::printf ("%s: %.1f s\n", argv[3], (double) mixL.size() / rate);
        return 0;
    }
    if (argc > 3 && juce::String (argv[1]) == "-loopdemo")   // -loopdemo <dir> <count> [firstSeed]: melody loops as MIDI (8 bars, 140 BPM)
    {
        const juce::File dir { juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]) };
        dir.createDirectory();
        const int count = juce::String (argv[3]).getIntValue();
        const uint32_t first = argc > 4 ? (uint32_t) juce::String (argv[4]).getLargeIntValue() : 1u;
        for (int i = 0; i < count; ++i)
        {
            const auto l = kk::loopFromSeed (kk::hash32 (first + (uint32_t) i * 7919u));
            juce::MidiMessageSequence seq;
            auto tempo = juce::MidiMessage::tempoMetaEvent (60000000 / 140); tempo.setTimeStamp (0); seq.addEvent (tempo);
            for (auto& n : kk::buildLoop (l, -1, 8, false, false))
            {
                seq.addEvent (juce::MidiMessage::noteOn (1, n.note, (juce::uint8) 100), std::round (n.start * 96));
                seq.addEvent (juce::MidiMessage::noteOff (1, n.note), std::round ((n.start + n.len) * 96));
            }
            seq.updateMatchedPairs();
            juce::MidiFile mf; mf.setTicksPerQuarterNote (96); mf.addTrack (seq);
            const auto name = juce::String::formatted ("KK Loop %02d - ", i + 1) + kk::keyName (l.key) + " MIN 140BPM.mid";
            auto f = dir.getChildFile (name); f.deleteFile();
            if (juce::FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
            std::printf ("%s\n", name.toRawUTF8());
        }
        return 0;
    }
    if (argc > 1 && juce::String (argv[1]) == "-longuse")   // host-like long session, then measure shutdown
    {
        const int seconds = argc > 2 ? juce::String (argv[2]).getIntValue() : 20;
        auto proc = std::make_unique<KeysKillaProcessor>();
        proc->prepareToPlay (48000, 256);
        std::atomic<bool> run { true };
        std::thread audio ([&]
        {
            juce::AudioBuffer<float> b (2, 256); kk::Rng r; r.seed (5); int k = 0;
            while (run)
            {
                juce::MidiBuffer m;
                if (k % 40 == 0) m.addEvent (juce::MidiMessage::noteOn (1, 48 + (int) (r.uni() * 24), (juce::uint8) 100), 0);
                if (k % 40 == 30) m.addEvent (juce::MidiMessage::allNotesOff (1), 0);
                proc->processBlock (b, m); ++k;
                std::this_thread::sleep_for (std::chrono::microseconds (2000));
            }
        });
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc->createEditor());
       
        auto* ke = dynamic_cast<KeysKillaEditor*> (ed.get());
        const auto end = juce::Time::getMillisecondCounterHiRes() + seconds * 1000.0;
        int action = 0;
        while (juce::Time::getMillisecondCounterHiRes() < end)
        {
            juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
            switch (action++ % 12)
            {
                case 0: proc->breed(); break;
                case 1: proc->previewChild (action % 6); break;
                case 2: proc->setCurrentProgram (action % proc->getNumPrograms()); break;
                case 3: ke->showView (1 + action % 8); break;
                case 4: ke->showView (9); break;
                case 5: ke->showView (11); break;
                case 6: proc->setParentChild (0, 2); break;
                case 7: { juce::MemoryBlock mb; proc->getStateInformation (mb); } break;
                case 8: proc->rollDice (0); break;
                case 9: ed.reset(); ed.reset (proc->createEditor()); ke = dynamic_cast<KeysKillaEditor*> (ed.get()); break;
                case 10: proc->setTreeMode (action % 2); proc->treeBreed(); proc->playTreeResult (action % 6); break;
                case 11: ke->showView (13 + action % 2); break;
                default: break;
            }
        }
        run = false; audio.join();
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        ed.reset();
        const auto t1 = juce::Time::getMillisecondCounterHiRes();
        juce::MemoryBlock mb; proc->getStateInformation (mb);
        const auto t2 = juce::Time::getMillisecondCounterHiRes();
        proc->releaseResources(); proc.reset();
        const auto t3 = juce::Time::getMillisecondCounterHiRes();
        std::printf ("after %d s: close editor %.1f ms, save state %.1f ms (%d bytes), destroy plugin %.1f ms\n", seconds, t1 - t0, t2 - t1, (int) mb.getSize(), t3 - t2);
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
                if (c != c808 && perSub[categoryNames()[c] + " / " + sname] == 0) { std::printf ("  EMPTY SUBCATEGORY %s / %s\n", categoryNames()[c].toRawUTF8(), sname.toRawUTF8()); ++missing; }
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
