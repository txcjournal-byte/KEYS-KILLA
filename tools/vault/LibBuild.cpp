// VaultLibBuild - builds the CHROME VAULT sample library from CC0 source recordings (docs/VAULT.md, "Sample library").
//
//   VaultLibBuild [-map tools/vault/library_map.json] [-out /path/Library] [-only id1,id2] [-threads N] [-dry]
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
#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Source/vault/VaultLibrary.h"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <map>
#include <mutex>
#include <regex>
#include <thread>
#include <tuple>

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
    double pitchFrom = 0.06;   // s: where the pitch analysis of a one-shot starts (later for pianos: the attack glides sharp)
    double pitchSpan = 0;      // s: length of the pitch analysis (0 = auto: 0.35 s one-shots, the whole sustain of loops)
    bool pitchFromSet = false; // an explicit "pitchFrom" also applies to loops (voices: tune the start the ear hears first)
    void read (const juce::var& v)
    {
        if (v.hasProperty ("tune")) keepTuning = v["tune"].toString() == "keep";
        if (v.hasProperty ("pitchFrom")) { pitchFrom = (double) v["pitchFrom"]; pitchFromSet = true; }
        if (v.hasProperty ("pitchSpan")) pitchSpan = (double) v["pitchSpan"];
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
float detectF0 (const juce::AudioBuffer<float>& b, int from, int len, double fExpected, float& aperOut)
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
    const int frames = juce::jlimit (7, 64, len / std::max (1, w / 2));
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
    if (clear.size() >= 3 && clear.size() < f0s.size()) f0s = clear;
    std::sort (f0s.begin(), f0s.end()); std::sort (aps.begin(), aps.end());
    aperOut = aps[aps.size() / 2];
    return f0s[f0s.size() / 2];
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
    const int xf = juce::jlimit ((int) (0.05 * outRate), (int) (0.15 * outRate), (int) (o.xfadeMs * 0.001 * outRate));
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
    const int xfUse = it.loopCorr < 0.6f ? (int) (0.15 * outRate) : xf;
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
    const int pre = (int) (o.preRollMs * 0.001 * outRate);
    const int start = std::max (0, std::min (first, onset - pre));
    int end = std::min (n, last + (int) (0.02 * outRate));
    const int cap = (int) (o.maxSeconds * outRate);
    bool capped = false;
    if (end - start > cap) { end = start + cap; capped = true; }
    juce::AudioBuffer<float> t (ch, end - start);
    for (int c = 0; c < ch; ++c) t.copyFrom (c, 0, a, c, start, end - start);
    // fade-in over the pre-roll (raised cosine), fade-out at the end
    const int fin = std::max (8, onset - start);
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
    const double fExp = 440.0 * std::pow (2.0, (it.namedNote - 69) / 12.0);
    const int n = it.audio.getNumSamples();
    int from = (int) (o.pitchFrom * outRate), len = (int) ((o.pitchFrom > 0.2 ? 1.0 : 0.35) * outRate);
    if (o.loop && ! o.pitchFromSet) { from = (int) (0.35 * outRate); len = std::max ((int) (0.5 * outRate), n - from - (int) (0.4 * outRate)); }
    if (o.pitchSpan > 0) len = (int) (o.pitchSpan * outRate);
    len = std::min (len, n - from);
    float ap = 1;
    it.f0 = len > 1024 ? detectF0 (it.audio, from, len, fExp, ap) : 0.0f;
    it.aperiodicity = ap;
    it.pitchOk = it.f0 > 0 && ap < 0.35f;
    if (o.loop && ! findLoop (it, o)) it.msg << "NO LOOP FOUND (plays to the end); ";
}
} // namespace

//==============================================================================
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;   // message manager (juce::JSON, file APIs)
    juce::String mapPath = "tools/vault/library_map.json", outOverride, only;
    int threads = juce::jlimit (1, 8, (int) std::thread::hardware_concurrency());
    bool dry = false;
    for (int i = 1; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (a == "-map" && i + 1 < argc) mapPath = argv[++i];
        else if (a == "-out" && i + 1 < argc) outOverride = argv[++i];
        else if (a == "-only" && i + 1 < argc) only = argv[++i];
        else if (a == "-threads" && i + 1 < argc) threads = juce::jmax (1, juce::String (argv[++i]).getIntValue());
        else if (a == "-dry") dry = true;
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
                auto files = dir.findChildFiles (juce::File::findFiles, false, glob);
                files.sort();
                int skipped = 0;
                for (auto& f : files)
                {
                    std::smatch m;
                    const std::string name = f.getFileName().toStdString();
                    if (! std::regex_match (name, m, rx)) { ++skipped; continue; }
                    Item it;
                    it.src = f; it.release = isRel; it.levelDb = levelDb;
                    // groups in pattern order: {note} {vel} {rr} (each optional)
                    const auto pat = sv["pattern"].toString();
                    std::vector<std::pair<int, juce::String>> order;
                    for (auto* key : { "{note}", "{vel}", "{rr}" }) if (pat.contains (key)) order.push_back ({ pat.indexOf (key), key });
                    std::sort (order.begin(), order.end());
                    for (size_t g = 0; g < order.size(); ++g)
                    {
                        const juce::String val (m[(size_t) g + 1].str());
                        if (order[g].second == "{note}") it.noteName = val;
                        else if (order[g].second == "{vel}") it.velName = val;
                        else it.rr = std::max (0, val.getIntValue() - 1);
                    }
                    it.namedNote = parseNote (it.noteName);
                    if (it.namedNote < 0) { ++skipped; continue; }
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

            // pitch: the naming convention offset (octaves) from the median, then per file
            std::vector<int> offs;
            for (auto& it : art.items)
                if (! it.release && it.pitchOk && it.namedNote >= 36)
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
                it.root = it.namedNote + octOff;
                if (it.release) continue;
                if (it.pitchOk)
                {
                    const double detected = 12.0 * std::log2 (it.f0 / 440.0) + 69.0;
                    const int near = (int) std::lround (detected);
                    if (near != it.root && it.root >= 36)
                    {
                        log ("!! " + it.src.getFileName() + ": named " + it.noteName + " (" + juce::String (it.root) + ") but YIN hears MIDI "
                             + juce::String (detected, 2) + (std::abs (near - it.root) <= 2 ? " -> root corrected" : " -> kept the name (check!)"));
                        if (std::abs (near - it.root) <= 2) it.root = near;
                    }
                    it.cents = (float) ((detected - it.root) * 100.0);
                    if (std::abs (it.cents) > 50.0f) it.cents = 0.0f;
                    if (art.o.keepTuning && std::abs (it.cents) <= 30.0f) { it.msg << "tuning kept (" << juce::String (it.cents, 1) << " c); "; it.cents = 0.0f; }
                }
                else it.msg << "pitch not verified (aperiodicity " << juce::String (it.aperiodicity, 2) << "); ";
            }

            // a corrected root can land on a note that already has a take with the same layer / round robin: give it the next
            // free round robin (the output file name is built from root / layer / rr and would otherwise overwrite it)
            {
                std::map<std::tuple<int, juce::String, bool>, std::vector<int>> used;
                for (auto& it : art.items)
                {
                    auto& rrs = used[{ it.root, it.velName, it.release }];
                    while (std::find (rrs.begin(), rrs.end(), it.rr) != rrs.end()) ++it.rr;
                    rrs.push_back (it.rr);
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

            // zones: key ranges per layer from that layer's roots (midpoints), velocity ranges split evenly over the layers
            for (int t = 0; t < 2; ++t)
                for (int l = 0; l < nl; ++l)
                {
                    std::vector<int> roots;
                    for (auto& it : art.items) if ((int) it.release == t && it.layer == l && std::find (roots.begin(), roots.end(), it.root) == roots.end()) roots.push_back (it.root);
                    std::sort (roots.begin(), roots.end());
                    for (auto& it : art.items)
                    {
                        if ((int) it.release != t || it.layer != l) continue;
                        const auto pos = (size_t) (std::find (roots.begin(), roots.end(), it.root) - roots.begin());
                        it.lo = pos == 0 ? std::max (0, it.root - art.o.extendKeys) : (roots[pos - 1] + it.root) / 2 + 1;
                        it.hi = pos + 1 == roots.size() ? std::min (127, it.root + art.o.extendKeys) : (it.root + roots[pos + 1]) / 2;
                        it.vlo = 1 + (127 * l) / nl;
                        it.vhi = (127 * (l + 1)) / nl;
                    }
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
                zones.add (juce::var (z));
                log (juce::String::formatted ("  %-44s root %3d (%s%s) %+6.1f c  ap %.2f  loud %6.1f  gain %+5.1f dB  keys %3d-%3d  vel %3d-%3d",
                                              it.src.getFileName().toRawUTF8(), it.root, it.noteName.toRawUTF8(), it.release ? " rel" : "",
                                              (double) it.cents, (double) it.aperiodicity, (double) it.loud, it.gainDb, it.lo, it.hi, it.vlo, it.vhi)
                     + (it.loopEnd > 0 ? juce::String::formatted ("  loop %.3f-%.3f s xf %d ms corr %.3f", it.loopStart / outRate, it.loopEnd / outRate,
                                                                 (int) (it.xfade * 1000 / outRate), (double) it.loopCorr) : juce::String())
                     + (it.msg.isNotEmpty() ? "  [" + it.msg.trim() + "]" : juce::String()));
            }
            auto* a = new juce::DynamicObject();
            a->setProperty ("name", art.name);
            a->setProperty ("oneShot", art.o.oneShot);
            a->setProperty ("release", art.o.release);
            a->setProperty ("releaseDecayDb", art.o.releaseDecayDb);
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
