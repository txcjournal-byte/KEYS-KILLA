// CHROME VAULT plucked / struck string models: guitars, electric basses and the struck-string "funk keys".
//
// One voice = 1-2 single-delay-loop digital waveguides (Karplus-Strong extended, Jaffe & Smith 1983; Karjalainen,
// Valimaki & Tolonen 1998) with
//   * a 3rd-order Lagrange fractional delay (exact tuning, smooth bends),
//   * a one-pole loss filter fitted per note to two decay targets: T60 of the fundamental and T60 at 2 kHz
//     (Valimaki et al. 1996), key-tracked per octave,
//   * a cascade of identical first-order allpasses for string stiffness: the coefficient is solved per note so that a
//     reference partial lands on f_k = k f0 sqrt((1 + B k^2) / (1 + B)) (Van Duyne & Smith 1994 / Rauhala & Valimaki 2006),
//   * commuted synthesis (Smith 1993; Karjalainen et al. 1993): the excitation is the instrument body's impulse response
//     (synthesised from measured body modes: air mode A0, top-plate modes T(1,1)..., banjo head modes) shaped by the
//     plucking finger / pick (2 one-pole low-passes, velocity -> brightness) and the pluck-position comb,
//   * a second loop: the second polarisation (slightly detuned, slower decay -> double decay + gentle beating) or,
//     for the 12-string, the course partner (octave string below G4, unison above),
//   * electric instruments: magnetic pickup position combs (neck / bridge blend), pickup resonance, an amp (soft clip with
//     anti-derivative anti-aliasing), tone stack and a speaker-cabinet response; basses: DI-style amp,
//   * articulations (m.artic): 0 pluck, 1 palm mute (extra damping, darker, +3 cents of string pressure), 2 harmonic.
//   * slap bass: fret-collision limiter inside the loop for the first 150 ms; fretless: "mwah" formant sweep + slide-in.
#include "BuiltinModels.h"
#include "ModelDsp.h"

namespace vault::models
{
namespace
{
using namespace dsp;

enum class Exc : uint8_t { pick, finger, thumb, tangent };
enum class Body : uint8_t { none, steel, nylon, archtop, banjo };
enum class Amp : uint8_t { acoustic, clean, crunch, jazz, bass, bassSlap, clav };

struct Mode { float hz, t60, db; };

// body modes (Hz, T60 s, level dB). Low modes from the guitar acoustics literature (Fletcher & Rossing, "The Physics of
// Musical Instruments" ch. 9-10: dreadnought A0 ~100 Hz, T(1,1)1 ~190 Hz, T(1,1)2 ~230 Hz, A1 ~380 Hz; classical A0 ~95 Hz,
// T(1,1) ~200 Hz; archtop f-hole air mode ~115 Hz), the denser upper modes and their damping are generic estimates.
// Banjo: the head is a light membrane, admittance 20-30 dB above a guitar top, many modes from ~300 Hz up, fast decay
// (Woodhouse, Politzer & Mansour 2021, Acta Acustica 5, 15/16).
// Peak levels are the listed dB; Q of the low modes ~10-15 (T60 = 6.9 Q / (pi f)). The coupled top / back mode is placed at
// ~255 Hz ("voicing" of the body): the C5 fundamental (261.6 Hz) gets body support, as on a warm-sounding guitar - the
// spectrum envelope measured at C5 (tools/vault/models_measure.py) keeps H1 the strongest partial.
const Mode steelModes[] = { { 98, 0.13f, 0 }, { 200, 0.10f, 1 }, { 255, 0.08f, 0 }, { 375, 0.06f, -3 }, { 440, 0.05f, -5 },
                            { 550, 0.045f, -6 }, { 660, 0.04f, -7 }, { 780, 0.035f, -7 }, { 950, 0.03f, -9 }, { 1150, 0.025f, -10 },
                            { 1400, 0.02f, -11 }, { 1800, 0.018f, -13 }, { 2300, 0.015f, -15 }, { 3000, 0.012f, -18 } };
const Mode nylonModes[] = { { 97, 0.16f, 0 }, { 200, 0.12f, 2 }, { 255, 0.09f, -2 }, { 390, 0.07f, -4 }, { 470, 0.06f, -5 },
                            { 560, 0.054f, -7 }, { 720, 0.042f, -8 }, { 900, 0.036f, -9 }, { 1150, 0.03f, -11 }, { 1500, 0.024f, -13 },
                            { 2000, 0.018f, -16 } };
const Mode archModes[] = { { 116, 0.12f, -2 }, { 215, 0.09f, 0 }, { 290, 0.07f, -2 }, { 420, 0.06f, -4 }, { 560, 0.048f, -5 },
                           { 700, 0.042f, -7 }, { 900, 0.036f, -8 }, { 1200, 0.03f, -10 }, { 1600, 0.024f, -12 } };
const Mode banjoModes[] = { { 165, 0.06f, -6 }, { 262, 0.07f, 0 }, { 420, 0.06f, -2 }, { 560, 0.054f, -2 }, { 700, 0.048f, -3 },
                            { 860, 0.042f, -3 }, { 1050, 0.036f, -4 }, { 1300, 0.03f, -5 }, { 1600, 0.027f, -7 }, { 2000, 0.024f, -9 },
                            { 2500, 0.021f, -12 }, { 3100, 0.018f, -15 } };

struct StringProfile
{
    const char* id; const char* name;
    int refNote;            // the note the reference values below are given for
    float t60Fund, t60Hi;   // T60 of the fundamental and at 2 kHz (or 2 f0 if higher) at refNote, s
    float t60Oct;           // T60 multiplier per octave up
    float B, bOct;          // inharmonicity coefficient at refNote, multiplier per octave up
    int apSections;         // dispersion allpass sections
    int loops; bool twelve; float loop2Cents, loop2Gain, loop2T60;
    Exc exc; float pickPos, pluckLo, pluckHi, tilt, click;
    Body body; float bodyMix;
    bool pickups; float pickNeck, pickBridge, pickupDef, pickupHz, pickupQ;
    Amp amp; float driveDef;
    float outLP, outHP, release, gainDb, keyDbPerOct;
    float slap, growl, slide, releaseCents;
    int maxVoices;
};

// gainDb / keyDbPerOct are calibrated with VaultModelLab -sweep: velocity 1.0 peaks about -9 dBFS at C5, even across keys
const StringProfile profiles[] = {
    // id                     name                      ref  T0    Thi   oct   B       bOct ap loops 12   l2c   l2g   l2t   exc          pos   pLo   pHi   tilt click body          mix   pu     neck  brdg  def   puHz  puQ  amp             drv   LP     HP    rel   gain  kdb  slap growl slide relc voices
    { "guitar.acoustic",      "Steel Acoustic Guitar",  60,  5.0f, 1.1f, 0.78f, 6e-5f,  2.2f, 2, 2, false, 0.7f, 0.35f, 1.8f, Exc::pick,   0.12f, 1400, 5000, 1.0f, 0.04f, Body::steel,   1.0f, false, 0,    0,    0,    0,    0,   Amp::acoustic,  0,    8000,  60,   0.25f, -0.2f, 4.0f,   0,   0,    0,    0,   12 },
    { "guitar.nylon",         "Nylon Classical Guitar", 60,  3.6f, 0.45f,0.78f, 2e-5f,  2.0f, 2, 2, false, 0.5f, 0.30f, 1.7f, Exc::finger, 0.17f, 700,  2600, 0.8f, 0.01f, Body::nylon,   1.0f, false, 0,    0,    0,    0,    0,   Amp::acoustic,  0,    6500,  60,   0.30f, 1.5f, 4.0f,   0,   0,    0,    0,   12 },
    { "guitar.banjo",         "Banjo",                  60,  1.6f, 0.40f,0.80f, 8e-5f,  2.2f, 2, 1, false, 0,    0,     1,    Exc::pick,   0.09f, 1800, 6000, 1.0f,0.05f, Body::banjo,   1.2f, false, 0,    0,    0,    0,    0,   Amp::acoustic,  0,    5000,  90,   0.15f, 1.9f, 3.0f,   0,   0,    0,    0,   12 },
    { "guitar.twelve",        "12-String Guitar",       60,  4.5f, 1.0f, 0.78f, 6e-5f,  2.2f, 2, 2, true,  3.0f, 0.25f, 1.0f, Exc::pick,   0.12f, 1400, 5000, 1.0f, 0.04f, Body::steel,   1.0f, false, 0,    0,    0,    0,    0,   Amp::acoustic,  0,    8000,  60,   0.25f, 1.7f, 3.5f,   0,   0,    0,    0,   10 },
    { "guitar.electric",      "Clean Electric Guitar",  60,  7.0f, 1.6f, 0.78f, 6e-5f,  2.2f, 2, 1, false, 0,    0,     1,    Exc::pick,   0.13f, 1400, 4500, 0.8f, 0.02f, Body::none,    0,    true,  0.25f, 0.09f,0.0f,3000, 1.1f,Amp::clean,     0.15f,6000,  70,   0.12f, 19.0f, 1.4f,   0,   0,    0,    0,   12 },
    { "guitar.crunch",        "Crunch Electric Guitar", 60,  7.0f, 1.6f, 0.78f, 6e-5f,  2.2f, 2, 1, false, 0,    0,     1,    Exc::pick,   0.13f, 1400, 4500, 0.8f, 0.02f, Body::none,    0,    true,  0.25f, 0.09f, 0.4f, 2600, 0.9f,Amp::crunch, 0.4f, 5000,  90,   0.12f, 10.4f, 1.5f,   0,   0,    0,    0,   12 },
    { "guitar.jazz",          "Jazz Archtop Guitar",    60,  3.5f, 0.35f,0.78f, 4e-5f,  2.0f, 2, 1, false, 0,    0,     1,    Exc::pick, 0.17f, 800,  2500, 0.6f, 0.01f, Body::archtop, 0.35f,true, 0.25f,0.10f, 0.0f, 2000, 0.8f,Amp::jazz,      0.1f, 3500,  70,   0.15f, 10.1f, 5.0f,   0,   0,    0,    0,   12 },
    { "bass.finger",          "Finger Bass",            40,  7.0f, 0.8f, 0.70f, 1.2e-4f,2.0f, 4, 1, false, 0.4f, 0.25f, 1.5f, Exc::finger, 0.22f, 500,  2200, 0.6f, 0.01f, Body::none,    0,    true,  0.23f,0.095f,0.5f, 2500, 0.8f,Amp::bass,      0.15f,4500,  35,   0.10f, 22.0f, 2.5f,   0,   0,    0,    0,   10 },
    { "bass.pick",            "Pick Bass",              40,  7.0f, 1.0f, 0.70f, 1.2e-4f,2.0f, 4, 1, false, 0.4f, 0.25f, 1.5f, Exc::pick, 0.22f, 1500, 5000, 0.45f, 0.05f, Body::none,    0,    true,  0.17f,0.095f, 0.0f, 3200, 1.0f,Amp::bass,      0.25f,5000,  35,   0.08f, 21.6f, 1.4f,   0,   0,    0,    0,   10 },
    { "bass.slap",            "Slap Bass",              40,  6.0f, 1.2f, 0.70f, 1.2e-4f,2.0f, 4, 1, false, 0,    0,     1,    Exc::thumb,  0.30f, 3000, 8000, 0.9f, 0.08f, Body::none,    0,    true,  0.23f,0.095f,0.5f, 3500, 0.9f,Amp::bassSlap,  0.2f, 5500,  35,   0.08f, 6.0f, 4.0f,   1,   0,    0,    0,   10 },
    { "bass.fretless",        "Fretless Bass",          40,  5.0f, 0.35f,0.70f, 9e-5f,  2.0f, 4, 1, false, 0.5f, 0.30f, 1.6f, Exc::finger, 0.20f, 500,  2000, 0.6f, 0.01f, Body::none,    0,    true,  0.23f,0.095f,0.7f, 2500, 0.9f,Amp::bass,      0.15f,4000,  35,   0.10f, 21.0f, 1.5f,   0,   1,    25,   0,   10 },
    { "ep.clav",              "Funk Keys",              60,  3.0f, 1.0f, 0.70f, 1e-4f,  2.0f, 2, 1, false, 0,    0,     1,    Exc::tangent, 0.22f, 3000, 6000, 0.4f, 0.0f,  Body::none,    0,    true, 0.2f, 0.08f, 0.2f, 3000, 1.0f,Amp::clav,      0.2f, 4500,  80,   0.06f, 18.6f, 1.0f,   0,   0,    0,    -8,  12 },
};

const StringProfile* findProfile (const juce::String& id)
{
    for (auto& p : profiles) if (id == p.id) return &p;
    return nullptr;
}

enum ParamIndex { pArtic, pTone, pDecay, pRelease, pPickPos, pBody, pPickup, pDrive, pStiff, pDetune, pLevel, pVelSens, numParams };

std::vector<ParamInfo> paramsFor (const StringProfile& p)
{
    return { { "m.artic",   0.0f, 2.0f,  0.0f,  "articulation: 0 pluck, 1 palm mute, 2 harmonic" },
             { "m.tone",    0.0f, 1.0f,  0.5f,  "brightness: pluck hardness and output tone (0.5 = as designed)" },
             { "m.decay",   0.2f, 4.0f,  1.0f,  "string decay time multiplier" },
             { "m.release", 0.02f, 3.0f, p.release, "T60 after note-off, s" },
             { "m.pickPos", 0.03f, 0.5f, p.pickPos, "pluck position, fraction of the string from the bridge" },
             { "m.body",    0.0f, 1.5f,  1.0f,  "body / resonance amount (1 = as designed)" },
             { "m.pickup",  0.0f, 1.0f,  p.pickupDef, "pickup: 0 neck .. 1 bridge (electric instruments)" },
             { "m.drive",   0.0f, 1.0f,  p.driveDef, "amp drive (electric instruments)" },
             { "m.stiff",   0.0f, 4.0f,  1.0f,  "string stiffness (inharmonicity) multiplier" },
             { "m.detune",  0.0f, 20.0f, p.loop2Cents, "second string / polarisation detune, cents" },
             { "m.level",   0.0f, 2.0f,  1.0f,  "output level multiplier" },
             { "m.velSens", 0.0f, 1.0f,  0.7f,  "velocity -> level (0 = fixed level, brightness still follows velocity)" } };
}

// one waveguide loop
struct Loop
{
    DelayLine dl;
    float lossB = 0.0f, lossA = 0.0f, lossZ = 0.0f;   // one-pole loss y = b x - a y1
    Allpass1 ap[4];
    float apC = 0.0f; int numAp = 0;
    Ramp delay;                                        // delay-line length (samples, fractional)
    float t60Scale = 1.0f, gain = 1.0f, ratio = 1.0f;  // decay multiplier, output gain, frequency ratio to the note
    int combP = 1, start = 0;                          // excitation pluck-position comb (samples), injection start offset
    float hz = 440.0f;

    void reset() { dl.clear(); lossZ = 0.0f; for (auto& a : ap) a.reset(); }
};

class StringVoice final : public ModelVoice
{
public:
    explicit StringVoice (const StringProfile& p) : prof (p) {}

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate > 0 ? sampleRate : 48000.0;
        const int maxDelay = (int) (sr / 15.0) + 64;
        for (auto& l : loops) l.dl.resize (maxDelay);
        excLen = (int) (0.085 * sr);
        excBuf.assign ((size_t) excLen + 8, 0.0f);
        bodyIR.assign ((size_t) excLen, 0.0f);
        buildBody();
        const auto defs = paramsFor (prof);
        for (int i = 0; i < numParams; ++i) par[i] = defs[(size_t) i].def;
        kill();
    }

    void setParams (const float* v, int n) override { for (int i = 0; i < numParams && i < n; ++i) par[i] = v[i]; }

    void noteOn (int midiNote, float velocity, float freqHz) override
    {
        note = midiNote;
        vel = clampf (velocity, 0.0f, 1.0f);
        artic = (int) std::lround (par[pArtic]);
        rng.seed ((uint32_t) (midiNote * 7919 + (int) (vel * 127.0f) * 131 + 17));
        baseHz = std::max (8.0f, freqHz);
        curHz = baseHz;
        released = false; active = true; age = 0; silentBlocks = 0;
        slideCents = prof.slide; relCents = 0.0f;

        const float oct = ((float) midiNote - (float) prof.refNote) / 12.0f;
        const float decayMul = par[pDecay] * (artic == 2 ? 1.4f : 1.0f);
        t0 = prof.t60Fund * std::pow (prof.t60Oct, oct) * decayMul;
        tHi = prof.t60Hi * std::pow (prof.t60Oct, oct * 0.5f) * decayMul;
        if (artic == 1) { t0 = std::min (t0, 0.45f); tHi = std::min (tHi, 0.06f); }   // palm mute: the palm damps every partial
        bCoef = prof.B * std::pow (prof.bOct, oct) * par[pStiff];

        // loops: 0 = main string; 1 = second polarisation or 12-string course partner
        numLoops = prof.loops;
        for (int k = 0; k < numLoops; ++k)
        {
            auto& L = loops[k];
            L.reset();
            L.ratio = 1.0f; L.gain = 1.0f; L.t60Scale = 1.0f; L.start = 0;
            if (k == 1)
            {
                L.gain = prof.loop2Gain; L.t60Scale = prof.loop2T60;
                if (prof.twelve && midiNote < 68) { L.ratio = 2.0f; L.start = (int) (0.006 * sr); }   // octave string, picked first
                else L.ratio = centsToRatio (par[pDetune]);
            }
            const float mute = artic == 1 ? 3.0f : 0.0f;   // pressing / palm: slightly sharp
            L.hz = baseHz * L.ratio * centsToRatio (mute + (k == 0 ? -slideCents : 0.0f));
            design (L, L.hz, t0 * L.t60Scale, tHi * L.t60Scale);
            if (k == 1) L.numAp = std::min (L.numAp, 2);   // the quieter second loop: lighter stiffness filter (tuning stays exact)
            L.delay.jump (loopDelay (L, L.hz));
            const float pos = artic == 2 ? 0.5f : par[pPickPos];
            L.combP = std::max (1, (int) std::lround (pos * (float) (sr / L.hz)));
        }
        buildExcitation();
        excPos = 0;

        // output chain
        setupOutput();
        float excPeak = 0.0f;
        for (int i = 0; i < excLen; ++i) excPeak = std::max (excPeak, std::abs (excBuf[(size_t) i]));
        slapC = prof.slap > 0 ? prof.slap * 0.6f * excPeak : 0.0f;
        growlT = 0.0f;
        fadeOut = 1.0f;
    }

    void noteOff (float) override
    {
        if (released) return;
        released = true;
        const float rel = std::max (0.02f, par[pRelease]);
        for (int k = 0; k < numLoops; ++k)
        {
            auto& L = loops[k];
            design (L, L.hz, std::min (rel, t0 * L.t60Scale), std::min (rel * 0.5f, tHi * L.t60Scale), false);
        }
        if (prof.releaseCents != 0.0f) relCents = prof.releaseCents;
        retune (32);
    }

    void setPitch (float freqHz) override
    {
        if (std::abs (freqHz - curHz) < 1.0e-4f * curHz) return;
        curHz = std::max (8.0f, freqHz);
        retune (128);
    }

    void render (float* left, float* right, int n) override
    {
        if (! active) return;
        juce::ScopedNoDenormals noDenormals;
        int done = 0;
        while (done < n)
        {
            const int len = std::min (32, n - done);
            controlBlock (len);
            float* l = left + done; float* r = right + done;
            for (int i = 0; i < len; ++i)
            {
                float out = 0.0f, out2 = 0.0f;
                for (int k = 0; k < numLoops; ++k)
                {
                    auto& L = loops[k];
                    float in = 0.0f;
                    const int ep = excPos - L.start;
                    if (ep >= 0 && ep < excLen) in = excBuf[(size_t) ep] - (ep >= L.combP ? excBuf[(size_t) (ep - L.combP)] : 0.0f);
                    float y = L.dl.read (L.delay.next());
                    L.lossZ = L.lossB * y - L.lossA * L.lossZ; y = L.lossZ;
                    for (int a = 0; a < L.numAp; ++a) y = L.ap[a].tick (y);
                    if (k == 0 && slapC > 0.0f)    // fret collision: the slapped string hits the frets (energy-losing limiter)
                    {
                        const float ay = std::abs (y);
                        if (ay > slapC) y = (y > 0 ? 1.0f : -1.0f) * (slapC + (ay - slapC) * 0.3f);
                    }
                    y += in * L.gain;
                    L.dl.write (y);
                    if (k == 0) out = y; else out2 = y;
                }
                ++excPos;
                float s;
                if (prof.pickups)
                {
                    // pickup = string displacement sensed at the pickup position: y[n] - y[n - P] comb, neck / bridge blend
                    const auto& L0 = loops[0];
                    // (fractional taps: a bend changes the tension, not where the pickup sits on the string)
                    s = out * (puMixN + puMixB) + out2 * 0.3f;
                    if (puMixN > 0.0f) s -= L0.dl.readLinear (puNeck + 1.0f) * puMixN;
                    if (puMixB > 0.0f) s -= L0.dl.readLinear (puBridge + 1.0f) * puMixB;
                }
                else s = out + out2;
                s = chain (s) * fadeOut;
                blockPeak = std::max (blockPeak, std::abs (s));
                l[i] += s * panL; r[i] += s * panR;
                if (prof.twelve) { const float w = out2 * outGain * 0.12f * fadeOut; l[i] -= w; r[i] += w; }   // course partner slightly wide
            }
            silence.peak = std::max (blockPeak, silence.peak * 0.5f);
            blockPeak = 0.0f;
            done += len;
            age += len;
            if (excPos > excLen + loops[0].start)
            {
                const float floor = released ? 2.0e-5f : 6.0e-6f;
                silentBlocks = silence.peak < floor ? silentBlocks + 1 : 0;
                if (silentBlocks > 8) { kill(); return; }
            }
        }
    }

    void kill() override
    {
        active = false; released = false;
        for (auto& L : loops) L.reset();
        for (auto& b : bq) b.reset();
        clip.reset(); dc.reset(); silence.peak = 0.0f;
    }
    bool isActive() const override { return active; }

private:
    const StringProfile& prof;
    double sr = 48000.0;
    float par[numParams] {};
    Loop loops[2];
    int numLoops = 1;
    std::vector<float> excBuf, bodyIR;
    int excLen = 4096, excPos = 0;
    Rng rng;
    int note = 60, artic = 0, age = 0, silentBlocks = 0;
    float vel = 1.0f, baseHz = 440.0f, curHz = 440.0f, t0 = 4.0f, tHi = 1.0f, bCoef = 0.0f;
    float slideCents = 0.0f, relCents = 0.0f, slapC = 0.0f, growlT = 0.0f, fadeOut = 1.0f;
    bool active = false, released = false;
    // output chain
    Biquad bq[9]; int numBq = 0; int growlIdx = -1;
    SoftClipAA clip; DcBlock dc;
    float preGain = 1.0f, postGain = 1.0f, bias = 0.0f, biasOut = 0.0f, outGain = 1.0f, panL = 1.0f, panR = 1.0f;
    bool useClip = false;
    float puNeck = 1.0f, puBridge = 1.0f, puMixN = 1.0f, puMixB = 0.0f;
    Silence silence; float blockPeak = 0.0f;

    //------------------------------------------------------------------------------------------------------------
    // loss filter H(z) = g (1 + a) / (1 + a z^-1) fitted to the per-period loop gains at f0 and at the "high" frequency
    void design (Loop& L, float hz, float t0s, float thiS, bool withDispersion = true)
    {
        const double w0 = 2.0 * 3.141592653589793 * hz / sr;
        const double fh = std::min (sr * 0.4, std::max (2000.0, 2.0 * hz));
        const double w1 = 2.0 * 3.141592653589793 * fh / sr;
        const double G0 = std::pow (10.0, -3.0 / (std::max (0.01f, t0s) * hz));
        const double G1 = std::pow (10.0, -3.0 / (std::max (0.005f, std::min (thiS, t0s)) * hz));
        const double R = std::min (1.0, G1 / G0);
        auto ratio = [&] (double a) { return std::sqrt ((1 + 2 * a * std::cos (w0) + a * a) / (1 + 2 * a * std::cos (w1) + a * a)); };
        // a strongly low-passing one-pole is itself dispersive (its phase delay falls with frequency -> sharp partials):
        // limit it more at high notes, where the partials are few samples apart
        const double aMin = -0.9 + 0.5 * juce::jlimit (0.0, 1.0, (w0 - 0.04) / 0.11);
        double lo = aMin, hi = 0.0;
        if (ratio (lo) > R) hi = lo;
        else for (int it = 0; it < 40; ++it) { const double m = 0.5 * (lo + hi); if (ratio (m) > R) hi = m; else lo = m; }
        const double a = hi;
        double g = G0 * std::sqrt (1 + 2 * a * std::cos (w0) + a * a) / (1 + a);
        g = std::min (g, 0.99995);
        L.lossA = (float) a; L.lossB = (float) (g * (1 + a));
        if (! withDispersion) return;
        // stiffness: allpass coefficient so that partial kRef sits on its inharmonic frequency
        const double N = sr / hz;
        const int apCount = prof.apSections;
        L.apC = 0.0f; L.numAp = 0;
        if (bCoef > 1.0e-7f && apCount > 0)
        {
            const int kRef = juce::jlimit (2, 30, (int) std::lround (2500.0 / hz));
            const double fk = kRef * hz * std::sqrt ((1.0 + bCoef * kRef * kRef) / (1.0 + bCoef));
            const double wk = 2.0 * 3.141592653589793 * fk / sr;
            if (wk < 3.0)
            {
                auto lossPd = [&] (double w) { return std::atan2 (-a * std::sin (w), 1 + a * std::cos (w)) / w; };
                // loop phase at wk with the delay line set so that the fundamental is exactly in tune
                auto err = [&] (double c)
                {
                    const double ddl = N - apCount * Allpass1::phaseDelay (c, w0) - lossPd (w0);
                    return wk * (ddl + apCount * Allpass1::phaseDelay (c, wk) + lossPd (wk)) - 2.0 * 3.141592653589793 * kRef;
                };
                auto ddlOf = [&] (double c) { return N - apCount * Allpass1::phaseDelay (c, w0) - lossPd (w0); };
                double cLo = -0.9, cHi = 0.0;
                while (cLo < -0.05 && ddlOf (cLo) < 4.0) cLo *= 0.8;           // keep a usable delay line at high notes
                if (err (cLo) >= 0.0) cHi = cLo;
                else for (int it = 0; it < 40; ++it) { const double m = 0.5 * (cLo + cHi); if (err (m) > 0.0) cHi = m; else cLo = m; }
                L.apC = (float) cHi;
                L.numAp = std::abs (cHi) > 1.0e-4 ? apCount : 0;
            }
        }
        for (int i = 0; i < 4; ++i) L.ap[i].c = L.apC;
    }

    // delay-line length that puts the fundamental exactly on hz (phase delays of the loss filter and allpasses removed)
    float loopDelay (const Loop& L, float hz) const
    {
        const double w0 = 2.0 * 3.141592653589793 * hz / sr;
        const double a = L.lossA;
        const double lossPd = std::atan2 (-a * std::sin (w0), 1 + a * std::cos (w0)) / w0;
        double d = sr / hz - lossPd - (L.numAp > 0 ? L.numAp * Allpass1::phaseDelay (L.apC, w0) : 0.0);
        const double maxD = (double) L.dl.mask - 8.0;
        return (float) juce::jlimit (2.0, maxD, d);
    }

    void retune (int rampSamples)
    {
        for (int k = 0; k < numLoops; ++k)
        {
            auto& L = loops[k];
            const float mute = artic == 1 ? 3.0f : 0.0f;
            L.hz = curHz * L.ratio * centsToRatio (mute + relCents + (k == 0 ? -slideCents : 0.0f));
            L.delay.set (loopDelay (L, L.hz), rampSamples);
        }
        if (prof.pickups)
        {
            const float N = (float) (sr / loops[0].hz);
            puNeck = std::max (1.0f, prof.pickNeck * N);
            puBridge = std::max (1.0f, prof.pickBridge * N);
        }
    }

    //------------------------------------------------------------------------------------------------------------
    void buildBody()
    {
        std::fill (bodyIR.begin(), bodyIR.end(), 0.0f);
        const Mode* modes = nullptr; int nm = 0;
        switch (prof.body)
        {
            case Body::steel:   modes = steelModes; nm = (int) std::size (steelModes); break;
            case Body::nylon:   modes = nylonModes; nm = (int) std::size (nylonModes); break;
            case Body::archtop: modes = archModes;  nm = (int) std::size (archModes); break;
            case Body::banjo:   modes = banjoModes; nm = (int) std::size (banjoModes); break;
            case Body::none: break;
        }
        bodyIR[0] = 1.0f;   // the direct (bridge) path
        if (nm == 0) return;
        Rng nr; nr.seed (4242);
        OnePoleLP nlp; nlp.setHz (prof.body == Body::banjo ? 3500.0f : 2500.0f, sr);
        const int len = (int) bodyIR.size();
        double peak = 0.0;
        std::vector<double> tmp ((size_t) len, 0.0);
        for (int m = 0; m < nm; ++m)
        {
            const double w = 2.0 * 3.141592653589793 * modes[m].hz / sr;
            const double dec = std::pow (0.001, 1.0 / (modes[m].t60 * sr));
            // resonance peak height ~ amplitude x T60: scale so the listed dB is the mode's peak in the body response
            const double amp = std::pow (10.0, modes[m].db / 20.0) / modes[m].t60 * 0.05;
            double e = amp;
            for (int i = 0; i < len; ++i) { tmp[(size_t) i] += e * std::sin (w * (i + 1)); e *= dec; }
        }
        // dense upper modes as a short decaying noise (statistical high-frequency body response)
        double ne = 0.08;
        const double nd = std::pow (0.001, 1.0 / (0.025 * sr));
        for (int i = 0; i < len; ++i) { tmp[(size_t) i] += nlp.tick (nr.next()) * ne; ne *= nd; }
        for (int i = 0; i < len; ++i) peak = std::max (peak, std::abs (tmp[(size_t) i]));
        const double norm = peak > 0 ? 0.6 / peak : 0.0;
        for (int i = 0; i < len; ++i)
        {
            const double fade = i > len * 2 / 3 ? 0.5 + 0.5 * std::cos (3.141592653589793 * (i - len * 2 / 3) / (len / 3.0)) : 1.0;
            bodyIR[(size_t) i] += (float) (tmp[(size_t) i] * norm * fade * prof.bodyMix);
        }
    }

    // commuted excitation: body IR -> pluck (finger / pick hardness: 2 one-poles) -> spectral tilt of a pluck (1/k);
    // the pluck-position comb is applied per loop at injection
    void buildExcitation()
    {
        const float tone = std::exp2 ((par[pTone] - 0.5f) * 3.0f);
        float fc = (prof.pluckLo + (prof.pluckHi - prof.pluckLo) * vel * vel) * tone;
        if (artic == 1) fc *= 0.45f;
        if (artic == 2) fc = std::min (fc, baseHz * 1.6f);
        OnePoleLP p1, p2, tilt;
        p1.setHz (fc, sr); p2.setHz (fc * 1.3f, sr);
        tilt.setHz (std::max (40.0f, baseHz * prof.tilt), sr);
        const float bodyAmt = par[pBody];
        const int clickLen = (int) (0.0015 * sr);
        const float velGain = 1.0f - par[pVelSens] + par[pVelSens] * (0.12f + 0.88f * vel * vel);
        // a loop of N samples turns an impulse into a pulse train whose partials are ~2/N high: scale by N so the
        // fundamental's level does not depend on the note (key tracking on top: keyDbPerOct)
        float amp = velGain * (prof.exc == Exc::thumb ? 1.6f : 1.0f) * (261.63f / baseHz);
        if (artic == 2) amp *= prof.pickups ? 1.4f : 0.75f;   // level of a harmonic next to the open note
        for (int i = 0; i < excLen; ++i)
        {
            float x = i == 0 ? 1.0f : 0.0f;
            if (prof.body != Body::none) x = (i == 0 ? 1.0f - bodyAmt : 0.0f) + bodyIR[(size_t) i] * bodyAmt;
            // the direct (impulse) part: a loop of N samples turns it into partials ~2/N high and N grows with the sample
            // rate - scale it so the level does not depend on the sample rate (the modal body part is already rate-free)
            if (i == 0) x += (float) (sr / 48000.0) - 1.0f;
            if (i < clickLen) x += rng.next() * prof.click * (1.0f - (float) i / (float) clickLen) * vel;
            if (prof.exc == Exc::thumb && i < (int) (0.004 * sr))   // thumb flesh thud (not scaled with the loop length)
                x += 0.25f * (baseHz / 261.63f) * std::sin (pi * (float) i / (float) (0.004 * sr));
            x = p2.tick (p1.tick (x));
            x = tilt.tick (x) * amp;
            excBuf[(size_t) i] = x;
        }
        // remove the excitation's mean so no DC enters the loop (the comb removes the rest)
        float m = 0.0f; for (int i = 0; i < excLen; ++i) m += excBuf[(size_t) i];
        m /= (float) excLen;
        for (int i = 0; i < excLen; ++i)
        {
            const float w = i < excLen / 2 ? 1.0f : 0.5f + 0.5f * std::cos (pi * (float) (i - excLen / 2) / (float) (excLen / 2));
            excBuf[(size_t) i] = (excBuf[(size_t) i] - m * w) * w;
        }
    }

    void setupOutput()
    {
        numBq = 0; growlIdx = -1; useClip = false; bias = 0; biasOut = 0; preGain = 1; postGain = 1;
        const float tone = std::exp2 ((par[pTone] - 0.5f) * 1.5f);
        const float drive = par[pDrive];
        auto add = [&] () -> Biquad& { auto& b = bq[numBq++]; b.reset(); return b; };
        if (prof.pickups)
        {
            const double N = sr / baseHz;
            puNeck = std::max (1.0f, (float) (prof.pickNeck * N));
            puBridge = std::max (1.0f, (float) (prof.pickBridge * N));
            const float m = par[pPickup];
            puMixN = 1.0f - 0.7f * m;   // the bridge setting keeps some neck pickup (fuller, comfortable at C5)
            puMixB = m;
            add().lowpass (prof.pickupHz * (0.85 + 0.3 * m), prof.pickupQ, sr);   // coil inductance + cable capacitance resonance
        }
        switch (prof.amp)
        {
            case Amp::acoustic:
                add().highpass (prof.outHP, 0.7, sr);
                add().lowpass (prof.outLP * tone, 0.6, sr);
                add().peak (3200, 0.9, -3.0, sr);         // tame the 2-5 kHz presence region
                break;
            case Amp::clean:
                useClip = true; preGain = 0.8f + 2.0f * drive; postGain = 1.0f / (0.7f + 0.5f * drive);
                add().peak (650, 0.8, -2.5, sr);           // tone stack mid scoop
                add().highpass (prof.outHP, 0.7, sr);      // cabinet: open-back, low resonance
                add().peak (120, 1.0, 2.0, sr);
                add().lowpass (prof.outLP * tone, 0.7, sr);
                add().lowpass (prof.outLP * 1.3 * tone, 0.9, sr);
                break;
            case Amp::crunch:
                useClip = true; preGain = 2.0f + 14.0f * drive * drive; postGain = 0.9f / (1.0f + 0.6f * drive); bias = 0.06f;
                add().highpass (110, 0.7, sr);             // tight low end into the drive
                add().peak (700, 0.7, 1.5, sr);
                add().peak (240, 0.7, 3.5, sr);            // speaker / cabinet low-mid body after the drive
                add().highpass (prof.outHP, 0.7, sr);      // closed-back cabinet
                add().lowpass (prof.outLP * tone, 0.75, sr);
                add().lowpass (prof.outLP * 1.15 * tone, 0.6, sr);
                add().peak (2800, 1.2, -4.0, sr);
                break;
            case Amp::jazz:
                useClip = true; preGain = 0.8f + 1.5f * drive; postGain = 1.0f;
                add().lowpass (prof.outLP * tone, 0.6, sr);   // tone control rolled back
                add().highpass (prof.outHP, 0.7, sr);
                add().peak (180, 0.9, 2.0, sr);
                break;
            case Amp::bass:
            case Amp::bassSlap:
                useClip = true; preGain = 0.8f + 2.0f * drive; postGain = 1.0f;
                add().highpass (prof.outHP, 0.7, sr);
                if (prof.amp == Amp::bassSlap) { add().peak (500, 0.8, -5.0, sr); add().lowShelf (90, 3.0, sr); }
                add().lowpass (prof.outLP * tone, 0.7, sr);
                if (prof.growl > 0) { growlIdx = numBq; add().peak (1200, 2.0, 0.0, sr); }
                break;
            case Amp::clav:
                useClip = true; preGain = 1.0f + 3.0f * drive; postGain = 1.0f;
                add().highpass (prof.outHP, 0.7, sr);
                add().lowpass (prof.outLP * tone, 0.7, sr);   // "medium" filter switch
                add().peak (1100, 1.0, 2.0, sr);
                break;
        }
        dc.setHz (20.0f, sr); dc.reset(); clip.reset();
        // level: calibrated gain + key tracking, the user level
        const float oct = ((float) note - 60.0f) / 12.0f;
        outGain = dbToGain (prof.gainDb + prof.keyDbPerOct * oct) * par[pLevel];
        panL = panR = 1.0f;
    }

    float chain (float s)
    {
        if (useClip)
        {
            float x = s * preGain + bias;
            x = clip.tick (x) - biasOut;
            s = x * postGain;
        }
        for (int b = 0; b < numBq; ++b) s = bq[b].tick (s);
        return dc.tick (s) * outGain;
    }

    void controlBlock (int len)
    {
        if (useClip && bias != 0.0f && biasOut == 0.0f) biasOut = bias / std::sqrt (1.0f + bias * bias);
        const float t = (float) age / (float) sr;
        if (slapC > 0.0f) { slapC *= std::exp ((float) len / (0.04f * (float) sr)); if (t > 0.15f) slapC = 0.0f; }
        if (slideCents > 0.0f)
        {
            slideCents *= std::exp (-(float) len / (0.025f * (float) sr));
            if (slideCents < 0.05f) slideCents = 0.0f;
            retune (len);
        }
        if (growlIdx >= 0 && t < 0.9f && (age % 128) < len)   // every 128 samples, settled after ~0.9 s
        {
            // fretless "mwah": a soft formant sliding from ~1.3 kHz down to ~500 Hz over the first 400 ms
            const float g = std::exp (-t / 0.18f);
            bq[growlIdx].peak (500.0 + 800.0 * g, 1.6, 7.0 * (0.4f + 0.6f * g) * prof.growl, sr);
        }
    }
};
} // namespace

juce::StringArray stringModelIds()
{
    juce::StringArray s;
    for (auto& p : profiles) s.add (p.id);
    return s;
}

ModelInfo stringModelInfo (const juce::String& id)
{
    const auto* p = findProfile (id);
    jassert (p != nullptr);
    if (p == nullptr) return {};
    return { p->id, p->name, paramsFor (*p), p->maxVoices, [p] { return std::make_unique<StringVoice> (*p); } };
}
} // namespace vault::models
