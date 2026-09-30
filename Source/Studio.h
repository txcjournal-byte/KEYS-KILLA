#pragma once
#include <array>
#include <cmath>
#include "DspUtil.h"

// Studio-grade reverb and MASTER stage (v0.13).
namespace kk
{
// 8-line feedback delay network: input diffusion, pre-delay, modulated lines, per-line damping,
// Hadamard mixing, RT60-accurate decay, low-cut / high-cut on the wet signal (clean low end).
class FdnReverb
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        for (auto& l : lines) l.prepare ((int) (0.35f * sr) + 64);
        for (auto& d : diff) d.prepare ((int) (0.03f * sr) + 8);
        pre.prepare ((int) (0.2f * sr) + 8);
        wetHp[0].setHz (160.0f, sr); wetHp[1].setHz (160.0f, sr);
        reset();
    }
    void reset()
    {
        for (auto& l : lines) l.clear();
        for (auto& d : diff) d.clear();
        pre.clear();
        damp.fill (0.0f); hcut.fill (0.0f); wetHp[0].reset(); wetHp[1].reset();
        phase = 0;
    }
    // type: 0 hall, 1 plate, 2 cloud. size 0..1, mix 0..1
    void process (float* L, float* R, int n, int type, float size, float mix, bool freeze)
    {
        if (mix <= 0.0005f && ! freeze && quiet) return;
        static constexpr float baseMs[N] { 37.1f, 43.7f, 51.3f, 59.9f, 67.3f, 73.1f, 83.9f, 97.1f };
        static constexpr float diffMs[4] { 4.7f, 7.3f, 11.1f, 15.7f };
        const float scale = type == 1 ? 0.55f + size * 0.6f : type == 2 ? 1.4f + size * 1.2f : 0.8f + size * 1.1f;
        const float rt60 = type == 1 ? 0.7f + size * 3.0f : type == 2 ? 5.0f + size * 15.0f : 1.0f + size * 5.0f;
        const float dampHz = type == 1 ? 9000.0f : type == 2 ? 3200.0f : 5200.0f;
        const float modDepth = (type == 2 ? 0.012f : 0.004f) * sr * 0.05f;
        const float preSamples = (type == 1 ? 0.004f : 0.012f + size * 0.025f) * sr;
        const float dampA = std::exp (-twoPi * std::min (dampHz, sr * 0.4f) / sr), hcutA = std::exp (-twoPi * std::min (11000.0f, sr * 0.4f) / sr);
        std::array<float, N> len {}, gain {};
        for (int i = 0; i < N; ++i)
        {
            len[(size_t) i] = baseMs[i] * 0.001f * sr * scale;
            gain[(size_t) i] = freeze ? 1.0f : std::pow (10.0f, -3.0f * len[(size_t) i] / (rt60 * sr));
        }
        const float wet = freeze ? std::max (mix, 0.5f) : mix;
        const float dryGain = 1.0f - wet * 0.35f, wetGain = wet * (type == 2 ? 0.9f : 0.75f);
        const float modInc = twoPi * 0.37f / sr;
        float energy = 0;
        for (int s = 0; s < n; ++s)
        {
            float in = freeze ? 0.0f : (L[s] + R[s]) * 0.5f;
            pre.push (in);
            in = pre.read (preSamples);
            for (int d = 0; d < 4; ++d)   // Schroeder all-pass diffusion
            {
                const float dl = diff[(size_t) d].read (diffMs[d] * 0.001f * sr);
                const float v = in + 0.62f * dl;
                diff[(size_t) d].push (v);
                in = dl - 0.62f * v;
            }
            phase += modInc; if (phase > twoPi) phase -= twoPi;
            std::array<float, N> y {};
            for (int i = 0; i < N; ++i)
            {
                const float m = modDepth * std::sin (phase + (float) i * 0.785f);
                float v = lines[(size_t) i].read (len[(size_t) i] + m);
                damp[(size_t) i] = v + dampA * (damp[(size_t) i] - v);   // high frequencies decay faster
                y[(size_t) i] = damp[(size_t) i];
            }
            hadamard (y);
            for (int i = 0; i < N; ++i)
                lines[(size_t) i].push (y[(size_t) i] * gain[(size_t) i] + in * ((i & 1) ? -0.35f : 0.35f));
            float wl = (y[0] - y[2] + y[4] - y[6]) * 0.5f, wr = (y[1] - y[3] + y[5] - y[7]) * 0.5f;
            hcut[0] = wl + hcutA * (hcut[0] - wl); hcut[1] = wr + hcutA * (hcut[1] - wr);
            wl = hcut[0] - wetHp[0].lp (hcut[0]); wr = hcut[1] - wetHp[1].lp (hcut[1]);   // low cut keeps the low end dry
            energy += wl * wl + wr * wr;
            L[s] = L[s] * dryGain + wl * wetGain;
            R[s] = R[s] * dryGain + wr * wetGain;
        }
        quiet = energy < 1.0e-9f;
    }
private:
    static constexpr int N = 8;
    static void hadamard (std::array<float, N>& x)
    {
        for (int h = 1; h < N; h <<= 1)
            for (int i = 0; i < N; i += h * 2)
                for (int j = i; j < i + h; ++j) { const float a = x[(size_t) j], b = x[(size_t) (j + h)]; x[(size_t) j] = a + b; x[(size_t) (j + h)] = a - b; }
        for (auto& v : x) v *= 0.35355339f;   // 1 / sqrt (8)
    }
    float sr = 44100, phase = 0;
    bool quiet = true;
    std::array<DelayLine, N> lines;
    std::array<DelayLine, 4> diff;
    DelayLine pre;
    std::array<float, N> damp {};
    std::array<float, 2> hcut {};
    OnePole wetHp[2];
};

// MASTER: glue compression, gentle saturation, air, mono lows and a smooth peak limiter -
// presets come out full and finished, like commercial libraries. amount 0 = bypass, 1 = full.
class MasterStage
{
public:
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        atk = 1.0f - std::exp (-1.0f / (0.012f * sr));
        rel = 1.0f - std::exp (-1.0f / (0.16f * sr));
        limRel = 1.0f - std::exp (-1.0f / (0.09f * sr));
        for (auto& a : air) a.shelf (true, std::min (10000.0f, sr * 0.4f), 1.8f, sr);   // stays below Nyquist at low rates
        for (auto& b : body) b.shelf (false, 110.0f, 1.2f, sr);
        for (auto& s : sideLp) s.setHz (140.0f, sr);
        widHp.setHz (200.0f, sr);
        wid.prepare ((int) (0.03f * sr) + 8);
        reset();
    }
    void reset()
    {
        env = 0; limGain = 1; gr = 0;
        for (auto& a : air) a.z1 = a.z2 = 0;
        for (auto& b : body) b.z1 = b.z2 = 0;
        for (auto& s : sideLp) s.reset();
        widHp.reset(); wid.clear();
    }
    // returns the output peak before limiting (for the overload light)
    // width 0..1 (the preset's stereo width): mono-safe widening from a decorrelated copy above 250 Hz
    float process (float* L, float* R, int n, float amount, float width = 0.5f)
    {
        float peak = 0;
        const float a = std::clamp (amount, 0.0f, 1.0f);
        const float thresh = 0.18f, ratio = 1.0f + 2.2f * a;                 // about 2-4 dB of glue on typical material
        const float makeup = std::pow (10.0f, (2.5f * a) / 20.0f);
        const float satMix = 0.35f * a;
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            if (a > 0.001f)
            {
                // mono lows: side signal below 140 Hz removed
                float s = (l - r) * 0.5f; const float m = (l + r) * 0.5f;
                s -= sideLp[0].lp (s) * a;
                // stereo: a 9 ms decorrelated copy of the mid (above 250 Hz) added as side - cancels in mono
                wid.push (m - widHp.lp (m));
                if (width >= 0.0f) s += wid.read (0.009f * sr) * 0.95f * a * std::clamp (0.45f + width * 0.9f, 0.0f, 1.0f);   // width < 0: bass, keep it centred
                l = m + s; r = m - s;
                // glue compressor (RMS-ish detector, soft knee)
                const float lev = std::sqrt (0.5f * (l * l + r * r));
                env += (lev - env) * (lev > env ? atk : rel);
                float g = 1.0f;
                if (env > thresh) g = std::pow (env / thresh, 1.0f / ratio - 1.0f);
                gr = g;
                l *= g * makeup; r *= g * makeup;
                // tone: a little weight and air
                const float lt = body[0].tick (air[0].tick (l)), rt = body[1].tick (air[1].tick (r));
                l += (lt - l) * a; r += (rt - r) * a;
                // warm saturation (even-ish harmonics), blended
                l += (std::tanh (l * 1.3f) / 1.3f * 1.05f - l) * satMix;
                r += (std::tanh (r * 1.3f) / 1.3f * 1.05f - r) * satMix;
            }
            peak = std::max (peak, std::max (std::abs (l), std::abs (r)));
            // smooth peak limiter, ceiling -0.8 dBFS
            const float p = std::max (std::abs (l), std::abs (r));
            const float want = p > ceiling ? ceiling / p : 1.0f;
            limGain = want < limGain ? want : limGain + (1.0f - limGain) * limRel;
            l *= limGain; r *= limGain;
            L[i] = std::clamp (l, -0.999f, 0.999f); R[i] = std::clamp (r, -0.999f, 0.999f);
        }
        return peak;
    }
    float gainReduction() const { return gr; }
private:
    struct Shelf
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
        inline float tick (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
        void shelf (bool high, float f, float gainDb, float srate)
        {
            const float A = std::pow (10.0f, gainDb / 40.0f), w = twoPi * f / srate, cs = std::cos (w), sn = std::sin (w);
            const float al = sn / 2.0f * std::sqrt (2.0f), sq = 2.0f * std::sqrt (A) * al;
            float B0, B1, B2, A0, A1, A2;
            if (! high) { B0 = A * ((A + 1) - (A - 1) * cs + sq); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - sq);
                          A0 = (A + 1) + (A - 1) * cs + sq; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - sq; }
            else        { B0 = A * ((A + 1) + (A - 1) * cs + sq); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - sq);
                          A0 = (A + 1) - (A - 1) * cs + sq; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - sq; }
            b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
        }
    };
    float sr = 44100, atk = 0.01f, rel = 0.001f, limRel = 0.001f, env = 0, limGain = 1, gr = 1;
    static constexpr float ceiling = 0.912f;   // -0.8 dBFS
    Shelf air[2], body[2];
    OnePole sideLp[2], widHp;
    DelayLine wid;
};
} // namespace kk
