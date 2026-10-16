// CHROME VAULT v3 tests (part of VaultTests): the CVB v3 format, the v2 -> v3 conversion (against the frozen v2 engine),
// every FX type, LFOs, macros, quick knobs, arpeggiator, gate, locks, CPU, plugin state.
//   VaultTests -v3                        only these tests
//   VaultTests -convcheck <bank.cvb> [step] [threads]   every step-th preset of a v1 / v2 bank: v2 engine vs converted v3
#include "../Source/vault/VaultProcessor.h"
#include "../Source/ParamKeys.h"
#include "legacy/LegacyVaultEngine.h"
#include <atomic>
#include <cstdio>
#include <thread>
#include <map>
#include <set>

namespace v3test
{
int fails = 0;
void check (bool ok, const juce::String& what)
{
    std::printf ("%s %s\n", ok ? "   ok:" : "!! FAIL:", what.toRawUTF8());
    if (! ok) ++fails;
}

struct Diff { float maxAbs = 0, rmsRef = 0, rmsDiff = 0; bool finite = true; float db() const { return 20.0f * std::log10 (std::max (1.0e-12f, rmsDiff) / std::max (1.0e-9f, rmsRef)); } };
Diff compare (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    Diff d;
    double ra = 0, rd = 0;
    const int n = std::min (a.getNumSamples(), b.getNumSamples());
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < n; ++i)
        {
            const float x = a.getSample (c, i), y = b.getSample (c, i);
            if (! std::isfinite (x) || ! std::isfinite (y)) d.finite = false;
            ra += (double) x * x; rd += (double) (x - y) * (x - y);
            d.maxAbs = std::max (d.maxAbs, std::abs (x - y));
        }
    d.rmsRef = (float) std::sqrt (ra / std::max (1, 2 * n));
    d.rmsDiff = (float) std::sqrt (rd / std::max (1, 2 * n));
    return d;
}

// the listening phrase (C5 E5 G5 C6, then the C5 triad) + a few controllers
juce::MidiBuffer phrase (double sr, bool controllers)
{
    juce::MidiBuffer mb;
    auto at = [sr] (double t) { return (int) std::round (t * sr); };
    const int mel[] { 60, 64, 67, 72 };
    for (int i = 0; i < 4; ++i)
    {
        mb.addEvent (juce::MidiMessage::noteOn (1, mel[i], (juce::uint8) (80 + 10 * i)), at (0.3 * i));
        mb.addEvent (juce::MidiMessage::noteOff (1, mel[i]), at (0.3 * i + 0.27));
    }
    for (int n : { 60, 64, 67 })
    {
        mb.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 96), at (1.3));
        mb.addEvent (juce::MidiMessage::noteOff (1, n), at (2.3));
    }
    if (controllers)
    {
        mb.addEvent (juce::MidiMessage::pitchWheel (1, 8192 + 3000), at (1.6));
        mb.addEvent (juce::MidiMessage::controllerEvent (1, 1, 90), at (1.7));
        mb.addEvent (juce::MidiMessage::pitchWheel (1, 8192), at (1.9));
        mb.addEvent (juce::MidiMessage::controllerEvent (1, 64, 127), at (2.0));
        mb.addEvent (juce::MidiMessage::controllerEvent (1, 64, 0), at (2.6));
    }
    return mb;
}

// a v2 preset through the v2 engine and its v3 conversion (written + read back as CVB v3) through the v3 engine
Diff convDiff (const VaultPreset& legacy, double sr, const float* macros, bool controllers, VaultPreset* converted = nullptr)
{
    auto up = legacy;
    vault::upgradeToV3 (up);
    std::vector<VaultPreset> back;
    const auto mb = vault::serializeBank ({ up }, true);
    vault::deserializeBank (mb.getData(), mb.getSize(), back);
    if (converted != nullptr && ! back.empty()) *converted = back.front();
    const auto midi = phrase (sr, controllers);
    const int len = (int) (3.2 * sr);
    const auto a = LegacyVaultEngine::renderMidi (legacy, midi, len, sr, macros);
    const auto b = VaultEngine::renderMidi (back.empty() ? up : back.front(), midi, len, sr, macros);
    return compare (a, b);
}
} // namespace v3test

int vaultConvCheck (const juce::File& bankFile, int step, int threads)
{
    using namespace v3test;
    juce::MemoryBlock mb;
    std::vector<VaultPreset> bank;
    int version = 0;
    if (! bankFile.loadFileAsData (mb) || ! vault::deserializeBank (mb.getData(), mb.getSize(), bank, nullptr, &version, true))
    { std::printf ("cannot read %s\n", bankFile.getFullPathName().toRawUTF8()); return 1; }
    std::printf ("%s: CVB v%d, %d presets, every %d-th, library %s\n", bankFile.getFileName().toRawUTF8(), version, (int) bank.size(), step,
                 vault::SampleLibrary::getShared()->libraryFound() ? "found" : "MISSING");
    std::vector<int> idx;
    const auto only = juce::SystemStats::getEnvironmentVariable ("CV_CONV_ONLY", {});   // one preset id: per-pass details
    for (int i = 0; i < (int) bank.size(); i += std::max (1, step))
        if (only.isEmpty() || bank[(size_t) i].id == only) idx.push_back (i);
    if (only.isNotEmpty()) for (int i = 0; i < (int) bank.size(); ++i) if (bank[(size_t) i].id == only && idx.empty()) idx.push_back (i);
    std::vector<float> worst (idx.size(), -300.0f), worstMax (idx.size(), 0.0f);
    std::vector<int> slots (idx.size(), 0);
    std::atomic<int> next { 0 };
    std::vector<std::thread> pool;
    const float moved[4] { 0.15f, 0.85f, 0.72f, 0.9f }, low[4] { 0.9f, 0.1f, 0.2f, 0.05f };
    for (int t = 0; t < std::max (1, threads); ++t)
        pool.emplace_back ([&]
        {
            for (int k; (k = next++) < (int) idx.size();)
            {
                const auto& p = bank[(size_t) idx[(size_t) k]];
                VaultPreset conv;
                for (int pass = 0; pass < 3; ++pass)
                {
                    const auto d = convDiff (p, 48000.0, pass == 0 ? nullptr : pass == 1 ? moved : low, pass == 0, pass == 0 ? &conv : nullptr);
                    worst[(size_t) k] = std::max (worst[(size_t) k], d.rmsRef < 1.0e-7f ? -300.0f : d.db());
                    if (only.isNotEmpty()) std::printf ("   pass %d: %.1f dB, max %.2e\n", pass, d.db(), d.maxAbs);
                    worstMax[(size_t) k] = std::max (worstMax[(size_t) k], d.maxAbs);
                }
                slots[(size_t) k] = (int) conv.fx.size();
            }
        });
    for (auto& t : pool) t.join();
    int bad = 0, maxSlots = 0;
    float w = -300, wm = 0;
    std::map<int, int> perSlots;
    for (size_t k = 0; k < idx.size(); ++k)
    {
        ++perSlots[slots[k]];
        maxSlots = std::max (maxSlots, slots[k]);
        w = std::max (w, worst[k]); wm = std::max (wm, worstMax[k]);
        if (worst[k] > -90.0f || worstMax[k] > 1.0e-3f)
        {
            if (bad++ < 25) std::printf ("   %s %s: diff %.1f dB rms, max %.2e\n", bank[(size_t) idx[k]].id.toRawUTF8(), bank[(size_t) idx[k]].name.toRawUTF8(), worst[k], worstMax[k]);
        }
    }
    std::printf ("chain sizes:"); for (auto& [s, c] : perSlots) std::printf (" %d slots: %d;", s, c); std::printf ("\n");
    std::printf ("worst difference %.1f dB (rms, vs the v2 render), max sample difference %.2e; %d of %d presets above -90 dB / 1e-3\n",
                 w, wm, bad, (int) idx.size());
    return bad;
}


//==============================================================================
#include "../tools/vault/recipes/Recipe.h"
namespace allocHook { extern thread_local bool countAllocs; extern std::atomic<int> allocs; }

namespace v3test
{
constexpr double srT = 48000.0;

VaultPart synth (std::initializer_list<std::pair<const char*, float>> vals, const char* label = "Saw", float level = 1.0f, float pan = 0.0f, int oct = 0)
{
    VaultPart p;
    p.kind = VaultPartKind::synth; p.label = label; p.level = level; p.pan = pan; p.octave = oct;
    for (auto& [k, v] : vals) p.values.push_back ({ k, v });
    return p;
}
// a plain sustained saw (v3, no FX): the base of the FX / gate / arp tests
VaultPreset base (const char* id = "t3.base")
{
    VaultPreset p;
    p.id = id; p.name = "V3 Test"; p.category = vault::catSynth; p.v3 = true;
    p.parts.push_back (synth ({ { ID::engine, 0 }, { ID::wave, 0.5f }, { ID::unison, 1 }, { ID::attack, 0.002f }, { ID::sustain, 1.0f },
                                { ID::release, 0.15f }, { ID::cutoff, 5000 }, { ID::velSens, 0.0f } }));
    return p;
}

struct Rig
{
    std::unique_ptr<VaultEngine> eng = std::make_unique<VaultEngine>();
    double sr; int block;
    VaultTransport t;
    int64_t pos = 0;
    explicit Rig (double rate = srT, int blk = 256) : sr (rate), block (blk) { eng->prepare (sr, block); }
    void load (const VaultPreset& p, bool instant = true) { eng->setPreset (VaultEngine::compile (p), instant); eng->serviceResources(); }
    // renders `seconds`; `events` (sample -> message) are absolute; host = transport playing from ppq = pos * bps
    juce::AudioBuffer<float> run (double seconds, const std::vector<std::pair<int64_t, juce::MidiMessage>>& events = {}, bool host = false, double bpm = 120.0,
                                  const std::function<void (int64_t)>& perBlock = {})
    {
        const int total = (int) std::round (seconds * sr);
        juce::AudioBuffer<float> out (2, total), buf (2, block);
        juce::MidiBuffer mb;
        for (int s0 = 0; s0 < total; s0 += block)
        {
            const int len = std::min (block, total - s0);
            mb.clear();
            for (auto& [at, m] : events) if (at >= pos && at < pos + len) mb.addEvent (m, (int) (at - pos));
            if (perBlock) perBlock (pos);
            t.bpm = bpm; t.playing = host; t.ppq = (double) pos * bpm / 60.0 / sr;
            buf.setSize (2, len, false, false, true);
            eng->process (buf, mb, t);
            for (int c = 0; c < 2; ++c) out.copyFrom (c, s0, buf, c, 0, len);
            pos += len;
        }
        return out;
    }
    int entryOf (vault::FxType type, bool presetSlot = true) const
    {
        for (int e = 0; e < eng->chainSize(); ++e) if (eng->chainType (e) == type && (! presetSlot || eng->chainSlot (e) >= 0)) return e;
        return -1;
    }
};
std::vector<std::pair<int64_t, juce::MidiMessage>> chord (std::initializer_list<int> notes, double onSec, double offSec, double sr = srT, juce::uint8 vel = 100)
{
    std::vector<std::pair<int64_t, juce::MidiMessage>> e;
    for (int n : notes)
    {
        e.push_back ({ (int64_t) (onSec * sr), juce::MidiMessage::noteOn (1, n, vel) });
        if (offSec > onSec) e.push_back ({ (int64_t) (offSec * sr), juce::MidiMessage::noteOff (1, n) });
    }
    return e;
}
float maxJump (const juce::AudioBuffer<float>& b, int from = 1, int to = -1)
{
    float m = 0;
    if (to < 0) to = b.getNumSamples();
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = std::max (1, from); i < to; ++i) m = std::max (m, std::abs (b.getSample (c, i) - b.getSample (c, i - 1)));
    return m;
}
int subnormals (const juce::AudioBuffer<float>& b)
{
    int n = 0;
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i) n += std::fpclassify (b.getSample (c, i)) == FP_SUBNORMAL ? 1 : 0;
    return n;
}
bool finite (const juce::AudioBuffer<float>& b)
{
    for (int c = 0; c < b.getNumChannels(); ++c)
        for (int i = 0; i < b.getNumSamples(); ++i) if (! std::isfinite (b.getSample (c, i))) return false;
    return true;
}
float rms (const juce::AudioBuffer<float>& b, int from, int to)
{
    double a = 0;
    for (int c = 0; c < 2; ++c) for (int i = from; i < to; ++i) a += (double) b.getSample (c, i) * b.getSample (c, i);
    return (float) std::sqrt (a / std::max (1, 2 * (to - from)));
}
float centroid (const juce::AudioBuffer<float>& b, int from)
{
    constexpr int order = 13, n = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> d (2 * n, 0.0f);
    for (int i = 0; i < n && from + i < b.getNumSamples(); ++i) d[(size_t) i] = 0.5f * (b.getSample (0, from + i) + b.getSample (1, from + i)) * (0.5f - 0.5f * std::cos (6.2831853f * i / n));
    fft.performFrequencyOnlyForwardTransform (d.data());
    double num = 0, den = 0;
    for (int i = 1; i < n / 2; ++i) { const double pw = (double) d[(size_t) i] * d[(size_t) i]; num += pw * i * srT / n; den += pw; }
    return (float) (num / std::max (1e-20, den));
}

// a v3 preset with every section in use (serializer / state tests)
VaultPreset richPreset()
{
    VaultPreset p = base ("t3.rich");
    p.name = "Rich V3";
    p.parts.push_back (synth ({ { ID::engine, 1 }, { ID::sustain, 0.6f } }, "Bell", 0.6f, 0.3f, 1));
    recipe::design (p)
        .fx ({ recipe::fx::comp (-20, 3), recipe::fx::drive (recipe::fx::Drive::tape, 0.2f, 0.8f, 9000), recipe::fx::chorus (0.3f),
               recipe::fx::eq (1.0f, -1.5f, 0.5f, 0.0f, 800), recipe::fx::filter (recipe::fx::Filter::lowPass, 6000, 0.2f, 0.3f, 0.2f, 0.5f),
               recipe::fx::delay ("1/8D", 0.3f, 0.15f), recipe::fx::reverb (recipe::fx::Reverb::shimmer, 0.7f, 1.3f, 0.25f), recipe::fx::width (0.65f, 120) })
        .finish (0.3f)
        .clearMacros()
        .macro (0, recipe::macro ("Hammer").to ("layer2.level", 0.2f, 1.2f).to ("fx.drive.amount", 0.0f, 0.5f, 0.4f).to ("cutoff", -1, 1), 0.35f)
        .macro (1, recipe::macro ("Air").to ("fx.reverb.mix", 0.1f, 0.6f).to ("fx3.mix", 0.0f, 0.6f).to ("quick.width", -0.2f, 0.3f), 0.6f)
        .macro (2, recipe::macro ("Wobble").to ("lfo1.depth", 0, 1).to ("gate.mix", 0, 1))
        .macro (3, recipe::macro ("Motion").to ("lfo2.rate", 0.2f, 6.0f, -0.5f))
        .lfo (0, recipe::lfo (VaultLfo::triangle).sync ("1/4D").fade (0.5f).to ("cutoff", 0.5f).to ("fx.delay.mix", 0.2f))
        .lfo (1, recipe::lfo (VaultLfo::sampleHold).rate (3.0f).free().phase (0.25f).to ("pan", 0.3f).to ("layer1.tune", 0.1f))
        .modWheel ("lfo1.depth", 1.0f).velocity ("fx.reverb.size", 0.2f).aftertouch ("pitch", 0.5f)
        .arp (recipe::arp (VaultArp::upDown, "1/16T", 2, 0.6f, 0.2f))
        .gate (recipe::gate ("1/8", { 1, 0.2f, 0.8f, 0 }, 0.9f, 0.3f, 0.7f));
    p.arp.vel[3] = 0.5f; p.arp.type[5] = VaultArp::tie; p.arp.oct[7] = -1;
    return p;
}

// the pre-v3 bank layout (CVB version 2), for the "old files still load" test
juce::MemoryBlock writeV2 (const std::vector<VaultPreset>& bank)
{
    juce::StringArray keys;
    for (int k = 0; k < kk::vk::count; ++k) keys.add (kk::vk::info (k).id);
    for (auto& sp : vault::sampleParams()) keys.add (sp.id);
    for (auto& p : bank) for (auto& part : p.parts) for (auto& [id, v] : part.values) keys.addIfNotAlreadyThere (id);
    juce::MemoryOutputStream pl;
    pl.writeShort ((short) keys.size());
    for (auto& k : keys) pl.writeString (k);
    for (auto& p : bank)
    {
        pl.writeString (p.id); pl.writeString (p.name); pl.writeByte ((char) p.category); pl.writeFloat (p.gain);
        for (float m : p.macroDefaults) pl.writeFloat (m);
        pl.writeByte ((char) p.parts.size());
        for (auto& part : p.parts)
        {
            pl.writeByte ((char) part.kind); pl.writeString (part.source); pl.writeString (part.articulation); pl.writeString (part.label);
            pl.writeFloat (part.level); pl.writeFloat (part.pan); pl.writeByte ((char) part.octave); pl.writeByte ((char) part.semi);
            pl.writeShort ((short) part.values.size());
            for (auto& [id, v] : part.values) { pl.writeShort ((short) keys.indexOf (id)); pl.writeFloat (v); }
        }
    }
    juce::MemoryOutputStream out;
    out.write ("CVB1", 4); out.writeShort (2); out.writeShort (0); out.writeInt ((int) bank.size()); out.writeInt ((int) pl.getDataSize());
    out.write (pl.getData(), pl.getDataSize());
    return out.getMemoryBlock();
}

void formatTests()
{
    std::printf ("-- v3 format\n");
    const auto rich = richPreset();
    juce::String why;
    check (vault::validatePreset (rich, &why), "a preset with every v3 section validates " + why);
    check ((int) rich.fx.size() == 8 && rich.macros[0].name == "Hammer", "recipe::design builds an 8-slot chain + named macros");
    std::vector<VaultPreset> back;
    int version = 0;
    for (bool gz : { true, false })
    {
        const auto mb = vault::serializeBank ({ rich, base() }, gz);
        back.clear();
        check (vault::deserializeBank (mb.getData(), mb.getSize(), back, nullptr, &version) && back.size() == 2 && back[0] == rich && version == vault::bankFormatVersion,
               juce::String ("CVB v3 round trip, every new field (") + (gz ? "gzip" : "raw") + ")");
    }
    // v2 files: still read; the legacy preset comes back converted, or as stored with keepLegacy
    {
        VaultPreset old;
        old.id = "t2.old"; old.name = "Old Pad"; old.category = vault::catPads; old.macroDefaults[1] = 0.7f;
        old.parts.push_back (synth ({ { ID::engine, 0 }, { ID::revMix, 0.4f }, { ID::revType, 1 }, { ID::chorus, 0.3f }, { ID::delayMix, 0.2f },
                                      { ID::delayTime, 4 }, { ID::eqHigh, -3 }, { ID::width, 0.8f }, { ID::drive, 0.25f }, { ID::master, 0.4f } }));
        const auto mb = writeV2 ({ old });
        std::vector<VaultPreset> conv, raw;
        const bool ok = vault::deserializeBank (mb.getData(), mb.getSize(), conv, nullptr, &version)
                     && vault::deserializeBank (mb.getData(), mb.getSize(), raw, nullptr, nullptr, true);
        auto expect = old; vault::upgradeToV3 (expect);
        check (ok && version == 2 && conv.size() == 1 && conv[0] == expect && conv[0].v3 && raw[0] == old && ! raw[0].v3, "CVB v2 is still read (converted, or kept with keepLegacy)");
        const auto& c = conv[0];
        juce::StringArray types;
        for (auto& f : c.fx) types.add (vault::fxTypeInfo (f.type).id);
        check (types.joinIntoString (",") == "drive,chorus,eq,delay,reverb,width", "v2 master FX -> chain in the rack order (" + types.joinIntoString (",") + ")");
        check (c.macros[1].name == "Space" && c.macros[1].targets.size() == 1 && c.macros[1].targets[0].target == "classic.space" && c.macroDefaults[1] == 0.7f,
               "v2 macros -> sound macros with classic targets");
        bool noFxKeys = true;
        for (auto& [k, v] : c.parts[0].values) noFxKeys &= k != ID::revMix && k != ID::chorus && k != ID::width && k != ID::master;
        check (noFxKeys && std::abs (c.finish - 0.4f) < 1e-6f, "the converted parts carry no FX keys, MASTER -> finish");
        auto again = c; vault::upgradeToV3 (again);
        check (again == c, "upgradeToV3 is a no-op on a v3 preset");
    }
    // invalid v3 presets
    {
        auto bad = [&] (std::function<void (VaultPreset&)> f) { auto p = base(); f (p); return ! vault::validatePreset (p); };
        check (bad ([] (VaultPreset& p) { for (int i = 0; i < 9; ++i) p.fx.push_back (VaultFxSlot::make (vault::FxType::eq)); }), "rejects 9 slots");
        check (bad ([] (VaultPreset& p) { for (int i = 0; i < 3; ++i) p.fx.push_back (VaultFxSlot::make (vault::FxType::reverb)); }), "rejects 3 reverbs");
        check (bad ([] (VaultPreset& p) { p.fx.push_back (VaultFxSlot::make (vault::FxType::delay)); p.fx[0].p[2] = 3.0f; }), "rejects an FX parameter out of range");
        check (bad ([] (VaultPreset& p) { p.mod.lfo[0].dests.push_back ({ "fx.reverb.nope", 0.5f }); }), "rejects an unknown target");
        check (bad ([] (VaultPreset& p) { for (int i = 0; i < 5; ++i) p.mod.lfo[0].dests.push_back ({ "cutoff", 0.1f }); }), "rejects 5 LFO destinations");
        check (bad ([] (VaultPreset& p) { p.macros[0].name = "X"; for (int i = 0; i < 9; ++i) p.macros[0].targets.push_back ({ "pitch", 0, 1, 0 }); }), "rejects 9 macro targets");
        check (bad ([] (VaultPreset& p) { p.arp.octaves = 5; }), "rejects 5 arp octaves");
        check (bad ([] (VaultPreset& p) { p.mod.modWheel.push_back ({ "layer3.level", 0.5f }); }), "rejects a target on a missing layer");
        check (bad ([] (VaultPreset& p) { p.mod.modWheel.push_back ({ "fx2.mix", 0.5f }); }), "rejects a slot target past the chain");
    }
    // targets
    {
        check (vault::parseTarget ("fx.reverb.mix").kind == vault::TargetRef::fxType && vault::parseTarget ("layer2.cutoff").a == 1
               && vault::parseTarget ("quick.eqTilt").kind == vault::TargetRef::quick && ! vault::parseTarget ("layer5.level").valid()
               && ! vault::parseTarget ("bogus").valid(), "target ids parse");
        check (vault::describeTarget ("fx.delay.feedback") == "Delay Feedback" && vault::describeTarget ("layer2.level") == "Layer 2 Level", "target names for the UI");
        bool divOk = vault::divisionIndex ("1/16T") >= 0 && std::abs (vault::divisionBeats (vault::divisionIndex ("1/8D")) - 0.75) < 1e-12
                  && std::abs (vault::divisionBeats (vault::divisionIndex ("1/4T")) - 2.0 / 3.0) < 1e-12;
        check (divOk, "tempo divisions incl. triplet / dotted");
    }
}

// EVOLVE's FxRack, now built from the shared stages, against the frozen copy of the old rack: random settings, every stage
void evolveRackTests()
{
    std::printf ("-- EVOLVE FX rack (refactored into stages) vs the frozen rack\n");
    kk::Rng rng; rng.seed (99);
    float worst = 0;
    for (int trial = 0; trial < 24; ++trial)
    {
        kk::FxParams f;
        auto u = [&] { return rng.uni(); };
        f.drive = u() < 0.5f ? u() : 0; f.driveType = (int) (u() * 5); f.cleanLow = u() < 0.3f; f.crush = u() < 0.4f ? u() * 0.6f : 0; f.wow = u() < 0.4f ? u() : 0;
        f.chorus = u(); f.phaser = u() < 0.5f ? u() : 0; f.flanger = u() < 0.5f ? u() : 0; f.delayMix = u() * 0.6f; f.delayFb = u() * 0.8f;
        f.delayBeats = 0.25 + (int) (u() * 4) * 0.25; f.delayMode = (int) (u() * 3); f.revMix = u() * 0.6f; f.revSize = u(); f.revType = (int) (u() * 3);
        f.eqLow = (u() - 0.5f) * 20; f.eqHigh = (u() - 0.5f) * 20; f.reverse = u() < 0.4f ? u() : 0; f.width = u(); f.ghost = u() < 0.3f ? u() : 0;
        f.circuit = u() < 0.4f ? u() : 0; f.circBeats = 0.25; f.body = u() < 0.3f ? 1 + (int) (u() * 6) : 0; f.punch = u() < 0.4f ? u() : 0;
        f.halftime = u() < 0.3f ? u() : 0; f.master = u(); f.monoLows = u() < 0.3f; f.eco = u() < 0.2f; f.bpm = 90 + u() * 80; f.seed = (uint32_t) (u() * 9999);
        f.outGain = 0.5f + u();
        auto oldR = std::make_unique<kkv2::FxRack>(); auto newR = std::make_unique<kk::FxRack>();
        oldR->prepare (48000.0, 128); newR->prepare (48000.0, 128);
        kk::Rng sig; sig.seed (5 + (uint32_t) trial);
        float l1[128], r1[128], l2[128], r2[128], g[128];
        double beat = 0;
        for (int b = 0; b < 600; ++b)
        {
            for (int i = 0; i < 128; ++i)
            {
                const float x = (b < 300 ? 0.4f * std::sin (0.031f * (float) (b * 128 + i)) : 0.0f) + 0.05f * sig.bi();
                l1[i] = l2[i] = x; r1[i] = r2[i] = 0.8f * x + 0.02f * sig.bi(); g[i] = 0.1f * x;
            }
            f.beatPos = beat; beat += 128 * f.bpm / 60.0 / 48000.0;
            oldR->process (l1, r1, g, 128, f); newR->process (l2, r2, g, 128, f);
            for (int i = 0; i < 128; ++i) worst = std::max ({ worst, std::abs (l1[i] - l2[i]), std::abs (r1[i] - r2[i]) });
        }
    }
    check (worst == 0.0f, "FxRack from stages = the old FxRack, bit for bit (24 random racks x 600 blocks, max diff " + juce::String (worst) + ")");
}

void conversionTests()
{
    std::printf ("-- v2 -> v3 conversion (frozen v2 engine vs v3 engine)\n");
    auto fb = VaultProcessor::buildFallbackBank();
    std::vector<VaultPreset> set;
    for (size_t i = 0; i < fb.size(); i += 5) set.push_back (fb[i]);
    // hand-made v2 presets with every FX the rack had (incl. lo-fi, reverse, punch, body, ghost, phaser, flanger, bass mode)
    VaultPreset a; a.id = "t2.fx1"; a.name = "All FX"; a.category = vault::catSynth;
    a.parts.push_back (synth ({ { ID::engine, 0 }, { ID::crush, 0.3f }, { ID::wow, 0.4f }, { ID::reverse, 0.3f }, { ID::punch, 0.5f }, { ID::body, 3 },
                                { ID::phaser, 0.5f }, { ID::flanger, 0.4f }, { ID::ghost, 0.4f }, { ID::revMix, 0.3f }, { ID::revType, 2 }, { ID::delayMix, 0.3f },
                                { ID::delayMode, 2 }, { ID::eqLow, 4 }, { ID::width, 0.9f } }));
    a.parts.push_back (synth ({ { ID::engine, 2 }, { ID::sustain, 0.5f } }, "Pluck", 0.7f, -0.4f, 1));
    a.parts[0].values.push_back ({ ID::circuit, 0.6f }); a.parts[0].values.push_back ({ ID::halftime, 0.4f });
    VaultPreset b; b.id = "t2.bass"; b.name = "Bass Mode"; b.category = vault::catBass;
    b.parts.push_back (synth ({ { ID::engine, 6 }, { ID::mono, 1 }, { ID::bassMode, 1 }, { ID::drive, 0.5f }, { ID::driveType, 3 }, { ID::width, 0.3f }, { ID::master, 0.6f } }));
    set.push_back (a); set.push_back (b);
    const float moved[4] { 0.1f, 0.95f, 0.8f, 1.0f };
    int bad = 0, overFull = 0; float worst = -300;
    for (auto& p : set)
        for (int pass = 0; pass < 2; ++pass)
        {
            int dropped = 0;
            { auto u = p; vault::upgradeToV3 (u, &dropped); }
            if (dropped > 0) { overFull += pass == 0 ? 1 : 0; continue; }   // > 8 FX stages (FUTURE): converted with its first 8

            const auto d = convDiff (p, 44100.0, pass == 0 ? nullptr : moved, pass == 0);
            const float db = d.rmsRef < 1e-7f ? -300.0f : d.db();
            worst = std::max (worst, db);
            if (d.maxAbs > 1.0e-6f || ! d.finite)
            {
                ++bad;
                auto u = p; vault::upgradeToV3 (u);
                juce::StringArray ty; for (auto& f : u.fx) ty.add (vault::fxTypeInfo (f.type).id);
                if (bad < 6) std::printf ("   %s pass %d: %.1f dB max %.2e  [%s]\n", p.id.toRawUTF8(), pass, db, d.maxAbs, ty.joinIntoString (",").toRawUTF8());
            }
        }
    check (bad == 0, "converted presets render exactly like the v2 engine (" + juce::String ((int) set.size() - overFull) + " presets x macros as designed / moved, worst "
                     + juce::String (worst, 1) + " dB; " + juce::String (overFull) + " EVOLVE FUTURE sounds with > 8 FX stages skipped)");
    check (overFull <= 2, "at most a couple of starter sounds need more than 8 slots");
}

// every FX type: clean output (finite, no denormals), on / off toggles without clicks, extreme settings stay finite
void fxTypeTests()
{
    std::printf ("-- FX slot types\n");
    for (int t = 0; t < vault::numFxTypes; ++t)
    {
        const auto type = (vault::FxType) t;
        const auto& ti = vault::fxTypeInfo (type);
        auto p = base ("t3.fx");
        auto slot = VaultFxSlot::make (type);
        if (const int mp = vault::fxMainParam (type); mp >= 0 && type != vault::FxType::eq && type != vault::FxType::width) slot.p[(size_t) mp] = 0.6f;
        if (type == vault::FxType::eq) slot.set ("tilt", 4.0f).set ("mid", -6.0f);
        if (type == vault::FxType::width) slot.set ("width", 0.9f).set ("monoBelow", 150.0f);
        if (type == vault::FxType::filter) slot.set ("lfo", 0.5f).set ("env", 0.4f);
        p.fx = { slot };
        auto off = p; off.fx[0].on = false;
        const auto ev = chord ({ 60, 64, 67 }, 0.01, 1.7);
        auto render = [&] (int mode)   // 0 on, 1 off, 2 toggled every 0.2 s
        {
            Rig r;
            r.load (mode == 1 ? off : p);
            const auto cOn = VaultEngine::compile (p), cOff = VaultEngine::compile (off);
            bool state = true;
            return r.run (2.0, ev, false, 120.0, [&] (int64_t pos)
            {
                if (mode != 2) return;
                const bool want = ((pos / (int64_t) (0.2 * srT)) % 2) == 0;
                if (want != state) { state = want; r.eng->updateSound (want ? cOn : cOff); }
            });
        };
        const auto on = render (0), offB = render (1), tog = render (2);
        const float ref = std::max (maxJump (on), maxJump (offB));
        const float jt = maxJump (tog);
        check (finite (on) && finite (tog) && subnormals (on) == 0 && subnormals (tog) == 0 && on.getMagnitude (0, on.getNumSamples()) < 1.0f
                   && rms (on, (int) (0.2 * srT), (int) (1.5 * srT)) > 1e-3f,
               juce::String (ti.name) + ": renders cleanly (finite, no denormals, sounding)");
        check (jt <= ref * 1.25f + 0.002f, juce::String (ti.name) + ": on / off toggles click-free (max step " + juce::String (jt, 4) + " vs " + juce::String (ref, 4) + ")");
        // extreme corners
        bool fin = true;
        for (int corner = 0; corner < 2; ++corner)
        {
            auto q = p;
            for (size_t i = 0; i < ti.params.size(); ++i) q.fx[0].p[i] = corner == 0 ? ti.params[i].hi : ti.params[i].lo;
            Rig r; r.load (q);
            const auto b = r.run (1.2, chord ({ 48, 60, 72 }, 0.0, 0.8, srT, 127));
            fin &= finite (b) && subnormals (b) == 0 && b.getMagnitude (0, b.getNumSamples()) < 1.0f;
        }
        check (fin, juce::String (ti.name) + ": every parameter at its min / max stays finite and under the limiter");
    }
    // the global FX switch fades too
    {
        auto p = base(); p.fx = { recipe::fx::reverb (recipe::fx::Reverb::hall, 0.7f, 1.0f, 0.5f), recipe::fx::delay ("1/8", 0.5f, 0.4f) };
        Rig r; r.load (p);
        const auto b = r.run (2.0, chord ({ 60, 67 }, 0.0, 1.8), false, 120.0, [&] (int64_t pos) { r.eng->setFxOn (((pos / 9600) % 2) == 0); });
        Rig r2; r2.load (p);
        const auto ref = r2.run (2.0, chord ({ 60, 67 }, 0.0, 1.8));
        check (maxJump (b) <= maxJump (ref) * 1.25f + 0.002f, "the FX switch (all slots) toggles click-free");
    }
}

void quickAndMacroTests()
{
    std::printf ("-- quick knobs, macros, LFOs\n");
    // MAIN reverb knob on a preset without a reverb: the default hall, silent at neutral, up to 0.7 mix
    {
        Rig r; r.load (base());
        r.run (0.1);
        const int e = r.entryOf (vault::FxType::reverb, false);
        const float m0 = e >= 0 ? r.eng->chainParam (e, 0) : -1;
        r.eng->setQuick (vault::qReverb, 1.0f);
        r.run (0.4);
        const float m1 = e >= 0 ? r.eng->chainParam (e, 0) : -1;
        check (e >= 0 && r.eng->chainSlot (e) < 0 && m0 == 0.0f && std::abs (m1 - 0.7f) < 1e-3f, "reverb knob without a reverb slot drives a default hall (0 -> "
               + juce::String (m1, 3) + ")");
        auto withRev = base(); withRev.fx = { recipe::fx::reverb (recipe::fx::Reverb::plate, 0.5f, 1.0f, 0.3f) };
        Rig r2; r2.load (withRev); r2.eng->setQuick (vault::qReverb, 0.25f); r2.run (0.4);
        const int e2 = r2.entryOf (vault::FxType::reverb);
        check (e2 >= 0 && std::abs (r2.eng->chainParam (e2, 0) - 0.15f) < 1e-3f && r2.eng->chainSize() == 6, "reverb knob scales the preset's own reverb (0.3 -> 0.15 at 25 %)");
    }
    // cutoff knob darkens, as designed at neutral
    {
        auto renderQ = [] (float q) { Rig r; r.eng->setQuick (vault::qCutoff, q); r.load (base()); return r.run (1.0, chord ({ 60 }, 0.0, 0.9)); };
        const float cN = centroid (renderQ (0.5f), 9600), cD = centroid (renderQ (0.15f), 9600), cB = centroid (renderQ (0.8f), 9600);
        check (cD < cN * 0.8f && cB > cN * 1.05f, "cutoff knob: darker / brighter (centroid " + juce::String ((int) cD) + " / " + juce::String ((int) cN) + " / " + juce::String ((int) cB) + " Hz)");
        Rig a; a.load (base()); const auto x = a.run (1.0, chord ({ 60 }, 0.0, 0.9));
        Rig b; b.eng->setQuick (vault::qCutoff, 0.5f); b.load (base()); const auto y = b.run (1.0, chord ({ 60 }, 0.0, 0.9));
        check (compare (x, y).maxAbs == 0.0f, "every knob at neutral = exactly the designed sound");
    }
    // macros: min / max / curve, absolute and relative targets
    {
        auto p = base();
        p.fx = { recipe::fx::delay ("1/8", 0.3f, 0.2f), recipe::fx::reverb (recipe::fx::Reverb::hall, 0.5f, 1.0f, 0.2f) };
        p.macros[0] = recipe::macro ("Space").to ("fx.reverb.mix", 0.1f, 0.6f);
        p.macros[1] = recipe::macro ("Echo").to ("fx1.feedback", 0.2f, 0.8f, 0.5f).to ("finish", 0.0f, 0.4f);
        p.macros[2] = recipe::macro ("Dark").to ("quick.reverb", 0.0f, 0.5f);
        Rig r; r.load (p);
        auto at = [&] (float m0, float m1, float m2)
        {
            r.eng->setMacro (0, m0); r.eng->setMacro (1, m1); r.eng->setMacro (2, m2);
            r.run (0.5);
            return std::array<float, 3> { r.eng->chainParam (r.entryOf (vault::FxType::reverb), 0), r.eng->chainParam (r.entryOf (vault::FxType::delay), 2), r.eng->effectiveFinish() };
        };
        const auto lo = at (0, 0, 0), hi = at (1, 1, 0), mid = at (0.5f, 0.5f, 0);
        const float curveMid = 0.2f + 0.6f * std::pow (0.5f, 2.5f);
        check (std::abs (lo[0] - 0.1f) < 1e-3f && std::abs (hi[0] - 0.6f) < 1e-3f && std::abs (mid[0] - 0.35f) < 2e-3f, "macro -> reverb mix: 0.1 / 0.35 / 0.6");
        check (std::abs (lo[1] - 0.2f) < 1e-3f && std::abs (hi[1] - 0.8f) < 1e-3f && std::abs (mid[1] - curveMid) < 2e-3f, "macro curve (slow start): feedback "
               + juce::String (mid[1], 3) + " at 50 %");
        check (std::abs (hi[2] - 0.4f) < 1e-3f && std::abs (lo[2]) < 1e-3f, "macro -> finish");
        const auto rel = at (0.5f, 0.5f, 1.0f);   // quick.reverb +0.5 offset on top of the (neutral) knob: 0.35 -> up to 0.7
        check (std::abs (rel[0] - 0.7f) < 2e-3f, "macro on a MAIN knob (relative): reverb " + juce::String (rel[0], 3));
    }
    // LFO sync accuracy at 120 / 140 BPM (host transport), straight and triplet
    for (auto [bpm, div] : { std::pair<double, const char*> { 120.0, "1/4" }, { 140.0, "1/4" }, { 140.0, "1/8T" }, { 120.0, "1/16D" } })
    {
        auto p = base();
        p.mod.lfo[0] = recipe::lfo (VaultLfo::saw).sync (div).free().to ("volume", 0.3f);
        Rig r; r.load (p);
        float worst = 0;
        const double cyc = vault::divisionBeats (vault::divisionIndex (div));
        r.run (8.0, chord ({ 60 }, 0.0, 7.9), true, bpm, [&] (int64_t pos)
        {
            if (pos == 0) return;
            // the UI value: the last chunk of the previous block (its start beat)
            const double beat = (double) (pos - 128) * bpm / 60.0 / srT;
            const double expect = beat / cyc - std::floor (beat / cyc);
            float d = std::abs ((float) expect - r.eng->lfoPhase (0));
            d = std::min (d, 1.0f - d);
            worst = std::max (worst, d);
        });
        check (worst < 1.0e-4f, juce::String ("LFO ") + div + " at " + juce::String ((int) bpm) + " BPM follows the host beat (worst phase error " + juce::String (worst, 6) + ")");
    }
    {   // free-running rate
        auto p = base();
        p.mod.lfo[1] = recipe::lfo (VaultLfo::sine).rate (2.5f).free().to ("pan", 0.2f);
        Rig r; r.load (p);
        r.run (3.0);
        const double expect = 2.5 * (double) (r.pos - 128) / srT;   // the phase at the start of the last chunk
        float d = std::abs ((float) (expect - std::floor (expect)) - r.eng->lfoPhase (1));
        d = std::min (d, 1.0f - d);
        check (d < 2.0e-3f, "free LFO rate 2.5 Hz (phase error " + juce::String (d, 5) + ")");
    }
    // LFO on volume makes a tremolo at the synced rate (audio)
    {
        auto p = base();
        p.mod.lfo[0] = recipe::lfo (VaultLfo::sine).sync ("1/8").free().to ("volume", 0.8f);
        Rig r; r.load (p);
        const auto b = r.run (4.0, chord ({ 60 }, 0.0, 3.9), true, 120.0);
        // envelope minima every 1/8 at 120 BPM = 0.25 s
        std::vector<int> minima;
        const int fr = 240;
        std::vector<float> env;
        for (int i = 0; i + fr < b.getNumSamples(); i += fr) env.push_back (rms (b, i, i + fr));
        for (size_t i = 2; i + 2 < env.size(); ++i)
            if (env[i] < env[i - 1] && env[i] <= env[i + 1] && env[i] < env[i - 2] && env[i] <= env[i + 2] && i * fr > srT * 0.5) minima.push_back ((int) (i * fr));
        double avg = 0;
        for (size_t i = 1; i < minima.size(); ++i) avg += minima[i] - minima[i - 1];
        avg = minima.size() > 1 ? avg / (double) (minima.size() - 1) / srT : 0;
        check (minima.size() >= 10 && std::abs (avg - 0.25) < 0.003, "LFO -> volume tremolo period " + juce::String (avg, 4) + " s (1/8 at 120 BPM = 0.25 s)");
    }
}

void arpGateTests()
{
    std::printf ("-- arpeggiator, gate\n");
    auto arpRun = [] (VaultArp a, double bpm, bool host, std::vector<std::pair<int64_t, juce::MidiMessage>> ev, double seconds)
    {
        auto p = base(); p.arp = a; p.arp.on = true;
        Rig r; r.eng->setArpOn (true); r.load (p);
        r.run (seconds, ev, host, bpm);
        std::vector<VaultEngine::NoteLog> log (512);
        log.resize ((size_t) r.eng->readNoteLog (log.data(), 512));
        return log;
    };
    auto ons = [] (const std::vector<VaultEngine::NoteLog>& l) { std::vector<VaultEngine::NoteLog> o; for (auto& e : l) if (e.on) o.push_back (e); return o; };
    {   // 140 BPM, 1/16, Up, host playing from bar 1
        VaultArp a = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
        const auto log = arpRun (a, 140.0, true, chord ({ 60, 64, 67 }, 0.0, 3.0), 3.2);
        const auto on = ons (log);
        const double step = 0.25 * 60.0 / 140.0 * srT;
        int worst = 0; bool order = on.size() >= 20;
        const int seq[3] { 60, 64, 67 };
        for (size_t k = 0; k < on.size() && k < 40; ++k)
        {
            worst = std::max (worst, (int) std::abs ((double) on[k].sample - std::floor (k * step + 1e-6)));
            order &= on[k].note == seq[k % 3];
        }
        int gateErr = 0;
        for (auto& e : log) if (! e.on) { const double s = std::fmod ((double) e.sample, step); gateErr = std::max (gateErr, (int) std::abs (s - step * 0.5)); if (e.sample > 2.9 * srT) break; }
        check (order && worst <= 1, "arp 1/16 at 140 BPM: steps on the grid (worst " + juce::String (worst) + " samples), Up order, " + juce::String ((int) on.size()) + " notes");
        check (gateErr <= 2, "arp gate 50 %: notes end half a step later (error " + juce::String (gateErr) + " samples)");
    }
    {   // triplets + octaves + swing + Down / Up-Down / Chord
        VaultArp a = recipe::arp (VaultArp::up, "1/8T", 2, 0.8f);
        const auto on = ons (arpRun (a, 140.0, true, chord ({ 60, 64, 67 }, 0.0, 3.0), 3.0));
        const double step = (1.0 / 3.0) * 60.0 / 140.0 * srT;
        int worst = 0; bool ord = on.size() >= 12;
        const int seq[6] { 60, 64, 67, 72, 76, 79 };
        for (size_t k = 0; k < on.size() && k < 18; ++k) { worst = std::max (worst, (int) std::abs ((double) on[k].sample - std::floor (k * step + 1e-6))); ord &= on[k].note == seq[k % 6]; }
        check (ord && worst <= 1, "arp 1/8 triplets at 140 BPM, 2 octaves (worst " + juce::String (worst) + " samples)");
        VaultArp s = recipe::arp (VaultArp::up, "1/16", 1, 0.5f, 0.5f);
        const auto sw = ons (arpRun (s, 120.0, true, chord ({ 60 }, 0.0, 2.0), 2.0));
        const double st = 0.25 * 0.5 * srT;
        check (sw.size() > 6 && std::abs ((double) sw[1].sample - (st + 0.25 * st)) <= 1.5 && std::abs ((double) sw[2].sample - 2 * st) <= 1.5, "arp swing 50 %: every 2nd step later by 1/8 step");
        VaultArp d = recipe::arp (VaultArp::down, "1/16", 1);
        const auto dn = ons (arpRun (d, 120.0, true, chord ({ 60, 64, 67 }, 0.0, 1.0), 1.0));
        VaultArp ud = recipe::arp (VaultArp::upDown, "1/16", 1);
        const auto udn = ons (arpRun (ud, 120.0, true, chord ({ 60, 64, 67 }, 0.0, 1.0), 1.0));
        VaultArp ch = recipe::arp (VaultArp::chord, "1/8", 1);
        const auto chn = ons (arpRun (ch, 120.0, true, chord ({ 60, 64, 67 }, 0.0, 1.0), 1.0));
        check (dn.size() > 3 && dn[0].note == 67 && dn[1].note == 64 && dn[2].note == 60, "arp Down");
        check (udn.size() > 5 && udn[0].note == 60 && udn[1].note == 64 && udn[2].note == 67 && udn[3].note == 64 && udn[4].note == 60, "arp Up-Down (no repeated ends)");
        check (chn.size() >= 6 && chn[0].sample == chn[2].sample && chn[3].sample - chn[0].sample == (int64_t) std::floor (0.5 * 0.5 * srT + 1e-6), "arp Chord: all notes per 1/8 step");
    }
    {   // lanes: rest, tie, octave, velocity
        VaultArp a = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
        a.steps = 4; a.type = {}; a.type.fill (VaultArp::play);
        a.type[1] = VaultArp::rest; a.type[3] = VaultArp::tie; a.oct[2] = 1; a.vel[0] = 0.5f;
        const auto log = arpRun (a, 120.0, true, chord ({ 60 }, 0.0, 1.0, srT, 127), 1.0);
        const auto on = ons (log);
        const double st = 0.125 * srT;
        bool ok = on.size() >= 4 && on[0].sample == 0 && std::abs (on[0].velocity - 0.5f) < 0.02f && on[1].note == 72 && std::abs ((double) on[1].sample - 2 * st) <= 1
                  && std::abs ((double) on[2].sample - 4 * st) <= 1;
        int64_t offOf2 = -1;
        for (auto& e : log) if (! e.on && e.note == 72) { offOf2 = e.sample; break; }
        ok &= std::abs ((double) offOf2 - (2 * st + 1.5 * st)) <= 2;   // step 3 ties: lasts 1 + 0.5 steps
        check (ok, "arp lanes: rest, tie, octave, velocity");
    }
    {   // free running (host stopped): starts with the key; latch keeps playing after release
        VaultArp a = recipe::arp (VaultArp::up, "1/8", 1, 0.5f);
        a.latch = true;
        const auto on = ons (arpRun (a, 120.0, false, chord ({ 62 }, 0.3, 0.6), 2.0));
        check (on.size() >= 6 && on[0].sample == (int64_t) (0.3 * srT) && on.back().sample > (int64_t) (1.5 * srT), "arp free-running from the key, latch holds it");
    }
    {   // MIDI export: 2 bars of 1/16 over a triad = 32 notes, on the 960 PPQ grid
        VaultArp a = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
        const auto mf = vault::arpPatternToMidi (a, { { 60, 0.8f }, { 64, 0.8f }, { 67, 0.8f } }, 2.0, 140.0);
        int notes = 0, offGrid = 0;
        if (auto* tr = mf.getTrack (0))
            for (auto* e : *tr)
                if (e->message.isNoteOn()) { ++notes; offGrid += std::abs (std::fmod (e->message.getTimeStamp(), 240.0)) > 0.51 ? 1 : 0; }
        check (notes == 32 && offGrid == 0 && mf.getTimeFormat() == 960, "arp MIDI export: 32 notes in 2 bars, on the 1/16 grid (" + juce::String (notes) + ")");
    }
    {   // gate: 1/16 at 120 BPM, on / off, depth 1, smooth 0 -> 125 ms steps open / closed
        auto p = base();
        p.gate = recipe::gate ("1/16", { 1, 0 }, 1.0f, 0.0f, 1.0f, true);
        Rig r; r.eng->setGateOn (true); r.load (p);
        const auto b = r.run (2.5, chord ({ 60 }, 0.0, 2.4), true, 120.0);
        const int step = (int) (0.125 * srT);
        float openMin = 1, closedMax = 0;
        for (int k = 2; k < 18; ++k)
        {
            const float v = rms (b, k * step + step / 5, (k + 1) * step - step / 5);
            if (k % 2 == 0) openMin = std::min (openMin, v); else closedMax = std::max (closedMax, v);
        }
        check (openMin > 0.02f && closedMax < openMin * 0.01f, "gate 1/16 at 120 BPM: open steps sound, closed steps -40 dB (" + juce::String (juce::Decibels::gainToDecibels (closedMax / openMin), 1) + " dB)");
        // the switch fades (no click), mix 0.5 = half depth
        Rig r2; r2.load (p);
        const auto t = r2.run (2.0, chord ({ 60 }, 0.0, 1.9), true, 120.0, [&] (int64_t pos) { r2.eng->setGateOn (pos > (int64_t) (0.53 * srT)); });
        check (maxJump (t, (int) (0.5 * srT), (int) (0.6 * srT)) < maxJump (b) * 1.25f + 0.002f, "gate switch: no click");
        auto half = p; half.gate.mix = 0.5f;
        Rig r3; r3.eng->setGateOn (true); r3.load (half);
        const auto h = r3.run (1.0, chord ({ 60 }, 0.0, 0.95), true, 120.0);
        const float hc = rms (h, 3 * step + step / 5, 4 * step - step / 5), ho = rms (h, 2 * step + step / 5, 3 * step - step / 5);
        check (std::abs (hc / ho - 0.5f) < 0.08f, "gate mix 50 %: closed steps at half level (" + juce::String (hc / ho, 2) + ")");
        // swing
        auto sw = p; sw.gate.swing = 1.0f;
        Rig r4; r4.eng->setGateOn (true); r4.load (sw);
        const auto g = r4.run (1.0, chord ({ 60 }, 0.0, 0.95), true, 120.0);
        // with swing 1, step 1 (closed) starts at 1.5 steps: 1.1..1.4 steps still open
        check (rms (g, (int) (1.1 * step), (int) (1.4 * step)) > openMin * 0.5f && rms (g, (int) (1.6 * step), (int) (1.9 * step)) < openMin * 0.05f, "gate swing moves every 2nd step");
        // arp + gate with the host stopped: one free clock from the key (the arp's notes have gaps - they must not restart
        // the gate's pattern on every step): gate 1/16 open, open, closed, closed under a 1/16 arp
        auto ag = base();
        ag.arp = recipe::arp (VaultArp::up, "1/16", 1, 0.9f, 0.0f, true);
        ag.gate = recipe::gate ("1/16", { 1, 1, 0, 0 }, 1.0f, 0.0f, 1.0f, true);
        Rig r5; r5.eng->setArpOn (true); r5.eng->setGateOn (true); r5.load (ag);
        const int key = (int) (0.3013 * srT);
        const auto a5 = r5.run (2.6, chord ({ 60 }, 0.3013, 2.5), false, 120.0);
        float aOpen = 1, aClosed = 0;
        for (int k = 4; k < 16; ++k)
        {
            const float v = rms (a5, key + k * step + step / 5, key + (k + 1) * step - step / 5);
            if (k % 4 < 2) aOpen = std::min (aOpen, v); else aClosed = std::max (aClosed, v);
        }
        check (aOpen > 0.02f && aClosed < aOpen * 0.05f, "arp + gate, host stopped: both on one clock from the key - the gate pattern runs on under the arp's notes (open "
               + juce::String (aOpen, 3) + ", closed " + juce::String (aClosed, 4) + ")");
    }
}

// the arpeggiator alone, block by block like the engine runs it: keys split the blocks at their sample
struct ArpSim
{
    struct Key { int64_t at; int note; float vel; bool on; bool pedal = false; };
    struct E { int64_t sample; int note; float vel; bool on; };
    vault::Arp arp;
    double sr = srT, bpm = 120.0;
    int block = 256;
    std::vector<vault::Arp::Event> ev = std::vector<vault::Arp::Event> ((size_t) vault::Arp::maxEvents);
    std::vector<E> out;
    int64_t pos = 0;
    double barBeats = 4.0, loopStart = 0.0, loopEnd = 0.0;
    // clock (block start sample) -> { beat at that sample, host playing }
    std::function<std::pair<double, bool> (int64_t)> clock = [this] (int64_t p) { return std::make_pair ((double) p * bpm / 60.0 / sr, true); };
    explicit ArpSim (const VaultArp& a) { arp.setSettings (a); arp.reset(); }
    double bps() const { return bpm / 60.0 / sr; }
    void run (int64_t samples, std::vector<Key> keys = {})
    {
        std::stable_sort (keys.begin(), keys.end(), [] (const Key& x, const Key& y) { return x.at < y.at; });
        size_t ki = 0;
        const int64_t end = pos + samples;
        while (pos < end)
        {
            const int len = (int) std::min<int64_t> (block, end - pos);
            const auto [b0, host] = clock (pos);
            int seg = 0;
            auto proc = [&] (int upTo)
            {
                if (upTo <= seg) return;
                vault::Arp::Clock c; c.beat0 = b0 + bps() * seg; c.bps = bps(); c.hostPlaying = host; c.barStart = 0.0; c.barBeats = barBeats;
                if (host) { c.loopStart = loopStart; c.loopEnd = loopEnd; }
                const int n = arp.process (upTo - seg, c, ev.data());
                for (int i = 0; i < n; ++i) out.push_back ({ pos + seg + ev[(size_t) i].sample, ev[(size_t) i].note, ev[(size_t) i].velocity, ev[(size_t) i].on });
                seg = upTo;
            };
            while (ki < keys.size() && keys[ki].at < pos + len)
            {
                const auto& k = keys[ki++];
                proc ((int) std::max<int64_t> (0, k.at - pos));
                if (k.pedal) arp.setPedal (k.on);
                else if (k.on) arp.keyDown (k.note, k.vel);
                else arp.keyUp (k.note);
            }
            proc (len);
            pos += len;
        }
    }
    std::vector<E> ons (int64_t from = 0, int64_t to = INT64_MAX) const { std::vector<E> o; for (auto& e : out) if (e.on && e.sample >= from && e.sample < to) o.push_back (e); return o; }
    // notes still sounding (an on without its off)
    int hanging() const
    {
        int s[128] {};
        for (auto& e : out) s[e.note] += e.on ? 1 : -1;
        int h = 0;
        for (int v : s) h += v != 0 ? 1 : 0;
        return h;
    }
    bool balanced() const   // never two ons of a note without an off between, never an off of a silent note
    {
        bool on[128] {};
        for (auto& e : out) { if (e.on == on[e.note]) return false; on[e.note] = e.on; }
        return true;
    }
};
std::vector<ArpSim::Key> keysOf (std::initializer_list<int> notes, int64_t on, int64_t off, float vel = 0.8f)
{
    std::vector<ArpSim::Key> k;
    for (int n : notes) { k.push_back ({ on, n, vel, true }); if (off > on) k.push_back ({ off, n, 0.0f, false }); }
    return k;
}

void arpProTests()
{
    std::printf ("-- arpeggiator: timing, chord changes, transport, latch, export\n");
    // ---- note orders and octave modes (one cycle)
    {
        auto order = [] (int mode, int octs, int octMode, std::vector<int> played)
        {
            VaultArp a; a.mode = (uint8_t) mode; a.octaves = (uint8_t) octs; a.octMode = (uint8_t) octMode;
            std::vector<int> o (4096);
            o.resize ((size_t) vault::Arp::orderOf (a, played.data(), (int) played.size(), o.data(), (int) o.size()));
            return o;
        };
        using V = std::vector<int>;
        const V ceg { 64, 60, 67 };   // played E C G
        bool ok = order (VaultArp::up, 1, 0, ceg) == V { 60, 64, 67 } && order (VaultArp::down, 1, 0, ceg) == V { 67, 64, 60 }
               && order (VaultArp::upDown, 1, 0, ceg) == V { 60, 64, 67, 64 } && order (VaultArp::downUp, 1, 0, ceg) == V { 67, 64, 60, 64 }
               && order (VaultArp::upAndDown, 1, 0, ceg) == V { 60, 64, 67, 67, 64, 60 } && order (VaultArp::asPlayed, 1, 0, ceg) == V { 64, 60, 67 }
               && order (VaultArp::converge, 1, 0, { 60, 64, 67, 71 }) == V { 60, 71, 64, 67 } && order (VaultArp::diverge, 1, 0, { 60, 64, 67, 71 }) == V { 67, 64, 71, 60 }
               && order (VaultArp::thumb, 1, 0, ceg) == V { 60, 64, 60, 67 } && order (VaultArp::pinky, 1, 0, ceg) == V { 60, 67, 64, 67 };
        check (ok, "arp note orders: Up, Down, Up-Down, Down-Up, Up & Down, As Played, Converge, Diverge, Thumb, Pinky");
        const bool oct = order (VaultArp::up, 2, VaultArp::octUp, ceg) == V { 60, 64, 67, 72, 76, 79 }
                      && order (VaultArp::up, 2, VaultArp::octDown, ceg) == V { 48, 52, 55, 60, 64, 67 }
                      && order (VaultArp::down, 2, VaultArp::octUp, ceg) == V { 79, 76, 72, 67, 64, 60 }
                      && order (VaultArp::up, 3, VaultArp::octUpDown, ceg) == V { 60, 64, 67, 72, 76, 79, 84, 88, 91, 72, 76, 79 }
                      && order (VaultArp::up, 2, VaultArp::octAlternate, ceg) == V { 60, 72, 64, 76, 67, 79 }
                      && order (VaultArp::asPlayed, 2, VaultArp::octUp, ceg) == V { 64, 60, 67, 76, 72, 79 }
                      && order (VaultArp::up, 2, VaultArp::octUp, { 60, 72 }) == V { 60, 72, 84 };   // no repeated pitch
        check (oct, "arp octave modes: Up, Down, Up-Down passes, Alternate, As Played passes, no doubled pitches");
        // every key down, 4 octaves, every order / octave mode: the sequence fits, nothing hangs
        bool big = true;
        for (int mode = 0; mode < VaultArp::numModes; ++mode)
            for (int om = 0; om < VaultArp::numOctModes; ++om)
            {
                VaultArp a = recipe::arp ((VaultArp::Mode) mode, "1/64", 4, 0.5f);
                a.octMode = (uint8_t) om;
                ArpSim s (a);
                std::vector<ArpSim::Key> k;
                for (int n = 0; n < 128; ++n) { k.push_back ({ n, n, 0.8f, true }); k.push_back ({ 20000 + n, n, 0.0f, false }); }
                s.run (30000, k);
                big &= ! s.ons().empty() && s.balanced() && s.hanging() == 0;
            }
        check (big, "arp: all 128 keys x 4 octaves in every order / octave mode - fits, no hanging notes");
    }
    // ---- timing on the host grid at 90 / 140 / 170 BPM: straight, triplet, dotted, 1/32, 1/64T, swing 0 / 62.5 / 75 %
    {
        int worst = 0, runs = 0, bad = 0;
        juce::String badWhat;
        for (double bpm : { 90.0, 140.0, 170.0 })
            for (const char* rate : { "1/16", "1/8T", "1/16D", "1/4", "1/32", "1/64T" })
                for (float swing : { 0.0f, 0.5f, 1.0f })
                {
                    VaultArp a = recipe::arp (VaultArp::up, rate, 1, 0.5f, swing);
                    ArpSim s (a); s.bpm = bpm; s.block = 333;   // odd block size: steps fall anywhere in a block
                    s.run ((int64_t) (2.6 * srT), keysOf ({ 60 }, 0, (int64_t) (1.9 * srT)));   // + time for the last note to end
                    const double L = vault::divisionBeats (vault::divisionIndex (rate));
                    const auto on = s.ons();
                    int expectN = 0;
                    for (int64_t k = 0;; ++k) { const double b = vault::StepGrid::start (k, 0, L, swing); if (b / s.bps() >= 1.9 * srT) break; ++expectN; }
                    bool ok = (int) on.size() == expectN;
                    for (size_t k = 0; k < on.size(); ++k)
                    {
                        const double want = vault::StepGrid::start ((int64_t) k, 0, L, swing) / s.bps();
                        const int err = (int) std::abs ((double) on[k].sample - std::floor (want + 1e-6));
                        worst = std::max (worst, err); ok &= err <= 1;
                    }
                    ok &= s.balanced() && s.hanging() == 0;
                    ++runs;
                    if (! ok) { ++bad; badWhat << " " << rate << "@" << (int) bpm << "/sw" << swing << " (" << (int) on.size() << " vs " << expectN << ")"; }
                }
        check (bad == 0 && worst <= 1, "arp timing on the host grid, " + juce::String (runs) + " runs (90/140/170 BPM, straight / triplet / dotted, swing 0-75 %): worst "
                                         + juce::String (worst) + " samples" + badWhat);
    }
    // ---- the same through the engine (block 256), 1/8T + swing at 170 BPM
    {
        auto p = base(); p.arp = recipe::arp (VaultArp::up, "1/8T", 1, 0.5f, 0.6f, true);
        Rig r; r.eng->setArpOn (true); r.load (p);
        r.run (2.0, chord ({ 60 }, 0.0, 1.9), true, 170.0);
        std::vector<VaultEngine::NoteLog> log (512);
        log.resize ((size_t) r.eng->readNoteLog (log.data(), 512));
        const double bps = 170.0 / 60.0 / srT, L = 1.0 / 3.0;
        int k = 0, worst = 0; bool ok = true;
        for (auto& e : log) if (e.on) { const int err = (int) std::abs ((double) e.sample - std::floor (vault::StepGrid::start (k++, 0, L, 0.6f) / bps + 1e-6)); worst = std::max (worst, err); ok &= err <= 1; }
        check (ok && k > 10, "engine arp: 1/8T swing 65 % at 170 BPM on the grid (worst " + juce::String (worst) + " samples, " + juce::String (k) + " notes)");
    }
    // ---- free clock (host stopped): from the key, dotted rate at 90 BPM
    {
        VaultArp a = recipe::arp (VaultArp::up, "1/16D", 1, 0.5f);
        ArpSim s (a); s.bpm = 90.0;
        s.clock = [&] (int64_t p) { return std::make_pair (3.3 + (double) p * s.bps(), false); };
        const int64_t key = 12345;
        s.run ((int64_t) (2.0 * srT), keysOf ({ 60, 64 }, key, (int64_t) (1.8 * srT)));
        const auto on = s.ons();
        const double step = 0.375 / s.bps();
        int worst = 0;
        for (size_t k = 0; k < on.size(); ++k) worst = std::max (worst, (int) std::abs ((double) (on[k].sample - key) - std::floor (k * step + 1e-6)));
        check (on.size() == 7 && on[0].sample == key && worst <= 1 && s.hanging() == 0, "arp free clock: starts on the key, 1/16 dotted at 90 BPM (worst " + juce::String (worst) + ")");
    }
    // ---- chord change on a grid line through the engine, the new chord's ons BEFORE the old chord's offs (as some hosts
    //      send them): the engine orders offs -> ons on one sample, so it is a clean new chord
    for (int retrig : { (int) VaultArp::retrigChord, (int) VaultArp::retrigOff })
    {
        auto p = base(); p.arp = recipe::arp (VaultArp::up, "1/16", 1, 0.9f, 0.0f, true);
        p.arp.retrig = (uint8_t) retrig;
        Rig r; r.eng->setArpOn (true); r.load (p);
        const int64_t change = (int64_t) (1.0 * srT);   // beat 2 at 120 BPM: a step line
        std::vector<std::pair<int64_t, juce::MidiMessage>> ev;
        for (int n : { 60, 64, 67 }) ev.push_back ({ 0, juce::MidiMessage::noteOn (1, n, (juce::uint8) 100) });
        for (int n : { 65, 69, 72 }) ev.push_back ({ change, juce::MidiMessage::noteOn (1, n, (juce::uint8) 100) });
        for (int n : { 60, 64, 67 }) ev.push_back ({ change, juce::MidiMessage::noteOff (1, n) });
        for (int n : { 65, 69, 72 }) ev.push_back ({ (int64_t) (2.0 * srT), juce::MidiMessage::noteOff (1, n) });
        r.run (2.3, ev, true, 120.0);
        std::vector<VaultEngine::NoteLog> log (512);
        log.resize ((size_t) r.eng->readNoteLog (log.data(), 512));
        std::vector<VaultEngine::NoteLog> on;
        int bal[128] {}; bool twice = false;
        for (auto& e : log) { if (e.on) on.push_back (e); bal[e.note] += e.on ? 1 : -1; twice |= bal[e.note] > 1 || bal[e.note] < 0; }
        int hang = 0; for (int x : bal) hang += x != 0 ? 1 : 0;
        const double step = 0.125 * srT;
        bool oneEach = true, noOld = true;
        for (size_t i = 0; i < on.size(); ++i)
        {
            oneEach &= std::abs ((double) on[i].sample - std::floor ((double) i * step + 1e-6)) <= 1;   // one note per step line: none missing, no double
            if (on[i].sample >= change) noOld &= on[i].note == 65 || on[i].note == 69 || on[i].note == 72;
        }
        int firstNew = -1;
        for (auto& e : on) if (e.sample >= change) { firstNew = e.note; break; }
        // New chord: starts from its lowest note; Off: runs on (after the last note the next one above it, else the lowest)
        const bool startOk = retrig == VaultArp::retrigChord ? firstNew == 65 : firstNew >= 0;
        check (oneEach && noOld && startOk && hang == 0 && ! twice && on.size() == 16,
               juce::String ("engine arp: chord change on the grid line (ons before offs), retrigger ") + (retrig == VaultArp::retrigChord ? "New chord" : "Off")
               + ": one note per step, no old notes, no hanging notes (" + juce::String ((int) on.size()) + " notes, first new " + juce::String (firstNew) + ")");
    }
    // ---- legato chord change mid-step (the new chord overlaps the old one by 40 ms): runs on, never doubles
    {
        VaultArp a = recipe::arp (VaultArp::up, "1/8", 1, 1.0f);
        ArpSim s (a);
        auto k = keysOf ({ 60, 64, 67 }, 0, (int64_t) (1.04 * srT));
        auto k2 = keysOf ({ 62, 65, 69 }, (int64_t) (1.0 * srT), (int64_t) (2.0 * srT));
        k.insert (k.end(), k2.begin(), k2.end());
        s.run ((int64_t) (2.2 * srT), k);
        const auto on = s.ons();
        int64_t minGap = INT64_MAX;
        for (size_t i = 1; i < on.size(); ++i) minGap = std::min (minGap, on[i].sample - on[i - 1].sample);
        bool after = true;
        for (auto& e : on) if (e.sample > (int64_t) (1.05 * srT)) after &= e.note == 62 || e.note == 65 || e.note == 69;
        check (minGap >= (int64_t) (0.25 * srT) - 1 && after && s.balanced() && s.hanging() == 0, "arp legato chord change: steps stay one per 1/8, only the new notes after the old ones are up");
    }
    // ---- a chord played a little late on the host grid: the step line it missed plays at once; too late -> the next line
    {
        VaultArp a = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
        ArpSim s1 (a);
        const int64_t line = (int64_t) (0.5 * srT), late = line + (int64_t) (0.012 * srT);
        s1.run ((int64_t) (1.0 * srT), keysOf ({ 60 }, late, (int64_t) (0.9 * srT)));
        ArpSim s2 (a);
        const int64_t tooLate = line + (int64_t) (0.05 * srT);
        s2.run ((int64_t) (1.0 * srT), keysOf ({ 60 }, tooLate, (int64_t) (0.9 * srT)));
        const auto o1 = s1.ons(), o2 = s2.ons();
        check (! o1.empty() && o1[0].sample == late && o1.size() > 1 && std::abs ((double) o1[1].sample - (double) (line + (int64_t) (0.125 * srT))) <= 1
               && ! o2.empty() && std::abs ((double) o2[0].sample - (double) (line + (int64_t) (0.125 * srT))) <= 1,
               "arp on the host grid: a chord 12 ms late plays at once (then on the grid), 50 ms late waits for the next step");
    }
    // ---- Chord mode: keys a few ms apart (free clock) join the chord step instead of waiting a step
    {
        VaultArp a = recipe::arp (VaultArp::chord, "1/8", 1, 0.5f);
        ArpSim s (a);
        s.clock = [&] (int64_t p) { return std::make_pair (7.7 + (double) p * s.bps(), false); };
        std::vector<ArpSim::Key> k { { 1000, 60, 0.8f, true }, { 1000 + 240, 64, 0.8f, true }, { 1000 + 480, 67, 0.8f, true } };
        for (int n : { 60, 64, 67 }) k.push_back ({ (int64_t) (1.0 * srT), n, 0.0f, false });
        s.run ((int64_t) (1.2 * srT), k);
        const auto first = s.ons (0, 2000), second = s.ons (2000 + (int64_t) (0.2 * srT), 2000 + (int64_t) (0.3 * srT));
        check (first.size() == 3 && second.size() == 3 && second[0].sample == second[2].sample && s.hanging() == 0,
               "arp Chord: keys 5 / 10 ms after the first join its step at once, then step together");
    }
    // ---- transport: loop jumps (a host that splits the block at the loop end / one that does not but reports its loop
    //      points / one that reports neither), then stop -> the free clock; no hanging notes, no double triggers
    for (int variant = 0; variant < 3; ++variant)
    {
        VaultArp a = recipe::arp (VaultArp::upDown, "1/16", 2, 0.95f);
        ArpSim s (a);
        const double bps = s.bps();
        const int64_t loopLen = (int64_t) std::llround (4.0 / bps);   // one bar
        const int64_t stopAt = 3 * loopLen + 10000;
        s.block = variant == 0 ? 250 : 333;                            // 250 divides the loop: the blocks end on the loop end
        if (variant == 1) { s.loopStart = 0.0; s.loopEnd = 4.0; }
        s.clock = [&] (int64_t p) -> std::pair<double, bool>
        {
            if (p < stopAt) return { (double) (p % loopLen) * bps, true };   // the block that holds the wrap reports the old pass
            return { 9.0 + (double) (p - stopAt) * bps, false };
        };
        s.run (4 * loopLen, keysOf ({ 60, 64, 67 }, 0, 4 * loopLen - 2000));
        auto passNotes = [&] (int pass) { std::vector<int> n; for (auto& e : s.ons (pass * loopLen, (pass + 1) * loopLen)) n.push_back (e.note); return n; };
        const auto p1 = passNotes (0), p2 = passNotes (1), p3 = passNotes (2);
        bool starts = true;
        for (int pass : { 1, 2, 3 }) { const auto on = s.ons (pass * loopLen - 1, pass * loopLen + 2); starts &= on.size() == 1 && on[0].note == 60; }
        const auto all = s.ons();
        int64_t minGap = INT64_MAX;
        for (size_t i = 1; i < all.size(); ++i) minGap = std::min (minGap, all[i].sample - all[i - 1].sample);
        const juce::String name = variant == 0 ? "split blocks" : variant == 1 ? "a wrap inside a block, loop points known" : "a wrap inside a block, no loop points";
        if (variant < 2)
            check (p1.size() == 16 && p1 == p2 && p2 == p3 && starts, "arp loop (" + name + "): every pass plays the same 16 notes from the loop start, on the sample");
        check (s.balanced() && s.hanging() == 0 && minGap >= (int64_t) (0.125 * srT) - 2,
               "arp loop (" + name + ") + stop: no hanging notes, no double trigger (closest notes " + juce::String ((int) minGap) + " samples)");
        if (variant == 0)
        {
            const auto before = s.ons (stopAt - 7000, stopAt), after = s.ons (stopAt, stopAt + 7000);
            check (before.size() == 1 && after.size() == 1 && std::abs ((double) (after[0].sample - before[0].sample) - 0.125 * srT) <= 2,
                   "arp transport stop: the pattern carries on in the same rhythm on the free clock");
        }
    }
    // ---- latch: replace / add, the sustain pedal holds the keys
    {
        auto latchRun = [] (uint8_t mode)
        {
            VaultArp a = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
            a.latch = true; a.latchMode = mode;
            ArpSim s (a);
            auto k = keysOf ({ 60, 64, 67 }, 0, (int64_t) (0.3 * srT));
            auto k2 = keysOf ({ 62 }, (int64_t) (0.6 * srT), (int64_t) (0.7 * srT));
            k.insert (k.end(), k2.begin(), k2.end());
            s.run ((int64_t) (1.6 * srT), k);
            std::set<int> late;
            for (auto& e : s.ons ((int64_t) (1.0 * srT))) late.insert (e.note);
            return late;
        };
        check (latchRun (VaultArp::latchReplace) == std::set<int> { 62 } && latchRun (VaultArp::latchAdd) == std::set<int> { 60, 62, 64, 67 },
               "arp latch: a new chord replaces the held one (On) / joins it (Add), the pattern runs on after the keys are up");
        VaultArp a = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
        ArpSim s (a);
        auto k = keysOf ({ 60, 64 }, 0, (int64_t) (0.2 * srT));
        k.push_back ({ 100, 0, 0, true, true });
        k.push_back ({ (int64_t) (0.8 * srT), 0, 0, false, true });
        s.run ((int64_t) (1.5 * srT), k);
        check (s.ons ((int64_t) (0.5 * srT), (int64_t) (0.78 * srT)).size() >= 3 && s.ons ((int64_t) (0.95 * srT)).empty() && s.hanging() == 0,
               "arp + sustain pedal: released keys keep playing while the pedal is down, stop after it goes up");
    }
    // ---- velocity source, 32 steps, LENGTH, SLIDE, Every bar retrigger in 3/4
    {
        auto vel = [] (uint8_t m)
        {
            VaultArp a = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
            a.velMode = m; a.vel.fill (0.5f);
            ArpSim s (a);
            s.run ((int64_t) (0.3 * srT), keysOf ({ 60 }, 0, (int64_t) (0.25 * srT), 0.8f));
            return s.ons().empty() ? -1.0f : s.ons()[0].vel;
        };
        check (std::abs (vel (VaultArp::velPlayedPattern) - 0.4f) < 1e-3f && std::abs (vel (VaultArp::velPattern) - 0.5f) < 1e-3f && std::abs (vel (VaultArp::velPlayed) - 0.8f) < 1e-3f,
               "arp velocity: played x steps / steps / played");
        VaultArp a = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
        a.steps = 32; a.type[20] = VaultArp::rest; a.oct[25] = 1; a.len[3] = 0.5f; a.slide[5] = 1;
        ArpSim s (a);
        s.run ((int64_t) (8.3 * srT), keysOf ({ 60 }, 0, (int64_t) (8.2 * srT)));
        const auto on = s.ons();
        const double st = 0.125 * srT;
        auto onAt = [&] (int step) { for (auto& e : on) if (std::abs ((double) e.sample - step * st) <= 1) return e.note; return -1; };
        auto offAfter = [&] (int step, int note) { for (auto& e : s.out) if (! e.on && e.note == note && e.sample > (int64_t) (step * st)) return (double) e.sample - step * st; return -1.0; };
        const bool lanes = onAt (20) == -1 && onAt (52) == -1 && onAt (25) == 72 && onAt (57) == 72 && onAt (21) == 60 && onAt (32) == 60;
        const double l3 = offAfter (3, 60), l5 = offAfter (5, 60);
        check (lanes && std::abs (l3 - 0.25 * st) <= 2 && l5 >= st && l5 <= 1.3 * st && s.balanced(),
               "arp 32 steps: a rest at 21, an octave at 26 every 2 bars; LENGTH 50 % x gate 50 % = 1/4 step; SLIDE overlaps the next note");
        VaultArp b = recipe::arp (VaultArp::up, "1/16", 1, 0.5f);
        b.retrig = VaultArp::retrigBar; b.steps = 16; b.oct[0] = 1;   // lane 1 = an octave up: marks where the lanes start
        ArpSim t (b); t.barBeats = 3.0;
        t.run ((int64_t) (3.2 * srT), keysOf ({ 60, 64, 67 }, 0, (int64_t) (3.1 * srT)));
        bool barOk = true;
        for (int bar = 0; bar < 2; ++bar)
        {
            const auto o = t.ons ((int64_t) (bar * 1.5 * srT), (int64_t) (bar * 1.5 * srT) + 5);   // 3 beats at 120 = 1.5 s
            barOk &= o.size() == 1 && o[0].note == 72;   // lane 1 (+1 octave) on the lowest note
        }
        check (barOk && t.hanging() == 0, "arp Every bar (3/4): the lanes and the order start again on each bar line");
    }
    // ---- rate change while playing (free clock): no double trigger, the due step still comes
    {
        VaultArp a = recipe::arp (VaultArp::up, "1/8", 1, 0.5f);
        ArpSim s (a);
        s.clock = [&] (int64_t p) { return std::make_pair (7.7 + (double) p * s.bps(), false); };
        s.run ((int64_t) (0.6 * srT), keysOf ({ 60, 64 }, 0, (int64_t) (1.5 * srT)));
        a.rate = (uint8_t) vault::divisionIndex ("1/32");
        s.arp.setSettings (a);
        s.run ((int64_t) (1.0 * srT));
        const auto on = s.ons();
        int64_t minGap = INT64_MAX;
        for (size_t i = 1; i < on.size(); ++i) minGap = std::min (minGap, on[i].sample - on[i - 1].sample);
        check (minGap >= (int64_t) (0.0625 * srT) - 1 && s.hanging() == 0, "arp rate change while playing: no double trigger, no hanging note");
    }
    // ---- engine: arp off / preset switch / all-notes-off while notes sound -> nothing hangs
    {
        auto p = base(); p.arp = recipe::arp (VaultArp::up, "1/16", 2, 1.0f, 0.0f, true);
        auto q = base ("t3.other"); q.arp = recipe::arp (VaultArp::chord, "1/8T", 1, 1.0f, 0.0f, true);
        Rig r; r.eng->setArpOn (true); r.load (p);
        r.run (0.4, chord ({ 60, 64, 67 }, 0.0, 0.0), true);
        r.load (q, false);                 // preset switch while the arp plays
        r.run (0.4, {}, true);
        r.eng->setArpOn (false);           // arp off: the keys still down play
        r.run (0.2, {}, true);
        r.eng->setArpOn (true);
        r.run (0.3, {}, true);
        const int64_t p0 = r.pos;
        std::vector<std::pair<int64_t, juce::MidiMessage>> offs;
        for (int n : { 60, 64, 67 }) offs.push_back ({ p0 + 10, juce::MidiMessage::noteOff (1, n) });
        r.run (0.5, offs, false);          // transport stops, keys up
        std::vector<VaultEngine::NoteLog> log (512);
        log.resize ((size_t) r.eng->readNoteLog (log.data(), 512));
        int bal[128] {}; bool twice = false;
        for (auto& e : log) { bal[e.note] += e.on ? 1 : -1; twice |= bal[e.note] > 1; }
        int hang = 0; for (int b : bal) hang += b != 0 ? 1 : 0;
        check (log.size() < 512 && hang == 0 && ! twice, "engine arp: preset switch, arp off / on, transport stop, keys up - every note ends (" + juce::String ((int) log.size()) + " events)");
        r.run (0.6);
        const auto tail = r.run (0.2);
        check (tail.getMagnitude (0, tail.getNumSamples()) < 1.0e-4f, "engine arp: silent after everything is up");
    }
    // ---- the on-screen keyboard (report item 13): VaultEngine::soundingNotes shows the arp's notes (latched too), short
    // notes between two UI frames still light once, nothing stays lit after the arp stops
    {
        auto p = base(); p.arp = recipe::arp (VaultArp::up, "1/64", 1, 0.1f, 0.0f, true);
        p.arp.latch = true;
        Rig r; r.eng->setArpOn (true); r.load (p);
        std::set<int> seen; bool foreign = false;
        int reads = 0;
        auto read = [&] (int64_t)
        {
            if (++reads % 8 != 0) return;   // ~ a 30 Hz UI timer at 256-sample blocks
            uint64_t m[2] {};
            r.eng->soundingNotes (m);
            for (int k = 0; k < 128; ++k)
                if ((m[k >> 6] >> (k & 63)) & 1u) { seen.insert (k); foreign |= k != 60 && k != 64 && k != 67; }
        };
        r.run (1.5, chord ({ 60, 64, 67 }, 0.05, 0.2), true, 170.0, read);   // keys up at 0.2 s: the latch keeps the pattern
        const bool latched = seen == std::set<int> { 60, 64, 67 } && ! foreign;
        seen.clear();
        r.run (0.5, {}, true, 170.0, read);
        const bool stillLatched = seen == std::set<int> { 60, 64, 67 };
        r.eng->setArpOn (false);
        r.run (0.1, {}, true, 170.0);
        uint64_t m[2] { 1, 1 };
        r.eng->soundingNotes (m);   // consumes the "started" bits of the run above
        r.eng->soundingNotes (m);
        const bool dark = m[0] == 0 && m[1] == 0 && ! r.eng->isNoteSounding (60);
        // without the arp: the played keys
        r.run (0.1, { { r.pos + 10, juce::MidiMessage::noteOn (1, 62, (juce::uint8) 100) } }, false);
        const bool direct = r.eng->isNoteSounding (62);
        r.eng->allNotesOff (false);
        r.eng->soundingNotes (m);
        r.eng->soundingNotes (m);
        check (latched && stillLatched && dark && direct && m[0] == 0 && m[1] == 0,
               "on-screen keyboard: soundingNotes shows the arp's 1/64 notes at 170 BPM (latched, keys up) and nothing after the arp / all notes stop");
    }
    // ---- processor bypass while the arp plays: nothing keeps playing afterwards
    {
        VaultProcessor p;
        p.prepareToPlay (48000.0, 256);
        p.editSound ([] (VaultPreset& s) { s.arp = recipe::arp (VaultArp::up, "1/16", 1, 0.8f); s.arp.latch = false; });
        p.setSwitch (VaultProcessor::idArpOn, true);
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer mb;
        for (int i = 0; i < 4; ++i) { mb.clear(); p.processBlock (buf, mb); }
        mb.clear(); mb.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 0); mb.addEvent (juce::MidiMessage::noteOn (1, 64, (juce::uint8) 100), 0);
        p.processBlock (buf, mb);
        for (int i = 0; i < 40; ++i) { mb.clear(); p.processBlock (buf, mb); }
        const bool playing = buf.getMagnitude (0, 256) > 1.0e-3f;
        const bool running = p.arpStep() >= 0;
        for (int i = 0; i < 20; ++i) { mb.clear(); p.processBlockBypassed (buf, mb); }   // the key-ups happen while bypassed
        std::vector<VaultEngine::NoteLog> log (512);
        const int before = p.audioEngine().readNoteLog (log.data(), 512);
        for (int i = 0; i < 200; ++i) { mb.clear(); p.processBlock (buf, mb); }
        const int after = p.audioEngine().readNoteLog (log.data(), 512);
        int newOns = 0;
        for (int i = before; i < after; ++i) newOns += log[(size_t) i].on ? 1 : 0;
        check (running && p.arpStep() < 0 && newOns == 0 && before < 512, "processor bypass while the arp plays: the arp stops, no new notes afterwards ("
                                                                            + juce::String (playing ? "was audible" : "was silent") + ")");
    }
    // ---- MIDI export = what the engine plays (host from bar 1): order, swing, ties, slide, octaves, velocity mode
    {
        VaultArp a = recipe::arp (VaultArp::asPlayed, "1/16", 2, 0.6f, 0.4f, true);
        a.octMode = VaultArp::octAlternate; a.velMode = VaultArp::velPattern; a.steps = 12;
        a.type[3] = VaultArp::tie; a.type[6] = VaultArp::rest; a.slide[8] = 1; a.len[1] = 0.4f; a.oct[10] = -1;
        for (int i = 0; i < vault::numSteps; ++i) a.vel[(size_t) i] = 0.4f + 0.05f * (float) (i % 12);
        const double bpm = 128.0;
        const std::vector<std::pair<int, float>> held { { 67, 0.7f }, { 60, 0.7f }, { 64, 0.7f } };
        const auto mf = vault::arpPatternToMidi (a, held, 2.0, bpm);
        struct N { double tick; int note; int vel; bool on; };
        std::vector<N> fileEv;
        if (auto* tr = mf.getTrack (0))
            for (auto* e : *tr)
                if (e->message.isNoteOn()) fileEv.push_back ({ e->message.getTimeStamp(), e->message.getNoteNumber(), e->message.getVelocity(), true });
        auto p = base(); p.arp = a;
        Rig r; r.eng->setArpOn (true); r.load (p);
        std::vector<std::pair<int64_t, juce::MidiMessage>> ev;
        for (auto& [k, v] : held) ev.push_back ({ 0, juce::MidiMessage::noteOn (1, k, v) });
        const double secs = 2.0 * 4.0 * 60.0 / bpm;
        r.run (secs, ev, true, bpm);
        std::vector<VaultEngine::NoteLog> log (512);
        log.resize ((size_t) r.eng->readNoteLog (log.data(), 512));
        std::vector<N> engEv;
        const double ticksPerSample = bpm / 60.0 / srT * 960.0;
        for (auto& e : log) if (e.on && e.sample < (int64_t) std::floor (secs * srT)) engEv.push_back ({ (double) e.sample * ticksPerSample, e.note, juce::jlimit (1, 127, juce::roundToInt (e.velocity * 127.0f)), true });
        bool same = engEv.size() == fileEv.size() && ! fileEv.empty();
        double worstTick = 0;
        for (size_t i = 0; same && i < fileEv.size(); ++i)
        {
            worstTick = std::max (worstTick, std::abs (engEv[i].tick - fileEv[i].tick));
            same &= engEv[i].note == fileEv[i].note && std::abs (engEv[i].vel - fileEv[i].vel) <= 1 && std::abs (engEv[i].tick - fileEv[i].tick) <= 1.0;
        }
        check (same, "arp MIDI export = the engine's notes over 2 bars (" + juce::String ((int) fileEv.size()) + " notes vs " + juce::String ((int) engEv.size())
                     + ", worst " + juce::String (worstTick, 2) + " ticks)");
    }
    // ---- the gate on the arp's step clock: swing 75 % -> open 1.5 steps, closed 0.5 step
    {
        auto p = base();
        p.gate = recipe::gate ("1/16", { 1, 0 }, 1.0f, 0.0f, 1.0f, true); p.gate.swing = 1.0f;
        Rig r; r.eng->setGateOn (true); r.load (p);
        const auto b = r.run (2.0, chord ({ 60 }, 0.0, 1.9), true, 120.0);
        const double L = 0.125 * srT;
        float openMin = 1, closedMax = 0;
        for (int k = 2; k <= 12; k += 2)
        {
            openMin = std::min (openMin, rms (b, (int) (k * L + 0.15 * L), (int) (k * L + 1.35 * L)));
            closedMax = std::max (closedMax, rms (b, (int) (k * L + 1.62 * L), (int) (k * L + 1.9 * L)));
        }
        check (openMin > 0.02f && closedMax < openMin * 0.05f, "gate swing 75 % on the shared step clock: open 1.5 steps, closed 0.5 (open " + juce::String (openMin, 3)
                                                              + ", closed " + juce::String (closedMax, 4) + ")");
    }
    // ---- CVB v4: the new arp fields round trip; the shipped v3 bank reads with the defaults
    {
        auto p = base ("t3.arp4");
        p.arp = recipe::arp (VaultArp::pinky, "1/64T", 3, 0.3f, 0.2f, true);
        p.arp.octMode = VaultArp::octUpDown; p.arp.velMode = VaultArp::velPlayed; p.arp.retrig = VaultArp::retrigBar; p.arp.latch = true;
        p.arp.latchMode = VaultArp::latchAdd; p.arp.steps = 32; p.arp.len[31] = 0.25f; p.arp.slide[30] = 1; p.arp.vel[29] = 0.3f; p.arp.type[28] = VaultArp::tie;
        p.gate.steps = 32; p.gate.level[31] = 0.5f;
        juce::String why;
        const bool valid = vault::validatePreset (p, &why);
        const auto mb = vault::serializeBank ({ p }, true);
        std::vector<VaultPreset> back;
        int ver = 0;
        check (valid && vault::deserializeBank (mb.getData(), mb.getSize(), back, nullptr, &ver) && back.size() == 1 && back[0] == p && ver == 4,
               "CVB v4: modes, 32-step lanes, LENGTH / SLIDE round trip " + why);
        juce::MemoryBlock fb;
        std::vector<VaultPreset> bank;
        int fver = 0;
        const auto f = juce::File::getCurrentWorkingDirectory().getChildFile ("resources/vault/factory.cvb");
        if (f.loadFileAsData (fb) && vault::deserializeBank (fb.getData(), fb.getSize(), bank, nullptr, &fver) && fver == 3)
        {
            bool ok = ! bank.empty();
            for (auto& x : bank) ok &= x.arp.retrig == VaultArp::retrigChord && x.arp.len[20] == 1.0f && x.gate.level.size() == (size_t) vault::numSteps && vault::validatePreset (x);
            check (ok, "the v3 factory bank still reads (new arp fields at their defaults, every preset valid)");
        }
    }
}

void lockAndStateTests()
{
    std::printf ("-- locks, plugin state, editing\n");
    {   // engine: FX lock keeps the chain over a preset switch
        auto a = base ("t3.a"); a.fx = { recipe::fx::chorus (0.4f), recipe::fx::reverb (recipe::fx::Reverb::plate, 0.6f, 1.0f, 0.4f) };
        a.arp = recipe::arp (VaultArp::down, "1/8T", 3);
        auto b = base ("t3.b"); b.fx = { recipe::fx::delay ("1/4", 0.4f, 0.3f) };
        Rig r; r.load (a);
        r.run (0.2, chord ({ 60 }, 0.0, 0.0));
        r.eng->setLocks (true, false, false);
        r.load (b, false);
        r.run (0.3);
        check (r.entryOf (vault::FxType::reverb) >= 0 && r.entryOf (vault::FxType::chorus) >= 0 && r.entryOf (vault::FxType::delay) < 0,
               "FX lock: the next preset keeps the chain");
        r.eng->setLocks (false, false, false);
        r.load (a, false); r.run (0.2);
        r.load (b, false); r.run (0.3);
        check (r.entryOf (vault::FxType::delay) >= 0 && r.entryOf (vault::FxType::reverb) < 0, "unlocked: the preset's own chain");
        const auto out = r.run (0.5, chord ({ 64 }, 0.0, 0.0));
        check (finite (out), "switching with locks stays finite");
    }
    {   // processor: locks, MAIN knobs, switches, edits; state save / restore of every new parameter
        VaultProcessor p;
        p.prepareToPlay (48000.0, 256);
        const int n = (int) p.bank().size();
        p.selectPreset (std::min (3, n - 1));
        p.setLock (VaultProcessor::Section::arp, true);
        p.editSound ([] (VaultPreset& s)
        {
            s.fx.insert (s.fx.begin(), recipe::fx::comp (-24, 4));
            s.macros[3] = recipe::macro ("Sweep").to ("fx.reverb.mix", 0.0f, 0.5f);
            s.mod.lfo[0] = recipe::lfo (VaultLfo::triangle).sync ("1/2").to ("cutoff", 0.3f);
            s.arp = recipe::arp (VaultArp::random, "1/8D", 3, 0.4f, 0.1f);
            s.gate = recipe::gate ("1/32", { 1, 0.5f, 0, 1 }, 0.7f);
        });
        const auto edited = p.currentSound();
        check (p.isSoundEdited() && p.macroName (3) == "Sweep", "editSound changes the current sound");
        p.selectPreset (std::min (5, n - 1));
        check (p.currentSound().arp == edited.arp && ! (p.currentSound().gate == edited.gate), "arp lock: the next preset keeps the edited arp, not the gate");
        p.setQuick (vault::qCutoff, 0.3f); p.setQuick (vault::qReverb, 0.8f); p.setQuick (vault::qVibrato, 0.4f); p.setQuick (vault::qPan, 0.6f);
        p.setSwitch (VaultProcessor::idArpOn, true); p.setSwitch (VaultProcessor::idFxOn, false); p.setSwitch (VaultProcessor::idGateOn, true);
        p.setLock (VaultProcessor::Section::fx, true);
        p.setQuickFxOn (vault::qDelay, true);
        p.editSound ([] (VaultPreset& s) { if (! s.parts.empty()) s.parts[0].level = 0.77f; });
        juce::MemoryBlock st;
        p.getStateInformation (st);
        VaultProcessor q;
        q.setStateInformation (st.getData(), (int) st.getSize());
        bool quickOk = true;
        for (int i = 0; i < vault::numQuick; ++i) quickOk &= std::abs (q.getQuick (i) - p.getQuick (i)) < 1e-4f;
        check (q.currentPreset() == p.currentPreset() && q.currentSound() == p.currentSound() && q.isSoundEdited(), "state: the edited sound comes back");
        check (quickOk && q.getSwitch (VaultProcessor::idArpOn) && ! q.getSwitch (VaultProcessor::idFxOn) && q.getSwitch (VaultProcessor::idGateOn)
                   && q.getLock (VaultProcessor::Section::fx) && q.getLock (VaultProcessor::Section::arp) && ! q.getLock (VaultProcessor::Section::gate),
               "state: MAIN knobs, switches, locks");
        check (q.fxSlotForQuick (vault::qDelay) >= 0 && q.isQuickFxOn (vault::qDelay), "MAIN delay LED added a delay slot");
        q.revertSound();
        check (! q.isSoundEdited() || q.getLock (VaultProcessor::Section::fx) || q.getLock (VaultProcessor::Section::arp), "revertSound");
        // the edit reaches the audio engine without a preset switch
        juce::AudioBuffer<float> buf (2, 256); juce::MidiBuffer mb;
        q.prepareToPlay (48000.0, 256);
        for (int i = 0; i < 8; ++i) q.processBlock (buf, mb);
        q.editSound ([] (VaultPreset& s) { s.fx = { recipe::fx::eq (3, 0, 0), recipe::fx::phaser (0.5f) }; });
        for (int i = 0; i < 4; ++i) q.processBlock (buf, mb);
        bool hasPhaser = false;
        for (int e = 0; e < q.audioEngine().chainSize(); ++e) hasPhaser |= q.audioEngine().chainType (e) == vault::FxType::phaser && q.audioEngine().chainSlot (e) == 1;
        check (hasPhaser, "editSound reaches the audio thread (chain updated in place)");
        const auto f = q.writeArpMidiFile (1.0);
        check (f.existsAsFile() && f.getSize() > 30, "arp MIDI drag file");
        f.deleteFile();
        auto* param = q.apvts.getParameter (VaultProcessor::idQuick[vault::qCutoff]);
        check (param != nullptr && param->isAutomatable() && q.apvts.getParameter (VaultProcessor::idGateOn) != nullptr, "MAIN knobs / switches are automatable parameters");
    }
}

void cpuAndAllocTests()
{
    std::printf ("-- CPU, allocation\n");
    // 4 synth layers (unison), a full 8-slot chain, 2 LFOs, a 4-note chord: 48 kHz / 256
    VaultPreset p = base ("t3.cpu");
    p.parts.clear();
    for (int k = 0; k < 4; ++k)
        p.parts.push_back (synth ({ { ID::engine, (float) (k % 3) }, { ID::unison, 4 }, { ID::detune, 0.4f }, { ID::sustain, 1 }, { ID::layerB, 1 },
                                    { ID::lfoFilter, 0.2f }, { ID::release, 0.5f } }, "L", 0.5f, -0.6f + 0.4f * k, k % 2));
    recipe::design (p)
        .fx ({ recipe::fx::comp (-18, 3), recipe::fx::drive (recipe::fx::Drive::tube, 0.3f), recipe::fx::chorus (0.3f, 0.4f, 0.5f, true), recipe::fx::phaser (0.3f),
               recipe::fx::eq (1, -2, 1, 0.5f), recipe::fx::filter (recipe::fx::Filter::lowPass, 5000, 0.2f, 0.3f, 0.3f), recipe::fx::delay ("1/8D", 0.4f, 0.25f),
               recipe::fx::reverb (recipe::fx::Reverb::hall, 0.7f, 1.0f, 0.3f) })
        .lfo (0, recipe::lfo (VaultLfo::sine).sync ("1/4").to ("cutoff", 0.4f).to ("fx.delay.mix", 0.1f))
        .lfo (1, recipe::lfo (VaultLfo::triangle).rate (0.3f).to ("pan", 0.3f).to ("fx.chorus.depth", 0.2f));
    check (vault::validatePreset (p) && p.fx.size() == 8, "CPU preset: 4 layers, 8 slots, 2 LFOs");
    Rig r (48000.0, 256);
    r.load (p);
    r.run (0.5, chord ({ 48, 55, 60, 64 }, 0.0, 0.0));
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    const auto b = r.run (10.0, {}, true, 128.0);
    const double ms = juce::Time::getMillisecondCounterHiRes() - t0;
    const double ratio = ms / 10000.0;
    check (ratio < 0.25 && finite (b), "CPU: " + juce::String (ratio * 100.0, 1) + " % of real time (budget 25 %)");
    // no allocation on the audio thread: arp, gate, LFOs, macros, quick knobs, edits, FX switch, a preset switch
    {
        auto q = richPreset();
        q.arp.on = true;
        const auto c1 = VaultEngine::compile (q), c2 = VaultEngine::compile (p);
        auto q2 = q; q2.fx[2].on = false; q2.mod.lfo[0].depth = 0.3f;
        const auto c3 = VaultEngine::compile (q2);
        VaultEngine e;
        e.prepare (48000.0, 256);
        e.setArpOn (true); e.setGateOn (true);
        e.setPreset (c1, true);
        juce::AudioBuffer<float> buf (2, 256);
        juce::MidiBuffer mb; mb.ensureSize (4096);
        VaultTransport t; t.playing = true; t.bpm = 133;
        allocHook::allocs = 0;
        for (int k = 0; k < 300; ++k)
        {
            mb.clear();
            if (k % 40 == 1) { mb.addEvent (juce::MidiMessage::noteOn (1, 60 + k % 7, (juce::uint8) 100), 3); mb.addEvent (juce::MidiMessage::noteOn (1, 67, (juce::uint8) 90), 9); }
            if (k % 40 == 30) { mb.addEvent (juce::MidiMessage::noteOff (1, 60 + (k - 29) % 7), 5); mb.addEvent (juce::MidiMessage::noteOff (1, 67), 6); }
            if (k % 13 == 0) mb.addEvent (juce::MidiMessage::controllerEvent (1, 1, k % 127), 10);
            if (k % 17 == 0) mb.addEvent (juce::MidiMessage::channelPressureChange (1, k % 127), 20);
            allocHook::countAllocs = true;
            if (k == 100) e.updateSound (c3);
            if (k == 150) e.setPreset (c2);
            if (k == 200) { e.setLocks (true, true, true); e.setPreset (c1); }
            if (k % 25 == 0) { e.setMacro (k / 25 % 4, (float) (k % 100) / 100.0f); e.setQuick (k / 25 % vault::numQuick, (float) (k % 90) / 100.0f); e.setFxOn (k % 50 != 0); }
            if (k == 250) e.setArpOn (false);
            e.process (buf, mb, t);
            allocHook::countAllocs = false;
            t.ppq += 256 * 133.0 / 60.0 / 48000.0;
        }
        check (allocHook::allocs.load() == 0, "v3 engine: no allocation on the audio thread (arp, gate, LFOs, macros, knobs, edits, locks, switches: "
               + juce::String (allocHook::allocs.load()) + ")");
    }
}

void recipeComfortTests()
{
    std::printf ("-- recipe API + C5 comfort with FX\n");
    // a designed v3 preset: the FX keep the C5 comfort rules (no harsh / piercing highs) for a gentle chain
    auto p = base ("t3.recipe");
    p.parts[0].values.push_back ({ ID::cutoff, 2500 });
    recipe::design (p).fx ({ recipe::fx::eq (1.0f, 0, -2.0f), recipe::fx::chorus (0.25f), recipe::fx::reverb (recipe::fx::Reverb::hall, 0.5f, 1.0f, 0.2f) })
        .macro (0, recipe::macro ("Body").to ("fx.eq.low", -3, 4), 0.5f);
    const auto b = VaultEngine::render (p, vault::homeNote, 0.8f, 2.0, srT);
    const float s25 = [&]
    {
        constexpr int order = 15, n = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<float> d (2 * n, 0.0f);
        for (int i = 0; i < n && i + 2400 < b.getNumSamples(); ++i) d[(size_t) i] = 0.5f * (b.getSample (0, 2400 + i) + b.getSample (1, 2400 + i)) * (0.5f - 0.5f * std::cos (6.2831853f * i / n));
        fft.performFrequencyOnlyForwardTransform (d.data());
        double tot = 0, band = 0;
        for (int i = 1; i < n / 2; ++i) { const double f = i * srT / n, pw = (double) d[(size_t) i] * d[(size_t) i]; tot += pw; if (f >= 2000 && f < 5000) band += pw; }
        return (float) (band / std::max (1e-20, tot));
    }();
    check (vault::validatePreset (p) && finite (b) && s25 < 0.10f, "recipe::design preset renders, 2-5 kHz share " + juce::String (s25 * 100, 1) + " % (gate limit 10 %)");
    {   // the example of docs/VAULT.md "v3 recipe API", verbatim
        using namespace recipe;   // in a recipe file
        VaultPreset p;            // id, name, category, parts as before (no FX keys needed)
        p.id = "cv.keys.0900"; p.name = "Warm Tine Room"; p.category = vault::catKeysOrgan;
        p.parts.push_back (modelPart ("ep.tine.mellow", "Tine Piano"));
        design (p)
            .fx ({ fx::comp (-20, 2.5f, 15, 120), fx::drive (fx::Drive::tape, 0.15f, 0.7f, 9000), fx::chorus (0.25f, 0.5f),
                   fx::eq (1.0f, 0, -2.0f), fx::delay ("1/8D", 0.25f, 0.10f), fx::reverb (fx::Reverb::room, 0.5f, 1.0f, 0.18f) })
            .finish (0.2f)
            .clearMacros()
            .macro (0, macro ("Bark").to ("fx.drive.amount", 0.0f, 0.6f).to ("fx.eq.mid", 0, 3), 0.25f)
            .macro (1, macro ("Space").to ("fx.reverb.mix", 0.05f, 0.45f).to ("fx.delay.mix", 0.0f, 0.3f), 0.3f)
            .macro (2, macro ("Tremolo").to ("lfo1.depth", 0, 1), 0.0f)
            .macro (3, macro ("Tone").to ("cutoff", -1.5f, 1.0f), 0.6f)
            .lfo (0, lfo (VaultLfo::sine).sync ("1/8T").to ("volume", 0.5f).to ("pan", 0.3f))
            .modWheel ("lfo1.depth", 1.0f)
            .velocity ("fx.drive.amount", 0.2f);
        juce::String why;
        const auto c = VaultEngine::compile (p);
        int targets = 0;
        for (int m = 0; m < 4; ++m) targets += c.ctl.numMacroTargets[(size_t) m];
        const auto r = VaultEngine::render (p, vault::homeNote, 0.8f, 2.0, srT);
        check (vault::validatePreset (p, &why) && targets == 6 && c.ctl.lfo[0].numDests == 2 && c.ctl.numVel == 1 && finite (r) && r.getMagnitude (0, r.getNumSamples()) > 0.01f,
               "recipe API: the docs example compiles, validates, resolves every target and plays " + why);
    }
}
} // namespace v3test

int vaultV3Tests()
{
    v3test::fails = 0;
    v3test::formatTests();
    v3test::evolveRackTests();
    v3test::conversionTests();
    v3test::fxTypeTests();
    v3test::quickAndMacroTests();
    v3test::arpGateTests();
    v3test::arpProTests();
    v3test::lockAndStateTests();
    v3test::cpuAndAllocTests();
    v3test::recipeComfortTests();
    return v3test::fails;
}
