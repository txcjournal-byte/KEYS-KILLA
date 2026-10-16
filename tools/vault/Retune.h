#pragma once
// VaultLibBuild -retune: the library tuning pass (owner report item 0, "neighbouring keys play the same pitch").
//
// The builder tunes every file on its own (YIN or the spectrum at its root). Where two zones meet on the keyboard (key k
// plays zone A, key k+1 zone B), their own small errors add up: a piano bass sample 15 cents sharp next to one 15 cents
// flat makes a 70-cent "semitone", a mbira tine read on an inharmonic partial lands a whole semitone off and two keys
// play nearly the same pitch. This pass measures the zones against EACH OTHER, the way the pitch test measures neighbouring
// keys (tools/vault/PitchAnalysis.h): for every pair of zones that meet on the keyboard (neighbouring keys, the same key in
// another velocity layer / round robin) the cross-correlation of their log spectra gives their relative tuning; YIN
// anchors the absolute pitch where it is reliable (articulations tuned by YIN), the current tuning anchors it weakly
// everywhere. A robust least-squares fit (re-weighted, outlier pairs dropped) gives every zone's deviation d (cents);
// the zone's fine tune becomes fine - d (and the root moves when the result passes +-50 cents: same pitch, tidier JSON).
// Writes library.json in place (only "root" / "fine" change) and retune_report.txt. -dry: report only.
#include "PitchAnalysis.h"
#include "../../Source/vault/VaultLibrary.h"
#include <map>
#include <set>

namespace retune
{
struct ZoneM
{
    int json = -1;                 // index in the articulation's JSON "zones"
    int root = 60, lo = 0, hi = 127, vlo = 1, vhi = 127, rr = 0;
    float fine = 0;
    int root0 = 60; float fine0 = 0;   // as loaded
    vaultpitch::Analysis a;
    bool usable = false;
    double yinDev = 0;             // cents: YIN pitch vs root (fine applied)
    bool yinOk = false;
    double d = 0;                  // solved deviation (cents)
};

struct Pair { int a = 0, b = 0; double r = 0, w = 0; float peak = 0; };   // r = d_b - d_a measured

inline double midiCents (double hz) { return 6900.0 + 1200.0 * std::log2 (hz / 440.0); }

// a zone played at `key` the way the sampler plays it (resampled from its start to 44.1 kHz, up to 1.2 s; linear
// interpolation is enough for a pitch measurement) and measured the way the pitch test measures a key
inline vaultpitch::Analysis playAt (const vault::SampleInstrumentData::Zone& Z, int root, float fine, int key, vaultpitch::Analyser& an,
                                    std::vector<float>& mono)
{
    constexpr double outSr = 44100.0;
    const auto* au = Z.audio;
    const double inc = std::exp2 ((key - root) / 12.0 + fine / 1200.0) * au->sampleRate / outSr;
    const int n = (int) std::min (1.6 * outSr, (double) (au->frames - Z.start - 2) / inc);
    mono.assign ((size_t) std::max (0, n), 0.0f);
    if (n < 4096) return {};
    const float norm = 1.0f / (32768.0f * (float) au->channels);
    for (int c = 0; c < au->channels; ++c)
    {
        const int16_t* src = au->channel (c);
        for (int i = 0; i < n; ++i)
        {
            const double pos = Z.start + i * inc;
            const int ip = (int) pos;
            const float t = (float) (pos - ip);
            mono[(size_t) i] += ((float) src[ip] * (1.0f - t) + (float) src[ip + 1] * t) * norm;
        }
    }
    const double f = 440.0 * std::pow (2.0, (key - 69) / 12.0);
    return an.analyse (mono.data(), n, outSr, f, 4.5, vaultpitch::lowCutRatio * f);
}

// the interval (cents) from note a to note b as the pitch test sees it (vaultpitch::compareNotes): the value most of its
// measurements agree on (YIN, the spectra, the strongest partial, the YIN tracks); w = the confidence of that agreement
inline bool measureInterval (const vaultpitch::Analysis& a, const vaultpitch::Analysis& b, const std::vector<float>& monoB, double fB,
                             double lowCutA, vaultpitch::Analyser& an, float expect, float& cents, double& w, bool preferFund = false)
{
    if (a.silent || b.silent) return false;
    const auto iv = vaultpitch::compareNotes (a, b, monoB.data(), (int) monoB.size(), 44100.0, fB, lowCutA, an, expect);
    // tuned bars / tines (relative-only articulations): the fundamental partial is the pitch; their upper modes are
    // inharmonic and differ from bar to bar, so the whole spectrum lines up at a wrong shift
    if (preferFund && iv.f1 < 9000.0f && std::abs (iv.f1 - expect) < 90.0f && (iv.f1b >= 9000.0f || std::abs (iv.f1 - iv.f1b) <= 10.0f))
    {
        cents = iv.f1b < 9000.0f ? 0.5f * (iv.f1 + iv.f1b) : iv.f1;
        w = 1.0;
        return true;
    }
    int votes = 0;
    if (! iv.consensus (expect, cents, votes) || std::abs (cents - expect) > 90.0f) return false;
    if (votes == 1 && iv.unpitched) return false;   // one stray measurement on a noisy note
    w = votes >= 3 ? 1.0 : votes == 2 ? 0.7 : 0.3;
    return true;
}

// the same two notes later on (from 0.4 s, while both are within 20 dB of their peak): a sustained sample whose pitch
// settles or drifts is tuned on its average, not on its first moments (a slow attack plays the later part)
inline bool measureIntervalLate (const vaultpitch::Analysis& a, const vaultpitch::Analysis& b, const std::vector<float>& monoA, const std::vector<float>& monoB,
                                 double fA, double fB, vaultpitch::Analyser& an, float expect, float& cents, double& w, bool preferFund = false)
{
    const int from = (int) (0.4 * 44100.0), len = std::min ({ vaultpitch::maxWinLen, (int) monoA.size() - from, (int) monoB.size() - from });
    if (a.silent || b.silent || len < 8192) return false;
    const auto aL = an.analyseAt (monoA.data(), (int) monoA.size(), 44100.0, fA, from, len, 4.5, vaultpitch::lowCutRatio * fA);
    const auto bL = an.analyseAt (monoB.data(), (int) monoB.size(), 44100.0, fB, from, len, 4.5, vaultpitch::lowCutRatio * fB);
    if (aL.silent || bL.silent || aL.levelDb < a.levelDb - 20.0f || bL.levelDb < b.levelDb - 20.0f) return false;
    return measureInterval (aL, bL, monoB, fB, vaultpitch::lowCutRatio * fA, an, expect, cents, w, preferFund);
}

// solves for the zones' deviations; returns the number of pairs kept
inline int solve (std::vector<ZoneM>& z, std::vector<Pair>& pairs, bool yinAnchors, juce::String& log)
{
    // the builder's own tuning is the prior: weak where YIN anchors the pitch, stronger for the relative-only articulations
    // (bars, tines, bells: their spectra also line up at wrong shifts now and then)
    const double wPrior = yinAnchors ? 0.02 : 0.25, wYin = 0.08;
    for (auto& q : z) q.d = 0;
    std::vector<double> pw (pairs.size(), 1.0);
    // a zone that meets only one other zone (the end of the range, a lone velocity layer): one comparison is weak
    // evidence, the zone's own tuning counts as much as it (the two meet half way)
    std::vector<int> pairCount (z.size(), 0);
    for (auto& pr : pairs) { ++pairCount[(size_t) pr.a]; ++pairCount[(size_t) pr.b]; }
    for (int pass = 0; pass < 4; ++pass)
    {
        for (int it = 0; it < 400; ++it)
        {
            double change = 0;
            for (size_t i = 0; i < z.size(); ++i)
            {
                if (! z[i].usable) continue;
                // prior: the zone's tuning as built (the rounds re-measure; the prior is the original, not this round's)
                const double wp = wPrior + (pairCount[i] == 1 ? 1.0 : 0.0);
                double num = wp * (z[i].fine - z[i].fine0), den = wp;
                if (yinAnchors && z[i].yinOk) { num += wYin * z[i].yinDev; den += wYin; }
                for (size_t p = 0; p < pairs.size(); ++p)
                {
                    const auto& pr = pairs[p];
                    const double w = pr.w * pw[p];
                    if (w <= 0) continue;
                    if (pr.a == (int) i) { num += w * (z[(size_t) pr.b].d - pr.r); den += w; }
                    else if (pr.b == (int) i) { num += w * (z[(size_t) pr.a].d + pr.r); den += w; }
                }
                const double nd = num / den;
                change = std::max (change, std::abs (nd - z[i].d));
                z[i].d = nd;
            }
            if (change < 0.01) break;
        }
        // robust: pairs that disagree with the fit by more than 20 cents (timbre change, noise) lose their weight
        int dropped = 0;
        for (size_t p = 0; p < pairs.size(); ++p)
        {
            const double e = std::abs (z[(size_t) pairs[p].b].d - z[(size_t) pairs[p].a].d - pairs[p].r);
            const double nw = e < 8.0 ? 1.0 : e < 20.0 ? 8.0 / e : 0.0;
            if (nw == 0.0 && pw[p] > 0.0) ++dropped;
            pw[p] = nw;
        }
        if (pass == 3 && dropped > 0) log << "      " << dropped << " pairs disagree with the fit by > 20 cents (dropped)\n";
    }
    int kept = 0;
    for (double w : pw) kept += w > 0 ? 1 : 0;
    return kept;
}

struct Fix { juce::String file; int oldRoot = 60; float oldFine = 0; int root = 60; float fine = 0; };

// the plugin's built-in tuning patch (Source/vault/LibraryTuningData.cpp): a library.json from before this pass is
// corrected at load time, so an installed library plays in tune without a new download (VaultLibrary.cpp)
inline bool writePatch (const juce::File& f, std::vector<Fix> fixes)
{
    std::sort (fixes.begin(), fixes.end(), [] (const Fix& a, const Fix& b) { return a.file < b.file; });
    juce::String s;
    s << "// GENERATED by VaultLibBuild -retune -emit " << f.getFileName() << " (tools/vault/Retune.h) - do not edit.\n"
      << "// The library tuning pass: zones of a library.json written before the pass (no \"retuned\" flag) get these root / fine\n"
      << "// values when they still hold the old ones (Source/vault/VaultLibrary.cpp).\n"
      << "#include \"LibraryTuning.h\"\n\nnamespace vault\n{\nnamespace\n{\nconst ZoneTuneFix fixes[] = {\n";
    for (auto& x : fixes)
        s << "    { \"" << x.file << "\", " << x.oldRoot << ", " << juce::String (x.oldFine, 1) << "f, " << x.root << ", " << juce::String (x.fine, 1) << "f },\n";
    s << "};\n}\n\nconst ZoneTuneFix* libraryTuneFixes (int& count)\n{\n    count = (int) (sizeof (fixes) / sizeof (fixes[0]));\n    return fixes;\n}\n} // namespace vault\n";
    return f.replaceWithText (s);
}

inline int run (const juce::File& outDir, const juce::StringArray& onlyIds, bool dry, int threads, const juce::File& emit = {})
{
    std::vector<Fix> fixes;
    auto lib = std::make_shared<vault::SampleLibrary> (outDir);
    if (! lib->libraryFound()) { std::printf ("no library at %s\n", outDir.getFullPathName().toRawUTF8()); return 1; }
    lib->setMemoryBudget ((int64_t) 1536 * 1024 * 1024);   // older instruments are evicted as the pass moves on
    const auto jsonFile = outDir.getChildFile ("library.json");
    auto json = juce::JSON::parse (jsonFile);
    if (! json.isObject()) { std::printf ("cannot parse %s\n", jsonFile.getFullPathName().toRawUTF8()); return 1; }
    juce::String report;
    int changedZones = 0, totalZones = 0, bigMoves = 0;
    for (int ii = 0; ii < lib->numInstruments(); ++ii)
    {
        auto* h = lib->instrument (ii);
        const auto& desc = h->desc;
        if (! onlyIds.isEmpty() && ! onlyIds.contains (desc.id)) continue;
        juce::var ij;
        for (auto& v : *json["instruments"].getArray()) if (v["id"].toString() == desc.id) ij = v;
        if (! ij.isObject() || ! lib->loadNow (h) || h->data() == nullptr) { report << "== " << desc.id << ": not loaded, skipped\n"; continue; }
        const auto* data = h->data();
        juce::String instLog;
        instLog << "== " << desc.id << " \"" << desc.name << "\"\n";
        for (size_t ai = 0; ai < desc.articulations.size() && ai < data->articulations.size(); ++ai)
        {
            const auto& ad = desc.articulations[ai];
            const auto& A = data->articulations[ai];
            auto aj = ij["articulations"][(int) ai];
            const auto mode = aj.hasProperty ("pitchCheck") ? aj["pitchCheck"].toString() : juce::String ("yin");
            // the non-release zones in loader order (= A.zones)
            std::vector<ZoneM> z;
            for (int zi = 0; zi < (int) ad.zones.size(); ++zi)
            {
                const auto& sz = ad.zones[(size_t) zi];
                if (sz.releaseTrigger) continue;
                ZoneM m;
                m.json = zi; m.root = sz.root; m.fine = sz.fine;
                // the values in library.json, not the loader's (which applies the plugin's built-in patch to a library
                // from before the pass): every run starts from the library as built, the result does not drift run to run
                if (const auto* za = aj["zones"].getArray(); za != nullptr && zi < za->size())
                {
                    m.root = (int) (*za)[zi].getProperty ("root", m.root);
                    m.fine = (float) (double) (*za)[zi].getProperty ("fine", (double) m.fine);
                } m.lo = sz.lo; m.hi = sz.hi; m.vlo = sz.vlo; m.vhi = sz.vhi; m.rr = sz.rr;
                z.push_back (m);
            }
            if (z.size() != A.zones.size()) { instLog << "   " << ad.name << ": zone count mismatch, skipped\n"; continue; }
            for (auto& q : z) { q.root0 = q.root; q.fine0 = q.fine; }
            const bool yinAnchors = mode == "yin";
            // the zones that can be measured
            for (size_t j = 0; j < z.size(); ++j)
                z[j].usable = A.zones[j].audio != nullptr && A.zones[j].audio->frames - A.zones[j].start >= 4096;
            // the pairs that meet on the keyboard, at the keys where they meet: zone i at key k next to zone j at k + 1, or
            // both at key k (another velocity layer / round robin)
            struct Meet { int i, j, ki, kj; };
            std::vector<Meet> meets;
            {
                std::set<std::pair<int, int>> seen;
                for (int k = 0; k < 128; ++k)
                    for (size_t i = 0; i < z.size(); ++i)
                    {
                        if (! z[i].usable || k < z[i].lo || k > z[i].hi) continue;
                        for (size_t j = 0; j < z.size(); ++j)
                        {
                            if (j == i || ! z[j].usable) continue;
                            const bool sameKey = k >= z[j].lo && k <= z[j].hi, nextKey = k + 1 >= z[j].lo && k + 1 <= z[j].hi;
                            if (! sameKey && ! nextKey) continue;
                            if (z[i].vhi < z[j].vlo || z[j].vhi < z[i].vlo)
                                if (! (sameKey && z[i].rr == 0 && z[j].rr == 0)) continue;   // other velocity layers: compared at the same key
                            if (! seen.insert ({ (int) std::min (i, j), (int) std::max (i, j) }).second) continue;
                            meets.push_back ({ (int) i, (int) j, k, sameKey ? k : k + 1 });
                        }
                    }
            }
            // measure -> solve -> apply, three rounds (the zones are re-measured at their new tuning)
            int kept = 0, pairCount = 0;
            juce::String alog;
            for (int round = 0; round < 3; ++round)
            {
                // every zone at every key it is heard at in a pair, and at its root (the YIN anchor)
                std::map<std::pair<int, int>, vaultpitch::Analysis> res;
                for (auto& m : meets) { res[{ m.i, m.ki }]; res[{ m.j, m.kj }]; }
                if (yinAnchors) for (size_t j = 0; j < z.size(); ++j) if (z[j].usable) res[{ (int) j, z[j].root }];
                std::vector<std::pair<int, int>> todo;
                for (auto& kv : res) todo.push_back (kv.first);
                std::vector<vaultpitch::Analysis> out (todo.size());
                std::vector<std::vector<float>> outMono (todo.size());
                std::atomic<int> next { 0 };
                std::vector<std::thread> ths;
                for (int t = 0; t < std::max (1, threads); ++t)
                    ths.emplace_back ([&]
                    {
                        vaultpitch::Analyser an;
                        for (int q = next++; q < (int) todo.size(); q = next++)
                        {
                            const auto& m = z[(size_t) todo[(size_t) q].first];
                            out[(size_t) q] = playAt (A.zones[(size_t) todo[(size_t) q].first], m.root, m.fine, todo[(size_t) q].second, an, outMono[(size_t) q]);
                        }
                    });
                for (auto& t : ths) t.join();
                std::map<std::pair<int, int>, std::vector<float>> monos;
                for (size_t q = 0; q < todo.size(); ++q) { res[todo[q]] = std::move (out[q]); monos[todo[q]] = std::move (outMono[q]); }
                for (size_t j = 0; j < z.size(); ++j)
                {
                    z[j].yinOk = false;
                    if (! yinAnchors || ! z[j].usable) continue;
                    const auto& an = res[{ (int) j, z[j].root }];
                    if (an.confident)
                    {
                        z[j].yinDev = midiCents (an.f0) - z[j].root * 100.0;   // the played pitch against the key
                        z[j].yinOk = std::abs (z[j].yinDev) < 120.0;
                    }
                }
                std::vector<Pair> pairs;
                {
                    // the pairs measured in parallel (each compares two notes on the same window, an FFT or two)
                    std::vector<Pair> found (meets.size());
                    std::vector<char> okm (meets.size(), 0);
                    std::atomic<int> nx { 0 };
                    std::vector<std::thread> pt;
                    for (int t = 0; t < std::max (1, threads); ++t)
                        pt.emplace_back ([&]
                        {
                            vaultpitch::Analyser an;
                            for (int q = nx++; q < (int) meets.size(); q = nx++)
                            {
                                const auto& m = meets[(size_t) q];
                                const float expect = (float) ((m.kj - m.ki) * 100);
                                float cents = 0; double w = 0;
                                const double fB = 440.0 * std::pow (2.0, (m.kj - 69) / 12.0), fA = 440.0 * std::pow (2.0, (m.ki - 69) / 12.0);
                                const auto& ra = res.at ({ m.i, m.ki });
                                const auto& rb = res.at ({ m.j, m.kj });
                                const bool early = measureInterval (ra, rb, monos.at ({ m.j, m.kj }), fB, vaultpitch::lowCutRatio * fA, an, expect, cents, w, ! yinAnchors);
                                float c2 = 0; double w2 = 0;
                                const bool late = measureIntervalLate (ra, rb, monos.at ({ m.i, m.ki }), monos.at ({ m.j, m.kj }), fA, fB, an, expect, c2, w2, ! yinAnchors);
                                if (! early && ! late) continue;
                                if (! early) { cents = c2; w = w2; }
                                else if (late) { cents = (float) ((cents * w + c2 * w2) / (w + w2)); w = std::max (w, w2); }
                                auto& pr = found[(size_t) q];
                                pr.a = m.i; pr.b = m.j; pr.r = cents - expect;
                                pr.w = w * (m.ki == m.kj ? 0.5 : 1.0);   // neighbouring keys first: what the ear compares
                                okm[(size_t) q] = 1;
                            }
                        });
                    for (auto& t : pt) t.join();
                    for (size_t q = 0; q < meets.size(); ++q) if (okm[q]) pairs.push_back (found[q]);
                }
                pairCount = (int) pairs.size();
                if (const auto dbg = juce::SystemStats::getEnvironmentVariable ("RETUNE_DEBUG", {}); dbg.isNotEmpty() && (desc.id + "/" + ad.name).contains (dbg))
                {
                    std::printf ("   round %d: %d meets, %d pairs measured\n", round, (int) meets.size(), (int) pairs.size());
                    for (auto& m : meets)
                    {
                        const Pair* f = nullptr;
                        for (auto& pr : pairs) if (pr.a == m.i && pr.b == m.j) f = &pr;
                        std::printf ("     %3d@%d -> %3d@%d: %s\n", z[(size_t) m.i].root, m.ki, z[(size_t) m.j].root, m.kj,
                                     f != nullptr ? (juce::String (f->r, 1) + " w " + juce::String (f->w, 2)).toRawUTF8() : "not measured");
                    }
                }
                juce::String rlog;
                kept = solve (z, pairs, yinAnchors, rlog);
                if (round == 2) alog = rlog;
                // the global offset: keep the articulation's overall tuning where YIN is not trusted (the mean change is 0)
                if (! yinAnchors)
                {
                    double sum = 0; int n = 0;
                    for (auto& q : z) if (q.usable) { sum += q.d; ++n; }
                    if (n > 0) for (auto& q : z) q.d -= sum / n;
                }
                // YIN articulations: no zone is moved more than 10 cents away from its own YIN pitch (the neighbours of an
                // unsteady sample - ensemble tremolo, a pitch that drifts - must not pull it out of tune on its own)
                // (every key the zone was measured at in this round counts: a zone is also heard 3-4 keys from its root)
                if (yinAnchors)
                {
                    std::vector<double> lo (z.size(), -1.0e9), hi (z.size(), 1.0e9);
                    for (auto& kv : res)
                    {
                        const auto& an = kv.second;
                        if (! an.confident) continue;
                        const double dev = vaultpitch::foldOctave ((float) (midiCents (an.f0) - kv.first.second * 100.0));
                        if (std::abs (dev) >= 120.0) continue;
                        lo[(size_t) kv.first.first] = std::max (lo[(size_t) kv.first.first], dev - 10.0);
                        hi[(size_t) kv.first.first] = std::min (hi[(size_t) kv.first.first], dev + 10.0);
                    }
                    // ... unless three or more comparisons with other zones agree on the move (an FM clav sample whose
                    // attack misleads YIN by 25 cents is out of tune against every neighbour and velocity layer)
                    std::vector<int> touching (z.size(), 0);
                    for (auto& pr : pairs) { ++touching[(size_t) pr.a]; ++touching[(size_t) pr.b]; }
                    for (size_t j = 0; j < z.size(); ++j)
                        if (z[j].usable && lo[j] > -1.0e8 && touching[j] < 3)
                            z[j].d = lo[j] <= hi[j] ? juce::jlimit (lo[j], hi[j], z[j].d) : 0.5 * (lo[j] + hi[j]);
                }
                double maxD = 0;
                for (auto& q : z) if (q.usable) { q.fine -= (float) q.d; maxD = std::max (maxD, std::abs (q.d)); }
                // YIN articulations: the median YIN deviation (after this round's move) is the articulation's absolute
                // tuning error - the fit keeps the zones in tune with each other, this keeps the whole set on A = 440
                if (yinAnchors)
                {
                    std::vector<double> dev;
                    for (auto& q : z) if (q.usable && q.yinOk) dev.push_back (q.yinDev - q.d);
                    if (dev.size() >= 3)
                    {
                        std::nth_element (dev.begin(), dev.begin() + (long) dev.size() / 2, dev.end());
                        const double m = dev[dev.size() / 2];
                        for (auto& q : z) if (q.usable) q.fine -= (float) m;
                        maxD = std::max (maxD, std::abs (m));
                    }
                }
                if (maxD < 0.5) break;
            }
            for (auto& q : z) { q.d = (q.root * 100.0 - q.fine) - (q.root0 * 100.0 - q.fine0); q.fine = q.fine0; q.root = q.root0; }   // total change
            int moved = 0;
            double worst = 0;
            juce::String zlog;
            auto zonesArr = aj["zones"].getArray();
            for (auto& q : z)
            {
                ++totalZones;
                if (! q.usable) continue;
                // the zone's tuning after the pass: moved by d (>= 1 cent), else as loaded (the loader may already have
                // applied the built-in patch - then those values differ from library.json and are written / emitted too)
                double nf = q.fine;
                int nr = q.root;
                if (std::abs (q.d) >= 1.0)
                {
                    nf = q.fine - q.d;
                    const int shift = (int) std::lround (nf / 100.0);   // keep |fine| <= 50: root - n, fine - 100 n plays the same
                    nr -= shift; nf -= 100.0 * shift;
                    ++moved; ++changedZones;
                    worst = std::max (worst, std::abs (q.d));
                    if (std::abs (q.d) >= 30.0) { ++bigMoves; zlog << "      " << juce::String (zonesArr != nullptr ? (*zonesArr)[q.json]["file"].toString() : juce::String())
                                                                      .fromLastOccurrenceOf ("/", false, false)
                                                              << ": " << juce::String (q.d, 1) << " cents off (root " << q.root << " fine " << juce::String (q.fine, 1)
                                                              << " -> root " << nr << " fine " << juce::String (nf, 1) << ")\n"; }
                }
                if (zonesArr == nullptr) continue;
                // the patch keys on the values in library.json
                const auto& zj = (*zonesArr)[q.json];
                const int jRoot = (int) zj.getProperty ("root", 60);
                const float jFine = (float) (double) zj.getProperty ("fine", 0.0), outFine = (float) (std::round (nf * 10.0) / 10.0);
                if (jRoot == nr && std::abs (jFine - outFine) < 0.05f) continue;
                fixes.push_back ({ zj["file"].toString(), jRoot, jFine, nr, outFine });
                if (! dry)
                    if (auto* zo = zj.getDynamicObject())
                    {
                        zo->setProperty ("root", nr);
                        zo->setProperty ("fine", (double) outFine);
                    }
            }
            instLog << "   " << ad.name.paddedRight (' ', 16) << (int) z.size() << " zones, " << kept << " / " << pairCount << " pairs, "
                    << moved << " retuned (worst " << juce::String (worst, 1) << " c)" << (yinAnchors ? "" : " [relative only: " + mode + "]") << "\n" << alog << zlog;
            if (! dry && aj.getDynamicObject() != nullptr) aj.getDynamicObject()->setProperty ("retuned", true);
        }
        std::printf ("%s", instLog.toRawUTF8());
        report << instLog;
    }
    report << "\n" << changedZones << " of " << totalZones << " zones retuned, " << bigMoves << " by 30 cents or more\n";
    std::printf ("%d of %d zones retuned, %d by 30 cents or more%s\n", changedZones, totalZones, bigMoves, dry ? " (dry run: nothing written)" : "");
    outDir.getChildFile ("retune_report.txt").replaceWithText (report);
    if (emit != juce::File()) std::printf ("tuning patch: %d zones -> %s %s\n", (int) fixes.size(), emit.getFullPathName().toRawUTF8(),
                                           writePatch (emit, fixes) ? "written" : "NOT written");
    if (dry) return 0;
    const bool ok = jsonFile.replaceWithText (juce::JSON::toString (json, false));
    vault::SampleLibrary check (outDir);
    std::printf ("library.json %s, loader %s\n", ok ? "written" : "NOT written", check.libraryFound() ? "OK" : "FAILED");
    return ok && check.libraryFound() ? 0 : 1;
}
} // namespace retune
