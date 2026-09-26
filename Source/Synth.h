#pragma once
#include "DspUtil.h"
#include "Params.h"
#include <array>

namespace kk
{
// Snapshot of everything a voice needs, filled once per block
struct VoiceParams
{
    int   engine = 0, octave = 0, unison = 1;
    float wave = 0, detune = 0, sub = 0, fmRatio = 2, fmAmt = 0.3f;
    float cutoff = 12000, reso = 0.1f, fenv = 0, fdecay = 0.4f;
    float attack = 0.002f, decay = 0.6f, sustain = 0.8f, release = 0.3f, velSens = 0.6f;
    float lfoPitch = 0, lfoFilter = 0, lfoAmp = 0;
    float glide = 0; bool mono = false;
    float ghost = 0, bend = 0, punch = 0;
    float pitchWheelSemis = 0, modWheel = 0;
};

class Voice
{
public:
    bool  active = false, held = false, sustained = false;
    int   note = -1;
    uint32_t age = 0;
    float vel = 0;
    Adsr  amp;

    void prepare (float sampleRate)
    {
        sr = sampleRate;
        ks.assign (16384, 0.0f);
        noise.seed ((uint32_t) (reinterpret_cast<uintptr_t> (this) & 0xffffffff));
    }

    void start (int n, float v, const VoiceParams& p, float glideFromSemi, uint32_t stamp)
    {
        const bool wasActive = active;
        note = n; vel = v; held = true; sustained = false; age = stamp;
        targetSemi = (float) n;
        curSemi = (glideFromSemi >= 0 && p.glide > 0.0005f) ? glideFromSemi : (float) n;
        noteTime = 0; relTime = 0; fEnv = 1.0f; ghostEnv = wasActive ? ghostEnv : 0.0f;
        if (! wasActive)
        {
            for (auto& ph : phase) ph = noise.uni();
            subPhase = 0; modPhase = 0; fbPrev = 0;
            fl.reset(); fr.reset();
            for (auto& s : form) s.reset();
            std::fill (ks.begin(), ks.end(), 0.0f);
            ksPrev = 0;
        }
        tapePhase = noise.uni();
        if (p.engine == engPluck) excite (p);
        active = true;
        amp.set (sr, p.attack, p.decay, p.sustain, p.release);
        amp.on();
    }

    void legatoTo (int n) { note = n; targetSemi = (float) n; }
    void release() { held = false; sustained = false; amp.off(); }
    void kill()    { active = false; held = false; amp.kill(); note = -1; }
    bool releasing() const { return amp.st == Adsr::Release; }

    // Adds into L/R/ghost
    void render (float* L, float* R, float* G, int num, const VoiceParams& p, const float* lfo)
    {
        if (! active) return;
        amp.set (sr, p.attack, p.decay, p.sustain, p.release);

        const int nUni = (p.engine == engVA || p.engine == engVox || p.engine == engFM) ? p.unison : 1;
        float uniRatio[7], uniPanL[7], uniPanR[7];
        for (int u = 0; u < nUni; ++u)
        {
            const float off = nUni == 1 ? 0.0f : ((float) u / (float) (nUni - 1)) * 2.0f - 1.0f;
            uniRatio[u] = std::exp2 (off * p.detune * 45.0f / 1200.0f);
            const float pan = off * 0.85f;
            uniPanL[u] = std::sqrt (0.5f * (1.0f - pan));
            uniPanR[u] = std::sqrt (0.5f * (1.0f + pan));
        }
        const float uniNorm = 1.0f / std::sqrt ((float) nUni);
        const float resK = 2.0f * (1.0f - p.reso * 0.96f);
        const float glideCoef = p.glide > 0.0005f ? std::exp (-1.0f / (p.glide * 0.35f * sr)) : 0.0f;
        const float fDecCoef  = std::exp (-4.6f / std::max (1.0f, p.fdecay * sr));
        const float velGain   = 1.0f - p.velSens + p.velSens * vel;
        const float dt        = 1.0f / sr;
        const float relDive   = std::max (0.12f, p.release * 0.9f);

        // formant targets (Vox)
        static const float vowels[5][3] { { 730, 1090, 2440 }, { 530, 1840, 2480 }, { 270, 2290, 3010 },
                                          { 570, 840, 2410 }, { 300, 870, 2240 } };
        SvfCoef formC[3];
        if (p.engine == engVox)
        {
            const float vp = p.wave * 3.999f; const int vi = (int) vp; const float vf = vp - (float) vi;
            for (int k = 0; k < 3; ++k)
                formC[k].set (vowels[vi][k] + (vowels[vi + 1][k] - vowels[vi][k]) * vf, 0.12f, sr);
        }
        float harmW[8]; float harmSum = 0;
        if (p.engine == engOrgan)
            for (int h = 0; h < 8; ++h) { harmW[h] = std::pow (1.0f / (float) (h + 1), 2.0f - 1.8f * p.wave); harmSum += harmW[h]; }

        const float ksG = std::pow (0.001f, 1.0f / (semisToHz (targetSemi + (float) (p.octave * 12)) * std::max (0.05f, p.decay * 1.5f)));
        SvfCoef fc;
        for (int i = 0; i < num; ++i)
        {
            // --- pitch -----------------------------------------------------
            curSemi = glideCoef > 0 ? targetSemi + (curSemi - targetSemi) * glideCoef : targetSemi;
            float semi = curSemi + (float) (p.octave * 12) + p.pitchWheelSemis;
            const float vibRamp = std::min (1.0f, noteTime * 3.0f);
            semi += lfo[i] * (p.lfoPitch + p.modWheel * 0.5f) * 1.5f * vibRamp;
            if (p.bend > 0)
            {
                semi -= p.bend * 2.5f * std::exp (-noteTime * 16.0f);                            // scoop in
                if (releasing()) semi -= p.bend * 24.0f * std::min (1.0f, relTime / relDive);     // end-of-note dive
                if (p.bend > 0.6f)                                                                // broken tape
                {
                    const float d = (p.bend - 0.6f) * 2.5f;
                    semi += d * (0.45f * std::sin (twoPi * (0.6f * noteTime + tapePhase))
                               + 0.2f * std::sin (twoPi * (2.3f * noteTime + tapePhase * 3.0f)));
                }
            }
            if (p.punch > 0) semi += p.punch * 14.0f * std::exp (-noteTime * 45.0f);
            if (p.engine == engSub) semi += p.fmAmt * 24.0f * std::exp (-noteTime * 22.0f);

            const float freq = std::clamp (semisToHz (semi), 8.0f, sr * 0.45f);
            const float inc  = freq / sr;

            // --- oscillators ---------------------------------------------
            float oL = 0, oR = 0;
            switch (p.engine)
            {
                case engVA:
                case engVox:
                {
                    const float w = p.engine == engVox ? 0.0f : p.wave;
                    for (int u = 0; u < nUni; ++u)
                    {
                        const float d = inc * uniRatio[u];
                        float& t = phase[u];
                        float s;
                        const float saw = 2.0f * t - 1.0f - polyBlep (t, d);
                        if (w <= 0.5f)
                        {
                            const float t2 = t + 0.5f >= 1.0f ? t - 0.5f : t + 0.5f;
                            const float sq = (t < 0.5f ? 1.0f : -1.0f) + polyBlep (t, d) - polyBlep (t2, d);
                            s = saw + (sq - saw) * (w * 2.0f);
                        }
                        else
                        {
                            const float pw = 0.5f - (w - 0.5f) * 0.8f;
                            float t2 = t + 1.0f - pw; if (t2 >= 1.0f) t2 -= 1.0f;
                            s = (t < pw ? 1.0f : -1.0f) + polyBlep (t, d) - polyBlep (t2, d);
                        }
                        t += d; if (t >= 1.0f) t -= 1.0f;
                        oL += s * uniPanL[u]; oR += s * uniPanR[u];
                    }
                    oL *= uniNorm * 1.2f; oR *= uniNorm * 1.2f;
                    if (p.engine == engVox)
                    {
                        float vl = 0, vr = 0;
                        for (int k = 0; k < 3; ++k)
                        {
                            form[k].tick (formC[k], oL);     vl += form[k].bp * formC[k].k * (k == 0 ? 1.0f : k == 1 ? 0.7f : 0.4f);
                            form[k + 3].tick (formC[k], oR); vr += form[k + 3].bp * formC[k].k * (k == 0 ? 1.0f : k == 1 ? 0.7f : 0.4f);
                        }
                        oL = vl * 1.6f; oR = vr * 1.6f;
                    }
                    break;
                }
                case engFM:
                {
                    const float index = p.fmAmt * 7.0f * (0.25f + 0.75f * fEnv) * (0.5f + 0.5f * vel);
                    const float mod = std::sin (twoPi * modPhase + p.wave * 1.5f * fbPrev);
                    fbPrev = mod;
                    modPhase += inc * p.fmRatio; modPhase -= std::floor (modPhase);
                    for (int u = 0; u < nUni; ++u)
                    {
                        const float s = std::sin (twoPi * phase[u] + index * mod);
                        phase[u] += inc * uniRatio[u]; if (phase[u] >= 1.0f) phase[u] -= 1.0f;
                        oL += s * uniPanL[u]; oR += s * uniPanR[u];
                    }
                    oL *= uniNorm; oR *= uniNorm;
                    break;
                }
                case engPluck:
                {
                    const float damp = 0.5f + 0.5f * p.wave;
                    const float len  = sr / freq - (1.0f - damp);
                    const float rp = (float) ksW - len;
                    int i0 = (int) std::floor (rp); const float fr = rp - (float) i0;
                    const float a = ks[(size_t) (i0 & 16383)], b = ks[(size_t) ((i0 + 1) & 16383)];
                    const float y = a + (b - a) * fr;
                    ks[(size_t) ksW] = ksG * (damp * y + (1.0f - damp) * ksPrev);
                    ksPrev = y;
                    ksW = (ksW + 1) & 16383;
                    oL = oR = y * 1.4f;
                    break;
                }
                case engOrgan:
                {
                    float s = 0; float& t = phase[0];
                    for (int h = 0; h < 8; ++h)
                        if (inc * (float) (h + 1) < 0.45f) s += harmW[h] * std::sin (twoPi * t * (float) (h + 1));
                    t += inc; if (t >= 1.0f) t -= 1.0f;
                    oL = oR = s / harmSum * 1.3f;
                    break;
                }
                case engFlute:
                {
                    float& t = phase[0];
                    const float s = std::sin (twoPi * t) + 0.18f * std::sin (twoPi * 2.0f * t) + 0.06f * std::sin (twoPi * 3.0f * t);
                    t += inc; if (t >= 1.0f) t -= 1.0f;
                    const float breath = noise.bi() * p.wave * 0.35f * (0.4f + 0.6f * std::exp (-noteTime * 6.0f));
                    oL = oR = s + breath;
                    break;
                }
                case engSub:
                default:
                {
                    float& t = phase[0];
                    const float drv = 1.0f + p.wave * 6.0f;
                    const float s = std::tanh (std::sin (twoPi * t) * drv) / std::tanh (drv);
                    t += inc; if (t >= 1.0f) t -= 1.0f;
                    oL = oR = s;
                    break;
                }
            }

            // --- filter --------------------------------------------------
            if ((i & 7) == 0)
            {
                float oct = p.fenv * fEnv * 6.0f + p.lfoFilter * 4.0f * (0.5f * lfo[i] - 0.5f)
                          + (curSemi - 60.0f) / 12.0f * 0.4f + p.punch * fEnv * 2.0f;
                fc.set (p.cutoff * std::exp2 (oct), resK, sr);
            }
            fl.tick (fc, oL); fr.tick (fc, oR);
            float yL = fl.lp, yR = fr.lp;

            // --- sub -----------------------------------------------------
            if (p.sub > 0.001f)
            {
                const float s = std::sin (twoPi * subPhase) * p.sub * 0.9f;
                subPhase += inc * 0.5f; if (subPhase >= 1.0f) subPhase -= 1.0f;
                yL += s; yR += s;
            }

            const float env = amp.next();
            float gAmp = env * velGain * (1.0f - p.lfoAmp * (0.5f - 0.5f * lfo[i]));
            L[i] += yL * gAmp * 0.35f;
            R[i] += yR * gAmp * 0.35f;

            // --- ghost layer (octave up, slow swell) --------------------
            if (p.ghost > 0.001f)
            {
                ghostEnv += (1.0f - ghostEnv) * (1.0f / (0.35f * sr));
                const float gs = std::sin (twoPi * ghostPhase) * 0.8f + 0.2f * std::sin (twoPi * 2.0f * ghostPhase);
                ghostPhase += inc * 2.003f; if (ghostPhase >= 1.0f) ghostPhase -= 1.0f;
                G[i] += gs * ghostEnv * env * velGain * p.ghost * 0.22f;
            }

            fEnv *= fDecCoef;
            noteTime += dt;
            if (releasing()) relTime += dt;
            if (amp.st == Adsr::Idle) { active = false; held = false; note = -1; break; }
        }
    }

    float level() const { return amp.v; }
    float currentSemi() const { return curSemi; }

private:
    void excite (const VoiceParams& p)
    {
        const float freq = std::clamp (semisToHz ((float) note + (float) (p.octave * 12)), 20.0f, sr * 0.45f);
        const int len = std::min (16000, (int) (sr / freq) + 2);
        const float bright = 0.15f + 0.85f * p.wave;
        float z = 0;
        for (int j = 0; j < len; ++j)
        {
            z += (noise.bi() - z) * bright;
            ks[(size_t) ((ksW - len + j) & 16383)] += z * (0.6f + 0.4f * vel);
        }
    }

    float sr = 44100;
    float phase[7] {}, subPhase = 0, modPhase = 0, fbPrev = 0, ghostPhase = 0, ghostEnv = 0, tapePhase = 0;
    float curSemi = 60, targetSemi = 60, noteTime = 0, relTime = 0, fEnv = 0;
    SvfState fl, fr, form[6];
    std::vector<float> ks; int ksW = 0; float ksPrev = 0;
    Rng noise;
};

class SynthEngine
{
public:
    static constexpr int numVoices = 16;

    void prepare (float sr) { for (auto& v : voices) v.prepare (sr); allOff (true); }

    void noteOn (int n, float vel, const VoiceParams& p)
    {
        ++stamp;
        if (p.mono)
        {
            removeFromStack (n);
            if (stackSize < (int) stack.size()) stack[(size_t) stackSize++] = n;
            auto& v = voices[0];
            for (int i = 1; i < numVoices; ++i) if (voices[(size_t) i].active) voices[(size_t) i].release();
            if (v.active && v.held && ! v.releasing()) v.legatoTo (n);
            else v.start (n, vel, p, lastSemi, stamp);
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
                if (stackSize > 0) { v.legatoTo (stack[(size_t) stackSize - 1]); lastSemi = (float) v.note; }
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

    void render (float* L, float* R, float* G, int num, const VoiceParams& p, const float* lfo)
    {
        for (auto& v : voices) v.render (L, R, G, num, p, lfo);
    }

    int activeNotes (std::array<bool, 128>& out) const
    {
        int c = 0;
        out.fill (false);
        for (auto& v : voices) if (v.active && v.note >= 0 && ! v.releasing()) { out[(size_t) v.note] = true; ++c; }
        return c;
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
