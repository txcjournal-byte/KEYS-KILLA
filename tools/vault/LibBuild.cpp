// VaultLibBuild - builds the CHROME VAULT sample library from CC0 source recordings (docs/VAULT.md, "Sample library").
//
//   VaultLibBuild [-map tools/vault/library_map.json] [-out /path/Library] [-only id1,id2] [-threads N] [-dry]
//   VaultLibBuild -qc [-out /path/Library] [-only ids] [-demo /dir]     measure a built library (qc_report.txt, qc.json)
//
// Per source file: read, resample to 44.1 kHz (windowed sinc), remove DC, trim silence (with a short pre-roll and a fade-in
// over it), cap the length (fade the tail), verify / correct the root note with YIN (octave-convention errors are fixed per
// articulation, single mismatches reported, the residual detune stored as zone fine-tune), measure loudness (BS.1770
// K-weighting, loudest 400 ms), find a crossfade loop for sustained articulations (rising zero crossings, normalized
// cross-correlation around both ends, >= 300 ms, crossfade 50-150 ms), then per articulation match the velocity layers'
// loudness (a smoothed per-register curve per layer; natural layer spacing kept, or imposed with "layerDb" for sources that
// were normalized), set release samples relative to their note, normalise the instrument peak, encode OggVorbis (q 0.6).
// Writes <out>/library.json, <out>/audio/<instrument>/*.ogg and <out>/build_report.txt.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Source/vault/VaultLibrary.h"
#include "../../Source/vault/VaultEngine.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <thread>

namespace
{
constexpr double outRate = 44100.0;

struct Opts
{
    int quality = 6;
    double preRollMs = 4, silenceDb = -62, maxSeconds = 16, tailFadeMs = 250, peakDb = -1.0;
    double minLoopMs = 300, maxLoopMs = 2600, xfadeMs = 100;
    int extendKeys = 6;
    double release = 0.3, releaseDecayDb = 6.0;
    bool loop = false, oneShot = false;
    bool keepTuning = false;   // "tune": "keep" = natural tuning (stretch tuning, bar tuning), only > 30 cents fixed
    bool trustNames = false;   // "tune": "name" = inharmonic sources (bells, chimes): the file names are the roots, no pitch correction
    double pitchFrom = 0.06;   // s: where the pitch analysis of a one-shot starts (later for pianos: the attack glides sharp)
    int maxStretch = 4;        // semitones a zone may be played away from its root before the report flags it
    bool spectrumPitch = false; // "pitchMethod": "spectrum" - fine tune from the fundamental's spectral peak (mallets, tines)
    double pitchSpan = 0;      // s: length of the pitch analysis (0 = auto: 0.35 s one-shots, the whole sustain of loops)
    bool pitchFromSet = false; // an explicit "pitchFrom" also applies to loops (voices: tune the start the ear hears first)
    bool densePitch = false;   // "pitchMedian": "dense" - YIN frames every half window, median of the clearly periodic ones (voices)
    void read (const juce::var& v)
    {
        if (v.hasProperty ("tune")) { keepTuning = v["tune"].toString() == "keep"; trustNames = v["tune"].toString() == "name"; }
        if (v.hasProperty ("maxStretch")) maxStretch = (int) v["maxStretch"];
        if (v.hasProperty ("pitchMethod")) spectrumPitch = v["pitchMethod"].toString() == "spectrum";
        if (v.hasProperty ("pitchFrom")) { pitchFrom = (double) v["pitchFrom"]; pitchFromSet = true; }
        if (v.hasProperty ("pitchSpan")) pitchSpan = (double) v["pitchSpan"];
        if (v.hasProperty ("pitchMedian")) densePitch = v["pitchMedian"].toString() == "dense";
        auto num = [&] (const char* k, double& d) { if (v.hasProperty (k)) d = (double) v[k]; };
        if (v.hasProperty ("quality")) quality = (int) v["quality"];
        if (v.hasProperty ("extendKeys")) extendKeys = (int) v["extendKeys"];
        num ("preRollMs", preRollMs); num ("silenceDb", silenceDb); num ("maxSeconds", maxSeconds); num ("tailFadeMs", tailFadeMs);
        num ("peakDb", peakDb); num ("minLoopMs", minLoopMs); num ("maxLoopMs", maxLoopMs); num ("xfadeMs", xfadeMs);
        num ("release", release); num ("releaseDecayDb", releaseDecayDb);
        if (v.hasProperty ("loop")) loop = (bool) v["loop"];
        if (v.hasProperty ("oneShot")) oneShot = (bool) v["oneShot"];
    }
};

struct Item   // one source file
{
    juce::File src;
    juce::String noteName, velName;
    int rr = 0, namedNote = 60, root = 60, layer = 0;
    bool release = false;
    double levelDb = -22.0;                 // release samples: loudness relative to their note
    // pitch from the audio instead of the file name ("pitch": "detect"): harmonic-sum search over [detLo, detHi] (MIDI)
    // with the source's partial ratios (timpani: 1, 1.5, 1.98); files sharing a {group} get the group's median pitch
    bool detect = false;
    double detLo = 36, detHi = 84;
    std::vector<double> partials { 1.0, 2.0, 3.0, 4.0 };
    juce::String group;
    float noiseDb = 0;                      // recording noise floor: RMS before the onset vs the peak (0 = not measurable)
    int anaFrom = 0, anaLen = 0;            // the stretch the pitch was measured on
    bool octaveFixed = false;               // the map set the octave ("octave"): no automatic octave correction
    // analysis
    juce::AudioBuffer<float> audio;         // processed, 44.1 kHz
    float f0 = 0, aperiodicity = 1, cents = 0;
    bool pitchOk = false;
    float loud = -100;                      // K-weighted loudness (loudest 400 ms), dB
    int loopStart = 0, loopEnd = 0, xfade = 0;
    float loopCorr = 0;
    double gainDb = 0;
    juce::String outName, msg;
    int lo = 0, hi = 127, vlo = 1, vhi = 127;
};

int parseNote (const juce::String& s)   // "C#4" / "Db4" / "A#-1" -> MIDI (C4 = 60)
{
    if (s.isEmpty()) return -1;
    static const int pcs[7] { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
    const juce::juce_wchar c = juce::CharacterFunctions::toUpperCase (s[0]);
    if (c < 'A' || c > 'G') return -1;
    int pc = pcs[c - 'A'];
    int i = 1;
    if (s[i] == '#') { ++pc; ++i; } else if (s[i] == 'b') { --pc; ++i; }
    const auto octStr = s.substring (i);
    if (! octStr.containsOnly ("-0123456789") || octStr.isEmpty()) return -1;
    return 12 * (octStr.getIntValue() + 1) + pc;
}

std::regex makeRegex (const juce::String& pattern)
{
    std::string out;
    for (int i = 0; i < pattern.length(); ++i)
    {
        if (pattern.substring (i).startsWith ("{note}")) { out += "([A-Ga-g][#b]?-?[0-9])"; i += 5; continue; }
        if (pattern.substring (i).startsWith ("{vel}"))  { out += "([A-Za-z0-9]+)"; i += 4; continue; }
        if (pattern.substring (i).startsWith ("{rr}"))   { out += "([0-9]+)"; i += 3; continue; }
        if (pattern.substring (i).startsWith ("{group}")) { out += "([A-Za-z0-9]+)"; i += 6; continue; }
        const juce::juce_wchar c = pattern[i];
        if (c == '*') { out += ".*"; continue; }
        if (juce::String ("\\^$.|?+()[]{}").containsChar (c)) out += '\\';
        out += (char) c;
    }
    return std::regex (out);
}

//==============================================================================
// DSP helpers
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double tick (double x) { const double y = b0 * x + z1; z1 = b1 * x - a1 * y + z2; z2 = b2 * x - a2 * y; return y; }
};

// BS.1770 K-weighting at 44.1 kHz (shelf + RLB high-pass), loudest 400 ms window, LUFS-like dB
float loudnessDb (const juce::AudioBuffer<float>& b)
{
    const int n = b.getNumSamples(), nc = b.getNumChannels();
    if (n <= 0) return -100.0f;
    std::vector<double> pw ((size_t) n, 0.0);
    for (int c = 0; c < nc; ++c)
    {
        Biquad s1 { 1.53084886, -2.65097996, 1.16916684, -1.66375011, 0.71265753 };    // 48k coefficients are close enough at 44.1k
        Biquad s2 { 1.0, -2.0, 1.0, -1.99004745, 0.99007225 };
        const float* x = b.getReadPointer (c);
        for (int i = 0; i < n; ++i) { const double y = s2.tick (s1.tick (x[i])); pw[(size_t) i] += y * y; }
    }
    const int win = (int) (0.4 * outRate), hop = (int) (0.1 * outRate);
    double best = 0, acc = 0;
    if (n <= win) { for (double v : pw) acc += v; best = acc / std::max (1, n); }
    else
        for (int s0 = 0; s0 + win <= n; s0 += hop)
        {
            acc = 0;
            for (int i = 0; i < win; ++i) acc += pw[(size_t) (s0 + i)];
            best = std::max (best, acc / win);
        }
    return (float) (-0.691 + 10.0 * std::log10 (std::max (1.0e-12, best)));
}

// YIN (de Cheveigne & Kawahara 2002) on one frame; returns f0 (0 = none) and the aperiodicity (cmnd minimum)
float yinFrame (const float* x, int w, double sr, double fmin, double fmax, float& aper)
{
    const int tauMin = std::max (2, (int) std::floor (sr / fmax)), tauMax = std::min (w - 2, (int) std::ceil (sr / fmin));
    aper = 1.0f;
    if (tauMax <= tauMin + 2) return 0.0f;
    std::vector<double> d ((size_t) tauMax + 2, 0.0), cm ((size_t) tauMax + 2, 1.0);
    for (int tau = 1; tau <= tauMax + 1; ++tau)
    {
        double s = 0;
        for (int j = 0; j < w - tauMax - 1; ++j) { const double e = (double) x[j] - x[j + tau]; s += e * e; }
        d[(size_t) tau] = s;
    }
    double run = 0;
    for (int tau = 1; tau <= tauMax + 1; ++tau) { run += d[(size_t) tau]; cm[(size_t) tau] = run > 0 ? d[(size_t) tau] * tau / run : 1.0; }
    int best = -1;
    for (int tau = tauMin; tau <= tauMax; ++tau)
        if (cm[(size_t) tau] < 0.15)
        {
            while (tau + 1 <= tauMax && cm[(size_t) tau + 1] < cm[(size_t) tau]) ++tau;
            best = tau; break;
        }
    if (best < 0)   // no dip under the threshold: the global minimum
    {
        best = tauMin;
        for (int tau = tauMin; tau <= tauMax; ++tau) if (cm[(size_t) tau] < cm[(size_t) best]) best = tau;
    }
    aper = (float) cm[(size_t) best];
    // parabolic interpolation
    double t = best;
    if (best > 1 && best < tauMax)
    {
        const double a = cm[(size_t) best - 1], b = cm[(size_t) best], c = cm[(size_t) best + 1];
        const double den = a - 2 * b + c;
        if (std::abs (den) > 1e-12) t = best + 0.5 * (a - c) / den;
    }
    return (float) (sr / t);
}

// median f0 over several frames of the mid signal starting at `from`
// dense = frames every half window, median of the clearly periodic ones ("pitchMedian": "dense", voices); otherwise the
// median of 7 evenly spread frames (instruments - vibrato strings tune best on the plain 7-frame median)
float detectF0 (const juce::AudioBuffer<float>& b, int from, int len, double fExpected, float& aperOut, bool dense = false)
{
    std::vector<float> mono ((size_t) b.getNumSamples());
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        float s = 0; for (int c = 0; c < b.getNumChannels(); ++c) s += b.getSample (c, i);
        mono[(size_t) i] = s / (float) b.getNumChannels();
    }
    const double fmin = fExpected * 0.45, fmax = std::min (5000.0, fExpected * 2.2);
    const int w = juce::jlimit (1024, 8192, (int) (3.2 * outRate / fmin));
    std::vector<float> f0s, aps;
    // frames every half window over the span (at least 7, at most 64): a sung note drifts, a few frames can miss its centre
    const int frames = dense ? juce::jlimit (7, 64, len / std::max (1, w / 2)) : 7;
    for (int k = 0; k < frames; ++k)
    {
        const int s0 = from + (int) ((double) k / (double) frames * std::max (0, len - w));
        if (s0 < 0 || s0 + w > (int) mono.size()) continue;
        float ap;
        const float f = yinFrame (mono.data() + s0, w, outRate, fmin, fmax, ap);
        if (f > 0) { f0s.push_back (f); aps.push_back (ap); }
    }
    if (f0s.empty()) { aperOut = 1; return 0; }
    // clearly periodic frames only (YIN dip < 0.15) when there are enough: breathy / noisy frames bias the median
    std::vector<float> clear;
    for (size_t i = 0; i < f0s.size(); ++i) if (aps[i] < 0.15f) clear.push_back (f0s[i]);
    if (dense && clear.size() >= 3 && clear.size() < f0s.size()) f0s = clear;
    std::sort (f0s.begin(), f0s.end()); std::sort (aps.begin(), aps.end());
    aperOut = aps[aps.size() / 2];
    return f0s[f0s.size() / 2];
}

// harmonic-sum pitch search for sources without a usable note name (timpani, tuned drums, slit drums): the log spectrum
// of [from, from + len) is scored at f0 x every partial ratio (0.02-semitone grid over [lo, hi] MIDI); the best f0 wins.
// The first partial weighs most, so a sub-octave (whose partials miss the spectrum) loses to the real principal tone.
double harmonicSumMidi (const juce::AudioBuffer<float>& b, int from, int len, double lo, double hi, const std::vector<double>& partials)
{
    constexpr int order = 17, n = 1 << order;
    from = juce::jlimit (0, std::max (0, b.getNumSamples() - 1), from);
    len = std::min ({ len, n, b.getNumSamples() - from });
    if (len < 2048) return 0.0;
    std::vector<float> data ((size_t) n * 2, 0.0f);
    for (int i = 0; i < len; ++i)
    {
        float s = 0; for (int c = 0; c < b.getNumChannels(); ++c) s += b.getSample (c, from + i);
        data[(size_t) i] = s * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) len));
    }
    juce::dsp::FFT fft (order);
    fft.performFrequencyOnlyForwardTransform (data.data());
    float mx = 1e-12f;
    for (int k = 1; k < n / 2; ++k) mx = std::max (mx, data[(size_t) k]);
    std::vector<float> L ((size_t) n / 2);
    for (int k = 0; k < n / 2; ++k) L[(size_t) k] = std::max (0.0f, 60.0f + 20.0f * std::log10 (data[(size_t) k] / mx + 1e-9f));
    double best = -1, bestM = 0;
    std::vector<std::pair<double, double>> scores;
    for (double m = lo; m <= hi; m += 0.02)
    {
        const double f0 = 440.0 * std::pow (2.0, (m - 69.0) / 12.0);
        double sc = 0;
        for (size_t p = 0; p < partials.size(); ++p)
        {
            const double fr = f0 * partials[p];
            const int k0 = (int) (fr * 0.985 * n / outRate), k1 = std::min (n / 2 - 1, (int) (fr * 1.015 * n / outRate) + 1);
            float v = 0; for (int k = std::max (1, k0); k <= k1; ++k) v = std::max (v, L[(size_t) k]);
            sc += v * (p == 0 ? 1.0 : 0.8);
        }
        scores.push_back ({ m, sc });
        if (sc > best) { best = sc; bestM = m; }
    }
    // every band is +-1.5 % wide, so the best score holds on a plateau around the true pitch: take the middle of the
    // plateau that holds the first best grid point (taking its first point would read 25 cents flat)
    {
        size_t i0 = 0;
        while (i0 < scores.size() && scores[i0].second < best - 1e-6) ++i0;
        size_t i1 = i0;
        while (i1 + 1 < scores.size() && scores[i1 + 1].second >= best - 1e-6) ++i1;
        if (i0 < scores.size()) bestM = 0.5 * (scores[i0].first + scores[i1].first);
    }
    // refine on the STRONGEST of the listed partials near the winner (best signal-to-noise; a weak fundamental would make
    // the estimate wander): parabolic interpolation of its log-magnitude peak, divided by its ratio
    const double f0 = 440.0 * std::pow (2.0, (bestM - 69.0) / 12.0);
    int kb = -1; double ratio = 1.0;
    for (size_t p = 0; p < partials.size(); ++p)
        for (int k = (int) (f0 * partials[p] * 0.98 * n / outRate); k <= (int) (f0 * partials[p] * 1.02 * n / outRate) + 1; ++k)
            if (k > 1 && k < n / 2 - 1 && (kb < 0 || data[(size_t) k] > data[(size_t) kb])) { kb = k; ratio = partials[p]; }
    if (kb > 1 && kb < n / 2 - 1 && data[(size_t) kb] >= data[(size_t) kb - 1] && data[(size_t) kb] >= data[(size_t) kb + 1])
    {
        const double a = std::log (data[(size_t) kb - 1] + 1e-12), c0 = std::log (data[(size_t) kb] + 1e-12), c = std::log (data[(size_t) kb + 1] + 1e-12);
        const double den = a - 2 * c0 + c;
        const double k = kb + (std::abs (den) > 1e-12 ? 0.5 * (a - c) / den : 0.0);
        const double m = 12.0 * std::log2 (k * outRate / n / ratio / 440.0) + 69.0;
        if (std::abs (m - bestM) < 0.3) return m;
    }
    return bestM;
}

// YIN disagrees with the expected root: the harmonic-sum spectrum (+-2.5 semitones around the root, harmonics 1-5) decides.
// Both agree (within 0.6 semitone) -> the root moves to them; the spectrum confirms the root -> the name stays (the spectrum
// gives the detune); YIN is far off (> 3 semitones: octave / garbage) and the spectrum finds a clear pitch inside the window
// -> the spectrum alone; otherwise the root stays without a fine tune. YIN alone mis-hears bright piano trebles, weak bass
// fundamentals (organ pedals, low strings) and mallet bars.
struct PitchVerdict { int root = 60; double measured = 60, spectrum = 0; juce::String verdict; };
PitchVerdict pitchVerdict (const juce::AudioBuffer<float>& b, int from, int len, int root, double yinMidi)
{
    PitchVerdict v;
    v.root = root; v.measured = root;
    const double hs = harmonicSumMidi (b, from, len, root - 2.5, root + 2.5, { 1.0, 2.0, 3.0, 4.0, 5.0 });
    v.spectrum = hs;
    const int hsNear = (int) std::lround (hs);
    const bool clear = hs > 0 && std::abs (hs - root) < 2.3;   // not stuck at the window edge
    if (clear && std::abs (hs - yinMidi) < 0.6 && hsNear != root) { v.root = hsNear; v.measured = hs; v.verdict = "root corrected (YIN and spectrum agree)"; }
    else if (clear && hsNear == root) { v.measured = hs; v.verdict = "the name is right (spectrum)"; }
    else if (clear && std::abs (yinMidi - root) > 3.0) { v.root = hsNear; v.measured = hs; v.verdict = "root from the spectrum (YIN unusable)"; }
    else v.verdict = "kept the name, no fine tune (check!)";
    return v;
}

std::vector<float> envelope (const juce::AudioBuffer<float>& b, int hop)
{
    std::vector<float> e;
    for (int s0 = 0; s0 < b.getNumSamples(); s0 += hop)
    {
        double acc = 0; const int len = std::min (hop, b.getNumSamples() - s0);
        for (int c = 0; c < b.getNumChannels(); ++c) { const float* x = b.getReadPointer (c, s0); for (int i = 0; i < len; ++i) acc += (double) x[i] * x[i]; }
        e.push_back ((float) std::sqrt (acc / std::max (1, len * b.getNumChannels())));
    }
    return e;
}

// loop search: rising zero crossings of the mid signal, loop >= minLoop, crossfade xf before loopEnd blending with the
// frames before loopStart. Score = normalized cross-correlation of the windows [-xf, +12 ms] around both points
// (decimated by 4) - level mismatch + a small bonus for longer loops.
bool findLoop (Item& it, const Opts& o)
{
    const auto& b = it.audio;
    const int n = b.getNumSamples();
    const int hop = 441;   // 10 ms
    const auto env = envelope (b, hop);
    if (env.size() < 50) return false;
    // the attack peak: the loudest point of the first second (a crescendo later in the note must not move it)
    const int early = std::min ((int) env.size(), 100);
    const int pk = (int) (std::max_element (env.begin(), env.begin() + early) - env.begin());
    const float pkv = *std::max_element (env.begin(), env.end());
    // sustain level: median of the envelope after the attack where it is above -24 dB of the peak
    std::vector<float> sus;
    for (size_t i = (size_t) pk + 15; i < env.size(); ++i) if (env[i] > pkv * 0.063f) sus.push_back (env[i]);
    if (sus.size() < 40) return false;
    std::sort (sus.begin(), sus.end());
    const float med = sus[sus.size() / 2];
    int attackEnd = pk + 15;   // >= 150 ms after the peak
    while (attackEnd < (int) env.size() && env[(size_t) attackEnd] > med * 1.6f) ++attackEnd;
    int stableEnd = (int) env.size() - 1;
    while (stableEnd > attackEnd && env[(size_t) stableEnd] < med * 0.6f) --stableEnd;
    stableEnd -= 10;   // stay 100 ms clear of the release
    const int xf = juce::jlimit ((int) (0.05 * outRate), (int) (0.5 * outRate), (int) (o.xfadeMs * 0.001 * outRate));
    const int minL = (int) (o.minLoopMs * 0.001 * outRate), maxL = (int) (o.maxLoopMs * 0.001 * outRate);
    const int regionStart = attackEnd * hop + xf + 2205, regionEnd = std::min (stableEnd * hop, n - (int) (0.05 * outRate));
    if (regionEnd - regionStart < minL) return false;

    std::vector<float> mid ((size_t) n);
    for (int i = 0; i < n; ++i) { float s = 0; for (int c = 0; c < b.getNumChannels(); ++c) s += b.getSample (c, i); mid[(size_t) i] = s; }
    std::vector<int> zc;
    for (int i = regionStart; i < regionEnd; ++i) if (mid[(size_t) i - 1] < 0.0f && mid[(size_t) i] >= 0.0f) zc.push_back (i);
    if (zc.size() < 4) return false;
    const int post = (int) (0.012 * outRate);
    auto ncc = [&] (int a, int e2)
    {
        double sab = 0, saa = 0, sbb = 0;
        for (int i = -xf; i < post; i += 4)
        {
            const double x = mid[(size_t) (a + i)], y = mid[(size_t) (e2 + i)];
            sab += x * y; saa += x * x; sbb += y * y;
        }
        const double c = sab / std::sqrt (std::max (1e-20, saa * sbb));
        const double lev = std::abs (10.0 * std::log10 (std::max (1e-20, saa) / std::max (1e-20, sbb)));
        return c - 0.05 * lev;
    };
    // loop-end candidates: the last ~0.5 s of the stable region
    std::vector<int> ends;
    for (int i = (int) zc.size() - 1; i >= 0 && (int) ends.size() < 14; --i)
        if (zc[(size_t) i] > regionEnd - (int) (0.5 * outRate) || ends.empty()) ends.push_back (zc[(size_t) i]);
    double best = -10; int bs = 0, be = 0;
    for (int e2 : ends)
    {
        if (e2 + post >= n) continue;
        for (int s : zc)
        {
            const int L = e2 - s;
            if (L < minL || L > maxL || s - xf - 4 < regionStart - xf) continue;
            const double sc = ncc (s, e2) + 0.03 * (double) L / outRate;
            if (sc > best) { best = sc; bs = s; be = e2; }
        }
    }
    if (be <= bs) return false;
    it.loopCorr = (float) (best - 0.03 * (double) (be - bs) / outRate);
    // a weakly correlated loop (ensembles, vibrato) gets the longest crossfade
    const int xfUse = it.loopCorr < 0.6f ? std::max (xf, (int) (0.15 * outRate)) : xf;
    it.loopStart = bs; it.loopEnd = be; it.xfade = std::min ({ xfUse, (be - bs) / 3, bs - 16 });
    return true;
}

//==============================================================================
struct Articulation
{
    juce::String name;
    Opts o;
    std::map<juce::String, double> layerDb;
    juce::StringArray velOrder;
    std::vector<Item> items;
};
struct Instrument
{
    juce::String id, name, family, credit;
    std::vector<Articulation> arts;
};

std::mutex printLock;
juce::String report;
void log (const juce::String& s)
{
    const std::lock_guard<std::mutex> g (printLock);
    std::printf ("%s\n", s.toRawUTF8());
    std::fflush (stdout);
    report << s << "\n";
}

void parallelFor (int count, int threads, const std::function<void (int)>& fn)
{
    std::atomic<int> next { 0 };
    std::vector<std::thread> pool;
    for (int t = 0; t < threads; ++t)
        pool.emplace_back ([&] { for (int i; (i = next.fetch_add (1)) < count;) fn (i); });
    for (auto& t : pool) t.join();
}

// read + resample + DC + trim + length cap; false if unreadable / silent
bool loadItem (Item& it, const Opts& o)
{
    juce::AudioFormatManager fm;
    fm.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> r (fm.createReaderFor (it.src));
    if (r == nullptr || r->lengthInSamples <= 0) { it.msg = "unreadable"; return false; }
    const int nc = juce::jlimit (1, 2, (int) r->numChannels);
    juce::AudioBuffer<float> in (nc, (int) r->lengthInSamples);
    r->read (&in, 0, in.getNumSamples(), 0, true, nc > 1);
    // identical channels: store mono
    int ch = nc;
    if (nc == 2)
    {
        double diff = 0, sum = 0;
        for (int i = 0; i < in.getNumSamples(); ++i) { diff += std::abs (in.getSample (0, i) - in.getSample (1, i)); sum += std::abs (in.getSample (0, i)); }
        if (diff < sum * 1e-4) ch = 1;
    }
    juce::AudioBuffer<float> a (ch, in.getNumSamples());
    for (int c = 0; c < ch; ++c) a.copyFrom (c, 0, in, c, 0, in.getNumSamples());
    if (std::abs (r->sampleRate - outRate) > 0.5)
    {
        const double ratio = r->sampleRate / outRate;
        const int outN = (int) std::floor (in.getNumSamples() / ratio) - 8;
        juce::AudioBuffer<float> rs (ch, std::max (1, outN));
        for (int c = 0; c < ch; ++c)
        {
            juce::WindowedSincInterpolator interp;
            interp.process (ratio, a.getReadPointer (c), rs.getWritePointer (c), rs.getNumSamples());
        }
        a = std::move (rs);
        it.msg << "resampled " << juce::String (r->sampleRate) << " Hz; ";
    }
    const int n = a.getNumSamples();
    // DC: mean removal + 5 Hz one-pole high-pass
    for (int c = 0; c < ch; ++c)
    {
        float* x = a.getWritePointer (c);
        double mean = 0; for (int i = 0; i < n; ++i) mean += x[i]; mean /= n;
        const double R = std::exp (-2.0 * juce::MathConstants<double>::pi * 5.0 / outRate);
        double x1 = 0, y1 = 0;
        for (int i = 0; i < n; ++i) { const double v = x[i] - mean; const double y = v - x1 + R * y1; x1 = v; y1 = y; x[i] = (float) y; }
    }
    // trim: first / last sample above silenceDb (relative to the peak), pre-roll before the onset
    float pk = 0;
    for (int c = 0; c < ch; ++c) pk = std::max (pk, a.getMagnitude (c, 0, n));
    if (pk < 1.0e-5f) { it.msg = "silent"; return false; }
    const float thr = pk * juce::Decibels::decibelsToGain ((float) o.silenceDb);
    int first = n, last = 0;
    for (int i = 0; i < n; ++i) for (int c = 0; c < ch; ++c) if (std::abs (a.getSample (c, i)) > thr) { first = std::min (first, i); last = std::max (last, i); }
    // the onset proper (-40 dB of the peak) decides where the pre-roll starts
    const float onThr = pk * 0.01f;
    int onset = first;
    for (int i = first; i < n; ++i) { bool hit = false; for (int c = 0; c < ch; ++c) hit |= std::abs (a.getSample (c, i)) > onThr; if (hit) { onset = i; break; } }
    // noise floor: the RMS of the lead-in before the onset (at least 20 ms of it, ending 10 ms before the onset) vs the peak
    if (const int nEnd = onset - (int) (0.01 * outRate); nEnd >= (int) (0.02 * outRate))
    {
        double acc = 0;
        const int n0 = std::max (0, nEnd - (int) (0.5 * outRate));
        for (int c = 0; c < ch; ++c) for (int i = n0; i < nEnd; ++i) acc += (double) a.getSample (c, i) * a.getSample (c, i);
        it.noiseDb = (float) juce::jmin (-0.1, 10.0 * std::log10 (acc / ((nEnd - n0) * ch) + 1e-20) - 20.0 * std::log10 (pk));
    }
    const int pre = (int) (o.preRollMs * 0.001 * outRate);
    const int start = std::max (0, std::min (first, onset - pre));
    int end = std::min (n, last + (int) (0.02 * outRate));
    const int cap = (int) (o.maxSeconds * outRate);
    bool capped = false;
    if (end - start > cap) { end = start + cap; capped = true; }
    juce::AudioBuffer<float> t (ch, end - start);
    for (int c = 0; c < ch; ++c) t.copyFrom (c, 0, a, c, start, end - start);
    // fade-in over the pre-roll (raised cosine), fade-out at the end
    const int fin = std::max ((int) (0.0015 * outRate), onset - start);   // at least 1.5 ms: files that start mid-sound do not click
    const int fout = capped ? (int) (o.tailFadeMs * 0.001 * outRate) : (int) (0.02 * outRate);
    for (int c = 0; c < ch; ++c)
    {
        float* x = t.getWritePointer (c);
        for (int i = 0; i < std::min (fin, t.getNumSamples()); ++i) x[i] *= (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * i / fin));
        const int nn = t.getNumSamples();
        for (int i = 0; i < std::min (fout, nn); ++i) x[nn - 1 - i] *= (float) (0.5 - 0.5 * std::cos (juce::MathConstants<double>::pi * i / fout));
    }
    if (capped) it.msg << "capped at " << juce::String (o.maxSeconds, 1) << " s; ";
    it.audio = std::move (t);
    return true;
}

void analyseItem (Item& it, const Opts& o)
{
    it.loud = loudnessDb (it.audio);
    if (it.release) return;
    const double fExp = 440.0 * std::pow (2.0, (std::max (0, it.namedNote) - 69) / 12.0);
    const int n = it.audio.getNumSamples();
    int from = (int) (o.pitchFrom * outRate), len = (int) ((o.pitchFrom > 0.2 ? 1.0 : 0.35) * outRate);
    if (o.loop && ! o.pitchFromSet) { from = (int) (0.35 * outRate); len = std::max ((int) (0.5 * outRate), n - from - (int) (0.4 * outRate)); }
    if (o.pitchSpan > 0) len = (int) (o.pitchSpan * outRate);
    len = std::min (len, n - from);
    it.anaFrom = from; it.anaLen = len;
    if (it.detect)
    {
        // pitch from the spectrum: 80 ms after the start (past the strike) for up to 0.85 s, or the stable sustain
        const int f2 = o.loop ? from : (int) (0.08 * outRate), l2 = o.loop ? len : std::min (n - f2, (int) (0.85 * outRate));
        const double m = harmonicSumMidi (it.audio, f2, l2, it.detLo, it.detHi, it.partials);
        it.f0 = m > 0 ? (float) (440.0 * std::pow (2.0, (m - 69.0) / 12.0)) : 0.0f;
        it.aperiodicity = 0;
        it.pitchOk = it.f0 > 0;
        if (it.pitchOk) it.namedNote = (int) std::lround (m);
    }
    else
    {
        float ap = 1;
        it.f0 = len > 1024 ? detectF0 (it.audio, from, len, fExp, ap, o.densePitch) : 0.0f;
        it.aperiodicity = ap;
        it.pitchOk = it.f0 > 0 && ap < 0.35f;
        if (! it.pitchOk && ! o.loop && from > (int) (0.1 * outRate))
        {
            // short-lived notes (piano top octave, high plucks) have decayed by pitchFrom: measure right after the attack
            const int f2 = (int) (0.08 * outRate), l2 = std::min (n - f2, (int) (0.4 * outRate));
            float ap2 = 1;
            const float f = l2 > 1024 ? detectF0 (it.audio, f2, l2, fExp, ap2, o.densePitch) : 0.0f;
            if (f > 0 && ap2 < 0.35f) { it.f0 = f; it.aperiodicity = ap2; it.pitchOk = true; it.anaFrom = f2; it.anaLen = l2; }
        }
    }
    if (o.loop && ! findLoop (it, o)) it.msg << "NO LOOP FOUND (plays to the end); ";
    else if (o.loop && ! o.pitchFromSet && ! it.detect && it.pitchOk && it.loopEnd - it.loopStart > 4096)
    {
        // a held note plays the sustain up to the loop end, then the loop: tune to that stretch (what is heard), not to
        // the tail after the loop end. Not when the map sets "pitchFrom" (voices: the start the ear hears first is tuned)
        const int a0 = std::min ((int) (0.35 * outRate), it.loopStart);
        float ap2 = 1;
        const float fl = detectF0 (it.audio, a0, it.loopEnd - a0, it.f0, ap2);
        if (fl > 0 && ap2 < 0.35f && std::abs (1200.0 * std::log2 (fl / it.f0)) < 50.0)
        {
            it.f0 = fl; it.aperiodicity = ap2;
            it.anaFrom = a0; it.anaLen = it.loopEnd - a0;
        }
    }
}
} // namespace

//==============================================================================
// -qc: measure the BUILT library the way the plugin loads it (vault::SampleLibrary): per zone the tuning (YIN, after the
// zone's fine tune), the start (first-frame level, onset jump), the loop wrap (sample jump vs the music around it, level dip
// in the crossfade); per articulation the velocity-layer order, key gaps and stretch; per instrument disk + RAM size.
// -demo <dir>: a 2 s note at C5 (MIDI 60, FL Studio naming) of every instrument's first articulation, velocity 100.
// Writes <out>/qc_report.txt and <out>/qc.json.
namespace qc
{
struct Stats { float peak = 0, rms = 0, maxJump = 0; };
Stats stats (const juce::AudioBuffer<float>& b, int from, int to)
{
    Stats s; double acc = 0; int n = 0;
    from = std::max (1, from); to = std::min (b.getNumSamples(), to);
    for (int c = 0; c < b.getNumChannels(); ++c)
    {
        const float* x = b.getReadPointer (c);
        for (int i = from; i < to; ++i) { s.peak = std::max (s.peak, std::abs (x[i])); s.maxJump = std::max (s.maxJump, std::abs (x[i] - x[i - 1])); acc += (double) x[i] * x[i]; ++n; }
    }
    s.rms = (float) std::sqrt (acc / std::max (1, n));
    return s;
}
// 2-5 kHz share of the energy and the fundamental vs the strongest of harmonics 2..16 (dB), Hann window, 32k FFT
float bandShare (const juce::AudioBuffer<float>& b, int from, int len, double lo, double hi, double f0, float& fundVsStrongestDb)
{
    const int order = 15, n = 1 << order;
    juce::dsp::FFT fft (order);
    std::vector<float> data ((size_t) n * 2, 0.0f);
    len = std::min ({ len, n, b.getNumSamples() - from });
    for (int i = 0; i < len; ++i)
    {
        float s = 0; for (int c = 0; c < b.getNumChannels(); ++c) s += b.getSample (c, from + i);
        data[(size_t) i] = s * (0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) len));
    }
    fft.performFrequencyOnlyForwardTransform (data.data());
    double tot = 0, band = 0;
    for (int k = 1; k < n / 2; ++k) { const double f = k * outRate / n, p = (double) data[(size_t) k] * data[(size_t) k]; tot += p; if (f >= lo && f < hi) band += p; }
    auto peakNear = [&] (double hz) { float m = 0; const int k0 = (int) (hz * 0.97 * n / outRate), k1 = (int) (hz * 1.03 * n / outRate) + 1;
                                      for (int k = std::max (1, k0); k <= std::min (n / 2 - 1, k1); ++k) m = std::max (m, data[(size_t) k]); return m; };
    const float fund = peakNear (f0);
    float strongest = 1e-12f;
    for (int h = 2; h <= 16 && h * f0 < outRate / 2; ++h) strongest = std::max (strongest, peakNear (h * f0));
    fundVsStrongestDb = juce::Decibels::gainToDecibels (fund + 1e-9f) - juce::Decibels::gainToDecibels (strongest + 1e-9f);
    return (float) (band / std::max (1e-20, tot));
}
void writeWav (const juce::AudioBuffer<float>& b, const juce::File& f)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
    if (os == nullptr) return;
    if (auto w = wav.createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (outRate).withNumChannels (b.getNumChannels()).withBitsPerSample (24)))
        w->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
}
juce::String fl (int midi)   // FL Studio note name: MIDI 60 = C5
{
    static const char* n[12] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    return juce::String (n[((midi % 12) + 12) % 12]) + juce::String (midi / 12);
}
} // namespace qc

float meanCentsNow (float sumAbs, int n) { return n > 0 ? sumAbs / (float) n : 0.0f; }

int runQc (const juce::File& outDir, const juce::StringArray& onlyIds, const juce::File& demoDir)
{
    auto lib = std::make_shared<vault::SampleLibrary> (outDir);
    if (! lib->libraryFound()) { std::printf ("no library at %s: %s\n", outDir.getFullPathName().toRawUTF8(), lib->errorMessage().toRawUTF8()); return 1; }
    lib->setMemoryBudget ((int64_t) 6 * 1024 * 1024 * 1024);
    const auto json = juce::JSON::parse (outDir.getChildFile ("library.json"));
    if (demoDir != juce::File()) demoDir.createDirectory();
    juce::String rep;
    auto out = [&] (const juce::String& s) { std::printf ("%s\n", s.toRawUTF8()); std::fflush (stdout); rep << s << "\n"; };
    juce::Array<juce::var> qInsts;
    int64_t totalDisk = 0, totalRam = 0;
    int nFlags = 0;
    for (int ii = 0; ii < lib->numInstruments(); ++ii)
    {
        auto* h = lib->instrument (ii);
        const auto& d = h->desc;
        if (! onlyIds.isEmpty() && ! onlyIds.contains (d.id)) continue;
        juce::var ij;
        for (auto& v : *json["instruments"].getArray()) if (v["id"].toString() == d.id) ij = v;
        const bool loaded = lib->loadNow (h);
        const auto* data = h->data();
        int64_t disk = 0;
        for (auto& f : outDir.getChildFile ("audio").getChildFile (d.id).findChildFiles (juce::File::findFiles, false)) disk += f.getSize();
        totalDisk += disk;
        const int64_t ram = data != nullptr ? (int64_t) data->bytes : 0;
        totalRam += ram;
        out ("== " + d.id + " \"" + d.name + "\" (" + d.family + ")  " + (loaded ? "loads" : "LOAD FAILED") + ", disk " + juce::String (disk / 1048576.0, 1)
             + " MB, RAM " + juce::String (ram / 1048576.0, 1) + " MB");
        auto* qi = new juce::DynamicObject();
        qi->setProperty ("id", d.id); qi->setProperty ("name", d.name); qi->setProperty ("family", d.family);
        qi->setProperty ("diskBytes", (juce::int64) disk); qi->setProperty ("ramBytes", (juce::int64) ram); qi->setProperty ("loads", loaded);
        juce::Array<juce::var> qArts;
        if (data == nullptr) { ++nFlags; qi->setProperty ("articulations", qArts); qInsts.add (juce::var (qi)); continue; }
        for (size_t ai = 0; ai < d.articulations.size() && ai < data->articulations.size(); ++ai)
        {
            const auto& ad = d.articulations[ai];
            const auto& A = data->articulations[ai];
            juce::var aj = ij["articulations"][(int) ai];
            const juce::String mode = aj.hasProperty ("pitchCheck") ? aj["pitchCheck"].toString() : "yin";
            const double pitchFrom = aj.hasProperty ("pitchFrom") ? (double) aj["pitchFrom"] : 0.06;
            std::vector<const vault::SampleZone*> dz;
            std::vector<juce::var> zj;
            int zi = 0;
            for (auto& z : ad.zones)
            {
                if (! z.releaseTrigger) { dz.push_back (&z); zj.push_back (aj["zones"][zi]); }
                ++zi;
            }
            float worstCents = 0, sumAbs = 0; int nPitch = 0, nOct = 0, nSpectrum = 0, nOver25 = 0; juce::String worstZone;
            float worstStart = 0, worstLoop = 0, worstDip = 0; int nLoops = 0;
            float worstNoise = -200; int nNoise = 0; float sumNoise = 0;
            std::map<int, std::vector<std::pair<int, float>>> layerLoud;   // root -> (vlo, loudness) of rr 0
            for (size_t j = 0; j < A.zones.size() && j < dz.size(); ++j)
            {
                const auto& z = A.zones[j];
                const auto* au = z.audio;
                if (au == nullptr || au->frames < 64) continue;
                juce::AudioBuffer<float> b (au->channels, au->frames);
                for (int c = 0; c < au->channels; ++c)
                {
                    const int16_t* src = au->channel (c);
                    float* dst = b.getWritePointer (c);
                    for (int i = 0; i < au->frames; ++i) dst[i] = (float) src[i] * (1.0f / 32768.0f) * z.gain;
                }
                const auto file = dz[j]->file.fromLastOccurrenceOf ("/", false, false);
                const float pk = std::max (1e-6f, b.getMagnitude (0, b.getNumSamples()));
                // tuning
                juce::String pitchTxt = mode == "yin" ? "n/a (below E1)" : "n/a (" + mode + ")";
                if (mode == "spectrum")
                {
                    // struck bars / tines: the fundamental's spectral peak, as the builder measured it
                    const int n = b.getNumSamples();
                    const int from = std::min ((int) (pitchFrom * outRate), n / 2), len = std::min (n - from, (int) (0.35 * outRate));
                    const double hs = len > 2048 ? harmonicSumMidi (b, from, len, z.root - 0.75, z.root + 0.75, { 1.0 }) : 0.0;
                    if (hs > 0)
                    {
                        const float c = (float) ((hs - z.root) * 100.0 + z.fine);
                        pitchTxt = juce::String (c, 1) + " c (spectrum)";
                        sumAbs += std::abs (c); ++nPitch; if (std::abs (c) > 25.0f) ++nOver25;
                        if (std::abs (c) > std::abs (worstCents)) { worstCents = c; worstZone = file; }
                    }
                    else { pitchTxt = "unclear"; ++nOct; }
                }
                if (mode == "yin" && z.root >= 28)   // below E1 (piano / organ bottom) neither YIN nor the spectrum is reliable
                {
                    const int n = b.getNumSamples();
                    int from = (int) (pitchFrom * outRate), len = (int) ((pitchFrom > 0.2 ? 1.0 : 0.35) * outRate);
                    if (z.looped) { from = std::min ((int) (0.35 * outRate), z.loopStart); len = z.loopEnd - from; }   // as the builder: sustain up to the loop end
                    len = std::min (len, n - from);
                    float ap = 1;
                    const double fExp = 440.0 * std::pow (2.0, (z.root - 69) / 12.0);
                    float f = len > 1024 ? detectF0 (b, from, len, fExp, ap) : 0.0f;
                    if ((f <= 0 || ap >= 0.35f) && ! z.looped && from > (int) (0.1 * outRate))
                    {
                        // decayed by pitchFrom (piano top octave, high plucks): right after the attack
                        from = (int) (0.08 * outRate); len = std::min (n - from, (int) (0.4 * outRate));
                        f = len > 1024 ? detectF0 (b, from, len, fExp, ap) : 0.0f;
                    }
                    // YIN first; where it is unclear or more than half a semitone off, the harmonic-sum spectrum around the
                    // root (as in the builder) - YIN alone mis-hears missing fundamentals and bright trebles. A spectrum
                    // more than a semitone off with no YIN to back it counts as "no measurable pitch".
                    double m = f > 0 && ap < 0.35f ? 12.0 * std::log2 (f / 440.0) + 69.0 : -1000.0;
                    juce::String how = "yin";
                    if (len > 2048 && std::abs (m - z.root) > 0.5)
                    {
                        const double hs = harmonicSumMidi (b, from, len, z.root - 2.5, z.root + 2.5, { 1.0, 2.0, 3.0, 4.0, 5.0 });
                        if (hs > 0 && (std::abs (hs - z.root) <= 1.0 || std::abs (hs - m) < 0.6)) { m = hs; how = "spectrum"; ++nSpectrum; }
                        else if (std::abs (m - z.root) > 1.0) m = -1000.0;
                    }
                    if (m > -100.0)
                    {
                        const float c = (float) ((m - z.root) * 100.0 + z.fine);
                        pitchTxt = juce::String (c, 1) + " c" + (how == "yin" ? "" : " (spectrum)");
                        sumAbs += std::abs (c); ++nPitch; if (std::abs (c) > 25.0f) ++nOver25;
                        if (std::abs (c) > std::abs (worstCents)) { worstCents = c; worstZone = file; }
                    }
                    else { pitchTxt = "unclear (ap " + juce::String (ap, 2) + ")"; ++nOct; }
                }
                // start: first frame level vs the peak, the biggest jump in the first 3 ms vs the next 50 ms
                const float first = std::abs (b.getSample (0, 0)) / pk;
                const float j0 = qc::stats (b, 1, (int) (0.003 * outRate)).maxJump, j1 = qc::stats (b, (int) (0.003 * outRate), (int) (0.053 * outRate)).maxJump;
                const float startRatio = j0 / std::max (1e-6f, j1);
                worstStart = std::max (worstStart, startRatio);
                // loop: rebuild what the sampler plays around the wrap (linear crossfade over xfade frames before loopEnd)
                juce::String loopTxt;
                if (z.looped && z.xfade > 0)
                {
                    const int ls = z.loopStart, le = z.loopEnd, xf = z.xfade, L = le - ls;
                    const int pre = std::min ((int) (0.25 * outRate), le - xf - 1), post = std::min ((int) (0.25 * outRate), L - xf - 1);
                    juce::AudioBuffer<float> y (b.getNumChannels(), pre + xf + post);
                    for (int c = 0; c < b.getNumChannels(); ++c)
                    {
                        const float* x = b.getReadPointer (c);
                        float* o = y.getWritePointer (c);
                        int k = 0;
                        for (int p = le - xf - pre; p < le - xf; ++p) o[k++] = x[p];
                        for (int p = le - xf; p < le; ++p) { const float t = (float) (p - (le - xf)) / (float) xf; o[k++] = x[p] + t * (x[p - L] - x[p]); }
                        for (int p = ls; p < ls + post; ++p) o[k++] = x[p];
                    }
                    const int w = pre + xf, ms = (int) (0.001 * outRate);
                    const float at = qc::stats (y, w - 2 * ms, w + 2 * ms).maxJump;
                    const float around = std::max (qc::stats (y, w - 200 * ms, w - 10 * ms).maxJump, qc::stats (y, w + 10 * ms, w + 200 * ms).maxJump);
                    const float ratio = at / std::max (1e-6f, around);
                    // level dip: the quietest 20 ms in the crossfade vs the 50 ms before it, minus the dip the music has on its
                    // own (the quietest 20 ms in the 250 ms before the crossfade: tremolo, vibrato and bow changes breathe)
                    const float before = qc::stats (y, pre - 50 * ms, pre).rms;
                    float minRms = 1e9f, natural = 1e9f;
                    for (int s0 = pre; s0 + 20 * ms <= pre + xf; s0 += 5 * ms) minRms = std::min (minRms, qc::stats (y, s0, s0 + 20 * ms).rms);
                    for (int s0 = std::max (1, pre - 250 * ms); s0 + 20 * ms <= pre; s0 += 5 * ms) natural = std::min (natural, qc::stats (y, s0, s0 + 20 * ms).rms);
                    // slow swings (rubbed glass beats, bow changes) need a longer look back: up to 600 ms in the original
                    for (int s0 = std::max (1, le - xf - (int) (0.6 * outRate)); s0 + 20 * ms <= le - xf; s0 += 10 * ms)
                    {
                        double acc = 0; int cnt = 0;
                        for (int c = 0; c < b.getNumChannels(); ++c) for (int i = s0; i < s0 + 20 * ms; ++i) { acc += (double) b.getSample (c, i) * b.getSample (c, i); ++cnt; }
                        natural = std::min (natural, (float) std::sqrt (acc / std::max (1, cnt)));
                    }
                    const float dipRaw = juce::Decibels::gainToDecibels (std::max (1e-9f, minRms) / std::max (1e-9f, before));
                    const float dipNat = natural < 1e8f ? juce::Decibels::gainToDecibels (std::max (1e-9f, natural) / std::max (1e-9f, before)) : 0.0f;
                    const float dip = std::min (0.0f, dipRaw - std::min (0.0f, dipNat));
                    worstLoop = std::max (worstLoop, ratio); worstDip = std::min (worstDip, dip); ++nLoops;
                    loopTxt = juce::String::formatted ("  loop %.2f s wrap %.2fx dip %+.1f dB (raw %+.1f, the music itself %+.1f)", L / outRate, (double) ratio, (double) dip,
                                                       (double) dipRaw, (double) dipNat);
                }
                // noise floor measured by the builder (lead-in before the onset)
                juce::String noiseTxt;
                if (zj[j].hasProperty ("noise"))
                {
                    const float nd = (float) zj[j]["noise"];
                    worstNoise = std::max (worstNoise, nd); sumNoise += nd; ++nNoise;
                    noiseTxt = juce::String::formatted ("  noise %.0f dB", (double) nd);
                }
                const float loud = loudnessDb (b);
                if (z.rr == 0) layerLoud[z.root].push_back ({ z.vlo, loud });
                rep << juce::String::formatted ("   %-6s %-34s root %-4s fine %+6.1f  vel %3d-%3d rr %d  pitch %-14s  start %.3f/%.2fx  loud %6.1f", ad.name.substring (0, 6).toRawUTF8(),
                                                file.toRawUTF8(), qc::fl (z.root).toRawUTF8(), (double) z.fine, z.vlo, z.vhi, z.rr, pitchTxt.toRawUTF8(),
                                                (double) first, (double) startRatio, (double) loud)
                    << noiseTxt << loopTxt << "\n";
            }
            // velocity layers: louder layer = louder sample at every root that has several layers
            int inversions = 0, cmpNotes = 0; float minStep = 99, maxStep = -99;
            for (auto& [root, v] : layerLoud)
            {
                if (v.size() < 2) continue;
                std::sort (v.begin(), v.end());
                ++cmpNotes;
                for (size_t k = 1; k < v.size(); ++k)
                {
                    const float st = v[k].second - v[k - 1].second;
                    minStep = std::min (minStep, st); maxStep = std::max (maxStep, st);
                    if (st < 0.5f) ++inversions;
                }
            }
            // key coverage per velocity layer
            std::map<int, std::vector<bool>> cover;
            int lowKey = 127, highKey = 0, inner = 0, edge = 0, gaps = 0;
            std::map<int, std::pair<int, int>> layerSpan;
            for (auto* z : dz)
            {
                auto& cv = cover[z->vlo]; cv.resize (128, false);
                for (int k = z->lo; k <= z->hi; ++k) cv[(size_t) k] = true;
                lowKey = std::min (lowKey, z->lo); highKey = std::max (highKey, z->hi);
            }
            std::set<int> rootSet; for (auto* z : dz) rootSet.insert (z->root);
            const int lowRoot = rootSet.empty() ? 0 : *rootSet.begin(), highRoot = rootSet.empty() ? 0 : *rootSet.rbegin();
            for (auto* z : dz)
            {
                const int down = z->root - z->lo, up = z->hi - z->root;
                if (z->root == lowRoot) edge = std::max (edge, down); else inner = std::max (inner, down);
                if (z->root == highRoot) edge = std::max (edge, up); else inner = std::max (inner, up);
            }
            // a key is playable when any layer covers it (the sampler falls back to the nearest velocity layer)
            for (int k = lowKey; k <= highKey; ++k)
            {
                bool any = false;
                for (auto& [vlo, cv] : cover) any |= cv[(size_t) k];
                if (! any) ++gaps;
            }
            const float meanCents = nPitch > 0 ? sumAbs / (float) nPitch : 0.0f;
            juce::StringArray flags;
            // tuning: the mean |error| over the measured zones above 8 cents, or more than 10 % of them beyond 25 cents
            // (single outliers are usually the measurement: plucks glide, bars and tines have stretched partials)
            if (meanCentsNow (sumAbs, nPitch) > 8.0f || nOver25 * 10 > std::max (1, nPitch))
                flags.add ("tuning: mean " + juce::String (meanCentsNow (sumAbs, nPitch), 1) + " c, " + juce::String (nOver25) + " of "
                           + juce::String (nPitch) + " zones > 25 c (worst " + juce::String (worstCents, 1) + " c, " + worstZone + ")");
            if (nOct > 0) flags.add (juce::String (nOct) + " zone(s) without a measurable pitch");
            if (worstStart > 3.0f) flags.add ("start jump " + juce::String (worstStart, 1) + "x");
            if (nLoops > 0 && (worstLoop > 1.5f || worstDip < -6.0f)) flags.add ("loop wrap " + juce::String (worstLoop, 2) + "x dip " + juce::String (worstDip, 1) + " dB");
            if (inversions > 0) flags.add (juce::String (inversions) + " velocity step(s) < 0.5 dB");
            if (gaps > 0) flags.add (juce::String (gaps) + " key gap(s)");
            if (inner > 4) flags.add ("stretch " + juce::String (inner) + " semitones");
            if (nNoise > 0 && worstNoise > -45.0f) flags.add ("noise " + juce::String (worstNoise, 0) + " dB");
            nFlags += flags.size();
            out (juce::String::formatted ("  %-12s %3d zones %2d layers  keys %s-%s (roots %s-%s)  stretch %d/%d  pitch |c| mean %.1f max %.1f  loops %d (wrap %.2fx, dip %.1f dB)  vel steps %.1f..%.1f dB  noise %s",
                                          ad.name.toRawUTF8(), (int) dz.size(), (int) cover.size(), qc::fl (lowKey).toRawUTF8(), qc::fl (highKey).toRawUTF8(),
                                          qc::fl (lowRoot).toRawUTF8(), qc::fl (highRoot).toRawUTF8(), inner, edge, (double) meanCents, (double) std::abs (worstCents), nLoops,
                                          (double) worstLoop, (double) worstDip, cmpNotes > 0 ? (double) minStep : 0.0, cmpNotes > 0 ? (double) maxStep : 0.0,
                                          nNoise > 0 ? (juce::String (juce::roundToInt (sumNoise / nNoise)) + " avg / " + juce::String (juce::roundToInt (worstNoise)) + " worst").toRawUTF8() : "n/a")
                 + (flags.isEmpty() ? juce::String() : "  !! " + flags.joinIntoString ("; ")));
            auto* qa = new juce::DynamicObject();
            qa->setProperty ("name", ad.name); qa->setProperty ("oneShot", ad.oneShot); qa->setProperty ("zones", (int) dz.size());
            qa->setProperty ("releaseZones", (int) A.releaseZones.size());
            qa->setProperty ("layers", (int) cover.size()); qa->setProperty ("lo", lowKey); qa->setProperty ("hi", highKey);
            qa->setProperty ("rootLo", lowRoot); qa->setProperty ("rootHi", highRoot); qa->setProperty ("stretch", inner); qa->setProperty ("edge", edge);
            qa->setProperty ("pitchCheck", mode); qa->setProperty ("centsMean", meanCents); qa->setProperty ("centsMax", std::abs (worstCents));
            qa->setProperty ("pitchOver25", nOver25); qa->setProperty ("pitchBySpectrum", nSpectrum); qa->setProperty ("pitchUnclear", nOct); qa->setProperty ("pitchMeasured", nPitch);
            qa->setProperty ("loops", nLoops); qa->setProperty ("loopWrap", worstLoop); qa->setProperty ("loopDip", worstDip);
            qa->setProperty ("velMinStep", cmpNotes > 0 ? minStep : 0.0f); qa->setProperty ("velInversions", inversions);
            qa->setProperty ("gaps", gaps); qa->setProperty ("startJump", worstStart);
            qa->setProperty ("noiseAvg", nNoise > 0 ? sumNoise / nNoise : 0.0f); qa->setProperty ("noiseWorst", nNoise > 0 ? worstNoise : 0.0f);
            qa->setProperty ("flags", flags.joinIntoString ("; "));
            qArts.add (juce::var (qa));
        }
        qi->setProperty ("articulations", qArts);
        // demo: 2 s at C5 (or the covered key nearest to it), first articulation, velocity 100, note-off at 1.6 s
        if (demoDir != juce::File() && ! d.articulations.empty())
        {
            int key = vault::homeNote, bestD = 1000;
            for (auto& z : d.articulations[0].zones)
                if (! z.releaseTrigger)
                    for (int k = z.lo; k <= z.hi; ++k) if (std::abs (k - vault::homeNote) < bestD) { bestD = std::abs (k - vault::homeNote); key = k; }
            VaultPreset p;
            p.id = "qc." + d.id; p.name = d.name; p.category = vault::catPiano;
            VaultPart part; part.kind = VaultPartKind::sample; part.source = d.id; part.articulation = d.articulations[0].name; part.label = d.name;
            p.parts.push_back (part);
            juce::MidiBuffer m;
            m.addEvent (juce::MidiMessage::noteOn (1, key, (juce::uint8) 100), 0);
            m.addEvent (juce::MidiMessage::noteOff (1, key), (int) (1.6 * outRate));
            const auto b = VaultEngine::renderMidi (p, m, (int) (2.0 * outRate), outRate, nullptr, lib);
            const auto st = qc::stats (b, 1, b.getNumSamples());
            const double f0 = 440.0 * std::pow (2.0, (key - 69) / 12.0);
            float ap = 1, fundDb = 0;
            const float f = detectF0 (b, (int) (0.15 * outRate), (int) (0.8 * outRate), f0, ap);
            const float share = qc::bandShare (b, (int) (0.1 * outRate), (int) (0.8 * outRate), 2000.0, 5000.0, f0, fundDb);
            qc::writeWav (b, demoDir.getChildFile (d.id + ".wav"));
            const float cents = f > 0 ? (float) (1200.0 * std::log2 (f / f0)) : 0.0f;
            out (juce::String::formatted ("  demo %s: peak %.1f dBFS, loudness %.1f dB, pitch %+.1f c, fundamental vs strongest harmonic %+.1f dB, 2-5 kHz %.1f %%",
                                          qc::fl (key).toRawUTF8(), (double) juce::Decibels::gainToDecibels (st.peak), (double) loudnessDb (b), (double) cents, (double) fundDb,
                                          (double) share * 100.0));
            auto* dj = new juce::DynamicObject();
            dj->setProperty ("key", key); dj->setProperty ("peakDb", juce::Decibels::gainToDecibels (st.peak)); dj->setProperty ("loudDb", loudnessDb (b));
            dj->setProperty ("cents", cents); dj->setProperty ("fundVsStrongestDb", fundDb); dj->setProperty ("presence", share);
            qi->setProperty ("demo", juce::var (dj));
        }
        qInsts.add (juce::var (qi));
        lib->unpin (h);
        lib->setMemoryBudget (1);   // let it go before the next one
        lib->waitUntilIdle (2000);
        lib->setMemoryBudget ((int64_t) 6 * 1024 * 1024 * 1024);
    }
    out (juce::String::formatted ("TOTAL: %d instruments, %.1f MB on disk (audio), %.1f MB RAM decoded, %d flag(s)", qInsts.size(),
                                  totalDisk / 1048576.0, totalRam / 1048576.0, nFlags));
    auto* root = new juce::DynamicObject();
    root->setProperty ("instruments", qInsts);
    root->setProperty ("diskBytes", (juce::int64) totalDisk); root->setProperty ("ramBytes", (juce::int64) totalRam);
    const bool partial = ! onlyIds.isEmpty();
    outDir.getChildFile (partial ? "qc_partial.json" : "qc.json").replaceWithText (juce::JSON::toString (juce::var (root), false));
    outDir.getChildFile (partial ? "qc_partial_report.txt" : "qc_report.txt").replaceWithText (rep);
    return 0;
}

//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;   // message manager (juce::JSON, file APIs)
    juce::String mapPath = "tools/vault/library_map.json", outOverride, only;
    int threads = juce::jlimit (1, 8, (int) std::thread::hardware_concurrency());
    bool dry = false, qcMode = false;
    juce::String demoPath;
    for (int i = 1; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a == "-map" && i + 1 < argc) mapPath = argv[++i];
        else if (a == "-out" && i + 1 < argc) outOverride = argv[++i];
        else if (a == "-only" && i + 1 < argc) only = argv[++i];
        else if (a == "-threads" && i + 1 < argc) threads = juce::jmax (1, juce::String (argv[++i]).getIntValue());
        else if (a == "-dry") dry = true;
        else if (a == "-qc") qcMode = true;
        else if (a == "-demo" && i + 1 < argc) demoPath = argv[++i];
    }
    const auto cwd = juce::File::getCurrentWorkingDirectory();
    const auto mapFile = juce::File::isAbsolutePath (mapPath) ? juce::File (mapPath) : cwd.getChildFile (mapPath);
    const auto map = juce::JSON::parse (mapFile);
    if (! map.isObject()) { std::printf ("cannot read map %s\n", mapFile.getFullPathName().toRawUTF8()); return 1; }
    const juce::File outDir (outOverride.isNotEmpty() ? outOverride : map["output"].toString());
    if (! juce::File::isAbsolutePath (outDir.getFullPathName())) { std::printf ("output must be absolute\n"); return 1; }
    std::map<juce::String, juce::File> roots;
    if (auto* ro = map["roots"].getDynamicObject())
        for (auto& p : ro->getProperties()) roots[p.name.toString()] = juce::File (p.value.toString());
    Opts defaults;
    defaults.read (map["defaults"]);
    const auto onlyIds = juce::StringArray::fromTokens (only, ",", "");
    if (qcMode) return runQc (outDir, onlyIds, demoPath.isNotEmpty() ? juce::File (demoPath) : juce::File());

    // previous library.json: instruments not rebuilt this run are kept, and so are the credit lines of other maps
    // (a second map, e.g. library_map_vocals.json, adds its own "credits" line)
    juce::Array<juce::var> keep;
    juce::StringArray credits { "Built from VSCO 2 Community Edition (Versilian Studios) and VCSL (Versilian Community Sample Library), both CC0 1.0." };
    if (const auto old = juce::JSON::parse (outDir.getChildFile ("library.json")); old.isObject())
    {
        if (auto* arr = old["instruments"].getArray())
            for (auto& v : *arr) keep.add (v);
        credits.addTokens (old["credits"].toString(), "\n", "");
    }
    // a map's own line ("Vocals: ...") replaces the line with the same "<label>:" from an earlier build
    for (auto line : juce::StringArray::fromTokens (map["credits"].toString(), "\n", ""))
    {
        if ((line = line.trim()).isEmpty()) continue;
        if (line.containsChar (':'))
            for (int k = credits.size(); --k >= 1;)
                if (credits[k].upToFirstOccurrenceOf (":", true, false) == line.upToFirstOccurrenceOf (":", true, false)) credits.remove (k);
        credits.add (line);
    }
    credits.trim(); credits.removeEmptyStrings(); credits.removeDuplicates (false);
    const auto reportName = map.hasProperty ("report") ? map["report"].toString() : juce::String ("build_report.txt");

    std::vector<Instrument> insts;
    for (auto& iv : *map["instruments"].getArray())
    {
        Instrument in;
        in.id = iv["id"].toString(); in.name = iv["name"].toString(); in.family = iv["family"].toString(); in.credit = iv["credit"].toString();
        if (! onlyIds.isEmpty() && ! onlyIds.contains (in.id)) continue;
        for (auto& av : *iv["articulations"].getArray())
        {
            Articulation art;
            art.name = av["name"].toString();
            art.o = defaults;
            art.o.read (av);
            if (auto* ld = av["layerDb"].getDynamicObject())
                for (auto& p : ld->getProperties()) art.layerDb[p.name.toString()] = (double) p.value;
            if (auto* vo = av["velOrder"].getArray()) for (auto& v : *vo) art.velOrder.add (v.toString());
            for (auto& sv : *av["sources"].getArray())
            {
                const auto spec = sv["files"].toString();
                const auto rootKey = spec.upToFirstOccurrenceOf (":", false, false);
                const auto rel = spec.fromFirstOccurrenceOf (":", false, false);
                if (roots.count (rootKey) == 0) { log ("!! unknown root " + rootKey); return 1; }
                const auto dir = roots[rootKey].getChildFile (rel.upToLastOccurrenceOf ("/", false, false));
                const auto glob = rel.fromLastOccurrenceOf ("/", false, false);
                const auto rx = makeRegex (sv["pattern"].toString());
                const bool isRel = sv["trigger"].toString() == "release";
                const double levelDb = sv.hasProperty ("levelDb") ? (double) sv["levelDb"] : -22.0;
                const juce::String fixedVel = sv.hasProperty ("vel") ? sv["vel"].toString() : juce::String();   // a fixed layer name
                const int octaveShift = sv.hasProperty ("octave") ? (int) sv["octave"] : 0;                    // names this many octaves off
                juce::StringArray exclude;   // file-name wildcards left out (broken takes, unclear pitch)
                if (auto* ex = sv["exclude"].getArray()) for (auto& v : *ex) exclude.add (v.toString());
                const bool detect = sv["pitch"].toString() == "detect";
                double detLo = 36, detHi = 84;
                if (auto* rg = sv["range"].getArray(); rg != nullptr && rg->size() == 2) { detLo = (double) (*rg)[0]; detHi = (double) (*rg)[1]; }
                std::vector<double> partials { 1.0, 2.0, 3.0, 4.0 };
                if (auto* pa = sv["partials"].getArray(); pa != nullptr && ! pa->isEmpty()) { partials.clear(); for (auto& v : *pa) partials.push_back ((double) v); }
                auto files = dir.findChildFiles (juce::File::findFiles, false, glob);
                files.sort();
                int skipped = 0;
                for (auto& f : files)
                {
                    std::smatch m;
                    const std::string name = f.getFileName().toStdString();
                    if (! std::regex_match (name, m, rx)) { ++skipped; continue; }
                    bool excluded = false;
                    for (auto& e : exclude) excluded |= f.getFileName().matchesWildcard (e, true);
                    if (excluded) { ++skipped; log ("   excluded by the map: " + f.getFileName()); continue; }
                    Item it;
                    it.src = f; it.release = isRel; it.levelDb = levelDb;
                    it.detect = detect; it.detLo = detLo; it.detHi = detHi; it.partials = partials;
                    // groups in pattern order: {note} {vel} {rr} (each optional)
                    const auto pat = sv["pattern"].toString();
                    std::vector<std::pair<int, juce::String>> order;
                    for (auto* key : { "{note}", "{vel}", "{rr}", "{group}" }) if (pat.contains (key)) order.push_back ({ pat.indexOf (key), key });
                    std::sort (order.begin(), order.end());
                    for (size_t g = 0; g < order.size(); ++g)
                    {
                        const juce::String val (m[(size_t) g + 1].str());
                        if (order[g].second == "{note}") it.noteName = val;
                        else if (order[g].second == "{vel}") it.velName = val;
                        else if (order[g].second == "{group}") it.group = val;
                        else it.rr = std::max (0, val.getIntValue() - 1);
                    }
                    if (fixedVel.isNotEmpty()) it.velName = fixedVel;
                    it.namedNote = parseNote (it.noteName);
                    if (it.namedNote >= 0) it.namedNote += 12 * octaveShift;
                    it.octaveFixed = sv.hasProperty ("octave");
                    if (it.namedNote < 0 && ! it.detect) { ++skipped; continue; }
                    art.items.push_back (std::move (it));
                }
                log (in.id + " / " + art.name + ": " + juce::String (files.size() - skipped) + " files from " + dir.getFullPathName()
                     + (skipped > 0 ? " (" + juce::String (skipped) + " not matching the pattern)" : juce::String()) + (isRel ? " [release]" : ""));
            }
            in.arts.push_back (std::move (art));
        }
        insts.push_back (std::move (in));
    }
    if (dry) return 0;

    juce::Array<juce::var> instJson;
    for (auto& in : insts)
    {
        const auto audioDir = outDir.getChildFile ("audio").getChildFile (in.id);
        audioDir.deleteRecursively();
        audioDir.createDirectory();
        double instPeak = 0;
        // pass 1 (parallel): load + analyse
        for (auto& art : in.arts)
        {
            parallelFor ((int) art.items.size(), threads, [&] (int i)
            {
                auto& it = art.items[(size_t) i];
                if (loadItem (it, art.o)) analyseItem (it, art.o);
            });
            art.items.erase (std::remove_if (art.items.begin(), art.items.end(), [] (const Item& it)
                             { if (it.audio.getNumSamples() == 0) log ("!! skipped " + it.src.getFileName() + ": " + it.msg); return it.audio.getNumSamples() == 0; }),
                             art.items.end());

            // detected pitches: files of one {group} (one drum tuning, one bar) share the group's median pitch
            {
                std::map<juce::String, std::vector<float>> groups;
                for (auto& it : art.items) if (it.detect && it.pitchOk && it.group.isNotEmpty()) groups[it.group].push_back (it.f0);
                for (auto& it : art.items)
                    if (it.detect && it.group.isNotEmpty() && groups.count (it.group) != 0)
                    {
                        auto v = groups[it.group];
                        std::sort (v.begin(), v.end());
                        it.f0 = v[v.size() / 2]; it.pitchOk = true;
                        it.namedNote = (int) std::lround (12.0 * std::log2 (it.f0 / 440.0) + 69.0);
                        it.noteName = "group " + it.group;
                    }
                juce::StringArray lost;
                for (auto& it : art.items) if (it.detect && ! it.release && ! it.pitchOk) lost.add (it.src.getFileName());
                art.items.erase (std::remove_if (art.items.begin(), art.items.end(), [] (const Item& it) { return it.detect && ! it.release && ! it.pitchOk; }),
                                 art.items.end());
                for (auto& f : lost) log ("!! dropped " + f + ": no pitch found");
                for (auto& it : art.items) if (it.detect && it.noteName.isEmpty()) it.noteName = "detected";
            }
            // pitch: the naming convention offset (octaves) from the median, then per file
            std::vector<int> offs;
            for (auto& it : art.items)
                if (! it.release && ! it.detect && ! it.octaveFixed && ! art.o.trustNames && it.pitchOk && it.namedNote >= 36)
                    offs.push_back ((int) std::lround (12.0 * std::log2 (it.f0 / 440.0) + 69.0) - it.namedNote);
            int octOff = 0;
            if (! offs.empty())
            {
                std::sort (offs.begin(), offs.end());
                octOff = 12 * (int) std::lround (offs[offs.size() / 2] / 12.0);
            }
            if (octOff != 0) log (in.id + " / " + art.name + ": file names are " + juce::String (octOff / 12) + " octave(s) off (corrected)");
            for (auto& it : art.items)
            {
                it.root = it.namedNote + (it.detect || it.octaveFixed ? 0 : octOff);
                if (it.release) continue;
                if (art.o.trustNames)
                {
                    if (it.pitchOk) it.msg << "named pitch kept (YIN " << juce::String (12.0 * std::log2 (it.f0 / 440.0) + 69.0, 2) << "); ";
                    it.cents = 0.0f;
                }
                else if (it.detect)
                {
                    it.cents = (float) ((12.0 * std::log2 (it.f0 / 440.0) + 69.0 - it.root) * 100.0);
                    it.msg << "pitch detected (" << juce::String (12.0 * std::log2 (it.f0 / 440.0) + 69.0, 2) << "); ";
                }
                else if (it.pitchOk)
                {
                    const double detected = 12.0 * std::log2 (it.f0 / 440.0) + 69.0;
                    const int near = (int) std::lround (detected);
                    double measured = detected;
                    if (near != it.root && it.root >= 24)
                    {
                        // YIN disagrees with the name: a second opinion before anything moves (pitchVerdict)
                        const auto v = pitchVerdict (it.audio, it.anaFrom, it.anaLen, it.root, detected);
                        log ("!! " + it.src.getFileName() + ": named " + it.noteName + " (" + juce::String (it.root) + ") but YIN hears MIDI "
                             + juce::String (detected, 2) + ", the spectrum " + juce::String (v.spectrum, 2) + " -> " + v.verdict);
                        it.root = v.root;
                        measured = v.measured;
                    }
                    // struck bars / tines (pitchMethod "spectrum"): YIN is pulled by the inharmonic upper partials; the
                    // fundamental's spectral peak (+-0.6 semitone around the root) gives the pitch
                    if (art.o.spectrumPitch)
                    {
                        const double hs = harmonicSumMidi (it.audio, it.anaFrom, it.anaLen, it.root - 0.75, it.root + 0.75, { 1.0 });
                        if (hs > 0)
                        {
                            // traditional tunings (mbira, kalimba keys) sit between semitones: map to the nearest one
                            if ((int) std::lround (hs) != it.root)
                            {
                                log ("   " + it.src.getFileName() + ": fundamental at MIDI " + juce::String (hs, 2) + " -> root " + juce::String ((int) std::lround (hs)));
                                it.root = (int) std::lround (hs);
                            }
                            measured = hs;
                        }
                    }
                    it.cents = (float) ((measured - it.root) * 100.0);
                    if (std::abs (it.cents) > 50.0f) it.cents = 0.0f;
                    if (art.o.keepTuning && std::abs (it.cents) <= 30.0f) { it.msg << "tuning kept (" << juce::String (it.cents, 1) << " c); "; it.cents = 0.0f; }
                }
                else if (art.o.spectrumPitch && it.anaLen > 2048)
                {
                    // no YIN pitch (tines, buzzing bars): the fundamental's spectral peak alone
                    const double hs = harmonicSumMidi (it.audio, it.anaFrom, it.anaLen, it.root - 0.75, it.root + 0.75, { 1.0 });
                    if (hs > 0)
                    {
                        if ((int) std::lround (hs) != it.root)
                            log ("   " + it.src.getFileName() + ": fundamental at MIDI " + juce::String (hs, 2) + " -> root " + juce::String ((int) std::lround (hs)));
                        it.root = (int) std::lround (hs);
                        it.cents = (float) ((hs - it.root) * 100.0);
                        it.msg << "pitch from the spectrum (no YIN pitch); ";
                    }
                    else it.msg << "pitch not verified (aperiodicity " << juce::String (it.aperiodicity, 2) << "); ";
                }
                else it.msg << "pitch not verified (aperiodicity " << juce::String (it.aperiodicity, 2) << "); ";
            }

            // round robins: every key / layer cell numbers its takes 0..n-1 (corrected roots, several strings or keys of one
            // pitch, sources without {rr} can otherwise collide - and the output file names must stay unique)
            {
                std::map<juce::String, std::vector<Item*>> cells;
                for (auto& it : art.items)
                    cells[juce::String (it.release ? "r" : "a") + juce::String (it.root) + "|" + it.velName].push_back (&it);
                for (auto& [key, list] : cells)
                {
                    std::stable_sort (list.begin(), list.end(), [] (const Item* a, const Item* b)
                                      { return a->rr != b->rr ? a->rr < b->rr : a->src.getFileName() < b->src.getFileName(); });
                    for (size_t k = 0; k < list.size(); ++k) list[k]->rr = (int) k;
                }
            }

            // velocity layers: global order
            juce::StringArray layers;
            for (auto& it : art.items) if (! it.release && ! layers.contains (it.velName)) layers.add (it.velName);
            if (! art.velOrder.isEmpty())
            {
                juce::StringArray ordered;
                for (auto& v : art.velOrder) if (layers.contains (v)) ordered.add (v);
                for (auto& v : layers) if (! ordered.contains (v)) ordered.add (v);
                layers = ordered;
            }
            else
            {
                std::sort (layers.begin(), layers.end(), [] (const juce::String& a, const juce::String& b)
                           { return a.getIntValue() != b.getIntValue() ? a.getIntValue() < b.getIntValue() : a < b; });
            }
            const int nl = std::max (1, layers.size());
            for (auto& it : art.items) it.layer = std::max (0, layers.indexOf (it.velName));
            // per layer, smoothed loudness per register: median over +-7 semitones, widened until it holds 3 takes
            auto curve = [&] (int layer, int note)
            {
                std::vector<float> v;
                for (int range = 7; range <= 128 && v.size() < 3; range += 5)
                {
                    v.clear();
                    for (auto& it : art.items) if (! it.release && it.layer == layer && std::abs (it.root - note) <= range) v.push_back (it.loud);
                }
                if (v.empty()) return 0.0f;
                std::sort (v.begin(), v.end());
                return v.size() % 2 == 1 ? v[v.size() / 2] : 0.5f * (v[v.size() / 2 - 1] + v[v.size() / 2]);
            };
            const int top = nl - 1;
            std::vector<double> D ((size_t) nl, 0.0);
            for (int l = 0; l < nl; ++l)
            {
                if (art.layerDb.count (layers[l]) != 0 && art.layerDb.count (layers[top]) != 0) D[(size_t) l] = art.layerDb[layers[l]] - art.layerDb[layers[top]];
                else
                {
                    std::vector<float> d;
                    for (auto& it : art.items) if (! it.release && it.layer == l) d.push_back (curve (l, it.root) - curve (top, it.root));
                    std::sort (d.begin(), d.end());
                    D[(size_t) l] = d.empty() ? 0.0 : d[d.size() / 2];
                }
            }
            // monotonic, at least 2.5 dB apart, the top layer at 0, the whole range at most 18 dB (soft notes stay usable)
            D[(size_t) top] = 0;
            for (int l = top - 1; l >= 0; --l) D[(size_t) l] = std::min (D[(size_t) l], D[(size_t) l + 1] - 2.5);
            if (D[0] < -18.0) { const double k = 18.0 / -D[0]; for (auto& d : D) d *= k; }
            juce::String ds; for (int l = 0; l < nl; ++l) ds << layers[l] << "=" << juce::String (D[(size_t) l], 1) << " dB  ";
            log (in.id + " / " + art.name + ": velocity layers " + ds);
            for (auto& it : art.items)
            {
                if (it.release) continue;
                const double target = curve (top, it.root) + D[(size_t) it.layer];
                it.gainDb = target - it.loud;
            }
            // a take more than 12 dB away from its own layer's register curve is broken (near-silent / clipped / wrong):
            // leave it out, its neighbours cover the keys
            // the deviation: against the top layer of the same note (relative to the typical layer step), else against the
            // layer's register curve; the top layer itself only against its curve (18 dB: registers differ a lot)
            std::map<int, float> topAt;
            for (auto& it : art.items) if (! it.release && it.layer == top) topAt[it.root] = it.loud;
            std::vector<std::vector<float>> steps ((size_t) nl);
            for (auto& it : art.items) if (! it.release && it.layer != top && topAt.count (it.root)) steps[(size_t) it.layer].push_back (it.loud - topAt[it.root]);
            for (auto& st : steps) std::sort (st.begin(), st.end());
            std::vector<float> dev;
            for (auto& it : art.items)
            {
                float d = 0.0f;
                if (! it.release)
                {
                    const auto& st = steps[(size_t) it.layer];
                    if (it.layer != top && topAt.count (it.root) && st.size() >= 3) d = (it.loud - topAt[it.root]) - st[st.size() / 2];
                    else d = (it.loud - curve (it.layer, it.root)) * (it.layer == top ? 12.0f / 18.0f : 1.0f);
                }
                dev.push_back (d);
            }
            {
                size_t k = 0;
                art.items.erase (std::remove_if (art.items.begin(), art.items.end(), [&] (const Item& it)
                {
                    const float d = dev[k++];
                    const bool bad = ! it.release && std::abs (d) > 12.0f;
                    if (bad) log ("!! dropped " + it.src.getFileName() + ": " + juce::String (d, 1) + " dB off its layer (loudness "
                                  + juce::String (it.loud, 1) + ")");
                    return bad;
                }), art.items.end());
            }
            for (auto& it : art.items) if (! it.release) it.gainDb = juce::jlimit (-24.0, 24.0, it.gainDb);
            // release samples: levelDb under the attack sample of the same note / layer (after its gain)
            for (auto& it : art.items)
            {
                if (! it.release) continue;
                const Item* match = nullptr;
                for (auto& a : art.items)
                    if (! a.release && a.root == it.root && a.velName == it.velName) { match = &a; break; }
                if (match == nullptr) for (auto& a : art.items) if (! a.release && (match == nullptr || std::abs (a.root - it.root) < std::abs (match->root - it.root))) match = &a;
                const double attackLoud = match != nullptr ? match->loud + match->gainDb : -20.0;
                it.gainDb = juce::jlimit (-40.0, 20.0, attackLoud + it.levelDb - it.loud);
                it.layer = std::max (0, layers.indexOf (it.velName));
            }
            for (auto& it : art.items)
                for (int c = 0; c < it.audio.getNumChannels(); ++c)
                    instPeak = std::max (instPeak, (double) it.audio.getMagnitude (c, 0, it.audio.getNumSamples()) * juce::Decibels::decibelsToGain (it.gainDb));

            // zones: key ranges per layer from that layer's roots (midpoints), velocity ranges split evenly over the layers.
            // A zone plays at most maxStretch semitones from its root while another layer covers the key within that
            // distance (the sampler then falls back to the nearest velocity layer); only keys NO layer reaches within
            // maxStretch keep the long stretch (reported). The outermost roots reach extendKeys beyond themselves.
            for (int t = 0; t < 2; ++t)
            {
                std::vector<int> allRoots;
                for (auto& it : art.items) if ((int) it.release == t) allRoots.push_back (it.root);
                if (allRoots.empty()) continue;
                const int gLo = std::max (0, *std::min_element (allRoots.begin(), allRoots.end()) - art.o.extendKeys);
                const int gHi = std::min (127, *std::max_element (allRoots.begin(), allRoots.end()) + art.o.extendKeys);
                std::vector<int> fullLo (art.items.size()), fullHi (art.items.size());
                for (int l = 0; l < nl; ++l)
                {
                    std::vector<int> roots;
                    for (auto& it : art.items) if ((int) it.release == t && it.layer == l && std::find (roots.begin(), roots.end(), it.root) == roots.end()) roots.push_back (it.root);
                    std::sort (roots.begin(), roots.end());
                    for (size_t k = 0; k < art.items.size(); ++k)
                    {
                        auto& it = art.items[k];
                        if ((int) it.release != t || it.layer != l) continue;
                        const auto pos = (size_t) (std::find (roots.begin(), roots.end(), it.root) - roots.begin());
                        fullLo[k] = pos == 0 ? gLo : (roots[pos - 1] + it.root) / 2 + 1;
                        fullHi[k] = pos + 1 == roots.size() ? gHi : (it.root + roots[pos + 1]) / 2;
                        fullLo[k] = std::min (fullLo[k], it.root); fullHi[k] = std::max (fullHi[k], it.root);
                        it.lo = std::max (fullLo[k], it.root - art.o.maxStretch);
                        it.hi = std::min (fullHi[k], it.root + art.o.maxStretch);
                        if (it.root - art.o.extendKeys <= gLo) it.lo = std::max (fullLo[k], gLo);   // the outermost roots: extendKeys
                        if (it.root + art.o.extendKeys >= gHi) it.hi = std::min (fullHi[k], gHi);
                        it.vlo = 1 + (127 * l) / nl;
                        it.vhi = (127 * (l + 1)) / nl;
                    }
                }
                std::vector<bool> covered (128, false);
                for (auto& it : art.items) if ((int) it.release == t) for (int k = it.lo; k <= it.hi; ++k) covered[(size_t) k] = true;
                // keys no layer reaches within maxStretch: the zones next to them stretch further (contiguously)
                for (size_t k = 0; k < art.items.size(); ++k)
                {
                    auto& it = art.items[k];
                    if ((int) it.release != t) continue;
                    while (it.lo - 1 >= fullLo[k] && ! covered[(size_t) it.lo - 1]) --it.lo;
                    while (it.hi + 1 <= fullHi[k] && ! covered[(size_t) it.hi + 1]) ++it.hi;
                }
            }
            // key range check: the farthest a zone is played from its root inside the range and at the outer edges, and
            // keys no zone covers
            {
                int inner = 0, edge = 0, gaps = 0; juce::String worst;
                int rLo = 127, rHi = 0;
                for (auto& it : art.items) if (! it.release) { rLo = std::min (rLo, it.root); rHi = std::max (rHi, it.root); }
                std::vector<bool> covered (128, false);
                for (auto& it : art.items)
                {
                    if (it.release) continue;
                    for (int k = it.lo; k <= it.hi; ++k) covered[(size_t) k] = true;
                    const int down = it.root - it.lo, up = it.hi - it.root;
                    if (it.root != rLo && down > inner) { inner = down; worst = it.src.getFileName(); }
                    if (it.root != rHi && up > inner) { inner = up; worst = it.src.getFileName(); }
                    edge = std::max ({ edge, it.root == rLo ? down : 0, it.root == rHi ? up : 0 });
                }
                for (int k = std::max (0, rLo - art.o.extendKeys); k <= std::min (127, rHi + art.o.extendKeys); ++k) if (! covered[(size_t) k]) ++gaps;
                log (in.id + " / " + art.name + ": stretch inside the range <= " + juce::String (inner) + " semitones, at the edges " + juce::String (edge)
                     + (inner > art.o.maxStretch ? "  !! more than " + juce::String (art.o.maxStretch) + " (" + worst + ": no layer has a closer sample)" : juce::String())
                     + (gaps > 0 ? "  !! " + juce::String (gaps) + " keys uncovered" : juce::String()));
            }
        }

        // pass 2 (parallel): instrument gain, encode
        const double norm = juce::Decibels::decibelsToGain (in.arts.empty() ? 0.0 : (double) defaults.peakDb) / std::max (1e-6, instPeak);
        juce::Array<juce::var> artJson;
        for (auto& art : in.arts)
        {
            parallelFor ((int) art.items.size(), threads, [&] (int i)
            {
                auto& it = art.items[(size_t) i];
                const float g = (float) (juce::Decibels::decibelsToGain (it.gainDb) * norm);
                it.audio.applyGain (g);
                // looped: keep 300 ms after the loop end (never played while held), fade the very end
                if (it.loopEnd > 0)
                {
                    const int keepN = std::min (it.audio.getNumSamples(), it.loopEnd + (int) (0.3 * outRate));
                    it.audio.setSize (it.audio.getNumChannels(), keepN, true, true, false);
                    const int f = (int) (0.1 * outRate);
                    for (int c = 0; c < it.audio.getNumChannels(); ++c)
                        for (int k = 0; k < f && k < keepN; ++k) it.audio.getWritePointer (c)[keepN - 1 - k] *= (float) k / (float) f;
                }
                it.outName = art.name + "_" + juce::String (it.root).paddedLeft ('0', 3) + "_v" + juce::String (it.layer + 1) + "_r" + juce::String (it.rr + 1)
                           + (it.release ? "_rel" : "") + ".ogg";
                const auto f = audioDir.getChildFile (it.outName);
                f.deleteFile();
                juce::OggVorbisAudioFormat ogg;
                std::unique_ptr<juce::OutputStream> os (f.createOutputStream());
                auto w = ogg.createWriterFor (os, juce::AudioFormatWriterOptions().withSampleRate (outRate).withNumChannels (it.audio.getNumChannels())
                                                       .withBitsPerSample (16).withQualityOptionIndex (art.o.quality));
                if (w == nullptr || ! w->writeFromAudioSampleBuffer (it.audio, 0, it.audio.getNumSamples())) it.msg << "ENCODE FAILED; ";
            });
            juce::Array<juce::var> zones;
            for (auto& it : art.items)
            {
                auto* z = new juce::DynamicObject();
                z->setProperty ("file", in.id + "/" + it.outName);
                z->setProperty ("root", it.root);
                z->setProperty ("fine", std::round (-it.cents * 10.0f) / 10.0f);
                z->setProperty ("lo", it.lo); z->setProperty ("hi", it.hi);
                z->setProperty ("vlo", it.vlo); z->setProperty ("vhi", it.vhi);
                z->setProperty ("rr", it.rr);
                z->setProperty ("gain", 0.0);
                z->setProperty ("frames", it.audio.getNumSamples());
                z->setProperty ("channels", it.audio.getNumChannels());
                if (it.loopEnd > 0) { z->setProperty ("loopStart", it.loopStart); z->setProperty ("loopEnd", it.loopEnd); z->setProperty ("xfade", it.xfade); }
                if (it.release) z->setProperty ("release", true);
                if (it.noiseDb < 0.0f) z->setProperty ("noise", std::round (it.noiseDb * 10.0f) / 10.0f);   // informative: QC
                zones.add (juce::var (z));
                log (juce::String::formatted ("  %-44s root %3d (%s%s) %+6.1f c  ap %.2f  loud %6.1f  gain %+5.1f dB  keys %3d-%3d  vel %3d-%3d",
                                              it.src.getFileName().toRawUTF8(), it.root, it.noteName.toRawUTF8(), it.release ? " rel" : "",
                                              (double) it.cents, (double) it.aperiodicity, (double) it.loud, it.gainDb, it.lo, it.hi, it.vlo, it.vhi)
                     + (it.loopEnd > 0 ? juce::String::formatted ("  loop %.3f-%.3f s xf %d ms corr %.3f", it.loopStart / outRate, it.loopEnd / outRate,
                                                                 (int) (it.xfade * 1000 / outRate), (double) it.loopCorr) : juce::String())
                     + (it.noiseDb < 0.0f ? juce::String::formatted ("  noise %.0f dB", (double) it.noiseDb) : juce::String())
                     + (it.msg.isNotEmpty() ? "  [" + it.msg.trim() + "]" : juce::String()));
            }
            auto* a = new juce::DynamicObject();
            a->setProperty ("name", art.name);
            a->setProperty ("oneShot", art.o.oneShot);
            a->setProperty ("release", art.o.release);
            a->setProperty ("releaseDecayDb", art.o.releaseDecayDb);
            // informative, for -qc: how the roots were found and where the pitch of a one-shot is stable
            bool anyDetect = false; for (auto& it : art.items) anyDetect |= it.detect;
            a->setProperty ("pitchCheck", art.o.trustNames ? "name" : anyDetect ? "detect" : art.o.spectrumPitch ? "spectrum" : "yin");
            a->setProperty ("pitchFrom", art.o.pitchFrom);
            a->setProperty ("zones", zones);
            artJson.add (juce::var (a));
            for (auto& it : art.items) it.audio.setSize (1, 0);   // free the RAM
        }
        auto* o = new juce::DynamicObject();
        o->setProperty ("id", in.id); o->setProperty ("name", in.name); o->setProperty ("family", in.family); o->setProperty ("credit", in.credit);
        o->setProperty ("articulations", artJson);
        instJson.add (juce::var (o));
        for (int k = keep.size(); --k >= 0;) if (keep[k]["id"].toString() == in.id) keep.remove (k);
        log (in.id + ": done, peak normalisation " + juce::String (juce::Decibels::gainToDecibels (norm), 1) + " dB, "
             + juce::String (audioDir.getNumberOfChildFiles (juce::File::findFiles)) + " files");
    }
    for (auto& k : keep) instJson.add (k);

    auto* root = new juce::DynamicObject();
    root->setProperty ("format", "CHROME VAULT library");
    root->setProperty ("version", 1);
    root->setProperty ("sampleRate", outRate);
    root->setProperty ("credits", credits.joinIntoString ("\n"));
    root->setProperty ("instruments", instJson);
    outDir.createDirectory();
    const bool ok = outDir.getChildFile ("library.json").replaceWithText (juce::JSON::toString (juce::var (root), false));
    outDir.getChildFile (reportName).replaceWithText (report);
    // verify: the plugin's own loader reads it back
    vault::SampleLibrary lib (outDir);
    std::printf ("library.json %s: %d instruments, loader %s %s\n", ok ? "written" : "NOT written", lib.numInstruments(),
                 lib.libraryFound() ? "OK" : "FAILED", lib.errorMessage().toRawUTF8());
    return ok && lib.libraryFound() ? 0 : 1;
}
