#include <thread>
#include <atomic>
#include <map>
// Offline render of every factory preset: checks for NaN/Inf, silence, DC and loudness spread.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include "../Source/plugins/digga/PluginProcessor.h"
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
    // the old ARP switch is ignored (loops replace it); RESET restores the loaded sound
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
        const auto rest = run (0.0f);
        check (rest.first > 0.01f, "ARP removed (v0.14): an old arp setting never silences a note");
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
    // low sample rates (thumbnails render at 16 kHz): the whole chain stays finite and alive
    {
        KeysKillaProcessor p; p.prepareToPlay (16000, 400);
        juce::AudioBuffer<float> b (2, 400); bool finite = true; float late = 0;
        for (int k = 0; k < 30; ++k)
        {
            juce::MidiBuffer m; if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
            p.processBlock (b, m);
            for (int ch = 0; ch < 2; ++ch) for (int n = 0; n < 400; ++n) finite &= std::isfinite (b.getSample (ch, n));
            if (k > 10) late = std::max (late, b.getMagnitude (0, 400));
        }
        check (finite && late > 1.0e-3f, "16 kHz render stays finite and keeps sounding");
    }
    // v0.17 modules: Voodoo Killa (HALF), Effector Killa (EFFECTOR), Digga Killa (DIGGA) inside KEYS KILLA
    {
        KeysKillaProcessor p; p.setCurrentProgram (3); p.prepareToPlay (44100, 512);
        check (p.module (KeysKillaProcessor::modHalf) != nullptr && p.module (KeysKillaProcessor::modEffector) != nullptr
               && p.module (KeysKillaProcessor::modDigga) != nullptr, "the three KILLA plugins are inside");
        juce::AudioBuffer<float> b (2, 512);
        auto render = [&] (float& peak, double& energy)
        {
            bool finite = true; peak = 0; energy = 0;
            for (int k = 0; k < 200; ++k)
            {
                juce::MidiBuffer m;
                if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
                if (k == 150) m.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
                p.processBlock (b, m);
                for (int ch = 0; ch < 2; ++ch) for (int i = 0; i < 512; ++i) { const float v = b.getSample (ch, i); finite &= std::isfinite (v); peak = std::max (peak, std::abs (v)); energy += v * v; }
            }
            return finite;
        };
        float pk; double dry, en;
        check (render (pk, dry) && pk > 0.01f, "melody plays with the modules loaded");
        set (p, ID::efxOn, 1);
        check (render (pk, en) && pk > 0.001f && pk <= 1.0f, "EFFECTOR (Effector Killa) on the melody stays finite");
        set (p, ID::efxOn, 0); set (p, ID::halfOn, 1);
        check (render (pk, en) && pk <= 1.0f, "HALF (Voodoo Killa) on the melody stays finite");
        set (p, ID::halfOn, 0);
        set (p, ID::playMode, 1);
        render (pk, en);
        check (! p.playing[60].load(), "keys -> DIGGA: the synth stays silent");
        set (p, ID::playMode, 0);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (q.getLatencySamples() >= 0, "state with the three modules restores");
        std::printf ("MODULES: latency HALF %d, EFFECTOR %d samples\n", p.module (KeysKillaProcessor::modHalf)->getLatencySamples(),
                     p.module (KeysKillaProcessor::modEffector)->getLatencySamples());
        // ROLLS generator
        std::set<std::vector<int>> seen; bool inside = true;
        for (int sd = 1; sd <= 200; ++sd)
            for (int st = 0; st < 4; ++st)
            {
                const auto r = kk::makeRolls ((uint32_t) sd, st, 2, 0.6f);
                std::vector<int> sig; for (auto& h : r) { sig.push_back ((int) std::lround (h.beat * 96)); inside &= h.beat >= 0 && h.beat < 8.0; }
                seen.insert (sig);
            }
        check (inside && seen.size() > 700, "ROLLS: a new pattern for (almost) every seed / style");
        // v0.22 808 lines + snare rolls: trap styles, new for every seed, inside the bars
        for (int kind = 0; kind < 2; ++kind)
        {
            std::set<std::vector<int>> seen2; bool in2 = true, slides = false;
            for (int sd = 1; sd <= 100; ++sd)
                for (int st = 0; st < 4; ++st)
                {
                    const auto r = kind == 0 ? kk::make808 ((uint32_t) sd, st, 2, 0.6f) : kk::makeSnares ((uint32_t) sd, st, 2, 0.6f);
                    std::vector<int> sig;
                    for (size_t k = 0; k < r.size(); ++k)
                    {
                        sig.push_back ((int) std::lround (r[k].beat * 96) * 64 + r[k].semi);
                        in2 &= r[k].beat >= 0 && r[k].beat < 8.0 && r[k].vel > 0 && r[k].vel <= 1 && r[k].len > 0;
                        if (kind == 0 && k + 1 < r.size()) slides |= r[k].beat + r[k].len > r[k + 1].beat + 1.0e-6;
                    }
                    in2 &= ! r.empty();
                    seen2.insert (sig);
                }
            check (in2 && seen2.size() > 350 && (kind == 1 || slides), kind == 0 ? "808 PATTERNS: trap lines with slides, new every time" : "SNARE ROLLS: new every time, inside the bars");
        }
        for (int d = 0; d < 3; ++d)
        {
            p.generatePattern (d, 1, 2, 0.6f);
            const auto f = p.exportPatternMidi (d);
            juce::MidiFile mf; juce::FileInputStream is (f);
            check (f.existsAsFile() && mf.readFrom (is) && mf.getTrack (0)->getNumEvents() > 4, ("PATTERN " + juce::String (d) + " exports a MIDI clip").toRawUTF8());
        }
        // an edited pattern comes back with the project
        std::vector<kk::RollHit> mine { { 0.0, 0.9f, 0, 0.75 }, { 1.5, 0.6f, 5, 0.5 }, { 3.0, 1.0f, -2, 1.0 } };
        p.setPattern (0, mine, 1);
        juce::MemoryBlock pmb; p.getStateInformation (pmb);
        KeysKillaProcessor pq (false); pq.setStateInformation (pmb.getData(), (int) pmb.getSize());
        const auto back = pq.pattern (0);
        check (back.size() == 3 && back[1].semi == 5 && std::abs (back[1].beat - 1.5) < 1.0e-3 && pq.patBars[0] == 1, "the edited 808 pattern comes back with the project");
    }
    // v0.18 DRUM BOOST: your 808 in, boosted 808 out (in tune on the keys)
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_808_test.wav");
        {
            juce::AudioBuffer<float> b808 (1, 44100);
            for (int i = 0; i < 44100; ++i) b808.setSample (0, i, 0.7f * std::sin (kk::twoPi * 55.0f * (float) i / 44100.0f) * std::exp (-(float) i / 20000.0f));
            f.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), 44100, 1, 24, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (b808, 0, b808.getNumSamples());
        }
        check (p.loadDrum (0, f), "DRUM BOOST loads a WAV");
        auto s = p.drum (0).current();
        check (s != nullptr && std::abs (s->rootHz - 55.0f) < 2.0f && s->rootNote == 33, "808 note detected (A1, 55 Hz)");
        for (int k : { 0, 2, 3, 5, 6, 7 }) set (p, ID::boost (0, k).toRawUTF8(), k == 0 ? 12.0f : k == 5 ? 18.0f : 1.0f);
        p.renderDrum (0);
        s = p.drum (0).current();
        bool ok = s != nullptr; float pk = 0;
        if (ok) for (int c = 0; c < 2; ++c) for (int i = 0; i < s->audio.getNumSamples(); ++i) { const float v = s->audio.getSample (c, i); ok &= std::isfinite (v); pk = std::max (pk, std::abs (v)); }
        check (ok && pk <= 0.97f && pk > 0.5f, "808 boost: loud, finite, never over -0.3 dBFS");
        const auto out = p.exportDrum (0);
        check (out.existsAsFile() && out.getSize() > 10000, "808 boost exports a WAV for FL");
        set (p, ID::playMode, (float) KeysKillaProcessor::play808);
        juce::AudioBuffer<float> b (2, 512); float peak = 0;
        for (int k = 0; k < 40; ++k)
        {
            juce::MidiBuffer m; if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 33, (juce::uint8) 110), 0);
            p.processBlock (b, m); peak = std::max (peak, b.getMagnitude (0, 512));
        }
        check (peak > 0.2f && ! p.playing[33].load(), "the keys play the boosted 808, not the synth");
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (q.drum (0).hasSample(), "the 808 comes back with the project");
        // the 808 pattern plays the boosted 808 (preview on the page)
        set (p, ID::playMode, 0);
        p.setPattern (0, { { 0.0, 1.0f, 0, 0.5 }, { 1.0, 1.0f, 3, 0.5 } }, 1);
        p.patPlay = 0; peak = 0;
        for (int k = 0; k < 80; ++k) { juce::MidiBuffer m; p.processBlock (b, m); peak = std::max (peak, b.getMagnitude (0, 512)); }
        p.patPlay = -1;
        check (peak > 0.2f && p.patBeat.load() >= 0.0f, "PATTERN preview plays the 808");
        f.deleteFile();
    }
    // DIGGA KILLA inside KEYS KILLA: a dropped sample is analysed and cut into loops / one-shots
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        auto* dg = dynamic_cast<digga::DiggaKillaProcessor*> (p.module (KeysKillaProcessor::modDigga));
        check (dg != nullptr, "DIGGA module is Digga Killa");
        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_digga_loop.wav");
        {
            juce::AudioBuffer<float> b (2, 44100 * 8);
            juce::Random rnd (3);
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                const int beat = i % 22050;
                const float kick = beat < 4000 ? std::sin ((float) beat * 0.012f) * std::exp (-(float) beat / 1500.0f) : 0.0f;
                const float pad = 0.25f * std::sin ((float) i * 0.031f) + 0.15f * std::sin ((float) i * 0.047f);
                const float hat = (i % 5512) < 600 ? (rnd.nextFloat() - 0.5f) * 0.3f * std::exp (-(float) (i % 5512) / 200.0f) : 0.0f;
                b.setSample (0, i, kick + pad + hat); b.setSample (1, i, kick + pad * 0.9f + hat);
            }
            f.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), 44100, 2, 16, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
        if (dg != nullptr)
        {
            dg->getSampleStore().loadFile (f);
            juce::AudioBuffer<float> b (2, 512);
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            while (juce::Time::getMillisecondCounterHiRes() - t0 < 60000.0)
            {
                juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
                juce::MidiBuffer m; p.processBlock (b, m);
                if (dg->getSampleStore().getStatus() == digga::SampleStore::Status::ready && ! dg->getEngine().getTree().isEmpty()) break;
            }
            const auto st = dg->getSampleStore().getStatus();
            std::printf ("DIGGA: status %d, results %s after %.1f s\n", (int) st, dg->getEngine().getTree().isEmpty() ? "none" : "ready",
                         (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
            check (st == digga::SampleStore::Status::ready, "DIGGA loads a dropped sample");
            check (! dg->getEngine().getTree().isEmpty(), "DIGGA makes loops / one-shots from it");
            // CHOP / SLICE: the sample in slices, pads, keys, drag out, PAIR
            check (p.chopFromDigga() && p.chop.current() != nullptr && p.chop.current()->numSlices() >= 4, "CHOP: DIGGA's sample is cut at the hits");
            p.chop.autoSlice (16);
            check (p.chop.current()->numSlices() == 16, "CHOP: 16 equal slices");
            auto mk = p.chop.current()->marks; mk[3] += 2000; p.chop.setMarks (mk);
            check (p.chop.current()->marks[3] == mk[3], "CHOP: a moved cut stays where you put it");
            set (p, ID::playMode, (float) KeysKillaProcessor::playChop);
            juce::AudioBuffer<float> cb (2, 512); float cpk = 0;
            for (int k = 0; k < 30; ++k) { juce::MidiBuffer m; if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, kk::ChopLab::firstNote + 2, (juce::uint8) 110), 0); p.processBlock (cb, m); cpk = std::max (cpk, cb.getMagnitude (0, 512)); }
            check (cpk > 0.05f, "CHOP: the keys play the slices");
            set (p, ID::playMode, 0);
            const auto wf = p.chop.exportSlice (2), mf = p.chop.exportMidi (130.0);
            check (wf.existsAsFile() && wf.getSize() > 1000 && mf.existsAsFile(), "CHOP: drag a slice as WAV and the chop as MIDI");
            check (p.chopToPair (2, 3) && p.pairParents[3] != nullptr, "CHOP: a slice goes into PAIR");
            kk::SliceFx fx; fx.semi = 12; fx.rev = true; fx.pan = -1.0f;
            const int before = p.chop.slice (2).getNumSamples();
            p.chop.setFx (2, fx);
            const auto sb = p.chop.slice (2);
            check (std::abs (sb.getNumSamples() - before / 2) < 4 && sb.getMagnitude (1, 0, sb.getNumSamples()) < 1.0e-4f, "CHOP: per-slice pitch +12 halves it, pan hard left");
            p.chop.autoSlice (1, 120.0, 8);
            check (p.chop.current()->numSlices() >= 4, "CHOP: 1/8 grid at the project tempo");
            p.chop.autoSlice (-1);
            check (p.chop.current()->numSlices() >= 2, "CHOP: NOTES mode cuts");
            const auto ff = p.chop.flipMidi (140.0, 7);
            juce::MidiFile fm; juce::FileInputStream fis (ff);
            check (ff.existsAsFile() && fm.readFrom (fis) && fm.getTrack (0)->getNumEvents() > 8, "CHOP: FLIP makes a new MIDI pattern");
        }
        f.deleteFile();
    }
    // PAIR YOUR OWN: two of your sounds -> 6 children that play on the keys and in loops
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory);
        auto make = [&] (const char* name, float hz, bool saw)
        {
            auto f = tmp.getChildFile (name);
            juce::AudioBuffer<float> b (1, 44100 * 2);
            float ph = 0;
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                ph += hz / 44100.0f; ph -= std::floor (ph);
                const float v = saw ? 2.0f * ph - 1.0f : std::sin (kk::twoPi * ph) + 0.3f * std::sin (kk::twoPi * ph * 3.0f);
                b.setSample (0, i, 0.6f * v * std::exp (-(float) i / 30000.0f));
            }
            f.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), 44100, 1, 24, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            return f;
        };
        const auto fa = make ("kk_pair_a.wav", 261.63f, false), fb = make ("kk_pair_b.wav", 220.0f, true);
        check (p.loadPairParent (0, fa) && p.loadPairParent (1, fb), "PAIR loads your WAVs");
        check (p.pairParents[0]->pitched && p.pairParents[0]->rootNote == 60 && p.pairParents[1]->rootNote == 57, "PAIR detects their pitch (C5, A4)");
        int ok = 0;
        for (int fl = 0; fl < kk::numFlavors; ++fl)
        {
            p.pairFlavor = fl;
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            p.pairBreed();
            const auto ms = juce::Time::getMillisecondCounterHiRes() - t0;
            bool good = p.pairKids.size() == 6;
            for (auto& k : p.pairKids)
            {
                float pk = 0; bool fin = true;
                for (int c = 0; c < 2; ++c) for (int i = 0; i < k->audio.getNumSamples(); ++i) { const float v = k->audio.getSample (c, i); fin &= std::isfinite (v); pk = std::max (pk, std::abs (v)); }
                good &= fin && pk > 0.3f && pk < 0.95f && k->audio.getNumSamples() > 2000;
            }
            if (good) ++ok;
            std::printf ("PAIR flavor %d: 6 children in %.0f ms, e.g. %s\n", fl, ms, p.pairKids.empty() ? "-" : p.pairKids[0]->method.toRawUTF8());
        }
        check (ok == kk::numFlavors, "PAIR breeds 6 finite, loud children in every flavour");
        p.selectPairKid (0, false);
        p.togglePairLoop (0);
        juce::AudioBuffer<float> b (2, 512); float peak = 0;
        for (int k = 0; k < 300; ++k) { juce::MidiBuffer m; p.processBlock (b, m); peak = std::max (peak, b.getMagnitude (0, 512)); }
        check (peak > 0.05f && p.loopPlaying(), "PAIR loop plays the child");
        const auto wav = p.exportPairKid (0), mid = p.exportPairLoop();
        check (wav.existsAsFile() && mid.existsAsFile(), "PAIR drags out WAV and MIDI");
        fa.deleteFile(); fb.deleteFile();
    }
    // HARVEST: a "song" with bass, keys, pad, lead and drums -> sounds sorted into the bank
    {
        const double R = 44100.0;
        juce::AudioBuffer<float> song (2, (int) (R * 16)); song.clear();
        juce::Random rnd (5);
        auto addTone = [&] (double t0, double dur, float hz, float amp, float attack, float decayT, bool saw, float vib)
        {
            const int a = (int) (t0 * R), n = (int) (dur * R);
            float ph = 0;
            for (int i = 0; i < n && a + i < song.getNumSamples(); ++i)
            {
                const float t = (float) i / (float) R;
                const float f = hz * (1.0f + vib * std::sin (kk::twoPi * 5.5f * t));
                ph += f / (float) R; ph -= std::floor (ph);
                const float env = std::min (1.0f, t / attack) * std::exp (-t / decayT) * std::min (1.0f, (float) (n - i) / (0.01f * (float) R));
                const float v = saw ? (2.0f * ph - 1.0f) * 0.6f : std::sin (kk::twoPi * ph) + 0.25f * std::sin (kk::twoPi * ph * 2.0f);
                for (int c = 0; c < 2; ++c) song.addSample (c, a + i, v * env * amp);
            }
        };
        auto addHit = [&] (double t0, float amp, float decay)
        {
            const int a = (int) (t0 * R);
            for (int i = 0; i < (int) (R * 0.3); ++i) for (int c = 0; c < 2; ++c) song.addSample (c, a + i, (rnd.nextFloat() * 2 - 1) * amp * std::exp (-(float) i / (decay * (float) R)));
        };
        for (int k = 0; k < 4; ++k) addTone (0.0 + k * 1.0, 0.9, 55.0f * (k % 2 ? 1.5f : 1.0f), 0.6f, 0.005f, 2.0f, false, 0);   // bass
        for (int k = 0; k < 4; ++k) addTone (4.0 + k * 0.75, 0.7, 523.25f * (k % 2 ? 1.2f : 1.0f), 0.4f, 0.003f, 0.25f, false, 0);   // keys
        for (int k = 0; k < 2; ++k) addTone (7.2 + k * 2.0, 1.9, 330.0f, 0.35f, 0.25f, 30.0f, true, 0);   // pad
        for (int k = 0; k < 2; ++k) addTone (11.3 + k * 1.2, 1.1, 660.0f, 0.35f, 0.02f, 30.0f, true, 0.03f);   // lead (vibrato)
        for (int k = 0; k < 6; ++k) addHit (13.8 + k * 0.35, 0.5f, 0.03f);   // drums
        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_harvest_demo.wav");
        f.deleteFile();
        {
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), R, 2, 24, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (song, 0, song.getNumSamples());
        }
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        p.harvestFile (f);
        while (p.harvesting()) juce::Thread::sleep (10);
        p.moduleHousekeeping();
        int per[kk::numCats] {};
        for (auto& it : p.bank) ++per[it.cat];
        std::printf ("HARVEST: %d sounds in %.0f ms -", (int) p.bank.size(), juce::Time::getMillisecondCounterHiRes() - t0);
        for (int c = 0; c < kk::numCats; ++c) std::printf (" %s %d", kk::harvestCatShort (c), per[c]);
        std::printf ("\n");
        check (p.bank.size() >= 6, "HARVEST pulls sounds out of a song");
        check (per[kk::catBass] > 0 && per[kk::catDrum] > 0 && (per[kk::catKeys] + per[kk::catPluck]) > 0 && (per[kk::catPad] + per[kk::catLead] + per[kk::catVocal]) > 0,
               "HARVEST sorts bass / keys / pad-lead / drums");
        p.pairDice (0); p.pairDice (1); p.pairBreed();
        check (p.pairKids.size() == 6, "HARVEST bank breeds");
    }
    // PAIR FROM VST: host a VST3 instrument (KEYS KILLA itself here), capture a note into the bank
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        auto vst3 = juce::File::getCurrentWorkingDirectory().getChildFile ("build15/KeysKilla_artefacts/Release/VST3/KEYS KILLA.vst3");
        if (vst3.exists())
        {
            const auto err = p.loadVst (vst3.getFullPathName());
            check (err.isEmpty() && p.vst.loaded(), "VST host loads a VST3 instrument");
            std::printf ("VST: %s %s\n", p.vst.name().toRawUTF8(), err.toRawUTF8());
            const size_t before = p.bank.size();
            const auto name = p.captureVst (60);
            check (name.isNotEmpty() && p.bank.size() == before + 1, "VST capture lands in the bank");
            if (! p.bank.empty()) std::printf ("VST capture: %s -> %s\n", name.toRawUTF8(), kk::harvestCatShort (p.bank.front().cat));
            set (p, ID::playMode, (float) KeysKillaProcessor::playVst);
            juce::AudioBuffer<float> b (2, 512); float pk = 0;
            for (int k = 0; k < 60; ++k) { juce::MidiBuffer m; if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0); p.processBlock (b, m); pk = std::max (pk, b.getMagnitude (0, 512)); }
            check (pk > 0.01f, "the keys play the hosted plugin");
            // PAIR FROM VST, A + B: the plugin's sounds listed in KEYS KILLA, a click = the sound in A / B, BREED from the two
            check (p.loadVstSide (1, vst3.getFullPathName()).isEmpty() && p.vstSounds[1].size() > 10, "VST side B lists the plugin's sounds");
            check (p.pickVstSound (1, 7) && p.pairParents[1] != nullptr && p.pairParents[1]->name.contains (p.vstSounds[1][7].name), "VST side B: a sound from the list becomes SOUND B");
            check (p.pickVstSound (0, -1) && p.pairParents[0] != nullptr, "VST side A: the current sound becomes SOUND A");
            p.pairUse = 2; p.pairBreed();
            check (p.pairKids.size() == 6, "VST A x B breeds six children");
            const auto bankBefore = p.bank.size();
            check (p.vstSideToBank (1) && p.bank.size() == bankBefore + 1, "VST side B: SAVE TO BANK");
            // GRAB SOUNDS: the plugin's own presets, one by one, without its window
            check (p.vst.numPrograms() > 10, "the hosted plugin shares its preset list");
            const auto g5 = p.captureVstProgram (5, 60);
            check (g5.contains (p.vst.programNameAt (5)) && g5.isNotEmpty(), "GRAB: a preset of the plugin lands in the bank by its name");
            // clean the test capture out of the user's bank folder
            for (auto& it : p.bank) if (it.saved && it.origin == p.vst.name()) for (auto& f : KeysKillaProcessor::bankFolder().findChildFiles (juce::File::findFiles, true, juce::File::createLegalFileName (it.sound->name).substring (0, 80) + "*.wav")) f.deleteFile();
        }
        else std::printf ("VST: (no built VST3 to host - skipped)\n");
    }
    // FAMILY TREE / BREED LAB pictures never go silent when the keys play PAIR / VST / drums
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        set (p, ID::playMode, (float) KeysKillaProcessor::playPair);
        p.setAncestorCurrent (0); p.setAncestorPreset (1, 60);
        p.treeBreed(); while (p.renderNextThumbnail()) {}
        p.breed(); while (p.renderNextThumbnail()) {}
        float mx = 0; for (auto& c : p.treeKids()) for (float v : c.wave) mx = std::max (mx, v);
        check (mx > 0.5f, "waveform pictures are drawn even while the keys play PAIR");
        // BREED LAB LOOP mode: the child's play button starts its loop, a second press stops it
        set (p, ID::playMode, 0);
        p.mainLoopMode = true; p.playChild (1);
        check (p.loopIsChild (1) && p.loopPlaying(), "BREED LAB LOOP: a child plays its loop");
        p.playChild (1);
        check (! p.loopPlaying(), "BREED LAB LOOP: second press stops it");
    }
    // PAIR flavours change the children (same children, new flavour)
    {
        KeysKillaProcessor p (false); p.prepareToPlay (44100, 512);
        p.pairDice (0); p.pairDice (1); p.pairFlavor = 1; p.pairBreed();
        const auto clean = p.pairKids;
        p.pairFlavor = 2; p.pairBreed (false);
        int differ = 0;
        for (size_t k = 0; k < clean.size() && k < p.pairKids.size(); ++k) differ += clean[k]->audio.getNumSamples() != p.pairKids[k]->audio.getNumSamples() || clean[k]->method != p.pairKids[k]->method;
        check (differ == 6, "PAIR: a new flavour changes all six children");
        std::set<juce::String> methods; for (auto& k : clean) methods.insert (k->method);
        check (methods.size() >= 5, "PAIR: the six children are six different characters");
    }
    // MY SOUNDS: folders you create, sounds saved into them, rename, delete
    {
        KeysKillaProcessor p (false); p.prepareToPlay (44100, 512);
        const juce::String f1 = "kk test folder", f2 = "kk test folder 2";
        kk::Library::deleteFolder (f1); kk::Library::deleteFolder (f2);
        check (kk::Library::folders().contains (kk::Library::defaultFolder()), "MY SOUNDS: the default folder exists");
        check (kk::Library::createFolder (f1) && kk::Library::folders().contains (f1), "MY SOUNDS: create a folder");
        juce::AudioBuffer<float> b (2, 22050);
        for (int i = 0; i < b.getNumSamples(); ++i) { const float v = 0.5f * std::sin ((float) i * 0.05f) * std::exp (-(float) i / 8000.0f); b.setSample (0, i, v); b.setSample (1, i, v); }
        auto snd = kk::PairLab::fromBuffer (b, 44100.0, 44100.0, "kk test sound");
        const auto saved = p.saveToFolder (snd, f1);
        check (saved.existsAsFile() && kk::Library::sounds (f1).size() == 1 && p.lastFolder == f1, "MY SOUNDS: save a sound into a folder");
        check (kk::Library::renameFolder (f1, f2) && kk::Library::sounds (f2).size() == 1, "MY SOUNDS: rename a folder (the sounds move with it)");
        check (kk::Library::deleteSound (kk::Library::sounds (f2)[0]) && kk::Library::sounds (f2).isEmpty(), "MY SOUNDS: delete a sound");
        check (kk::Library::deleteFolder (f2) && ! kk::Library::folders().contains (f2), "MY SOUNDS: delete a folder");
    }
    // DRUM KIT: seven drum slots, the boosted sound saved into a kit folder (808s / Kicks / ... like a bought kit)
    {
        KeysKillaProcessor p (false); p.prepareToPlay (44100, 512);
        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_kick_test.wav");
        {
            juce::AudioBuffer<float> b (1, 22050);
            for (int i = 0; i < b.getNumSamples(); ++i) b.setSample (0, i, 0.8f * std::sin (kk::twoPi * (60.0f + 200.0f * std::exp (-(float) i / 900.0f)) * (float) i / 44100.0f) * std::exp (-(float) i / 6000.0f));
            f.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), 44100, 1, 24, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
        check (p.loadDrum (kk::slotKick, f) && p.drum (kk::slotKick).hasSample(), "KICK slot loads a drum");
        set (p, ID::playMode, (float) KeysKillaProcessor::modeOfDrum (kk::slotKick));
        juce::AudioBuffer<float> b (2, 512); float pk = 0;
        for (int k = 0; k < 20; ++k) { juce::MidiBuffer m; if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, p.drum (kk::slotKick).current()->rootNote, (juce::uint8) 110), 0); p.processBlock (b, m); pk = std::max (pk, b.getMagnitude (0, 512)); }
        check (pk > 0.1f, "the keys play the KICK slot");
        const juce::String kit = "kk test kit";
        kk::Kits::kit (kit).deleteRecursively();
        const auto saved = p.saveDrumToKit (kk::slotKick, kit);
        check (saved.existsAsFile() && saved.getParentDirectory().getFileName() == "Kicks" && kk::Kits::count (kit) == 1, "SAVE TO KIT: the kick lands in <kit>/Kicks");
        kk::Kits::kit (kit).deleteRecursively(); f.deleteFile();
    }
    // shelves: the preset name decides first
    check (kk::harvestCatFromName ("Surge XT Brass Stab C5") == kk::catBrass && kk::harvestCatFromName ("Lush Strings") == kk::catStrings
           && kk::harvestCatFromName ("Init Saw") < 0, "BANK: brass / strings found by the preset name");
    // KEYS KILLA sees the plugins FL Studio found (its plugin database), instruments first
    {
        const auto il = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("Image-Line");
        if (! il.exists())
        {
            auto fake = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_fake_plugins").getChildFile ("Fake Synth.vst3");
            fake.createDirectory();
            auto nfo = il.getChildFile ("FL Studio/Presets/Plugin database/Installed/Generators/VST3/Fake Synth.nfo");
            nfo.getParentDirectory().createDirectory();
            nfo.replaceWithText ("ps_file_name_0=Fake Synth\nps_file_filename_0=" + fake.getFullPathName() + "\n");
            kk::VstHost h;
            const auto list = h.listInstalled();
            check (list.size() > 0 && list[0] == fake.getFullPathName(), "VST list includes FL Studio's plugin database, instruments first");
            il.deleteRecursively(); fake.getParentDirectory().deleteRecursively();
        }
    }
    // v0.16 SOUND WORLDS, TRANCE GATE, CLIPPER
    {
        KeysKillaProcessor p; p.setCurrentProgram (3); p.prepareToPlay (44100, 512);
        juce::AudioBuffer<float> b (2, 512);
        auto render = [&] (double& rms, float& peak, std::vector<float>* out)
        {
            double e = 0; peak = 0; bool finite = true;
            for (int k = 0; k < 160; ++k)
            {
                juce::MidiBuffer m;
                if (k == 0) for (int nn : { 60, 63, 67 }) m.addEvent (juce::MidiMessage::noteOn (1, nn, (juce::uint8) 100), 0);
                if (k == 120) m.addEvent (juce::MidiMessage::allNotesOff (1), 0);
                p.processBlock (b, m);
                for (int i = 0; i < 512; ++i)
                {
                    const float v = b.getSample (0, i); finite &= std::isfinite (v) && std::isfinite (b.getSample (1, i));
                    if (k < 120) e += v * v; peak = std::max (peak, std::abs (v)); if (out) out->push_back (v);
                }
            }
            juce::MidiBuffer none; for (int k = 0; k < 80; ++k) p.processBlock (b, none);
            rms = 10.0 * std::log10 (e / (120.0 * 512.0) + 1e-12);
            return finite;
        };
        double r0; float pk; std::vector<float> dry;
        render (r0, pk, &dry);
        bool ok = true; int changed = 0;
        for (int w = 1; w < kk::numWorlds; ++w)
        {
            set (p, ID::world, (float) w);
            double r; std::vector<float> wet;
            ok &= render (r, pk, &wet);
            double diff = 0; for (size_t i = 0; i < std::min (dry.size(), wet.size()); ++i) diff += std::abs (wet[i] - dry[i]);
            if (diff > 5.0) ++changed;
            if (std::abs (r - r0) > 4.0) { std::printf ("!! world %s level %.1f dB vs %.1f dB\n", kk::WorldStage::name (w), r, r0); ++fails; }
        }
        check (ok, "SOUND WORLDS stay finite");
        check (changed == kk::numWorlds - 1, "every SOUND WORLD changes the sound");
        set (p, ID::world, 0);
        render (r0, pk, nullptr);   // fresh dry reference
        set (p, ID::gate, 2);
        double rg; render (rg, pk, nullptr);
        std::printf ("GATE: %.1f dB vs dry %.1f dB\n", rg, r0);
        check (rg < r0 - 1.5, "TRANCE GATE chops the sound");
        set (p, ID::gate, 0);
        for (int c = 1; c <= 3; ++c)
        {
            set (p, ID::clipMode, (float) c); set (p, ID::clipDrive, 12.0f);
            double rc; check (render (rc, pk, nullptr) && pk <= 0.97f, "CLIPPER keeps the ceiling");
        }
        set (p, ID::clipMode, 0);
        std::printf ("WORLDS: %d of %d change the sound, all within 4 dB of the dry level\n", changed, kk::numWorlds - 1);
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
        bool fitOk = true;   // beat 1 of bar 1 is a tone of the root chord
        for (uint32_t sd = 1; sd <= 500; ++sd)
        {
            const auto x = kk::loopFromSeed (sd);
            for (auto& n : kk::buildLoop (x, 0, 8, false, true)) if (n.start == 0.0f) fitOk &= kk::loopdata::chordTone (n.note, 0);
        }
        check (fitOk, "melodies start on a chord tone");
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
        std::set<std::vector<int>> firstBars;
        for (auto& r : p.treeKids()) { std::vector<int> v; for (auto& n : kk::buildLoop (r.g.loop, 0, 8, false, true)) if (n.start < 4.0f) v.push_back (n.note * 100 + (int) (n.start * 4)); firstBars.insert (v); }
        check (firstBars.size() == 6, "six different first bars in one BREED");
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
    std::setvbuf (stdout, nullptr, _IONBF, 0);   // CI: every line reaches the log even if the process dies
    std::printf ("KEYS KILLA tests: start\n");
    juce::ScopedJuceInitialiser_GUI init;
    const bool verbose = argc > 1 && juce::String (argv[1]) == "-v";
    std::printf ("KEYS KILLA tests: juce ready\n");
    KeysKillaProcessor p;
    std::printf ("KEYS KILLA tests: processor ready (%d KB)\n", (int) (sizeof (KeysKillaProcessor) / 1024));
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
            auto setp = [&] (const char* id, float v) { auto* q = proc->apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); };
            switch (action++ % 17)
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
                case 12: ke->showView (15 + action % 6); setp (ID::playMode, (float) (action % 2)); break;
                case 13: setp (ID::efxOn, (float) (action % 2)); break;
                case 14: proc->generatePattern (action % 3, action % 4, 1 << (action % 3), 0.5f); proc->patPlay = action % 2 ? -1 : action % 3; break;
                case 15: setp (ID::halfOn, (float) (action % 2)); break;
                case 16: ke->showView (15 + action % 6); break;
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
