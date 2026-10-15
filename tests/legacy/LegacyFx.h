#pragma once
// FROZEN COPY of the CHROME VAULT v2 master FX (kk::FxRack, kk::FdnReverb, kk::MasterStage as of the v2 factory bank),
// namespace kkv2. Only the v3 conversion tests use it (tests/legacy/LegacyVaultEngine.*): the v2 engine with this rack is
// the reference a converted v3 preset must sound like. Never edit (see docs/VAULT.md, "v3").
#include "../../Source/DspUtil.h"
#include "../../Source/Fx.h"
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace kkv2
{
using namespace kk;

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
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    inline float tick (float x) { const float y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
    void shelf (bool high, float f, float gainDb, float sr)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f), w = twoPi * f / sr, cs = std::cos (w), sn = std::sin (w);
        const float al = sn / 2.0f * std::sqrt (2.0f), sq = 2.0f * std::sqrt (A) * al;
        float B0, B1, B2, A0, A1, A2;
        if (! high) { B0 = A * ((A + 1) - (A - 1) * cs + sq); B1 = 2 * A * ((A - 1) - (A + 1) * cs); B2 = A * ((A + 1) - (A - 1) * cs - sq);
                      A0 = (A + 1) + (A - 1) * cs + sq; A1 = -2 * ((A - 1) + (A + 1) * cs); A2 = (A + 1) + (A - 1) * cs - sq; }
        else        { B0 = A * ((A + 1) + (A - 1) * cs + sq); B1 = -2 * A * ((A - 1) + (A + 1) * cs); B2 = A * ((A + 1) + (A - 1) * cs - sq);
                      A0 = (A + 1) - (A - 1) * cs + sq; A1 = 2 * ((A - 1) - (A + 1) * cs); A2 = (A + 1) - (A - 1) * cs - sq; }
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};

class FxRack
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = (float) sampleRate;
        os.initProcessing ((size_t) maxBlock);
        os.reset();
        low[0].assign ((size_t) maxBlock, 0.0f); low[1].assign ((size_t) maxBlock, 0.0f);
        for (auto& d : wowDl) d.prepare ((int) (0.05f * sr));
        for (auto& d : chDl)  d.prepare ((int) (0.06f * sr));
        for (auto& d : flDl)  d.prepare ((int) (0.02f * sr));
        for (auto& d : dly)   d.prepare ((int) (4.5f * sr));
        for (auto& d : ring)  d.prepare ((int) (1.5f * sr));
        for (auto& d : revRing) d.prepare ((int) (1.5f * sr));
        for (auto& d : htRing) d.prepare ((int) (6.5f * sr));
        envFast = envSlow = 0; smPunch = smHalf = 0;
        atkFast = 1.0f - std::exp (-1.0f / (0.0008f * sr)); relFast = 1.0f - std::exp (-1.0f / (0.03f * sr));
        atkSlow = 1.0f - std::exp (-1.0f / (0.025f * sr)); relSlow = 1.0f - std::exp (-1.0f / (0.25f * sr));
        ghostRing.prepare ((int) (1.5f * sr));
        ghostDly.prepare ((int) (4.5f * sr));
        fdn.prepare (sampleRate); mastering.prepare (sampleRate);
        ghostRev.setSampleRate (sampleRate); ghostRev.reset();
        for (auto& f : splitA) f.setHz (140.0f, sr);
        for (auto& f : splitB) f.setHz (140.0f, sr);
        for (auto& f : sideLp) f.setHz (120.0f, sr);
        for (auto& f : tapeLp) f.setHz (3200.0f, sr);
        ghostHp.setHz (250.0f, sr);
        for (auto& d : dc) d.setHz (8.0f, sr);
        for (auto& d : dc) d.x1 = d.y1 = 0;
        for (auto* f : { &splitA[0], &splitA[1], &splitB[0], &splitB[1], &sideLp[0], &sideLp[1], &tapeLp[0], &tapeLp[1], &ghostHp }) f->reset();
        apFb[0] = apFb[1] = 0;
        for (auto& h : subsonic) { h.b0 = 1; h.z1 = h.z2 = 0; }
        {   // 2nd-order Butterworth high-pass at 22 Hz for bass mode
            const float w = twoPi * 22.0f / sr, cs = std::cos (w), al = std::sin (w) / std::sqrt (2.0f), a0 = 1.0f + al;
            for (auto& h : subsonic) { h.b0 = (1 + cs) / 2 / a0; h.b1 = -(1 + cs) / a0; h.b2 = (1 + cs) / 2 / a0; h.a1 = -2 * cs / a0; h.a2 = (1 - al) / a0; }
        }
        for (auto& b : bodyS) for (auto& s : b) s.reset();
        for (auto& a : apState) for (auto& v : a) v = 0;
        crackle.seed (777);
        noiseEnv = 0; noiseRel = std::exp (-1.0f / (0.25f * sr));
        smDrive = smChorus = smDelay = smBody = smPhaser = smFlanger = smReverse = 0; smDelaySamples = -1;
        wowPhase = flutPhase = chPhase = phPhase = flPhase = 0; holdCount = 0; holdL = holdR = 0;
        grainPhase = revGrain = 0; lastStep = -1; evType = 0;
        lastEqLow = lastEqHigh = 999.0f;
        for (auto& e : eq) e = {};
    }

    void process (float* L, float* R, const float* G, int n, const FxParams& p)
    {
        sm = 1.0f - std::exp (-1.0f / (0.02f * sr));   // 20 ms parameter smoothing
        for (int slot : p.order)
        {
            switch (slot)
            {
                case fxDrive:   drive (L, R, n, p); break;
                case fxBody:    body (L, R, n, p); break;
                case fxLofi:    lofi (L, R, n, p); break;
                case fxCircuit: circuitBend (L, R, n, p); break;
                case fxChorus:  chorus (L, R, n, p); break;
                case fxPhaser:  phaser (L, R, n, p); break;
                case fxFlanger: flanger (L, R, n, p); break;
                case fxEq:      eqStage (L, R, n, p); break;
                case fxDelay:   delay (L, R, n, p); break;
                case fxReverb:  reverb (L, R, n, p); break;
                case fxReverse: reverseStage (L, R, n, p); break;
                case fxPunch:   punchStage (L, R, n, p); break;
                case fxHalftime: halftimeStage (L, R, n, p); break;
                default: break;
            }
        }
        ghostBus (L, R, G, n, p);

        // width, mono lows, output
        const float sideGain = p.width * 2.0f;
        for (int i = 0; i < n; ++i)
        {
            const float m = (L[i] + R[i]) * 0.5f;
            float s = (L[i] - R[i]) * 0.5f * sideGain;
            if (p.monoLows) s -= sideLp[0].lp (s);
            float l = dc[0].tick ((m + s) * p.outGain), r = dc[1].tick ((m - s) * p.outGain);
            if (p.monoLows) { l = subsonic[0].tick (l); r = subsonic[1].tick (r); }
            L[i] = l; R[i] = r;
        }
        peakPre = std::max (peakPre, mastering.process (L, R, n, p.master, p.monoLows ? -1.0f : p.width));   // MASTER stage + limiter
    }

    float peakPre = 0;   // pre-limiter peak (for overload warning), reset by owner

    // Silences every delay line / filter state without allocating (audio thread safe). Used by CHROME VAULT at a preset
    // switch: the new sound starts in a clean rack, so the old content can't jump when the FX settings change (a reverb
    // size change moves the read points of lines still full of the old sound -> clicks). EVOLVE does not call it.
    void clearState()
    {
        os.reset();
        for (int c = 0; c < 2; ++c)
            for (auto* d : { &wowDl[c], &chDl[c], &flDl[c], &dly[c], &ring[c], &revRing[c], &htRing[c] }) d->clear();
        ghostRing.clear(); ghostDly.clear();
        ghostRev.reset(); fdn.reset(); mastering.reset();
        for (auto* f : { &splitA[0], &splitA[1], &splitB[0], &splitB[1], &sideLp[0], &sideLp[1], &tapeLp[0], &tapeLp[1], &ghostHp }) f->reset();
        for (auto& d : dc) d.x1 = d.y1 = 0;
        for (auto& b : bodyS) for (auto& s : b) s.reset();
        for (auto& b : eq) b.z1 = b.z2 = 0;
        for (auto& b : subsonic) b.z1 = b.z2 = 0;
        for (auto& a : apState) for (auto& v : a) v = 0;
        apFb[0] = apFb[1] = 0;
        envFast = envSlow = 0; noiseEnv = 0;
        smDrive = smChorus = smDelay = smBody = smPhaser = smFlanger = smReverse = smPunch = smHalf = 0; smDelaySamples = -1;
        holdCount = 0; holdL = holdR = 0;
    }

private:
    // ---------------------------------------------------------------------------------
    void drive (float* L, float* R, int n, const FxParams& p)
    {
        if (p.drive <= 0.001f && smDrive <= 0.001f) return;
        if (p.cleanLow)
            for (int i = 0; i < n; ++i)
            {
                const float lL = splitB[0].lp (splitA[0].lp (L[i]));
                const float lR = splitB[1].lp (splitA[1].lp (R[i]));
                low[0][(size_t) i] = lL; low[1][(size_t) i] = lR;
                L[i] -= lL; R[i] -= lR;
            }
        if (p.eco)
        {
            for (int i = 0; i < n; ++i)
            {
                smDrive += (p.drive - smDrive) * sm;
                L[i] = shape (L[i], smDrive, p.driveType); R[i] = shape (R[i], smDrive, p.driveType);
            }
        }
        else
        {
            float* chans[] { L, R };
            juce::dsp::AudioBlock<float> block (chans, 2, (size_t) n);
            auto up = os.processSamplesUp (block);
            const int un = (int) up.getNumSamples();
            float* uL = up.getChannelPointer (0); float* uR = up.getChannelPointer (1);
            const float smU = sm * 0.5f;
            for (int i = 0; i < un; ++i)
            {
                smDrive += (p.drive - smDrive) * smU;
                uL[i] = shape (uL[i], smDrive, p.driveType);
                uR[i] = shape (uR[i], smDrive, p.driveType);
            }
            os.processSamplesDown (block);
        }
        if (p.cleanLow)
            for (int i = 0; i < n; ++i) { L[i] += low[0][(size_t) i]; R[i] += low[1][(size_t) i]; }
    }

    void body (float* L, float* R, int n, const FxParams& p)
    {
        if (p.body <= 0 && smBody <= 0.001f) return;
        static const float modes[6][5] { { 420, 1180, 2350, 3900, 5600 }, { 520, 1250, 1680, 2750, 4100 },
                                        { 1450, 2610, 3900, 5500, 7200 }, { 310, 855, 1680, 2780, 4150 },
                                        { 180, 420, 890, 1600, 2500 },    { 110, 220, 330, 440, 660 } };
        static const float qs[6] { 30, 70, 90, 45, 8, 60 };
        const int b = std::max (0, p.body - 1);
        SvfCoef c[5];
        for (int k = 0; k < 5; ++k) c[k].set (modes[b][k], 1.0f / qs[b], sr);
        const float target = p.body > 0 ? p.bodyMix : 0.0f;
        for (int i = 0; i < n; ++i)
        {
            smBody += (target - smBody) * sm;
            float wL = 0, wR = 0;
            for (int k = 0; k < 5; ++k)
            {
                bodyS[0][k].tick (c[k], L[i]); bodyS[1][k].tick (c[k], R[i]);
                const float g = c[k].k * (1.0f - 0.12f * (float) k);
                wL += bodyS[0][k].bp * g; wR += bodyS[1][k].bp * g;
            }
            wL = std::tanh (wL * 1.4f); wR = std::tanh (wR * 1.4f);
            L[i] += (wL - L[i]) * smBody; R[i] += (wR - R[i]) * smBody;
        }
    }

    void lofi (float* L, float* R, int n, const FxParams& p)
    {
        if (p.crush <= 0.001f && p.wow <= 0.001f) return;
        const float levels = std::exp2 (16.0f - p.crush * 12.0f);
        const int hold = 1 + (int) (p.crush * p.crush * 10.0f);
        const float wIncA = 0.55f / sr, wIncB = 6.5f / sr;
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            if (p.wow > 0.001f)
            {
                wowDl[0].push (l); wowDl[1].push (r);
                const float d = (0.004f + p.wow * (0.0035f * std::sin (twoPi * wowPhase) + 0.0004f * std::sin (twoPi * flutPhase))) * sr;
                wowPhase += wIncA; flutPhase += wIncB;
                if (wowPhase >= 1) wowPhase -= 1;
                if (flutPhase >= 1) flutPhase -= 1;
                l = wowDl[0].read (d); r = wowDl[1].read (d);
                // vinyl hiss + crackle only while there is sound (no pops in silence)
                const float in = std::abs (l) + std::abs (r);
                noiseEnv = in > noiseEnv ? in : noiseEnv * noiseRel;
                const float gate = std::min (1.0f, noiseEnv * 6.0f);
                if (gate > 1.0e-4f)
                {
                    float noiseV = crackle.bi() * 0.002f * p.wow;
                    if (crackle.uni() < p.wow * 12.0f / sr) noiseV += crackle.bi() * 0.12f * p.wow;
                    l += noiseV * gate; r += noiseV * gate;
                }
            }
            if (p.crush > 0.001f)
            {
                if (++holdCount >= hold) { holdCount = 0; holdL = l; holdR = r; }
                l = std::round (holdL * levels) / levels; r = std::round (holdR * levels) / levels;
            }
            L[i] = l; R[i] = r;
        }
    }

    void circuitBend (float* L, float* R, int n, const FxParams& p)
    {
        const double beatsPerSample = p.bpm / 60.0 / sr;
        const double stepBeats = p.circBeats;
        const float stepLen = (float) (stepBeats / beatsPerSample);
        for (int i = 0; i < n; ++i)
        {
            ring[0].push (L[i]); ring[1].push (R[i]);
            const double beat = p.beatPos + beatsPerSample * i;
            const int64_t step = (int64_t) std::floor (beat / stepBeats);
            if (step != lastStep)
            {
                lastStep = step;
                Rng r; r.seed (hash32 ((uint32_t) step * 2654435761u ^ p.seed));
                evType = (p.circuit > 0.001f && r.uni() < p.circuit * 0.75f) ? 1 + (int) (r.uni() * 4.0f) : 0;
                evSlice = std::max (8.0f, stepLen / (float) (2 << (int) (r.uni() * 3.0f)));
                evT = 0;
            }
            if (evType == 0) continue;
            const float t = evT++;
            const float fade = std::min ({ 1.0f, t / 48.0f, std::max (0.0f, stepLen - t) / 48.0f });
            float wl = 0, wr = 0;
            switch (evType)
            {
                case 1: { const float d = 1.0f + t + evSlice - std::fmod (t, evSlice); wl = ring[0].read (d); wr = ring[1].read (d); break; } // stutter
                case 2: break;                                                                                                          // dropout
                case 3: { const float d = 1.0f + t * 0.5f; wl = ring[0].read (d); wr = ring[1].read (d); break; }                          // tape stop
                default: { const float d = 1.0f + std::fmod (t, 16.0f); wl = std::round (ring[0].read (d) * 6.0f) / 6.0f;                 // crush
                           wr = std::round (ring[1].read (d) * 6.0f) / 6.0f; break; }
            }
            L[i] += (wl - L[i]) * fade; R[i] += (wr - R[i]) * fade;
        }
    }

    void chorus (float* L, float* R, int n, const FxParams& p)
    {
        const float inc = 0.33f / sr;
        for (int i = 0; i < n; ++i)
        {
            smChorus += (p.chorus - smChorus) * sm;
            chDl[0].push (L[i]); chDl[1].push (R[i]);
            if (smChorus < 0.001f) continue;
            const float mL = std::sin (twoPi * chPhase), mR = std::sin (twoPi * (chPhase + 0.25f));
            chPhase += inc; if (chPhase >= 1) chPhase -= 1;
            const float wl = chDl[0].read ((0.012f + 0.004f * mL) * sr);
            const float wr = chDl[1].read ((0.014f + 0.004f * mR) * sr);
            const float mix = smChorus * 0.7f;
            L[i] = L[i] * (1.0f - mix * 0.4f) + wr * mix;
            R[i] = R[i] * (1.0f - mix * 0.4f) + wl * mix;
        }
    }

    void phaser (float* L, float* R, int n, const FxParams& p)
    {
        if (p.phaser <= 0.001f && smPhaser <= 0.001f) return;
        const float inc = 0.35f / sr;
        float a[2] { 0, 0 };
        for (int i = 0; i < n; ++i)
        {
            smPhaser += (p.phaser - smPhaser) * sm;
            if ((i & 15) == 0)
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float m = 0.5f + 0.5f * std::sin (twoPi * (phPhase + ch * 0.25f));
                    const float f = 250.0f * std::exp2 (m * 3.5f);
                    const float t = std::tan (pi * f / sr);
                    a[ch] = (t - 1.0f) / (t + 1.0f);
                }
            phPhase += inc; if (phPhase >= 1) phPhase -= 1;
            float* io[2] { &L[i], &R[i] };
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = *io[ch] + apFb[ch] * 0.5f * smPhaser;
                for (int k = 0; k < 4; ++k)
                {
                    const float y = a[ch] * x + apState[ch][k];
                    apState[ch][k] = x - a[ch] * y;
                    x = y;
                }
                apFb[ch] = x;
                *io[ch] = *io[ch] * (1.0f - 0.5f * smPhaser) + x * 0.5f * smPhaser;   // classic notch mix
            }
        }
    }

    void flanger (float* L, float* R, int n, const FxParams& p)
    {
        if (p.flanger <= 0.001f && smFlanger <= 0.001f) { for (int i = 0; i < n; ++i) { flDl[0].push (L[i]); flDl[1].push (R[i]); } return; }
        const float inc = 0.22f / sr;
        for (int i = 0; i < n; ++i)
        {
            smFlanger += (p.flanger - smFlanger) * sm;
            const float mL = 0.5f + 0.5f * std::sin (twoPi * flPhase), mR = 0.5f + 0.5f * std::sin (twoPi * (flPhase + 0.3f));
            flPhase += inc; if (flPhase >= 1) flPhase -= 1;
            const float dl = (0.0007f + 0.0045f * mL) * sr, dr = (0.0007f + 0.0045f * mR) * sr;
            const float wl = flDl[0].read (dl), wr = flDl[1].read (dr);
            flDl[0].push (L[i] + wl * 0.65f * smFlanger);
            flDl[1].push (R[i] + wr * 0.65f * smFlanger);
            L[i] = L[i] * (1.0f - 0.3f * smFlanger) + wl * 0.6f * smFlanger;
            R[i] = R[i] * (1.0f - 0.3f * smFlanger) + wr * 0.6f * smFlanger;
        }
    }

    void eqStage (float* L, float* R, int n, const FxParams& p)
    {
        if (std::abs (p.eqLow) < 0.05f && std::abs (p.eqHigh) < 0.05f) return;
        if (p.eqLow != lastEqLow)  { for (int ch = 0; ch < 2; ++ch) eq[ch].shelf (false, 160.0f, p.eqLow, sr);  lastEqLow = p.eqLow; }
        if (p.eqHigh != lastEqHigh){ for (int ch = 0; ch < 2; ++ch) eq[ch + 2].shelf (true, 6000.0f, p.eqHigh, sr); lastEqHigh = p.eqHigh; }
        for (int i = 0; i < n; ++i)
        {
            L[i] = eq[2].tick (eq[0].tick (L[i]));
            R[i] = eq[3].tick (eq[1].tick (R[i]));
        }
    }

    void delay (float* L, float* R, int n, const FxParams& p)
    {
        const float target = (float) (p.delayBeats * 60.0 / p.bpm) * sr;
        if (smDelaySamples < 0) smDelaySamples = target;
        for (int i = 0; i < n; ++i)
        {
            smDelay += (p.delayMix - smDelay) * sm;
            smDelaySamples += (target - smDelaySamples) * 0.0005f;
            float d = smDelaySamples;
            if (p.delayMode == 2) { d *= 1.0f + 0.004f * std::sin (twoPi * wowPhase); }
            const float dl = dly[0].read (d), dr = dly[1].read (p.delayMode == 1 ? d * 1.02f : d);
            switch (p.delayMode)
            {
                case 1:  dly[0].push (L[i] + dl * p.delayFb); dly[1].push (R[i] + dr * p.delayFb); break;       // stereo
                case 2:  { const float fb = tapeLp[0].lp (std::tanh ((dl + dr) * 0.5f * p.delayFb * 1.2f));        // tape
                           dly[0].push ((L[i] + R[i]) * 0.5f + fb); dly[1].push ((L[i] + R[i]) * 0.5f + fb); break; }
                default: dly[0].push ((L[i] + R[i]) * 0.5f + dr * p.delayFb); dly[1].push (dl * p.delayFb); break; // ping-pong
            }
            L[i] += dl * smDelay * 0.8f; R[i] += dr * smDelay * 0.8f;
        }
    }

    void reverb (float* L, float* R, int n, const FxParams& p)
    {
        fdn.process (L, R, n, p.revType, p.revSize, p.revMix, p.freeze);
    }

    void reverseStage (float* L, float* R, int n, const FxParams& p)
    {
        const float P = (float) (60.0 / p.bpm * 0.5) * sr;   // 1/8 note reversed grains
        const float gInc = 1.0f / std::max (64.0f, P);
        for (int i = 0; i < n; ++i)
        {
            smReverse += (p.reverse - smReverse) * sm;
            revRing[0].push (L[i]); revRing[1].push (R[i]);
            if (smReverse < 0.001f) continue;
            float g1 = revGrain, g2 = revGrain + 0.5f; if (g2 >= 1) g2 -= 1;
            const float w1 = std::sin (pi * g1), w2 = std::sin (pi * g2);
            const float l = revRing[0].read (1.0f + 2.0f * g1 * P) * w1 + revRing[0].read (1.0f + 2.0f * g2 * P) * w2;
            const float r = revRing[1].read (1.0f + 2.0f * g1 * P) * w1 + revRing[1].read (1.0f + 2.0f * g2 * P) * w2;
            revGrain += gInc; if (revGrain >= 1) revGrain -= 1;
            L[i] += (l * 0.7f - L[i]) * smReverse; R[i] += (r * 0.7f - R[i]) * smReverse;
        }
    }

    // Transient shaper: fast vs slow envelope, boosts the attack up to about +12 dB
    void punchStage (float* L, float* R, int n, const FxParams& p)
    {
        if (p.punch <= 0.001f && smPunch <= 0.001f) return;
        for (int i = 0; i < n; ++i)
        {
            smPunch += (p.punch - smPunch) * sm;
            const float x = std::abs (L[i]) + std::abs (R[i]);
            envFast += (x - envFast) * (x > envFast ? atkFast : relFast);
            envSlow += (x - envSlow) * (x > envSlow ? atkSlow : relSlow);
            const float tr = std::max (0.0f, envFast - envSlow) / (envSlow + 1.0e-3f);
            const float g = 1.0f + std::min (3.0f, tr * 3.0f) * smPunch;
            L[i] *= g; R[i] *= g;
        }
    }

    // Half-speed buffer: replays each 2-beat window at half speed (half-time feel), synced to the host beat
    void halftimeStage (float* L, float* R, int n, const FxParams& p)
    {
        const double beatsPerSample = p.bpm / 60.0 / sr;
        const float winLen = (float) (2.0 / beatsPerSample);   // 2 beats of output
        for (int i = 0; i < n; ++i)
        {
            htRing[0].push (L[i]); htRing[1].push (R[i]);
            smHalf += (p.halftime - smHalf) * sm;
            if (smHalf < 0.001f) continue;
            const double beat = p.beatPos + beatsPerSample * i;
            const float t = (float) (beat / 2.0 - std::floor (beat / 2.0)) * winLen;   // 0 .. winLen
            const float d = 1.0f + t * 0.5f;                                           // read at half speed
            const float fade = std::min ({ 1.0f, t / 256.0f, (winLen - t) / 256.0f });
            const float wl = htRing[0].read (d) * fade, wr = htRing[1].read (d) * fade;
            L[i] += (wl - L[i]) * smHalf; R[i] += (wr - R[i]) * smHalf;
        }
    }

    void ghostBus (float* L, float* R, const float* G, int n, const FxParams& p)
    {
        if (p.ghost <= 0.001f) return;
        const float P = 0.28f * sr;
        const float gInc = 1.0f / P;
        const float dSamples = 0.75f * 60.0f / (float) p.bpm * sr;
        for (int i = 0; i < n; ++i)
        {
            const float send = G[i] + (L[i] + R[i]) * 0.5f * p.ghost * 0.45f;
            ghostRing.push (send - ghostHp.lp (send));
            float gOut;
            if (p.ghostRev)
            {
                float g1 = grainPhase, g2 = grainPhase + 0.5f; if (g2 >= 1) g2 -= 1;
                const float rev1 = ghostRing.read (1.0f + 2.0f * g1 * P) * std::sin (pi * g1);
                const float rev2 = ghostRing.read (1.0f + 2.0f * g2 * P) * std::sin (pi * g2);
                grainPhase += gInc; if (grainPhase >= 1) grainPhase -= 1;
                gOut = (rev1 + rev2) * 0.7f + G[i] * 0.6f;
            }
            else gOut = ghostRing.read (P * 0.5f) + G[i] * 0.6f;
            ghostDly.push (gOut + ghostDly.read (dSamples) * 0.45f);
            gL[i] = gOut; gR[i] = ghostDly.read (dSamples) * 0.8f;
        }
        juce::Reverb::Parameters gp;
        gp.roomSize = 0.7f + p.ghostBlur * 0.28f; gp.damping = 0.6f;
        gp.wetLevel = 0.4f + p.ghostBlur * 0.6f; gp.dryLevel = 0.25f + (1.0f - p.ghostBlur) * 0.4f; gp.width = 1.0f;
        ghostRev.setParameters (gp);
        ghostRev.processStereo (gL, gR, n);
        for (int i = 0; i < n; ++i) { L[i] += gL[i] * p.ghost; R[i] += gR[i] * p.ghost; }
    }

    static float softLimit (float x)
    {
        const float a = std::abs (x);
        if (a < 0.85f) return x;
        return std::copysign (0.85f + 0.14f * std::tanh ((a - 0.85f) / 0.14f), x);
    }

    static float shape (float x, float amt, int type)
    {
        if (amt < 0.0005f) return x;
        const float g = type == 3 ? 1.0f + amt * amt * 150.0f : 1.0f + amt * amt * 40.0f;
        const float makeup = 1.0f / (1.0f + 0.55f * std::log2 (g));
        float y;
        switch (type)
        {
            case 1:  y = std::tanh (g * x + 0.2f) - 0.197375f; break;                          // tape (asym)
            case 2:  y = std::clamp (g * x, -1.0f, 1.0f); break;                                // hard clip
            case 3:  { float h = std::clamp (g * x * 1.3f, -1.0f, 1.0f);                       // blown out
                       h = 0.75f * h + 0.25f * std::copysign (1.0f - std::exp (-std::abs (g * x)), x);
                       y = std::round (h * 10.0f) / 10.0f * 0.35f + h * 0.65f; break; }
            case 4:  y = std::sin (g * x * 1.2f); break;                                        // foldback
            default: y = std::tanh (g * x); break;                                              // soft
        }
        const float wet = std::min (1.0f, amt * 25.0f);
        return x + (y * makeup * 1.6f - x) * wet;
    }

    float sr = 44100, sm = 0.01f;
    juce::dsp::Oversampling<float> os { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
    std::vector<float> low[2];
    OnePole splitA[2], splitB[2], sideLp[2], ghostHp, tapeLp[2];
    DcBlock dc[2];
    SvfState bodyS[2][5];
    DelayLine wowDl[2], chDl[2], flDl[2], dly[2], ring[2], revRing[2], htRing[2], ghostRing, ghostDly;
    float envFast = 0, envSlow = 0, smPunch = 0, smHalf = 0, atkFast = 0.5f, relFast = 0.01f, atkSlow = 0.01f, relSlow = 0.001f;
    juce::Reverb ghostRev;
    FdnReverb fdn;
    MasterStage mastering;
    Biquad eq[4], subsonic[2];
    float lastEqLow = 999, lastEqHigh = 999;
    float apState[2][4] {}, apFb[2] {};
    Rng crackle;
    float noiseEnv = 0, noiseRel = 0.9999f;
    static constexpr int maxBlock = 4096;
    float gL[maxBlock] {}, gR[maxBlock] {};
    float smDrive = 0, smChorus = 0, smDelay = 0, smBody = 0, smPhaser = 0, smFlanger = 0, smReverse = 0, smDelaySamples = -1;
    float wowPhase = 0, flutPhase = 0, chPhase = 0, phPhase = 0, flPhase = 0, grainPhase = 0, revGrain = 0;
    int holdCount = 0; float holdL = 0, holdR = 0;
    int64_t lastStep = -1; int evType = 0; float evSlice = 1, evT = 0;
};
} // namespace kkv2
