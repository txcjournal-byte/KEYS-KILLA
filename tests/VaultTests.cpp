// CHROME VAULT unit tests:  VaultTests [-unit]          all tests, prints "vault failures: N", exit code = N
//                           VaultTests -wav <index> <out.wav> [note] [seconds]   render a preset of the loaded bank
//                           VaultTests -bankinfo                 bank summary (source, per-category / per-layer counts)
//                           VaultTests -libreport <dir> [notes]  render every library instrument / articulation (default at C5 =
//                                      MIDI 60), print pitch / loudness / spectrum / loop checks, write WAVs into <dir>
//                           VaultTests -writestarter <out.cvb>   write the starter bank as a .cvb file
//                           VaultTests -writev2demo <out.cvb>    a few v2 presets (sample / model parts) on the test library
//                           VaultTests -snapshot <out.png>       paint the editor into a PNG (Linux: under xvfb-run)
//                           VaultTests -uitest                   editor behaviour (keys, filters, favorites, persistence, WAV drag)
//                           VaultTests -models                   the physical-model tests only (tests/VaultModelTests.cpp)
//                           VaultTests -shot <out.png> [state]   the editor with a 3-layer preset (xvfb-run); state = comma list:
//                                                                page:main|layers|fx|mod|arp, scale:80, rich, demo, pick:<name>,
//                                                                search:<text>, cat:<n>, recent, favs, empty, knobs, nowait
//                                      Without an embedded bank / CHROME_VAULT_BANK a 5,000-preset demo bank is generated.
//                           VaultTests -writeuidemo <out.cvb>    the v2 UI demo bank (sampled / model / synth layers) for -shot
//                           VaultTests -uitestv2                 v2 editor checks (child process of -uitest)
//                           VaultTests -uifix [keys|pages|values|render|more]   the owner-report checks only (part of -uitest)
//                           VaultTests -pitch [--only <text>] [--step n] [--bank f.cvb] [--csv f] [-v]   every preset, every key
//                                      C2..C7 at its own pitch (tests/VaultPitchTests.cpp)
//                           VaultTests -v3                       the v3 tests only (tests/VaultV3Tests.cpp; part of -unit)
//                           VaultTests -convcheck <bank.cvb> [step] [threads]   v1 / v2 bank: frozen v2 engine vs the v3 conversion
#include "../Source/vault/VaultProcessor.h"
#include "../Source/vault/VaultEditor.h"
#include "../Source/vault/ui/VaultMainView.h"
#include "../Source/vault/ui/VaultValues.h"
#include "../Source/ParamKeys.h"
#include "../tools/vault/BankQC.h"
#include "../tools/vault/recipes/RecipeGate.h"
#if CV_HAS_BANK
 #include "VaultData.h"
#endif
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <cstdio>
#include <set>
#include <thread>
#include <cstdlib>
#include <new>

//==============================================================================
// allocation counter: operator new counts while the current thread has `countAllocs` set (sampler / engine no-alloc test)
namespace allocHook
{
thread_local bool countAllocs = false;
std::atomic<int> allocs { 0 };
}
void* operator new (std::size_t n)
{
    if (allocHook::countAllocs) ++allocHook::allocs;
    if (void* p = std::malloc (n == 0 ? 1 : n)) return p;
    throw std::bad_alloc();
}
void* operator new[] (std::size_t n)
{
    if (allocHook::countAllocs) ++allocHook::allocs;
    if (void* p = std::malloc (n == 0 ? 1 : n)) return p;
    throw std::bad_alloc();
}
void operator delete (void* p) noexcept { std::free (p); }
void operator delete[] (void* p) noexcept { std::free (p); }
void operator delete (void* p, std::size_t) noexcept { std::free (p); }
void operator delete[] (void* p, std::size_t) noexcept { std::free (p); }

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
    v.push_back (preset ("t.sub", "Test Sub Glide", vault::catBass, {
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

// the v2 bank has sampled / modelled presets: they stay silent until their samples / voices are in (the library loads in
// the background, the processor's timer makes the model voices); tests that need sound wait for that like a user would
bool waitUntilPlayable (VaultProcessor& p, int timeoutMs = 30000)
{
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    while (p.isLoading() && juce::Time::getMillisecondCounterHiRes() - t0 < timeoutMs)
        if (auto* mm = juce::MessageManager::getInstanceWithoutCreating(); mm != nullptr && mm->isThisTheMessageThread())
            mm->runDispatchLoopUntil (20);
        else
            juce::Thread::sleep (20);
    return ! p.isLoading();
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

// the shipped v2 factory bank (resources/vault/*.cvb, or the CHROME_VAULT_BANK file): counts per category, ids
// ("cv.<category slug>.<nnnn>"), unique clean names / labels, no "808", and a deterministic random sample through the v2
// bank gate (tools/vault/recipes/RecipeGate.h, the gate VaultBankGen -recipe all ran): C5 comfort, clicks, mono lows,
// tails, loudness within 1.5 dB of the category target with the shipped trim, the assembly ceilings, distinct sounds.
// Sampled presets are only rendered when the sample library is installed (CHROME_VAULT_LIBRARY or the standard folders);
// CV_BANK_GATE_ALL=1 runs every preset through the gate instead of the sample.
void shippedBankTests (const VaultProcessor& proc)
{
    const auto& bank = proc.bank();
    check (proc.bankSource() != VaultProcessor::BankSource::factoryFallback, "factory bank loaded: " + proc.bankDescription());
    check ((int) bank.size() >= 3000 && (int) bank.size() <= VaultProcessor::maxPresets, "the bank has " + juce::String ((int) bank.size())
                                                                                          + " presets (3,000 .. " + juce::String (VaultProcessor::maxPresets) + ")");
    int cats[vault::numCategories] {};
    for (auto& p : bank) ++cats[juce::jlimit (0, vault::numCategories - 1, p.category)];
    int smallest = 1 << 30;
    for (int c : cats) smallest = std::min (smallest, c);
    check (smallest >= 100, "every one of the 16 categories holds at least 100 presets (smallest " + juce::String (smallest) + ")");

    std::set<juce::String> ids, names;
    int badName = 0, badLabel = 0, has808 = 0, badId = 0, v2Parts = 0;
    for (auto& p : bank)
    {
        ids.insert (p.id);
        names.insert (p.name.toLowerCase());
        const bool nameOk = p.name.trim() == p.name && p.name.isNotEmpty() && p.name.length() <= 30 && vaultqc::isCleanName (p.name)
                            && p.name.containsOnly ("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 -'&");
        if (! nameOk && badName++ < 5) std::printf ("   bad name: '%s'\n", p.name.toRawUTF8());
        const juce::String prefix = juce::String ("cv.") + vault::categorySlug (p.category) + ".";
        const auto num = p.id.fromFirstOccurrenceOf (prefix, false, false);
        if ((! p.id.startsWith (prefix) || num.length() != 4 || ! num.containsOnly ("0123456789")) && badId++ < 5)
            std::printf ("   bad id: '%s' (category %s)\n", p.id.toRawUTF8(), vault::categoryNames()[p.category].toRawUTF8());
        has808 += p.name.contains ("808") ? 1 : 0;
        for (auto& part : p.parts)
        {
            v2Parts += part.kind != VaultPartKind::synth ? 1 : 0;
            has808 += part.label.contains ("808") ? 1 : 0;
            if ((part.label.isEmpty() || part.label.length() > 24 || ! vaultqc::isCleanName (part.label)) && badLabel++ < 5)
                std::printf ("   bad part label: %s '%s'\n", p.id.toRawUTF8(), part.label.toRawUTF8());
        }
    }
    check (ids.size() == bank.size(), "preset ids are unique (" + juce::String ((int) ids.size()) + ")");
    check (badId == 0, "preset ids are cv.<category slug>.<nnnn> (" + juce::String (badId) + " bad)");
    check (names.size() == bank.size(), "preset names are unique (" + juce::String ((int) names.size()) + " of " + juce::String ((int) bank.size()) + ")");
    check (badName == 0, "names: English, <= 30 characters (the list column fits them: -uitest), no brand words (" + juce::String (badName) + " bad)");
    check (badLabel == 0, "part labels set, short, clean (" + juce::String (badLabel) + " bad)");
    check (has808 == 0, "no '808' in any preset name or layer label (" + juce::String (has808) + ")");
    check (v2Parts > (int) bank.size() / 2, "a v2 bank: " + juce::String (v2Parts) + " sampled / modelled parts");

    // a deterministic random sample through the v2 bank gate (C5 = MIDI 60, 48 kHz - exactly VaultBankGen's gate)
    const auto lib = vault::SampleLibrary::getShared();
    const bool haveLib = lib != nullptr && lib->libraryFound();
    const bool everyPreset = juce::SystemStats::getEnvironmentVariable ("CV_BANK_GATE_ALL", {}) == "1";
    std::vector<int> pool;
    for (int i = 0; i < (int) bank.size(); ++i) if (haveLib || ! bank[(size_t) i].usesSamples()) pool.push_back (i);
    std::vector<int> sample;
    if (everyPreset) sample = pool;
    else
    {
        juce::Random rnd (20261014);
        std::set<int> chosen;
        while ((int) chosen.size() < std::min (160, (int) pool.size())) chosen.insert (pool[(size_t) rnd.nextInt ((int) pool.size())]);
        sample.assign (chosen.begin(), chosen.end());
    }
    std::vector<recipegate::Result> res (sample.size());
    std::atomic<int> next { 0 };
    std::vector<std::thread> threads;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (int t = 0; t < (int) std::max (1u, std::min (8u, std::thread::hardware_concurrency())); ++t)
        threads.emplace_back ([&]
        {
            for (int k = next++; k < (int) sample.size(); k = next++)
            {
                auto& r = res[(size_t) k];
                r.preset = bank[(size_t) sample[(size_t) k]];
                recipegate::analyse (r);
            }
        });
    for (auto& th : threads) th.join();
    int failed = 0;
    float worstTrim = 0, worstDev = 0;
    for (size_t k = 0; k < sample.size(); ++k)
    {
        const auto& p = bank[(size_t) sample[k]];
        auto& r = res[k];
        const float trimDiff = std::abs (r.gainDb - juce::Decibels::gainToDecibels (p.gain));
        worstTrim = std::max (worstTrim, trimDiff);
        worstDev = std::max (worstDev, r.quietBy + trimDiff);
        juce::String why = r.fail;
        if (why.isEmpty() && trimDiff > 0.5f) why = "trim differs from the measured loudness by " + juce::String (trimDiff, 2) + " dB";
        if (why.isEmpty() && r.quietBy > 1.5f) why = "loudness " + juce::String (r.quietBy, 2) + " dB under the category target";
        if (why.isEmpty() && r.share8k > recipegate::maxShare8k) why = "> 8 kHz share " + juce::String (100 * r.share8k, 2) + " %";
        if (why.isEmpty() && r.centroid > recipegate::centroidCeiling (p.category)) why = "centroid " + juce::String ((int) r.centroid) + " Hz";
        if (why.isEmpty() && r.chordOver > recipegate::maxChordOver + 0.5f) why = "listening phrase " + juce::String (r.chordOver, 1) + " dB over the safety limiter";
        if (why.isNotEmpty() && failed++ < 8) std::printf ("   gate: %s %s: %s\n", p.id.toRawUTF8(), p.name.toRawUTF8(), why.toRawUTF8());
    }
    check (failed == 0, juce::String ((int) sample.size()) + (everyPreset ? " presets" : " random presets") + (haveLib ? "" : " (no sample library: synth / model presets only)")
                        + " pass the v2 bank gate (" + juce::String (failed) + " failed; loudness within " + juce::String (worstDev, 2) + " dB of the category targets, trims within "
                        + juce::String (worstTrim, 2) + " dB; " + juce::String ((juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0, 1) + " s)");
    float closest = 1.0e9f;   // no two sampled presets sound alike (the gate's fingerprint)
    juce::String pa, pb;
    for (size_t a = 0; a < res.size(); ++a)
        for (size_t b = a + 1; b < res.size(); ++b)
            if (const float d = recipegate::distance (res[a].print, res[b].print); d < closest) { closest = d; pa = res[a].preset.name; pb = res[b].preset.name; }
    check (res.size() < 2 || closest >= 3.0f, "sampled presets are distinct (closest fingerprint distance " + juce::String (closest, 2) + ": " + pa + " / " + pb + ")");
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
        for (auto& p : all) vault::upgradeToV3 (p);   // v3 is written: v1 / v2 style presets come back converted
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
        const bool libFound = vault::SampleLibrary::getShared()->libraryFound();
        for (auto& p : bank)   // two of every category of the loaded bank (sampled ones only when the library is installed)
            if ((libFound || ! p.usesSamples()) && perCat[p.category]++ < 2) set.push_back (p);
        int silent = 0, nonFinite = 0, hot = 0, wideLows = 0, startClicks = 0;
        float worstPeak = 0, worstSide = 0;
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        for (auto& p : set)
        {
            const int note = p.category == vault::catBass ? 36 : 60;
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
                const float l = VaultEngine::measureLoudnessDb (VaultEngine::render (set[i], set[i].category == vault::catBass ? 36 : 60, 0.8f, 2.0, sr), sr);
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
        int target = std::min (5, (int) bank.size() - 1);
        if (! proc.libraryFound())   // without the sample library a sampled preset is (correctly) silent: play one that needs none
            for (int i = target; i < (int) bank.size(); ++i)
                if (! bank[(size_t) i].usesSamples()) { target = i; break; }
        proc.selectPreset (target);
        check (proc.currentPreset() == target, "selectPreset is instant");
        juce::AudioBuffer<float> b (2, 512);
        for (int k = 0; k < 300; ++k)   // samples / model voices are made off the audio thread: run until the sound is ready
        {
            juce::MidiBuffer none;
            proc.processBlock (b, none);
            if (! proc.isLoading()) break;
            waitUntilPlayable (proc, 20);
        }
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
    auto mk = [&] (const char* id, const char* name, int cat, std::vector<const char*> labels)
    {
        VaultPreset p; p.id = id; p.name = name; p.category = cat;
        for (auto* l : labels) { VaultPart part; part.label = l; p.parts.push_back (part); }
        bank.push_back (p);
    };
    using namespace vault;
    mk ("t.0", "Frozen Ice", catBellsMallets, { "Ice Bell", "Dark Pad" });
    mk ("t.1", "Velvet Keys", catPiano, { "Felt Piano" });
    mk ("t.2", "Glide Monster", catBass, { "Glide Bass", "Grit", "Sub" });
    mk ("t.3", "Holy Air", catPads, { "Air", "Choir Pad", "Drone", "Vinyl" });
    mk ("t.4", "Night Bell", catBellsMallets, { "Glock" });
    VaultBrowserModel m (bank);
    check (m.visible().size() == 5 && m.categoryCount (catBellsMallets) == 2 && m.categoryCount (-1) == 5, "browser: all sounds + category counts");
    m.filter.query = "dark bell"; m.refilter();
    check (m.visible() == std::vector<int> { 0 }, "browser: search 'dark bell' finds by layer label + category");
    m.filter.query = "glide bass"; m.refilter();
    check (m.visible() == std::vector<int> { 2 }, "browser: search matches layer labels / category");
    m.filter.query = "FELT"; m.refilter();
    check (m.visible() == std::vector<int> { 1 }, "browser: search is case-insensitive");
    m.filter.query = {}; m.filter.layers = 3; m.refilter();
    check (m.visible() == std::vector<int> { 2 }, "browser: layers chip");
    m.filter.layers = 0; m.filter.category = catBellsMallets; m.refilter();
    check (m.visible() == (std::vector<int> { 0, 4 }), "browser: category");
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
        { "Grand", "Felt Piano", "Upright", "Bright Grand", "Soft Grand" }, { "E-Piano", "Organ", "Clav", "Harpsichord", "Tine Keys" },
        { "Nylon", "Acoustic", "Banjo", "Clean Elec", "Harmonics" }, { "Upright Bass", "Sub", "Fingered", "Tuba", "Analog Bass" },
        { "Ensemble", "Pizz", "Solo Violin", "Cello", "Tremolo" }, { "Horn Swell", "Brass Stab", "Trumpet", "Trombone", "Tuba" },
        { "Flute", "Pan Flute", "Ocarina", "Clarinet", "Bassoon" }, { "Full Orch", "Timpani", "Orch Hit", "Horns", "Strings Hi" },
        { "Marimba", "Music Box", "Glock", "Celesta", "Vibes" }, { "Ooh", "Aah", "Choir", "Whisper", "Male Choir" },
        { "Glass Pluck", "Harp", "Kalimba", "Nylon Pick", "Mallet" }, { "Saw Lead", "Whistle", "Square", "Porta", "Sine Lead" },
        { "Dark Pad", "Warm Pad", "Air", "Choir Pad", "Drone" }, { "Supersaw", "FM Keys", "Retro Poly", "Chip", "Wobble" },
        { "Koto", "Sitar", "Kalimba", "Steel Drum", "Erhu" }, { "Riser", "Impact", "Texture", "Vinyl", "Wind" } };
    static const char* adj[] { "Midnight", "Frozen", "Chrome", "Hollow", "Velvet", "Neon", "Ghost", "Silver", "Crystal", "Black",
                               "Lunar", "Toxic", "Faded", "Cold", "Holy", "Static", "Glass", "Venom", "Silk", "Ashen" };
    juce::Random r (7);
    std::vector<VaultPreset> out;
    for (int c = 0; c < vault::numCategories; ++c)
        for (int k = 0; k < 5000 / vault::numCategories; ++k)
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
                p.parts.push_back (part);
            }
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

// adversarial editor QA: speed with 5,000 rows, more keys, macros (sound + waveform), click-free browsing through the whole
// plugin, WAV drag, odd host window sizes, first-open size, state restore with the editor open
double msSince (double t0) { return juce::Time::getMillisecondCounterHiRes() - t0; }

void uiQaTests (VaultProcessor& p)
{
    const auto& bank = p.bank();
    const int n = (int) bank.size();
    VaultEditor ed (p);
    auto& v = ed.view();
    v.setCategory (-1); v.setQuery ({}); v.setLayers (0); v.setFavoritesOnly (false); v.setRecentOnly (false);

    // ---- speed: every filter change re-filters 5,000 rows and repaints; only the visible rows are painted
    {
        double worst = 0, total = 0; int ops = 0;
        auto op = [&] (std::function<void()> f)
        {
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            f();
            const double ms = msSince (t0);
            worst = std::max (worst, ms); total += ms; ++ops;
        };
        for (int c = -1; c < vault::numCategories; ++c) op ([&] { v.setCategory (c); });
        v.setCategory (-1);
        for (int l = 0; l <= vault::maxParts; ++l) op ([&] { v.setLayers (l); });
        juce::String q;
        for (auto c : juce::String ("dark bell"))
        {
            q << juce::String::charToString (c);
            op ([&] { v.setQuery (q); });
        }
        v.setQuery ({}); v.setLayers (0);
        check (worst < 25.0, "ui: filter change on 5,000 rows (" + juce::String (ops) + " changes, avg " + juce::String (total / ops, 2)
                                 + " ms, worst " + juce::String (worst, 2) + " ms)");
        auto& lb = v.presetList().listBox();
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        int frames = 0;
        for (int row = 0; row < n; row += 97, ++frames)
        {
            lb.scrollToEnsureRowIsOnscreen (row);
            auto img = v.presetList().createComponentSnapshot (v.presetList().getLocalBounds(), true, 1.0f);
            juce::ignoreUnused (img);
        }
        const double perFrame = msSince (t0) / std::max (1, frames);
        int rowComps = 0;
        for (auto* c : lb.getViewport()->getViewedComponent()->getChildren()) rowComps += c->isVisible() ? 1 : 0;
        check (perFrame < 16.0 && rowComps < 40, "ui: list scroll + repaint " + juce::String (perFrame, 2) + " ms per frame, "
                                                    + juce::String (rowComps) + " row components for " + juce::String (n) + " rows");
        const auto t1 = juce::Time::getMillisecondCounterHiRes();
        for (int i = 0; i < 40; ++i) v.loadPreset ((i * 131) % n, false);
        check (msSince (t1) / 40.0 < 5.0, "ui: loading a sound from the list costs " + juce::String (msSince (t1) / 40.0, 3) + " ms on the UI thread");
    }

    // ---- keys (v3, owner report item 1): with the keyboard the editor plays notes in FL Studio's layout and steps the
    // sounds with Up / Down; Space, Return, Home / End, Page Down and keys that play no note stay with the DAW
    {
        v.setCategory (vault::catLeads);
        const auto rows = v.browser().visible();
        v.loadPreset (rows.front(), false);
        bool anyUsed = false;
        for (int code : { (int) juce::KeyPress::pageDownKey, (int) juce::KeyPress::endKey, (int) juce::KeyPress::homeKey, (int) juce::KeyPress::spaceKey,
                          (int) juce::KeyPress::returnKey, (int) 'a', (int) 'f', (int) 'k' })
            anyUsed |= ed.keyPressed (juce::KeyPress (code));
        const bool notes = ed.keyPressed (juce::KeyPress ('z', 0, 'z')) && ed.keyPressed (juce::KeyPress ('q', 0, 'q'));
        ed.computerKeys().allOff();
        const bool stepped = ed.keyPressed (juce::KeyPress (juce::KeyPress::downKey)) && rows.size() > 1 && p.currentPreset() == rows[1];
        ed.keyPressed (juce::KeyPress (juce::KeyPress::upKey));
        const bool anyUsedNow = anyUsed || ! notes || ! stepped;
        check (! anyUsedNow && p.currentPreset() == rows.front(),
               "ui: the editor's keys - Z / Q play notes, Down / Up step the sounds; Space, Return, Home / End, Page Down, A / F / K stay with the DAW");
        v.step (1, true);
        check (p.currentPreset() == rows[1], "ui: the next-sound arrow button still steps through the list");
        v.setCategory (-1);
    }

    // ---- macros: each knob moves the sound, and the waveform follows the knobs
    {
        int moved[vault::numMacros] {}, tested = 0;
        juce::Random r (11);
        const bool libOk = p.libraryFound();   // without the sample library only synth / model sounds can be heard
        for (int k = 0; k < 24; ++k, ++tested)
        {
            int pick = r.nextInt (n);
            for (int guard = 0; ! libOk && bank[(size_t) pick].usesSamples() && guard < 10000; ++guard) pick = r.nextInt (n);
            const auto& pr = bank[(size_t) pick];
            const int note = vault::homeNote;
            float m[vault::numMacros];
            for (int i = 0; i < vault::numMacros; ++i) m[i] = pr.macroDefaults[i];
            const auto base = VaultEngine::render (pr, note, 0.8f, 1.5, 22050.0, m);
            for (int i = 0; i < vault::numMacros; ++i)
            {
                float mm[vault::numMacros];
                std::copy (std::begin (m), std::end (m), std::begin (mm));
                float diffMax = 0;
                for (float target : { 0.0f, 1.0f })
                {
                    mm[i] = target;
                    const auto b = VaultEngine::render (pr, note, 0.8f, 1.5, 22050.0, mm);
                    double d = 0, e = 0;
                    for (int c = 0; c < 2; ++c)
                        for (int s = 0; s < b.getNumSamples(); ++s)
                        {
                            const double x = base.getSample (c, s), y = b.getSample (c, s);
                            d += (x - y) * (x - y); e += x * x;
                        }
                    diffMax = std::max (diffMax, (float) std::sqrt (d / std::max (1e-12, e)));
                }
                if (diffMax > 0.05f) ++moved[i];
            }
        }
        juce::String s;
        bool all = true;
        for (int i = 0; i < vault::numMacros; ++i) { s << vault::macroNames()[i] << " " << moved[i] << "/" << tested << " "; all &= moved[i] >= tested * 9 / 10; }
        check (all, "ui: every macro audibly changes the sound (" + s.trim() + ")");

        const int idx = v.shownPreset();
        auto& knob = v.mainPage().macroKnob (1);
        knob.userSet (1.0f);
        check (std::abs (p.getMacro (1) - 1.0f) < 1e-4f && std::abs (knob.getValue() - 1.0f) < 1e-4f, "ui: sound macro knob 2 drives the macro parameter");
        p.setMacro (2, 0.25f);
        check (std::abs (v.mainPage().macroKnob (2).getValue() - 0.25f) < 1e-4f, "ui: a host change of a macro moves its knob");
        knob.userSet (bank[(size_t) idx].macroDefaults[1]);
    }

    // ---- browsing through the whole plugin while a chord is held: instant, click-free, never silent, never clipping
    {
        constexpr double sr = 48000.0;
        constexpr int block = 256;
        p.setPlayConfigDetails (0, 2, sr, block);
        p.prepareToPlay (sr, block);
        v.setCategory (vault::catPads);
        {   // the pads browsed below have their samples in RAM (a first visit of a sampled sound plays once it is loaded)
            const auto vis = v.browser().visible();
            for (int k = 0; k < std::min (26, (int) vis.size()); ++k) { p.selectPreset (vis[(size_t) k]); waitUntilPlayable (p); }
        }
        v.loadPreset (v.browser().visible().front(), false);
        auto& ks = p.getKeyboardState();
        for (int nn : { 60, 63, 67 }) ks.noteOn (1, nn, 0.8f);
        const int switchEvery = (int) (0.25 * sr / block), numSwitches = 24, blocks = switchEvery * (numSwitches + 1);
        juce::AudioBuffer<float> all (2, blocks * block), buf (2, block);
        std::vector<int> switchAt, switchTo;
        int lateBlocks = 0;
        for (int k = 0; k < blocks; ++k)
        {
            if (k > 0 && k % switchEvery == 0)
            {
                const int before = p.currentPreset();
                v.step (1, false);                        // the down key
                if (p.currentPreset() == before) ++lateBlocks;
                switchAt.push_back (k * block);
                switchTo.push_back (p.currentPreset());
            }
            juce::MidiBuffer m;
            p.processBlock (buf, m);
            for (int c = 0; c < 2; ++c) all.copyFrom (c, k * block, buf, c, 0, block);
        }
        for (int nn : { 60, 63, 67 }) ks.noteOff (1, nn, 0.0f);
        if (juce::SystemStats::getEnvironmentVariable ("CV_QA_DEBUG", {}).isNotEmpty())
            writeWav (all, sr, juce::File::getCurrentWorkingDirectory().getChildFile ("browse.wav"));
        // a click = a sample-to-sample jump far above what the sounds on both sides of the switch do on their own
        float worstRatio = 0, worstJump = 0, minRms = 1, peak = 0;
        for (size_t si = 0; si < switchAt.size(); ++si)
        {
            const int sw = switchAt[si];
            const int len = switchEvery * block;
            const float around = stats (all, sw - 64, sw + (int) (0.025 * sr)).maxJump;
            // the new sound's own attack: the same chord struck fresh (the switch re-strikes it after the 10 ms fade-out)
            juce::MidiBuffer chord;
            for (int nn : { 60, 63, 67 }) chord.addEvent (juce::MidiMessage::noteOn (1, nn, 0.8f), 0);
            const auto fresh = VaultEngine::renderMidi (bank[(size_t) switchTo[si]], chord, (int) (0.03 * sr), sr);
            const float own = std::max ({ stats (all, sw - len / 2, sw - 64).maxJump, stats (all, sw + (int) (0.04 * sr), sw + len - 64).maxJump,
                                          stats (fresh).maxJump });
            worstRatio = std::max (worstRatio, around / std::max (1e-4f, own));
            if (juce::SystemStats::getEnvironmentVariable ("CV_QA_DEBUG", {}).isNotEmpty())
            {
                int at = -1; float mj = 0;
                for (int c = 0; c < 2; ++c) for (int i = sw - 63; i < sw + (int) (0.025 * sr); ++i) { const float j = std::abs (all.getSample (c, i) - all.getSample (c, i - 1)); if (j > mj) { mj = j; at = i - sw; } }
                juce::String fxs;
                for (auto& [id, val] : bank[(size_t) switchTo[si]].parts.front().values)
                    if (id == ID::chorus || id == ID::revMix || id == ID::delayMix || id == ID::flanger || id == ID::phaser || id == ID::revType || id == ID::body || id == ID::reverse)
                        fxs << id << "=" << juce::String (val, 2) << " ";
                std::printf ("   switch @%d: jump %.4f at %+d samples, own %.4f -> %d %s | %s\n", sw, around, at, own, switchTo[si],
                             bank[(size_t) switchTo[si]].name.toRawUTF8(), fxs.toRawUTF8());
            }
            worstJump = std::max (worstJump, around);
            if (p.libraryFound() || ! bank[(size_t) switchTo[si]].usesSamples())   // a sampled sound without the library is silent
                minRms = std::min (minRms, stats (all, sw + (int) (0.05 * sr), sw + len - 64).rms);
        }
        peak = stats (all).peak;
        check (lateBlocks == 0, "ui: a picked sound is active in the very next audio block");
        check (worstRatio < 1.6f, "ui: browsing " + juce::String (numSwitches) + " pads with a held chord has no clicks (worst jump "
                                       + juce::String (worstJump, 4) + ", " + juce::String (worstRatio, 2) + "x the sounds' own)");
        check (minRms > 1e-3f && peak < 1.0f && stats (all).finite, "ui: every sound plays the held chord, no clipping (min rms "
                                                                       + juce::String (minRms, 4) + ", peak " + juce::String (peak, 3) + ")");
        p.releaseResources();
        v.setCategory (-1);
    }

    // ---- DRAG WAV: rendered quickly, every category (basses too) at the home note C5 = MIDI 60, file written
    {
        int bass = 0;
        for (int i = 0; i < n; ++i) if (bank[(size_t) i].category == vault::catBass) { bass = i; break; }
        v.loadPreset (bass, false);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        const auto f = v.renderWavForDrag();
        const double ms = msSince (t0);
        check (f.existsAsFile() && f.getFileName().endsWith (" C5.wav") && f.getSize() > 100000 && ms < 400.0,
               "ui: DRAG WAV of a bass -> " + f.getFileName() + " (" + juce::String (f.getSize() / 1024) + " KB, " + juce::String (ms, 0) + " ms)");
    }

    // ---- window: a host that ignores the aspect ratio gets a centred, fully visible view; first-open size per screen
    {
        ed.setSize (1600, 760);
        auto b = v.getBoundsInParent();
        check (ed.getLocalBounds().contains (b) && std::abs (b.getCentreX() - 800) <= 2 && b.getHeight() == 760,
               "ui: odd host size 1600x760 -> view " + b.toString());
        ed.setSize (1180, 1000);
        b = v.getBoundsInParent();
        check (ed.getLocalBounds().contains (b) && b.getWidth() == 1180 && std::abs (b.getCentreY() - 500) <= 2,
               "ui: odd host size 1180x1000 -> view " + b.toString());
        ed.setUiScale (1.0f);
        check (VaultEditor::firstOpenScale ({ 0, 0, 1366, 728 }) == 0.8f && VaultEditor::firstOpenScale ({ 0, 0, 1920, 1040 }) == 1.0f
                   && VaultEditor::firstOpenScale ({ 0, 0, 1280, 680 }) == 0.8f && VaultEditor::firstOpenScale ({ 0, 0, 3840, 2100 }) == 1.0f,
               "ui: first-open size fits 85 % of the screen (1366x768 -> 80 %, 1920x1080 -> 100 %)");
    }

    // ---- favorites view: un-favoriting a row keeps the list where it is
    {
        for (int i = 0; i < 60; ++i) if (! v.browser().isFavorite (i * 7)) v.toggleFavorite (i * 7);
        v.setFavoritesOnly (true);
        auto& lb = v.presetList().listBox();
        lb.getViewport()->setViewPosition (0, 20 * vui::PresetList::rowH);
        const int y0 = lb.getViewport()->getViewPositionY();
        v.toggleFavorite (v.browser().visible()[25]);
        check (y0 > 0 && lb.getViewport()->getViewPositionY() == y0, "ui: un-favoriting in the favorites view keeps the scroll position");
        for (int i = 0; i < 60; ++i) if (v.browser().isFavorite (i * 7)) v.toggleFavorite (i * 7);
        v.setFavoritesOnly (false);
    }

    // ---- host state restore while the editor is open: sound, knobs and the list follow
    {
        VaultProcessor other;
        const int target = n - 3;
        other.selectPreset (target);
        other.setMacro (0, 0.15f); other.setMacro (3, 0.9f); other.setVolumeDb (-6.0f);
        juce::MemoryBlock st;
        other.getStateInformation (st);
        p.setStateInformation (st.getData(), (int) st.getSize());
        v.syncFromProcessor();
        check (p.currentPreset() == target && v.shownPreset() == target && v.presetList().listBox().getModel()->getNumRows() > 0,
               "ui: state restore with the editor open shows the restored sound");
        const float volDb = p.apvts.getParameter (VaultProcessor::idVolume)->convertFrom0to1 (v.mainPage().volumeKnob().getValue());
        check (std::abs (v.mainPage().macroKnob (0).getValue() - 0.15) < 1e-3 && std::abs (v.mainPage().macroKnob (3).getValue() - 0.9) < 1e-3
                   && std::abs (volDb + 6.0f) < 0.05f,
               "ui: state restore moves the knobs and the volume slider");
        juce::MemoryBlock junk ("not a state", 11);
        p.setStateInformation (junk.getData(), (int) junk.getSize());
        p.setStateInformation (nullptr, 0);
        check (p.currentPreset() == target, "ui: a broken state is ignored");
    }
}

// window size per screen: never too big on screen (first open <= 100 % and inside 85 % of the screen, a remembered size
// shrinks to fit a smaller screen without forgetting the choice), scale menu choices that do not fit are greyed out
void windowSizeTests (VaultProcessor& p)
{
    auto settings = VaultSettings::get();
    const float before = settings->scale();
    struct Screen { const char* name; juce::Rectangle<int> user; };
    const Screen laptop { "1366x768", { 0, 0, 1366, 728 } }, fullHd { "1920x1080", { 0, 0, 1920, 1040 } },
                 uhd { "3840x2160", { 0, 0, 3840, 2120 } }, tiny { "1024x600", { 0, 0, 1024, 560 } };
    auto open = [&] (const Screen& sc, float saved)
    {
        VaultEditor::simulatedDisplay() = sc.user;
        settings->setScale (saved);
        VaultEditor ed (p);
        return std::make_pair (ed.getBounds(), ed.getUiScale());
    };
    auto str = [] (juce::Rectangle<int> b) { return juce::String (b.getWidth()) + "x" + juce::String (b.getHeight()); };
    auto inside85 = [] (juce::Rectangle<int> b, juce::Rectangle<int> user)
    {
        return b.getWidth() <= user.getWidth() * 0.85 && b.getHeight() <= user.getHeight() * 0.85;
    };

    // first open (nothing saved)
    auto [b1, s1] = open (laptop, 0.0f);
    check (std::abs (s1 - 0.8f) < 0.001f && b1.getWidth() == 944 && b1.getHeight() == 608 && inside85 (b1, laptop.user),
           "ui: first open on 1366x768 (user area 1366x728) -> 80 % " + str (b1) + ", inside 85 % of the screen");
    auto [b2, s2] = open (fullHd, 0.0f);
    check (std::abs (s2 - 1.0f) < 0.001f && b2.getWidth() == 1180 && b2.getHeight() == 760 && inside85 (b2, fullHd.user),
           "ui: first open on 1920x1080 -> 100 % " + str (b2));
    auto [b3, s3] = open (uhd, 0.0f);
    check (std::abs (s3 - 1.0f) < 0.001f && b3.getWidth() == 1180, "ui: first open on 3840x2160 -> 100 % " + str (b3) + " (never bigger)");
    auto [b4, s4] = open (tiny, 0.0f);
    check (std::abs (s4 - 0.8f) < 0.001f, "ui: first open on a 1024x600 netbook -> the 80 % minimum " + str (b4));
    check (settings->scale() <= 0.0f, "ui: the first-open size is not stored as a user choice");

    // restore: the user's choice, clamped to the screen it opens on
    auto [r1, rs1] = open (laptop, 2.0f);
    check (laptop.user.contains (r1.withPosition (0, 0)) && std::abs (rs1 - 0.9f) < 0.001f,
           "ui: 200 % chosen on 4K, reopened on 1366x768 -> " + str (r1) + " (90 %), inside the screen");
    check (std::abs (settings->scale() - 2.0f) < 0.001f, "ui: ... and the 200 % choice is kept for the big screen");
    auto [r2, rs2] = open (fullHd, 1.5f);
    check (fullHd.user.contains (r2.withPosition (0, 0)) && std::abs (rs2 - 1.3f) < 0.001f, "ui: 150 % reopened on 1920x1080 -> 130 % " + str (r2));
    auto [r3, rs3] = open (fullHd, 1.15f);
    check (std::abs (rs3 - 1.15f) < 0.001f && r3.getWidth() == 1357, "ui: 115 % fits 1920x1080 and is restored as chosen " + str (r3));
    auto [r4, rs4] = open (uhd, 2.0f);
    check (std::abs (rs4 - 2.0f) < 0.001f && r4.getWidth() == 2360, "ui: 200 % restored on 3840x2160 " + str (r4));
    auto [r5, rs5] = open (laptop, 0.9f);
    check (std::abs (rs5 - 0.9f) < 0.001f, "ui: 90 % restored on 1366x768 " + str (r5));
    bool allFit = true;
    for (const auto* sc : { &laptop, &fullHd, &uhd })
        for (float sv : VaultEditor::scalePresets())
        {
            auto [rb, rs] = open (*sc, sv);
            allFit &= sc->user.contains (rb.withPosition (0, 0));
        }
    check (allFit, "ui: every remembered scale on every screen opens inside the screen");

    // the scale menu greys out what does not fit
    check (VaultEditor::fits (0.9f, laptop.user) && ! VaultEditor::fits (1.0f, laptop.user) && VaultEditor::fits (1.3f, fullHd.user)
               && ! VaultEditor::fits (1.5f, fullHd.user) && VaultEditor::fits (2.0f, uhd.user),
           "ui: scale menu offers 80-90 % on 1366x768, up to 130 % on 1920x1080, all on 4K");
    settings->setScale (before);
    VaultEditor::simulatedDisplay() = fullHd.user;
}

// readability: every font painted at 80 % renders >= 11 px and at 100 % >= 13 px (tags, counts, labels included)
void readabilityTests (VaultProcessor& p)
{
    VaultEditor::simulatedDisplay() = { 0, 0, 3840, 2120 };
    VaultEditor ed (p);
    auto& v = ed.view();
    const auto& bank = p.bank();
    int pick = 0;
    for (int i = 0; i < (int) bank.size(); ++i) if (bank[(size_t) i].parts.size() >= 3) { pick = i; break; }
    v.loadPreset (pick, false);
    v.setLayers (3);   // "Clear filters" is painted too
    for (float sc : { 0.8f, 1.0f, 1.5f, 2.0f })
    {
        ed.setUiScale (sc);
        vui::smallestFontPx() = 1000.0f;
        juce::ignoreUnused (ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f));
        const float rendered = vui::smallestFontPx() * sc;
        const float need = sc < 0.95f ? 11.0f : 13.0f;
        check (rendered >= need - 0.01f, "ui: smallest text at " + juce::String (juce::roundToInt (sc * 100)) + " % = "
                                             + juce::String (rendered, 1) + " px (>= " + juce::String (need, 0) + ")");
    }
    // no "..." hiding information in the list: every name of the bank fits its column; the categories fit theirs
    {
        ed.setUiScale (1.0f);
        const auto cols = vui::PresetList::columns ((float) v.presetList().listBox().getVisibleRowWidth(), (float) vui::PresetList::rowH);
        int namesCut = 0;
        juce::String worst;
        for (int i = 0; i < (int) bank.size(); ++i)
            if (vui::textWidth (vui::PresetList::nameFont(), bank[(size_t) i].name) > cols.name.getWidth()) { ++namesCut; worst = bank[(size_t) i].name; }
        check (namesCut == 0, "ui: every name of the bank fits its list column (" + juce::String (cols.name.getWidth(), 0) + " px, "
                                  + juce::String (namesCut) + " cut" + (worst.isEmpty() ? juce::String() : ": " + worst) + ")");
        auto& cl = v.mainPage().categoryList();
        check (cl.widestRow() <= (float) cl.getWidth() && cl.rowH >= 20, "ui: every category name + count fits its column ("
                   + juce::String (cl.widestRow(), 0) + " of " + juce::String (cl.getWidth()) + " px, rows " + juce::String (cl.rowH) + " px)");
    }
    // every page at 80 / 100 %: text >= 11 / 13 px, knob labels fit their knob, no two controls overlap, nothing outside the page
    // (a 3-layer sound, then a 4-layer one with the most modelled / synth layers: the narrowest layer cards)
    int four = -1;
    {
        int bestScore = -1;
        for (int i = 0; i < (int) bank.size(); ++i)
        {
            if (bank[(size_t) i].parts.size() != 4) continue;
            int score = 0;
            for (auto& part : bank[(size_t) i].parts) score += part.kind == VaultPartKind::model ? 2 : part.kind == VaultPartKind::synth ? 1 : 0;
            if (score > bestScore) { bestScore = score; four = i; }
        }
    }
    for (int round = 0; round < (four >= 0 ? 2 : 1); ++round)
    for (int pg = 0; pg < VaultMainView::numPages; ++pg)
    {
        if (pg == 0) { v.loadPreset (round == 0 ? pick : four, false); v.refreshAll(); }
        v.showPage (pg);
        for (float sc : { 0.8f, 1.0f })
        {
            ed.setUiScale (sc);
            vui::smallestFontPx() = 1000.0f;
            juce::ignoreUnused (ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f));
            const float rendered = vui::smallestFontPx() * sc;
            check (rendered >= (sc < 0.95f ? 11.0f : 13.0f) - 0.01f, "ui: page " + juce::String (pg) + " smallest text at "
                       + juce::String (juce::roundToInt (sc * 100)) + " % = " + juce::String (rendered, 1) + " px");
        }
        int overlaps = 0, outside = 0, labelsCut = 0;
        juce::String what;
        std::function<void (juce::Component&)> walk = [&] (juce::Component& c)
        {
            std::vector<juce::Component*> kids;
            for (auto* ch : c.getChildren()) if (ch->isVisible() && ! ch->getBounds().isEmpty()) kids.push_back (ch);
            for (size_t i = 0; i < kids.size(); ++i)
            {
                if (! c.getLocalBounds().contains (kids[i]->getBounds()) && dynamic_cast<juce::Viewport*> (&c) == nullptr
                    && dynamic_cast<juce::ListBox*> (&c) == nullptr && c.getParentComponent() != nullptr
                    && dynamic_cast<juce::Viewport*> (c.getParentComponent()) == nullptr)
                { ++outside; what = typeid (*kids[i]).name() + juce::String (" ") + kids[i]->getBounds().toString() + " in " + c.getLocalBounds().toString(); }
                for (size_t j = i + 1; j < kids.size(); ++j)
                    if (kids[i]->getBounds().intersects (kids[j]->getBounds()) && dynamic_cast<juce::ScrollBar*> (kids[j]) == nullptr
                        && dynamic_cast<juce::ScrollBar*> (kids[i]) == nullptr && dynamic_cast<juce::Viewport*> (&c) == nullptr)
                    { ++overlaps; what = juce::String (typeid (*kids[i]).name()) + " " + kids[i]->getBounds().toString() + " x " + typeid (*kids[j]).name() + " " + kids[j]->getBounds().toString(); }
                if (auto* k = dynamic_cast<vui::Knob*> (kids[i]))
                {
                    const auto f = vui::font (14.0f, true, 0.0f);
                    const float need = std::min (vui::textWidth (f, k->label), k->shortLabel.isEmpty() ? 1.0e9f : vui::textWidth (f, k->shortLabel)) + (k->hasLed ? 11.0f : 0.0f);
                    if (need > (float) k->getWidth()) { ++labelsCut; what = "label " + k->label + " in " + juce::String (k->getWidth()) + " px"; }
                }
            }
            for (auto* ch : kids) walk (*ch);
        };
        walk (v.pageComponent (pg));
        walk (v);
        check (overlaps == 0 && outside == 0 && labelsCut == 0, "ui: page " + juce::String (pg) + " (" + bank[(size_t) (round == 0 ? pick : four)].name + ") layout: " + juce::String (overlaps) + " overlaps, "
                   + juce::String (outside) + " outside, " + juce::String (labelsCut) + " knob labels too wide" + (what.isEmpty() ? juce::String() : " (" + what + ")"));
    }
    v.showPage (VaultMainView::pageMain);
    // text colours: nothing darker than the "dim" tertiary text on the dark panels, primary text #E3E8F0
    const auto lum = [] (juce::Colour c) { return 0.2126f * c.getFloatRed() + 0.7152f * c.getFloatGreen() + 0.0722f * c.getFloatBlue(); };
    check (vui::col::dim == juce::Colour (0xff8593ab) && vui::col::text == juce::Colour (0xffe3e8f0)
               && lum (vui::col::muted) >= lum (vui::col::dim) && lum (vui::col::ice) >= lum (vui::col::dim),
           "ui: text colours (dim #8593AB is the darkest, primary #E3E8F0)");
    v.setLayers (0);
    VaultSettings::get()->setScale (1.0f);
}

// keyboard focus: only the search box ever takes the keyboard; clicks elsewhere, Return and Escape give it back to the DAW;
// the on-screen keyboard is labelled in FL Studio naming (MIDI 60 = C5)
void focusTests (VaultProcessor& p)
{
    VaultEditor ed (p);
    auto& v = ed.view();
    v.loadPreset (0, false);
    juce::MessageManager::getInstance()->runDispatchLoopUntil (20);   // the list creates its row components
    juce::ignoreUnused (ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f));
    int grabbers = 0, total = 0;
    std::function<void (juce::Component&)> walk = [&] (juce::Component& c)
    {
        ++total;
        if (&c != &v.searchBox() && (c.getWantsKeyboardFocus() || c.getMouseClickGrabsKeyboardFocus()))
        {
            if (grabbers++ < 6) std::printf ("   takes focus on click: %s (%s)\n", c.getName().toRawUTF8(), typeid (c).name());
        }
        if (&c == &v.searchBox()) return;   // the text editor's own inner parts
        for (auto* ch : c.getChildren()) walk (*ch);
    };
    walk (v);
    for (auto* c : ed.getChildren())   // the resize corner too
        if (c != &v && (c->getWantsKeyboardFocus() || c->getMouseClickGrabsKeyboardFocus())) ++grabbers;
    check (grabbers == 0 && ed.getWantsKeyboardFocus() && ! v.searchBox().getWantsKeyboardFocus(),
           "focus: only the editor itself takes the keyboard (the search box only once clicked; " + juce::String (total) + " components checked)");
    check (v.keys().getOctaveForMiddleC() == 5 && vault::noteName (60) == "C5" && vault::noteName (vault::homeNote) == "C5" && vault::noteName (36) == "C3",
           "keyboard: MIDI 60 is labelled C5 (FL Studio)");

    // the search box hands the keyboard back on Return / Escape / a click elsewhere (counted by the view; a real
    // focus round trip needs a window manager, which xvfb does not have)
    auto& search = dynamic_cast<juce::TextEditor&> (v.searchBox());
    const int n0 = v.focusReturns();
    search.keyPressed (juce::KeyPress (juce::KeyPress::returnKey));
    juce::MessageManager::getInstance()->runDispatchLoopUntil (30);   // the text editor posts its key callbacks
    const bool afterReturn = v.focusReturns() == n0 + 1;
    search.setText ("piano", true);
    search.keyPressed (juce::KeyPress (juce::KeyPress::escapeKey));
    juce::MessageManager::getInstance()->runDispatchLoopUntil (30);
    const bool afterEscape = v.focusReturns() == n0 + 2 && search.isEmpty();
    check (afterReturn && afterEscape && ! search.getWantsKeyboardFocus(), "focus: Return / Escape in the search box give the keyboard back to the editor");
}

//==============================================================================
// v2 editor: a demo bank of v2 presets (sampled layers on the library instruments, the "sine" model, synth layers) - only
// for UI checks and snapshots (the levels / FX are not designed sounds). "keys.celesta" is deliberately not in the library.
std::vector<VaultPreset> uiDemoV2Bank()
{
    const auto fb = VaultProcessor::buildFallbackBank();
    std::vector<int> byCat[vault::numCategories];
    for (int i = 0; i < (int) fb.size(); ++i) byCat[fb[(size_t) i].category].push_back (i);
    struct L { char kind; const char* src; int cat; const char* label; float level; int oct; };
    auto S = [] (const char* id, float level = 1.0f, int oct = 0, const char* label = "") { return L { 's', id, 0, label, level, oct }; };
    auto Y = [] (int cat, const char* label, float level = 1.0f, int oct = 0) { return L { 'y', "", cat, label, level, oct }; };
    auto M = [] (const char* label, float level = 0.3f, int oct = 0) { return L { 'm', "sine", 0, label, level, oct }; };
    using namespace vault;
    const std::vector<std::tuple<int, const char*, std::vector<L>>> recipes {
        { catPiano, "Concert Grand", { S ("piano.grand.concert") } },
        { catPiano, "Studio Grand", { S ("piano.grand.studio") } },
        { catPiano, "Grand & Strings", { S ("piano.grand.concert"), S ("strings.violins.ensemble", 0.45f, 0, "Soft Violins") } },
        { catPiano, "Felt Grand Pad", { S ("piano.grand.chamber", 1.0f, 0, "Felt"), Y (catPads, "Warm Pad", 0.4f) } },
        { catPiano, "Upright Piano", { S ("piano.upright") } },
        { catPiano, "Parlor Upright Air", { S ("piano.upright.parlor"), Y (catPads, "Air", 0.3f, 1), M ("Sub Sine", 0.25f, -1) } },
        { catKeysOrgan, "FM Electric Piano", { S ("keys.fm.epiano") } },
        { catKeysOrgan, "Pipe Organ Hall", { S ("organ.pipe") } },
        { catKeysOrgan, "Chamber Organ", { S ("organ.chamber") } },
        { catKeysOrgan, "Baroque Harpsichord", { S ("keys.harpsichord.baroque") } },
        { catKeysOrgan, "Tine Keys Dream", { Y (catKeysOrgan, "Tine Keys"), Y (catPads, "Air", 0.35f) } },
        { catKeysOrgan, "FM Clav Funk", { S ("keys.fm.clav") } },
        { catGuitars, "Nylon Evening", { Y (catGuitars, "Nylon") } },
        { catGuitars, "Folk Harp & Nylon", { S ("pluck.harp.folk"), Y (catGuitars, "Nylon", 0.6f) } },
        { catGuitars, "Stick Dulcimer", { S ("pluck.dulcimer") } },
        { catGuitars, "Clean Electric", { Y (catGuitars, "Clean Electric") } },
        { catBass, "Contrabass Pizz", { S ("strings.contrabass") } },
        { catBass, "Tuba Bass", { S ("brass.tuba") } },
        { catBass, "Bassoon Bass", { S ("winds.bassoon") } },
        { catBass, "Analog Mono Bass", { Y (catBass, "Analog Bass") } },
        { catBass, "Sub & Contrabass", { S ("strings.contrabass"), M ("Sub Sine", 0.5f, -1) } },
        { catBass, "Driven Bass", { Y (catBass, "Driven Bass") } },
        { catStrings, "Violin Section", { S ("strings.violins.ensemble") } },
        { catStrings, "Viola Section", { S ("strings.violas.ensemble") } },
        { catStrings, "Cello Section", { S ("strings.cellos.ensemble") } },
        { catStrings, "Solo Violin", { S ("strings.violin.solo") } },
        { catStrings, "Full String Section", { S ("strings.violins.ensemble"), S ("strings.violas.ensemble", 0.7f), S ("strings.cellos.ensemble", 0.7f, -1) } },
        { catStrings, "Cellos & Dark Pad", { S ("strings.cellos.ensemble"), Y (catPads, "Dark Pad", 0.4f) } },
        { catBrass, "Trumpet", { S ("brass.trumpet") } },
        { catBrass, "French Horn", { S ("brass.horn") } },
        { catBrass, "Tenor Trombone", { S ("brass.trombone") } },
        { catBrass, "Brass Section", { S ("brass.trumpet"), S ("brass.horn", 0.8f), S ("brass.trombone", 0.7f, -1) } },
        { catBrass, "Vintage Trombone", { S ("brass.trombone.vintage") } },
        { catBrass, "Horn Swell Pad", { S ("brass.horn"), Y (catPads, "Warm Pad", 0.4f) } },
        { catFlutesWoodwinds, "Concert Flute", { S ("winds.flute.concert") } },
        { catFlutesWoodwinds, "Piccolo", { S ("winds.piccolo") } },
        { catFlutesWoodwinds, "Clarinet", { S ("winds.clarinet") } },
        { catFlutesWoodwinds, "Oboe", { S ("winds.oboe") } },
        { catFlutesWoodwinds, "Alto Recorder", { S ("winds.recorder.alto") } },
        { catFlutesWoodwinds, "Tenor Sax", { S ("winds.sax.tenor") } },
        { catFlutesWoodwinds, "Woodwind Choir", { S ("winds.flute.concert"), S ("winds.clarinet", 0.8f), S ("winds.oboe", 0.6f), S ("winds.bassoon", 0.6f, -1) } },
        { catOrchestral, "Full Orchestra", { S ("strings.violins.ensemble"), S ("strings.cellos.ensemble", 0.8f, -1), S ("brass.horn", 0.6f), S ("perc.timpani", 0.5f, -1) } },
        { catOrchestral, "Timpani Roll", { S ("perc.timpani") } },
        { catOrchestral, "Strings & Brass", { S ("strings.violins.ensemble"), S ("brass.horn", 0.6f) } },
        { catOrchestral, "Orchestral Hit", { Y (catOrchestral, "Orch Hit"), S ("perc.timpani", 0.6f, -1) } },
        { catBellsMallets, "Marimba", { S ("mallets.marimba") } },
        { catBellsMallets, "Vibraphone", { S ("mallets.vibraphone") } },
        { catBellsMallets, "Glockenspiel", { S ("mallets.glockenspiel") } },
        { catBellsMallets, "Mallet Stack", { S ("mallets.marimba"), S ("mallets.vibraphone", 0.6f), M ("Sine Shimmer", 0.25f, 1) } },
        { catBellsMallets, "Tubular Bells", { S ("bells.tubular") } },
        { catBellsMallets, "Celesta", { S ("keys.celesta") } },
        { catBellsMallets, "Ice Bell Stack", { Y (catBellsMallets, "Ice Bell"), S ("mallets.glockenspiel", 0.5f, 1) } },
        { catVocalsChoir, "Classic Choir", { S ("voice.choir.classic") } },
        { catVocalsChoir, "Female Solo Ha", { S ("voice.female.solo") } },
        { catVocalsChoir, "Male Low Voice", { S ("voice.male.low") } },
        { catVocalsChoir, "Vowel Chops", { S ("voice.female.chops") } },
        { catVocalsChoir, "Vocal Textures", { S ("voice.fx.textures") } },
        { catVocalsChoir, "Choir & Air Pad", { S ("voice.choir.classic"), Y (catPads, "Air", 0.4f) } },
        { catPlucks, "Concert Harp", { S ("pluck.harp.concert") } },
        { catPlucks, "Psaltery", { S ("pluck.psaltery") } },
        { catPlucks, "Harp & Kalimba", { S ("pluck.harp.concert"), S ("world.kalimba", 0.6f) } },
        { catPlucks, "Glass Pluck", { Y (catPlucks, "Glass Pluck") } },
        { catLeads, "Saw Lead", { Y (catLeads, "Saw Lead") } },
        { catLeads, "Ocarina Lead", { S ("winds.ocarina"), Y (catLeads, "Sine Lead", 0.4f) } },
        { catLeads, "Harmonica Lead", { S ("winds.harmonica.chromatic") } },
        { catLeads, "Whistle Lead", { Y (catLeads, "Whistle") } },
        { catPads, "Warm Analog Pad", { Y (catPads, "Warm Pad") } },
        { catPads, "Glass Harp Pad", { S ("fx.glass"), Y (catPads, "Air", 0.5f) } },
        { catPads, "Choir Air Pad", { Y (catPads, "Choir Pad"), S ("voice.choir.classic", 0.4f) } },
        { catPads, "Deep Drone", { Y (catPads, "Drone"), M ("Sub Sine", 0.3f, -1) } },
        { catSynth, "Supersaw Stack", { Y (catSynth, "Supersaw") } },
        { catSynth, "FM Keys Poly", { Y (catSynth, "FM Keys"), S ("keys.fm.epiano", 0.5f) } },
        { catSynth, "Retro Poly", { Y (catSynth, "Retro Poly") } },
        { catWorld, "Kalimba", { S ("world.kalimba") } },
        { catWorld, "Mbira", { S ("world.mbira") } },
        { catWorld, "Balafon", { S ("world.balafon") } },
        { catWorld, "Dan Tranh", { S ("world.dantranh") } },
        { catWorld, "Didgeridoo", { S ("world.didgeridoo") } },
        { catWorld, "Slit Drum", { S ("world.slitdrum") } },
        { catFxAtmos, "Glass Harp", { S ("fx.glass") } },
        { catFxAtmos, "Riser", { Y (catFxAtmos, "Riser") } },
        { catFxAtmos, "Ghost Voices", { S ("voice.fx.textures"), Y (catFxAtmos, "Texture", 0.5f) } },
    };
    std::vector<VaultPreset> out;
    int n = 0;
    for (const auto& [cat, name, layers] : recipes)
    {
        VaultPreset p;
        p.id = "cv.uidemo." + juce::String (n++).paddedLeft ('0', 3);
        p.name = name;
        p.category = cat;
        for (const auto& l : layers)
        {
            VaultPart part;
            if (l.kind == 'y')
            {
                const auto& pool = byCat[l.cat].empty() ? byCat[catPads] : byCat[l.cat];
                part = fb[(size_t) pool[(size_t) (n % (int) pool.size())]].parts.front();
            }
            else
            {
                part.kind = l.kind == 's' ? VaultPartKind::sample : VaultPartKind::model;
                part.source = l.src;
            }
            part.label = l.label;
            part.level = l.level;
            part.octave = l.oct;
            p.parts.push_back (part);
        }
        out.push_back (std::move (p));
    }
    return out;
}

// the v2 editor in a child process with the v2 demo bank (CHROME_VAULT_BANK) and the library chosen by the parent
// (CHROME_VAULT_LIBRARY: a missing folder, or the real library): instrument names in the list / layer rows, the calm
// "library not found" card while synth sounds keep playing, the loading state, the UI never waiting for samples
int uiV2Tests()
{
    uiSetup ("uitestv2");
    VaultEditor::simulatedDisplay() = { 0, 0, 1920, 1040 };
    VaultProcessor p;
    const auto& bank = p.bank();
    const bool lib = p.libraryFound();
    std::printf ("   (bank: %s, library: %s)\n", p.bankDescription().toRawUTF8(), lib ? p.libraryPath().getFullPathName().toRawUTF8() : "not found");
    auto find = [&] (const char* name) { for (int i = 0; i < (int) bank.size(); ++i) if (bank[(size_t) i].name == name) return i; return -1; };
    const int grandStrings = find ("Grand & Strings"), synthOnly = find ("Saw Lead"), celesta = find ("Celesta"), mallets = find ("Mallet Stack");
    check (p.bankSource() == VaultProcessor::BankSource::file && grandStrings >= 0 && synthOnly >= 0 && celesta >= 0 && mallets >= 0,
           "ui v2: the v2 demo bank is loaded (" + juce::String ((int) bank.size()) + " presets)");
    if (grandStrings < 0 || synthOnly < 0 || celesta < 0 || mallets < 0) return fails;

    VaultEditor ed (p);
    auto& v = ed.view();
    // ---- the list shows the main instrument(s); the layer rows each part's instrument / model
    {
        const auto& inst = v.browser().instruments (grandStrings);
        const auto expectStrings = lib ? juce::String ("Violin Section") : juce::String ("Ensemble Violins");
        const auto line = vui::PresetList::infoText (inst, 400.0f);
        check (inst.size() == 2 && inst[0] == "Concert Grand" && inst[1] == expectStrings && line == "Concert Grand" + vui::midDot() + expectStrings,
               "ui v2: list info line = the instruments (\"" + line + "\")");
        const auto& mi = v.browser().instruments (mallets);
        check (mi.size() == 3 && mi[2] == "Sine", "ui v2: a model layer is named by its model (" + mi.joinIntoString (", ") + ")");
        check (v.browser().instruments (synthOnly).joinIntoString (",") == "Saw Lead", "ui v2: a synth layer is named by its label");
        const auto narrow = vui::PresetList::infoText (inst, vui::textWidth (vui::PresetList::tagFont(), "Concert Grand") + 4.0f);
        check (narrow == "Concert Grand", "ui v2: instruments that do not fit are left out whole (never cut with \"...\")");
        v.setQuery (lib ? "violin section" : "ensemble violins");
        bool hit = false;
        for (int i : v.browser().visible()) hit |= i == grandStrings;
        check (hit, "ui v2: search finds a preset by its instrument name");
        v.setQuery ({});
    }
    // ---- the library is not installed: a calm card, synth presets still play
    if (! lib)
    {
        v.loadPreset (grandStrings, false);
        v.syncFromProcessor();
        auto& info = v.mainPage();
        check (info.notice() == vui::MainPage::Notice::libraryMissing && info.noticeTitle().startsWith ("Sound library not found")
                   && info.noticeTitle().contains ("run the CHROME VAULT installer") && v.statusHint().startsWith ("Sound library not found"),
               "ui v2: library missing -> \"" + info.noticeTitle() + "\" + " + p.libraryPath().getFullPathName());
        check (info.partState (bank[(size_t) grandStrings].parts[0]) == vui::MainPage::PartState::missing,
               "ui v2: its sampled layers are marked not installed");
        juce::ignoreUnused (ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f));
        v.loadPreset (synthOnly, false);
        v.syncFromProcessor();
        check (info.notice() == vui::MainPage::Notice::none && ! v.statusHint().contains ("library"), "ui v2: a synth sound shows no library notice");
        constexpr double sr = 48000.0;
        constexpr int block = 512;
        p.setPlayConfigDetails (0, 2, sr, block);
        p.prepareToPlay (sr, block);
        juce::AudioBuffer<float> buf (2, block);
        float rms = 0;
        for (int k = 0; k < 40; ++k)
        {
            juce::MidiBuffer m;
            if (k == 0) m.addEvent (juce::MidiMessage::noteOn (1, vault::homeNote, 0.8f), 0);
            p.processBlock (buf, m);
            if (k > 4) rms = std::max (rms, buf.getRMSLevel (0, 0, block));
        }
        p.releaseResources();
        check (rms > 1e-3f, "ui v2: synth presets play without the library (rms " + juce::String (rms, 4) + ")");
    }
    // ---- the library is installed: picking a sampled sound never waits; "Loading..." until its samples are in; an
    //      instrument the installed library lacks gets the "newer sound library" card
    else
    {
        // the biggest instrument of the bank that is not in RAM yet
        int pick = -1;
        int64_t biggest = -1;
        for (int i = 0; i < (int) bank.size(); ++i)
        {
            const auto& pr = bank[(size_t) i];
            if (pr.parts.size() != 1 || pr.parts[0].kind != VaultPartKind::sample) continue;
            auto* h = p.sampleLibrary()->find (pr.parts[0].source);
            if (h == nullptr || h->isLoaded()) continue;
            int64_t frames = 0;
            for (auto& a : h->desc.articulations) for (auto& z : a.zones) frames += z.frames;
            if (frames > biggest) { biggest = frames; pick = i; }
        }
        if (pick >= 0)
        {
            const auto t0 = juce::Time::getMillisecondCounterHiRes();
            v.loadPreset (pick, true);
            v.syncFromProcessor();
            const double ms = msSince (t0);
            const auto st = v.mainPage().notice();
            juce::ignoreUnused (ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f));
            check (ms < 20.0 && st == vui::MainPage::Notice::loading && v.statusHint().startsWith ("Loading samples"),
                   "ui v2: picking \"" + bank[(size_t) pick].name + "\" takes " + juce::String (ms, 2) + " ms on the UI thread and shows Loading");
            const auto until = juce::Time::getMillisecondCounter() + 30000;
            while (p.isLoading() && juce::Time::getMillisecondCounter() < until) juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
            v.syncFromProcessor();
            check (v.mainPage().notice() == vui::MainPage::Notice::none
                       && v.mainPage().partState (bank[(size_t) pick].parts[0]) == vui::MainPage::PartState::ready,
                   "ui v2: the loading state ends when the samples are in");
        }
        else check (true, "ui v2: (every demo instrument already in RAM - loading state not tested)");
        v.loadPreset (celesta, false);
        v.syncFromProcessor();
        check (v.mainPage().notice() == vui::MainPage::Notice::instrumentMissing && v.mainPage().noticeTitle().contains ("newer sound library"),
               "ui v2: an instrument the installed library lacks -> \"" + v.mainPage().noticeTitle() + "\"");
    }
    // ---- readability of the v2 states at 80 / 100 %
    for (float sc : { 0.8f, 1.0f })
    {
        ed.setUiScale (sc);
        vui::smallestFontPx() = 1000.0f;
        juce::ignoreUnused (ed.createComponentSnapshot (ed.getLocalBounds(), true, 1.0f));
        check (vui::smallestFontPx() >= vui::minTextPx, "ui v2: smallest text " + juce::String (vui::smallestFontPx() * sc, 1) + " px at "
                                                        + juce::String (juce::roundToInt (sc * 100)) + " %");
    }
    return fails;
}

// runs uiV2Tests in a child process (the bank / library are chosen per process): once without a library, once with the
// real one when it is installed (or named by CHROME_VAULT_LIBRARY)
void runUiV2Children()
{
    const auto tmp = uiSetup ("uitest-parent");
    const auto bankFile = tmp.getChildFile ("uidemo_v2.cvb");
    vault::saveBankFile (bankFile, uiDemoV2Bank());
    const auto setEnv = [] (const char* k, const juce::String& val)
    {
       #if JUCE_WINDOWS
        _putenv_s (k, val.toRawUTF8());
       #else
        if (val.isEmpty()) unsetenv (k); else setenv (k, val.toRawUTF8(), 1);
       #endif
    };
    const auto oldBank = juce::SystemStats::getEnvironmentVariable ("CHROME_VAULT_BANK", {});
    const auto oldLib = juce::SystemStats::getEnvironmentVariable ("CHROME_VAULT_LIBRARY", {});
    juce::File realLib;
    for (auto& f : vault::SampleLibrary::searchLocations()) if (f.getChildFile ("library.json").existsAsFile()) { realLib = f; break; }
    setEnv ("CHROME_VAULT_BANK", bankFile.getFullPathName());
    for (int run = 0; run < 2; ++run)
    {
        if (run == 1 && realLib == juce::File()) { std::printf ("   (no sound library installed: the loading checks are skipped)\n"); break; }
        setEnv ("CHROME_VAULT_LIBRARY", run == 0 ? tmp.getChildFile ("no-library-here").getFullPathName() : realLib.getFullPathName());
        juce::ChildProcess c;
        const auto exe = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getFullPathName();
        const bool started = c.start (juce::StringArray { exe, "-uitestv2" });
        const auto out = started ? c.readAllProcessOutput() : juce::String();
        c.waitForProcessToFinish (60000);
        const int code = started ? (int) c.getExitCode() : -1;
        std::printf ("%s", out.toRawUTF8());
        check (started && code == 0, juce::String ("ui v2: child run ") + (run == 0 ? "without a library" : "with the library") + " (exit " + juce::String (code) + ")");
    }
    setEnv ("CHROME_VAULT_BANK", oldBank);
    setEnv ("CHROME_VAULT_LIBRARY", oldLib);
}

// v3 editor pages: the knobs edit the sound / the host parameters, double-click returns to the preset, LEDs switch the
// chain slots, modulation shows as a green ring, the ADVANCED pages follow the sound
vui::Knob* findKnob (juce::Component& c, const juce::String& label)
{
    for (auto* ch : c.getChildren())
    {
        if (! ch->isVisible()) continue;
        if (auto* k = dynamic_cast<vui::Knob*> (ch); k != nullptr && k->label == label) return k;
        if (auto* k = findKnob (*ch, label)) return k;
    }
    return nullptr;
}

void v3PageTests (VaultProcessor& p)
{
    VaultEditor ed (p);
    auto& v = ed.view();
    const auto& bank = p.bank();
    int pick = -1;
    for (int i = 0; i < (int) bank.size() && pick < 0; ++i)
        if (bank[(size_t) i].parts.size() >= 2 && bank[(size_t) i].parts[0].kind != VaultPartKind::model) pick = i;
    if (pick < 0) pick = 0;
    v.loadPreset (pick, false);
    v.syncFromProcessor();
    auto& main = v.mainPage();

    // MAIN knobs = host parameters (both ways; the knob shows the effective value), double-click = as designed
    {
        auto& k = main.quickKnob (vault::qReso);
        const float designed = k.resetValue;
        const float to = designed < 0.5f ? designed + 0.3f : designed - 0.3f;
        k.userSet (to);
        const auto c = vui::val::Ctx::of (p);
        const bool drives = std::abs (p.getQuick (vault::qReso) - 0.5f) > 0.05f
                         && std::abs (vui::val::quickPosition (p.currentSound(), c, vault::qReso) - to) < 0.01f;
        p.setQuick (vault::qReso, 0.75f);
        const bool follows = std::abs (k.getValue() - vui::val::quickPosition (p.currentSound(), vui::val::Ctx::of (p), vault::qReso)) < 1e-4f;
        if (! follows) std::printf ("   reso follows: knob %.3f want %.3f dragging %d\n", k.getValue(), vui::val::quickPosition (p.currentSound(), vui::val::Ctx::of (p), vault::qReso), (int) k.isDragging());
        if (k.onReset) k.onReset();
        if (! (drives && follows && std::abs (p.getQuick (vault::qReso) - 0.5f) < 1e-6f && std::abs (k.getValue() - designed) < 1e-4f))
            std::printf ("   reso: designed %.3f to %.3f param %.3f drives %d follows %d value %.3f enabled %d inactive %d\n", designed, to, p.getQuick (vault::qReso),
                         (int) drives, (int) follows, k.getValue(), (int) k.isEnabledLook(), (int) k.inactive);
        check (drives && follows && std::abs (p.getQuick (vault::qReso) - 0.5f) < 1e-6f && std::abs (k.getValue() - designed) < 1e-4f,
               "ui v3: a MAIN knob drives its host parameter, follows the host and double-clicks back to the sound as designed ("
               + k.currentValueText() + ")");
    }
    // EFFECTS LED: switches the reverb slot (adds a default one when the sound has none) and back
    {
        auto& k = main.quickKnob (vault::qReverb);
        const bool before = p.isQuickFxOn (vault::qReverb);
        if (k.onLedClick) k.onLedClick();
        const bool mid = p.isQuickFxOn (vault::qReverb);
        if (k.onLedClick) k.onLedClick();
        check (k.hasLed && mid != before && p.isQuickFxOn (vault::qReverb) == before && k.ledOn == before,
               "ui v3: the REVERB LED switches the chain's reverb (" + juce::String (before ? "on" : "off") + " -> " + (mid ? "on" : "off") + " -> back)");
    }
    // sound macros: named per preset
    {
        bool named = true;
        for (int i = 0; i < vault::numMacros; ++i)
        {
            const auto n = p.macroName (i).trim();
            named &= n.isEmpty() ? ! main.macroKnob (i).isEnabledLook() : main.macroKnob (i).label == n.toUpperCase();
        }
        check (named, "ui v3: the 4 sound macros carry the sound's macro names (" + main.macroKnob (0).label + ", " + main.macroKnob (1).label + ", ...)");
    }
    // LAYERS: LEVEL of layer A edits the sound, Reset to preset reverts it
    {
        v.showPage (VaultMainView::pageLayers);
        auto* lvl = findKnob (v.layersPage(), "LEVEL");
        bool ok = lvl != nullptr && v.layersPage().numCards() == (int) bank[(size_t) pick].parts.size();
        if (lvl != nullptr)
        {
            lvl->userSet (0.25f);   // level 0.5 (a sound macro that sets the level gets its range moved)
            ok &= std::abs (vui::val::layerValue (p.currentSound(), vui::val::Ctx::of (p), 0, vui::val::LKey::level) - 0.5f) < 1e-3f && p.isSoundEdited();
            p.revertSound();
            v.refreshAll();
            ok &= ! p.isSoundEdited() && std::abs (lvl->getValue() - lvl->resetValue) < 1e-4f;
        }
        check (ok, "ui v3: LAYERS - one card per layer, LEVEL edits the sound, revert restores it");
    }
    // FX: a slot card per chain slot; its knobs edit the slot
    {
        p.editSound ([] (VaultPreset& x)
        {
            int rev = 0;
            for (auto& s : x.fx) rev += s.type == vault::FxType::reverb ? 1 : 0;
            if (rev == 0 && (int) x.fx.size() < vault::maxFxSlots) x.fx.push_back (VaultFxSlot::make (vault::FxType::reverb));
        });
        v.showPage (VaultMainView::pageFx);
        auto* size = findKnob (v.fxPage(), "SIZE");
        bool ok = size != nullptr;
        if (size != nullptr)
        {
            size->userSet (0.9f);
            const auto& snd = p.currentSound();
            const int slot = vui::val::firstSlotOfType (snd, vault::FxType::reverb);
            const float got = vui::val::fxValue (snd, vui::val::Ctx::of (p), slot, vault::FxType::reverb, vault::fxTypeInfo (vault::FxType::reverb).indexOf ("size"));
            ok &= std::abs (got - 0.9f) < 2e-3f;
        }
        check (ok, "ui v3: FX - the reverb card's SIZE knob edits the chain");
        p.revertSound();
    }
    // MOD: an LFO destination makes the MAIN knob green; the chip and the routing knob show up
    {
        p.editSound ([] (VaultPreset& x) { x.mod.lfo[0].dests = { { "cutoff", 1.0f } }; x.mod.lfo[0].depth = 1.0f; x.mod.modWheel = { { "fx.reverb.mix", 0.3f } }; });
        v.showPage (VaultMainView::pageMod);
        v.refreshAll();
        auto* wheel = findKnob (v.modPage(), "REVERB MIX");
        v.showPage (VaultMainView::pageMain);
        bool any = false;
        const float amt = vui::mod::amountOn (p.currentSound(), "cutoff", any);
        check (any && std::abs (amt - 0.25f) < 1e-4f && main.quickKnob (vault::qCutoff).isModulated() && wheel != nullptr
                   && main.quickKnob (vault::qReverb).isModulated(),
               "ui v3: MOD - LFO 1 -> cutoff and the wheel -> reverb mix show as green rings on the MAIN knobs, the wheel routing as a knob");
        p.revertSound();
    }
    // ARP · GATE: knobs and lanes edit the arp / gate, the power buttons are the host switches
    {
        v.showPage (VaultMainView::pageArp);
        auto* oct = findKnob (v.arpPage(), "OCTAVES");
        auto* depth = findKnob (v.arpPage(), "DEPTH");
        bool ok = oct != nullptr && depth != nullptr;
        if (ok)
        {
            oct->userSet (1.0f);
            depth->userSet (0.5f);
            ok &= p.currentSound().arp.octaves == 4 && std::abs (p.currentSound().gate.depth - 0.5f) < 1e-3f;
        }
        p.setSwitch (VaultProcessor::idArpOn, true);
        v.syncFromProcessor();
        ok &= p.getSwitch (VaultProcessor::idArpOn);
        p.setSwitch (VaultProcessor::idArpOn, false);
        check (ok, "ui v3: ARP / GATE page - OCTAVES / DEPTH edit the arp / gate");
        p.revertSound();
        v.showPage (VaultMainView::pageMain);
    }
    // report item 5 / 3: ARP RATE and GATE RATE label the whole rate ("1/16T", "1/8D") and sit at the same angle for the
    // same value; turning the ARP RATE knob reaches every rate (dotted / triplet too)
    {
        v.showPage (VaultMainView::pageArp);
        std::vector<vui::Knob*> rates;
        std::function<void (juce::Component&)> collect = [&] (juce::Component& c)
        {
            for (auto* ch : c.getChildren())
            {
                if (auto* k = dynamic_cast<vui::Knob*> (ch); k != nullptr && k->label == "RATE") rates.push_back (k);
                collect (*ch);
            }
        };
        collect (v.arpPage());
        bool ok = rates.size() == 2, labels = true, angles = true, reach = true;
        juce::String bad;
        if (ok)
        {
            for (const char* name : { "1/16T", "1/8D", "1/4", "1/64T", "1/2D", "1/32" })
            {
                const int d = vault::divisionIndex (name);
                p.editSound ([d] (VaultPreset& x) { x.arp.rate = (uint8_t) d; x.gate.rate = (uint8_t) d; });
                v.syncFromProcessor();
                v.arpPage().refresh();
                const bool l = rates[0]->currentValueText() == name && rates[1]->currentValueText() == name;
                const bool a = std::abs (rates[0]->getValue() - rates[1]->getValue()) < 1.0e-6f;
                if (! l || ! a) bad << name << " (" << rates[0]->currentValueText() << " / " << rates[1]->currentValueText() << ") ";
                labels &= l; angles &= a;
            }
            const auto& list = vui::ArpGatePage::rateKnobDivisions();
            for (int i = 0; i < (int) list.size(); ++i)
            {
                rates[0]->userSet ((float) i / (float) (list.size() - 1));
                reach &= p.currentSound().arp.rate == list[(size_t) i];
            }
            reach &= list.size() >= 19 && vault::divisionIndex ("1/16T") >= 0
                     && std::find (list.begin(), list.end(), vault::divisionIndex ("1/16T")) != list.end()
                     && std::find (list.begin(), list.end(), vault::divisionIndex ("1/64D")) != list.end();
        }
        check (ok && labels && angles && reach, "ui v3: ARP RATE shows \"1/16T\" / \"1/8D\" like GATE RATE, both knobs at the same angle for the same rate, every rate reachable " + bad);
        p.revertSound();
        v.showPage (VaultMainView::pageMain);
    }
    // a preset switch shows the new sound on every page (no stale cards)
    {
        int other = -1;
        for (int i = 0; i < (int) bank.size() && other < 0; ++i) if (i != pick && bank[(size_t) i].parts.size() != bank[(size_t) pick].parts.size()) other = i;
        if (other >= 0)
        {
            v.showPage (VaultMainView::pageLayers);
            p.selectPreset (other);   // from the host
            v.syncFromProcessor();
            check (v.layersPage().numCards() == (int) bank[(size_t) other].parts.size() && v.shownPreset() == other,
                   "ui v3: a host preset change rebuilds the LAYERS page (" + juce::String (v.layersPage().numCards()) + " cards)");
            v.showPage (VaultMainView::pageMain);
        }
    }
    p.resetQuick();
}

//==============================================================================
// v3 owner fixes (FL Studio report): the computer keyboard plays notes, typing never lands in the search box unless it
// was clicked, the same preset clicked again keeps the edits, every tab / ADVANCED switches pages, every knob shows the
// value the engine really uses (50 random presets, MAIN / FX against the running engine), LAYERS / FX / MOD knobs change
// the sound (renders)

// a processor that renders (44.1 kHz, 256-sample blocks, no host)
struct Rig
{
    static constexpr int bs = 256;
    VaultProcessor p;
    Rig() { p.prepareToPlay (44100.0, bs); }
    ~Rig() { p.releaseResources(); }
    void blocks (int n)
    {
        juce::AudioBuffer<float> b (2, bs);
        juce::MidiBuffer m;
        for (int i = 0; i < n; ++i) { b.clear(); m.clear(); p.processBlock (b, m); }
    }
    void load (int idx)
    {
        p.selectPreset (idx);
        blocks (8);
        // samples / model voices: the processor's timer services them, the audio blocks see that they are ready (the
        // engine's "loading" state is updated by process(), so blocks keep running while waiting)
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        while (p.isLoading() && juce::Time::getMillisecondCounterHiRes() - t0 < 30000.0)
        {
            if (auto* mm = juce::MessageManager::getInstanceWithoutCreating(); mm != nullptr && mm->isThisTheMessageThread()) mm->runDispatchLoopUntil (10);
            else juce::Thread::sleep (10);
            blocks (2);
        }
        blocks (40);   // the switch crossfade, the macro / knob smoothing
    }
    // one note (velocity 100), held `holdS`, then `tailS` of release / FX tail
    juce::AudioBuffer<float> note (int noteNum = vault::homeNote, double holdS = 0.7, double tailS = 0.6)
    {
        const int nHold = (int) (holdS * 44100.0 / bs), nTail = (int) (tailS * 44100.0 / bs);
        juce::AudioBuffer<float> out (2, (nHold + nTail) * bs), b (2, bs);
        juce::MidiBuffer m;
        for (int i = 0; i < nHold + nTail; ++i)
        {
            b.clear(); m.clear();
            if (i == 0) m.addEvent (juce::MidiMessage::noteOn (1, noteNum, (juce::uint8) 100), 0);
            if (i == nHold) m.addEvent (juce::MidiMessage::noteOff (1, noteNum), 0);
            p.processBlock (b, m);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, i * bs, b, c, 0, bs);
        }
        return out;
    }
};
float relDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    double d = 0, e = 0;
    const int n = std::min (a.getNumSamples(), b.getNumSamples());
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < n; ++i) { const double x = a.getSample (c, i), y = b.getSample (c, i); d += (x - y) * (x - y); e += x * x; }
    return (float) std::sqrt (d / std::max (1.0e-12, e));
}
juce::MouseEvent fakeMouse (juce::Component& c, juce::Point<float> pos = { 4.0f, 4.0f })
{
    return juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), pos, juce::ModifierKeys::leftButtonModifier, 1.0f, 0.0f, 0.0f,
                             0.0f, 0.0f, &c, &c, juce::Time::getCurrentTime(), pos, juce::Time::getCurrentTime(), 1, false);
}
void click (juce::Component& c)   // a mouse click through the component's own handlers (Component's public interface)
{
    c.mouseDown (fakeMouse (c));
    c.mouseUp (fakeMouse (c));
}
template <typename T> T* findChildOfType (juce::Component& c, const juce::String& text = {})
{
    for (auto* ch : c.getChildren())
    {
        if (auto* t = dynamic_cast<T*> (ch); t != nullptr && (text.isEmpty() || t->getButtonText() == text)) return t;
        if (auto* t = findChildOfType<T> (*ch, text)) return t;
    }
    return nullptr;
}

void ownerFixKeyboardTests()
{
    VaultProcessor p;
    VaultEditor ed (p);
    auto& v = ed.view();
    auto& keys = ed.computerKeys();
    auto& st = p.getKeyboardState();
    std::set<int> down;   // the keys "physically" down (KeyPress::isKeyCurrentlyDown has no real keyboard under xvfb)
    keys.isKeyDown = [&down] (int k) { return down.count ((int) juce::CharacterFunctions::toLowerCase ((juce::juce_wchar) k)) > 0; };
    auto press = [&] (int c) { down.insert (c); ed.keyStateChanged (true); return ed.keyPressed (juce::KeyPress (c, 0, (juce::juce_wchar) c)); };
    auto release = [&] (int c) { down.erase (c); ed.keyStateChanged (false); };

    // FL Studio layout: Z = C4 (48), Q = C5 (60), the rows chromatic
    {
        const juce::String lower ("zsxdcvgbhnjm,"), upper ("q2w3er5t6y7ui9o0p");
        bool ok = true;
        for (int i = 0; i < lower.length(); ++i) ok &= keys.noteFor (lower[i]) == 48 + i;
        for (int i = 0; i < upper.length(); ++i) ok &= keys.noteFor (upper[i]) == 60 + i;
        ok &= keys.noteFor ('a') < 0 && keys.noteFor ('f') < 0 && keys.noteFor ('Q') == 60;
        check (ok, "keys: FL Studio typing-keyboard layout (Z S X D C V G B H N J M , = C4..C5, Q 2 W 3 E R 5 T 6 Y 7 U I 9 O 0 P = C5..E6)");
    }
    {
        const bool used = press ('q');
        const bool on60 = st.isNoteOn (1, 60);
        press ('z');
        const bool both = st.isNoteOn (1, 60) && st.isNoteOn (1, 48);
        release ('q');
        const bool offQ = ! st.isNoteOn (1, 60) && st.isNoteOn (1, 48);
        release ('z');
        check (used && on60 && both && offQ && ! st.isNoteOn (1, 48) && keys.numHeld() == 0,
               "keys: Q plays C5 (MIDI 60), Z plays C4, each stops when its key goes up");
    }
    {
        ed.keyPressed (juce::KeyPress (juce::KeyPress::rightKey));
        press ('q');
        const bool up = keys.octaveShift() == 1 && st.isNoteOn (1, 72);
        ed.keyPressed (juce::KeyPress (juce::KeyPress::leftKey));   // the held note stops with the octave change
        const bool stopped = ! st.isNoteOn (1, 72);
        release ('q');
        ed.keyPressed (juce::KeyPress (juce::KeyPress::leftKey));
        press ('z');
        const bool downOct = keys.octaveShift() == -1 && st.isNoteOn (1, 36);
        release ('z');
        ed.keyPressed (juce::KeyPress (juce::KeyPress::rightKey));
        check (up && stopped && downOct && keys.octaveShift() == 0 && v.keys().computerKeysLow() == 48,
               "keys: Right / Left arrow = octave up / down (held notes end), the on-screen keyboard marks the range");
    }
    {
        press ('e');
        const bool on = st.isNoteOn (1, 64);
        ed.focusLost (juce::Component::focusChangedDirectly);
        check (on && ! st.isNoteOn (1, 64) && keys.numHeld() == 0, "keys: every note stops when the editor loses the keyboard");
        down.clear();
    }
    {
        const bool tab = ed.keyPressed (juce::KeyPress (juce::KeyPress::tabKey));
        const bool modded = ! ed.keyPressed (juce::KeyPress ('q', juce::ModifierKeys::ctrlModifier, 'q'));
        check (tab && modded && ! st.isNoteOn (1, 60), "keys: Tab is kept (never moves the keyboard into the search box), Ctrl+key is left to the host");
    }
    // the notes play: the keyboard state feeds the processor
    {
        Rig rig;
        int synth = 0;
        for (int i = 0; i < (int) rig.p.bank().size(); ++i) if (! rig.p.bank()[(size_t) i].usesSamples()) { synth = i; break; }
        rig.load (synth);
        VaultEditor ed2 (rig.p);
        std::set<int> d2;
        ed2.computerKeys().isKeyDown = [&d2] (int k) { return d2.count (k) > 0; };
        d2.insert ('q'); ed2.keyStateChanged (true); ed2.keyPressed (juce::KeyPress ('q', 0, 'q'));
        juce::AudioBuffer<float> b (2, Rig::bs);
        juce::MidiBuffer m;
        float pk = 0;
        for (int i = 0; i < 60; ++i) { b.clear(); m.clear(); rig.p.processBlock (b, m); pk = std::max (pk, b.getMagnitude (0, Rig::bs)); }
        d2.erase ('q'); ed2.keyStateChanged (false);
        check (pk > 0.01f && ! rig.p.getKeyboardState().isNoteOn (1, 60), "keys: a computer key plays the sound (peak " + juce::String (pk, 3) + ") and lets go");
    }
    // the search box: only a click puts the keyboard there; Return / Escape / a click elsewhere hand it back; a toggle never
    {
        auto& search = dynamic_cast<vui::SearchField&> (v.searchBox());
        juce::Component wrapper;   // the host's window content: what gets the keyboard when the window is activated
        wrapper.setSize (1200, 800);
        wrapper.addAndMakeVisible (ed);
        auto trav = wrapper.createKeyboardFocusTraverser();
        const auto all = trav->getAllComponents (&wrapper);
        const bool defaultIsEditor = trav->getDefaultComponent (&wrapper) == &ed && std::find (all.begin(), all.end(), &search) == all.end();
        auto* fxToggle = findChildOfType<vui::LedToggle> (v, "FX");
        const int r0 = v.focusReturns();
        static_cast<juce::Component&> (search).mouseDown (fakeMouse (search));   // a click into the search box
        const bool armed = search.getWantsKeyboardFocus() && search.clicksIn == 1;
        if (fxToggle != nullptr) ed.mouseDown (fakeMouse (*fxToggle).getEventRelativeTo (fxToggle));   // then the FX toggle
        const bool disarmed = ! search.getWantsKeyboardFocus() && v.focusReturns() == r0 + 1;
        const bool typed = ed.keyPressed (juce::KeyPress ('q', 0, 'q')) && search.isEmpty();
        ed.keyStateChanged (false);
        check (defaultIsEditor && armed && fxToggle != nullptr && disarmed && typed,
               "focus: window activation / Tab give the keyboard to the editor (never the search box); a click into the search box arms it, "
               "a click on the FX toggle takes it back - the next key plays a note, nothing lands in the search box");
        if (! (defaultIsEditor && armed && disarmed && typed))
            std::printf ("   default editor %d (default = %s), armed %d, disarmed %d (returns %d -> %d), typed %d\n", (int) defaultIsEditor,
                         trav->getDefaultComponent (&wrapper) != nullptr ? typeid (*trav->getDefaultComponent (&wrapper)).name() : "none",
                         (int) armed, (int) disarmed, r0, v.focusReturns(), (int) typed);
        wrapper.removeChildComponent (&ed);
        // (a real window round trip needs a window manager: xvfb has none, the keyboard state above is what JUCE routes)
    }
}

void ownerFixPageTests()
{
    VaultProcessor p;
    VaultEditor ed (p);
    auto& v = ed.view();
    const auto& bank = p.bank();
    // tabs: a mouse-down on any tab switches the page; ADVANCED opens LAYERS
    {
        bool ok = true;
        for (int pg : { (int) VaultMainView::pageLayers, (int) VaultMainView::pageFx, (int) VaultMainView::pageMod, (int) VaultMainView::pageArp, (int) VaultMainView::pageMain })
        {
            click (v.tab (pg));
            ok &= v.currentPage() == pg && v.pageComponent (pg).isVisible();
        }
        click (v.advancedButton());
        ok &= v.currentPage() == VaultMainView::pageLayers && v.advancedButton().getToggleState();
        click (v.tab (VaultMainView::pageMain));
        ok &= v.currentPage() == VaultMainView::pageMain && ! v.advancedButton().getToggleState();
        check (ok, "tabs: MAIN / LAYERS / FX / MOD / ARP - GATE switch on a click, ADVANCED is a clickable group header (opens LAYERS)");
    }
    // the same preset clicked again keeps every edit; another preset starts from the bank preset
    {
        int idx = -1;
        for (int i = 0; i < (int) bank.size() && idx < 0; ++i) if (bank[(size_t) i].parts.size() >= 2) idx = i;
        v.loadPreset (idx, false);
        p.editSound ([] (VaultPreset& x) { x.parts[0].level = 0.37f; if (! x.fx.empty()) x.fx[0].on = ! x.fx[0].on; });
        p.setQuick (vault::qReverb, 0.8f);
        v.presetList().onClick (idx);   // the list row of the current sound
        const bool kept = p.isSoundEdited() && std::abs (p.currentSound().parts[0].level - 0.37f) < 1e-6f && std::abs (p.getQuick (vault::qReverb) - 0.8f) < 1e-6f;
        v.presetList().onClick (idx + 1);
        v.presetList().onClick (idx);
        check (kept && ! p.isSoundEdited() && std::abs (p.getQuick (vault::qReverb) - 0.5f) < 1e-6f,
               "browse: clicking the sound that plays keeps its edits (layers, FX, MAIN knobs); another sound and back starts fresh");
    }
}

// every knob shows what the engine uses: 50 random presets, random MAIN knobs / macros, MAIN + FX knobs against the running
// engine's FX chain, LAYERS knobs against the effective values, knob position == value text
void ownerFixValueTests()
{
    Rig rig;
    auto& p = rig.p;
    VaultEditor ed (p);
    auto& v = ed.view();
    const auto& bank = p.bank();
    juce::Random rnd (4711);
    int presets = 0, knobsChecked = 0, engineChecked = 0, bad = 0, moved = 0;
    juce::String firstBad;
    auto fail = [&] (const juce::String& what) { if (bad++ == 0) firstBad = what; };
    std::vector<float> lastPos (vault::numQuick, -1.0f);
    for (int n = 0; n < 50; ++n)
    {
        const int idx = rnd.nextInt ((int) bank.size());
        v.loadPreset (idx, false);
        // a few MAIN knobs and a macro moved (host side, like automation)
        for (int q : { (int) vault::qReverb, (int) vault::qDelay, (int) vault::qCutoff, (int) vault::qAttack })
            if (rnd.nextBool()) p.setQuick (q, rnd.nextFloat());
        if (rnd.nextBool()) p.setMacro (rnd.nextInt (4), rnd.nextFloat());
        rig.blocks (8);
        waitUntilPlayable (p, 20000);
        rig.blocks (220);   // the knob / macro smoothing lands exactly
        v.syncFromProcessor(); v.syncFromProcessor(); v.syncFromProcessor();
        ++presets;
        const auto& snd = p.currentSound();
        const auto c = vui::val::Ctx::of (p);
        const auto& eng = p.audioEngine();
        auto engineEntry = [&] (vault::FxType t, int slot)
        {
            for (int e = 0; e < eng.chainSize(); ++e)
                if (eng.chainType (e) == t && eng.chainSlot (e) == slot) return e;
            return -1;
        };
        // MAIN
        auto& main = v.mainPage();
        for (int q = 0; q < vault::numQuick; ++q)
        {
            auto& k = main.quickKnob (q);
            const float want = k.inactive ? 0.0f : vui::val::quickPosition (snd, c, q);   // an effect that is off sits at the minimum
            ++knobsChecked;
            {   // "off" / "0%" = the knob at its minimum (owner report item 3)
                const auto txt = k.currentValueText();
                if ((txt == "off" || txt.startsWith ("0% ") || txt == "0%") && k.arcOrigin < 0.01f && k.getValue() > 1e-3f)
                    fail ("MAIN " + k.label + " reads " + txt + " but sits at " + juce::String (k.getValue(), 3));
            }
            if (std::abs (k.getValue() - want) > 1e-4f) fail ("MAIN " + k.label + " at " + juce::String (k.getValue(), 3) + ", effective " + juce::String (want, 3));
            if (k.currentValueText() != vui::val::quickText (snd, c, q, k.getValue(), p.getSwitch (VaultProcessor::idFxOn), p.isQuickFxOn (q)))
                fail ("MAIN " + k.label + " text");
            if (lastPos[(size_t) q] >= 0 && std::abs (lastPos[(size_t) q] - k.getValue()) > 1e-3f) ++moved;
            lastPos[(size_t) q] = k.getValue();
            const auto view = vui::val::quickView (snd, q);
            if (k.inactive)
            {
                if (k.currentValueText() != "off") fail ("MAIN " + k.label + " is off but reads " + k.currentValueText());
                continue;
            }
            if (view.kind == vui::val::QuickView::fxKnob && ! k.isModulated())   // (a velocity / wheel routing moves the engine's value: the green ring)
            {
                const int e = engineEntry (view.type, view.slot);
                if (e < 0) { fail ("MAIN " + k.label + ": no engine slot"); continue; }
                const float engineV = eng.chainParam (e, vault::fxMainParam (view.type));
                const float shown = view.fromNorm (k.getValue());
                ++engineChecked;
                if (std::abs (engineV - shown) > 0.002f * std::max (1.0f, std::abs (view.hi - view.lo)))
                    fail ("MAIN " + k.label + " shows " + juce::String (shown, 4) + ", engine " + juce::String (engineV, 4) + " (" + snd.name + ")");
            }
        }
        // FX page
        v.showPage (VaultMainView::pageFx);
        for (auto& sk : v.fxPage().slotKnobs())
        {
            const auto t = snd.fx[(size_t) sk.slot].type;
            const auto& in = vault::fxTypeInfo (t).params[(size_t) sk.param];
            const int e = engineEntry (t, sk.slot);
            ++knobsChecked;
            if (e < 0) { fail ("FX slot " + juce::String (sk.slot) + ": no engine slot"); continue; }
            if (sk.knob->isModulated()) continue;   // the green ring: a routing / LFO moves the engine's value
            const float engineV = eng.chainParam (e, sk.param);
            ++engineChecked;
            if (std::abs (in.toNorm (engineV) - sk.knob->getValue()) > 0.003f)
                fail ("FX " + juce::String (in.name) + " knob " + juce::String (in.fromNorm (sk.knob->getValue()), 3) + ", engine " + juce::String (engineV, 3) + " (" + snd.name + ")");
        }
        // LAYERS: level / pan / cutoff / attack ... are the effective values
        v.showPage (VaultMainView::pageLayers);
        for (int L = 0; L < v.layersPage().numCards(); ++L)
            if (auto* card = v.layersPage().cardComponent (L))
                for (auto [label, key] : { std::pair<const char*, vui::val::LKey> { "LEVEL", vui::val::LKey::level }, { "PAN", vui::val::LKey::pan } })
                    if (auto* k = findKnob (*card, label))
                    {
                        ++knobsChecked;
                        const float eff = vui::val::layerValue (snd, c, L, key);
                        const float pos = key == vui::val::LKey::level ? eff / 2.0f : (eff + 1.0f) * 0.5f;
                        if (std::abs (k->getValue() - pos) > 1e-4f) fail ("LAYERS " + juce::String (label));
                    }
        v.showPage (VaultMainView::pageMain);
        p.resetQuick();
    }
    check (bad == 0 && presets == 50 && engineChecked > 300 && moved > 50,
           "values: 50 random presets - " + juce::String (knobsChecked) + " knobs at their effective value, " + juce::String (engineChecked)
           + " MAIN / FX knobs == the running engine, knobs move with the preset (" + juce::String (moved) + " moves)"
           + (bad > 0 ? " - " + juce::String (bad) + " bad, first: " + firstBad : juce::String()));

    // turning a knob makes its value what plays: the FX page reverb mix (a sound macro sets it in most presets) and a MAIN knob
    {
        int idx = -1;
        for (int i = 0; i < (int) bank.size() && idx < 0; ++i)
        {
            auto x = bank[(size_t) i];
            vault::upgradeToV3 (x);
            for (auto& m : x.macros) for (auto& t : m.targets) if (t.target == "fx.reverb.mix" && ! x.usesSamples()) idx = i;
        }
        if (idx >= 0)
        {
            v.loadPreset (idx, false);
            rig.blocks (60);
            v.showPage (VaultMainView::pageFx);
            vui::Knob* mix = nullptr; int slot = -1, param = -1;
            for (auto& sk : v.fxPage().slotKnobs())
                if (p.currentSound().fx[(size_t) sk.slot].type == vault::FxType::reverb && juce::String (vault::fxTypeInfo (vault::FxType::reverb).params[(size_t) sk.param].id) == "mix")
                { mix = sk.knob; slot = sk.slot; param = sk.param; break; }
            bool ok = mix != nullptr;
            if (ok)
            {
                p.setQuick (vault::qReverb, 0.8f);
                rig.blocks (60);
                mix->userSet (0.42f);
                rig.blocks (220);
                v.syncFromProcessor(); v.syncFromProcessor(); v.syncFromProcessor();
                int e = -1;
                for (int i = 0; i < p.audioEngine().chainSize(); ++i) if (p.audioEngine().chainSlot (i) == slot) e = i;
                const float engineV = e >= 0 ? p.audioEngine().chainParam (e, param) : -1.0f;
                ok &= std::abs (engineV - 0.42f) < 0.003f && std::abs (mix->getValue() - 0.42f) < 0.003f
                   && std::abs (p.getQuick (vault::qReverb) - 0.5f) < 1e-6f
                   && std::abs (v.mainPage().quickKnob (vault::qReverb).getValue() - 0.42f) < 0.003f;
                std::printf ("   reverb mix: engine %.3f, FX knob %.3f, MAIN knob %.3f\n", engineV, mix->getValue(), v.mainPage().quickKnob (vault::qReverb).getValue());
            }
            check (ok, "values: the FX page reverb MIX (set by a sound macro) turned to 42% plays 42%; the MAIN REVERB knob shows the same");
            v.showPage (VaultMainView::pageMain);
        }
    }
}

// the rest of the owner's report in the editor: browse memory (6), modulation ring (7), search (8), window size (9), FX
// slots (11), names (12), keyboard (13), edited badge (14), one-layer sounds (15), host text (16), Return / Escape
int countButtons (juce::Component& c, const juce::String& text)
{
    int n = 0;
    for (auto* ch : c.getChildren())
    {
        if (auto* b = dynamic_cast<juce::Button*> (ch); b != nullptr && b->isVisible() && b->getButtonText() == text) ++n;
        n += countButtons (*ch, text);
    }
    return n;
}

void ownerFixMoreTests()
{
    using namespace vault;
    // ---- 8: search - the best name matches first, the category counts follow the results
    {
        VaultProcessor p;
        const auto& bank = p.bank();
        VaultBrowserModel m (bank);
        bool order = true, exact = true, counts = true;
        int queries = 0;
        juce::Random rnd (99);
        for (int t = 0; t < 40; ++t)
        {
            const auto& pr = bank[(size_t) rnd.nextInt ((int) bank.size())];
            const auto word = pr.name.upToFirstOccurrenceOf (" ", false, false).toLowerCase();
            for (const auto& q : { word, pr.name.toLowerCase() })
            {
                m.filter = {};
                m.filter.query = q;
                m.refilter();
                ++queries;
                auto words = juce::StringArray::fromTokens (q, " ,;/+", "\"");
                words.removeEmptyStrings();
                int last = -1;
                for (int i : m.visible()) { const int sc = m.nameScore (i, q.trim(), words); order &= sc >= last; last = sc; }
                if (q == pr.name.toLowerCase())
                    exact &= ! m.visible().empty() && bank[(size_t) m.visible().front()].name.equalsIgnoreCase (pr.name);
                int sum = 0;
                for (int c = 0; c < vault::numCategories; ++c) sum += m.categoryCount (c);
                counts &= sum == m.categoryCount (-1) && m.categoryCount (-1) == (int) m.visible().size();
                m.filter.category = pr.category;
                m.refilter();
                counts &= (int) m.visible().size() == m.categoryCount (pr.category);
            }
        }
        m.filter = {}; m.filter.query = "warm"; m.refilter();
        juce::String top;
        for (int i = 0; i < std::min (3, (int) m.visible().size()); ++i) top << bank[(size_t) m.visible()[(size_t) i]].name << " | ";
        std::printf ("   \"warm\" -> %s (%d found)\n", top.toRawUTF8(), (int) m.visible().size());
        check (order && exact && counts, "search: exact name first, names starting with the word next, then the rest; the category counts follow the results ("
               + juce::String (queries) + " searches)");
    }
    // ---- 6: the window opens on the category it showed, scrolled to the sound that plays (also the first time)
    {
        VaultProcessor p;
        const auto& bank = p.bank();
        int deep = -1;
        for (int i = (int) bank.size() - 1; i >= 0 && deep < 0; --i) if (bank[(size_t) i].category == catStrings) deep = i;
        bool firstOk = false, keptOk = false;
        {
            p.selectPreset ((int) bank.size() * 3 / 4);
            VaultEditor ed (p);
            auto& v = ed.view();
            firstOk = v.presetList().isRowVisible (v.browser().rowOf (p.currentPreset())) && v.shownPreset() == p.currentPreset();
            v.setCategory (catStrings);
            v.loadPreset (deep, false);
        }
        {
            VaultEditor ed (p);
            auto& v = ed.view();
            keptOk = v.browser().filter.category == catStrings && v.mainPage().categoryList().getSelected() == catStrings
                     && v.presetList().isRowVisible (v.browser().rowOf (deep)) && v.shownPreset() == deep;
        }
        juce::MemoryBlock st;
        p.getStateInformation (st);
        VaultProcessor p2;
        p2.setStateInformation (st.getData(), (int) st.getSize());
        check (firstOk && keptOk && p2.editorMemory.category == catStrings,
               "browse: the list opens scrolled to the playing sound (first open too); a reopened window keeps the category (saved with the project)");
    }
    // ---- Return / Escape in the search box hand the keyboard back
    {
        VaultProcessor p;
        VaultEditor ed (p);
        auto& v = ed.view();
        auto& search = dynamic_cast<vui::SearchField&> (v.searchBox());
        bool ok = true;
        for (int key : { juce::KeyPress::returnKey, juce::KeyPress::escapeKey })
        {
            static_cast<juce::Component&> (search).mouseDown (fakeMouse (search));
            ok &= search.getWantsKeyboardFocus();
            const int r0 = v.focusReturns();
            search.keyPressed (juce::KeyPress (key));
            juce::MessageManager::getInstance()->runDispatchLoopUntil (30);   // (TextEditor posts Return / Escape)
            ok &= v.focusReturns() == r0 + 1 && ! search.getWantsKeyboardFocus();
        }
        check (ok, "focus: Return and Escape in the search box hand the keyboard back (the keys play again)");
    }
    // ---- 2 + 14: any edit shows "edited"; clicking the sound again keeps edits and badge; Revert is explicit
    {
        VaultProcessor p;
        VaultEditor ed (p);
        auto& v = ed.view();
        int idx = 0;   // a synth sound: no "Loading..." in the badge's place
        for (int i = 5; i < (int) p.bank().size(); ++i) if (! p.bank()[(size_t) i].usesSamples()) { idx = i; break; }
        v.loadPreset (idx, false);
        auto paintOnce = [&] { ed.createComponentSnapshot (ed.getLocalBounds()); return ! v.mainPage().editedBadge().isEmpty(); };
        const bool clean = ! p.hasEdits() && ! paintOnce();
        if (! clean) std::printf ("   not clean after load: hasEdits %d, sound edited %d\n", (int) p.hasEdits(), (int) p.isSoundEdited());
        // three MAIN knobs (the owner's check)
        p.setQuick (qCutoff, 0.7f); p.setQuick (qReverb, 0.8f); p.setQuick (qAttack, 0.3f);
        v.syncFromProcessor(); v.syncFromProcessor(); v.syncFromProcessor();
        const bool badge = p.hasEdits() && paintOnce();
        v.presetList().onClick (idx);
        v.syncFromProcessor(); v.syncFromProcessor(); v.syncFromProcessor();
        const bool kept = std::abs (p.getQuick (qCutoff) - 0.7f) < 1e-6f && std::abs (p.getQuick (qReverb) - 0.8f) < 1e-6f
                       && std::abs (p.getQuick (qAttack) - 0.3f) < 1e-6f && paintOnce();
        v.mainPage().revertToPreset();
        const bool reverted = ! p.hasEdits() && ! paintOnce();
        p.setMacro (0, p.bank()[(size_t) idx].macroDefaults[0] > 0.5f ? 0.1f : 0.9f);
        const bool macroEdit = p.hasEdits();
        p.revertAll();
        p.setSwitch (VaultProcessor::idFxOn, false);
        const bool switchEdit = p.hasEdits();
        p.revertAll();
        if (! (clean && badge && kept && reverted && macroEdit && switchEdit))
            std::printf ("   clean %d badge %d kept %d reverted %d macro %d switch %d\n", (int) clean, (int) badge, (int) kept, (int) reverted, (int) macroEdit, (int) switchEdit);
        check (clean && badge && kept && reverted && macroEdit && switchEdit && ! p.hasEdits(),
               "edited: 3 MAIN knobs edited -> \"edited\"; the same sound clicked again keeps the values and the badge; a macro or a switch counts too; Revert clears it");
    }
    // ---- 7: right-click -> Modulate with... : the green ring shows at once on every page
    {
        Rig rig;
        int idx = -1;
        for (int i = 0; i < (int) rig.p.bank().size() && idx < 0; ++i)
            if (rig.p.bank()[(size_t) i].parts[0].kind == VaultPartKind::synth && ! rig.p.bank()[(size_t) i].usesSamples()) idx = i;
        rig.load (std::max (0, idx));
        VaultEditor ed (rig.p);
        auto& v = ed.view();
        auto& ctx = v.context();
        juce::StringArray bad;
        auto tryKnob = [&] (const juce::String& name, int page, const std::function<vui::Knob* ()>& find, const juce::String& target)
        {
            v.showPage (page);
            auto* k = find();
            if (k == nullptr) { bad.add (name + " (no knob)"); return; }
            vui::mod::apply (ctx, target, -1);
            const bool before = k->isModulated();
            vui::mod::apply (ctx, target, 2);   // mod wheel
            if (before || ! k->isModulated()) bad.add (name);
            vui::mod::apply (ctx, target, -1);
            if (k->isModulated()) bad.add (name + " (remove)");
        };
        tryKnob ("MAIN cutoff", VaultMainView::pageMain, [&] { return &v.mainPage().quickKnob (qCutoff); }, vui::MainPage::quickTarget (qCutoff));
        tryKnob ("MAIN reverb", VaultMainView::pageMain, [&] { return &v.mainPage().quickKnob (qReverb); }, vui::MainPage::quickTarget (qReverb));
        tryKnob ("LAYERS cutoff", VaultMainView::pageLayers, [&] { auto* c = v.layersPage().cardComponent (0); return c != nullptr ? findKnob (*c, "CUTOFF") : nullptr; }, "layer1.cutoff");
        if (! rig.p.currentSound().fx.empty())
            tryKnob ("FX slot 1", VaultMainView::pageFx, [&] { auto ks = v.fxPage().slotKnobs(); for (auto& sk : ks) if (sk.slot == 0) return sk.knob; return (vui::Knob*) nullptr; },
                     [&] { for (auto& sk : v.fxPage().slotKnobs()) if (sk.slot == 0) return "fx1." + juce::String (vault::fxTypeInfo (rig.p.currentSound().fx[0].type).params[(size_t) sk.param].id); return juce::String(); }());
        tryKnob ("MOD LFO 1 rate", VaultMainView::pageMod, [&] { return findKnob (v.modPage(), "RATE"); }, "lfo1.rate");
        v.showPage (VaultMainView::pageMain);
        check (bad.isEmpty(), "modulation: \"Modulate with...\" draws the green ring at once on MAIN / LAYERS / FX / MOD" + (bad.isEmpty() ? juce::String() : " - not: " + bad.joinIntoString (", ")));
    }
    // ---- 11 + 12: empty FX slots all "+ Add effect"; "Mono Below 0 Hz" = off; a slot target named by its effect
    {
        VaultProcessor p;
        const auto& bank = p.bank();
        int idx = -1;
        for (int i = 0; i < (int) bank.size() && idx < 0; ++i)
        {
            auto x = bank[(size_t) i];
            upgradeToV3 (x);
            if (x.fx.size() >= 2 && x.fx.size() <= 5) idx = i;
        }
        VaultEditor ed (p);
        auto& v = ed.view();
        v.loadPreset (std::max (0, idx), false);
        v.showPage (VaultMainView::pageFx);
        const int empty = (int) maxFxSlots - (int) p.currentSound().fx.size();
        const int adds = countButtons (v.fxPage(), "Add effect");
        const auto& wi = fxTypeInfo (FxType::width);
        const auto monoTxt = vui::fxValueText (wi.params[(size_t) wi.indexOf ("monoBelow")], 0.0f);
        VaultPreset snd;
        snd.fx = { VaultFxSlot::make (FxType::delay), VaultFxSlot::make (FxType::reverb) };
        const auto nm = vui::mod::shortName ("fx2.mix", &snd);
        v.showPage (VaultMainView::pageMain);
        check (adds == empty && monoTxt == "off" && nm == "Reverb Mix",
               "FX: every empty slot shows + Add effect (" + juce::String (adds) + "/" + juce::String (empty) + "), mono below 0 Hz reads \"" + monoTxt
               + "\", fx2.mix of a delay + reverb chain is \"" + nm + "\"");
    }
    // ---- 13: the on-screen keyboard shows what sounds - nothing at rest (no lit C5), the played key, the arp's latched notes
    {
        Rig rig;
        int idx = -1;
        for (int i = 0; i < (int) rig.p.bank().size() && idx < 0; ++i)
            if (! rig.p.bank()[(size_t) i].usesSamples() && rig.p.bank()[(size_t) i].category == catPlucks) idx = i;
        rig.load (std::max (0, idx));
        VaultEditor ed (rig.p);
        auto& v = ed.view();
        auto& kb = v.keys();
        auto lit = [&] { std::vector<int> n; for (int k = 0; k < 128; ++k) if (kb.isLit (k)) n.push_back (k); return n; };
        v.syncFromProcessor();
        const bool rest = lit().empty();
        rig.p.getKeyboardState().noteOn (1, 64, 0.8f);
        rig.blocks (4);
        v.syncFromProcessor();
        const auto one = lit();
        rig.p.getKeyboardState().noteOff (1, 64, 0.0f);
        bool dark = false;
        for (int i = 0; i < 400 && ! dark; ++i) { rig.blocks (4); v.syncFromProcessor(); dark = lit().empty(); }
        // arp, latched: the keys go up, its notes keep lighting
        rig.p.editSound ([] (VaultPreset& x) { x.arp.on = true; x.arp.latch = true; x.arp.mode = VaultArp::up; });
        rig.p.setSwitch (VaultProcessor::idArpOn, true);
        rig.blocks (4);
        for (int n : { 60, 64, 67 }) rig.p.getKeyboardState().noteOn (1, n, 0.8f);
        rig.blocks (2);
        for (int n : { 60, 64, 67 }) rig.p.getKeyboardState().noteOff (1, n, 0.0f);
        std::set<int> seen;
        for (int i = 0; i < 60; ++i) { rig.blocks (3); v.syncFromProcessor(); for (int n : lit()) seen.insert (n); }
        const bool arpOk = seen.size() >= 2 && std::all_of (seen.begin(), seen.end(), [] (int n) { return n % 12 == 0 || n % 12 == 4 || n % 12 == 7; });
        rig.p.setSwitch (VaultProcessor::idArpOn, false);
        check (rest && one == std::vector<int> { 64 } && dark && arpOk,
               "keyboard: nothing lit at rest (no permanent C5), a played key lights while it sounds, the latched arp's notes light (" + juce::String ((int) seen.size()) + " notes)");
    }
    // ---- 15: a one-layer sound fills the LAYERS page (+ Add layer columns); adding / removing a layer works
    {
        VaultProcessor p;
        const auto& bank = p.bank();
        int idx = -1;
        for (int i = 0; i < (int) bank.size() && idx < 0; ++i) if (bank[(size_t) i].parts.size() == 1 && ! bank[(size_t) i].usesSamples()) idx = i;
        VaultEditor ed (p);
        auto& v = ed.view();
        v.loadPreset (std::max (0, idx), false);
        v.showPage (VaultMainView::pageLayers);
        auto& lp = v.layersPage();
        const bool filled = lp.numCards() == 1 && lp.numAddCards() == 1 && countButtons (lp, "Add layer") == 1
                         && lp.cardComponent (0)->getRight() > lp.getWidth() / 3 + 40;   // the layer card takes half the page
        lp.addLayer (1);
        lp.refresh();
        const bool added = p.currentSound().parts.size() == 2 && p.currentSound().parts[1].octave == p.currentSound().parts[0].octave + 1
                         && lp.numCards() == 2 && lp.numAddCards() == 1;   // (3 columns: A, B, + Add layer)
        lp.removeLastLayer();
        lp.refresh();
        const bool removed = p.currentSound().parts.size() == 1 && lp.numAddCards() == 1;
        v.showPage (VaultMainView::pageMain);
        check (filled && added && removed, "LAYERS: a one-layer sound fills half the page next to + Add layer; a copy an octave up adds a layer, the last one can go again");
    }
    // ---- 16: host parameter text in the plugin's units
    {
        VaultProcessor p;
        VaultEditor ed (p);
        auto& v = ed.view();
        auto* cut = p.apvts.getParameter (VaultProcessor::idQuick[qCutoff]);
        juce::String other, otherRev;
        std::thread t ([&] { other = cut->getText (0.35f, 64); otherRev = p.apvts.getParameter (VaultProcessor::idQuick[qReverb])->getText (0.5f, 64); });
        t.join();
        const auto mine = cut->getText (cut->getValue(), 64);
        const auto knob = v.mainPage().quickKnob (qCutoff).currentValueText();
        const auto revHost = p.apvts.getParameter (VaultProcessor::idQuick[qReverb])->getText (p.getQuick (qReverb), 64);
        const auto revKnob = v.mainPage().quickKnob (qReverb).currentValueText();
        const bool ok = other == juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")) + "1.2 oct" && otherRev == "as designed" && mine == knob && revHost == revKnob
                        && ! mine.contains ("%") && VaultProcessor::quickParamFromText (qCutoff, other) > 0.34f && VaultProcessor::quickParamFromText (qCutoff, other) < 0.36f;
        std::printf ("   host text: cutoff \"%s\" (knob \"%s\"), other thread \"%s\", reverb \"%s\" (knob \"%s\")\n", mine.toRawUTF8(), knob.toRawUTF8(),
                     other.toRawUTF8(), revHost.toRawUTF8(), revKnob.toRawUTF8());
        check (ok, "host text: the MAIN knob parameters read like the knobs (\"" + mine + "\", \"" + revHost + "\"), offsets in units elsewhere (\"" + other + "\")");
    }
    // ---- 9: the size menu offers only sizes that fit; the window never gets larger than the screen; the menu shows fully
    {
        const auto keep = VaultEditor::simulatedDisplay();
        VaultEditor::simulatedDisplay() = { 0, 0, 1366, 728 };   // a 1366 x 768 laptop with a taskbar
        VaultProcessor p;
        VaultEditor ed (p);
        auto& v = ed.view();
        const auto area = VaultEditor::simulatedDisplay();
        bool fitAll = ! v.scaleChoices().empty();
        for (float sc : v.scaleChoices()) fitAll &= VaultEditor::fits (sc, area) || sc <= VaultEditor::minScale + 0.001f;
        ed.setUiScale (1.5f);
        const bool clamped = ed.getWidth() <= area.getWidth() && ed.getHeight() <= area.getHeight();
        const auto* cons = ed.getConstrainer();
        const bool limits = cons != nullptr && cons->getMaximumWidth() <= area.getWidth() && cons->getMaximumHeight() <= area.getHeight();
        // the menu (a desktop window JUCE keeps inside the screen of its target) fits under the size button: the window is on
        // the screen and the menu is shorter than the window below the button (xvfb has no window manager for a real one)
        const float sc = ed.getUiScale();
        const int menuH = juce::roundToInt ((float) (v.scaleChoices().size() * 26 + 8) * sc);
        const auto btnBottom = juce::roundToInt ((float) (VaultMainView::topH - 12) * sc);
        const bool menuShown = true, menuInside = btnBottom + menuH <= ed.getHeight() && ed.getHeight() <= area.getHeight();
        const int before = juce::Desktop::getInstance().getNumComponents();
        VaultEditor::simulatedDisplay() = keep;
        check (fitAll && clamped && limits && menuShown && menuInside && juce::Desktop::getInstance().getNumComponents() <= before,
               "size: the menu lists only sizes that fit the screen (" + juce::String ((int) v.scaleChoices().size()) + " on 1366 x 728), 150% asked on that screen gives "
               + juce::String (ed.getWidth()) + " x " + juce::String (ed.getHeight()) + ", the corner drag stops at the screen, the menu opens fully on the screen");
    }
}

// LAYERS / FX / MOD controls change the sound: two identical processors, the knob moved on one, the renders differ
void ownerFixRenderTests()
{
    VaultProcessor probe;
    const auto& bank = probe.bank();
    // presets: a synth-only one with a level macro, a sampled one (with the library), one with a model layer
    std::vector<int> picks;
    auto hasLevelMacro = [] (VaultPreset x) { vault::upgradeToV3 (x); for (auto& m : x.macros) for (auto& t : m.targets) if (t.target.endsWith (".level")) return true; return false; };
    for (int i = 0; i < (int) bank.size() && picks.size() < 1; ++i)
        if (! bank[(size_t) i].usesSamples() && bank[(size_t) i].parts[0].kind == VaultPartKind::synth && hasLevelMacro (bank[(size_t) i])) picks.push_back (i);
    if (probe.libraryFound())
        for (int i = 0; i < (int) bank.size() && picks.size() < 2; ++i)
            if (bank[(size_t) i].parts.size() == 1 && bank[(size_t) i].parts[0].kind == VaultPartKind::sample && bank[(size_t) i].category == vault::catStrings) picks.push_back (i);
    for (int i = 0; i < (int) bank.size(); ++i)
        if (bank[(size_t) i].parts[0].kind == VaultPartKind::model && ! bank[(size_t) i].usesSamples()) { picks.push_back (i); break; }

    for (int idx : picks)
    {
        const auto& pr = bank[(size_t) idx];
        Rig ref;
        ref.load (idx);
        const auto a = ref.note();
        juce::String report;
        int silent = 0, total = 0;
        juce::StringArray dead;
        auto tryControl = [&] (int page, const std::function<vui::Knob* (VaultMainView&)>& find, const juce::String& name, bool mustChange)
        {
            Rig rig;
            rig.load (idx);
            VaultEditor ed (rig.p);
            auto& v = ed.view();
            v.showPage (page);
            auto* k = find (v);
            if (k == nullptr || ! k->isEnabledLook()) return;
            const float to = k->getValue() < 0.5f ? 1.0f : 0.0f;
            k->userSet (to);
            rig.blocks (20);
            const float d = relDiff (a, rig.note());
            ++total;
            report << name << " " << juce::String (d, 2) << "  ";
            if (d < 0.02f) { ++silent; if (mustChange) dead.add (name); }
        };
        // baseline: an untouched second processor renders the same
        {
            Rig same;
            same.load (idx);
            const float d0 = relDiff (a, same.note());
            report << "baseline " << juce::String (d0, 4) << " | ";
        }
        const bool synthA = pr.parts[0].kind == VaultPartKind::synth, sampleA = pr.parts[0].kind == VaultPartKind::sample;
        for (auto [label, must] : { std::pair<const char*, bool> { "LEVEL", true }, { "PAN", true }, { "OCTAVE", true }, { "FINE", synthA }, { "SEMI", true },
                                    { "A", synthA || sampleA }, { "D", false }, { "S", false }, { "R", synthA || sampleA }, { "CUTOFF", synthA || sampleA },
                                    { "RESO", false }, { "ENV", false }, { "VEL", false }, { "KEY", false } })
            tryControl (VaultMainView::pageLayers, [label = juce::String (label)] (VaultMainView& v) { auto* c = v.layersPage().cardComponent (0); return c != nullptr ? findKnob (*c, label) : nullptr; },
                        juce::String ("L1 ") + label, must);
        for (auto [label, must] : { std::pair<const char*, bool> { "UNISON", synthA }, { "DETUNE", false }, { "VEL SENS", false } })
            tryControl (VaultMainView::pageLayers, [label = juce::String (label)] (VaultMainView& v) { return findKnob (v.layersPage(), label); },
                        juce::String ("VOICE ") + label, must);
        // the FX slot knobs (the wet effects: mix / amount knobs must change the sound)
        {
            Rig rig; rig.load (idx);
            VaultEditor ed (rig.p);
            ed.view().showPage (VaultMainView::pageFx);
            const auto sks = ed.view().fxPage().slotKnobs();
            for (int i = 0; i < (int) sks.size(); ++i)
            {
                const auto t = rig.p.currentSound().fx[(size_t) sks[(size_t) i].slot].type;
                const auto& in = vault::fxTypeInfo (t).params[(size_t) sks[(size_t) i].param];
                const bool must = rig.p.currentSound().fx[(size_t) sks[(size_t) i].slot].on && juce::String (in.id) == "mix";
                tryControl (VaultMainView::pageFx, [i] (VaultMainView& v) { auto ks = v.fxPage().slotKnobs(); return i < (int) ks.size() ? ks[(size_t) i].knob : nullptr; },
                            juce::String ("FX") + juce::String (sks[(size_t) i].slot + 1) + " " + vault::fxTypeInfo (t).name + " " + in.name, must);
            }
        }
        // MOD: LFO 1 rate (when it moves something)
        if (! pr.v3 || true)
        {
            auto x = pr; vault::upgradeToV3 (x);
            if (! x.mod.lfo[0].dests.empty() && ! x.mod.lfo[0].sync)
                tryControl (VaultMainView::pageMod, [] (VaultMainView& v) { return findKnob (v.modPage(), "RATE"); }, "LFO1 RATE", false);
        }
        std::printf ("   %s: %s\n", pr.name.toRawUTF8(), report.toRawUTF8());
        check (dead.isEmpty() && total > 8, "render: every LAYERS / FX control of \"" + pr.name + "\" changes the sound (" + juce::String (total) + " controls"
               + (dead.isEmpty() ? juce::String (")") : "; no effect: " + dead.joinIntoString (", ") + ")"));
    }
}

// editor behaviour (run under xvfb-run): keys step through the filtered list, favorites / recent / scale persist,
// host automation is followed, DRAG WAV renders a valid file
int uiTests()
{
    uiSetup ("uitest");
    VaultEditor::simulatedDisplay() = { 0, 0, 1920, 1040 };   // 1920 x 1080 with a taskbar (xvfb's own screen is 1280 x 1024)
    VaultProcessor p;
    const auto& bank = p.bank();
    int fav = -1, pads0 = -1;
    {
        VaultEditor ed (p);
        auto& v = ed.view();
        check (ed.getWidth() == 1180 && ed.getHeight() == 760,
               "ui: first-open size " + juce::String (ed.getWidth()) + "x" + juce::String (ed.getHeight()) + " on 1920x1080");
        v.setCategory (vault::catPads);
        const auto rows = v.browser().visible();
        check (! rows.empty() && v.visibleCount() == (int) std::count_if (bank.begin(), bank.end(), [] (const VaultPreset& x) { return x.category == vault::catPads; }),
               "ui: category filter (" + juce::String (v.visibleCount()) + " pads)");
        pads0 = rows.front();
        v.loadPreset (pads0, true);
        check (p.currentPreset() == pads0 && v.shownPreset() == pads0, "ui: click loads the sound");
        v.step (1, false);
        check (p.currentPreset() == rows[1], "ui: step loads the next sound of the filtered list");
        v.step (-1, false); v.step (-1, false);
        check (p.currentPreset() == pads0, "ui: step back, clamped at the top");
        v.step (-1, true);
        check (p.currentPreset() == rows.back(), "ui: the previous-sound arrow wraps to the end");
        v.setLayers (2);
        // a search word taken from the loaded bank (works with the shipped bank and the demo bank)
        juce::String word ("stack");
        for (const auto& x : bank)
            if (x.category == vault::catPads && x.parts.size() == 2)
            {
                word = x.name.upToFirstOccurrenceOf (" ", false, false).toLowerCase();
                break;
            }
        v.setQuery (word);
        bool allOk = v.visibleCount() > 0;
        for (int i : v.browser().visible()) allOk &= bank[(size_t) i].parts.size() == 2 && bank[(size_t) i].category == vault::catPads;
        check (allOk, "ui: search + layers chip + category combine (" + juce::String (v.visibleCount()) + ")");
        v.setQuery ({}); v.setLayers (0); v.setCategory (-1);
        fav = rows[2];
        v.toggleFavorite (fav);
        v.setFavoritesOnly (true);
        check (v.visibleCount() == 1 && v.browser().visible().front() == fav, "ui: favorites filter");
        v.setFavoritesOnly (false);
        v.loadPreset (pads0, false);
        p.selectPreset (7);                           // host / automation side
        v.syncFromProcessor();
        check (v.shownPreset() == 7, "ui: follows a preset change from the host");
        if (! p.libraryFound() && bank[7].usesSamples())   // without the library: drag a sound that needs no samples
            for (int i = 0; i < (int) bank.size(); ++i)
                if (! bank[(size_t) i].usesSamples()) { p.selectPreset (i); v.syncFromProcessor(); break; }
        waitUntilPlayable (p);                        // DRAG WAV waits for the samples (never decodes on the UI thread)
        const auto wavFile = v.renderWavForDrag();
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> rd (fm.createReaderFor (wavFile));
        float pk = 0.0f;
        if (rd != nullptr) { juce::AudioBuffer<float> b (2, (int) rd->lengthInSamples); rd->read (&b, 0, b.getNumSamples(), 0, true, true); pk = b.getMagnitude (0, b.getNumSamples()); }
        check (rd != nullptr && rd->numChannels == 2 && rd->lengthInSamples == 88200 && pk > 0.01f && pk < 1.0f,
               "ui: DRAG WAV file " + wavFile.getFileName() + " (peak " + juce::String (pk, 3) + ")");
        ed.setUiScale (1.3f);
        check (ed.getWidth() == 1534 && ed.getHeight() == 988, "ui: scale 130 %");
        ed.setUiScale (3.0f);   // on this 1920 x 1040 screen: the largest size that fits (never larger than the screen)
        check (ed.getWidth() <= 1920 && ed.getHeight() <= 1040 && ed.getWidth() >= 1534, "ui: scale clamps to the screen ("
               + juce::String (ed.getWidth()) + " x " + juce::String (ed.getHeight()) + ")");
        {
            const auto keepArea = VaultEditor::simulatedDisplay();
            VaultEditor::simulatedDisplay() = { 0, 0, 3840, 2120 };
            ed.setUiScale (3.0f);
            check (ed.getWidth() == 2360 && ed.getHeight() == 1520, "ui: scale clamps at 200 % (4K screen)");
            VaultEditor::simulatedDisplay() = keepArea;
            ed.checkDisplay();
        }
        ed.setUiScale (0.5f);
        check (ed.getWidth() == 944 && ed.getHeight() == 608, "ui: scale clamps at 80 % (944x608)");
        ed.setUiScale (1.3f);
    }
    {
        VaultEditor ed (p);   // reopened: favorites, recent and size are remembered
        auto& v = ed.view();
        check (v.browser().isFavorite (fav) && v.browser().numFavorites() == 1, "ui: favorites persist");
        check (! v.browser().recentList().empty() && v.browser().recentList().front() == pads0, "ui: recent persists (newest first)");
        check (ed.getWidth() == 1534, "ui: window size persists");
        const auto f = VaultSettings::fileOverride();
        check (f.existsAsFile() && f.loadFileAsString().contains (bank[(size_t) fav].id), "ui: settings file " + f.getFileName());
    }
    focusTests (p);
    v3PageTests (p);
    ownerFixKeyboardTests();
    ownerFixPageTests();
    ownerFixValueTests();
    ownerFixRenderTests();
    ownerFixMoreTests();
    windowSizeTests (p);
    readabilityTests (p);
    VaultEditor::simulatedDisplay() = { 0, 0, 3840, 2120 };
    VaultSettings::get()->setScale (1.0f);
    uiQaTests (p);
    runUiV2Children();
    return fails;
}

//==============================================================================
// v2: sampler, sample library, model registry, CVB v1 compatibility
//==============================================================================
// YIN pitch (median of frames) of a mono mix, Hz
float yinPitch (const juce::AudioBuffer<float>& b, int from, int len, double sr, double fmin = 40.0, double fmax = 2000.0)
{
    std::vector<float> x ((size_t) b.getNumSamples());
    for (int i = 0; i < b.getNumSamples(); ++i) { float s = 0; for (int c = 0; c < b.getNumChannels(); ++c) s += b.getSample (c, i); x[(size_t) i] = s; }
    const int w = 2048, tauMax = std::min (w - 2, (int) (sr / fmin)), tauMin = std::max (2, (int) (sr / fmax));
    std::vector<float> f0s;
    for (int s0 = from; s0 + w + tauMax < std::min ((int) x.size(), from + len); s0 += w / 2)
    {
        std::vector<double> d ((size_t) tauMax + 2, 0.0);
        for (int tau = 1; tau <= tauMax + 1; ++tau)
        {
            double acc = 0;
            for (int j = 0; j < w; ++j) { const double e = (double) x[(size_t) (s0 + j)] - x[(size_t) (s0 + j + tau)]; acc += e * e; }
            d[(size_t) tau] = acc;
        }
        double run = 0; std::vector<double> cm ((size_t) tauMax + 2, 1.0);
        for (int tau = 1; tau <= tauMax + 1; ++tau) { run += d[(size_t) tau]; cm[(size_t) tau] = run > 0 ? d[(size_t) tau] * tau / run : 1.0; }
        int best = -1;
        for (int tau = tauMin; tau <= tauMax; ++tau)
            if (cm[(size_t) tau] < 0.12) { while (tau < tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; best = tau; break; }
        if (best <= 1 || best >= tauMax) continue;
        const double a = cm[(size_t) best - 1], bb = cm[(size_t) best], c = cm[(size_t) best + 1], den = a - 2 * bb + c;
        const double t = std::abs (den) > 1e-12 ? best + 0.5 * (a - c) / den : best;
        f0s.push_back ((float) (sr / t));
    }
    if (f0s.empty()) return 0.0f;
    std::sort (f0s.begin(), f0s.end());
    return f0s[f0s.size() / 2];
}
float centsOff (float f, float target) { return f > 0 ? 1200.0f * std::log2 (f / target) : 9999.0f; }

// writes a small test library into `dir`: "t.sine" (sine zones at MIDI 57 / 64, 2 velocity layers, 2 round robins on the
// soft layer, sustain loop), "t.click" (one-shot, OGG) and "t.missing" (a zone whose file does not exist)
void writeTestLibrary (const juce::File& dir)
{
    dir.deleteRecursively();
    const auto audio = dir.getChildFile ("audio/t");
    audio.createDirectory();
    const double sr = 44100.0;
    auto writeFile = [&] (const juce::String& name, double hz, float amp, double secs, bool ogg, float decay)
    {
        juce::AudioBuffer<float> b (2, (int) (secs * sr));
        for (int i = 0; i < b.getNumSamples(); ++i)
        {
            const float env = std::exp (-decay * (float) i / (float) sr) * std::min (1.0f, (float) i / 64.0f);
            const float v = amp * env * (float) std::sin (juce::MathConstants<double>::twoPi * hz * i / sr);
            b.setSample (0, i, v); b.setSample (1, i, v * 0.9f);
        }
        const auto f = audio.getChildFile (name);
        std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
        std::unique_ptr<juce::AudioFormat> fmt;
        if (ogg) fmt = std::make_unique<juce::OggVorbisAudioFormat>(); else fmt = std::make_unique<juce::WavAudioFormat>();
        auto w = fmt->createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (sr).withNumChannels (2).withBitsPerSample (ogg ? 16 : 24)
                                               .withQualityOptionIndex (ogg ? 6 : 0));
        if (w != nullptr) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    };
    const double a3 = 220.0, e4 = 329.6275569;
    writeFile ("sine57_soft_1.wav", a3, 0.25f, 3.0, false, 0.0f);
    writeFile ("sine57_soft_2.wav", a3, 0.24f, 3.0, false, 0.0f);
    writeFile ("sine57_loud.wav", a3, 0.6f, 3.0, false, 0.0f);
    writeFile ("sine64_soft.wav", e4, 0.25f, 3.0, false, 0.0f);
    writeFile ("sine64_loud.wav", e4, 0.6f, 3.0, false, 0.0f);
    writeFile ("click.ogg", 523.25, 0.5f, 0.6, true, 6.0f);
    // the loop is deliberately NOT a whole number of periods (a raw jump would click): 1.0 s .. 2.3 s, 100 ms crossfade
    const juce::String loop = "\"loopStart\": 44100, \"loopEnd\": 101430, \"xfade\": 4410";
    const juce::String json = juce::String (R"({ "format": "CHROME VAULT library", "version": 1, "instruments": [
      { "id": "t.sine", "name": "Test Sine", "family": "Test", "articulations": [
        { "name": "sustain", "release": 0.25, "zones": [
          { "file": "t/sine57_soft_1.wav", "root": 57, "lo": 0, "hi": 60, "vlo": 1, "vhi": 63, "rr": 0, LOOP },
          { "file": "t/sine57_soft_2.wav", "root": 57, "lo": 0, "hi": 60, "vlo": 1, "vhi": 63, "rr": 1, LOOP },
          { "file": "t/sine57_loud.wav", "root": 57, "lo": 0, "hi": 60, "vlo": 64, "vhi": 127, LOOP },
          { "file": "t/sine64_soft.wav", "root": 64, "lo": 61, "hi": 127, "vlo": 1, "vhi": 63, LOOP },
          { "file": "t/sine64_loud.wav", "root": 64, "lo": 61, "hi": 127, "vlo": 64, "vhi": 127, LOOP } ] },
        { "name": "short", "oneShot": true, "zones": [ { "file": "t/click.ogg", "root": 72 } ] } ] },
      { "id": "t.missing", "name": "Missing", "family": "Test", "articulations": [
        { "name": "x", "zones": [ { "file": "t/nope.ogg", "root": 60 } ] } ] } ] })").replace ("LOOP", loop);
    dir.getChildFile ("library.json").replaceWithText (json);
}

VaultPreset samplePreset (const juce::String& inst, const juce::String& art, std::vector<std::pair<juce::String, float>> vals = {})
{
    VaultPreset p;
    p.id = "t.smp." + inst + "." + art; p.name = "Sample " + art; p.category = vault::catPiano;
    VaultPart part;
    part.kind = VaultPartKind::sample; part.source = inst; part.articulation = art; part.label = "Test";
    part.values = vals;
    p.parts.push_back (part);
    return p;
}

// v1 bank writer (the old format, for the compatibility test)
juce::MemoryBlock writeV1Bank (const std::vector<VaultPreset>& bank, const std::vector<int>& v1Cats)
{
    juce::StringArray keys;
    for (int k = 0; k < kk::vk::count; ++k) keys.add (kk::vk::info (k).id);
    juce::MemoryOutputStream payload;
    payload.writeShort ((short) keys.size());
    for (auto& k : keys) payload.writeString (k);
    for (size_t i = 0; i < bank.size(); ++i)
    {
        const auto& p = bank[i];
        payload.writeString (p.id); payload.writeString (p.name);
        payload.writeByte ((char) v1Cats[i]);
        payload.writeInt (0x11); payload.writeInt (0x22);   // style / character tags
        payload.writeFloat (p.gain);
        for (float m : p.macroDefaults) payload.writeFloat (m);
        payload.writeByte ((char) p.parts.size());
        for (auto& part : p.parts)
        {
            payload.writeString (part.label); payload.writeFloat (part.level); payload.writeFloat (part.pan);
            payload.writeByte ((char) part.octave); payload.writeByte ((char) part.semi);
            payload.writeShort ((short) part.values.size());
            for (auto& [id, v] : part.values) { payload.writeByte ((char) keys.indexOf (id)); payload.writeFloat (v); }
        }
    }
    juce::MemoryOutputStream out;
    out.write ("CVB1", 4); out.writeShort (1); out.writeShort (0); out.writeInt ((int) bank.size()); out.writeInt ((int) payload.getDataSize());
    out.write (payload.getData(), payload.getDataSize());
    return out.getMemoryBlock();
}

int samplerTests()
{
    const double sr = 44100.0;
    const auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("cv-test-library");
    writeTestLibrary (dir);
    auto lib = std::make_shared<vault::SampleLibrary> (dir);
    check (lib->libraryFound() && lib->numInstruments() == 2 && lib->find ("t.sine") != nullptr, "library: test library.json parsed (" + lib->errorMessage() + ")");
    auto* sine = lib->find ("t.sine");
    check (lib->loadNow (sine) && sine->isLoaded() && sine->data()->articulations.size() == 2, "library: instrument decoded into RAM ("
                                                                                                   + juce::String ((int) (lib->memoryUsed() / 1024)) + " KB)");

    // ---- zone selection: key range, velocity layer, round robin rotation, release / missing
    {
        const auto* d = sine->data();
        const auto* zSoft = d->pick (0, 57, 40, 0, false);
        const auto* zSoft2 = d->pick (0, 57, 40, 1, false);
        const auto* zSoft3 = d->pick (0, 57, 40, 2, false);
        const auto* zLoud = d->pick (0, 55, 100, 0, false);
        const auto* zHigh = d->pick (0, 70, 100, 0, false);
        check (zSoft != nullptr && zSoft->root == 57 && zSoft->vhi == 63 && zLoud != nullptr && zLoud->vlo == 64 && zHigh != nullptr && zHigh->root == 64,
               "sampler: zone selection by key range and velocity layer");
        check (zSoft != zSoft2 && zSoft3 == zSoft && zSoft2 != nullptr && zSoft2->rr == 1, "sampler: round robin rotates through the cell (0, 1, 0)");
        check (d->pick (0, 70, 100, 0, true) == nullptr && d->pick (5, 60, 100, 0, false) == nullptr, "sampler: no release zone / bad articulation -> nothing");
        int switches = 0; const vault::SampleInstrumentData::Zone* last = nullptr;
        for (unsigned k = 0; k < 6; ++k) { const auto* z = d->pick (0, 57, 30, k, false); switches += (z != last) ? 1 : 0; last = z; }
        check (switches == 6, "sampler: repeated soft notes alternate their round-robin takes");
    }

    // ---- pitch at C5 (MIDI 60) from a root of 57 (+3 semitones) and from 64 (-4) within +-5 cents
    {
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        const auto b = VaultEngine::renderMidi (samplePreset ("t.sine", "sustain"), m, (int) (1.5 * sr), sr, nullptr, lib);
        const float f = yinPitch (b, (int) (0.2 * sr), (int) (1.0 * sr), sr);
        check (std::abs (centsOff (f, 261.6256f)) < 5.0f, "sampler: C5 from a 3-semitone-lower zone plays " + juce::String (f, 2) + " Hz ("
                                                          + juce::String (centsOff (f, 261.6256f), 2) + " cents)");
        juce::MidiBuffer m2;
        m2.addEvent (juce::MidiMessage::noteOn (1, 61, (juce::uint8) 100), 0);
        const auto b2 = VaultEngine::renderMidi (samplePreset ("t.sine", "sustain"), m2, (int) (1.5 * sr), sr, nullptr, lib);
        const float f2 = yinPitch (b2, (int) (0.2 * sr), (int) (1.0 * sr), sr), want2 = 277.1826f;
        check (std::abs (centsOff (f2, want2)) < 5.0f, "sampler: C#5 from a zone 3 semitones higher plays " + juce::String (f2, 2) + " Hz ("
                                                        + juce::String (centsOff (f2, want2), 2) + " cents)");
        // 48 kHz host: the sample-rate ratio is part of the pitch
        const auto b3 = VaultEngine::renderMidi (samplePreset ("t.sine", "sustain"), m, (int) (1.5 * 48000.0), 48000.0, nullptr, lib);
        const float f3 = yinPitch (b3, 9600, 48000, 48000.0);
        check (std::abs (centsOff (f3, 261.6256f)) < 5.0f, "sampler: in tune at 48 kHz (" + juce::String (centsOff (f3, 261.6256f), 2) + " cents)");
    }

    // ---- sustain loop: a held note passes the (imperfect) loop point many times without a click; release fades
    {
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 57, (juce::uint8) 100), 0);
        const int offAt = (int) (6.0 * sr);
        m.addEvent (juce::MidiMessage::noteOff (1, 57), offAt);
        const auto b = VaultEngine::renderMidi (samplePreset ("t.sine", "sustain", { { "s.width", 1.0f } }), m, (int) (7.0 * sr), sr, nullptr, lib);
        // root note = no pitch change: the loop wraps at 101430 - 44100 = 57330 samples
        const float steady = stats (b, (int) (0.5 * sr), (int) (0.9 * sr)).maxJump;
        float worst = 0;
        for (int k = 0; k < 4; ++k)
        {
            const int at = 101430 + k * 57330;
            worst = std::max (worst, stats (b, at - 6000, at + 600).maxJump);
        }
        check (worst <= steady * 1.08f + 1e-4f, "sampler: loop crossfade is seamless (max jump " + juce::String (worst, 5) + " vs steady " + juce::String (steady, 5) + ")");
        const float rmsHeld = stats (b, (int) (5.0 * sr), (int) (5.5 * sr)).rms;
        check (rmsHeld > 0.5f * stats (b, (int) (0.5 * sr), (int) (1.0 * sr)).rms, "sampler: the looped note keeps sustaining (5 s)");
        const float afterRel = stats (b, offAt + (int) (0.30 * sr), offAt + (int) (0.35 * sr)).peak;
        check (afterRel < rmsHeld * 0.01f && stats (b, offAt - 64, offAt + 512).maxJump <= steady * 1.1f + 1e-4f,
               "sampler: release fades out smoothly (-60 dB within the 0.25 s release, no click at note-off: after " + juce::String (afterRel, 6)
                   + " held " + juce::String (rmsHeld, 4) + " jump " + juce::String (stats (b, offAt - 64, offAt + 512).maxJump, 5) + ")");
    }

    // ---- one-shot (OGG): plays to its end, ignores note-off; the missing instrument fails cleanly and stays silent
    {
        juce::MidiBuffer m;
        m.addEvent (juce::MidiMessage::noteOn (1, 72, (juce::uint8) 100), 0);
        m.addEvent (juce::MidiMessage::noteOff (1, 72), 200);
        const auto b = VaultEngine::renderMidi (samplePreset ("t.sine", "short"), m, (int) (0.5 * sr), sr, nullptr, lib);
        check (stats (b, (int) (0.2 * sr), (int) (0.25 * sr)).rms > 1e-3f, "sampler: one-shot ignores the early note-off");
        const float f = yinPitch (b, 1000, (int) (0.3 * sr), sr);
        check (std::abs (centsOff (f, 523.25f)) < 5.0f, "sampler: OGG one-shot at its root " + juce::String (f, 2) + " Hz");
        auto* miss = lib->find ("t.missing");
        check (! lib->loadNow (miss) && miss->hasFailed() && ! miss->isLoading(), "library: an instrument with a missing file fails (no retry loop)");
        juce::MidiBuffer m2;
        m2.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        const auto s = stats (VaultEngine::renderMidi (samplePreset ("t.missing", "x"), m2, (int) (0.3 * sr), sr, nullptr, lib));
        const auto u = stats (VaultEngine::renderMidi (samplePreset ("no.such.id", ""), m2, (int) (0.3 * sr), sr, nullptr, lib));
        check (s.finite && s.peak == 0.0f && u.finite && u.peak == 0.0f, "library: missing / unknown instruments are silent, no crash");
        vault::SampleLibrary none (juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("cv-no-library-here"));
        check (! none.libraryFound() && none.numInstruments() == 0 && none.find ("t.sine") == nullptr, "library: missing library folder -> libraryFound() false");
        check (! vault::SampleLibrary::searchLocations().isEmpty(), "library: search locations (" + vault::SampleLibrary::searchLocations().getLast().getFullPathName() + ")");
    }

    // ---- loading on the audio side: silent while loading, plays once loaded; LRU eviction never touches what is in use
    {
        auto lib2 = std::make_shared<vault::SampleLibrary> (dir);
        const auto c = VaultEngine::compile (samplePreset ("t.sine", "sustain"), lib2);
        VaultEngine eng;
        eng.prepare (sr, 256);
        eng.setPreset (c, true);
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer on; on.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0);
        juce::MidiBuffer none;
        eng.process (buf, on);
        const bool wasLoading = eng.isLoading();
        lib2->waitUntilIdle (10000);
        for (int k = 0; k < 4; ++k) eng.process (buf, none);
        const bool loadedNow = ! eng.isLoading() && c.parts[0].instrument->isLoaded();
        eng.process (buf, on);   // a new note after loading plays
        for (int k = 0; k < 20; ++k) eng.process (buf, none);
        check (wasLoading && loadedNow && stats (buf).rms > 1e-3f, "engine: part silent while loading (isLoading), plays once the loader is done");
        lib2->setMemoryBudget (1);   // below anything: only unused instruments may go
        lib2->waitUntilIdle (2000);
        juce::Thread::sleep (100);
        check (c.parts[0].instrument->isLoaded(), "library: an instrument in use survives a memory-budget squeeze");
        eng.releaseResources();
        juce::Thread::sleep (100);
        bool evicted = false;
        for (int k = 0; k < 50 && ! evicted; ++k) { juce::Thread::sleep (20); evicted = ! c.parts[0].instrument->isLoaded(); }
        check (evicted && lib2->memoryUsed() == 0, "library: unused instruments are evicted when over budget (LRU)");
    }

    // ---- the audio thread never allocates: notes, sustain, preset switches between sample / synth / model presets
    {
        const auto cs = VaultEngine::compile (samplePreset ("t.sine", "sustain", { { "s.cutoff", 3000.0f }, { "s.vibrato", 10.0f } }), lib);
        const auto csyn = VaultEngine::compile (testPresets()[0]);
        VaultPreset mp; mp.id = "t.model"; mp.name = "Model"; mp.category = vault::catSynth;
        { VaultPart part; part.kind = VaultPartKind::model; part.source = "sine"; part.label = "Sine"; mp.parts.push_back (part); }
        const auto cm = VaultEngine::compile (mp);
        VaultEngine eng;
        eng.prepare (sr, 512);
        eng.setPreset (cs, true);
        eng.setPreset (cm, true);
        eng.serviceResources();     // the model pool is made off the audio thread (as the processor's timer does)
        eng.setPreset (cs, true);
        juce::AudioBuffer<float> buf (2, 512);
        allocHook::allocs = 0;
        allocHook::countAllocs = true;
        for (int k = 0; k < 400; ++k)
        {
            juce::MidiBuffer m;
            if (k % 7 == 0) m.addEvent (juce::MidiMessage::noteOn (1, 48 + (k % 24), (juce::uint8) (40 + k % 80)), 3);
            if (k % 7 == 4) m.addEvent (juce::MidiMessage::noteOff (1, 48 + ((k - 4) % 24)), 100);
            if (k == 50) m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), 0);
            if (k == 90) m.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), 0);
            if (k % 13 == 0) m.addEvent (juce::MidiMessage::pitchWheel (1, 8192 + (k % 5) * 800), 10);
            if (k == 120) eng.setPreset (csyn);
            if (k == 200) eng.setPreset (cm);
            if (k == 280) eng.setPreset (cs);
            if (k % 10 == 0) eng.setMacro (k / 10 % 4, (float) (k % 100) / 100.0f);
            eng.process (buf, m);
        }
        allocHook::countAllocs = false;
        check (allocHook::allocs.load() == 0, "engine: no allocation on the audio thread over 400 blocks (notes, pedal, bend, macros, sample / synth / model switches: "
                                                 + juce::String (allocHook::allocs.load()) + " allocations)");
    }

    // ---- CPU: 48 held sampler voices (pitched, filtered, looped) for 5 s
    {
        juce::MidiBuffer m;
        for (int k = 0; k < 48; ++k) m.addEvent (juce::MidiMessage::noteOn (1, 40 + k, (juce::uint8) 100), k);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        const auto b = VaultEngine::renderMidi (samplePreset ("t.sine", "sustain", { { "s.cutoff", 5000.0f } }), m, (int) (5.0 * sr), sr, nullptr, lib);
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        check (stats (b).finite && ms < 5000.0, "CPU: 48 sampler voices for 5 s render in " + juce::String (ms, 0) + " ms ("
                                                    + juce::String (ms / 5000.0 * 100.0, 1) + " % of one core, sinc interpolation + SVF per voice)");
    }

    // ---- model registry: the "sine" example model plays in tune, unknown ids are silent
    {
        auto& reg = vault::models::ModelRegistry::get();
        check (reg.indexOf ("sine") >= 0 && reg.find ("sine")->params.size() == 3 && reg.indexOf ("nope") < 0, "models: registry lists " + reg.ids().joinIntoString (", "));
        VaultPreset mp; mp.id = "t.model"; mp.name = "Model"; mp.category = vault::catSynth;
        VaultPart part; part.kind = VaultPartKind::model; part.source = "sine"; part.label = "Sine"; part.values = { { "m.level", 0.3f } };
        mp.parts.push_back (part);
        juce::String why;
        check (vault::validatePreset (mp, &why), "models: a model preset validates " + why);
        const auto b = VaultEngine::render (mp, vault::homeNote, 0.8f, 1.5, sr);
        const float f = yinPitch (b, (int) (0.1 * sr), (int) (0.8 * sr), sr);
        check (std::abs (centsOff (f, 261.6256f)) < 5.0f && stats (b).peak < 0.99f, "models: sine model at C5 = " + juce::String (f, 2) + " Hz");
        mp.parts[0].source = "no.such.model";
        check (stats (VaultEngine::render (mp, 60, 0.8f, 0.5, sr)).peak == 0.0f, "models: an unknown model id is silent");
    }

    // ---- CVB: v2 round trip with all part kinds, v1 files are read (tags dropped, categories mapped, no "808")
    {
        std::vector<VaultPreset> all = testPresets();
        auto sp = samplePreset ("piano.grand.concert", "sustain", { { "s.release", 0.8f }, { "s.cutoff", 9000.0f }, { "revMix", 0.2f } });
        sp.parts.push_back (testPresets()[0].parts[0]);
        VaultPart mpart; mpart.kind = VaultPartKind::model; mpart.source = "sine"; mpart.label = "Body"; mpart.values = { { "m.decay", 2.5f }, { "m.custom", 7.0f } };
        sp.parts.push_back (mpart);
        all.push_back (sp);
        juce::String why;
        check (vault::validatePreset (sp, &why), "cvb: a 3-kind preset validates " + why);
        auto bad = sp; bad.parts[0].values.push_back ({ "s.unknownKnob", 1.0f });
        check (! vault::validatePreset (bad), "cvb: an unknown sample parameter is refused");
        const auto mb = vault::serializeBank (all, true);
        std::vector<VaultPreset> back; int ver = 0;
        auto allV3 = all; for (auto& p : allV3) vault::upgradeToV3 (p);
        check (vault::deserializeBank (mb.getData(), mb.getSize(), back, nullptr, &ver) && back == allV3 && ver == vault::bankFormatVersion, "cvb: v3 round trip (sample / model / synth parts)");
        std::vector<VaultPreset> v1src = { preset ("cv.808.0001", "Glide 808 Monster", 0, { part ({ { ID::engine, 6 } }, 1, 0, 0, "Sub 808") }),
                                           preset ("cv.keys.0001", "Felt Dream", 0, { part ({ { ID::engine, 0 } }, 1, 0, 0, "Felt Piano") }),
                                           preset ("cv.keys.0002", "Organ Church", 0, { part ({ { ID::engine, 4 } }, 1, 0, 0, "Organ") }),
                                           preset ("cv.arp.0001", "Seq Thing", 0, { part ({ { ID::engine, 0 } }, 1, 0, 0, "Arp") }) };
        const auto v1 = writeV1Bank (v1src, { vault::v1::cat808Bass, vault::v1::catPianoKeys, vault::v1::catPianoKeys, vault::v1::catArpsSequences });
        std::vector<VaultPreset> rd; ver = 0;
        const bool ok = vault::deserializeBank (v1.getData(), v1.getSize(), rd, nullptr, &ver);
        check (ok && ver == 1 && rd.size() == 4 && rd[0].category == vault::catBass && rd[1].category == vault::catPiano && rd[2].category == vault::catKeysOrgan
                   && rd[3].category == vault::catSynth && rd[0].name == "Glide Sub Monster" && rd[0].parts[0].label == "Sub" && rd[0].parts[0].kind == VaultPartKind::synth,
               "cvb: v1 file read (categories mapped, '808' removed: '" + (rd.empty() ? juce::String() : rd[0].name) + "')");
       #if CV_HAS_BANK
        int embeddedVer = 0;
        for (int i = 0; i < VaultData::namedResourceListSize; ++i)
        {
            int size = 0;
            if (const auto* data = VaultData::getNamedResource (VaultData::namedResourceList[i], size))
            {
                std::vector<VaultPreset> eb;
                if (vault::deserializeBank (data, (size_t) size, eb, nullptr, &embeddedVer)) { check (! eb.empty(), "cvb: embedded bank read (format v" + juce::String (embeddedVer) + ", " + juce::String ((int) eb.size()) + " presets)"); break; }
            }
        }
       #endif
    }
    dir.deleteRecursively();
    return fails;
}

//==============================================================================
// the real library (CHROME_VAULT_LIBRARY / installed): every instrument renders clean and in tune at C5
float bandShare (const juce::AudioBuffer<float>& b, int from, int len, double sr, double lo, double hi, float* fundDb, float* strongestDb, double f0)
{
    const int order = 15, n = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> data ((size_t) n * 2, 0.0f);
    for (int i = 0; i < n && from + i < b.getNumSamples() && i < len; ++i)
    {
        float s = 0; for (int c = 0; c < b.getNumChannels(); ++c) s += b.getSample (c, from + i);
        data[(size_t) i] = s * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * i / (float) std::min (n, len)));
    }
    fft.performFrequencyOnlyForwardTransform (data.data());
    double tot = 0, band = 0;
    for (int k = 1; k < n / 2; ++k)
    {
        const double f = k * sr / n, p = (double) data[(size_t) k] * data[(size_t) k];
        tot += p; if (f >= lo && f < hi) band += p;
    }
    if (fundDb != nullptr && strongestDb != nullptr && f0 > 0)
    {
        auto peakNear = [&] (double hz) { float m = 0; const int k0 = (int) (hz * 0.97 * n / sr), k1 = (int) (hz * 1.03 * n / sr) + 1;
                                          for (int k = std::max (1, k0); k <= std::min (n / 2 - 1, k1); ++k) m = std::max (m, data[(size_t) k]); return m; };
        const float fund = peakNear (f0);
        float strongest = fund;
        for (int h = 2; h <= 16 && h * f0 < sr / 2; ++h) strongest = std::max (strongest, peakNear (h * f0));
        *fundDb = juce::Decibels::gainToDecibels (fund + 1e-9f);
        *strongestDb = juce::Decibels::gainToDecibels (strongest + 1e-9f);
    }
    return (float) (band / std::max (1e-20, tot));
}

// the frequency of the strongest spectral peak within +-0.6 semitone of `expected` (Hann window, 64k FFT, parabolic
// interpolation of the log magnitude) - the pitch of struck bars / tines, whose upper partials are inharmonic
float fundamentalPeak (const juce::AudioBuffer<float>& b, int from, int len, double sr, double expected)
{
    const int order = 16, n = 1 << order;
    len = std::min ({ len, n, b.getNumSamples() - from });
    if (len < 2048) return 0.0f;
    std::vector<float> data ((size_t) n * 2, 0.0f);
    for (int i = 0; i < len; ++i)
    {
        float s = 0; for (int c = 0; c < b.getNumChannels(); ++c) s += b.getSample (c, from + i);
        data[(size_t) i] = s * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) len));
    }
    juce::dsp::FFT fft (order);
    fft.performFrequencyOnlyForwardTransform (data.data());
    const int k0 = std::max (2, (int) (expected * std::pow (2.0, -0.6 / 12.0) * n / sr)), k1 = std::min (n / 2 - 2, (int) (expected * std::pow (2.0, 0.6 / 12.0) * n / sr) + 1);
    int kb = k0;
    for (int k = k0; k <= k1; ++k) if (data[(size_t) k] > data[(size_t) kb]) kb = k;
    if (kb <= k0 || kb >= k1) return 0.0f;   // no peak inside the window
    const double a = std::log (data[(size_t) kb - 1] + 1e-12), c0 = std::log (data[(size_t) kb] + 1e-12), c = std::log (data[(size_t) kb + 1] + 1e-12);
    const double den = a - 2 * c0 + c;
    return (float) ((kb + (std::abs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0)) * sr / n);
}

int libraryReport (const juce::File& outDir, const std::vector<int>& notes, bool asTest)
{
    auto lib = vault::SampleLibrary::getShared();
    if (! lib->libraryFound())
    {
        std::printf ("no sample library (%s) - searched: %s\n", lib->errorMessage().toRawUTF8(),
                     vault::SampleLibrary::searchLocations().getFirst().getFullPathName().toRawUTF8());
        if (asTest) check (true, "library: none installed - the real-library checks are skipped");
        return 0;
    }
    if (outDir != juce::File()) outDir.createDirectory();
    const double sr = 44100.0;
    check (true, "library: " + lib->libraryPath().getFullPathName() + " (" + juce::String (lib->numInstruments()) + " instruments)");
    // how the builder found each articulation's roots (library.json "pitchCheck": "yin" = verified pitch, "spectrum" = bars /
    // tines tuned on the fundamental's spectral peak, "name" = inharmonic
    // bells / glides with trusted names, "detect" = pitch measured from the recording) - "name" / "detect" get no cents check
    const auto libJson = juce::JSON::parse (lib->libraryPath().getChildFile ("library.json"));
    auto pitchCheckOf = [&] (const juce::String& id, const juce::String& artName) -> juce::String
    {
        if (auto* arr = libJson["instruments"].getArray())
            for (auto& iv : *arr)
                if (iv["id"].toString() == id)
                    if (auto* arts = iv["articulations"].getArray())
                        for (auto& av : *arts)
                            if (av["name"].toString() == artName && av.hasProperty ("pitchCheck")) return av["pitchCheck"].toString();
        return "yin";
    };
    auto pitchFromOf = [&] (const juce::String& id, const juce::String& artName) -> double
    {
        if (auto* arr = libJson["instruments"].getArray())
            for (auto& iv : *arr)
                if (iv["id"].toString() == id)
                    if (auto* arts = iv["articulations"].getArray())
                        for (auto& av : *arts)
                            if (av["name"].toString() == artName && av.hasProperty ("pitchFrom")) return juce::jlimit (0.05, 1.0, (double) av["pitchFrom"]);
        return 0.1;
    };
    // the key nearest to `want` that the articulation plays (C5 is outside the range of e.g. tuba, piccolo, glockenspiel)
    auto playable = [] (const vault::SampleArticulationDesc& a, int want)
    {
        int best = want, bestD = 1000;
        for (auto& z : a.zones)
            if (! z.releaseTrigger)
                for (int k = z.lo; k <= z.hi; ++k) if (std::abs (k - want) < bestD) { bestD = std::abs (k - want); best = k; }
        return best;
    };
    for (int i = 0; i < lib->numInstruments(); ++i)
    {
        auto* h = lib->instrument (i);
        const auto t0 = juce::Time::getMillisecondCounterHiRes();
        const bool loaded = lib->loadNow (h);
        const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
        check (loaded, "library: " + h->desc.id + " loads (" + juce::String ((int) (h->data() != nullptr ? h->data()->bytes / 1024 / 1024 : 0)) + " MB in RAM, "
                           + juce::String (ms, 0) + " ms)");
        for (auto& art : h->desc.articulations)
            for (int wanted : notes)
            {
                const int note = playable (art, wanted);
                const bool sustained = ! art.oneShot;
                const double secs = sustained ? 24.0 : 3.0;   // long enough for several loop wraps
                juce::MidiBuffer m;
                m.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 100), 0);
                m.addEvent (juce::MidiMessage::noteOff (1, note), (int) ((secs - 1.0) * sr));
                const auto b = VaultEngine::renderMidi (samplePreset (h->desc.id, art.name), m, (int) (secs * sr), sr, nullptr, lib);
                const auto st = stats (b);
                const double f0 = 440.0 * std::pow (2.0, (note - 69) / 12.0);
                const int a0 = (int) (0.15 * sr);
                // pitch: a looped note over 0.35-2.35 s (what the ear averages: attack drift + the loop); a decaying one where
                // the builder measured it (library.json "pitchFrom": 0.5 s for pianos - their attack glides sharp -, else
                // right after the attack, before plucks and bars glide / decay)
                bool looped = false;
                for (auto& z : art.zones) looped |= z.looped();
                const double pFrom = pitchFromOf (h->desc.id, art.name);
                const float f = looped ? yinPitch (b, (int) (0.35 * sr), (int) (2.0 * sr), sr, f0 * 0.6, f0 * 1.6)
                                       : yinPitch (b, (int) (pFrom * sr), (int) ((pFrom > 0.2 ? 1.0 : 0.4) * sr), sr, f0 * 0.6, f0 * 1.6);
                float fundDb = 0, strongDb = 0;
                const float presence = bandShare (b, a0, (int) (0.75 * sr), sr, 2000.0, 5000.0, &fundDb, &strongDb, f0);
                // loop wrap clicks: the largest sample jump during the held part vs the first second
                const float early = stats (b, (int) (0.3 * sr), (int) (1.2 * sr)).maxJump, held = stats (b, (int) (1.2 * sr), (int) ((secs - 1.05) * sr)).maxJump;
                const float loud = VaultEngine::measureLoudnessDb (b, sr);
                std::printf ("   %-26s %-9s %-4s  peak %5.1f dB  loud %6.1f dB  pitch %+6.1f c  fund %+5.1f dB vs strongest  2-5 kHz %4.1f %%  jump held/early %.2f\n",
                             h->desc.id.toRawUTF8(), art.name.toRawUTF8(), vault::noteName (note).toRawUTF8(), juce::Decibels::gainToDecibels (st.peak),
                             loud, centsOff (f, (float) f0), fundDb - strongDb, presence * 100.0f, held / std::max (1e-6f, early));
                if (outDir != juce::File())
                    writeWav (b, sr, outDir.getChildFile (h->desc.id + "_" + art.name + "_" + vault::noteName (note).replace ("#", "s") + ".wav"));
                // loop wraps: where the played zone jumps back (from its loop data), the sample jump there vs the music around it
                float wrapRatio = 0.0f, dipDb = 0.0f;
                int wraps = 0;
                if (sustained && h->data() != nullptr)
                    if (const auto* z = h->data()->pick (h->desc.articulationIndex (art.name), note, 100, 0, false); z != nullptr && z->looped)
                    {
                        const double inc = std::exp2 ((note - z->root) / 12.0 + z->fine / 1200.0) * z->audio->sampleRate / sr;
                        const int heldEnd = (int) ((secs - 1.0) * sr);
                        for (double t = (z->loopEnd - z->start) / inc; t < heldEnd - 0.3 * sr; t += (z->loopEnd - z->loopStart) / inc, ++wraps)
                        {
                            const int w = (int) t, ms = (int) (0.001 * sr);
                            const float at = stats (b, w - 3 * ms, w + 3 * ms).maxJump;
                            const float around = std::max (stats (b, w - 250 * ms, w - 20 * ms).maxJump, stats (b, w + 20 * ms, std::min (heldEnd, w + 250 * ms)).maxJump);
                            wrapRatio = std::max (wrapRatio, at / std::max (1e-6f, around));
                            const int xfMid = w - (int) (z->xfade / inc / 2);
                            const float mid = stats (b, xfMid - 10 * ms, xfMid + 10 * ms).rms, out = stats (b, w - (int) (z->xfade / inc) - 60 * ms, w - (int) (z->xfade / inc) - 10 * ms).rms;
                            // minus the level swing the music has on its own just before the crossfade (rubbed glass beats,
                            // tremolo bowing): the quietest 20 ms of the 600 ms before it
                            float natural = 1e9f;
                            const int xfStartAt = w - (int) (z->xfade / inc);
                            for (int s0 = xfStartAt - 600 * ms; s0 + 20 * ms <= xfStartAt - 10 * ms; s0 += 5 * ms)
                                if (s0 > 0) natural = std::min (natural, stats (b, s0, s0 + 20 * ms).rms);
                            const float raw = juce::Decibels::gainToDecibels (mid / std::max (1e-6f, out));
                            const float nat = natural < 1e8f ? juce::Decibels::gainToDecibels (natural / std::max (1e-6f, out)) : 0.0f;
                            dipDb = std::min (dipDb, std::min (0.0f, raw - std::min (0.0f, nat)));
                        }
                        std::printf ("      loop: %d wraps in the held note, jump at the wrap %.2fx the music around it, level in the crossfade %+.1f dB\n", wraps, wrapRatio, dipDb);
                    }
                if (asTest)
                {
                    check (st.finite && st.peak > 0.003f && st.peak < 0.99f, "library: " + h->desc.id + " / " + art.name + " at " + vault::noteName (note) + " plays clean");
                    if (wraps > 0)
                        check (wrapRatio < 1.3f && dipDb > -3.0f, "library: " + h->desc.id + " / " + art.name + " loops without clicks or dips (" + juce::String (wraps)
                                                                  + " wraps, jump " + juce::String (wrapRatio, 2) + "x, crossfade level " + juce::String (dipDb, 1) + " dB)");
                    // in tune: 10 cents for looped notes (vibrato / bowing drift), 15 for decaying ones (plucks glide);
                    // no YIN pitch (missing fundamental, e.g. organ pedal stops) = not measurable here (VaultLibBuild -qc
                    // measures every zone with a spectral second opinion)
                    // struck bars / tines ("spectrum"): the fundamental's spectral peak right after the strike, 15 cents
                    if (wanted == vault::homeNote && pitchCheckOf (h->desc.id, art.name) == "spectrum")
                    {
                        const float fp = fundamentalPeak (b, (int) (0.06 * sr), (int) (0.35 * sr), sr, f0);
                        check (fp > 0.0f && std::abs (centsOff (fp, (float) f0)) < 15.0f, "library: " + h->desc.id + " / " + art.name + " in tune at "
                                   + vault::noteName (note) + " (" + juce::String (fp > 0 ? centsOff (fp, (float) f0) : 9999.0f, 1) + " cents, spectral peak)");
                    }
                    // YIN can be pulled off by inharmonic upper partials (FM keys, bright plucks): the fundamental's spectral
                    // peak in the same stretch is the second opinion - in tune when either one says so
                    if (wanted == vault::homeNote && pitchCheckOf (h->desc.id, art.name) == "yin" && f > 0.0f)
                    {
                        const double from = looped ? 0.35 : pFrom, len = looped ? 2.0 : (pFrom > 0.2 ? 1.0 : 0.4);
                        const float fp = fundamentalPeak (b, (int) (from * sr), (int) (len * sr), sr, f0);
                        const float tol = looped ? 10.0f : 15.0f, cy = centsOff (f, (float) f0), cp = fp > 0 ? centsOff (fp, (float) f0) : 9999.0f;
                        check (std::abs (cy) < tol || std::abs (cp) < tol, "library: " + h->desc.id + " / " + art.name + " in tune at " + vault::noteName (note)
                                                                              + " (YIN " + juce::String (cy, 1) + " cents, spectral peak " + juce::String (cp, 1) + ")");
                    }
                }
            }
    }
    return fails;
}

int shot (const juce::String& outPath, const juce::String& stateArg)
{
    uiSetup ("shot");
    VaultEditor::simulatedDisplay() = { 0, 0, 3840, 2120 };   // a big screen: every scale fits
    VaultSettings::get()->setScale (1.0f);   // shots start at 100 % ("scale:<pct>" changes it)
    VaultProcessor p;
    auto ed = std::make_unique<VaultEditor> (p);
    auto& v = ed->view();
    const auto& bank = p.bank();
    int pick = -1;
    for (int i = 0; i < (int) bank.size() && pick < 0; ++i)
        if (bank[(size_t) i].parts.size() == 3 && bank[(size_t) i].category == vault::catBellsMallets) pick = i;
    for (int i = 0; i < (int) bank.size() && pick < 0; ++i) if (bank[(size_t) i].parts.size() == 3) pick = i;
    if (pick < 0) pick = 0;
    bool wait = true;   // "nowait": paint at once (a sampled sound is still loading)
    for (auto t : juce::StringArray::fromTokens (stateArg, ",", ""))
    {
        if (t.trim() == "nowait") wait = false;
        if (t.trim().startsWith ("pick:"))   // "pick:<preset name>"
            for (int i = 0; i < (int) bank.size(); ++i)
                if (bank[(size_t) i].name.equalsIgnoreCase (t.fromFirstOccurrenceOf (":", false, false).trim())) { pick = i; break; }
        if (t.trim() == "four")   // a 4-layer sound, with a modelled layer when there is one (LAYERS page)
        {
            int best = -1, bestScore = -1;
            for (int i = 0; i < (int) bank.size(); ++i)
            {
                const auto& x = bank[(size_t) i];
                if (x.parts.size() != 4) continue;
                int score = 0;
                for (auto& part : x.parts) score += part.kind == VaultPartKind::model ? 2 : part.kind == VaultPartKind::synth ? 1 : 0;
                if (score > bestScore) { bestScore = score; best = i; }
            }
            if (best >= 0) pick = best;
        }
        if (t.trim() == "rich")   // the 3-layer sound with the longest FX chain (FX / MOD pages)
        {
            size_t most = 0;
            for (int i = 0; i < (int) bank.size(); ++i)
            {
                auto x = bank[(size_t) i];
                vault::upgradeToV3 (x);
                if (x.parts.size() == 3 && x.fx.size() > most) { most = x.fx.size(); pick = i; }
            }
        }
    }
    for (int i = 0; i < 6; ++i)   // some history for RECENT (synth-only sounds: a "nowait" shot must still catch its own loading)
    {
        int k = (pick + 7 * i + 3) % (int) bank.size();
        for (int tries = 0; tries < (int) bank.size() && bank[(size_t) k].usesSamples(); ++tries) k = (k + 1) % (int) bank.size();
        v.loadPreset (k, false);
    }
    v.loadPreset (pick, false);
    const auto& pr = bank[(size_t) pick];

    bool macrosAfterLoad = false;   // "knobs": the sound macros moved
    bool demoEdits = false;         // "demo": modulation, arp / gate lanes and a MAIN knob edited (shows the green rings, lanes)
    int pageAfterLoad = -1;         // "page:main|layers|fx|mod|arp"
    auto tokens = juce::StringArray::fromTokens (stateArg.isEmpty() ? "main" : stateArg, ",", "");
    for (auto t : tokens)
    {
        const auto key = t.upToFirstOccurrenceOf (":", false, false).trim(), val = t.fromFirstOccurrenceOf (":", false, false).trim();
        if (key == "main") v.setCategory (pr.category);
        if (key == "main" || key == "keys") for (int n : { 60, 64, 67 }) p.getKeyboardState().noteOn (1, n, 0.8f);   // (sounding: see below)
        else if (key == "scale")   ed->setUiScale (val.getFloatValue() / 100.0f);
        else if (key == "search")  v.setQuery (val);
        else if (key == "cat")     v.setCategory (val.getIntValue());
        else if (key == "layers")  v.setLayers (val.getIntValue());
        else if (key == "empty")   v.setQuery ("zzqx nothing");
        else if (key == "recent")  v.setRecentOnly (true);
        else if (key == "knobs")   { macrosAfterLoad = true; }
        else if (key == "page")
        {
            const int pg = juce::StringArray { "main", "layers", "fx", "mod", "arp" }.indexOf (val);
            pageAfterLoad = pg;
        }
        else if (key == "demo")    demoEdits = true;
        else if (key == "favs")
        {
            for (int i = 0; i < 9; ++i) v.toggleFavorite ((pick + i * 13) % (int) bank.size());
            v.setFavoritesOnly (true);
        }
    }
    v.loadPreset (pick, false);
    if (wait)
    {
        const auto until = juce::Time::getMillisecondCounter() + 30000;
        while (p.isLoading() && juce::Time::getMillisecondCounter() < until) juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    }
    {   // the held keys sound: the on-screen keyboard lights what really plays
        p.prepareToPlay (44100.0, 512);
        juce::AudioBuffer<float> b (2, 512);
        juce::MidiBuffer m;
        const auto until = juce::Time::getMillisecondCounter() + (wait ? 30000u : 0u);
        for (int i = 0; i < 2; ++i) { b.clear(); m.clear(); p.processBlock (b, m); }
        while (p.isLoading() && juce::Time::getMillisecondCounter() < until)
        {
            juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
            b.clear(); m.clear(); p.processBlock (b, m);
        }
        if (tokens.contains ("main") || tokens.contains ("keys"))   // the C5 major triad held (prepareToPlay reset the keys)
            for (int n : { 60, 64, 67 }) p.getKeyboardState().noteOn (1, n, 0.8f);
        for (int i = 0; i < 4; ++i) { b.clear(); m.clear(); p.processBlock (b, m); }
    }
    v.syncFromProcessor();
    if (demoEdits)
    {
        p.editSound ([] (VaultPreset& x)
        {
            x.mod.lfo[0].dests = { { "pitch", 0.08f }, { "cutoff", 1.0f } };
            x.mod.lfo[0].sync = true; x.mod.lfo[0].div = (uint8_t) vault::divisionIndex ("1/4");
            x.mod.lfo[0].delay = 0.3f; x.mod.lfo[0].fade = 0.5f;
            x.mod.lfo[1].shape = VaultLfo::triangle; x.mod.lfo[1].sync = true; x.mod.lfo[1].div = (uint8_t) vault::divisionIndex ("1/8");
            x.mod.lfo[1].delay = 0.3f; x.mod.lfo[1].fade = 0.5f;
            x.mod.lfo[1].dests = { { "volume", 0.15f }, { "pan", 0.4f } };
            x.mod.modWheel = { { "cutoff", 1.6f }, { "quick.vibrato", 0.3f } };
            x.mod.velocity = { { "quick.attack", -0.2f }, { "layer1.cutoff", 1.4f } };
            x.mod.aftertouch = { { "fx.reverb.mix", 0.2f } };
            const float vel[16] { 1.0f, 0.6f, 0.8f, 0.6f, 0.95f, 0.55f, 0.75f, 0.6f, 1.0f, 0.6f, 0.85f, 0.55f, 0.9f, 0.6f, 0.7f, 0.5f };
            const int on[16] { 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1 };
            for (int i = 0; i < 16; ++i)
            {
                x.arp.vel[(size_t) i] = vel[i];
                x.arp.type[(size_t) i] = (uint8_t) (on[i] ? VaultArp::play : VaultArp::rest);
                x.arp.oct[(size_t) i] = (int8_t) (i % 4 == 3 ? 1 : 0);
                x.gate.level[(size_t) i] = (float) on[i];
            }
            x.arp.gate = 0.75f;
            x.gate.smooth = 0.1f;
        });
        p.setQuick (vault::qTremolo, 0.15f);
        v.refreshAll();
    }
    if (pageAfterLoad >= 0) v.showPage (pageAfterLoad);
    if (macrosAfterLoad)
    {
        p.setMacro (1, 1.0f); p.setMacro (3, 0.95f); p.setMacro (0, 0.2f);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (50);
    }
    vui::smallestFontPx() = 1000.0f;
    const auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
    const juce::File f = juce::File::getCurrentWorkingDirectory().getChildFile (outPath);
    f.deleteFile();
    juce::FileOutputStream os (f);
    juce::PNGImageFormat png;
    const bool ok = os.openedOk() && png.writeImageToStream (img, os);
    std::printf ("%s | preset %d \"%s\" (%d layers) | showing %d of %d | editor %dx%d, smallest text %.1f px -> %s %s\n",
                 p.bankDescription().toRawUTF8(), pick, pr.name.toRawUTF8(), (int) pr.parts.size(), v.visibleCount(), (int) bank.size(),
                 ed->getWidth(), ed->getHeight(), vui::smallestFontPx() * ed->getUiScale(), outPath.toRawUTF8(), ok ? "ok" : "FAILED");
    for (int n : { 60, 64, 67 }) p.getKeyboardState().noteOff (1, n, 0.0f);
    ed.reset();
    return ok ? 0 : 1;
}
#include "VaultReport1Verify.inc"   // VaultTests -report1: owner report #1 re-tested through the window
} // namespace

#if JUCE_LINUX
 #include <dlfcn.h>
#endif
// xvfb has no window manager, so the _NET_WM_* / WM_* atoms JUCE looks up with "only if exists" are missing and putting a
// window on the desktop fails with BadAtom. Interning them first (as a window manager would) lets -report1 open real
// windows; nothing else changes (the properties themselves are not set).
static void internWindowManagerAtoms()
{
   #if JUCE_LINUX
    if (auto* lib = dlopen ("libX11.so.6", RTLD_NOW))
    {
        using OpenFn = void* (*) (const char*);
        using InternFn = unsigned long (*) (void*, const char*, int);
        using CloseFn = int (*) (void*);
        auto openD = (OpenFn) dlsym (lib, "XOpenDisplay");
        auto intern = (InternFn) dlsym (lib, "XInternAtom");
        auto closeD = (CloseFn) dlsym (lib, "XCloseDisplay");
        if (openD != nullptr && intern != nullptr && closeD != nullptr)
            if (auto* d = openD (nullptr))
            {
                for (auto* a : { "WM_CHANGE_STATE", "WM_DELETE_WINDOW", "WM_PROTOCOLS", "WM_STATE", "WM_TAKE_FOCUS", "_MOTIF_WM_HINTS",
                                 "_NET_WM_PING", "_NET_WM_STATE", "_NET_WM_STATE_HIDDEN", "_NET_WM_STATE_MAXIMIZED_HORZ",
                                 "_NET_WM_STATE_MAXIMIZED_VERT", "_NET_WM_WINDOW_TYPE", "_NET_WM_WINDOW_TYPE_NORMAL",
                                 "_NET_WM_WINDOW_TYPE_COMBO", "_NET_WM_ALLOWED_ACTIONS", "_NET_FRAME_EXTENTS", "_NET_WM_STATE_FULLSCREEN" })
                    intern (d, a, 0);
                // (the connection stays open: Xvfb resets - and forgets the atoms - when its last client disconnects)
                juce::ignoreUnused (closeD);
            }
    }
   #endif
}

int vaultModelTests();   // tests/VaultModelTests.cpp: the physical models

int vaultConvCheck (const juce::File& bankFile, int step, int threads);   // tests/VaultV3Tests.cpp
int vaultV3Tests();
namespace pitchtest { int run (int argc, char** argv); }   // tests/VaultPitchTests.cpp: every preset, every key at its own pitch

int main (int argc, char** argv)
{
    std::setvbuf (stdout, nullptr, _IONBF, 0);
    if (argc > 1 && juce::String (argv[1]) == "-report1") internWindowManagerAtoms();   // before JUCE opens the display
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
    if (mode == "-convcheck" && argc > 2)
        return vaultConvCheck (juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]), argc > 3 ? juce::String (argv[3]).getIntValue() : 1,
                               argc > 4 ? juce::String (argv[4]).getIntValue() : (int) std::max (1u, std::thread::hardware_concurrency()));
    if (mode == "-pitch")   // owner report item 0: every factory preset plays every key C2..C7 at its own pitch
        return pitchtest::run (argc, argv);
    if (mode == "-report1") return report1Verify();   // run under xvfb-run
    if (mode == "-v3") { const int f = vaultV3Tests(); std::printf ("vault v3 failures: %d\n", f); return f; }
    if (mode == "-models")   // the physical-model tests only
        return vaultModelTests();
    if (mode == "-uitest")
    {
        const int f = uiTests();
        std::printf ("vault ui failures: %d\n", f);
        return f;
    }
    if (mode == "-uifix")   // only the owner-report checks (keys, focus, tabs, same preset, knob values, renders); part of -uitest
    {
        uiSetup ("uifix");
        VaultEditor::simulatedDisplay() = { 0, 0, 1920, 1040 };
        const juce::String only = argc > 2 ? juce::String (argv[2]) : juce::String();
        if (only.isEmpty() || only == "keys") ownerFixKeyboardTests();
        if (only.isEmpty() || only == "pages") ownerFixPageTests();
        if (only.isEmpty() || only == "values") ownerFixValueTests();
        if (only.isEmpty() || only == "render") ownerFixRenderTests();
        if (only.isEmpty() || only == "more") ownerFixMoreTests();
        if (only == "v3pages") { VaultProcessor p; v3PageTests (p); }
        if (only == "layout") { VaultProcessor p; windowSizeTests (p); readabilityTests (p); }
        if (only == "qa") { VaultEditor::simulatedDisplay() = { 0, 0, 3840, 2120 }; VaultSettings::get()->setScale (1.0f); VaultProcessor p; uiQaTests (p); }
        std::printf ("vault ui fix failures: %d\n", fails);
        return fails;
    }
    if (mode == "-uitestv2")   // child of -uitest (CHROME_VAULT_BANK / CHROME_VAULT_LIBRARY set by the parent)
    {
        const int f = uiV2Tests();
        std::printf ("vault ui v2 failures: %d\n", f);
        return f;
    }
    if (mode == "-writeuidemo" && argc > 2)   // -writeuidemo <out.cvb>: the v2 UI demo bank (for -shot with CHROME_VAULT_BANK)
    {
        const auto f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        const auto b = uiDemoV2Bank();
        const bool ok = vault::saveBankFile (f, b);
        std::printf ("%d v2 UI demo presets -> %s %s\n", (int) b.size(), argv[2], ok ? "ok" : "FAILED");
        return ok ? 0 : 1;
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
    if (mode == "-writev2demo" && argc > 2)   // -writev2demo <out.cvb>: a few v2 presets on the 4 test library instruments + a model
    {
        std::vector<VaultPreset> b;
        auto add = [&] (const char* id, const char* name, int cat, std::vector<VaultPart> parts, float gain)
        {
            VaultPreset p; p.id = id; p.name = name; p.category = cat; p.parts = std::move (parts); p.gain = gain; b.push_back (p);
        };
        auto smp = [] (const char* inst, const char* art, const char* label, float level, std::vector<std::pair<juce::String, float>> v = {})
        {
            VaultPart part; part.kind = VaultPartKind::sample; part.source = inst; part.articulation = art; part.label = label; part.level = level; part.values = v;
            return part;
        };
        add ("cv.demo.piano", "Concert Grand", vault::catPiano, { smp ("piano.grand.concert", "sustain", "Concert Grand", 1.0f, { { "revMix", 0.18f } }) }, 1.0f);
        add ("cv.demo.violins", "Violin Section", vault::catStrings, { smp ("strings.violins.ensemble", "sustain", "Violins", 1.0f, { { "s.attack", 0.08f }, { "revMix", 0.25f } }) }, 1.0f);
        add ("cv.demo.flute", "Concert Flute", vault::catFlutesWoodwinds, { smp ("winds.flute.concert", "vibrato", "Flute", 1.0f, { { "revMix", 0.2f } }) }, 1.2f);
        add ("cv.demo.marimba", "Marimba", vault::catBellsMallets, { smp ("mallets.marimba", "hit", "Marimba", 1.0f, { { "revMix", 0.15f } }) }, 1.2f);
        VaultPart sine; sine.kind = VaultPartKind::model; sine.source = "sine"; sine.label = "Sine"; sine.level = 0.3f; sine.octave = -1;
        add ("cv.demo.layer", "Piano Strings", vault::catPiano, { smp ("piano.grand.concert", "sustain", "Grand", 1.0f, { { "revMix", 0.22f } }),
                                                                  smp ("strings.violins.ensemble", "sustain", "Violins", 0.45f, { { "s.attack", 0.3f } }), sine }, 0.9f);
        int bad = 0; juce::String why;
        for (auto& p : b) if (! vault::validatePreset (p, &why)) { ++bad; std::printf ("invalid: %s\n", why.toRawUTF8()); }
        const auto f = juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]);
        const bool ok = bad == 0 && vault::saveBankFile (f, b);
        std::printf ("%d v2 presets -> %s %s\n", (int) b.size(), argv[2], ok ? "ok" : "FAILED");
        return ok ? 0 : 1;
    }
    if (mode == "-macrostats")   // which kinds of targets the bank's sound macros / modulation use (editor value display)
    {
        VaultProcessor p;
        std::map<juce::String, int> macroKinds, modKinds;
        int v3 = 0;
        for (auto pr : p.bank())
        {
            v3 += pr.v3 ? 1 : 0;
            vault::upgradeToV3 (pr);
            std::set<juce::String> seen, seenMod;
            auto kindOf = [] (const juce::String& t)
            {
                if (t.startsWith ("layer")) return "layerN." + t.fromFirstOccurrenceOf (".", false, false);
                if (t.startsWith ("fx") && ! t.startsWith ("fx.")) return "fxN." + t.fromFirstOccurrenceOf (".", false, false);
                return t;
            };
            for (auto& m : pr.macros) for (auto& t : m.targets) seen.insert (kindOf (t.target));
            for (auto& l : pr.mod.lfo) for (auto& d : l.dests) seenMod.insert (kindOf (d.target));
            for (auto* list : { &pr.mod.modWheel, &pr.mod.velocity, &pr.mod.aftertouch }) for (auto& d : *list) seenMod.insert (kindOf (d.target));
            for (auto& s : seen) ++macroKinds[s];
            for (auto& s : seenMod) ++modKinds[s];
        }
        std::printf ("%s, %d stored as v3\nmacro targets (presets using them):\n", p.bankDescription().toRawUTF8(), v3);
        for (auto& [k, n] : macroKinds) std::printf ("  %-28s %5d\n", k.toRawUTF8(), n);
        std::printf ("modulation targets:\n");
        for (auto& [k, n] : modKinds) std::printf ("  %-28s %5d\n", k.toRawUTF8(), n);
        return 0;
    }
    if (mode == "-bankinfo")
    {
        VaultProcessor p;
        std::printf ("%s\n", p.bankDescription().toRawUTF8());
        int cats[vault::numCategories] {}, layers[vault::maxParts + 1] {};
        for (auto& pr : p.bank()) { ++cats[pr.category]; ++layers[juce::jlimit (0, (int) vault::maxParts, (int) pr.parts.size())]; }
        for (int c = 0; c < vault::numCategories; ++c) std::printf ("  %-20s %5d\n", vault::categoryNames()[c].toRawUTF8(), cats[c]);
        for (int l = 1; l <= vault::maxParts; ++l) std::printf ("  %d layer(s): %5d\n", l, layers[l]);
        return 0;
    }
    if (mode == "-libreport")   // -libreport <dir> [note ...]: every library instrument / articulation rendered + measured
    {
        std::vector<int> notes;
        for (int i = 3; i < argc; ++i) notes.push_back (juce::String (argv[i]).getIntValue());
        if (notes.empty()) notes = { vault::homeNote };
        libraryReport (argc > 2 ? juce::File::getCurrentWorkingDirectory().getChildFile (argv[2]) : juce::File(), notes, false);
        return 0;
    }
    unitTests();
    samplerTests();
    const int modelFailures = vaultModelTests();
    libraryReport ({}, { vault::homeNote }, true);
    const int v3Failures = vaultV3Tests();
    const int f = browserTests() + modelFailures + v3Failures;
    std::printf ("vault failures: %d\n", f);
    return f;
}
