// v0.42 EVOLVE FX PRO pages (included by FxMainPage.h): REMIX REEL, DIAL-UP, WARP DRIVE, DOODLE, FINAL BOSS
// v0.43 organic pages: LIQUID, INTENT, EROSION

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

// ---------------------------------------------------------------- LIQUID ----------------------------------------------------------------
class LiquidPage : public Component, private Timer
{
public:
    LiquidPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "LIQUID on / off - the kick carves its own hole in the track, the bass flows around it", [this] { proc.liquid.on = ! proc.liquid.on.load(); refresh(); });
        btn (scBtn, "SIDECHAIN", "KICK SOURCE: the kick routed into this plugin's sidechain input", [this] { proc.liquid.source = kk::pro::lsSidechain; proc.liquid.on = true; refresh(); });
        btn (selfBtn, "SELF", "KICK SOURCE: no sidechain - the kicks in this track's own low end", [this] { proc.liquid.source = kk::pro::lsSelf; proc.liquid.on = true; refresh(); });
        const Colour a (0xff22d3ee), b2 (0xffff3b8a);
        auto pct = [] (double v) { return String (roundToInt (v * 100)) + " %"; };
        auto k = [&] (std::atomic<float>& v, const char* n, double def, std::function<String (double)> txt, const String& tip)
        { knobs.push_back (std::make_unique<RackKnob> (v, n, 0, 1, def, a, b2)); knobs.back()->valueText = std::move (txt); knobs.back()->setTooltip (tip); addAndMakeVisible (*knobs.back()); };
        k (proc.liquid.flow, "FLOW", 0.6, pct, "How much of the carved-out bass flows one band up (an octave higher) - the bass stays audible");
        k (proc.liquid.depth, "DEPTH", 0.85, pct, "How deep the hole is where the kick lands");
        k (proc.liquid.viscosity, "VISCOSITY", 0.35, [] (double v) { return String (roundToInt (30.0 * std::pow (25.0, v))) + " ms"; }, "How fast the liquid closes the hole again: water (fast) .. mercury (slow)");
        k (proc.liquid.mix, "MIX", 1.0, pct, "Dry / wet");
        for (int b = 0; b < kk::pro::LiquidState::numBands; ++b) hole[(size_t) b] = proc.liquid.mHole[(size_t) b].load();
        kick = proc.liquid.mKick.load();
        startTimerHz (30);
        refresh();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const Colour cTrack (0xff22d3ee), cKick (0xffff3b8a);
        proHeader (g, *this, "LIQUID", "ORGANIC ERA", "the kick does not turn the bass down - it carves its own hole in the low end, and the bass flows around it like mercury", cTrack);
        const auto V = vessel().toFloat();
        const auto R = V.reduced (14, 10).withTrimmedBottom (30);
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
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (glass);
            const float yBase = R.getY() + R.getHeight() * 0.3f;
            Path track, kickP, surf;
            const int steps = 160;
            for (int s = 0; s <= steps; ++s)
            {
                const float x = R.getX() + R.getWidth() * (float) s / (float) steps;
                const float y = yBase + std::sin (x * 0.02f + phase * 2.0f) * 4.0f + std::sin (x * 0.047f - phase * 1.3f) * 2.5f + at (holes, x) * R.getHeight() * 0.42f - at (bulge, x) * R.getHeight() * 0.14f;
                if (s == 0) { track.startNewSubPath (x, R.getBottom()); surf.startNewSubPath (x, y); } else surf.lineTo (x, y);
                track.lineTo (x, y);
                const float ky = R.getBottom() - at (holes, x) * R.getHeight() * 0.5f - 6.0f * kick + std::sin (x * 0.03f + phase * 3.0f) * 3.0f * at (holes, x);
                if (s == 0) kickP.startNewSubPath (x, R.getBottom() + 2);
                kickP.lineTo (x, ky);
            }
            track.lineTo (R.getRight(), R.getBottom()); track.closeSubPath();
            kickP.lineTo (R.getRight(), R.getBottom() + 2); kickP.closeSubPath();
            g.setGradientFill (ColourGradient (cTrack.withAlpha (0.85f), 0, yBase, Colour (0xff0b2f5a), 0, R.getBottom(), false)); g.fillPath (track);
            g.setGradientFill (ColourGradient (cKick.brighter (0.3f), 0, R.getBottom() - R.getHeight() * 0.5f, Colour (0xff5a0a2e), 0, R.getBottom(), false)); g.fillPath (kickP);
            g.setColour (cKick.withAlpha (0.35f)); g.strokePath (kickP, PathStrokeType (6.0f));
            g.setColour (Colours::white.withAlpha (0.75f)); g.strokePath (surf, PathStrokeType (2.0f));
            // the spill: drops flow from each hole into the band above
            Random rr (11);
            for (int b = 0; b < nb; ++b)
                for (int d = 0; d < 9; ++d)
                {
                    const float amt = hole[(size_t) b] * flow;
                    if (amt < 0.04f) break;
                    const float u = std::fmod (rr.nextFloat() + phase * 0.35f, 1.0f);
                    const float x0 = R.getX() + rw * ((float) b + 0.5f), x1 = x0 + rw;
                    const float x = x0 + (x1 - x0) * u, y = yBase + R.getHeight() * 0.4f * hole[(size_t) b] * (1.0f - u) - std::sin (u * MathConstants<float>::pi) * R.getHeight() * 0.07f * amt - 6.0f;
                    const float r = 3.0f + 5.0f * rr.nextFloat() * amt;
                    g.setColour (cTrack.brighter (0.5f).withAlpha (jmin (1.0f, amt) * (1.0f - u * 0.6f))); g.fillEllipse (x - r, y - r, 2 * r, 2 * r);
                }
            // rising bubbles in the track liquid
            for (int i = 0; i < 40; ++i)
            {
                const float u = std::fmod (rr.nextFloat() + phase * (0.05f + 0.1f * rr.nextFloat()), 1.0f), x = R.getX() + rr.nextFloat() * R.getWidth();
                const float y = R.getBottom() - u * (R.getBottom() - yBase - 10.0f), r = 1.5f + 2.5f * rr.nextFloat();
                g.setColour (Colours::white.withAlpha (0.25f * (1.0f - u))); g.drawEllipse (x - r, y - r, 2 * r, 2 * r, 1.0f);
            }
            // band borders
            for (int b = 1; b < regions; ++b) { g.setColour (Colours::white.withAlpha (0.08f)); g.fillRect (R.getX() + rw * (float) b, R.getY(), 1.0f, R.getHeight()); }
        }
        g.setColour (Colours::white.withAlpha (0.35f)); g.strokePath (glass, PathStrokeType (2.5f));
        g.setColour (Colours::white.withAlpha (0.08f)); g.fillRoundedRectangle (R.getX() + 18, R.getY() + 10, 10, R.getHeight() * 0.7f, 5);
        // band names + how deep each one is carved
        static const char* names[] { "30-45 Hz", "45-70 Hz", "70-110 Hz", "110-180 Hz", "180-300 Hz", "300+ Hz" };
        for (int b = 0; b < regions; ++b)
        {
            const auto cell = Rectangle<float> (R.getX() + rw * (float) b, R.getBottom() + 8, rw, 18);
            g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.12f)); g.drawText (names[b], cell, Justification::centred);
            if (b < nb && hole[(size_t) b] > 0.02f) { g.setColour (Colours::white.withAlpha (0.9f)); g.setFont (kk::modern::font (13.0f, true, 0.05f)); g.drawText ("-" + String (roundToInt (hole[(size_t) b] * 100)) + "%", Rectangle<float> (cell.getX(), R.getBottom() - 30, rw, 20), Justification::centred); }
            if (b == nb && flow > 0.01f) { g.setColour (cTrack.brighter (0.4f)); g.setFont (kk::modern::font (11.0f, true, 0.12f)); g.drawText ("FLOWS HERE", Rectangle<float> (cell.getX(), R.getY() + 12, rw, 16), Justification::centred); }
        }
        // the right column
        const auto C = column();
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText ("KICK SOURCE", C.getX(), C.getY(), 200, 14, Justification::centredLeft);
        const bool sc = proc.liquid.source.load() == kk::pro::lsSidechain, live = proc.liquid.mSidechain.load();
        g.setColour (sc && ! live && proc.liquid.on.load() ? Colour (0xffffb020) : t.text.withAlpha (0.8f)); g.setFont (kk::modern::font (12.5f, true, 0.03f));
        g.drawFittedText (! sc ? String ("SELF: the kicks in this track's own low end") : live ? String ("SIDECHAIN: kick arriving") : String ("SIDECHAIN: nothing arriving yet - route the kick in (below)"), C.getX(), C.getY() + 66, C.getWidth(), 18, Justification::centredLeft, 1, 0.8f);
        // kick meter
        const auto km = Rectangle<float> ((float) C.getX(), (float) C.getY() + 440, (float) C.getWidth(), 14);
        g.setColour (Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (km, 7);
        g.setGradientFill (ColourGradient (cKick.darker (0.3f), km.getX(), 0, cKick.brighter (0.4f), km.getRight(), 0, false)); g.fillRoundedRectangle (km.withWidth (km.getWidth() * jlimit (0.0f, 1.0f, kick)), 7);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText ("KICK", (int) km.getX(), (int) km.getY() - 18, 100, 14, Justification::centredLeft);
        // how to route the kick (FL Studio mixer)
        const auto how = Rectangle<float> ((float) C.getX(), (float) C.getY() + 476, (float) C.getWidth(), (float) (V.getBottom() - C.getY() - 476));
        kk::modern::well (g, how, 12.0f);
        g.setColour (cTrack); g.setFont (kk::modern::font (13.0f, true, 0.25f));
        g.drawText ("ROUTE THE KICK IN", how.reduced (18, 14).removeFromTop (18), Justification::centredLeft);
        g.setColour (t.text); g.setFont (kk::modern::font (15.0f, true, 0.02f));
        g.drawFittedText ("1.  put EVOLVE FX PRO on the bass (or the whole beat) track\n"
                          "2.  in the FL mixer select the KICK track\n"
                          "3.  right-click the send arrow under the track with this plugin  ->  \"Sidechain to this track\"\n"
                          "4.  KICK SOURCE: SIDECHAIN.   no routing?  SELF hears the kicks in the track itself",
                          how.reduced (18, 14).withTrimmedTop (28).withHeight (150).toNearestInt(), Justification::topLeft, 9, 0.9f);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.0f, true, 0.02f));
        g.drawFittedText ("DEPTH  how deep the kick carves\nFLOW  how much of the bass flows one octave up\nVISCOSITY  water closes the hole fast, mercury slowly",
                          how.reduced (18, 14).withTrimmedTop (150).toNearestInt(), Justification::topLeft, 4, 0.9f);
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32);
        const auto C = column();
        scBtn.setBounds (C.getX(), C.getY() + 20, C.getWidth() / 2 - 4, 40); selfBtn.setBounds (C.getX() + C.getWidth() / 2 + 4, C.getY() + 20, C.getWidth() / 2 - 4, 40);
        const int kw = C.getWidth() / 2;
        for (int i = 0; i < (int) knobs.size(); ++i) knobs[(size_t) i]->setBounds (C.getX() + (i % 2) * kw, C.getY() + 100 + (i / 2) * 160, kw - 8, 150);
    }
private:
    Rectangle<int> vessel() const { return { 24, 100, getWidth() - 24 - 470, getHeight() - 140 }; }
    Rectangle<int> column() const { return { getWidth() - 440, 100, 416, getHeight() - 140 }; }
    void refresh()
    {
        onBtn.selected = proc.liquid.on.load(); onBtn.setButtonText (proc.liquid.on.load() ? "ON" : "OFF"); onBtn.repaint();
        scBtn.selected = proc.liquid.source.load() == kk::pro::lsSidechain; selfBtn.selected = ! scBtn.selected; scBtn.repaint(); selfBtn.repaint();
        for (auto& k : knobs) k->sync();
        repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.04f;
        const float visc = proc.liquid.viscosity.load();
        for (int b = 0; b < kk::pro::LiquidState::numBands; ++b) { const float m = proc.liquid.mHole[(size_t) b].load(); hole[(size_t) b] += (m - hole[(size_t) b]) * (m > hole[(size_t) b] ? 0.7f : 0.35f - 0.27f * visc); }
        kick += (proc.liquid.mKick.load() - kick) * 0.4f;
        repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf }, scBtn { lnf }, selfBtn { lnf };
    std::vector<std::unique_ptr<RackKnob>> knobs;
    std::array<float, kk::pro::LiquidState::numBands> hole {};
    float kick = 0, phase = 0;
};

// ---------------------------------------------------------------- INTENT ----------------------------------------------------------------
class IntentPage : public Component, private Timer
{
public:
    IntentPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "INTENT on / off - one breath moves rasp, width, filter, reverb freeze, tremor and attack together", [this] { proc.intent.on = ! proc.intent.on.load(); refresh(); });
        static const char* tips[] { "Oxygen drains: the sound gets a raspy throat, closes into mono, the air freezes",
                                    "Aggression: harder attacks, a growl, a tighter image", "Stress: it shakes, the pitch jitters, the room closes in",
                                    "Calm: soft attacks, a wide warm room, a darker top" };
        for (int m = 0; m < kk::pro::numIntentModes; ++m)
        {
            auto b = std::make_unique<HotButton> (lnf, kk::pro::intentModeName (m)); b->framed = true; b->setTooltip (tips[m]);
            b->onClick = [this, m] { proc.intent.mode = m; proc.intent.on = true; refresh(); };
            addAndMakeVisible (*b); chips.push_back (std::move (b));
        }
        mix = std::make_unique<RackKnob> (proc.intent.mix, "MIX", 0, 1, 1, stateColour (0), Colour (0xffa78bfa)); addAndMakeVisible (*mix);
        breath = proc.intent.level.load();
        startTimerHz (30);
        refresh();
    }
    static Colour stateColour (int m) { static const uint32 c[] { 0xff38bdf8, 0xffff4d2e, 0xffd4f542, 0xffa78bfa }; return Colour (c[jlimit (0, 3, m)]); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const int mode = proc.intent.mode.load();
        const auto col = stateColour (mode);
        proHeader (g, *this, "INTENT", "ORGANIC ERA", "one state, one breath - it moves many muscles at once: rasp, width, filter, frozen air, tremor, attack.  drag the breath down", col);
        kk::pro::IntentDsp::Targets tg; kk::pro::IntentDsp::mapping (mode, breath, tg);
        const bool on = proc.intent.on.load();
        const float rasp = on ? proc.intent.mRasp.load() : tg.rasp, width = on ? proc.intent.mWidth.load() : tg.width, freeze = on ? proc.intent.mFreeze.load() : tg.freeze;
        const float hz = on ? proc.intent.mFilterHz.load() : tg.lpHz, trem = on ? proc.intent.mTremor.load() : tg.tremor;
        // the lungs
        const auto A = lungArea().toFloat();
        kk::modern::well (g, A, 16.0f);
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (A.toNearestInt());
            Random jr ((int64) (phase * 30.0f));
            const auto c = A.getCentre().translated (trem * 4.0f * (jr.nextFloat() - 0.5f), A.getHeight() * 0.06f + trem * 4.0f * (jr.nextFloat() - 0.5f));
            const float s = jmin (A.getWidth(), A.getHeight()) * 0.5f;
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
                    if (ay < 0) x *= 0.75f + 0.25f * (1.0f + ay);   // narrower at the top
                    if (inner && ay > 0.1f) x *= 1.0f - 0.35f * ay;   // the heart's notch
                    if (i == 0) lobe.startNewSubPath (lc.x + x, lc.y + y); else lobe.lineTo (lc.x + x, lc.y + y);
                }
                lobe.closeSubPath();
                g.setGradientFill (ColourGradient (lobeCol.brighter (0.35f).withAlpha (0.95f), lc.x - (float) side * rx * 0.3f, lc.y - ry * 0.4f, lobeCol.darker (0.8f).withAlpha (0.9f), lc.x + (float) side * rx, lc.y + ry, true));
                g.fillPath (lobe);
                g.setColour (lobeCol.brighter (0.6f).withAlpha (0.8f)); g.strokePath (lobe, PathStrokeType (2.0f + 2.0f * pulse));
                // bronchi: branches that open as the breath fills
                Random br (side > 0 ? 7 : 13);
                std::function<void (Point<float>, float, float, int)> branch = [&] (Point<float> p0, float ang, float len, int depth)
                {
                    const Point<float> p1 (p0.x + std::sin (ang) * len, p0.y + std::cos (ang) * len);
                    g.setColour (Colours::white.withAlpha (0.18f + 0.1f * (float) depth)); g.drawLine (Line<float> (p0, p1), 0.8f + 1.2f * (float) depth);
                    if (depth > 0) for (int k = 0; k < 2; ++k) branch (p1, ang + (k == 0 ? -1.0f : 1.0f) * (0.35f + 0.3f * br.nextFloat()), len * 0.68f, depth - 1);
                };
                const Point<float> hilum (c.x + (float) side * sep * 0.35f, c.y - s * 0.2f);
                branch (hilum, (float) side * 0.55f, s * 0.2f * swell, 4);
                // frost when the air freezes
                if (freeze > 0.02f)
                    for (int f = 0; f < 9; ++f)
                    {
                        const Point<float> fp (lc.x + (br.nextFloat() - 0.5f) * rx * 1.1f, lc.y + (br.nextFloat() - 0.5f) * ry * 1.4f);
                        const float fr = (6.0f + 12.0f * br.nextFloat()) * freeze;
                        g.setColour (Colours::white.withAlpha (0.75f * freeze));
                        for (int k = 0; k < 6; ++k) { const float a = (float) k * MathConstants<float>::pi / 3.0f; g.drawLine (fp.x, fp.y, fp.x + std::cos (a) * fr, fp.y + std::sin (a) * fr, 1.2f); }
                    }
            }
            // the trachea
            Path tr; tr.startNewSubPath (c.x, A.getY() + 10); tr.lineTo (c.x, c.y - s * 0.28f);
            tr.quadraticTo (c.x, c.y - s * 0.18f, c.x - sep * 0.35f, c.y - s * 0.2f); tr.startNewSubPath (c.x, c.y - s * 0.28f); tr.quadraticTo (c.x, c.y - s * 0.18f, c.x + sep * 0.35f, c.y - s * 0.2f);
            g.setColour (lobeCol.withAlpha (0.55f)); g.strokePath (tr, PathStrokeType (s * 0.07f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setColour (Colours::white.withAlpha (0.25f));
            for (int r = 0; r < 7; ++r) { const float y = A.getY() + 24 + (float) r * (c.y - s * 0.3f - A.getY() - 24) / 7.0f; g.fillRoundedRectangle (c.x - s * 0.045f, y, s * 0.09f, 3.0f, 1.5f); }
        }
        g.setColour (col); g.setFont (kk::modern::font (16.0f, true, 0.3f));
        g.drawText (String (kk::pro::intentModeName (mode)) + (on ? "" : "   (off)"), A.reduced (22, 16).removeFromTop (22), Justification::centredLeft);
        // the breath fader
        const auto F = fader().toFloat();
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText ("BREATH", F.withHeight (14).translated (0, -20), Justification::centred);
        g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (F, F.getWidth() * 0.5f);
        const auto fill = F.reduced (6).withTrimmedTop ((F.getHeight() - 12) * (1.0f - breath));
        g.setGradientFill (ColourGradient (col.brighter (0.4f), 0, fill.getY(), col.darker (0.7f), 0, fill.getBottom(), false)); g.fillRoundedRectangle (fill, (F.getWidth() - 12) * 0.5f);
        for (int k = 1; k < 10; ++k) { g.setColour (Colours::white.withAlpha (0.12f)); g.fillRect (F.getX() + 10, F.getY() + F.getHeight() * (float) k / 10.0f, F.getWidth() - 20, 1.0f); }
        g.setColour (Colours::white); g.fillEllipse (F.getCentreX() - 16, fill.getY() - 4, 32, 12);
        g.setColour (t.text); g.setFont (kk::modern::font (28.0f, true, 0.0f)); g.drawText (String (roundToInt (breath * 100)), F.withHeight (40).translated (0, F.getHeight() + 6), Justification::centred);
        // what the muscles do
        const auto M = muscles();
        auto readout = [&] (int i, const String& name, const String& val, float u)
        {
            const auto r = Rectangle<float> ((float) M.getX(), (float) M.getY() + (float) i * 92.0f, (float) M.getWidth(), 80.0f);
            kk::modern::well (g, r, 10.0f);
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText (name, r.reduced (14, 10).removeFromTop (14), Justification::centredLeft);
            g.setColour (t.text); g.setFont (kk::modern::font (26.0f, true, 0.0f)); g.drawText (val, r.reduced (14, 8).withTrimmedTop (18).withHeight (32), Justification::centredLeft);
            const auto bar = r.reduced (14, 0).withTrimmedTop (r.getHeight() - 14).withHeight (5);
            g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (bar, 2.5f);
            g.setColour (col); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * jlimit (0.0f, 1.0f, u)), 2.5f);
        };
        readout (0, "RASP", String (roundToInt (rasp * 100)) + " %", rasp);
        readout (1, "WIDTH", String (roundToInt (width * 100)) + " %", width / 1.5f);
        readout (2, "FREEZE", String (roundToInt (freeze * 100)) + " %", freeze);
        readout (3, "FILTER", hz >= 19500.0f ? String ("OPEN") : hz >= 1000.0f ? String (hz / 1000.0f, 1) + " kHz" : String (roundToInt (hz)) + " Hz", std::log (hz / 20.0f) / std::log (1000.0f));
        readout (4, "TREMOR", String (roundToInt (trem * 100)) + " %", trem);
        static const char* what[] { "pull the breath down: the throat rasps, the image closes into mono, the air freezes",
                                    "pull the breath down: the hits bite harder, a growl, a tighter image",
                                    "pull the breath down: it shakes, the pitch jitters, the room closes in",
                                    "pull the breath down: soft attacks, a wide warm room, a darker top" };
        g.setColour (col); g.setFont (kk::modern::font (12.0f, true, 0.25f));
        g.drawText (kk::pro::intentModeName (mode), M.getX(), M.getY() + 470, M.getWidth(), 16, Justification::centredLeft);
        g.setColour (t.text); g.setFont (kk::modern::font (14.5f, true, 0.02f));
        g.drawFittedText (what[jlimit (0, 3, mode)], M.getX(), M.getY() + 490, M.getWidth(), 80, Justification::topLeft, 4, 0.9f);
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32);
        const auto A = lungArea();
        const int cw = A.getWidth() / (int) chips.size();
        for (int i = 0; i < (int) chips.size(); ++i) chips[(size_t) i]->setBounds (A.getX() + i * cw, 100, cw - 8, 40);
        const auto M = muscles();
        mix->setBounds (M.getX() + M.getWidth() / 2 - 70, getHeight() - 40 - 150, 140, 150);
    }
    void mouseDown (const MouseEvent& e) override { if (fader().expanded (10).contains (e.getPosition())) { dragging = true; setLevel (e.position.y); } }
    void mouseDrag (const MouseEvent& e) override { if (dragging) setLevel (e.position.y); }
    void mouseUp (const MouseEvent&) override { dragging = false; }
    void mouseDoubleClick (const MouseEvent& e) override { if (fader().expanded (10).contains (e.getPosition())) { proc.intent.level = 1.0f; breath = 1.0f; repaint(); } }
private:
    Rectangle<int> lungArea() const { return { 24, 154, getWidth() - 24 - 470, getHeight() - 154 - 40 }; }
    Rectangle<int> fader() const { return { getWidth() - 440, 124, 110, getHeight() - 124 - 100 }; }
    Rectangle<int> muscles() const { return { getWidth() - 306, 100, 282, 452 }; }
    void setLevel (float y)
    {
        const auto F = fader().toFloat();
        proc.intent.level = jlimit (0.0f, 1.0f, 1.0f - (y - F.getY()) / F.getHeight()); proc.intent.on = true;
        breath = proc.intent.level.load(); refresh();
    }
    void refresh()
    {
        onBtn.selected = proc.intent.on.load(); onBtn.setButtonText (proc.intent.on.load() ? "ON" : "OFF"); onBtn.repaint();
        for (int i = 0; i < (int) chips.size(); ++i) { chips[(size_t) i]->selected = proc.intent.mode.load() == i; chips[(size_t) i]->repaint(); }
        mix->sync(); repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f; breath += (proc.intent.level.load() - breath) * 0.3f;
        pulse = std::max (pulse * 0.85f, jmin (1.0f, proc.intent.mPulse.load() * (proc.intent.on.load() ? 1.5f : 0.0f)));
        repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> chips;
    std::unique_ptr<RackKnob> mix;
    float breath = 1, phase = 0, pulse = 0; bool dragging = false;
};

// ---------------------------------------------------------------- EROSION ----------------------------------------------------------------
class ErosionPage : public Component, private Timer
{
public:
    ErosionPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (onBtn, "OFF", "EROSION on / off - the sound tires when you push it, sinks into rumble when you starve it", [this] { proc.erosion.on = ! proc.erosion.on.load(); refresh(); });
        const Colour a (0xffff8a3d), b2 (0xff8b5a2b);
        auto pct = [] (double v) { return String (roundToInt (v * 100)) + " %"; };
        auto k = [&] (std::atomic<float>& v, const char* n, double def, std::function<String (double)> txt, const String& tip)
        { knobs.push_back (std::make_unique<RackKnob> (v, n, 0, 1, def, a, b2)); knobs.back()->valueText = std::move (txt); knobs.back()->setTooltip (tip); addAndMakeVisible (*knobs.back()); };
        k (proc.erosion.sensitivity, "SENSITIVITY", 0.5, [] (double v) { return String (kk::pro::ErosionState::thresholdDb ((float) v), 0) + " dB"; }, "Where the load starts: higher = it tires earlier (and starves later)");
        k (proc.erosion.fatigue, "FATIGUE", 0.5, pct, "How fast it tires under load");
        k (proc.erosion.recovery, "RECOVERY", 0.5, [] (double v) { return String (12.0 * std::pow (0.05, v), 1) + " s"; }, "How long the material needs to recover");
        k (proc.erosion.mix, "MIX", 1.0, pct, "Dry / wet");
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
        fat = proc.erosion.mFatigue.load(); starve = proc.erosion.mStarve.load(); level = proc.erosion.mLevel.load();
        startTimerHz (30);
        refresh();
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const Colour col (0xffff8a3d);
        proHeader (g, *this, "EROSION", "ORGANIC ERA", "push it and it tires: the highs wear off, bubbles crackle.  starve it and it sinks into rumble.  silence stays silent", col);
        const auto S = slab().toFloat();
        const float sens = proc.erosion.sensitivity.load(), thr = kk::pro::ErosionState::thresholdDb (sens), flo = kk::pro::ErosionState::floorDb (sens);
        const float heat = jlimit (0.0f, 1.0f, (level - thr) / 6.0f);
        // the material
        g.setGradientFill (ColourGradient (Colour (0xff8a8f98), S.getX(), S.getY(), Colour (0xff3d424b), S.getRight(), S.getBottom(), false));
        g.fillRoundedRectangle (S, 14);
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (S.toNearestInt());
            Random gr (99);
            for (int i = 0; i < 900; ++i) { g.setColour (Colours::black.withAlpha (0.06f + 0.1f * gr.nextFloat())); g.fillRect (S.getX() + gr.nextFloat() * S.getWidth(), S.getY() + gr.nextFloat() * S.getHeight(), 2.0f, 2.0f); }
            if (heat > 0) { g.setGradientFill (ColourGradient (col.withAlpha (0.45f * heat), S.getCentreX(), S.getY(), col.withAlpha (0.0f), S.getCentreX(), S.getY() + S.getHeight() * 0.5f, false)); g.fillRect (S); }
            // the highs wear off: the surface loses its shine as it tires
            g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.22f * (1.0f - fat)), S.getX(), S.getY(), Colours::white.withAlpha (0.0f), S.getX() + S.getWidth() * 0.4f, S.getY() + S.getHeight() * 0.4f, false)); g.fillRect (S);
            auto px = [&S] (Point<float> n) { return Point<float> (S.getX() + n.x * S.getWidth(), S.getY() + n.y * S.getHeight()); };
            for (auto& c : cracks)
                if (c.born < fat)
                {
                    const float grow = jlimit (0.0f, 1.0f, (fat - c.born) * 40.0f);
                    const auto a = px (c.a), b = a + (px (c.b) - a) * grow;
                    g.setColour (Colour (0xff120c08).withAlpha (0.85f)); g.drawLine (Line<float> (a, b), 1.0f + 3.0f * c.w * jmin (1.0f, fat * 1.5f));
                    g.setColour (col.withAlpha (0.35f * heat)); g.drawLine (Line<float> (a, b), 0.8f);
                }
            for (auto& b : bubbles)
                if (b.born < fat)
                {
                    const float pop = std::fmod (phase * (0.6f + b.ph) + b.ph * 7.0f, 1.0f);
                    const float r = b.r * S.getWidth() * (0.4f + 0.8f * pop) * jmin (1.0f, (fat - b.born) * 6.0f);
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
                for (int w = 0; w < 3; ++w)   // rumble waves
                {
                    Path rw; const float y0 = top + 30.0f + (float) w * 26.0f;
                    for (int i = 0; i <= 60; ++i) { const float u = (float) i / 60.0f, y = y0 + std::sin (u * 5.0f + phase * (0.8f + 0.3f * (float) w)) * 6.0f * starve; if (i == 0) rw.startNewSubPath (S.getX() + u * S.getWidth(), y); else rw.lineTo (S.getX() + u * S.getWidth(), y); }
                    g.setColour (Colour (0xffb08a5a).withAlpha (0.2f * starve)); g.strokePath (rw, PathStrokeType (1.5f));
                }
            }
        }
        g.setColour (Colours::black.withAlpha (0.5f)); g.drawRoundedRectangle (S, 14, 2.0f);
        g.setColour (Colours::white.withAlpha (0.9f)); g.setFont (kk::modern::font (15.0f, true, 0.25f));
        g.drawText (fat > 0.6f ? "WORN OUT" : fat > 0.2f ? "TIRING" : starve > 0.3f ? "SINKING" : level < kk::pro::ErosionDsp::gateDb ? "AT REST" : "HOLDING", S.reduced (20, 16).removeFromTop (20), Justification::centredLeft);
        // meters
        const auto C = column();
        auto meter = [&] (int row, const String& name, float u, Colour c, const String& val)
        {
            const auto r = Rectangle<float> ((float) C.getX(), (float) C.getY() + (float) row * 62.0f + 20.0f, (float) C.getWidth(), 16.0f);
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f)); g.drawText (name, (int) r.getX(), (int) r.getY() - 18, 200, 14, Justification::centredLeft);
            g.setColour (t.text); g.setFont (kk::modern::font (12.5f, true, 0.05f)); g.drawText (val, (int) r.getRight() - 120, (int) r.getY() - 18, 120, 14, Justification::centredRight);
            g.setColour (Colours::black.withAlpha (0.45f)); g.fillRoundedRectangle (r, 8);
            g.setGradientFill (ColourGradient (c.darker (0.4f), r.getX(), 0, c.brighter (0.3f), r.getRight(), 0, false)); g.fillRoundedRectangle (r.withWidth (r.getWidth() * jlimit (0.0f, 1.0f, u)), 8);
            return r;
        };
        meter (0, "FATIGUE", fat, col, String (roundToInt (fat * 100)) + " %");
        meter (1, "STARVE", starve, Colour (0xffb08a5a), String (roundToInt (starve * 100)) + " %");
        const auto lr = meter (2, "INPUT LOAD", (level + 80.0f) / 80.0f, Colour (0xff9ca3af), level > -99.0f ? String (level, 1) + " dB" : String ("--"));
        auto mark = [&] (float db, Colour c) { const float x = lr.getX() + lr.getWidth() * jlimit (0.0f, 1.0f, (db + 80.0f) / 80.0f); g.setColour (c); g.fillRect (x - 1.0f, lr.getY() - 4, 2.0f, lr.getHeight() + 8); };
        mark (thr, col); mark (flo, Colour (0xffb08a5a)); mark (kk::pro::ErosionDsp::gateDb, t.dim);
        g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.1f));
        g.drawText ("silence  |  starves below " + String (flo, 0) + "  |  tires above " + String (thr, 0) + " dB", (int) lr.getX(), (int) lr.getBottom() + 6, (int) lr.getWidth(), 14, Justification::centredLeft);
        // what it does
        const auto how = Rectangle<float> ((float) C.getX(), (float) C.getY() + 540, (float) C.getWidth(), (float) (S.getBottom() - C.getY() - 540));
        if (how.getHeight() > 60)
        {
            kk::modern::well (g, how, 12.0f);
            g.setColour (t.text); g.setFont (kk::modern::font (15.0f, true, 0.02f));
            g.drawFittedText ("LOUD:  the top end wears off and comes back slowly, cavitation bubbles crackle - more the harder you push\n"
                              "QUIET:  a subsonic rumble rises under it, the phase starts to drift\n"
                              "SILENCE:  nothing - the material rests",
                              how.reduced (18, 14).withHeight (150).toNearestInt(), Justification::topLeft, 8, 0.9f);
            g.setColour (t.dim); g.setFont (kk::modern::font (13.0f, true, 0.02f));
            g.drawFittedText ("SENSITIVITY  where the load starts\nFATIGUE  how fast it tires\nRECOVERY  how fast it heals",
                              how.reduced (18, 14).withTrimmedTop (112).toNearestInt(), Justification::topLeft, 4, 0.9f);
        }
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32);
        const auto C = column();
        const int kw = C.getWidth() / 2;
        for (int i = 0; i < (int) knobs.size(); ++i) knobs[(size_t) i]->setBounds (C.getX() + (i % 2) * kw, C.getY() + 210 + (i / 2) * 160, kw - 8, 150);
    }
private:
    struct Crack { Point<float> a, b; float born, w; };
    struct Bubble { Point<float> p; float born, r, ph; };
    Rectangle<int> slab() const { return { 24, 100, getWidth() - 24 - 470, getHeight() - 140 }; }
    Rectangle<int> column() const { return { getWidth() - 440, 100, 416, getHeight() - 140 }; }
    void refresh() { onBtn.selected = proc.erosion.on.load(); onBtn.setButtonText (proc.erosion.on.load() ? "ON" : "OFF"); onBtn.repaint(); for (auto& k : knobs) k->sync(); repaint(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f;
        const bool on = proc.erosion.on.load();
        fat += ((on ? proc.erosion.mFatigue.load() : 0.0f) - fat) * 0.3f; starve += ((on ? proc.erosion.mStarve.load() : 0.0f) - starve) * 0.3f;
        level += ((on ? proc.erosion.mLevel.load() : -100.0f) - level) * 0.4f;
        repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf };
    std::vector<std::unique_ptr<RackKnob>> knobs;
    std::vector<Crack> cracks; std::vector<Bubble> bubbles;
    float fat = 0, starve = 0, level = -100, phase = 0;
};
