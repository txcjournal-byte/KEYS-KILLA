// CHROME VAULT unit tests:  VaultTests [-unit]          all tests, prints "vault failures: N", exit code = N
//                           VaultTests -wav <index> <out.wav> [note] [seconds]   render a preset of the loaded bank
//                           VaultTests -bankinfo                 bank summary (source, per-category / per-layer counts)
//                           VaultTests -writestarter <out.cvb>   write the starter bank as a .cvb file
//                           VaultTests -snapshot <out.png>       paint the editor into a PNG (Linux: under xvfb-run)
#include "../Source/vault/VaultProcessor.h"
#include "../Source/ParamKeys.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>
#include <set>

namespace
{
int fails = 0;
void check (bool ok, const juce::String& what)
{
    std::printf ("%s %s\n", ok ? "   ok:" : "!! FAIL:", what.toRawUTF8());
    if (! ok) ++fails;
}

struct Stats { float peak = 0, rms = 0; bool finite = true; float maxJump = 0; };
Stats stats (const juce::AudioBuffer<float>& b, int from = 0, int to = -1)
{
    Stats s;
    if (to < 0) to = b.getNumSamples();
    double sum = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
    {
        const float* x = b.getReadPointer (c);
        for (int i = from; i < to; ++i)
        {
            if (! std::isfinite (x[i])) { s.finite = false; continue; }
            s.peak = std::max (s.peak, std::abs (x[i]));
            sum += (double) x[i] * x[i];
            if (i > from) s.maxJump = std::max (s.maxJump, std::abs (x[i] - x[i - 1]));
        }
    }
    s.rms = (float) std::sqrt (sum / std::max (1, (to - from) * b.getNumChannels()));
    return s;
}

// RMS of the side (8th-order low-pass) and mid (4th-order low-pass) signals below `hz`
std::pair<float, float> lowSideMid (const juce::AudioBuffer<float>& b, double sr, double hz)
{
    struct Bq { double b0, b1, b2, a1, a2, z1 = 0, z2 = 0; double tick (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; } };
    auto lp = [&] (double q) { const double w = juce::MathConstants<double>::twoPi * hz / sr, cs = std::cos (w), al = std::sin (w) / (2 * q), a0 = 1 + al;
                               return Bq { (1 - cs) / 2 / a0, (1 - cs) / a0, (1 - cs) / 2 / a0, -2 * cs / a0, (1 - al) / a0 }; };
    Bq s1 = lp (0.5412), s2 = lp (1.3066), s3 = lp (0.5412), s4 = lp (1.3066), m1 = lp (0.5412), m2 = lp (1.3066);
    double es = 0, em = 0;
    const float* L = b.getReadPointer (0); const float* R = b.getReadPointer (1);
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double s = s4.tick (s3.tick (s2.tick (s1.tick ((L[i] - R[i]) * 0.5)))), m = m2.tick (m1.tick ((L[i] + R[i]) * 0.5));
        es += s * s; em += m * m;
    }
    const double n = std::max (1, b.getNumSamples());
    return { (float) std::sqrt (es / n), (float) std::sqrt (em / n) };
}

VaultPart part (std::initializer_list<std::pair<const char*, float>> vals, float level = 1, float pan = 0, int oct = 0, const char* label = "Layer")
{
    VaultPart p;
    for (auto& [k, v] : vals) p.values.push_back ({ k, v });
    p.level = level; p.pan = pan; p.octave = oct; p.label = label;
    return p;
}

VaultPreset preset (const char* id, const char* name, int cat, std::vector<VaultPart> parts)
{
    VaultPreset p;
    p.id = id; p.name = name; p.category = cat; p.parts = std::move (parts);
    p.styles = 1u << vault::styleTrap; p.chars = 1u << vault::charWide;
    return p;
}

// a few hand-made multi-layer presets: wide pads, a mono 808, hard-panned layers, a huge reverb
std::vector<VaultPreset> testPresets()
{
    std::vector<VaultPreset> v;
    v.push_back (preset ("t.pad", "Test Wide Pad", vault::catPads, {
        part ({ { ID::engine, 0 }, { ID::wave, 0.5f }, { ID::unison, 7 }, { ID::detune, 0.6f }, { ID::attack, 0.3f }, { ID::sustain, 1 }, { ID::release, 1.2f },
                { ID::cutoff, 3000 }, { ID::chorus, 0.6f }, { ID::revMix, 0.5f }, { ID::width, 1.0f } }, 1, -1, 0, "Wide Saw"),
        part ({ { ID::engine, 7 }, { ID::wave, 0.4f }, { ID::unison, 5 }, { ID::detune, 0.8f }, { ID::sustain, 1 }, { ID::release, 1.5f } }, 0.8f, 1, -1, "Low Glass") }));
    v.push_back (preset ("t.808", "Test Sub Glide", vault::cat808Bass, {
        part ({ { ID::engine, 6 }, { ID::mono, 1 }, { ID::bassMode, 1 }, { ID::glide, 0.1f }, { ID::sustain, 0.7f }, { ID::decay, 1.2f }, { ID::drive, 0.3f },
                { ID::octave, -1 } }, 1, 0, 0, "Sub"),
        part ({ { ID::engine, 0 }, { ID::wave, 1 }, { ID::unison, 6 }, { ID::detune, 0.7f }, { ID::cutoff, 900 }, { ID::sustain, 0.5f } }, 0.5f, 0.8f, 0, "Grit") }));
    v.push_back (preset ("t.bell", "Test Four Layer Bell", vault::catBellsMallets, {
        part ({ { ID::engine, 1 }, { ID::fmAlgo, 4 }, { ID::fmAmt, 0.5f }, { ID::sustain, 0 }, { ID::decay, 1.5f }, { ID::revMix, 0.4f }, { ID::delayMix, 0.3f } }, 1, -0.5f, 0, "FM Bell"),
        part ({ { ID::engine, 9 }, { ID::wave, 0.8f }, { ID::sustain, 0 }, { ID::decay, 2.0f } }, 0.7f, 0.5f, 1, "Glass"),
        part ({ { ID::engine, 2 }, { ID::wave, 0.6f }, { ID::sustain, 0 }, { ID::decay, 0.8f } }, 0.6f, -1, 0, "Pluck"),
        part ({ { ID::engine, 0 }, { ID::wave, 0.2f }, { ID::unison, 4 }, { ID::detune, 0.4f }, { ID::attack, 0.5f }, { ID::sustain, 0.8f } }, 0.4f, 1, -1, "Air") }));
    v.push_back (preset ("t.lead", "Test Hot Lead", vault::catLeads, {
        part ({ { ID::engine, 0 }, { ID::wave, 1 }, { ID::unison, 8 }, { ID::detune, 1 }, { ID::drive, 0.9f }, { ID::driveType, 3 }, { ID::gain, 12 }, { ID::sustain, 1 } }, 2, 0, 1, "Saw Stack") }));
    v.back().gain = 8.0f;   // far too hot on purpose: the safety limiter must hold the peak
    return v;
}

void writeWav (const juce::AudioBuffer<float>& b, double sr, const juce::File& f)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::OutputStream> (f.createOutputStream()))
        if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), sr, 2, 24, {}, 0)))
        {
            os.release();
            w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
        }
}

struct DummyProc : juce::AudioProcessor
{
    DummyProc() : apvts (*this, nullptr, "T", createLayout()) {}
    const juce::String getName() const override { return "T"; }
    void prepareToPlay (double, int) override {}
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override {}
    double getTailLengthSeconds() const override { return 0; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    juce::AudioProcessorEditor* createEditor() override { return nullptr; }
    bool hasEditor() const override { return false; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override {}
    void setStateInformation (const void*, int) override {}
    juce::AudioProcessorValueTreeState apvts;
};

int unitTests()
{
    const double sr = 44100.0;
    VaultProcessor proc;
    const auto& bank = proc.bank();
    std::printf ("bank: %s\n", proc.bankDescription().toRawUTF8());
    check (! bank.empty(), "a bank is loaded");

    // ---- the key table mirrors the real EVOLVE parameter layout
    {
        DummyProc d;
        int bad = 0;
        for (int k = 0; k < kk::vk::count; ++k)
        {
            const auto& in = kk::vk::info (k);
            auto* p = dynamic_cast<juce::RangedAudioParameter*> (d.apvts.getParameter (in.id));
            if (p == nullptr) { std::printf ("   missing param %s\n", in.id); ++bad; continue; }
            const auto& r = p->getNormalisableRange();
            const float def = p->convertFrom0to1 (p->getDefaultValue());
            const bool isInt = dynamic_cast<juce::AudioParameterFloat*> (p) == nullptr;
            if (std::abs (r.start - in.lo) > 1e-4f || std::abs (r.end - in.hi) > 1e-4f || std::abs (def - in.def) > 1e-3f || isInt != in.isInt)
            { std::printf ("   key %s: table %g..%g def %g int %d / layout %g..%g def %g int %d\n", in.id, in.lo, in.hi, in.def, (int) in.isInt,
                           r.start, r.end, def, (int) isInt); ++bad; }
        }
        check (bad == 0, "ParamKeys table matches createLayout() (" + juce::String (kk::vk::count) + " keys)");
        check (kk::vk::keyOf ("cutoff") == kk::vk::cutoff && kk::vk::keyOf ("mmAmt3") == kk::vk::mmAmt (2) && kk::vk::keyOf ("arp") < 0, "keyOf");
    }

    // ---- every preset of the loaded bank is valid
    {
        int invalid = 0; juce::String why;
        std::set<juce::String> ids;
        for (auto& p : bank) { if (! vault::validatePreset (p, &why)) { if (invalid++ < 5) std::printf ("   %s\n", why.toRawUTF8()); } ids.insert (p.id); }
        check (invalid == 0, "all " + juce::String ((int) bank.size()) + " bank presets validate");
        check (ids.size() == bank.size(), "preset ids are unique");
        auto fb = VaultProcessor::buildFallbackBank();
        int cats[vault::numCategories] {};
        for (auto& p : fb) ++cats[p.category];
        int used = 0; for (int c : cats) used += c > 0;
        check (fb.size() >= 150 && used >= 12, "fallback bank: " + juce::String ((int) fb.size()) + " presets in " + juce::String (used) + " categories");
    }

    // ---- serializer round trip (compressed and raw), damaged data is refused
    {
        auto all = VaultProcessor::buildFallbackBank();
        for (auto& t : testPresets()) all.push_back (t);
        all.back().parts[0].values.push_back ({ "someFutureParam", 0.25f });   // unknown IDs survive a round trip
        all.back().macroDefaults[2] = 0.8f; all.back().name = juce::CharPointer_UTF8 ("Caf\xc3\xa9 Lead");
        for (bool gz : { true, false })
        {
            const auto mb = vault::serializeBank (all, gz);
            std::vector<VaultPreset> back; juce::String err;
            const bool ok = vault::deserializeBank (mb.getData(), mb.getSize(), back, &err);
            check (ok && back == all, juce::String ("serializer round trip ") + (gz ? "gzip" : "raw") + " (" + juce::String ((int) mb.getSize() / 1024) + " KB, "
                                       + juce::String ((int) all.size()) + " presets) " + err);
        }
        auto mb = vault::serializeBank (all, true);
        std::vector<VaultPreset> back;
        juce::MemoryBlock bad (mb); static_cast<char*> (bad.getData())[2] = 'X';
        check (! vault::deserializeBank (bad.getData(), bad.getSize(), back) && back.empty(), "bad magic refused");
        juce::MemoryBlock cut (mb.getData(), mb.getSize() / 2);
        check (! vault::deserializeBank (cut.getData(), cut.getSize(), back) && back.empty(), "truncated bank refused");
        const auto f = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("vault_test.cvb");
        check (vault::saveBankFile (f, all) && vault::loadBankFile (f, back) && back == all, "bank file save / load");
        f.deleteFile();
    }

    // ---- offline render: sound, finite, no clipping, mono lows, clean start
    {
        std::vector<VaultPreset> set = testPresets();
        int perCat[vault::numCategories] {};
        for (auto& p : bank) if (perCat[p.category]++ < 2) set.push_back (p);   // two of every category of the loaded bank
        int silent = 0, nonFinite = 0, hot = 0, wideLows = 0, startClicks = 0;
        float worstPeak = 0, worstSide = 0;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (auto& p : set)
        {
            const int note = p.category == vault::cat808Bass ? 36 : 60;
            const auto b = VaultEngine::render (p, note, 0.8f, 2.0, sr);
            const auto s = stats (b);
            silent += s.rms < 1.0e-4f; nonFinite += ! s.finite; hot += s.peak >= 0.99f;
            worstPeak = std::max (worstPeak, s.peak);
            // mono lows: a low note (lots of energy under 120 Hz), side under 90 Hz against the whole signal
            const auto lowB = VaultEngine::render (p, 36, 0.8f, 2.0, sr);
            const auto [side, mid] = lowSideMid (lowB, sr, 90.0);
            const float ratio = side / std::max (1.0e-6f, stats (lowB).rms);
            worstSide = std::max (worstSide, ratio);
            if (ratio > 0.03f) { ++wideLows; std::printf ("   wide lows (side %.4f mid %.4f rms %.4f): %s\n", side, mid, stats (lowB).rms, p.name.toRawUTF8()); }
            // note start: from silence the first samples are ~0 and the attack ramps (no step)
            const auto st = stats (b, 0, 16);
            if (std::abs (b.getSample (0, 0)) > 0.01f || std::abs (b.getSample (1, 0)) > 0.01f || st.maxJump > 0.5f * s.maxJump + 0.02f)
            { ++startClicks; std::printf ("   start jump %.3f (steady %.3f): %s\n", st.maxJump, s.maxJump, p.name.toRawUTF8()); }
            if (! s.finite || s.rms < 1.0e-4f || s.peak >= 0.99f) std::printf ("   %s: rms %.5f peak %.3f finite %d\n", p.name.toRawUTF8(), s.rms, s.peak, (int) s.finite);
        }
        const double ms = (juce::Time::getMillisecondCounterHiRes() - t0) / (double) set.size();
        check (silent == 0, "render(): " + juce::String ((int) set.size()) + " presets make sound (" + juce::String (ms, 1) + " ms per 2 s render)");
        check (nonFinite == 0, "render(): no NaN / inf");
        check (hot == 0, "render(): peak < 0.99 (worst " + juce::String (worstPeak, 3) + ", incl. an 8x too hot preset)");
        check (wideLows == 0, "mono below 120 Hz: side under 90 Hz < -30 dB of the signal (worst " + juce::String (juce::Decibels::gainToDecibels (worstSide), 1) + " dB)");
        check (startClicks == 0, "no clicks at note start");
        const auto a = VaultEngine::render (set[0], 60, 0.8f, 1.0, sr), b = VaultEngine::render (set[0], 60, 0.8f, 1.0, sr);
        bool same = true; for (int i = 0; i < a.getNumSamples(); ++i) same &= a.getSample (0, i) == b.getSample (0, i);
        check (same, "render() is deterministic");
        const float hiMacros[4] { 1, 1, 1, 1 }, loMacros[4] { 0, 0, 0, 0 };
        const auto hi = stats (VaultEngine::render (set[0], 60, 0.8f, 1.5, sr, hiMacros)), lo = stats (VaultEngine::render (set[0], 60, 0.8f, 1.5, sr, loMacros));
        check (hi.finite && lo.finite && hi.peak < 0.99f && lo.peak < 0.99f && hi.rms > 1e-4f && lo.rms > 1e-4f, "macros at 0 and 1 stay clean");
        {   // loudness of the starter bank (EVOLVE-calibrated): within a sane window around the target
            float lo = 0, hi = -100;
            for (size_t i = 4; i < set.size(); ++i)
            {
                const float l = VaultEngine::measureLoudnessDb (VaultEngine::render (set[i], set[i].category == vault::cat808Bass ? 36 : 60, 0.8f, 2.0, sr), sr);
                lo = std::min (lo, l); hi = std::max (hi, l);
            }
            check (hi - lo < 20.0f && hi < -3.0f, "loudness of the bank renders " + juce::String (lo, 1) + " .. " + juce::String (hi, 1) + " dB (target "
                                                   + juce::String (VaultEngine::targetLoudnessDb, 0) + ")");
        }
        const auto t1 = juce::Time::getMillisecondCounterHiRes();
        VaultEngine::render (set[2], 60, 0.8f, 10.0, sr);
        check (true, "CPU: 10 s of a 4-layer preset renders in " + juce::String (juce::Time::getMillisecondCounterHiRes() - t1, 0) + " ms");
        // note end: the note-off point is smooth
        const auto e = VaultEngine::render (set[0], 60, 0.8f, 2.0, sr);
        const int off = (int) (2.0 * sr * 0.7);
        check (stats (e, off - 64, off + 256).maxJump <= stats (e, off - 4096, off - 64).maxJump * 1.5f + 0.01f, "no click at note end");
    }

    // ---- preset switch while a note is held: 10 ms declick, no jump
    {
        auto tp = testPresets();
        VaultPreset a = preset ("t.sa", "Switch A", vault::catPads, { part ({ { ID::engine, 0 }, { ID::wave, 0 }, { ID::cutoff, 700 }, { ID::sustain, 1 }, { ID::revMix, 0 } }) });
        VaultPreset b = preset ("t.sb", "Switch B", vault::catPads, { part ({ { ID::engine, 0 }, { ID::wave, 0.3f }, { ID::cutoff, 1100 }, { ID::sustain, 1 }, { ID::revMix, 0 } }, 1, 0, 1) });
        const auto ca = VaultEngine::compile (a), cb = VaultEngine::compile (b), cpad = VaultEngine::compile (tp[0]);
        VaultEngine eng;
        eng.prepare (sr, 256);
        eng.setPreset (ca, true);
        const int block = 256, blocks = (int) (1.5 * sr / block);
        juce::AudioBuffer<float> all (2, blocks * block), buf (2, block);
        for (int k = 0; k < blocks; ++k)
        {
            juce::MidiBuffer m;
            if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 57, (juce::uint8) 100), 0);
            if (k == blocks / 2) eng.setPreset (cb);
            if (k == blocks * 3 / 4) eng.setPreset (cpad);
            eng.process (buf, m);
            for (int c = 0; c < 2; ++c) all.copyFrom (c, k * block, buf, c, 0, block);
        }
        const int sw = blocks / 2 * block, sw2 = blocks * 3 / 4 * block;
        const float steady = std::max (stats (all, sw - 8192, sw - 512).maxJump, stats (all, sw + 4096, sw + 8192).maxJump);
        const float atSwitch = stats (all, sw - 256, sw + 2048).maxJump;
        const float steady2 = std::max (stats (all, sw2 - 4096, sw2 - 512).maxJump, stats (all, sw2 + 6000, sw2 + 9000).maxJump);
        const float atSwitch2 = stats (all, sw2 - 256, sw2 + 2048).maxJump;
        check (atSwitch <= steady * 1.5f + 0.003f && atSwitch2 <= steady2 * 1.5f + 0.003f,
               "preset switch has no click (jump " + juce::String (atSwitch, 4) + " vs steady " + juce::String (steady, 4) + ", "
               + juce::String (atSwitch2, 4) + " vs " + juce::String (steady2, 4) + ")");
        check (stats (all, sw + 4096, sw + 8192).rms > 1e-3f, "the held note plays on with the new preset");
    }

    // ---- the plugin: preset parameter, MIDI, state save / restore
    {
        proc.setPlayConfigDetails (0, 2, sr, 512);
        proc.prepareToPlay (sr, 512);
        const int target = std::min (5, (int) bank.size() - 1);
        proc.selectPreset (target);
        check (proc.currentPreset() == target, "selectPreset is instant");
        juce::AudioBuffer<float> b (2, 512);
        float pk = 0; bool fin = true;
        for (int k = 0; k < 120; ++k)
        {
            juce::MidiBuffer m;
            if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 10);
            if (k == 60) m.addEvent (juce::MidiMessage::noteOff (1, 60), 0);
            proc.processBlock (b, m);
            const auto s = stats (b); pk = std::max (pk, s.peak); fin &= s.finite;
        }
        check (fin && pk > 1e-3f && pk < 0.99f, "processBlock plays (peak " + juce::String (pk, 3) + ")");
        proc.setMacro (1, 0.8f); proc.setVolumeDb (-3.0f);
        juce::MemoryBlock state;
        proc.getStateInformation (state);
        VaultProcessor other;
        other.setStateInformation (state.getData(), (int) state.getSize());
        check (other.currentPreset() == target && std::abs (other.getMacro (1) - 0.8f) < 1e-3f && std::abs (other.getVolumeDb() + 3.0f) < 0.05f
                   && other.currentPresetData()->id == bank[(size_t) target].id, "state save / restore (preset id, macros, volume)");
        proc.selectPreset ((int) bank.size() - 1);
        check (std::abs (proc.getMacro (0) - bank.back().macroDefaults[0]) < 1e-3f, "selecting a preset resets the macros to its defaults");
        auto* prm = proc.apvts.getParameter (VaultProcessor::idPreset);
        prm->setValueNotifyingHost (prm->convertTo0to1 (0.0f));   // host automation
        { juce::MidiBuffer none; proc.processBlock (b, none); }
        check (proc.currentPreset() == 0, "the preset parameter is automatable");
        proc.releaseResources();
    }
    return fails;
}
} // namespace

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String mode = argc > 1 ? juce::String (argv[1]) : juce::String ("-unit");
    if (mode == "-wav" && argc > 3)
    {
        VaultProcessor p;
        const int idx = juce::jlimit (0, (int) p.bank().size() - 1, juce::String (argv[2]).getIntValue());
        const int note = argc > 4 ? juce::String (argv[4]).getIntValue() : 60;
        const double secs = argc > 5 ? juce::String (argv[5]).getDoubleValue() : 3.0;
        const auto b = VaultEngine::render (p.bank()[(size_t) idx], note, 0.8f, secs, 44100.0);
        writeWav (b, 44100.0, juce::File::getCurrentWorkingDirectory().getChildFile (argv[3]));
        std::printf ("%s -> %s (peak %.3f)\n", p.bank()[(size_t) idx].name.toRawUTF8(), argv[3], b.getMagnitude (0, b.getNumSamples()));
        return 0;
    }
    if (mode == "-snapshot" && argc > 2)   // -snapshot <out.png>: the editor painted into an image (run under xvfb-run on Linux)
    {
        VaultProcessor p;
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        const juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        f.deleteFile();
        juce::FileOutputStream os (f);
        juce::PNGImageFormat png;
        const bool ok = os.openedOk() && png.writeImageToStream (img, os);
        std::printf ("editor %dx%d -> %s %s\n", ed->getWidth(), ed->getHeight(), argv[2], ok ? "ok" : "FAILED");
        ed.reset();
        return ok ? 0 : 1;
    }
    if (mode == "-writestarter" && argc > 2)   // -writestarter <out.cvb>: the starter bank as a bank file (format sample)
    {
        const auto f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        const auto b = VaultProcessor::buildFallbackBank();
        const bool ok = vault::saveBankFile (f, b);
        std::printf ("%d presets -> %s (%d KB) %s\n", (int) b.size(), argv[2], (int) (f.getSize() / 1024), ok ? "ok" : "FAILED");
        return ok ? 0 : 1;
    }
    if (mode == "-bankinfo")
    {
        VaultProcessor p;
        std::printf ("%s\n", p.bankDescription().toRawUTF8());
        int cats[vault::numCategories] {}, layers[vault::maxParts + 1] {};
        for (auto& pr : p.bank()) { ++cats[pr.category]; ++layers[juce::jlimit (0, (int) vault::maxParts, (int) pr.parts.size())]; }
        for (int c = 0; c < vault::numCategories; ++c)
            std::printf ("  %-18s %5d / %d\n", vault::categoryNames()[c].toRawUTF8(), cats[c], vault::categoryTarget (c));
        for (int l = 1; l <= vault::maxParts; ++l) std::printf ("  %d layer(s): %5d / %d\n", l, layers[l], vault::layerCountTarget (l));
        return 0;
    }
    const int f = unitTests();
    std::printf ("vault failures: %d\n", f);
    return f;
}
