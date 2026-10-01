#pragma once
#include <array>
#include <memory>
#include <vector>
#include <cmath>
#include <juce_audio_formats/juce_audio_formats.h>
#include "Drums.h"

// v0.15 DIGGA: drop a sample, it is chopped (transients or even), the slices play on the keys
// (CHOP: C5 = slice 1, C#5 = slice 2 ...) or the whole sample plays chromatically (KEYS).
namespace kk
{
struct DiggaParams { int mode = 0; int slices = 16; int chop = 0; float pitch = 0; bool reverse = false; float level = 0; };

class Digga
{
public:
    struct Sample
    {
        juce::AudioBuffer<float> audio;
        double rate = 44100;
        std::vector<int> starts;          // slice start samples (sorted, starts[0] == 0)
        std::vector<float> peaks;         // waveform overview, 0..1
        juce::String name, path;
        int chopMode = -1, chopCount = 0;
        int sliceEnd (int k) const { return k + 1 < (int) starts.size() ? starts[(size_t) k + 1] : audio.getNumSamples(); }
    };

    void prepare (double sampleRate) { sr = sampleRate; for (auto& v : voices) v = {}; }

    // ---------- message thread ----------
    bool load (const juce::File& f, int chopMode, int count)
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr || r->lengthInSamples <= 0) return false;
        auto s = std::make_shared<Sample>();
        const int len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 600.0));   // up to 10 minutes
        s->audio.setSize (2, len);
        r->read (&s->audio, 0, len, 0, true, true);
        if (r->numChannels == 1) s->audio.copyFrom (1, 0, s->audio, 0, 0, len);
        s->rate = r->sampleRate; s->name = f.getFileNameWithoutExtension(); s->path = f.getFullPathName();
        const float pk = s->audio.getMagnitude (0, len);
        if (pk > 1.0e-6f && pk < 0.5f) s->audio.applyGain (0.7f / pk);   // quiet samples come up to a usable level
        overview (*s);
        chop (*s, chopMode, count);
        publish (s);
        return true;
    }
    void rechop (int chopMode, int count)
    {
        auto cur = current();
        if (cur == nullptr || (cur->chopMode == chopMode && cur->chopCount == count)) return;
        auto s = std::make_shared<Sample> (*cur);
        chop (*s, chopMode, count);
        publish (s);
    }
    void clear() { publish (nullptr); }
    std::shared_ptr<const Sample> current() const { const juce::SpinLock::ScopedLockType l (lock); return sample; }

    // ---------- audio thread ----------
    void add (const DrumEvent& e) { if (nEvents < (int) events.size()) events[(size_t) nEvents++] = e; }
    void allOff() { for (auto& v : voices) if (v.active) v.rel = true; }
    void process (float* L, float* R, int n, const DiggaParams& p)
    {
        const juce::SpinLock::ScopedTryLockType tl (lock);
        if (! tl.isLocked() || sample == nullptr) { nEvents = 0; if (tl.isLocked() && sample == nullptr) for (auto& v : voices) v.active = false; return; }
        const Sample& s = *sample;
        const float* src[2] { s.audio.getReadPointer (0), s.audio.getReadPointer (1) };
        const int total = s.audio.getNumSamples();
        const float gain = juce::Decibels::decibelsToGain (p.level);
        const double fadeIn = 1.0 / (0.002 * sr), fadeOut = 1.0 / (0.012 * sr);
        int ev = 0;
        for (int i = 0; i < n; ++i)
        {
            while (ev < nEvents && events[(size_t) ev].pos <= i) { handle (events[(size_t) ev], s, p); ++ev; }
            float l = 0, r = 0;
            for (auto& v : voices)
            {
                if (! v.active) continue;
                const double len = (double) (v.end - v.start);
                if (v.pos >= len - 1) { v.active = false; continue; }
                const double at = v.reverse ? (double) v.end - 1.0 - v.pos : (double) v.start + v.pos;
                const int i0 = juce::jlimit (0, total - 1, (int) at); const int i1 = std::min (total - 1, i0 + 1);
                const float fr = (float) (at - std::floor (at));
                float env = (float) std::min (1.0, v.pos / v.inc * fadeIn);
                const double left = (len - v.pos) / v.inc;           // output samples to the slice end
                env *= (float) std::min (1.0, left * fadeOut);
                if (v.rel) { v.relGain -= (float) fadeOut; if (v.relGain <= 0) { v.active = false; continue; } env *= v.relGain; }
                const float g = env * v.vel * gain;
                l += (src[0][i0] + (src[0][i1] - src[0][i0]) * fr) * g;
                r += (src[1][i0] + (src[1][i1] - src[1][i0]) * fr) * g;
                v.pos += v.inc;
            }
            L[i] += l; R[i] += r;
        }
        while (ev < nEvents) handle (events[(size_t) ev++], s, p);
        nEvents = 0;
    }
    bool busy() const { for (auto& v : voices) if (v.active) return true; return false; }
    // UI: 0..1 play position of the newest voice inside the sample, -1 = silent
    float playhead() const { return playPos.load(); }

private:
    struct Voice { bool active = false, rel = false, reverse = false; int note = -1, start = 0, end = 0; double pos = 0, inc = 1; float vel = 1, relGain = 1; };

    void handle (const DrumEvent& e, const Sample& s, const DiggaParams& p)
    {
        if (! e.on) { for (auto& v : voices) if (v.active && v.note == e.note) v.rel = true; return; }
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) v = &voices[(size_t) (rr++ % voices.size())];
        *v = {};
        v->active = true; v->note = e.note; v->vel = 0.3f + 0.7f * e.vel; v->reverse = p.reverse;
        double semis = p.pitch;
        if (p.mode == 0 && ! s.starts.empty())   // CHOP
        {
            const int k = ((e.note - 60) % (int) s.starts.size() + (int) s.starts.size()) % (int) s.starts.size();
            v->start = s.starts[(size_t) k]; v->end = s.sliceEnd (k);
        }
        else { v->start = 0; v->end = s.audio.getNumSamples(); semis += e.note - 60; }
        v->inc = s.rate / sr * std::exp2 (semis / 12.0);
        playPos = (float) v->start / (float) std::max (1, s.audio.getNumSamples());
    }

    void publish (std::shared_ptr<Sample> s)
    {
        std::shared_ptr<const Sample> old;
        { const juce::SpinLock::ScopedLockType l (lock); old = std::move (sample); sample = std::move (s); }
        // `old` is released here, on the message thread
    }
    static void overview (Sample& s)
    {
        const int cols = 600, len = s.audio.getNumSamples();
        s.peaks.assign ((size_t) cols, 0.0f);
        float mx = 1.0e-6f;
        for (int c = 0; c < cols; ++c)
        {
            const int a = (int) ((juce::int64) len * c / cols), b = std::max (a + 1, (int) ((juce::int64) len * (c + 1) / cols));
            s.peaks[(size_t) c] = std::max (s.audio.getMagnitude (0, a, b - a), s.audio.getMagnitude (1, a, b - a));
            mx = std::max (mx, s.peaks[(size_t) c]);
        }
        for (auto& x : s.peaks) x /= mx;
    }
    // chopMode 0 = transients, 1 = even
    static void chop (Sample& s, int chopMode, int count)
    {
        const int len = s.audio.getNumSamples();
        count = std::max (1, count);
        s.chopMode = chopMode; s.chopCount = count;
        s.starts.clear();
        if (chopMode == 1 || len < count * 2048)
        {
            for (int k = 0; k < count; ++k) s.starts.push_back ((int) ((juce::int64) len * k / count));
            return;
        }
        const int hop = std::max (64, (int) (s.rate * 0.005));
        const int frames = len / hop;
        std::vector<float> flux ((size_t) frames, 0.0f);
        float prev = 0;
        for (int f = 0; f < frames; ++f)
        {
            float e = 0;
            for (int i = f * hop; i < (f + 1) * hop; ++i) { const float m = s.audio.getSample (0, i) + s.audio.getSample (1, i); e += m * m; }
            const float le = std::log (1.0e-7f + e / (float) hop);
            flux[(size_t) f] = std::max (0.0f, le - prev);
            prev = le;
        }
        const int minGap = std::max (2, frames / (count * 3));
        std::vector<int> cand ((size_t) frames);
        for (int f = 0; f < frames; ++f) cand[(size_t) f] = f;
        std::sort (cand.begin(), cand.end(), [&] (int a, int b) { return flux[(size_t) a] > flux[(size_t) b]; });
        std::vector<int> picked { 0 };
        for (int f : cand)
        {
            if ((int) picked.size() >= count) break;
            if (flux[(size_t) f] <= 0.5f) break;
            bool ok = true;
            for (int q : picked) if (std::abs (q - f) < minGap) { ok = false; break; }
            if (ok) picked.push_back (f);
        }
        std::sort (picked.begin(), picked.end());
        for (int f : picked) s.starts.push_back (f * hop);
        // not enough hits: split the longest slices
        while ((int) s.starts.size() < count)
        {
            int best = 0, bestLen = 0;
            for (int k = 0; k < (int) s.starts.size(); ++k) { const int l = s.sliceEnd (k) - s.starts[(size_t) k]; if (l > bestLen) { bestLen = l; best = k; } }
            s.starts.insert (s.starts.begin() + best + 1, s.starts[(size_t) best] + bestLen / 2);
        }
    }

    double sr = 44100;
    mutable juce::SpinLock lock;
    std::shared_ptr<const Sample> sample;
    std::array<Voice, 8> voices;
    std::array<DrumEvent, 256> events;
    int nEvents = 0;
    unsigned rr = 0;
    std::atomic<float> playPos { -1.0f };
};
} // namespace kk
