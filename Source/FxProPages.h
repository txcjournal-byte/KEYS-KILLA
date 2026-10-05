// v0.42 EVOLVE FX PRO pages (included by FxMainPage.h): REMIX REEL, DIAL-UP, WARP DRIVE, DOODLE, FINAL BOSS
// v0.43 organic pages: LIQUID, INTENT, EROSION (v0.44: no knobs - touch the liquid, the mood map, the material)

static void proHeader (Graphics& g, Component& c, const String& title, const String& era, const String& sub, Colour col)
{
    const auto& t = kk::theme();
    pageBackdrop (g, c);
    g.setColour (col); g.setFont (kk::modern::font (30.0f, true, 0.1f));
    g.drawText (title, 24, 12, 360, 40, Justification::centredLeft);
    g.setColour (col.withAlpha (0.8f)); g.setFont (kk::modern::font (10.5f, true, 0.3f));
    g.drawText (era, 26, 50, 300, 14, Justification::centredLeft);
    g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
    g.drawText (sub, 300, 18, c.getWidth() - 320, 28, Justification::centredLeft);
}

// ---------------------------------------------------------------- REMIX REEL ----------------------------------------------------------------
class ReelPage : public Component, private Timer
{
public:
    ReelPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "REMIX REEL on / off - it re-cuts the music in time with FL", [this] { proc.reel.on = ! proc.reel.on.load(); refresh(); });
        btn (initBtn, "INIT", "Empty grid", [this] { proc.reel.clear(); repaint(); });
        btn (diceAll, "DICE ALL", "A new pattern for every row", [this] { for (int r = 0; r < kk::pro::numReelRows; ++r) proc.reel.dice (r, (uint32) Time::getMillisecondCounter() + (uint32) r * 31u); proc.reel.on = true; refresh(); });
        static const char* pr[] { "RE-CUT", "BUILD-UP", "BRAKE", "GLITCH POP", "CHOP" };
        for (int i = 0; i < 5; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, pr[i]); b->framed = true; b->setTooltip ("A ready pattern: " + String (pr[i]));
            b->onClick = [this, i] { proc.reel.preset (i); proc.reel.on = true; refresh(); };
            addAndMakeVisible (*b); presets.push_back (std::move (b));
        }
        mix = std::make_unique<RackKnob> (proc.reel.mix, "MIX", 0, 1, 1, Colour (0xffff4fd8), Colour (0xff22d3ee));
        addAndMakeVisible (*mix);
        startTimerHz (30);
        refresh();
    }
    static Colour rowColour (int r) { static const uint32 c[] { 0xfff2f2f2, 0xffff3b8a, 0xffa855f7, 0xff2ee6c6, 0xffe9f542, 0xff22a8ee }; return Colour (c[std::clamp (r, 0, 5)]); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        proHeader (g, *this, "REMIX REEL", "SOCIAL ERA", "a reel of 16 steps re-cuts what plays: slices, loops, envelopes, filters, crush, stops.  click = next  right-click = clear  wheel = change", Colour (0xffff3b8a));
        const auto G = gridArea();
        g.setColour (Colour (0xff0b0d12).withAlpha (t.night ? 0.7f : 0.88f)); g.fillRoundedRectangle (G.toFloat().expanded (10), 14);
        const float cw = (float) G.getWidth() / 16.0f, rh = (float) G.getHeight() / (float) kk::pro::numReelRows;
        const int now = proc.reel.stepNow.load();
        for (int s = 0; s < 16; ++s)
        {
            g.setColour (Colours::white.withAlpha (s == now ? 0.95f : 0.5f)); g.setFont (kk::modern::font (11.0f, true, 0.0f));
            g.drawText (String (s + 1), Rectangle<float> ((float) G.getX() + cw * (float) s, (float) G.getY() - 20, cw, 16), Justification::centred);
            if (s % 4 == 0 && s > 0) { g.setColour (Colours::white.withAlpha (0.12f)); g.fillRect ((float) G.getX() + cw * (float) s - 1, (float) G.getY(), 1.0f, (float) G.getHeight()); }
        }
        if (now >= 0 && proc.reel.on.load()) { g.setColour (Colours::white.withAlpha (0.07f)); g.fillRect ((float) G.getX() + cw * (float) now, (float) G.getY(), cw, (float) G.getHeight()); }
        for (int r = 0; r < kk::pro::numReelRows; ++r)
        {
            const auto c = rowColour (r);
            const float y = (float) G.getY() + rh * (float) r;
            g.setColour (c); g.setFont (kk::modern::font (15.0f, true, 0.12f));
            g.drawText (kk::pro::reelRowName (r), Rectangle<float> (24, y, (float) G.getX() - 34, rh), Justification::centredLeft);
            for (int s = 0; s < 16; ++s)
            {
                const auto cell = Rectangle<float> ((float) G.getX() + cw * (float) s, y, cw, rh).reduced (3.0f, rh * 0.18f);
                const int v = proc.reel.grid[(size_t) r][(size_t) s].load();
                if (v == 0) { g.setColour (c.withAlpha (0.12f)); g.fillRoundedRectangle (cell.withSizeKeepingCentre (cell.getHeight() * 0.42f, cell.getHeight() * 0.42f), 3); continue; }
                const bool live = s == now && proc.reel.on.load();
                g.setColour (live ? c.brighter (0.4f) : c); g.fillRoundedRectangle (cell, 5);
                g.setColour (Colours::black.withAlpha (0.85f)); g.setFont (kk::modern::font (r == kk::pro::rrSlice ? 17.0f : 10.5f, true, 0.02f));
                g.drawFittedText (r == kk::pro::rrSlice ? String (v) : String (kk::pro::reelValueName (r, v)), cell.toNearestInt(), Justification::centred, 1, 0.6f);
            }
            // dice
            const auto d = diceRect (r).toFloat();
            g.setColour (c.withAlpha (0.25f)); g.fillRoundedRectangle (d, 5); g.setColour (c); g.drawRoundedRectangle (d, 5, 1.2f);
            for (int k = 0; k < 4; ++k) g.fillEllipse (d.getX() + d.getWidth() * (k % 2 ? 0.66f : 0.34f) - 2, d.getY() + d.getHeight() * (k / 2 ? 0.66f : 0.34f) - 2, 4, 4);
        }
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32); initBtn.setBounds (376, 56, 70, 32); diceAll.setBounds (452, 56, 100, 32);
        int x = 570; for (auto& b : presets) { b->setBounds (x, 56, 106, 32); x += 110; }
        mix->setBounds (getWidth() - 110, 6, 90, 86);
    }
    void mouseDown (const MouseEvent& e) override
    {
        for (int r = 0; r < kk::pro::numReelRows; ++r) if (diceRect (r).contains (e.getPosition())) { proc.reel.dice (r, (uint32) Time::getMillisecondCounter()); proc.reel.on = true; refresh(); return; }
        const auto c = cellAt (e.position); if (c.x < 0) return;
        auto& cell = proc.reel.grid[(size_t) c.y][(size_t) c.x];
        if (e.mods.isPopupMenu()) cell = 0;
        else cell = (cell.load() + 1) % kk::pro::reelChoices (c.y);
        proc.reel.on = true; refresh();
    }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        const auto c = cellAt (e.position); if (c.x < 0) return;
        auto& cell = proc.reel.grid[(size_t) c.y][(size_t) c.x];
        const int n = kk::pro::reelChoices (c.y);
        cell = (cell.load() + (w.deltaY > 0 ? 1 : n - 1)) % n;
        repaint();
    }
private:
    Rectangle<int> gridArea() const { return { 150, 130, getWidth() - 150 - 80, getHeight() - 170 }; }
    Rectangle<int> diceRect (int r) const { const auto G = gridArea(); const int rh = G.getHeight() / kk::pro::numReelRows; return { G.getRight() + 22, G.getY() + r * rh + rh / 2 - 15, 30, 30 }; }
    Point<int> cellAt (Point<float> p) const
    {
        const auto G = gridArea().toFloat(); if (! G.contains (p)) return { -1, -1 };
        return { jlimit (0, 15, (int) ((p.x - G.getX()) / G.getWidth() * 16.0f)), jlimit (0, kk::pro::numReelRows - 1, (int) ((p.y - G.getY()) / G.getHeight() * (float) kk::pro::numReelRows)) };
    }
    void refresh() { onBtn.selected = proc.reel.on.load(); onBtn.setButtonText (proc.reel.on.load() ? "ON" : "OFF"); onBtn.repaint(); mix->sync(); repaint(); }
    void timerCallback() override { if (! isShowing()) return; const int n = proc.reel.stepNow.load(); if (n != lastNow) { lastNow = n; repaint(); } }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf }, initBtn { lnf }, diceAll { lnf };
    std::vector<std::unique_ptr<HotButton>> presets;
    std::unique_ptr<RackKnob> mix;
    int lastNow = -2;
};

// ---------------------------------------------------------------- DIAL-UP ----------------------------------------------------------------
class DialPage : public Component, private Timer
{
public:
    DialPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "DIAL-UP on / off", [this] { proc.dial.on = ! proc.dial.on.load(); refresh(); });
        for (int m = 0; m < kk::pro::numDialModes; ++m)
        {
            auto b = std::make_unique<HotButton> (lnf, String (m + 1) + "  " + kk::pro::dialModeName (m)); b->framed = true;
            b->onClick = [this, m] { proc.dial.applyMode (m); proc.dial.on = true; for (auto& k : knobs) k->sync(); refresh(); };
            addAndMakeVisible (*b); keys.push_back (std::move (b));
        }
        const Colour a (0xff4dd2ff), b2 (0xff2563eb);
        auto k = [&] (std::atomic<float>& v, const char* n) { knobs.push_back (std::make_unique<RackKnob> (v, n, 0, 1, 0.3, a, b2)); addAndMakeVisible (*knobs.back()); };
        k (proc.dial.signal, "BAD SIGNAL"); k (proc.dial.crush, "CRUSH"); k (proc.dial.tinny, "TINNY"); k (proc.dial.robot, "ROBOT"); k (proc.dial.mix, "MIX");
        startTimerHz (20);
        refresh();
    }
    void paint (Graphics& g) override
    {
        proHeader (g, *this, "DIAL-UP", "RARE ERA", "every way an old phone ruins a sound: the line, the tiny speaker, bad signal, robot toys, walkie-talkies", Colour (0xff4dd2ff));
        // the phone
        const auto ph = phoneRect().toFloat();
        g.setGradientFill (ColourGradient (Colour (0xffb8c2cc), ph.getX(), ph.getY(), Colour (0xff5d6873), ph.getRight(), ph.getBottom(), false));
        g.fillRoundedRectangle (ph, 40);
        g.setColour (Colour (0xff4dd2ff).withAlpha (proc.dial.on.load() ? 0.7f : 0.2f)); g.drawRoundedRectangle (ph.expanded (3), 42, 3.0f);
        const auto scr = screenRect().toFloat();
        g.setColour (Colour (0xff0d1a12)); g.fillRoundedRectangle (scr, 8);
        g.setColour (Colour (0xff9cff9c)); g.setFont (Font (FontOptions (Font::getDefaultMonospacedFontName(), 22.0f, Font::bold)));
        g.drawText (kk::pro::dialModeName (proc.dial.mode.load()), scr.reduced (12, 10).removeFromTop (30), Justification::centredLeft);
        g.setFont (Font (FontOptions (Font::getDefaultMonospacedFontName(), 15.0f, Font::plain)));
        g.drawText (proc.dial.on.load() ? "ON THE LINE  " + String ((int) (phase * 2.0f) / 60) + ":" + String ((int) (phase * 2.0f) % 60).paddedLeft ('0', 2) : String ("HUNG UP"), scr.reduced (12, 10).withTrimmedTop (40).removeFromTop (20), Justification::centredLeft);
        // signal bars: they fall with BAD SIGNAL
        const float sig = proc.dial.signal.load();
        for (int b = 0; b < 5; ++b)
        {
            const bool lit = proc.dial.on.load() && (float) b < 5.0f * (1.0f - sig * (0.5f + 0.5f * std::sin (phase * 3.0f + (float) b)));
            g.setColour (Colour (0xff9cff9c).withAlpha (lit ? 0.95f : 0.15f));
            g.fillRect (scr.getRight() - 70 + (float) b * 11, scr.getY() + 34 - (float) b * 5, 7.0f, 6.0f + (float) b * 5);
        }
        // a voice wave that breaks up
        Path wv; const auto wr = scr.reduced (12, 10).withTrimmedTop (70);
        for (int i = 0; i <= 80; ++i)
        {
            const float u = (float) i / 80.0f; float y = std::sin (u * 30.0f + phase * 8.0f) * std::sin (u * 3.0f + phase);
            if (sig > 0.3f && std::fmod (u * 7.0f + phase, 1.0f) < sig * 0.4f) y = 0;
            y = std::round (y * (16.0f - 12.0f * proc.dial.crush.load())) / (16.0f - 12.0f * proc.dial.crush.load());
            const float yy = wr.getCentreY() - y * wr.getHeight() * 0.4f;
            if (i == 0) wv.startNewSubPath (wr.getX() + wr.getWidth() * u, yy); else wv.lineTo (wr.getX() + wr.getWidth() * u, yy);
        }
        g.setColour (Colour (0xff9cff9c)); g.strokePath (wv, PathStrokeType (1.6f));
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32);
        const auto ph = phoneRect();
        const int kw = (ph.getWidth() - 60) / 2;
        for (int i = 0; i < (int) keys.size(); ++i) keys[(size_t) i]->setBounds (ph.getX() + 24 + (i % 2) * (kw + 12), screenRect().getBottom() + 26 + (i / 2) * 50, kw, 42);
        const int kx = ph.getRight() + 60, kwid = (getWidth() - kx - 30) / 5;
        for (int i = 0; i < (int) knobs.size(); ++i) knobs[(size_t) i]->setBounds (kx + i * kwid, getHeight() / 2 - 70, kwid - 6, 120);
    }
private:
    Rectangle<int> phoneRect() const { return { 120, 110, 400, getHeight() - 140 }; }
    Rectangle<int> screenRect() const { const auto p = phoneRect(); return { p.getX() + 34, p.getY() + 70, p.getWidth() - 68, 190 }; }
    void refresh()
    {
        onBtn.selected = proc.dial.on.load(); onBtn.setButtonText (proc.dial.on.load() ? "ON" : "OFF"); onBtn.repaint();
        for (int i = 0; i < (int) keys.size(); ++i) { keys[(size_t) i]->selected = proc.dial.mode.load() == i; keys[(size_t) i]->repaint(); }
        repaint();
    }
    void timerCallback() override { if (! isShowing()) return; phase += 0.05f; repaint (screenRect().expanded (4)); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> keys;
    std::vector<std::unique_ptr<RackKnob>> knobs;
    float phase = 0;
};

// ---------------------------------------------------------------- WARP DRIVE ----------------------------------------------------------------
class WarpPage : public Component, private Timer
{
public:
    WarpPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "WARP DRIVE on / off", [this] { proc.warp.on = ! proc.warp.on.load(); refresh(); });
        for (int m = 0; m < kk::pro::numWarpModes; ++m)
        {
            auto b = std::make_unique<HotButton> (lnf, kk::pro::warpModeName (m)); b->framed = true;
            b->onClick = [this, m] { proc.warp.applyMode (m); proc.warp.on = true; for (auto& k : knobs) k->sync(); refresh(); };
            addAndMakeVisible (*b); modes.push_back (std::move (b));
        }
        const Colour a (0xffb04dff), b2 (0xff22d3ee);
        auto k = [&] (std::atomic<float>& v, const char* n, double lo, double hi, double def, std::function<String (double)> txt)
        { knobs.push_back (std::make_unique<RackKnob> (v, n, lo, hi, def, a, b2)); knobs.back()->valueText = std::move (txt); addAndMakeVisible (*knobs.back()); };
        k (proc.warp.semis, "PITCH", -24, 24, 0, [] (double v) { return (v > 0 ? "+" : "") + String (v, 1) + " st"; });
        k (proc.warp.shiftHz, "SHIFT", -500, 500, 0, [] (double v) { return String (roundToInt (v)) + " Hz"; });
        k (proc.warp.wobble, "WOBBLE", 0, 1, 0, [] (double v) { return String (roundToInt (v * 100)) + " %"; });
        k (proc.warp.mix, "MIX", 0, 1, 0.5, [] (double v) { return String (roundToInt (v * 100)) + " %"; });
        startTimerHz (30);
        refresh();
    }
    void paint (Graphics& g) override
    {
        proHeader (g, *this, "WARP DRIVE", "FUTURE ERA", "bend space and pitch: octaves, chipmunks, demons, alien frequency shifts, wobble", Colour (0xffb04dff));
        const auto r = tunnel().toFloat();
        g.setColour (Colour (0xff05030c)); g.fillRoundedRectangle (r, 16);
        Graphics::ScopedSaveState ss (g); g.reduceClipRegion (r.toNearestInt());
        const float speed = 0.2f + std::abs (proc.warp.semis.load()) / 12.0f + std::abs (proc.warp.shiftHz.load()) / 300.0f;
        const auto c = r.getCentre() + Point<float> (std::sin (phase * 0.7f) * 30.0f * proc.warp.wobble.load(), std::cos (phase * 0.9f) * 20.0f * proc.warp.wobble.load());
        for (int i = 0; i < 18; ++i)
        {
            const float u = std::fmod ((float) i / 18.0f + phase * speed * 0.1f, 1.0f);
            const float rad = 10.0f + u * u * r.getWidth() * 0.8f;
            g.setColour (Colour (0xffb04dff).interpolatedWith (Colour (0xff22d3ee), u).withAlpha ((proc.warp.on.load() ? 0.8f : 0.25f) * u));
            g.drawEllipse (c.x - rad, c.y - rad * 0.6f, rad * 2, rad * 1.2f, 1.5f + 2.5f * u);
        }
        Random rr (5);
        for (int i = 0; i < 80; ++i)
        {
            const float ang = rr.nextFloat() * MathConstants<float>::twoPi, u = std::fmod (rr.nextFloat() + phase * speed * 0.15f, 1.0f);
            const float d = u * u * r.getWidth() * 0.6f;
            g.setColour (Colours::white.withAlpha (u)); g.fillEllipse (c.x + std::cos (ang) * d, c.y + std::sin (ang) * d * 0.6f, 2.0f + 2.0f * u, 2.0f + 2.0f * u);
        }
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32);
        const auto t = tunnel();
        const int mw = t.getWidth() / (int) modes.size();
        for (int i = 0; i < (int) modes.size(); ++i) modes[(size_t) i]->setBounds (t.getX() + i * mw, t.getBottom() + 14, mw - 6, 36);
        const int kx = t.getRight() + 30, kh = (getHeight() - 140) / 4;
        for (int i = 0; i < (int) knobs.size(); ++i) knobs[(size_t) i]->setBounds (kx, 100 + i * kh, getWidth() - kx - 30, kh - 8);
    }
private:
    Rectangle<int> tunnel() const { return { 24, 100, getWidth() - 24 - 260, getHeight() - 180 }; }
    void refresh()
    {
        onBtn.selected = proc.warp.on.load(); onBtn.setButtonText (proc.warp.on.load() ? "ON" : "OFF"); onBtn.repaint();
        for (int i = 0; i < (int) modes.size(); ++i) { modes[(size_t) i]->selected = proc.warp.mode.load() == i; modes[(size_t) i]->repaint(); }
        repaint();
    }
    void timerCallback() override { if (! isShowing()) return; phase += 0.04f; repaint (tunnel()); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> modes;
    std::vector<std::unique_ptr<RackKnob>> knobs;
    float phase = 0;
};

// ---------------------------------------------------------------- FINAL BOSS ----------------------------------------------------------------
class BossPage : public Component, private Timer
{
public:
    BossPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "FINAL BOSS on / off: the last stage on the master - loudness + PEAK SAFE (never over the ceiling)", [this] { proc.boss.on = ! proc.boss.on.load(); refresh(); });
        btn (resetBtn, "RESET", "Start measuring the whole-song loudness again", [this] { proc.boss.resetIntegrated = true; });
        for (int t = 0; t < kk::pro::numBossTargets; ++t)
        {
            auto b = std::make_unique<HotButton> (lnf, kk::pro::bossTargetName (t)); b->framed = true;
            b->setTooltip (t == 0 ? String ("Only measure") : "AUTO: the level walks slowly to " + String (kk::pro::bossTargetLufs (t), 0) + " LUFS (short-term)");
            b->onClick = [this, t] { proc.boss.target = t; if (t > 0) proc.boss.on = true; refresh(); };
            addAndMakeVisible (*b); targets.push_back (std::move (b));
        }
        drive = std::make_unique<RackKnob> (proc.boss.drive, "DRIVE", -12, 18, 0, Colour (0xffff3b5c), Colour (0xffffd23f)); drive->valueText = [] (double v) { return (v > 0 ? "+" : "") + String (v, 1) + " dB"; };
        ceil = std::make_unique<RackKnob> (proc.boss.ceiling, "CEILING", -6, 0, -1, Colour (0xffff3b5c), Colour (0xffffd23f)); ceil->valueText = [] (double v) { return String (v, 1) + " dB"; };
        addAndMakeVisible (*drive); addAndMakeVisible (*ceil);
        startTimerHz (20);
        refresh();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        proHeader (g, *this, "FINAL BOSS", "RARE ERA - THE LAST LEVEL", "the master's final stage: loudness you can trust (LUFS), a level that walks to your target, PEAK SAFE never passes the ceiling", Colour (0xffff3b5c));
        auto bar = [&] (Rectangle<float> r, const String& name, float lufs, float target, Colour c)
        {
            g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (r, 8);
            const float u = jlimit (0.0f, 1.0f, (lufs + 40.0f) / 40.0f);
            g.setGradientFill (ColourGradient (c.darker (0.4f), r.getX(), 0, c.brighter (0.3f), r.getRight(), 0, false));
            g.fillRoundedRectangle (r.withWidth (r.getWidth() * u).reduced (2), 6);
            for (int k = 1; k < 20; ++k) { g.setColour (Colours::black.withAlpha (0.35f)); g.fillRect (r.getX() + r.getWidth() * (float) k / 20.0f, r.getY(), 2.0f, r.getHeight()); }
            if (target > -60.0f) { const float tx = r.getX() + r.getWidth() * jlimit (0.0f, 1.0f, (target + 40.0f) / 40.0f); g.setColour (Colours::white); g.fillRect (tx - 1.5f, r.getY() - 6, 3.0f, r.getHeight() + 12); }
            g.setColour (t.text); g.setFont (kk::modern::font (13.0f, true, 0.2f));
            g.drawText (name, (int) r.getX(), (int) r.getY() - 22, 300, 18, Justification::centredLeft);
            g.drawText (lufs > -69.0f ? String (lufs, 1) + " LUFS" : String ("-- LUFS"), (int) r.getRight() - 160, (int) r.getY() - 22, 160, 18, Justification::centredRight);
        };
        const auto A = meterArea().toFloat();
        const float tgt = proc.boss.target.load() > 0 ? kk::pro::bossTargetLufs (proc.boss.target.load()) : -99.0f;
        bar (A.withHeight (46), "SHORT-TERM (the boss's health)", proc.boss.mShort.load(), tgt, Colour (0xffff3b5c));
        bar (A.withHeight (30).translated (0, 96), "MOMENTARY", proc.boss.mMomentary.load(), tgt, Colour (0xffff8a3d));
        g.setColour (t.text); g.setFont (kk::modern::font (40.0f, true, 0.0f));
        const auto nums = A.withTrimmedTop (170);
        auto num = [&] (int col, const String& big, const String& small)
        {
            const auto c = nums.withWidth (nums.getWidth() / 4.0f).translated (nums.getWidth() / 4.0f * (float) col, 0);
            g.setColour (t.text); g.setFont (kk::modern::font (36.0f, true, 0.0f)); g.drawText (big, c.withHeight (46), Justification::centredLeft);
            g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.2f)); g.drawText (small, c.withTrimmedTop (48).withHeight (18), Justification::centredLeft);
        };
        const float integ = proc.boss.mIntegrated.load(), pk = proc.boss.mPeak.load();
        num (0, integ > -69.0f ? String (integ, 1) : String ("--"), "WHOLE SONG LUFS");
        num (1, pk > -69.0f ? String (pk, 1) : String ("--"), "PEAK dB");
        num (2, String (proc.boss.mGr.load(), 1), "PEAK SAFE dB");
        num (3, (proc.boss.mAuto.load() > 0 ? "+" : "") + String (proc.boss.mAuto.load(), 1), "AUTO dB");
        // how to beat the boss
        const auto how = Rectangle<float> (A.getX(), A.getBottom() + 90, A.getWidth(), std::min (150.0f, (float) getHeight() - A.getBottom() - 110));
        if (how.getHeight() > 60)
        {
            kk::modern::well (g, how, 12.0f);
            g.setColour (Colour (0xffff3b5c)); g.setFont (kk::modern::font (14.0f, true, 0.25f));
            g.drawText ("HOW TO BEAT THE BOSS", how.reduced (22, 16).removeFromTop (20), Justification::centredLeft);
            g.setColour (t.text); g.setFont (kk::modern::font (15.0f, true, 0.03f));
            g.drawFittedText ("1.  put EVOLVE FX PRO as the LAST plugin on the MASTER\n"
                              "2.  pick where the song goes: STREAMING, SOUNDCLOUD or CLUB - the white line is the goal\n"
                              "3.  play the loudest part - AUTO walks the level to the goal, PEAK SAFE stops every peak at the CEILING\n"
                              "4.  RESET, then play the whole song once - WHOLE SONG LUFS is the number the platforms see",
                              how.reduced (22, 16).withTrimmedTop (30).toNearestInt(), Justification::topLeft, 6);
        }
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32); resetBtn.setBounds (376, 56, 80, 32);
        const auto A = meterArea();
        const int tw = A.getWidth() / (int) targets.size();
        for (int i = 0; i < (int) targets.size(); ++i) targets[(size_t) i]->setBounds (A.getX() + i * tw, A.getBottom() + 20, tw - 8, 40);
        drive->setBounds (A.getRight() + 30, A.getY(), 120, 130); ceil->setBounds (A.getRight() + 30, A.getY() + 150, 120, 130);
    }
private:
    Rectangle<int> meterArea() const { return { 40, 140, getWidth() - 40 - 200, 260 }; }
    void refresh()
    {
        onBtn.selected = proc.boss.on.load(); onBtn.setButtonText (proc.boss.on.load() ? "ON" : "OFF"); onBtn.repaint();
        for (int i = 0; i < (int) targets.size(); ++i) { targets[(size_t) i]->selected = proc.boss.target.load() == i; targets[(size_t) i]->repaint(); }
        drive->sync(); ceil->sync(); repaint();
    }
    void timerCallback() override { if (isShowing()) repaint(); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf }, resetBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> targets;
    std::unique_ptr<RackKnob> drive, ceil;
};

// ---------------------------------------------------------------- DOODLE ----------------------------------------------------------------
class DoodlePage : public Component
{
public:
    DoodlePage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        static const char* pens[] { "1/16", "1/8", "LONG", "CHORDS", "ERASE" };
        static const char* tips[] { "quick notes along the line", "eighth notes", "long notes", "chords along the line", "erase strokes" };
        for (int i = 0; i < 5; ++i)
        {
            auto b = std::make_unique<HotButton> (lnf, pens[i]); b->framed = true; b->setTooltip (String ("Pen: ") + tips[i]);
            b->onClick = [this, i] { pen = i; refresh(); };
            addAndMakeVisible (*b); penBtns.push_back (std::move (b));
        }
        for (int k = 0; k < 12; ++k) keyBox.addItem (kk::mel::keyName (k), k + 1);
        keyBox.setSelectedId (1, dontSendNotification); keyBox.onChange = [this] { rebuild(); }; addAndMakeVisible (keyBox);
        for (int sc = 0; sc < kk::mel::numScales; ++sc) scaleBox.addItem (kk::mel::scaleName (sc), sc + 1);
        scaleBox.setSelectedId (1, dontSendNotification); scaleBox.onChange = [this] { rebuild(); }; addAndMakeVisible (scaleBox);
        btn (barsBtn, "4 BARS", "2 or 4 bars", [this] { bars = bars == 4 ? 2 : 4; barsBtn.setButtonText (String (bars) + " BARS"); rebuild(); });
        btn (randomBtn, "RANDOM DOODLE", "A random shape - a random melody", [this] { randomDoodle(); });
        btn (clearBtn, "CLEAR", "An empty page", [this] { strokes.clear(); rebuild(); });
        btn (playBtn, "PLAY", "Hear it (a preview sound) - again = stop", [this] { if (proc.loopPlaying()) proc.stopLoop(); else play(); });
        randomBtn.hero = true;
        dragMidi.makeFile = [this] { return exportMidi(); };
        dragMidi.setTooltip ("Your doodle as MIDI - drop it on any instrument in FL");
        addAndMakeVisible (dragMidi);
        refresh();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        proHeader (g, *this, "DOODLE", "SOCIAL ERA", "draw lines, curves, zig-zags - they become a melody in your key.  higher = higher notes, pens = note lengths", Colour (0xffffd23f));
        const auto c = canvas().toFloat();
        g.setColour (Colour (0xfffdfaf2)); g.fillRoundedRectangle (c.expanded (8), 12);
        const int rows = 22, cols = bars * 16;
        for (int r = 0; r <= rows; ++r)
        {
            const bool root = (rows - r) % (int) kk::mel::scaleSteps (scale()).size() == 0;
            g.setColour (Colour (0xff1d2430).withAlpha (root ? 0.22f : 0.07f)); g.fillRect (c.getX(), c.getY() + c.getHeight() * (float) r / rows, c.getWidth(), root ? 1.5f : 1.0f);
        }
        for (int s = 0; s <= cols; ++s) { g.setColour (Colour (0xff1d2430).withAlpha (s % 16 == 0 ? 0.3f : s % 4 == 0 ? 0.12f : 0.04f)); g.fillRect (c.getX() + c.getWidth() * (float) s / cols, c.getY(), 1.0f, c.getHeight()); }
        // the notes it makes
        for (auto& n : melody.notes)
        {
            const float x = c.getX() + c.getWidth() * n.start / melody.beats(), w = std::max (3.0f, c.getWidth() * n.len / melody.beats() - 1.0f);
            const int d = kk::mel::pitchToDegree (n.pitch, melody.key, melody.scale, 0.5f) + 7;
            const float y = c.getBottom() - c.getHeight() * (float) (d + 1) / rows;
            g.setColour (Colour (0xff1d2430).withAlpha (0.18f)); g.fillRoundedRectangle (x, y, w, c.getHeight() / rows - 1.0f, 2.0f);
        }
        // the strokes
        for (auto& st : strokes)
        {
            if (st.pts.size() < 2) continue;
            Path p; p.startNewSubPath (toPx (st.pts[0])); for (size_t i = 1; i < st.pts.size(); ++i) p.lineTo (toPx (st.pts[i]));
            const auto col = penColour (st.pen);
            g.setColour (col.withAlpha (0.25f)); g.strokePath (p, PathStrokeType (10.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (col); g.strokePath (p, PathStrokeType (3.5f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.04f));
        g.drawText (String ((int) melody.notes.size()) + " notes  .  " + kk::mel::keyName (melody.key) + " " + kk::mel::scaleName (melody.scale), (int) c.getX(), (int) c.getBottom() + 10, 400, 18, Justification::centredLeft);
    }
    void resized() override
    {
        int x = 300;
        for (auto& b : penBtns) { b->setBounds (x, 56, 80, 32); x += 84; }
        x += 10; keyBox.setBounds (x, 58, 64, 28); scaleBox.setBounds (x + 70, 58, 170, 28); barsBtn.setBounds (x + 246, 56, 80, 32);
        const auto c = canvas();
        randomBtn.setBounds (c.getRight() - 470, c.getBottom() + 14, 160, 38); clearBtn.setBounds (c.getRight() - 304, c.getBottom() + 14, 80, 38);
        playBtn.setBounds (c.getRight() - 218, c.getBottom() + 14, 80, 38); dragMidi.setBounds (c.getRight() - 132, c.getBottom() + 12, 132, 42);
    }
    void mouseDown (const MouseEvent& e) override
    {
        if (! canvas().toFloat().contains (e.position)) return;
        if (pen == 4) { erase (e.position); return; }
        strokes.push_back ({ pen, { toNorm (e.position) } });
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (pen == 4) { erase (e.position); return; }
        if (strokes.empty() || ! canvas().toFloat().expanded (20).contains (e.position)) return;
        strokes.back().pts.push_back (toNorm (e.position));
        repaint();
    }
    void mouseUp (const MouseEvent&) override { rebuild(); }
private:
    struct Stroke { int pen; std::vector<Point<float>> pts; };
    Rectangle<int> canvas() const { return { 40, 120, getWidth() - 80, getHeight() - 200 }; }
    int scale() const { return jmax (0, scaleBox.getSelectedId() - 1); }
    static Colour penColour (int p) { static const uint32 c[] { 0xffff8a3d, 0xff2563eb, 0xff16a34a, 0xffdb2777, 0xff888888 }; return Colour (c[jlimit (0, 4, p)]); }
    Point<float> toNorm (Point<float> p) const { const auto c = canvas().toFloat(); return { jlimit (0.0f, 1.0f, (p.x - c.getX()) / c.getWidth()), jlimit (0.0f, 1.0f, (p.y - c.getY()) / c.getHeight()) }; }
    Point<float> toPx (Point<float> n) const { const auto c = canvas().toFloat(); return { c.getX() + n.x * c.getWidth(), c.getY() + n.y * c.getHeight() }; }
    void erase (Point<float> p)
    {
        const auto n = toNorm (p);
        strokes.erase (std::remove_if (strokes.begin(), strokes.end(), [&] (const Stroke& s) { for (auto& q : s.pts) if (q.getDistanceFrom (n) < 0.03f) return true; return false; }), strokes.end());
        rebuild();
    }
    void randomDoodle()
    {
        Random r; Stroke s; s.pen = r.nextInt (3);
        const int kind = r.nextInt (3); const float f = 1.0f + r.nextFloat() * 5.0f, ph = r.nextFloat() * 6.0f;
        for (int i = 0; i <= 120; ++i)
        {
            const float u = (float) i / 120.0f;
            float y = kind == 0 ? 0.5f + 0.3f * std::sin (u * f * 6.28f + ph) : kind == 1 ? 0.5f + 0.35f * (std::fmod (u * f, 1.0f) < 0.5f ? std::fmod (u * f, 1.0f) * 2 - 0.5f : 1.5f - std::fmod (u * f, 1.0f) * 2) : 0.5f + 0.3f * std::sin (u * 9.0f + ph) * std::cos (u * f * 3.0f);
            s.pts.push_back ({ u, jlimit (0.05f, 0.95f, y) });
        }
        strokes.push_back (s);
        rebuild();
    }
    void rebuild()
    {
        melody = {}; melody.bars = bars; melody.key = jmax (0, keyBox.getSelectedId() - 1); melody.scale = scale(); melody.name = "Doodle";
        const int cols = bars * 16, rows = 22;
        for (auto& st : strokes)
        {
            if (st.pts.size() < 2) continue;
            const int every = st.pen == 0 ? 1 : st.pen == 1 ? 2 : st.pen == 2 ? 8 : 4;
            float lo = 1, hi = 0; for (auto& q : st.pts) { lo = std::min (lo, q.x); hi = std::max (hi, q.x); }
            for (int col = 0; col < cols; col += every)
            {
                const float x = ((float) col + 0.5f) / (float) cols;
                if (x < lo || x > hi) continue;
                // the line's height here (nearest point in x)
                float best = 2, y = 0.5f; for (auto& q : st.pts) if (std::abs (q.x - x) < best) { best = std::abs (q.x - x); y = q.y; }
                if (best > 1.5f / (float) cols + 0.01f) continue;
                const int deg = (int) std::round ((1.0f - y) * (float) (rows - 1)) - 7;
                kk::mel::Note n; n.start = (float) col * 0.25f; n.len = (float) every * 0.25f * 0.92f; n.vel = 0.85f;
                n.pitch = kk::mel::degreeToPitch (deg, melody.key, melody.scale, 0.5f);
                melody.notes.push_back (n);
                if (st.pen == 3) for (int add : { 2, 4 }) { auto c2 = n; c2.pitch = kk::mel::degreeToPitch (deg - add, melody.key, melody.scale, 0.5f); c2.vel = 0.7f; melody.notes.push_back (c2); }
            }
        }
        std::sort (melody.notes.begin(), melody.notes.end(), [] (auto& a, auto& b) { return a.start < b.start || (a.start == b.start && a.pitch < b.pitch); });
        melody.notes.erase (std::unique (melody.notes.begin(), melody.notes.end(), [] (auto& a, auto& b) { return a.start == b.start && a.pitch == b.pitch; }), melody.notes.end());
        // repeated notes of a 1/16 line join into longer ones
        std::vector<kk::mel::Note> joined;
        for (auto& n : melody.notes)
        {
            bool merged = false;
            for (auto it = joined.rbegin(); it != joined.rend() && it->start >= n.start - 4.0f; ++it)
                if (it->pitch == n.pitch && std::abs (it->start + it->len / 0.92f - n.start) < 0.01f) { it->len = (n.start + n.len) - it->start; merged = true; break; }
            if (! merged) joined.push_back (n);
        }
        melody.notes = joined;
        if (proc.loopPlaying()) play();
        repaint();
    }
    void play()
    {
        if (melody.notes.empty()) return;
        std::vector<kk::LoopNote> ln; for (auto& n : melody.notes) ln.push_back ({ n.start, n.len, n.pitch, false });
        proc.playCustomLoop (ln, melody.beats());
    }
    File exportMidi()
    {
        if (melody.notes.empty()) return {};
        const double bpm = proc.lastBpm.load() > 30 ? proc.lastBpm.load() : 140.0;
        auto mf = kk::mel::toMidiFile (melody, bpm);
        auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("EVOLVE Doodles"); dir.createDirectory();
        auto f = dir.getChildFile ("Doodle - " + String (kk::mel::keyName (melody.key)) + " " + kk::mel::scaleName (melody.scale) + " - " + String (roundToInt (bpm)) + "BPM.mid");
        f.deleteFile(); if (FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
        return f;
    }
    void refresh() { for (int i = 0; i < (int) penBtns.size(); ++i) { penBtns[(size_t) i]->selected = pen == i; penBtns[(size_t) i]->repaint(); } repaint(); }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    std::vector<std::unique_ptr<HotButton>> penBtns;
    ComboBox keyBox, scaleBox;
    HotButton barsBtn { lnf }, randomBtn { lnf }, clearBtn { lnf }, playBtn { lnf };
    DragFileButton dragMidi { "DRAG MIDI", TC (0xff36ff6a) };
    std::vector<Stroke> strokes;
    kk::mel::Melody melody;
    int pen = 0, bars = 4;
};

// v0.44: the organic pages have no knobs - you touch the liquid, the map, the material.
// one row of a "touch it" card: a glyph, what to do, what it changes, and how much (a bar, no numbers)
static void gestureRow (Graphics& g, Rectangle<float> r, int glyph, const String& title, const String& sub, float amount, Colour col)
{
    const auto& t = kk::theme();
    const auto gr = r.removeFromLeft (r.getHeight()).reduced (6);
    g.setColour (col.withAlpha (0.14f)); g.fillEllipse (gr);
    g.setColour (col.withAlpha (0.8f)); g.drawEllipse (gr, 1.4f);
    const auto c = gr.getCentre(); const float s = gr.getWidth() * 0.28f;
    Path p;
    switch (glyph)
    {
        case 0: p.startNewSubPath (c.x, c.y - s * 1.2f); p.lineTo (c.x, c.y + s * 0.9f); p.startNewSubPath (c.x - s * 0.6f, c.y + s * 0.3f); p.lineTo (c.x, c.y + s * 0.9f); p.lineTo (c.x + s * 0.6f, c.y + s * 0.3f); break;   // down
        case 1: p.startNewSubPath (c.x, c.y + s * 1.2f); p.lineTo (c.x, c.y - s * 0.9f); p.startNewSubPath (c.x - s * 0.6f, c.y - s * 0.3f); p.lineTo (c.x, c.y - s * 0.9f); p.lineTo (c.x + s * 0.6f, c.y - s * 0.3f); break;   // up
        case 2: p.addCentredArc (c.x, c.y, s, s, 0, 0.3f, 5.6f, true); p.lineTo (c.x + std::sin (5.6f) * s + s * 0.5f, c.y - std::cos (5.6f) * s - s * 0.1f); break;                                                // swirl
        case 3: p.addEllipse (c.x - s * 0.35f, c.y - s * 0.35f, s * 0.7f, s * 0.7f); p.addEllipse (c.x - s * 1.1f, c.y - s * 1.1f, s * 2.2f, s * 2.2f); break;                                                      // press
        case 4: for (int k = 0; k < 3; ++k) { const float y = c.y - s * 0.7f + (float) k * s * 0.7f; p.startNewSubPath (c.x - s, y); p.quadraticTo (c.x, y - s * 0.5f, c.x + s, y); } break;                              // rub
        case 5: p.startNewSubPath (c.x - s * 1.2f, c.y); p.lineTo (c.x + s * 1.2f, c.y); p.startNewSubPath (c.x, c.y - s); p.lineTo (c.x, c.y - s * 0.3f); p.startNewSubPath (c.x, c.y + s); p.lineTo (c.x, c.y + s * 0.3f); break;   // line
        default: p.addEllipse (c.x - s, c.y - s, s * 2, s * 2); p.startNewSubPath (c.x, c.y); p.lineTo (c.x + s * 0.7f, c.y - s * 0.7f); break;                                                                       // point
    }
    g.setColour (col.brighter (0.3f)); g.strokePath (p, PathStrokeType (2.0f, PathStrokeType::curved, PathStrokeType::rounded));
    r.removeFromLeft (10);
    g.setColour (t.text); g.setFont (kk::modern::font (13.5f, true, 0.18f)); g.drawText (title, r.removeFromTop (r.getHeight() * 0.36f), Justification::bottomLeft);
    g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.02f)); g.drawFittedText (sub, r.removeFromTop (r.getHeight() * 0.55f).toNearestInt(), Justification::centredLeft, 1, 0.8f);
    const auto bar = r.withHeight (5.0f).withY (r.getY() + 3.0f);
    g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (bar, 2.5f);
    g.setGradientFill (ColourGradient (col.darker (0.4f), bar.getX(), 0, col.brighter (0.3f), bar.getRight(), 0, false)); g.fillRoundedRectangle (bar.withWidth (jmax (5.0f, bar.getWidth() * jlimit (0.0f, 1.0f, amount))), 2.5f);
}

// ---------------------------------------------------------------- LIQUID ----------------------------------------------------------------
class LiquidPage : public Component, public SettableTooltipClient, private Timer
{
public:
    LiquidPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "LIQUID on / off - the kick carves its own hole in the track, the bass flows around it", [this] { proc.liquid.on = ! proc.liquid.on.load(); refresh(); });
        btn (scBtn, "SIDECHAIN", "KICK SOURCE: the kick routed into this plugin's sidechain input", [this] { proc.liquid.source = kk::pro::lsSidechain; proc.liquid.on = true; refresh(); });
        btn (selfBtn, "SELF", "KICK SOURCE: no sidechain - the kicks in this track's own low end", [this] { proc.liquid.source = kk::pro::lsSelf; proc.liquid.on = true; refresh(); });
        setTooltip ("Drag the pink drop down into the vessel = a deeper hole.  Swirl the mouse in circles in the liquid: fast = water (closes fast), slow = mercury (slow).  "
                    "Lift the surface of the bass = more of it flows up.  Wheel over the vessel = dry / wet.");
        for (int b = 0; b < kk::pro::LiquidState::numBands; ++b) hole[(size_t) b] = proc.liquid.mHole[(size_t) b].load();
        kick = proc.liquid.mKick.load();
        startTimerHz (30);
        refresh();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const Colour cKick (0xffff3b8a);
        const float visc = proc.liquid.viscosity.load(), mix = proc.liquid.mix.load(), depth = proc.liquid.depth.load();
        const Colour cTrack = Colour (0xff22d3ee).interpolatedWith (Colour (0xffc9d2dc), visc * 0.75f);   // water .. mercury
        proHeader (g, *this, "LIQUID", "ORGANIC ERA", "the kick does not turn the bass down - it carves its own hole in the low end, and the bass flows around it like mercury", Colour (0xff22d3ee));
        const auto V = vessel().toFloat();
        const auto R = inner();
        const int nb = kk::pro::LiquidState::numBands, regions = nb + 1;
        const float rw = R.getWidth() / (float) regions, flow = proc.liquid.flow.load();
        auto at = [&] (const std::array<float, 6>& v, float x)   // smooth curve through the region centres
        {
            const float u = jlimit (0.0f, (float) regions - 1.0f, (x - R.getX()) / rw - 0.5f);
            const int i0 = (int) u; const int i1 = std::min (regions - 1, i0 + 1); const float f = 0.5f - 0.5f * std::cos (MathConstants<float>::pi * (u - (float) i0));
            return v[(size_t) i0] + (v[(size_t) i1] - v[(size_t) i0]) * f;
        };
        std::array<float, 6> holes {}, bulge {};
        for (int b = 0; b < nb; ++b) { holes[(size_t) b] = hole[(size_t) b]; bulge[(size_t) b + 1] += flow * hole[(size_t) b]; }
        // the vessel
        Path glass; glass.addRoundedRectangle (R.getX(), R.getY() - 6, R.getWidth(), R.getHeight() + 6, 46.0f, 46.0f, false, false, true, true);
        g.setColour (Colour (0xff050912).withAlpha (t.night ? 0.75f : 0.85f)); g.fillPath (glass);
        const float liqA = 0.35f + 0.65f * mix;
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (glass);
            const float yBase = surfaceY();
            Path track, kickP, surf;
            const int steps = 160;
            for (int s = 0; s <= steps; ++s)
            {
                const float x = R.getX() + R.getWidth() * (float) s / (float) steps;
                const float wave = (1.0f - 0.6f * visc);
                const float y = yBase + (std::sin (x * 0.02f + phase * 2.0f) * 4.0f + std::sin (x * 0.047f - phase * 1.3f) * 2.5f) * wave + at (holes, x) * R.getHeight() * 0.42f * depth - at (bulge, x) * R.getHeight() * 0.14f;
                if (s == 0) { track.startNewSubPath (x, R.getBottom()); surf.startNewSubPath (x, y); } else surf.lineTo (x, y);
                track.lineTo (x, y);
                const float ky = R.getBottom() - at (holes, x) * R.getHeight() * 0.5f * depth - 6.0f * kick + std::sin (x * 0.03f + phase * 3.0f) * 3.0f * at (holes, x);
                if (s == 0) kickP.startNewSubPath (x, R.getBottom() + 2);
                kickP.lineTo (x, ky);
            }
            track.lineTo (R.getRight(), R.getBottom()); track.closeSubPath();
            kickP.lineTo (R.getRight(), R.getBottom() + 2); kickP.closeSubPath();
            g.setGradientFill (ColourGradient (cTrack.withAlpha (0.85f * liqA), 0, yBase, Colour (0xff0b2f5a).withAlpha (liqA), 0, R.getBottom(), false)); g.fillPath (track);
            // the swirl: the liquid's own motion (water spins fast, mercury barely turns)
            {
                const Point<float> sc (swirlAt.x > 0 ? swirlAt : Point<float> (R.getX() + R.getWidth() * 0.62f, (yBase + R.getBottom()) * 0.5f));
                for (int arm = 0; arm < 3; ++arm)
                {
                    Path sp;
                    for (int i = 0; i <= 50; ++i)
                    {
                        const float u = (float) i / 50.0f, ang = swirlPh + (float) arm * 2.094f + u * 5.0f, rad = 8.0f + u * (60.0f + 50.0f * swirlGlow);
                        const Point<float> q (sc.x + std::cos (ang) * rad * 1.5f, sc.y + std::sin (ang) * rad * 0.55f);
                        if (i == 0) sp.startNewSubPath (q); else sp.lineTo (q);
                    }
                    g.setColour (Colours::white.withAlpha ((0.08f + 0.3f * swirlGlow) * liqA)); g.strokePath (sp, PathStrokeType (1.2f + 1.5f * swirlGlow));
                }
            }
            g.setGradientFill (ColourGradient (cKick.brighter (0.3f), 0, R.getBottom() - R.getHeight() * 0.5f, Colour (0xff5a0a2e), 0, R.getBottom(), false)); g.fillPath (kickP);
            g.setColour (cKick.withAlpha (0.35f)); g.strokePath (kickP, PathStrokeType (6.0f));
            g.setColour (Colours::white.withAlpha (surfHot ? 1.0f : 0.75f)); g.strokePath (surf, PathStrokeType (surfHot ? 3.5f : 2.0f));
            // the spill: drops flow from each hole into the band above
            Random rr (11);
            for (int b = 0; b < nb; ++b)
                for (int d = 0; d < 9; ++d)
                {
                    const float amt = hole[(size_t) b] * flow;
                    if (amt < 0.04f) break;
                    const float u = std::fmod (rr.nextFloat() + phase * 0.35f * (1.2f - visc), 1.0f);
                    const float x0 = R.getX() + rw * ((float) b + 0.5f), x1 = x0 + rw;
                    const float x = x0 + (x1 - x0) * u, y = yBase + R.getHeight() * 0.4f * hole[(size_t) b] * (1.0f - u) - std::sin (u * MathConstants<float>::pi) * R.getHeight() * 0.07f * amt - 6.0f;
                    const float r = 3.0f + 5.0f * rr.nextFloat() * amt;
                    g.setColour (cTrack.brighter (0.5f).withAlpha (jmin (1.0f, amt) * (1.0f - u * 0.6f))); g.fillEllipse (x - r, y - r, 2 * r, 2 * r);
                }
            for (int i = 0; i < 40; ++i)   // rising bubbles (slower in mercury)
            {
                const float u = std::fmod (rr.nextFloat() + phase * (0.05f + 0.1f * rr.nextFloat()) * (1.15f - visc), 1.0f), x = R.getX() + rr.nextFloat() * R.getWidth();
                const float y = R.getBottom() - u * (R.getBottom() - yBase - 10.0f), r = 1.5f + 2.5f * rr.nextFloat();
                g.setColour (Colours::white.withAlpha (0.25f * (1.0f - u))); g.drawEllipse (x - r, y - r, 2 * r, 2 * r, 1.0f);
            }
            for (int b = 1; b < regions; ++b) { g.setColour (Colours::white.withAlpha (0.08f)); g.fillRect (R.getX() + rw * (float) b, R.getY(), 1.0f, R.getHeight()); }
        }
        g.setColour (Colours::white.withAlpha (0.35f)); g.strokePath (glass, PathStrokeType (2.5f));
        g.setColour (Colours::white.withAlpha (0.08f)); g.fillRoundedRectangle (R.getX() + 18, R.getY() + 10, 10, R.getHeight() * 0.7f, 5);
        // the kick's drop: drag it down into the vessel
        {
            const auto d = dropPos();
            g.setColour (cKick.withAlpha (0.35f));
            { Path th; th.startNewSubPath (d.x, R.getY() - 4); th.lineTo (d.x, d.y - 26); Path dashed; const float dash[] { 4.0f, 5.0f }; PathStrokeType (1.2f).createDashedStroke (dashed, th, dash, 2); g.fillPath (dashed); }
            const float s = dropHot ? 1.15f : 1.0f, pulse = 1.0f + 0.12f * kick;
            const float r = 17.0f * s * pulse;
            g.setGradientFill (ColourGradient (cKick.withAlpha (0.55f), d.x, d.y, cKick.withAlpha (0.0f), d.x + r * 2.6f, d.y, true)); g.fillEllipse (d.x - r * 2.6f, d.y - r * 2.6f, r * 5.2f, r * 5.2f);
            Path drop; drop.startNewSubPath (d.x, d.y - r * 1.9f);
            drop.cubicTo (d.x + r * 0.35f, d.y - r * 0.9f, d.x + r, d.y - r * 0.4f, d.x + r, d.y + r * 0.15f);
            drop.addCentredArc (d.x, d.y + r * 0.15f, r, r, 0, MathConstants<float>::halfPi, MathConstants<float>::pi * 1.5f, false);
            drop.cubicTo (d.x - r, d.y - r * 0.4f, d.x - r * 0.35f, d.y - r * 0.9f, d.x, d.y - r * 1.9f);
            g.setGradientFill (ColourGradient (Colours::white, d.x - r * 0.4f, d.y - r * 0.3f, cKick, d.x + r, d.y + r, true)); g.fillPath (drop);
            g.setColour (Colours::white.withAlpha (0.85f)); g.strokePath (drop, PathStrokeType (1.4f));
            g.setColour (Colours::white.withAlpha (0.8f)); g.setFont (kk::modern::font (11.0f, true, 0.3f));
            g.drawText ("KICK", Rectangle<float> (d.x - 40, d.y + r + 6, 80, 14), Justification::centred);
        }
        // band names (words, no numbers)
        static const char* names[] { "SUB", "DEEP", "LOW", "BODY", "WARM", "ABOVE" };
        for (int b = 0; b < regions; ++b)
        {
            const auto cell = Rectangle<float> (R.getX() + rw * (float) b, R.getBottom() + 8, rw, 18);
            g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.25f)); g.drawText (names[b], cell, Justification::centred);
            if (b == nb && flow > 0.01f) { g.setColour (cTrack.brighter (0.4f)); g.setFont (kk::modern::font (11.0f, true, 0.12f)); g.drawText ("FLOWS HERE", Rectangle<float> (cell.getX(), R.getY() + 12, rw, 16), Justification::centred); }
        }
        g.setColour (Colours::white.withAlpha (0.7f)); g.setFont (kk::modern::font (12.0f, true, 0.25f));
        g.drawText (visc < 0.25f ? "LIKE WATER" : visc < 0.55f ? "LIKE OIL" : visc < 0.8f ? "LIKE HONEY" : "LIKE MERCURY", Rectangle<float> (R.getX() + 40, R.getY() + 12, 220, 16), Justification::centredLeft);
        // the right column
        const auto C = column();
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText ("KICK SOURCE", C.getX(), C.getY(), 200, 14, Justification::centredLeft);
        const bool sc = proc.liquid.source.load() == kk::pro::lsSidechain, live = proc.liquid.mSidechain.load();
        g.setColour (sc && ! live && proc.liquid.on.load() ? Colour (0xffffb020) : t.text.withAlpha (0.8f)); g.setFont (kk::modern::font (12.5f, true, 0.03f));
        g.drawFittedText (! sc ? String ("SELF: the kicks in this track's own low end") : live ? String ("SIDECHAIN: kick arriving") : String ("SIDECHAIN: nothing arriving yet - route the kick in (below)"), C.getX(), C.getY() + 66, C.getWidth(), 18, Justification::centredLeft, 1, 0.8f);
        // touch it
        const auto T = Rectangle<float> ((float) C.getX(), (float) C.getY() + 98, (float) C.getWidth(), 300.0f);
        kk::modern::well (g, T, 12.0f);
        g.setColour (Colour (0xff22d3ee)); g.setFont (kk::modern::font (13.0f, true, 0.25f));
        g.drawText ("TOUCH THE LIQUID", T.reduced (18, 12).removeFromTop (18), Justification::centredLeft);
        const float rh = (T.getHeight() - 44) / 3.0f;
        gestureRow (g, Rectangle<float> (T.getX() + 14, T.getY() + 38, T.getWidth() - 32, rh - 6), 0, "DRAG THE DROP DOWN", "deeper = the kick carves a deeper hole", depth, cKick);
        gestureRow (g, Rectangle<float> (T.getX() + 14, T.getY() + 38 + rh, T.getWidth() - 32, rh - 6), 2, "SWIRL THE LIQUID", "fast circles = water, slow = mercury", visc, Colour (0xffc9d2dc));
        gestureRow (g, Rectangle<float> (T.getX() + 14, T.getY() + 38 + rh * 2, T.getWidth() - 32, rh - 6), 1, "LIFT THE SURFACE", "higher = more bass flows one octave up", flow, Colour (0xff22d3ee));
        // kick meter
        const auto km = Rectangle<float> ((float) C.getX(), T.getBottom() + 34, (float) C.getWidth(), 14);
        g.setColour (Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (km, 7);
        g.setGradientFill (ColourGradient (cKick.darker (0.3f), km.getX(), 0, cKick.brighter (0.4f), km.getRight(), 0, false)); g.fillRoundedRectangle (km.withWidth (km.getWidth() * jlimit (0.0f, 1.0f, kick)), 7);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText ("KICK", (int) km.getX(), (int) km.getY() - 18, 100, 14, Justification::centredLeft);
        // how to route the kick (FL Studio mixer)
        const auto how = Rectangle<float> ((float) C.getX(), km.getBottom() + 22, (float) C.getWidth(), V.getBottom() - km.getBottom() - 22);
        kk::modern::well (g, how, 12.0f);
        g.setColour (Colour (0xff22d3ee)); g.setFont (kk::modern::font (13.0f, true, 0.25f));
        g.drawText ("ROUTE THE KICK IN", how.reduced (18, 14).removeFromTop (18), Justification::centredLeft);
        g.setColour (t.text); g.setFont (kk::modern::font (16.0f, true, 0.02f));
        g.drawFittedText ("1.  put EVOLVE FX PRO on the bass (or the whole beat) track\n\n"
                          "2.  in the FL mixer select the KICK track\n\n"
                          "3.  right-click the send arrow under the track with this plugin  ->  \"Sidechain to this track\"\n\n"
                          "4.  KICK SOURCE: SIDECHAIN.   no routing?  SELF hears the kicks in the track itself",
                          how.reduced (18, 14).withTrimmedTop (34).toNearestInt(), Justification::topLeft, 14, 0.9f);
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32);
        const auto C = column();
        scBtn.setBounds (C.getX(), C.getY() + 20, C.getWidth() / 2 - 4, 40); selfBtn.setBounds (C.getX() + C.getWidth() / 2 + 4, C.getY() + 20, C.getWidth() / 2 - 4, 40);
    }
    void mouseMove (const MouseEvent& e) override
    {
        const bool d = e.position.getDistanceFrom (dropPos()) < 30.0f, s = ! d && nearSurface (e.position);
        if (d != dropHot || s != surfHot) { dropHot = d; surfHot = s; repaint(); }
        setMouseCursor (d || s ? MouseCursor::UpDownResizeCursor : inner().contains (e.position) ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor);
    }
    void mouseDown (const MouseEvent& e) override
    {
        mode = mNone;
        if (e.position.getDistanceFrom (dropPos()) < 30.0f) mode = mDrop;
        else if (nearSurface (e.position)) mode = mSurface;
        else if (inner().contains (e.position)) { mode = mSwirl; trail.clear(); swirlAngle = 0; lastAng = 0; haveAng = false; }
        if (mode == mNone) return;
        proc.liquid.on = true; refresh();
        mouseDrag (e);
    }
    void mouseDrag (const MouseEvent& e) override
    {
        const auto R = inner();
        if (mode == mDrop) proc.liquid.depth = jlimit (0.0f, 1.0f, (e.position.y - R.getY() - R.getHeight() * 0.06f) / (R.getHeight() * 0.8f));
        else if (mode == mSurface) proc.liquid.flow = jlimit (0.0f, 1.0f, (0.5f - (e.position.y - R.getY()) / R.getHeight()) / 0.34f);
        else if (mode == mSwirl)
        {
            // the angular speed of the circles around their own centre
            const double now = Time::getMillisecondCounterHiRes();
            trail.push_back ({ e.position, now });
            while (trail.size() > 2 && now - trail.front().t > 600.0) trail.erase (trail.begin());
            Point<float> cen; for (auto& q : trail) cen += q.p; cen /= (float) trail.size();
            const float ang = std::atan2 (e.position.y - cen.y, e.position.x - cen.x);
            if (haveAng && e.position.getDistanceFrom (cen) > 12.0f)
            {
                float d = ang - lastAng; while (d > MathConstants<float>::pi) d -= MathConstants<float>::twoPi; while (d < -MathConstants<float>::pi) d += MathConstants<float>::twoPi;
                swirlAngle += d;
                const double span = jmax (1.0, now - trail.front().t);
                if (std::abs (swirlAngle) > MathConstants<float>::pi && trail.size() > 6)
                {
                    float total = 0; for (size_t i = 1; i < trail.size(); ++i) { const auto a0 = std::atan2 (trail[i - 1].p.y - cen.y, trail[i - 1].p.x - cen.x), a1 = std::atan2 (trail[i].p.y - cen.y, trail[i].p.x - cen.x); float dd = a1 - a0; while (dd > MathConstants<float>::pi) dd -= MathConstants<float>::twoPi; while (dd < -MathConstants<float>::pi) dd += MathConstants<float>::twoPi; total += dd; }
                    const float omega = std::abs (total) / (float) (span / 1000.0);   // rad / s
                    const float target = jlimit (0.0f, 1.0f, 1.0f - (omega - 3.0f) / 13.0f);
                    proc.liquid.viscosity = proc.liquid.viscosity.load() + (target - proc.liquid.viscosity.load()) * 0.12f;
                    swirlGlow = 1.0f; swirlDir = total > 0 ? 1.0f : -1.0f;
                }
            }
            lastAng = ang; haveAng = true; swirlAt = cen;
        }
        repaint();
    }
    void mouseUp (const MouseEvent&) override { mode = mNone; }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        if (! vessel().contains (e.getPosition())) return;
        proc.liquid.mix = jlimit (0.0f, 1.0f, proc.liquid.mix.load() + w.deltaY * 0.25f); repaint();
    }
private:
    enum Mode { mNone, mDrop, mSurface, mSwirl };
    struct TP { Point<float> p; double t; };
    Rectangle<int> vessel() const { return { 24, 100, getWidth() - 24 - 470, getHeight() - 140 }; }
    Rectangle<int> column() const { return { getWidth() - 440, 100, 416, getHeight() - 140 }; }
    Rectangle<float> inner() const { return vessel().toFloat().reduced (14, 10).withTrimmedBottom (30); }
    float surfaceY() const { const auto R = inner(); return R.getY() + R.getHeight() * (0.5f - 0.34f * proc.liquid.flow.load()); }
    bool nearSurface (Point<float> p) const { const auto R = inner(); return p.x > R.getX() + R.getWidth() / 6.0f && R.contains (p) && std::abs (p.y - surfaceY()) < 22.0f; }
    Point<float> dropPos() const { const auto R = inner(); return { R.getX() + R.getWidth() / 6.0f * 1.5f, R.getY() + R.getHeight() * (0.06f + 0.8f * proc.liquid.depth.load()) }; }
    void refresh()
    {
        onBtn.selected = proc.liquid.on.load(); onBtn.setButtonText (proc.liquid.on.load() ? "ON" : "OFF"); onBtn.repaint();
        scBtn.selected = proc.liquid.source.load() == kk::pro::lsSidechain; selfBtn.selected = ! scBtn.selected; scBtn.repaint(); selfBtn.repaint();
        repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.04f;
        const float visc = proc.liquid.viscosity.load();
        swirlPh += swirlDir * (0.02f + 0.16f * (1.0f - visc)); swirlGlow *= 0.96f;
        for (int b = 0; b < kk::pro::LiquidState::numBands; ++b) { const float m = proc.liquid.mHole[(size_t) b].load(); hole[(size_t) b] += (m - hole[(size_t) b]) * (m > hole[(size_t) b] ? 0.7f : 0.35f - 0.27f * visc); }
        kick += (proc.liquid.mKick.load() - kick) * 0.4f;
        repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf }, scBtn { lnf }, selfBtn { lnf };
    std::array<float, kk::pro::LiquidState::numBands> hole {};
    float kick = 0, phase = 0, swirlPh = 0, swirlGlow = 0, swirlDir = 1, swirlAngle = 0, lastAng = 0;
    bool haveAng = false, dropHot = false, surfHot = false;
    Mode mode = mNone; std::vector<TP> trail; Point<float> swirlAt { -1, -1 };
};

// ---------------------------------------------------------------- INTENT ----------------------------------------------------------------
class IntentPage : public Component, public SettableTooltipClient, private Timer
{
public:
    IntentPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "INTENT on / off - one breath moves rasp, width, filter, reverb freeze, tremor and attack together", [this] { proc.intent.on = ! proc.intent.on.load(); refresh(); });
        setTooltip ("MOOD MAP: drag the glowing point.  The centre = relaxed and clean, further out = more intense.  "
                    "Its direction blends the two nearest states.  Double-click = back to calm centre.  Wheel = dry / wet.");
        pt = pointFromState();
        breath = proc.intent.level.load();
        startTimerHz (30);
        refresh();
    }
    static Colour stateColour (int m) { static const uint32 c[] { 0xff38bdf8, 0xffff4d2e, 0xffd4f542, 0xffa78bfa }; return Colour (c[jlimit (0, 3, m)]); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const int mode = proc.intent.mode.load(), mode2 = proc.intent.mode2.load();
        const float blend = proc.intent.blend.load(), level = proc.intent.level.load();
        const auto col = stateColour (mode).interpolatedWith (stateColour (mode2), blend);
        proHeader (g, *this, "INTENT", "ORGANIC ERA", "a mood map: one point moves many muscles at once - rasp, width, filter, frozen air, tremor, attack.  drag it out of the calm centre", stateColour (mode));
        kk::pro::IntentDsp::Targets tg; kk::pro::IntentDsp::mappingBlend (mode, mode2, blend, breath, tg);
        const bool on = proc.intent.on.load();
        const float rasp = on ? proc.intent.mRasp.load() : tg.rasp, width = on ? proc.intent.mWidth.load() : tg.width, freeze = on ? proc.intent.mFreeze.load() : tg.freeze;
        const float hz = on ? proc.intent.mFilterHz.load() : tg.lpHz, trem = on ? proc.intent.mTremor.load() : tg.tremor;
        // ---- the mood map
        const auto M = mapArea().toFloat();
        g.setColour (Colour (0xff04060c).withAlpha (t.night ? 0.8f : 0.9f)); g.fillRoundedRectangle (M, 18);
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (M.toNearestInt());
            const float big = std::max (M.getWidth(), M.getHeight());
            for (int c = 0; c < 4; ++c)
            {
                const auto cp = corner (c);
                const auto sc = stateColour (c);
                const float w = c == mode ? 1.0f - blend : c == mode2 ? blend : 0.0f;
                g.setGradientFill (ColourGradient (sc.withAlpha (0.32f + 0.3f * w * (1.0f - level)), cp.x, cp.y, sc.withAlpha (0.0f), cp.x + big * 0.55f, cp.y, true));
                g.fillRect (M);
            }
            const auto cen = M.getCentre();
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.16f), cen.x, cen.y, Colours::white.withAlpha (0.0f), cen.x + M.getHeight() * 0.3f, cen.y, true));
            g.fillEllipse (cen.x - M.getHeight() * 0.3f, cen.y - M.getHeight() * 0.3f, M.getHeight() * 0.6f, M.getHeight() * 0.6f);
            // intensity rings (they breathe)
            for (int k = 1; k <= 5; ++k)
            {
                const float u = (float) k / 5.0f, sw = 1.0f + 0.015f * std::sin (phase * 1.3f - (float) k);
                const float rx = u * M.getWidth() * 0.5f / 1.0f * sw, ry = u * M.getHeight() * 0.5f * sw;
                g.setColour (Colours::white.withAlpha (0.05f + 0.03f * (float) (k == 5))); g.drawEllipse (cen.x - rx, cen.y - ry, rx * 2, ry * 2, 1.0f);
            }
            g.setColour (Colours::white.withAlpha (0.05f)); g.drawLine (M.getX(), cen.y, M.getRight(), cen.y, 1.0f); g.drawLine (cen.x, M.getY(), cen.x, M.getBottom(), 1.0f);
            // the point, its pull towards the two states, its trail
            const auto P = toPx (pt);
            for (int c : { mode, mode2 })
            {
                const float w = c == mode ? 1.0f - blend : blend;
                if (w < 0.03f || (c == mode2 && mode2 == mode)) continue;
                g.setColour (stateColour (c).withAlpha (0.15f + 0.6f * w * (1.0f - level))); g.drawLine (Line<float> (P, corner (c)), 1.0f + 3.0f * w);
            }
            for (size_t i = 0; i < trailPts.size(); ++i)
            {
                const float a = (float) (i + 1) / (float) trailPts.size();
                const auto q = toPx (trailPts[i]);
                g.setColour (col.withAlpha (0.25f * a)); g.fillEllipse (q.x - 6 * a, q.y - 6 * a, 12 * a, 12 * a);
            }
            const float pr = 20.0f + 8.0f * pulse + 3.0f * std::sin (phase * 2.0f);
            const auto pc = col.withSaturation (jmin (1.0f, col.getSaturation() * 1.3f)).withBrightness (1.0f);
            for (int k = 0; k < 3; ++k) { const float u = std::fmod (phase * 0.4f + (float) k / 3.0f, 1.0f), rr = pr * (1.2f + 2.5f * u); g.setColour (pc.withAlpha (0.6f * (1.0f - u))); g.drawEllipse (P.x - rr, P.y - rr, rr * 2, rr * 2, 2.0f); }
            g.setGradientFill (ColourGradient (pc.withAlpha (0.85f), P.x, P.y, pc.withAlpha (0.0f), P.x + pr * 3.2f, P.y, true)); g.fillEllipse (P.x - pr * 3.2f, P.y - pr * 3.2f, pr * 6.4f, pr * 6.4f);
            g.setGradientFill (ColourGradient (Colours::white, P.x - pr * 0.3f, P.y - pr * 0.3f, pc, P.x + pr, P.y + pr, true)); g.fillEllipse (P.x - pr, P.y - pr, pr * 2, pr * 2);
            g.setColour (Colours::white.withAlpha (0.9f)); g.drawEllipse (P.x - pr, P.y - pr, pr * 2, pr * 2, dragging ? 2.5f : 1.5f);
        }
        g.setColour (col.withAlpha (0.4f)); g.drawRoundedRectangle (M, 18, 1.2f);
        // the states in the corners
        static const char* what[] { "the air runs out: a rasp, mono, frozen air", "harder hits, a growl, a tight image", "it shakes, the pitch jitters, the room closes in", "soft attacks, a wide warm room, a darker top" };
        for (int c = 0; c < 4; ++c)
        {
            const bool left = c == kk::pro::imOxygen || c == kk::pro::imCalm, topRow = c == kk::pro::imOxygen || c == kk::pro::imAggression;
            const auto box = Rectangle<float> (left ? M.getX() + 22 : M.getRight() - 342, topRow ? M.getY() + 18 : M.getBottom() - 60, 320, 44);
            const auto j = left ? Justification::centredLeft : Justification::centredRight;
            g.setColour (stateColour (c)); g.setFont (kk::modern::font (20.0f, true, 0.3f)); g.drawText (kk::pro::intentModeName (c), box.withHeight (24), j);
            g.setColour (Colours::white.withAlpha (0.6f)); g.setFont (kk::modern::font (12.0f, true, 0.02f)); g.drawText (what[c], box.withTrimmedTop (26), j);
        }
        g.setColour (Colours::white.withAlpha (0.55f)); g.setFont (kk::modern::font (12.0f, true, 0.35f));
        g.drawText (level > 0.97f ? "RELAXED  .  CLEAN" : level > 0.6f ? "A LITTLE" : level > 0.3f ? "STRONGER" : "AT ITS EXTREME", Rectangle<float> (M.getCentreX() - 150, M.getCentreY() + 26, 300, 16), Justification::centred);
        // ---- the lungs (smaller, top of the column)
        const auto A = lungArea().toFloat();
        kk::modern::well (g, A, 16.0f);
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (A.toNearestInt());
            Random jr ((int64) (phase * 30.0f));
            const auto c = A.getCentre().translated (trem * 4.0f * (jr.nextFloat() - 0.5f), A.getHeight() * 0.08f + trem * 4.0f * (jr.nextFloat() - 0.5f));
            const float s = jmin (A.getWidth(), A.getHeight()) * 0.48f;
            const float rate = mode == kk::pro::imAggression ? 2.6f : mode == kk::pro::imStress ? 3.4f : mode == kk::pro::imCalm ? 0.8f : 1.2f;
            const float swell = (0.6f + 0.4f * breath) * (1.0f + 0.04f * std::sin (phase * rate) + 0.08f * pulse);
            const float sep = s * (0.16f + 0.24f * jmin (1.5f, width)) * swell;
            const auto lobeCol = col.interpolatedWith (Colour (0xffe8f6ff), freeze * 0.7f);
            g.setGradientFill (ColourGradient (lobeCol.withAlpha (0.22f * (0.4f + 0.6f * breath)), c.x, c.y, lobeCol.withAlpha (0.0f), c.x + s * 1.2f, c.y, true));
            g.fillEllipse (c.x - s * 1.2f, c.y - s * 1.2f, s * 2.4f, s * 2.4f);
            for (int side = -1; side <= 1; side += 2)
            {
                const Point<float> lc (c.x + (float) side * sep, c.y + s * 0.02f);
                const float rx = s * 0.44f * swell, ry = s * 0.74f * swell;
                Path lobe;
                for (int i = 0; i <= 120; ++i)
                {
                    const float a = MathConstants<float>::twoPi * (float) i / 120.0f;
                    const float ax = std::sin (a), ay = -std::cos (a);
                    const bool inner = ax * (float) side < 0;
                    float r = 1.0f + 0.03f * std::sin (3.0f * a + phase) + rasp * 0.07f * std::sin (a * 23.0f + phase * 5.0f) * std::sin (a * 7.0f - phase * 2.0f);
                    float x = ax * rx * r * (inner ? 0.62f : 1.0f), y = ay * ry * r;
                    if (ay < 0) x *= 0.75f + 0.25f * (1.0f + ay);
                    if (inner && ay > 0.1f) x *= 1.0f - 0.35f * ay;
                    if (i == 0) lobe.startNewSubPath (lc.x + x, lc.y + y); else lobe.lineTo (lc.x + x, lc.y + y);
                }
                lobe.closeSubPath();
                g.setGradientFill (ColourGradient (lobeCol.brighter (0.35f).withAlpha (0.95f), lc.x - (float) side * rx * 0.3f, lc.y - ry * 0.4f, lobeCol.darker (0.8f).withAlpha (0.9f), lc.x + (float) side * rx, lc.y + ry, true));
                g.fillPath (lobe);
                g.setColour (lobeCol.brighter (0.6f).withAlpha (0.8f)); g.strokePath (lobe, PathStrokeType (1.6f + 2.0f * pulse));
                Random br (side > 0 ? 7 : 13);
                std::function<void (Point<float>, float, float, int)> branch = [&] (Point<float> p0, float ang, float len, int depth)
                {
                    const Point<float> p1 (p0.x + std::sin (ang) * len, p0.y + std::cos (ang) * len);
                    g.setColour (Colours::white.withAlpha (0.18f + 0.1f * (float) depth)); g.drawLine (Line<float> (p0, p1), 0.6f + 1.0f * (float) depth);
                    if (depth > 0) for (int k = 0; k < 2; ++k) branch (p1, ang + (k == 0 ? -1.0f : 1.0f) * (0.35f + 0.3f * br.nextFloat()), len * 0.68f, depth - 1);
                };
                branch (Point<float> (c.x + (float) side * sep * 0.35f, c.y - s * 0.2f), (float) side * 0.55f, s * 0.2f * swell, 4);
                if (freeze > 0.02f)
                    for (int f = 0; f < 7; ++f)
                    {
                        const Point<float> fp (lc.x + (br.nextFloat() - 0.5f) * rx * 1.1f, lc.y + (br.nextFloat() - 0.5f) * ry * 1.4f);
                        const float fr = (5.0f + 9.0f * br.nextFloat()) * freeze;
                        g.setColour (Colours::white.withAlpha (0.75f * freeze));
                        for (int k = 0; k < 6; ++k) { const float a = (float) k * MathConstants<float>::pi / 3.0f; g.drawLine (fp.x, fp.y, fp.x + std::cos (a) * fr, fp.y + std::sin (a) * fr, 1.1f); }
                    }
            }
            Path tr; tr.startNewSubPath (c.x, A.getY() + 40); tr.lineTo (c.x, c.y - s * 0.28f);
            tr.quadraticTo (c.x, c.y - s * 0.18f, c.x - sep * 0.35f, c.y - s * 0.2f); tr.startNewSubPath (c.x, c.y - s * 0.28f); tr.quadraticTo (c.x, c.y - s * 0.18f, c.x + sep * 0.35f, c.y - s * 0.2f);
            g.setColour (lobeCol.withAlpha (0.55f)); g.strokePath (tr, PathStrokeType (s * 0.07f, PathStrokeType::curved, PathStrokeType::rounded));
        }
        g.setColour (t.text); g.setFont (kk::modern::font (14.0f, true, 0.3f));
        const String name = blend > 0.08f ? String (kk::pro::intentModeName (mode)) + " + " + kk::pro::intentModeName (mode2) : String (kk::pro::intentModeName (mode));
        g.drawText (level > 0.97f ? String ("RELAXED") + (on ? "" : "   (off)") : name + (on ? "" : "   (off)"), A.reduced (18, 12).removeFromTop (20), Justification::centredLeft);
        // ---- what the muscles do (bars, no numbers)
        const auto Mu = muscles();
        auto bar = [&] (int i, const String& n, float u)
        {
            auto r = Rectangle<float> ((float) Mu.getX(), (float) Mu.getY() + (float) i * 52.0f, (float) Mu.getWidth(), 40.0f);
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText (n, r.removeFromTop (16), Justification::centredLeft);
            const auto b = r.withHeight (12.0f).withY (r.getY() + 3);
            g.setColour (Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (b, 6);
            g.setGradientFill (ColourGradient (col.darker (0.4f), b.getX(), 0, col.brighter (0.4f), b.getRight(), 0, false)); g.fillRoundedRectangle (b.withWidth (jmax (12.0f, b.getWidth() * jlimit (0.0f, 1.0f, u))), 6);
        };
        bar (0, "RASP", rasp); bar (1, "WIDTH", width / 1.5f); bar (2, "FROZEN AIR", freeze);
        bar (3, "OPEN TOP", std::log (hz / 20.0f) / std::log (1000.0f)); bar (4, "TREMOR", trem);
        // dry / wet
        const auto w = Rectangle<float> ((float) Mu.getX(), (float) Mu.getBottom() + 14, (float) Mu.getWidth(), 34);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.05f));
        g.drawFittedText ("drag the point out of the centre  .  double-click = calm  .  wheel over the map = dry / wet", w.toNearestInt(), Justification::centredLeft, 2, 0.85f);
    }
    void resized() override { onBtn.setBounds (300, 56, 70, 32); }
    void mouseDown (const MouseEvent& e) override { if (mapArea().contains (e.getPosition())) { dragging = true; setPoint (e.position); } }
    void mouseDrag (const MouseEvent& e) override { if (dragging) setPoint (e.position); }
    void mouseUp (const MouseEvent&) override { dragging = false; repaint(); }
    void mouseDoubleClick (const MouseEvent& e) override { if (mapArea().contains (e.getPosition())) { proc.intent.level = 1.0f; proc.intent.blend = 0.0f; pt = {}; trailPts.clear(); repaint(); } }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        if (! mapArea().contains (e.getPosition())) return;
        proc.intent.mix = jlimit (0.0f, 1.0f, proc.intent.mix.load() + w.deltaY * 0.25f); repaint();
    }
private:
    // corners in the order around the map: OXYGEN (top left), AGGRESSION (top right), STRESS (bottom right), CALM (bottom left)
    static Point<float> cornerN (int c) { static const float x[] { -1, 1, 1, -1 }, y[] { -1, -1, 1, 1 }; return { x[jlimit (0, 3, c)], y[jlimit (0, 3, c)] }; }
    static float cornerAngle (int c) { return std::atan2 (cornerN (c).y, cornerN (c).x); }
    static constexpr float reach = 1.2f;   // |p| at which the state is at its extreme
    Point<float> corner (int c) const { const auto M = mapArea().toFloat().reduced (40); return { M.getCentreX() + cornerN (c).x * M.getWidth() * 0.5f, M.getCentreY() + cornerN (c).y * M.getHeight() * 0.5f }; }
    Point<float> toPx (Point<float> n) const { const auto M = mapArea().toFloat().reduced (40); return { M.getCentreX() + n.x * M.getWidth() * 0.5f, M.getCentreY() + n.y * M.getHeight() * 0.5f }; }
    Point<float> toN (Point<float> p) const { const auto M = mapArea().toFloat().reduced (40); return { jlimit (-1.0f, 1.0f, (p.x - M.getCentreX()) / (M.getWidth() * 0.5f)), jlimit (-1.0f, 1.0f, (p.y - M.getCentreY()) / (M.getHeight() * 0.5f)) }; }
    void setPoint (Point<float> px)
    {
        pt = toN (px);
        const float r = jmin (1.0f, pt.getDistanceFromOrigin() / reach);
        float th = std::atan2 (pt.y, pt.x) + 0.75f * MathConstants<float>::pi;   // 0 at OXYGEN
        while (th < 0) th += MathConstants<float>::twoPi; while (th >= MathConstants<float>::twoPi) th -= MathConstants<float>::twoPi;
        const float segf = th / MathConstants<float>::halfPi; const int seg = jlimit (0, 3, (int) segf); const float f = segf - (float) seg;
        const int a = seg, b = (seg + 1) % 4;
        if (f < 0.5f) { proc.intent.mode = a; proc.intent.mode2 = b; proc.intent.blend = f; } else { proc.intent.mode = b; proc.intent.mode2 = a; proc.intent.blend = 1.0f - f; }
        proc.intent.level = 1.0f - r; proc.intent.on = true;
        trailPts.push_back (pt); if (trailPts.size() > 14) trailPts.erase (trailPts.begin());
        refresh();
    }
    Point<float> pointFromState() const
    {
        const int a = proc.intent.mode.load(), b = proc.intent.mode2.load();
        const float bl = proc.intent.blend.load(), r = (1.0f - jlimit (0.0f, 1.0f, proc.intent.level.load())) * reach;
        float th = cornerAngle (a);
        if (bl > 0.0f && a != b) { float d = cornerAngle (b) - th; while (d > MathConstants<float>::pi) d -= MathConstants<float>::twoPi; while (d < -MathConstants<float>::pi) d += MathConstants<float>::twoPi; th += d * bl; }
        return { jlimit (-1.0f, 1.0f, std::cos (th) * r), jlimit (-1.0f, 1.0f, std::sin (th) * r) };
    }
    Rectangle<int> mapArea() const { return { 24, 100, getWidth() - 24 - 470, getHeight() - 140 }; }
    Rectangle<int> lungArea() const { return { getWidth() - 440, 100, 416, 380 }; }
    Rectangle<int> muscles() const { return { getWidth() - 440, 502, 416, 5 * 52 }; }
    void refresh() { onBtn.selected = proc.intent.on.load(); onBtn.setButtonText (proc.intent.on.load() ? "ON" : "OFF"); onBtn.repaint(); repaint(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f; breath += (proc.intent.level.load() - breath) * 0.3f;
        if (! dragging) { pt = pointFromState(); if (! trailPts.empty()) trailPts.erase (trailPts.begin()); }
        pulse = std::max (pulse * 0.85f, jmin (1.0f, proc.intent.mPulse.load() * (proc.intent.on.load() ? 1.5f : 0.0f)));
        repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf };
    Point<float> pt; std::vector<Point<float>> trailPts;
    float breath = 1, phase = 0, pulse = 0; bool dragging = false;
};

// ---------------------------------------------------------------- EROSION ----------------------------------------------------------------
class ErosionPage : public Component, public SettableTooltipClient, private Timer
{
public:
    ErosionPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "EROSION on / off - the sound tires when you push it, sinks into rumble when you starve it", [this] { proc.erosion.on = ! proc.erosion.on.load(); refresh(); });
        setTooltip ("Press and hold the material: the longer you press, the faster it tires.  Rub it: slow strokes heal it slowly, fast rubbing heals it fast.  "
                    "Drag the glowing load line: lower = it feels the load earlier.  Wheel = dry / wet.");
        Random r (2024);   // the cracks it will grow, in the order they appear
        for (int c = 0; c < 9; ++c)
        {
            Point<float> at (0.08f + 0.84f * r.nextFloat(), r.nextFloat() < 0.6f ? 0.0f : 0.15f + 0.5f * r.nextFloat());
            float ang = MathConstants<float>::pi * (0.35f + 0.3f * r.nextFloat());
            const float born = (float) c / 9.0f * 0.6f;
            for (int s = 0; s < 14; ++s)
            {
                ang += (r.nextFloat() - 0.5f) * 0.9f;
                const Point<float> q (at.x + std::cos (ang) * 0.035f, at.y + std::sin (ang) * 0.045f);
                cracks.push_back ({ at, q, born + (float) s / 14.0f * 0.4f, 1.0f - (float) s / 18.0f });
                if (r.nextFloat() < 0.25f) { Point<float> bp = q; float ba = ang + (r.nextBool() ? 0.9f : -0.9f); for (int b = 0; b < 4; ++b) { const Point<float> bq (bp.x + std::cos (ba) * 0.025f, bp.y + std::sin (ba) * 0.03f); cracks.push_back ({ bp, bq, born + (float) (s + b) / 14.0f * 0.4f + 0.1f, 0.5f }); bp = bq; ba += (r.nextFloat() - 0.5f) * 0.8f; } }
                if (r.nextFloat() < 0.3f) bubbles.push_back ({ q, born + 0.15f + 0.3f * r.nextFloat(), 0.006f + 0.012f * r.nextFloat(), r.nextFloat() });
                at = q;
            }
        }
        for (int k = 0; k < 12; ++k) { const float a = (float) k * 0.5236f + r.nextFloat() * 0.3f; pressRays.push_back ({ a, 0.6f + 0.6f * r.nextFloat() }); }
        fat = proc.erosion.mFatigue.load(); starve = proc.erosion.mStarve.load(); level = proc.erosion.mLevel.load();
        startTimerHz (30);
        refresh();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const Colour col (0xffff8a3d), heal (0xffffd27a);
        proHeader (g, *this, "EROSION", "ORGANIC ERA", "push it and it tires: the highs wear off, bubbles crackle.  starve it and it sinks into rumble.  silence stays silent", col);
        const auto S = slab().toFloat();
        const float sens = proc.erosion.sensitivity.load(), thr = kk::pro::ErosionState::thresholdDb (sens);
        const float heat = jlimit (0.0f, 1.0f, (level - thr) / 6.0f);
        const float shownFat = jmax (fat, preview);
        g.setGradientFill (ColourGradient (Colour (0xff8a8f98), S.getX(), S.getY(), Colour (0xff3d424b), S.getRight(), S.getBottom(), false));
        g.fillRoundedRectangle (S, 14);
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (S.toNearestInt());
            Random gr (99);
            for (int i = 0; i < 900; ++i) { g.setColour (Colours::black.withAlpha (0.06f + 0.1f * gr.nextFloat())); g.fillRect (S.getX() + gr.nextFloat() * S.getWidth(), S.getY() + gr.nextFloat() * S.getHeight(), 2.0f, 2.0f); }
            // the load: a hot tide rising from below, it reaches the line when the input reaches the load
            const float yl = lineY();
            const float tide = jlimit (S.getY(), S.getBottom(), yl + (thr - level) * (S.getBottom() - yl) / jmax (1.0f, thr + 80.0f));
            g.setGradientFill (ColourGradient (col.withAlpha (0.10f + 0.3f * heat), 0, tide, col.withAlpha (0.0f), 0, S.getBottom(), false)); g.fillRect (S.withTop (tide));
            g.setColour (col.withAlpha (0.25f + 0.4f * heat)); g.fillRect (S.getX(), tide, S.getWidth(), 1.5f);
            if (heat > 0) { g.setGradientFill (ColourGradient (col.withAlpha (0.45f * heat), S.getCentreX(), S.getY(), col.withAlpha (0.0f), S.getCentreX(), S.getY() + S.getHeight() * 0.5f, false)); g.fillRect (S); }
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.22f * (1.0f - shownFat)), S.getX(), S.getY(), Colours::white.withAlpha (0.0f), S.getX() + S.getWidth() * 0.4f, S.getY() + S.getHeight() * 0.4f, false)); g.fillRect (S);
            auto px = [&S] (Point<float> n) { return Point<float> (S.getX() + n.x * S.getWidth(), S.getY() + n.y * S.getHeight()); };
            for (auto& c : cracks)
                if (c.born < shownFat)
                {
                    const float grow = jlimit (0.0f, 1.0f, (shownFat - c.born) * 40.0f);
                    const auto a = px (c.a), b = a + (px (c.b) - a) * grow;
                    g.setColour (Colour (0xff120c08).withAlpha (0.85f)); g.drawLine (Line<float> (a, b), 1.0f + 3.0f * c.w * jmin (1.0f, shownFat * 1.5f));
                    g.setColour (col.withAlpha (0.35f * heat)); g.drawLine (Line<float> (a, b), 0.8f);
                }
            for (auto& b : bubbles)
                if (b.born < shownFat)
                {
                    const float pop = std::fmod (phase * (0.6f + b.ph) + b.ph * 7.0f, 1.0f);
                    const float r = b.r * S.getWidth() * (0.4f + 0.8f * pop) * jmin (1.0f, (shownFat - b.born) * 6.0f);
                    const auto c = px (b.p);
                    g.setColour (Colours::white.withAlpha (0.7f * (1.0f - pop))); g.drawEllipse (c.x - r, c.y - r, 2 * r, 2 * r, 1.4f);
                    if (pop > 0.85f) { g.setColour (col.withAlpha (0.8f)); for (int k = 0; k < 5; ++k) { const float an = (float) k * 1.2566f + b.ph * 6.0f; g.fillEllipse (c.x + std::cos (an) * r * 1.5f - 1.5f, c.y + std::sin (an) * r * 1.5f - 1.5f, 3, 3); } }
                }
            // starving: dark sediment rises and the surface wobbles
            if (starve > 0.005f)
            {
                const float top = S.getBottom() - S.getHeight() * (0.06f + 0.5f * starve);
                Path sed; sed.startNewSubPath (S.getX(), S.getBottom());
                for (int i = 0; i <= 80; ++i) { const float u = (float) i / 80.0f; sed.lineTo (S.getX() + S.getWidth() * u, top + std::sin (u * 9.0f + phase * 0.7f) * 8.0f * starve + std::sin (u * 23.0f - phase * 1.7f) * 4.0f * starve); }
                sed.lineTo (S.getRight(), S.getBottom()); sed.closeSubPath();
                g.setGradientFill (ColourGradient (Colour (0xff2b1d12).withAlpha (0.75f + 0.2f * starve), 0, top, Colour (0xff070503), 0, S.getBottom(), false)); g.fillPath (sed);
                Random sr (5);
                for (int i = 0; i < (int) (160 * starve); ++i)
                {
                    const float x = S.getX() + sr.nextFloat() * S.getWidth(), y = top + 14 + sr.nextFloat() * (S.getBottom() - top - 14) + std::sin (phase * 0.5f + (float) i) * 3.0f;
                    g.setColour (Colour (0xffb08a5a).withAlpha (0.25f + 0.3f * sr.nextFloat())); g.fillEllipse (x, y, 2.5f, 2.5f);
                }
            }
            // the press: stress rays grow from your finger while you hold
            if (pressGlow > 0.01f)
            {
                const float f = proc.erosion.fatigue.load();
                for (auto& ray : pressRays)
                {
                    const float len = (30.0f + 160.0f * f) * ray.len * pressGlow;
                    Path cr; Point<float> q = pressAt; float ang = ray.a; cr.startNewSubPath (q);
                    for (int s = 0; s < 6; ++s) { ang += std::sin ((float) s * 1.7f + ray.a * 3.0f) * 0.35f; q += Point<float> (std::cos (ang), std::sin (ang)) * (len / 6.0f); cr.lineTo (q); }
                    g.setColour (Colour (0xff120c08).withAlpha (0.8f * pressGlow)); g.strokePath (cr, PathStrokeType (1.0f + 2.0f * f));
                    g.setColour (col.withAlpha (0.55f * pressGlow)); g.strokePath (cr, PathStrokeType (0.8f));
                }
                const float pr = 18.0f + 30.0f * f;
                g.setGradientFill (ColourGradient (col.withAlpha (0.6f * pressGlow), pressAt.x, pressAt.y, col.withAlpha (0.0f), pressAt.x + pr * 2, pressAt.y, true)); g.fillEllipse (pressAt.x - pr * 2, pressAt.y - pr * 2, pr * 4, pr * 4);
            }
            // the healing touch: a golden trail
            for (size_t i = 0; i < rubTrail.size(); ++i)
            {
                const float a = rubTrail[i].life;
                if (a <= 0.01f) continue;
                const auto q = rubTrail[i].p; const float r = 10.0f + 18.0f * a;
                g.setGradientFill (ColourGradient (heal.withAlpha (0.45f * a), q.x, q.y, heal.withAlpha (0.0f), q.x + r, q.y, true)); g.fillEllipse (q.x - r, q.y - r, r * 2, r * 2);
            }
        }
        g.setColour (Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (S, 14, 2.0f);
        // the load line
        {
            const float yl = lineY();
            g.setColour (col.withAlpha (lineHot ? 0.35f : 0.18f)); g.fillRect (S.getX(), yl - 6, S.getWidth(), 12.0f);
            g.setColour (col.brighter (0.3f)); g.fillRect (S.getX() + 8, yl - 1.0f, S.getWidth() - 16, lineHot ? 3.0f : 2.0f);
            for (int k = 0; k < 2; ++k)
            {
                const float hx = k == 0 ? S.getX() + 8 : S.getRight() - 30;
                g.setColour (col); g.fillRoundedRectangle (hx, yl - 9, 22, 18, 6);
                g.setColour (Colours::black.withAlpha (0.6f)); for (int j = 0; j < 3; ++j) g.fillRect (hx + 6 + (float) j * 4, yl - 4, 1.5f, 8.0f);
            }
            g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (kk::modern::font (11.0f, true, 0.3f));
            g.drawText ("LOAD LINE", Rectangle<float> (S.getX() + 38, yl - 22, 200, 14), Justification::centredLeft);
        }
        g.setColour (Colours::white.withAlpha (0.9f)); g.setFont (kk::modern::font (15.0f, true, 0.25f));
        g.drawText (fat > 0.6f ? "WORN OUT" : fat > 0.2f ? "TIRING" : starve > 0.3f ? "SINKING" : level < kk::pro::ErosionDsp::gateDb ? "AT REST" : "HOLDING", S.reduced (20, 16).removeFromTop (20), Justification::centredLeft);
        // meters (no numbers)
        const auto C = column();
        auto meter = [&] (int row, const String& name, float u, Colour c)
        {
            const auto r = Rectangle<float> ((float) C.getX(), (float) C.getY() + (float) row * 54.0f + 20.0f, (float) C.getWidth(), 14.0f);
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText (name, (int) r.getX(), (int) r.getY() - 18, 300, 14, Justification::centredLeft);
            g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r, 7);
            g.setGradientFill (ColourGradient (c.darker (0.4f), r.getX(), 0, c.brighter (0.3f), r.getRight(), 0, false)); g.fillRoundedRectangle (r.withWidth (r.getWidth() * jlimit (0.0f, 1.0f, u)), 7);
            return r;
        };
        meter (0, "TIREDNESS", fat, col);
        meter (1, "SINKING", starve, Colour (0xffb08a5a));
        const auto lr = meter (2, "LOAD", (level + 80.0f) / 80.0f, Colour (0xff9ca3af));
        const float tx = lr.getX() + lr.getWidth() * jlimit (0.0f, 1.0f, (thr + 80.0f) / 80.0f);
        g.setColour (col); g.fillRect (tx - 1.0f, lr.getY() - 4, 2.0f, lr.getHeight() + 8);
        // touch it
        const auto T = Rectangle<float> ((float) C.getX(), (float) C.getY() + 176, (float) C.getWidth(), 300.0f);
        kk::modern::well (g, T, 12.0f);
        g.setColour (col); g.setFont (kk::modern::font (13.0f, true, 0.25f));
        g.drawText ("TOUCH THE MATERIAL", T.reduced (18, 12).removeFromTop (18), Justification::centredLeft);
        const float rh = (T.getHeight() - 44) / 3.0f;
        gestureRow (g, Rectangle<float> (T.getX() + 14, T.getY() + 38, T.getWidth() - 32, rh - 6), 3, "PRESS AND HOLD", "the longer you press, the faster it tires", proc.erosion.fatigue.load(), col);
        gestureRow (g, Rectangle<float> (T.getX() + 14, T.getY() + 38 + rh, T.getWidth() - 32, rh - 6), 4, "RUB IT", "slow strokes heal slowly, fast rubbing heals fast", proc.erosion.recovery.load(), heal);
        gestureRow (g, Rectangle<float> (T.getX() + 14, T.getY() + 38 + rh * 2, T.getWidth() - 32, rh - 6), 5, "DRAG THE LOAD LINE", "lower = it feels the load earlier", sens, Colour (0xff9ca3af));
        // what it does
        const auto how = Rectangle<float> ((float) C.getX(), T.getBottom() + 16, (float) C.getWidth(), S.getBottom() - T.getBottom() - 16);
        if (how.getHeight() > 60)
        {
            kk::modern::well (g, how, 12.0f);
            g.setColour (col); g.setFont (kk::modern::font (13.0f, true, 0.25f));
            g.drawText ("WHAT IT DOES", how.reduced (18, 14).removeFromTop (18), Justification::centredLeft);
            g.setColour (t.text); g.setFont (kk::modern::font (16.0f, true, 0.02f));
            g.drawFittedText ("LOUD:  the top end wears off and comes back slowly, cavitation bubbles crackle - more the harder you push\n\n"
                              "QUIET:  a subsonic rumble rises under it, the phase starts to drift\n\n"
                              "SILENCE:  nothing - the material rests",
                              how.reduced (18, 14).withTrimmedTop (34).toNearestInt(), Justification::topLeft, 12, 0.9f);
        }
    }
    void resized() override { onBtn.setBounds (300, 56, 70, 32); }
    void mouseMove (const MouseEvent& e) override { const bool h = nearLine (e.position); if (h != lineHot) { lineHot = h; repaint(); } setMouseCursor (h ? MouseCursor::UpDownResizeCursor : slab().contains (e.getPosition()) ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); }
    void mouseDown (const MouseEvent& e) override
    {
        mode = mNone;
        if (nearLine (e.position)) mode = mLine;
        else if (slab().contains (e.getPosition())) { mode = mPress; pressAt = e.position; pressStart = Time::getMillisecondCounterHiRes(); lastRub = e.position; lastRubT = pressStart; }
        if (mode == mNone) return;
        proc.erosion.on = true; refresh();
        if (mode == mLine) mouseDrag (e);
    }
    void mouseDrag (const MouseEvent& e) override
    {
        const auto S = slab().toFloat();
        if (mode == mLine) { proc.erosion.sensitivity = jlimit (0.0f, 1.0f, ((e.position.y - S.getY()) / S.getHeight() - 0.12f) / 0.5f); repaint(); return; }
        if (mode == mPress && e.position.getDistanceFrom (pressAt) > 10.0f) { mode = mRub; pressGlow = 0; }
        if (mode == mRub)
        {
            const double now = Time::getMillisecondCounterHiRes();
            const float d = e.position.getDistanceFrom (lastRub), dt = (float) jmax (1.0, now - lastRubT);
            if (d > 2.0f)
            {
                const float speed = d / dt * 1000.0f;   // px / s
                rubSpeed += (speed - rubSpeed) * 0.15f;
                const float target = jlimit (0.0f, 1.0f, (rubSpeed - 150.0f) / 2200.0f);
                proc.erosion.recovery = proc.erosion.recovery.load() + (target - proc.erosion.recovery.load()) * 0.08f;
                preview = jmax (0.0f, preview - 0.004f * (0.3f + proc.erosion.recovery.load()));
                rubTrail.push_back ({ e.position, 1.0f }); if (rubTrail.size() > 50) rubTrail.erase (rubTrail.begin());
                lastRub = e.position; lastRubT = now;
            }
            repaint();
        }
    }
    void mouseUp (const MouseEvent&) override { mode = mNone; }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        if (! slab().contains (e.getPosition())) return;
        proc.erosion.mix = jlimit (0.0f, 1.0f, proc.erosion.mix.load() + w.deltaY * 0.25f); repaint();
    }
private:
    enum Mode { mNone, mLine, mPress, mRub };
    struct Crack { Point<float> a, b; float born, w; };
    struct Bubble { Point<float> p; float born, r, ph; };
    struct Ray { float a, len; };
    struct Rub { Point<float> p; float life; };
    Rectangle<int> slab() const { return { 24, 100, getWidth() - 24 - 470, getHeight() - 140 }; }
    Rectangle<int> column() const { return { getWidth() - 440, 100, 416, getHeight() - 140 }; }
    float lineY() const { const auto S = slab().toFloat(); return S.getY() + S.getHeight() * (0.12f + 0.5f * proc.erosion.sensitivity.load()); }
    bool nearLine (Point<float> p) const { return slab().toFloat().contains (p) && std::abs (p.y - lineY()) < 14.0f; }
    void refresh() { onBtn.selected = proc.erosion.on.load(); onBtn.setButtonText (proc.erosion.on.load() ? "ON" : "OFF"); onBtn.repaint(); repaint(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f;
        const bool on = proc.erosion.on.load();
        fat += ((on ? proc.erosion.mFatigue.load() : 0.0f) - fat) * 0.3f; starve += ((on ? proc.erosion.mStarve.load() : 0.0f) - starve) * 0.3f;
        level += ((on ? proc.erosion.mLevel.load() : -100.0f) - level) * 0.4f;
        if (mode == mPress)
        {
            const float held = (float) (Time::getMillisecondCounterHiRes() - pressStart) / 1000.0f;
            if (held > 0.25f) { proc.erosion.fatigue = jlimit (0.0f, 1.0f, (held - 0.25f) / 3.0f); pressGlow = jmin (1.0f, pressGlow + 0.15f); preview = jmax (preview, 0.15f + 0.75f * proc.erosion.fatigue.load()); }
        }
        else { pressGlow *= 0.9f; preview = jmax (0.0f, preview - 0.004f); }
        for (auto& r : rubTrail) r.life *= 0.93f;
        while (! rubTrail.empty() && rubTrail.front().life < 0.01f) rubTrail.erase (rubTrail.begin());
        repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf };
    std::vector<Crack> cracks; std::vector<Bubble> bubbles; std::vector<Ray> pressRays; std::vector<Rub> rubTrail;
    float fat = 0, starve = 0, level = -100, phase = 0, preview = 0, pressGlow = 0, rubSpeed = 0;
    Mode mode = mNone; Point<float> pressAt, lastRub; double pressStart = 0, lastRubT = 0; bool lineHot = false;
};
