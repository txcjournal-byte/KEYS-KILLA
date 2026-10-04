#pragma once
#include "MixLab.h"
#include <juce_core/juce_core.h>
#include <functional>

// v0.41 COACH: listens to what comes out of EVOLVE (or to your beat when EVOLVE sits on the master as an effect),
// compares it with what the style usually sounds like, and tells you what to change - with the reason, and a FIX.
// It is a rule engine built from mixing practice (tonal balance per genre, crest factor, phase, resonances),
// so every tip can explain itself.  Plus: AUTO EQ (resonances + balance) and EQ EVOLVE (curves that mutate).
namespace kk::coach
{
enum Region { rSub, rBass, rLowMid, rMid, rUpperMid, rPresence, rBrilliance, rAir, numRegions };
inline const char* regionName (int r)
{
    static const char* n[] { "SUB", "BASS", "LOW MIDS", "MIDS", "UPPER MIDS", "PRESENCE", "BRILLIANCE", "AIR" };
    return n[std::clamp (r, 0, (int) numRegions - 1)];
}
inline float regionLo (int r) { static const float f[] { 20, 60, 150, 400, 1000, 2500, 5000, 10000 }; return f[std::clamp (r, 0, 7)]; }
inline float regionHi (int r) { static const float f[] { 60, 150, 400, 1000, 2500, 5000, 10000, 20000 }; return f[std::clamp (r, 0, 7)]; }
inline float regionMid (int r) { return std::sqrt (regionLo (r) * regionHi (r)); }

// what a finished track of the style tends to look like: energy per region relative to the mids (dB)
// genres follow kk::mel (TRAP, DARK TRAP, DRILL, PLUGG, R&B, LO-FI, HOUSE, DNB, AFRO); -1 = a neutral reference
inline std::array<float, numRegions> target (int genre, bool wholeBeat = true)
{
    if (! wholeBeat)   // one melodic sound inside a beat: it leaves the low end to the 808 and the kick
    {
        if (genre == 5 || genre == 4) return { -12.0f, -5.0f, -1.0f, 0.0f, -2.0f, -4.0f, -7.0f, -12.0f };   // lo-fi / r&b keys keep some warmth
        return { -16.0f, -8.0f, -2.0f, 0.0f, -1.0f, -2.5f, -5.0f, -9.0f };
    }
    switch (genre)
    {
        case 0: case 1: case 2: return { 7.0f, 5.0f, 0.5f, 0.0f, -2.5f, -4.5f, -7.0f, -11.0f };   // trap / drill: big sub, soft top
        case 3:                 return { 5.0f, 4.0f, 0.5f, 0.0f, -2.0f, -3.5f, -5.5f, -9.0f };    // plugg: brighter
        case 4:                 return { 4.5f, 4.0f, 1.0f, 0.0f, -2.5f, -4.5f, -7.0f, -11.0f };   // r&b: warm
        case 5:                 return { 3.0f, 4.0f, 2.0f, 0.0f, -3.5f, -6.5f, -10.0f, -16.0f };  // lo-fi: dark and warm
        case 6:                 return { 4.0f, 5.0f, 0.5f, 0.0f, -2.0f, -3.5f, -5.5f, -9.0f };    // house
        case 7:                 return { 5.0f, 3.5f, -0.5f, 0.0f, -1.5f, -3.0f, -5.0f, -8.0f };   // dnb: bright
        case 8:                 return { 4.0f, 4.0f, 0.5f, 0.0f, -2.0f, -4.0f, -6.0f, -10.0f };   // afro
        default:                return { 4.0f, 3.5f, 0.5f, 0.0f, -2.0f, -4.0f, -6.5f, -10.0f };
    }
}

struct Reading
{
    bool silent = true;
    std::array<float, numRegions> region {};     // dB (relative to the mids after normalise())
    float rmsDb = -100, peakDb = -100, crest = 12, corr = 1, width = 0;
    std::vector<std::pair<float, float>> resonances;   // (Hz, dB above the local average)
    int key = -1; bool minor = true; float keyConf = 0;
};

// power spectrum (bins 0..N/2) -> a reading
inline Reading read (const std::vector<float>& power, float sr, const MixLabState& st, const std::vector<float>* persist = nullptr)
{
    Reading r;
    r.rmsDb = 20.0f * std::log10 (st.mRms.load() + 1e-9f);
    r.peakDb = 20.0f * std::log10 (st.mPeak.load() + 1e-9f);
    r.crest = st.mCrest.load(); r.corr = st.mCorr.load(); r.width = st.mWidth.load();
    r.silent = r.rmsDb < -60.0f || power.size() < 64;
    if (r.silent) return r;
    const int bins = (int) power.size();
    const float hzPerBin = sr * 0.5f / (float) (bins - 1);
    for (int g = 0; g < numRegions; ++g)
    {
        double s = 0; int c = 0;
        for (int b = std::max (1, (int) (regionLo (g) / hzPerBin)); b <= std::min (bins - 1, (int) (regionHi (g) / hzPerBin)); ++b) { s += power[(size_t) b]; ++c; }
        // energy per octave-ish region, normalised by width in octaves so wide regions do not win
        const float oct = std::log2 (regionHi (g) / regionLo (g));
        r.region[(size_t) g] = 10.0f * std::log10 ((float) (s / std::max (1, c)) * std::max (1, c) / std::max (0.3f, oct) + 1e-20f);
    }
    const float ref = r.region[(size_t) rMid];
    for (auto& v : r.region) v -= ref;
    // resonances: narrow peaks well above their neighbourhood (200 Hz .. 9 kHz)
    std::vector<float> db ((size_t) bins);
    for (int b = 0; b < bins; ++b) db[(size_t) b] = 10.0f * std::log10 (power[(size_t) b] + 1e-20f);
    for (int b = std::max (2, (int) (200.0f / hzPerBin)); b < std::min (bins - 2, (int) (9000.0f / hzPerBin)); ++b)
    {
        if (! (db[(size_t) b] > db[(size_t) b - 1] && db[(size_t) b] >= db[(size_t) b + 1])) continue;
        const int span = std::max (4, b / 6);   // about a third of an octave around
        double s = 0; int c = 0;
        for (int k = std::max (1, b - span); k <= std::min (bins - 1, b + span); ++k) if (std::abs (k - b) > 1) { s += db[(size_t) k]; ++c; }
        const float above = db[(size_t) b] - (float) (s / std::max (1, c));
        // a resonance is there whatever note plays: it must stick out in (almost) every moment, not only in the long average
        const bool steady = persist == nullptr || (b < (int) persist->size() && (*persist)[(size_t) b] > 0.85f);
        if (above > 9.0f && steady) r.resonances.push_back ({ (float) b * hzPerBin, above });
    }
    std::sort (r.resonances.begin(), r.resonances.end(), [] (auto& a, auto& b) { return a.second > b.second; });
    if (r.resonances.size() > 3) r.resonances.resize (3);
    // key: pitch salience (a note's fundamental + its harmonics, so the 5th harmonic does not pass for a major third), Krumhansl profiles
    float pc[12] {};
    auto amp = [&] (float hz) { const int b = (int) std::round (hz / hzPerBin); if (b < 1 || b >= bins - 1) return 0.0f; return std::sqrt (std::max ({ power[(size_t) b - 1], power[(size_t) b], power[(size_t) b + 1] })); };
    std::array<float, 128> sal {};
    for (int m = 36; m <= 88; ++m)
    {
        const float f0 = 440.0f * std::pow (2.0f, (float) (m - 69) / 12.0f);
        float sv = 0, w = 1;
        for (int h = 1; h <= 6; ++h, w *= 0.75f) sv += w * amp (f0 * (float) h);
        sal[(size_t) m] = sv;
    }
    for (int m = 36; m <= 88; ++m)
    {
        // a note counts when it is not just the harmonic of a lower note (an octave or a twelfth below)
        float v = sal[(size_t) m] - 0.6f * std::max (m >= 48 ? sal[(size_t) m - 12] : 0.0f, m >= 55 ? sal[(size_t) m - 19] : 0.0f);
        if (v > 0) pc[m % 12] += v * v;
    }
    static const float majP[12] { 6.35f, 2.23f, 3.48f, 2.33f, 4.38f, 4.09f, 2.52f, 5.19f, 2.39f, 3.66f, 2.29f, 2.88f };
    static const float minP[12] { 6.33f, 2.68f, 3.52f, 5.38f, 2.60f, 3.53f, 2.54f, 4.75f, 3.98f, 2.69f, 3.34f, 3.17f };
    float best = -2, second = -2;
    for (int k = 0; k < 12; ++k)
        for (int mm = 0; mm < 2; ++mm)
        {
            const float* p = mm ? minP : majP;
            float mx = 0, mp = 0; for (int i = 0; i < 12; ++i) { mx += pc[i]; mp += p[i]; } mx /= 12; mp /= 12;
            float num = 0, dx = 0, dp = 0;
            for (int i = 0; i < 12; ++i) { const float a = pc[(i + k) % 12] - mx, b = p[i] - mp; num += a * b; dx += a * a; dp += b * b; }
            const float c = num / std::sqrt (dx * dp + 1e-12f);
            if (c > best) { second = best; best = c; r.key = k; r.minor = mm == 1; } else if (c > second) second = c;
        }
    r.keyConf = best - second;
    return r;
}

enum FixKind { fixNone, fixLower, fixLouder, fixLessSquash, fixTameSpikes, fixRegionCut, fixRegionBoost, fixResonance, fixMonoBass, fixUseKey };
struct Tip
{
    int severity = 0;            // 0 info, 1 small, 2 clear, 3 big
    juce::String title, why, fixLabel;
    int fix = fixNone; float hz = 0, db = 0; int region = -1;
};

inline juce::String noteOf (float hz)
{
    static const char* n[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int m = (int) std::round (69.0f + 12.0f * std::log2 (std::max (1.0f, hz) / 440.0f));
    return juce::String (n[((m % 12) + 12) % 12]) + juce::String (m / 12 - 1);
}
inline juce::String hzText (float hz) { return hz >= 10000.0f ? juce::String ((int) std::round (hz / 1000.0f)) + " kHz" : hz >= 1000.0f ? juce::String (hz / 1000.0f, 1) + " kHz" : juce::String ((int) std::round (hz)) + " Hz"; }

inline std::vector<Tip> advise (const Reading& r, int genre, bool wholeBeat = true)
{
    std::vector<Tip> out;
    if (r.silent) return out;
    if (r.peakDb > -0.3f)
        out.push_back ({ 3, "IT HITS 0 dB", "The loudest peaks touch the ceiling - FL's master will clip or squash them. Leave 1-3 dB of headroom and get loudness from balance, not from the fader.", "LOWER 3 dB", fixLower, 0, -3 });
    else if (r.rmsDb < -32.0f)
        out.push_back ({ 1, "QUIET", "The average level is low. You judge sounds louder = better, so compare at equal level. A gentle GLUE compressor with auto gain brings it up evenly.", "GLUE IT", fixLouder });
    if (r.crest < 6.5f && r.rmsDb > -40)
        out.push_back ({ 2, "SQUASHED", "Peaks and average are only " + juce::String (r.crest, 1) + " dB apart: the hits lost their punch. Ease the compression (lower ratio, or blend it in parallel).", "LESS SQUASH", fixLessSquash });
    else if (r.crest > (wholeBeat ? 19.0f : 23.0f))
        out.push_back ({ 1, "SPIKY", "Some hits jump " + juce::String (r.crest, 1) + " dB above the rest. A PUNCH compressor (slow attack) keeps the snap and evens the body.", "PUNCH IT", fixTameSpikes });
    const auto T = target (genre, wholeBeat);
    const juce::String style = genre >= 0 ? juce::String (genre <= 8 ? std::array<const char*, 9> { "trap", "dark trap", "drill", "plugg", "r&b", "lo-fi", "house", "dnb", "afro" }[(size_t) genre] : "this style") : juce::String ("a balanced mix");
    int worst = -1; float worstD = 0;
    for (int g = 0; g < numRegions; ++g)
    {
        if (g == rMid) continue;
        const float d = r.region[(size_t) g] - T[(size_t) g];
        const float tol = g == rSub || g == rAir ? 5.0f : 3.5f;
        if (! wholeBeat && g <= rLowMid && r.region[(size_t) g] < T[(size_t) g]) continue;   // a melody sound with little low end is right
        if (std::abs (d) > tol && std::abs (d) - tol > std::abs (worstD)) { worst = g; worstD = d > 0 ? d - tol : d + tol; }
    }
    if (worst >= 0)
    {
        const bool much = worstD > 0;
        static const char* whyMuchSound[] { "This sound has a lot of sub: it will fight the 808 and the kick. Cut it here and let the 808 own the low end.",
                                            "Heavy bass in a melody muddies the 808. A low shelf down (or a low cut) leaves it room." };
        static const char* whyMuch[] { "Too much sub eats headroom and makes the 808 and kick fight.", "Heavy bass masks the kick and the 808 - the low end turns to one blur.",
                                       "Low mids are where mud lives: boxy, cardboard, crowded.", "Strong mids sound honky and nasal.",
                                       "Upper mids poke out and tire the ear quickly.", "2.5-5 kHz is where the ear is most sensitive - too much is harsh.",
                                       "Too much brilliance sounds thin and sizzly.", "Too much air makes hats and breaths hiss." };
        static const char* whyLess[] { "Little sub: the track will feel small on big speakers.", "Thin bass: no weight under the melody.",
                                       "Few low mids: warmth is missing, it sounds hollow.", "Scooped mids: the melody hides in the mix.",
                                       "Few upper mids: the sound loses definition.", "Little presence: it sounds far away, behind a curtain.",
                                       "Dull: the sparkle is missing.", "No air: closed and dark at the top." };
        Tip t; t.severity = std::abs (worstD) > 4 ? 3 : 2; t.region = worst; t.hz = regionMid (worst);
        t.title = juce::String (much ? "TOO MUCH " : "NOT ENOUGH ") + regionName (worst) + "  (" + hzText (regionLo (worst)) + " - " + hzText (regionHi (worst)) + ")";
        const juce::String reason = (! wholeBeat && much && worst <= rBass) ? juce::String (whyMuchSound[worst]) : juce::String (much ? whyMuch[worst] : whyLess[worst]);
        t.why = reason + " Compared with " + (wholeBeat ? style : "a melody sound for " + style) + " it is " + juce::String (std::min (12.0f, std::abs (worstD)), 1) + (std::abs (worstD) > 12.0f ? "+" : "") + " dB " + (much ? "over." : "under.");
        t.fix = much ? fixRegionCut : fixRegionBoost; t.db = much ? -std::min (6.0f, std::abs (worstD)) : std::min (4.5f, std::abs (worstD));
        if (! wholeBeat && much && worst <= rBass) t.hz = worst == rSub ? 90.0f : 140.0f;   // a melody: a low cut, the 808 owns that range
        t.fixLabel = juce::String (much ? "CUT " : "LIFT ") + juce::String (std::abs (t.db), 1) + " dB";
        out.push_back (t);
    }
    if (! r.resonances.empty())
    {
        const auto& rs = r.resonances.front();
        out.push_back ({ 2, "RINGING AT " + hzText (rs.first) + "  (" + noteOf (rs.first) + ")", "One frequency sticks out " + juce::String (rs.second, 1) + " dB above its neighbours - it rings in every note. A narrow DYNAMIC cut only works when it rings.", "TAME IT", fixResonance, rs.first, -std::min (9.0f, rs.second * 0.6f) });
    }
    if (r.corr < 0.15f)
        out.push_back ({ 3, "PHASE: IT DISAPPEARS IN MONO", "Left and right cancel each other (correlation " + juce::String (r.corr, 2) + "). On phones and club systems parts will vanish. Keep the bass mono and reduce stereo wideners.", "MONO BASS", fixMonoBass });
    else if (r.width > 0.75f)
        out.push_back ({ 1, "VERY WIDE", "The sides are as loud as the middle. Wide is exciting, but the 808, kick and lead vocal belong in the middle.", "", fixNone });
    if (wholeBeat && r.key >= 0 && r.keyConf > 0.04f)
    {
        static const char* kn[] { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
        out.push_back ({ 0, juce::String ("IN ") + kn[r.key] + (r.minor ? " MINOR" : " MAJOR"), "That is the key the music sits in. Melodies, 808 notes and samples in the same key glue together.", "USE THIS KEY", fixUseKey, 0, 0, r.key * 2 + (r.minor ? 1 : 0) });
    }
    std::stable_sort (out.begin(), out.end(), [] (const Tip& a, const Tip& b) { return a.severity > b.severity; });
    return out;
}

// 100 = nothing to fix
inline int score (const std::vector<Tip>& tips)
{
    int s = 100;
    for (auto& t : tips) s -= t.severity == 3 ? 22 : t.severity == 2 ? 12 : t.severity == 1 ? 5 : 0;
    return std::clamp (s, 0, 100);
}
inline const char* level (int score)
{
    return score >= 92 ? "MASTER" : score >= 80 ? "PRO" : score >= 65 ? "SKILLED" : score >= 45 ? "LEARNING" : "ROOKIE";
}

// the EQ band to use for a fix: an empty band near the frequency, or the closest one
inline int freeBand (MixLabState& st, float hz, int wantType)
{
    int best = -1; float bd = 1e9f;
    for (int b = 1; b < MixLabState::numBands - 1; ++b)
    {
        auto& bb = st.band[(size_t) b];
        const float d = std::abs (std::log2 (bb.freq.load() / hz)) + (bb.on.load() ? 3.0f : 0.0f) + (bb.type.load() != wantType ? 0.5f : 0.0f);
        if (d < bd) { bd = d; best = b; }
    }
    return best;
}

// FIX: what a tip changes (returns a short note of what was done)
inline juce::String applyFix (MixLabState& st, const Tip& t)
{
    switch (t.fix)
    {
        case fixLower: st.eqOn = true; st.eqOut = st.eqOut.load() + t.db; return "output -3 dB";
        case fixLouder: st.compOn = true; st.compStyle = csGlue; st.ratio = 2.5f; st.attack = 20; st.release = 150; st.knee = 8; st.thresh = std::max (-30.0f, std::min (-10.0f, 20.0f * std::log10 (st.mRms.load() + 1e-9f) + 4.0f)); st.autoGain = true; st.compMix = 1; return "GLUE compressor on, auto gain";
        case fixLessSquash: st.compOn = true; st.ratio = std::max (1.5f, st.ratio.load() * 0.6f); st.compMix = std::min (st.compMix.load(), 0.6f); return "ratio lower, 60% parallel";
        case fixTameSpikes: st.compOn = true; st.compStyle = csPunch; st.ratio = 4; st.attack = 12; st.release = 80; st.knee = 4; st.thresh = std::max (-30.0f, 20.0f * std::log10 (st.mPeak.load() + 1e-9f) - 8.0f); st.autoGain = true; return "PUNCH compressor, attack 12 ms";
        case fixRegionCut: case fixRegionBoost:
        {
            st.eqOn = true;
            const int g = t.region;
            int b; int type;
            if (g <= rBass && t.fix == fixRegionCut && t.hz > 0)   // a melody sound: low cut
            {
                auto& lc = st.band[0]; lc.on = true; lc.type = eqLowCut; lc.freq = t.hz; lc.slope = 2; lc.q = 0.71f;
                return "low cut at " + hzText (t.hz) + " (room for the 808)";
            }
            if (g == rSub && t.fix == fixRegionCut) { b = 0; type = eqLowCut; }
            else if (g <= rBass) { b = 1; type = eqLowShelf; }
            else if (g >= rAir) { b = 6; type = eqHighShelf; }
            else { type = eqBell; b = freeBand (st, regionMid (g), eqBell); }
            auto& bb = st.band[(size_t) b];
            bb.on = true; bb.type = type;
            if (type == eqLowCut) { bb.freq = 35.0f; bb.slope = 2; return "low cut at 35 Hz"; }
            bb.freq = type == eqLowShelf ? (g == rSub ? 70.0f : 160.0f) : type == eqHighShelf ? 9000.0f : regionMid (g);
            bb.gain = t.db; bb.q = type == eqBell ? 0.8f : 0.71f;
            bb.dyn = (g == rPresence || g == rLowMid) && t.fix == fixRegionCut ? 0.5f : 0.0f;
            return juce::String (eqTypeName (type)) + " " + juce::String (t.db, 1) + " dB at " + hzText (bb.freq.load());
        }
        case fixResonance:
        {
            st.eqOn = true;
            const int b = freeBand (st, t.hz, eqBell);
            auto& bb = st.band[(size_t) b];
            bb.on = true; bb.type = eqBell; bb.freq = t.hz; bb.q = 8.0f; bb.gain = t.db * 0.5f; bb.dyn = 0.6f;
            return "dynamic notch at " + hzText (t.hz);
        }
        case fixMonoBass: st.tmOn = true; st.tmMono = std::max (st.tmMono.load(), 0.35f); return "more mono (TIME MACHINE)";
        default: return {};
    }
}

// ---------------- AUTO EQ: tame resonances, nudge the balance toward the style ----------------
inline juce::String autoEq (MixLabState& st, const Reading& r, int genre)
{
    if (r.silent) return "play something first";
    st.eqOn = true;
    int done = 0;
    for (auto& rs : r.resonances)
    {
        Tip t; t.fix = fixResonance; t.hz = rs.first; t.db = -std::min (9.0f, rs.second * 0.6f);
        applyFix (st, t); ++done;
    }
    const auto T = target (genre);
    // half of the difference, gently (it is a starting point, your ears finish it)
    auto setBand = [&] (int b, int type, float hz, float gain, float q)
    {
        auto& bb = st.band[(size_t) b];
        if (std::abs (gain) < 0.8f) return;
        bb.on = true; bb.type = type; bb.freq = hz; bb.gain = std::clamp (gain, -6.0f, 4.5f); bb.q = q; bb.dyn = 0; ++done;
    };
    setBand (1, eqLowShelf, 110.0f, 0.5f * (T[rBass] - r.region[rBass] + T[rSub] - r.region[rSub]) * 0.5f, 0.71f);
    setBand (6, eqHighShelf, 8000.0f, 0.5f * (T[rBrilliance] - r.region[rBrilliance] + T[rAir] - r.region[rAir]) * 0.5f, 0.71f);
    const float lm = 0.5f * (T[rLowMid] - r.region[rLowMid]);
    if (lm < -0.8f) { const int b = freeBand (st, 300.0f, eqBell); setBand (b, eqBell, 300.0f, lm, 0.9f); }
    const float pr = 0.5f * (T[rPresence] - r.region[rPresence]);
    if (std::abs (pr) > 0.8f) { const int b = freeBand (st, 3500.0f, eqBell); setBand (b, eqBell, 3500.0f, pr, 0.9f); if (pr < 0) st.band[(size_t) b].dyn = 0.5f; }
    if (! st.band[0].on.load()) { st.band[0].on = true; st.band[0].type = eqLowCut; st.band[0].freq = 25.0f; st.band[0].slope = 2; ++done; }
    return done > 0 ? juce::String (done) + " moves (resonances + balance)" : juce::String ("already balanced");
}

// ---------------- EQ EVOLVE: curves that mutate - you pick by ear ----------------
struct EqSnap
{
    struct B { bool on = false; int type = 0, slope = 1; float freq = 1000, gain = 0, q = 0.9f, dyn = 0; };
    std::array<B, MixLabState::numBands> b;
    float out = 0;
};
inline EqSnap snap (const MixLabState& st)
{
    EqSnap s;
    for (int i = 0; i < MixLabState::numBands; ++i)
    {
        auto& x = st.band[(size_t) i];
        s.b[(size_t) i] = { x.on.load(), x.type.load(), x.slope.load(), x.freq.load(), x.gain.load(), x.q.load(), x.dyn.load() };
    }
    s.out = st.eqOut.load();
    return s;
}
inline void apply (MixLabState& st, const EqSnap& s)
{
    for (int i = 0; i < MixLabState::numBands; ++i)
    {
        auto& x = st.band[(size_t) i]; auto& y = s.b[(size_t) i];
        x.on = y.on; x.type = y.type; x.slope = y.slope; x.freq = y.freq; x.gain = y.gain; x.q = y.q; x.dyn = y.dyn;
    }
    st.eqOut = s.out;
}
inline EqSnap mutate (const EqSnap& p, uint32_t seed, float wild)
{
    Rng r; r.seed (seed * 2654435761u + 17u);
    EqSnap c = p;
    int changed = 0;
    for (int i = 0; i < MixLabState::numBands; ++i)
    {
        auto& b = c.b[(size_t) i];
        const bool cut = b.type == eqLowCut || b.type == eqHighCut;
        if (! b.on && ! cut && r.uni() < 0.18f + 0.25f * wild) { b.on = true; b.gain = r.bi() * (2.0f + 3.0f * wild); b.q = 0.6f + r.uni() * 1.4f; ++changed; }
        if (! b.on) continue;
        if (r.uni() < 0.7f)
        {
            b.freq = std::clamp (b.freq * std::exp2 (r.bi() * (0.3f + 1.0f * wild)), 20.0f, 20000.0f);
            if (! cut) b.gain = std::clamp (b.gain + r.bi() * (1.5f + 4.0f * wild), -12.0f, 9.0f);
            if (b.type == eqBell) b.q = std::clamp (b.q * std::exp2 (r.bi() * (0.5f + wild)), 0.3f, 6.0f);
            ++changed;
        }
    }
    if (changed == 0) { auto& b = c.b[3]; b.on = true; b.gain = r.bi() * 4.0f; b.freq = 400.0f * std::exp2 (r.uni() * 4.0f); }
    // keep the level about the same (louder always sounds better - that is not a fair choice)
    float sum = 0; int nb = 0;
    for (auto& b : c.b) if (b.on && (b.type == eqBell || b.type == eqLowShelf || b.type == eqHighShelf)) { sum += b.gain * (b.type == eqBell ? std::min (1.0f, 1.0f / b.q) : 0.5f); ++nb; }
    c.out = std::clamp (p.out - 0.35f * sum / std::max (1, nb) * std::min (3, nb) / 3.0f, -12.0f, 6.0f);
    return c;
}
// the curve of a snapshot (dB) at hz
inline float curveDb (const EqSnap& s, float hz, float sr)
{
    float db = s.out;
    for (auto& b : s.b)
        if (b.on)
        {
            const float one = eqMagDb (eqCoefs (b.type, b.freq, b.gain, b.q, sr), hz, sr);
            db += one * ((b.type == eqLowCut || b.type == eqHighCut) ? (float) b.slope : 1.0f);
        }
    return db;
}

// lessons: one appears at a time, picked for what the COACH hears
inline const juce::StringArray& lessons()
{
    static const juce::StringArray l {
        "Cut before you boost: removing what is too much sounds more natural than adding what is missing.",
        "Compare at the same level: a louder version always seems better. The EQ EVOLVE keeps levels matched for that reason.",
        "808 and kick: give each its own space - the kick's punch around 60-100 Hz, the 808's tone a little higher or lower.",
        "A melody in the 2-5 kHz range cuts through on phone speakers - but that is where harshness lives too.",
        "Slow attack (10-30 ms) on a compressor lets the hit through, then controls the body: that is punch.",
        "Parallel compression (MIX 40-60%) keeps the life of the dry sound and adds density underneath.",
        "Mono below 120 Hz: bass in stereo loses power on club systems and phones.",
        "Resonances ring in every note: narrow DYNAMIC cuts remove them only when they ring.",
        "Vintage colour works best in small doses on many layers rather than a lot on one.",
        "Trap melodies breathe: short bell notes and space let the 808 and the vocal through.",
        "Leave 1-3 dB headroom on the master: the mastering (or FL's limiter) needs room to work.",
        "Reference: play a track you love at the same level and switch back and forth.",
        "Every boost of 3 dB is twice the power: small moves go a long way.",
        "If two sounds fight, ask which one leads - then carve a little space out of the other.",
        "Glue: 2-3 dB of gain reduction with a slow release on the whole melody bus makes the parts feel like one record.",
        "Tape wobble is movement: a tiny amount makes static synths feel recorded.",
        "Bit crushing adds harmonics: it can make a dull sample cut without any EQ.",
        "Change one thing at a time and listen for 10 seconds - your ears adapt fast.",
    };
    return l;
}
} // namespace kk::coach
