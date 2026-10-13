// CHROME VAULT unit tests:  VaultTests [-unit]          all tests, prints "vault failures: N", exit code = N
//                           VaultTests -wav <index> <out.wav> [note] [seconds]   render a preset of the loaded bank
//                           VaultTests -bankinfo                 bank summary (source, per-category / per-layer counts)
//                           VaultTests -writestarter <out.cvb>   write the starter bank as a .cvb file
//                           VaultTests -snapshot <out.png>       paint the editor into a PNG (Linux: under xvfb-run)
//                           VaultTests -uitest                   editor behaviour (keys, filters, favorites, persistence, WAV drag)
//                           VaultTests -shot <out.png> [state]   the browser UI with a 3-layer preset + a filter (xvfb-run)
//                                      state: comma list of  scale:<pct> search:<text> cat:<n> style:<n> char:<n> layers:<n>
//                                      favs recent empty keys   (default "main" = its category + a style chip + a held chord)
//                                      Without an embedded bank / CHROME_VAULT_BANK a 5,000-preset demo bank is generated.
#include "../Source/vault/VaultProcessor.h"
#include "../Source/vault/VaultEditor.h"
#include "../Source/vault/ui/VaultMainView.h"
#include "../Source/ParamKeys.h"
#include "../tools/vault/BankQC.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <cstdio>
#include <set>
#include <thread>

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

// the shipped factory bank (resources/vault/*.cvb, or the CHROME_VAULT_BANK file): exact counts, unique ids / names, valid tags,
// clean names, and a deterministic random sample of 200 presets through the generator's quality gate (tools/vault/BankQC.h)
void shippedBankTests (const VaultProcessor& proc)
{
    const auto& bank = proc.bank();
    check (proc.bankSource() != VaultProcessor::BankSource::factoryFallback, "factory bank loaded: " + proc.bankDescription());
    check ((int) bank.size() == VaultProcessor::maxPresets, "the bank has exactly " + juce::String (VaultProcessor::maxPresets) + " presets ("
                                                             + juce::String ((int) bank.size()) + ")");
    int cats[vault::numCategories] {}, layers[vault::maxParts + 1] {};
    for (auto& p : bank) { ++cats[juce::jlimit (0, vault::numCategories - 1, p.category)]; ++layers[juce::jlimit (0, (int) vault::maxParts, (int) p.parts.size())]; }
    bool catsOk = true, layersOk = true;
    for (int c = 0; c < vault::numCategories; ++c)
        if (cats[c] != vault::categoryTarget (c)) { catsOk = false; std::printf ("   %s: %d / %d\n", vault::categoryNames()[c].toRawUTF8(), cats[c], vault::categoryTarget (c)); }
    for (int l = 1; l <= vault::maxParts; ++l)
        if (layers[l] != vault::layerCountTarget (l)) { layersOk = false; std::printf ("   %d layers: %d / %d\n", l, layers[l], vault::layerCountTarget (l)); }
    check (catsOk, "presets per category match the plan (808 & Bass 520 ... FX & Atmos 240)");
    check (layersOk, "layer mix matches the plan (1600 / 1900 / 1100 / 400)");

    std::set<juce::String> ids, names;
    int badName = 0, badTags = 0, badLabel = 0;
    auto bits = [] (uint32_t m) { int n = 0; for (; m != 0; m &= m - 1) ++n; return n; };
    for (auto& p : bank)
    {
        ids.insert (p.id);
        names.insert (p.name.toLowerCase());
        const bool nameOk = p.name.trim() == p.name && p.name.isNotEmpty() && p.name.length() <= 22 && vaultqc::isCleanName (p.name)
                            && p.name.containsOnly ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -'&");
        if (! nameOk && badName++ < 5) std::printf ("   bad name: '%s'\n", p.name.toRawUTF8());
        const bool tagsOk = (p.styles >> vault::numStyles) == 0 && (p.chars >> vault::numCharacters) == 0
                            && bits (p.styles) >= 1 && bits (p.styles) <= 3 && bits (p.chars) >= 1 && bits (p.chars) <= 3;
        if (! tagsOk && badTags++ < 5) std::printf ("   bad tags: %s styles %x chars %x\n", p.id.toRawUTF8(), p.styles, p.chars);
        for (auto& part : p.parts)
            if ((part.label.isEmpty() || part.label.length() > 16 || ! vaultqc::isCleanName (part.label)) && badLabel++ < 5)
                std::printf ("   bad part label: %s '%s'\n", p.id.toRawUTF8(), part.label.toRawUTF8());
    }
    check (ids.size() == bank.size(), "preset ids are unique (" + juce::String ((int) ids.size()) + ")");
    check (names.size() == bank.size(), "preset names are unique (" + juce::String ((int) names.size()) + ")");
    check (badName == 0, "names: English, <= 22 characters, no brand words (" + juce::String (badName) + " bad)");
    check (badTags == 0, "tags: 1-3 styles and 1-3 characters from the fixed lists (" + juce::String (badTags) + " bad)");
    check (badLabel == 0, "part labels set, short, clean (" + juce::String (badLabel) + " bad)");

    // a deterministic random sample through the quality gate (two notes, 48 kHz - exactly the generator's gate)
    juce::Random rnd (20261013);
    std::set<int> chosen;
    while ((int) chosen.size() < std::min (200, (int) bank.size())) chosen.insert (rnd.nextInt ((int) bank.size()));
    const std::vector<int> sample (chosen.begin(), chosen.end());
    std::vector<juce::String> why (sample.size());
    std::vector<vaultqc::Print> prints (sample.size());
    std::vector<float> loud (sample.size());
    std::atomic<int> next { 0 };
    std::vector<std::thread> pool;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (int t = 0; t < (int) std::max (1u, std::min (8u, std::thread::hardware_concurrency())); ++t)
        pool.emplace_back ([&]
        {
            for (int k = next++; k < (int) sample.size(); k = next++)
            {
                const auto& p = bank[(size_t) sample[(size_t) k]];
                const auto aL = vaultqc::renderAndAnalyse (p, vaultqc::noteLow (p.category));
                const auto aH = vaultqc::renderAndAnalyse (p, vaultqc::noteHigh (p.category));
                auto w = vaultqc::gate (aL, p.category, 1.0f, false);
                if (w.isEmpty()) w = vaultqc::gate (aH, p.category, 1.0f, false);
                loud[(size_t) k] = 0.5f * (aL.loud + aH.loud);
                if (w.isEmpty() && std::abs (loud[(size_t) k] - vaultqc::targetLoudness (p.category)) > vaultqc::loudnessTolerance)
                    w = "loudness " + juce::String (loud[(size_t) k], 2) + " dB";
                why[(size_t) k] = w;
                prints[(size_t) k] = aL.print;
            }
        });
    for (auto& th : pool) th.join();
    int failed = 0;
    float worstDev = 0;
    for (size_t k = 0; k < sample.size(); ++k)
    {
        const auto& p = bank[(size_t) sample[k]];
        worstDev = std::max (worstDev, std::abs (loud[k] - vaultqc::targetLoudness (p.category)));
        if (why[k].isNotEmpty() && failed++ < 8) std::printf ("   gate: %s %s: %s\n", p.id.toRawUTF8(), p.name.toRawUTF8(), why[k].toRawUTF8());
    }
    check (failed == 0, juce::String ((int) sample.size()) + " random presets pass the quality gate (" + juce::String (failed) + " failed, loudness within "
                        + juce::String (worstDev, 2) + " dB of the category targets, " + juce::String ((juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0, 1) + " s)");
    float closest = 1.0e9f;   // no two sampled presets sound alike (low-note fingerprint)
    for (size_t a = 0; a < prints.size(); ++a)
        for (size_t b = a + 1; b < prints.size(); ++b) closest = std::min (closest, vaultqc::printDistance (prints[a], prints[b]));
    check (closest > 1.0f, "sampled presets are distinct (closest fingerprint distance " + juce::String (closest, 2) + ")");
}

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

   #if CV_HAS_BANK
    shippedBankTests (proc);
   #else
    if (proc.bankSource() == VaultProcessor::BankSource::file) shippedBankTests (proc);
   #endif

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
// browser logic (search, filters, favorites, recent, stepping) on a small hand-made bank
int browserTests()
{
    std::vector<VaultPreset> bank;
    auto mk = [&] (const char* id, const char* name, int cat, uint32_t st, uint32_t ch, std::vector<const char*> labels)
    {
        VaultPreset p; p.id = id; p.name = name; p.category = cat; p.styles = st; p.chars = ch;
        for (auto* l : labels) { VaultPart part; part.label = l; p.parts.push_back (part); }
        bank.push_back (p);
    };
    using namespace vault;
    mk ("t.0", "Frozen Ice", catBellsMallets, 1u << styleTrap, 1u << charDark, { "Ice Bell", "Dark Pad" });
    mk ("t.1", "Velvet Keys", catPianoKeys, 1u << styleLoFi, 1u << charWarm, { "Felt Piano" });
    mk ("t.2", "Glide Monster", cat808Bass, 1u << styleRage, 1u << charHard, { "Glide 808", "Grit", "Sub" });
    mk ("t.3", "Holy Air", catPads, 1u << styleAmbient, 1u << charWide, { "Air", "Choir Pad", "Drone", "Vinyl" });
    mk ("t.4", "Night Bell", catBellsMallets, 1u << styleDrill, 1u << charBright, { "Glock" });
    VaultBrowserModel m (bank);
    check (m.visible().size() == 5 && m.categoryCount (catBellsMallets) == 2 && m.categoryCount (-1) == 5, "browser: all sounds + category counts");
    m.filter.query = "dark bell"; m.refilter();
    check (m.visible() == std::vector<int> { 0 }, "browser: search 'dark bell' finds by tag + category");
    m.filter.query = "glide 808"; m.refilter();
    check (m.visible() == std::vector<int> { 2 }, "browser: search matches layer labels / category");
    m.filter.query = "FELT"; m.refilter();
    check (m.visible() == std::vector<int> { 1 }, "browser: search is case-insensitive");
    m.filter.query = {}; m.filter.layers = 3; m.refilter();
    check (m.visible() == std::vector<int> { 2 }, "browser: layers chip");
    m.filter.layers = 0; m.filter.category = catBellsMallets; m.refilter();
    check (m.visible() == (std::vector<int> { 0, 4 }), "browser: category");
    m.filter.style = styleDrill; m.refilter();
    check (m.visible() == std::vector<int> { 4 }, "browser: style chip");
    m.filter = {}; m.filter.character = charWide; m.refilter();
    check (m.visible() == std::vector<int> { 3 }, "browser: character chip");
    m.filter = {}; m.refilter();
    check (m.step (0, 1, false) == 1 && m.step (4, 1, false) == 4 && m.step (4, 1, true) == 0 && m.step (0, -1, true) == 4,
           "browser: step clamps (up / down) and wraps (arrows)");
    m.filter.category = catBellsMallets; m.refilter();
    check (m.step (1, 1, true) == 0 && m.step (0, 1, true) == 4, "browser: step goes through the filtered list only");
    m.setFavoriteIds ({ "t.3", "nope" });
    m.filter = {}; m.filter.favoritesOnly = true; m.refilter();
    check (m.visible() == std::vector<int> { 3 } && m.favoriteIds() == juce::StringArray { "t.3" }, "browser: favorites by id");
    for (int i = 0; i < 40; ++i) m.pushRecent (i % 5);
    m.pushRecent (2);
    m.filter = {}; m.filter.recentOnly = true; m.refilter();
    check (m.visible().front() == 2 && m.visible().size() == 5, "browser: recent newest first, no duplicates");
    VaultBrowserModel m2 (bank);
    m2.setRecentIds (m.recentIds());
    check (m2.recentList() == m.recentList(), "browser: recent restores by id");
    // 5,000 presets: filtering per keystroke stays fast
    std::vector<VaultPreset> big;
    for (int i = 0; i < 5000; ++i) { auto p = bank[(size_t) (i % 5)]; p.id = "b." + juce::String (i); big.push_back (p); }
    VaultBrowserModel mb (big);
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (auto q : { "d", "da", "dar", "dark", "dark b", "dark be", "dark bel", "dark bell" }) { mb.filter.query = q; mb.refilter(); }
    const double ms = (juce::Time::getMillisecondCounterHiRes() - t0) / 8.0;
    check (mb.visible().size() == 1000 && ms < 15.0, "browser: 5,000-row search per keystroke " + juce::String (ms, 2) + " ms");
    return fails;
}

// a 5,000-preset demo bank from the starter sounds (1-4 layers, invented names) - only for UI snapshots
std::vector<VaultPreset> demoBank()
{
    const auto fb = VaultProcessor::buildFallbackBank();
    std::vector<int> byCat[vault::numCategories];
    for (int i = 0; i < (int) fb.size(); ++i) byCat[fb[(size_t) i].category].push_back (i);
    static const char* labels[vault::numCategories][5] {
        { "Glide 808", "Sub", "Grit", "Reese", "Punch" }, { "Ice Bell", "Music Box", "Glock", "Celesta", "Marimba" },
        { "Felt Piano", "Grand", "E-Piano", "Dark Keys", "Upright" }, { "Glass Pluck", "Rage Pluck", "Harp", "Nylon Pick", "Zap" },
        { "Saw Lead", "Whistle", "Rage Lead", "Square", "Porta" }, { "Dark Pad", "Warm Pad", "Air", "Choir Pad", "Drone" },
        { "Ensemble", "Pizz", "Solo Violin", "Cello", "Tremolo" }, { "Horn Stab", "Brass Swell", "Trumpet", "Tuba", "Drill Horn" },
        { "Flute", "Pan Flute", "Ocarina", "Clarinet", "Breath" }, { "Nylon", "Acoustic", "Ambient Gtr", "Clean Elec", "Harmonics" },
        { "Ooh", "Aah", "Vox Chop", "Choir", "Whisper" }, { "Supersaw", "FM Keys", "Retro Poly", "Chip", "Wobble" },
        { "Trap Arp", "Glitch Seq", "Gate", "Bounce", "Ripple" }, { "Orch Hit", "Timpani", "Full Orch", "Horns", "Strings Hi" },
        { "Koto", "Sitar", "Kalimba", "Steel Drum", "Erhu" }, { "Riser", "Impact", "Texture", "Vinyl", "Wind" } };
    static const char* adj[] { "Midnight", "Frozen", "Chrome", "Hollow", "Velvet", "Neon", "Ghost", "Silver", "Crystal", "Black",
                               "Lunar", "Toxic", "Faded", "Cold", "Holy", "Static", "Glass", "Venom", "Silk", "Ashen" };
    juce::Random r (7);
    std::vector<VaultPreset> out;
    for (int c = 0; c < vault::numCategories; ++c)
        for (int k = 0; k < vault::categoryTarget (c); ++k)
        {
            const float u = r.nextFloat();
            const int n = u < 0.32f ? 1 : u < 0.70f ? 2 : u < 0.92f ? 3 : 4;
            VaultPreset p;
            p.id = "cv.demo." + juce::String ((int) out.size());
            p.category = c;
            for (int i = 0; i < n; ++i)
            {
                const int cat = i == 0 ? c : r.nextInt (vault::numCategories);
                const auto& pool = byCat[cat].empty() ? byCat[vault::catPads] : byCat[cat];
                const auto& src = fb[(size_t) pool[(size_t) r.nextInt ((int) pool.size())]];
                VaultPart part = src.parts.front();
                part.label = labels[cat][r.nextInt (5)];
                part.level = std::array<float, 4> { 1.0f, 0.72f, 0.55f, 0.4f }[(size_t) i];
                if (i > 0 && r.nextInt (4) == 0) part.octave = r.nextBool() ? 1 : -1;
                if (i == 0) { p.styles = src.styles; p.chars = src.chars; }
                p.parts.push_back (part);
            }
            p.styles |= 1u << r.nextInt (vault::numStyles);
            p.chars |= 1u << r.nextInt (vault::numCharacters);
            p.name = juce::String (adj[r.nextInt (20)]) + " " + labels[c][k % 5] + (n > 1 ? " Stack" : "");
            out.push_back (std::move (p));
        }
    return out;
}

// UI runs: a private settings file, and the 5,000 demo bank when no real bank is available
juce::File uiSetup (const juce::String& name)
{
    const auto tmp = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("cv-shot");
    tmp.createDirectory();
    VaultSettings::fileOverride() = tmp.getChildFile (name + ".settings");
    VaultSettings::fileOverride().deleteFile();
   #if ! CV_HAS_BANK
    if (juce::SystemStats::getEnvironmentVariable ("CHROME_VAULT_BANK", {}).isEmpty())
    {
        const auto f = tmp.getChildFile ("demo.cvb");
        if (! f.existsAsFile()) vault::saveBankFile (f, demoBank());
       #if JUCE_WINDOWS
        _putenv_s ("CHROME_VAULT_BANK", f.getFullPathName().toRawUTF8());
       #else
        setenv ("CHROME_VAULT_BANK", f.getFullPathName().toRawUTF8(), 1);
       #endif
    }
   #endif
    return tmp;
}

// editor behaviour (run under xvfb-run): keys step through the filtered list, favorites / recent / scale persist,
// host automation is followed, DRAG WAV renders a valid file
int uiTests()
{
    uiSetup ("uitest");
    VaultProcessor p;
    const auto& bank = p.bank();
    int fav = -1, pads0 = -1;
    {
        VaultEditor ed (p);
        auto& v = ed.view();
        check (ed.getWidth() >= 826 && ed.getWidth() <= 1180 && std::abs ((double) ed.getWidth() / ed.getHeight() - 1180.0 / 760.0) < 0.01,
               "ui: first-open size " + juce::String (ed.getWidth()) + "x" + juce::String (ed.getHeight()));
        v.setCategory (vault::catPads);
        const auto rows = v.browser().visible();
        check (! rows.empty() && v.visibleCount() == (int) std::count_if (bank.begin(), bank.end(), [] (const VaultPreset& x) { return x.category == vault::catPads; }),
               "ui: category filter (" + juce::String (v.visibleCount()) + " pads)");
        pads0 = rows.front();
        v.loadPreset (pads0, true);
        check (p.currentPreset() == pads0 && v.shownPreset() == pads0, "ui: click loads the sound");
        ed.keyPressed (juce::KeyPress (juce::KeyPress::downKey));
        check (p.currentPreset() == rows[1], "ui: down key loads the next sound of the filtered list");
        ed.keyPressed (juce::KeyPress (juce::KeyPress::upKey));
        ed.keyPressed (juce::KeyPress (juce::KeyPress::upKey));
        check (p.currentPreset() == pads0, "ui: up key, clamped at the top");
        ed.keyPressed (juce::KeyPress (juce::KeyPress::leftKey));
        check (p.currentPreset() == rows.back(), "ui: left arrow wraps to the end");
        v.setLayers (2);
        v.setQuery ("stack");
        bool allOk = v.visibleCount() > 0;
        for (int i : v.browser().visible()) allOk &= bank[(size_t) i].parts.size() == 2 && bank[(size_t) i].category == vault::catPads;
        check (allOk, "ui: search + layers chip + category combine (" + juce::String (v.visibleCount()) + ")");
        v.setQuery ({}); v.setLayers (0); v.setCategory (-1);
        fav = rows[2];
        v.toggleFavorite (fav);
        v.setFavoritesOnly (true);
        check (v.visibleCount() == 1 && v.browser().visible().front() == fav, "ui: favorites filter");
        v.setFavoritesOnly (false);
        v.randomSound();
        check (p.currentPreset() >= 0, "ui: random loads a sound");
        v.loadPreset (pads0, false);
        p.selectPreset (7);                           // host / automation side
        v.syncFromProcessor();
        check (v.shownPreset() == 7, "ui: follows a preset change from the host");
        const auto wavFile = v.renderWavForDrag();
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (wavFile));
        float pk = 0.0f;
        if (rd != nullptr) { juce::AudioBuffer<float> b (2, (int) rd->lengthInSamples); rd->read (&b, 0, b.getNumSamples(), 0, true, true); pk = b.getMagnitude (0, b.getNumSamples()); }
        check (rd != nullptr && rd->numChannels == 2 && rd->lengthInSamples == 88200 && pk > 0.01f && pk < 1.0f,
               "ui: DRAG WAV file " + wavFile.getFileName() + " (peak " + juce::String (pk, 3) + ")");
        ed.setUiScale (1.25f);
        check (ed.getWidth() == 1475 && ed.getHeight() == 950, "ui: scale 125 %");
        ed.setUiScale (3.0f);
        check (ed.getWidth() == 1770, "ui: scale clamps at 150 %");
        ed.setUiScale (1.25f);
    }
    {
        VaultEditor ed (p);   // reopened: favorites, recent and size are remembered
        auto& v = ed.view();
        check (v.browser().isFavorite (fav) && v.browser().numFavorites() == 1, "ui: favorites persist");
        check (! v.browser().recentList().empty() && v.browser().recentList().front() == pads0, "ui: recent persists (newest first)");
        check (ed.getWidth() == 1475, "ui: window size persists");
        const auto f = VaultSettings::fileOverride();
        check (f.existsAsFile() && f.loadFileAsString().contains (bank[(size_t) fav].id), "ui: settings file " + f.getFileName());
    }
    return fails;
}

int shot (const juce::String& outPath, const juce::String& stateArg)
{
    uiSetup ("shot");
    VaultSettings::get()->setScale (1.0f);   // shots start at 100 % ("scale:<pct>" changes it)
    VaultProcessor p;
    auto ed = std::make_unique<VaultEditor> (p);
    auto& v = ed->view();
    const auto& bank = p.bank();
    int pick = -1;
    for (int i = 0; i < (int) bank.size() && pick < 0; ++i)
        if (bank[(size_t) i].parts.size() == 3 && bank[(size_t) i].category == vault::catBellsMallets && bank[(size_t) i].styles != 0) pick = i;
    for (int i = 0; i < (int) bank.size() && pick < 0; ++i) if (bank[(size_t) i].parts.size() == 3) pick = i;
    if (pick < 0) pick = 0;
    for (int i = 0; i < 6; ++i) v.loadPreset ((pick + 7 * i + 3) % (int) bank.size(), false);   // some history for RECENT
    v.loadPreset (pick, false);
    const auto& pr = bank[(size_t) pick];

    auto tokens = juce::StringArray::fromTokens (stateArg.isEmpty() ? "main" : stateArg, ",", "");
    for (auto t : tokens)
    {
        const auto key = t.upToFirstOccurrenceOf (":", false, false).trim(), val = t.fromFirstOccurrenceOf (":", false, false).trim();
        if (key == "main")
        {
            v.setCategory (pr.category);
            for (int s = 0; s < vault::numStyles; ++s) if (pr.styles & (1u << s)) { v.setStyle (s); break; }
        }
        if (key == "main" || key == "keys") for (int n : { 60, 64, 67 }) p.getKeyboardState().noteOn (1, n, 0.8f);
        else if (key == "scale")   ed->setUiScale (val.getFloatValue() / 100.0f);
        else if (key == "search")  v.setQuery (val);
        else if (key == "cat")     v.setCategory (val.getIntValue());
        else if (key == "style")   v.setStyle (val.getIntValue());
        else if (key == "char")    v.setCharacter (val.getIntValue());
        else if (key == "layers")  v.setLayers (val.getIntValue());
        else if (key == "empty")   v.setQuery ("zzqx nothing");
        else if (key == "recent")  v.setRecentOnly (true);
        else if (key == "favs")
        {
            for (int i = 0; i < 9; ++i) v.toggleFavorite ((pick + i * 13) % (int) bank.size());
            v.setFavoritesOnly (true);
        }
    }
    v.loadPreset (pick, false);
    v.thumbnails().waitFor (pick, 10000);
    const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    const juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (outPath);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat png;
    const bool ok = os.openedOk() && png.writeImageToStream (img, os);
    std::printf ("%s | preset %d \"%s\" (%d layers) | showing %d of %d | editor %dx%d -> %s %s\n", p.bankDescription().toRawUTF8(), pick,
                 pr.name.toRawUTF8(), (int) pr.parts.size(), v.visibleCount(), (int) bank.size(), ed->getWidth(), ed->getHeight(),
                 outPath.toRawUTF8(), ok ? "ok" : "FAILED");
    for (int n : { 60, 64, 67 }) p.getKeyboardState().noteOff (1, n, 0.0f);
    ed.reset();
    return ok ? 0 : 1;
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
    if (mode == "-uitest")
    {
        const int f = uiTests();
        std::printf ("vault ui failures: %d\n", f);
        return f;
    }
    if (mode == "-shot" && argc > 2)
        return shot (argv[2], argc > 3 ? juce::String (argv[3]) : juce::String());
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
    unitTests();
    const int f = browserTests();
    std::printf ("vault failures: %d\n", f);
    return f;
}
