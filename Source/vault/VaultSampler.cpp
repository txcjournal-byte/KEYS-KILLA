#include "VaultSampler.h"
#include <cmath>

namespace vault
{
namespace
{
double besselI0 (double x)
{
    double sum = 1.0, term = 1.0;
    for (int k = 1; k < 40; ++k) { term *= (x / (2.0 * k)) * (x / (2.0 * k)); sum += term; if (term < 1.0e-12 * sum) break; }
    return sum;
}
constexpr float i16 = 1.0f / 32767.0f;
constexpr float ln1000 = 6.907755f;    // -60 dB
}

const SincTables& SincTables::get()
{
    static const std::unique_ptr<SincTables> t = []
    {
        auto s = std::make_unique<SincTables>();
        constexpr double beta = 6.5;
        const double i0b = besselI0 (beta);
        for (int t2 = 0; t2 < numTables; ++t2)
        {
            const double fc = 0.92 / std::pow (2.0, t2 / 12.0);   // cutoff relative to the source Nyquist
            for (int ph = 0; ph <= phases; ++ph)
            {
                const double frac = (double) ph / phases;
                double sum = 0.0, row[taps];
                for (int j = 0; j < taps; ++j)
                {
                    const double x = (double) (j - (half - 1)) - frac;   // distance of tap j from the read position
                    const double r = x / half;
                    const double w = std::abs (r) >= 1.0 ? 0.0 : besselI0 (beta * std::sqrt (1.0 - r * r)) / i0b;
                    const double a = juce::MathConstants<double>::pi * fc * x;
                    const double sinc = std::abs (a) < 1.0e-9 ? 1.0 : std::sin (a) / a;
                    row[j] = fc * sinc * w;
                    sum += row[j];
                }
                for (int j = 0; j < taps; ++j) s->k[t2][ph][j] = (float) (row[j] / sum);   // unity DC gain at every phase
            }
        }
        return s;
    }();
    return *t;
}

//==============================================================================
void SamplerPart::prepare (double sampleRate)
{
    sr = sampleRate > 0 ? sampleRate : 44100.0;
    SincTables::get();
    for (int i = 0; i < numSampleParams; ++i) p[i] = sampleParams()[(size_t) i].def;
    allOff (true);
    sustain = false;
}

void SamplerPart::setSource (const SampleInstrumentData* d, int articulation)
{
    if (d != inst || articulation != art) allOff (true);
    inst = d;
    art = d != nullptr ? juce::jlimit (0, std::max (0, (int) d->articulations.size() - 1), articulation) : 0;
}

void SamplerPart::setParams (const float* sp) { for (int i = 0; i < numSampleParams; ++i) p[i] = sp[i]; }

void SamplerPart::setControl (float cutoffMul, float vibratoCents, float pitchWheel, float extraSemis)
{
    const bool cutChanged = std::abs (cutoffMul - cutMul) > 0.003f * cutMul;
    cutMul = cutoffMul; vibCents = vibratoCents; wheel = pitchWheel; extraSt = extraSemis;
    for (auto& v : voices)
        if (v.stage != Voice::off) { updatePitch (v); if (cutChanged) updateFilter (v); }
}

int SamplerPart::activeVoices() const
{
    int n = 0;
    for (auto& v : voices) n += v.stage != Voice::off ? 1 : 0;
    return n;
}

void SamplerPart::updatePitch (Voice& v)
{
    float semis = wheel * p[spBendRange] + vibCents * std::sin (vibPhase) * 0.01f;
    if (extraSt != 0.0f) semis += extraSt;
    v.inc = v.incBase * std::exp2 (semis / 12.0);
    v.table = SincTables::tableFor (v.inc);
}

void SamplerPart::updateFilter (Voice& v)
{
    const int type = (int) p[spFilterType];
    float fc = p[spCutoff] * cutMul * std::exp2 (-4.0f * p[spVelCutoff] * (1.0f - v.vel));
    fc = juce::jlimit (20.0f, 20000.0f, fc);
    v.filt = type != 0 || fc < 19000.0f;
    if (v.filt) v.fc.set (fc, 1.414f - 1.3f * juce::jlimit (0.0f, 1.0f, p[spReso]), (float) sr);   // k = 1/Q: Butterworth at reso 0
}

SamplerPart::Voice* SamplerPart::freeVoice()
{
    // polyphony: the oldest sounding note (released ones first) fades out fast when the limit is reached
    const int limit = juce::jlimit (1, maxVoices, (int) p[spPolyphony]);
    int playing = 0;
    for (auto& v : voices) playing += (v.stage != Voice::off && v.stage != Voice::fade && ! v.isRelease) ? 1 : 0;
    while (playing >= limit)
    {
        Voice* oldest = nullptr;
        for (auto& v : voices)
            if (v.stage != Voice::off && v.stage != Voice::fade && ! v.isRelease)
            {
                const bool vRel = v.stage == Voice::release, oRel = oldest != nullptr && oldest->stage == Voice::release;
                if (oldest == nullptr || (vRel && ! oRel) || (vRel == oRel && v.age < oldest->age)) oldest = &v;
            }
        if (oldest == nullptr) break;
        oldest->stage = Voice::fade; oldest->fadeStep = oldest->env / (float) (0.004 * sr) + 1.0e-7f;
        --playing;
    }
    for (auto& v : voices) if (v.stage == Voice::off) return &v;
    // every slot busy (fades + release samples): take the quietest
    Voice* q = &voices[0];
    for (auto& v : voices) if (v.env * v.gain < q->env * q->gain) q = &v;
    q->stage = Voice::off;
    return q;
}

void SamplerPart::startVoice (Voice& v, const SampleInstrumentData::Zone& z, int note, float vel, float gain, bool isRelease)
{
    const auto& a = inst->articulations[(size_t) art];
    v.z = &z;
    v.d0 = z.audio->channel (0);
    v.d1 = z.audio->channels > 1 ? z.audio->channel (1) : nullptr;
    v.frames = z.audio->frames;
    v.note = note; v.vel = vel; v.gain = gain * z.gain * i16;
    const int startOffset = isRelease ? 0 : (int) std::lround (p[spStart] * 0.001 * z.audio->sampleRate);
    v.pos = (double) juce::jlimit (0, std::max (0, v.frames - 2), z.start + startOffset);
    v.incBase = std::exp2 ((note - z.root) / 12.0 + (z.fine + (isRelease ? 0.0f : p[spTune])) / 1200.0) * z.audio->sampleRate / sr;
    updatePitch (v);
    v.isRelease = isRelease;
    const float oneShotParam = p[spOneShot];
    v.oneShot = isRelease || (oneShotParam < -0.5f ? a.oneShot : oneShotParam > 0.5f);
    if (z.looped) v.oneShot = false;
    v.held = ! isRelease; v.sustained = false;
    v.age = ++ageCounter; v.startedAt = clock;
    // envelope: natural onset (0.5 ms declick, 3 ms when the start is moved into the sound) or the ADSR override
    const float atk = p[spAttack] > 0.0f ? p[spAttack] : (v.pos > (double) z.start + 0.5 ? 0.003f : 0.0005f);
    v.env = 0.0f; v.atkStep = 1.0f / std::max (1.0f, atk * (float) sr);
    v.susLevel = p[spDecay] > 0.0f ? p[spSustain] : 1.0f;
    v.decMul = p[spDecay] > 0.0f ? std::exp (-ln1000 / (p[spDecay] * (float) sr)) : 1.0f;
    const float rel = p[spRelease] > 0.0f ? p[spRelease] : a.release;
    v.relMul = std::exp (-ln1000 / (std::max (0.005f, rel) * (float) sr));
    v.stage = Voice::attack;
    for (auto& f : v.f) f.reset();
    updateFilter (v);
}

void SamplerPart::noteOn (int note, float velocity)
{
    if (inst == nullptr || ! juce::isPositiveAndBelow (note, 128)) return;
    const int vel127 = juce::jlimit (1, 127, (int) std::lround (velocity * 127.0f));
    const auto* z = inst->pick (art, note, vel127, rr[note], false);
    ++rr[note];
    if (z == nullptr) return;
    const float vel = juce::jlimit (0.02f, 1.0f, velocity);
    // the same key struck again: the old note fades over (at most) 250 ms - no build-up, no cut
    int sameKey = 0;
    for (auto& v : voices)
        if (v.stage != Voice::off && v.stage != Voice::fade && ! v.isRelease && v.note == note)
        {
            if (! v.oneShot) { v.stage = Voice::release; v.relMul = std::min (v.relMul, std::exp (-ln1000 / (0.25f * (float) sr))); v.held = v.sustained = false; }
            else if (++sameKey >= 2) { v.stage = Voice::fade; v.fadeStep = v.env / (float) (0.02 * sr) + 1.0e-7f; }
        }
    auto* v = freeVoice();
    const float velGain = std::pow (vel, 1.6f * p[spVelSens]);
    startVoice (*v, *z, note, vel, velGain, false);
    noteVel[note] = vel;
    noteStart[note] = clock;
}

void SamplerPart::releaseVoice (Voice& v)
{
    if (v.oneShot || v.stage == Voice::off || v.stage == Voice::fade) { v.held = false; return; }
    v.stage = Voice::release;
    v.held = v.sustained = false;
}

void SamplerPart::noteOff (int note)
{
    if (! juce::isPositiveAndBelow (note, 128)) return;
    bool any = false;
    for (auto& v : voices)
        if (v.stage != Voice::off && v.held && v.note == note && ! v.isRelease)
        {
            any = true;
            if (sustain) { v.held = false; v.sustained = true; }
            else releaseVoice (v);
        }
    // release samples (piano dampers, string releases): only for a real key release, quieter the longer the note was held
    if (any && ! sustain && inst != nullptr && p[spReleaseSamples] > 0.5f)
    {
        const auto& a = inst->articulations[(size_t) art];
        if (! a.releaseZones.empty())
            if (const auto* z = inst->pick (art, note, juce::jlimit (1, 127, (int) std::lround (noteVel[note] * 127.0f)), rr[note], true))
            {
                const float heldSec = (float) (clock - noteStart[note]) / (float) sr;
                const float g = a.releaseGain * std::pow (noteVel[note], 1.6f * p[spVelSens])
                                * std::pow (10.0f, -std::min (30.0f, heldSec * a.releaseDecayDb) / 20.0f);
                startVoice (*freeVoice(), *z, note, noteVel[note], g, true);
            }
    }
}

void SamplerPart::setSustain (bool down)
{
    sustain = down;
    if (! down)
        for (auto& v : voices)
            if (v.sustained) releaseVoice (v);
}

void SamplerPart::allOff (bool hard)
{
    for (auto& v : voices)
    {
        if (hard) { v.stage = Voice::off; v.env = 0; }
        else if (v.stage != Voice::off) { v.oneShot = false; releaseVoice (v); }
        v.held = v.sustained = false;
    }
}

void SamplerPart::render (float* l, float* r, int n)
{
    if (n <= 0) return;
    for (auto& v : voices)
        if (v.stage != Voice::off) renderVoice (v, l, r, n);
    clock += n;
    vibPhase += juce::MathConstants<float>::twoPi * p[spVibRate] * (float) n / (float) sr;
    if (vibPhase > juce::MathConstants<float>::twoPi) vibPhase -= juce::MathConstants<float>::twoPi;
    // stereo width of the part (M/S): 1 = as recorded
    const float w = p[spWidth];
    if (std::abs (w - 1.0f) > 1.0e-3f)
        for (int i = 0; i < n; ++i)
        {
            const float m = (l[i] + r[i]) * 0.5f, s = (l[i] - r[i]) * 0.5f * w;
            l[i] = m + s; r[i] = m - s;
        }
}

void SamplerPart::renderVoice (Voice& v, float* l, float* r, int n)
{
    const auto& tab = SincTables::get().k[v.table];
    const auto* z = v.z;
    const bool looped = z->looped && ! v.isRelease;
    const double loopLen = (double) (z->loopEnd - z->loopStart);
    const double xfStart = (double) (z->loopEnd - z->xfade);
    const float xfInv = z->xfade > 0 ? 1.0f / (float) z->xfade : 0.0f;
    const int16_t* d0 = v.d0; const int16_t* d1 = v.d1;
    const double endPos = (double) (v.frames - 1);

    auto interp = [&] (double pos, float& outL, float& outR)
    {
        const int ip = (int) pos;
        const float fph = (float) (pos - ip) * (float) SincTables::phases;
        const int ph = (int) fph;
        const float pf = fph - (float) ph;
        const float* k0 = tab[ph]; const float* k1 = tab[ph + 1];
        const int16_t* a = d0 + ip - (SincTables::half - 1);
        float acc0 = 0.0f, acc1 = 0.0f;
        if (d1 != nullptr)
        {
            const int16_t* b = d1 + ip - (SincTables::half - 1);
            for (int j = 0; j < SincTables::taps; ++j) { const float w = k0[j] + pf * (k1[j] - k0[j]); acc0 += w * (float) a[j]; acc1 += w * (float) b[j]; }
            outL = acc0; outR = acc1;
        }
        else
        {
            for (int j = 0; j < SincTables::taps; ++j) acc0 += (k0[j] + pf * (k1[j] - k0[j])) * (float) a[j];
            outL = outR = acc0;
        }
    };

    for (int i = 0; i < n; ++i)
    {
        // envelope
        switch (v.stage)
        {
            case Voice::attack:
                v.env += v.atkStep;
                if (v.env >= 1.0f) { v.env = 1.0f; v.stage = v.decMul < 1.0f ? Voice::decay : Voice::hold; }
                break;
            case Voice::decay:   v.env = v.susLevel + (v.env - v.susLevel) * v.decMul; break;
            case Voice::hold:    break;
            case Voice::release: v.env *= v.relMul; if (v.env < 1.0e-4f) { v.stage = Voice::off; return; } break;
            case Voice::fade:    v.env -= v.fadeStep; if (v.env <= 0.0f) { v.env = 0.0f; v.stage = Voice::off; return; } break;
            case Voice::off:     return;
        }
        float sL, sR;
        if (looped && v.pos >= xfStart && xfInv > 0.0f)
        {
            // loop crossfade: the end of the loop blends into the frames just before the loop start, so the jump back
            // to loopStart continues exactly where the blend ended
            float aL, aR, bL, bR;
            interp (v.pos, aL, aR);
            interp (v.pos - loopLen, bL, bR);
            const float t = (float) (v.pos - xfStart) * xfInv;
            sL = aL + t * (bL - aL); sR = aR + t * (bR - aR);
        }
        else interp (v.pos, sL, sR);
        if (v.filt)
        {
            const int type = (int) p[spFilterType];
            v.f[0].tick (v.fc, sL); v.f[1].tick (v.fc, sR);
            sL = type == 1 ? v.f[0].hp : type == 2 ? v.f[0].bp : v.f[0].lp;
            sR = type == 1 ? v.f[1].hp : type == 2 ? v.f[1].bp : v.f[1].lp;
        }
        const float g = v.env * v.gain;
        l[i] += sL * g; r[i] += sR * g;

        v.pos += v.inc;
        if (looped) { if (v.pos >= (double) z->loopEnd) v.pos -= loopLen; }
        else if (v.pos >= endPos) { v.stage = Voice::off; return; }
    }
}
} // namespace vault
