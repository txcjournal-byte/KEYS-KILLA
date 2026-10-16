// CHROME VAULT pitch test (owner report item 0: "neighbouring keys play the same pitch").
//
//   VaultTests -pitch [options]
//     --bank <file.cvb>   the bank to check (default: CHROME_VAULT_BANK, else resources/vault/factory.cvb)
//     --only <text>       only presets whose id or name contains <text> (case-insensitive)
//     --step <n>          every n-th preset (quick runs)
//     --lo <key> --hi <key>  the key range (MIDI, default 24..96 = C2..C8 in FL Studio naming, C5 = MIDI 60: the owner's
//                         C2..C7 plus the octave above, so C2..C7 in scientific naming is covered too)
//     --threads <n>       worker threads (default: all cores)
//     --nofx              the FX chain off (the dry layers)
//     --csv <file>        one row per key: preset, key, level, f0 (YIN), confidence, interval to the key below
//     --wav <dir>         write every rendered key as <dir>/<preset id>_<key>.wav (with --only)
//     -v                  print every key of the failing presets
//
// Every preset renders every key on its own (fresh engine, the sound as designed: macros, arp / gate switches, FX), at
// 44.1 kHz, velocity 0.8, 1.2 s held, and is measured with tools/vault/PitchAnalysis.h (the window: the note after its
// attack while it is within 20 dB of its peak; a YIN fundamental; a log-frequency spectrum; the strongest spectral peak).
// A pair of neighbouring keys passes when the interval is one semitone within +-10 cents, measured by any of
//   * YIN: both keys have a confident YIN pitch (octave slips of the detector folded away),
//   * the spectrum: the cross-correlation of the two log spectra peaks at +100 cents - every partial moves by one
//     semitone (bells, noisy layers, chords and FX sounds),
//   * partial tracking: the strongest spectral peak moves by one semitone (octave folded; tines with sympathetic
//     ringing of the other tines, whose spectra do not line up as a whole),
//   * the YIN tracks frame by frame on the same window (pitch sweeps, sequences, vibrato synced to the note).
// Independent measurements agreeing on exactly one semitone by accident is unlikely; a key that plays the wrong
// pitch fails all three. A pair that measures ~0 cents is reported as "SAME PITCH". Keys under -70 dBFS are silent
// (reported, not checked: outside the instrument's playable range), so are keys where every layer plays below 32.7 Hz
// (C1 scientific: a bass transposed two octaves down, at C2..B3 - below what the analysis resolves and what the ear hears
// as pitch); a pair where no method finds a pitch (noise: whispers, wind, thunder) is reported as unpitched.
// The preset as a whole must play the played key's pitch class: when YIN finds one common tuning for most keys (>= 75 %
// agree within 25 cents, YIN confident on at least half the sounding keys), it must be within 30 cents of the key (mod
// octave) - a sequence centred a tritone away or a quarter tone off is consistent from key to key and still wrong.
// Informative: single keys whose YIN pitch is more than 25 cents off the preset's common tuning (inharmonic sounds
// mislead YIN, so this is a warning, not a failure).
#include "../Source/vault/VaultEngine.h"
#include "../Source/vault/VaultLibrary.h"
#include "../Source/vault/VaultTypes.h"
#include "../tools/vault/PitchAnalysis.h"
#include <juce_dsp/juce_dsp.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <set>
#include <thread>

namespace pitchtest
{
namespace
{
using namespace vaultpitch;
constexpr double sr = 44100.0;
constexpr int renderLen = (int) (1.2 * sr), slowRenderLen = (int) (3.0 * sr);   // slow attacks (swells, drones): 3 s
int noteVelocity = 102;   // --vel: the MIDI velocity of every test note (0.8 x 127)
thread_local bool gateOpen = false;   // render with the preset's gate switched off (the gated-sound fallback)

struct KeyResult : Analysis
{
    int key = 0;
    bool infrasonic = false;   // every layer plays below 32.7 Hz here (a bass two octaves down at C2..B3)
};

struct PairResult
{
    int key = 0;                 // the upper key of the pair
    float yinCents = 0, xcCents = 0, xcPeak = 0, pkCents = 0, trCents = 0, frCents = 0, f1Cents = 0;
    bool yinOk = false, xcOk = false, pkOk = false, trOk = false, frOk = false, f1Ok = false, evaluated = false, same = false, unpitched = false;
    juce::String layerInfo;      // -v: the layer-by-layer check
    bool settledOk = false;      // passes on the settled part of the note (0.1 s later)
    bool layersOk = false;       // the whole sound fails, every layer on its own passes (a layer enters / leaves at its range edge)
    bool heldOk = false;         // a short note too short / too low to measure passes with its sample layers' decay held
    bool gateOk = false;         // a gated sound passes with the gate open (an amplitude pattern, it cannot move a pitch)
    bool ok() const { return yinOk || xcOk || pkOk || trOk || frOk || f1Ok || settledOk || layersOk || heldOk || gateOk; }
};

struct PresetResult
{
    int index = 0;
    juce::String id, name, why, layers;
    float offKey = 0;          // cents: the whole preset plays this far from the played keys (YIN, mod octave), 0 = in tune
    int silentKeys = 0, infrasonicKeys = 0, unpitchedPairs = 0, failedPairs = 0, samePairs = 0, evaluated = 0, tuneOff = 0, layerPairs = 0, heldPairs = 0;
    std::vector<int> silent;
    std::vector<KeyResult> keys;
    std::vector<PairResult> pairs;
    juce::String seqFail;      // the key after its lower neighbour (touching notes) does not play the key's own pitch
    bool pass() const { return failedPairs == 0 && offKey == 0.0f && seqFail.isEmpty(); }
};

struct Options
{
    juce::File bank;
    juce::String only, csv, wavDir, resultsFile;
    std::set<juce::String> ids;   // --ids <file>: only these preset ids (one per line, e.g. the failures of a --results file)
    int step = 1, lo = 24, hi = 96, threads = 0;
    bool noFx = false, verbose = false, dump = false, library = false;
};

//==============================================================================
// one key in a fresh engine; prevKey >= 0: that key is played first for `prevLen` samples (released as the key starts, the
// way touching notes in a piano roll play; prevKey 128 = silence instead: the same timing without a previous note - free
// running LFOs / FX are where they would be) and the returned buffer starts at the key's note-on
juce::AudioBuffer<float> renderKey (const VaultCompiledPreset& c, int key, bool fxOn, int prevKey = -1, int prevLen = 0, int length = renderLen)
{
    auto eng = std::make_unique<VaultEngine>();
    for (int m = 0; m < vault::numMacros; ++m) eng->setMacro (m, c.macroDefaults[m]);
    eng->setArpOn (c.ctl.arp.on);
    eng->setGateOn (c.ctl.gate.on && ! gateOpen);
    eng->setFxOn (fxOn);
    constexpr int block = 512;
    eng->prepare (sr, block, std::max (1, c.numParts), false);
    eng->setPreset (c, true);
    eng->serviceResources();
    const int pre = prevKey >= 0 ? prevLen : 0, total = pre + length;
    juce::AudioBuffer<float> out (2, length), buf (2, block);
    for (int s0 = 0; s0 < total; s0 += block)
    {
        const int len = std::min (block, total - s0);
        juce::MidiBuffer midi;
        if (prevKey >= 0 && prevKey < 128 && s0 == 0) midi.addEvent (juce::MidiMessage::noteOn (1, prevKey, (juce::uint8) noteVelocity), 0);
        if (pre >= s0 && pre < s0 + len)
        {
            if (prevKey >= 0 && prevKey < 128) midi.addEvent (juce::MidiMessage::noteOff (1, prevKey), pre - s0);
            if (key >= 0) midi.addEvent (juce::MidiMessage::noteOn (1, key, (juce::uint8) noteVelocity), pre - s0);   // key -1: the first note's tail alone
        }
        buf.setSize (2, len, false, false, true);
        eng->process (buf, midi);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < len; ++i)
                if (s0 + i >= pre) out.setSample (ch, s0 + i - pre, buf.getSample (ch, i));
    }
    return out;
}

// one part of the preset on its own: the other layers muted (silent instrument / model, level 0, no level modulation)
VaultCompiledPreset soloPart (const VaultCompiledPreset& c, int keep)
{
    auto s = c;
    for (int k = 0; k < s.numParts; ++k)
    {
        if (k == keep) continue;
        auto& cp = s.parts[(size_t) k];
        cp.level = 0.0f;
        cp.instrument = nullptr;
        cp.model = -1;
    }
    auto mute = [&] (VaultCompiledTarget& t)
    {
        if (t.ref.kind == vault::TargetRef::layer && t.ref.a != keep && t.ref.b == vault::TargetRef::lLevel) { t.amount = 0; t.min = t.max = 0; }
    };
    for (auto& m : s.ctl.macro) for (auto& t : m) mute (t);
    for (auto& l : s.ctl.lfo) for (auto& t : l.dests) mute (t);
    for (auto& t : s.ctl.mw) mute (t);
    for (auto& t : s.ctl.vel) mute (t);
    for (auto& t : s.ctl.at) mute (t);
    return s;
}

// neighbouring keys a (below) and b: the interval by YIN, the spectrum, the strongest partial and the YIN tracks
// (vaultpitch::compareNotes, shared with the library tuning pass)
PairResult comparePair (const KeyResult& a, const KeyResult& b, const std::vector<float>& ma, const std::vector<float>& mb, double lowCutA, Analyser& an)
{
    PairResult pr;
    pr.key = b.key;
    const auto iv = compareNotes (a, b, mb.data(), (int) mb.size(), sr, 440.0 * std::pow (2.0, (b.key - 69) / 12.0), lowCutA, an);
    pr.yinCents = iv.yin; pr.yinOk = iv.yinOk;
    const bool second = iv.xc2Peak > iv.xcPeak && ! (std::abs (iv.xc - 100.0f) <= 10.0f);
    pr.xcCents = second ? iv.xc2 : iv.xc; pr.xcPeak = second ? iv.xc2Peak : iv.xcPeak; pr.xcOk = iv.xcOk;
    pr.pkCents = iv.pk; pr.pkOk = iv.pkOk;
    pr.trCents = iv.tr; pr.trOk = iv.trOk;
    pr.f1Cents = iv.f1 < 9000.0f ? iv.f1 : 0.0f; pr.f1Ok = iv.f1Ok;
    pr.unpitched = iv.unpitched; pr.evaluated = iv.evaluated; pr.same = iv.same;
    // sweeps: short frames on the same stretch of time (vaultpitch::frameShift), when nothing else agreed
    if (! pr.ok() && ! a.silent && ! b.silent)
    {
        float share = 0;
        pr.frCents = frameShift (ma.data(), (int) ma.size(), mb.data(), (int) mb.size(), a.from, a.len, sr, lowCutA, 100.0f, share);
        pr.frOk = share >= 0.5f && std::abs (pr.frCents - 100.0f) <= 10.0f;
        // the settled note: the same window 0.1 s later (a pluck's attack bends sharp and is the loudest part of the
        // window; the pitch it settles on is the note's pitch)
        if (! pr.frOk)
        {
            const int from = a.from + (int) (0.1 * sr), len = std::min (a.len - (int) (0.1 * sr), (int) std::min (ma.size(), mb.size()) - from);
            if (len >= 4096)
            {
                const double fa = 440.0 * std::pow (2.0, (a.key - 69) / 12.0), fb = 440.0 * std::pow (2.0, (b.key - 69) / 12.0);
                const auto aL = an.analyseAt (ma.data(), (int) ma.size(), sr, fa, from, len, 4.5, lowCutA);
                const auto bL = an.analyseAt (mb.data(), (int) mb.size(), sr, fb, from, len, 4.5, lowCutA);
                if (aL.silent || bL.silent || aL.levelDb <= a.levelDb - 26.0f) pr.layerInfo << " [settled: too quiet " << juce::String (aL.levelDb, 1) << " / " << juce::String (a.levelDb, 1) << "]";
                if (! aL.silent && ! bL.silent && aL.levelDb > a.levelDb - 26.0f)
                {
                    const auto iv2 = compareNotes (aL, bL, mb.data(), (int) mb.size(), sr, fb, lowCutA, an);
                    pr.settledOk = iv2.ok();
                    pr.layerInfo << " [settled yin " << juce::String (iv2.yin, 0) << " xc " << juce::String (iv2.xc, 0) << "/" << juce::String (iv2.xc2, 0)
                                 << " f1 " << juce::String (iv2.f1, 0) << "/" << juce::String (iv2.f1b, 0) << " tr " << juce::String (iv2.tr, 0) << "]";
                    if (pr.settledOk) { pr.unpitched = false; pr.evaluated = true; }
                }
            }
        }
        if (share >= 0.5f) { pr.unpitched = false; pr.evaluated = true; }
    }
    return pr;
}

PresetResult checkPreset (const VaultPreset& p, int index, const Options& o, Analyser& an)
{
    PresetResult res;
    res.index = index; res.id = p.id; res.name = p.name;
    for (auto& part : p.parts)
    {
        res.layers << (part.kind == VaultPartKind::sample ? "sample " : part.kind == VaultPartKind::model ? "model " : "synth ")
                   << part.source << (part.articulation.isNotEmpty() ? "/" + part.articulation : juce::String())
                   << " oct " << part.octave << " semi " << part.semi << " lvl " << juce::String (part.level, 2);
        if (o.verbose)
            for (auto& [k, v] : part.values) res.layers << " " << k << "=" << juce::String (v, 3);
        res.layers << "; ";
    }
    const auto lib = vault::SampleLibrary::getShared();
    const auto compiled = VaultEngine::compile (p, lib);
    // the instruments stay pinned while this preset is checked: the other worker threads load theirs meanwhile, and an
    // instrument no engine holds between two renders would otherwise be evicted (its keys then render silent and are
    // skipped as "outside the range")
    struct Pins
    {
        std::vector<vault::InstrumentHandle*> hs;
        vault::SampleLibrary* lib = nullptr;
        ~Pins() { if (lib != nullptr) for (auto* h : hs) lib->unpin (h); }
    } pins;
    pins.lib = compiled.library.get();
    for (int k = 0; k < compiled.numParts; ++k)
        if (auto* h = compiled.parts[(size_t) k].instrument; h != nullptr && compiled.library != nullptr)
        {
            compiled.library->pin (h);
            pins.hs.push_back (h);
            compiled.library->loadNow (h);
        }
    // the highest / lowest note any layer sounds at a key (a sample layer outside its instrument's range is silent there)
    // does layer `k` sound at `key` (a sample layer outside its instrument's range is silent there)
    auto layerSounds = [&] (int key, int k)
    {
        const auto& cp = compiled.parts[(size_t) k];
        const int note = key + cp.transpose;
        if (cp.kind != VaultPartKind::sample) return true;
        const auto* d = cp.instrument != nullptr ? cp.instrument->data() : nullptr;
        if (d == nullptr || ! juce::isPositiveAndBelow (note, 128) || ! juce::isPositiveAndBelow (cp.articulation, (int) d->articulations.size())) return false;
        const auto& A = d->articulations[(size_t) cp.articulation];
        return A.keyStart[(size_t) note] < A.keyStart[(size_t) note + 1];
    };
    // a synth layer's own oscillator transposition (layer A, and layer B when it is on): its lowest note
    auto synthShift = [&] (int k)
    {
        if (k >= (int) p.parts.size() || p.parts[(size_t) k].kind != VaultPartKind::synth) return 0;
        const auto& part = p.parts[(size_t) k];
        int lo = (int) part.get ("octave", 0) * 12 + (int) part.get ("semi", 0);
        if (part.get ("layerB", 0) > 0.5f) lo = std::min (lo, (int) part.get ("octaveB", 0) * 12 + (int) part.get ("semiB", 0));
        return lo;
    };
    // the highest / lowest note any sounding layer (or layer `only`, -1 = all) plays at a key
    auto layerNote = [&] (int key, bool highest, int only)
    {
        int best = highest ? -1000 : 1000;
        for (int k = 0; k < compiled.numParts; ++k)
        {
            if ((only >= 0 && k != only) || ! layerSounds (key, k)) continue;
            const int note = key + compiled.parts[(size_t) k].transpose + synthShift (k);
            best = highest ? std::max (best, note) : std::min (best, note);
        }
        return best;
    };
    auto highestNote = [&] (int key) { return layerNote (key, true, -1); };
    // the spectrum below 0.45 x the lowest layer's fundamental is left out: no partial of the note is there (sub
    // oscillators sit an octave down), only body knocks / thumps / rumble that do not move with the key
    auto lowCut = [&] (int key, int only) { const int n = layerNote (key, false, only); return n >= 1000 ? 0.0 : lowCutRatio * 440.0 * std::pow (2.0, (n - 69) / 12.0); };
    // the render length: 1.2 s, or 3 s for a sound still swelling at 1 s (slow attacks: the window must hold the note,
    // not its first rise) - judged on C5 (or the middle of the range)
    int len = renderLen;
    {
        const int k = juce::jlimit (o.lo, o.hi, 60);
        const auto b = renderKey (compiled, k, ! o.noFx);
        float early = 0, late = 0;
        for (int ch = 0; ch < 2; ++ch)
        {
            early = std::max (early, b.getRMSLevel (ch, 0, (int) (0.9 * sr)));
            late = std::max (late, b.getRMSLevel (ch, (int) (0.9 * sr), renderLen - (int) (0.9 * sr)));
        }
        if (late > early * 1.12f) len = slowRenderLen;
    }
    // one key of a (solo) sound rendered and measured
    auto measure = [&] (const VaultCompiledPreset& c, int key, int only, std::vector<float>& mono)
    {
        const auto b = renderKey (c, key, ! o.noFx, -1, 0, len);
        mono.resize ((size_t) b.getNumSamples());
        for (int i = 0; i < b.getNumSamples(); ++i) mono[(size_t) i] = 0.5f * (b.getSample (0, i) + b.getSample (1, i));
        return an.analyse (mono.data(), (int) mono.size(), sr, 440.0 * std::pow (2.0, (key - 69) / 12.0), 4.5, lowCut (key, only));
    };
    // the same sound with every sample / synth layer's decay envelope held (sustain 1): an amplitude envelope does not change a
    // pitch, so a short note (a decay of a fraction of a second, a damped / staccato variant) that is too short - or too low -
    // for the analysis to resolve +-10 cents is measured on the same zones held instead (the dry sustained preset of the
    // same instrument is checked on its own anyway). Only for pairs whose window is < 0.3 s or whose fundamental is < 65 Hz.
    std::unique_ptr<VaultCompiledPreset> held;
    {
        auto ph = p;
        bool any = false;
        for (auto& part : ph.parts)
        {
            if (part.kind == VaultPartKind::sample && part.get ("s.decay", 0.0f) > 0.0f && part.get ("s.sustain", 1.0f) < 0.99f)
            { part.set ("s.sustain", 1.0f); any = true; }
            // a synth layer's amplitude envelope, unless the mod matrix routes an envelope to its pitch
            if (part.kind == VaultPartKind::synth && part.get ("sustain", 1.0f) < 0.99f)
            {
                bool pitchRoute = false;
                for (int r = 1; r <= 8; ++r)
                    pitchRoute |= (int) part.get ("mmDst" + juce::String (r), -1.0f) == 0 && part.get ("mmAmt" + juce::String (r), 0.0f) != 0.0f;
                if (! pitchRoute) { part.set ("sustain", 1.0f); any = true; }
            }
        }
        if (any) held = std::make_unique<VaultCompiledPreset> (VaultEngine::compile (ph, lib));
    }
    std::vector<std::vector<float>> monos ((size_t) (o.hi - o.lo + 1));
    for (int key = o.lo; key <= o.hi; ++key)
    {
        const auto b = renderKey (compiled, key, ! o.noFx, -1, 0, len);
        if (o.wavDir.isNotEmpty())
        {
            const auto f = juce::File::getCurrentWorkingDirectory().getChildFile (o.wavDir).getChildFile (p.id + "_" + juce::String (key) + ".wav");
            f.getParentDirectory().createDirectory();
            f.deleteFile();
            if (auto os = f.createOutputStream())
            {
                juce::WavAudioFormat fmt;
                std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (os.release(), sr, 2, 24, {}, 0));
                if (w != nullptr) w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
            }
        }
        auto& mono = monos[(size_t) (key - o.lo)];
        mono.resize ((size_t) b.getNumSamples());
        for (int i = 0; i < b.getNumSamples(); ++i) mono[(size_t) i] = 0.5f * (b.getSample (0, i) + b.getSample (1, i));
        KeyResult kr;
        static_cast<Analysis&> (kr) = an.analyse (mono.data(), (int) mono.size(), sr, 440.0 * std::pow (2.0, (key - 69) / 12.0), 4.5, lowCut (key, -1));
        kr.key = key;
        if (const int hn = highestNote (key); hn > -1000 && hn < 24) { kr.infrasonic = true; kr.silent = true; ++res.infrasonicKeys; }
        else if (kr.silent) { ++res.silentKeys; res.silent.push_back (key); }
        res.keys.push_back (std::move (kr));
    }
    // neighbouring keys
    for (size_t i = 1; i < res.keys.size(); ++i)
    {
        const auto& a = res.keys[i - 1];
        const auto& b = res.keys[i];
        auto pr = comparePair (a, b, monos[(size_t) (a.key - o.lo)], monos[(size_t) (b.key - o.lo)], lowCut (a.key, -1), an);
        // a layered sound that fails as a whole: every layer on its own. A layer entering or leaving at the edge of its
        // instrument's range (a duet a third apart below the violin's lowest note), or the strongest partial changing
        // from one layer to the other, moves the measured "pitch" of the mix although every layer plays its key; a
        // layer that plays a wrong pitch fails on its own as well.
        if (pr.evaluated && ! pr.ok() && compiled.numParts > 1)
        {
            int checked = 0, failedLayers = 0;
            for (int part = 0; part < compiled.numParts; ++part)
            {
                if (! layerSounds (a.key, part) || ! layerSounds (b.key, part)) continue;
                const auto solo = soloPart (compiled, part);
                std::vector<float> ma, mb;
                KeyResult ka, kb;
                static_cast<Analysis&> (ka) = measure (solo, a.key, part, ma);
                static_cast<Analysis&> (kb) = measure (solo, b.key, part, mb);
                ka.key = a.key; kb.key = b.key;
                if (ka.silent || kb.silent) continue;
                const auto lp = comparePair (ka, kb, ma, mb, lowCut (a.key, part), an);
                pr.layerInfo << " [L" << (part + 1) << " yin " << juce::String (lp.yinCents, 0) << " xc " << juce::String (lp.xcCents, 0) << " pk "
                             << juce::String (lp.pkCents, 0) << " tr " << juce::String (lp.trCents, 0) << (lp.evaluated ? (lp.ok() ? " ok]" : " FAIL]") : " unpitched]");
                if (! lp.evaluated) continue;
                ++checked;
                if (! lp.ok()) ++failedLayers;
            }
            if (checked > 0 && failedLayers == 0) { pr.layersOk = true; ++res.layerPairs; }
        }
        if (pr.evaluated && ! pr.ok() && held != nullptr)
        {
            const int shortLen = (int) (0.3 * sr);
            const double fLow = 440.0 * std::pow (2.0, (layerNote (a.key, false, -1) - 69) / 12.0);
            if (a.len < shortLen || b.len < shortLen || fLow < 65.0)
            {
                std::vector<float> ma, mb;
                KeyResult ka, kb;
                static_cast<Analysis&> (ka) = measure (*held, a.key, -1, ma);
                static_cast<Analysis&> (kb) = measure (*held, b.key, -1, mb);
                ka.key = a.key; kb.key = b.key;
                if (! ka.silent && ! kb.silent)
                {
                    const auto hp = comparePair (ka, kb, ma, mb, lowCut (a.key, -1), an);
                    pr.heldOk = hp.evaluated && hp.ok();
                    pr.layerInfo << " [held yin " << juce::String (hp.yinCents, 0) << " xc " << juce::String (hp.xcCents, 0) << " tr "
                                 << juce::String (hp.trCents, 0) << (pr.heldOk ? " ok]" : " FAIL]");
                    if (pr.heldOk) ++res.heldPairs;
                }
            }
        }
        // a gated sound (the GATE chops the note into steps) whose pair fails: the same pair with the gate open - the gate is
        // an amplitude pattern; its steps leave windows too short to measure +-10 cents, it cannot change a pitch
        if (pr.evaluated && ! pr.ok() && compiled.ctl.gate.on)
        {
            gateOpen = true;
            std::vector<float> ma, mb;
            KeyResult ka, kb;
            static_cast<Analysis&> (ka) = measure (compiled, a.key, -1, ma);
            static_cast<Analysis&> (kb) = measure (compiled, b.key, -1, mb);
            gateOpen = false;
            ka.key = a.key; kb.key = b.key;
            if (! ka.silent && ! kb.silent)
            {
                const auto gp = comparePair (ka, kb, ma, mb, lowCut (a.key, -1), an);
                pr.gateOk = gp.evaluated && gp.ok();
                pr.layerInfo << " [gate open yin " << juce::String (gp.yinCents, 0) << " xc " << juce::String (gp.xcCents, 0) << (pr.gateOk ? " ok]" : " FAIL]");
            }
        }
        if (pr.evaluated)
        {
            ++res.evaluated;
            if (! pr.ok()) { ++res.failedPairs; res.samePairs += pr.same ? 1 : 0; }
        }
        else if (! a.silent && ! b.silent) ++res.unpitchedPairs;
        res.pairs.push_back (pr);
    }
    // the piano roll: C5 right after B4 (touching notes, FL Studio naming) must play C5's own pitch - glide, per-note
    // random pitch and voice reuse must not carry the previous note into the next one. C5 alone and C5 after B4 are
    // compared on the same window (relative: within 8 cents by any measurement, or the same pitch by YIN).
    {
        constexpr int prevKey = 59, key = 60, prevLen = (int) (0.35 * sr);
        if (key >= o.lo && key <= o.hi && prevKey >= o.lo)
        {
            const auto& alone = res.keys[(size_t) (key - o.lo)];
            const auto& before = res.keys[(size_t) (prevKey - o.lo)];
            if (! alone.silent && ! before.silent && ! alone.infrasonic)
            {
                auto toMono = [] (const juce::AudioBuffer<float>& b)
                {
                    std::vector<float> m ((size_t) b.getNumSamples());
                    for (int i = 0; i < b.getNumSamples(); ++i) m[(size_t) i] = 0.5f * (b.getSample (0, i) + b.getSample (1, i));
                    return m;
                };
                // the reference: the same timing with silence instead of B4
                const auto ma = toMono (renderKey (compiled, key, ! o.noFx, 128, prevLen, len));
                const auto mono = toMono (renderKey (compiled, key, ! o.noFx, prevKey, prevLen, len));
                if (o.wavDir.isNotEmpty())   // --wav: the two renders as <id>_seq_ref.wav / <id>_seq_after.wav (mono)
                    for (auto [m, tag] : { std::pair<const std::vector<float>*, const char*> { &ma, "ref" }, { &mono, "after" } })
                    {
                        const auto f = juce::File::getCurrentWorkingDirectory().getChildFile (o.wavDir).getChildFile (p.id + "_seq_" + tag + ".wav");
                        f.getParentDirectory().createDirectory();
                        f.deleteFile();
                        juce::AudioBuffer<float> wb (1, (int) m->size());
                        for (int i = 0; i < wb.getNumSamples(); ++i) wb.setSample (0, i, (*m)[(size_t) i]);
                        if (auto os = f.createOutputStream())
                        {
                            juce::WavAudioFormat fmt;
                            std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (os.release(), sr, 1, 24, {}, 0));
                            if (w != nullptr) w->writeFromAudioSampleBuffer (wb, 0, wb.getNumSamples());
                        }
                    }
                const double f = 440.0 * std::pow (2.0, (key - 69) / 12.0);
                const Analysis ref = an.analyseAt (ma.data(), (int) ma.size(), sr, f, alone.from, alone.len, 4.5, lowCut (key, -1));
                const Analysis after = an.analyseAt (mono.data(), (int) mono.size(), sr, f, alone.from, alone.len, 4.5, lowCut (key, -1));
                const auto iv = compareNotes (ref, after, mono.data(), (int) mono.size(), sr, f, lowCut (key, -1), an, 0.0f, 8.0f);
                float share = 0;
                const float fr = frameShift (ma.data(), (int) ma.size(), mono.data(), (int) mono.size(), alone.from, alone.len, sr, lowCut (key, -1), 0.0f, share);
                bool ok = iv.ok() || (share >= 0.5f && std::abs (fr) <= 8.0f) || ! iv.evaluated;
                // B4's own release tail (a pad releasing over 2 s, a reverb) sounds under C5 and pulls the measurement
                // toward B4 - that is two notes, not C5 playing a wrong pitch: B4 rendered alone (the same timing, no C5) is
                // subtracted and what is left must be C5's own pitch (a glide or a reused voice would still be off)
                if (! ok)
                {
                    const auto tail = toMono (renderKey (compiled, -1, ! o.noFx, prevKey, prevLen, len));
                    std::vector<float> rest (mono.size());
                    for (size_t i = 0; i < rest.size(); ++i) rest[i] = mono[i] - (i < tail.size() ? tail[i] : 0.0f);
                    const Analysis restA = an.analyseAt (rest.data(), (int) rest.size(), sr, f, alone.from, alone.len, 4.5, lowCut (key, -1));
                    const auto iv2 = compareNotes (ref, restA, rest.data(), (int) rest.size(), sr, f, lowCut (key, -1), an, 0.0f, 8.0f);
                    ok = iv2.evaluated && iv2.ok();
                }
                if (! ok)
                {
                    float c = 0; int votes = 0;
                    iv.consensus (0.0f, c, votes, 8.0f);
                    res.seqFail << "C5 after B4 plays " << juce::String (c, 0) << " cents off C5 alone (yin " << juce::String (iv.yin, 0) << " xc "
                                << juce::String (iv.xc, 0) << " frames " << juce::String (fr, 0) << "); ";
                }
            }
        }
    }
    // a YIN pitch is trusted for the absolute checks when the strongest partial agrees with it (mod octave): a strong
    // inharmonic partial (music box tines, bells) pulls YIN off by tens of cents while the fundamental is in tune
    auto trustedPitch = [] (const KeyResult& k)
    {
        return ! k.silent && k.confident && k.peakHz > 0.0f && std::abs (foldOctave (1200.0f * std::log2 (k.f0 / k.peakHz))) <= 30.0f;
    };
    // one tuning: the confident YIN pitches against the played keys (mod octave)
    std::vector<float> devs;
    for (auto& k : res.keys)
        if (trustedPitch (k))
            devs.push_back (foldOctave (1200.0f * std::log2 (k.f0 / (440.0f * std::pow (2.0f, (k.key - 69) / 12.0f)))));
    if (devs.size() >= 8)
    {
        // circular median: try every value as the centre, keep the one with the most values within 25 cents
        float centre = 0; int bestIn = -1;
        for (float c : devs)
        {
            int in = 0;
            for (float d : devs) in += std::abs (foldOctave (d - c)) <= 25.0f ? 1 : 0;
            if (in > bestIn) { bestIn = in; centre = c; }
        }
        // the common tuning (mean of the values around the centre) must be the played key's pitch class: a sound that
        // plays every key a tritone / a quarter tone away is consistent from key to key and still wrong
        {
            double sum = 0; int n = 0;
            for (float d : devs) if (std::abs (foldOctave (d - centre)) <= 25.0f) { sum += foldOctave (d - centre); ++n; }
            centre = foldOctave (centre + (float) (sum / std::max (1, n)));
        }
        int sounding = 0;
        for (auto& k : res.keys) sounding += k.silent ? 0 : 1;
        // (layers a fifth / fourth apart are a chord by design: YIN hears their common root, the check is left out)
        bool oneClass = true;
        for (int k = 1; k < compiled.numParts; ++k)
            oneClass &= ((compiled.parts[(size_t) k].transpose - compiled.parts[0].transpose) % 12 + 12) % 12 == 0;
        if (oneClass && bestIn >= (int) devs.size() * 3 / 4 && (int) devs.size() * 2 >= sounding && std::abs (centre) > 30.0f) res.offKey = centre;
        // the presets whose pitches do not settle (wide vibrato, glides, sequences) are left to the neighbour check
        if (bestIn >= (int) devs.size() * 3 / 4)
            for (auto& k : res.keys)
                if (trustedPitch (k))
                {
                    const float d = foldOctave (1200.0f * std::log2 (k.f0 / (440.0f * std::pow (2.0f, (k.key - 69) / 12.0f))) - centre);
                    if (std::abs (d) > 25.0f && std::abs (d) < 600.0f)
                    {
                        // a lone outlier key the neighbour check has already counted is not counted twice
                        bool counted = false;
                        for (auto& pr : res.pairs) if ((pr.key == k.key || pr.key == k.key + 1) && pr.evaluated && ! pr.ok()) counted = true;
                        if (! counted) ++res.tuneOff;
                    }
                }
    }
    if (! res.pass())
    {
        juce::String w;
        w << res.seqFail;
        if (res.offKey != 0.0f) w << "every key plays " << juce::String (res.offKey, 0) << " cents away from the played note (YIN, mod octave); ";
        int listed = 0;
        for (auto& pr : res.pairs)
            if (pr.evaluated && ! pr.ok() && listed++ < 6)
                w << vault::noteName (pr.key - 1) << "->" << vault::noteName (pr.key) << " " << (pr.same ? "SAME PITCH " : "")
                  << "yin " << juce::String (pr.yinCents, 0) << "c xc " << juce::String (pr.xcCents, 0) << "c (" << juce::String (pr.xcPeak, 2) << ") peak "
                  << juce::String (pr.pkCents, 0) << "c track " << juce::String (pr.trCents, 0) << "c frames " << juce::String (pr.frCents, 0) << "c f1 " << juce::String (pr.f1Cents, 0) << "c; ";

        res.why = w;
    }
    return res;
}
} // namespace

int run (int argc, char** argv)
{
    Options o;
    for (int i = 2; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        auto next = [&] { return i + 1 < argc ? juce::String (argv[++i]) : juce::String(); };
        if (a == "--bank") o.bank = juce::File::getCurrentWorkingDirectory().getChildFile (next());
        else if (a == "--only") o.only = next().toLowerCase();
        else if (a == "--step") o.step = std::max (1, next().getIntValue());
        else if (a == "--lo") o.lo = juce::jlimit (0, 127, next().getIntValue());
        else if (a == "--hi") o.hi = juce::jlimit (0, 127, next().getIntValue());
        else if (a == "--threads") o.threads = std::max (1, next().getIntValue());
        else if (a == "--csv") o.csv = next();
        else if (a == "--nofx") o.noFx = true;
        else if (a == "--wav") o.wavDir = next();
        else if (a == "--results") o.resultsFile = next();
        else if (a == "--dump") o.dump = true;
        else if (a == "--library") o.library = true;
        else if (a == "--ids") { juce::StringArray l; juce::File::getCurrentWorkingDirectory().getChildFile (next()).readLines (l); for (auto& x : l) if (x.trim().isNotEmpty()) o.ids.insert (x.trim().upToFirstOccurrenceOf ("\t", false, false)); }
        else if (a == "--vel") noteVelocity = juce::jlimit (1, 127, next().getIntValue());
        else if (a == "-v") o.verbose = true;
    }
    if (o.bank == juce::File())
    {
        const auto env = juce::SystemStats::getEnvironmentVariable ("CHROME_VAULT_BANK", {});
        o.bank = env.isNotEmpty() ? juce::File (env) : juce::File::getCurrentWorkingDirectory().getChildFile ("resources/vault/factory.cvb");
    }
    std::vector<VaultPreset> bank;
    juce::String err;
    if (! vault::loadBankFile (o.bank, bank, &err)) { std::printf ("!! FAIL: cannot load the bank %s (%s)\n", o.bank.getFullPathName().toRawUTF8(), err.toRawUTF8()); return 1; }
    if (o.dump)   // --dump: every preset's layers with their values (one line each), nothing rendered
    {
        for (auto& p : bank)
        {
            std::printf ("%s\t%s", p.id.toRawUTF8(), p.name.toRawUTF8());
            for (auto& part : p.parts)
            {
                std::printf ("\t%s %s/%s oct %d semi %d", part.kind == VaultPartKind::sample ? "sample" : part.kind == VaultPartKind::model ? "model" : "synth",
                             part.source.toRawUTF8(), part.articulation.toRawUTF8(), part.octave, part.semi);
                for (auto& [k, v] : part.values) std::printf (" %s=%g", k.toRawUTF8(), v);
            }
            std::printf ("\n");
        }
        return 0;
    }
    const auto lib = vault::SampleLibrary::getShared();
    const bool haveLib = lib != nullptr && lib->libraryFound();
    // --library: instead of the bank, every articulation of every library instrument as a plain one-layer preset (the zones
    // as the sampler plays them, no envelope / FX): which zone boundaries are out of tune, independent of any preset
    if (o.library && haveLib)
    {
        bank.clear();
        for (int i = 0; i < lib->numInstruments(); ++i)
        {
            const auto& d = lib->instrument (i)->desc;
            for (auto& a : d.articulations)
            {
                VaultPreset p;
                p.id = "lib." + d.id + "/" + a.name;
                p.name = d.name + " / " + a.name;
                p.v3 = true;
                VaultPart part;
                part.kind = VaultPartKind::sample;
                part.source = d.id;
                part.articulation = a.name;
                part.label = d.name;
                p.parts.push_back (part);
                bank.push_back (p);
            }
        }
    }
    std::vector<int> todo;
    int skippedNoLib = 0;
    for (int i = 0; i < (int) bank.size(); i += o.step)
    {
        const auto& p = bank[(size_t) i];
        if (o.only.isNotEmpty() && ! p.id.toLowerCase().contains (o.only) && ! p.name.toLowerCase().contains (o.only)) continue;
        if (! o.ids.empty() && o.ids.count (p.id) == 0) continue;
        if (! haveLib && p.usesSamples()) { ++skippedNoLib; continue; }
        todo.push_back (i);
    }
    // --results <file>: one line per finished preset (id, PASS / FAIL, why), appended as the run goes; a re-run with the
    // same file skips the presets already in it and counts their verdicts (long runs resume after an interruption)
    int prevPass = 0, prevFail = 0;
    std::unique_ptr<juce::FileOutputStream> resultsOut;
    if (o.resultsFile.isNotEmpty())
    {
        const auto rf = juce::File::getCurrentWorkingDirectory().getChildFile (o.resultsFile);
        std::set<juce::String> doneIds;
        juce::StringArray lines;
        if (rf.existsAsFile()) rf.readLines (lines);
        for (auto& l : lines)
        {
            const auto f = juce::StringArray::fromTokens (l, "\t", "");
            if (f.size() < 2 || doneIds.count (f[0]) > 0) continue;
            doneIds.insert (f[0]);
            if (f[1] == "PASS") ++prevPass; else { ++prevFail; std::printf ("!! FAIL (earlier run): %s\n", l.fromFirstOccurrenceOf ("\t", false, false).toRawUTF8()); }
        }
        todo.erase (std::remove_if (todo.begin(), todo.end(), [&] (int i) { return doneIds.count (bank[(size_t) i].id) > 0; }), todo.end());
        if (! doneIds.empty()) std::printf ("resuming: %d presets from %s (%d failed)\n", (int) doneIds.size(), rf.getFileName().toRawUTF8(), prevFail);
        resultsOut = std::make_unique<juce::FileOutputStream> (rf);
        if (! resultsOut->openedOk()) resultsOut.reset();
    }
    std::printf ("pitch test: %d presets of %s (%d in the bank), keys %s..%s, sample library %s\n", (int) todo.size(),
                 o.bank.getFileName().toRawUTF8(), (int) bank.size(), vault::noteName (o.lo).toRawUTF8(), vault::noteName (o.hi).toRawUTF8(),
                 haveLib ? "found" : "NOT FOUND (sampled presets skipped)");
    std::vector<PresetResult> results (todo.size());
    std::atomic<int> next { 0 }, done { 0 };
    std::mutex printLock;
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    const int nThreads = o.threads > 0 ? o.threads : (int) std::max (1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    for (int t = 0; t < nThreads; ++t)
        threads.emplace_back ([&]
        {
            Analyser an;
            for (int k = next++; k < (int) todo.size(); k = next++)
            {
                results[(size_t) k] = checkPreset (bank[(size_t) todo[(size_t) k]], todo[(size_t) k], o, an);
                const int d = ++done;
                const auto& r = results[(size_t) k];
                std::lock_guard<std::mutex> lk (printLock);
                if (! r.pass()) std::printf ("!! FAIL: %s %s: %s\n", r.id.toRawUTF8(), r.name.toRawUTF8(), r.why.toRawUTF8());
                if (resultsOut != nullptr)
                {
                    *resultsOut << r.id << "\t" << (r.pass() ? "PASS" : "FAIL") << "\t" << r.name << ": " << r.why << "\n";
                    resultsOut->flush();
                }
                if (d % 100 == 0) std::printf ("   ... %d / %d (%.0f s)\n", d, (int) todo.size(), (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
            }
        });
    for (auto& th : threads) th.join();

    int passed = 0, failed = 0, withSilent = 0, silentKeys = 0, infraKeys = 0, unpitchedPresets = 0, samePresets = 0, pairs = 0, tuneWarn = 0, layerPairs = 0, heldPairs = 0;
    for (auto& r : results)
    {
        passed += r.pass() ? 1 : 0; failed += r.pass() ? 0 : 1;
        withSilent += r.silentKeys > 0 ? 1 : 0; silentKeys += r.silentKeys; infraKeys += r.infrasonicKeys;
        unpitchedPresets += r.unpitchedPairs > 0 ? 1 : 0;
        samePresets += r.samePairs > 0 ? 1 : 0;
        tuneWarn += r.tuneOff > 0 ? 1 : 0;
        layerPairs += r.layerPairs;
        heldPairs += r.heldPairs;
        pairs += r.evaluated;
        if (o.verbose && ! r.pass())
        {
            std::printf ("   %s %s: %s\n", r.id.toRawUTF8(), r.name.toRawUTF8(), r.layers.toRawUTF8());
            for (size_t i = 0; i < r.keys.size(); ++i)
            {
                const auto& k = r.keys[i];
                const PairResult* pr = i > 0 ? &r.pairs[i - 1] : nullptr;
                std::printf ("     %-4s %6.1f dB  f0 %8.2f Hz (%s, %.2f)", vault::noteName (k.key).toRawUTF8(), k.levelDb, k.f0, k.confident ? "yin" : "---", k.periodicity);
                if (pr != nullptr) std::printf ("  step yin %6.1f xc %6.1f (%.2f) pk %6.1f tr %6.1f fr %6.1f f1 %6.1f%s", pr->yinCents, pr->xcCents, pr->xcPeak, pr->pkCents, pr->trCents, pr->frCents, pr->f1Cents,
                                                pr->evaluated ? (pr->ok() ? "" : "  <-- FAIL") : (pr->unpitched ? "  unpitched" : "  silent"));
                if (pr != nullptr && pr->layerInfo.isNotEmpty()) std::printf ("%s", pr->layerInfo.toRawUTF8());
                std::printf ("\n");
            }
        }
    }
    // reports: silent keys (by design: outside an instrument's playable range) and unpitched pairs
    {
        int listed = 0;
        for (auto& r : results)
            if (r.silentKeys > 0 && listed++ < 25)
                std::printf ("   silent keys: %s %s: %d (%s..%s)\n", r.id.toRawUTF8(), r.name.toRawUTF8(), r.silentKeys,
                             vault::noteName (r.silent.front()).toRawUTF8(), vault::noteName (r.silent.back()).toRawUTF8());
        listed = 0;
        for (auto& r : results)
            if (r.unpitchedPairs > 0 && listed++ < 25)
                std::printf ("   unpitched (no measurable pitch): %s %s: %d key pairs\n", r.id.toRawUTF8(), r.name.toRawUTF8(), r.unpitchedPairs);
    }
    if (o.csv.isNotEmpty())
    {
        juce::String csv ("preset,name,key,level_db,f0_hz,yin_confident,periodicity,step_yin_cents,step_xc_cents,xc_peak,pair_ok,step_peak,step_track,step_frames,step_f1\n");
        for (auto& r : results)
            for (size_t i = 0; i < r.keys.size(); ++i)
            {
                const auto& k = r.keys[i];
                csv << r.id << ",\"" << r.name << "\"," << k.key << "," << juce::String (k.levelDb, 1) << "," << juce::String (k.f0, 2) << ","
                    << (k.confident ? 1 : 0) << "," << juce::String (k.periodicity, 2);
                if (i > 0)
                {
                    const auto& pr = r.pairs[i - 1];
                    csv << "," << juce::String (pr.yinCents, 1) << "," << juce::String (pr.xcCents, 1) << "," << juce::String (pr.xcPeak, 2) << ","
                        << (! pr.evaluated ? "skip" : pr.ok() ? "ok" : "FAIL") << "," << juce::String (pr.pkCents, 1) << "," << juce::String (pr.trCents, 1)
                        << "," << juce::String (pr.frCents, 1) << "," << juce::String (pr.f1Cents, 1);
                }
                csv << "\n";
            }
        juce::File::getCurrentWorkingDirectory().getChildFile (o.csv).replaceWithText (csv);
    }
    passed += prevPass; failed += prevFail;
    std::printf ("%s %d / %d presets play every key at its own pitch (one semitone +-10 cents between neighbours; %d key pairs measured; "
                 "%d presets with same-pitch neighbours; %d presets with silent keys, %d keys; %d sub-bass keys (< 32.7 Hz) skipped; "
                 "%d presets with unpitched pairs; %d layered key pairs passed layer by layer; %d short-note pairs passed held; %d presets with YIN tuning warnings%s; %.0f s)\n",
                 failed == 0 ? "   ok:" : "!! FAIL:", passed, passed + failed, pairs, samePresets, withSilent, silentKeys, infraKeys, unpitchedPresets, layerPairs, heldPairs, tuneWarn,
                 skippedNoLib > 0 ? (", " + juce::String (skippedNoLib) + " sampled presets skipped: no library").toRawUTF8() : "",
                 (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0);
    std::printf ("vault pitch failures: %d\n", failed);
    return failed;
}
} // namespace pitchtest
