#pragma once
#include "DspUtil.h"
#include <juce_dsp/juce_dsp.h>

namespace kk
{
struct FxParams
{
    float drive = 0; int driveType = 0; bool cleanLow = false;
    float crush = 0, wow = 0, chorus = 0;
    float delayMix = 0, delayFb = 0.35f; double delayBeats = 0.75;
    float revMix = 0.15f, revSize = 0.6f, width = 0.5f;
    float ghost = 0, circuit = 0; int body = 0; float bodyMix = 0.5f;
    bool  monoLows = false;
    float outGain = 1.0f;
    double bpm = 120, beatPos = 0;   // beat position at block start
    uint32_t seed = 1234;
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
        for (auto& d : dly)   d.prepare ((int) (4.5f * sr));
        for (auto& d : ring)  d.prepare ((int) (1.5f * sr));
        ghostRing.prepare ((int) (1.5f * sr));
        ghostDly.prepare ((int) (4.5f * sr));
        rev.setSampleRate (sampleRate); rev.reset();
        ghostRev.setSampleRate (sampleRate); ghostRev.reset();
        for (auto& f : splitA) f.setHz (140.0f, sr);
        for (auto& f : splitB) f.setHz (140.0f, sr);
        for (auto& f : sideLp) f.setHz (120.0f, sr);
        ghostHp.setHz (250.0f, sr);
        for (auto& b : bodyS) for (auto& s : b) s.reset();
        crackle.seed (777);
        smDrive = smChorus = smDelay = smBody = 0; smDelaySamples = -1;
        wowPhase = flutPhase = chPhase = 0; holdCount = 0; holdL = holdR = 0;
        grainPhase = 0; lastStep = -1; evType = 0;
    }

    void process (float* L, float* R, const float* G, int n, const FxParams& p)
    {
        const float sm = 1.0f - std::exp (-1.0f / (0.02f * sr));   // 20 ms parameter smoothing

        // ---------- drive (2x oversampled), optional clean low -----------
        if (p.drive > 0.001f || smDrive > 0.001f)
        {
            if (p.cleanLow)
                for (int i = 0; i < n; ++i)
                {
                    const float lL = splitB[0].lp (splitA[0].lp (L[i]));
                    const float lR = splitB[1].lp (splitA[1].lp (R[i]));
                    low[0][(size_t) i] = lL; low[1][(size_t) i] = lR;
                    L[i] -= lL; R[i] -= lR;
                }
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
            if (p.cleanLow)
                for (int i = 0; i < n; ++i) { L[i] += low[0][(size_t) i]; R[i] += low[1][(size_t) i]; }
        }

        // ---------- body swap (resonant body) ----------------------------
        if (p.body > 0 || smBody > 0.001f)
        {
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

        // ---------- lo-fi: bitcrush, SR reduction, wow/flutter, vinyl ----
        if (p.crush > 0.001f || p.wow > 0.001f)
        {
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
                    if (wowPhase >= 1) wowPhase -= 1; if (flutPhase >= 1) flutPhase -= 1;
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

        // ---------- circuit bend (tempo-synced glitches) ------------------
        {
            const double beatsPerSample = p.bpm / 60.0 / sr;
            const double stepBeats = 0.25;
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
                    evSlice = stepLen / (float) (2 << (int) (r.uni() * 3.0f));
                    evT = 0;
                }
                if (evType == 0) continue;
                const float t = evT++;
                const float fade = std::min ({ 1.0f, t / 48.0f, std::max (0.0f, stepLen - t) / 48.0f });
                float wl = 0, wr = 0;
                switch (evType)
                {
                    case 1: { const float d = t + evSlice - std::fmod (t, evSlice); wl = ring[0].read (d); wr = ring[1].read (d); break; } // stutter
                    case 2: break;                                                                                                   // dropout
                    case 3: { const float d = 1.0f + t * 0.5f; wl = ring[0].read (d); wr = ring[1].read (d); break; }                   // tape stop-ish
                    default: { const float d = 1.0f + std::fmod (t, 16.0f); wl = std::round (ring[0].read (d) * 6.0f) / 6.0f;          // hard crush
                               wr = std::round (ring[1].read (d) * 6.0f) / 6.0f; break; }
                }
                L[i] += (wl - L[i]) * fade; R[i] += (wr - R[i]) * fade;
            }
        }

        // ---------- chorus ----------------------------------------------
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

        // ---------- ghost bus: reversed grains + shadow reverb ------------
        if (p.ghost > 0.001f)
        {
            const float P = 0.28f * sr;
            const float gInc = 1.0f / P;
            for (int i = 0; i < n; ++i)
            {
                const float send = G[i] + (L[i] + R[i]) * 0.5f * p.ghost * 0.45f;
                ghostRing.push (send - ghostHp.lp (send));
                float g1 = grainPhase, g2 = grainPhase + 0.5f; if (g2 >= 1) g2 -= 1;
                const float rev1 = ghostRing.read (1.0f + 2.0f * g1 * P) * std::sin (pi * g1);
                const float rev2 = ghostRing.read (1.0f + 2.0f * g2 * P) * std::sin (pi * g2);
                grainPhase += gInc; if (grainPhase >= 1) grainPhase -= 1;
                const float gOut = (rev1 + rev2) * 0.7f + G[i] * 0.6f;
                ghostDly.push (gOut + ghostDly.read (0.75f * 60.0f / (float) p.bpm * sr) * 0.45f);
                gL[i] = gOut; gR[i] = ghostDly.read (0.75f * 60.0f / (float) p.bpm * sr) * 0.8f;
            }
            {
                juce::Reverb::Parameters gp; gp.roomSize = 0.93f; gp.damping = 0.6f; gp.wetLevel = 0.9f; gp.dryLevel = 0.25f; gp.width = 1.0f;
                ghostRev.setParameters (gp);
                const int m = n;
                ghostRev.processStereo (gL, gR, m);
                for (int i = 0; i < m; ++i) { L[i] += gL[i] * p.ghost; R[i] += gR[i] * p.ghost; }
            }
        }

        // ---------- ping-pong delay ---------------------------------------
        {
            const float target = (float) (p.delayBeats * 60.0 / p.bpm) * sr;
            if (smDelaySamples < 0) smDelaySamples = target;
            for (int i = 0; i < n; ++i)
            {
                smDelay += (p.delayMix - smDelay) * sm;
                smDelaySamples += (target - smDelaySamples) * 0.0005f;
                const float dl = dly[0].read (smDelaySamples), dr = dly[1].read (smDelaySamples);
                dly[0].push ((L[i] + R[i]) * 0.5f + dr * p.delayFb);
                dly[1].push (dl * p.delayFb);
                L[i] += dl * smDelay * 0.8f; R[i] += dr * smDelay * 0.8f;
            }
        }

        // ---------- reverb ------------------------------------------------
        {
            juce::Reverb::Parameters rp;
            rp.roomSize = 0.45f + p.revSize * 0.53f; rp.damping = 0.45f;
            rp.wetLevel = p.revMix * 0.55f; rp.dryLevel = 1.0f - p.revMix * 0.45f; rp.width = 1.0f;
            rev.setParameters (rp);
            rev.processStereo (L, R, n);
        }

        // ---------- width, mono lows, output ------------------------------
        const float sideGain = p.width * 2.0f;
        for (int i = 0; i < n; ++i)
        {
            const float m = (L[i] + R[i]) * 0.5f;
            float s = (L[i] - R[i]) * 0.5f * sideGain;
            if (p.monoLows) s -= sideLp[0].lp (s);
            float l = dc[0].tick ((m + s) * p.outGain), r = dc[1].tick ((m - s) * p.outGain);
            peakPre = std::max (peakPre, std::max (std::abs (l), std::abs (r)));
            L[i] = softLimit (l); R[i] = softLimit (r);
        }
    }

    float peakPre = 0;   // pre-limiter peak (for overload warning), reset by owner

private:
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

    float sr = 44100;
    juce::dsp::Oversampling<float> os { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, false };
    std::vector<float> low[2];
    OnePole splitA[2], splitB[2], sideLp[2], ghostHp;
    DcBlock dc[2];
    SvfState bodyS[2][5];
    DelayLine wowDl[2], chDl[2], dly[2], ring[2], ghostRing, ghostDly;
    juce::Reverb rev, ghostRev;
    Rng crackle;
    static constexpr int maxGhost = 4096;
    float gL[maxGhost] {}, gR[maxGhost] {};
    float smDrive = 0, smChorus = 0, smDelay = 0, smBody = 0, smDelaySamples = -1;
    float wowPhase = 0, flutPhase = 0, chPhase = 0, grainPhase = 0;
    int holdCount = 0; float holdL = 0, holdR = 0;
    int64_t lastStep = -1; int evType = 0; float evSlice = 1, evT = 0;
};
} // namespace kk
