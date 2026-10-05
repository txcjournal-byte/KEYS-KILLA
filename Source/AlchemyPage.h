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
        dragWav.makeFile = [this] { return cur.valid() ? proc.exportGenomeWav (cur) : File(); };
        dragWav.setTooltip ("Drag the sound into FL as a WAV");
        addAndMakeVisible (dragWav);
        cur = proc.alchemy (proc.alcExc.load(), proc.alcBody.load(), proc.alcMatter.load(), proc.alcSize.load(), proc.alcSeed);
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
        column (0, "1   EXCITE", ex, proc.alcExc.load());
        column (1, "2   BODY", bo, proc.alcBody.load());
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
    }
    void mouseMove (const MouseEvent& e) override
    {
        int hc = -1, hk = -1;
        for (int col = 0; col < 2; ++col) for (int i = 0; i < 5; ++i) if (card (col, i).contains (e.getPosition())) { hc = col; hk = i; }
        if (hc != hoverCol || hk != hoverCard) { hoverCol = hc; hoverCard = hk; repaint(); }
    }
    void mouseExit (const MouseEvent&) override { hoverCol = hoverCard = -1; repaint(); }
    void mouseDown (const MouseEvent& e) override
    {
        for (int col = 0; col < 2; ++col)
            for (int i = 0; i < 5; ++i)
                if (card (col, i).contains (e.getPosition())) { (col == 0 ? proc.alcExc : proc.alcBody) = i; make (true); return; }
        if (padArea().contains (e.getPosition())) { dragging = true; setMatter (e.position); }
    }
    void mouseDrag (const MouseEvent& e) override { if (dragging) setMatter (e.position); }
    void mouseUp (const MouseEvent&) override { if (dragging) { dragging = false; make (true); } }

    void debugSet (int ex, int bo, float m, float s) { proc.alcExc = ex; proc.alcBody = bo; proc.alcMatter = m; proc.alcSize = s; cur = proc.alchemy (ex, bo, m, s, proc.alcSeed); repaint(); }
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
    void make (bool audition)
    {
        cur = proc.alchemy (proc.alcExc.load(), proc.alcBody.load(), proc.alcMatter.load(), proc.alcSize.load(), proc.alcSeed);
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
    void timerCallback() override { if (! isShowing()) return; phase += 0.05f; repaint (padArea().expanded (4)); }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton playBtn { lnf }, mutateBtn { lnf }, plantBtn { lnf }, saveBtn { lnf }, useBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    KeysKillaProcessor::Genome cur;
    std::function<void()> onPicked;
    String pickTitle, note;
    int hoverCol = -1, hoverCard = -1;
    bool dragging = false;
    uint32 lastLive = 0;
    float phase = 0;
};
