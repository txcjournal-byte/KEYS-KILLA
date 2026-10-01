#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>
#include <juce_audio_formats/juce_audio_formats.h>
#include "DspUtil.h"
#include "Studio.h"

// v0.18 DRUM BOOST: drop your 808 / snare / clap / hi-hat (a WAV from FL), boost it, play it on the keys,
// drag the boosted WAV back into the FL channel rack. Rendered offline (message thread) - no DSP on the
// audio thread except playing the finished sample.
namespace kk
{
enum DrumKind { drum808, drumSnare, drumHat, numDrumKinds };

struct BoostParams
{
    float gain = 0;      // dB
    float pitch = 0;     // semitones (resampled, like a sampler)
    float punch = 0;     // transient boost 0..1
    float drive = 0;     // 0..1
    int   sat = 0;       // 0 tape, 1 tube, 2 fold
    float clip = 0;      // clipper drive dB 0..18
    float low = 0;       // 808: SUB shelf, snare: BODY bell, hat: (unused)   -1..1
    float high = 0;      // 808: TONE (grit), snare: SNAP, hat: AIR          -1..1
    float decay = 1;     // 0.05..1 (1 = the original length)
    float room = 0;      // snare / clap: short plate 0..1
    float width = 0;     // stereo above 300 Hz 0..1
    float deres = 0;     // hat: 4 kHz de-resonator 0..1
};

struct DrumSample
{
    juce::AudioBuffer<float> audio;      // boosted, at the plugin rate
    std::vector<float> peaks, peaksDry;  // overviews 0..1
    int rootNote = 60;                   // 808: detected note (C5 = 60 when unknown)
    float rootHz = 0;
    juce::String name;
};

class DrumBoost
{
public:
    // ---------- message thread ----------
    bool load (const juce::File& f, double sampleRate)
    {
        juce::AudioFormatManager fm; fm.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (f));
        if (r == nullptr || r->lengthInSamples <= 0) return false;
        const int len = (int) std::min<juce::int64> (r->lengthInSamples, (juce::int64) (r->sampleRate * 30.0));
        juce::AudioBuffer<float> in (2, len);
        r->read (&in, 0, len, 0, true, true);
        if (r->numChannels == 1) in.copyFrom (1, 0, in, 0, 0, len);
        // to the plugin rate once, so playback is a plain read
        const double ratio = r->sampleRate / sampleRate;
        const int outLen = std::max (1, (int) std::floor (len / ratio));
        dry.setSize (2, outLen);
        for (int c = 0; c < 2; ++c)
        {
            juce::LagrangeInterpolator li;
            li.process (ratio, in.getReadPointer (c), dry.getWritePointer (c), outLen, len, 0);
        }
        rate = sampleRate; path = f.getFullPathName(); name = f.getFileNameWithoutExtension();
        return true;
    }
    bool hasSample() const { return dry.getNumSamples() > 0; }
    void clear() { dry.setSize (0, 0); path.clear(); name.clear(); publish (nullptr); }
    juce::String filePath() const { return path; }
    double loadedRate() const { return rate; }
    juce::String sampleName() const { return name; }

    // full offline render of the boost chain
    void render (DrumKind kind, const BoostParams& p)
    {
        if (! hasSample()) { publish (nullptr); return; }
        auto out = std::make_shared<DrumSample>();
        out->name = name;
        const float sr = (float) rate;
        // 1. pitch = resample (shorter / longer, like a sampler)
        const double ratio = std::exp2 (p.pitch / 12.0);
        const int len = std::max (16, (int) std::floor (dry.getNumSamples() / ratio));
        const int tail = p.room > 0.001f ? (int) (sr * 0.8f) : 0;
        auto& a = out->audio;
        a.setSize (2, len + tail);
        a.clear();
        for (int c = 0; c < 2; ++c)
        {
            juce::LagrangeInterpolator li;
            li.process (ratio, dry.getReadPointer (c), a.getWritePointer (c), len, dry.getNumSamples(), 0);
        }
        float* L = a.getWritePointer (0); float* R = a.getWritePointer (1);
        const int n = len + tail;
        // 2. punch: transient shaper (fast vs slow envelope)
        if (p.punch > 0.001f)
        {
            float fast = 0, slow = 0;
            const float fa = std::exp (-1.0f / (0.0008f * sr)), fr = std::exp (-1.0f / (0.012f * sr));
            const float sa = std::exp (-1.0f / (0.012f * sr)), srl = std::exp (-1.0f / (0.12f * sr));
            for (int i = 0; i < len; ++i)
            {
                const float x = std::max (std::abs (L[i]), std::abs (R[i]));
                fast = x + (x > fast ? fa : fr) * (fast - x);
                slow = x + (x > slow ? sa : srl) * (slow - x);
                const float g = 1.0f + std::clamp ((fast - slow) / (slow + 1.0e-4f), 0.0f, 3.0f) * p.punch * 1.4f;
                L[i] *= g; R[i] *= g;
            }
        }
        // 3. decay: shorter tail (exponential fade reaching -60 dB at decay x length)
        if (p.decay < 0.999f)
        {
            const int end = std::max (32, (int) (len * std::max (0.03f, p.decay)));
            const int hold = std::min (end / 4, (int) (0.01f * sr));
            for (int i = hold; i < len; ++i)
            {
                const float t = (float) (i - hold) / (float) std::max (1, end - hold);
                const float g = i >= end ? 0.0f : std::pow (0.001f, t);
                L[i] *= g; R[i] *= g;
            }
        }
        // 4. tone
        Shelf lowS[2], highS[2]; SvfCoef bell; SvfState bs[2], notch[2];
        if (kind == drum808)
        {
            for (auto& s : lowS) s.shelf (false, 70.0f, p.low * 9.0f, sr);              // SUB
            for (auto& s : highS) s.shelf (true, 1200.0f, p.high * 9.0f, sr);           // TONE: grit / presence
        }
        else if (kind == drumSnare)
        {
            bell.set (220.0f, 1.0f, sr);                                                 // BODY
            for (auto& s : highS) s.shelf (true, 5000.0f, p.high * 10.0f, sr);          // SNAP
        }
        else
        {
            for (auto& s : highS) s.shelf (true, 9000.0f, p.high * 10.0f, sr);          // AIR
            bell.set (4200.0f, 4.0f, sr);                                                // DE-RES notch
        }
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < 2; ++c)
            {
                float& x = c == 0 ? L[i] : R[i];
                if (kind == drum808) x = highS[c].tick (lowS[c].tick (x));
                else if (kind == drumSnare) { bs[c].tick (bell, x); x = highS[c].tick (x + bs[c].bp * p.low * 1.6f); }
                else { notch[c].tick (bell, x); x = highS[c].tick (x - notch[c].bp * p.deres * 0.8f); }
            }
        // 5. drive: 808 keeps the sub clean (saturates above 120 Hz, like 808 Killa), others full band
        if (p.drive > 0.001f)
        {
            const float k = 1.0f + p.drive * 9.0f;
            OnePole split[2]; for (auto& s : split) s.setHz (120.0f, sr);
            for (int i = 0; i < n; ++i)
                for (int c = 0; c < 2; ++c)
                {
                    float& x = c == 0 ? L[i] : R[i];
                    const float low = kind == drum808 ? split[c].lp (x) : 0.0f;
                    const float hi = x - low;
                    float y = saturate (p.sat, hi * k) / std::sqrt (k) * 1.3f;
                    x = low + hi + (y - hi) * std::min (1.0f, p.drive * 1.5f);
                }
        }
        // 6. room (snare / clap): short plate
        if (p.room > 0.001f)
        {
            FdnReverb verb; verb.prepare (sr);
            verb.process (L, R, n, 1, 0.25f, p.room * 0.6f, false);
        }
        // 7. width above 300 Hz (the low end stays mono)
        if (p.width > 0.001f)
        {
            DelayLine d; d.prepare ((int) (0.02f * sr) + 8);
            OnePole hp; hp.setHz (300.0f, sr);
            for (int i = 0; i < n; ++i)
            {
                const float m = 0.5f * (L[i] + R[i]);
                d.push (m - hp.lp (m));
                const float side = d.read (0.011f * sr) * p.width * 0.8f;
                L[i] += side; R[i] -= side;
            }
        }
        // 8. gain + clipper (+ never over -0.3 dBFS)
        const float g = juce::Decibels::decibelsToGain (p.gain), cd = juce::Decibels::decibelsToGain (p.clip);
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < 2; ++c)
            {
                float& x = c == 0 ? L[i] : R[i];
                x *= g;
                if (p.clip > 0.05f) { const float u = std::clamp (x * cd, -1.5f, 1.5f); x = (u - (4.0f / 27.0f) * u * u * u) * 0.966f; }
                x = std::clamp (x, -0.966f, 0.966f);
            }
        // trim silence at the end
        int last = n - 1;
        while (last > 64 && std::abs (L[last]) < 1.0e-4f && std::abs (R[last]) < 1.0e-4f) --last;
        a.setSize (2, last + 1, true);
        out->peaks = overview (a);
        out->peaksDry = overview (dry);
        if (kind == drum808) detectRoot (*out);
        publish (out);
    }

    // drag & drop: the boosted sample as a 24-bit WAV
    juce::File exportWav (const juce::String& suffix) const
    {
        auto s = current();
        if (s == nullptr) return {};
        auto dir = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("KEYS KILLA Drums");
        dir.createDirectory();
        auto f = dir.getChildFile (juce::File::createLegalFileName (s->name + " - KK " + suffix) + ".wav");
        f.deleteFile();
        juce::WavAudioFormat wav;
        if (auto os = std::unique_ptr<juce::OutputStream> (new juce::FileOutputStream (f)))
            if (auto w = std::unique_ptr<juce::AudioFormatWriter> (wav.createWriterFor (os.get(), rate, 2, 24, {}, 0)))
            {
                os.release();
                w->writeFromAudioSampleBuffer (s->audio, 0, s->audio.getNumSamples());
            }
        return f;
    }

    std::shared_ptr<const DrumSample> current() const { const juce::SpinLock::ScopedLockType l (lock); return sample; }

    // ---------- audio thread: play the boosted sample (pitched by the keys) ----------
    void noteOn (int note, float vel, int offset)
    {
        Voice* v = nullptr;
        for (auto& x : voices) if (! x.active) { v = &x; break; }
        if (v == nullptr) v = &voices[(size_t) (rr++ % voices.size())];
        *v = {};
        v->active = true; v->note = note; v->vel = 0.25f + 0.75f * vel; v->delay = offset;
        pendingRoot = true;
    }
    void noteOff (int note) { for (auto& v : voices) if (v.active && v.note == note) v.rel = true; }
    void allOff() { for (auto& v : voices) v.rel = true; }
    // root: the note that plays the sample unpitched (808: its detected note, others C5)
    void render (float* L, float* R, int n)
    {
        const juce::SpinLock::ScopedTryLockType tl (lock);
        if (! tl.isLocked() || sample == nullptr) return;
        const auto& s = *sample;
        const int total = s.audio.getNumSamples();
        const float* a = s.audio.getReadPointer (0); const float* b = s.audio.getReadPointer (1);
        const float relStep = 1.0f / (0.015f * (float) rate);
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
                float g = v.vel;
                if (v.rel) { v.relGain -= relStep; if (v.relGain <= 0) { v.active = false; break; } g *= v.relGain; }
                L[i] += (a[i0] + (a[i0 + 1] - a[i0]) * fr) * g;
                R[i] += (b[i0] + (b[i0 + 1] - b[i0]) * fr) * g;
                v.pos += inc;
            }
            playPos = (float) (v.pos / std::max (1, total));
        }
    }
    float playhead() const { for (auto& v : voices) if (v.active) return playPos.load(); return -1.0f; }

private:
    struct Voice { bool active = false, rel = false; int note = 60, delay = 0; double pos = 0; float vel = 1, relGain = 1; };
    struct Shelf
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        inline float tick (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
        void shelf (bool high, float f, float gainDb, float srate)
        {
            const float A = std::pow (10.0f, gainDb / 40.0f), w = twoPi * std::min (f, srate * 0.45f) / srate, cs = std::cos (w), sn = std::sin (w);
            const float al = sn / 2.0f * std::sqrt (2.0f), sq = 2.0f * std::sqrt (A) * al;
            float B0, B1, B2, A0, A1, A2;
            if (! high) { B0 = A * ((A + 1) - (A - 1) * cs + sq); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - sq);
                          A0 = (A + 1) + (A - 1) * cs + sq; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - sq; }
            else        { B0 = A * ((A + 1) + (A - 1) * cs + sq); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - sq);
                          A0 = (A + 1) - (A - 1) * cs + sq; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - sq; }
            b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
        }
    };
    static float saturate (int mode, float x)
    {
        if (mode == 0) return 2.0f / pi * std::atan (x);                       // tape
        if (mode == 1) return x >= 0.0f ? x / (1.0f + x) : std::tanh (x);       // tube (asymmetric)
        const float t = 1.0f, period = 4.0f * t;                               // foldback
        if (std::abs (x) <= t) return x;
        float y = std::fmod (x + t, period); if (y < 0) y += period;
        return y < 2.0f * t ? y - t : 3.0f * t - y;
    }
    static std::vector<float> overview (const juce::AudioBuffer<float>& a)
    {
        const int cols = 480, len = a.getNumSamples();
        std::vector<float> pk ((size_t) cols, 0.0f);
        if (len <= 0) return pk;
        for (int c = 0; c < cols; ++c)
        {
            const int s0 = (int) ((juce::int64) len * c / cols), s1 = std::max (s0 + 1, (int) ((juce::int64) len * (c + 1) / cols));
            pk[(size_t) c] = std::max (a.getMagnitude (0, s0, s1 - s0), a.getMagnitude (1, s0, s1 - s0));
        }
        return pk;
    }
    // 808 root: autocorrelation of the sustained part, 25 - 160 Hz
    void detectRoot (DrumSample& s) const
    {
        const int len = s.audio.getNumSamples();
        const float sr = (float) rate;
        const int start = std::min (len - 1, (int) (0.06f * sr)), win = std::min (len - start, (int) (0.25f * sr));
        if (win < (int) (sr / 25.0f) * 2) return;
        const float* x = s.audio.getReadPointer (0) + start;
        const int minLag = (int) (sr / 160.0f), maxLag = (int) (sr / 25.0f);
        double best = 0; int bestLag = 0;
        double e0 = 0; for (int i = 0; i < win; ++i) e0 += x[i] * x[i];
        if (e0 < 1.0e-6) return;
        for (int lag = minLag; lag <= maxLag && lag < win / 2; ++lag)
        {
            double acc = 0;
            for (int i = 0; i + lag < win; ++i) acc += x[i] * x[i + lag];
            acc /= (win - lag);
            if (acc > best) { best = acc; bestLag = lag; }
        }
        if (bestLag <= 0 || best < 0.25 * e0 / win) return;
        s.rootHz = sr / (float) bestLag;
        s.rootNote = (int) std::lround (69.0 + 12.0 * std::log2 (s.rootHz / 440.0));
    }
    void publish (std::shared_ptr<DrumSample> s)
    {
        std::shared_ptr<const DrumSample> old;
        { const juce::SpinLock::ScopedLockType l (lock); old = std::move (sample); sample = std::move (s); }
    }

    juce::AudioBuffer<float> dry;
    double rate = 44100;
    juce::String path, name;
    mutable juce::SpinLock lock;
    std::shared_ptr<const DrumSample> sample;
    std::array<Voice, 6> voices;
    unsigned rr = 0;
    bool pendingRoot = false;
    std::atomic<float> playPos { -1.0f };
};
} // namespace kk
