// CHROME VAULT voice models: solo female / male singer, choir ensemble, vocal pad.
//
// Source-filter synthesis (Fant 1960; Klatt 1980):
//   * glottal source: the Liljencrants-Fant (LF) flow-derivative pulse (Fant, Liljencrants & Lin 1985) shaped by the single
//     voice-quality parameter Rd (Fant 1995: Rap = (-1 + 4.8 Rd) / 100, Rkp = (22.4 + 11.8 Rd) / 100,
//     Rgp = Rkp / (4 (0.11 Rd / (0.5 + 1.2 Rkp) - Rap)); Rd ~0.5 tense / pressed, ~1 modal, ~2.5 breathy).
//     Pulses are precomputed once per Rd as band-limited wavetables (one table per octave: no aliasing at any pitch).
//   * aspiration: high-passed noise gated by the open phase of the glottal cycle (pulsed breath noise), louder for
//     breathy voice, the only source for whisper and for the "h" of the "ha" articulation.
//   * vocal tract: 5 two-pole resonators in cascade (Klatt), unity gain at DC, frequencies / bandwidths from the singing
//     formant tables of the Csound manual appendix ("Formant values": soprano / alto / tenor / bass, vowels a o u i).
//     Choral and pop voices do not use the operatic "singer's formant" (Sundberg 1987): F3-F5 bandwidths are widened
//     and a gentle 3 kHz dip keeps the 2-5 kHz region tame. Soprano formant tuning: F1 >= 1.1 f0.
//   * life: vibrato (onset after ~250 ms, 5-6 Hz), jitter (random f0 wander ~0.2 %), shimmer (~0.3 dB), slow drift.
//   * choir: 8-24 singers, each with its own pulse phase, pitch scatter (Gaussian, default SD 12 cents - inside the
//     14 cents listeners accept in unison choir sound, Ternstrom 1993), vibrato rate / depth / phase, jitter and onset;
//     singers are grouped by voice type (soprano / alto / tenor / bass) and each group has its own formant filter with
//     a few % formant scatter (no two singers have the same vocal tract) and its own place in the stereo field.
#include "BuiltinModels.h"
#include "ModelDsp.h"
#include <juce_dsp/juce_dsp.h>

namespace vault::models
{
namespace
{
using namespace dsp;

//==============================================================================
// LF pulse wavetables
struct Glottal
{
    static constexpr int N = 2048, numLevels = 10, numRd = 6;
    static constexpr float rds[numRd] = { 0.5f, 0.9f, 1.3f, 1.8f, 2.3f, 2.8f };
    std::vector<float> tab;
    float te[numRd] {};
    float harm[numRd][65] {};   // amplitude of harmonics 1..64 of each pulse (time-domain units of the tables)

    static const Glottal& get() { static const Glottal g; return g; }
    const float* table (int r, int level) const { return tab.data() + ((size_t) r * numLevels + (size_t) level) * (N + 1); }
    static int levelFor (double f0, double sr)
    {
        const double hmax = 0.42 * sr / std::max (1.0, f0);
        int l = 0;
        while (l < numLevels - 1 && (double) ((N / 2 - 1) >> l) > hmax) ++l;
        return l;
    }

    // one period (T0 = 1) of the LF flow derivative, Ee = 1
    static void lfPeriod (double rd, std::vector<double>& out, double& teOut)
    {
        const double ra = (-1.0 + 4.8 * rd) / 100.0, rk = (22.4 + 11.8 * rd) / 100.0;
        const double rg = rk / (4.0 * (0.11 * rd / (0.5 + 1.2 * rk) - ra));
        const double tp = 1.0 / (2.0 * rg), te = tp * (1.0 + rk), ta = ra, tc = 1.0;
        teOut = te;
        // return phase constant: eps * ta = 1 - exp(-eps (tc - te))
        double eps = 1.0 / ta;
        for (int i = 0; i < 100; ++i) eps = (1.0 - std::exp (-eps * (tc - te))) / ta;
        const double wg = 3.141592653589793 / tp;
        const double aRet = -(1.0 / (eps * ta)) * ((1.0 - std::exp (-eps * (tc - te))) / eps - (tc - te) * std::exp (-eps * (tc - te)));
        auto area = [&] (double al)
        {
            const double e0 = -1.0 / (std::exp (al * te) * std::sin (wg * te));
            const double I = (std::exp (al * te) * (al * std::sin (wg * te) - wg * std::cos (wg * te)) + wg) / (al * al + wg * wg);
            return e0 * I + aRet;
        };
        // net flow over the period must be zero: solve alpha
        double lo = -50.0, hi = 300.0;
        const bool incr = area (hi) > area (lo);
        for (int i = 0; i < 200; ++i)
        {
            const double m = 0.5 * (lo + hi);
            if ((area (m) > 0.0) == incr) hi = m; else lo = m;
        }
        const double al = 0.5 * (lo + hi);
        const double e0 = -1.0 / (std::exp (al * te) * std::sin (wg * te));
        const int n = (int) out.size();
        for (int i = 0; i < n; ++i)
        {
            const double t = (double) i / n;
            out[(size_t) i] = t <= te ? e0 * std::exp (al * t) * std::sin (wg * t)
                                      : -(1.0 / (eps * ta)) * (std::exp (-eps * (t - te)) - std::exp (-eps * (tc - te)));
        }
    }

    Glottal()
    {
        tab.assign ((size_t) numRd * numLevels * (N + 1), 0.0f);
        juce::dsp::FFT fft (11);   // 2048
        std::vector<double> raw ((size_t) N);
        std::vector<float> spec ((size_t) N * 2), work ((size_t) N * 2);
        for (int r = 0; r < numRd; ++r)
        {
            double teR = 0.5;
            lfPeriod (rds[r], raw, teR);
            te[r] = (float) teR;
            double mean = 0; for (auto v : raw) mean += v; mean /= N;
            std::fill (spec.begin(), spec.end(), 0.0f);
            for (int i = 0; i < N; ++i) spec[(size_t) i] = (float) (raw[(size_t) i] - mean);
            fft.performRealOnlyForwardTransform (spec.data());
            for (int k = 1; k <= 64; ++k)
                harm[r][k] = 2.0f * std::sqrt (spec[(size_t) (2 * k)] * spec[(size_t) (2 * k)] + spec[(size_t) (2 * k + 1)] * spec[(size_t) (2 * k + 1)]) / (float) N;
            double rawRms = 0; for (auto v : raw) rawRms += (v - mean) * (v - mean);
            rawRms = std::sqrt (rawRms / N);
            float scale = 1.0f;
            for (int l = 0; l < numLevels; ++l)
            {
                const int hmax = (N / 2 - 1) >> l;
                work = spec;
                work[0] = work[1] = 0.0f;   // DC
                for (int k = hmax + 1; k <= N / 2; ++k) { work[(size_t) (2 * k)] = 0.0f; work[(size_t) (2 * k + 1)] = 0.0f; }
                fft.performRealOnlyInverseTransform (work.data());
                if (l == 0)
                {
                    double rms = 0; for (int i = 0; i < N; ++i) rms += (double) work[(size_t) i] * work[(size_t) i];
                    rms = std::sqrt (rms / N);
                    scale = rms > 0 ? (float) (rawRms / rms) : 1.0f;
                }
                float* t = tab.data() + ((size_t) r * numLevels + (size_t) l) * (N + 1);
                for (int i = 0; i < N; ++i) t[i] = work[(size_t) i] * scale;
                t[N] = t[0];
            }
        }
    }
};

inline float lookup (const float* t, float phase)
{
    const float idx = phase * (float) Glottal::N;
    const int i = (int) idx;
    const float f = idx - (float) i;
    return t[i] + f * (t[i + 1] - t[i]);
}

//==============================================================================
// formant tables (Csound manual, appendix "Formant values"); order: ah (a), oh (o), oo (u), ee (i)
struct FormantSet { float f[5], bw[5]; };
const FormantSet soprano[4] = { { { 800, 1150, 2900, 3900, 4950 }, { 80, 90, 120, 130, 140 } },
                                { { 450, 800, 2830, 3800, 4950 },  { 70, 80, 100, 130, 135 } },
                                { { 325, 700, 2700, 3800, 4950 },  { 50, 60, 170, 180, 200 } },
                                { { 270, 2140, 2950, 3900, 4950 }, { 60, 90, 100, 120, 120 } } };
const FormantSet alto[4] =    { { { 800, 1150, 2800, 3500, 4950 }, { 80, 90, 120, 130, 140 } },
                                { { 450, 800, 2830, 3500, 4950 },  { 70, 80, 100, 130, 135 } },
                                { { 325, 700, 2530, 3500, 4950 },  { 50, 60, 170, 180, 200 } },
                                { { 350, 1700, 2700, 3700, 4950 }, { 50, 100, 120, 150, 200 } } };
const FormantSet tenor[4] =   { { { 650, 1080, 2650, 2900, 3250 }, { 80, 90, 120, 130, 140 } },
                                { { 400, 800, 2600, 2800, 3000 },  { 40, 80, 100, 120, 120 } },
                                { { 350, 600, 2700, 2900, 3300 },  { 40, 60, 100, 120, 120 } },
                                { { 290, 1870, 2800, 3250, 3540 }, { 40, 90, 100, 120, 120 } } };
const FormantSet bass[4] =    { { { 600, 1040, 2250, 2450, 2750 }, { 60, 70, 110, 120, 130 } },
                                { { 400, 750, 2400, 2600, 2900 },  { 40, 80, 100, 120, 120 } },
                                { { 350, 600, 2400, 2675, 2950 },  { 40, 80, 100, 120, 120 } },
                                { { 250, 1750, 2600, 3050, 3340 }, { 60, 90, 100, 120, 120 } } };
enum VoiceType { vtSoprano, vtAlto, vtTenor, vtBass };
const FormantSet* tableFor (int vt) { return vt == vtSoprano ? soprano : vt == vtAlto ? alto : vt == vtTenor ? tenor : bass; }

// vowel position 0..3 (continuous) -> formants; upper formants widened ("soft", non-operatic)
FormantSet vowelAt (int vt, float v, float shift, float soft)
{
    const auto* t = tableFor (vt);
    v = clampf (v, 0.0f, 3.0f);
    const int a = std::min (2, (int) v);
    const float f = v - (float) a;
    FormantSet o {};
    for (int k = 0; k < 5; ++k)
    {
        o.f[k] = (t[a].f[k] + f * (t[a + 1].f[k] - t[a].f[k])) * shift;
        o.bw[k] = (t[a].bw[k] + f * (t[a + 1].bw[k] - t[a].bw[k])) * (k >= 2 ? soft : 1.0f + 0.3f * (soft - 1.0f));
    }
    // keep formants ordered / apart after shifts
    for (int k = 1; k < 5; ++k) o.f[k] = std::max (o.f[k], o.f[k - 1] + 150.0f);
    return o;
}

// very high notes: the formants spread out and broaden (no sharp resonance survives far above a voice's range), so no
// vowel / shift / pitch combination can build a runaway resonance gain
void highVoice (FormantSet& fs, float f0)
{
    for (int k = 0; k < 5; ++k) fs.bw[k] = std::max (fs.bw[k], 0.15f * f0);
    for (int k = 1; k < 5; ++k) fs.f[k] = std::max (fs.f[k], fs.f[k - 1] + std::max (200.0f, 0.5f * (fs.bw[k] + fs.bw[k - 1])));
}

// output guard: transparent below -6 dBFS, a soft knee to at most -0.9 dBFS above (extreme settings only)
inline float safety (float y)
{
    const float a = std::abs (y);
    if (a <= 0.5f) return y;
    const float s = 0.5f + 0.4f * std::tanh ((a - 0.5f) / 0.4f);
    return y < 0.0f ? -s : s;
}

struct Tract
{
    Formant f[5];
    void set (const FormantSet& s, double sr)
    {
        for (int k = 0; k < 5; ++k)
        {
            const float hz = std::min (s.f[k], (float) (sr * 0.45));
            f[k].set (hz, s.bw[k], sr);
        }
    }
    float tick (float x) { for (auto& r : f) x = r.tick (x); return x; }
    // F1..F4 only (ensembles: F5 ~5 kHz sits above the section's post low-pass anyway)
    float tick4 (float x) { for (int k = 0; k < 4; ++k) x = f[k].tick (x); return x; }
    void reset() { for (auto& r : f) r.reset(); }
};

// |cascade response| at hz for a formant set (the same resonators as Tract, evaluated analytically)
float tractMag (const FormantSet& fs, float hz, double sr)
{
    const double w = 2.0 * 3.141592653589793 * hz / sr;
    const double cw = std::cos (w), sw = std::sin (w), c2w = std::cos (2 * w), s2w = std::sin (2 * w);
    double m = 1.0;
    for (int k = 0; k < 5; ++k)
    {
        const double f = std::min ((double) fs.f[k], sr * 0.45);
        const double r = std::exp (-3.141592653589793 * fs.bw[k] / sr);
        const double c1 = 2.0 * r * std::cos (2.0 * 3.141592653589793 * f / sr), c2 = -r * r, b0 = 1.0 - c1 - c2;
        const double re = 1.0 - c1 * cw - c2 * c2w, im = c1 * sw + c2 * s2w;
        m *= b0 / std::max (1e-12, std::sqrt (re * re + im * im));
    }
    return (float) m;
}

// C5 comfort rule for voices: the sung fundamental must stay the loudest partial (within 2 dB). Where a formant would lift
// a harmonic above it (an "ah" at C5: F1 ~800 Hz sits on the 3rd harmonic), the F1 / F2 bandwidths are widened (a softer,
// breathier tract - what choral and pop singers do at that pitch) until the rule holds. Below ~200 Hz the margin grows
// (low voices naturally have formant harmonics above H1 and that is not harsh). Also returns 1 / RMS of the voiced
// harmonics (below 5 kHz) so every vowel / pitch plays at the same level.
struct Shape { float bw1 = 1.0f, bw2 = 1.0f, norm = 1.0f; };
Shape comfortShape (FormantSet fs, const float* gA, const float* gB, float mix, float f0, double sr, float baseMarginDb = 2.0f)
{
    Shape sh;
    const int K = juce::jlimit (1, 64, (int) (5000.0f / std::max (20.0f, f0)));
    const float marginDb = baseMarginDb + std::max (0.0f, (200.0f - f0) / 10.0f);
    const bool shapeIt = f0 >= 150.0f;   // low voices: formant harmonics above H1 are natural, not harsh - keep the tract
    const float margin = dbToGain (marginDb);
    for (int it = 0; it < 10; ++it)
    {
        float h1 = 0.0f, hmax = 0.0f; double sum = 0.0;
        for (int k = 1; k <= K; ++k)
        {
            const float g = gA[k] * (1.0f - mix) + gB[k] * mix;
            const float m = g * tractMag (fs, (float) k * f0, sr);
            sum += (double) m * m;
            if (k == 1) h1 = m; else hmax = std::max (hmax, m);
        }
        sh.norm = sum > 0 ? (float) (1.0 / std::sqrt (sum)) : 1.0f;
        if (! shapeIt || hmax <= h1 * margin) break;
        fs.bw[0] *= 1.3f; fs.bw[1] *= 1.15f; sh.bw1 *= 1.3f; sh.bw2 *= 1.15f;
    }
    return sh;
}

//==============================================================================
enum ParamIndex { pVowel, pArtic, pBreath, pVibrato, pVibRate, pAttack, pRelease, pBright, pFormant, pLevel, pVelSens,
                  pVoices, pGender, pDetune, pSpread, pMotion, numParams };

enum class VKind : uint8_t { female, male, choir, pad };

struct VoiceProfile
{
    const char* id; const char* name; VKind kind;
    float breath, vibrato, attack, release, gender, voices, detune, spread, motion, vowel;
    float gainDb; int maxVoices;
};

// gainDb calibrated with VaultModelLab -sweep (velocity 1.0 at C5 peaks about -9 dBFS)
const VoiceProfile profiles[] = {
    { "voice.female", "Female Voice", VKind::female, 0.10f, 0.40f, 0.06f, 0.25f, 1.0f, 1, 0, 0, 0, 0, -11.7f, 12 },
    { "voice.male",   "Male Voice",   VKind::male,   0.06f, 0.35f, 0.06f, 0.25f, 0.0f, 1, 0, 0, 0, 0, -12.5f, 12 },
    { "voice.choir",  "Choir",        VKind::choir,  0.10f, 0.35f, 0.18f, 0.6f,  0.6f, 16, 12, 0.7f, 0, 0, -15.7f, 8 },
    { "voice.pad",    "Vocal Pad",    VKind::pad,    0.30f, 0.20f, 1.2f,  2.5f,  0.8f, 12, 14, 0.9f, 0.5f, 2.0f, -14.8f, 8 },
};

// the keys a voice model sings (MIDI, FL Studio naming C5 = 60): below / above it stays silent, the way a sampled
// instrument is silent outside its range. A glottal model at 35 Hz or a whisper at 4 kHz has no clear pitch left (owner
// report item 0: every key that sounds plays its own pitch)
int lowestNote (VKind k)  { return k == VKind::female ? 36 : k == VKind::pad ? 33 : 28; }
int highestNote (VKind k) { return k == VKind::female ? 89 : 127; }

const VoiceProfile* findProfile (const juce::String& id)
{
    for (auto& p : profiles) if (id == p.id) return &p;
    return nullptr;
}

std::vector<ParamInfo> paramsFor (const VoiceProfile& p)
{
    return { { "m.vowel",   0.0f, 3.0f, p.vowel, "vowel: 0 ah, 1 oh, 2 oo, 3 ee (in between = morph)" },
             { "m.artic",   0.0f, 3.0f, 0.0f, "articulation: 0 sung, 1 'ha' (aspirated short), 2 breathy, 3 whisper" },
             { "m.breath",  0.0f, 1.0f, p.breath, "aspiration noise" },
             { "m.vibrato", 0.0f, 1.0f, p.vibrato, "vibrato depth (1 = 60 cents)" },
             { "m.vibRate", 3.0f, 8.0f, 5.3f, "vibrato rate, Hz" },
             { "m.attack",  0.005f, 4.0f, p.attack, "attack time, s" },
             { "m.release", 0.02f, 6.0f, p.release, "release time (T60), s" },
             { "m.bright",  0.0f, 1.0f, 0.5f, "voice quality: 0 soft / breathy .. 1 pressed / bright" },
             { "m.formant", -1.0f, 1.0f, 0.0f, "formant shift (-1 = 15 % lower / bigger, +1 = 15 % higher)" },
             { "m.level",   0.0f, 2.0f, 1.0f, "output level multiplier" },
             { "m.velSens", 0.0f, 1.0f, 0.5f, "velocity -> level" },
             { "m.voices",  1.0f, 24.0f, p.voices, "singers (choir / pad: 8-24)" },
             { "m.gender",  0.0f, 1.0f, p.gender, "choir mix: 0 men (tenor + bass) .. 1 women (soprano + alto)" },
             { "m.detune",  0.0f, 30.0f, p.detune, "pitch scatter between singers, cents (SD)" },
             { "m.spread",  0.0f, 1.0f, p.spread, "stereo spread of the sections" },
             { "m.motion",  0.0f, 1.0f, p.motion, "slow vowel / colour drift (pad)" } };
}

//==============================================================================
// shared helpers
struct Env
{
    float v = 0.0f, atkK = 0.01f, relMul = 0.999f; bool rel = false;
    void start (float attack, float release, double sr)
    {
        v = 0.0f; rel = false;
        atkK = 1.0f - std::exp (-1.0f / (float) (std::max (0.002f, attack) * 0.33f * sr));
        relMul = t60ToMul (std::max (0.02f, release), sr);
    }
    float next() { if (rel) v *= relMul; else v += (1.0f - v) * atkK; return v; }
};

//==============================================================================
// solo voice
class SoloVoice final : public ModelVoice
{
public:
    explicit SoloVoice (const VoiceProfile& p) : prof (p) {}

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate > 0 ? sampleRate : 48000.0;
        (void) Glottal::get();   // build the tables off the audio thread
        const auto defs = paramsFor (prof);
        for (int i = 0; i < numParams; ++i) par[i] = defs[(size_t) i].def;
        kill();
    }
    void setParams (const float* v, int n) override { for (int i = 0; i < numParams && i < n; ++i) par[i] = v[i]; }

    void noteOn (int midiNote, float velocity, float freqHz) override
    {
        if (midiNote < lowestNote (prof.kind) || midiNote > highestNote (prof.kind)) { kill(); return; }   // outside the voice's range
        note = midiNote; vel = clampf (velocity, 0.0f, 1.0f);
        hz = std::max (8.0f, freqHz);
        rng.seed ((uint32_t) (midiNote * 7907 + (int) (vel * 127.0f) * 53 + (prof.kind == VKind::male ? 3 : 11)));
        artic = juce::jlimit (0, 3, (int) std::lround (par[pArtic]));
        vt = prof.kind == VKind::male ? vtTenor : vtAlto;
        phase = 0.0f; t = 0.0f; age = 0;
        vibPhase = 0.0f; jit = 0.0f; jitT = 0.0f; drift = 0.0f; fall = 0.0f;
        env.start (artic == 1 ? 0.012f : par[pAttack], par[pRelease], sr);
        tract.reset(); nhpX1 = nhpY1 = 0.0f; post1.reset(); post2.reset(); post3.reset(); dc.reset();
        post1.highpass (prof.kind == VKind::male ? 70.0 : 110.0, 0.7, sr);
        post2.peak (3000.0, 0.8, -2.5, sr);
        post3.lowpass (artic == 3 ? 3500.0 : 7000.0, 0.6, sr);   // whisper: no hiss above the formants
        dc.setHz (20.0f, sr);
        released = false; active = true; silent = 0;
        const float velGain = 1.0f - par[pVelSens] + par[pVelSens] * (0.25f + 0.75f * vel);
        outGain = dbToGain (prof.gainDb) * par[pLevel] * velGain * (artic == 2 ? 0.6f : artic == 3 ? (prof.kind == VKind::male ? 0.8f : 0.4f) : 1.0f);   // breath noise adds level
        controlBlock (true);
    }

    void noteOff (float) override { if (! released) { released = true; env.rel = true; } }
    void setPitch (float f) override { hz = std::max (8.0f, f); }

    void render (float* left, float* right, int n) override
    {
        if (! active) return;
        juce::ScopedNoDenormals noDenormals;
        int done = 0;
        while (done < n)
        {
            const int len = std::min (32, n - done);
            controlBlock (false);
            float peak = 0.0f;
            const float* t0 = Glottal::get().table (rd0, level);
            const float* t1 = Glottal::get().table (rd0 + 1, level);
            const float te = teMix;
            for (int i = 0; i < len; ++i)
            {
                phase += inc.next();
                if (phase >= 1.0f) phase -= 1.0f;
                const float g = lookup (t0, phase) * (1.0f - rdMix) + lookup (t1, phase) * rdMix;
                const float open = phase < te ? 1.0f : 0.3f;
                const float asp = nhpTick (rng.next()) * (asp0 + (1.0f - asp0) * open * pulse) * aspAmp.next();
                float y = tract.tick (g * voiced.next() + asp);
                y = post3.tick (post2.tick (post1.tick (dc.tick (y))));
                y = safety (y * env.next() * outGain * norm.next());
                peak = std::max (peak, std::abs (y));
                left[done + i] += y; right[done + i] += y;
            }
            done += len; age += len; t = (float) age / (float) sr;
            if (released || oneShotEnded) silent = (peak < 2.0e-5f) ? silent + 1 : 0;
            if (silent > 8) { kill(); return; }
        }
    }

    void kill() override { active = false; released = false; oneShotEnded = false; }
    bool isActive() const override { return active; }

private:
    const VoiceProfile& prof;
    double sr = 48000.0;
    float par[numParams] {};
    Rng rng;
    Env env;
    Tract tract;
    float nhpX1 = 0.0f, nhpY1 = 0.0f;
    Biquad post1, post2, post3; DcBlock dc;
    Ramp inc, voiced, aspAmp, norm;
    Shape shape; float shapeF0 = 0.0f, shapeVowel = -1.0f, shapeRd = 0.0f, shapeWide = 1.0f, shapeShift = 1.0f;
    int note = 60, artic = 0, vt = vtAlto, level = 4, rd0 = 2, age = 0, silent = 0;
    float vel = 1.0f, hz = 440.0f, phase = 0.0f, t = 0.0f, vibPhase = 0.0f, jit = 0.0f, jitT = 0.0f, drift = 0.0f, fall = 0.0f;
    float rdMix = 0.0f, teMix = 0.6f, asp0 = 0.3f, pulse = 1.0f, outGain = 1.0f;
    bool active = false, released = false, oneShotEnded = false;

    // first-order high-pass on the aspiration noise (turbulence noise sits above ~500 Hz)
    float nhpTick (float x) { const float y = 0.94f * (nhpY1 + x - nhpX1); nhpX1 = x; nhpY1 = y; return y; }

    void controlBlock (bool first)
    {
        const int blk = 32;
        // ---- articulation shape
        float rd = (prof.kind == VKind::male ? 1.05f : 1.15f) + (0.5f - par[pBright]) * 1.0f - 0.35f * (vel - 0.5f);
        float voicedLvl = 1.0f, aspLvl = par[pBreath] * 0.35f, f1Wide = 1.0f, pitchFall = 0.0f;
        pulse = 1.0f; asp0 = 0.3f;
        if (artic == 2) { rd = std::max (rd, 2.3f); aspLvl += 0.35f; }
        if (artic == 3) { rd = 2.8f; voicedLvl = 0.10f; aspLvl = 1.1f; asp0 = 1.0f; f1Wide = 1.6f; }
        if (artic == 1)   // "ha": breath onset, voiced burst that decays, falling pitch, breathy end (one-shot ~0.75 s)
        {
            const float hEnd = 0.07f;
            aspLvl = t < hEnd ? 1.0f : 0.12f + 0.5f * std::max (0.0f, t - 0.45f);
            asp0 = t < hEnd ? 1.0f : 0.3f;
            voicedLvl = t < 0.05f ? 0.0f : (t < 0.09f ? (t - 0.05f) / 0.04f : std::exp (-(t - 0.09f) / 0.32f));
            rd = 1.0f + std::min (1.3f, t * 2.2f);
            f1Wide = t < hEnd ? 1.8f : 1.0f;
            if (t > 0.42f) pitchFall = -std::min (50.0f, (t - 0.42f) * 250.0f);
            if (t > 0.72f && ! env.rel) { env.relMul = t60ToMul (0.18f, sr); env.rel = true; oneShotEnded = true; }
        }
        rd = clampf (rd, Glottal::rds[0], Glottal::rds[Glottal::numRd - 1] - 0.001f);
        rd0 = 0;
        while (rd0 < Glottal::numRd - 2 && rd > Glottal::rds[rd0 + 1]) ++rd0;
        rdMix = (rd - Glottal::rds[rd0]) / (Glottal::rds[rd0 + 1] - Glottal::rds[rd0]);
        teMix = Glottal::get().te[rd0] + rdMix * (Glottal::get().te[rd0 + 1] - Glottal::get().te[rd0]);
        // ---- pitch life: vibrato with delayed onset, jitter, slow drift
        const float vibEnv = clampf ((t - 0.25f) / 0.4f, 0.0f, 1.0f) * (artic == 1 ? 0.0f : 1.0f);
        const float vibC = 60.0f * par[pVibrato] * vibEnv * std::sin (vibPhase);
        vibPhase += twoPi * par[pVibRate] * (float) blk / (float) sr;
        if (vibPhase > twoPi) vibPhase -= twoPi;
        jitT += 0.2f * (rng.next() * 1.5f - jitT);          // low-passed random: ~2 cents jitter
        jit += 0.05f * (jitT - jit);
        drift += 0.002f * (rng.next() * 2.0f - drift);      // slow drift within +-2 cents (it was +-6: each key leaned its own way)
        const float cents = vibC + jit + drift + pitchFall;
        const float f0 = hz * centsToRatio (cents);
        level = Glottal::levelFor (f0, sr);
        const float target = f0 / (float) sr;
        if (first) { inc.jump (target); voiced.jump (voicedLvl); aspAmp.jump (aspLvl); }
        else { inc.set (target, blk); voiced.set (voicedLvl, blk); aspAmp.set (aspLvl, blk); }
        // shimmer: tiny amplitude wander rides on the voiced level ramp
        voiced.v *= 1.0f + 0.03f * rng.next() * (first ? 0.0f : 1.0f);
        // ---- vocal tract
        const float shift = std::exp2 (par[pFormant] * 0.2f);
        auto fs = vowelAt (vt, par[pVowel], shift, 2.2f - 1.2f * par[pBright]);
        fs.bw[0] *= f1Wide;
        highVoice (fs, f0);
        if (fs.f[0] < f0 * 1.25f) fs.f[0] = f0 * 1.25f;    // formant tuning: F1 kept above the sung f0 (high voices)
        if (fs.f[1] < fs.f[0] + 200.0f) fs.f[1] = fs.f[0] + 200.0f;
        if (first || std::abs (f0 - shapeF0) > 0.03f * shapeF0 || std::abs (par[pVowel] - shapeVowel) > 0.02f
            || std::abs (rd - shapeRd) > 0.12f || f1Wide != shapeWide || std::abs (shift - shapeShift) > 0.01f)
        {
            const auto& G = Glottal::get();
            shape = comfortShape (fs, G.harm[rd0], G.harm[rd0 + 1], rdMix, f0, sr, 1.0f);
            shapeF0 = f0; shapeVowel = par[pVowel]; shapeRd = rd; shapeWide = f1Wide; shapeShift = shift;
            if (first) norm.jump (shape.norm); else norm.set (shape.norm, 256);
        }
        fs.bw[0] *= shape.bw1; fs.bw[1] *= shape.bw2;
        tract.set (fs, sr);
    }
};

//==============================================================================
// choir / pad: many singers, grouped by voice type, one formant filter per group.
// Runs at half the sample rate (formants and pulses stay below ~10 kHz anyway, the post filter rolls off above
// 4.5-6.5 kHz), then a 16-tap polyphase interpolator brings it back: half the CPU for 8-24 singers.
class ChoirVoice final : public ModelVoice
{
public:
    explicit ChoirVoice (const VoiceProfile& p) : prof (p) {}
    static constexpr int maxSingers = 24, hblk = 16;   // control block: 16 half-rate samples = 32 output samples

    void prepare (double sampleRate, int) override
    {
        sr = sampleRate > 0 ? sampleRate : 48000.0;
        hsr = sr * 0.5;
        (void) Glottal::get();
        const auto defs = paramsFor (prof);
        for (int i = 0; i < numParams; ++i) par[i] = defs[(size_t) i].def;
        // odd-phase taps of the 2x interpolator: Blackman-windowed sinc centred between taps 3 and 4
        float sum = 0.0f;
        for (int j = 0; j < 8; ++j)
        {
            const double d = 3.5 - j;
            const double w = 0.42 + 0.5 * std::cos (3.141592653589793 * d / 4.0) + 0.08 * std::cos (2.0 * 3.141592653589793 * d / 4.0);
            ip[j] = (float) (std::sin (3.141592653589793 * d) / (3.141592653589793 * d) * w);
            sum += ip[j];
        }
        for (auto& c : ip) c /= sum;
        kill();
    }
    void setParams (const float* v, int n) override { for (int i = 0; i < numParams && i < n; ++i) par[i] = v[i]; }

    void noteOn (int midiNote, float velocity, float freqHz) override
    {
        if (midiNote < lowestNote (prof.kind) || midiNote > highestNote (prof.kind)) { kill(); return; }   // outside the voice's range
        note = midiNote; vel = clampf (velocity, 0.0f, 1.0f);
        hz = std::max (8.0f, freqHz);
        rng.seed ((uint32_t) (midiNote * 15485863 + (int) (vel * 127.0f) * 97 + 1));
        numSingers = juce::jlimit (1, maxSingers, (int) std::lround (par[pVoices]));
        const int numFemale = (int) std::lround (par[pGender] * (float) numSingers);
        // (narrower up high: a wide spread above C6 is heard as two pitches and every key leans another way with the formants)
        const float sd = par[pDetune] * highSpread();
        Singer tmp[maxSingers];
        // the pitch scatter: the singers sit on the quantiles of a normal distribution (SD = m.detune), shuffled across the
        // singers on every note - a symmetric spread, densest at the note itself. Random Gaussian draws left each key's
        // section a few cents off in its own direction, often as two clusters either side of the note, and neighbouring
        // keys of a choir / pad sound measured up to 30 cents more or less than a semitone (owner report item 0)
        float scatter[maxSingers];
        for (int s = 0; s < numSingers; ++s)
        {
            const double q = ((double) s + 0.5) / (double) numSingers;   // inverse normal CDF (Acklam's rational approximation)
            const double a[] { -3.969683028665376e+01, 2.209460984245205e+02, -2.759285104469687e+02, 1.383577518672690e+02, -3.066479806614716e+01, 2.506628277459239e+00 };
            const double b[] { -5.447609879822406e+01, 1.615858368580409e+02, -1.556989798598866e+02, 6.680131188771972e+01, -1.328068155288572e+01 };
            const double c[] { -7.784894002430293e-03, -3.223964580411365e-01, -2.400758277161838e+00, -2.549732539343734e+00, 4.374664141464968e+00, 2.938163982698783e+00 };
            const double d[] { 7.784695709041462e-03, 3.224671290700398e-01, 2.445134137142996e+00, 3.754408661907416e+00 };
            double z;
            if (q < 0.02425) { const double r = std::sqrt (-2.0 * std::log (q)); z = (((((c[0] * r + c[1]) * r + c[2]) * r + c[3]) * r + c[4]) * r + c[5]) / ((((d[0] * r + d[1]) * r + d[2]) * r + d[3]) * r + 1.0); }
            else if (q > 1.0 - 0.02425) { const double r = std::sqrt (-2.0 * std::log (1.0 - q)); z = -(((((c[0] * r + c[1]) * r + c[2]) * r + c[3]) * r + c[4]) * r + c[5]) / ((((d[0] * r + d[1]) * r + d[2]) * r + d[3]) * r + 1.0); }
            else { const double r = q - 0.5, r2 = r * r; z = (((((a[0] * r2 + a[1]) * r2 + a[2]) * r2 + a[3]) * r2 + a[4]) * r2 + a[5]) * r / (((((b[0] * r2 + b[1]) * r2 + b[2]) * r2 + b[3]) * r2 + b[4]) * r2 + 1.0); }
            scatter[s] = clampf ((float) z, -2.5f, 2.5f) * sd;
        }
        for (int s = numSingers - 1; s > 0; --s)   // shuffled (deterministic per note: the seed above)
        {
            const int j = juce::jlimit (0, s, (int) ((0.5f + 0.5f * rng.next()) * (float) (s + 1)));
            std::swap (scatter[s], scatter[j]);
        }
        for (int s = 0; s < numSingers; ++s)
        {
            auto& g = tmp[s];
            g.cents = scatter[s];
            g.vibRate = 4.6f + 1.4f * (0.5f + 0.5f * rng.next());
            g.vibDepth = 0.5f + 0.5f * (0.5f + 0.5f * rng.next());
            g.vibPhase = pi * (1.0f + rng.next());
            g.jit = 0.0f; g.jitT = 0.0f;
            g.phase = 0.5f + 0.5f * rng.next();
            g.delay = (int) ((0.5f + 0.5f * rng.next()) * 0.04f * (float) hsr);   // ragged ensemble entry
            g.amp = dbToGain (2.0f * rng.next());
            const bool female = s < numFemale;
            g.group = female ? (s % 2 == 0 ? vtSoprano : vtAlto) : (s % 2 == 0 ? vtTenor : vtBass);
        }
        // the section is in tune on average: remove the level-weighted mean of the scatter (louder singers pull the pitch)
        float meanC = 0.0f, wsum = 0.0f;
        for (int s = 0; s < numSingers; ++s) { const float w = tmp[s].amp * tmp[s].amp; meanC += tmp[s].cents * w; wsum += w; }
        meanC /= std::max (1.0e-6f, wsum);
        // sort by section so each group's singers are contiguous (tight inner loop)
        int k = 0;
        for (int gi = 0; gi < 4; ++gi)
        {
            grp[gi].begin = k;
            for (int s = 0; s < numSingers; ++s)
                if (tmp[s].group == gi) { singer[k] = tmp[s]; singer[k].cents -= meanC; ph[k] = singer[k].phase; ampEff[k] = 0.0f; ++k; }
            grp[gi].end = k;
            grp[gi].used = grp[gi].end > grp[gi].begin;
            grp[gi].tract.reset();
            grp[gi].shift = 1.0f + 0.03f * rng.next();
        }
        const float sp = par[pSpread];
        const float pans[4] = { -0.55f * sp, 0.35f * sp, -0.3f * sp, 0.55f * sp };
        for (int gi = 0; gi < 4; ++gi)
        {
            const float a = (pans[gi] + 1.0f) * 0.25f * pi;
            grp[gi].gl = std::cos (a) * 1.4142f; grp[gi].gr = std::sin (a) * 1.4142f;
        }
        env.start (par[pAttack], par[pRelease], hsr);
        t = 0.0f; age = 0; motionPhase = 0.25f * twoPi;
        nhpX1 = nhpY1 = 0.0f;
        post1.highpass (prof.kind == VKind::pad ? 120.0 : 90.0, 0.7, hsr); post1.reset();
        post2.peak (3000.0, 0.8, -4.0, hsr); post2.reset();
        post3.lowpass (prof.kind == VKind::pad ? 4500.0 : 6500.0, 0.6, hsr); post3.reset();
        post1r = post1; post2r = post2; post3r = post3;
        std::fill (std::begin (histL), std::end (histL), 0.0f); std::fill (std::begin (histR), std::end (histR), 0.0f);
        fifoPos = 2 * hblk;
        released = false; active = true; silent = 0;
        const float velGain = 1.0f - par[pVelSens] + par[pVelSens] * (0.25f + 0.75f * vel);
        outGain = dbToGain (prof.gainDb) * par[pLevel] * velGain;
        for (auto& gr : grp) { gr.shapeVowel = -1.0f; gr.shapeRd = -1; }
        controlBlock (true);
    }

    void noteOff (float) override { if (! released) { released = true; env.rel = true; } }
    // the section's pitch spread (scatter and vibrato) narrows above ~G4: up high the fundamental sits on the first formant,
    // whose slope leans a wide cluster of partials sharp or flat - a different amount on every key
    float highSpread() const { return clampf (400.0f / hz, 0.25f, 1.0f); }
    void setPitch (float f) override { hz = std::max (8.0f, f); }

    void render (float* left, float* right, int n) override
    {
        if (! active) return;
        juce::ScopedNoDenormals noDenormals;
        int done = 0;
        while (done < n)
        {
            if (fifoPos >= 2 * hblk)
            {
                generate();
                if (! active) return;
            }
            const int m = std::min (2 * hblk - fifoPos, n - done);
            for (int i = 0; i < m; ++i) { left[done + i] += fifoL[fifoPos + i]; right[done + i] += fifoR[fifoPos + i]; }
            fifoPos += m; done += m;
        }
    }

    void kill() override { active = false; released = false; fifoPos = 2 * hblk; }
    bool isActive() const override { return active; }

private:
    struct Singer { float phase, cents, vibRate, vibDepth, vibPhase, jit, jitT, amp; int delay, group; };
    struct Group { Tract tract; bool used = false; int begin = 0, end = 0; float shift = 1.0f, gl = 1.0f, gr = 1.0f;
                   Shape shape; Ramp norm; float shapeVowel = -1.0f, shapeF0 = 0.0f; int shapeRd = -1; };
    const VoiceProfile& prof;
    double sr = 48000.0, hsr = 24000.0;
    float par[numParams] {};
    Singer singer[maxSingers] {};
    float ph[maxSingers] {}, inc[maxSingers] {}, ampEff[maxSingers] {};
    Group grp[4];
    int numSingers = 16, note = 60, level = 4, rd0 = 2, age = 0, silent = 0, fifoPos = 2 * hblk;
    Rng rng; Env env;
    Biquad post1, post2, post3, post1r, post2r, post3r;
    float ip[8] {}, histL[8] {}, histR[8] {}, fifoL[2 * hblk] {}, fifoR[2 * hblk] {};
    float nhpX1 = 0.0f, nhpY1 = 0.0f;
    float vel = 1.0f, hz = 440.0f, t = 0.0f, asp = 0.0f, gate = 0.7f, outGain = 1.0f, motionPhase = 0.0f;
    bool active = false, released = false;

    float nhpTick (float x) { const float y = 0.94f * (nhpY1 + x - nhpX1); nhpX1 = x; nhpY1 = y; return y; }

    // one control block: 16 half-rate samples -> 32 output samples into the fifo
    void generate()
    {
        controlBlock (false);
        const float* tb = Glottal::get().table (rd0, level);
        float peak = 0.0f;
        for (int m = 0; m < hblk; ++m)
        {
            const float nz = nhpTick (rng.next()) * asp * gate;
            float l = 0.0f, r = 0.0f;
            for (int gi = 0; gi < 4; ++gi)
            {
                auto& g = grp[gi];
                if (! g.used) continue;
                float acc = 0.0f;
                for (int j = g.begin; j < g.end; ++j)
                {
                    float p = ph[j] + inc[j];
                    p -= p >= 1.0f ? 1.0f : 0.0f;
                    ph[j] = p;
                    acc += lookup (tb, p) * ampEff[j];
                }
                const float y = g.tract.tick4 (acc + nz) * g.norm.next();
                l += y * g.gl; r += y * g.gr;
            }
            const float e = env.next() * outGain;
            l = post3.tick (post2.tick (post1.tick (l))) * e;
            r = post3r.tick (post2r.tick (post1r.tick (r))) * e;
            l = safety (l); r = safety (r);
            // 2x polyphase interpolation: even output = the input delayed 4, odd = windowed sinc between taps 3 and 4
            for (int j = 7; j > 0; --j) { histL[j] = histL[j - 1]; histR[j] = histR[j - 1]; }
            histL[0] = l; histR[0] = r;
            float ol = 0.0f, orr = 0.0f;
            for (int j = 0; j < 8; ++j) { ol += ip[j] * histL[j]; orr += ip[j] * histR[j]; }
            fifoL[2 * m] = histL[4]; fifoR[2 * m] = histR[4];
            fifoL[2 * m + 1] = ol; fifoR[2 * m + 1] = orr;
            peak = std::max (peak, std::max (std::abs (l), std::abs (r)));
        }
        fifoPos = 0;
        age += hblk; t = (float) age / (float) hsr;
        if (released) silent = peak < 2.0e-5f ? silent + 1 : 0;
        if (silent > 16) kill();
    }

    void controlBlock (bool first)
    {
        // voice quality: choirs sing lighter than soloists; the pad is breathy
        float rd = (prof.kind == VKind::pad ? 2.0f : 1.4f) + (0.5f - par[pBright]) * 0.8f - 0.3f * (vel - 0.5f);
        rd = clampf (rd, Glottal::rds[0], Glottal::rds[Glottal::numRd - 1]);
        rd0 = 0;
        for (int r = 1; r < Glottal::numRd; ++r) if (std::abs (Glottal::rds[r] - rd) < std::abs (Glottal::rds[rd0] - rd)) rd0 = r;
        const auto& G = Glottal::get();
        asp = par[pBreath] * 0.3f * std::sqrt ((float) numSingers);   // breath of the whole section (incoherent sum)
        gate = 0.3f + 0.7f * G.te[rd0];      // many singers: the pulsed breath noise averages to the open-phase share
        level = Glottal::levelFor (hz * 1.06f, hsr);
        const float vibEnv = clampf ((t - 0.3f) / 0.6f, 0.0f, 1.0f);
        // every singer moves on its own (scatter, jitter, vibrato at its own rate and phase), the section as a whole stays
        // on the note: the level-weighted mean of the singers who are singing is taken out at every control block (else the
        // centre wanders by +-10 cents with the random phases and every key of a choir sound is tuned differently)
        float cs[maxSingers];
        float meanC = 0.0f, wsum = 0.0f;
        for (int s = 0; s < numSingers; ++s)
        {
            auto& g = singer[s];
            g.vibPhase += twoPi * g.vibRate * (float) hblk / (float) hsr;
            if (g.vibPhase > twoPi) g.vibPhase -= twoPi;
            g.jitT += 0.2f * (rng.next() * 3.0f - g.jitT);
            g.jit += 0.05f * (g.jitT - g.jit);
            cs[s] = g.cents + g.jit + 60.0f * par[pVibrato] * highSpread() * g.vibDepth * vibEnv * std::sin (g.vibPhase);
            if (g.delay > 0) { g.delay -= hblk; ampEff[s] = 0.0f; } else ampEff[s] = g.amp;
            const float w = ampEff[s] * ampEff[s];
            meanC += cs[s] * w; wsum += w;
        }
        meanC = wsum > 0.0f ? meanC / wsum : 0.0f;
        for (int s = 0; s < numSingers; ++s) inc[s] = hz * centsToRatio (cs[s] - meanC) / (float) hsr;
        if (! first && (age % (hblk * 4)) != 0) return;   // formants: every 64 half-rate samples
        float vowel = par[pVowel];
        if (par[pMotion] > 0.0f)   // slow drift between the set vowel and its neighbour (pad)
        {
            motionPhase += twoPi * 0.07f * (float) (hblk * 4) / (float) hsr;
            vowel = clampf (vowel + par[pMotion] * 0.6f * (0.5f - 0.5f * std::cos (motionPhase)), 0.0f, 3.0f);
        }
        const float shift = std::exp2 (par[pFormant] * 0.2f);
        for (int gi = 0; gi < 4; ++gi)
        {
            auto& gr = grp[gi];
            if (! gr.used) continue;
            auto fs = vowelAt (gi, vowel, shift * gr.shift, 2.4f - 1.2f * par[pBright]);
            if (fs.f[0] < hz * 1.25f) fs.f[0] = hz * 1.25f;
            if (fs.f[1] < fs.f[0] + 200.0f) fs.f[1] = fs.f[0] + 200.0f;
            highVoice (fs, hz);
            if (first || std::abs (vowel - gr.shapeVowel) > 0.02f || std::abs (hz - gr.shapeF0) > 0.03f * gr.shapeF0 || rd0 != gr.shapeRd)
            {
                gr.shape = comfortShape (fs, G.harm[rd0], G.harm[rd0], 0.0f, hz, hsr, -3.0f);   // ensembles smear partials: H1 must lead
                gr.shapeVowel = vowel; gr.shapeF0 = hz; gr.shapeRd = rd0;
                const float g = gr.shape.norm / std::sqrt ((float) numSingers);
                if (first) gr.norm.jump (g); else gr.norm.set (g, hblk * 4);
            }
            fs.bw[0] *= gr.shape.bw1; fs.bw[1] *= gr.shape.bw2;
            gr.tract.set (fs, hsr);
        }
    }
};
} // namespace

juce::StringArray voiceModelIds()
{
    juce::StringArray s;
    for (auto& p : profiles) s.add (p.id);
    return s;
}

ModelInfo voiceModelInfo (const juce::String& id)
{
    const auto* p = findProfile (id);
    jassert (p != nullptr);
    if (p == nullptr) return {};
    if (p->kind == VKind::choir || p->kind == VKind::pad)
        return { p->id, p->name, paramsFor (*p), p->maxVoices, [p] { return std::make_unique<ChoirVoice> (*p); } };
    return { p->id, p->name, paramsFor (*p), p->maxVoices, [p] { return std::make_unique<SoloVoice> (*p); } };
}
} // namespace vault::models
