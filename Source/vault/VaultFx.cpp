#include "VaultFx.h"

namespace vault
{
namespace
{
inline float smoothCoef (double sr) { return 1.0f - std::exp (-1.0f / (0.02f * (float) sr)); }   // FxRack's 20 ms smoothing

// dry / wet blend helper: `mix` 1 = wet only (exactly the stage's output)
struct DryBuf
{
    std::vector<float> l, r;
    void prepare (int n) { l.assign ((size_t) n, 0.0f); r.assign ((size_t) n, 0.0f); }
    void keep (const float* L, const float* R, int n) { std::copy (L, L + n, l.data()); std::copy (R, R + n, r.data()); }
    void blend (float* L, float* R, int n, float mix) const
    {
        for (int i = 0; i < n; ++i) { L[i] = l[(size_t) i] + (L[i] - l[(size_t) i]) * mix; R[i] = r[(size_t) i] + (R[i] - r[(size_t) i]) * mix; }
    }
};

struct EqProc final : FxProcessor
{
    kk::EqStage st;
    void prepare (double sr, int) override { st.prepare ((float) sr); }
    void clear() override { st.clear(); }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        // p: tilt, low, lowFreq, mid, midFreq, midQ, high, highFreq. Tilt = high shelf +t, low shelf -t/2 (the classic TONE)
        const float tilt = p[0];
        st.process (L, R, n, p[1] - tilt * 0.5f, p[6] + tilt, p[2], p[7], p[3], p[4], p[5]);
    }
};

// feed-forward compressor: stereo-linked peak detector, soft knee, makeup, mix
struct CompProc final : FxProcessor
{
    double sr = 44100; float env = 0; DryBuf dry;
    void prepare (double s, int maxBlock) override { sr = s; dry.prepare (maxBlock); env = 0; }
    void clear() override { env = 0; }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        const float thr = p[0], ratio = std::max (1.0f, p[1]), knee = std::max (0.01f, p[6]);
        const float atk = 1.0f - std::exp (-1.0f / (std::max (0.05f, p[2]) * 0.001f * (float) sr));
        const float rel = 1.0f - std::exp (-1.0f / (std::max (5.0f, p[3]) * 0.001f * (float) sr));
        const float makeup = juce::Decibels::decibelsToGain (p[4]), mix = juce::jlimit (0.0f, 1.0f, p[5]);
        if (mix < 1.0f) dry.keep (L, R, n);
        const float slope = 1.0f - 1.0f / ratio;
        for (int i = 0; i < n; ++i)
        {
            const float x = std::max (std::abs (L[i]), std::abs (R[i]));
            env += (x - env) * (x > env ? atk : rel);
            const float lev = 20.0f * std::log10 (env + 1.0e-9f);
            const float over = lev - thr;
            float red = 0;
            if (over > knee * 0.5f) red = over * slope;
            else if (over > -knee * 0.5f) { const float k = over + knee * 0.5f; red = slope * k * k / (2.0f * knee); }
            const float g = std::pow (10.0f, -red / 20.0f) * makeup;
            L[i] *= g; R[i] *= g;
        }
        if (mix < 1.0f) dry.blend (L, R, n, mix);
    }
};

struct DriveProc final : FxProcessor
{
    kk::DriveStage st; double sr = 44100; DryBuf dry; kk::OnePole tone[2]; float lastTone = -1;
    int holdCount = 0; float holdL = 0, holdR = 0; float smAmt = 0;
    void prepare (double s, int maxBlock) override { sr = s; st.prepare ((float) s, maxBlock); dry.prepare (maxBlock); clear(); }
    void clear() override { st.clear(); tone[0].reset(); tone[1].reset(); holdCount = 0; holdL = holdR = 0; smAmt = 0; }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        // p: amount, type, tone, mix, cleanLow, output
        const float amount = p[0], mix = juce::jlimit (0.0f, 1.0f, p[3]);
        const int type = juce::jlimit (0, 5, (int) p[1]);
        if (amount <= 0.001f && st.smDrive <= 0.001f && smAmt <= 0.001f) return;
        if (mix < 1.0f) dry.keep (L, R, n);
        if (type == 5)
        {
            // bit: sample-rate + bit-depth reduction (the EVOLVE lo-fi crusher), amount smoothed per block
            smAmt += (amount - smAmt) * std::min (1.0f, smoothCoef (sr) * (float) n);
            const float levels = std::exp2 (14.0f - smAmt * 11.0f);
            const int hold = 1 + (int) (smAmt * smAmt * 12.0f);
            for (int i = 0; i < n; ++i)
            {
                if (++holdCount >= hold) { holdCount = 0; holdL = L[i]; holdR = R[i]; }
                L[i] = std::round (holdL * levels) / levels; R[i] = std::round (holdR * levels) / levels;
            }
        }
        else
        {
            smAmt = 0;
            st.process (L, R, n, amount, type, p[4] > 0.5f, false, smoothCoef (sr));
        }
        if (p[2] < 19000.0f)
        {
            if (p[2] != lastTone) { lastTone = p[2]; for (auto& t : tone) t.setHz (std::min (p[2], (float) sr * 0.45f), (float) sr); }
            for (int i = 0; i < n; ++i) { L[i] = tone[0].lp (L[i]); R[i] = tone[1].lp (R[i]); }
        }
        if (std::abs (p[5]) > 0.01f)
        {
            const float g = juce::Decibels::decibelsToGain (p[5]);
            for (int i = 0; i < n; ++i) { L[i] *= g; R[i] *= g; }
        }
        if (mix < 1.0f) dry.blend (L, R, n, mix);
    }
};

struct LofiProc final : FxProcessor
{
    kk::LofiStage st; DryBuf dry;
    void prepare (double sr, int maxBlock) override { st.prepare ((float) sr); dry.prepare (maxBlock); }
    void clear() override { st.clear(); }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        const float mix = juce::jlimit (0.0f, 1.0f, p[2]);
        if (mix < 1.0f) dry.keep (L, R, n);
        st.process (L, R, n, p[0], p[1]);
        if (mix < 1.0f) dry.blend (L, R, n, mix);
    }
};

struct ChorusProc final : FxProcessor
{
    kk::ChorusStage st; double sr = 44100;
    void prepare (double s, int) override { sr = s; st.prepare ((float) s); }
    void clear() override { st.clear(); }
    bool alwaysRun() const override { return true; }   // keeps its lines filled (FxRack did too)
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        st.process (L, R, n, p[0], smoothCoef (sr), p[1], 0.008f * p[2], p[3] > 0.5f);
    }
};

struct PhaserProc final : FxProcessor
{
    kk::PhaserStage st; double sr = 44100;
    void prepare (double s, int) override { sr = s; st.prepare ((float) s); }
    void clear() override { st.clear(); }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        st.process (L, R, n, p[0], smoothCoef (sr), p[1], 3.5f * p[2], p[3], p[4]);
    }
};

struct FlangerProc final : FxProcessor
{
    kk::FlangerStage st; double sr = 44100;
    void prepare (double s, int) override { sr = s; st.prepare ((float) s); }
    void clear() override { st.clear(); }
    bool alwaysRun() const override { return true; }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        st.process (L, R, n, p[0], smoothCoef (sr), p[1], 0.0045f * p[2], p[3]);
    }
};

// multimode SVF with an envelope follower and its own LFO (auto-wah / filter sweeps)
struct FilterProc final : FxProcessor
{
    double sr = 44100; kk::SvfState s[2]; kk::SvfCoef c; float env = 0, lfoPh = 0, smCut = -1; DryBuf dry;
    void prepare (double r, int maxBlock) override { sr = r; dry.prepare (maxBlock); clear(); }
    void clear() override { s[0].reset(); s[1].reset(); env = 0; smCut = -1; }
    void process (float* L, float* R, int n, const float* p, const FxContext& ctx) override
    {
        // p: cutoff, reso, mode, env, lfo, lfoRate, lfoSync, lfoDiv, mix
        const float mix = juce::jlimit (0.0f, 1.0f, p[8]);
        if (mix <= 0.0005f) return;
        if (mix < 1.0f) dry.keep (L, R, n);
        const int mode = juce::jlimit (0, 2, (int) p[2]);
        const float k = 2.0f - 1.9f * juce::jlimit (0.0f, 1.0f, p[1]);
        const float atk = 1.0f - std::exp (-1.0f / (0.004f * (float) sr)), rel = 1.0f - std::exp (-1.0f / (0.15f * (float) sr));
        const double beatsPerSample = ctx.bpm / 60.0 / sr;
        const float lfoInc = (float) (1.0 / sr) * p[5];
        const float cutSm = 1.0f - std::exp (-16.0f / (0.004f * (float) sr));
        for (int i0 = 0; i0 < n; i0 += 16)
        {
            const int len = std::min (16, n - i0);
            float ph;
            if (p[6] > 0.5f) { const double x = (ctx.beatPos + beatsPerSample * i0) / divisionBeats ((int) p[7]); ph = (float) (x - std::floor (x)); }
            else { ph = lfoPh; lfoPh += lfoInc * (float) len; lfoPh -= std::floor (lfoPh); }
            const float lfo = std::sin (kk::twoPi * ph);
            const float oct = p[3] * 4.0f * std::min (1.0f, env * 4.0f) + p[4] * 4.0f * lfo * 0.5f;
            const float target = std::log2 (juce::jlimit (20.0f, 20000.0f, p[0])) + oct;
            smCut = smCut < 0 ? target : smCut + (target - smCut) * cutSm;
            c.set (juce::jlimit (20.0f, (float) sr * 0.45f, std::exp2 (smCut)), k, (float) sr);
            for (int i = i0; i < i0 + len; ++i)
            {
                const float x = std::max (std::abs (L[i]), std::abs (R[i]));
                env += (x - env) * (x > env ? atk : rel);
                s[0].tick (c, L[i]); s[1].tick (c, R[i]);
                L[i] = mode == 0 ? s[0].lp : mode == 1 ? s[0].hp : s[0].bp;
                R[i] = mode == 0 ? s[1].lp : mode == 1 ? s[1].hp : s[1].bp;
            }
        }
        if (mix < 1.0f) dry.blend (L, R, n, mix);
    }
};

struct DelayProc final : FxProcessor
{
    kk::DelayStage st; double sr = 44100; float lastHi = -1, lastLo = -1, wob = 0;
    void prepare (double s, int) override { sr = s; st.prepare ((float) s); lastHi = lastLo = -1; wob = 0; }
    void clear() override { st.clear(); }
    bool alwaysRun() const override { return true; }   // FxRack ran its delay all the time (echoes of what came before)
    void process (float* L, float* R, int n, const float* p, const FxContext& ctx) override
    {
        // p: mix, time, feedback, mode, sync, ms, hiCut, loCut, flutter
        if (p[6] != lastHi || p[7] != lastLo) { lastHi = p[6]; lastLo = p[7]; st.setFeedbackFilters (p[6], p[7]); }
        const double secs = std::min (4.3, p[4] > 0.5f ? divisionBeats ((int) p[1]) * 60.0 / ctx.bpm : (double) p[5] * 0.001);
        float depth = 0.004f;
        if (p[8] > 0.0005f) { wob += (float) n * 0.55f / (float) sr; wob -= std::floor (wob); depth = 0.004f + 0.012f * p[8]; }
        else wob = 0;
        st.process (L, R, n, p[0], juce::jlimit (0.0f, 0.95f, p[2]), secs, juce::jlimit (0, 2, (int) p[3]), wob, smoothCoef (sr), depth);
    }
};

struct ReverbProc final : FxProcessor
{
    kk::FdnReverb fdn;
    void prepare (double s, int) override { fdn.prepare (s); }
    void clear() override { fdn.reset(); }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        // p: mix, size, type, decay, predelay, damping, lowCut, shimmer, freeze
        const int type = juce::jlimit (0, 4, (int) p[2]);
        if (type <= 2 && p[3] == 1.0f && p[4] == 0.0f && p[5] == 0.0f && p[6] == 160.0f)
            fdn.process (L, R, n, type, p[1], p[0], p[8] > 0.5f);   // exactly the EVOLVE / v2 reverb
        else
        {
            kk::FdnReverb::Extra ex;
            ex.decayMul = p[3]; ex.preMs = p[4]; ex.dampMul = std::exp2 (-p[5] * 1.5f); ex.lowCutHz = p[6]; ex.shimmer = p[7];
            fdn.process (L, R, n, type, p[1], p[0], p[8] > 0.5f, ex);
        }
    }
};

struct WidthProc final : FxProcessor
{
    double sr = 44100; kk::OnePole lp; float lastHz = -1;
    void prepare (double s, int) override { sr = s; clear(); }
    void clear() override { lp.reset(); }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        const float sideGain = p[0] * 2.0f;
        const bool mono = p[1] > 1.0f;
        if (mono && p[1] != lastHz) { lastHz = p[1]; lp.setHz (p[1], (float) sr); }
        for (int i = 0; i < n; ++i)
        {
            const float m = (L[i] + R[i]) * 0.5f;
            float s = (L[i] - R[i]) * 0.5f * sideGain;
            if (mono) s -= lp.lp (s);
            L[i] = m + s; R[i] = m - s;
        }
    }
};

struct ReverseProc final : FxProcessor
{
    kk::ReverseStage st; double sr = 44100;
    void prepare (double s, int) override { sr = s; st.prepare ((float) s); }
    void clear() override { st.clear(); }
    bool alwaysRun() const override { return true; }
    void process (float* L, float* R, int n, const float* p, const FxContext& ctx) override
    {
        const double beats = std::min (divisionBeats ((int) p[1]), 0.65 * ctx.bpm / 60.0);   // the grain (x2) fits the 1.5 s ring
        st.process (L, R, n, p[0], ctx.bpm, smoothCoef (sr), beats);
    }
};

struct TransientProc final : FxProcessor
{
    kk::PunchStage st; double sr = 44100;
    void prepare (double s, int) override { sr = s; st.prepare ((float) s); }
    void clear() override { st.clear(); }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override { st.process (L, R, n, p[0], smoothCoef (sr)); }
};

struct BodyProc final : FxProcessor
{
    kk::BodyStage st; double sr = 44100;
    void prepare (double s, int) override { sr = s; st.prepare ((float) s); }
    void clear() override { st.clear(); }
    void process (float* L, float* R, int n, const float* p, const FxContext&) override
    {
        st.process (L, R, n, juce::jlimit (1, 6, (int) p[0]), p[1], smoothCoef (sr));
    }
};

struct GhostProc final : FxProcessor
{
    std::unique_ptr<kk::GhostStage> st = std::make_unique<kk::GhostStage>();
    std::vector<float> zeros;
    void prepare (double s, int maxBlock) override { st->prepare (s); zeros.assign ((size_t) std::max (maxBlock, 1), 0.0f); }
    void clear() override { st->clear(); }
    void process (float* L, float* R, int n, const float* p, const FxContext& ctx) override
    {
        st->process (L, R, ctx.ghostSend != nullptr ? ctx.ghostSend : zeros.data(), std::min (n, (int) kk::GhostStage::maxBlock), p[0], p[1], p[2] > 0.5f, ctx.bpm);
    }
};
struct StutterProc final : FxProcessor
{
    kk::CircuitStage st;
    void prepare (double s, int) override { st.prepare ((float) s); }
    void clear() override { st.clear(); }
    void process (float* L, float* R, int n, const float* p, const FxContext& ctx) override
    {
        st.process (L, R, n, p[0], divisionBeats ((int) p[1]), (uint32_t) p[2], ctx.bpm, ctx.beatPos);
    }
};

struct HalftimeProc final : FxProcessor
{
    kk::HalftimeStage st; double sr = 44100;
    void prepare (double s, int) override { sr = s; st.prepare ((float) s); }
    void clear() override { st.clear(); }
    void process (float* L, float* R, int n, const float* p, const FxContext& ctx) override { st.process (L, R, n, p[0], ctx.bpm, ctx.beatPos, smoothCoef (sr)); }
};
} // namespace

std::unique_ptr<FxProcessor> FxProcessor::create (FxType t)
{
    switch (t)
    {
        case FxType::eq:         return std::make_unique<EqProc>();
        case FxType::compressor: return std::make_unique<CompProc>();
        case FxType::drive:      return std::make_unique<DriveProc>();
        case FxType::lofi:       return std::make_unique<LofiProc>();
        case FxType::chorus:     return std::make_unique<ChorusProc>();
        case FxType::phaser:     return std::make_unique<PhaserProc>();
        case FxType::flanger:    return std::make_unique<FlangerProc>();
        case FxType::filter:     return std::make_unique<FilterProc>();
        case FxType::delay:      return std::make_unique<DelayProc>();
        case FxType::reverb:     return std::make_unique<ReverbProc>();
        case FxType::width:      return std::make_unique<WidthProc>();
        case FxType::reverse:    return std::make_unique<ReverseProc>();
        case FxType::transient:  return std::make_unique<TransientProc>();
        case FxType::body:       return std::make_unique<BodyProc>();
        case FxType::ghost:      return std::make_unique<GhostProc>();
        case FxType::stutter:    return std::make_unique<StutterProc>();
        case FxType::halftime:   return std::make_unique<HalftimeProc>();
        case FxType::numTypes:   break;
    }
    return std::make_unique<EqProc>();
}

//==============================================================================
void FxChain::prepare (double sampleRate, int maxBlockSize)
{
    sr = sampleRate;
    maxBlock = std::max (1, maxBlockSize);
    for (int t = 0; t < numFxTypes; ++t)
        for (int k = 0; k < maxSameFxType; ++k)
        {
            auto& p = pool[(size_t) (t * maxSameFxType + k)];
            if (p == nullptr) p = FxProcessor::create ((FxType) t);
            p->prepare (sr, maxBlock);
        }
    dryL.assign ((size_t) maxBlock, 0.0f); dryR.assign ((size_t) maxBlock, 0.0f);
    xfStep = (float) (1.0 / (0.010 * sr / 1.0));
    mastering.prepare (sampleRate);
    for (auto& f : sideLp) { f.setHz (120.0f, (float) sr); f.reset(); }
    for (auto& d : dc) { d.setHz (8.0f, (float) sr); d.x1 = d.y1 = 0; }
    {   // 2nd-order Butterworth high-pass at 22 Hz for bass mode (as FxRack)
        const float w = kk::twoPi * 22.0f / (float) sr, cs = std::cos (w), al = std::sin (w) / std::sqrt (2.0f), a0 = 1.0f + al;
        for (auto& h : subsonic) { h.b0 = (1 + cs) / 2 / a0; h.b1 = -(1 + cs) / a0; h.b2 = (1 + cs) / 2 / a0; h.a1 = -2 * cs / a0; h.a2 = (1 - al) / a0; h.z1 = h.z2 = 0; }
    }
    numEntries = 0;
    used.fill (false);
}

void FxChain::clear()
{
    for (auto& p : pool) if (p != nullptr) p->clear();
    mastering.reset();
    for (auto& f : sideLp) f.reset();
    for (auto& d : dc) d.x1 = d.y1 = 0;
    for (auto& b : subsonic) b.z1 = b.z2 = 0;
    for (int i = 0; i < numEntries; ++i) gains[(size_t) i] = entries[(size_t) i].on ? 1.0f : 0.0f;
}

void FxChain::setLayout (const Entry* e, int count)
{
    count = juce::jlimit (0, maxEntries, count);
    std::array<bool, (size_t) numFxTypes * maxSameFxType> nowUsed {};
    std::array<float, maxEntries> newGains {};
    std::array<int, maxEntries> newInst {};
    int perType[numFxTypes] {};
    for (int i = 0; i < count; ++i)
    {
        const int t = juce::jlimit (0, numFxTypes - 1, (int) e[i].type);
        const int k = std::min (perType[t]++, maxSameFxType - 1);
        newInst[(size_t) i] = t * maxSameFxType + k;
        // the same processor in the same role keeps its state and crossfade position
        float g = e[i].on ? 1.0f : 0.0f;
        for (int j = 0; j < numEntries; ++j)
            if (inst[(size_t) j] == newInst[(size_t) i]) { g = gains[(size_t) j]; break; }
        newGains[(size_t) i] = g;
        if (! used[(size_t) newInst[(size_t) i]] && ! nowUsed[(size_t) newInst[(size_t) i]]) pool[(size_t) newInst[(size_t) i]]->clear();
        nowUsed[(size_t) newInst[(size_t) i]] = true;
    }
    for (int i = 0; i < count; ++i) { entries[(size_t) i] = e[i]; inst[(size_t) i] = newInst[(size_t) i]; gains[(size_t) i] = newGains[(size_t) i]; }
    numEntries = count;
    used = nowUsed;
}

FxProcessor* FxChain::procFor (int i) const { return pool[(size_t) inst[(size_t) i]].get(); }

void FxChain::process (float* L, float* R, int n, const FxContext& ctx, bool allOn, const Output& out)
{
    n = std::min (n, maxBlock);
    // the v2 rack applied its width in the output loop: a trailing Width slot without mono-below (fully on) is fused into it,
    // so a converted preset gets exactly the old arithmetic
    float sideGain = 1.0f;
    int last = numEntries;
    if (numEntries > 0)
    {
        const auto& e = entries[(size_t) numEntries - 1];
        const bool on = e.on && allOn;
        if (e.type == FxType::width && e.p[1] <= 1.0f && on && gains[(size_t) numEntries - 1] >= 1.0f)
        {
            sideGain = e.p[0] * 2.0f;
            last = numEntries - 1;
        }
    }
    for (int i = 0; i < last; ++i)
    {
        auto& e = entries[(size_t) i];
        auto* proc = procFor (i);
        const bool on = e.on && allOn;
        float& g = gains[(size_t) i];
        if (! on && g <= 0.0f) continue;                            // bypassed
        if (on && g >= 1.0f) { proc->process (L, R, n, e.p, ctx); continue; }
        // crossfade: dry + (wet - dry) * g, g ramps over 10 ms
        if (g <= 0.0f) proc->clear();                               // switched on again: start clean (no stale tail)
        std::copy (L, L + n, dryL.data()); std::copy (R, R + n, dryR.data());
        proc->process (L, R, n, e.p, ctx);
        for (int s = 0; s < n; ++s)
        {
            g = on ? std::min (1.0f, g + xfStep) : std::max (0.0f, g - xfStep);
            L[s] = dryL[(size_t) s] + (L[s] - dryL[(size_t) s]) * g;
            R[s] = dryR[(size_t) s] + (R[s] - dryR[(size_t) s]) * g;
        }
    }
    if (last < numEntries) gains[(size_t) last] = 1.0f;
    // output stage (FxRack's tail): width, mono lows, DC, output gain, finish + its peak limiter
    for (int i = 0; i < n; ++i)
    {
        const float m = (L[i] + R[i]) * 0.5f;
        float s = (L[i] - R[i]) * 0.5f * sideGain;
        if (out.monoLows) s -= sideLp[0].lp (s);
        float l = dc[0].tick ((m + s) * out.outGain), r = dc[1].tick ((m - s) * out.outGain);
        if (out.monoLows) { l = subsonic[0].tick (l); r = subsonic[1].tick (r); }
        L[i] = l; R[i] = r;
    }
    mastering.process (L, R, n, out.finish, out.monoLows ? -1.0f : out.width);
}
} // namespace vault
