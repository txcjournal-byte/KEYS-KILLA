#pragma once
#ifndef KK_VERSION_STRING
 #define KK_VERSION_STRING "dev"
#endif
// ADVANCED page: ENGINE A / ENGINE B / FILTER / MOD / FX / EXCLUSIVE / PLAY / SETTINGS.
// Included from PluginEditor.cpp after KKLookAndFeel and the processor are known.

//==============================================================================
// Generic grid of controls for a list of parameter ids
class ParamGrid : public Component
{
public:
    ParamGrid (KeysKillaProcessor& p, const StringArray& ids, int columns = 10, const StringArray& labels = {}) : cols (columns)
    {
        for (int n = 0; n < ids.size(); ++n)
        {
            const auto& id = ids[n];
            auto* rp = p.apvts.getParameter (id);
            if (! rp) continue;
            auto item = std::make_unique<Item>();
            item->label.setText (labels[n].isNotEmpty() ? labels[n] : rp->getName (20).toUpperCase(), dontSendNotification);
            item->label.setJustificationType (Justification::centred);
            item->label.setFont (serif (19.0f, false, 0.05f));
            item->label.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (item->label);
            const auto tip = paramTooltip (id);
            if (auto* c = dynamic_cast<AudioParameterChoice*> (rp))
            {
                item->combo = std::make_unique<ComboBox>();
                item->combo->addItemList (c->choices, 1);
                item->combo->setTooltip (tip);
                addAndMakeVisible (*item->combo);
                item->ca = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, id, *item->combo);
            }
            else if (dynamic_cast<AudioParameterBool*> (rp))
            {
                item->toggle = std::make_unique<ToggleButton> ("ON");
                item->toggle->setTooltip (tip);
                addAndMakeVisible (*item->toggle);
                item->ba = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, id, *item->toggle);
            }
            else
            {
                item->slider = std::make_unique<Slider> (Slider::RotaryHorizontalVerticalDrag, Slider::TextBoxBelow);
                item->slider->setTextBoxStyle (Slider::TextBoxBelow, false, 120, 26);
                item->slider->setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
                item->slider->setVelocityModeParameters (0.6, 1, 0.02, true, ModifierKeys::ctrlModifier);
                item->slider->setDoubleClickReturnValue (true, rp->convertFrom0to1 (rp->getDefaultValue()));
                item->slider->setTooltip (tip);
                addAndMakeVisible (*item->slider);
                item->sa = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, id, *item->slider);
            }
            items.push_back (std::move (item));
        }
    }
    int rowsNeeded() const { return ((int) items.size() + cols - 1) / cols; }
    void resized() override
    {
        const int cw = getWidth() / cols, rh = jmin (124, getHeight() / jmax (1, rowsNeeded()));
        for (size_t i = 0; i < items.size(); ++i)
        {
            auto& it = *items[i];
            Rectangle<int> cell ((int) (i % (size_t) cols) * cw, (int) (i / (size_t) cols) * rh, cw, rh);
            it.label.setBounds (cell.removeFromTop (26));
            if (it.slider) it.slider->setBounds (cell.reduced (8, 0).withTrimmedBottom (4));
            if (it.combo) it.combo->setBounds (cell.withSizeKeepingCentre (cw - 10, 32));
            if (it.toggle) it.toggle->setBounds (cell.withSizeKeepingCentre (80, 32));
        }
    }
private:
    struct Item
    {
        Label label;
        std::unique_ptr<Slider> slider; std::unique_ptr<ComboBox> combo; std::unique_ptr<ToggleButton> toggle;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> sa;
        std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> ca;
        std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> ba;
    };
    int cols;
    std::vector<std::unique_ptr<Item>> items;
};

//==============================================================================
// Small live visualisers
class KKVisual : public Component, private Timer
{
public:
    KKVisual (KeysKillaProcessor& p, KKLookAndFeel& l, String t) : proc (p), lnf (l), title (std::move (t)) { startTimerHz (15); }
    float val (const char* id) const { return proc.apvts.getRawParameterValue (id)->load(); }
    void paint (Graphics& g) override
    {
        const auto& s = *lnf.skin;
        auto r = getLocalBounds().toFloat();
        g.setColour (TC (0xff0a0808));
        g.fillRoundedRectangle (r, 5);
        g.setColour (s.panelEdge); g.drawRoundedRectangle (r.reduced (0.5f), 5, 1);
        g.setColour (s.textDim); g.setFont (serif (16.0f, false, 0.2f));
        g.drawText (title, r.reduced (8, 4), Justification::topLeft);
        auto area = r.reduced (10, 22).translated (0, 6);
        Path p; drawCurve (p, area);
        g.setColour (s.accent.withAlpha (0.25f)); g.strokePath (p, PathStrokeType (5.0f));
        g.setColour (s.accent); g.strokePath (p, PathStrokeType (1.8f));
    }
protected:
    virtual void drawCurve (Path& p, Rectangle<float> a) = 0;
    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
private:
    void timerCallback() override
    {
        float h = 0; hashParams (h);
        if (h != lastHash) { lastHash = h; repaint(); }
    }
    virtual void hashParams (float& h) = 0;
    String title;
    float lastHash = -1;
};

class EnvView : public KKVisual
{
public:
    EnvView (KeysKillaProcessor& p, KKLookAndFeel& l, String t, const char* a, const char* d, const char* s, const char* r)
        : KKVisual (p, l, std::move (t)), ia (a), id_ (d), is (s), ir (r) {}
private:
    void hashParams (float& h) override { h = val (ia) * 1.1f + val (id_) * 3.3f + val (is) * 7.7f + val (ir) * 13.1f; }
    void drawCurve (Path& p, Rectangle<float> r) override
    {
        const float a = val (ia), d = val (id_), s = val (is), rel = val (ir);
        const float total = a + d + 0.4f + rel;
        auto X = [&] (float t) { return r.getX() + r.getWidth() * std::sqrt (t / total); };
        p.startNewSubPath (r.getX(), r.getBottom());
        p.lineTo (X (a), r.getY());
        for (int i = 1; i <= 16; ++i) { const float t = (float) i / 16.0f; p.lineTo (X (a + d * t), r.getBottom() - r.getHeight() * (s + (1 - s) * std::exp (-4.6f * t))); }
        p.lineTo (X (a + d + 0.4f), r.getBottom() - r.getHeight() * s);
        for (int i = 1; i <= 16; ++i) { const float t = (float) i / 16.0f; p.lineTo (X (a + d + 0.4f + rel * t), r.getBottom() - r.getHeight() * s * std::exp (-4.6f * t)); }
    }
    const char *ia, *id_, *is, *ir;
};

class LfoView : public KKVisual
{
public:
    LfoView (KeysKillaProcessor& p, KKLookAndFeel& l, String t, const char* shape) : KKVisual (p, l, std::move (t)), ishape (shape) {}
private:
    void hashParams (float& h) override { h = val (ishape); }
    void drawCurve (Path& p, Rectangle<float> r) override
    {
        const int shape = (int) val (ishape);
        Random rnd (3); float sh = rnd.nextFloat() * 2 - 1; float lastPh = 0;
        for (int i = 0; i <= 200; ++i)
        {
            const float x = (float) i / 200.0f * 2.0f; float ph = x - std::floor (x);
            if (ph < lastPh) sh = rnd.nextFloat() * 2 - 1;
            lastPh = ph;
            const float v = kk::lfoShape (shape, ph, sh);
            const Point<float> pt (r.getX() + r.getWidth() * (float) i / 200.0f, r.getCentreY() - v * r.getHeight() * 0.45f);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
    }
    const char* ishape;
};

class WaveView : public KKVisual
{
public:
    WaveView (KeysKillaProcessor& p, KKLookAndFeel& l, int layerIndex)
        : KKVisual (p, l, layerIndex == 0 ? "LAYER A WAVE" : "LAYER B WAVE"), layer (layerIndex) {}
private:
    const char* id (const char* a, const char* b) const { return layer == 0 ? a : b; }
    void hashParams (float& h) override
    {
        h = val (id (ID::engine, ID::engineB)) * 1.3f + val (id (ID::wave, ID::waveB)) * 5.1f + val (id (ID::fmAmt, ID::fmAmtB)) * 9.7f
          + val (id (ID::fmRatio, ID::fmRatioB)) * 0.37f + val (id (ID::warpMode, ID::warpModeB)) * 17.3f + val (id (ID::fmAlgo, ID::fmAlgoB)) * 23.1f;
    }
    void drawCurve (Path& p, Rectangle<float> r) override
    {
        const int eng = (int) val (id (ID::engine, ID::engineB));
        const float w = val (id (ID::wave, ID::waveB)), fm = val (id (ID::fmAmt, ID::fmAmtB)), ratio = val (id (ID::fmRatio, ID::fmRatioB));
        const auto& wt = kk::WavetableBank::get();
        const int warp = (int) val (id (ID::warpMode, ID::warpModeB));
        for (int i = 0; i <= 256; ++i)
        {
            const float t = (float) i / 256.0f;
            float v;
            switch (eng)
            {
                case engVA:  { const float saw = 2 * t - 1, sq = t < 0.5f ? 1.0f : -1.0f;
                               v = w <= 0.5f ? saw + (sq - saw) * w * 2 : (t < 0.5f - (w - 0.5f) * 0.8f ? 1.0f : -1.0f); break; }
                case engFM:  v = std::sin (kk::twoPi * t + fm * 5.0f * std::sin (kk::twoPi * t * ratio)); break;
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
                    v = wt.read (ph, w, 0.001f); break;
                }
                case engOrgan: { v = 0; float n = 0; for (int h = 1; h <= 8; ++h) { const float a = std::pow (1.0f / h, 2.0f - 1.8f * w); v += a * std::sin (kk::twoPi * t * h); n += a; } v /= n * 0.7f; break; }
                case engSub: { const float d = 1 + w * 6; v = std::tanh (std::sin (kk::twoPi * t) * d) / std::tanh (d); break; }
                case engPluck: case engModal: v = std::sin (kk::twoPi * t) * std::exp (-t * 2.5f) + 0.3f * std::sin (kk::twoPi * t * 5.4f) * std::exp (-t * 6); break;
                default: v = 2 * t - 1; break;   // vox / orchestral start from saws
            }
            const Point<float> pt (r.getX() + r.getWidth() * t, r.getCentreY() - jlimit (-1.2f, 1.2f, v) * r.getHeight() * 0.42f);
            if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
        }
    }
    int layer;
};

//==============================================================================
class ModMatrixView : public Component
{
public:
    ModMatrixView (KeysKillaProcessor& p)
    {
        for (int s = 0; s < numModSlots; ++s)
        {
            auto& r = rows[(size_t) s];
            r.src.addItemList (Choices::modSources, 1); r.dst.addItemList (Choices::modDests, 1);
            r.amt.setSliderStyle (Slider::LinearHorizontal); r.amt.setTextBoxStyle (Slider::TextBoxRight, false, 56, 18);
            r.amt.setDoubleClickReturnValue (true, 0.0);
            r.src.setTooltip ("Mod matrix source."); r.dst.setTooltip ("Mod matrix destination."); r.amt.setTooltip ("Mod amount (negative inverts).");
            for (Component* c : { (Component*) &r.src, (Component*) &r.dst, (Component*) &r.amt }) addAndMakeVisible (c);
            r.sa = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ID::mmSrc (s), r.src);
            r.da = std::make_unique<AudioProcessorValueTreeState::ComboBoxAttachment> (p.apvts, ID::mmDst (s), r.dst);
            r.aa = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (p.apvts, ID::mmAmt (s), r.amt);
        }
    }
    void paint (Graphics& g) override
    {
        g.setFont (serif (15.0f, false, 0.2f));
        g.setColour (findColour (Label::textColourId));
        g.drawText ("MOD MATRIX   SOURCE", 0, 0, 260, 16, Justification::centredLeft);
        g.drawText ("DESTINATION", getWidth() * 36 / 100, 0, 150, 16, Justification::centredLeft);
        g.drawText ("AMOUNT", getWidth() * 70 / 100, 0, 100, 16, Justification::centredLeft);
    }
    void resized() override
    {
        const int rh = (getHeight() - 18) / numModSlots;
        for (int s = 0; s < numModSlots; ++s)
        {
            auto row = Rectangle<int> (0, 18 + s * rh, getWidth(), rh).reduced (0, 2);
            auto& r = rows[(size_t) s];
            r.src.setBounds (row.removeFromLeft (getWidth() * 34 / 100)); row.removeFromLeft (6);
            r.dst.setBounds (row.removeFromLeft (getWidth() * 32 / 100)); row.removeFromLeft (6);
            r.amt.setBounds (row);
        }
    }
private:
    struct Row
    {
        ComboBox src, dst; Slider amt;
        std::unique_ptr<AudioProcessorValueTreeState::ComboBoxAttachment> sa, da;
        std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> aa;
    };
    std::array<Row, numModSlots> rows;
};

//==============================================================================
// Drag the rows to change the FX order
class FxOrderList : public Component, public SettableTooltipClient
{
public:
    FxOrderList (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    { setTooltip ("Drag a slot up or down to change the effect order."); order = proc.getFxOrder(); }
    void paint (Graphics& g) override
    {
        order = dragging ? order : proc.getFxOrder();
        const auto& s = *lnf.skin;
        g.setFont (serif (11.0f, false, 0.2f)); g.setColour (s.textDim);
        g.drawText ("FX ORDER (DRAG)", 0, 0, getWidth(), 16, Justification::centredLeft);
        for (int i = 0; i < kk::numFxSlots; ++i)
        {
            auto r = rowRect (i).toFloat();
            const bool active = dragging && i == dragRow;
            g.setColour (active ? s.accent.withAlpha (0.3f) : (TC (0xff141010)));
            g.fillRoundedRectangle (r.reduced (1), 3);
            g.setColour (active ? s.accent : s.panelEdge); g.drawRoundedRectangle (r.reduced (1), 3, 1);
            g.setColour (s.text); g.setFont (serif (12.0f, false, 0.1f));
            g.drawText (String (i + 1) + ".  " + kk::fxSlotName (order[(size_t) i]), r.reduced (8, 0), Justification::centredLeft);
            g.drawText ("=", r.reduced (8, 0), Justification::centredRight);
        }
    }
    void mouseDown (const MouseEvent& e) override { dragRow = rowAt (e.y); dragging = dragRow >= 0; repaint(); }
    void mouseDrag (const MouseEvent& e) override
    {
        if (! dragging) return;
        const int r = jlimit (0, kk::numFxSlots - 1, rowAt (e.y) < 0 ? (e.y < 18 ? 0 : kk::numFxSlots - 1) : rowAt (e.y));
        if (r != dragRow) { std::swap (order[(size_t) r], order[(size_t) dragRow]); dragRow = r; repaint(); }
    }
    void mouseUp (const MouseEvent&) override { if (dragging) proc.setFxOrder (order); dragging = false; repaint(); }
private:
    Rectangle<int> rowRect (int i) const { const int rh = (getHeight() - 18) / kk::numFxSlots; return { 0, 18 + i * rh, getWidth(), rh }; }
    int rowAt (int y) const { for (int i = 0; i < kk::numFxSlots; ++i) if (rowRect (i).contains (4, y)) return i; return -1; }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    std::array<int, kk::numFxSlots> order {};
    int dragRow = -1; bool dragging = false;
};

//==============================================================================
class MorphCorners : public Component
{
public:
    MorphCorners (KeysKillaProcessor& p) : proc (p)
    {
        const char* names[] { "CLASSIC (top left)", "MELODIC (top right)", "RAW (bottom left)", "AGGRESSIVE (bottom right)" };
        for (int c = 0; c < 4; ++c)
        {
            auto& cb = boxes[(size_t) c];
            cb.addItem ("Character only (no preset)", 1);
            const auto& ps = factoryPresets();
            for (int i = 0; i < (int) ps.size(); ++i)
                if (! ps[(size_t) i].name.endsWith (" Lo-Fi") && ! ps[(size_t) i].name.endsWith (" Dark") && ! ps[(size_t) i].name.endsWith (" Blown"))
                    cb.addItem (ps[(size_t) i].name + "  (" + categoryNames()[ps[(size_t) i].cat] + ")", i + 2);
            cb.setSelectedId (proc.morphCorner (c) + 2, dontSendNotification);
            cb.setTooltip ("Preset placed in this ERA MORPH corner. Moving the XY pad blends the corner sounds.");
            cb.onChange = [this, c] { proc.setMorphCorner (c, boxes[(size_t) c].getSelectedId() - 2); };
            labels[(size_t) c].setText (names[c], dontSendNotification);
            labels[(size_t) c].setFont (serif (11.0f, false, 0.1f));
            addAndMakeVisible (cb); addAndMakeVisible (labels[(size_t) c]);
        }
        title.setText ("ERA MORPH CORNERS", dontSendNotification); title.setFont (serif (12.0f, false, 0.2f));
        addAndMakeVisible (title);
    }
    void resized() override
    {
        title.setBounds (0, 0, getWidth(), 18);
        const int h = (getHeight() - 20) / 4;
        for (int c = 0; c < 4; ++c)
        {
            auto r = Rectangle<int> (0, 20 + c * h, getWidth(), h);
            labels[(size_t) c].setBounds (r.removeFromTop (16));
            boxes[(size_t) c].setBounds (r.removeFromTop (24));
        }
    }
private:
    KeysKillaProcessor& proc;
    std::array<ComboBox, 4> boxes; std::array<Label, 4> labels; Label title;
};

class DiceLocks : public Component
{
public:
    DiceLocks (KeysKillaProcessor& p) : proc (p)
    {
        title.setText ("DICE LOCKS (kept when rolling)", dontSendNotification); title.setFont (serif (12.0f, false, 0.2f));
        addAndMakeVisible (title);
        for (int i = 0; i < KeysKillaProcessor::numLocks; ++i)
        {
            auto& t = toggles[(size_t) i];
            t.setButtonText (KeysKillaProcessor::lockName (i));
            t.setToggleState (proc.diceLocks[(size_t) i], dontSendNotification);
            t.onClick = [this, i] { proc.diceLocks[(size_t) i] = toggles[(size_t) i].getToggleState(); };
            t.setTooltip ("Keep this section unchanged when you roll the DICE.");
            addAndMakeVisible (t);
        }
    }
    void resized() override
    {
        title.setBounds (0, 0, getWidth(), 18);
        for (int i = 0; i < KeysKillaProcessor::numLocks; ++i)
            toggles[(size_t) i].setBounds ((i % 2) * getWidth() / 2, 22 + (i / 2) * 28, getWidth() / 2, 26);
    }
private:
    KeysKillaProcessor& proc;
    std::array<ToggleButton, KeysKillaProcessor::numLocks> toggles; Label title;
};

//==============================================================================
class SettingsView : public Component
{
public:
    std::function<void (int)> onSkin, onSize;
    SettingsView (KeysKillaProcessor& p, int skinIndex) : proc (p)
    {
        eco.setButtonText ("ECO MODE: fewer unison voices, no oversampling, slower modulation (saves CPU)");
        eco.setColour (ToggleButton::textColourId, TC (0xffe6e0dc));
        eco.setToggleState (proc.eco.load(), dontSendNotification);
        eco.onClick = [this] { proc.eco = eco.getToggleState(); };
        skin.addItem ("CHROME (light)", 1); skin.addItem ("BLOOD (dark)", 2);
        skin.setSelectedId (skinIndex + 1, dontSendNotification);
        skin.onChange = [this] { if (onSkin) onSkin (skin.getSelectedId() - 1); };
        for (int pct : { 50, 60, 70, 85, 100 }) size.addItem (String (pct) + " %", pct);
        size.setSelectedId (openSettings()->getIntValue ("labScale18", 85), dontSendNotification);
        size.onChange = [this] { if (onSize) onSize (size.getSelectedId()); };
        info.setText (String ("BREED LAB ") + KK_VERSION_STRING + "\nDon't browse sounds. Breed them. All sounds are generated by synthesis - no samples.\n"
                      "User presets: " + KeysKillaProcessor::userPresetDir().getFullPathName(), dontSendNotification);
        info.setFont (serif (19.0f, false, 0.03f));
        info.setJustificationType (Justification::topLeft);
        for (auto* l : { &skinL, &sizeL }) l->setFont (serif (16.0f, false, 0.2f));
        skinL.setText ("DEFAULT SKIN (all instances)", dontSendNotification); sizeL.setText ("WINDOW SIZE", dontSendNotification);
        for (Component* c : { (Component*) &eco, (Component*) &size, (Component*) &info, (Component*) &sizeL })
            addAndMakeVisible (c);
        ignoreUnused (skinIndex);
    }
    void resized() override
    {
        auto r = getLocalBounds().reduced (20);
        eco.setBounds (r.removeFromTop (30)); r.removeFromTop (16);
        sizeL.setBounds (r.removeFromTop (24)); size.setBounds (r.removeFromTop (34).withWidth (260)); r.removeFromTop (24);
        info.setBounds (r.removeFromTop (130));
    }
private:
    KeysKillaProcessor& proc;
    ToggleButton eco; ComboBox skin, size; Label info, skinL, sizeL;
};

//==============================================================================
class AdvancedPage : public Component
{
public:
    std::function<void (int)> onSkin, onSize;

    AdvancedPage (KeysKillaProcessor& p, KKLookAndFeel& l, int skinIndex) : proc (p), lnf (l)
    {
        using namespace ID;
        auto page = [this] (const String& name, StringArray ids, std::vector<Component*> views, int layout)
        {
            auto* pg = new TabPage (proc, ids, std::move (views), layout);
            tabs.addTab (name, Colours::transparentBlack, pg, true);
        };
        page ("ENGINE A", { engine, octave, semi, fine, wave, unison, detune, fmRatio, fmRatio2, fmAmt, fmAlgo, warpMode, levelA, sub },
              { new WaveView (proc, lnf, 0) }, 0);
        page ("ENGINE B", { layerB, engineB, octaveB, semiB, fineB, waveB, unisonB, detuneB, fmRatioB, fmRatio2B, fmAmtB, fmAlgoB, warpModeB, levelB },
              { new WaveView (proc, lnf, 1) }, 0);
        page ("FILTER", { filterType, cutoff, reso, keyTrack, fenv, fattack, fdecay, fsustain, frelease, attack, decay, sustain, release, velSens },
              { new EnvView (proc, lnf, "AMP ENVELOPE (ENV 1)", attack, decay, sustain, release),
                new EnvView (proc, lnf, "FILTER ENVELOPE (ENV 2)", fattack, fdecay, fsustain, frelease) }, 0);
        page ("MOD", { lfoRate, lfoSync, lfoDiv, lfoShape, lfoPitch, lfoFilter, lfoAmp, wobTarget, lfo2Rate, lfo2Sync, lfo2Div, lfo2Shape,
                       e3attack, e3decay, e3sustain, e3release },
              { new ModMatrixView (proc), new LfoView (proc, lnf, "LFO 1", lfoShape), new LfoView (proc, lnf, "LFO 2", lfo2Shape),
                new EnvView (proc, lnf, "MOD ENVELOPE (ENV 3)", e3attack, e3decay, e3sustain, e3release) }, 1);
        page ("FX", { punch, halftime, timeM, drive, driveType, crush, wow, chorus, phaser, flanger, eqLow, eqHigh, reverse, delayMix, delayTime, delayFb, delayMode,
                      revMix, revSize, revType, freeze, width, gain, master, world, worldAmt, gate, gateDepth, clipMode, clipDrive },
              { new FxOrderList (proc, lnf) }, 2);
        page ("KILLA", { ghost, ghostOct, ghostRev, ghostBlur, bend, bendMode, bendSemis, tape, circuit, circRate, body, bodyMix,
                         future, alive, drift, seed }, {}, 0);
        page ("PLAY", { mono, legato, glide, bendRange, bassMode, keyLock, key, scale, chord, chordType, strum }, {}, 0);
        auto* settings = new SettingsView (proc, skinIndex);
        settings->onSkin = [this] (int s) { if (onSkin) onSkin (s); };
        settings->onSize = [this] (int s) { if (onSize) onSize (s); };
        tabs.addTab ("SETTINGS", Colours::transparentBlack, settings, true);
        tabs.setTabBarDepth (44);
        tabs.setOutline (0);
        addAndMakeVisible (tabs);
        closeBtn.setButtonText ("CLOSE");
        closeBtn.onClick = [this] { setVisible (false); };
        addAndMakeVisible (closeBtn);
        resetBtn.setButtonText ("RESET");
        resetBtn.setTooltip ("RESET: put the controls of this tab back to how the sound was loaded");
        resetBtn.onClick = [this] { if (auto* tp = dynamic_cast<TabPage*> (tabs.getCurrentContentComponent())) proc.resetParams (tp->ids); };
        addAndMakeVisible (resetBtn);
    }
    void showTab (int i) { tabs.setCurrentTabIndex (i); }
    void paint (Graphics& g) override
    {
        g.fillAll ((TC (0xf5080606)));
        drawPanel (g, getLocalBounds().toFloat().reduced (10), *lnf.skin, "ADVANCED");
    }
    void resized() override
    {
        closeBtn.setBounds (getWidth() - 140, 14, 116, 36);
        resetBtn.setBounds (getWidth() - 268, 14, 116, 36);
        tabs.setBounds (getLocalBounds().reduced (20).withTrimmedTop (34));
        for (int i = 0; i < tabs.getNumTabs(); ++i)
            tabs.getTabbedButtonBar().getTabButton (i)->setColour (TextButton::textColourOffId, lnf.skin->text);
    }
private:
    // one tab: views on top (layout depends on the tab), parameter grid below
    class TabPage : public Component
    {
    public:
        TabPage (KeysKillaProcessor& p, const StringArray& idList, std::vector<Component*> v, int layoutType)
            : ids (idList), grid (p, idList, 10), layout (layoutType)
        {
            for (auto* c : v) { views.emplace_back (c); addAndMakeVisible (c); }
            addAndMakeVisible (grid);
        }
        void resized() override
        {
            auto r = getLocalBounds().reduced (8);
            switch (layout)
            {
                case 1:   // MOD: matrix left, visuals right, grid below
                {
                    auto top = r.removeFromTop (250);
                    views[0]->setBounds (top.removeFromLeft (560));
                    top.removeFromLeft (12);
                    const int h = top.getHeight() / 3;
                    for (int i = 1; i < 4; ++i) views[(size_t) i]->setBounds (top.removeFromTop (h).reduced (0, 3));
                    r.removeFromTop (8);
                    grid.setBounds (r);
                    break;
                }
                case 2:   // FX: order list left, grid right
                    views[0]->setBounds (r.removeFromLeft (220));
                    r.removeFromLeft (12);
                    grid.setBounds (r);
                    break;
                case 3:   // EXCLUSIVE: grid top, corners + locks below
                {
                    grid.setBounds (r.removeFromTop (210));
                    r.removeFromTop (10);
                    views[0]->setBounds (r.removeFromLeft (520));
                    r.removeFromLeft (30);
                    views[1]->setBounds (r.removeFromLeft (360).removeFromTop (120));
                    break;
                }
                default:
                {
                    if (! views.empty())
                    {
                        auto top = r.removeFromTop (150);
                        const int w = top.getWidth() / (int) views.size();
                        for (auto& v : views) v->setBounds (top.removeFromLeft (w).reduced (4, 0));
                        r.removeFromTop (12);
                    }
                    grid.setBounds (r.withHeight (jmin (r.getHeight(), grid.rowsNeeded() * 124)));
                    break;
                }
            }
        }
        StringArray ids;
    private:
        std::vector<std::unique_ptr<Component>> views;
        ParamGrid grid;
        int layout;
    };

    KeysKillaProcessor& proc;
    KKLookAndFeel& lnf;
    TabbedComponent tabs { TabbedButtonBar::TabsAtTop };
    TextButton closeBtn, resetBtn;
};
