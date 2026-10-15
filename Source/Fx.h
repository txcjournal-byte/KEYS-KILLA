#pragma once
#include "DspUtil.h"
#include "Studio.h"
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace kk
{
enum FxSlot { fxDrive, fxBody, fxLofi, fxCircuit, fxChorus, fxPhaser, fxFlanger, fxEq, fxDelay, fxReverb, fxReverse, fxPunch, fxHalftime, numFxSlots };

inline std::array<int, numFxSlots> defaultFxOrder()
{
    return { fxPunch, fxDrive, fxBody, fxLofi, fxCircuit, fxHalftime, fxChorus, fxPhaser, fxFlanger, fxEq, fxDelay, fxReverb, fxReverse };
}

inline const char* fxSlotName (int s)
{
    static const char* n[] { "Drive", "Body Swap", "Lo-Fi", "Circuit Bend", "Chorus", "Phaser", "Flanger", "EQ", "Delay", "Reverb", "Reverse", "Punch", "Half-Time" };
    return n[std::clamp (s, 0, numFxSlots - 1)];
}

struct FxParams
{
    float drive = 0; int driveType = 0; bool cleanLow = false;
    float crush = 0, wow = 0, chorus = 0, phaser = 0, flanger = 0;
    float delayMix = 0, delayFb = 0.35f; double delayBeats = 0.75; int delayMode = 0;
    float revMix = 0.15f, revSize = 0.6f; int revType = 0; bool freeze = false;
    float eqLow = 0, eqHigh = 0, reverse = 0;
    float width = 0.5f;
    float ghost = 0, ghostBlur = 0.6f; bool ghostRev = true;
    float circuit = 0; double circBeats = 0.25;
    int body = 0; float bodyMix = 0.5f;
    bool  monoLows = false, eco = false;
    float outGain = 1.0f;
    double bpm = 120, beatPos = 0;   // beat position at block start
    uint32_t seed = 1234;
    std::array<int, numFxSlots> order = defaultFxOrder();
    float punch = 0, halftime = 0;
    float master = 0.7f;
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
    void peak (float f, float q, float gainDb, float sr)
    {
        const float A = std::pow (10.0f, gainDb / 40.0f), w = twoPi * f / sr, cs = std::cos (w), al = std::sin (w) / (2.0f * std::max (0.05f, q));
        const float A0 = 1.0f + al / A;
        b0 = (1.0f + al * A) / A0; b1 = -2.0f * cs / A0; b2 = (1.0f - al * A) / A0; a1 = -2.0f * cs / A0; a2 = (1.0f - al / A) / A0;
    }
};

// ---------------------------------------------------------------------------------
// The FX stages. FxRack (EVOLVE, EVOLVE FX PRO) runs them in its slot order; CHROME VAULT wraps each one as an
// independent chain slot (Source/vault/VaultFx.h). Every stage is allocation-free after prepare(); the extra arguments
// (rate, depth, feedback ...) default to the values FxRack always used, so FxRack sounds exactly as before.
struct DriveStage
{
    void prepare (float sampleRate, int maxBlock)
    {
        sr = sampleRate;
        os.initProcessing ((size_t) maxBlock);
        os.reset();
        low[0].assign ((size_t) maxBlock, 0.0f); low[1].assign ((size_t) maxBlock, 0.0f);
        for (auto& f : splitA) { f.setHz (140.0f, sr); f.reset(); }
        for (auto& f : splitB) { f.setHz (140.0f, sr); f.reset(); }
        smDrive = 0;
    }
    void clear() { os.reset(); for (auto& f : splitA) f.reset(); for (auto& f : splitB) f.reset(); smDrive = 0; }
    bool idle (float drive) const { return drive <= 0.001f && smDrive <= 0.001f; }
    void process (float* L, float* R, int n, float drive, int driveType, bool cleanLow, bool eco, float sm)
    {
        if (drive <= 0.001f && smDrive <= 0.001f) return;
        if (cleanLow)
            for (int i = 0; i < n; ++i)
            {
                const float lL = splitB[0].lp (splitA[0].lp (L[i]));
                const float lR = splitB[1].lp (splitA[1].lp (R[i]));
                low[0][(size_t) i] = lL; low[1][(size_t) i] = lR;
                L[i] -= lL; R[i] -= lR;
            }
        if (eco)
        {
            for (int i = 0; i < n; ++i)
            {
                smDrive += (drive - smDrive) * sm;
                L[i] = shape (L[i], smDrive, driveType); R[i] = shape (R[i], smDrive, driveType);
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
                smDrive += (drive - smDrive) * smU;
                uL[i] = shape (uL[i], smDrive, driveType);
                uR[i] = shape (uR[i], smDrive, driveType);
            }
            os.processSamplesDown (block);
        }
        if (cleanLow)
            for (int i = 0; i < n; ++i) { L[i] += low[0][(size_t) i]; R[i] += low[1][(size_t) i]; }
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
    float sr = 44100, smDrive = 0;
    juce::dsp::Oversampling<float> os { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
    std::vector<float> low[2];
    OnePole splitA[2], splitB[2];
};

struct BodyStage
{
    void prepare (float sampleRate) { sr = sampleRate; clear(); }
    void clear() { for (auto& b : bodyS) for (auto& s : b) s.reset(); smBody = 0; }
    void process (float* L, float* R, int n, int bodyType, float bodyMix, float sm)
    {
        if (bodyType <= 0 && smBody <= 0.001f) return;
        static const float modes[6][5] { { 420, 1180, 2350, 3900, 5600 }, { 520, 1250, 1680, 2750, 4100 },
                                        { 1450, 2610, 3900, 5500, 7200 }, { 310, 855, 1680, 2780, 4150 },
                                        { 180, 420, 890, 1600, 2500 },    { 110, 220, 330, 440, 660 } };
        static const float qs[6] { 30, 70, 90, 45, 8, 60 };
        const int b = std::min (5, std::max (0, bodyType - 1));
        SvfCoef c[5];
        for (int k = 0; k < 5; ++k) c[k].set (modes[b][k], 1.0f / qs[b], sr);
        const float target = bodyType > 0 ? bodyMix : 0.0f;
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
    float sr = 44100, smBody = 0;
    SvfState bodyS[2][5];
};

struct LofiStage
{
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& d : wowDl) d.prepare ((int) (0.05f * sr));
        crackle.seed (777);
        noiseEnv = 0; noiseRel = std::exp (-1.0f / (0.25f * sr));
        wowPhase = flutPhase = 0; holdCount = 0; holdL = holdR = 0;
    }
    void clear() { for (auto& d : wowDl) d.clear(); noiseEnv = 0; holdCount = 0; holdL = holdR = 0; }
    void process (float* L, float* R, int n, float crush, float wow)
    {
        if (crush <= 0.001f && wow <= 0.001f) return;
        const float levels = std::exp2 (16.0f - crush * 12.0f);
        const int hold = 1 + (int) (crush * crush * 10.0f);
        const float wIncA = 0.55f / sr, wIncB = 6.5f / sr;
        for (int i = 0; i < n; ++i)
        {
            float l = L[i], r = R[i];
            if (wow > 0.001f)
            {
                wowDl[0].push (l); wowDl[1].push (r);
                const float d = (0.004f + wow * (0.0035f * std::sin (twoPi * wowPhase) + 0.0004f * std::sin (twoPi * flutPhase))) * sr;
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
                    float noiseV = crackle.bi() * 0.002f * wow;
                    if (crackle.uni() < wow * 12.0f / sr) noiseV += crackle.bi() * 0.12f * wow;
                    l += noiseV * gate; r += noiseV * gate;
                }
            }
            if (crush > 0.001f)
            {
                if (++holdCount >= hold) { holdCount = 0; holdL = l; holdR = r; }
                l = std::round (holdL * levels) / levels; r = std::round (holdR * levels) / levels;
            }
            L[i] = l; R[i] = r;
        }
    }
    float sr = 44100, noiseEnv = 0, noiseRel = 0.9999f, wowPhase = 0, flutPhase = 0, holdL = 0, holdR = 0;
    int holdCount = 0;
    DelayLine wowDl[2];
    Rng crackle;
};

// chorus: two modulated taps (12 / 14 ms, +-4 ms at 0.33 Hz) crossed over; ensemble = three taps per side at 120 degrees
struct ChorusStage
{
    void prepare (float sampleRate) { sr = sampleRate; for (auto& d : chDl) d.prepare ((int) (0.06f * sr)); chPhase = 0; smChorus = 0; }
    void clear() { for (auto& d : chDl) d.clear(); smChorus = 0; }
    void process (float* L, float* R, int n, float amount, float sm, float rateHz = 0.33f, float depthSec = 0.004f, bool ensemble = false)
    {
        const float inc = rateHz / sr;
        for (int i = 0; i < n; ++i)
        {
            smChorus += (amount - smChorus) * sm;
            chDl[0].push (L[i]); chDl[1].push (R[i]);
            if (smChorus < 0.001f) continue;
            const float mL = std::sin (twoPi * chPhase), mR = std::sin (twoPi * (chPhase + 0.25f));
            chPhase += inc; if (chPhase >= 1) chPhase -= 1;
            float wl = chDl[0].read ((0.012f + depthSec * mL) * sr);
            float wr = chDl[1].read ((0.014f + depthSec * mR) * sr);
            if (ensemble)
            {
                const float a = std::sin (twoPi * (chPhase + 0.3333f)), b = std::sin (twoPi * (chPhase + 0.6667f));
                const float c = std::sin (twoPi * (chPhase + 0.5833f)), d = std::sin (twoPi * (chPhase + 0.9167f));
                wl = (wl + chDl[0].read ((0.0165f + depthSec * a) * sr) + chDl[0].read ((0.0095f + depthSec * b) * sr)) * 0.6f;
                wr = (wr + chDl[1].read ((0.0175f + depthSec * c) * sr) + chDl[1].read ((0.0105f + depthSec * d) * sr)) * 0.6f;
            }
            const float mix = smChorus * 0.7f;
            L[i] = L[i] * (1.0f - mix * 0.4f) + wr * mix;
            R[i] = R[i] * (1.0f - mix * 0.4f) + wl * mix;
        }
    }
    float sr = 44100, chPhase = 0, smChorus = 0;
    DelayLine chDl[2];
};

// phaser: 4 first-order all-passes per side swept over `octaves` above `baseHz`, feedback
struct PhaserStage
{
    void prepare (float sampleRate) { sr = sampleRate; phPhase = 0; smPhaser = 0; clear(); }
    void clear() { for (auto& a : apState) for (auto& v : a) v = 0; apFb[0] = apFb[1] = 0; smPhaser = 0; }
    bool idle (float amount) const { return amount <= 0.001f && smPhaser <= 0.001f; }
    void process (float* L, float* R, int n, float amount, float sm, float rateHz = 0.35f, float octaves = 3.5f, float feedback = 0.5f, float baseHz = 250.0f)
    {
        if (amount <= 0.001f && smPhaser <= 0.001f) return;
        const float inc = rateHz / sr;
        float a[2] { 0, 0 };
        for (int i = 0; i < n; ++i)
        {
            smPhaser += (amount - smPhaser) * sm;
            if ((i & 15) == 0)
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float m = 0.5f + 0.5f * std::sin (twoPi * (phPhase + ch * 0.25f));
                    const float f = std::min (baseHz * std::exp2 (m * octaves), sr * 0.45f);
                    const float t = std::tan (pi * f / sr);
                    a[ch] = (t - 1.0f) / (t + 1.0f);
                }
            phPhase += inc; if (phPhase >= 1) phPhase -= 1;
            float* io[2] { &L[i], &R[i] };
            for (int ch = 0; ch < 2; ++ch)
            {
                float x = *io[ch] + apFb[ch] * feedback * smPhaser;
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
    float sr = 44100, phPhase = 0, smPhaser = 0;
    float apState[2][4] {}, apFb[2] {};
};

struct FlangerStage
{
    void prepare (float sampleRate) { sr = sampleRate; for (auto& d : flDl) d.prepare ((int) (0.02f * sr)); flPhase = 0; smFlanger = 0; }
    void clear() { for (auto& d : flDl) d.clear(); smFlanger = 0; }
    bool idle (float amount) const { return amount <= 0.001f && smFlanger <= 0.001f; }
    void process (float* L, float* R, int n, float amount, float sm, float rateHz = 0.22f, float depthSec = 0.0045f, float feedback = 0.65f)
    {
        if (amount <= 0.001f && smFlanger <= 0.001f) { for (int i = 0; i < n; ++i) { flDl[0].push (L[i]); flDl[1].push (R[i]); } return; }
        const float inc = rateHz / sr;
        for (int i = 0; i < n; ++i)
        {
            smFlanger += (amount - smFlanger) * sm;
            const float mL = 0.5f + 0.5f * std::sin (twoPi * flPhase), mR = 0.5f + 0.5f * std::sin (twoPi * (flPhase + 0.3f));
            flPhase += inc; if (flPhase >= 1) flPhase -= 1;
            const float dl = (0.0007f + depthSec * mL) * sr, dr = (0.0007f + depthSec * mR) * sr;
            const float wl = flDl[0].read (dl), wr = flDl[1].read (dr);
            flDl[0].push (L[i] + wl * feedback * smFlanger);
            flDl[1].push (R[i] + wr * feedback * smFlanger);
            L[i] = L[i] * (1.0f - 0.3f * smFlanger) + wl * 0.6f * smFlanger;
            R[i] = R[i] * (1.0f - 0.3f * smFlanger) + wr * 0.6f * smFlanger;
        }
    }
    float sr = 44100, flPhase = 0, smFlanger = 0;
    DelayLine flDl[2];
};

// low shelf + optional mid peak + high shelf (RBJ); bypassed while every gain is under 0.05 dB
struct EqStage
{
    void prepare (float sampleRate) { sr = sampleRate; lastLow = lastHigh = lastMid = 999.0f; for (auto& e : eq) e = {}; for (auto& e : mid) e = {}; }
    void clear() { for (auto& b : eq) b.z1 = b.z2 = 0; for (auto& b : mid) b.z1 = b.z2 = 0; }
    static bool flat (float lowDb, float highDb, float midDb = 0) { return std::abs (lowDb) < 0.05f && std::abs (highDb) < 0.05f && std::abs (midDb) < 0.05f; }
    void process (float* L, float* R, int n, float lowDb, float highDb, float lowHz = 160.0f, float highHz = 6000.0f, float midDb = 0, float midHz = 1000.0f, float midQ = 0.7f)
    {
        if (flat (lowDb, highDb, midDb)) return;
        if (! juce::exactlyEqual (lowDb, lastLow) || ! juce::exactlyEqual (lowHz, lastLowHz))  { for (int ch = 0; ch < 2; ++ch) eq[ch].shelf (false, lowHz, lowDb, sr);  lastLow = lowDb; lastLowHz = lowHz; }
        if (! juce::exactlyEqual (highDb, lastHigh) || ! juce::exactlyEqual (highHz, lastHighHz)) { for (int ch = 0; ch < 2; ++ch) eq[ch + 2].shelf (true, std::min (highHz, sr * 0.45f), highDb, sr); lastHigh = highDb; lastHighHz = highHz; }
        if (std::abs (midDb) >= 0.05f)
        {
            if (! juce::exactlyEqual (midDb, lastMid) || ! juce::exactlyEqual (midHz, lastMidHz) || ! juce::exactlyEqual (midQ, lastMidQ)) { for (auto& m : mid) m.peak (std::min (midHz, sr * 0.45f), midQ, midDb, sr); lastMid = midDb; lastMidHz = midHz; lastMidQ = midQ; }
            for (int i = 0; i < n; ++i)
            {
                L[i] = eq[2].tick (mid[0].tick (eq[0].tick (L[i])));
                R[i] = eq[3].tick (mid[1].tick (eq[1].tick (R[i])));
            }
            return;
        }
        for (int i = 0; i < n; ++i)
        {
            L[i] = eq[2].tick (eq[0].tick (L[i]));
            R[i] = eq[3].tick (eq[1].tick (R[i]));
        }
    }
    float sr = 44100, lastLow = 999, lastHigh = 999, lastMid = 999, lastLowHz = 160.0f, lastHighHz = 6000.0f, lastMidHz = 0, lastMidQ = 0;
    Biquad eq[4], mid[2];
};

// tempo delay: 0 ping-pong, 1 stereo (straight, right 2 % longer), 2 tape (mono feedback through a 3.2 kHz low-pass, wobble)
struct DelayStage
{
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        for (auto& d : dly) d.prepare ((int) (4.5f * sr));
        for (auto& f : tapeLp) { f.setHz (3200.0f, sr); f.reset(); }
        for (auto& f : fbLp) f.reset();
        for (auto& f : fbHp) f.reset();
        smDelay = 0; smDelaySamples = -1;
    }
    void clear() { for (auto& d : dly) d.clear(); for (auto& f : tapeLp) f.reset(); for (auto& f : fbLp) f.reset(); for (auto& f : fbHp) f.reset(); smDelay = 0; smDelaySamples = -1; }
    // feedback filters (CHROME VAULT): high cut / low cut in the repeats, 0 = off
    void setFeedbackFilters (float hiCutHz, float loCutHz)
    {
        fbLpOn = hiCutHz > 0.0f && hiCutHz < 19000.0f; if (fbLpOn) for (auto& f : fbLp) f.setHz (std::min (hiCutHz, sr * 0.45f), sr);
        fbHpOn = loCutHz > 25.0f; if (fbHpOn) for (auto& f : fbHp) f.setHz (loCutHz, sr);
    }
    // seconds: the delay time; wobblePhase: tape mode wow phase 0..1 (FxRack passes its lo-fi wow phase)
    void process (float* L, float* R, int n, float mix, float fb, double seconds, int mode, float wobblePhase, float sm, float wobbleDepth = 0.004f)
    {
        const float target = (float) seconds * sr;
        if (smDelaySamples < 0) smDelaySamples = target;
        for (int i = 0; i < n; ++i)
        {
            smDelay += (mix - smDelay) * sm;
            smDelaySamples += (target - smDelaySamples) * 0.0005f;
            float d = smDelaySamples;
            if (mode == 2) { d *= 1.0f + wobbleDepth * std::sin (twoPi * wobblePhase); }
            float dl = dly[0].read (d), dr = dly[1].read (mode == 1 ? d * 1.02f : d);
            if (fbLpOn) { dl = fbLp[0].lp (dl); dr = fbLp[1].lp (dr); }
            if (fbHpOn) { dl -= fbHp[0].lp (dl); dr -= fbHp[1].lp (dr); }
            switch (mode)
            {
                case 1:  dly[0].push (L[i] + dl * fb); dly[1].push (R[i] + dr * fb); break;       // stereo
                case 2:  { const float fbv = tapeLp[0].lp (std::tanh ((dl + dr) * 0.5f * fb * 1.2f));        // tape
                           dly[0].push ((L[i] + R[i]) * 0.5f + fbv); dly[1].push ((L[i] + R[i]) * 0.5f + fbv); break; }
                default: dly[0].push ((L[i] + R[i]) * 0.5f + dr * fb); dly[1].push (dl * fb); break; // ping-pong
            }
            L[i] += dl * smDelay * 0.8f; R[i] += dr * smDelay * 0.8f;
        }
    }
    float sr = 44100, smDelay = 0, smDelaySamples = -1;
    bool fbLpOn = false, fbHpOn = false;
    DelayLine dly[2];
    OnePole tapeLp[2], fbLp[2], fbHp[2];
};

// reversed grains of `grainBeats` (1/8 note in FxRack)
struct ReverseStage
{
    void prepare (float sampleRate) { sr = sampleRate; for (auto& d : revRing) d.prepare ((int) (1.5f * sr)); revGrain = 0; smReverse = 0; }
    void clear() { for (auto& d : revRing) d.clear(); smReverse = 0; }
    void process (float* L, float* R, int n, float amount, double bpm, float sm, double grainBeats = 0.5)
    {
        const float P = (float) (60.0 / bpm * grainBeats) * sr;   // the caller keeps 2 * P inside the 1.5 s ring
        const float gInc = 1.0f / std::max (64.0f, P);
        for (int i = 0; i < n; ++i)
        {
            smReverse += (amount - smReverse) * sm;
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
    float sr = 44100, revGrain = 0, smReverse = 0;
    DelayLine revRing[2];
};

// Transient shaper: fast vs slow envelope, boosts the attack up to about +12 dB
struct PunchStage
{
    void prepare (float sampleRate)
    {
        sr = sampleRate;
        envFast = envSlow = 0; smPunch = 0;
        atkFast = 1.0f - std::exp (-1.0f / (0.0008f * sr)); relFast = 1.0f - std::exp (-1.0f / (0.03f * sr));
        atkSlow = 1.0f - std::exp (-1.0f / (0.025f * sr)); relSlow = 1.0f - std::exp (-1.0f / (0.25f * sr));
    }
    void clear() { envFast = envSlow = 0; smPunch = 0; }
    void process (float* L, float* R, int n, float amount, float sm)
    {
        if (amount <= 0.001f && smPunch <= 0.001f) return;
        for (int i = 0; i < n; ++i)
        {
            smPunch += (amount - smPunch) * sm;
            const float x = std::abs (L[i]) + std::abs (R[i]);
            envFast += (x - envFast) * (x > envFast ? atkFast : relFast);
            envSlow += (x - envSlow) * (x > envSlow ? atkSlow : relSlow);
            const float tr = std::max (0.0f, envFast - envSlow) / (envSlow + 1.0e-3f);
            const float g = 1.0f + std::min (3.0f, tr * 3.0f) * smPunch;
            L[i] *= g; R[i] *= g;
        }
    }
    float sr = 44100, envFast = 0, envSlow = 0, smPunch = 0, atkFast = 0.5f, relFast = 0.01f, atkSlow = 0.01f, relSlow = 0.001f;
};

// the ghost bus: the voices' ghost send (G) + a share of the mix -> reversed grains (or a plain delay) -> echo -> blur reverb
struct GhostStage
{
    static constexpr int maxBlock = 4096;
    void prepare (double sampleRate)
    {
        sr = (float) sampleRate;
        ghostRing.prepare ((int) (1.5f * sr));
        ghostDly.prepare ((int) (4.5f * sr));
        ghostRev.setSampleRate (sampleRate); ghostRev.reset();
        ghostHp.setHz (250.0f, sr); ghostHp.reset();
        grainPhase = 0;
    }
    void clear() { ghostRing.clear(); ghostDly.clear(); ghostRev.reset(); ghostHp.reset(); }
    void process (float* L, float* R, const float* G, int n, float ghost, float blur, bool rev, double bpm)
    {
        if (ghost <= 0.001f) return;
        const float P = 0.28f * sr;
        const float gInc = 1.0f / P;
        const float dSamples = 0.75f * 60.0f / (float) bpm * sr;
        for (int i = 0; i < n; ++i)
        {
            const float send = G[i] + (L[i] + R[i]) * 0.5f * ghost * 0.45f;
            ghostRing.push (send - ghostHp.lp (send));
            float gOut;
            if (rev)
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
        gp.roomSize = 0.7f + blur * 0.28f; gp.damping = 0.6f;
        gp.wetLevel = 0.4f + blur * 0.6f; gp.dryLevel = 0.25f + (1.0f - blur) * 0.4f; gp.width = 1.0f;
        ghostRev.setParameters (gp);
        ghostRev.processStereo (gL, gR, n);
        for (int i = 0; i < n; ++i) { L[i] += gL[i] * ghost; R[i] += gR[i] * ghost; }
    }
    float sr = 44100, grainPhase = 0;
    DelayLine ghostRing, ghostDly;
    OnePole ghostHp;
    juce::Reverb ghostRev;
    float gL[maxBlock] {}, gR[maxBlock] {};
};

// circuit bend: on a beat grid, random events (stutter, dropout, tape stop, crush) - deterministic per step and seed
struct CircuitStage
{
    void prepare (float sampleRate) { sr = sampleRate; for (auto& d : ring) d.prepare ((int) (1.5f * sr)); lastStep = -1; evType = 0; }
    void clear() { for (auto& d : ring) d.clear(); }
    void process (float* L, float* R, int n, float circuit, double stepBeats, uint32_t seed, double bpm, double beatPos)
    {
        const double beatsPerSample = bpm / 60.0 / sr;
        const float stepLen = (float) (stepBeats / beatsPerSample);
        for (int i = 0; i < n; ++i)
        {
            ring[0].push (L[i]); ring[1].push (R[i]);
            const double beat = beatPos + beatsPerSample * i;
            const int64_t step = (int64_t) std::floor (beat / stepBeats);
            if (step != lastStep)
            {
                lastStep = step;
                Rng r; r.seed (hash32 ((uint32_t) step * 2654435761u ^ seed));
                evType = (circuit > 0.001f && r.uni() < circuit * 0.75f) ? 1 + (int) (r.uni() * 4.0f) : 0;
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
    float sr = 44100;
    DelayLine ring[2];
    int64_t lastStep = -1; int evType = 0; float evSlice = 1, evT = 0;
};

// Half-speed buffer: replays each 2-beat window at half speed (half-time feel), synced to the host beat
struct HalftimeStage
{
    void prepare (float sampleRate) { sr = sampleRate; for (auto& d : htRing) d.prepare ((int) (6.5f * sr)); smHalf = 0; }
    void clear() { for (auto& d : htRing) d.clear(); smHalf = 0; }
    void process (float* L, float* R, int n, float amount, double bpm, double beatPos, float sm)
    {
        const double beatsPerSample = bpm / 60.0 / sr;
        const float winLen = (float) (2.0 / beatsPerSample);   // 2 beats of output
        for (int i = 0; i < n; ++i)
        {
            htRing[0].push (L[i]); htRing[1].push (R[i]);
            smHalf += (amount - smHalf) * sm;
            if (smHalf < 0.001f) continue;
            const double beat = beatPos + beatsPerSample * i;
            const float t = (float) (beat / 2.0 - std::floor (beat / 2.0)) * winLen;   // 0 .. winLen
            const float d = 1.0f + t * 0.5f;                                           // read at half speed
            const float fade = std::min ({ 1.0f, t / 256.0f, (winLen - t) / 256.0f });
            const float wl = htRing[0].read (d) * fade, wr = htRing[1].read (d) * fade;
            L[i] += (wl - L[i]) * smHalf; R[i] += (wr - R[i]) * smHalf;
        }
    }
    float sr = 44100, smHalf = 0;
    DelayLine htRing[2];
};

class FxRack
{
public:
    void prepare (double sampleRate, int maxBlock)
    {
        sr = (float) sampleRate;
        driveSt.prepare (sr, maxBlock);
        circuitSt.prepare (sr);
        halftimeSt.prepare (sr);
        punchSt.prepare (sr);
        ghostSt.prepare (sampleRate);
        fdn.prepare (sampleRate); mastering.prepare (sampleRate);
        for (auto& f : sideLp) f.setHz (120.0f, sr);
        for (auto& d : dc) d.setHz (8.0f, sr);
        for (auto& d : dc) d.x1 = d.y1 = 0;
        for (auto* f : { &sideLp[0], &sideLp[1] }) f->reset();
        for (auto& h : subsonic) { h.b0 = 1; h.z1 = h.z2 = 0; }
        {   // 2nd-order Butterworth high-pass at 22 Hz for bass mode
            const float w = twoPi * 22.0f / sr, cs = std::cos (w), al = std::sin (w) / std::sqrt (2.0f), a0 = 1.0f + al;
            for (auto& h : subsonic) { h.b0 = (1 + cs) / 2 / a0; h.b1 = -(1 + cs) / a0; h.b2 = (1 + cs) / 2 / a0; h.a1 = -2 * cs / a0; h.a2 = (1 - al) / a0; }
        }
        bodySt.prepare (sr);
        phaserSt.prepare (sr);
        lofiSt.prepare (sr);
        chorusSt.prepare (sr); flangerSt.prepare (sr); delaySt.prepare (sr); reverseSt.prepare (sr);
        eqSt.prepare (sr);
    }

    void process (float* L, float* R, const float* G, int n, const FxParams& p)
    {
        sm = 1.0f - std::exp (-1.0f / (0.02f * sr));   // 20 ms parameter smoothing
        for (int slot : p.order)
        {
            switch (slot)
            {
                case fxDrive:   driveSt.process (L, R, n, p.drive, p.driveType, p.cleanLow, p.eco, sm); break;
                case fxBody:    bodySt.process (L, R, n, p.body, p.bodyMix, sm); break;
                case fxLofi:    lofiSt.process (L, R, n, p.crush, p.wow); break;
                case fxCircuit: circuitBend (L, R, n, p); break;
                case fxChorus:  chorusSt.process (L, R, n, p.chorus, sm); break;
                case fxPhaser:  phaserSt.process (L, R, n, p.phaser, sm); break;
                case fxFlanger: flangerSt.process (L, R, n, p.flanger, sm); break;
                case fxEq:      eqSt.process (L, R, n, p.eqLow, p.eqHigh); break;
                case fxDelay:   delaySt.process (L, R, n, p.delayMix, p.delayFb, p.delayBeats * 60.0 / p.bpm, p.delayMode, lofiSt.wowPhase, sm); break;
                case fxReverb:  fdn.process (L, R, n, p.revType, p.revSize, p.revMix, p.freeze); break;
                case fxReverse: reverseSt.process (L, R, n, p.reverse, p.bpm, sm); break;
                case fxPunch:   punchSt.process (L, R, n, p.punch, sm); break;
                case fxHalftime: halftimeStage (L, R, n, p); break;
                default: break;
            }
        }
        ghostSt.process (L, R, G, n, p.ghost, p.ghostBlur, p.ghostRev, p.bpm);

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

    // Silences every delay line / filter state without allocating (audio thread safe).
    void clearState()
    {
        driveSt.clear(); bodySt.clear(); lofiSt.clear(); chorusSt.clear(); phaserSt.clear(); flangerSt.clear();
        eqSt.clear(); delaySt.clear(); reverseSt.clear(); punchSt.clear(); ghostSt.clear();
        circuitSt.clear(); halftimeSt.clear();
        fdn.reset(); mastering.reset();
        for (auto* f : { &sideLp[0], &sideLp[1] }) f->reset();
        for (auto& d : dc) d.x1 = d.y1 = 0;
        for (auto& b : subsonic) b.z1 = b.z2 = 0;
    }

private:
    void circuitBend (float* L, float* R, int n, const FxParams& p) { circuitSt.process (L, R, n, p.circuit, p.circBeats, p.seed, p.bpm, p.beatPos); }
    void halftimeStage (float* L, float* R, int n, const FxParams& p) { halftimeSt.process (L, R, n, p.halftime, p.bpm, p.beatPos, sm); }

    float sr = 44100, sm = 0.01f;
    DriveStage driveSt; BodyStage bodySt; LofiStage lofiSt; ChorusStage chorusSt; PhaserStage phaserSt; FlangerStage flangerSt;
    EqStage eqSt; DelayStage delaySt; ReverseStage reverseSt; PunchStage punchSt; GhostStage ghostSt; CircuitStage circuitSt; HalftimeStage halftimeSt;
    OnePole sideLp[2];
    DcBlock dc[2];
    FdnReverb fdn;
    MasterStage mastering;
    Biquad subsonic[2];
};
} // namespace kk
