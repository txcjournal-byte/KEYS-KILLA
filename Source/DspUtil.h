#pragma once
#include <cmath>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace kk
{
constexpr float twoPi = 6.283185307179586f;
constexpr float pi    = 3.141592653589793f;

inline float semisToHz (float midi) { return 440.0f * std::exp2 ((midi - 69.0f) / 12.0f); }
inline float clamp01 (float x) { return std::clamp (x, 0.0f, 1.0f); }

// Deterministic, allocation-free RNG (xorshift32)
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    void seed (uint32_t v) { s = v ? v : 0x9E3779B9u; for (int i = 0; i < 4; ++i) next(); }
    uint32_t next() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
    float uni()     { return (float) (next() >> 8) * (1.0f / 16777216.0f); }   // [0,1)
    float bi()      { return uni() * 2.0f - 1.0f; }                             // [-1,1)
};

inline uint32_t hash32 (uint32_t x)
{
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}

inline float polyBlep (float t, float dt)
{
    if (t < dt)        { t /= dt; return t + t - t * t - 1.0f; }
    if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
    return 0.0f;
}

// Cytomic TPT state-variable filter
struct SvfCoef
{
    float a1 = 0, a2 = 0, a3 = 0, k = 1;
    void set (float fc, float resK, float sr)
    {
        fc = std::clamp (fc, 10.0f, sr * 0.45f);
        const float g = std::tan (pi * fc / sr);
        k  = resK;
        a1 = 1.0f / (1.0f + g * (g + k));
        a2 = g * a1;
        a3 = g * a2;
    }
};

struct SvfState
{
    float ic1 = 0, ic2 = 0, lp = 0, bp = 0;
    inline void tick (const SvfCoef& c, float v0)
    {
        const float v3 = v0 - ic2;
        const float v1 = c.a1 * ic1 + c.a2 * v3;
        const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
        ic1 = 2.0f * v1 - ic1;
        ic2 = 2.0f * v2 - ic2;
        lp = v2; bp = v1;
    }
    void reset() { ic1 = ic2 = lp = bp = 0; }
};

struct OnePole
{
    float z = 0, a = 0;
    void setHz (float hz, float sr) { a = std::exp (-twoPi * hz / sr); }
    inline float lp (float x) { z = x + a * (z - x); return z; }
    void reset() { z = 0; }
};

struct DcBlock
{
    float x1 = 0, y1 = 0;
    inline float tick (float x) { float y = x - x1 + 0.995f * y1; x1 = x; y1 = y; return y; }
};

struct DelayLine
{
    std::vector<float> buf;
    int mask = 0, w = 0;
    void prepare (int minSize)
    {
        int n = 1; while (n < minSize) n <<= 1;
        buf.assign ((size_t) n, 0.0f); mask = n - 1; w = 0;
    }
    void clear() { std::fill (buf.begin(), buf.end(), 0.0f); }
    inline void push (float x) { buf[(size_t) w] = x; w = (w + 1) & mask; }
    // delay in samples (>= 1), linear interpolation
    inline float read (float d) const
    {
        d = std::clamp (d, 1.0f, (float) mask - 2.0f);
        const float rp = (float) w - d;
        int i0 = (int) std::floor (rp);
        const float fr = rp - (float) i0;
        const float a = buf[(size_t) (i0 & mask)], b = buf[(size_t) ((i0 + 1) & mask)];
        return a + (b - a) * fr;
    }
};

struct Adsr
{
    enum Stage { Idle, Attack, Decay, Sustain, Release } st = Idle;
    float v = 0, aInc = 0.01f, dCoef = 0.999f, rCoef = 0.999f, sus = 1;
    void set (float sr, float a, float d, float s, float r)
    {
        aInc  = 1.0f / std::max (1.0f, a * sr);
        dCoef = std::exp (-4.6f / std::max (1.0f, d * sr));
        rCoef = std::exp (-4.6f / std::max (1.0f, r * sr));
        sus   = s;
    }
    void on()  { st = Attack; }
    void off() { if (st != Idle) st = Release; }
    void kill() { st = Idle; v = 0; }
    inline float next()
    {
        switch (st)
        {
            case Attack:  v += aInc; if (v >= 1.0f) { v = 1.0f; st = Decay; } break;
            case Decay:   v = sus + (v - sus) * dCoef;
                          if (sus < 1.0e-4f && v < 1.0e-5f) { v = 0; st = Idle; }
                          else if (std::abs (v - sus) < 1.0e-4f) st = Sustain;
                          break;
            case Sustain: v = sus; break;
            case Release: v *= rCoef; if (v < 1.0e-5f) { v = 0; st = Idle; } break;
            case Idle:    break;
        }
        return v;
    }
};
} // namespace kk
