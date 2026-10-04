#pragma once
#include "DspUtil.h"
#include <array>
#include <atomic>
#include <vector>
#include <cmath>
#include <algorithm>

// v0.42 EVOLVE FX PRO modules - names from three eras:
//   rare era:     DIAL-UP (an old phone line), FINAL BOSS (loudness + peak safe)
//   social era:   REMIX REEL (a step looper that re-cuts the music), DOODLE (draw a melody)
//   future era:   WARP DRIVE (pitch + frequency warping)
namespace kk::pro
{
// ---------------------------------------------------------------- REMIX REEL ----------------------------------------------------------------
enum ReelRow { rrSlice, rrLoop, rrEnv, rrFx1, rrFilter, rrFx2, numReelRows };
inline const char* reelRowName (int r) { static const char* n[] { "SLICE", "LOOP", "ENV", "FX 1", "FILTER", "FX 2" }; return n[std::clamp (r, 0, 5)]; }
inline int reelChoices (int r) { return r == rrSlice ? 17 : r == rrLoop ? 5 : r == rrEnv ? 5 : r == rrFx1 ? 5 : r == rrFilter ? 5 : 4; }
inline const char* reelValueName (int r, int v)
{
    static const char* loop[] { "", "1/2", "1/4", "1/8", "REV" };
    static const char* env[] { "", "IN", "OUT", "GATE", "DIP" };
    static const char* fx1[] { "", "CRUSH", "OCT+", "OCT-", "DIRT" };
    static const char* flt[] { "", "LP", "HP", "DARK", "BAND" };
    static const char* fx2[] { "", "STOP", "ECHO", "SPIN" };
    switch (r) { case rrLoop: return loop[std::clamp (v, 0, 4)]; case rrEnv: return env[std::clamp (v, 0, 4)]; case rrFx1: return fx1[std::clamp (v, 0, 4)];
                 case rrFilter: return flt[std::clamp (v, 0, 4)]; case rrFx2: return fx2[std::clamp (v, 0, 3)]; default: return ""; }
}

struct ReelState
{
    std::atomic<bool> on { false };
    std::array<std::array<std::atomic<int>, 16>, numReelRows> grid {};
    std::atomic<float> mix { 1.0f };
    std::atomic<int> stepNow { -1 };
    ReelState() { clear(); }
    void clear() { for (auto& r : grid) for (auto& c : r) c = 0; }
    void dice (int row, uint32_t seed)
    {
        Rng r; r.seed (seed * 2654435761u + (uint32_t) row * 97u);
        for (int s = 0; s < 16; ++s)
        {
            int v = 0;
            const float p = s % 4 == 0 ? 0.12f : 0.3f;
            if (r.uni() < p)
                v = row == rrSlice ? 1 + (int) (r.next() % 16) : 1 + (int) (r.next() % (uint32_t) (reelChoices (row) - 1));
            grid[(size_t) row][(size_t) s] = v;
            if (v != 0 && row != rrSlice && s < 15 && r.uni() < 0.35f) { grid[(size_t) row][(size_t) s + 1] = v; ++s; }
        }
    }
    void preset (int which)
    {
        clear();
        auto set = [this] (int row, std::initializer_list<std::pair<int, int>> cells) { for (auto& c : cells) grid[(size_t) row][(size_t) c.first] = c.second; };
        switch (which)
        {
            case 0: set (rrSlice, { { 4, 1 }, { 5, 1 }, { 12, 9 }, { 13, 9 } }); set (rrLoop, { { 14, 3 }, { 15, 3 } }); break;                      // RE-CUT
            case 1: set (rrLoop, { { 6, 2 }, { 7, 3 }, { 14, 3 }, { 15, 3 } }); set (rrFilter, { { 12, 2 }, { 13, 2 }, { 14, 2 }, { 15, 2 } }); break;   // BUILD-UP
            case 2: set (rrFx2, { { 7, 1 }, { 15, 1 } }); set (rrLoop, { { 11, 4 } }); set (rrEnv, { { 3, 3 }, { 11, 3 } }); break;                   // BRAKE
            case 3: set (rrFx1, { { 2, 2 }, { 3, 2 }, { 10, 3 }, { 11, 3 } }); set (rrSlice, { { 6, 3 }, { 14, 11 } }); set (rrFx2, { { 15, 2 } }); break;   // GLITCH POP
            default: set (rrEnv, { { 1, 3 }, { 3, 3 }, { 5, 3 }, { 7, 3 }, { 9, 3 }, { 11, 3 }, { 13, 3 }, { 15, 3 } }); set (rrFilter, { { 0, 4 }, { 8, 4 } }); break;   // CHOP
        }
    }
};

class ReelDsp
{
public:
    void prepare (double rate)
    {
        sr = (float) rate;
        len = (int) (rate * 14.0);   // two bars at 34 BPM
        bufL.assign ((size_t) len, 0.0f); bufR.assign ((size_t) len, 0.0f);
        w = 0; barStart[0] = barStart[1] = 0; lastBar = -1; lastStep = -1; tapePos = 0;
        echoL.prepare ((int) (rate * 2.0) + 8); echoR.prepare ((int) (rate * 2.0) + 8);
        for (auto& s : flt) s.reset();
        hold[0] = hold[1] = 0; holdN = 0; env = 0;
    }
    void process (float* L, float* R, int n, ReelState& st, double beatPos, double bps)
    {
        const bool on = st.on.load();
        const double stepBeats = 0.25;
        const int stepLen = std::max (64, (int) std::round (stepBeats / std::max (1.0e-9, bps)));
        const float mix = std::clamp (st.mix.load(), 0.0f, 1.0f);
        const int ramp = std::max (16, (int) (sr * 0.002f));
        for (int i = 0; i < n; ++i)
        {
            const float dl = L[i], dr = R[i];
            bufL[(size_t) w] = dl; bufR[(size_t) w] = dr;
            const double beat = beatPos + bps * i;
            const int bar = (int) std::floor (beat / 4.0);
            const double inBar = beat - bar * 4.0;
            const int s = std::clamp ((int) (inBar / stepBeats), 0, 15);
            const int pos = (int) ((inBar - s * stepBeats) / std::max (1.0e-9, bps));
            if (bar != lastBar) { barStart[1] = barStart[0]; barStart[0] = (w - (int) (inBar / std::max (1.0e-9, bps)) + len) % len; lastBar = bar; }
            if (s != lastStep) { lastStep = s; tapePos = 0; st.stepNow = on ? s : -1; }
            float l = dl, r = dr;
            if (on)
            {
                auto cell = [&st, s] (int row) { return st.grid[(size_t) row][(size_t) s].load(); };
                const int slice = cell (rrSlice), loop = cell (rrLoop), envv = cell (rrEnv), fx1 = cell (rrFx1), fl = cell (rrFilter), fx2 = cell (rrFx2);
                const bool reads = slice > 0 || loop > 0 || fx1 == 2 || fx1 == 3 || fx2 == 1 || fx2 == 3;
                if (reads)
                {
                    const int k = slice > 0 ? slice - 1 : s;
                    int rp = pos, rep = stepLen;
                    if (loop >= 1 && loop <= 3) { rep = std::max (32, stepLen >> loop); rp = pos % rep; }
                    if (loop == 4) rp = stepLen - 1 - pos;
                    if (fx1 == 2) rp = (2 * rp) % std::max (1, rep);
                    if (fx1 == 3) rp = rp / 2;
                    if (fx2 == 1) { tapePos += std::max (0.0f, 1.0f - (float) pos / (float) stepLen * 1.1f); rp = (int) tapePos; }
                    if (fx2 == 3) rp = stepLen - 1 - (int) std::pow ((float) pos / (float) stepLen, 0.6f) * (stepLen - 1);   // spin back
                    // the read must not run past the write head: steps that have not played yet (in this bar) come from the bar before
                    const bool ahead = k > s || (k == s && rp > pos);
                    const int base = ((ahead ? barStart[1] : barStart[0]) + k * stepLen) % len;
                    const int idx = (base + std::clamp (rp, 0, stepLen - 1)) % len;
                    const int ph = rp % std::max (1, rep);
                    const float win = std::min (1.0f, std::min ((float) ph / (float) ramp, (float) (rep - ph) / (float) ramp));
                    l = bufL[(size_t) idx] * win; r = bufR[(size_t) idx] * win;
                }
                // ENV
                const float fr = (float) pos / (float) stepLen;
                float g = 1.0f;
                if (envv == 1) g = fr; else if (envv == 2) g = 1.0f - fr; else if (envv == 3) g = fr < 0.5f ? 1.0f : 0.0f; else if (envv == 4) g = 0.3f + 0.7f * std::abs (2.0f * fr - 1.0f);
                env += (g - env) * 0.01f; l *= env; r *= env;
                // FX 1: crush / dirt
                if (fx1 == 1) { if (--holdN <= 0) { holdN = 6; hold[0] = std::round (l * 24.0f) / 24.0f; hold[1] = std::round (r * 24.0f) / 24.0f; } l = hold[0]; r = hold[1]; }
                if (fx1 == 4) { l = fastTanh (l * 4.0f) * 0.5f; r = fastTanh (r * 4.0f) * 0.5f; }
                // FILTER
                if (fl > 0)
                {
                    SvfCoef c;
                    const float hz = fl == 1 ? 9000.0f * std::exp2 (-fr * 5.0f) : fl == 2 ? 100.0f * std::exp2 (fr * 6.0f) : fl == 3 ? 600.0f : 1200.0f;
                    c.set (hz, fl == 4 ? 0.6f : 1.4f, sr);
                    flt[0].tick (c, l); flt[1].tick (c, r);
                    l = fl == 2 ? flt[0].hp : fl == 4 ? flt[0].bp * 1.6f : flt[0].lp;
                    r = fl == 2 ? flt[1].hp : fl == 4 ? flt[1].bp * 1.6f : flt[1].lp;
                }
                // FX 2: echo throw
                const float el = echoL.read ((float) stepLen * 3.0f), er = echoR.read ((float) stepLen * 3.0f);
                echoL.push ((fx2 == 2 ? l : 0.0f) + er * 0.45f); echoR.push ((fx2 == 2 ? r : 0.0f) + el * 0.45f);
                l += el * 0.7f; r += er * 0.7f;
            }
            else { echoL.push (0); echoR.push (0); }
            L[i] = dl + (l - dl) * (on ? mix : 0.0f); R[i] = dr + (r - dr) * (on ? mix : 0.0f);
            w = (w + 1) % len;
        }
    }
private:
    float sr = 44100;
    std::vector<float> bufL, bufR; int len = 1, w = 0, barStart[2] {}, lastBar = -1, lastStep = -1;
    float tapePos = 0, hold[2] {}, env = 1; int holdN = 0;
    DelayLine echoL, echoR; std::array<SvfState, 2> flt;
};

// ---------------------------------------------------------------- DIAL-UP ----------------------------------------------------------------
enum DialMode { dmLandline, dmVoiceNote, dmBadSignal, dmRobotToy, dmWalkie, dmBlownSpeaker, numDialModes };
inline const char* dialModeName (int m) { static const char* n[] { "LANDLINE", "VOICE NOTE", "BAD SIGNAL", "ROBOT TOY", "WALKIE", "BLOWN SPEAKER" }; return n[std::clamp (m, 0, (int) numDialModes - 1)]; }
struct DialState
{
    std::atomic<bool> on { false };
    std::atomic<int> mode { dmLandline };
    std::atomic<float> signal { 0.3f }, crush { 0.3f }, tinny { 0.5f }, robot { 0.0f }, mix { 1.0f };
    void applyMode (int m)
    {
        mode = m;
        switch (m)
        {
            case dmLandline:     signal = 0.05f; crush = 0.15f; tinny = 0.6f; robot = 0.0f; break;
            case dmVoiceNote:    signal = 0.1f;  crush = 0.35f; tinny = 0.35f; robot = 0.0f; break;
            case dmBadSignal:    signal = 0.8f;  crush = 0.55f; tinny = 0.5f; robot = 0.1f; break;
            case dmRobotToy:     signal = 0.0f;  crush = 0.6f;  tinny = 0.4f; robot = 0.8f; break;
            case dmWalkie:       signal = 0.3f;  crush = 0.25f; tinny = 0.9f; robot = 0.0f; break;
            default:             signal = 0.15f; crush = 0.2f;  tinny = 1.0f; robot = 0.0f; break;
        }
    }
};
class DialDsp
{
public:
    void prepare (double rate) { sr = (float) rate; for (auto& s : hp) s.reset(); for (auto& s : lp) s.reset(); for (auto& s : lp2) s.reset(); for (auto& s : pk) s.reset(); rng.seed (555); drop = 1; dropT = 1; pkt.assign ((size_t) (rate * 0.08), 0.0f); pktW = 0; repeating = 0; ringPh = 0; holdN = 0; }
    void process (float* L, float* R, int n, DialState& st)
    {
        if (! st.on.load()) return;
        const int mode = st.mode.load();
        const float sig = st.signal.load(), cr = st.crush.load(), tin = st.tinny.load(), rob = st.robot.load(), mix = st.mix.load();
        const float lo = mode == dmVoiceNote ? 120.0f : mode == dmBlownSpeaker ? 500.0f : 300.0f + 300.0f * tin;
        const float hi = mode == dmVoiceNote ? 7000.0f : mode == dmWalkie ? 2600.0f : 3400.0f - 1200.0f * tin;
        SvfCoef hc, lc, pc, lc2; hc.set (lo, 1.2f, sr); lc.set (hi, 1.1f, sr); pc.set (1800.0f + 1200.0f * tin, 0.35f, sr); lc2.set (hi * 1.25f, 1.3f, sr);
        const float levels = std::pow (2.0f, 12.0f - 9.0f * cr);
        const int holdLen = 1 + (int) (cr * 7.0f);
        const int pl = (int) pkt.size();
        for (int i = 0; i < n; ++i)
        {
            const float dl = L[i], dr = R[i];
            float x = 0.5f * (dl + dr);   // a phone is mono
            hp[0].tick (hc, x); x = hp[0].hp; lp[0].tick (lc, x); x = lp[0].lp;
            pk[0].tick (pc, x); x += pk[0].bp * (1.5f * tin);          // the tiny speaker's honk
            x = fastTanh (x * (1.5f + 3.0f * tin + (mode == dmBlownSpeaker ? 4.0f : 0.0f))) * 0.6f;
            lp2[0].tick (lc2, x); x = lp2[0].lp;   // the speaker cannot play what the distortion adds up high
            if (cr > 0.01f) { if (--holdN <= 0) { holdN = holdLen; held = std::round (x * levels) / levels; } x = held; }
            if (rob > 0.01f) { ringPh += (60.0f + 120.0f * rob) / sr; if (ringPh >= 1) ringPh -= 1; x = x * (1.0f - rob) + x * std::sin (twoPi * ringPh) * rob * 1.6f; }
            // BAD SIGNAL: dropouts and packets that repeat
            pkt[(size_t) pktW] = x; pktW = (pktW + 1) % pl;
            if (sig > 0.01f && rng.uni() < sig * 6.0f / sr)
            {
                if (rng.uni() < 0.5f) dropT = 0.0f; else repeating = (int) (sr * (0.04f + 0.12f * rng.uni()));
            }
            else if (dropT < 1.0f && rng.uni() < 25.0f / sr) dropT = 1.0f;
            drop += (dropT - drop) * 0.02f;
            if (repeating > 0) { --repeating; x = pkt[(size_t) ((pktW - (int) (sr * 0.03f) + pl * 4) % pl)]; }
            x *= drop;
            if (mode == dmWalkie && drop < 0.5f) x += rng.bi() * 0.03f * (1.0f - drop);   // squelch hiss
            L[i] = dl + (x - dl) * mix; R[i] = dr + (x - dr) * mix;
        }
    }
private:
    float sr = 44100; std::array<SvfState, 1> hp, lp, pk, lp2; Rng rng;
    float drop = 1, dropT = 1, held = 0, ringPh = 0; int holdN = 0, pktW = 0, repeating = 0; std::vector<float> pkt;
};

// ---------------------------------------------------------------- WARP DRIVE ----------------------------------------------------------------
enum WarpMode { wmUp, wmDown, wmChipmunk, wmDemon, wmWobble, wmAlien, numWarpModes };
inline const char* warpModeName (int m) { static const char* n[] { "OCTAVE UP", "OCTAVE DOWN", "CHIPMUNK", "DEMON", "WOBBLE", "ALIEN" }; return n[std::clamp (m, 0, (int) numWarpModes - 1)]; }
struct WarpState
{
    std::atomic<bool> on { false };
    std::atomic<float> semis { 12.0f }, shiftHz { 0.0f }, wobble { 0.0f }, mix { 0.5f };
    std::atomic<int> mode { wmUp };
    void applyMode (int m)
    {
        mode = m;
        switch (m)
        {
            case wmUp:       semis = 12;  shiftHz = 0;    wobble = 0;    mix = 0.45f; break;
            case wmDown:     semis = -12; shiftHz = 0;    wobble = 0;    mix = 0.5f; break;
            case wmChipmunk: semis = 7;   shiftHz = 0;    wobble = 0.1f; mix = 1.0f; break;
            case wmDemon:    semis = -7;  shiftHz = -30;  wobble = 0.05f; mix = 1.0f; break;
            case wmWobble:   semis = 0;   shiftHz = 0;    wobble = 0.7f; mix = 1.0f; break;
            default:         semis = 0;   shiftHz = 180;  wobble = 0.2f; mix = 0.8f; break;
        }
    }
};
class WarpDsp
{
public:
    void prepare (double rate) { sr = (float) rate; for (auto& d : buf) d.prepare ((int) (rate * 0.2) + 8); ph = 0; wobPh = 0; shPh = 0; for (auto& h : hb) h = {}; }
    void process (float* L, float* R, int n, WarpState& st)
    {
        if (! st.on.load()) return;
        const float semis = st.semis.load(), sh = st.shiftHz.load(), wob = st.wobble.load(), mix = st.mix.load();
        const float win = 0.06f * sr;
        for (int i = 0; i < n; ++i)
        {
            wobPh += 3.5f / sr; if (wobPh >= 1) wobPh -= 1;
            const float ratio = std::exp2 ((semis + wob * 1.2f * std::sin (twoPi * wobPh)) / 12.0f);
            ph += (1.0f - ratio) / win; ph -= std::floor (ph);   // the grain read head drifts against the write head
            const float a1 = ph, a2 = std::fmod (ph + 0.5f, 1.0f);
            const float w1 = std::sin (pi * a1), w2 = std::sin (pi * a2);
            float out[2];
            const float in[2] { L[i], R[i] };
            for (int c = 0; c < 2; ++c)
            {
                buf[(size_t) c].push (in[c]);
                float y = buf[(size_t) c].read (1.0f + a1 * win) * w1 + buf[(size_t) c].read (1.0f + a2 * win) * w2;
                if (std::abs (sh) > 0.5f)
                {
                    float re, im; hb[(size_t) c].tick (y, re, im);
                    const float cs = std::cos (twoPi * shPh), sn = std::sin (twoPi * shPh);
                    y = re * cs - im * sn;
                }
                out[c] = y;
            }
            shPh += sh / sr; shPh -= std::floor (shPh);
            L[i] = in[0] + (out[0] - in[0]) * mix; R[i] = in[1] + (out[1] - in[1]) * mix;
        }
    }
private:
    struct Hilbert
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
    float sr = 44100, ph = 0, wobPh = 0, shPh = 0;
    std::array<DelayLine, 2> buf; std::array<Hilbert, 2> hb;
};

// ---------------------------------------------------------------- FINAL BOSS ----------------------------------------------------------------
enum BossTarget { btOff, btStreaming, btSoundcloud, btClub, numBossTargets };
inline const char* bossTargetName (int t) { static const char* n[] { "METER ONLY", "STREAMING  -14", "SOUNDCLOUD  -9", "CLUB  -7" }; return n[std::clamp (t, 0, 3)]; }
inline float bossTargetLufs (int t) { static const float v[] { -99, -14, -9, -7 }; return v[std::clamp (t, 0, 3)]; }
struct BossState
{
    std::atomic<bool> on { false };
    std::atomic<int> target { btOff };
    std::atomic<float> drive { 0.0f }, ceiling { -1.0f };
    std::atomic<float> mMomentary { -70 }, mShort { -70 }, mIntegrated { -70 }, mGr { 0 }, mPeak { -70 }, mAuto { 0 };
    std::atomic<bool> resetIntegrated { false };
};
class BossDsp
{
public:
    static int lookahead (double rate) { return (int) (rate * 0.0015); }
    void prepare (double rate)
    {
        sr = (float) rate; la = lookahead (rate);
        dl.assign ((size_t) (la + 1), 0.0f); dr.assign ((size_t) (la + 1), 0.0f); wpos = 0;
        hs.set (1500.0f, 0.71f, sr); hpC.set (38.0f, 1.2f, sr); for (auto& s : kS) s.reset(); for (auto& s : kH) s.reset();
        msq = ssq = 0; integSum = 0; integN = 0; gain = 1; autoDb = 0;
        peakWin.assign ((size_t) (la + 1), 0.0f);
    }
    void process (float* L, float* R, int n, BossState& st)
    {
        if (st.resetIntegrated.exchange (false)) { integSum = 0; integN = 0; }
        const bool on = st.on.load();
        const float aM = std::exp (-1.0f / (0.4f * sr)), aS = std::exp (-1.0f / (3.0f * sr));
        float pk = 0, grMin = 0;
        const float ceil = std::pow (10.0f, std::clamp (st.ceiling.load(), -12.0f, 0.0f) / 20.0f);
        const int tgt = st.target.load();
        const float drive = std::pow (10.0f, (st.drive.load() + (on ? autoDb : 0.0f)) / 20.0f);
        const float rel = std::exp (-1.0f / (0.08f * sr));
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            if (on) { l *= drive; r *= drive; }
            // loudness (K-weighted, mono sum of powers)
            float kl = l, kr = r;
            kS[0].tick (hs, kl); kl = kl + kS[0].hp * 0.58f; kS[1].tick (hs, kr); kr = kr + kS[1].hp * 0.58f;   // +4 dB high shelf, approx.
            kH[0].tick (hpC, kl); kl = kH[0].hp; kH[1].tick (hpC, kr); kr = kH[1].hp;
            const float p = kl * kl + kr * kr;
            msq = aM * msq + (1 - aM) * p; ssq = aS * ssq + (1 - aS) * p;
            // limiter with lookahead: the delayed signal is turned down before a peak arrives
            dl[(size_t) wpos] = l; dr[(size_t) wpos] = r;
            peakWin[(size_t) wpos] = std::max (std::abs (l), std::abs (r));
            float ahead = 0; for (float v : peakWin) ahead = std::max (ahead, v);
            const float need = on && ahead > ceil ? ceil / ahead : 1.0f;
            gain = need < gain ? need : rel * gain + (1 - rel) * need;
            const int rp = (wpos + 1) % (la + 1);
            float ol = dl[(size_t) rp], orr = dr[(size_t) rp];
            if (on) { ol = std::clamp (ol * gain, -ceil, ceil); orr = std::clamp (orr * gain, -ceil, ceil); }
            wpos = rp;
            L[i] = ol; R[i] = orr;
            pk = std::max (pk, std::max (std::abs (ol), std::abs (orr)));
            grMin = std::min (grMin, 20.0f * std::log10 (std::max (1.0e-6f, gain)));
        }
        const float mom = -0.691f + 10.0f * std::log10 (msq + 1e-12f), sh = -0.691f + 10.0f * std::log10 (ssq + 1e-12f);
        if (mom > -70.0f) { integSum += std::pow (10.0f, mom / 10.0f); ++integN; }
        st.mMomentary = mom; st.mShort = sh; st.mIntegrated = integN > 0 ? 10.0f * std::log10 ((float) (integSum / integN)) : -70.0f;
        st.mGr = grMin; st.mPeak = 20.0f * std::log10 (pk + 1e-9f);
        // AUTO: the gain walks slowly toward the target loudness (1.5 dB per second at most)
        if (on && tgt != btOff && sh > -45.0f)
        {
            const float diff = bossTargetLufs (tgt) - sh;
            autoDb += std::clamp (diff, -1.0f, 1.0f) * 1.5f * (float) n / sr;
            autoDb = std::clamp (autoDb, -12.0f, 18.0f);
        }
        st.mAuto = autoDb;
    }
private:
    float sr = 44100; int la = 64, wpos = 0;
    std::vector<float> dl, dr, peakWin;
    SvfCoef hs, hpC; std::array<SvfState, 2> kS, kH;
    float msq = 0, ssq = 0, gain = 1, autoDb = 0; double integSum = 0; long integN = 0;
};
} // namespace kk::pro
