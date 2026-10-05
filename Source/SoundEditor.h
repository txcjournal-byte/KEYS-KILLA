#pragma once
// v0.29 SOUND EDIT: the whole sound on one page - OSC, LFO, PITCH, FILTER, AMP and FX - with graphs you can drag.
// Edits the KEYS KILLA sound that is loaded (factory preset, your saved preset or a BREED child).
// Included from PluginEditor.cpp after AdvancedPage.h (needs KKLookAndFeel, HotButton, the processor).

namespace kkedit
{
// each section wears its own two neon colours
struct Hue { Colour a, b; };
inline Hue hueOsc()    { return { TC (0xffff2f6d), TC (0xffff8a3d) }; }
inline Hue hueLfo()    { return { TC (0xff9b4dff), TC (0xffff3fd2) }; }
inline Hue huePitch()  { return { TC (0xff22d3ee), TC (0xff4d7dff) }; }
inline Hue hueFilter() { return { TC (0xff2ee6a6), TC (0xff22d3ee) }; }
inline Hue hueAmp()    { return { TC (0xffffb020), TC (0xffff6a3d) }; }
inline Hue hueFx()     { return { TC (0xffff3fd2), TC (0xff9b4dff) }; }
// every control scales with the page (1512 design units wide)
inline float pageScale (const Component& c) { return jmax (0.45f, (float) c.getParentWidth() / 1652.0f); }

inline void panel (Graphics& g, Rectangle<float> r, Hue h, float k)
{
    for (int i = 3; i >= 1; --i) { g.setColour (h.a.withAlpha (0.045f * (float) i)); g.drawRoundedRectangle (r.expanded ((float) (4 - i) * 2.0f * k), 12 * k, 2.0f * k); }
    g.setGradientFill (ColourGradient (TC (0xff1b1940), 0, r.getY(), TC (0xff100e26), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 10 * k);
    g.setGradientFill (ColourGradient (h.a, r.getX(), r.getY(), h.b.withAlpha (0.7f), r.getRight(), r.getBottom(), false));
    g.drawRoundedRectangle (r.reduced (0.6f), 10 * k, 1.6f * k);
}
// title chip of a panel: [ OSC ]  LAYER A
inline void title (Graphics& g, Rectangle<float> r, const String& name, const String& sub, Hue h, float k)
{
    Font f (FontOptions (13.0f * k, Font::bold)); f.setExtraKerningFactor (0.12f);
    GlyphArrangement ga; ga.addLineOfText (f, name, 0, 0);
    const float w = ga.getBoundingBox (0, -1, true).getWidth() + 22 * k;
    auto chip = Rectangle<float> (r.getX() + 12 * k, r.getY() + 10 * k, w, 22 * k);
    g.setGradientFill (ColourGradient (h.a, chip.getX(), 0, h.b, chip.getRight(), 0, false)); g.fillRoundedRectangle (chip, 6 * k);
    g.setColour (TC (0xffffffff)); g.setFont (f); g.drawText (name, chip, Justification::centred);
    if (sub.isNotEmpty())
    {
        g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (12.0f * k)).withExtraKerningFactor (0.1f));
        g.drawText (sub, Rectangle<float> (chip.getRight() + 10 * k, chip.getY(), 300 * k, chip.getHeight()), Justification::centredLeft);
    }
}
inline void screen (Graphics& g, Rectangle<float> r, Hue h, float k, int vx = 8, int vy = 4)
{
    g.setColour (TC (0xff0b0a1c)); g.fillRoundedRectangle (r, 6 * k);
    g.setColour (h.a.withAlpha (0.08f));
    for (int i = 1; i < vx; ++i) g.drawVerticalLine ((int) (r.getX() + r.getWidth() * (float) i / (float) vx), r.getY() + 2, r.getBottom() - 2);
    for (int i = 1; i < vy; ++i) g.drawHorizontalLine ((int) (r.getY() + r.getHeight() * (float) i / (float) vy), r.getX() + 2, r.getRight() - 2);
    g.setColour (h.a.withAlpha (0.25f)); g.drawRoundedRectangle (r, 6 * k, 1.0f);
}
// glowing curve, optionally filled down to 'floorY'
inline void curve (Graphics& g, const Path& p, Hue h, Rectangle<float> r, float k, float floorY = -1)
{
    if (floorY > 0)
    {
        Path f (p); f.lineTo (p.getCurrentPosition().x, floorY); f.lineTo (p.getBounds().getX(), floorY); f.closeSubPath();
        g.setGradientFill (ColourGradient (h.a.withAlpha (0.35f), 0, r.getY(), h.b.withAlpha (0.05f), 0, floorY, false)); g.fillPath (f);
    }
    g.setColour (h.a.withAlpha (0.25f)); g.strokePath (p, PathStrokeType (6.0f * k, PathStrokeType::curved, PathStrokeType::rounded));
    g.setGradientFill (ColourGradient (h.a, r.getX(), 0, h.b, r.getRight(), 0, false));
    g.strokePath (p, PathStrokeType (2.2f * k, PathStrokeType::curved, PathStrokeType::rounded));
}
inline void handle (Graphics& g, Point<float> c, Hue h, float k, bool hot)
{
    const float r = (hot ? 7.0f : 5.5f) * k;
    g.setColour (h.a.withAlpha (0.35f)); g.fillEllipse (c.x - r * 1.8f, c.y - r * 1.8f, r * 3.6f, r * 3.6f);
    g.setColour (TC (0xffffffff)); g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
    g.setColour (h.a); g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 2.0f * k);
}

//==============================================================================
// knob: neon arc in the section's colours, name below, value while you touch it
class Knob : public Slider
{
public:
    Knob (KeysKillaProcessor& p, const String& id, const String& name, Hue h) : Slider (RotaryHorizontalVerticalDrag, NoTextBox), label (name), hue (h)
    {
        setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
        setVelocityModeParameters (0.6, 1, 0.02, true, ModifierKeys::ctrlModifier);
        if (auto* rp = p.apvts.getParameter (id))
        {
            param = rp;
            setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
            setTooltip (paramTooltip (id).isNotEmpty() ? paramTooltip (id) + "  (double-click = default)" : String());
        }
        att = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, *this);
    }
    void paint (Graphics& g) override
    {
        // v0.44: no knob - a living cell
        const float k = pageScale (*this);
        kk::cell::draw (g, getLocalBounds().toFloat(), (float) valueToProportionOfLength (getValue()), getMinimum() < 0 && getMaximum() > 0, hue.a, hue.b, label, isMouseOverOrDragging(), false, 18 * k, (float) label.hashCode() * 0.001f);
    }
    void mouseEnter (const MouseEvent& e) override { Slider::mouseEnter (e); repaint(); }
    void mouseExit (const MouseEvent& e) override { Slider::mouseExit (e); repaint(); }
private:
    String label; Hue hue;
    RangedAudioParameter* param = nullptr;
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> att;
};

//==============================================================================
// a row of chips for a choice (or on/off) parameter
class Chips : public Component, public SettableTooltipClient
{
public:
    Chips (KeysKillaProcessor& p, const String& id, Hue h, StringArray names = {}) : proc (p), hue (h)
    {
        param = p.apvts.getParameter (id);
        if (auto* c = dynamic_cast<AudioParameterChoice*> (param)) items = names.isEmpty() ? c->choices : names;
        else if (param != nullptr && dynamic_cast<AudioParameterBool*> (param)) { items = names.isEmpty() ? StringArray { param->getName (20) } : names; isBool = true; }
        for (auto& s : items) s = s.toUpperCase();
        setTooltip (paramTooltip (id));
        ignoreUnused (proc);
    }
    int current() const
    {
        if (param == nullptr) return -1;
        if (isBool) return param->getValue() > 0.5f ? 0 : -1;
        return roundToInt (param->convertFrom0to1 (param->getValue()));
    }
    void paint (Graphics& g) override
    {
        const float k = pageScale (*this);
        const int cur = current();
        for (int i = 0; i < items.size(); ++i)
        {
            auto r = cell (i).reduced (2.0f * k, 0);
            const bool on = i == cur;
            if (on)
            {
                g.setColour (hue.a.withAlpha (0.3f)); g.fillRoundedRectangle (r.expanded (2 * k), 7 * k);
                g.setGradientFill (ColourGradient (hue.a, r.getX(), 0, hue.b, r.getRight(), 0, false)); g.fillRoundedRectangle (r, 5 * k);
            }
            else
            {
                g.setColour (i == hot ? TC (0xff2a2654) : TC (0xff17153a)); g.fillRoundedRectangle (r, 5 * k);
                g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (r, 5 * k, 1.0f);
            }
            g.setColour (on ? TC (0xffffffff) : TC (0xffc8c4e8));
            g.setFont (Font (FontOptions (jmin (12.0f * k, r.getHeight() * 0.55f), Font::bold)).withExtraKerningFactor (0.06f));
            g.drawFittedText (items[i], r.reduced (3 * k, 0).toNearestInt(), Justification::centred, 1, 0.6f);
        }
    }
    void mouseMove (const MouseEvent& e) override { const int h = at (e.position); if (h != hot) { hot = h; repaint(); } }
    void mouseExit (const MouseEvent&) override { hot = -1; repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        const int i = at (e.position);
        if (i < 0 || param == nullptr) return;
        param->beginChangeGesture();
        if (isBool) param->setValueNotifyingHost (current() == 0 ? 0.0f : 1.0f);
        else param->setValueNotifyingHost (param->convertTo0to1 ((float) i));
        param->endChangeGesture();
        repaint();
        if (onPick) onPick (i);
    }
    std::function<void (int)> onPick;
    int columns = 0;   // 0 = all in one row
private:
    Rectangle<float> cell (int i) const
    {
        const int cols = columns > 0 ? columns : items.size();
        const int rows = (items.size() + cols - 1) / jmax (1, cols);
        const float w = (float) getWidth() / (float) cols, h = (float) getHeight() / (float) jmax (1, rows);
        return { (float) (i % cols) * w, (float) (i / cols) * h + 1, w, h - 2 };
    }
    int at (Point<float> p) const { for (int i = 0; i < items.size(); ++i) if (cell (i).contains (p)) return i; return -1; }
    KeysKillaProcessor& proc; Hue hue;
    RangedAudioParameter* param = nullptr;
    StringArray items; bool isBool = false; int hot = -1;
};

// two or more page tabs inside a panel (LFO 1 | LFO 2, FILTER | ENVELOPE ...)
class Tabs : public Component
{
public:
    Tabs (StringArray n, Hue h) : names (std::move (n)), hue (h) {}
    int sel = 0;
    std::function<void (int)> onChange;
    void paint (Graphics& g) override
    {
        const float k = pageScale (*this);
        const float w = (float) getWidth() / (float) names.size();
        for (int i = 0; i < names.size(); ++i)
        {
            auto r = Rectangle<float> ((float) i * w, 0, w, (float) getHeight()).reduced (2 * k, 0);
            if (i == sel) { g.setGradientFill (ColourGradient (hue.a, r.getX(), 0, hue.b, r.getRight(), 0, false)); g.fillRoundedRectangle (r, 6 * k); }
            else { g.setColour (TC (0xff17153a)); g.fillRoundedRectangle (r, 6 * k); g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (r, 6 * k, 1.0f); }
            g.setColour (i == sel ? TC (0xffffffff) : TC (0xffc8c4e8));
            g.setFont (Font (FontOptions (12.0f * k, Font::bold)).withExtraKerningFactor (0.1f));
            g.drawText (names[i], r, Justification::centred);
        }
    }
    void mouseDown (const MouseEvent& e) override
    {
        const int i = jlimit (0, names.size() - 1, (int) (e.position.x / ((float) getWidth() / (float) names.size())));
        if (i != sel) { sel = i; repaint(); if (onChange) onChange (i); }
    }
private:
    StringArray names; Hue hue;
};

//==============================================================================
// graphs: base class reads parameters and drags them
class Graph : public Component, public SettableTooltipClient
{
public:
    Graph (KeysKillaProcessor& p, Hue h) : proc (p), hue (h) {}
    float v (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void set (const char* id, float value)
    {
        if (auto* rp = proc.apvts.getParameter (id)) rp->setValueNotifyingHost (rp->convertTo0to1 (rp->getNormalisableRange().snapToLegalValue (value)));
    }
    void begin (std::initializer_list<const char*> ids) { for (auto* id : ids) if (auto* rp = proc.apvts.getParameter (id)) { rp->beginChangeGesture(); touched.push_back (rp); } }
    void mouseUp (const MouseEvent&) override { for (auto* rp : touched) rp->endChangeGesture(); touched.clear(); drag = -1; repaint(); }
    void mouseExit (const MouseEvent&) override { if (drag < 0) { hot = -1; repaint(); } }
    float k() const { return pageScale (*this); }
    Rectangle<float> area() const { return getLocalBounds().toFloat().reduced (10 * k(), 10 * k()); }
protected:
    KeysKillaProcessor& proc; Hue hue;
    std::vector<RangedAudioParameter*> touched;
    int drag = -1, hot = -1;
};

// OSC wave of layer A or B
class WaveGraph : public Graph
{
public:
    using Graph::Graph;
    int layer = 0;
    void paint (Graphics& g) override
    {
        auto r = area();
        screen (g, getLocalBounds().toFloat(), hue, k(), 8, 4);
        const char* e = layer == 0 ? ID::engine : ID::engineB;
        const int eng = (int) v (e);
        const float w = v (layer == 0 ? ID::wave : ID::waveB), fm = v (layer == 0 ? ID::fmAmt : ID::fmAmtB), ratio = v (layer == 0 ? ID::fmRatio : ID::fmRatioB);
        const int warp = (int) v (layer == 0 ? ID::warpMode : ID::warpModeB);
        const int uni = (int) v (layer == 0 ? ID::unison : ID::unisonB);
        const float det = v (layer == 0 ? ID::detune : ID::detuneB);
        const auto& wt = kk::WavetableBank::get();
        auto sample = [&] (float t) -> float
        {
            switch (eng)
            {
                case engVA:  { const float saw = 2 * t - 1, sq = t < 0.5f ? 1.0f : -1.0f;
                               return w <= 0.5f ? saw + (sq - saw) * w * 2 : (t < 0.5f - (w - 0.5f) * 0.8f ? 1.0f : -1.0f); }
                case engFM:  return std::sin (kk::twoPi * t + fm * 5.0f * std::sin (kk::twoPi * t * ratio));
                case engWavetable:
                {
                    float ph = t;
                    switch (warp)
                    {
                        case 1: ph = t * (1 + fm * 6); ph -= std::floor (ph); break;
                        case 2: ph = (t < 0.5f ? t : 1 - t) * 2 * (0.5f + fm * 0.5f); ph -= std::floor (ph); break;
                        case 3: { const float st = 64 - fm * 60; ph = std::floor (t * st) / st; break; }
                        case 4: ph = t + fm * 0.3f * std::sin (kk::twoPi * t * 2); ph -= std::floor (ph); break;
                        default: ph = std::pow (t, 1 + fm * 3); break;
                    }
                    return wt.read (ph, w, 0.001f);
                }
                case engOrgan: { float s = 0, n = 0; for (int h = 1; h <= 8; ++h) { const float a = std::pow (1.0f / (float) h, 2.0f - 1.8f * w); s += a * std::sin (kk::twoPi * t * (float) h); n += a; } return s / (n * 0.7f); }
                case engSub: { const float d = 1 + w * 6; return std::tanh (std::sin (kk::twoPi * t) * d) / std::tanh (d); }
                case engPluck: case engModal: return std::sin (kk::twoPi * t) * std::exp (-t * 2.5f) + 0.3f * std::sin (kk::twoPi * t * 5.4f) * std::exp (-t * 6);
                default: return 2 * t - 1;
            }
        };
        // two cycles, unison shown as fainter detuned copies behind
        for (int u = jmin (uni, 4) - 1; u >= 0; --u)
        {
            Path p;
            const float off = u == 0 ? 0.0f : det * 0.06f * (float) u * (u % 2 ? 1.0f : -1.0f);
            for (int i = 0; i <= 300; ++i)
            {
                const float x = (float) i / 300.0f; float t = x * 2.0f + off; t -= std::floor (t);
                const Point<float> pt (r.getX() + r.getWidth() * x, r.getCentreY() - jlimit (-1.15f, 1.15f, sample (t)) * r.getHeight() * 0.42f);
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            if (u == 0) curve (g, p, hue, r, k());
            else { g.setColour (hue.b.withAlpha (0.28f)); g.strokePath (p, PathStrokeType (1.4f * k())); }
        }
        g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (11.0f * k(), Font::bold)).withExtraKerningFactor (0.1f));
        g.drawText (Choices::engines[eng].toUpperCase() + (uni > 1 ? "  x" + String (uni) : String()), r.removeFromTop (14 * k()), Justification::topRight);
        g.drawText ("drag up/down = WAVE", r.removeFromBottom (14 * k()), Justification::bottomRight);
    }
    void mouseDown (const MouseEvent&) override { begin ({ layer == 0 ? ID::wave : ID::waveB }); drag = 0; startY = v (layer == 0 ? ID::wave : ID::waveB); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (drag != 0) return;
        set (layer == 0 ? ID::wave : ID::waveB, jlimit (0.0f, 1.0f, startY - (float) e.getDistanceFromDragStartY() / (area().getHeight() * 1.5f)));
    }
private:
    float startY = 0;
};

// LFO: moving wave with a dot that runs at the real rate
class LfoGraph : public Graph, private Timer
{
public:
    LfoGraph (KeysKillaProcessor& p, Hue h) : Graph (p, h) {}
    int which = 0;
    void visibilityChanged() override { if (isShowing()) startTimerHz (30); else stopTimer(); }
    void paint (Graphics& g) override
    {
        auto r = area();
        screen (g, getLocalBounds().toFloat(), hue, k(), 12, 4);
        const int shape = (int) v (which == 0 ? ID::lfoShape : ID::lfo2Shape);
        const bool sync = v (which == 0 ? ID::lfoSync : ID::lfo2Sync) > 0.5f;
        const float rate = v (which == 0 ? ID::lfoRate : ID::lfo2Rate);
        const int cycles = sync ? 4 : jlimit (1, 12, roundToInt (rate * 0.6f) + 1);
        Random rnd (7); float sh = rnd.nextFloat() * 2 - 1, lastPh = 0;
        Path p;
        for (int i = 0; i <= 400; ++i)
        {
            const float x = (float) i / 400.0f, ph0 = x * (float) cycles; const float ph = ph0 - std::floor (ph0);
            if (ph < lastPh) sh = rnd.nextFloat() * 2 - 1;
            lastPh = ph;
            const Point<float> pt (r.getX() + r.getWidth() * x, r.getCentreY() - kk::lfoShape (shape, ph, sh) * r.getHeight() * 0.42f);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        curve (g, p, hue, r, k());
        // running dot
        const float x = (float) std::fmod (phase, 1.0);
        const auto pt = p.getPointAlongPath (p.getLength() * x);
        handle (g, pt, hue, k(), false);
        g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (11.0f * k(), Font::bold)).withExtraKerningFactor (0.1f));
        g.drawText (sync ? "SYNC  " + Choices::lfoDivs[(int) v (which == 0 ? ID::lfoDiv : ID::lfo2Div)] : String (rate, 2) + " Hz", r.removeFromTop (14 * k()), Justification::topRight);
    }
private:
    void timerCallback() override
    {
        const bool sync = v (which == 0 ? ID::lfoSync : ID::lfo2Sync) > 0.5f;
        const float rate = v (which == 0 ? ID::lfoRate : ID::lfo2Rate);
        const int cycles = sync ? 4 : jlimit (1, 12, roundToInt (rate * 0.6f) + 1);
        const double hz = sync ? (2.333 / Choices::lfoDivBeats ((int) v (which == 0 ? ID::lfoDiv : ID::lfo2Div))) : rate;   // 140 bpm
        phase += hz / 30.0 / (double) cycles;
        if (phase > 1000) phase = 0;
        repaint();
    }
    double phase = 0;
};

// ADSR you can drag: attack peak, decay/sustain corner, release end
class EnvGraph : public Graph
{
public:
    EnvGraph (KeysKillaProcessor& p, Hue h, const char* a, const char* d, const char* s, const char* r) : Graph (p, h), ia (a), id_ (d), is (s), ir (r)
    { setTooltip ("Drag the points: attack, decay + sustain, release. Double-click = default."); }
    void paint (Graphics& g) override
    {
        screen (g, getLocalBounds().toFloat(), hue, k(), 8, 4);
        const auto pts = points();
        auto r = area();
        Path p; p.startNewSubPath (r.getX(), r.getBottom());
        p.lineTo (pts[0]);
        const float s = v (is);
        for (int i = 1; i <= 20; ++i) { const float t = (float) i / 20.0f; p.lineTo (pts[0].x + (pts[1].x - pts[0].x) * t, r.getBottom() - r.getHeight() * (s + (1 - s) * std::exp (-4.6f * t))); }
        p.lineTo (pts[2].x - r.getWidth() * 0.0f, pts[1].y);
        const float relX0 = pts[2].x;
        p.lineTo (relX0, pts[1].y);
        for (int i = 1; i <= 20; ++i) { const float t = (float) i / 20.0f; p.lineTo (relX0 + (pts[3].x - relX0) * t, r.getBottom() - r.getHeight() * s * std::exp (-4.6f * t)); }
        curve (g, p, hue, r, k(), r.getBottom());
        const int show[] { 0, 1, 3 };
        for (int i : show) handle (g, pts[(size_t) i], hue, k(), i == drag || i == hot);
        g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (10.5f * k(), Font::bold)).withExtraKerningFactor (0.1f));
        g.drawText ("A " + text (ia) + "   D " + text (id_) + "   S " + String (roundToInt (s * 100)) + "%   R " + text (ir), r.removeFromTop (14 * k()), Justification::topRight);
    }
    void mouseMove (const MouseEvent& e) override { const int h = near (e.position); if (h != hot) { hot = h; repaint(); } }
    void mouseDown (const MouseEvent& e) override
    {
        drag = near (e.position);
        if (drag == 0) begin ({ ia }); else if (drag == 1) begin ({ id_, is }); else if (drag == 3) begin ({ ir });
    }
    void mouseDrag (const MouseEvent& e) override
    {
        auto r = area();
        const float seg = r.getWidth() * 0.3f;
        auto tFrom = [&] (float px, float lo, float hi) { const float x = jlimit (0.0f, 1.0f, px / seg); return lo + (hi - lo) * x * x; };
        if (drag == 0) set (ia, tFrom (e.position.x - r.getX(), 0.001f, 5.0f));
        else if (drag == 1)
        {
            set (id_, tFrom (e.position.x - points()[0].x, 0.005f, 8.0f));
            set (is, jlimit (0.0f, 1.0f, (r.getBottom() - e.position.y) / r.getHeight()));
        }
        else if (drag == 3) set (ir, tFrom (e.position.x - points()[2].x, 0.005f, 8.0f));
        repaint();
    }
    void mouseDoubleClick (const MouseEvent&) override
    {
        for (auto* id : { ia, id_, is, ir }) if (auto* rp = proc.apvts.getParameter (id)) { rp->beginChangeGesture(); rp->setValueNotifyingHost (rp->getDefaultValue()); rp->endChangeGesture(); }
        repaint();
    }
private:
    String text (const char* id) const { auto* rp = proc.apvts.getParameter (id); return rp ? rp->getCurrentValueAsText() : String(); }
    // times drawn on a square-root scale so short attacks stay visible
    std::array<Point<float>, 4> points() const
    {
        auto r = area();
        const float seg = r.getWidth() * 0.3f;
        auto X = [&] (float t, float lo, float hi) { return seg * std::sqrt (jlimit (0.0f, 1.0f, (t - lo) / (hi - lo))); };
        const float ax = r.getX() + X (v (ia), 0.001f, 5.0f) + 2;
        const float dx = ax + X (v (id_), 0.005f, 8.0f) + 2;
        const float sy = r.getBottom() - r.getHeight() * v (is);
        const float sx = dx + r.getWidth() * 0.08f;
        const float rx = sx + X (v (ir), 0.005f, 8.0f) + 2;
        return { Point<float> (ax, r.getY()), Point<float> (dx, sy), Point<float> (sx, sy), Point<float> (rx, r.getBottom()) };
    }
    int near (Point<float> p) const
    {
        const auto pts = points(); int best = -1; float bd = 18 * k();
        for (int i : { 0, 1, 3 }) if (const float d = pts[(size_t) i].getDistanceFrom (p); d < bd) { bd = d; best = i; }
        return best;
    }
    const char *ia, *id_, *is, *ir;
};

// FILTER: response curve, drag the point = cutoff (left/right) + resonance (up/down)
class FilterGraph : public Graph
{
public:
    FilterGraph (KeysKillaProcessor& p, Hue h) : Graph (p, h) { setTooltip ("Drag the point: left / right = CUTOFF, up / down = RESONANCE."); }
    static float xOf (float hz) { return std::log (hz / 20.0f) / std::log (1000.0f); }   // 20 Hz .. 20 kHz
    static float hzOf (float x) { return 20.0f * std::pow (1000.0f, jlimit (0.0f, 1.0f, x)); }
    float mag (float hz) const
    {
        const int type = (int) v (ID::filterType);
        const float fc = v (ID::cutoff), q = 0.5f + v (ID::reso) * 9.0f;
        const float w = hz / fc;
        const float w2 = w * w;
        const float denom = std::sqrt ((1 - w2) * (1 - w2) + (w / q) * (w / q));
        switch (type)
        {
            case 3:  return w2 / denom;                              // high pass
            case 4:  return (w / q) / denom * q * 0.7f;              // band pass
            case 5:  return std::abs (1 - w2) / denom;               // notch
            case 6:  return 1.0f + (q * 0.6f) * ((w / q) / denom);   // peak
            case 1:  return 1.0f / std::pow (denom, 2.0f);           // ladder: 24 dB
            default: return 1.0f / denom;                            // clean / dirty: 12 dB
        }
    }
    void paint (Graphics& g) override
    {
        screen (g, getLocalBounds().toFloat(), hue, k(), 10, 4);
        auto r = area();
        auto Y = [&] (float m) { const float db = jlimit (-36.0f, 24.0f, Decibels::gainToDecibels (m, -60.0f)); return r.getY() + r.getHeight() * (1.0f - (db + 36.0f) / 60.0f); };
        Path p;
        for (int i = 0; i <= 240; ++i)
        {
            const float x = (float) i / 240.0f;
            const Point<float> pt (r.getX() + r.getWidth() * x, Y (mag (hzOf (x))));
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (TC (0xffc8c4e8).withAlpha (0.25f)); g.drawHorizontalLine ((int) Y (1.0f), r.getX(), r.getRight());
        curve (g, p, hue, r, k(), r.getBottom());
        handle (g, dot(), hue, k(), drag == 0 || hot == 0);
        g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (10.5f * k(), Font::bold)).withExtraKerningFactor (0.1f));
        auto* c = proc.apvts.getParameter (ID::cutoff);
        g.drawText (Choices::filters[(int) v (ID::filterType)].toUpperCase() + "   " + (c ? c->getCurrentValueAsText() : String()) + "   RES " + String (roundToInt (v (ID::reso) * 100)) + "%",
                    r.removeFromTop (14 * k()), Justification::topRight);
    }
    void mouseMove (const MouseEvent& e) override { const int h = dot().getDistanceFrom (e.position) < 20 * k() ? 0 : -1; if (h != hot) { hot = h; repaint(); } }
    void mouseDown (const MouseEvent&) override { begin ({ ID::cutoff, ID::reso }); drag = 0; }
    void mouseDrag (const MouseEvent& e) override
    {
        auto r = area();
        set (ID::cutoff, jlimit (40.0f, 20000.0f, hzOf ((e.position.x - r.getX()) / r.getWidth())));
        set (ID::reso, jlimit (0.0f, 1.0f, (r.getBottom() - r.getHeight() * 0.25f - e.position.y) / (r.getHeight() * 0.6f)));
        repaint();
    }
private:
    Point<float> dot() const
    {
        auto r = area();
        return { r.getX() + r.getWidth() * xOf (v (ID::cutoff)), r.getBottom() - r.getHeight() * 0.25f - r.getHeight() * 0.6f * v (ID::reso) };
    }
};

// PITCH over one held note: glide in, drift / tape wobble, the BEND at the end
class PitchGraph : public Graph
{
public:
    PitchGraph (KeysKillaProcessor& p, Hue h) : Graph (p, h) { setTooltip ("How the pitch moves over one note: glide in, drift and tape wobble, the bend at the end."); }
    void paint (Graphics& g) override
    {
        screen (g, getLocalBounds().toFloat(), hue, k(), 8, 4);
        auto r = area();
        const float glide = v (ID::glide), drift = v (ID::drift), tape = v (ID::tape), bend = v (ID::bend), semis = v (ID::bendSemis);
        const int mode = (int) v (ID::bendMode);
        const float mid = r.getCentreY(), span = r.getHeight() * 0.42f;
        Path p;
        for (int i = 0; i <= 300; ++i)
        {
            const float t = (float) i / 300.0f;
            float st = 0;
            if (glide > 0.001f) st -= 3.0f * std::exp (-t / (0.02f + glide * 0.4f));
            st += (drift * 0.35f + tape * 0.6f) * std::sin (t * 23.0f) * 0.5f + tape * 0.4f * std::sin (t * 61.0f + 1.3f) * 0.3f;
            if (t > 0.7f && bend > 0.001f)
            {
                const float b = (t - 0.7f) / 0.3f, amt = bend * semis;
                switch (mode)
                {
                    case 0:  st -= amt * b * b; break;                         // dive
                    case 1:  st += amt * b * b; break;                         // rise
                    case 2:  st -= amt * std::sin (b * MathConstants<float>::pi) * 0.6f; break;   // dip
                    case 3:  st += b > 0.5f ? 12.0f * bend : 0.0f; break;      // octave jump
                    default: st += amt * std::sin (b * 17.0f) * 0.5f; break;   // random
                }
            }
            const Point<float> pt (r.getX() + r.getWidth() * t, mid - jlimit (-1.1f, 1.1f, st / 12.0f) * span);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
        g.setColour (TC (0xffc8c4e8).withAlpha (0.25f)); g.drawHorizontalLine ((int) mid, r.getX(), r.getRight());
        curve (g, p, hue, r, k());
        g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (10.5f * k(), Font::bold)).withExtraKerningFactor (0.1f));
        g.drawText ("+12", r.removeFromTop (14 * k()), Justification::topLeft);
        g.drawText ("-12", r.removeFromBottom (14 * k()), Justification::bottomLeft);
    }
};

//==============================================================================
class SoundEditPage : public Component, private Timer
{
public:
    std::function<void()> onClose, onSave, onMatrix;

    SoundEditPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l),
        gWaveA (p, hueOsc()), gWaveB (p, hueOsc()), lfoG (p, hueLfo()), pitchG (p, huePitch()), filterG (p, hueFilter()),
        fenvG (p, hueFilter(), ID::fattack, ID::fdecay, ID::fsustain, ID::frelease),
        ampG (p, hueAmp(), ID::attack, ID::decay, ID::sustain, ID::release),
        modG (p, hueAmp(), ID::e3attack, ID::e3decay, ID::e3sustain, ID::e3release),
        layerTabs ({ "LAYER A", "LAYER B" }, hueOsc()), lfoTabs ({ "LFO 1", "LFO 2" }, hueLfo()),
        filterTabs ({ "FILTER", "ENVELOPE" }, hueFilter()), ampTabs ({ "AMP", "MOD ENV" }, hueAmp())
    {
        using namespace ID;
        // ---- OSC (A and B: two sets, one visible)
        for (int L = 0; L < 2; ++L)
        {
            auto& o = osc[(size_t) L];
            const bool a = L == 0;
            o.engine = std::make_unique<Chips> (p, a ? engine : engineB, hueOsc(), StringArray { "VA", "FM", "PLUCK", "VOX", "ORGAN", "FLUTE", "SUB 808", "TABLE", "ORCH", "MODAL" });
            o.engine->columns = 5;
            o.algo = std::make_unique<Chips> (p, a ? fmAlgo : fmAlgoB, hueOsc());
            o.warp = std::make_unique<Chips> (p, a ? warpMode : warpModeB, hueOsc());
            o.engine->onPick = [this] (int) { layoutOsc(); };
            const char* ids[] { a ? wave : waveB, a ? detune : detuneB, a ? unison : unisonB, a ? fmAmt : fmAmtB, a ? fmRatio : fmRatioB,
                                a ? octave : octaveB, a ? semi : semiB, a ? fine : fineB, a ? levelA : levelB };
            const char* nm[] { "WAVE", "DETUNE", "UNISON", "FM / WARP", "FM RATIO", "OCTAVE", "SEMI", "FINE", "LEVEL" };
            for (int i = 0; i < 9; ++i) o.knobs.push_back (std::make_unique<Knob> (p, ids[i], nm[i], hueOsc()));
            if (a) o.knobs.push_back (std::make_unique<Knob> (p, sub, "SUB", hueOsc()));
            else o.on = std::make_unique<Chips> (p, layerB, hueOsc(), StringArray { "LAYER B ON" });
            for (Component* c : o.all()) addChildComponent (c);
        }
        gWaveB.layer = 1;
        addChildComponent (gWaveA); addChildComponent (gWaveB);
        layerTabs.onChange = [this] (int) { layoutOsc(); };
        addAndMakeVisible (layerTabs);

        // ---- LFO
        for (int L = 0; L < 2; ++L)
        {
            auto& f = lfo[(size_t) L];
            const bool a = L == 0;
            f.shape = std::make_unique<Chips> (p, a ? lfoShape : lfo2Shape, hueLfo(), StringArray { "SINE", "TRI", "SAW", "SQUARE", "S&H" });
            f.sync = std::make_unique<Chips> (p, a ? lfoSync : lfo2Sync, hueLfo(), StringArray { "TEMPO SYNC" });
            f.div = std::make_unique<Chips> (p, a ? lfoDiv : lfo2Div, hueLfo());
            f.knobs.push_back (std::make_unique<Knob> (p, a ? lfoRate : lfo2Rate, "RATE", hueLfo()));
            if (a)
            {
                f.knobs.push_back (std::make_unique<Knob> (p, lfoPitch, "> PITCH", hueLfo()));
                f.knobs.push_back (std::make_unique<Knob> (p, lfoFilter, "> FILTER", hueLfo()));
                f.knobs.push_back (std::make_unique<Knob> (p, lfoAmp, "> VOLUME", hueLfo()));
            }
            for (Component* c : f.all()) addChildComponent (c);
        }
        matrixBtn.setButtonText ("MOD MATRIX"); matrixBtn.framed = true; matrixBtn.tint = hueLfo().a;
        matrixBtn.setTooltip ("LFO 2 moves things through the MOD MATRIX: choose what it moves there.");
        matrixBtn.onClick = [this] { if (onMatrix) onMatrix(); };
        addChildComponent (matrixBtn);
        addAndMakeVisible (lfoG);
        lfoTabs.onChange = [this] (int t) { lfoG.which = t; layoutLfo(); lfoG.repaint(); };
        addAndMakeVisible (lfoTabs);

        // ---- PITCH
        addAndMakeVisible (pitchG);
        pitchChips = std::make_unique<Chips> (p, bendMode, huePitch(), StringArray { "DIVE", "RISE", "DIP", "OCT JUMP", "RANDOM" });
        monoChip = std::make_unique<Chips> (p, mono, huePitch(), StringArray { "MONO" });
        legatoChip = std::make_unique<Chips> (p, legato, huePitch(), StringArray { "LEGATO" });
        for (Component* c : { (Component*) pitchChips.get(), (Component*) monoChip.get(), (Component*) legatoChip.get() }) addAndMakeVisible (c);
        const char* pid[] { glide, bend, bendSemis, drift, tape, bendRange };
        const char* pnm[] { "GLIDE", "BEND", "BEND SIZE", "DRIFT", "TAPE", "WHEEL RANGE" };
        for (int i = 0; i < 6; ++i) { pitchKnobs.push_back (std::make_unique<Knob> (p, pid[i], pnm[i], huePitch())); addAndMakeVisible (*pitchKnobs.back()); }

        // ---- FILTER
        filterChips = std::make_unique<Chips> (p, ID::filterType, hueFilter(), StringArray { "CLEAN", "LADDER", "DIRTY", "HPF", "BPF", "NOTCH", "PEAK" });
        addAndMakeVisible (*filterChips);
        addAndMakeVisible (filterG); addChildComponent (fenvG);
        const char* fid[] { cutoff, reso, keyTrack, fenv };
        const char* fnm[] { "CUTOFF", "RESONANCE", "KEY TRACK", "ENV AMOUNT" };
        for (int i = 0; i < 4; ++i) { filterKnobs.push_back (std::make_unique<Knob> (p, fid[i], fnm[i], hueFilter())); addAndMakeVisible (*filterKnobs.back()); }
        filterTabs.onChange = [this] (int t) { filterG.setVisible (t == 0); fenvG.setVisible (t == 1); filterChips->setVisible (t == 0); };
        addAndMakeVisible (filterTabs);

        // ---- AMP
        addAndMakeVisible (ampG); addChildComponent (modG);
        const char* aid[] { velSens, gain, width };
        const char* anm[] { "VELOCITY", "VOLUME", "WIDTH" };
        for (int i = 0; i < 3; ++i) { ampKnobs.push_back (std::make_unique<Knob> (p, aid[i], anm[i], hueAmp())); addAndMakeVisible (*ampKnobs.back()); }
        ampTabs.onChange = [this] (int t) { ampG.setVisible (t == 0); modG.setVisible (t == 1); };
        addAndMakeVisible (ampTabs);

        // ---- FX: pick an effect, its controls appear
        fxNames = { "DRIVE", "CRUSH", "TAPE WOW", "CHORUS", "PHASER", "FLANGER", "DELAY", "REVERB", "EQ", "PUNCH" };
        auto fx = [this, &p] (int slot, std::initializer_list<std::pair<const char*, const char*>> knobs, std::initializer_list<const char*> chips = {})
        {
            auto& s = fxSlots[(size_t) slot];
            for (auto& kv : knobs) s.knobs.push_back (std::make_unique<Knob> (p, kv.first, kv.second, hueFx()));
            for (auto* c : chips) s.chips.push_back (std::make_unique<Chips> (p, c, hueFx()));
            for (auto& k : s.knobs) addChildComponent (*k);
            for (auto& c : s.chips) addChildComponent (*c);
            s.level = knobs.begin()->first;
        };
        fx (0, { { drive, "DRIVE" } }, { driveType });
        fx (1, { { crush, "CRUSH" } });
        fx (2, { { wow, "WOW" } });
        fx (3, { { chorus, "CHORUS" } });
        fx (4, { { phaser, "PHASER" } });
        fx (5, { { flanger, "FLANGER" } });
        fx (6, { { delayMix, "MIX" }, { delayFb, "FEEDBACK" } }, { delayTime, delayMode });
        fx (7, { { revMix, "MIX" }, { revSize, "SIZE" } }, { revType, freeze });
        fx (8, { { eqLow, "LOW" }, { eqHigh, "HIGH" } });
        fx (9, { { punch, "PUNCH" }, { timeM, "TIME" } });
        const char* mid[] { master, worldAmt };
        const char* mnm[] { "MASTER", "WORLD AMT" };
        for (int i = 0; i < 2; ++i) { masterKnobs.push_back (std::make_unique<Knob> (p, mid[i], mnm[i], hueFx())); addAndMakeVisible (*masterKnobs.back()); }

        // ---- header
        auto hb = [this] (HotButton& b, const String& t, const String& tip, Colour tint, std::function<void()> f)
        { b.setButtonText (t); b.framed = true; b.tint = tint; b.setTooltip (tip); b.onClick = std::move (f); addAndMakeVisible (b); };
        hb (closeBtn, "CLOSE", "Back", TC (0xffff2f6d), [this] { if (onClose) onClose(); });
        hb (saveBtn, "SAVE", "Save this sound as your preset", TC (0xffffb020), [this] { if (onSave) onSave(); });
        hb (resetBtn, "RESET", "Put every control on this page back to how the sound was loaded", TC (0xff22d3ee), [this] { proc.resetParams (allIds()); repaint(); });
        hb (synthBtn, "PLAY THIS SOUND", "The keys play a sample sound now (PAIR / VST / DIGGA / drums). Click: the keys play this synth sound again.", TC (0xff36ff6a),
            [this] { if (auto* rp = proc.apvts.getParameter (ID::playMode)) { rp->beginChangeGesture(); rp->setValueNotifyingHost (rp->convertTo0to1 (0.0f)); rp->endChangeGesture(); } });
        addChildComponent (synthBtn);

        setWantsKeyboardFocus (false);
        layoutOsc(); layoutLfo();
        startTimerHz (12);
    }

    void paint (Graphics& g) override
    {
        const float k = kf();
        {
            Graphics::ScopedSaveState ss (g);
            Path clip; clip.addRoundedRectangle (getLocalBounds().toFloat(), 12 * k); g.reduceClipRegion (clip);
            pageBackdrop (g, *this, 0.75f);
        }
        // v0.34: no coloured glows (calm)
        auto glow = [] (Point<float>, float, Colour) {};
        glow (U (300, 200), 420 * k, hueOsc().a); glow (U (1200, 200), 380 * k, hueLfo().a); glow (U (760, 480), 380 * k, hueFilter().a); glow (U (1300, 700), 320 * k, hueFx().a);
        kk::modern::waves (g, U (320, 34), U (1512, 6), 22 * k, hueOsc().a, hueLfo().b, 5, 0.22f);

        // header
        g.setColour (TC (0xffffffff)); g.setFont (Font (FontOptions (30.0f * k, Font::bold)).withExtraKerningFactor (0.06f));
        g.drawText ("SOUND", UR (16, 6, 130, 46), Justification::centredLeft);
        g.setGradientFill (ColourGradient (hueOsc().a, U (128, 0).x, 0, hueLfo().b, U (220, 0).x, 0, false));
        g.drawText ("EDIT", UR (128, 6, 120, 46), Justification::centredLeft);
        g.setColour (TC (0xffe6e3ff)); g.setFont (Font (FontOptions (17.0f * k)));
        g.drawFittedText (proc.currentName(), UR (232, 10, 520, 38).toNearestInt(), Justification::centredLeft, 1);
        if (samplePlays)
        {
            g.setColour (TC (0xff36ff6a)); g.setFont (Font (FontOptions (11.5f * k, Font::bold)));
            g.drawFittedText ("THE KEYS PLAY A SAMPLE SOUND NOW - EDIT CHANGES THIS SYNTH SOUND", UR (232, 40, 560, 16).toNearestInt(), Justification::centredLeft, 1);
        }

        panel (g, UR (rOsc), hueOsc(), k);       title (g, UR (rOsc), "OSC", layerTabs.sel == 0 ? "LAYER A" : (proc.apvts.getRawParameterValue (ID::layerB)->load() > 0.5f ? "LAYER B" : "LAYER B  (off)"), hueOsc(), k);
        panel (g, UR (rLfo), hueLfo(), k);       title (g, UR (rLfo), "LFO", lfoTabs.sel == 0 ? "MOVES PITCH / FILTER / VOLUME" : "THROUGH THE MOD MATRIX", hueLfo(), k);
        panel (g, UR (rPitch), huePitch(), k);   title (g, UR (rPitch), "PITCH", "ONE NOTE", huePitch(), k);
        panel (g, UR (rFilter), hueFilter(), k); title (g, UR (rFilter), "FILTER", {}, hueFilter(), k);
        panel (g, UR (rAmp), hueAmp(), k);       title (g, UR (rAmp), "AMP", {}, hueAmp(), k);
        panel (g, UR (rFx), hueFx(), k);         title (g, UR (rFx), "FX", "PICK AN EFFECT - THE LIT ONES ARE ON", hueFx(), k);
        if (lfoTabs.sel == 1)
        {
            g.setColour (TC (0xffc8c4e8)); g.setFont (Font (FontOptions (12.0f * k)));
            g.drawFittedText ("LFO 2 has no fixed target: in the MOD MATRIX pick \"LFO 2\" as a source and what it should move.",
                              UR (rLfo.getX() + 300, rLfo.getBottom() - 92, rLfo.getWidth() - 320, 40).toNearestInt(), Justification::centredLeft, 2);
        }
        // FX chooser
        for (int i = 0; i < numFx; ++i)
        {
            auto r = fxCell (i);
            const bool sel = i == fxSel, on = fxOn (i);
            if (sel)
            {
                g.setColour (hueFx().a.withAlpha (0.3f)); g.fillRoundedRectangle (r.expanded (2 * k), 8 * k);
                g.setGradientFill (ColourGradient (hueFx().a, r.getX(), 0, hueFx().b, r.getRight(), 0, false)); g.fillRoundedRectangle (r, 6 * k);
            }
            else { g.setColour (TC (0xff17153a)); g.fillRoundedRectangle (r, 6 * k); g.setColour (TC (0xff3a3264)); g.drawRoundedRectangle (r, 6 * k, 1.0f); }
            g.setColour (on ? TC (0xff36ff6a) : TC (0xff3a3264)); g.fillEllipse (r.getX() + 8 * k, r.getCentreY() - 3.5f * k, 7 * k, 7 * k);
            g.setColour (sel ? TC (0xffffffff) : TC (0xffc8c4e8)); g.setFont (Font (FontOptions (11.5f * k, Font::bold)).withExtraKerningFactor (0.06f));
            g.drawFittedText (fxNames[i], r.withTrimmedLeft (18 * k).toNearestInt(), Justification::centred, 1, 0.7f);
        }
        g.setColour (TC (0xff3a3264)); g.drawVerticalLine ((int) U (rFx.getRight() - 230, 0).x, U (0, rFx.getY() + 46).y, U (0, rFx.getBottom() - 12).y);
    }

    void resized() override
    {
        auto place = [&] (Component& c, float x, float y, float w, float h) { c.setBounds (UR (x, y, w, h).toNearestInt()); };
        place (closeBtn, 1390, 10, 108, 38); place (saveBtn, 1274, 10, 108, 38); place (resetBtn, 1158, 10, 108, 38); place (synthBtn, 960, 10, 190, 38);

        // OSC
        place (layerTabs, rOsc.getRight() - 236, rOsc.getY() + 10, 222, 24);
        for (int L = 0; L < 2; ++L)
        {
            auto& o = osc[(size_t) L];
            place (*o.engine, rOsc.getX() + 14, rOsc.getY() + 42, 430, 50);
            if (o.on) place (*o.on, rOsc.getX() + 456, rOsc.getY() + 42, 140, 24);
            place (*o.algo, rOsc.getX() + 456, rOsc.getY() + 68, 440, 24);
            place (*o.warp, rOsc.getX() + 456, rOsc.getY() + 68, 380, 24);
        }
        place (gWaveA, rOsc.getX() + 14, rOsc.getY() + 100, rOsc.getWidth() - 28, 120);
        place (gWaveB, rOsc.getX() + 14, rOsc.getY() + 100, rOsc.getWidth() - 28, 120);
        for (int L = 0; L < 2; ++L)
        {
            auto& o = osc[(size_t) L];
            const float kw = (rOsc.getWidth() - 28) / 10.0f;
            for (size_t i = 0; i < o.knobs.size(); ++i) place (*o.knobs[i], rOsc.getX() + 14 + kw * (float) i, rOsc.getY() + 226, kw, 70);
        }
        // LFO
        place (lfoTabs, rLfo.getRight() - 196, rLfo.getY() + 10, 182, 24);
        for (auto& f : lfo)
        {
            place (*f.shape, rLfo.getX() + 14, rLfo.getY() + 42, 300, 24);
            place (*f.sync, rLfo.getX() + 324, rLfo.getY() + 42, 120, 24);
            place (*f.div, rLfo.getX() + 14, rLfo.getY() + 70, rLfo.getWidth() - 28, 22);
        }
        place (lfoG, rLfo.getX() + 14, rLfo.getY() + 100, rLfo.getWidth() - 28, 120);
        for (auto& f : lfo)
            for (size_t i = 0; i < f.knobs.size(); ++i) place (*f.knobs[i], rLfo.getX() + 14 + 96.0f * (float) i, rLfo.getY() + 226, 92, 70);
        place (matrixBtn, rLfo.getRight() - 170, rLfo.getY() + 246, 156, 34);
        // PITCH
        place (pitchG, rPitch.getX() + 14, rPitch.getY() + 42, rPitch.getWidth() - 28, 98);
        place (*pitchChips, rPitch.getX() + 14, rPitch.getY() + 146, 300, 22);
        place (*monoChip, rPitch.getX() + 322, rPitch.getY() + 146, 66, 22);
        place (*legatoChip, rPitch.getX() + 392, rPitch.getY() + 146, 74, 22);
        for (size_t i = 0; i < pitchKnobs.size(); ++i) place (*pitchKnobs[i], rPitch.getX() + 10 + 76.0f * (float) i, rPitch.getY() + 172, 76, 66);
        // FILTER
        place (filterTabs, rFilter.getRight() - 216, rFilter.getY() + 10, 202, 22);
        place (filterG, rFilter.getX() + 14, rFilter.getY() + 70, rFilter.getWidth() - 28, 98);
        place (fenvG, rFilter.getX() + 14, rFilter.getY() + 42, rFilter.getWidth() - 28, 126);
        place (*filterChips, rFilter.getX() + 14, rFilter.getY() + 42, rFilter.getWidth() - 28, 24);
        for (size_t i = 0; i < filterKnobs.size(); ++i) place (*filterKnobs[i], rFilter.getX() + 14 + (rFilter.getWidth() - 28) / 4.0f * (float) i, rFilter.getY() + 172, (rFilter.getWidth() - 28) / 4.0f, 66);
        // AMP
        place (ampTabs, rAmp.getRight() - 196, rAmp.getY() + 10, 182, 22);
        place (ampG, rAmp.getX() + 14, rAmp.getY() + 42, rAmp.getWidth() - 28, 126);
        place (modG, rAmp.getX() + 14, rAmp.getY() + 42, rAmp.getWidth() - 28, 126);
        for (size_t i = 0; i < ampKnobs.size(); ++i) place (*ampKnobs[i], rAmp.getX() + 14 + (rAmp.getWidth() - 28) / 3.0f * (float) i, rAmp.getY() + 172, (rAmp.getWidth() - 28) / 3.0f, 66);
        // FX
        for (auto& s : fxSlots)
        {
            for (size_t i = 0; i < s.knobs.size(); ++i) place (*s.knobs[i], rFx.getX() + 640 + 96.0f * (float) i, rFx.getY() + 44, 92, 74);
            for (size_t i = 0; i < s.chips.size(); ++i)
            {
                place (*s.chips[i], rFx.getX() + 640 + 96.0f * (float) s.knobs.size() + 10, rFx.getY() + 48 + 34.0f * (float) i, 300, 26);
            }
        }
        for (size_t i = 0; i < masterKnobs.size(); ++i) place (*masterKnobs[i], rFx.getRight() - 216 + 104.0f * (float) i, rFx.getY() + 44, 96, 74);
        layoutFx();
    }

    void mouseDown (const MouseEvent& e) override
    {
        for (int i = 0; i < numFx; ++i) if (fxCell (i).contains (e.position)) { fxSel = i; layoutFx(); repaint(); return; }
    }

    StringArray allIds() const
    {
        using namespace ID;
        return { engine, octave, semi, fine, wave, unison, detune, fmRatio, fmAmt, fmAlgo, warpMode, levelA, sub,
                 layerB, engineB, octaveB, semiB, fineB, waveB, unisonB, detuneB, fmRatioB, fmAmtB, fmAlgoB, warpModeB, levelB,
                 lfoRate, lfoSync, lfoDiv, lfoShape, lfoPitch, lfoFilter, lfoAmp, lfo2Rate, lfo2Sync, lfo2Div, lfo2Shape,
                 glide, bend, bendMode, bendSemis, drift, tape, bendRange, mono, legato,
                 filterType, cutoff, reso, keyTrack, fenv, fattack, fdecay, fsustain, frelease,
                 attack, decay, sustain, release, velSens, gain, width, e3attack, e3decay, e3sustain, e3release,
                 drive, driveType, crush, wow, chorus, phaser, flanger, delayMix, delayFb, delayTime, delayMode,
                 revMix, revSize, revType, freeze, eqLow, eqHigh, punch, timeM, master, worldAmt };
    }

private:
    // design space: 1512 x 798 right of the left tiles (OX), above the keyboard; the page itself also runs under the tiles
    static constexpr float OX = 140.0f, DW = 1512.0f, DH = 798.0f;
    float kf() const { return jmax (0.3f, (float) getWidth() / (DW + OX)); }
    Point<float> U (float x, float y) const { return { (x + OX) * (float) getWidth() / (DW + OX), y * (float) getHeight() / DH }; }
    Rectangle<float> UR (float x, float y, float w, float h) const { const auto a = U (x, y), b = U (x + w, y + h); return { a.x, a.y, b.x - a.x, b.y - a.y }; }
    Rectangle<float> UR (Rectangle<float> r) const { return UR (r.getX(), r.getY(), r.getWidth(), r.getHeight()); }

    const Rectangle<float> rOsc { 8, 62, 900, 304 }, rLfo { 920, 62, 584, 304 },
                           rPitch { 8, 376, 486, 246 }, rFilter { 504, 376, 500, 246 }, rAmp { 1014, 376, 490, 246 },
                           rFx { 8, 632, 1496, 160 };
    static constexpr int numFx = 10;
    Rectangle<float> fxCell (int i) const { return UR (rFx.getX() + 14 + (float) (i % 5) * 122.0f, rFx.getY() + 46 + (float) (i / 5) * 52.0f, 116, 44); }
    bool fxOn (int i) const
    {
        const auto* s = &fxSlots[(size_t) i];
        if (s->level == nullptr) return false;
        const float x = proc.apvts.getRawParameterValue (s->level)->load();
        if (i == 8) return std::abs (x) > 0.05f || std::abs (proc.apvts.getRawParameterValue (ID::eqHigh)->load()) > 0.05f;
        if (i == 9) return x > 0.01f || std::abs (proc.apvts.getRawParameterValue (ID::timeM)->load() - 0.5f) > 0.02f;
        return x > 0.001f;
    }

    void layoutOsc()
    {
        const int L = layerTabs.sel;
        for (int i = 0; i < 2; ++i)
        {
            auto& o = osc[(size_t) i];
            const bool vis = i == L;
            const int eng = (int) proc.apvts.getRawParameterValue (i == 0 ? ID::engine : ID::engineB)->load();
            o.engine->setVisible (vis);
            for (auto& kn : o.knobs) kn->setVisible (vis);
            if (o.on) o.on->setVisible (vis);
            o.algo->setVisible (vis && eng == engFM);
            o.warp->setVisible (vis && eng == engWavetable);
        }
        gWaveA.setVisible (L == 0); gWaveB.setVisible (L == 1);
        repaint();
    }
    void layoutLfo()
    {
        const int L = lfoTabs.sel;
        for (int i = 0; i < 2; ++i)
        {
            auto& f = lfo[(size_t) i];
            const bool vis = i == L;
            const bool sync = proc.apvts.getRawParameterValue (i == 0 ? ID::lfoSync : ID::lfo2Sync)->load() > 0.5f;
            f.shape->setVisible (vis); f.sync->setVisible (vis); f.div->setVisible (vis && sync);
            for (size_t k = 0; k < f.knobs.size(); ++k) f.knobs[k]->setVisible (vis && (k > 0 || ! sync));
        }
        matrixBtn.setVisible (L == 1);
        repaint();
    }
    void layoutFx()
    {
        for (int i = 0; i < numFx; ++i)
        {
            for (auto& kn : fxSlots[(size_t) i].knobs) kn->setVisible (i == fxSel);
            for (auto& c : fxSlots[(size_t) i].chips) c->setVisible (i == fxSel);
        }
    }

    void timerCallback() override
    {
        // repaint what changed (presets, undo, automation, the macro knobs ...)
        float h = 0; int n = 1;
        for (auto& id : allIds()) h += proc.apvts.getRawParameterValue (id)->load() * (float) (n++ % 97 + 1) * 0.013f;
        const bool sp = (int) proc.apvts.getRawParameterValue (ID::playMode)->load() != 0;
        if (sp != samplePlays) { samplePlays = sp; synthBtn.setVisible (false); repaint(); }   // v0.37: EDIT opens the sample editor for samples
        if (proc.currentName() != lastName) { lastName = proc.currentName(); repaint(); }
        if (h != lastHash)
        {
            lastHash = h;
            const int eA = (int) proc.apvts.getRawParameterValue (ID::engine)->load(), eB = (int) proc.apvts.getRawParameterValue (ID::engineB)->load();
            const bool s1 = proc.apvts.getRawParameterValue (ID::lfoSync)->load() > 0.5f, s2 = proc.apvts.getRawParameterValue (ID::lfo2Sync)->load() > 0.5f;
            if (eA != lastEng[0] || eB != lastEng[1]) { lastEng = { eA, eB }; layoutOsc(); }
            if (s1 != lastSync[0] || s2 != lastSync[1]) { lastSync = { s1, s2 }; layoutLfo(); }
            for (auto* c : getChildren()) c->repaint();
            repaint();
        }
    }

    struct Osc
    {
        std::unique_ptr<Chips> engine, algo, warp, on;
        std::vector<std::unique_ptr<Knob>> knobs;
        std::vector<Component*> all() { std::vector<Component*> v { engine.get(), algo.get(), warp.get() }; if (on) v.push_back (on.get()); for (auto& k : knobs) v.push_back (k.get()); return v; }
    };
    struct Lfo
    {
        std::unique_ptr<Chips> shape, sync, div;
        std::vector<std::unique_ptr<Knob>> knobs;
        std::vector<Component*> all() { std::vector<Component*> v { shape.get(), sync.get(), div.get() }; for (auto& k : knobs) v.push_back (k.get()); return v; }
    };
    struct FxSlot
    {
        std::vector<std::unique_ptr<Knob>> knobs;
        std::vector<std::unique_ptr<Chips>> chips;
        const char* level = nullptr;
    };

    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
    WaveGraph gWaveA, gWaveB;
    LfoGraph lfoG;
    PitchGraph pitchG;
    FilterGraph filterG;
    EnvGraph fenvG, ampG, modG;
    Tabs layerTabs, lfoTabs, filterTabs, ampTabs;
    std::array<Osc, 2> osc;
    std::array<Lfo, 2> lfo;
    HotButton matrixBtn { lnf }, closeBtn { lnf }, saveBtn { lnf }, resetBtn { lnf }, synthBtn { lnf };
    std::unique_ptr<Chips> pitchChips, monoChip, legatoChip, filterChips;
    std::vector<std::unique_ptr<Knob>> pitchKnobs, filterKnobs, ampKnobs, masterKnobs;
    std::array<FxSlot, 10> fxSlots;
    StringArray fxNames;
    int fxSel = 0;
    float lastHash = -1;
    std::array<int, 2> lastEng { -1, -1 };
    std::array<bool, 2> lastSync { false, false };
    bool samplePlays = false;
    String lastName;
};
} // namespace kkedit
