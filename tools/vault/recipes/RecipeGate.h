#pragma once
// CHROME VAULT v2 recipe quality gate (VaultBankGen -recipe <name>|all). Header-only, included by BankGen.cpp.
//
// Every preset of the recipe(s) is rendered at 48 kHz, velocity 0.8, at C5 (MIDI 60 = the home note, FL Studio naming),
// note off after 1.5 s and rendered on until the tail is gone, plus 2.5 s at C4 / C6 and C5 at velocity 0.35 / 1.0.
//   gate      NaN, silence, DC, clicks (note start / end / anywhere: BankQC / BankAudit detectors), clipping (vel 1.0 peak
//             under the safety limiter after normalisation), mono-sum loss > 3 dB, side energy under 120 Hz,
//             tail <= 10 s (Pads / FX & Atmos <= 16 s)
//   comfort   at C5: the sounding fundamental within 3 dB of the strongest harmonic (not Bells & Mallets / FX & Atmos),
//             2-5 kHz <= 10 % of the power, > 6 kHz <= 3 %, octave-up parts >= 12 dB (K-weighted) under part 0,
//             velocity 0.35 -> 1.0 opens >= 3 dB
//   loudness  K-weighted loudest 400 ms at C5 set to the category target (BankQC.h) by VaultPreset::gain (quieter only
//             when the velocity-1.0 peak would reach 0.84)
//   distinct  fingerprint = log-mel (24 bands) x 6 time segments at C5 + x 3 at C4 / C6 + 12 cepstral coefficients,
//             normalised to the render's own maximum; distance = RMS dB. A preset closer than -dist (default 3.0) to an
//             already accepted one (recipe order) is rejected.
#include "Recipe.h"
#include "../BankQC.h"
#include "../BankAudit.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <atomic>
#include <map>
#include <set>
#include <thread>

namespace recipegate
{
constexpr double sr = 48000.0;
constexpr double holdSeconds = 1.5;
inline float tailLimit (int category) { return category == vault::catPads || category == vault::catFxAtmos ? 16.0f : 10.0f; }
inline bool inharmonicCategory (int category) { return category == vault::catBellsMallets || category == vault::catFxAtmos; }

inline juce::AudioBuffer<float> render (const VaultPreset& p, int note, float vel, double offSeconds, double totalSeconds)
{
    juce::MidiBuffer mb;
    mb.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) juce::jlimit (1, 127, (int) std::round (vel * 127.0f))), 0);
    mb.addEvent (juce::MidiMessage::noteOff (1, note), (int) std::round (offSeconds * sr));
    return VaultEngine::renderMidi (p, mb, (int) std::round (totalSeconds * sr), sr);
}

// the sounding pitch of a key on part 0
inline float soundingHz (const VaultPreset& p, int key)
{
    const auto& a = p.parts.front();
    const int n = key + 12 * a.octave + a.semi;
    return 440.0f * std::pow (2.0f, (float) (n - 69) / 12.0f);
}

struct Spectrum { std::vector<float> pow; double binHz = 1; };
// power spectrum of the mono sum over [t0, t0 + 2^order / sr)
inline Spectrum spectrum (const juce::AudioBuffer<float>& b, double t0, int order = 15)
{
    const int n = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> d ((size_t) (2 * n), 0.0f);
    const int s0 = (int) (t0 * sr);
    for (int i = 0; i < n; ++i)
    {
        const int s = s0 + i;
        if (s >= b.getNumSamples()) break;
        const float w = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (n - 1));
        d[(size_t) i] = 0.5f * (b.getSample (0, s) + b.getSample (1, s)) * w;
    }
    fft.performFrequencyOnlyForwardTransform (d.data());
    Spectrum sp;
    sp.binHz = sr / n;
    sp.pow.resize ((size_t) (n / 2));
    for (int i = 0; i < n / 2; ++i) sp.pow[(size_t) i] = d[(size_t) i] * d[(size_t) i];
    return sp;
}
inline double bandPower (const Spectrum& s, double lo, double hi)
{
    double acc = 0;
    for (int i = std::max (1, (int) (lo / s.binHz)); i < std::min ((int) s.pow.size(), (int) (hi / s.binHz)); ++i) acc += s.pow[(size_t) i];
    return acc;
}
inline float peakDb (const Spectrum& s, double lo, double hi)
{
    float m = 0;
    for (int i = std::max (1, (int) (lo / s.binHz)); i <= std::min ((int) s.pow.size() - 1, (int) (hi / s.binHz)); ++i) m = std::max (m, s.pow[(size_t) i]);
    return 10.0f * std::log10 (std::max (1.0e-20f, m));
}

// fingerprint: log-mel bands per time segment, dB under the render's own maximum, floored at -60
constexpr int melBands = 24;
inline std::vector<float> melFrames (const juce::AudioBuffer<float>& b, const std::vector<std::pair<double, double>>& segs)
{
    constexpr int order = 11, n = 1 << order;
    juce::dsp::FFT fft (order);
    auto mel = [] (double hz) { return 2595.0 * std::log10 (1.0 + hz / 700.0); };
    const double m0 = mel (40.0), m1 = mel (16000.0);
    std::vector<float> out;
    std::vector<float> d ((size_t) (2 * n));
    for (auto [t0, t1] : segs)
    {
        std::vector<double> band (melBands, 1.0e-12);
        for (int s0 = (int) (t0 * sr); s0 + n <= std::min (b.getNumSamples(), (int) (t1 * sr) + n / 2); s0 += n / 2)
        {
            std::fill (d.begin(), d.end(), 0.0f);
            for (int i = 0; i < n; ++i)
                d[(size_t) i] = 0.5f * (b.getSample (0, s0 + i) + b.getSample (1, s0 + i))
                                * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (n - 1)));
            fft.performFrequencyOnlyForwardTransform (d.data());
            for (int i = 1; i < n / 2; ++i)
            {
                const double hz = i * sr / n;
                if (hz < 40.0 || hz >= 16000.0) continue;
                const int k = juce::jlimit (0, melBands - 1, (int) ((mel (hz) - m0) / (m1 - m0) * melBands));
                band[(size_t) k] += (double) d[(size_t) i] * d[(size_t) i];
            }
        }
        for (auto v : band) out.push_back ((float) (10.0 * std::log10 (v)));
    }
    float mx = -300;
    for (auto v : out) mx = std::max (mx, v);
    for (auto& v : out) v = std::max (-60.0f, v - mx);
    return out;
}

struct Result
{
    VaultPreset preset;
    juce::String fail;            // empty = passed the gate
    float loud = -120, gainDb = 0, quietBy = 0, peak127 = 0;
    float h1Db = 0, share25 = 0, share6k = 0, centroid = 0, tail = 0, velRange = 0, octUpDb = -99, monoLoss = 0, spike = 0;
    std::vector<float> print;
    float minDist = 1.0e9f; juce::String nearest;
};

inline void analyse (Result& r)
{
    auto& p = r.preset;
    const int cat = p.category;
    p.gain = 1.0f;
    const float lim = tailLimit (cat);
    const auto c5 = render (p, vault::homeNote, 0.8f, holdSeconds, holdSeconds + lim + 1.0);
    const auto a = vaultqc::analyse (c5, sr, holdSeconds);
    if (! a.finite) { r.fail = "nan"; return; }
    r.loud = a.loud;
    if (a.rms < 1.0e-5f || a.loud < -55.0f) { r.fail = "silent"; return; }
    // tail: last 10 ms frame within 60 dB of the loudest, from the note-off
    {
        const int fr = (int) (0.01 * sr), nf = c5.getNumSamples() / fr;
        std::vector<float> e ((size_t) nf);
        float mx = -200;
        for (int f = 0; f < nf; ++f)
        {
            double acc = 0;
            for (int i = f * fr; i < (f + 1) * fr; ++i) acc += 0.5 * ((double) c5.getSample (0, i) * c5.getSample (0, i) + (double) c5.getSample (1, i) * c5.getSample (1, i));
            e[(size_t) f] = vaultaudit::db (acc / fr); mx = std::max (mx, e[(size_t) f]);
        }
        int last = 0;
        for (int f = 0; f < nf; ++f) if (e[(size_t) f] >= mx - 60.0f) last = f;
        r.tail = (float) ((last + 1) * 0.01 - holdSeconds);
        if (last >= nf - 2) r.tail = lim + 1.0f;
    }
    juce::AudioBuffer<float> head (2, (int) (2.5 * sr));
    for (int c = 0; c < 2; ++c) head.copyFrom (c, 0, c5, c, 0, head.getNumSamples());
    float sideTot = -120, sideMid = -120, midTot = -120;
    vaultaudit::lowBand (head, sideTot, sideMid, midTot);
    r.monoLoss = a.monoLossDb;
    r.spike = vaultaudit::worstSpike (head);
    double sl = 0, sr2 = 0;
    for (int i = 0; i < head.getNumSamples(); ++i) { sl += head.getSample (0, i); sr2 += head.getSample (1, i); }
    const float dcAbs = (float) std::max (std::abs (sl / head.getNumSamples()), std::abs (sr2 / head.getNumSamples()));

    // comfort spectrum (0.05 .. 0.73 s)
    const auto sp = spectrum (c5, 0.05);
    {
        const double tot = bandPower (sp, 20, 20000);
        r.share25 = (float) (bandPower (sp, 2000, 5000) / std::max (1e-20, tot));
        r.share6k = (float) (bandPower (sp, 6000, 20000) / std::max (1e-20, tot));
        double num = 0;
        for (int i = 1; i < (int) sp.pow.size(); ++i) num += i * sp.binHz * sp.pow[(size_t) i];
        r.centroid = (float) (num / std::max (1e-20, tot));
        const float f0 = soundingHz (p, vault::homeNote);
        const float h1 = peakDb (sp, f0 * 0.97, f0 * 1.03);
        float strongest = h1;
        for (int k = 2; k <= 24 && k * f0 < 16000.0f; ++k) strongest = std::max (strongest, peakDb (sp, k * f0 * 0.985, k * f0 * 1.015));
        r.h1Db = h1 - strongest;
    }
    // velocity
    const auto v1 = render (p, vault::homeNote, 1.0f, holdSeconds, 2.0);
    const auto vl = render (p, vault::homeNote, 0.35f, holdSeconds, 2.0);
    r.peak127 = v1.getMagnitude (0, v1.getNumSamples());
    r.velRange = vaultqc::analyse (v1, sr, holdSeconds).loud - vaultqc::analyse (vl, sr, holdSeconds).loud;
    // octave-up parts vs part 0
    for (size_t k = 1; k < p.parts.size(); ++k)
    {
        const int t0 = 12 * p.parts[0].octave + p.parts[0].semi, tk = 12 * p.parts[k].octave + p.parts[k].semi;
        if (tk < t0 + 12) continue;
        VaultPreset solo = p, main = p;
        solo.parts = { p.parts[k] };
        solo.parts[0].level = p.parts[k].level * juce::Decibels::decibelsToGain (p.parts[k].get ("gain", 0.0f) - p.parts[0].get ("gain", 0.0f));
        main.parts = { p.parts[0] };
        const float ls = vaultqc::analyse (render (solo, vault::homeNote, 0.8f, holdSeconds, 2.0), sr, holdSeconds).loud;
        const float lm = vaultqc::analyse (render (main, vault::homeNote, 0.8f, holdSeconds, 2.0), sr, holdSeconds).loud;
        r.octUpDb = std::max (r.octUpDb, ls - lm);
    }
    // loudness normalisation
    const float target = vaultqc::targetLoudness (cat);
    float g = target - r.loud;
    const float peakAt = juce::Decibels::gainToDecibels (std::max (1.0e-6f, r.peak127)) + g;
    if (peakAt > juce::Decibels::gainToDecibels (0.84f)) { r.quietBy = peakAt - juce::Decibels::gainToDecibels (0.84f); g -= r.quietBy; }
    r.gainDb = g;
    r.preset.gain = juce::Decibels::decibelsToGain (g);
    const float lin = r.preset.gain;

    // fingerprint
    {
        const auto c4 = render (p, vault::homeNote - 12, 0.8f, holdSeconds, 2.0);
        const auto c6 = render (p, vault::homeNote + 12, 0.8f, holdSeconds, 2.0);
        r.print = melFrames (c5, { { 0.0, 0.05 }, { 0.05, 0.15 }, { 0.15, 0.4 }, { 0.4, 0.8 }, { 0.8, 1.4 }, { 1.5, 2.1 } });
        const auto mid = melFrames (c5, { { 0.05, 1.4 } });
        for (int q = 1; q <= 12; ++q)   // cepstrum of the held log-mel spectrum (scaled to dB-like units)
        {
            double c = 0;
            for (int m = 0; m < melBands; ++m) c += mid[(size_t) m] * std::cos (juce::MathConstants<double>::pi * q * (m + 0.5) / melBands);
            r.print.push_back ((float) (c * 2.0 / melBands) * 2.0f);
        }
        for (auto* b : { &c4, &c6 })
            for (auto v : melFrames (*b, { { 0.0, 0.1 }, { 0.1, 0.5 }, { 0.5, 1.4 } })) r.print.push_back (v);
    }

    // the gate (levels at the final gain)
    if (a.peak * lin >= 0.885f || r.peak127 * lin >= 0.885f) r.fail = "clip";
    else if (dcAbs > 0.002f && dcAbs > 0.02f * a.rms) r.fail = "dc";
    else if (a.firstSample * lin > 0.01f || a.onsetJump > 1.5f * a.laterJump + 0.02f / lin) r.fail = "click-start";
    else if (a.offJump > 1.5f * a.preOffJump + 0.01f / lin) r.fail = "click-end";
    else if (r.spike > 8.0f) r.fail = "click-spike";
    else if (a.monoLossDb > 3.0f) r.fail = "mono-loss";
    else if (sideTot > -30.0f || (midTot > -30.0f && sideMid > -20.0f)) r.fail = "low-side";
    else if (r.tail > lim) r.fail = "tail";
    else if (! inharmonicCategory (cat) && r.h1Db < -3.0f) r.fail = "fundamental";
    else if (r.share25 > 0.10f) r.fail = "harsh-2-5k";
    else if (r.share6k > 0.03f) r.fail = "piercing-highs";
    else if (r.octUpDb > -12.0f) r.fail = "octave-layer";
    else if (r.velRange < 3.0f) r.fail = "velocity";
    else if (r.quietBy > 3.0f) r.fail = "too-peaky";
    else if (auto np = vaultaudit::nameProblem (p.name); np.isNotEmpty()) r.fail = "name: " + np;
    else if (p.name.containsIgnoreCase ("808")) r.fail = "name: 808";
    else
        for (auto& part : p.parts)
            if (part.label.containsIgnoreCase ("808") || ! vaultqc::isCleanName (part.label)) r.fail = "label";
}

inline float distance (const std::vector<float>& a, const std::vector<float>& b)
{
    double s = 0;
    const size_t n = std::min (a.size(), b.size());
    for (size_t i = 0; i < n; ++i) { const double d = a[i] - b[i]; s += d * d; }
    return (float) std::sqrt (s / std::max ((size_t) 1, n));
}

inline void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b)
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

// VaultBankGen -recipe <name>|all [-out bank.cvb] [-report report.md] [-threads N] [-dist 3.0] [-demo <dir> <id,id,...>]
inline int runRecipes (int argc, char** argv)
{
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const juce::String which = argv[2];
    juce::String outPath, reportPath, demoDir, demoIds;
    int threads = (int) std::max (1u, std::thread::hardware_concurrency());
    float minDist = 3.0f;
    for (int i = 3; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a == "-out" && i + 1 < argc) outPath = argv[++i];
        else if (a == "-report" && i + 1 < argc) reportPath = argv[++i];
        else if (a == "-threads" && i + 1 < argc) threads = juce::jmax (1, juce::String (argv[++i]).getIntValue());
        else if (a == "-dist" && i + 1 < argc) minDist = (float) juce::String (argv[++i]).getDoubleValue();
        else if (a == "-demo" && i + 2 < argc) { demoDir = argv[++i]; demoIds = argv[++i]; }
    }
    RecipeContext ctx;
    ctx.library = vault::SampleLibrary::getShared();
    std::printf ("library: %s (%s)\n", ctx.library->libraryPath().getFullPathName().toRawUTF8(), ctx.library->libraryFound() ? "found" : "MISSING");
    std::vector<Result> all;
    for (auto& e : recipe::registry())
        if (which == "all" || which == e.name)
            for (auto& p : e.build (ctx)) { Result r; r.preset = p; all.push_back (std::move (r)); }
    if (all.empty()) { std::printf ("no recipe '%s' (registered:", which.toRawUTF8()); for (auto& e : recipe::registry()) std::printf (" %s", e.name.toRawUTF8()); std::printf (")\n"); return 1; }
    std::printf ("%d candidate presets, %d threads\n", (int) all.size(), threads);
    {
        std::set<juce::String> ids, names;
        for (auto& r : all)
        {
            juce::String why;
            if (! vault::validatePreset (r.preset, &why)) r.fail = "invalid: " + why;
            else if (! ids.insert (r.preset.id).second) r.fail = "duplicate id";
            else if (! names.insert (r.preset.name.toLowerCase()).second) r.fail = "duplicate name";
        }
    }
    std::atomic<int> next { 0 }, done { 0 };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&]
        {
            for (int i; (i = next++) < (int) all.size();)
            {
                if (all[(size_t) i].fail.isEmpty()) analyse (all[(size_t) i]);
                const int d = ++done;
                if (d % 25 == 0) std::printf ("  %d / %d\n", d, (int) all.size());
            }
        });
    for (auto& t : pool) t.join();
    // distinctness, in recipe order
    std::vector<int> accepted;
    for (int i = 0; i < (int) all.size(); ++i)
    {
        auto& r = all[(size_t) i];
        for (int j : accepted)
        {
            const float d = distance (r.print, all[(size_t) j].print);
            if (d < r.minDist) { r.minDist = d; r.nearest = all[(size_t) j].preset.name; }
        }
        if (r.fail.isEmpty() && r.minDist < minDist) r.fail = "near-duplicate";
        if (r.fail.isEmpty()) accepted.push_back (i);
    }
    // min pairwise distance among the accepted
    float minPair = 1.0e9f; juce::String pairA, pairB;
    for (size_t x = 0; x < accepted.size(); ++x)
        for (size_t y = x + 1; y < accepted.size(); ++y)
        {
            const float d = distance (all[(size_t) accepted[x]].print, all[(size_t) accepted[y]].print);
            if (d < minPair) { minPair = d; pairA = all[(size_t) accepted[x]].preset.name; pairB = all[(size_t) accepted[y]].preset.name; }
        }
    std::map<juce::String, int> fails;
    std::map<int, int> perCat;
    for (auto& r : all) { if (r.fail.isNotEmpty()) ++fails[r.fail.upToFirstOccurrenceOf (":", false, false)]; else ++perCat[r.preset.category]; }
    std::printf ("accepted %d of %d; min pairwise distance %.2f (%s / %s)\n", (int) accepted.size(), (int) all.size(), minPair, pairA.toRawUTF8(), pairB.toRawUTF8());
    for (auto& [c, n] : perCat) std::printf ("  %-20s %d\n", vault::categoryNames()[c].toRawUTF8(), n);
    for (auto& [f, n] : fails) std::printf ("  rejected %-16s %d\n", f.toRawUTF8(), n);

    if (outPath.isNotEmpty())
    {
        std::vector<VaultPreset> bank;
        for (int i : accepted) bank.push_back (all[(size_t) i].preset);
        if (! vault::saveBankFile (cwd.getChildFile (outPath), bank)) { std::printf ("cannot write %s\n", outPath.toRawUTF8()); return 1; }
        std::printf ("wrote %s (%d presets)\n", outPath.toRawUTF8(), (int) bank.size());
    }
    if (reportPath.isNotEmpty())
    {
        juce::String s;
        s << "# Recipe gate report: " << which << "\n\naccepted " << (int) accepted.size() << " / " << (int) all.size()
          << ", min pairwise distance " << juce::String (minPair, 2) << " (" << pairA << " / " << pairB << "), threshold " << minDist << "\n\n"
          << "| id | name | parts | result | loud | gain dB | H1 dB | 2-5k % | >6k % | centroid | tail s | vel dB | oct-up dB | nearest (dist) |\n"
          << "|---|---|---|---|---|---|---|---|---|---|---|---|---|---|\n";
        for (auto& r : all)
        {
            juce::StringArray parts;
            for (auto& pt : r.preset.parts) parts.add (pt.label + (pt.octave != 0 ? " (" + juce::String (pt.octave > 0 ? "+" : "") + juce::String (pt.octave) + ")" : juce::String()));
            s << "| " << r.preset.id << " | " << r.preset.name << " | " << parts.joinIntoString (" + ") << " | " << (r.fail.isEmpty() ? "ok" : r.fail)
              << " | " << juce::String (r.loud, 1) << " | " << juce::String (r.gainDb, 1) << " | " << juce::String (r.h1Db, 1) << " | " << juce::String (100 * r.share25, 1)
              << " | " << juce::String (100 * r.share6k, 2) << " | " << juce::String ((int) r.centroid) << " | " << juce::String (r.tail, 1) << " | " << juce::String (r.velRange, 1)
              << " | " << (r.octUpDb > -98 ? juce::String (r.octUpDb, 1) : juce::String ("-")) << " | " << r.nearest << " (" << juce::String (r.minDist < 1e8f ? r.minDist : 0.0f, 2) << ") |\n";
        }
        cwd.getChildFile (reportPath).replaceWithText (s);
        std::printf ("wrote %s\n", reportPath.toRawUTF8());
    }
    if (demoDir.isNotEmpty())
    {
        auto dir = cwd.getChildFile (demoDir);
        dir.createDirectory();
        for (auto& id : juce::StringArray::fromTokens (demoIds, ",", ""))
            for (auto& r : all)
                if (r.preset.id == id)
                {
                    auto b = render (r.preset, vault::homeNote, 0.8f, 1.8, 3.0);
                    const int fade = (int) (0.05 * sr);
                    for (int c = 0; c < 2; ++c) b.applyGainRamp (c, b.getNumSamples() - fade, fade, 1.0f, 0.0f);
                    const auto f = dir.getChildFile (r.preset.name.replaceCharacters (" /", "__") + " C5.wav");
                    writeWav (f, b);
                    std::printf ("demo %s\n", f.getFullPathName().toRawUTF8());
                }
    }
    return 0;
}
} // namespace recipegate
