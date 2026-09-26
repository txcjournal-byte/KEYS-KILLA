#pragma once
#include "DspUtil.h"
#include <juce_dsp/juce_dsp.h>
#include <array>

namespace kk
{
enum FxSlot { fxDrive, fxBody, fxLofi, fxCircuit, fxChorus, fxPhaser, fxFlanger, fxEq, fxDelay, fxReverb, fxReverse, numFxSlots };

inline const char* fxSlotName (int s)
{
    static const char* n[] { "Drive", "Body Swap", "Lo-Fi", "Circuit Bend", "Chorus", "Phaser", "Flanger", "EQ", "Delay", "Reverb", "Reverse" };
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
    std::array<int, numFxSlots> order { fxDrive, fxBody, fxLofi, fxCircuit, fxChorus, fxPhaser, fxFlanger, fxEq, fxDelay, fxReverb, fxReverse };
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
        ghostRing.prepare ((int) (1.5f * sr));
        ghostDly.prepare ((int) (4.5f * sr));
        rev.setSampleRate (sampleRate); rev.reset();
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
            peakPre = std::max (peakPre, std::max (std::abs (l), std::abs (r)));
            L[i] = softLimit (l); R[i] = softLimit (r);
        }
    }

    float peakPre = 0;   // pre-limiter peak (for overload warning), reset by owner

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
                float noiseV = crackle.bi() * 0.002f * p.wow;
                if (crackle.uni() < p.wow * 12.0f / sr) noiseV += crackle.bi() * 0.25f * p.wow;
                l += noiseV; r += noiseV;
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
        juce::Reverb::Parameters rp;
        switch (p.revType)
        {
            case 1:  rp.roomSize = 0.3f + p.revSize * 0.5f; rp.damping = 0.15f; break;    // plate
            case 2:  rp.roomSize = 0.8f + p.revSize * 0.19f; rp.damping = 0.7f; break;    // cloud
            default: rp.roomSize = 0.45f + p.revSize * 0.53f; rp.damping = 0.45f; break;  // hall
        }
        const float mix = p.freeze ? std::max (p.revMix, 0.5f) : p.revMix;
        rp.wetLevel = mix * (p.revType == 2 ? 0.7f : 0.55f);
        rp.dryLevel = 1.0f - mix * 0.45f;
        rp.width = 1.0f;
        rp.freezeMode = p.freeze ? 1.0f : 0.0f;
        rev.setParameters (rp);
        rev.processStereo (L, R, n);
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
    DelayLine wowDl[2], chDl[2], flDl[2], dly[2], ring[2], revRing[2], ghostRing, ghostDly;
    juce::Reverb rev, ghostRev;
    Biquad eq[4], subsonic[2];
    float lastEqLow = 999, lastEqHigh = 999;
    float apState[2][4] {}, apFb[2] {};
    Rng crackle;
    static constexpr int maxBlock = 4096;
    float gL[maxBlock] {}, gR[maxBlock] {};
    float smDrive = 0, smChorus = 0, smDelay = 0, smBody = 0, smPhaser = 0, smFlanger = 0, smReverse = 0, smDelaySamples = -1;
    float wowPhase = 0, flutPhase = 0, chPhase = 0, phPhase = 0, flPhase = 0, grainPhase = 0, revGrain = 0;
    int holdCount = 0; float holdL = 0, holdR = 0;
    int64_t lastStep = -1; int evType = 0; float evSlice = 1, evT = 0;
};
} // namespace kk
