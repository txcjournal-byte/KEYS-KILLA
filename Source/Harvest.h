#pragma once
#include <algorithm>
#include <cmath>
#include <vector>
#include "PairLab.h"
#include <cstdio>
#include <cstdlib>

// v0.21 HARVEST: drop a song / sample (or DIGGA's cuts) and KEYS KILLA collects new sounds from it:
// every hit and note is cut out, listened to (pitch, attack, decay, brightness, noise, low end) and sorted
// into the sound bank by character. The bank feeds PAIR / BREED / loops - no factory sounds needed.
namespace kk
{
enum HarvestCat { catBass, catKeys, catPluck, catPad, catLead, catVocal, catDrum, catFx, numCats };

inline const char* harvestCatName (int c)
{
    static const char* n[] { "BASS", "KEYS / PIANO", "PLUCK / BELL", "PAD / STRINGS", "LEAD / BRASS / FLUTE", "VOX", "DRUMS", "FX" };
    return n[std::clamp (c, 0, (int) numCats - 1)];
}
inline const char* harvestCatShort (int c)
{
    static const char* n[] { "BASS", "KEYS", "PLUCK", "PAD", "LEAD", "VOX", "DRUMS", "FX" };
    return n[std::clamp (c, 0, (int) numCats - 1)];
}

struct HarvestItem { PairPtr sound; int cat = catFx; float score = 0; };

class Harvest
{
public:
    static constexpr int perCategory = 8;

    // cut a recording into sounds and sort them (message / worker thread; a 3 minute song takes ~1-3 s)
    static std::vector<HarvestItem> run (const juce::AudioBuffer<float>& in, double srcRate, double rate, const juce::String& source)
    {
        std::vector<HarvestItem> out;
        auto whole = PairLab::fromBufferRaw (in, srcRate, rate, 600.0);
        if (whole.getNumSamples() < (int) (rate * 0.05)) return out;
        const auto cuts = segments (whole, rate);
        int idx = 0;
        for (auto [a, b] : cuts)
        {
            if (b - a < (int) (rate * 0.07)) continue;
            juce::AudioBuffer<float> seg (2, b - a);
            for (int c = 0; c < 2; ++c) seg.copyFrom (c, 0, whole, c, a, b - a);
            const auto f = features (seg, rate);
            if (f.rms < 0.004f) continue;                 // too quiet to be useful
            auto s = PairLab::fromBuffer (seg, rate, rate, source + " #" + juce::String (++idx));
            if (s == nullptr) continue;
            HarvestItem it; it.sound = s; it.cat = classify (f, *s); it.score = quality (f, *s, it.cat);
            if (std::getenv ("KK_HARVEST_DEBUG") != nullptr)
                std::printf ("  seg %.2f-%.2fs %s pitched %d root %d f0 %.0f harm %.2f flat %.2f low %.2f atk %.0f sus %.2f len %.2f var %.2f cen %.0f\n",
                             a / rate, b / rate, harvestCatShort (it.cat), (int) s->pitched, s->rootNote, f.f0, f.harmonic, f.flatness, f.lowRatio, f.attackMs, f.sustain, f.lenSec, f.pitchVar, f.centroid);
            auto& ps = const_cast<PairSound&> (*s);
            ps.method = harvestCatShort (it.cat);
            out.push_back (std::move (it));
        }
        return keepBest (std::move (out));
    }

    // merge new sounds into a bank (best per category, near-duplicates dropped)
    static void merge (std::vector<HarvestItem>& bank, std::vector<HarvestItem> add)
    {
        for (auto& a : add) bank.push_back (std::move (a));
        bank = keepBest (std::move (bank));
    }

private:
    struct Feat { float rms = 0, centroid = 0, flatness = 0, lowRatio = 0, attackMs = 0, sustain = 0, lenSec = 0, pitchVar = 0, harmonic = 0, f0 = 0; };

    // onsets from the energy flux of 10 ms frames; each sound runs to the next onset (max 2.5 s, min 70 ms)
    static std::vector<std::pair<int, int>> segments (const juce::AudioBuffer<float>& a, double rate)
    {
        const int hop = std::max (64, (int) (rate * 0.01)), frames = a.getNumSamples() / hop;
        std::vector<float> e ((size_t) frames, 0.0f), flux ((size_t) frames, 0.0f);
        for (int f = 0; f < frames; ++f)
        {
            float s = 0;
            for (int i = f * hop; i < (f + 1) * hop; ++i) { const float m = a.getSample (0, i) + a.getSample (1, i); s += m * m; }
            e[(size_t) f] = std::log (1.0e-7f + s / (float) hop);
        }
        for (int f = 1; f < frames; ++f) flux[(size_t) f] = std::max (0.0f, e[(size_t) f] - e[(size_t) f - 1]);
        // adaptive threshold: local median-ish mean + margin
        std::vector<int> on;
        const int w = 30, minGap = (int) (0.09 * rate / hop);
        for (int f = 1; f < frames - 1; ++f)
        {
            float m = 0; int c = 0;
            for (int j = std::max (0, f - w); j < std::min (frames, f + w); ++j) { m += flux[(size_t) j]; ++c; }
            m /= (float) c;
            if (flux[(size_t) f] > m * 2.2f + 0.6f && flux[(size_t) f] >= flux[(size_t) f - 1] && flux[(size_t) f] >= flux[(size_t) f + 1]
                && (on.empty() || f - on.back() >= minGap))
                on.push_back (f);
        }
        if (on.empty() || on.front() > 2) on.insert (on.begin(), 0);
        std::vector<std::pair<int, int>> cuts;
        const int maxLen = (int) (rate * 2.5);
        for (size_t k = 0; k < on.size(); ++k)
        {
            const int a0 = std::max (0, on[k] * hop - hop / 2);
            int b0 = k + 1 < on.size() ? on[k + 1] * hop - hop / 2 : a.getNumSamples();
            b0 = std::min (b0, a0 + maxLen);
            if (b0 > a0) cuts.push_back ({ a0, b0 });
        }
        return cuts;
    }

    static Feat features (const juce::AudioBuffer<float>& s, double rate)
    {
        Feat f;
        const int n = s.getNumSamples();
        f.lenSec = (float) (n / rate);
        std::vector<float> mono ((size_t) n);
        for (int i = 0; i < n; ++i) mono[(size_t) i] = 0.5f * (s.getSample (0, i) + s.getSample (1, i));
        double sum = 0; for (float v : mono) sum += v * v;
        f.rms = (float) std::sqrt (sum / std::max (1, n));
        // envelope: attack (time to 90 % of peak) and sustain (energy of the last half vs the first half)
        const int hop = std::max (32, (int) (rate * 0.005));
        std::vector<float> env;
        for (int i = 0; i + hop <= n; i += hop) { float m = 0; for (int j = i; j < i + hop; ++j) m = std::max (m, std::abs (mono[(size_t) j])); env.push_back (m); }
        if (! env.empty())
        {
            const float pk = *std::max_element (env.begin(), env.end());
            int ai = 0; while (ai < (int) env.size() && env[(size_t) ai] < pk * 0.9f) ++ai;
            f.attackMs = (float) (ai * hop / rate * 1000.0);
            double first = 0, second = 0; const size_t h = env.size() / 2;
            for (size_t i = 0; i < env.size(); ++i) (i < h ? first : second) += env[i];
            f.sustain = (float) (second / (first + 1e-9));
        }
        // spectrum of the steady part
        constexpr int order = 11, N = 1 << order;
        juce::dsp::FFT fft (order);
        std::vector<float> buf ((size_t) N * 2, 0.0f);
        const int st = std::min (std::max (0, n - N), (int) (rate * 0.03));
        for (int i = 0; i < N && st + i < n; ++i) buf[(size_t) i] = mono[(size_t) (st + i)] * (0.5f - 0.5f * std::cos (twoPi * (float) i / (float) N));
        fft.performFrequencyOnlyForwardTransform (buf.data());
        double num = 0, den = 0, lowE = 0, logSum = 0; int bins = 0;
        for (int k = 1; k < N / 2; ++k)
        {
            const double hz = k * rate / N, m = buf[(size_t) k] + 1e-9;
            num += hz * m; den += m;
            if (hz < 200.0) lowE += m * m;
            logSum += std::log (m); ++bins;
        }
        double tot = 0; for (int k = 1; k < N / 2; ++k) tot += (double) buf[(size_t) k] * buf[(size_t) k];
        f.centroid = (float) (num / (den + 1e-12));
        f.lowRatio = (float) (lowE / (tot + 1e-12));
        f.flatness = (float) (std::exp (logSum / std::max (1, bins)) / (den / std::max (1, bins) + 1e-12));
        // pitch stability over three windows (vocals / leads bend, keys stay)
        float p[3] {}; int ok = 0;
        for (int w = 0; w < 3; ++w)
        {
            const int a0 = (int) (n * (0.15 + 0.3 * w)), len = std::min ((int) (rate * 0.08), n - a0);
            const float hz = yin (mono.data() + a0, len, rate, f.harmonic);
            if (hz > 0) p[ok++] = hz;
        }
        if (ok >= 2) f.pitchVar = std::abs (12.0f * std::log2 (p[ok - 1] / p[0]));
        f.f0 = ok > 0 ? p[0] : 0.0f;
        return f;
    }
    static float yin (const float* x, int win, double rate, float& clarity)
    {
        const int maxLag = std::min ((int) (rate / 40.0), win / 2), minLag = (int) (rate / 1500.0);
        if (maxLag <= minLag + 2) return 0;
        double run = 0; int best = -1; double bestV = 1.0;
        for (int lag = 1; lag <= maxLag; ++lag)
        {
            double acc = 0;
            for (int i = 0; i + lag < win; ++i) { const double v = x[i] - x[i + lag]; acc += v * v; }
            run += acc;
            const double d = acc * lag / (run + 1e-12);
            if (lag >= minLag && d < bestV) { bestV = d; best = lag; }
            if (lag >= minLag && d < 0.12) { best = lag; bestV = d; break; }
        }
        clarity = std::max (clarity, (float) (1.0 - bestV));
        return best > 0 && bestV < 0.3 ? (float) (rate / best) : 0.0f;
    }

    static int classify (const Feat& f, const PairSound& s)
    {
        // bright saws / brass are flat-ish in the spectrum but strongly periodic: periodicity wins
        const bool pitched = s.pitched && ((f.harmonic > 0.7f && f.flatness < 0.35f) || (f.harmonic > 0.85f && f.flatness < 0.6f));
        if (! pitched)
        {
            if (f.attackMs < 25.0f && f.lenSec < 1.2f) return catDrum;
            return catFx;
        }
        const float hz = f.f0 > 0 ? f.f0 : 440.0f * std::exp2 ((s.rootNote - 69) / 12.0f);
        if (hz < 130.0f || (f.lowRatio > 0.55f && hz < 220.0f)) return catBass;
        if (f.pitchVar > 0.7f && f.centroid > 500.0f && f.centroid < 3500.0f && f.sustain > 0.45f) return catVocal;
        if (f.attackMs > 45.0f && f.sustain > 0.55f) return catPad;
        if (f.sustain > 0.6f && f.lenSec > 0.35f) return catLead;
        if (f.centroid > 2200.0f || f.lenSec < 0.35f) return catPluck;
        return catKeys;
    }
    static float quality (const Feat& f, const PairSound& s, int cat)
    {
        float q = std::min (1.0f, f.rms * 6.0f) + std::min (1.0f, f.lenSec);
        if (cat != catDrum && cat != catFx) q += s.pitched ? f.harmonic : 0.0f;
        return q;
    }
    static std::vector<HarvestItem> keepBest (std::vector<HarvestItem> all)
    {
        std::sort (all.begin(), all.end(), [] (const HarvestItem& a, const HarvestItem& b) { return a.score > b.score; });
        std::vector<HarvestItem> keep;
        int count[numCats] {};
        for (auto& it : all)
        {
            if (count[it.cat] >= perCategory) continue;
            bool dup = false;   // same note and nearly the same length = the same sound repeated in the song
            for (auto& k : keep)
                if (k.cat == it.cat && k.sound->rootNote == it.sound->rootNote
                    && std::abs (k.sound->audio.getNumSamples() - it.sound->audio.getNumSamples()) < it.sound->audio.getNumSamples() / 6) { dup = true; break; }
            if (dup) continue;
            ++count[it.cat];
            keep.push_back (std::move (it));
        }
        std::stable_sort (keep.begin(), keep.end(), [] (const HarvestItem& a, const HarvestItem& b) { return a.cat < b.cat; });
        return keep;
    }
};
} // namespace kk
