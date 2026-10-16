#include "VaultPages.h"
#include "VaultValues.h"
#include "../../ParamKeys.h"

// CHROME VAULT v3 LAYERS page: one card per layer (level / pan / octave / fine, amp envelope, filter - or the model's own
// parameters for a modelled layer) and the VOICE row (poly / mono / legato, glide, unison, detune, velocity, bend).
namespace vui
{
namespace
{
constexpr int voiceH = 84, gap = 12;

// one knob bound to a value of a layer: the part's own fields (level / pan / octave / semi) or a key of `values`
struct Binding
{
    juce::String key;
    float lo = 0, hi = 1, def = 0;
    float skew = 1;            // v = lo + (hi - lo) * n^skew
    bool log = false;
    int steps = 0;
    std::function<juce::String (float)> text;
    juce::String target;       // modulation target ("" = none)
    bool enabled = true;
    bool eff = false;          // the knob shows / sets the effective value (vui::val: sound macros, MAIN knobs, EVOLVE macros)
    val::LKey lkey = val::LKey::level;

    float get (const VaultPart& p) const
    {
        if (key == "level") return p.level;
        if (key == "pan") return p.pan;
        if (key == "octave") return (float) p.octave;
        if (key == "semi") return (float) p.semi;
        return p.get (key, def);
    }
    void set (VaultPart& p, float v) const
    {
        v = juce::jlimit (lo, hi, v);
        if (key == "level") p.level = v;
        else if (key == "pan") p.pan = v;
        else if (key == "octave") p.octave = juce::roundToInt (v);
        else if (key == "semi") p.semi = juce::roundToInt (v);
        else p.set (key, steps > 1 ? std::round (v) : v);
    }
    float toNorm (float v) const
    {
        if (hi <= lo) return 0.0f;
        v = juce::jlimit (lo, hi, v);
        if (log && lo > 0) return std::log (v / lo) / std::log (hi / lo);
        const float n = (v - lo) / (hi - lo);
        return std::abs (skew - 1.0f) > 1.0e-6f ? std::pow (n, 1.0f / skew) : n;
    }
    float fromNorm (float n) const
    {
        n = juce::jlimit (0.0f, 1.0f, n);
        float v = log && lo > 0 ? lo * std::pow (hi / lo, n) : lo + (hi - lo) * (std::abs (skew - 1.0f) > 1.0e-6f ? std::pow (n, skew) : n);
        if (steps > 1) v = std::round (v);
        return juce::jlimit (lo, hi, v);
    }
};

float synthDef (const char* id)
{
    const int k = kk::vk::keyOf (id);
    return k >= 0 ? kk::vk::info (k).def : 0.0f;
}
juce::String panText (float v)
{
    const int p = juce::roundToInt (v * 100.0f);
    return p == 0 ? juce::String ("C") : (p < 0 ? "L " : "R ") + juce::String (std::abs (p));
}
juce::String centsText (float v) { return (v > 0.5f ? "+" : "") + juce::String (juce::roundToInt (v)) + " ct"; }
juce::String octText (float v) { const int o = juce::roundToInt (v); return (o > 0 ? "+" : "") + juce::String (o); }

Binding b (const char* key, float lo, float hi, float def, std::function<juce::String (float)> text, const juce::String& target = {})
{
    Binding x; x.key = key; x.lo = lo; x.hi = hi; x.def = def; x.text = std::move (text); x.target = target; return x;
}
Binding timeB (const char* key, float lo, float hi, float def, const juce::String& zeroText, const juce::String& target = {})
{
    auto x = b (key, lo, hi, def, [zeroText] (float v) { return v <= 0.0f && zeroText.isNotEmpty() ? zeroText : secondsText (v); }, target);
    x.skew = 3.0f;
    return x;
}
// a model parameter's knob label (+ the short form for narrow cards)
std::pair<juce::String, juce::String> modelLabel (const juce::String& id)
{
    static const std::map<juce::String, std::pair<const char*, const char*>> m {
        { "attack", { "ATTACK", "ATK" } }, { "release", { "RELEASE", "REL" } }, { "formant", { "FORMANT", "FORM" } },
        { "pickPos", { "PICK POS", "PICK" } }, { "tremRate", { "TREM RATE", "T-RATE" } }, { "velSens", { "VEL SENS", "VEL" } },
        { "vibRate", { "VIB RATE", "V-RATE" } }, { "vibrato", { "VIBRATO", "VIB" } }, { "voicing", { "VOICING", "VOICE" } },
        { "breath", { "BREATH", "BRTH" } }, { "bright", { "BRIGHT", "BRT" } }, { "detune", { "DETUNE", "DET" } },
        { "spread", { "SPREAD", "SPRD" } }, { "motion", { "MOTION", "MOT" } }, { "pickup", { "PICKUP", "PU" } },
        { "gender", { "GENDER", "GEND" } }, { "voices", { "VOICES", "VOIC" } } };
    const auto key = id.fromFirstOccurrenceOf (".", false, false);
    if (const auto it = m.find (key); it != m.end()) return { it->second.first, it->second.second };
    return { key.toUpperCase(), {} };
}
Binding effB (Binding x, val::LKey k) { x.eff = true; x.lkey = k; return x; }
Binding disabled() { Binding x; x.enabled = false; x.text = [] (float) { return juce::String (juce::CharPointer_UTF8 ("\xe2\x80\x94")); }; return x; }
} // namespace

//==============================================================================
struct LayersPage::Card : public juce::Component
{
    Card (LayersPage& o, UiContext& c, int idx) : owner (o), ctx (c), index (idx)
    {
        noFocus (*this);
        addAndMakeVisible (power);
        power.onClick = [this] { togglePower(); };
        for (auto* k : { &level, &pan, &octave, &fine, &a, &d, &s, &r, &cutoff, &reso, &env, &key }) addAndMakeVisible (*k);
        level.label = "LEVEL"; pan.label = "PAN"; octave.label = "OCTAVE"; octave.shortLabel = "OCT"; fine.label = "FINE";
        a.label = "A"; d.label = "D"; s.label = "S"; r.label = "R";
        cutoff.label = "CUTOFF"; cutoff.shortLabel = "CUT"; reso.label = "RESO"; env.label = "ENV"; key.label = "KEY";
        for (auto* k : { &level, &pan, &octave, &fine }) k->diameter = 38;
        for (auto* k : { &a, &d, &s, &r, &cutoff, &reso, &env, &key }) k->diameter = 34;
        addAndMakeVisible (filterSeg);
        filterSeg.onSelect = [this] (int i) { setFilter (i); };
        bind();
    }

    void bind()
    {
        const auto& snd = ctx.sound();
        if (! juce::isPositiveAndBelow (index, (int) snd.parts.size())) return;
        const auto& part = snd.parts[(size_t) index];
        kind = part.kind;
        const juce::String L = "layer" + juce::String (index + 1) + ".";
        bindings.clear();
        auto add = [&] (Knob& k, Binding bb) { bindings.push_back ({ &k, std::move (bb) }); };
        auto lvl = b ("level", 0, 2, 1, [] (float v) { return pct (v); }, L + "level");
        add (level, effB (lvl, val::LKey::level));
        auto pn = b ("pan", -1, 1, 0, panText, L + "pan");
        add (pan, effB (pn, val::LKey::pan));
        auto oc = b ("octave", -3, 3, 0, octText); oc.steps = 7;
        add (octave, oc);
        modelKnobs.clear();
        if (kind == VaultPartKind::synth)
        {
            fine.label = "FINE";
            add (fine, b ("fine", -50, 50, 0, centsText, L + "tune"));
            add (a, effB (timeB ("attack", 0.001f, 10, synthDef ("attack"), {}, L + "attack"), val::LKey::attack));
            add (d, timeB ("decay", 0.005f, 8, synthDef ("decay"), {}));
            add (s, b ("sustain", 0, 1, synthDef ("sustain"), [] (float v) { return pct (v); }));
            add (r, effB (timeB ("release", 0.005f, 12, synthDef ("release"), {}, L + "release"), val::LKey::release));
            auto co = b ("cutoff", 40, 20000, synthDef ("cutoff"), [] (float v) { return hzText (v); }, L + "cutoff"); co.log = true;
            add (cutoff, effB (co, val::LKey::cutoff));
            add (reso, effB (b ("reso", 0, 1, synthDef ("reso"), [] (float v) { return pct (v); }, L + "reso"), val::LKey::reso));
            env.label = "ENV";
            add (env, effB (b ("fenv", -1, 1, synthDef ("fenv"), [] (float v) { return signedPct (v); }), val::LKey::fenv));
            key.label = "KEY";
            add (key, b ("keyTrack", 0, 1, synthDef ("keyTrack"), [] (float v) { return pct (v); }));
            filterSeg.setOptions ({ "LP 12", "LP 24", "HP", "BP" });
        }
        else if (kind == VaultPartKind::sample)
        {
            fine.label = "FINE";
            add (fine, b ("s.tune", -100, 100, 0, centsText, L + "tune"));
            add (a, effB (timeB ("s.attack", 0, 10, 0, "sample", L + "attack"), val::LKey::attack));
            add (d, timeB ("s.decay", 0, 30, 0, "natural"));
            add (s, b ("s.sustain", 0, 1, 1, [] (float v) { return pct (v); }));
            add (r, effB (timeB ("s.release", 0, 20, 0, "sample", L + "release"), val::LKey::release));
            auto co = b ("s.cutoff", 20, 20000, 20000, [] (float v) { return v >= 19950.0f ? juce::String ("open") : hzText (v); }, L + "cutoff"); co.log = true;
            add (cutoff, effB (co, val::LKey::cutoff));
            add (reso, effB (b ("s.reso", 0, 1, 0, [] (float v) { return pct (v); }, L + "reso"), val::LKey::reso));
            env.label = "VEL";
            add (env, b ("s.velCutoff", 0, 1, 0, [] (float v) { return pct (v); }));
            key.label = "KEY";
            add (key, disabled());
            filterSeg.setOptions ({ "LP", "HP", "BP" });
        }
        else
        {
            fine.label = "SEMI";
            auto sm = b ("semi", -12, 12, 0, [] (float v) { const int x = juce::roundToInt (v); return (x > 0 ? "+" : "") + juce::String (x) + " st"; });
            sm.steps = 25;
            add (fine, sm);
            for (auto* k : { &a, &d, &s, &r, &cutoff, &reso, &env, &key }) k->setVisible (false);
            filterSeg.setVisible (false);
            if (const auto* mi = vault::models::ModelRegistry::get().find (part.source))
            {
                modelName = mi->name;
                const int n = std::min (8, (int) mi->params.size());
                for (int i = 0; i < n; ++i)
                {
                    const auto& pi = mi->params[(size_t) i];
                    const auto lbl = modelLabel (pi.id);
                    auto k = std::make_unique<Knob> (lbl.first, 34);
                    k->shortLabel = lbl.second;
                    addAndMakeVisible (*k);
                    Binding mb = b (pi.id, pi.lo, pi.hi, pi.def, nullptr);
                    const float lo = pi.lo, hi = pi.hi;
                    mb.text = [lo, hi] (float v) { return lo >= 0 && hi <= 1 ? pct (v) : juce::String (v, std::abs (v) < 10 ? 2 : 0); };
                    k->setTooltip (pi.what);
                    bindings.push_back ({ k.get(), mb });
                    modelKnobs.push_back (std::move (k));
                }
                moreParams = (int) mi->params.size() - n;
            }
        }
        if (kind != VaultPartKind::model)
        {
            for (auto* k : { &a, &d, &s, &r, &cutoff, &reso, &env, &key }) k->setVisible (true);
            filterSeg.setVisible (true);
        }
        for (auto& [k, bb] : bindings)
        {
            Knob* kp = k;
            const Binding bc = bb;
            kp->valueText = [kp, bc] (float n) { return bc.text ? bc.text (bc.fromNorm (n)) : pct (n); };
            kp->steps = bc.steps;
            kp->arcOrigin = bc.lo < 0 ? 0.5f : 0.0f;
            kp->setEnabledLook (bc.enabled);
            const int idx = index;
            kp->onChange = [this, bc, idx] (float n)
            {
                if (bc.eff)
                {
                    // the effective value: through the sound macros (a macro that sets the level gets its range moved) and
                    // the MAIN knobs (they stay where they are)
                    const auto c = val::Ctx::of (ctx.proc);
                    ctx.edit ([&] (VaultPreset& p) { val::setLayerValue (p, c, idx, bc.lkey, bc.fromNorm (n)); });
                    return;
                }
                ctx.edit ([&] (VaultPreset& p) { if (juce::isPositiveAndBelow (idx, (int) p.parts.size())) bc.set (p.parts[(size_t) idx], bc.fromNorm (n)); });
            };
            kp->onReset = [this, bc] { resetBinding (bc); };
            kp->onRightClick = [this, kp, bc] (const juce::MouseEvent&) { if (bc.target.isNotEmpty()) mod::showMenu (ctx, *kp, bc.target); };
        }
        resized();
        refresh();
    }

    void refresh()
    {
        const auto& snd = ctx.sound();
        if (! juce::isPositiveAndBelow (index, (int) snd.parts.size())) return;
        const auto& part = snd.parts[(size_t) index];
        const auto& bank = ctx.bankSound();
        const VaultPart* bp = juce::isPositiveAndBelow (index, (int) bank.parts.size()) ? &bank.parts[(size_t) index] : nullptr;
        const auto c = val::Ctx::of (ctx.proc);
        for (auto& [k, bb] : bindings)
        {
            if (! bb.enabled) { k->setValue (0.0f); continue; }
            k->setValue (bb.toNorm (bb.eff ? val::layerValue (snd, c, index, bb.lkey) : bb.get (part)));
            k->resetValue = bb.toNorm (bp != nullptr ? (bb.eff ? val::layerValue (bank, c, index, bb.lkey) : bb.get (*bp)) : bb.def);
            mod::show (*k, snd, bb.target);
        }
        power.setToggleState (val::layerValue (snd, c, index, val::LKey::level) > 0.0001f, juce::dontSendNotification);
        if (kind == VaultPartKind::synth)
        {
            const int ft = juce::roundToInt (part.get ("filterType", 0));
            filterSeg.setSelected (ft == 0 ? 0 : ft == 1 ? 1 : ft == 3 ? 2 : ft == 4 ? 3 : -1);
        }
        else if (kind == VaultPartKind::sample) filterSeg.setSelected (juce::jlimit (0, 2, juce::roundToInt (part.get ("s.filterType", 0))));
        title = juce::String::charToString ((juce::juce_wchar) ('A' + index)) + upDot()
              + (kind == VaultPartKind::synth && part.label.isNotEmpty() ? part.label : VaultBrowserModel::instrumentName (part, ctx.proc.sampleLibrary()));
        repaint();
    }

    void setFilter (int i)
    {
        const int idx = index;
        const auto k = kind;
        ctx.edit ([idx, i, k] (VaultPreset& p)
        {
            if (! juce::isPositiveAndBelow (idx, (int) p.parts.size())) return;
            if (k == VaultPartKind::synth) p.parts[(size_t) idx].set ("filterType", (float) (i == 0 ? 0 : i == 1 ? 1 : i == 2 ? 3 : 4));
            else p.parts[(size_t) idx].set ("s.filterType", (float) i);
        });
    }

    // double-click on a knob: the bank preset's value (stored value + the ranges of the sound macros that set / move it)
    void resetBinding (const Binding& bb)
    {
        const auto& bank = ctx.bankSound();
        const int idx = index;
        if (! juce::isPositiveAndBelow (idx, (int) bank.parts.size())) return;
        const auto& bp = bank.parts[(size_t) idx];
        const juce::String layerT = "layer" + juce::String (idx + 1) + "." + bb.target.fromFirstOccurrenceOf (".", false, false);
        ctx.edit ([&] (VaultPreset& p)
        {
            if (! juce::isPositiveAndBelow (idx, (int) p.parts.size()) || p.parts[(size_t) idx].kind != bp.kind) return;
            bb.set (p.parts[(size_t) idx], bb.get (bp));
            if (bb.target.isEmpty()) return;
            for (int m = 0; m < vault::numSoundMacros; ++m)
            {
                auto& t = p.macros[(size_t) m].targets;
                const auto& bt = bank.macros[(size_t) m].targets;
                for (int i = 0; i < (int) t.size() && i < (int) bt.size(); ++i)
                    if (t[(size_t) i].target == bt[(size_t) i].target && t[(size_t) i].target == layerT) t[(size_t) i] = bt[(size_t) i];
            }
        });
    }

    void togglePower()
    {
        // off = the level really 0 (also when a sound macro sets the level: its range goes to 0), on = back as it was
        const int idx = index;
        const auto mkey = ctx.sound().id + ":" + juce::String (idx);
        auto& muted = owner.muted;
        const auto c = val::Ctx::of (ctx.proc);
        const bool on = val::layerValue (ctx.sound(), c, idx, val::LKey::level) > 0.0001f;
        const juce::String levelT = "layer" + juce::String (idx + 1) + ".level";
        ctx.edit ([&] (VaultPreset& p)
        {
            if (! juce::isPositiveAndBelow (idx, (int) p.parts.size())) return;
            auto& lv = p.parts[(size_t) idx].level;
            if (on)
            {
                LayersPage::MuteMemo memo;
                memo.level = lv;
                for (int m = 0; m < vault::numSoundMacros; ++m)
                    for (int i = 0; i < (int) p.macros[(size_t) m].targets.size(); ++i)
                        if (auto& t = p.macros[(size_t) m].targets[(size_t) i]; t.target == levelT)
                        {
                            memo.targets.push_back ({ m, i, t.min, t.max });
                            t.min = t.max = 0.0f;
                        }
                muted[mkey] = memo;
                lv = 0.0f;
                return;
            }
            if (const auto it = muted.find (mkey); it != muted.end())
            {
                lv = it->second.level > 0.0001f ? it->second.level : 1.0f;
                for (auto& [m, i, mn, mx] : it->second.targets)
                    if (juce::isPositiveAndBelow (i, (int) p.macros[(size_t) m].targets.size()) && p.macros[(size_t) m].targets[(size_t) i].target == levelT)
                    { p.macros[(size_t) m].targets[(size_t) i].min = mn; p.macros[(size_t) m].targets[(size_t) i].max = mx; }
                muted.erase (it);
            }
            else
            {
                lv = std::max (lv, 1.0f);
                for (auto& mac : p.macros)
                    for (auto& t : mac.targets)
                        if (t.target == levelT && t.min <= 0.0001f && t.max <= 0.0001f) t.min = t.max = 1.0f;
            }
        });
    }

    void resized() override
    {
        const int w = getWidth();
        power.setBounds (14, 14, 20, 20);
        auto row = [&] (std::initializer_list<Knob*> ks, int y)
        {
            const int cw = (w - 20) / 4;
            int i = 0;
            for (auto* k : ks) { k->setBounds (10 + i * cw, y, cw, k->idealHeight()); ++i; }
        };
        row ({ &level, &pan, &octave, &fine }, 50);
        row ({ &a, &d, &s, &r }, 244);
        filterSeg.setBounds (14, 352, std::min (w - 28, filterSeg.idealWidth()), 28);
        row ({ &cutoff, &reso, &env, &key }, 392);
        const int cw = (w - 20) / 4;
        for (int i = 0; i < (int) modelKnobs.size(); ++i)
            modelKnobs[(size_t) i]->setBounds (10 + (i % 4) * cw, 166 + (i / 4) * 90, cw, modelKnobs[(size_t) i]->idealHeight());
    }

    void paint (juce::Graphics& g) override
    {
        using namespace juce;
        drawPanel (g, getLocalBounds().toFloat());
        const auto& snd = ctx.sound();
        if (! juce::isPositiveAndBelow (index, (int) snd.parts.size())) return;
        const auto& part = snd.parts[(size_t) index];
        // header: title + what plays it
        const String kindText = kind == VaultPartKind::sample ? "sampled" : kind == VaultPartKind::model ? "modelled" : "synth";
        const auto pf = font (14.0f);
        const float pw = textWidth (pf, kindText) + 20.0f;
        auto pill = Rectangle<float> ((float) getWidth() - 14.0f - pw, 12.0f, pw, 24.0f);
        g.setColour (col::pillLine);
        g.drawRoundedRectangle (pill.reduced (0.5f), 6.0f, 1.0f);
        g.setFont (pf);
        g.setColour (col::dim);
        g.drawText (kindText, pill, Justification::centred, false);
        g.setFont (font (16.0f, true));
        g.setColour (part.level > 0.0001f ? col::text : col::dim);
        g.drawText (title, Rectangle<float> (42.0f, 10.0f, pill.getX() - 50.0f, 28.0f), Justification::centredLeft, true);

        const float W = (float) getWidth();
        if (kind == VaultPartKind::model)
        {
            drawHeader (g, "MODEL" + upDot() + modelName.toUpperCase(), { 14.0f, 140.0f, W - 28.0f, 18.0f });
            if (moreParams > 0)
            {
                g.setFont (font (14.0f));
                g.setColour (col::dim);
                g.drawText ("+ " + String (moreParams) + " more set by the sound designer", Rectangle<float> (14.0f, 352.0f, W - 28.0f, 20.0f), Justification::centredLeft, true);
            }
            return;
        }
        drawHeader (g, "AMP ENVELOPE", { 14.0f, 140.0f, W - 28.0f, 18.0f });
        // envelope graph
        auto gr = Rectangle<float> (14.0f, 164.0f, W - 28.0f, 70.0f);
        g.setColour (col::field);
        g.fillRoundedRectangle (gr, 8.0f);
        g.setColour (col::wellLine);
        g.drawRoundedRectangle (gr.reduced (0.5f), 8.0f, 1.0f);
        {
            float A, D, S, R;
            bool natural = false;
            const auto c = val::Ctx::of (ctx.proc);
            if (kind == VaultPartKind::synth)
            {
                A = val::layerValue (snd, c, index, val::LKey::attack); D = part.get ("decay", synthDef ("decay"));
                S = part.get ("sustain", synthDef ("sustain")); R = val::layerValue (snd, c, index, val::LKey::release);
            }
            else
            {
                A = val::layerValue (snd, c, index, val::LKey::attack); D = part.get ("s.decay", 0); S = part.get ("s.sustain", 1);
                R = val::layerValue (snd, c, index, val::LKey::release);
                if (D <= 0.0f) { natural = true; S = 1.0f; D = 0.3f; }
                if (R <= 0.0f) R = 0.4f;
            }
            auto tw = [] (float t) { return 0.08f + std::log1p (t * 8.0f) / std::log1p (80.0f); };
            const float wa = tw (A), wd = tw (D), wsus = 0.6f, wr = tw (R);
            const float total = wa + wd + wsus + wr;
            auto in = gr.reduced (10.0f, 8.0f);
            const float sx = in.getWidth() / total;
            float x = in.getX();
            const float y0 = in.getBottom(), y1 = in.getY(), ys = y1 + (1.0f - S) * in.getHeight();
            Path p;
            p.startNewSubPath (x, y0);
            x += wa * sx; p.lineTo (x, y1);
            x += wd * sx; p.lineTo (x, natural ? y1 + in.getHeight() * 0.12f : ys);
            x += wsus * sx; p.lineTo (x, natural ? y1 + in.getHeight() * 0.25f : ys);
            x += wr * sx; p.lineTo (x, y0);
            Path fill (p);
            fill.lineTo (in.getX(), y0);
            fill.closeSubPath();
            g.setColour (col::ice.withAlpha (0.10f));
            g.fillPath (fill);
            g.setColour (col::ice);
            g.strokePath (p, PathStrokeType (1.6f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        drawHeader (g, "FILTER", { 14.0f, 328.0f, W - 28.0f, 18.0f });
    }

    // right-click on the card: remove the last layer (a sound keeps at least one)
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! e.mods.isPopupMenu()) return;
        const int n = (int) ctx.sound().parts.size();
        juce::PopupMenu m;
        m.setLookAndFeel (&getLookAndFeel());
        m.addItem (1, "Remove this layer", n > 1 && index == n - 1);
        juce::Component::SafePointer<LayersPage> safe (&owner);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withMousePosition(), [safe] (int r) { if (safe != nullptr && r == 1) safe->removeLastLayer(); });
    }

    LayersPage& owner;
    UiContext& ctx;
    int index;
    VaultPartKind kind = VaultPartKind::synth;
    PowerButton power;
    Knob level, pan, octave, fine, a, d, s, r, cutoff, reso, env, key;
    Segments filterSeg;
    std::vector<std::unique_ptr<Knob>> modelKnobs;
    std::vector<std::pair<Knob*, Binding>> bindings;
    juce::String title, modelName;
    int moreParams = 0;
};

//==============================================================================
LayersPage::LayersPage (UiContext& c) : Page (c)
{
    const char* names[] { "GLIDE", "UNISON", "DETUNE", "VEL SENS", "BEND" };
    for (int i = 0; i < 5; ++i)
    {
        auto& k = voice[(size_t) i];
        k.label = names[i];
        k.diameter = 34;
        addAndMakeVisible (k);
    }
    voice[1].steps = 8;
    voice[4].steps = 25;
    voice[0].valueText = [] (float v) { return v < 0.004f ? juce::String ("off") : pct (v); };
    voice[1].valueText = [] (float v) { return juce::String (1 + juce::roundToInt (v * 7.0f)); };
    voice[2].valueText = [] (float v) { return pct (v); };
    voice[3].valueText = [] (float v) { return pct (v); };
    voice[4].valueText = [] (float v) { return juce::String (juce::CharPointer_UTF8 ("\xc2\xb1")) + juce::String (juce::roundToInt (v * 24.0f)); };
    // the VOICE row sets every layer it applies to (synth: all five, sampled: velocity + bend)
    auto setAll = [this] (int which, float n)
    {
        ctx.edit ([which, n] (VaultPreset& p)
        {
            for (auto& part : p.parts)
            {
                const bool synth = part.kind == VaultPartKind::synth, sample = part.kind == VaultPartKind::sample;
                switch (which)
                {
                    case 0: break;   // glide: below (the effective value, MAIN GLIDE included)
                    case 1: if (synth) part.set ("unison", (float) (1 + juce::roundToInt (n * 7.0f))); break;
                    case 2: if (synth) part.set ("detune", n); break;
                    case 3: if (synth) part.set ("velSens", n); else if (sample) part.set ("s.velSens", n); break;
                    case 4:
                        if (synth) part.set ("bendRange", (float) std::max (1, juce::roundToInt (n * 24.0f)));
                        else if (sample) part.set ("s.bendRange", (float) juce::roundToInt (n * 24.0f));
                        break;
                    default: break;
                }
            }
        });
    };
    for (int i = 1; i < 5; ++i) voice[(size_t) i].onChange = [setAll, i] (float n) { setAll (i, n); };
    voice[0].onChange = [this] (float n)
    {
        const auto c = val::Ctx::of (ctx.proc);
        ctx.edit ([&] (VaultPreset& p)
        {
            for (int k = 0; k < (int) p.parts.size(); ++k)
                if (p.parts[(size_t) k].kind == VaultPartKind::synth) val::setLayerValue (p, c, k, val::LKey::glide, n);
        });
    };
    addAndMakeVisible (voiceMode);
    voiceMode.upper = true;
    voiceMode.onSelect = [this] (int i)
    {
        ctx.edit ([i] (VaultPreset& p)
        {
            for (auto& part : p.parts)
                if (part.kind == VaultPartKind::synth) { part.set ("mono", i == 0 ? 0.0f : 1.0f); part.set ("legato", i == 2 ? 1.0f : 0.0f); }
        });
    };
    addAndMakeVisible (resetBtn);
    resetBtn.onClick = [this]
    {
        ctx.proc.revertAll();
        if (ctx.soundEdited) ctx.soundEdited();
        ctx.say ("Back to the sound as designed");
    };
    resetBtn.setTooltip ("Every edit of this sound (layers, FX, MAIN knobs, macros) back to the preset");
    rebuild();
}

LayersPage::~LayersPage() = default;

//==============================================================================
// an empty column: "+ Add layer" (a copy of layer A - as it is, an octave up or down); a sound with fewer than 3 layers
// never leaves half the page empty
struct LayersPage::AddCard : public juce::Component
{
    AddCard (LayersPage& o, UiContext& c) : owner (o), ctx (c)
    {
        noFocus (*this);
        addAndMakeVisible (btn);
        btn.onClick = [this] { menu(); };
        btn.setTooltip ("Add a layer to this sound: a copy of layer A to change (another octave, envelope, filter...)");
    }
    void resized() override
    {
        btn.setBounds (getLocalBounds().withSizeKeepingCentre (std::min (getWidth() - 40, btn.idealWidth() + 14), 34));
    }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        g.setColour (col::panB.withAlpha (0.55f));
        g.fillRoundedRectangle (r, 10.0f);
        juce::Path p, dashed;
        p.addRoundedRectangle (r.reduced (0.5f), 10.0f);
        const float dash[] { 5.0f, 5.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, p, dash, 2);
        g.setColour (col::line3);
        g.fillPath (dashed);
        g.setFont (font (14.0f));
        g.setColour (col::dim);
        g.drawText ("Layer " + juce::String::charToString ((juce::juce_wchar) ('A' + (int) ctx.sound().parts.size())),
                    btn.getBounds().translated (0, 40).toFloat().withHeight (20.0f).expanded (60.0f, 0.0f), juce::Justification::centred, false);
    }
    void menu()
    {
        juce::PopupMenu m;
        m.setLookAndFeel (&getLookAndFeel());
        const auto& snd = ctx.sound();
        const bool room = ! snd.parts.empty() && (int) snd.parts.size() < (int) vault::maxParts;
        m.addSectionHeader ("Add a layer");
        m.addItem (1, "Copy of layer A", room);
        m.addItem (2, "Copy of layer A, one octave up", room);
        m.addItem (3, "Copy of layer A, one octave down", room);
        juce::Component::SafePointer<AddCard> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&btn), [safe] (int r) { if (safe != nullptr && r > 0) safe->owner.addLayer (r - 1); });
    }
    LayersPage& owner;
    UiContext& ctx;
    Pill btn { "Add layer", Pill::Icon::plus };
};

void LayersPage::addLayer (int how)
{
    const auto& snd = ctx.sound();
    if (snd.parts.empty() || (int) snd.parts.size() >= (int) vault::maxParts) return;
    ctx.edit ([how] (VaultPreset& p)
    {
        auto part = p.parts.front();
        part.level = juce::jlimit (0.0f, 2.0f, part.level * 0.7f);
        if (how == 1) part.octave = juce::jlimit (-3, 3, part.octave + 1);
        if (how == 2) part.octave = juce::jlimit (-3, 3, part.octave - 1);
        p.parts.push_back (part);
    });
    ctx.say ("Layer " + juce::String::charToString ((juce::juce_wchar) ('A' + (int) ctx.sound().parts.size() - 1)) + " added (a copy of layer A)");
}

void LayersPage::removeLastLayer()
{
    const int n = (int) ctx.sound().parts.size();
    if (n <= 1) return;
    const juce::String t = "layer" + juce::String (n) + ".";
    ctx.edit ([n, t] (VaultPreset& p)
    {
        p.parts.pop_back();
        // nothing may point at the layer that is gone
        for (auto& m : p.macros) m.targets.erase (std::remove_if (m.targets.begin(), m.targets.end(), [&] (const VaultMacroTarget& x) { return x.target.startsWith (t); }), m.targets.end());
        auto strip = [&] (std::vector<VaultModDest>& l) { l.erase (std::remove_if (l.begin(), l.end(), [&] (const VaultModDest& d) { return d.target.startsWith (t); }), l.end()); };
        for (auto& l : p.mod.lfo) strip (l.dests);
        strip (p.mod.modWheel); strip (p.mod.velocity); strip (p.mod.aftertouch);
        juce::ignoreUnused (n);
    });
    ctx.say ("Layer " + juce::String::charToString ((juce::juce_wchar) ('A' + n - 1)) + " removed");
}

int LayersPage::numCards() const { return (int) cards.size(); }
juce::Component* LayersPage::cardComponent (int i) const { return juce::isPositiveAndBelow (i, (int) cards.size()) ? cards[(size_t) i].get() : nullptr; }

void LayersPage::rebuild()
{
    const auto& snd = ctx.sound();
    juce::String k = snd.id;
    for (auto& p : snd.parts) k << "|" << (int) p.kind << p.source;
    if (k == layoutKey) return;
    layoutKey = k;
    cards.clear();
    addCards.clear();
    for (int i = 0; i < (int) snd.parts.size(); ++i)
    {
        cards.push_back (std::make_unique<Card> (*this, ctx, i));
        addAndMakeVisible (*cards.back());
    }
    if (snd.parts.size() < 3)   // one "+ Add layer" column next to the layers (they share the page)
    {
        addCards.push_back (std::make_unique<AddCard> (*this, ctx));
        addAndMakeVisible (*addCards.back());
    }
    resized();
}

void LayersPage::resized()
{
    const int n = (int) cards.size() + (int) addCards.size() >= 3 ? std::max (3, (int) cards.size()) : (int) cards.size() + (int) addCards.size();
    const int cardH = getHeight() - voiceH - gap;
    const int cw = (getWidth() - (n - 1) * gap) / n;
    for (int i = 0; i < (int) cards.size(); ++i) cards[(size_t) i]->setBounds (i * (cw + gap), 0, cw, cardH);
    for (int i = 0; i < (int) addCards.size(); ++i) addCards[(size_t) i]->setBounds (((int) cards.size() + i) * (cw + gap), 0, cw, cardH);
    voiceR = { 0, getHeight() - voiceH, getWidth(), voiceH };
    int x = voiceR.getX() + 16 + (int) std::ceil (headerWidth ("VOICE")) + 18;
    voiceMode.setBounds (x, voiceR.getCentreY() - 15, voiceMode.idealWidth(), 30);
    x = voiceMode.getRight() + 26;
    for (auto& k : voice) { k.setBounds (x, voiceR.getY() + 6, 96, k.idealHeight()); x += 104; }
    resetBtn.setBounds (voiceR.getRight() - 16 - resetBtn.idealWidth(), voiceR.getCentreY() - 15, resetBtn.idealWidth(), 30);
}

void LayersPage::paint (juce::Graphics& g)
{
    drawPanel (g, voiceR.toFloat());
    drawHeader (g, "VOICE", voiceR.toFloat().withTrimmedLeft (16.0f));
}

void LayersPage::refresh()
{
    rebuild();
    for (auto& c : cards) c->refresh();
    const auto& snd = ctx.sound();
    const VaultPart* synth = nullptr; const VaultPart* sample = nullptr;
    for (auto& p : snd.parts)
    {
        if (p.kind == VaultPartKind::synth && synth == nullptr) synth = &p;
        if (p.kind == VaultPartKind::sample && sample == nullptr) sample = &p;
    }
    const bool hasSynth = synth != nullptr, hasSample = sample != nullptr;
    voiceMode.setEnabledLook (hasSynth);
    for (int i = 0; i < 3; ++i) voice[(size_t) i].setEnabledLook (hasSynth);
    voice[3].setEnabledLook (hasSynth || hasSample);
    voice[4].setEnabledLook (hasSynth || hasSample);
    if (hasSynth)
    {
        const bool mono = synth->get ("mono", 0) > 0.5f, legato = synth->get ("legato", 1) > 0.5f;
        voiceMode.setSelected (! mono ? 0 : legato ? 2 : 1);
        voice[0].setValue (val::layerValue (snd, val::Ctx::of (ctx.proc), (int) (synth - snd.parts.data()), val::LKey::glide));
        voice[1].setValue ((synth->get ("unison", 1) - 1.0f) / 7.0f);
        voice[2].setValue (synth->get ("detune", 0.2f));
        voice[3].setValue (synth->get ("velSens", 0.6f));
        voice[4].setValue (synth->get ("bendRange", 2) / 24.0f);
    }
    else
    {
        voiceMode.setSelected (0);
        for (int i = 0; i < 3; ++i) voice[(size_t) i].setValue (0.0f);
        if (hasSample)
        {
            voice[3].setValue (sample->get ("s.velSens", 0.5f));
            voice[4].setValue (sample->get ("s.bendRange", 2) / 24.0f);
        }
    }
    // double-click: the bank preset's value
    const auto& bank = ctx.bankSound();
    for (auto& p : bank.parts)
    {
        if (p.kind == VaultPartKind::synth)
        {
            voice[0].resetValue = p.get ("glide", 0); voice[1].resetValue = (p.get ("unison", 1) - 1.0f) / 7.0f;
            voice[2].resetValue = p.get ("detune", 0.2f); voice[3].resetValue = p.get ("velSens", 0.6f); voice[4].resetValue = p.get ("bendRange", 2) / 24.0f;
            break;
        }
        if (p.kind == VaultPartKind::sample) { voice[3].resetValue = p.get ("s.velSens", 0.5f); voice[4].resetValue = p.get ("s.bendRange", 2) / 24.0f; break; }
    }
}
} // namespace vui
