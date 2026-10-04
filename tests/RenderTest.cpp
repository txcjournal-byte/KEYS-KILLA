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


static int libraryCheck (KeysKillaProcessor& p, bool verbose)
{
    // v0.30 SOUND LIBRARY: every new sound is clean (tail goes silent - no hiss, hum or endless noise), stereo, no clipping, no DC
    int newSounds = 0, dirty = 0;
    for (int i = 0; i < p.getNumPrograms(); ++i)
    {
        const auto& pr = factoryPresets()[(size_t) i];
        if (pr.version != "0.30") continue;
        ++newSounds;
        p.setCurrentProgram (i); p.prepareToPlay (44100, 441);
        const bool low = pr.isBass() || pr.cat == cDrums || pr.cat == cChip;
        const int root = pr.isBass() ? 36 : 60;
        juce::AudioBuffer<float> b (2, 441);
        double mid = 0, side = 0, tail = 0, dcSum = 0; float pk = 0; long dcN = 0; bool fin = true;
        for (int k = 0; k < 1100; ++k)   // 11 s: 1.2 s of notes, then the release, reverb and echo must die away
        {
            juce::MidiBuffer m;
            if (k == 0) { m.addEvent (juce::MidiMessage::noteOn (1, root, (juce::uint8) 100), 0); if (! pr.isBass()) m.addEvent (juce::MidiMessage::noteOn (1, root + 7, (juce::uint8) 100), 3); }
            if (k == 120) { m.addEvent (juce::MidiMessage::noteOff (1, root), 0); m.addEvent (juce::MidiMessage::noteOff (1, root + 7), 0); }
            p.processBlock (b, m);
            for (int j = 0; j < 441; ++j)
            {
                const float l = b.getSample (0, j), r = b.getSample (1, j);
                fin &= std::isfinite (l) && std::isfinite (r);
                pk = std::max (pk, std::max (std::abs (l), std::abs (r)));
                if (k < 120) { mid += (l + r) * (l + r) * 0.25; side += (l - r) * (l - r) * 0.25; dcSum += l + r; dcN += 2; }
                if (k >= 1050) tail += (double) l * l + (double) r * r;
            }
        }
        const float tailDb = (float) juce::Decibels::gainToDecibels (std::sqrt (tail / (50.0 * 441 * 2)), -150.0);
        const float stereoDb = (float) juce::Decibels::gainToDecibels (std::sqrt (side / std::max (1e-12, mid)), -150.0);
        const float dcv = (float) (dcSum / std::max (1L, dcN));
        const bool ok = fin && pk <= 1.0f && tailDb < -80.0f && std::abs (dcv) < 0.02f && (low || stereoDb > -30.0f);
        if (! ok || verbose)
            std::printf ("%s NEW %-28s peak %.2f  tail %6.1f dB  stereo %6.1f dB  dc %+.4f\n", ok ? "  " : "!!", pr.name.toRawUTF8(), pk, tailDb, stereoDb, dcv);
        if (! ok) ++dirty;
    }
    std::printf ("SOUND LIBRARY: %d new sounds, %d not clean\n", newSounds, dirty);
    return (dirty > 0 || newSounds < 100) ? 1 : 0;
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
    // v0.32: KEYS KILLA alone (VOODOO / EFFECTOR / DIGGA KILLA are separate plugins); old projects still load
    {
        KeysKillaProcessor p; p.setCurrentProgram (3); p.prepareToPlay (44100, 512);
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
        check (render (pk, dry) && pk > 0.01f, "melody plays");
        set (p, ID::efxOn, 1); set (p, ID::halfOn, 1); set (p, ID::playMode, 1);   // an old project with HALF / EFFECTOR / DIGGA on
        check (render (pk, en) && pk > 0.01f && pk <= 1.0f, "old project: the keys play the KEYS KILLA sound");
        set (p, ID::efxOn, 0); set (p, ID::halfOn, 0); set (p, ID::playMode, 0);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (q.getLatencySamples() == 0, "no latency, state restores");
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
        // v0.32: kick, open hat, perc and FX patterns - every drum page generates its own trap patterns
        for (int kind = 0; kind < 4; ++kind)
        {
            std::set<std::vector<int>> seenK; bool inK = true; size_t hits = 0;
            for (int sd = 1; sd <= 60; ++sd)
                for (int st = 0; st < 4; ++st)
                {
                    const auto r = kind == 0 ? kk::makeKicks ((uint32_t) sd, st, 2, 0.6f) : kind == 1 ? kk::makeOpenHats ((uint32_t) sd, st, 2, 0.6f)
                                 : kind == 2 ? kk::makePercs ((uint32_t) sd, st, 2, 0.6f) : kk::makeFxHits ((uint32_t) sd, st, 2, 0.6f);
                    std::vector<int> sig; for (auto& h : r) { sig.push_back ((int) std::lround (h.beat * 96) * 100 + h.semi); inK &= h.beat >= 0 && h.beat + h.len <= 8.0 + 1e-9; }
                    seenK.insert (sig); hits += r.size();
                }
            static const char* nm[] { "KICK", "OPEN HAT", "PERC", "FX" };
            check (inK && hits > 0 && seenK.size() > (kind == 3 ? 4u : 60u), (juce::String (nm[kind]) + " patterns: varied, inside the bars").toRawUTF8());
        }
        {
            KeysKillaProcessor kp; kp.prepareToPlay (44100, 512);
            bool all = true;
            for (int d = 0; d < kk::numDrumSlots; ++d) { kp.generatePattern (d, d % 4, 2, 0.6f); all &= ! kp.pattern (d).empty() && kp.exportPatternMidi (d).existsAsFile(); }
            check (all, "DRUM KIT: all 7 drums generate a pattern and drag it out as MIDI");
        }
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
    // SAMPLER / CHOP: your sample is loaded, cut, played, dragged out, sent to PAIR and remembered
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_sampler_loop.wav");
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
        {
            // CHOP / SLICE: the sample in slices, pads, keys, drag out, PAIR
            check (p.chopLoadFile (f) && p.chop.current() != nullptr && p.chop.current()->numSlices() >= 4, "SAMPLER: your sample loads and is cut at the hits");
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
            {   // v0.35 SAMPLER tools: selection as WAV / loop / parent, MUTATE / KILL keep the length and some of the original, UNDO, CLEAR
                const auto src0 = p.chop.current()->src;
                const int L = src0->getNumSamples();
                const auto rf = p.exportChopRegion (L / 4, L / 2, true);
                check (rf.existsAsFile() && rf.getSize() > 2000, "SAMPLER: the selected part drags out as a WAV loop");
                check (p.chopRegionToParent (L / 4, L / 2, 1) && p.labWav[1] && p.labAudioMode(), "SAMPLER: the selected part becomes a BREED LAB parent");
                p.clearParent (1);
                check (p.chopMutate (L / 4, L / 2, false), "SAMPLER: MUTATE the selection");
                const auto m1 = p.chop.current()->src;
                double num = 0, d1 = 0, d2 = 0; float diff = 0;
                for (int i = L / 4; i < L / 2; ++i) { const float x = src0->getSample (0, i), y = m1->getSample (0, i); num += x * y; d1 += x * x; d2 += y * y; diff = std::max (diff, std::abs (x - y)); }
                const double corr = num / std::sqrt (d1 * d2 + 1e-12);
                bool outsideSame = true; for (int i = 0; i < L / 4; i += 97) outsideSame &= src0->getSample (0, i) == m1->getSample (0, i);
                check (m1->getNumSamples() == L && diff > 0.05f && corr > 0.05 && corr < 0.97 && outsideSame, "SAMPLER: MUTATE changes the part, keeps a trace of it, the rest untouched");
                check (p.chopMutate (0, 0, true) && p.chop.current()->src->getNumSamples() == L, "SAMPLER: KILL the whole sample");
                check (p.chopUndoMutate() && p.chopUndoMutate() && p.chop.current()->src == src0, "SAMPLER: UNDO goes back to the original");
                p.chop.playRegion (100, 20000, true);
                juce::AudioBuffer<float> rb (2, 512); float rpk = 0;
                for (int k = 0; k < 60; ++k) { juce::MidiBuffer m; p.processBlock (rb, m); rpk = std::max (rpk, rb.getMagnitude (0, 512)); }
                check (rpk > 0.02f && p.chop.regionPlaying(), "SAMPLER: the selection plays as a loop");
                p.chop.stopAll(); { juce::MidiBuffer m; p.processBlock (rb, m); }
                check (! p.chop.regionPlaying(), "SAMPLER: SPACE / STOP stops it");
            }
            p.chop.autoSlice (1, 120.0, 8);
            check (p.chop.current()->numSlices() >= 4, "CHOP: 1/8 grid at the project tempo");
            p.chop.autoSlice (-1);
            check (p.chop.current()->numSlices() >= 2, "CHOP: NOTES mode cuts");
            const auto ff = p.chop.flipMidi (140.0, 7);
            juce::MidiFile fm; juce::FileInputStream fis (ff);
            check (ff.existsAsFile() && fm.readFrom (fis) && fm.getTrack (0)->getNumEvents() > 8, "CHOP: FLIP makes a new MIDI pattern");
            const int cuts = p.chop.current()->numSlices();
            juce::MemoryBlock mb; p.getStateInformation (mb);
            KeysKillaProcessor q; q.setStateInformation (mb.getData(), (int) mb.getSize());
            check (q.chop.current() != nullptr && q.chop.current()->numSlices() == cuts, "SAMPLER: the project remembers your sample and your cuts");
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
    // v0.37 EVOLVE: the sound on the keys, GENES for your sounds, sounds on C, SAMPLE EDIT, STEP FX, FLIPS, musical KILL
    {
        auto makeWav = [] (const char* name, float hz, bool stereoSaw)
        {
            auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile (name);
            juce::AudioBuffer<float> b (2, 44100);
            float ph = 0;
            for (int i = 0; i < b.getNumSamples(); ++i)
            {
                ph += hz / 44100.0f; ph -= std::floor (ph);
                const float v = stereoSaw ? 2.0f * ph - 1.0f : std::sin (kk::twoPi * ph);
                b.setSample (0, i, 0.5f * v * std::exp (-(float) i / 25000.0f));
                b.setSample (1, i, 0.5f * (stereoSaw ? -v : v) * std::exp (-(float) i / 20000.0f));
            }
            f.deleteFile();
            juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), 44100, 2, 24, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            return f;
        };
        auto renderFor = [] (KeysKillaProcessor& p, int blocks, int note, std::vector<float>* outL = nullptr)
        {
            juce::AudioBuffer<float> b (2, 512); float peak = 0; bool fin = true;
            for (int k = 0; k < blocks; ++k)
            {
                juce::MidiBuffer m;
                if (note >= 0 && k == 0) m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
                if (note >= 0 && k == blocks / 2) m.addEvent (juce::MidiMessage::noteOff (1, note), 0);
                p.processBlock (b, m);
                for (int c = 0; c < 2; ++c) for (int i = 0; i < 512; ++i) { const float v = b.getSample (c, i); fin &= std::isfinite (v); peak = std::max (peak, std::abs (v)); }
                if (outL) outL->insert (outL->end(), b.getReadPointer (0), b.getReadPointer (0) + 512);
            }
            return fin ? peak : -1.0f;
        };
        const auto fa = makeWav ("kk37_a.wav", 233.08f, false), fb = makeWav ("kk37_b.wav", 196.0f, true);   // A#3, G3: not on C
        KeysKillaProcessor p; p.setCurrentProgram (0); p.prepareToPlay (44100, 512);
        check (p.labDropFile (0, fa) && p.labDropFile (1, fb), "v0.37: two WAVs into BREED LAB");
        p.labBreedAudio();
        check (p.pairKids.size() == 6 && p.pairKidGenes.size() == 6, "v0.37: your sounds breed 6 children with genes");
        int onC = 0, pitched = 0; bool stereoOk = true;
        for (auto& k : p.pairKids) { if (k->pitched) { ++pitched; onC += k->rootNote % 12 == 0 ? 1 : 0; } stereoOk &= k->audio.getNumChannels() == 2; }
        check (pitched == 0 || onC == pitched, "v0.37: every pitched child sits on C (layers in FL's channel rack)");
        std::printf ("TUNE: %d of %d pitched children on C\n", onC, pitched);
        check ((int) p.apvts.getRawParameterValue (ID::playMode)->load() == KeysKillaProcessor::playPair, "v0.37: an audio child takes the keys");
        // GENES on your sounds: A/B splices a new child
        p.setChildGene (0, KeysKillaProcessor::geneBody, 1 - p.pairKidGenes[0][0]);
        check (p.pairKids[0]->method.startsWith ("GENES") && p.geneOfSelected (0) == p.pairKidGenes[0][0], "v0.37: a GENE click splices the audio child");
        {
            const auto& a = p.pairKids[0]->audio;
            float pk = a.getMagnitude (0, a.getNumSamples()); bool fin = true; double side = 0;
            for (int i = 0; i < a.getNumSamples(); ++i) { fin &= std::isfinite (a.getSample (0, i)) && std::isfinite (a.getSample (1, i)); side += std::abs (a.getSample (0, i) - a.getSample (1, i)); }
            check (fin && pk > 0.3f && pk < 0.95f, "v0.37: the spliced child is clean and loud");
            check (side > 1.0, "v0.37: the spliced child is stereo");
        }
        // the keys play it; the knobs colour it (SPACE up = a longer, wider tail)
        const float dry = renderFor (p, 120, 60);
        check (dry > 0.05f, "v0.37: the audio child plays on the keys");
        std::vector<float> a0, a1;
        renderFor (p, 40, -1); renderFor (p, 160, 60, &a0);
        set (p, ID::m2, 1.0f); set (p, ID::m3, 0.6f);
        renderFor (p, 40, -1); renderFor (p, 160, 60, &a1);
        double tail0 = 0, tail1 = 0;
        for (size_t i = a0.size() * 3 / 4; i < a0.size(); ++i) tail0 += std::abs (a0[i]);
        for (size_t i = a1.size() * 3 / 4; i < a1.size(); ++i) tail1 += std::abs (a1[i]);
        check (tail1 > tail0 * 1.5 + 1.0e-3, "v0.37: SPACE / DIRT knobs work on your sound");
        std::printf ("KNOBS ON SAMPLES: tail %.3f -> %.3f\n", tail0, tail1);
        // a bank child takes the keys back
        p.breed(); p.selectChild (1);
        check ((int) p.apvts.getRawParameterValue (ID::playMode)->load() == KeysKillaProcessor::playKeys, "v0.37: a bank child takes the keys back");
        // the audio loop plays
        p.labBreedAudio();
        p.togglePairLoop (2);
        const float loopPk = renderFor (p, 400, -1);
        check (p.loopPlaying() && loopPk > 0.02f, "v0.37: BREED LAB LOOP plays your audio child");
        p.stopLoop(); renderFor (p, 60, -1);
        // SAMPLE EDIT: tune / reverse / start, rendered for SAVE / DRAG
        p.useSample (p.pairKids[0], false);
        p.sampleEdit[KeysKillaProcessor::seReverse] = 1.0f; p.sampleEdit[KeysKillaProcessor::seTune] = 12.0f; p.sampleEdit[KeysKillaProcessor::seSpace] = 0.5f;
        auto ed = p.editedSample();
        check (ed != nullptr && ed->audio.getNumSamples() > 1000 && ed->audio.getMagnitude (0, ed->audio.getNumSamples()) > 0.1f, "v0.37: SAMPLE EDIT renders the edited sound");
        const float edPk = renderFor (p, 100, 60);
        check (edPk > 0.02f, "v0.37: SAMPLE EDIT plays on the keys (reversed, tuned)");
        // state round trip: SAMPLE EDIT, STEP FX, the sample on the keys
        p.stepPreset (2); p.stepOn = true; p.stepRate = 2;
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor q; q.prepareToPlay (44100, 512); q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (std::abs (q.sampleEdit[KeysKillaProcessor::seTune].load() - 12.0f) < 0.01f && q.stepOn.load() && q.stepRate.load() == 2 && q.stepToString() == p.stepToString(),
               "v0.37: SAMPLE EDIT and STEP FX come back with the project");
        check (q.activeSample() != nullptr && (int) q.apvts.getRawParameterValue (ID::playMode)->load() == KeysKillaProcessor::playPair, "v0.37: the sample on the keys comes back with the project");
        // STEP FX change the sound in time, stay finite
        {
            KeysKillaProcessor s; s.setCurrentProgram (5); s.prepareToPlay (44100, 512);
            std::vector<float> o0, o1;
            renderFor (s, 200, 60, &o0);
            s.panic(); renderFor (s, 100, -1);
            for (int f = 0; f < KeysKillaProcessor::numStepFx; ++f) for (int st = 0; st < KeysKillaProcessor::numSteps; ++st) s.stepGrid[(size_t) f][(size_t) st] = (st + f) % 3 == 0;
            s.stepOn = true;
            const float pk = renderFor (s, 200, 60, &o1);
            double diff = 0; for (size_t i = 0; i < std::min (o0.size(), o1.size()); ++i) diff += std::abs (o0[i] - o1[i]);
            check (pk > 0.0f && pk <= 1.0f && diff > 1.0, "v0.37: STEP FX change the sound and stay clean");
            int fxOn = 0; for (int k = 0; k < 6; ++k) { auto g = s.fxSurprise ((uint32_t) k * 77u); for (auto o : g.on) fxOn += o ? 1 : 0; }
            check (fxOn >= 12, "v0.37: SURPRISE FX makes chains with effects in them");
            auto g0 = s.fxSurprise (5); auto g1 = s.fxMutate (g0, 0.5f, 9); s.fxApply (g1);
            check (s.fxCurrent().name.isEmpty() && s.rack.anyOn(), "v0.37: an FX chain goes onto the rack");
            const float rk = renderFor (s, 200, 60);
            check (rk > 0.0f && rk <= 1.0f, "v0.37: FX chains stay clean");
        }
        // SAMPLER: MONO pads, FLIPS, a musical KILL
        {
            KeysKillaProcessor c; c.prepareToPlay (44100, 512);
            check (c.chopLoadFile (makeWav ("kk37_chop.wav", 110.0f, true)), "v0.37: SAMPLER loads");
            c.chop.autoSlice (8);
            c.flipSeed();
            check (c.flips.size() == 7 && c.flipCenter == 0 && c.flips[0].kids.size() == 6, "v0.37: FLIPS: a seed and 6 children");
            c.flipGrow (c.flips[0].kids[3], false, 0.8f);
            check (c.flips.size() == 13, "v0.37: FLIPS grow from a child");
            c.flipPlay (c.flips[0].kids[1]);
            const float fpk = renderFor (c, 300, -1);
            check (fpk > 0.02f && c.flipStepNow.load() >= 0, "v0.37: a FLIP plays the chops in time");
            c.flipPlay (-1);
            check (c.flipMidiFile (0).existsAsFile() && c.flipWavFile (0).existsAsFile(), "v0.37: FLIP drags out as MIDI and WAV");
            auto src = c.chop.current()->src;
            const float rmsBefore = src->getRMSLevel (0, 0, src->getNumSamples());
            check (c.chopMutate (0, 0, true), "v0.37: KILL runs");
            auto after = c.chop.current()->src;
            const float rmsAfter = after->getRMSLevel (0, 0, after->getNumSamples());
            bool fin = true; for (int i = 0; i < after->getNumSamples(); ++i) fin &= std::isfinite (after->getSample (0, i));
            check (fin && after->getNumSamples() == src->getNumSamples() && rmsAfter > rmsBefore * 0.4f && rmsAfter < rmsBefore * 2.5f, "v0.37: KILL keeps the length and the loudness (musical, not broken)");
            // MONO pads: the second pad stops the first
            c.chopChoke = true;
            c.chopPad = 0; renderFor (c, 4, -1); c.chopPad = 3; renderFor (c, 8, -1);
            check (c.chop.lastHit.load() == 3 || c.chop.lastHit.load() == -1, "v0.37: MONO pads");
            c.chopClear();
        }
        for (auto f : { fa, fb }) f.deleteFile();
    }
    // v0.38 EVOLVE IDEA MODE + MY TASTE
    {
        KeysKillaProcessor p; p.setCurrentProgram (0); p.prepareToPlay (44100, 512);
        const auto keepTaste = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("taste.txt").loadFileAsString();
        p.tasteLoaded = true; p.taste = {};
        p.evoWild = 0.5f; p.evoSeedPreset (10);
        const auto& kidsIdx = p.evo[0].kids;
        int withFx = 0, withLoop = 0;
        for (int k : kidsIdx) { const auto& n = p.evo[(size_t) k]; for (auto o : n.fx.on) if (o) { ++withFx; break; } withLoop += n.g.loop.valid ? 1 : 0; }
        check (kidsIdx.size() == 6 && withFx >= 3, "v0.38 IDEA: children carry effects");
        const int kid = kidsIdx[3];
        auto idea = p.renderIdea (kid, 44100.0);
        check (idea.getNumSamples() > 44100 && idea.getMagnitude (0, idea.getNumSamples()) > 0.05f && idea.getMagnitude (0, idea.getNumSamples()) <= 1.0f,
               "v0.40 IDEA: the sound renders through its effects");
        check (p.evoExportIdea (kid).existsAsFile(), "v0.38 IDEA: the idea drags out as a WAV");
        // MY TASTE: when it knows what you like, the six it keeps are closer to it
        const auto target = p.evoDescribe (p.evo[(size_t) kidsIdx[1]]);
        auto meanDist = [&] (float amt)
        {
            double sum = 0; int cnt = 0;
            for (int rep = 0; rep < 6; ++rep)
            {
                p.taste = {}; p.taste.like = target; p.taste.nLike = 12; p.evoTasteAmt = amt;
                p.evoGrow (0, true);
                for (int k : p.evo[0].kids)
                {
                    const auto d = p.evoDescribe (p.evo[(size_t) k]);
                    double s2 = 0; for (int q = 0; q < 8; ++q) s2 += (d[(size_t) q] - target[(size_t) q]) * (d[(size_t) q] - target[(size_t) q]);
                    sum += std::sqrt (s2); ++cnt;
                }
            }
            return sum / std::max (1, cnt);
        };
        const double dAny = meanDist (0.0f), dTaste = meanDist (1.0f);
        std::printf ("MY TASTE: distance to your taste %.3f (ANY) -> %.3f (MY TASTE)\n", dAny, dTaste);
        check (dTaste < dAny, "v0.38 MY TASTE: the children move towards what you like");
        p.taste = {};
        p.evoPick (p.evo[0].kids[2]);
        check (p.tastePicks() >= 1 && p.taste.nDislike > 0.5f, "v0.38 MY TASTE: a pick is learned (and the others a little less)");
        // the ideas come back with the project
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor q; q.prepareToPlay (44100, 512); q.setStateInformation (mb.getData(), (int) mb.getSize());
        bool fxSame = q.evo.size() == p.evo.size();
        for (size_t i = 0; fxSame && i < p.evo.size(); ++i) fxSame = p.evo[i].fx.on == q.evo[i].fx.on;
        check (fxSame && q.evoIdea == p.evoIdea, "v0.38 IDEA: the effects of every idea come back with the project");
        auto tf = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("taste.txt");
        if (keepTaste.isEmpty()) tf.deleteFile(); else tf.replaceWithText (keepTaste);
    }
    // v0.39 EVOLVE: LAYERS, BEAT, POCKET + crosses, ALIVE / CATCH, MAP data, WORLDS
    {
        KeysKillaProcessor p; p.setCurrentProgram (0); p.prepareToPlay (44100, 512);
        const auto tf = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory).getChildFile ("KEYS KILLA").getChildFile ("taste.txt");
        const auto keepTaste = tf.loadFileAsString();
        p.tasteLoaded = true; p.taste = {}; p.evoTasteAmt = 0.0f;
        p.evoSeedPreset (20);
        const auto c = p.evo[(size_t) p.evoCenter];
        // only the EFFECTS: same sound, other effect chains
        p.evoLayer = KeysKillaProcessor::layerFx; p.evoGrow (p.evoCenter, true);
        int sameSound = 0, otherFx = 0;
        for (int k : p.evo[(size_t) p.evoCenter].kids) { const auto& n = p.evo[(size_t) k]; sameSound += n.g.v == c.g.v; otherFx += n.fx.on != c.fx.on || n.fx.v != c.fx.v; }
        check (sameSound == 6 && otherFx >= 5, "v0.40 LAYERS: FX changes only the effects");
        p.evoLayer = KeysKillaProcessor::layerAll; p.evoGrow (p.evoCenter, true);
        // the idea (sound + FX) drags out as a one-shot
        const auto iw = p.evoExportIdea (p.evo[(size_t) p.evoCenter].kids[0]);
        check (iw.existsAsFile() && iw.getSize() > 10000, "v0.40: SOUND + FX drags out as a WAV");
        // POCKET + a cross between two trees
        p.evoPocketAdd (p.evo[(size_t) p.evoCenter].kids[2]);
        check (p.pocket.size() == 1, "v0.39 POCKET keeps an idea");
        p.evoSeedPreset (300);   // another tree
        check (p.pocket.size() == 1, "v0.39 POCKET stays when a new tree starts");
        const int before = (int) p.evo.size();
        p.evoCross (p.pocket[0], p.evoCenter);
        check ((int) p.evo.size() > before && p.evo[(size_t) p.evoCenter].name.contains (" x ") && p.evo[(size_t) p.evoCenter].kids.size() == 6, "v0.39 CROSS: two trees make a hybrid that grows");
        // ALIVE drifts the sound a little, CATCH keeps the moment
        const auto v0 = p.genomeFromCurrentPublic();
        for (int i = 0; i < 6; ++i) p.evoAliveStep (0.6f);
        const auto v1 = p.genomeFromCurrentPublic();
        float moved = 0; for (size_t i = 0; i < v0.v.size(); ++i) moved += std::abs (v0.v[i] - v1.v[i]);
        check (moved > 0.01f, "v0.39 ALIVE: the idea changes by itself");
        const int caughtFrom = p.evoCenter;
        p.evoCatch();
        check (p.evo[(size_t) p.evoCenter].parent == caughtFrom && p.evo[(size_t) p.evoCenter].name.startsWith ("Caught"), "v0.39 CATCH: the moment becomes the middle");
        // WORLD file round trip (and the project keeps the POCKET)
        auto wf = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_world.evolve");
        check (p.evoSaveWorld (wf), "v0.39 WORLD saves");
        KeysKillaProcessor q; q.prepareToPlay (44100, 512);
        check (q.evoLoadWorld (wf) && q.evo.size() == p.evo.size() && q.evoCenter == p.evoCenter && q.pocket.size() == 1, "v0.39 WORLD opens with every idea, the middle and the POCKET");
        bool sameWay = true; for (size_t i = 0; i < p.evo.size() && i < q.evo.size(); ++i) sameWay &= p.evo[i].picked == q.evo[i].picked && p.evo[i].fx.on == q.evo[i].fx.on;
        check (sameWay, "v0.39 WORLD keeps the effects and your way");
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor r2; r2.prepareToPlay (44100, 512); r2.setStateInformation (mb.getData(), (int) mb.getSize());
        check (r2.pocket.size() == 1 && r2.evo.size() == p.evo.size(), "v0.39: the project keeps the tree and the POCKET");
        wf.deleteFile();
        if (keepTaste.isEmpty()) tf.deleteFile(); else tf.replaceWithText (keepTaste);
    }
    // v0.40 MELODY: in the key, singable, variations, your melody (key found), LISTEN from FL, plays with the sound, MIDI out
    {
        using namespace kk::mel;
        int outOfKey = 0, wideRange = 0; double meanInt = 0; int cnt = 0;
        for (int sc = 0; sc < numScales; ++sc)
            for (int k = 0; k < 12; k += 5)
            {
                Style st; auto m = generate (100u + (uint32_t) (sc * 31 + k), k, sc, 8, st);
                const auto& steps = scaleSteps (sc);
                int lo = 127, hi = 0;
                for (size_t i = 0; i < m.notes.size(); ++i)
                {
                    const int pc = ((m.notes[i].pitch - k) % 12 + 12) % 12;
                    outOfKey += std::find (steps.begin(), steps.end(), pc) == steps.end();
                    lo = std::min (lo, m.notes[i].pitch); hi = std::max (hi, m.notes[i].pitch);
                    if (i) { meanInt += std::abs (m.notes[i].pitch - m.notes[i - 1].pitch); ++cnt; }
                }
                wideRange += hi - lo > 26;
                for (int v = 0; v < 8; ++v)
                {
                    auto c = vary (m, v, 55u + (uint32_t) v, 0.5f, st);
                    for (auto& n : c.notes) { const int pc = ((n.pitch - k) % 12 + 12) % 12; outOfKey += std::find (steps.begin(), steps.end(), pc) == steps.end(); }
                }
            }
        std::printf ("MELODY: mean step %.2f semitones, %d melodies wider than 2 octaves, %d notes out of key\n", meanInt / std::max (1, cnt), wideRange, outOfKey);
        check (outOfKey == 0, "v0.40 MELODY: every note (and every child's note) is in the key");
        check (meanInt / std::max (1, cnt) < 5.0 && wideRange <= 3, "v0.40 MELODY: singable lines (small steps, not too wide)");
        Style st; auto m = generate (7, 9, scMinor, 8, st);
        int differ = 0; for (int v = 0; v < 8; ++v) { auto c = vary (m, v, 99u + (uint32_t) v, 0.5f, st); bool same = c.notes.size() == m.notes.size(); if (same) for (size_t i = 0; i < c.notes.size(); ++i) same &= c.notes[i].pitch == m.notes[i].pitch && std::abs (c.notes[i].start - m.notes[i].start) < 1e-4f; differ += ! same; }
        check (differ >= 7, "v0.40 MELODY: the 8 children are 8 different variations");
        // your melody: key detection (A minor, E major)
        std::vector<Note> am; const int ap[] { 69, 72, 76, 74, 72, 71, 69, 64, 69, 72, 76, 79, 77, 76, 74, 72, 69 };
        for (int i = 0; i < 17; ++i) am.push_back ({ 8.0f + (float) i * 0.5f, i == 16 ? 2.0f : 0.45f, ap[i], 0.8f });
        auto mine = fromNotes (am, 0);
        check (mine.key == 9 && mine.scale == scMinor && mine.bars == 4 && std::abs (mine.notes[0].start) < 1e-4f, "v0.40 MELODY: your melody - A minor found, moved to bar 1");
        std::vector<Note> em; const int ep[] { 64, 66, 68, 69, 71, 73, 75, 76, 71, 68, 64, 76 };
        for (int i = 0; i < 12; ++i) em.push_back ({ (float) i * 0.5f, 0.45f, ep[i], 0.8f });
        auto emj = fromNotes (em, 0);
        check (emj.key == 4 && emj.scale == scMajor, "v0.40 MELODY: E major found");
        // in the plugin: LISTEN to FL, GENERATE from it, play it with the sound, MIDI out
        KeysKillaProcessor p; p.setCurrentProgram (3); p.prepareToPlay (44100, 512);
        p.melListen (true);
        juce::AudioBuffer<float> b (2, 512);
        const double spb = 44100.0 * 60.0 / 140.0;   // samples per beat (no host: 140 BPM)
        for (int blk = 0; blk < 360; ++blk)
        {
            juce::MidiBuffer mb;
            for (int i = 0; i < 17; ++i)
            {
                const int on = (int) (i * 0.5 * spb), off = (int) ((i * 0.5 + 0.45) * spb);
                if (on >= blk * 512 && on < (blk + 1) * 512) mb.addEvent (juce::MidiMessage::noteOn (1, ap[i], (juce::uint8) 100), on - blk * 512);
                if (off >= blk * 512 && off < (blk + 1) * 512) mb.addEvent (juce::MidiMessage::noteOff (1, ap[i]), off - blk * 512);
            }
            p.processBlock (b, mb);
        }
        p.melListen (false);
        std::printf ("LISTEN: heard %d, kept %d notes, key %d scale %d\n", p.melHeardNotes(), (int) p.melMine.notes.size(), p.melMine.key, p.melMine.scale);
        check (p.melHasMine && p.melMine.notes.size() >= 15 && p.melMine.key == 9, "v0.40 MELODY: LISTEN catches the melody FL plays (A minor)");
        p.melFromMine = true; p.melGenerate();
        check (p.melShown.size() == 8 && p.melParent == 0 && p.mels[0].how == "YOURS", "v0.40 MELODY: 8 variations of your melody");
        p.melFromMine = false; p.melKey = 2; p.melScale = scDorian; p.melBars = 16; p.melGenerate();
        check (p.melShown.size() == 8 && p.mels[(size_t) p.melShown[3]].bars == 16 && p.mels[(size_t) p.melShown[3]].key == 2, "v0.40 MELODY: SURPRISE ME in D dorian, 16 bars");
        p.melEvolve (p.melShown[2], false);
        check (p.melParent >= 0 && p.melShown.size() == 8 && p.mels[(size_t) p.melShown[0]].gen == 2, "v0.40 MELODY: a click grows 8 children");
        p.melPlay (p.melShown[1]);
        float pk = 0; for (int i = 0; i < 300; ++i) { juce::MidiBuffer mb; p.processBlock (b, mb); pk = std::max (pk, b.getMagnitude (0, 512)); }
        check (p.loopPlaying() && pk > 0.02f, "v0.40 MELODY: the melody plays with the sound on the keys");
        p.melPlay (-1);
        const auto f = p.melExport (p.melShown[1]);
        juce::MidiFile mf; juce::FileInputStream in (f);
        check (f.existsAsFile() && in.openedOk() && mf.readFrom (in) && mf.getTrack (0)->getNumEvents() > 10, "v0.40 MELODY: drags out as MIDI");
    }
    // v0.41 GENRES: trap hooks on the 1/8 grid that repeat, house stabs on the off-beats, high DNB arps, in key, chords layer
    {
        using namespace kk::mel;
        int badKey = 0, totalNotes = 0, badChords = 0;
        for (int g = 0; g < numGenres; ++g)
        {
            double grid8 = 0, n8 = 0, med = 0; int nm = 0, span = 0, chordN = 0; double rep = 0;
            for (int k = 0; k < 12; k += 3)
            {
                const auto gi = genreInfo (g);
                Style st; auto m = generateGenre (500u + (uint32_t) (g * 97 + k), g, k, gi.scale, 8, st, (float) gi.bpm);
                const auto& steps = scaleSteps (gi.scale);
                int lo = 127, hi = 0;
                std::vector<int> ps;
                for (auto& n : m.notes)
                {
                    const int pc = ((n.pitch - k) % 12 + 12) % 12;
                    badKey += std::find (steps.begin(), steps.end(), pc) == steps.end(); ++totalNotes;
                    lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); ps.push_back (n.pitch);
                    const float q = n.start * 2.0f; grid8 += std::abs (q - std::round (q)) < 0.06f; n8 += 1;
                }
                for (auto& n : m.chords) { const int pc = ((n.pitch - k) % 12 + 12) % 12; badChords += std::find (steps.begin(), steps.end(), pc) == steps.end(); }
                chordN += (int) m.chords.size();
                std::sort (ps.begin(), ps.end()); if (! ps.empty()) { med += ps[ps.size() / 2] - k; ++nm; }
                span = std::max (span, hi - lo);
                // bars 1-2 vs bars 3-4: the hook repeats
                std::set<std::pair<int, int>> a, b2;
                for (auto& n : m.notes) { const int q = (int) std::round (n.start * 4.0f); if (q < 32) a.insert ({ q, n.pitch }); else if (q < 64) b2.insert ({ q - 32, n.pitch }); }
                int inter = 0; for (auto& x : a) inter += b2.count (x) > 0;
                rep += (double) inter / std::max<size_t> (1, std::max (a.size(), b2.size()));
                if (g == gHouse)
                {
                    int off = 0; for (auto& c : m.chords) { const float fr = c.start - std::floor (c.start); off += fr > 0.4f && fr < 0.6f; }
                    check (off >= (int) m.chords.size() * 9 / 10, "v0.41 HOUSE: the chord stabs sit on the off-beats");
                }
            }
            std::printf ("GENRE %-9s on 1/8 grid %3.0f%%  median pitch %5.1f  widest %2d  repeat %.2f  chord notes %d\n", genreName (g), 100.0 * grid8 / std::max (1.0, n8), med / std::max (1, nm), span, rep / 4.0, chordN);
            if (g == gTrap) check (grid8 / n8 > 0.85 && rep / 4.0 > 0.6, "v0.41 TRAP: hooks on the 1/8 grid that repeat like real trap loops");
            if (g == gDnb) check (med / std::max (1, nm) > 70.0, "v0.41 DNB: the arpeggios sit high (above the bass)");
            check (span <= 30 && chordN > 0, (juce::String ("v0.41 ") + genreName (g) + ": a playable range and a chords layer").toRawUTF8());
        }
        std::printf ("GENRES: %d of %d notes out of key (dark half-step leans), %d chord notes out of key\n", badKey, totalNotes, badChords);
        check (badKey * 40 < totalNotes && badChords == 0, "v0.41 GENRES: in the key (only rare dark half-step leans), chords always in key");
        {   // AUDIO -> MIDI: a played melody (tones with harmonics, 140 BPM, 1/8 notes) comes back as the same notes
            const int ap[] { 69, 72, 76, 74, 72, 71, 69, 64, 69, 72, 76, 79, 77, 76, 74, 72 };
            const double rate = 44100.0, spb = rate * 60.0 / 140.0;
            juce::AudioBuffer<float> a (1, (int) (spb * 0.5 * 17));
            a.clear();
            for (int i = 0; i < 16; ++i)
            {
                const int s0 = (int) (i * 0.5 * spb), len = (int) (0.42 * spb);
                const double f = 440.0 * std::pow (2.0, (ap[i] - 69) / 12.0);
                for (int k = 0; k < len; ++k)
                {
                    const double env = std::min (1.0, k / 200.0) * std::min (1.0, (len - k) / 400.0);
                    double v = 0; for (int h = 1; h <= 4; ++h) v += std::sin (2 * juce::MathConstants<double>::pi * f * h * k / rate) / h;
                    a.setSample (0, s0 + k, (float) (0.3 * env * v));
                }
            }
            auto heard = notesFromAudio (a, rate, 140.0);
            int match = 0; for (size_t i = 0; i < heard.size() && i < 16; ++i) match += heard[i].pitch == ap[i] && std::abs (heard[i].start - 0.5f * (float) i) < 0.08f;
            std::printf ("AUDIO -> MIDI: %d notes heard, %d of 16 right (pitch + time)\n", (int) heard.size(), match);
            check (heard.size() == 16 && match >= 15, "v0.41 AUDIO -> MIDI: a played melody becomes the same MIDI notes");
            auto mm = fromNotes (heard, 0);
            check (mm.key == 9 && mm.scale == scMinor, "v0.41 AUDIO -> MIDI: and its key (A minor) is found");
        }
        // in the plugin: the genre sets scale + tempo, the MIDI carries the chords when asked
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        p.melSetGenre (gHouse);
        check (p.melScale == scDorian && std::abs (p.melBpm - 126.0f) < 0.5f, "v0.41: HOUSE picks its scale and 126 BPM");
        p.melLayers = 1; p.melGenerate();
        const int idx = p.melShown[0];
        const auto f = p.melExport (idx);
        juce::MidiFile mf; juce::FileInputStream in (f);
        int ons = 0; if (in.openedOk() && mf.readFrom (in)) for (int i = 0; i < mf.getTrack (0)->getNumEvents(); ++i) ons += mf.getTrack (0)->getEventPointer (i)->message.isNoteOn();
        check (f.getFileName().contains ("HOUSE") && f.getFileName().contains ("126BPM") && ons == (int) (p.mels[(size_t) idx].notes.size() + p.mels[(size_t) idx].chords.size()), "v0.41: + CHORDS drags melody and chords in one MIDI (named HOUSE 126BPM)");
        p.melEvolve (idx, false);
        check (p.mels[(size_t) p.melShown[1]].genre == gHouse && ! p.mels[(size_t) p.melShown[1]].chords.empty(), "v0.41: the children keep the genre and the chords");
    }
    // v0.41 MIX LAB: EQ (bell, 24 dB cut, dynamic), compressor, TIME MACHINE (silence stays silent), COACH, state
    {
        const double rate = 44100.0; const int N = 44100;
        auto sine = [&] (float hz, float amp) { juce::AudioBuffer<float> b (2, N); for (int i = 0; i < N; ++i) { const float v = amp * std::sin (kk::twoPi * hz * (float) i / (float) rate); b.setSample (0, i, v); b.setSample (1, i, v); } return b; };
        auto runLab = [&] (kk::MixLabState& st, juce::AudioBuffer<float> b) { kk::MixLabDsp d; d.prepare (rate, 512); for (int o = 0; o < N; o += 512) { const int n = std::min (512, N - o); d.process (b.getWritePointer (0) + o, b.getWritePointer (1) + o, n, st); } return b; };
        auto tailRms = [&] (const juce::AudioBuffer<float>& b) { return b.getRMSLevel (0, N / 2, N / 2); };
        auto db = [] (float x) { return 20.0f * std::log10 (x + 1e-9f); };
        {
            kk::MixLabState st; st.band[3].on = true; st.band[3].type = kk::eqBell; st.band[3].freq = 1000; st.band[3].gain = 6; st.band[3].q = 1;
            const float g = db (tailRms (runLab (st, sine (1000, 0.25f)))) - db (tailRms (sine (1000, 0.25f)));
            kk::MixLabState lc; lc.band[0].on = true; lc.band[0].type = kk::eqLowCut; lc.band[0].freq = 200; lc.band[0].slope = 2;
            const float cut = db (tailRms (runLab (lc, sine (50, 0.25f)))) - db (tailRms (sine (50, 0.25f)));
            std::printf ("MIX LAB EQ: bell +6 at 1k -> %+.2f dB, 24 dB low cut at 200 Hz -> 50 Hz %+.1f dB\n", g, cut);
            check (std::abs (g - 6.0f) < 0.5f && cut < -40.0f, "v0.41 SHAPE EQ: a bell boosts what it should, a 24 dB cut really cuts");
            kk::MixLabState dy; dy.band[4].on = true; dy.band[4].type = kk::eqBell; dy.band[4].freq = 3000; dy.band[4].gain = 0; dy.band[4].q = 2; dy.band[4].dyn = 1.0f;
            const float loud = db (tailRms (runLab (dy, sine (3000, 0.5f)))) - db (tailRms (sine (3000, 0.5f)));
            const float quiet = db (tailRms (runLab (dy, sine (3000, 0.003f)))) - db (tailRms (sine (3000, 0.003f)));
            std::printf ("MIX LAB DYNAMIC band: loud %+.1f dB, quiet %+.1f dB\n", loud, quiet);
            check (loud < -6.0f && quiet > -1.0f, "v0.41 DYNAMIC EQ: cuts only when that range is loud");
        }
        {
            kk::MixLabState st; st.compOn = true; st.compStyle = kk::csClean; st.thresh = -20; st.ratio = 4; st.knee = 0; st.attack = 2; st.release = 50; st.autoGain = false; st.scHp = false;
            auto out = runLab (st, sine (220, 0.5f));
            const float gr = db (tailRms (sine (220, 0.5f))) - db (tailRms (out));
            std::printf ("MIX LAB COMP: -6 dBFS sine, -20 dB threshold 4:1 -> %.1f dB less (meter %.1f)\n", gr, st.mGr.load());
            check (gr > 7.0f && gr < 13.0f && st.mGr.load() < -6.0f, "v0.41 PUNCH COMP: it compresses by the ratio above the threshold");
            for (int s2 = 0; s2 < kk::numCompStyles; ++s2) { st.compStyle = s2; auto o2 = runLab (st, sine (220, 0.5f)); check (o2.getMagnitude (0, N) < 1.2f && std::isfinite (o2.getRMSLevel (0, 0, N)), "v0.41 PUNCH COMP: every character is stable"); }
        }
        {
            int changed = 0; bool finite = true; float silentOut = 0;
            for (int m = 0; m < kk::numTm; ++m)
            {
                kk::MixLabState st; st.tmOn = true; st.tmModOn[(size_t) m] = true; st.tmAmt[(size_t) m] = 0.8f;
                auto in = sine (440, 0.3f); auto out = runLab (st, in);
                float diff = 0; for (int i = N / 2; i < N; ++i) diff += std::abs (out.getSample (0, i) - in.getSample (0, i));
                changed += diff / (N / 2) > 0.003f; finite &= std::isfinite (out.getRMSLevel (0, 0, N)) && out.getMagnitude (0, N) < 1.5f;
                juce::AudioBuffer<float> z (2, N); z.clear(); auto zo = runLab (st, z); silentOut = std::max (silentOut, zo.getMagnitude (0, N));
            }
            std::printf ("TIME MACHINE: %d of 6 modules change the sound, silence -> peak %.6f\n", changed, silentOut);
            check (changed == 6 && finite && silentOut < 1e-4f, "v0.41 TIME MACHINE: all six modules colour the sound, silence stays silent (the noise follows the music)");
            kk::MixLabState st; kk::applyEra (st, 0.0f);
            check (st.tmLp.load() < 6000.0f && st.tmMono.load() > 0.9f && st.tmModOn[kk::tmNoise].load(), "v0.41 ERA: the oldest machine is narrow, mono and noisy");
            kk::applyEra (st, 5.0f / 7.0f);
            int anyOn = 0; for (auto& o : st.tmModOn) anyOn += o.load();
            check (anyOn == 0 && st.tmLp.load() > 19000.0f, "v0.41 ERA: CLEAN (2026) is clean");
        }
        {
            // the COACH: a spectrum with far too much low mid -> it says so, the FIX puts a cut there
            const int bins = 2049; const float sr = 44100.0f, hz = sr * 0.5f / (bins - 1);
            std::vector<float> pw ((size_t) bins);
            for (int b = 1; b < bins; ++b) { const float f = b * hz; pw[(size_t) b] = 1e-4f / std::max (30.0f, f) * (f > 150 && f < 400 ? 40.0f : 1.0f) * (f < 60 ? 3.0f : 1.0f); }
            kk::MixLabState st; st.mRms = 0.1f; st.mPeak = 0.4f; st.mCrest = 12; st.mCorr = 0.8f; st.mWidth = 0.3f;
            auto rd = kk::coach::read (pw, sr, st);
            auto tips = kk::coach::advise (rd, 0, true);
            bool saw = false; for (auto& t : tips) if (t.fix == kk::coach::fixRegionCut && t.region == kk::coach::rLowMid) { saw = true; kk::coach::applyFix (st, t); }
            int cutBand = -1; for (int b = 0; b < kk::MixLabState::numBands; ++b) if (st.band[(size_t) b].on.load() && st.band[(size_t) b].gain.load() < -1 && st.band[(size_t) b].freq.load() > 150 && st.band[(size_t) b].freq.load() < 400) cutBand = b;
            check (saw && cutBand >= 0, "v0.41 COACH: hears too much low mids and FIX cuts there");
            st.mPeak = 1.0f; auto t2 = kk::coach::advise (kk::coach::read (pw, sr, st), 0, true);
            check (! t2.empty() && t2[0].fix == kk::coach::fixLower && t2[0].severity == 3, "v0.41 COACH: hitting 0 dB is the first thing it tells you");
            // key from harmonic tones (A minor triad with saw-like harmonics)
            std::vector<float> k2 ((size_t) bins, 1e-12f);
            for (int note : { 57, 60, 64, 69, 45 })
                for (int h = 1; h <= 8; ++h) { const float f = 440.0f * std::pow (2.0f, (note - 69) / 12.0f) * h; const int b = (int) std::round (f / hz); if (b < bins) k2[(size_t) b] += 1.0f / (h * h); }
            st.mPeak = 0.5f; auto rk = kk::coach::read (k2, sr, st);
            std::printf ("COACH key: %d %s (conf %.3f)\n", rk.key, rk.minor ? "minor" : "major", rk.keyConf);
            check (rk.key == 9 && rk.minor, "v0.41 COACH: finds A minor from harmonic tones");
            // AUTO EQ + EVOLVE
            kk::MixLabState a2; a2.mRms = 0.1f; a2.mPeak = 0.4f;
            check (kk::coach::autoEq (a2, kk::coach::read (pw, sr, a2), 0).contains ("moves"), "v0.41 AUTO EQ makes moves on an unbalanced sound");
            auto base = kk::coach::snap (a2); int differ = 0;
            for (int k = 0; k < 4; ++k) { auto m = kk::coach::mutate (base, 100u + (uint32_t) k, 0.5f); float d = 0; for (float f : { 100.0f, 500.0f, 2000.0f, 8000.0f }) d += std::abs (kk::coach::curveDb (m, f, sr) - kk::coach::curveDb (base, f, sr)); differ += d > 1.0f; }
            check (differ == 4, "v0.41 EQ EVOLVE: four different curves grow from yours");
        }
        {
            // SPACE: every mode makes a tail that lasts after the sound stops, FREEZE holds it, nothing blows up; ECHO lands on the beat
            for (int mode = 0; mode < kk::numSpaceModes; ++mode)
            {
                kk::MixLabState st; st.spOn = true; st.spMode = mode; st.spMix = 0.5f; st.spDecay = 0.5f; st.spPre = 10;
                juce::AudioBuffer<float> b (2, N * 2); b.clear();
                for (int i = 0; i < 4410; ++i) { const float v = 0.4f * std::sin (kk::twoPi * 330.0f * (float) i / (float) rate); b.setSample (0, i, v); b.setSample (1, i, v); }
                kk::MixLabDsp d; d.prepare (rate, 512);
                for (int o = 0; o < N * 2; o += 512) d.process (b.getWritePointer (0) + o, b.getWritePointer (1) + o, std::min (512, N * 2 - o), st);
                const float tail = b.getRMSLevel (0, 8820, 8820), late = b.getRMSLevel (0, N * 2 - 4410, 4410);
                std::printf ("SPACE %-8s tail %.4f  late %.5f  peak %.3f\n", kk::spaceModeName (mode), tail, late, b.getMagnitude (0, N * 2));
                check (tail > 0.003f && b.getMagnitude (0, N * 2) < 1.5f && std::isfinite (late), (juce::String ("v0.41 SPACE ") + kk::spaceModeName (mode) + ": a tail, stable").toRawUTF8());
                if (mode == kk::spRoom) check (late < tail * 0.05f, "v0.41 SPACE ROOM: dies away quickly");
            }
            {
                kk::MixLabState st; st.dlOn = true; st.dlMode = kk::dlDigital; st.dlMix = 1.0f; st.dlFb = 0.0f; st.dlTime = 0; st.bpm = 120.0f;   // 1/4 at 120 = 0.5 s
                juce::AudioBuffer<float> b (2, N); b.clear(); b.setSample (0, 0, 1.0f); b.setSample (1, 0, 1.0f);
                kk::MixLabDsp d; d.prepare (rate, 512);
                for (int o = 0; o < N; o += 512) d.process (b.getWritePointer (0) + o, b.getWritePointer (1) + o, std::min (512, N - o), st);
                int at = 0; float mx = 0; for (int i = 100; i < N; ++i) if (std::abs (b.getSample (0, i)) > mx) { mx = std::abs (b.getSample (0, i)); at = i; }
                std::printf ("ECHO 1/4 at 120 BPM: first echo at %d samples (expected 22050)\n", at);
                check (std::abs (at - 22050) < 30, "v0.41 ECHO: the echo lands on the beat (1/4 at 120 BPM)");
            }
            for (int mode = 0; mode < kk::numEchoModes; ++mode)
            {
                kk::MixLabState st; st.dlOn = true; st.dlMode = mode; st.dlMix = 0.6f; st.dlFb = 0.85f; st.dlTime = 1; st.bpm = 140.0f;
                auto b = sine (500, 0.4f);
                kk::MixLabDsp d; d.prepare (rate, 512);
                for (int o = 0; o < N; o += 512) d.process (b.getWritePointer (0) + o, b.getWritePointer (1) + o, std::min (512, N - o), st);
                check (std::isfinite (b.getRMSLevel (0, 0, N)) && b.getMagnitude (0, N) < 3.0f && b.getRMSLevel (0, N / 2, N / 2) > 0.05f, (juce::String ("v0.41 ECHO ") + kk::echoModeName (mode) + ": echoes, stable at high feedback").toRawUTF8());
            }
        }
        {
            KeysKillaProcessor p; p.prepareToPlay (44100, 512);
            p.mixLab.compOn = true; p.mixLab.thresh = -31.0f; p.mixLab.band[5].on = true; p.mixLab.band[5].freq = 4321.0f; p.mixLab.tmOn = true; p.mixLab.tmAmt[2] = 0.77f; p.coachGenre = kk::mel::gHouse;
            juce::MemoryBlock mb; p.getStateInformation (mb);
            KeysKillaProcessor r2; r2.prepareToPlay (44100, 512); r2.setStateInformation (mb.getData(), (int) mb.getSize());
            check (r2.mixLab.compOn.load() && std::abs (r2.mixLab.thresh.load() + 31.0f) < 0.01f && r2.mixLab.band[5].on.load() && std::abs (r2.mixLab.band[5].freq.load() - 4321.0f) < 0.5f
                   && r2.mixLab.tmOn.load() && std::abs (r2.mixLab.tmAmt[2].load() - 0.77f) < 0.01f && r2.coachGenre == kk::mel::gHouse, "v0.41 MIX LAB: the project keeps every setting");
        }
    }
    // v0.41 MATCH: a sound of the bank, rendered to a WAV, is found again (and every strand can be planted)
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        auto g = p.currentGenome();
        p.loadPreset (17); auto target = p.renderGenomeAudio (p.currentGenome(), 44100.0, 1.6);
        auto wf = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_match_target.wav");
        wf.deleteFile();
        { juce::WavAudioFormat wav; std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (wf), 44100.0, 2, 24, {}, 0)); if (w) w->writeFromAudioSampleBuffer (target, 0, target.getNumSamples()); }
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        check (p.evoMatchStart (wf), "v0.41 MATCH: starts on a WAV");
        while (p.matchRunning.load() && juce::Time::getMillisecondCounterHiRes() - t0 < 240000.0) juce::Thread::sleep (50);
        auto st = p.matchStrands();
        float best = 0; for (auto& x : st) best = std::max (best, x.match);
        std::printf ("MATCH: %d strands, best %.0f%%, %.1f s\n", (int) st.size(), best, (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
        check (st.size() == 4 && best > 85.0f, "v0.41 MATCH: four strands, the closest is very close to the sound");
        p.evoSeedGenome (st[1].g);
        check (! p.evo.empty() && p.evo[0].g.valid(), "v0.41 MATCH: a strand plants a new EVOLVE tree");
        wf.deleteFile();
        juce::ignoreUnused (g);
    }
    // v0.42 LISTEN AUDIO: EVOLVE as an effect on another plugin's track - its sound becomes the melody
    {
        KeysKillaProcessor p; p.enableAllBuses(); p.prepareToPlay (44100, 512);
        const int ap[] { 69, 72, 76, 74, 72, 71, 69, 64, 69, 72, 76, 79, 77, 76, 74, 72 };
        const double spb = 44100.0 * 60.0 / 140.0;
        const int total = (int) (spb * 0.5 * 17);
        std::vector<float> a ((size_t) total, 0.0f);
        for (int i = 0; i < 16; ++i)
        {
            const int s0 = (int) (i * 0.5 * spb), len = (int) (0.42 * spb);
            const double f = 440.0 * std::pow (2.0, (ap[i] - 69) / 12.0);
            for (int k = 0; k < len; ++k) { const double env = std::min (1.0, k / 200.0) * std::min (1.0, (len - k) / 400.0); double v = 0; for (int h = 1; h <= 4; ++h) v += std::sin (2 * juce::MathConstants<double>::pi * f * h * k / 44100.0) / h; a[(size_t) (s0 + k)] = (float) (0.3 * env * v); }
        }
        p.melListen (true);
        juce::AudioBuffer<float> b (2, 512);
        for (int o = 0; o < total; o += 512)
        {
            b.clear();
            for (int i = 0; i < 512 && o + i < total; ++i) { b.setSample (0, i, a[(size_t) (o + i)]); b.setSample (1, i, a[(size_t) (o + i)]); }
            juce::MidiBuffer mb; p.processBlock (b, mb);
        }
        p.melListen (false);
        std::printf ("LISTEN AUDIO: inputs %d, %d notes, key %d, from %s\n", p.getTotalNumInputChannels(), (int) p.melMine.notes.size(), p.melMine.key, p.melListenSource.toRawUTF8());
        check (p.melHasMine && p.melListenSource == "AUDIO" && p.melMine.notes.size() >= 15 && p.melMine.key == 9, "v0.42 LISTEN AUDIO: the sound of another plugin becomes the melody (A minor)");
    }
    // v0.42: a sound edited in EDIT drags / saves as edited (not as it was before the edit)
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        juce::AudioBuffer<float> b (2, 44100);
        for (int i = 0; i < b.getNumSamples(); ++i) { const float v = 0.5f * std::sin (kk::twoPi * 261.63f * (float) i / 44100.0f) * std::exp (-(float) i / 15000.0f); b.setSample (0, i, v); b.setSample (1, i, v); }
        auto snd = kk::PairLab::fromBuffer (b, 44100.0, 44100.0, "kid");
        p.useSample (snd, false);
        check (p.withEdits (snd) == snd, "v0.42 EDIT: an unedited sound drags as it is");
        p.sampleEdit[KeysKillaProcessor::seTune] = 7.0f;
        auto e = p.withEdits (snd);
        check (e != snd && e != nullptr && e->audio.getNumSamples() < snd->audio.getNumSamples() * 0.8, "v0.42 EDIT: the edited sound (tune +7) is what drags / saves");
        p.resetSampleEdit();
    }
    // v0.42 EVOLVE FX CHAINS: every chain changes the sound and stays safe, a saved chain comes back the same
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        int changed = 0; bool safe = true;
        for (int c = 0; c < KeysKillaProcessor::chainNames().size(); ++c)
        {
            p.chainApply (c);
            const auto t = p.chainToTree ("x").createXml()->toString();
            p.chainReset();
            changed += p.chainToTree ("x").createXml()->toString() != t;
            p.chainApply (c);
            juce::AudioBuffer<float> b (2, 512); float pk = 0;
            for (int i = 0; i < 200; ++i) { b.clear(); juce::MidiBuffer mb; if (i == 0) mb.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0); p.processBlock (b, mb); pk = std::max (pk, b.getMagnitude (0, 512)); }
            safe &= std::isfinite (pk) && pk <= 1.0f;
        }
        check (changed == KeysKillaProcessor::chainNames().size() && safe, "v0.42 CHAINS: every chain sets something, the output stays safe");
        p.chainApply (7); p.chainEvolve (0.5f);
        const auto before = p.chainToTree ("mine").createXml()->toString();
        const auto f = p.chainSave ("kk test chain");
        p.chainReset();
        check (f.existsAsFile() && p.chainLoad (f) && p.chainToTree ("mine").createXml()->toString() == before, "v0.42 CHAINS: a saved chain loads back exactly");
        f.deleteFile();
    }
    // v0.42 SOUND WORLD: thousands of dots on land, every region has sounds, dots differ, regions connect
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        const auto& ds = kk::world::dots();
        std::vector<int> perRegion (kk::world::regions().size(), 0);
        for (auto& d : ds) ++perRegion[(size_t) d.region];
        int emptyRegions = 0, noPresets = 0;
        for (size_t r = 0; r < perRegion.size(); ++r) { emptyRegions += perRegion[r] < 50; noPresets += kk::world::regionPresets ((int) r).empty(); }
        std::printf ("SOUND WORLD: %d dots, %d regions without dots, %d without sounds\n", (int) ds.size(), emptyRegions, noPresets);
        check (ds.size() > 10000 && emptyRegions == 0 && noPresets == 0, "v0.42 SOUND WORLD: every region has many dots and its own sounds");
        std::set<juce::String> names; int valid = 0;
        for (int i = 0; i < (int) ds.size(); i += (int) ds.size() / 40) { auto g = p.worldSound (i); valid += g.valid(); names.insert (g.name); }
        auto h = p.worldConnect (3, 9, 5);
        check (valid >= 40 && names.size() >= 20 && h.valid() && h.name.contains ("EUROPE") && h.name.contains ("EAST ASIA"), "v0.42 SOUND WORLD: dots give many different sounds, two regions connect");
        p.worldPlay (h, false);
        float pk = 0; juce::AudioBuffer<float> b (2, 512);
        for (int i = 0; i < 100; ++i) { juce::MidiBuffer mb; if (i == 0) mb.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0); p.processBlock (b, mb); pk = std::max (pk, b.getMagnitude (0, 512)); }
        check (pk > 0.01f, "v0.42 SOUND WORLD: the hybrid plays on the keys");
    }
    // v0.42 EVOLVE FX PRO modules: REMIX REEL, DIAL-UP, WARP DRIVE, FINAL BOSS
    {
        const double rate = 44100.0; const int N = 44100 * 2;
        auto tone = [&] (float hz, float amp) { juce::AudioBuffer<float> b (2, N); for (int i = 0; i < N; ++i) { const float v = amp * std::sin (kk::twoPi * hz * (float) i / (float) rate) * (0.6f + 0.4f * std::sin ((float) i * 0.0007f)); b.setSample (0, i, v); b.setSample (1, i, v); } return b; };
        auto zc = [] (const juce::AudioBuffer<float>& b, int from) { int z = 0; for (int i = from + 1; i < b.getNumSamples(); ++i) z += (b.getSample (0, i - 1) < 0) != (b.getSample (0, i) < 0); return z; };
        {   // REEL: a 1/8 roll on every step changes the music, stays finite
            kk::pro::ReelState st; st.on = true; for (int s2 = 0; s2 < 16; ++s2) st.grid[kk::pro::rrLoop][(size_t) s2] = 3;
            kk::pro::ReelDsp d; d.prepare (rate);
            auto in = tone (330, 0.4f), out = in;
            const double bps = 120.0 / 60.0 / rate;
            for (int o = 0; o < N; o += 512) d.process (out.getWritePointer (0) + o, out.getWritePointer (1) + o, std::min (512, N - o), st, bps * o, bps);
            float diff = 0; for (int i = N / 2; i < N; ++i) diff += std::abs (out.getSample (0, i) - in.getSample (0, i));
            check (diff / (N / 2) > 0.01f && std::isfinite (out.getRMSLevel (0, 0, N)) && out.getMagnitude (0, N) < 1.5f, "v0.42 REMIX REEL: the steps re-cut the music");
            int changedPresets = 0; for (int pr = 0; pr < 5; ++pr) { st.preset (pr); int cells = 0; for (auto& row : st.grid) for (auto& c : row) cells += c.load() != 0; changedPresets += cells > 0; }
            check (changedPresets == 5, "v0.42 REMIX REEL: five ready patterns");
        }
        {   // DIAL-UP: a phone line cuts the highs; every mode stays safe
            for (int m = 0; m < kk::pro::numDialModes; ++m)
            {
                kk::pro::DialState st; st.on = true; st.applyMode (m); st.signal = 0.0f;
                kk::pro::DialDsp d; d.prepare (rate);
                auto hi = tone (9000, 0.3f), out = hi;
                for (int o = 0; o < N; o += 512) d.process (out.getWritePointer (0) + o, out.getWritePointer (1) + o, std::min (512, N - o), st);
                const float att = 20.0f * std::log10 ((out.getRMSLevel (0, N / 2, N / 2) + 1e-9f) / hi.getRMSLevel (0, N / 2, N / 2));
                if (m == kk::pro::dmLandline) std::printf ("DIAL-UP LANDLINE: 9 kHz %.1f dB\n", att);
                check ((m == kk::pro::dmVoiceNote || att < -12.0f) && std::isfinite (att) && out.getMagnitude (0, N) < 1.2f, (juce::String ("v0.42 DIAL-UP ") + kk::pro::dialModeName (m) + ": phone band, safe").toRawUTF8());
            }
        }
        {   // WARP DRIVE: an octave up doubles the pitch
            kk::pro::WarpState st; st.on = true; st.applyMode (kk::pro::wmUp); st.mix = 1.0f;
            kk::pro::WarpDsp d; d.prepare (rate);
            auto in = tone (220, 0.4f), out = in;
            for (int o = 0; o < N; o += 512) d.process (out.getWritePointer (0) + o, out.getWritePointer (1) + o, std::min (512, N - o), st);
            const float ratio = (float) zc (out, N / 2) / (float) std::max (1, zc (in, N / 2));
            std::printf ("WARP DRIVE octave up: pitch ratio %.2f\n", ratio);
            check (ratio > 1.7f && ratio < 2.4f, "v0.42 WARP DRIVE: OCTAVE UP doubles the pitch");
        }
        {   // FINAL BOSS: never over the ceiling; AUTO walks toward the target loudness
            kk::pro::BossState st; st.on = true; st.ceiling = -1.0f; st.target = kk::pro::btStreaming;
            kk::pro::BossDsp d; d.prepare (rate);
            auto in = tone (110, 1.6f), out = in;
            for (int o = 0; o < N; o += 512) d.process (out.getWritePointer (0) + o, out.getWritePointer (1) + o, std::min (512, N - o), st);
            std::printf ("FINAL BOSS: peak %.2f dB, short-term %.1f LUFS, auto %.1f dB, GR %.1f dB\n", 20.0f * std::log10 (out.getMagnitude (0, N)), st.mShort.load(), st.mAuto.load(), st.mGr.load());
            check (out.getMagnitude (0, N) <= std::pow (10.0f, -1.0f / 20.0f) + 1e-4f, "v0.42 FINAL BOSS: the output never passes the ceiling (-1 dB)");
            check (st.mAuto.load() < -0.5f && st.mShort.load() > -30.0f, "v0.42 FINAL BOSS: AUTO turns a too-loud track down toward -14 LUFS");
        }
        {   // state
            KeysKillaProcessor p; p.prepareToPlay (44100, 512);
            p.reel.on = true; p.reel.grid[2][5] = 3; p.dial.on = true; p.dial.applyMode (kk::pro::dmWalkie); p.warp.on = true; p.warp.semis = -5; p.boss.target = kk::pro::btClub;
            juce::MemoryBlock mb; p.getStateInformation (mb);
            KeysKillaProcessor r2; r2.prepareToPlay (44100, 512); r2.setStateInformation (mb.getData(), (int) mb.getSize());
            check (r2.reel.on.load() && r2.reel.grid[2][5].load() == 3 && r2.dial.mode.load() == kk::pro::dmWalkie && std::abs (r2.warp.semis.load() + 5.0f) < 0.01f && r2.boss.target.load() == kk::pro::btClub, "v0.42 FX PRO: the project keeps the modules");
        }
    }
    // v0.42 SAMPLER MELODY: a sample on the keys plays generated melodies, out as a WAV
    {
        KeysKillaProcessor p; p.prepareToPlay (44100, 512);
        juce::AudioBuffer<float> b (2, 22050);
        for (int i = 0; i < b.getNumSamples(); ++i) { const float v = 0.5f * std::sin (kk::twoPi * 261.63f * (float) i / 44100.0f) * std::exp (-(float) i / 8000.0f); b.setSample (0, i, v); b.setSample (1, i, v); }
        auto snd = kk::PairLab::tuned (kk::PairLab::fromBuffer (b, 44100.0, 44100.0, "test pluck"), 44100.0);
        p.useSample (snd, false);
        p.melSetGenre (kk::mel::gTrap); p.melGenerate();
        const auto f = p.melExportWav (p.melShown[0]);
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (f));
        check (f.existsAsFile() && rd != nullptr && rd->lengthInSamples > 44100 * 10, "v0.42 SAMPLER MELODY: the melody played by the sample drags out as a WAV");
        f.deleteFile();
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
        // SOUND KITS: a sound saved into a kit lands in the right category folder (by its name) or the one you pick
        const juce::String kit = "KK TEST SOUND KIT";
        kk::SoundKits::deleteKit (kit);
        check (kk::SoundKits::createKit (kit) && kk::SoundKits::kits().contains (kit), "SOUND KITS: create a kit");
        auto bass = kk::PairLab::fromBuffer (b, 44100.0, 44100.0, "Glide Reese Bass");
        const auto kb = p.saveToSoundKit (bass, kit);
        check (kb.existsAsFile() && kb.getParentDirectory().getFileName() == "Bass" && p.lastSoundKit == kit, "SOUND KITS: a bass lands in <kit>/Bass");
        const auto kp = p.saveToSoundKit (snd, kit, "Pads");
        check (kp.existsAsFile() && kp.getParentDirectory().getFileName() == "Pads" && kk::SoundKits::count (kit) == 2, "SOUND KITS: or in the category you pick");
        check (kk::SoundKits::renameKit (kit, kit + " 2") && kk::SoundKits::count (kit + " 2") == 2, "SOUND KITS: rename a kit");
        check (kk::SoundKits::deleteKit (kit + " 2") && ! kk::SoundKits::kits().contains (kit + " 2"), "SOUND KITS: delete a kit");
    }
    // v0.36 EVOLVE: seed -> 6 children -> grow from any of them, SAFE vs WILD, audio seeds, blend, export, project
    {
        KeysKillaProcessor p (false); p.prepareToPlay (44100, 512);
        p.evoSeedPreset (0);
        check (p.evo.size() == 7 && p.evoCenter == 0 && p.evo[0].kids.size() == 6, "EVOLVE: a seed grows 6 children");
        const int k3 = p.evo[0].kids[3];
        p.evoFocus (k3);
        check (p.evoCenter == k3 && p.evo[(size_t) k3].kids.size() == 6 && p.evoPath().size() == 2, "EVOLVE: click a child - it becomes the middle and grows");
        auto dist = [&] (float wild)
        {
            p.evoWild = wild; p.evoSeedPreset (5);
            double d = 0;
            for (int k : p.evo[0].kids) for (size_t i = 0; i < p.evo[0].g.v.size(); ++i) d += std::abs (p.evo[(size_t) k].g.v[i] - p.evo[0].g.v[i]);
            return d;
        };
        double safe = 0, wild = 0; for (int r = 0; r < 3; ++r) { safe += dist (0.0f); wild += dist (1.0f); }
        check (wild > safe * 1.3, "EVOLVE: WILD children wander further than SAFE ones");
        std::set<juce::String> names; for (auto& n : p.evo) names.insert (n.name);
        check (names.size() == p.evo.size(), "EVOLVE: every child has its own name");
        const auto wf = p.evoExportWav (p.evo[0].kids[0]), mf = p.evoExportMidi (p.evo[0].kids[0]);
        check (wf.existsAsFile() && wf.getSize() > 2000 && mf.existsAsFile(), "EVOLVE: drag a child as WAV and as a melody");
        const float before = p.apvts.getParameter (ID::cutoff)->getValue();
        p.evoMorph (0, p.evo[0].kids[5], 1.0f);
        const float after = p.apvts.getParameter (ID::cutoff)->getValue();
        check (std::abs (after - p.evo[(size_t) p.evo[0].kids[5]].g.v[(size_t) p.apvts.getParameter (ID::cutoff)->getParameterIndex()]) < 1.0e-3f || before != after, "EVOLVE: blend moves the sound to the child");
        // the tree comes back with the project
        p.evoFocus (p.evo[0].kids[1]);
        juce::MemoryBlock mb; p.getStateInformation (mb);
        KeysKillaProcessor q (false); q.setStateInformation (mb.getData(), (int) mb.getSize());
        check (q.evo.size() == p.evo.size() && q.evoCenter == p.evoCenter && q.evo[(size_t) q.evoCenter].name == p.evo[(size_t) p.evoCenter].name, "EVOLVE: the tree is saved with the project");
        // an audio seed (your WAV)
        auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_evo_seed.wav");
        {
            juce::AudioBuffer<float> b (2, 44100);
            for (int i = 0; i < b.getNumSamples(); ++i) { const float x = 0.5f * std::sin ((float) i * 0.0627f) * std::exp (-(float) i / 12000.0f); b.setSample (0, i, x); b.setSample (1, i, x); }
            f.deleteFile(); juce::WavAudioFormat wav;
            std::unique_ptr<juce::AudioFormatWriter> w (wav.createWriterFor (new juce::FileOutputStream (f), 44100, 2, 16, {}, 0));
            if (w) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
        check (p.evoSeedFromFile (f) && p.evo[0].isAudio() && p.evo[0].kids.size() == 6 && p.evo[(size_t) p.evo[0].kids[0]].isAudio(), "EVOLVE: your WAV is a seed - 6 audio children");
        p.evoFocus (p.evo[0].kids[2]);
        check (p.evo[(size_t) p.evoCenter].kids.size() == 6, "EVOLVE: audio children grow too");
    }
    // v0.34 NAMES: no genre / city words, unique names and IDs, old names still find their sound
    {
        const auto& ps = factoryPresets();
        std::set<juce::String> names, ids; bool clean = true, legacy = true;
        for (auto& pr : ps)
        {
            for (auto* w : { "Trap", "Drill", "Plugg", "Rage", "Atlanta", "ATL", "Detroit", "UK", "Memphis", "Phonk" })
                if (juce::StringArray::fromTokens (pr.name, " ", "").contains (w) || pr.name.contains ("Supertrap")) { clean = false; std::printf ("   genre name: %s\n", pr.name.toRawUTF8()); }
            names.insert (pr.name); ids.insert (pr.id);
            legacy &= findFactoryPreset (pr.legacyName) == (int) (&pr - ps.data()) && findFactoryPreset (pr.id) == (int) (&pr - ps.data());
        }
        check (clean, "NAMES: no Trap / Drill / Plugg / Rage / city words in sound names");
        check (names.size() == ps.size() && ids.size() == ps.size(), "NAMES: every name and ID is unique");
        check (legacy, "NAMES: the old name and the ID find the same sound");
        check (currentFactoryName ("Classic Trap Bell") == "Classic Minor Bell" && ps[0].legacyName == "Classic Trap Bell", "NAMES: favourites move to the new name");
        // a v0.33 project (index + old name) opens the same sound under its new name
        KeysKillaProcessor a (false), b (false);
        a.setCurrentProgram (0);
        juce::MemoryBlock mb; a.getStateInformation (mb);
        auto xml = juce::AudioProcessor::getXmlFromBinary (mb.getData(), (int) mb.getSize());
        xml->setAttribute ("presetName", "Classic Trap Bell");
        juce::AudioProcessor::copyXmlToBinary (*xml, mb);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        check (b.currentName() == "Classic Minor Bell" && b.currentPresetIndex() == 0, "NAMES: an old project opens the same sound with its new name");
    }
    // v0.34 SOUND PACKS: export -> .kkpack -> install -> PACKS list
    {
        KeysKillaProcessor p (false); p.prepareToPlay (44100, 512);
        p.setCurrentProgram (5);
        auto pre = KeysKillaProcessor::userPresetDir().getChildFile ("KK TEST PACK SOUND.kkpreset");
        check (p.saveUserPreset (pre), "PACKS: save a sound");
        auto pk = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KK TEST.kkpack");
        check (p.exportPack (pk, "KK TEST PACK") && pk.existsAsFile(), "PACKS: export my sounds as a .kkpack");
        pre.deleteFile();
        juce::String err;
        const auto name = p.installPack (pk, &err);
        bool found = false;
        for (auto& s : p.packSounds()) found |= s.name == "KK TEST PACK SOUND" && s.pack == "KK TEST PACK" && s.cat == factoryPresets()[5].cat;
        check (name == "KK TEST PACK" && found, "PACKS: install puts the sounds in Documents/KEYS KILLA/Packs with their pack + category");
        bool loads = false;
        for (auto& s : p.packSounds()) if (s.pack == "KK TEST PACK") loads = p.loadUserPreset (s.file);
        check (loads, "PACKS: a pack sound loads");
        auto bad = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("kk_not_a_pack.kkpack");
        bad.replaceWithText ("hello");
        check (p.installPack (bad, &err).isEmpty() && err.isNotEmpty(), "PACKS: a broken file is refused with a message");
        check (p.removePack ("KK TEST PACK"), "PACKS: remove a pack");
        pk.deleteFile(); bad.deleteFile();
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
        check (same && q.getTreeMode() == KeysKillaProcessor::treeSound && q.loopBars() == 16 && q.loopKey() == 5, "family tree survives save / load (v0.40: sounds only)");
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
    if (argc > 1 && juce::String (argv[1]) == "-unit") { const int f = unitTests(); std::printf ("unit failures: %d\n", f); return f; }
    if (argc > 1 && juce::String (argv[1]) == "-lib") return libraryCheck (p, true);   // v0.30 sound library cleanliness only
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
                case 4: ke->showView (action % 2 ? 9 : 26); setp (ID::cutoff, 200.0f + 90.0f * (float) (action % 100)); break;   // browser or SOUND EDIT
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
    if (argc > 1 && juce::String (argv[1]) == "-names")   // -names : index | name | old name | category | id
    {
        int i = 0;
        for (auto& pr : factoryPresets())
            std::printf ("%d|%s|%s|%s|%s\n", i++, pr.name.toRawUTF8(), pr.legacyName.toRawUTF8(), categoryNames()[pr.cat].toRawUTF8(), pr.id.toRawUTF8());
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
        const juce::String th (argv[3]);   // glass | night (old: a number)
        juce::PropertiesFile (o).setValue ("theme", th.containsIgnoreCase ("night") || th == "1" ? 1 : 0);
        juce::PropertiesFile (o).setValue ("scale", argc > 5 ? juce::String (argv[5]).getIntValue() : 60);
        p.setCurrentProgram (1);
        if (argc > 4 && juce::String (argv[4]).getIntValue() >= 90)   // 90..98: the EVOLVE FX PRO pages
        {
            juce::Image kkFxSnapshot (KeysKillaProcessor&, int);
            p.prepareToPlay (44100, 512);
            if (juce::String (argv[4]).getIntValue() == 92) p.chainApply (5);
            auto img = kkFxSnapshot (p, juce::String (argv[4]).getIntValue() - 90);
            juce::File out (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]));
            out.deleteFile(); juce::FileOutputStream os (out); juce::PNGImageFormat().writeImageToStream (img, os);
            return 0;
        }
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
    failures += libraryCheck (p, verbose);
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
