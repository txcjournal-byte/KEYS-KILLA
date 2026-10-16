#pragma once
// CHROME VAULT pitch measurement, shared by the pitch test (tests/VaultPitchTests.cpp, VaultTests -pitch) and the library
// tuning pass (tools/vault/Retune.h, VaultLibBuild -retune): one definition of "the pitch of a note".
//   * the window: from 20 ms after the sound reaches a third of its peak level, as long as it stays within 20 dB of the
//     peak, 93 .. 370 ms (plucks: the note itself, not the decayed tail)
//   * YIN (FFT difference function, frames of 2048, median of the periodic frames)
//   * a log-frequency magnitude spectrum (5-cent grid, 35 Hz .. 9 kHz, square-root amplitude, peaks only): two notes are
//     compared by cross-correlating their spectra - the shift that lines up every partial (bells, noisy and layered sounds)
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <cmath>
#include <vector>

namespace vaultpitch
{
constexpr int maxWinLen = 32768;
// callers pass fLow = lowCutRatio x the fundamental of the lowest note sounding (the spectrum below holds no partial of
// it); the fundamental partial is looked for at fLow / lowCutRatio
constexpr double lowCutRatio = 0.45;
constexpr int fftOrder = 15, fftSize = 1 << fftOrder;
constexpr double gridLoHz = 35.0, gridHiHz = 9000.0, gridStepCents = 5.0;
inline int gridSize() { return (int) (1200.0 * std::log2 (gridHiHz / gridLoHz) / gridStepCents) + 1; }

// YIN over frames of 2048 samples (hop 1024) with the difference function from an FFT autocorrelation
struct Yin
{
    double sr = 44100.0;
    static constexpr int w = 2048, order = 13, n = 1 << order;   // w + tauMax <= n / 2 is not needed: linear correlation fits in n
    juce::dsp::FFT fft { order };
    std::vector<float> a = std::vector<float> ((size_t) n * 2), b = std::vector<float> ((size_t) n * 2);

    // returns the median f0 of the periodic frames (0 if fewer than 40 % are periodic), `share` = periodic share
    std::vector<float> track;   // the last run: f0 per frame (hop w / 2), 0 = not periodic

    float run (const float* x, int len, double fmin, double fmax, float& share, float& spreadCents)
    {
        track.clear();
        const int tauMax = std::min (w - 2, (int) (sr / fmin)), tauMin = std::max (2, (int) (sr / fmax));
        std::vector<float> f0s;
        int frames = 0;
        std::vector<double> d ((size_t) tauMax + 2), cm ((size_t) tauMax + 2);
        for (int s0 = 0; s0 + w + tauMax <= len; s0 += w / 2)
        {
            ++frames;
            track.push_back (0.0f);
            // r(tau) = sum_{j<w} x[s0+j] x[s0+j+tau]
            std::fill (a.begin(), a.end(), 0.0f); std::fill (b.begin(), b.end(), 0.0f);
            for (int j = 0; j < w + tauMax; ++j) a[(size_t) j] = x[s0 + j];
            for (int j = 0; j < w; ++j) b[(size_t) j] = x[s0 + j];
            fft.performRealOnlyForwardTransform (a.data());
            fft.performRealOnlyForwardTransform (b.data());
            for (int k = 0; k <= n / 2; ++k)
            {
                const float ar = a[(size_t) 2 * k], ai = a[(size_t) 2 * k + 1], br = b[(size_t) 2 * k], bi = b[(size_t) 2 * k + 1];
                a[(size_t) 2 * k] = ar * br + ai * bi;          // A * conj (B)
                a[(size_t) 2 * k + 1] = ai * br - ar * bi;
            }
            fft.performRealOnlyInverseTransform (a.data());
            // energies: e0 = sum_{j<w} x[s0+j]^2, et(tau) = sum_{j<w} x[s0+j+tau]^2
            double e0 = 0;
            for (int j = 0; j < w; ++j) e0 += (double) x[s0 + j] * x[s0 + j];
            if (e0 < 1.0e-9) continue;
            double et = e0;
            double run = 0;
            for (int tau = 1; tau <= tauMax + 1; ++tau)
            {
                et += (double) x[s0 + w - 1 + tau] * x[s0 + w - 1 + tau] - (double) x[s0 + tau - 1] * x[s0 + tau - 1];
                d[(size_t) tau] = std::max (0.0, e0 + et - 2.0 * a[(size_t) tau]);
                run += d[(size_t) tau];
                cm[(size_t) tau] = run > 0 ? d[(size_t) tau] * tau / run : 1.0;
            }
            int best = -1;
            for (int tau = tauMin; tau <= tauMax; ++tau)
                if (cm[(size_t) tau] < 0.15) { while (tau < tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau; best = tau; break; }
            if (best <= 1 || best >= tauMax) continue;
            const double p = cm[(size_t) best - 1], q = cm[(size_t) best], r = cm[(size_t) best + 1], den = p - 2 * q + r;
            const double t = std::abs (den) > 1e-12 ? best + 0.5 * (p - r) / den : best;
            f0s.push_back ((float) (sr / t));
            track.back() = f0s.back();
        }
        share = frames > 0 ? (float) f0s.size() / (float) frames : 0.0f;
        spreadCents = 9999.0f;
        if (f0s.empty() || share < 0.4f) return 0.0f;
        std::sort (f0s.begin(), f0s.end());
        const float med = f0s[f0s.size() / 2];
        const float q1 = f0s[f0s.size() / 4], q3 = f0s[(f0s.size() * 3) / 4];
        spreadCents = 1200.0f * std::log2 (q3 / std::max (1.0e-3f, q1));
        return med;
    }
};

// one sound measured: level, YIN fundamental, log-frequency spectrum (see the top of this file)
struct Analysis
{
    float levelDb = -200.0f;
    bool silent = true;
    float f0 = 0.0f;           // YIN, Hz (0 = none)
    float periodicity = 0.0f;  // share of periodic frames
    float peakHz = 0.0f;       // the strongest spectral peak above 30 Hz (parabolic interpolation of its log magnitude)
    float f1Hz = 0.0f;         // the fundamental partial: the strongest peak within +-150 cents of the expected fundamental,
                               // at most 30 dB under the strongest peak (0 = none): tuned bars / tines whose upper modes
                               // are inharmonic and differ from bar to bar
    std::vector<float> track;  // YIN f0 per frame (2048 samples, hop 1024, from `from`), 0 = not periodic
    bool confident = false;    // periodic in >= 60 % of the frames, the frames agree within 25 cents
    int from = 0, len = 0;     // the analysed window
    std::vector<float> logSpec;
};

struct Analyser
{
    juce::dsp::FFT fft { fftOrder };
    std::vector<float> buf = std::vector<float> ((size_t) fftSize * 2);
    Yin yin;

    // x: mono, n samples at sr; fExpected: the pitch the note should have (YIN searches fExpected / range .. x range)
    // fLow: the spectrum below it is left out (the note's partials cannot be there: body knocks, thumps, rumble that do not
    // move with the key)
    Analysis analyse (const float* xIn, int n, double sr, double fExpected, double range = 4.5, double fLow = 0.0)
    {
        Analysis r;
        yin.sr = sr;
        const float* xp = xIn;
        auto x = [xp] (size_t i) { return xp[i]; };
        // level envelope (20 ms frames)
        const int fl = (int) (0.02 * sr);
        std::vector<float> env;
        float mx = 0;
        for (int s0 = 0; s0 + fl <= n; s0 += fl)
        {
            double acc = 0;
            for (int i = 0; i < fl; ++i) acc += (double) x ((size_t) (s0 + i)) * x ((size_t) (s0 + i));
            env.push_back ((float) std::sqrt (acc / fl));
            mx = std::max (mx, env.back());
        }
        r.levelDb = juce::Decibels::gainToDecibels (mx, -200.0f);
        r.silent = r.levelDb < -70.0f;
        if (r.silent) return r;
        // the window: from 20 ms after the sound reaches a third of its peak level (attack transients skipped), as long as
        // the note stays within 20 dB of its peak (short plucks: the note itself, not the knock / body resonances of the
        // decayed tail), 93 .. 370 ms
        int startFrame = 0;
        while (startFrame < (int) env.size() && env[(size_t) startFrame] < mx * 0.33f) ++startFrame;
        int endFrame = startFrame;
        while (endFrame + 1 < (int) env.size() && env[(size_t) endFrame + 1] >= mx * 0.1f) ++endFrame;
        const int winLen = juce::jlimit (4096, maxWinLen, (endFrame - startFrame) * fl);
        int s0 = std::min (n - winLen, startFrame * fl + (int) (0.02 * sr));
        s0 = std::max (0, s0);
        measure (r, xp, s0, winLen, sr, fExpected, range, fLow);
        return r;
    }

    // the same measurements on a given window (two notes compared on the same stretch of time: sweeps, pitch envelopes)
    Analysis analyseAt (const float* xp, int n, double sr, double fExpected, int from, int len, double range = 4.5, double fLow = 0.0)
    {
        Analysis r;
        yin.sr = sr;
        from = juce::jlimit (0, std::max (0, n - 1), from);
        len = std::min (len, n - from);
        double acc = 0;
        for (int i = 0; i < len; ++i) acc += (double) xp[from + i] * xp[from + i];
        r.levelDb = juce::Decibels::gainToDecibels ((float) std::sqrt (acc / std::max (1, len)), -200.0f);
        r.silent = len < 2048 || r.levelDb < -80.0f;
        if (! r.silent) measure (r, xp, from, len, sr, fExpected, range, fLow);
        return r;
    }

    void measure (Analysis& r, const float* xp, int s0, int winLen, double sr, double fExpected, double range, double fLow)
    {
        const float* w = xp + s0;
        r.from = s0; r.len = winLen;
        float spread = 0;
        r.f0 = yin.run (w, winLen, std::max (25.0, fExpected / range), std::min (5000.0, fExpected * range), r.periodicity, spread);
        r.confident = r.f0 > 0.0f && r.periodicity >= 0.6f && spread < 25.0f;
        r.track = yin.track;
        // log-frequency magnitude spectrum
        std::fill (buf.begin(), buf.end(), 0.0f);
        for (int i = 0; i < winLen; ++i)
            buf[(size_t) i] = w[i] * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (winLen - 1)));
        fft.performFrequencyOnlyForwardTransform (buf.data(), true);
        const double binHz = sr / fftSize;
        float mag = 0;
        int kPeak = 0;
        const int kMin = (int) (std::max (30.0, fLow) / binHz) + 1;
        for (int k = 1; k < fftSize / 2; ++k)
        {
            if (k >= kMin) mag = std::max (mag, buf[(size_t) k]);
            if (k >= kMin && k < fftSize / 2 - 1 && (kPeak == 0 || buf[(size_t) k] > buf[(size_t) kPeak])) kPeak = k;
        }
        if (kPeak > 1)
        {
            const double a = std::log (buf[(size_t) kPeak - 1] + 1e-12), c0 = std::log (buf[(size_t) kPeak] + 1e-12), c = std::log (buf[(size_t) kPeak + 1] + 1e-12);
            const double den = a - 2 * c0 + c;
            r.peakHz = (float) ((kPeak + (std::abs (den) > 1e-12 ? juce::jlimit (-0.5, 0.5, 0.5 * (a - c) / den) : 0.0)) * binHz);
        }
        if (mag <= 0) { r.silent = true; return; }
        {
            const double fund = fLow > 0.0 ? fLow / lowCutRatio : fExpected;
            const int k0 = std::max (2, (int) (fund * std::pow (2.0, -150.0 / 1200.0) / binHz)), k1 = std::min (fftSize / 2 - 2, (int) (fund * std::pow (2.0, 150.0 / 1200.0) / binHz) + 1);
            int kb = -1;
            for (int k = k0; k <= k1; ++k)
                if (buf[(size_t) k] > buf[(size_t) k - 1] && buf[(size_t) k] >= buf[(size_t) k + 1] && (kb < 0 || buf[(size_t) k] > buf[(size_t) kb])) kb = k;
            if (kb > 0 && buf[(size_t) kb] >= mag * 0.0316f)
            {
                const double a = std::log (buf[(size_t) kb - 1] + 1e-12), c0 = std::log (buf[(size_t) kb] + 1e-12), c = std::log (buf[(size_t) kb + 1] + 1e-12);
                const double den = a - 2 * c0 + c;
                r.f1Hz = (float) ((kb + (std::abs (den) > 1e-12 ? juce::jlimit (-0.5, 0.5, 0.5 * (a - c) / den) : 0.0)) * binHz);
            }
        }
        std::vector<float> g ((size_t) gridSize());
        const double half = std::pow (2.0, gridStepCents / 2400.0);
        for (int i = 0; i < gridSize(); ++i)
        {
            const double f = gridLoHz * std::pow (2.0, i * gridStepCents / 1200.0);
            const double b0 = f / half / binHz, b1 = f * half / binHz;
            float v = 0;
            if (b1 - b0 >= 1.0)
                for (int k = (int) std::ceil (b0); k <= (int) std::floor (b1) && k < fftSize / 2; ++k) v = std::max (v, buf[(size_t) k]);
            else
            {
                const double fb = f / binHz;
                const int k = (int) fb;
                const float t = (float) (fb - k);
                if (k + 1 < fftSize / 2) v = buf[(size_t) k] * (1 - t) + buf[(size_t) k + 1] * t;
            }
            g[(size_t) i] = f < fLow ? 0.0f : std::sqrt (v / mag);   // amplitude, square-root compressed: the note's partials dominate, quiet fixed
                                                   // components (reverb modes, noise) do not pull the correlation to 0
        }
        // peaks only: minus the local average (+-60 cents), negatives clipped
        r.logSpec.assign ((size_t) gridSize(), 0.0f);
        const int hw = (int) (60.0 / gridStepCents);
        double run = 0; int cnt = 0;
        for (int i = 0; i < std::min (gridSize(), hw); ++i) { run += g[(size_t) i]; ++cnt; }
        for (int i = 0; i < gridSize(); ++i)
        {
            if (i + hw < gridSize()) { run += g[(size_t) (i + hw)]; ++cnt; }
            if (i - hw - 1 >= 0) { run -= g[(size_t) (i - hw - 1)]; --cnt; }
            r.logSpec[(size_t) i] = std::max (0.0f, g[(size_t) i] - (float) (run / std::max (1, cnt)));
        }
    }
};

// cross-correlation of two log spectra: the shift (cents) that maps a onto b, and the normalised peak value; lags searched
// in centre +- range (cents)
inline float spectralShift (const std::vector<float>& a, const std::vector<float>& b, float& peak, float centre = 0.0f, float range = 400.0f)
{
    peak = 0;
    if (a.empty() || b.empty()) return 0;
    const int c0 = (int) std::lround (centre / gridStepCents), lagSteps = (int) std::lround (range / gridStepCents);
    std::vector<float> c ((size_t) (2 * lagSteps + 1));
    for (int L = c0 - lagSteps; L <= c0 + lagSteps; ++L)
    {
        double ab = 0, aa = 0, bb = 0;
        for (int i = std::max (0, -L); i < gridSize() && i + L < gridSize(); ++i)
        {
            const double u = a[(size_t) i], v = b[(size_t) (i + L)];
            ab += u * v; aa += u * u; bb += v * v;
        }
        c[(size_t) (L - c0 + lagSteps)] = aa > 0 && bb > 0 ? (float) (ab / std::sqrt (aa * bb)) : 0.0f;
    }
    int best = 0;
    for (int i = 1; i < (int) c.size(); ++i) if (c[(size_t) i] > c[(size_t) best]) best = i;
    peak = c[(size_t) best];
    double off = 0;
    if (best > 0 && best < (int) c.size() - 1)
    {
        const double p = c[(size_t) best - 1], q = c[(size_t) best], r = c[(size_t) best + 1], den = p - 2 * q + r;
        if (std::abs (den) > 1e-12) off = juce::jlimit (-0.5, 0.5, 0.5 * (p - r) / den);
    }
    return (float) ((best - lagSteps + c0 + off) * gridStepCents);
}

// two YIN tracks on the same window (pitch sweeps, vibrato, sequences: frame by frame the interval stays): the median
// interval (cents, b over a) of the frames periodic in both; `share` = those frames / all frames (0 = no measure)
inline float trackInterval (const std::vector<float>& a, const std::vector<float>& b, float& share)
{
    std::vector<float> d;
    const size_t n = std::min (a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
        if (a[i] > 0.0f && b[i] > 0.0f) d.push_back (1200.0f * std::log2 (b[i] / a[i]));
    share = n > 0 ? (float) d.size() / (float) n : 0.0f;
    if (d.empty()) return 0.0f;
    std::sort (d.begin(), d.end());
    return d[d.size() / 2];
}

inline float foldOctave (float cents) { return cents - 1200.0f * std::round (cents / 1200.0f); }

// sweeps (risers, pitch envelopes, slow glides): two notes compared frame by frame on the same stretch of time - short
// frames (4096 samples, hop 2048) in which the pitch hardly moves, the log spectra of each frame cross-correlated, the
// median shift of the frames that line up clearly (peak >= 0.5). Every partial of a sweep moves with the key at every
// moment; over a long window the smeared partials are weighted by the filter / FX differently on each key, the frames
// are not. Returns 0 with share 0 when fewer than 3 frames line up.
inline float frameShift (const float* xa, int na, const float* xb, int nb, int from, int len, double sr, double fLow, float expect, float& share)
{
    constexpr int fl = 4096, order = 13, n = 1 << order;
    share = 0.0f;
    juce::dsp::FFT fft { order };
    std::vector<float> buf ((size_t) n * 2);
    const double binHz = sr / n;
    const int gs = gridSize();
    auto spec = [&] (const float* x, int s0, std::vector<float>& out)
    {
        std::fill (buf.begin(), buf.end(), 0.0f);
        for (int i = 0; i < fl; ++i) buf[(size_t) i] = x[s0 + i] * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (fl - 1)));
        fft.performFrequencyOnlyForwardTransform (buf.data(), true);
        float mag = 0;
        for (int k = 1; k < n / 2; ++k) mag = std::max (mag, buf[(size_t) k]);
        out.assign ((size_t) gs, 0.0f);
        if (mag <= 0) return false;
        std::vector<float> g ((size_t) gs);
        for (int i = 0; i < gs; ++i)
        {
            const double f = gridLoHz * std::pow (2.0, i * gridStepCents / 1200.0), fb = f / binHz;
            const int k = (int) fb;
            const float t = (float) (fb - k);
            const float v = k + 1 < n / 2 ? buf[(size_t) k] * (1 - t) + buf[(size_t) k + 1] * t : 0.0f;
            g[(size_t) i] = f < fLow ? 0.0f : std::sqrt (v / mag);
        }
        const int hw = (int) (60.0 / gridStepCents);
        double run = 0; int cnt = 0;
        for (int i = 0; i < std::min (gs, hw); ++i) { run += g[(size_t) i]; ++cnt; }
        for (int i = 0; i < gs; ++i)
        {
            if (i + hw < gs) { run += g[(size_t) (i + hw)]; ++cnt; }
            if (i - hw - 1 >= 0) { run -= g[(size_t) (i - hw - 1)]; --cnt; }
            out[(size_t) i] = std::max (0.0f, g[(size_t) i] - (float) (run / std::max (1, cnt)));
        }
        return true;
    };
    std::vector<float> sa, sb, shifts;
    int frames = 0;
    for (int s0 = from; s0 + fl <= from + len && s0 + fl <= na && s0 + fl <= nb; s0 += fl / 2)
    {
        ++frames;
        if (! spec (xa, s0, sa) || ! spec (xb, s0, sb)) continue;
        float peak = 0;
        const float c = spectralShift (sa, sb, peak, expect, 150.0f);
        if (peak >= 0.5f) shifts.push_back (c);
    }
    if (shifts.size() < 3) return 0.0f;
    share = (float) shifts.size() / (float) std::max (1, frames);
    std::sort (shifts.begin(), shifts.end());
    return shifts[shifts.size() / 2];
}

// two notes compared the way the pitch test compares neighbouring keys (a = the lower / first note, b = the other one,
// `expect` = the interval they should have, cents): YIN on both, the spectra on each note's own window and b on a's window
// (the same stretch of time: sweeps, pitch envelopes, short notes), the strongest partial, the YIN tracks frame by frame.
// The interval passes when any of them is `expect` +- tol.
struct Interval
{
    float yin = 0, xc = 0, xcPeak = 0, xc2 = 0, xc2Peak = 0, pk = 9999, pk2 = 9999, tr = 0, trShare = 0, f1 = 9999, f1b = 9999;
    bool yinOk = false, xcOk = false, pkOk = false, trOk = false, f1Ok = false;
    bool evaluated = false, unpitched = false, same = false;
    bool ok() const { return yinOk || xcOk || pkOk || trOk || f1Ok; }

    // the interval most of the measurements agree on (within +-tol): the largest group, YIN's group on a tie; `votes` =
    // how many agree. false when nothing was measured.
    bool consensus (float expect, float& cents, int& votes, float tol = 10.0f) const
    {
        std::vector<std::pair<float, bool>> v;   // (value, is YIN based)
        auto fold = [expect] (float c) { return expect + foldOctave (c - expect); };
        if (yin != 0.0f) v.push_back ({ fold (yin), true });
        if (trShare >= 0.6f) v.push_back ({ fold (tr), true });
        if (f1 < 9000.0f) v.push_back ({ f1, true });
        if (f1b < 9000.0f) v.push_back ({ f1b, true });
        if (xcPeak >= 0.3f) v.push_back ({ xc, false });
        if (xc2Peak >= 0.3f) v.push_back ({ xc2, false });
        if (pk < 9000.0f && std::abs (fold (pk) - expect) < 90.0f) v.push_back ({ fold (pk), false });
        if (pk2 < 9000.0f && std::abs (fold (pk2) - expect) < 90.0f) v.push_back ({ fold (pk2), false });
        int best = 0; bool bestYin = false; float bestC = 0;
        for (auto& c : v)
        {
            int n = 0; bool hasYin = false; double sum = 0;
            for (auto& d : v) if (std::abs (d.first - c.first) <= tol) { ++n; hasYin |= d.second; sum += d.first; }
            if (n > best || (n == best && hasYin && ! bestYin)) { best = n; bestYin = hasYin; bestC = (float) (sum / n); }
        }
        votes = best; cents = bestC;
        return best > 0;
    }
};

inline Interval compareNotes (const Analysis& a, const Analysis& b, const float* monoB, int nB, double sr, double fB, double lowCutA, Analyser& an,
                              float expect = 100.0f, float tol = 10.0f)
{
    Interval r;
    if (a.silent || b.silent) return r;
    if (a.confident && b.confident)
    {
        r.yin = 1200.0f * std::log2 (b.f0 / a.f0);
        r.yinOk = std::abs (foldOctave (r.yin - expect)) <= tol;
    }
    const auto bSame = an.analyseAt (monoB, nB, sr, fB, a.from, a.len, 4.5, lowCutA);
    r.xc = spectralShift (a.logSpec, b.logSpec, r.xcPeak, expect);
    if (! bSame.silent) r.xc2 = spectralShift (a.logSpec, bSame.logSpec, r.xc2Peak, expect);
    r.xcOk = (r.xcPeak >= 0.2f && std::abs (r.xc - expect) <= tol) || (r.xc2Peak >= 0.2f && std::abs (r.xc2 - expect) <= tol);
    auto peakStep = [] (float fa, float fb) { return fa > 0.0f && fb > 0.0f ? 1200.0f * std::log2 (fb / fa) : 9999.0f; };
    r.pk = peakStep (a.peakHz, b.peakHz);
    if (! bSame.silent) r.pk2 = peakStep (a.peakHz, bSame.peakHz);
    r.pkOk = (r.pk < 9000.0f && std::abs (foldOctave (r.pk - expect)) <= tol) || (r.pk2 < 9000.0f && std::abs (foldOctave (r.pk2 - expect)) <= tol);
    if (! bSame.silent) r.tr = trackInterval (a.track, bSame.track, r.trShare);
    // the fundamental partials (both notes' own windows, and b on a's window)
    if (a.f1Hz > 0.0f && b.f1Hz > 0.0f) r.f1 = 1200.0f * std::log2 (b.f1Hz / a.f1Hz);
    if (a.f1Hz > 0.0f && ! bSame.silent && bSame.f1Hz > 0.0f) r.f1b = 1200.0f * std::log2 (bSame.f1Hz / a.f1Hz);
    r.f1Ok = (r.f1 < 9000.0f && std::abs (r.f1 - expect) <= tol) || (r.f1b < 9000.0f && std::abs (r.f1b - expect) <= tol);
    const bool trPitched = r.trShare >= 0.6f;
    r.trOk = trPitched && std::abs (foldOctave (r.tr - expect)) <= tol;
    // pitched: YIN on both notes, spectra that line up clearly (noise lines up at ~0.4 by chance) or periodic frames
    const bool yinPitched = (a.confident && b.confident) || trPitched, xcPitched = std::max (r.xcPeak, r.xc2Peak) >= 0.5f;
    r.unpitched = ! yinPitched && ! xcPitched;
    r.evaluated = ! r.unpitched || r.ok();
    if (expect != 0.0f)
        r.same = (a.confident && b.confident && std::abs (foldOctave (r.yin)) <= 15.0f) || (trPitched && std::abs (foldOctave (r.tr)) <= 15.0f)
              || (xcPitched && std::abs (r.xcPeak >= r.xc2Peak ? r.xc : r.xc2) <= 15.0f);
    return r;
}

} // namespace vaultpitch
