#include <complex>
// v0.44 EVOLVE FX PRO - TOUCH pages (included by FxMainPage.h after FxProPages.h): HOLOROOM, GRAB
// No knobs, no numbers: the hand moves the sound itself, every movement is heard at once.

// a small "how to touch it" chip row along the bottom of a TOUCH page
static void touchHints (Graphics& g, Rectangle<float> row, const StringArray& hints, Colour col)
{
    const auto& t = kk::theme();
    const float gap = 10.0f, w = (row.getWidth() - gap * (float) (hints.size() - 1)) / (float) hints.size();
    for (int i = 0; i < hints.size(); ++i)
    {
        const auto r = Rectangle<float> (row.getX() + (float) i * (w + gap), row.getY(), w, row.getHeight());
        g.setColour (col.withAlpha (t.night ? 0.08f : 0.12f)); g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
        g.setColour (col.withAlpha (0.45f)); g.drawRoundedRectangle (r.reduced (0.5f), r.getHeight() * 0.5f, 1.0f);
        const auto verb = hints[i].upToFirstOccurrenceOf ("=", false, false).trim(), what = hints[i].fromFirstOccurrenceOf ("=", false, false).trim();
        g.setFont (kk::modern::font (12.5f, true, 0.12f));
        const float vw = GlyphArrangement::getStringWidth (g.getCurrentFont(), verb) + 16.0f;
        g.setColour (t.night ? col.brighter (0.4f) : col.darker (0.9f)); g.drawText (verb, r.withTrimmedLeft (16).withWidth (vw), Justification::centredLeft);
        g.setColour (t.text.withAlpha (0.85f)); g.setFont (kk::modern::font (12.5f, true, 0.02f));
        g.drawFittedText (what, r.withTrimmedLeft (16 + vw).withTrimmedRight (12).toNearestInt(), Justification::centredLeft, 1, 0.8f);
    }
}

// ---------------------------------------------------------------- HOLOROOM ----------------------------------------------------------------
class HoloroomPage : public Component, public SettableTooltipClient, private Timer
{
public:
    HoloroomPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        onBtn.setButtonText ("OFF"); onBtn.framed = true; addAndMakeVisible (onBtn);
        onBtn.setTooltip ("HOLOROOM on / off - the sound is the orb, the room is around it");
        onBtn.onClick = [this] { proc.holoroom.on = ! proc.holoroom.on.load(); refresh(); };
        setTooltip ("Drag the orb: left / right = pan + width, deeper = further away (quieter, darker, more room).  "
                    "Right-drag (or shift-drag) = up / down (airy / heavy).  Drag a corner of the room = its size.  Double-click the orb = back to the centre.");
        level = proc.holoroom.mLevel.load(); wet = proc.holoroom.mWet.load();
        startTimerHz (30);
        refresh();
    }
    static Colour cyan()    { return Colour (0xff3ee8ff); }
    static Colour magenta() { return Colour (0xffff4fd8); }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        proHeader (g, *this, "HOLOROOM", "TOUCH", "the sound is the glowing orb - move it through the room.  near = present and dry, far = small, dark, far away", cyan());
        const auto A = area().toFloat();
        const auto& st = proc.holoroom;
        const bool on = st.on.load();
        const float room = st.room.load(), ox = st.x.load(), od = st.depth.load(), oh = st.height.load();
        // the void
        g.setGradientFill (ColourGradient (Colour (0xff070b18), A.getCentreX(), A.getCentreY(), Colour (0xff010208), A.getX(), A.getY(), true));
        g.fillRoundedRectangle (A, 18);
        g.saveState(); g.reduceClipRegion (A.toNearestInt());
        const Geo G = geo (room);
        const float glowA = on ? 1.0f : 0.45f;
        // walls, ceiling, floor
        auto quad = [&] (Point<float> a, Point<float> b, Point<float> c, Point<float> d) { Path p; p.startNewSubPath (a); p.lineTo (b); p.lineTo (c); p.lineTo (d); p.closeSubPath(); return p; };
        const auto fl0 = G.P (-1, 0, 0), fr0 = G.P (1, 0, 0), fl1 = G.P (-1, 0, 1), fr1 = G.P (1, 0, 1);
        const auto cl0 = G.P (-1, 1, 0), cr0 = G.P (1, 1, 0), cl1 = G.P (-1, 1, 1), cr1 = G.P (1, 1, 1);
        g.setGradientFill (ColourGradient (cyan().withAlpha (0.10f * glowA), fl1.x, fl1.y, Colour (0xff0b1630).withAlpha (0.6f), fl0.x, fl0.y, false));
        g.fillPath (quad (fl0, fr0, fr1, fl1));
        g.setGradientFill (ColourGradient (magenta().withAlpha (0.07f * glowA), cl1.x, cl1.y, Colours::transparentBlack, cl0.x, cl0.y, false));
        g.fillPath (quad (fl0, fl1, cl1, cl0)); g.fillPath (quad (fr0, fr1, cr1, cr0));
        g.setColour (Colour (0xff0d1630).withAlpha (0.55f)); g.fillPath (quad (fl1, fr1, cr1, cl1));
        // floor grid (perspective)
        for (int i = 0; i <= 8; ++i)
        {
            const float u = -1.0f + 2.0f * (float) i / 8.0f;
            g.setColour (cyan().withAlpha ((i == 4 ? 0.32f : 0.16f) * glowA)); g.drawLine (Line<float> (G.P (u, 0, 0), G.P (u, 0, 1)), 1.0f);
        }
        for (int i = 0; i <= 10; ++i)
        {
            const float z = (float) i / 10.0f;
            g.setColour (cyan().withAlpha (0.16f * glowA * (1.0f - 0.5f * z))); g.drawLine (Line<float> (G.P (-1, 0, z), G.P (1, 0, z)), 1.0f);
            g.setColour (magenta().withAlpha (0.07f * glowA)); g.drawLine (Line<float> (G.P (-1, 0, z), G.P (-1, 1, z)), 1.0f); g.drawLine (Line<float> (G.P (1, 0, z), G.P (1, 1, z)), 1.0f);
        }
        for (int i = 1; i < 4; ++i) { const float v = (float) i / 4.0f; g.setColour (magenta().withAlpha (0.06f * glowA)); g.drawLine (Line<float> (G.P (-1, v, 0), G.P (-1, v, 1)), 1.0f); g.drawLine (Line<float> (G.P (1, v, 0), G.P (1, v, 1)), 1.0f); g.drawLine (Line<float> (G.P (-1, v, 1), G.P (1, v, 1)), 1.0f); }
        // the room's edges
        auto edge = [&] (Point<float> a, Point<float> b, float w) { g.setColour (cyan().withAlpha (0.12f * glowA)); g.drawLine (Line<float> (a, b), w + 5.0f); g.setColour (cyan().withAlpha (0.75f * glowA)); g.drawLine (Line<float> (a, b), w); };
        edge (fl0, fl1, 1.4f); edge (fr0, fr1, 1.4f); edge (cl0, cl1, 1.2f); edge (cr0, cr1, 1.2f);
        edge (fl1, fr1, 1.4f); edge (cl1, cr1, 1.2f); edge (fl1, cl1, 1.2f); edge (fr1, cr1, 1.2f);
        // the room corners: drag them
        for (auto c : { fl0, fr0, cl0, cr0 })
        {
            const bool hot = cornerHot;
            g.setColour (cyan().withAlpha (hot ? 0.35f : 0.18f)); g.fillEllipse (c.x - 16, c.y - 16, 32, 32);
            g.setColour (cyan().withAlpha (0.9f)); g.drawEllipse (c.x - 9, c.y - 9, 18, 18, 2.0f);
            g.fillEllipse (c.x - 3.5f, c.y - 3.5f, 7, 7);
        }
        // the orb, its shadow, its reflections
        const auto orb = G.P (ox, 0.5f + 0.4f * oh, od), shadow = G.P (ox, 0, od);
        const float sc = G.scale (od), rad = 40.0f * sc;
        const float pulse = jlimit (0.0f, 1.0f, level * 1.4f);
        const float shR = rad * (1.6f - 0.4f * oh);
        g.setGradientFill (ColourGradient (Colours::black.withAlpha (0.7f), shadow.x, shadow.y, Colours::transparentBlack, shadow.x + shR * 1.4f, shadow.y, true));
        g.fillEllipse (shadow.x - shR * 1.4f, shadow.y - shR * 0.35f, shR * 2.8f, shR * 0.7f);
        g.setGradientFill (ColourGradient (cyan().withAlpha ((0.35f + 0.4f * pulse) * glowA), shadow.x, shadow.y, cyan().withAlpha (0.0f), shadow.x + shR * 1.8f, shadow.y, true));
        g.fillEllipse (shadow.x - shR * 1.8f, shadow.y - shR * 0.45f, shR * 3.6f, shR * 0.9f);
        g.setColour (Colours::black.withAlpha (0.75f)); g.fillEllipse (shadow.x - shR * 0.7f, shadow.y - shR * 0.17f, shR * 1.4f, shR * 0.34f);
        g.setColour (cyan().withAlpha (0.6f * glowA)); g.drawEllipse (shadow.x - shR, shadow.y - shR * 0.25f, shR * 2, shR * 0.5f, 1.2f);
        g.setColour (cyan().withAlpha (0.2f)); g.drawLine (Line<float> (shadow, orb), 1.0f);
        const float refl = jlimit (0.15f, 1.0f, 0.25f + room * 0.4f + wet * 3.0f) * glowA;
        const float vOrb = 0.5f + 0.4f * oh;
        const Point<float> hits[] { G.P (-1, vOrb, od), G.P (1, vOrb, od), G.P (ox, vOrb, 1), G.P (ox, 1, od) };
        for (auto h : hits)
        {
            Path ln; ln.startNewSubPath (orb); ln.lineTo (h);
            Path dashed; const float dash[] { 6.0f, 7.0f }; PathStrokeType (1.0f).createDashedStroke (dashed, ln, dash, 2);
            g.setColour (magenta().withAlpha (0.45f * refl)); g.fillPath (dashed);
            const float hr = 5.0f + 9.0f * refl * (0.5f + 0.5f * std::sin (phase * 3.0f + h.x * 0.01f));
            g.setGradientFill (ColourGradient (magenta().withAlpha (0.8f * refl), h.x, h.y, magenta().withAlpha (0.0f), h.x + hr, h.y, true));
            g.fillEllipse (h.x - hr, h.y - hr, hr * 2, hr * 2);
        }
        // waves leaving the orb
        for (int k = 0; k < 4; ++k)
        {
            const float u = std::fmod (phase * 0.5f + (float) k * 0.25f, 1.0f), rr = rad * (1.2f + 3.5f * u);
            g.setColour (cyan().withAlpha ((0.35f + 0.5f * pulse) * (1.0f - u) * glowA)); g.drawEllipse (orb.x - rr, orb.y - rr, rr * 2, rr * 2, 1.5f);
        }
        const float halo = rad * (2.4f + 1.6f * pulse);
        g.setGradientFill (ColourGradient (cyan().withAlpha ((0.35f + 0.45f * pulse) * glowA), orb.x, orb.y, cyan().withAlpha (0.0f), orb.x + halo, orb.y, true));
        g.fillEllipse (orb.x - halo, orb.y - halo, halo * 2, halo * 2);
        const float core = rad * (1.0f + 0.12f * pulse);
        g.setGradientFill (ColourGradient (Colours::white, orb.x - core * 0.3f, orb.y - core * 0.35f, cyan().interpolatedWith (magenta(), jlimit (0.0f, 1.0f, 0.5f - 0.5f * oh)).withAlpha (0.95f), orb.x + core, orb.y + core, true));
        g.fillEllipse (orb.x - core, orb.y - core, core * 2, core * 2);
        g.setColour (Colours::white.withAlpha (0.8f)); g.drawEllipse (orb.x - core, orb.y - core, core * 2, core * 2, 1.5f);
        if (dragMode != dmNone && dragMode != dmRoom) { g.setColour (Colours::white.withAlpha (0.5f)); g.drawEllipse (orb.x - core - 8, orb.y - core - 8, core * 2 + 16, core * 2 + 16, 1.0f); }
        // where it is, in words
        g.setColour (Colours::white.withAlpha (0.85f)); g.setFont (kk::modern::font (15.0f, true, 0.25f));
        g.drawText (roomWord (room), A.reduced (24, 14).removeFromTop (22), Justification::centred);
        g.setColour (cyan().withAlpha (0.85f)); g.setFont (kk::modern::font (13.0f, true, 0.12f));
        g.drawText (placeWords (ox, od, oh) + (on ? "" : "    (off - touch the orb)"), A.reduced (24, 14).withTrimmedTop (24).removeFromTop (20), Justification::centred);
        g.restoreState();
        g.setColour (cyan().withAlpha (0.35f)); g.drawRoundedRectangle (A, 18, 1.2f);
        (void) t;
        touchHints (g, hintRow(), { "DRAG THE ORB = where it sits", "RIGHT-DRAG = up: airy, down: heavy", "DRAG A CORNER = the size of the room", "DOUBLE-CLICK = back to the centre" }, cyan());
    }
    void resized() override { onBtn.setBounds (300, 56, 70, 32); }
    void mouseMove (const MouseEvent& e) override { const bool c = nearCorner (e.position); if (c != cornerHot) { cornerHot = c; repaint(); } setMouseCursor (c || nearOrb (e.position) ? MouseCursor::DraggingHandCursor : MouseCursor::NormalCursor); }
    void mouseDown (const MouseEvent& e) override
    {
        auto& st = proc.holoroom;
        if (! area().toFloat().contains (e.position)) return;
        dragMode = dmNone;
        if (nearOrb (e.position))
        {
            dragMode = e.mods.isPopupMenu() || e.mods.isShiftDown() ? dmHeight : dmMove;
            const Geo G = geo (st.room.load());
            grabOffset = G.P (st.x.load(), 0, st.depth.load()) - e.position;
            startHeight = st.height.load();
        }
        else if (nearCorner (e.position)) dragMode = dmRoom;
        else if (! e.mods.isPopupMenu()) { dragMode = dmMove; grabOffset = {}; }
        else { dragMode = dmHeight; startHeight = st.height.load(); }
        st.on = true; refresh();
        mouseDrag (e);
    }
    void mouseDrag (const MouseEvent& e) override
    {
        auto& st = proc.holoroom;
        const auto A = area().toFloat();
        if (dragMode == dmMove)
        {
            const Geo G = geo (st.room.load());
            const auto p = e.position + grabOffset;
            const float z = G.depthAtFloorY (p.y);
            const auto c = G.rectAt (z);
            st.x = jlimit (-1.0f, 1.0f, (p.x - c.getCentreX()) / (c.getWidth() * 0.5f)); st.depth = z;
        }
        else if (dragMode == dmHeight)
        {
            const Geo G = geo (st.room.load());
            st.height = jlimit (-1.0f, 1.0f, startHeight - (float) e.getDistanceFromDragStartY() / (G.rectAt (st.depth.load()).getHeight() * 0.4f));
        }
        else if (dragMode == dmRoom)
        {
            const float ux = std::abs (e.position.x - A.getCentreX()) / (A.getWidth() * 0.5f), uy = std::abs (e.position.y - A.getCentreY()) / (A.getHeight() * 0.5f);
            st.room = jlimit (0.0f, 1.0f, std::max ((ux - 0.55f) / 0.42f, (uy - 0.7f) / 0.28f));
        }
        repaint();
    }
    void mouseUp (const MouseEvent&) override { dragMode = dmNone; repaint(); }
    void mouseDoubleClick (const MouseEvent& e) override { if (nearOrb (e.position) || area().contains (e.getPosition())) { proc.holoroom.centre(); repaint(); } }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        if (! nearOrb (e.position)) return;
        proc.holoroom.height = jlimit (-1.0f, 1.0f, proc.holoroom.height.load() + w.deltaY * 0.5f); proc.holoroom.on = true; refresh();
    }
private:
    enum DragMode { dmNone, dmMove, dmHeight, dmRoom };
    // the room in perspective: u -1..1 across, v 0..1 floor..ceiling, z 0..1 front..back
    struct Geo
    {
        Rectangle<float> F, B; float k = 1, ratio = 0.5f;
        Rectangle<float> rectAt (float z) const
        {
            const float s = 1.0f / (1.0f + k * z), tt = (1.0f - s) / (1.0f - ratio);
            const float cx = F.getCentreX() + (B.getCentreX() - F.getCentreX()) * tt, cy = F.getCentreY() + (B.getCentreY() - F.getCentreY()) * tt;
            return Rectangle<float> (F.getWidth() * s, F.getHeight() * s).withCentre ({ cx, cy });
        }
        float scale (float z) const { return 1.0f / (1.0f + k * z); }
        Point<float> P (float u, float v, float z) const { const auto r = rectAt (z); return { r.getCentreX() + u * r.getWidth() * 0.5f, r.getBottom() - v * r.getHeight() }; }
        float depthAtFloorY (float y) const
        {
            if (y >= rectAt (0).getBottom()) return 0.0f;
            if (y <= rectAt (1).getBottom()) return 1.0f;
            float lo = 0, hi = 1;
            for (int i = 0; i < 24; ++i) { const float m = 0.5f * (lo + hi); if (rectAt (m).getBottom() > y) lo = m; else hi = m; }
            return 0.5f * (lo + hi);
        }
    };
    Rectangle<int> area() const { return { 24, 100, getWidth() - 48, getHeight() - 168 }; }
    Rectangle<float> hintRow() const { return { 24.0f, (float) getHeight() - 54.0f, (float) getWidth() - 48.0f, 36.0f }; }
    Geo geo (float room) const
    {
        const auto A = area().toFloat();
        Geo G;
        G.F = Rectangle<float> (A.getWidth() * (0.58f + 0.4f * room), A.getHeight() * (0.72f + 0.27f * room)).withCentre (A.getCentre().translated (0, A.getHeight() * 0.02f));
        G.ratio = 0.52f - 0.14f * room;
        G.B = Rectangle<float> (G.F.getWidth() * G.ratio, G.F.getHeight() * G.ratio).withCentre (G.F.getCentre().translated (0, -G.F.getHeight() * 0.07f));
        G.k = 1.0f / G.ratio - 1.0f;
        return G;
    }
    bool nearOrb (Point<float> p) const
    {
        const auto& st = proc.holoroom; const Geo G = geo (st.room.load());
        return p.getDistanceFrom (G.P (st.x.load(), 0.5f + 0.4f * st.height.load(), st.depth.load())) < 40.0f * G.scale (st.depth.load()) * 1.7f + 6.0f;
    }
    bool nearCorner (Point<float> p) const
    {
        const Geo G = geo (proc.holoroom.room.load());
        for (auto c : { G.P (-1, 0, 0), G.P (1, 0, 0), G.P (-1, 1, 0), G.P (1, 1, 0) }) if (p.getDistanceFrom (c) < 22.0f) return true;
        return false;
    }
    static String roomWord (float r) { return r < 0.15f ? "A SMALL BOOTH" : r < 0.35f ? "A STUDIO ROOM" : r < 0.55f ? "A CLUB" : r < 0.8f ? "A CONCERT HALL" : "A CATHEDRAL"; }
    static String placeWords (float x, float d, float h)
    {
        const String side = std::abs (x) < 0.12f ? "in the middle" : x < -0.7f ? "hard left" : x < 0 ? "to the left" : x > 0.7f ? "hard right" : "to the right";
        const String dist = d < 0.12f ? "right in your face" : d < 0.35f ? "close and present" : d < 0.65f ? "across the room" : d < 0.88f ? "far away" : "at the back wall";
        const String up = h > 0.35f ? "  .  up in the air (bright)" : h < -0.35f ? "  .  down low (heavy)" : "";
        return dist + "  .  " + side + up;
    }
    void refresh() { onBtn.selected = proc.holoroom.on.load(); onBtn.setButtonText (proc.holoroom.on.load() ? "ON" : "OFF"); onBtn.repaint(); repaint(); }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f;
        level += (proc.holoroom.mLevel.load() - level) * 0.35f; wet += (proc.holoroom.mWet.load() - wet) * 0.25f;
        repaint (area());
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf };
    DragMode dragMode = dmNone; Point<float> grabOffset; float startHeight = 0;
    float level = 0, wet = 0, phase = 0; bool cornerHot = false;
};

// ---------------------------------------------------------------- GRAB ----------------------------------------------------------------
class GrabPage : public Component, public SettableTooltipClient, private Timer
{
public:
    static constexpr int numBands = 64;
    GrabPage (KeysKillaProcessor& p, KKLookAndFeel& l) : proc (p), lnf (l)
    {
        onBtn.setButtonText ("OFF"); onBtn.framed = true; addAndMakeVisible (onBtn);
        onBtn.setTooltip ("GRAB on / off - the spectrum is matter in your hand");
        onBtn.onClick = [this] { proc.grab.on = ! proc.grab.on.load(); refresh(); };
        clearBtn.setButtonText ("LET GO"); clearBtn.framed = true; addAndMakeVisible (clearBtn);
        clearBtn.setTooltip ("Release every grip - the sound springs back");
        clearBtn.onClick = [this] { proc.grab.releaseAll(); refresh(); };
        setTooltip ("Grab the ribbon: drag up = pull it towards you (louder), down = push it back (quieter), sideways = move along the spectrum.  "
                    "Right-drag a grip up = SQUEEZE (the band is compressed).  Shake while holding = TEAR (it saturates; hold still to mend).  "
                    "Wheel over a grip = narrower / wider.  Double-click a grip = let it go.");
        for (auto& b : bands) b = -100.0f;
        startTimerHz (30);
        refresh();
    }
    static Colour hue (float u)   // low = hot red .. high = cool violet
    {
        static const uint32 c[] { 0xffff3b5c, 0xffff8a3d, 0xffffd23f, 0xff36ff9a, 0xff3ee8ff, 0xffa78bfa };
        const float x = jlimit (0.0f, 4.999f, u * 5.0f); const int i = (int) x;
        return Colour (c[i]).interpolatedWith (Colour (c[i + 1]), x - (float) i);
    }
    void paint (Graphics& g) override
    {
        const auto& t = kk::theme();
        const Colour col (0xffff6a3d);
        proHeader (g, *this, "GRAB", "TOUCH", "the spectrum is matter - grab it.  pull it towards you, push it away, squeeze it, tear it", col);
        if (! analysed) analyse();
        const auto A = area().toFloat();
        const bool on = proc.grab.on.load();
        g.setGradientFill (ColourGradient (Colour (0xff0b0712), A.getCentreX(), A.getCentreY(), Colour (0xff020105), A.getX(), A.getY(), true));
        g.fillRoundedRectangle (A, 18);
        g.saveState(); g.reduceClipRegion (A.toNearestInt());
        // depth lines: towards you (up) / away (down)
        for (int k = -3; k <= 3; ++k)
        {
            const float y = A.getCentreY() - (float) k * A.getHeight() * 0.11f;
            g.setColour (Colours::white.withAlpha (k == 0 ? 0.08f : 0.035f)); g.fillRect (A.getX(), y, A.getWidth(), 1.0f);
        }
        static const char* regions[] { "SUB", "BASS", "LOW MIDS", "MIDS", "PRESENCE", "AIR" };
        static const float edges[] { 30, 60, 250, 800, 3000, 8000, 18000 };
        for (int r = 0; r < 6; ++r)
        {
            const float x0 = xOf (edges[r]), x1 = xOf (edges[r + 1]);
            if (r > 0) { g.setColour (Colours::white.withAlpha (0.05f)); g.fillRect (x0, A.getY(), 1.0f, A.getHeight()); }
            g.setColour (Colours::white.withAlpha (0.32f)); g.setFont (kk::modern::font (11.0f, true, 0.3f));
            g.drawText (regions[r], Rectangle<float> (x0, A.getBottom() - 26, x1 - x0, 16), Justification::centred);
        }
        // the ribbon: centre line and thickness from the live spectrum + every grip's hand
        const int steps = 220;
        std::vector<float> cy ((size_t) steps + 1), th ((size_t) steps + 1), sq ((size_t) steps + 1), tr ((size_t) steps + 1);
        for (int s = 0; s <= steps; ++s)
        {
            const float u = (float) s / (float) steps, hz = hzOf (u);
            const float db = bandAt (u);
            float half = 7.0f + A.getHeight() * 0.27f * jlimit (0.0f, 1.0f, (db + 78.0f) / 66.0f);
            float y = A.getCentreY() + std::sin (u * 17.0f + phase * 1.3f) * 3.0f + std::sin (u * 41.0f - phase * 2.1f) * 1.5f;
            float squeeze = 0, tear = 0;
            for (int k = 0; k < kk::pro::GrabState::numGrips; ++k)
            {
                const auto& gp = proc.grab.grips[(size_t) k];
                if (! gp.active.load()) continue;
                const float oct = std::log2 (hz / gp.freq.load()), w = 1.1f / jmax (0.3f, gp.q.load());
                const float shape = std::exp (-(oct * oct) / (2.0f * w * w * 0.25f));
                const float gdb = gp.gainDb.load();
                y -= gdb * A.getHeight() * 0.014f * shape;
                half *= std::pow (10.0f, gdb / 40.0f * shape);
                squeeze = jmax (squeeze, gp.squeeze.load() * shape); tear = jmax (tear, gp.tear.load() * shape);
            }
            half *= 1.0f - 0.55f * squeeze;
            cy[(size_t) s] = y; th[(size_t) s] = half; sq[(size_t) s] = squeeze; tr[(size_t) s] = tear;
        }
        Path ribbon, top, bot;
        for (int s = 0; s <= steps; ++s) { const float x = A.getX() + A.getWidth() * (float) s / (float) steps; if (s == 0) top.startNewSubPath (x, cy[0] - th[0]); else top.lineTo (x, cy[(size_t) s] - th[(size_t) s]); }
        ribbon = top;
        for (int s = steps; s >= 0; --s) { const float x = A.getX() + A.getWidth() * (float) s / (float) steps; ribbon.lineTo (x, cy[(size_t) s] + th[(size_t) s]); if (s == steps) bot.startNewSubPath (x, cy[(size_t) s] + th[(size_t) s]); else bot.lineTo (x, cy[(size_t) s] + th[(size_t) s]); }
        ribbon.closeSubPath();
        {
            ColourGradient cg (hue (0.0f).withAlpha (on ? 0.55f : 0.3f), A.getX(), 0, hue (1.0f).withAlpha (on ? 0.55f : 0.3f), A.getRight(), 0, false);
            for (int k = 1; k < 6; ++k) cg.addColour ((double) k / 6.0, hue ((float) k / 6.0f).withAlpha (on ? 0.55f : 0.3f));
            g.setGradientFill (cg); g.fillPath (ribbon);
            g.setColour (Colours::white.withAlpha (0.06f)); g.strokePath (ribbon, PathStrokeType (14.0f));
        }
        // the living filaments inside the matter
        for (int f = 0; f < 9; ++f)
        {
            const float off = -0.85f + 1.7f * (float) f / 8.0f;
            Path fil;
            for (int s = 0; s <= steps; s += 2)
            {
                const float u = (float) s / (float) steps, x = A.getX() + A.getWidth() * u;
                const float wob = std::sin (u * (9.0f + (float) f) + phase * (0.8f + 0.15f * (float) f) + (float) f) * 0.12f;
                const float y = cy[(size_t) s] + th[(size_t) s] * jlimit (-0.95f, 0.95f, off + wob) * (1.0f - 0.4f * sq[(size_t) s]);
                if (s == 0) fil.startNewSubPath (x, y); else fil.lineTo (x, y);
            }
            g.setColour (Colours::white.withAlpha (0.10f + 0.05f * (float) (f % 3))); g.strokePath (fil, PathStrokeType (1.0f));
        }
        g.setColour (Colours::white.withAlpha (0.75f)); g.strokePath (top, PathStrokeType (1.6f)); g.strokePath (bot, PathStrokeType (1.6f));
        // squeeze: tight rings around the matter / tear: frayed strands and sparks
        Random rr (77 + (int) (phase * 6.0f));
        for (int s = 0; s <= steps; s += 3)
        {
            const float x = A.getX() + A.getWidth() * (float) s / (float) steps;
            if (sq[(size_t) s] > 0.05f) { g.setColour (Colour (0xffffe08a).withAlpha (0.55f * sq[(size_t) s])); g.drawLine (x, cy[(size_t) s] - th[(size_t) s] - 4, x, cy[(size_t) s] + th[(size_t) s] + 4, 1.4f); }
            if (tr[(size_t) s] > 0.05f)
                for (int k = 0; k < 2; ++k)
                {
                    const float side = rr.nextBool() ? -1.0f : 1.0f, y0 = cy[(size_t) s] + side * th[(size_t) s];
                    const float len = (6.0f + 26.0f * rr.nextFloat()) * tr[(size_t) s];
                    const Point<float> a (x, y0), b (x + (rr.nextFloat() - 0.5f) * len, y0 + side * len);
                    g.setColour (Colour (0xffff3b5c).withAlpha (0.7f * tr[(size_t) s])); g.drawLine (Line<float> (a, b), 1.1f);
                    if (rr.nextFloat() < 0.3f) { g.setColour (Colours::white.withAlpha (0.8f * tr[(size_t) s])); g.fillEllipse (b.x - 1.5f, b.y - 1.5f, 3, 3); }
                }
        }
        // the grips: glowing finger-marks
        for (int k = 0; k < kk::pro::GrabState::numGrips; ++k)
        {
            const auto& gp = proc.grab.grips[(size_t) k];
            if (! gp.active.load()) continue;
            const auto c = gripPos (k);
            const float r = gripRadius (k), sqz = gp.squeeze.load(), tear = gp.tear.load();
            const Colour gc = gp.gainDb.load() >= 0 ? Colour (0xffffd23f) : Colour (0xff3ee8ff);
            const bool hot = k == active;
            g.setGradientFill (ColourGradient (gc.withAlpha (hot ? 0.55f : 0.35f), c.x, c.y, gc.withAlpha (0.0f), c.x + r * 2.2f, c.y, true));
            g.fillEllipse (c.x - r * 2.2f, c.y - r * 2.2f, r * 4.4f, r * 4.4f);
            for (int ring = 0; ring < 5; ++ring)   // the fingerprint
            {
                const float rr2 = r * (0.25f + 0.17f * (float) ring);
                Path arc; arc.addCentredArc (c.x, c.y, rr2 * 1.1f, rr2, 0.15f * (float) ring, 0.6f + 0.3f * (float) ring, MathConstants<float>::twoPi - 0.4f + 0.2f * (float) ring, true);
                g.setColour (Colours::white.withAlpha (0.8f - 0.1f * (float) ring)); g.strokePath (arc, PathStrokeType (1.3f));
            }
            g.setColour (gc.withAlpha (0.9f)); g.drawEllipse (c.x - r, c.y - r, r * 2, r * 2, hot ? 2.4f : 1.6f);
            if (sqz > 0.02f)   // brackets pressing in
                for (int side = -1; side <= 1; side += 2)
                {
                    const float x = c.x + (float) side * (r + 8.0f - 4.0f * sqz);
                    Path br; br.startNewSubPath (x - (float) side * 6.0f, c.y - r * 0.8f); br.lineTo (x, c.y - r * 0.8f); br.lineTo (x, c.y + r * 0.8f); br.lineTo (x - (float) side * 6.0f, c.y + r * 0.8f);
                    g.setColour (Colour (0xffffe08a).withAlpha (0.4f + 0.6f * sqz)); g.strokePath (br, PathStrokeType (1.5f + 2.0f * sqz));
                }
            if (tear > 0.02f)
                for (int s = 0; s < 8; ++s)
                {
                    const float a = (float) s * 0.785f + phase * 2.0f, l2 = r * (1.2f + 0.8f * tear * rr.nextFloat());
                    g.setColour (Colour (0xffff3b5c).withAlpha (0.8f * tear)); g.drawLine (c.x + std::cos (a) * r, c.y + std::sin (a) * r, c.x + std::cos (a) * l2, c.y + std::sin (a) * l2, 1.5f);
                }
        }
        if (countGrips() == 0)
        {
            g.setColour (Colours::white.withAlpha (0.55f)); g.setFont (kk::modern::font (16.0f, true, 0.2f));
            g.drawText ("GRAB THE RIBBON ANYWHERE", A.withTrimmedTop (A.getHeight() * 0.12f).withHeight (24), Justification::centred);
        }
        g.restoreState();
        g.setColour (col.withAlpha (0.35f)); g.drawRoundedRectangle (A, 18, 1.2f);
        for (int k = 0; k < kk::pro::GrabState::numGrips; ++k)   // free fingers: six dots
        {
            const bool used = k < countGrips();
            g.setColour (used ? Colour (0xffffd23f) : t.dim.withAlpha (0.35f)); g.fillEllipse (476.0f + (float) k * 16.0f, 66.0f, 10.0f, 10.0f);
        }
        touchHints (g, hintRow(), { "DRAG UP = louder", "DRAG DOWN = quieter", "RIGHT-DRAG = squeeze", "SHAKE IT = tear", "WHEEL = width", "DOUBLE-CLICK = let go" }, col);
    }
    void resized() override { onBtn.setBounds (300, 56, 70, 32); clearBtn.setBounds (376, 56, 84, 32); }
    void mouseMove (const MouseEvent& e) override { const int h = gripAt (e.position); if (h != active) { active = h; repaint(); } }
    void mouseExit (const MouseEvent&) override { if (dragging < 0 && active >= 0) { active = -1; repaint(); } }
    void mouseDown (const MouseEvent& e) override
    {
        const auto A = area().toFloat();
        if (! A.contains (e.position)) return;
        int k = gripAt (e.position);
        squeezing = e.mods.isPopupMenu();
        if (k < 0)
        {
            if (squeezing) return;
            k = proc.grab.add (hzOf ((e.position.x - A.getX()) / A.getWidth()));
            if (k < 0) return;
        }
        dragging = active = k;
        auto& gp = proc.grab.grips[(size_t) k];
        startGain = gp.gainDb.load(); startSqueeze = gp.squeeze.load(); startHz = gp.freq.load();
        lastX = e.position.x; lastDir = 0; reversals.clear(); stillSince = Time::getMillisecondCounterHiRes(); lastPos = e.position;
        proc.grab.on = true; refresh();
    }
    void mouseDrag (const MouseEvent& e) override
    {
        if (dragging < 0) return;
        auto& gp = proc.grab.grips[(size_t) dragging];
        const auto A = area().toFloat();
        const double now = Time::getMillisecondCounterHiRes();
        if (squeezing) { gp.squeeze = jlimit (0.0f, 1.0f, startSqueeze - (float) e.getDistanceFromDragStartY() / (A.getHeight() * 0.35f)); repaint(); return; }
        gp.gainDb = jlimit (-24.0f, 24.0f, startGain - (float) e.getDistanceFromDragStartY() / (A.getHeight() * 0.014f));
        // TEAR: fast left-right reversals while holding
        const float dx = e.position.x - lastX;
        if (std::abs (dx) > 2.0f)
        {
            const int dir = dx > 0 ? 1 : -1;
            if (lastDir != 0 && dir != lastDir) { reversals.push_back (now); if (reversals.size() == 1) shakeHz = gp.freq.load(); }
            lastDir = dir; lastX = e.position.x;
        }
        while (! reversals.empty() && now - reversals.front() > 450.0) reversals.erase (reversals.begin());
        if (reversals.size() >= 2) { gp.tear = jmin (1.0f, gp.tear.load() + 0.035f); gp.freq = shakeHz; }   // shaking: the grip stays where it is
        else gp.freq = jlimit (kk::pro::GrabState::minHz, kk::pro::GrabState::maxHz, startHz * std::pow (kk::pro::GrabState::maxHz / kk::pro::GrabState::minHz, (float) e.getDistanceFromDragStartX() / A.getWidth()));
        if (e.position.getDistanceFrom (lastPos) > 3.0f) { stillSince = now; lastPos = e.position; }
        repaint();
    }
    void mouseUp (const MouseEvent&) override { dragging = -1; squeezing = false; repaint(); }
    void mouseDoubleClick (const MouseEvent& e) override
    {
        const int k = gripAt (e.position);
        if (k >= 0) { auto& gp = proc.grab.grips[(size_t) k]; gp.active = false; gp.gainDb = 0; gp.squeeze = 0; gp.tear = 0; dragging = active = -1; repaint(); }
    }
    void mouseWheelMove (const MouseEvent& e, const MouseWheelDetails& w) override
    {
        const int k = gripAt (e.position); if (k < 0) return;
        auto& gp = proc.grab.grips[(size_t) k];
        gp.q = jlimit (0.3f, 10.0f, gp.q.load() * std::exp2 (w.deltaY * 2.0f));
        repaint();
    }
private:
    Rectangle<int> area() const { return { 24, 100, getWidth() - 48, getHeight() - 168 }; }
    Rectangle<float> hintRow() const { return { 24.0f, (float) getHeight() - 54.0f, (float) getWidth() - 48.0f, 36.0f }; }
    static float hzOf (float u) { return kk::pro::GrabState::minHz * std::pow (kk::pro::GrabState::maxHz / kk::pro::GrabState::minHz, jlimit (0.0f, 1.0f, u)); }
    float xOf (float hz) const { const auto A = area().toFloat(); return A.getX() + A.getWidth() * std::log (hz / kk::pro::GrabState::minHz) / std::log (kk::pro::GrabState::maxHz / kk::pro::GrabState::minHz); }
    float bandAt (float u) const { const float b = jlimit (0.0f, (float) numBands - 1.001f, u * (float) (numBands - 1)); const int i = (int) b; return bands[(size_t) i] + (bands[(size_t) i + 1] - bands[(size_t) i]) * (b - (float) i); }
    Point<float> gripPos (int k) const
    {
        const auto A = area().toFloat(); const auto& gp = proc.grab.grips[(size_t) k];
        return { xOf (gp.freq.load()), A.getCentreY() - gp.gainDb.load() * A.getHeight() * 0.014f };
    }
    float gripRadius (int k) const { return jlimit (14.0f, 46.0f, 30.0f / std::sqrt (proc.grab.grips[(size_t) k].q.load() / 1.4f)); }
    int gripAt (Point<float> p) const
    {
        int best = -1; float bd = 1.0e9f;
        for (int k = 0; k < kk::pro::GrabState::numGrips; ++k)
            if (proc.grab.grips[(size_t) k].active.load()) { const float d = p.getDistanceFrom (gripPos (k)); if (d < gripRadius (k) + 10.0f && d < bd) { bd = d; best = k; } }
        return best;
    }
    int countGrips() const { int n = 0; for (auto& gp : proc.grab.grips) n += gp.active.load() ? 1 : 0; return n; }
    // the live spectrum: the last 2048 mono samples -> 64 log bands (dB)
    void analyse()
    {
        analysed = true;
        const int w = proc.grab.ringW.load (std::memory_order_acquire);
        constexpr int N = 2048;
        std::fill (fftData.begin(), fftData.end(), 0.0f);
        for (int i = 0; i < N; ++i)
        {
            const float win = 0.5f - 0.5f * std::cos (MathConstants<float>::twoPi * (float) i / (float) (N - 1));
            fftData[(size_t) i] = proc.grab.ring[(size_t) ((w - N + i) & (kk::pro::GrabState::ringSize - 1))].load (std::memory_order_relaxed) * win;
        }
        magnitudes (fftData.data(), N);
        const float binHz = (float) (proc.getSampleRate() > 0 ? proc.getSampleRate() : 44100.0) / (float) N;
        for (int b = 0; b < numBands; ++b)
        {
            const float f0 = hzOf ((float) b / (float) numBands), f1 = hzOf ((float) (b + 1) / (float) numBands);
            const int i0 = jlimit (1, N / 2 - 1, (int) (f0 / binHz)), i1 = jlimit (i0, N / 2 - 1, (int) (f1 / binHz));
            float pw = 0; for (int i = i0; i <= i1; ++i) pw = jmax (pw, fftData[(size_t) i]);
            const float db = 20.0f * std::log10 (pw / (float) N * 4.0f + 1.0e-6f);
            bands[(size_t) b] = db > bands[(size_t) b] ? bands[(size_t) b] + (db - bands[(size_t) b]) * 0.6f : bands[(size_t) b] + (db - bands[(size_t) b]) * 0.15f;
        }
    }
    // a small in-place radix-2 FFT: the first n values (real) -> their magnitudes in [0, n/2]
    static void magnitudes (float* x, int n)
    {
        std::vector<std::complex<float>> c ((size_t) n);
        for (int i = 0; i < n; ++i) c[(size_t) i] = x[i];
        for (int i = 1, j = 0; i < n; ++i) { int bit = n >> 1; for (; j & bit; bit >>= 1) j ^= bit; j ^= bit; if (i < j) std::swap (c[(size_t) i], c[(size_t) j]); }
        for (int len = 2; len <= n; len <<= 1)
        {
            const float ang = -MathConstants<float>::twoPi / (float) len; const std::complex<float> wl (std::cos (ang), std::sin (ang));
            for (int i = 0; i < n; i += len) { std::complex<float> w (1.0f, 0.0f); for (int k = 0; k < len / 2; ++k) { const auto u = c[(size_t) (i + k)], v = c[(size_t) (i + k + len / 2)] * w; c[(size_t) (i + k)] = u + v; c[(size_t) (i + k + len / 2)] = u - v; w *= wl; } }
        }
        for (int i = 0; i <= n / 2; ++i) x[i] = std::abs (c[(size_t) i]);
    }
    void refresh()
    {
        onBtn.selected = proc.grab.on.load(); onBtn.setButtonText (proc.grab.on.load() ? "ON" : "OFF"); onBtn.repaint();
        repaint();
    }
    void timerCallback() override
    {
        if (! isShowing()) return;
        phase += 0.05f;
        analyse();
        // holding a grip still mends its tear
        if (dragging >= 0 && ! squeezing && Time::getMillisecondCounterHiRes() - stillSince > 1200.0)
        {
            auto& gp = proc.grab.grips[(size_t) dragging];
            gp.tear = jmax (0.0f, gp.tear.load() - 0.02f);
        }
        repaint();
    }
    KeysKillaProcessor& proc; KKLookAndFeel& lnf;
    HotButton onBtn { lnf }, clearBtn { lnf };
    std::array<float, 4096> fftData {};
    std::array<float, numBands> bands {};
    bool analysed = false, squeezing = false;
    int dragging = -1, active = -1, lastDir = 0;
    float startGain = 0, startSqueeze = 0, startHz = 1000, lastX = 0, shakeHz = 1000, phase = 0;
    double stillSince = 0; Point<float> lastPos;
    std::vector<double> reversals;
};
