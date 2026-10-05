#pragma once
#include "FxPro.h"
#include <string>
#include <cstring>
#include <vector>

// v0.45 EVOLVE FX PRO WORLDS - the twins of the EVOLVE worlds, played with gestures (no knobs, no numbers):
//   CLUB      (PARTY twin)  - your track plays in a club: disco ball = sweep, strobe = gate / stutter, crowd = build-up, how full = space, DROP lever
//   SEASONING (COOK twin)   - shake salt, pepper, chilli, sugar, ice, smoke over a finished track
//   ENGINE    (GARAGE twin) - the track runs through an engine: REV = drive, GEAR = character, EXHAUST = tone, TURBO = boost + whistle
//   MOOD WORDS (WORDS twin) - type how it should feel, the FX PRO modules arrange themselves
//   DRAW      - a drawn curve over 1 / 2 / 4 / 8 bars automates one target (filter, space, drive, width, volume)
// Every module: a state of atomics (the page writes, the audio thread reads), a DSP class that never allocates in process(),
// and off = the signal passes bit for bit.
namespace kk::pro
{
// a small 4-line FDN shared by the worlds (allocates only in prepare)
struct MiniVerb
{
    std::array<DelayLine, 4> d; float len[4] {}, g[4] {}, lp[4] {}, damp = 0.3f, sr = 44100;
    void prepare (double rate) { sr = (float) rate; for (auto& x : d) x.prepare ((int) (rate * 0.2) + 8); for (auto& v : lp) v = 0; set (1.0f, 0.3f, 1.0f); }
    void clear() { for (auto& x : d) x.clear(); for (auto& v : lp) v = 0; }
    void set (float rt, float dampK, float size)
    {
        static const float ms[4] { 29.7f, 37.1f, 41.1f, 43.7f };
        for (int k = 0; k < 4; ++k) { len[k] = ms[k] * 0.001f * sr * std::clamp (size, 0.2f, 2.0f); g[k] = std::pow (10.0f, -3.0f * len[k] / (std::max (0.05f, rt) * sr)); }
        damp = std::clamp (dampK, 0.0f, 0.95f);
    }
    inline void tick (float inL, float inR, float& oL, float& oR)
    {
        float y[4]; for (int k = 0; k < 4; ++k) { y[k] = d[(size_t) k].read (len[k]); lp[k] += (y[k] - lp[k]) * (1.0f - damp); y[k] = lp[k]; }
        const float h0 = 0.5f * (y[0] + y[1] + y[2] + y[3]), h1 = 0.5f * (y[0] - y[1] + y[2] - y[3]), h2 = 0.5f * (y[0] + y[1] - y[2] - y[3]), h3 = 0.5f * (y[0] - y[1] - y[2] + y[3]);
        d[0].push (0.5f * inL + g[0] * h0); d[1].push (0.5f * inR + g[1] * h1); d[2].push (0.5f * inL + g[2] * h2); d[3].push (0.5f * inR + g[3] * h3);
        oL = (y[0] + y[2]) * 0.7f; oR = (y[1] + y[3]) * 0.7f;
    }
};

// ---------------------------------------------------------------- CLUB ----------------------------------------------------------------
// spin: the disco ball (0 = still .. 1 = spinning fast) = a filter + phaser sweep, faster and deeper.  strobe: held = a tempo-free gate,
// a tap = a short burst; hard strobe = the open part stutters.  crowd: hands up = a high-pass lift + a rising noise riser.
// fullness: an empty club = hollow and long, a full one = warm and tight.  lever: the DROP - pulled down = a low-pass sweep down,
// let go = it slams back open.
struct ClubState
{
    std::atomic<bool> on { false };
    std::atomic<float> spin { 0.0f }, crowd { 0.0f }, fullness { 0.6f }, lever { 0.0f }, strobeRate { 0.5f };
    std::atomic<bool> strobeHeld { false };
    std::atomic<int> strobeTaps { 0 };
    std::atomic<float> mSweep { 0 }, mStrobe { 0 }, mDrop { 0 }, mLevel { 0 };   // meters for the page
    void tapStrobe() { strobeTaps.fetch_add (1); }
    void calm() { spin = 0.0f; crowd = 0.0f; lever = 0.0f; strobeHeld = false; }
};
class ClubDsp
{
public:
    void prepare (double rate, int maxBlock)
    {
        (void) maxBlock; sr = (float) rate;
        for (auto& d : stut) d.prepare ((int) (rate * 0.3) + 8);
        verb.prepare (rate);
        for (auto& d : comb) d.prepare ((int) (rate * 0.02) + 8);
        for (auto& s : sw) s.reset(); for (auto& s : hp) s.reset(); for (auto& s : dropF) s.reset(); for (auto& s : dropF2) s.reset(); for (auto& s : warm) s.reset(); for (auto& s : rs) s.reset();
        for (auto& c : ap) for (auto& a : c) a = 0;
        spinS = crowdS = 0; fullS = 0.6f; dropS = 0; strobeEnv = 0; gEnv = 1; swPh = 0; stPh = 0; env = 0; ctr = 0; lastTaps = -1; started = false; lvl = 0;
        rng.seed (4711);
        aSm = std::exp (-1.0f / (0.05f * sr)); aDropDown = std::exp (-1.0f / (0.35f * sr)); aSlam = std::exp (-1.0f / (0.004f * sr));
        aGate = std::exp (-1.0f / (0.0015f * sr)); aStrobeRel = std::exp (-1.0f / (0.45f * sr)); aEnv = std::exp (-1.0f / (0.25f * sr));
    }
    void process (float* L, float* R, int n, ClubState& st)
    {
        if (! st.on.load()) { if (st.mLevel.load() > 0.0f) { st.mLevel = 0; st.mSweep = 0; st.mStrobe = 0; st.mDrop = 0; } started = false; return; }
        const float tSpin = std::clamp (st.spin.load(), 0.0f, 1.0f), tCrowd = std::clamp (st.crowd.load(), 0.0f, 1.0f), tFull = std::clamp (st.fullness.load(), 0.0f, 1.0f);
        const float tLever = std::clamp (st.lever.load(), 0.0f, 1.0f), rate = 4.0f + 14.0f * std::clamp (st.strobeRate.load(), 0.0f, 1.0f);
        const bool held = st.strobeHeld.load();
        const int taps = st.strobeTaps.load();
        if (! started) { started = true; spinS = tSpin; crowdS = tCrowd; fullS = tFull; dropS = tLever; lastTaps = taps; ctr = 0; verb.clear(); }
        bool burst = false;
        if (taps != lastTaps) { lastTaps = taps; burst = true; }
        float pk = 0, lfoOut = 0;
        for (int i = 0; i < n; ++i)
        {
            spinS += (tSpin - spinS) * (1 - aSm); crowdS += (tCrowd - crowdS) * (1 - aSm); fullS += (tFull - fullS) * (1 - aSm);
            dropS = tLever > dropS ? aDropDown * dropS + (1 - aDropDown) * tLever : aSlam * dropS + (1 - aSlam) * tLever;
            if (burst && i == 0) strobeEnv = 1.0f;
            strobeEnv = held ? 1.0f : strobeEnv * aStrobeRel;
            if (strobeEnv < 1.0e-4f) strobeEnv = 0.0f;
            if (--ctr <= 0)
            {
                ctr = 16;
                const float hz = 0.07f + 3.2f * std::pow (spinS, 1.6f);
                swPh += hz * 16.0f / sr; swPh -= std::floor (swPh);
                lfo = 0.5f + 0.5f * std::sin (twoPi * swPh);
                swC.set (std::exp (std::log (220.0f) + (std::log (11000.0f) - std::log (220.0f)) * lfo), 0.55f, sr);
                const float t = std::tan (pi * std::min (220.0f * std::exp2 (lfo * 4.0f), sr * 0.4f) / sr); apA = (t - 1.0f) / (t + 1.0f);
                hpC.set (20.0f + 700.0f * crowdS * crowdS, 1.1f, sr);
                rsC.set (500.0f * std::exp2 (crowdS * 4.3f), 0.45f, sr);
                dropC.set (20000.0f * std::pow (0.011f, dropS), 1.3f, sr);
                warmC.set (230.0f, 1.0f, sr);
                verb.set (0.35f + 2.4f * std::pow (1.0f - fullS, 1.5f), 0.15f + 0.65f * fullS, 0.7f + 0.9f * (1.0f - fullS));
                combD = 0.0061f * sr; combFb = 0.6f * (1.0f - fullS) * (1.0f - fullS);
                wet = 0.10f + 0.26f * (1.0f - fullS);
            }
            const float dl = L[i], dr = R[i];
            float x[2] { dl, dr };
            env = aEnv * env + (1 - aEnv) * 0.5f * (dl * dl + dr * dr);
            // DISCO BALL: a resonant low-pass sweep + a phaser, both deeper with the spin
            const float depth = std::min (1.0f, spinS * 2.2f);
            if (depth > 1.0e-4f)
                for (int c = 0; c < 2; ++c)
                {
                    sw[(size_t) c].tick (swC, x[c]);
                    float y = x[c] + depth * 0.97f * (sw[(size_t) c].lp - x[c]);
                    float a = y; for (int k = 0; k < 4; ++k) { const float o = apA * a + ap[c][k]; ap[c][k] = a - apA * o; a = o; }
                    x[c] = y + depth * 0.6f * (0.5f * (y + a) - y);
                }
            // CROWD: the low end lifts out
            if (crowdS > 1.0e-4f) for (int c = 0; c < 2; ++c) { hp[(size_t) c].tick (hpC, x[c]); x[c] += std::min (1.0f, crowdS * 6.0f) * (hp[(size_t) c].hp - x[c]); }
            // FULLNESS: the room (empty = hollow comb + long tail, full = warm + tight)
            {
                float vl, vr; verb.tick (x[0], x[1], vl, vr);
                for (int c = 0; c < 2; ++c)
                {
                    const float cb = comb[(size_t) c].read (combD + (float) c * 9.0f);
                    comb[(size_t) c].push (x[c] + combFb * cb);
                    warm[(size_t) c].tick (warmC, x[c]);
                    x[c] += 0.45f * combFb * cb + 0.35f * fullS * warmC.k * warm[(size_t) c].bp;
                }
                x[0] += wet * vl; x[1] += wet * vr;
            }
            // CROWD: the riser (noise rising with the hands, following the music's level)
            if (crowdS > 1.0e-4f)
            {
                const float lvlN = 0.45f * std::pow (crowdS, 1.5f) * std::min (1.0f, std::sqrt (env) * 4.0f);
                rs[0].tick (rsC, rng.bi()); rs[1].tick (rsC, rng.bi());
                x[0] += lvlN * rs[0].bp; x[1] += lvlN * rs[1].bp;
            }
            // DROP: the low-pass falls with the lever and slams back open
            if (dropS > 1.0e-4f)
                for (int c = 0; c < 2; ++c)
                {
                    dropF[(size_t) c].tick (dropC, x[c]); dropF2[(size_t) c].tick (dropC, dropF[(size_t) c].lp);
                    x[c] += std::min (1.0f, dropS * 8.0f) * (dropF2[(size_t) c].lp - x[c]);
                }
            // STROBE: a tempo-free gate, at full strobe the open part stutters
            stut[0].push (x[0]); stut[1].push (x[1]);
            stPh += rate / sr; if (stPh >= 1.0f) stPh -= 1.0f;
            if (strobeEnv > 0.0f || gEnv < 0.9999f)
            {
                const float period = sr / rate, pos = stPh * period, slice = period * 0.22f;
                const float stutter = std::clamp ((strobeEnv - 0.55f) * 2.2f, 0.0f, 1.0f);
                if (stutter > 0.0f)
                {
                    const float ph = std::fmod (pos, slice), dly = pos - ph + 1.0f, fade = 0.0015f * sr;
                    const float w = std::min (1.0f, std::min (ph / fade, (slice - ph) / fade));
                    for (int c = 0; c < 2; ++c) x[c] += stutter * (stut[(size_t) c].read (dly) * w - x[c]);
                }
                const float gT = stPh < 0.45f ? 1.0f : 1.0f - 0.97f * strobeEnv;
                gEnv = aGate * gEnv + (1 - aGate) * gT;
                if (strobeEnv <= 0.0f && gEnv > 0.9999f) gEnv = 1.0f;
                x[0] *= gEnv; x[1] *= gEnv;
            }
            L[i] = x[0]; R[i] = x[1];
            pk = std::max (pk, std::max (std::abs (x[0]), std::abs (x[1])));
            lfoOut = lfo * depth;
        }
        lvl = std::max (pk, lvl * 0.9f);
        st.mLevel = lvl; st.mSweep = lfoOut; st.mStrobe = strobeEnv * (stPh < 0.45f ? 1.0f : 0.25f); st.mDrop = dropS;
    }
private:
    float sr = 44100, aSm = 0, aDropDown = 0, aSlam = 0, aGate = 0, aStrobeRel = 0, aEnv = 0;
    float spinS = 0, crowdS = 0, fullS = 0.6f, dropS = 0, strobeEnv = 0, gEnv = 1, swPh = 0, stPh = 0, lfo = 0.5f, apA = 0, env = 0, lvl = 0;
    float combD = 100, combFb = 0, wet = 0;
    int ctr = 0, lastTaps = -1; bool started = false;
    float ap[2][4] {};
    SvfCoef swC, hpC, rsC, dropC, warmC;
    std::array<SvfState, 2> sw, hp, dropF, dropF2, warm, rs;
    std::array<DelayLine, 2> stut, comb;
    MiniVerb verb; Rng rng;
};

// ---------------------------------------------------------------- SEASONING ----------------------------------------------------------------
enum Spice { spSalt, spPepper, spChilli, spSugar, spIce, spSmoke, numSpices };
inline const char* spiceName (int s) { static const char* n[] { "SALT", "PEPPER", "CHILLI", "SUGAR", "ICE", "SMOKE" }; return n[std::clamp (s, 0, (int) numSpices - 1)]; }
inline const char* spiceDoes (int s) { static const char* n[] { "brightness + air", "grit + saturation", "aggression: bite + drive", "shine: chorus + sparkle", "space: a cool, bright tail", "darkness + lo-fi" }; return n[std::clamp (s, 0, (int) numSpices - 1)]; }
struct SeasonState
{
    std::atomic<bool> on { false };
    std::array<std::atomic<float>, numSpices> dose {};   // 0..1 how much of each is on the plate
    std::atomic<float> mLevel { 0 };
    SeasonState() { clean(); }
    void clean() { for (auto& d : dose) d = 0.0f; }
    void add (int s, float amount) { auto& d = dose[(size_t) std::clamp (s, 0, (int) numSpices - 1)]; d = std::clamp (d.load() + amount, 0.0f, 1.0f); }
};
class SeasonDsp
{
public:
    void prepare (double rate, int maxBlock)
    {
        (void) maxBlock; sr = (float) rate;
        verb.prepare (rate);
        for (auto& d : cho) d.prepare ((int) (rate * 0.04) + 8);
        for (auto& s : salt) s.reset(); for (auto& s : air) s.reset(); for (auto& s : exc) s.reset(); for (auto& s : smk) s.reset(); for (auto& s : smk2) s.reset(); for (auto& s : iceHp) s.reset();
        for (auto& d : dc) { d = {}; d.setHz (10.0f, sr); }
        cur.fill (0); envF = envS = 0; holdN = 0; held[0] = held[1] = 0; chPh[0] = 0; chPh[1] = 0.37f; ctr = 0; lvl = 0; iceWas = false;
        aSm = std::exp (-1.0f / (0.06f * sr)); aF = std::exp (-1.0f / (0.0015f * sr)); aS = std::exp (-1.0f / (0.05f * sr));
        saltC.set (5200.0f, 0.8f, sr); airC.set (12500.0f, 1.0f, sr); excC.set (3200.0f, 0.9f, sr); iceHpC.set (380.0f, 1.0f, sr);
    }
    void process (float* L, float* R, int n, SeasonState& st)
    {
        if (! st.on.load()) { if (st.mLevel.load() > 0.0f) st.mLevel = 0; return; }
        float tgt[numSpices]; for (int s = 0; s < numSpices; ++s) tgt[s] = std::clamp (st.dose[(size_t) s].load(), 0.0f, 1.0f);
        float pk = 0;
        for (int i = 0; i < n; ++i)
        {
            for (int s = 0; s < numSpices; ++s) { cur[(size_t) s] += (tgt[s] - cur[(size_t) s]) * (1 - aSm); if (tgt[s] == 0.0f && cur[(size_t) s] < 1.0e-5f) cur[(size_t) s] = 0.0f; }
            const float salt_ = cur[spSalt], pep = cur[spPepper], chi = cur[spChilli], sug = cur[spSugar], ice = cur[spIce], smo = cur[spSmoke];
            if (--ctr <= 0) { ctr = 16; smkC.set (20000.0f * std::pow (0.075f, smo), 0.9f, sr); if (ice > 1.0e-4f) verb.set (0.9f + 4.2f * ice, 0.05f, 1.5f); }
            const float dl = L[i], dr = R[i];
            float x[2] { dl, dr };
            // CHILLI: the hits bite harder, then a hot drive
            if (chi > 1.0e-4f)
            {
                const float a = 0.5f * (std::abs (dl) + std::abs (dr));
                envF = aF * envF + (1 - aF) * a; envS = aS * envS + (1 - aS) * a;
                const float onset = std::clamp (envF / (envS + 1.0e-5f) - 1.0f, 0.0f, 2.0f);
                const float tg = 1.0f + 1.6f * chi * onset, drv = 1.0f + 5.0f * chi;
                for (int c = 0; c < 2; ++c) { const float v = x[c] * tg; x[c] = v + chi * 0.7f * (fastTanh (drv * v) / std::sqrt (drv) - v); }
            }
            // PEPPER: grit (an asymmetric saturation, DC removed)
            if (pep > 1.0e-4f)
            {
                const float drv = 1.0f + 14.0f * pep, b = 0.15f * pep;
                for (int c = 0; c < 2; ++c)
                {
                    const float sat = (fastTanh (drv * (x[c] + b)) - fastTanh (drv * b)) / std::pow (drv, 0.55f);
                    x[c] = dc[(size_t) c].tick (x[c] + pep * (sat - x[c]));
                }
            }
            // SMOKE: dark and lo-fi
            if (smo > 1.0e-4f)
            {
                if (--holdN <= 0) { holdN = 1 + (int) (smo * 5.0f); const float lv = std::exp2 (14.0f - 8.0f * smo); held[0] = std::round (x[0] * lv) / lv; held[1] = std::round (x[1] * lv) / lv; }
                for (int c = 0; c < 2; ++c)
                {
                    const float v = x[c] + 0.6f * smo * (held[c] - x[c]);
                    smk[(size_t) c].tick (smkC, v); smk2[(size_t) c].tick (smkC, smk[(size_t) c].lp);
                    x[c] = v + std::min (1.0f, smo * 4.0f) * (smk2[(size_t) c].lp - v);
                }
            }
            // SALT: brightness + air
            if (salt_ > 1.0e-4f)
            {
                const float g = std::pow (10.0f, 10.0f * salt_ / 20.0f) - 1.0f;
                for (int c = 0; c < 2; ++c) { salt[(size_t) c].tick (saltC, x[c]); air[(size_t) c].tick (airC, x[c]); x[c] += g * salt[(size_t) c].hp + 0.6f * salt_ * airC.k * air[(size_t) c].bp; }
            }
            // SUGAR: shine - an exciter and a slow chorus that opens the sides
            if (sug > 1.0e-4f)
            {
                for (int k = 0; k < 2; ++k) { chPh[k] += (0.55f + 0.27f * (float) k) / sr; chPh[k] -= std::floor (chPh[k]); }
                const float d0 = (0.011f + 0.0022f * std::sin (twoPi * chPh[0])) * sr, d1 = (0.015f + 0.0022f * std::sin (twoPi * chPh[1])) * sr;
                float ch[2];
                for (int c = 0; c < 2; ++c) { cho[(size_t) c].push (x[c]); ch[c] = cho[(size_t) c].read (c == 0 ? d0 : d1); }
                for (int c = 0; c < 2; ++c)
                {
                    exc[(size_t) c].tick (excC, x[c]);
                    x[c] += sug * (0.35f * fastTanh (4.0f * exc[(size_t) c].hp) + 0.4f * (ch[c] - x[c]) + 0.25f * ch[1 - c]);
                }
            }
            // ICE: a cool, bright tail
            if (ice > 1.0e-4f)
            {
                if (! iceWas) { verb.clear(); iceWas = true; }
                float il = x[0], ir = x[1];
                iceHp[0].tick (iceHpC, il); iceHp[1].tick (iceHpC, ir);
                float vl, vr; verb.tick (iceHp[0].hp, iceHp[1].hp, vl, vr);
                x[0] += 0.55f * ice * vl; x[1] += 0.55f * ice * vr;
            }
            else iceWas = false;
            L[i] = x[0]; R[i] = x[1];
            pk = std::max (pk, std::max (std::abs (x[0]), std::abs (x[1])));
        }
        lvl = std::max (pk, lvl * 0.9f); st.mLevel = lvl;
    }
private:
    float sr = 44100, aSm = 0, aF = 0, aS = 0, envF = 0, envS = 0, held[2] {}, chPh[2] {}, lvl = 0;
    int holdN = 0, ctr = 0; bool iceWas = false;
    std::array<float, numSpices> cur {};
    SvfCoef saltC, airC, excC, smkC, iceHpC;
    std::array<SvfState, 2> salt, air, exc, smk, smk2, iceHp;
    std::array<DcBlock, 2> dc; std::array<DelayLine, 2> cho; MiniVerb verb;
};

// ---------------------------------------------------------------- ENGINE ----------------------------------------------------------------
enum EngineGear { egTube, egTape, egDiode, egFuzz, egCrush, egFold, numGears };
inline const char* gearName (int g) { static const char* n[] { "TUBE", "TAPE", "DIODE", "FUZZ", "CRUSH", "FOLD" }; return n[std::clamp (g, 0, (int) numGears - 1)]; }
enum EngineExhaust { exStock, exSport, exStraight, exRumble, numExhausts };
inline const char* exhaustName (int e) { static const char* n[] { "STOCK", "SPORT", "STRAIGHT PIPE", "RUMBLE" }; return n[std::clamp (e, 0, (int) numExhausts - 1)]; }
struct EngineState
{
    std::atomic<bool> on { false };
    std::atomic<float> rev { 0.0f };          // 0 idle .. 1 redline = the drive
    std::atomic<int> gear { egTube }, exhaust { exStock };
    std::atomic<bool> turbo { false };        // held = boost + a gentle whistle
    std::atomic<float> mRpm { 0 }, mTurbo { 0 }, mLevel { 0 };
};
class EngineDsp
{
public:
    void prepare (double rate, int maxBlock)
    {
        (void) maxBlock; sr = (float) rate;
        for (auto& d : dc) { d = {}; d.setHz (8.0f, sr); }
        for (auto& s : tapeLp) s.reset(); for (auto& s : eqA) s.reset(); for (auto& s : eqB) s.reset();
        revS = 0; turboS = 0; envIn = envOut = 1.0e-6f; comp = 1; holdN = 0; held[0] = held[1] = 0; whPh = 0; ctr = 0; started = false; lvl = 0; lastExh = -1;
        aSm = std::exp (-1.0f / (0.04f * sr)); aTUp = std::exp (-1.0f / (0.3f * sr)); aTDown = std::exp (-1.0f / (0.6f * sr)); aEnv = std::exp (-1.0f / (0.15f * sr));
        tapeC.set (9000.0f, 1.0f, sr);
    }
    // the gear's shaper: v is the driven sample, rv the rev (for CRUSH)
    static float shape (int gear, float v, float rv)
    {
        switch (gear)
        {
            case egTube:  return fastTanh (v + 0.35f) - fastTanh (0.35f);
            case egTape:  return v / (1.0f + std::abs (v));
            case egDiode: return v / std::sqrt (std::sqrt (1.0f + v * v * v * v));
            case egFuzz:  { const float a = std::abs (4.0f * v); return (v < 0 ? -1.0f : 1.0f) * (1.0f - std::exp (-a)); }
            case egCrush: { const float lv = std::exp2 (9.0f - 6.0f * rv); return std::round (fastTanh (v) * lv) / lv; }
            default:      return std::sin (v * 1.3f);
        }
    }
    void process (float* L, float* R, int n, EngineState& st)
    {
        if (! st.on.load()) { if (st.mLevel.load() > 0.0f) { st.mLevel = 0; st.mRpm = 0; st.mTurbo = 0; } started = false; return; }
        const float tRev = std::clamp (st.rev.load(), 0.0f, 1.0f);
        const int gear = std::clamp (st.gear.load(), 0, (int) numGears - 1), exh = std::clamp (st.exhaust.load(), 0, (int) numExhausts - 1);
        const bool turbo = st.turbo.load();
        if (! started) { started = true; revS = tRev; ctr = 0; }
        if (exh != lastExh) { lastExh = exh; for (auto& s : eqA) s.reset(); for (auto& s : eqB) s.reset(); ctr = 0; }
        float pk = 0;
        for (int i = 0; i < n; ++i)
        {
            revS += (tRev - revS) * (1 - aSm);
            turboS = turbo ? aTUp * turboS + (1 - aTUp) : aTDown * turboS;
            if (turboS < 1.0e-5f) turboS = 0.0f;
            if (--ctr <= 0)
            {
                ctr = 16;
                if (exh == exSport) { eqAC.set (1600.0f, 0.7f, sr); }
                else if (exh == exStraight) { eqAC.set (140.0f, 1.2f, sr); eqBC.set (3500.0f, 0.9f, sr); }
                else if (exh == exRumble) { eqAC.set (110.0f, 1.0f, sr); eqBC.set (4800.0f, 1.0f, sr); }
            }
            const float dl = L[i], dr = R[i];
            const float drv = std::pow (10.0f, (32.0f * revS + 9.0f * turboS) / 20.0f);
            const float wet = std::min (1.0f, revS * 5.0f + turboS);
            float x[2] { dl, dr };
            envIn = aEnv * envIn + (1 - aEnv) * 0.5f * (dl * dl + dr * dr);
            if (wet > 0.0f)
            {
                float y[2];
                for (int c = 0; c < 2; ++c)
                {
                    float v = shape (gear, x[c] * drv, revS);
                    if (gear == egTape) { tapeLp[(size_t) c].tick (tapeC, v); v = tapeLp[(size_t) c].lp; }
                    y[c] = dc[(size_t) c].tick (v);
                }
                if (gear == egCrush) { if (--holdN <= 0) { holdN = 1 + (int) (revS * 9.0f); held[0] = y[0]; held[1] = y[1]; } y[0] = held[0]; y[1] = held[1]; }
                // the level stays where it was: the drive changes the character, not the loudness
                envOut = aEnv * envOut + (1 - aEnv) * 0.5f * (y[0] * y[0] + y[1] * y[1]);
                const float want = std::clamp (std::sqrt ((envIn + 1.0e-9f) / (envOut + 1.0e-9f)), 0.05f, 1.0f);
                comp += (want - comp) * 0.002f;
                for (int c = 0; c < 2; ++c) x[c] += wet * (y[c] * comp - x[c]);
            }
            // EXHAUST: the tone of the pipe
            for (int c = 0; c < 2; ++c)
            {
                if (exh == exSport) { eqA[(size_t) c].tick (eqAC, x[c]); x[c] += 0.75f * eqAC.k * eqA[(size_t) c].bp; }
                else if (exh == exStraight) { eqA[(size_t) c].tick (eqAC, x[c]); const float h = eqA[(size_t) c].hp; eqB[(size_t) c].tick (eqBC, h); x[c] = h + 0.8f * eqB[(size_t) c].hp; }
                else if (exh == exRumble) { eqA[(size_t) c].tick (eqAC, x[c]); const float v = x[c] + 1.0f * eqA[(size_t) c].lp; eqB[(size_t) c].tick (eqBC, v); x[c] = 0.8f * eqB[(size_t) c].lp; }
            }
            // TURBO: a gentle whistle that follows the music
            if (turboS > 0.0f)
            {
                whPh += (2200.0f + 2600.0f * turboS * (0.5f + 0.5f * revS)) / sr; whPh -= std::floor (whPh);
                const float w = 0.03f * turboS * std::min (1.0f, std::sqrt (envIn) * 3.0f) * std::sin (twoPi * whPh);
                x[0] += w; x[1] += w;
            }
            // a soft ceiling: above full scale the engine bends, it never explodes
            for (auto& v : x) if (std::abs (v) > 1.0f) v = (v < 0 ? -1.0f : 1.0f) * (1.0f + 0.25f * fastTanh ((std::abs (v) - 1.0f) * 4.0f));
            L[i] = x[0]; R[i] = x[1];
            pk = std::max (pk, std::max (std::abs (x[0]), std::abs (x[1])));
        }
        lvl = std::max (pk, lvl * 0.9f);
        st.mLevel = lvl; st.mRpm = revS; st.mTurbo = turboS;
    }
private:
    float sr = 44100, aSm = 0, aTUp = 0, aTDown = 0, aEnv = 0, revS = 0, turboS = 0, envIn = 0, envOut = 0, comp = 1, held[2] {}, whPh = 0, lvl = 0;
    int holdN = 0, ctr = 0, lastExh = -1; bool started = false;
    SvfCoef tapeC, eqAC, eqBC;
    std::array<SvfState, 2> tapeLp, eqA, eqB; std::array<DcBlock, 2> dc;
};

// ---------------------------------------------------------------- DRAW (automation) ----------------------------------------------------------------
// A curve drawn over 1 / 2 / 4 / 8 bars moves one target.  It follows FL's play head; when FL stops it keeps running on its own clock.
enum DrawTarget { dtFilter, dtSpace, dtDrive, dtWidth, dtVolume, numDrawTargets };
inline const char* drawTargetName (int t) { static const char* n[] { "FILTER", "SPACE", "DRIVE", "WIDTH", "VOLUME" }; return n[std::clamp (t, 0, (int) numDrawTargets - 1)]; }
inline const char* drawTargetDoes (int t) { static const char* n[] { "low = closed and dark, high = open", "low = dry, high = deep in a hall", "low = clean, high = burning",
                                                                     "low = mono, middle = as it is, high = extra wide", "low = silent, high = full" }; return n[std::clamp (t, 0, (int) numDrawTargets - 1)]; }
struct DrawAutoState
{
    static constexpr int numPoints = 128;
    std::atomic<bool> on { false };
    std::atomic<int> target { dtFilter }, bars { 2 };
    std::array<std::atomic<float>, numPoints> curve {};
    std::atomic<float> mPos { 0 }, mValue { 0 };
    DrawAutoState() { shape (0); }
    static int barsFor (int i) { static const int b[] { 1, 2, 4, 8 }; return b[std::clamp (i, 0, 3)]; }
    void shape (int which)   // ready shapes: 0 build-up ramp, 1 a wave, 2 a fall, 3 steps
    {
        for (int k = 0; k < numPoints; ++k)
        {
            const float u = (float) k / (float) (numPoints - 1);
            float v = which == 0 ? 0.15f + 0.85f * u * u : which == 1 ? 0.5f + 0.42f * std::sin (twoPi * 2.0f * u) : which == 2 ? 1.0f - 0.85f * std::sqrt (u) : (float) ((int) (u * 4.0f) % 2 == 0 ? 0.85f : 0.3f);
            curve[(size_t) k] = std::clamp (v, 0.0f, 1.0f);
        }
    }
    float at (float phase) const
    {
        const float p = std::clamp (phase, 0.0f, 1.0f) * (float) (numPoints - 1);
        const int i = std::min ((int) p, numPoints - 2);
        const float a = curve[(size_t) i].load(), b = curve[(size_t) i + 1].load();
        return a + (b - a) * (p - (float) i);
    }
};
class DrawAutoDsp
{
public:
    void prepare (double rate, int maxBlock)
    {
        (void) maxBlock; sr = (float) rate; verb.prepare (rate); verb.set (2.2f, 0.35f, 1.3f);
        for (auto& s : lp) s.reset(); for (auto& s : lp2) s.reset();
        vS = 0; ctr = 0; started = false; lastTarget = -1;
        aSm = std::exp (-1.0f / (0.004f * sr));
    }
    static double phaseOf (double beat, int bars) { const double len = 4.0 * (double) std::max (1, bars); const double p = std::fmod (beat, len) / len; return p < 0 ? p + 1.0 : p; }
    void process (float* L, float* R, int n, DrawAutoState& st, double beatPos, double bps)
    {
        if (! st.on.load()) { started = false; return; }
        const int tgt = std::clamp (st.target.load(), 0, (int) numDrawTargets - 1), bars = std::clamp (st.bars.load(), 1, 8);
        if (tgt != lastTarget) { lastTarget = tgt; verb.clear(); for (auto& s : lp) s.reset(); for (auto& s : lp2) s.reset(); ctr = 0; }
        if (! started) { started = true; vS = st.at ((float) phaseOf (beatPos, bars)); ctr = 0; }
        float ph = 0;
        for (int i = 0; i < n; ++i)
        {
            ph = (float) phaseOf (beatPos + bps * (double) i, bars);
            vS = aSm * vS + (1 - aSm) * st.at (ph);
            const float v = std::clamp (vS, 0.0f, 1.0f);
            if (--ctr <= 0) { ctr = 16; if (tgt == dtFilter) fc.set (60.0f * std::exp (std::log (20000.0f / 60.0f) * v), 0.9f, sr); }
            float x[2] { L[i], R[i] };
            switch (tgt)
            {
                case dtFilter: for (int c = 0; c < 2; ++c) { lp[(size_t) c].tick (fc, x[c]); lp2[(size_t) c].tick (fc, lp[(size_t) c].lp); x[c] = lp2[(size_t) c].lp; } break;
                case dtSpace:  { float vl, vr; verb.tick (x[0], x[1], vl, vr); x[0] += 0.7f * v * vl; x[1] += 0.7f * v * vr; } break;
                case dtDrive:  { const float drv = 1.0f + 24.0f * v * v; for (int c = 0; c < 2; ++c) x[c] += std::min (1.0f, v * 4.0f) * (fastTanh (drv * x[c]) / std::sqrt (drv) - x[c]); } break;
                case dtWidth:  { const float m = 0.5f * (x[0] + x[1]), s = 0.5f * (x[0] - x[1]) * 2.0f * v; x[0] = m + s; x[1] = m - s; } break;
                default:       { const float g = v * v; x[0] *= g; x[1] *= g; } break;
            }
            L[i] = x[0]; R[i] = x[1];
        }
        st.mPos = ph; st.mValue = vS;
    }
private:
    float sr = 44100, aSm = 0, vS = 0; int ctr = 0, lastTarget = -1; bool started = false;
    SvfCoef fc; std::array<SvfState, 2> lp, lp2; MiniVerb verb;
};

// ---------------------------------------------------------------- MOOD WORDS ----------------------------------------------------------------
// Type how the track should feel.  A built-in dictionary (EN + CZ, no diacritics needed) turns meaning words into a mood vector;
// any other word is hashed - the same word always gives the same mood.  The vector then arranges the FX PRO modules.
enum MoodDim { mdDark, mdBright, mdSpace, mdGrit, mdLofi, mdWobble, mdAnger, mdCalm, mdWater, mdDance, mdSweet, mdCold, mdWarm, mdOld, mdAlien, mdFar, mdNear, mdGlitch, numMoodDims };
enum MoodModule { mmHolo, mmIntent, mmDial, mmWarp, mmLiquid, mmErosion, mmClub, mmSeason, mmEngine, numMoodModules };
inline const char* moodModuleName (int m) { static const char* n[] { "HOLOROOM", "INTENT", "DIAL-UP", "WARP DRIVE", "LIQUID", "EROSION", "CLUB", "SEASONING", "ENGINE" }; return n[std::clamp (m, 0, (int) numMoodModules - 1)]; }
struct MoodWord { const char* w; const char* code; };   // code: letter = dimension, digit = weight / 9
// D dark  B bright  S space  G grit  L lofi  W wobble  A anger  C calm  U underwater  P party/dance  Y sweet  I icy  H warm  O old  X alien  F far  N near  Z glitch
inline const std::vector<MoodWord>& moodDictionary()
{
    static const std::vector<MoodWord> d {
        // EN
        { "night", "D6S4C2F2" }, { "drive", "G3P3H2N2" }, { "underwater", "U9D5F4" }, { "water", "U6S2" }, { "ocean", "U6S6F3" }, { "angry", "A9G5" }, { "rage", "A9G7Z2" },
        { "dreamy", "C6S7Y4W2" }, { "dream", "C5S6Y3W2" }, { "dark", "D9" }, { "bright", "B9" }, { "shiny", "B6Y6" }, { "warm", "H8" }, { "cold", "I8B2" }, { "icy", "I9B3" },
        { "ice", "I9" }, { "space", "S9F4" }, { "spacey", "S8X3F3" }, { "far", "F9S3" }, { "distant", "F8S4" }, { "close", "N9" }, { "intimate", "N9C3" }, { "club", "P9S3" },
        { "dance", "P9" }, { "party", "P9B3" }, { "rave", "P8A3Z3" }, { "phone", "O7L5" }, { "radio", "O7L4" }, { "old", "O8L4" }, { "vintage", "O8H4L3" }, { "retro", "O7L4" },
        { "lofi", "L9O4D3" }, { "dirty", "G8L3" }, { "gritty", "G9" }, { "grit", "G9" }, { "clean", "B4C3" }, { "soft", "C6H3" }, { "sweet", "Y9" }, { "sugar", "Y9" },
        { "sad", "D6C4S4" }, { "happy", "B6Y5P3" }, { "euphoric", "P7B6S5Y4" }, { "chill", "C8H3" }, { "calm", "C9" }, { "aggressive", "A9G6" }, { "heavy", "D5G6A4" },
        { "punchy", "A5N4" }, { "wide", "S5B2" }, { "small", "N6" }, { "huge", "S8F4" }, { "big", "S6" }, { "cathedral", "S9F6" }, { "cave", "S7D6F4" }, { "alien", "X9W3" },
        { "robot", "X7O3L3" }, { "future", "X7B4" }, { "glitch", "Z9L3" }, { "broken", "Z7L5O3" }, { "tired", "L5D4C4" }, { "rain", "U4C4D3S3" }, { "fire", "A6H8G4" },
        { "smoke", "D6L5" }, { "smoky", "D6L5" }, { "spicy", "A7G4" }, { "hot", "H7A4" }, { "wobble", "W9" }, { "drunk", "W8L3" }, { "sleepy", "C7D4L3" }, { "ghost", "F6S7D4W3" },
        { "haunted", "D7S7W4" }, { "metal", "G7B4" }, { "engine", "G7A5" }, { "fast", "P5A4" }, { "slow", "C6D3" }, { "deep", "D6S5" }, { "airy", "B7S4" }, { "sky", "S7B5F4" },
        { "storm", "A6D5S5Z3" }, { "love", "H7Y6C3" }, { "tape", "O6H5L3" }, { "vinyl", "O7L5H3" }, { "neon", "B6P5X3" }, { "techno", "P8D4X2" }, { "house", "P8H4" },
        { "disco", "P9Y4B3" }, { "trap", "D5G4A3" }, { "ambient", "S8C7F4" }, { "cinematic", "S8F5D3" }, { "summer", "H7B5Y4" }, { "winter", "I8C3" }, { "frozen", "I9S3" },
        // CZ (written without diacritics; the page folds them away)
        { "noc", "D6S4C2F2" }, { "nocni", "D6S4C2F2" }, { "jizda", "G3P3H2N2" }, { "voda", "U6S2" }, { "vodou", "U9D5F4" }, { "pod", "U2" }, { "more", "U6S6F3" },
        { "nastvany", "A9G5" }, { "nastvana", "A9G5" }, { "zly", "A8G4" }, { "vztek", "A9G7Z2" }, { "snovy", "C6S7Y4W2" }, { "sen", "C5S6Y3W2" }, { "temny", "D9" }, { "tma", "D9" },
        { "tmavy", "D8" }, { "svetly", "B9" }, { "jasny", "B8" }, { "teply", "H8" }, { "tepla", "H8" }, { "studeny", "I8B2" }, { "led", "I9" }, { "ledovy", "I9B3" },
        { "vesmir", "S9F4X2" }, { "daleko", "F9S3" }, { "blizko", "N9" }, { "klub", "P9S3" }, { "tanec", "P9" }, { "party", "P9B3" }, { "parba", "P9A2" }, { "telefon", "O7L5" },
        { "stary", "O8L4" }, { "stara", "O8L4" }, { "spinavy", "G8L3" }, { "cisty", "B4C3" }, { "mekky", "C6H3" }, { "sladky", "Y9" }, { "cukr", "Y9" }, { "smutny", "D6C4S4" },
        { "vesely", "B6Y5P3" }, { "klidny", "C9" }, { "klid", "C9" }, { "agresivni", "A9G6" }, { "tezky", "D5G6A4" }, { "siroky", "S5B2" }, { "maly", "N6" }, { "velky", "S6" },
        { "obrovsky", "S8F4" }, { "jeskyne", "S7D6F4" }, { "mimozemsky", "X9W3" }, { "budoucnost", "X7B4" }, { "rozbity", "Z7L5O3" }, { "unaveny", "L5D4C4" }, { "dest", "U4C4D3S3" },
        { "ohen", "A6H8G4" }, { "kour", "D6L5" }, { "ostry", "A7G4" }, { "palivy", "A7G4" }, { "horky", "H7A4" }, { "opily", "W8L3" }, { "ospaly", "C7D4L3" },
        { "duch", "F6S7D4W3" }, { "kov", "G7B4" }, { "motor", "G7A5" }, { "rychly", "P5A4" }, { "pomaly", "C6D3" }, { "hluboky", "D6S5" }, { "nebe", "S7B5F4" }, { "bourka", "A6D5S5Z3" },
        { "laska", "H7Y6C3" }, { "leto", "H7B5Y4" }, { "zima", "I8C3" }, { "zmrzly", "I9S3" }, { "kazeta", "O6H5L3" }, { "deska", "O7L5H3" },
    };
    return d;
}
inline bool moodStopWord (const std::string& w)
{
    static const char* s[] { "a", "an", "the", "and", "of", "in", "on", "at", "like", "very", "so", "my", "it", "is", "to", "with", "for", "but", "or", "too", "bit", "little",
                             "je", "na", "v", "ve", "se", "jak", "moc", "hodne", "velmi", "trochu", "s", "z", "do", "i", "nebo", "ale", "to" };
    for (auto* x : s) if (w == x) return true;
    return false;
}
// lower case + Czech diacritics folded away (UTF-8): "Noční JÍZDA" -> "nocni jizda"
inline std::string moodFold (const std::string& in)
{
    static const char* map[][2] { { "\xc3\xa1", "a" }, { "\xc4\x8d", "c" }, { "\xc4\x8f", "d" }, { "\xc3\xa9", "e" }, { "\xc4\x9b", "e" }, { "\xc3\xad", "i" }, { "\xc5\x88", "n" }, { "\xc3\xb3", "o" },
                                  { "\xc5\x99", "r" }, { "\xc5\xa1", "s" }, { "\xc5\xa5", "t" }, { "\xc3\xba", "u" }, { "\xc5\xaf", "u" }, { "\xc3\xbd", "y" }, { "\xc5\xbe", "z" },
                                  { "\xc3\x81", "a" }, { "\xc4\x8c", "c" }, { "\xc4\x8e", "d" }, { "\xc3\x89", "e" }, { "\xc4\x9a", "e" }, { "\xc3\x8d", "i" }, { "\xc5\x87", "n" }, { "\xc3\x93", "o" },
                                  { "\xc5\x98", "r" }, { "\xc5\xa0", "s" }, { "\xc5\xa4", "t" }, { "\xc3\x9a", "u" }, { "\xc5\xae", "u" }, { "\xc3\x9d", "y" }, { "\xc5\xbd", "z" } };
    std::string out; out.reserve (in.size());
    for (size_t i = 0; i < in.size();)
    {
        bool hit = false;
        if ((unsigned char) in[i] >= 0x80 && i + 1 < in.size())
            for (auto& m : map) if (in[i] == m[0][0] && in[i + 1] == m[0][1]) { out += m[1]; i += 2; hit = true; break; }
        if (hit) continue;
        const char c = in[i++];
        out += (c >= 'A' && c <= 'Z') ? (char) (c - 'A' + 'a') : c;
    }
    return out;
}
inline uint32_t moodHash (const std::string& w) { uint32_t h = 2166136261u; for (unsigned char c : w) { h ^= c; h *= 16777619u; } return hash32 (h); }
struct MoodWordHit { std::string word; bool known = false; };
struct MoodResult
{
    std::array<float, numMoodDims> v {};
    std::vector<MoodWordHit> words;
    std::array<bool, numMoodModules> moved {};
    std::array<std::string, numMoodModules> how;   // a few words per module: what it did
    int count() const { int c = 0; for (bool m : moved) c += m ? 1 : 0; return c; }
};
inline int moodDimOf (char c) { static const char* L = "DBSGLWACUPYIHOXFNZ"; for (int i = 0; i < numMoodDims; ++i) if (L[i] == c) return i; return -1; }
// the reading alone (deterministic, no side effects)
inline MoodResult moodRead (const std::string& text)
{
    MoodResult r;
    const auto t = moodFold (text);
    std::string w;
    auto flush = [&r, &w]
    {
        if (w.empty()) return;
        if (! moodStopWord (w))
        {
            const MoodWord* hit = nullptr;
            for (auto& d : moodDictionary()) if (w == d.w) { hit = &d; break; }
            // plurals / endings: "dreams", "angrily" -> try without the last letters
            if (hit == nullptr && w.size() > 4) for (auto& d : moodDictionary()) { const size_t n = std::strlen (d.w); if (n >= 4 && w.compare (0, n, d.w) == 0 && w.size() - n <= 3) { hit = &d; break; } }
            if (hit != nullptr)
            {
                for (const char* c = hit->code; c[0] != 0 && c[1] != 0; c += 2) if (const int k = moodDimOf (c[0]); k >= 0) r.v[(size_t) k] += (float) (c[1] - '0') / 9.0f;
                r.words.push_back ({ w, true });
            }
            else
            {
                const uint32_t h = moodHash (w);
                r.v[(size_t) (h % numMoodDims)] += 0.35f + (float) ((h >> 16) & 7) / 20.0f;
                r.v[(size_t) ((h >> 8) % numMoodDims)] += 0.2f + (float) ((h >> 20) & 7) / 25.0f;
                r.words.push_back ({ w, false });
            }
        }
        w.clear();
    };
    for (char c : t)
    {
        const unsigned char u = (unsigned char) c;
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || u >= 0x80) w += c; else flush();
    }
    flush();
    for (auto& x : r.v) x = std::clamp (x, 0.0f, 1.0f);
    return r;
}
struct MoodTargets
{
    HoloState& holo; IntentState& intent; DialState& dial; WarpState& warp; LiquidState& liquid; ErosionState& erosion;
    ClubState& club; SeasonState& season; EngineState& engine;
};
// the arrangement: every mood module is set (or switched off) from the vector
inline MoodResult moodApply (const std::string& text, MoodTargets t)
{
    MoodResult r = moodRead (text);
    const auto& v = r.v;
    auto V = [&v] (int d) { return v[(size_t) d]; };
    auto mark = [&r] (int m, const std::string& how) { r.moved[(size_t) m] = true; r.how[(size_t) m] = how; };
    // HOLOROOM: where the track sits
    if (V (mdSpace) > 0.2f || V (mdFar) > 0.2f || V (mdNear) > 0.2f || V (mdWater) > 0.3f)
    {
        t.holo.on = true; t.holo.x = 0.0f;
        const float depth = std::clamp (0.25f + 0.65f * V (mdFar) + 0.25f * V (mdWater) - 0.25f * V (mdNear), 0.0f, 1.0f);
        t.holo.depth = depth; t.holo.room = std::clamp (0.15f + 0.8f * V (mdSpace) + 0.15f * V (mdFar) - 0.3f * V (mdNear), 0.0f, 1.0f);
        t.holo.height = std::clamp (0.8f * (V (mdBright) - V (mdDark) - 0.5f * V (mdWater)), -1.0f, 1.0f);
        mark (mmHolo, std::string (depth > 0.6f ? "far away" : depth < 0.15f ? "in your face" : "across the room") + (t.holo.room.load() > 0.6f ? ", a huge room" : t.holo.room.load() > 0.35f ? ", a mid-size room" : ", a small room"));
    }
    else t.holo.on = false;
    // INTENT: the body's state
    {
        const float sc[numIntentModes] { V (mdWater), V (mdAnger), 0.6f * V (mdGlitch) + 0.3f * V (mdAnger) * V (mdDark), V (mdCalm) };
        int best = 0; for (int k = 1; k < numIntentModes; ++k) if (sc[k] > sc[best]) best = k;
        if (sc[best] > 0.25f)
        {
            t.intent.on = true; t.intent.mode = best; t.intent.mode2 = best; t.intent.blend = 0.0f; t.intent.mix = 1.0f;
            t.intent.level = std::clamp (1.0f - 0.75f * sc[best], 0.15f, 1.0f);
            static const char* how[] { "out of breath", "aggression", "stress", "calm" };
            mark (mmIntent, how[best]);
        }
        else t.intent.on = false;
    }
    // DIAL-UP: old machines
    if (const float s = std::max (V (mdOld), 0.8f * V (mdLofi)); s > 0.4f)
    {
        const int m = V (mdAlien) > 0.4f ? dmRobotToy : V (mdGlitch) > 0.4f ? dmBadSignal : V (mdOld) >= V (mdLofi) ? dmLandline : dmVoiceNote;
        t.dial.applyMode (m); t.dial.on = true; t.dial.mix = std::clamp (0.25f + 0.5f * s, 0.0f, 1.0f);
        mark (mmDial, std::string ("an old ") + (m == dmRobotToy ? "robot toy" : m == dmBadSignal ? "line with bad signal" : m == dmLandline ? "phone line" : "voice note"));
    }
    else t.dial.on = false;
    // WARP DRIVE: other worlds
    if (V (mdAlien) > 0.3f || V (mdWobble) > 0.35f)
    {
        const int m = V (mdAlien) >= V (mdWobble) ? wmAlien : wmWobble;
        t.warp.applyMode (m); t.warp.on = true; t.warp.mix = t.warp.mix.load() * std::clamp (0.4f + 0.6f * std::max (V (mdAlien), V (mdWobble)), 0.0f, 1.0f);
        mark (mmWarp, m == wmAlien ? "alien shift" : "wobble");
    }
    else t.warp.on = false;
    // LIQUID: the dance floor's low end
    if (V (mdDance) > 0.35f) { t.liquid.on = true; t.liquid.source = lsSelf; t.liquid.flow = 0.4f + 0.4f * V (mdDance); t.liquid.depth = 0.7f; mark (mmLiquid, "the kick carves the bass"); }
    else t.liquid.on = false;
    // EROSION: worn material
    if (const float s = 0.6f * V (mdGrit) + 0.6f * V (mdOld) + 0.4f * V (mdLofi); s > 0.5f)
    {
        t.erosion.on = true; t.erosion.sensitivity = std::clamp (0.3f + 0.5f * s, 0.0f, 1.0f); t.erosion.fatigue = std::clamp (0.3f + 0.5f * V (mdGrit), 0.0f, 1.0f); t.erosion.recovery = 0.5f; t.erosion.mix = 1.0f;
        mark (mmErosion, "it wears and tires");
    }
    else t.erosion.on = false;
    // CLUB
    if (V (mdDance) > 0.25f)
    {
        t.club.on = true; t.club.calm();
        t.club.spin = std::clamp (0.2f + 0.5f * V (mdDance) + 0.3f * V (mdWobble), 0.0f, 1.0f);
        t.club.crowd = std::clamp (0.35f * V (mdDance) * (0.5f + V (mdAnger)), 0.0f, 1.0f);
        t.club.fullness = std::clamp (0.6f + 0.35f * V (mdWarm) - 0.4f * V (mdCold) - 0.3f * V (mdSpace), 0.0f, 1.0f);
        mark (mmClub, t.club.fullness.load() > 0.5f ? "a packed club" : "an empty club");
    }
    else t.club.on = false;
    // SEASONING
    {
        const float d[numSpices] { 0.8f * V (mdBright), 0.8f * V (mdGrit), 0.8f * V (mdAnger), 0.7f * V (mdSweet), 0.8f * V (mdCold) + 0.2f * V (mdSpace), 0.7f * std::max (V (mdDark), V (mdLofi)) };
        bool any = false; std::string how;
        for (int s = 0; s < numSpices; ++s)
        {
            const float q = d[s] > 0.12f ? std::clamp (d[s], 0.0f, 1.0f) : 0.0f;
            t.season.dose[(size_t) s] = q;
            if (q > 0.0f) { any = true; if (! how.empty()) how += " + "; std::string nm = spiceName (s); for (auto& ch : nm) ch = (char) (ch - 'A' + 'a'); how += nm; }
        }
        t.season.on = any;
        if (any) mark (mmSeason, how);
    }
    // ENGINE
    if (const float s = std::max (V (mdAnger), V (mdGrit)); s > 0.5f)
    {
        t.engine.on = true; t.engine.rev = std::clamp (0.2f + 0.5f * s, 0.0f, 1.0f); t.engine.turbo = false;
        t.engine.gear = V (mdLofi) > 0.4f ? egCrush : V (mdAnger) > V (mdGrit) ? egFuzz : V (mdWarm) > 0.3f || V (mdOld) > 0.3f ? egTape : egTube;
        t.engine.exhaust = V (mdDark) > 0.4f ? exRumble : V (mdBright) > 0.4f ? exStraight : exSport;
        mark (mmEngine, std::string (gearName (t.engine.gear.load())) + " gear, " + exhaustName (t.engine.exhaust.load()));
    }
    else t.engine.on = false;
    return r;
}
} // namespace kk::pro
