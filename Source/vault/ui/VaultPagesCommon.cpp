#include "VaultPages.h"

// CHROME VAULT v3 editor: text formatting and the "Modulate with..." machinery shared by every page.
namespace vui
{
juce::String pct (float v01) { return juce::String (juce::roundToInt (v01 * 100.0f)) + "%"; }
juce::String signedPct (float v)
{
    const int p = juce::roundToInt (v * 100.0f);
    return (p > 0 ? "+" : "") + juce::String (p) + "%";
}
juce::String secondsText (float s)
{
    if (s < 0.0995f) return juce::String (juce::roundToInt (s * 1000.0f)) + " ms";
    if (s < 9.95f) return juce::String (s, 1) + " s";
    return juce::String (juce::roundToInt (s)) + " s";
}
juce::String hzText (float hz)
{
    if (hz >= 9950.0f) return juce::String (juce::roundToInt (hz / 1000.0f)) + " kHz";
    if (hz >= 995.0f) return juce::String (hz / 1000.0f, 1) + " kHz";
    if (hz >= 9.95f) return juce::String (juce::roundToInt (hz)) + " Hz";
    return juce::String (hz, 2) + " Hz";
}
juce::String choiceName (const vault::FxParamInfo& in, float v)
{
    if (in.choices == nullptr) return {};
    const auto names = juce::StringArray::fromTokens (in.choices, "|", "");
    const int i = juce::roundToInt (v - in.lo);
    return juce::isPositiveAndBelow (i, names.size()) ? names[i] : juce::String();
}
juce::String fxValueText (const vault::FxParamInfo& in, float v)
{
    using F = vault::FxParamInfo;
    if (in.flags & F::toggle) return v > 0.5f ? "On" : "Off";
    if (in.choices != nullptr) return choiceName (in, v);
    const juce::String unit (in.unit);
    if (unit == "dB") return (v > 0.05f ? "+" : "") + juce::String (v, std::abs (v) < 9.95f ? 1 : 0) + " dB";
    if (unit == "Hz") return in.lo <= 0.0f && v < 0.5f ? juce::String ("off") : hzText (v);   // "Mono Below 0 Hz" = off
    if (unit == "ms") return v >= 1000.0f ? juce::String (v / 1000.0f, 2) + " s" : juce::String (juce::roundToInt (v)) + " ms";
    if (unit == ":1") return juce::String (v, v < 9.95f ? 1 : 0) + ":1";
    if (unit == "x") return juce::String (v, 2) + "x";
    if (in.flags & F::integer) return juce::String (juce::roundToInt (v));
    if (in.lo >= 0.0f && in.hi <= 1.0f) return pct (v);
    if (in.lo < 0.0f && in.hi <= 1.0f) return signedPct (v);
    return juce::String (v, 2);
}

namespace mod
{
float range (const juce::String& target)
{
    using T = vault::TargetRef;
    const auto t = vault::parseTarget (target);
    if (t.kind == T::pitch) return 12.0f;
    if (t.kind == T::cutoff) return 4.0f;
    if (t.kind == T::layer) return t.b == T::lTune ? 12.0f : (t.b == T::lCutoff || t.b == T::lAttack || t.b == T::lRelease) ? 4.0f : 1.0f;
    return 1.0f;
}

juce::String amountText (const juce::String& target, float a)
{
    using T = vault::TargetRef;
    const auto t = vault::parseTarget (target);
    const juce::String sign = a > 0 ? "+" : a < 0 ? juce::String (juce::CharPointer_UTF8 ("\xe2\x88\x92")) : juce::String();
    const float m = std::abs (a);
    if (t.kind == T::pitch || (t.kind == T::layer && t.b == T::lTune))
        return m < 0.995f ? sign + juce::String (juce::roundToInt (m * 100.0f)) + " ct" : sign + juce::String (m, m < 9.95f ? 1 : 0) + " st";
    if (t.kind == T::cutoff || (t.kind == T::layer && (t.b == T::lCutoff || t.b == T::lAttack || t.b == T::lRelease)))
        return sign + juce::String (m, 2) + " oct";
    return sign + juce::String (juce::roundToInt (m * 100.0f)) + "%";
}

juce::String longName (const juce::String& target, const VaultPreset* p)
{
    using T = vault::TargetRef;
    const auto t = vault::parseTarget (target);
    if (t.kind == T::fxSlot)
    {
        // "fx1.mix" -> "Reverb Mix" (the effect that sits in that slot; a second one of a type: "Reverb 2 Mix")
        const auto tail = target.fromFirstOccurrenceOf (".", false, false);
        if (p != nullptr && juce::isPositiveAndBelow ((int) t.a, (int) p->fx.size()))
        {
            const auto type = p->fx[(size_t) t.a].type;
            const auto& ti = vault::fxTypeInfo (type);
            int nth = 0, same = 0;
            for (int i = 0; i < (int) p->fx.size(); ++i)
                if (p->fx[(size_t) i].type == type) { ++same; if (i <= (int) t.a) ++nth; }
            const int pi = ti.indexOf (tail.toRawUTF8());
            const juce::String prm = pi >= 0 ? juce::String (ti.params[(size_t) pi].name) : tail.substring (0, 1).toUpperCase() + tail.substring (1);
            return juce::String (ti.name) + (same > 1 ? " " + juce::String (nth) : juce::String()) + " " + prm;
        }
        return "Effect " + juce::String (t.a + 1) + " " + tail.substring (0, 1).toUpperCase() + tail.substring (1);
    }
    return vault::describeTarget (target);
}

juce::String shortName (const juce::String& target, const VaultPreset* p)
{
    using T = vault::TargetRef;
    const auto t = vault::parseTarget (target);
    if (t.kind == T::layer)
        return "L" + juce::String (t.a + 1) + " " + juce::StringArray { "Level", "Pan", "Cutoff", "Reso", "Attack", "Release", "Tune" }[t.b];
    if (t.kind == T::reso) return "Reso";
    return longName (target, p);
}

// two targets that move the same value: "fx.<type>.<param>" (the first slot of that type) and "fx<n>.<param>" when slot n
// is that first slot - a routing on either shows its ring on the MAIN knob and on the FX page's knob
bool sameTarget (const VaultPreset& p, const juce::String& a, const juce::String& b)
{
    if (a == b) return true;
    using T = vault::TargetRef;
    const auto ta = vault::parseTarget (a), tb = vault::parseTarget (b);
    auto fxOf = [&] (const T& t, const juce::String& id, int& slot, juce::String& param) -> bool
    {
        if (t.kind == T::fxSlot)
        {
            slot = t.a;
            param = id.fromFirstOccurrenceOf (".", false, false);
            return juce::isPositiveAndBelow (slot, (int) p.fx.size());
        }
        if (t.kind == T::fxType)
        {
            const auto type = (vault::FxType) t.a;
            slot = -1;
            for (int i = 0; i < (int) p.fx.size(); ++i) if (p.fx[(size_t) i].type == type) { slot = i; break; }
            const auto& ti = vault::fxTypeInfo (type);
            param = juce::isPositiveAndBelow ((int) t.b, (int) ti.params.size()) ? juce::String (ti.params[(size_t) t.b].id) : juce::String();
            return slot >= 0;
        }
        return false;
    };
    int sa = -1, sb = -1;
    juce::String pa, pb;
    return fxOf (ta, a, sa, pa) && fxOf (tb, b, sb, pb) && sa == sb && pa == pb && pa.isNotEmpty();
}

float amountOn (const VaultPreset& p, const juce::String& target, bool& any)
{
    any = false;
    float sum = 0;
    for (auto& l : p.mod.lfo)
        for (auto& d : l.dests)
            if (sameTarget (p, d.target, target)) { any = true; sum += d.amount * l.depth; }
    for (auto* list : { &p.mod.modWheel, &p.mod.velocity, &p.mod.aftertouch })
        for (auto& d : *list)
            if (sameTarget (p, d.target, target)) { any = true; sum += d.amount; }
    return juce::jlimit (-1.0f, 1.0f, sum / range (target));
}

void show (Knob& k, const VaultPreset& p, const juce::String& target)
{
    if (target.isEmpty()) { k.setModulated (false, 0.0f); return; }
    bool any = false;
    float a = amountOn (p, target, any);
    if (any && std::abs (a) < 0.04f) a = a < 0 ? -0.04f : 0.04f;   // always visible
    k.setModulated (any, any ? a : 0.0f);
}

namespace
{
float defaultAmount (const juce::String& target, int source)   // 0 1 = LFO, 2 wheel, 3 velocity, 4 aftertouch
{
    using T = vault::TargetRef;
    const auto t = vault::parseTarget (target);
    const bool pitchLike = t.kind == T::pitch || (t.kind == T::layer && t.b == T::lTune);
    if (pitchLike) return source <= 1 ? 0.15f : source == 2 ? 2.0f : source == 4 ? 0.5f : 0.0f;
    const float r = range (target);
    return (source <= 1 ? 0.25f : source == 2 ? 0.5f : 0.3f) * r;
}
std::vector<VaultModDest>* listFor (VaultPreset& p, int source)
{
    switch (source)
    {
        case 0: return &p.mod.lfo[0].dests;
        case 1: return &p.mod.lfo[1].dests;
        case 2: return &p.mod.modWheel;
        case 3: return &p.mod.velocity;
        default: return &p.mod.aftertouch;
    }
}
const char* sourceName (int s)
{
    static const char* n[] { "LFO 1", "LFO 2", "Mod Wheel", "Velocity", "Aftertouch" };
    return n[juce::jlimit (0, 4, s)];
}
} // namespace

void showMenu (UiContext& ctx, juce::Component& anchor, const juce::String& target)
{
    if (target.isEmpty()) return;
    auto snapshot = ctx.sound();
    juce::PopupMenu m;
    m.setLookAndFeel (&anchor.getLookAndFeel());
    m.addSectionHeader ("Modulate " + longName (target, &snapshot) + " with" + juce::String (juce::CharPointer_UTF8 ("\xe2\x80\xa6")));
    bool anyAt = false;
    for (int s = 0; s < 5; ++s)
    {
        auto* list = listFor (snapshot, s);
        float amount = 0;
        bool has = false;
        for (auto& d : *list) if (d.target == target) { has = true; amount = d.amount; }
        anyAt |= has;
        const int cap = s <= 1 ? (int) vault::maxLfoDests : (int) vault::maxRoutings;
        if (has) m.addItem (1 + s, juce::String (sourceName (s)) + "  " + amountText (target, amount) + "  (click to remove)", true, true);
        else m.addItem (1 + s, sourceName (s), (int) list->size() < cap, false);
    }
    m.addSeparator();
    m.addItem (9, "Remove all modulation", anyAt);
    juce::Component::SafePointer<juce::Component> safe (&anchor);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&anchor), [&ctx, safe, target] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        apply (ctx, target, r == 9 ? -1 : r - 1);
        if (safe != nullptr) safe->repaint();
    });
}

void apply (UiContext& ctx, const juce::String& target, int s)
{
    juce::String msg;
    ctx.edit ([&] (VaultPreset& p)
    {
        if (s < 0)
        {
            for (int k = 0; k < 5; ++k)
            {
                auto* l = listFor (p, k);
                l->erase (std::remove_if (l->begin(), l->end(), [&] (const VaultModDest& d) { return d.target == target; }), l->end());
            }
            msg = "Modulation of " + longName (target, &p) + " removed";
            return;
        }
        s = juce::jlimit (0, 4, s);
        auto* l = listFor (p, s);
        const auto it = std::find_if (l->begin(), l->end(), [&] (const VaultModDest& d) { return d.target == target; });
        if (it != l->end()) { l->erase (it); msg = juce::String (sourceName (s)) + " no longer moves " + longName (target, &p); return; }
        const int cap = s <= 1 ? (int) vault::maxLfoDests : (int) vault::maxRoutings;
        if ((int) l->size() >= cap) return;
        const float a = defaultAmount (target, s);
        l->push_back ({ target, a });
        if (s <= 1 && p.mod.lfo[s].depth <= 0.0f) p.mod.lfo[s].depth = 1.0f;
        msg = juce::String (sourceName (s)) + juce::String (juce::CharPointer_UTF8 (" \xe2\x86\x92 ")) + longName (target, &p) + " "
            + amountText (target, a) + " (the amount: MOD page)";
    });
    // the green ring shows at once on whatever page is open (ctx.edit refreshed it)
    if (msg.isNotEmpty()) ctx.say (msg);
}

void addTargetsMenu (juce::PopupMenu& menu, const VaultPreset& p, int firstId, juce::StringArray& ids)
{
    auto add = [&] (juce::PopupMenu& m, const juce::String& id, const juce::String& text)
    {
        m.addItem (firstId + ids.size(), text.isNotEmpty() ? text : vault::describeTarget (id));
        ids.add (id);
    };
    juce::PopupMenu sound, quick, layers, fx, rhythm, lfo;
    for (auto* id : { "pitch", "cutoff", "reso", "volume", "pan", "finish" }) add (sound, id, {});
    for (int q = 0; q < vault::numQuick; ++q) add (quick, juce::String ("quick.") + vault::quickInfo()[(size_t) q].id, vault::quickInfo()[(size_t) q].name);
    for (int i = 0; i < (int) p.parts.size(); ++i)
    {
        juce::PopupMenu l;
        for (auto* prm : { "level", "pan", "cutoff", "reso", "attack", "release", "tune" })
            add (l, "layer" + juce::String (i + 1) + "." + prm, juce::String (prm).substring (0, 1).toUpperCase() + juce::String (prm).substring (1));
        layers.addSubMenu ("Layer " + juce::String::charToString ((juce::juce_wchar) ('A' + i)) + (p.parts[(size_t) i].label.isNotEmpty() ? upDot() + p.parts[(size_t) i].label : juce::String()), l);
    }
    for (int i = 0; i < (int) p.fx.size(); ++i)
    {
        const auto& ti = vault::fxTypeInfo (p.fx[(size_t) i].type);
        juce::PopupMenu f;
        for (auto& prm : ti.params)
            if ((prm.flags & (vault::FxParamInfo::integer | vault::FxParamInfo::toggle)) == 0)
                add (f, "fx" + juce::String (i + 1) + "." + prm.id, prm.name);
        fx.addSubMenu (juce::String (i + 1) + upDot() + ti.name, f);
    }
    for (auto* id : { "gate.depth", "gate.mix", "gate.smooth", "arp.gate" }) add (rhythm, id, {});
    for (auto* id : { "lfo1.depth", "lfo1.rate", "lfo2.depth", "lfo2.rate" }) add (lfo, id, {});
    menu.addSubMenu ("Sound", sound);
    menu.addSubMenu ("MAIN knobs", quick);
    if (layers.getNumItems() > 0) menu.addSubMenu ("Layers", layers);
    if (fx.getNumItems() > 0) menu.addSubMenu ("Effects", fx);
    menu.addSubMenu ("Arp / Gate", rhythm);
    menu.addSubMenu ("LFO", lfo);
}
} // namespace mod
} // namespace vui
