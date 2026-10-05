// v0.43 LIFE page (included by PluginEditor.cpp): no piano roll - melodies come out of physics and behaviour.
// GRAVITY (throw impulses into a field), PREDATOR (a creature hunts), SWARM (a flock holds the harmony),
// and METABOLISM: the loop ages every time it repeats until you touch it.
class LifePage : public Component, private Timer
{
public:
    enum Mode { mGravity, mPredator, mSwarm };
    LifePage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        auto btn = [this] (HotButton& b, const String& t, const String& tip, std::function<void()> fn) { b.setButtonText (t); b.framed = true; b.setTooltip (tip); b.onClick = std::move (fn); addAndMakeVisible (b); };
        static const char* names[] { "GRAVITY", "PREDATOR", "SWARM" };
        static const char* tips[] { "Throw impulses into a field: mass = how hard, bounces = the rhythm, friction = it speeds up / slows down by itself",
                                    "A creature hunts its prey: the chase climbs, circling = one chord, the pounce = the accent with a slide",
                                    "A flock holds the harmony: together = a tight chord in your key, startled = a cascade over the octaves" };
        for (int i = 0; i < 3; ++i)
        {
            modeBtns[(size_t) i] = std::make_unique<HotButton> (lnf, names[i]);
            modeBtns[(size_t) i]->framed = true; modeBtns[(size_t) i]->setTooltip (tips[i]);
            modeBtns[(size_t) i]->onClick = [this, i] { mode = i; age = 0; rebuild(); refreshMode(); };
            addAndMakeVisible (*modeBtns[(size_t) i]);
        }
        btn (playBtn, "PLAY", "Play it with the sound on your keys (again = stop)", [this] { if (proc.loopPlaying() && proc.loopOwnerId() == 4) { proc.stopLoop(); } else { age = 0; play(); } refreshPlay(); });
        btn (newBtn, "NEW LIFE", "A new field / hunt / flock", [this] { seed = (uint32) Random::getSystemRandom().nextInt() | 1u; if (mode == mGravity) field = kk::live::randomField (seed); if (mode == mSwarm) startles = { (float) (4 + Random::getSystemRandom().nextInt (bars * 4 - 6)) }; age = 0; rebuild(); });
        btn (clearBtn, "CLEAR", "GRAVITY: remove every throw and line.  SWARM: no more frights", [this] { field.throws.clear(); field.ledges.clear(); startles.clear(); rebuild(); });
        btn (escapeBtn, "HUNT", "HUNT: the chase climbs.  ESCAPE: the creature runs away - the lines fall", [this] { escape = ! escape; escapeBtn.setButtonText (escape ? "ESCAPE" : "HUNT"); rebuild(); });
        btn (touchBtn, "TOUCH", "Touch the loop: it is young again (METABOLISM starts over)", [this] { age = 0; if (proc.loopPlaying() && proc.loopOwnerId() == 4) play(); repaint(); });
        btn (barsBtn, "4 BARS", "2, 4 or 8 bars", [this] { bars = bars == 2 ? 4 : bars == 4 ? 8 : 2; barsBtn.setButtonText (String (bars) + " BARS"); rebuild(); });
        newBtn.hero = true;
        for (int k = 0; k < 12; ++k) keyBox.addItem (kk::mel::keyName (k), k + 1);
        keyBox.setSelectedId (10, dontSendNotification); keyBox.onChange = [this] { rebuild(); }; addAndMakeVisible (keyBox);
        for (int sc = 0; sc < kk::mel::numScales; ++sc) scaleBox.addItem (kk::mel::scaleName (sc), sc + 1);
        scaleBox.setSelectedId (1, dontSendNotification); scaleBox.onChange = [this] { rebuild(); }; addAndMakeVisible (scaleBox);
        auto knob = [this] (std::atomic<float>& v, const char* n, Colour a, Colour b)
        {
            auto k = std::make_unique<RackKnob> (v, n, 0, 1, v.load(), a, b);
            auto inner = k->onValueChange; k->onValueChange = [this, inner] { inner(); dirty = true; };
            addAndMakeVisible (*k); return k;
        };
        const Colour c1 (0xff36ff6a), c2 (0xff22d3ee);
        kGravity = knob (aGravity, "GRAVITY", c1, c2); kFriction = knob (aFriction, "FRICTION", c1, c2); kBounce = knob (aBounce, "BOUNCE", c1, c2); kMass = knob (aMass, "MASS", c1, c2);
        kHunger = knob (aHunger, "HUNGER", Colour (0xffff3b5c), Colour (0xffff8a3d)); kPrey = knob (aPrey, "PREY SPEED", Colour (0xffff3b5c), Colour (0xffff8a3d)); kRate = knob (aRate, "NOTE RATE", Colour (0xffff3b5c), Colour (0xffff8a3d));
        kBirds = knob (aBirds, "BIRDS", Colour (0xffa78bfa), c2); kCohesion = knob (aCohesion, "TOGETHER", Colour (0xffa78bfa), c2); kCalm = knob (aCalm, "CALM", Colour (0xffa78bfa), c2);
        kMeta = knob (aMeta, "METABOLISM", Colour (0xffffd23f), Colour (0xffff8a3d)); kLife = knob (aLife, "LIFESPAN", Colour (0xffffd23f), Colour (0xffff8a3d));
        kLife->valueText = [] (double v) { return String (lifespanOf ((float) v)) + " loops"; };
        dragMidi.makeFile = [this] { return exportMidi(); };
        dragMidi.setTooltip ("This music as MIDI (with the slides as pitch-bend) - drop it on any instrument in FL");
        addAndMakeVisible (dragMidi);
        field = kk::live::randomField (seed);
        refreshMode();
        rebuild();
        startTimerHz (30);
    }
    ~LifePage() override { stopTimer(); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        pageBackdrop (g, *this);
        g.setColour (t.text); g.setFont (kk::modern::font (30.0f, true, 0.06f));
        g.drawText ("LIFE", 24, 12, 120, 40, Justification::centredLeft);
        static const char* subs[] { "drag in the field = throw (longer drag = faster).  right-drag = draw a ledge.  mass = how hard, bounces = rhythm, friction = rubato",
                                    "the hunter chases (the line climbs), circles its prey (one chord), then pounces (the accent slides in)",
                                    "together the flock is one chord.  click the field = frighten it - it scatters into a cascade" };
        g.setColour (t.dim); g.setFont (kk::modern::font (13.5f, true, 0.04f));
        g.drawText (subs[mode], 140, 18, getWidth() - 160, 28, Justification::centredLeft);
        const auto f = fieldArea().toFloat();
        g.setColour (Colour (0xff04060a).withAlpha (t.night ? 0.8f : 0.9f)); g.fillRoundedRectangle (f.expanded (6), 16);
        {
            Graphics::ScopedSaveState ss (g); g.reduceClipRegion (f.toNearestInt());
            if (mode == mGravity) drawGravity (g, f); else if (mode == mPredator) drawPredator (g, f); else drawSwarm (g, f);
        }
        drawRoll (g, rollArea().toFloat());
        drawMetabolism (g, metaArea().toFloat());
    }
    void resized() override
    {
        int x = 140; for (auto& b : modeBtns) { b->setBounds (x, 56, 120, 34); x += 126; }
        x += 14; keyBox.setBounds (x, 59, 60, 28); scaleBox.setBounds (x + 66, 59, 160, 28); barsBtn.setBounds (x + 232, 56, 84, 34);
        newBtn.setBounds (x + 330, 56, 120, 34); clearBtn.setBounds (x + 456, 56, 80, 34); escapeBtn.setBounds (x + 456, 56, 80, 34);
        const auto side = sideArea();
        playBtn.setBounds (side.getX(), side.getY(), side.getWidth(), 46);
        dragMidi.setBounds (side.getX(), side.getY() + 56, side.getWidth(), 46);
        const int kw = side.getWidth() / 2, kh = 112;
        auto place = [&] (std::vector<RackKnob*> ks)
        {
            for (int i = 0; i < (int) ks.size(); ++i) ks[(size_t) i]->setBounds (side.getX() + (i % 2) * kw, side.getY() + 120 + (i / 2) * (kh + 6), kw - 4, kh);
        };
        place ({ kGravity.get(), kFriction.get(), kBounce.get(), kMass.get() });
        place ({ kHunger.get(), kPrey.get(), kRate.get() });
        place ({ kBirds.get(), kCohesion.get(), kCalm.get() });
        const auto m = metaArea();
        kMeta->setBounds (m.getRight() - 300, m.getY() - 4, 90, m.getHeight() + 6); kLife->setBounds (m.getRight() - 205, m.getY() - 4, 90, m.getHeight() + 6);
        touchBtn.setBounds (m.getRight() - 105, m.getY() + 18, 100, 38);
    }
    void mouseDown (const MouseEvent& e) override
    {
        const auto f = fieldArea().toFloat();
        if (! f.contains (e.position)) return;
        const auto n = norm (e.position);
        if (mode == mGravity) { dragStart = n; dragNow = n; dragKind = e.mods.isPopupMenu() ? 2 : 1; return; }
        if (mode == mSwarm)
        {
            const float at = proc.loopPlaying() && proc.loopOwnerId() == 4 ? std::fmod (proc.loopBeat.load() + 0.5f, (float) bars * 4.0f) : nowBeat;
            startles.push_back (at); age = 0; rebuild();
        }
    }
    void mouseDrag (const MouseEvent& e) override { if (dragKind > 0) { dragNow = norm (e.position); repaint (fieldArea()); } }
    void mouseUp (const MouseEvent&) override
    {
        if (dragKind == 1)
        {
            kk::live::Throw t; t.x = dragStart.x; t.y = dragStart.y;
            t.vx = (dragNow.x - dragStart.x) * 3.0f; t.vy = (dragNow.y - dragStart.y) * 3.0f; t.mass = aMass.load();
            t.at = proc.loopPlaying() && proc.loopOwnerId() == 4 ? std::floor (std::fmod (proc.loopBeat.load() + 1.0f, (float) bars * 4.0f)) : std::floor (nowBeat);
            if (field.throws.size() >= 8) field.throws.erase (field.throws.begin());
            field.throws.push_back (t);
        }
        else if (dragKind == 2 && dragStart.getDistanceFrom (dragNow) > 0.03f)
        {
            if (field.ledges.size() >= 6) field.ledges.erase (field.ledges.begin());
            field.ledges.push_back ({ dragStart.x, dragStart.y, dragNow.x, dragNow.y });
        }
        if (dragKind > 0) { dragKind = 0; age = 0; rebuild(); }
    }
    void debugMode (int m) { mode = m; rebuild(); refreshMode(); nowBeat = 6.0f; }
private:
    static int lifespanOf (float v) { return 2 + (int) std::round (v * 14.0f); }
    Rectangle<int> sideArea() const { return { getWidth() - 250, 104, 226, getHeight() - 210 }; }
    Rectangle<int> fieldArea() const { return { 24, 104, getWidth() - 24 - 270, getHeight() - 104 - 200 }; }
    Rectangle<int> rollArea() const { const auto f = fieldArea(); return { f.getX(), f.getBottom() + 14, f.getWidth(), 86 }; }
    Rectangle<int> metaArea() const { return { 24, getHeight() - 78, getWidth() - 48, 62 }; }
    Point<float> norm (Point<float> p) const { const auto f = fieldArea().toFloat(); return { jlimit (0.0f, 1.0f, (p.x - f.getX()) / f.getWidth()), jlimit (0.0f, 1.0f, (p.y - f.getY()) / f.getHeight()) }; }
    Point<float> px (Point<float> n) const { const auto f = fieldArea().toFloat(); return { f.getX() + n.x * f.getWidth(), f.getY() + n.y * f.getHeight() }; }
    int key() const { return jmax (0, keyBox.getSelectedId() - 1); }
    int scale() const { return jmax (0, scaleBox.getSelectedId() - 1); }

    void refreshMode()
    {
        for (int i = 0; i < 3; ++i) { modeBtns[(size_t) i]->selected = mode == i; modeBtns[(size_t) i]->repaint(); }
        for (auto* k : { kGravity.get(), kFriction.get(), kBounce.get(), kMass.get() }) k->setVisible (mode == mGravity);
        for (auto* k : { kHunger.get(), kPrey.get(), kRate.get() }) k->setVisible (mode == mPredator);
        for (auto* k : { kBirds.get(), kCohesion.get(), kCalm.get() }) k->setVisible (mode == mSwarm);
        clearBtn.setVisible (mode != mPredator); escapeBtn.setVisible (mode == mPredator);
        repaint();
    }
    void rebuild()
    {
        dirty = false;
        field.gravity = aGravity.load(); field.friction = aFriction.load(); field.bounce = aBounce.load();
        if (mode == mGravity) { res = kk::live::gravity (field, key(), scale(), bars); hunt = {}; flock = {}; }
        else if (mode == mPredator)
        {
            kk::live::Hunt h; h.hunger = aHunger.load(); h.preySpeed = aPrey.load(); h.rate = aRate.load(); h.escape = escape; h.seed = seed;
            hunt = kk::live::predator (h, key(), scale(), bars); res = hunt;
        }
        else
        {
            kk::live::Flock fl; fl.birds = 6 + (int) std::round (aBirds.load() * 18.0f); fl.cohesion = aCohesion.load(); fl.calm = aCalm.load(); fl.startles = startles; fl.seed = seed;
            flock = kk::live::swarm (fl, key(), scale(), bars); res = flock;
        }
        base = res.notes;
        if (proc.loopPlaying() && proc.loopOwnerId() == 4) play();
        repaint();
    }
    kk::live::Metabolism meta() const { kk::live::Metabolism m; m.rate = aMeta.load(); m.lifespan = lifespanOf (aLife.load()); m.seed = seed; return m; }
    void play()
    {
        res.notes = kk::live::age (base, age, meta(), key(), scale(), res.beats);
        if (res.notes.empty()) { proc.stopLoop(); refreshPlay(); repaint(); return; }
        std::vector<kk::LoopNote> ln; for (auto& n : res.notes) ln.push_back ({ n.start, n.len, n.pitch, false });
        proc.playCustomLoop (ln, res.beats);
        refreshPlay();
    }
    void refreshPlay() { const bool on = proc.loopPlaying() && proc.loopOwnerId() == 4; playBtn.setButtonText (on ? "STOP" : "PLAY"); playBtn.selected = on; playBtn.repaint(); }
    File exportMidi()
    {
        if (res.notes.empty()) return {};
        const double bpm = proc.lastBpm.load() > 30 ? proc.lastBpm.load() : 140.0;
        auto mf = kk::live::toMidi (res, bpm);
        static const char* nm[] { "Gravity", "Predator", "Swarm" };
        auto dir = File::getSpecialLocation (File::tempDirectory).getChildFile ("EVOLVE Life"); dir.createDirectory();
        auto f = dir.getChildFile (String (nm[mode]) + " - " + kk::mel::keyName (key()) + " " + kk::mel::scaleName (scale()) + " - " + String (roundToInt (bpm)) + "BPM.mid");
        f.deleteFile(); if (FileOutputStream os { f }; os.openedOk()) mf.writeTo (os, 1);
        return f;
    }
    float beatNow() const { return proc.loopPlaying() && proc.loopOwnerId() == 4 && proc.loopBeat.load() >= 0 ? proc.loopBeat.load() : nowBeat; }
    const kk::live::Frame* frameAt (float beat) const
    {
        if (res.frames.empty()) return nullptr;
        const float u = jlimit (0.0f, 0.9999f, beat / std::max (1.0f, res.beats));
        return &res.frames[(size_t) (u * (float) res.frames.size())];
    }
    void flashes (Graphics& g, float beat, std::function<Point<float> (const kk::live::LNote&)> where, Colour c)
    {
        for (auto& n : res.notes)
        {
            const float d = beat - n.start;
            if (d < 0 || d > 0.6f) continue;
            const auto p = where (n); const float a = 1.0f - d / 0.6f, r = 8.0f + 30.0f * (1.0f - a);
            g.setColour (c.withAlpha (0.6f * a)); g.drawEllipse (p.x - r, p.y - r, r * 2, r * 2, 2.0f);
        }
    }
    void drawGravity (Graphics& g, Rectangle<float> f)
    {
        // lanes = the scale degrees on the floor
        for (int i = 0; i <= 13; ++i) { g.setColour (Colour (0xff36ff6a).withAlpha (i % 7 == 3 ? 0.18f : 0.06f)); g.fillRect (f.getX() + f.getWidth() * ((float) i + 0.5f) / 13.0f, f.getY(), 1.0f, f.getHeight()); }
        g.setColour (Colour (0xff36ff6a).withAlpha (0.5f)); g.fillRect (f.getX(), f.getBottom() - 3, f.getWidth(), 3.0f);
        for (auto& L : field.ledges) { g.setColour (Colour (0xff22d3ee)); g.drawLine (Line<float> (px ({ L.x0, L.y0 }), px ({ L.x1, L.y1 })), 4.0f); }
        for (auto& t : field.throws)
        {
            const auto a = px ({ t.x, t.y });
            g.setColour (Colours::white.withAlpha (0.35f)); g.drawEllipse (a.x - 6, a.y - 6, 12, 12, 1.2f);
            g.drawArrow (Line<float> (a, a + Point<float> (t.vx, t.vy) * f.getWidth() * 0.12f), 1.4f, 7, 7);
        }
        const float beat = beatNow();
        if (auto* fr = frameAt (beat))
            for (int i = 0; i < (int) fr->pos.size(); ++i)
            {
                if (fr->pos[(size_t) i].x < 0) continue;
                const auto p = px (fr->pos[(size_t) i]);
                const float m = i < (int) field.throws.size() ? field.throws[(size_t) i].mass : 0.5f, r = 7.0f + 10.0f * m;
                g.setColour (Colour (0xff36ff6a).withAlpha (0.25f)); g.fillEllipse (p.x - r * 1.7f, p.y - r * 1.7f, r * 3.4f, r * 3.4f);
                g.setGradientFill (ColourGradient (Colours::white, p.x - r * 0.4f, p.y - r * 0.4f, Colour (0xff36ff6a), p.x + r, p.y + r, true)); g.fillEllipse (p.x - r, p.y - r, r * 2, r * 2);
            }
        flashes (g, beat, [this] (const kk::live::LNote& n) { const auto* fr = frameAt (n.start); Point<float> best (0.5f, 1.0f);
                                                              if (fr) { float bd = 1e9f; for (auto& q : fr->pos) if (q.x >= 0 && std::abs (q.y - 1.0f) < bd) { bd = std::abs (q.y - 1.0f); best = q; } }
                                                              return px (best); }, Colour (0xff36ff6a));
        if (dragKind > 0)
        {
            g.setColour (dragKind == 1 ? Colours::white : Colour (0xff22d3ee));
            if (dragKind == 1) g.drawArrow (Line<float> (px (dragStart), px (dragNow)), 2.0f, 12, 12); else g.drawLine (Line<float> (px (dragStart), px (dragNow)), 4.0f);
        }
        if (field.throws.empty()) { g.setColour (Colours::white.withAlpha (0.5f)); g.setFont (kk::modern::font (18.0f, true, 0.1f)); g.drawText ("drag here to throw", f.toNearestInt(), Justification::centred); }
    }
    void drawPredator (Graphics& g, Rectangle<float> f)
    {
        const float beat = beatNow();
        if (auto* fr = frameAt (beat); fr != nullptr && fr->pos.size() >= 2)
        {
            const size_t idx = (size_t) (&*fr - &res.frames[0]);
            // the trail
            Path trail; bool started = false;
            for (size_t k = idx > 60 ? idx - 60 : 0; k <= idx; ++k) { const auto p = px (res.frames[k].pos[0]); if (! started) { trail.startNewSubPath (p); started = true; } else trail.lineTo (p); }
            g.setColour (Colour (0xffff3b5c).withAlpha (0.45f)); g.strokePath (trail, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
            const auto me = px (fr->pos[0]), prey = px (fr->pos[1]);
            const int st = idx < hunt.stateAt.size() ? hunt.stateAt[idx] : 0;
            g.setColour (Colour (0xffffd23f)); g.fillEllipse (prey.x - 7, prey.y - 7, 14, 14);
            g.setColour (Colour (0xffffd23f).withAlpha (0.3f)); g.drawEllipse (prey.x - 14, prey.y - 14, 28, 28, 1.5f);
            // the hunter: a sharp shape facing its prey
            const float ang = std::atan2 (prey.y - me.y, prey.x - me.x);
            Path body; body.addTriangle (18, 0, -12, -10, -12, 10);
            g.setColour (st == kk::live::hsPounce ? Colours::white : Colour (0xffff3b5c));
            g.fillPath (body, AffineTransform::rotation (ang).translated (me.x, me.y));
            g.setColour (Colours::white.withAlpha (0.8f)); g.setFont (kk::modern::font (13.0f, true, 0.3f));
            g.drawText (kk::live::huntStateName (st), Rectangle<float> (me.x - 60, me.y - 34, 120, 16), Justification::centred);
        }
        flashes (g, beat, [this] (const kk::live::LNote& n) { const auto* fr = frameAt (n.start); return fr ? px (fr->pos[0]) : Point<float>(); }, Colour (0xffff3b5c));
    }
    void drawSwarm (Graphics& g, Rectangle<float> f)
    {
        const float beat = beatNow();
        for (auto s : startles) { const float x = f.getX() + f.getWidth() * s / std::max (1.0f, res.beats); g.setColour (Colour (0xffff3b5c).withAlpha (0.4f)); g.fillRect (x - 1, f.getBottom() - 16, 2.0f, 16.0f); }
        if (auto* fr = frameAt (beat))
        {
            const size_t idx = (size_t) (fr - &res.frames[0]);
            const float spread = idx < flock.spreadAt.size() ? flock.spreadAt[idx] : 0;
            const auto col = Colour (0xffa78bfa).interpolatedWith (Colour (0xffff3b5c), jlimit (0.0f, 1.0f, (spread - 0.08f) * 6.0f));
            for (size_t i = 0; i < fr->pos.size(); ++i)
            {
                const auto p = px (fr->pos[i]);
                const auto prev = idx > 0 && i < res.frames[idx - 1].pos.size() ? px (res.frames[idx - 1].pos[i]) : p;
                const float ang = std::atan2 (p.y - prev.y, p.x - prev.x);
                Path bird; bird.startNewSubPath (-8, -6); bird.lineTo (6, 0); bird.lineTo (-8, 6);
                g.setColour (col); g.strokePath (bird, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::rotation (ang).translated (p.x, p.y));
            }
            g.setColour (Colours::white.withAlpha (0.6f)); g.setFont (kk::modern::font (13.0f, true, 0.3f));
            g.drawText (spread > 0.16f ? "SCATTERED - CASCADE" : "TOGETHER - CHORD", f.reduced (14).toNearestInt(), Justification::topRight);
        }
        flashes (g, beat, [this, f] (const kk::live::LNote& n) { return Point<float> (f.getX() + f.getWidth() * 0.5f, f.getBottom() - f.getHeight() * (float) (n.pitch - 36) / 60.0f); }, Colour (0xffa78bfa));
    }
    void drawRoll (Graphics& g, Rectangle<float> r)
    {
        kk::modern::well (g, r, 10.0f);
        if (res.notes.empty()) return;
        int lo = 127, hi = 0; for (auto& n : res.notes) { lo = std::min (lo, n.pitch); hi = std::max (hi, n.pitch); }
        const float rows = (float) std::max (12, hi - lo + 1); const auto a = r.reduced (10, 8);
        const float beat = beatNow();
        static const uint32 cols[] { 0xff36ff6a, 0xffff3b5c, 0xffa78bfa };
        for (auto& n : res.notes)
        {
            const float x = a.getX() + a.getWidth() * n.start / res.beats, w = std::max (2.0f, a.getWidth() * n.len / res.beats - 1);
            const float y = a.getBottom() - a.getHeight() * (float) (n.pitch - lo + 1) / rows;
            g.setColour (Colour (cols[mode]).withAlpha (0.35f + 0.6f * n.vel)); g.fillRoundedRectangle (x, y, w, std::max (2.5f, a.getHeight() / rows - 1), 1.5f);
            if (std::abs (n.bend) > 0.01f) { g.setColour (Colours::white); g.drawLine (x - 6, y + 6, x, y + 1, 1.2f); }
        }
        g.setColour (Colours::white.withAlpha (0.7f)); g.fillRect (a.getX() + a.getWidth() * std::fmod (beat, res.beats) / res.beats, a.getY(), 1.5f, a.getHeight());
        g.setColour (kk::theme().dim); g.setFont (kk::modern::font (11.0f, true, 0.1f));
        g.drawText (String ((int) res.notes.size()) + " notes", r.reduced (10, 4).toNearestInt(), Justification::topRight);
    }
    void drawMetabolism (Graphics& g, Rectangle<float> r)
    {
        const auto& t = kk::theme();
        kk::modern::well (g, r, 12.0f);
        const auto m = meta();
        const float life = kk::live::lifeLeft (m, age);
        g.setColour (Colour (0xffffd23f)); g.setFont (kk::modern::font (14.0f, true, 0.25f));
        g.drawText ("METABOLISM", (int) r.getX() + 16, (int) r.getY() + 6, 200, 18, Justification::centredLeft);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.5f, true, 0.03f));
        g.drawText (age == 0 ? String ("young - every repeat it breathes, oxidises and finally falls apart.  TOUCH = young again")
                             : life > 0 ? "repeat " + String (age) + ": breathing, oxidising" : "too old - it is falling apart. move on, or TOUCH it",
                    (int) r.getX() + 16, (int) r.getY() + 26, (int) r.getWidth() - 360, 16, Justification::centredLeft);
        const auto bar = Rectangle<float> (r.getX() + 16, r.getBottom() - 16, r.getWidth() - 360, 7);
        g.setColour (Colours::black.withAlpha (0.4f)); g.fillRoundedRectangle (bar, 3.5f);
        g.setColour (Colour (0xff36ff6a).interpolatedWith (Colour (0xff8a5a2b), 1.0f - life)); g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * std::max (0.02f, life)), 3.5f);
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        if (dirty) rebuild();
        const bool mine = proc.loopPlaying() && proc.loopOwnerId() == 4;
        if (mine)
        {
            const float b = proc.loopBeat.load();
            if (b >= 0 && b + 0.5f < lastBeat && aMeta.load() > 0.0f) { ++age; play(); }   // a new repeat: the loop ages
            lastBeat = b;
        }
        else { nowBeat = std::fmod (nowBeat + 1.0f / 30.0f * 2.0f, std::max (1.0f, res.beats)); lastBeat = -1; }
        if (playBtn.selected != mine) refreshPlay();
        repaint (fieldArea().expanded (8)); repaint (rollArea()); repaint (metaArea());
    }

    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    std::array<std::unique_ptr<HotButton>, 3> modeBtns;
    HotButton playBtn { lnf }, newBtn { lnf }, clearBtn { lnf }, escapeBtn { lnf }, touchBtn { lnf }, barsBtn { lnf };
    ComboBox keyBox, scaleBox;
    DragFileButton dragMidi { "DRAG MIDI", TC (0xff36ff6a) };
    std::atomic<float> aGravity { 0.45f }, aFriction { 0.3f }, aBounce { 0.75f }, aMass { 0.6f }, aHunger { 0.6f }, aPrey { 0.5f }, aRate { 0.5f },
                       aBirds { 0.35f }, aCohesion { 0.7f }, aCalm { 0.5f }, aMeta { 0.5f }, aLife { 0.4f };
    std::unique_ptr<RackKnob> kGravity, kFriction, kBounce, kMass, kHunger, kPrey, kRate, kBirds, kCohesion, kCalm, kMeta, kLife;
    kk::live::GravityField field;
    kk::live::Result res;
    kk::live::HuntResult hunt;
    kk::live::SwarmResult flock;
    std::vector<kk::live::LNote> base;
    std::vector<float> startles { 9.0f };
    int mode = mGravity, bars = 4, age = 0, dragKind = 0;
    bool escape = false, dirty = false;
    uint32 seed = 20261005u;
    float nowBeat = 0, lastBeat = -1;
    Point<float> dragStart, dragNow;
};
