#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <juce_audio_basics/juce_audio_basics.h>

// Small DSP building blocks shared by the CHROME VAULT physical models (Source/vault/models/*).
// Everything here is allocation-free after construction / resize(), deterministic and float-only on the audio path.
namespace vault::models::dsp
{
constexpr float pi = 3.14159265358979f;
constexpr float twoPi = 6.28318530717959f;

inline float dbToGain (float db) { return std::pow (10.0f, db * 0.05f); }
inline float centsToRatio (float c) { return std::exp2 (c / 1200.0f); }
inline float clampf (float x, float lo, float hi) { return x < lo ? lo : (x > hi ? hi : x); }
// per-sample multiplier that makes an exponential fall 60 dB in t60 seconds
inline float t60ToMul (float t60, double sr) { return (float) std::pow (0.001, 1.0 / (std::max (1.0e-4f, t60) * sr)); }

// xorshift32 noise in -1..1, seeded per note (deterministic renders)
struct Rng
{
    uint32_t s = 0x9e3779b9u;
    void seed (uint32_t v) { s = v * 2654435761u + 0x6d2b79f5u; if (s == 0) s = 1; }
    float next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (int32_t) s * 4.6566129e-10f; }
};

// one-pole low-pass y += (1 - a)(x - y)
struct OnePoleLP
{
    float a = 0.0f, z = 0.0f;
    void setHz (float hz, double sr) { a = (float) std::exp (-2.0 * 3.141592653589793 * std::max (1.0f, hz) / sr); }
    float tick (float x) { z = x + a * (z - x); return z; }
    void reset() { z = 0.0f; }
};

// DC blocker / gentle high-pass (one zero at DC, one pole)
struct DcBlock
{
    float r = 0.995f, x1 = 0.0f, y1 = 0.0f;
    void setHz (float hz, double sr) { r = (float) std::exp (-2.0 * 3.141592653589793 * hz / sr); }
    float tick (float x) { const float y = x - x1 + r * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0.0f; }
};

// RBJ biquad, transposed direct form II
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    float tick (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void reset() { z1 = z2 = 0.0f; }
    void norm (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0); a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
    static double w (double hz, double sr) { return 2.0 * 3.141592653589793 * std::min (hz, sr * 0.49) / sr; }
    void lowpass (double hz, double q, double sr)
    {
        const double w0 = w (hz, sr), c = std::cos (w0), al = std::sin (w0) / (2 * q);
        norm ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highpass (double hz, double q, double sr)
    {
        const double w0 = w (hz, sr), c = std::cos (w0), al = std::sin (w0) / (2 * q);
        norm ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void bandpass (double hz, double q, double sr)   // 0 dB peak
    {
        const double w0 = w (hz, sr), c = std::cos (w0), al = std::sin (w0) / (2 * q);
        norm (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    void peak (double hz, double q, double db, double sr)
    {
        const double A = std::pow (10.0, db / 40.0), w0 = w (hz, sr), c = std::cos (w0), al = std::sin (w0) / (2 * q);
        norm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void lowShelf (double hz, double db, double sr)
    {
        const double A = std::pow (10.0, db / 40.0), w0 = w (hz, sr), c = std::cos (w0), s = std::sin (w0);
        const double al = s / 2 * std::sqrt (2.0), sq = 2 * std::sqrt (A) * al;
        norm (A * ((A + 1) - (A - 1) * c + sq), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - sq),
              (A + 1) + (A - 1) * c + sq, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - sq);
    }
    void highShelf (double hz, double db, double sr)
    {
        const double A = std::pow (10.0, db / 40.0), w0 = w (hz, sr), c = std::cos (w0), s = std::sin (w0);
        const double al = s / 2 * std::sqrt (2.0), sq = 2 * std::sqrt (A) * al;
        norm (A * ((A + 1) + (A - 1) * c + sq), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - sq),
              (A + 1) - (A - 1) * c + sq, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - sq);
    }
    void flat() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
};

// two-pole resonator with unity gain at DC (Klatt cascade formant): y = b0 x + c1 y1 + c2 y2
struct Formant
{
    float b0 = 1, c1 = 0, c2 = 0, y1 = 0, y2 = 0;
    void set (float hz, float bw, double sr)
    {
        const double r = std::exp (-3.141592653589793 * bw / sr);
        const double cc1 = 2.0 * r * std::cos (2.0 * 3.141592653589793 * hz / sr), cc2 = -r * r;
        c1 = (float) cc1; c2 = (float) cc2; b0 = (float) (1.0 - cc1 - cc2);
    }
    float tick (float x) { const float y = b0 * x + c1 * y1 + c2 * y2; y2 = y1; y1 = y; return y; }
    void reset() { y1 = y2 = 0.0f; }
};

// power-of-two circular delay line with 3rd-order Lagrange fractional read (delay >= 2 samples)
struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, w = 0;
    void resize (int minLen)
    {
        int n = 64; while (n < minLen + 8) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); w = 0; }
    void write (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    // value written `d` samples ago (d >= 1 -> the most recent write is d = 1)
    float tap (int d) const { return buf[(size_t) ((w - d) & mask)]; }
    // fractional delay d (>= 1): linear interpolation (cheap taps, e.g. pickup positions)
    float readLinear (float d) const
    {
        const int di = (int) d;
        const float f = d - (float) di;
        const float a = tap (di), b = tap (di + 1);
        return a + f * (b - a);
    }
    // fractional delay d (>= 2): Lagrange-3 over the 4 taps around it
    float read (float d) const
    {
        const int di = (int) d;
        const float f = d - (float) di + 1.0f;                 // 1 <= f < 2: optimal centre of the 4-tap Lagrange
        const float x0 = tap (di - 1), x1 = tap (di), x2 = tap (di + 1), x3 = tap (di + 2);
        const float fm1 = f - 1.0f, fm2 = f - 2.0f, fm3 = f - 3.0f;
        return -x0 * fm1 * fm2 * fm3 * (1.0f / 6.0f) + x1 * f * fm2 * fm3 * 0.5f - x2 * f * fm1 * fm3 * 0.5f + x3 * f * fm1 * fm2 * (1.0f / 6.0f);
    }
};

// first-order allpass (c + z^-1) / (1 + c z^-1)
struct Allpass1
{
    float c = 0.0f, x1 = 0.0f, y1 = 0.0f;
    float tick (float x) { const float y = c * x + x1 - c * y1; x1 = x; y1 = y; return y; }
    void reset() { x1 = y1 = 0.0f; }
    // phase delay in samples at w (rad/sample)
    static double phaseDelay (double c, double w) { return (w - 2.0 * std::atan2 (c * std::sin (w), 1.0 + c * std::cos (w))) / w; }
};

// soft clipper x / sqrt(1 + x^2) with first-order anti-derivative anti-aliasing (F = sqrt(1 + x^2))
struct SoftClipAA
{
    float x1 = 0.0f, F1 = 1.0f;
    float tick (float x)
    {
        const float F = std::sqrt (1.0f + x * x), d = x - x1;
        float y;
        if (std::abs (d) > 1.0e-4f) y = (F - F1) / d;
        else { const float m = 0.5f * (x + x1); y = m / std::sqrt (1.0f + m * m); }
        x1 = x; F1 = F;
        return y;
    }
    void reset() { x1 = 0.0f; F1 = 1.0f; }
};

// linear ramp for parameters updated once per block
struct Ramp
{
    float v = 0.0f, step = 0.0f; int left = 0;
    void set (float target, int n) { if (n <= 0) { v = target; left = 0; step = 0; } else { step = (target - v) / (float) n; left = n; } }
    void jump (float target) { v = target; left = 0; step = 0; }
    float next() { if (left > 0) { v += step; --left; } return v; }
};

// envelope follower for "is the voice silent yet" decisions (block peak with a slow fall)
struct Silence
{
    float peak = 0.0f;
    void block (const float* x, int n) { float m = 0; for (int i = 0; i < n; ++i) m = std::max (m, std::abs (x[i])); peak = std::max (m, peak * 0.5f); }
};
} // namespace vault::models::dsp
