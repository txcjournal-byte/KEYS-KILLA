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
// v0.43 organic modules (the sound behaves like a living thing):
//   LIQUID (spectral cannibalism - the kick carves its hole, the bass flows around it), INTENT (one breath drives many muscles),
//   EROSION (material fatigue: overload tires the sound, starving it lets it sink into rumble)
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

// ---------------------------------------------------------------- LIQUID ----------------------------------------------------------------
// Two liquids in one vessel: where the kick lands (per band, 30-300 Hz) the track is carved out, and the removed energy
// flows one band up (an octave-up "spill"), so the bass stays audible while the kick owns its frequencies.
enum LiquidSource { lsSidechain, lsSelf };
struct LiquidState
{
    static constexpr int numBands = 5;
    std::atomic<bool> on { false };
    std::atomic<int> source { lsSidechain };
    std::atomic<float> flow { 0.6f }, depth { 0.85f }, viscosity { 0.35f }, mix { 1.0f };
    std::array<std::atomic<float>, numBands> mHole {};   // 0..1 how deep each band is carved right now
    std::atomic<float> mKick { 0 };                      // 0..1 kick level
    std::atomic<bool> mSidechain { false };              // a sidechain signal is arriving
};
class LiquidDsp
{
public:
    static constexpr int numBands = LiquidState::numBands;
    static float edge (int k) { static const float e[numBands] { 45.0f, 70.0f, 110.0f, 180.0f, 300.0f }; return e[std::clamp (k, 0, numBands - 1)]; }
    static float centre (int k) { return k == 0 ? 37.0f : std::sqrt (edge (k - 1) * edge (k)); }
    static constexpr float bandK = 0.55f;   // band width (1 / Q) - neighbouring bands overlap a little
    void prepare (double rate, int maxBlock)
    {
        (void) maxBlock; sr = (float) rate;
        for (int k = 0; k < numBands; ++k) { bpC[(size_t) k].set (centre (k), bandK, sr); spC[(size_t) k].set (2.0f * centre (k), 1.0f, sr); }
        for (auto& ch : bell) for (auto& s : ch) s.reset();
        for (auto& ch : det) for (auto& s : ch) s.reset();
        for (auto& ch : sp) for (auto& s : ch) s.reset();
        fast.fill (0); slow.fill (0); hole.fill (0);
        aAtt = std::exp (-1.0f / (0.001f * sr)); aRelDet = std::exp (-1.0f / (0.06f * sr)); aSlow = std::exp (-1.0f / (0.25f * sr)); aHoleAtt = std::exp (-1.0f / (0.002f * sr));
    }
    // scL / scR: the sidechain (nullptr when nothing is connected - then the track's own low end is the kick)
    void process (float* L, float* R, int n, LiquidState& st, const float* scL, const float* scR)
    {
        if (! st.on.load()) { if (st.mKick.load() > 0.0f) { for (auto& h : st.mHole) h = 0; st.mKick = 0; } st.mSidechain = false; return; }
        const bool useSc = st.source.load() == lsSidechain && scL != nullptr;
        const float flow = std::clamp (st.flow.load(), 0.0f, 1.0f), depth = std::clamp (st.depth.load(), 0.0f, 1.0f), mix = std::clamp (st.mix.load(), 0.0f, 1.0f);
        const float aRel = std::exp (-1.0f / ((0.03f * std::pow (25.0f, std::clamp (st.viscosity.load(), 0.0f, 1.0f))) * sr));   // water 30 ms .. mercury 750 ms
        float kickMax = 0, scPeak = 0;
        for (int i = 0; i < n; ++i)
        {
            const float in[2] { L[i], R[i] };
            // the kick, per band (band-passes with unity gain at their centre)
            float lvl[numBands], dmax = 1.0e-9f;
            const float m = useSc ? 0.5f * (scL[i] + (scR != nullptr ? scR[i] : scL[i])) : 0.5f * (in[0] + in[1]);
            if (useSc) scPeak = std::max (scPeak, std::abs (m));
            for (int k = 0; k < numBands; ++k)
            {
                auto& s = det[0][(size_t) k]; s.tick (bpC[(size_t) k], m);
                const float a = std::abs (bandK * s.bp);
                auto& f = fast[(size_t) k];
                f = a > f ? aAtt * f + (1 - aAtt) * a : aRelDet * f + (1 - aRelDet) * a;
                if (useSc) lvl[k] = f;
                else   // SELF: only the onsets of the track's own low end (a held bass is not a kick)
                {
                    slow[(size_t) k] = aSlow * slow[(size_t) k] + (1 - aSlow) * f;
                    lvl[k] = std::max (0.0f, f - 1.4f * slow[(size_t) k]) * 2.5f;
                }
                dmax = std::max (dmax, lvl[k]);
            }
            const float amount = std::clamp ((20.0f * std::log10 (dmax + 1.0e-9f) + 40.0f) / 24.0f, 0.0f, 1.0f);   // -40 dBFS = nothing, -16 dBFS = all
            kickMax = std::max (kickMax, amount);
            // the hole: a chain of dynamic bell cuts (x - h * band: exactly (1 - h) at each centre, untouched when h = 0)
            float out[2] { in[0], in[1] }, spill[2] {};
            for (int k = 0; k < numBands; ++k)
            {
                const float tgt = depth * amount * std::pow (lvl[k] / dmax, 1.5f);   // the bands where the kick lives get the deepest hole
                hole[(size_t) k] = tgt > hole[(size_t) k] ? aHoleAtt * hole[(size_t) k] + (1 - aHoleAtt) * tgt : aRel * hole[(size_t) k] + (1 - aRel) * tgt;
                const float h = hole[(size_t) k];
                for (int c = 0; c < 2; ++c)
                {
                    auto& b = bell[(size_t) c][(size_t) k]; b.tick (bpC[(size_t) k], out[c]);
                    const float removed = h * bandK * b.bp;
                    out[c] -= removed;
                    sp[(size_t) c][(size_t) k].tick (spC[(size_t) k], std::abs (removed));   // the removed liquid, one octave up, poured into the band above
                    spill[c] += sp[(size_t) c][(size_t) k].bp;
                }
            }
            for (int c = 0; c < 2; ++c) out[c] += flow * 2.2f * spill[c];
            L[i] = in[0] + (out[0] - in[0]) * mix; R[i] = in[1] + (out[1] - in[1]) * mix;
        }
        for (int k = 0; k < numBands; ++k) st.mHole[(size_t) k] = hole[(size_t) k];
        st.mKick = kickMax; st.mSidechain = useSc && scPeak > 1.0e-4f;
    }
private:
    float sr = 44100, aAtt = 0, aRelDet = 0, aSlow = 0, aHoleAtt = 0;
    std::array<SvfCoef, numBands> bpC, spC;
    std::array<std::array<SvfState, numBands>, 2> bell, sp;
    std::array<std::array<SvfState, numBands>, 1> det;
    std::array<float, numBands> fast {}, slow {}, hole {};
};

// ---------------------------------------------------------------- INTENT ----------------------------------------------------------------
// One breath instead of dozens of automations: a biological state moves many "muscles" (rasp, width, filter, reverb freeze,
// tremor, attack) at once, each with its own curve.  LEVEL = the breath: 1 = relaxed and clean, 0 = the state at its extreme.
enum IntentMode { imOxygen, imAggression, imStress, imCalm, numIntentModes };
inline const char* intentModeName (int m) { static const char* n[] { "OXYGEN", "AGGRESSION", "STRESS", "CALM" }; return n[std::clamp (m, 0, (int) numIntentModes - 1)]; }
struct IntentState
{
    std::atomic<bool> on { false };
    std::atomic<int> mode { imOxygen };
    std::atomic<float> level { 1.0f }, mix { 1.0f };
    std::atomic<float> mRasp { 0 }, mWidth { 1 }, mFreeze { 0 }, mFilterHz { 20000 }, mTremor { 0 }, mPulse { 0 };
};
class IntentDsp
{
public:
    struct Targets { float rasp = 0, width = 1, lpHz = 20000, verbMix = 0, verbFb = 0.5f, freeze = 0, tremor = 0, transient = 0; };
    static void mapping (int state, float level, Targets& t)
    {
        const float d = 1.0f - std::clamp (level, 0.0f, 1.0f);   // how far the breath is pulled down
        auto smooth = [] (float a, float b, float x) { const float u = std::clamp ((x - a) / (b - a), 0.0f, 1.0f); return u * u * (3.0f - 2.0f * u); };
        t = {};
        switch (state)
        {
            case imOxygen:     t.rasp = 0.95f * std::pow (d, 1.3f); t.width = 1.0f - 0.95f * d; t.lpHz = 20000.0f * std::pow (0.08f, std::pow (d, 1.2f));
                               t.verbMix = 0.45f * d; t.verbFb = 0.6f + 0.3f * d; t.freeze = smooth (0.45f, 1.0f, d); t.tremor = 0.15f * d * d; break;
            case imAggression: t.rasp = 0.8f * std::pow (d, 0.8f); t.width = 1.0f - 0.3f * d; t.transient = d; t.verbMix = 0.05f * d; t.verbFb = 0.4f; t.tremor = 0.05f * d; break;
            case imStress:     t.tremor = d; t.rasp = 0.35f * d; t.width = 1.0f - 0.5f * d; t.lpHz = 20000.0f * std::pow (0.35f, d); t.verbMix = 0.2f * d; t.verbFb = 0.75f; t.transient = 0.3f * d; break;
            default:           t.width = 1.0f + 0.5f * d; t.lpHz = 20000.0f * std::pow (0.25f, d); t.verbMix = 0.4f * d; t.verbFb = 0.75f + 0.2f * d; t.freeze = 0.25f * d * d; t.transient = -0.6f * d; break;
        }
    }
    void prepare (double rate, int maxBlock)
    {
        (void) maxBlock; sr = (float) rate;
        static const float ms[4] { 31.0f, 43.0f, 53.0f, 67.0f };
        for (int k = 0; k < 4; ++k) { fdn[(size_t) k].prepare ((int) (rate * 0.08) + 8); len[k] = ms[k] * 0.001f * sr; damp[k] = 0; }
        for (auto& d : jit) d.prepare ((int) (rate * 0.01) + 8);
        for (auto& s : flt) s.reset();
        for (auto& d : dc) { d = {}; d.setHz (10.0f, sr); }
        cur = {}; envF = envS = 0; tremPh = 0; tremHz = 10; jitPh = 0; rng.seed (4242); fltCount = 0; pulse = 0;
        aSm = std::exp (-1.0f / (0.03f * sr)); aF = std::exp (-1.0f / (0.002f * sr)); aS = std::exp (-1.0f / (0.04f * sr));
    }
    void process (float* L, float* R, int n, IntentState& st)
    {
        if (! st.on.load()) return;
        Targets tg; mapping (st.mode.load(), st.level.load(), tg);
        const float mix = std::clamp (st.mix.load(), 0.0f, 1.0f);
        float pk = 0;
        for (int i = 0; i < n; ++i)
        {
            // every muscle moves smoothly
            cur.rasp += (tg.rasp - cur.rasp) * (1 - aSm); cur.width += (tg.width - cur.width) * (1 - aSm); cur.lpHz += (tg.lpHz - cur.lpHz) * (1 - aSm);
            cur.verbMix += (tg.verbMix - cur.verbMix) * (1 - aSm); cur.verbFb += (tg.verbFb - cur.verbFb) * (1 - aSm); cur.freeze += (tg.freeze - cur.freeze) * (1 - aSm);
            cur.tremor += (tg.tremor - cur.tremor) * (1 - aSm); cur.transient += (tg.transient - cur.transient) * (1 - aSm);
            if (--fltCount <= 0) { fltCount = 16; fc.set (std::min (cur.lpHz, sr * 0.45f), 0.9f, sr); }
            const float dl = L[i], dr = R[i];
            float x[2] { dl, dr };
            // attack: AGGRESSION bites harder, CALM rounds the hits off
            const float a = 0.5f * (std::abs (dl) + std::abs (dr));
            envF = aF * envF + (1 - aF) * a; envS = aS * envS + (1 - aS) * a;
            const float onset = std::clamp (envF / (envS + 1.0e-5f) - 1.0f, 0.0f, 3.0f);
            const float tGain = std::clamp (cur.transient > 0 ? 1.0f + cur.transient * onset * 0.8f : 1.0f + cur.transient * std::min (onset, 1.0f) * 0.6f, 0.3f, 3.4f);
            // tremor: a shaking amplitude and a jittering pitch (STRESS)
            if (rng.uni() < 4.0f / sr) tremHz = 8.0f + 7.0f * rng.uni();
            tremPh += tremHz / sr; tremPh -= std::floor (tremPh); jitPh += (tremHz * 0.37f) / sr; jitPh -= std::floor (jitPh);
            const float am = 1.0f - 0.55f * cur.tremor * (0.5f + 0.5f * std::sin (twoPi * tremPh));
            for (int c = 0; c < 2; ++c)
            {
                float v = x[c] * tGain;
                jit[(size_t) c].push (v);
                const float j = jit[(size_t) c].read (sr * (0.002f + 0.0012f * std::sin (twoPi * (jitPh + 0.25f * (float) c))));
                v = (v + cur.tremor * (j - v)) * am;
                // rasp: asymmetric saturation (the throat), DC removed
                const float drv = 1.0f + 12.0f * cur.rasp, bias = 0.25f * cur.rasp;
                const float sat = (fastTanh (drv * (v + bias)) - fastTanh (drv * bias)) * 0.7f;
                v = dc[(size_t) c].tick (v + cur.rasp * (sat - v));
                flt[(size_t) c].tick (fc, v); x[c] = flt[(size_t) c].lp;
            }
            // a small 4-line reverb whose feedback reaches a freeze, its input gated when frozen
            const float fb = cur.verbFb + (0.995f - cur.verbFb) * cur.freeze, gIn = 0.5f * (1.0f - cur.freeze), dmp = 0.35f * (1.0f - cur.freeze);
            float y[4]; for (int k = 0; k < 4; ++k) { y[k] = fdn[(size_t) k].read (len[k]); damp[k] += (y[k] - damp[k]) * (1.0f - dmp); y[k] = damp[k]; }
            const float h0 = 0.5f * (y[0] + y[1] + y[2] + y[3]), h1 = 0.5f * (y[0] - y[1] + y[2] - y[3]), h2 = 0.5f * (y[0] + y[1] - y[2] - y[3]), h3 = 0.5f * (y[0] - y[1] - y[2] + y[3]);
            fdn[0].push (x[0] * gIn + fb * h0); fdn[1].push (x[1] * gIn + fb * h1); fdn[2].push (x[0] * gIn + fb * h2); fdn[3].push (x[1] * gIn + fb * h3);
            float l = x[0] + cur.verbMix * 0.7f * (y[0] + y[2]), r = x[1] + cur.verbMix * 0.7f * (y[1] + y[3]);
            // width: down to a claustrophobic mono
            const float m = 0.5f * (l + r), s = 0.5f * (l - r) * cur.width;
            l = m + s; r = m - s;
            L[i] = dl + (l - dl) * mix; R[i] = dr + (r - dr) * mix;
            pk = std::max (pk, std::abs (m));
        }
        pulse = std::max (pk, pulse * 0.85f);
        st.mRasp = cur.rasp; st.mWidth = cur.width; st.mFreeze = cur.freeze; st.mFilterHz = cur.lpHz; st.mTremor = cur.tremor; st.mPulse = pulse;
    }
private:
    float sr = 44100, aSm = 0, aF = 0, aS = 0, envF = 0, envS = 0, tremPh = 0, tremHz = 10, jitPh = 0, pulse = 0, len[4] {}, damp[4] {};
    int fltCount = 0; Targets cur; SvfCoef fc; Rng rng;
    std::array<DelayLine, 4> fdn; std::array<DelayLine, 2> jit; std::array<SvfState, 2> flt; std::array<DcBlock, 2> dc;
};

// ---------------------------------------------------------------- EROSION ----------------------------------------------------------------
// Material fatigue: a hot signal tires the sound (the highs wear off and recover slowly, cavitation bubbles crackle),
// a starved one sinks into a subsonic rumble with an unstable phase.  Silence stays silent.
struct ErosionState
{
    std::atomic<bool> on { false };
    std::atomic<float> sensitivity { 0.5f }, fatigue { 0.5f }, recovery { 0.5f }, mix { 1.0f };
    std::atomic<float> mFatigue { 0 }, mStarve { 0 }, mLevel { -100 };
    static float thresholdDb (float sens) { return -4.0f - 14.0f * std::clamp (sens, 0.0f, 1.0f); }   // 0.5 = -11 dBFS
    static float floorDb (float sens) { return -42.0f + 14.0f * std::clamp (sens, 0.0f, 1.0f); }      // 0.5 = -35 dBFS
};
class ErosionDsp
{
public:
    static constexpr float gateDb = -70.0f;
    void prepare (double rate, int maxBlock)
    {
        (void) maxBlock; sr = (float) rate;
        for (auto& ch : lp) for (auto& s : ch) s.reset();
        for (auto& o : rum) for (auto& p : o) { p.reset(); p.setHz (28.0f, sr); }
        for (auto& d : rumDc) { d = {}; d.setHz (12.0f, sr); }
        for (auto& p : pops) p = {};
        for (auto& a : ap) a = {};
        ms = 0; fat = 0; starve = 0; gateG = 0; wetW = 0; fltCount = 0; rng.seed (777); wob[0] = wob[1] = wobT[0] = wobT[1] = 0;
        aMs = std::exp (-1.0f / (0.05f * sr));
    }
    void process (float* L, float* R, int n, ErosionState& st)
    {
        if (! st.on.load()) return;
        const float sens = st.sensitivity.load(), thr = ErosionState::thresholdDb (sens), flo = ErosionState::floorDb (sens), mix = std::clamp (st.mix.load(), 0.0f, 1.0f);
        const float speed = 0.05f + 0.7f * std::clamp (st.fatigue.load(), 0.0f, 1.0f);                           // per second, 6 dB over
        const float recPerS = 1.0f / (12.0f * std::pow (0.05f, std::clamp (st.recovery.load(), 0.0f, 1.0f)));     // 12 s .. 0.6 s to recover fully
        const float aSt = std::exp (-1.0f / (0.8f * sr)), aStRel = std::exp (-1.0f / (0.3f * sr)), gStep = 1.0f / (0.02f * sr);
        float lvlDb = -100;
        for (int i = 0; i < n; ++i)
        {
            const float dl = L[i], dr = R[i];
            ms = aMs * ms + (1 - aMs) * 0.5f * (dl * dl + dr * dr);
            lvlDb = 10.0f * std::log10 (ms + 1.0e-12f);
            const float over = lvlDb - thr;
            fat = std::clamp (over > 0 ? fat + speed * (0.3f + std::min (over / 6.0f, 2.0f)) / sr : fat - recPerS / sr, 0.0f, 1.0f);
            const float stT = lvlDb > gateDb && lvlDb < flo ? std::clamp ((flo - lvlDb) / 15.0f, 0.0f, 1.0f) : 0.0f;
            starve = stT > starve ? aSt * starve + (1 - aSt) * stT : aStRel * starve + (1 - aStRel) * stT;
            gateG = lvlDb > gateDb ? std::min (1.0f, gateG + gStep) : std::max (0.0f, gateG - gStep);
            if (--fltCount <= 0) { fltCount = 32; fc.set (18000.0f * std::exp2 (-5.0f * fat), 1.41421356f, sr); }
            wetW += (std::min (1.0f, fat * 200.0f) - wetW) * 0.002f;
            float x[2] { dl, dr };
            // fatigue: the highs wear off
            for (int c = 0; c < 2; ++c)
            {
                lp[(size_t) c][0].tick (fc, x[c]); lp[(size_t) c][1].tick (fc, lp[(size_t) c][0].lp);
                x[c] += wetW * (lp[(size_t) c][1].lp - x[c]);
            }
            // cavitation: bubbles pop, more of them the harder the overload
            if (over > 0 && rng.uni() < fat * std::min (over / 6.0f + 0.2f, 1.5f) * 50.0f / sr)
                for (auto& p : pops) if (p.amp < 1.0e-4f) { p.ph = 0; p.inc = (900.0f + 2600.0f * rng.uni()) / sr; p.amp = std::sqrt (ms) * (0.3f + 0.5f * rng.uni()); p.dec = std::exp (-1.0f / ((0.0006f + 0.002f * rng.uni()) * sr)); p.pan = rng.uni(); break; }
            for (auto& p : pops)
                if (p.amp >= 1.0e-4f) { const float v = std::sin (twoPi * p.ph) * p.amp; p.ph += p.inc; p.ph -= std::floor (p.ph); p.inc *= 1.0004f; p.amp *= p.dec; x[0] += v * (1.0f - p.pan) * gateG; x[1] += v * p.pan * gateG; }
            // starving: a subsonic sediment and a phase that cannot hold still
            if (gateG > 0)
            {
                const float nz = rng.bi(), nl = nz + 0.4f * rng.bi(), nr = nz + 0.4f * rng.bi();
                const float rl = rumDc[0].tick (rum[0][1].lp (rum[0][0].lp (nl))), rr = rumDc[1].tick (rum[1][1].lp (rum[1][0].lp (nr)));
                x[0] += rl * 2.2f * starve * gateG; x[1] += rr * 2.2f * starve * gateG;
            }
            if (rng.uni() < 2.0f / sr) { wobT[0] = rng.bi(); wobT[1] = rng.bi(); }
            for (int c = 0; c < 2; ++c)
            {
                wob[c] += (wobT[c] - wob[c]) * (0.6f / sr);
                auto& a = ap[(size_t) c];
                if (starve > 1.0e-4f)
                {
                    const float k = 1.0f - starve * (0.5f + 0.45f * wob[c]);   // 1 = no phase shift
                    const float y = k * x[c] + a.x1 - k * a.y1; a.x1 = x[c]; a.y1 = y; x[c] = y;
                }
                else a.x1 = a.y1 = x[c];
            }
            L[i] = dl + (x[0] - dl) * mix; R[i] = dr + (x[1] - dr) * mix;
        }
        st.mFatigue = fat; st.mStarve = starve; st.mLevel = lvlDb;
    }
private:
    struct Pop { float ph = 0, inc = 0, amp = 0, dec = 0, pan = 0.5f; };
    struct Ap { float x1 = 0, y1 = 0; };
    float sr = 44100, ms = 0, fat = 0, starve = 0, gateG = 0, aMs = 0, wetW = 0, wob[2] {}, wobT[2] {};
    int fltCount = 0; SvfCoef fc; Rng rng;
    std::array<std::array<SvfState, 2>, 2> lp; std::array<std::array<OnePole, 2>, 2> rum; std::array<DcBlock, 2> rumDc;
    std::array<Pop, 8> pops; std::array<Ap, 2> ap;
};
} // namespace kk::pro
