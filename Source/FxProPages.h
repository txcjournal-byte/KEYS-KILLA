// v0.42 EVOLVE FX PRO pages (included by FxMainPage.h): REMIX REEL, DIAL-UP, WARP DRIVE, DOODLE, FINAL BOSS

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
