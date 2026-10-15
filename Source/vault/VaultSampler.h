#pragma once
#include "VaultLibrary.h"
#include "../DspUtil.h"

// CHROME VAULT sample part: a multi-zone sampler playing one articulation of one library instrument.
// 64 voices (+8 spare slots for the fast fades of stolen voices), 16-tap windowed-sinc interpolation with an anti-alias
// cutoff that follows the pitch ratio, sustain loops with a runtime crossfade, release fade or release samples,
// ADSR override, 12 dB SVF per voice, pitch bend, vibrato. Everything is allocation-free after prepare().
namespace vault
{
// polyphase windowed-sinc kernels (Kaiser, 16 taps, 256 phases + 1 for linear phase interpolation). Table t is for
// read increments up to 2^(t/12) (t = 0: <= 1.0): cutoff = 0.92 x Nyquist / increment, so pitching a sample up by up to
// 12 semitones does not fold its top octave back down (aliasing), pitching down does not leave images.
struct SincTables
{
    static constexpr int taps = 16, half = 8, phases = 256, numTables = 13;
    float k[numTables][phases + 1][taps];
    static const SincTables& get();
    static int tableFor (double increment)
    {
        if (increment <= 1.0) return 0;
        const int t = (int) std::ceil (12.0 * std::log2 (increment) - 1.0e-6);
        return std::min (numTables - 1, std::max (0, t));
    }
};

class SamplerPart
{
public:
    static constexpr int maxVoices = 64, poolSize = maxVoices + 8;

    void prepare (double sampleRate);                  // allocation-free afterwards (there is nothing to allocate anyway)
    // audio thread. The instrument data must stay valid while voices play (the engine holds the handle acquired).
    void setSource (const SampleInstrumentData* data, int articulation);
    const SampleInstrumentData* source() const { return inst; }
    void setParams (const float* sampleParams);        // vault::numSampleParams values (sanitized)
    // per 128-sample chunk: TONE cutoff factor, MOTION vibrato depth (cents), the pitch wheel (-1..1) and a pitch offset
    // in semitones (v3 modulation)
    void setControl (float cutoffMul, float vibratoCents, float pitchWheel, float extraSemis = 0.0f);

    void noteOn (int note, float velocity);            // note includes the part transpose
    void noteOff (int note);
    void setSustain (bool down);
    void allOff (bool hard);                           // hard: silence now, else release everything
    // ADD n samples into l / r (the part's own buffer; the stereo width is applied to this range)
    void render (float* l, float* r, int n);
    int activeVoices() const;

private:
    struct Voice
    {
        const SampleInstrumentData::Zone* z = nullptr;
        const int16_t* d0 = nullptr; const int16_t* d1 = nullptr;
        double pos = 0, incBase = 1, inc = 1;
        int note = -1, frames = 0, table = 0;
        float vel = 1, gain = 1, env = 0, atkStep = 1, susLevel = 1, decMul = 1, relMul = 0.99f, fadeStep = 0;
        enum Stage { off, attack, decay, hold, release, fade } stage = off;
        bool held = false, sustained = false, isRelease = false, oneShot = false;
        uint32_t age = 0; int64_t startedAt = 0;
        kk::SvfCoef fc; kk::SvfState f[2]; bool filt = false;
    };
    void startVoice (Voice& v, const SampleInstrumentData::Zone& z, int note, float vel, float gain, bool isRelease);
    void releaseVoice (Voice& v);
    void updateFilter (Voice& v);
    void updatePitch (Voice& v);
    Voice* freeVoice();
    void renderVoice (Voice& v, float* l, float* r, int n);

    double sr = 44100.0;
    const SampleInstrumentData* inst = nullptr;
    int art = 0;
    float p[numSampleParams] {};
    float cutMul = 1.0f, vibCents = 0.0f, wheel = 0.0f, vibPhase = 0.0f, extraSt = 0.0f;
    bool sustain = false;
    uint32_t ageCounter = 0;
    int64_t clock = 0;
    uint8_t rr[128] {};
    float noteVel[128] {};
    int64_t noteStart[128] {};
    Voice voices[poolSize];
};
} // namespace vault
