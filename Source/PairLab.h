#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <complex>
#include <memory>
#include <vector>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include "DspUtil.h"
#include "Studio.h"

// v0.20 PAIR YOUR OWN: drop 2-4 of your own sounds (WAV, Digga one-shots ...) -> BREED -> 6 children that
// really are both parents (spectral morph, cross synthesis, layers, envelope swap), optionally turned into
// BIT / ATMOS / AMBIENT versions. Every child plays on the keys (its pitch is detected) and plays the loops.
namespace kk
{
struct PairSound
{
    juce::AudioBuffer<float> audio;      // stereo, at the plugin rate
    std::vector<float> peaks;            // overview 0..1
    juce::String name, method;           // method: how a child was made ("MORPH A>B 60%" ...)
    int rootNote = 60;                   // detected pitch (C5 when unpitched)
    bool pitched = false;
};
using PairPtr = std::shared_ptr<const PairSound>;

enum PairFlavor { flavorAny, flavorClean, flavorBit, flavorAtmos, flavorAmbient, numFlavors };

class PairLab
{
public:
    static constexpr int maxParents = 4, numKids = 6;
    static constexpr double maxSeconds = 10.0;

    // ---------- message thread ----------
    static PairPtr fromFile (const juce::File& f, double rate)
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr || r->lengthInSamples <= 0) return nullptr;
        const int len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * maxSeconds));
        juce::AudioBuffer<float> in (2, len);
        r->read (&in, 0, len, 0, true, true);
        if (r->numChannels == 1) in.copyFrom (1, 0, in, 0, 0, len);
        return fromBuffer (in, r->sampleRate, rate, f.getFileNameWithoutExtension());
    }
    // resample to the plugin rate (stereo), at most maxSec seconds
    static juce::AudioBuffer<float> fromBufferRaw (const juce::AudioBuffer<float>& in, double srcRate, double rate, double maxSec)
    {
        juce::AudioBuffer<float> out;
        if (in.getNumSamples() <= 0 || in.getNumChannels() <= 0) return out;
        const double ratio = srcRate / rate;
        const int len = std::min ((int) (rate * maxSec), std::max (1, (int) std::floor (in.getNumSamples() / ratio)));
        out.setSize (2, len);
        for (int c = 0; c < 2; ++c)
        {
            juce::LagrangeInterpolator li;
            li.process (ratio, in.getReadPointer (std::min (c, in.getNumChannels() - 1)), out.getWritePointer (c), len, in.getNumSamples(), 0);
        }
        return out;
    }
    static PairPtr fromBuffer (const juce::AudioBuffer<float>& in, double srcRate, double rate, const juce::String& name)
    {
        if (in.getNumSamples() <= 0 || in.getNumChannels() <= 0) return nullptr;
        auto s = std::make_shared<PairSound>();
        const double ratio = srcRate / rate;
        const int len = std::min ((int) (rate * maxSeconds), std::max (1, (int) std::floor (in.getNumSamples() / ratio)));
        s->audio.setSize (2, len);
        for (int c = 0; c < 2; ++c)
        {
            juce::LagrangeInterpolator li;
            li.process (ratio, in.getReadPointer (std::min (c, in.getNumChannels() - 1)), s->audio.getWritePointer (c), len, in.getNumSamples(), 0);
        }
        s->name = name;
        finish (*s, rate);
        return s;
    }

    // BREED: six children from the parents (2..4). flavor: what they turn into.
    static std::vector<PairPtr> breed (const std::vector<PairPtr>& parents, uint32_t seed, int flavor, double rate)
    {
        std::vector<PairPtr> kids;
        std::vector<PairPtr> ps;
        for (auto& p : parents) if (p != nullptr) ps.push_back (p);
        if (ps.empty()) return kids;
        Rng rng; rng.seed (hash32 (seed * 2654435761u + 17u));
        static constexpr int methods = 5;
        for (int k = 0; k < numKids; ++k)
        {
            // pick a pair (a single parent breeds with itself: variations)
            const int ia = (int) (rng.uni() * (float) ps.size()) % (int) ps.size();
            int ib = ps.size() > 1 ? (int) (rng.uni() * (float) (ps.size() - 1)) % (int) (ps.size() - 1) : ia;
            if (ps.size() > 1 && ib >= ia) ++ib;
            const auto& A = *ps[(size_t) ia];
            const auto& B = *ps[(size_t) ib];
            const int method = (k + (int) (rng.uni() * 3.0f)) % methods;    // every BREED shows different methods
            const float t = 0.3f + 0.4f * rng.uni();
            auto kid = std::make_shared<PairSound>();
            juce::String how;
            switch (method)
            {
                case 0: morph (A, B, t, kid->audio); how = "MORPH " + String (juce::roundToInt (t * 100)) + "%"; break;
                case 1: cross (A, B, kid->audio); how = "CROSS"; break;                 // A's notes, B's colour
                case 2: layer (A, B, t, rate, kid->audio); how = "LAYER"; break;
                case 3: envSwap (A, B, kid->audio); how = "SHAPE"; break;               // B played with A's envelope
                default: cross (B, A, kid->audio); how = "CROSS"; break;                // B's notes, A's colour
            }
            int fl = flavor;
            if (fl == flavorAny) { const float x = rng.uni(); fl = x < 0.45f ? flavorClean : x < 0.65f ? flavorBit : x < 0.85f ? flavorAtmos : flavorAmbient; }
            if (fl == flavorBit) { bit (kid->audio, rng, rate); how += " + BIT"; }
            else if (fl == flavorAtmos) { atmos (kid->audio, rng, rate); how += " + ATMOS"; }
            else if (fl == flavorAmbient) { ambient (kid->audio, rng, rate); how += " + AMBIENT"; }
            kid->name = A.name.substring (0, 14) + (ia == ib ? String() : " x " + B.name.substring (0, 14));
            kid->method = how;
            finish (*kid, rate);
            kids.push_back (kid);
        }
        return kids;
    }

    static juce::File exportWav (const PairSound& s, double rate, const juce::String& tag)
    {
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Pair");
        dir.createDirectory();
        auto f = dir.getChildFile (juce::File::createLegalFileName ("KK " + tag + " - " + s.name).substring (0, 110) + ".wav");
        f.deleteFile();
        juce::WavAudioFormat wav;
        auto os = std::make_unique<juce::FileOutputStream> (f);
        if (os->openedOk())
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), rate, 2, 24, {}, 0)))
            {
                os.release();
                w->writeFromAudioSampleBuffer (s.audio, 0, s.audio.getNumSamples());
            }
        return f;
    }

    // ---------- audio thread: the selected sound on the keys / in the loops ----------
    void setSound (PairPtr s) { const juce::SpinLock::ScopedLockType l (lock); playing = std::move (s); }
    PairPtr sound() const { const juce::SpinLock::ScopedLockType l (lock); return playing; }
    void noteOn (int note, float vel, int offset)
    {
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) v = &voices[(size_t) (rr++ % voices.size())];
        *v = {};
        v->active = true; v->note = note; v->vel = 0.25f + 0.75f * vel; v->delay = offset;
    }
    void noteOff (int note) { for (auto& v : voices) if (v.active && v.note == note && ! v.rel) { v.rel = true; break; } }
    void allOff() { for (auto& v : voices) v.rel = true; }
    void render (float* L, float* R, int n, double rate)
    {
        const juce::SpinLock::ScopedTryLockType tl (lock);
        if (! tl.isLocked() || playing == nullptr) return;
        const auto& s = *playing;
        const int total = s.audio.getNumSamples();
        const float* a = s.audio.getReadPointer (0); const float* b = s.audio.getReadPointer (1);
        const float relStep = 1.0f / (0.12f * (float) rate);
        for (auto& v : voices)
        {
            if (! v.active) continue;
            const double inc = std::exp2 ((v.note - s.rootNote) / 12.0);
            for (int i = 0; i < n; ++i)
            {
                if (v.delay > 0) { --v.delay; continue; }
                const int i0 = (int) v.pos;
                if (i0 + 1 >= total) { v.active = false; break; }
                const float fr = (float) (v.pos - i0);
                float g = v.vel * 0.8f;
                if (v.rel) { v.relGain -= relStep; if (v.relGain <= 0) { v.active = false; break; } g *= v.relGain; }
                L[i] += (a[i0] + (a[i0 + 1] - a[i0]) * fr) * g;
                R[i] += (b[i0] + (b[i0 + 1] - b[i0]) * fr) * g;
                v.pos += inc;
            }
        }
    }

private:
    struct Voice { bool active = false, rel = false; int note = 60, delay = 0; double pos = 0; float vel = 1, relGain = 1; };
    using String = juce::String;
    static constexpr int fftOrder = 11, N = 1 << fftOrder, hop = N / 4;

    // ---------------- STFT helpers ----------------
    struct Spectrum { std::vector<std::vector<std::complex<float>>> frames; int length = 0; };
    static const std::vector<float>& window()
    {
        static const std::vector<float> w = [] { std::vector<float> v ((size_t) N); for (int i = 0; i < N; ++i) v[(size_t) i] = 0.5f - 0.5f * std::cos (twoPi * (float) i / (float) N); return v; }();
        return w;
    }
    static Spectrum analyse (const juce::AudioBuffer<float>& a)
    {
        Spectrum sp; sp.length = a.getNumSamples();
        juce::dsp::FFT fft (fftOrder);
        const auto& w = window();
        std::vector<std::complex<float>> in ((size_t) N), out ((size_t) N);
        for (int pos = -N + hop; pos < sp.length; pos += hop)
        {
            for (int i = 0; i < N; ++i)
            {
                const int s = pos + i;
                const float x = s >= 0 && s < sp.length ? 0.5f * (a.getSample (0, s) + a.getSample (1, s)) : 0.0f;
                in[(size_t) i] = { x * w[(size_t) i], 0.0f };
            }
            fft.perform (in.data(), out.data(), false);
            sp.frames.emplace_back (out.begin(), out.begin() + N / 2 + 1);
        }
        return sp;
    }
    static void synth (const Spectrum& sp, int length, juce::AudioBuffer<float>& out)
    {
        out.setSize (2, length); out.clear();
        juce::dsp::FFT fft (fftOrder);
        const auto& w = window();
        std::vector<std::complex<float>> in ((size_t) N), res ((size_t) N);
        int pos = -N + hop;
        for (auto& fr : sp.frames)
        {
            for (int k = 0; k <= N / 2; ++k) in[(size_t) k] = fr[(size_t) k];
            for (int k = 1; k < N / 2; ++k) in[(size_t) (N - k)] = std::conj (fr[(size_t) k]);
            fft.perform (in.data(), res.data(), true);
            for (int i = 0; i < N; ++i)
            {
                const int s = pos + i;
                if (s >= 0 && s < length) out.addSample (0, s, res[(size_t) i].real() * w[(size_t) i] * (2.0f / 3.0f));
            }
            pos += hop;
        }
        out.copyFrom (1, 0, out, 0, 0, length);
    }
    static float magAt (const Spectrum& sp, float frame, int bin)
    {
        if (sp.frames.empty()) return 0.0f;
        const int f = std::clamp ((int) frame, 0, (int) sp.frames.size() - 1);
        return std::abs (sp.frames[(size_t) f][(size_t) bin]);
    }
    static std::vector<float> envelope (const std::vector<std::complex<float>>& fr)   // smoothed spectral envelope
    {
        const int K = (int) fr.size();
        std::vector<float> m ((size_t) K), e ((size_t) K);
        for (int k = 0; k < K; ++k) m[(size_t) k] = std::abs (fr[(size_t) k]);
        for (int k = 0; k < K; ++k)
        {
            const int w = 2 + k / 12;   // wider at the top (roughly constant-Q)
            float s = 0; int c = 0;
            for (int j = std::max (0, k - w); j <= std::min (K - 1, k + w); ++j) { s += m[(size_t) j]; ++c; }
            e[(size_t) k] = s / (float) c + 1.0e-6f;
        }
        return e;
    }

    // ---------------- the ways to breed ----------------
    // MORPH: magnitudes blended geometrically, A's phases (A's timing, a sound between both)
    static void morph (const PairSound& A, const PairSound& B, float t, juce::AudioBuffer<float>& out)
    {
        auto a = analyse (A.audio); const auto b = analyse (B.audio);
        const float scale = (float) b.frames.size() / (float) std::max<size_t> (1, a.frames.size());
        for (size_t f = 0; f < a.frames.size(); ++f)
            for (int k = 0; k <= N / 2; ++k)
            {
                auto& c = a.frames[f][(size_t) k];
                const float ma = std::abs (c) + 1.0e-9f, mb = magAt (b, (float) f * scale, k) + 1.0e-9f;
                c *= std::pow (mb / ma, t);
            }
        synth (a, A.audio.getNumSamples(), out);
    }
    // CROSS: A's notes / fine structure through B's spectral envelope (talkbox-style cross synthesis)
    static void cross (const PairSound& A, const PairSound& B, juce::AudioBuffer<float>& out)
    {
        auto a = analyse (A.audio); const auto b = analyse (B.audio);
        const float scale = (float) b.frames.size() / (float) std::max<size_t> (1, a.frames.size());
        for (size_t f = 0; f < a.frames.size(); ++f)
        {
            const auto ea = envelope (a.frames[f]);
            const auto eb = envelope (b.frames[(size_t) std::clamp ((int) ((float) f * scale), 0, (int) b.frames.size() - 1)]);
            for (int k = 0; k <= N / 2; ++k) a.frames[f][(size_t) k] *= std::min (40.0f, eb[(size_t) k] / ea[(size_t) k]);
        }
        synth (a, A.audio.getNumSamples(), out);
    }
    // LAYER: B tuned to A's pitch, stacked, B slightly later and wider
    static void layer (const PairSound& A, const PairSound& B, float t, double rate, juce::AudioBuffer<float>& out)
    {
        const double ratio = A.pitched && B.pitched ? std::exp2 ((A.rootNote - B.rootNote) / 12.0) : 1.0;
        const int len = A.audio.getNumSamples();
        out.makeCopyOf (A.audio);
        out.applyGain (1.0f - 0.5f * t);
        juce::AudioBuffer<float> bt (2, len); bt.clear();
        for (int c = 0; c < 2; ++c)
        {
            juce::LagrangeInterpolator li;
            const int avail = (int) std::floor ((B.audio.getNumSamples() - 4) / ratio);
            li.process (ratio, B.audio.getReadPointer (c), bt.getWritePointer (c), std::min (len, std::max (0, avail)), B.audio.getNumSamples(), 0);
        }
        const int off = (int) (rate * 0.006);
        for (int c = 0; c < 2; ++c)
            for (int i = off; i < len; ++i) out.addSample (c, i, bt.getSample (c, i - off) * (0.4f + 0.6f * t) * (c == 0 ? 1.0f : 0.92f));
    }
    // SHAPE: B's sound with A's dynamics (A's attack, decay, rhythm)
    static void envSwap (const PairSound& A, const PairSound& B, juce::AudioBuffer<float>& out)
    {
        const int len = A.audio.getNumSamples();
        out.setSize (2, len); out.clear();
        auto env = [] (const juce::AudioBuffer<float>& x, int i, float& e)
        { const float v = std::max (std::abs (x.getSample (0, i)), std::abs (x.getSample (1, i))); e = v > e ? v : e * 0.9993f; return e; };
        float ea = 0, eb = 0;
        const int bl = B.audio.getNumSamples();
        for (int i = 0; i < len; ++i)
        {
            const int j = bl > 0 ? i % bl : 0;
            const float ga = env (A.audio, i, ea), gb = env (B.audio, j, eb);
            const float g = ga / (gb + 0.02f);
            for (int c = 0; c < 2; ++c) out.setSample (c, i, B.audio.getSample (c, j) * std::min (g, 8.0f));
        }
    }
    // ---------------- flavours ----------------
    static void bit (juce::AudioBuffer<float>& a, Rng& rng, double rate)
    {
        const float bits = 5.0f + rng.uni() * 5.0f, q = std::exp2 (bits - 1.0f);
        const int hold = std::max (1, (int) (rate / (6000.0 + rng.uni() * 10000.0)));
        for (int c = 0; c < 2; ++c)
        {
            float held = 0;
            for (int i = 0; i < a.getNumSamples(); ++i)
            {
                if (i % hold == 0) held = std::round (a.getSample (c, i) * q) / q;
                a.setSample (c, i, held);
            }
        }
    }
    static void atmos (juce::AudioBuffer<float>& a, Rng& rng, double rate)
    {
        const int tail = (int) (rate * 3.0);
        juce::AudioBuffer<float> o (2, a.getNumSamples() + tail); o.clear();
        for (int c = 0; c < 2; ++c) o.copyFrom (c, 0, a, c, 0, a.getNumSamples());
        OnePole lp[2]; for (auto& l : lp) l.setHz (2500.0f + rng.uni() * 3000.0f, (float) rate);
        for (int c = 0; c < 2; ++c) for (int i = 0; i < o.getNumSamples(); ++i) o.setSample (c, i, lp[c].lp (o.getSample (c, i)));
        FdnReverb v; v.prepare (rate);
        v.process (o.getWritePointer (0), o.getWritePointer (1), o.getNumSamples(), 0, 0.85f, 0.65f, false);
        a = std::move (o);
    }
    // AMBIENT: the sound smeared into a slow evolving pad (spectral blur with random phases, 2x longer)
    static void ambient (juce::AudioBuffer<float>& a, Rng& rng, double rate)
    {
        auto sp = analyse (a);
        Spectrum o;
        const int frames = (int) sp.frames.size() * 2 + (int) (rate * 2.0 / hop);
        o.frames.resize ((size_t) frames, std::vector<std::complex<float>> ((size_t) (N / 2 + 1)));
        std::vector<float> avg ((size_t) (N / 2 + 1), 0.0f);
        for (int f = 0; f < frames; ++f)
        {
            const int src = std::min ((int) sp.frames.size() - 1, f / 2);
            for (int k = 0; k <= N / 2; ++k)
            {
                const float m = src >= 0 ? std::abs (sp.frames[(size_t) src][(size_t) k]) : 0.0f;
                avg[(size_t) k] = avg[(size_t) k] * 0.94f + m * 0.06f;
                const float fade = f < frames - (int) (rate * 1.5 / hop) ? 1.0f : (float) (frames - f) / (float) (rate * 1.5 / hop);
                o.frames[(size_t) f][(size_t) k] = std::polar (avg[(size_t) k] * 1.6f * fade, (float) (rng.uni() * twoPi));
            }
        }
        const int len = frames * hop;
        juce::AudioBuffer<float> out;
        synth (o, len, out);
        // a little stereo: decorrelate the right side
        DelayLine d; d.prepare ((int) (0.03 * rate) + 8);
        for (int i = 0; i < len; ++i) { d.push (out.getSample (1, i)); out.setSample (1, i, d.read ((float) (0.017 * rate))); }
        a = std::move (out);
    }

    // ---------------- shared ----------------
    static void finish (PairSound& s, double rate)
    {
        auto& a = s.audio;
        // remove DC, normalise to -1 dBFS, fade the end, trim silence
        for (int c = 0; c < 2; ++c)
        {
            float x1 = 0, y1 = 0;
            for (int i = 0; i < a.getNumSamples(); ++i) { const float x = a.getSample (c, i); const float y = x - x1 + 0.9995f * y1; x1 = x; y1 = y; a.setSample (c, i, y); }
        }
        int last = a.getNumSamples() - 1;
        while (last > 256 && std::abs (a.getSample (0, last)) < 2.0e-4f && std::abs (a.getSample (1, last)) < 2.0e-4f) --last;
        a.setSize (2, last + 1, true);
        const float pk = a.getMagnitude (0, a.getNumSamples());
        if (pk > 1.0e-6f) a.applyGain (0.89f / pk);
        const int fade = std::min (a.getNumSamples() / 4, (int) (rate * 0.02));
        for (int c = 0; c < 2; ++c) a.applyGainRamp (c, a.getNumSamples() - fade, fade, 1.0f, 0.0f);
        // overview
        const int cols = 200, len = a.getNumSamples();
        s.peaks.assign ((size_t) cols, 0.0f);
        for (int c = 0; c < cols && len > 0; ++c)
        {
            const int s0 = (int) ((juce::int64) len * c / cols), s1 = std::max (s0 + 1, (int) ((juce::int64) len * (c + 1) / cols));
            s.peaks[(size_t) c] = std::max (a.getMagnitude (0, s0, s1 - s0), a.getMagnitude (1, s0, s1 - s0));
        }
        detectPitch (s, rate);
    }
    // pitch: autocorrelation of the steadiest 150 ms after the attack, 55 - 1100 Hz
    static void detectPitch (PairSound& s, double rate)
    {
        const int len = s.audio.getNumSamples();
        const int start = std::min (len - 1, (int) (0.05 * rate)), win = std::min (len - start, (int) (0.15 * rate));
        s.pitched = false; s.rootNote = 60;
        if (win < (int) (rate / 55.0) * 2) return;
        const float* x = s.audio.getReadPointer (0) + start;
        const int minLag = (int) (rate / 1100.0), maxLag = (int) (rate / 55.0);
        double e0 = 0; for (int i = 0; i < win; ++i) e0 += x[i] * x[i];
        if (e0 < 1.0e-6) return;
        // normalised difference (YIN-style) with the first dip below the threshold
        std::vector<double> d ((size_t) maxLag + 1, 0.0);
        double run = 0;
        int best = -1;
        for (int lag = 1; lag <= maxLag && lag < win / 2; ++lag)
        {
            double acc = 0;
            for (int i = 0; i + lag < win; ++i) { const double v = x[i] - x[i + lag]; acc += v * v; }
            run += acc;
            d[(size_t) lag] = acc * lag / (run + 1e-12);
            if (lag >= minLag && d[(size_t) lag] < 0.15 && best < 0) best = lag;
            if (best > 0 && lag > best && d[(size_t) lag] > d[(size_t) best]) break;
            if (best > 0 && d[(size_t) lag] < d[(size_t) best]) best = lag;
        }
        if (best <= 0) return;
        const double hz = rate / best;
        s.rootNote = std::clamp ((int) std::lround (69.0 + 12.0 * std::log2 (hz / 440.0)), 24, 108);
        s.pitched = true;
    }

    mutable juce::SpinLock lock;
    PairPtr playing;
    std::array<Voice, 12> voices;
    unsigned rr = 0;
};
} // namespace kk
