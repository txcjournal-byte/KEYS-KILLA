#include "VaultPages.h"
#include "VaultValues.h"
#include "../VaultMod.h"

// CHROME VAULT v3 MOD page: the 2 LFOs (shape, rate / delay / fade / phase, sync, retrigger, destinations) and the mod
// wheel / velocity / aftertouch routings. Any knob of the editor: right-click -> "Modulate with...".
namespace vui
{
namespace
{
constexpr int waveExtra = 56;   // the taller wave display (the page is filled to the keyboard)
constexpr int cardH = 272 + waveExtra, gap = 12;
constexpr int maxRouteKnobW = 100;
std::vector<VaultModDest>& routeList (VaultPreset& p, int s) { return s == 0 ? p.mod.modWheel : s == 1 ? p.mod.velocity : p.mod.aftertouch; }
const std::vector<VaultModDest>& routeList (const VaultPreset& p, int s) { return s == 0 ? p.mod.modWheel : s == 1 ? p.mod.velocity : p.mod.aftertouch; }
juce::String bullet() { return juce::String (juce::CharPointer_UTF8 ("\xe2\x97\x8f ")); }
} // namespace

//==============================================================================
// a destination chip: "● Cutoff · 25%" - drag up / down for the amount, double-click = remove, right-click = menu
struct ModPage::Chip : public juce::Component, public juce::SettableTooltipClient
{
    Chip (UiContext& c, int l, int i) : ctx (c), lfo (l), index (i)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        setTooltip ("Drag up / down: amount (shift = fine), double-click: remove, right-click: invert / remove");
    }
    const VaultModDest* dest() const
    {
        const auto& d = ctx.sound().mod.lfo[lfo].dests;
        return juce::isPositiveAndBelow (index, (int) d.size()) ? &d[(size_t) index] : nullptr;
    }
    juce::String text() const
    {
        const auto* d = dest();
        return d == nullptr ? juce::String() : bullet() + mod::shortName (d->target, &ctx.sound()) + upDot() + mod::amountText (d->target, d->amount);
    }
    int idealWidth() const { return (int) std::ceil (textWidth (font (14.0f), text()) + 24.0f); }
    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (isMouseOverOrDragging() ? juce::Colour (0xff0f1d1a) : juce::Colour (0xff0b111a));
        g.fillRoundedRectangle (r, 6.0f);
        g.setColour (isMouseOverOrDragging() ? col::ok.withAlpha (0.8f) : col::okLine);
        g.drawRoundedRectangle (r, 6.0f, 1.0f);
        g.setFont (font (14.0f));
        g.setColour (col::text);
        g.drawText (text(), r.reduced (10.0f, 0.0f), juce::Justification::centredLeft, false);
    }
    void mouseEnter (const juce::MouseEvent&) override { repaint(); }
    void mouseExit (const juce::MouseEvent&) override { repaint(); }
    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { menu(); return; }
        if (const auto* d = dest()) { start = d->amount; range = mod::range (d->target); }
    }
    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu() || dest() == nullptr) return;
        const float fine = e.mods.isShiftDown() ? 0.1f : 1.0f;
        const float v = juce::jlimit (-range, range, start - (float) e.getDistanceFromDragStartY() / 160.0f * range * fine);
        const int l = lfo, i = index;
        ctx.edit ([l, i, v] (VaultPreset& p) { auto& d = p.mod.lfo[l].dests; if (juce::isPositiveAndBelow (i, (int) d.size())) d[(size_t) i].amount = v; });
    }
    void mouseDoubleClick (const juce::MouseEvent&) override { remove(); }
    void remove()
    {
        const int l = lfo, i = index;
        ctx.edit ([l, i] (VaultPreset& p) { auto& d = p.mod.lfo[l].dests; if (juce::isPositiveAndBelow (i, (int) d.size())) d.erase (d.begin() + i); });
    }
    void menu()
    {
        juce::PopupMenu m;
        m.setLookAndFeel (&getLookAndFeel());
        m.addItem (1, "Invert");
        m.addItem (2, "Remove");
        juce::Component::SafePointer<Chip> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
        {
            if (safe == nullptr) return;
            if (r == 2) { safe->remove(); return; }
            if (r == 1)
            {
                const int l = safe->lfo, i = safe->index;
                safe->ctx.edit ([l, i] (VaultPreset& p) { auto& d = p.mod.lfo[l].dests; if (juce::isPositiveAndBelow (i, (int) d.size())) d[(size_t) i].amount = -d[(size_t) i].amount; });
            }
        });
    }
    UiContext& ctx;
    int lfo, index;
    float start = 0, range = 1;
};

//==============================================================================
struct ModPage::LfoCard : public juce::Component
{
    LfoCard (UiContext& c, int i) : ctx (c), lfo (i)
    {
        noFocus (*this);
        addAndMakeVisible (power);
        power.onClick = [this]
        {
            // off = depth really 0 (also when a sound macro sets the depth - most presets do: its range goes to 0 too),
            // on = back as it was
            const int l = lfo;
            const auto c = val::Ctx::of (ctx.proc);
            const bool on = val::lfoValue (ctx.sound(), c, l, false) > 0.0f;
            const juce::String depthT = "lfo" + juce::String (l + 1) + ".depth";
            ctx.edit ([&] (VaultPreset& p)
            {
                auto& d = p.mod.lfo[l].depth;
                if (on)
                {
                    lastDepth = d;
                    lastTargets.clear();
                    for (int m = 0; m < vault::numSoundMacros; ++m)
                        for (int i = 0; i < (int) p.macros[(size_t) m].targets.size(); ++i)
                            if (auto& t = p.macros[(size_t) m].targets[(size_t) i]; t.target == depthT)
                            { lastTargets.push_back ({ m, i, t.min, t.max }); t.min = t.max = 0.0f; }
                    d = 0.0f;
                    return;
                }
                d = lastDepth > 0.0f ? lastDepth : 1.0f;
                for (auto& [m, i, mn, mx] : lastTargets)
                    if (juce::isPositiveAndBelow (i, (int) p.macros[(size_t) m].targets.size()) && p.macros[(size_t) m].targets[(size_t) i].target == depthT)
                    { p.macros[(size_t) m].targets[(size_t) i].min = mn; p.macros[(size_t) m].targets[(size_t) i].max = mx; }
                if (lastTargets.empty())
                    for (auto& mac : p.macros)
                        for (auto& t : mac.targets)
                            if (t.target == depthT && t.min <= 0.0f && t.max <= 0.0f) t.min = t.max = 1.0f;
                lastTargets.clear();
            });
        };
        addAndMakeVisible (shapes);
        shapes.setOptions ({ "Sine", "Triangle", "Square", "Saw", "S&H" });
        shapes.onSelect = [this] (int s) { const int l = lfo; ctx.edit ([l, s] (VaultPreset& p) { p.mod.lfo[l].shape = (uint8_t) s; }); };
        addAndMakeVisible (syncSeg);
        syncSeg.upper = true;
        syncSeg.padX = 9;
        syncSeg.onSelect = [this] (int s)
        {
            const int l = lfo;
            ctx.edit ([l, s] (VaultPreset& p)
            {
                auto& L = p.mod.lfo[l];
                L.sync = s == 0;
                if (L.sync)   // the division nearest to the free rate at 120 BPM
                {
                    int best = 0; double err = 1e9;
                    const auto& dv = vault::divisions();
                    for (int k = 0; k < (int) dv.size(); ++k)
                    {
                        const double hz = 2.0 / dv[(size_t) k].beats;
                        const double e = std::abs (std::log (hz / std::max (0.01, (double) L.rate)));
                        if (e < err) { err = e; best = k; }
                    }
                    L.div = (uint8_t) best;
                }
            });
        };
        addAndMakeVisible (retrigSeg);
        retrigSeg.upper = true;
        retrigSeg.padX = 9;
        retrigSeg.onSelect = [this] (int s) { const int l = lfo; ctx.edit ([l, s] (VaultPreset& p) { p.mod.lfo[l].retrig = s == 0; }); };
        for (auto* k : { &rate, &delay, &fade, &phase }) { k->diameter = 36; addAndMakeVisible (*k); }
        rate.label = "RATE"; delay.label = "DELAY"; fade.label = "FADE"; phase.label = "PHASE";
        delay.valueText = [] (float n) { return secondsText (n * n * 10.0f); };
        fade.valueText = [] (float n) { return secondsText (n * n * 10.0f); };
        phase.valueText = [] (float n) { return juce::String (juce::roundToInt (n * 360.0f)) + juce::String (juce::CharPointer_UTF8 ("\xc2\xb0")); };
        rate.onChange = [this] (float n)
        {
            const int l = lfo;
            const auto c = val::Ctx::of (ctx.proc);
            ctx.edit ([l, n, c] (VaultPreset& p)
            {
                auto& L = p.mod.lfo[l];
                if (L.sync) L.div = (uint8_t) juce::roundToInt (n * (float) (vault::divisions().size() - 1));
                else val::setLfoValue (p, c, l, true, 0.05f * std::pow (400.0f, n));   // (a sound macro may set the rate)
            });
        };
        delay.onChange = [this] (float n) { const int l = lfo; ctx.edit ([l, n] (VaultPreset& p) { p.mod.lfo[l].delay = n * n * 10.0f; }); };
        fade.onChange = [this] (float n) { const int l = lfo; ctx.edit ([l, n] (VaultPreset& p) { p.mod.lfo[l].fade = n * n * 10.0f; }); };
        phase.onChange = [this] (float n) { const int l = lfo; ctx.edit ([l, n] (VaultPreset& p) { p.mod.lfo[l].phase = n; }); };
        rate.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, rate, "lfo" + juce::String (lfo + 1) + ".rate"); };
        addAndMakeVisible (addBtn);
        addBtn.onClick = [this] { addMenu(); };
    }

    void addMenu()
    {
        juce::PopupMenu m;
        m.setLookAndFeel (&getLookAndFeel());
        juce::StringArray ids;
        mod::addTargetsMenu (m, ctx.sound(), 1, ids);
        juce::Component::SafePointer<LfoCard> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&addBtn), [safe, ids] (int r)
        {
            if (safe == nullptr || r <= 0 || r > ids.size()) return;
            const auto target = ids[r - 1];
            const int l = safe->lfo;
            const float a = juce::String (target) == "pitch" ? 0.15f : 0.25f * mod::range (target);
            safe->ctx.edit ([l, target, a] (VaultPreset& p)
            {
                auto& L = p.mod.lfo[l];
                for (auto& d : L.dests) if (d.target == target) return;
                if ((int) L.dests.size() < vault::maxLfoDests) L.dests.push_back ({ target, a });
                if (L.depth <= 0.0f) L.depth = 1.0f;
            });
        });
    }

    void refresh()
    {
        const auto& L = ctx.sound().mod.lfo[lfo];
        const auto c = val::Ctx::of (ctx.proc);
        power.setToggleState (val::lfoValue (ctx.sound(), c, lfo, false) > 0.0f && ! L.dests.empty(), juce::dontSendNotification);   // lit = it moves something
        shapes.setSelected (L.shape);
        syncSeg.setSelected (L.sync ? 0 : 1);
        retrigSeg.setSelected (L.retrig ? 0 : 1);
        const int nd = (int) vault::divisions().size();
        rate.steps = L.sync ? nd : 0;
        rate.sensitivity = L.sync ? 0.6f : 1.0f;
        if (L.sync) { rate.setValue ((float) L.div / (float) (nd - 1)); rate.valueText = [nd] (float n) { return juce::String (vault::divisions()[(size_t) juce::roundToInt (n * (float) (nd - 1))].name); }; }
        else { rate.setValue (std::log (juce::jlimit (0.05f, 20.0f, val::lfoValue (ctx.sound(), c, lfo, true)) / 0.05f) / std::log (400.0f)); rate.valueText = [] (float n) { return juce::String (0.05f * std::pow (400.0f, n), 2) + " Hz"; }; }
        delay.setValue (std::sqrt (juce::jlimit (0.0f, 10.0f, L.delay) / 10.0f));
        fade.setValue (std::sqrt (juce::jlimit (0.0f, 10.0f, L.fade) / 10.0f));
        phase.setValue (L.phase);
        const auto& B = ctx.bankSound().mod.lfo[lfo];
        delay.resetValue = std::sqrt (B.delay / 10.0f); fade.resetValue = std::sqrt (B.fade / 10.0f); phase.resetValue = B.phase;
        rate.resetValue = B.sync == L.sync ? (L.sync ? (float) B.div / (float) (nd - 1) : std::log (juce::jlimit (0.05f, 20.0f, B.rate) / 0.05f) / std::log (400.0f)) : rate.getValue();
        mod::show (rate, ctx.sound(), "lfo" + juce::String (lfo + 1) + ".rate");
        for (auto* k : { &rate, &delay, &fade, &phase }) k->repaint();
        // chips
        if ((int) chips.size() != (int) L.dests.size())
        {
            chips.clear();
            for (int i = 0; i < (int) L.dests.size(); ++i) { chips.push_back (std::make_unique<Chip> (ctx, lfo, i)); addAndMakeVisible (*chips.back()); }
        }
        addBtn.setVisible ((int) L.dests.size() < vault::maxLfoDests);
        layoutChips();
        for (auto& c : chips) c->repaint();
        repaint();
    }

    void layoutChips()
    {
        int x = 14;
        const int y = 232 + waveExtra;
        for (auto& c : chips) { const int w = c->idealWidth(); c->setBounds (x, y, w, 30); x += w + 8; }
        addBtn.setBounds (x, y, addBtn.idealWidth(), 30);
    }

    void resized() override
    {
        const int w = getWidth();
        power.setBounds (14, 14, 22, 22);
        shapes.setBounds (w - 14 - shapes.idealWidth(), 11, shapes.idealWidth(), 30);
        int x = 10;
        for (auto* k : { &rate, &delay, &fade, &phase }) { k->setBounds (x, 124 + waveExtra, 70, k->idealHeight()); x += 70; }
        syncSeg.setBounds (x + 6, 132 + waveExtra, syncSeg.idealWidth(), 30);
        retrigSeg.setBounds (syncSeg.getRight() + 8, 132 + waveExtra, retrigSeg.idealWidth(), 30);
        layoutChips();
    }

    juce::Rectangle<float> waveRect() const { return { 14.0f, 52.0f, (float) getWidth() - 28.0f, 62.0f + (float) waveExtra }; }

    void paint (juce::Graphics& g) override
    {
        drawPanel (g, getLocalBounds().toFloat());
        const auto& L = ctx.sound().mod.lfo[lfo];
        const bool on = L.depth > 0.0f && ! L.dests.empty();   // an LFO with no destination is shown off (it moves nothing)
        g.setFont (font (18.0f, true));
        g.setColour (on ? col::text : col::dim);
        g.drawText ("LFO " + juce::String (lfo + 1), juce::Rectangle<float> (44.0f, 10.0f, 120.0f, 30.0f), juce::Justification::centredLeft, false);
        auto w = waveRect();
        g.setColour (col::field);
        g.fillRoundedRectangle (w, 8.0f);
        g.setColour (col::wellLine);
        g.drawRoundedRectangle (w.reduced (0.5f), 8.0f, 1.0f);
        auto in = w.reduced (2.0f, 9.0f);
        juce::Path p;
        juce::Random rr (7 + lfo);
        float sh = rr.nextFloat() * 2.0f - 1.0f, lastPh = 0;
        const int n = 240;
        const float cycles = 2.0f;
        for (int i = 0; i <= n; ++i)
        {
            const float t = (float) i / (float) n * cycles;
            const float ph = (t + L.phase) - std::floor (t + L.phase);
            if (ph < lastPh) sh = rr.nextFloat() * 2.0f - 1.0f;
            lastPh = ph;
            const float v = vault::lfoValue (L.shape, ph, sh);
            const float x = in.getX() + (float) i / (float) n * in.getWidth();
            const float y = in.getCentreY() - v * in.getHeight() * 0.5f;
            if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
        }
        g.setColour (on ? col::ok : col::dim);
        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        // where the LFO is now (while notes play)
        if (on && livePhase >= 0.0f)
        {
            const float x = in.getX() + livePhase / cycles * in.getWidth();
            g.setColour (col::ok.withAlpha (0.35f));
            g.fillRect (x - 0.5f, in.getY() - 4.0f, 1.5f, in.getHeight() + 8.0f);
            g.setColour (col::ok);
            g.fillEllipse (juce::Rectangle<float> (7.0f, 7.0f).withCentre ({ x, in.getCentreY() - liveValue * in.getHeight() * 0.5f }));
        }
        drawHeader (g, "DESTINATIONS", { 14.0f, 206.0f + (float) waveExtra, 300.0f, 20.0f });
        if (L.dests.empty())
        {
            g.setFont (font (14.0f));
            g.setColour (col::dim);
            g.drawText (juce::String (juce::CharPointer_UTF8 ("or right-click any knob \xe2\x86\x92 Modulate with\xe2\x80\xa6")),
                        juce::Rectangle<float> ((float) addBtn.getRight() + 12.0f, 232.0f, (float) getWidth() - (float) addBtn.getRight() - 26.0f, 30.0f),
                        juce::Justification::centredLeft, true);
        }
    }

    void tick()
    {
        const float ph = ctx.proc.audioEngine().lfoPhase (lfo), v = ctx.proc.lfoOutput (lfo);
        const bool moving = std::abs (ph - livePhaseRaw) > 1.0e-6f;
        livePhaseRaw = ph;
        const float shown = moving ? ph : -1.0f;
        if (std::abs (shown - livePhase) > 1.0e-4f || std::abs (v - liveValue) > 1.0e-3f)
        {
            livePhase = shown;
            liveValue = juce::jlimit (-1.0f, 1.0f, v / std::max (0.001f, ctx.sound().mod.lfo[lfo].depth));
            repaint (waveRect().toNearestInt().expanded (4));
        }
    }

    UiContext& ctx;
    int lfo;
    std::vector<std::tuple<int, int, float, float>> lastTargets;   // the depth macro ranges while switched off
    float lastDepth = 1.0f, livePhase = -1.0f, livePhaseRaw = -1.0f, liveValue = 0.0f;
    PowerButton power;
    Segments shapes, syncSeg { { "SYNC", "HZ" } }, retrigSeg { { "RETRIG", "FREE" } };
    Knob rate, delay, fade, phase;
    std::vector<std::unique_ptr<Chip>> chips;
    Pill addBtn { "Add", Pill::Icon::plus };
};

//==============================================================================
ModPage::ModPage (UiContext& c) : Page (c)
{
    for (int i = 0; i < vault::numLfos; ++i)
    {
        lfos[(size_t) i] = std::make_unique<LfoCard> (ctx, i);
        addAndMakeVisible (*lfos[(size_t) i]);
    }
    auto addRoute = [this] (int source, juce::Component& anchor)
    {
        juce::PopupMenu m;
        m.setLookAndFeel (&getLookAndFeel());
        juce::StringArray ids;
        mod::addTargetsMenu (m, ctx.sound(), 1, ids);
        juce::Component::SafePointer<ModPage> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&anchor), [safe, ids, source] (int r)
        {
            if (safe == nullptr || r <= 0 || r > ids.size()) return;
            const auto target = ids[r - 1];
            const bool pitch = target == "pitch";
            const float a = source == 0 ? (pitch ? 2.0f : 0.5f * mod::range (target)) : (pitch ? 0.5f : 0.3f * mod::range (target));
            safe->ctx.edit ([source, target, a] (VaultPreset& p)
            {
                auto& l = routeList (p, source);
                for (auto& d : l) if (d.target == target) return;
                if ((int) l.size() < vault::maxRoutings) l.push_back ({ target, a });
            });
        });
    };
    addAndMakeVisible (addWheel); addAndMakeVisible (addVel); addAndMakeVisible (addAt);
    addWheel.onClick = [this, addRoute] { addRoute (0, addWheel); };
    addVel.onClick = [this, addRoute] { addRoute (1, addVel); };
    addAt.onClick = [this, addRoute] { addRoute (2, addAt); };
    refresh();
}

ModPage::~ModPage() = default;

void ModPage::resized()
{
    const int cw = (getWidth() - gap) / 2;
    for (int i = 0; i < vault::numLfos; ++i) lfos[(size_t) i]->setBounds (i * (cw + gap), 0, cw, cardH);
    routesR = { 0, cardH + gap, getWidth(), getHeight() - cardH - gap };
    int x = routesR.getRight() - 16;
    for (auto* b : { &addAt, &addVel, &addWheel }) { x -= b->idealWidth(); b->setBounds (x, routesR.getY() + 12, b->idealWidth(), 30); x -= 8; }
    // the routings grouped by source: "MOD WHEEL" / "VELOCITY" / "AFTERTOUCH" over each group, the amount always shown
    routeGroups.clear();
    int nGroups = 0, prev = -1;
    for (auto& id : routeIds) if (id.first != prev) { ++nGroups; prev = id.first; }
    const int routeKnobW = routeKnobs.empty() ? maxRouteKnobW
                         : std::min (maxRouteKnobW, (routesR.getWidth() - 20 - 18 * std::max (0, nGroups - 1)) / (int) routeKnobs.size());
    int x0 = routesR.getX() + 10, prevSrc = -1;
    // the captions + knobs sit in the middle of the panel's free height (under its title row)
    const int blockH = 26 + Knob::heightFor (44);
    const int capY = routesR.getY() + 50 + std::max (12, (routesR.getHeight() - 50 - blockH) / 2 - 6);
    for (int i = 0; i < (int) routeKnobs.size(); ++i)
    {
        const int src = routeIds[(size_t) i].first;
        if (src != prevSrc)
        {
            if (prevSrc >= 0) x0 += 18;
            routeGroups.push_back ({ src, { x0, capY, 0, 18 } });
            prevSrc = src;
        }
        routeKnobs[(size_t) i]->setBounds (x0, capY + 26, routeKnobW, routeKnobs[(size_t) i]->idealHeight());
        routeGroups.back().second.setRight (x0 + routeKnobW);
        x0 += routeKnobW;
    }
}

void ModPage::paint (juce::Graphics& g)
{
    drawPanel (g, routesR.toFloat());
    const juce::String t ("MOD WHEEL" + upDot() + "VELOCITY" + upDot() + "AFTERTOUCH");
    const auto f = font (16.0f, true);
    g.setFont (f);
    g.setColour (col::text);
    g.drawText (t, juce::Rectangle<float> ((float) routesR.getX() + 16.0f, (float) routesR.getY() + 12.0f, 520.0f, 30.0f), juce::Justification::centredLeft, false);
    g.setFont (font (14.0f));
    g.setColour (col::dim);
    const float x = (float) routesR.getX() + 16.0f + textWidth (f, t) + 16.0f;
    g.drawText (juce::String (juce::CharPointer_UTF8 ("right-click any knob \xe2\x86\x92 Modulate with\xe2\x80\xa6 \xc2\xb7 a green ring shows the amount")),
                juce::Rectangle<float> (x, (float) routesR.getY() + 12.0f, (float) addWheel.getX() - x - 10.0f, 30.0f), juce::Justification::centredLeft, true);
    for (auto& [src, r] : routeGroups)
    {
        const juce::String name (src == 0 ? "MOD WHEEL" : src == 1 ? "VELOCITY" : "AFTERTOUCH");
        drawHeader (g, name, r.toFloat().withSizeKeepingCentre (std::max ((float) r.getWidth(), headerWidth (name) + 6.0f), (float) r.getHeight()), juce::Justification::centred);
        g.setColour (col::line3);
        const float tw = headerWidth (name);
        const float cx = (float) r.getCentreX(), y = (float) r.getCentreY();
        if (r.getWidth() > tw + 30.0f)
        {
            g.fillRect ((float) r.getX() + 8.0f, y, cx - tw * 0.5f - 8.0f - ((float) r.getX() + 8.0f), 1.0f);
            g.fillRect (cx + tw * 0.5f + 8.0f, y, (float) r.getRight() - 8.0f - (cx + tw * 0.5f + 8.0f), 1.0f);
        }
    }
    if (routeKnobs.empty())
    {
        g.drawText ("No routings yet: the mod wheel, velocity and aftertouch can move any knob.",
                    juce::Rectangle<float> ((float) routesR.getX() + 16.0f, (float) routesR.getY() + 70.0f, (float) routesR.getWidth() - 32.0f, 24.0f),
                    juce::Justification::centredLeft, true);
    }
}

void ModPage::rebuildRoutes()
{
    const auto& snd = ctx.sound();
    juce::String key;
    for (int s = 0; s < 3; ++s) for (auto& d : routeList (snd, s)) key << s << d.target << ";";
    if (key == routeKey) return;
    routeKey = key;
    routeKnobs.clear();
    routeIds.clear();
    for (int s = 0; s < 3; ++s)
        for (int i = 0; i < (int) routeList (snd, s).size(); ++i)
        {
            const auto target = routeList (snd, s)[(size_t) i].target;
            auto k = std::make_unique<Knob> (mod::shortName (target, &snd).toUpperCase(), 44);
            k->arcOrigin = 0.5f;
            k->resetValue = 0.5f;
            const float r = mod::range (target);
            k->valueAlways = true;
            k->valueText = [target, r] (float n) { return mod::amountText (target, (n - 0.5f) * 2.0f * r); };
            k->onChange = [this, s, i, r] (float n)
            {
                ctx.edit ([s, i, r, n] (VaultPreset& p) { auto& l = routeList (p, s); if (juce::isPositiveAndBelow (i, (int) l.size())) l[(size_t) i].amount = (n - 0.5f) * 2.0f * r; });
            };
            Knob* kp = k.get();
            k->onRightClick = [this, s, i, kp] (const juce::MouseEvent&)
            {
                juce::PopupMenu m;
                m.setLookAndFeel (&getLookAndFeel());
                m.addItem (1, "Remove");
                juce::Component::SafePointer<ModPage> safe (this);
                m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (kp), [safe, s, i] (int res)
                {
                    if (safe == nullptr || res != 1) return;
                    safe->ctx.edit ([s, i] (VaultPreset& p) { auto& l = routeList (p, s); if (juce::isPositiveAndBelow (i, (int) l.size())) l.erase (l.begin() + i); });
                });
            };
            k->setTooltip (juce::String (s == 0 ? "Mod wheel" : s == 1 ? "Velocity" : "Aftertouch") + juce::String (juce::CharPointer_UTF8 (" \xe2\x86\x92 "))
                           + mod::longName (target, &snd) + " (right-click: remove)");
            addAndMakeVisible (*k);
            routeKnobs.push_back (std::move (k));
            routeIds.push_back ({ s, i });
        }
    resized();
    repaint();
}

void ModPage::refresh()
{
    for (auto& l : lfos) l->refresh();
    rebuildRoutes();
    const auto& snd = ctx.sound();
    for (int k = 0; k < (int) routeKnobs.size(); ++k)
    {
        const auto [s, i] = routeIds[(size_t) k];
        const auto& l = routeList (snd, s);
        if (! juce::isPositiveAndBelow (i, (int) l.size())) continue;
        const float r = mod::range (l[(size_t) i].target);
        routeKnobs[(size_t) k]->setValue (0.5f + l[(size_t) i].amount / (2.0f * r));
        routeKnobs[(size_t) k]->setModulated (true, 0.0f);
        routeKnobs[(size_t) k]->repaint();
    }
    const bool full[] { (int) snd.mod.modWheel.size() >= vault::maxRoutings, (int) snd.mod.velocity.size() >= vault::maxRoutings,
                        (int) snd.mod.aftertouch.size() >= vault::maxRoutings };
    addWheel.setEnabled (! full[0]); addVel.setEnabled (! full[1]); addAt.setEnabled (! full[2]);
    addWheel.dimmed = full[0]; addVel.dimmed = full[1]; addAt.dimmed = full[2];
}

void ModPage::tick()
{
    for (auto& l : lfos) l->tick();
}
} // namespace vui
