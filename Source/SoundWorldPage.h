// v0.42 SOUND WORLD page (included by PluginEditor.cpp). The world as dots - hover = hear, click = keep,
// CONNECT two regions = a hybrid. DRAG WAV / SAVE / PLANT IN EVOLVE.
class SoundWorldPage : public Component, private Timer
{
public:
    SoundWorldPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (connectBtn, "CONNECT", "CONNECT: click one region, then another - a new sound grows from both", [this] { connecting = ! connecting; connectA = -1; connectBtn.selected = connecting; connectBtn.repaint(); note = connecting ? "click the first region" : String(); repaint(); });
        btn (playBtn, "PLAY", "Hear the sound again", [this] { if (proc.worldCurrent.valid()) proc.worldPlay (proc.worldCurrent, true); });
        btn (plantBtn, "PLANT IN EVOLVE", "This sound becomes the seed of a new EVOLVE tree", [this] { if (proc.worldCurrent.valid()) { proc.evoSeedGenome (proc.worldCurrent); note = "planted - open EVOLVE"; repaint(); } });
        btn (saveBtn, "SAVE", "Save this sound into your folders / sound kits", [this]
        {
            if (! proc.worldCurrent.valid()) return;
            const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
            auto snd = kk::PairLab::fromBuffer (proc.renderGenomeAudio (proc.worldCurrent, rate, 3.0), rate, rate, proc.worldCurrent.name);
            saveToFolderMenu (proc, { snd }, &saveBtn, [safe = SafePointer<SoundWorldPage> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->repaint(); } });
        });
        btn (diceBtn, "SURPRISE ME", "A random place on Earth", [this]
        {
            const auto& ds = kk::world::dots();
            pick ((int) (Random::getSystemRandom().nextInt ((int) ds.size())));
        });
        plantBtn.hero = true;
        dragWav.makeFile = [this] { return proc.worldCurrent.valid() ? proc.exportGenomeWav (proc.worldCurrent) : File(); };
        dragWav.setTooltip ("Drag the sound into FL as a WAV");
        addAndMakeVisible (dragWav);
        startTimerHz (20);
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("SOUND WORLD", 24, 12, 280, 40, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText ("every dot is a sound of its land.  hover = hear   click = keep   CONNECT two regions = a new sound from both", 290, 18, getWidth() - 310, 28, Justification::centredLeft);
        const auto m = mapArea();
        g.setColour (Colour (0xff05070c).withAlpha (t.night ? 0.75f : 0.88f)); g.fillRoundedRectangle (m.toFloat().expanded (6), 16);
        if (cache.isNull() || cacheSize != m.getBottomRight() - m.getPosition() || cacheTheme != kk::themeIndex()) buildCache();
        g.drawImageAt (cache, m.getX(), m.getY());
        const auto& ds = kk::world::dots();
        const auto& rs = kk::world::regions();
        // twinkle
        for (auto& tw : twinkles)
        {
            if (! isPositiveAndBelow (tw.first, (int) ds.size())) continue;
            const auto pt = toScreen (ds[(size_t) tw.first].lon, ds[(size_t) tw.first].lat);
            const float a = std::sin (tw.second * MathConstants<float>::pi);
            g.setColour (Colour (rs[(size_t) ds[(size_t) tw.first].region].colour).brighter (0.6f).withAlpha (0.9f * a)); g.fillEllipse (pt.x - 2.5f, pt.y - 2.5f, 5, 5);
        }
        // region names at their hearts
        for (int r = 0; r < (int) rs.size(); ++r)
        {
            const auto c = toScreen (rs[(size_t) r].lon, rs[(size_t) r].lat);
            const bool on = r == hoverRegion || r == proc.worldRegion || r == connectA;
            g.setColour (Colour (rs[(size_t) r].colour).withAlpha (on ? 1.0f : 0.55f)); g.setFont (kk::modern::font (on ? 13.0f : 10.5f, true, 0.18f));
            g.drawText (rs[(size_t) r].name, Rectangle<float> (c.x - 80, c.y - 9, 160, 18), Justification::centred);
        }
        // connection
        if (proc.worldRegion >= 0 && proc.worldRegionB >= 0)
        {
            const auto a = toScreen (rs[(size_t) proc.worldRegion].lon, rs[(size_t) proc.worldRegion].lat), b = toScreen (rs[(size_t) proc.worldRegionB].lon, rs[(size_t) proc.worldRegionB].lat);
            Path arc; arc.startNewSubPath (a); arc.quadraticTo ((a.x + b.x) * 0.5f, std::min (a.y, b.y) - a.getDistanceFrom (b) * 0.35f, b.x, b.y);
            g.setColour (Colours::white.withAlpha (0.25f)); g.strokePath (arc, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
            g.setGradientFill (ColourGradient (Colour (rs[(size_t) proc.worldRegion].colour), a, Colour (rs[(size_t) proc.worldRegionB].colour), b, false));
            g.strokePath (arc, PathStrokeType (2.6f, PathStrokeType::curved, PathStrokeType::rounded));
            // a spark travels along it
            const float u = std::fmod (phase * 0.5f, 1.0f);
            const auto sp = arc.getPointAlongPath (arc.getLength() * u);
            g.setColour (Colours::white); g.fillEllipse (sp.x - 4, sp.y - 4, 8, 8);
        }
        // the kept dot, the hovered dot
        auto ring = [&] (int dot, float rad, float alpha)
        {
            if (! isPositiveAndBelow (dot, (int) ds.size())) return;
            const auto pt = toScreen (ds[(size_t) dot].lon, ds[(size_t) dot].lat);
            const auto c = Colour (rs[(size_t) ds[(size_t) dot].region].colour);
            g.setColour (c.withAlpha (0.25f * alpha)); g.fillEllipse (pt.x - rad * 1.8f, pt.y - rad * 1.8f, rad * 3.6f, rad * 3.6f);
            g.setColour (Colours::white.withAlpha (alpha)); g.drawEllipse (pt.x - rad, pt.y - rad, rad * 2, rad * 2, 2.0f);
        };
        ring (proc.worldDot, 9.0f + 2.0f * std::sin (phase * 4.0f), 1.0f);
        ring (hoverDot, 7.0f, 0.8f);
        if (connectA >= 0) { const auto c = toScreen (rs[(size_t) connectA].lon, rs[(size_t) connectA].lat); g.setColour (Colour (rs[(size_t) connectA].colour)); g.drawEllipse (c.x - 22, c.y - 22, 44, 44, 2.5f); }
        // right panel
        const auto P = panelArea();
        kk::modern::plate (g, P.toFloat(), 16.0f);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.25f));
        g.drawText ("THE SOUND", P.getX() + 16, P.getY() + 14, 200, 16, Justification::centredLeft);
        const int shownRegion = proc.worldRegionB >= 0 ? proc.worldRegion : (hoverRegion >= 0 ? hoverRegion : proc.worldRegion);
        g.setColour (t.text); g.setFont (kk::modern::font (18.0f, true, 0.03f));
        g.drawFittedText (proc.worldCurrent.valid() ? proc.worldCurrent.name : String ("hover the map"), Rectangle<int> (P.getX() + 16, P.getY() + 34, P.getWidth() - 32, 50), Justification::topLeft, 2, 0.8f);
        if (shownRegion >= 0)
        {
            const auto& R = rs[(size_t) shownRegion];
            g.setColour (Colour (R.colour)); g.setFont (kk::modern::font (14.0f, true, 0.2f));
            g.drawText (proc.worldRegionB >= 0 ? String (R.name) + "  x  " + rs[(size_t) proc.worldRegionB].name : String (R.name), P.getX() + 16, P.getY() + 88, P.getWidth() - 32, 20, Justification::centredLeft);
            g.setColour (t.dim); g.setFont (kk::modern::font (12.5f, false, 0.0f));
            g.drawFittedText (R.hint, Rectangle<int> (P.getX() + 16, P.getY() + 110, P.getWidth() - 32, 36), Justification::topLeft, 2, 0.85f);
        }
        // legend
        const int ly = legendTop();
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.25f));
        g.drawText ("REGIONS - click = a sound", P.getX() + 16, ly - 20, P.getWidth() - 32, 16, Justification::centredLeft);
        for (int r = 0; r < (int) rs.size(); ++r)
        {
            const auto rr = legendRow (r);
            g.setColour (Colour (rs[(size_t) r].colour)); g.fillEllipse ((float) rr.getX(), (float) rr.getCentreY() - 5, 10, 10);
            g.setColour (r == hoverLegend ? t.text : t.text.withAlpha (0.8f)); g.setFont (kk::modern::font (12.0f, true, 0.1f));
            g.drawText (rs[(size_t) r].name, rr.withTrimmedLeft (18), Justification::centredLeft);
        }
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (13.0f, true, 0.03f)); g.drawText (note, m.getX(), m.getBottom() + 8, m.getWidth(), 20, Justification::centredLeft); }
    }
    void resized() override
    {
        const auto P = panelArea();
        int y = P.getY() + 154;
        playBtn.setBounds (P.getX() + 14, y, (P.getWidth() - 34) / 2, 36); diceBtn.setBounds (playBtn.getRight() + 6, y, (P.getWidth() - 34) / 2, 36); y += 42;
        connectBtn.setBounds (P.getX() + 14, y, P.getWidth() - 28, 36); y += 42;
        plantBtn.setBounds (P.getX() + 14, y, P.getWidth() - 28, 40); y += 46;
        saveBtn.setBounds (P.getX() + 14, y, (P.getWidth() - 34) / 2, 40); dragWav.setBounds (saveBtn.getRight() + 6, y - 2, (P.getWidth() - 34) / 2, 44);
        cache = {};
    }
    void mouseMove (const MouseEvent& e) override
    {
        const int d = dotAt (e.position);
        int lg = -1; for (int r = 0; r < (int) kk::world::regions().size(); ++r) if (legendRow (r).contains (e.getPosition())) lg = r;
        if (d != hoverDot || lg != hoverLegend)
        {
            hoverDot = d; hoverLegend = lg; hoverSince = Time::getMillisecondCounter(); heard = false;
            hoverRegion = d >= 0 ? kk::world::dots()[(size_t) d].region : -1;
            repaint();
        }
    }
    void mouseExit (const MouseEvent&) override
    {
        if (hoverHeard >= 0 && kept.valid()) proc.worldPlay (kept, false);   // back to the sound you kept
        hoverDot = -1; hoverRegion = -1; hoverHeard = -1; repaint();
    }
    void mouseUp (const MouseEvent& e) override
    {
        for (int r = 0; r < (int) kk::world::regions().size(); ++r)
            if (legendRow (r).contains (e.getPosition()))
            {
                const auto& ds = kk::world::dots();
                std::vector<int> mine; for (int i = 0; i < (int) ds.size(); ++i) if (ds[(size_t) i].region == r) mine.push_back (i);
                if (! mine.empty()) { if (connecting) regionClick (r); else pick (mine[(size_t) Random::getSystemRandom().nextInt ((int) mine.size())]); }
                return;
            }
        const int d = dotAt (e.position);
        if (d < 0) return;
        if (connecting) { regionClick (kk::world::dots()[(size_t) d].region); return; }
        pick (d);
    }
private:
    Rectangle<int> mapArea() const { return { 24, 66, getWidth() - 24 - 340, getHeight() - 66 - 40 }; }
    Rectangle<int> panelArea() const { return { getWidth() - 324, 60, 304, getHeight() - 76 }; }
    int legendTop() const { return panelArea().getY() + 380; }
    Rectangle<int> legendRow (int r) const { const auto P = panelArea(); return { P.getX() + 16 + (r % 2) * ((P.getWidth() - 32) / 2), legendTop() + (r / 2) * 26, (P.getWidth() - 32) / 2, 22 }; }
    Point<float> toScreen (float lon, float lat) const
    {
        const auto m = mapArea().toFloat();
        return { m.getX() + m.getWidth() * (lon + 180.0f) / 360.0f, m.getY() + m.getHeight() * (80.0f - lat) / 140.0f };
    }
    void buildCache()
    {
        const auto m = mapArea();
        cacheSize = m.getBottomRight() - m.getPosition(); cacheTheme = kk::themeIndex();
        cache = Image (Image::ARGB, jmax (1, m.getWidth()), jmax (1, m.getHeight()), true);
        Graphics g (cache);
        const auto& rs = kk::world::regions();
        screen.clear(); grid.clear();
        gridW = cache.getWidth() / 12 + 1; gridH = cache.getHeight() / 12 + 1;
        grid.assign ((size_t) (gridW * gridH), {});
        const auto& ds = kk::world::dots();
        for (int i = 0; i < (int) ds.size(); ++i)
        {
            const auto p = toScreen (ds[(size_t) i].lon, ds[(size_t) i].lat) - m.getPosition().toFloat();
            screen.push_back (p);
            const float heart = 1.0f - 0.55f * kk::world::distanceFromHeart (ds[(size_t) i]);
            g.setColour (Colour (rs[(size_t) ds[(size_t) i].region].colour).withAlpha (0.35f + 0.55f * ds[(size_t) i].bright * heart));
            g.fillEllipse (p.x - 1.4f, p.y - 1.4f, 2.8f, 2.8f);
            const int gx = jlimit (0, gridW - 1, (int) (p.x / 12)), gy = jlimit (0, gridH - 1, (int) (p.y / 12));
            grid[(size_t) (gy * gridW + gx)].push_back (i);
        }
    }
    int dotAt (Point<float> pos) const
    {
        if (screen.empty()) return -1;
        const auto m = mapArea();
        const auto p = pos - m.getPosition().toFloat();
        const int gx = (int) (p.x / 12), gy = (int) (p.y / 12);
        int best = -1; float bd = 10.0f;
        for (int yy = gy - 1; yy <= gy + 1; ++yy)
            for (int xx = gx - 1; xx <= gx + 1; ++xx)
            {
                if (xx < 0 || yy < 0 || xx >= gridW || yy >= gridH) continue;
                for (int i : grid[(size_t) (yy * gridW + xx)]) { const float d = screen[(size_t) i].getDistanceFrom (p); if (d < bd) { bd = d; best = i; } }
            }
        return best;
    }
    void pick (int dot)
    {
        proc.worldDot = dot; proc.worldRegion = kk::world::dots()[(size_t) dot].region; proc.worldRegionB = -1;
        kept = proc.worldSound (dot);
        proc.worldPlay (kept, true); hoverHeard = -1;
        note = {};
        repaint();
    }
    void regionClick (int r)
    {
        if (connectA < 0) { connectA = r; note = "now click the second region"; repaint(); return; }
        if (r == connectA) return;
        proc.worldRegion = connectA; proc.worldRegionB = r; proc.worldDot = -1;
        kept = proc.worldConnect (connectA, r, (uint32) Time::getMillisecondCounter());
        proc.worldPlay (kept, true); hoverHeard = -1;
        note = "connected - CONNECT again for another hybrid";
        connectA = -1; connecting = false; connectBtn.selected = false; connectBtn.repaint();
        repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f;
        const auto& ds = kk::world::dots();
        for (auto& tw : twinkles) tw.second += 0.04f;
        twinkles.erase (std::remove_if (twinkles.begin(), twinkles.end(), [] (auto& tw) { return tw.second >= 1.0f; }), twinkles.end());
        auto& rnd = Random::getSystemRandom();
        while (twinkles.size() < 90) twinkles.push_back ({ rnd.nextInt ((int) ds.size()), rnd.nextFloat() * 0.3f });
        const auto now = Time::getMillisecondCounter();
        if (hoverDot >= 0 && ! heard && now - hoverSince > 260 && ! connecting) { heard = true; proc.worldPlay (proc.worldSound (hoverDot), true); hoverHeard = hoverDot; }
        repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton connectBtn { lnf }, playBtn { lnf }, plantBtn { lnf }, saveBtn { lnf }, diceBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    Image cache; Point<int> cacheSize; int cacheTheme = -1;
    KeysKillaProcessor::Genome kept;
    std::vector<Point<float>> screen; std::vector<std::vector<int>> grid; int gridW = 1, gridH = 1;
    std::vector<std::pair<int, float>> twinkles;
    String note;
    int hoverDot = -1, hoverRegion = -1, hoverLegend = -1, hoverHeard = -1, connectA = -1;
    uint32 hoverSince = 0; bool heard = false, connecting = false; float phase = 0;
};
