// CHROME VAULT electric piano / toy piano models.
//
// The vibrating part (tine + tone bar, reed, steel rod) is a small set of exponentially decaying modes computed with
// complex rotators (exact frequencies, no aliasing, a few multiplies per mode). The character comes from the
// transducer, as in the real instruments:
//   * tine piano: the electromagnetic pickup senses the tine tip moving in front of the pole piece. The flux is a bell
//     shaped function of the tip displacement, Phi(x) = 1 / sqrt(1 + ((x + offset) / h)^2), the output is dPhi/dt.
//     The pickup "voicing" (offset of the tine against the pole) sets the balance of fundamental and 2nd harmonic; a
//     harder strike drives the tip further into the non-linear region ("bark") (Pfeifle 2017, DAFx-17).
//     Higher cantilever modes (ratios 6.27, 17.55 of a clamped-free beam, Fletcher & Rossing ch. 2) give the bell attack.
//   * reed piano: an electrostatic pickup - the reed is one plate of a capacitor; C ~ 1 / (gap - x) is asymmetric, so the
//     output dC/dt gains even harmonics with amplitude: the reed piano's "bark" at forte (Pfeifle 2017), plus a preamp.
//   * felt toy piano: a struck steel rod (clamped-free bar modes, soft felt hammer = weak upper modes), a small box.
// Release: the damper felt (tine / reed) shortens every mode; the toy piano has no dampers (rings, then fades).
#include "BuiltinModels.h"
#include "ModelDsp.h"

namespace vault::models
{
namespace
{
using namespace dsp;

enum class Kind : uint8_t { tine, reed, toy };

struct ModeSpec { float ratio, amp, t60, velAmp; };   // frequency ratio, level, T60 at the reference note, extra level at velocity 1

struct KeysProfile
{
    const char* id; const char* name; Kind kind;
    int refNote; float t60Fund, t60Oct;     // fundamental T60 at refNote, multiplier per octave up
    ModeSpec modes[5]; int numModes;
    float offset, excursionLo, excursionHi; // pickup voicing (tine / reed): offset, displacement at velocity 0 / 1 (units of h)
    float release;                          // T60 after note-off (s)
    float lp, hp;                           // output filter
    float drive;                            // preamp drive default
    float thump;                            // mechanical noise level
    float gainDb, keyDbPerOct;
    int maxVoices;
};

// gainDb / keyDbPerOct: calibrated with VaultModelLab -sweep (velocity 1.0 at C5 peaks about -9 dBFS)
const KeysProfile profiles[] = {
    { "ep.tine.mellow", "Tine Electric Piano", Kind::tine, 60, 6.0f, 0.72f,
      { { 1.0f, 1.0f, 1.0f, 0.0f }, { 1.0007f, 0.10f, 1.6f, 0.0f }, { 6.27f, 0.010f, 0.12f, 0.02f }, { 17.55f, 0.0f, 0.015f, 0.003f } }, 4,
      1.3f, 0.25f, 0.85f, 0.14f, 4500, 40, 0.15f, 0.010f, -3.0f, 0, 16 },
    { "ep.tine.bell", "Bell Tine Electric Piano", Kind::tine, 60, 6.5f, 0.72f,
      { { 1.0f, 1.0f, 1.0f, 0.0f }, { 1.0007f, 0.12f, 1.6f, 0.0f }, { 6.27f, 0.035f, 0.30f, 0.05f }, { 17.55f, 0.004f, 0.04f, 0.008f } }, 4,
      0.45f, 0.30f, 0.95f, 0.14f, 6000, 40, 0.2f, 0.012f, -4.2f, 0, 16 },
    { "ep.reed", "Reed Electric Piano", Kind::reed, 60, 3.2f, 0.70f,
      { { 1.0f, 1.0f, 1.0f, 0.0f }, { 6.1f, 0.012f, 0.05f, 0.02f }, { 1.0004f, 0.05f, 1.3f, 0.0f } }, 3,
      0.0f, 0.12f, 0.75f, 0.11f, 4800, 90, 0.35f, 0.012f, -8.0f, 0, 16 },
    { "keys.toy", "Felt Toy Piano", Kind::toy, 60, 1.9f, 0.75f,
      { { 1.0f, 1.0f, 1.0f, 0.0f }, { 6.27f, 0.20f, 0.50f, 0.15f }, { 17.55f, 0.04f, 0.12f, 0.04f }, { 2.0f, 0.08f, 0.5f, 0.0f } }, 4,
      0.0f, 0.0f, 0.0f, 0.6f, 7000, 60, 0.0f, 0.02f, -12.4f, 0, 16 },
};

const KeysProfile* findProfile (const juce::String& id)
{
    for (auto& p : profiles) if (id == p.id) return &p;
    return nullptr;
}

enum ParamIndex { pTone, pDecay, pRelease, pDrive, pBell, pTrem, pTremRate, pVoicing, pLevel, pVelSens, numParams };

std::vector<ParamInfo> paramsFor (const KeysProfile& p)
{
    return { { "m.tone",     0.0f, 1.0f, 0.5f,  "brightness (0.5 = as designed)" },
             { "m.decay",    0.2f, 4.0f, 1.0f,  "decay time multiplier" },
             { "m.release",  0.02f, 4.0f, p.release, "T60 after note-off, s" },
             { "m.drive",    0.0f, 1.0f, p.drive, "preamp drive" },
             { "m.bell",     0.0f, 2.0f, 1.0f,  "attack bell / upper mode level multiplier" },
             { "m.trem",     0.0f, 1.0f, 0.0f,  "tremolo depth (stereo pan on tine pianos, level on reed / toy)" },
             { "m.tremRate", 0.5f, 10.0f, 4.5f, "tremolo rate, Hz" },
             { "m.voicing",  -1.0f, 1.0f, 0.0f, "pickup voicing: - = rounder, + = more 2nd harmonic / bark" },
             { "m.level",    0.0f, 2.0f, 1.0f,  "output level multiplier" },
             { "m.velSens",  0.0f, 1.0f, 0.75f, "velocity -> level (0 = fixed level, tone still follows velocity)" } };
}

struct Osc { float re = 0, im = 0, cr = 1, ci = 0, amp = 0, ratio = 1, mulHeld = 1, mulRel = 1; };

class KeysVoice final : public ModelVoice
{
public:
    explicit KeysVoice (const KeysProfile& p) : prof (p) {}

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate > 0 ? sampleRate : 48000.0;
        const auto defs = paramsFor (prof);
        for (int i = 0; i < numParams; ++i) par[i] = defs[(size_t) i].def;
        kill();
    }
    void setParams (const float* v, int n) override { for (int i = 0; i < numParams && i < n; ++i) par[i] = v[i]; }

    void noteOn (int midiNote, float velocity, float freqHz) override
    {
        note = midiNote; vel = clampf (velocity, 0.0f, 1.0f);
        hz = std::max (8.0f, freqHz);
        rng.seed ((uint32_t) (midiNote * 104729 + (int) (vel * 127.0f) * 37 + 5));
        const float oct = ((float) midiNote - (float) prof.refNote) / 12.0f;
        const float t0 = prof.t60Fund * std::pow (prof.t60Oct, oct) * par[pDecay];
        const float bell = par[pBell];
        const float tone = std::exp2 ((par[pTone] - 0.5f) * 2.0f);
        numOsc = 0;
        for (int m = 0; m < prof.numModes; ++m)
        {
            const auto& ms = prof.modes[m];
            if (ms.ratio * hz > 0.42 * sr) continue;
            auto& o = osc[numOsc++];
            const bool upper = ms.ratio > 2.5f;
            o.ratio = ms.ratio;
            o.amp = (ms.amp + ms.velAmp * vel * vel) * (upper ? bell * tone : 1.0f);
            // upper modes: their own T60 at the reference note, shorter higher up (radiation + clamping losses)
            const float t = upper ? ms.t60 * std::pow (0.7f, oct) : t0 * ms.t60;
            o.mulHeld = t60ToMul (std::max (0.004f, t), sr);
            const float rel = std::max (0.02f, par[pRelease]) * (upper ? 0.4f : 1.0f);
            o.mulRel = t60ToMul (std::min (t, rel), sr);
            o.re = 0.0f; o.im = 0.0f;
        }
        setRot();
        // the hammer: modes start with velocity (sin phase), the contact time smooths the onset (neoprene tip ~0.5-1 ms)
        for (int i = 0; i < numOsc; ++i) { osc[i].re = osc[i].amp; osc[i].im = 0.0f; }
        attack = 0.0f;
        attackK = 1.0f - std::exp (-1.0f / (float) ((prof.kind == Kind::toy ? 0.0012 : 0.0007) * sr));
        const float exc = prof.excursionLo + (prof.excursionHi - prof.excursionLo) * vel;
        excursion = exc * (1.0f + 0.25f * par[pVoicing]);
        offset = prof.offset * (1.0f - 0.35f * par[pVoicing]);
        phiPrev = pickup (0.0f);
        wNorm = 1.0f / (twoPi * hz / (float) sr);
        thump = prof.thump * (0.3f + 0.7f * vel); thumpMul = t60ToMul (0.03f, sr);
        thumpLp.setHz (prof.kind == Kind::toy ? 1800.0f : 700.0f, sr); thumpLp.reset();
        // output chain
        lp.lowpass (prof.lp * tone, 0.7, sr); lp.reset();
        hp.highpass (prof.hp, 0.7, sr); hp.reset();
        body.peak (prof.kind == Kind::toy ? 650.0 : 160.0, 0.9, prof.kind == Kind::reed ? 1.5 : 2.5, sr); body.reset();
        drive = 1.0f + 4.0f * par[pDrive] * (prof.kind == Kind::reed ? (0.4f + 0.6f * vel) : 1.0f);
        clip.reset();
        const float velGain = 1.0f - par[pVelSens] + par[pVelSens] * (0.1f + 0.9f * vel * vel);
        outGain = dbToGain (prof.gainDb + prof.keyDbPerOct * ((float) note - 60.0f) / 12.0f) * par[pLevel] * velGain / drive * (1.0f + 0.6f * (drive - 1.0f));
        tremPhase = 0.0f;
        released = false; active = true; silentBlocks = 0; age = 0;
    }

    void noteOff (float) override
    {
        if (released) return;
        released = true;
        // damper felt on the tine / reed (the toy piano has none: its modes keep their own decay up to m.release)
        thump += prof.kind == Kind::toy ? 0.0f : prof.thump * 0.35f;
    }

    void setPitch (float f) override
    {
        if (std::abs (f - hz) < 1.0e-4f * hz) return;
        hz = std::max (8.0f, f);
        wNorm = 1.0f / (twoPi * hz / (float) sr);
        setRot();
    }

    void render (float* left, float* right, int n) override
    {
        if (! active) return;
        juce::ScopedNoDenormals noDenormals;
        int done = 0;
        while (done < n)
        {
            const int len = std::min (32, n - done);
            // tremolo per block
            const float tr = par[pTrem];
            float gl = 1.0f, gr = 1.0f;
            if (tr > 0.0f)
            {
                const float s = std::sin (tremPhase);
                tremPhase += twoPi * par[pTremRate] * (float) len / (float) sr;
                if (tremPhase > twoPi) tremPhase -= twoPi;
                if (prof.kind == Kind::tine) { gl = 1.0f - tr * 0.5f * (1.0f + s); gr = 1.0f - tr * 0.5f * (1.0f - s); }
                else gl = gr = 1.0f - tr * 0.4f * (1.0f + s);
            }
            float peak = 0.0f;
            for (int i = 0; i < len; ++i)
            {
                attack += (1.0f - attack) * attackK;
                float x = 0.0f;
                for (int m = 0; m < numOsc; ++m)
                {
                    auto& o = osc[m];
                    const float mul = released ? o.mulRel : o.mulHeld;
                    const float re = (o.re * o.cr - o.im * o.ci) * mul;
                    const float im = (o.re * o.ci + o.im * o.cr) * mul;
                    o.re = re; o.im = im;
                    x += im;
                }
                x *= attack;
                float y;
                if (prof.kind == Kind::toy) y = x;
                else
                {
                    const float phi = pickup (x * excursion);
                    y = (phi - phiPrev) * wNorm;     // dPhi/dt normalised to the fundamental's angular step
                    phiPrev = phi;
                }
                const float nz = thumpLp.tick (rng.next()) * thump;
                thump *= thumpMul;
                y = y + nz;
                if (drive > 1.0f) y = clip.tick (y * drive);
                y = lp.tick (hp.tick (body.tick (y))) * outGain;
                peak = std::max (peak, std::abs (y));
                left[done + i] += y * gl; right[done + i] += y * gr;
            }
            done += len; age += len;
            silentBlocks = peak < (released ? 3.0e-5f : 5.0e-6f) && age > (int) (0.05 * sr) ? silentBlocks + 1 : 0;
            if (silentBlocks > 8) { kill(); return; }
        }
    }

    void kill() override { active = false; released = false; numOsc = 0; }
    bool isActive() const override { return active; }

private:
    const KeysProfile& prof;
    double sr = 48000.0;
    float par[numParams] {};
    Osc osc[5]; int numOsc = 0;
    Rng rng;
    int note = 60, silentBlocks = 0, age = 0;
    float vel = 1.0f, hz = 440.0f, attack = 0.0f, attackK = 0.1f, excursion = 0.5f, offset = 1.0f, phiPrev = 0.0f, wNorm = 1.0f;
    float thump = 0.0f, thumpMul = 0.99f, drive = 1.0f, outGain = 1.0f, tremPhase = 0.0f;
    OnePoleLP thumpLp; Biquad lp, hp, body; SoftClipAA clip;
    bool active = false, released = false;

    void setRot()
    {
        for (int m = 0; m < numOsc; ++m)
        {
            const double w = 2.0 * 3.141592653589793 * hz * osc[m].ratio / sr;
            osc[m].cr = (float) std::cos (w); osc[m].ci = (float) std::sin (w);
        }
    }

    // transducer: tine = magnetic (bell-shaped flux around the pole), reed = electrostatic (asymmetric capacitance)
    float pickup (float x) const
    {
        if (prof.kind == Kind::tine)
        {
            const float u = x + offset;
            return 1.0f / std::sqrt (1.0f + u * u);
        }
        const float u = clampf (x, -0.8f, 0.8f);
        return 1.0f / (1.0f - u * (0.8f + 0.2f * par[pVoicing]));
    }
};
} // namespace

juce::StringArray keysModelIds()
{
    juce::StringArray s;
    for (auto& p : profiles) s.add (p.id);
    return s;
}

ModelInfo keysModelInfo (const juce::String& id)
{
    const auto* p = findProfile (id);
    jassert (p != nullptr);
    if (p == nullptr) return {};
    return { p->id, p->name, paramsFor (*p), p->maxVoices, [p] { return std::make_unique<KeysVoice> (*p); } };
}
} // namespace vault::models
