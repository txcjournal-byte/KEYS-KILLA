// v0.43 ALCHEMY page (included by PluginEditor.cpp): no presets to browse. 1 EXCITE (what hits / rubs / sparks the matter),
// 2 BODY (what resonates), 3 MATTER (drag: glass ... mud, tiny ... giant). The sound is on the keys at once.
// Also opened as the sound chooser (parents, the EVOLVE seed, the melody sound): then USE IT sends the sound back.
class AlchemyPage : public Component, private Timer
{
public:
    AlchemyPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (playBtn, "PLAY", "Hear it again (it is on your keys too)", [this] { if (cur.valid()) proc.alcUse (cur, true); });
        btn (mutateBtn, "NEW MUTATION", "The same recipe, another mutation of the matter", [this] { proc.alcSeed = (uint32) Random::getSystemRandom().nextInt() | 1u; make (true); });
        btn (plantBtn, "PLANT IN EVOLVE", "This sound becomes the seed of a new EVOLVE tree", [this] { if (cur.valid()) { proc.evoSeedGenome (cur); note = "planted - open EVOLVE"; repaint(); } });
        btn (saveBtn, "SAVE", "Save this sound into your folders / sound kits", [this]
        {
            if (! cur.valid()) return;
            const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
            auto snd = kk::PairLab::fromBuffer (proc.renderGenomeAudio (cur, rate, 3.0), rate, rate, cur.name);
            saveToFolderMenu (proc, { snd }, &saveBtn, [safe = SafePointer<AlchemyPage> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->repaint(); } });
        });
        btn (useBtn, "USE IT", "Use this sound", [this] { if (cur.valid() && onPicked) { auto fn = onPicked; onPicked = nullptr; useBtn.setVisible (false); fn(); } });
        useBtn.hero = true; useBtn.setVisible (false);
        btn (makeTab, "MAKE", "MAKE: what excites the matter and the body that resonates", [this] { setView (false); });
        btn (sculptTab, "SCULPT", "SCULPT: the sound is a lump of matter in your hands - stretch it, rub it, hold it, tear it", [this] { setView (true); });
        makeTab.selected = true;
        dragWav.makeFile = [this] { return cur.valid() ? proc.exportGenomeWav (cur) : File(); };
        dragWav.setTooltip ("Drag the sound into FL as a WAV");
        addAndMakeVisible (dragWav);
        cur = recipe();
        startTimerHz (30);
    }
    // the chooser mode: title + what happens with the sound
    void pick (const String& title, std::function<void()> fn) { pickTitle = title; onPicked = std::move (fn); useBtn.setVisible (onPicked != nullptr); resized(); repaint(); }
    void visibilityChanged() override { if (! isVisible() && onPicked) { onPicked = nullptr; useBtn.setVisible (false); } }

    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("ALCHEMY", 24, 12, 220, 40, Justification::centredLeft);
        g.setColour (pickTitle.isNotEmpty() && onPicked ? kk::accentText() : t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText (onPicked ? pickTitle + "  -  make it, then USE IT" : String ("no presets.  pick what EXCITES the matter, the BODY that resonates, then shape the MATTER.  it is on your keys at once"),
                    220, 18, getWidth() - 240, 28, Justification::centredLeft);
        const auto& ex = kk::alc::exciters(); const auto& bo = kk::alc::bodies();
        auto column = [&] (int col, const char* head, const std::vector<kk::alc::Part>& parts, int sel)
        {
            const auto c = colArea (col);
            g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (14.0f, true, 0.3f));
            g.drawText (head, c.getX(), c.getY() - 24, c.getWidth(), 20, Justification::centredLeft);
            for (int i = 0; i < (int) parts.size(); ++i)
            {
                const auto r = card (col, i).toFloat();
                const auto pc = Colour (parts[(size_t) i].colour);
                const bool on = i == sel, hov = col == hoverCol && i == hoverCard;
                g.setGradientFill (ColourGradient (pc.withAlpha (on ? 0.3f : hov ? 0.16f : 0.08f), r.getX(), r.getY(), Colours::black.withAlpha (0.2f), r.getRight(), r.getBottom(), false));
                g.fillRoundedRectangle (r, 12);
                g.setColour (on ? pc : pc.withAlpha (0.35f)); g.drawRoundedRectangle (r.reduced (0.5f), 12, on ? 2.2f : 1.0f);
                const auto ic = r.withWidth (r.getHeight()).reduced (14);
                drawPartIcon (g, col, i, ic, pc, on);
                g.setColour (on ? Colours::white : t.text); g.setFont (kk::modern::font (16.0f, true, 0.12f));
                g.drawText (parts[(size_t) i].name, r.withTrimmedLeft (r.getHeight()).withTrimmedTop (10).withHeight (24).toNearestInt(), Justification::centredLeft);
                g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.02f));
                g.drawFittedText (parts[(size_t) i].hint, r.withTrimmedLeft (r.getHeight()).withTrimmedTop (34).withTrimmedRight (8).toNearestInt(), Justification::topLeft, 2, 0.85f);
            }
        };
        if (sculpting) drawSculpt (g);
        else
        {
            column (0, "1   EXCITE", ex, proc.alcExc.load());
            column (1, "2   BODY", bo, proc.alcBody.load());
        }
        drawMatter (g);
        // the result
        const auto res = resultArea().toFloat();
        g.setColour (t.text); g.setFont (kk::modern::font (24.0f, true, 0.05f));
        g.drawFittedText (cur.valid() ? cur.name : String ("-"), res.withHeight (34).toNearestInt(), Justification::centredLeft, 1, 0.7f);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.1f));
        g.drawText (String (ex[(size_t) proc.alcExc.load()].name) + "  x  " + bo[(size_t) proc.alcBody.load()].name, res.withTrimmedTop (36).withHeight (18).toNearestInt(), Justification::centredLeft);
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (13.0f, true, 0.03f)); g.drawText (note, res.withTrimmedTop (120).withHeight (20).toNearestInt(), Justification::centredLeft); }
    }
    void resized() override
    {
        const auto res = resultArea();
        const int bw = (res.getWidth() - 20) / 3;
        playBtn.setBounds (res.getX(), res.getY() + 62, bw, 40); mutateBtn.setBounds (res.getX() + bw + 10, res.getY() + 62, bw, 40); plantBtn.setBounds (res.getX() + 2 * (bw + 10), res.getY() + 62, bw, 40);
        saveBtn.setBounds (res.getX(), res.getY() + 150, bw, 40); dragWav.setBounds (res.getX() + bw + 10, res.getY() + 148, bw, 44);
        useBtn.setBounds (res.getX() + 2 * (bw + 10), res.getY() + 148, bw, 44);
        makeTab.setBounds (getWidth() - 250, 14, 110, 34); sculptTab.setBounds (getWidth() - 134, 14, 110, 34);
    }
    void mouseMove (const MouseEvent& e) override
    {
        if (sculpting) return;
        int hc = -1, hk = -1;
        for (int col = 0; col < 2; ++col) for (int i = 0; i < 5; ++i) if (card (col, i).contains (e.getPosition())) { hc = col; hk = i; }
        if (hc != hoverCol || hk != hoverCard) { hoverCol = hc; hoverCard = hk; repaint(); }
    }
    void mouseExit (const MouseEvent&) override { hoverCol = hoverCard = -1; repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        if (sculpting && stage().contains (e.getPosition())) { sculptDown (e); return; }
        if (! sculpting)
        for (int col = 0; col < 2; ++col)
            for (int i = 0; i < 5; ++i)
                if (card (col, i).contains (e.getPosition())) { (col == 0 ? proc.alcExc : proc.alcBody) = i; make (true); return; }
        if (padArea().contains (e.getPosition())) { dragging = true; setMatter (e.position); }
    }
    void mouseDrag (const MouseEvent& e) override { if (sculptMode > 0) sculptDrag (e); else if (dragging) setMatter (e.position); }
    void mouseUp (const MouseEvent&) override
    {
        if (sculptMode > 0) { sculptMode = 0; if (! sculptMoved) { ripple = 1.0f; } hearSculpt(); return; }
        if (dragging) { dragging = false; make (true); }
    }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        if (sculpting && stage().contains (e.getPosition())) { stretch = bright = heat = cool = split = 0; hearSculpt(); }
    }

    void debugSet (int ex, int bo, float m, float s) { proc.alcExc = ex; proc.alcBody = bo; proc.alcMatter = m; proc.alcSize = s; cur = recipe(); repaint(); }
    // SCULPT what is on the keys now (from the top bar): the current sound becomes the matter in your hands
    void sculptCurrent()
    {
        sculptBase = proc.currentGenome(); useBase = sculptBase.valid();
        stretch = bright = heat = cool = split = 0;
        cur = recipe(); setView (true);
    }
    void debugSculpt() { setView (true); stretch = 0.6f; heat = 0.5f; split = 0.45f; cur = recipe(); repaint(); }
private:
    Rectangle<int> colArea (int col) const { const int w = (getWidth() - 48 - 40) / 3 - 20; return { 24 + col * (w + 20), 96, w, getHeight() - 120 }; }
    Rectangle<int> card (int col, int i) const { const auto c = colArea (col); const int h = (c.getHeight() - 4 * 10) / 5; return { c.getX(), c.getY() + i * (h + 10), c.getWidth(), h }; }
    Rectangle<int> rightArea() const { const auto c = colArea (1); return { c.getRight() + 40, 96, getWidth() - c.getRight() - 40 - 24, getHeight() - 120 }; }
    Rectangle<int> padArea() const { const auto r = rightArea(); return r.withHeight (r.getHeight() - 220); }
    Rectangle<int> resultArea() const { const auto r = rightArea(); return r.withTrimmedTop (r.getHeight() - 200); }

    void setMatter (Point<float> p)
    {
        const auto r = padArea().toFloat();
        proc.alcMatter = jlimit (0.0f, 1.0f, (p.x - r.getX()) / r.getWidth());
        proc.alcSize = jlimit (0.0f, 1.0f, (p.y - r.getY()) / r.getHeight());
        const auto now = Time::getMillisecondCounter();
        if (now - lastLive > 110) { lastLive = now; make (false); }   // live while you drag (no note spam)
        repaint();
    }
    KeysKillaProcessor::Genome recipe() const
    {
        if (useBase) return proc.sculpt (sculptBase, stretch, bright, heat, cool, split);
        return proc.sculpt (proc.alchemy (proc.alcExc.load(), proc.alcBody.load(), proc.alcMatter.load(), proc.alcSize.load(), proc.alcSeed), stretch, bright, heat, cool, split);
    }
    void setView (bool s) { sculpting = s; makeTab.selected = ! s; sculptTab.selected = s; makeTab.repaint(); sculptTab.repaint(); repaint(); }
    Rectangle<int> stage() const { const auto a = colArea (0), b = colArea (1); return a.getUnion (b); }
    // ---- SCULPT gestures: stretch / squash (sideways from the edge), bright / dark (up / down), rub = heat, hold still = it freezes,
    //      right-drag = tear it in two (a second layer), click = poke it (hear it), double-click = back to the raw matter
    void sculptDown (const MouseEvent& e)
    {
        sculptMode = e.mods.isPopupMenu() ? 3 : 1; sculptMoved = false;
        downPos = lastPos = e.position; downMs = lastMoveMs = Time::getMillisecondCounter(); lastDir = 0;
        s0.x = stretch; s0.y = bright; s0.z = split;
    }
    void sculptDrag (const MouseEvent& e)
    {
        const auto d = e.position - downPos;
        if (! sculptMoved && d.getDistanceFromOrigin() > 6) sculptMoved = true;
        const auto st = stage().toFloat(); const float cx = st.getCentreX();
        if (sculptMode == 3) split = jlimit (0.0f, 1.0f, s0.z + std::abs (d.x) / 320.0f);
        else if (sculptMoved)
        {
            // rubbing: fast changes of direction heat the matter
            const float mx = e.position.x - lastPos.x;
            const int dir = mx > 2 ? 1 : mx < -2 ? -1 : 0;
            if (dir != 0 && lastDir != 0 && dir != lastDir && Time::getMillisecondCounter() - lastMoveMs < 220) heat = jmin (1.0f, heat + 0.05f);
            if (dir != 0) lastDir = dir;
            if (e.position.getDistanceFrom (lastPos) > 2) lastMoveMs = Time::getMillisecondCounter();
            if (std::abs (d.x) > std::abs (d.y) * 1.2f)
            {
                const float outward = (downPos.x < cx ? -d.x : d.x);
                stretch = jlimit (-1.0f, 1.0f, s0.x + outward / 260.0f);
            }
            else if (std::abs (d.y) > std::abs (d.x) * 1.2f) bright = jlimit (-1.0f, 1.0f, s0.y - d.y / 220.0f);
        }
        lastPos = e.position;
        live();
    }
    void hearSculpt() { cur = recipe(); proc.alcUse (cur, true); note.clear(); repaint(); }
    void live()
    {
        const auto now = Time::getMillisecondCounter();
        if (now - lastLive > 110) { lastLive = now; cur = recipe(); proc.alcUse (cur, false); }
        repaint();
    }
    void drawSculpt (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto st = stage().toFloat();
        g.setColour (Colour (0xff05070c).withAlpha (t.night ? 0.7f : 0.88f)); g.fillRoundedRectangle (st, 18);
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (14.0f, true, 0.3f));
        g.drawText ("SCULPT THE SOUND", (int) st.getX(), (int) st.getY() - 24, 300, 20, Justification::centredLeft);
        const auto c = st.getCentre();
        const float rad = std::min (st.getWidth(), st.getHeight()) * 0.26f;
        const float sx = 1.0f + 0.45f * stretch, sy = 1.0f - 0.25f * stretch;
        const auto hot = Colour (0xffff5a1f), ice = Colour (0xff9be7ff);
        auto base = Colour (0xff8f7cff).interpolatedWith (hot, heat).interpolatedWith (ice, cool * 0.85f);
        const float freezeSlow = 1.0f - 0.85f * cool;
        auto lump = [&] (Point<float> centre, float scale, float seedPh)
        {
            Path p; const int n = 72;
            for (int i = 0; i <= n; ++i)
            {
                const float a = (float) i / (float) n * MathConstants<float>::twoPi;
                float k = 1.0f + 0.06f * std::sin (a * 3.0f + phase * 1.3f * freezeSlow + seedPh) + 0.04f * std::sin (a * 5.0f - phase * 2.1f * freezeSlow);
                k += heat * 0.08f * std::sin (a * 23.0f + phase * 9.0f);                                       // hot: it boils
                k += bright * 0.22f * std::pow (std::max (0.0f, -std::sin (a)), 6.0f);                          // bright: a peak on top
                k -= std::min (0.0f, bright) * 0.18f * std::pow (std::max (0.0f, std::sin (a)), 2.0f);          // dark: it sags
                if (cool > 0.05f) k += cool * 0.12f * (std::abs (std::fmod (a / MathConstants<float>::twoPi * 8.0f, 1.0f) - 0.5f) - 0.25f);   // frozen: facets
                const auto pt = centre + Point<float> (std::cos (a) * rad * scale * sx * k, std::sin (a) * rad * scale * sy * k);
                if (i == 0) p.startNewSubPath (pt); else p.lineTo (pt);
            }
            p.closeSubPath();
            g.setColour (base.withAlpha (0.18f + 0.2f * heat)); g.fillPath (p, AffineTransform::scale (1.25f, 1.25f, centre.x, centre.y));
            g.setGradientFill (ColourGradient (base.brighter (0.7f), centre.x - rad * 0.5f, centre.y - rad * 0.6f, base.darker (0.6f), centre.x + rad, centre.y + rad, true));
            g.fillPath (p);
            g.setColour (Colours::white.withAlpha (0.35f + 0.4f * cool)); g.strokePath (p, PathStrokeType (1.6f));
        };
        const float gap = split * rad * 1.3f;
        if (split > 0.05f) { lump (c - Point<float> (gap, 0), 0.8f, 0.0f); lump (c + Point<float> (gap, 0), 0.8f - 0.25f * split, 2.0f); }
        else lump (c, 1.0f, 0.0f);
        if (ripple > 0) { const float r = rad * (1.6f - ripple * 0.5f); g.setColour (Colours::white.withAlpha (ripple * 0.5f)); g.drawEllipse (c.x - r * sx, c.y - r * sy, r * 2 * sx, r * 2 * sy, 2.0f); }
        if (cool > 0.05f) for (int i = 0; i < 18; ++i) { Random rr (i + 3); const auto p = c + Point<float> ((rr.nextFloat() - 0.5f) * rad * 2.6f * sx, (rr.nextFloat() - 0.5f) * rad * 2.2f); g.setColour (ice.withAlpha (0.5f * cool)); g.drawLine (p.x - 4, p.y, p.x + 4, p.y, 1.0f); g.drawLine (p.x, p.y - 4, p.x, p.y + 4, 1.0f); }
        // what your hands did
        g.setFont (kk::modern::font (12.0f, true, 0.25f));
        auto tag = [&] (int i, const String& n, float v, Colour col)
        {
            const auto r = Rectangle<float> (st.getX() + 20 + (float) i * 150, st.getBottom() - 46, 140, 26);
            g.setColour (col.withAlpha (0.12f + 0.5f * std::abs (v))); g.fillRoundedRectangle (r, 13);
            g.setColour (Colours::white.withAlpha (0.5f + 0.5f * std::abs (v))); g.drawText (n, r.toNearestInt(), Justification::centred);
        };
        tag (0, stretch >= 0 ? "STRETCHED" : "SQUASHED", stretch, Colour (0xff8f7cff));
        tag (1, bright >= 0 ? "SHARP" : "SAGGING", bright, Colour (0xffffd23f));
        tag (2, "HOT", heat, hot); tag (3, "FROZEN", cool, ice); tag (4, "TORN", split, Colour (0xffff4fd8));
        g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, true, 0.03f));
        g.drawFittedText ("drag sideways from its edge = stretch / squash      up / down = sharp / sagging      rub it = heat\nhold it still = it freezes      right-drag = tear it in two      click = hear it      double-click = raw matter",
                          st.reduced (20, 14).withHeight (40).toNearestInt(), Justification::topLeft, 2);
    }
    void make (bool audition)
    {
        useBase = false;   // MAKE / MATTER / a new mutation: back to the alchemy recipe
        cur = recipe();
        proc.alcUse (cur, audition);
        note.clear();
        repaint();
    }
    void drawMatter (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto r = padArea().toFloat();
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (14.0f, true, 0.3f));
        g.drawText ("3   MATTER", (int) r.getX(), (int) r.getY() - 24, 200, 20, Justification::centredLeft);
        Graphics::ScopedSaveState ss (g);
        Path clip; clip.addRoundedRectangle (r, 16); g.reduceClipRegion (clip);
        g.setGradientFill (ColourGradient (Colour (0xff0e3a4f), r.getX(), r.getY(), Colour (0xff3a2614), r.getRight(), r.getY(), false));
        g.fillRect (r);
        // glass facets on the left, mud bubbles on the right
        Random rr (9);
        for (int i = 0; i < 26; ++i)
        {
            const float x = r.getX() + rr.nextFloat() * r.getWidth() * 0.45f, y = r.getY() + rr.nextFloat() * r.getHeight(), s = 20 + rr.nextFloat() * 50;
            Path f; f.addTriangle (x, y, x + s, y + s * 0.3f, x + s * 0.4f, y + s);
            g.setColour (Colours::white.withAlpha (0.04f + 0.05f * std::sin (phase + (float) i))); g.fillPath (f);
        }
        for (int i = 0; i < 22; ++i)
        {
            const float x = r.getX() + r.getWidth() * (0.55f + 0.45f * rr.nextFloat()), s = 6 + rr.nextFloat() * 18;
            const float y = r.getBottom() - std::fmod (rr.nextFloat() * r.getHeight() + phase * (8 + 10 * rr.nextFloat()), r.getHeight());
            g.setColour (Colour (0xff8a5a2b).withAlpha (0.25f)); g.drawEllipse (x, y, s, s, 1.5f);
        }
        g.setColour (Colours::white.withAlpha (0.55f)); g.setFont (kk::modern::font (12.0f, true, 0.3f));
        g.drawText ("GLASS", r.reduced (12).toNearestInt(), Justification::centredLeft);
        g.drawText ("MUD", r.reduced (12).toNearestInt(), Justification::centredRight);
        g.drawText ("TINY", r.reduced (12).toNearestInt(), Justification::centredTop);
        g.drawText ("GIANT", r.reduced (12).toNearestInt(), Justification::centredBottom);
        // the drop of matter: sharp crystal when glassy, a soft dripping blob when muddy - bigger when giant
        const float m = proc.alcMatter.load(), sz = proc.alcSize.load();
        const Point<float> c (r.getX() + m * r.getWidth(), r.getY() + sz * r.getHeight());
        const float rad = 18.0f + 26.0f * sz;
        Path blob;
        const int n = 48;
        for (int i = 0; i <= n; ++i)
        {
            const float a = (float) i / (float) n * MathConstants<float>::twoPi;
            const float crystal = std::abs (std::fmod (a / MathConstants<float>::twoPi * 6.0f, 1.0f) - 0.5f) * 2.0f;   // 6 points
            const float soft = 0.08f * std::sin (a * 3.0f + phase * 2.0f) + 0.05f * std::sin (a * 5.0f - phase * 3.0f);
            float k = (1.0f - m) * (0.65f + 0.35f * crystal) + m * (1.0f + soft);
            if (m > 0.5f && a > 1.2f && a < 1.95f) k += (m - 0.5f) * 0.9f * std::sin ((a - 1.2f) / 0.75f * MathConstants<float>::pi);   // a drip
            const auto pt = c + Point<float> (std::cos (a), std::sin (a)) * rad * k;
            if (i == 0) blob.startNewSubPath (pt); else blob.lineTo (pt);
        }
        blob.closeSubPath();
        const auto ca = Colour (0xff9be7ff).interpolatedWith (Colour (0xff6b4423), m);
        g.setColour (ca.withAlpha (0.25f)); g.fillPath (blob, AffineTransform::scale (1.35f, 1.35f, c.x, c.y));
        g.setGradientFill (ColourGradient (ca.brighter (0.6f), c.x - rad, c.y - rad, ca.darker (0.4f), c.x + rad, c.y + rad, false));
        g.fillPath (blob);
        g.setColour (Colours::white.withAlpha (0.7f - 0.5f * m)); g.strokePath (blob, PathStrokeType (1.5f));
        g.setColour (Colours::white.withAlpha (0.9f)); g.setFont (kk::modern::font (13.0f, true, 0.2f));
        g.drawText (kk::alc::matterWord (m).toUpperCase(), Rectangle<float> (c.x - 60, c.y + rad * 1.3f + 4, 120, 18), Justification::centred);
    }
    static void drawPartIcon (Graphics& g, int col, int i, Rectangle<float> r, Colour c, bool on)
    {
        g.setColour (c.withAlpha (on ? 1.0f : 0.75f));
        const auto cx = r.getCentreX(), cy = r.getCentreY(), w = r.getWidth() * 0.5f;
        const PathStrokeType st (2.0f, PathStrokeType::curved, PathStrokeType::rounded);
        Path p;
        if (col == 0)
        {
            switch (i)
            {
                case 0: p.addRectangle (cx - w * 0.9f, cy - w * 0.9f, w * 0.9f, w * 0.5f); p.startNewSubPath (cx - w * 0.45f, cy - w * 0.4f); p.lineTo (cx - w * 0.45f, cy + w); // hammer
                        for (int k = 0; k < 4; ++k) { const float a = 0.3f + k * 0.4f; p.startNewSubPath (cx + w * 0.2f + std::cos (a) * w * 0.3f, cy - w * 0.6f + std::sin (a) * w * 0.3f); p.lineTo (cx + w * 0.2f + std::cos (a) * w * 0.8f, cy - w * 0.6f + std::sin (a) * w * 0.8f); } break;
                case 1: for (int k = 0; k < 4; ++k) { p.startNewSubPath (cx - w, cy - w * 0.6f + k * w * 0.4f); for (int x = 1; x <= 12; ++x) p.lineTo (cx - w + w * 2 * x / 12.0f, cy - w * 0.6f + k * w * 0.4f + std::sin ((float) x * 1.7f + k) * w * 0.12f); } break;
                case 2: p.startNewSubPath (cx + w * 0.2f, cy - w); p.lineTo (cx - w * 0.5f, cy + w * 0.1f); p.lineTo (cx + w * 0.1f, cy + w * 0.1f); p.lineTo (cx - w * 0.3f, cy + w); break;
                case 3: p.addCentredArc (cx, cy, w * 0.8f, w * 0.8f, 0, 0.5f, 5.5f, true); p.addCentredArc (cx, cy, w * 0.45f, w * 0.45f, 0, 1.0f, 5.0f, true); break;
                default: for (int k = 0; k < 3; ++k) p.addCentredArc (cx - w * 0.6f, cy, w * (0.5f + 0.4f * k), w * (0.5f + 0.4f * k), 0, 0.6f, 2.5f, true); p.addEllipse (cx - w * 0.9f, cy - w * 0.15f, w * 0.3f, w * 0.3f); break;
            }
        }
        else
        {
            switch (i)
            {
                case 0: p.addRoundedRectangle (cx - w * 0.75f, cy - w * 0.22f, w * 1.5f, w * 0.44f, w * 0.2f); p.addEllipse (cx - w, cy - w * 0.4f, w * 0.45f, w * 0.45f); p.addEllipse (cx - w, cy - w * 0.05f, w * 0.45f, w * 0.45f);
                        p.addEllipse (cx + w * 0.55f, cy - w * 0.4f, w * 0.45f, w * 0.45f); p.addEllipse (cx + w * 0.55f, cy - w * 0.05f, w * 0.45f, w * 0.45f); break;
                case 1: p.addEllipse (cx - w * 0.8f, cy - w * 0.4f, w * 1.0f, w * 1.0f); p.addEllipse (cx + w * 0.1f, cy - w * 0.9f, w * 0.6f, w * 0.6f); p.addEllipse (cx + w * 0.3f, cy + w * 0.2f, w * 0.45f, w * 0.45f); break;
                case 2: p.startNewSubPath (cx, cy - w); p.cubicTo (cx + w, cy + w * 0.1f, cx + w * 0.6f, cy + w, cx, cy + w); p.cubicTo (cx - w * 0.6f, cy + w, cx - w, cy + w * 0.1f, cx, cy - w); break;
                case 3: p.addPolygon ({ cx, cy }, 6, w * 0.95f); p.startNewSubPath (cx, cy - w * 0.95f); p.lineTo (cx, cy + w * 0.95f); p.startNewSubPath (cx - w * 0.82f, cy - w * 0.47f); p.lineTo (cx + w * 0.82f, cy + w * 0.47f); break;
                default: p.addEllipse (cx - w * 0.9f, cy - w * 0.9f, w * 1.8f, w * 1.8f); g.setColour (c.withAlpha (0.35f)); g.fillEllipse (cx - w * 0.5f, cy - w * 0.5f, w, w); g.setColour (c); break;
            }
        }
        g.strokePath (p, st);
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f;
        if (sculpting)
        {
            // holding still on the matter: it freezes
            if (sculptMode == 1 && Time::getMillisecondCounter() - lastMoveMs > 450 && cool < 1.0f) { cool = jmin (1.0f, cool + 0.02f); live(); }
            if (ripple > 0) ripple = jmax (0.0f, ripple - 0.05f);
            repaint (stage().expanded (4));
        }
        repaint (padArea().expanded (4));
    }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton playBtn { lnf }, mutateBtn { lnf }, plantBtn { lnf }, saveBtn { lnf }, useBtn { lnf }, makeTab { lnf }, sculptTab { lnf };
    bool sculpting = false, sculptMoved = false, useBase = false;
    KeysKillaProcessor::Genome sculptBase;
    int sculptMode = 0, lastDir = 0;
    float stretch = 0, bright = 0, heat = 0, cool = 0, split = 0, ripple = 0;
    Point<float> downPos, lastPos;
    struct { float x = 0, y = 0, z = 0; } s0;
    uint32 downMs = 0, lastMoveMs = 0;
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    KeysKillaProcessor::Genome cur;
    std::function<void()> onPicked;
    String pickTitle, note;
    int hoverCol = -1, hoverCard = -1;
    bool dragging = false;
    uint32 lastLive = 0;
    float phase = 0;
};
