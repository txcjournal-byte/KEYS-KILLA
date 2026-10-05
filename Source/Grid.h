#pragma once
#include <juce_audio_formats/juce_audio_formats.h>
#include <algorithm>
#include <vector>
#include "Words.h"

// v0.45 GRID: a page of graph paper (16 x 10 squares). Every square holds a tiny hidden sound gene and its PLACE means something:
// left -> right = dark -> bright, top -> bottom = short -> long, and faint colour zones = material families (what excites the
// matter x the body that resonates).  Up to 10 chosen squares blend into ONE sound (an alchemy recipe + sculpt).
// The WAV under the grid can be edited: reverse, trim start / end, longer / shorter (resampled), fade in / out.
namespace kk::grid
{
constexpr int cols = 16, rows = 10, maxPicks = 10;
using Recipe = kk::words::Recipe;

struct Zone { const char* name; const char* word; int exc, body; juce::uint32 colour; float cx, cy; };
inline const std::vector<Zone>& zones()
{
    static const std::vector<Zone> z {
        { "METAL", "Metal", 0, 3, 0xff9fb4d8, 0.12f, 0.22f },
        { "AIR", "Air", 3, 1, 0xff8ff0c0, 0.48f, 0.12f },
        { "SPARK", "Spark", 2, 3, 0xff5ad1ff, 0.88f, 0.25f },
        { "BONE", "Bone", 1, 0, 0xfff1d9a8, 0.18f, 0.78f },
        { "LIQUID", "Liquid", 1, 2, 0xff3dd6c6, 0.55f, 0.62f },
        { "MAGMA", "Magma", 4, 4, 0xffff5a3c, 0.88f, 0.85f },
    };
    return z;
}
inline int cellIndex (int c, int r) { return r * cols + c; }
inline int cellCol (int i) { return i % cols; }
inline int cellRow (int i) { return i / cols; }
inline float cellX (int i) { return (float) cellCol (i) / (float) (cols - 1); }
inline float cellY (int i) { return (float) cellRow (i) / (float) (rows - 1); }

// the zone a square lies in (nearest centre, the border wobbles a little so the zones look grown, not drawn)
inline int zoneOf (int i)
{
    const float x = cellX (i), y = cellY (i);
    const float wob = ((float) (kk::hash32 ((uint32_t) i * 2654435761u + 5u) % 1000u) / 1000.0f - 0.5f) * 0.05f;
    int best = 0; float bd = 1.0e9f;
    for (int k = 0; k < (int) zones().size(); ++k)
    {
        const auto& z = zones()[(size_t) k];
        const float dx = (x - z.cx) * 1.6f, dy = y - z.cy, d = dx * dx + dy * dy + wob * (float) (k % 3);
        if (d < bd) { bd = d; best = k; }
    }
    return best;
}

// the hidden gene of a square
struct Gene { float matter, size, heat, cool; uint32_t seed; };
inline Gene geneOf (int i)
{
    Rng r; r.seed (kk::hash32 ((uint32_t) i * 40503u + 911u));
    Gene g;
    g.matter = 0.15f + 0.7f * r.uni(); g.size = 0.2f + 0.6f * r.uni();
    const float a = r.uni(), b = r.uni();
    g.heat = a > 0.75f ? (a - 0.75f) * 2.4f : 0.0f; g.cool = b > 0.8f ? (b - 0.8f) * 3.0f : 0.0f;
    g.seed = r.next() | 1u;
    return g;
}
// the place: left = dark, right = bright; top = short, bottom = long
inline float brightOf (int i) { return -0.9f + 1.8f * cellX (i); }
inline float stretchOf (int i) { return -0.9f + 1.8f * cellY (i); }

// the chosen squares -> one recipe
inline Recipe blend (const std::vector<int>& picks)
{
    Recipe r;
    if (picks.empty()) return r;
    std::array<int, 8> count {}; std::array<int, 8> first {}; first.fill (1 << 20);
    float m = 0, s = 0, h = 0, c = 0, br = 0, st = 0;
    std::vector<int> sorted = picks; std::sort (sorted.begin(), sorted.end());
    uint32_t mix = 0;
    for (int k = 0; k < (int) picks.size(); ++k)
    {
        const int i = picks[(size_t) k];
        const auto g = geneOf (i); const int z = zoneOf (i);
        ++count[(size_t) z]; first[(size_t) z] = std::min (first[(size_t) z], k);
        m += g.matter; s += g.size; h += g.heat; c += g.cool; br += brightOf (i); st += stretchOf (i);
    }
    for (auto i : sorted) mix = kk::hash32 (mix * 31u + (uint32_t) i + 1u);
    const float n = (float) picks.size();
    int zBest = 0, zSecond = -1;
    for (int z = 1; z < (int) zones().size(); ++z)
        if (count[(size_t) z] > count[(size_t) zBest] || (count[(size_t) z] == count[(size_t) zBest] && first[(size_t) z] < first[(size_t) zBest])) zBest = z;
    for (int z = 0; z < (int) zones().size(); ++z)
        if (z != zBest && count[(size_t) z] > 0 && (zSecond < 0 || count[(size_t) z] > count[(size_t) zSecond] || (count[(size_t) z] == count[(size_t) zSecond] && first[(size_t) z] < first[(size_t) zSecond]))) zSecond = z;
    int distinct = 0; for (auto x : count) distinct += x > 0;
    const auto& zb = zones()[(size_t) zBest];
    r.exc = zb.exc;
    r.body = zSecond >= 0 ? zones()[(size_t) zSecond].body : zb.body;     // two families: the first one strikes, the second one resonates
    r.matter = juce::jlimit (0.0f, 1.0f, m / n); r.size = juce::jlimit (0.0f, 1.0f, s / n);
    r.heat = juce::jlimit (0.0f, 1.0f, h / n); r.cool = juce::jlimit (0.0f, 1.0f, c / n);
    r.bright = juce::jlimit (-1.0f, 1.0f, br / n); r.stretch = juce::jlimit (-1.0f, 1.0f, st / n);
    r.split = distinct >= 3 ? juce::jmin (0.8f, 0.2f * (float) (distinct - 2)) : 0.0f;
    r.seed = mix | 1u;
    r.name = juce::String (zb.word) + (zSecond >= 0 ? juce::String (" ") + zones()[(size_t) zSecond].word : juce::String()) + " Grid " + juce::String ((int) (mix % 90u) + 10);
    return r;
}

// ---- the WAV edit: reverse -> trim (start / end, 0..1 of what you see) -> longer / shorter (resampled) -> fade in / out
struct Edit
{
    float start = 0, end = 1, length = 1, fadeIn = 0, fadeOut = 0; bool reverse = false;
    bool neutral() const { return start <= 0 && end >= 1 && length == 1 && fadeIn <= 0 && fadeOut <= 0 && ! reverse; }
};

inline juce::AudioBuffer<float> apply (const juce::AudioBuffer<float>& src, const Edit& e)
{
    juce::AudioBuffer<float> out;
    const int n = src.getNumSamples(), chs = src.getNumChannels();
    if (n <= 0 || chs <= 0) return out;
    const float s0 = juce::jlimit (0.0f, 1.0f, std::min (e.start, e.end)), s1 = juce::jlimit (0.0f, 1.0f, std::max (e.start, e.end));
    const int a = (int) std::floor (s0 * (float) n), b = std::max (a + 1, (int) std::floor (s1 * (float) n));
    const int seg = std::min (b, n) - a;
    if (seg <= 0) return out;
    const double len = juce::jlimit (0.25, 4.0, (double) e.length);
    const int outN = std::max (1, (int) std::lround ((double) seg * len));
    out.setSize (chs, outN);
    for (int ch = 0; ch < chs; ++ch)
    {
        const float* in = src.getReadPointer (ch);
        float* o = out.getWritePointer (ch);
        auto at = [&] (int k) { k = juce::jlimit (0, seg - 1, k); return in[e.reverse ? n - 1 - (a + k) : a + k]; };
        if (outN == seg) for (int i = 0; i < outN; ++i) o[i] = at (i);
        else
            for (int i = 0; i < outN; ++i)
            {
                const double pos = (double) i * (double) (seg - 1) / (double) std::max (1, outN - 1);
                const int k = (int) pos; const float f = (float) (pos - k);
                o[i] = at (k) + (at (k + 1) - at (k)) * f;
            }
        const int fi = (int) std::floor (juce::jlimit (0.0f, 1.0f, e.fadeIn) * (float) outN), fo = (int) std::floor (juce::jlimit (0.0f, 1.0f, e.fadeOut) * (float) outN);
        for (int i = 0; i < fi; ++i) o[i] *= (float) i / (float) fi;
        for (int i = 0; i < fo; ++i) o[outN - 1 - i] *= (float) i / (float) fo;
    }
    return out;
}

// the reversed view of the source (what the trim handles are drawn over)
inline std::vector<float> peaks (const juce::AudioBuffer<float>& b, int count, bool reverse = false)
{
    std::vector<float> p ((size_t) std::max (1, count), 0.0f);
    const int n = b.getNumSamples();
    if (n <= 0) return p;
    for (int k = 0; k < count; ++k)
    {
        const int i0 = (int) ((juce::int64) k * n / count), i1 = std::max (i0 + 1, (int) ((juce::int64) (k + 1) * n / count));
        float mx = 0;
        for (int ch = 0; ch < b.getNumChannels(); ++ch) for (int i = i0; i < i1 && i < n; ++i) mx = std::max (mx, std::abs (b.getSample (ch, i)));
        p[(size_t) (reverse ? count - 1 - k : k)] = mx;
    }
    return p;
}

inline bool writeWav (const juce::File& f, const juce::AudioBuffer<float>& b, double rate)
{
    if (b.getNumSamples() <= 0) return false;
    f.deleteFile();
    juce::WavAudioFormat wav;
    auto os = std::make_unique<juce::FileOutputStream> (f);
    if (! os->openedOk()) return false;
    if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), rate, (unsigned int) b.getNumChannels(), 24, {}, 0)))
    {
        os.release();
        return w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
    return false;
}
} // namespace kk::grid
