#pragma once
#include <juce_dsp/juce_dsp.h>
#include "DspUtil.h"

namespace kk
{
// Band-limited wavetables generated in code: 8 frames x 11 mip levels (1024 .. 1 harmonics).
class WavetableBank
{
public:
    static constexpr int size = 2048, frames = 8, levels = 11;

    static const WavetableBank& get() { static const WavetableBank b; return b; }

    // phase 0..1, position 0..1, inc = freq/sr
    inline float read (float phase, float pos, float inc) const
    {
        const float maxH = 0.45f / std::max (inc, 1.0e-6f);
        int lvl = 0;
        while (lvl < levels - 1 && (float) (1024 >> lvl) > maxH) ++lvl;
        const float fp = std::clamp (pos, 0.0f, 1.0f) * (float) (frames - 1);
        const int f0 = std::min ((int) fp, frames - 2);
        const float ff = fp - (float) f0;
        const float x = phase * (float) size;
        const int i0 = (int) x & (size - 1), i1 = (i0 + 1) & (size - 1);
        const float fr = x - std::floor (x);
        const float* a = tables[(size_t) (f0 * levels + lvl)].data();
        const float* b = tables[(size_t) ((f0 + 1) * levels + lvl)].data();
        const float va = a[i0] + (a[i1] - a[i0]) * fr;
        const float vb = b[i0] + (b[i1] - b[i0]) * fr;
        return va + (vb - va) * ff;
    }

    static const char* frameName (int i)
    {
        static const char* n[] { "Sine", "Triangle", "Saw", "Square", "Pulse", "Glass", "Digital", "Harsh" };
        return n[std::clamp (i, 0, frames - 1)];
    }

private:
    WavetableBank()
    {
        tables.resize ((size_t) (frames * levels));
        juce::dsp::FFT fft (11);   // 2048
        std::vector<float> buf ((size_t) size * 2);
        for (int f = 0; f < frames; ++f)
        {
            float frameScale = 1.0f;   // one scale per frame so all mip levels have equal loudness
            for (int l = 0; l < levels; ++l)
            {
                const int maxH = 1024 >> l;
                std::fill (buf.begin(), buf.end(), 0.0f);
                // complex spectrum in interleaved re/im (JUCE real-only inverse layout)
                for (int n = 1; n <= std::min (maxH, size / 2 - 1); ++n)
                {
                    float amp = 0, ph = 0;
                    const float fn = (float) n;
                    switch (f)
                    {
                        case 0: amp = n == 1 ? 1.0f : 0.0f; break;
                        case 1: amp = (n % 2) ? 1.0f / (fn * fn) * ((n / 2) % 2 ? -1.0f : 1.0f) : 0.0f; break;
                        case 2: amp = 1.0f / fn; break;
                        case 3: amp = (n % 2) ? 1.0f / fn : 0.0f; break;
                        case 4: amp = std::sin (pi * fn * 0.25f) / fn; break;
                        case 5: amp = 0.7f * std::exp (-0.5f * std::pow ((fn - 7.0f) / 2.5f, 2.0f)) + 0.6f / std::pow (fn, 1.5f); break;
                        case 6: amp = (n == 1 || n == 3 || n == 7 || n == 11 || n == 13 || n == 19 || n == 23) ? 1.0f / std::sqrt (fn) : 0.0f; break;
                        default: amp = 1.0f / std::sqrt (fn); ph = (float) (hash32 ((uint32_t) n * 7919u) % 1000) / 1000.0f * twoPi; break;
                    }
                    // sin(n x + ph): X[n] = -j/2 * amp * e^{j ph} * size
                    const float re = 0.5f * amp * std::sin (ph) * (float) size;
                    const float im = -0.5f * amp * std::cos (ph) * (float) size;
                    buf[(size_t) (2 * n)] = re;      buf[(size_t) (2 * n + 1)] = im;
                    buf[(size_t) (2 * (size - n))] = re; buf[(size_t) (2 * (size - n) + 1)] = -im;
                }
                fft.performRealOnlyInverseTransform (buf.data());
                auto& t = tables[(size_t) (f * levels + l)];
                t.assign (buf.begin(), buf.begin() + size);
                if (l == 0)
                {
                    float peak = 1.0e-9f;
                    for (float v : t) peak = std::max (peak, std::abs (v));
                    frameScale = 1.0f / peak;
                }
                for (float& v : t) v *= frameScale;
            }
        }
    }

    std::vector<std::vector<float>> tables;
};
} // namespace kk
