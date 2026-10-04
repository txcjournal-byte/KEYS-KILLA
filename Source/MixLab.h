#pragma once
#include "DspUtil.h"
#include <array>
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>

// v0.41 MIX LAB: the last stage of EVOLVE's output (and of any audio you put through it as an insert).
//   SHAPE EQ      8 bands (bell / shelves / cuts / notch), each can be DYNAMIC (it only cuts when that range gets loud),
//                 a live analyzer, AUTO (finds resonances + balances the tone) and EVOLVE (curves that mutate, you pick by ear)
//   PUNCH COMP    a compressor with five characters (CLEAN, PUNCH, GLUE, OPTO, VINTAGE), soft knee, parallel mix,
//                 sidechain low cut (the bass does not pump it), auto gain
//   TIME MACHINE  colour from a century of machines: NOISE, WOBBLE, DISTORT, DIGITAL, SPACE, FADE + the ERA timeline
// The UI writes atomics, the audio thread reads them; nothing allocates in process().
namespace kk
{
enum EqType { eqBell, eqLowShelf, eqHighShelf, eqLowCut, eqHighCut, eqNotch, numEqTypes };
inline const char* eqTypeName (int t)
{
    static const char* n[] { "BELL", "LOW SHELF", "HIGH SHELF", "LOW CUT", "HIGH CUT", "NOTCH" };
    return n[std::clamp (t, 0, (int) numEqTypes - 1)];
}
enum CompStyle { csClean, csPunch, csGlue, csOpto, csVintage, numCompStyles };
inline const char* compStyleName (int s)
{
    static const char* n[] { "CLEAN", "PUNCH", "GLUE", "OPTO", "VINTAGE" };
    return n[std::clamp (s, 0, (int) numCompStyles - 1)];
}
enum TmModule { tmNoise, tmWobble, tmDistort, tmDigital, tmSpace, tmFade, numTm };
inline const char* tmName (int m)
{
    static const char* n[] { "NOISE", "WOBBLE", "DISTORT", "DIGITAL", "SPACE", "FADE" };
    return n[std::clamp (m, 0, (int) numTm - 1)];
}

enum SpaceMode { spRoom, spPlate, spHall, spCloud, spShimmer, numSpaceModes };
inline const char* spaceModeName (int m) { static const char* n[] { "ROOM", "PLATE", "HALL", "CLOUD", "SHIMMER" }; return n[std::clamp (m, 0, (int) numSpaceModes - 1)]; }
enum EchoMode { dlDigital, dlTape, dlPingPong, dlLofi, dlGhost, dlGrain, numEchoModes };
inline const char* echoModeName (int m) { static const char* n[] { "CLEAN", "TAPE", "PING-PONG", "LO-FI", "GHOST", "GRAIN" }; return n[std::clamp (m, 0, (int) numEchoModes - 1)]; }
inline double echoBeats (int t) { static const double b[] { 1.0, 0.5, 0.75, 0.25, 1.0 / 3.0, 2.0 }; return b[std::clamp (t, 0, 5)]; }
inline const char* echoTimeName (int t) { static const char* n[] { "1/4", "1/8", "1/8 D", "1/16", "1/4 T", "1/2" }; return n[std::clamp (t, 0, 5)]; }

struct Coefs { float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
struct BiState
{
    float z1 = 0, z2 = 0;
    inline float tick (const Coefs& c, float x) { const float y = c.b0 * x + z1; z1 = c.b1 * x - c.a1 * y + z2; z2 = c.b2 * x - c.a2 * y; return y; }
    void reset() { z1 = z2 = 0; }
};

// RBJ cookbook
inline Coefs eqCoefs (int type, float f, float gainDb, float q, float sr)
{
    f = std::clamp (f, 16.0f, sr * 0.46f); q = std::clamp (q, 0.1f, 30.0f);
    const float A = std::pow (10.0f, gainDb / 40.0f), w = twoPi * f / sr, cs = std::cos (w), sn = std::sin (w);
    const float al = sn / (2.0f * q);
    float b0 = 1, b1 = 0, b2 = 0, a0 = 1, a1 = 0, a2 = 0;
    switch (type)
    {
        case eqLowShelf:
        case eqHighShelf:
        {
            const float s = 2.0f * std::sqrt (A) * al;
            if (type == eqLowShelf) { b0 = A * ((A + 1) - (A - 1) * cs + s); b1 = 2 * A * ((A - 1) - (A + 1) * cs); b2 = A * ((A + 1) - (A - 1) * cs - s);
                                      a0 = (A + 1) + (A - 1) * cs + s; a1 = -2 * ((A - 1) + (A + 1) * cs); a2 = (A + 1) + (A - 1) * cs - s; }
            else                    { b0 = A * ((A + 1) + (A - 1) * cs + s); b1 = -2 * A * ((A - 1) + (A + 1) * cs); b2 = A * ((A + 1) + (A - 1) * cs - s);
                                      a0 = (A + 1) - (A - 1) * cs + s; a1 = 2 * ((A - 1) - (A + 1) * cs); a2 = (A + 1) - (A - 1) * cs - s; }
            break;
        }
        case eqLowCut:  b0 = (1 + cs) / 2; b1 = -(1 + cs); b2 = (1 + cs) / 2; a0 = 1 + al; a1 = -2 * cs; a2 = 1 - al; break;
        case eqHighCut: b0 = (1 - cs) / 2; b1 = 1 - cs;    b2 = (1 - cs) / 2; a0 = 1 + al; a1 = -2 * cs; a2 = 1 - al; break;
        case eqNotch:   b0 = 1; b1 = -2 * cs; b2 = 1; a0 = 1 + al; a1 = -2 * cs; a2 = 1 - al; break;
        default:        b0 = 1 + al * A; b1 = -2 * cs; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cs; a2 = 1 - al / A; break;
    }
    return { b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0 };
}
// magnitude of one band at frequency hz (dB) - for drawing the curve
inline float eqMagDb (const Coefs& c, float hz, float sr)
{
    const float w = twoPi * hz / sr;
    const float cr1 = std::cos (w), ci1 = -std::sin (w), cr2 = std::cos (2 * w), ci2 = -std::sin (2 * w);
    const float nr = c.b0 + c.b1 * cr1 + c.b2 * cr2, ni = c.b1 * ci1 + c.b2 * ci2;
    const float dr = 1 + c.a1 * cr1 + c.a2 * cr2, di = c.a1 * ci1 + c.a2 * ci2;
    return 10.0f * std::log10 ((nr * nr + ni * ni) / std::max (1e-12f, dr * dr + di * di) + 1e-12f);
}

struct MixLabState
{
    static constexpr int numBands = 8;
    struct Band
    {
        std::atomic<bool> on { false };
        std::atomic<int> type { eqBell }, slope { 1 };       // slope: cuts 12 (1) or 24 (2) dB/oct
        std::atomic<float> freq { 1000 }, gain { 0 }, q { 0.9f }, dyn { 0 };   // dyn 0..1: cuts up to 12 dB more when the band is loud
    };
    std::atomic<bool> eqOn { true };
    std::array<Band, numBands> band;
    std::atomic<float> eqOut { 0 };                           // dB
    // comp
    std::atomic<bool> compOn { false }, autoGain { true }, scHp { true };
    std::atomic<int> compStyle { csGlue };
    std::atomic<float> thresh { -18 }, ratio { 3 }, attack { 15 }, release { 120 }, knee { 6 }, compMix { 1 }, compOut { 0 };
    // time machine
    std::atomic<bool> tmOn { false };
    std::array<std::atomic<bool>, numTm> tmModOn {};
    std::array<std::atomic<float>, numTm> tmAmt {};
    std::atomic<int> noiseType { 0 };                        // 0 vinyl, 1 tape hiss, 2 hum
    std::atomic<float> wobbleRate { 0.5f }, distTone { 0.5f }, digRate { 0.5f }, spaceSize { 0.5f }, fadeRate { 0.4f };
    std::atomic<float> magnitude { 1.0f }, era { 0.5f }, tmLp { 20000 }, tmHp { 20 }, tmMono { 0 }, tmMix { 1 };
    // space + echo
    std::atomic<bool> spOn { false }, spFreeze { false }, dlOn { false };
    std::atomic<int> spMode { spPlate }, dlMode { dlTape }, dlTime { 2 };
    std::atomic<float> spMix { 0.25f }, spDecay { 0.5f }, spPre { 20.0f }, spTone { 0.6f }, spMod { 0.4f }, spWidth { 1.0f }, spDuck { 0.0f };
    std::atomic<float> dlMix { 0.25f }, dlFb { 0.4f }, dlTone { 0.6f }, dlDuck { 0.0f };
    std::atomic<float> bpm { 120.0f };
    // meters (audio -> UI)
    std::atomic<int> inputBlocks { 0 };                      // blocks of audio coming IN (EVOLVE as an effect on a track / the master)
    std::atomic<float> mIn { 0 }, mOut { 0 }, mGr { 0 }, mRms { 0 }, mPeak { 0 }, mCorr { 1 }, mWidth { 0 }, mCrest { 12 };
    std::array<std::atomic<float>, numBands> mDyn {};        // dynamic cut now (dB, <= 0)
    // analyzer rings (mono): what goes in (pre) and what comes out (post)
    static constexpr int ringSize = 8192;
    std::array<float, ringSize> preRing {}, postRing {}, ringL {}, ringR {};
    std::atomic<int> ringW { 0 };

    MixLabState()
    {
        static const float f[numBands] { 30, 90, 250, 700, 1800, 4000, 9000, 18000 };
        static const int t[numBands] { eqLowCut, eqLowShelf, eqBell, eqBell, eqBell, eqBell, eqHighShelf, eqHighCut };
        for (int i = 0; i < numBands; ++i) { band[(size_t) i].freq = f[i]; band[(size_t) i].type = t[i]; band[(size_t) i].q = t[i] == eqBell ? 0.9f : 0.71f; }
        for (auto& a : tmAmt) a = 0.4f;
        for (auto& o : tmModOn) o = false;
        for (auto& d : mDyn) d = 0;
    }
    void resetEq()
    {
        MixLabState fresh;
        for (int i = 0; i < numBands; ++i)
        {
            auto& b = band[(size_t) i]; auto& s = fresh.band[(size_t) i];
            b.on = false; b.type = s.type.load(); b.freq = s.freq.load(); b.gain = 0; b.q = s.q.load(); b.dyn = 0; b.slope = 1;
        }
        eqOut = 0;
    }
};

class MixLabDsp
{
public:
    void prepare (double rate, int)
    {
        sr = (float) rate;
        for (auto& b : bands) { for (auto& st : b.s) for (auto& x : st) x.reset(); b.det.reset(); b.env = 0; b.cur = {}; b.valid = false; }
        grSm = 0; envC = 0; rmsC = 0; optoSlow = 0;
        scC.set (110.0f, 1.4f, sr); for (auto& s : scS) s.reset();
        wob.prepare ((int) (0.06f * sr) + 8); wob2.prepare ((int) (0.06f * sr) + 8);
        for (auto& d : fdn) d.prepare ((int) (0.25f * sr) + 8);
        for (auto& z : fdnLp) z = 0;
        noiseEnv = 0; holdL = holdR = 0; holdCnt = 0; fadeVal = 1; fadeTarget = 1; fadeLp[0] = fadeLp[1] = 0; wowPh = flutPh = humPh = 0;
        for (auto& s : tmLpS) s.reset(); for (auto& s : tmHpS) s.reset();
        crackRng.seed (91); rng.seed (1234);
        tsLp[0].reset(); tsLp[1].reset();
        lpHzSm = 20000; hpHzSm = 20;
        distLp[0] = distLp[1] = 0;
        hissLp = hissHp = rumble = 0;
        prepareSpace();
        rmsSm = 0; peakHold = 0; corrNum = 0; corrL = corrR = 1e-9f; sideSm = 0; midSm = 1e-9f;
    }

    void process (float* L, float* R, int n, MixLabState& st)
    {
        if (n <= 0) return;
        // analyzer: pre
        int w = st.ringW.load (std::memory_order_relaxed);
        const int w0 = w;
        for (int i = 0; i < n; ++i) { st.preRing[(size_t) ((w0 + i) & (MixLabState::ringSize - 1))] = 0.5f * (L[i] + R[i]); }
        float inPk = 0; for (int i = 0; i < n; ++i) inPk = std::max (inPk, std::max (std::abs (L[i]), std::abs (R[i])));
        st.mIn = std::max (inPk, st.mIn.load() * 0.9f);

        if (st.eqOn.load()) eq (L, R, n, st);
        if (st.compOn.load()) comp (L, R, n, st); else { grSm = 0; st.mGr = 0; }
        if (st.tmOn.load()) timeMachine (L, R, n, st);
        if (st.dlOn.load() || dlTail > 0) echo (L, R, n, st);
        if (st.spOn.load() || spTail > 0) space (L, R, n, st);

        // analyzer: post + meters for the COACH
        float pk = 0, ss = 0;
        for (int i = 0; i < n; ++i)
        {
            const size_t idx = (size_t) ((w0 + i) & (MixLabState::ringSize - 1));
            st.postRing[idx] = 0.5f * (L[i] + R[i]); st.ringL[idx] = L[i]; st.ringR[idx] = R[i];
            const float a = std::max (std::abs (L[i]), std::abs (R[i]));
            pk = std::max (pk, a); ss += 0.5f * (L[i] * L[i] + R[i] * R[i]);
            corrNum = corrNum * 0.9995f + L[i] * R[i]; corrL = corrL * 0.9995f + L[i] * L[i]; corrR = corrR * 0.9995f + R[i] * R[i];
            const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]);
            midSm = midSm * 0.9995f + m * m; sideSm = sideSm * 0.9995f + s * s;
        }
        w = (w0 + n) & (MixLabState::ringSize - 1);
        st.ringW.store (w, std::memory_order_release);
        const float blkRms = std::sqrt (ss / (float) n);
        const float a = std::exp (-(float) n / (0.4f * sr));
        rmsSm = a * rmsSm + (1 - a) * blkRms * blkRms;
        peakHold = std::max (pk, peakHold * std::exp (-(float) n / (1.5f * sr)));
        st.mOut = std::max (pk, st.mOut.load() * 0.9f);
        st.mRms = std::sqrt (rmsSm); st.mPeak = peakHold;
        if (rmsSm > 1e-8f) st.mCrest = 20.0f * std::log10 (std::max (1e-6f, peakHold) / std::sqrt (rmsSm));
        st.mCorr = corrL > 1e-7f && corrR > 1e-7f ? corrNum / std::sqrt (corrL * corrR) : 1.0f;
        st.mWidth = midSm > 1e-9f ? std::sqrt (sideSm / (midSm + sideSm)) : 0.0f;
    }

private:
    struct BandDsp
    {
        std::array<std::array<BiState, 2>, 2> s;   // [stage][channel]
        Coefs cur; BiState det; Coefs detC; float env = 0; bool valid = false;
        float f = -1, g = 99, q = -1; int t = -1; float lastDyn = 0;
    };
    std::array<BandDsp, MixLabState::numBands> bands;
    float sr = 44100;

    void eq (float* L, float* R, int n, MixLabState& st)
    {
        constexpr int sub = 32;
        const float envA = std::exp (-1.0f / (0.004f * sr)), envR = std::exp (-1.0f / (0.12f * sr));
        for (int b = 0; b < MixLabState::numBands; ++b)
        {
            auto& bs = st.band[(size_t) b]; auto& d = bands[(size_t) b];
            if (! bs.on.load()) { if (d.valid) { for (auto& stg : d.s) for (auto& x : stg) x.reset(); d.valid = false; } st.mDyn[(size_t) b] = 0; continue; }
            const int type = bs.type.load(); const int stages = (type == eqLowCut || type == eqHighCut) ? std::clamp (bs.slope.load(), 1, 2) : 1;
            const float tf = bs.freq.load(), tg = bs.gain.load(), tq = bs.q.load(), dyn = bs.dyn.load();
            if (! d.valid) { d.f = tf; d.g = tg; d.q = tq; d.t = type; d.valid = true; d.env = 0; d.det.reset(); }
            if (dyn > 0.001f) d.detC = eqCoefs (eqBell, d.f, 18.0f, std::max (0.5f, d.q), sr);   // the band's range, loud
            for (int i0 = 0; i0 < n; i0 += sub)
            {
                const int m = std::min (sub, n - i0);
                // smooth toward the targets (no zipper when you drag a node)
                const float k = 1.0f - std::exp (-(float) m / (0.03f * sr));
                d.f *= std::pow (tf / std::max (1.0f, d.f), k); d.g += (tg - d.g) * k; d.q += (tq - d.q) * k; d.t = type;
                float extra = 0;
                if (dyn > 0.001f)
                {
                    for (int i = i0; i < i0 + m; ++i)
                    {
                        const float x = std::abs (d.det.tick (d.detC, 0.5f * (L[i] + R[i]))) * 0.125f;   // remove the +18 dB of the detector
                        d.env = x > d.env ? envA * d.env + (1 - envA) * x : envR * d.env + (1 - envR) * x;
                    }
                    const float lvl = 20.0f * std::log10 (d.env + 1e-9f);
                    extra = -12.0f * dyn * std::clamp ((lvl + 42.0f) / 24.0f, 0.0f, 1.0f);
                }
                d.lastDyn = extra;
                d.cur = eqCoefs (d.t, d.f, d.g + extra, d.q, sr);
                for (int stg = 0; stg < stages; ++stg)
                    for (int i = i0; i < i0 + m; ++i) { L[i] = d.s[(size_t) stg][0].tick (d.cur, L[i]); R[i] = d.s[(size_t) stg][1].tick (d.cur, R[i]); }
            }
            st.mDyn[(size_t) b] = d.lastDyn;
        }
        const float og = std::pow (10.0f, st.eqOut.load() / 20.0f);
        if (std::abs (og - 1.0f) > 1e-4f) for (int i = 0; i < n; ++i) { L[i] *= og; R[i] *= og; }
    }

    // ---------------- PUNCH COMP ----------------
    float grSm = 0, envC = 0, rmsC = 0, optoSlow = 0;
    SvfCoef scC; std::array<SvfState, 2> scS;
    void comp (float* L, float* R, int n, MixLabState& st)
    {
        const int style = st.compStyle.load();
        const float T = st.thresh.load(), Rt = std::max (1.0f, st.ratio.load()), W = std::max (0.0f, st.knee.load());
        float atkMs = std::max (0.05f, st.attack.load()), relMs = std::max (5.0f, st.release.load());
        if (style == csPunch) atkMs = std::max (atkMs, 8.0f);          // lets the hit through, then grabs
        if (style == csVintage) atkMs = std::min (atkMs, 2.0f);        // fast and colourful
        const float aA = std::exp (-1.0f / (atkMs * 0.001f * sr)), aR = std::exp (-1.0f / (relMs * 0.001f * sr));
        const float aRslow = std::exp (-1.0f / (relMs * 0.004f * sr));
        const float rmsA = std::exp (-1.0f / (0.012f * sr));
        const bool rms = style == csGlue || style == csOpto, hp = st.scHp.load();
        const float mix = std::clamp (st.compMix.load(), 0.0f, 1.0f);
        float makeup = st.compOut.load();
        if (st.autoGain.load()) makeup += -std::min (0.0f, T) * (1.0f - 1.0f / Rt) * 0.45f;
        const float mk = std::pow (10.0f, makeup / 20.0f);
        float grMax = 0;
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            if (hp) { scS[0].tick (scC, l); scS[1].tick (scC, r); l = scS[0].hp; r = scS[1].hp; }
            float x = std::max (std::abs (l), std::abs (r));
            if (rms) { rmsC = rmsA * rmsC + (1 - rmsA) * x * x; x = std::sqrt (rmsC) * 1.41f; }
            const float lvl = 20.0f * std::log10 (x + 1e-9f);
            float gc;
            const float over = lvl - T;
            if (2 * over < -W) gc = lvl;
            else if (W > 0 && 2 * std::abs (over) <= W) gc = lvl + (1.0f / Rt - 1.0f) * (over + W / 2) * (over + W / 2) / (2 * W);
            else gc = T + over / Rt;
            const float gr = gc - lvl;   // <= 0
            if (gr < grSm) grSm = aA * grSm + (1 - aA) * gr;
            else if (style == csOpto)    // two-stage release: quick at first, then it lets go slowly (like a light cell)
            {
                optoSlow = std::min (optoSlow, grSm);
                optoSlow = aRslow * optoSlow + (1 - aRslow) * gr;
                grSm = aR * grSm + (1 - aR) * gr; grSm = std::min (grSm, optoSlow * 0.5f);
            }
            else grSm = aR * grSm + (1 - aR) * gr;
            grMax = std::min (grMax, grSm);
            const float g = std::pow (10.0f, grSm / 20.0f) * mk;
            float wl = L[i] * g, wr = R[i] * g;
            if (style == csVintage) { wl = fastTanh (wl * 1.4f) / 1.25f; wr = fastTanh (wr * 1.4f) / 1.25f; }
            else if (style == csPunch) { wl = wl - 0.08f * wl * wl * wl; wr = wr - 0.08f * wr * wr * wr; }
            L[i] = L[i] * (1 - mix) + wl * mix; R[i] = R[i] * (1 - mix) + wr * mix;
        }
        st.mGr = grMax;
    }

    // ---------------- SPACE: an 8-line feedback delay network with diffusion, modulation, shimmer, freeze, ducking ----------------
    static constexpr int nLines = 8;
    std::array<DelayLine, nLines> fdnL; std::array<float, nLines> fdnDamp {}; std::array<float, nLines> modPh {};
    std::array<DelayLine, 4> diff; DelayLine preL, preR;
    DelayLine shimBuf; float shimPh = 0;
    float duckEnv = 0, spSmMix = 0; int spTail = 0;
    struct Hilbert   // two allpass chains 90 degrees apart (frequency shifting)
    {
        float xa[4][2] {}, ya[4][2] {}, xb[4][2] {}, yb[4][2] {}, bDelay = 0;
        static float sec (float c, float x, float (&xm)[2], float (&ym)[2]) { const float y = c * (x + ym[1]) - xm[1]; xm[1] = xm[0]; xm[0] = x; ym[1] = ym[0]; ym[0] = y; return y; }
        void tick (float x, float& re, float& im)
        {
            static const float A[4] { 0.6923878f * 0.6923878f, 0.9360654f * 0.9360654f, 0.9882295f * 0.9882295f, 0.9987488f * 0.9987488f };
            static const float B[4] { 0.4021921f * 0.4021921f, 0.8561711f * 0.8561711f, 0.9722910f * 0.9722910f, 0.9952885f * 0.9952885f };
            float a = x, b = x;
            for (int k = 0; k < 4; ++k) { a = sec (A[k], a, xa[k], ya[k]); b = sec (B[k], b, xb[k], yb[k]); }
            re = a; im = bDelay; bDelay = b;
        }
    };
    Hilbert hil[2]; float ghostPh = 0;
    struct Grain { float pos = 0, len = 1, age = 1, rate = 1; bool on = false; };
    std::array<Grain, 6> grains {}; int grainTimer = 0;
    DelayLine echoL, echoR; float echoLp[2] {}, echoHp[2] {}, echoWow = 0, echoSmT = -1, echoHold[2] {}; int echoHoldN = 0, dlTail = 0; float dlDuckEnv = 0;
    void prepareSpace()
    {
        for (auto& d : fdnL) d.prepare ((int) (0.35f * sr) + 16);
        for (auto& d : diff) d.prepare ((int) (0.03f * sr) + 16);
        preL.prepare ((int) (0.25f * sr) + 16); preR.prepare ((int) (0.25f * sr) + 16);
        shimBuf.prepare ((int) (0.2f * sr) + 16);
        echoL.prepare ((int) (4.2f * sr) + 16); echoR.prepare ((int) (4.2f * sr) + 16);
        for (auto& z : fdnDamp) z = 0;
        for (int i = 0; i < nLines; ++i) modPh[(size_t) i] = (float) i / nLines;
        duckEnv = 0; spSmMix = 0; spTail = 0; dlTail = 0; echoSmT = -1; dlDuckEnv = 0;
    }
    void space (float* L, float* R, int n, MixLabState& st)
    {
        const bool on = st.spOn.load();
        if (on) spTail = (int) (sr * 30.0f); else spTail -= n;
        const int mode = st.spMode.load();
        const float decay = std::clamp (st.spDecay.load(), 0.0f, 1.0f);
        // base sizes (s) and RT60 per mode
        float size = 0.6f, rt = 1.2f, diffAmt = 0.6f, modDepth = 0.0004f, hiCut = 9000.0f;
        switch (mode)
        {
            case spRoom:    size = 0.35f; rt = 0.3f + 1.5f * decay; diffAmt = 0.55f; hiCut = 7000; break;
            case spPlate:   size = 0.55f; rt = 0.6f + 4.0f * decay; diffAmt = 0.75f; hiCut = 12000; break;
            case spHall:    size = 0.9f;  rt = 1.0f + 6.0f * decay; diffAmt = 0.7f; hiCut = 8000; break;
            case spCloud:   size = 1.0f;  rt = 3.0f + 27.0f * decay; diffAmt = 0.8f; modDepth = 0.0015f; hiCut = 6500; break;
            default:        size = 0.9f;  rt = 2.0f + 12.0f * decay; diffAmt = 0.75f; modDepth = 0.001f; hiCut = 10000; break;
        }
        hiCut *= 0.35f + 0.9f * std::clamp (st.spTone.load(), 0.0f, 1.0f);
        modDepth *= 0.3f + 1.4f * std::clamp (st.spMod.load(), 0.0f, 1.0f);
        const bool freeze = st.spFreeze.load();
        static const float lens[nLines] { 0.0371f, 0.0437f, 0.0491f, 0.0557f, 0.0613f, 0.0683f, 0.0743f, 0.0811f };
        float g[nLines], len[nLines];
        for (int i = 0; i < nLines; ++i) { len[i] = lens[i] * size * 2.6f * sr; g[i] = freeze ? 1.0f : std::pow (10.0f, -3.0f * (len[i] / sr) / std::max (0.1f, rt)); }
        const float dampA = std::exp (-twoPi * hiCut / sr);
        const float pre = std::clamp (st.spPre.load(), 0.0f, 240.0f) * 0.001f * sr;
        const float mixT = on ? std::clamp (st.spMix.load(), 0.0f, 1.0f) : 0.0f, width = std::clamp (st.spWidth.load(), 0.0f, 1.0f);
        const float duck = std::clamp (st.spDuck.load(), 0.0f, 1.0f);
        const float dA = std::exp (-1.0f / (0.01f * sr)), dR = std::exp (-1.0f / (0.35f * sr));
        static const float dl[4] { 0.0047f, 0.0083f, 0.0123f, 0.0171f };
        for (int i = 0; i < n; ++i)
        {
            spSmMix += (mixT - spSmMix) * 0.0005f;
            const float inL = L[i], inR = R[i];
            preL.push (inL); preR.push (inR);
            float x = freeze ? 0.0f : 0.5f * (preL.read (std::max (1.0f, pre)) + preR.read (std::max (1.0f, pre)));
            for (int k = 0; k < 4; ++k)   // input diffusion (allpasses)
            {
                const float d = diff[(size_t) k].read (dl[k] * sr * (0.6f + 0.6f * size));
                const float v = x + diffAmt * d;
                diff[(size_t) k].push (v);
                x = d - diffAmt * v;
            }
            float o[nLines];
            for (int k = 0; k < nLines; ++k)
            {
                modPh[(size_t) k] += (0.13f + 0.07f * (float) k) / sr; if (modPh[(size_t) k] >= 1) modPh[(size_t) k] -= 1;
                o[k] = fdnL[(size_t) k].read (len[k] + modDepth * sr * std::sin (twoPi * modPh[(size_t) k]));
            }
            // Hadamard 8x8 (fast) mixes the lines
            float h[nLines]; for (int k = 0; k < nLines; ++k) h[k] = o[k];
            for (int span = 1; span < nLines; span <<= 1)
                for (int a = 0; a < nLines; a += span * 2)
                    for (int b = a; b < a + span; ++b) { const float u = h[b], v = h[b + span]; h[b] = u + v; h[b + span] = u - v; }
            float shim = 0;
            if (mode == spShimmer)   // an octave up in the loop: two crossfaded grains reading twice as fast
            {
                shimBuf.push (o[0] + o[3]);
                shimPh += 1.0f / (0.08f * sr); if (shimPh >= 1) shimPh -= 1;
                const float win = 0.08f * sr;
                const float a1 = shimPh, a2 = std::fmod (shimPh + 0.5f, 1.0f);
                const float r1 = shimBuf.read (1.0f + (1.0f - a1) * win), r2 = shimBuf.read (1.0f + (1.0f - a2) * win);
                shim = (r1 * std::sin (pi * a1) + r2 * std::sin (pi * a2)) * 0.5f;
            }
            for (int k = 0; k < nLines; ++k)
            {
                float v = h[k] * 0.35355f * g[k] + (k % 2 == 0 ? x : -x) * 0.5f + (k < 2 ? shim * 0.35f * g[k] : 0.0f);
                if (! freeze) { fdnDamp[(size_t) k] = v + dampA * (fdnDamp[(size_t) k] - v); v = fdnDamp[(size_t) k]; }
                fdnL[(size_t) k].push (std::clamp (v, -4.0f, 4.0f));
            }
            float wl = (o[0] + o[2] + o[4] + o[6]) * 0.5f, wr = (o[1] + o[3] + o[5] + o[7]) * 0.5f;
            const float m = 0.5f * (wl + wr), sd = 0.5f * (wl - wr) * width;
            wl = m + sd; wr = m - sd;
            const float lvl = std::max (std::abs (inL), std::abs (inR));
            duckEnv = lvl > duckEnv ? dA * duckEnv + (1 - dA) * lvl : dR * duckEnv + (1 - dR) * lvl;
            const float dg = 1.0f - duck * std::min (1.0f, duckEnv * 4.0f);
            L[i] = inL + wl * spSmMix * dg; R[i] = inR + wr * spSmMix * dg;
        }
    }
    // ---------------- ECHO: tempo delay - clean, tape (wobble + warmth), ping-pong, lo-fi ----------------
    void echo (float* L, float* R, int n, MixLabState& st)
    {
        const bool on = st.dlOn.load();
        if (on) dlTail = (int) (sr * 12.0f); else dlTail -= n;
        const int mode = st.dlMode.load();
        const float bpmv = std::clamp (st.bpm.load(), 40.0f, 240.0f);
        const float target = std::min (4.0f * sr, (float) (echoBeats (st.dlTime.load()) * 60.0 / bpmv) * sr);
        if (echoSmT < 0) echoSmT = target;
        const float fb = std::clamp (st.dlFb.load(), 0.0f, 0.95f), mix = on ? std::clamp (st.dlMix.load(), 0.0f, 1.0f) : 0.0f;
        const float lpA = std::exp (-twoPi * (1500.0f + 12000.0f * std::clamp (st.dlTone.load(), 0.0f, 1.0f)) / sr), hpA = std::exp (-twoPi * 120.0f / sr);
        const float duck = std::clamp (st.dlDuck.load(), 0.0f, 1.0f);
        const float dA = std::exp (-1.0f / (0.01f * sr)), dR = std::exp (-1.0f / (0.3f * sr));
        for (int i = 0; i < n; ++i)
        {
            echoSmT += (target - echoSmT) * 0.0002f;   // time changes glide (tape-like), no clicks
            float t = echoSmT;
            if (mode == dlTape) { echoWow += 0.7f / sr; if (echoWow >= 1) echoWow -= 1; t += 0.0012f * sr * std::sin (twoPi * echoWow); }
            float yl = echoL.read (std::max (2.0f, t)), yr = echoR.read (std::max (2.0f, t));
            auto tone = [&] (float v, int c)
            {
                echoLp[c] = v + lpA * (echoLp[c] - v);
                echoHp[c] = echoLp[c] + hpA * (echoHp[c] - echoLp[c]);
                float o = echoLp[c] - echoHp[c] * (mode == dlLofi ? 1.0f : 0.0f) - (echoHp[c] * 0.15f);
                if (mode == dlTape) o = fastTanh (o * 1.3f) / 1.15f;
                return o;
            };
            float fl = tone (yl, 0), fr = tone (yr, 1);
            if (mode == dlLofi)
            {
                if (--echoHoldN <= 0) { echoHoldN = 4; echoHold[0] = std::round (fl * 64.0f) / 64.0f; echoHold[1] = std::round (fr * 64.0f) / 64.0f; }
                fl = echoHold[0]; fr = echoHold[1];
            }
            if (mode == dlGhost)   // each echo shifted a few Hz up: it spirals, ghostly
            {
                ghostPh += 6.0f / sr; if (ghostPh >= 1) ghostPh -= 1;
                const float c = std::cos (twoPi * ghostPh), sn = std::sin (twoPi * ghostPh);
                float re, im; hil[0].tick (fl, re, im); fl = re * c - im * sn;
                hil[1].tick (fr, re, im); fr = re * c - im * sn;
            }
            if (mode == dlGrain)   // a cloud of short grains from the last second, some an octave up / down
            {
                if (--grainTimer <= 0)
                {
                    grainTimer = (int) (sr * (0.025f + 0.05f * rng.uni()));
                    for (auto& gr : grains) if (! gr.on)
                    {
                        gr.on = true; gr.age = 0; gr.len = sr * (0.06f + 0.14f * rng.uni());
                        gr.pos = std::max (gr.len * 2.0f + 4.0f, t * (0.3f + 1.2f * rng.uni()));
                        const float x = rng.uni(); gr.rate = x < 0.15f ? 2.0f : x < 0.25f ? 0.5f : 1.0f;
                        break;
                    }
                }
                float gl = 0, gr2 = 0;
                for (auto& gr : grains)
                {
                    if (! gr.on) continue;
                    const float w = std::sin (pi * gr.age / gr.len);
                    const float d = std::max (2.0f, gr.pos - gr.age * (gr.rate - 1.0f));
                    gl += echoL.read (std::min (d, 4.0f * sr)) * w; gr2 += echoR.read (std::min (d, 4.0f * sr)) * w;
                    gr.age += 1.0f; if (gr.age >= gr.len) gr.on = false;
                }
                yl = gl * 0.6f; yr = gr2 * 0.6f; fl = yl; fr = yr;
            }
            const float inL = L[i], inR = R[i];
            if (mode == dlPingPong) { echoL.push (0.5f * (inL + inR) + fr * fb); echoR.push (fl * fb); }
            else { echoL.push (inL + fl * fb); echoR.push (inR + fr * fb); }
            const float lvl = std::max (std::abs (inL), std::abs (inR));
            dlDuckEnv = lvl > dlDuckEnv ? dA * dlDuckEnv + (1 - dA) * lvl : dR * dlDuckEnv + (1 - dR) * lvl;
            const float dg = 1.0f - duck * std::min (1.0f, dlDuckEnv * 4.0f);
            L[i] = inL + yl * mix * dg; R[i] = inR + yr * mix * dg;
        }
    }

    // ---------------- TIME MACHINE ----------------
    DelayLine wob, wob2;
    std::array<DelayLine, 4> fdn; std::array<float, 4> fdnLp {};
    float noiseEnv = 0, holdL = 0, holdR = 0; int holdCnt = 0;
    float fadeVal = 1, fadeTarget = 1, fadeLp[2] {}; float wowPh = 0, flutPh = 0, humPh = 0;
    std::array<SvfState, 2> tmLpS, tmHpS; std::array<SvfState, 2> tsLp; float lpHzSm = 20000, hpHzSm = 20;
    float distLp[2] {}; float hissLp = 0, hissHp = 0, rumble = 0;
    Rng crackRng, rng;
    float rmsSm = 0, peakHold = 0, corrNum = 0, corrL = 1e-9f, corrR = 1e-9f, sideSm = 0, midSm = 1e-9f;
    float wowDrift = 0, wowDriftT = 0;

    void timeMachine (float* L, float* R, int n, MixLabState& st)
    {
        const float mag = std::clamp (st.magnitude.load(), 0.0f, 1.5f);
        auto amt = [&] (int m) { return st.tmModOn[(size_t) m].load() ? std::clamp (st.tmAmt[(size_t) m].load() * mag, 0.0f, 1.5f) : 0.0f; };
        const float aNoise = amt (tmNoise), aWob = amt (tmWobble), aDist = amt (tmDistort), aDig = amt (tmDigital), aSpace = amt (tmSpace), aFade = amt (tmFade);
        const float mix = std::clamp (st.tmMix.load(), 0.0f, 1.0f);
        // the era's bandwidth (old machines lose lows and highs) + mono
        const float lpT = std::clamp (st.tmLp.load(), 1000.0f, 20000.0f), hpT = std::clamp (st.tmHp.load(), 10.0f, 600.0f), mono = std::clamp (st.tmMono.load(), 0.0f, 1.0f);
        const float sm = 1.0f - std::exp (-(float) n / (0.05f * sr));
        lpHzSm += (lpT - lpHzSm) * sm; hpHzSm += (hpT - hpHzSm) * sm;
        SvfCoef lpC, hpC; lpC.set (std::min (lpHzSm, sr * 0.45f), 1.3f, sr); hpC.set (hpHzSm, 1.3f, sr);
        const bool bandOn = lpHzSm < 19000.0f || hpHzSm > 25.0f;
        // WOBBLE: wow (slow, drifting) + flutter (fast, small)
        const float rate = 0.2f + 1.6f * std::clamp (st.wobbleRate.load(), 0.0f, 1.0f);
        const float wowDepth = aWob * 0.0035f * sr, flutDepth = aWob * 0.00025f * sr, base = 0.012f * sr;
        // DISTORT
        const float drive = 1.0f + 10.0f * aDist, toneA = std::exp (-twoPi * (2500.0f + 15000.0f * std::clamp (st.distTone.load(), 0.0f, 1.0f)) / sr);
        // DIGITAL: bits + sample rate
        const float bits = 16.0f - 12.0f * std::clamp (aDig, 0.0f, 1.0f);
        const float levels = std::pow (2.0f, bits - 1.0f);
        const int hold = 1 + (int) (std::clamp (aDig, 0.0f, 1.0f) * (2.0f + 10.0f * std::clamp (st.digRate.load(), 0.0f, 1.0f)));
        // SPACE
        static const float dl[4] { 0.0297f, 0.0371f, 0.0411f, 0.0437f };
        const float size = 0.6f + 1.8f * std::clamp (st.spaceSize.load(), 0.0f, 1.0f), fb = 0.55f + 0.3f * std::clamp (st.spaceSize.load(), 0.0f, 1.0f);
        // FADE: magnetic dropouts
        const float fadeRate = 0.3f + 3.0f * std::clamp (st.fadeRate.load(), 0.0f, 1.0f);
        const int noiseType = st.noiseType.load();
        const float envA = std::exp (-1.0f / (0.004f * sr)), envR = std::exp (-1.0f / (0.9f * sr));
        for (int i = 0; i < n; ++i)
        {
            const float dryL = L[i], dryR = R[i];
            float l = L[i], r = R[i];
            if (mono > 0.001f) { const float m = 0.5f * (l + r); l += (m - l) * mono; r += (m - r) * mono; }
            if (bandOn) { tmLpS[0].tick (lpC, l); tmLpS[1].tick (lpC, r); l = tmLpS[0].lp; r = tmLpS[1].lp; tmHpS[0].tick (hpC, l); tmHpS[1].tick (hpC, r); l = tmHpS[0].hp; r = tmHpS[1].hp; }
            if (aDist > 0.001f)
            {
                auto sat = [&] (float x, float& z) { const float y = fastTanh (drive * x + 0.15f * aDist) - fastTanh (0.15f * aDist); z = y + toneA * (z - y); return z / std::sqrt (drive) * 1.2f; };
                const float sl = sat (l, distLp[0]), sr2 = sat (r, distLp[1]);
                l = l + (sl - l) * std::min (1.0f, aDist * 1.3f); r = r + (sr2 - r) * std::min (1.0f, aDist * 1.3f);
            }
            if (aWob > 0.001f)
            {
                wowPh += rate / sr; if (wowPh >= 1) { wowPh -= 1; wowDriftT = rng.bi(); }
                wowDrift += (wowDriftT - wowDrift) * 0.00005f;
                flutPh += (6.5f + 2.0f * wowDrift) / sr; if (flutPh >= 1) flutPh -= 1;
                const float d = base + wowDepth * (std::sin (twoPi * wowPh) * 0.7f + 0.3f * wowDrift) + flutDepth * std::sin (twoPi * flutPh);
                wob.push (l); wob2.push (r);
                l = wob.read (std::max (1.0f, d)); r = wob2.read (std::max (1.0f, d + 0.0004f * sr * aWob));
            }
            if (aDig > 0.001f)
            {
                if (--holdCnt <= 0) { holdCnt = hold; holdL = std::round (l * levels) / levels; holdR = std::round (r * levels) / levels; }
                const float k = std::min (1.0f, aDig * 1.5f);
                l += (holdL - l) * k; r += (holdR - r) * k;
            }
            if (aFade > 0.001f)
            {
                if (rng.uni() < fadeRate / sr) fadeTarget = 1.0f - aFade * (0.3f + 0.6f * rng.uni());   // a weak spot on the tape
                else if (rng.uni() < fadeRate * 2.0f / sr) fadeTarget = 1.0f;
                fadeVal += (fadeTarget - fadeVal) * 0.0006f;
                const float lpk = (0.15f + 0.85f * fadeVal * fadeVal) * (1.0f - 0.45f * std::min (1.0f, aFade));   // worn tape is a little dull; a dip dulls it more
                fadeLp[0] += (l - fadeLp[0]) * lpk; fadeLp[1] += (r - fadeLp[1]) * lpk;
                l = fadeLp[0] * fadeVal; r = fadeLp[1] * fadeVal;
            }
            if (aSpace > 0.001f)
            {
                std::array<float, 4> o;
                for (int k = 0; k < 4; ++k) o[(size_t) k] = fdn[(size_t) k].read (dl[k] * size * sr * 0.5f);
                const float s0 = o[0] + o[1] + o[2] + o[3];
                for (int k = 0; k < 4; ++k)
                {
                    float x = (o[(size_t) k] - 0.5f * s0) * fb + (k % 2 == 0 ? l : r) * 0.35f;
                    fdnLp[(size_t) k] += (x - fdnLp[(size_t) k]) * 0.45f;
                    fdn[(size_t) k].push (fdnLp[(size_t) k]);
                }
                const float wet = std::min (0.6f, aSpace * 0.55f);
                l += (o[0] + o[2]) * wet; r += (o[1] + o[3]) * wet;
            }
            if (aNoise > 0.001f)
            {
                // the noise follows the music (silence stays silent)
                const float in = std::max (std::abs (dryL), std::abs (dryR));
                noiseEnv = in > noiseEnv ? envA * noiseEnv + (1 - envA) * in : envR * noiseEnv + (1 - envR) * in;
                const float lvl = std::min (1.0f, noiseEnv * 4.0f) * aNoise;
                float nz = 0;
                const float wn = rng.bi();
                if (noiseType == 0)   // vinyl: surface + crackle + rumble
                {
                    hissLp += (wn - hissLp) * 0.25f;
                    nz = hissLp * 0.022f;
                    if (crackRng.uni() < 0.0006f + 0.002f * aNoise) nz += crackRng.bi() * (0.08f + 0.25f * crackRng.uni());
                    rumble += (wn - rumble) * 0.002f; nz += rumble * 0.15f;
                }
                else if (noiseType == 1) { hissHp = wn - hissLp; hissLp += (wn - hissLp) * 0.35f; nz = hissHp * 0.03f; }   // tape hiss
                else { humPh += 50.0f / sr; if (humPh >= 1) humPh -= 1; nz = (std::sin (twoPi * humPh) + 0.35f * std::sin (3 * twoPi * humPh)) * 0.012f; }   // mains hum
                l += nz * lvl; r += (noiseType == 2 ? nz : nz * 0.9f + rng.bi() * 0.002f) * lvl;
            }
            L[i] = dryL + (l - dryL) * mix; R[i] = dryR + (r - dryR) * mix;
        }
    }
};

// ---------------- ERA: the timeline of machines (a macro over TIME MACHINE) ----------------
struct EraPoint { const char* name; float year; float amt[numTm]; bool on[numTm]; int noise; float lp, hp, mono; };
inline const std::vector<EraPoint>& eras()
{
    static const std::vector<EraPoint> e {
        { "SHELLAC",   1930, { 0.75f, 0.45f, 0.35f, 0.0f, 0.15f, 0.35f }, { true, true, true, false, true, true },  0, 4200,  280, 1.0f },
        { "TAPE",      1962, { 0.30f, 0.45f, 0.45f, 0.0f, 0.25f, 0.20f }, { true, true, true, false, true, true },  1, 12000, 45,  0.3f },
        { "VINYL",     1974, { 0.55f, 0.20f, 0.25f, 0.0f, 0.15f, 0.0f },  { true, true, true, false, true, false }, 0, 15000, 35,  0.1f },
        { "SAMPLER",   1988, { 0.0f,  0.0f,  0.25f, 0.55f, 0.1f, 0.0f },  { false, false, true, true, true, false }, 1, 13000, 30,  0.0f },
        { "CASSETTE",  1995, { 0.35f, 0.55f, 0.30f, 0.0f, 0.1f, 0.45f },  { true, true, true, false, true, true },  1, 10000, 60,  0.0f },
        { "CLEAN",     2026, { 0.0f,  0.0f,  0.0f,  0.0f, 0.0f, 0.0f },   { false, false, false, false, false, false }, 1, 20000, 20, 0.0f },
        { "HYPER",     2100, { 0.0f,  0.0f,  0.55f, 0.35f, 0.45f, 0.0f }, { false, false, true, true, true, false }, 1, 20000, 25,  0.0f },
        { "FUTURE",    3000, { 0.0f,  0.25f, 0.2f,  0.15f, 0.8f, 0.0f },  { false, true, true, true, true, false },  2, 20000, 30,  0.0f },
    };
    return e;
}
inline void applyEra (MixLabState& st, float pos)
{
    const auto& e = eras();
    pos = std::clamp (pos, 0.0f, 1.0f) * (float) (e.size() - 1);
    const int a = std::min ((int) pos, (int) e.size() - 2); const float t = pos - (float) a;
    const auto& A = e[(size_t) a]; const auto& B = e[(size_t) a + 1];
    for (int m = 0; m < numTm; ++m)
    {
        const float v = A.amt[m] * (1 - t) + B.amt[m] * t;
        st.tmAmt[(size_t) m] = v;
        st.tmModOn[(size_t) m] = (t < 0.5f ? A.on[m] : B.on[m]) || v > 0.05f;
    }
    st.noiseType = t < 0.5f ? A.noise : B.noise;
    st.tmLp = std::exp (std::log (A.lp) * (1 - t) + std::log (B.lp) * t);
    st.tmHp = std::exp (std::log (A.hp) * (1 - t) + std::log (B.hp) * t);
    st.tmMono = A.mono * (1 - t) + B.mono * t;
    st.era = std::clamp (pos / (float) (e.size() - 1), 0.0f, 1.0f);
}
} // namespace kk
