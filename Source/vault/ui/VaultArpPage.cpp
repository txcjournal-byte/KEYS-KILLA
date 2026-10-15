#include "VaultPages.h"

// CHROME VAULT v3 ARP · GATE page: the arpeggiator (modes, rate + straight / triplet / dotted, octaves, gate, swing, steps,
// latch, 16-step velocity / on-tie / octave lanes, MIDI drag) and the gate (16 steps, rate, depth, smooth, swing, mix).
// Patterns start from "Init"; both are off unless the sound turns them on.
namespace vui
{
//==============================================================================
// one 16-step lane
class StepLane : public juce::Component, public juce::SettableTooltipClient
{
public:
    enum class Kind { velocity, onTie, octave, gate };
    StepLane (Kind k, const juce::String& l) : kind (k), label (l)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        if (kind == Kind::onTie) setTooltip ("Click: step on / off. Right-click (or alt-click): tie it to the step before");
        if (kind == Kind::octave) setTooltip ("Click: one octave up, right-click: one octave down");
        if (kind == Kind::velocity || kind == Kind::gate) setTooltip ("Click or drag over the steps to draw");
    }
    std::function<float (int)> get;              // velocity 0..1, type (VaultArp::StepType), octave -2..2, gate level 0..1
    std::function<void (int, float)> set;
    std::function<bool (int)> isRest;            // velocity lane: the step plays nothing (dimmed)
    int steps = 16, current = -1;
    static constexpr int labelW = 96, cellGap = 4;

    juce::Rectangle<float> cell (int i) const
    {
        const float w = ((float) getWidth() - (float) labelW - 15.0f * cellGap) / 16.0f;
        return { (float) labelW + (float) i * (w + cellGap), 0.0f, w, (float) getHeight() };
    }
    void setCurrent (int c) { if (c != current) { current = c; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        auto f = font (14.0f, false, 0.12f);
        g.setFont (f);
        g.setColour (col::dim);
        g.drawText (label, juce::Rectangle<float> (0.0f, 0.0f, (float) labelW - 6.0f, (float) getHeight()), juce::Justification::centredLeft, false);
        for (int i = 0; i < 16; ++i)
        {
            const auto c = cell (i);
            const float a = i < steps ? 1.0f : 0.3f;
            const float v = get ? get (i) : 0.0f;
            switch (kind)
            {
                case Kind::velocity: case Kind::gate:
                {
                    g.setColour (col::stepOff.withAlpha (a));
                    g.fillRoundedRectangle (c, 3.0f);
                    const bool rest = isRest && isRest (i);
                    const float h = std::max (kind == Kind::gate ? 0.0f : 3.0f, v * c.getHeight());
                    if (h > 0.5f)
                    {
                        auto bar = c.withTop (c.getBottom() - h);
                        if (rest) g.setColour (col::ice2.withAlpha (0.25f * a));
                        else g.setGradientFill (juce::ColourGradient (col::ice.withAlpha (a), 0, bar.getY(), col::deep.withAlpha (a), 0, c.getBottom(), false));
                        g.fillRoundedRectangle (bar, 3.0f);
                    }
                    break;
                }
                case Kind::onTie:
                {
                    const int t = juce::roundToInt (v);
                    if (t == VaultArp::rest) { g.setColour (col::stepOff.withAlpha (a)); g.fillRoundedRectangle (c, 3.0f); }
                    else
                    {
                        auto bar = c;
                        if (t == VaultArp::tie && i > 0) bar = bar.withLeft (bar.getX() - (float) cellGap - 3.0f);   // joined to the step before
                        g.setGradientFill (juce::ColourGradient (col::ice.withAlpha (a), 0, bar.getY(), col::deep.withAlpha (a), 0, bar.getBottom(), false));
                        g.fillRoundedRectangle (bar, 3.0f);
                        if (t == VaultArp::tie)
                        {
                            g.setColour (col::ink.withAlpha (0.8f));
                            g.setFont (font (14.0f, true));
                            g.drawText ("TIE", bar, juce::Justification::centred, false);
                        }
                    }
                    break;
                }
                case Kind::octave:
                {
                    g.setColour (col::stepOff.withAlpha (a));
                    g.fillRoundedRectangle (c, 3.0f);
                    const int o = juce::roundToInt (v);
                    g.setFont (font (14.0f));
                    g.setColour ((o != 0 ? col::ice : col::dim).withAlpha (std::max (a, 0.6f)));
                    g.drawText ((o > 0 ? "+" : "") + juce::String (o), c, juce::Justification::centred, false);
                    break;
                }
            }
            if (i == current)
            {
                g.setColour (col::silver);
                g.drawRoundedRectangle (c.expanded (1.0f), 3.5f, 1.2f);
            }
        }
    }

    int cellAt (float x) const
    {
        for (int i = 0; i < 16; ++i) if (cell (i).expanded ((float) cellGap * 0.5f, 0.0f).getHorizontalRange().contains (x)) return i;
        return -1;
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = cellAt (e.position.x);
        if (i < 0 || ! set || ! get) return;
        const bool alt = e.mods.isPopupMenu() || e.mods.isAltDown();
        switch (kind)
        {
            case Kind::velocity: case Kind::gate: draw (e); break;
            case Kind::onTie:
            {
                const int t = juce::roundToInt (get (i));
                if (alt) set (i, (float) (t == VaultArp::tie ? VaultArp::play : VaultArp::tie));
                else set (i, (float) (t == VaultArp::rest ? VaultArp::play : VaultArp::rest));
                break;
            }
            case Kind::octave:
            {
                int o = juce::roundToInt (get (i)) + (alt ? -1 : 1);
                if (o > 2) o = -2;
                if (o < -2) o = 2;
                set (i, (float) o);
                break;
            }
        }
        repaint();
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (kind == Kind::velocity || kind == Kind::gate) draw (e); }

private:
    void draw (const juce::MouseEvent& e)
    {
        const int i = cellAt (e.position.x);
        if (i < 0) return;
        const auto c = cell (i);
        float v = juce::jlimit (0.0f, 1.0f, (c.getBottom() - e.position.y) / c.getHeight());
        if (kind == Kind::gate && e.getDistanceFromDragStart() < 3 && e.mouseWasClicked())
            v = get (i) > 0.5f ? 0.0f : 1.0f;
        if (kind == Kind::velocity) v = std::max (0.05f, v);
        set (i, v);
        repaint();
    }
    Kind kind;
    juce::String label;
};

//==============================================================================
namespace
{
constexpr int arpH = 300, gap = 12, gateH = 214;
juce::String divName (int i) { return vault::divisions()[(size_t) juce::jlimit (0, (int) vault::divisions().size() - 1, i)].name; }
bool sameLanes (const VaultArp& a, const VaultArp& b) { return a.vel == b.vel && a.type == b.type && a.oct == b.oct && a.steps == b.steps; }
bool sameGate (const VaultGate& a, const VaultGate& b) { return a.level == b.level && a.steps == b.steps; }
} // namespace

const juce::StringArray& ArpGatePage::rateBases()
{
    static const juce::StringArray b { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32" };
    return b;
}
int ArpGatePage::rateIndex (int base, int feel)
{
    const auto name = rateBases()[juce::jlimit (0, rateBases().size() - 1, base)] + (feel == 1 ? "T" : feel == 2 ? "D" : "");
    int i = vault::divisionIndex (name.toRawUTF8());
    if (i < 0) i = vault::divisionIndex (rateBases()[juce::jlimit (0, rateBases().size() - 1, base)].toRawUTF8());
    return std::max (0, i);
}
std::pair<int, int> ArpGatePage::baseAndFeel (int div)
{
    auto n = divName (div);
    int feel = 0;
    if (n.endsWith ("T")) { feel = 1; n = n.dropLastCharacters (1); }
    else if (n.endsWith ("D")) { feel = 2; n = n.dropLastCharacters (1); }
    const int b = rateBases().indexOf (n);
    return { b >= 0 ? b : 4, feel };
}

ArpGatePage::ArpGatePage (UiContext& c) : Page (c)
{
    auto& proc = ctx.proc;
    velLane = std::make_unique<StepLane> (StepLane::Kind::velocity, "VELOCITY");
    typeLane = std::make_unique<StepLane> (StepLane::Kind::onTie, "ON / TIE");
    octLane = std::make_unique<StepLane> (StepLane::Kind::octave, "OCTAVE");
    gateLane = std::make_unique<StepLane> (StepLane::Kind::gate, "STEPS");
    for (auto* l : { velLane.get(), typeLane.get(), octLane.get(), gateLane.get() }) addAndMakeVisible (*l);
    velLane->get = [this] (int i) { return ctx.sound().arp.vel[(size_t) i]; };
    velLane->isRest = [this] (int i) { return ctx.sound().arp.type[(size_t) i] == VaultArp::rest; };
    velLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.arp.vel[(size_t) i] = v; }); };
    typeLane->get = [this] (int i) { return (float) ctx.sound().arp.type[(size_t) i]; };
    typeLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.arp.type[(size_t) i] = (uint8_t) juce::roundToInt (v); }); };
    octLane->get = [this] (int i) { return (float) ctx.sound().arp.oct[(size_t) i]; };
    octLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.arp.oct[(size_t) i] = (int8_t) juce::roundToInt (v); }); };
    gateLane->get = [this] (int i) { return ctx.sound().gate.level[(size_t) i]; };
    gateLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.gate.level[(size_t) i] = v; }); };

    // ---- arpeggiator
    addAndMakeVisible (arpPower);
    arpPower.onClick = [this] { ctx.proc.setSwitch (VaultProcessor::idArpOn, ! ctx.proc.getSwitch (VaultProcessor::idArpOn)); if (ctx.soundEdited) ctx.soundEdited(); };
    addAndMakeVisible (arpPattern);
    arpPattern.onClick = [this] { arpPatternMenu(); };
    addAndMakeVisible (latchBtn);
    latchBtn.onClick = [this] { ctx.edit ([] (VaultPreset& p) { p.arp.latch = ! p.arp.latch; }); };
    latchBtn.setTooltip ("Latch: the pattern keeps playing after the keys are released (play new keys to change it)");
    addAndMakeVisible (arpKeep);
    arpKeep.onClick = [this]
    {
        const bool l = ! ctx.proc.getLock (VaultProcessor::Section::arp);
        ctx.proc.setLock (VaultProcessor::Section::arp, l);
        ctx.say (l ? "The arpeggiator stays when you pick another sound" : "Every sound brings its own arpeggiator settings again");
        if (ctx.soundEdited) ctx.soundEdited();
    };
    addAndMakeVisible (modeSeg);
    modeSeg.onSelect = [this] (int m) { ctx.edit ([m] (VaultPreset& p) { p.arp.mode = (uint8_t) m; }); };
    addAndMakeVisible (feelSeg);
    feelSeg.onSelect = [this] (int f)
    {
        const auto bf = baseAndFeel (ctx.sound().arp.rate);
        const int r = rateIndex (bf.first, f);
        ctx.edit ([r] (VaultPreset& p) { p.arp.rate = (uint8_t) r; });
    };
    arpRate.steps = rateBases().size();
    arpRate.valueText = [] (float n) { return rateBases()[juce::roundToInt (n * (float) (rateBases().size() - 1))]; };
    arpRate.onChange = [this] (float n)
    {
        const auto bf = baseAndFeel (ctx.sound().arp.rate);
        const int r = rateIndex (juce::roundToInt (n * (float) (rateBases().size() - 1)), bf.second);
        ctx.edit ([r] (VaultPreset& p) { p.arp.rate = (uint8_t) r; });
    };
    arpOct.steps = 4;
    arpOct.valueText = [] (float n) { return juce::String (1 + juce::roundToInt (n * 3.0f)); };
    arpOct.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.arp.octaves = (uint8_t) (1 + juce::roundToInt (n * 3.0f)); }); };
    arpGate.valueText = [] (float n) { return pct (0.05f + 0.95f * n); };
    arpGate.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.arp.gate = 0.05f + 0.95f * n; }); };
    arpGate.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, arpGate, "arp.gate"); };
    arpSwing.valueText = [] (float n) { return pct (n); };
    arpSwing.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.arp.swing = n; }); };
    arpSteps.steps = 16;
    arpSteps.valueText = [] (float n) { return juce::String (1 + juce::roundToInt (n * 15.0f)); };
    arpSteps.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.arp.steps = (uint8_t) (1 + juce::roundToInt (n * 15.0f)); }); };
    for (auto* k : { &arpRate, &arpOct, &arpGate, &arpSwing, &arpSteps }) addAndMakeVisible (*k);
    addAndMakeVisible (midiBtn);
    midiBtn.onDragOut = [this]
    {
        const auto f = ctx.proc.writeArpMidiFile (2.0);
        if (f.existsAsFile())
        {
            ctx.say ("Drop the MIDI into your DAW: 2 bars of the arp pattern");
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, &midiBtn);
        }
    };
    midiBtn.onClickOnly = [this] { ctx.say ("Drag this button into your DAW: the arp pattern as 2 bars of MIDI (held keys, else the C5 triad)"); };

    // ---- gate
    addAndMakeVisible (gatePower);
    gatePower.onClick = [this] { ctx.proc.setSwitch (VaultProcessor::idGateOn, ! ctx.proc.getSwitch (VaultProcessor::idGateOn)); if (ctx.soundEdited) ctx.soundEdited(); };
    addAndMakeVisible (gatePattern);
    gatePattern.onClick = [this] { gatePatternMenu(); };
    addAndMakeVisible (gateKeep);
    gateKeep.onClick = [this]
    {
        const bool l = ! ctx.proc.getLock (VaultProcessor::Section::gate);
        ctx.proc.setLock (VaultProcessor::Section::gate, l);
        ctx.say (l ? "The gate stays when you pick another sound" : "Every sound brings its own gate settings again");
        if (ctx.soundEdited) ctx.soundEdited();
    };
    const int nd = (int) vault::divisions().size();
    gRate.steps = nd;
    gRate.sensitivity = 0.6f;
    gRate.valueText = [nd] (float n) { return divName (juce::roundToInt (n * (float) (nd - 1))); };
    gRate.onChange = [this, nd] (float n) { ctx.edit ([n, nd] (VaultPreset& p) { p.gate.rate = (uint8_t) juce::roundToInt (n * (float) (nd - 1)); }); };
    gDepth.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.depth = n; }); };
    gSmooth.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.smooth = n; }); };
    gSwing.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.swing = n; }); };
    gMix.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.mix = n; }); };
    for (auto* k : { &gDepth, &gSmooth, &gSwing, &gMix }) k->valueText = [] (float n) { return pct (n); };
    gDepth.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, gDepth, "gate.depth"); };
    gSmooth.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, gSmooth, "gate.smooth"); };
    gMix.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, gMix, "gate.mix"); };
    for (auto* k : { &gRate, &gDepth, &gSmooth, &gSwing, &gMix }) addAndMakeVisible (*k);
    juce::ignoreUnused (proc);
    refresh();
}

ArpGatePage::~ArpGatePage() = default;

void ArpGatePage::resized()
{
    arpR = { 0, 0, getWidth(), arpH };
    gateR = { 0, arpH + gap, getWidth(), gateH };
    // arp header
    {
        const int y = arpR.getY() + 12;
        arpPower.setBounds (arpR.getX() + 16, y + 4, 22, 22);
        int x = arpR.getX() + 46 + (int) std::ceil (textWidth (font (18.0f, true, 0.02f), "ARPEGGIATOR")) + 16;
        arpPattern.setBounds (x, y + 1, arpPattern.idealWidth(), 28); x = arpPattern.getRight() + 8;
        latchBtn.setBounds (x, y + 1, latchBtn.idealWidth(), 28); x = latchBtn.getRight() + 8;
        arpKeep.setBounds (x, y + 1, arpKeep.idealWidth(), 28);
        const int mw = modeSeg.idealWidth();
        modeSeg.setBounds (arpR.getRight() - 16 - mw, y, mw, 30);
    }
    {
        const int y = arpR.getY() + 54;
        int x = arpR.getX() + 10;
        arpRate.setBounds (x, y, 88, arpRate.idealHeight()); x += 96;
        feelSeg.setBounds (x, y + 10, feelSeg.idealWidth(), 30); x = feelSeg.getRight() + 18;
        for (auto* k : { &arpOct, &arpGate, &arpSwing, &arpSteps }) { k->setBounds (x, y, 92, k->idealHeight()); x += 96; }
        midiBtn.setBounds (arpR.getRight() - 16 - midiBtn.idealWidth(), y + 10, midiBtn.idealWidth(), 30);
    }
    velLane->setBounds (arpR.getX() + 16, arpR.getY() + 150, arpR.getWidth() - 32, 62);
    typeLane->setBounds (arpR.getX() + 16, arpR.getY() + 220, arpR.getWidth() - 32, 26);
    octLane->setBounds (arpR.getX() + 16, arpR.getY() + 254, arpR.getWidth() - 32, 26);
    // gate
    {
        const int y = gateR.getY() + 12;
        gatePower.setBounds (gateR.getX() + 16, y + 4, 22, 22);
        int x = gateR.getX() + 46 + (int) std::ceil (textWidth (font (18.0f, true, 0.02f), "GATE")) + 16;
        gatePattern.setBounds (x, y + 1, gatePattern.idealWidth(), 28); x = gatePattern.getRight() + 8;
        gateKeep.setBounds (x, y + 1, gateKeep.idealWidth(), 28);
        int kx = gateR.getX() + 10;
        for (auto* k : { &gRate, &gDepth, &gSmooth, &gSwing, &gMix }) { k->setBounds (kx, gateR.getY() + 54, 92, k->idealHeight()); kx += 96; }
    }
    gateLane->setBounds (gateR.getX() + 16, gateR.getY() + 152, gateR.getWidth() - 32, 44);
}

void ArpGatePage::paint (juce::Graphics& g)
{
    drawPanel (g, arpR.toFloat());
    drawPanel (g, gateR.toFloat());
    g.setFont (font (18.0f, true, 0.02f));
    g.setColour (ctx.proc.getSwitch (VaultProcessor::idArpOn) ? col::text : col::muted);
    g.drawText ("ARPEGGIATOR", juce::Rectangle<float> ((float) arpR.getX() + 46.0f, (float) arpR.getY() + 12.0f, 200.0f, 30.0f), juce::Justification::centredLeft, false);
    g.setColour (ctx.proc.getSwitch (VaultProcessor::idGateOn) ? col::text : col::muted);
    g.drawText ("GATE", juce::Rectangle<float> ((float) gateR.getX() + 46.0f, (float) gateR.getY() + 12.0f, 100.0f, 30.0f), juce::Justification::centredLeft, false);
    g.setFont (font (14.0f));
    g.setColour (col::dim);
    const float nx = (float) gateKeep.getRight() + 16.0f;
    g.drawText (juce::String (juce::CharPointer_UTF8 ("rhythmic volume chop \xc2\xb7 synced to the DAW tempo")),
                juce::Rectangle<float> (nx, (float) gateR.getY() + 12.0f, (float) gateR.getRight() - 16.0f - nx, 30.0f), juce::Justification::centredRight, true);
}

void ArpGatePage::refresh()
{
    const auto& snd = ctx.sound();
    const auto& a = snd.arp;
    const auto& gt = snd.gate;
    const auto& bank = ctx.bankSound();
    arpPower.setToggleState (ctx.proc.getSwitch (VaultProcessor::idArpOn), juce::dontSendNotification);
    gatePower.setToggleState (ctx.proc.getSwitch (VaultProcessor::idGateOn), juce::dontSendNotification);
    latchBtn.setToggleState (a.latch, juce::dontSendNotification);
    arpKeep.setToggleState (ctx.proc.getLock (VaultProcessor::Section::arp), juce::dontSendNotification);
    gateKeep.setToggleState (ctx.proc.getLock (VaultProcessor::Section::gate), juce::dontSendNotification);
    modeSeg.setSelected (a.mode);
    const auto bf = baseAndFeel (a.rate);
    feelSeg.setSelected (bf.second);
    const float nb = (float) (rateBases().size() - 1);
    arpRate.setValue ((float) bf.first / nb);
    arpRate.resetValue = (float) baseAndFeel (bank.arp.rate).first / nb;
    arpOct.setValue ((float) (a.octaves - 1) / 3.0f);        arpOct.resetValue = (float) (bank.arp.octaves - 1) / 3.0f;
    arpGate.setValue ((a.gate - 0.05f) / 0.95f);             arpGate.resetValue = (bank.arp.gate - 0.05f) / 0.95f;
    arpSwing.setValue (a.swing);                             arpSwing.resetValue = bank.arp.swing;
    arpSteps.setValue ((float) (a.steps - 1) / 15.0f);       arpSteps.resetValue = (float) (bank.arp.steps - 1) / 15.0f;
    mod::show (arpGate, snd, "arp.gate");
    const int nd = (int) vault::divisions().size();
    gRate.setValue ((float) gt.rate / (float) (nd - 1));     gRate.resetValue = (float) bank.gate.rate / (float) (nd - 1);
    gDepth.setValue (gt.depth);                              gDepth.resetValue = bank.gate.depth;
    gSmooth.setValue (gt.smooth);                            gSmooth.resetValue = bank.gate.smooth;
    gSwing.setValue (gt.swing);                              gSwing.resetValue = bank.gate.swing;
    gMix.setValue (gt.mix);                                  gMix.resetValue = bank.gate.mix;
    mod::show (gDepth, snd, "gate.depth"); mod::show (gSmooth, snd, "gate.smooth"); mod::show (gMix, snd, "gate.mix");
    for (auto* k : { &arpRate, &arpOct, &arpGate, &arpSwing, &arpSteps, &gRate, &gDepth, &gSmooth, &gSwing, &gMix }) k->repaint();
    for (auto* l : { velLane.get(), typeLane.get(), octLane.get() }) { l->steps = a.steps; l->repaint(); }
    gateLane->steps = gt.steps;
    gateLane->repaint();
    const VaultArp init;
    const auto an = sameLanes (a, init) ? juce::String ("Init") : sameLanes (a, bank.arp) ? juce::String ("Preset") : juce::String ("Custom");
    if (arpPattern.getButtonText() != "Pattern: " + an) { arpPattern.setButtonText ("Pattern: " + an); resized(); }
    const VaultGate ginit;
    const auto gn = sameGate (gt, ginit) ? juce::String ("Init") : sameGate (gt, bank.gate) ? juce::String ("Preset") : juce::String ("Custom");
    if (gatePattern.getButtonText() != "Pattern: " + gn) { gatePattern.setButtonText ("Pattern: " + gn); resized(); }
    repaint (arpR.withHeight (44));
    repaint (gateR.withHeight (44));
}

void ArpGatePage::tick()
{
    const bool arpOn = ctx.proc.getSwitch (VaultProcessor::idArpOn);
    const int st = arpOn ? ctx.proc.arpStep() : -1;
    for (auto* l : { velLane.get(), typeLane.get(), octLane.get() }) l->setCurrent (st);
}

void ArpGatePage::arpPatternMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    m.addItem (1, "Init");
    m.addItem (2, "The sound's own pattern", ! sameLanes (ctx.bankSound().arp, VaultArp()));
    m.addSeparator();
    m.addItem (10, "All steps on");
    m.addItem (11, "Accent on the beats");
    m.addItem (12, "Velocity ramp up");
    m.addItem (13, "Velocity ramp down");
    m.addItem (14, "Random velocities");
    m.addItem (15, "Random octaves");
    m.addSeparator();
    m.addItem (20, "Shift left");
    m.addItem (21, "Shift right");
    m.addItem (22, "Reverse");
    juce::Component::SafePointer<ArpGatePage> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&arpPattern), [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        auto& c = safe->ctx;
        const auto own = c.bankSound().arp;
        juce::Random rng;
        c.edit ([&] (VaultPreset& p)
        {
            auto& a = p.arp;
            const VaultArp init;
            const int n = a.steps;
            switch (r)
            {
                case 1: { const bool keepOn = a.on; a = init; a.on = keepOn; break; }
                case 2: a.vel = own.vel; a.type = own.type; a.oct = own.oct; a.steps = own.steps; break;
                case 10: for (auto& t : a.type) t = VaultArp::play; break;
                case 11: for (int i = 0; i < 16; ++i) a.vel[(size_t) i] = i % 4 == 0 ? 1.0f : 0.68f; break;
                case 12: for (int i = 0; i < 16; ++i) a.vel[(size_t) i] = 0.35f + 0.65f * (float) (i % n) / (float) std::max (1, n - 1); break;
                case 13: for (int i = 0; i < 16; ++i) a.vel[(size_t) i] = 1.0f - 0.65f * (float) (i % n) / (float) std::max (1, n - 1); break;
                case 14: for (auto& v : a.vel) v = 0.45f + 0.55f * rng.nextFloat(); break;
                case 15: for (auto& o : a.oct) o = (int8_t) (rng.nextFloat() < 0.7f ? 0 : rng.nextBool() ? 1 : -1); break;
                case 20: case 21: case 22:
                {
                    auto rot = [&] (auto& arr)
                    {
                        if (r == 22) std::reverse (arr.begin(), arr.begin() + n);
                        else if (r == 20) std::rotate (arr.begin(), arr.begin() + 1, arr.begin() + n);
                        else std::rotate (arr.begin(), arr.begin() + n - 1, arr.begin() + n);
                    };
                    rot (a.vel); rot (a.type); rot (a.oct);
                    if (a.type[0] == VaultArp::tie) a.type[0] = VaultArp::play;
                    break;
                }
                default: break;
            }
        });
    });
}

void ArpGatePage::gatePatternMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    m.addItem (1, "Init");
    m.addItem (2, "The sound's own pattern", ! sameGate (ctx.bankSound().gate, VaultGate()));
    m.addSeparator();
    m.addItem (10, "All open");
    m.addItem (11, "Off-beats");
    m.addItem (12, "Invert");
    m.addItem (13, "Random");
    m.addSeparator();
    m.addItem (20, "Shift left");
    m.addItem (21, "Shift right");
    juce::Component::SafePointer<ArpGatePage> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&gatePattern), [safe] (int r)
    {
        if (safe == nullptr || r <= 0) return;
        auto& c = safe->ctx;
        const auto own = c.bankSound().gate;
        juce::Random rng;
        c.edit ([&] (VaultPreset& p)
        {
            auto& g = p.gate;
            const int n = g.steps;
            switch (r)
            {
                case 1: { const bool keepOn = g.on; g = VaultGate(); g.on = keepOn; break; }
                case 2: g.level = own.level; g.steps = own.steps; break;
                case 10: for (auto& v : g.level) v = 1.0f; break;
                case 11: for (int i = 0; i < 16; ++i) g.level[(size_t) i] = i % 2 == 1 ? 1.0f : 0.0f; break;
                case 12: for (auto& v : g.level) v = 1.0f - v; break;
                case 13: for (auto& v : g.level) v = rng.nextFloat() < 0.65f ? 1.0f : 0.0f; break;
                case 20: std::rotate (g.level.begin(), g.level.begin() + 1, g.level.begin() + n); break;
                case 21: std::rotate (g.level.begin(), g.level.begin() + n - 1, g.level.begin() + n); break;
                default: break;
            }
        });
    });
}
} // namespace vui
