// v0.45 EVOLVE FX PRO - WORLDS pages (included by FxMainPage.h after FxTouchPages.h): CLUB, SEASONING, ENGINE, MOOD WORDS, DRAW
// Every EVOLVE world has an FX PRO twin: PARTY -> CLUB, COOK -> SEASONING, GARAGE -> ENGINE, WORDS -> MOOD WORDS.
// No knobs, no numbers: spin, flash, shake, rev, shift, type, draw.  Timers only animate while the page is showing.

static Rectangle<int> worldArea (const Component& c) { return { 24, 100, c.getWidth() - 48, c.getHeight() - 168 }; }
static Rectangle<float> worldHintRow (const Component& c) { return { 24.0f, (float) c.getHeight() - 54.0f, (float) c.getWidth() - 48.0f, 36.0f }; }
static void worldFrame (Graphics& g, Rectangle<float> A, Colour top, Colour bottom)
{
    g.setGradientFill (ColourGradient (top, A.getCentreX(), A.getY(), bottom, A.getCentreX(), A.getBottom(), false));
    g.fillRoundedRectangle (A, 18);
}
static void worldOff (Graphics& g, Rectangle<float> A, bool on, Colour col)
{
    if (on) return;
    g.setColour (Colours::black.withAlpha (0.35f)); g.fillRoundedRectangle (A, 18);
    g.setColour (col.withAlpha (0.9f)); g.setFont (kk::modern::font (14.0f, true, 0.3f));
    g.drawText ("OFF  -  TOUCH ANYTHING TO START", A.withHeight (30).translated (0, A.getHeight() - 40), Justification::centred);
}

// ---------------------------------------------------------------- CLUB ----------------------------------------------------------------
class ClubPage : public Component, public SettableTooltipClient, private Timer
{
public:
    ClubPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        onBtn.setButtonText ("OFF"); onBtn.framed = true; addAndMakeVisible (onBtn);
        onBtn.setTooltip ("CLUB on / off - your track plays in the club");
        onBtn.onClick = [this] { proc.club.on = ! proc.club.on.load(); refresh(); };
        calmBtn.setButtonText ("LIGHTS ON"); calmBtn.framed = true; addAndMakeVisible (calmBtn);
        calmBtn.setTooltip ("The party stops: the ball stands still, the crowd calms down, the lever goes back up");
        calmBtn.onClick = [this] { proc.club.calm(); refresh(); };
        setTooltip ("Spin the disco ball in circles = the sweep (faster = deeper).  Tap the strobe = a burst, hold it = a gate; tap faster = a faster strobe.  "
                    "Drag the crowd up = the build-up, sideways = how full the club is.  Pull the DROP lever down and let go = the filter drop.");
        spinDir = 1.0f;
        startTimerHz (30);
        refresh();
    }
    static Colour pink()   { return Colour (0xffff4fd8); }
    static Colour violet() { return Colour (0xff8b5cff); }
    static Colour cyan()   { return Colour (0xff3ee8ff); }
    static Colour gold()   { return Colour (0xffffd23f); }
    static Colour beam (int k) { static const uint32 c[] { 0xffff4fd8, 0xff3ee8ff, 0xffffd23f, 0xff8b5cff, 0xff36ff9a }; return Colour (c[(size_t) (k % 5)]); }
    void paint (Graphics& g) override
    {
        proHeader (g, *this, "CLUB", "WORLDS  -  THE PARTY'S TWIN", "your track plays in a club.  spin the disco ball, flash the strobe, raise the crowd, fill the room - and pull the DROP", pink());
        const auto A = area().toFloat();
        const auto& st = proc.club;
        const bool on = st.on.load();
        const Geo G = geo();
        const float spin = st.spin.load(), crowd = st.crowd.load(), full = st.fullness.load();
        const float glow = on ? 0.35f + 0.65f * spin : 0.25f;
        worldFrame (g, A, Colour (0xff160726), Colour (0xff050208));
        g.saveState(); g.reduceClipRegion (A.toNearestInt());
        // the dance floor in perspective, its tiles light up with the beat of the ball
        const float fy = G.floorY, fb = A.getBottom();
        const Point<float> vp (A.getCentreX(), fy - A.getHeight() * 0.9f);
        const int rows = 7, cols = 14, step = (int) (phase * (1.5f + 5.0f * spin));
        for (int r = 0; r < rows; ++r)
        {
            const float t0 = std::pow ((float) r / rows, 1.5f), t1 = std::pow ((float) (r + 1) / rows, 1.5f);
            const float y0 = fy + (fb - fy) * t0, y1 = fy + (fb - fy) * t1;
            auto xAt = [&] (float u, float y) { const float k = (y - vp.y) / (fb - vp.y); return A.getCentreX() + (u - 0.5f) * A.getWidth() * 1.5f * k; };
            for (int c = 0; c < cols; ++c)
            {
                const float u0 = (float) c / cols, u1 = (float) (c + 1) / cols;
                Path q; q.startNewSubPath (xAt (u0, y0), y0); q.lineTo (xAt (u1, y0), y0); q.lineTo (xAt (u1, y1), y1); q.lineTo (xAt (u0, y1), y1); q.closeSubPath();
                const bool lit = (kk::hash32 ((uint32) (r * 131 + c * 17 + step * 7)) % 4u) == 0u;
                g.setColour (beam (r + c + step).withAlpha (lit ? 0.1f + 0.35f * glow : 0.04f)); g.fillPath (q);
                g.setColour (Colours::white.withAlpha (0.05f)); g.strokePath (q, PathStrokeType (1.0f));
            }
        }
        // light beams and spots from the ball
        const int nb = 12;
        for (int k = 0; k < nb; ++k)
        {
            const float a = ballAngle * 0.7f + (float) k * MathConstants<float>::twoPi / (float) nb;
            const Point<float> end (G.ball.x + std::cos (a) * A.getWidth() * 0.7f, G.ball.y + (0.35f + 0.65f * std::abs (std::sin (a))) * A.getHeight() * 0.95f);
            const Point<float> dir = (end - G.ball) / jmax (1.0f, end.getDistanceFrom (G.ball)), nrm (-dir.y, dir.x);
            const float w = 6.0f + 20.0f * glow;
            Path ray; ray.startNewSubPath (G.ball); ray.lineTo (end + nrm * w); ray.lineTo (end - nrm * w); ray.closeSubPath();
            g.setGradientFill (ColourGradient (beam (k).withAlpha (0.28f * glow), G.ball.x, G.ball.y, beam (k).withAlpha (0.0f), end.x, end.y, false));
            g.fillPath (ray);
            const float sr = 10.0f + 16.0f * glow;
            g.setGradientFill (ColourGradient (beam (k).withAlpha (0.55f * glow), end.x, end.y, beam (k).withAlpha (0.0f), end.x + sr, end.y, true));
            g.fillEllipse (end.x - sr, end.y - sr * 0.5f, sr * 2, sr);
        }
        paintCrowd (g, G, crowd, full, on);
        // the DJ booth and the DROP lever
        paintBooth (g, G);
        // the ball on its string
        g.setColour (Colours::white.withAlpha (0.35f)); g.drawLine (G.ball.x, A.getY(), G.ball.x, G.ball.y - G.R, 1.5f);
        paintBall (g, G);
        // the strobe lamp
        {
            const auto L = G.lamp;
            g.setColour (Colour (0xff1b1726)); g.fillRoundedRectangle (L, 10);
            g.setColour (Colours::white.withAlpha (0.25f)); g.drawRoundedRectangle (L, 10, 1.4f);
            const auto lens = L.reduced (14, 12);
            g.setColour (Colours::white.withAlpha (0.1f + 0.9f * flash)); g.fillRoundedRectangle (lens, 6);
            for (int k = 0; k < 6; ++k) { g.setColour (Colours::black.withAlpha (0.25f)); g.fillRect (lens.getX() + 4, lens.getY() + 3 + (float) k * lens.getHeight() / 6.0f, lens.getWidth() - 8, 1.0f); }
            g.setColour (Colours::white.withAlpha (0.7f)); g.setFont (kk::modern::font (11.0f, true, 0.3f));
            g.drawText (lampDown ? "HOLDING" : "STROBE", L.translated (0, L.getHeight() + 4).withHeight (16), Justification::centred);
        }
        // the flash fills the room
        if (flash > 0.02f) { g.setColour (Colours::white.withAlpha (0.42f * flash)); g.fillRect (A); }
        // in words
        g.setColour (Colours::white.withAlpha (0.92f)); g.setFont (kk::modern::font (17.0f, true, 0.25f));
        const auto words = Rectangle<float> (G.ball.x + G.R + 60, A.getY() + 26, A.getRight() - 160 - (G.ball.x + G.R + 60), 24);
        g.drawFittedText (fullWord (full), words.toNearestInt(), Justification::centredLeft, 1, 0.8f);
        g.setColour (pink().withAlpha (0.95f)); g.setFont (kk::modern::font (13.0f, true, 0.1f));
        g.drawFittedText (stateWords (spin, crowd, st.lever.load()), words.translated (0, 26).withHeight (40).toNearestInt(), Justification::topLeft, 2, 0.8f);
        g.setColour (Colours::white.withAlpha (0.45f)); g.setFont (kk::modern::font (10.5f, true, 0.3f));
        g.drawText ("SPIN ME", Rectangle<float> (G.ball.x - 60, G.ball.y + G.R + 8, 120, 14), Justification::centred);
        g.drawText ("THE CROWD  -  DRAG UP / SIDEWAYS", Rectangle<float> (G.crowd.getX() + 10, G.crowd.getY() - 18, 420, 14), Justification::centredLeft);
        worldOff (g, A, on, pink());
        g.restoreState();
        g.setColour (pink().withAlpha (0.35f)); g.drawRoundedRectangle (A, 18, 1.2f);
        touchHints (g, worldHintRow (*this), { "CIRCLE THE BALL = sweep", "TAP / HOLD THE STROBE = gate", "CROWD UP = build-up", "CROWD SIDEWAYS = how full", "PULL THE LEVER = DROP" }, pink());
    }
    void resized() override { onBtn.setBounds (300, 56, 70, 32); calmBtn.setBounds (376, 56, 110, 32); }
    void mouseDown (const MouseEvent& e) override
    {
        const Geo G = geo(); const auto p = e.position;
        auto& st = proc.club;
        mode = cmNone;
        if (p.getDistanceFrom (G.ball) < G.R * 1.4f) { mode = cmBall; lastAng = std::atan2 (p.y - G.ball.y, p.x - G.ball.x); lastT = Time::getMillisecondCounterHiRes(); omegaS = 0; }
        else if (G.lamp.expanded (12).contains (p))
        {
            mode = cmStrobe; lampDown = true;
            const double now = Time::getMillisecondCounterHiRes();
            if (now - lastTap < 1500.0) st.strobeRate = jlimit (0.0f, 1.0f, (float) ((1000.0 / (now - lastTap) - 1.0) / 5.0));   // tap faster = a faster strobe
            lastTap = now;
            st.tapStrobe(); st.strobeHeld = true;
        }
        else if (G.slot.expanded (30, 24).contains (p)) mode = cmLever;
        else if (G.crowd.contains (p)) { mode = cmCrowd; axis = 0; startCrowd = st.crowd.load(); startFull = st.fullness.load(); }
        if (mode == cmNone) return;
        st.on = true; refresh();
        mouseDrag (e);
    }
    void mouseDrag (const MouseEvent& e) override
    {
        const Geo G = geo(); const auto p = e.position;
        auto& st = proc.club;
        if (mode == cmBall)
        {
            const float a = std::atan2 (p.y - G.ball.y, p.x - G.ball.x);
            float d = a - lastAng; while (d > MathConstants<float>::pi) d -= MathConstants<float>::twoPi; while (d < -MathConstants<float>::pi) d += MathConstants<float>::twoPi;
            const double now = Time::getMillisecondCounterHiRes();
            const float dt = (float) jmax (4.0, now - lastT) / 1000.0f;
            if (p.getDistanceFrom (G.ball) > 8.0f)
            {
                omegaS += (d / dt - omegaS) * 0.35f;
                st.spin = jlimit (0.0f, 1.0f, std::abs (omegaS) / 16.0f);
                if (std::abs (d) > 0.002f) spinDir = d > 0 ? 1.0f : -1.0f;
                ballAngle += d;
            }
            lastAng = a; lastT = now;
        }
        else if (mode == cmLever) st.lever = jlimit (0.0f, 1.0f, (p.y - G.slot.getY()) / G.slot.getHeight());
        else if (mode == cmCrowd)
        {
            const float dx = (float) e.getDistanceFromDragStartX(), dy = (float) e.getDistanceFromDragStartY();
            if (axis == 0 && jmax (std::abs (dx), std::abs (dy)) > 6.0f) axis = std::abs (dy) > std::abs (dx) ? 2 : 1;
            if (axis == 2) st.crowd = jlimit (0.0f, 1.0f, startCrowd - dy / (G.crowd.getHeight() * 0.9f));
            if (axis == 1) st.fullness = jlimit (0.0f, 1.0f, startFull + dx / (G.crowd.getWidth() * 0.7f));
        }
        repaint();
    }
    void mouseUp (const MouseEvent&) override
    {
        if (mode == cmStrobe) { proc.club.strobeHeld = false; lampDown = false; }
        if (mode == cmLever) proc.club.lever = 0.0f;   // let go: it slams back
        mode = cmNone; repaint();
    }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        const Geo G = geo();
        if (e.position.getDistanceFrom (G.ball) < G.R * 1.4f) proc.club.spin = 0.0f;
        else if (G.crowd.contains (e.position)) proc.club.crowd = 0.0f;
        repaint();
    }
private:
    enum Mode { cmNone, cmBall, cmStrobe, cmLever, cmCrowd };
    struct Geo { Point<float> ball; float R = 60; Rectangle<float> lamp, slot, crowd; float floorY = 0; };
    Rectangle<int> area() const { return worldArea (*this); }
    Geo geo() const
    {
        const auto A = area().toFloat(); Geo G;
        G.R = jlimit (36.0f, 66.0f, A.getHeight() * 0.12f);
        G.ball = { A.getCentreX() - 60.0f, A.getY() + G.R + 36.0f };
        G.lamp = Rectangle<float> (130, 80).withPosition (A.getX() + 40, A.getY() + 36);
        G.slot = Rectangle<float> (40, A.getHeight() * 0.42f).withPosition (A.getRight() - 96, A.getY() + 70);
        G.floorY = A.getY() + A.getHeight() * 0.46f;
        G.crowd = Rectangle<float> (A.getX() + 16, A.getY() + A.getHeight() * 0.56f, A.getWidth() - 220, A.getHeight() * 0.44f - 8);
        return G;
    }
    void paintBall (Graphics& g, const Geo& G)
    {
        const float R = G.R; const auto c = G.ball;
        g.setGradientFill (ColourGradient (Colour (0xffd8dbe6), c.x - R * 0.4f, c.y - R * 0.45f, Colour (0xff2a2d3a), c.x + R, c.y + R, true));
        g.fillEllipse (c.x - R, c.y - R, R * 2, R * 2);
        const int lat = 9;
        for (int i = 0; i < lat; ++i)
        {
            const float ph0 = -1.4f + 2.8f * (float) i / lat, ph1 = -1.4f + 2.8f * (float) (i + 1) / lat, phm = 0.5f * (ph0 + ph1);
            const float rr = R * std::cos (phm), y0 = c.y + R * std::sin (ph0), y1 = c.y + R * std::sin (ph1);
            const int cnt = jmax (4, (int) std::round (22.0f * std::cos (phm)));
            for (int k = 0; k < cnt; ++k)
            {
                const float th = (float) k * MathConstants<float>::twoPi / (float) cnt + ballAngle;
                const float cz = std::cos (th);
                if (cz <= 0.05f) continue;
                const float x = c.x + rr * std::sin (th), w = rr * MathConstants<float>::twoPi / (float) cnt * cz * 0.85f;
                const bool sparkle = (kk::hash32 ((uint32) (i * 97 + k * 13 + (int) (phase * 8.0f))) % 23u) == 0u;
                const float b = 0.25f + 0.6f * cz * (0.6f + 0.4f * std::sin (phm + 0.6f));
                g.setColour (sparkle ? Colours::white : Colour::fromFloatRGBA (b, b, b * 1.08f, 0.9f));
                g.fillRect (x - w * 0.5f, y0 + 0.7f, jmax (1.0f, w), jmax (1.0f, y1 - y0 - 1.4f));
                if (sparkle) { const float s = 6.0f + 6.0f * proc.club.spin.load(); g.setColour (Colours::white.withAlpha (0.8f)); g.drawLine (x - s, (y0 + y1) * 0.5f, x + s, (y0 + y1) * 0.5f, 1.0f); g.drawLine (x, y0 - s * 0.6f, x, y1 + s * 0.6f, 1.0f); }
            }
        }
        g.setColour (Colours::white.withAlpha (0.55f)); g.drawEllipse (c.x - R, c.y - R, R * 2, R * 2, 1.2f);
        if (mode == cmBall) { g.setColour (pink().withAlpha (0.6f)); g.drawEllipse (c.x - R - 10, c.y - R - 10, R * 2 + 20, R * 2 + 20, 1.5f); }
    }
    void paintCrowd (Graphics& g, const Geo& G, float crowd, float full, bool on)
    {
        const auto C = G.crowd;
        const int maxP = 54, n = 5 + (int) std::round (full * (float) (maxP - 5));
        const float energy = on ? jlimit (0.0f, 1.0f, 0.3f + level * 1.5f) : 0.2f;
        for (int row = 0; row < 3; ++row)
        {
            Random rr (4242 + row);
            for (int i = 0; i < maxP / 3; ++i)
            {
                const int idx = i * 3 + row;
                const float u = rr.nextFloat(), jit = rr.nextFloat(), hue = rr.nextFloat();
                if (idx >= n) continue;
                const float s = (0.62f + 0.22f * (float) row) * jlimit (0.7f, 1.3f, C.getHeight() / 220.0f);
                const float x = C.getX() + 20 + u * (C.getWidth() - 40);
                const float bounce = std::sin (phase * (5.0f + 4.0f * crowd) + (float) idx * 1.7f) * (1.0f + 7.0f * crowd * energy) * s;
                const float y = C.getY() + C.getHeight() * (0.22f + 0.3f * (float) row) - std::abs (bounce);
                const float headR = 11.0f * s, shoulder = y + headR + 6.0f * s;
                const auto rim = beam ((int) (hue * 5.0f)).withAlpha (0.55f + 0.3f * glowNow());
                // arms: down at rest, up with the crowd (each person a little different)
                const float up = jlimit (0.0f, 1.0f, crowd * 1.3f - 0.25f * jit + 0.1f * std::sin (phase * 3.0f + (float) idx));
                for (int side = -1; side <= 1; side += 2)
                {
                    const Point<float> sh (x + (float) side * 13.0f * s, shoulder + 2.0f * s);
                    const Point<float> el = sh + Point<float> ((float) side * (10.0f + 4.0f * (1.0f - up)) * s, (16.0f - 32.0f * up) * s);
                    const Point<float> hand = el + Point<float> ((float) side * (3.0f - 5.0f * up) * s, (14.0f - 32.0f * up) * s);
                    g.setColour (Colour (0xff0b0714)); g.drawLine (Line<float> (sh, el), 6.0f * s); g.drawLine (Line<float> (el, hand), 5.0f * s);
                    g.setColour (rim.withAlpha (0.45f)); g.drawLine (Line<float> (sh, el), 1.2f); g.drawLine (Line<float> (el, hand), 1.2f); g.fillEllipse (hand.x - 2.5f * s, hand.y - 2.5f * s, 5.0f * s, 5.0f * s);
                }
                Path body; body.addRoundedRectangle (x - 17.0f * s, shoulder, 34.0f * s, 80.0f * s, 12.0f * s);
                g.setColour (Colour (0xff0b0714)); g.fillPath (body);
                g.setColour (rim.withAlpha (0.4f)); g.strokePath (body, PathStrokeType (1.2f));
                g.setColour (Colour (0xff0b0714)); g.fillEllipse (x - headR, y, headR * 2, headR * 2);
                g.setColour (rim); g.drawEllipse (x - headR, y, headR * 2, headR * 2, 1.3f);
            }
        }
    }
    void paintBooth (Graphics& g, const Geo& G)
    {
        const auto A = area().toFloat(); const auto S = G.slot;
        // the lever
        g.setColour (Colours::white.withAlpha (0.8f)); g.setFont (kk::modern::font (13.0f, true, 0.35f));
        g.drawText ("DROP", S.withHeight (18).translated (-30, -26).withWidth (S.getWidth() + 60), Justification::centred);
        g.setColour (Colour (0xff09070f)); g.fillRoundedRectangle (S, S.getWidth() * 0.5f);
        g.setColour (pink().withAlpha (0.3f + 0.5f * leverShown)); g.drawRoundedRectangle (S, S.getWidth() * 0.5f, 1.5f);
        const float hy = S.getY() + 16 + (S.getHeight() - 32) * leverShown;
        g.setColour (Colour (0xff8d93a6)); g.fillRect (S.getCentreX() - 3, S.getY() + 10, 6.0f, hy - S.getY() - 10);
        const float kr = 19.0f;
        g.setGradientFill (ColourGradient (Colour (0xffff7a8a), S.getCentreX() - 6, hy - 8, Colour (0xffb0102a), S.getCentreX() + kr, hy + kr, true));
        g.fillEllipse (S.getCentreX() - kr, hy - kr, kr * 2, kr * 2);
        if (leverShown > 0.05f) { g.setGradientFill (ColourGradient (Colour (0xffff3b5c).withAlpha (0.5f * leverShown), S.getCentreX(), hy, Colour (0xffff3b5c).withAlpha (0.0f), S.getCentreX() + 60, hy, true)); g.fillEllipse (S.getCentreX() - 60, hy - 60, 120, 120); }
        // the decks
        const auto D = Rectangle<float> (190, 70).withPosition (A.getRight() - 200, A.getBottom() - 92);
        g.setColour (Colour (0xff17121f)); g.fillRoundedRectangle (D, 8);
        g.setColour (violet().withAlpha (0.5f)); g.drawRoundedRectangle (D, 8, 1.2f);
        for (int k = 0; k < 2; ++k)
        {
            const Point<float> c (D.getX() + 48 + (float) k * 94, D.getCentreY());
            g.setColour (Colour (0xff050407)); g.fillEllipse (c.x - 28, c.y - 28, 56, 56);
            const float a = phase * (2.0f + 4.0f * proc.club.spin.load()) * (k == 0 ? 1.0f : -1.0f);
            g.setColour (pink().withAlpha (0.7f)); g.drawLine (c.x, c.y, c.x + std::cos (a) * 24, c.y + std::sin (a) * 24, 2.0f);
            g.setColour (Colours::white.withAlpha (0.15f)); g.drawEllipse (c.x - 18, c.y - 18, 36, 36, 1.0f);
        }
    }
    float glowNow() const { return jlimit (0.0f, 1.0f, proc.club.spin.load()); }
    static String fullWord (float f) { return f < 0.15f ? "AN EMPTY CLUB  -  hollow, long echoes" : f < 0.4f ? "A FEW PEOPLE  -  the room still rings" : f < 0.65f ? "A GOOD NIGHT  -  warm room" : f < 0.85f ? "BUSY  -  warm and tight" : "PACKED  -  warm, tight, no echo"; }
    static String stateWords (float spin, float crowd, float lever)
    {
        String s = spin < 0.03f ? "the ball stands still" : spin < 0.3f ? "the ball turns slowly" : spin < 0.7f ? "the ball spins" : "the ball flies";
        s << (crowd < 0.1f ? "  .  the crowd chills" : crowd < 0.5f ? "  .  the crowd warms up" : crowd < 0.85f ? "  .  hands up - it builds" : "  .  everyone screams - the build-up peaks");
        if (lever > 0.05f) s << "  .  DROP!";
        return s;
    }
    void refresh() { onBtn.selected = proc.club.on.load(); onBtn.setButtonText (proc.club.on.load() ? "ON" : "OFF"); onBtn.repaint(); repaint(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        auto& st = proc.club;
        phase += 1.0f / 30.0f;
        if (mode != cmBall) ballAngle += spinDir * (0.004f + 0.22f * st.spin.load());
        const float lv = st.lever.load();
        leverShown = lv > leverShown ? lv : leverShown + (lv - leverShown) * 0.55f;
        flash = jmax (st.on.load() ? st.mStrobe.load() : 0.0f, flash * 0.5f);
        level += (st.mLevel.load() - level) * 0.3f;
        if (onBtn.selected != st.on.load()) refresh();
        repaint (area());
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf }, calmBtn { lnf };
    Mode mode = cmNone; int axis = 0;
    float lastAng = 0, omegaS = 0, spinDir = 1, ballAngle = 0, phase = 0, leverShown = 0, flash = 0, level = 0, startCrowd = 0, startFull = 0;
    double lastT = 0, lastTap = -1.0e9; bool lampDown = false;
};

// ---------------------------------------------------------------- SEASONING ----------------------------------------------------------------
class SeasoningPage : public Component, public SettableTooltipClient, private Timer
{
public:
    SeasoningPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        onBtn.setButtonText ("OFF"); onBtn.framed = true; addAndMakeVisible (onBtn);
        onBtn.setTooltip ("SEASONING on / off - the dish with or without what you put on it");
        onBtn.onClick = [this] { proc.season.on = ! proc.season.on.load(); refresh(); };
        cleanBtn.setButtonText ("CLEAN PLATE"); cleanBtn.framed = true; addAndMakeVisible (cleanBtn);
        cleanBtn.setTooltip ("Wipe the plate - the track comes back plain");
        cleanBtn.onClick = [this] { proc.season.clean(); grains.clear(); refresh(); };
        setTooltip ("Grab a shaker and shake it over the plate - the harder you shake, the more goes on.  A click = a pinch.  Double-click a shaker = brush that one off.");
        startTimerHz (30);
        refresh();
    }
    static Colour orange() { return Colour (0xffff9a3d); }
    static Colour spiceColour (int s) { static const uint32 c[] { 0xfff4f7fb, 0xff7b7f8c, 0xffff3b30, 0xffff9ad5, 0xff7fe7ff, 0xffa29bb8 }; return Colour (c[(size_t) jlimit (0, 5, s)]); }
    void paint (Graphics& g) override
    {
        proHeader (g, *this, "SEASONING", "WORLDS  -  THE COOK'S TWIN", "season a finished track like a dish: shake salt, pepper, chilli, sugar, ice or smoke over the plate.  more shaking = more taste", orange());
        const auto A = area().toFloat();
        const auto& st = proc.season;
        const bool on = st.on.load();
        worldFrame (g, A, Colour (0xff20140c), Colour (0xff0a0705));
        g.saveState(); g.reduceClipRegion (A.toNearestInt());
        // the table: dark wood grain
        for (int k = 0; k < 18; ++k)
        {
            const float y = A.getY() + A.getHeight() * (0.3f + 0.04f * (float) k);
            Path p; p.startNewSubPath (A.getX(), y);
            for (int s = 1; s <= 24; ++s) { const float x = A.getX() + A.getWidth() * (float) s / 24.0f; p.lineTo (x, y + 3.0f * std::sin ((float) s * 0.9f + (float) k * 1.3f)); }
            g.setColour (Colour (0xffc58a52).withAlpha (0.05f)); g.strokePath (p, PathStrokeType (1.2f));
        }
        paintPlate (g, on);
        // grains in the air
        for (auto& gr : grains) paintGrain (g, gr.s, gr.p, 1.0f, gr.seed);
        for (int s = 0; s < kk::pro::numSpices; ++s)
        {
            const auto home = shakerHome (s);
            if (s == held) { g.setColour (spiceColour (s).withAlpha (0.25f)); Path dash, outline; outline.addRoundedRectangle (home, 12.0f); const float d[] { 5.0f, 5.0f }; PathStrokeType (1.0f).createDashedStroke (dash, outline, d, 2); g.fillPath (dash); }
            else paintShaker (g, home, s, 0.0f);
            g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (kk::modern::font (13.0f, true, 0.3f));
            g.drawText (kk::pro::spiceName (s), home.withY (home.getBottom() + 6).withHeight (16).expanded (20, 0), Justification::centred);
            g.setColour (spiceColour (s).withAlpha (0.75f)); g.setFont (kk::modern::font (10.5f, true, 0.04f));
            g.drawFittedText (kk::pro::spiceDoes (s), home.withY (home.getBottom() + 22).withHeight (28).expanded (26, 0).toNearestInt(), Justification::centredTop, 2, 0.8f);
        }
        if (held >= 0) paintShaker (g, Rectangle<float> (shakerW, shakerH).withCentre (heldPos), held, tilt);
        // what it tastes like
        g.setColour (Colours::white.withAlpha (0.9f)); g.setFont (kk::modern::font (16.0f, true, 0.12f));
        g.drawText (tasteWords(), Rectangle<float> (A.getX(), A.getBottom() - 40, A.getWidth(), 24), Justification::centred);
        if (note.isNotEmpty()) { g.setColour (orange()); g.setFont (kk::modern::font (13.0f, true, 0.1f)); g.drawText (note, Rectangle<float> (A.getX(), A.getBottom() - 64, A.getWidth(), 20), Justification::centred); }
        worldOff (g, A, on, orange());
        g.restoreState();
        g.setColour (orange().withAlpha (0.35f)); g.drawRoundedRectangle (A, 18, 1.2f);
        touchHints (g, worldHintRow (*this), { "GRAB + SHAKE = season", "OVER THE PLATE = it lands", "CLICK = a pinch", "DOUBLE-CLICK = brush it off" }, orange());
    }
    void resized() override { onBtn.setBounds (300, 56, 70, 32); cleanBtn.setBounds (376, 56, 130, 32); }
    void mouseDown (const MouseEvent& e) override
    {
        held = -1;
        for (int s = 0; s < kk::pro::numSpices; ++s) if (shakerHome (s).expanded (6).contains (e.position)) held = s;
        if (held < 0) return;
        heldPos = e.position; grabOff = shakerHome (held).getCentre() - e.position; heldPos = e.position + grabOff;
        lastP = e.position; dir[0] = dir[1] = 0; travel[0] = travel[1] = 0; downT = Time::getMillisecondCounterHiRes(); shook = false; tilt = 0;
        repaint();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (held < 0) return;
        heldPos = e.position + grabOff;
        const auto d = e.position - lastP; lastP = e.position;
        tilt = jlimit (-0.9f, 0.9f, tilt * 0.6f + d.x * 0.03f + (overPlate() ? 0.25f : 0.0f));
        for (int ax = 0; ax < 2; ++ax)
        {
            const float v = ax == 0 ? d.x : d.y;
            if (std::abs (v) < 1.0f) continue;
            const int dr = v > 0 ? 1 : -1;
            travel[ax] += std::abs (v);
            if (dir[ax] != 0 && dr != dir[ax]) { if (travel[ax] > 10.0f) shake (jlimit (0.025f, 0.07f, travel[ax] / 1200.0f + 0.02f)); travel[ax] = 0; }
            dir[ax] = dr;
        }
        repaint();
    }
    void mouseUp (const MouseEvent& e) override
    {
        if (held >= 0 && ! shook && e.getDistanceFromDragStart() < 5 && Time::getMillisecondCounterHiRes() - downT < 400.0) { heldPos = shakerHome (held).getCentre() + Point<float> (0, 40); pinch(); }
        held = -1; repaint();
    }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        for (int s = 0; s < kk::pro::numSpices; ++s) if (shakerHome (s).expanded (6).contains (e.position)) { proc.season.dose[(size_t) s] = 0.0f; note = String ("brushed off the ") + String (kk::pro::spiceName (s)).toLowerCase(); }
        repaint();
    }
private:
    struct Grain { Point<float> p, v; int s = 0; float landY = 0; uint32 seed = 0; bool lands = true; };
    static constexpr float shakerW = 92.0f, shakerH = 150.0f;
    Rectangle<int> area() const { return worldArea (*this); }
    Rectangle<float> shakerHome (int s) const
    {
        const auto A = area().toFloat();
        const float gap = (A.getWidth() - 6.0f * shakerW) / 7.0f;
        return { A.getX() + gap + (float) s * (shakerW + gap), A.getY() + 20.0f, shakerW, shakerH };
    }
    Point<float> plateC() const { const auto A = area().toFloat(); return { A.getCentreX(), A.getY() + A.getHeight() * 0.67f }; }
    float plateRx() const { return jmin (area().getWidth() * 0.32f, 420.0f); }
    float plateRy() const { return jmin (plateRx() * 0.38f, area().getHeight() * 0.25f); }
    Point<float> capPos() const { return heldPos + Point<float> (std::sin (tilt) * shakerH * 0.5f, -std::cos (tilt) * shakerH * 0.5f + (tilt != 0 ? shakerH * 0.25f : 0.0f)); }
    bool overPlate() const { return std::abs (heldPos.x - plateC().x) < plateRx() * 0.9f; }
    void shake (float amount)
    {
        shook = true;
        const bool lands = overPlate();
        if (lands) { proc.season.add (held, amount); proc.season.on = true; note = {}; refresh(); }
        else note = "shake it over the plate";
        spawn (held, 7, lands);
    }
    void pinch() { proc.season.add (held, 0.03f); proc.season.on = true; spawn (held, 4, true); note = String ("a pinch of ") + String (kk::pro::spiceName (held)).toLowerCase(); refresh(); }
    void spawn (int s, int count, bool lands)
    {
        const auto c = heldPos.translated (0, shakerH * 0.4f), pc = plateC();
        for (int k = 0; k < count && grains.size() < 400; ++k)
        {
            Grain gr; gr.s = s; gr.p = c + Point<float> (rnd.nextFloat() * 20.0f - 10.0f, rnd.nextFloat() * 8.0f);
            gr.v = { rnd.nextFloat() * 3.0f - 1.5f, 1.0f + rnd.nextFloat() * 2.0f };
            gr.landY = lands ? pc.y + (rnd.nextFloat() - 0.6f) * plateRy() * 0.9f : (float) area().getBottom() + 10.0f;
            gr.seed = (uint32) rnd.nextInt(); gr.lands = lands;
            grains.push_back (gr);
        }
    }
    void paintShaker (Graphics& g, Rectangle<float> r, int s, float tlt)
    {
        g.saveState();
        g.addTransform (AffineTransform::rotation (tlt, r.getCentreX(), r.getCentreY()));
        const auto body = r.withTrimmedTop (r.getHeight() * 0.26f), cap = r.withHeight (r.getHeight() * 0.3f).reduced (6, 0);
        const auto col = spiceColour (s);
        const float lvl = 1.0f - 0.7f * proc.season.dose[(size_t) s].load();
        g.setColour (Colours::white.withAlpha (0.08f)); g.fillRoundedRectangle (body, 14);
        const auto content = body.reduced (5).withTrimmedTop ((body.getHeight() - 10) * (1.0f - lvl));
        g.setGradientFill (ColourGradient (col.withAlpha (0.85f), content.getX(), content.getY(), col.darker (0.6f).withAlpha (0.9f), content.getRight(), content.getBottom(), false));
        g.fillRoundedRectangle (content, 10);
        Random rr (77 + s);
        for (int k = 0; k < 26; ++k) { const float x = content.getX() + rr.nextFloat() * content.getWidth(), y = content.getY() + rr.nextFloat() * content.getHeight(); g.setColour ((s == 1 ? Colours::black : Colours::white).withAlpha (0.25f)); g.fillRect (x, y, 2.0f, 2.0f); }
        g.setColour (Colours::white.withAlpha (0.45f)); g.drawRoundedRectangle (body, 14, 1.3f);
        g.setColour (Colours::white.withAlpha (0.25f)); g.fillRoundedRectangle (body.getX() + 8, body.getY() + 10, 6, body.getHeight() - 26, 3);
        g.setGradientFill (ColourGradient (Colour (0xffe6e9f0), cap.getX(), cap.getY(), Colour (0xff6c7080), cap.getRight(), cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 8);
        for (int k = 0; k < 5; ++k) { g.setColour (Colours::black.withAlpha (0.55f)); g.fillEllipse (cap.getX() + cap.getWidth() * (0.18f + 0.16f * (float) k) - 2.5f, cap.getY() + 6, 5, 5); }
        g.restoreState();
    }
    void paintGrain (Graphics& g, int s, Point<float> p, float alpha, uint32 seed)
    {
        const auto col = spiceColour (s).withAlpha (alpha);
        const float r1 = (float) (seed % 100u) / 100.0f, rot = r1 * 6.28f;
        switch (s)
        {
            case kk::pro::spSalt:   g.setColour (col); g.fillRect (p.x - 1.6f, p.y - 1.6f, 3.2f, 3.2f); break;
            case kk::pro::spPepper: g.setColour (Colour (0xff1c1d22).withAlpha (alpha)); g.fillEllipse (p.x - 2.0f, p.y - 2.0f, 4.0f, 4.0f); break;
            case kk::pro::spChilli: { Path f; f.addRectangle (-4.0f, -1.5f, 8.0f, 3.0f); g.setColour (col); g.fillPath (f, AffineTransform::rotation (rot).translated (p)); break; }
            case kk::pro::spSugar:  { const float tw = 0.5f + 0.5f * std::sin (phase * 6.0f + rot * 3.0f); g.setColour (col.interpolatedWith (Colours::white, tw)); g.fillEllipse (p.x - 1.8f, p.y - 1.8f, 3.6f, 3.6f);
                                      if (tw > 0.85f) { g.setColour (Colours::white.withAlpha (alpha * 0.8f)); g.drawLine (p.x - 4, p.y, p.x + 4, p.y, 0.8f); g.drawLine (p.x, p.y - 4, p.x, p.y + 4, 0.8f); } break; }
            case kk::pro::spIce:    g.setColour (col.withAlpha (0.55f * alpha)); g.fillRoundedRectangle (p.x - 5, p.y - 4, 10, 8, 2); g.setColour (Colours::white.withAlpha (0.8f * alpha)); g.drawRoundedRectangle (p.x - 5, p.y - 4, 10, 8, 2, 1.0f); break;
            default:                g.setColour (col.withAlpha (0.5f * alpha)); g.fillEllipse (p.x - 3, p.y - 3, 6, 6); break;
        }
    }
    void paintPlate (Graphics& g, bool on)
    {
        const auto c = plateC(); const float rx = plateRx(), ry = plateRy();
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillEllipse (c.x - rx * 1.06f, c.y - ry * 0.9f + 14, rx * 2.12f, ry * 2.1f);
        g.setGradientFill (ColourGradient (Colour (0xfff7f4ee), c.x - rx * 0.3f, c.y - ry, Colour (0xffb9b4ab), c.x + rx, c.y + ry, false));
        g.fillEllipse (c.x - rx, c.y - ry, rx * 2, ry * 2);
        g.setColour (Colour (0xffd8d3c9)); g.drawEllipse (c.x - rx * 0.78f, c.y - ry * 0.78f, rx * 1.56f, ry * 1.56f, 2.0f);
        // the dish: the track itself, a glossy sauce that ripples with the music
        const float drx = rx * 0.7f, dry = ry * 0.7f;
        g.setGradientFill (ColourGradient (Colour (0xff6b1d1d), c.x - drx * 0.3f, c.y - dry * 0.4f, Colour (0xff2a0b0b), c.x + drx, c.y + dry, true));
        g.fillEllipse (c.x - drx, c.y - dry, drx * 2, dry * 2);
        Path ring;
        for (int k = 0; k <= 96; ++k)
        {
            const float a = (float) k / 96.0f * MathConstants<float>::twoPi;
            const float w = 1.0f + (on ? 0.05f + 0.25f * level : 0.02f) * std::sin (a * 9.0f + phase * 4.0f) * std::sin (a * 3.0f - phase * 1.7f);
            const Point<float> q (c.x + std::cos (a) * drx * 0.62f * w, c.y + std::sin (a) * dry * 0.62f * w);
            if (k == 0) ring.startNewSubPath (q); else ring.lineTo (q);
        }
        g.setColour (Colour (0xffff9a3d).withAlpha (0.5f)); g.strokePath (ring, PathStrokeType (2.0f));
        g.setColour (Colours::white.withAlpha (0.25f)); g.fillEllipse (c.x - drx * 0.5f, c.y - dry * 0.75f, drx * 0.5f, dry * 0.3f);
        // what is on it (stable positions: more seasoning = more grains, never a reshuffle)
        for (int s = 0; s < kk::pro::numSpices; ++s)
        {
            const float d = proc.season.dose[(size_t) s].load();
            if (s == kk::pro::spSmoke)
            {
                const int n = (int) std::round (d * 9.0f);
                for (int k = 0; k < n; ++k)
                {
                    const float u = std::fmod (phase * 0.25f + (float) k / 9.0f, 1.0f), x0 = c.x + ((float) k - 4.0f) * rx * 0.15f;
                    Path w; w.startNewSubPath (x0, c.y - ry * 0.2f);
                    for (int j = 1; j <= 12; ++j) { const float t = (float) j / 12.0f; w.lineTo (x0 + 12.0f * std::sin (t * 6.0f + phase * 2.0f + (float) k), c.y - ry * 0.2f - t * (60.0f + 80.0f * u)); }
                    g.setColour (spiceColour (s).withAlpha (0.35f * (1.0f - u))); g.strokePath (w, PathStrokeType (5.0f + 6.0f * u, PathStrokeType::curved, PathStrokeType::rounded));
                }
                continue;
            }
            const int n = (int) std::round (d * (s == kk::pro::spIce ? 26.0f : 150.0f));
            Random rr (7919 * (s + 1) + 3);
            for (int k = 0; k < n; ++k)
            {
                const float u = std::sqrt (rr.nextFloat()) * 0.8f, a = rr.nextFloat() * MathConstants<float>::twoPi;
                paintGrain (g, s, { c.x + std::cos (a) * rx * u, c.y + std::sin (a) * ry * u }, 1.0f, (uint32) rr.nextInt());
            }
        }
    }
    String tasteWords() const
    {
        static const char* w[][3] { { "a pinch of salt", "salty", "very salty" }, { "a little pepper", "peppery", "full of pepper" }, { "a hint of chilli", "hot", "on fire" },
                                    { "a touch of sugar", "sweet", "candy" }, { "a cube of ice", "icy", "frozen" }, { "a wisp of smoke", "smoky", "deep in smoke" } };
        StringArray parts;
        for (int s = 0; s < kk::pro::numSpices; ++s) { const float d = proc.season.dose[(size_t) s].load(); if (d > 0.01f) parts.add (w[s][d < 0.25f ? 0 : d < 0.65f ? 1 : 2]); }
        return parts.isEmpty() ? String ("plain  -  nothing on it yet") : parts.joinIntoString ("  .  ");
    }
    void refresh() { onBtn.selected = proc.season.on.load(); onBtn.setButtonText (proc.season.on.load() ? "ON" : "OFF"); onBtn.repaint(); repaint(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 1.0f / 30.0f;
        level += (proc.season.mLevel.load() - level) * 0.3f;
        for (auto& gr : grains) { gr.v.y += 0.7f; gr.p += gr.v; }
        grains.erase (std::remove_if (grains.begin(), grains.end(), [] (const Grain& gr) { return gr.p.y >= gr.landY; }), grains.end());
        if (onBtn.selected != proc.season.on.load()) refresh();
        repaint (area());
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf }, cleanBtn { lnf };
    std::vector<Grain> grains; Random rnd { 99 };
    int held = -1, dir[2] {}; float travel[2] {}, tilt = 0, phase = 0, level = 0; bool shook = false; double downT = 0;
    Point<float> heldPos, grabOff, lastP; String note;
};

// ---------------------------------------------------------------- ENGINE ----------------------------------------------------------------
class EnginePage : public Component, public SettableTooltipClient, private Timer
{
public:
    EnginePage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        onBtn.setButtonText ("OFF"); onBtn.framed = true; addAndMakeVisible (onBtn);
        onBtn.setTooltip ("ENGINE on / off - the track runs through the engine");
        onBtn.onClick = [this] { proc.engine.on = ! proc.engine.on.load(); refresh(); };
        idleBtn.setButtonText ("IDLE"); idleBtn.framed = true; addAndMakeVisible (idleBtn);
        idleBtn.setTooltip ("Foot off the pedal - the needle falls back to idle");
        idleBtn.onClick = [this] { proc.engine.rev = 0.0f; refresh(); };
        setTooltip ("Drag the needle (or press the pedal down) = REV, the drive.  Click a gear = the character.  Click an exhaust = the tone.  Hold TURBO = a boost with a gentle whistle.");
        rpmShown = proc.engine.on.load() ? proc.engine.rev.load() : 0.0f;
        startTimerHz (30);
        refresh();
    }
    static Colour red()  { return Colour (0xffff4d3d); }
    static Colour amber() { return Colour (0xffffb23d); }
    void paint (Graphics& g) override
    {
        proHeader (g, *this, "ENGINE", "WORLDS  -  THE GARAGE'S TWIN", "the track runs through an engine: rev it for drive, shift the gears for character, pick the exhaust for tone, hold TURBO", red());
        const auto A = area().toFloat();
        const auto& st = proc.engine;
        const bool on = st.on.load();
        const Geo G = geo();
        worldFrame (g, A, Colour (0xff17181d), Colour (0xff07070a));
        g.saveState(); g.reduceClipRegion (A.toNearestInt());
        // floor lines of the garage
        for (int k = 0; k < 9; ++k) { const float y = A.getY() + A.getHeight() * (0.62f + 0.05f * (float) k); g.setColour (Colours::white.withAlpha (0.025f + 0.01f * (float) k)); g.fillRect (A.getX(), y, A.getWidth(), 1.0f); }
        paintPipe (g, G);
        paintEngine (g, G);
        paintGauge (g, G);
        paintPedal (g, G);
        paintGate (g, G);
        paintTips (g, G);
        paintTurbo (g, G);
        worldOff (g, A, on, red());
        g.restoreState();
        g.setColour (red().withAlpha (0.35f)); g.drawRoundedRectangle (A, 18, 1.2f);
        touchHints (g, worldHintRow (*this), { "DRAG THE NEEDLE = rev (drive)", "PEDAL DOWN = rev", "CLICK A GEAR = character", "CLICK AN EXHAUST = tone", "HOLD TURBO = boost" }, red());
    }
    void resized() override { onBtn.setBounds (300, 56, 70, 32); idleBtn.setBounds (376, 56, 80, 32); knob = gearPos (proc.engine.gear.load()); }
    void mouseDown (const MouseEvent& e) override
    {
        const Geo G = geo(); const auto p = e.position;
        auto& st = proc.engine;
        mode = emNone;
        if (p.getDistanceFrom (G.gauge) < G.gR * 1.05f) mode = emNeedle;
        else if (G.pedal.expanded (10).contains (p)) { mode = emPedal; startRev = st.rev.load(); }
        else if (p.getDistanceFrom (G.turbo) < G.tR) { mode = emTurbo; st.turbo = true; }
        else if (G.gate.expanded (30).contains (p)) { int best = -1; float bd = 46.0f; for (int k = 0; k < kk::pro::numGears; ++k) { const float d = p.getDistanceFrom (gearPos (k)); if (d < bd) { bd = d; best = k; } } if (best >= 0) { st.gear = best; mode = emGear; } }
        else for (int k = 0; k < kk::pro::numExhausts; ++k) if (G.tips[(size_t) k].contains (p)) { st.exhaust = k; mode = emTip; }
        if (mode == emNone) return;
        st.on = true; refresh();
        mouseDrag (e);
    }
    void mouseDrag (const MouseEvent& e) override
    {
        const Geo G = geo(); const auto p = e.position;
        auto& st = proc.engine;
        if (mode == emNeedle)
        {
            float a = std::atan2 (p.y - G.gauge.y, p.x - G.gauge.x);
            while (a < a0) a += MathConstants<float>::twoPi;
            float u = (a - a0) / sweep;
            if (u > 1.0f) u = u > 1.0f + (MathConstants<float>::twoPi / sweep - 1.0f) * 0.5f ? 0.0f : 1.0f;   // the dead zone at the bottom: snap to the nearer end
            st.rev = jlimit (0.0f, 1.0f, u);
        }
        else if (mode == emPedal) st.rev = jlimit (0.0f, 1.0f, startRev + (float) e.getDistanceFromDragStartY() / (G.pedal.getHeight() * 1.3f));
        repaint();
    }
    void mouseUp (const MouseEvent&) override { if (mode == emTurbo) proc.engine.turbo = false; mode = emNone; repaint(); }
private:
    enum Mode { emNone, emNeedle, emPedal, emTurbo, emGear, emTip };
    struct Geo { Point<float> gauge, turbo; float gR = 150, tR = 60; Rectangle<float> engine, gate, pedal; std::array<Rectangle<float>, 4> tips; };
    static constexpr float a0 = 0.75f * 3.14159265f, sweep = 1.5f * 3.14159265f;
    Rectangle<int> area() const { return worldArea (*this); }
    Geo geo() const
    {
        const auto A = area().toFloat(); Geo G;
        G.gR = jmin (A.getHeight() * 0.33f, A.getWidth() * 0.13f);
        G.gauge = { A.getX() + G.gR + 40.0f, A.getY() + G.gR + 30.0f };
        G.pedal = Rectangle<float> (84.0f, jmin (110.0f, A.getBottom() - (G.gauge.y + G.gR) - 24.0f)).withPosition (G.gauge.x - 42.0f, G.gauge.y + G.gR + 14.0f);
        G.engine = Rectangle<float> (A.getWidth() * 0.3f, A.getHeight() * 0.6f).withCentre ({ A.getX() + A.getWidth() * 0.47f, A.getY() + A.getHeight() * 0.36f });
        G.gate = Rectangle<float> (430.0f, jmin (150.0f, A.getHeight() * 0.3f)).withCentre ({ A.getX() + A.getWidth() * 0.47f, A.getBottom() - jmin (150.0f, A.getHeight() * 0.3f) * 0.5f - 22.0f });
        const float x0 = A.getX() + A.getWidth() * 0.69f, w = A.getRight() - 24.0f - x0, cw = (w - 14.0f) * 0.5f, ch = jmin (110.0f, A.getHeight() * 0.22f);
        for (int k = 0; k < 4; ++k) G.tips[(size_t) k] = { x0 + (float) (k % 2) * (cw + 14.0f), A.getY() + 30.0f + (float) (k / 2) * (ch + 14.0f), cw, ch };
        G.tR = jmin (70.0f, A.getHeight() * 0.14f);
        G.turbo = { x0 + w * 0.5f, A.getBottom() - G.tR - 30.0f };
        return G;
    }
    Point<float> gearPos (int k) const
    {
        const auto R = geo().gate; const int col = k / 2, row = k % 2;
        return { R.getX() + R.getWidth() * (0.12f + 0.3f * (float) col), row == 0 ? R.getY() + 22.0f : R.getBottom() - 22.0f };
    }
    void paintGauge (Graphics& g, const Geo& G)
    {
        const auto c = G.gauge; const float R = G.gR, rpm = rpmShown;
        g.setGradientFill (ColourGradient (Colour (0xff2a2c34), c.x, c.y - R, Colour (0xff0c0d11), c.x, c.y + R, false)); g.fillEllipse (c.x - R, c.y - R, R * 2, R * 2);
        g.setColour (Colours::white.withAlpha (0.25f)); g.drawEllipse (c.x - R, c.y - R, R * 2, R * 2, 2.0f);
        Path redZone; redZone.addCentredArc (c.x, c.y, R * 0.86f, R * 0.86f, 0.0f, a0 + sweep * 0.8f + MathConstants<float>::halfPi, a0 + sweep + MathConstants<float>::halfPi, true);
        g.setColour (Colour (0xffff3b30).withAlpha (0.8f)); g.strokePath (redZone, PathStrokeType (9.0f));
        for (int k = 0; k <= 40; ++k)
        {
            const float u = (float) k / 40.0f, a = a0 + sweep * u, big = k % 5 == 0 ? 1.0f : 0.0f;
            const Point<float> p1 (c.x + std::cos (a) * R * (0.92f - 0.1f * big), c.y + std::sin (a) * R * (0.92f - 0.1f * big)), p2 (c.x + std::cos (a) * R * 0.97f, c.y + std::sin (a) * R * 0.97f);
            g.setColour ((u > 0.8f ? Colour (0xffff6b5b) : Colours::white).withAlpha (u <= rpm ? 0.95f : 0.4f)); g.drawLine (Line<float> (p1, p2), big > 0 ? 3.0f : 1.4f);
        }
        // the lit arc up to the needle
        Path lit; lit.addCentredArc (c.x, c.y, R * 0.74f, R * 0.74f, 0.0f, a0 + MathConstants<float>::halfPi, a0 + sweep * jmax (0.001f, rpm) + MathConstants<float>::halfPi, true);
        g.setColour (amber().interpolatedWith (red(), rpm).withAlpha (0.25f + 0.5f * rpm)); g.strokePath (lit, PathStrokeType (6.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::white.withAlpha (0.6f)); g.setFont (kk::modern::font (11.0f, true, 0.3f));
        g.drawText ("IDLE", Rectangle<float> (c.x - R * 0.6f - 30, c.y + R * 0.4f, 60, 14), Justification::centred);
        g.setColour (Colour (0xffff6b5b)); g.drawText ("REDLINE", Rectangle<float> (c.x + R * 0.52f - 40, c.y + R * 0.4f, 80, 14), Justification::centred);
        const float shake = rpm > 0.85f ? (rpm - 0.85f) * 0.15f * std::sin (phase * 40.0f) : 0.0f;
        const float an = a0 + sweep * rpm + shake;
        const Point<float> tip (c.x + std::cos (an) * R * 0.82f, c.y + std::sin (an) * R * 0.82f), tail (c.x - std::cos (an) * R * 0.15f, c.y - std::sin (an) * R * 0.15f);
        g.setColour (red().withAlpha (0.3f)); g.drawLine (Line<float> (tail, tip), 10.0f);
        g.setColour (red()); g.drawLine (Line<float> (tail, tip), 3.5f);
        g.setGradientFill (ColourGradient (Colour (0xff8c909c), c.x - 6, c.y - 6, Colour (0xff22242a), c.x + 14, c.y + 14, true)); g.fillEllipse (c.x - 16, c.y - 16, 32, 32);
        g.setColour (Colours::white.withAlpha (0.9f)); g.setFont (kk::modern::font (15.0f, true, 0.3f));
        g.drawText (revWord (rpm), Rectangle<float> (c.x - R * 0.7f, c.y + R * 0.5f, R * 1.4f, 22), Justification::centred);
        g.setColour (Colours::white.withAlpha (0.45f)); g.setFont (kk::modern::font (10.5f, true, 0.3f));
        g.drawText ("REV", Rectangle<float> (c.x - 40, c.y - R * 0.45f, 80, 14), Justification::centred);
    }
    void paintPedal (Graphics& g, const Geo& G)
    {
        if (G.pedal.getHeight() < 40.0f) return;
        const auto P = G.pedal; const float press = proc.engine.rev.load();
        g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (P.translated (4, 6), 10);
        const auto top = P.withTrimmedBottom (P.getHeight() * 0.35f * press);
        g.setGradientFill (ColourGradient (Colour (0xff4a4d57), top.getX(), top.getY(), Colour (0xff1a1b20), top.getRight(), top.getBottom(), false)); g.fillRoundedRectangle (top, 10);
        for (int k = 0; k < 5; ++k) { g.setColour (Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (top.getX() + 10, top.getY() + 10 + (float) k * (top.getHeight() - 20) / 5.0f, top.getWidth() - 20, 4, 2); }
        g.setColour (Colours::white.withAlpha (0.3f)); g.drawRoundedRectangle (top, 10, 1.2f);
        g.setColour (Colours::white.withAlpha (0.55f)); g.setFont (kk::modern::font (10.0f, true, 0.3f));
        g.drawText ("PEDAL", P.withY (P.getBottom() + 2).withHeight (14).expanded (20, 0), Justification::centred);
    }
    void paintEngine (Graphics& g, const Geo& G)
    {
        const float rpm = rpmShown, heat = jlimit (0.0f, 1.0f, rpm * 1.1f + turboShown * 0.3f);
        const float vib = (rpm * 2.0f + turboShown) * std::sin (phase * 55.0f);
        const auto E = G.engine.translated (vib * 0.6f, vib * 0.4f);
        const auto steel = Colour (0xff767b88).interpolatedWith (Colour (0xffff5a2a), heat * 0.6f);
        // two banks of cylinders in a V, pistons running with the crank
        for (int bank = 0; bank < 2; ++bank)
        {
            const float ang = bank == 0 ? -0.5f : 0.5f;
            const Point<float> pivot (E.getCentreX(), E.getBottom() - E.getHeight() * 0.25f);
            for (int k = 2; k >= 0; --k)
            {
                g.saveState();
                g.addTransform (AffineTransform::rotation (ang, pivot.x, pivot.y));
                const float cw = E.getWidth() * 0.13f, chh = E.getHeight() * 0.56f;
                const auto cyl = Rectangle<float> (cw, chh).withPosition (pivot.x - cw * 0.5f + (float) k * cw * 0.42f * (bank == 0 ? -1.0f : 1.0f) * 0.0f, pivot.y - chh).translated ((float) k * 10.0f, (float) -k * 12.0f);
                const float shade = 1.0f - 0.22f * (float) k;
                g.setGradientFill (ColourGradient (steel.brighter (0.3f).withMultipliedBrightness (shade), cyl.getX(), cyl.getY(), steel.darker (0.8f).withMultipliedBrightness (shade), cyl.getRight(), cyl.getY(), false));
                g.fillRoundedRectangle (cyl, 6);
                for (int f = 0; f < 7; ++f) { g.setColour (Colours::black.withAlpha (0.35f)); g.fillRect (cyl.getX() - 6, cyl.getY() + 8 + (float) f * 9.0f, cyl.getWidth() + 12, 3.0f); }   // cooling fins
                g.setColour (Colours::white.withAlpha (0.18f * shade)); g.drawRoundedRectangle (cyl, 6, 1.0f);
                const float pist = 0.5f + 0.5f * std::sin (crank + (float) k * 2.094f + (float) bank * 3.14159f);
                const auto pr = Rectangle<float> (cw * 0.7f, chh * 0.18f).withCentre ({ cyl.getCentreX(), cyl.getY() + chh * (0.35f + 0.4f * pist) });
                g.setColour (Colour (0xffd7dbe4).withAlpha (0.85f)); g.fillRoundedRectangle (pr, 3);
                if (heat > 0.3f && pist < 0.2f) { g.setColour (Colour (0xffffb23d).withAlpha ((heat - 0.3f) * 0.8f)); g.fillEllipse (cyl.getCentreX() - 8, cyl.getY() + 4, 16, 12); }   // ignition
                g.restoreState();
            }
        }
        // the block
        const auto block = Rectangle<float> (E.getWidth() * 0.62f, E.getHeight() * 0.36f).withCentre ({ E.getCentreX(), E.getBottom() - E.getHeight() * 0.2f });
        g.setGradientFill (ColourGradient (Colour (0xff4b4f5a), block.getX(), block.getY(), Colour (0xff15161a), block.getRight(), block.getBottom(), false));
        g.fillRoundedRectangle (block, 14);
        g.setColour (Colours::white.withAlpha (0.2f)); g.drawRoundedRectangle (block, 14, 1.4f);
        if (heat > 0.05f) { g.setGradientFill (ColourGradient (Colour (0xffff5a2a).withAlpha (0.45f * heat), block.getCentreX(), block.getCentreY(), Colour (0xffff5a2a).withAlpha (0.0f), block.getRight() + 40, block.getCentreY(), true)); g.fillEllipse (block.expanded (50, 40)); }
        // the crank pulley
        const Point<float> pc (block.getCentreX(), block.getCentreY() + 4);
        g.setColour (Colour (0xff111216)); g.fillEllipse (pc.x - 26, pc.y - 26, 52, 52);
        g.setColour (Colours::white.withAlpha (0.6f)); for (int k = 0; k < 3; ++k) { const float a = crank + (float) k * 2.094f; g.drawLine (pc.x, pc.y, pc.x + std::cos (a) * 22, pc.y + std::sin (a) * 22, 2.0f); }
        // the gear's name on the block
        g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (kk::modern::font (13.0f, true, 0.35f));
        g.drawText (kk::pro::gearName (proc.engine.gear.load()), block.withTrimmedTop (block.getHeight() * 0.62f), Justification::centred);
    }
    void paintPipe (Graphics& g, const Geo& G)
    {
        const auto E = G.engine; const auto T = G.tips[(size_t) jlimit (0, 3, proc.engine.exhaust.load())];
        Path pipe; const Point<float> a (E.getRight() - E.getWidth() * 0.2f, E.getBottom() - E.getHeight() * 0.12f), b (T.getX() + 10, T.getCentreY());
        pipe.startNewSubPath (a); pipe.cubicTo (a.x + 120, a.y + 60, b.x - 140, b.y + 40, b.x, b.y);
        g.setColour (Colours::black.withAlpha (0.5f)); g.strokePath (pipe, PathStrokeType (20.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colour (0xff8a8e99).interpolatedWith (Colour (0xffc46a2a), rpmShown * 0.7f)); g.strokePath (pipe, PathStrokeType (14.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (Colours::white.withAlpha (0.25f)); g.strokePath (pipe, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded), AffineTransform::translation (0, -4));
    }
    void paintTips (Graphics& g, const Geo& G)
    {
        const int sel = proc.engine.exhaust.load();
        static const char* how[] { "as it is", "a bite in the mids", "bright, no low mud", "fat lows, dark top" };
        for (int k = 0; k < kk::pro::numExhausts; ++k)
        {
            const auto R = G.tips[(size_t) k]; const bool on = k == sel;
            g.setColour ((on ? red().withAlpha (0.14f) : Colours::white.withAlpha (0.04f))); g.fillRoundedRectangle (R, 12);
            g.setColour (on ? red() : Colours::white.withAlpha (0.18f)); g.drawRoundedRectangle (R, 12, on ? 2.0f : 1.0f);
            // the pipe ends
            const int n = k == 0 ? 1 : k == 1 ? 2 : k == 2 ? 1 : 4;
            const float pr = k == 2 ? 24.0f : k == 3 ? 11.0f : k == 0 ? 13.0f : 15.0f;
            const auto zone = R.withTrimmedBottom (34).withTrimmedLeft (8);
            for (int i = 0; i < n; ++i)
            {
                const float cx = zone.getX() + 30 + (n == 4 ? (float) (i % 2) * 28.0f : (float) i * 36.0f), cy = zone.getCentreY() + (n == 4 ? ((float) (i / 2) - 0.5f) * 26.0f : 0.0f);
                g.setGradientFill (ColourGradient (Colour (0xffe9ecf2), cx - pr, cy - pr, Colour (0xff5c606b), cx + pr, cy + pr, false)); g.fillEllipse (cx - pr, cy - pr, pr * 2, pr * 2);
                g.setColour (Colour (0xff0a0a0c)); g.fillEllipse (cx - pr * 0.7f, cy - pr * 0.7f, pr * 1.4f, pr * 1.4f);
                if (on && rpmShown > 0.2f)   // flames out of the selected pipe
                    for (int f = 0; f < 5; ++f)
                    {
                        const float len = (20.0f + 70.0f * rpmShown + 40.0f * turboShown) * (0.6f + 0.4f * std::sin (phase * 23.0f + (float) f * 1.7f + (float) i));
                        Path fl; fl.startNewSubPath (cx, cy - pr * 0.5f); fl.quadraticTo (cx + len * 0.6f, cy - pr * 0.6f + (float) f - 2.0f, cx + len, cy + ((float) f - 2.0f) * 2.0f); fl.quadraticTo (cx + len * 0.6f, cy + pr * 0.6f, cx, cy + pr * 0.5f); fl.closeSubPath();
                        g.setColour ((f % 2 == 0 ? Colour (0xffffb23d) : Colour (0xff4da3ff)).withAlpha (0.25f * rpmShown)); g.fillPath (fl);
                    }
            }
            g.setColour (Colours::white.withAlpha (on ? 0.95f : 0.7f)); g.setFont (kk::modern::font (12.5f, true, 0.25f));
            g.drawText (kk::pro::exhaustName (k), R.withTrimmedTop (R.getHeight() - 36).withHeight (16).reduced (10, 0), Justification::centredLeft);
            g.setColour (Colours::white.withAlpha (0.5f)); g.setFont (kk::modern::font (10.5f, true, 0.04f));
            g.drawText (how[k], R.withTrimmedTop (R.getHeight() - 20).withHeight (16).reduced (10, 0), Justification::centredLeft);
        }
        g.setColour (Colours::white.withAlpha (0.45f)); g.setFont (kk::modern::font (10.5f, true, 0.3f));
        g.drawText ("EXHAUST", G.tips[0].withY (G.tips[0].getY() - 20).withHeight (14), Justification::centredLeft);
    }
    void paintGate (Graphics& g, const Geo& G)
    {
        const auto R = G.gate;
        g.setColour (Colour (0xff0c0c10)); g.fillRoundedRectangle (R, 14);
        g.setColour (Colours::white.withAlpha (0.15f)); g.drawRoundedRectangle (R, 14, 1.2f);
        // the H pattern
        g.setColour (Colours::white.withAlpha (0.35f));
        const float ym = R.getCentreY();
        g.drawLine (gearPos (0).x, ym, gearPos (4).x, ym, 6.0f);
        for (int c = 0; c < 3; ++c) g.drawLine (gearPos (c * 2).x, gearPos (c * 2).y, gearPos (c * 2 + 1).x, gearPos (c * 2 + 1).y, 6.0f);
        const int sel = proc.engine.gear.load();
        for (int k = 0; k < kk::pro::numGears; ++k)
        {
            const auto p = gearPos (k);
            g.setColour (k == sel ? amber() : Colours::white.withAlpha (0.55f)); g.setFont (kk::modern::font (11.5f, true, 0.25f));
            g.drawText (kk::pro::gearName (k), Rectangle<float> (p.x + 26, p.y - 8, 80, 16), Justification::centredLeft);
        }
        // the stick knob
        g.setColour (Colours::black.withAlpha (0.4f)); g.fillEllipse (knob.x - 20, knob.y - 14, 44, 36);
        g.setGradientFill (ColourGradient (Colour (0xffeceff5), knob.x - 6, knob.y - 8, Colour (0xff3a3d46), knob.x + 16, knob.y + 16, true)); g.fillEllipse (knob.x - 17, knob.y - 17, 34, 34);
        g.setColour (amber().withAlpha (0.8f)); g.drawEllipse (knob.x - 17, knob.y - 17, 34, 34, 1.5f);
        g.setColour (Colours::white.withAlpha (0.45f)); g.setFont (kk::modern::font (10.5f, true, 0.3f));
        g.drawText ("GEAR", R.withY (R.getY() - 18).withHeight (14), Justification::centredLeft);
    }
    void paintTurbo (Graphics& g, const Geo& G)
    {
        const auto c = G.turbo; const float r = G.tR, t = turboShown;
        if (t > 0.01f) { g.setGradientFill (ColourGradient (Colour (0xff4da3ff).withAlpha (0.5f * t), c.x, c.y, Colour (0xff4da3ff).withAlpha (0.0f), c.x + r * 1.8f, c.y, true)); g.fillEllipse (c.x - r * 1.8f, c.y - r * 1.8f, r * 3.6f, r * 3.6f); }
        g.setGradientFill (ColourGradient (Colour (0xff3a3e48), c.x - r * 0.5f, c.y - r * 0.5f, Colour (0xff101116), c.x + r, c.y + r, true)); g.fillEllipse (c.x - r, c.y - r, r * 2, r * 2);
        g.setColour ((t > 0.05f ? Colour (0xff4da3ff) : Colours::white.withAlpha (0.35f))); g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, 2.0f);
        for (int k = 0; k < 9; ++k)   // the turbine blades
        {
            const float a = turbAngle + (float) k * MathConstants<float>::twoPi / 9.0f;
            Path b; b.startNewSubPath (c.x + std::cos (a) * r * 0.18f, c.y + std::sin (a) * r * 0.18f);
            b.quadraticTo (c.x + std::cos (a + 0.5f) * r * 0.6f, c.y + std::sin (a + 0.5f) * r * 0.6f, c.x + std::cos (a + 0.9f) * r * 0.85f, c.y + std::sin (a + 0.9f) * r * 0.85f);
            g.setColour (Colours::white.withAlpha (0.35f + 0.4f * t)); g.strokePath (b, PathStrokeType (2.2f));
        }
        g.setColour (Colours::white.withAlpha (0.95f)); g.setFont (kk::modern::font (14.0f, true, 0.4f));
        g.drawText ("TURBO", Rectangle<float> (c.x - r, c.y + r + 6, r * 2, 18), Justification::centred);
        g.setColour (Colours::white.withAlpha (0.45f)); g.setFont (kk::modern::font (10.0f, true, 0.2f));
        g.drawText ("hold", Rectangle<float> (c.x - r, c.y - 7, r * 2, 14), Justification::centred);
    }
    static String revWord (float r) { return r < 0.05f ? "idle" : r < 0.3f ? "cruising" : r < 0.6f ? "pulling" : r < 0.85f ? "screaming" : "REDLINE"; }
    void refresh() { onBtn.selected = proc.engine.on.load(); onBtn.setButtonText (proc.engine.on.load() ? "ON" : "OFF"); onBtn.repaint(); repaint(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        const auto& st = proc.engine;
        phase += 1.0f / 30.0f;
        const float rv = st.on.load() ? st.rev.load() : 0.0f;
        rpmShown += (rv - rpmShown) * 0.25f;
        turboShown += ((st.turbo.load() && st.on.load() ? 1.0f : 0.0f) - turboShown) * 0.15f;
        crank += 0.15f + 1.6f * rpmShown;
        turbAngle += 0.05f + 0.6f * turboShown + 0.3f * rpmShown;
        knob += (gearPos (st.gear.load()) - knob) * 0.35f;
        if (onBtn.selected != st.on.load()) refresh();
        repaint (area());
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf }, idleBtn { lnf };
    Mode mode = emNone; float startRev = 0, phase = 0, rpmShown = 0, turboShown = 0, crank = 0, turbAngle = 0; Point<float> knob;
};

// ---------------------------------------------------------------- DRAW (automation) ----------------------------------------------------------------
class DrawAutoPage : public Component, public SettableTooltipClient, private Timer
{
public:
    DrawAutoPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        onBtn.setButtonText ("OFF"); onBtn.framed = true; addAndMakeVisible (onBtn);
        onBtn.setTooltip ("DRAW on / off - the curve moves its target in time with FL");
        onBtn.onClick = [this] { proc.drawAuto.on = ! proc.drawAuto.on.load(); refresh(); };
        static const char* tips[] { "the curve opens and closes a filter", "the curve sends the track into a hall", "the curve burns the track", "the curve squeezes it to mono or spreads it wide", "the curve is the volume" };
        for (int k = 0; k < kk::pro::numDrawTargets; ++k)
        {
            auto b = std::make_unique<HotButton> (lnf, kk::pro::drawTargetName (k)); b->framed = true; b->setTooltip (tips[k]); b->tint = targetColour (k);
            b->onClick = [this, k] { proc.drawAuto.target = k; proc.drawAuto.on = true; refresh(); };
            addAndMakeVisible (*b); targetBtns.push_back (std::move (b));
        }
        static const char* bars[] { "1 BAR", "2 BARS", "4 BARS", "8 BARS" };
        for (int k = 0; k < 4; ++k)
        {
            auto b = std::make_unique<HotButton> (lnf, bars[k]); b->framed = true; b->setTooltip ("How long the curve is - it follows FL's play head");
            b->onClick = [this, k] { proc.drawAuto.bars = kk::pro::DrawAutoState::barsFor (k); refresh(); };
            addAndMakeVisible (*b); barBtns.push_back (std::move (b));
        }
        static const char* shapes[] { "BUILD-UP", "WAVE", "FALL", "STEPS", "FLAT" };
        for (int k = 0; k < 5; ++k)
        {
            auto b = std::make_unique<HotButton> (lnf, shapes[k]); b->framed = true; b->setTooltip (k < 4 ? String ("A ready curve: ") + shapes[k] : String ("A flat line where the target sounds as it is"));
            b->onClick = [this, k] { if (k < 4) proc.drawAuto.shape (k); else flat(); proc.drawAuto.on = true; refresh(); };
            addAndMakeVisible (*b); shapeBtns.push_back (std::move (b));
        }
        headPos = proc.drawAuto.mPos.load(); headVal = proc.drawAuto.mValue.load();
        setTooltip ("Draw the curve with the mouse - it moves the chosen target bar after bar, in time with FL (when FL stops it keeps running on its own).  Double-click = a flat line.");
        startTimerHz (30);
        refresh();
    }
    static Colour yellow() { return Colour (0xffffd23f); }
    static Colour targetColour (int k) { static const uint32 c[] { 0xff3ee8ff, 0xffa78bfa, 0xffff6a3d, 0xff36ff9a, 0xffffd23f }; return Colour (c[(size_t) jlimit (0, 4, k)]); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        proHeader (g, *this, "DRAW", "TOUCH  -  DRAWN AUTOMATION", "draw a curve over 1, 2, 4 or 8 bars - it moves one thing in time with your track: the filter, the space, the drive, the width or the volume", yellow());
        const auto& st = proc.drawAuto;
        const int tg = st.target.load();
        const auto col = targetColour (tg);
        g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.3f));
        g.drawText ("IT MOVES", 24, 108, 90, 20, Justification::centredLeft);
        g.drawText ("SHAPE", 24, 152, 90, 20, Justification::centredLeft);
        g.setColour (t.night ? col : col.darker (0.9f)); g.setFont (kk::modern::font (13.0f, true, 0.05f));
        g.drawFittedText (String (kk::pro::drawTargetName (tg)) + ":  " + kk::pro::drawTargetDoes (tg), Rectangle<int> (shapeBtns.back()->getRight() + 20, 146, getWidth() - shapeBtns.back()->getRight() - 44, 32), Justification::centredLeft, 1, 0.8f);
        const auto C = canvas().toFloat();
        const bool on = st.on.load();
        worldFrame (g, C, Colour (0xff10121a), Colour (0xff06070b));
        g.saveState(); g.reduceClipRegion (C.toNearestInt());
        const int bars = st.bars.load(), beats = bars * 4;
        for (int b = 0; b <= beats; ++b)
        {
            const float x = C.getX() + C.getWidth() * (float) b / (float) beats;
            g.setColour (Colours::white.withAlpha (b % 4 == 0 ? 0.16f : 0.05f)); g.fillRect (x, C.getY(), b % 4 == 0 ? 1.5f : 1.0f, C.getHeight());
        }
        for (int k = 1; k < 4; ++k) { g.setColour (Colours::white.withAlpha (k == 2 ? 0.08f : 0.04f)); g.fillRect (C.getX(), C.getY() + C.getHeight() * (float) k / 4.0f, C.getWidth(), 1.0f); }
        // the curve
        Path line, fill;
        const int n = kk::pro::DrawAutoState::numPoints;
        for (int k = 0; k < n; ++k)
        {
            const Point<float> p (C.getX() + C.getWidth() * (float) k / (float) (n - 1), yOf (st.curve[(size_t) k].load()));
            if (k == 0) { line.startNewSubPath (p); fill.startNewSubPath (C.getX(), C.getBottom()); fill.lineTo (p); } else { line.lineTo (p); fill.lineTo (p); }
        }
        fill.lineTo (C.getRight(), C.getBottom()); fill.closeSubPath();
        g.setGradientFill (ColourGradient (col.withAlpha (on ? 0.35f : 0.15f), C.getX(), C.getY(), col.withAlpha (0.02f), C.getX(), C.getBottom(), false)); g.fillPath (fill);
        g.setColour (col.withAlpha (0.25f)); g.strokePath (line, PathStrokeType (10.0f, PathStrokeType::curved, PathStrokeType::rounded));
        g.setColour (on ? col : col.withAlpha (0.5f)); g.strokePath (line, PathStrokeType (3.0f, PathStrokeType::curved, PathStrokeType::rounded));
        // the play head
        if (on)
        {
            const float x = C.getX() + C.getWidth() * headPos, y = yOf (headVal);
            g.setColour (Colours::white.withAlpha (0.55f)); g.fillRect (x - 0.75f, C.getY(), 1.5f, C.getHeight());
            g.setGradientFill (ColourGradient (col.withAlpha (0.8f), x, y, col.withAlpha (0.0f), x + 26, y, true)); g.fillEllipse (x - 26, y - 26, 52, 52);
            g.setColour (Colours::white); g.fillEllipse (x - 6, y - 6, 12, 12);
        }
        // what the top and the bottom mean
        static const char* top[] { "OPEN", "DEEP IN A HALL", "BURNING", "EXTRA WIDE", "FULL" };
        static const char* bot[] { "CLOSED", "DRY", "CLEAN", "MONO", "SILENT" };
        g.setColour (Colours::white.withAlpha (0.6f)); g.setFont (kk::modern::font (12.0f, true, 0.3f));
        g.drawText (top[tg], C.reduced (14, 10).removeFromTop (16), Justification::topLeft);
        g.drawText (bot[tg], C.reduced (14, 10).removeFromBottom (16), Justification::bottomLeft);
        if (! on) { g.setColour (yellow().withAlpha (0.9f)); g.setFont (kk::modern::font (14.0f, true, 0.3f)); g.drawText ("OFF  -  DRAW A LINE TO START", C.withTrimmedTop (C.getHeight() * 0.42f).withHeight (24), Justification::centred); }
        g.restoreState();
        g.setColour (yellow().withAlpha (0.35f)); g.drawRoundedRectangle (C, 18, 1.2f);
        touchHints (g, worldHintRow (*this), { "DRAW = the curve", "PICK WHAT IT MOVES = the words above", "BARS = how long", "DOUBLE-CLICK = flat" }, yellow());
    }
    void resized() override
    {
        onBtn.setBounds (300, 56, 70, 32);
        int x = 110; for (auto& b : targetBtns) { b->setBounds (x, 102, 120, 34); x += 126; }
        x = getWidth() - 24 - 4 * 92; for (auto& b : barBtns) { b->setBounds (x, 102, 88, 34); x += 92; }
        x = 110; for (auto& b : shapeBtns) { b->setBounds (x, 146, 104, 32); x += 110; }
    }
    void mouseDown (const MouseEvent& e) override
    {
        if (! canvas().toFloat().contains (e.position)) { drawing = false; return; }
        drawing = true; last = e.position; paintAt (e.position, e.position); proc.drawAuto.on = true; refresh();
    }
    void mouseDrag (const MouseEvent& e) override { if (! drawing) return; paintAt (last, e.position); last = e.position; repaint (canvas()); }
    void mouseUp (const MouseEvent&) override { drawing = false; }
    void mouseDoubleClick (const MouseEvent& e) override { if (canvas().contains (e.getPosition())) { flat(); repaint(); } }
private:
    Rectangle<int> canvas() const { return { 24, 196, getWidth() - 48, getHeight() - 196 - 70 }; }
    float yOf (float v) const { const auto C = canvas().toFloat().reduced (0, 14); return C.getBottom() - C.getHeight() * jlimit (0.0f, 1.0f, v); }
    void paintAt (Point<float> a, Point<float> b)
    {
        const auto C = canvas().toFloat(), Cv = C.reduced (0, 14);
        const int n = kk::pro::DrawAutoState::numPoints;
        auto idx = [&] (float x) { return jlimit (0, n - 1, (int) std::round ((x - C.getX()) / C.getWidth() * (float) (n - 1))); };
        auto val = [&] (float y) { return jlimit (0.0f, 1.0f, (Cv.getBottom() - y) / Cv.getHeight()); };
        const int i0 = idx (a.x), i1 = idx (b.x);
        const float v0 = val (a.y), v1 = val (b.y);
        if (i0 == i1) { proc.drawAuto.curve[(size_t) i1] = v1; return; }
        const int lo = jmin (i0, i1), hi = jmax (i0, i1);
        for (int k = lo; k <= hi; ++k) { const float u = (float) (k - i0) / (float) (i1 - i0); proc.drawAuto.curve[(size_t) k] = v0 + (v1 - v0) * u; }
    }
    void flat()
    {
        static const float neutral[] { 1.0f, 0.0f, 0.0f, 0.5f, 1.0f };
        for (auto& c : proc.drawAuto.curve) c = neutral[jlimit (0, 4, proc.drawAuto.target.load())];
    }
    void refresh()
    {
        const auto& st = proc.drawAuto;
        onBtn.selected = st.on.load(); onBtn.setButtonText (st.on.load() ? "ON" : "OFF"); onBtn.repaint();
        for (int k = 0; k < (int) targetBtns.size(); ++k) { targetBtns[(size_t) k]->selected = k == st.target.load(); targetBtns[(size_t) k]->repaint(); }
        for (int k = 0; k < (int) barBtns.size(); ++k) { barBtns[(size_t) k]->selected = kk::pro::DrawAutoState::barsFor (k) == st.bars.load(); barBtns[(size_t) k]->repaint(); }
        repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        const auto& st = proc.drawAuto;
        if (onBtn.selected != st.on.load()) refresh();
        if (! st.on.load()) return;
        headPos = st.mPos.load(); headVal = st.mValue.load();
        repaint (canvas());
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf };
    std::vector<std::unique_ptr<HotButton>> targetBtns, barBtns, shapeBtns;
    bool drawing = false; Point<float> last; float headPos = 0, headVal = 0;
};

// ---------------------------------------------------------------- MOOD WORDS ----------------------------------------------------------------
class MoodWordsPage : public Component, private Timer
{
public:
    MoodWordsPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        box.setFont (kk::modern::font (24.0f, true, 0.04f));
        box.setIndents (18, 14);
        box.setTextToShowWhenEmpty ("type how the track should feel ...", kk::theme().dim);
        box.setColour (TextEditor::backgroundColourId, Colour (0xff0c1418)); box.setColour (TextEditor::textColourId, Colours::white);
        box.setColour (TextEditor::outlineColourId, teal().withAlpha (0.45f)); box.setColour (TextEditor::focusedOutlineColourId, teal());
        box.setColour (CaretComponent::caretColourId, teal()); box.setColour (TextEditor::highlightColourId, teal().withAlpha (0.35f));
        box.onTextChange = [this] { debounce = 9; };
        box.onReturnKey = [this] { apply(); };
        box.setTooltip ("Any words, English or Czech: night drive, underwater, angry, dreamy, nocni jizda ... unknown words get their own mood too");
        addAndMakeVisible (box);
        static const char* ideas[] { "night drive", "underwater", "angry", "dreamy", "noční jízda", "disco party", "old phone", "frozen cave" };
        for (auto* w : ideas)
        {
            auto b = std::make_unique<HotButton> (lnf, String (CharPointer_UTF8 (w)).toUpperCase()); b->framed = true; b->setTooltip ("Try it");
            b->onClick = [this, s = String (CharPointer_UTF8 (w))] { box.setText (s, false); apply(); };
            addAndMakeVisible (*b); ideaBtns.push_back (std::move (b));
        }
        startTimerHz (30);
    }
    std::function<void (int)> onOpenPage;   // FxMainPage: open a module's page (flat page index)
    static int pageOf (int m) { static const int pg[] { 2, 10, 5, 6, 9, 11, 14, 15, 16 }; return pg[jlimit (0, 8, m)]; }
    static Colour teal() { return Colour (0xff2ee6c6); }
    void debugType (const String& s) { box.setText (s, false); apply(); phase = 1.3f; shown = res.v; for (auto& p : pulse) p *= 0.4f; }
    void paint (Graphics& g) override
    {
        proHeader (g, *this, "MOOD WORDS", "WORLDS  -  THE WORDS' TWIN", "type how the track should feel - the FX PRO modules arrange themselves.  the same words always give the same mix", teal());
        const auto& t = kk::theme();
        // the words it read
        {
            float x = 24.0f; const float y = 214.0f;
            g.setColour (t.dim); g.setFont (kk::modern::font (11.0f, true, 0.3f)); g.drawText ("HEARD", 24, (int) y, 70, 26, Justification::centredLeft); x += 70.0f;
            if (res.words.empty()) { g.setColour (t.dim.withAlpha (0.8f)); g.setFont (kk::modern::font (13.0f, true, 0.04f)); g.drawText ("nothing yet - the mood modules rest", (int) x, (int) y, 500, 26, Justification::centredLeft); }
            for (auto& w : res.words)
            {
                const String s (CharPointer_UTF8 (w.word.c_str()));
                g.setFont (kk::modern::font (13.0f, true, 0.1f));
                const float tw = GlyphArrangement::getStringWidth (g.getCurrentFont(), s) + 28.0f;
                if (x + tw > (float) getWidth() - 290.0f) break;
                const Rectangle<float> r (x, y, tw, 26);
                const auto c = w.known ? teal() : Colour (0xffa78bfa);
                g.setColour (c.withAlpha (w.known ? 0.2f : 0.1f)); g.fillRoundedRectangle (r, 13);
                if (w.known) { g.setColour (c); g.drawRoundedRectangle (r.reduced (0.5f), 13, 1.2f); }
                else { Path o, d; o.addRoundedRectangle (r.reduced (0.5f), 13); const float dash[] { 4.0f, 3.0f }; PathStrokeType (1.2f).createDashedStroke (d, o, dash, 2); g.setColour (c); g.fillPath (d); }
                g.setColour (t.night ? Colours::white : t.text); g.drawText (s, r, Justification::centred);
                x += tw + 8.0f;
            }
            if (! res.words.empty())
            {
                g.setColour (t.dim); g.setFont (kk::modern::font (10.5f, true, 0.04f));
                g.drawFittedText ("solid = a word it knows\ndashed = a new word, its own mood", getWidth() - 274, (int) y - 4, 250, 34, Justification::centredRight, 2);
            }
        }
        {
            const auto P = flowerArea().getUnion (cardsArea()).expanded (10, 10).withBottom (getHeight() - 16).toFloat();
            worldFrame (g, P, Colour (0xff0c1418), Colour (0xff05080a));
            g.setColour (teal().withAlpha (0.3f)); g.drawRoundedRectangle (P, 18, 1.2f);
        }
        paintFlower (g, flowerArea().toFloat());
        for (int m = 0; m < kk::pro::numMoodModules; ++m) paintCard (g, m);
        g.setColour (t.dim); g.setFont (kk::modern::font (12.0f, true, 0.04f));
        g.setColour (teal().withAlpha (0.8f)); g.drawText ("click a lit module to open it and keep shaping it by hand", cardsArea().getX(), getHeight() - 46, cardsArea().getWidth(), 20, Justification::centredLeft);
    }
    void resized() override
    {
        box.setBounds (24, 100, getWidth() - 48, 56);
        int x = 24; for (auto& b : ideaBtns) { b->setBounds (x, 166, 150, 34); x += 156; }
    }
    void mouseUp (const MouseEvent& e) override
    {
        for (int m = 0; m < kk::pro::numMoodModules; ++m)
            if (card (m).contains (e.position) && onOpenPage) { onOpenPage (pageOf (m)); return; }
    }
    void mouseMove (const MouseEvent& e) override
    {
        int h = -1; for (int m = 0; m < kk::pro::numMoodModules; ++m) if (card (m).contains (e.position)) h = m;
        if (h != hover) { hover = h; setMouseCursor (h >= 0 ? MouseCursor::PointingHandCursor : MouseCursor::NormalCursor); repaint (cardsArea()); }
    }
private:
    Rectangle<int> flowerArea() const { return { 34, 266, jmin (430, getWidth() / 3), getHeight() - 266 - 30 }; }
    Rectangle<int> cardsArea() const { const auto F = flowerArea(); return { F.getRight() + 20, 266, getWidth() - F.getRight() - 54, getHeight() - 266 - 60 }; }
    Rectangle<float> card (int m) const
    {
        const auto C = cardsArea().toFloat(); const float gap = 12.0f, w = (C.getWidth() - 2 * gap) / 3.0f, h = (C.getHeight() - 2 * gap) / 3.0f;
        return { C.getX() + (float) (m % 3) * (w + gap), C.getY() + (float) (m / 3) * (h + gap), w, h };
    }
    static Colour dimColour (int d) { static const uint32 c[] { 0xff5b6bff, 0xffffe066, 0xffa78bfa, 0xffff6a3d, 0xffb0a090, 0xffff4fd8, 0xffff3b30, 0xff36ff9a, 0xff22a8ee, 0xffff4fd8, 0xffff9ad5, 0xff7fe7ff, 0xffffa040, 0xffd2b48c, 0xff9dff3d, 0xff8b9cff, 0xffffd0a0, 0xffff2e88 };
                                    return Colour (c[(size_t) jlimit (0, (int) kk::pro::numMoodDims - 1, d)]); }
    static const char* dimName (int d) { static const char* n[] { "DARK", "BRIGHT", "SPACE", "GRIT", "LO-FI", "WOBBLE", "ANGER", "CALM", "WATER", "DANCE", "SWEET", "ICE", "WARM", "OLD", "ALIEN", "FAR", "NEAR", "GLITCH" }; return n[jlimit (0, 17, d)]; }
    void paintFlower (Graphics& g, Rectangle<float> F)
    {
        g.setColour (Colours::white.withAlpha (0.03f)); g.fillRoundedRectangle (F, 16.0f);
        const auto c = F.getCentre().translated (0, 6); const float R = jmin (F.getWidth(), F.getHeight()) * 0.5f - 46.0f;
        const int n = kk::pro::numMoodDims;
        for (int k = 1; k <= 3; ++k) { g.setColour (Colours::white.withAlpha (0.05f)); g.drawEllipse (c.x - R * (float) k / 3.0f, c.y - R * (float) k / 3.0f, R * 2.0f * (float) k / 3.0f, R * 2.0f * (float) k / 3.0f, 1.0f); }
        for (int d = 0; d < n; ++d)
        {
            const float a = -MathConstants<float>::halfPi + MathConstants<float>::twoPi * (float) d / (float) n;
            const float v = shown[(size_t) d], breath = 1.0f + 0.04f * std::sin (phase * 2.0f + (float) d);
            const float len = R * (0.12f + 0.88f * v) * breath, w = 0.13f;
            Path petal; petal.startNewSubPath (c);
            petal.quadraticTo (c.x + std::cos (a - w * 2.0f) * len * 0.7f, c.y + std::sin (a - w * 2.0f) * len * 0.7f, c.x + std::cos (a) * len, c.y + std::sin (a) * len);
            petal.quadraticTo (c.x + std::cos (a + w * 2.0f) * len * 0.7f, c.y + std::sin (a + w * 2.0f) * len * 0.7f, c.x, c.y); petal.closeSubPath();
            const auto col = dimColour (d);
            g.setColour (col.withAlpha (0.12f + 0.55f * v)); g.fillPath (petal);
            g.setColour (col.withAlpha (0.25f + 0.6f * v)); g.strokePath (petal, PathStrokeType (1.2f));
            const Point<float> lp (c.x + std::cos (a) * (R + 22.0f), c.y + std::sin (a) * (R + 22.0f));
            g.setColour ((v > 0.05f ? col : Colours::white.withAlpha (0.3f))); g.setFont (kk::modern::font (10.0f, true, 0.15f));
            g.drawText (dimName (d), Rectangle<float> (lp.x - 34, lp.y - 7, 68, 14), Justification::centred);
        }
        g.setColour (Colours::white.withAlpha (0.8f)); g.fillEllipse (c.x - 5, c.y - 5, 10, 10);
        g.setColour (Colours::white.withAlpha (0.5f)); g.setFont (kk::modern::font (11.0f, true, 0.3f));
        g.drawText ("THE MOOD", F.reduced (14, 10).removeFromTop (16), Justification::topLeft);
    }
    void paintCard (Graphics& g, int m)
    {
        const auto r = card (m);
        const bool moved = res.moved[(size_t) m];
        const auto col = moduleColour (m);
        const float pl = pulse[(size_t) m];
        g.setColour (moved ? col.withAlpha (0.14f + 0.2f * pl) : Colours::white.withAlpha (0.03f)); g.fillRoundedRectangle (r, 12);
        g.setColour (moved ? col.withAlpha (0.8f + 0.2f * pl) : Colours::white.withAlpha (m == hover ? 0.3f : 0.12f)); g.drawRoundedRectangle (r, 12, moved ? 1.8f + 2.0f * pl : 1.0f);
        if (moved && pl > 0.01f) { g.setColour (col.withAlpha (0.35f * pl)); g.drawRoundedRectangle (r.expanded (4.0f + 8.0f * (1.0f - pl)), 15, 1.5f); }
        g.setColour (moved ? Colours::white : Colours::white.withAlpha (0.4f)); g.setFont (kk::modern::font (15.0f, true, 0.2f));
        g.drawText (kk::pro::moodModuleName (m), r.reduced (16, 12).removeFromTop (20), Justification::centredLeft);
        g.setColour (moved ? col : Colours::white.withAlpha (0.28f)); g.setFont (kk::modern::font (12.5f, true, 0.04f));
        g.drawFittedText (moved ? String (CharPointer_UTF8 (res.how[(size_t) m].c_str())) : String ("resting"), r.reduced (16, 12).withTrimmedTop (26).toNearestInt(), Justification::topLeft, 3, 0.85f);
        if (moved)
        {
            g.setColour (col); g.fillEllipse (r.getRight() - 22, r.getY() + 14, 8, 8);
            g.setColour (col.withAlpha (m == hover ? 1.0f : 0.6f)); g.setFont (kk::modern::font (11.0f, true, 0.3f));
            g.drawText ("OPEN  >", r.reduced (16, 10).removeFromBottom (16), Justification::centredRight);
        }
    }
    static Colour moduleColour (int m) { static const uint32 c[] { 0xff3ee8ff, 0xffff6a3d, 0xff4dd2ff, 0xffb04dff, 0xff22d3ee, 0xffc9a36b, 0xffff4fd8, 0xffff9a3d, 0xffff4d3d }; return Colour (c[(size_t) jlimit (0, 8, m)]); }
    void apply()
    {
        debounce = -1;
        const auto text = box.getText();
        kk::pro::MoodTargets tg { proc.holoroom, proc.intent, proc.dial, proc.warp, proc.liquid, proc.erosion, proc.club, proc.season, proc.engine };
        const auto before = res.moved;
        res = kk::pro::moodApply (text.toLowerCase().toStdString(), tg);
        for (int m = 0; m < kk::pro::numMoodModules; ++m) if (res.moved[(size_t) m] && (! before[(size_t) m] || text != lastText)) pulse[(size_t) m] = 1.0f;
        lastText = text;
        repaint();
    }
    void timerCallback() override
    {
        if (debounce > 0 && --debounce == 0) apply();
        if (! isShowing()) return;
        phase += 1.0f / 30.0f;
        for (int d = 0; d < kk::pro::numMoodDims; ++d) shown[(size_t) d] += (res.v[(size_t) d] - shown[(size_t) d]) * 0.2f;
        for (auto& p : pulse) p *= 0.94f;
        repaint (flowerArea()); repaint (cardsArea().expanded (14));
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    TextEditor box;
    std::vector<std::unique_ptr<HotButton>> ideaBtns;
    kk::pro::MoodResult res; String lastText;
    std::array<float, kk::pro::numMoodDims> shown {}; std::array<float, kk::pro::numMoodModules> pulse {};
    int debounce = -1, hover = -1; float phase = 0;
};
