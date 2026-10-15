#include "VaultPages.h"

// CHROME VAULT v3 FX page: the 8-slot chain of the current sound. Each slot card: drag grip (reorder), power, type,
// variant / type menu (▾), its 3 main knobs. FX presets (whole chains), Bypass all, Keep while browsing.
namespace vui
{
namespace
{
constexpr int headerH = 34, gridTop = 48, cardH = 140, gap = 12, cols = 4;
using vault::FxType;

int variantParam (FxType t)
{
    switch ((int) t)
    {
        case (int) FxType::reverb: return 2;     // type
        case (int) FxType::drive: return 1;      // type
        case (int) FxType::delay: return 3;      // mode
        case (int) FxType::chorus: return 3;     // mode
        case (int) FxType::filter: return 2;     // mode
        case (int) FxType::body: return 0;       // model
        default: return -1;
    }
}
int countType (const VaultPreset& p, FxType t) { int n = 0; for (auto& s : p.fx) n += s.type == t ? 1 : 0; return n; }

struct ChainPreset { const char* name; std::function<std::vector<VaultFxSlot>()> make; };
VaultFxSlot slotOf (FxType t, std::initializer_list<std::pair<const char*, float>> values)
{
    auto s = VaultFxSlot::make (t);
    for (auto& [k, v] : values) s.set (k, v);
    return s;
}
float div (const char* name) { return (float) vault::divisionIndex (name); }
const std::vector<ChainPreset>& chainPresets()
{
    // whole chains, all moderate (nothing harsh: gentle highs, no hard clipping)
    static const std::vector<ChainPreset> c {
        { "Warm Room", [] { return std::vector<VaultFxSlot> {
              slotOf (FxType::compressor, { { "threshold", -20 }, { "ratio", 2.5f }, { "attack", 15 }, { "release", 120 } }),
              slotOf (FxType::eq, { { "low", 1.0f }, { "high", -2.0f } }),
              slotOf (FxType::reverb, { { "mix", 0.18f }, { "size", 0.5f }, { "type", 3 }, { "decay", 1.0f } }) }; } },
        { "Wide Hall", [] { return std::vector<VaultFxSlot> {
              slotOf (FxType::chorus, { { "mix", 0.2f }, { "rate", 0.33f } }),
              slotOf (FxType::delay, { { "mix", 0.12f }, { "time", div ("1/8D") }, { "feedback", 0.25f } }),
              slotOf (FxType::reverb, { { "mix", 0.3f }, { "size", 0.7f }, { "type", 0 }, { "decay", 1.4f } }),
              slotOf (FxType::width, { { "width", 0.7f }, { "monoBelow", 150 } }) }; } },
        { "Tape Echo", [] { return std::vector<VaultFxSlot> {
              slotOf (FxType::drive, { { "amount", 0.2f }, { "type", 1 }, { "mix", 0.7f }, { "tone", 9000 } }),
              slotOf (FxType::delay, { { "mix", 0.2f }, { "time", div ("1/4") }, { "feedback", 0.35f }, { "mode", 2 }, { "hiCut", 6000 } }),
              slotOf (FxType::reverb, { { "mix", 0.12f }, { "size", 0.4f }, { "type", 1 }, { "decay", 0.8f } }) }; } },
        { "Lo-Fi Room", [] { return std::vector<VaultFxSlot> {
              slotOf (FxType::lofi, { { "crush", 0.25f }, { "wow", 0.3f }, { "mix", 0.8f } }),
              slotOf (FxType::eq, { { "low", -1.0f }, { "high", -4.0f } }),
              slotOf (FxType::reverb, { { "mix", 0.15f }, { "size", 0.4f }, { "type", 3 }, { "decay", 0.8f } }) }; } },
        { "Shimmer Space", [] { return std::vector<VaultFxSlot> {
              slotOf (FxType::eq, { { "high", -1.5f } }),
              slotOf (FxType::delay, { { "mix", 0.18f }, { "time", div ("1/4D") }, { "feedback", 0.4f }, { "hiCut", 7000 } }),
              slotOf (FxType::reverb, { { "mix", 0.35f }, { "size", 0.8f }, { "type", 4 }, { "decay", 2.0f }, { "shimmer", 0.35f } }) }; } },
        { "Punchy", [] { return std::vector<VaultFxSlot> {
              slotOf (FxType::transient, { { "attack", 0.35f } }),
              slotOf (FxType::compressor, { { "threshold", -18 }, { "ratio", 3 }, { "attack", 8 }, { "release", 100 } }),
              slotOf (FxType::drive, { { "amount", 0.15f }, { "type", 0 }, { "tone", 12000 } }) }; } },
        { "Dreamy Chorus", [] { return std::vector<VaultFxSlot> {
              slotOf (FxType::chorus, { { "mix", 0.4f }, { "rate", 0.5f }, { "mode", 1 } }),
              slotOf (FxType::delay, { { "mix", 0.15f }, { "time", div ("1/8") }, { "feedback", 0.3f } }),
              slotOf (FxType::reverb, { { "mix", 0.25f }, { "size", 0.7f }, { "type", 2 }, { "decay", 1.4f } }) }; } },
    };
    return c;
}
} // namespace

std::array<int, 3> FxPage::mainParams (FxType t)
{
    const auto& ti = vault::fxTypeInfo (t);
    auto i = [&] (const char* id) { return ti.indexOf (id); };
    switch ((int) t)
    {
        case (int) FxType::eq:         return { i ("low"), i ("mid"), i ("high") };
        case (int) FxType::compressor: return { i ("threshold"), i ("ratio"), i ("makeup") };
        case (int) FxType::drive:      return { i ("amount"), i ("tone"), i ("mix") };
        case (int) FxType::lofi:       return { i ("crush"), i ("wow"), i ("mix") };
        case (int) FxType::chorus:     return { i ("rate"), i ("depth"), i ("mix") };
        case (int) FxType::phaser:     return { i ("rate"), i ("depth"), i ("mix") };
        case (int) FxType::flanger:    return { i ("rate"), i ("feedback"), i ("mix") };
        case (int) FxType::filter:     return { i ("cutoff"), i ("reso"), i ("env") };
        case (int) FxType::delay:      return { i ("time"), i ("feedback"), i ("mix") };
        case (int) FxType::reverb:     return { i ("size"), i ("decay"), i ("mix") };
        case (int) FxType::width:      return { i ("width"), i ("monoBelow"), -1 };
        case (int) FxType::reverse:    return { i ("length"), i ("mix"), -1 };
        case (int) FxType::transient:  return { i ("attack"), -1, -1 };
        case (int) FxType::body:       return { i ("mix"), -1, -1 };
        case (int) FxType::ghost:      return { i ("blur"), i ("mix"), -1 };
        case (int) FxType::stutter:    return { i ("rate"), i ("amount"), -1 };
        case (int) FxType::halftime:   return { i ("mix"), -1, -1 };
        default:                 return { -1, -1, -1 };
    }
}

juce::String FxPage::variantText (const VaultFxSlot& s)
{
    const int v = variantParam (s.type);
    if (v < 0)
        switch ((int) s.type)
        {
            case (int) FxType::eq:         return "3-Band";
            case (int) FxType::compressor: return "Stereo";
            case (int) FxType::lofi:       return "Tape";
            case (int) FxType::phaser: case (int) FxType::flanger: case (int) FxType::width: return "Stereo";
            case (int) FxType::transient:  return "Shaper";
            case (int) FxType::ghost:      return s.get ("reverse") > 0.5f ? "Reverse" : "Forward";
            default:                 return "Synced";
        }
    return choiceName (vault::fxTypeInfo (s.type).params[(size_t) v], s.p[(size_t) v]);
}

//==============================================================================
struct FxPage::SlotCard : public juce::Component
{
    SlotCard (FxPage& o, UiContext& c, int s) : owner (o), ctx (c), slot (s)
    {
        noFocus (*this);
        addChildComponent (power);
        addChildComponent (variant);
        variant.small = true;
        addChildComponent (addBtn);
        for (auto& k : knobs) { k.diameter = 36; addChildComponent (k); }
        power.onClick = [this]
        {
            const int sl = slot;
            ctx.edit ([sl] (VaultPreset& p) { if (juce::isPositiveAndBelow (sl, (int) p.fx.size())) p.fx[(size_t) sl].on = ! p.fx[(size_t) sl].on; });
        };
        variant.onClick = [this] { owner.showSlotMenu (slot, variant); };
        addBtn.onClick = [this] { owner.showSlotMenu (slot, addBtn); };
        for (int k = 0; k < 3; ++k)
        {
            knobs[(size_t) k].onChange = [this, k] (float n)
            {
                const int sl = slot, pi = params[(size_t) k];
                ctx.edit ([sl, pi, n] (VaultPreset& p)
                {
                    if (! juce::isPositiveAndBelow (sl, (int) p.fx.size()) || pi < 0) return;
                    auto& fx = p.fx[(size_t) sl];
                    fx.p[(size_t) pi] = vault::fxTypeInfo (fx.type).params[(size_t) pi].fromNorm (n);
                });
            };
            knobs[(size_t) k].onRightClick = [this, k] (const juce::MouseEvent&)
            {
                if (targets[(size_t) k].isNotEmpty()) mod::showMenu (ctx, knobs[(size_t) k], targets[(size_t) k]);
            };
        }
    }

    void refresh()
    {
        const auto& snd = ctx.sound();
        empty = slot >= (int) snd.fx.size();
        power.setVisible (! empty);
        variant.setVisible (! empty);
        addBtn.setVisible (empty && slot == (int) snd.fx.size());
        if (empty)
        {
            for (auto& k : knobs) k.setVisible (false);
            setAlpha (1.0f);
            repaint();
            return;
        }
        const auto& s = snd.fx[(size_t) slot];
        const auto& ti = vault::fxTypeInfo (s.type);
        power.setToggleState (s.on, juce::dontSendNotification);
        setAlpha (s.on ? 1.0f : 0.5f);
        name = juce::String (ti.name).toUpperCase();
        const auto vt = variantText (s);
        if (variant.getButtonText() != vt) { variant.setButtonText (vt); resized(); }
        params = mainParams (s.type);
        const auto& bank = ctx.bankSound();
        const VaultFxSlot* bs = slot < (int) bank.fx.size() && bank.fx[(size_t) slot].type == s.type ? &bank.fx[(size_t) slot] : nullptr;
        for (int k = 0; k < 3; ++k)
        {
            auto& kn = knobs[(size_t) k];
            const int pi = params[(size_t) k];
            kn.setVisible (pi >= 0);
            if (pi < 0) { targets[(size_t) k].clear(); continue; }
            const auto& in = ti.params[(size_t) pi];
            juce::String lbl (in.name);
            lbl = lbl.toUpperCase().replace ("THRESHOLD", "THRESH").replace ("FEEDBACK", "FEEDBK").replace ("MONO BELOW", "MONO <")
                     .replace ("RESONANCE", "RESO").replace ("ENV AMOUNT", "ENV");
            kn.label = lbl;
            const bool stepped = (in.flags & (vault::FxParamInfo::integer | vault::FxParamInfo::toggle)) != 0;
            kn.steps = stepped ? juce::roundToInt (in.hi - in.lo) + 1 : 0;
            kn.sensitivity = kn.steps > 12 ? 0.6f : 1.0f;
            kn.arcOrigin = in.lo < 0 && in.hi > 0 ? 0.5f : 0.0f;
            const auto info = in;
            kn.valueText = [info] (float n) { return fxValueText (info, info.fromNorm (n)); };
            kn.setValue (in.toNorm (s.p[(size_t) pi]));
            kn.resetValue = in.toNorm (bs != nullptr ? bs->p[(size_t) pi] : in.def);
            targets[(size_t) k] = stepped ? juce::String() : "fx" + juce::String (slot + 1) + "." + in.id;
            mod::show (kn, snd, targets[(size_t) k]);
            kn.repaint();
        }
        resized();
        repaint();
    }

    juce::Rectangle<int> gripRect() const { return { 6, 10, 20, 26 }; }

    void resized() override
    {
        const int w = getWidth();
        power.setBounds (30, 13, 20, 20);
        const int vw = std::min (std::max (60, variant.idealWidth()), w / 2 - 10);
        variant.setBounds (w - 12 - vw, 11, vw, 24);
        addBtn.setBounds (getLocalBounds().withSizeKeepingCentre (std::min (w - 40, addBtn.idealWidth() + 10), 34));
        int n = 0;
        for (int k = 0; k < 3; ++k) n += params[(size_t) k] >= 0 ? 1 : 0;
        const int cw = (w - 20) / 3;
        int i = 0;
        for (int k = 0; k < 3; ++k)
            if (params[(size_t) k] >= 0)
            {
                // fewer than 3 knobs: centred
                const int x0 = 10 + (3 - n) * cw / 2;
                knobs[(size_t) k].setBounds (x0 + i * cw, 46, cw, knobs[(size_t) k].idealHeight());
                ++i;
            }
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat();
        if (empty)
        {
            g.setColour (col::panB.withAlpha (0.6f));
            g.fillRoundedRectangle (r, 10.0f);
            juce::Path p;
            p.addRoundedRectangle (r.reduced (0.5f), 10.0f);
            juce::Path dashed;
            const float dash[] { 5.0f, 5.0f };
            juce::PathStrokeType (1.0f).createDashedStroke (dashed, p, dash, 2);
            g.setColour (col::line3);
            g.fillPath (dashed);
            if (! addBtn.isVisible())
            {
                g.setFont (font (14.0f));
                g.setColour (col::dim);
                g.drawText ("Slot " + juce::String (slot + 1), r, juce::Justification::centred, false);
            }
            return;
        }
        drawPanel (g, r);
        if (owner.dragFrom == slot) { g.setColour (col::ice.withAlpha (0.08f)); g.fillRoundedRectangle (r, 10.0f); }
        // grip
        const auto gr = gripRect().toFloat();
        g.setColour (juce::Colour (0xff4a5670));
        for (int i = 0; i < 2; ++i)
            for (int j = 0; j < 3; ++j)
                g.fillEllipse (gr.getX() + 6.0f + (float) i * 6.0f, gr.getY() + 6.0f + (float) j * 6.0f, 2.6f, 2.6f);
        g.setFont (font (14.0f, true, 0.08f));
        g.setColour (col::text);
        g.drawText (name, juce::Rectangle<float> (58.0f, 10.0f, (float) variant.getX() - 64.0f, 26.0f), juce::Justification::centredLeft, true);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (! empty && gripRect().expanded (4).contains (e.getPosition())) owner.beginDrag (slot);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (owner.dragFrom >= 0) owner.dragTo (e.getEventRelativeTo (&owner).getPosition()); }
    void mouseUp (const juce::MouseEvent&) override { if (owner.dragFrom >= 0) owner.endDrag(); }
    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (! empty && gripRect().expanded (4).contains (e.getPosition()) ? juce::MouseCursor::DraggingHandCursor : juce::MouseCursor::NormalCursor);
    }

    FxPage& owner;
    UiContext& ctx;
    int slot;
    bool empty = true;
    PowerButton power;
    Dropdown variant;
    Pill addBtn { "Add effect", Pill::Icon::plus };
    std::array<Knob, 3> knobs;
    std::array<int, 3> params { -1, -1, -1 };
    std::array<juce::String, 3> targets;
    juce::String name;
};

//==============================================================================
FxPage::FxPage (UiContext& c) : Page (c)
{
    for (int i = 0; i < vault::maxFxSlots; ++i)
    {
        slots.push_back (std::make_unique<SlotCard> (*this, ctx, i));
        addAndMakeVisible (*slots.back());
    }
    addAndMakeVisible (presetBtn);
    presetBtn.onClick = [this] { showPresetMenu(); };
    addAndMakeVisible (bypassBtn);
    bypassBtn.onClick = [this]
    {
        const bool on = ctx.proc.getSwitch (VaultProcessor::idFxOn);
        ctx.proc.setSwitch (VaultProcessor::idFxOn, ! on);
        if (ctx.soundEdited) ctx.soundEdited();
    };
    bypassBtn.setTooltip ("Every effect of the chain off / on (the same as FX in the top bar)");
    addAndMakeVisible (keepBtn);
    keepBtn.onClick = [this]
    {
        const bool l = ! ctx.proc.getLock (VaultProcessor::Section::fx);
        ctx.proc.setLock (VaultProcessor::Section::fx, l);
        ctx.say (l ? "This FX chain stays when you pick another sound" : "Every sound brings its own FX chain again");
        if (ctx.soundEdited) ctx.soundEdited();
    };
    keepBtn.setTooltip ("Keep this FX chain while browsing other sounds");
    refresh();
}

FxPage::~FxPage() = default;

juce::Rectangle<int> FxPage::cardRect (int i) const
{
    const int cw = (getWidth() - (cols - 1) * gap) / cols;
    return { (i % cols) * (cw + gap), gridTop + (i / cols) * (cardH + gap), cw, cardH };
}

void FxPage::resized()
{
    for (int i = 0; i < (int) slots.size(); ++i) slots[(size_t) i]->setBounds (cardRect (i));
    int x = getWidth();
    auto place = [&] (juce::Component& b, int w) { x -= w; b.setBounds (x, 2, w, 30); x -= 8; };
    place (bypassBtn, bypassBtn.idealWidth());
    place (presetBtn, std::min (300, presetBtn.idealWidth()));
    place (keepBtn, keepBtn.idealWidth());
}

void FxPage::paint (juce::Graphics& g)
{
    const float hx = 0.0f;
    const auto t = juce::String ("FX CHAIN");
    drawHeader (g, t, { hx, 2.0f, 120.0f, 30.0f });
    g.setFont (font (14.0f));
    g.setColour (col::dim);
    g.drawText (juce::String (juce::CharPointer_UTF8 ("drag the grip to reorder \xc2\xb7 \xe2\x97\x8f on / off \xc2\xb7 \xe2\x96\xbe type")),
                juce::Rectangle<float> (hx + headerWidth (t) + 16.0f, 2.0f, (float) keepBtn.getX() - headerWidth (t) - 30.0f, 30.0f),
                juce::Justification::centredLeft, true);
    g.drawText ("Every sound comes with its own effects already set. The same effects are on the MAIN page as quick knobs; here you get the full controls.",
                juce::Rectangle<float> (0.0f, (float) getHeight() - 26.0f, (float) getWidth(), 22.0f), juce::Justification::centredLeft, true);
}

void FxPage::paintOverChildren (juce::Graphics& g)
{
    if (dragFrom < 0 || dragOver < 0 || dragOver == dragFrom) return;
    const auto r = cardRect (dragOver).toFloat();
    g.setColour (col::ice);
    const float x = dragOver < dragFrom ? r.getX() - 7.0f : r.getRight() + 5.0f;
    g.fillRoundedRectangle (x, r.getY() + 6.0f, 2.5f, r.getHeight() - 12.0f, 1.2f);
}

void FxPage::refresh()
{
    for (auto& s : slots) s->refresh();
    const bool fxOn = ctx.proc.getSwitch (VaultProcessor::idFxOn);
    bypassBtn.setToggleState (! fxOn, juce::dontSendNotification);
    keepBtn.setToggleState (ctx.proc.getLock (VaultProcessor::Section::fx), juce::dontSendNotification);
    const auto t = "FX preset: " + chainName();
    if (presetBtn.getButtonText() != t) { presetBtn.setButtonText (t); resized(); }
    repaint();
}

juce::String FxPage::chainName() const
{
    const auto& snd = ctx.sound();
    if (snd.fx.empty()) return "None";
    if (snd.fx == ctx.bankSound().fx) return snd.name;
    for (auto& c : chainPresets()) if (appliedChain == c.name && c.make() == snd.fx) return c.name;
    return "Custom";
}

int FxPage::slotAt (juce::Point<int> p) const
{
    for (int i = 0; i < (int) ctx.sound().fx.size(); ++i) if (cardRect (i).expanded (gap / 2).contains (p)) return i;
    return -1;
}
void FxPage::beginDrag (int slot) { dragFrom = slot; dragOver = slot; repaint(); }
void FxPage::dragTo (juce::Point<int> p)
{
    const int o = slotAt (p);
    if (o >= 0 && o != dragOver) { dragOver = o; repaint(); }
}
void FxPage::endDrag()
{
    const int from = dragFrom, to = dragOver;
    dragFrom = dragOver = -1;
    if (from >= 0 && to >= 0 && from != to)
    {
        ctx.edit ([from, to] (VaultPreset& p)
        {
            if (! juce::isPositiveAndBelow (from, (int) p.fx.size()) || ! juce::isPositiveAndBelow (to, (int) p.fx.size())) return;
            auto s = p.fx[(size_t) from];
            p.fx.erase (p.fx.begin() + from);
            p.fx.insert (p.fx.begin() + to, s);
        });
        ctx.say ("Effect moved to slot " + juce::String (to + 1));
    }
    refresh();
}

void FxPage::showSlotMenu (int slot, juce::Component& anchor)
{
    const auto snd = ctx.sound();
    const bool exists = slot < (int) snd.fx.size();
    juce::PopupMenu m, types, variants;
    m.setLookAndFeel (&getLookAndFeel());
    for (int t = 0; t < vault::numFxTypes; ++t)
    {
        const auto ft = (FxType) t;
        const bool cur = exists && snd.fx[(size_t) slot].type == ft;
        const bool ok = cur || countType (snd, ft) < vault::maxSameFxType;
        types.addItem (100 + t, vault::fxTypeInfo (ft).name, ok, cur);
    }
    if (! exists)
    {
        m.addSectionHeader ("Add effect");
        for (int t = 0; t < vault::numFxTypes; ++t)
        {
            const auto ft = (FxType) t;
            m.addItem (100 + t, vault::fxTypeInfo (ft).name, countType (snd, ft) < vault::maxSameFxType && (int) snd.fx.size() < vault::maxFxSlots);
        }
    }
    else
    {
        const auto& s = snd.fx[(size_t) slot];
        const int vp = variantParam (s.type);
        if (vp >= 0)
        {
            const auto& in = vault::fxTypeInfo (s.type).params[(size_t) vp];
            const auto names = juce::StringArray::fromTokens (in.choices, "|", "");
            for (int i = 0; i < names.size(); ++i) m.addItem (200 + i, names[i], true, juce::roundToInt (s.p[(size_t) vp] - in.lo) == i);
            m.addSeparator();
        }
        m.addSubMenu ("Change type", types);
        m.addItem (301, s.on ? "Switch off" : "Switch on");
        m.addItem (302, "Move left", slot > 0);
        m.addItem (303, "Move right", slot < (int) snd.fx.size() - 1);
        m.addItem (304, "Reset to the preset's settings", slot < (int) ctx.bankSound().fx.size() && ctx.bankSound().fx[(size_t) slot].type == s.type);
        m.addSeparator();
        m.addItem (305, "Remove");
    }
    juce::Component::SafePointer<FxPage> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&anchor), [safe, slot, exists] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        auto& c = safe->ctx;
        const auto bankFx = c.bankSound().fx;
        c.edit ([&] (VaultPreset& p)
        {
            if (r >= 100 && r < 100 + vault::numFxTypes)
            {
                auto s = VaultFxSlot::make ((FxType) (r - 100));
                if (s.type == FxType::width) s.set ("width", 0.7f);
                if (s.type == FxType::drive) s.set ("amount", 0.15f);
                if (exists && slot < (int) p.fx.size()) { s.on = p.fx[(size_t) slot].on; p.fx[(size_t) slot] = s; }
                else if ((int) p.fx.size() < vault::maxFxSlots) p.fx.push_back (s);
                return;
            }
            if (! juce::isPositiveAndBelow (slot, (int) p.fx.size())) return;
            auto& s = p.fx[(size_t) slot];
            if (r >= 200 && r < 300) { const int vp = variantParam (s.type); if (vp >= 0) s.p[(size_t) vp] = vault::fxTypeInfo (s.type).params[(size_t) vp].lo + (float) (r - 200); }
            else if (r == 301) s.on = ! s.on;
            else if (r == 302 && slot > 0) std::swap (p.fx[(size_t) slot], p.fx[(size_t) slot - 1]);
            else if (r == 303 && slot + 1 < (int) p.fx.size()) std::swap (p.fx[(size_t) slot], p.fx[(size_t) slot + 1]);
            else if (r == 304 && slot < (int) bankFx.size()) s = bankFx[(size_t) slot];
            else if (r == 305) p.fx.erase (p.fx.begin() + slot);
        });
    });
}

void FxPage::showPresetMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    m.addItem (1, "As designed: " + ctx.sound().name, true, ctx.sound().fx == ctx.bankSound().fx);
    m.addItem (2, "None (dry)");
    m.addSeparator();
    for (int i = 0; i < (int) chainPresets().size(); ++i) m.addItem (10 + i, chainPresets()[(size_t) i].name);
    juce::Component::SafePointer<FxPage> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        auto& c = safe->ctx;
        std::vector<VaultFxSlot> chain;
        juce::String name;
        if (r == 1) { chain = c.bankSound().fx; name = c.sound().name; }
        else if (r == 2) name = "None";
        else if (juce::isPositiveAndBelow (r - 10, (int) chainPresets().size()))
        {
            chain = chainPresets()[(size_t) r - 10].make();
            name = chainPresets()[(size_t) r - 10].name;
        }
        safe->appliedChain = name;
        c.edit ([&] (VaultPreset& p) { p.fx = chain; });
        c.say ("FX preset: " + name);
    });
}
} // namespace vui
