// v0.44 FEED page (included by PluginEditor.cpp): sounds stream to you like a feed - no browsing, no knobs.
// Each card plays itself in a little phrase.  Swipe it: right = KEEP, left = SKIP, up = MORE LIKE THIS, down = SOMETHING ELSE.
// The feed learns your taste from every swipe (what excites the matter, the body, glass or mud, tiny or giant).
class FeedPage : public Component, private Timer
{
public:
    FeedPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        btn (stopBtn, "STOP", "Stop / start the little phrase the cards play", [this] { autoplay = ! autoplay; if (! autoplay) { if (proc.loopOwnerId() == 4) proc.stopLoop(); } else hear (true); refreshBtns(); });
        btn (plantBtn, "PLANT IN EVOLVE", "The chosen kept sound becomes the seed of a new EVOLVE tree", [this] { if (auto* g = chosen()) { proc.evoSeedGenome (*g); note = "planted - open EVOLVE"; repaint(); } });
        btn (saveBtn, "SAVE", "Save the chosen kept sound into your folders / sound kits", [this]
        {
            auto* g = chosen(); if (g == nullptr) return;
            const double rate = proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0;
            auto snd = kk::PairLab::fromBuffer (proc.renderGenomeAudio (*g, rate, 3.0), rate, rate, g->name);
            saveToFolderMenu (proc, { snd }, &saveBtn, [safe = SafePointer<FeedPage> (this)] (String m) { if (safe != nullptr) { safe->note = m; safe->repaint(); } });
        });
        btn (forgetBtn, "FORGET MY TASTE", "Start learning your taste from zero", [this] { taste = {}; storeTaste(); repaint(); });
        dragWav.makeFile = [this] { auto* g = chosen(); return g != nullptr ? proc.exportGenomeWav (*g) : File(); };
        dragWav.setTooltip ("Drag the chosen kept sound into FL as a WAV");
        addAndMakeVisible (dragWav);
        loadTaste();
        deal (0);
        refreshBtns();
        startTimerHz (40);
    }
    ~FeedPage() override { stopTimer(); }
    void visibilityChanged() override { if (isVisible() && autoplay) hear (false); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("FEED", 24, 12, 120, 40, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText ("sounds come to you.  swipe:  right = KEEP   left = SKIP   up = MORE LIKE THIS   down = SOMETHING ELSE.  it learns your taste", 140, 18, getWidth() - 160, 28, Justification::centredLeft);
        drawDirections (g);
        // the next card peeks from behind
        drawCard (g, next, cardHome().translated (0, 14).reduced (16, 10), 0.0f, 0.5f);
        const auto home = cardHome().toFloat();
        const auto off = flying ? flyOff : drag;
        const float rot = off.x / 900.0f;
        drawCard (g, card, home.translated (off.x, off.y).toNearestInt(), rot, 1.0f);
        drawTaste (g);
        drawShelf (g);
        if (note.isNotEmpty()) { g.setColour (kk::accentText()); g.setFont (kk::modern::font (13.0f, true, 0.03f)); g.drawText (note, shelfArea().getX(), shelfArea().getY() - 24, 600, 20, Justification::centredLeft); }
    }
    void resized() override
    {
        const auto sh = shelfArea();
        stopBtn.setBounds (getWidth() - 260, 14, 110, 34); forgetBtn.setBounds (getWidth() - 260, tasteArea().getBottom() + 8, 236, 34);
        saveBtn.setBounds (sh.getRight() - 470, sh.getY() - 2, 110, 40); plantBtn.setBounds (sh.getRight() - 352, sh.getY() - 2, 190, 40); dragWav.setBounds (sh.getRight() - 154, sh.getY() - 4, 154, 44);
    }
    void mouseDown (const MouseEvent& e) override
    {
        if (flying) return;
        for (int i = 0; i < (int) shelf.size(); ++i)
            if (shelfSlot (i).contains (e.position)) { chosenIdx = i; proc.alcUse (shelf[(size_t) i].g, true); if (proc.loopOwnerId() == 4) proc.stopLoop(); autoplay = false; refreshBtns(); repaint(); return; }
        grabbing = cardHome().contains (e.getPosition());
        drag = {};
    }
    void mouseDrag (const MouseEvent& e) override { if (grabbing && ! flying) { drag = e.position - e.mouseDownPosition; repaint(); } }
    void mouseUp (const MouseEvent&) override
    {
        if (! grabbing || flying) return;
        grabbing = false;
        const int dir = drag.x > 150 ? 1 : drag.x < -150 ? 2 : drag.y < -130 ? 3 : drag.y > 130 ? 4 : 0;
        if (dir == 0)
        {
            if (drag.getDistanceFromOrigin() < 6) hear (true);   // a tap = hear it again
            drag = {}; repaint(); return;
        }
        swipe (dir);
    }
    void debugSwipes() { swipe (1); flying = false; deal (1); swipe (1); flying = false; deal (1); swipe (2); flying = false; deal (2); drag = { 90, -20 }; }
private:
    struct Recipe { int e = 0, b = 3; float m = 0.3f, s = 0.5f; uint32 seed = 1; };
    struct Card { Recipe r; KeysKillaProcessor::Genome g; };
    struct Taste { std::array<float, 5> e { 1, 1, 1, 1, 1 }, b { 1, 1, 1, 1, 1 }; float m = 0.4f, s = 0.5f; int swipes = 0; };
    Rectangle<int> cardHome() const { const int w = 420, h = 470; return { (getWidth() - 260) / 2 - w / 2 + 10, 110, w, h }; }
    Rectangle<int> tasteArea() const { return { getWidth() - 260, 110, 236, 350 }; }
    Rectangle<int> shelfArea() const { return { 24, getHeight() - 150, getWidth() - 48, 130 }; }
    Rectangle<float> shelfSlot (int i) const { const auto a = shelfArea().toFloat(); return { a.getX() + 10 + (float) i * 92.0f, a.getY() + 46, 80, 80 }; }
    const KeysKillaProcessor::Genome* chosen() const { return isPositiveAndBelow (chosenIdx, (int) shelf.size()) ? &shelf[(size_t) chosenIdx].g : nullptr; }

    static int pickWeighted (const std::array<float, 5>& w, Random& r, bool least)
    {
        if (least) { int k = 0; for (int i = 1; i < 5; ++i) if (w[(size_t) i] < w[(size_t) k]) k = i; return k; }
        float sum = 0; for (auto x : w) sum += std::pow (x, 1.5f);
        float u = r.nextFloat() * sum;
        for (int i = 0; i < 5; ++i) { u -= std::pow (w[(size_t) i], 1.5f); if (u <= 0) return i; }
        return 4;
    }
    Recipe invent (int how, const Recipe& from)
    {
        auto& rnd = Random::getSystemRandom();
        Recipe r;
        r.seed = (uint32) rnd.nextInt() | 1u;
        auto gauss = [&] { return (rnd.nextFloat() + rnd.nextFloat() + rnd.nextFloat() - 1.5f) * 0.6f; };
        if (how == 3) { r.e = from.e; r.b = from.b; r.m = jlimit (0.0f, 1.0f, from.m + gauss() * 0.25f); r.s = jlimit (0.0f, 1.0f, from.s + gauss() * 0.25f); return r; }
        const bool far = how == 4;
        r.e = pickWeighted (taste.e, rnd, far); r.b = pickWeighted (taste.b, rnd, far);
        r.m = jlimit (0.0f, 1.0f, (far ? 1.0f - taste.m : taste.m) + gauss() * 0.5f);
        r.s = jlimit (0.05f, 0.95f, (far ? 1.0f - taste.s : taste.s) + gauss() * 0.4f);
        return r;
    }
    Card make (const Recipe& r) { Card c; c.r = r; c.g = proc.alchemy (r.e, r.b, r.m, r.s, r.seed); return c; }
    void deal (int how)
    {
        if (how == 0 || ! next.g.valid()) card = make (invent (0, {}));
        else card = how == 3 ? make (invent (3, card.r)) : next;
        if (how == 3) {}   // MORE LIKE THIS: the new card grows from the one you pushed up
        next = make (invent (how == 4 ? 4 : 0, card.r));
        drag = {}; flyOff = {};
        hear (true);
        repaint();
    }
    void learn (const Recipe& r, float amount)
    {
        auto& we = taste.e[(size_t) r.e]; auto& wb = taste.b[(size_t) r.b];
        if (amount > 0) { we += amount; wb += amount; taste.m += (r.m - taste.m) * 0.3f * amount; taste.s += (r.s - taste.s) * 0.3f * amount; }
        else { we = jmax (0.15f, we * 0.85f); wb = jmax (0.15f, wb * 0.85f); }
        ++taste.swipes;
        storeTaste();
    }
    void swipe (int dir)
    {
        const Recipe r = card.r;
        if (dir == 1) { learn (r, 1.0f); shelf.insert (shelf.begin(), card); if (shelf.size() > 12) shelf.pop_back(); chosenIdx = 0; note = "kept: " + card.g.name; }
        else if (dir == 2) learn (r, -1.0f);
        else if (dir == 3) learn (r, 0.5f);
        flying = true; flyDir = dir; flyOff = drag; pending = dir;
    }
    void hear (bool note1)
    {
        if (! card.g.valid()) return;
        proc.alcUse (card.g, note1 && ! autoplay);
        if (! autoplay) return;
        // a little phrase in the sound's own register so you hear it in music, not as one note
        Random r ((int64) card.r.seed);
        const bool bass = kk::alc::exciters()[(size_t) card.r.e].dna.front().first == c808;
        static const int shapes[4][8] { { 0, 2, 4, 2, 7, 4, 2, 0 }, { 0, 4, 7, 9, 7, 4, 2, 4 }, { 7, 4, 2, 0, 2, 4, 0, -3 }, { 0, 0, 4, 2, 0, 7, 5, 4 } };
        const auto& sh = shapes[r.nextInt (4)];
        std::vector<kk::LoopNote> ln;
        float pos = 0;
        for (int i = 0; i < 8; ++i)
        {
            const float len = (i % 4 == 3) ? 1.0f : 0.5f + 0.5f * (float) r.nextInt (2);
            const int pitch = kk::mel::degreeToPitch (sh[i] * (bass ? 1 : 1), 9, kk::mel::scMinor, bass ? 0.0f : 0.5f) - (bass ? 12 : 0);
            ln.push_back ({ pos, len * 0.9f, pitch, bass });
            pos += len;
            if (pos >= 8.0f) break;
        }
        proc.playCustomLoop (ln, 8.0);
    }
    void refreshBtns() { stopBtn.setButtonText (autoplay ? "STOP" : "PLAY"); stopBtn.selected = autoplay; stopBtn.repaint(); }
    void loadTaste()
    {
        if (auto s = openSettings())
        {
            StringArray v; v.addTokens (s->getValue ("feedTaste"), ",", "");
            if (v.size() == 13) { for (int i = 0; i < 5; ++i) { taste.e[(size_t) i] = v[i].getFloatValue(); taste.b[(size_t) i] = v[5 + i].getFloatValue(); } taste.m = v[10].getFloatValue(); taste.s = v[11].getFloatValue(); taste.swipes = v[12].getIntValue(); }
        }
    }
    void storeTaste()
    {
        if (auto s = openSettings())
        {
            String v; for (auto x : taste.e) v << String (x, 3) << ","; for (auto x : taste.b) v << String (x, 3) << ",";
            v << String (taste.m, 3) << "," << String (taste.s, 3) << "," << taste.swipes;
            s->setValue ("feedTaste", v);
        }
    }
    // ---- drawing
    void drawDirections (Graphics& g)
    {
        const auto c = cardHome().toFloat();
        g.setFont (kk::modern::font (14.0f, true, 0.25f));
        auto lab = [&] (const String& s, Rectangle<float> r, Colour col, float lit) { g.setColour (col.withAlpha (0.35f + 0.6f * lit)); g.drawText (s, r.toNearestInt(), Justification::centred); };
        lab ("KEEP  >", { c.getRight() + 10, c.getCentreY() - 12, 150, 24 }, Colour (0xff36ff6a), jlimit (0.0f, 1.0f, drag.x / 150.0f));
        lab ("<  SKIP", { c.getX() - 160, c.getCentreY() - 12, 150, 24 }, Colour (0xffff3b5c), jlimit (0.0f, 1.0f, -drag.x / 150.0f));
        lab ("MORE LIKE THIS", { c.getCentreX() - 120, c.getY() - 30, 240, 22 }, Colour (0xffffd23f), jlimit (0.0f, 1.0f, -drag.y / 130.0f));
        lab ("SOMETHING ELSE", { c.getCentreX() - 120, c.getBottom() + 8, 240, 22 }, Colour (0xff22d3ee), jlimit (0.0f, 1.0f, drag.y / 130.0f));
    }
    void drawCard (Graphics& g, const Card& cd, Rectangle<int> rr, float rot, float alpha)
    {
        if (! cd.g.valid()) return;
        Graphics::ScopedSaveState ss (g);
        const auto r = rr.toFloat();
        g.addTransform (AffineTransform::rotation (rot, r.getCentreX(), r.getBottom()));
        const auto ec = Colour (kk::alc::exciters()[(size_t) cd.r.e].colour), bc = Colour (kk::alc::bodies()[(size_t) cd.r.b].colour);
        g.setColour (Colours::black.withAlpha (0.35f * alpha)); g.fillRoundedRectangle (r.translated (0, 10), 26);
        g.setGradientFill (ColourGradient (Colour (0xff12101f).withAlpha (alpha), r.getX(), r.getY(), Colour (0xff070810).withAlpha (alpha), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 26);
        g.setGradientFill (ColourGradient (ec.withAlpha (0.35f * alpha), r.getX(), r.getY(), bc.withAlpha (0.3f * alpha), r.getRight(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 26);
        g.setColour (Colours::white.withAlpha (0.25f * alpha)); g.drawRoundedRectangle (r.reduced (0.5f), 26, 1.5f);
        // the living matter of this sound
        const auto c = Point<float> (r.getCentreX(), r.getY() + r.getHeight() * 0.42f);
        const float rad = r.getWidth() * (0.18f + 0.12f * cd.r.s);
        Path blob; const int n = 64;
        for (int i = 0; i <= n; ++i)
        {
            const float a = (float) i / (float) n * MathConstants<float>::twoPi;
            const float crystal = std::abs (std::fmod (a / MathConstants<float>::twoPi * 6.0f, 1.0f) - 0.5f) * 2.0f;
            const float soft = 0.07f * std::sin (a * 3.0f + phase * 2.0f + (float) cd.r.seed) + 0.04f * std::sin (a * 5.0f - phase * 3.0f);
            const float k = (1.0f - cd.r.m) * (0.7f + 0.3f * crystal) + cd.r.m * (1.0f + soft) + 0.03f * std::sin (phase * 4.0f) * (float) (&cd == &card);
            const auto pt = c + Point<float> (std::cos (a), std::sin (a)) * rad * k;
            if (i == 0) blob.startNewSubPath (pt); else blob.lineTo (pt);
        }
        blob.closeSubPath();
        g.setColour (ec.withAlpha (0.25f * alpha)); g.fillPath (blob, AffineTransform::scale (1.4f, 1.4f, c.x, c.y));
        g.setGradientFill (ColourGradient (ec.brighter (0.5f).withAlpha (alpha), c.x - rad, c.y - rad, bc.darker (0.3f).withAlpha (alpha), c.x + rad, c.y + rad, false));
        g.fillPath (blob);
        g.setColour (Colours::white.withAlpha (alpha)); g.setFont (kk::modern::font (24.0f, true, 0.04f));
        g.drawFittedText (cd.g.name, r.withTrimmedTop (r.getHeight() * 0.72f).withHeight (34).reduced (18, 0).toNearestInt(), Justification::centred, 1, 0.7f);
        g.setColour (Colours::white.withAlpha (0.6f * alpha)); g.setFont (kk::modern::font (12.0f, true, 0.18f));
        g.drawText (String (kk::alc::exciters()[(size_t) cd.r.e].name) + "  x  " + kk::alc::bodies()[(size_t) cd.r.b].name,
                    r.withTrimmedTop (r.getHeight() * 0.72f + 40).withHeight (18).toNearestInt(), Justification::centred);
        // the stamp while you swipe
        if (&cd == &card)
        {
            const auto off = flying ? flyOff : drag;
            String stamp; Colour sc;
            if (off.x > 60) { stamp = "KEEP"; sc = Colour (0xff36ff6a); } else if (off.x < -60) { stamp = "SKIP"; sc = Colour (0xffff3b5c); }
            else if (off.y < -60) { stamp = "MORE"; sc = Colour (0xffffd23f); } else if (off.y > 60) { stamp = "ELSE"; sc = Colour (0xff22d3ee); }
            if (stamp.isNotEmpty())
            {
                const auto sr = Rectangle<float> (r.getCentreX() - 90, r.getY() + 30, 180, 56);
                g.setColour (sc); g.drawRoundedRectangle (sr, 10, 4.0f);
                g.setFont (kk::modern::font (34.0f, true, 0.2f)); g.drawText (stamp, sr.toNearestInt(), Justification::centred);
            }
        }
    }
    void drawTaste (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto a = tasteArea().toFloat();
        kk::modern::well (g, a, 14.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (14.0f, true, 0.25f));
        g.drawText ("YOUR TASTE", a.reduced (14, 10).withHeight (20).toNearestInt(), Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.1f));
        g.drawText (taste.swipes == 0 ? String ("learning from your swipes") : "learned from " + String (taste.swipes) + " swipes", a.reduced (14, 10).withTrimmedTop (22).withHeight (16).toNearestInt(), Justification::centredLeft);
        auto bars = [&] (float y, const std::vector<kk::alc::Part>& parts, const std::array<float, 5>& w)
        {
            float mx = 0.01f; for (auto x : w) mx = std::max (mx, x);
            for (int i = 0; i < 5; ++i)
            {
                const float yy = a.getY() + y + (float) i * 20.0f, u = w[(size_t) i] / mx;
                g.setColour (Colour (parts[(size_t) i].colour).withAlpha (0.3f + 0.6f * u)); g.fillRoundedRectangle (a.getX() + 14, yy, (a.getWidth() - 28) * u, 14, 4);
                g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (kk::modern::font (10.0f, true, 0.12f));
                g.drawText (parts[(size_t) i].name, (int) a.getX() + 18, (int) yy, 200, 14, Justification::centredLeft);
            }
        };
        bars (52, kk::alc::exciters(), taste.e);
        bars (162, kk::alc::bodies(), taste.b);
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (11.5f, true, 0.1f));
        g.drawText ("MATTER  " + kk::alc::matterWord (taste.m).toUpperCase(), (int) a.getX() + 14, (int) a.getBottom() - 44, (int) a.getWidth() - 28, 16, Justification::centredLeft);
        g.drawText ("SIZE  " + String (taste.s < 0.35f ? "SMALL" : taste.s > 0.65f ? "HUGE" : "MEDIUM"), (int) a.getX() + 14, (int) a.getBottom() - 26, (int) a.getWidth() - 28, 16, Justification::centredLeft);
    }
    void drawShelf (Graphics& g)
    {
        const auto& t = kk::theme();
        const auto a = shelfArea().toFloat();
        kk::modern::well (g, a, 14.0f);
        g.setColour (t.text); g.setFont (kk::modern::font (14.0f, true, 0.25f));
        g.drawText ("KEPT", (int) a.getX() + 14, (int) a.getY() + 10, 200, 20, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.05f));
        g.drawText (shelf.empty() ? String ("swipe right to keep a sound - it lands here") : String ("click = it is on your keys, then SAVE / PLANT / DRAG WAV"), (int) a.getX() + 70, (int) a.getY() + 12, 600, 18, Justification::centredLeft);
        for (int i = 0; i < (int) shelf.size(); ++i)
        {
            const auto s = shelfSlot (i);
            if (s.getRight() > a.getRight() - 480) break;
            const auto ec = Colour (kk::alc::exciters()[(size_t) shelf[(size_t) i].r.e].colour), bc = Colour (kk::alc::bodies()[(size_t) shelf[(size_t) i].r.b].colour);
            g.setGradientFill (ColourGradient (ec, s.getX(), s.getY(), bc, s.getRight(), s.getBottom(), false));
            g.fillEllipse (s.reduced (8));
            if (i == chosenIdx) { g.setColour (Colours::white); g.drawEllipse (s.reduced (4), 2.5f); }
        }
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.04f;
        if (flying)
        {
            const Point<float> to = flyDir == 1 ? Point<float> (900, 80) : flyDir == 2 ? Point<float> (-900, 80) : flyDir == 3 ? Point<float> (0, -800) : Point<float> (0, 800);
            flyOff += (to - flyOff) * 0.22f;
            if (flyOff.getDistanceFrom (to) < 60) { flying = false; deal (pending); }
        }
        repaint();
    }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton stopBtn { lnf }, plantBtn { lnf }, saveBtn { lnf }, forgetBtn { lnf };
    DragFileButton dragWav { "DRAG WAV", TC (0xff36ff6a) };
    Card card, next;
    std::vector<Card> shelf;
    Taste taste;
    Point<float> drag, flyOff;
    bool grabbing = false, flying = false, autoplay = true;
    int flyDir = 0, pending = 0, chosenIdx = -1;
    float phase = 0;
    String note;
};
