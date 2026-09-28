#pragma once
#include "DspUtil.h"
#include "Params.h"
#include "Wavetable.h"
#include <array>

namespace kk
{
struct LayerParams
{
    bool  on = true;
    int   engine = 0, octave = 0, semi = 0, unison = 1, fmAlgo = 0, warpMode = 0;
    float fine = 0, wave = 0, detune = 0.2f, fmRatio = 2, fmRatio2 = 3, fmAmt = 0.3f, level = 1;
};

// Snapshot of everything a voice needs, filled once per block
struct VoiceParams
{
    LayerParams layer[2];
    float sub = 0;
    int   filterType = 0;
    float cutoff = 12000, reso = 0.1f, keyTrack = 0.4f, fenv = 0;
    float fA = 0.001f, fD = 0.4f, fS = 0, fR = 0.3f;
    float attack = 0.002f, decay = 0.6f, sustain = 0.8f, release = 0.3f, velSens = 0.6f;
    float e3A = 0.01f, e3D = 0.5f, e3S = 0, e3R = 0.3f;
    float lfoPitch = 0, lfoFilter = 0, lfoAmp = 0;
    int   wobTarget = 0; float wobble = 0;
    bool  mono = false, legato = true; float glide = 0, bendRange = 2;
    float ghost = 0; int ghostOct = 0;
    float bend = 0; int bendMode = 0; float bendSemis = -12; bool tape = false;
    float punch = 0;
    float pitchWheel = 0, modWheel = 0, aftertouch = 0;
    float alive = 0, drift = 0;
    int   mmSrc[numModSlots] {}, mmDst[numModSlots] {}; float mmAmt[numModSlots] {};
    bool  eco = false;
};

// Per-layer oscillator state
struct LayerState
{
    float phase[8] {}, phase2[8] {}, op[3] {}, fbPrev = 0;
    SvfState form[6];
    std::vector<float> ks; int ksW = 0; float ksPrev = 0;
    float modePh[6] {}, modeAmp[6] {}, modeCoef[6] {}, modeRatio[6] {}; int nModes = 0;
    OnePole brassL, brassR; float brassEnv = 0;

    void reset (Rng& r)
    {
        for (auto& p : phase) p = r.uni();
        for (auto& p : phase2) p = r.uni();
        for (auto& o : op) o = 0;
        fbPrev = 0;
        for (auto& f : form) f.reset();
        std::fill (ks.begin(), ks.end(), 0.0f); ksPrev = 0;
        for (auto& a : modeAmp) a = 0;
        brassL.reset(); brassR.reset(); brassEnv = 0;
    }
};

// Values recomputed at control rate
struct LayerCtl
{
    int   nUni = 1;
    float uniRatio[8] {}, uniPanL[8] {}, uniPanR[8] {}, uniNorm = 1;
    float wave = 0, fm = 0, level = 1, fmIndex = 0, ksG = 0.99f, damp = 0.5f, brassA = 0.9f;
    SvfCoef formC[3]; float formG[3] { 1, 0.7f, 0.4f };
    float harmW[8] {}, harmNorm = 1;
};

class Voice
{
public:
    bool  active = false, held = false, sustained = false;
    int   note = -1;
    uint32_t age = 0;
    float vel = 0, polyAT = 0;
    Adsr  amp, env2, env3;

    void prepare (float sampleRate, int index)
    {
        sr = sampleRate;
        for (auto& l : ls) l.ks.assign (16384, 0.0f);
        noise.seed (12345u + (uint32_t) index * 7919u);   // deterministic: offline render == playback
    }

    void start (int n, float v, const VoiceParams& p, float glideFromSemi, uint32_t stamp)
    {
        const bool wasActive = active;
        note = n; vel = v; held = true; sustained = false; age = stamp; polyAT = 0;
        targetSemi = (float) n;
        curSemi = (glideFromSemi >= 0 && p.glide > 0.0005f) ? glideFromSemi : (float) n;
        noteTime = 0; relTime = 0;
        noteRand = noise.bi();
        tapePhase = noise.uni();
        {   // ALIVE: bounded per-note micro variation (deterministic: seeded voice noise)
            const float a = p.alive / 5.0f;
            aliveCents = noise.bi() * a * 14.0f;
            aliveAtk = 1.0f + noise.bi() * a * 0.35f;
            aliveCut = noise.bi() * a * 0.45f;
            alivePan = noise.bi() * a * 0.35f;
            aliveGain = 1.0f + noise.bi() * a * 0.1f;
            driftPh1 = noise.uni(); driftPh2 = noise.uni();
            driftF1 = 0.05f + noise.uni() * 0.6f; driftF2 = 0.4f + noise.uni() * 1.6f;
        }
        ctlIdx = 0;
        if (! wasActive)
        {
            for (auto& l : ls) l.reset (noise);
            subPhase = 0; ghostEnv = 0; bendState = 0;
            fl.reset(); fr.reset(); ladL.reset(); ladR.reset();
        }
        for (int k = 0; k < 2; ++k)
        {
            const auto& lp = p.layer[k];
            if (! lp.on) continue;
            if (lp.engine == engPluck) excitePluck (ls[k], lp);
            if (lp.engine == engModal) exciteModal (ls[k], lp, p.decay);
            ls[k].brassEnv = 0;
        }
        active = true;
        amp.set (sr, p.attack, p.decay, p.sustain, p.release);
        env2.set (sr, p.fA, p.fD, p.fS, p.fR);
        env3.set (sr, p.e3A, p.e3D, p.e3S, p.e3R);
        amp.on(); env2.on(); env3.on();
        if (! wasActive) { env2.v = 0; env3.v = 0; }
    }

    void legatoTo (int n) { note = n; targetSemi = (float) n; }
    void release() { held = false; sustained = false; amp.off(); env2.off(); env3.off(); }
    void kill()    { active = false; held = false; amp.kill(); env2.kill(); env3.kill(); note = -1; }
    bool releasing() const { return amp.st == Adsr::Release; }
    float level() const { return amp.v; }

    // Adds into L/R/ghost
    void render (float* L, float* R, float* G, int num, const VoiceParams& p, const float* lfo1, const float* lfo2)
    {
        if (! active) return;
        amp.set (sr, p.attack * aliveAtk, p.decay, p.sustain, p.release);
        env2.set (sr, p.fA, p.fD, p.fS, p.fR);
        env3.set (sr, p.e3A, p.e3D, p.e3S, p.e3R);
        const float driftCents = p.drift * 35.0f;

        const int ctlMask = p.eco ? 31 : 7;
        const float glideCoef = p.glide > 0.0005f ? std::exp (-1.0f / (p.glide * 0.35f * sr)) : 0.0f;
        const float velGain = 1.0f - p.velSens + p.velSens * vel;
        const float dt = 1.0f / sr;
        static const float ghostMul[] { 2.003f, 0.4985f, 4.006f, 1.004f };
        const float gMul = ghostMul[std::clamp (p.ghostOct, 0, 3)];
        const auto& wt = WavetableBank::get();

        for (int i = 0; i < num; ++i)
        {
            // ---------------- control rate --------------------------------
            if ((ctlIdx++ & ctlMask) == 0)
                updateControl (p, lfo1[i], lfo2[i]);

            // ---------------- pitch ---------------------------------------
            curSemi = glideCoef > 0 ? targetSemi + (curSemi - targetSemi) * glideCoef : targetSemi;
            float semi = curSemi + p.pitchWheel * p.bendRange + modPitch + aliveCents * 0.01f;
            if (driftCents > 0.01f)
                semi += driftCents * 0.01f * (0.6f * std::sin (twoPi * (driftF1 * noteTime + driftPh1))
                                            + 0.4f * std::sin (twoPi * (driftF2 * noteTime + driftPh2)));
            const float vibRamp = std::min (1.0f, noteTime * 3.0f);
            semi += lfo1[i] * (p.lfoPitch + p.modWheel * 0.5f) * 1.5f * vibRamp;
            if (p.wobTarget == 3) semi += lfo1[i] * p.wobble * 2.0f;
            semi += bendOffset (p);
            if (p.punch > 0) semi += p.punch * 14.0f * std::exp (-noteTime * 45.0f);

            // ---------------- layers --------------------------------------
            float oL = 0, oR = 0;
            for (int k = 0; k < 2; ++k)
            {
                const auto& lp = p.layer[k];
                if (! lp.on) continue;
                float ls_ = semi + (float) (lp.octave * 12 + lp.semi) + lp.fine * 0.01f;
                if (lp.engine == engSub) ls_ += lp.fmAmt * 24.0f * std::exp (-noteTime * 22.0f);
                const float freq = std::clamp (semisToHz (ls_), 8.0f, sr * 0.45f);
                float a = 0, b = 0;
                layerSample (ls[k], ctl[k], lp, freq / sr, freq, a, b, wt);
                oL += a * ctl[k].level; oR += b * ctl[k].level;
            }

            // ---------------- filter --------------------------------------
            float yL, yR;
            switch (p.filterType)
            {
                case 1:  yL = ladL.tick (oL, ladG, effReso * 0.95f); yR = ladR.tick (oR, ladG, effReso * 0.95f); break;
                case 2:  fl.tick (fc, fastTanh (oL * 2.0f) * 0.8f); fr.tick (fc, fastTanh (oR * 2.0f) * 0.8f);
                         yL = fastTanh (fl.lp * 1.6f); yR = fastTanh (fr.lp * 1.6f); break;
                case 3:  fl.tick (fc, oL); fr.tick (fc, oR); yL = fl.hp; yR = fr.hp; break;
                case 4:  fl.tick (fc, oL); fr.tick (fc, oR); yL = fl.bp * fc.k; yR = fr.bp * fc.k; break;
                case 5:  fl.tick (fc, oL); fr.tick (fc, oR); yL = fl.lp + fl.hp; yR = fr.lp + fr.hp; break;             // notch
                case 6:  fl.tick (fc, oL); fr.tick (fc, oR); yL = oL + fl.bp * fc.k * 1.5f; yR = oR + fr.bp * fc.k * 1.5f; break;   // peak
                default: fl.tick (fc, oL); fr.tick (fc, oR); yL = fl.lp; yR = fr.lp; break;
            }

            // ---------------- sub layer -----------------------------------
            if (effSub > 0.001f)
            {
                const float s = std::sin (twoPi * subPhase) * effSub * 0.9f;
                subPhase += std::clamp (semisToHz (semi - 12.0f), 8.0f, sr * 0.45f) / sr;
                if (subPhase >= 1.0f) subPhase -= 1.0f;
                yL += s; yR += s;
            }

            const float env = amp.next();
            env2.next(); env3.next();
            float trem = 1.0f - p.lfoAmp * (0.5f - 0.5f * lfo1[i]);
            if (p.wobTarget == 1) trem *= 1.0f - p.wobble * (0.5f - 0.5f * lfo1[i]);
            const float gAmp = env * velGain * trem * ampMod * 0.35f;
            L[i] += yL * gAmp * panL;
            R[i] += yR * gAmp * panR;

            // ---------------- ghost layer ---------------------------------
            if (p.ghost > 0.001f)
            {
                ghostEnv += (1.0f - ghostEnv) * (1.0f / (0.35f * sr));
                const float gs = std::sin (twoPi * ghostPhase) * 0.8f + 0.2f * std::sin (twoPi * 2.0f * ghostPhase);
                ghostPhase += std::clamp (semisToHz (semi), 8.0f, sr * 0.2f) / sr * gMul;
                if (ghostPhase >= 1.0f) ghostPhase -= 1.0f;
                G[i] += gs * ghostEnv * env * velGain * p.ghost * 0.22f;
            }

            noteTime += dt;
            if (releasing()) relTime += dt;
            if (amp.st == Adsr::Idle) { active = false; held = false; note = -1; break; }
        }
    }

private:
    float bendOffset (const VoiceParams& p)
    {
        float off = 0;
        if (p.bend > 0.0005f)
        {
            const float relDive = std::max (0.12f, p.release * 0.9f);
            const float relAmt = releasing() ? std::min (1.0f, relTime / relDive) : 0.0f;
            switch (p.bendMode)
            {
                case 0: off = -std::abs (p.bendSemis) * p.bend * relAmt; break;                        // dive
                case 1: off =  std::abs (p.bendSemis) * p.bend * relAmt; break;                        // rise
                case 2: off = -std::abs (p.bendSemis) * p.bend * 0.3f * std::exp (-noteTime * 16.0f)  // dip in
                              - p.bend * 0.5f * relAmt; break;
                case 3: off = (noteTime < 0.09f ? 12.0f : 0.0f) * std::min (1.0f, p.bend * 3.0f)      // octave jump
                              + std::copysign (12.0f, p.bendSemis) * (p.bend > 0.5f ? relAmt : 0.0f); break;
                default: off = noteRand * p.bend * 1.2f
                              + p.bend * 0.35f * std::sin (twoPi * (0.3f * noteTime + tapePhase)); break; // random
            }
        }
        if (p.tape)   // broken tape: unstable sliding pitch
        {
            const float d = 0.4f + p.bend * 1.2f;
            off += d * (0.45f * std::sin (twoPi * (0.6f * noteTime + tapePhase))
                      + 0.2f * std::sin (twoPi * (2.3f * noteTime + tapePhase * 3.0f)));
        }
        // smooth hard jumps (octave jump)
        bendState += (off - bendState) * (p.bendMode == 3 ? 0.02f : 1.0f);
        return bendState;
    }

    void updateControl (const VoiceParams& p, float l1, float l2)
    {
        float src[10];
        src[srcOff] = 0; src[srcEnv2] = env2.v; src[srcEnv3] = env3.v; src[srcLfo1] = l1; src[srcLfo2] = l2;
        src[srcVel] = vel; src[srcModWheel] = p.modWheel; src[srcAftertouch] = std::max (p.aftertouch, polyAT);
        src[srcKey] = ((float) note - 60.0f) / 24.0f; src[srcRandom] = noteRand;
        float m[numDests] {};
        for (int s = 0; s < numModSlots; ++s)
            if (p.mmSrc[s] > 0 && p.mmAmt[s] != 0.0f)
                m[std::clamp (p.mmDst[s], 0, numDests - 1)] += src[std::clamp (p.mmSrc[s], 0, 9)] * p.mmAmt[s];

        modPitch = m[dstPitch] * 12.0f;
        ampMod = std::clamp (1.0f + m[dstAmp], 0.0f, 2.0f) * aliveGain;
        const float pan = std::clamp (m[dstPan] + alivePan, -1.0f, 1.0f);
        panL = std::sqrt (1.0f - pan) ; panR = std::sqrt (1.0f + pan);
        effSub = clamp01 (p.sub + m[dstSub]);
        effReso = clamp01 (p.reso + m[dstReso]);

        float oct = p.fenv * env2.v * 6.0f + p.lfoFilter * 4.0f * (0.5f * l1 - 0.5f)
                  + p.keyTrack * (curSemi - 60.0f) / 12.0f + p.punch * env2.v * 2.0f + m[dstCutoff] * 6.0f + aliveCut;
        if (p.wobTarget == 0) oct += p.wobble * 4.0f * (0.5f * l1 - 0.5f);
        const float cut = std::clamp (p.cutoff * std::exp2 (oct), 20.0f, sr * 0.45f);
        if (p.filterType == 1) ladG = std::min (0.95f, 1.0f - std::exp (-twoPi * cut / sr));
        else                   fc.set (cut, 2.0f * (1.0f - effReso * (p.filterType == 2 ? 0.985f : 0.96f)), sr);

        const float wobWave = p.wobTarget == 2 ? p.wobble * 0.5f * (0.5f * l1 + 0.5f) : 0.0f;
        for (int k = 0; k < 2; ++k)
        {
            const auto& lp = p.layer[k];
            if (! lp.on) continue;
            auto& c = ctl[k];
            c.wave  = clamp01 (lp.wave + (k == 0 ? m[dstWaveA] : m[dstWaveB]) + wobWave);
            c.fm    = clamp01 (lp.fmAmt + (k == 0 ? m[dstFmA] : m[dstFmB]));
            c.level = std::clamp (lp.level * (1.0f + (k == 0 ? m[dstLevelA] : m[dstLevelB])), 0.0f, 2.0f);
            const bool uniEngine = lp.engine == engVA || lp.engine == engVox || lp.engine == engFM
                                || lp.engine == engWavetable || lp.engine == engOrchestral;
            c.nUni = uniEngine ? std::clamp (p.eco ? std::min (lp.unison, 3) : lp.unison, 1, 8) : 1;
            const float det = clamp01 (lp.detune + m[dstDetune]);
            for (int u = 0; u < c.nUni; ++u)
            {
                const float off = c.nUni == 1 ? 0.0f : ((float) u / (float) (c.nUni - 1)) * 2.0f - 1.0f;
                c.uniRatio[u] = std::exp2 (off * det * 45.0f / 1200.0f);
                const float pn = off * 0.85f;
                c.uniPanL[u] = std::sqrt (0.5f * (1.0f - pn));
                c.uniPanR[u] = std::sqrt (0.5f * (1.0f + pn));
            }
            c.uniNorm = 1.0f / std::sqrt ((float) c.nUni);
            c.fmIndex = c.fm * 7.0f * (0.25f + 0.75f * env2.v) * (0.5f + 0.5f * vel);

            if (lp.engine == engVox || lp.engine == engOrchestral)
            {
                static const float vowels[5][3] { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 },
                                                  { 570, 840, 2410 }, { 300, 870, 2240 } };
                const float vpos = (lp.engine == engVox ? c.wave : c.fm) * 3.999f;
                const int vi = (int) vpos; const float vf = vpos - (float) vi;
                for (int f = 0; f < 3; ++f)
                    c.formC[f].set (vowels[vi][f] + (vowels[vi + 1][f] - vowels[vi][f]) * vf, 0.12f, sr);
            }
            if (lp.engine == engOrgan)
            {
                c.harmNorm = 0;
                for (int h = 0; h < 8; ++h) { c.harmW[h] = std::pow (1.0f / (float) (h + 1), 2.0f - 1.8f * c.wave); c.harmNorm += c.harmW[h]; }
                c.harmNorm = 1.3f / c.harmNorm;
            }
            if (lp.engine == engPluck)
            {
                const float f0 = std::max (20.0f, semisToHz (curSemi + (float) (lp.octave * 12 + lp.semi)));
                c.damp = 0.5f + 0.5f * c.wave;
                c.ksG = std::pow (0.001f, 1.0f / (f0 * std::max (0.05f, p.decay * 1.5f)));
            }
            if (lp.engine == engOrchestral)
            {
                auto& s = ls[k];
                s.brassEnv += (1.0f - s.brassEnv) * std::min (1.0f, (float) (p.eco ? 32 : 8) / (0.08f * sr));
                const float bc = 250.0f + 5500.0f * s.brassEnv * (0.6f + 0.4f * vel);
                c.brassA = std::exp (-twoPi * bc / sr);
            }
        }
    }

    inline float warpPhase (float ph, int mode, float w) const
    {
        switch (mode)
        {
            case 1:  { const float x = ph * (1.0f + w * 6.0f); return x - std::floor (x); }                // sync
            case 2:  { float x = (ph < 0.5f ? ph : 1.0f - ph) * 2.0f * (0.5f + w * 0.5f); return x - std::floor (x); } // mirror
            case 3:  { const float st = 64.0f - w * 60.0f; return std::floor (ph * st) / st; }            // quantize
            case 4:  { float x = ph + w * 0.3f * std::sin (twoPi * ph * 2.0f); return x - std::floor (x); } // fm
            default: return std::pow (ph, 1.0f + w * 3.0f);                                                // bend
        }
    }

    inline float saw (float t, float d) const { return 2.0f * t - 1.0f - polyBlep (t, d); }

    void layerSample (LayerState& s, const LayerCtl& c, const LayerParams& lp, float inc, float freq,
                      float& oL, float& oR, const WavetableBank& wt)
    {
        switch (lp.engine)
        {
            case engVA:
            case engVox:
            {
                const float w = lp.engine == engVox ? 0.0f : c.wave;
                for (int u = 0; u < c.nUni; ++u)
                {
                    const float d = inc * c.uniRatio[u];
                    float& t = s.phase[u];
                    float v;
                    const float sw = saw (t, d);
                    if (w <= 0.5f)
                    {
                        const float t2 = t + 0.5f >= 1.0f ? t - 0.5f : t + 0.5f;
                        const float sq = (t < 0.5f ? 1.0f : -1.0f) + polyBlep (t, d) - polyBlep (t2, d);
                        v = sw + (sq - sw) * (w * 2.0f);
                    }
                    else
                    {
                        const float pw = 0.5f - (w - 0.5f) * 0.8f;
                        float t2 = t + 1.0f - pw; if (t2 >= 1.0f) t2 -= 1.0f;
                        v = (t < pw ? 1.0f : -1.0f) + polyBlep (t, d) - polyBlep (t2, d);
                    }
                    t += d; if (t >= 1.0f) t -= 1.0f;
                    oL += v * c.uniPanL[u]; oR += v * c.uniPanR[u];
                }
                oL *= c.uniNorm * 1.2f; oR *= c.uniNorm * 1.2f;
                if (lp.engine == engVox) formant (s, c, oL, oR);
                break;
            }
            case engFM:
            {
                const float I = c.fmIndex, fb = c.wave * 1.5f;
                const float r1 = lp.fmRatio, r2 = lp.fmRatio2;
                float mod1 = 0, car2Amt = 0, mod2 = 0;
                auto adv = [&] (float& ph, float r) { ph += inc * r; ph -= std::floor (ph); };
                switch (lp.fmAlgo)
                {
                    case 1: { const float m4 = std::sin (twoPi * s.op[2] + fb * s.fbPrev); s.fbPrev = m4;             // 4-stack
                              const float m3 = std::sin (twoPi * s.op[1] + I * 0.5f * m4);
                              mod1 = std::sin (twoPi * s.op[0] + I * 0.6f * m3);
                              adv (s.op[0], r1); adv (s.op[1], r2); adv (s.op[2], 1.0f); break; }
                    case 3: { const float m4 = std::sin (twoPi * s.op[2] + fb * s.fbPrev); s.fbPrev = m4;             // 3 > 1
                              mod1 = std::sin (twoPi * s.op[0]) + 0.5f * std::sin (twoPi * s.op[1]) + 0.3f * m4;
                              adv (s.op[0], r1); adv (s.op[1], r2); adv (s.op[2], 1.0f); break; }
                    case 2: case 4: case 5:                                                                            // two carriers
                    {
                        const float m2 = std::sin (twoPi * s.op[0] + fb * s.fbPrev); s.fbPrev = m2;
                        mod1 = m2;
                        const float r4 = lp.fmAlgo == 4 ? r2 * 1.41f : lp.fmAlgo == 5 ? r2 : r2 * r1;
                        mod2 = std::sin (twoPi * s.op[2]);
                        car2Amt = lp.fmAlgo == 5 ? 0.25f * env2.v : lp.fmAlgo == 4 ? 0.4f : 0.5f;
                        adv (s.op[0], r1); adv (s.op[2], r4);
                        break;
                    }
                    default: { const float m2 = std::sin (twoPi * s.op[0] + fb * s.fbPrev); s.fbPrev = m2;           // 2-op
                               mod1 = m2; adv (s.op[0], r1); break; }
                }
                const float c3ratio = lp.fmAlgo == 5 ? r2 : r2;
                for (int u = 0; u < c.nUni; ++u)
                {
                    float v = std::sin (twoPi * s.phase[u] + I * (lp.fmAlgo == 5 ? 0.5f : 1.0f) * mod1);
                    if (car2Amt > 0)
                    {
                        v = v * (1.0f - car2Amt) + car2Amt * std::sin (twoPi * s.phase2[u] + I * 0.8f * mod2);
                        s.phase2[u] += inc * c3ratio * c.uniRatio[u]; s.phase2[u] -= std::floor (s.phase2[u]);
                    }
                    s.phase[u] += inc * c.uniRatio[u]; if (s.phase[u] >= 1.0f) s.phase[u] -= 1.0f;
                    oL += v * c.uniPanL[u]; oR += v * c.uniPanR[u];
                }
                oL *= c.uniNorm; oR *= c.uniNorm;
                break;
            }
            case engPluck:
            {
                const float len = sr / freq - (1.0f - c.damp);
                const float rp = (float) s.ksW - len;
                const int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
                const float a = s.ks[(size_t) (i0 & 16383)], b = s.ks[(size_t) ((i0 + 1) & 16383)];
                const float y = a + (b - a) * fr;
                s.ks[(size_t) s.ksW] = c.ksG * (c.damp * y + (1.0f - c.damp) * s.ksPrev);
                s.ksPrev = y;
                s.ksW = (s.ksW + 1) & 16383;
                oL = oR = y * 1.4f;
                break;
            }
            case engOrgan:
            {
                float v = 0; float& t = s.phase[0];
                for (int h = 0; h < 8; ++h)
                    if (inc * (float) (h + 1) < 0.45f) v += c.harmW[h] * std::sin (twoPi * t * (float) (h + 1));
                t += inc; if (t >= 1.0f) t -= 1.0f;
                oL = oR = v * c.harmNorm;
                break;
            }
            case engFlute:
            {
                float& t = s.phase[0];
                const float v = std::sin (twoPi * t) + 0.18f * std::sin (twoPi * 2.0f * t) + 0.06f * std::sin (twoPi * 3.0f * t);
                t += inc; if (t >= 1.0f) t -= 1.0f;
                const float breath = noise.bi() * c.wave * 0.35f * (0.4f + 0.6f * std::exp (-noteTime * 6.0f));
                oL = oR = v + breath;
                break;
            }
            case engWavetable:
            {
                for (int u = 0; u < c.nUni; ++u)
                {
                    float& t = s.phase[u];
                    const float d = inc * c.uniRatio[u];
                    const float v = wt.read (warpPhase (t, lp.warpMode, c.fm), c.wave, lp.warpMode == 1 ? d * (1.0f + c.fm * 6.0f) : d);
                    t += d; if (t >= 1.0f) t -= 1.0f;
                    oL += v * c.uniPanL[u]; oR += v * c.uniPanR[u];
                }
                oL *= c.uniNorm; oR *= c.uniNorm;
                break;
            }
            case engOrchestral:
            {
                float sL = 0, sR = 0;
                for (int u = 0; u < c.nUni; ++u)
                {
                    const float d = inc * c.uniRatio[u];
                    float& t = s.phase[u];
                    const float v = saw (t, d);
                    t += d; if (t >= 1.0f) t -= 1.0f;
                    sL += v * c.uniPanL[u]; sR += v * c.uniPanR[u];
                }
                sL *= c.uniNorm; sR *= c.uniNorm;
                const float wb = clamp01 (1.0f - c.wave * 2.0f), wc = clamp01 (c.wave * 2.0f - 1.0f), ws = 1.0f - wb - wc;
                s.brassL.a = c.brassA; s.brassR.a = c.brassA;
                const float bL = s.brassL.lp (sL) * 1.4f, bR = s.brassR.lp (sR) * 1.4f;
                float cL = sL, cR = sR;
                if (wc > 0) formant (s, c, cL, cR);
                const float bow = noise.bi() * 0.03f;
                oL = wb * bL + ws * (sL * 0.8f + bow) + wc * cL;
                oR = wb * bR + ws * (sR * 0.8f + bow) + wc * cR;
                oL = std::round (oL * 512.0f) / 512.0f; oR = std::round (oR * 512.0f) / 512.0f;   // cheap-workstation grit
                break;
            }
            case engModal:
            {
                float v = 0;
                for (int k = 0; k < s.nModes; ++k)
                {
                    if (inc * s.modeRatio[k] < 0.45f) v += std::sin (twoPi * s.modePh[k]) * s.modeAmp[k];
                    s.modePh[k] += inc * s.modeRatio[k]; s.modePh[k] -= std::floor (s.modePh[k]);
                    s.modeAmp[k] *= s.modeCoef[k];
                }
                oL = oR = v * 0.8f;
                break;
            }
            case engSub:
            default:
            {
                float& t = s.phase[0];
                const float drv = 1.0f + c.wave * 6.0f;
                const float v = std::tanh (std::sin (twoPi * t) * drv) / std::tanh (drv);
                t += inc; if (t >= 1.0f) t -= 1.0f;
                oL = oR = v;
                break;
            }
        }
    }

    void formant (LayerState& s, const LayerCtl& c, float& l, float& r)
    {
        float vl = 0, vr = 0;
        for (int k = 0; k < 3; ++k)
        {
            s.form[k].tick (c.formC[k], l);     vl += s.form[k].bp * c.formC[k].k * c.formG[k];
            s.form[k + 3].tick (c.formC[k], r); vr += s.form[k + 3].bp * c.formC[k].k * c.formG[k];
        }
        l = vl * 1.6f; r = vr * 1.6f;
    }

    void excitePluck (LayerState& s, const LayerParams& lp)
    {
        const float freq = std::clamp (semisToHz ((float) note + (float) (lp.octave * 12 + lp.semi)), 20.0f, sr * 0.45f);
        const int len = std::min (16000, (int) (sr / freq) + 2);
        const float bright = 0.15f + 0.85f * lp.wave;
        float z = 0;
        for (int j = 0; j < len; ++j)
        {
            z += (noise.bi() - z) * bright;
            s.ks[(size_t) ((s.ksW - len + j) & 16383)] += z * (0.6f + 0.4f * vel);
        }
    }

    void exciteModal (LayerState& s, const LayerParams& lp, float decay)
    {
        static const float kal[] { 1.0f, 5.4f, 11.3f, 18.9f }, kalA[] { 1.0f, 0.35f, 0.15f, 0.08f };
        static const float mar[] { 1.0f, 3.93f, 9.24f, 15.5f }, marA[] { 1.0f, 0.5f, 0.2f, 0.1f };
        static const float bel[] { 0.5f, 1.0f, 1.19f, 1.56f, 2.0f, 2.74f }, belA[] { 0.6f, 1.0f, 0.7f, 0.5f, 0.4f, 0.3f };
        const float* r; const float* a; int n;
        if (lp.wave < 0.34f)      { r = kal; a = kalA; n = 4; }
        else if (lp.wave < 0.67f) { r = mar; a = marA; n = 4; }
        else                      { r = bel; a = belA; n = 6; }
        s.nModes = n;
        const float bright = 0.3f + lp.fmAmt * 1.4f;
        const float strike = 0.6f + 0.4f * vel;
        for (int k = 0; k < n; ++k)
        {
            s.modeRatio[k] = r[k];
            s.modeAmp[k] += a[k] * (k == 0 ? 1.0f : bright) * strike;
            const float t60 = std::max (0.1f, decay * 1.5f) / (1.0f + r[k] * 0.2f);
            s.modeCoef[k] = std::pow (0.001f, 1.0f / (t60 * sr));
        }
    }

    float sr = 44100;
    LayerState ls[2];
    LayerCtl ctl[2];
    float subPhase = 0, ghostPhase = 0, ghostEnv = 0, tapePhase = 0, noteRand = 0, bendState = 0;
    float curSemi = 60, targetSemi = 60, noteTime = 0, relTime = 0;
    float modPitch = 0, ampMod = 1, panL = 1, panR = 1, effSub = 0, effReso = 0, ladG = 0.5f;
    float aliveCents = 0, aliveAtk = 1, aliveCut = 0, alivePan = 0, aliveGain = 1;
    float driftPh1 = 0, driftPh2 = 0, driftF1 = 0.2f, driftF2 = 1.0f;
    uint32_t ctlIdx = 0;
    SvfCoef fc;
    SvfState fl, fr;
    Ladder ladL, ladR;
    Rng noise;
};

class SynthEngine
{
public:
    static constexpr int numVoices = 32;

    void prepare (float sr)
    {
        for (int i = 0; i < numVoices; ++i) voices[(size_t) i].prepare (sr, i);
        allOff (true);
        lastSemi = -1; stamp = 0; sustainDown = false;
    }

    void noteOn (int n, float vel, const VoiceParams& p)
    {
        ++stamp;
        if (p.mono)
        {
            removeFromStack (n);
            if (stackSize < (int) stack.size()) stack[(size_t) stackSize++] = n;
            auto& v = voices[0];
            for (int i = 1; i < numVoices; ++i) if (voices[(size_t) i].active) voices[(size_t) i].release();
            if (v.active && v.held && ! v.releasing() && p.legato) v.legatoTo (n);
            else v.start (n, vel, p, v.active ? lastSemi : (p.glide > 0.0005f ? lastSemi : -1.0f), stamp);
            lastSemi = (float) n;
            return;
        }

        Voice* target = nullptr;
        for (auto& v : voices) if (v.active && v.note == n) { target = &v; break; }
        if (! target) for (auto& v : voices) if (! v.active) { target = &v; break; }
        if (! target)
        {
            float best = 1e9f;
            for (auto& v : voices) if (v.releasing() && v.level() < best) { best = v.level(); target = &v; }
        }
        if (! target)
        {
            uint32_t oldest = UINT32_MAX;
            for (auto& v : voices) if (v.age < oldest) { oldest = v.age; target = &v; }
        }
        target->start (n, vel, p, lastSemi, stamp);
        lastSemi = (float) n;
    }

    void noteOff (int n, const VoiceParams& p)
    {
        if (p.mono)
        {
            removeFromStack (n);
            auto& v = voices[0];
            if (v.active && v.held && v.note == n)
            {
                if (stackSize > 0)
                {
                    const int back = stack[(size_t) stackSize - 1];
                    if (p.legato) v.legatoTo (back); else v.start (back, v.vel, p, lastSemi, ++stamp);
                    lastSemi = (float) back;
                }
                else if (sustainDown) v.sustained = true;
                else v.release();
            }
            return;
        }
        for (auto& v : voices)
            if (v.active && v.held && v.note == n)
            {
                if (sustainDown) v.sustained = true; else v.release();
            }
    }

    void polyAftertouch (int n, float value)
    {
        for (auto& v : voices) if (v.active && v.note == n) v.polyAT = value;
    }

    void setSustain (bool down)
    {
        sustainDown = down;
        if (! down)
            for (auto& v : voices) if (v.active && v.sustained) v.release();
    }

    void allOff (bool hard)
    {
        stackSize = 0;
        for (auto& v : voices) { if (hard) v.kill(); else if (v.active) v.release(); }
    }

    void render (float* L, float* R, float* G, int num, const VoiceParams& p, const float* lfo1, const float* lfo2)
    {
        for (auto& v : voices) v.render (L, R, G, num, p, lfo1, lfo2);
    }

    void activeNotes (std::array<bool, 128>& out) const
    {
        out.fill (false);
        for (auto& v : voices) if (v.active && v.note >= 0 && ! v.releasing()) out[(size_t) v.note] = true;
    }

private:
    void removeFromStack (int n)
    {
        int w = 0;
        for (int i = 0; i < stackSize; ++i) if (stack[(size_t) i] != n) stack[(size_t) w++] = stack[(size_t) i];
        stackSize = w;
    }

    std::array<Voice, numVoices> voices;
    std::array<int, 64> stack {};
    int stackSize = 0;
    uint32_t stamp = 0;
    float lastSemi = -1;
    bool sustainDown = false;
};
} // namespace kk
