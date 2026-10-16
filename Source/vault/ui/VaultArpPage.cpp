#include "VaultPages.h"

// CHROME VAULT v3 ARP · GATE page: the arpeggiator (order, octave range + mode, rate + straight / triplet / dotted, gate,
// swing, steps 1-32, latch, velocity source, retrigger, 32-step velocity / length / on-tie / slide / octave lanes, MIDI drag)
// and the gate (32 steps, rate, steps, depth, smooth, swing, mix). Both use one step clock (vault::StepGrid): the song's
// grid while the host plays, the first key's while it is stopped. Both are off unless the sound turns them on.
namespace vui
{
//==============================================================================
// one 32-step lane (cells grouped by four)
class StepLane : public juce::Component, public juce::SettableTooltipClient
{
public:
    enum class Kind { velocity, length, onTie, slide, octave, gate };
    StepLane (Kind k, const juce::String& l) : kind (k), label (l)
    {
        noFocus (*this);
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
        switch (kind)
        {
            case Kind::onTie:  setTooltip ("Click: step on / off. Right-click (or alt-click): tie it to the note before (one long note)"); break;
            case Kind::slide:  setTooltip ("Click: the note slides into the next one (legato - mono / legato layers glide)"); break;
            case Kind::octave: setTooltip ("Click: one octave up, right-click: one octave down"); break;
            case Kind::length: setTooltip ("Each step's note length (x GATE). Click or drag over the steps to draw"); break;
            default:           setTooltip ("Click or drag over the steps to draw"); break;
        }
    }
    std::function<float (int)> get;              // velocity / length / gate level 0..1, type (VaultArp::StepType), slide 0/1, octave -2..2
    std::function<void (int, float)> set;
    std::function<bool (int)> isRest;            // the step plays nothing (dimmed)
    int steps = 16, current = -1;
    static constexpr int labelW = 96, cellGap = 3, groupGap = 5, cells = vault::numSteps;

    juce::Rectangle<float> cell (int i) const
    {
        const float w = ((float) getWidth() - (float) labelW - (float) (cells - 1) * cellGap - (float) (cells / 4 - 1) * groupGap) / (float) cells;
        return { (float) labelW + (float) i * (w + cellGap) + (float) (i / 4) * groupGap, 0.0f, w, (float) getHeight() };
    }
    void setCurrent (int c) { if (c != current) { current = c; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        g.setFont (font (14.0f, false, 0.12f));
        g.setColour (col::dim);
        g.drawText (label, juce::Rectangle<float> (0.0f, 0.0f, (float) labelW - 6.0f, (float) getHeight()), juce::Justification::centredLeft, false);
        for (int i = 0; i < cells; ++i)
        {
            const auto c = cell (i);
            const float a = i < steps ? 1.0f : 0.28f;
            const float v = get ? get (i) : 0.0f;
            const bool rest = isRest && isRest (i);
            switch (kind)
            {
                case Kind::velocity: case Kind::length: case Kind::gate:
                {
                    g.setColour (col::stepOff.withAlpha (a));
                    g.fillRoundedRectangle (c, 3.0f);
                    const float h = std::max (kind == Kind::gate ? 0.0f : 3.0f, v * c.getHeight());
                    if (h > 0.5f)
                    {
                        auto bar = c.withTop (c.getBottom() - h);
                        if (rest) g.setColour (col::ice2.withAlpha (0.22f * a));
                        else if (kind == Kind::length) g.setColour (col::silver.withAlpha (0.55f * a));   // length: a quieter tone than velocity
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
                        if (t == VaultArp::tie && i > 0) bar = bar.withLeft (cell (i - 1).getRight() - 3.0f);   // joined to the step before
                        g.setGradientFill (juce::ColourGradient (col::ice.withAlpha (a), 0, bar.getY(), col::deep.withAlpha (a), 0, bar.getBottom(), false));
                        g.fillRoundedRectangle (bar, 3.0f);
                        if (t == VaultArp::tie && c.getWidth() >= 24.0f)
                        {
                            g.setColour (col::ink.withAlpha (0.8f));
                            g.setFont (font (14.0f, true));
                            g.drawText ("TIE", c, juce::Justification::centred, false);
                        }
                    }
                    break;
                }
                case Kind::slide:
                {
                    g.setColour (col::stepOff.withAlpha (a));
                    g.fillRoundedRectangle (c, 3.0f);
                    if (v > 0.5f)
                    {
                        // a rising ramp: this note glides into the next
                        juce::Path p;
                        const auto r = c.reduced (4.0f, 4.0f);
                        p.startNewSubPath (r.getX(), r.getBottom());
                        p.lineTo (r.getRight(), r.getY());
                        g.setColour ((rest ? col::dim : col::ice).withAlpha (a));
                        g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
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
        for (int i = 0; i < cells; ++i)
            if (cell (i).expanded ((float) groupGap * 0.5f, 0.0f).getHorizontalRange().contains (x)) return i;
        return -1;
    }
    void mouseDown (const juce::MouseEvent& e) override
    {
        const int i = cellAt (e.position.x);
        if (i < 0 || ! set || ! get) return;
        const bool alt = e.mods.isPopupMenu() || e.mods.isAltDown();
        switch (kind)
        {
            case Kind::velocity: case Kind::length: case Kind::gate: draw (e); break;
            case Kind::onTie:
            {
                const int t = juce::roundToInt (get (i));
                if (alt) set (i, (float) (t == VaultArp::tie ? VaultArp::play : VaultArp::tie));
                else set (i, (float) (t == VaultArp::rest ? VaultArp::play : VaultArp::rest));
                break;
            }
            case Kind::slide: set (i, get (i) > 0.5f ? 0.0f : 1.0f); break;
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
    void mouseDrag (const juce::MouseEvent& e) override { if (kind == Kind::velocity || kind == Kind::length || kind == Kind::gate) draw (e); }

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
        if (kind == Kind::length) v = std::max (0.05f, v);
        set (i, v);
        repaint();
    }
    Kind kind;
    juce::String label;
};

//==============================================================================
namespace
{
constexpr int arpH = 344, gap = 12, gateH = 222;
juce::String divName (int i) { return vault::divisions()[(size_t) juce::jlimit (0, (int) vault::divisions().size() - 1, i)].name; }
bool sameLanes (const VaultArp& a, const VaultArp& b)
{
    return a.vel == b.vel && a.type == b.type && a.oct == b.oct && a.len == b.len && a.slide == b.slide && a.steps == b.steps;
}
bool sameGate (const VaultGate& a, const VaultGate& b) { return a.level == b.level && a.steps == b.steps; }
const juce::StringArray& octModeNames()   { static const juce::StringArray n { "Up", "Down", "Up-Down", "Alternate" }; return n; }
const juce::StringArray& velModeNames()   { static const juce::StringArray n { juce::String (juce::CharPointer_UTF8 ("Played \xc3\x97 Steps")), "Steps", "Played" }; return n; }
const juce::StringArray& retrigNames()    { static const juce::StringArray n { "Off", "New chord", "Every note", "Every bar" }; return n; }
const juce::StringArray& latchNames()     { static const juce::StringArray n { "Off", "On", "Add" }; return n; }
int latchIndex (const VaultArp& a) { return ! a.latch ? 0 : a.latchMode == VaultArp::latchAdd ? 2 : 1; }

// the plainly named starting patterns (rhythm only: rate, steps, gate, lanes - the order and octaves stay)
const juce::StringArray& startPatterns()
{
    static const juce::StringArray n { "Accented 16ths", "Offbeat 8ths", "Gated 8ths", "Octave Bounce", "3-3-2 Pulse", "Slide Steps" };
    return n;
}
void applyStart (VaultArp& a, int which)
{
    const VaultArp init;
    a.vel = init.vel; a.type = init.type; a.oct = init.oct; a.len = init.len; a.slide = init.slide;
    a.rate = (uint8_t) vault::divisionIndex ("1/16"); a.steps = 16; a.gate = 0.8f; a.swing = 0.0f;
    switch (which)
    {
        case 0: for (int i = 0; i < vault::numSteps; ++i) a.vel[(size_t) i] = i % 4 == 0 ? 1.0f : i % 2 == 0 ? 0.85f : 0.68f; a.gate = 0.6f; break;
        case 1: a.rate = (uint8_t) vault::divisionIndex ("1/8"); a.steps = 8;
                for (int i = 0; i < vault::numSteps; ++i) a.type[(size_t) i] = i % 2 == 1 ? VaultArp::play : VaultArp::rest;
                a.gate = 0.5f; break;
        case 2: a.rate = (uint8_t) vault::divisionIndex ("1/8"); a.steps = 8; a.gate = 0.3f;
                for (int i = 0; i < vault::numSteps; ++i) a.vel[(size_t) i] = i % 2 == 0 ? 1.0f : 0.8f; break;
        case 3: for (int i = 0; i < vault::numSteps; ++i) a.oct[(size_t) i] = (int8_t) (i % 2); a.gate = 0.5f; break;
        case 4:
        {
            // 3 + 3 + 2 sixteenths: a note held over its group
            const int starts[] { 0, 3, 6, 8, 11, 14 };
            for (int i = 0; i < vault::numSteps; ++i) a.type[(size_t) i] = VaultArp::tie;
            for (int i = 0; i < vault::numSteps; ++i) for (int s : starts) if (i % 16 == s) a.type[(size_t) i] = VaultArp::play;
            for (int i = 0; i < vault::numSteps; ++i) a.vel[(size_t) i] = i % 16 == 0 ? 1.0f : 0.8f;
            a.gate = 0.85f; break;
        }
        case 5: for (int i = 0; i < vault::numSteps; ++i) a.slide[(size_t) i] = (uint8_t) (i % 4 == 2 ? 1 : 0); a.gate = 0.6f; break;
        default: break;
    }
}
} // namespace

const juce::StringArray& ArpGatePage::rateBases()
{
    static const juce::StringArray b { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };
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
    return { rateBases().indexOf (n), feel };   // -1: "2 bars" ...
}
const std::vector<int>& ArpGatePage::rateKnobDivisions()
{
    static const std::vector<int> list = []
    {
        std::vector<int> r;
        const auto& d = vault::divisions();
        for (int i = 0; i < (int) d.size(); ++i)   // the bar rates, slowest first
            if (juce::String (d[(size_t) i].name).endsWith ("bars")) r.push_back (i);
        std::sort (r.begin(), r.end(), [&d] (int a, int b) { return d[(size_t) a].beats > d[(size_t) b].beats; });
        for (const auto& base : rateBases())       // then per base: dotted, straight, triplet
            for (const char* feel : { "D", "", "T" })
                if (const int i = vault::divisionIndex ((base + feel).toRawUTF8()); i >= 0) r.push_back (i);
        return r;
    }();
    return list;
}
float ArpGatePage::rateKnobPos (int div)
{
    const auto& l = rateKnobDivisions();
    if (l.size() < 2) return 0.0f;
    auto it = std::find (l.begin(), l.end(), div);
    if (it == l.end())   // not on the knob (should not happen): the closest length
    {
        const double b = vault::divisionBeats (div);
        it = std::min_element (l.begin(), l.end(), [b] (int x, int y)
                               { return std::abs (std::log (vault::divisionBeats (x) / b)) < std::abs (std::log (vault::divisionBeats (y) / b)); });
    }
    return (float) (it - l.begin()) / (float) (l.size() - 1);
}
int ArpGatePage::rateFromKnob (float pos)
{
    const auto& l = rateKnobDivisions();
    return l[(size_t) juce::jlimit (0, (int) l.size() - 1, juce::roundToInt (juce::jlimit (0.0f, 1.0f, pos) * (float) (l.size() - 1)))];
}
juce::String ArpGatePage::rateKnobText (float pos) { return divName (rateFromKnob (pos)); }
const juce::StringArray& ArpGatePage::orderNames()
{
    // VaultArp::Mode order
    static const juce::StringArray n { "Up", "Down", "Up-Down", "As Played", "Random", "Chord", "Down-Up", "Up & Down",
                                       "Converge", "Diverge", "Thumb", "Pinky" };
    return n;
}
double ArpGatePage::exportBars (const VaultArp& a)
{
    const double patternBeats = vault::divisionBeats (a.rate) * juce::jlimit (1, (int) vault::numSteps, (int) a.steps);
    return juce::jlimit (2.0, 16.0, std::ceil (patternBeats / 4.0 - 1.0e-9));
}

void ArpGatePage::choiceMenu (Dropdown& d, const juce::StringArray& items, int current, std::function<void (int)> pick)
{
    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    for (int i = 0; i < items.size(); ++i) m.addItem (i + 1, items[i], true, i == current);
    juce::Component::SafePointer<ArpGatePage> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&d), [safe, pick] (int r) { if (safe != nullptr && r > 0) pick (r - 1); });
}

ArpGatePage::ArpGatePage (UiContext& c) : Page (c)
{
    velLane = std::make_unique<StepLane> (StepLane::Kind::velocity, "VELOCITY");
    lenLane = std::make_unique<StepLane> (StepLane::Kind::length, "LENGTH");
    typeLane = std::make_unique<StepLane> (StepLane::Kind::onTie, "ON / TIE");
    slideLane = std::make_unique<StepLane> (StepLane::Kind::slide, "SLIDE");
    octLane = std::make_unique<StepLane> (StepLane::Kind::octave, "OCTAVE");
    gateLane = std::make_unique<StepLane> (StepLane::Kind::gate, "STEPS");
    for (auto* l : { velLane.get(), lenLane.get(), typeLane.get(), slideLane.get(), octLane.get(), gateLane.get() }) addAndMakeVisible (*l);
    auto rest = [this] (int i) { return ctx.sound().arp.type[(size_t) i] == VaultArp::rest; };
    velLane->get = [this] (int i) { return ctx.sound().arp.vel[(size_t) i]; };
    velLane->isRest = rest;
    velLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.arp.vel[(size_t) i] = v; }); };
    lenLane->get = [this] (int i) { return ctx.sound().arp.len[(size_t) i]; };
    lenLane->isRest = rest;
    lenLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.arp.len[(size_t) i] = juce::jlimit (0.05f, 1.0f, v); }); };
    typeLane->get = [this] (int i) { return (float) ctx.sound().arp.type[(size_t) i]; };
    typeLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.arp.type[(size_t) i] = (uint8_t) juce::roundToInt (v); }); };
    slideLane->get = [this] (int i) { return (float) ctx.sound().arp.slide[(size_t) i]; };
    slideLane->isRest = rest;
    slideLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.arp.slide[(size_t) i] = (uint8_t) (v > 0.5f ? 1 : 0); }); };
    octLane->get = [this] (int i) { return (float) ctx.sound().arp.oct[(size_t) i]; };
    octLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.arp.oct[(size_t) i] = (int8_t) juce::roundToInt (v); }); };
    gateLane->get = [this] (int i) { return ctx.sound().gate.level[(size_t) i]; };
    gateLane->set = [this] (int i, float v) { ctx.edit ([i, v] (VaultPreset& p) { p.gate.level[(size_t) i] = v; }); };

    // ---- arpeggiator
    addAndMakeVisible (arpPower);
    arpPower.onClick = [this] { ctx.proc.setSwitch (VaultProcessor::idArpOn, ! ctx.proc.getSwitch (VaultProcessor::idArpOn)); if (ctx.soundEdited) ctx.soundEdited(); };
    addAndMakeVisible (arpPattern);
    arpPattern.onClick = [this] { arpPatternMenu(); };
    addAndMakeVisible (latchDd);
    latchDd.setTooltip ("Latch: the pattern keeps playing after the keys are released. On: a new chord replaces it. Add: new keys join it. "
                        "The sustain pedal holds the keys too.");
    latchDd.onClick = [this]
    {
        choiceMenu (latchDd, { "Off", "On (a new chord replaces the held one)", "Add (new keys join the held ones)" }, latchIndex (ctx.sound().arp), [this] (int i)
        {
            ctx.edit ([i] (VaultPreset& p) { p.arp.latch = i > 0; p.arp.latchMode = (uint8_t) (i == 2 ? VaultArp::latchAdd : VaultArp::latchReplace); });
        });
    };
    addAndMakeVisible (arpKeep);
    arpKeep.onClick = [this]
    {
        const bool l = ! ctx.proc.getLock (VaultProcessor::Section::arp);
        ctx.proc.setLock (VaultProcessor::Section::arp, l);
        ctx.say (l ? "The arpeggiator stays when you pick another sound" : "Every sound brings its own arpeggiator settings again");
        if (ctx.soundEdited) ctx.soundEdited();
    };
    addAndMakeVisible (orderDd);
    orderDd.setTooltip ("The order the held notes play in");
    orderDd.onClick = [this]
    {
        choiceMenu (orderDd, { "Up", "Down", "Up-Down", "As Played", "Random", "Chord (all notes on each step)", "Down-Up",
                               "Up & Down (top and bottom twice)", "Converge (outside in)", "Diverge (inside out)",
                               "Thumb (lowest note between the others)", "Pinky (highest note between the others)" },
                    ctx.sound().arp.mode, [this] (int m) { ctx.edit ([m] (VaultPreset& p) { p.arp.mode = (uint8_t) m; }); });
    };
    addAndMakeVisible (octModeDd);
    octModeDd.setTooltip ("How OCTAVES spreads the notes: Up / Down = over the whole range, Up-Down = one octave per pass up and back, "
                          "Alternate = each note followed by its octaves");
    octModeDd.onClick = [this]
    {
        choiceMenu (octModeDd, { "Up (the order runs over the octaves above)", "Down (the octaves below)", "Up-Down (one octave per pass, up and back)",
                                 "Alternate (each note, then its octaves)" },
                    ctx.sound().arp.octMode, [this] (int m) { ctx.edit ([m] (VaultPreset& p) { p.arp.octMode = (uint8_t) m; }); });
    };
    addAndMakeVisible (velModeDd);
    velModeDd.setTooltip ("Where a note's velocity comes from: how hard you played x the VELOCITY steps, the steps alone, or your playing alone");
    velModeDd.onClick = [this]
    {
        choiceMenu (velModeDd, { juce::String (juce::CharPointer_UTF8 ("Played \xc3\x97 Steps")), "Steps (the VELOCITY lane only)", "Played (how hard you play only)" },
                    ctx.sound().arp.velMode, [this] (int m) { ctx.edit ([m] (VaultPreset& p) { p.arp.velMode = (uint8_t) m; }); });
    };
    addAndMakeVisible (retrigDd);
    retrigDd.setTooltip ("When the order starts again from its first note: Off = it runs on through chord changes, New chord = keys "
                         "pressed after all were up, Every note, Every bar");
    retrigDd.onClick = [this]
    {
        choiceMenu (retrigDd, { "Off (runs on through chord changes)", "New chord", "Every note", "Every bar" },
                    ctx.sound().arp.retrig, [this] (int m) { ctx.edit ([m] (VaultPreset& p) { p.arp.retrig = (uint8_t) m; }); });
    };
    addAndMakeVisible (feelSeg);
    feelSeg.onSelect = [this] (int f)
    {
        const auto bf = baseAndFeel (ctx.sound().arp.rate);
        if (bf.first < 0 || (f != 0 && vault::divisionIndex ((rateBases()[bf.first] + (f == 1 ? "T" : "D")).toRawUTF8()) < 0))
        {
            ctx.say ("Triplet and dotted rates go from 1/2 to 1/64");
            refresh();
            return;
        }
        const int r = rateIndex (bf.first, f);
        ctx.edit ([r] (VaultPreset& p) { p.arp.rate = (uint8_t) r; });
    };
    // RATE: the same list and angle as the gate's RATE; the label says the whole rate ("1/16T", "1/8D")
    arpRate.steps = (int) rateKnobDivisions().size();
    arpRate.sensitivity = 0.6f;
    arpRate.valueText = [] (float n) { return rateKnobText (n); };
    arpRate.onChange = [this] (float n)
    {
        const int r = rateFromKnob (n);
        ctx.edit ([r] (VaultPreset& p) { p.arp.rate = (uint8_t) r; });
    };
    arpOct.steps = 4;
    arpOct.valueText = [] (float n) { return juce::String (1 + juce::roundToInt (n * 3.0f)); };
    arpOct.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.arp.octaves = (uint8_t) (1 + juce::roundToInt (n * 3.0f)); }); };
    arpGate.valueText = [] (float n) { return pct (0.05f + 0.95f * n); };
    arpGate.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.arp.gate = 0.05f + 0.95f * n; }); };
    arpGate.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, arpGate, "arp.gate"); };
    arpGate.setTooltip ("Note length (share of a step) - the LENGTH lane scales it per step, ties make notes longer");
    arpSwing.valueText = [] (float n) { return pct (0.5f + 0.25f * n); };   // 50 % = straight .. 75 %
    arpSwing.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.arp.swing = n; }); };
    arpSwing.setTooltip ("Swing: every second step later - 50 % is straight, 66 % a triplet feel, 75 % a dotted feel");
    arpSteps.steps = vault::numSteps;
    arpSteps.sensitivity = 0.7f;
    arpSteps.valueText = [] (float n) { return juce::String (1 + juce::roundToInt (n * (float) (vault::numSteps - 1))); };
    arpSteps.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.arp.steps = (uint8_t) (1 + juce::roundToInt (n * (float) (vault::numSteps - 1))); }); };
    for (auto* k : { &arpRate, &arpOct, &arpGate, &arpSwing, &arpSteps }) addAndMakeVisible (*k);
    addAndMakeVisible (midiBtn);
    midiBtn.onDragOut = [this]
    {
        const double bars = exportBars (ctx.sound().arp);
        const auto f = ctx.proc.writeArpMidiFile (bars);
        if (f.existsAsFile())
        {
            ctx.say ("Drop the MIDI into your DAW: " + juce::String ((int) bars) + " bars of what the arp plays");
            juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, &midiBtn);
        }
    };
    midiBtn.onClickOnly = [this] { ctx.say ("Drag this button into your DAW: what the arp plays as MIDI (the held keys, else the C5 triad)"); };

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
    gRate.steps = (int) rateKnobDivisions().size();
    gRate.sensitivity = 0.6f;
    gRate.valueText = [] (float n) { return rateKnobText (n); };
    gRate.onChange = [this] (float n) { const int r = rateFromKnob (n); ctx.edit ([r] (VaultPreset& p) { p.gate.rate = (uint8_t) r; }); };
    gSteps.steps = vault::numSteps;
    gSteps.sensitivity = 0.7f;
    gSteps.valueText = [] (float n) { return juce::String (1 + juce::roundToInt (n * (float) (vault::numSteps - 1))); };
    gSteps.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.steps = (uint8_t) (1 + juce::roundToInt (n * (float) (vault::numSteps - 1))); }); };
    gDepth.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.depth = n; }); };
    gSmooth.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.smooth = n; }); };
    gSwing.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.swing = n; }); };
    gMix.onChange = [this] (float n) { ctx.edit ([n] (VaultPreset& p) { p.gate.mix = n; }); };
    for (auto* k : { &gDepth, &gSmooth, &gMix }) k->valueText = [] (float n) { return pct (n); };
    gSwing.valueText = [] (float n) { return pct (0.5f + 0.25f * n); };
    gDepth.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, gDepth, "gate.depth"); };
    gSmooth.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, gSmooth, "gate.smooth"); };
    gMix.onRightClick = [this] (const juce::MouseEvent&) { mod::showMenu (ctx, gMix, "gate.mix"); };
    for (auto* k : { &gRate, &gSteps, &gDepth, &gSmooth, &gSwing, &gMix }) addAndMakeVisible (*k);
    refresh();
}

ArpGatePage::~ArpGatePage() = default;

void ArpGatePage::resized()
{
    arpR = { 0, 0, getWidth(), arpH };
    gateR = { 0, arpH + gap, getWidth(), gateH };
    // arp header: power, title, Pattern, Latch, Keep ... Drag MIDI
    {
        const int y = arpR.getY() + 12;
        arpPower.setBounds (arpR.getX() + 16, y + 4, 22, 22);
        int x = arpR.getX() + 46 + (int) std::ceil (textWidth (font (18.0f, true, 0.02f), "ARPEGGIATOR")) + 16;
        arpPattern.setBounds (x, y + 1, arpPattern.idealWidth(), 28); x = arpPattern.getRight() + 8;
        latchDd.setBounds (x, y + 1, latchDd.idealWidth(), 28); x = latchDd.getRight() + 8;
        arpKeep.setBounds (x, y + 1, arpKeep.idealWidth(), 28);
        midiBtn.setBounds (arpR.getRight() - 16 - midiBtn.idealWidth(), y + 1, midiBtn.idealWidth(), 28);
    }
    // knobs + the four modes (two columns of dropdowns on the right)
    {
        const int y = arpR.getY() + 52;
        int x = arpR.getX() + 10;
        arpRate.setBounds (x, y, 84, arpRate.idealHeight()); x += 88;
        feelSeg.setBounds (x, y + 10, feelSeg.idealWidth(), 30); x = feelSeg.getRight() + 12;
        for (auto* k : { &arpOct, &arpGate, &arpSwing, &arpSteps }) { k->setBounds (x, y, 84, k->idealHeight()); x += 88; }
        const int w1 = std::max (orderDd.idealWidth(), velModeDd.idealWidth()), w2 = std::max (octModeDd.idealWidth(), retrigDd.idealWidth());
        const int x2 = arpR.getRight() - 16 - w2, x1 = x2 - 8 - w1;
        orderDd.setBounds (x1, y + 4, w1, 28);   octModeDd.setBounds (x2, y + 4, w2, 28);
        velModeDd.setBounds (x1, y + 40, w1, 28); retrigDd.setBounds (x2, y + 40, w2, 28);
    }
    const int lx = arpR.getX() + 16, lw = arpR.getWidth() - 32;
    int ly = arpR.getY() + 142;
    velLane->setBounds (lx, ly, lw, 54);   ly += 60;
    lenLane->setBounds (lx, ly, lw, 34);   ly += 40;
    typeLane->setBounds (lx, ly, lw, 26);  ly += 32;
    slideLane->setBounds (lx, ly, lw, 22); ly += 28;
    octLane->setBounds (lx, ly, lw, 24);
    // gate
    {
        const int y = gateR.getY() + 12;
        gatePower.setBounds (gateR.getX() + 16, y + 4, 22, 22);
        int x = gateR.getX() + 46 + (int) std::ceil (textWidth (font (18.0f, true, 0.02f), "GATE")) + 16;
        gatePattern.setBounds (x, y + 1, gatePattern.idealWidth(), 28); x = gatePattern.getRight() + 8;
        gateKeep.setBounds (x, y + 1, gateKeep.idealWidth(), 28);
        int kx = gateR.getX() + 10;
        for (auto* k : { &gRate, &gSteps, &gDepth, &gSmooth, &gSwing, &gMix }) { k->setBounds (kx, gateR.getY() + 52, 84, k->idealHeight()); kx += 88; }
    }
    gateLane->setBounds (gateR.getX() + 16, gateR.getY() + 146, gateR.getWidth() - 32, 58);
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
    const float ax = (float) arpKeep.getRight() + 16.0f, aw = (float) midiBtn.getX() - 16.0f - ax;
    if (aw > 120.0f)
        g.drawText ("synced to the DAW grid while it plays", juce::Rectangle<float> (ax, (float) arpR.getY() + 12.0f, aw, 30.0f), juce::Justification::centredRight, true);
    const float nx = (float) gateKeep.getRight() + 16.0f;
    g.drawText (juce::String (juce::CharPointer_UTF8 ("rhythmic volume chop \xc2\xb7 same grid as the arpeggiator")),
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
    arpKeep.setToggleState (ctx.proc.getLock (VaultProcessor::Section::arp), juce::dontSendNotification);
    gateKeep.setToggleState (ctx.proc.getLock (VaultProcessor::Section::gate), juce::dontSendNotification);
    const auto bf = baseAndFeel (a.rate);
    feelSeg.setSelected (bf.second);
    feelSeg.setEnabledLook (bf.first > 0);                   // triplet / dotted exist from 1/2 on
    const float ns = (float) (vault::numSteps - 1);
    arpRate.setValue (rateKnobPos (a.rate));                 arpRate.resetValue = rateKnobPos (bank.arp.rate);
    arpOct.setValue ((float) (a.octaves - 1) / 3.0f);        arpOct.resetValue = (float) (bank.arp.octaves - 1) / 3.0f;
    arpGate.setValue ((a.gate - 0.05f) / 0.95f);             arpGate.resetValue = (bank.arp.gate - 0.05f) / 0.95f;
    arpSwing.setValue (a.swing);                             arpSwing.resetValue = bank.arp.swing;
    arpSteps.setValue ((float) (a.steps - 1) / ns);          arpSteps.resetValue = (float) (bank.arp.steps - 1) / ns;
    mod::show (arpGate, snd, "arp.gate");
    gRate.setValue (rateKnobPos (gt.rate));                  gRate.resetValue = rateKnobPos (bank.gate.rate);
    gSteps.setValue ((float) (gt.steps - 1) / ns);           gSteps.resetValue = (float) (bank.gate.steps - 1) / ns;
    gDepth.setValue (gt.depth);                              gDepth.resetValue = bank.gate.depth;
    gSmooth.setValue (gt.smooth);                            gSmooth.resetValue = bank.gate.smooth;
    gSwing.setValue (gt.swing);                              gSwing.resetValue = bank.gate.swing;
    gMix.setValue (gt.mix);                                  gMix.resetValue = bank.gate.mix;
    mod::show (gDepth, snd, "gate.depth"); mod::show (gSmooth, snd, "gate.smooth"); mod::show (gMix, snd, "gate.mix");
    for (auto* k : { &arpRate, &arpOct, &arpGate, &arpSwing, &arpSteps, &gRate, &gSteps, &gDepth, &gSmooth, &gSwing, &gMix }) k->repaint();
    for (auto* l : { velLane.get(), lenLane.get(), typeLane.get(), slideLane.get(), octLane.get() }) { l->steps = a.steps; l->repaint(); }
    gateLane->steps = gt.steps;
    gateLane->repaint();

    bool relayout = false;
    auto setText = [&relayout] (juce::Button& b, const juce::String& t) { if (b.getButtonText() != t) { b.setButtonText (t); relayout = true; } };
    const VaultArp init;
    setText (arpPattern, "Pattern: " + (sameLanes (a, init) ? juce::String ("Init") : sameLanes (a, bank.arp) ? juce::String ("Preset") : juce::String ("Custom")));
    setText (latchDd, "Latch: " + latchNames()[latchIndex (a)]);
    setText (orderDd, "Order: " + orderNames()[juce::jlimit (0, orderNames().size() - 1, (int) a.mode)]);
    setText (octModeDd, "Octaves: " + octModeNames()[juce::jlimit (0, octModeNames().size() - 1, (int) a.octMode)]);
    setText (velModeDd, "Velocity: " + velModeNames()[juce::jlimit (0, velModeNames().size() - 1, (int) a.velMode)]);
    setText (retrigDd, "Retrigger: " + retrigNames()[juce::jlimit (0, retrigNames().size() - 1, (int) a.retrig)]);
    octModeDd.setEnabled (a.octaves > 1);
    const VaultGate ginit;
    setText (gatePattern, "Pattern: " + (sameGate (gt, ginit) ? juce::String ("Init") : sameGate (gt, bank.gate) ? juce::String ("Preset") : juce::String ("Custom")));
    if (relayout) resized();
    repaint (arpR.withHeight (44));
    repaint (gateR.withHeight (44));
}

void ArpGatePage::tick()
{
    const bool arpOn = ctx.proc.getSwitch (VaultProcessor::idArpOn);
    const int st = arpOn ? ctx.proc.arpStep() : -1;
    for (auto* l : { velLane.get(), lenLane.get(), typeLane.get(), slideLane.get(), octLane.get() }) l->setCurrent (st);
}

void ArpGatePage::arpPatternMenu()
{
    juce::PopupMenu m;
    m.setLookAndFeel (&getLookAndFeel());
    m.addItem (1, "Init");
    m.addItem (2, "The sound's own pattern", ! sameLanes (ctx.bankSound().arp, VaultArp()));
    juce::PopupMenu starts;
    for (int i = 0; i < startPatterns().size(); ++i) starts.addItem (100 + i, startPatterns()[i]);
    m.addSubMenu ("Start from", starts);
    m.addSeparator();
    m.addItem (10, "All steps on");
    m.addItem (11, "Accent on the beats");
    m.addItem (12, "Velocity ramp up");
    m.addItem (13, "Velocity ramp down");
    m.addItem (14, "Random velocities");
    m.addItem (15, "Random octaves");
    m.addItem (16, "Lengths back to full");
    m.addItem (17, "No slides");
    m.addSeparator();
    m.addItem (20, "Shift left");
    m.addItem (21, "Shift right");
    m.addItem (22, "Reverse");
    m.addItem (23, "Double the length (copy the steps)", ctx.sound().arp.steps <= vault::numSteps / 2);
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
            constexpr int ns = vault::numSteps;
            if (r >= 100) { applyStart (a, r - 100); return; }
            switch (r)
            {
                case 1: { const bool keepOn = a.on; a = init; a.on = keepOn; break; }
                case 2: a.vel = own.vel; a.type = own.type; a.oct = own.oct; a.len = own.len; a.slide = own.slide; a.steps = own.steps; break;
                case 10: for (auto& t : a.type) t = VaultArp::play; break;
                case 11: for (int i = 0; i < ns; ++i) a.vel[(size_t) i] = i % 4 == 0 ? 1.0f : 0.68f; break;
                case 12: for (int i = 0; i < ns; ++i) a.vel[(size_t) i] = 0.35f + 0.65f * (float) (i % n) / (float) std::max (1, n - 1); break;
                case 13: for (int i = 0; i < ns; ++i) a.vel[(size_t) i] = 1.0f - 0.65f * (float) (i % n) / (float) std::max (1, n - 1); break;
                case 14: for (auto& v : a.vel) v = 0.45f + 0.55f * rng.nextFloat(); break;
                case 15: for (auto& o : a.oct) o = (int8_t) (rng.nextFloat() < 0.7f ? 0 : rng.nextBool() ? 1 : -1); break;
                case 16: a.len.fill (1.0f); break;
                case 17: a.slide.fill (0); break;
                case 20: case 21: case 22:
                {
                    auto rot = [&] (auto& arr)
                    {
                        if (r == 22) std::reverse (arr.begin(), arr.begin() + n);
                        else if (r == 20) std::rotate (arr.begin(), arr.begin() + 1, arr.begin() + n);
                        else std::rotate (arr.begin(), arr.begin() + n - 1, arr.begin() + n);
                    };
                    rot (a.vel); rot (a.type); rot (a.oct); rot (a.len); rot (a.slide);
                    if (a.type[0] == VaultArp::tie) a.type[0] = VaultArp::play;
                    break;
                }
                case 23:
                    if (n <= ns / 2)
                    {
                        for (int i = 0; i < n; ++i)
                        {
                            a.vel[(size_t) (n + i)] = a.vel[(size_t) i]; a.type[(size_t) (n + i)] = a.type[(size_t) i]; a.oct[(size_t) (n + i)] = a.oct[(size_t) i];
                            a.len[(size_t) (n + i)] = a.len[(size_t) i]; a.slide[(size_t) (n + i)] = a.slide[(size_t) i];
                        }
                        a.steps = (uint8_t) (2 * n);
                    }
                    break;
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
    m.addItem (14, "Quarter pulses");
    m.addItem (12, "Invert");
    m.addItem (13, "Random");
    m.addSeparator();
    m.addItem (20, "Shift left");
    m.addItem (21, "Shift right");
    m.addItem (23, "Double the length (copy the steps)", ctx.sound().gate.steps <= vault::numSteps / 2);
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
            constexpr int ns = vault::numSteps;
            switch (r)
            {
                case 1: { const bool keepOn = g.on; g = VaultGate(); g.on = keepOn; break; }
                case 2: g.level = own.level; g.steps = own.steps; break;
                case 10: for (auto& v : g.level) v = 1.0f; break;
                case 11: for (int i = 0; i < ns; ++i) g.level[(size_t) i] = i % 2 == 1 ? 1.0f : 0.0f; break;
                case 14: for (int i = 0; i < ns; ++i) g.level[(size_t) i] = i % 4 == 0 ? 1.0f : 0.0f; break;
                case 12: for (auto& v : g.level) v = 1.0f - v; break;
                case 13: for (auto& v : g.level) v = rng.nextFloat() < 0.65f ? 1.0f : 0.0f; break;
                case 20: std::rotate (g.level.begin(), g.level.begin() + 1, g.level.begin() + n); break;
                case 21: std::rotate (g.level.begin(), g.level.begin() + n - 1, g.level.begin() + n); break;
                case 23: if (n <= ns / 2) { for (int i = 0; i < n; ++i) g.level[(size_t) (n + i)] = g.level[(size_t) i]; g.steps = (uint8_t) (2 * n); } break;
                default: break;
            }
        });
    });
}
} // namespace vui
